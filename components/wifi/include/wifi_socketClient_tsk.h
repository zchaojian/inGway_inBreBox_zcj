/*
 * wifi_socketClient_tsk.h
 *
 *  Created on: Dec 10, 2020
 *      Author: zchaojian
 */

#ifndef COMPONENTS_WIFI_INCLUDE_WIFI_SOCKETCLIENT_TSK_H_
#define COMPONENTS_WIFI_INCLUDE_WIFI_SOCKETCLIENT_TSK_H_

#include "wifi_tsk.h"
#include "udp_brdcast.h"

extern void Wifi_vSocketClient_Close(void);
extern void Wifi_SocketClient_vInit(uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily);
extern void Wifi_SocketClient_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);



#endif /* COMPONENTS_WIFI_INCLUDE_WIFI_SOCKETCLIENT_TSK_H_ */
