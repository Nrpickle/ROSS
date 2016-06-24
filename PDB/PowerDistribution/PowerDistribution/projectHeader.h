/*
 * projectHeader.h
 *
 * Created: 12/30/2015 1:59:06 PM
 *  Author: Nick McComb | nickmccomb.net
 */ 


#ifndef PROJECTHEADER_H_
#define PROJECTHEADER_H_

#ifndef F_CPU
#define F_CPU 32000000  //Define clock speed for _delay
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "macros.h"
#include "usart_driver.h"
#include "avr_compiler.h"
#include <stddef.h>

#define COMP_USART USARTC0
#define ONOFF_USART USARTD0

/*
TEMP SENSOR SELECT
Select a temperature sensor that the board is using.
The first sensor used was the 36, while the 37 has a smaller range but is more precise.
*/
//#define TMP36
#define TMP37

/*  DEBUGGING  */
//The following are debugging options available

//#define OUTPUT_LOT_AND_WAFER_INFO
//#define PWM_DEBUGGING_OUTPUT
//#define RSSI_DEBUGGING_OUTPUT
//#define OUTPUT_TEMP_SENSOR_VOLTAGE
//#define BATT_VOLTAGE_RAW_COUNT_OUTPUT
//#define BATT_VOLTAGE_RAW_OUTPUT
//#define CURRENT_COUNT_RAW_OUTPUT
//#define CURRENT_VOLTAGE_RAW_OUTPUT



//Datatype Definitions
struct RSSI_type {
	//"User" class variables
	uint8_t value;
	
	//"System" class variables
	uint8_t measuring;
	uint16_t timeDifference;
	uint16_t countDifference;
	uint16_t sampleCount;
	uint16_t sampleCountTemp;
};



//Enumerations
enum measuring {MEASURING, NOT_MEASURING}; //Works with the RSSI interpret

//Used for generating a serial number
enum {
	LOTNUM0=8,  // Lot Number Byte 0, ASCII
	LOTNUM1,    // Lot Number Byte 1, ASCII
	LOTNUM2,    // Lot Number Byte 2, ASCII
	LOTNUM3,    // Lot Number Byte 3, ASCII
	LOTNUM4,    // Lot Number Byte 4, ASCII
	LOTNUM5,    // Lot Number Byte 5, ASCII
	WAFNUM =16, // Wafer Number
	COORDX0=18, // Wafer Coordinate X Byte 0
	COORDX1,    // Wafer Coordinate X Byte 1
	COORDY0,    // Wafer Coordinate Y Byte 0
	COORDY1,    // Wafer Coordinate Y Byte 1
};

enum {
	XTEND,  //Note, this is a different voltage reference than the rest
	COMP,
	SYS_5V,
	THROTTLE,
	REAR
};

enum {
	REAR_BATT,
	ELECTRONICS	
};

#endif /* PROJECTHEADER_H_ */