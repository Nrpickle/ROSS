#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>

//Define state machine states
#define receiveData 0
#define takeProfile 1
#define fastIn 2
#define slowIn 3
#define halt 4

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
int state = receiveData;
int header;
int winchSpeedOut;
int winchSpeedIn;
int upperByte;
int lowerByte;
int checksum;
int maxSpeedOut;
int maxSpeedIn;
int maxWinchSpeedOut;
int maxWinchSpeedIn;
int depth;
bool motorRunning = false;

void setup() {
  // put your setup code here, to run once:
  delay(10000);
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
    
    case receiveData:
      while(Serial.available() < 7) {} //Wait for buffer to fill then save values in parameter array
      for (int i = 0; i < 7; i++){
        incomingByte = Serial.read();
        parameters[i] = incomingByte; //Save serial data into array
        digitalWrite(13, HIGH);
      }
      delay(500);
      digitalWrite(13, LOW); //Blink LED to indicate completion of data transmission
      
      header = parameters[0];
      winchSpeedOut = parameters[1]; //Save array contents to corisponding variables
      winchSpeedIn = parameters[2];
      upperByte = parameters[3];
      lowerByte = parameters[4];
      checksum = parameters[5];
      
      if(checksum != (((winchSpeedOut ^ winchSpeedIn) ^ upperByte) ^ lowerByte)){ //Check to make sure data was received without corruption
        Serial.print("ERROR: Data is corrupted.");
        state = receiveData; //Return to same state without powering winch
      }
      else if(header == 255)
        state = takeProfile;
      else if(header == 0xAA)
        state = fastIn;
      else if(header == 0xBB)
        state = slowIn;
      else if(header == 0xCC)
        state = halt;
      else if(header == 0xDD)
        state = remoteStart;
      else if (header == 0xEE)
        state = remoteStop;
      break;
    
    case takeProfile:   
      upperByte = upperByte << 8;
      depth = upperByte + lowerByte;
      depth = depth * 1000; //pings/meter ratio (current is place holder)
      
      maxSpeedOut = winchSpeedOut*90/254;
      maxSpeedIn = winchSpeedIn*90/254;
      maxWinchSpeedOut = 90 + maxSpeedOut;
      maxWinchSpeedIn = 90 - maxSpeedIn;
      bool interrupted;
      
      interrupted = lineOut(maxWinchSpeedOut, depth);
      if(interrupted == false){
        ESC.write(90);
        delay(1000);
        lineIn(maxWinchSpeedIn, maxSpeedIn);
        ESC.write(90);
        winchEncoder.write(0); //reset count to prevent error propagatoin
      }
      state = receiveData;
      break;
      
    case fastIn:
      lineIn(0,90);
      state = receiveData;
      break;
      
    case slowIn:
      lineIn(45,45);
      state = receiveData;
      break;
      
    case halt:
      ESC.write(90);
      state = receiveData;
      break;
    
    case remoteStart:
      if (motorRunning == false){ //Prevent remote start from executing if motor already running
        digitalWrite(remoteStop, LOW);
        digitalWrite(remoteStart, HIGH);
        digitalWrite(remoteStartLED, HIGH);
        delay(startTime);
        digitalWrite(remoteStart, LOW);
        delay(2000-startTime);
        digitalWrite(remoteStartLED, LOW);
        motorRunning = true;
      }
      state = receiveData;
      break;
      
    case remoteStop:
      digitalWrite(remoteStop, HIGH);
      digitalWrite(remoteStopLED, HIGH);
      delay(2000);
      digitalWrite(remoteStopLED, LOW);
      motorRunning = false;
      state = receiveData;
      break;
      
    default:
      state = receiveData;
      break;
  } 
}

void softStopOut(int upperValue, int multiplyer){
  for(int x = multiplyer; x > .5; x = x - .1){
    int newVal = upperValue*x;
    if(newVal > 90)
      ESC.write(newVal);
    else
      ESC.write(90);
    delay(100);
  }
  ESC.write(90);
  state = receiveData;
}

void softStopIn(int maximum){
  for(int x = 0; x  < 90; x = x + 18){
    int newVal = maximum + x;
    if(newVal < 90)
      ESC.write(newVal);
     else
       ESC.write(90);
     delay(100);
  }
  ESC.write(90);
  state = receiveData;
}

bool lineOut(int maxSpeed, int dist){
  while(winchEncoder.read() < (dist - 200000)){ //Let line out
    ESC.write(maxSpeed);
    if(Serial.available()){
      softStopOut(maxSpeed, .9);
      return true;
    }
  }    
  while(winchEncoder.read() < (dist - 100000)){ 
    ESC.write(maxSpeed*.9); 
    if(Serial.available()){ //Stop if new serial data is sent
      softStopOut(maxSpeed, .8);
      return true;
    }
  }
  while(winchEncoder.read() < (dist - 10000)){ 
    ESC.write(maxWinchSpeedOut*.7);
    if(Serial.available()){
      softStopOut(maxSpeed, .6);
      return true;
    }
  }
  while(winchEncoder.read() < (dist)){ 
    ESC.write(maxSpeed*.6); 
    if(Serial.available()){
      softStopOut(maxSpeed, .5);
      return true;
    }
  }
  return false;
}

void lineIn(int fullSpeed, int maxSpeed){
  while(winchEncoder.read() > 200000){ //Bring line back in
      ESC.write(fullSpeed);
      if(Serial.available()){
        softStopIn(fullSpeed);
        return;
      }
  }
  while(winchEncoder.read() > 100000){
    ESC.write(fullSpeed + .2*maxSpeed);
    if(Serial.available()){
      softStopIn(fullSpeed + .2*maxSpeed);
      return;
    }
  }
  while(winchEncoder.read() > 10000){
    ESC.write(fullSpeed + .5*maxSpeed);
    if(Serial.available()){
      softStopIn(fullSpeed + .5*maxSpeed);
      return;
    }
  }
  while(winchEncoder.read() > 0){
    ESC.write(fullSpeed + .8*maxSpeed);
    if(Serial.available()){
      softStopIn(fullSpeed + .8*maxSpeed);
      return;
    }
  }
}

int outCheck(int numberCheck){
  if (numberCheck < 90)
    return 100;
  else
    return numberCheck;
}

int inCheck(int numberCheck){
  if(numberCheck > 90)
    return 80;
  else
    return numberCheck;
}



