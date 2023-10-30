cmake_minimum_required(VERSION 3.18)
project(datax-cpp)

set(CMAKE_CXX_STANDARD 17)

add_library(${PROJECT_NAME} STATIC
        datax.cpp
        datax.h)
target_compile_options(${PROJECT_NAME} PUBLIC "-O3")
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(${PROJECT_NAME} protobuf::libprotobuf ${GRPCPP_LIBRARIES})

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
