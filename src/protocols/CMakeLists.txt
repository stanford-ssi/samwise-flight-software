add_library(protocols file_transfer.c)

target_link_libraries(protocols PRIVATE slate pico_stdlib)

target_include_directories(protocols PUBLIC 
  "${PROJECT_SOURCE_DIR}/src/protocols"
)
