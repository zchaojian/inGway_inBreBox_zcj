/*
 * comm1_tsk.c
 *
 *  Created on: Jul 31, 2020
 *      Author: liwei
 */
#include "comm1_tsk.h"

static const char *TAG = "Comm1_Task";

static ModbusRtuDataTPDF *mModbusRtuDataSend;
static ModbusRtuDataTPDF *mModbusRtuDataRec;
/*static DeviceTPDF *mComm1Device[CONFIG_DEVICE_MAXSURPPORT];
static uint8_t mComm1DeviceCount = 0;*/
static char *mDataSend;
static char *mDataRec;

QueueHandle_t mComm1QueueSend;
QueueHandle_t mComm1QueueRec;

const char * COM1_TAG = "COM1";

static void vCOM1_Init(void)
{
	free(mModbusRtuDataSend);
	free(mModbusRtuDataRec);
	free(mDataSend);
	free(mDataRec);
	URT1_vInit(&mPartitionTable.tCom1Config.tUartConfig);
	mDataSend = malloc(CONFIG_UART_RX_BUFFSIZE);
	mDataRec = malloc(CONFIG_UART_RX_BUFFSIZE);
	/* set master or slave mode to send/receive Rtu data  */
	if(mPartitionTable.tCom1Config.usProtocolType == PROTOCOL_TYPE_MODBUS_RTU)
	{
		mModbusRtuDataSend = malloc(sizeof(ModbusRtuDataTPDF));
		mModbusRtuDataRec = malloc(sizeof(ModbusRtuDataTPDF));
		if(mPartitionTable.tCom1Config.usJointPortType == JOINT_PORT_DOWN)
		{
			mModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_MASTER;
			mModbusRtuDataRec->tModbusMstSlvMode = MODBUS_MODE_MASTER;
		}
		else
		{
			mModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
			mModbusRtuDataRec->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
		}
	}
	/*for(uint8_t i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		if(mDevice[i].tDeviceConfig.usCommPort == DEVICE_COMMPORT_TYPE_COM1)
		{
			mComm1Device[mComm1DeviceCount] = &mDevice[i];
			mComm1DeviceCount++;
		}
	}*/
}

