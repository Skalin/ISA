cmake_minimum_required(VERSION 3.7)
project(ISA)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp lib/auth.cpp lib/base.cpp lib/base.h lib/auth.h)
add_executable(ISA ${SOURCE_FILES})