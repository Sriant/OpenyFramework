/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include "k_kernel.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

static inline void LogUsart(const uint8_t *pdat, uint16_t len) {
	while (len--) {
		LL_USART_TransmitData8(USART2, *pdat++);
		while (LL_USART_IsActiveFlag_TXE(USART2) == RESET);
	}
}

void k_print(int level, const char *fmt, ...) {
    const char *level_str[] = {"[INFO] ", "[DEBUG] ", "[ERROR] "};
#define LOG_BUFFER_SIZE 256
    char buf[LOG_BUFFER_SIZE];

    LogUsart((uint8_t *)level_str[level], strlen(level_str[level]));
    va_list args;
    va_start(args, fmt);
    int cnt = vsnprintf(buf, LOG_BUFFER_SIZE, fmt, args);
    va_end(args);
    LogUsart((uint8_t *)buf, strlen(buf));
    LogUsart((uint8_t *)"\r\n", 2);
    return;
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

struct context_data {
	uint8_t id;
    char *name;
    uint32_t data;
	k_work_user_t work;
};

typedef struct AppEvent AppEvent_t;
typedef void (*AppEventHandler_t) (const AppEvent_t* event);

enum AppEventType {
	NONE = 0,
	TIMER,
};

struct AppEvent {
	union {
		struct {
			void *context;
		} TimerEvent;
	};
	
	enum AppEventType type;
	AppEventHandler_t handler;
};

K_MSGQ_DEFINE(sEventMsgq, sizeof(AppEvent_t), 10, 4);

void event_handler_test (const AppEvent_t* event) {
	if (event->type == TIMER) {
		struct context_data *ctx = (struct context_data *)event->TimerEvent.context;
		K_LOG_INFO("%s event hanlder", ctx->name);
	}
}

void k_work_user_test_handler(struct k_work_user *work) {
	struct context_data *ctx = (struct context_data *)work->context;
	K_LOG_INFO("%s workhandler", ctx->name);
}

void k_timer_test_callback(k_timer_t *timer) {
	struct context_data *ctx = (struct context_data *)timer->user_data;
	int err;
	
	/* work test, submit in ISR */
	ctx->data++;
	ctx->work.context = ctx;
	ctx->work.handler = k_work_user_test_handler;
	err = k_work_user_submit(&ctx->work);
	if (0 != err) {
		K_LOG_ERROR("k_work_user_submit failed %d! %s", err, ctx->name);
	}

	/* event messages test, put in ISR */
	AppEvent_t event;
	event.type = TIMER;
	event.handler = event_handler_test;
	event.TimerEvent.context = timer->user_data;
	err = k_msgq_put(&sEventMsgq, &event);
	if (0 != err) {
		K_LOG_ERROR("k_msgq_put failed %d! %s", err, ctx->name);
	}
}

void k_timer_test(void) {
	static struct context_data sTestData[2] = {0};
    k_timer_t *timer[2];
    char *str[] = {"timer0", "timer1"};

    for (size_t i = 0; i < 2; i++) {
        sTestData[i].name = str[i];
		sTestData[i].id = i;
        timer[i] = k_timer_create(k_timer_test_callback, &sTestData[i]);
        if (NULL == timer[i]) {
            K_LOG_ERROR("timer1 create failed");
            return;
        }
    }

    k_timer_start(timer[0], K_MSEC(50), K_MSEC(50));
    k_timer_start(timer[1], K_MSEC(50), K_MSEC(50));
	
	K_LOG_INFO("timer start !!!");
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
  MX_USART2_UART_Init();
  MX_TIM11_Init();
  /* USER CODE BEGIN 2 */

	LL_TIM_ClearFlag_UPDATE(TIM11);
	LL_TIM_EnableIT_UPDATE(TIM11);
	LL_TIM_EnableCounter(TIM11);
	
	k_timer_test();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {  
	/* main task */
	AppEvent_t event;
	if (0 == k_msgq_get(&sEventMsgq, &event)) {
		if (event.handler) event.handler(&event);
	}
	
	if (0 == k_work_user_wait()) {
		continue;
	} else {
		/* PM */
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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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

#ifdef  USE_FULL_ASSERT
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
