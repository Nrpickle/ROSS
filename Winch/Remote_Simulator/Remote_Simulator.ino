#include <Servo.h>
#define MAX_FORWARD 1910 //Maximum, minimum, and neutral pulse widths in microseconds
#define NEUTRAL 1479
#define MAX_REVERSE 1048
Servo ESC;
void setup() {
  // put your setup code here, to run once:
  ESC.attach(9, MAX_REVERSE, MAX_FORWARD);
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(!Serial.available());
  int val = Serial.read();
  if(val == 1)
    ESC.writeMicroseconds(MAX_REVERSE);
  else if(val == 3)
    ESC.writeMicroseconds(MAX_FORWARD);
  else
    ESC.writeMicroseconds(NEUTRAL);
}
