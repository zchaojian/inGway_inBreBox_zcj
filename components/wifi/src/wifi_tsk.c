/*
 * wifi_tsk.c
 *
 *  Created on: Nov 1, 2020
 *      Author: zchaojian
 */
#include "wifi_tsk.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define ESPTOUCH_DONE_BIT  BIT2

QueueHandle_t mWifiQueueSend;
QueueHandle_t mWifiQueueRec;
static ServerProtocolTypeTPDF mServerProtocolType;
uint8_t mWifiConnected = 0;

static const char *SC_TAG = "wifi smart config";
static EventGroupHandle_t s_wifi_event_group;

static const char *TAG = "wifi task";
/* convert modbus data  */
static void vConvert_MB_HiLo_Byte(char *cDataIn, uint8_t uiLen)
{
	char temp;
	if(uiLen % 2 == 1)
	{
		uiLen += 1;
	}
	for(int i = 0; i < uiLen; i += 2)
	{
		temp = cDataIn[i];
		cDataIn[i] = cDataIn[i + 1];
		cDataIn[i + 1] = temp;
	}
}

static void vWifi_SmartConfig_Event_Handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
    	ESP_LOGI(SC_TAG, "smart configure task start");
    	ESP_ERROR_CHECK( esp_smartconfig_set_type(SC_TYPE_ESPTOUCH) );
    	smartconfig_start_config_t cfg = SMARTCONFIG_START_CONFIG_DEFAULT();
    	ESP_ERROR_CHECK( esp_smartconfig_start(&cfg) );
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(SC_TAG, "WiFi Connected to ap");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SCAN_DONE)
    {
        ESP_LOGI(SC_TAG, "Scan done");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_FOUND_CHANNEL)
    {
        ESP_LOGI(SC_TAG, "Found channel");
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_GOT_SSID_PSWD)
    {
        ESP_LOGI(SC_TAG, "Got SSID and password");

        smartconfig_event_got_ssid_pswd_t *evt = (smartconfig_event_got_ssid_pswd_t *)event_data;
        wifi_config_t wifi_config;
        char cSSID[16];
        char cPWD[16];
        int iSSIDLen;
        int iPWDLen;

        memset(cSSID, 0, sizeof(cSSID));
        memset(cPWD, 0, sizeof(cPWD));


        bzero(&wifi_config, sizeof(wifi_config_t));
        memcpy(wifi_config.sta.ssid, evt->ssid, sizeof(wifi_config.sta.ssid));
        memcpy(wifi_config.sta.password, evt->password, sizeof(wifi_config.sta.password));
        wifi_config.sta.bssid_set = evt->bssid_set;
        if (wifi_config.sta.bssid_set == true) {
            memcpy(wifi_config.sta.bssid, evt->bssid, sizeof(wifi_config.sta.bssid));
        }
        /*ESP_LOGW(TAG, "size:wifi_ssid len: %d", sizeof(wifi_config.sta.ssid));
        ESP_LOGW(TAG, "stlen:wifi_ssid len: %d", strlen((char*)wifi_config.sta.ssid));
        ESP_LOGW(TAG, "size:evt_ssid len: %d", sizeof(evt->ssid));
        ESP_LOGW(TAG, "stlen:evt_ssid len: %d", strlen((char*)evt->ssid));

        ESP_LOGW(TAG, "size:wifi_pwd len: %d", sizeof(wifi_config.sta.password));
        ESP_LOGW(TAG, "stlen:wifi_pwd len: %d", strlen((char*)wifi_config.sta.password));
        ESP_LOGW(TAG, "size:evt_pwd len: %d", sizeof(evt->password));
        ESP_LOGW(TAG, "stlen:evt_pwd len: %d", strlen((char*)evt->password));*/


        iSSIDLen = strlen((char*)evt->ssid);
        iPWDLen = strlen((char*)evt->password);
        memcpy(cSSID, evt->ssid, iSSIDLen);
        memcpy(cPWD, evt->password, iPWDLen);
        //esp_log_buffer_hex(TAG,cSSID, sizeof(cSSID));
        //esp_log_buffer_hex(TAG,cPWD, sizeof(cPWD));

        vConvert_MB_HiLo_Byte(cSSID, iSSIDLen);
        vConvert_MB_HiLo_Byte(cPWD, iPWDLen);

        memcpy(mPartitionTable.tWifiConfig.cStaSSID, cSSID, sizeof(mPartitionTable.tWifiConfig.cStaSSID));
        memcpy(mPartitionTable.tWifiConfig.cStaPWD, cPWD, sizeof(mPartitionTable.tWifiConfig.cStaPWD));

        /* write will linked sta ssid to nvs flash */
        uint32_t uiIndex = PARTITION_TABLE_ADDR_WIFI_STASSID1_ADDR - 1;//STA SSID start address: 0x003B - 1
        uint32_t uiSize = sizeof(mPartitionTable.tWifiConfig.cStaSSID) >> 1;//16 chars write to 8 registers.
        while(uiSize)
        {
        	CFG_vSaveConfig(uiIndex++);
        	uiSize--;
        }

        /* write will linked sta pwd to nvs flash */
        uiIndex = PARTITION_TABLE_ADDR_WIFI_STAPASSWD1_ADDR - 1;//STA PWD start address:0x0043 - 1
        uiSize = sizeof(mPartitionTable.tWifiConfig.cStaPWD) >> 1;//16 chars write to 8 registers.
        while(uiSize)
        {
        	CFG_vSaveConfig(uiIndex++);
        	uiSize--;
        }

        ESP_LOGI(SC_TAG, "SSID:%s", mPartitionTable.tWifiConfig.cStaSSID);
        ESP_LOGI(SC_TAG, "PASSWORD:%s", mPartitionTable.tWifiConfig.cStaPWD);

        ESP_ERROR_CHECK( esp_wifi_disconnect() );
        ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
        ESP_ERROR_CHECK( esp_wifi_connect() );
    }
    else if (event_base == SC_EVENT && event_id == SC_EVENT_SEND_ACK_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, ESPTOUCH_DONE_BIT);
    }
}

