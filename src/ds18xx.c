/*
 * ds18xx.c
 *
 *  Created on: 5 Şub 2022
 *      Author: neraiv
 */

#include "ds18xx.h"
#include "one_wire.h"

// Scratchpad locations
#define TEMP_LSB        0
#define TEMP_MSB        1
#define HIGH_ALARM_TEMP 2
#define LOW_ALARM_TEMP  3
#define CONFIGURATION   4
#define INTERNAL_BYTE   5
#define COUNT_REMAIN    6
#define COUNT_PER_C     7
#define SCRATCHPAD_CRC  8

// Device resolution
#define TEMP_9_BIT  0x1F //  9 bit
#define TEMP_10_BIT 0x3F // 10 bit
#define TEMP_11_BIT 0x5F // 11 bit
#define TEMP_12_BIT 0x7F // 12 bit

//Device command list
#define READ_ROM            0x33  //if there is no device connected , tell device to send its ROM
#define CONVERT_T           0x44  //Tells device the convert temperature.(This conversion time changes according to resolution.See setResolution for more detail)
#define WRITE_SCRATCHPAD    0x4E  //Write to device scratchpad
#define READ_SCRATCHPAD     0xBE  //Read from scratchpad
#define COPY_SCRATCHPAD     0x48  //Copy scratch pad to the EEPROM .So it wont forget
#define RECALL_SCRATHCPAD   0xB8  //Recall from EEPROM to scrathcpad
#define READ_POWER_SUPPLY   0xB4

//If device disconnected or something horrible happened send 127 as temperature
#define DEVICE_DISCONNECTED 0x7F
uint16_t convertTime = 100;

//
// Sets a device high and low temperature alarm.
// When DS18 sensor reaches higher value than high temp or lower value than low temp
// sensor sets its alarm flag. If user search for alarm search
// (one_wire_search(deviceAddress,MODE_CONDITIONAL)) onyl the devices which alarm flag set will
// respond.
//
void setAlarmTemp(DeviceAddress deviceAddress,int8_t highAlarmTemp,int8_t lowAlarmTemp)
{
	// make sure the alarm temperature is within the device's range
	if (highAlarmTemp > 125)
		highAlarmTemp = 125;
	else if (highAlarmTemp < -55)
		highAlarmTemp = -55;

	if (lowAlarmTemp > 125)
		lowAlarmTemp = 125;
	else if (lowAlarmTemp < -55)
		lowAlarmTemp = -55;

	ScratchPad scratchPad;
	if (isConnected(deviceAddress, scratchPad)) {
		if(scratchPad[HIGH_ALARM_TEMP]==highAlarmTemp && scratchPad[LOW_ALARM_TEMP] == lowAlarmTemp) return;
		scratchPad[HIGH_ALARM_TEMP] = (uint8_t) highAlarmTemp;
		scratchPad[LOW_ALARM_TEMP] = (uint8_t) lowAlarmTemp;
		writeScratchPad(deviceAddress, scratchPad);
	}
}

