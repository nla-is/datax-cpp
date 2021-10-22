cmake_minimum_required(VERSION 3.20)
project(datax-cpp)

include(FetchContent)

FetchContent_Declare(json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG v3.7.3)

FetchContent_GetProperties(json)
if(NOT json_POPULATED)
    FetchContent_Populate(json)
    add_subdirectory(${json_SOURCE_DIR} ${json_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

set(CMAKE_CXX_STANDARD 14)

add_library(${PROJECT_NAME}
        datax.cpp
        datax.h)

target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
