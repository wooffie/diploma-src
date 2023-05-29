/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2022 STMicroelectronics.
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
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "afe.h"
#include "heartmonitor.h"
#include "ssd1306.h"
#include "bee.h"
#include "lis2dtw12_reg.h"
#include "stdio.h"
#include "string.h"
#include "ledhelper.h"
#include <stdlib.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define SENSOR_BUS hi2c1
#define BOOT_TIME 100 //ms

#define LED_DEPTH 25
#define ACC_DEPTH 25

#define LED_ON_TIME 60 // secs

#define SpO_OK_low 85 // percents
#define HR_limit_low 50 // bpm
#define HR_limit_high 150 // bpm

#define CLOCK 25 // Hz

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

FlagStatus LED_ON = SET;
uint16_t LED_OFF_tim = 0;

uint16_t BEE_rate = 1024;

#define ACC_THRESHOLD 3000 // 3 G
static int16_t data_acceleration[ACC_DEPTH][3];
static int32_t data_acceleration_sum;
static int16_t data_raw_acceleration[3];
static int16_t data_raw_temperature;
static float temperature = 0.0;
static uint8_t whoamI, rst;
static lis2dtw12_ctrl4_int1_pad_ctrl_t ctrl4_int1_pad;

FlagStatus AFE_status = SET;
float AFE_GREEN[LED_DEPTH];
float AFE_RED[LED_DEPTH];
float AFE_IR[LED_DEPTH];

FlagStatus DATA_RDY = SET;

float SP_R;
int SP_DISP = 0;

#define TIME_TO_IDLE 20
uint16_t IDLE_timer = 0;
FlagStatus IDLE = RESET;

int HR_COUNT = 0;
float HR_FIN = 80;
int HR_OK = 0;
float HR_DISP = 80;
float HR = 80;

uint8_t tx_buff[3] = { 0, 0, 0 };

// HR CONFIG
#define HR_MON_SIZE  100
#define HR_THRESHOLD  0.35
#define HR_BPF_ORDER  4
#define HR_BPF_LOW  0.8
#define HR_BPF_HIGH  8.5
#define HR_SSF_SIZE  9
#define HR_MA_GREEN_SIZE  5
#define HR_MA_REDIR_SIZE  5

#define SAMPLING_RATE 25
#define ROUNDS 4

#ifdef HR_DEBUG
float AFE_GREEN_COPY[LED_DEPTH];

float HR_M;
float HR_B;
float HR_S;
float HR_P;
float HR_G;
static float acceleration_mg[3];
float SP_RED, SP_IR;

#endif

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
void LED_on();
void LED_off();
void TPS_on();
void TPS_off();

float getAverageFloat(float *array, uint16_t size);

float findMin(float *array, uint16_t size);
float findMax(float *array, uint16_t size);

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
		uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
		uint16_t len);

static void platform_delay(uint32_t ms);
static void platform_init(void);

uint16_t t_t(int16_t temp);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// SWO trace
int _write(int file, char *ptr, int len) {
	int i = 0;
	for (i = 0; i < len; i++) {
		ITM_SendChar((*ptr++));
	}
	return len;
}

