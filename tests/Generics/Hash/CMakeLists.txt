set(proj "TestHasher")

add_executable(${proj}
  AvalancheTest.cpp
  Bitslice.cpp
  Bitvec.cpp
  KeysetTest.cpp
  Main.cpp
  Platform.cpp
  Random.cpp
  SpeedTest.cpp
  Stats.cpp
  Test.cpp
  Types.cpp
)
target_link_libraries(${proj}
  PRIVATE
    Generics
)

add_test(NAME ${proj}
         COMMAND ${proj})

add_executable(TestXxh3 Xxh3Test.cpp)
target_link_libraries(TestXxh3 PRIVATE Generics)
get_target_property(generics_hash_definitions Generics COMPILE_DEFINITIONS)
foreach(definition IN LISTS generics_hash_definitions)
  if(definition MATCHES "^GENERICS_XXH3_HAS_")
    target_compile_definitions(TestXxh3 PRIVATE ${definition})
  endif()
endforeach()
add_test(NAME TestXxh3 COMMAND TestXxh3)

add_executable(TestFastHash FastHashTest.cpp)
target_link_libraries(TestFastHash PRIVATE Generics)
add_test(NAME TestFastHash COMMAND TestFastHash)

add_executable(FastHashBenchmark FastHashBenchmark.cpp)
target_link_libraries(FastHashBenchmark PRIVATE Generics)
