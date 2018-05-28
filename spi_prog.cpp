// Simple SPI Programming tool

#include "SpiWrapper.hpp"
#include "EepromProg.hpp"

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

int main(void)
{
	SpiWrapper spi("i:0x0403:0x6014", INTERFACE_A);

	EepromProg prog(spi);

	std::cout << "Read ID" << std::endl;
	std::vector<uint8_t> data = prog.readId();
	std::cout << "Received ID (len " << std::dec << data.size() << "): ";
	print_data(data);

	std::cout << "Sector erase" << std::endl;
	prog.sectorErase(0);
	
	std::cout << "Read from 0" << std::endl;
	data = prog.read(0,1);
	std::cout << "Received : ";
	print_data(data);

	std::cout << "Write to 0" << std::endl;
	prog.write(0,75);

	std::cout << "Read from 0" << std::endl;
	data = prog.read(0,1);
	std::cout << "Received : " ;
	print_data(data);

	return 0;
}

