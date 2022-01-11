cmake_minimum_required(VERSION 3.18)
project(datax-cpp)

set(CMAKE_CXX_STANDARD 14)

find_package(Protobuf REQUIRED)
#find_package(gRPC REQUIRED)

find_program(GRPC_CPP_PLUGIN grpc_cpp_plugin)
find_library(GRPC_CPP_LIBRARY grpc++ libgrpc++)

include_directories(${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})

#get_target_property(GRPC_CPP_PLUGIN gRPC::grpc_cpp_plugin LOCATION)


message(STATUS "${Protobuf_PROTOC_EXECUTABLE} ${GRPC_CPP_PLUGIN}")

add_custom_command(OUTPUT sdk.grpc.pb.cc sdk.grpc.pb.h sdk.pb.cc sdk.pb.h
        COMMAND protoc --grpc_out . --cpp_out . --plugin=protoc-gen-grpc=${GRPC_CPP_PLUGIN} -I ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/sdk.proto)

add_library(${PROJECT_NAME} STATIC
        datax.cpp
        datax.h
        sdk.grpc.pb.cc
        sdk.grpc.pb.h
        sdk.pb.cc
        sdk.pb.h)
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(${PROJECT_NAME} protobuf::libprotobuf ${GRPC_CPP_LIBRARY})

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