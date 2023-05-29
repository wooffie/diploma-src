/*
 * AFE.c
 *
 *  Created on: Nov 17, 2022
 *      Author: wooffie
 */

#include <afe.h>
#include "spi.h"

/**
 * @def AFE_SPI_DIS()
 * @brief Disable AFE SPI communication.
 *
 * This macro is used to disable the SPI communication with the AFE (Analog Front-End) module.
 */
#define AFE_SPI_DIS() HAL_GPIO_WritePin(AFE_SPI_EN_GPIO_Port, AFE_SPI_EN_Pin, GPIO_PIN_SET)
/**
 * @def AFE_SPI_EN()
 * @brief Enable AFE SPI communication.
 *
 * This macro is used to enable the SPI communication with the AFE (Analog Front-End) module.
 */
#define AFE_SPI_EN() HAL_GPIO_WritePin(AFE_SPI_EN_GPIO_Port, AFE_SPI_EN_Pin, GPIO_PIN_RESET)

/* SPI handle for AFE communication */
SPI_HandleTypeDef *afe_spi;
/**
 * @brief Array for AFE floating-point data.
 *
 * This array holds ADC result data from AFE.
 * The size of the array is defined by FIFO_DEPTH.
 */
float AFE_FLOAT[FIFO_DEPTH];

/**
 * @brief First AFE ILED variable.
 *
 * This variable represents the first AFE ILED value (TX1 LED).
 */
uint8_t AFE_ILED_1;
/**
 * @brief Second AFE ILED variable.
 *
 * This variable represents the second AFE ILED value (TX2 LED).
 */
uint8_t AFE_ILED_2;

/**
 * @brief Third AFE ILED variable.
 *
 * This variable represents the third AFE ILED value (TX3 LED).
 */
uint8_t AFE_ILED_3;

/**
 * @brief Fourth AFE ILED variable.
 *
 * This variable represents the fourth AFE ILED value (TX4 LED).
 */
uint8_t AFE_ILED_4;

/**
 * @brief Enables read from AFE
 *
 * This function enables the read operation from the AFE  device by writing
 * a bit to the control register at address 0x00.
 */
void AFE_READ_ENABLE() {
	AFE_WRITE(0x00, 0x000041);
}

/**
 * @brief Disables read from AFE
 *
 * This function enables the read operation from the AFE  device by writing
 * a zero bit to the control register at address 0x00.
 */
void AFE_READ_DISABLE() {
	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/
}

/**
 * @brief Writes data to the AFE register
 *
 * This function writes the provided data to the specified register address of the AFE (Analog Front-End) device.
 *
 * @param reg_address The address of the register to write to.
 * @param data The data to be written to the register.
 */
void AFE_WRITE(uint8_t reg_address, unsigned long data) {
	AFE_SPI_EN();

	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/

	AFE_SPI_DIS();
}

/**
 * @brief Reads data from the AFE register
 *
 * This function reads the data from the specified register address of the AFE device.
 *
 * @param reg_address The address of the register to read from.
 * @return Register value.
 */
unsigned long AFE_READ(uint8_t reg_address) {


	AFE_SPI_EN();

	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/

	AFE_SPI_DIS();

	return result;
}

/**
 * @brief Initializes the AFE module
 *
 * This function initializes the AFE module by configuring the SPI interface and performing
 * the necessary initialization steps for proper operation.
 *
 * @param hspi Pointer to the SPI handle structure.
 */
void AFE_INIT(SPI_HandleTypeDef *hspi) {
	afe_spi = hspi;

	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/

	AFE_REG_INIT();
}

/**
 * @brief Initializes the AFE (Analog Front-End) register settings
 *
 * This function initializes the AFE register settings by writing the appropriate values to
 * the AFE registers. It configures various parameters such as FIFO, window size, phases,
 * and LED settings.
 */
void AFE_REG_INIT() {
	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/
}

/**
 * @brief Sets the LED strength for AFE
 *
 * This function sets the LED strength for the AFE by updating the values of `AFE_ILED_1`,
 * `AFE_ILED_2`, `AFE_ILED_3`, and `AFE_ILED_4`. It then calls the `AFE_ILED_UPDATE()`
 * function to apply the updated LED strengths.
 *
 * @param iled1 The LED strength for LED 1
 * @param iled2 The LED strength for LED 2
 * @param iled3 The LED strength for LED 3
 * @param iled4 The LED strength for LED 4
 */
void AFE_LED_STRENGTH(uint8_t iled1, uint8_t iled2, uint8_t iled3,
		uint8_t iled4) {
	AFE_ILED_1 = iled1;
	AFE_ILED_2 = iled2;
	AFE_ILED_3 = iled3;
	AFE_ILED_4 = iled4;
	AFE_ILED_UPDATE();
}

/**
 * @brief Updates the LED strengths for AFE
 *
 * This function updates the LED strengths for the AFE by writing the values of `AFE_ILED_1`,
 * `AFE_ILED_2`, `AFE_ILED_3`, and `AFE_ILED_4` to the corresponding registers.
 */
void AFE_ILED_UPDATE() {
	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/
}


/**
 * @brief Reads data from the AFE FIFO
 *
 * This function reads data from the AFE FIFO by sending a command to the AFE and receiving the
 * data via SPI. The size parameter determines the number of samples to read from the FIFO.
 * The received data is stored in the AFE_Buffer array, and then converted to floating-point
 * values and stored in the AFE_FLOAT array.
 *
 * @param size The number of samples to read from the FIFO
 */
void AFE_FIFO_READ(uint16_t size) {
	/*
		This part of code can't be shown cause of Texas Instruments NDA restriction
	*/
}

