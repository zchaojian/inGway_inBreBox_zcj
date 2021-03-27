/*
 * variable_app.c
 *
 *  Created on: Jul 20, 2020
 *      Author: liwei
 */
#include "config_app.h"

#define CONFIG_DEFAULT_VALUE_SERVER1_PORT			2345
#define CONFIG_DEFAULT_VALUE_SERVER2_PORT			2346
#define CONFIG_DEFAULT_VALUE_SERVER3_PORT			2347
#define CONFIG_DEFAULT_VALUE_SERIES_CODE			0x0601
#define CONFIG_DEFAULT_VALUE_VER_ID					0x0001
#define CONFIG_DEFAULT_VALUE_VERSION				0x0101

#define WIFI_STA_SSID		0x003B	// 16B
#define WIFI_STA_PASSWORD	0x0043	// 16B
#define WIFI_AP_SSID		0x0055	// 16B
#define WIFI_AP_PASSWORD	0x005D	// 16B
#define SERVER1_IP			0x0073	// 32B
#define SERVER2_IP			0x008A	// 32B
#define SERVER3_IP			0x00A1	// 32B
#define SN_CODE				0x020F	// 24B
#define PRODUCT_DESCRIPTION	0x021B	// 24B
#define GATEWAY_ASSERT_CODE	0x0228	// 16B
#define DEVICE01_ASSERT_CODE	0x0230	// 16B
#define DEVICE02_ASSERT_CODE	0x0238	// 16B
#define DEVICE03_ASSERT_CODE	0x0240	// 16B
#define DEVICE04_ASSERT_CODE	0x0248	// 16B
#define DEVICE05_ASSERT_CODE	0x0250	// 16B
#define DEVICE06_ASSERT_CODE	0x0258	// 16B
#define DEVICE07_ASSERT_CODE	0x0260	// 16B
#define DEVICE08_ASSERT_CODE	0x0268	// 16B
#define DEVICE09_ASSERT_CODE	0x0270	// 16B
#define DEVICE10_ASSERT_CODE	0x0278	// 16B
#define DEVICE11_ASSERT_CODE	0x0280	// 16B
#define DEVICE12_ASSERT_CODE	0x0288	// 16B
#define DEVICE13_ASSERT_CODE	0x0290	// 16B
#define DEVICE14_ASSERT_CODE	0x0298	// 16B
#define DEVICE15_ASSERT_CODE	0x02A0	// 16B
#define DEVICE16_ASSERT_CODE	0x02A8	// 16B
#define DEVICE17_ASSERT_CODE	0x02B0	// 16B
#define DEVICE18_ASSERT_CODE	0x02B8	// 16B
#define DEVICE19_ASSERT_CODE	0x02C0	// 16B
#define DEVICE20_ASSERT_CODE	0x02C8	// 16B
#define DEVICE21_ASSERT_CODE	0x02D0	// 16B
#define DEVICE22_ASSERT_CODE	0x02D8	// 16B
#define DEVICE23_ASSERT_CODE	0x02E0	// 16B
#define DEVICE24_ASSERT_CODE	0x02E8	// 16B
#define DEVICE25_ASSERT_CODE	0x02F0	// 16B
#define DEVICE26_ASSERT_CODE	0x03F8	// 16B
#define DEVICE27_ASSERT_CODE	0x0300	// 16B
#define DEVICE28_ASSERT_CODE	0x0308	// 16B
#define DEVICE29_ASSERT_CODE	0x0310	// 16B
#define DEVICE30_ASSERT_CODE	0x0318	// 16B
#define DEVICE31_ASSERT_CODE	0x0320	// 16B
#define DEVICE32_ASSERT_CODE	0x0328	// 16B
#define PLATFORM_IP				0x0340	// 32B
#define PLATFORM_CLIENT_ID		0x0350	// 32B
#define PLATFORM_USERNAME		0x0360	// 32B
#define PLATFORM_PASSWORD		0x0370	// 32B
#define PLATFORM_LWT_TOPIC		0x0380	// 16B
#define PLATFORM_LWT_MSG		0x0388	// 16B
#define PLATFORM_SUB_TOPIC		0x0390	// 64B
#define PLATFORM_PUB_TOPIC		0x03B0	// 64B

PartitionTableTPDF mPartitionTable;
const char *mCfgNusNamespace = "VarPrt";
nvs_handle  mCfgNusHandle;
const char *TAG = "CFG";

