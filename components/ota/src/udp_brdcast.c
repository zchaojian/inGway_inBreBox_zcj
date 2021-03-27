/* 1.Realize UDP broadcast to make others discover inGway information and linking
 * information.
 * 2.Need connect ETH or WIFI, before Call this function.
*/
#include "udp_brdcast.h"

#define HOST_IP_ADDR INADDR_BROADCAST

#define BROADCAST_PORT 52077

#define B_CLASS_NAME_LEN 		8
#define B_CLASS_CODE_LEN 		2
#define B_DIP_TYPE_LEN			4
#define B_VERSION_NUM_LEN 		4
#define B_STATE_OF_RUN_LEN		1
#define B_SERIAL_NUM_LEN 		8
#define B_PORT_OF_DOWNLOAD_LEN 	5

int udpSocket = -1;
static uint8_t uUDPServerCreated = 0;

static const char *TAG = "Udp_Broadscat";
/* name		indx
 * UUUU-DIP	0	: 	UUUU class name
 * 34 		8	: 	UUUU class code
 * 1123 	10	:	DIP type
 * 0101 	14	:	version number
 * F 		18	:	state of run
 * 00001100 19	:	product of serial number
 * 22222 	27	: 	port of download file
 *
 *
 * intotal 		:	32
 */
static char cReportBroadcastPort[B_CLASS_NAME_LEN + B_CLASS_CODE_LEN + B_DIP_TYPE_LEN + \
									   B_VERSION_NUM_LEN + B_STATE_OF_RUN_LEN + B_SERIAL_NUM_LEN + \
									   B_PORT_OF_DOWNLOAD_LEN];

static InGwayBroadcastFrameTPDF mInGwayBroadcastFrame;
static uint8_t ucNewReportBroadcastPort[66] = {0};
static uint8_t ucBrdFrame[66] = {
		0x00, 0x01,
		0x00, 0x00,
		0x00, 0x3c,
		0x00,
		0x11,
		0x39,
		0x01, 0x07, 0x01, 0x01,
		0x01,
		0x10, 0x09,
		0x10, 0x01,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

		0x32, 0x32, 0x32, 0x32, 0x32, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};//66 bytes 0x42 bytes

