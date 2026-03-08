#include "21voc.h"
#include "stdio.h"

/* 全局/静态变量（保持与原代码一致） */
uint8_t rx_buf[9] = {0};       // 存储9字节帧数据（帧头+8字节数据）
uint8_t rx_cnt = 0;            // 接收字节计数
uint8_t rx_state = 0;          // 接收状态：0=等帧头，1=接收后续数据
uint8_t data_ready = 0;        // 数据就绪标志（通知main解析）
#define VOC_UART_TIMEOUT 100   // 轮询接收超时时间（ms），可根据传感器波特率调整

/**
 * @brief  轮询方式接收VOC201传感器数据（替代原中断回调）
 * @note   建议在main函数的while循环中周期性调用
 */
void VOC201_Receive(void)
{
  uint8_t curr_data = 0;
  HAL_StatusTypeDef uart_status;

  // 轮询读取1个字节（阻塞，直到收到数据或超时）
  uart_status = HAL_UART_Receive(&huart3, &curr_data, 1, VOC_UART_TIMEOUT);

  // 仅处理“接收成功”的情况（超时/错误则直接退出，不处理）
  if (uart_status == HAL_OK)
  {
    // 复用原有的状态机逻辑（帧头检测+字节计数）
    switch (rx_state)
    {
      case 0:  // 等待帧头0x2C
        if (curr_data == 0x2C)
        {
          rx_buf[rx_cnt] = curr_data;  // 帧头存入缓冲区
          rx_state = 1;                // 切换到接收后续数据状态
          rx_cnt++;                    // 计数+1
          printf("检测到帧头0x2C，开始接收后续数据\r\n");
        }
        // 非帧头：不处理，rx_state/rx_cnt保持初始值
        break;

      case 1:  // 接收后续8字节数据
        rx_buf[rx_cnt] = curr_data;    // 接收字节存入缓冲区
        rx_cnt++;                      // 计数+1

        // 检测是否收满9字节（帧头+8字节）
        if (rx_cnt >= 9)
        {
          data_ready = 1;              // 通知main函数解析数据
          rx_state = 0;                // 重置状态
          rx_cnt = 0;                  // 重置计数
          printf("9字节数据接收完成，可解析\r\n");
        }
        break;
    }
  }
  // 接收超时/错误：直接退出，不影响状态机
  else if (uart_status == HAL_TIMEOUT)
  {
    // 超时可选择重置状态（防止异常）
    rx_state = 0;
    rx_cnt = 0;
  }
}

