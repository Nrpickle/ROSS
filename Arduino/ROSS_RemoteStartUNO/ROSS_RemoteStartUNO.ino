#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
//#include <avr/sleep.h>

/*

ROSS R/C Test File

Nick McComb [mccombn@onid.oregonstate.edu]
Written August 2015 [at sea]

Arduino UNO Pin Allocation

D4 - R/C Channel Input
D3 - KILL Relay Output
D2 - START Relay Output


*/

const byte RC_INPUT = 4;
const byte KILL_RELAY = 3;
const byte START_RELAY = 2;
const short SAFETY_THRESHOLD = 24; //The test loop runs at ~4hz, so SAFETY_THRESHOLD/4 = no. seconds, eg 24 = ~6seconds
                                   //NOTE: This variable cannot go higher than 254 at the moment, you need to change the declaration 

//#define DEBUG  //Use to include debug-related strings. Sends a lot of text, usually.
#define DEBUG_MINIMAL

/* GPS RELATED DECLARATIONS */
SoftwareSerial GPSSerial(8, 7);
Adafruit_GPS GPS(&GPSSerial);

//Set whether you want to echo the GPS data to the serial console
#define GPSECHO true
#define LOG_FIXONLY false //Log only when we have a GPS fix?

boolean usingInterrupt = false;
void useInterrupt(boolean);  //<-- totes not necessary TODO remove this line

const byte chipSelect = 10;
const byte ledPin = 13;

File logfile;

// read a Hex value and return the decimal equivalent
uint8_t parseHex(char c) {
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

//Note: Handy little binkly blinky function (thanks, Ada!)
// blink out an error code
void error(uint8_t errno) {
  /*
  if (SD.errorCode()) {
   putstring("SD error: ");
   Serial.print(card.errorCode(), HEX);
   Serial.print(',');
   Serial.println(card.errorData(), HEX);
   }
   */
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(ledPin, HIGH);
      delay(100);
      digitalWrite(ledPin, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}


void setup(){
  Serial.begin(115200); 
  Serial.println("[Begin ROSS GPS Log and Remote Start]");
  
  pinMode(RC_INPUT, INPUT);
  pinMode(KILL_RELAY, OUTPUT);
  pinMode(START_RELAY, OUTPUT);
  
  pinMode(ledPin, OUTPUT);
  
  //GPS mode setting
  pinMode(chipSelect, OUTPUT);
  
  //see if the sd card is present, and can be initialized
  if (!SD.begin(chipSelect, 11, 12, 13)) {
    //if (!SD.begin(chipSelect)) {      // if you're using an UNO, you can use this line instead
    Serial.println("#!! Card init failed !!#");
    error(2);
  }
  
  
  char altfilename[15];
  strcpy(altfilename, "GPS000.TXT");
  
  for (int i = 0; i < 1000; i++) {
    Serial.println(i);
    //altfilename[3] = '0' + i/100;
    altfilename[4] = '0' + i/10;
    altfilename[5] = '0' + i%10;
    Serial.println(altfilename);
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(altfilename)) {
      Serial.println("Break!");
      break;
    }
    
  }
  logfile = SD.open(altfilename, FILE_WRITE);
  //while(1);
  /* */
  
  char filename[15];
  strcpy(filename, "GPSLOG00.TXT");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
  }
  
  //logfile = SD.open(filename, FILE_WRITE);
  if (!logfile) {
    Serial.print("#!! Couldn't create ");
    Serial.print(filename);
    Serial.println(" !!#");
    error(3);
  }
  Serial.print("# Writing to ");
  Serial.print(altfilename);
  Serial.println(" #");
  
  //Open GPS connection
  GPS.begin(9600);
  
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  GPS.sendCommand(PGCMD_NOANTENNA);
  
  useInterrupt(true);
  
  Serial.println("# Init finished. Ready to run main program. #");
}


// Interrupt is called once a millisecond, looks for any new GPS data, and stores it
SIGNAL(TIMER0_COMPA_vect) {
  char c = GPS.read();
  // if you want to debug, this is a good time to do it!
  #ifdef UDR0
      if (GPSECHO)
        if (c) UDR0 = c;  
      // writing direct to UDR0 is much much faster than Serial.print 
      // but only one character can be written at a time. 
  #endif
}

void useInterrupt(boolean v) {
  if (v) {
    // Timer0 is already used for millis() - we'll just interrupt somewhere
    // in the middle and call the "Compare A" function above
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    usingInterrupt = true;
  } 
  else {
    // do not call the interrupt function COMPA anymore
    TIMSK0 &= ~_BV(OCIE0A);
    usingInterrupt = false;
  }
}

