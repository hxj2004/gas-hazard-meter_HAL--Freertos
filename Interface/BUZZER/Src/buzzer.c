#include "buzzer.h"

#include "humiture.h"
#include "gass_measuer.h"
#include "stdio.h"
#include "gpio.h"
extern int8_t temp;
extern int8_t humid;
extern float mq2_value;
extern float ch2o;
extern float co2;
extern float tvoc;

// 定义危险阈值 (单位均为 PPM)         
float THRESHOLD_MQ2 = 3000.0f;    // 可燃气体 > 1000 ppm 报警
float THRESHOLD_CH2O = 0.08f;  // 甲醛 > 0.08 ppm 报警
float THRESHOLD_TVOC = 0.60f;  // TVOC > 0.60 ppm 报警
float THRESHOLD_CO2 = 6000.0f; // CO2 > 1500 ppm 报警

uint8_t isDanger = 0; // 标记位

// 扫描并报警主函数
void scanAndWarn(void)
{
  printf("当前阈值THRESHOLD_MQ2 = %.2f ppm\n", THRESHOLD_MQ2);
  printf("当前阈值THRESHOLD_CH2O = %.2f ppm\n", THRESHOLD_CH2O);
  printf("当前阈值THRESHOLD_TVOC = %.2f ppm\n", THRESHOLD_TVOC);
  printf("当前阈值THRESHOLD_CO2 = %.2f ppm\n", THRESHOLD_CO2);

  // 1. 读取传感器数据
  voc201_read();
  mq2_read();
  humiture_read();

  // 2. 危险判断逻辑
      isDanger = 0;

  // 检查 MQ2 (可燃气体)
  if (mq2_value > THRESHOLD_MQ2)
  {
    isDanger = 1;
  }

  // 检查 CH2O (甲醛)
  // 注意：甲醛ppm数值通常很小，如果传感器预热没好可能会飘零
  if (ch2o > THRESHOLD_CH2O)
  {
    isDanger = 1;
  }

  // 检查 TVOC
  if (tvoc > THRESHOLD_TVOC)
  {
    isDanger = 1;
  }

  // 检查 CO2
  if (co2 > THRESHOLD_CO2)
  {
    isDanger = 1;
  }

  // 3. 执行报警动作 (有源蜂鸣器)
  if (isDanger)
  {
    // 数据超标：蜂鸣器响
    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_SET); // 打开蜂鸣器 (蜂鸣器正极接高电平，负极接地)
  }
  else
  {
    // 数据正常：蜂鸣器停

    HAL_GPIO_WritePin(Buzzer_GPIO_Port, Buzzer_Pin, GPIO_PIN_RESET); // 打开蜂鸣器 (蜂鸣器正极接高电平，负极接地)
  }
}
