/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015 4:35:31 PM
 * Author : Nick McComb | nickmccomb.net
 */ 

#define F_CPU 2000000

#include <avr/io.h>
#include <util/delay.h>
#include "Macros.h"

//Function Prototypes
void initIO();

int main(void)
{
	initIO();

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