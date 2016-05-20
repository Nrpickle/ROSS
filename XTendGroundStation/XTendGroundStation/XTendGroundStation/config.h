/*
 * config.h
 *
 * Created: 5/18/2016 2:15:57 PM
 *  Author: nrpic_000
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include "main.h"

//'System' Class Functions
void configureIO();
void configureExternalOscillator();
void configure32MhzInternalOsc();
void configureTimerCounter();
void configureRTC();
void configureADCs();
void configureUSART();

//Timer Counter Defines
#define TC_1024_100MS  0x0C35
#define TC_1024_500MS  0x3D09
#define TC_1024_1000MS 0x7A12

//GLOBAL Variables *gasp*
extern volatile uint8_t broadcastStatus;
extern volatile uint8_t globalToggle;

#endif /* CONFIG_H_ */