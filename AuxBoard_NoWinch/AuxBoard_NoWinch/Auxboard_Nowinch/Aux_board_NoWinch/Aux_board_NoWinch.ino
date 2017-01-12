//Winch control version 2.0
//#define DEBUG //Uncoment to print debugging information via serial
//#define WINCH //Uncomment if deployment uses a winch
struct Winch_TYPE {
  uint8_t currentSpeed;
  uint8_t prevSpeed;
  uint8_t currentDir; // UP,DOWN,STOP defined in enum
  uint8_t prevDir;
  uint8_t prevDesiredDir;
  uint8_t prevDesiredSpeed;
} winch;

#include <Servo.h>
#include <Encoder.h>
#include <Timer.h>

Servo ESC; //Create ESC object
Encoder winchEncoder(3,2); //Create encoder object
Timer statusTimer; //Create a timer for sending status messages
  
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
#define MAX_FORWARD 1910 //Maximum, minimum, and neutral pulse widths in microseconds
#define NEUTRAL 1479
#define MAX_REVERSE 1048
#define REV(x) 3936*x //Converts revolutions into encoder pings

//Speed constants
#define SLOW_DIST 2 //Distance in revolutions from full upright to begin changing winch speed in
#define LIFT_SPEED 65 //Speed for lifting the A-frame when maintaining or returning after a profile
#define FAST_IN_SPEED 65 //Speed for returning fast AND maintaining
#define SLOW_IN_SPEED 55 //Speed for returning slow AND maintaining

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

const double RAMP_TIME = 500; //Time it takes to change speed in milliseconds
const float pi = 3.14159;
uint64_t t0 = 0; //Beginning time for speed change
int16_t speedDifference = 0; //Difference between desired and current speed
int parameters[7];

int firstbyte; //First byte to check consistency with RPI
int secondbyte = 0x00; //Second byte to check consistency with RPI
int incomingByte = 0;

int state = CHECK_BUFFER;
int header;
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
bool stopReturnFast = false;
bool winchUseAttempted = false;
bool go = false;


void setup() {
  // put your setup code here, to run once:
  Serial1.begin(57600);
  #ifdef DEBUG
    Serial.begin(9600);
  #endif
  winch.currentSpeed = 100; //Initialize all struct values to stationary
  winch.prevSpeed = 100;
  winch.currentDir = STOP;
  winch.prevDir = STOP;
  winch.prevDesiredDir = STOP;
  winch.prevDesiredSpeed = 0;
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
  
  statusTimer.every(1000, sendStatus); //Send a status message every second
  
  ESC.attach(9, MAX_REVERSE, MAX_FORWARD); //Connect ESC with maximum and minimum puse width values
  ESC.writeMicroseconds(NEUTRAL); //Start the winch in neutral
  delay(5000); //Allow ESC to receive neutral signal for proper amount of time
}

void loop() {
  statusTimer.update();
  
  switch(state){
    
    case CHECK_BUFFER:
      if(buffSize == 7) //Update parameters if the serial buffer is full (new packet fully received)
        updateParameters();
      state = CONTROL_WINCH;
     break;
     
     case CONTROL_WINCH:
       if(header == 0xCC){ //STOP:Halt
         changeSpeed(0, STOP);
         halt = true;
         depthReached = true;
       }
         
       if(header == 0xAA){//STOP:Return at full speed
         changeSpeed(0, STOP);
         depthReached = true;
         if(!digitalRead(up) == false) //Return the winch to its upright position and maintian
           changeSpeed(FAST_IN_SPEED, UP);
         if(!digitalRead(up) == true){
           changeSpeed(0, STOP);
           winchEncoder.write(0);
         }
       }
       
       if(header == 0xBB){//STOP:Return slower
         changeSpeed(0, STOP);
         depthReached = true;
         if(!digitalRead(up) == false) //Return the winch to its upright position and maintian
           changeSpeed(SLOW_IN_SPEED, UP);
         if(!digitalRead(up) == true){
           changeSpeed(0, STOP);
           winchEncoder.write(0);
         }
       }
       
       if(header == 0xEE)//Stop the motor
         remoteStop(); 
       if(header == 0xDD)//Start the motor
         remoteStart();
       if(header == 255)//Take a profile - normal operation
         takeProfile();
         
       if(returned == true)
         state = MAINTAIN;
       else
       state = CHECK_BUFFER;
     break;
     
   case MAINTAIN:
     if(!digitalRead(up) == false)
       changeSpeed(LIFT_SPEED, UP);
     else{
       changeSpeed(0, STOP);
       winchEncoder.write(0);
     }
     state = CHECK_BUFFER;
   break;
   }
}

