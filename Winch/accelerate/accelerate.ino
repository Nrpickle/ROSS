//#define DEBUG //Uncoment to print debugging information via serial
struct Winch_TYPE {
  uint8_t currentSpeed;
  uint8_t prevSpeed;
  uint8_t currentDir; // UP,DOWN,STOP defined in enum
  uint8_t prevDir;
  bool newChange;
} winch;

#include <Servo.h>
#include <Encoder.h>
#include <Timer.h>

Servo ESC; //Create ESC object
Encoder winchEncoder(3,2); //Create encoder object
  
enum{ //Assign integer values to each direction
  UP,
  DOWN,
  STOP
};
  
#define MAX_FORWARD 1910 //Maximum, minimum, and neutral pulse widths in microseconds
#define NEUTRAL 1479
#define MAX_REVERSE 1048
#define REV(x) 3936*x //Converts revolutions into encoder pings
#define up 14 //Hall Effect sensor indicating upright position

const double RAMP_TIME = 500; //Time it takes to change speed in milliseconds
const float pi = 3.14159;
uint64_t t0 = 0; //Beginning time for speed change
int16_t speedDifference = 0; //Difference between desired and current speed
bool destReached = false;

void setup() {
  // put your setup code here, to run once:
  #ifdef DEBUG
    Serial.begin(9600);
  #endif
  winch.currentSpeed = 100; //Initialize all struct values to stationary
  winch.prevSpeed = 100;
  winch.currentDir = STOP;
  winch.prevDir = STOP;
  winch.newChange = true;
  pinMode(up, INPUT);
  ESC.attach(9, MAX_REVERSE, MAX_FORWARD); //Connect ESC with maximum and minimum puse width values
  ESC.writeMicroseconds(NEUTRAL); //Start the winch in neutral
  delay(5000); //Allow ESC to receive neutral signal for proper amount of time
}

void loop() {
 //This maintains the A-Frame's upright position indefinitely
  if(!digitalRead(up) == false)
    changeSpeed(65, UP);
  if(!digitalRead(up) == true)
    changeSpeed(0, STOP);
  delay(5);
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
  #ifdef DEBUG
    Serial.print("[Desired: ");
    Serial.print(newSpeed);
    Serial.print(" Current: ");
    Serial.print(winch.currentSpeed);
    Serial.println("]");
  #endif
  

  //Check if no change is needed (if we are going the desired speed and direction)
  if(newDir == winch.currentDir && newSpeed == winch.currentSpeed){  //If the command is to continue moving the same speed and direction...
    winch.prevSpeed = winch.currentSpeed;
    winch.newChange = true; //Next time we write a new speed we know it will be at t0, the beginning of a speed change
    #ifdef DEBUG
      Serial.println("[REACHED END CASE]");
    #endif
    return;   //...return without altering speed
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
    
    winch.newChange = false; //We are in the process of changing speed so the next time the function is called we know not to reset t0
  }
   
  uint64_t deltaT = millis() - t0;
  winch.currentSpeed = (double)winch.prevSpeed + (double)speedDifference*.5*(1-cos((pi*(double)deltaT)/RAMP_TIME)); //Accelerate in a sinusoidal intensity
  
  constrain(winch.currentSpeed, 0, 200); //Do not write above or below the maximum pulse widths
  uint16_t speedToWrite = map(winch.currentSpeed, 0, 200, MAX_REVERSE, MAX_FORWARD); //Convert from sinusoid magnitude to pulse width
  ESC.writeMicroseconds(speedToWrite); //Write the scaled value
  #ifdef DEBUG
    Serial.println(speedToWrite);
  #endif 
  if(winch.currentSpeed == newSpeed)
    winch.prevSpeed = newSpeed; //Set the starting point for the next speed change
}
