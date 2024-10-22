# Add submodule libraries (like pico-sdk)
git submodule update --init

# For pico sdk
mkdir -p $HOME/.pico-sdk/cmake/
touch $HOME/.pico-sdk/cmake/pico-vscode.cmake

