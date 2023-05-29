/**
 * @file bee.c
 * @brief Source code of functions for using SWARM API
 * @author Burkov Egor
 * @date 2022-11-14
 */

#include "bee.h"
#include "stdio.h"
#include "crc.h"
#include "usart.h"
#include "stm32f1xx_hal.h"
#include <string.h>

uint16_t getCRC(uint8_t *Buffer, uint16_t BufferSize);

/**
 * @brief Array for storing the response.
 *
 * This array is used for storing the response received from the BEE module.
 * It has a size of BEE_SIZE.
 */
uint8_t bee_response[BEE_SIZE];

/**
 * @brief Pointer for filling the response.
 *
 * This variable is used as a pointer to indicate the position for filling the response array.
 */
uint16_t response_pointer;

/**
 * @brief Response status flag.
 *
 * This flag indicates whether the response is ready to be read or not.
 */
FlagStatus response_rdy;

/**
 * @brief Array for sending data.
 *
 * This array is used for storing the data to be sent via the BEE module.
 * It has a size of BEE_SIZE.
 */
uint8_t bee_payload[BEE_SIZE];
/**
 * @brief Buffer for interrupt-safe data storage.
 *
 * This buffer is used for temporary storage of data to avoid violating the bee_payload during interrupts.
 * It has a size of BEE_SIZE.
 */
uint8_t bee_buffer[BEE_SIZE];

/**
 * @brief UART handler for BEE module.
 *
 * This variable holds the UART handler for communication with the BEE module.
 */
UART_HandleTypeDef *bee_uart;

/**
 * @brief  Tx Transfer completed callbacks
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module
 * @retval None
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == bee_uart->Instance) {
		HAL_UART_AbortReceive_IT(bee_uart);
		HAL_UART_AbortReceiveCpltCallback(bee_uart);
		response_pointer = 0;
		response_rdy = RESET;
		bee_response[1] = 0xFF;
		HAL_UART_Receive_IT(bee_uart, (uint8_t*) bee_response, 1);
	}

}


/**
 * @brief  Rx Transfer completed callbacks
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == bee_uart->Instance) {
		response_pointer++;
		if (response_pointer > bee_response[1] + 3) {
			response_rdy = SET;
			return;
		}
		if(response_pointer >= 3 && bee_response[response_pointer-2] == 0x1B){ // escaped ESC
			if(bee_response[response_pointer-1] == 0x53) {// escaped SYN
				bee_response[response_pointer-2] = BEE_SYNC;
			}
			response_pointer--;
		}
		HAL_UART_Receive_IT(bee_uart, &bee_response[response_pointer], 1);
	}
}

/**
 * @brief  Define UART for swarm bee module
 * @param  huart Pointer to a UART_HandleTypeDef structure that contains
 *               the configuration information for the specified UART module
 * @retval None
 */
void BEE_init(UART_HandleTypeDef *huart) {
	bee_uart = huart;
}

/**
 * @brief  Send command to swarm bee module in binary mode
 * @note   Synchronization word and CRC will be pasted in this function
 * @param  command Pointer to command
 * @param  Size  Amount of bytes in command array
 * @retval int Zero - transmission started, otherwise error code
 */
int BEE_TX_B(uint8_t *command) {
	uint16_t size = command[0] + 1;
	if (size > BEE_SIZE - 3)
		return -1;
	uint16_t payload_size = size + 3;
	memcpy(&bee_payload[1], command, size);
	bee_payload[0] = BEE_SYNC;
	uint16_t crc = getCRC(bee_payload, payload_size);
	bee_payload[payload_size - 2] = (crc) & 0xFF;
	bee_payload[payload_size - 1] = (crc >> 8) & 0xFF;

	uint16_t i = 1;
	while (i != BEE_SIZE && i != payload_size) {
		if (bee_payload[i] == 0x1B) {
			memmove(&bee_payload[i + 1], &bee_payload[i],
					(payload_size - i) * sizeof(uint8_t));
			bee_payload[i + 1] = 0x45;
			payload_size++;
		}
		if (bee_payload[i] == BEE_SYNC) {
			memmove(&bee_payload[i + 1], &bee_payload[i],
					(payload_size - i) * sizeof(uint8_t));
			bee_payload[i] = 0x1B;
			bee_payload[i + 1] = 0x53;
			payload_size++;
		}
		i++;
	}
	if (i == BEE_SIZE)
		return -2;
	BEE_clear_response();
	HAL_UART_Transmit_IT(bee_uart, (uint8_t*) bee_payload, payload_size);
	return 0;
}