void ConvertUint16ToString(uint16_t *source, char *dest, int number)
{
	char temp = 0;
	memcpy(dest, (char *)source, number*2);
	for(int i=0; i < number; i++)
	{
		temp = dest[i];
		dest[i] = dest[i + 1];
		dest[i + 1] = temp;
		i++;
	}
}

/* transmit hex registers addr to a Decimal register addr */
char *CFG_dGet_Partition_Desc(uint16_t usIndex)
{
	static char cRet[] = "Addr\0\0\0\0";
	cRet[4] = 0x30 + ((usIndex / 100) % 10);
	cRet[5] = 0x30 + ((usIndex / 10) % 10);
	cRet[6] = 0x30 + (usIndex % 10);
	return(cRet);
}

static void vCfg_Device_Delete(uint16_t usDeviceIndex)
{
	uint32_t uiDeviceAddr = ((uint32_t)(&mPartitionTable.tDeviceConfig[0]) - (uint32_t)(&mPartitionTable) + (usDeviceIndex * sizeof(DeviceConfigTPDF))) >> 1;
	uint32_t uiCount = (sizeof(DeviceConfigTPDF) * (CONFIG_DEVICE_MAXSURPPORT - usDeviceIndex - 1)) >> 1;
	uint16_t i;
	if(usDeviceIndex < CONFIG_DEVICE_MAXSURPPORT - 1)
	{
		for(i = usDeviceIndex; i < CONFIG_DEVICE_MAXSURPPORT - 1; i++)
		{
			memcpy(&mPartitionTable.tDeviceConfig[i], &mPartitionTable.tDeviceConfig[i + 1], sizeof(DeviceConfigTPDF));
		}
		memset(&mPartitionTable.tDeviceConfig[CONFIG_DEVICE_MAXSURPPORT - 1], 0, sizeof(DeviceConfigTPDF));
		for(i = 0; i < uiCount; i++)
		{
			CFG_vSaveConfig(i + uiDeviceAddr);
		}
	}
}

uint32_t CFG_dConvertString_To_IP(char *cChar)
{
	char cIP[32];
	for(uint8_t i = 0; i < 10; i++)
	{
		cIP[i*2] = cChar[i*2 + 1];
		cIP[i*2 + 1] = cChar[i*2];
	}
	return(inet_addr(cIP));
}


uint32_t CFG_dConvert_To_IP(uint16_t usIpH, uint16_t usIpL)
{
	uint8_t i[4];
	i[0] = (usIpH >> 8) & 0x00ff;
	i[1] = usIpH & 0x00ff;
	i[2] = (usIpL >> 8) & 0x00ff;
	i[3] = usIpL & 0x00ff;
	return(i[0] + (i[1] <<8) + (i[2] << 16) + (i[3] << 24));
}


void CFG_dConvert_To_Uint16(uint32_t uiIP, uint16_t *usIpH, uint16_t *usIpL)
{
	uint8_t *i = (uint8_t *)&uiIP;
	*usIpH = (i[0] << 8) + (i[1]);
	*usIpL = (i[2] << 8) + (i[3]);
}


