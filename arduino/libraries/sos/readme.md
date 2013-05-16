## todo

* replace all addresses by l2addr --done
* create a macro for the coap return codes --done
* change handlers signatures : uint8_t handler(Message &in, Message &out); --done
* handle the options in the recv process (EthernetRaw) --done
* delete all pointers when we don''t need them and replace them by aliases -- done
* get the name of the resource (in the payload of the Message) -- done
* do the part in the l2addr class to compare l2addr_eth and byte * --done

* change the Coap class to get 
	* the payload -- done
	* the options 
* complete the Retransmit class
	* do timers
* complete the Rmanager class
	* add the return of the .well-known/ (list all resources)
* complete the Sos class

* do tests on
	* recording packet to retransmit
	* receiving hello message
	* switch between different states
	* timers (retransmit & so)
	* listing all resources that we record
