#define reedPin 1

#include <Servo.h>

Servo ESC;

void setup() {
  // put your setup code here, to run once:
  pinMode(reedPin, INPUT);
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
  ESC.write(90);
  digitalWrite(13, HIGH);
  delay(2000);
  while (!digitalRead(reedPin)){
    ESC.write(180);
  }
  ESC.write(90);
}