// debug
int counter = 0;

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
	MX_DMA_Init();
	MX_I2C1_Init();
	MX_USART1_UART_Init();
	MX_SPI1_Init();
	MX_TIM2_Init();
	MX_ADC1_Init();
	MX_TIM3_Init();
	/* USER CODE BEGIN 2 */

	HAL_TIM_Base_Start_IT(&htim2);
	HAL_TIM_Base_Start_IT(&htim3);

	/* AFE INIT START*/
	// Enable power for 4.2V
	TPS_on();
	AFE_status = SET;
	// Initialize AFE4420
	AFE_INIT(&hspi1);
	// Set voltage on LEDs
	AFE_LED_STRENGTH(0, 30, 30, 30);
	AFE_ILED_UPDATE();
	/* AFE INIT END */

	/* LED INIT START*/
	LED_on();
	LED_pribor();
	/* LED INIT END */

	/* LIS INIT START */
	stmdev_ctx_t dev_ctx;
	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &SENSOR_BUS;
	// Initialize platform specific hardware
	platform_init();
	// Wait sensor boot time
	platform_delay(BOOT_TIME);
	// Check device ID
	lis2dtw12_device_id_get(&dev_ctx, &whoamI);

	if (whoamI != LIS2DTW12_ID)
		while (1) {
			// Device not found
		}

	// Restore default configuration
	lis2dtw12_reset_set(&dev_ctx, PROPERTY_ENABLE);

	do {
		lis2dtw12_reset_get(&dev_ctx, &rst);
	} while (rst);

	//  Enable Block Data Update
	lis2dtw12_block_data_update_set(&dev_ctx, PROPERTY_ENABLE);
	// interrupt
	lis2dtw12_int_notification_set(&dev_ctx, LIS2DTW12_INT_LATCHED);
	lis2dtw12_pin_polarity_set(&dev_ctx, LIS2DTW12_ACTIVE_HIGH);
	lis2dtw12_pin_int1_route_get(&dev_ctx, &ctrl4_int1_pad);
	ctrl4_int1_pad.int1_wu = PROPERTY_ENABLE;
	lis2dtw12_pin_int1_route_set(&dev_ctx, &ctrl4_int1_pad);

	// sleep
	lis2dtw12_wkup_dur_set(&dev_ctx, 0x2);
	lis2dtw12_act_sleep_dur_set(&dev_ctx, 0x2);
	lis2dtw12_wkup_threshold_set(&dev_ctx, 0x2);
	/* Data sent to wake-up interrupt function */
	lis2dtw12_wkup_feed_data_set(&dev_ctx, LIS2DTW12_HP_FEED);
	/* Config activity / inactivity or stationary / motion detection */
	lis2dtw12_act_mode_set(&dev_ctx, LIS2DTW12_DETECT_ACT_INACT);

	// Set full scale
	lis2dtw12_full_scale_set(&dev_ctx, LIS2DTW12_8g);
	// Configure filtering chain
	// Accelerometer - filter path / bandwidth
	lis2dtw12_filter_path_set(&dev_ctx, LIS2DTW12_LPF_ON_OUT);
	lis2dtw12_filter_bandwidth_set(&dev_ctx, LIS2DTW12_ODR_DIV_4);
	// Configure FIFO
	lis2dtw12_fifo_watermark_set(&dev_ctx, ACC_DEPTH);
	lis2dtw12_fifo_mode_set(&dev_ctx, LIS2DTW12_STREAM_MODE);
	// Configure power mode
	lis2dtw12_power_mode_set(&dev_ctx, LIS2DTW12_HIGH_PERFORMANCE);
	//lis2dtw12_power_mode_set(&dev_ctx, LIS2DTW12_CONT_LOW_PWR_LOW_NOISE_12bit);
	// Set Output Data Rate */
	lis2dtw12_data_rate_set(&dev_ctx, LIS2DTW12_XL_ODR_25Hz);
	/* LIS INIT END */

	/* BEE INIT START */
	// ENABLE SWARM PA0 (MOD_EN) -> MOD_EN
	HAL_GPIO_WritePin(MOD_EN_GPIO_Port, MOD_EN_Pin, GPIO_PIN_SET);

	// Set swarm into external control mode PA1 -> A_MODE
	HAL_GPIO_WritePin(A_MODE_GPIO_Port, A_MODE_Pin, GPIO_PIN_RESET);

	HAL_Delay(100);

	// First msg = SWARM RADIO. Must be read!
	HAL_UART_Receive(&huart1, (uint8_t*) bee_response, 128, 100);
	HAL_Delay(1000);

	// Set uart handler for bee
	BEE_init(&huart1);
	BEE_set_binary();
	HAL_Delay(100);
	// Switch bee in binary mode (from ASCII)

	// Set interval between messages from bee
	BEE_b_sbiv(BEE_rate);
	HAL_Delay(100);
	/* BEE INIT END */

	HR_HeartMonitor *HR_heartMonitor = HR_heartMonitor_new(SAMPLING_RATE,
	HR_MON_SIZE,
	HR_THRESHOLD, HR_BPF_ORDER, HR_BPF_LOW, HR_BPF_HIGH, HR_SSF_SIZE,
	HR_MA_GREEN_SIZE,
	HR_MA_REDIR_SIZE);

	MA_filter *ma_red = MA_new(7);
	MA_filter *ma_ir = MA_new(7);