void Version_Cal(char *cVersionIn, char *cVersionOut)
{
	char cVersionLong[32];
    char cVersionShort[4];
    uint8_t ucHi[2];
    uint8_t ucLo[2];
    int iDotIndex = 0;
    int iEndIndex = 0;
    uint16_t usHiValue;
    uint16_t usLoValue;

    memcpy(cVersionLong, cVersionIn, 32);

    /* search dot index and end index in string version */
	for (int i = 0; (cVersionLong[i] != '\0') && (i < 5); i++)
	{
	    if (cVersionLong[i] == '.')
	    {
	        iDotIndex = i;
	    }

	    iEndIndex = i;
	}

	if (iDotIndex > 0)
	{
		/* according to dot index, transmit High Value bit */
	    switch (iDotIndex)
	    {
	    case 5:
	        usHiValue = (cVersionLong[0] - '0') * 10000 + (cVersionLong[1] - '0') * 1000 + (cVersionLong[2] - '0') * 100 + (cVersionLong[3] - '0') * 10 + (cVersionLong[4] - '0');
	        break;
	    case 4:
	        usHiValue = (cVersionLong[0] - '0') * 1000 + (cVersionLong[1] - '0') * 100 + (cVersionLong[2] - '0') * 10 + (cVersionLong[3] - '0');
	        break;
	    case 3:
	        usHiValue = (cVersionLong[0] - '0') * 100 + (cVersionLong[1] - '0') * 10 + (cVersionLong[2] - '0');
	        break;
	    case 2:
	        usHiValue = (cVersionLong[0] - '0') * 10 + (cVersionLong[1] - '0');
	        break;
	    case 1:
	        usHiValue = cVersionLong[0] - '0';
	        break;
	    default:usHiValue = 0;
	    }
	    ESP_LOGD(TAG, "usHiValue = %d", usHiValue);

	    if( usHiValue < 10)
	    {
	    	ucHi[0] = usHiValue >> 8U;
	    	ucHi[1] = usHiValue & 0xFF;
	    }
	    else if (usHiValue < 100)
	    {
	    	ucHi[0] = usHiValue / 10;
	    	ucHi[1] = usHiValue % 10;
	    }
	    else
	    {
	    	ESP_LOGE(TAG, "version number error!");
	    }

	    ESP_LOGD(TAG, "ucHi[0] = %d ucHi[1] = %d", ucHi[0], ucHi[1]);
	    ESP_LOGD(TAG, "ucHi[0] = 0x%02X ucHi[1] = 0x%02X", ucHi[0], ucHi[1]);

	    /* according to dot index, transmit Low Value bit */
	    ESP_LOGD(TAG, " end-dot = %d", iEndIndex - iDotIndex);
	    switch (iEndIndex - iDotIndex)
	    {
	    case 5:
	        usLoValue = (cVersionLong[iDotIndex + 1] - '0') * 10000 + (cVersionLong[iDotIndex + 2] - '0') * 1000 + (cVersionLong[iDotIndex + 3] - '0') * 100 + (cVersionLong[iDotIndex + 4] - '0') * 10 + (cVersionLong[iDotIndex + 5] - '0');
	        break;
	    case 4:
	        usLoValue = (cVersionLong[iDotIndex + 1] - '0') * 1000 + (cVersionLong[iDotIndex + 2] - '0') * 100 + (cVersionLong[iDotIndex + 3] - '0') * 10 + (cVersionLong[iDotIndex + 4] - '0');
	        break;
	    case 3:
	        usLoValue = (cVersionLong[iDotIndex + 1] - '0') * 100 + (cVersionLong[iDotIndex + 2] - '0') * 10 + (cVersionLong[iDotIndex + 3] - '0');
	        break;
	    case 2:
	        usLoValue = (cVersionLong[iDotIndex + 1] - '0') * 10 + (cVersionLong[iDotIndex + 2] - '0');
	        break;
	    case 1:
	        usLoValue = cVersionLong[iDotIndex + 1] - '0';
	        break;
	    default:usLoValue = 0;
	    }
	    ESP_LOGD(TAG, "usLoValue = %d", usLoValue);

	    if( usLoValue < 10)
	   {
	    	ucLo[0] = usLoValue >> 8U;
	    	ucLo[1] = usLoValue & 0xFF;
	   }
	   else if (usHiValue < 100)
	   {
		   ucLo[0] = usLoValue / 10;
		   ucLo[1] = usLoValue % 10;
	   }
	   else
	   {
		   ESP_LOGE(TAG, "version number error!");
	   }


	    ESP_LOGD(TAG, "ucLo[0] = %d ucLo[1] = %d", ucLo[0], ucLo[1]);
	    ESP_LOGD(TAG, "ucLo[0] = 0x%02X ucLo[1] = 0x%02X", ucLo[0], ucLo[1]);
	}
	else	//not dot in version number
	{
		/* according to dot index, transmit High Value bit */
	    switch (iEndIndex)
	    {
	    case 4:
	        usHiValue = (cVersionLong[0] - '0') * 10000 + (cVersionLong[1] - '0') * 1000 + (cVersionLong[2] - '0') * 100 + (cVersionLong[3] - '0') * 10 + (cVersionLong[4] - '0');
	        break;
	    case 3:
	        usHiValue = (cVersionLong[0] - '0') * 1000 + (cVersionLong[1] - '0') * 100 + (cVersionLong[2] - '0') * 10 + (cVersionLong[3] - '0');
	        break;
	    case 2:
	        usHiValue = (cVersionLong[0] - '0') * 100 + (cVersionLong[1] - '0') * 10 + (cVersionLong[2] - '0');
	        break;
	    case 1:
	        usHiValue = (cVersionLong[0] - '0') * 10 + (cVersionLong[1] - '0');
	        break;
	    case 0:
	        usHiValue = cVersionLong[0] - '0';
	        break;
	    default:usHiValue = 0;
	    }
	    ESP_LOGD(TAG, "usHiValue = %d", usHiValue);
	    if( usHiValue < 10)
	    {
	    	ucHi[0] = usHiValue >> 8U;
	    	ucHi[1] = usHiValue & 0xFF;
	    }
	    else if (usHiValue < 100)
	    {
	    	ucHi[0] = usHiValue / 10;
	    	ucHi[1] = usHiValue % 10;
	    }
	    else
	    {
	    	ESP_LOGE(TAG, "version number error!\n");
	    }
	    ESP_LOGD(TAG, "ucHi[0] = %d ucHi[1] = %d", ucHi[0], ucHi[1]);
	    ESP_LOGD(TAG, "ucHi[0] = 0x%02X ucHi[1] = 0x%02X", ucHi[0], ucHi[1]);

	    usLoValue = 0;
	    ESP_LOGD(TAG, "usLoValue = %d", usLoValue);
	    ucLo[0] = usLoValue >> 8U;
	    ucLo[1] = usLoValue & 0xFF;
	    ESP_LOGD(TAG, "ucLo[0] = %d ucLo[1] = %d", ucLo[0], ucLo[1]);
	    ESP_LOGD(TAG, "ucLo[0] = 0x%02X ucLo[1] = 0x%02X", ucLo[0], ucLo[1]);
	}

	/* string short version  */
	cVersionShort[0] = ucHi[0] + '0';
	cVersionShort[1] = ucHi[1] + '0';
	cVersionShort[2] = ucLo[0] + '0';
	cVersionShort[3] = ucLo[1] + '0';

	memcpy(cVersionOut, cVersionShort, 4);
}

