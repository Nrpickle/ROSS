#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <avr/sleep.h>

SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false //Set to true if you want to echo to serial monitor
#define LOG_FIXONLY false //Set to true if you only want to log when GPS is locked

boolean usingInterrupt = false; //Set to true to use interrupt

bool started = false;
#define startPin //Goes high upon button push. NC switch connects to GND. Pulled up internally.
#define startLED //Illuminates when start button is pushed
#define startMotor //Starts the motor when held high
#define startTime //How long startMotor is held high (in milliseconds)
#define stopPin
#define stopLED
#define stopMotor

// Set the pins used
#define chipSelect 10
#define ledPin 13

File logfile;

uint8_t parseHex(char c) { //Translate from hexadecimal to decimal 
  if (c < '0')
    return 0;
  if (c <= '9')
    return c - '0';
  if (c < 'A')
    return 0;
  if (c <= 'F')
    return (c - 'A')+10;
}

// blink error code
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

void setup() {
  Serial.begin(115200);
  Serial.println("\r\nUltimate GPSlogger Shield");
  pinMode(ledPin, OUTPUT);

  pinMode(10, OUTPUT); //Default chip select

  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card init. failed!");
    error(2);
  }
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

  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    error(3);
  }
  Serial.print("Writing to "); 
  Serial.println(filename);

  // connect to the GPS at the desired rate
  GPS.begin(9600);

  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); //RCM and GGA

  // Set the update rate
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_5HZ);  //100 millihertz (once every 10 seconds), 1Hz or 5Hz update rate

  // Turn off updates on antenna status, if the firmware permits it
  GPS.sendCommand(PGCMD_NOANTENNA);

  useInterrupt(true);

  Serial.println("Ready!");
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

void loop() {
  if(startPin == HIGH)
    localStart();
  if(stopPin == HIGH)
    localStop();
  if (! usingInterrupt) {
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
      return;  // Wait for a new sentence if fail to parse

    // Sentence successfully parsed 
    Serial.println("OK");
    if (LOG_FIXONLY && !GPS.fix) {
      Serial.print("No Fix");
      return;
    }

    //Log parsed data
    Serial.println("Log");

    uint8_t stringsize = strlen(stringptr);
    if (stringsize != logfile.write((uint8_t *)stringptr, stringsize))    //write the string to the SD file
        error(4);
    if (strstr(stringptr, "RMC") || strstr(stringptr, "GGA"))   logfile.flush();
    Serial.println();
  }
}

void localStart{
  if(started == false){
    digitalWrite(stopMotor, LOW);
    digitalWrite(startMotor, HIGH);
    delay(startTime);
    digitalWrite(startMotor, LOW);
    pulseLED(startLED);
    started = true;
  }
  else
    flickerLED(startLED);
}

void localStop{
  if(started == true){
    digitalWrite(startMotor, LOW); //Redundant for safety
    digitalWrite(stopMotor, HIGH); //Keep stop held high
    pulseLED(stopLED);
  }
  else
    flickerLED(stopLED);
}
