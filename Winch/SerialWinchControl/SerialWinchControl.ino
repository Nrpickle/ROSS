#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>
#include <Bounce2.h>

Servo ESC;
Encoder winchEncoder(2,3);

#define reed1 14
#define reed2 15
#define debounceTime 10 //set minimum length of time a switch must be held (in ms)

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
  pinMode(reed1, INPUT);
  pinMode(reed2, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  
  while(!Serial.available()); //Wait for serial input
  
  int parameters[7];
  int incomingByte = 0;
  
  while(Serial.available() < 7) {} //Wait for buffer to fill then save values in parameter array
  for (int i = 0; i < 7; i++){
    incomingByte = Serial.read();
    parameters[i] = incomingByte;
    digitalWrite(13, HIGH);
  }
  delay(500);
  digitalWrite(13, LOW); //Blink LED to indicate completion of data transmission
  
  int winchSpeedOut = parameters[1]; //save array contents to corisponding variables
  int winchSpeedIn = parameters[2];
  int upperByte = parameters[3];
  int lowerByte = parameters[4];
  int checksum = parameters[5];
 
  /*
  if(checksum != (((winchSpeedOut ^ winchSpeedIn) ^ upperByte) ^ lowerByte)){
    Serial.print("ERROR: Data is corrupted.");
    return;
  }
  else
    Serial.print("Data transmissions successful.");
  */  
  upperByte = upperByte << 8;
  int depth = upperByte + lowerByte;
  
  depth = depth * 1000; //pings/meter ratio (current is place holder)
  
  int maxSpeedOut = winchSpeedOut*90/254;
  int maxSpeedIn = winchSpeedIn*90/254;
  int maxWinchSpeedOut = 90 + maxSpeedOut;
  int maxWinchSpeedIn = 90 - maxSpeedIn;
  
  bool reedSwitch1 = debounce(digitalRead(reed1), reed1);
  while(reedSwitch1){
    ESC.write(115);
    reedSwitch1 = debounce(digitalRead(reed1), reed1);
  }
  while(winchEncoder.read() < (depth - 100000))
    ESC.write(maxWinchSpeedOut);
  while(winchEncoder.read() < (depth - 50000))
    ESC.write(maxWinchSpeedOut*.9); 
  while(winchEncoder.read() < (depth - 10000))
    ESC.write(maxWinchSpeedOut*.8);
  while(winchEncoder.read() < (depth))
    ESC.write(maxWinchSpeedOut*.7); 
  ESC.write(90);
  
  delay(1000);
 
  while(winchEncoder.read() > 100000)
   ESC.write(maxWinchSpeedIn);
  while(winchEncoder.read() > 50000)
    ESC.write(maxWinchSpeedIn + .2*maxSpeedIn);
  while(winchEncoder.read() > 10000)
    ESC.write(maxWinchSpeedIn + .5*maxSpeedIn);
//  while (!debounce(digitalRead(reed1),reed1))
//    ESC.write(maxWinchSpeedIn + .5*maxSpeedIn);
  while(debounce(digitalRead(reed2),reed2))
    ESC.write(65);
  ESC.write(90);
  winchEncoder.write(0); //Reset encoder to reduce error propagation. 
}

//This debounce function will mitigate erronious switch activations
//bool debounce(bool readVal, bool pin){
//  bool currentVal = digitalRead(pin);
//  if(currentVal != readVal){
//    delay(debounceTime);
//    currentVal = digitalRead(pin);
//  }
//  return currentVal;
//}

bool debounce(bool readVal, bool pin){
  if(readVal != prevVal)
    lastDebounceTime = millis();
}
