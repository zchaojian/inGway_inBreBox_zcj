/*
 * uart_svc.h
 *
 *  Created on: Jul 17, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_UART_SVC_H_
#define EXAMPLES_WG_WG_MAIN_UART_SVC_H_

#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "ingwaycomm.h"

#define CONFIG_UART_TX1_GPIO				(GPIO_NUM_13)
#define CONFIG_UART_RX1_GPIO				(GPIO_NUM_5)
#define CONFIG_UART_CT1_GPIO				(GPIO_NUM_2)

#define CONFIG_UART_TX2_GPIO				(GPIO_NUM_4)
#define CONFIG_UART_RX2_GPIO				(GPIO_NUM_16)
#define CONFIG_UART_CT2_GPIO				(GPIO_NUM_15)
#define CONFIG_UART_RX_BUFFSIZE				256

typedef enum
{
	COMM_PORT_COM1 = 1,
	COMM_PORT_COM2 = 2,
}CommPortTPDF;

typedef struct
{
	uint16_t usCommPort;
	uint16_t usBaudrateH;
	uint16_t usBaudrateL;
	uint16_t usWordSize;
	uint16_t usParity;
	uint16_t usStopBits;
}UartConfigTPDF;

extern void URT1_vInit(UartConfigTPDF *tCfg);
extern uint8_t URT1_dRx(char *ucData, uint16_t *usLen);
extern int URT1_dData_Send(char *cData, uint16_t usLen);

extern void URT2_vInit(UartConfigTPDF *tCfg);
extern uint8_t URT2_dRx(char *ucData, uint16_t *usLen);
extern int URT2_dData_Send(char *cData, uint16_t usLen);

#endif /* EXAMPLES_WG_WG_MAIN_UART_SVC_H_ */
