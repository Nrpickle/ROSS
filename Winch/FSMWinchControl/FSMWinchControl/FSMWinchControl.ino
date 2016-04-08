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

void setup() {
  // put your setup code here, to run once:
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
      //state = takeProfile; //This is for trouble shooting and should be removed. It negates the entire if statement.
      break;
    
    case takeProfile:   
      upperByte = upperByte << 8;
      depth = upperByte + lowerByte;
      depth = depth * 1000; //pings/meter ratio (current is place holder)
      
      maxSpeedOut = winchSpeedOut*90/254;
      maxSpeedIn = winchSpeedIn*90/254;
      maxWinchSpeedOut = 90 + maxSpeedOut;
      maxWinchSpeedIn = 90 - maxSpeedIn;
  
      while(winchEncoder.read() < (depth - 200000)){ //send winch out
        ESC.write(maxWinchSpeedOut);
        if(Serial.available()){
          ESC.write(maxWinchSpeedOut*.9);
          delay(100);
          ESC.write(maxWinchSpeedOut*.8);
          delay(100);
          ESC.write(maxWinchSpeedOut*.7);
          delay(100);
          ESC.write(maxWinchSpeedOut*.6);
          delay(100);
          ESC.write(90);
          state = receiveData;
          return;
        }
      }
      while(winchEncoder.read() < (depth - 100000)){ 
        ESC.write(maxWinchSpeedOut*.9); 
        if(Serial.available()){ //Stop if a new serial data is sent
          ESC.write(maxWinchSpeedOut*.8);
          delay(100);
          ESC.write(maxWinchSpeedOut*.7);
          delay(100);
          ESC.write(maxWinchSpeedOut*.6);
          delay(100);
          ESC.write(90);
          state = receiveData;
          return;
        }
      }

      while(winchEncoder.read() < (depth - 10000)){ 
        ESC.write(maxWinchSpeedOut*.7);
        if(Serial.available()){
          ESC.write(maxWinchSpeedOut*.6);
          delay(100);
          ESC.write(90);
          state = receiveData;
          return;;
        }
      }

      while(winchEncoder.read() < (depth)){ 
        ESC.write(maxWinchSpeedOut*.6); 
        if(Serial.available()){
          ESC.write(90);
          state = receiveData;
          return;
        }
      }

      ESC.write(90);
      delay(1000);
      while(winchEncoder.read() > 200000) //bring back in
        ESC.write(maxWinchSpeedIn);
      while(winchEncoder.read() > 100000)
        ESC.write(maxWinchSpeedIn + .2*maxSpeedIn);
      while(winchEncoder.read() > 10000)
        ESC.write(maxWinchSpeedIn + .5*maxSpeedIn);
      while(winchEncoder.read() > 0)
        ESC.write(maxWinchSpeedIn + .8*maxSpeedIn);
      ESC.write(90);
      
      winchEncoder.write(0); //reset count to prevent error propagatoin
      state = receiveData;
      break;
      
    case fastIn:
      //fast in code
      break;
      
    case slowIn:
      //fast out code
      break;
      
    case halt:
      ESC.write(90);
      state = receiveData;
      break;
    
    case remoteStart:
      digitalWrite(remoteStop, LOW);
      digitalWrite(remoteStart, HIGH);
      digitalWrite(remoteStartLED, HIGH);
      delay(startTime);
      digitalWrite(remoteStart, LOW);
      delay(2000-startTime);
      digitalWrite(remoteStartLED, LOW);
      state = receiveData;
      break;
      
    case remoteStop:
      digitalWrite(remoteStop, HIGH);
      digitalWrite(remoteStopLED, HIGH);
      delay(2000);
      digitalWrite(remoteStopLED, LOW);
      state = receiveData;
      break;
      
    default:
      state = receiveData;
      break;
  } 
}


