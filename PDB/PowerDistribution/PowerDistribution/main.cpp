/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015 4:35:31 PM
 * Author : Nick McComb | nickmccomb.net
 */ 

#ifndef F_CPU
#define F_CPU 32000000
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "Macros.h"
#include "usart_driver.h"
#include "avr_compiler.h"

//Function Prototypes
void initIO();
void configureExternalOscillator();
void configure32MhzInternalOsc();
void configureUSART();
void SendStringPC(char *stufftosend);
void SendNumPC(uint16_t numToSend);

//Global Data

#define COMP_USART USARTC0

/* Variable used to send and receive USART data. */
uint8_t sendData;
uint8_t receivedData;


int main(void)
{
	initIO();
	//configureExternalOscillator();
	configure32MhzInternalOsc();
	configureUSART();
	
	uint16_t counter = 0;
	
    while (1) 
    {
		_delay_ms(1);
		
		STATUS_CLR();
		ERROR_SET();
		
		_delay_ms(1);
		
		STATUS_SET();
		ERROR_CLR();
		
		SendNumPC(counter++);
		SendStringPC("\n\r");

    }
}

void configureExternalOscillator(){
	
	//Configure for 4Mhz external crystal. Take 16K clock cycle because startup time doesn't matter.
	OSC_XOSCCTRL = OSC_FRQRANGE_2TO9_gc | OSC_XOSCSEL_XTAL_16KCLK_gc;
	
	//Enable external oscillator
	OSC_CTRL |= OSC_XOSCEN_bm;
	
	//Wait for clock stabilization
	while(!(OSC_STATUS & OSC_XOSCRDY_bm));
	
	/*
	
	//Set PLL multiplication of 8 (4Mhz * 8 = 32Mhz)
	//PLLFAC is the lowest significant 5 bits
	OSC.PLLCTRL |= 8;
	
	//Set PLL input source (external clock)
	OSC.PLLCTRL |= OSC_PLLSRC_XOSC_gc;
	
	//Enable the PLL
	OSC.CTRL |= OSC_PLLEN_bm;
	
	//Wait for PLL to stabilize
	while(!(OSC_STATUS & OSC_PLLRDY_bm));
	*/
	
}
 
void initIO(void){
	//Set STATUS and ERROR LEDs to be outputs
	PORTC.DIRSET = PIN1_bm;
	PORTC.DIRSET = PIN0_bm;
	
	//Set the Relay Pin to be an output
	PORTC.DIRSET = PIN5_bm;
	
	//Initialize output values
	STATUS_CLR();
	ERROR_CLR();
	
	REAR_RELAY_CLR();
		
}

void configure32MhzInternalOsc()
{
	OSC_CTRL |= OSC_RC32MEN_bm; //Setup 32Mhz crystal
	
	while(!(OSC_STATUS & OSC_RC32MRDY_bm));
	
	CCP = CCP_IOREG_gc; //Trigger protection mechanism
	CLK_CTRL = CLK_SCLKSEL_RC32M_gc; //Enable internal  32Mhz crystal
	
	
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