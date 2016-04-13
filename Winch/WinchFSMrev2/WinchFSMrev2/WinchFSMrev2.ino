#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>

//Define states
#define checkBuffer 0
#define controlWinch 1
#define sendStatus 2

//Define remote start/stop pins
#define remoteStart 5
#define remoteStop 6
#define remoteStartLED 7
#define remoteStopLED 8
#define startTime 750 //How long to hold remote start HIGH

Servo ESC; //Create ESC object
Encoder winchEncoder(2,3); //Create encoder object

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
int speedOut; //
int speedIn; //
int depth;
bool motorRunning = false;
bool depthReached = false;

void setup() {
  // put your setup code here, to run once:
  delay(5000); //Increase for final revision
  Serial.begin(9600);
  ESC.attach(9);
  ESC.write(180);
  delay(4000);
  ESC.write(0);
  delay(4000);
  ESC.write(90);
  delay(6000);
  
  pinMode(13, OUTPUT);
  pinMode(remoteStart, OUTPUT);
  pinMode(remoteStop, OUTPUT);
  pinMode(remoteStartLED, OUTPUT);
  pinMode(remoteStopLED, OUTPUT);
  
  digitalWrite(remoteStart, LOW);
  digitalWrite(remoteStop, HIGH);
  digitalWrite(remoteStartLED, LOW);
  digitalWrite(remoteStopLED, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
  switch(state){
    case checkBuffer:
      if(buffSize == 7){
        header = parameters[0];
        speedOut = parameters[1]; //Save array contents to corisponding variables
        speedIn = parameters[2];
        upperByte = parameters[3];
        lowerByte = parameters[4];
        checksum = parameters[5];
        buffSize = 0;
      
        upperByte = upperByte << 8;
        depth = upperByte + lowerByte;
        depth = depth * 1000; //pings/meter ratio (current is place holder)
        
        speedOut = 90 + (speedOut*90/254);
        speedIn = 90 - (speedIn*90/254);
        digitalWrite(13,HIGH);
      }
      state = controlWinch;
     break;
      
    case controlWinch:
      if(header == 255){
        if(depthReached == false){
          if(winchEncoder.read()<depth)
            ESC.write(speedOut);
          else if(winchEncoder.read()>= depth){
            ESC.write(90);
            depthReached = true;
          }
        }
        else if(depthReached == true){
          if(winchEncoder.read()>0)
            ESC.write(speedIn);
          else if(winchEncoder.read()<=0){
            ESC.write(90);
            depthReached = false;
          }  
        }
      }
    
    
    
  }
}


void serialEvent(){
  parameters[buffSize] = Serial.read();
  buffSize++;
}
