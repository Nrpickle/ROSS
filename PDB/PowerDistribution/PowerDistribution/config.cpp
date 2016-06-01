/*
 * config.cpp
 *
 * Created: 5/26/2016 10:12:21 AM
 *  Author: Nick McComb | www.nickmccomb.net
 */ 

#include "config.h"

extern RSSI_type RSSI;

void configureADCs(){
	// Batt Input Voltage
	// Halve Input
	//Set to single ended input
	//Set to 12-bit mode
		
	ADCA.CTRLB = (ADC_RESOLUTION_MT12BIT_gc | ADC_CONMODE_bm);	//Sets resolution to 12 bit and sets conversion mode to signed
	ADCA.REFCTRL = ADC_REFSEL_AREFA_gc;                              //Reference the "rail splitter" 2.5v reference
	ADCA.EVCTRL = 0; //Disable events
	ADCA.PRESCALER = ADC_PRESCALER_DIV512_gc;
	ADCA.CALL = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL0));
	ADCA.CALH = ReadCalibrationByte(offsetof(NVM_PROD_SIGNATURES_t, ADCACAL1));
	_delay_us(400);
	//ADCA.CH0.AVGCTRL = ADC_SAMPNUM_256X_gc;
	ADCA.CH0.CTRL = (ADC_CH_GAIN_1X_gc | ADC_CH_INPUTMODE_DIFFWGAINL_gc);
	ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN8_gc | ADC_CH_MUXNEGL_PIN1_gc);  //Init the ADC MUX to use the PIN8 input
																	 //and set the negative input to the GND
																     //reference on PORTA PIN1
	ADCA.CH0.INTCTRL = 0; // Set COMPLETE interrupts
	ADCA.CTRLA = ADC_ENABLE_bm;
	
	/*
	//ADCA.REFCTRL 
	
	//Temperature sensor is on PA0 (ADC0)
	//ADCA.CH0.CTRL = ADC_CH_INPUTMODE_SINGLEENDED_gc;

	//Select Pin8 group setting
	//ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN0_gc;
	
	//Enable the ADC
	//ADCA.CTRLA = ADC_ENABLE_bm;
	//Wait a ms for ADC to start up. This is _way_ overkill. Spec is 24 ADC clock cycles
	//_delay_ms(1);
	//ADCA.CH0.CTRL = ADC_CH_START_bm;
	//
	//ADCA.CTRLA |= ADC_CH8START_bm;
	*/	
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

void configureIO(void){
	//Set STATUS and ERROR LEDs to be outputs
	PORTC.DIRSET = PIN1_bm;
	PORTC.DIRSET = PIN0_bm;
	
	//Set the Relay Pin to be an output
	PORTC.DIRSET = PIN5_bm;
	
	//Set the settings switches to be inputs
	PORTC.DIRCLR = PIN2_bm;
	PORTC.DIRCLR = PIN3_bm;
	
	//Set pullups on the setting switches
	PORTC.PIN2CTRL = PORT_OPC_PULLUP_gc;
	PORTC.PIN3CTRL = PORT_OPC_PULLUP_gc;
	
	//SET ADC Pints to be inputs
	PORTA.DIRCLR = PIN0_bm;  //2.5v ref (AFREF Pin)
	PORTA.DIRCLR = PIN1_bm;  //Ground reference
	PORTD.DIRCLR = PIN0_bm;  //Temp-Sensor Pin
	PORTD.DIRCLR = PIN1_bm;  //Voltage Sense - Electronics Battery
	PORTD.DIRCLR = PIN2_bm;  //Voltage Sense - Rear Battery
	
	//Setup the RSSI input
	PORTA.DIRCLR = PIN2_bm;				//Set the RSSI pin to be an input
	
	//Setup the steering signal I/O
	PORTD.DIRCLR = PIN4_bm;  //Set the STEER_SIG_3v3 pin as an input
	PORTD.DIRSET = PIN5_bm;	 //Set the STEER_SIG_3v3_PROCESSED pin to be an output

	//Setup steering control PWM interrupts
	PORTD.INTCTRL  = PMIC_MEDLVLEN_bm;	//Set PORTC's interrupt to be medium level
	PORTD.INTMASK  = PIN4_bm;			//Configure the PWM input pin as an interrupt
	PORTD.PIN4CTRL = PORT_ISC_RISING_gc;  //Init the pin as a rising edge interrupt only

	
	//Initialize output values
	STATUS_CLR();
	ERROR_CLR();
	
	REAR_RELAY_CLR();
	
	STEER_SIG_CLR();	
}

