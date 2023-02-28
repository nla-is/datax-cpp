cmake_minimum_required(VERSION 3.18)
project(repeater)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(datax-cpp
        GIT_REPOSITORY https://github.com/nla-is/datax-cpp.git
        GIT_TAG 00a0f6fdef34f86f5254faf20a51782c94ed324a
        )
FetchContent_MakeAvailable(datax-cpp)

add_executable(${PROJECT_NAME}
        main.cpp)
target_link_libraries(${PROJECT_NAME} datax-cpp)
target_compile_options(${PROJECT_NAME} PUBLIC "-O3")
