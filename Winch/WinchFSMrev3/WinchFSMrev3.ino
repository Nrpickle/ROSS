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
#define down 10
#define up 11
#define startTime 750 //How long remote start is held HIGH

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

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(13, OUTPUT); //Remove for final revision
  digitalWrite(13, HIGH); //Remove after debugging
  delay(5000); //Ensure the winch is powered on before calibration sequence begins //Increase for final revision
  ESC.attach(9);//Begin calibration sequence
  ESC.write(180);
  delay(4000);
  ESC.write(0);
  delay(4000);
  ESC.write(90);
  delay(6000);
  
  pinMode(remoteStartPin, OUTPUT);
  pinMode(remoteStopPin, OUTPUT);
  pinMode(remoteStartLED, OUTPUT);
  pinMode(remoteStopLED, OUTPUT);
  pinMode(down, INPUT);
  pinMode(up, INPUT);
  
  digitalWrite(remoteStartPin, LOW);
  digitalWrite(remoteStopPin, HIGH);
  digitalWrite(remoteStartLED, LOW);
  digitalWrite(remoteStopLED, LOW);
  statusTimer.every(5000, sendStatus);
}

void loop() {
  // put your main code here, to run repeatedly:
  statusTimer.update();
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
        speedIn = 0;
        takeProfile();
      }
      else if(header == 0xBB){//STOP:Return at half speed
        if(softStopped == false)
          softStopped = softStop();
        depthReached = true;
        speedIn = 45;
        takeProfile();
      }
      else if(header == 0xCC){
        if(softStopped == false)
          softStopped = softStop();
        halt = true;
        depthReached = true;
      }
      else if(header == 0xDD)
        remoteStart();
      else if(header == 0xEE)
        remoteStop();
      state = checkBuffer;
    break; 
  }
}


void serialEvent(){
  if(Serial.available()){
    parameters[buffSize] = Serial.read();
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
  upperByte = upperByte << 8;
  depth = upperByte + lowerByte;
  depth = depth * 1000; //pings/meter ratio (current is place holder)
  speedOut = 90 + (speedOut*90/254);
  speedIn = 90 - (speedIn*90/254);
}

void takeProfile(){
   if(depthReached == false){
    if(digitalRead(down) == false)//Slowly let A-frame down from upright position
      ESC.write(110);
    else if(winchEncoder.read() < (depth - 40000)) //Increase to full speed once A-fram is down
      ESC.write(speedOut);
    else if(winchEncoder.read() < (depth - 30000)) //Reduce speed as sensor nears destination
      ESC.write(min(160, speedOut));
    else if(winchEncoder.read() < (depth - 20000))
      ESC.write(min(145, speedOut));
    else if(winchEncoder.read() < (depth - 10000))
      ESC.write(min(130, speedOut));
    else if((winchEncoder.read() < depth))
      ESC.write(min(115, speedOut));
    else if(winchEncoder.read()>= depth){
      ESC.write(90);
      depthReached = true;
      delay(500);//Delay half a second to reduce mechanical stress
    }
  }
  else if(depthReached == true && halt == false){
    if(digitalRead(up) == true) //Stop when A-frame is in full upright position
      ESC.write(90);
    else if(digitalRead(down) == false && digitalRead(up) == false) //Slow down when A-fram lifts up. If down switch fails, this statement will execute until A-frame is fully upright
      ESC.write(70);
    else if(winchEncoder.read() > 40000)
      ESC.write(speedIn);
    else if(winchEncoder.read() > 30000)
      ESC.write(max(20, speedIn));
    else if(winchEncoder.read() > 20000)
      ESC.write(max(35, speedIn));
    else if(winchEncoder.read() > 10000)
      ESC.write(max(50, speedIn));
    else if(winchEncoder.read() > 0)
      ESC.write(max(65, speedIn));
    else if(winchEncoder.read() <= 0) //Winch will still stop if switches fail
      ESC.write(90);  
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
//  currentSpeed = ESC.read();
//  
//  Serial.print("Speed: ");
//  
//  Serial.print("RPM");
//  Serial.println(); //New line

Serial.println("Test message");
  
  
  
}
