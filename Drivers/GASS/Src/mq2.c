#include "mq2.h"
#include <math.h> // 必须包含数学库

// 参数定义
#define MQ2_READ_TIMES  10
#define VCC_MQ2         5.0f   // 传感器供电电压
#define ADC_REF_VOLTAGE 3.3f   // STM32 ADC参考电压
#define RL              2000.0f // 负载电阻 2KΩ (单位：欧姆)
#define R0              6640.0f // 【修正点】基准电阻 6.64kΩ -> 6640Ω (必须与RL单位一致)

void MQ2_Init(void)
{
  // 你的adc.c里配置了ContinuousConvMode=ENABLE
  // 所以这里调用一次Start即可，后续adc会自动连续转换
  HAL_ADC_Start(&hadc1); 
}

// 必要的宏定义 (如果没有在.h里定义，请补上)
// 这里的 CALIB_PARA_A 和 B 是针对 LPG/可燃气体的标准曲线参数
#define CALIB_PARA_A  574.25f 
#define CALIB_PARA_B  -2.222f 

float MQ2_GetConcentration_PPM(void)
{
  uint32_t adc_sum = 0;
  
  // 1. 采样 (保持你原有的逻辑)
  for (uint8_t i = 0; i < MQ2_READ_TIMES; i++)
  {
    adc_sum += HAL_ADC_GetValue(&hadc1);
    HAL_Delay(2);
  }
  
  if (adc_sum == 0) return 0.0f;
  float adc_raw = (float)adc_sum / MQ2_READ_TIMES;

  // 2. 换算电压
  float vol = (adc_raw * ADC_REF_VOLTAGE) / 4095.0f;
  
  // 简单保护，防止电压等于VCC导致分母为0
  if (vol >= VCC_MQ2) vol = VCC_MQ2 - 0.01f; 
  if (vol <= 0.02f) vol = 0.02f; // 防止电压为0

  // 3. 计算传感器电阻 RS
  // 根据分压公式 Vout = VCC * (RL / (RL + RS)) 推导：
  // RS = RL * (VCC - Vout) / Vout
  float RS = RL * (VCC_MQ2 - vol) / vol;

  // 4. 计算 PPM (使用标准幂函数公式)
  // 这里的 R0 建议你在洁净空气中实测一次 RS 算出来，
  // 或者暂时先用你的 6640.0f，如果数值偏大或偏小再调整 R0
  float ratio = RS / R0; 
  
  // 避免 ratio 为负或 0
  if(ratio <= 0) ratio = 0.01f;

  // 公式：PPM = a * ratio^b
  float ppm = CALIB_PARA_A * pow(ratio, CALIB_PARA_B);

  return ppm;
}
