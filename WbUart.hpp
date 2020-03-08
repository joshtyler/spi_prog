#ifndef WB_UART_HPP
#define WB_UART_HPP
// Class to talk to a wishbone over uart Interface
// Assumed to be 8n1 UART, baud configurable
// Uses this UART library https://github.com/wjwwood/serial

#warning "Super hacky ifdef for prints"
//#define DEBUG_PRINTS

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

static inline void print_vec(std::vector<uint8_t> in)
{
	std::cout << "Size " << std::dec << in.size() << ". Data: " << std::hex;
	for(auto datum : in)
		std::cout << "0x" << (int) datum << ", ";
	std::cout << std::dec << std::endl;
}

template<class DATA_T, int ADDR_BITS> class WbUart : public WbInterface<DATA_T>
{
public:
	WbUart(std::string dev_path, uint32_t baud)
	:serial(dev_path, baud, serial::Timeout::simpleTimeout(1000), serial::eightbits, serial::parity_none, serial::stopbits_one, serial::flowcontrol_hardware)//serial::Timeout::max()))
	{
		if(!serial.isOpen())
		{
			throw WbUartException("Could not open UART");
		}
	};

	virtual void write(uintptr_t addr, std::vector<DATA_T> data) override
	{
		auto packet = format_write_transaction(addr, data);
		#ifdef DEBUG_PRINTS
		std::cout << "(wr) Sending packet. ";
		print_vec(packet);
		#endif
		serial.write(packet);
	};

	virtual std::vector<DATA_T> read(uintptr_t addr, size_t num) override
	{

		std::vector<DATA_T> ret;

		for(auto i=0u; i<num; i+=255)
		{
			auto next_inc = num-i < 255? num-i : 255;

			// Assume write to constant address
			auto packet = format_transaction_metadata(false, next_inc, addr );

			#ifdef DEBUG_PRINTS
			std::cout << "(rd) Sending packet. ";
			print_vec(packet);
			#endif
			serial.write(packet);

			// Get data back
			std::vector<uint8_t> buf;
			auto num_read = serial.read(buf, num*sizeof(DATA_T));

			if(num_read != num*sizeof(DATA_T))
			{
				throw WbUartException("Timed out when reading");
			}


			auto part = data_to_uint8_t(buf);

			#ifdef DEBUG_PRINTS
			std::cout << "(rd)Got packet. ";
			print_vec(part);
			#endif
			ret.insert(ret.end(), part.begin(), part.end());
		}
		return ret;
	};

private:
	serial::Serial serial;

	std::vector<std::vector<uint8_t>> split_vector(std::vector<uint8_t> in)
	{
		constexpr unsigned int size = 255;

		std::vector<std::vector<uint8_t>> ret;

		unsigned int num = in.size()/size + (in.size()%size != 0);

		for(auto i=0u; i<num; i++)
		{
			unsigned int cur_size = (i == num-1)? (in.size()%size? in.size()%size : 255 ) : size;
			ret.push_back(std::vector<uint8_t>(in.begin()+i*size,in.begin()+(i*size)+cur_size));
		}
		return ret;
	}

	std::vector<uint8_t> format_transaction_metadata(bool write, uint8_t count, uintptr_t addr)
	{
		// First bit is !r/w
		// Then address in big endian format (assume we are doing constant address access if we have to split)
		// Then one byte of count
		// Then data verbatim for tx, or nothing for rx

		// This probably isn't the most efficient way of doing it
		// Many of these things are constants we know at compile time
		// At the very least we could preallocate the vector
		// But I'm tired
		// And the data is going over a UART, so really who cares...
		// If the answer turns out to be "me", I'll refactor

		std::vector<uint8_t> ret;

		// Operation
		ret.push_back(write);

		// Addr
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

		// Size
		ret.push_back(count);

		return ret;

	}

	std::vector<uint8_t> data_to_uint8_t(std::vector<DATA_T> data)
	{
		std::vector<uint8_t> data_u8;
		if(sizeof(DATA_T) != 1)
		{
			for(auto datum : data)
			{
				boost::endian::native_to_big_inplace(datum);
				for(auto i=0u; i<sizeof(datum); i++)
				{
					data_u8.push_back(reinterpret_cast<uint8_t*>(&datum)[i]);
				}
			}
		} else {
			data_u8 =  data;
		}
		return data_u8;
	}

	std::vector<uint8_t> format_write_transaction(uintptr_t addr, std::vector<DATA_T> data)
	{
		std::vector<uint8_t> ret;
		ret.reserve(data.size()*sizeof(DATA_T));

		auto data_split = split_vector(data_to_uint8_t(data));
		//std::cout << data_split.size() << std::endl;
		for(auto part : data_split)
		{
			//std::cout << part.size() << std::endl;

			auto meta = format_transaction_metadata(true, part.size(), addr);
			ret.insert(ret.end(), meta.begin(), meta.end());

			// Data
			ret.insert(ret.end(), part.begin(), part.end());
		}

		return ret;

	}
};
#endif
