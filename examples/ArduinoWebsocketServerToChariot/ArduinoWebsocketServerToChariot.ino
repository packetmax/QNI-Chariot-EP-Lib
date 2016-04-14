#include <ChariotEPLib.h>

/*
  Runs Chariot CoAP Service backend on Ethernet Shield + websocket server. 
  
  Context:  Arduino Mega 2560--Chariot-to-Mega serial arrangement.

  This sketch shows how to run Qualia's Chariot CoAP service over a websocket server 
  backend.
   
  It passes arriving Arduino and Chariot(CoAP) commands from a websocket frontend 
  through the service backend across Arduino's serial port to Chariot. Any 
  response strings from Chariot or command input from the Serial Monitor are conveyed to the backend for
  processing. Some modifications have been made to "Websocket-Arduino, a simple websocket implementation
  for Arduino" (see its source code for copyright notices):
    1.) Frames are limited to 125B. Data sent to websocket will be so-segmented.
    2.) In the WebsocketServer constructor, the port number used is 1337. This is also used by default in 
        our websocket frontend. 
        
  by George Wayne, Qualia Networks Incorporated
  
 */
 
#include <String.h>
#include <Base64.h>
#include <sha1.h>
#include <WebSocket.h>
#include <SPI.h>
#include <Ethernet.h>

#define STR_TERM_CHAR '\0'
#define SerialMon if(debug)Serial

// If using the Serial port--type an integer within 5 secs to activate.
static bool debug = false;

#define MAX_CHARIOT_CMD_LEN   (128-1)
#define MAX_CHARIOT_IO_MILLIS (500)
#define MAX_CONCURRENT_CHARIOT_MSGS (4)

void coapResponse(WebSocket &socket);
// Enable websocket debug tracing to Serial port. See Websocket.h.
#define DEBUG

// Here we define a maximum framelength to 125 bytes--see RFC 
#define MAX_DATA_FRAME_LENGTH 125

#include <WebSocket.h>

// Hardwired as required by Websocket Library 
byte mac[] = { 0x52, 0x4F, 0x43, 0x4B, 0x45, 0x54 };
byte ip[] = { 192, 168, 0 , 77 };

// Create a Websocket server and sockets
WebSocketServer wsServer;
#define MAX_WEB_SOCKETS (4) // See <websocket.h>
WebSocket *openSockets[MAX_WEB_SOCKETS]  = {NULL};

void onConnect(WebSocket &socket) {
  WebSocket *ws_p = (WebSocket *)&socket;

  int i;
  for (i = 0; i < MAX_WEB_SOCKETS-1; i++) {
    if (ws_p == openSockets[i]) {
      SerialMon.println(F("ERROR! Websocket connection cannot be opened--no available slots."));
      return; // we already have this one cached (ERR)
    }
  }

  // Plug socket into first open slot in connection table.
  for (i = 0; i < MAX_WEB_SOCKETS-1; i++) {
    if (openSockets[i] == NULL) {
      openSockets[i] = ws_p;
      SerialMon.print(F("Websocket connection opened = "));
      SerialMon.println(i);
      return;
    }
  }
}

/*   
`* Called by the server when a data frame is received.
 */
void onData(WebSocket &socket, char* dataString, byte frameLength) 
{
  /*
   * Candidate URL or command received over the websocket server.
   */
  String chariotUrlOrCmd = String(dataString);
     
  /**
   * End of chariotUrlOrCmd line input reached--trim whitespace and nul terminate
   */
  chariotUrlOrCmd.trim();
  SerialMon.print(F("chariot command to be sent to Chariot(len= "));
  SerialMon.print(frameLength); //chariotUrlOrCmd.length());
  SerialMon.print(F("): "));
  SerialMon.println(chariotUrlOrCmd);
  if (chariotUrlOrCmd.length() >= MAX_CHARIOT_CMD_LEN) {
    chariotUrlOrCmd.setCharAt(MAX_CHARIOT_CMD_LEN, '\0');
  }

  // Simplify all comparisons, make all input lower case.
  chariotUrlOrCmd.toLowerCase();
  
 /**
  * process coap:// and coap:// URI's here
  */
  if (chariotUrlOrCmd.indexOf(F("coap")) != -1) {
    chariotUrlOrCmd += "\n\0";    // terminate for Chariot
    ChariotClient.print(chariotUrlOrCmd);
    coapResponse(socket);
  }
    
 /**
  * process local Chariot command URI's here
  */
  else if (chariotUrlOrCmd.indexOf(F("chariot")) != -1) {
    chariotUrlOrCmd.remove(0, 8); // remove "chariot/"
    chariotUrlOrCmd += "\n\0";    // terminate for Chariot

    ChariotClient.print(chariotUrlOrCmd);
    coapResponse(socket);
  }

  /**
   * process local arduino firmata cmds here
   */
  else if (chariotUrlOrCmd.indexOf(F("arduino/"))!= -1) {
     chariotUrlOrCmd.remove(0, 8);
     processArduinoCmd(socket, chariotUrlOrCmd);
  }

}

