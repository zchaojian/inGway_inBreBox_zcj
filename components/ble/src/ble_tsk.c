/*
 * ble_tsk.c
 *
 *  Created on: Jul 24, 2020
 *      Author: liwei
 */
#include "ble_tsk.h"

static const char *TAG = "Ble_Task";

static ModbusRtuDataTPDF mcModbusRtuData;
static ModbusRtuDataTPDF mcModbusRtuDataRec;
static ModbusRtuDataTPDF mSModbusRtuData;

QueueHandle_t mBleQueueSend;
QueueHandle_t mBleQueueRec;
QueueHandle_t mSBleQueueSend;

const char * BLES_MBData = "BLES:ModbusData";

static DeviceTPDF *dBle_Device_Find_ByAddr(uint16_t usAddr)
{
	for(uint16_t i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		if(mDevice[i].tDeviceConfig.usCommPort == DEVICE_COMMPORT_TYPE_BLE && mDevice[i].tDeviceConfig.usConvertAddr == usAddr)
		{
			return(&mDevice[i]);
		}
	}
	return(NULL);
}

static void vBle_Tsk_C_Data_Rec_Callback(BleRemoteDeviceTPDF *tTag, char *cData, uint16_t usLen)
{
	MODB_vAnalysis((uint8_t *)cData, usLen, &mcModbusRtuDataRec);
	mcModbusRtuDataRec.tTag = tTag;
}

static void vBle_Tsk_S_Data_Rec_Callback(char *cData, uint16_t usLen)
{
	static uint8_t ucSdataSend[256];
	ModbusRtuDataTPDF tModbusRtuDataSend;
	MODB_vAnalysis_Pubilc((uint8_t *)cData, usLen, &mSModbusRtuData);
	esp_log_buffer_hex(BLES_MBData, cData,usLen);
	ESP_LOGW(BLES_MBData, "mSModbusRtuData.ucEffect:%d",mSModbusRtuData.ucEffect);
	if(mSModbusRtuData.ucEffect == MODBUS_RESULT_SUCCESS && mSModbusRtuData.ucSlaveAddr == 0)
	{
		ESP_LOGI(TAG, "MODBUS_RESULT_SUCCESS");
		CFG_dProcess_Protocol(&mSModbusRtuData, &tModbusRtuDataSend);
		if(tModbusRtuDataSend.ucEffect == MODBUS_RESULT_SUCCESS)
		{
			uint16_t k = MODB_dBuild(&tModbusRtuDataSend, ucSdataSend);
			esp_log_buffer_hex(BLES_MBData, ucSdataSend,k);
			BLE_S_dData_Send((char *)ucSdataSend, k);
		}
	}
}

static void vBle_Tsk_Init(void)
{
	mSModbusRtuData.ucSlaveAddr = 0;
	mSModbusRtuData.tModbusMstSlvMode = MODBUS_MODE_SLAVE;
	mcModbusRtuData.tModbusMstSlvMode = MODBUS_MODE_MASTER;
	mcModbusRtuDataRec.tModbusMstSlvMode = MODBUS_MODE_MASTER;
	mcModbusRtuDataRec.ucSlaveAddr = 0;
	BLE_C_vRegister_Data_Rec_Callback(vBle_Tsk_C_Data_Rec_Callback);
	BLE_S_vRegister_Data_Rec_Callback(vBle_Tsk_S_Data_Rec_Callback);
	BLE_vInit();
}

