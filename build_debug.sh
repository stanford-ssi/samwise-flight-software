# Remove previous build directory if it exists
rm -rf build

cmake -B build -DPROFILE=PICUBED-DEBUG || return 1
cmake --build build --parallel || return 1
