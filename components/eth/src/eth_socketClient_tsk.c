/*
 * eth_socketClient_tsk.c
 *
 *  Created on: Dec 10, 2020
 *      Author: zchaojian
 */

#include "eth_socketClient_tsk.h"

static uint8_t mEthClientConnected = 0;
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

static const char *TAG = "Eth_SocketClient";

/* define tcp client send timer*/
TimerHandle_t TcpClientSendTimerHandle;
/* define a timer callback function */
static void vTcpClientSendTimerCallback(TimerHandle_t xTimer);
static void vStartTcpClientTimer();


static void vETH_SocketClient_Init(void)
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
		ESP_LOGE(TAG, "Ethernet socket client creat fail!");
		shutdown(mClientSocket, SHUT_RDWR);
		close(mClientSocket);
		return;
	}
	ESP_LOGI(TAG, "Ethernet socket client created!");
    int err = connect(mClientSocket, mClientAddrFamily == AF_INET6? (struct sockaddr *)&tDestAddr6: (struct sockaddr *)&tDestAddr,
					sizeof(struct sockaddr_in6));
    if (err != 0)
    {
    	ESP_LOGE(TAG, "Ethernet socket client connect fail!");
    	shutdown(mClientSocket, SHUT_RDWR);
    	close(mClientSocket);
        return;
    }
    ESP_LOGI(TAG, "Ethernet socket client connected! mClientSocket:%d",mClientSocket);
    mEthClientConnected = 1;
    vStartTcpClientTimer();
}

static void vTcp_Client_Task(void *pvParameters)
{
	char cRxBuffer[CONFIG_ETHERNET_SOCKET_FRAME_SIZE];
	while(1)
	{
		uUDPServerCreated = Udp_Brdcast_Status();
		if(mConnected & uUDPServerCreated)
		{
			if(!mEthClientConnected)
			{
				vETH_SocketClient_Init();
			}
			else
			{
#ifdef ELECON_FCONVERTER_DEVICE_CODE
				send(mClientSocket, ELECON_FCONVERTER_DEVICE_CODE, sizeof(ELECON_FCONVERTER_DEVICE_CODE) - 1, 0);
				ESP_LOGI(TAG, "Send ELECON_FCONVERTER_DEVICE_CODE success");
#endif
		        while (1)
		        {
		            int iLen = recv(mClientSocket, cRxBuffer, sizeof(cRxBuffer) - 1, 0);
		            // Error occurred during receiving
		            if (iLen < 0)
		            {
		            	ESP_LOGE(TAG, "Ethernet socket client data rec fail!");
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
		        	ESP_LOGE(TAG, "Ethernet socket client disconnected!");
		            shutdown(mClientSocket, 0);
		            close(mClientSocket);
		        }
		        mEthClientConnected = 0;
			}
		}
		vTaskDelay(200);
	}
}

void Eth_SocketClient_vInit(uint32_t uiSocketIP, uint16_t usSocketPort, int iAddrFamily)
{
	free(mClientModbusRtuDataSend);
	free(mClientModbusRtuDataRec);
	free(mClientDataSend);
	free(mClientDataRec);
	mClientDataSend = malloc(CONFIG_ETHERNET_SOCKET_FRAME_SIZE);
	mClientDataRec = malloc(CONFIG_ETHERNET_SOCKET_FRAME_SIZE);
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

void Eth_SocketClient_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData)
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
		ETH_vData_Send(tMyQueueData.Reserver[0], mClientDataSend, i);
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
				ETH_vData_Send((int)mClientModbusRtuDataRec->tTag, mClientDataSend, i);
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
					ETH_vData_Send((int)mClientModbusRtuDataRec->tTag, mClientDataSend, i);
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

void Eth_vSocketClient_Close(void)
{
	if(mEthClientConnected)
	{
		close(mClientSocket);
	}
}

void vStartTcpClientTimer()
{
	BaseType_t ret = 0;
	TcpClientSendTimerHandle = xTimerCreate((const char*)"TcpClientSendTimer",              /* 软件定时器名称 */
						        (TickType_t )(10 * 1000 / portTICK_PERIOD_MS),  /* 定时周期，单位为时钟节拍 */
							    (UBaseType_t)pdTRUE,                       		/* 定时器模式，是否为周期定时模式 */
							    (void*)1,                                   	/* 定时器ID号 */
							    (TimerCallbackFunction_t)vTcpClientSendTimerCallback);	/*定时器回调函数 */

	if((TcpClientSendTimerHandle != NULL))
        ret = xTimerStart(TcpClientSendTimerHandle,0);	    /* 创建成功，开启定时器*/
    else
    	ESP_LOGE(TAG, "TCP Client Timer Create failure !!! \n");  /* 定时器创建失败 */

    if(ret == pdPASS)
        ESP_LOGI(TAG, "TCP Client Timer Start OK.");           /* 定时器启动成功*/
    else
    	ESP_LOGE(TAG,"TCP Client Timer Start err. \n");          /* 定时器启动失败*/
}

void vTcpClientSendTimerCallback(TimerHandle_t xTimer)
{
    int uiSendDataFlag;
    char *cHeart = "CC";
    if(mEthClientConnected > 0)
    {
    	uiSendDataFlag = send(mClientSocket, cHeart, 2, 0);
    	if(uiSendDataFlag < 0)
    	{
    		Eth_vSocketClient_Close();
    		esp_timer_stop(TcpClientSendTimerHandle);
    		//esp_timer_delete(esp_timer_delete);
    	}
    }
}
