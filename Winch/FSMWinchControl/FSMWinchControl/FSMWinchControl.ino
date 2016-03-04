#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>

#define remStartPin 5
#define remStopPin 6

#define receiveData 0
#define takeProfile 1
#define fastIn 2
#define slowIn 3
#define halt 4
#define remoteStart 5
#define remoteStop 6

Servo ESC;
Encoder winchEncoder(2,3);

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
}

void loop() {
  // put your main code here, to run repeatedly:
  switch(state){ 
    
    case receiveData:
      while(Serial.available() < 7) {} //Wait for buffer to fill then save values in parameter array
      for (int i = 0; i < 7; i++){
        incomingByte = Serial.read();
        parameters[i] = incomingByte;
        digitalWrite(13, HIGH);
      }
      delay(500);
      digitalWrite(13, LOW); //Blink LED to indicate completion of data transmission
      
      header = parameters[1];
      winchSpeedOut = parameters[1]; //save array contents to corisponding variables
      winchSpeedIn = parameters[2];
      upperByte = parameters[3];
      lowerByte = parameters[4];
      checksum = parameters[5];
      
      if(checksum != (((winchSpeedOut ^ winchSpeedIn) ^ upperByte) ^ lowerByte)){
        Serial.print("ERROR: Data is corrupted.");
        state = receiveData; //return to same state without powering winch
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
  
      while(winchEncoder.read() < (depth - 200000)) //send winch out
        ESC.write(maxWinchSpeedOut);
      while(winchEncoder.read() < (depth - 100000))
        ESC.write(maxWinchSpeedOut*.9); 
      while(winchEncoder.read() < (depth - 10000))
        ESC.write(maxWinchSpeedOut*.7);
      while(winchEncoder.read() < (depth))
        ESC.write(maxWinchSpeedOut*.6); 
      ESC.write(90);
      
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
      digitalWrite(remStartPin, HIGH);
      delay(5000);
      digitalWrite(remStartPin, LOW);
      state = receiveData;
      break;
      
    case remoteStop:
      digitalWrite(remStopPin, HIGH);
      delay(5000);
      digitalWrite(remStopPin, LOW);
      state = receiveData;
      break;
      
    default:
      state = receiveData;
      break;
  } 
}