#ifdef HR_DEBUG
	// Debug
	printf("Starting debug cycle!\r\n");
	while (1) {
		while (HAL_GPIO_ReadPin(AFE_ADC_RDY_GPIO_Port, AFE_ADC_RDY_Pin)
				!= GPIO_PIN_SET)
			;
		AFE_FIFO_READ(LED_DEPTH * 4);
		for (uint16_t i = 0; i < LED_DEPTH; i++) {
			AFE_RED[i] = AFE_FLOAT[i * 4];
			AFE_IR[i] = AFE_FLOAT[1 + i * 4];
			AFE_GREEN[i] = AFE_FLOAT[2 + i * 4];
			AFE_GREEN_COPY[i] = AFE_GREEN[i];
		}

		MA_process(ma_red, AFE_RED, 25);
		MA_process(ma_ir, AFE_IR, 25);

		// Read data from LIS2
		uint8_t val;
		lis2dtw12_fifo_data_level_get(&dev_ctx, &val);

		if (val >= 25) {
			lis2dtw12_fifo_data_level_get(&dev_ctx, &val);
			for (int i = ACC_DEPTH - 1; i >= 0; i--) {
				memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
				lis2dtw12_acceleration_raw_get(&dev_ctx, data_raw_acceleration);
				data_acceleration[i][0] = lis2dtw12_from_fs8_lp1_to_mg(
						data_raw_acceleration[0]);
				data_acceleration[i][1] = lis2dtw12_from_fs8_lp1_to_mg(
						data_raw_acceleration[1]);
				data_acceleration[i][2] = lis2dtw12_from_fs8_lp1_to_mg(
						data_raw_acceleration[2]);
			}

		} else {

		}

		lis2dtw12_temperature_raw_get(&dev_ctx, &data_raw_temperature);
		temperature = lis2dtw12_from_lsb_to_celsius(data_raw_temperature);

		// Control green voltage
		float led_green_average = getAverageFloat(AFE_GREEN, LED_DEPTH);

		if (led_green_average > 0.9 && AFE_ILED_2 > 11) {
			AFE_ILED_2 -= 10;
			AFE_ILED_UPDATE();
			HR_COUNT = 0;

		}
		if (led_green_average < 0.4 && AFE_ILED_2 < 91) {
			AFE_ILED_2 += 10;
			AFE_ILED_UPDATE();
			HR_COUNT = 0;
		}

		// Add samples

		HR_heartMonitor_addGreen(HR_heartMonitor, AFE_GREEN, LED_DEPTH);
		HR_heartMonitor_addRedIr(HR_heartMonitor, AFE_RED, AFE_IR, LED_DEPTH);

		// If okay - calculate HR

		HR_heartMonitor_peaksFromGreen(HR_heartMonitor);
		HR = HR_heartMonitor_heartRateFromPeaks(HR_heartMonitor);
		if (!isnanf(HR)) {
			HR_DISP = 2.0 * HR_DISP / 3.0 + HR / 3.0;
		} else {
			HR = 0.0;
		}
		SP_R = HR_heartMonitor_ratioFromPeaks(HR_heartMonitor);
		if (isnanf(SP_R)) {
			SP_R = 0.0;
		}
		SP_DISP = 110.0 - 25.0 * SP_R;

		HR_COUNT++;

		// Check finger on sensor
		if (led_green_average < 0.5 && AFE_ILED_2 >= 91) {
			HR_FIN = 0;
		} else {
			HR_FIN = HR_DISP;
		}

		// Monitor displaying
		for (int i = 0; i < LED_DEPTH; i++) {
			HR_G = AFE_GREEN_COPY[i];
			HR_B = d1[i];
			HR_S = d2[i];
			HR_P = HR_heartMonitor->peaks[HR_heartMonitor->size - LED_DEPTH + i]
					* HR_G;

			acceleration_mg[0] = data_acceleration[i][0];
			acceleration_mg[1] = data_acceleration[i][1];
			acceleration_mg[2] = data_acceleration[i][2];

			SP_RED = AFE_RED[i];
			SP_IR = AFE_IR[i];
			HAL_Delay(30); // for monitor alignment
		}

		counter++;
		__HAL_TIM_SET_COUNTER(&htim2, 0);
		LED_OFF_tim = 0;
		LED_update(temperature, HR_DISP, SP_DISP);
		printf_bee_response();

		// fuzz
		// uint8_t tmp[3] = { counter % 128, counter + 5, counter - 10 };
		// BEE_b_fnin((uint8_t*) tmp, 3);

		tx_buff[0] = (uint8_t) HR_DISP;
		tx_buff[1] = (uint8_t) temperature;
		tx_buff[2] = (uint8_t) SP_DISP;

		BEE_b_fnin((uint8_t*) tx_buff, 3);

	}
