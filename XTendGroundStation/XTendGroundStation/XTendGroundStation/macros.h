

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

//Switch Input Macros
#define CHECK_DIP_SW_2(void) (!(PORTC.IN & PIN2_bm)) //Returns true if bit 1 of the DIP Switch is "ON"
#define CHECK_DIP_SW_1(void) (!(PORTC.IN & PIN3_bm)) //Returns true if bit 2 of the DIP Switch is "ON"


//RSSI Input from the XTend
#define READ_RSSI_PIN(void) (PORTA.IN & PIN2_bm)

/*** "SYSTEM" Class MACROS ***/

//Interrupt enables
#define LOW_LEVEL_INTERRUPTS_ENABLE()  (PMIC.CTRL |= PMIC_LOLVLEN_bm)
#define MED_LEVEL_INTERRUPTS_ENABLE()  (PMIC.CTRL |= PMIC_MEDLVLEN_bm)
#define HIGH_LEVEL_INTERRUPTS_ENABLE() (PMIC.CTRL |= PMIC_HILVLEN_bm)