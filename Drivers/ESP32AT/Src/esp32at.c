#include "esp32at.h"
#include "usart.h"
#include "string.h"
#include "stdio.h"
#include <ctype.h> // 必须包含，用于 isdigit 函数

#define ESP32C3_UART_HANDLE &huart1
#define ESP32C3_RESP_TIMEOUT 3000 

// 引用 main.c 定义的全局变量
extern uint8_t g_rx_buffer[512];
extern volatile uint8_t g_rx_flag;
extern volatile uint16_t g_rx_len;


// 发送AT指令（保持不变）
// 修改 esp32at.c

HAL_StatusTypeDef ESP32C3_AT_SendCmd(const char *cmd)
{
    if (cmd == NULL || ESP32C3_UART_HANDLE == NULL)
        return HAL_ERROR;

    // 1. 打印调试信息 (注意：这里我们手动加\r\n打印，但不存入数组)
    printf("TX: %s\r\n", cmd);

    // 2. 发送指令本体
    // 假设指令很长，我们直接发指针指向的内容，不占用额外栈空间
    if (HAL_UART_Transmit(ESP32C3_UART_HANDLE, (uint8_t *)cmd, strlen(cmd), 1000) != HAL_OK)
    {
        return HAL_ERROR;
    }

    // 3. 紧接着发送回车换行符 "\r\n"
    // AT指令必须以 \r\n 结尾
    return HAL_UART_Transmit(ESP32C3_UART_HANDLE, (uint8_t *)"\r\n", 2, 100);
}

// 【重点修改】发送并等待响应（改为非阻塞等待标志位）
uint8_t ESP32C3_AT_SendCmdAndWaitResp(const char *cmd, const char *target_resp,
                                      uint8_t *resp_buf, uint16_t *resp_len)
{
    // 1. 清除旧的接收标志和缓冲区，防止残留数据干扰
    g_rx_flag = 0;
    memset(g_rx_buffer, 0, 256);

    // 2. 发送指令
    if (ESP32C3_AT_SendCmd(cmd) != HAL_OK)
        return 0;

    // 3. 等待中断回调置位 g_rx_flag
    uint32_t start_time = HAL_GetTick();
    while (g_rx_flag == 0)
    {
        // 超时退出
        if (HAL_GetTick() - start_time > ESP32C3_RESP_TIMEOUT)
        {
            printf("Timeout! No response.\r\n");
            return 0;
        }
        // 这里的 HAL_Delay(1) 可能会影响实时性，但在初始化阶段没问题
        // 如果是高并发场景，这里需要改成查询 tick
        HAL_Delay(1); 
    }

    // 4. 到这里说明数据收到了（中断触发了）
    // 此时 g_rx_buffer 里已经是数据了
    printf("RX: %s\r\n", g_rx_buffer); // 打印接收到的数据

    // 5. 检查是否包含目标字符串 (如 "OK")
    if (strstr((char *)g_rx_buffer, target_resp) != NULL)
    {
        // 复制数据返回给上层（如果需要）
        if (resp_buf != NULL) strcpy((char *)resp_buf, (char *)g_rx_buffer);
        if (resp_len != NULL) *resp_len = g_rx_len;
        return 1; // 成功
    }
    
    return 0; // 收到数据了，但不是我们要的（比如返回了 ERROR）
}

// 重启函数（适配修改）
uint8_t ESP32C3_AT_Reset(uint8_t *resp_buf)
{
    // 发送 RST，等待 ready (时间要长)
    // 这里简单处理，实际上应该改 timeout 参数，这里暂时用 HAL_Delay 模拟
    ESP32C3_AT_SendCmd("AT+RST");
    HAL_Delay(3000); 

    // 必须清一下标志位，因为重启过程可能产生乱码触发中断
    g_rx_flag = 0; 
    
    // 关闭回显
    return ESP32C3_AT_SendCmdAndWaitResp("ATE0", "OK", resp_buf, NULL);
}

// 测试函数（适配修改）
uint8_t ESP32C3_AT_Test(uint8_t *resp_buf)
{
    return ESP32C3_AT_SendCmdAndWaitResp("AT", "OK", resp_buf, NULL);
}

