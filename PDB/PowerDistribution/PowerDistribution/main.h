/*
 * main.h
 *
 * Created: 12/30/2015 1:45:45 PM
 *  Author: Nick McComb [nickmccomb.net]
 */ 


#ifndef MAIN_H_
#define MAIN_H_

#include "projectHeader.h"

//Function Prototypes
int16_t sampleTempSensorVoltage(void);
int16_t sampleBatteryVoltage(void);
double ADCCountToVoltage(uint16_t adcCount);

//'User' Class Functions
double getEBoxTemperature();
double getElectronicsBatteryVoltage();
double getSystemCurrent(uint8_t currentSelect);

//EXTERNAL functions (not defined in MAIN)

//config
extern void configureIO();
extern void configureExternalOscillator();
extern void configure32MhzInternalOsc();
extern void configureTimerCounter();
extern void configureRTC();
extern void configureADCs();
extern void configureXCL();
extern uint8_t ReadCalibrationByte( uint8_t index );
//usart
extern void configureUSART();
extern void SendStringPC(char *stufftosend);
extern void SendNumPC(uint16_t numToSend);
extern void SendFloatPC(double numToSend);

//Global Variables used
extern volatile uint8_t broadcastStatus;
extern volatile uint64_t longCounter;


//DEBUGGING
//The following are debugging options available

//#define OUTPUT_TEMP_SENSOR_VOLTAGE

#endif /* MAIN_H_ */