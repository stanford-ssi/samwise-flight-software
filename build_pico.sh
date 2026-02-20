# Remove previous build directory if it exists
rm -rf build_pico

cmake -B build_pico -DPROFILE=PICO || return 1
cmake --build build_pico --parallel || return 1