void WIFI_vInit_SmartConfig(void)
{
	EventBits_t uxBits;
    //s_wifi_event_group = xEventGroupCreate();

    /*ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());*/
    /*esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);*/

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );

    ESP_ERROR_CHECK( esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &vWifi_SmartConfig_Event_Handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &vWifi_SmartConfig_Event_Handler, NULL) );
    ESP_ERROR_CHECK( esp_event_handler_register(SC_EVENT, ESP_EVENT_ANY_ID, &vWifi_SmartConfig_Event_Handler, NULL) );

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );

    /* wait smart config finish */
    uxBits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT, true, true, portMAX_DELAY);
    if(uxBits & (WIFI_CONNECTED_BIT | ESPTOUCH_DONE_BIT))
    {
        ESP_LOGI(SC_TAG, "smartconfig over");
        esp_smartconfig_stop();

        /* set smart configure enable status to disenable */
        mPartitionTable.ReserveConfig.SmartConfigEnable = WIFI_SC_DISENABLE;
        CFG_vSaveConfig(0x200 - 1);
        esp_restart();
    }

}

void vWifi_Base_Init()
{
	s_wifi_event_group = xEventGroupCreate();
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
	assert(sta_netif);
}

void vSmartConfig_vInit()
{
	/* esp_touch function */
	if(mPartitionTable.ReserveConfig.SmartConfigEnable == WIFI_SC_ENABLE)
	{
		WIFI_vInit_SmartConfig();
	}
	ESP_LOGI(TAG, "SmartConfigEnable:%d", mPartitionTable.ReserveConfig.SmartConfigEnable);
}


