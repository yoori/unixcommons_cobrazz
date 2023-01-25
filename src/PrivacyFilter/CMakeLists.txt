set(proj "PrivacyFilter")

add_library(${proj}  SHARED
    Filter.cpp
)


target_link_libraries(${proj} crypto)


#install(TARGETS ${proj} DESTINATION ${INSTALL_LIB})