#include "rmanager.h"

Rmanager::Rmanager() 
{ 
}

Rmanager::~Rmanager() 
{ 
	reset();
}

void Rmanager::add_resource(char *name, int namelen,
		uint8_t (*handler)(Message &in, Message &out)) 
{
	rmanager_s * newresource = (rmanager_s *) malloc(sizeof(rmanager_s));
	newresource->name = (char *) malloc(namelen +1);
	newresource->namelen = namelen;
	newresource->h = handler;

	memcpy(newresource->name, name, namelen);
	newresource->name[namelen] = '\0';

	newresource->s = _resources;
	_resources = newresource;
}

void Rmanager::delete_resource(rmanager_s *r) 
{
	if(r == NULL) {
		return;
	}
	else if(r == _resources) {
		_resources = _resources->s;
	}
	else {
		rmanager_s * tmp = _resources;
		while( tmp->s != r ) tmp = tmp->s;
		tmp->s = r->s;
	}
	free(r->name);
	free(r);
}

// TODO
uint8_t Rmanager::request_resource(Message &in, Message &out) 
{
	uint8_t res = 0;
	in.reset_get_option();
	/*
	   for(option * o = in.get_option() ; o != NULL ; o = in.get_option()) {
	   switch(o->optcode()) {
	   case option::MO_None			: Serial.println(F("MO_None")); break;
	   case option::MO_Content_Format	: Serial.println(F("MO_Content_Format")); break;
	   case option::MO_Etag			: Serial.println(F("MO_Etag")); break;
	   case option::MO_Location_Path	: Serial.println(F("MO_Location_Path")); break;
	   case option::MO_Location_Query	: Serial.println(F("MO_Location_Query")); break;
	   case option::MO_Max_Age			: Serial.println(F("MO_Max_Age")); break;
	   case option::MO_Proxy_Uri		: Serial.println(F("MO_Proxy_Uri")); break;
	   case option::MO_Proxy_Scheme	: Serial.println(F("MO_Proxy_Scheme")); break;
	   case option::MO_Uri_Host		: Serial.println(F("MO_Uri_Host")); break;
	   case option::MO_Uri_Path		: Serial.println(F("MO_Uri_Path")); break;
	   case option::MO_Uri_Port		: Serial.println(F("MO_Uri_Port")); break;
	   case option::MO_Uri_Query		: Serial.println(F("MO_Uri_Query")); break;
	   case option::MO_Accept			: Serial.println(F("MO_Accept")); break;
	   case option::MO_If_None_Match	: Serial.println(F("MO_If_None_Match")); break;
	   case option::MO_If_Match		: Serial.println(F("MO_If_Match")); break;
	   default :
	   Serial.print(F("\033[31mERROR\033[00m"));
	   Serial.print(o->optcode());
	   break;
	   }
	   }
	   exec_request(in, out);
	   */
	return res;
}

void Rmanager::get_all_resources(Message &out) 
{
	int limit = 150;
	char buf[limit];
	for(int i = 0 ; i < limit ; i++)
		buf[i] = '\0';
	int size = 0;
	// We need to send a message with a payload like : 
	// </temp>;title="the temp";rt="Temp",</light>;title="Luminosity";rt="light-lux"
	for(rmanager_s * tmp = _resources ; tmp != NULL ; tmp = tmp->s) 
	{
		int len;

		len = out.get_payload_length();
		if(len)
		{
			size = len + 1; // for the ','
			if(size < limit)
			{
				char *tmp = (char*) out.get_payload_copy();
				snprintf(buf, size +1, "%s,", tmp );
				free(tmp);
			}
		}

		len = tmp->namelen * 3; // * 3 because it's also title + rt here FIXME
		size += len + 18; // for the '</>;title="";rt=""'

		if(size < limit)
		{
			snprintf(buf, size +1, "%s</%s>;title=\"%s\";rt=\"%s\"",
					buf, tmp->name, tmp->name, tmp->name);
			out.set_payload(size, (uint8_t*)buf);
		}
	}
}

// TODO
// We have to know how a resource will be requested
rmanager_s * Rmanager::get_resource_instance(Message &in) 
{
	/*
	for(rmanager_s * tmp = _resources; tmp != NULL ; tmp = tmp->s)
	{
	}
	*/
}

uint8_t Rmanager::exec_request(Message &in, Message &out) 
{
	rmanager_s * tmp = get_resource_instance(in);
	if(tmp == NULL)
		return 1;
	return (*tmp->h)(in, out);
}

void Rmanager::reset() 
{
	while(_resources != NULL) {
		delete_resource(_resources);
	}
}