void changeSpeed(uint8_t newSpeed, uint8_t newDir){

  //If we want to go UP
  if(newDir == UP){
    newSpeed = 100 - newSpeed;
  }
  //If we want to go down
  else if(newDir == DOWN){
    newSpeed = 100 + newSpeed;
  }
  //Else we want to STOP
  else{
    newSpeed = 100;
  }
  #ifdef DEBUG
    Serial1.print("[Desired: ");
    Serial1.print(newSpeed);
    Serial1.print(" Current: ");
    Serial1.print(winch.currentSpeed);
    Serial1.println("]");
  #endif
  

  //Check if no change is needed (if we are going the desired speed and direction)
  if(newDir == winch.currentDir && newSpeed == winch.currentSpeed){  //If the command is to continue moving the same speed and direction...
    winch.prevSpeed = winch.currentSpeed;
    winch.prevDesiredSpeed = newSpeed; //Next time we write a new speed we know it will be at t0, the beginning of a speed change
    winch.prevDesiredDir = newDir;
    #ifdef DEBUG
      Serial1.println("[REACHED END CASE]");
    #endif
    return;   //...return without altering speed
  }


  //This will catch if we have gotten to this spot while the previous call to the function was "no change requested"
  if(winch.prevDesiredSpeed != newSpeed && winch.prevDesiredDir != newDir){
    //If that's the case, then we want to initialize the acceleration
    t0 = millis();
    speedDifference = newSpeed - winch.prevSpeed;
    //Avoid errors comparing different data types. If the difference is a negative value, make it slightly more negative. Same for a positive speed difference. 
    if(speedDifference < 0)
      speedDifference -= 1;
    else if (speedDifference > 0)
      speedDifference += 1;
  }
   
  uint64_t deltaT = millis() - t0;
  winch.currentSpeed = (double)winch.prevSpeed + (double)speedDifference*.5*(1-cos((pi*(double)deltaT)/RAMP_TIME)); //Accelerate sinusoidally
  constrain(winch.currentSpeed, 0, 200); //Do not write above or below the maximum pulse widths
  uint16_t speedToWrite = map(winch.currentSpeed, 0, 200, MAX_REVERSE, MAX_FORWARD); //Convert from sinusoid magnitude to pulse width
  ESC.writeMicroseconds(speedToWrite); //Write the scaled value
  #ifdef DEBUG
    Serial1.println(speedToWrite);
  #endif 
  if(winch.currentSpeed == newSpeed)
    winch.prevSpeed = newSpeed; //Set the starting point for the next speed change
    
  if(winch.currentSpeed > 100)
    winch.currentDir = DOWN;
  else if(winch.currentSpeed < 100)
    winch.currentDir = UP;
  else if(winch.currentSpeed == 100)
    winch.currentDir = STOP;
    
  winch.prevDesiredSpeed = newSpeed; //Next time we write a new speed we know it will be at t0, the beginning of a speed change
  winch.prevDesiredDir = newDir;
}

void serialEvent1()
{
  if(Serial1.available()){
   
     if (go){ //check to see if the go statement is true (Data is synced)
       parameters[buffSize] = Serial1.read();
       buffSize++;
     }
     else{  //If the go statement isn't true, wait until the data is synced and then make go statement true
  
          firstbyte = Serial1.read(); //Read the first byte coming from the RPI
      
          if((firstbyte == 255) && (secondbyte == 255)){
            go = true;
          }
          else{
            secondbyte = firstbyte; //store value to check again
            go = false;
          }
     }
  }
}

