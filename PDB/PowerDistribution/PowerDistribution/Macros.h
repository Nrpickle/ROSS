
/* This file contains all of the Macros that are used in the rest of the code */

//LED Macros
#define STATUS_SET(void) (PORTC.OUTSET = PIN0_bm)
#define STATUS_CLR(void) (PORTC.OUTCLR = PIN0_bm)

#define ERROR_SET(void) (PORTC.OUTSET = PIN1_bm)
#define ERROR_CLR(void) (PORTC.OUTCLR = PIN1_bm)

#define REAR_RELAY_SET(void) (PORTC.OUTSET = PIN5_bm)
#define REAR_RELAY_CLR(void) (PORTC.OUTCLR = PIN5_bm)