#include "App_Freertos.h"
// --- 1. 定义数据结构体 (非常关键) ---
typedef struct {
    float mq2;
    float tvoc;
    float ch2o;
    float co2;
    int8_t temp;
    int8_t humid;
} SensorData_t;

extern float mq2_value;
extern float ch2o;
extern float tvoc;
extern float co2;
extern int8_t temp;
extern int8_t humid;
extern uint8_t g_is_at_mode;

// --- 3. 操作系统句柄定义 ---
TaskHandle_t xHandle_Acquisition = NULL;
TaskHandle_t xHandle_DispAlmUp   = NULL;
TaskHandle_t xHandle_CloudCmd    = NULL;

QueueHandle_t     xQueue_SensorData = NULL; // 传感器数据队列
SemaphoreHandle_t xSem_UART_RX      = NULL; // 串口接收完成信号量
SemaphoreHandle_t xMutex_UART_TX    = NULL; // 保护ESP32串口发送
SemaphoreHandle_t xMutex_Threshold  = NULL; // 保护报警阈值
SemaphoreHandle_t xMutex_SensorData = NULL;




// --- 私有函数声明 ---
void vTask_DataAcquisition(void *pvParameters);
void vTask_DisplayAlarmUpload(void *pvParameters);
void vTask_CloudCommand(void *pvParameters);

uint8_t ESP32_OneNET_Post_Sensors_Data(const SensorData_t *rxData);
uint8_t ESP32_OneNET_Reply_Get(char *request_id, const SensorData_t *latestData);
void vTask_VOC301_Reader(void);
void FreeRTOS_Start(void)
{
    // 1. 创建队列 (长度为1即可，因为我们只关心最新数据)
    xQueue_SensorData = xQueueCreate(1, sizeof(SensorData_t));
    
    // 2. 创建信号量与互斥锁
    xSem_UART_RX     = xSemaphoreCreateBinary();
    xMutex_UART_TX   = xSemaphoreCreateMutex();
    xMutex_Threshold = xSemaphoreCreateMutex();

    // 3. 创建任务
    // 采集任务优先级最高，保证实时性
    xTaskCreate(vTask_DataAcquisition, "Acquisition", 512, NULL, 3, &xHandle_Acquisition);
    // 处理任务优先级中等
    xTaskCreate(vTask_DisplayAlarmUpload, "DispAlmUp", 512, NULL, 4, &xHandle_DispAlmUp);
    // 命令解析优先级最低，避免阻塞核心控制逻辑
    xTaskCreate(vTask_CloudCommand, "CloudCmd", 512, NULL, 5, &xHandle_CloudCmd);

    printf(">>> FreeRTOS Scheduler Starting (New Architecture)...\r\n");
    vTaskStartScheduler();
}

