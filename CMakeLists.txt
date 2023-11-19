cmake_minimum_required(VERSION 3.26)
project(ServC C)

set(CMAKE_C_STANDARD 23)
link_libraries(ws2_32 wsock32)
add_executable(ServC main.c)
