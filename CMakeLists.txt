cmake_minimum_required(VERSION 3.16)
project(spi_prog)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)

add_executable(spi_prog
        src/FileUtility.cpp
        src/FileUtility.h
        src/ParseUtility.cpp
        src/ParseUtility.h
        src/spi_prog.cpp
        src/SpiFlash.cpp
        src/SpiFlash.hpp
        src/SpiInterface.hpp
        src/SpiWrapper.cpp
        src/SpiWrapper.hpp
        src/VectorUtility.h
        src/WbInterface.hpp
        src/WbSpiWrapper.cpp
        src/WbSpiWrapper.hpp
        src/WbUart.hpp)

add_subdirectory(thirdparty/libftdi)
target_link_libraries(spi_prog ftdi1 pthread)
