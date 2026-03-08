#ifndef __APP_FREERTOS_H__
#define __APP_FREERTOS_H__

// 1. 必须最先包含 FreeRTOS 核心
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "event_groups.h"

// 2. 包含你的业务头文件
#include "main.h"
#include "gpio.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

// 引入你的驱动
#include "humiture.h"
#include "gass_measuer.h"
#include "buzzer.h"
#include "oled.h"
#include "onenet.h"
#include "esp32at.h"
#include "21voc.h"

// --- 外部变量声明 (引用原来的全局变量) ---
extern float THRESHOLD_MQ2;
extern float THRESHOLD_CH2O;
extern float THRESHOLD_TVOC;
extern float THRESHOLD_CO2;

extern uint8_t g_rx_buffer[256];

// --- FreeRTOS 句柄声明 ---
extern SemaphoreHandle_t xSem_UART_RX; // 串口接收信号量
extern SemaphoreHandle_t xMutex_Data;  // 数据保护互斥锁

// --- 函数声明 ---
void FreeRTOS_Start(void);

#endif /* __APP_FREERTOS_H__ */