//数据采集
// 任务1：数据采集 (生产者)
static void vTask_DataAcquisition(void *pvParameters)
{
    SensorData_t currentData = {0};
    for (;;)
    {

        voc201_read();
        currentData.tvoc = tvoc; 
        currentData.ch2o = ch2o;
        currentData.co2  = co2;


        
        // 2. 读取 MQ2 和 DHT11
        mq2_read();
        currentData.mq2 = mq2_value;

        vTaskSuspendAll();
        humiture_read();
        currentData.temp = temp;
        currentData.humid = humid;
        xTaskResumeAll();


        xQueueOverwrite(xQueue_SensorData, &currentData);

        // 3. 延时，让出 CPU
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}
//数据显示报警以及数据上报
// 任务2：显示、报警与上传 (消费者1)
static void vTask_DisplayAlarmUpload(void *pvParameters)
{
    SensorData_t rxData;
    float cur_th_mq2, cur_th_tvoc, cur_th_ch2o, cur_th_co2;

    for (;;)
    {
        // --- A. 阻塞等待数据 ---
        // portMAX_DELAY 表示死等。只要采集任务不发数据，这个任务就一直挂起，不占 CPU
        if (xQueueReceive(xQueue_SensorData, &rxData, portMAX_DELAY) == pdTRUE)
        {
            // --- B. 立即刷新 OLED 屏幕 ---
            OLED_Show_mq2(rxData.mq2);
            OLED_Show_humiture(rxData.temp, rxData.humid);
            OLED_Show_voc201(rxData.tvoc, rxData.ch2o, (uint16_t)rxData.co2);

            // --- C. 立即判断报警 ---
            // 1. 安全地获取最新的报警阈值 (因为云端指令任务可能会修改它们)
            if (xSemaphoreTake(xMutex_Threshold, portMAX_DELAY) == pdTRUE) 
            {
                cur_th_mq2  = THRESHOLD_MQ2;
                cur_th_tvoc = THRESHOLD_TVOC;
                cur_th_ch2o = THRESHOLD_CH2O;
                cur_th_co2  = THRESHOLD_CO2;
                xSemaphoreGive(xMutex_Threshold);
            }

            // 2. 逻辑判断
            uint8_t isDanger = 0;
            if (rxData.mq2 > cur_th_mq2)   isDanger = 1;
            if (rxData.ch2o > cur_th_ch2o) isDanger = 1;
            if (rxData.tvoc > cur_th_tvoc) isDanger = 1;
            if (rxData.co2 > cur_th_co2)   isDanger = 1;

            // 3. 控制蜂鸣器
            if (isDanger) {
                HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET);
            } else {
                HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET);
            }

            // --- D. 立即上报云平台 ---
            // 申请串口发送锁，防止恰好在这个瞬间，云端下发了指令需要回复，导致串口冲突
            // --- D. 立即上报云平台 ---
            if (xSemaphoreTake(xMutex_UART_TX, pdMS_TO_TICKS(1000)) == pdTRUE) 
            {
                g_is_at_mode = 1; 
                printf("[DispAlmUp] Uploading data to OneNET...\r\n");
                
                // 检查上传是否成功

                if (ESP32_OneNET_Post_Sensors_Data(&rxData) == 1)
                {
                    // 成功
                }
                else
                {
                    // === 重连逻辑 ===
                    printf(">>> [错误] MQTT上传失败，连接可能已断开！\r\n");
                    printf(">>> [重连] 正在执行重连流程...\r\n");
                    
                    // 调用你写好的全套重连函数
                    onenet_init(g_rx_buffer); 
                    
                    // 重连后需要重新开启串口中断接收，防止因为复位丢失中断！
                    HAL_UARTEx_ReceiveToIdle_IT(&huart1, g_rx_buffer, 256);
                }
                g_is_at_mode = 0;
                xSemaphoreGive(xMutex_UART_TX);

            }
        }
    vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
}

