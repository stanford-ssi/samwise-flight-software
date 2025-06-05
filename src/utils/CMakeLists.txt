add_library(utils INTERFACE)

target_link_libraries(utils INTERFACE slate)

target_include_directories(utils INTERFACE 
  "${PROJECT_SOURCE_DIR}/src/utils"
)
