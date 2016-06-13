/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015
 * Author : Nick McComb | nickmccomb.net
 
 TODOs
 

 - Enable all current sensing appropriately
 - Enable current sense fault detection (is this actually desired?)

 COMPLETED
 - Enable PWM passthrough / steering control
 - Enable PWM Override
 - Enable On/Off passthrough
 - Ensure accurate ambient temperature sensor measurements
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
	configureSerialNumber();
		
	LOW_LEVEL_INTERRUPTS_ENABLE();
	MED_LEVEL_INTERRUPTS_ENABLE();
	HIGH_LEVEL_INTERRUPTS_ENABLE();
	sei();								//Enable global interrupts
	
	uint8_t receivedUSARTData;
	
	RSSI.measuring = NOT_MEASURING;
	RSSI.timeDifference = 0;
	RSSI.sampleCount = 0;

	//Init string with basic documentation
	SendStringPC("\n\r#[INIT ROSS PDB]\n\r");
	SendStringPC("#Firmware version ");
	SendStringPC(FIRMWARE_VERSION_STR);
	SendStringPC("\n\r#Serial Number: ");
	if(serialNumber == -1)
		SendStringPC("NOT SET");
	else
		SendNumPC(serialNumber);
	SendStringPC("\n\r#Msg format: Electronics Batt Volt | Rear Batt Volt | Ebox Temperature | 5v_SYS Curr | 5v_Comp Curr | Throttle Current | XTend RSSI | \"Remote Input\" \n\r");
	
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
			
			//Rear relay processing
			if(receivedUSARTData == 'y')
				REAR_RELAY_SET();
			else if(receivedUSARTData == 'n')
				REAR_RELAY_CLR();
			
			//Override canceling
			if(receivedUSARTData == 70){  //Then we need to cancel our override (if it exists)
				pixhawkOverride = 0;
				pixhawkOverrideCountdown = 0;
			}
			
			//Steering Override processing
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
			double electronicsBatteryVoltage = getBatteryVoltage(ELECTRONICS);
			double rearBatteryVoltage = getBatteryVoltage(REAR_BATT);
			//double XTendCurrent = getSystemCurrent(THROTTLE);
			double sysCurrent = getSystemCurrent(SYS_5V);
			double compCurrent = getSystemCurrent(COMP);
			double throttleCurrent = getSystemCurrent(THROTTLE);
			//double zero = 0.0;
			
			getXTendRSSI();	//This is better here than in an interrupt because the frequency
							//of the interpreted PWM signal is so high, there's no reason
							//to sample it continuously. Also, on the same side of the coin,
							//it is quick to sample.
			
			
			//Actually output the desired values
			//Not the most elegant code in the world, but it works...
			
			SendFloatPC(electronicsBatteryVoltage);	//Send the battery voltage
			SendStringPC("|");
			if(rearBatteryVoltage < 0)
				SendFloatPC((double) 0);
			else
				SendFloatPC(rearBatteryVoltage);		//Send the rear battery voltage
			SendStringPC("|");
			SendFloatPC(EBoxTemp);	//Send the EBox Temperature
			SendStringPC("|");
			SendFloatPC(sysCurrent);		//Send 5v_SYS Curr
			SendStringPC("|");
			SendFloatPC(compCurrent);		//Send computer Curr (RPi)
			SendStringPC("|");
			SendFloatPC(throttleCurrent);
			SendStringPC("|");
			SendNumPC(RSSI.value);
			if(RSSI.value == 0)
				SendStringPC("0");
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
			//Be careful if you change these, as they are referenced elsewhere (e.g. Override controls)
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
  switch(serialNumber){
	  case 1:
		return adcCount * 0.0011982182628062362 + 0.0023407572383072894; //I figure the compiler will trim off what it can't actually use...
	  default:
		return 1;
  }
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

int16_t sampleBatteryVoltage(uint8_t batterySelect){
	if(batterySelect == ELECTRONICS)
		ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN9_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for batt voltage sense
	else //if (batterySelect == REAR_BATT)
		ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN10_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for rear batt voltage sense
		
	ADCA.CH0.CTRL |= ADC_CH_START_bm;
	
	//while(((ADCA.INTFLAGS >> ADC_CH0IF_bp) & (1U << 0)) != (1U << 0)); // (1U << n) where n is the adc channel, so zero for this one
	//ADCA.INTFLAGS = ((1U << 0) << ADC_CH0IF_bp);
	while(!(ADCA.INTFLAGS & (1 << 0)));
	ADCA.INTFLAGS = (1 << 0);
	
	return 	ADCA.CH0.RES;
}

