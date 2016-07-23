#include "FastLED.h"
#include "TinyGPS.h"
#include <LEDFader.h>
#include <SoftwareSerial.h>
#include <SD.h>


//LED Definitions
#define WS2182_DATA_PIN 6
#define NUM_LEDS 12
CRGB leds[NUM_LEDS];

////////// GPS AND LOGGING DEFINITIONS  //////////
//Software Serial Definitons for GPS
#define PMTK_SET_NMEA_OUTPUT_RMCGGA "$PMTK314,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*28"
#define PMTK_SET_NMEA_UPDATE_5HZ  "$PMTK220,200*2C"
#define PMTK_SET_NMEA_UPDATE_10HZ "$PMTK220,100*2F"
#define PGCMD_NOANTENNA "$PGCMD,33,0*6D"

TinyGPS gps;
SoftwareSerial gps_serial(8, 7);
char in_byte = 0;
String nmea_line;

//GPS  Variables
bool good_encode = false;
float flat, flon;
unsigned long fix_age, chars;
int year;
byte month, day, hour, minute, second, hundredths;
unsigned short sentences, failed;
bool should_encode = true;

//SD Card Logging variables
#define chip_select 10
File logfile;
String folder_name_path;
String full_logfile_path;

////////// INDICATION, ENGINE START, EBOX COMMUNICATION DEFINITIONS //////////
#define startPin 4
#define startLEDPin 5
#define stopPin 2
#define stopLEDPin 3

HSVHue temp_color;

LEDFader start_led = LEDFader(startLEDPin);
LEDFader stop_led = LEDFader(stopLEDPin);

uint16_t start_pulse_rate = 250;  //This is the time in ms for each up or down section of the fade. So one pulse is two times this
uint16_t stop_pulse_rate = 250;

uint8_t start_num_pulses = 3;
uint8_t stop_num_pulses = 3;

uint8_t start_pulse_count = 0;
uint8_t stop_pulse_count = 0;

bool start_pressed = false;
bool stop_pressed = false;

uint8_t stopped = 0;
uint8_t started = 1;
uint8_t started_or_stopped = stopped;

uint8_t ring_byte;

uint8_t engine_start_byte = 'S';
uint8_t engine_stop_byte = 'K';

////////// SYSTEM AND STATE DEFINITIONS HANDLING //////////
//Main loop state
uint8_t wait_for_fix = 0;
uint8_t setup_log_file = 1;
uint8_t log_gps_data = 2;
uint8_t current_system_state = wait_for_fix;

void setup() {
  Serial.begin(57600);

  FastLED.addLeds<NEOPIXEL, WS2182_DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(127);
  set_solid(HUE_PURPLE);

  gps_serial.begin(9600);
  delay(1000); //Per the documentation for setup time on the GPS once serial is enabled
  gps_serial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  gps_serial.println(PMTK_SET_NMEA_UPDATE_10HZ);
  gps_serial.println(PGCMD_NOANTENNA);

  if (!SD.begin(chip_select)) {
    //ERROR
  }

  start_led.fade(0, 500);
  stop_led.fade(255, 500);

  pinMode(startPin, INPUT);
  pinMode(stopPin, INPUT);
}

