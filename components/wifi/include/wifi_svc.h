/*
 * wifi_svc.h
 *
 *  Created on: Jul 21, 2020
 *      Author: liwei
 */

#ifndef COMPONENTS_WIFI_INCLUDE_WIFI_SVC_H_
#define COMPONENTS_WIFI_INCLUDE_WIFI_SVC_H_

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include <lwip/netdb.h>
#include "esp_smartconfig.h"

#include "ingwaycomm.h"

#define CONFIG_WIFI_SOCKET_FRAME_SIZE		256

#ifndef AF_INET
#define AF_INET				2
#endif

#ifndef AF_INET6
#define AF_INET6			10
#endif

#ifndef INADDR_ANY
#define INADDR_ANY          ((u32_t)0x00000000UL)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	WIFI_STA_START						= 1,
	WIFI_STA_CONNECTED 					= 2,
	WIFI_STA_DISCONNECTED 				= 3,
	WIFI_STA_STOP						= 4,
	WIFI_IP_GOT							= 5,
	WIFI_AP_CONNECTED					= 6,
	WIFI_AP_DISCONNECTED				= 7
}WIFIEventTypeTPDF;

typedef struct
{
	WIFIEventTypeTPDF tType;
	void *tTag;
	char *cData;
	uint16_t usLen;
}WIFIEventTPDF;

typedef enum
{
	WIFI_DHCP_MODE_STATIC = 1,
	WIFI_DHCP_MODE_DYNAMIC = 2,
}WifiDhcpModeTPDF;

typedef enum
{
	WIFI_DNS_MODE_STATIC = 1,
	WIFI_DNS_MODE_DYNAMIC = 2,
}WifiDnsModeTPDF;

typedef enum
{
	WIFI_SC_ENABLE = 1,
	WIFI_SC_DISENABLE = 2,
}WifiSmartConfigModeTPDF;

typedef struct
{
	uint16_t usMode;
	///<Sta config
	uint16_t usStaIPH;
	uint16_t usStaIPL;
	uint16_t usStaGateWayH;
	uint16_t usStaGateWayL;
	uint16_t usStaSubMaskH;
	uint16_t usStaSubMaskL;
	uint16_t usStaDNSH;
	uint16_t usStaDNSL;
	char cStaSSID[16];
	char cStaPWD[16];
	uint16_t usStaDHCPMode;
	uint16_t usStaDNSMode;
	uint16_t usReserver1[4];
	///<Ap config
	uint16_t usApIPH;
	uint16_t usApIPL;
	uint16_t usApSubMaskH;
	uint16_t usApSubMaskL;
	char cApSSID[16];
	char cApPWD[16];
	uint16_t usApIpPoolStartH;
	uint16_t usApIpPoolStartL;
	uint16_t usApIpPoolEndH;
	uint16_t usApIpPoolEndL;
	uint16_t usApDHCPMode;
	uint16_t usReserver2[4];
}WifiConfigTPDF;

typedef void (*WIFI_vEvent_CallBack)(WIFIEventTPDF *tEvent);

void WIFI_vInit_softap(WifiConfigTPDF *tCfg);

void WIFI_vInit_sta(WifiConfigTPDF *tCfg);

int WIFI_vData_Send(int iSocket, char *cData, uint16_t usLen);

void WIFI_vRegister_Callback(WIFI_vEvent_CallBack callback);

#ifdef __cplusplus
}
#endif

#endif /* COMPONENTS_WIFI_INCLUDE_WIFI_SVC_H_ */
