cmake_minimum_required(VERSION 3.13)
project(joy)

set(CMAKE_CXX_STANDARD 14)

add_executable(joy main.cpp Joystick.cpp Joystick.h)