// 设置WiFi模式
uint8_t ESP32C3_AT_SetWiFiMode(uint8_t mode)
{
    char cmd[32] = {0};
    sprintf(cmd, "AT+CWMODE=%d", mode);
    // 这里的 resp_buf 传 NULL，因为我们只关心是否成功返回 OK，不需要提取内容
    return ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL);
}

// 开启STA模式DHCP
uint8_t ESP32C3_AT_EnableSTA_DHCP(void)
{
    // 开启DHCP，确保能自动获取IP
    return ESP32C3_AT_SendCmdAndWaitResp("AT+CWDHCP=1,1", "OK", NULL, NULL);
}

// 连接WiFi（核心修改版）
uint8_t ESP32C3_AT_ConnectWiFi(const char *ssid, const char *pwd)
{
    char cmd[128] = {0};
    uint32_t start_time = HAL_GetTick();
    const uint32_t WIFI_TIMEOUT = 15000; // 15秒超长等待

    // 1. 设置模式
    if (!ESP32C3_AT_SetWiFiMode(1)) {
        printf("Set Mode Failed!\r\n");
        return 0;
    }
    HAL_Delay(500);

    // 2. 开启DHCP
    if (!ESP32C3_AT_EnableSTA_DHCP()) {
        printf("Enable DHCP Failed!\r\n");
    }
    HAL_Delay(500);

    // 3. 发送连接指令
    // 注意：g_rx_flag 在 SendCmd 前会被清零，所以这里手动处理一下
    g_rx_flag = 0;
    memset(g_rx_buffer, 0, 256);
    
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", ssid, pwd ? pwd : "");
    printf("Connecting to %s...\r\n", ssid);
    ESP32C3_AT_SendCmd(cmd); // 只发送，不使用 WaitResp，因为我们要自定义等待逻辑

    // 4. 自定义等待循环（处理多段响应）
    while ((HAL_GetTick() - start_time) < WIFI_TIMEOUT)
    {
        // 如果中断接收到了数据
        if (g_rx_flag == 1)
        {
            printf("WiFi Loop RX: %s\r\n", g_rx_buffer); // 打印当前收到的这一包数据

            // --- 成功判断 ---
            // 只要收到 "WIFI CONNECTED" 或者 "WIFI GOT IP" 都算成功
            if (strstr((char *)g_rx_buffer, "WIFI CONNECTED") || 
                strstr((char *)g_rx_buffer, "WIFI GOT IP") ||
                strstr((char *)g_rx_buffer, "OK")) 
            {
                // 注意：有时候 OK 会比 WIFI CONNECTED 先到或者后到
                // 为了保险，最好检测 WIFI CONNECTED
                if (strstr((char *)g_rx_buffer, "WIFI CONNECTED") || strstr((char *)g_rx_buffer, "WIFI GOT IP")) 
                {
                    printf("WiFi Connected Success!\r\n");
                    return 1;
                }
            }

            // --- 失败判断 ---
            if (strstr((char *)g_rx_buffer, "ERROR") || strstr((char *)g_rx_buffer, "FAIL"))
            {
                printf("WiFi Connect Failed (ERROR).\r\n");
                return 0;
            }

            // --- 关键步骤：清空标志位和缓冲区，准备接收下一行数据 ---
            // 因为 ESP32 是一行一行发的，可能先发 "WIFI DISCONNECT"，再发 "WIFI CONNECTED"
            // 如果不清空，g_rx_flag 一直是 1，死循环就卡住了
            g_rx_flag = 0;
            memset(g_rx_buffer, 0, 256);
            
            // 重新开启中断接收（虽然回调里开启了，但这里确保逻辑闭环）
            // 注意：你的回调函数里必须有 HAL_UARTEx_ReceiveToIdle_IT 才能保证连续接收
        }
        
        HAL_Delay(10); // 短延时，避免死循环占用总线
    }

    printf("WiFi Connect Timeout!\r\n");
    return 0;
}

