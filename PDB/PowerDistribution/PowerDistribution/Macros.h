
/* This file contains all of the Macros that are used in the rest of the code */

/*** "SYSTEM" Class MACROS ***/

//Interrupt enables
#define LOW_LEVEL_INTERRUPTS_ENABLE() (PMIC.CTRL |= PMIC_LOLVLEN_bm)
#define MED_LEVEL_INTERRUPTS_ENABLE() (PMIC.CTRL |= PMIC_MEDLVLEN_bm)
#define HIGH_LEVEL_INTERRUPTS_ENABLE() (PMIC.CTRL |= PMIC_HILVLEN_bm)

/*** "USER" Class MACROS ***/

//LED Macros
#define STATUS_SET(void) (PORTC.OUTSET = PIN0_bm)
#define STATUS_CLR(void) (PORTC.OUTCLR = PIN0_bm)

#define ERROR_SET(void) (PORTC.OUTSET = PIN1_bm)
#define ERROR_CLR(void) (PORTC.OUTCLR = PIN1_bm)

//Rear relay control
#define REAR_RELAY_SET(void) (PORTC.OUTSET = PIN5_bm)
#define REAR_RELAY_CLR(void) (PORTC.OUTCLR = PIN5_bm)

//DIP Switch Inputs
#define CHECK_DIP_SW_2(void) (!(PORTC.IN & PIN2_bm)) //Returns true if bit 1 of the DIP Switch is "ON"
#define CHECK_DIP_SW_1(void) (!(PORTC.IN & PIN3_bm)) //Returns true if bit 2 of the DIP Switch is "ON"

//XTend RSSI Input
#define READ_RSSI_PIN(void) (PORTA.IN & PIN2_bm)

//Steering Signal output
#define STEER_SIG_SET(void) (PORTD.OUTSET = PIN5_bm)
#define STEER_SIG_CLR(void) (PORTD.OUTCLR = PIN5_bm)

//Steering Signal Input
#define CHECK_STEER_SIG_INPUT() (PORTC.IN & PIN4_bm)

//Timer Counter Macros
#define TC_1024_100MS  0x0C35
#define TC_1024_500MS  0x3D09
#define TC_1024_1000MS 0x7A12

//PWM Generation Macros
#define TC_PWM_1MS 500
#define TC_PWM_1_1MS 550
#define TC_PWM_1_2MS 600
#define TC_PWM_1_3MS 650
#define TC_PWM_2MS 1000