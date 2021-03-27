/*
 * ble_svc.h
 *
 *  Created on: Jul 21, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_BLE_SVC_H_
#define EXAMPLES_WG_WG_MAIN_BLE_SVC_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ingwaycomm.h"
#include "config_app.h"

#define INVALID_HANDLE   						0
#define PROFILE_A_APP_ID						3
#define REMOTE_SERVICE_FILTER_UART_UUID			{0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x01,0x00,0x40,0x6e}
#define REMOTE_SERVICE_FILTER_UART_TX_UUID		{0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x02,0x00,0x40,0x6e}
#define REMOTE_SERVICE_FILTER_UART_RX_UUID		{0x9e,0xca,0xdc,0x24,0x0e,0xe5,0xa9,0xe0,0x93,0xf3,0xa3,0xb5,0x03,0x00,0x40,0x6e}

#define GATTS_TAG 								"CHINTWAY"//"CHINT_"//_21100001"//"CHINTWAY"
#define GATTS_NUM_HANDLE_TEST_A    				4
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 			0x40
#define GATTS_PREPARE_BUF_MAX_SIZE 				1024
#define GATTS_ADV_CONFIG_FLAG      				(1 << 0)
#define GATTS_SCAN_RSP_CONFIG_FLAG				(1 << 1)
#define GATTS_SERVICE_UUID_TEST_A   			0x00FF
#define GATTS_CHAR_UUID_TEST_A       			0xFF01
#define GATTS_DESCR_UUID_TEST_A     			0x3333
#define GATTS_NUM_HANDLE_TEST_A     			4

typedef enum
{
	BLE_REMOTE_DEVICE_STATE_NONE = 0,
	BLE_REMOTE_DEVICE_STATE_CONNECTING = 1,
	BLE_REMOTE_DEVICE_STATE_GETSERVER = 2,
	BLE_REMOTE_DEVICE_STATE_CONNECTED = 3,
}BleRemoteDeviceStateTPDF;

typedef struct
{
	BleRemoteDeviceStateTPDF ucConnectState;
	uint8_t ucMac[6];
    uint16_t usGattcIf;
    uint16_t usConnId;
    uint16_t usServiceStartHandle;
    uint16_t usServiceEndHandle;
    uint16_t usCharHandle;
    uint16_t usCharSendHandle;
    esp_gattc_char_elem_t *mCharElemResult;
    esp_gattc_descr_elem_t *mDescrElemResult;
    void *vTag;
}BleRemoteDeviceTPDF;

typedef void (*BLECDataRecCallBackTPDF)(BleRemoteDeviceTPDF *tTag, char *cData, uint16_t usLen);
typedef void (*BLECScanResultTPDF)(uint8_t *ucMac, uint8_t *ucData);

extern void BLE_vInit(void);

extern uint8_t BLE_C_dData_Send(BleRemoteDeviceTPDF *tTag, char *cData, uint16_t usLen);

extern uint8_t BLE_S_dData_Send(char *cData, uint16_t usLen);

extern void BLE_C_vStartScan(void);

extern void BLE_C_vRegister_ScanResult_Callback(BLECScanResultTPDF tCallback);

extern void BLE_C_vRegister_Data_Rec_Callback(BLECDataRecCallBackTPDF tCallback);

extern void BLE_S_vRegister_Data_Rec_Callback(DataRecCallBackTPDF tCallback);

extern BleRemoteDeviceTPDF *BLE_C_Connect_Taget(uint8_t *ucMac);

#endif /* EXAMPLES_WG_WG_MAIN_BLE_SVC_H_ */
