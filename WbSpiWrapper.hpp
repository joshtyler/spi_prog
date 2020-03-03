#ifndef WB_SPI_WRAPPER_HPP
#define WB_SPI_WRAPPER_HPP

// Class to talk to the Wishbone SPI peripheral

// Register map:
// 0x00 : Reserved
// 0x01 : Config
//      : Bit 0 : CS
// 0x01 : Receive count
// 0x02 : data

#include "WbInterface.hpp"
#include "SpiInterface.hpp"

template<class DATA_T> class WbSpiWrapper : public SpiInterface
{
	public:
		WbSpiWrapper(WbInterface<DATA_T> *iface, uintptr_t base_addr)
		:iface(iface)
		{

		};
		std::vector<uint8_t> xferSpi(std::vector<uint8_t> data) override
		{
			std::vector<uint8_t> ret;
			iface->write(1,{0}); // CS low
			// For now, do 255 at a time
			#warning "This is a stupid place to split it. The splitting logic is all broken"
			// If a larger vector is passed into write, it will just try and write too much (e.g. a 258 sized vector)
			// Splitting to 252 is a hack that gives us leg room for the three byte header
			auto vecs = split_vector(data);
			for(auto vec : vecs)
			{
				iface->write(2,vec);
				auto temp = iface->read(2,vec.size());
				ret.insert(ret.end(), temp.begin(), temp.end());
			}
			iface->write(1,{1}); // CS high
			return ret;
		}
	private:
		WbInterface<DATA_T> *iface;

		// Temporary
		// Copied and pasted from WbUart.hpp
		std::vector<std::vector<uint8_t>> split_vector(std::vector<uint8_t> in)
		{
			constexpr unsigned int size = 252;

			std::vector<std::vector<uint8_t>> ret;

			unsigned int num = in.size()/size + (in.size()%size != 0);

			for(auto i=0u; i<num; i++)
			{
				unsigned int cur_size = (i == num-1)? (in.size()%size? in.size()%size : 255 ) : size;
				ret.push_back(std::vector<uint8_t>(in.begin()+i*size,in.begin()+(i*size)+cur_size));
			}
			return ret;
		}

};
#endif