/**
 * @brief  Clear response array
 * @retval None
 */
void BEE_clear_response() {
	for (int i = 0; i < BEE_SIZE; i++) {
		bee_response[i] = 0x00;
	}
}

/**
 * @brief  Set swarm bee module from ASCII into binary mode
 * @note   Do nothing if module is already in binary mode
 * @retval None
 */
void BEE_set_binary() {
	HAL_UART_Transmit_IT(bee_uart, (uint8_t*) "SBIN\r\n", (uint16_t) 6);
}

/**
 * @brief Sets the broadcast interval for sending the Node ID.
 *
 * This function sets the broadcast interval in which the Node ID will be sent. The blink interval is specified
 * in milliseconds [ms]. The return value indicates the transmission status.
 *
 * @param blinkrate The blink interval in milliseconds [ms].
 * @return The transmission status.
 */
int BEE_b_sbiv(uint16_t blinkrate) {
	uint8_t sbiv[] = { 0x04, 0x55, 0x31, 0x00, 0x00 };
	uint16_t size = 5;
	sbiv[size - 2] = (blinkrate >> 8) & 0xFF;
	sbiv[size - 1] = (blinkrate) & 0xFF;
	return BEE_TX_B(sbiv);
}

/**
 * @brief Sets the broadcast interval for sending the Node ID and returns the result.
 *
 * This function sets the broadcast interval for sending the Node ID by calling the BEE_b_sbiv() function
 * internally. It returns the result of the operation or the error code if the transmission was not successful.
 *
 * @param blinkrate The blink interval in milliseconds [ms].
 * @return The result of the operation or the error code if the transmission was not successful.
 */
int BEE_b_sbiv_ret(uint16_t blinkrate) {
	response_rdy = RESET;
	int tx_status = BEE_b_sbiv(blinkrate);
	if (tx_status)
		return tx_status;
	while (!response_rdy)
		;
	BEE_deescape_response();
	if (getCRC(bee_response, response_pointer) != 0)
		return -2;
	return (bee_response[4] << 8) + bee_response[5];
}

/**
 * @brief Fills the data buffer of each node ID notification with the specified payload.
 *
 * This function fills the data buffer of each node ID notification with the provided payload data of length
 * `size`. The payload data will be transmitted with every node ID notification until deleted by the host
 * application. The data is not visible for swarm devices as no *DNO is generated. This command can be used
 * to provide data to other products such as TDOA RTLS.
 *
 * @param payload The payload data to be transmitted.
 * @param size The size of the payload data.
 * @return The transmission status.
 */
int BEE_b_fnin(uint8_t *payload, uint16_t size) {
	bee_buffer[0] = 3 + size;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x28;
	bee_buffer[3] = size;
	for (int i = 0; i < size; i++) {
		bee_buffer[4 + i] = payload[i];
	}
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Fills the data buffer of each node ID notification with the specified payload and returns the result.
 *
 * This function fills the data buffer of each node ID notification with the provided payload data of length
 * `size` by calling the BEE_b_fnin() function internally. It returns the result of the transmission or the
 * error code if the transmission was not successful.
 *
 * @param payload The payload data to be transmitted.
 * @param size The size of the payload data.
 * @return The error code of the transmission or the error code of transmission that was not successful.
 */
int BEE_b_fnin_ret(uint8_t *payload, uint16_t size) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_fnin(payload, size));
	if (status)
		return status;
	return bee_response[4];
}

// swarm radio setup commands

/**
 * @brief Sets the Node ID of the swarm node
 *
 * This function sets the Node ID of the swarm node to the specified ID. The ID parameter is a
 * 6-byte array representing the Node ID. If the ID is set to all zeros (000000000000), the original
 * unique Node ID, which was set during device production, is reset (if supported by the microcontroller).
 * The valid range for the ID is 000000000000 to FFFFFFFFFFFE.
 *
 * @param id The 6-byte Node ID to set
 * @return The code of the error occurred during the operation
 */
