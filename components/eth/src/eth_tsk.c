/*
 * eth_tsk.c
 *
 *  Created on: Jul 26, 2020
 *      Author: liwei
 */
#include "eth_tsk.h"

QueueHandle_t mEthQueueSend;
QueueHandle_t mEthQueueRec;
static ServerProtocolTypeTPDF mServerProtocolType;
uint8_t mConnected = 0;

static const char *TAG = "Eth_Task";

static void vEth_Sntp_CallBack(struct tm *tResult)
{
	MyQueueDataTPDF tMyQueueData;
	tMyQueueData.tMyQueueCommand.tMyQueueCommandType = Sntp;
	xQueueSend(mEthQueueRec, (void * )&tMyQueueData, (TickType_t)0);
	ESP_LOGI(TAG, "SNTP:year:%d,month:%d,day:%d,hour:%d,min:%d,sec:%d\n", tResult->tm_year + 1900,
																   tResult->tm_mon + 1,
																   tResult->tm_mday,
																   tResult->tm_hour,
																   tResult->tm_min,
																   tResult->tm_sec);
}

/* copy eth connected status from ETH_vRegister_Callback() */
void vEth_Event_CallBack(ETHEventTPDF *tEvent)
{
	static uint8_t ucSntpDone = 0;
	switch(tEvent->tType)
	{
		case ETH_IP_GOT:
			mConnected = 1;
			vEth_Dhcp_GetIP_Save_Flash(&(((ip_event_got_ip_t*)tEvent->tTag)->ip_info));
			if(!ucSntpDone)
			{
				SNTP_vInit(vEth_Sntp_CallBack);
				ucSntpDone = 1;
			}
			break;
		case ETH_PHY_DISCONNECTED:
			mConnected = 0;
			Eth_vSocket_Close();
			break;
		default:
			break;
	}
}

/* Use function return Network link status */
uint8_t Eth_Connected_Status(void)
{
	return mConnected;
}

static void vEth_vInit(void)
{
	mServerProtocolType = (ServerProtocolTypeTPDF)0;
	uint32_t uiModbusIP = 0;
	uint16_t usModbusPort = 0;
	ETH_vRegister_Callback(vEth_Event_CallBack);
	if(mPartitionTable.tServer1Config.usCommType == SERVER_COMM_TYPE_ETH)
	{
		uiModbusIP = CFG_dConvertString_To_IP(mPartitionTable.tServer1Config.cIP);
		usModbusPort = mPartitionTable.tServer1Config.usPort;
		mServerProtocolType = (ServerProtocolTypeTPDF)mPartitionTable.tServer1Config.usProtocolType;
	}
	else if(mPartitionTable.tServer2Config.usCommType == SERVER_COMM_TYPE_ETH)
	{
		uiModbusIP = CFG_dConvertString_To_IP(mPartitionTable.tServer2Config.cIP);
		usModbusPort = mPartitionTable.tServer2Config.usPort;
		mServerProtocolType = (ServerProtocolTypeTPDF)mPartitionTable.tServer2Config.usProtocolType;
	}
	else if(mPartitionTable.tServer3Config.usCommType == SERVER_COMM_TYPE_ETH)
	{
		uiModbusIP = CFG_dConvertString_To_IP(mPartitionTable.tServer3Config.cIP);
		usModbusPort = mPartitionTable.tServer3Config.usPort;
		mServerProtocolType = (ServerProtocolTypeTPDF)mPartitionTable.tServer3Config.usProtocolType;
	}
	ESP_LOGI(TAG, "Eth Init:IP = %x,Port = %d",uiModbusIP,usModbusPort);
	ETH_vInit(&mPartitionTable.tEthernetConfig);
	Eth_Socket_vInit(/*uiModbusIP, usModbusPort, AF_INET*/); ///tcp   client  task
	ESP_LOGE(TAG, "mServerProtocolType:%d", mServerProtocolType);
	switch(mServerProtocolType)
	{
		case SERVER_PROTOCOL_TYPE_MQTT:
			//Eth_Mqtt_vInit();
			ESP_LOGE(TAG, "MQTT Task don't support");
			break;
		case SERVER_PROTOCOL_TYPE_MODBUS_RTU:
			Eth_SocketClient_vInit(uiModbusIP, usModbusPort, AF_INET);//tcp   client  task
			break;
		default:
			break;
	}

}

/* Set dhcp acquired IP information to mPartitionTable.tEthernetConfig and save to nvs flash */
void vEth_Dhcp_GetIP_Save_Flash(esp_netif_ip_info_t *ip_info)
{
	uint32_t uiIndex;
	uint32_t uiSize ;
	if(mPartitionTable.tEthernetConfig.EthernetIPType == ETHERNET_IP_TYPE_DHCP)
	{
		mPartitionTable.tEthernetConfig.usIPH = ip4_addr1_16(&(ip_info->ip)) << 8U | (ip4_addr2_16(&(ip_info->ip)) & 0xFF);
		mPartitionTable.tEthernetConfig.usIPL = ip4_addr3_16(&(ip_info->ip)) << 8U | (ip4_addr4_16(&(ip_info->ip)) & 0xFF);
		mPartitionTable.tEthernetConfig.usGateWayH = ip4_addr1_16(&(ip_info->gw)) << 8U | (ip4_addr2_16(&(ip_info->gw)) & 0xFF);
		mPartitionTable.tEthernetConfig.usGateWayL = ip4_addr3_16(&(ip_info->gw)) << 8U | (ip4_addr4_16(&(ip_info->gw)) & 0xFF);
		mPartitionTable.tEthernetConfig.usSubMaskH = ip4_addr1_16(&(ip_info->netmask)) << 8U | (ip4_addr2_16(&(ip_info->netmask)) & 0xFF);
		mPartitionTable.tEthernetConfig.usSubMaskL = ip4_addr3_16(&(ip_info->netmask)) << 8U | (ip4_addr4_16(&(ip_info->netmask)) & 0xFF);

		/* write acquired IP, GateWay, net mask address infor to nvs flash */
		uiIndex = PARTITION_TABLE_ADDR_EHERNET_IP_HADDR - 1;//ETH IP start address: 0x0021 - 1
		uiSize = 6;//IPH, IPL, GateWayH, GateWayL, SubMaskH, SubMaskl,  write to 6 registers.
		while(uiSize)
		{
			CFG_vSaveConfig(uiIndex++);
			uiSize--;
		}
	}
}


uint8_t Eth_dTsk_GetModbusMode(void)
{
	return(mServerProtocolType == SERVER_PROTOCOL_TYPE_MODBUS_RTU);
}

void Eth_vTsk_Start(void *pvParameters)
{
	MyQueueDataTPDF tMyQueueData;

	vEth_vInit();

	while(1)
	{
		Eth_Socket_vTsk(mConnected, tMyQueueData);
		vTaskDelay(1);
		switch(mServerProtocolType)
		{
			case SERVER_PROTOCOL_TYPE_MQTT:
				vTaskDelay(200);
				Eth_Mqtt_vTsk(mConnected, tMyQueueData);
				break;
			case SERVER_PROTOCOL_TYPE_MODBUS_RTU:
				Eth_SocketClient_vTsk(mConnected, tMyQueueData);
				break;
			default:
				break;
		}
	}
}

