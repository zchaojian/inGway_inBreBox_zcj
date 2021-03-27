/*
 * ble_svc.c
 *
 *  Created on: Jul 21, 2020
 *      Author: liwei
 */
#include "ble_svc.h"

#include "esp_err.h"

static const char *TAG = "Ble_Service";
const char * BLE_SVC = "BLE";
const char * BLE_SERVER = "BLES";

static esp_bt_uuid_t mRemoteFilterServiceUuid =
{
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = REMOTE_SERVICE_FILTER_UART_UUID,},
};

static esp_bt_uuid_t mRemoteFilterCharTxUuid =
{
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = REMOTE_SERVICE_FILTER_UART_TX_UUID,},
};

static esp_bt_uuid_t mRemoteFilterCharRxUuid =
{
    .len = ESP_UUID_LEN_128,
    .uuid = {.uuid128 = REMOTE_SERVICE_FILTER_UART_RX_UUID,},
};

static esp_bt_uuid_t mLocalNotifyDescrUuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t mBleScanParams =
{
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x2f,
    .scan_window            = 0x28,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};
static BleRemoteDeviceTPDF *mBleRemoteDevices;
static uint8_t mBleCScanReady = 0;


static void vGatts_Profile_Event_Handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static uint8_t mAdvServiceUuid128[32] =
{
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xEE, 0x00, 0x00, 0x00,
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static uint8_t mGattsChar1Str[] = {0x11,0x22,0x33};
static esp_gatt_char_prop_t mGattsProperty = 0;

static esp_attr_value_t mGattsDemoChar1Val =
{
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len     = sizeof(mGattsChar1Str),
    .attr_value   = mGattsChar1Str,
};

static uint8_t mGattsAdvConfigDone = 0;

static esp_ble_adv_data_t mGattsAdvData =
{
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = false,
    .min_interval = 0x0006,
    .max_interval = 0x0010,
    .appearance = 0x00,
    .manufacturer_len = 0,
    .p_manufacturer_data =  NULL,
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = ESP_UUID_LEN_128,
    .p_service_uuid = mAdvServiceUuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
static esp_ble_adv_data_t mGattsScanRspData =
{
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .appearance = 0x00,
    .manufacturer_len = 0, //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data =  NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = ESP_UUID_LEN_128,
    .p_service_uuid =  mAdvServiceUuid128,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
/** */
static esp_ble_adv_params_t mGattsAdvParams =
{
    .adv_int_min        = 0x20,
    .adv_int_max        = 0x40,
    .adv_type           = ADV_TYPE_IND,
    .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
    .channel_map        = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct GattsProfileInstTPDF
{
    esp_gatts_cb_t tGattsCb;
    uint16_t usGattsIf;
    uint16_t usConnId;
    uint16_t usServiceHandle;
    esp_gatt_srvc_id_t tServiceId;
    uint16_t usCharHandle;
    esp_bt_uuid_t tCharUuid;
    esp_gatt_perm_t tPerm;
    esp_gatt_char_prop_t tProperty;
    uint16_t usDescrHandle;
    esp_bt_uuid_t tDescrUuid;
    esp_gatt_if_t tSendGattsIf;
    uint16_t usSendConnId;
    uint8_t ucSendEn;
};

static struct GattsProfileInstTPDF mGattsGlProfileTab=
{
	.tGattsCb = vGatts_Profile_Event_Handler,
	.usGattsIf = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
	.ucSendEn = 0,
};

typedef struct
{
    uint8_t	*ucPrepareBuf;
    int	iPrepareLen;
}GattsPrepareTypeEnvTPDF;

static GattsPrepareTypeEnvTPDF mGattsPrepareWriteEnv;

void vExample_Write_Event_Env(esp_gatt_if_t gatts_if, GattsPrepareTypeEnvTPDF *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void vExample_Exec_Write_Event_Env(GattsPrepareTypeEnvTPDF *prepare_write_env, esp_ble_gatts_cb_param_t *param);

static DataRecCallBackTPDF mSDataRecCallBack;
static BLECDataRecCallBackTPDF mCDataRecCallBack;
static BLECScanResultTPDF mBLECScanResult;
static uint32_t mTimeTick = 0;

static void vGattc_Profile_Event_Handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *tData = (esp_ble_gattc_cb_param_t *)param;

    switch (event)
    {
    case ESP_GATTC_REG_EVT:
    	ESP_LOGW(BLE_SVC, "ESP_GATTC_REG_EVT, T = %d", mTimeTick);
    	//printf("BLE:ESP_GATTC_REG_EVT, T = %d\n", mTimeTick);
    	esp_ble_gap_set_scan_params(&mBleScanParams);
    	break;
    case ESP_GATTC_CONNECT_EVT:
    	mBleRemoteDevices->usConnId = tData->connect.conn_id;
    	ESP_LOGW(BLE_SVC, "ESP_GATTC_CONNECT_EVT, T = %d", mTimeTick);
    	//printf("BLE:ESP_GATTC_CONNECT_EVT, T= %d\n", mTimeTick);
    	esp_ble_gattc_search_service(gattc_if, tData->connect.conn_id, &mRemoteFilterServiceUuid);
    	break;
    case ESP_GATTC_OPEN_EVT:
        break;
    case ESP_GATTC_CFG_MTU_EVT:
    	break;
    case ESP_GATTC_SEARCH_RES_EVT:
    {
    	ESP_LOGW(BLE_SVC, "ESP_GATTC_SEARCH_RES_EVT, T = %d", mTimeTick);
    	//printf("BLE:ESP_GATTC_SEARCH_RES_EVT, T = %d\n", mTimeTick);
    	if (tData->search_res.srvc_id.uuid.len == ESP_UUID_LEN_128)
    	{
    		if(memcmp(tData->search_res.srvc_id.uuid.uuid.uuid128, mRemoteFilterServiceUuid.uuid.uuid128, 16) == 0)
    		{
    			mBleRemoteDevices->ucConnectState = BLE_REMOTE_DEVICE_STATE_GETSERVER;
    			mBleRemoteDevices->usServiceStartHandle = tData->search_res.start_handle;
    			mBleRemoteDevices->usServiceEndHandle = tData->search_res.end_handle;
    		}
    	}
    	break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
    	ESP_LOGW(BLE_SVC, "ESP_GATTC_SEARCH_CMPL_EVT, T = %d", mTimeTick);
    	//printf("BLE:ESP_GATTC_SEARCH_CMPL_EVT, T = %d\n", mTimeTick);
        if (tData->search_cmpl.status != ESP_GATT_OK)
        {
            break;
        }
        if(mBleRemoteDevices->ucConnectState == BLE_REMOTE_DEVICE_STATE_GETSERVER)
        {
        	uint16_t count = 0;
        	esp_gatt_status_t status = esp_ble_gattc_get_attr_count(gattc_if,
                													tData->search_cmpl.conn_id,
                                                                    ESP_GATT_DB_CHARACTERISTIC,
																	mBleRemoteDevices->usServiceStartHandle,
																	mBleRemoteDevices->usServiceEndHandle,
                                                                    INVALID_HANDLE,
                                                                    &count);
        	if (count == 2)	///<two chars
        	{
        		mBleRemoteDevices->mCharElemResult = (esp_gattc_char_elem_t *)malloc(sizeof(esp_gattc_char_elem_t) * count);
        		if (mBleRemoteDevices->mCharElemResult)
        		{
        			///<find rx char
        			status = esp_ble_gattc_get_char_by_uuid(gattc_if,
                        									tData->search_cmpl.conn_id,
															mBleRemoteDevices->usServiceStartHandle,
															mBleRemoteDevices->usServiceEndHandle,
															mRemoteFilterCharRxUuid,
															mBleRemoteDevices->mCharElemResult,
															&count);
        			if (count > 0 && (mBleRemoteDevices->mCharElemResult[0].properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY))
        			{
        				mBleRemoteDevices->usCharHandle = mBleRemoteDevices->mCharElemResult[0].char_handle;
                        esp_ble_gattc_register_for_notify(gattc_if,
                        								  mBleRemoteDevices->ucMac,
														  mBleRemoteDevices->mCharElemResult[0].char_handle);
        			}
        			///<find tx char
        			status = esp_ble_gattc_get_char_by_uuid(gattc_if,
        													tData->search_cmpl.conn_id,
															mBleRemoteDevices->usServiceStartHandle,
															mBleRemoteDevices->usServiceEndHandle,
															mRemoteFilterCharTxUuid,
															&mBleRemoteDevices->mCharElemResult[1],
															&count);
        			if (count > 0 && (mBleRemoteDevices->mCharElemResult[1].properties & ESP_GATT_CHAR_PROP_BIT_WRITE))
        			{
        				mBleRemoteDevices->usCharSendHandle = mBleRemoteDevices->mCharElemResult[1].char_handle;
        			}
        		}
        		free(mBleRemoteDevices->mCharElemResult);
        	}
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT:
        if (tData->reg_for_notify.status == ESP_GATT_OK)
        {
            uint16_t count = 0;
            uint16_t notify_en = 1;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count(gattc_if,
            															mBleRemoteDevices->usConnId,
                                                                        ESP_GATT_DB_DESCRIPTOR,
																		mBleRemoteDevices->usServiceStartHandle,
																		mBleRemoteDevices->usServiceEndHandle,
																		mBleRemoteDevices->usCharHandle,
                                                                        &count);
            if (count > 0)
            {
            	mBleRemoteDevices->mDescrElemResult = malloc(sizeof(esp_gattc_descr_elem_t) * count);
                ret_status = esp_ble_gattc_get_descr_by_char_handle(gattc_if,
                													mBleRemoteDevices->usConnId,
																	tData->reg_for_notify.handle,
																	mLocalNotifyDescrUuid,
																	mBleRemoteDevices->mDescrElemResult,
																	&count);
                if (count > 0 && mBleRemoteDevices->mDescrElemResult[0].uuid.len == ESP_UUID_LEN_16 && mBleRemoteDevices->mDescrElemResult[0].uuid.uuid.uuid16 == ESP_GATT_UUID_CHAR_CLIENT_CONFIG)
                {
                    ret_status = esp_ble_gattc_write_char_descr(gattc_if,
                    											mBleRemoteDevices->usConnId,
																mBleRemoteDevices->mDescrElemResult[0].handle,
                                                                sizeof(notify_en),
																(uint8_t *)&notify_en,
																ESP_GATT_WRITE_TYPE_NO_RSP,
																ESP_GATT_AUTH_REQ_NONE);
                    if (ret_status == ESP_GATT_OK)
                    {
                    	ESP_LOGW(BLE_SVC, "ESP_GATTC_REG_FOR_NOTIFY_EVT, T = %d", mTimeTick);
                    	//printf("BLE:ESP_GATTC_REG_FOR_NOTIFY_EVT, T = %d\n", mTimeTick);
                    }
                }
                free(mBleRemoteDevices->mDescrElemResult);
            }
        }
        break;
    case ESP_GATTC_NOTIFY_EVT:
        if (tData->notify.is_notify)
        {
        	mCDataRecCallBack(&mBleRemoteDevices, (char *)tData->notify.value, tData->notify.value_len);
        }
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (tData->write.status != ESP_GATT_OK)
        {
            break;
        }
        ESP_LOGW(BLE_SVC, "ESP_GATTC_WRITE_DESCR_EVT, T = %d", mTimeTick);
        //printf("BLE:ESP_GATTC_WRITE_DESCR_EVT, T = %d\n", mTimeTick);
        if(mBleRemoteDevices->ucConnectState != BLE_REMOTE_DEVICE_STATE_CONNECTED)
        {
        	mBleRemoteDevices->ucConnectState = BLE_REMOTE_DEVICE_STATE_CONNECTED;
        }
        uint8_t write_char_data[1];
        for (int i = 0; i < sizeof(write_char_data); ++i)
        {
            write_char_data[i] = i % 256;
        }
        esp_ble_gattc_write_char(gattc_if,
        						 mBleRemoteDevices->usConnId,
								 mBleRemoteDevices->usCharHandle,
								 sizeof(write_char_data),
								 write_char_data, ESP_GATT_WRITE_TYPE_RSP, ESP_GATT_AUTH_REQ_NONE);
        break;
    case ESP_GATTC_SRVC_CHG_EVT:
        break;
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (tData->write.status != ESP_GATT_OK)
        {
        	ESP_LOGW(BLE_SVC, "ESP_GATTC_WRITE_CHAR_EVT,error = %d", tData->write.status);
            //printf("BLE:ESP_GATTC_WRITE_CHAR_EVT,error = %d\n", tData->write.status);
            break;
        }
        break;
    case ESP_GATTC_DISCONNECT_EVT:
    	esp_ble_gap_disconnect(mBleRemoteDevices->ucMac);
    	mBleRemoteDevices->ucConnectState = BLE_REMOTE_DEVICE_STATE_NONE;
    	ESP_LOGW(BLE_SVC, "ESP_GATTC_DISCONNECT_EVT,if = %d, reason = %d", gattc_if, tData->disconnect.reason);
        //printf("ESP_GATTC_DISCONNECT_EVT,if = %d, reason = %d\n", gattc_if, tData->disconnect.reason);
        break;
    default:
        break;
    }
}

static void vEsp_Gap_Cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
	switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        mGattsAdvConfigDone &= (~GATTS_ADV_CONFIG_FLAG);
        if (mGattsAdvConfigDone == 0)
        {
            esp_ble_gap_start_advertising(&mGattsAdvParams);
            printf("BLES:ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT\n");
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
    	mGattsAdvConfigDone &= (~GATTS_SCAN_RSP_CONFIG_FLAG);
        if (mGattsAdvConfigDone == 0)
        {
            esp_ble_gap_start_advertising(&mGattsAdvParams);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
        	printf("BLES:ESP_GAP_BLE_ADV_START_COMPLETE_EVT,error\n");
        }
        break;
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    	ESP_LOGI(TAG, "ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT");
    	mBleCScanReady = 1;
        break;
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            printf("BLES:ESP_GAP_BLE_SCAN_START_COMPLETE_EVT, error = %d\n", param->scan_start_cmpl.status);
            break;
        }
        printf("BLES:ESP_GAP_BLE_SCAN_START_COMPLETE_EVT, T = %d\n", mTimeTick);
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt)
        {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
        	//ESP_LOGI(TAG, "ESP_GAP_SEARCH_INQ_RES_EVT");
        	if(mBLECScanResult)
        	{
        		//void vDevdisc_BleScanResult(uint8_t *ucMac, uint8_t *ucData) function callback
        		//bda: ble device address		ble_adv: ble Advertising data, Received EIR
        		mBLECScanResult(scan_result->scan_rst.bda, scan_result->scan_rst.ble_adv);
        	}
        	if(mBleRemoteDevices->ucConnectState != BLE_REMOTE_DEVICE_STATE_NONE)
        	{
        		break;
        	}
        	if(memcmp(mBleRemoteDevices->ucMac, scan_result->scan_rst.bda, 6) == 0)
        	{
        		printf("BLE:DEVICE FOUND %x:%x:%x:%x:%x:%x\n",scan_result->scan_rst.bda[0],scan_result->scan_rst.bda[1],
        	        	scan_result->scan_rst.bda[2],scan_result->scan_rst.bda[3],scan_result->scan_rst.bda[4],scan_result->scan_rst.bda[5]);
        		mBleRemoteDevices->ucConnectState = BLE_REMOTE_DEVICE_STATE_CONNECTING;
        		esp_ble_gap_stop_scanning();
        		esp_ble_gattc_open(mBleRemoteDevices->usGattcIf, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
        		break;
        	}
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            printf("BLE:ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT, error = %d\n", param->scan_stop_cmpl.status);
            break;
        }
        printf("BLE:ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT\n");
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
        	printf("BLE:ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT, error = %d\n", param->adv_stop_cmpl.status);
            break;
        }
        printf("BLE:ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT\n");
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        break;
    default:
    	printf("BLE:OTHER EVENT:%d\n", event);
        break;
    }
}

static void vEsp_Gattc_Cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
        	mBleRemoteDevices->usGattcIf = gattc_if;
        }
        else
        {
            printf("BLE:REG APP FAILD, APP_ID %04x, status %d\n",param->reg.app_id,param->reg.status);
            return;
        }
    }
    if (gattc_if == ESP_GATT_IF_NONE || gattc_if == mBleRemoteDevices->usGattcIf)
    {
    	vGattc_Profile_Event_Handler(event, gattc_if, param);
    }
}

