#ifndef __MQ2_H__
#define __MQ2_H__

#include "stm32f1xx_hal.h"
#include "adc.h"
#include "usart.h"
#include "math.h"

void MQ2_Init(void);

float MQ2_GetConcentration_PPM(void);

#endif /* __MQ2_H__ */



