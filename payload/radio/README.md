# 2400 radio files
Modified from `sx128x_linux_driver` project. Linux driver for Semtech SX128x series.

## Building
To build the executables (`Lora_[rx|tx]`) run:
```
mkdir build
cd build

sudo cmake ..
sudo make -j4
```

This project is part of the **STM32 Sucks!** project.

## Requirements
- C++17
- CMake 3.14+
- Linux kernel 4.8+

## Dependencies
They will be automatically installed by CPM.
- [sx128x_driver](https://github.com/YukiWorkshop/sx128x_driver)
- [GPIOPlusPlus](https://github.com/YukiWorkshop/GPIOPlusPlus)
- [SPPI](https://github.com/YukiWorkshop/SPPI)

## Example
See `Test.cpp`

## In action
![image](https://user-images.githubusercontent.com/34613827/97797249-8b041d80-1c56-11eb-9618-3c05292d82d3.png)

![image](https://user-images.githubusercontent.com/34613827/97797279-bedf4300-1c56-11eb-96dd-e6801185be53.png)


## License
LGPLv3
