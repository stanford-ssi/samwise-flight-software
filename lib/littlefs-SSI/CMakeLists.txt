cmake_minimum_required(VERSION 3.13)
project(littlefs C)

# Collect core sources (exclude test/runner programs)
file(GLOB LFS_CORE_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
  # "${CMAKE_CURRENT_SOURCE_DIR}/bd/*.c"
)

# Remove test/runner sources that build executables
list(REMOVE_ITEM LFS_CORE_SRCS
  "${CMAKE_CURRENT_SOURCE_DIR}/runners/test_runner.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/runners/bench_runner.c"
)

# Create static library
add_library(littlefs STATIC ${LFS_CORE_SRCS})

# Public include dir so dependents can include <lfs.h>
target_include_directories(littlefs PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Place archives in top-level build lib dir for consistency with other libs
set_target_properties(littlefs PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# Optional: enable PIC for use in mixed toolchains
if(NOT CMAKE_POSITION_INDEPENDENT_CODE)
  set_target_properties(littlefs PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()