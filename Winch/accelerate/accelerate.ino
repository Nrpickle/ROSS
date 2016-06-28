typedef struct {
  uint8_t currentSpeed;
  uint8_t prevSpeed;
  uint8_t currentDir; // up, down, stationary
  uint8_t prevDir;
  bool newChange;
  //uint8_t updated;
} Winch;

//#include <Stdint.h>
#include <Servo.h>
#include <Encoder.h>
#include <Timer.h>

Servo ESC; //Create ESC object
Encoder winchEncoder(3,2); //Create encoder object
  
enum{
  UP,
  DOWN,
  STOP
};
  
#define MAX_FORWARD 1910 //Maximum, minimum, and neutral pulse widths in microseconds
#define NEUTRAL 1479
#define MAX_REVERSE 1048
#define RAMP_TIME 500 //Time it takes to change speed in milliseconds
#define REV(x) 3936*x //Converts revolutions into encoder pings

const float pi = 3.14159;
uint64_t t0 = 0; //Beginning time for speed change
uint8_t speedDifference = 0; //Difference between desired and current speed
bool destReached = false;
Winch winch; //= {
//    .currentSpeed = 100
//    .prevSpeed = 100
//    .currentDir = STOP // up, down, stop
//    .prevDir = STOP
//    .newChange = true
//  };

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  winch.currentSpeed = 100;
  winch.prevSpeed = 100;
  winch.currentDir = STOP;
  winch.prevDir = STOP;
  winch.newChange = true;
  
  ESC.attach(9, MAX_REVERSE, MAX_FORWARD); //Connect ESC
  //calibrateESC();
  ESC.writeMicroseconds(NEUTRAL);
  delay(5000); //Allow ESC to receive neutral signalfor proper amount of time
}

void loop() {
  // put your main code here, to run repeatedly:
  //if(millis() < 15000)
  //if(winchEncoder.read() <= REV(15) //&& destReached == false){
    changeSpeed(50, DOWN);
//  if(winchEncoder.read() > REV(15)){
//    changeSpeed(100, UP);
//    destReached = true;
//  }
//  if(winchEncoder.read() <= REV(0)){
//    changeSpeed(0, STOP);
//    winchEncoder.write(0);
//    destReached = false;
//  }
//  else{
//    changeSpeed(0, STOP);
  
  //}
  
}

void changeSpeed(uint8_t newSpeed, uint8_t newDir){
  if(newDir == winch.currentDir && newSpeed == winch.currentSpeed){
    winch.prevSpeed = winch.currentSpeed;
    winch.newChange = true;
    return;
  }
  if(newDir == UP){
    newSpeed = 100 - newSpeed;
    winch.currentDir = UP;
  }
  else if(newDir == DOWN){
    newSpeed = 100 + newSpeed;
    winch.currentDir = DOWN;
  }
  else{
    newSpeed = 100;
    winch.currentDir = STOP;
  }
    
  if(winch.newChange == true){
    t0 = millis();
    speedDifference = newSpeed - winch.prevSpeed;
    winch.newChange = false;
  }
   
  uint64_t deltaT = millis() - t0;
  winch.currentSpeed = winch.prevSpeed + speedDifference*.5*(1-cos((pi*deltaT)/RAMP_TIME));
  
  uint16_t speedToWrite = map(winch.currentSpeed, 0, 200, MAX_FORWARD, MAX_REVERSE);
  ESC.writeMicroseconds(speedToWrite); 
  //Serial.println(speedToWrite);
  
  if(winch.currentSpeed == newSpeed)
    winch.prevSpeed = newSpeed;
}
