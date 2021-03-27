/*
 * wifi_tsk.h
 *
 *  Created on: Nov 1, 2020
 *      Author: zchaojian
 */

#ifndef COMPONENTS_WIFI_INCLUDE_WIFI_TSK_H_
#define COMPONENTS_WIFI_INCLUDE_WIFI_TSK_H_

#define VALUE_TCPSOCKET_MODBUS_PORT		502

#include "config_app.h"
#include "wifi_svc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "myqueue.h"
#include "modbus_svc.h"
#include "mqtt_client.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "sntp_svc.h"

#include "udp_brdcast.h"
#include "native_ota.h"

extern QueueHandle_t mWifiQueueSend;
extern QueueHandle_t mWifiQueueRec;

extern void Wifi_vTsk_Start(void *pvParameters);

extern void vWifi_Base_Init();

extern void vSmartConfig_vInit();

extern void Wifi_vSocket_Close(void);
extern void Wifi_Socket_vInit(/*uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily*/);
extern void Wifi_Socket_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

extern void Wifi_Mqtt_vInit(void);
extern void Wifi_Mqtt_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

extern void Wifi_SocketClient_vInit(uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily);
extern void Wifi_SocketClient_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData);

#endif /* COMPONENTS_WIFI_INCLUDE_WIFI_TSK_H_ */
