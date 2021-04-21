# Minimum CMake required
cmake_minimum_required(VERSION 3.15)

# Project
project(spaceart)

# Uncomment this line to only compile, and not link
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -c")

# Protobuf
include(FindProtobuf)
set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf REQUIRED)
message(STATUS "Using protobuf ${Protobuf_VERSION}")
include_directories(${PROTOBUF_INCLUDE_DIRS})

# Protobuf-compiler
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)

# gRPC
find_package(gRPC REQUIRED)
message(STATUS "Using gRPC ${gRPC_VERSION}")
set(_GRPC_GRPCPP gRPC::grpc++)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Proto file
get_filename_component(hw_proto "spaceart.proto" ABSOLUTE)
get_filename_component(hw_proto_path "${hw_proto}" PATH)

#set(opentelemetry-cpp_DIR ~/include/lib/cmake/opentelemetry-cpp)
find_package(opentelemetry-cpp REQUIRED)

# Generated sources
set(hw_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/spaceart.pb.cc")
set(hw_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/spaceart.pb.h")
set(hw_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/spaceart.grpc.pb.cc")
set(hw_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/spaceart.grpc.pb.h")

add_custom_command(
      OUTPUT "${hw_proto_srcs}" "${hw_proto_hdrs}" "${hw_grpc_srcs}" "${hw_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${hw_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${hw_proto}"
      DEPENDS "${hw_proto}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

#set(CMAKE_LIBRARY_PATH ~/include/lib/)

#set(MY_INCLUDES "/Users/lucas/include/include")
#include_directories(${MY_INCLUDES})

#set(INCLUDE_RANDOM "/Users/lucas/source/opentelemetry-cpp/sdk/src/common/")
#include_directories(${INCLUDE_RANDOM})

#find_library(TRACE opentelemetry_trace ~/include/lib/libopentelemetry_trace.a)
#find_library(EXPORT opentelemetry_exporter_ostream_span ~/include/lib/#libopentelemetry_exporter_ostream_span.a)
#find_library(COMMON opentelemetry_common ~/include/lib/libopentelemetry_common.a)
#find_library(RESOURCES opentelemetry_resources ~/include/lib/libopentelemetry_resources.a)



# Targets (client|server)
foreach(_target
  viewer presenter registrar gallery)
  target_include_directories(${_target} PRIVATE ${OPENTELEMETRY_CPP_INCLUDE_DIRS})
  add_executable(${_target} "${_target}.cc"
    ${hw_proto_srcs}
    ${hw_grpc_srcs})
  target_link_libraries(${_target}
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${PROTOBUF_LIBRARY}
    ${TRACE}
    ${EXPORT}
    ${COMMON}
    ${RESOURCES}
    ${OPENTELEMETRY_CPP_LIBRARIES}
)
endforeach()
