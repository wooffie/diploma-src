/*
 * led_helper.h
 *
 *  Created on: 21 нояб. 2022 г.
 *      Author: wooffie
 */

#ifndef INC_LEDHELPER_H_
#define INC_LEDHELPER_H_

#include "stm32f1xx_hal.h"


void LED_init();
void LED_update(uint8_t t, uint8_t h, uint8_t o);
void LED_pribor();

void inc_led_status();

#endif /* INC_LEDHELPER_H_ */
