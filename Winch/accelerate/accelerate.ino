struct Winch_TYPE {
  uint8_t currentSpeed;
  uint8_t prevSpeed;
  uint8_t currentDir; // up, down, stop
  uint8_t prevDir;
  bool newChange;
  //uint8_t updated;
} winch;

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
//#define RAMP_TIME 500 //Time it takes to change speed in milliseconds
#define REV(x) 3936*x //Converts revolutions into encoder pings

const double RAMP_TIME = 500;
const float pi = 3.14159;
uint64_t t0 = 0; //Beginning time for speed change
uint8_t speedDifference = 0; //Difference between desired and current speed
bool destReached = false;


//= {
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
  //Testing code
  
  if(millis()<15000)
    changeSpeed(50, DOWN);
  else if(millis()<20000)
    changeSpeed(0, STOP);
  else if(millis()<25000)    
    changeSpeed(50, UP);
  else
    changeSpeed(0, STOP);
  
  if(!winch.newChange){
    Serial.print("[main loop]");
    Serial.println(winch.currentSpeed);
  }
  
  delay(5);
  //Serial.println("hi!");
}

void changeSpeed(uint8_t newSpeed, uint8_t newDir){

  //If we want to go UP
  if(newDir == UP){
    newSpeed = 100 - newSpeed;
    winch.currentDir = UP;
  }
  //If we want to go down
  else if(newDir == DOWN){
    newSpeed = 100 + newSpeed;
    winch.currentDir = DOWN;
  }
  //Else we want to STOP
  else{
    newSpeed = 100;
    winch.currentDir = STOP;
  }
  
  Serial.print("[comp ");
  Serial.print(newSpeed);
  Serial.print(" | ");
  Serial.print(winch.currentSpeed);
  Serial.println("]");
  

  //Check if no change is needed (if we are going the speed we want)
  if(newDir == winch.currentDir && newSpeed == winch.currentSpeed){  //If the command is to continue moving the same speed...
    winch.prevSpeed = winch.currentSpeed;
    winch.newChange = true;
    Serial.println("[REACHED END CASE]");
    return;   //We want to return
  }


  //This will catch if we have gotten to this spot while the previous call to the function was "no change requested"
  if(winch.newChange == true){
    //If that's the case, then we want to initialize the acceleration
    t0 = millis();
    speedDifference = newSpeed - winch.prevSpeed;
    
    if(speedDifference < 0)
      speedDifference -= 1;
    else if (speedDifference > 0)
      speedDifference += 1;
    
    winch.newChange = false;
  }
   
  uint64_t deltaT = millis() - t0;
  winch.currentSpeed = (double)winch.prevSpeed + (double)speedDifference*.5*(1-cos((pi*(double)deltaT)/RAMP_TIME));
  
  /*
  if(!winch.newChange){
    Serial.print("[changeSpeed]");
    Serial.println(speedDifference);
  }
  */
  
  uint16_t speedToWrite = map(winch.currentSpeed, 0, 200, MAX_REVERSE, MAX_FORWARD);
  ESC.writeMicroseconds(speedToWrite); 
  
  Serial.println(speedToWrite);
  
  if(winch.currentSpeed == newSpeed)
    winch.prevSpeed = newSpeed;
}
