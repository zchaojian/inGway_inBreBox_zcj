/* 1.Create a native OTA task.

This example code is in the Public Domain (or CC0 licensed, at your option.)

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef _NATIVE_OTA_H
#define _NATIVE_OTA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "lwip/sockets.h"

#include "native_ota.h"
#include "download_tcp_server.h"
//#include "eth_tsk.h"

#define HASH_LEN 32 /* SHA-256 digest length */

typedef enum {
	OTA_UPDATE_START = 0x01,
	OTA_UPDATE_FINISH = 0x02,
	OTA_UPDATE_TRANSACTIONID_HI = 0xFF,
	OTA_UPDATE_TRANSACTIONID_LO = 0xFF,
	OTA_UPDATE_RESERVE,
} ota_update_state_t;

void print_sha256 (const uint8_t *image_hash, const char *label);

/* Create a native OTA task. It can realize download BIN file through ETH or WIFI */
void Native_Ota_Start(void);

//Use function return Network link status
extern uint8_t Eth_Connected_Status(void);

/* Use function return wifi Network link status */
extern uint8_t Wifi_Connected_Status(void);

/* return udp broadcast server status */
extern uint8_t Udp_Brdcast_Status();

bool diagnostic(void);

#ifdef __cplusplus
}
#endif

#endif