void vExample_Write_Event_Env(esp_gatt_if_t gatts_if, GattsPrepareTypeEnvTPDF *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;
    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            if (prepare_write_env->ucPrepareBuf == NULL)
            {
                prepare_write_env->ucPrepareBuf = (uint8_t *)malloc(GATTS_PREPARE_BUF_MAX_SIZE*sizeof(uint8_t));
                prepare_write_env->iPrepareLen = 0;
                if (prepare_write_env->ucPrepareBuf == NULL)
                {
                	printf("BLES:GATT SERVER PREP NO MEM\n");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }
            else
            {
                if(param->write.offset > GATTS_PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_OFFSET;
                }
                else if ((param->write.offset + param->write.len) > GATTS_PREPARE_BUF_MAX_SIZE)
                {
                    status = ESP_GATT_INVALID_ATTR_LEN;
                }
            }
            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
            esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
            if (response_err != ESP_OK)
            {
            	printf("BLES:SEND RESPONSE ERROR\n");
            }
            free(gatt_rsp);
            if (status != ESP_GATT_OK)
            {
                return;
            }
            memcpy(prepare_write_env->ucPrepareBuf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->iPrepareLen += param->write.len;
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void vExample_Exec_Write_Event_Env(GattsPrepareTypeEnvTPDF *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        //esp_log_buffer_hex(GATTS_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }
    else
    {
    	printf("BLES:ESP_GATT_PREP_WRITE_CANCEL\n");
    }
    if (prepare_write_env->ucPrepareBuf)
    {
        free(prepare_write_env->ucPrepareBuf);
        prepare_write_env->ucPrepareBuf = NULL;
    }
    prepare_write_env->iPrepareLen = 0;
}

static void vGatts_Profile_Event_Handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param) {
	char cBleBrdcast[64];
	int i;
	i = sizeof(GATTS_TAG);
	/*memset(cBleBrdcast, 0, sizeof(cBleBrdcast));
	memcpy(cBleBrdcast, GATTS_TAG, i);
	esp_log_buffer_hex(TAG, cBleBrdcast, sizeof(cBleBrdcast));

	memcpy(cBleBrdcast + i, mPartitionTable.BasicInfor.ProductSN, sizeof(mPartitionTable.BasicInfor.ProductSN));
	for(int j = 0;j < sizeof(cBleBrdcast); j++)
	{
		cBleBrdcast[j + i] = mPartitionTable.BasicInfor.ProductSN[j];
	}
	esp_log_buffer_hex(TAG, cBleBrdcast, sizeof(cBleBrdcast));*/

    switch (event)
    {
    case ESP_GATTS_REG_EVT:
    	ESP_LOGI(BLE_SERVER, "ESP_GATTS_REG_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        //printf("BLES:ESP_GATTS_REG_EVT, status %d, app_id %d\n", param->reg.status, param->reg.app_id);
        mGattsGlProfileTab.tServiceId.is_primary = true;
        mGattsGlProfileTab.tServiceId.id.inst_id = 0x00;
        mGattsGlProfileTab.tServiceId.id.uuid.len = ESP_UUID_LEN_16;
        mGattsGlProfileTab.tServiceId.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_TEST_A;
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(GATTS_TAG);
        if (set_dev_name_ret)
        {
        	printf("BLES:SET DEVICE NAME FAILD, ERROR = %d\n", set_dev_name_ret);
        }
        esp_err_t ret = esp_ble_gap_config_adv_data(&mGattsAdvData);
        if (ret)
        {
        	printf("BLES:CONFIG ADV DATA FAILD, ERROR = %d\n", ret);
        }
        mGattsAdvConfigDone |= GATTS_ADV_CONFIG_FLAG;
        ret = esp_ble_gap_config_adv_data(&mGattsScanRspData);
        if (ret)
        {
        	printf("BLES:CONFIG SCAN RESPONSE DATA FAILD, ERROR = %d\n", ret);
        }
        mGattsAdvConfigDone |= GATTS_SCAN_RSP_CONFIG_FLAG;
        esp_ble_gatts_create_service(gatts_if, &mGattsGlProfileTab.tServiceId, GATTS_NUM_HANDLE_TEST_A);
        break;
    case ESP_GATTS_READ_EVT:
    {
    	ESP_LOGI(BLE_SERVER, "ESP_GATTS_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
    	//printf("BLES:ESP_GATTS_READ_EVT, conn_id %d, trans_id %d, handle %d\n", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: {
    	ESP_LOGI(BLE_SERVER, "ESP_GATTS_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
    	//printf("BLES:ESP_GATTS_WRITE_EVT, conn_id %d, trans_id %d, handle %d\n", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep)
        {
        	mGattsGlProfileTab.tSendGattsIf = gatts_if;
        	mGattsGlProfileTab.usSendConnId = param->write.conn_id;
        	mGattsGlProfileTab.ucSendEn = 1;
        	if(mSDataRecCallBack)
        	{
        		mSDataRecCallBack((char *)param->write.value,param->write.len);
        	}
            if (mGattsGlProfileTab.usDescrHandle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    if (mGattsProperty & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                    	printf("BLES:NOTIFY ENABLE\n");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i%0xff;
                        }
                        //the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, mGattsGlProfileTab.usCharHandle,
                                                sizeof(notify_data), notify_data, false);
                    }
                }
                else if (descr_value == 0x0002)
                {
                    if (mGattsProperty & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                    	printf("BLES:INDICATE ENABLE\n");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i%0xff;
                        }
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, mGattsGlProfileTab.usCharHandle,
                                                sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000)
                {
                	printf("BLES:NOTIFY/INDICATE DISABLE\n");
                }
                else
                {
                	printf("BLES:UNKNOW DESCR VALUE\n");
                }

            }
        }
        vExample_Write_Event_Env(gatts_if, &mGattsPrepareWriteEnv, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
    	ESP_LOGI(BLE_SERVER, "ESP_GATTS_EXEC_WRITE_EVT");
    	//printf("BLES:ESP_GATTS_EXEC_WRITE_EVT\n");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        vExample_Exec_Write_Event_Env(&mGattsPrepareWriteEnv, param);
        break;
    case ESP_GATTS_MTU_EVT:
    	ESP_LOGI(BLE_SERVER, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
    	//printf("BLES:ESP_GATTS_MTU_EVT, MTU %d\n", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
    	ESP_LOGI(BLE_SERVER, "CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
    	//printf("BLES:CREATE_SERVICE_EVT, status %d,  service_handle %d\n", param->create.status, param->create.service_handle);
    	mGattsGlProfileTab.usServiceHandle = param->create.service_handle;
    	mGattsGlProfileTab.tCharUuid.len = ESP_UUID_LEN_16;
    	mGattsGlProfileTab.tCharUuid.uuid.uuid16 = GATTS_CHAR_UUID_TEST_A;
        esp_ble_gatts_start_service(mGattsGlProfileTab.usServiceHandle);
        mGattsProperty = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
        esp_err_t add_char_ret = esp_ble_gatts_add_char(mGattsGlProfileTab.usServiceHandle, &mGattsGlProfileTab.tCharUuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
														mGattsProperty,
                                                        &mGattsDemoChar1Val, NULL);
        if (add_char_ret)
        {
        	printf("BLES:ADD CHAR FAILD, error = %d\n",add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        uint16_t length = 0;
        const uint8_t *prf_char;
        ESP_LOGI(BLE_SERVER, "ESP_GATTS_ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        /*printf("BLES:ESP_GATTS_ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d\n",
                param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);*/
        mGattsGlProfileTab.usCharHandle = param->add_char.attr_handle;
        mGattsGlProfileTab.tDescrUuid.len = ESP_UUID_LEN_16;
        mGattsGlProfileTab.tDescrUuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
        esp_err_t get_attr_ret = esp_ble_gatts_get_attr_value(param->add_char.attr_handle,  &length, &prf_char);
        if (get_attr_ret == ESP_FAIL)
        {
        	printf("BLES:ILLEGAL HANDLE\n");
        }

        printf("BLES:THE GATTS DEMO CHAR LENTH = %d\n", length);
        for(int i = 0; i < length; i++)
        {
        	printf("BLES:PRF_CHAR[%x] = %d\n",i,prf_char[i]);
        }
        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(mGattsGlProfileTab.usServiceHandle, &mGattsGlProfileTab.tDescrUuid,
                                                                ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
        if (add_descr_ret)
        {
        	printf("BLES:ADD CHAR DESCR FAILD, error = %d\n", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
    	mGattsGlProfileTab.usDescrHandle = param->add_char_descr.attr_handle;
        printf("BLES:ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d\n",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
    	printf("BLES:SERVICE_START_EVT, status %d, service_handle %d\n",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
        conn_params.latency = 0;
        conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
        conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
        printf("BLES:ESP_GATTS_CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:\n",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        mGattsGlProfileTab.usConnId = param->connect.conn_id;
        esp_ble_gap_update_conn_params(&conn_params);
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
    	printf("BLES:ESP_GATTS_DISCONNECT_EVT,if = %d, disconnect reason 0x%x\n", gatts_if, param->disconnect.reason);
        esp_ble_gap_start_advertising(&mGattsAdvParams);
        break;
    case ESP_GATTS_CONF_EVT:
    	printf("BLES:ESP_GATTS_CONF_EVT, status %d attr_handle %d\n", param->conf.status, param->conf.handle);
        if (param->conf.status != ESP_GATT_OK)
        {
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void vEsp_Gatts_Event_Handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
        	mGattsGlProfileTab.usGattsIf = gatts_if;
        }
        else
        {
            printf("BLES:REG APP FAILD, app_id %04x, status %d\n",param->reg.app_id, param->reg.status);
            return;
        }
    }
    if (gatts_if == ESP_GATT_IF_NONE || gatts_if == mGattsGlProfileTab.usGattsIf)
    {
    	if (mGattsGlProfileTab.tGattsCb)
    	{
    		mGattsGlProfileTab.tGattsCb(event, gatts_if, param);
    	}
    }
}

BleRemoteDeviceTPDF *BLE_C_Connect_Taget(uint8_t *ucMac)
{
	uint16_t i;
	if(!mBleCScanReady)
	{
		return(NULL);
	}
	if(mBleRemoteDevices->ucConnectState == BLE_REMOTE_DEVICE_STATE_CONNECTED && memcmp(mBleRemoteDevices->ucMac, ucMac, 6) == 0)
	{
		return(mBleRemoteDevices);
	}
	if(mBleRemoteDevices->ucConnectState == BLE_REMOTE_DEVICE_STATE_CONNECTED)
	{
		esp_ble_gattc_close(mBleRemoteDevices->usGattcIf, mBleRemoteDevices->usConnId);
		for(i = 0; i < 1000; i++)
		{
			vTaskDelay(1);
			if(mBleRemoteDevices->ucConnectState == BLE_REMOTE_DEVICE_STATE_NONE)
			{
				break;
			}
		}
	}
	mBleRemoteDevices->ucConnectState = BLE_REMOTE_DEVICE_STATE_NONE;
	memcpy(mBleRemoteDevices->ucMac, ucMac, 6);
	esp_ble_gap_start_scanning(30);
	return(mBleRemoteDevices);
}

void BLE_C_vStartScan(void)
{
	 esp_err_t err;
	if(!mBleCScanReady)
	{
		return;
	}
	ESP_LOGI(TAG, "esp_ble_gap_start_scanning");
	err = esp_ble_gap_start_scanning(30);
	if(err != ESP_OK)
	{
		esp_err_to_name(err);
	}
}

void BLE_C_vRegister_ScanResult_Callback(BLECScanResultTPDF tCallback)
{
	mBLECScanResult = tCallback;
}

void BLE_C_vRegister_Data_Rec_Callback(BLECDataRecCallBackTPDF tCallback)
{
	mCDataRecCallBack = tCallback;
}

void BLE_S_vRegister_Data_Rec_Callback(DataRecCallBackTPDF tCallback)
{
	mSDataRecCallBack = tCallback;
}

uint8_t BLE_S_dData_Send(char *cData, uint16_t usLen)
{
	if(mGattsGlProfileTab.ucSendEn)
	{
		esp_ble_gatts_send_indicate(mGattsGlProfileTab.tSendGattsIf, mGattsGlProfileTab.usSendConnId,
									mGattsGlProfileTab.usCharHandle, usLen, (uint8_t *)cData, false);
		mGattsGlProfileTab.ucSendEn = 0;
		return(1);
	}
	return(0);
}

uint8_t BLE_C_dData_Send(BleRemoteDeviceTPDF *tTag, char *cData, uint16_t usLen)
{
	if(tTag->ucConnectState == BLE_REMOTE_DEVICE_STATE_CONNECTED)
	{
		if(esp_ble_gattc_write_char(tTag->usGattcIf, tTag->usConnId, tTag->usCharSendHandle, usLen, (uint8_t *)cData,
									ESP_GATT_WRITE_TYPE_RSP,ESP_GATT_AUTH_REQ_NONE) == ESP_OK)
		{
			return(1);
		}
	}
	return(0);
}

void BLE_vInit(void)
{
	esp_err_t tRet;
	ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
	esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	tRet = esp_bt_controller_init(&bt_cfg);
	mBleRemoteDevices = malloc(sizeof(BleRemoteDeviceTPDF));
	memset(mBleRemoteDevices, 0, sizeof(BleRemoteDeviceTPDF));
	mBleRemoteDevices->usGattcIf = ESP_GATT_IF_NONE;
	if(tRet)
	{
		printf("initialize controller failed\n");
		return;
	}
	tRet = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (tRet)
    {
    	printf("enable controller failed %s\n",esp_err_to_name(tRet));
        return;
    }
    tRet = esp_bluedroid_init();
    if (tRet)
    {
    	printf("init bluetooth failed\n");
        return;
    }
    tRet = esp_bluedroid_enable();
    if (tRet)
    {
    	printf("enable bluetooth failed\n");
        return;
    }
    tRet = esp_ble_gap_register_callback(vEsp_Gap_Cb);
    if (tRet)
    {
    	printf("gap register failed\n");
        return;
    }

    tRet = esp_ble_gatts_register_callback(vEsp_Gatts_Event_Handler);
    if(tRet)
    {
    	printf("gattc register failed\n");
    	return;
    }
    tRet = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (tRet)
    {
    	printf("gatts app register error, error code = %x", tRet);
    	return;
    }
    tRet = esp_ble_gattc_register_callback(vEsp_Gattc_Cb);
    if(tRet)
    {
    	printf("gattc register failed\n");
        return;
    }
    tRet = esp_ble_gattc_app_register(0);
    if (tRet)
    {
    	printf("gattc app register failed\n");
    }
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(160);
    if (local_mtu_ret)
    {
    	printf("set local  MTU failed\n");
    }
}
