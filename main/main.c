/* Hello World Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_flash_partitions.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"

#include "eth_mqtt_tsk.h"
#include "uart_svc.h"
#include "device_app.h"
#include "config_app.h"
#include "eth_tsk.h"
#include "wifi_tsk.h"
#include "comm1_tsk.h"
#include "comm2_tsk.h"
#include "ble_tsk.h"
#include "devdisc_tsk.h"
#include "udp_brdcast.h"
#include "blink.h"
#include "key.h"

static const char *TAG = "Main";
#define HASH_LEN 32 /* SHA-256 digest length */

/* Send MyQueueDataTPDF sturncture data to a specified message Queue.
 * like mqtt task, socket task */
static uint8_t Main_vRequest_From_Ram(DeviceTPDF *tDev, MyQueueDataTPDF *tMyQueueDataUp, QueueHandle_t mSendTOQueue)
{
	uint16_t i;
	uint16_t *tData = DEV_dGet_Mem(tDev, tMyQueueDataUp->tMyQueueCommand.usRegAddr, tMyQueueDataUp->tMyQueueCommand.usRegLen);
	if(tData)
	{
		for(i = 0; i < tMyQueueDataUp->tMyQueueCommand.usRegLen; i++)
		{
			tMyQueueDataUp->tMyQueueCommand.cData[i * 2] = (tData[i] >> 8) & 0x00ff;
			tMyQueueDataUp->tMyQueueCommand.cData[i * 2 + 1] = tData[i] & 0x00ff;
		}
		tMyQueueDataUp->tMyQueueCommand.usDatalen = tMyQueueDataUp->tMyQueueCommand.usRegLen * 2;
		xQueueSend(mSendTOQueue, (void * )tMyQueueDataUp, (TickType_t)0);
		return(1);
	}
	return(0);
}

/* Send MyQueueDataTPDF structure data to down port, like com1, com2, ble.etc.
 * And waiting return data from the down port with in timeout */
static uint8_t Main_vRequest_From_Down(DeviceTPDF *tDev, MyQueueDataTPDF *tMyQueueDataUp)
{
	uint16_t usDelay;
	switch(tDev->tDeviceConfig.usCommPort)
	{
		case DEVICE_COMMPORT_TYPE_COM1:
			xQueueSend(mComm1QueueSend, (void * )tMyQueueDataUp, (TickType_t)0);
			if(tMyQueueDataUp->tMyQueueCommand.tMyQueueCommandType == MultiDeviceCtrl)
			{
				usDelay = 800;
			}
			else
			{
				usDelay = mPartitionTable.tEthernetConfig.EthernetTimeout / 10;
			}
			//usDelay = mPartitionTable.tEthernetConfig.EthernetTimeout / 10;
			ESP_LOGW(TAG, "TCP usDelay:%d", usDelay);
			ESP_LOGI(TAG, "Send tMyQueueDataUp data to mComm1QueueSend");
			while(usDelay)
			{
				vTaskDelay(1);
				usDelay--;
				if(xQueueReceive(mComm1QueueRec, (void * )tMyQueueDataUp, (portTickType)0))
				{
					break;
				}
			}
			if(!usDelay)
			{
				return(0);
			}
			return(1);
		case DEVICE_COMMPORT_TYPE_COM2:
			xQueueSend(mComm2QueueSend, (void * )tMyQueueDataUp, (TickType_t)0);
			usDelay = mPartitionTable.tEthernetConfig.EthernetTimeout / 10;
			ESP_LOGI(TAG, "Send tMyQueueDataUp data to mComm2QueueSend");
			while(usDelay)
			{
				vTaskDelay(1);
				usDelay--;
				if(xQueueReceive(mComm2QueueRec, (void * )tMyQueueDataUp, (portTickType)0))
				{
					break;
				}
			}
			if(!usDelay)
			{
				return(0);
			}
			return(1);
		case DEVICE_COMMPORT_TYPE_CAN:
			break;
		case DEVICE_COMMPORT_TYPE_BLE:
			xQueueSend(mBleQueueSend, (void * )tMyQueueDataUp, (TickType_t)0);
			usDelay = mPartitionTable.tEthernetConfig.EthernetTimeout;
			while(usDelay)
			{
				vTaskDelay(1);
				usDelay--;
				if(xQueueReceive(mBleQueueRec, (void * )tMyQueueDataUp, (portTickType)0))
				{
					break;
				}
			}
			if(!usDelay)
			{
				return(0);
			}
			return(1);
	}
	return(0);
}

