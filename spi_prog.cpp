// Simple SPI Programming tool

#include "SpiWrapper.hpp"
#include "EepromProg.hpp"

#include <fstream>
#include <iostream>

#include "ftdi.h"

void print_data(std::vector<uint8_t> data)
{
	for(unsigned int i = 0; i < data.size(); i++)
	{
		if(i > 0)
		{
			std::cout << ", ";
		}

		std::cout << std::hex << (int)data[i] << std::dec;
	}

	std::cout << std::endl;
}

int main(int argc, char* argv[])
{
	if(argc != 2)
	{
		std::cerr << "Usage: spi_prog [binary_file]" << std::endl;
		return 1;
	}

	std::ifstream is;
    std::vector<uint8_t> dataIn;

    is.open(argv[1], std::ios::binary);
    is.seekg(0, std::ios::end);
    size_t filesize=is.tellg();
    is.seekg(0, std::ios::beg);

    dataIn.resize(filesize/sizeof(uint8_t));

    is.read((char *)dataIn.data(), filesize);

//SpiWrapper spi("i:0x0403:0x6014", INTERFACE_A);
SpiWrapper spi("i:0x0403:0x6010", INTERFACE_A);

	EepromProg prog(spi);

	std::cout << "Read ID" << std::endl;
	std::vector<uint8_t> data = prog.readId();
	std::cout << "Received ID (len " << std::dec << data.size() << "): ";
	print_data(data);

	prog.program(0, dataIn);

	std::cout << "Read from 0" << std::endl;

	typeof(dataIn) dataOut;
	dataOut = prog.read(0,dataIn.size());

	if(dataOut == dataIn)
	{
		std::cout << "Data verified correctly" << std::endl;
	} else {
		std::cout << "WARNING: Verifcation error" << std::endl;
		return 1;
	}

	/*
	std::cout << "Sent : ";
	print_data(dataIn);
	std::cout << "Received : ";
	print_data(dataOut);
	*/
	return 0;
}
