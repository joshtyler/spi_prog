#include "EepromProg.hpp"
#include <vector>
#include <iostream>
#include <unistd.h>
#include<boost/timer/progress_display.hpp>

std::vector<uint8_t> EepromProg::read(int addr, int num)
{
	waitUntilReady();

	spi->setCs(false);
	std::vector<uint8_t> transmit(4, 0xFF);
	transmit[0] = static_cast<uint8_t>(SpiCmd::read);
	transmit[1] = (addr >> 16) & 0xFF;
	transmit[2] = (addr >> 8)  & 0xFF;
	transmit[3] = (addr >> 0)  & 0xFF;

	spi->send(transmit);

	auto ret = spi->receive(num);
	spi->setCs(true);

	return ret;
}


std::vector<uint8_t> EepromProg::readId(void)
{
	waitUntilReady();

	std::vector<uint8_t> transmit(10, 0xFF);
	transmit[0] = static_cast<uint8_t>(SpiCmd::readId);

	spi->setCs(false);
	std::vector<uint8_t> result = spi->transfer(transmit);
	spi->setCs(true);

	// Remove first element as this is the ID
	result.erase(result.begin(), result.begin()+1);

	return result;
}

void EepromProg::releasePowerDown(void)
{
	std::vector<uint8_t> transmit = {0xAB,0xFF,0xFF,0xFF,0xFF};
	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);
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

	//std::cout << "Write to " << addr << ". Size: " << (transmit.size()-4) << std::endl;

	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);
}

void EepromProg::chipErase(void)
{
	waitUntilReady();
	enableWriting();

	std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::chipErase)};

	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);
}



uint8_t EepromProg::readStatusRegister(int reg)
{
	SpiCmd cmd;
	switch(reg)
	{
		case 1 : cmd = SpiCmd::readStatusRegister1; break;
		case 2 : cmd = SpiCmd::readStatusRegister2; break;
		case 3 : cmd = SpiCmd::readStatusRegister3; break;
		default : throw EepromException("Attempt to read invalid status register");
	}

	std::vector<uint8_t> transmit = {static_cast<uint8_t>(cmd), 0xFF};

	spi->setCs(false);
	std::vector<uint8_t> result = spi->transfer(transmit);
	spi->setCs(true);

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
			spi->setCs(false);
			spi->send(transmit);
			spi->setCs(true);
		}

		{
			std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::writeStatusRegister), 0x00};
			spi->setCs(false);
			spi->send(transmit);
			spi->setCs(true);
		}
	}

	// Check for write enable latch
	if((status & 0x1) == 0)
	{
		std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::writeEnable)};
		spi->setCs(false);
		spi->send(transmit);
		spi->setCs(true);
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

	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);

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

	int eraseEnd = addr+data.size();
	if((eraseEnd % sectorSize) != 0)
	{
		std::cerr << "Warning. Length not aligned with sector size. Data at end of sector will be erased" << std::endl;
		eraseEnd += (eraseEnd % sectorSize);
	}

	//Erase necessary data
	for(int i= addr; i < eraseEnd; i+=sectorSize)
	{
		std::cout << "Erasing sector at 0x" << std::hex << i << std::dec << std::endl;
		sectorErase(i);
	}

	//Program in pages
	unsigned long expectedCount = data.size()/pageSize;
	boost::timer::progress_display show_progress(expectedCount, std::cerr,"");
	auto start = data.begin();
	decltype(start) end;
	do {
		if(distance(start,data.end()) < pageSize)
		{
			end = data.end();
		} else {
			end = start + (pageSize);
		}
		write(addr, start, end);
		++show_progress;
		addr = addr + pageSize;
		start = start + pageSize;
	} while(end != data.end());
}
