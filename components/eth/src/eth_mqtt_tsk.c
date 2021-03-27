/*
 * eth_mqtt_task.c
 *
 *  Created on: Aug 4, 2020
 *      Author: liwei
 */
#include "eth_tsk.h"
#include "eth_mqtt_tsk.h"

static MqttPublishDataTPDF *mMqttPublishData;
static uint16_t mMqttPublishDataCount;
static uint8_t mQttConnected = 0;
static esp_mqtt_client_handle_t mClient;
static MyQueueDataTPDF mMyQueueData;

static char *TAG = "eth mqtt task";
static void vMySprintf_Hex(char *source, uint32_t data, uint8_t len)
{
	uint8_t j;
	for(uint8_t i = 0; i < len; i++)
	{
		j = data & 0x0000000f;
		if(j < 10)
		{
			source[len - 1 - i] = j + 0x30;
		}
		else
		{
			source[len - 1 - i] = j - 10 + 'A';
		}
		data >>= 4;
	}
}

static char *vMySprintf_BuildChar(uint8_t *ucMem, DevicePartitionTableDataTypeTPDF tType)
{
	uint32_t i;
	int32_t si;
	int16_t ii;
	float j;
	char k[128];
	static char  *l;
	free(l);
	memset(k, 0, 32);
	switch(tType)
	{
		case DEVICE_PARTITION_TABLE_DATA_TYPE_FLOAT:
			i = ucMem[2] + (ucMem[3] << 8) + (ucMem[0] << 16) + (ucMem[1] << 24);
			j = *((float *)&i);
			sprintf(k, "%.2f\n", j);
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		case DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F1:
		case DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F2:
			i = ucMem[0] + (ucMem[1] << 8);
			j = (float)i;
			if(tType == DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F1)
			{
				j /= 10.0f;
				sprintf(k, "%.1f\n", j);
			}
			else
			{
				j /= 100.0f;
				sprintf(k, "%.2f\n", j);
			}
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		case DEVICE_PARTITION_TABLE_DATA_TYPE_SINT_F1:
		case DEVICE_PARTITION_TABLE_DATA_TYPE_SINT_F2:
			ii = ucMem[0] + (ucMem[1] << 8);
			j = (float)ii;
			if(tType == DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F1)
			{
				j /= 10.0f;
				sprintf(k, "%.1f\n", j);
			}
			else
			{
				j /= 100.0f;
				sprintf(k, "%.2f\n", j);
			}
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		case DEVICE_PARTITION_TABLE_DATA_TYPE_STATE:
		{
			uint32_t m,n;
			m = 1;
			n = *ucMem;
			k[0] = '[';
			for(i = 0; i < 16; i++)
			{
				if(n & 1)
				{
					memcpy(&k[m], "true,", 5);
					m += 5;
				}
				else
				{
					memcpy(&k[m], "false,", 6);
					m += 6;
				}
				n >>= 1;
			}
			k[m++] = ']';
			l = malloc(m + 1);
			memcpy(l,k,m);
			l[m] = 0;
			return(l);
		}
		case DEVICE_PARTITION_TABLE_DATA_TYPE_DINT:
			i = ucMem[2] + (ucMem[3] << 8) + (ucMem[0] << 16) + (ucMem[1] << 24);
			sprintf(k, "%d\n", i);
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		case DEVICE_PARTITION_TABLE_DATA_TYPE_SDINT:
			si = ucMem[2] + (ucMem[3] << 8) + (ucMem[0] << 16) + (ucMem[1] << 24);
			sprintf(k, "%d\n", si);
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		case DEVICE_PARTITION_TABLE_DATA_TYPE_SINT:
			ii = ucMem[0] + (ucMem[1] << 8);
			sprintf(k, "%d\n", ii);
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		case DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F1:
		case DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F2:
		case DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F3:
			i = ucMem[2] + (ucMem[3] << 8) + (ucMem[0] << 16) + (ucMem[1] << 24);
			j = (float)i;
			if(tType == DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F1)
			{
				j /= 10.0f;
				sprintf(k, "%.1f\n", j);
			}
			else if(tType == DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F2)
			{
				j /= 100.0f;
				sprintf(k, "%.2f\n", j);
			}
			else
			{
				j /= 1000.0f;
				sprintf(k, "%.3f\n", j);
			}
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
		default:
			i = ucMem[0] + (ucMem[1] << 8);
			sprintf(k, "%d\n", i);
			for(i = 0; k[i] != '\n';i++);
			l = malloc(i + 1);
			memcpy(l,k,i);
			l[i] = 0;
			return(l);
	}
}

