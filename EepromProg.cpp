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

void EepromProg::write(int addr, uint8_t data)
{
	waitUntilReady();
	enableWriting();

	waitUntilReady();
	std::vector<uint8_t> transmit(5);
	transmit[0] = static_cast<uint8_t>(SpiCmd::byteProgram);
	transmit[1] = (addr >> 16) & 0xFF;
	transmit[2] = (addr >> 8)  & 0xFF;
	transmit[3] = (addr >> 0)  & 0xFF;
	transmit[4] = data;

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
