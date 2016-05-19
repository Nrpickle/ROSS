/*
 * usartROSS.cpp
 *
 * Created: 12/30/2015 1:48:15 PM
 *  Author: Nick McComb [nickmccomb.net]
 */ 

#include "usart_helper.h"
#include <stdio.h>

//Sends a string to the computer
void SendStringPC(char *stufftosend){
	for(int i = 0 ; stufftosend[i] != '\0' ; i++){
		while(!USART_IsTXDataRegisterEmpty(&COMP_USART));
		USART_PutChar(&COMP_USART, stufftosend[i]);
	}
}

//Can only send 16 bit numbers... TODO: Should this change?
void SendNumPC(uint16_t numToSend){
	char buffer[20];
	itoa(numToSend, buffer, 10);
	SendStringPC(buffer);
}

void SendFloatPC(double numToSend){
	char buffer[100];
	
	int d1 = numToSend;
	float f2 = numToSend - d1;
	int d2 = trunc(f2 * 10000);
	
	sprintf(buffer, "%d.%04d", d1, abs(d2));
	
	SendStringPC(buffer);
} 
