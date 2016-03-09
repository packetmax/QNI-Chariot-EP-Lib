/*
 * ChariotLib.cpp - Library for interfacing to Chariot IoT Shield
 *               
 * Created by George Wayne for Qualia Networks, Inc., 2016.
 * BSD license, all text above must be included in any redistribution.
 */
 
#include "ChariotEPLib.h"

#if LEONARDO_HOST
	SoftwareSerial ChariotClient(RX_PIN, TX_PIN);
#elif UNO_HOST
	SoftwareSerial ChariotClient(RX_PIN,  TX_PIN);
#elif MEGA_DUE_HOST

#else
	#error Board type not supported by Chariot at this time--contact Tech Support.
#endif

ChariotEPClass::ChariotEPClass()
{
	chariotAvailable = false;
	nextRsrcId = 0;
}

ChariotEPClass::~ChariotEPClass()
{
	// do nothing
}

boolean ChariotEPClass::begin() 
{	
#if LEONARDO_HOST
	pinMode(RX_PIN, INPUT);
	pinMode(TX_PIN, OUTPUT);
	arduinoType	= LEONARDO;
#elif UNO_HOST
	pinMode(RX_PIN, INPUT);
	pinMode(TX_PIN, OUTPUT);
	arduinoType = UNO;
#elif MEGA_DUE_HOST
	arduinoType = MEGA_DUE;
#else
	#error Board type not supported by Chariot at this time--contact Qualia Networks Tech Support.
#endif
	
	/*
	 * Start Chariot/Arduino channel
	 */
	ChariotClient.begin(9600);
	Serial.println(F("Chariot communication channel initialized."));
	Serial.println(F("...waiting for Chariot to come online"));
	
	/*
	 * Set event pins and wait for Chariot to come up
	 *     --Note: exints are active LOW--so set HIGH for init
	 */
	pinMode(RSRC_EVENT_INT_PIN, OUTPUT);
	digitalWrite(RSRC_EVENT_INT_PIN, HIGH);
  
	// This pin driven HIGH when Chariot is active
	pinMode(CHARIOT_STATE_PIN, INPUT);
	while (digitalRead(CHARIOT_STATE_PIN) == 0) {
		delay(50);
		Serial.print(".");
	}
	Serial.println(F("...Chariot online"));
	
	// Read Chariot startup response.
	if (!ChariotClient.available()) {
		chariotSignal(RSRC_EVENT_INT_PIN);
	}
	coapResponse();	
	chariotAvailable = true;
	
	// Take Chariot's temp at startup and display.
	Serial.print(F("\nSystem temp at startup: "));
	Serial.print(readTMP275(CELSIUS), 2);
	Serial.println('C');
	Serial.println();

	// initialize vent resources--these are stored in Chariot
	nextRsrcId = 0;
	
	int i;
	for (i=0; i<MAX_RESOURCES; i++) {
		rsrcURIs[i] = "";
		rsrcATTRs[i] = "";
		putCallbacks[i] = NULL;
		rsrcChariotBufSizes[i] = 0;
	}
	
	chariotAvailable = true;
}

inline uint8_t ChariotEPClass::getArduinoModel()
{
	return arduinoType;
}

int ChariotEPClass::available()
{
	return ChariotClient.available();
}

int ChariotEPClass::getIdFromURI(String& uri)
{
	int i;
	for (i=0; i < nextRsrcId; i++) {
		if (rsrcURIs[i] == uri) {
			return i;
		}
	}
	return -1;
}

int ChariotEPClass::setPutHandler(int handle, void (*putCallback)(String& putCmd))
{
	if ((putCallback == NULL) || (handle < 0) || (handle > (nextRsrcId-1))) {
		return -1;
	}
	
	putCallbacks[handle] = putCallback;
	return 1;
		
}

