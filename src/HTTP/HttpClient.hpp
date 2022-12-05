#ifndef HTTP_HTTPCLIENT_HPP
#define HTTP_HTTPCLIENT_HPP

#include <Sync/MutexPtr.hpp>

#include <HTTP/HTTPCookie.hpp>
#include <HTTP/HttpAsync.hpp>


namespace HTTP
{
  typedef Sync::MutexRefPtr<HTTP::ClientCookieFacility>
    CookiePoolPtr;
  typedef ReferenceCounting::QualPtr<CookiePoolPtr> CookiePool_var;

  HttpInterface*
  CreateCookieClient(HttpInterface* pool, CookiePoolPtr* cookie)
    /*throw (eh::Exception)*/;
}

#endif
