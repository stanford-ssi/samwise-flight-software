add_library(packet packet.c)

if(PROFILE STREQUAL "TEST")
    # Test build - no Pico SDK
    target_link_libraries(packet PUBLIC logger tinycrypt)
else()
    # Embedded build - link Pico SDK
    target_link_libraries(packet PUBLIC pico_stdlib logger tinycrypt)
endif()

# Add TinyCrypt include directory as PUBLIC so dependents inherit it
target_include_directories(packet PUBLIC
  "${PROJECT_SOURCE_DIR}/src/packet"
  "${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/include"
)

# Add test subdirectory
add_subdirectory(test)
