cmake_minimum_required(VERSION 3.18)
project(datax-cpp)

include(FetchContent)

FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG v3.7.3
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
        )

FetchContent_MakeAvailable(json)
set(CMAKE_CXX_STANDARD 14)

add_library(${PROJECT_NAME} STATIC
        datax.cpp
        datax.h)
target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/include)

# install (if required)
target_include_directories(
        ${PROJECT_NAME}
        INTERFACE $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-config)
install(
        EXPORT ${PROJECT_NAME}-config
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})
install(
        DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/include/${PROJECT_NAME}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME})

add_executable(test-sdk
        test-sdk.cpp)
target_link_libraries(test-sdk ${PROJECT_NAME})