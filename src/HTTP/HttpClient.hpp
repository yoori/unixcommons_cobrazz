#pragma once

#include <Sync/MutexPtr.hpp>

#include <HTTP/HTTPCookie.hpp>
#include <HTTP/HttpAsync.hpp>


namespace HTTP
{
  using CookiePoolPtr = Sync::MutexRefPtr<HTTP::ClientCookieFacility>;
  using CookiePool_var = ReferenceCounting::QualPtr<CookiePoolPtr>;

  HttpInterface* CreateCookieClient(HttpInterface* pool, CookiePoolPtr* cookie)
    /*throw (eh::Exception)*/;
}
