/*
 * ChariotLib.cpp - Library for interfacing to Chariot IoT Shield
 *               
 * Created by George Wayne for Qualia Networks, Inc., 2016.
 * BSD license, all text above must be included in any redistribution.
 */
 
#include "ChariotEPLib.h"

#if UNO_HOST==1 || LEONARDO_HOST==1
	SoftwareSerial ChariotClient(RX_PIN, TX_PIN);
#elif MEGA_DUE_HOST==1

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
#error Leonardo has been discontinued and is not support by Chariot
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
	SerialMon.println(F("Chariot communication channel initialized."));
	SerialMon.println(F("...waiting for Chariot to come online"));
	
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
		SerialMon.print(".");
	}
	SerialMon.println(F("...Chariot online"));
		
	// Take Chariot's temp at startup and display.
	SerialMon.print(F("\nSystem temp at startup: "));
	SerialMon.print(readTMP275(CELSIUS), 2);
	SerialMon.println('C');
	SerialMon.println(F("\ntype \"help\" to see available Serial commands"));
	SerialMon.println();
	
	// Wait for Chariot startup response.
	if (!ChariotClient.available()) {
		String Cmd = "sys/status<\n\0";
		ChariotClient.print(Cmd);
	}
	chariotPrintResponse();	
	chariotAvailable = true;

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

uint8_t ChariotEPClass::getArduinoModel() { return arduinoType; }
void ChariotEPClass::enableDebugMsgs() { debug = true; }
void ChariotEPClass::disableDebugMsgs() { debug = false; }

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

int ChariotEPClass::setPutHandler(int handle, String * (*putCallback)(String& putCmd))
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
	int terminator = input.indexOf("<<");
	if (terminator != -1)
		input.remove(terminator, 2);
	SerialMon.print(input);
	
	int goodResponse = input.indexOf("chariot/2.01 CREATED");
	if (goodResponse == -1)
	{ 
		nextRsrcId--;
		return -1;
	}
	
	SerialMon.print(F("  "));
	SerialMon.println(uri);
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
	rsrcString += "<\n\0";

	rsrcChariotBufSizes[rsrcNbr] = min(bufLen, MAX_BUFLEN);
	   
	ChariotClient.print(rsrcString);
    chariotSignal(RSRC_EVENT_INT_PIN); 
       
	while (ChariotClient.available() == 0) ;  // wait on coap response
	// Parse this for result of last resource operation
	String input = ChariotClient.readStringUntil('\r');
	int terminator = input.indexOf("<<");
	if (terminator != -1)
		input.remove(terminator, 2);
	SerialMon.print(input);

	if (!(input.length() >= 20) || !input.startsWith("chariot/2.01 CREATED", 0))
	{ 
		nextRsrcId--;
		return -1;
	}
	
	SerialMon.print(F("    "));
	SerialMon.println(String(uri));
	return rsrcNbr;
}

bool ChariotEPClass::triggerResourceEvent(int handle, String& eventVal, bool signalChariot)
{
	String ev = "";
	String chariotResponse = "";
	
	if ((handle < 0) || (handle > (nextRsrcId-1))) {
		SerialMon.print(F("Bad handle: "));
		SerialMon.println(handle);
		return false;
	}
		
	ev = "rsrc=";
	ev += handle;
	ev += "%";
	ev += "value=";
	ev += eventVal;
	ev += "<\n\0";
	if (ev.length() > rsrcChariotBufSizes[handle]) {
		SerialMon.print(F("triggerResourceEvent: "));
		SerialMon.print(ev);
		SerialMon.print(F(" of length: "));
		SerialMon.print(ev.length());
		SerialMon.print(F(" exceeds allowable length of: "));
		SerialMon.println(rsrcChariotBufSizes[handle]);
		return false;
	}
	// Send Chariot the resource state change
	ChariotClient.print(ev); 
	
	while (ChariotClient.available() == 0) ;  // wait on response
	// Parse response for result of last resource operation
	if (ChariotClient.available()) {
		chariotResponse = ChariotClient.readStringUntil('\r');
		int terminator = chariotResponse.indexOf("<<");
		if (terminator != -1)
			chariotResponse.remove(terminator, 2);
	} else {
		SerialMon.println(F("No response from Chariot"));
		return false;
	}

	int goodResponse = chariotResponse.indexOf("CREATED");
	if (goodResponse == -1)
	{ 
		SerialMon.print(F("Chariot response indicates an error. handle = "));
		SerialMon.println(handle);
		SerialMon.println(ev);
		SerialMon.print(F("signal = "));
		SerialMon.println(signalChariot);
		SerialMon.print(F("response from Chariot = "));
		SerialMon.println(chariotResponse);
		SerialMon.print(F("len = "));
		SerialMon.println(chariotResponse.length());
		return false;
	}
	// Signal Chariot to notify all subscribers
	if (signalChariot) {
		chariotSignal(RSRC_EVENT_INT_PIN); 
	}
	return true;
}

