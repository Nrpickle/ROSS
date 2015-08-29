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
const short SAFETY_THRESHOLD = 24; //The test loop runs at ~4hz, so SAFETY_THRESHOLD/4 = no. seconds, eg 24 = ~6seconds



#define DEBUG

void setup(){
  Serial.begin(115200); 
  Serial.println("[Begin ROSS R/C Test]");
  
  pinMode(RC_INPUT, INPUT);
  pinMode(KILL_RELAY, OUTPUT);
  pinMode(START_RELAY, OUTPUT);
}


void loop(){
  
  static int safetyCounter = 0; //Init this var as 0, this variable will count the number of iterations that a kill or start signal is sent, so that they are not sent for more than about 10 seconds. 
                                //If this variable reaches too high, then no relay can activate
  
  
  //Check only ~4hz, but allow GPS to check properly
  for(int i = 0; i < 12; ++i){
    if (i == 0){
      //Method to check RC Signal Length
      short RC7_len;
      RC7_len = pulseIn(RC_INPUT, HIGH, 20500);  //Timeout is 20.5 mS because that's the "frame" of a PWM signal
      #ifdef DEBUG
        Serial.print("RC7 Value: ");
        Serial.println(RC7_len);
      #endif
        
      if(!RC7_len){  //RC Transmitter not-on, do nothing
        #ifdef DEBUG
          Serial.println("## RC Transmitter not on! ##");
        #endif
        digitalWrite(KILL_RELAY, LOW);
        digitalWrite(START_RELAY, LOW);
        safetyCounter = 0;
      }
      else if (RC7_len < 1300){  //Switch in low position, start engine
        #ifdef DEBUG
          Serial.println("[R/C Transmitter in low position]");
        #endif
        if(safetyCounter < SAFETY_THRESHOLD){
          digitalWrite(KILL_RELAY, LOW);
          digitalWrite(START_RELAY, HIGH);
        }
        else {
          digitalWrite(KILL_RELAY, LOW);
          digitalWrite(START_RELAY, LOW);
        }
        ++safetyCounter;
      }
      else if (RC7_len > 1300 && RC7_len < 1700){  //Switch in middle position, do nothing
        #ifdef DEBUG
          Serial.println("[R/C Transmitter in middle position]");
        #endif
        digitalWrite(KILL_RELAY, LOW);
        digitalWrite(START_RELAY, LOW);
        safetyCounter = 0;
      }
      else if (RC7_len > 1700){ //Switch in high position, kill engine
        #ifdef DEBUG
          Serial.println("[R/C Transmitter in high position]");
        #endif
        if(safetyCounter < SAFETY_THRESHOLD){
          digitalWrite(KILL_RELAY, HIGH);
          digitalWrite(START_RELAY, LOW);        
        }
        else{
          digitalWrite(KILL_RELAY, LOW);
          digitalWrite(START_RELAY, LOW);        
        }
        ++safetyCounter;
      }
    #ifdef DEBUG
      if(digitalRead(KILL_RELAY)){
        Serial.println("KILL_RELAY activated");
      }
      else {
        Serial.println("KILL_RELAY deactivated");
      }
      if(digitalRead(START_RELAY)){
        Serial.println("START_RELAY activated");
      }
      else {
        Serial.println("START_RELAY deactivated");
      }
    #endif
    }
    else {  //Do GPS stuff
      //Serial.println("[Processing GPS]");
      for(int j = 0; j < 20; ++j){
        delay(1);
      }
    }
  }
  
  //delay(100); //Poor lonely delay
}