void vCOM1_JointDown_PassThrough_Modbus(void)
{
	const char *COM_P = "COM1:Passthrough";
	const char *COM_PR = "COM1:Passthrough:Rec";
	MyQueueDataTPDF tMyQueueData;
	uint16_t usDelay;
	uint16_t i, j, n, m;
	if(xQueueReceive(mComm1QueueSend, (void * )&tMyQueueData, (portTickType)0))
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
			URT1_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			return;
		}
		/*else if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == BleAdv)
		{
			mModbusRtuDataSend->ucSlaveAddr = 0;
			mModbusRtuDataSend->tFunction = WriteReg;
			mModbusRtuDataSend->usRegAddr = 0x282E;
			mModbusRtuDataSend->usRegCount = 1;
			mModbusRtuDataSend->usDataLen = 2;
			mModbusRtuDataSend->ucData[0] = 0x5a;
			mModbusRtuDataSend->ucData[1] = 0xa5;
			i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
			URT1_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			return;
		}*/
		else if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == BleAdv)
		{
			mModbusRtuDataSend->ucSlaveAddr = 0;
			mModbusRtuDataSend->tFunction = WriteMultReg;
			mModbusRtuDataSend->usRegAddr = 0x0000;
			mModbusRtuDataSend->usRegCount = 0x0001;
			mModbusRtuDataSend->usDataLen = 2;
			mModbusRtuDataSend->ucData[0] = 0x00;
			mModbusRtuDataSend->ucData[1] = 0x04;
			i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
			URT1_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			return;
		}
		else if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == BroadcastAddr)
		{
			mModbusRtuDataSend->ucSlaveAddr = 0;
			mModbusRtuDataSend->tFunction = WriteMultReg;
			mModbusRtuDataSend->usRegAddr = 0x0000;//0x282F
			mModbusRtuDataSend->usRegCount = 1 + 4;//4
			mModbusRtuDataSend->usDataLen = 2 + 8;//8
			mModbusRtuDataSend->ucData[0] = 0x00;
			mModbusRtuDataSend->ucData[1] = 0x05;
			for(i = 0; i < 8; i++)
			{
				mModbusRtuDataSend->ucData[i + 2] = tMyQueueData.tMyQueueCommand.cData[i];
			}
			mModbusRtuDataRec->ucSlaveAddr = tMyQueueData.tMyQueueCommand.cData[7];
			ESP_LOGW(TAG, "tFunction code:%d", mModbusRtuDataSend->tFunction);
			i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
			URT1_dData_Send(mDataSend, i);
			esp_log_buffer_hex(COM_P, mDataSend, i);
			usDelay = mPartitionTable.tCom1Config.usTimeout / 10;
			while(usDelay)
			{
				vTaskDelay(1);
				usDelay--;
				if(URT1_dRx(mDataRec, &j))
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
				ESP_LOGE(COM_P, "ana fail");
				return;
			}
			mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
			tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mModbusRtuDataRec->tFunction;
			tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
			tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
			tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
			memcpy(tMyQueueData.tMyQueueCommand.cData,mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
			xQueueSend(mComm1QueueRec, (void * )&tMyQueueData, (TickType_t)0);
			return;
		}
		else if(tMyQueueData.tMyQueueCommand.tMyQueueCommandType == MultiDeviceCtrl)
		{
			uint16_t usCtrlDeviceNum = tMyQueueData.tMyQueueCommand.usRegLen - 2;
			uint16_t usDelayCtrlTime;
			esp_log_buffer_hex(COM_P, tMyQueueData.tMyQueueCommand.cData, (usCtrlDeviceNum + 2) * 2 );
			ESP_LOGW(TAG, "CtrlDeviceNum:%d", usCtrlDeviceNum);
			for(n = 0; n < usCtrlDeviceNum; n++)
			{
				printf("n: %d\n",n);
				mModbusRtuDataSend->ucSlaveAddr = (uint8_t)((tMyQueueData.tMyQueueCommand.cData[4 + 2 * n] << 8) + \
															(tMyQueueData.tMyQueueCommand.cData[4 + 2 * n + 1]));
				mModbusRtuDataSend->tFunction = WriteMultReg;
				mModbusRtuDataSend->usRegAddr = 0x0000;//0x282F
				mModbusRtuDataSend->usRegCount = 1;
				mModbusRtuDataSend->usDataLen = 2;
				mModbusRtuDataSend->ucData[0] = tMyQueueData.tMyQueueCommand.cData[0];
				mModbusRtuDataSend->ucData[1] = tMyQueueData.tMyQueueCommand.cData[1];
				mModbusRtuDataRec->ucSlaveAddr = mModbusRtuDataSend->ucSlaveAddr;
				esp_log_buffer_hex(COM_P, mModbusRtuDataSend->ucData, 2);
				usDelayCtrlTime = (tMyQueueData.tMyQueueCommand.cData[2] << 8) + tMyQueueData.tMyQueueCommand.cData[3];
				ESP_LOGW(TAG, "usDelayCtrlTime:%d", usDelayCtrlTime);
				i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
				printf("MODB_dBuild  i: %d\n",i);
				vTaskDelay(usDelayCtrlTime / 10 );
				URT1_dData_Send(mDataSend, i);
				esp_log_buffer_hex(COM_P, mDataSend, i);
				usDelay = mPartitionTable.tCom1Config.usTimeout / 10;
				while(usDelay)
				{
					vTaskDelay(1);
					usDelay--;
					if(URT1_dRx(mDataRec, &j))
					{
						esp_log_buffer_hex(COM_PR, mDataRec, j);
						break;
					}
				}
				if(!usDelay)
				{
					ESP_LOGE(COM_P, "time out");
					mModbusRtuDataRec->ucData[2 * n] = 0x01;
					tMyQueueData.tMyQueueCommand.cData[4 + 2 * n] = mModbusRtuDataRec->ucData[2 * n];
					printf("cData: %d\n",tMyQueueData.tMyQueueCommand.cData[4 + 2 * n]);
					continue;
				}
				MODB_vAnalysis((uint8_t *)mDataRec, j, mModbusRtuDataRec);
				if(mModbusRtuDataRec->ucEffect == MODBUS_RESULT_FAIL)
				{
					ESP_LOGE(COM_P, "ana fail");
					mModbusRtuDataRec->ucData[2 * n] = 0x01;
					tMyQueueData.tMyQueueCommand.cData[4 + 2 * n] = mModbusRtuDataRec->ucData[2 * n];
					//return;
				}
				/*mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				tMyQueueData.tMyQueueCommand.tMyQueueCommandType = 0xf8;//mModbusRtuDataRec->tFunction;
				tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
				tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
				tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
				memcpy(tMyQueueData.tMyQueueCommand.cData,mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
				xQueueSend(mComm1QueueRec, (void * )&tMyQueueData, (TickType_t)0);*/

				mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				if(mModbusRtuDataRec->tFunction == WriteMultReg)
				{
					mModbusRtuDataRec->ucData[2 * n] = 0x00;
				}
				else
				{
					mModbusRtuDataRec->ucData[2 * n] = 0x01;
				}
				tMyQueueData.tMyQueueCommand.cData[4 + 2 * n] = mModbusRtuDataRec->ucData[2 * n];
			}
			esp_log_buffer_hex(COM_PR, tMyQueueData.tMyQueueCommand.cData, (usCtrlDeviceNum + 2) * 2);
			mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
			/*tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mModbusRtuDataRec->tFunction;
			tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
			tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
			tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
			memcpy(tMyQueueData.tMyQueueCommand.cData,mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);*/
			//xQueueSend(mComm1QueueRec, (void * )&tMyQueueData, (TickType_t)0);
			return;
		}
		mModbusRtuDataSend->tFunction = tMyQueueData.tMyQueueCommand.tMyQueueCommandType;
		mModbusRtuDataSend->ucSlaveAddr = tMyQueueData.tMyQueueCommand.usSlaveAddr;
		mModbusRtuDataSend->usRegAddr = tMyQueueData.tMyQueueCommand.usRegAddr;
		mModbusRtuDataSend->usRegCount = tMyQueueData.tMyQueueCommand.usRegLen;
		mModbusRtuDataSend->usDataLen = tMyQueueData.tMyQueueCommand.usDatalen;
		memcpy(mModbusRtuDataSend->ucData,tMyQueueData.tMyQueueCommand.cData, tMyQueueData.tMyQueueCommand.usDatalen);
		mModbusRtuDataRec->ucSlaveAddr = mModbusRtuDataSend->ucSlaveAddr;
		ESP_LOGW(TAG, "mModbusRtuDataSend->tFunction:0x%02x----", mModbusRtuDataSend->tFunction);
		i = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
		URT1_dData_Send(mDataSend, i);
		esp_log_buffer_hex(COM_P, mDataSend, i);
		usDelay = mPartitionTable.tCom1Config.usTimeout / 10;
		while(usDelay)
		{
			vTaskDelay(1);
			usDelay--;
			if(URT1_dRx(mDataRec, &j))
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
			ESP_LOGE(COM_P, "ana fail");
			return;
		}
		mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
		tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mModbusRtuDataRec->tFunction;
		tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
		tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
		tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
		memcpy(tMyQueueData.tMyQueueCommand.cData,mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
		xQueueSend(mComm1QueueRec, (void * )&tMyQueueData, (TickType_t)0);
	}
}


