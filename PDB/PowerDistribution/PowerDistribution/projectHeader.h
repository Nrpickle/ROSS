/*
 * projectHeader.h
 *
 * Created: 12/30/2015 1:59:06 PM
 *  Author: Nick McComb
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

//Global Variables *gasp*


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

#endif /* PROJECTHEADER_H_ */