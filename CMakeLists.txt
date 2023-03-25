cmake_minimum_required(VERSION 3.20.0)
project(Moribund C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wstrict-aliasing=1")

add_executable(moribund
    moribund.c
)
