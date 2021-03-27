/*
 * modbus_svc.h
 *
 *  Created on: Jul 24, 2020
 *      Author: liwei
 */

#ifndef EXAMPLES_WG_WG_MAIN_MODBUS_SVC_H_
#define EXAMPLES_WG_WG_MAIN_MODBUS_SVC_H_
#include <stdint.h>
#include "esp_log.h"
#define CONFIG_MODBUS_MAXSURPPORT_PARTITION		10

typedef enum
{
	MODBUS_MODE_SLAVE = 0,
	MODBUS_MODE_MASTER = 1,
}ModbusMstSlvModeTPDF;

typedef enum
{
	ReadCoil = 1,
	ReadInput = 2,
	ReadReg = 3,
	ReadInputReg = 4,
	WriteCoil = 5,
	WriteReg = 6,
	WriteMultReg = 0x10,
	ReportSlaveID = 0x11,
	MultiDeviceCtrl = 0xf8,
	BroadcastAddr = 0xf9,
	BleAdv = 0xfa,		/* BLE broadcast enable */
	DeviceDiscoveryReStart = 0xfb,
	DeviceDiscoveryContinue = 0xfc,
	DeviceDiscoveryStop = 0xfd,
	Sntp = 0xfe,
}ModbusFunctionTPDF;

typedef enum
{
	MODBUS_RESULT_FAIL = 0,	//analysis data fail
	MODBUS_RESULT_SUCCESS = 1,	//analysis data sucess
}ModbusResultTPDF;

typedef enum
{
	MODBUS_ERROR_CODE_FUNCTION = 1,
	MODBUS_ERROR_CODE_REGADDR = 2,
	MODBUS_ERROR_CODE_DATA = 3,
	MODBUS_ERROR_CODE_OTHER = 4,
}ModbusErrorCodeTPDF;

typedef enum
{
	MODBUS_PARTITION_COIL_READ = 1,
	MODBUS_PARTITION_COIL_WRITE = 2,
	MODBUS_PARTITION_REG_READ = 4,
	MODBUS_PARTITION_REG_WRITE = 8,
}ModbusParttionTableRWTPDF;

typedef struct
{
	ModbusParttionTableRWTPDF tModbusParttionTableRW;
	uint16_t usStartRegAddr;
	uint16_t usTableLen;
	uint8_t *ucTablePoint;
}ModbusPartitionTableTPDF;

typedef struct
{
	ModbusPartitionTableTPDF tModbusPartitionTable[CONFIG_MODBUS_MAXSURPPORT_PARTITION];
	uint8_t uctModbusPartitionTableLen;
	uint8_t ucSlaveAddr;
	ModbusFunctionTPDF tFunction;
	ModbusMstSlvModeTPDF tModbusMstSlvMode;
	uint16_t usRegAddr;
	uint16_t usRegCount;
	uint16_t usDataLen;
	uint16_t usHead;
	uint8_t ucData[256];
	ModbusResultTPDF ucEffect;	// data frame analysis if success
	uint8_t ucError;
	void *tTag;   //data source use fd flag
}ModbusRtuDataTPDF;

typedef struct
{
	ModbusPartitionTableTPDF tModbusPartitionTable[CONFIG_MODBUS_MAXSURPPORT_PARTITION];
	uint8_t uctModbusPartitionTableLen;
	uint8_t ucSlaveAddr;
	ModbusFunctionTPDF tFunction;
	ModbusMstSlvModeTPDF tModbusMstSlvMode;
	uint16_t usRegAddr;
	uint16_t usRegCount;
	uint16_t usDataLen;
	uint16_t usHead;
	uint8_t ucData[1024+10];
	ModbusResultTPDF ucEffect;	// data frame analysis if success
	uint8_t ucError;
	void *tTag;   //data source use fd flag
}OTAModbusRtuDataTPDF;

uint16_t dCrcCheck(unsigned char *ucData,unsigned short usLen);

extern uint8_t MODB_dBuild(ModbusRtuDataTPDF *tDataIn, unsigned char *ucDataOut);

extern void MODB_vAnalysis(uint8_t *ucDataIn, uint16_t usDatalen, ModbusRtuDataTPDF *tDataOut);

extern void MODB_vAnalysis_Pubilc(uint8_t *ucDataIn, uint16_t usDatalen, ModbusRtuDataTPDF *tDataOut);

extern uint8_t MODB_dBuild_TCP(ModbusRtuDataTPDF *tDataIn, unsigned char *ucDataOut);

extern void MODB_vAnalysis_TCP(uint8_t *ucDataIn, uint16_t usDatalen, ModbusRtuDataTPDF *tDataOut);

#endif /* EXAMPLES_WG_WG_MAIN_MODBUS_SVC_H_ */
