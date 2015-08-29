/*

ROSS R/C Test File

Nick McComb [mccombn@onid.oregonstate.edu]
Written August 2015 [at sea]

Arduino UNO Pin Allocation

D4 - R/C Channel Input
D3 - KILL Relay Output
D2 - START Relay Output


*/

const byte RC_INPUT = 4;
const byte KILL_RELAY = 3;
const byte START_RELAY = 2;

#define DEBUG

void setup(){
  Serial.begin(115200); 
  Serial.println("[Begin ROSS R/C Test]");
  
  pinMode(RC_INPUT, INPUT);
}


void loop(){
  
  
  //Method to check RC Signal Length
  short RC7_len;
  RC7_len = pulseIn(RC_INPUT, HIGH, 20500);  //Timeout is 20.5 mS because that's the "frame" of a PWM signal
  Serial.print("RC7 Value: ");
  Serial.println(RC7_len);

  if(!RC7_len){  //RC Transmitter not-on
    Serial.println("## RC Transmitter not on! ##");
  }
  else if (RC7_len < 1300){  //Switch in low position
    Serial.println("[R/C Transmitter in low position]");
  }
  else if (RC7_len > 1300 && RC7_len < 1700){  //Switch in middle position
    Serial.println("[R/C Transmitter in middle position]");
  }
  else if (RC7_len > 1700){ //Switch in high position
    Serial.println("[R/C Transmitter in high position]");
  }
  
  
  //delay(100);
}
