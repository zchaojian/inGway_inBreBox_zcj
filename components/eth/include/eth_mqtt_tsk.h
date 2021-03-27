/*
 * eth_mqtt_tsk.h
 *
 *  Created on: Aug 4, 2020
 *      Author: liwei
 */
#ifndef EXAMPLES_WG_WG_MAIN_ETH_MQTT_TSK_H_
#define EXAMPLES_WG_WG_MAIN_ETH_MQTT_TSK_H_
#include "device_app.h"

#define CONFIG_ETH_MQTT_DEFAULT_PUBLISH_FORMAT		"A0000R0000T00"
#define CONFIG_ETH_MQTT_DEFAULT_SUBSCRIBE_FORMAT	"A%xR%xT%x:%d:"
#define CONFIG_ETH_MQTT_DEFAULT_PUBLISH_MAX			50
#define CONFIG_ETH_MQTT_DEFAULT_PUBLISH_TICKS		200

#define CONFIG_ETH_MQTT_DEFAULT_HOST				"183.230.40.39"
#define CONFIG_ETH_MQTT_DEFAULT_PORT				6002

#define DEFINE_DEVICE								2
#define CONFIG_ETH_MQTT_DEFAULT_PRODUCT_ID			"364418"

#if DEFINE_DEVICE == 1
#define CONFIG_ETH_MQTT_DEFAULT_AUTH_INFOR			"20200803"
#define CONFIG_ETH_MQTT_DEFAULT_DEVICE_ID			"615226885"
#elif DEFINE_DEVICE == 2
#define CONFIG_ETH_MQTT_DEFAULT_AUTH_INFOR			"20200811"
#define CONFIG_ETH_MQTT_DEFAULT_DEVICE_ID			"617637799"
#endif

#define TIETA_INGWAY_NUM				2
#if TIETA_INGWAY_NUM
#define CONFIG_MQTT_PRODUCT_ID			"399602"
#endif

#if TIETA_INGWAY_NUM == 1
#define CONFIG_MQTT_AUTH_INFOR			"202101160001"
#define CONFIG_MQTT_DEVICE_ID			"670707064"
#elif TIETA_INGWAY_NUM == 2
#define CONFIG_MQTT_AUTH_INFOR			"202101160002"
#define CONFIG_MQTT_DEVICE_ID			"670707315"
#endif

//#define INGWAY_DEMO_NUM				2
#if INGWAY_DEMO_NUM
#define CONFIG_MQTT_PRODUCT_ID			"399832"
#endif

#if INGWAY_DEMO_NUM == 1
#define CONFIG_MQTT_AUTH_INFOR			"202101180001"
#define CONFIG_MQTT_DEVICE_ID			"671463292"
#elif INGWAY_DEMO_NUM == 2
#define CONFIG_MQTT_AUTH_INFOR			"202101180002"
#define CONFIG_MQTT_DEVICE_ID			"671464925"
#endif

typedef struct
{
	DevicePartitionTableDataTypeTPDF tDevicePartitionTableDataType;
	char *cFromat;
	uint8_t *ucMem;
}MqttPublishDataTPDF;

typedef struct
{
	char *cProductID;
	char *cAuthInfor;
	char *cDeviceID;
}OnenetDeviceInfoTPDF;

#endif
