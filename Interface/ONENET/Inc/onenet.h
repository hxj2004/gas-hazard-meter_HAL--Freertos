#ifndef __ONENET_H__
#define __ONENET_H__

#include "stm32f1xx_hal.h"

void onenet_init(uint8_t *resp_buf);

uint8_t ESP32_OneNET_Post_All_Sensors(void);


uint8_t ESP32_OneNET_Reply_Get(char *request_id);

#endif /* __ONENET_H__ */




