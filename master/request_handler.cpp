/**
 * @file request_handler.cpp
 * @brief request_handler class implementation
 *
 * Note: this file is a modified version of the equivalent
 * file from the ASIO example "http server2".
 * See source file for license and credits.
 */

//
// request_handler.cpp
// ~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2014 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include "request_handler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <list>
#include <boost/lexical_cast.hpp>
#include "http/mime_types.hpp"
#include "http/reply.hpp"
#include "http/request.hpp"
#include "http/server.hpp"

#include "conf.h"
#include "option.h"
#include "resource.h"
#include "slave.h"
#include "casan.h"
#include "master.h"

extern conf cf ;
extern master master ;

/**
 * @brief Request handler creation
 *
 * This modified version is reduced from the original, but interface
 * (the `doc_root` unused parameter) did not change in order to keep the
 * rest of the HTTP server unmodified.
 */

request_handler::request_handler(const std::string& doc_root)
  : doc_root_(doc_root)
{
}

/**
 * @brief Handles an HTTP request
 *
 * This method directly handles badly formatted requests, and if
 * not, gives control to the master::handle_http method.
 *
 * @param req the request HTTP message
 * @param rep our reply
 */

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

  master.handle_http (request_path, req, rep) ;
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
