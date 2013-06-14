For information on installing libraries, see: http://arduino.cc/en/Guide/Libraries

## todo

* how to know if a message is acceptable ?
	* message id
	* token
	* type (after a CON => NON or ACK)

* complete the Retransmit class
	* check if the main loop is complete
* complete the Sos class
	* do the code in other SOS states (Running & so)

* do tests on
	* timers in the retransmit class
	* recording packet to retransmit
	* switch between different states
	* timers (retransmit & so)


## some explanations
In the code and in this file I talk about « payload », there is two different 
payloads that we talk about : the payload in the ethernet point of view and the
CoAP one. The payload of CoAP is the payload of the ethernet without CoAP 
headers, token and options. Because there were some difficulties to know the 
exact length of the ethernet payload (and so the other one, which is linked),
we choose to add two bytes -payload length- before the ethernet payload.

So, the ethernet payload is : 
[mac sender | mac receiver | ethtype | paylen | payload]
