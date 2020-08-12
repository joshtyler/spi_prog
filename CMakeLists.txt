cmake_minimum_required(VERSION 3.16)
project(spi_prog)

set(CMAKE_CXX_STANDARD 17)

include_directories(src)
include_directories(thirdparty/cxxopts/include)

add_subdirectory(thirdparty/libftdi)
# Not sure why this next line is needed, doesn't include_directories sort for us?
include_directories(thirdparty/libftdi/src)

set(Boost_USE_STATIC_LIBS ON)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost 1.65 REQUIRED COMPONENTS system)
message(STATUS "Boost version: ${Boost_VERSION}")
include_directories(${Boost_INCLUDE_DIRS})

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

target_link_libraries(spi_prog ftdi1 pthread ${Boost_LIBRARIES})
