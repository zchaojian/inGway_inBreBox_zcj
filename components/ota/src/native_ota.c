/* 1.create a native update APP of ESP32.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "native_ota.h"


typedef struct {
	uint16_t usSpecialFunction;
	uint16_t usUpdateObj;
	uint8_t  ucDevice[32];
	uint16_t usSoftVersion;
	uint16_t usHardVersion;
	uint32_t uiBinSize;
	uint16_t usEveryDataPackSize;
}OTAUpdateInfoTPDF;

#define BUFFSIZE 1024
#define DATA_HEADER 6
#define CRC_LEN	2
#define RECV_BUF_SIZE	BUFFSIZE + DATA_HEADER + 4

#define CONFIG_EXAMPLE_GPIO_DIAGNOSTIC 0


static const char *TAG = "native_ota";
/*an ota data write buffer ready to write to the flash*/
//static unsigned char ota_write_data[BUFFSIZE + DATA_HEADER + CRC_LEN] = { 0 };
static unsigned char ota_write_data[RECV_BUF_SIZE] = { 0 };
static OTAModbusRtuDataTPDF mOTAModbusRtuDataRec;
static OTAUpdateInfoTPDF mOTAUpdateInfo;


static int client_fd = -1;
static int server_fd = -1;

static void tcp_cleanup()
{
    ESP_LOGE(TAG, "Shutting down client socket and restarting...");
    shutdown(client_fd, 0);
    close(client_fd);
}

static void ota_update_error_break()
{
	ESP_LOGE(TAG, "OTA receive data error, break...");
}

static void ota_update_error_continue()
{
	ESP_LOGE(TAG, "OTA update fail and rebuild TCP client...");
}

void print_sha256 (const uint8_t *image_hash, const char *label)
{
    char hash_print[HASH_LEN * 2 + 1];
    hash_print[HASH_LEN * 2] = 0;
    for (int i = 0; i < HASH_LEN; ++i) {
        sprintf(&hash_print[i * 2], "%02x", image_hash[i]);
    }
    ESP_LOGI(TAG, "%s: %s", label, hash_print);
}

static esp_err_t start_update_ota()
{
	static int buf_size = 32;
	unsigned char header_buf[buf_size];

	unsigned char ONOFF_transactionID[2] = {0xFF, 0xFF};
	unsigned char start_sig[] = {0x00, 0x06, 0x00, 0x00, 0x00, OTA_UPDATE_START};

	int len = recv(client_fd, header_buf, buf_size, 0);
	// Error occurred during receiving
	if (len < 0)
	{
		ESP_LOGE(TAG, "Receive start update signal failed: errno %d", errno);
		return ESP_FAIL;
	}
	// Connection closed
	else if (len == 0)
	{
		ESP_LOGE(TAG, "Receive start signal failed");
		ESP_LOGE(TAG, "Connection closed");
		return ESP_FAIL;
	}
	// Data received
	else
	{
		//vMODB_vAnalysis_TCP_OTA((uint8_t *)ota_write_data, len , mOTAModbusRtuDataRec);
		//mOTAModbusRtuDataRec->tTag = (void *)client_fd;


		if(memcmp(ONOFF_transactionID, header_buf, 2) != 0)
		{
			ESP_LOGE(TAG, "Receive OTA update signal: ON transactionID is WRONG");
			return ESP_FAIL;
		}

		if(memcmp(start_sig, header_buf + 6, 6) != 0)
		{
			ESP_LOGE(TAG, "Receive OTA update signal: start is WRONG");
			return ESP_FAIL;
		}
	}
	ESP_LOGI(TAG, "OTA update is started! Please send *.bin binary file,  Wait... ");
	send(client_fd, header_buf, len, 0);
	return ESP_OK;
}

static esp_err_t finish_update_ota()
{
	int err = send(client_fd, ota_write_data, DATA_HEADER + 6, 0);
	if (err < 0)
	{
		ESP_LOGE(TAG, "Send checksum failed");
		return ESP_FAIL;
	}
	return ESP_OK;
}

