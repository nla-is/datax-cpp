cmake_minimum_required(VERSION 3.14)
project(repeater)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(datax-cpp
        GIT_REPOSITORY https://github.com/nla-is/datax-cpp.git
        GIT_TAG v2.0.0-alpha.4
        )
FetchContent_MakeAvailable(datax-cpp)

add_executable(${PROJECT_NAME}
        main.cpp)
target_link_libraries(${PROJECT_NAME} datax-cpp pthread)
target_compile_options(${PROJECT_NAME} PUBLIC "-O3")