int BEE_b_snid(uint8_t *id) {
	bee_buffer[0] = 0x08; // len
	bee_buffer[1] = 0x55; // set
	bee_buffer[2] = 0x00; // cmd
	for (int i = 0; i < 6; i++) {
		bee_buffer[3 + i] = id[i];
	}
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Returns the Node ID of the swarm node
 *
 * This function retrieves the current Node ID of the swarm node and writes it into the provided id
 * parameter, which is a 6-byte array. The function sends a command to the swarm node and waits for
 * the response. The response is then de-escaped and the Node ID is extracted from it and written
 * into the id parameter. The function returns an error code indicating the success or failure of
 * the operation.
 *
 * @param id The 6-byte array to store the given and retrieved Node ID
 * @return The code of the error occurred during the operation
 */
int BEE_b_snid_ret(uint8_t *id) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_snid(id));
	if (status)
		return status;
	for (int i = 0; i < 6; i++) {
		id[i] = bee_response[3 + i];
	}
	return 0;
}

/**
 * @brief Readout of configured Node ID of node connected to host
 *
 * This function retrieves the configured Node ID of the node connected to the host. The Node ID
 * is written into the provided id parameter, which is a 6-byte array. The function sends a command
 * to the connected node and waits for the response. The response is then processed, and the Node ID
 * is extracted from it and written into the id parameter.
 *
 * @param id The 6-byte array to store the retrieved Node ID
 * @return The code of the error occurred during the TX operation
 */
int BEE_b_gnid_ret(uint8_t *id) {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x54;
	bee_buffer[2] = 0x00;
	response_rdy = RESET;
	int status = status_helper(BEE_TX_B(bee_buffer));
	if (status)
		return status;
	for (int i = 0; i < 6; i++) {
		id[i] = bee_response[3 + i];
	}
	return 0;
}

/**
 * @brief Saves all settings including Node ID permanently to EEPROM and returns the result.
 *
 * This function saves all settings, including the Node ID, permanently to the EEPROM. It should be
 * called after changing settings if they need to persist through a power cycle or switch off state.
 * The function sends a command to the connected node to initiate the saving operation. It waits for
 * the response and returns the result of the saving operation.
 *
 * @return The code of the error occurred during the TX operation
 */
int BEE_b_sset() {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x01;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Saves all settings including Node ID permanently to EEPROM and returns the result.
 *
 * This function saves all settings, including the Node ID, permanently to the EEPROM. It should be
 * called after changing settings if they need to persist through a power cycle or switch off state.
 * The function sends a command to the connected node to initiate the saving operation. It waits for
 * the response and returns the result of the saving operation.
 *
 * @return The result of the saving operation or the error code if the transmission was not successful
 *         - 0: Saving of all parameters successfully verified
 *         - 1: Saving of parameters not successful; verification failed
 *         - Error codes returned by BEE_TX_B() if the transmission was not successful
 */
int BEE_b_sset_ret() {
	response_rdy = RESET;
	int status = status_helper(BEE_b_sset());
	if (status)
		return status;
	return bee_response[4];
}

/**
 * @brief Restores all parameter settings from EEPROM.
 *
 * This function sends a command to the connected node to initiate the restoring operation of all parameter settings
 * from EEPROM. The return value indicates the transmission status.
 *
 * @return The transmission status
 */
int BEE_b_rset() {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x02;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Restores all parameter settings from EEPROM and returns the result.
 *
 * This function restores all parameter settings from EEPROM by calling the BEE_rset() function internally. It
 * returns the result of the restoring operation or the error code if the transmission was not successful.
 *
 * @return The result of the restoring operation or the error code if the transmission was not successful
 */
int BEE_b_rset_ret() {
	response_rdy = RESET;
	int status = status_helper(BEE_b_rset());
	if (status)
		return status;
	return bee_response[4];
}

/**
 * @brief Resets the device configuration to factory settings.
 *
 * This function sends a command to the device to reset its configuration to factory settings. The return value
 * indicates the transmission status.
 *
 * @return The transmission status
 */
int BEE_b_sfac() {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x03;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Resets the device configuration to factory settings and returns the result.
 *
 * This function resets the device configuration to factory settings by calling the BEE_b_sfac() function
 * internally. It returns the result of the configuration or the error code if the transmission was not
 * successful.
 *
 * @return The result of the reset or the error code if the transmission was not successful
 */
int BEE_b_sfac_ret() {
	response_rdy = RESET;
	int status = status_helper(BEE_b_sfac());
	if (status)
		return status;
	return bee_response[4];
}

/**
 * @brief Sets the transmission power of the node.
 *
 * This function sets the transmission power of the node. The power value should be in the range of 0 to 63,
 * where 0 represents the minimum power and 63 represents the maximum power. The return value indicates the
 * transmission power that was set.
 *
 * @param power The transmission power value
 * @return The transmission status
 */
int BEE_b_stxp(uint8_t power) {
	if (power > 63)
		return -3;
	bee_buffer[0] = 0x03;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x05;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Sets the transmission power of the node and returns the result.
 *
 * This function sets the transmission power of the node by calling the BEE_b_stxp() function internally. It
 * returns the result of the power setting or the error code if the transmission was not successful.
 *
 * @param power The transmission power value
 * @return The result of the power setting or the error code if the transmission was not successful
 */
int BEE_b_stxp_ret(uint8_t power) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_stxp(power));
	if (status)
		return status;
	return bee_response[4];
}

/**
 * @brief Sets the node's PHY syncword.
 *
 * This function sets the PHY syncword of the node to the specified value. The syncword determines which messages
 * the node will listen to. Only messages with the same syncword will be processed by the node. The syncword value
 * should be in the range of 0 to 12. The return value indicates the transmission status.
 *
 * @param sync The syncword value
 * @return The transmission status
 */
int BEE_b_ssyc(uint8_t sync) {
	if (sync > 12)
		return -3;
	bee_buffer[0] = 0x03;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x06;
	bee_buffer[3] = sync;
	return BEE_TX_B(bee_buffer);
}

/*
 * @brief Sets the node's PHY syncword and returns the transmission status.
 *
 * This function sets the PHY syncword of the node to the specified value by calling the `BEE_b_ssyc()` function internally.
 * It returns the transmission status of the operation.
 *
 * @param sync The syncword value
 * @return The syncword that was set
 */
int BEE_b_ssyc_ret(uint8_t sync) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_ssyc(sync));
	if (status)
		return status;
	return bee_response[4];
}

