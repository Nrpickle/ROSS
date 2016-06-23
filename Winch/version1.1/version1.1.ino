//Version 1.1
#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>
#include <Timer.h>

//Define states
#define checkBuffer 0
#define controlWinch 1

//Define remote start/stop pins
#define remoteStartPin 5
#define remoteStopPin 6
#define remoteStartLED 7
#define remoteStopLED 8
#define down 11 //CHANGE TO REFLECT NEW AUX BOARD WIRING
#define downLED 13
#define up 14 //CHANGE TO REFLECT NEW AUX BOARD WIRING
#define upLED 15
#define startTime 750 //How long remote start is held HIGH in milliseconds

Servo ESC; //Create ESC object
Encoder winchEncoder(3,2); //Create encoder object
Timer statusTimer; //Create timer object

int parameters[7];
int incomingByte = 0;

int state = checkBuffer;
int header;
int winchSpeedOut;
int winchSpeedIn;
int upperByte;
int lowerByte;
int checksum;
int buffSize = 0;
int speedOut;
int speedIn;
int currentSpeed;
long long depth;
bool motorRunning = false;
bool depthReached = false;
bool softStopped = false;
bool halt = false;
bool returned = true;
bool dataCorrupted = false;

void setup() {
  // put your setup code here, to run once:
  delay(20000); //Make sure winch has time to power on before calibration
  Serial1.begin(57600);
  ESC.attach(9); //Connect ESC
  calibrateESC();
  ESC.write(90);
  pinMode(remoteStartPin, OUTPUT);
  pinMode(remoteStopPin, OUTPUT);
  pinMode(remoteStartLED, OUTPUT);
  pinMode(remoteStopLED, OUTPUT);
  pinMode(upLED, OUTPUT);
  pinMode(downLED, OUTPUT);
  pinMode(down, INPUT);
  pinMode(up, INPUT);
  
  digitalWrite(remoteStartPin, LOW);
  digitalWrite(remoteStopPin, HIGH);
  digitalWrite(remoteStartLED, LOW);
  digitalWrite(remoteStopLED, LOW);
  statusTimer.every(1000, sendStatus);
  while(digitalRead(up) == true) //Make sure the winch starts in the upright position
    ESC.write(40);
  ESC.write(90);
  winchEncoder.write(0);
}

void loop() {
  // put your main code here, to run repeatedly:
  statusTimer.update();
  if(digitalRead(up) != true) //Use LEDs for calibration
    digitalWrite(upLED, HIGH);
  else
    digitalWrite(upLED, LOW);
  if(digitalRead(down) != true)
    digitalWrite(downLED, HIGH);
  else
    digitalWrite(downLED, LOW);
  switch(state){
    case checkBuffer:
      if(buffSize == 7)
        updateParameters();
      state = controlWinch;
     break;
      
    case controlWinch:
      if(header == 255)//Take a profile - normal operation
        takeProfile();
      else if(header == 0xAA){//STOP:Return at full speed
        if(softStopped == false)
          softStopped = softStop();
        depthReached = true;
        if(digitalRead(up) == true) //Return the winch to its upright position and maintian
          ESC.write(40);
        else{
          ESC.write(90);
          winchEncoder.write(0);
        }
      }
      else if(header == 0xBB){//STOP:Return slower
        if(softStopped == false)
          softStopped = softStop();
        depthReached = true;
        if(digitalRead(up) == true) //Return the winch to its upright position and maintian
          ESC.write(50);
        else{
          ESC.write(90);
          winchEncoder.write(0);
        }
      }
      else if(header == 0xCC){ //STOP:Halt
        if(softStopped == false)
          softStopped = softStop();
        halt = true;
        depthReached = true;
      }
      else if(header == 0xDD)
        remoteStart();
      else if(header == 0xEE)
        remoteStop();
      else if(header == 0xAB)
        calibrateESC();
      state = checkBuffer;
    break; 
  }
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
  softStopped = false;
  halt = false;
  if(checksum == (((speedOut ^ speedIn) ^ upperByte) ^ lowerByte)){
    upperByte = upperByte << 8;
    depth = upperByte + lowerByte;
    depth = depth * 3936; //pings/revolution
    speedOut = 90 + (speedOut*90/254);
    speedIn = 90 - (speedIn*90/254);
    dataCorrupted = false;
  }
  else{
    depth = 0;
    speedOut = 90;
    speedIn = 90;
    dataCorrupted = true;
  }
  
}

