/*
 * PowerDistribution.cpp
 *
 * Created: 12/22/2015 4:35:31 PM
 * Author : nrpic_000
 */ 

#define F_CPU 2000000

#include <avr/io.h>
#include <util/delay.h>


#define STATUS_SET(void) (PORTC.OUTSET = PIN0_bm)
#define STATUS_CLR(void) (PORTC.OUTCLR = PIN0_bm)

#define ERROR_SET(void) (PORTC.OUTSET = PIN1_bm)
#define ERROR_CLR(void) (PORTC.OUTCLR = PIN1_bm)


int main(void)
{
	PORTC.DIRSET = PIN1_bm;
	PORTC.DIRSET = PIN0_bm;
	
	STATUS_SET();
	
    /* Replace with your application code */
    while (1) 
    {
		_delay_ms(500);
		ERROR_SET();
		STATUS_CLR();
		_delay_ms(500);
		STATUS_SET();
		ERROR_CLR();
    }
}