/*----------------------------------------------------------------------*/
void ChariotEPClass::process() 
{
  // read the command--terminate with '\n'
  String command = ChariotClient.readStringUntil('\0');
  bool validCmd;
  
  validCmd = command.startsWith(F("arduino/"), 0);
  if (!validCmd) {
	validCmd = command.startsWith(F("event/"), 0);
  }
  
  if (!validCmd) {
	SerialMon.print(F("Unrecognized input from Chariot: "));
	SerialMon.println(command);
	return;
  }
  
#if EP_DEBUG 
  SerialMon.print(command);
#endif
  
  if (command.startsWith(F("arduino/"), 0)) {
	  command.remove(0, 8);
	  
	  // is "digital" command?
	  if (command.startsWith(F("digital/"), 0)) {
		command.remove(0, 8);
		digitalCommand(command);
		return; 
	  }

	  // is "analog" command?
	  if (command.startsWith(F("analog/"), 0)) {
		command.remove(0, 7);
		analogCommand(command);
		return; 
	  }

	  // is "mode" command?
	  if (command.startsWith(F("mode/"), 0)) {
		command.remove(0, 5);
		modeCommand(command);
		return; 
	  }
	  return;
  }

  // is "put" of parameters for event resource?
  else {
	  int id = -1;
	  String param = "";
	  if (command.startsWith(F("event/"), 0) != -1) {
		param = command;
		
		int paramStart = param.indexOf('&');
		if (paramStart != -1) {
			param.remove(0, paramStart+1);
			param.trim();
			command.remove(paramStart, command.length()-1);
#if EP_DEBUG
  SerialMon.println(command);
  SerialMon.println(param);
#endif
		} else {
			SerialMon.println(F("PUT parameters did not arrive"));
			SerialMon.println(command);
			return;
		}
		
		id = getIdFromURI(command);
		if ((id != -1) && (putCallbacks[id] != NULL) &&( param != ""))
		{
			String *Str;
			if ((Str = putCallbacks[id](param)) != NULL)
			{
				delay(250);
				triggerResourceEvent(id, *Str, true);
			}
		}
		return;
	  } 
#if EP_DEBUG
	  else {
		SerialMon.print(F("Command: "));
		SerialMon.print(command);
		SerialMon.print(F(" not understood. ID was: "));
		SerialMon.println(id);
		return;
	  }
#endif
  }
}

int ChariotEPClass::coapResponseGet(String& response)
{
  char ch;
  uint8_t ltSeen = 0;

  response = "";
  while (ltSeen < 2) {
    if (ChariotClient.available() > 0) {
      ch = (char)ChariotClient.read();
  
      if (ch != '<') {
        response += ch;
      }
      else {
        ltSeen++;
      }
    }
  }
  return ChariotClient.available();
}

void ChariotEPClass::digitalCommand(String& command) {
  int pin, value;
   String response;

  // Read pin number
  if (pinValParse(command, &pin, &value)) {
  
#if EP_DEBUG 
    SerialMon.print(F("pin="));
    SerialMon.println(pin, DEC);
#endif

    // If value is not -1 it means we have an URL
    // with a value like: "/digital/13/1"
    if (value >= 0) {
#if EP_DEBUG 
      SerialMon.println(F("command is WRITE"));
#endif
	  if (value > 1)
		goto error;
		
      digitalWrite(pin, value);
    }
    else {
      value = digitalRead(pin);
#if EP_DEBUG 
      SerialMon.println(F("command is READ"));
#endif
    }
  
    // Send pin response to requestor
    response = "Pin D";
    response += pin;
    response += " set to ";
    response += value;
    response += "<\n\0";
    ChariotClient.print(response);
  
#if EP_DEBUG 
    SerialMon.println(response);
#endif
    return;
  }
error: // Pin value not available.
  SerialMon.print(F("digital command--pin values incorrect or missing. Pin = "));
  SerialMon.print(pin);
  SerialMon.print(F(" Value = "));
  SerialMon.println(value);
  SerialMon.println(F("Operation cancelled."));
  // Return response
  ChariotClient.print(F("Arduino could not complete digital pin request.<\n\0"));
}

