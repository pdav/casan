#include "rmanager.h"

Rmanager::Rmanager() 
{ 
}

Rmanager::~Rmanager() 
{ 
	reset();
}

void Rmanager::add_resource(Resource *r) 
{
	rmanager_s * newresource = (rmanager_s *) malloc(sizeof(rmanager_s));
	newresource->s = _resources;
	newresource->r = r;
	_resources = newresource;
}



// returns  0 if ok 
uint8_t Rmanager::request_resource(Message &in, Message &out) 
{
	in.reset_get_option();

	for(option * o = in.get_option() ; o != NULL ; o = in.get_option())
	{

		if (o->optcode() == option::MO_Uri_Path)
		{
			// if the request is to get all resources
			if (o->optlen() >= (sizeof SOS_RESOURCES_ALL -1) && 
					strncmp( (const char *) o->val(), 
						SOS_RESOURCES_ALL, 
						(sizeof SOS_RESOURCES_ALL -1)) == 0)
			{
				out.set_type(COAP_TYPE_ACK);
				out.set_id(in.get_id());
				out.set_token(in.get_token_length(), in.get_token());
				out.set_code(COAP_RETURN_CODE(2,5));
				get_all_resources(out);
				return 0;

			}
			else
			{

				rmanager_s * tmp = get_resource_instance(o);
				if(tmp == NULL)
					return 1;

				out.set_type(COAP_TYPE_ACK);
				out.set_id(in.get_id());
				out.set_token(in.get_token_length(), in.get_token());
				
				
				Resource *r = tmp->r;
				handler_s h = r->get_handler((coap_code_t) in.get_code());
				uint8_t ret = (*h.handler)( in, out );
				
				
				//uint8_t ret = (*tmp->r->get_handler(in.get_code()))(in, out);
				
				bool found = false;
				for(option * o2 = out.get_option() ; o2 != NULL ; o2 = out.get_option()) {
					
					if (o2->optcode() == option::MO_Content_Format) {
						found = true;
						break;
					}
				}
				
				if(!found) {
					option o3 ;

					o3.optcode (option::MO_Content_Format) ;
					o3.optval (option::cf_text_plain) ;
					out.push_option (o3) ;
				}

				return ret;
			}
		}

	}

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
	return 1;
}

void Rmanager::get_all_resources(Message &out) 
{
	int limit = 150;
	char buf[limit];
	int size = 0;

	for(int i = 0 ; i < limit ; i++)
		buf[i] = '\0';

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

		len = strlen(tmp->r->_name) + strlen(tmp->r->_title) + strlen(tmp->r->_rt); 
		size += len + 17; // for the '<>;title="";rt=""'

		if(size < limit)
		{
			snprintf(buf, size +1, "%s<%s>;title=\"%s\";rt=\"%s\"",
					buf, tmp->r->_name, tmp->r->_title, tmp->r->_rt);
			out.set_payload(size, (uint8_t*)buf);
		}
	}
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
	delete r->r;
	free(r);
}

void Rmanager::reset() 
{
	while(_resources != NULL) {
		delete_resource(_resources);
	}
}

// We have to know how a resource will be requested
rmanager_s * Rmanager::get_resource_instance(option *o) 
{

	for(rmanager_s * tmp = _resources; tmp != NULL ; tmp = tmp->s)
	{
		if(tmp->r->check_name((char *)o->val(), o->optlen())) {
			return tmp;
		}
	}

	return NULL;
}

void Rmanager::print(void)
{
	char buf[150];
	for(rmanager_s * tmp = _resources; tmp != NULL ; tmp = tmp->s)
	{
		Serial.print(F("resource name : "));
		memcpy(buf, tmp->r->_name, strlen(tmp->r->_name)+1);
		Serial.println(buf);

		Serial.print(F("resource title : "));
		memcpy(buf, tmp->r->_title, strlen(tmp->r->_title)+1);
		Serial.println(buf);

		Serial.print(F("resource rt : "));
		memcpy(buf, tmp->r->_rt, strlen(tmp->r->_rt)+1);
		Serial.println(buf);
	}
}
