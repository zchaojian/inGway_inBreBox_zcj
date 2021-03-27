/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "config_app.h"
#include "eth_tsk.h"
#include "wifi_tsk.h"
#include "blink.h"
/* Can run 'make menuconfig' to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO 12

static char *TAG = "blink";

/*
 * inGw DI, DO GPIO
 */
#define GPIO_DI1 34
#define GPIO_DI2 35
#define GPIO_DO1 32
#define GPIO_DO2 33

//#define GPIO_KEY 36

const char *TIP = "DI/DO";

SemaphoreHandle_t xDI1ToAFDDBinSemph;

void vLed_Flash(eLedDisplayState tTaskState)
{
	switch(tTaskState)
	{
	case LED_ESP_TOUCH_CONFIG:
		gpio_set_level(BLINK_GPIO, 0);
		vTaskDelay(1000/ portTICK_PERIOD_MS);

		//gpio_set_level(BLINK_GPIO, 1);
		//vTaskDelay(1000 / portTICK_PERIOD_MS);
		break;
	case LED_WIFI_NOT_CONNECT:
	case LED_ETH_NOT_CONNECT:
		gpio_set_level(BLINK_GPIO, 0);
		vTaskDelay(200 / portTICK_PERIOD_MS);

		gpio_set_level(BLINK_GPIO, 1);
		vTaskDelay(200 / portTICK_PERIOD_MS);
		break;
	case GW_NORMAL_STATE:
		gpio_set_level(BLINK_GPIO, 0);
		vTaskDelay(1000 / portTICK_PERIOD_MS);

		gpio_set_level(BLINK_GPIO, 1);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		break;
	default:
		break;

	}
}

void sytem_blink_task()
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */
    gpio_pad_select_gpio(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    /*gpio_pad_select_gpio(GPIO_KEY);
    ///Set the GPIO as a push/pull output
    gpio_set_direction(GPIO_KEY, GPIO_MODE_INPUT);*/

    uint32_t uiFunction = 0;
    uiFunction = (uint32_t)mPartitionTable.usFunctionH << 16U | (uint32_t) mPartitionTable.usFunctionL;
    eLedDisplayState tTaskState;
    uint8_t mWifiConnected = 0;
    uint8_t mConnected = 0;

    while(1)
    {
        /* Blink off (output low) */
    	/*if(gpio_get_level(GPIO_KEY) == 0)//0: low ; 1: high
    	{
    		//ESP_LOGI("blink", "++++++++++++0+++++++");
    	}*/
    	//printf("Turning off the LED\n");
    	//gpio_set_level(BLINK_GPIO, 0);
    	//vTaskDelay(1000 / portTICK_PERIOD_MS);

        /* Blink on (output high) */
    	/*if(gpio_get_level(GPIO_KEY) == 1)//0: low ; 1: high
    	{
    		//ESP_LOGI("blink", "++++++++++++1+++++++");
    		if ((uiFunction & (0x3 << 10)) >> 10 == 0x01)// 0x3 << 10 take WIFI state
    		{
    			mPartitionTable.SmartConfigEnable = WIFI_SC_ENABLE;
    			CFG_vSaveConfig(PARTITION_TABLE_ADDR_GATEWAY_SMART_CONFIG_ENABLE - 1);
    			esp_wifi_stop();
    			vSmartConfig_vInit();
    		}
    	}*/
    	//printf("Turning on the LED\n");
    	//gpio_set_level(BLINK_GPIO, 1);
    	//vTaskDelay(1000 / portTICK_PERIOD_MS);
    	mWifiConnected = Wifi_Connected_Status();
    	mConnected = Eth_Connected_Status();
    	tTaskState = 7;
    	if(mPartitionTable.ReserveConfig.SmartConfigEnable == WIFI_SC_ENABLE)
    	{
    		tTaskState = LED_ESP_TOUCH_CONFIG;
    	}
    	else if(((uiFunction & (0x3 << 10)) >> 10 == 0x01) && (mWifiConnected == 0))
    	{
    		tTaskState = LED_WIFI_NOT_CONNECT;
    	}
    	else if(((uiFunction & (0x3 << 6)) >> 6 == 0x01) && (mConnected == 0))
    	{
    		tTaskState = LED_ETH_NOT_CONNECT;
    	}
    	else /*if(mWifiConnected == 1)*/
    	{
    		tTaskState = GW_NORMAL_STATE;
    	}
    	//ESP_LOGW(TAG, "tTaskState:%d",tTaskState);
    	vLed_Flash(tTaskState);

    	//save inGateway connected status
    	if (((uiFunction & (0x3 << 10)) >> 10 == 0x01) && (mWifiConnected != mPartitionTable.BasicInfor.RunState))// 0x3 << 10 take WIFI state
    	{
    		mPartitionTable.BasicInfor.RunState = mWifiConnected;
    		CFG_vSaveConfig(0x020C - 1);
    	}
    	else if(((uiFunction & (0x3 << 6)) >> 6 == 0x01) && mConnected != mPartitionTable.BasicInfor.RunState)// 0x3 << 6 take ETH state
    	{
    		mPartitionTable.BasicInfor.RunState = mConnected;
    		CFG_vSaveConfig(0x020C - 1);
    	}
    }
}

void DI_DO_task()
{
    /* Configure the IOMUX register for pad BLINK_GPIO (some pads are
       muxed to GPIO on reset already, but some default to other
       functions and need to be switched to GPIO. Consult the
       Technical Reference for a list of pads and their default
       functions.)
    */

	/* Set the GPIO as a push/pull output */
    gpio_pad_select_gpio(GPIO_DI1);
    gpio_set_direction(GPIO_DI1, GPIO_MODE_INPUT);
    /* Set the GPIO as a push/pull output */
    gpio_pad_select_gpio(GPIO_DI2);
    gpio_set_direction(GPIO_DI2, GPIO_MODE_INPUT);

    /* Set the GPIO as a push/pull output */
    gpio_pad_select_gpio(GPIO_DO1);
    gpio_set_direction(GPIO_DO1, GPIO_MODE_OUTPUT);
    /* Set the GPIO as a push/pull output */
    gpio_pad_select_gpio(GPIO_DO2);
    gpio_set_direction(GPIO_DO2, GPIO_MODE_OUTPUT);

    xDI1ToAFDDBinSemph = xSemaphoreCreateBinary();

    while(1)
    {
    	int iDI1State;
    	//int iDI2State;

        iDI1State = gpio_get_level(GPIO_DI1);
        if(iDI1State == 0)
        {
        	if(xDI1ToAFDDBinSemph != NULL)
        	{
        		//xSemaphoreTake( xSemaphore, xBlockTime );
        		xSemaphoreGive( xDI1ToAFDDBinSemph );
        	}
        	else
        	{
        		ESP_LOGE(TIP, "Semaphore xDI1ToAFDDBinSemph not exist");
        	}

        }
        else if(iDI1State == 1)
        {
        	ESP_LOGI(TIP, "DI Wait trigger level");
        }
        else
        {
        	ESP_LOGE(TIP, "DI 1 State ERROR");
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