void onDisconnect(WebSocket &socket) {
  // Find and remove socket in connection table.
  WebSocket *ws_p = &socket;

  int i;
  for (i = 0; i < MAX_WEB_SOCKETS-1; i++) {
    if (openSockets[i] == ws_p) {
      openSockets[i] = NULL;
      SerialMon.print(F("Websocket connection closed = "));
      SerialMon.println(i);
      return;
    }
  }
}

void setup() 
{
 // Check for availability of arduino IDE/Tools/Port for console and debug:
  Serial.begin(9600);
  while (!Serial.available() &&  millis() < 5000) ;
  if (Serial.available()) {
    int startupInt = Serial.read(); // Enter single char for Serial startup.
  
    Serial.println(F("Serial port is active"));
    debug = true;
    ChariotEP.enableDebugMsgs();
  } else {
    ChariotEP.disableDebugMsgs();
  }

  //---Put Arduino resources on the air---
  ChariotEP.begin();

  //---Put Ethernet on the air (so to speak)---
  Ethernet.begin(mac, ip);
  
  wsServer.registerConnectCallback(&onConnect);
  wsServer.registerDataCallback(&onData);
  wsServer.registerDisconnectCallback(&onDisconnect);  
  wsServer.begin();
  
  delay(100); // Give Ethernet time to get ready
  SerialMon.println(F("Setup complete."));
}

void loop() {
  int more, msgNdx;
  String response[MAX_CONCURRENT_CHARIOT_MSGS];   
  
  /*
   * asynchronous response(s) arrived?
   */
   more = ChariotClient.available();
   msgNdx = 0;
   while (more && (msgNdx < MAX_CONCURRENT_CHARIOT_MSGS-1)) {
      more = ChariotEP.coapResponseGet(response[msgNdx++]);
   }

  if (msgNdx) {
    int x;
    for (x = 0; x < msgNdx; x++)
    {
      if (wsServer.connectionCount() > 0) {
        int i;
        WebSocket *ws_p;
        char *responseStr = (char *)response[x].c_str();
        
        for (i = 0; i < MAX_WEB_SOCKETS-1; i++) {
          if (ws_p = openSockets[i]) {
            ws_p->send(responseStr, response[x].length());
          }
        }
      }
      else {
        SerialMon.println(response[x]);
      }
    }
  }
  
  /* 
   *  Filter your own inputs first--pass everthing else here.
   *   --try typing 'sys/help' into the Serial window
   */
  if (debug && Serial.available()) {
    ChariotEP.serialChariotCmd();
  }
  
  // Should be called for each loop.
  wsServer.listen();
  delay(50);
}


/**
 * Handle requests
 */