void ChariotEPClass::analogCommand(String& command) {
  int pin, value;
  String response;

  // Read pin number
  if (pinValParse(command, &pin, &value)) {

#if EP_DEBUG 
  SerialMon.print(F("pin="));
  SerialMon.println(pin, DEC);
#endif

  // Write is requested with a value like: "/analog/5/120"
	if (value >= 0) {
#if EP_DEBUG 
		SerialMon.println(F("command is WRITE"));
#endif
		analogWrite(pin, value);
	}
	else {
		value = analogRead(pin);
#if EP_DEBUG 
  SerialMon.println(F("command is READ"));
#endif
	}

	// Send pin response to requestor
	response = "Pin A";
	response += pin;
	response += " set to ";
	response += value;
	response += "<\n\0";
	ChariotClient.print(response);
  
#if EP_DEBUG 
  SerialMon.println(response);
#endif
  } else { // Pin value not available.
	SerialMon.print(F("analog command--pin values incorrect or missing. Pin = "));
	SerialMon.print(pin);
	SerialMon.print(F(" Value = "));
	SerialMon.println(value);
	SerialMon.println(F("Operation cancelled."));
	// Return response
	ChariotClient.print(F("Arduino could not complete analog pin request.<\n\0"));
  }
}

void ChariotEPClass::modeCommand(String& command) {
  int pin; int value;
  String response, mode;

  // Read pin number and mode to set
  if (pinValParse(command, &pin, &value)) {
  
#if EP_DEBUG 
  SerialMon.print(F("pin="));
  SerialMon.print(pin, DEC);
  SerialMon.print(F(" val="));
  SerialMon.println(value, DEC);
#endif

  if (value == INPUT) {
    pinMode(pin, INPUT);
	mode = "INPUT";
  } else if (value  == OUTPUT) {
    pinMode(pin, OUTPUT);
	mode = "OUTPUT";
  } else if (value == INPUT_PULLUP) {
    pinMode(pin, INPUT_PULLUP);
	mode = "INPUT_PULLUP";
  } else {
	goto mode_error;
  }
  
 #if EP_DEBUG 
	SerialMon.print(F("mode is "));
	SerialMon.println(mode);
#endif

    // Send pin response to requestor
    response = "Pin D";
    response += pin;
    response += " configured as ";
    response += mode;
    response += "<\n\0";
    ChariotClient.print(response);
    return;
  }
#if EP_DEBUG 
  SerialMon.print(F("Arduino remote error: invalid mode requested: "));
  SerialMon.println(mode);
#endif
mode_error:
  ChariotClient.print(F("Arduino remote error: invalid mode "));
  ChariotClient.print(mode);
  ChariotClient.print("<\n\0");
}

/**
 * Parse pin number and possible value parameter from command
 */
 bool ChariotEPClass::pinValParse(String& command, int *pin, int *value) {
  int slash, secondSlash;
   
  /**
   * Do we have /pin/value, /pin/mode or simply pin?
   */
   slash = command.indexOf('/');
   if (slash != -1) {
       String pinStr, valStr;
       int valStrlen;
       
       pinStr = valStr = command;
       valStr.remove(0, slash+1);
       secondSlash = valStr.indexOf('/');
       if (secondSlash != -1) {
          valStr.remove(secondSlash, valStr.length()-1);
       }
       valStrlen = valStr.length();
       pinStr.remove(slash, pinStr.length()-1);
       *pin = pinStr.toInt();
       if (valStrlen) {
			if (valStr.indexOf("input_pullup") != -1) {
				*value = INPUT_PULLUP;
			} else if (valStr.indexOf("output") != -1) {
				*value = OUTPUT;
			} else if (valStr.indexOf("input") != -1) {
				*value = INPUT;
			} else {
				*value = valStr.toInt();
			}
       } else {
        *value = -1;
       }
       return true;
   } else {
     *pin = command.toInt();
	 *value = -1;
     return true;
   }
   
error:
  return false;
}

