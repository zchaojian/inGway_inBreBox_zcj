/* 1.Create a system blink task to display the normal work of system.

This example code is in the Public Domain (or CC0 licensed, at your option.)

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _BLINK_H
#define _BLINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	LED_ESP_TOUCH_CONFIG,
	LED_ETH_CONNECT,
	LED_WIFI_CONNECT,
	GW_SMART_CONFIG_STATE,
	GW_IP_NOT_GET_STATE,
	GW_TCP_SERVER_OK_STATE,
	GW_OTA_STATE,
	GW_NORMAL_STATE,
	LED_ETH_NOT_CONNECT,
	LED_WIFI_NOT_CONNECT,
	GW_TCP_CLIENT_IN_STATE,
	GW_TCP_CLIENT_OUT_STATE,
	GW_TCP_SERVER_IN_STATE,
	GW_TCP_SERVER_OUT_STATE,
	GW_MQTT_CLIENT_IN_STATE,
	GW_MQTT_CLIENT_OUT_STATE,
}eLedDisplayState;

/* Create a system blink task to display the normal work of system. */
void sytem_blink_task();

/* Use function return wifi Network link status */
extern uint8_t Wifi_Connected_Status(void);

#ifdef __cplusplus
}
#endif

#endif
