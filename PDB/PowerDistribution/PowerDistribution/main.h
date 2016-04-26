/*
 * main.h
 *
 * Created: 12/30/2015 1:45:45 PM
 *  Author: Nick McComb [nickmccomb.net]
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "projectHeader.h"
#include "usartROSS.h"

//Function Prototypes
//'System' Class Functions
void configureIO();
void configureExternalOscillator();
void configure32MhzInternalOsc();
void configureTimerCounter();
void configureADCs();
static uint8_t ReadCalibrationByte( uint8_t index );
int16_t sampleTempSensorVoltage(void);
int16_t sampleBatteryVoltage(void);
double ADCCountToVoltage(uint16_t adcCount);

//'User' Class Functions
double getEBoxTemperature();
double getElectronicsBatteryVoltage();
double getSystemCurrent(uint8_t currentSelect);


/*
TEMP SENSOR SELECT
Select a temperature sensor that the board is using.
The first sensor used was the 36, while the 37 has a smaller range but is more precise.
*/
#define TMP36
//#define TMP37

#endif /* MAIN_H_ */