// 从响应中解析IP地址（不依赖关键字，兼容手机热点）
uint8_t ESP32C3_AT_ParseSTAIP(const char *resp, char *ip_buf)
{
    if (resp == NULL || ip_buf == NULL) return 0;

    const char *p = resp;
    while (*p != '\0')
    {
        if (isdigit((unsigned char)*p))
        {
            char temp_ip[16] = {0};
            int dot_count = 0;
            int i = 0;

            // 提取连续的数字和点
            while (isdigit((unsigned char)*p) || *p == '.')
            {
                if (*p == '.') {
                    dot_count++;
                    if (dot_count > 3) break;
                }
                temp_ip[i++] = *p++;
                if (i >= 15) break;
            }
            temp_ip[i] = '\0';

            // 简单校验
            if (dot_count == 3 && strlen(temp_ip) >= 7)
            {
                strcpy(ip_buf, temp_ip);
                return 1;
            }
        }
        p++;
    }
    return 0;
}


// 获取并解析IP
uint8_t ESP32C3_AT_GetSTAIP(char *ip_buf)
{
    // 1. 发送 AT+CIFSR 并等待响应
    // 注意：IP响应通常包含在 OK 之前或同时
    // 我们复用 SendCmdAndWaitResp，它会把收到的内容留在 g_rx_buffer 中
    if (!ESP32C3_AT_SendCmdAndWaitResp("AT+CIFSR", "OK", NULL, NULL))
    {
        // 有些固件 AT+CIFSR 不回 OK，而是直接回 IP，所以如果找不到 OK
        // 我们再检查一下是否包含点号（.），如果有也算成功
        if (strstr((char *)g_rx_buffer, ".") == NULL) {
            printf("Get IP Cmd Failed\r\n");
            return 0;
        }
    }

    printf("Parsing IP from: %s\r\n", g_rx_buffer);

    // 2. 解析全局缓冲区中的数据
    if (ESP32C3_AT_ParseSTAIP((char *)g_rx_buffer, ip_buf))
    {
        printf("Parsed IP: %s\r\n", ip_buf);
        return 1;
    }
    
    return 0;
}

/* ================= MQTT 配置与连接 (适配中断接收版) ================= */

/**
 * @brief  配置中移物联网平台MQTT连接认证信息
 * @retval 1=成功，0=失败
 */
uint8_t ESP32C3_AT_MQTT_Config_CMCC(void)
{
    // 中移物联网平台MQTT认证配置指令
    // 注意：这里包含了大量的转义字符 \"，请确保复制完整
    const char *mqtt_cfg_cmd = "AT+MQTTUSERCFG=0,1,\"gassscan\",\"SVQx377cH4\",\"version=2018-10-31&res=products%2FSVQx377cH4%2Fdevices%2Fgassscan&et=1796981057000&method=md5&sign=YfBKPqkfejAbCY9tclAgKw%3D%3D\",0,0,\"\"";

    printf("--- [MQTT] Config Auth Info ---\r\n");

    // 直接调用通用发送等待函数
    // 预期响应是 "OK"
    // 这里的 buffer 传 NULL，因为我们不需要把 "OK" 拿出来给上层看，知道成功就行
    if (ESP32C3_AT_SendCmdAndWaitResp(mqtt_cfg_cmd, "OK", NULL, NULL))
    {
        printf("--- [MQTT] Config Success! ---\r\n");
        return 1;
    }
    else
    {
        printf("--- [MQTT] Config Failed! ---\r\n");
        return 0;
    }
}

/**
 * @brief  连接中移物联网平台MQTT服务器
 * @retval 1=连接成功，0=失败
 */
uint8_t ESP32C3_AT_MQTT_Connect_CMCC(void)
{
    // OneNET MQTT 连接指令
    const char *mqtt_conn_cmd = "AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1";

    printf("--- [MQTT] Connecting to Cloud... ---\r\n");

    // 连接指令可能需要稍长的时间，如果你的网络较差，
    // 建议暂时修改 ESP32C3_AT_SendCmdAndWaitResp 里的超时逻辑，或者增加重试
    // 这里默认等待 "OK"
    if (ESP32C3_AT_SendCmdAndWaitResp(mqtt_conn_cmd, "OK", NULL, NULL))
    {
        printf("--- [MQTT] Connected to OneNET! ---\r\n");
        return 1;
    }
    
    // 如果返回 ERROR 或者超时
    printf("--- [MQTT] Connect Failed! ---\r\n");
    return 0;
}


