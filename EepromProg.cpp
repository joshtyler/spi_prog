#include "EepromProg.hpp"
#include <vector>
#include <iostream>
#include <unistd.h>

std::vector<uint8_t> EepromProg::read(int addr, int num)
{
	waitUntilReady();

	std::vector<uint8_t> transmit(num+4, 0xFF);
	transmit[0] = static_cast<uint8_t>(SpiCmd::read);
	transmit[1] = (addr >> 16) & 0xFF;
	transmit[2] = (addr >> 8)  & 0xFF;
	transmit[3] = (addr >> 0)  & 0xFF;

	std::vector<uint8_t> result = spi.xferSpi(transmit);

	// Remove first 4 elements as this is command and address
	result.erase(result.begin(), result.begin() + 4);

	return result;
}


std::vector<uint8_t> EepromProg::readId(void)
{
	waitUntilReady();

	std::vector<uint8_t> transmit(10, 0xFF);
	transmit[0] = static_cast<uint8_t>(SpiCmd::readId);

	std::vector<uint8_t> result = spi.xferSpi(transmit);

	// Remove first element as this is the ID
	result.erase(result.begin(), result.begin()+1);

	return result;
}

// Writes in page program mode
// Takes iterator to first byte to Program
// Returns iterator to last byte programmed
void EepromProg::write(int addr, std::vector<uint8_t>::iterator start, std::vector<uint8_t>::iterator end)
{
	if(std::distance(start,end) > pageSize)
	{
		throw EepromException("Attempt to write more than page size: " + std::to_string(std::distance(start,end)));
	}

	waitUntilReady();
	enableWriting();

	waitUntilReady();
	std::vector<uint8_t> transmit(4);
	transmit[0] = static_cast<uint8_t>(SpiCmd::byteProgram);
	transmit[1] = (addr >> 16) & 0xFF;
	transmit[2] = (addr >> 8)  & 0xFF;
	transmit[3] = (addr >> 0)  & 0xFF;
	for(auto it=start; it != end; it++)
	{
		transmit.push_back(*it);
	}

	std::cout << "Write to " << addr << ". Size: " << (transmit.size()-4) << std::endl;

	spi.xferSpi(transmit);
}

void EepromProg::chipErase(void)
{
	waitUntilReady();
	enableWriting();

	std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::chipErase)};

	spi.xferSpi(transmit);
}



uint8_t EepromProg::readStatusRegister(void)
{
	std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::readStatusRegister), 0xFF};

	std::vector<uint8_t> result = spi.xferSpi(transmit);

	return result[1];
}

void EepromProg::waitUntilReady(void)
{
	uint8_t busy;
	do
	{
		busy = readStatusRegister();
		//std::cout << "Waiting: " << std::hex << (int)busy << std::dec << std::endl;

		busy = busy & 0x01;
	} while (busy);
}

void EepromProg::enableWriting(void)
{
	waitUntilReady();

	uint8_t status = readStatusRegister();
	// Check for block write protection
	if((status & 0x0C) != 0)
	{
		{
			std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::enableWriteStatusRegister)};
			spi.xferSpi(transmit);
		}

		{
			std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::writeStatusRegister), 0x00};
			spi.xferSpi(transmit);
		}
	}

	// Check for write enable latch
	if((status & 0x1) == 0)
	{
		std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::writeEnable)};
		spi.xferSpi(transmit);
	}
}

void EepromProg::sectorErase(int addr)
{
	waitUntilReady();
	enableWriting();
	readStatusRegister(); //Debug

	std::vector<uint8_t> transmit(4, 0xFF);
	transmit[0] = static_cast<uint8_t>(SpiCmd::sectorErase);
	transmit[1] = (addr >> 16) & 0xFF;
	transmit[2] = (addr >> 8)  & 0xFF;
	transmit[3] = (addr >> 0)  & 0xFF;

	spi.xferSpi(transmit);

	//sleep(4);

	waitUntilReady();
}

void EepromProg::program(int addr, std::vector<uint8_t> data)
{
	// Ensure address is aligned with sector size
	if((addr % sectorSize) != 0)
	{
		throw EepromException("Address not aligned with sector size");
	}

	int eraseEnd = data.size();
	if((eraseEnd % sectorSize) != 0)
	{
		std::cerr << "Warning. Length not aligned with sector size. Data at end of sector will be erased" << std::endl;
		eraseEnd += (eraseEnd % sectorSize);
	}

	//Erase necessary data
	for(int i= addr; i < eraseEnd; i+=sectorSize)
	{
		std::cout << "Erasing sector at " + std::to_string(i) << std::endl;
		sectorErase(i);
	}

	//Program in pages
	auto start = data.begin();
	typeof(start) end;
	do {
		if(distance(start,data.end()) < pageSize)
		{
			end = data.end();
		} else {
			end = start + (pageSize);
		}
		write(addr, start, end);
		addr = addr + pageSize;
		start = start + pageSize;
	} while(end != data.end());
}
