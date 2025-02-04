# Helper script to recursively clang-format every file
# Install clang-format on mac: brew install clang-format
echo Formatting all files...
find src -name "*.cpp" -o -name "*.c" -o -name "*.h" | xargs -I {} clang-format -i {}
echo Done!
