## todo

* replace all addresses by l2addr --done
* create a macro for the coap return codes --done
* change handlers signatures : uint8_t handler(Message &in, Message &out); --done
* handle the options in the recv process (EthernetRaw) --done
* delete all pointers when we don''t need them and replace them by aliases -- done
* get the name of the resource (in the payload of the Message) -- done

* change the Coap class to get the options and the payload
* do the part in the l2addr class to compare l2addr_eth and byte *
* complete the Retransmit class
* complete the Sos class
