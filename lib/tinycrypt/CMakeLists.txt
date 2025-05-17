# Edit the OBJS content to add/remove primitives needed from TinyCrypt library:
set(TINYCRYPT_PRIMITIVES hmac sha256 utils)

cmake_minimum_required(VERSION 3.13)
project(TinyCrypt C)

if(DEFINED TINYCRYPT_PRIMITIVES)
    set(TINYCRYPT_SOURCES)
    foreach(primitive ${TINYCRYPT_PRIMITIVES})
        list(APPEND TINYCRYPT_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/lib/source/${primitive}.c")
    endforeach()
else()
    file(GLOB TINYCRYPT_SOURCES "lib/source/*.c")
endif()

add_library(tinycrypt STATIC ${TINYCRYPT_SOURCES})

target_include_directories(tinycrypt PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/include
)

set_target_properties(tinycrypt PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)