// 云平台命令以及返回 (任务3：消费者2)
void vTask_CloudCommand(void *pvParameters)
{
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, g_rx_buffer, 256);
        // 1. 初始化 (此时 g_is_at_mode 内部默认为 0，记得在 onenet_init 开始前设为 1)

    for (;;)
    {
        // 1. 死等信号量（只要串口中断没收完数据，这里就不消耗 CPU）
        if (xSemaphoreTake(xSem_UART_RX, portMAX_DELAY) == pdTRUE)
        {
            // 【新增安全检查】如果正在进行 AT 初始化，跳过这次处理
            // 防止 AT 指令的返回结果被这个任务错误解析
            if (g_is_at_mode == 1) {
                memset(g_rx_buffer, 0, 256);
                continue; 
            }
            printf("\r\n【Cloud RX Event】: %s\r\n", g_rx_buffer);

            // ==========================================
            // A. 解析【设置阈值 SET】指令
            // ==========================================
            if (strstr((char *)g_rx_buffer, "thing/property/set") != NULL)
            {
                printf(">>> 收到云平台【设置阈值】指令！\r\n");

                // 【安全机制】申请互斥锁，修改阈值，防止读写冲突
                if (xSemaphoreTake(xMutex_Threshold, pdMS_TO_TICKS(500)) == pdTRUE)
                {
                    char *p;
                    if ((p = strstr((char *)g_rx_buffer, "\"mq2_value\":")) != NULL) THRESHOLD_MQ2 = atof(p + 12);
                    if ((p = strstr((char *)g_rx_buffer, "\"ch2o\":")) != NULL) THRESHOLD_CH2O = atof(p + 7);
                    if ((p = strstr((char *)g_rx_buffer, "\"co2\":")) != NULL) THRESHOLD_CO2 = atof(p + 6);
                    if ((p = strstr((char *)g_rx_buffer, "\"tvoc\":")) != NULL) THRESHOLD_TVOC = atof(p + 7);
                    
                    printf(">>> 新阈值 -> MQ2:%.2f, CH2O:%.2f, CO2:%.2f, TVOC:%.2f\r\n", 
                            THRESHOLD_MQ2, THRESHOLD_CH2O, THRESHOLD_CO2, THRESHOLD_TVOC);
                            
                    xSemaphoreGive(xMutex_Threshold); // 修改完毕，释放锁
                }

                // 提取 ID 并回复云平台
                char *id_start = strstr((char *)g_rx_buffer, "\"id\":\"");
                if (id_start != NULL)
                {
                    id_start += 6;
                    char *id_end = strchr(id_start, '\"');
                    if (id_end != NULL)
                    {
                        char request_id[32] = {0};
                        int id_len = id_end - id_start;
                        if (id_len > 31) id_len = 31;
                        strncpy(request_id, id_start, id_len);

                        // 【串口安全机制】申请发送锁，回复云平台
                        if (xSemaphoreTake(xMutex_UART_TX, pdMS_TO_TICKS(1000)) == pdTRUE) {
                            g_is_at_mode = 1;
                            ESP32_OneNET_Reply_Set(request_id, 200);
                            g_is_at_mode = 1;
                            xSemaphoreGive(xMutex_UART_TX);
                            printf(">>> 阈值更新完毕，已回复 ID: %s\r\n", request_id);
                        }
                    }
                }
            }
            // ==========================================
            // B. 解析【获取数据 GET】指令
            // ==========================================
            else if (strstr((char *)g_rx_buffer, "thing/property/get") != NULL)
            {
                printf(">>> 收到云平台【获取数据】请求！\r\n");

                char *id_start = strstr((char *)g_rx_buffer, "\"id\":\"");
                if (id_start != NULL)
                {
                    id_start += 6;
                    char *id_end = strchr(id_start, '\"');
                    if (id_end != NULL)
                    {
                        char request_id[32] = {0};
                        int id_len = id_end - id_start;
                        if (id_len > 31) id_len = 31;
                        strncpy(request_id, id_start, id_len);

                        // 重点来了：直接从队列里“偷瞄”最新数据，不需要去读传感器！
                        SensorData_t latestData = {0};
                        xQueuePeek(xQueue_SensorData, &latestData, 0);

                        // 【串口安全机制】申请发送锁，回复数据
                        if (xSemaphoreTake(xMutex_UART_TX, pdMS_TO_TICKS(1000)) == pdTRUE) {
                            
                            // ?? 注意这里：你需要修改你原本的 ESP32_OneNET_Reply_Get 函数
                            // 让它接收 latestData 结构体指针，就像之前我们修改 POST 函数一样！
                            g_is_at_mode = 1;
                            ESP32_OneNET_Reply_Get(request_id, &latestData);
                            g_is_at_mode = 0;
                            
                            xSemaphoreGive(xMutex_UART_TX);
                            printf(">>> 数据获取成功，已回复 ID: %s\r\n", request_id);
                        }
                    }
                }
            }

            // 安全清空缓冲区，等待下一包
            memset(g_rx_buffer, 0, 256);
        }
    }
}















