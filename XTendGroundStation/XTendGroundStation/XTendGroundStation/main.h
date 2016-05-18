/** 

  Ground Station Main
  
  Written by Nick McComb
  May 2016
  
**/

#include "macros.h"

//Function Prototypes
//'System' Class Functions
void configureIO();
void configureExternalOscillator();
void configure32MhzInternalOsc();
void configureTimerCounter();
void configureRTC();
void configureADCs();
//static uint8_t ReadCalibrationByte( uint8_t index );