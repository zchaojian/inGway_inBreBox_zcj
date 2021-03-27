/*
 * device_app.c
 *
 *  Created on: Jul 27, 2020
 *      Author: liwei
 */
#include "device_app.h"

static const char *TAG = "Device_app";

DeviceTPDF mDevice[CONFIG_DEVICE_MAXSURPPORT];

/* Through MainType to confirm DEV partition table info, and set every parameter */
void DEV_vCreat_PartitionTable_From_Example(DeviceMainTypeTPDF tMaintype, DevicePartitionTableTPDF *tDevicePartitionTable)
{
	ESP_LOGI(TAG, "tMaintype:%d", tMaintype);
	switch(tMaintype)
	{
		case DEVICE_MAIN_TYPE_TEMPRATURE:
			tDevicePartitionTable[0].usLen = 1;
			tDevicePartitionTable[0].usStartAddr = 0x0040;
			tDevicePartitionTable[0].tDevicePartitionTableRegType = ReadInputReg;
			tDevicePartitionTable[0].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_UINT;
			break;
		case DEVICE_MAIN_TYPE_CHINT_METER_MCB://NB2LE-40ZT  Yibiao
			tDevicePartitionTable[0].usLen = 18;
			tDevicePartitionTable[0].usStartAddr = 0x0b00;
			tDevicePartitionTable[0].tDevicePartitionTableRegType = ReadInputReg;
			tDevicePartitionTable[0].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_FLOAT;
			tDevicePartitionTable[1].usLen = 1;
			tDevicePartitionTable[1].usStartAddr = 0x0a04;
			tDevicePartitionTable[1].tDevicePartitionTableRegType = ReadInputReg;
			tDevicePartitionTable[1].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_UINT;

			tDevicePartitionTable[2].usLen = 1;
			tDevicePartitionTable[2].usStartAddr = 0x0900;
			tDevicePartitionTable[2].tDevicePartitionTableRegType = ReadInputReg;
			tDevicePartitionTable[2].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_UINT;
			break;
		case DEVICE_MAIN_TYPE_ROUTER:
			tDevicePartitionTable[0].usLen = 8;
			tDevicePartitionTable[0].usStartAddr = 0x3400;
			tDevicePartitionTable[0].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[0].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F2;
			break;
		case DEVICE_MAIN_TYPE_CHINT_MCB://NB2LE-80ZT  Liwei
			///<state
			tDevicePartitionTable[0].usLen = 1;
			tDevicePartitionTable[0].usStartAddr = 0x0000;
			tDevicePartitionTable[0].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[0].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_STATE;
			///<current
			tDevicePartitionTable[1].usLen = 3;
			tDevicePartitionTable[1].usStartAddr = 0x0001;
			tDevicePartitionTable[1].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[1].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F2;
			///<voltage
			tDevicePartitionTable[2].usLen = 3;
			tDevicePartitionTable[2].usStartAddr = 0x0005;
			tDevicePartitionTable[2].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[2].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_UINT_F1;
			///<fac freq
			tDevicePartitionTable[3].usLen = 2;
			tDevicePartitionTable[3].usStartAddr = 0x000b;
			tDevicePartitionTable[3].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[3].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_SINT_F2;
			///<lkc
			tDevicePartitionTable[4].usLen = 2;
			tDevicePartitionTable[4].usStartAddr = 0x000d;
			tDevicePartitionTable[4].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[4].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_SINT;
			///<power
			tDevicePartitionTable[5].usLen = 8;
			tDevicePartitionTable[5].usStartAddr = 0x000e;
			tDevicePartitionTable[5].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[5].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_SDINT;
			///<eng
			tDevicePartitionTable[6].usLen = 2;
			tDevicePartitionTable[6].usStartAddr = 0x0016;
			tDevicePartitionTable[6].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[6].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_DINT;
			break;
		case DEVICE_MAIN_TYPE_CHINT_MCB_DC:
			///<state
			tDevicePartitionTable[0].usLen = 1;
			tDevicePartitionTable[0].usStartAddr = 0x0001;
			tDevicePartitionTable[0].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[0].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_STATE;
			///<voltage
			tDevicePartitionTable[1].usLen = 2;
			tDevicePartitionTable[1].usStartAddr = 0x0002;
			tDevicePartitionTable[1].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[1].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F2;
			///<current
			tDevicePartitionTable[2].usLen = 2;
			tDevicePartitionTable[2].usStartAddr = 0x0004;
			tDevicePartitionTable[2].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[2].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F3;
			///<power
			tDevicePartitionTable[3].usLen = 2;
			tDevicePartitionTable[3].usStartAddr = 0x0006;
			tDevicePartitionTable[3].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[3].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F1;
			///<eng
			tDevicePartitionTable[4].usLen = 2;
			tDevicePartitionTable[4].usStartAddr = 0x0008;
			tDevicePartitionTable[4].tDevicePartitionTableRegType = ReadReg;
			tDevicePartitionTable[4].tDevicePartitionTableDataType = DEVICE_PARTITION_TABLE_DATA_TYPE_DINT_F3;
			break;
		default:
			break;
	}
}

