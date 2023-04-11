/*
 * ds18xx.h
 *
 *  Created on: 5 Şub 2022
 *      Author: neraiv
 */

#ifndef INC_DS18XX_H_
#define INC_DS18XX_H_


//#ifdef __STM32F4xx_HAL_H
//#include "stm32f4xx_hal.h"
//#endif
//
//#ifdef __STM32F1xx_HAL_H
//#include "stm32f1xx_hal.h"
//#endif

#include "main.h"
#include "one_wire.h"

typedef uint8_t ScratchPad[9];

// Reads temperature in C
// This functions can be used in a situation which there is only one device connceted.
// U don't have to define DeviceAddress.
// Returns 127 if reading fails or device disconnected.
//
float getTemperature_OneDevice();

//
// Gets a device temperature value. Returns 127 if reading fails or device disconnected.
//
float getTemperature(uint8_t *ROM);

//
// Sets a device high and low temperature alarm.
// When DS18 sensor reaches higher value than high temp or lower value than low temp
// sensor sets its alarm flag. If user search for alarm search
// (one_wire_search(deviceAddress,MODE_CONDITIONAL)) onyl the devices which alarm flag set will
// respond.
//
void setAlarmTemp(DeviceAddress deviceAddress,int8_t highAlarmTemp,int8_t lowAlarmTemp);

//
// Sets the devices resulations
//
void setResolution(uint8_t newResolution,const uint8_t deviceCount);

//
//Check if the device connected. This func also read scratchpad
//
uint8_t isConnected(DeviceAddress deviceAddress,ScratchPad scratchPad);

//
// Only reads scratchpad from given deviceAddress
//
uint8_t readScratchpad(DeviceAddress deviceAddress,ScratchPad scratchPad);

//
// Write 9 bytes scratchPad to given deviceAddress.
//
void writeScratchPad(DeviceAddress deviceAddress,ScratchPad scratchPad);

#endif /* INC_DS18XX_H_ */
