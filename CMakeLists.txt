cmake_minimum_required(VERSION 3.10.2)
project(Socket C)

set(CMAKE_C_STANDARD 99)

add_executable(Socket server.c)
target_link_libraries(Socket pthread)
