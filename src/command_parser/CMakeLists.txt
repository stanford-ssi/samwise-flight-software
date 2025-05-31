add_library(command_parser command_parser.c)

# Specify targets to link compiled binaries
target_link_libraries(command_parser PUBLIC slate packet common error)

# Specify directories with relevant header files
target_include_directories(command_parser PUBLIC
  "${PROJECT_SOURCE_DIR}/src/command_parser"
)
