/*
 * XTendGroundStation.cpp
 *
 * Created: 5/17/2016 1:22:22 PM
 * Author : Nick McComb | www.nickmccomb.net
 */ 

/* 

Resources Allocation:
T/C 1: Sending information over Serial
T/C 2: PWM for LEDs?

RTC:   RSSI Interpret


*/

#include "main.h"

//GLOBAL Variables *gasp*
uint8_t broadcastStatus = 0;


int main(void)
{
	configureExternalOscillator();
	configureIO();
	configureUSART();
	configureTimerCounter();
	
	LOW_LEVEL_INTERRUPTS_ENABLE();
	sei();
	
	uint16_t delay = 100;
	uint16_t counter = 1;
	uint8_t toggle = 0;
	
	SIG_STR_1_SET();
	_delay_ms(delay);
	SIG_STR_1_CLR();
	_delay_ms(delay);
	SIG_STR_2_SET();
	_delay_ms(delay);
	SIG_STR_2_CLR();
	_delay_ms(delay);
	SIG_STR_3_SET();
	_delay_ms(delay);
	SIG_STR_3_CLR();
	_delay_ms(delay);
	SIG_STR_2_SET();
	_delay_ms(delay);
	SIG_STR_2_CLR();
	_delay_ms(delay);
	SIG_STR_1_SET();
	_delay_ms(delay);
	SIG_STR_1_CLR();
	_delay_ms(delay);
	SIG_STR_1_SET();
	SIG_STR_2_SET();
	SIG_STR_3_SET();
	_delay_ms(delay * 2);
	SIG_STR_1_CLR();
	SIG_STR_2_CLR();
	SIG_STR_3_CLR();
	_delay_ms(delay * 2);
	
	int comp = 5;
	
    /* Replace with your application code */
    while (1) 
    {
		_delay_ms(1);
		
		if(broadcastStatus){
			broadcastStatus = 0;
			
			TCC4.CNT = 0;
			
			if(toggle){
				toggle = 0;
				SIG_STR_1_CLR();
			}
			else{
				toggle = 1;
				SIG_STR_1_SET();
			}
			
			SendStringPC((char *)"\n\rHello, world! Iteration number: ");
			SendNumPC(counter++);
			
			if(CHECK_DIP_SW_1()){
				TCC4.PER = TC_1024_100MS;  //100mS delay
			}
			else{
				TCC4.PER = TC_1024_500MS;  //500mS delay
			}
		}
		
    }
}
