/*
 * iot_svc.h
 *
 *  Created on: Jul 21, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_IOT_SVC_H_
#define EXAMPLES_WG_WG_MAIN_IOT_SVC_H_

#include "driver/uart.h"
#include "driver/gpio.h"
#include "ingwaycomm.h"

typedef enum
{
	IOT_TYPE_NB = 1,
	IOT_TYPE_4G = 2,
	IOT_TYPE_LORA = 3,
	IOT_TYPE_HPLC = 4,
}IotTypeTPDF;

typedef struct
{
	uint16_t usIotType;
	uint16_t usReserver[4];
}IotConfigTPDF;

#endif /* EXAMPLES_WG_WG_MAIN_IOT_SVC_H_ */