/*

PWM Interpret "Pseudocode"

On rising edge, start counter
Set overflow to be 24 milliseconds (see TCC5 OVF vector)
DELAY (25us?)
Set interrupt mode to falling only
Wait for falling edge
if timer has overflown, then we missed the appropriate edge, throw out our data
If timer is good, calculate PWM high time
Store in global variable

*/
ISR(PORTD_INT_vect){
	PORTD.INTFLAGS = PIN4_bm;

	
	if(PWMMeasuringStatus == NOT_MEASURING){  //We encountered the first part of the wave
		TCC5.CNT = 0;	//Start counting
		PWMMeasuringStatus = MEASURING;
		
		_delay_us(25);  //Delay ~6-7 clock cycles
		
		PORTD.PIN4CTRL = PORT_ISC_FALLING_gc; //Set the interrupt to wait for a falling wave (end of signal)
	}
	else { //We finished encountering the wave (process the data)
		steeringPWMPeriod = TCC5.CNT * 2;
		PWMMeasuringStatus = NOT_MEASURING;
		
		PORTD.PIN4CTRL = PORT_ISC_RISING_gc; 
	}
	
}

//This function will be called on the edges of the RSSI signal 
//*CURRENTLY DISABLED*
ISR(PORTA_INT_vect){
	cli();
	
	PORTA.INTFLAGS = PIN2_bm;  //Reset the interrupt flag for this pin
	
	if(RSSI.measuring == NOT_MEASURING && READ_RSSI_PIN()){   //We detected one of these ____/---
		RTC.CNT = 0;		//We want to start counting the counter now
		RSSI.measuring = MEASURING;
		

	}
	else if (RSSI.measuring == MEASURING && !READ_RSSI_PIN()){  //That means we are at this point ---\____
		RSSI.countDifference = RTC.CNT;
		
		RSSI.sampleCount++;
	}
	else {
		ERROR_SET();
	}
	
	
	//_delay_us(200);
	sei();
}

void configure32MhzInternalOsc(){
	OSC_CTRL |= OSC_RC32MEN_bm; //Setup 32Mhz crystal
	
	while(!(OSC_STATUS & OSC_RC32MRDY_bm));
	
	CCP = CCP_IOREG_gc; //Trigger protection mechanism
	CLK_CTRL = CLK_SCLKSEL_RC32M_gc; //Enable internal  32Mhz crystal
}

