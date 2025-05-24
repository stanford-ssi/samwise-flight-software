add_library(packet packet.c)

target_link_libraries(packet PUBLIC pico_stdlib logger tinycrypt)

# Add TinyCrypt include directory as PUBLIC so dependents inherit it
target_include_directories(packet PUBLIC
  "${PROJECT_SOURCE_DIR}/src/packet"
  "${PROJECT_SOURCE_DIR}/lib/tinycrypt/lib/include"
)