static uint8_t ucUdp_VerCal(char *cSrc,int iDotIndex, int iVerLen)
{
    uint8_t ucVer = 0;
    switch (iVerLen)
    {
    case 1:
        ucVer = cSrc[iDotIndex - 1] - '0';
        break;
    case 2:
        ucVer = (cSrc[iDotIndex - 2] - '0') * 10 + cSrc[iDotIndex - 1] - '0';
        break;
    case 3:
        ucVer = (cSrc[iDotIndex - 3] - '0') * 100 + (cSrc[iDotIndex - 2] - '0') * 10 + cSrc[iDotIndex - 1] - '0';
        break;
    default:
        break;
    }
    return ucVer;
}

/* Calculate version number. The function mean to converter string version to uint8_ hex version */
void vUdp_VersionAnalysis(char *cVersionIn, uint8_t *ucVersionOut)
{
	char cVersionLong[32];
	int iCharVersionLen;
	int iFirstDotIndex = 0;
	int iSecondDotIndex = 0;
	int iEndIndex = 0;
	int iDotFlag = 0;
	int iFirstVerLen;
	int iSecondVerLen;
	int iLastVerLen;

	uint8_t ucFirstVer;
	uint8_t ucSecondVer;
	uint8_t ucLastVer;

	memset(cVersionLong, 0, sizeof(cVersionLong));
	memcpy(cVersionLong, cVersionIn, 32);
	iCharVersionLen = strlen(cVersionLong);

	/* search dot index and end index in string version */
	for (int i = 0; cVersionLong[i] != '\0' && (i < iCharVersionLen); i++)
	{
	    if ((cVersionLong[i] == '.') && iDotFlag == 1)
	    {
	    	iSecondDotIndex = i;
	    }
	    if ((cVersionLong[i] == '.') && iDotFlag == 0)
	    {
	    	iFirstDotIndex = i;
	        iDotFlag = 1;
	    }
	    iEndIndex = i;
	}
	iDotFlag = 0;
	iEndIndex = iEndIndex + 1;

	iFirstVerLen = iFirstDotIndex;
	iSecondVerLen = iSecondDotIndex - (iFirstDotIndex + 1);
	iLastVerLen = iEndIndex - 1 - iSecondDotIndex;

	ucFirstVer = ucUdp_VerCal(cVersionLong, iFirstDotIndex, iFirstVerLen);
	ucSecondVer = ucUdp_VerCal(cVersionLong, iSecondDotIndex, iSecondVerLen);
	ucLastVer = ucUdp_VerCal(cVersionLong, iEndIndex, iLastVerLen);

	*ucVersionOut = (ucFirstVer << 4) + ucSecondVer;
	*(ucVersionOut + 1) = ucLastVer;
}

