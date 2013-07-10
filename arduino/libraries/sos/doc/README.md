## explanations on the library

The library is in libraries/sos/.

See HowThisWorks-dependencyDiagram.dia to know the dependencies 
between the different classes.

For information on installing libraries, see: http://arduino.cc/en/Guide/Libraries

In the code and in this file I talk about « payload », there is two different 
payloads that we talk about : the payload in the ethernet point of view and the
CoAP one. The payload of CoAP is the payload of the ethernet without CoAP 
headers, token and options. Because there were some difficulties to know the 
exact length of the ethernet payload (and so the other one, which is linked),
we choose to add two bytes -payload length- before the ethernet payload.

The ethernet payload is : 
[mac sender | mac receiver | ethtype | paylen | payload]

## todo

* how to know if a message is acceptable ?
	* message id
	* token
	* type (after a CON => NON or ACK)

* complete the rmanager class
	* put ID & token like input msg in output msg
	* be able to send an ACK then do the job & send and CON msg when finished

* complete the Retransmit class
	* check if the main loop is complete
* complete the Sos class
	* do the code in other SOS states (Running & so)

* do tests on
	* timers in the retransmit class
	* recording packet to retransmit
	* switch between different states
	* timers (retransmit & so)

