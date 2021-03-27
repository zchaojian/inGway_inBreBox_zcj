/*
 * wifi_socketClient_tsk.c
 *
 *  Created on: Dec 10, 2020
 *      Author: zchaojian
 */

#include "wifi_socketClient_tsk.h"

static uint8_t mWifiClientConnected = 0;
static int mClientSocket = 0;
static uint8_t mConnected = 0;
static uint32_t mClientIP = 0;
static uint16_t mClientPort = 0;
static int mClientAddrFamily;
static ModbusRtuDataTPDF *mClientModbusRtuDataSend;
static ModbusRtuDataTPDF *mClientModbusRtuDataRec;
static char *mClientDataSend;
static char *mClientDataRec;

static uint8_t uUDPServerCreated = 0;

static const char *TAG = "Wifi_SocketClient";

static void vWIFI_SocketClient_Init(void)
{
	static struct sockaddr_in tDestAddr;
	static struct sockaddr_in6 tDestAddr6 = { 0 };
	vTaskDelay(50);
	if(mClientAddrFamily == AF_INET)
	{
		tDestAddr.sin_addr.s_addr = mClientIP;//CONFIG_ETHERNET_SOCKET_CLIENT_IP;
		tDestAddr.sin_family = AF_INET;
		tDestAddr.sin_port = htons(mClientPort);//htons(CONFIG_ETHERNET_SOCKET_CLIENT_PORT);
	}
	else if(mClientAddrFamily == AF_INET6)
	{
		inet6_aton(mClientIP, &tDestAddr6.sin6_addr);
		tDestAddr6.sin6_family = AF_INET6;
		tDestAddr6.sin6_port = htons(mClientPort);
		//dest_addr.sin6_scope_id = esp_netif_get_netif_impl_index(EXAMPLE_INTERFACE);
	}
	mClientSocket=  socket(mClientAddrFamily, SOCK_STREAM, mClientAddrFamily == AF_INET6? IPPROTO_IPV6:IPPROTO_IP);
	if (mClientSocket < 0)
	{
		ESP_LOGE(TAG, "WIFI socket client creat fail!");
		shutdown(mClientSocket, SHUT_RDWR);
		close(mClientSocket);
		return;
	}
	ESP_LOGI(TAG, "WIFI socket client created!");
    int err = connect(mClientSocket, mClientAddrFamily == AF_INET6? (struct sockaddr *)&tDestAddr6: (struct sockaddr *)&tDestAddr,
					sizeof(struct sockaddr_in6));
    if (err != 0)
    {
    	ESP_LOGE(TAG, "WIFI socket client connect fail!");
    	shutdown(mClientSocket, SHUT_RDWR);
    	close(mClientSocket);
        return;
    }
    ESP_LOGI(TAG, "WIFI socket client connected! mClientSocket:%d",mClientSocket);
    mWifiClientConnected = 1;
}

static void vTcp_Client_Task(void *pvParameters)
{
	char cRxBuffer[CONFIG_WIFI_SOCKET_FRAME_SIZE];
	while(1)
	{
		uUDPServerCreated = Udp_Brdcast_Status();
		if(mConnected & uUDPServerCreated)
		{
			if(!mWifiClientConnected)
			{
				vWIFI_SocketClient_Init();
			}
			else
			{
		        while (1)
		        {
		            int iLen = recv(mClientSocket, cRxBuffer, sizeof(cRxBuffer) - 1, 0);
		            // Error occurred during receiving
		            if (iLen < 0)
		            {
		            	ESP_LOGE(TAG, "WIFI socket client data rec fail!");
		                break;
		            }
		            else  // Data received
		            {
		            	cRxBuffer[iLen] = 0; // Null-terminate whatever we received and treat like a string
		            	esp_log_buffer_hex(TAG, cRxBuffer, iLen);
		    			MODB_vAnalysis_TCP((uint8_t *)cRxBuffer, iLen, mClientModbusRtuDataRec);
		    			mClientModbusRtuDataRec->tTag = (void *)mClientSocket;
		            }
		            vTaskDelay(2);
		        }
		        if (mClientSocket != -1)
		        {
		        	ESP_LOGE(TAG, "WIFI socket client disconnected!");
		            shutdown(mClientSocket, 0);
		            close(mClientSocket);
		        }
		        mWifiClientConnected = 0;
			}
		}
		vTaskDelay(200);
	}
}

void Wifi_SocketClient_vInit(uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily)
{
	free(mClientModbusRtuDataSend);
	free(mClientModbusRtuDataRec);
	free(mClientDataSend);
	free(mClientDataRec);
	mClientDataSend = malloc(CONFIG_WIFI_SOCKET_FRAME_SIZE);
	mClientDataRec = malloc(CONFIG_WIFI_SOCKET_FRAME_SIZE);
	mClientIP = uiSocketIP;
	mClientPort = usSocketPort;
	mClientAddrFamily = iAddrFamily;
	mClientModbusRtuDataSend = malloc(sizeof(ModbusRtuDataTPDF));
	mClientModbusRtuDataRec = malloc(sizeof(ModbusRtuDataTPDF));
	mClientModbusRtuDataSend->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
	mClientModbusRtuDataRec->tModbusMstSlvMode = MODBUS_MODE_SLAVE;
	mClientModbusRtuDataSend->ucEffect = MODBUS_RESULT_FAIL;
	mClientModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
	mClientModbusRtuDataSend->ucError = 0;
	mClientModbusRtuDataRec->ucError = 0;

	xTaskCreate(vTcp_Client_Task, "socket client task", 4096, NULL, 4, NULL);
}

