set(proj "Generics")

include(CheckCXXCompilerFlag)

add_library(${proj} SHARED
  Allocator.cpp
  ActiveObject.cpp
  AppUtils.cpp
  CommonDecimal.cpp
  CompositeActiveObject.cpp
  CountryCodeManip.cpp
  CRC.cpp
  Crypto.cpp
  Hash.cpp
  Listener.cpp
  MMap.cpp
  MemBuf.cpp
  Network.cpp
  Periodic.cpp
  Plug.cpp
  Proc.cpp
  Rand.cpp
  Scheduler.cpp
  SimdCapabilities.cpp
  Singleton.cpp
  Statistics.cpp
  TaskRunner.cpp
  TaskPool.cpp
  Time.cpp
  ThreadRunner.cpp
  Uuid.cpp
  Values.cpp
  ../String/Analyzer.cpp
  ../String/AsciiStringManip.cpp
  ../String/BasicAnalyzer.cpp
  ../String/InterConvertion.cpp
  ../String/RegEx.cpp
  ../String/StringManip.cpp
  ../String/TextTemplate.cpp
  ../String/UTF8AllProperties.cpp
  ../String/UTF8Case.cpp
  ../String/UTF8CaseLower.cpp
  ../String/UTF8CaseSimplify.cpp
  ../String/UTF8CaseUniform.cpp
  ../String/UTF8CaseUpper.cpp
  ../String/UTF8Category.cpp
  ../String/UTF8IsDigit.cpp
  ../String/UTF8IsLetter.cpp
  ../String/UTF8IsLowerLetter.cpp
  ../String/UTF8IsSpace.cpp
  ../String/UTF8IsTitleLetter.cpp
  ../String/UTF8IsUpperLetter.cpp
  ../String/UnicodeNormTables.cpp
  ../String/UnicodeNormalizer.cpp
  ../String/UnicodeSymbol.cpp

  CompositeMetricsProvider.cpp
)

target_link_libraries(${proj}
  PUBLIC
  crypto

  PRIVATE
  Sync
  pthread
  ev
  pcre2-8
  dl
  Iconv::Iconv
  Boost::headers
  #String
)

set(string_manip_json_escape_x86 OFF)
if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64|i[3-6]86)$")
  set(string_manip_json_escape_x86 ON)
endif()

if(string_manip_json_escape_x86)
  check_cxx_compiler_flag(
    "-msse2"
    STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_SSE2)
  check_cxx_compiler_flag(
    "-mavx2"
    STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_AVX2)
  check_cxx_compiler_flag(
    "-mavx512f"
    STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_AVX512F)
  check_cxx_compiler_flag(
    "-mavx512bw"
    STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_AVX512BW)

  if(STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_SSE2)
    target_sources(${proj} PRIVATE
      ../String/StringManipJsonEscapeSse2.cpp
    )
    set_source_files_properties(
      ../String/StringManipJsonEscapeSse2.cpp
      PROPERTIES COMPILE_OPTIONS "-msse2")
    target_compile_definitions(${proj} PRIVATE
      STRING_MANIP_JSON_ESCAPE_HAS_SSE2)
  endif()

  if(STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_AVX2)
    target_sources(${proj} PRIVATE
      ../String/StringManipJsonEscapeAvx2.cpp
    )
    set_source_files_properties(
      ../String/StringManipJsonEscapeAvx2.cpp
      PROPERTIES COMPILE_OPTIONS "-mavx2")
    target_compile_definitions(${proj} PRIVATE
      STRING_MANIP_JSON_ESCAPE_HAS_AVX2)
  endif()

  if(STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_AVX512F AND
    STRING_MANIP_JSON_ESCAPE_CAN_COMPILE_AVX512BW)
    target_sources(${proj} PRIVATE
      ../String/StringManipJsonEscapeAvx512.cpp
    )
    set_source_files_properties(
      ../String/StringManipJsonEscapeAvx512.cpp
      PROPERTIES COMPILE_OPTIONS "-mavx512f;-mavx512bw")
    target_compile_definitions(${proj} PRIVATE
      STRING_MANIP_JSON_ESCAPE_HAS_AVX512BW)
  endif()
endif()

#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})
