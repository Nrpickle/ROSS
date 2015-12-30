/*
 * usartROSS.cpp
 *
 * Created: 12/30/2015 1:48:15 PM
 *  Author: Nick McComb [nickmccomb.net]
 */ 

#include "usartROSS.h"

//Sends a string to the computer
void SendStringPC(char *stufftosend){
	for(int i = 0 ; stufftosend[i] != '\0' ; i++){
		while(!USART_IsTXDataRegisterEmpty(&COMP_USART));
		USART_PutChar(&COMP_USART, stufftosend[i]);
	}
}

void SendNumPC(uint16_t numToSend){
	char buffer[20];
	itoa(numToSend, buffer, 10);
	SendStringPC(buffer);
}