/*
 * modbus_svc.c
 *
 *  Created on: Jul 24, 2020
 *      Author: liwei
 */

#include "modbus_svc.h"

static const char *TAG = "Modbus_Service";

const uint8_t ucCRCHiTbl[] =
{0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01,
0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01,
0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81,
0x40};
const uint8_t ucCRCLoTbl[] =
{0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4,
0x04, 0xCC, 0x0C, 0x0D, 0xCD, 0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 0x1E, 0xDE, 0xDF, 0x1F, 0xDD,
0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7,
0x37, 0xF5, 0x35, 0x34, 0xF4, 0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 0xEB, 0x2B, 0x2A, 0xEA, 0xEE,
0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2,
0x62, 0x66, 0xA6, 0xA7, 0x67, 0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 0x78, 0xB8, 0xB9, 0x79, 0xBB,
0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91,
0x51, 0x93, 0x53, 0x52, 0x92, 0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 0x99, 0x59, 0x58, 0x98, 0x88,
0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80,
0x40};

uint16_t dCrcCheck(unsigned char *ucData,unsigned short usLen)
{
  uint8_t ucCRCHi=0xff;
  uint8_t ucCRCLo=0xff;
  uint16_t usIndex;
  while(usLen--)
  {
    usIndex = ucCRCHi ^ (*ucData++);
    ucCRCHi = ucCRCLo ^ ucCRCHiTbl[usIndex];
    ucCRCLo = ucCRCLoTbl[usIndex];
  }
  return (ucCRCHi << 8 | ucCRCLo);
}

/* combine structure data to a modbus data frame */
uint8_t MODB_dBuild(ModbusRtuDataTPDF *tDataIn, unsigned char *ucDataOut)
{
	unsigned char i;
	unsigned short usCrc;
	ucDataOut[0] = tDataIn->ucSlaveAddr;
	ucDataOut[1] = tDataIn->tFunction;
	ESP_LOGW(TAG, "Fuction code: 0x%02x", tDataIn->tFunction);
	if(tDataIn->tModbusMstSlvMode == MODBUS_MODE_MASTER)
	{
		switch (tDataIn->tFunction)
		{
			case ReadCoil:
			case ReadInput:
			case ReadInputReg:
			case ReadReg:
				ucDataOut[2] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[3] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[4] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[5] = tDataIn->usRegCount & 0x00ff;
				usCrc = dCrcCheck(ucDataOut,6);
				ucDataOut[6] = (usCrc >> 8) & 0x00ff;
				ucDataOut[7] = usCrc & 0x00ff;
				return(8);
			case WriteCoil:
			case WriteReg:
				ucDataOut[2] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[3] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[4] = tDataIn->ucData[0];
				ucDataOut[5] = tDataIn->ucData[1];
				usCrc = dCrcCheck(ucDataOut,6);
				ucDataOut[6] = (usCrc >> 8) & 0x00ff;
				ucDataOut[7] = usCrc & 0x00ff;
				return(8);
			case WriteMultReg:
				ucDataOut[2] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[3] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[4] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[5] = tDataIn->usRegCount & 0x00ff;
				ucDataOut[6] = tDataIn->usDataLen;
				for(i = 0; i < tDataIn->usDataLen; i++)
				{
					ucDataOut[7 + i] = tDataIn->ucData[i];
				}
				usCrc = dCrcCheck(ucDataOut,7 + i);
				ucDataOut[7 + i] = (usCrc >> 8) & 0x00ff;
				ucDataOut[8 + i] = usCrc & 0x00ff;
				return(9 + i);
			case ReportSlaveID:
				usCrc = dCrcCheck(ucDataOut, 2);
				ucDataOut[2] = (usCrc >> 8) & 0x00ff;
				ucDataOut[3] = usCrc & 0x00ff;
				return(4);
			default:
				ESP_LOGE(TAG, "MB Master mode Function code NOT support! %s, %d", /*tDataIn->tFunction,*/ __func__,__LINE__);
				return(0);
		}
	}
	if(tDataIn->ucError)
	{
		ucDataOut[1] = tDataIn->tFunction | 0x80;
		ucDataOut[2] = tDataIn->ucError;
		usCrc = dCrcCheck(ucDataOut,3);
		ucDataOut[3] = (usCrc >> 8) & 0x00ff;
		ucDataOut[4] = usCrc & 0x00ff;
		return(5);
	}
	else
	{
		switch(tDataIn->tFunction)
		{
			case ReadCoil:
			case ReadInput:
			case ReadReg:
			case ReadInputReg:
				ucDataOut[2] = tDataIn->usDataLen & 0x00ff;
				for(i = 0; i < tDataIn->usDataLen; i++)
				{
					ucDataOut[3 + i] =  tDataIn->ucData[i];
				}
				usCrc = dCrcCheck(ucDataOut,i + 3);
				ucDataOut[i + 3] = (usCrc >> 8) & 0x00ff;
				ucDataOut[i + 4] = usCrc & 0x00ff;
				return(i + 5);
			case WriteCoil:
			case WriteReg:
				ucDataOut[2] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[3] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[4] = tDataIn->ucData[0];
				ucDataOut[5] = tDataIn->ucData[1];
				usCrc = dCrcCheck(ucDataOut,6);
				ucDataOut[6] = (usCrc >> 8) & 0x00ff;
				ucDataOut[7] = usCrc & 0x00ff;
				return(8);
			case WriteMultReg:
				ucDataOut[2] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[3] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[4] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[5] = tDataIn->usRegCount & 0x00ff;
				usCrc = dCrcCheck(ucDataOut,6);
				ucDataOut[6] = (usCrc >> 8) & 0x00ff;
				ucDataOut[7] = usCrc & 0x00ff;
				return(8);
			case ReportSlaveID:
				usCrc = dCrcCheck(ucDataOut, 2);
				ucDataOut[2] = (usCrc >> 8) & 0x00ff;
				ucDataOut[3] = usCrc & 0x00ff;
				return(4);
			default:
				ESP_LOGE(TAG, "MB Other mode Function code 0x%02x NOT support! %s, %d", tDataIn->tFunction, __func__,__LINE__);
				return(0);
		}
	}
	return(0);
}