/**
 * @brief Returns the firmware version of the swarm Node.
 *
 * This function sends a command to the connected node to retrieve the firmware version. The firmware version
 * is a 5-part version number consisting of major number, minor number, sub-minor, release candidate number, and
 * the number of changes after the last tag. Example: ver2.0.3-rc0-98 is translated to 0x02003062.
 *
 * @return The transmission status of the command
 */
int BEE_b_gfwv() {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x54;
	bee_buffer[2] = 0x08;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Returns the firmware version of the swarm Node and writes it to the specified version array.
 *
 * This function retrieves the firmware version by calling the BEE_b_gfwv() function internally. The firmware
 * version is written to the provided version array, where each element corresponds to a part of the version.
 *
 * @param version Pointer to the array where the firmware version will be written
 * @return The firmware version status or the transmission status if the command was not successful
 */
int BEE_b_gfwv_ret(uint8_t *version) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_gfwv());
	if (status)
		return status;
	for (int i = 0; i < 4; i++) {
		version[i] = bee_response[4 + i];
	}
	return 0;
}

/*
 * Description: Read the unique ID of µC.
 Return value description: =<UID>
 <UID>
 Range: 0 … FFFFFFFFFFFFFFFFFFFFFFFF
 */
int BEE_b_guid() {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x54;
	bee_buffer[2] = 0x09;
	return BEE_TX_B(bee_buffer);
}

int BEE_b_guid_ret(uint8_t *id) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_guid());
	if (status)
		return status;
	for (int i = 0; i < 12; i++) {
		id[i] = bee_response[4 + i];
	}
	return 0;
}

/**
 * @brief Sets the UART bit rate.
 *
 * This function sets the UART bit rate to the desired speed. Please note that the actual achievable speed
 * may differ from the desired speed, and the closest possible value will be configured and returned. The
 * error in the configured speed will be within 1% of the desired value. For example, if the desired speed
 * is 115200 Bps, the configured speed may be 115108 Bps, which corresponds to a 0.08% error.
 *
 * @param speed The desired UART bit rate
 * @return The transmission status of the command
 */

int BEE_b_suas(uint32_t speed) {
	bee_buffer[0] = 0x06;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x0a;
	bee_buffer[3] = speed >> 24;
	bee_buffer[4] = speed >> 16;
	bee_buffer[5] = speed >> 8;
	bee_buffer[6] = speed;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Sets the UART bit rate and retrieves the configured speed.
 *
 * This function sets the UART bit rate by calling the BEE_b_suas() function internally. It also retrieves
 * the configured speed and writes it to the provided speed pointer.
 *
 * @param speed Pointer to the variable where the configured UART speed will be written
 * @return The status of the UART configuration or the transmission status if the command was not successful
 */
int BEE_b_suas_ret(uint32_t *speed) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_suas(*speed));
	if (status)
		return status;
	*speed = ((((((bee_response[4] << 8) + bee_response[3]) << 8)
			+ bee_response[2]) << 8) + bee_response[1]);
	return 0;
}

