# Add submodule libraries (like pico-sdk)
git submodule update --init
(cd ./pico-sdk && git submodule update --init) # For TinyUSB submodule for USB support

# For pico sdk
mkdir -p $HOME/.pico-sdk/cmake/
touch $HOME/.pico-sdk/cmake/pico-vscode.cmake

# Add directory for build outputs
mkdir ./build

# pre-commit hook for formatting
ln -sf ../../scripts/pre-commit-hook.sh ./.git/hooks/pre-commit

# NOTE: YOU MUST ALSO ADD THE: Arm GNU Toolchain (arm-none-eabi-gcc)
#
# On mac:
# brew install --cask gcc-arm-embedded
