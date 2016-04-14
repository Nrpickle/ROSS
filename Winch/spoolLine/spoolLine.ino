#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Servo.h>
#include <Encoder.h>
Servo ESC; //Create ESC object
Encoder winchEncoder(3,2); //Create encoder object
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  delay(5000); //Ensure the winch is powered on before calibration sequence begins //Increase for final revision
  ESC.attach(9);//Begin calibration sequence
  ESC.write(180);
  delay(4000);
  ESC.write(0);
  delay(4000);
  ESC.write(90);
  delay(6000);
  winchEncoder.write(0);
  while(winchEncoder.read() < 90000)
    {ESC.write(115);}
  ESC.write(90);
}

void loop() {
  // put your main code here, to run repeatedly:
  while(!Serial.available());
  ESC.write(80);


}
