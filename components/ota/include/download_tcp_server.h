/*
 * download_tcp_server.h
 *
 *  Created on: Mar 5, 2020
 *      Author: zchaojian
 */

#ifndef _DOWNLOAD_TCP_SERVER_H_
#define _DOWNLOAD_TCP_SERVER_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
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
#include "modbus_svc.h"		//calculate CRC
#include "native_ota.h"

/* A function will create create a tcp server and listen the tcp client connect.
 * return server fd
 * */
int download_tcp_server();

/* accept client link and return client fd */
int tcp_client_accept(int listen_sock);

/* the function will read a tcp socket transmit data.
 *
 *
 *
 */
int tcp_server_read(int client_sock, unsigned char *buffer, int buf_len);

bool tcp_server_write(int client_sock, unsigned char *header_buffer, int header_buf_len, unsigned char response_state);

//void tcp_cleanup();

#ifdef __cplusplus
}
#endif



#endif /* COMPONENTS_NATIVE_OTA_INCLUDE_DOWNLOAD_TCP_SERVER_H_ */
