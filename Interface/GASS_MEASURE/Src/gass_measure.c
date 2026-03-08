#include "gass_measuer.h"
#include "mq2.h"
#include "oled.h"
#include "usart.h"
#include "stdio.h"
#include "21voc.h"
float mq2_value = 0;
float tvoc = 0, ch2o = 0, co2 = 0;      // 分别存储TVOC、CH2O和CO2浓度值
void Gas_Measure_Init(void)
{
  MQ2_Init();
}
void mq2_read(void)
{
  mq2_value = MQ2_GetConcentration_PPM();
  printf("Gas concentration: %.4f ppm\n", mq2_value);
  OLED_Show_mq2(mq2_value);
}

extern uint8_t data_ready;
extern uint8_t rx_buf[9];
extern uint8_t rx_cnt;
uint8_t rx_tim = 1;

void VOC301_ParseData(void)
{
  uint8_t check_sum_calc = 0; // 计算得到的校验和


  // 2. 计算校验和（协议要求：B9 = B1+B2+...+B8的uint8和）
  for (uint8_t i = 0; i < 8; i++) // 累加前8字节（B1~B8）
  {
    check_sum_calc += rx_buf[i];
  }
  if (check_sum_calc != rx_buf[8]) // 对比计算值与接收的B9
  {
           rx_tim = 1;
    return;
  }
  rx_tim = 0;

  // 3. 计算实际浓度（协议公式：(高字节*256 + 低字节) * 0.001 mg/m?）
  // TVOC浓度（B3=高字节，B4=低字节）
  tvoc = ((rx_buf[2] << 8) | rx_buf[3]) / 1000.0f;
  // 甲醛(CH2O)浓度（B5=高字节，B6=低字节）
  ch2o = ((rx_buf[4] << 8) | rx_buf[5]) / 1000.0f;
  // CO2浓度（B7=高字节，B8=低字节）
  co2 = ((rx_buf[6] << 8) | rx_buf[7]);


  printf("TVOC浓度：%.3f ppm\r\n", tvoc);
  printf("CH2O浓度：%.3f ppm\r\n", ch2o);
  printf("CO2浓度：%.3f ppm\r\n", co2);

  OLED_Show_voc201(tvoc, ch2o, co2);
}

void voc201_read(void)
{
  while (rx_tim == 1)
  {
    VOC201_Receive();
    VOC301_ParseData();
  }
  data_ready = 0;
  rx_tim = 1;
  
}
