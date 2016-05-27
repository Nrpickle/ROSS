/*
 * config.h
 *
 * Created: 5/26/2016 10:13:41 AM
 *  Author: nrpic_000
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include "projectHeader.h"

//'System' Class Functions
void configureIO();
void configureExternalOscillator();
void configure32MhzInternalOsc();
void configureTimerCounter();
void configureRTC();
void configureADCs();
void configureXCL();
uint8_t ReadCalibrationByte( uint8_t index );

volatile uint8_t broadcastStatus = 0;
volatile uint64_t longCounter = 0;

#endif /* CONFIG_H_ */