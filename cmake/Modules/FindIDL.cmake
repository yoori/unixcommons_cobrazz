# Redistribution and use is allowed under the OSI-approved 3-clause BSD license.
# Copyright (c) 2018 Apriorit Inc. All rights reserved.

set(IDL_FOUND TRUE)

function(add_idl _target _idlfile target_dir)
    get_filename_component(IDL_FILE_NAME_WE ${_idlfile} NAME_WE)
#    set(MIDL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Generated)
    set(MIDL_OUTPUT_PATH ${target_dir})
    set(MIDL_OUTPUT ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.ipp)
    set(OUTPUTC ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.cpp)
    set(OUTPUTS ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}_s.cpp)

    if(${CMAKE_SIZEOF_VOID_P} EQUAL 4)
        set(MIDL_ARCH win32)
    else()
        set(MIDL_ARCH x64)
    endif()

    
#    target_sources(${_target} PUBLIC 
#	${IDL_FILE_NAME_WE}C.cpp
#	${IDL_FILE_NAME_WE}S.cpp
#    )
#    set (MIDL_FLAGS "-Sp -in -ci .ipp -cs .cpp -hc .hpp -hs _s.hpp -ss _s.cpp ")
    
#    add_custom_target(${TGT} ALL
#    COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir}
#    )

    file(MAKE_DIRECTORY ${target_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target_dir})
    set (SRC ${CMAKE_CURRENT_LIST_DIR}/${_idlfile})
    set(FINDIDL_TARGET ${_target}_gen)
    add_custom_target(${FINDIDL_TARGET} DEPENDS "${OUTPUTS}" )
#    add_custom_target(${_target}_ggg COMMAND "touch" "${OUTPUTS}")
    add_custom_target(${_target}_ggg DEPENDS ${OUTPUTS})

# SOURCES ${_idlfile}
    add_custom_command(
       OUTPUT ${MIDL_OUTPUT} ${OUTPUTC} ${OUTPUTS}
#       COMMAND ${CMAKE_COMMAND} -E echo "tao_idl gen outfiles to ${MIDL_OUTPUT_PATH} from ${SRC}"
       COMMAND tao_idl ARGS  -Sp -in -ci .ipp -cs .cpp -hc .hpp -hs _s.hpp -ss _s.cpp -I ${PROJECT_SOURCE_DIR}/src/CORBA  ${SRC} -o ${MIDL_OUTPUT_PATH} 2> NUL
       #${MIDL_FLAGS}

#       COMMAND ${CMAKE_COMMAND} -E echo "done generating ${MIDL_OUTPUT_PATH}"
#       COMMAND ${CMAKE_COMMAND} -E echo "OUTPUTC ${OUTPUTC};"
       COMMAND sed -i "'s/if (0 == &_tao_elem)/if (true)/g'" ${OUTPUTC}
#       COMMAND ${CMAKE_COMMAND} -E echo "done sed"
       COMMENT "Add IDL. ${_target}"
#####################
#       add_custom_command(OUTPUT "${SRC}" COMMAND ${CMAKE_COMMAND} -E touch "${SRC}") #More reliable touch, use cmake itself to touch the file
#add_custom_target(generate_version_h DEPENDS "${SRC}")
#add_executable(myprog ${test_SOURCES})
#add_dependencies(myprog generate_version_h)
##################



#/usr/bin/tao_idl -o . -I../../../../src/CORBA -Sp -in -ci .ipp -cs .cpp -hc .hpp -hs _s.hpp -ss _s.cpp ../../../../src/CORBA/CORBACommons/CorbaObjectRef.idl
       #/h ${MIDL_OUTPUT}
#       DEPENDS ${_target}_z1
#       DEPENDS ${OUTPUTC} ${OUTPUTS}
    DEPENDS ${_target}_ggg
#    DEPENDS ${SRC}
    #${FINDIDL_TARGET}
#       DEPENDS    ${SRC}
       #${TGT}
       #  ${FINDIDL_TARGET}
       #${_target}_z1
#       DEPENDS  ${_target}
#       VERBATIM
       )

#        add_dependencies(${FINDIDL_TARGET} ${SRC})
#    add_custom_target( ${_target}_z1  DEPENDS ${OUTPUTC} ${OUTPUTS} SOURCES ${SRC} )

#       add_dependencies(${OUTPUTS} ${SRC})
#       add_dependencies(${OUTPUTC} ${CMAKE_CURRENT_LIST_DIR}/${_idlfile})

