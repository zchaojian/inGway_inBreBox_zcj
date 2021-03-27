/*
 * uart_svc.c
 *
 *  Created on: Jul 17, 2020
 *      Author: liwei
 */
#include "uart_svc.h"

static QueueHandle_t mUart_Queue;

/* read data frame from uart hardware and receice queue event */
uint8_t URT1_dRx(char *ucData, uint16_t *usLen)
{
	uart_event_t event;
	if(xQueueReceive(mUart_Queue, (void * )&event, (portTickType)0))
	{
		switch(event.type)
		{
			case UART_DATA:
				uart_read_bytes(UART_NUM_1, (uint8_t*)ucData, event.size, 0);
				*usLen = event.size;
				return(1);
			case UART_BUFFER_FULL:
				uart_flush_input(UART_NUM_1);
				xQueueReset(mUart_Queue);
				break;
			default:
				break;
		}
	}
	return(0);
}

/* send modbus data frame to uart hardware */
int URT1_dData_Send(char *cData, uint16_t usLen)
{
	int i = uart_write_bytes(UART_NUM_1, cData, usLen);
	uart_flush_input(UART_NUM_1);
	return(i);
}

void URT1_vInit(UartConfigTPDF *tCfg)
{
    const uart_config_t uart_config = {
        .baud_rate = (tCfg->usBaudrateH << 16) + tCfg->usBaudrateL,
        .data_bits = tCfg->usWordSize,
        .parity = tCfg->usParity,
        .stop_bits = tCfg->usStopBits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_1, CONFIG_UART_RX_BUFFSIZE * 2, CONFIG_UART_RX_BUFFSIZE * 2, 20, &mUart_Queue, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, CONFIG_UART_TX1_GPIO, CONFIG_UART_RX1_GPIO, CONFIG_UART_CT1_GPIO, UART_PIN_NO_CHANGE);
    uart_set_mode(UART_NUM_1, UART_MODE_RS485_HALF_DUPLEX);
}