/* 
 * Toggle Chariot's interrupt line 
 * to signal request--put me in a function
 * Chariot will respond with "Chariot ready"
 */
void ChariotEPClass::chariotSignal(int pin) {
  noInterrupts();
  digitalWrite(pin, LOW);
  delay(1);
  digitalWrite(pin, HIGH);
  interrupts();
}

void ChariotEPClass::chariotPrintResponse()
{ 
  String response = "";
#define CHAR_AT_A_TIME
#ifdef CHAR_AT_A_TIME
  char ch;
  uint8_t ltSeen = 0;
  
  while (ltSeen < 2) {
    if (ChariotClient.available() > 0) {
      ch = (char)ChariotClient.read();
  
      if (ch != '<') {
        response += ch;
      }
      else {
        ltSeen++;
      }
    }
  }
  SerialMon.println(response);
#else
	int terminator;
	
	while (ChariotClient.available());
	while (ChariotClient.available() > 0) {
		response = ChariotClient.readStringUntil('\n');
		terminator = response.indexOf("<<");
		if (terminator != -1) {
			response.remove(terminator, 2);
		}
		SerialMon.println(response);
	}
#endif
#undef CHAR_AT_A_TIME
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

  // Process commands
  String Cmd;
	
  if (chariotLclCmd == "help") {
	serialChariotCmdHelp();
	return;
  }

  if  ((chariotLclCmd == "motes") || (chariotLclCmd == "hosts") || (chariotLclCmd == "health"))
  { 
	Cmd = "sys/";
	Cmd += chariotLclCmd;
	Cmd += "<\n\0";
	ChariotClient.print(Cmd);
    chariotPrintResponse();
  }
  else if ((chariotLclCmd == "radio") || (chariotLclCmd == "temp") || (chariotLclCmd == "accel")) 
  {
    Cmd = "sensors/";
	Cmd += chariotLclCmd;
	Cmd += "<\n\0";
	
	ChariotClient.print(Cmd);
    chariotPrintResponse();
  } else if ((chariotLclCmd.startsWith("chan", 0)) || (chariotLclCmd.startsWith("txpwr", 0)) ||
			(chariotLclCmd.startsWith("panid", 0)) || (chariotLclCmd.startsWith("panaddr", 0)))
  {
	Cmd = "sys/";
	Cmd += chariotLclCmd;
	Cmd += "<\n\0";
	ChariotClient.print(Cmd);
	while(ChariotClient.available() == 0) ;
    chariotPrintResponse();
  } 
#if EP_DEBUG
  else {
	SerialMon.print("\"");
	SerialMon.print(chariotLclCmd);
	SerialMon.print("\" ");
	SerialMon.println(F("not understood."));
  }
#endif
}

void ChariotEPClass::serialChariotCmdHelp()
{
	SerialMon.println();
	SerialMon.println(F("Available Chariot commands from Arduino Serial port:"));
	SerialMon.println(F("motes  -- see who's in our neighborhood by DNS name"));
	SerialMon.println(F("health -- display status, Chariot console info, DNS name"));
	SerialMon.println(F("radio  -- display RF signal quality parameters LQI and RSSI"));
	SerialMon.println(F("temp   -- display board temp in Celsius"));
	SerialMon.println(F("chan or chan=[11..26] get or set 802.11.4 comm channel (26 is default)"));
	SerialMon.println(F("txpwr or txpwr=[0..15], 0 being the highest setting"));
	SerialMon.println(F("panid or panid=\"0x\" + up to 4 hex digits, not all \"F\""));
	SerialMon.println(F("panaddr or panaddr=\"0x\" + up to 4 hex digits, not all \"F\""));
	SerialMon.println();
}

/*-----------------------------------------------------------------------------------------------*/
/* There isn't a really good reason for this to be here. A separate sensors class should be used.*/
/* --although TMP275 is in the EP...                                                             */
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
SerialMon.print(F("TMP275 bits: "));
SerialMon.print(t, BIN); Serial.print("  0x"); Serial.println(t, HEX);
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