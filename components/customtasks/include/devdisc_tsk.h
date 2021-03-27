/*
 * devdisc_tsk.h
 *
 *  Created on: Sep 29, 2020
 *      Author: liwei
 */

#ifndef COMPONENTS_CUSTOMTASKS_INCLUDE_DEVDISC_TSK_H_
#define COMPONENTS_CUSTOMTASKS_INCLUDE_DEVDISC_TSK_H_

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "eth_mqtt_tsk.h"
#include "uart_svc.h"
#include "device_app.h"
#include "config_app.h"
#include "eth_tsk.h"
#include "comm1_tsk.h"
#include "comm2_tsk.h"
#include "ble_tsk.h"

#define CONFIG_BLEDEVICE_NAME			"CHINT_MCCB"
#define CONFIG_BLEDEVICE_NAMELEN		10
#define CONFIG_BLEDEVICE_MAXSURPPORT	32
#define CONFIG_BLEDEVICE_ADDR_OFFSET	100

extern QueueHandle_t mDevDiscQueueSend;
extern QueueHandle_t mDevDiscQueueRec;

extern void DEVDISC_Task(void *pvParameters);

#endif /* COMPONENTS_CUSTOMTASKS_INCLUDE_DEVDISC_TSK_H_ */
