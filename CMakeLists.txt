cmake_minimum_required(VERSION 3.16)

project(mini-redis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(echo_server src/main.cpp)