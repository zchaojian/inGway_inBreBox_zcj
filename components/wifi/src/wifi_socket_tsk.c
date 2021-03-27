/*
 * wifi_socket_tsk.c
 *
 *  Created on: Oct 12, 2020
 *      Author: zchaojian
 */

#include "wifi_tsk.h"
#include "udp_brdcast.h"

#define WIFI_SERVER_SOCKET_PORT 502

static uint8_t mWifiServerListened = 0;
static int mServerSocket = 0;
static uint8_t mConnected = 0;
static int mAddrFamily;
static ModbusRtuDataTPDF *mModbusRtuDataSend;
static ModbusRtuDataTPDF *mModbusRtuDataRec;
static char *mDataSend;
static char *mDataRec;

static const char *TAG = "Wifi_Socket";

static void vWIFI_SocketServer_Init(void)
{
	static struct sockaddr_in6 tDestAddr;
    static struct sockaddr_in *tDestAddrIp4 = (struct sockaddr_in *)&tDestAddr;
    vTaskDelay(50);
    if(mAddrFamily == AF_INET)
    {
    	tDestAddrIp4->sin_addr.s_addr = htonl(0);
    	tDestAddrIp4->sin_family = AF_INET;
    	tDestAddrIp4->sin_port = htons(WIFI_SERVER_SOCKET_PORT);//mPartitionTable.tEthernetConfig.EthernetPort
    }
    else if (mAddrFamily == AF_INET6)
    {
    	bzero(&tDestAddr.sin6_addr.un, sizeof(tDestAddr.sin6_addr.un));
    	tDestAddr.sin6_family = AF_INET6;
    	tDestAddr.sin6_port = htons(WIFI_SERVER_SOCKET_PORT);//mPartitionTable.tEthernetConfig.EthernetPort);
    }
    mServerSocket = socket(mAddrFamily, SOCK_STREAM, mAddrFamily == AF_INET6? IPPROTO_IPV6:IPPROTO_IP);
    if (mServerSocket < 0)
    {
    	ESP_LOGE(TAG, "Wifi socket server creat fail!");
    	shutdown(mServerSocket, SHUT_RDWR);
    	close(mServerSocket);
    	return;
    }
    ESP_LOGI(TAG, "Wifi socket server created!");

    int iReuseAddrFlag = 1;
    int iErr = setsockopt(mServerSocket, SOL_SOCKET, SO_REUSEADDR, &iReuseAddrFlag, sizeof(iReuseAddrFlag));
    if(iErr < 0)
    {
    	ESP_LOGE(TAG, "wifi socket server reuse addr fiald");
    	close(mServerSocket);
    	return;
    }

    iErr = bind(mServerSocket, (struct sockaddr *)&tDestAddr, sizeof(tDestAddr));
    if (iErr != 0)
    {
    	ESP_LOGE(TAG, "Wifi socket server bind creat fail!");
    	close(mServerSocket);
    	return;
    }
    ESP_LOGI(TAG, "Wifi socket server bind created!");
    iErr = listen(mServerSocket, 1);
    if (iErr != 0)
    {
    	ESP_LOGE(TAG, "Wifi socket server listened fail!");
    	shutdown(mServerSocket, SHUT_RDWR);
    	close(mServerSocket);
    	return;
    }
    ESP_LOGI(TAG, "Wifi socket server listened!");
    mWifiServerListened = 1;
}

/* Use function return tcp socket server establish status */
uint8_t Wifi_SocketServer_Status(void)
{
	return mWifiServerListened;
}

