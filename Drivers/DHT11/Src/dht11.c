#include "dht11.h"
#include "i2c.h"
#include "usart.h"
#include "stdio.h"
uint8_t data[5] = {0};
/// 起始信号
#define DHT11_DATA_H HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET)
#define DHT11_DATA_L HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_RESET)

// 读取引脚值
#define DHT11_DATA_READ HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin)

/// @brief DHT11上电需要1秒稳定
/// @param
void Inf_DHT11_Init(void)
{
  DHT11_DATA_H;
  HAL_Delay(1000);
}

/// @brief 读取DHT11数据
/// @param temp 温度
/// @param humid  湿度
void Inf_DHT11_getData(int8_t *temp, int8_t *humid)
{
  int8_t n_temp, n_humid;
  // 发送起始信号
  DHT11_DATA_L;
  HAL_Delay(20);
  DHT11_DATA_H;

  // 接收响应信号,设置超时时间，如果模块损坏，不能正确响应，则直接返回
  uint32_t count_max = 0xfffff; // 超时时间设置为10ms
                                // printf("等待信号拉低\r\n");
  while (DHT11_DATA_READ == GPIO_PIN_SET && count_max--)
  {
    // 等待信号拉低
  }
  // printf("等待信号拉高\r\n");
  count_max = 10000;
  while (DHT11_DATA_READ == GPIO_PIN_RESET && count_max--)
  {
    // 等待信号拉高会持续83us
    // 等待信号拉高
  }
  // printf("等待信号拉高,未超时\r\n");
  count_max = 10000;
  while (DHT11_DATA_READ == GPIO_PIN_SET && count_max--)
  {
    // 等待信号拉低会持续87us
    // 等待信号拉低
  }
  // 3.判断是否超时
  if (count_max == 0)
  {
    // printf("DHT11响应失败");
    return;
  }
  // printf("接入数据获取");
  for (uint8_t i = 0; i < 5; i++)
  {
    data[i] = 0;
    for (uint8_t j = 0; j < 8; j++)
    {
      // printf("等待高电平");
      while (DHT11_DATA_READ == GPIO_PIN_RESET)
      {
      }
      // printf("延时获取数据");
      Inf_DHT11_Delay_us(40);
      if (DHT11_DATA_READ == GPIO_PIN_SET)
      {
        data[i] |= (1 << (7 - j));

        while (DHT11_DATA_READ == GPIO_PIN_SET)
        {
        }
      }
      else
      {
      }
    }
  }
  for (int i = 0; i < 5; i++)
  {
    printf("data = %d\r\n", data[i]);
  }

  // 校验和计算
  uint32_t sum = data[0] + data[1] + data[2] + data[3];
  if ((uint8_t)sum == data[4])
  {
    printf("校验成功");
    n_humid = data[0];
    n_temp = data[2];
    if (data[3] & 0x80)
    {
      n_temp = -n_temp;
    }
  }
  else
  {

    printf("校验失败");
  }
  *temp = n_temp;
  *humid = n_humid;
  for (int i = 0; i < 5; i++)
  {
    data[i] = 0;
  }
  return;
}

/**
 * @brief 延时函数，用于精确控制时间。
 * 
 * @param us 参数
 */
void Inf_DHT11_Delay_us(uint32_t us)
{
  uint32_t ticks = (SystemCoreClock / 1000000) * us; // 72MHz → 72 ticks/us
  ticks /= 5;                                        // 补偿 while 循环开销（实际需根据芯片微调）
  while (ticks--)
  {
    __NOP();
  }
}