void processArduinoCmd(WebSocket &socket, String command) {  
  int pin = -1, value = -1;
  String errStr = command;
  String response = "";
  char *response_ptr;
  
  /**
   * digitalCommand(client);
   */
  if (command.indexOf(F("digital")) != -1) {
    command.remove(0, sizeof("digital/")-1);
    
    if (true == ChariotEP.pinValParse(command, &pin, &value)) {
      /**
       * Sanity check digital pin and value.
       *  Note: digital pins in use by SoftwareSerial port
       *        are unavailable for farbling.
       */
      if ((pin < 0) || (pin > 53)) { // Mega 2560-specific
        errStr = "invalid digital pin: " + String(pin);
        goto error;
      }
      if ((value != -1) && ((value < 0) || (value > 1))) {
        errStr = "invalid digital value: " + String(value);
        goto error;
      }
      
      if (value != -1) {
        digitalWrite(pin, value);
      }
      else {
          value = digitalRead(pin);
      }
      
      // Send feedback to requestor
      response = String("Pin D") + pin + String(" set to ") + value + "\n";
      response_ptr = (char *)response.c_str();
      socket.send(response_ptr, response.length());
    } else {
       goto error;
    }
    return;
  }

  /**
   * is "analog" command?
   */
  else if (command.indexOf("analog") != -1) {
    command.remove(0, sizeof("analog/")-1);
    
    if (true == ChariotEP.pinValParse(command, &pin, &value)) {
      /**
       * Sanity check analog pin and value 
       */
      if ((pin < 0) || (pin > 15)) { // Mega 2560-specific
        errStr = "invalid analog pin: " + String(pin);
        goto error;
      }
      if ((value != -1) && ((value < 0) || (value > 1023))) {
        errStr = "invalid analog value: " + String(value);
        goto error;
      }
      
      if (value != -1) {
        analogWrite(pin, value);
               
        // Send feedback to requestor
        response = String("Pin A") + pin + String(" set to analog ") + value + String("\n");
        response_ptr = (char *)response.c_str();
        socket.send(response_ptr, response.length());
      }
      else {
        value = analogRead(pin);
              
        // Send feedback to requestor
        response = String("Pin A") + pin + String(" reads analog ") + value + String("\n");
        response_ptr = (char *)response.c_str();
        socket.send(response_ptr, response.length());
      }
    } else {
       goto error;
    }
    return;
  }

  /**
   * is "mode" command?
   */
  else if (command.indexOf("mode") != -1) {
    int mode_ndx;
    
    command.remove(0, sizeof("mode/")-1);
Serial.println("Mode command processing started");
 #ifdef NOT_NEEDED  
    if ((mode_ndx = command.indexOf("input_pullup/")) != -1) {
      mode_val = INPUT_PULLUP;
      command.remove(0, sizeof("input_pullup/"));
    } else if ((mode_ndx = command.indexOf("input/")) != -1) {
      mode_val = INPUT;
      command.remove(0, sizeof("input/"));
    } else if ((mode_ndx = command.indexOf("output/")) != -1) {
      mode_val = OUTPUT;
      command.remove(0, sizeof("output/"));
    } else {
     errStr = "unknown mode ";
     goto error;
    }
#endif
    // Parse mode and pin--set value.   
Serial.print("string: "); Serial.print(command); Serial.println(" sent to pinValParse");
    if (true == ChariotEP.pinValParse(command, &pin, &value)) {
Serial.print("pinValParse for Mode returned: "); Serial.print(value);
Serial.print(" for pin: "); Serial.println(pin);
      if (value != -1) {
        pinMode(pin, value);
        response = String("pin D") + pin + String(" configured as ");
        switch (value) {
          case INPUT:
            response += "INPUT\n";
            break;
          case INPUT_PULLUP:
            response += "INPUT_PULLUP\n";
            break;
          case OUTPUT:
            response += "OUTPUT\n";
            break;
          default:
            response += String(value);
            response += "\n";
        }
        response_ptr = (char *)response.c_str();
        socket.send(response_ptr, response.length());
      }
    } else {
Serial.print("Mode command returned: "); Serial.print(value);
Serial.print(" for pin: "); Serial.println(pin);
      errStr = "invalid mode requested";
      goto error;
    }
    return;
  }

  error:
    response = "4.00 Bad Request: " + errStr +"\n";
Serial.println(response);
    response_ptr = (char *)response.c_str();
    socket.send(response_ptr, response.length());
    return;
}

/*
 * Process Chariot local and coap[s]:// responses
 */
void coapResponse(WebSocket &socket) 
{
  String response = "";
  uint32_t started, len = 0;

  started = millis();
  ChariotEP.coapResponseGet(response);
  
  if (response.length() > 0) {
    char *response_ptr = (char *)response.c_str();
    String responseTime = String(millis()-started) + "(ms)\n";

    if (response.length() > MAX_DATA_FRAME_LENGTH) {
      int len = response.length();
      int next = MAX_DATA_FRAME_LENGTH;
       
     while (next > 0) {
        socket.send(response_ptr, next);
        SerialMon.print(F("Sending "));
        SerialMon.print(next);
        SerialMon.print(F(" bytes at "));
        SerialMon.print((int)response_ptr, HEX);
        SerialMon.println();
        len -= next;
        response_ptr += next;
        next = min(len, MAX_DATA_FRAME_LENGTH);
        if (len > 0) {
          delay(100);
        }
      }
    } else {
      socket.send(response_ptr, response.length());
    }
    
    response_ptr = (char *)responseTime.c_str();
    socket.send(response_ptr, responseTime.length());
  }
}
