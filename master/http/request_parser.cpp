//
// request_parser.cpp
// ~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_parser.hpp"
#include "request.hpp"

namespace http {
namespace server2 {

request_parser::request_parser()
  : state_(method_start)
{
}

void request_parser::reset()
{
  state_ = method_start;
}

boost::tribool request_parser::consume(request& req, char input, int remaining)
{
  switch (state_)
  {
  case method_start:
    if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      state_ = method;
      req.method.push_back(input);
      return boost::indeterminate;
    }
  case method:
    if (input == ' ')
    {
      state_ = uri;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req.method.push_back(input);
      return boost::indeterminate;
    }
  case uri_start:
    if (is_ctl(input))
    {
      return false;
    }
    else
    {
      state_ = uri;
      req.uri.push_back(input);
      return boost::indeterminate;
    }
  case uri:
    if (input == ' ')
    {
      state_ = http_version_h;
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      req.uri.push_back(input);
      return boost::indeterminate;
    }
  case http_version_h:
    if (input == 'H')
    {
      state_ = http_version_t_1;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_t_1:
    if (input == 'T')
    {
      state_ = http_version_t_2;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_t_2:
    if (input == 'T')
    {
      state_ = http_version_p;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_p:
    if (input == 'P')
    {
      state_ = http_version_slash;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_slash:
    if (input == '/')
    {
      req.http_version_major = 0;
      req.http_version_minor = 0;
      state_ = http_version_major_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_major_start:
    if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      state_ = http_version_major;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_major:
    if (input == '.')
    {
      state_ = http_version_minor_start;
      return boost::indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_major = req.http_version_major * 10 + input - '0';
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_minor_start:
    if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      state_ = http_version_minor;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case http_version_minor:
    if (input == '\r')
    {
      state_ = expecting_newline_1;
      return boost::indeterminate;
    }
    else if (is_digit(input))
    {
      req.http_version_minor = req.http_version_minor * 10 + input - '0';
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case expecting_newline_1:
    if (input == '\n')
    {
      state_ = header_line_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case header_line_start:
    if (input == '\r')
    {
      state_ = expecting_newline_3;
      return boost::indeterminate;
    }
    else if (!req.headers.empty() && (input == ' ' || input == '\t'))
    {
      state_ = header_lws;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req.headers.push_back(header());
      req.headers.back().name.push_back(input);
      state_ = header_name;
      return boost::indeterminate;
    }
  case header_lws:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return boost::indeterminate;
    }
    else if (input == ' ' || input == '\t')
    {
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      state_ = header_value;
      req.headers.back().value.push_back(input);
      return boost::indeterminate;
    }
  case header_name:
    if (input == ':')
    {
      state_ = space_before_header_value;
      return boost::indeterminate;
    }
    else if (!is_char(input) || is_ctl(input) || is_tspecial(input))
    {
      return false;
    }
    else
    {
      req.headers.back().name.push_back(input);
      return boost::indeterminate;
    }
  case space_before_header_value:
    if (input == ' ')
    {
      state_ = header_value;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case header_value:
    if (input == '\r')
    {
      state_ = expecting_newline_2;
      return boost::indeterminate;
    }
    else if (is_ctl(input))
    {
      return false;
    }
    else
    {
      req.headers.back().value.push_back(input);
      return boost::indeterminate;
    }
  case expecting_newline_2:
    if (input == '\n')
    {
      state_ = header_line_start;
      return boost::indeterminate;
    }
    else
    {
      return false;
    }
  case expecting_newline_3:
    if (input == '\n')
    {
	if (remaining > 0)
	{
	    state_ = expecting_post_args;
	    return boost::indeterminate;
	}
	return true ;
    }
    else return false ;
  case expecting_post_args:
    req.rawargs += input;
    if (remaining > 0)
	return boost::indeterminate;
    else
	return true ;
  default:
    return false;
  }
}

bool request_parser::is_char(int c)
{
  return c >= 0 && c <= 127;
}

bool request_parser::is_ctl(int c)
{
  return (c >= 0 && c <= 31) || c == 127;
}

bool request_parser::is_tspecial(int c)
{
  switch (c)
  {
  case '(': case ')': case '<': case '>': case '@':
  case ',': case ';': case ':': case '\\': case '"':
  case '/': case '[': case ']': case '?': case '=':
  case '{': case '}': case ' ': case '\t':
    return true;
  default:
    return false;
  }
}

bool request_parser::is_digit(int c)
{
  return c >= '0' && c <= '9';
}

int request_parser::hex_value (int c)
{
    int r = -1 ;
    if (c >= '0' && c <= '9')
	r = c - '0' ;
    if (c >= 'a' && c <= 'f')
	r = c - 'a' + 10 ;
    if (c >= 'A' && c <= 'F')
	r = c - 'A' + 10 ;
    return r ;
}

bool request_parser::query_decode (request &req)
{
    enum { undetermined, www_form_urlencoded } encoding  = undetermined ;
    bool r = true ;

    // Look for Content-Type header
    for (auto &h : req.headers)
    {
	if (h.name == "Content-Type")
	{
	    if (h.value == "application/x-www-form-urlencoded")
	    {
		encoding = www_form_urlencoded ;
		break ;
	    }
	}
    }

    if (encoding == www_form_urlencoded)
    {
	enum { name_start,
		name,
		name_pcent1,
		name_pcent2,
		value,
		value_pcent1,
		value_pcent2,
		error,
	    } state = name_start ;
	unsigned char pcentchar ;

	for (auto c : req.rawargs)
	{
	    int x ;			// current hex value

	    switch (state)
	    {
		case name_start:
		    req.postargs.push_back (post_arg ()) ;
		    if (c == '%')
		    {
			state = name_pcent1 ;
			pcentchar = 0 ;
		    }
		    else if (c == '=')
		    {
			state = error ;
		    }
		    else if (c == '&')
		    {
			state = error ;
		    }
		    else
		    {
			if (c == '+')
			    c = ' ' ;
			state = name ;
			req.postargs.back ().name.push_back (c) ;
		    }
		    break ;
		case name:
		    if (c == '%')
		    {
			state = name_pcent1 ;
			pcentchar = 0 ;
		    }
		    else if (c == '=')
		    {
			state = value ;
		    }
		    else if (c == '&')
		    {
			state = error ;
		    }
		    else
		    {
			if (c == '+')
			    c = ' ' ;
			req.postargs.back ().name.push_back (c) ;
		    }
		    break ;
		case name_pcent1:
		    x = hex_value (c) ;
		    if (x == -1)
			state = error ;
		    else
		    {
			pcentchar = pcentchar * 16 + x ;
			state = name_pcent2 ;
		    }
		    break ;
		case name_pcent2:
		    x = hex_value (c) ;
		    if (x == -1)
			state = error ;
		    else
		    {
			pcentchar = pcentchar * 16 + x ;
			req.postargs.back ().name.push_back (pcentchar) ;
			state = name ;
		    }
		    break ;
		case value:
		    if (c == '%')
		    {
			state = value_pcent1 ;
			pcentchar = 0 ;
		    }
		    else if (c == '&')
		    {
			state = name_start ;
		    }
		    else
		    {
			if (c == '+')
			    c = ' ' ;
			req.postargs.back ().value.push_back (c) ;
		    }
		    break ;
		case value_pcent1:
		    x = hex_value (c) ;
		    if (x == -1)
			state = error ;
		    else
		    {
			pcentchar = pcentchar * 16 + x ;
			state = value_pcent2 ;
		    }
		    break ;
		case value_pcent2:
		    x = hex_value (c) ;
		    if (x == -1)
			state = error ;
		    else
		    {
			pcentchar = pcentchar * 16 + x ;
			req.postargs.back ().value.push_back (pcentchar) ;
			state = value ;
		    }
		    break ;
		case error:
		    // silently skip remaining characters
		    break ;
		default:
		    state = error ;
		    break ;
	    }
	}
	if (state != value)
	    r = false ;
    }

    return r ;
}

} // namespace server2
} // namespace http
