/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "i2c.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "oled.h"
#include "stdio.h"
#include "humiture.h"
#include "gass_measuer.h"
#include "esp32at.h"
#include "string.h"
#include "onenet.h"
#include "buzzer.h"
#include <stdlib.h>
#include "21voc.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t g_rx_buffer[256] = {0};
volatile uint8_t g_rx_flag = 0; // 接收完成标志 (1=完成)
volatile uint16_t g_rx_len = 0; // 实际接收到的长度

// 定义一个变量记录上一次执行任务的时间
uint32_t last_task_time = 0;
// 定义任务执行的间隔时间 (ms)，这里设为 10000 10秒执行一次任务
const uint32_t TASK_INTERVAL = 1000;

void get_topicAndMessage(void);
void get_sensorData(void);
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t isover = 0;
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  printf("Hello World\n");

 OLED_Init();
 Gas_Measure_Init();
 humiture_init();

//  HAL_UARTEx_ReceiveToIdle_IT(&huart1, g_rx_buffer, 256);

//  onenet_init(g_rx_buffer);


  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
    get_topicAndMessage();
    // ---------------------------------------------------------
    // 任务 1：高优先级任务 - 串口数据处理 (全速轮询)
    // ---------------------------------------------------------
    // 这里没有 Delay，CPU 会以最快速度不断检查这个标志位
    // 一旦串口中断回调置位了 g_rx_flag，这里能毫秒级响应

    // ---------------------------------------------------------
    // 任务 2：低优先级周期性任务 - 传感器读取与上传 (每3秒一次)
    // ---------------------------------------------------------
    // 检查当前时间 - 上一次执行时间 是否超过 间隔
    get_sensorData();

    /* USER CODE BEGIN 3 */
    // 这里不需要额外的 Delay 了，让 CPU 全速跑起来检查 g_rx_flag
       // 只要没有数据处理标志，就进入浅睡眠
  //  if (g_rx_flag == 0) {
  //      HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
  //  }
    /* USER CODE END 3 */
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart->Instance == USART1) // 确认是 ESP32 的串口
  {
    g_rx_len = Size; // 记录长度
    g_rx_flag = 1;   // 置位标志，通知 main 或 AT函数 数据到了

    // 加上结束符，方便字符串处理
    if (g_rx_len < 256)
      g_rx_buffer[g_rx_len] = '\0';

    // 【核心】重新开启中断接收，防止下次收不到
    HAL_UARTEx_ReceiveToIdle_IT(&huart1, g_rx_buffer, 256);
  }
}

