#define reedPin 1
#define ledPin 13

#include <Servo.h>

Servo ESC;

void setup() {
  // put your setup code here, to run once:
  pinMode(reedPin, INPUT);
  pinMode(ledPin, OUTPUT);
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
  if (digitalRead(reedPin) == LOW){
     ESC.write(90);
  }
  if (digitalRead(reedPin) == HIGH){
     ESC.write(110);
  }
}
