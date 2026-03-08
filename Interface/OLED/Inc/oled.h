#ifndef __OLED_H__
#define __OLED_H__

#include "stm32f1xx_hal.h"

#include "ssd1315.h"

void OLED_Init(void);

void OLED_Show_humiture(int8_t temp,int8_t humid);

void OLED_Show_mq2(uint16_t mq2_value);

void OLED_Show_voc201(float tvoc,float ch2o,uint16_t co2);


#endif /* __OLED_H__ */