/**
 * @brief Enable or disable the backchannel air interface.
 *
 * This function enables or disables the backchannel air interface of the swarm bee device. When enabled,
 * the node will respond to AIR interface read/write commands over the air interface. When disabled, the
 * node will not respond to such commands. Please note that for security reasons, this command can only
 * be used over the UART and not over the air interface.
 *
 * @param enable Flag indicating whether to enable or disable the backchannel air interface (0 = disable, 1 = enable)
 * @return The transmission status of the command or an error code if the enable parameter is invalid
 */
int BEE_b_eair(uint8_t enable) {
	if (enable != 1 && enable != 0)
		return -3;
	bee_buffer[0] = 0x03;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x0c;
	bee_buffer[3] = enable;
	return BEE_TX_B(bee_buffer);
}
/**
 * @brief Enable or disable the backchannel air interface and retrieve the status.
 *
 * This function enables or disables the backchannel air interface by calling the BEE_b_eair() function
 * internally. It also retrieves the status of the backchannel air interface and returns it.
 *
 * @param enable Flag indicating whether to enable or disable the backchannel air interface (0 = disable, 1 = enable)
 * @return The status of the backchannel air interface or the transmission status of the command if the enable parameter is invalid
 */
int BEE_b_eair_ret(uint8_t enable) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_eair(enable));
	if (status)
		return status;

	return bee_response[4];
}
// data communication commands

/**
 * @brief Enable or disable data notification.
 *
 * This function enables or disables the data notification feature of the swarm bee device. When enabled,
 * the node will trigger the host when a data packet has been received. When disabled, the node will not
 * trigger the host for data packet reception. The parameter 'notify' indicates whether to enable (1) or
 * disable (0) data notification.
 *
 * @param notify Flag indicating whether to enable or disable data notification (0 = disable, 1 = enable)
 * @return The transmission status of the command or an error code if the notify parameter is invalid
 */
int BEE_b_edan(uint8_t notify) {
	if (notify != 1 && notify != 0) {
		return -3;
	}
	bee_buffer[0] = 0x03;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x20;
	bee_buffer[3] = 0x01;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Enable or disable data notification and retrieve the status.
 *
 * This function enables or disables the data notification feature by calling the BEE_b_edan() function
 * internally. It also retrieves the status of the data notification and returns it.
 *
 * @param notify Flag indicating whether to enable or disable data notification (0 = disable, 1 = enable)
 * @return The status of the data notification or the transmission status of the command if the notify parameter is invalid
 */
int BEE_b_edan_ret(uint8_t notify) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_edan(notify));
	if (status)
		return status;
	return bee_buffer[4];
}

/**
 * @brief Sends user payload to a node with the specified options.
 *
 * This function sends the user payload to the specified node. The transmission can occur immediately or
 * after the node's blink, depending on the selected option.
 *
 * @param option   The transmission option.
 *                 Range: 0 ... 1
 *                 0 = Immediate transmission
 *                 1 = Wait for node blink before transmitting
 *                 Note: The receiver must be enabled to receive node blinks. Disable power saving (SPSA 0)
 *                 to receive asynchronous node blinks.
 * @param id       The 6-byte Node ID of the target node.
 *                 Note: If ID is set to all FF (0xFFFFFFFFFFFF), it will send always after receiving a node ID
 *                 notification until the timeout.
 *                 Range: 000000000001 ... FFFFFFFFFFFE
 * @param len      The length of the payload in bytes (hex).
 *                 Range: 01 ... 80
 * @param data     The payload to be transmitted.
 *                 Range: 00 ... FF
 * @param timeout  The maximum time in milliseconds to wait for the node's blink.
 *                 Range: 0 ... 65000
 * @return         0 on success, an error code otherwise.
 */
