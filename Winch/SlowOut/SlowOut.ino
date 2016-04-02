#include <Servo.h>
Servo ESC;
void setup() {
  // put your setup code here, to run once:
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
  delay(1000);
  ESC.write(115);
}