extern float THRESHOLD_MQ2;
extern float THRESHOLD_CH2O;
extern float THRESHOLD_TVOC;
extern float THRESHOLD_CO2;
void get_topicAndMessage()
{
  if (g_rx_flag == 1)
  {
    printf("【RX Event】: %s\r\n", g_rx_buffer);
    char *cmd_set_ptr = strstr((char *)g_rx_buffer, "thing/property/set");

    if (cmd_set_ptr != NULL)
    {
      printf(">>> 收到云平台【设置阈值】指令！\r\n");

      // --- 1. 解析各个阈值 ---
      // 使用 atof 函数，它会从指针位置开始读数字，直到遇到非数字字符(如 , 或 })停止

      // 1.1 解析 mq2_value
      char *p_mq2 = strstr((char *)g_rx_buffer, "\"mq2_value\":");
      if (p_mq2 != NULL)
      {
        // 指针移动 12位，跳过 "mq2_value": 这段字符
        THRESHOLD_MQ2 = atof(p_mq2 + 12);
        printf("Set Limit MQ2: %.2f\r\n", THRESHOLD_MQ2);
      }

      // 1.2 解析 ch2o
      char *p_ch2o = strstr((char *)g_rx_buffer, "\"ch2o\":");
      if (p_ch2o != NULL)
      {
        // 跳过 "ch2o": (7个字符)
        THRESHOLD_CH2O = atof(p_ch2o + 7);
        printf("Set Limit CH2O: %.2f\r\n", THRESHOLD_CH2O);
      }

      // 1.3 解析 co2
      char *p_co2 = strstr((char *)g_rx_buffer, "\"co2\":");
      if (p_co2 != NULL)
      {
        // 跳过 "co2": (6个字符)
        THRESHOLD_CO2 = atof(p_co2 + 6);
        printf("Set Limit CO2: %.2f\r\n", THRESHOLD_CO2);
      }

      // 1.4 解析 tvoc
      char *p_tvoc = strstr((char *)g_rx_buffer, "\"tvoc\":");
      if (p_tvoc != NULL)
      {
        // 跳过 "tvoc": (7个字符)
        THRESHOLD_TVOC = atof(p_tvoc + 7);
        printf("Set Limit TVOC: %.2f\r\n", THRESHOLD_TVOC);
      }

      // --- 2. 解析 ID 并回复 (告诉云平台我改好了) ---
      // 通常 OneNET 设置指令也需要回复，否则云端会显示“超时”
      char *id_start = strstr((char *)g_rx_buffer, "\"id\":\"");
      if (id_start != NULL)
      {
        id_start += 6;
        char *id_end = strchr(id_start, '\"');
        if (id_end != NULL)
        {
          char request_id[32] = {0};
          int id_len = id_end - id_start;
          if (id_len > 31)
            id_len = 31;
          strncpy(request_id, id_start, id_len);

          // 这里你需要一个回复 Set 成功的函数，格式通常很简单，code 200 即可
          // 如果没有专门的 Set 回复函数，有些平台用 Get 回复函数也能糊弄过去
          ESP32_OneNET_Reply_Set(request_id,200);
          printf(">>> 阈值更新完毕，已回复 ID: %s\r\n", request_id);
        }
      }

      // 立即用新阈值检测一次
      scanAndWarn();
    }
    else
    {
      // --- 新增：解析云平台下发的“属性获取”指令 ---

      // 1. 判断是不是 get 请求
      // 你的 Log 显示 Topic 是 .../thing/property/get
      char *cmd_ptr = strstr((char *)g_rx_buffer, "thing/property/get");

      if (cmd_ptr != NULL)
      {
        printf(">>> 收到云平台数据请求！正在解析ID...\r\n");

        // 2. 解析 ID
        // 目标字符串格式: ... "id":"2", ...
        char *id_start = strstr((char *)g_rx_buffer, "\"id\":\"");

        if (id_start != NULL)
        {
          // 指针移动 6 位，跳过 "id":" 这6个字符，指向 ID 的第一个字符
          id_start += 6;

          // 找到 ID 结束的引号
          char *id_end = strchr(id_start, '\"');

          if (id_end != NULL)
          {
            // 临时存放 ID 的缓冲区
            char request_id[32] = {0};

            // 计算 ID 长度
            int id_len = id_end - id_start;
            if (id_len > 31)
              id_len = 31; // 防止溢出

            // 复制 ID
            strncpy(request_id, id_start, id_len);
            request_id[id_len] = '\0'; // 加上结束符

            printf(">>> 解析成功，Request ID: %s\r\n", request_id);

            // 3. 立即刷新传感器数据
            printf(">>> 正在刷新传感器数据...\r\n");
            scanAndWarn();

            // 4. 调用回复函数
            ESP32_OneNET_Reply_Get(request_id);
          }
        }
      }
    }

    // ---------------------------------------------

    // 清除标志位
    g_rx_flag = 0;
  }
}

void get_sensorData(void)
{
    // 检查时间间隔
    if (HAL_GetTick() - last_task_time >= TASK_INTERVAL)
    {
        last_task_time = HAL_GetTick(); // 更新时间

        printf("\r\n--- [任务开始] 读取传感器并上传 ---\r\n");

        // 1. 读取传感器数据 (更新全局变量)
        scanAndWarn(); 
        
        // 稍微延时，给传感器缓冲
        HAL_Delay(10);

        // 2. 尝试上传数据，并检查返回值
        // ESP32_OneNET_Post_All_Sensors 返回 1 代表成功，0 代表失败
        if (ESP32_OneNET_Post_All_Sensors() == 1)
        {
            printf(">>> 上传成功 (Success)\r\n");
        }
        else
        {
            // === 这里是新增的重连逻辑 ===
            printf(">>> [错误] MQTT上传失败，连接可能已断开！\r\n");
            printf(">>> [重连] 正在执行重连流程...\r\n");

            // 调用初始化函数进行全套重连
            // 你的 onenet_init 里包含了 AT+RST，这很好，能彻底复位 ESP32 状态
            onenet_init(g_rx_buffer);

            printf(">>> [重连] 重连流程结束，等待下一次周期尝试上传。\r\n");
        }

        printf("--- [任务结束] 继续监听串口 ---\r\n");
    }
}
/* USER CODE END 4 */

/**
 * @brief  Period elapsed callback in non blocking mode
 * @note   This function is called  when TIM1 interrupt took place, inside
 * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
 * a global variable "uwTick" used as application time base.
 * @param  htim : TIM handle
 * @retval None
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