int BEE_b_sdat(uint8_t option, uint8_t *id, uint8_t len, uint8_t *data,
		uint16_t timeout) {
	bee_buffer[0] = 0x0B + len;
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x21;
	bee_buffer[3] = option;
	for (int i = 0; i < 6; i++) {
		bee_buffer[3 + i] = id[i];
	}
	bee_buffer[9] = len;
	for (int i = 0; i < len; i++) {
		bee_buffer[10 + i] = data[i];
	}
	bee_buffer[10 + len] = timeout >> 8;
	bee_buffer[11 + len] = timeout;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Sends user payload to a node and checks the transmission status.
 *
 * This function sends the user payload to the specified node using the specified options. It also checks
 * the status of the transmission and returns the result.
 *
 * @param option   The transmission option.
 *                 Range: 0 ... 1
 *                 0 = Immediate transmission
 *                 1 = Wait for node blink before transmitting
 *                 Note: The receiver must be enabled to receive node blinks. Disable power saving (SPSA 0)
 *                 to receive asynchronous node blinks.
 * @param id       The 6-byte Node ID of the target node.
 *                 Note: If ID is set to all FF (0xFFFFFFFFFFFF), it will send always after receiving a node ID
 *                 notification until the timeout.
 *                 Range: 000000000001 ... FFFFFFFFFFFE
 * @param len      The length of the payload in bytes (hex).
 *                 Range: 01 ... 80
 * @param data     The payload to be transmitted.
 *                 Range: 00 ... FF
 * @param timeout  The maximum time in milliseconds to wait for the node's blink.
 *                 Range: 0 ... 65000
 * @return         0 on success, an error code otherwise.
 */
int BEE_b_sda_ret(uint8_t option, uint8_t *id, uint8_t len, uint8_t *data,
		uint16_t timeout) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_sdat(option, id, len, data, timeout));
	if (status)
		return status;
	// check payload by youself!
	return 0;
}

/**
 * @brief Gets data: Reads out transmitted data.
 *
 * This function reads out any pending transmitted data. It returns the number of bytes in the pending message,
 * the ID of the node that sent the message, and the payload itself.
 *
 * @return The number of bytes in the pending message.
 *         Range: 00 ... 80
 *         00 = No pending message available
 *         01 ... 80 = Number of bytes in the message
 *
 *
 * @return 0 on success, an error code otherwise.
 */
int BEE_b_gdat() {
	bee_buffer[0] = 0x02;
	bee_buffer[1] = 0x54;
	bee_buffer[2] = 0x27;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Gets data: Reads out transmitted data and checks the status.
 *
 * This function reads out any pending transmitted data, checks the status of the operation, and returns the
 * result. It returns the number of bytes in the pending message, the ID of the node that sent the message, and
 * the payload itself.
 *
 * @param bytes_size  Pointer to a variable to store the number of bytes in the message.
 * @param id          Pointer to an array to store the ID of the node that sent the message.
 * @param bytes       Pointer to an array to store the received payload.
 *
 * @return 0 on success, an error code otherwise.
 */
int BEE_b_gdat_ret(uint8_t *bytes_size, uint8_t *id, uint8_t *bytes) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_gdat());
	if (status)
		return status;
	*bytes_size = bee_response[4];
	if (*bytes_size == 0)
		return 0;
	for (int i = 0; i < 6; i++) {
		id[i] = bee_response[5 + i];
	}
	for (int i = 0; i < *bytes_size; i++) {
		bytes[i] = bee_response[11 + i];
	}

	return 0;
}

/**
 * @brief Broadcasts data to nodes.
 *
 * This function broadcasts data to nodes according to the specified options. The options include immediate
 * broadcast transmission, transmission after any node ID blink, or canceling the broadcasting. The function
 * returns the status of the transmission.
 *
 * @param option    The broadcast option:
 *                  - 0: Immediate broadcast transmission
 *                  - 1: Send the packet after any node ID blink
 *                  - 2: Cancel broadcasting
 * @param len       The length of the payload in bytes (hex).
 *                  Range: 01 ... 70
 * @param data      The payload to be transmitted.
 *                  Range: 00 ... FF
 * @param timeout   The timeout value in milliseconds during which the node transmits after a node ID blink.
 *                  Range: 0 ... 65000
 *                  0 = Timeout disabled, broadcast will always occur.
 *                  > 0 = Time in milliseconds during which the node transmits after a node ID blink.
 *
 * @return The status of the transmission:
 *         Range: 0 ... 1
 *         0 = Successfully transmitted
 *         1 = Transmission failed
 *
 *         If option is 2, the function always returns 0.
 */