int ChariotEPClass::createResource(String& uri, uint8_t bufLen, String& attrib)
{
	int rsrcNbr;
	
	if ((uri == NULL) || (bufLen == 0) || (bufLen > (MAX_BUFLEN-1)) || (attrib == NULL)) {
		return -1;
	}
	
	if (nextRsrcId == MAX_BUFLEN)
		return -1;
		
	rsrcNbr = nextRsrcId++;
	
	rsrcURIs[rsrcNbr] = uri;
	rsrcATTRs[rsrcNbr] = attrib;
		
	String rsrcString = "rsrc=";
	rsrcString += rsrcNbr;
	rsrcString += "%maxlen=";
	rsrcString += bufLen;
	rsrcString += "%uri=";
	rsrcString += String(uri);
	rsrcString += "%attr=";
	rsrcString += String(attrib);
	rsrcString += "\n\0";

	rsrcChariotBufSizes[rsrcNbr] = min(bufLen, MAX_BUFLEN);
	
	ChariotClient.print(rsrcString);
    chariotSignal(RSRC_EVENT_INT_PIN);  // Publish Create via CoAP
      
	while (ChariotClient.available() == 0) ;  // wait on response
	// Parse this for result of last resource operation
	String input = ChariotClient.readStringUntil('\r');
	input.remove(input.indexOf("<<"), 2);
	Serial.print(input);

	if (!(input.length() >= 20) || !input.startsWith("chariot/2.01 CREATED", 0))
	{ 
		nextRsrcId--;
		return -1;
	}
	
	Serial.print(F("    "));
	Serial.println(uri);
	return rsrcNbr;
}

// use F("uri...") and F("attrib...") in your sketch to save memory for Uno and Leonardo
int ChariotEPClass::createResource(const __FlashStringHelper* uri, uint8_t bufLen, const __FlashStringHelper* attrib)
{
	int rsrcNbr;
	
	if ((uri == NULL) || (bufLen == 0) || (bufLen > (MAX_BUFLEN-1)) || (attrib == NULL)) {
		return -1;
	}
	
	if (nextRsrcId == MAX_BUFLEN)
		return -1;
		
	rsrcNbr = nextRsrcId++;
	
	rsrcURIs[rsrcNbr] = uri;
	rsrcATTRs[rsrcNbr] = attrib;

	String rsrcString = "rsrc=";
	rsrcString += rsrcNbr;
	rsrcString += "%maxlen=";
	rsrcString += bufLen;
	rsrcString += "%uri=";
	rsrcString += String(uri);
	rsrcString += "%attr=";
	rsrcString += String(attrib);
	rsrcString += "\n\0";

	rsrcChariotBufSizes[rsrcNbr] = min(bufLen, MAX_BUFLEN);
	   
	ChariotClient.print(rsrcString);
    chariotSignal(RSRC_EVENT_INT_PIN); 
       
	while (ChariotClient.available() == 0) ;  // wait on coap response
	// Parse this for result of last resource operation
	String input = ChariotClient.readStringUntil('\r');
	input.remove(input.indexOf("<<"), 2);
	Serial.print(input);

	if (!(input.length() >= 20) || !input.startsWith("chariot/2.01 CREATED", 0))
	{ 
		nextRsrcId--;
		return -1;
	}
	
	Serial.print(F("    "));
	Serial.println(String(uri));
	return rsrcNbr;
}

bool ChariotEPClass::triggerResourceEvent(int handle, String& eventVal, bool signalChariot)
{
	String ev = "";
	
	if ((handle < 0) || (handle > (nextRsrcId-1)))
		return false;
		
	ev = "rsrc=";
	ev += handle;
	ev += "%";
	ev += "value=";
	ev += eventVal;
	ev += "<\n\0";
	if (ev.length() > rsrcChariotBufSizes[handle])
		return false;
		
	ChariotClient.print(ev); 
	if (signalChariot) {
		chariotSignal(RSRC_EVENT_INT_PIN); 
	}
	
	while (ChariotClient.available() == 0) ;  // wait on response
	// Parse this for result of last resource operation
	String input = ChariotClient.readStringUntil('\r');
	input.remove(input.indexOf("<<"), 2);
	Serial.print(input);

	if (!(input.length() >= 20) || !input.startsWith("chariot/2.01 CREATED", 0))
	{ 
		Serial.println();
		return false;
	}
	
	Serial.print(F("    "));
	Serial.println(ev);
	return true;
}

