set(PROTO_GRPC_USRV_PLUGIN ${CMAKE_CURRENT_LIST_DIR}/protoc_usrv_plugin)

function(generate_grpc_files)
  set(options)
  set(one_value_args CPP_FILES CPP_USRV_FILES GENERATED_INCLUDES SOURCE_PATH)
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(GEN_RPC "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(GEN_RPC_INCLUDE_DIRECTORIES)
    set(include_options)
    foreach(include ${GEN_RPC_INCLUDE_DIRECTORIES})
      if(NOT IS_ABSOLUTE ${include})
        if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${include})
          set(include ${CMAKE_CURRENT_SOURCE_DIR}/${include})
        elseif(EXISTS ${CMAKE_SOURCE_DIR}/${include})
          set(include ${CMAKE_SOURCE_DIR}/${include})
        endif()
      endif()
      get_filename_component(include "${include}" REALPATH BASE_DIR "/")
      if(EXISTS ${include})
        list(APPEND include_options -I ${include})
      else()
        message(WARNING "Include directory ${include} for protoc generator not found")
      endif()
    endforeach()
  endif()

  set(GENERATED_PROTO_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
  get_filename_component(GENERATED_PROTO_DIR "${GENERATED_PROTO_DIR}" REALPATH BASE_DIR "/")

  if(NOT "${GEN_RPC_SOURCE_PATH}" STREQUAL "")
    if(NOT IS_ABSOLUTE ${GEN_RPC_SOURCE_PATH})
      message(SEND_ERROR "SOURCE_PATH='${GEN_RPC_SOURCE_PATH}' is a relative path, which is unsupported.")
    endif()
    set(root_path "${GEN_RPC_SOURCE_PATH}")
  else()
    set(root_path "${CMAKE_CURRENT_SOURCE_DIR}/proto")
  endif()

  get_filename_component(root_path "${root_path}" REALPATH BASE_DIR "/")
  message(STATUS "Generating sources for protos in ${root_path}:")

  set(proto_dependencies_globs ${GEN_RPC_INCLUDE_DIRECTORIES})
  list(TRANSFORM proto_dependencies_globs APPEND "/*.proto")
  list(APPEND proto_dependencies_globs
    "${root_path}/*.proto"
  )

  file(GLOB_RECURSE proto_dependencies ${proto_dependencies_globs})
  list(GET proto_dependencies 0 newest_proto_dependency)
  foreach(dependency ${proto_dependencies})
    if("${dependency}" IS_NEWER_THAN "${newest_proto_dependency}")
      set(newest_proto_dependency "${dependency}")
    endif()
  endforeach()

  foreach (proto_file ${GEN_RPC_PROTOS})
    get_filename_component(proto_file "${proto_file}" REALPATH BASE_DIR "${root_path}")

    get_filename_component(path ${proto_file} DIRECTORY)
    get_filename_component(name_base ${proto_file} NAME_WE)
    file(RELATIVE_PATH rel_path "${root_path}" "${path}")

    if(rel_path)
      set(path_base "${rel_path}/${name_base}")
    else()
      set(path_base "${name_base}")
    endif()

    set(did_generate_proto_sources FALSE)
    if("${newest_proto_dependency}" IS_NEWER_THAN "${GENERATED_PROTO_DIR}/${path_base}.pb.cc")
      execute_process(
        COMMAND mkdir -p proto
        COMMAND ${PROTOBUF_PROTOC} ${include_options}
              --cpp_out=${GENERATED_PROTO_DIR}
              --grpc_out=${GENERATED_PROTO_DIR}
              --usrv_out=${GENERATED_PROTO_DIR}
              -I ${root_path}
              --plugin=protoc-gen-grpc=${PROTO_GRPC_CPP_PLUGIN}
              --plugin=protoc-gen-usrv=${PROTO_GRPC_USRV_PLUGIN}
              ${proto_file}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE execute_process_result
      )
      if(execute_process_result)
        message(SEND_ERROR "Error while generating gRPC sources for ${path_base}.proto")
      else()
        set(did_generate_proto_sources TRUE)
      endif()
    else()
      message(STATUS "Reused previously generated sources for ${path_base}.proto")
    endif()

    set(files
      ${GENERATED_PROTO_DIR}/${path_base}.pb.h
      ${GENERATED_PROTO_DIR}/${path_base}.pb.cc
    )

    if (EXISTS ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.hpp)
      if(did_generate_proto_sources)
        message(STATUS "Generated sources for ${path_base}.proto with gRPC")
      endif()

      set(usrv_files
        ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_client.usrv.pb.cpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.usrv.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.usrv.pb.cpp
      )
      set(usrv_wrap_files
        ${GENERATED_PROTO_DIR}/${path_base}_client.cobrazz.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_client.cobrazz.pb.cpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.cobrazz.pb.hpp
        ${GENERATED_PROTO_DIR}/${path_base}_service.cobrazz.pb.cpp
      )
      list(APPEND files
        ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.h
        ${GENERATED_PROTO_DIR}/${path_base}.grpc.pb.cc
      )
    elseif(did_generate_proto_sources)
      message(STATUS "Generated sources for ${path_base}.proto")
    endif()

    set_source_files_properties(${files} ${usrv_files} ${usrv_wrap_files} PROPERTIES GENERATED 1)
    list(APPEND generated_cpps ${files})
    list(APPEND generated_usrv_cpps ${usrv_files})
    list(APPEND generated_usrv_cpps ${usrv_wrap_files})
  endforeach()

  if(GEN_RPC_GENERATED_INCLUDES)
    set(${GEN_RPC_GENERATED_INCLUDES} ${GENERATED_PROTO_DIR} PARENT_SCOPE)
  endif()
  if(GEN_RPC_CPP_FILES)
    set(${GEN_RPC_CPP_FILES} ${generated_cpps} PARENT_SCOPE)
  endif()
  if(GEN_RPC_CPP_USRV_FILES)
    set(${GEN_RPC_CPP_USRV_FILES} ${generated_usrv_cpps} PARENT_SCOPE)
  endif()
endfunction()

function(add_grpc_library NAME)
  set(options)
  set(one_value_args SOURCE_PATH)
  set(multi_value_args PROTOS INCLUDE_DIRECTORIES)
  cmake_parse_arguments(RPC_LIB "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  generate_grpc_files(
    PROTOS ${RPC_LIB_PROTOS}
    INCLUDE_DIRECTORIES ${RPC_LIB_INCLUDE_DIRECTORIES}
    SOURCE_PATH ${RPC_LIB_SOURCE_PATH}
    GENERATED_INCLUDES include_paths
    CPP_FILES generated_sources
    CPP_USRV_FILES generated_usrv_sources
  )

  add_library(${NAME} STATIC ${generated_sources} ${generated_usrv_sources})
  target_compile_options(${NAME} PUBLIC -Wno-unused-parameter)

  if(NOT TARGET userver-core)
    message(SEND_ERROR "GrpcTargets: no available userver-core library.")
  endif()

  if(NOT TARGET userver-grpc)
    message(SEND_ERROR "GrpcTargets: no available userver-grpc library.")
  endif()

  if(NOT TARGET userver-api-common-protos)
    message(SEND_ERROR "GrpcTargets: no available userver-api-common-protos library.")
  endif()

  target_link_libraries(${NAME} PUBLIC
    userver-core
    userver-grpc
    userver-api-common-protos # userver proto files
  )

  target_include_directories(${NAME}
    PUBLIC
      ${include_paths}
  )

endfunction()