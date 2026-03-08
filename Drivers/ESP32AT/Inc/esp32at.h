#ifndef __ESP32AT_H__
#define __ESP32AT_H__

#include "stm32f1xx_hal.h"

/* ================= OneNET MQTT 主题配置 ================= */
// 基础信息
#define ONENET_PID          "SVQx377cH4"
#define ONENET_DEVICE       "gassscan"

// 主题前缀 (不要动)
#define TOPIC_PREFIX        "$sys/" ONENET_PID "/" ONENET_DEVICE "/thing/property/"

// 1. 属性上报 (设备 -> 云)
#define TOPIC_POST          TOPIC_PREFIX "post"
#define TOPIC_POST_REPLY    TOPIC_PREFIX "post/reply"  // 需要订阅

// 2. 属性设置 (云 -> 设备)
#define TOPIC_SET           TOPIC_PREFIX "set"         // 需要订阅
#define TOPIC_SET_REPLY     TOPIC_PREFIX "set_reply"   // 收到设置后，回复给云端

// 3. 属性获取 (云 -> 设备)
#define TOPIC_GET           TOPIC_PREFIX "get"         // 需要订阅
#define TOPIC_GET_REPLY     TOPIC_PREFIX "get_reply"   // 收到查询后，回复给云端

uint8_t ESP32C3_AT_SendCmdAndWaitResp(const char *cmd, const char *target_resp,
                                      uint8_t *resp_buf, uint16_t *resp_len);
                                      
uint8_t ESP32C3_AT_Reset(uint8_t *resp_buf);

uint8_t ESP32C3_AT_Test(uint8_t *resp_buf);

uint8_t ESP32C3_AT_SetWiFiMode(uint8_t mode);

uint8_t ESP32C3_AT_EnableSTA_DHCP(void);

uint8_t ESP32C3_AT_ConnectWiFi(const char *ssid, const char *pwd);

uint8_t ESP32C3_AT_ParseSTAIP(const char *resp, char *ip_buf);

uint8_t ESP32C3_AT_GetSTAIP(char *ip_buf);

//onenet
uint8_t ESP32C3_AT_MQTT_Config_CMCC(void);


uint8_t ESP32C3_AT_MQTT_Connect_CMCC(void);

uint8_t ESP32_OneNET_Subscribe_All(void);

// 专门的数据上报函数
uint8_t ESP32_OneNET_Post_All_Sensors(void);

uint8_t ESP32_OneNET_Reply_Set(char *msg_id, uint8_t code);




#endif /* __ESP32AT_H__ */


