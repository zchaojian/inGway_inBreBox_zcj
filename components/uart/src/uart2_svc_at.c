/*
 * uart2_svc_at.c
 *
 *  Created on: Dec 10, 2020
 *      Author: zchaojian
 */
#include "uart_svc.h"

static QueueHandle_t mUart_Queue;

uint8_t URT2_dRx_AT(char *ucData, uint16_t *usLen)
{
	uart_event_t event;
	if(xQueueReceive(mUart_Queue, (void * )&event, (portTickType)0))
	{
		switch(event.type)
		{
			case UART_DATA:
				uart_read_bytes(UART_NUM_2, (uint8_t*)ucData, event.size, 0);
				*usLen = event.size;
				return(1);
			 case UART_BUFFER_FULL:
			    uart_flush_input(UART_NUM_2);
			    xQueueReset(mUart_Queue);
			    break;
			default:
				break;
		}
	}
	return(0);
}

int URT2_dData_Send_AT(char *cData, uint16_t usLen)
{
	int i = uart_write_bytes(UART_NUM_2, cData, usLen);
	uart_flush_input(UART_NUM_2);
	return(i);
}

void URT2_vInit_AT(UartConfigTPDF *tCfg)
{
    const uart_config_t uart_config = {
    	.baud_rate = 115200,
		.data_bits = UART_DATA_8_BITS,
		.parity = UART_PARITY_DISABLE,
		.stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_2, CONFIG_UART_RX_BUFFSIZE * 2, CONFIG_UART_RX_BUFFSIZE * 2, 20, &mUart_Queue, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, CONFIG_UART_TX2_GPIO, CONFIG_UART_RX2_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}







