cmake_minimum_required(VERSION 3.18)
project(datax-cpp)

set(CMAKE_CXX_STANDARD 14)

find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)

if(NOT GRPC_CPP_PLUGIN)
    find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin REQUIRED)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Protobuf compiler: ${Protobuf_PROTOC_EXECUTABLE}")
message(STATUS "gRPC C++ plugin: ${GRPC_CPP_PLUGIN}")

set(PROTOCOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/datax-sdk-protocol/v1/datax-sdk-protocol.proto)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.proto
        COMMAND ${CMAKE_COMMAND} -E copy ${PROTOCOL_FILE} ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.proto
        DEPENDS ${PROTOCOL_FILE})

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.grpc.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.pb.h
        COMMAND ${Protobuf_PROTOC_EXECUTABLE} --grpc_out ${CMAKE_CURRENT_BINARY_DIR} --cpp_out ${CMAKE_CURRENT_BINARY_DIR} --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.proto
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.proto)

add_custom_target(generate
        DEPENDS sdk.grpc.pb.cc)
add_library(${PROJECT_NAME} STATIC
        datax.cpp
        datax.h
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.grpc.pb.h
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/datax-sdk-protocol-v1.pb.h)
target_compile_options(${PROJECT_NAME} PUBLIC "-O3")
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(${PROJECT_NAME} protobuf::libprotobuf gRPC::grpc++)

# install (if required)
target_include_directories(${PROJECT_NAME}
        INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-config)
install(EXPORT ${PROJECT_NAME}-config
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(FILES ${CMAKE_CURRENT_LIST_DIR}/datax.h ${CMAKE_CURRENT_LIST_DIR}/json.hpp
       TYPE INCLUDE)

add_executable(test-sdk
        test-sdk.cpp)
target_link_libraries(test-sdk ${PROJECT_NAME})