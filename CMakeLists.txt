cmake_minimum_required(VERSION 3.28)

project(logger
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(third_party)
