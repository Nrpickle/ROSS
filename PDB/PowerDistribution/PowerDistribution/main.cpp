/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015
 * Author : Nick McComb | nickmccomb.net
 */ 

#define FIRMWARE_VERSION_STR ".1"
#define FIRMWARE_VERSION .1


#include "main.h"
#include "usartROSS.h"
#include "avr_compiler.h"
#include <avr/io.h>
#include <stddef.h>


//Global Variables *gasp*
int toggle = 0;
volatile uint64_t longCounter = 0;
volatile uint8_t broadcastStatus = 0;

int main(void)
{
	configureIO();
	configureExternalOscillator();
	configureUSART();	//Set up for 57600 Baud
	configureTimerCounter();
	configureADCs();

	//PMIC.CTRL |= PMIC_LOLVLEN_bm;
	
	LOW_LEVEL_INTERRUPTS_ENABLE();
	sei();
	
	uint8_t receivedUSARTData;
	
	SendStringPC((char *)"#[INIT ROSS PDB]\n\r");
	SendStringPC((char *)"#Firmware version ");
	SendStringPC((char *)FIRMWARE_VERSION_STR);
	SendStringPC((char *)"\n\r#Msg format: Electronics Batt Volt | Rear Batt Volt | Ebox Temperature | 5v_SYS Curr | 5v_Comp Curr \n\r");
	
	
    while (1) 
    {

		_delay_ms(1);

		//Check for commands from the computer
		if(USART_IsRXComplete(&COMP_USART)){
			receivedUSARTData = USART_GetChar(&COMP_USART);
			if(receivedUSARTData == 'y')
				REAR_RELAY_SET();
			else if(receivedUSARTData == 'n')
				REAR_RELAY_CLR();
		}		
				
		//if (timingCounter++ == timingThreshold) {
		if(broadcastStatus){
			broadcastStatus = 0;
			
			//Calculated desired output values
			double EBoxTemp = getEBoxTemperature();
			double electronicsBatteryVoltage = getElectronicsBatteryVoltage();
			double zero = 0.0;
			
			//Actually output the desired values
			//Not the most elegant code in the world, but it works...
			
			//Send the battery voltage
			SendFloatPC(electronicsBatteryVoltage);
			SendStringPC((char *)"|");
			//Send the rear battery voltage
			SendFloatPC(zero);
			SendStringPC((char *)"|");
			//Send the EBox Temperature
			SendFloatPC(EBoxTemp);
			SendStringPC((char *)"|");
			//Send 5v_SYS Curr
			SendFloatPC(zero);
			SendStringPC((char *)"|");
			//Send 5v_Comp Curr
			SendFloatPC(zero);
			//SendStringPC((char *)"|");
			
			SendStringPC((char *)"\n\r");
	
			//Check the updating speed setting
			if(CHECK_DIP_SW_1()){
				TCC4.PER = TC_1024_100MS;  //100mS delay
			}
			else{
				TCC4.PER = TC_1024_500MS;  //500mS delay
			}
		}
    }
}

//Secret sauce
double ADCCountToVoltage(uint16_t adcCount){
  
  //Testing and comparing voltages to corresponding count values resulted in this fun function:
  return adcCount * 0.0011982182628062362 + 0.0023407572383072894; //I figure the compiler will trim off what it can't actually use...
	
}

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
	
	//Set pullups on the switches
	PORTC.PIN2CTRL = PORT_OPC_PULLUP_gc;
	PORTC.PIN3CTRL = PORT_OPC_PULLUP_gc;
	
	//SET ADC Pints to be inputs
	PORTA.DIRCLR = PIN0_bm;  //2.5v ref (AFREF Pin)
	PORTA.DIRCLR = PIN1_bm;  //Ground reference
	PORTD.DIRCLR = PIN0_bm;  //Temp-Sensor Pin
	PORTD.DIRCLR = PIN1_bm;  //Voltage Sense - Electronics Battery
	PORTD.DIRCLR = PIN2_bm;  //Voltage Sense - Rear Battery
	
	
	//Initialize output values
	STATUS_CLR();
	ERROR_CLR();
	
	REAR_RELAY_CLR();
		
}