void vBle_JointDown_PassThrough_Modbus(void)
{
	MyQueueDataTPDF tMyQueueData;
	BleRemoteDeviceTPDF *tBleDevice;
	DeviceTPDF *tDevice;
	char ucDataSend[256];
	uint16_t usDelay;
	uint16_t i, j;
	if(xQueueReceive(mBleQueueSend, (void * )&tMyQueueData, (portTickType)0))
	{
		if(tMyQueueData.tMyQueueCommand.usSlaveAddr == 0)
		{
			///<Broadcast command
		}
		tDevice = dBle_Device_Find_ByAddr(tMyQueueData.tMyQueueCommand.usSlaveAddr);
		if(tDevice == NULL)
		{
			return;
		}
		tBleDevice = NULL;
		for(j = 0; j < 100; j++)
		{
			tBleDevice = BLE_C_Connect_Taget(tDevice->tDeviceConfig.tAddr.ucBleMac);
			if(tBleDevice != NULL)
			{
				break;
			}
			vTaskDelay(1);
		}
		if(tBleDevice == NULL)
		{
			return;
		}
		for(j = 0; j < 200; j++)
		{
			if(tBleDevice->ucConnectState == BLE_REMOTE_DEVICE_STATE_CONNECTED)
			{
				break;
			}
			vTaskDelay(1);
		}
		if(tBleDevice->ucConnectState != BLE_REMOTE_DEVICE_STATE_CONNECTED)
		{
			return;
		}
		mcModbusRtuData.tFunction = tMyQueueData.tMyQueueCommand.tMyQueueCommandType;
		mcModbusRtuData.usRegAddr = tMyQueueData.tMyQueueCommand.usRegAddr;
		mcModbusRtuData.usRegCount = tMyQueueData.tMyQueueCommand.usRegLen;
		mcModbusRtuData.usDataLen = tMyQueueData.tMyQueueCommand.usDatalen;
		memcpy(mcModbusRtuData.ucData,tMyQueueData.tMyQueueCommand.cData, tMyQueueData.tMyQueueCommand.usDatalen);
		i = MODB_dBuild(&mcModbusRtuData, (uint8_t *)ucDataSend);
		mcModbusRtuDataRec.ucEffect = MODBUS_RESULT_FAIL;
		BLE_C_dData_Send(tBleDevice, ucDataSend, i);
		printf("BLE:passtough\r\n");
		usDelay = 50;
		while(usDelay)
		{
			vTaskDelay(1);
			usDelay--;
			if(mcModbusRtuDataRec.ucEffect == MODBUS_RESULT_SUCCESS && mcModbusRtuDataRec.tFunction == mcModbusRtuData.tFunction)
			{
				break;
			}
		}
		if(!usDelay)
		{
			return;
		}
		if(mcModbusRtuDataRec.ucEffect == MODBUS_RESULT_FAIL)
		{
			printf("Ble:passtough:ana fail\n");
			return;
		}
		mcModbusRtuDataRec.ucEffect = MODBUS_RESULT_FAIL;
		tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mcModbusRtuDataRec.tFunction;
		tMyQueueData.tMyQueueCommand.usSlaveAddr = mcModbusRtuDataRec.ucSlaveAddr;
		tMyQueueData.tMyQueueCommand.usRegAddr = mcModbusRtuDataRec.usRegAddr;
		tMyQueueData.tMyQueueCommand.usRegLen = mcModbusRtuDataRec.usRegCount;
		tMyQueueData.tMyQueueCommand.usDatalen = mcModbusRtuDataRec.usDataLen;
		memcpy(tMyQueueData.tMyQueueCommand.cData,mcModbusRtuDataRec.ucData, tMyQueueData.tMyQueueCommand.usDatalen);
		xQueueSend(mBleQueueRec, (void * )&tMyQueueData, (TickType_t)0);
	}
}

