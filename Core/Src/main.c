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
#include "i2c.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "calibration.h"
#include "config.h"
#include "display.h"
#include "input.h"
#include "loadcell.h"
#include "motor.h"
#include <math.h>

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
static float s_target_kg = FORCE_DEFAULT_KG;
static uint32_t s_display_tick = 0;
static float s_last_saved_target_kg = FORCE_DEFAULT_KG;
static uint32_t s_last_target_change_tick = 0;
static uint32_t s_last_flash_save_tick = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

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
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  motor_init();
  loadcell_init();
  input_init();
  calib_init();

  {
    float saved_scale;
    int32_t saved_offset;
    float saved_target;
    if (flash_load(&saved_scale, &saved_offset, &saved_target)) {
      loadcell_set_scale(saved_scale);
      loadcell_set_offset(saved_offset);
      if (saved_target > 0.0f && saved_target <= FORCE_MAX_KG) {
        s_target_kg = saved_target;
      }
    }
  }
  s_last_saved_target_kg = s_target_kg;
  s_last_target_change_tick = HAL_GetTick();
  s_last_flash_save_tick = HAL_GetTick();

  display_init();
  display_set_screen(SCREEN_MAIN);
  display_set_force(0.0f, s_target_kg);

  HAL_TIM_Base_Start_IT(&htim2);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    loadcell_update();
    input_update();
    motor_update();

    float force = loadcell_get_kg();
    int8_t enc_delta = input_enc_get_delta();
    DisplayScreen cur_screen = display_get_screen();

    if (input_stop_pressed()) {
      input_stop_clear();
      if (cur_screen == SCREEN_ERROR) {
        motor_clear_error();
        display_set_screen(SCREEN_MAIN);
        cur_screen = SCREEN_MAIN;
      } else {
        display_show_error("STOP: press again");
        cur_screen = SCREEN_ERROR;
      }
    }

    if (loadcell_has_error() && motor_is_running()) {
      motor_stop();
      display_show_error("HX711 no response");
    }

    if (cur_screen == SCREEN_MAIN) {

      if (motor_is_running()) {
        if (force >= s_target_kg * OVERLOAD_FACTOR) {
          motor_emergency_stop();
          display_show_error("OVERLOAD!");
        } else if (force >= s_target_kg) {
          motor_stop();
        }
      }

      bool joy_up = input_joy_up();
      bool joy_down = input_joy_down();

      if (joy_up && !motor_is_limit_top()) {
        uint16_t spd = (force >= s_target_kg * SLOWDOWN_THRESHOLD) ? SPEED_SLOW : SPEED_FAST;
        motor_move_up(spd);
      } else if (joy_down && !motor_is_limit_bot()) {
        uint16_t spd = (force >= s_target_kg * SLOWDOWN_THRESHOLD) ? SPEED_SLOW : SPEED_FAST;
        motor_move_down(spd);
      } else if (!joy_up && !joy_down && motor_is_running()) {
        motor_stop();
      }

      if (enc_delta != 0) {
        s_target_kg += (float)enc_delta * FORCE_STEP_KG;
        if (s_target_kg < FORCE_STEP_KG) s_target_kg = FORCE_STEP_KG;
        if (s_target_kg > FORCE_MAX_KG) s_target_kg = FORCE_MAX_KG;
        s_last_target_change_tick = HAL_GetTick();
      }

      if (input_enc_sw_pressed()) {
        display_set_screen(SCREEN_MENU);
      }

      if ((HAL_GetTick() - s_display_tick) >= 100U) {
        s_display_tick = HAL_GetTick();
        display_set_force(force, s_target_kg);
      }

      if (!motor_is_running()) {
        uint32_t now = HAL_GetTick();
        if ((now - s_last_target_change_tick) >= 1500U &&
            (now - s_last_flash_save_tick) >= 15000U &&
            fabsf(s_target_kg - s_last_saved_target_kg) >= 0.05f)
        {
          if (flash_save(loadcell_get_scale(), loadcell_get_offset(), s_target_kg)) {
            s_last_saved_target_kg = s_target_kg;
            s_last_flash_save_tick = now;
          }
        }
      }

    } else if (cur_screen == SCREEN_MENU) {

      if (enc_delta > 0) display_menu_next();
      else if (enc_delta < 0) display_menu_prev();

      if (input_enc_sw_pressed()) {
        uint8_t item = display_menu_get_item();
        switch (item) {
          case 0: display_set_screen(SCREEN_MAIN); break;
          case 1: calib_start(s_target_kg); break;
          case 2: display_set_screen(SCREEN_SETTINGS); break;
          case 3: motor_reset_position(); display_set_screen(SCREEN_MAIN); break;
          default: break;
        }
      }

    } else if (cur_screen == SCREEN_CALIBRATION) {

      if (calib_is_active()) {
        calib_adjust(enc_delta);
        if (input_enc_sw_pressed()) calib_confirm();
        calib_update();
      } else {
        display_set_screen(SCREEN_MAIN);
      }

    } else if (cur_screen == SCREEN_SETTINGS) {

      if (input_enc_sw_pressed()) {
        display_set_screen(SCREEN_MENU);
      }
    }

    display_update();
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
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
  if (GPIO_Pin == STOP_BTN_Pin) {
    motor_emergency_stop();
    input_stop_set();
  }
  if (GPIO_Pin == ENC_CLK_Pin) {
    input_enc_isr();
  }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
    motor_tim_tick();
    input_debounce_tick();
  }
}

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
