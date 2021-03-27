/*
 * key.h
 *
 *  Created on: Feb 2, 2021
 *      Author: zchaojian
 */

#ifndef COMPONENTS_BLINK_INCLUDE_KEY_H_
#define COMPONENTS_BLINK_INCLUDE_KEY_H_


#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"
#include "config_app.h"
#include "wifi_tsk.h"

//---------- 设定 --------------
#define 	GPIO_KEY 		36				//按键使用的IO口
#define  	GPIO_LEVEL_PUSH	1				//按下时的电平
//按键结构体的初始化，暂时放到了main()中

	//-----------------------------
typedef enum{
	KEY_LEVEL_PUSH,		//按下
	KEY_LEVEL_UNPUSH	//未按下
}enum_key_level;		//定义按键的 按下/未按下 状态

typedef enum{
    key_none,					//无
    key_first_push,				//第一击 按下
    key_first_long_trig,		//第一击 长按触发
	key_first_long_period,		//第一击 长按后的周期性输出（例：长按3s后判断为长按，长按中每隔1s输出key_first_long_perid）
    key_first_return_trig,		//第一击 返回触发（不确定是要“返回” 还是要“连击” ）
    key_first_returnd,			//第一击 确认返回
    key_second_push,			//第二击 按下
    key_second_long_trig,		//第二击 长按
	key_second_long_period,		//第二击 长按后的周期性输出（例：长按3s后判断为长按，长按中每隔1s输出key_first_long_perid）
    key_second_return_trig,		//第二击 返回触发（不确定是要“返回” 还是要“连击” ）
    key_second_returnd,			//第二击 确认返回
    key_third_push,				//第三击 按下
    key_third_returnd,			//第三击 确认返回
    key_reset               	//长按后复位，一般用于长按检测后的返回
}enum_key_value;


typedef struct{
	uint8_t  	    ucEnable;			///<public 使能
	uint8_t 	    ucStatus;			///<static 状态：按键运行状态机
	enum_key_value	ucNumber;			///<public 按键值：用于读取按键值
	enum_key_value(*read_level)(void);	///<public 读取按键状态（按下/未按下，消抖程序请放到这个API里面）
	uint16_t 	    usCount;			///<static 按下计数
	uint16_t 		c_long_push;		///<public 长按触发判定（单位：运行周期）
	uint16_t		c_long_period;		///<public 长按时的周期性事件
	uint16_t 		c_return;			///<public 连击判定间隔（C_return内按下即为连击）（单位：运行周期）
}KeyTPDF;								///<单击功能按键


extern void vKey_task(void *pvParameters);

#endif /* COMPONENTS_BLINK_INCLUDE_KEY_H_ */