void configure32MhzInternalOsc(){
	OSC_CTRL |= OSC_RC32MEN_bm; //Setup 32Mhz crystal
	
	while(!(OSC_STATUS & OSC_RC32MRDY_bm));
	
	CCP = CCP_IOREG_gc; //Trigger protection mechanism
	CLK_CTRL = CLK_SCLKSEL_RC32M_gc; //Enable internal  32Mhz crystal
}

/*
Function documentation:
	
Assuming 1024 prescaler, the following values are known for 32Mhz
	
100mS = 0x0C35
500mS = 0x3D09
1S    = 0x7A12

*/
void configureTimerCounter(){
	//Set the timer to run (with a prescaler)	
	TCC4.CTRLA = TC45_CLKSEL_DIV1024_gc;	//Configure a 1024 prescaler (we want very broad timing here, exact precision isn't required)
	TCC4.PER = TC_1024_500MS;               //500mS delay
											//Default delay value. Reference pre-calculated table up above for more information
	
	TCC4.CTRLB = TC45_WGMODE_NORMAL_gc;		//Configure the timer for Normal mode operation
	
	TCC4.INTCTRLA = TC45_OVFINTLVL_LO_gc;	//Set a low priority overflow interrupt
}

//Handles compare vector for T/C 4
ISR (TCC4_OVF_vect){
	++longCounter;
	STATUS_CLR();
	
	TCC4.INTFLAGS |= 0b1;  //Reset overflow interrupt
	
	broadcastStatus = 1;	
}

/* Read NVM signature. From http://www.avrfreaks.net/forum/xmega-production-signature-row */
static uint8_t ReadCalibrationByte( uint8_t index ){
	uint8_t result;

	/* Load the NVM Command register to read the calibration row. */
	NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc;
	result = pgm_read_byte(index);

	/* Clean up NVM Command register. */
	NVM_CMD = NVM_CMD_NO_OPERATION_gc;

	return( result );
}

int16_t sampleTempSensorVoltage(void){
	ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN8_gc | ADC_CH_MUXNEGL_PIN1_gc);//ADC_CH_MUXNEG0_bm);
	ADCA.CH0.CTRL |= ADC_CH_START_bm;
	
	_delay_us(400);
	
	//while(((ADCA.INTFLAGS >> ADC_CH0IF_bp) & (1U << 8)) != (1U << 8)); // (1U << n) where n is the adc channel, so zero for this one
	
	while(!(ADCA.INTFLAGS & (1 << 0)));
	ADCA.INTFLAGS = (1 << 0);
	
	return 	ADCA.CH0.RES;
}

int16_t sampleBatteryVoltage(void){
	ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN9_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for batt voltage sense
	ADCA.CH0.CTRL |= ADC_CH_START_bm;
	
	//while(((ADCA.INTFLAGS >> ADC_CH0IF_bp) & (1U << 0)) != (1U << 0)); // (1U << n) where n is the adc channel, so zero for this one
	//ADCA.INTFLAGS = ((1U << 0) << ADC_CH0IF_bp);
	while(!(ADCA.INTFLAGS & (1 << 0)));
	ADCA.INTFLAGS = (1 << 0);
	
	return 	ADCA.CH0.RES;
}

double getEBoxTemperature(){
	
	int avgVal = 100;
	uint16_t temperature = 0;
	uint64_t sum = 0;
	for(int i = 0; i < avgVal; ++i){
		sum += sampleTempSensorVoltage();
	}
	temperature = sum / avgVal;
	
	double temperatureVoltage = ADCCountToVoltage(temperature);  //((float) temperature/ 4096) * 2.5;
	SendStringPC((char *)"[tmpVolt:");
	SendFloatPC(temperatureVoltage);
	SendStringPC((char *)"]");
	
	#ifdef TMP36
		double temperatureFloat = 100.0 * temperatureVoltage - 50.0;
	#endif
	
	return temperatureFloat;
}

double getElectronicsBatteryVoltage(){

	int avgVal = 50;
	uint32_t sum = 0;
	
	for(int i = 0; i < avgVal; ++i){
		sum += sampleBatteryVoltage();
	}
	uint16_t electronicsVoltageCount = sum / avgVal;
	double electronicsVoltage = ADCCountToVoltage(electronicsVoltageCount);
	double calculatedElectronicsVoltage =  (electronicsVoltage / .56) + (10.0 - .05);

	return calculatedElectronicsVoltage;
}

double getSystemCurrent(uint8_t currentSelect){
	
	return 7.7;
}

