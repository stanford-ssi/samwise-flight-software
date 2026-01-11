# Remove previous build directory if it exists
rm -rf build_tests

cmake -B build_tests -DPROFILE=TESTS -DCMAKE_BUILD_TYPE=Debug || return 1
cmake --build build_tests || return 1
cd build_tests || return 1
ctest --output-on-failure || (cd .. && return 1)
cd .. || return 1

# If you want to run individual tests with memory leak analysis:
# valgrind build_tests/src/filesys/test/filesys_test --leak-check=full
# Note that debug symbols are included, so gdb can be used

# For vscode, you can also add the following configuration to debug in ./.vscode/launch.json:
# "configurations": [
#     {
#         "name": "(gdb) Launch",
#         "type": "cppdbg",
#         "request": "launch",
#         "program": "${workspaceFolder}/build_tests/src/filesys/test/filesys_test",
#         "args": [],
#         "stopAtEntry": false,
#         "cwd": "${fileDirname}",
#         "environment": [],
#         "externalConsole": false,
#         "MIMode": "gdb",
#         "setupCommands": [
#             {
#                 "description": "Enable pretty-printing for gdb",
#                 "text": "-enable-pretty-printing",
#                 "ignoreFailures": true
#             },
#             {
#                 "description": "Set Disassembly Flavor to Intel",
#                 "text": "-gdb-set disassembly-flavor intel",
#                 "ignoreFailures": true
#             }
#         ]
#     },
# ]