/**
 * @file request_handler.hpp
 * @brief request_handler class interface
 *
 * Note: this file is a modified version of the equivalent
 * file from the Boost "http server2".
 * See source file for license and credits.
 */

//
// request_handler.hpp
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

#ifndef REQUEST_HANDLER_HPP
#define REQUEST_HANDLER_HPP

#include <string>
#include <boost/noncopyable.hpp>

namespace http {
namespace server2 {
struct reply;
struct request;
}
}

/// The common handler for all incoming requests.
class request_handler
  : private boost::noncopyable
{
public:
  /// Construct with a directory containing files to be served.
  explicit request_handler(const std::string& doc_root);

  /// Handle a request and produce a reply.
  void handle_request(const http::server2::request& req, http::server2::reply& rep);

private:
  /// The directory containing the files to be served.
  std::string doc_root_;

  /// Perform URL-decoding on a string. Returns false if the encoding was
  /// invalid.
  static bool url_decode(const std::string& in, std::string& out);
};

#endif // REQUEST_HANDLER_HPP
