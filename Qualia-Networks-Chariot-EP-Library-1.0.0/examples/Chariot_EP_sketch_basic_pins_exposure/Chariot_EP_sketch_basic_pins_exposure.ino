#include <ChariotEPLib.h>

 /*
 This example for the Arduino Uno shows how to use the
 Chariot to provide access the digital and analog pins
 on the board through REST calls. It also shows how sketches 
 can interoperate over the Chariot 6LoWPAN/CoAP network to
 also control other sketches in a connected mesh.

  Possible commands to this sketch (handled automatically):
 
  Remote webapp:                                               Resulting action by ChariotEPLib
  --------------------------------------------------------------------------------------------------
  coap://chariot.c350e.local/arduino/mode?put&pin=13&val=input    -> pinMode(13, INPUT)
  coap://chariot.c350e.local/arduino/digital?put&pin=13&val=0     -> digitalWrite(13, LOW)
  coap://chariot.c350e.local/arduino/digital?get&pin=13           -> digitalRead(13)           
  coap://chariot.c350e.local/arduino/analog?get&pin=5             -> analogRead(5)
  coap://chariot.c350e.local/arduino/analog?put&pin=13&val=128    -> analogWrite(2, 123) //set PWM duty cycle
 * 
 * by George Wayne, Qualia Networks Incorporated
 */

void setup() {  
 // arduino IDE/Tools/Port for console and debug:
  Serial.begin(9600);
  while (!Serial) {
    // wait serial port initialization
  }

  //---Put Arduino resources on the air---
  ChariotEP.begin();
  
  Serial.println(F("Setup complete."));
}

void loop() {
  
  //---answer RESTful API calls automatically---
  if (ChariotEP.available()) {
   ChariotEP.process();
  }


  //---type 'sys/help' for available
  if (Serial.available()) {
    ChariotEP.serialChariotCmd();
  }
  delay(50);
}