void updateParameters(){
  header = parameters[0];
  speedOut = parameters[1]; //Save array contents to corresponding variables
  speedIn = parameters[2];
  upperByte = parameters[3];
  lowerByte = parameters[4];
  checksum = parameters[5];
  Serial.println(header);
  Serial.println(speedOut);
  Serial.println(speedIn);
  Serial.println(upperByte);
  Serial.println(lowerByte);
  Serial.println(checksum);
  buffSize = 0; //Reset buffer size and control variables
  //kickstart = true; //reset kickstart for another profile
  //go = false; //reset the go condition to check data again for consistency 
  secondbyte = 0x00; // reset second byte to zero
  //warningCounter = 0;
  depthReached = false;
  halt = false;
  if(checksum == (((speedOut ^ speedIn) ^ upperByte) ^ lowerByte)){
    upperByte = upperByte << 8;
    depth = upperByte + lowerByte;
    depth = REV(depth); //pings/revolution
    speedOut = speedOut*100/254; //Scale from 0-254 to 0 - 100
    speedIn  = speedIn*100/254; //Scale from 0-254 to 0 - 100
    dataCorrupted = false;
  }
  else{
    depth = 0;
    speedOut = 0;
    speedIn = 0;
    dataCorrupted = true;
  } 
}

void sendStatus(){
  #ifdef WINCH
  Serial1.print("STATUS ");
  if(dataCorrupted == false){
    if(returned == true){
      Serial1.print("1  "); //Ready
    }
    else{
      Serial1.print("0  "); //Busy
    }
  }
  else{
    Serial1.print("3  "); //Data corrupted
    dataCorrupted = false;
  }
  Serial1.print("Dir ");
  if(winch.currentDir == UP)
    Serial1.print("up  ");
  else if(winch.currentDir == DOWN)
    Serial1.print("down  ");
  else if(winch.currentDir == STOP)
    Serial1.print("stationary  ");
  
  Serial1.print("Rev ");
  long long pingsFromSurface = winchEncoder.read();
  pingsFromSurface = pingsFromSurface/3936;
  long revsFromSurface = (long) pingsFromSurface;
  Serial1.println(revsFromSurface);
  #endif
  
  #ifndef WINCH
  if(winchUseAttempted == true){
    Serial1.println("Error: Winch not enabled.");
    winchUseAttempted = false;
  }
  #endif
}

inline void remoteStart(){
  if (motorRunning == false){ //Prevent remote start from executing if motor already running
    digitalWrite(remoteStopPin, LOW);
    digitalWrite(remoteStartPin, HIGH);
    digitalWrite(remoteStartLED, HIGH);
    delay(startTime);
    digitalWrite(remoteStartPin, LOW);
    digitalWrite(remoteStartLED, LOW);
    motorRunning = true;
  }
}

inline void remoteStop(){
  if(motorRunning == true){
    digitalWrite(remoteStopPin, HIGH);
    digitalWrite(remoteStopLED, HIGH);
    motorRunning = false;
  }
}

void takeProfile(){
   if(depthReached == false){
     //Can't use switches with current version of Aux Board
    if(!digitalRead(down) == false){//Slowly let A-frame down from upright position
      changeSpeed(30, DOWN);
      returned = false;
    }
    else if((winchEncoder.read() < depth)){
      changeSpeed(speedOut, DOWN);
      returned = false;
    }
    else if(winchEncoder.read()>= depth){
      changeSpeed(0, STOP);
      depthReached = true;
      returned = false;
    }
  }
  else if(depthReached == true && halt == false){
    if(!digitalRead(up) == true){ //Stop when A-frame is in full upright position
      changeSpeed(0, STOP);
      winchEncoder.write(0); //Account for line stretching - reset after each cast
      returned = true;
    }
    else if(winchEncoder.read() > 20000){ //change back to else if
      changeSpeed(speedIn, UP);
      returned = false;
    }
    else if(winchEncoder.read() > 0){
      changeSpeed(LIFT_SPEED, UP);
      returned = false;
    }
    else if(!digitalRead(down) == false && !digitalRead(up) == false){ //Slow down when A-fram lifts up
      changeSpeed(LIFT_SPEED, UP);
      returned = false;
    }
    else if(winchEncoder.read() <= (-500*3936)){ //Winch will stop if line snaps and can't engage sensors
      changeSpeed(0, STOP);
      returned = true;
    }  
  }
}


