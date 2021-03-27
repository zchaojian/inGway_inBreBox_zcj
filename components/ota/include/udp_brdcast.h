/*
 * udp_broadcast.h
 *
 * 1.Create a udp broadcast task to transmit the local OTA tcp task of IP, PORT,
 * and the version of download binary file.
 *
 *  Created on: Mar 10, 2020
 *      Author: zchaojian
 */

#ifndef _UDP_BRDCAST_H_
#define _UDP_BRDCAST_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "eth_tsk.h"

typedef struct
{
	uint8_t ucFirstMidVer;
	uint8_t ucLastVer;
}VersionTPDF;

typedef struct
{
	uint8_t ucDeviceType[4];//FactoryID, class, series, sub-series
	uint8_t ucRunStatus;
	VersionTPDF tSoftwareVersion;
	VersionTPDF tHardwareVersion;
	uint8_t ucProductSN[24];
	uint8_t ucProductDescription[24];
}InGwayInfoTPDF;

typedef struct
{
	uint16_t ucTransactionID;
	uint16_t ucProtocolID;
	uint16_t ucLen;
	uint8_t ucUnitID;
	uint8_t ucFunctionCode;
	uint8_t ucNextLen;
	InGwayInfoTPDF tInGwayInfo;
}InGwayBroadcastFrameTPDF;

/* Create a udp broadcast task. It can transmit the local OTA tcp task of IP, PORT and version */
void Udp_Brdcast_Start(void);

/* return udp broadcast fd */
int Udp_Brdcast_fd();

/* calculate version number from string to unsigned char number */
void Version_Cal(char *cVersionIn, char *cVersionOut);

/* Calculate version number. The function mean to converter string version to uint8_ hex version */
void vUdp_VersionAnalysis(char *cVersionIn, uint8_t *ucVersionOut);

/* return udp broadcast server status */
uint8_t Udp_Brdcast_Status();

/* Use function return eth Network link status */
extern uint8_t Eth_Connected_Status(void);

/* Use function return tcp socket server establish status */
extern uint8_t Eth_SocketServer_Status(void);

/* Use function return wifi Network link status */
extern uint8_t Wifi_Connected_Status(void);

/* Use function return wifi Network link status */
extern uint8_t Wifi_SocketServer_Status(void);

#ifdef __cplusplus
}
#endif

#endif /* _UDP_BCAST_H_ */