// 【优化】参数改为结构体指针，加上 const 防止内部误改数据，执行效率更高
uint8_t ESP32_OneNET_Post_Sensors_Data(const SensorData_t *pData)
{
    // 【修改点1】static 节省栈空间。
    // 因为这个函数是在 xMutex_UART_TX 互斥锁内被调用的，所以不用担心多任务抢占导致的线程安全问题！
    static char payload[512];
    char cmd[256];

    const char *topic = "$sys/SVQx377cH4/gassscan/thing/property/post";
    int offset = 0;

    // 1. 拼接头部
    offset += sprintf(payload + offset, "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{");

    // 2. 拼接整数
    offset += sprintf(payload + offset, "\"temp\":{\"value\":%d},", pData->temp);
    offset += sprintf(payload + offset, "\"humid\":{\"value\":%d},", pData->humid);

    // 3. 拼接浮点数
    offset += sprintf(payload + offset, "\"mq2_value\":{\"value\":%.3f},", pData->mq2);
    offset += sprintf(payload + offset, "\"ch2o\":{\"value\":%.3f},", pData->ch2o);
    
    // 【修复 BUG 1】：co2 在结构体中是 float，转为 int 后再用 %d 发送
    offset += sprintf(payload + offset, "\"co2\":{\"value\":%d},", (int)pData->co2);

    // 【修复 BUG 2】：最后一个数据，改回使用传入的参数 pData->tvoc，去掉逗号
    offset += sprintf(payload + offset, "\"tvoc\":{\"value\":%.3f}", pData->tvoc);

    // 4. 拼接尾部
    offset += sprintf(payload + offset, "}}");

    // 打印检查完整性
    printf("【JSON Length】: %d\r\n", offset);
    printf("【JSON Content】: %s\r\n", payload);

    // --- 下面是发送流程 ---
    sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, offset);

    if (ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL) != 1)
    {
        printf("【Err】CMD Head Fail\r\n");
        return 0;
    }

    // 串口发送原始数据 (HAL_UART_Transmit 内部是阻塞的，有超时时间，这在加锁的情况下是允许的)
    HAL_UART_Transmit(&huart1, (uint8_t *)payload, offset, 1000);
    
    // 【修复 BUG 3】：用 vTaskDelay 替换 HAL_Delay，交出 200ms 的 CPU 控制权给别的任务！
    vTaskDelay(pdMS_TO_TICKS(200));

    printf("【Success】Post OK\r\n");
    return 1;
}








// 【注意】参数已经加了 const SensorData_t *latestData
uint8_t ESP32_OneNET_Reply_Get(char *request_id, const SensorData_t *latestData)
{
    static char payload[512]; 
    char cmd[256];     

    // 回复的 Topic 必须是 get_reply
    const char *topic = "$sys/SVQx377cH4/gassscan/thing/property/get_reply";

    int offset = 0;

    // 拼接 JSON 头部
    offset += sprintf(payload + offset, 
        "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{", 
        request_id);

    // 【修改点1】将原来的 temp, humid 等全局变量，全部替换为 latestData->xxx
    offset += sprintf(payload + offset, "\"temp\":%d,", latestData->temp);
    offset += sprintf(payload + offset, "\"humid\":%d,", latestData->humid);
    offset += sprintf(payload + offset, "\"mq2_value\":%.2f,", latestData->mq2);
    offset += sprintf(payload + offset, "\"ch2o\":%.2f,", latestData->ch2o);
    
    // co2 强转为 int
    offset += sprintf(payload + offset, "\"co2\":%d,", (int)latestData->co2);
    
    // 【修改点2】最后一个没有逗号，同样替换为 latestData->tvoc
    offset += sprintf(payload + offset, "\"tvoc\":%.2f", latestData->tvoc);

    // 封尾
    offset += sprintf(payload + offset, "}}");

    printf("【Reply Debug】: %s\r\n", payload);

    // 发送 AT 指令头
    sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, offset);
    
    if (ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL) != 1)
    {
        printf("【Reply Err】指令头发送失败\r\n");
        return 0;
    }

    // 发送 JSON 实体
    HAL_UART_Transmit(&huart1, (uint8_t *)payload, offset, 1000);
    
    // 【修改点3】将 HAL_Delay(200) 替换为 FreeRTOS 的非阻塞延时！
    vTaskDelay(pdMS_TO_TICKS(200)); 
    
    printf("【Reply Success】回复查询成功\r\n");
    return 1;
}