#endif

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */

		/* USER CODE BEGIN 3 */

		// Button status
		FlagStatus butt_wakeup = RESET;
		// Data is not ready while sensor don't give correct data
		DATA_RDY = RESET;

		// Delay by INT of AFE
		while (HAL_GPIO_ReadPin(AFE_ADC_RDY_GPIO_Port, AFE_ADC_RDY_Pin)
				!= GPIO_PIN_SET) {
			// Wakeup button
			if (!HAL_GPIO_ReadPin(SB1_GPIO_Port, SB1_Pin)) {
				butt_wakeup = SET;
			}
		}

		// Check wakeup flag LIS
		lis2dtw12_all_sources_t all_source;

		// LED wake up
		if (HAL_GPIO_ReadPin(LIS_INT_GPIO_Port, LIS_INT_Pin) || butt_wakeup) {
			lis2dtw12_all_sources_get(&dev_ctx, &all_source);
			if (all_source.wake_up_src.wu_ia || butt_wakeup) {
				// Stop idle
				IDLE_timer = 0;
				IDLE = RESET;
				// Reset led idle timer
				LED_OFF_tim = 0;
				// Switch on led if off
				if (!LED_ON) {
					LED_on();
					LED_pribor();
					LED_ON = SET;
				}
			}
		}

		// Wakeup button can on afe's leds
		if (butt_wakeup) {
			__HAL_TIM_SET_COUNTER(&htim3, 0);
			AFE_status = SET;
			TPS_on();
		}

		// Afe's led on - read values
		if (AFE_status && !IDLE) {
			HR_COUNT++;
			// Read fifo
			AFE_FIFO_READ(LED_DEPTH * 4);
			for (uint16_t i = 0; i < LED_DEPTH; i++) {
				AFE_RED[i] = AFE_FLOAT[i * 4];
				AFE_IR[i] = AFE_FLOAT[1 + i * 4];
				AFE_GREEN[i] = AFE_FLOAT[2 + i * 4];
			}
			MA_process(ma_red, AFE_RED, LED_DEPTH);
			MA_process(ma_ir, AFE_IR, LED_DEPTH);

			// Read data from LIS2
			uint8_t val;
			lis2dtw12_fifo_data_level_get(&dev_ctx, &val);

			if (val >= ACC_DEPTH) {
				lis2dtw12_fifo_data_level_get(&dev_ctx, &val);
				data_acceleration_sum = 0;
				for (int i = ACC_DEPTH - 1; i >= 0; i--) {
					memset(data_raw_acceleration, 0x00, 3 * sizeof(int16_t));
					lis2dtw12_acceleration_raw_get(&dev_ctx,
							data_raw_acceleration);
					data_acceleration[i][0] = lis2dtw12_from_fs8_lp1_to_mg(
							data_raw_acceleration[0]);
					data_acceleration[i][1] = lis2dtw12_from_fs8_lp1_to_mg(
							data_raw_acceleration[1]);
					data_acceleration[i][2] = lis2dtw12_from_fs8_lp1_to_mg(
							data_raw_acceleration[2]);
					data_acceleration_sum += abs(data_acceleration[i][0])
							+ abs(data_acceleration[i][1])
							+ abs(data_acceleration[i][2]);
				}
				data_acceleration_sum /= ACC_DEPTH;
			}

			// control some acc to remove to sharp moves
			if (data_acceleration_sum > ACC_THRESHOLD) {
				HR_COUNT = 0;
			}

			// Get temperature
			lis2dtw12_temperature_raw_get(&dev_ctx, &data_raw_temperature);
			temperature = lis2dtw12_from_lsb_to_celsius(data_raw_temperature);

			// Control green voltage
			float led_green_average = getAverageFloat(AFE_GREEN, LED_DEPTH);

			if (led_green_average > 0.9 && AFE_ILED_2 > 11) {
				HR_OK = 0;
				AFE_ILED_2 -= 10;
				AFE_ILED_UPDATE();
				HR_COUNT = 0;

			}
			if (led_green_average < 0.4 && AFE_ILED_2 < 91) {
				HR_OK = 0;
				AFE_ILED_2 += 10;
				AFE_ILED_UPDATE();
				HR_COUNT = 0;
			}

			// Add samples to HR module
			HR_heartMonitor_addGreen(HR_heartMonitor, AFE_GREEN, LED_DEPTH);
			HR_heartMonitor_addRedIr(HR_heartMonitor, AFE_RED, AFE_IR,
			LED_DEPTH);

			// Now filers are stable and can find HR and SPO
			if (HR_COUNT > ROUNDS) {

				HR_heartMonitor_peaksFromGreen(HR_heartMonitor);

				// Data will be valid - data_rdy set
				DATA_RDY = SET;

				// Check heartrate
				HR = HR_heartMonitor_heartRateFromPeaks(HR_heartMonitor);
				if (!isnanf(HR)) {
					HR_DISP = 2.0 * HR_DISP / 3.0 + HR / 3.0;
				} else {
					HR = 0.0;
				}

				// Check SPO
				SP_R = HR_heartMonitor_ratioFromPeaks(HR_heartMonitor);
				if (isnanf(SP_R)) {
					SP_R = 0.0;
					SP_DISP = 0.0;
				} else {
					SP_DISP = 110.0 - 25.0 * SP_R;
					if (SP_DISP >= 100.0) {
						SP_DISP = 99.9;
					}
				}

				// Check for limits
				if (HR < HR_limit_high && HR > HR_limit_low
						&& SP_DISP > SpO_OK_low) {
					HR_OK++;
				} else {
					HR_OK = 0;
				}

			}

			// Check finger/wrist on sensor
			if (led_green_average < 0.5 && AFE_ILED_2 >= 91) {
				HR_OK = 0;
				HR_COUNT = 0;
				HR_FIN = 0;
				SP_DISP = 0;
				IDLE_timer++;
			} else {
				IDLE_timer = 0;
				HR_FIN = HR_DISP;
			}

			// No finger - can start idle
			if (IDLE_timer > TIME_TO_IDLE) {
				IDLE = SET;
				IDLE_timer = 0;
				AFE_status = RESET;
				TPS_off();
				LED_ON = RESET;
				LED_off();
			}

			// Values is good, can turn off led's and wait period before next session
			if (HR_OK > 10) {
				AFE_status = RESET;
				TPS_off();
			} else {
				// reset turning led's on timer
				__HAL_TIM_SET_COUNTER(&htim3, 0);
			}
		}

		// If have new data - send it in swarm
		if (DATA_RDY) {
			tx_buff[0] = (uint8_t) HR_DISP;
			tx_buff[1] = (uint8_t) temperature;
			tx_buff[2] = (uint8_t) SP_DISP;
			BEE_b_fnin(tx_buff, 3);
		}

		// If oled is on
		if (LED_ON && !IDLE && DATA_RDY) {
			LED_update(temperature, HR_FIN, SP_DISP);
		}
	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
	RCC_PeriphCLKInitTypeDef PeriphClkInit = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
	PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
	PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV8;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
		Error_Handler();
	}
}

