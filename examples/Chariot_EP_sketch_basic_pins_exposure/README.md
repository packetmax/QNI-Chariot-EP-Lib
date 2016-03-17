This sketch uses the ChariotEPLib and the Chariot Web-of-Things shield for
Arduino. It shows how to make all of Arduino's pins globally available in just a
couple of lines of code. Examples of digital and analog pin accesses available
to remote web applications or browsers is:

|Remote webapp:                                              |Resulting action by ChariotEPLib    |
|------------------------------------------------------------|------------------------------------|
|coap://chariot.c350e.local/arduino/mode?put&pin=13&val=input|`pinMode(13, INPUT)`                |
|coap://chariot.c350e.local/arduino/digital?put&pin=13&val=0 |`digitalWrite(13, LOW)`             |
|coap://chariot.c350e.local/arduino/digital?get&pin=13       |`digitalRead(13)`                   |
|coap://chariot.c350e.local/arduino/analog?get&pin=5         |`analogRead(5)`                     |
|coap://chariot.c350e.local/arduino/analog?put&pin=13&val=128|`analogWrite(2, 123)  //set PWM duty cycle`|

	
In this URL format, "coap:" is the internet-of-things analogue to "http:".
Chariot can also perform a great number of other RESTful operations, and even
enable your sketch to do new and exciting applications. For more on this, check
the top level documentation of this library.

The sketch establishes its connection to the web-of-things with the line of code
in setup():

```c++
	//---Put Arduino resources on the air---
	ChariotEP.begin();
```

The restful calls come to Arduino through the ChariotEPLib API and will be
processed autmatically for you in this example sketch via the following lines of
code (see 39-42):

```c++
	//---answer RESTful API calls automatically---
	if (ChariotEP.available()) {
		ChariotEP.process();
	}
```

You are free to do some or all of this processing in your sketch if needed. See
our library code (ChariotEPLib.cpp) find out how it's done.


> Qualia Networks Incorporated -- Chariot IoT Shield and software for Arduino              
> Copyright, Qualia Networks, Inc., 2016.	
