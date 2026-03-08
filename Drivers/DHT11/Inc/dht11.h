#ifndef __DHT11_H__
#define __DHT11_H__

#include "stm32f1xx_hal.h"


/// @brief DHT11上电需要1秒稳定
/// @param  
void Inf_DHT11_Init(void);

/// @brief 读取DHT11数据
/// @param temp 温度
/// @param humid  湿度
void Inf_DHT11_getData(int8_t *temp, int8_t *humid);

void Inf_DHT11_Delay_us(uint32_t us);

#endif /* __DHT11_H__ */