void MODB_vAnalysis_Pubilc(uint8_t *ucDataIn, uint16_t usDatalen, ModbusRtuDataTPDF *tDataOut)
{
	unsigned short i,j;
	tDataOut->ucEffect = MODBUS_RESULT_FAIL;
	tDataOut->usDataLen = 0;
	for(i = 0; i < usDatalen; i++)
	{
		tDataOut->ucSlaveAddr = ucDataIn[i];
		if(tDataOut->tModbusMstSlvMode == MODBUS_MODE_MASTER && usDatalen - i >= 5)
		{
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[i + 1];
			switch(tDataOut->tFunction)
			{
				case ReadCoil:
				case ReadInput:
				case ReadReg:
				case ReadInputReg:
					tDataOut->usDataLen = ucDataIn[i + 2];
					j = dCrcCheck(&ucDataIn[i],tDataOut->usDataLen + 3);
					if(j == ((ucDataIn[i + tDataOut->usDataLen + 3] << 8) + ucDataIn[i + tDataOut->usDataLen + 4]))
					{
						for (int j = 0; j < tDataOut->usDataLen; j++)
						{
							tDataOut->ucData[j] = ucDataIn[i + 3 + j];
						}
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case WriteCoil:
				case WriteReg:
					if (usDatalen - i >= 8)
					{
						j = dCrcCheck(&ucDataIn[i],6);
						if (j == ((ucDataIn[i + 6] << 8) + (ucDataIn[i + 7])))
						{
							tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
							tDataOut->ucData[0] = ucDataIn[i + 4];
							tDataOut->ucData[1] = ucDataIn[i + 5];
							tDataOut->usDataLen = 2;
							tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
							return;
						}
					}
					break;
				case WriteMultReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->usRegCount = (ucDataIn[i + 4] << 8) + ucDataIn[i + 5];
					j = dCrcCheck(&ucDataIn[i],6);
					if (j == ((ucDataIn[i + 6] << 8) + (ucDataIn[i + 7])))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case ReportSlaveID:
					j = dCrcCheck(&ucDataIn[i], 2);
					if(j == ((ucDataIn[i + 2] << 8) + ucDataIn[i + 3]))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				default:
					if(tDataOut->tFunction & 0x80)
					{
						j = dCrcCheck(&ucDataIn[i],3);
						if (j == ((ucDataIn[i + 3] << 8) + (ucDataIn[i + 4])))
						{
							tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
							tDataOut->ucError = ucDataIn[i + 2];
							return;
						}
					}
					ESP_LOGE(TAG, "MB Master mode Function code 0x%02x NOT support! %s, %d", tDataOut->tFunction, __func__,__LINE__);
					break;
			}
		}
		else if(tDataOut->tModbusMstSlvMode == MODBUS_MODE_SLAVE && usDatalen - i >= 8)
		{
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[i + 1];
			switch(tDataOut->tFunction)
			{
				case ReadCoil:
				case ReadInput:
				case ReadReg:
				case ReadInputReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->usRegCount = (ucDataIn[i + 4] << 8) + ucDataIn[i + 5];
					j = dCrcCheck(&ucDataIn[i],6);
					if(j == ((ucDataIn[i + 6] << 8) + ucDataIn[i + 7]))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case WriteCoil:
				case WriteReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->ucData[0] = ucDataIn[i + 4];
					tDataOut->ucData[1] = ucDataIn[i + 5];
					tDataOut->usDataLen = 2;
					j = dCrcCheck(&ucDataIn[i],6);
					if(j == ((ucDataIn[i + 6] << 8) + ucDataIn[i + 7]))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case WriteMultReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->usRegCount = (ucDataIn[i + 4] << 8) + ucDataIn[i + 5];
					tDataOut->usDataLen = ucDataIn[i + 6];
					j = dCrcCheck(&ucDataIn[i],7 + ucDataIn[i + 6]);
					if(j == ((ucDataIn[i + tDataOut->usDataLen + 7] << 8) + ucDataIn[i + tDataOut->usDataLen + 8]))
					{
						for(j = 0; j < tDataOut->usDataLen; j++)
						{
							tDataOut->ucData[j] = ucDataIn[i + j + 7];
						}
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case ReportSlaveID:
					j = dCrcCheck(&ucDataIn[i], 2);
					if(j == ((ucDataIn[i + 2] << 8) + ucDataIn[i + 3]))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				default:
					ESP_LOGE(TAG, "MB Slave mode Function code 0x%02x NOT support! %s, %d", tDataOut->tFunction, __func__,__LINE__);
					return;
			}
		}
	}
}

/* analysis Modbus RTU data frames to ModbusRtuDataTPDF structure data */
void MODB_vAnalysis(uint8_t *ucDataIn, uint16_t usDatalen, ModbusRtuDataTPDF *tDataOut)
{
	unsigned short i,j;
	tDataOut->ucEffect = MODBUS_RESULT_FAIL;
	tDataOut->usDataLen = 0;
	esp_log_buffer_hex(TAG, ucDataIn, usDatalen);
	for(i = 0; i < usDatalen; i++)
	{
		ESP_LOGW(TAG, "i:%d data:%d", i, ucDataIn[i]);
		if(tDataOut->ucSlaveAddr != ucDataIn[i])
		{
			continue;
		}
		if(tDataOut->tModbusMstSlvMode == MODBUS_MODE_MASTER && usDatalen - i >= 5)
		{
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[i + 1];
			ESP_LOGW(TAG, "M tFuction code : 0x%02x", ucDataIn[i + 1]);
			switch(tDataOut->tFunction)
			{
				case ReadCoil:
				case ReadInput:
				case ReadReg:
				case ReadInputReg:
					tDataOut->usDataLen = ucDataIn[i + 2];
					j = dCrcCheck(&ucDataIn[i],tDataOut->usDataLen + 3);
					if(j == ((ucDataIn[i + tDataOut->usDataLen + 3] << 8) + ucDataIn[i + tDataOut->usDataLen + 4]))
					{
						for (int j = 0; j < tDataOut->usDataLen; j++)
						{
							tDataOut->ucData[j] = ucDataIn[i + 3 + j];
						}
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case WriteCoil:
				case WriteReg:
					if (usDatalen - i >= 8)
					{
						j = dCrcCheck(&ucDataIn[i],6);
						if (j == ((ucDataIn[i + 6] << 8) + (ucDataIn[i + 7])))
						{
							tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
							tDataOut->ucData[0] = ucDataIn[i + 4];
							tDataOut->ucData[1] = ucDataIn[i + 5];
							tDataOut->usDataLen = 2;
							tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
							return;
						}
					}
					break;
				case WriteMultReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->usRegCount = (ucDataIn[i + 4] << 8) + ucDataIn[i + 5];
					j = dCrcCheck(&ucDataIn[i],6);
					if (j == ((ucDataIn[i + 6] << 8) + (ucDataIn[i + 7])))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case ReportSlaveID:
					tDataOut->usDataLen = ucDataIn[i + 2];
					j = dCrcCheck(&ucDataIn[i],3 + ucDataIn[i + 2]);
					ESP_LOGW(TAG, "CRC j = 0x%04x", j);
					if(j == ((ucDataIn[i + tDataOut->usDataLen + 3] << 8) + ucDataIn[i + tDataOut->usDataLen + 4]))
					{
						for(j = 0; j < tDataOut->usDataLen; j++)
						{
							tDataOut->ucData[j] = ucDataIn[i + j + 3];
						}
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					return;
				default:
					if(tDataOut->tFunction & 0x80)
					{
						j = dCrcCheck(&ucDataIn[i],3);
						if (j == ((ucDataIn[i + 3] << 8) + (ucDataIn[i + 4])))
						{
							tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
							tDataOut->ucError = ucDataIn[i + 2];
							return;
						}
					}
					ESP_LOGE(TAG, "MB Master mode Function code 0x%02x NOT support! %s, %d", tDataOut->tFunction, __func__,__LINE__);
					break;
			}
		}
		else if(tDataOut->tModbusMstSlvMode == MODBUS_MODE_SLAVE && usDatalen - i >= 8)
		{
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[i + 1];
			ESP_LOGW(TAG, "S tFuction code : 0x%02x", tDataOut->tFunction);
			switch(tDataOut->tFunction)
			{
				case ReadCoil:
				case ReadInput:
				case ReadReg:
				case ReadInputReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->usRegCount = (ucDataIn[i + 4] << 8) + ucDataIn[i + 5];
					j = dCrcCheck(&ucDataIn[i],6);
					if(j == ((ucDataIn[i + 6] << 8) + ucDataIn[i + 7]))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case WriteCoil:
				case WriteReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->ucData[0] = ucDataIn[i + 4];
					tDataOut->ucData[1] = ucDataIn[i + 5];
					tDataOut->usDataLen = 2;
					j = dCrcCheck(&ucDataIn[i],6);
					if(j == ((ucDataIn[i + 6] << 8) + ucDataIn[i + 7]))
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case WriteMultReg:
					tDataOut->usRegAddr = (ucDataIn[i + 2] << 8) + ucDataIn[i + 3];
					tDataOut->usRegCount = (ucDataIn[i + 4] << 8) + ucDataIn[i + 5];
					tDataOut->usDataLen = ucDataIn[i + 6];
					j = dCrcCheck(&ucDataIn[i],7 + ucDataIn[i + 6]);
					if(j == ((ucDataIn[i + tDataOut->usDataLen + 7] << 8) + ucDataIn[i + tDataOut->usDataLen + 8]))
					{
						for(j = 0; j < tDataOut->usDataLen; j++)
						{
							tDataOut->ucData[j] = ucDataIn[i + j + 7];
						}
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					break;
				case ReportSlaveID:
					tDataOut->usDataLen = ucDataIn[i + 2];
					j = dCrcCheck(&ucDataIn[i],3 + ucDataIn[i + 2]);
					ESP_LOGW(TAG, "CRC j = 0x%04x", j);
					if(j == ((ucDataIn[i + tDataOut->usDataLen + 3] << 8) + ucDataIn[i + tDataOut->usDataLen + 4]))
					{
						for(j = 0; j < tDataOut->usDataLen; j++)
						{
							tDataOut->ucData[j] = ucDataIn[i + j + 3];
						}
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						return;
					}
					return;
				default:
					ESP_LOGE(TAG, "MB Slave mode Function code 0x%02x NOT support! %s, %d", tDataOut->tFunction, __func__,__LINE__);
					return;
			}
		}
	}
}

/* Combine ModbusRtuDataTPDF structure data into Modbus TCP data frames */
uint8_t MODB_dBuild_TCP(ModbusRtuDataTPDF *tDataIn, unsigned char *ucDataOut)
{
	unsigned char i;
	unsigned short j;
	ucDataOut[0] = (tDataIn->usHead >> 8) & 0x00ff;
	ucDataOut[1] = tDataIn->usHead & 0x00ff;
	ucDataOut[2] = 0;
	ucDataOut[3] = 0;
	ucDataOut[6] = tDataIn->ucSlaveAddr;
	ucDataOut[7] = tDataIn->tFunction;
	//esp_log_buffer_hex(TAG, tDataIn->ucData, tDataIn->usDataLen);
	if(tDataIn->tModbusMstSlvMode == MODBUS_MODE_MASTER)
	{
		switch (tDataIn->tFunction)
		{
			case ReadCoil:
			case ReadInput:
			case ReadInputReg:
			case ReadReg:
				ucDataOut[8] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[9] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[10] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[11] = tDataIn->usRegCount & 0x00ff;
				ucDataOut[4] = 0;
				ucDataOut[5] = 6;
				return(12);
			case WriteCoil:
			case WriteReg:
				ucDataOut[8] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[9] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[10] = tDataIn->ucData[0];
				ucDataOut[11] = tDataIn->ucData[1];
				ucDataOut[4] = 0;
				ucDataOut[5] = 6;
				return(12);
			case WriteMultReg:
				ucDataOut[8] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[9] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[10] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[11] = tDataIn->usRegCount & 0x00ff;
				ucDataOut[12] = tDataIn->usDataLen;
				for(i = 0; i < tDataIn->usDataLen; i++)
				{
					ucDataOut[13 + i] = tDataIn->ucData[i];
				}
				j = i + 7;
				ucDataOut[4] = (j >> 8) & 0x00ff;;
				ucDataOut[5] = j & 0x00ff;
				return(j + 6);
			case ReportSlaveID:
				ucDataOut[4] = 0;
				ucDataOut[5] = 2;
				return(8);
			default:
				ESP_LOGE(TAG, "MB Master mode Function code 0x%02x NOT support! %s, %d", tDataIn->tFunction, __func__,__LINE__);
				return(0);
		}
	}
	else
	{
		if(tDataIn->ucError)
		{
			ucDataOut[7] = tDataIn->tFunction | 0x80;
			ucDataOut[8] = tDataIn->ucError;
			ucDataOut[4] = 0;
			ucDataOut[5] = 3;
			return(9);
		}
		switch(tDataIn->tFunction)
		{
			case ReadCoil:
			case ReadInput:
			case ReadReg:
			case ReadInputReg:
				ucDataOut[8] = tDataIn->usDataLen & 0x00ff;
				for(i = 0; i < tDataIn->usDataLen; i++)
				{
					ucDataOut[9 + i] =  tDataIn->ucData[i];
				}
				j = i + 3;
				ucDataOut[4] = (j >> 8) & 0x00ff;;
				ucDataOut[5] = j & 0x00ff;
				return(j + 6);
			case WriteCoil:
			case WriteReg:
				ucDataOut[8] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[9] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[10] = tDataIn->ucData[0];
				ucDataOut[11] = tDataIn->ucData[1];
				ucDataOut[4] = 0;
				ucDataOut[5] = 6;
				return(12);
			case WriteMultReg:
				ucDataOut[8] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[9] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[10] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[11] = tDataIn->usRegCount & 0x00ff;
				ucDataOut[4] = 0;
				ucDataOut[5] = 6;
				return(12);
			case ReportSlaveID:
				ucDataOut[8] = tDataIn->usDataLen & 0x00ff;
				for(i = 0; i < tDataIn->usDataLen; i++)
				{
					ucDataOut[9 + i] = tDataIn->ucData[i];
				}
				j = i + 3;
				ucDataOut[4] = (j >> 8) & 0x00ff;;
				ucDataOut[5] = j & 0x00ff;
				return(j + 6);
			case MultiDeviceCtrl:
				ucDataOut[8] = (tDataIn->usRegAddr >> 8) & 0x00ff;
				ucDataOut[9] = tDataIn->usRegAddr & 0x00ff;
				ucDataOut[10] = (tDataIn->usRegCount >> 8) & 0x00ff;
				ucDataOut[11] = tDataIn->usRegCount & 0x00ff;
				ucDataOut[12] = tDataIn->usDataLen & 0x00ff;
				for(i = 0; i < tDataIn->usDataLen; i++)
				{
					ucDataOut[13 + i] = tDataIn->ucData[i];
				}
				j = i + 7;
				ucDataOut[4] = (j >> 8) & 0x00ff;;
				ucDataOut[5] = j & 0x00ff;
				return (j + 6);
			default:
				ESP_LOGE(TAG, "MB Other mode Function code 0x%02x NOT support! %s, %d", tDataIn->tFunction, __func__,__LINE__);
				return(0);
		}
	}
	return(0);
}

/* Analyse receive data frames from ETH, and set analysize result to ModbusRtuDataTPDF variables */
void MODB_vAnalysis_TCP(uint8_t *ucDataIn, uint16_t usDatalen, ModbusRtuDataTPDF *tDataOut)
{
	unsigned short j;
	tDataOut->ucEffect = MODBUS_RESULT_FAIL;
	tDataOut->usDataLen = 0;
	if(ucDataIn[2] == 0 && ucDataIn[3] == 0 && usDatalen  == ((ucDataIn[4] << 8) + (ucDataIn[5]) + 6)) // Don't support
	{
		tDataOut->usHead = (ucDataIn[0] << 8) + ucDataIn[1];
		//ESP_LOGW(TAG, "tDataOut->tModbusMstSlvMode:%d", tDataOut->tModbusMstSlvMode);
		if(tDataOut->tModbusMstSlvMode == MODBUS_MODE_MASTER)
		{
			tDataOut->ucSlaveAddr = ucDataIn[6];
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[7];
			switch(tDataOut->tFunction)
			{
				case ReadCoil:
				case ReadInput:
				case ReadReg:
				case ReadInputReg:
					tDataOut->usDataLen = ucDataIn[8];
					for (int j = 0; j < tDataOut->usDataLen; j++)
					{
						tDataOut->ucData[j] = ucDataIn[9 + j];
					}
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case WriteCoil:
				case WriteReg:
					tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
					tDataOut->ucData[0] = ucDataIn[10];
					tDataOut->ucData[1] = ucDataIn[11];
					tDataOut->usDataLen = 2;
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case WriteMultReg:
					tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
					tDataOut->usRegCount = (ucDataIn[10] << 8) + ucDataIn[11];
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case ReportSlaveID:
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case MultiDeviceCtrl:
					tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
					tDataOut->usRegCount = (ucDataIn[10] << 8) + ucDataIn[11];
					tDataOut->usDataLen = ucDataIn[8];
					for (int j = 0; j < tDataOut->usDataLen; j++)
					{
						tDataOut->ucData[j] = ucDataIn[9 + j];
					}
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					break;
				default:
					if(tDataOut->tFunction & 0x80)
					{
						tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
						tDataOut->ucError = ucDataIn[8];
						return;
					}
					ESP_LOGE(TAG, "MB Master mode Function code 0x%02x NOT support! %s, %d", tDataOut->tFunction, __func__,__LINE__);
					break;
			}
		}
		else if(tDataOut->tModbusMstSlvMode == MODBUS_MODE_SLAVE)
		{
			tDataOut->ucSlaveAddr = ucDataIn[6];
			tDataOut->tFunction = (ModbusFunctionTPDF)ucDataIn[7];
			switch(tDataOut->tFunction)
			{
				case ReadCoil:
				case ReadInput:
				case ReadReg:
				case ReadInputReg:
					tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
					tDataOut->usRegCount = (ucDataIn[10] << 8) + ucDataIn[11];
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case WriteCoil:
				case WriteReg:
					tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
					tDataOut->ucData[0] = ucDataIn[10];
					tDataOut->ucData[1] = ucDataIn[11];
					tDataOut->usDataLen = 2;
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case WriteMultReg:
				case MultiDeviceCtrl:
					tDataOut->usRegAddr = (ucDataIn[8] << 8) + ucDataIn[9];
					tDataOut->usRegCount = (ucDataIn[10] << 8) + ucDataIn[11];
					tDataOut->usDataLen = ucDataIn[12];
					for(j = 0; j < tDataOut->usDataLen; j++)
					{
						tDataOut->ucData[j] = ucDataIn[13 + j];
					}
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
					return;
				case ReportSlaveID:
					tDataOut->ucEffect = MODBUS_RESULT_SUCCESS;
				return;
				default:
					ESP_LOGE(TAG, "MB Slave mode Function code 0x%02x NOT support! %s, %d", tDataOut->tFunction, __func__,__LINE__);
					return;
			}
		}
	}
}
