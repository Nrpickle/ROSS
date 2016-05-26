/** 

  Ground Station Main
  
  Written by Nick McComb
  May 2016
  
**/

//#ifndef MAIN_H_
//#define MAIN_H_

#ifndef F_CPU
#define F_CPU 32000000  //Define clock speed for _delay
//#define F_CPU 4000000
#endif

#define COMP_USART USARTC0

#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "macros.h"
#include "config.h"
#include "usart_helper.h"

#ifdef __cplusplus
extern "C" {
#endif
	#include "usart_driver.h"
#ifdef __cplusplus
}
#endif

//RSSI Interpret Settings
#define RSSI_READ_TIMEOUT 10000 //uS
#define RSSI_DEBOUNCE_DELAY 20   //uS

//Function Prototypes

//static uint8_t ReadCalibrationByte( uint8_t index );
void processRSSI();
void outputSignalStrength(uint8_t str); //Accepts 0, 1, 2, or 3

//#endif /* MAIN_H_ */