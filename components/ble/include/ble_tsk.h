/*
 * ble_tsk.h
 *
 *  Created on: Jul 24, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_BLE_TSK_H_
#define EXAMPLES_WG_WG_MAIN_BLE_TSK_H_

#define CONFIG_BLE_TSK_MAXSURPPORT		6

#include "ble_svc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "myqueue.h"
#include "modbus_svc.h"
#include "config_app.h"
#include "device_app.h"
#include "nvs.h"
#include "nvs_flash.h"

extern QueueHandle_t mBleQueueSend;
extern QueueHandle_t mSBleQueueSend;
extern QueueHandle_t mBleQueueRec;

extern void Ble_vTsk_Start(void *pvParameters);
#endif /* EXAMPLES_WG_WG_MAIN_BLE_TSK_H_ */
