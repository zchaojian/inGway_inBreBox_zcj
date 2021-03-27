/*
 * key_tsk.c
 *
 *  Created on: Jan 24, 2021
 *      Author: zchaojian
 */

/*============================================================================================
 *   带有三击+长按的按键检测
 *       单击、长按、双击、双击长按、三击（未做三击长按）
 *   
 *	 描述：	key_ucNumber:详见 enum_key_value
 *	 	 注：长按时，每隔一个长按周期，key_ucNumber被重新赋值一次长按（可用于长按时的周期性事件）	  					
 *				

	第一击长按  ________                        _________
						|______________ _ _ _   |
	单击：	    ________        ______
					    |______|
				________        ______
	第二击长按    		|______|      |____________________
	双击        ________        ______        ____________
	                    |______|      |______|
	第三击      ________        ______        _____       _____
	                    |______|      |______|     |_____|

	优化：可以将mKeyVolume改为结构体指针

 *============================================================================================*/

#include "key.h"

static const char *TAG = "key_tsk";

//消抖:在读取按键当前状态的时候进行消抖
enum_key_value ucReadKeyLevel()
{
	static uint8_t now=0;
	static uint8_t old=0;
	static enum_key_level Level_now = KEY_LEVEL_UNPUSH;
	static uint8_t usCount=0;
	const  uint8_t C_shake=3;				//消抖次数
	old = now;
	now = gpio_get_level(GPIO_KEY);
	//
	if(now==old)
	{
		usCount++;
	}
	else
	{
		usCount = 0;
	}
	if(usCount>=C_shake)					//消抖判断
	{
		usCount = 0;
		if(now == GPIO_LEVEL_PUSH)
		{
			Level_now = KEY_LEVEL_PUSH;		//消抖后的结果
		}
		else
		{
			Level_now = KEY_LEVEL_UNPUSH;	//消抖后的结果
		}
	}
	return Level_now;						//消抖中，返回之前的按键状态
}

KeyTPDF mKeyVolume;

///< 按键检测主程序
void func_mKey(KeyTPDF* key)
{
	#define C_long_push	  key->c_long_push       //长按判定（单位：运行周期）
	#define C_return 	  key->c_return          //连击判定间隔（C_return内按下即为连击）（单位：运行周期）
	#define C_long_period key->c_long_period
	uint8_t ucKeyLevel;
	if(key->ucEnable)
	{
		if((uint32_t)(key->read_level) == 0)		
		{
			return;		//函数指针为空，未初始化，直接返回
		}
		ucKeyLevel = key->read_level();				//读取按键当前状态
		switch(key->ucStatus)
		{
			case 0: //初始化
					key->ucNumber = key_none;
					key->usCount = 0;
					key->ucStatus ++;
					break;
			case 1: //等待第一次按下
					if(ucKeyLevel == KEY_LEVEL_PUSH)	
					{
						key->ucStatus ++;
                        key->ucNumber = key_first_push;
					}
					else
					{

					}
					break;
			case 2: //第一次按下
					if(ucKeyLevel == KEY_LEVEL_PUSH)	
					{
						key->usCount++;
						if(key->usCount>= C_long_push)		//长按判定-->状态3
						{
							key->usCount = 0;
							key->ucStatus = 3;
							key->ucNumber = key_first_long_trig;
						}
					}
					else							//第一次按下时间不满足长按 --> 状态4
					{
						key->ucStatus = 4;
                        key->ucNumber = key_first_return_trig;
					}
					break;
			case 3: //第一击长按
					if(ucKeyLevel == KEY_LEVEL_PUSH)	//
					{
						key->usCount++;
						if(key->usCount>= C_long_period)			//周期性长按输出
						{
							key->usCount = 0;
							key->ucNumber = key_first_long_period;
						}
					}
					else
					{
						key->usCount  = 0;
						key->ucStatus = 1;
                        key->ucNumber = key_reset;      //第一击长按后返回
					}
					break;
			case 4: //（未达成长按）第一次返回
					if(ucKeyLevel == KEY_LEVEL_PUSH)	//按下
					{
						//第二次按下
						key->ucStatus = 5;
						key->usCount=0;
                        key->ucNumber = key_second_push;
					}
					else
					{
						key->usCount++;
						if(key->usCount>= C_return)			//返回1s以上-->确认返回--.回到状态1
						{
							key->usCount = 0;
							key->ucStatus = 1;
                            key->ucNumber = key_first_returnd;
						}
					}
					break;
			case 5: //第二次按下
					if(ucKeyLevel == KEY_LEVEL_PUSH)	//按下
					{
						key->usCount++;
						if(key->usCount>= C_long_push)		//长按判定-->第二次长按
						{
							key->usCount = 0;
							key->ucStatus = 6;
                            key->ucNumber = key_second_long_trig;    //双击长按
						}
					}
					else							//未达成第二击长按-->第二击返回
					{
						key->usCount = 0;
						key->ucStatus = 7;
                        key->ucNumber = key_second_return_trig;
					}
					break;
			case 6: //第二击长按
					if(ucKeyLevel == KEY_LEVEL_PUSH)	//按下
					{
						key->usCount++;
						if(key->usCount >= C_long_period)	//周期性长按输出
						{
							key->usCount = 0;
							key->ucNumber = key_second_long_period;
						}
					}
					else							//返回-->状态1
					{
						key->usCount = 0;
						key->ucStatus = 0;
                        key->ucNumber = key_reset;   //返回状态1
					}
					break;
			case 7: //第二击返回
					if(ucKeyLevel == KEY_LEVEL_PUSH)	    //按下-->第三击按下
					{
						key->ucStatus = 8;			
						key->usCount=0;
                        key->ucNumber = key_third_push;     //按下-->第三击
					}
					else
					{
						key->usCount++;
						if(key->usCount >= C_return)
						{
							key->usCount = 0;
							key->ucStatus = 1;
                            key->ucNumber = key_second_returnd; //双击返回判定
						}
					}
					break;
			case 8: //第三击
					if(ucKeyLevel == KEY_LEVEL_PUSH)	//GPIO低电平：按下
					{

					}
					else
					{		
                        key->ucNumber = key_third_returnd;  //第三击返回判断
						key->ucStatus = 1;
						key->usCount = 0;
					}
					break;
			default:break;

		}
	}
	else
	{
		key->ucNumber = key_none;
		key->usCount = 0;
		key->ucStatus = 0;
	}
}


