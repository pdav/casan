//
// request.hpp
// ~~~~~~~~~~~
//
// Copyright (c) 2003-2008 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef HTTP_SERVER2_REQUEST_HPP
#define HTTP_SERVER2_REQUEST_HPP

#include <string>
#include <vector>
#include "header.hpp"

namespace http {
namespace server2 {

struct post_arg
{
    std::string name ;
    std::string value ;
} ;

/// A request received from a client.
struct request
{
  std::string method;
  std::string uri;
  int http_version_major;
  int http_version_minor;
  std::vector<header> headers;
  std::string rawargs;				// raw encoded POST query
  std::vector <post_arg> postargs;		// decoded POST query
};

} // namespace server2
} // namespace http

#endif // HTTP_SERVER2_REQUEST_HPP
