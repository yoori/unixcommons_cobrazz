#cmake_minimum_required (VERSION 2.6)


include_directories(. ${IDL_DIRECTORY})


ADD_SUBDIRECTORY(CORBACommons)
ADD_SUBDIRECTORY(CORBAConfigParser)
ADD_SUBDIRECTORY(ProbeObj)
ADD_SUBDIRECTORY(StatsTool)


