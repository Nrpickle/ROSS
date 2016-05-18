

/*** USER CLASS MACROS ***/

//LED Ouptut Macros
#define SIG_STR_1_SET(void) (PORTA.OUTSET = PIN4_bm)
#define SIG_STR_1_CLR(void) (PORTA.OUTCLR = PIN4_bm)

#define SIG_STR_2_SET(void) (PORTA.OUTSET = PIN5_bm)
#define SIG_STR_2_CLR(void) (PORTA.OUTCLR = PIN5_bm)

#define SIG_STR_3_SET(void) (PORTA.OUTSET = PIN6_bm)
#define SIG_STR_3_CLR(void) (PORTA.OUTCLR = PIN6_bm)


#define STATUS_SET(void) (PORTC.OUTSET = PIN0_bm)
#define STATUS_CLR(void) (PORTC.OUTCLR = PIN0_bm)

#define ERROR_SET(void) (PORTC.OUTSET = PIN1_bm)
#define ERROR_CLR(void) (PORTC.OUTCLR = PIN1_bm)