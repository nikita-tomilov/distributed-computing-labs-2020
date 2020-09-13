cmake_minimum_required(VERSION 3.15)
project(pa6 C)

set(CMAKE_C_STANDARD 99)
add_compile_options(-Wall -pedantic -Werror)

include_directories(.)
link_directories("../dylib")

add_executable(pa6
    bank_robbery.c
    banking.c
    banking.h
    common.h
    ipc.c
    ipc.h
    lamport.c
    lamport.h
    main.c
    main.h
    message.c
    message.h
    pa2345.h)
target_link_libraries(pa6 runtime)