void Wifi_SocketClient_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData)
{
	uint16_t i;
	mConnected = ucConnected;
	if(xQueueReceive(mEthQueueSend, (void * )&tMyQueueData, (portTickType)0))
	{
		mClientModbusRtuDataSend->tFunction = tMyQueueData.tMyQueueCommand.tMyQueueCommandType;
		mClientModbusRtuDataSend->ucSlaveAddr = tMyQueueData.tMyQueueCommand.usSlaveAddr;
		mClientModbusRtuDataSend->usRegAddr = tMyQueueData.tMyQueueCommand.usRegAddr;
		mClientModbusRtuDataSend->usRegCount = tMyQueueData.tMyQueueCommand.usRegLen;
		mClientModbusRtuDataSend->usDataLen = tMyQueueData.tMyQueueCommand.usDatalen;
		memcpy(mClientModbusRtuDataSend->ucData,tMyQueueData.tMyQueueCommand.cData, tMyQueueData.tMyQueueCommand.usDatalen);
		mClientModbusRtuDataSend->usHead = tMyQueueData.Reserver[1];
		i = MODB_dBuild_TCP(mClientModbusRtuDataSend, (uint8_t *)mClientDataSend);
		WIFI_vData_Send(tMyQueueData.Reserver[0], mClientDataSend, i);
	}
	if(mClientModbusRtuDataRec->ucEffect == MODBUS_RESULT_SUCCESS)
	{
		if(mClientModbusRtuDataRec->ucSlaveAddr == 0)
		{
			if(mClientModbusRtuDataRec->usRegAddr == 0x01fd && mClientModbusRtuDataRec->tFunction == WriteReg)
			{
				mClientModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				switch(mClientModbusRtuDataRec->ucData[1])
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
				xQueueSend(mEthQueueRec, (void * )&tMyQueueData, (TickType_t)0);
				mClientModbusRtuDataSend->usHead = mClientModbusRtuDataRec->usHead;
				mClientModbusRtuDataSend->ucSlaveAddr = 0;
				mClientModbusRtuDataSend->tFunction = WriteReg;
				mClientModbusRtuDataSend->usRegAddr = 0x01fd;
				mClientModbusRtuDataSend->ucData[0] = mClientModbusRtuDataRec->ucData[0];
				mClientModbusRtuDataSend->ucData[1] = mClientModbusRtuDataRec->ucData[1];
				mClientModbusRtuDataSend->ucError = 0;
				i = MODB_dBuild_TCP(mClientModbusRtuDataSend, (uint8_t *)mClientDataSend);
				WIFI_vData_Send((int)mClientModbusRtuDataRec->tTag, mClientDataSend, i);
			}
			else
			{
				CFG_dProcess_Protocol(mClientModbusRtuDataRec, mClientModbusRtuDataSend);
				if(mClientModbusRtuDataSend->ucEffect == MODBUS_RESULT_SUCCESS)
				{
					mClientModbusRtuDataSend->usHead = mClientModbusRtuDataRec->usHead;
					i = MODB_dBuild_TCP(mClientModbusRtuDataSend, (uint8_t *)mClientDataSend);
					ESP_LOGW(TAG, "before send() mClientModbusRtuDataRec->tTag:%d", (int)mClientModbusRtuDataRec->tTag);
					esp_log_buffer_hex(TAG, mClientDataSend, i);
					WIFI_vData_Send((int)mClientModbusRtuDataRec->tTag, mClientDataSend, i);
				}
			}
		}
		else
		{
			tMyQueueData.tMyQueueCommand.tMyQueueCommandType = mClientModbusRtuDataRec->tFunction;
			tMyQueueData.tMyQueueCommand.usSlaveAddr = mClientModbusRtuDataRec->ucSlaveAddr;
			tMyQueueData.tMyQueueCommand.usRegAddr = mClientModbusRtuDataRec->usRegAddr;
			tMyQueueData.tMyQueueCommand.usRegLen = mClientModbusRtuDataRec->usRegCount;
			tMyQueueData.tMyQueueCommand.usDatalen = mClientModbusRtuDataRec->usDataLen;
			memcpy(tMyQueueData.tMyQueueCommand.cData, mClientModbusRtuDataRec->ucData, tMyQueueData.tMyQueueCommand.usDatalen);
			tMyQueueData.Reserver[0] = (uint32_t)mClientModbusRtuDataRec->tTag;
			tMyQueueData.Reserver[1] = mClientModbusRtuDataRec->usHead;
			xQueueSend(mEthQueueRec, (void * )&tMyQueueData, (TickType_t)0);
		}
		mClientModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
	}
}

void Wifi_vSocketClient_Close(void)
{
	if(mWifiClientConnected)
	{
		close(mClientSocket);
	}
}




