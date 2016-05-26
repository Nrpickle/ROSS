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

struct RSSI_type {
	//"User" class variables
	uint16_t value;
	uint8_t timeoutOccured;
	
	//"System" class variables
	uint8_t measuring;
	uint16_t timeDifference;
	uint16_t countDifference;
	uint16_t sampleCount;
	uint16_t sampleCountTemp;
};

//GLOBAL Variables *gasp*
volatile uint8_t broadcastStatus = 0;
volatile uint8_t globalToggle = 0;
volatile RSSI_type RSSI;

int main(void)
{
	configureExternalOscillator();
	configureIO();
	configureUSART();
	configureTimerCounter();
	configureRTC();
	
	LOW_LEVEL_INTERRUPTS_ENABLE();
	sei();
	
	
	uint16_t delay = 100;
	uint16_t counter = 1;
	uint8_t toggle = 0;
	
	//RSSI.measuring = NOT_MEASURING;
	RSSI.timeDifference = 0;
	RSSI.sampleCount = 0;
	RSSI.timeoutOccured = 0;
	
	outputSignalStrength(0);
	_delay_ms(250);
	outputSignalStrength(1);
	_delay_ms(250);
	outputSignalStrength(2);
	_delay_ms(250);
	outputSignalStrength(3);
	_delay_ms(250);
	outputSignalStrength(2);
	_delay_ms(250);
	outputSignalStrength(1);
	_delay_ms(250);
	outputSignalStrength(0);
	_delay_ms(250);

	/*
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
	*/
	
	int comp = 5;
	
    /* Replace with your application code */
    while (1) 
    {
		_delay_ms(1);
		
		if(broadcastStatus){
			broadcastStatus = 0;
			
			TCC4.CNT = 0;
			
			//Get RSSI Value
						
			processRSSI();
			
			//For the following, 20 was chosen arbitrarily, then it was incremented by 80/3 (~26)
			if(RSSI.value < 20){
				outputSignalStrength(0);
			}
			else if(RSSI.value < 46){
				outputSignalStrength(1);
			}
			else if(RSSI.value < 73){
				outputSignalStrength(2);
			}
			else {
				outputSignalStrength(3);
			}
			
			 uint8_t basicOutputCounter;
			
			//DIP Switch 2 configured as "Verbose" signal strength output
			if(CHECK_DIP_SW_2()){
				SendStringPC((char *)"\n\rIteration number: ");
				SendNumPC(counter++);
				SendStringPC((char *)"\tRTC Value: ");
				SendNumPC(RTC.CNT);
				SendStringPC((char *)"\tRSSI Timeout: ");
				SendNumPC(RSSI.timeoutOccured);
				SendStringPC((char *)"\tRSSI Count: ");
				SendNumPC(RSSI.countDifference);
				SendStringPC((char* )"\tRSSI Value: ");
				SendNumPC(RSSI.value);
			}
			else {
				basicOutputCounter = (RSSI.value + 4) / 10;
				SendStringPC((char *)"\rSignal Strength: ");
				SendNumPC(RSSI.value);
				SendStringPC((char *)"%");
				
				SendStringPC("\t[");
				for(int i = 0; i < basicOutputCounter; i++){
					SendStringPC("#");
				}
				for(int i = 0; i < (10-basicOutputCounter); i++){
					SendStringPC(" ");
				}
				SendStringPC((char *)"] ");
				
			}
			
			
			if(CHECK_DIP_SW_1()){
				TCC4.PER = TC_1024_100MS;  //100mS delay
			}
			else{
				TCC4.PER = TC_1024_500MS;  //500mS delay
			}
		}
		
    }
}

//Processes the RSSI Signal from an XTend, defines can be found in main.h
//Function should timeout if no signal is detected.
void processRSSI(){
	uint16_t timeoutCounter = 0;
	uint16_t timeoutThreshold = RSSI_READ_TIMEOUT / RSSI_DEBOUNCE_DELAY + 1;
	RSSI.timeoutOccured = 0;
	
	ERROR_CLR();
	
	//Get RSSI from XTend
	RSSI.measuring = 0;
	do{   //Wait until we have a "Low" signal on the RSSI (wait for this ----\_____)
		_delay_us(RSSI_DEBOUNCE_DELAY);
		if(timeoutCounter++ > timeoutThreshold){
			ERROR_SET();
			break;
		}
	}while(READ_RSSI_PIN());
	
	timeoutCounter = 0;
	do{  //Wait until we have a "High" signal on the RSSI (wait for this ____/----)
		_delay_us(RSSI_DEBOUNCE_DELAY);
		if(timeoutCounter++ > timeoutThreshold){  //if the pulse is always low, this will timeout
			ERROR_SET();
			RSSI.timeoutOccured = 1;
			break;
		}
	}while(!READ_RSSI_PIN());
	
	RTC.CNT = 0;  //Start counting
	
	timeoutCounter = 0;
	do{   //Wait until we have a "Low" signal on the RSSI (wait for this ----\_____)
		_delay_us(RSSI_DEBOUNCE_DELAY);
		if(timeoutCounter++ > timeoutThreshold){
			STATUS_SET();
			RSSI.timeoutOccured = 1;
			break;
		}
	}while(READ_RSSI_PIN());
	
	RSSI.countDifference = RTC.CNT;
	
	//if(!RSSI.timeoutOccured){
		RSSI.value = RSSI.countDifference / 3.4;
	//}
	
	
	RSSI.sampleCount++;
}

void outputSignalStrength(uint8_t str){
	switch (str){
		case 0:
			SIG_STR_1_CLR();
			SIG_STR_2_CLR();
			SIG_STR_3_CLR();
			break;
		case 1:
			SIG_STR_1_SET();
			SIG_STR_2_CLR();
			SIG_STR_3_CLR();
			break;
		case 2:
			SIG_STR_1_SET();
			SIG_STR_2_SET();
			SIG_STR_3_CLR();
			break;
		case 3:
			SIG_STR_1_SET();
			SIG_STR_2_SET();
			SIG_STR_3_SET();
			break;
	}
}