#include <SPI.h>
#include <Adafruit_GPS.h>
#include <SoftwareSerial.h>
#include <SD.h>
#include <avr/sleep.h>
#include <Adafruit_NeoPixel.h>

SoftwareSerial mySerial(8, 7);
Adafruit_GPS GPS(&mySerial);

#define GPSECHO  false //Set to true if you want to echo to serial monitor
#define LOG_FIXONLY false //Set to true if you only want to log when GPS is locked

boolean usingInterrupt = false; //Set to true to use interrupt.3.

#define startPin 4//Goes high upon button push. NC switch connects to GND. Pulled up internally.
#define startLED 5 //Pulses when start button is pushed
#define stopPin 2
#define stopLED 3
#define LEDRing 6

Adafruit_NeoPixel ring = Adafruit_NeoPixel(12, LEDRing, NEO_GRB + NEO_KHZ800);
uint32_t colorVal = ring.Color(0,127,0); //Initialize the ring to half brightness green
//uint32_t colorVal = ring.Color(0,0,0); //Initialize the ring to off
int color = 0;
int pattern = 0;
int brightness = 0;
unsigned long previousBlinkTime = 0;
unsigned long currentBlinkTime = 0;
unsigned long previousPulseTime = 0;
unsigned long currentPulseTime = 0;
unsigned long previousRedButtonTime = 0;
unsigned long currentRedButtonTime = 0;
unsigned long previousGreenButtonTime = 0;
unsigned long currentGreenButtonTime = 0;
#define buttonPulseTime 5
#define blinkTime 500 //How frequently the ring will blink in milliseconds
#define pulseTime 2
int i = 0;
float per = 90;
float freq = 1.0/per;
bool buttonPulseRed = false;
bool buttonPulseGreen = false;
bool on = false;
int n = 0;
bool increase = true;
bool skip = false;
int pulseSpeed = 2; //Default pulse speed. Can be altered via serial.

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
  Serial.begin(9600); //Increase to 115200 baud rate if echoing to serial monitor 
  Serial.println("\r\nUltimate GPSlogger Shield");
  ring.begin();
  ring.show();
  solid(colorVal); //Initialize as colorVal's default (set at top)
  pinMode(ledPin, OUTPUT);

  pinMode(10, OUTPUT); //Default chip select
  pinMode(startPin, INPUT_PULLUP);
  pinMode(startLED, OUTPUT);
  pinMode(stopPin, INPUT_PULLUP);
  pinMode(startLED, OUTPUT);
  pinMode(LEDRing, OUTPUT);
  analogWrite(startLED, 127);
  analogWrite(stopLED, 127);

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
  currentBlinkTime = millis();
  currentPulseTime = millis();
  currentGreenButtonTime = millis();
  currentRedButtonTime = millis();
  if(pattern == 2){
    if(currentBlinkTime - previousBlinkTime >= blinkTime){
      LEDblink();
      previousBlinkTime = currentBlinkTime;
    }
  }
  else if(pattern == 3){
    if(currentPulseTime - previousPulseTime >= pulseTime){
      LEDpulse();
      previousPulseTime = currentPulseTime;
    }
  }
  if(digitalRead(startPin) == HIGH){
    Serial.write(10);
    buttonPulseGreen = true;
  }

  if(buttonPulseGreen == true){
    if(currentGreenButtonTime - previousGreenButtonTime >= buttonPulseTime){ 
      pulseLED(startLED);
      previousGreenButtonTime = currentGreenButtonTime;
    }
  }
  
  if(digitalRead(stopPin) == HIGH){
    Serial.write(20);
    buttonPulseRed = true;
  }
  
  if(buttonPulseRed == true){
    if(currentRedButtonTime - previousRedButtonTime >= buttonPulseTime){
      pulseLED(stopLED);
      previousRedButtonTime = currentRedButtonTime;
    }
  }
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

void pulseLED(int led){
  if(i < (2*per+2)){
    int brightness = 127.0*sin(freq*2*PI*i)+127;
    analogWrite(led, brightness);
    i++;
  }
  else{
    analogWrite(led, 127);
    i = 0;
    buttonPulseGreen = false;
    buttonPulseRed = false;
  }
}

void serialEvent(){
  int val = Serial.read();
  if(val == 33)
    color = 1; //red
  else if(val == 34)
    color = 2; //green
  else if(val == 35)
    color = 3; //blue
  else if(val == 36)
    color = 4; //yellow
  else if(val == 41)
    pattern = 1; //solid
  else if(val == 42)
    pattern = 2; //blink
  else if(val == 43)
    pattern = 3; //pulse
  else if(val == 51)
    brightness = 0;
  else if(val == 52)
    brightness = 1;
  else if(val == 53)
    brightness = 2;
  else if(val == 61)
    pulseSpeed = 1;
  else if(val == 62)
    pulseSpeed = 2;
  
  //Multiply value by brightness/2 to set intensity to
  //0 , 1/2 , or 1 [Brightness value can be 0, 1, or 2]
  if(color == 1)
    colorVal = ring.Color(brightness*255/2, 0, 0); 
  else if(color == 2)
    colorVal = ring.Color(0, brightness*255/2, 0);
  else if(color == 3)
    colorVal = ring.Color(0, 0, brightness*255/2);
  else if(color == 4)
    colorVal = ring.Color(brightness*255/2, brightness*255/2, 0);
  else
    colorVal = ring.Color(0,0,0);
  
  if(pattern == 1){
    solid(colorVal);
  }
}

void solid(uint32_t color){
  for(int i = 0; i < ring.numPixels(); i++)
    ring.setPixelColor(i, color);
  ring.show();
}

void LEDblink(){
  if(on == false){
    for(int i = 0; i < ring.numPixels(); i++)
      ring.setPixelColor(i, colorVal);
    ring.show();
    on = true;
  }
  else if(on == true){
    for(int i = 0; i < ring.numPixels(); i++)
      ring.setPixelColor(i, 0, 0, 0);
    ring.show();
    on = false;
  }
}

void LEDpulse(){
  if(pulseSpeed == 2)
    skip = false;
  if(skip == true){
    skip = false;
    return;
  }
  else{
    skip = true;
    if(increase == true){
      if(n < brightness*255/2)
        n++;
      else
        increase = false;
    }
    if(increase == false){
      if(n > 0)
        n--;
      else
        increase = true;
    }
    uint32_t red = colorVal >> 16;
    uint32_t green = colorVal & 65280; //000000001111111100000000
    green = green >> 8;
    uint32_t blue = colorVal & 255; //000000000000000011111111
    uint32_t pulseColor = ring.Color(n*red/255, n*green/255, n*blue/255);
    solid(pulseColor);
  } 
}

