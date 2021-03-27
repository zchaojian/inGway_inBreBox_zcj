/*
 * ingwaycomm.h
 *
 *  Created on: Jul 20, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_INGWAYCOMM_H_
#define EXAMPLES_WG_WG_MAIN_INGWAYCOMM_H_

#include <stdint.h>

#define STATE_GET(source,flag)		(source & (flag)? 1:0)
#define STATE_SET(source,flag)		(source |= (flag))
#define STATE_CLEAR(source,flag)	(source &= ~(flag))

typedef void (*DataRecCallBackTPDF)(char *cData, uint16_t usLen);
#endif /* EXAMPLES_WG_WG_MAIN_INGWAYCOMM_H_ */
