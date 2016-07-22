#define reedPin 11
#define reedPin2 10
#define NEUTRAL 1479

#include <Servo.h>

Servo ESC;

void setup() {
  // put your setup code here, to run once:
  pinMode(reedPin, INPUT);
  pinMode(reedPin2, INPUT);
  pinMode(13, OUTPUT);
 
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(reedPin) == LOW){
     digitalWrite(13, HIGH);
  }
  else if (digitalRead(reedPin2) == LOW){
     digitalWrite(13, HIGH);
  }
  else
    digitalWrite(13, LOW);
}
