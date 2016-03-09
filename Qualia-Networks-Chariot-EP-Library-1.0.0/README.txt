Chariot End Point Library for Arduino by Qualia Networks Incorporated (qualianetworks.com)

ChariotEPClass

Description

The ChariotEPClass class allows you to use and manipulate Arduino resources in new, more interesting and complex ways than other web technologies, such as found in Yun and Ethernet Shield. 

This library instantiates the object of this class for you. Its object is named "ChariotEP" (Chariot End Point). Also, upon detecting the Serial configuration of your Arduino, the communication channel named ChariotClient is derived from either Serial or SoftawareSerial class. For LEONARDO and UNO class boards, RX and TX pins must be jumpered to the Chariot. For MEGA_DUE class boards, Serial3 is jumpered. In both cases jumpering is to the Chariot's port labelled "UART1". See instructions that came with your Chariot.

This library provides simple, powerful web-of-things capabilities to any Arduino equipped with Qualia Networks' Chariot wireless shield. The public functions of the library turn your Arduino into a mote (i.e., 'particle') extending atomic URI-based access of the resources on your board to webapps or browsers running anywhere. 

Additionally this library provides Arduino with the ability to create and control dynamic event driven resources within a sketch. These resources become part of Chariot's RESTful web services, identified by unique URIs, which provide global addressing space enabling discovery and access.

Remote webapp:                                                   Resulting action by ChariotEPLib
--------------------------------------------------------------------------------------------------
coap://chariot.c350e.local/arduino/mode?put&pin=13&val=input    -> pinMode(13, INPUT)
coap://chariot.c350e.local/arduino/digital?put&pin=13&val=0     -> digitalWrite(13, LOW)
coap://chariot.c350e.local/arduino/digital?get&pin=13           -> digitalRead(13)           
coap://chariot.c350e.local/arduino/analog?get&pin=5             -> analogRead(5)
coap://chariot.c350e.local/arduino/analog?put&pin=13&val=128    -> analogWrite(2, 123) //set PWM duty cycle
	
In this URL format, "coap:" is the internet-of-things analogue to "http:". In addition to this, Chariot can also perform a great number of other RESTful operations, and even enable your sketch to do new and exciting applications. For more on this, check the top level documentation of this library.

ChariotEPLib class Functions
----------------------------

ChariotEPClass() - Constructs an instance of the ChariotEPClass class.

begin() - This method sets up the communication channel (named ChariotClient) to the Chariot Shield based on the Arduino model type. It sleeps on digital pin 8, waiting for Chariot to set it HIGH, indicating its availability and reads Chariot's inital status message. The last step is to read the Chariot temperature sensor, reporting it on the Serial window.
 
available() - gets the number of bytes (characters) available for reading from the ChariotClient serial port. This is data that's already arrived and stored in the receive buffer. See also process.

process() - provides processing of arriving RESTful function requests (GET/PUT/POST/OBS) for resources   such as processor pins, Chariot TMP275 temp sensor, FXOS8700cq 6-axis accelerometer, and all sensors and actuator resources created by sketches. 
	
createResource() - dynamic resource constuctor that assigns URI and Attributes to any resource controlled by your sketch.

triggerResourceEvent() - cause your triggered resource event to be published to all subscribers who are listening on your URI, such as here (assume your Chariot SN# c350e):
	coap://chariot.c350e.local/event-resource-name/trigger?obs

setPutHandler() - give the sketch access to data provided by RESTful remote PUT calls to the dynamic resource. For example:
	coap://chariot.c350e.local/event-resource-name/trigger?put&param=triggertemp&val=33
will cause your put handler to be invoked with the string "triggertemp=33

readTMP275() - get the current temperature from the Chariot onboard TMP275 sensor. It may be requested as FAHRENHEIT or CELSIUS. It is returned as a float type.

getArduinoModel() - returns a constant of type LEONARDO, UNO, or MEGA_DUE based on the hardware serial configuration detected at compile time.

Qualia Networks Incorporated -- Chariot IoT Shield and software for Arduino              
Copyright, Qualia Networks, Inc., 2016.	



