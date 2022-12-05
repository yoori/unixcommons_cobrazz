#ifndef HTTP_HTTPSYNC_HPP
#define HTTP_HTTPSYNC_HPP

#include <vector>

#include <HTTP/HttpAsync.hpp>


namespace HTTP
{
  typedef std::list<std::string> ExpectedHeaders;
  typedef std::vector<char> ResponseBody;

  /**
   * Function performs synchronous get request using
   * asynchronous http interface.
   * Result of request is returned either by assigns to passed references
   * or by exception
   * @param response_code response code
   * @param response_headers response headers listed in expected_headers
   * (if exist)
   * @param response_body response body
   * @param response_error received error (if any)
   * @param http reference to http interface to use for the request
   * @param http_request request URI
   * @param peer http server address
   * @param headers list of additional headers for request
   */
  void
  syncronous_get_request(
    int& response_code,
    HeaderList& response_headers,
    ResponseBody& response_body,
    std::string& response_error,

    HttpInterface& http,
    const char* http_request,
    const HttpServer& peer = HttpServer(),
    const HeaderList& headers = HeaderList())
    /*throw (eh::Exception, eh::DescriptiveException)*/;

  /**
   * Function performs synchronous post request using
   * asynchronous http interface.
   * Result of request is returned either by assigns to passed references
   * or by exception
   * @param response_code response code
   * @param response_headers response headers listed in expected_headers
   * (if exist)
   * @param response_body response body
   * @param response_error received error (if any)
   * @param http reference to http interface to use for the request
   * @param http_request request URI
   * @param body request data to post
   * @param peer http server address
   * @param headers list of additional headers for request
   */
  void
  syncronous_post_request(
    int& response_code,
    HeaderList& response_headers,
    ResponseBody& response_body,
    std::string& response_error,

    HttpInterface& http,
    const char* http_request,
    const String::SubString& body = String::SubString(),
    const HttpServer& peer = HttpServer(),
    const HeaderList& headers = HeaderList())
    /*throw (eh::Exception, eh::DescriptiveException)*/;
}

#endif
