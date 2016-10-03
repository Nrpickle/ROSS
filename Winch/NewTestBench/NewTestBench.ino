//Winch control version 2.2
//#define DEBUG //Uncoment to print debugging information via serial


#include <Servo.h>
#include <Encoder.h>
#include <Timer.h>

Servo ESC; //Create ESC object
Encoder winchEncoder(3,2); //Create encoder object
 //Create a timer for sending status messages
  
enum{ //Assign integer values to each direction
  UP,
  DOWN,
  STOP
};

enum{  //Assign integer values to each state
  CHECK_BUFFER,
  CONTROL_WINCH,
  MAINTAIN
};

#define ENCODER_OPTIMIZE_INTERRUPTS  
#define MAX_FORWARD 1980 //Maximum, minimum, and neutral pulse widths in microseconds
#define NEUTRAL 1534
#define MAX_REVERSE 1086
#define REV(x) 3936*x //Converts revolutions into encoder pings

//Speed constants
#define SLOW_DIST 5 //Distance in revolutions from full upright to begin changing winch speed in
#define LIFT_SPEED 0 //Speed for lifting the A-frame when finishing a profile
#define MAINTAIN_SPEED 30 //Speed for lifting the A-frame when maintaining
#define FAST_IN_SPEED 0 //Speed for returning fast AND maintaining
#define SLOW_IN_SPEED 0 //Speed for returning slow AND maintaining

//Define remote start/stop pins
#define remoteStartPin 5
#define remoteStopPin 6
#define remoteStartLED 7
#define remoteStopLED 8
#define startTime 750 //How long remote start is held HIGH in milliseconds
//Define sensor pins
#define down 10//Hall Effect sensor indicating lowered position
#define downLED 13
#define up 11 //Hall Effect sensor indicating upright position
#define upLED 15

const double RAMP_TIME = 50; //Time it takes to change speed in milliseconds
const float pi = 3.14159;
uint64_t t0 = 0; //Beginning time for speed change
int16_t speedDifference = 0; //Difference between desired and current speed
int parameters[7];
int incomingByte = 0;

int state = CHECK_BUFFER;
int header = 0;
int upperByte;
int lowerByte;
int checksum;
int buffSize = 0;
int speedOut;
int speedIn;
long long depth;
bool motorRunning = false;
bool depthReached = false;
bool halt = false;
bool returned = true;
bool dataCorrupted = false;

void setup() {
  // put your setup code here, to run once:
  
  //Set pin modes
  pinMode(remoteStartPin, OUTPUT);
  pinMode(remoteStopPin, OUTPUT);
  pinMode(remoteStartLED, OUTPUT);
  pinMode(remoteStopLED, OUTPUT);
  pinMode(upLED, OUTPUT);
  pinMode(downLED, OUTPUT);
  pinMode(down, INPUT);
  pinMode(up, INPUT);
  //Initialize pin states
  digitalWrite(remoteStartPin, LOW);
  digitalWrite(remoteStopPin, HIGH);
  digitalWrite(remoteStartLED, LOW);
  digitalWrite(remoteStopLED, LOW);
  
  
 ESC.attach(9, MAX_REVERSE, MAX_FORWARD); //Connect ESC with maximum and minimum puse width values
//  ESC.writeMicroseconds(MAX_FORWARD);
//  delay(4000);
//  ESC.writeMicroseconds(MAX_REVERSE);
//  delay(4000);
//   ESC.writeMicroseconds(NEUTRAL);
//  delay(6000);
  ESC.writeMicroseconds(NEUTRAL); //Start the winch in neutral
  delay(5000); //Allow ESC to receive neutral signal for proper amount of time
}

void loop() {
  if(!digitalRead(up) == false)
    ESC.writeMicroseconds(1086);
  else
    ESC.writeMicroseconds(NEUTRAL);
  delay(10);
    
}
