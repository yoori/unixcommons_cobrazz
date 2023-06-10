set (proj "HTTP")

add_library(${proj} SHARED
  Http.cpp
  HttpAsync.cpp
  HttpAsyncPool.cpp
  HttpAsyncPolicies.cpp
  HttpClient.cpp
  HttpConnection.cpp
  HTTPCookie.cpp
  HttpSync.cpp
  UrlAddress.cpp
)

target_link_libraries(${proj}
  Generics
  Stream
  event)