void CFG_vComm1_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tCom1Config) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(ComConfigTPDF) >> 1;
	mPartitionTable.tCom1Config.tUartConfig.usCommPort = COMM_PORT_COM1;
	mPartitionTable.tCom1Config.tUartConfig.usBaudrateH = (CONFIG_DEFAULT_VALUE_UART_BAUDRATE >> 16) & 0x0000ffff;
	mPartitionTable.tCom1Config.tUartConfig.usBaudrateL = CONFIG_DEFAULT_VALUE_UART_BAUDRATE & 0x0000ffff;
	mPartitionTable.tCom1Config.tUartConfig.usWordSize = UART_DATA_8_BITS;
	mPartitionTable.tCom1Config.tUartConfig.usParity = UART_PARITY_DISABLE;
	mPartitionTable.tCom1Config.tUartConfig.usStopBits = UART_STOP_BITS_1;
	mPartitionTable.tCom1Config.usProtocolType = PROTOCOL_TYPE_MODBUS_RTU;
	mPartitionTable.tCom1Config.usJointPortType = JOINT_PORT_DOWN;
	mPartitionTable.tCom1Config.usTimeout = CONFIG_DEFAULT_VALUE_UART_TIMEOUT;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vComm2_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tCom2Config) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(ComConfigTPDF) >> 1;
	mPartitionTable.tCom2Config.tUartConfig.usCommPort = COMM_PORT_COM1;
	mPartitionTable.tCom2Config.tUartConfig.usBaudrateH = (CONFIG_DEFAULT_VALUE_UART_BAUDRATE >> 16) & 0x0000ffff;
	mPartitionTable.tCom2Config.tUartConfig.usBaudrateL = CONFIG_DEFAULT_VALUE_UART_BAUDRATE & 0x0000ffff;
	mPartitionTable.tCom2Config.tUartConfig.usWordSize = UART_DATA_8_BITS;
	mPartitionTable.tCom2Config.tUartConfig.usParity = UART_PARITY_DISABLE;
	mPartitionTable.tCom2Config.tUartConfig.usStopBits = UART_STOP_BITS_1;
	mPartitionTable.tCom2Config.usProtocolType = PROTOCOL_TYPE_MODBUS_RTU;
	mPartitionTable.tCom2Config.usJointPortType = JOINT_PORT_DOWN;
	mPartitionTable.tCom2Config.usTimeout = CONFIG_DEFAULT_VALUE_UART_TIMEOUT;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vCan_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tCanConfig) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(CanConfigTPDF) >> 1;
	mPartitionTable.tCanConfig.usBaudrateH = (CONFIG_DEFAULT_VALUE_CAN_BAUDRATE >> 16) & 0x0000ffff;
	mPartitionTable.tCanConfig.usBaudrateL = CONFIG_DEFAULT_VALUE_CAN_BAUDRATE & 0x0000ffff;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vEthernet_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tEthernetConfig) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(EthernetConfigTPDF) >> 1;
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_ETHERNET_IP,&mPartitionTable.tEthernetConfig.usIPH,&mPartitionTable.tEthernetConfig.usIPL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_ETHERNET_GATEWAY,&mPartitionTable.tEthernetConfig.usGateWayH,&mPartitionTable.tEthernetConfig.usGateWayL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_ETHERNET_SUBMASK,&mPartitionTable.tEthernetConfig.usSubMaskH,&mPartitionTable.tEthernetConfig.usSubMaskL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_ETHERNET_DNS,&mPartitionTable.tEthernetConfig.usDNSH,&mPartitionTable.tEthernetConfig.usDNSL);
	mPartitionTable.tEthernetConfig.EthernetCliSerMode = ETHERNET_MODE_SERVER;
	mPartitionTable.tEthernetConfig.EthernetPort = CONFIG_DEFAULT_VALUE_ETHERNET_PORT;
	mPartitionTable.tEthernetConfig.EthernetTimeout = CONFIG_DEFAULT_VALUE_ETHERNET_TIMEOUT;
	mPartitionTable.tEthernetConfig.EthernetIPType = ETHERNET_IP_TYPE_DHCP;
	mPartitionTable.tEthernetConfig.EthernetDnsType = ETHERNET_DNS_TYPE_STATIC;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vWifi_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tWifiConfig) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(WifiConfigTPDF) >> 1;
	mPartitionTable.tWifiConfig.usMode = WIFI_MODE_STA;
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_STA_IP,&mPartitionTable.tWifiConfig.usStaIPH,&mPartitionTable.tWifiConfig.usStaIPL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_STA_GATEWAY,&mPartitionTable.tWifiConfig.usStaGateWayH,&mPartitionTable.tWifiConfig.usStaGateWayL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_STA_SUBMASK,&mPartitionTable.tWifiConfig.usStaSubMaskH,&mPartitionTable.tWifiConfig.usStaSubMaskL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_STA_DNS,&mPartitionTable.tWifiConfig.usStaDNSH,&mPartitionTable.tWifiConfig.usStaDNSL);
	mPartitionTable.tWifiConfig.usStaDHCPMode = WIFI_DHCP_MODE_DYNAMIC;
	mPartitionTable.tWifiConfig.usStaDNSMode = WIFI_DNS_MODE_STATIC;
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_AP_IP,&mPartitionTable.tWifiConfig.usApIPH,&mPartitionTable.tWifiConfig.usApIPL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_AP_SUBMASK,&mPartitionTable.tWifiConfig.usApSubMaskH,&mPartitionTable.tWifiConfig.usApSubMaskL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_AP_IP_POOL_START,&mPartitionTable.tWifiConfig.usApIpPoolStartH,&mPartitionTable.tWifiConfig.usApIpPoolStartL);
	CFG_dConvert_To_Uint16(CONFIG_DEFAULT_VALUE_WIFI_AP_IP_POOL_END,&mPartitionTable.tWifiConfig.usApIpPoolEndH,&mPartitionTable.tWifiConfig.usApIpPoolEndL);
	mPartitionTable.tWifiConfig.usApDHCPMode = WIFI_DHCP_MODE_DYNAMIC;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vIot_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tIotConfig) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(IotConfigTPDF) >> 1;
	mPartitionTable.tIotConfig.usIotType = IOT_TYPE_4G;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vServer1_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tServer1Config) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(ServerConfigTPDF) >> 1;
	mPartitionTable.tServer1Config.usPort = 0;
	mPartitionTable.tServer1Config.usCommType = SERVER_COMM_TYPE_ETH;
	mPartitionTable.tServer1Config.usProtocolType = SERVER_PROTOCOL_TYPE_NONE;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vServer2_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tServer2Config) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(ServerConfigTPDF) >> 1;
	mPartitionTable.tServer2Config.usPort = 0;
	mPartitionTable.tServer2Config.usCommType = SERVER_COMM_TYPE_ETH;
	mPartitionTable.tServer2Config.usProtocolType = SERVER_PROTOCOL_TYPE_NONE;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vServer3_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tServer3Config) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(ServerConfigTPDF) >> 1;
	mPartitionTable.tServer3Config.usPort = 0;
	mPartitionTable.tServer3Config.usCommType = SERVER_COMM_TYPE_ETH;
	mPartitionTable.tServer3Config.usProtocolType = SERVER_PROTOCOL_TYPE_NONE;
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}


