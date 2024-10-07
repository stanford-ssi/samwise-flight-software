# Helper script to recursively clang-format every file
echo Formatting all files...
find src -name "*.cpp" -o -name "*.c" -o -name "*.h" | xargs -I {} clang-format -i {}
echo Done!
