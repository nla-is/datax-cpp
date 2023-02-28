cmake_minimum_required(VERSION 3.18)
project(repeater)

set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

FetchContent_Declare(datax-cpp
        GIT_REPOSITORY https://github.com/nla-is/datax-cpp.git
        GIT_TAG 0f6cdaac0179871ba87bb67718989fac4d2909b2
        )
FetchContent_MakeAvailable(datax-cpp)

add_executable(${PROJECT_NAME}
        main.cpp)
target_link_libraries(${PROJECT_NAME} datax-cpp)
