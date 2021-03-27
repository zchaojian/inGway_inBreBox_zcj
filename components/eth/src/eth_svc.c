/*
 * ethernet_svc.c
 *
 *  Created on: Jul 17, 2020
 *      Author: liwei
 */
#include "eth_svc.h"

static const char *TAG = "Eth_Service";

typedef struct
{
	ETH_vEvent_CallBack tCallback;
	ETHEventTPDF tEvent;
}EtherNetProcessTPDF;
EtherNetProcessTPDF mEtherNetProcess = {0 };
static uint8_t mIpGot = 0;

static void vEth_Callback(void *tTag, ETHEventTypeTPDF tType, char *cData, uint16_t usLen)
{
	if(mEtherNetProcess.tCallback)
	{
		mEtherNetProcess.tEvent.tType = tType;
		mEtherNetProcess.tEvent.tTag = tTag;
		mEtherNetProcess.tEvent.cData = cData;
		mEtherNetProcess.tEvent.usLen = usLen;
		mEtherNetProcess.tCallback(&mEtherNetProcess.tEvent);
	}
}

static void vEth_Event_Handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    uint8_t mac_addr[6] = {0};
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;
    switch (event_id)
    {
    case ETHERNET_EVENT_CONNECTED:
    	esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
    	ESP_LOGI(TAG, "Ethernet connect!");
    	vEth_Callback(event_data, ETH_PHY_CONNECTED, NULL, 0);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
    	mIpGot = 0;
    	ESP_LOGI(TAG, "Ethernet disconnect!");
    	vEth_Callback(event_data, ETH_PHY_DISCONNECTED, NULL, 0);
        break;
    case ETHERNET_EVENT_START:
    	ESP_LOGI(TAG, "Ethernet start!");
    	vEth_Callback(event_data, ETH_PHY_START, NULL, 0);
        break;
    case ETHERNET_EVENT_STOP:
    	ESP_LOGI(TAG, "Ethernet stop!");
    	vEth_Callback(event_data, ETH_PHY_STOP, NULL, 0);
        break;
    default:
        break;
    }
}

static void vIPGot_Event_handler(void *arg, esp_event_base_t event_base,
                                 int32_t event_id, void *event_data)
{
	mIpGot = 1;
    ESP_LOGI(TAG, "Ethernet ip got!");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&(((ip_event_got_ip_t *)event_data)->ip_info.ip)));
    vEth_Callback(event_data, ETH_IP_GOT, NULL, 0);
}

static uint32_t Eth_dConvert_To_IP(uint16_t usIpH, uint16_t usIpL)
{
	uint8_t i[4];
	i[0] = (usIpH >> 8) & 0x00ff;
	i[1] = usIpH & 0x00ff;
	i[2] = (usIpL >> 8) & 0x00ff;
	i[3] = usIpL & 0x00ff;
	return(i[0] + (i[1] <<8) + (i[2] << 16) + (i[3] << 24));
}

/* Send TCP data frames to a file-descriptors pipe of TCP server or client */
int ETH_vData_Send(int iSocket, char *cData, uint16_t usLen)
{
	//esp_log_buffer_hex(TAG, cData, usLen);
	int iRet = send(iSocket, cData, usLen, MSG_NOSIGNAL);
	if(iRet < 0 )
	{
		ESP_LOGE(TAG, "ERROR ERROR");
	}
	return(iRet);
}

/* register ETH event callback function, and call ETH connected event status */
void ETH_vRegister_Callback(ETH_vEvent_CallBack callback)
{
	mEtherNetProcess.tCallback = callback;
}

void ETH_vInit(EthernetConfigTPDF *tCfg)
{
    // Initialize TCP/IP network interface (should be called only once in application)
    ESP_ERROR_CHECK(esp_netif_init());
    // Create default event loop that running in background
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);
    // Set default handlers to process TCP/IP stuffs
    ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));
    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &vEth_Event_Handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &vIPGot_Event_handler, NULL));

    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = CONFIG_ETHERNET_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_ETHERNET_PHY_RST_GPIO;
    mac_config.smi_mdc_gpio_num = CONFIG_ETHERNET_MDC_GPIO;
    mac_config.smi_mdio_gpio_num = CONFIG_ETHERNET_MDIO_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    /* start Ethernet driver state machine */
    if(tCfg->EthernetIPType == ETHERNET_IP_TYPE_STATIC)
    {
    	tcpip_adapter_dhcpc_stop(TCPIP_ADAPTER_IF_ETH);
    	tcpip_adapter_ip_info_t tCfgStatic;
    	tCfgStatic.gw.addr = Eth_dConvert_To_IP(tCfg->usGateWayH, tCfg->usGateWayL);
    	tCfgStatic.ip.addr = Eth_dConvert_To_IP(tCfg->usIPH, tCfg->usIPL);
    	tCfgStatic.netmask.addr = Eth_dConvert_To_IP(tCfg->usSubMaskH, tCfg->usSubMaskL);
    	tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_ETH,&tCfgStatic);
    }
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}


