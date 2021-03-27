/*
 * eth_socketClient_tsk.h
 *
 *  Created on: Dec 10, 2020
 *      Author: zchaojian
 */

#ifndef COMPONENTS_ETH_INCLUDE_ETH_SOCKETCLIENT_TSK_H_
#define COMPONENTS_ETH_INCLUDE_ETH_SOCKETCLIENT_TSK_H_

#include "eth_tsk.h"
#include "udp_brdcast.h"
#include "time.h"

extern void Eth_vSocketClient_Close(void);
extern void Eth_SocketClient_vInit(uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily);
extern void Eth_SocketClient_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

#define ELECON_FCONVERTER_DEVICE_CODE "7P6uKOOkus5NI226"

#endif /* COMPONENTS_ETH_INCLUDE_ETH_SOCKETCLIENT_TSK_H_ */
