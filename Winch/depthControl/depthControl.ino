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
}

void loop() {
  // put your main code here, to run repeatedly:
  
  Serial.print("Current depth is ");
  Serial.println(winchEncoder.read()); //multiply by conversion from encoder val to depth...
  delay(3000);
  Serial.print("Enter new sensor depth: ");
  while(!Serial.available());
  
  long newDepth = Serial.parseInt(); //multiply by conversion from encoder val to depth...
  Serial.println(newDepth);
  
  if(newDepth > winchEncoder.read()){ //let line out
    out(newDepth);
  }
  if(newDepth < winchEncoder.read()){ //bring line in
    in(newDepth);
  }
  Serial.println("Locatoin reached.");
}

void out(long newDepth){
  Serial.println("Letting line out.");
  while(winchEncoder.read() < (newDepth - 200000))
    ESC.write(180);
  while(winchEncoder.read() < (newDepth - 100000))
    ESC.write(160); 
  while(winchEncoder.read() < (newDepth - 10000))
    ESC.write(130);
  while(winchEncoder.read() < (newDepth))
    ESC.write(100); 
  ESC.write(90); 
}

void in(long newDepth){
  Serial.println("Taking line in.");
  while(winchEncoder.read() > newDepth + 200000)
    ESC.write(0);
  while(winchEncoder.read() > newDepth + 100000)
    ESC.write(20);
  while(winchEncoder.read() > newDepth + 10000)
    ESC.write(50);
  while(winchEncoder.read() > newDepth)
    ESC.write(80);
  ESC.write(90);
}