/* Native OTA to update esp32 program. Create a native OTA task. The task will
 * receive a binary file from PC socket */
void Native_Ota_Tsk(void *pvParameter)
{
	uint8_t mConnected = 0;
	uint8_t mWifiConnected = 0;
	uint8_t uUDPServerCreated = 0;
    esp_err_t err;
    /* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    esp_ota_handle_t update_handle = 0 ;
    const esp_partition_t *update_partition = NULL;

    ESP_LOGW(TAG, "===============================================");

    /* get boot and  running parting ,and check boot and running partition */
    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    ESP_LOGW(TAG, "-------++++++++++--------------++++++++++++++++++++++");
    if (configured != running) {
    	ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
    			 configured->address, running->address);
    	ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
    		 running->type, running->subtype, running->address);

    /* get next update partition */
    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
    		 update_partition->subtype, update_partition->address);
    assert(update_partition != NULL);

	while(1)
	{
		mConnected = Eth_Connected_Status();
		mWifiConnected = Wifi_Connected_Status();
		uUDPServerCreated = Udp_Brdcast_Status();

		if(mConnected | mWifiConnected)
		{

		}
		else
		{
			ESP_LOGE(TAG, "ETH/WIFI Network DON'T link!");
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			continue;
		}

		if(!uUDPServerCreated)
		{
			ESP_LOGE(TAG, "UDP broadcast server DON'T created, uUDPServerCreated:%d", uUDPServerCreated);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "Starting OTA example");

		if (server_fd < 0)
		{
			/* set up the tcp server socket */
			server_fd = download_tcp_server();
			if(server_fd < 0)
			{
				ESP_LOGE(TAG, "TCP server create failed");
			}
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "Native OTA tcp server already created, fd:%d", server_fd);

		client_fd = tcp_client_accept(server_fd);
		if(client_fd < 0)
		{
			ESP_LOGE(TAG, "client link TCP server failed");
			tcp_cleanup();
			ota_update_error_continue();
			continue;
		}

		/* get the information of start OTA update */
		err = start_update_ota();
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "Start update failed");
			tcp_cleanup();
			ota_update_error_continue();
			continue;
		}

		int binary_file_length = 0;
		/*deal with all receive packet*/
		bool image_header_was_checked = false;
		while (1)
		{
			unsigned char response[128];
			int data_read = tcp_server_read(client_fd, ota_write_data, BUFFSIZE + DATA_HEADER + CRC_LEN);
			if (data_read < 0)
			{
				ESP_LOGE(TAG, "Error: SSL data read error");
				tcp_cleanup();
				ota_update_error_break();
				break;
			}
			else if (data_read > 0)
			{
				memcpy(response, ota_write_data, DATA_HEADER);//store data header
				if (image_header_was_checked == false)
				{
					esp_app_desc_t new_app_info;
					if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
					{
						// check current version with downloading
						memcpy(&new_app_info, &(ota_write_data)[DATA_HEADER + sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
						ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

						esp_app_desc_t running_app_info;
						if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
						{
							ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
						}

						const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
						esp_app_desc_t invalid_app_info;
						if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK)
						{
							ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
						}

						// check current version with last invalid partition
						if (last_invalid_app != NULL)
						{
							if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0)
							{
								ESP_LOGW(TAG, "New version is the same as invalid version.");
								ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
								ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
								tcp_cleanup();
								ota_update_error_break();
								break;
							}
						}

						if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0)
						{
							ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
							tcp_cleanup();
							ota_update_error_break();
							break;
						}

						image_header_was_checked = true;

						err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
						if (err != ESP_OK)
						{
							ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
							tcp_cleanup();
							ota_update_error_break();
							break;
						}
						ESP_LOGI(TAG, "esp_ota_begin succeeded");
					}
					else
					{
						ESP_LOGE(TAG, "received package is not fit len");
						tcp_cleanup();
						ota_update_error_break();
						break;
					}
				}
				err = esp_ota_write( update_handle, (const void *)(ota_write_data + DATA_HEADER), data_read);
				if (err != ESP_OK)
				{
					ESP_LOGI(TAG, "esp_ota_write error!");
					//respose tcp data ok. ---1:response ok  ---0:response error
					tcp_server_write(client_fd, response, DATA_HEADER, 0);
					tcp_cleanup();
					ota_update_error_break();
					break;
				}
				binary_file_length += data_read;
				ESP_LOGI(TAG, "Written image length %d", binary_file_length);

				//respose tcp data ok. ---1:response ok  ---0:response error
				tcp_server_write(client_fd, response, DATA_HEADER, 1);
			}
			else if (data_read == 0)
			{
				ESP_LOGI(TAG, "All data received");
				break;
			}
		}
		ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);

		if (esp_ota_end(update_handle) != ESP_OK)
		{
			ESP_LOGE(TAG, "esp_ota_end failed!");
			tcp_cleanup();
			ota_update_error_continue();
			continue;
		}
		else
		{
			ESP_LOGI(TAG, "Checksum success, send checksum to host computer!");
			finish_update_ota();
			ESP_LOGI(TAG, "Checksum success, system will restart!");
		}

		err = esp_ota_set_boot_partition(update_partition);
		if (err != ESP_OK)
		{
			ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
			tcp_cleanup();
			ota_update_error_continue();
			continue;
		}
		ESP_LOGI(TAG, "Prepare to restart system!");
		for (int i = 5; i >= 0; i--)
		{
			ESP_LOGI(TAG,"Restarting in %d seconds...", i);
			vTaskDelay(1000 / portTICK_PERIOD_MS);
		}
		ESP_LOGI(TAG,"Restarting now.");
		esp_restart();
    }

    vTaskDelay(10000 / portTICK_PERIOD_MS);
    return ;
}

