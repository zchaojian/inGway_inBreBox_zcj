/*
 * sntp_svc.h
 *
 *  Created on: Aug 8, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_SNTP_SVC_H_
#define EXAMPLES_WG_WG_MAIN_SNTP_SVC_H_

#define CONFIG_DEFAULT_SNTP_TIMEOUT			10

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sntp.h"

typedef void (*SntpCompleteCallBackTPDF)(struct tm *tResult);

extern void SNTP_vInit(SntpCompleteCallBackTPDF tCallback);

#endif /* EXAMPLES_WG_WG_MAIN_SNTP_SVC_H_ */
