#include "SpiFlash.hpp"
#include <vector>
#include <iostream>
#include <unistd.h>

#include <boost/version.hpp>
// io_service changed to io_context in 1.66
#if (((BOOST_VERSION / 100000) == 1) && (BOOST_VERSION / 100 % 1000) >= 72)
	#include<boost/timer/progress_display.hpp>
	typedef boost::timer::progress_display display_t;
#else
	#include<boost/progress.hpp>
	typedef boost::progress_display display_t;
#endif

std::vector<uint8_t> SpiFlash::read(int addr, int num)
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


std::vector<uint8_t> SpiFlash::readId(void)
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

void SpiFlash::releasePowerDown(void)
{
	std::vector<uint8_t> transmit = {0xAB,0xFF,0xFF,0xFF,0xFF};
	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);
}

// Writes in page program mode
// Takes iterator to first byte to Program
// Returns iterator to last byte programmed
void SpiFlash::write(int addr, std::vector<uint8_t>::iterator start, std::vector<uint8_t>::iterator end)
{
	if(std::distance(start,end) > pageSize)
	{
		throw SpiFlashException("Attempt to write more than page size: " + std::to_string(std::distance(start,end)));
	}
	enableWriting();

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

void SpiFlash::chipErase(void)
{
	waitUntilReady();
	enableWriting();

	std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::chipErase)};

	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);
}



uint8_t SpiFlash::readStatusRegister(int reg)
{
	SpiCmd cmd;
	switch(reg)
	{
		case 1 : cmd = SpiCmd::readStatusRegister1; break;
		case 2 : cmd = SpiCmd::readStatusRegister2; break;
		case 3 : cmd = SpiCmd::readStatusRegister3; break;
		default : throw SpiFlashException("Attempt to read invalid status register");
	}

	std::vector<uint8_t> transmit = {static_cast<uint8_t>(cmd), 0xFF};

	spi->setCs(false);
	std::vector<uint8_t> result = spi->transfer(transmit);
	spi->setCs(true);

	return result[1];
}

void SpiFlash::waitUntilReady(void)
{
	uint8_t busy;
	do
	{
		busy = readStatusRegister();
		//std::cout << "Waiting: " << std::hex << (int)busy << std::dec << std::endl;

		busy = busy & 0x01;
	} while (busy);
}

void SpiFlash::checkAndDisableWriteProection(void)
{
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
}

void SpiFlash::enableWriting(void)
{

	std::vector<uint8_t> transmit = {static_cast<uint8_t>(SpiCmd::writeEnable)};
	spi->setCs(false);
	spi->send(transmit);
	spi->setCs(true);
}

void SpiFlash::sectorErase(int addr)
{
	waitUntilReady();
	enableWriting();

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

void SpiFlash::program(int addr, std::vector<uint8_t> data)
{
	// Ensure address is aligned with sector size
	if((addr % sectorSize) != 0)
	{
		throw SpiFlashException("Address not aligned with sector size");
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
	waitUntilReady();

	//Program in pages
	unsigned long expectedCount = data.size()/pageSize;
	display_t show_progress(expectedCount, std::cerr,"");
	auto start = data.begin();
	decltype(start) end;
	do {
		if(distance(start,data.end()) < pageSize)
		{
			end = data.end();
		} else {
			end = start + (pageSize);
		}
		waitUntilReady();
		write(addr, start, end);
		++show_progress;
		addr = addr + pageSize;
		start = start + pageSize;
	} while(end != data.end());
}
