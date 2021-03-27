/*
 * comm2_tsk.h
 *
 *  Created on: Jul 31, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_COMM2_TSK_H_
#define EXAMPLES_WG_WG_MAIN_COMM2_TSK_H_

#include "config_app.h"
#include "uart_svc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "myqueue.h"
#include "modbus_svc.h"
#include "config_app.h"
#include "device_app.h"

extern void COM2_vTsk_Start(void *pvParameters);
extern QueueHandle_t mComm2QueueSend;
extern QueueHandle_t mComm2QueueRec;

#endif /* EXAMPLES_WG_WG_MAIN_COMM2_TSK_H_ */
