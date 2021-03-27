/*
 * can_svc.h
 *
 *  Created on: Jul 21, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_CAN_SVC_H_
#define EXAMPLES_WG_WG_MAIN_CAN_SVC_H_

#include "ingwaycomm.h"

typedef struct
{
	unsigned short int usBaudrateH;
	unsigned short int usBaudrateL;
	unsigned short int usReserver[4];
}CanConfigTPDF;

#endif /* EXAMPLES_WG_WG_MAIN_CAN_SVC_H_ */
