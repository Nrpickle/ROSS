/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015
 * Author : Nick McComb | nickmccomb.net
 
 TODOs
 
 - Ensure accurate ambient temperature sensor measurements
 - Enable all current sensing appropriately
 - Enable current sense fault detection (is this actually desired?)

 COMPLETED
 - Enable PWM passthrough / steering control
 - Enable PWM Override
 - Enable On/Off passthrough
 */ 

#define FIRMWARE_VERSION_STR ".2"
#define FIRMWARE_VERSION .2

#include "main.h"

volatile RSSI_type RSSI;

//Global Variables *gasp*
volatile int toggle = 0;
volatile int temp = 1000;
uint8_t remoteInput = 0;
uint8_t remoteOutputCountdown = 0;  //Count to "only output start stop state for 5 states"

uint8_t  pixhawkOverride = 0;			//Shall we override the Pixhawk's PWM signal? :sunglasses:
uint16_t manualPWMOutput = 1500;		//uS of desired override PWM output
uint16_t pixhawkOverrideCountdown = 0;	//Amount of message cycles to maintain override

#define REMOTE_START_CHECK 0x1
#define REMOTE_STOP_CHECK  0x2

int main(void)
{
	configureIO();
	configureExternalOscillator();
	configureUSART();					//Set up for 57600 Baud
	configureTimerCounter();
	configureADCs();
	configureRTC();
	configureXCL();
	
	LOW_LEVEL_INTERRUPTS_ENABLE();
	MED_LEVEL_INTERRUPTS_ENABLE();
	HIGH_LEVEL_INTERRUPTS_ENABLE();
	sei();								//Enable global interrupts
	
	uint8_t receivedUSARTData;
	
	RSSI.measuring = NOT_MEASURING;
	RSSI.timeDifference = 0;
	RSSI.sampleCount = 0;

	//Init string with basic documentation
	SendStringPC((char *)"#[INIT ROSS PDB]\n\r");
	SendStringPC((char *)"#Firmware version ");
	SendStringPC((char *)FIRMWARE_VERSION_STR);
	SendStringPC((char *)"\n\r#Msg format: Electronics Batt Volt | Rear Batt Volt | Ebox Temperature | 5v_SYS Curr | 5v_Comp Curr | XTend RSSI | \"Remote Input\" \n\r");
	
	
    while (1) 
    {

		_delay_ms(1);		

		//Check for commands from the computer
		if(USART_IsRXComplete(&COMP_USART)){
			receivedUSARTData = USART_GetChar(&COMP_USART);
			
			//Check if the inputted command is within the range to be
			//forwarded to the ON/OFF switch
			if(receivedUSARTData >= 30 && receivedUSARTData <= 65){
				SendCharONOFF(receivedUSARTData);
			}
			
			if(receivedUSARTData == 'y')
				REAR_RELAY_SET();
			else if(receivedUSARTData == 'n')
				REAR_RELAY_CLR();
			
			if(receivedUSARTData == 70){  //Then we need to cancel our override (if it exists)
				pixhawkOverride = 0;
				pixhawkOverrideCountdown = 0;
			}
			
			if(receivedUSARTData >= 71 && receivedUSARTData <= 87){		//Then we need to process a PWM override
				pixhawkOverride = 1;
				manualPWMOutput = receivedUSARTData * 50 - 2450;  //This will generate the correct output per https://goo.gl/7wYL6b
				if(CHECK_DIP_SW_2()){	//5 second update time
					if(TCC4.PER == TC_1024_100MS)
						pixhawkOverrideCountdown = 50;
					else if(TCC4.PER == TC_1024_500MS)
						pixhawkOverrideCountdown = 10;
					else
						pixhawkOverrideCountdown = 50;
				}
				else {	//1 second update time
					if(TCC4.PER == TC_1024_100MS)
						pixhawkOverrideCountdown = 10;
					else if(TCC4.PER == TC_1024_500MS)
						pixhawkOverrideCountdown = 2;
					else
						pixhawkOverrideCountdown = 10;
				}
			}
		}		
		
		//Check for commands from the ON/OFF Switch
		if(USART_IsRXComplete(&ONOFF_USART)){
			receivedUSARTData = USART_GetChar(&ONOFF_USART);
			//CHECK FOR IF START OR STOP COMMAND
			if(receivedUSARTData == 10){	//Remote start requested
				remoteInput = REMOTE_START_CHECK;
				remoteOutputCountdown = STATIC_STATUS_OUTPUT_COUNT;
			}	
			else if(receivedUSARTData == 20){	//Remote stop requested
				remoteInput = REMOTE_STOP_CHECK;
				remoteOutputCountdown = STATIC_STATUS_OUTPUT_COUNT;
			}
		}
		
		if(pixhawkOverride){	//If we do want to override the signal
			TC_PWM_SET(manualPWMOutput);	//Output the desired override PWM output
		}
		else {  //We don't want to overrride the signal
			TC_PWM_SET(steeringPWMPeriod);	//Output the Pixhawk PWM signal
		}
		
		if(broadcastStatus){  //This variable becomes true every interval that the user wants info reported
			broadcastStatus = 0;
			
			TCC4.CNT = 0;	//We want to ensure the counter is 0 so that we can 
							//have a consistent report time. (We don't want to throw
							//out the accuracy of the TC)
			
			//Calculated desired output values
			double EBoxTemp = getEBoxTemperature();
			double electronicsBatteryVoltage = getElectronicsBatteryVoltage();
			double zero = 0.0;
			
			getXTendRSSI();	//This is better here than in an interrupt because the frequency
							//of the interpreted PWM signal is so high, there's no reason
							//to sample it continuously. Also, on the same side of the coin,
							//it is quick to sample.
			
			
			//Actually output the desired values
			//Not the most elegant code in the world, but it works...
			
			SendFloatPC(electronicsBatteryVoltage);	//Send the battery voltage
			SendStringPC((char *)"|");
			SendFloatPC(zero);		//Send the rear battery voltage
			SendStringPC((char *)"|");
			SendFloatPC(EBoxTemp);	//Send the EBox Temperature
			SendStringPC((char *)"|");
			SendFloatPC(zero);		//Send 5v_SYS Curr
			SendStringPC((char *)"|");
			SendFloatPC(zero);		//Send 5v_SYS Curr
			SendStringPC((char *)"|");
			SendNumPC(RSSI.value);
			if(RSSI.value == 0)
				SendNumPC(zero);
			SendStringPC((char *)"|");
			SendNumPC(remoteInput);
			//SendStringPC((char *)"|");
			
			debuggingOutput();
			
			//Newline
			SendStringPC((char *)"\n\r");
	
			//Check if we have outputs that need to "expire"
			if(--remoteOutputCountdown == 0){
				remoteInput = 0;
			}
			
			//Check on the output overriding
			if(pixhawkOverrideCountdown){	//If we are still counting down, this also means that
											//we are currently overriding our output
				--pixhawkOverrideCountdown;	//Decrement our counter
			}
			else {	//If we are not counting down, then we want to ensure we are outputting the Pixhawk PWM
				pixhawkOverride = 0;
			}
	
			//Check the updating speed setting
			//The speed shouldn't be set lower than maybe 75mS due to RSSI processing time
			if(CHECK_DIP_SW_1()){
				TCC4.PER = TC_1024_100MS;  //100mS delay
			}
			else{
				TCC4.PER = TC_1024_500MS;  //500mS delay
			}
		}
    }
}

