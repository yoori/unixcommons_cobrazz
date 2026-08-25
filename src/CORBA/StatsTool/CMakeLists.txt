add_executable(StatsTool
  Application.cpp
)

target_link_libraries(StatsTool
  PRIVATE
    TAO::AnyTypeCode
    TAO::TAO
    StatsIDL
)
