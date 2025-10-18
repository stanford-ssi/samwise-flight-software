samwise_add_test(
  NAME mram_test
  SOURCES mram_test.c ${PROJECT_SOURCE_DIR}/src/drivers/mram/mram.c
  LIBRARIES test_mocks
)
