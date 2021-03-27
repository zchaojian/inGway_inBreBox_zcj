/*
 * eth_tsk.h
 *
 *  Created on: Jul 26, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_ETH_TSK_H_
#define EXAMPLES_WG_WG_MAIN_ETH_TSK_H_

#define VALUE_TCPSOCKET_MODBUS_PORT		502

#include "config_app.h"
#include "eth_svc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "myqueue.h"
#include "modbus_svc.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sntp_svc.h"

#include "native_ota.h"

extern QueueHandle_t mEthQueueSend;
extern QueueHandle_t mEthQueueRec;

extern void Eth_vTsk_Start(void *pvParameters);
extern uint8_t Eth_dTsk_GetModbusMode(void);
/* Use function return Network link status */
extern uint8_t Eth_Connected_Status(void);

extern void Eth_vSocket_Close(void);
extern void Eth_Socket_vInit(/*uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily*/);
extern void Eth_Socket_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

extern void Eth_Mqtt_vInit(void);
extern void Eth_Mqtt_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

extern void Eth_SocketClient_vInit(uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily);
extern void Eth_SocketClient_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

extern void vEth_Event_CallBack(ETHEventTPDF *tEvent);

extern void vEth_Dhcp_GetIP_Save_Flash(esp_netif_ip_info_t *ip_info);

/*//Use function return Network link status
uint8_t Eth_Connected_Status(void);*/

#endif /* EXAMPLES_WG_WG_MAIN_ETH_TSK_H_ */
