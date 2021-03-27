/*
 * sntp_svc.c
 *
 *  Created on: Aug 8, 2020
 *      Author: liwei
 */
#include "sntp_svc.h"

static TaskHandle_t mTaskHandle;
static SntpCompleteCallBackTPDF mSntpCompleteCallBack;

void vSntp_Time_Sync_Notification_Cb(struct timeval *tv)
{
}

void vSNTP_Tsk_Start(void *pvParameters)
{
	time_t tNow;
    struct tm tTimeInfo;
    uint8_t i;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(vSntp_Time_Sync_Notification_Cb);
    sntp_init();
    for(i = 0; i < CONFIG_DEFAULT_SNTP_TIMEOUT; i++)
    {
    	if(sntp_get_sync_status() != SNTP_SYNC_STATUS_RESET)
    	{
    		break;
    	}
    	vTaskDelay(100);
    }
    if(i == CONFIG_DEFAULT_SNTP_TIMEOUT)
    {
    	vTaskDelete(mTaskHandle);
    	return;
    }
    setenv("TZ", "CST-8", 1);
    tzset();
    time(&tNow);
    localtime_r(&tNow, &tTimeInfo);
    if(mSntpCompleteCallBack)
    {
    	mSntpCompleteCallBack(&tTimeInfo);
    }
    vTaskDelete(mTaskHandle);
}

void SNTP_vInit(SntpCompleteCallBackTPDF tCallback)
{
	mSntpCompleteCallBack = tCallback;
	xTaskCreate(vSNTP_Tsk_Start, "sntp task", 4096, NULL, 4, &mTaskHandle);
}

