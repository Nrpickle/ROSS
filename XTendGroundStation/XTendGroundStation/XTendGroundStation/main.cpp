/*
 * XTendGroundStation.cpp
 *
 * Created: 5/17/2016 1:22:22 PM
 * Author : nrpic_000
 */ 


#ifndef F_CPU
//#define F_CPU 32000000  //Define clock speed for _delay
#define F_CPU 4000000
#endif

#include <stdint.h>
#include <stdlib.h>
#include <avr/io.h>
#include <util/delay.h>
#include "main.h"


int main(void)
{
	//configureExternalOscillator();
	//configureIO();
	
	PORTA.DIRSET = PIN4_bm;
	
	PORTA.OUTSET = PIN4_bm;
	
	_delay_ms(400);
	
	PORTA.OUTCLR = PIN4_bm;
	
	_delay_ms(400);
	
	PORTA.OUTSET = PIN4_bm;
	
	_delay_ms(400);
	
	PORTA.OUTCLR = PIN4_bm;
	
	_delay_ms(400);
	
    /* Replace with your application code */
    while (1) 
    {
		
		PORTA.OUTSET = PIN4_bm;
		
		_delay_ms(400);
		
		PORTA.OUTCLR = PIN4_bm;
		
		_delay_ms(400);
		/*
		SIG_STR_1_SET();
		SIG_STR_2_CLR();
		STATUS_CLR();
		_delay_us(50);
		*/
    }
}

void configureIO(){
	//Set the signal strength LEDs to be outputs
	PORTA.DIRSET = PIN4_bm;
	PORTA.DIRSET = PIN5_bm;
	PORTA.DIRSET = PIN6_bm;
	
}


void configureExternalOscillator(){
	int temp = 0;																			//Temporary variable for helping avoid 4 clock cycle limitation when updating secure registers
	
	//Enable external 4MHz oscillator
	OSC.XOSCCTRL = (OSC_FRQRANGE_2TO9_gc | OSC_XOSCSEL_XTAL_16KCLK_gc);						//Set external oscillator to be between 2 and 9 MHz and select it
	OSC.CTRL |= OSC_XOSCEN_bm;																//Enable the external oscillator
	while(!(OSC.STATUS & OSC_XOSCRDY_bm)){ERROR_SET();};									//While the external oscillator is not ready, set the error led
	ERROR_CLR();																			//Clear the error led if the external oscillator has stabilized
	
	//Enable phase locked loop to multiply external oscillator by 8 to get 32MHz
	temp = ((OSC_PLLSRC_XOSC_gc & OSC_PLLSRC_gm) | (OSC_PLLFAC_gm & 8));					//Set the external oscillator as the clock source for the pll and set to multiply by 8
	CCP = CCP_IOREG_gc;																		//Disable register security so we can update the pll control settings
	OSC.PLLCTRL = temp;																		//Write pll control settings to register
	OSC.CTRL |= OSC_PLLEN_bm;																//Enable the pll
	while(!(OSC.STATUS & OSC_PLLRDY_bm)){ERROR_SET();};										//While the pll is not ready, set the error led
	ERROR_CLR();																			//Disable the error led if successfully stabilized
	
	//Set system pll clock divisions and set up as source for all system clocks
	temp = ((CLK_PSADIV_gm & CLK_PSADIV_1_gc) | (CLK_PSBCDIV_gm & CLK_PSBCDIV_1_1_gc));		//Set system to use pll divided by 1 (no division)
	CCP = CCP_IOREG_gc;																		//Disable register security so we can update the clock source division setting
	CLK.CTRL = temp;																		//Write division settings to register
	
	temp = CLK_SCLKSEL_PLL_gc;																//Set pll as system clock source
	CCP = CCP_IOREG_gc;																		//Disable register security so we can update the system clock
	CLK.CTRL = temp;																		//Write clock source settings to register
	
	
}