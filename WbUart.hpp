#ifndef WB_UART_HPP
#define WB_UART_HPP
// Class to talk to a wishbone over uart Interface
// Assumed to be 8n1 UART, baud configurable

#include <iostream>
#include <string>
#include <exception>

#include <boost/endian.hpp>

#include <boost/asio/serial_port.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>

#include "VectorUtility.h"

#include "WbInterface.hpp"

class WbUartException : public std::runtime_error
{
	using std::runtime_error::runtime_error;
};

template<class DATA_T, int ADDR_BITS> class WbUart : public WbInterface<DATA_T>
{
public:
	WbUart(std::string dev_path, uint32_t baud, bool debug_prints=false)
	:serial(io, dev_path), debug_prints(debug_prints)
	{
		serial.set_option(boost::asio::serial_port_base::baud_rate(baud));
		// Hardware flow control seems to be broken for the CH340 chips in the linux kernel driver :(
		//serial.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::type::hardware));

	};

	virtual void write(uintptr_t addr, AddressMode addr_mode, typename std::vector<DATA_T>::iterator begin, typename std::vector<DATA_T>::iterator end) override
	{
		auto cur_iter = begin;
		while(cur_iter != end)
		{
			auto next_iter = VectorUtility::chunk<DATA_T>(cur_iter, end, 255);
			auto len = next_iter-cur_iter;
			auto meta = format_transaction_metadata(true, len, addr, addr_mode);
			if(debug_prints)
			{
				std::cout << "(wr) Sending metadata: ";
				VectorUtility::print(meta);
			}
			boost::asio::write(serial, boost::asio::buffer(meta));

			auto transmit_data = data_to_uint8(cur_iter,next_iter);

			if(debug_prints)
			{
				std::cout << "(wr) Sending data: ";
				VectorUtility::print(transmit_data);
			}
			boost::asio::write(serial, boost::asio::buffer(transmit_data));

			cur_iter = next_iter;
		}
	};

	virtual std::vector<DATA_T> read(uintptr_t addr, AddressMode addr_mode, size_t num) override
	{

		std::vector<DATA_T> ret;
		ret.reserve(num);

		for(auto i=0u; i<num; i+=255)
		{
			auto next_inc = num-i < 255? num-i : 255;

			// Assume write to constant address
			auto packet = format_transaction_metadata(false, next_inc, addr, addr_mode);

			if(debug_prints)
			{
				std::cout << "(rd) Sending metadata: ";
				VectorUtility::print(packet);
			}
			boost::asio::write(serial, boost::asio::buffer(packet));

			// Get data back
			const size_t num_to_read = num*sizeof(DATA_T);
			if(debug_prints)
			{
				std::cout << "(rd) Trying to read:" << num_to_read << std::endl;
			}
			std::vector<uint8_t> buf(num_to_read);
			auto num_read = boost::asio::read(serial, boost::asio::buffer(buf));

			if(num_read != num_to_read)
			{
				throw WbUartException("Timed out when reading");
			}

			auto part = uint8_to_data(buf.begin(),buf.end());

			if(debug_prints)
			{
				std::cout << "(rd)Got packet. ";
				VectorUtility::print(part);
			}

			ret.insert(ret.end(), part.begin(), part.end());
		}
		return ret;
	};

private:
	boost::asio::io_context io;
	boost::asio::serial_port serial;
	bool debug_prints;


	std::vector<uint8_t> format_transaction_metadata(bool write, uint8_t count, uintptr_t addr, AddressMode addr_mode)
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
		uint8_t flags = write;
		if(addr_mode == AddressMode::INCREMENT)
		{
			flags |= 0x2;
		}
		ret.push_back(flags);

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

	std::vector<uint8_t> data_to_uint8(typename std::vector<DATA_T>::iterator begin, typename std::vector<DATA_T>::iterator end)
	{
		std::vector<uint8_t> data;
		data.reserve(((end-begin)*sizeof(DATA_T)));
		if(sizeof(DATA_T) == 1)
		{
			// We can bypass the complex logic for the special case of size 1
			std::copy(begin,end,std::back_inserter(data));
		} else {
			for(auto iter = begin; iter != end; iter++)
			{
				auto datum = boost::endian::big_to_native(*iter);
				for(auto i=0u; i<sizeof(datum); i++)
				{
					data.push_back(reinterpret_cast<uint8_t*>(&datum)[i]);
				}
			}
		}
		return data;
	}

	std::vector<DATA_T> uint8_to_data(typename std::vector<uint8_t>::iterator begin, typename std::vector<uint8_t>::iterator end)
	{
		std::vector<DATA_T> data;

		size_t size_data_words = (end-begin)/sizeof(DATA_T) + ((end-begin)%sizeof(DATA_T) != 0);
		data.reserve(size_data_words);
		if(sizeof(DATA_T) == 1)
		{
			// We can bypass the complex logic for the special case of size 1
			std::copy(begin,end,std::back_inserter(data));
		} else {
			auto iter = begin;
			while(iter != end)
			{
				DATA_T datum(0);
				for(auto i=0u; i<sizeof(DATA_T); i++)
				{
					datum |= (static_cast<DATA_T>(*iter) << 8*i);
					iter++;
					if(iter == end)
					{
						break;
					}
				}

				boost::endian::native_to_big_inplace(datum);
				data.push_back(datum);
			}
		}
		return data;
	}
};
#endif
