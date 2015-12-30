/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015 4:35:31 PM
 * Author : Nick McComb | nickmccomb.net
 */ 

#define F_CPU 32000000

#include <avr/io.h>
#include <util/delay.h>
#include "Macros.h"

//Function Prototypes
void initIO();
void configureExternalOscillator();
void configure32MhzInternalOsc();

int main(void)
{
	initIO();
	//configureExternalOscillator();
	configure32MhzInternalOsc();
	
    while (1) 
    {
		_delay_ms(500);
		
		STATUS_CLR();
		ERROR_SET();
		
		_delay_ms(500);
		
		STATUS_SET();
		ERROR_CLR();
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
