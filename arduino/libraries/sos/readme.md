## todo

* replace all addresses by l2addr --done
* create a macro for the coap return codes --done
* change handlers signatures : uint8_t handler(Message &in, Message &out); --done
* handle the options in the recv process (EthernetRaw) --done
* delete all pointers when we don''t need them and replace them by aliases -- done
* get the name of the resource (in the payload of the Message) -- done
* do the part in the l2addr class to compare l2addr_eth and byte * --done

* change the Coap class to
	* get the options length
	* get the options
	* get the options sorted
	* delete all options
	* push/pop the options
	* get the options
	* get the payload length
	* get the payload -- done

* change the Message class to
	* delete all the useless functions

* complete the Retransmit class
	* check if the main loop is complete
* complete the Rmanager class
	* add the return of the .well-known/ (list all resources)
* complete the Sos class
	* do the code in other SOS states (Running & so)

* do tests on
	* receiving ethernet message and print the length of the eth payload -- done
	* receiving hello message (with sendtest)
	* answer to hello message (with sendtest)

	* listing all resources that we record
	* timers in the retransmit class
	* recording packet to retransmit
	* switch between different states
	* timers (retransmit & so)
	* Message class -- done
	* Coap class
		* recv correctly the payload
		* recv correctly the message id
		* recv correctly the token
		* recv correctly the options


## some explanations
In the code and in this file I talk about « payload », there is two different 
payloads that we talk about : the payload in the ethernet point of view and the
CoAP one. The payload of CoAP is the payload of the ethernet without CoAP 
headers, token and options. Because there were some difficulties to know the 
exact length of the ethernet payload (and so the other one, which is linked),
we choose to add two bytes -payload length- before the ethernet payload.

So, the ethernet payload is : 
[mac sender | mac receiver | ethtype | paylen | payload]