void loop() {
  ////////// GPS LOGGING //////////
  good_encode = false;
  if (gps_serial.available() > 0) {
    in_byte = gps_serial.read();
    if (should_encode) {
      if (gps.encode(in_byte)) {
        good_encode = true;
      } else {
        //ERROR
      }
    }
  }
  if (current_system_state == wait_for_fix) {

    if (good_encode) {
      gps.f_get_position(&flat, &flon, &fix_age);
      if (fix_age == TinyGPS::GPS_INVALID_AGE) {
        //ERRROR
      } else {
        //ERROR
        current_system_state = setup_log_file;
      }
    }
  } else if (current_system_state == setup_log_file) {
    //File naming is as follows
    //GPS_LOG--YEAR_MONTH_DAY--HOUR_MINUTE.txt
    if (good_encode) {
      gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &fix_age);

      folder_name_path = String(year) + "/" + String(month) + "/" + String(day);

      if (!SD.exists(folder_name_path)) {
        SD.mkdir(folder_name_path);
      }

      for (uint16_t i = 0 ; ; i++) {
        char buf[4];
        sprintf(buf, "%04u", i);
        full_logfile_path = folder_name_path + "/" + "LOG" + String(buf) + ".txt";
        if (!SD.exists(full_logfile_path)) {
          break;
        }
      }
      logfile = SD.open(full_logfile_path, FILE_WRITE);
      if (!logfile) {
        //Error
      } else {
        should_encode = false;
        current_system_state = log_gps_data;
      }
    }

  } else if (current_system_state == log_gps_data) {
    if (in_byte == '\n') {
      if (!logfile.println(nmea_line.c_str())) {
        //Error here
      }
      nmea_line = "";
      logfile.flush();
    } else {
      nmea_line += in_byte;
    }
  }

  ////////// INDICATION, ENGINE START, EBOX COMMUNICATION //////////
  stop_led.update();
  start_led.update();

  FastLED.show();

  if (digitalRead(startPin) && (started_or_stopped == stopped) && !stop_pressed) {
    started_or_stopped = started;
    start_pressed = true;
  }
  if (digitalRead(stopPin) && (started_or_stopped == started) && !start_pressed) {
    started_or_stopped = stopped;
    stop_pressed = true;
  }
  
  if (start_pressed) {
    if ((start_led.get_value() == 255) && (!start_led.is_fading())) {
      start_led.fade(0, start_pulse_rate);
    } else if ((start_led.get_value() == 0) && (!start_led.is_fading())) {
      start_led.fade(255, start_pulse_rate);
      start_pulse_count++;
    }

    if (start_pulse_count == start_num_pulses) {
      Serial.println(engine_start_byte);
      set_solid(HUE_GREEN);
      start_pulse_count = 0;
      stop_led.fade(0, stop_pulse_rate/2);
      start_pressed = false;
    }

  }
  
  if (stop_pressed) {
    if ((stop_led.get_value() == 255) && (!stop_led.is_fading())) {
      stop_led.fade(0, stop_pulse_rate);
    } else if ((stop_led.get_value() == 0) && (!stop_led.is_fading())) {
      stop_led.fade(255, stop_pulse_rate);
      stop_pulse_count++;
    }

    if (stop_pulse_count == stop_num_pulses) {
      Serial.println(engine_stop_byte);
      set_solid(HUE_RED);
      stop_pulse_count = 0;
      start_led.fade(0, start_pulse_rate/2);
      stop_pressed = false;
    }

  }

  ring_serial_handler();
  
}

void ring_serial_handler() {
  if(Serial.available() > 0){
    ring_byte = Serial.read();
  }
  //Serial.print("In Byte is: ");
  //Serial.println(ring_byte);
  switch(ring_byte){
    case 0:
      break;
    case 33:
      set_solid(HUE_RED);
      break;
    case 34:
      set_solid(HUE_GREEN);
      break;
    case 35:
      set_solid(HUE_BLUE);
      break;
    case 36:
      set_solid(HUE_YELLOW);
      break;
    case 41:
      break;
    case 42:
      break;
    case 43:
      break;
    case 51:
      FastLED.setBrightness(0);
      break;
    case 52:
    FastLED.setBrightness(127);
      break;
    case 53:
      FastLED.setBrightness(255);
      break;
    case 61:
      break;
    case 62:
      break;
    default:
      break;
  };
  ring_byte = 0;
//  if (val == 33)
//    color = 1; //red
//  else if (val == 34)
//    color = 2; //green
//  else if (val == 35)
//    color = 3; //blue
//  else if (val == 36)
//    color = 4; //yellow
//  else if (val == 41)
//    pattern = 1; //solid
//  else if (val == 42)
//    pattern = 2; //blink
//  else if (val == 43)
//    pattern = 3; //pulse
//  else if (val == 51)
//    brightness = 0;
//  else if (val == 52)
//    brightness = 1;
//  else if (val == 53)
//    brightness = 2;
//  else if (val == 61)
//    pulseSpeed = 1;
//  else if (val == 62)
//    pulseSpeed = 2;
//
//  //Multiply value by brightness/2 to set intensity to
//  //0 , 1/2 , or 1 [Brightness value can be 0, 1, or 2]
//  if (color == 1)
//    colorVal = ring.Color(brightness * 255 / 2, 0, 0);
//  else if (color == 2)
//    colorVal = ring.Color(0, brightness * 255 / 2, 0);
//  else if (color == 3)
//    colorVal = ring.Color(0, 0, brightness * 255 / 2);
//  else if (color == 4)
//    colorVal = ring.Color(brightness * 255 / 2, brightness * 255 / 2, 0);
//  else
//    colorVal = ring.Color(0, 0, 0);
//
//  if (pattern == 1) {
//    solid(colorVal);
//  }
}

void set_solid(HSVHue new_color) {
  for (uint8_t i = 0 ; i < NUM_LEDS ; i++) {
    leds[i] = CHSV(new_color, 255, 255);
  }
}