/* USER CODE BEGIN 4 */

// tim2 = control led
// tim3 = control AFE power supply
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
	if (htim->Instance == TIM2) {
		if (LED_OFF_tim++ > LED_ON_TIME && LED_ON) {
			LED_off();
			LED_ON = RESET;
		}
	}
	if (htim->Instance == TIM3) {
		if (!IDLE) {
			TPS_on();
			AFE_status = SET;
		}

	}

}

// Switch on LEd and init it
void LED_on() {
	// RESET = 0
	HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_SET);
	// LED_ON = 1
	HAL_GPIO_WritePin(OLED_ON_GPIO_Port, OLED_ON_Pin, GPIO_PIN_SET);
	HAL_Delay(10);
	// INIT
	LED_init();
	// Show logo
	HAL_Delay(10);
}

// Switch off LED
void LED_off() {
	// RESET = 1
	HAL_GPIO_WritePin(OLED_RESET_GPIO_Port, OLED_RESET_Pin, GPIO_PIN_RESET);
	// LED_ON = 0
	HAL_GPIO_WritePin(OLED_ON_GPIO_Port, OLED_ON_Pin, GPIO_PIN_RESET);
}

// Switch on AFE LED supply
void TPS_on() {
	HR_OK = 0;
	HAL_GPIO_WritePin(TPS61099_EN_GPIO_Port, TPS61099_EN_Pin, GPIO_PIN_SET);
}

