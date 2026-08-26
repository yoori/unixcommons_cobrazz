set(proj "PrivacyFilter")

add_library(${proj}  SHARED
    Filter.cpp
)

target_link_libraries(${proj}
  PRIVATE
    OpenSSL::Crypto
)
