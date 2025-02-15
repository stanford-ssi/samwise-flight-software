add_library(common INTERFACE)

target_link_libraries(common INTERFACE error)

target_include_directories(common INTERFACE 
  "${PROJECT_SOURCE_DIR}/src/common"
)
