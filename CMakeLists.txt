cmake_minimum_required(VERSION 3.14)
project(Socket C)

set(CMAKE_C_STANDARD 99)

add_executable(Socket server.c)
SET(CMAKE_C_FLAGS -pthread)