This sketch uses the ChariotEPLib and the Chariot Web-of-Things shield for
Arduino. It uses Chariot's builtin high-precision TI TMP273 temp sensor,
residing on the Chariot's I2C bus, which is shared with the Arduino to show how
triggerable events may be easily constructed and delivered to subscribers
everywhere.
 
The temp sensor is available to the outside world at /sensors/tmp275-c, using
the MDNS name of your board. For example, if you were using a Chariot owning the
Serial# c350e, you could issue a RESTful GET to retrieve its current reading
simply by sending this Chariot the URL: 

> coap://chariot.c350e.local/sensors/tmp275-c/?get
	
In this URL, "coap:" is the internet-of-things analogue to "http:". In addition
to this, Chariot can also perform a great number of other RESTful operations,
and even enable your sketch to do new and exciting ones. For more on this, check
the top level documentation of this library.

The sketch establishes its connection to the web-of-things with the line of code
in setup():
```c++
	//---Put Arduino resources on the air---
	ChariotEP.begin();
```
		
This makes all of Arduino's pin resources available remotely for reading,
setting and publishing. The sketch then creates a trigger resource, making it
possible to publish events concerning the temp sensor to all subscribers. This
is accomplished by this library by the following code:

```c++
	String trigger = "event/tmp275-c/trigger";
	String attr = "title=\"Trigger\?get|obs|put\"";
	
	eventHandle = ChariotEP.createResource(trigger, 63, attr);
```

The trigger is now available for GET, PUT and OBS(erve) service at (given
example address):

> coap://chariot.c350e.local/event/tmp275-c/trigger
	
The final setup step is to register a PUT handler. Line 110 of the example
sketch does this:

> `ChariotEP.setPutHandler(eventHandle, triggerPutCallback);`

This is very useful for sending parameters and commands to the event trigger.
PUT URLs are constructed as name/value pairs. In this example, the following
pairs are supported:

> coap://chariot.c350e.local/event/tmp275-c/trigger?put&param=triggerval&val=30
	
This example would result in the invocation of the PUT handler as:

> `triggerPutCallback("triggerval=30");`
	
Finally, when the event trigger condition is met, the example sends a message
that is received by all that have subscribed to the notification previously vin
the url:

> coap://chariot.c350e.local/event/tmp275-c/trigger?obs
	
The trigger message constructed by the example is coded as (line 89):

> `String triggeredVal = "{\"ID\":\"Trigger\",\"Triggered\":\"Yes\",\"State\":\"Off\"}";`
	
The Chariot EP library handles the delivery via the call (line 90):

> `ChariotEP.triggerResourceEvent(eventHandle, triggeredVal, true);`

	

> Qualia Networks Incorporated -- Chariot IoT Shield and software for Arduino              
> Copyright, Qualia Networks, Inc., 2016.