/* Set dhcp acquired IP information to mPartitionTable.tWifiConfig and save to nvs flash */
static void vWifi_Sta_Dhcp_GotIP_Save_Flash(esp_netif_ip_info_t *ip_info)
{
	uint32_t uiIndex;
	uint32_t uiSize ;
	if(mPartitionTable.tWifiConfig.usStaDHCPMode == WIFI_DHCP_MODE_DYNAMIC)
	{
		mPartitionTable.tWifiConfig.usStaIPH = ip4_addr1_16(&(ip_info->ip)) << 8U | (ip4_addr2_16(&(ip_info->ip)) & 0xFF);
		mPartitionTable.tWifiConfig.usStaIPL = ip4_addr3_16(&(ip_info->ip)) << 8U | (ip4_addr4_16(&(ip_info->ip)) & 0xFF);
		mPartitionTable.tWifiConfig.usStaGateWayH = ip4_addr1_16(&(ip_info->gw)) << 8U | (ip4_addr2_16(&(ip_info->gw)) & 0xFF);
		mPartitionTable.tWifiConfig.usStaGateWayL = ip4_addr3_16(&(ip_info->gw)) << 8U | (ip4_addr4_16(&(ip_info->gw)) & 0xFF);
		mPartitionTable.tWifiConfig.usStaSubMaskH = ip4_addr1_16(&(ip_info->netmask)) << 8U | (ip4_addr2_16(&(ip_info->netmask)) & 0xFF);
		mPartitionTable.tWifiConfig.usStaSubMaskL = ip4_addr3_16(&(ip_info->netmask)) << 8U | (ip4_addr4_16(&(ip_info->netmask)) & 0xFF);

		/* write acquired IP, GateWay, net mask address infor to nvs flash */
		uiIndex = PARTITION_TABLE_ADDR_WIFI_STAIP_HADDR - 1;//WIFI IP start address: 0x0021 - 1
		uiSize = 6;//IPH, IPL, GateWayH, GateWayL, SubMaskH, SubMaskl, write to 6 registers.
		while(uiSize)
		{
			CFG_vSaveConfig(uiIndex++);
			uiSize--;
		}
	}
}

static void vWifi_Sntp_CallBack(struct tm *tResult)
{
	MyQueueDataTPDF tMyQueueData;
	tMyQueueData.tMyQueueCommand.tMyQueueCommandType = Sntp;
	xQueueSend(mWifiQueueRec, (void * )&tMyQueueData, (TickType_t)0);
	ESP_LOGI(TAG, "SNTP:year:%d,month:%d,day:%d,hour:%d,min:%d,sec:%d\n", tResult->tm_year + 1900,
																   tResult->tm_mon + 1,
																   tResult->tm_mday,
																   tResult->tm_hour,
																   tResult->tm_min,
																   tResult->tm_sec);
}

/* copy wifi connected status from WIFI_vRegister_Callback() */
void vWifi_Event_CallBack(WIFIEventTPDF *tEvent)
{
	static uint8_t ucSntpDone = 0;
	switch(tEvent->tType)
	{
		case WIFI_IP_GOT:
			mWifiConnected = 1;
			vWifi_Sta_Dhcp_GotIP_Save_Flash(&(((ip_event_got_ip_t*)tEvent->tTag)->ip_info));
			if(!ucSntpDone)
			{
				SNTP_vInit(vWifi_Sntp_CallBack);
				ucSntpDone = 1;
			}
			break;
		case WIFI_STA_DISCONNECTED:
			mWifiConnected = 0;
			Wifi_vSocket_Close();
			/*mPartitionTable.ReserveConfig.SmartConfigEnable = WIFI_SC_ENABLE;
			CFG_vSaveConfig(PARTITION_TABLE_ADDR_GATEWAY_SMART_CONFIG_ENABLE - 1);
			esp_restart();*/
			break;
		default:
			break;
	}
}

/* Use function return wifi Network link status */
uint8_t Wifi_Connected_Status(void)
{
	return mWifiConnected;
}

