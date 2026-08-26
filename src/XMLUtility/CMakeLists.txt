set(proj "XMLUtility")

add_library(${proj}  SHARED
  DOMDocumentAdapter.cpp
  ParseErrorReporter.cpp
  StringManip.cpp
  Utility.cpp

)

target_link_libraries(${proj}
  PRIVATE
    XercesC::XercesC
)
