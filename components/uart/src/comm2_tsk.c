/*
 * comm2_tsk.c
 *
 *  Created on: Jul 31, 2020
 *      Author: liwei
 */
#include "comm2_tsk.h"

static ModbusRtuDataTPDF *mModbusRtuDataSend;
static ModbusRtuDataTPDF *mModbusRtuDataRec;
static char *mDataSend;
static char *mDataRec;

QueueHandle_t mComm2QueueSend;
QueueHandle_t mComm2QueueRec;

const char * COM2_TAG = "COM2";

static void vCOM2_Init(void)
{
	free(mModbusRtuDataSend);
	free(mModbusRtuDataRec);
	free(mDataSend);
	free(mDataRec);
	URT2_vInit(&mPartitionTable.tCom2Config.tUartConfig);
	mDataSend = malloc(CONFIG_UART_RX_BUFFSIZE);
	mDataRec = malloc(CONFIG_UART_RX_BUFFSIZE);
	/* set master or slave mode to send/receive Rtu data  */
	if(mPartitionTable.tCom2Config.usProtocolType == PROTOCOL_TYPE_MODBUS_RTU)
	{
		mModbusRtuDataSend = malloc(sizeof(ModbusRtuDataTPDF));
		mModbusRtuDataRec = malloc(sizeof(ModbusRtuDataTPDF));
		if(mPartitionTable.tCom2Config.usJointPortType == JOINT_PORT_DOWN)
		{
			mModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_MASTER;
			mModbusRtuDataRec->tModbusMstSlvMode = MODBUS_MODE_MASTER;
		}
		else
		{
			mModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
			mModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
		}
	}
}

void vCOM2_JointDown_PassThrough_Modbus(void)
{
	const char *COM_P = "COM2:Passthrough";
	const char *COM_PR = "COM2:Passthrough:Rec";
	MyQueueDataTPDF tMyQueueData;
	uint16_t usDelay;
	uint16_t i, j;
	if(xQueueReceive(mComm2QueueSend, (void * )&tMyQueueData, (portTickType)0))
	{
		if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == Sntp)
		{
			time_t tNow;
			struct tm tTimeInfo;
			time(&tNow);
			localtime_r(&tNow, &tTimeInfo);
			mModbusRtuDataSend->ucSlaveAddr = 0;
			mModbusRtuDataSend->tFunction = WriteMultReg;
			mModbusRtuDataSend->usRegAddr = 0x0140;
			mModbusRtuDataSend->usRegCount = 3;
			mModbusRtuDataSend->usDataLen = 6;
			mModbusRtuDataSend->ucData[0] = tTimeInfo.tm_year - 100;
			mModbusRtuDataSend->ucData[1] = tTimeInfo.tm_mon + 1;
			mModbusRtuDataSend->ucData[2] = tTimeInfo.tm_mday;
			mModbusRtuDataSend->ucData[3] = tTimeInfo.tm_hour;
			mModbusRtuDataSend->ucData[4] = tTimeInfo.tm_min;
			mModbusRtuDataSend->ucData[5] = tTimeInfo.tm_sec;
			i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
			URT2_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			return;
		}
		else if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == BleAdv)
		{
			mModbusRtuDataSend->ucSlaveAddr = 0;
			mModbusRtuDataSend->tFunction = WriteReg;
			mModbusRtuDataSend->usRegAddr = 0x282E;
			mModbusRtuDataSend->usRegCount = 1;
			mModbusRtuDataSend->usDataLen = 2;
			mModbusRtuDataSend->ucData[0] = 0x5a;
			mModbusRtuDataSend->ucData[1] = 0xa5;
			i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
			URT2_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			return;
		}
		else if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == BroadcastAddr)
		{
			mModbusRtuDataSend->ucSlaveAddr = 0;
			mModbusRtuDataSend->tFunction = WriteMultReg;
			mModbusRtuDataSend->usRegAddr = 0x282F;
			mModbusRtuDataSend->usRegCount = 4;
			mModbusRtuDataSend->usDataLen = 8;
			for(i = 0; i < 8; i++)
			{
				mModbusRtuDataSend->ucData[i] = tMyQueueData.tMyQueueCommand.cData[i];
			}
			mModbusRtuDataRec->ucSlaveAddr = tMyQueueData.tMyQueueCommand.cData[7];
			i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
			URT2_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			usDelay = mPartitionTable.tCom2Config.usTimeout / 10;
			while(usDelay)
			{
				vTaskDelay(1);
				usDelay--;
				if(URT2_dRx(mDataRec, &j))
				{
					esp_log_buffer_hex(COM_PR, mDataRec, j);
					break;
				}
			}
			if(!usDelay)
			{
				ESP_LOGE(COM_P, "time out");
				return;
			}
			MODB_vAnalysis((uint8_t *)mDataRec, j, mModbusRtuDataRec);
			if(mModbusRtuDataRec->ucEffect == MODBUS_RESULT_FAIL)
			{
				printf("COM2:passtough:ana fail\n");
				return;
			}
			mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
			tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mModbusRtuDataRec->tFunction;
			tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
			tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
			tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
			memcpy(tMyQueueData.tMyQueueCommand.cData,mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
			xQueueSend(mComm2QueueRec, (void * )&tMyQueueData, (TickType_t)0);
			return;
		}
		mModbusRtuDataSend->tFunction = tMyQueueData.tMyQueueCommand.tMyQueueCommandType;
		mModbusRtuDataSend->ucSlaveAddr = tMyQueueData.tMyQueueCommand.usSlaveAddr;
		mModbusRtuDataSend->usRegAddr = tMyQueueData.tMyQueueCommand.usRegAddr;
		mModbusRtuDataSend->usRegCount = tMyQueueData.tMyQueueCommand.usRegLen;
		mModbusRtuDataSend->usDataLen = tMyQueueData.tMyQueueCommand.usDatalen;
		memcpy(mModbusRtuDataSend->ucData,tMyQueueData.tMyQueueCommand.cData, tMyQueueData.tMyQueueCommand.usDatalen);
		mModbusRtuDataRec->ucSlaveAddr = mModbusRtuDataSend->ucSlaveAddr;
		i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
		URT2_dData_Send(mDataSend, i);
		esp_log_buffer_hex(COM_P, mDataSend, i);
		usDelay = mPartitionTable.tCom2Config.usTimeout / 10;
		while(usDelay)
		{
			vTaskDelay(1);
			usDelay--;
			if(URT2_dRx(mDataRec, &j))
			{
				esp_log_buffer_hex(COM_PR, mDataRec, j);
				break;
			}
		}
		if(!usDelay)
		{
			ESP_LOGE(COM_P, "time out");
			return;
		}
		MODB_vAnalysis((uint8_t *)mDataRec, j, mModbusRtuDataRec);
		if(mModbusRtuDataRec->ucEffect == MODBUS_RESULT_FAIL)
		{
			printf("COM2:passtough:ana fail\n");
			return;
		}
		mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
		tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mModbusRtuDataRec->tFunction;
		tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
		tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
		tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
		memcpy(tMyQueueData.tMyQueueCommand.cData,mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
		xQueueSend(mComm2QueueRec, (void * )&tMyQueueData, (TickType_t)0);
	}
}