int BEE_b_bdat(uint8_t option, uint8_t len, uint8_t *data, uint16_t timeout) {
	bee_buffer[1] = 0x55;
	bee_buffer[2] = 0x22;
	bee_buffer[3] = option;
	bee_buffer[4] = len;
	if (option == 0) {
		bee_buffer[0] = 0x04 + len;
		for (int i = 0; i < len; i++) {
			bee_buffer[5 + i] = data[i];
		}

	} else if (option == 1) {
		bee_buffer[0] = 0x06 + len;
		for (int i = 0; i < len; i++) {
			bee_buffer[5 + i] = data[i];
		}
		bee_buffer[5 + len] = timeout >> 8;
		bee_buffer[6 + len] = timeout;
	} else if (option == 2) {
		bee_buffer[0] = 0x03;
	} else {
		return -3;
	}

	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Broadcasts data to nodes and checks the status.
 *
 * This function broadcasts data to nodes according to the specified options, checks the status of the
 * transmission, and returns the result. The options include immediate broadcast transmission, transmission
 * after any node ID blink, or canceling the broadcasting. The function returns the status of the transmission
 * and the payload ID if the option is 1.
 *
 * @param option        The broadcast option:
 *                      - 0: Immediate broadcast transmission
 *                      - 1: Send the packet after any node ID blink
 *                      - 2: Cancel broadcasting
 * @param len           The length of the payload in bytes (hex).
 *                      Range: 01 ... 70
 * @param data          The payload to be transmitted.
 *                      Range: 00 ... FF
 * @param timeout       The timeout value in milliseconds during which the node transmits after a node ID blink.
 *                      Range: 0 ... 65000
 *                      0 = Timeout disabled, broadcast will always occur.
 *                      > 0 = Time in milliseconds during which the node transmits after a node ID blink.
 * @param payload_id    Pointer to an array to store the payload ID if the option is 1.
 *
 * @return 0 on success, an error code otherwise.
 */
int BEE_b_bdat_ret(uint8_t option, uint8_t len, uint8_t *data, uint16_t timeout,
		uint8_t *payload_id) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_bdat(option, len, data, timeout));
	if (status)
		return status;
	if (option == 1) {
		for (int i = 0; i < 8; i++) {
			payload_id[i] = bee_response[4 + i];
		}
	}
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Fills the ranging data buffer with the specified data.
 *
 * This function fills the ranging data buffer with the specified data of length <Len>. The ranging data is
 * contained within every following ranging packet itself. Each receiving swarm will generate a *DNO. This
 * operation may cause a significant amount of traffic for each swarm device. If <Len> is 0, the ranging data
 * buffer will be deleted.
 *
 * @param len   The length of the ranging data payload in bytes (hex).
 *              Range: 00 ... 74
 *              0 = Delete data
 * @param data  The payload to be transmitted.
 *              Range: 00 ... FF
 * @param get   Flag to indicate if the function is used for getting or setting the ranging data buffer:
 *              - SET: Fill the ranging data buffer with the specified data.
 *              - GET: Get the current data in the ranging data buffer.
 *
 * @return The status of the ranging data buffer fill operation:
 *         Range: 0 ... 1
 *         0 = Successful
 *         1 = Not successful
 */
int BEE_b_frad(uint8_t len, uint8_t *data, FlagStatus get) {
	if (get) {
		bee_buffer[0] = 0x3;
		bee_buffer[1] = 0x57;
		bee_buffer[2] = 0x2a;
		bee_buffer[3] = len;
	} else {
		bee_buffer[0] = 0x3 + len;
		bee_buffer[1] = 0x55;
		bee_buffer[2] = 0x2a;
		bee_buffer[3] = len;
		for (int i = 0; i < len; i++) {
			bee_buffer[4] = data[i];
		}
	}
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Fills the ranging data buffer with the specified data and checks the status.
 *
 * This function fills the ranging data buffer with the specified data of length <Len>, checks the status of the
 * operation, and returns the result. The ranging data is contained within every following ranging packet itself.
 * Each receiving swarm will generate a *DNO. This operation may cause a significant amount of traffic for each
 * swarm device. If <Len> is 0, the ranging data buffer will be deleted. If the get flag is set, the function
 * retrieves the current data in the ranging data buffer.
 *
 * @param len   The length of the ranging data payload in bytes (hex).
 *              Range: 00 ... 74
 *              0 = Delete data
 * @param data  The payload to be transmitted or the buffer to store the retrieved data.
 *              Range: 00 ... FF
 * @param get   Flag to indicate if the function is used for getting or setting the ranging data buffer:
 *              - SET: Fill the ranging data buffer with the specified data.
 *              - GET: Get the current data in the ranging data buffer.
 *
 * @return 0 on success, an error code otherwise.
 */
int BEE_b_frad_ret(uint8_t len, uint8_t *data, FlagStatus get) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_frad(len, data, get));
	if (status)
		return status;
	if (get) {
		for (int i = 0; i < bee_response[4]; i++) {
			data[i] = bee_response[5 + i];
		}
	}
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Enables or disables the Node ID broadcast notification.
 *
 * This function enables or disables the Node ID broadcast notification. When enabled, the node will trigger
 * the host when a Node ID Broadcast has been received. When disabled, the node will not trigger the host
 * upon receiving a Node ID Broadcast.
 *
 * @param notify  The notification setting:
 *                - 0: Node will not trigger the host when a Node ID Broadcast has been received.
 *                - 1: Node will trigger the host when a Node ID Broadcast has been received.
 *
 * @return Tx status
 */
