#include <ChariotEPLib.h>

 /*
 This example for the Arduino Uno shows how to use the
 Chariot to provide access the digital and analog pins
 on the board through REST calls. It also shows how sketches 
 can interoperate over the Chariot 6LoWPAN/CoAP network to
 also control other sketches in a connected mesh.

 Possible commands created in this shetch:
 
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

/*
 * Simple trigger state vars
 */
static uint8_t triggerState = OFF;
static bool    isTriggered = false;
static uint8_t triggerFunc = GT;
static float   triggerVal = 30.0;
static float   triggerCalOffset = -3.0;
static int     triggerPeriod = 1;
static int     triggerTimeUnit = SECONDS;
static unsigned long triggerChkTime;     // Holds the next check time.

// Resource creation yields positive handle
static int eventHandle = -1;

void triggerPutCallback(String& param); // RESTful PUTs on this URI come here.
bool triggerCreate();

void inline resetTriggerTime() 
{
  triggerChkTime = (triggerTimeUnit == SECONDS) ? (millis() + triggerPeriod*1000) :  
                                                  (millis() + triggerPeriod*1000*60); 
}

void setup() {  
 // arduino IDE/Tools/Port for console and debug:
  Serial.begin(9600);
  while (!Serial) {
    // wait serial port initialization
  }

  //---Put Arduino resources on the air---
  ChariotEP.begin();

  //---Create event trigger---
  if (triggerCreate()) {
    Serial.println(F("Setup complete."));
  }

  resetTriggerTime();
}

void loop() {
  /*
   * Answer remote RESTful GET, PUT, DELETE, OBSERVE API calls
   * transparently.
   */
  if (ChariotEP.available()) {
   ChariotEP.process();
  }

  /* 
   *  Filter your own inputs first--pass everthing else here.
   *   --try typing 'sys/help' into the Serial window
   */
  if (Serial.available()) {
    ChariotEP.serialChariotCmd();
  }

  /* 
   *  Examine trigger
   */
  if (millis() > triggerChkTime) {
    resetTriggerTime();
    if (triggerCheck()) {
      String triggeredVal = "{\"ID\":\"Trigger\",\"Triggered\":\"Yes\",\"State\":\"Off\"}";;
      ChariotEP.triggerResourceEvent(eventHandle, triggeredVal, true); 
    }
  }
  
  delay(50);
}

/*
 * Create a resource that Chariot connects to your web of things.
 *   --also set up the PUT callback that will receive RESTful API calls.
 */
bool triggerCreate()
{
  String trigger = "event/tmp275-c/trigger";
  String attr = "title=\"Trigger\?get|obs|put\"";
  String eventVal = "{\"ID\":\"Trigger\",\"Triggered\":\"No\",\"State\":\"Off\"}";
  
  if ((eventHandle = ChariotEP.createResource(trigger, 63, attr)) >= 0)   // create resource on Chariot
  {
    ChariotEP.triggerResourceEvent(eventHandle, eventVal, true);        // set its initial condition (JSON)
    ChariotEP.setPutHandler(eventHandle, triggerPutCallback);             // set RESTful PUT handler
    return true;
  } 
  
  Serial.println(F("Could not create resource!"));
  return false;
}

/*
 * Trigger example using Chariot's I2C TMP275 temp sensor
 */
bool triggerCheck()
{
  float t;
  
  if (triggerState == OFF)
    return false;
    
  // trigger condition present?
  t = ChariotEP.readTMP275(CELSIUS);
  if (((triggerFunc == GT) && (t > triggerVal))
      || ((triggerFunc == LT) && (t < triggerVal))) 
  {
      triggerOnOff(OFF);
      return true;
  }
  return false;
}

bool triggerOnOff(bool onOffSwitch)
{
  bool saveState = triggerState;
  triggerState = onOffSwitch;
  return saveState;
}

/*
 * This is the handler for all PUT API calls. By convention,
 * we will receive a string that looks like this:
 *   param=name val=value
 *   "name and val" are strings that consist of just about any 
 *   characters other than '='.
 */
void triggerPutCallback(String& param)
{
  String Name, Value;

  Name = param.substring(0, param.indexOf('='));
  Value = param.substring(param.indexOf('=')+1, '\r');

  if (Name == "triggerval") 
  {
    triggerVal = (float)Value.toInt();
    goto changeNotification;
  }

   if (Name == "caloffset") 
   {
    triggerCalOffset = (float)Value.toInt();
    goto changeNotification;
  }

  if (Name == "state") 
  {
    if (Value == "on") {
      // Reset trigger state.
      String triggerVal = "{\"ID\":\"Trigger\",\"Triggered\":\"No\",\"State\":\"On\"}";
      ChariotEP.triggerResourceEvent(eventHandle, triggerVal, true);
      triggerOnOff(true);
      return;
    } else if (Value == "off") {  // Preserve trigger state.
      triggerOnOff(false);
    } else {
      void badInput(String param);
      badInput(param);
      return;
    }
    goto changeNotification;
  }

  if (Name == "fetch") {
    void triggerFetch();
    triggerFetch();
    return;
  }

  if (Name == "func") {
    if (Value == "lt") {
     triggerFunc = LT;
    } else if (Value == "gt") {
      triggerFunc = GT;
    } else {
      badInput(param);
      return;
    }
  }

changeNotification:
  // Valid param and value exit
  String changedParam = "2.04 PARAMETER(" + param + ")" + " CHANGED";
  changedParam += "\n\0";
  ChariotClient.print(changedParam);
}

/*
 * Param not understood
 */
void badInput(String param) 
{
  String unknownInput = "4.02 UNKNOWN, MISSING, OR BAD PARAMETER(" + param + ")";
  unknownInput += "\n\0";
  ChariotClient.print(unknownInput);
  return;
}

/*
 * use PUT to fetch the trigger object
 *   --value is noise word-anystring works
 */
void triggerFetch() {
  String statusJSON = "{\"ID\":\"Trigger\",";

  if (triggerState == OFF) {
    statusJSON += "\"State\":\"Off\", "; 
  } else {
    statusJSON += "\"State\":\"On\", "; 
  }

  if (triggerFunc == GT) {
    statusJSON += "\"Func\":\"GT\", "; 
  } else {
    statusJSON += "\"Func\":\"LT\", "; 
  }

  statusJSON += "\"TriggerVal\":" + String(triggerVal) + ","; 
  statusJSON += "\"CalOffset\":" + String(triggerCalOffset) + ",";

  statusJSON += "\n\0";
  ChariotClient.print(statusJSON);
}


