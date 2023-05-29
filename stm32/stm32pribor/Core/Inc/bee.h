/**
 * @file bee.h
 * @brief Header file for Swarm bee LE V3 using SWARM API v 3.0
 * @author Burkov Egor
 * @date 2022-11-14
 */

#ifndef INC_BEE_H_
#define INC_BEE_H_

#include "stm32f1xx_hal.h"

/**
 * @brief Size of the BEE arrays.
 *
 * This constant defines the size of the BEE arrays used for storing data and response.
 * It specifies the maximum number of elements that each array can hold.
 */
#define BEE_SIZE 128

/**
 * @brief SYNC byte
 *
 * Used in binary mode SWARM API. Should be first element in message.
 * Also if some byte same with this element, it should be escaped with special byte.
 */
#define BEE_SYNC 0x7F

extern uint8_t bee_response[BEE_SIZE];

void BEE_init(UART_HandleTypeDef *huart);

int BEE_TX_B(uint8_t *command);

// Radio config commands
void BEE_set_binary();

int BEE_b_sbiv(uint16_t blinkrate);
int BEE_b_sbiv_ret(uint16_t blinkrate);
int BEE_b_snid(uint8_t *id);
int BEE_b_snid_ret(uint8_t *id);
int BEE_b_gnid_ret(uint8_t *id);
int BEE_b_sset();
int BEE_b_sset_ret();
int BEE_b_rset();
int BEE_b_rset_ret();
int BEE_b_sfac();
int BEE_b_sfac_ret();
int BEE_b_stxp(uint8_t power);
int BEE_b_stxp_ret(uint8_t power);
int BEE_b_ssyc(uint8_t sync);
int BEE_b_ssyc_ret(uint8_t sync);
int BEE_b_gfwv();
int BEE_b_gfwv_ret(uint8_t *version);
int BEE_b_guid();
int BEE_b_guid_ret(uint8_t *id);
int BEE_b_suas(uint32_t speed);
int BEE_b_suas_ret(uint32_t *speed);
int BEE_b_eair(uint8_t enable);
int BEE_b_eair_ret(uint8_t enable);

// Communication commands

int BEE_b_fnin(uint8_t *payload, uint16_t size);
int BEE_b_fnin_ret(uint8_t *payload, uint16_t size);
int BEE_b_edan(uint8_t notify);
int BEE_b_edan_ret(uint8_t notify);
int BEE_b_sdat(uint8_t option, uint8_t *id, uint8_t len, uint8_t *data,
		uint16_t timeout);
int BEE_b_sda_ret(uint8_t option, uint8_t *id, uint8_t len, uint8_t *data,
		uint16_t timeout);
int BEE_b_gdat();
int BEE_b_gdat_ret(uint8_t *bytes_size, uint8_t *id, uint8_t *bytes);
int BEE_b_bdat(uint8_t option, uint8_t len, uint8_t *data, uint16_t timeout);
int BEE_b_bdat_ret(uint8_t option, uint8_t len, uint8_t *data, uint16_t timeout,
		uint8_t *payload_id);
int BEE_b_frad(uint8_t len, uint8_t *data, FlagStatus get);
int BEE_b_frad_ret(uint8_t len, uint8_t *data, FlagStatus get);
int BEE_b_eidn(uint8_t notify);
int BEE_b_eidn_ret(uint8_t notify);
;

// utils
void BEE_deescape_response();
int status_helper(int status);
void BEE_clear_response();
void printf_bee_response();

#endif  /* INC_BEE_H_*/
