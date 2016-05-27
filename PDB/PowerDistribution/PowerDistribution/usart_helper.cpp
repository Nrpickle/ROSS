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
	
	//sprintf(buffer, "%f", numToSend);
	SendStringPC(buffer);
}

void configureUSART(void){
	//Set TX (pin7) to be output
	PORTC.DIRSET = PIN7_bm;
	//Set RX (pin6) to be input
	PORTC.DIRCLR = PIN6_bm;
	
	//Enable alternate pin location for USART0 in PORTC
	PORTC.REMAP |= (1 << 4);
	
	USART_Format_Set(&COMP_USART, USART_CHSIZE_8BIT_gc, USART_PMODE_DISABLED_gc, false);
	
	//Enable a 57600 baudrate
	USART_Baudrate_Set(&COMP_USART, 34, 0);
	
	// Enable both RX and TX.
	USART_Rx_Enable(&COMP_USART);
	USART_Tx_Enable(&COMP_USART);
}