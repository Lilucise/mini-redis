cmake_minimum_required(VERSION 3.16)

project(mini-redis CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(echo_server src/main.cpp)
target_include_directories(echo_server PRIVATE include) # 让 #include "thread_pool.h" 找得到
add_executable(thread_pool_test src/thread_pool.cpp)    # 回归测试保留