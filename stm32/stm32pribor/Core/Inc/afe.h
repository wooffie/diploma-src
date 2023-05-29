/**
 * @file afe.h
 * @brief Header file for AFE interacting using SPI
 * @author Burkov Egor
 * @date 2022-11-17
 */

#ifndef INC_AFE_H_
#define INC_AFE_H_

#include "stm32f1xx_hal.h"

/**
 * @brief Depth of the FIFO.
 *
 * This constant defines the depth of the FIFO (First-In-First-Out) buffer.
 * It specifies the maximum number of elements that the FIFO can hold.
 */
#define FIFO_DEPTH 128

extern float AFE_FLOAT[FIFO_DEPTH];

/**
 * @brief AFE_ILED_1, AFE_ILED_2, AFE_ILED_3, AFE_ILED_4 variables.
 *
 * These variables represent the current values of the AFE (Analog Front End) LED channels.
 * They are of type uint8_t and are used to control the current levels of the LED channels.
 * Note: The valid range for these variables is dependent on the specific hardware implementation.
 */
extern uint8_t AFE_ILED_1, AFE_ILED_2, AFE_ILED_3, AFE_ILED_4;
extern uint8_t AFE_ILED_1, AFE_ILED_2, AFE_ILED_3, AFE_ILED_4;

void AFE_INIT(SPI_HandleTypeDef * hspi);

void AFE_REG_INIT();

void AFE_ILED_UPDATE();
void AFE_LED_STRENGTH(uint8_t iled1, uint8_t iled2, uint8_t iled3,
		uint8_t iled4);

void AFE_READ_ENABLE();

void AFE_READ_DISABLE();

void AFE_FIFO_READ(uint16_t size);

void AFE_WRITE(uint8_t reg_address, unsigned long data);

unsigned long AFE_READ(uint8_t reg_address);

#endif /* INC_AFE_H_ */
