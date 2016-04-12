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
    
    case receiveData:
      saveData();
      
      if(ESC.read() > 90)
        softStopOut(ESC.read());
      else if(ESC.read() < 90)
        softStopIn(ESC.read());
      
//      if(checksum != (((winchSpeedOut ^ winchSpeedIn) ^ upperByte) ^ lowerByte)){ //Check to make sure data was received without corruption
//        Serial.print("ERROR: Data is corrupted.");
//        state = receiveData; //Return to same state without powering winch
//      }
      
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
        //ESC.write(90);
        delay(1000);
        lineIn(maxWinchSpeedIn);
        //ESC.write(90);
        winchEncoder.write(0); //reset count to prevent error propagatoin
      }
      state = receiveData;
      break;
      
    case fastIn:
      digitalWrite(13,HIGH);
      lineIn(0);
      state = receiveData;
      break;
      
    case slowIn:
      lineIn(45);
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
} //END OF LOOP FUNCTION

//Let line out at maximum speed and slow as sensor reaches destination
bool lineOut(int maxSpeed, int dist){
  while(winchEncoder.read() < (dist - 40000)){ //Let line out
    ESC.write(maxSpeed);
    if(Serial.available()){
      return true;
    }
  }
  while(winchEncoder.read() < (dist - 30000)){ //Let line out
    ESC.write(valComp(160, maxSpeed, false));
    if(Serial.available()){
      softStopOut(valComp(160, maxSpeed, false));
      return true;
    }
  }    
  while(winchEncoder.read() < (dist - 20000)){ 
    ESC.write(valComp(145, maxSpeed, false)); 
    if(Serial.available()){ //Stop if new serial data is sent
      softStopOut(valComp(145, maxSpeed, false));
      return true;
    }
  }
  while(winchEncoder.read() < (dist - 10000)){ 
    ESC.write(valComp(130, maxSpeed, false));
    if(Serial.available()){
      softStopOut(valComp(130, maxSpeed, false));
      return true;
    }
  }
  while(winchEncoder.read() < (dist)){ 
    ESC.write(valComp(115, maxSpeed, false)); 
    if(Serial.available()){
      softStopOut(valComp(115, maxSpeed, false));
      return true;
    }
  }
  ESC.write(90);
  return false;
}

//Take line back in at maximum speed and slow as sensor nears boat
void lineIn(int maxSpeed){
  while(winchEncoder.read() > 40000){ //Bring line back in
      ESC.write(maxSpeed);
      if(Serial.available()){
        //saveData();
        softStopIn(maxSpeed);
        return;
      }
  }
  while(winchEncoder.read() > 30000){ //Bring line back in
      ESC.write(valComp(20, maxSpeed, true));
      if(Serial.available()){
        softStopIn(valComp(20, maxSpeed, true));
        return;
      }
  }
  while(winchEncoder.read() > 20000){
    ESC.write(valComp(35, maxSpeed, true));
    if(Serial.available()){
      softStopIn(valComp(35, maxSpeed, true));
      return;
    }
  }
  while(winchEncoder.read() > 10000){
    ESC.write(valComp(50, maxSpeed, true));
    if(Serial.available()){
      softStopIn(valComp(50, maxSpeed, true));
      return;
    }
  }
  while(winchEncoder.read() > 0){
    ESC.write(valComp(65, maxSpeed, true));
    if(Serial.available()){
      softStopIn(valComp(65, maxSpeed, true));
      return;
    }
  }
  ESC.write(90);
}

//Prevent damage to winch when stopping
void softStopOut(int currentSpeed){
  for(int x = currentSpeed; x > 90; x = x - 10){
    ESC.write(x);
    delay(100);
  }
  ESC.write(90);  
}

//Prevent damage to winch when stopping
void softStopIn(int currentSpeed){
  for(int x = currentSpeed; x < 90; x = x + 10){
    ESC.write(x);
    delay(100);
  }
  ESC.write(90);  
}

void saveData(){
  while(Serial.available() < 7) {} //Wait for buffer to fill then save values in parameter array
  for (int i = 0; i < 7; i++){
    incomingByte = Serial.read();
    parameters[i] = incomingByte; //Save serial data into array
  }
  header = parameters[0];
  winchSpeedOut = parameters[1]; //Save array contents to corisponding variables
  winchSpeedIn = parameters[2];
  upperByte = parameters[3];
  lowerByte = parameters[4];
  checksum = parameters[5];  
}

//Compares a value to a maximum and returnes either the value or the max depending on wether the value should be greater or less than the max
int valComp(int value, int maxValue, bool lineIn){
  if((value > maxValue && lineIn == true)||(value < maxValue && lineIn == false))
    return value;
  else if((value > maxValue && lineIn == false)||(value < maxValue && lineIn == true))
    return maxValue;
  else
    return value; //If the values are equal
}

void blinkLED(int numberOfBlinks){
  for(int x = 0; x < numberOfBlinks; x++){
    digitalWrite(13, HIGH);
    delay(1000);
    digitalWrite(13, LOW);
    delay(1000);
  } 
}