static void vBle_C_Process(void)
{
	BleRemoteDeviceTPDF *tBleDevice;
	static char cData[256];
	uint8_t i, j, k, l;
	vBle_JointDown_PassThrough_Modbus();
	if(mPartitionTable.usGatewayRunMode == GATEWAY_MODE_PASSTHROUGH)
	{
		return;
	}
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		tBleDevice = NULL;
		if(mDevice[i].tDeviceConfig.usCommPort != DEVICE_COMMPORT_TYPE_BLE)
		{
			continue;
		}
		for(j = 0; j < 100; j++)
		{
			tBleDevice = BLE_C_Connect_Taget(mDevice[i].tDeviceConfig.tAddr.ucBleMac);
			if(tBleDevice != NULL)
			{
				break;
			}
			vTaskDelay(1);
		}
		if(tBleDevice == NULL)
		{
			continue;
		}
		for(j = 0; j < 200; j++)
		{
			if(tBleDevice->ucConnectState == BLE_REMOTE_DEVICE_STATE_CONNECTED)
			{
				break;
			}
			vTaskDelay(1);
		}
		if(tBleDevice->ucConnectState != BLE_REMOTE_DEVICE_STATE_CONNECTED)
		{
			continue;
		}
		for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
		{
			vTaskDelay(2);
			if(!mDevice[i].tDevicePartitionTable[j].usLen)
			{
				continue;
			}
			mcModbusRtuData.tFunction = mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableRegType;
			mcModbusRtuData.ucSlaveAddr = 0;
			mcModbusRtuData.usRegAddr = mDevice[i].tDevicePartitionTable[j].usStartAddr;
			mcModbusRtuData.usRegCount = mDevice[i].tDevicePartitionTable[j].usLen;
			k = MODB_dBuild(&mcModbusRtuData, (uint8_t *)cData);
			mcModbusRtuDataRec.ucEffect = MODBUS_RESULT_FAIL;
			BLE_C_dData_Send(tBleDevice, cData, k);
			for(k = 0; k < 20; k++)
			{
				vTaskDelay(1);
				if(mcModbusRtuDataRec.ucEffect != MODBUS_RESULT_SUCCESS || mcModbusRtuDataRec.tFunction != mcModbusRtuData.tFunction)
				{
					continue;
				}
				for(l = 0; l < mDevice[i].tDevicePartitionTable[j].usLen; l++)
				{
					mDevice[i].tDevicePartitionTable[j].usMemory[l] = (mcModbusRtuDataRec.ucData[l * 2] << 8) + mcModbusRtuDataRec.ucData[l * 2 + 1];
				}
				break;
			}
		}
	}
	vTaskDelay(2);
}

static void vBle_S_Process(void)
{
	MyQueueDataTPDF tMyQueueData;
	static char cData[256];
	if(xQueueReceive(mSBleQueueSend, (void * )&tMyQueueData, (portTickType)0))
	{
		mSModbusRtuData.tFunction = tMyQueueData.tMyQueueCommand.tMyQueueCommandType;
		mSModbusRtuData.ucSlaveAddr = tMyQueueData.tMyQueueCommand.usSlaveAddr;
		mSModbusRtuData.usRegAddr = tMyQueueData.tMyQueueCommand.usRegAddr;
		mSModbusRtuData.usRegCount = tMyQueueData.tMyQueueCommand.usRegLen;
		mSModbusRtuData.usDataLen = tMyQueueData.tMyQueueCommand.usDatalen;
		memcpy(mSModbusRtuData.ucData,tMyQueueData.tMyQueueCommand.cData, tMyQueueData.tMyQueueCommand.usDatalen);
		uint16_t i = MODB_dBuild(&mSModbusRtuData, (uint8_t *)cData);
		BLE_S_dData_Send(cData, i);
		ESP_LOGW(TAG, "BLE_S_SEND ");
	}
	if(mSModbusRtuData.ucEffect == MODBUS_RESULT_SUCCESS && mSModbusRtuData.ucSlaveAddr)
	{
		mSModbusRtuData.ucEffect = MODBUS_RESULT_FAIL;
		tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mSModbusRtuData.tFunction;
		tMyQueueData.tMyQueueCommand.usSlaveAddr = mSModbusRtuData.ucSlaveAddr;
		tMyQueueData.tMyQueueCommand.usRegAddr = mSModbusRtuData.usRegAddr;
		tMyQueueData.tMyQueueCommand.usRegLen = mSModbusRtuData.usRegCount;
		tMyQueueData.tMyQueueCommand.usDatalen = mSModbusRtuData.usDataLen;
		memcpy(tMyQueueData.tMyQueueCommand.cData, mSModbusRtuData.ucData, tMyQueueData.tMyQueueCommand.usDatalen);
		xQueueSend(mBleQueueRec, (void * )&tMyQueueData, (TickType_t)0);
		ESP_LOGW(TAG, "BLE_S_PROCESS ");
	}
}

void Ble_vTsk_Start(void *pvParameters)
{
	vBle_Tsk_Init();
	while(1)
	{
		vBle_C_Process();
		vBle_S_Process();
		vTaskDelay(2);
	}
}