/* Create a native OTA task. It can realize download BIN file through ETH or WIFI */
void Native_Ota_Start(void)
{
	xTaskCreate(Native_Ota_Tsk, "native ota task", 4096, NULL, 5, NULL);
	ESP_LOGW(TAG, "----------------------------++++++++++++++++++++++");
}

bool diagnostic(void)
{
    gpio_config_t io_conf;
    io_conf.intr_type    = GPIO_PIN_INTR_DISABLE;
    io_conf.mode         = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Diagnostics (5 sec)...");
    vTaskDelay(5000 / portTICK_PERIOD_MS);

    bool diagnostic_is_ok = gpio_get_level(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);

    gpio_reset_pin(CONFIG_EXAMPLE_GPIO_DIAGNOSTIC);
    return diagnostic_is_ok;
}


/*void vMODB_vAnalysis_TCP_OTA(uint8_t *ucDataIn, uint16_t usDataLen, OTAModbusRtuDataTPDF *tDataOut)
{
	unsigned short j;
	tDataOut->ucEffect = MODBUS_RESULT_FAIL;
	tDataOut->usDataLen = 0;
	if(ucDataIn[2] == 0 && ucDataIn[3] == 0 && usDataLen  == ((ucDataIn[4] << 8) + (ucDataIn[5]) + 6))
	{
		tDataOut->usHead = (ucDataIn[0] << 8) + ucDataIn[1];
		if(tDataOut->usHead == 0xffff)
		{
			tDataOut->ucSlaveAddr = ucDataIn[6];
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[7];
			switch(tDataOut->tFunction)
			{
			case WriteMultReg:
				tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
				tDataOut->usRegCount = (ucDataIn[10] << 8) + ucDataIn[11];
				tDataOut->usDataLen = ucDataIn[8];
				for(j = 0; j < tDataOut->usDataLen; j++)
				{
					tDataOut->ucData[j] = ucDataIn[13 + j];
				}
				tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
				break;
			default:
				break;
			}
		}
		else
		{
			tDataOut->usRegAddr = 0x0000;
			tDataOut->usRegCount = 0x0000;
			tDataOut->usDataLen =  (ucDataIn[4] << 8) + ucDataIn[5];
			for (int j = 0; j < tDataOut->usDataLen; j++)
			{
				tDataOut->ucData[j] = ucDataIn[6 + j];
			}
			tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;

		}
	}
}

uint16_t dOTA_SpecialFunctionAnalysis(OTAModbusRtuDataTPDF *tDataIn, OTAUpdateInfoTPDF *tDataOut)
{
	uint16_t usSpecialFunction;
	uint16_t usUpdateObj;
	uint8_t  ucDevice[32];
	uint16_t usSoftVersion;
	uint16_t usHardVersion;
	uint32_t uiBinSize;
	uint16_t usEveryDataPackSize;

	if(mOTAModbusRtuDataRec->ucEffect == MODBUS_RESULT_SUCCESS)//MODBUS_RESULT_SUCCESS means MB RTU data frame analysis success
	{
		if(mOTAModbusRtuDataRec->ucSlaveAddr == 0)
		{
			if(mOTAModbusRtuDataRec->usRegAddr == 0x0000 && mOTAModbusRtuDataRec->tFunction == WriteMultReg)
			{
				mOTAModbusRtuDataRec->ucEffect = MODBUS_RESULT_FAIL;
				mOTAUpdateInfo.usSpecialFunction = mOTAModbusRtuDataRec->ucData[0] << 8 + mOTAModbusRtuDataRec->ucData[1];
				switch(	mOTAUpdateInfo.usSpecialFunction)
				{
				case 0x1000:	//start OTA, and select update object.
					mOTAUpdateInfo.usUpdateObj = mOTAModbusRtuDataRec->ucData[2] << 8 + mOTAModbusRtuDataRec->ucData[3];
					break;
				case 0x1001:	//subDevice list of needed update
					for(int i = 0;i < mOTAModbusRtuDataRec.usRegCount - 1; i++)
					{
						mOTAUpdateInfo.ucDevice[i] = mOTAModbusRtuDataRec->ucData[2 + 2 * i + 1];
					}
					break;
				case 0x1002:	//update information
					mOTAUpdateInfo.usSoftVersion = mOTAModbusRtuDataRec->ucData[2] << 8 + mOTAModbusRtuDataRec->ucData[3];
					mOTAUpdateInfo.usHardVersion = mOTAModbusRtuDataRec->ucData[4] << 8 + mOTAModbusRtuDataRec->ucData[5];
					mOTAUpdateInfo.uiBinSize = mOTAModbusRtuDataRec->ucData[6] << 24 + mOTAModbusRtuDataRec->ucData[7] << 16 +
							mOTAModbusRtuDataRec->ucData[8] << 8 + mOTAModbusRtuDataRec->ucData[9];
					mOTAUpdateInfo.usEveryDataPackSize = mOTAModbusRtuDataRec->ucData[10] << 8 + mOTAModbusRtuDataRec->ucData[11];
					break;
				case 0x1003:	//send every pack data
					break;
				case 0x1004:	//finish OTA
					break;
				default:
					break;
				}
			}
			else
			{
				ESP_LOGE(TAG, "Function or Register not support!");
			}
		}
	}
	return 0;
}

static int vData_Recv(int iSocket, char *cData, uint16_t usLen)
{
	int len = recv(iSocket, cData, usLen, MSG_NOSIGNAL);
	// Error occurred during receiving
	if (len < 0)
	{
		ESP_LOGE(TAG, "Receive start update signal failed: errno %d", errno);
		return ESP_FAIL;
	}
	else if(len == 0)
	{
		ESP_LOGW(TAG, "Connection close");
		return 0;
	}
	return len;
}

 Send TCP data frames to a file-descriptors pipe of TCP server or client
static int vData_Send(int iSocket, char *cData, uint16_t usLen)
{
	//esp_log_buffer_hex(TAG, cData, usLen);
	int iRet = send(iSocket, cData, usLen, MSG_NOSIGNAL);
	if(iRet < 0 )
	{
		ESP_LOGE(TAG, "ERROR ERROR");
	}
	return(iRet);
}


void Native_Ota_Tsk(void *pvParameter)
{
	uint8_t mConnected = 0;
	uint8_t mWifiConnected = 0;
	uint8_t uUDPServerCreated = 0;
	int uiDataLen;
	esp_err_t err;
	 update handle : set by esp_ota_begin(), must be freed via esp_ota_end()
	esp_ota_handle_t update_handle = 0 ;
	const esp_partition_t *update_partition = NULL;

	 get boot and  running parting ,and check boot and running partition
	const esp_partition_t *configured = esp_ota_get_boot_partition();
	const esp_partition_t *running = esp_ota_get_running_partition();

	if (configured != running) {
		ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
				 configured->address, running->address);
		ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
	}
	ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
			 running->type, running->subtype, running->address);

	 get next update partition
	update_partition = esp_ota_get_next_update_partition(NULL);
	ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",
			 update_partition->subtype, update_partition->address);
	assert(update_partition != NULL);

	while(1)
	{
		mConnected = Eth_Connected_Status();
		mWifiConnected = Wifi_Connected_Status();
		uUDPServerCreated = Udp_Brdcast_Status();

		if(mConnected | mWifiConnected)
		{

		}
		else
		{
			ESP_LOGE(TAG, "ETH/WIFI Network DON'T link!");
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			continue;
		}

		if(!uUDPServerCreated)
		{
			ESP_LOGE(TAG, "UDP broadcast server DON'T created, uUDPServerCreated:%d", uUDPServerCreated);
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "Starting OTA example");

		if (server_fd < 0)
		{
			 set up the tcp server socket
			server_fd = download_tcp_server();
			if(server_fd < 0)
			{
				ESP_LOGE(TAG, "TCP server create failed");
			}
			vTaskDelay(2000 / portTICK_PERIOD_MS);
			continue;
		}
		ESP_LOGI(TAG, "Native OTA tcp server already created, fd:%d", server_fd);

		client_fd = tcp_client_accept(server_fd);
		if(client_fd < 0)
		{
			ESP_LOGE(TAG, "client link TCP server failed");
			tcp_cleanup();
			ota_update_error_continue();
			continue;
		}

		uiDataLen = vData_Recv(server_fd, ota_write_data, RECV_BUF_SIZE);
		if(uiDataLen <= 0)
		{
			ESP_LOGE(TAG, "Recv start OTA signal ERROR");
			tcp_cleanup();
			ota_update_error_continue();
			continue;
		}
		vMODB_vAnalysis_TCP_OTA(ota_write_data, uiDataLen, mOTAModbusRtuDataRec);
		dOTA_SpecialFunctionAnalysis(mOTAModbusRtuDataRec, mOTAUpdateInfo);
		if(mOTAUpdateInfo.usSpecialFunction == 0x1000)
		{
			mOTAUpdateInfo.usUpdateObj = mOTAModbusRtuDataRec->ucData[2] << 8 + mOTAModbusRtuDataRec->ucData[3];
			if(mOTAUpdateInfo.usUpdateObj == 0x0000)//gateway update
			{


			}
			else if(mOTAUpdateInfo.usUpdateObj == 0x0001)//breaker update
			{

			}
			else
			{
				//update object not support
				ESP_LOGE(TAG, "Update object NOT support");

				mModbusRtuDataSend->usHead = mOTAModbusRtuDataRec->usHead;
				mModbusRtuDataSend->ucSlaveAddr = 0;
				mModbusRtuDataSend->tFunction = WriteReg;
				mModbusRtuDataSend->usRegAddr = 0x01fd;
				mModbusRtuDataSend->ucData[0] = mOTAModbusRtuDataRec->ucData[0];
				mModbusRtuDataSend->ucData[1] = mOTAModbusRtuDataRec->ucData[1];
				mModbusRtuDataSend->ucError = 0;
				i = MODB_dBuild_TCP(mModbusRtuDataSend, (uint8_t *)mDataSend);
				WIFI_vData_Send((int)mOTAModbusRtuDataRec->tTag, mDataSend, i);

				vData_Send( );
				tcp_cleanup();
				ota_update_error_continue();
				continue;

			}

		}
		else
		{
			return 0x0001;//not start update progress.
		}

	}



}*/
