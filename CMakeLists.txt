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

add_library(${PROJECT_NAME}
        datax.cpp
        datax.h)

target_include_libraries(${PROJECT_NAME} INTERFACE ${CMAKE_CURRENT_BINARY_DIR})

add_executable(test-sdk
        test-sdk.cpp)
target_link_libraries(test-sdk ${PROJECT_NAME})