uint8_t UdpFrame_dBuild(InGwayBroadcastFrameTPDF *tDataIn, uint8_t *ucDataOut)
{
	ucDataOut[0] = (tDataIn->ucTransactionID >> 8) & 0x00ff;
	ucDataOut[1] = (tDataIn->ucTransactionID) & 0x00ff;
	ucDataOut[2] = (tDataIn->ucProtocolID >> 8) & 0x00ff;
	ucDataOut[3] = (tDataIn->ucProtocolID) & 0x00ff;
	ucDataOut[4] = (tDataIn->ucLen >> 8) & 0x00ff;
	ucDataOut[5] = (tDataIn->ucLen) & 0x00ff;
	ucDataOut[6] = tDataIn->ucUnitID;
	ucDataOut[7] = tDataIn->ucFunctionCode;
	ucDataOut[8] = tDataIn->ucNextLen;
	memcpy(ucDataOut + 9, tDataIn->tInGwayInfo.ucDeviceType,
			sizeof(tDataIn->tInGwayInfo.ucDeviceType));//4 bytes
	ucDataOut[13] = tDataIn->tInGwayInfo.ucRunStatus;
	ucDataOut[14] = tDataIn->tInGwayInfo.tSoftwareVersion.ucFirstMidVer;
	ucDataOut[15] = tDataIn->tInGwayInfo.tSoftwareVersion.ucLastVer;
	ucDataOut[16] = tDataIn->tInGwayInfo.tHardwareVersion.ucFirstMidVer;
	ucDataOut[17] = tDataIn->tInGwayInfo.tHardwareVersion.ucLastVer;
	memcpy(ucDataOut + 18, tDataIn->tInGwayInfo.ucProductSN,
				sizeof(tDataIn->tInGwayInfo.ucProductSN));//24 bytes
	memcpy(ucDataOut + 42, tDataIn->tInGwayInfo.ucProductDescription,
				sizeof(tDataIn->tInGwayInfo.ucProductDescription));//24 bytes
	return (42 + 24);
}