/*
TIMER COUNTERS

The 32E5 has one TC4 and two TC5s.

The TC4 is used for generation of accurate timing for reporting.

TC5 is used entirely for remote steering control and passthrough.

Assuming 1024 prescaler, the following values are known for 32Mhz
	
100mS = 0x0C35
500mS = 0x3D09
1S    = 0x7A12

*/
void configureTimerCounter(){
	//Configure the accurate reporting timer
	TCC4.CTRLA = TC45_CLKSEL_DIV1024_gc;	//Configure a 1024 prescaler (we want very broad timing here, exact precision isn't required)
	TCC4.PER = TC_1024_500MS;               //500mS delay
											//Default delay value. Reference pre-calculated table up above for more information
	TCC4.CTRLB = TC45_WGMODE_NORMAL_gc;		//Configure the timer for Normal mode operation
	TCC4.INTCTRLA = TC45_OVFINTLVL_LO_gc;	//Set a low priority overflow interrupt

	//Configure the PWM sense module
	//Input capture described on (168)
	TCC5.CTRLA = TC45_CLKSEL_DIV64_gc;		//Configure a 64 prescaler (will count ~10,000 in 20mS)
	TCC5.CTRLB = TC45_WGMODE_NORMAL_gc;		//Normal operation
	TCC5.PER   = 12000;						//This is set to be 20% longer than the 20mS cycle should be
	
	//Is the following necessary?
	TCC5.INTCTRLA = TC45_OVFINTLVL_MED_gc;	//Set a medium priority overflow interrupt (we want the PWM generation to remain stable)
											//as it will only take a few clock cycles compared to this interrupt
	
	//Configure the PWM generation module
	TCD5.CTRLA = TC45_CLKSEL_DIV64_gc;		//Configure a 64 prescaler (will count ~10,000 in 20mS)
	TCD5.CTRLB = TC45_WGMODE_NORMAL_gc;		//Normal operation
	TCD5.PER   = 10000;						//We want to establish a 50Hz control loop here (20ms period)
	TCD5.INTCTRLA = TC45_OVFINTLVL_HI_gc;	//Set a high priority overflow interrupt
	TCD5.INTCTRLB = TC45_CCAINTLVL_HI_gc;   
	
	TCD5.CCA = 950;		//Initial value for compare
}

//This triggers when a PWM signal hasn't been detected for the past ~24mS
ISR (TCC5_OVF_vect){
	TCC5.INTFLAGS |= 0b1;  //Reset interrupt flag
	
	PWMMeasuringStatus = NOT_MEASURING;		//Reset the "NOT MEASURING" flag
	PORTD.PIN4CTRL = PORT_ISC_RISING_gc;	//Reconfigure the port to wait for a high pulse
	
	steeringPWMPeriod = 1500;				//Set the steering PWM period to a stable "standby position"
}

/*
PWM Generation
______/---- ... ---\_____ ... ____/---- 
      ^--  1-2ms --^    20ms  ----^
*/

/*
Compare vector A for the PWM generation module
This situation ------\______

*/
ISR (TCD5_CCA_vect){
	STEER_SIG_CLR();
	//STATUS_SET();
}

/*
Overflow vector for the PWM generation module

This situation: _____/-----

*/


ISR (TCD5_OVF_vect){
	STEER_SIG_SET();
	
	TCD5.INTFLAGS |= 0b1;
	
	TCD5.CNT = 0;
}

//Handles compare vector for T/C 4
ISR (TCC4_OVF_vect){
	++longCounter;
	
	TCC4.INTFLAGS |= 0b1;  //Reset overflow interrupt
	
	broadcastStatus = 1;
}

/*
The real time clock is configured to handle XTend RSSI Interpret
*/
void configureRTC(){
	RTC.CTRL = RTC_CORREN_bm | RTC_PRESCALER_DIV1_gc;		//Enable the RTC correction process, and the RTC itself with no prescaler
	RTC.INTCTRL = RTC_COMPINTLVL_LO_gc | RTC_OVFINTLVL_LO_gc; //Enable the overflow and 
	
    OSC.CTRL |= OSC_RC32KEN_bm;								//Enable the 32.768kHz internal oscillator
	
	_delay_us(400);											//Wait for the oscillator to stabalize.
	
	CLK.RTCCTRL = CLK_RTCSRC_RCOSC32_gc;					//Set the RTC input as the 32.768kHz internal oscillator
	CLK.RTCCTRL |= CLK_RTCEN_bm;							//Enable the clock input
	
	//Testing setup code
	RTC.COMP = 16384; //~1 second? Assuming 32.768 KHz
	RTC.PER = 0xFF00;  //No tengo nuguien idea

}

ISR(RTC_OVF_vect){
	
}

ISR(RTC_COMP_vect){
	
	RTC.CNT = 0;
	RTC.INTFLAGS = 0x02;
	
	RSSI.countDifference = 0;
}

void configureXCL(){
	//XCL.INTCTRL = XCL_TC16
	
}

/* Read NVM signature. From http://www.avrfreaks.net/forum/xmega-production-signature-row */
uint8_t ReadCalibrationByte( uint8_t index ){
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return( result );
}
