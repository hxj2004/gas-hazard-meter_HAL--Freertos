#include "onenet.h"
#include "esp32at.h"
#include "usart.h"
#include "stdio.h"
#include "humiture.h"
#include "gass_measuer.h"
#include "string.h"
#define ESP32C3_UART_HANDLE &huart1
#define ESP32C3_RESP_TIMEOUT 3000

// 引用 main.c 定义的全局变量
extern uint8_t g_rx_buffer[256];
extern volatile uint8_t g_rx_flag;
extern volatile uint16_t g_rx_len;

extern int8_t temp;
extern int8_t humid;
extern float mq2_value;
extern float ch2o;
extern float co2;
extern float tvoc;

extern volatile uint8_t g_is_at_mode; 

void onenet_init(uint8_t *resp_buf)
{


  // 复位 (可选，建议上电做一次)
  ESP32C3_AT_Reset(g_rx_buffer);

  // 测试AT指令
  if (ESP32C3_AT_Test(g_rx_buffer))
  {
    printf("ESP32 Check OK!\r\n");
  }
  else
  {
    printf("ESP32 Check Failed!\r\n");
  }

  // 3. 连接WiFi (会自动处理 CWMODE 和 DHCP)
  // 请替换为你的热点名称和密码
  if (ESP32C3_AT_ConnectWiFi("qq", "1122334455"))
  {
    // 获取IP耗时等待
    HAL_Delay(3000);
    ESP32C3_AT_GetSTAIP((char *)g_rx_buffer);

    // === MQTT 流程开始 ===

    // A. 配置用户信息 (UserConfig)
    if (ESP32C3_AT_MQTT_Config_CMCC())
    {
      HAL_Delay(500); // 稍微延时，防止指令过快

      // B. 发起连接 (Connect)
      if (ESP32C3_AT_MQTT_Connect_CMCC())
      {

        // 【关键步骤】订阅所有主题
        ESP32_OneNET_Subscribe_All();
      }
    }
  }

}


uint8_t ESP32_OneNET_Post_All_Sensors(void)
{
  // 【修改点1】加上 static 关键字
  // 这样它就不占用栈空间，而是占用全局 RAM，避免爆栈
  static char payload[512];
  char cmd[256];

  const char *topic = "$sys/SVQx377cH4/gassscan/thing/property/post";

  // 【修改点2】分段拼接 (推荐)
  // 一次性 sprintf 可能会在内部消耗巨大资源，分段写更安全，也方便调试
  // 还可以明确看到是哪一段卡住了
  int offset = 0;

  // 1. 拼接头部
  offset += sprintf(payload + offset,
                    "{\"id\":\"123\",\"version\":\"1.0\",\"params\":{");

  // 2. 拼接整数 (强制转为 int 避免兼容性问题)
  offset += sprintf(payload + offset,
                    "\"temp\":{\"value\":%d},", (int)temp);

  offset += sprintf(payload + offset,
                    "\"humid\":{\"value\":%d},", (int)humid);

  // 3. 拼接浮点数
  offset += sprintf(payload + offset,
                    "\"mq2_value\":{\"value\":%.3f},", mq2_value);

  offset += sprintf(payload + offset,
                    "\"ch2o\":{\"value\":%.3f},", ch2o);

  offset += sprintf(payload + offset,
                     "\"co2\":{\"value\":%d},", (int)co2);

  // 4. 拼接最后一个 (注意：最后一个没有逗号)
  offset += sprintf(payload + offset,
                    "\"tvoc\":{\"value\":%.3f}", tvoc);

  // 5. 拼接尾部
  offset += sprintf(payload + offset, "}}");

  // 打印检查完整性
  printf("【JSON Length】: %d\r\n", offset);
  printf("【JSON Content】: %s\r\n", payload);

  // --- 下面是发送流程，保持不变 ---

  sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, offset);

  if (ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL) != 1)
  {
    printf("【Err】CMD Head Fail\r\n");
    return 0;
  }

  HAL_UART_Transmit(ESP32C3_UART_HANDLE, (uint8_t *)payload, offset, 1000);
  HAL_Delay(200);

  printf("【Success】Post OK\r\n");
  return 1;
}



// 处理云端查询并回复
// 参数 request_id: 从收到的字符串中解析出来的 id，例如 "2"
// uint8_t ESP32_OneNET_Reply_Get(char *request_id)
// {
//     static char payload[512]; 
//     char cmd[256];     

//     // 【关键点1】回复的 Topic 必须是 get_reply
//     const char *topic = "$sys/SVQx377cH4/gassscan/thing/property/get_reply";

//     int offset = 0;

//     // 【关键点2】JSON 格式要求：
//     // {
//     //    "id": "收到什么ID就回什么ID",
//     //    "code": 200,
//     //    "msg": "success",
//     //    "data": { ...传感器数据... }
//     // }
    
//     offset += sprintf(payload + offset, 
//         "{\"id\":\"%s\",\"code\":200,\"msg\":\"success\",\"data\":{", 
//         request_id);

//     // 拼接传感器数据 (注意类型匹配，co2若云端是int则强转int)
//     offset += sprintf(payload + offset, "\"temp\":%d,", (int)temp);
//     offset += sprintf(payload + offset, "\"humid\":%d,", (int)humid);
//     offset += sprintf(payload + offset, "\"mq2_value\":%.2f,", mq2_value);
//     offset += sprintf(payload + offset, "\"ch2o\":%.2f,", ch2o);
//     // 假设云端co2是int32，这里强转一下，防止报错
//     offset += sprintf(payload + offset, "\"co2\":%d,", (int)co2);
//     // 最后一个没有逗号
//     offset += sprintf(payload + offset, "\"tvoc\":%.2f", tvoc);

//     // 封尾
//     offset += sprintf(payload + offset, "}}");

//     printf("【Reply Debug】: %s\r\n", payload);

//     // 发送 AT 指令
//     sprintf(cmd, "AT+MQTTPUBRAW=0,\"%s\",%d,0,0", topic, offset);
    
//     if (ESP32C3_AT_SendCmdAndWaitResp(cmd, "OK", NULL, NULL) != 1)
//     {
//         printf("【Reply Err】指令头发送失败\r\n");
//         return 0;
//     }

//     HAL_UART_Transmit(ESP32C3_UART_HANDLE, (uint8_t *)payload, offset, 1000);
//     HAL_Delay(200); // 稍微等一下让数据发出去
    
//     printf("【Reply Success】回复查询成功\r\n");
//     return 1;
// }