static void vWifi_Server_Task(void *pvParameters)
{
	int iLen;
	char cRxBuffer[CONFIG_WIFI_SOCKET_FRAME_SIZE];
	char *TCP_ServerLog = "wifi_tcp_server";
	while(1)
	{
		ESP_LOGI(TCP_ServerLog, "mConnected: %d", mConnected);
		if(mConnected)
		{
			ESP_LOGI(TCP_ServerLog, "mWifiServerListened: %d", mWifiServerListened);
			if(!mWifiServerListened)
			{
				vWIFI_SocketServer_Init();
			}
			else
			{
		        struct sockaddr_in6 tSourceAddr; // Large enough for both IPv4 or IPv6
		        uint iAddrLen = sizeof(tSourceAddr);
		        int iClientSocket = accept(mServerSocket, (struct sockaddr *)&tSourceAddr, &iAddrLen);
		        if (iClientSocket < 0)
		        {
		        	ESP_LOGE(TAG, "Wifi socket server accept fail!");
		        	mWifiServerListened = 0;
		        }
		        else
		        {
		        	char cAddrStr[128];
		        	// Convert ip address to string
		        	if (tSourceAddr.sin6_family == AF_INET)
		        	{
		            	inet_ntoa_r(((struct sockaddr_in *)&tSourceAddr)->sin_addr.s_addr, cAddrStr, sizeof(cAddrStr) - 1);
		        	}
		        	else if (tSourceAddr.sin6_family == AF_INET6)
		        	{
		            	inet6_ntoa_r(tSourceAddr.sin6_addr, cAddrStr, sizeof(cAddrStr) - 1);
		        	}
		        	ESP_LOGI(TAG, "Wifi socket server client in, ip address: %s", cAddrStr);
		        	iLen = 1;
		        	while(iLen)
		        	{
		            	vTaskDelay(2);
		            	iLen = recv(iClientSocket, cRxBuffer, sizeof(cRxBuffer) - 1, 0);
		               	if (iLen < 0)
		               	{
		               		ESP_LOGE(TAG, "Wifi socket server data rec fail!");
		               		break;
		               	}
		               	else if (iLen == 0)
		               	{
		               		ESP_LOGE(TAG, "Wifi socket server client down!");
		               	}
		               	else
		               	{
		               		cRxBuffer[iLen] = 0;
		               		esp_log_buffer_hex(TCP_ServerLog, cRxBuffer, iLen);
		               		MODB_vAnalysis_TCP((uint8_t *)cRxBuffer, iLen, mModbusRtuDataRec);
		               		mModbusRtuDataRec->tTag = (void *)iClientSocket;
		               	}
		        	}
		        	shutdown(iClientSocket, 0);
		        	close(iClientSocket);
		        }
			}
		}
		vTaskDelay(200);
	}
}