void vCOM1_JointDown_TransMit_Modbus(void)
{
	uint16_t i, j, k, l;
	uint16_t usDelay;
	vCOM1_JointDown_PassThrough_Modbus();
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		vCOM1_JointDown_PassThrough_Modbus();
		//ESP_LOGI(TAG, "mDevice[%d].tDeviceConfig.usCommPort: %d", i, mDevice[i].tDeviceConfig.usCommPort);
		if(mDevice[i].tDeviceConfig.usCommPort != DEVICE_COMMPORT_TYPE_COM1)
		{
			continue;
		}
		for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
		{
			vTaskDelay(1);
			vCOM1_JointDown_PassThrough_Modbus();
			//ESP_LOGI(TAG, "mDevice[%d].tDevicePartitionTable[%d].usLen: %d", i, j, mDevice[i].tDevicePartitionTable[j].usLen);
			if(mDevice[i].tDevicePartitionTable[j].usLen)
			{
				mModbusRtuDataSend->tFunction = mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableRegType;
				mModbusRtuDataSend->ucSlaveAddr = mDevice[i].tDeviceConfig.usConvertAddr;
				mModbusRtuDataSend->usRegAddr = mDevice[i].tDevicePartitionTable[j].usStartAddr;
				mModbusRtuDataSend->usRegCount = mDevice[i].tDevicePartitionTable[j].usLen;
				k = MODB_dBuild(mModbusRtuDataSend, (uint8_t *)mDataSend);
				mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				mModbusRtuDataRec->ucSlaveAddr = mModbusRtuDataSend->ucSlaveAddr;
				URT1_dData_Send(mDataSend, k);
				usDelay = mPartitionTable.tCom1Config.usTimeout / 10;
				while(usDelay)
				{
					vTaskDelay(1);
					usDelay--;
					if(URT1_dRx(mDataRec, &l))
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
					esp_log_buffer_hex(TAG, mDataRec, l);
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
 * @brief	com1 task start.
 * @param pvParameters	when create a task, it can transmit parameter from outside
 * @return	no return value
 */
void COM1_vTsk_Start(void *pvParameters)
{
	vCOM1_Init();
	while(1)
	{
		vTaskDelay(1);
		switch(mPartitionTable.tCom1Config.usProtocolType)
		{
			case PROTOCOL_TYPE_MODBUS_RTU:
				if(mPartitionTable.tCom1Config.usJointPortType == JOINT_PORT_DOWN)
				{
					if(mPartitionTable.usGatewayRunMode == GATEWAY_MODE_TRANSMIT)
					{
						vCOM1_JointDown_TransMit_Modbus();
					}
					else
					{
						vCOM1_JointDown_PassThrough_Modbus();
					}
				}
				break;
		}
	}
}
