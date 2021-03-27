/*
 * myqueue.h
 *
 *  Created on: Jul 27, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_MYQUEUE_H_
#define EXAMPLES_WG_WG_MAIN_MYQUEUE_H_

#include "freertos/queue.h"
#include "modbus_svc.h"
#define CONFIG_QUEUE_DATA_MAX_LEN		128

typedef enum
{
	MYQUEUE_SOURCE_COMM1 = 1,
	MYQUEUE_SOURCE_COMM2 = 2,
	MYQUEUE_SOURCE_CAN = 3,
	MYQUEUE_SOURCE_ETHERNET = 4,
	MYQUEUE_SOURCE_WIFI = 5,
	MYQUEUE_SOURCE_IOT = 6,
}MyQueueSourceTPDF;

typedef struct
{
	ModbusFunctionTPDF tMyQueueCommandType;
	uint16_t usSlaveAddr;
	uint16_t usRegAddr;
	uint16_t usRegLen;
	uint16_t usDatalen;
	char cData[CONFIG_QUEUE_DATA_MAX_LEN];
}MyQueueCommandTPDF;

typedef struct
{
	MyQueueSourceTPDF tMyQueueSource;
	void *vTag;
	char *cSourceBuff;
	uint16_t usSourceBuffLen;
	MyQueueCommandTPDF tMyQueueCommand;
	uint32_t Reserver[4];
}MyQueueDataTPDF;

#endif /* EXAMPLES_WG_WG_MAIN_MYQUEUE_H_ */