static void ETH_vMqtt_Event_Handler_CB(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    uint32_t uiAddr, uiRegAddr, uiFlag, uiData;
    int msg_id;
    switch (event->event_id)
    {
        case MQTT_EVENT_CONNECTED:
        	msg_id = esp_mqtt_client_subscribe(client, "control", 0);
        	esp_mqtt_client_publish(client, "control", "1", 6, 0, 0);
        	mQttConnected = 1;
            printf("MQTT_EVENT_CONNECTED\n");
            break;
        case MQTT_EVENT_DISCONNECTED:
        	mQttConnected = 0;
        	printf("MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
        	printf("MQTT_EVENT_SUBSCRIBED, msg_id=%d\n", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
        	printf("MQTT_EVENT_UNSUBSCRIBED, msg_id=%d\n", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
        	printf("MQTT_EVENT_PUBLISHED, msg_id=%d\n", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
        	printf("MQTT_EVENT_DATA:");
        	printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			//sscanf() function realized set MB data from format string with the cloud.
        	if(sscanf(event->data, CONFIG_ETH_MQTT_DEFAULT_SUBSCRIBE_FORMAT, &uiAddr, &uiRegAddr, &uiFlag, &uiData) == 4)
        	{
        		mMyQueueData.tMyQueueCommand.tMyQueueCommandType = (ModbusFunctionTPDF)uiFlag;
        		mMyQueueData.tMyQueueCommand.usSlaveAddr = uiAddr;
        		mMyQueueData.tMyQueueCommand.usRegAddr = uiRegAddr;
        		if(mMyQueueData.tMyQueueCommand.tMyQueueCommandType == WriteMultReg
        		   || mMyQueueData.tMyQueueCommand.tMyQueueCommandType == WriteReg)
        		{
        			mMyQueueData.tMyQueueCommand.usRegLen = 1;
        			mMyQueueData.tMyQueueCommand.usDatalen = 2;
        			mMyQueueData.tMyQueueCommand.cData[0] = (uiData >> 8) & 0x000000ff;
        			mMyQueueData.tMyQueueCommand.cData[1] = uiData & 0x000000ff;
        			ESP_LOGI(TAG, "Write reg, slaves addr = %d, reg addr = %d, data0 = %d,data1 = %d, function = %d\n", uiAddr, uiRegAddr,
        				  mMyQueueData.tMyQueueCommand.cData[0],mMyQueueData.tMyQueueCommand.cData[1],uiFlag);
        			xQueueSend(mEthQueueRec, (void * )&mMyQueueData, (TickType_t)0);
        		}
        	}
            break;
        case MQTT_EVENT_ERROR:
        	printf("MQTT_EVENT_ERROR\n");
            break;
        default:
        	printf("Other event id:%d\n", event->event_id);
            break;
    }
}



////A0001R0000T10:0002:
////A0001R0000T10:0006:
////A0001R0000T10:0007:
static void ETH_vMqtt_Event_Handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
	ETH_vMqtt_Event_Handler_CB(event_data);
}

/*void Eth_Mqtt_vInit(void)
{
	uint16_t i, j, k;

	mMqttPublishDataCount = 0;
	mMqttPublishData = malloc(sizeof(MqttPublishDataTPDF) * CONFIG_ETH_MQTT_DEFAULT_PUBLISH_MAX);
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		if(mDevice[i].tDeviceConfig.usConvertAddr != 0)
		{
			for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
			{
				if(mDevice[i].tDevicePartitionTable[j].usLen)
				{
					for(k = 0; k < mDevice[i].tDevicePartitionTable[j].usLen; k++)
					{
						mMqttPublishData[mMqttPublishDataCount].cFromat = malloc(sizeof(CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT));
						memcpy(mMqttPublishData[mMqttPublishDataCount].cFromat,
								CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT,
								sizeof(CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT));
						//write converted modbus address-A to a Fromat string like "A0000R0000T00"
						vMySprintf_Hex(&mMqttPublishData[mMqttPublishDataCount].cFromat[1], mDevice[i].tDeviceConfig.usConvertAddr, 4);
						//write register-R start address to a Fromat string like "A0000R0000T00"
						vMySprintf_Hex(&mMqttPublishData[mMqttPublishDataCount].cFromat[6], mDevice[i].tDevicePartitionTable[j].usStartAddr + k, 4);
						//write modbus function code type-T to a Fromat string like "A0000R0000T00"
						vMySprintf_Hex(&mMqttPublishData[mMqttPublishDataCount].cFromat[11],
								       mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableRegType,
									   2);
						//set Device data type of publish, like "I:20" or "V:220.5" .etc.
						mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType = mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableDataType;
						//collected register data value
						mMqttPublishData[mMqttPublishDataCount].ucMem = (uint8_t *)(mDevice[i].tDevicePartitionTable[j].usMemory + k);//?
						if(mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType == DEVICE_PARTITION_TABLE_DATA_TYPE_FLOAT
						|| mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType == DEVICE_PARTITION_TABLE_DATA_TYPE_DINT
						|| mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType == DEVICE_PARTITION_TABLE_DATA_TYPE_SDINT)
						{
							k++;
						}
						mMqttPublishDataCount++;
					}
				}
			}
		}
	}
    esp_mqtt_client_config_t mqtt_cfg = {
		.host = CONFIG_ETH_MQTT_DEFAULT_HOST,
		.port = CONFIG_ETH_MQTT_DEFAULT_PORT,
		.username = CONFIG_ETH_MQTT_DEFAULT_PRODUCT_ID,
		.client_id = CONFIG_ETH_MQTT_DEFAULT_DEVICE_ID,
		.password = CONFIG_ETH_MQTT_DEFAULT_AUTH_INFOR,
		.disable_clean_session = 0,
		.keepalive = 120,
		.lwt_topic = "/lwt",
		.lwt_msg = "offline",
		.lwt_qos = 0,
		.lwt_retain = 0,
    };
    mClient = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mClient, ESP_EVENT_ANY_ID, ETH_vMqtt_Event_Handler, mClient);
    esp_mqtt_client_start(mClient);
}*/

///Tieta demo device
void Eth_Mqtt_vInit(void)
{
	uint16_t i, j, k;

	mMqttPublishDataCount = 0;
	mMqttPublishData = malloc(sizeof(MqttPublishDataTPDF) * CONFIG_ETH_MQTT_DEFAULT_PUBLISH_MAX);
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		if(mDevice[i].tDeviceConfig.usConvertAddr != 0)
		{
			for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
			{
				if(mDevice[i].tDevicePartitionTable[j].usLen)
				{
					for(k = 0; k < mDevice[i].tDevicePartitionTable[j].usLen; k++)
					{
						mMqttPublishData[mMqttPublishDataCount].cFromat = malloc(sizeof(CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT));
						memcpy(mMqttPublishData[mMqttPublishDataCount].cFromat,
								CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT,
								sizeof(CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT));
						//write converted modbus address-A to a Fromat string like "A0000R0000T00"
						vMySprintf_Hex(&mMqttPublishData[mMqttPublishDataCount].cFromat[1], mDevice[i].tDeviceConfig.usConvertAddr, 4);
						//write register-R start address to a Fromat string like "A0000R0000T00"
						vMySprintf_Hex(&mMqttPublishData[mMqttPublishDataCount].cFromat[6], mDevice[i].tDevicePartitionTable[j].usStartAddr + k, 4);
						//write modbus function code type-T to a Fromat string like "A0000R0000T00"
						vMySprintf_Hex(&mMqttPublishData[mMqttPublishDataCount].cFromat[11],
								       mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableRegType,
									   2);
						//set Device data type of publish, like "I:20" or "V:220.5" .etc.
						mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType = mDevice[i].tDevicePartitionTable[j].tDevicePartitionTableDataType;
						//collected register data value
						mMqttPublishData[mMqttPublishDataCount].ucMem = (uint8_t *)(mDevice[i].tDevicePartitionTable[j].usMemory + k);//?
						if(mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType == DEVICE_PARTITION_TABLE_DATA_TYPE_FLOAT
						|| mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType == DEVICE_PARTITION_TABLE_DATA_TYPE_DINT
						|| mMqttPublishData[mMqttPublishDataCount].tDevicePartitionTableDataType == DEVICE_PARTITION_TABLE_DATA_TYPE_SDINT)
						{
							k++;
						}
						mMqttPublishDataCount++;
					}
				}
			}
		}
	}
    esp_mqtt_client_config_t mqtt_cfg = {
		.host = CONFIG_ETH_MQTT_DEFAULT_HOST,
		.port = CONFIG_ETH_MQTT_DEFAULT_PORT,
		.username = CONFIG_MQTT_PRODUCT_ID,
		.client_id = CONFIG_MQTT_DEVICE_ID,
		.password = CONFIG_MQTT_AUTH_INFOR,
		.disable_clean_session = 0,
		.keepalive = 120,
		.lwt_topic = "/lwt",
		.lwt_msg = "offline",
		.lwt_qos = 0,
		.lwt_retain = 0,
    };
    mClient = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mClient, ESP_EVENT_ANY_ID, ETH_vMqtt_Event_Handler, mClient);
    esp_mqtt_client_start(mClient);
}

void Eth_Mqtt_vTsk_Delay(MyQueueDataTPDF tMyQueueData, uint16_t usDelay)
{
	while(usDelay)
	{
		vTaskDelay(1);
		if(xQueueReceive(mEthQueueSend, (void * )&tMyQueueData, (portTickType)0))
		{

		}
		usDelay--;
	}
}

void Eth_Mqtt_vPublish(MqttPublishDataTPDF *tData)
{
	uint16_t j;
	static char buf[128];
	memset(buf, 0, sizeof(buf));
	sprintf(&buf[3], "{\"%s\":%s}", tData->cFromat, vMySprintf_BuildChar(tData->ucMem,tData->tDevicePartitionTableDataType));
	j = strlen(&buf[3]);
	buf[0] = 3;
	buf[1] = j >> 8;
	buf[2] = j & 0xFF;
	esp_mqtt_client_publish(mClient, "$dp", buf, j + 3, 0, 0);
	buf[j + 3] = 0;
	//ESP_LOGW(TAG, "%s", buf + 3);
}

void Eth_Mqtt_vTsk(uint8_t ucConnected, MyQueueDataTPDF tMyQueueData)
{
	uint16_t i;

	mMyQueueData = tMyQueueData;
	if(ucConnected && mQttConnected)
	{
		Eth_Mqtt_vTsk_Delay(tMyQueueData, CONFIG_ETH_MQTT_DEFAULT_PUBLISH_TICKS);
		for(i = 0; i < mMqttPublishDataCount; i++)
		{
			Eth_Mqtt_vPublish(&mMqttPublishData[i]);
			Eth_Mqtt_vTsk_Delay(tMyQueueData, 1);
		}
	}
	else
	{
		Eth_Mqtt_vTsk_Delay(tMyQueueData, 1);
	}
}
