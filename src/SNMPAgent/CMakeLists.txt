set(proj "SNMPAgent")

add_library(${proj} SHARED
  SNMPAgentX.cpp
)

target_link_libraries(${proj}
  PRIVATE
  Generics
  pthread
  netsnmpagent
  netsnmp)