bool ChariotEPClass::triggerResourceEvent(int handle, const __FlashStringHelper* eventVal, bool signalChariot)
{
	String ev = "";
	
	if ((handle < 0) || (handle > (nextRsrcId-1)))
		return false;
		
	ev = "rsrc=";
	ev += handle;
	ev += "%";
	ev += "value=";
	ev += String(eventVal);
	ev += "<\n\0";
	if (ev.length() > rsrcChariotBufSizes[handle])
		return	false;
		
	ChariotClient.print(ev); 
	if (signalChariot) {
		chariotSignal(RSRC_EVENT_INT_PIN); 
	}
	
	while (ChariotClient.available() == 0) ;  // wait on response
	// Parse this for result of last resource operation
	String input = ChariotClient.readStringUntil('\r');
	input.remove(input.indexOf("<<"), 2);
	Serial.print(input);

	if (!(input.length() >= 20) || !input.startsWith("chariot/2.01 CREATED", 0)) {
		Serial.println();
		return false;
	}
	
	Serial.print(F("    "));
	Serial.println(ev);	
	return true;
}

/*----------------------------------------------------------------------*/
void ChariotEPClass::process() 
{
  // read the command--terminate with '\n'
  String command = ChariotClient.readStringUntil('/');
  
#if EP_DEBUG 
  Serial.println(command);
#endif
  // is "digital" command?
  if (command == F("digital")) {
    digitalCommand();
	return;
  }

  // is "analog" command?
  else if (command == F("analog")) {
    analogCommand();
	return;
  }

  // is "mode" command?
  else if (command == F("mode")) {
    modeCommand();
	return;
  }

  // is "put" parameters for event resource?
  else if (command == F("event") && (ChariotClient.available())) {
	command += "/"; // has to be put back into string
    command += ChariotClient.readStringUntil('&');
    
	String param = "";
	// Chariot only sends us qualified input--here we're tossing stuff we don't support in this lib vers
	if (ChariotClient.available()) {
		param = ChariotClient.readStringUntil('\r');
	}
	
	int id;
	id = getIdFromURI(command);
	if ((id != -1) && (putCallbacks[id] != NULL) &&( param != ""))
	{
		putCallbacks[id](param);
	}
	return;
  }
  Serial.println(command);
}

void ChariotEPClass::digitalCommand() {
  int pin, value;
  String response;

  // Read pin number
  pin = ChariotClient.parseInt();
#if EP_DEBUG 
  Serial.print(F("pin="));
  Serial.println(pin, DEC);
#endif
  // If the next character is a '/' it means we have an URL
  // with a value like: "/digital/13/1"
  if (ChariotClient.read() == '/') {
#if EP_DEBUG 
    Serial.println(F("command is WRITE"));
#endif
    value = ChariotClient.parseInt();
    digitalWrite(pin, value);
  }
  else {
    value = digitalRead(pin);
#if EP_DEBUG 
    Serial.println(F("command is READ"));
#endif
  }
  
  // Send pin response to requestor
  response = "Pin D";
  response += pin;
  response += " set to ";
  response += value;
  response += "\n\0";
  ChariotClient.print(response);
#if EP_DEBUG 
  Serial.print(response);
#endif
}

void ChariotEPClass::analogCommand() {
  int pin, value;
  String response;

  // Read pin number
  pin = ChariotClient.parseInt();
#if EP_DEBUG 
  Serial.print(F("pin="));
  Serial.println(pin, DEC);
#endif

  // If the next character is a '/' it means we have an URL
  // with a value like: "/analog/5/120"
  if (ChariotClient.read() == '/') {
#if EP_DEBUG 
    Serial.println(F("command is WRITE"));
#endif
    value = ChariotClient.parseInt();
    analogWrite(pin, value);
  }
  else {
    value = analogRead(pin);
#if EP_DEBUG 
    Serial.println(F("command is READ"));
#endif
  }

  // Send pin response to requestor
  response = "Pin A";
  response += pin;
  response += " set to ";
  response += value;
  response += "\n\0";
  ChariotClient.print(response);
#if EP_DEBUG 
  Serial.print(response);
#endif
}

