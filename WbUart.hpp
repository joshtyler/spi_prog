#ifndef WB_UART_HPP
#define WB_UART_HPP
// Class to talk to a wishbone over uart Interface
// Assumed to be 8n1 UART, baud configurable
// Uses this UART library https://github.com/wjwwood/serial

#include <iostream>
#include <string>
#include <exception>

#include <boost/endian.hpp>

#include <serial/serial.h> //https://github.com/wjwwood/serial

#include "WbInterface.hpp"

class WbUartException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};


template<class DATA_T, int ADDR_BITS> class WbUart : public WbInterface<DATA_T>
{
public:
	WbUart(std::string dev_path, uint32_t baud)
	:serial(dev_path, baud, serial::Timeout::simpleTimeout(serial::Timeout::max()))
	{
		if(!serial.isOpen())
		{
			throw WbUartException("Could not open UART");
		}
	};

	virtual void write(uintptr_t addr, std::vector<DATA_T> data) override
	{
		auto packet = format_transaction(true, addr, data);
//		std::cout << "(wr) Sending packet : ";
//		for(auto datum : packet)
//			std::cout << (int) datum << ", ";
//		std::cout << std::endl;

		serial.write(packet);
	};

	virtual std::vector<DATA_T> read(uintptr_t addr, size_t num) override
	{
		// Slight hack constructing an empty vector just to avoid a separate function
		auto packet = format_transaction(false, addr, std::vector<DATA_T>());
//		std::cout << "(rd) Sending packet : ";
//		for(auto datum : packet)
//			std::cout << (int) datum << ", ";
//		std::cout << std::endl;
		serial.write(packet);

		std::vector<uint8_t> buf;
		auto num_read = serial.read(buf, num*sizeof(DATA_T));

		if(num_read != num*sizeof(DATA_T))
		{
			throw WbUartException("Timed out when reading");
		}

		std::vector<DATA_T> ret;
		if(sizeof(DATA_T) == 1)
		{
			ret = buf;
		} else {
			for(auto datum : buf)
			{
				boost::endian::big_to_native_inplace(datum);
				for(auto i=0u; i<sizeof(datum); i++)
				{
					ret.push_back(reinterpret_cast<uint8_t*>(&datum)[i]);
				}
			}
		}
//		std::cout << "(rd)Got packet of size " << ret.size() << " (data[0]= " << (int)ret[0] << ")" << std::endl;
		return ret;
	};

private:
	serial::Serial serial;

	std::vector<uint8_t> format_transaction(bool write, uintptr_t addr, std::vector<DATA_T> data)
	{
		// First bit is !r/w
		// Then address in big endian format
		// Then data verbatim for tx, or nothing for tx

		// This probably isn't the most efficient way of doing it
		// Many of these things are constants we know at compile time
		// At the very least we could preallocate the vector
		// But I'm tired
		// And the data is going over a UART, so really who cares...
		// If the answer turns out to be "me", I'll refactor

		std::vector<uint8_t> ret;

		ret.push_back(write);

		boost::endian::native_to_big_inplace(addr);

		//Only send enough bytes to store the address

		// This is ceil() for integers
		constexpr unsigned int ADDR_BYTES = ADDR_BITS/8 + (ADDR_BITS%8 != 0);
		constexpr unsigned int START_IDX = sizeof(addr)-ADDR_BYTES;

		for(auto i=START_IDX; i< sizeof(addr); i++)
		{
			//N.B. No endianness conversion
			ret.push_back(reinterpret_cast<uint8_t*>(&addr)[i]);
		}

		for(auto datum : data)
		{
			boost::endian::native_to_big_inplace(datum);
			for(auto i=0u; i<sizeof(datum); i++)
			{
				ret.push_back(reinterpret_cast<uint8_t*>(&datum)[i]);
			}
		}

		return ret;

	}
};
#endif