uint16_t sampleCurrentSensor(uint8_t currentSelect){
	switch(currentSelect){
		case XTEND:
			ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN3_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for xtend current sense
			break;
		case COMP:
			ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN4_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for RPi current sense
			break;
		case SYS_5V:
			ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN5_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for 5v_SYS current sense
			break;
		case THROTTLE:
			ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN6_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for Throttle servo current sense
			break;
		case REAR:
			ADCA.CH0.MUXCTRL = (ADC_CH_MUXPOS_PIN7_gc | ADC_CH_MUXNEGL_PIN1_gc); //PIN for Rear Battery current sense
			break;
	}
		
	ADCA.CH0.CTRL |= ADC_CH_START_bm;		//Start the ADC
	
	while(!(ADCA.INTFLAGS & (1 << 0)));		//Wait until the ADC has finished processing
	ADCA.INTFLAGS = (1 << 0);				//Reset the interrupt flag
	
	return 	ADCA.CH0.RES;					//Return the result of the ADC calculation
	
	
	return 7;
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

double getBatteryVoltage(uint8_t batterySelect){

	int avgVal = 100;
	uint32_t sum = 0;
	
	for(int i = 0; i < avgVal; ++i){
		sum += sampleBatteryVoltage(batterySelect);
	}
	uint16_t electronicsVoltageCount = sum / avgVal;
	
	#ifdef BATT_VOLTAGE_RAW_COUNT_OUTPUT
		SendStringPC((char *)"[Raw Volt Count: ");
		SendNumPC(electronicsVoltageCount);
		SendStringPC((char *)"] ");
	#endif
	
	double electronicsVoltage = ADCCountToVoltage(electronicsVoltageCount);
	double calculatedElectronicsVoltage =  -4.1274 * pow(electronicsVoltage, 4) + 29.1147 * pow(electronicsVoltage, 3) - 75.1330 * pow(electronicsVoltage, 2) + 85.6459 * electronicsVoltage - 24.1509;
	
	//original, calculated sol'n: (electronicsVoltage / .56) + (10.0 - .05);

	#ifdef BATT_VOLTAGE_RAW_OUTPUT
	SendStringPC((char *)"[Raw Voltage: ");
	SendFloatPC(electronicsVoltage);
	SendStringPC((char *)"] ");
	#endif

	return calculatedElectronicsVoltage;
}


double getSystemCurrent(uint8_t currentSelect){
	
	int avgVal = 100;
	uint32_t sum = 0;
	
	for(int i = 0; i < avgVal; ++i){
		sum += sampleCurrentSensor(currentSelect);
	}
	uint16_t currentVoltageCount = sum / avgVal;
	
	#ifdef CURRENT_COUNT_RAW_OUTPUT
	SendStringPC((char *)"[Raw Volt Count: ");
	SendNumPC(currentVoltageCount);
	SendStringPC((char *)"] ");
	#endif
	
	double currentSenseVoltage = ADCCountToVoltage(currentVoltageCount);
	double calculatedCurrent;
	
	double preampCurrentSenseVoltage;
	if(currentSelect == XTEND){  //The XTend current is based on 5v supply, and doesn't go through an OpAmp
		preampCurrentSenseVoltage = 7.7; //This measurement is actually impossible... whoops
		calculatedCurrent = 7.7;
	}
	else{  //Anyother case besides XTend (e.g. ACS powered with 3v3
		preampCurrentSenseVoltage = -(currentSenseVoltage/1.5) + 3.3;
		calculatedCurrent = (preampCurrentSenseVoltage - 1.65) / 0.055;
	}

	#ifdef CURRENT_VOLTAGE_RAW_OUTPUT
	SendStringPC((char *)"[Raw Voltage (preamp): ");
	SendFloatPC(preampCurrentSenseVoltage);
	SendStringPC((char *)"] ");
	#endif

	return calculatedCurrent;
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