void CFG_vDevice_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.tDeviceConfig[0]) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = (sizeof(DeviceConfigTPDF) >> 1) * CONFIG_DEVICE_MAXSURPPORT;
	memset(&mPartitionTable.tDeviceConfig[0], 0, uiSize * 2);
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

void CFG_vReserve_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.ReserveConfig) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(ReserveConfigTPDF) >> 1;
	mPartitionTable.ReserveConfig.SeriesCode = CONFIG_DEFAULT_VALUE_SERIES_CODE;
	mPartitionTable.ReserveConfig.VerID = CONFIG_DEFAULT_VALUE_VER_ID;
	mPartitionTable.ReserveConfig.Version = CONFIG_DEFAULT_VALUE_VERSION;
	
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

void CFG_vBasicInfor_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.BasicInfor) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(BasicInforTPDF) >> 1;
	mPartitionTable.BasicInfor.FactoryID = 1;
	mPartitionTable.BasicInfor.ProductCategory = 7;
	mPartitionTable.BasicInfor.ProductSeries = 0x0601;
	mPartitionTable.BasicInfor.ProductSubfamily = 1;
	mPartitionTable.BasicInfor.RunState = 0;
	mPartitionTable.BasicInfor.SoftwareVersion = 1;
	mPartitionTable.BasicInfor.HardwareVersion = 1;
	memset(mPartitionTable.BasicInfor.ProductSN, 0, sizeof(mPartitionTable.BasicInfor.ProductSN));
	memset(mPartitionTable.BasicInfor.ProductDescription, 0, sizeof(mPartitionTable.BasicInfor.ProductDescription));
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

void CFG_vGatewayInfor_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.GatewayInfor) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(GatewayInfoTPDF) >> 1;
	mPartitionTable.GatewayInfor.ModbusAddr = 0x03;
	memset(mPartitionTable.GatewayInfor.AssetCode, 0, sizeof(mPartitionTable.GatewayInfor.AssetCode));
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

void CFG_vDeviceAssetCode_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.DeviceAssetCode) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(DeviceAssetCodeTPDF) >> 1;
	memset(mPartitionTable.DeviceAssetCode, 0, sizeof(mPartitionTable.DeviceAssetCode));
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

