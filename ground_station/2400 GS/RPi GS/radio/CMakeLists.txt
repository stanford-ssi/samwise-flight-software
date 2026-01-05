cmake_minimum_required(VERSION 3.14)
project(sx128x_linux_driver)

set(CMAKE_CXX_STANDARD 17)

set(CPM_DOWNLOAD_VERSION 0.27.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

if(NOT (EXISTS ${CPM_DOWNLOAD_LOCATION}))
    message(STATUS "Downloading CPM.cmake")
    file(DOWNLOAD https://github.com/TheLartians/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake ${CPM_DOWNLOAD_LOCATION})
endif()

include(${CPM_DOWNLOAD_LOCATION})

CPMAddPackage(
        NAME sx128x_driver
        GITHUB_REPOSITORY YukiWorkshop/sx128x_driver
        VERSION 0.0.4
)

CPMAddPackage(
        NAME GPIOPlusPlus
        GITHUB_REPOSITORY YukiWorkshop/GPIOPlusPlus
        VERSION 0.0.2
)

CPMAddPackage(
        NAME SPPI
        GITHUB_REPOSITORY YukiWorkshop/SPPI
        VERSION 0.0.5
)

add_library(sx128x_linux_driver SX128x_Linux.cpp SX128x_Linux.hpp)
target_include_directories(sx128x_linux_driver PUBLIC ${sx128x_driver_SOURCE_DIR} ${GPIOPlusPlus_SOURCE_DIR} ${SPPI_SOURCE_DIR})
target_link_libraries(sx128x_linux_driver pthread GPIOPlusPlus SPPI sx128x_driver)

# Build tx file
add_executable(Lora_tx Lora_tx.cpp)
target_link_libraries(Lora_tx sx128x_linux_driver)

# Build rx file
add_executable(Lora_rx Lora_rx.cpp)
target_link_libraries(Lora_rx sx128x_linux_driver)

# Build test file
add_executable(Test Test.cpp)
target_link_libraries(Test sx128x_linux_driver)