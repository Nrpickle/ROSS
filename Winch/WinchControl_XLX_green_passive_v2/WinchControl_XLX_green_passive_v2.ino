//Winch control version for use with the XLX green ESC
//Note: speeds and other variables need to be adjusted
//Note: code only checks for HES output when within slowing distance. May need to be changed.
#define DEBUG //Uncomment to print debugging information via serial
#define TEST //Uncomment to run test code
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
//Maximum, minimum, and neutral pulse widths in microseconds 
//ESC 1: 1910, 1479, 1048
//ESC 2: 1980, 1534, 1086
//ESC 3 (green XLX 12/1/2016): 2405, 1475, 545  
#define MAX_FORWARD 2405 //Maximum, minimum, and neutral pulse widths in microseconds
#define NEUTRAL 1475
#define MAX_REVERSE 545
#define REV(x) 3936*x //Converts revolutions into encoder pings

//Speed constants
#define SLOW_DIST 15 //Distance in revolutions from full upright to begin changing winch speed in
#define LIFT_SPEED 45//Speed for lifting the A-frame when finishing a profile
#define MAINTAIN_SPEED 40 //Speed for lifting the A-frame when maintaining
#define FAST_IN_SPEED 50 //Speed for returning fast AND maintaining
#define SLOW_IN_SPEED 30 //Speed for returning slow AND maintaining

//Define remote start/stop pins
#define remoteStartPin 5
#define remoteStopPin 6
#define remoteStartLED 7
#define remoteStopLED 8
#define startTime 750 //How long remote start is held HIGH in milliseconds
//Define sensor pins
#define downPin 10//Hall Effect sensor indicating lowered position
#define downLED 13
#define upPin 11 //Hall Effect sensor indicating upright position
#define upLED 15
//Define analog pins
#define resistancePin 0 //fishing rod strain gauge resistance (analog pin)

// resistance calibration settings for the fishing rod
double minResistance = 16924; //ohms - rod straight (no tension)
double stopResistance = 19181; //ohms - position of rod to send stop command (getting too slack)
double maxResistance = 28315; //ohms - rod bent (full tension)
double resistance = 28315;  //TEST

// frequency of status messages in ms
#ifdef DEBUG
    int statusFrequency = 100;
#else
    int statusFrequency = 1000;
#endif

const double RAMP_TIME = 500; //Time it takes to change speed in milliseconds
const double RAMP_TIME_DOWN = 3000; //Ramp time at beginning of profile to prevent tangling
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
  pinMode(downPin, INPUT);
  pinMode(upPin, INPUT);
  //Initialize pin states
  digitalWrite(remoteStartPin, LOW);
  digitalWrite(remoteStopPin, HIGH);
  digitalWrite(remoteStartLED, LOW);
  digitalWrite(remoteStopLED, LOW);
  
  
  statusTimer.every(statusFrequency, sendStatus); //Send a status message every X milliseconds
  
  ESC.attach(9, MAX_REVERSE, MAX_FORWARD); //Connect ESC with maximum and minimum puse width values
  ESC.writeMicroseconds(NEUTRAL); //Start the winch in neutral
  delay(5000); //Allow ESC to receive neutral signal for proper amount of time

  while(!digitalRead(upPin) != true)
    changeSpeed(LIFT_SPEED, UP);
    #ifdef DEBUG
      Serial1.println("Initial lift.");
    #endif

  changeSpeed(0, STOP);
  winchEncoder.write(0);
  returned = true;
}

void loop() {
  statusTimer.update();
  checkResistance();
  
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
         returned = true;
       }
         
       if(header == 0xAA){//STOP:Return at full speed
         //changeSpeed(0, STOP);
         depthReached = true;
         if(!digitalRead(upPin) == false) //Return the winch to its upright position and maintian
           changeSpeed(FAST_IN_SPEED, UP);
         if(!digitalRead(upPin) == true && winchEncoder.read() <= REV(SLOW_DIST)){
           changeSpeed(0, STOP);
           winchEncoder.write(0);
           returned = true;
           header = 0; //stop entering this if statement
         }
       }
       
       if(header == 0xBB){//STOP:Return slower
         //changeSpeed(0, STOP);
         depthReached = true;
         if(!digitalRead(upPin) == false) //Return the winch to its upright position and maintian
           changeSpeed(SLOW_IN_SPEED, UP);
         if(!digitalRead(upPin) == true && winchEncoder.read() <= REV(SLOW_DIST)){
           changeSpeed(0, STOP);
           winchEncoder.write(0);
           returned = true;
           header = 0; //stop entering this if statement
         }
       }
       
       if(header == 0xEE)//Stop the motor
         remoteStop(); 
       
       if(header == 0xDD)//Start the motor
         remoteStart();
       
       if(header == 255)//Take a profile - normal operation
         takeProfile();
         
       if(returned == true && halt == false)
         state = MAINTAIN;
       else
         state = CHECK_BUFFER;
       
       break;
     
    case MAINTAIN:
       //if(!digitalRead(upPin) == false || !digitalRead(downPin) == true)
       if(!digitalRead(upPin) == false)
         changeSpeed(MAINTAIN_SPEED, UP);
       else{
         changeSpeed(0, STOP);
         winchEncoder.write(0);
         returned = true;
       }
       state = CHECK_BUFFER;
       break;
   
  } //end switch
  delay(2);
} //end loop

