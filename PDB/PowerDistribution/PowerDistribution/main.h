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
uint16_t inline getXTendRSSI();

//'System' Class Functions
void inline debuggingOutput();

//EXTERNAL functions (not defined in MAIN)

//config
extern void configureIO();
extern void configureExternalOscillator();
extern void configure32MhzInternalOsc();
extern void configureTimerCounter();
extern void configureRTC();
extern void configureADCs();
extern void configureXCL();
extern void configureUSART();
extern uint8_t ReadCalibrationByte( uint8_t index );
//usart
extern void SendStringPC(char *stufftosend);
extern void SendCharPC(char charToSend);
extern void SendCharONOFF(char charToSend);
extern void SendNumPC(uint16_t numToSend);
extern void SendFloatPC(double numToSend);

//Global Variables used
extern volatile uint8_t broadcastStatus;
extern volatile uint64_t longCounter;
extern volatile uint64_t longTemp;
extern volatile uint16_t steeringPWMPeriod;


//Main program defines
#define RSSI_MAX_COUNT 273   //Calculated to have a max of 272.629 (32.768*.00832)
#define STATIC_STATUS_OUTPUT_COUNT 5	//Used to determine how many times thins like "remote output"
										//are outputted before the information "expires"


//DEBUGGING
//The following are debugging options available

//#define PWM_DEBUGGING_OUTPUT
//#define RSSI_DEBUGGING_OUTPUT
//#define OUTPUT_TEMP_SENSOR_VOLTAGE

#endif /* MAIN_H_ */