static void vWifi_vInit()
{
	mServerProtocolType = (ServerProtocolTypeTPDF)0;
	uint32_t uiModbusIP = 0;
	uint16_t usModbusPort = 0;
	WIFI_vRegister_Callback(vWifi_Event_CallBack);
	if(mPartitionTable.tServer1Config.usCommType == SERVER_COMM_TYPE_WIFI)
	{
		uiModbusIP = CFG_dConvertString_To_IP(mPartitionTable.tServer1Config.cIP);
		usModbusPort = mPartitionTable.tServer1Config.usPort;
		mServerProtocolType = (ServerProtocolTypeTPDF)mPartitionTable.tServer1Config.usProtocolType;
	}
	else if(mPartitionTable.tServer2Config.usCommType == SERVER_COMM_TYPE_WIFI)
	{
		uiModbusIP = CFG_dConvertString_To_IP(mPartitionTable.tServer2Config.cIP);
		usModbusPort = mPartitionTable.tServer2Config.usPort;
		mServerProtocolType = (ServerProtocolTypeTPDF)mPartitionTable.tServer2Config.usProtocolType;
	}
	else if(mPartitionTable.tServer3Config.usCommType == SERVER_COMM_TYPE_WIFI)
	{
		uiModbusIP = CFG_dConvertString_To_IP(mPartitionTable.tServer3Config.cIP);
		usModbusPort = mPartitionTable.tServer3Config.usPort;
		mServerProtocolType = (ServerProtocolTypeTPDF)mPartitionTable.tServer3Config.usProtocolType;
	}
	ESP_LOGI(TAG, "Eth Init:IP = %x,Port = %d",uiModbusIP,usModbusPort);

	/* WIFI AP and STA function initialization */
	if(mPartitionTable.tWifiConfig.usMode == WIFI_MODE_AP)
	{
		char *ssid = "chint-esp32";
		char *pwd = "12345678";
		memcpy(mPartitionTable.tWifiConfig.cApSSID, ssid, strlen(ssid));
		memcpy(mPartitionTable.tWifiConfig.cApPWD, pwd, strlen(pwd));
		mPartitionTable.tWifiConfig.cApSSID[strlen(ssid)] = 0;
		mPartitionTable.tWifiConfig.cApPWD[strlen(pwd)] = 0;

		WIFI_vInit_softap(&mPartitionTable.tWifiConfig);
	}
	else if (mPartitionTable.tWifiConfig.usMode == WIFI_MODE_STA)
	{
		WIFI_vInit_sta(&mPartitionTable.tWifiConfig);
	}
	else if (mPartitionTable.tWifiConfig.usMode == WIFI_MODE_APSTA)
	{
		ESP_LOGI(TAG, "wifi ap/sta mode not support");
	}
	else
	{
		ESP_LOGE(TAG, "WIFI mode ERROR!");
		return;
	}

	Wifi_Socket_vInit();
	ESP_LOGE(TAG, "mServerProtocolType:%d", mServerProtocolType);
	switch(mServerProtocolType)
	{
		case SERVER_PROTOCOL_TYPE_MQTT:
			Wifi_Mqtt_vInit();
			break;
		case SERVER_PROTOCOL_TYPE_MODBUS_RTU:
			Wifi_SocketClient_vInit(uiModbusIP, usModbusPort, AF_INET);
			break;
		default:
			break;
	}

}

void Wifi_vTsk_Start(void *pvParameters)
{
	MyQueueDataTPDF tMyQueueData;

	vWifi_Base_Init();
	vSmartConfig_vInit();
	vWifi_vInit();
	ESP_LOGW(TAG, "mWifiConnected: %d", mWifiConnected);
	while(1)
	{
		Wifi_Socket_vTsk(mWifiConnected, tMyQueueData);
		vTaskDelay(1);
		switch(mServerProtocolType)
		{
			case SERVER_PROTOCOL_TYPE_MQTT:
				Wifi_Mqtt_vTsk(mWifiConnected, tMyQueueData);
				break;
			case SERVER_PROTOCOL_TYPE_MODBUS_RTU:
				Wifi_SocketClient_vTsk(mWifiConnected, tMyQueueData);
				break;
			default:
				break;
		}
	}
}