uint16_t inline getXTendRSSI(){
	RTC.CNT = 0;
				
	//Get RSSI from XTend
	RSSI.measuring = 0;
	do{   //Wait until we have a "Low" signal on the RSSI (wait for this ----\_____)
		_delay_us(50);
					
		if(RTC.CNT > RSSI_MAX_COUNT)
		break;
					
	}while(READ_RSSI_PIN());
				
	do{  //Wait until we have a "High" signal on the RSSI (wait for this ____/----)
		_delay_us(50);
					
		if(RTC.CNT > RSSI_MAX_COUNT)
		break;
	}while(!READ_RSSI_PIN());
				
	RTC.CNT = 0;  //Start counting
				
	do{   //Wait until we have a "Low" signal on the RSSI (wait for this ----\_____)
		_delay_us(50);
					
		if(RTC.CNT > RSSI_MAX_COUNT)
		break;
	}while(READ_RSSI_PIN());
				
	RSSI.countDifference = RTC.CNT;
	
	RSSI.value = (100 * RSSI.countDifference) / RSSI_MAX_COUNT;
				
	RSSI.sampleCount++;
	
	return RSSI.countDifference;
}

//Secret sauce
double ADCCountToVoltage(uint16_t adcCount){
  
  //Testing and comparing voltages to corresponding count values resulted in this fun function:
  return adcCount * 0.0011982182628062362 + 0.0023407572383072894; //I figure the compiler will trim off what it can't actually use...

	
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
	
	#ifdef OUTPUT_TEMP_SENSOR_VOLTAGE
	SendStringPC((char *)"[tmpVolt:");
	SendFloatPC(temperatureVoltage);
	SendStringPC((char *)"]");
	#endif
	
	double temperatureFloat;
	
	#ifdef TMP36
	#ifdef TMP37
		#error "Too many temperature sensors were selected"
		temperatureVoltage = temperatureVoltage;
	#endif
	#endif
	
	#ifndef TMP36
	#ifndef TMP37
		#error "A temperature sensor was not selected!"
		temperatureVoltage = temperatureVoltage;
	#endif
	#endif
	
	#ifdef TMP36
		temperatureFloat = 100.0 * temperatureVoltage - 50.0;
	#endif
	#ifdef TMP37
		temperatureFloat = 50.0 * temperatureVoltage;
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

void inline debuggingOutput(){

	#ifdef PWM_DEBUGGING_OUTPUT	
		SendStringPC((char *)"\t[PWM Interpret: ");
		SendNumPC(steeringPWMPeriod);
		SendStringPC((char *)"] ");
	#endif
	
	#ifdef RSSI_DEBUGGING_OUTPUT
		SendStringPC((char *)"\tRSSI Samples: ");
		SendNumPC(RSSI.sampleCount);
		//			SendStringPC((char *)"RTC Counter Value: ");
		//			SendNumPC(RTC.CNT);
		SendStringPC((char *)"\tRSSI Count Value: ");
		SendNumPC(RSSI.countDifference);
	#endif
}