/**
 * @brief 	a key initialization.
 * @param	void
 *
 * @return 	none.
 */
static void vKey_Init(void)
{
	///< GPIO初始化
	gpio_pad_select_gpio(GPIO_KEY);
	//gpio_set_direction(GPIO_KEY, GPIO_MODE_INPUT_OUTPUT_OD);	///<检测高电平按键时下拉
	gpio_set_direction(GPIO_KEY, GPIO_MODE_INPUT);	///<检测高电平按键时下拉
	///< 按键结构体的初始化
	mKeyVolume.read_level = ucReadKeyLevel;			//设置读取按键状态函数指针
	mKeyVolume.c_long_push = 100;					//设定长按时间设置（单位：运行周期，例：运行为10，10*100->按1s以上触发长按）
	mKeyVolume.c_long_period = 100;					//设定长按时间设置（单位：运行周期，例：运行为10，10*100->按1s以上触发长按）
	mKeyVolume.c_return = 50;						//设定连击判断间隔时间，此时间内连击有效（单位：运行周期）
	mKeyVolume.ucEnable = 1;						//使能
}

/**
 * @brief 	a key task to realize more functions.
 * @param	*pvParameters 	primary parameter.
 *
 * @return	none.
 */
#if 1
///< 按键测试
void vKey_task(void *pvParameters)
{
	uint8_t x;
	vKey_Init();
	ESP_LOGI("KEY","key_init");

	uint32_t uiFunction = 0;
	uiFunction = (uint32_t)mPartitionTable.usFunctionH << 16U | (uint32_t) mPartitionTable.usFunctionL;
	//
	while(1)
	{
		vTaskDelay(1);
		func_mKey(&mKeyVolume);
		switch(mKeyVolume.ucNumber)
        {
            case key_none: //无
                
                break;
            case key_first_push: //按下
                ESP_LOGI("KEY","key_first_push");
                break;
            case key_first_long_trig: //
                ESP_LOGI("KEY","key_first_long_trig");
                break;
			case key_first_long_period: //
                ESP_LOGI("KEY","key_first_long_period");
                /*if ((uiFunction & (0x3 << 10)) >> 10 == 0x01)// 0x3 << 10 take WIFI state
                {
                	mPartitionTable.SmartConfigEnable = WIFI_SC_ENABLE;
                	CFG_vSaveConfig(PARTITION_TABLE_ADDR_GATEWAY_SMART_CONFIG_ENABLE - 1);
                	esp_wifi_stop();
                	vSmartConfig_vInit();
                	esp_restart();
                }*/
                //esp_restart();
                break;
            case key_first_return_trig:
                ESP_LOGI("KEY","key_first_return_trig");
                break;
            case key_first_returnd:
                ESP_LOGI("KEY","key_first_returnd");
                break;
            case key_second_push:
                ESP_LOGI("KEY","key_second_push");
                break;
            case key_second_long_trig:
                ESP_LOGI("KEY","key_second_long_trig");
                esp_restart();
                break;
			case key_second_long_period:
				ESP_LOGI("KEY","key_second_long_period");
                break;
            case key_second_return_trig:
                ESP_LOGI("KEY","key_second_return_trig");
                break;
            case key_second_returnd:
                ESP_LOGI("KEY","key_second_returnd");
                break;
            case key_third_push:
                ESP_LOGI("KEY","key_third_push");
                break;
            case key_third_returnd:
                ESP_LOGI("KEY","key_third_returnd");
                if ((uiFunction & (0x3 << 10)) >> 10 == 0x01)// 0x3 << 10 take WIFI state
                {
                	mPartitionTable.ReserveConfig.SmartConfigEnable = WIFI_SC_ENABLE;
                	CFG_vSaveConfig(PARTITION_TABLE_ADDR_GATEWAY_SMART_CONFIG_ENABLE - 1);
                	esp_wifi_stop();
                	//vSmartConfig_vInit();
                	esp_restart();
                }
                break;
            case key_reset:
                ESP_LOGI("KEY","key_reset");
                break;
            default:
                ESP_LOGI("KEY"," Error: %d",mKeyVolume.ucNumber );
                break;
        }
        if(mKeyVolume.ucNumber != key_none)
        {
            mKeyVolume.ucNumber = key_none;
        }
	}
}

#endif
