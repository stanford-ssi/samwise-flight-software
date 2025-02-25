add_library(logger logger.c)

target_link_libraries(logger PUBLIC pico_stdlib)


target_include_directories(logger PUBLIC
  "${PROJECT_SOURCE_DIR}/src/drivers/logger"
)
