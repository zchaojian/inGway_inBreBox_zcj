/*
 * wifi_svc.c
 *
 *  Created on: Nov 1, 2020
 *      Author: zchaojian
 */

#include "wifi_svc.h"

//ap
#define EXAMPLE_MAX_STA_CONN 		4
#define EXAMPLE_ESP_WIFI_CHANNEL 	1

//sta
#define EXAMPLE_ESP_MAXIMUM_RETRY	5
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define ESPTOUCH_DONE_BIT  BIT2
static int iServerRetryNum = 0;

static uint8_t mIpGot = 0;

static const char *TAG = "wifi service";
static const char *AP_TAG = "wifi ap";
static const char *STA_TAG = "wifi sta";
//static const char *SC_TAG = "wifi smart config";

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

typedef struct
{
	WIFI_vEvent_CallBack tCallback;
	WIFIEventTPDF tEvent;
}WifiProcessTPDF;
WifiProcessTPDF mWifiProcess = {0 };

static uint32_t Wifi_dConvert_To_IP(uint16_t usIpH, uint16_t usIpL)
{
	uint8_t i[4];
	i[0] = (usIpH >> 8) & 0x00ff;
	i[1] = usIpH & 0x00ff;
	i[2] = (usIpL >> 8) & 0x00ff;
	i[3] = usIpL & 0x00ff;
	return(i[0] + (i[1] <<8) + (i[2] << 16) + (i[3] << 24));
}

/* convert modbus data  */
static void vConvert_MB_HiLo_Byte(uint8_t *cDataIn, uint8_t uiLen)
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

static void vWifi_AP_Event_Handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(AP_TAG, "station "MACSTR" join, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
    else if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(AP_TAG, "station "MACSTR" leave, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

void WIFI_vInit_softap(WifiConfigTPDF *tCfg)
{
	/*ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());*/
	//esp_netif_create_default_wifi_ap();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
														ESP_EVENT_ANY_ID,
														&vWifi_AP_Event_Handler,
														NULL,
														NULL));
	wifi_config_t wifi_config = {
			.ap = {
			/*.ssid = {*tCfg->cApSSID},
			.ssid_len = strlen(tCfg->cApSSID),*/
			.channel = EXAMPLE_ESP_WIFI_CHANNEL,
			//.password = DEFINE_PWD,
			.max_connection = EXAMPLE_MAX_STA_CONN,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK
		},
	};
	/* set ssid and pwd from partition table */
	wifi_config.ap.ssid_len = strlen(tCfg->cApSSID);
	memcpy(wifi_config.ap.ssid, tCfg->cApSSID, wifi_config.ap.ssid_len);
	memcpy(wifi_config.ap.password, tCfg->cApPWD, strlen(tCfg->cApPWD));

	if (strlen(tCfg->cApPWD) == 0) {
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(AP_TAG, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
			tCfg->cApSSID, tCfg->cApPWD, EXAMPLE_ESP_WIFI_CHANNEL);
}

static void vWifi_Callback(void *tTag, WIFIEventTypeTPDF tType, char *cData, uint16_t usLen)
{
	if(mWifiProcess.tCallback)
	{
		mWifiProcess.tEvent.tType = tType;
		mWifiProcess.tEvent.tTag = tTag;
		mWifiProcess.tEvent.cData = cData;
		mWifiProcess.tEvent.usLen = usLen;
		mWifiProcess.tCallback(&mWifiProcess.tEvent);
	}
}

static void vWifi_STA_Event_Handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
        vWifi_Callback(event_data, WIFI_STA_START, NULL, 0);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (iServerRetryNum < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            iServerRetryNum++;
            ESP_LOGE(STA_TAG, "retry to connect to the AP");
            vTaskDelay(500);
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGE(STA_TAG,"connect to the AP fail");
        mIpGot = 0;
        vWifi_Callback(event_data, WIFI_STA_DISCONNECTED, NULL, 0);
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(STA_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        iServerRetryNum = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        mIpGot = 1;
        vWifi_Callback(event_data, WIFI_IP_GOT, NULL, 0);
    }
}

void WIFI_vInit_sta(WifiConfigTPDF *tCfg)
{
	s_wifi_event_group = xEventGroupCreate();

   /* ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());*/
    //esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &vWifi_STA_Event_Handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &vWifi_STA_Event_Handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /*.ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,*/
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };

    /* set ssid and pwd from partition table */
    memcpy(wifi_config.sta.ssid, tCfg->cStaSSID, sizeof(tCfg->cStaSSID));
    memcpy(wifi_config.sta.password, tCfg->cStaPWD, sizeof(tCfg->cStaPWD));

    vConvert_MB_HiLo_Byte(wifi_config.sta.ssid, sizeof(tCfg->cStaSSID));
    vConvert_MB_HiLo_Byte(wifi_config.sta.password, sizeof(tCfg->cStaPWD));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    if(tCfg->usStaDHCPMode == WIFI_DHCP_MODE_STATIC)
    {
    	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_STA);
    	tcpip_adapter_ip_info_t tCfgStatic;
    	tCfgStatic.gw.addr = Wifi_dConvert_To_IP(tCfg->usStaGateWayH, tCfg->usStaGateWayL);
    	tCfgStatic.ip.addr = Wifi_dConvert_To_IP(tCfg->usStaIPH, tCfg->usStaIPL);
    	tCfgStatic.netmask.addr = Wifi_dConvert_To_IP(tCfg->usStaSubMaskH, tCfg->usStaSubMaskL);
    	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_STA, &tCfgStatic);
    }
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(STA_TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(STA_TAG, "connected to ap SSID:%s password:%s",
        		wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGE(STA_TAG, "Failed to connect to SSID:%s, password:%s",
        		wifi_config.sta.ssid, wifi_config.sta.password);
    }
    else
    {
        ESP_LOGE(STA_TAG, "UNEXPECTED EVENT");
    }

    /* The event will not be processed after unregister */
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip));
    //ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    //vEventGroupDelete(s_wifi_event_group);
}


/* Send TCP data frames to a file-descriptors pipe of TCP server or client */
int WIFI_vData_Send(int iSocket, char *cData, uint16_t usLen)
{
	//esp_log_buffer_hex(TAG, cData, usLen);
	int iRet = send(iSocket, cData, usLen, MSG_NOSIGNAL);
	if(iRet < 0 )
	{
		ESP_LOGE(TAG, "ERROR ERROR");
	}
	return(iRet);
}

void WIFI_vRegister_Callback(WIFI_vEvent_CallBack callback)
{
	mWifiProcess.tCallback = callback;
}