void loop(){
  
  
  
  static byte safetyCounter = 0; //Init this var as 0, this variable will count the number of iterations that a kill or start signal is sent, so that they are not sent for more than about 10 seconds. 
                                //If this variable reaches too high, then no relay can activate
  
  
  //Check only ~4hz, but allow GPS to check properly
  digitalWrite(ledPin, LOW);
  for(byte i = 0; i < 12; ++i){
    if (i == 0){
      //Method to check RC Signal Length
      short RC7_len;
      RC7_len = pulseIn(RC_INPUT, HIGH, 20500);  //Timeout is 20.5 mS because that's the "frame" of a PWM signal
      #ifdef DEBUG
        Serial.print("RC7 Value: ");
        Serial.println(RC7_len);
      #endif
        
      if(!RC7_len){  //RC Transmitter not-on, do nothing
        #ifdef DEBUG_MINIMAL
          Serial.println("[Off]");
        #endif
        #ifdef DEBUG
          Serial.println("## RC Transmitter not on! ##");
        #endif
        digitalWrite(KILL_RELAY, LOW);
        digitalWrite(START_RELAY, LOW);
        //safetyCounter = 0; //We do not want to reset the safetyCounter here, because when
                             //we shut off the remote, it will drop here for a few seconds 
                             //returning to the previous value
      }
      else if (RC7_len < 1300 && RC7_len > 800){  //Switch in low position, start engine
        #ifdef DEBUG_MINIMAL
          Serial.println("[Low]");
        #endif
        #ifdef DEBUG
          Serial.println("[R/C Transmitter in low position]");
        #endif
        if(safetyCounter < SAFETY_THRESHOLD){
          digitalWrite(KILL_RELAY, LOW);
          digitalWrite(START_RELAY, HIGH);
        }
        else {
          digitalWrite(KILL_RELAY, LOW);
          digitalWrite(START_RELAY, LOW);
        }
        ++safetyCounter;
      }
      else if (RC7_len > 1300 && RC7_len < 1700){  //Switch in middle position, do nothing
        #ifdef DEBUG_MINIMAL
          Serial.println("[Mid]");
        #endif
        #ifdef DEBUG
          Serial.println("[R/C Transmitter in middle position]");
        #endif
        digitalWrite(KILL_RELAY, LOW);
        digitalWrite(START_RELAY, LOW);
        safetyCounter = 0;
      }
      else if (RC7_len > 1700){ //Switch in high position, kill engine
        #ifdef DEBUG_MINIMAL
          Serial.println("[High]");
        #endif
        #ifdef DEBUG
          Serial.println("[R/C Transmitter in high position]");
        #endif
        if(safetyCounter < SAFETY_THRESHOLD){
          digitalWrite(KILL_RELAY, HIGH);
          digitalWrite(START_RELAY, LOW);        
        }
        else{
          digitalWrite(KILL_RELAY, LOW);
          digitalWrite(START_RELAY, LOW);        
        }
        ++safetyCounter;
      }
    #ifdef DEBUG
      if(digitalRead(KILL_RELAY)){
        Serial.println("KILL_RELAY activated");
      }
      else {
        Serial.println("KILL_RELAY deactivated");
      }
      if(digitalRead(START_RELAY)){
        Serial.println("START_RELAY activated");
      }
      else {
        Serial.println("START_RELAY deactivated");
      }
    #endif
    }
    else {  //Do GPS stuff 
      if (!usingInterrupt) {
        // read data from the GPS in the 'main loop'
        char c = GPS.read();
        // if you want to debug, this is a good time to do it!
        if (GPSECHO)
          if (c) Serial.print(c);
      }
      
      // if a sentence is received, we can check the checksum, parse it...
      if (GPS.newNMEAreceived()) {
        // a tricky thing here is if we print the NMEA sentence, or data
        // we end up not listening and catching other sentences! 
        // so be very wary if using OUTPUT_ALLDATA and trying to print out data
        
        // Don't call lastNMEA more than once between parse calls!  Calling lastNMEA 
        // will clear the received flag and can cause very subtle race conditions if
        // new data comes in before parse is called again.
        char *stringptr = GPS.lastNMEA();
        
        if (!GPS.parse(stringptr))   // this also sets the newNMEAreceived() flag to false
          return;  // we can fail to parse a sentence in which case we should just wait for another

        // Sentence parsed! 
        Serial.println("OK");
        if (LOG_FIXONLY && !GPS.fix) {
          Serial.print("No Fix");
          return;
        }

        // Rad. lets log it!
        Serial.println("Log");

        uint8_t stringsize = strlen(stringptr);
        if (stringsize != logfile.write((uint8_t *)stringptr, stringsize))    //write the string to the SD file
            error(4);
        if (strstr(stringptr, "RMC") || strstr(stringptr, "GGA"))   logfile.flush();
        Serial.println();
      }
      
      for(int j = 0; j < 20; ++j){
        delay(1);
      }
    }
  } 
  
  //delay(100); //Poor lonely delay
}
