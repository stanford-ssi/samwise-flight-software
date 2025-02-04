add_library(error error.c)
target_link_libraries(error PUBLIC
  $<IF:$<BOOL:BUILD_TEST>,mock_pico,pico_stdlib>
)

target_include_directories(error PUBLIC
  "${PROJECT_SOURCE_DIR}/src/common"
  "${PROJECT_SOURCE_DIR}/src/mock_pico"
)