void takeProfile(){
   if(depthReached == false){
     //Can't use switches with current version of Aux Board
    if(!digitalRead(down) == false){//Slowly let A-frame down from upright position
      ESC.write(110);
      returned = false;
    }
    else if(winchEncoder.read() < (depth - 40000)){ //Increase to full speed once A-fram is down
      ESC.write(speedOut);
      returned = false;
    }
    else if(winchEncoder.read() < (depth - 30000)){ //Reduce speed as sensor nears destination
      ESC.write(min(160, speedOut));
      returned = false;
    }
    else if(winchEncoder.read() < (depth - 20000)){
      ESC.write(min(145, speedOut));
      returned = false;
    }
    else if(winchEncoder.read() < (depth - 10000)){
      ESC.write(min(130, speedOut));
      returned = false;
    }
    else if((winchEncoder.read() < depth)){
      ESC.write(min(115, speedOut));
      returned = false;
    }
    else if(winchEncoder.read()>= depth){
      ESC.write(90);
      depthReached = true;
      returned = false;
      delay(500);//Delay half a second to reduce mechanical stress
    }
  }
  else if(depthReached == true && halt == false){
    if(!digitalRead(up) == true){ //Stop when A-frame is in full upright position
      ESC.write(90);
      winchEncoder.write(0); //Account for line stretching - reset after each cast
      returned = true;
    }
    else if(winchEncoder.read() > 20000){ //change back to else if
      ESC.write(speedIn);
      returned = false;
    }
//    else if(winchEncoder.read() > 30000){
//      ESC.write(max(20, speedIn));
//      returned = false;
//    }
//    else if(winchEncoder.read() > 20000){
//      ESC.write(max(35, speedIn));
//      returned = false;
//    }
//    else if(winchEncoder.read() > 10000){
//      ESC.write(max(40, speedIn));
//      returned = false;
//    }
    else if(winchEncoder.read() > 0){
      ESC.write(max(40, speedIn));
      returned = false;
    }
    else if(!digitalRead(down) == false && digitalRead(up) == false){ //Slow down when A-fram lifts up
      ESC.write(40);
      returned = false;
    }
    else if(winchEncoder.read() <= (-20*3936)){ //Winch will stop if line snaps and can't engage sensors
      ESC.write(90);
      returned = true;
  }  
  }
}

bool softStop(){
  currentSpeed = ESC.read();
  if(currentSpeed > 90){ //If letting line out
    for(int x = currentSpeed; x > 90; x = x - 10){
      ESC.write(x);
      delay(100); //Reduce mechanical wear
    }
    ESC.write(90); //Bring to a stop
    delay(500);
    return true;
  }
  else if(currentSpeed < 90){ //If taking line in
    for(int x = currentSpeed; x < 90; x = x + 10){
      ESC.write(x);
      delay(100);
    }
    ESC.write(90);
    delay(500);
    return true;
  }
  else{
    ESC.write(90);
    return true;
  }
}
  
void remoteStart(){
  if (motorRunning == false){ //Prevent remote start from executing if motor already running
    digitalWrite(remoteStopPin, LOW);
    digitalWrite(remoteStartPin, HIGH);
    digitalWrite(remoteStartLED, HIGH);
    delay(startTime);
    digitalWrite(remoteStartPin, LOW);
    delay(2000-startTime);
    digitalWrite(remoteStartLED, LOW);
    motorRunning = true;
  }
}

void remoteStop(){
  if(motorRunning == true){
    digitalWrite(remoteStopPin, HIGH);
    digitalWrite(remoteStopLED, HIGH);
    delay(2000);
    digitalWrite(remoteStopLED, LOW);
    motorRunning = false;
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
  if(depthReached == true && returned == false)
    Serial1.print("up  ");
  else if(depthReached == false && returned == false)
    Serial1.print("down  ");
  else if(returned == true)
    Serial1.print("stationary  ");
  
  Serial1.print("Rev ");
  long long pingsFromSurface = winchEncoder.read();
  pingsFromSurface = pingsFromSurface/3936;
  long revsFromSurface = (long) pingsFromSurface;
  Serial1.println(revsFromSurface);
}

void calibrateESC(){
  ESC.write(180);
  delay(4000);
  ESC.write(0);
  delay(4000);
  ESC.write(90);
  delay(6000);
}
