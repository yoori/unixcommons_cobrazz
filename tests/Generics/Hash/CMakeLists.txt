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
