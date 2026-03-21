# Build debug picubed and run tests with the real file path set to the built binary.
bazel build :samwise --config=picubed-debug

bazel test --test_env=FTP_TEST_REAL_FILE_PATH="$(pwd)/bazel-bin/samwise.bin" $@ //...