uint16_t *DEV_dGet_Mem(DeviceTPDF *tDev, uint16_t usAddr, uint16_t usSize)
{
	DevicePartitionTableTPDF *j;
	if(!tDev)
	{
		return(0);
	}
	for(uint16_t i = 0; i < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; i++)
	{
		j = &tDev->tDevicePartitionTable[i];
		if(usAddr >= j->usStartAddr
		   &&((usAddr + usSize) <= (j->usStartAddr + j->usLen)))
		{
			return(j->usMemory + usAddr - j->usStartAddr);
		}
	}
	return(0);
}

DeviceTPDF *DEV_dFind_By_ConvertAddr(uint16_t usAddr)
{
	if(!usAddr)
	{
		return(0);
	}
	for(uint8_t i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		/* judge added device addr is same with addr in received data frame? */
		if(mDevice[i].tDeviceConfig.usConvertAddr == usAddr)
		{
			ESP_LOGI(TAG, "usAddr:0x%02x", usAddr);
			//ESP_LOGI(TAG, "mDevice[%x].tDeviceConfig.usConvertAddr:0x%02x", i,  mDevice[i].tDeviceConfig.usConvertAddr);
			//ESP_LOGI(TAG, "mDevice[%x].tDeviceConfig.usCommPort:%d", i, mDevice[i].tDeviceConfig.usCommPort);
			//ESP_LOGI(TAG, "mDevice[i].tDeviceConfig.tAddr:%s", mDevice[i].tDeviceConfig.tAddr);
			//ESP_LOGI(TAG, "mDevice[%x].tDeviceConfig.SecondType:%s", i, mDevice[i].tDeviceConfig.SecondType);
			ESP_LOGI(TAG, "mDevice[%x].tDeviceConfig.usMainType:%d", i, mDevice[i].tDeviceConfig.usMainType);
			return(&mDevice[i]);
		}
	}
	ESP_LOGI(TAG, "usAddr:0x%02x, Device addr don't add to \ndevice list! Only passthrough", usAddr);
	return(0);
}

void DEV_vClear(void)
{
	uint8_t i,j;
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		mDevice[i].tDeviceConfig.usConvertAddr = 0;
		for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
		{
				mDevice[i].tDevicePartitionTable[j].usLen = 0;
		}
	}
}

void DEV_vInit(void)
{
	uint8_t i,j;
	for(i = 0; i < CONFIG_DEVICE_MAXSURPPORT; i++)
	{
		if(mDevice[i].tDeviceConfig.usConvertAddr != 0)
		{
			DEV_vCreat_PartitionTable_From_Example(mDevice[i].tDeviceConfig.usMainType, mDevice[i].tDevicePartitionTable);
			for(j = 0; j < CONFIG_DEVICE_MAXSURPPORT_PARTITION_TABLES; j++)
			{
				if(mDevice[i].tDevicePartitionTable[j].usLen)
				{
					memset(mDevice[i].tDevicePartitionTable[j].usMemory, 0, CONFIG_DEVICE_PARTITION_TABLE_LEN * 2);
				}
			}
		}
	}
}
