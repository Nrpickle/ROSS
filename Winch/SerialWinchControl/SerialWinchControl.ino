#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>

Servo ESC;
Encoder winchEncoder(2,3);

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
  
  while(!Serial.available()); //Wait for serial input
  
  int parameters[8];
  int incomingByte = 0;
  
  while(Serial.available() < 8) {} //wait for buffer to fill then save values in parameter array
  for (int i = 0; i < 8; i++){
    incomingByte = Serial.read();
    parameters[i] = incomingByte;
    digitalWrite(13, HIGH);
  }
  delay(500);
  digitalWrite(13, LOW);
  
  int winchSpeed = parameters[1];
  //char testSend = (char) winchSpeed;
  //Serial.write(testSend);
  int depthByte1 = parameters[2];
  //Serial.write(depthByte1);
  int depthByte2 = parameters[3];
  //Serial.write(depthByte2);
  int duration = parameters[4];
  Serial.write(duration);
  int units = parameters[5];
  //Serial.write(units);
  int checksum = parameters[6];
  //Serial.write(checksum);

  /*
  if(checksum != ((((winchSpeed ^ depthByte1) ^ depthByte2) ^ duration) ^ units)){
    Serial.print("ERROR: Data is corrupted.");
    return;
  }
  else
    Serial.print("Data transmissions successful.");
  */  
  
  int depth = depthByte1 + depthByte2;
  //int depth = 500000; //Test depth
  depth = depth * 1000; //pings/meter ratio (current is place holder)
  int maxSpeed = winchSpeed*90/254;
  int maxWinchSpeedOut = 90 + maxSpeed;
  //int maxWinchSpeedOut = 180; //Test max speed
  int maxWinchSpeedIn = 90 - maxSpeed;
  //int maxWinchSpeedIn = 0; //test 
  
  if (units == 0){
    duration = duration * 1000;
  }
  else if (units == 255){
    duration = duration * 60000;
  }
  /*else{
    Serial.write("ERROR: Invalid duration units provided.");
    duration = 0;
    return;
  }*/

  while(winchEncoder.read() < (depth - 200000))
    ESC.write(maxWinchSpeedOut);
  while(winchEncoder.read() < (depth - 100000))
    ESC.write(maxWinchSpeedOut*.9); 
  while(winchEncoder.read() < (depth - 10000))
    ESC.write(maxWinchSpeedOut*.7);
  while(winchEncoder.read() < (depth))
    ESC.write(maxWinchSpeedOut*.6); 
  ESC.write(90);
  
  delay(duration);
  
  while(winchEncoder.read() > 200000)
    ESC.write(maxWinchSpeedIn);
  while(winchEncoder.read() > 100000)
    ESC.write(maxWinchSpeedIn + .2*maxSpeed);
  while(winchEncoder.read() > 10000)
    ESC.write(maxWinchSpeedIn + .5*maxSpeed);
  while(winchEncoder.read() > 0)
    ESC.write(maxWinchSpeedIn + .8*maxSpeed);
  ESC.write(90);
  winchEncoder.write(0);  
}