static void Main_Eth_Process(void)
{
	MyQueueDataTPDF tMyQueueDataUp;
	DeviceTPDF *tDev;
	if(xQueueReceive(mEthQueueRec, (void * )&tMyQueueDataUp, (portTickType)0))
	{
		if(tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == Sntp)
		{
			xQueueSend(mComm1QueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
			return;
		}
		else if(tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == DeviceDiscoveryReStart
				|| tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == DeviceDiscoveryContinue
				|| tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == DeviceDiscoveryStop)
		{
			ESP_LOGW(TAG, "tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType:0x%02x", tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType);
			xQueueSend(mDevDiscQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
			return;
		}
		ESP_LOGI(TAG, "Data to device address:0x%x", tMyQueueDataUp.tMyQueueCommand.usSlaveAddr);
		tDev = DEV_dFind_By_ConvertAddr(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr);
		if(!tDev)
		{
			/* if don't add device, the code means passthrough Modbus TCP-RTU data */
			if(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr >= 1 && tMyQueueDataUp.tMyQueueCommand.usSlaveAddr <= 40)
			{
				DeviceTPDF tDevVrt;
				tDevVrt.tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM1;
				if(Main_vRequest_From_Down(&tDevVrt, &tMyQueueDataUp))
				{
					xQueueSend(mEthQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
				}
			}
			else if(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr >= 41 && tMyQueueDataUp.tMyQueueCommand.usSlaveAddr <= 80)
			{
				DeviceTPDF tDevVrt;
				tDevVrt.tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM2;
				if(Main_vRequest_From_Down(&tDevVrt, &tMyQueueDataUp))
				{
					xQueueSend(mEthQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
				}
			}
			return;
		}
		if(mPartitionTable.usGatewayRunMode == GATEWAY_MODE_TRANSMIT
		  && (tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadCoil
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadInput
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadReg
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadInputReg))
		{
			if(Main_vRequest_From_Ram(tDev, &tMyQueueDataUp, mEthQueueSend))
			{
				return;
			}
		}
		if(Main_vRequest_From_Down(tDev, &tMyQueueDataUp))
		{
			xQueueSend(mEthQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
		}
	}
}

/* wifi mode , receive data from wifiQueueRec queue, and send to a need queue */
static void Main_Wifi_Process(void)
{
	MyQueueDataTPDF tMyQueueDataUp;
	DeviceTPDF *tDev;
	if(xQueueReceive(mWifiQueueRec, (void * )&tMyQueueDataUp, (portTickType)0))
	{
		if(tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == Sntp)
		{
			xQueueSend(mComm1QueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
			return;
		}
		else if(tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == DeviceDiscoveryReStart
				|| tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == DeviceDiscoveryContinue
				|| tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == DeviceDiscoveryStop)
		{
			xQueueSend(mDevDiscQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
			return;
		}
		else if(tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == MultiDeviceCtrl)
		{
			tDev = DEV_dFind_By_ConvertAddr(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr);
			xQueueSend(mWifiQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
			Main_vRequest_From_Down(tDev, &tMyQueueDataUp);
			return;
		}
		ESP_LOGI(TAG, "Data to device address:0x%x", tMyQueueDataUp.tMyQueueCommand.usSlaveAddr);
		tDev = DEV_dFind_By_ConvertAddr(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr);
		if(!tDev)
		{
			/* if don't add device, the code means passthrough Modbus TCP-RTU data */
			if(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr >= 1 && tMyQueueDataUp.tMyQueueCommand.usSlaveAddr <= 40)
			{
				DeviceTPDF tDevVrt;
				tDevVrt.tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM1;
				if(Main_vRequest_From_Down(&tDevVrt, &tMyQueueDataUp))
				{
					xQueueSend(mWifiQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
				}
			}
			else if(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr >= 41 && tMyQueueDataUp.tMyQueueCommand.usSlaveAddr <= 80)
			{
				DeviceTPDF tDevVrt;
				tDevVrt.tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM2;
				if(Main_vRequest_From_Down(&tDevVrt, &tMyQueueDataUp))
				{
					xQueueSend(mWifiQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
				}
			}
			return;
		}
		if(mPartitionTable.usGatewayRunMode == GATEWAY_MODE_TRANSMIT
		  && (tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadCoil
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadInput
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadReg
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadInputReg))
		{
			if(Main_vRequest_From_Ram(tDev, &tMyQueueDataUp, mWifiQueueSend))
			{
				return;
			}
		}
		if(Main_vRequest_From_Down(tDev, &tMyQueueDataUp))
		{
			xQueueSend(mWifiQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
		}
	}
}

static void Main_Ble_Process(void)
{
	MyQueueDataTPDF tMyQueueDataUp;
	DeviceTPDF *tDev;
	if(xQueueReceive(mBleQueueRec, (void * )&tMyQueueDataUp, (portTickType)0))
	{
		tDev = DEV_dFind_By_ConvertAddr(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr);
		if(!tDev)
		{
			/* if don't add device, the code means passthrough BLE Modbus RTU - UART Modbus RTU data */
			if(tMyQueueDataUp.tMyQueueCommand.usSlaveAddr >= 1 && tMyQueueDataUp.tMyQueueCommand.usSlaveAddr <= 40)
			{
				DeviceTPDF tDevVrt;
				tDevVrt.tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM1;
				if(Main_vRequest_From_Down(&tDevVrt, &tMyQueueDataUp))
				{
					xQueueSend(mSBleQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
				}
			}
			else if (tMyQueueDataUp.tMyQueueCommand.usSlaveAddr >= 41 && tMyQueueDataUp.tMyQueueCommand.usSlaveAddr <= 80)
			{

				DeviceTPDF tDevVrt;
				tDevVrt.tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM2;
				if(Main_vRequest_From_Down(&tDevVrt, &tMyQueueDataUp))
				{
					xQueueSend(mSBleQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
				}
			}
			return;
		}
		if(mPartitionTable.usGatewayRunMode == GATEWAY_MODE_TRANSMIT
		  && (tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadCoil
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadInput
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadReg
		  || tMyQueueDataUp.tMyQueueCommand.tMyQueueCommandType == ReadInputReg))
		{
			if(Main_vRequest_From_Ram(tDev, &tMyQueueDataUp, mEthQueueSend))
			{
				return;
			}
		}
		if(Main_vRequest_From_Down(tDev, &tMyQueueDataUp))
		{
			ESP_LOGW("MAIN_BLE_PROCESS", "MAIN_BLE_PROCESS check 444444");
			xQueueSend(mSBleQueueSend, (void * )&tMyQueueDataUp, (TickType_t)0);
		}
	}
}

static void Main_Task(void *pvParameters)
{
	while(1)
	{
		vTaskDelay(1);
		Main_Eth_Process();
		Main_Wifi_Process();
		Main_Ble_Process();
	}
}

void test(void)
{
	mDevice[0].tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM1;
	mDevice[0].tDeviceConfig.usMainType = DEVICE_MAIN_TYPE_CHINT_MCB;
	mDevice[0].tDeviceConfig.usConvertAddr = 1;

	mDevice[1].tDeviceConfig.usCommPort = DEVICE_COMMPORT_TYPE_COM1;
	mDevice[1].tDeviceConfig.usMainType = DEVICE_MAIN_TYPE_CHINT_MCB;
	mDevice[1].tDeviceConfig.usConvertAddr = 2;
}

static void Main_Create_Task_Queue()
{
	mComm1QueueSend = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mComm1QueueRec = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mComm2QueueSend = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mComm2QueueRec = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mEthQueueSend = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mEthQueueRec = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mBleQueueSend = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mSBleQueueSend = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mBleQueueRec = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mDevDiscQueueSend = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mDevDiscQueueRec = xQueueCreate(10,sizeof(MyQueueDataTPDF));
	mWifiQueueSend = xQueueCreate(1,sizeof(MyQueueDataTPDF));
	mWifiQueueRec = xQueueCreate(1,sizeof(MyQueueDataTPDF));
}

static void Main_Hardware_Config()
{
	uint32_t uiFunction = 0;
	uiFunction = (uint32_t)mPartitionTable.usFunctionH << 16U | (uint32_t) mPartitionTable.usFunctionL;
	ESP_LOGI(TAG, "uiFunction:%d",uiFunction);

	xTaskCreate(sytem_blink_task, "blink", 2048, NULL, 5, NULL);
	xTaskCreate(vKey_task, "key task", 2048, NULL, 5, NULL);

	//if ((uiFunction & (0x3 << 8)) >> 8 == 0x01)// 0x3 << 8 take BLE state
	{
	    xTaskCreate(Ble_vTsk_Start, "ble task", 4096, NULL, 4, NULL);
	}
	if ((uiFunction & (0x3 << 6)) >> 6 == 0x01)// 0x3 << 6 take ETH state
	{
		xTaskCreate(Eth_vTsk_Start, "ethernet task", 4096, NULL, 4, NULL);
	}
	else
	{
		ESP_LOGE(TAG, "Ethernet function NOT run!");
	}
	if((uiFunction & (0x3 << 0)) >> 0 == 0x01)//0x3 take COM1 state
	{
		xTaskCreate(COM1_vTsk_Start, "comm1 task", 4096, NULL, 4, NULL);
	}
	else
	{
		ESP_LOGE(TAG, "COM1 function NOT run!");
	}
	if ((uiFunction & (0x3 << 2)) >> 2 == 0x01)// 0x3 << 2 take COM2 state
	{
		xTaskCreate(COM2_vTsk_Start, "comm2 task", 4096, NULL, 4, NULL);
	}
	else
	{
		ESP_LOGE(TAG, "COM2 function NOT run!");
	}
	if ((uiFunction & (0x3 << 4)) >> 4 == 0x01)// 0x3 << 4 take CAN state
	{

	}
	else
	{
		ESP_LOGE(TAG, "CAN function NOT run!");
	}
	if ((uiFunction & (0x3 << 10)) >> 10 == 0x01)// 0x3 << 10 take WIFI state
	{
		xTaskCreate(Wifi_vTsk_Start, "wifi task", 4096, NULL, 4, NULL);
	}
	else
	{
		ESP_LOGE(TAG, "WIFI function NOT run!");
	}
	if ((uiFunction & (0x3 << 12)) >> 12 == 0x01)// 0x3 << 12 take IOT state
	{

	}
	else
	{
		ESP_LOGE(TAG, "IOT function NOT run!");
	}

	{
		/* system blink of GPIO 12 */
		//xTaskCreate(sytem_blink_task, "blink", 2048, NULL, 5, NULL);
		//xTaskCreate(vKey_task, "key task", 2048, NULL, 5, NULL);
		xTaskCreate(Main_Task, "main task", 4096, NULL, 4, NULL);
		xTaskCreate(DEVDISC_Task, "devdisc task", 4096, NULL, 4, NULL);
	}
}

static void Main_Write_Vesion_Number_To_NVS()
{
	//uint8_t ucRunVersionCal[4];
	uint8_t ucAppVer[2];
	uint16_t usAppVersion;

	/* version get from esp_app_desc_t structure */
	const esp_partition_t *running = esp_ota_get_running_partition();
	esp_app_desc_t running_app_info;
	if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
	{

	    ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
	}

	//Version_Cal(running_app_info.version, (char *)ucRunVersionCal);
	vUdp_VersionAnalysis(running_app_info.version, ucAppVer);

	/*ucAppVer[0] = (ucRunVersionCal[0] - '0') * 10 + (ucRunVersionCal[1] - '0');
	ucAppVer[1] = (ucRunVersionCal[2] - '0') * 10 + (ucRunVersionCal[3] - '0');
	usAppVersion = ((uint16_t)ucAppVer[0] << 8U) | (uint16_t)ucAppVer[1];
	ESP_LOGI(TAG, "usAppVersion: 0x%04x",usAppVersion);*/
	usAppVersion = ((uint16_t)ucAppVer[0] << 8U) | (uint16_t)ucAppVer[1];
	mPartitionTable.ReserveConfig.Version = usAppVersion;

	/* write version to NVS Flash */
	CFG_vSaveConfig(0x1fe);
	//ESP_LOGI(TAG, "mPartitionTable.ReserveConfig.Version: 0x%04x",mPartitionTable.ReserveConfig.Version);

	mPartitionTable.BasicInfor.HardwareVersion = 0x1001;
	CFG_vSaveConfig(0x020E - 1);
	mPartitionTable.BasicInfor.SoftwareVersion = usAppVersion;
	CFG_vSaveConfig(0x020D - 1);
}

static void Main_Get_ESP32_MAC_Addr()
{
	uint16_t uiIndex;
	uint16_t uiSize;
	//Get the derived MAC address for each network interface
	uint8_t derived_mac_addr[6] = {0};
	//Get MAC address for WiFi Station interface
	ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_STA));
	ESP_LOGW("WIFI_STA MAC", "\t0x %02x %02x %02x %02x %02x %02x",
			  derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
			  derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);

	/* write wifi sta MAC address infor to nvs flash */
	for(int i = 0; i < 3; i++)
	{
		mPartitionTable.BasicInfor.ProductDescription[i] = (derived_mac_addr[2 * i] << 8) + derived_mac_addr[2 * i + 1];
	}
	uiIndex = 0x021B - 1;//wifi sta MAC start address: 0x021B - 1, temporary storage Product Description
	uiSize = 3;
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}

	//Get MAC address for SoftAp interface
	ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_WIFI_SOFTAP));
	ESP_LOGW("SoftAP MAC", "\t0x %02x %02x %02x %02x %02x %02x",
			  derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
			  derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);

	/* write wifi softAP MAC address infor to nvs flash */
	for(int i = 0; i < 3; i++)
	{
		mPartitionTable.BasicInfor.ProductDescription[3 + i] = (derived_mac_addr[2 * i] << 8) + derived_mac_addr[2 * i + 1];
	}
	uiIndex = 0x021E - 1;//wifi softAP MAC start address: 0x021E - 1, temporary storage Product Description
	uiSize = 3;
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}

	//Get MAC address for Bluetooth
	ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_BT));
	ESP_LOGW("BT MAC", "\t0x %02x %02x %02x %02x %02x %02x",
			  derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
			  derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);

	/* write Bluetooth MAC address infor to nvs flash */
	for(int i = 0; i < 3; i++)
	{
		mPartitionTable.BasicInfor.ProductDescription[6 + i] = (derived_mac_addr[2 * i] << 8) + derived_mac_addr[2 * i + 1];
	}
	uiIndex = 0x0221 - 1;//Bluetooth MAC start address: 0x0221 - 1, temporary storage Product Description
	uiSize = 3;
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}

	//Get MAC address for Ethernet
	ESP_ERROR_CHECK(esp_read_mac(derived_mac_addr, ESP_MAC_ETH));
	ESP_LOGW("Ethernet MAC", "\t0x %02x %02x %02x %02x %02x %02x",
			  derived_mac_addr[0], derived_mac_addr[1], derived_mac_addr[2],
			  derived_mac_addr[3], derived_mac_addr[4], derived_mac_addr[5]);

	/* writeEthernet MAC address infor to nvs flash */
	for(int i = 0; i < 3; i++)
	{
		mPartitionTable.BasicInfor.ProductDescription[9 + i] = (derived_mac_addr[2 * i] << 8) + derived_mac_addr[2 * i + 1];
	}
	uiIndex = 0x0224 - 1;//Ethernet MAC start address: 0x0224 - 1, temporary storage Product Description
	uiSize = 3;
	while(uiSize)
	{
		CFG_vSaveConfig(uiIndex++);
		uiSize--;
	}
}

void app_main(void)
{

	ESP_LOGI(TAG, "System start!");
	Main_Create_Task_Queue();
	CFG_vInit();
	Main_Get_ESP32_MAC_Addr();
	DEV_vInit();
	Main_Write_Vesion_Number_To_NVS();
	Main_Hardware_Config();

    while(1)
    {
    	vTaskDelay(100);
    }
}