void ChariotEPClass::modeCommand() {
  int pin;
  String response;

  // Read pin number
  pin = ChariotClient.parseInt();
#if EP_DEBUG 
  Serial.print(F("pin="));
  Serial.println(pin, DEC);
#endif

  // If the next character is not a '/' we have a malformed URL
  if (ChariotClient.read() != '/') {
    ChariotClient.println(F("Arduino remote error: malformed URI"));
    return;
  }

  String mode = ChariotClient.readStringUntil('\r');

  if (mode == F("input")) {
#if EP_DEBUG 
    Serial.println(F("command is INPUT"));
#endif
    pinMode(pin, INPUT);

    // Send pin response to requestor
    response = "Pin D";
    response += pin;
    response += " configured as ";
    response += "INPUT";
    response += "\n\0";
    ChariotClient.print(response);
    return;
  }

  if (mode == F("output")) {
#if EP_DEBUG 
    Serial.println(F("command is OUTPUT"));
#endif
    pinMode(pin, OUTPUT);

    // Send pin response to requestor
    response = "Pin D";
    response += pin;
    response += " configured as ";
    response += "OUTPUT";
    response += "\n\0";
    ChariotClient.print(response);
    return;
  }
#if EP_DEBUG 
  Serial.print(F("Arduino remote error: invalid mode requested: "));
  Serial.println(mode);
#endif
  ChariotClient.print(F("Arduino remote error: invalid mode "));
  ChariotClient.print(mode);
}

/* Toggle Chariot's interrupt line 
 *  to signal request--put me in a function
 * Chariot will respond with "Chariot ready"
 */
void ChariotEPClass::chariotSignal(int pin) {
  noInterrupts();
  digitalWrite(pin, LOW);
  delay(1);
  digitalWrite(pin, HIGH);
  interrupts();
}

void ChariotEPClass::coapResponse()
{ 
  String initString = "";
  char ch;
  uint8_t ltSeen = 0, availChars;

  while (!ChariotClient.available()) ;
  
  while ((availChars = ChariotClient.available()) || (ltSeen < 2)) {
    if (availChars) {
      ch = (char)ChariotClient.read();
  
      if (ch != '<') {
        initString += ch;
      }
      else {
        ltSeen++;
      }
    }
  }
  Serial.println(initString);
}

/*
 * Process local chariot commands from the Serial port.
 *  NB: these must have 'chariot' prefix removed (i.e., sys/motes, sys/health, sys/status).
 */
void ChariotEPClass::serialChariotCmd()
{
  String chariotLclCmd = "";
  char newChar;
  bool terminator_seen = false;
  int len = 0;
  
  while (Serial.available()) {
    newChar = (char) Serial.read();
    len++;
   
    /**
     * End of line input reached--trim whitespace and nul terminate
     */
    if (newChar == LF) {
      chariotLclCmd += (char)'\0';
      terminator_seen = true;
    }

    if (!terminator_seen) {
      chariotLclCmd += newChar;
    }
  }

  // "sys/motes" is shortest command
  if (len >= 9) {
    Serial.print(F("Cmd: "));
    Serial.println(chariotLclCmd);
    ChariotClient.print(chariotLclCmd);
    coapResponse();
  }
}

/*-----------------------------------------------------------------------------------------------*/
/* There isn't a really good reason for this to be here. A separate sensors class could be used. */
/* --although TMP275 is in the EP...
/*-----------------------------------------------------------------------------------------------*/
float ChariotEPClass::readTMP275(uint8_t units)
{
  char tempHighByte, tempLowByte;
  double temperature;
  long t;

#ifndef I_AM_EXCLUSIVE_I2C_OWNER
  // init I2C
  Wire.begin();
#endif
  Wire.beginTransmission(TMP275_ADDRESS);
  Wire.write(1);
  Wire.write(B11100001);
  Wire.endTransmission();
  Wire.beginTransmission(TMP275_ADDRESS);
  Wire.write(0);
  Wire.endTransmission();
  Wire.requestFrom(TMP275_ADDRESS,2);
  delay(250);
  tempHighByte = Wire.read();
  tempLowByte  = Wire.read();

  t = word(tempHighByte,tempLowByte)/16;
  temperature = (t/10)*625; // TMP275 is accurate to .0625C
  temperature /= 1000.0;
  
#define TMP275_TRIGGER_DEBUG (0)
#if TMP275_TRIGGER_DEBUG
Serial.print(F("TMP275 bits: "));
Serial.print(t, BIN); Serial.print("  0x"); Serial.println(t, HEX);
#endif

  if (units == FAHRENHEIT) {
      temperature = temperature*1.8 + 32.0;
  } 
  Wire.endTransmission(true); // Let go of the bus
#ifndef I_AM_EXCLUSIVE_I2C_OWNER
  // terminate I2C
  Wire.end();
#endif
  return (float)temperature;
}
ChariotEPClass ChariotEP; // Create an object