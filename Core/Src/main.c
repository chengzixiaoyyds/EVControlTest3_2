/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "TimeSlide.h"
#include "Button.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  STATE_WAIT_FOR_HEADER1,
  STATE_WAIT_FOR_HEADER2,
  STATE_READ_Command,
  STATE_READ_Data,
  STATE_READ_TAIL1,
  STATE_READ_TAIL2
} SerialState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define RECEIVE_HEADER_BYTE1 0xFA
#define RECEIVE_HEADER_BYTE2 0xAF
#define RECEIVE_TAIL_BYTE1 0xFB
#define RECEIVE_TAIL_BYTE2 0xBF
#define BACK_HEADER_BYTE1 0xFC
#define BACK_HEADER_BYTE2 0xCF
#define BACK_COMMAND_BYTE 0x03
#define BACK_TAIL_BYTE1 0xFD
#define BACK_TAIL_BYTE2 0xDF
#define TRANSMIT_HEADER_BYTE1 0xFA
#define TRANSMIT_HEADER_BYTE2 0xAF
#define TRANSMIT_COMMAND_BYTE 0x01
#define TRANSMIT_TAIL_BYTE1 0xFB
#define TRANSMIT_TAIL_BYTE2 0xBF
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
uint8_t state = 0; //0待机, 1运行
Button button1;
uint8_t receiveBuffer; // 接收缓冲区
uint8_t transmitBackBuffer[6];
uint8_t transmitBuffer[6];
SerialState current_state = STATE_WAIT_FOR_HEADER1;
uint8_t command, data;
TimeSliceTask led1ToggleTask;
uint8_t led1ToggleTaskState = 0;
uint8_t button1TaskState = 0;
uint8_t transmitState = 0x01;
uint8_t led2State = 0; // 0由暗到亮，1由亮到暗
float led2Light = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void LED1_Toggle(void)
{
  HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); // 切换LED1状态
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if(huart == &huart1)
  {
    if(state == 1)
    {
      switch(current_state)
      {
        case STATE_WAIT_FOR_HEADER1:
          if(receiveBuffer == RECEIVE_HEADER_BYTE1)
          {
            current_state = STATE_WAIT_FOR_HEADER2;
          }
          break;
        case STATE_WAIT_FOR_HEADER2:
          if(receiveBuffer == RECEIVE_HEADER_BYTE2)
          {
            current_state = STATE_READ_Command;
          }
          else
          {
            current_state = STATE_WAIT_FOR_HEADER1;
          }
          break;
        case STATE_READ_Command:
          command = receiveBuffer;
          current_state = STATE_READ_Data;
          break;
        case STATE_READ_Data:
          data = receiveBuffer;
          current_state = STATE_READ_TAIL1;
          break;
        case STATE_READ_TAIL1:
          if(receiveBuffer == RECEIVE_TAIL_BYTE1)
          {
            current_state = STATE_READ_TAIL2;
          }
          else
          {
            current_state = STATE_WAIT_FOR_HEADER1;
          }
          break;
        case STATE_READ_TAIL2:
          if(receiveBuffer == RECEIVE_TAIL_BYTE2)
          {
            if(command == 0x01)
            {
              uint8_t result = 0x00; // 默认失败
              if(data == 0x00)
              {
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); // 关闭LED1
                result = 0x01; // 执行成功
              }
              else if(data == 0x01)
              {
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); // 打开LED1
                result = 0x01; // 执行成功
              }
              // 发送响应帧
              if(huart1.gState == HAL_UART_STATE_READY)
              {
                transmitBackBuffer[3] = result;
                HAL_UART_Transmit_IT(&huart1, transmitBackBuffer, sizeof(transmitBackBuffer));
              }
            }
            else if(command == 0x02)
            {
              if(data == 0x00)
              {
                // 0次闪烁：停止任务，关闭LED，立即返回成功
                TimeSlide_Disable(&led1ToggleTask);
                HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
                if(huart1.gState == HAL_UART_STATE_READY)
                {
                  transmitBackBuffer[3] = 0x01;
                  HAL_UART_Transmit_IT(&huart1, transmitBackBuffer, sizeof(transmitBackBuffer));
                }
              }
              else
              {
                led1ToggleTask.run_times = data * 2; // 设置翻转次数（闪烁次数×2）
                TimeSlide_Enable(&led1ToggleTask); // 启动任务
              }
            }
            current_state = STATE_WAIT_FOR_HEADER1;
          }
          else
          {
            current_state = STATE_WAIT_FOR_HEADER1;
          }
          break;
        default:
          current_state = STATE_WAIT_FOR_HEADER1;
      }
    }
    HAL_UART_Receive_IT(&huart1, &receiveBuffer, 1); // 继续接收下一个字节
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if(htim == &htim4)
  {
    if(state == 1)
    {
      if(led2State == 0)
      {
        led2Light += 100.00 / 1100.00;
        if(led2Light >= 100)
        {
          led2Light = 100;
          led2State = 1; // 切换到由亮到暗
        }
      }
      else
      {
        led2Light -= 100.00 / 700.00;
        if(led2Light <= 0)
        {
          led2Light = 0.0;
          led2State = 0; // 切换到由暗到亮
        }
      }
      uint8_t pwm_value = (uint8_t)led2Light; // 将亮度转换为PWM值
      __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwm_value); // 调整LED2亮度
    }
  }
}
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
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start_IT(&htim4);
  led1ToggleTask.func = LED1_Toggle;
  led1ToggleTask.period_ms = 25; // 翻转周期 25ms（50ms闪烁周期的一半）
  button1.dead_zone_time = 50;        // 防抖时间 50ms
  button1.long_press_threshold = 2000; // 长按阈值 2秒
  transmitBackBuffer[0] = BACK_HEADER_BYTE1;
  transmitBackBuffer[1] = BACK_HEADER_BYTE2;
  transmitBackBuffer[2] = BACK_COMMAND_BYTE;
  transmitBackBuffer[4] = BACK_TAIL_BYTE1;
  transmitBackBuffer[5] = BACK_TAIL_BYTE2;
  transmitBuffer[0] = TRANSMIT_HEADER_BYTE1;
  transmitBuffer[1] = TRANSMIT_HEADER_BYTE2;
  transmitBuffer[2] = TRANSMIT_COMMAND_BYTE;
  transmitBuffer[4] = TRANSMIT_TAIL_BYTE1;
  transmitBuffer[5] = TRANSMIT_TAIL_BYTE2;
  HAL_UART_Receive_IT(&huart1, &receiveBuffer, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    button1TaskState = ButtonUpdate(&button1, HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_RESET);
    if(state == 0)
    {
      if(button1TaskState == 2)
      {
        state = 1; // 切换到运行状态
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        button1TaskState = 0;
      }
    }
    else if(state == 1)
    {
      if(button1TaskState == 2)
      {
        state = 0; // 切换到待机状态
        TimeSlide_Disable(&led1ToggleTask);
        led1ToggleTaskState = 0;
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); 
        led2Light = 0.0;
        led2State = 0;
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        transmitState = 0x01;
        button1TaskState = 0;
      }
      else if(button1TaskState == 1)
      {
        button1TaskState = 0;
        if(huart1.gState == HAL_UART_STATE_READY)
        {
          transmitBuffer[3] = transmitState;
          HAL_UART_Transmit_IT(&huart1, transmitBuffer, sizeof(transmitBuffer));
          transmitState = (transmitState == 0x01) ? 0x00 : 0x01;
        }
      }
      led1ToggleTaskState = TimeSlide_Update(&led1ToggleTask);
      if(led1ToggleTaskState == 1)
      {
        led1ToggleTaskState = 0;
        if(huart1.gState == HAL_UART_STATE_READY)
        {
          transmitBackBuffer[3] = 0x01; // 执行成功
          HAL_UART_Transmit_IT(&huart1, transmitBackBuffer, sizeof(transmitBackBuffer));
        }
      }
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

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
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