void Wifi_Socket_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData)
{
	uint16_t i;
	mConnected = ucConnected;
	if(xQueueReceive(mWifiQueueSend, (void * )&tMyQueueData, (portTickType)0))
	{
		mModbusRtuDataSend->tFunction = tMyQueueData.tMyQueueCommand.tMyQueueCommandType;
		mModbusRtuDataSend->ucSlaveAddr = tMyQueueData.tMyQueueCommand.usSlaveAddr;
		mModbusRtuDataSend->usRegAddr = tMyQueueData.tMyQueueCommand.usRegAddr;
		mModbusRtuDataSend->usRegCount = tMyQueueData.tMyQueueCommand.usRegLen;
		mModbusRtuDataSend->usDataLen = tMyQueueData.tMyQueueCommand.usDatalen;
		memcpy(mModbusRtuDataSend->ucData,tMyQueueData.tMyQueueCommand.cData, tMyQueueData.tMyQueueCommand.usDatalen);
		mModbusRtuDataSend->usHead = tMyQueueData.Reserver[1];
		i = MODB_dBuild_TCP(mModbusRtuDataSend, (uint8_t *)mDataSend);
		WIFI_vData_Send(tMyQueueData.Reserver[0], mDataSend, i);
	}
	if(mModbusRtuDataRec->ucEffect == MODBUS_RESULT_SUCCESS)//MODBUS_RESULT_SUCCESS means MB RTU data frame analysis success
	{
		if(mModbusRtuDataRec->ucSlaveAddr == 0)
		{
			if(mModbusRtuDataRec->usRegAddr == 0x01fd && mModbusRtuDataRec->tFunction == WriteReg)
			{
				mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				switch(mModbusRtuDataRec->ucData[1])
				{
					case 1:
						tMyQueueData.tMyQueueCommand.tMyQueueCommandType = DeviceDiscoveryReStart;
						break;
					case 2:
						tMyQueueData.tMyQueueCommand.tMyQueueCommandType = DeviceDiscoveryContinue;
						break;
					default:
						tMyQueueData.tMyQueueCommand.tMyQueueCommandType = DeviceDiscoveryStop;
						break;
				}
				xQueueSend(mWifiQueueRec, (void * )&tMyQueueData, (TickType_t)0);
				mModbusRtuDataSend->usHead = mModbusRtuDataRec->usHead;
				mModbusRtuDataSend->ucSlaveAddr = 0;
				mModbusRtuDataSend->tFunction = WriteReg;
				mModbusRtuDataSend->usRegAddr = 0x01fd;
				mModbusRtuDataSend->ucData[0] = mModbusRtuDataRec->ucData[0];
				mModbusRtuDataSend->ucData[1] = mModbusRtuDataRec->ucData[1];
				mModbusRtuDataSend->ucError = 0;
				i = MODB_dBuild_TCP(mModbusRtuDataSend, (uint8_t *)mDataSend);
				WIFI_vData_Send((int)mModbusRtuDataRec->tTag, mDataSend, i);
			}
			else if (mModbusRtuDataRec->usRegAddr == 0x01fd && mModbusRtuDataRec->tFunction == WriteMultReg)
			{
				mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				switch(mModbusRtuDataRec->ucData[1])
				{
					case 1:
						tMyQueueData.tMyQueueCommand.tMyQueueCommandType = DeviceDiscoveryReStart;
						break;
					case 2:
						tMyQueueData.tMyQueueCommand.tMyQueueCommandType = DeviceDiscoveryContinue;
						break;
					default:
						tMyQueueData.tMyQueueCommand.tMyQueueCommandType = DeviceDiscoveryStop;
						break;
				}
				xQueueSend(mWifiQueueRec, (void * )&tMyQueueData, (TickType_t)0);
				mModbusRtuDataSend->usHead = mModbusRtuDataRec->usHead;
				mModbusRtuDataSend->ucSlaveAddr = 0;
				mModbusRtuDataSend->tFunction = WriteMultReg;
				mModbusRtuDataSend->usRegAddr = mModbusRtuDataRec->usRegAddr;
				mModbusRtuDataSend->usDataLen = mModbusRtuDataRec->usDataLen;
				mModbusRtuDataSend->usRegCount = mModbusRtuDataRec->usRegCount;
				mModbusRtuDataSend->ucData[0] = mModbusRtuDataRec->ucData[0];
				mModbusRtuDataSend->ucData[1] = mModbusRtuDataRec->ucData[1];
				mModbusRtuDataSend->ucError = 0;
				i = MODB_dBuild_TCP(mModbusRtuDataSend, (uint8_t *)mDataSend);
				esp_log_buffer_hex(TAG, mDataSend, i);
				WIFI_vData_Send((int)mModbusRtuDataRec->tTag, mDataSend, i);
			}
			else
			{
				CFG_dProcess_Protocol(mModbusRtuDataRec, mModbusRtuDataSend);
				if(mModbusRtuDataSend->ucEffect == MODBUS_RESULT_SUCCESS)
				{
					mModbusRtuDataSend->usHead = mModbusRtuDataRec->usHead;
					i = MODB_dBuild_TCP(mModbusRtuDataSend, (uint8_t *)mDataSend);
					esp_log_buffer_hex(TAG, mDataSend, i);
					WIFI_vData_Send((int)mModbusRtuDataRec->tTag, mDataSend, i);
				}
			}
			//before code is deal with inGway flash data
		}
		else
		{
			//Send MyQueueDataTPDF structure data to WIFI TCP
			tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mModbusRtuDataRec->tFunction;
			tMyQueueData.tMyQueueCommand.usSlaveAddr = mModbusRtuDataRec->ucSlaveAddr;
			tMyQueueData.tMyQueueCommand.usRegAddr = mModbusRtuDataRec->usRegAddr;
			tMyQueueData.tMyQueueCommand.usRegLen = mModbusRtuDataRec->usRegCount;
			tMyQueueData.tMyQueueCommand.usDatalen = mModbusRtuDataRec->usDataLen;
			memcpy(tMyQueueData.tMyQueueCommand.cData, mModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
			tMyQueueData.Reserver[0] = (uint32_t)mModbusRtuDataRec->tTag;
			tMyQueueData.Reserver[1] = mModbusRtuDataRec->usHead;
			xQueueSend(mWifiQueueRec, (void * )&tMyQueueData, (TickType_t)0);
		}
		mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
	}
}

void Wifi_vSocket_Close(void)
{
	if(mWifiServerListened)
	{
		close(mServerSocket);
	}
}

void Wifi_Socket_vInit(/*uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily*/)
{
	free(mModbusRtuDataSend);
	free(mModbusRtuDataRec);
	free(mDataSend);
	free(mDataRec);
	mDataSend = malloc(CONFIG_WIFI_SOCKET_FRAME_SIZE);
	mDataRec = malloc(CONFIG_WIFI_SOCKET_FRAME_SIZE);
	mAddrFamily = AF_INET;
	mModbusRtuDataSend = malloc(sizeof(ModbusRtuDataTPDF));
	mModbusRtuDataRec = malloc(sizeof(ModbusRtuDataTPDF));
	mModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
	mModbusRtuDataRec->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
	mModbusRtuDataSend->ucEffect = MODBUS_RESULT_FAIL;
	mModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
	mModbusRtuDataSend->ucError = 0;
	mModbusRtuDataRec->ucError = 0;

	//tcp server task only passthrough data from upper computer.
	xTaskCreate(vWifi_Server_Task, "wifi socket server task", 4096, NULL, 4, NULL);
	Udp_Brdcast_Start();
	Native_Ota_Start();
}
