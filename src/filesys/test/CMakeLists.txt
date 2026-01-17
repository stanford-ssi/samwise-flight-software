samwise_add_test(
  NAME mram_test
  SOURCES mram_test.c
)

samwise_add_test(
  NAME filesys_test
  SOURCES filesys_test.c ${PROJECT_SOURCE_DIR}/src/filesys/filesys.c
  LIBRARIES filesys
)
