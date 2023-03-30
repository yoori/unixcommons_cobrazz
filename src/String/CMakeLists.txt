set(proj "String")

add_library(${proj}  SHARED

 __dummmy.cpp
#    Analyzer.cpp
#  AsciiStringManip.cpp
#  BasicAnalyzer.cpp
#  InterConvertion.cpp
#  RegEx.cpp
#  StringManip.cpp
#  TextTemplate.cpp
#  UnicodeNormalizer.cpp
#  UnicodeNormTables.cpp
#  UnicodeSymbol.cpp
#  UTF8AllProperties.cpp
#  UTF8Case.cpp
#  UTF8CaseLower.cpp
#  UTF8CaseSimplify.cpp
#  UTF8CaseUniform.cpp
#  UTF8CaseUpper.cpp
#  UTF8Category.cpp
#  UTF8IsDigit.cpp
#  UTF8IsLetter.cpp
#  UTF8IsLowerLetter.cpp
#  UTF8IsUpperLetter.cpp
#  UTF8IsSpace.cpp
#  UTF8IsTitleLetter.cpp

)

target_link_libraries(${proj}
  #Generics
  pcre
  Iconv::Iconv
)

#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})
