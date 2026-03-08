#ifndef __21VOC_H__
#define __21VOC_H__

#include <stdint.h>

// 变量声明
extern uint8_t rx_buf[9];
extern volatile uint8_t data_ready;

// 函数声明 (这就是你缺少的函数声明)

void VOC201_Receive(void);        // 任务调用的读取


#endif