/* A udp broadcast task with special execute code to send version information */
void Udp_Brdcast_vTsk(void *pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    int broadcast = 1;
    //char run_version_cal[4];
    uint8_t ucSoftwareRunVesion[2];
    uint8_t mConnected = 0;
    uint8_t mWifiConnected = 0;
    uint8_t mEthServerListened = 0;
    uint8_t mWifiServerListened = 0;
    char *cProductSN = "20201215150500";
    char *cProductDescription = "22222";
    uint8_t ucBrdLen;

    memcpy(cReportBroadcastPort, "UUUU-DIP3411230101F0000110022222", 32);

    /* version get from esp_app_desc_t structure */
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_app_desc_t running_app_info;
    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK)
    {

        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
    }

    //Version_Cal(running_app_info.version, run_version_cal);

    vUdp_VersionAnalysis(running_app_info.version, ucSoftwareRunVesion);
    ESP_LOGW(TAG, "run version:");
    esp_log_buffer_hex(TAG, ucSoftwareRunVesion, sizeof(ucSoftwareRunVesion));

    memset(&mInGwayBroadcastFrame, 0, sizeof(mInGwayBroadcastFrame));
    mInGwayBroadcastFrame.ucTransactionID = 0x0001;
    mInGwayBroadcastFrame.ucProtocolID = 0x0000;
    mInGwayBroadcastFrame.ucLen = 0x3c;
    mInGwayBroadcastFrame.ucFunctionCode = 0x11;
    mInGwayBroadcastFrame.ucNextLen = 0x39;
    mInGwayBroadcastFrame.tInGwayInfo.ucDeviceType[0] = 0x01;
    mInGwayBroadcastFrame.tInGwayInfo.ucDeviceType[1] = 0x07;
    mInGwayBroadcastFrame.tInGwayInfo.ucDeviceType[2] = 0x01;
    mInGwayBroadcastFrame.tInGwayInfo.ucDeviceType[3] = 0x01;
    mInGwayBroadcastFrame.tInGwayInfo.ucRunStatus = 0x01;
    mInGwayBroadcastFrame.tInGwayInfo.tSoftwareVersion.ucFirstMidVer = ucSoftwareRunVesion[0];
    mInGwayBroadcastFrame.tInGwayInfo.tSoftwareVersion.ucLastVer = ucSoftwareRunVesion[1];
    mInGwayBroadcastFrame.tInGwayInfo.tHardwareVersion.ucFirstMidVer = 0x10;
    mInGwayBroadcastFrame.tInGwayInfo.tHardwareVersion.ucLastVer = 0x01;
    memcpy(mInGwayBroadcastFrame.tInGwayInfo.ucProductSN, cProductSN,
    		strlen(cProductSN));
    memcpy(mInGwayBroadcastFrame.tInGwayInfo.ucProductDescription, cProductDescription,
    		strlen(cProductDescription));

    while (1)
    {
    	/* Destination */
        struct sockaddr_in dest_addr;
        dest_addr.sin_addr.s_addr = htonl(HOST_IP_ADDR);
        dest_addr.sin_family = AF_INET;
        dest_addr.sin_port = htons(BROADCAST_PORT);
        dest_addr.sin_len = sizeof(dest_addr);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);
        mConnected = Eth_Connected_Status();
        mWifiConnected = Wifi_Connected_Status();
        mEthServerListened = Eth_SocketServer_Status();
        mWifiServerListened = Wifi_SocketServer_Status();
        if(mConnected | mWifiConnected)
        {

        }
        else
        {
        	ESP_LOGE(TAG, "ETH/WIFI Network DON'T link!");
        	vTaskDelay(5000 / portTICK_PERIOD_MS);
        	continue;
        }

        if(mEthServerListened | mWifiServerListened)
        {

        }
        else
        {
        	ESP_LOGE(TAG, "ETH/WIFI TCP Server DON'T listen!");
        	vTaskDelay(5000 / portTICK_PERIOD_MS);
        	continue;
        }


		udpSocket = socket(addr_family, SOCK_DGRAM, ip_protocol);
		if (udpSocket < 0)
		{
			ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
			return;
		}
		ESP_LOGI(TAG, "UDP Socket created fd: %d, sending to port: %d", udpSocket, BROADCAST_PORT);

		/* set udp socket to a broadcast */
		int rslt = setsockopt(udpSocket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast));
		if(rslt < 0)
		{
			ESP_LOGE(TAG, "Error in setting broadcast option: %d.", rslt);
			close(udpSocket);

			return;
		}

		while (1)
		{
			/* add running app version */
			//memcpy(cReportBroadcastPort + B_CLASS_NAME_LEN + B_CLASS_CODE_LEN + B_DIP_TYPE_LEN, run_version_cal, 4);
			cReportBroadcastPort[32] = 0;
			ucBrdLen = UdpFrame_dBuild(&mInGwayBroadcastFrame, ucNewReportBroadcastPort);

			int err = sendto(udpSocket, ucNewReportBroadcastPort, ucBrdLen, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
			if (err < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
				uUDPServerCreated = 0;
				close(udpSocket);
				break;
			}
			//ESP_LOGI(TAG, "Message sent!");
			uUDPServerCreated = 1;

			vTaskDelay(5000 / portTICK_PERIOD_MS);
		}

		if (udpSocket != -1)
		{
			ESP_LOGE(TAG, "Shutting down socket and restarting...");
			uUDPServerCreated = 0;
			shutdown(udpSocket, 0);
			close(udpSocket);
		}
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

/* Create a udp broadcast task. It can transmit the local OTA tcp task of IP, PORT and version */
void Udp_Brdcast_Start(void)
{
	xTaskCreate(Udp_Brdcast_vTsk, "UDP broadcast task", 4096, NULL, 4, NULL);
}

void Udp_Brdcast_Stop(void)
{
	shutdown(udpSocket, 0);
	close(udpSocket);
	vTaskDelay(5000 / portTICK_PERIOD_MS);
}

/* return udp broadcast server status */
uint8_t Udp_Brdcast_Status()
{
	return uUDPServerCreated;
}