#       MESSAGE("command idl " ${_idlfile})

    

    cmake_parse_arguments(FINDIDL "" "TLBIMP" "" ${ARGN})
 
    if(FINDIDL_TLBIMP)
        file(GLOB TLBIMPv7_FILES "C:/Program Files*/Microsoft SDKs/Windows/v7*/bin/TlbImp.exe") 
        file(GLOB TLBIMPv8_FILES "C:/Program Files*/Microsoft SDKs/Windows/v8*/bin/*/TlbImp.exe")
        file(GLOB TLBIMPv10_FILES "C:/Program Files*/Microsoft SDKs/Windows/v10*/bin/*/TlbImp.exe")

        list(APPEND TLBIMP_FILES ${TLBIMPv7_FILES} ${TLBIMPv8_FILES} ${TLBIMPv10_FILES})

        if(TLBIMP_FILES)
            list(GET TLBIMP_FILES -1 TLBIMP_FILE)
        endif()

        if (NOT TLBIMP_FILE)
            message(FATAL_ERROR "Cannot found tlbimp.exe. Try to download .NET Framework SDK and .NET Framework targeting pack.")
            return()
        endif()

        message(STATUS "Found tlbimp.exe: " ${TLBIMP_FILE})

        set(TLBIMP_OUTPUT_PATH ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

        if("${TLBIMP_OUTPUT_PATH}" STREQUAL "")
            set(TLBIMP_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
        endif()
        
        set(TLBIMP_OUTPUT ${TLBIMP_OUTPUT_PATH}/${FINDIDL_TLBIMP}.dll)

        add_custom_command(
            OUTPUT  ${TLBIMP_OUTPUT}
            COMMAND ${TLBIMP_FILE} "${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.tlb" "/out:${TLBIMP_OUTPUT}" ${TLBIMP_FLAGS}
            DEPENDS ${MIDL_OUTPUT_PATH}/${IDL_FILE_NAME_WE}.tlb
            COMMENT "Using ${TLBIMP_FILE}"
            VERBATIM
            )

        add_custom_target(${FINDIDL_TARGET} DEPENDS ${MIDL_OUTPUT} ${TLBIMP_OUTPUT} SOURCES ${_idlfile})

        add_library(${FINDIDL_TLBIMP} SHARED IMPORTED GLOBAL)
        add_dependencies(${FINDIDL_TLBIMP} ${FINDIDL_TARGET})

        set_target_properties(${FINDIDL_TLBIMP}
            PROPERTIES
            IMPORTED_LOCATION "${TLBIMP_OUTPUT}"
            IMPORTED_COMMON_LANGUAGE_RUNTIME "CSharp"
            )
    else()
#        add_custom_target(${FINDIDL_TARGET} DEPENDS ${MIDL_OUTPUT} SOURCES ${_idlfile} )
    endif()

    add_library(${_target} SHARED
#    ${IDL_FILE_NAME_WE}/${IDL_FILE_NAME_WE}S.cpp
#    ${IDL_FILE_NAME_WE}/${IDL_FILE_NAME_WE}C.cpp
      ${OUTPUTC} ${OUTPUTS}
      )

    target_link_libraries(
	${_target}
	ACE
	TAO TAO_AnyTypeCode  TAO_CodecFactory TAO_CosEvent TAO_CosNaming TAO_CosNotification TAO_DynamicAny TAO_EndpointPolicy TAO_FaultTolerance
	TAO_FT_ClientORB TAO_FT_ServerORB TAO_FTORB_Utils TAO_IORManip TAO_IORTable 
	TAO_Messaging TAO_PI TAO_PI_Server TAO_PortableGroup TAO_PortableServer TAO_Security TAO_SSLIOP TAO_TC TAO_TC_IIOP TAO_Valuetype ACE_SSL 

    )
    install(TARGETS ${_target} DESTINATION ${INSTALL_LIB})
    #SOURCES 
    #${OUTPUTC} ${OUTPUTS} 
#    add_library(${_target}LL STATIC
#	${OUTPUTC} ${OUTPUTS}
#    )
    add_dependencies(${_target} ${FINDIDL_TARGET})

    target_include_directories(${_target} PUBLIC
      $<BUILD_INTERFACE:${MIDL_OUTPUT_PATH}>
      $<INSTALL_INTERFACE:${MIDL_OUTPUT_PATH}>)

endfunction()
