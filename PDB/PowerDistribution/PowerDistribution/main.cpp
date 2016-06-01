/*
 * PowerDistribution.cpp
 * 
 * Made for the ROSS project. To view the code for this project, go to: 
 * https://github.com/Nrpickle/ROSS
 *
 * Created: 12/22/2015
 * Author : Nick McComb | nickmccomb.net
 
 TODOs
 
 - Enable accurate ambient temperature sensor measurements
 - Enable PWM passthrough / steering control
 - Enable On/Off passthrough
 - Enable all current sensing appropriately
 - Enable current sense fault detection
 
 */ 

#define FIRMWARE_VERSION_STR ".2"
#define FIRMWARE_VERSION .2

#include "main.h"

volatile RSSI_type RSSI;

//Global Variables *gasp*
volatile int toggle = 0;
volatile int temp = 1000;

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
		
		TC_PWM_SET(steeringPWMPeriod);
		
		if(broadcastStatus){  //This variable becomes true every interval that the user wants info reported
			broadcastStatus = 0;
			
			TCC4.CNT = 0;	//We want to ensure the counter is 0 so that we can 
							//have a consistent report time. (We don't want to throw
							//out the accuracy of the TC)
			
			//Calculated desired output values
			double EBoxTemp = getEBoxTemperature();
			double electronicsBatteryVoltage = getElectronicsBatteryVoltage();
			double zero = 0.0;
			
			STATUS_TOGGLE();
			
			/*
			
			//Get RSSI from XTend
			RSSI.measuring = 0;
			do{   //Wait until we have a "Low" signal on the RSSI (wait for this ----\_____)
				_delay_us(50);
			}while(READ_RSSI_PIN());
			
			do{  //Wait until we have a "High" signal on the RSSI (wait for this ____/----)
				_delay_us(50);
			}while(!READ_RSSI_PIN());
			
			RTC.CNT = 0;  //Start counting
			
			do{   //Wait until we have a "Low" signal on the RSSI (wait for this ----\_____)
				_delay_us(50);
			}while(READ_RSSI_PIN());
			
			RSSI.countDifference = RTC.CNT;
			
			// DO MATH TO GET LENGTH OF PULSE
			
			RSSI.sampleCount++;
			
			*/
			
			
			
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
			
			
			SendStringPC((char *)"[PWM Interpret: ");
			SendNumPC(steeringPWMPeriod);
			SendStringPC((char *)"] ");
			
			/*
			
			SendStringPC((char *)"RSSI Samples: ");
			SendNumPC(RSSI.sampleCount);
//			SendStringPC((char *)"RTC Counter Value: ");
//			SendNumPC(RTC.CNT);
			SendStringPC((char *)"\tRSSI Count Value: ");
			SendNumPC(RSSI.countDifference);
			
			*/
			
			//Newline
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