void CFG_vPlatform_DefaultConfig(uint8_t ucWriteflash)
{
	uint32_t uiIndex = (((char *)&mPartitionTable.PlatformConfig) - ((char *)&mPartitionTable)) >> 1;
	uint32_t uiSize = sizeof(PlatformConfigTPDF) >> 1;
	mPartitionTable.PlatformConfig.CommMode = 0x0002;
	mPartitionTable.PlatformConfig.ProtocolType = SERVER_PROTOCOL_TYPE_NONE;
	mPartitionTable.PlatformConfig.CommPort = 0;
	mPartitionTable.PlatformConfig.MQTTQos = 0x0000;
	mPartitionTable.PlatformConfig.TransportMode = 0x0001;
	mPartitionTable.PlatformConfig.LWTRetain = 0x0000;
	mPartitionTable.PlatformConfig.LWTQos = 0x0000;
	mPartitionTable.PlatformConfig.KeepaliveTime = 300;
	memset(mPartitionTable.PlatformConfig.IPConfig, 0, sizeof(mPartitionTable.PlatformConfig.IPConfig));
	memset(mPartitionTable.PlatformConfig.ClientIDConfig, 0, sizeof(mPartitionTable.PlatformConfig.ClientIDConfig));
	memset(mPartitionTable.PlatformConfig.UserNameConfig, 0, sizeof(mPartitionTable.PlatformConfig.UserNameConfig));
	memset(mPartitionTable.PlatformConfig.PasswordConfig, 0, sizeof(mPartitionTable.PlatformConfig.PasswordConfig));
	memset(mPartitionTable.PlatformConfig.LWTTopicConfig, 0, sizeof(mPartitionTable.PlatformConfig.LWTTopicConfig));
	memset(mPartitionTable.PlatformConfig.LWTMessageConfig, 0, sizeof(mPartitionTable.PlatformConfig.LWTMessageConfig));
	memset(mPartitionTable.PlatformConfig.SubscribeTopicConfig, 0, sizeof(mPartitionTable.PlatformConfig.SubscribeTopicConfig));
	memset(mPartitionTable.PlatformConfig.PublishTopicConfig, 0, sizeof(mPartitionTable.PlatformConfig.PublishTopicConfig));
	if(!ucWriteflash)
	{
		return;
	}
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

/* set every value default configure with every hardware to use if no configure */
void CFG_vDefaultConfig(void)
{
	CFG_vComm1_DefaultConfig(0);
	CFG_vComm2_DefaultConfig(0);
	CFG_vCan_DefaultConfig(0);
	CFG_vEthernet_DefaultConfig(0);
	CFG_vWifi_DefaultConfig(0);
	CFG_vIot_DefaultConfig(0);
	CFG_vServer1_DefaultConfig(0);
	CFG_vServer2_DefaultConfig(0);
	CFG_vServer3_DefaultConfig(0);
	mPartitionTable.usGatewayRunMode = 0x1555;
	CFG_vDevice_DefaultConfig(0);
	CFG_vReserve_DefaultConfig(0);
	CFG_vBasicInfor_DefaultConfig(0);
	CFG_vGatewayInfor_DefaultConfig(0);
	CFG_vDeviceAssetCode_DefaultConfig(0);
	CFG_vPlatform_DefaultConfig(0);
}


void CFG_vSaveConfig(uint16_t usIndex)
{
	ESP_LOGI(TAG, "CFG_vSaveConfig");
	uint16_t *usData = ((uint16_t *)&mPartitionTable) + usIndex;
	esp_err_t err = nvs_open(mCfgNusNamespace, NVS_READWRITE, &mCfgNusHandle);
	if(err == ESP_OK)
	{
		err = nvs_set_u16(mCfgNusHandle, CFG_dGet_Partition_Desc(usIndex), *usData);
		if(err == ESP_OK)
		{
			ESP_LOGI(TAG, "save config ok %s", CFG_dGet_Partition_Desc(usIndex));
		}
		else
		{
			ESP_LOGE(TAG, "save config fail %s", esp_err_to_name(err));
		}
		nvs_commit(mCfgNusHandle);
	}
	else
	{
		ESP_LOGE(TAG, "save config fail");
	}
	nvs_close(mCfgNusHandle);
}


/* analysis data and save nvs flash */
void CFG_dProcess_Protocol(ModbusRtuDataTPDF *tRec, ModbusRtuDataTPDF *tSend)
{
	static uint8_t ucProtect = 0;
	if(ucProtect)
	{
		return;
	}
	ucProtect = 1;
	if(tRec->ucEffect != MODBUS_RESULT_SUCCESS)
	{
		ucProtect = 0;
		return;
	}
	if(!tRec->usRegAddr)
	{
		ucProtect = 0;
		return;
	}
	tRec->ucEffect = MODBUS_RESULT_FAIL;
	tSend->ucEffect = MODBUS_RESULT_FAIL;
	tSend->tFunction = tRec->tFunction;
	tSend->ucSlaveAddr = tRec->ucSlaveAddr;
	tSend->usRegAddr = tRec->usRegAddr;
	tSend->usRegCount = tRec->usRegCount;
	tSend->ucEffect = MODBUS_RESULT_FAIL;
	tSend->ucError = 0;
	switch(tRec->tFunction)
	{
		case ReadReg:
		case ReadInputReg:
			if(tRec->usRegAddr + tRec->usRegCount <= 0x03CF)
			{
				tSend->usDataLen = tRec->usRegCount * 2;
				uint8_t *j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
				for(uint8_t i = 0; i < tRec->usRegCount; i++)
				{
					tSend->ucData[i * 2 + 1] = *(j++);
					tSend->ucData[i * 2] = *(j++);
				}
				tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			}
			else if(tRec->usRegAddr >= 0x8004 && (tRec->usRegAddr + tRec->usRegCount) <= 0x8007)
			{
				tSend->usDataLen = tRec->usRegCount * 2;
				uint8_t *j = ((uint8_t *)&mPartitionTable.ReserveConfig.SeriesCode) + ((tRec->usRegAddr - 0x8004) * 2);
				for(uint8_t i = 0; i < tRec->usRegCount; i++)
				{
					tSend->ucData[i * 2 + 1] = *(j++);
					tSend->ucData[i * 2] = *(j++);
				}
				tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			}
			else
			{
				tSend->ucError = MODBUS_ERROR_CODE_REGADDR;
				tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			}
			break;
		case WriteReg:
			switch(tRec->usRegAddr)
			{
				// Restore default configuration.
				case 0x00BA:
					switch((tRec->ucData[0] << 8) + tRec->ucData[1])
					{
						case 1:
							CFG_vComm1_DefaultConfig(1);
							break;
						case 2:
							CFG_vComm2_DefaultConfig(1);
							break;
						case 4:
							CFG_vCan_DefaultConfig(1);
							break;
						case 8:
							CFG_vEthernet_DefaultConfig(1);
							break;
						case 0x10:
							CFG_vWifi_DefaultConfig(1);
							break;
						case 0x20:
							CFG_vIot_DefaultConfig(1);
							break;
						case 0x40:
							CFG_vPlatform_DefaultConfig(1);
							break;
						case 0x3f:
							CFG_vComm1_DefaultConfig(1);
							CFG_vComm2_DefaultConfig(1);
							CFG_vCan_DefaultConfig(1);
							CFG_vEthernet_DefaultConfig(1);
							CFG_vWifi_DefaultConfig(1);
							CFG_vIot_DefaultConfig(1);
							CFG_vPlatform_DefaultConfig(1);
							break;
					}
					tSend->ucData[0] = tRec->ucData[0];
					tSend->ucData[1] = tRec->ucData[1];
					tSend->ucEffect = MODBUS_RESULT_SUCCESS;
					break;
				case 0x00BB:
					fflush(stdout);
					esp_restart();
					break;
				default:
					if(tRec->usRegAddr < 0x00BD)
					{
						tSend->usDataLen = 2;
						uint8_t *j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
						*(j++) = tRec->ucData[1];
						*(j++) = tRec->ucData[0];
						CFG_vSaveConfig((tRec->usRegAddr - 1));
						tSend->ucData[0] = tRec->ucData[0];
						tSend->ucData[1] = tRec->ucData[1];
						tSend->ucEffect = MODBUS_RESULT_SUCCESS;
					}
					else if(tRec->usRegAddr < 0x01FD)
					{
						uint16_t i = tRec->usRegAddr - 192;
						if((i % 10) == 0 && tRec->ucData[0] == 0 && tRec->ucData[1] == 0)
						{
							i /= 10;
							vCfg_Device_Delete(i);
						}
						else
						{
							uint8_t *j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
							*(j++) = tRec->ucData[1];
							*(j++) = tRec->ucData[0];
							CFG_vSaveConfig((tRec->usRegAddr - 1));
						}
						tSend->usDataLen = 2;
						tSend->ucData[0] = tRec->ucData[0];
						tSend->ucData[1] = tRec->ucData[1];
						tSend->ucEffect = MODBUS_RESULT_SUCCESS;
					}
					else if(tRec->usRegAddr <= 0x03CF)
					{
						tSend->usDataLen = 2;
						uint8_t *j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
						*(j++) = tRec->ucData[1];
						*(j++) = tRec->ucData[0];
						CFG_vSaveConfig((tRec->usRegAddr - 1));
						tSend->ucData[0] = tRec->ucData[0];
						tSend->ucData[1] = tRec->ucData[1];
						tSend->ucEffect = MODBUS_RESULT_SUCCESS;
					}
					else
					{
						tSend->ucError = MODBUS_ERROR_CODE_REGADDR;
						tSend->ucEffect = MODBUS_RESULT_SUCCESS;
					}
					break;
			}
			break;
		case WriteMultReg:
			if(tRec->usRegAddr + tRec->usRegCount <= 0x03CF)
			{
				uint8_t *j = NULL;
				ESP_LOGI(TAG, "tRec->usRegAddr:%x", tRec->usRegAddr);
				switch (tRec->usRegAddr)
				{
				case WIFI_STA_SSID:
				case WIFI_STA_PASSWORD:
				case WIFI_AP_SSID:
				case WIFI_AP_PASSWORD:
				case GATEWAY_ASSERT_CODE:
				case DEVICE01_ASSERT_CODE:
				case DEVICE02_ASSERT_CODE:
				case DEVICE03_ASSERT_CODE:
				case DEVICE04_ASSERT_CODE:
				case DEVICE05_ASSERT_CODE:
				case DEVICE06_ASSERT_CODE:
				case DEVICE07_ASSERT_CODE:
				case DEVICE08_ASSERT_CODE:
				case DEVICE09_ASSERT_CODE:
				case DEVICE10_ASSERT_CODE:
				case DEVICE11_ASSERT_CODE:
				case DEVICE12_ASSERT_CODE:
				case DEVICE13_ASSERT_CODE:
				case DEVICE14_ASSERT_CODE:
				case DEVICE15_ASSERT_CODE:
				case DEVICE16_ASSERT_CODE:
				case DEVICE17_ASSERT_CODE:
				case DEVICE18_ASSERT_CODE:
				case DEVICE19_ASSERT_CODE:
				case DEVICE20_ASSERT_CODE:
				case DEVICE21_ASSERT_CODE:
				case DEVICE22_ASSERT_CODE:
				case DEVICE23_ASSERT_CODE:
				case DEVICE24_ASSERT_CODE:
				case DEVICE25_ASSERT_CODE:
				case DEVICE26_ASSERT_CODE:
				case DEVICE27_ASSERT_CODE:
				case DEVICE28_ASSERT_CODE:
				case DEVICE29_ASSERT_CODE:
				case DEVICE30_ASSERT_CODE:
				case DEVICE31_ASSERT_CODE:
				case DEVICE32_ASSERT_CODE:
				case PLATFORM_LWT_TOPIC:
				case PLATFORM_LWT_MSG:
					j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
					for(uint8_t i = 0; i < 8; i++)
					{
						*(j++) = 0x00;
						*(j++) = 0x00;
						CFG_vSaveConfig((tRec->usRegAddr - 1 + i));
					}
					break;
				case SN_CODE:
				case PRODUCT_DESCRIPTION:
					j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
					for(uint8_t i = 0; i < 12; i++)
					{
						*(j++) = 0x00;
						*(j++) = 0x00;
						CFG_vSaveConfig((tRec->usRegAddr - 1 + i));
					}
					break;
				case SERVER1_IP:
				case SERVER2_IP:
				case SERVER3_IP:
				case PLATFORM_IP:
				case PLATFORM_CLIENT_ID:
				case PLATFORM_USERNAME:
				case PLATFORM_PASSWORD:
					j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
					for(uint8_t i = 0; i < 16; i++)
					{
						*(j++) = 0x00;
						*(j++) = 0x00;
						CFG_vSaveConfig((tRec->usRegAddr - 1 + i));
					}
					break;
				case PLATFORM_SUB_TOPIC:
				case PLATFORM_PUB_TOPIC:
					j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
					for(uint8_t i = 0; i < 32; i++)
					{
						*(j++) = 0x00;
						*(j++) = 0x00;
						CFG_vSaveConfig((tRec->usRegAddr - 1 + i));
					}
					break;
				default:
					break;
				}
				j = ((uint8_t *)&mPartitionTable) + ((tRec->usRegAddr - 1) * 2);
				for(uint8_t i = 0; i < tRec->usRegCount; i++)
				{
					*(j++) = tRec->ucData[i * 2 + 1];
					*(j++) = tRec->ucData[i * 2];
					CFG_vSaveConfig((tRec->usRegAddr - 1 + i));
				}
				tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			}
			else
			{
				tSend->ucError = MODBUS_ERROR_CODE_REGADDR;
				tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			}
			break;
		case ReportSlaveID:
			tSend->usDataLen = 9 + 12 * 2 + 12 * 2;
			tSend->ucData[0] = mPartitionTable.BasicInfor.FactoryID & 0x00ff;
			tSend->ucData[1] = mPartitionTable.BasicInfor.ProductCategory & 0x00ff;
			tSend->ucData[2] = mPartitionTable.BasicInfor.ProductSeries & 0x00ff;
			tSend->ucData[3] = mPartitionTable.BasicInfor.ProductSubfamily & 0x00ff;
			tSend->ucData[4] = mPartitionTable.BasicInfor.RunState & 0x00ff;
			tSend->ucData[5] = (mPartitionTable.BasicInfor.SoftwareVersion >> 8) & 0x00ff;
			tSend->ucData[6] = mPartitionTable.BasicInfor.SoftwareVersion & 0x00ff;
			tSend->ucData[7] = (mPartitionTable.BasicInfor.HardwareVersion >> 8) & 0x00ff;
			tSend->ucData[8] = mPartitionTable.BasicInfor.HardwareVersion & 0x00ff;
			for(int i = 0; i < 12; i++)
			{
				tSend->ucData[9 + 2 * i] = (mPartitionTable.BasicInfor.ProductSN[i] >> 8) & 0x00ff;
				tSend->ucData[9 + 2 * i + 1] = mPartitionTable.BasicInfor.ProductSN[i] & 0x00ff;
			}
			for(int i = 0; i < 12; i++)
			{
				tSend->ucData[34 + 2 * i] = (mPartitionTable.BasicInfor.ProductDescription[i] >> 8) & 0x00ff;
				tSend->ucData[34 + 2 * i + 1] = mPartitionTable.BasicInfor.ProductDescription[i] & 0x00ff;
			}
			tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			break;
		default:
			tSend->ucError = MODBUS_ERROR_CODE_FUNCTION;
			tSend->ucEffect = MODBUS_RESULT_SUCCESS;
			break;
	}
	ucProtect = 0;
}

void CFG_vInit(void)
{
	esp_err_t tRet;
    uint16_t usLen = sizeof(mPartitionTable) >> 1;
    uint16_t *usData = (uint16_t *)&mPartitionTable;
    tRet = nvs_flash_init();
    if (tRet == ESP_ERR_NVS_NO_FREE_PAGES || tRet == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
    	ESP_ERROR_CHECK(nvs_flash_erase());
    	tRet = nvs_flash_init();
    }
    tRet = nvs_open(mCfgNusNamespace, NVS_READWRITE, &mCfgNusHandle);
    if (tRet == ESP_OK)
    {
        ESP_LOGI(TAG, "open Done");
        /* get every value for given key in partition table */
        for(uint16_t i = 0; i < usLen; i++)
        {
        	tRet = nvs_get_u16(mCfgNusHandle, CFG_dGet_Partition_Desc(i), usData);
            usData++;
            if(tRet != ESP_OK)
            {
            	ESP_LOGE(TAG, "read fail %d", i);
            	break;
            }
        }
        if(tRet == ESP_OK)
        {
        	nvs_close(mCfgNusHandle);
        	/* copy device info to mDevice structures from partition table */
        	for(uint8_t i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
        	{
        		memcpy(&mDevice[i].tDeviceConfig, &mPartitionTable.tDeviceConfig[i], sizeof(DeviceConfigTPDF));
        	}
        	return;
        }
    }
    else
    {
    	ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(tRet));
    }
    CFG_vDefaultConfig();
    for(uint8_t i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
    {
    	memset(&mDevice[i].tDeviceConfig, 0, sizeof(DeviceConfigTPDF));
    }
    usData = (uint16_t *)&mPartitionTable;
    /* set configure to nvs flash */
    for(uint16_t i = 0; i < usLen; i++)
    {
    	tRet = nvs_set_u16(mCfgNusHandle, CFG_dGet_Partition_Desc(i), *usData);
    		usData++;
    	if(tRet != ESP_OK)
    	{
    		ESP_LOGE(TAG, "write fail %s", esp_err_to_name(tRet));
    		nvs_flash_erase();
    		return;
    	}
    }
    nvs_commit(mCfgNusHandle);
    nvs_close(mCfgNusHandle);
}
