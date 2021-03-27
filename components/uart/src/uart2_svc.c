/*
 * uart2_svc.c
 *
 *  Created on: Jul 26, 2020
 *      Author: liwei
 */
#include "uart_svc.h"

static QueueHandle_t mUart_Queue;

uint8_t URT2_dRx(char *ucData, uint16_t *usLen)
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

int URT2_dData_Send(char *cData, uint16_t usLen)
{
	int i = uart_write_bytes(UART_NUM_2, cData, usLen);
	uart_flush_input(UART_NUM_2);
	return(i);
}

void URT2_vInit(UartConfigTPDF *tCfg)
{
    const uart_config_t uart_config = {
    	.baud_rate = (tCfg->usBaudrateH << 16) + tCfg->usBaudrateL,
		.data_bits = tCfg->usWordSize,
		.parity = tCfg->usParity,
		.stop_bits = tCfg->usStopBits,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    uart_driver_install(UART_NUM_2, CONFIG_UART_RX_BUFFSIZE * 2, CONFIG_UART_RX_BUFFSIZE * 2, 20, &mUart_Queue, 0);
    uart_param_config(UART_NUM_2, &uart_config);
    uart_set_pin(UART_NUM_2, CONFIG_UART_TX2_GPIO, CONFIG_UART_RX2_GPIO, CONFIG_UART_CT2_GPIO, UART_PIN_NO_CHANGE);
    uart_set_mode(UART_NUM_2, UART_MODE_RS485_HALF_DUPLEX);
}



