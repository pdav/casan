//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Modified by Pierre David from the Boost "http server2"
// example by Christopher M. Kohlhoff
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <boost/lexical_cast.hpp>
#include "http/mime_types.hpp"
#include "http/reply.hpp"
#include "http/request.hpp"
#include "http/server.hpp"

#include "conf.h"
#include "option.h"
#include "resource.h"
#include "slave.h"
#include "sos.h"
#include "master.h"

extern conf cf ;
extern master master ;

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

void request_handler::handle_request(const http::server2::request& req, http::server2::reply& rep)
{
  std::string request_path;

  // analyzes "%hh" and "+" in req.uri to get a usable path
  if (!url_decode(req.uri, request_path))
  {
    rep = http::server2::reply::stock_reply(http::server2::reply::bad_request);
    return;
  }

#if 0
{
  std::cout << "req.method=" << req.method << "\n" ;
  std::cout << "req.uri=" << req.uri
  		<< " => request_path=" << request_path << "\n" ;
  for (auto &h : req.headers)
  {
    std::cout << "header <" << h.name << "," << h.value << ">\n" ;
  }
  std::cout << "args=" << req.rawargs << "\n" ;
  for (auto &h : req.postargs)
  {
    std::cout << "arg= <" << h.name << "," << h.value << ">\n" ;
  }
}
#endif

  // Request path must be absolute and not contain "..".
  if (request_path.empty() || request_path[0] != '/'
      || request_path.find("..") != std::string::npos)
  {
    rep = http::server2::reply::stock_reply(http::server2::reply::bad_request);
    return;
  }

  // Request path must not end with a slash (i.e. is a directory)
  if (request_path[request_path.size() - 1] == '/')
  {
    rep = http::server2::reply::stock_reply(http::server2::reply::bad_request);
    return;
  }

  if (request_path == "/debug")
  {
      // Fill out the reply to be sent to the client.
      rep.status = http::server2::reply::ok;
      rep.content = "<html><body><ul>"
		"<li><a href=\"/debug/conf\">configuration</a>"
		"<li><a href=\"/debug/run\">running status</a>"
		"<li><a href=\"/debug/slave\">slave status</a>"
		"</ul></body></html>" ;

      rep.headers.resize(2);
      rep.headers[0].name = "Content-Length";
      rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
      rep.headers[1].name = "Content-Type";
      rep.headers[1].value = "text/html" ;
  }
  else if (request_path == "/debug/conf")
  {
      // Fill out the reply to be sent to the client.
      rep.status = http::server2::reply::ok;
      rep.content = "<html><body><pre>"
			+ cf.html_debug ()
			+ "</pre></body></html>" ;

      rep.headers.resize(2);
      rep.headers[0].name = "Content-Length";
      rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
      rep.headers[1].name = "Content-Type";
      rep.headers[1].value = "text/html" ;
  }
  else if (request_path == "/debug/run")
  {
      // Fill out the reply to be sent to the client.
      rep.status = http::server2::reply::ok;
      rep.content = "<html><body><pre>"
			+ master.html_debug ()
			+ "</pre></body></html>" ;

      rep.headers.resize(2);
      rep.headers[0].name = "Content-Length";
      rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
      rep.headers[1].name = "Content-Type";
      rep.headers[1].value = "text/html" ;
  }
  else if (request_path == "/debug/slave")
  {
      // check which slave is concerned
      if (req.method == "POST")
      {
        enum sos::slave::status_code newstatus = sos::slave::SL_INACTIVE ;
	slaveid_t sid = -1 ;

	for (auto &a : req.postargs)
	{
	    if (a.name == "slaveid")
		sid = std::stoi (a.value) ;
	    else if (a.name == "status")
	    {
		if (a.value == "inactive")
		    newstatus = sos::slave::SL_INACTIVE ;
		else if (a.value == "runnning")
		    newstatus = sos::slave::SL_RUNNING ;
	    }
	}
	if (sid != -1)
	{
	  bool r = true ;

	  // r = master.force_slave_status (sid, newstatus) ;
	  if (r)
	  {
	      // Fill out the reply to be sent to the client.
	      rep.status = http::server2::reply::ok;
	      rep.content = "<html><body><pre>"
				"ok (but not implemented)"
				"</pre></body></html>" ;
	  }
	  else
	  {
	      // Fill out the reply to be sent to the client.
	      rep.status = http::server2::reply::not_modified;
	      rep.content = "<html><body><pre>"
				"not modified, really"
				"</pre></body></html>" ;
	  }
	}
	else
	{
	  // Fill out the reply to be sent to the client.
	  rep.status = http::server2::reply::bad_request;
	  rep.content = "<html><body><pre>"
			    "so bad request..."
			    "</pre></body></html>" ;
	}
      }
      else
      {
	  // Fill out the reply to be sent to the client.
	  rep.status = http::server2::reply::ok;
	  rep.content = "<html><body>"
			    "<form method=\"post\" action=\"/debug/slave\">"
				"slave id <input type=text name=slaveid><p>"
				"status <select size=1 name=status>"
				    "<option value=\"inactive\">INACTIVE"
				    "<option value=\"running\">RUNNING"
				"</select>"
				"<input type=submit value=set>"
			    "</pre></body></html>" ;
      }

      rep.headers.resize(2);
      rep.headers[0].name = "Content-Length";
      rep.headers[0].value = boost::lexical_cast<std::string>(rep.content.size());
      rep.headers[1].name = "Content-Type";
      rep.headers[1].value = "text/html" ;
  }
  else
  {
    rep = http::server2::reply::stock_reply(http::server2::reply::not_found);
    return;
  }
}

bool request_handler::url_decode(const std::string& in, std::string& out)
{
  out.clear();
  out.reserve(in.size());
  for (std::size_t i = 0; i < in.size(); ++i)
  {
    if (in[i] == '%')
    {
      if (i + 3 <= in.size())
      {
        int value;
        std::istringstream is(in.substr(i + 1, 2));
        if (is >> std::hex >> value)
        {
          out += static_cast<char>(value);
          i += 2;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }
    }
    else if (in[i] == '+')
    {
      out += ' ';
    }
    else
    {
      out += in[i];
    }
  }
  return true;
}