// Switch off AFE LED supply
void TPS_off() {
	HAL_GPIO_WritePin(TPS61099_EN_GPIO_Port, TPS61099_EN_Pin, GPIO_PIN_RESET);
}

float getAverageFloat(float *array, uint16_t size) {
	float sum = 0.0;
	for (uint16_t i = 0; i < size; i++) {
		sum += array[i];
	}
	return sum / size;
}

float findMin(float *array, uint16_t size) {
	float result = array[0];
	for (uint16_t i = 1; i < size; i++) {
		if (result > array[i]) {
			result = array[i];
		}
	}
	return result;
}

float findMax(float *array, uint16_t size) {
	float result = array[0];
	for (uint16_t i = 1; i < size; i++) {
		if (result < array[i]) {
			result = array[i];
		}
	}
	return result;
}

static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
		uint16_t len) {
	while (HAL_I2C_GetState(handle) != HAL_I2C_STATE_READY)
		;
	HAL_I2C_Mem_Write_DMA(handle, LIS2DTW12_I2C_ADD_L, reg,
	I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len);
	return 0;

}
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
		uint16_t len) {
	while (HAL_I2C_GetState(handle) != HAL_I2C_STATE_READY)
		;
	HAL_I2C_Mem_Read(handle, LIS2DTW12_I2C_ADD_L, reg,
	I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);
	return 0;
}

// linear approx between led_depth and acc_depth
void linear_approx(int16_t *src, int src_size, int16_t *dist, int dist_size) {
	float step = (float) (src_size - 1) / (dist_size - 1);

	for (int i = 0; i < dist_size; i++) {
		float index = i * step;
		int lower_index = (int) index;
		int upper_index = lower_index + 1;
		float weight = index - lower_index;
		dist[i] = src[lower_index] * (1 - weight) + src[upper_index] * weight;
	}
}

static void platform_delay(uint32_t ms) {
	HAL_Delay(ms);
}

static void platform_init(void) {
	return;
}

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	NVIC_SystemReset();

	__disable_irq();
	while (1) {
		HAL_Delay(10);

	}
	/* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param Har line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
