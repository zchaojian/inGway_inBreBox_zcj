/*
 * ethernet_svc.h
 *
 *  Created on: Jul 17, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_ETH_SVC_H_
#define EXAMPLES_WG_WG_MAIN_ETH_SVC_H_

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_event.h"
#include "esp_log.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "ingwaycomm.h"

#define CONFIG_ETHERNET_MDC_GPIO				23
#define CONFIG_ETHERNET_MDIO_GPIO				18
#define CONFIG_ETHERNET_PHY_RST_GPIO			-1
#define CONFIG_ETHERNET_PHY_ADDR				1

#define CONFIG_ETHERNET_SOCKET_FRAME_SIZE		256

#ifndef AF_INET
#define AF_INET				2
#endif

#ifndef AF_INET6
#define AF_INET6			10
#endif

#ifndef INADDR_ANY
#define INADDR_ANY          ((u32_t)0x00000000UL)
#endif

typedef enum
{
	ETH_PHY_START						= 1,
	ETH_PHY_CONNECTED 					= 2,
	ETH_PHY_DISCONNECTED 				= 3,
	ETH_PHY_STOP						= 4,
	ETH_IP_GOT							= 5,
}ETHEventTypeTPDF;

typedef struct
{
	ETHEventTypeTPDF tType;
	void *tTag;
	char *cData;
	uint16_t usLen;
}ETHEventTPDF;

typedef enum
{
	ETHERNET_IP_TYPE_STATIC = 1,
	ETHERNET_IP_TYPE_DHCP = 2,
}EthernetIPTypeTPDF;

typedef enum
{
	ETHERNET_DNS_TYPE_STATIC = 1,
	ETHERNET_DNS_TYPE_DYNAMIC = 2,
}EthernetDnsTypeTPDF;

typedef enum
{
	ETHERNET_MODE_CLIENT = 1,
	ETHERNET_MODE_SERVER = 2,
}EthernetCliSerModeTPDF;

typedef struct
{
	uint16_t usIPH;
	uint16_t usIPL;
	uint16_t usGateWayH;
	uint16_t usGateWayL;
	uint16_t usSubMaskH;
	uint16_t usSubMaskL;
	uint16_t usDNSH;
	uint16_t usDNSL;
	uint16_t EthernetIPType;
	uint16_t EthernetCliSerMode;
	uint16_t EthernetDnsType;
	uint16_t EthernetPort;
	uint16_t EthernetTimeout;
	uint16_t usReserver[4];
}EthernetConfigTPDF;

typedef void (*ETH_vEvent_CallBack)(ETHEventTPDF *tEvent);

extern void ETH_vInit(EthernetConfigTPDF *tCfg);
extern void ETH_vRegister_Callback(ETH_vEvent_CallBack callback);
extern int ETH_vData_Send(int iSocket, char *cData, uint16_t usLen);

#endif /* EXAMPLES_WG_WG_MAIN_ETH_SVC_H_ */