/**
 * @brief  订阅主题 (接收云端下发数据必备)
 * @param  topic: 订阅的主题，通常是 property/set
 * @retval 1=成功
 */
uint8_t ESP32C3_AT_MQTT_Subscribe(const char *topic)
{
    char cmd[256];
    // AT+MQTTSUB=0,"主题",1
    sprintf(cmd, "AT+MQTTSUB=0,\"%s\",1", topic);
    
    printf("--- [MQTT] Subscribing: %s ---\r\n", topic);
    
    return ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL);
}

/**
 * @brief  一次性订阅所有必要的主题
 *         1. set: 接收控制指令
 *         2. get: 接收查询指令
 *         3. post/reply: 接收上报是否成功的通知
 * @retval 1=全部成功, 0=有失败
 */
uint8_t ESP32_OneNET_Subscribe_All(void)
{
    // 订阅 set
    printf("--- [MQTT] Subscribing SET Topic ---\r\n");
    char cmd[256];
    sprintf(cmd, "AT+MQTTSUB=0,\"%s\",1", TOPIC_SET);
    if (!ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL)) return 0;
    
    HAL_Delay(200);

    // 订阅 get
    printf("--- [MQTT] Subscribing GET Topic ---\r\n");
    sprintf(cmd, "AT+MQTTSUB=0,\"%s\",1", TOPIC_GET);
    if (!ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL)) return 0;

    HAL_Delay(200);

    // 订阅 post/reply (可选，如果不想看上报结果可以不订)
    printf("--- [MQTT] Subscribing POST REPLY Topic ---\r\n");
    sprintf(cmd, "AT+MQTTSUB=0,\"%s\",1", TOPIC_POST_REPLY);
    if (!ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL)) return 0;

    return 1;
}


// 处理云端设置属性后的回复
// 参数 request_id: 从收到的字符串中解析出来的 id
// 参数 code: 状态码，通常成功传 200
uint8_t ESP32_OneNET_Reply_Set(char *request_id, uint8_t code)
{
    static char payload[256]; 
    char cmd[256];     

    // 1. 定义回复的主题
    // 必须是 $sys/{产品ID}/{设备名}/thing/property/set_reply
    // 请确保这里的产品ID和设备名与你的 Get 回复里的一致
    const char *topic = "$sys/SVQx377cH4/gassscan/thing/property/set_reply";

    int offset = 0;

    // 2. 组装 JSON (不需要搞复杂的转义了，就按正常的 C 语言字符串写)
    // 目标格式: {"id":"123", "code":200, "msg":"success"}
    
    offset += sprintf(payload + offset, 
        "{\"id\":\"%s\",\"code\":%d,\"msg\":\"success\"}", 
        request_id, code);

    printf("【Set Reply Debug】Payload: %s\r\n", payload);

    // 3. 发送 AT+MQTTPUBRAW 指令头
    // 告诉 ESP32 我要往这个 Topic 发 offset 个字节的数据
    sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, offset);
    
    // 发送指令头，等待 ">" 或者 "OK" (取决于你的 AT 固件版本，通常这里只要不报错就行)
    if (ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL) != 1)
    {
        // 如果这里打印了 Err，说明连指令头都错了，检查 topic 拼写
        printf("【Set Reply Err】指令头发送失败\r\n");
        return 0;
    }

    // 4. 发送真实的 JSON 数据
    // 直接把 payload 数组的内容吐给串口
    HAL_UART_Transmit(ESP32C3_UART_HANDLE, (uint8_t *)payload, offset, 1000);
    
    // 这一步延时很重要，给 ESP32 一点时间把数据经过 WiFi 发出去
    HAL_Delay(200); 
    
    printf("【Set Reply Success】回复设置成功\r\n");
    return 1;
}