int BEE_b_eidn(uint8_t notify) {
	if (notify != 1 && notify != 0)
		return -3;
	bee_buffer[0] = 0x03;
	bee_buffer[1] = 0x57;
	bee_buffer[2] = 0x26;
	bee_buffer[3] = notify;
	return BEE_TX_B(bee_buffer);
}

/**
 * @brief Enables or disables the Node ID broadcast notification and checks the status.
 *
 * This function enables or disables the Node ID broadcast notification, checks the status of the operation,
 * and returns the notification setting that has been set.
 *
 * @param notify  The notification setting:
 *                - 0: Node will not trigger the host when a Node ID Broadcast has been received.
 *                - 1: Node will trigger the host when a Node ID Broadcast has been received.
 *
 * @return The notification setting that has been set:
 *         - 0: Node will not trigger the host.
 *         - 1: Node will trigger the host.
 */
int BEE_b_eidn_ret(uint8_t notify) {
	response_rdy = RESET;
	int status = status_helper(BEE_b_eidn(notify));
	if (status)
		return status;
	return bee_response[4];
}

/**
 * @brief De-escapes the response received from the BEE module.
 *
 * This function de-escapes the response received from the BEE module by removing escape sequences and
 * restoring the original data. It modifies the `bee_response` buffer in-place.
 *
 * @return None.
 */
void BEE_deescape_response() {
	if (!response_rdy)
		return;
	uint16_t size = response_pointer;
	uint16_t i = 1;
	while (i != size - 1) {
		if (bee_response[i] == 0x1B) { // escaped ESC
			if (bee_response[i + 1] == 0x53) { // escaped SYN
				bee_response[i] = BEE_SYNC;
			}
			memmove(&bee_payload[i + 1], &bee_payload[i + 2],
					(size - i - 2) * sizeof(uint8_t));
			size--;
		}
		i++;
	}
}

/**
 * @brief Helper function to handle the response status.
 *
 * This function is a helper function that handles the response status of the communication with the BEE module.
 * It checks for errors, verifies the response integrity, and returns the appropriate status code.
 *
 * @param status  The status of the communication with the BEE module.
 * @return The status code:
 *         - 0: Success.
 *         - Negative values: Error codes.
 */
int status_helper(int status) {
	if (status)
		return status;
	while (!response_rdy)
		;
	if (getCRC(bee_response, response_pointer + 2) != 0) {
		return -2;
	}
	if (bee_response[0] != BEE_SYNC) {
		return -5;
	}
	if (bee_response[2] == 0x60) { // error, invert code
		return -bee_response[3];
	}
	return 0;
}

/**
 * @brief  Debug feature for printing response
 * @retval None
 */
void printf_bee_response() {
	for (int i = 0; i < BEE_SIZE; i++) {
		printf("%x ", bee_response[i]);
	}
	printf("\r\n");
}

/**
 * @brief  Calculate ANSI CRC for given array
 * @param  Buffer Pointer to array.
 * @param  BufferSize  Amount of bytes in array.
 * @retval int16_t 16bit of calculated CRC
 */
uint16_t getCRC(uint8_t *Buffer, uint16_t BufferSize) {
	uint8_t z[BufferSize];
	uint16_t crc;
	for (uint8_t i = 0; i < BufferSize; i++)
		z[i] = *Buffer++;
	crc = Crc16(z, BufferSize - 2);
	return crc;
}

