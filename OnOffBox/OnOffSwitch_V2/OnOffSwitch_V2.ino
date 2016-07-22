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
char in_byte;
String nmea_line;

//GPS  Variables
bool good_encode = false;
float flat, flon;
unsigned long fix_age, chars;
int year;
byte month, day, hour, minute, second, hundredths;
unsigned short sentences, failed;

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

LEDFader start_led = LEDFader(startLEDPin);
LEDFader stop_led = LEDFader(stopLEDPin);

////////// SYSTEM AND STATE DEFINITIONS HANDLING //////////
//Main loop state
uint8_t wait_for_fix = 0;
uint8_t setup_log_file = 1;
uint8_t log_gps_data = 2;
uint8_t current_system_state = wait_for_fix;

void setup() {
  Serial.begin(9600);

  FastLED.addLeds<NEOPIXEL, WS2182_DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(32);
  show_green_solid();

  gps_serial.begin(9600);
  delay(1000); //Per the documentation for setup time on the GPS once serial is enabled
  gps_serial.println(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  gps_serial.println(PMTK_SET_NMEA_UPDATE_10HZ);
  gps_serial.println(PGCMD_NOANTENNA);

  if (!SD.begin(chip_select)) {
    //ERROR
  }
  start_led.fade(200, 8000);
  stop_led.fade(200, 8000);
}

void loop() {
  ////////// GPS LOGGING //////////
  good_encode = false;
  if (gps_serial.available() > 0) {
    in_byte = gps_serial.read();
    if (gps.encode(in_byte)) {
      good_encode = true;
    } else {
      //ERROR
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
  
}

void show_green_solid(void) {
  for (uint8_t i = 0 ; i < NUM_LEDS ; i++) {
    leds[i] = CHSV(HUE_GREEN, 255, 255);
  }
  FastLED.show();
}