// Reads temperature in C
// This functions can be used in a situation which there is only one device connceted.
// U don't have to define DeviceAddress.
// Returns 127 if reading fails or device disconnected.
//
float getTemperature_OneDevice(){
	uint8_t i,cfg;
	uint16_t raw;
	ScratchPad scratchpad;
	float temp_in_C;

	if(onewire_reset()){            //Reset the one wire and see if there is device(s) connected.
		HAL_Delay(1);
		onewire_skipROM();          //Skip ROM address.
		onewire_write(CONVERT_T);   //Say convert temperature to the selected device. This conversion time changes depending on resolution.

		HAL_Delay(convertTime);     //This delay must change according to resolution

		if(onewire_reset()){
			HAL_Delay(1);
			onewire_skipROM();
			onewire_write(READ_SCRATCHPAD);  //Read scratchpad has 9 byte data.
			for(i=0;i<9;i++){
				scratchpad[i] = onewire_read();
			}
			raw = (scratchpad[1]<<8)|scratchpad[0];

			cfg = scratchpad[4] & 0x60;

			if(cfg == 0x00) {raw = raw & ~7; convertTime = 100;  }    // 9 bit resolution
			else if(cfg == 0x20) {raw = raw & ~3; convertTime = 200;  } //10 bit resolution
			else if(cfg == 0x40) {raw = raw & ~1;convertTime = 400;  } //11 bit resolution
			else convertTime = 750;
			// otherwise it is 12 bit resolution

			temp_in_C = (float)raw/16;
		}
	}
	else{
		temp_in_C = (float)DEVICE_DISCONNECTED; //
	}
	return temp_in_C;
}
//
// Gets a device temperature value.Returns 127 if reading fails or device disconnected.
//
float getTemperature(DeviceAddress ROM)
{
	uint8_t i,cfg;
	uint16_t raw;
	ScratchPad scratchpad;
	float temp_in_C;

	if(onewire_reset()){            //Reset the one wire and see if there is device(s) connected.
		HAL_Delay(1);
		onewire_selectROM(ROM);        //Select the specific ROM address.
		onewire_write(CONVERT_T);   //Say convert temperature to the selected device. This conversion time changes depending on resolution.

		HAL_Delay(convertTime);     //This delay must change according to resolution

		if(onewire_reset()){
			HAL_Delay(1);
			onewire_selectROM(ROM);
			onewire_write(READ_SCRATCHPAD);  //Read scratchpad has 9 byte data.
			for(i=0;i<9;i++){
				scratchpad[i] = onewire_read();
			}
			raw = (scratchpad[1]<<8)|scratchpad[0];

			cfg = scratchpad[4] & 0x60;

			if(cfg == 0x00) {raw = raw & ~7; convertTime = 100;  }    // 9 bit resolution
			else if(cfg == 0x20) {raw = raw & ~3; convertTime = 200;  } //10 bit resolution
			else if(cfg == 0x40) {raw = raw & ~1;convertTime = 400;  } //11 bit resolution
			else convertTime = 750;
			// otherwise it is 12 bit resolution

			temp_in_C = (float)raw/16;
		}
	}
	else{
		temp_in_C = (float)DEVICE_DISCONNECTED; //
	}
	return temp_in_C;
}

//
// Sets the devices resulations
//
void setResolution(uint8_t newResolution,const uint8_t deviceCount)
{
	uint8_t newValue;
	DeviceAddress deviceAddress;

	ScratchPad scratchPad;
	for(uint8_t i = 0;i<deviceCount;i++){

		onewire_search(deviceAddress,1);

		if(isConnected(deviceAddress,scratchPad)){
			switch(newResolution){
			   case 12:
				   newValue = TEMP_12_BIT;
				   break;
			   case 11:
				   newValue = TEMP_11_BIT;
				   break;
			   case 10:
			   	   newValue = TEMP_10_BIT;
	    		   break;
			   case 9:
			   	   newValue = TEMP_9_BIT;
	    		   break;
			}

			if (scratchPad[CONFIGURATION] != newValue){
				scratchPad[CONFIGURATION] = newValue;
			    writeScratchPad(deviceAddress, scratchPad);
			}
		}
	}
	onewire_reset_search();
}

//
// Write 9 bytes scratchPad to do given deviceAddress.
//
void writeScratchPad(DeviceAddress deviceAddress,ScratchPad scratchPad)
{
	onewire_reset();
	onewire_selectROM(deviceAddress);
	onewire_write_bit(WRITE_SCRATCHPAD);
	onewire_write(scratchPad[HIGH_ALARM_TEMP]);
	onewire_write(scratchPad[LOW_ALARM_TEMP]);
	onewire_write(scratchPad[CONFIGURATION]);
	onewire_write(COPY_SCRATCHPAD);
}

//
// Checks if the device at given address is on the line.
//
uint8_t isConnected(DeviceAddress deviceAddress,ScratchPad scratchPad)
{
	uint8_t b = readScratchpad(deviceAddress, scratchPad);
	return b && onewire_crc8(scratchPad, 8);
}

//
// Reads 9 bytes scratchPad
//
uint8_t readScratchpad(DeviceAddress deviceAddress,ScratchPad scratchPad)
{
	if(onewire_reset()){
		onewire_selectROM(deviceAddress);
		onewire_write(READ_SCRATCHPAD);
		for(uint8_t i = 0;i<9;i++){
			scratchPad[i] = onewire_read();
		}
		return 1;
	}
	else return 0;
}