void changeSpeed(uint8_t newSpeed, uint8_t newDir){
  double ramp = RAMP_TIME;
  if(newDir == DOWN)
    ramp = RAMP_TIME_DOWN;

  Serial.println(newSpeed);
  //If we want to go UP
  if(newDir == UP){
    newSpeed = 100 - newSpeed;
    //winch.currentDir = UP;
  }
  //If we want to go down
  else if(newDir == DOWN){
    newSpeed = 100 + newSpeed;
    //winch.currentDir = DOWN;
  }
  //Else we want to STOP
  else{
    newSpeed = 100;
    //winch.currentDir = STOP;
  }
  #ifdef DEBUG
    //Serial1.print("[Desired: ");
    //Serial1.print(newSpeed);
    //Serial1.print(" Current: ");
    //Serial1.print(winch.currentSpeed);
    //Serial1.println("]");
  #endif
  

  //Check if no change is needed (if we are going the desired speed and direction)
  if(newDir == winch.currentDir && newSpeed == winch.currentSpeed){  //If the command is to continue moving the same speed and direction...
    winch.prevSpeed = winch.currentSpeed;
    winch.prevDesiredSpeed = newSpeed; //Next time we write a new speed we know it will be at t0, the beginning of a speed change
    winch.prevDesiredDir = newDir;
    #ifdef DEBUG
      //Serial1.println("[REACHED END CASE]");
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
  winch.currentSpeed = (double)winch.prevSpeed + (double)speedDifference*.5*(1-cos((pi*(double)deltaT)/ramp)); //Accelerate sinusoidally
  constrain(winch.currentSpeed, 0, 200); //Do not write above or below the maximum pulse widths
  uint16_t speedToWrite = map(winch.currentSpeed, 0, 200, MAX_REVERSE, MAX_FORWARD); //Convert from sinusoid magnitude to pulse width
  ESC.writeMicroseconds(speedToWrite); //Write the scaled value
  #ifdef DEBUG
    //Serial1.println(speedToWrite);
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

void serialEvent1(){
  if(Serial1.available()){
    parameters[buffSize] = Serial1.read();
    buffSize++;
  }
}

void updateParameters(){
  header = parameters[0];
  speedOut = parameters[1]; //Save array contents to corisponding variables
  speedIn = parameters[2];
  upperByte = parameters[3];
  lowerByte = parameters[4];
  checksum = parameters[5];
  buffSize = 0; //Reset buffer size and control variables
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

  #ifdef DEBUG
    Serial1.print("[Resistance: ");
    Serial1.print(resistance);
    Serial1.println("]");
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
//    if(!digitalRead(downPin) == false){//Slowly let A-frame down from upright position
//      changeSpeed(30, DOWN);
//      returned = false;
//    }
     if((winchEncoder.read() < depth)){
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
    if(winchEncoder.read() > REV(SLOW_DIST)){
      changeSpeed(speedIn, UP);
      //Serial.println("Going speedIn in the UP direction"); //Debug speed in
      returned = false;
    }
    else{
      if(!digitalRead(upPin) == true){ //Stop when A-frame is in full upright position
        changeSpeed(0, STOP);
        winchEncoder.write(0); //Account for line stretching - reset after each cast
        returned = true;
        header = 0; //Will go straight into maintain mode rather than use take profile to maintain position
      }
      else{
        changeSpeed(LIFT_SPEED, UP); //This is the speed the winch will go when within five revolutions
      //Serial.println("Going Lift_Speed in the UP direction"); //debug speed in
      }
    }
  }
}

void checkResistance(){
  #ifdef TEST
    resistance = resistance - 1;  //TEST
  #endif
  if(resistance < stopResistance){
    changeSpeed(0, STOP);
    depthReached = true;  //we are acting like the cast reached the bottom of the profile
    halt = false; //we are acting like the cast reached the bottom of the profile
    returned = false;
    header = 255; //we need it to proceed as if it was at the bottom of a profile, even if it wasn't doing a profile
    resistance = maxResistance; //TEST
    #ifdef DEBUG
       Serial1.println("[RESISTANCE TOO LOW - STOPPED WINCH AND RETURNING TO SURFACE]");
    #endif
  }
}