void vCOM2_JointDown_TransMit_Modbus(void)
{
	uint16_t i, j, k, l;
	uint16_t usDelay;
	vCOM2_JointDown_PassThrough_Modbus();
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		vCOM2_JointDown_PassThrough_Modbus();
		if(mDevice[i].tDeviceConfig.usCommPort != DEVICE_COMMPORT_TYPE_COM2)
		{
			continue;
		}
		for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
		{
			vTaskDelay(1);
			vCOM2_JointDown_PassThrough_Modbus();
			if(mDevice[i].tDevicePartitionTable[j].usLen)
			{
				mModbusRtuDataSend->tFunction = mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableRegType;
				mModbusRtuDataSend->ucSlaveAddr = mDevice[i].tDeviceConfig.usConvertAddr;
				mModbusRtuDataSend->usRegAddr = mDevice[i].tDevicePartitionTable[j].usStartAddr;
				mModbusRtuDataSend->usRegCount = mDevice[i].tDevicePartitionTable[j].usLen;
				k = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
				mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				mModbusRtuDataRec->ucSlaveAddr = mModbusRtuDataSend->ucSlaveAddr;
				URT2_dData_Send(mDataSend, k);
				usDelay = mPartitionTable.tCom2Config.usTimeout / 10;
				while(usDelay)
				{
					vTaskDelay(1);
					usDelay--;
					if(URT2_dRx(mDataRec, &l))
					{
						break;
					}
				}
				if(!usDelay)
				{
					continue;
				}
				MODB_vAnalysis((uint8_t *)mDataRec, l, mModbusRtuDataRec);
				if(mModbusRtuDataRec->ucEffect == MODBUS_RESULT_FAIL)
				{
					continue;
				}
				if(mModbusRtuDataRec->tFunction == mModbusRtuDataSend->tFunction)
				{
					for(l = 0; l < mDevice[i].tDevicePartitionTable[j].usLen; l++)
					{
						mDevice[i].tDevicePartitionTable[j].usMemory[l] =
									(mModbusRtuDataRec->ucData[l * 2] << 8) + mModbusRtuDataRec->ucData[l * 2 + 1];
					}
				}
			}
		}
	}
	vTaskDelay(1);
}
/**
 * @brief	com2 task start.
 * @param pvParameters	when create a task, it can transmit parameter from outside
 * @return	no return value
 */
void COM2_vTsk_Start(void *pvParameters)
{
	vCOM2_Init();
	while(1)
	{
		vTaskDelay(1);
		switch(mPartitionTable.tCom1Config.usProtocolType)
		{
			case PROTOCOL_TYPE_MODBUS_RTU:
				if(mPartitionTable.tCom2Config.usJointPortType == JOINT_PORT_DOWN)
				{
					if(mPartitionTable.usGatewayRunMode == GATEWAY_MODE_TRANSMIT)
					{
						vCOM2_JointDown_TransMit_Modbus();
					}
					else
					{
						vCOM2_JointDown_PassThrough_Modbus();
					}
				}
				break;
		}
	}
}
