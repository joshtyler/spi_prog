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
			// For now we will just do the really naive thing of one byte at a time
			// Not a software limitation, but will help with firmware bringup
			iface->write(1,{0}); // CS low
			for(auto datum : data)
			{
				iface->write(2,{datum});
				auto temp = iface->read(2,1);
				ret.insert(ret.end(), temp.begin(), temp.end());
			}
			iface->write(1,{1}); // CS high
			return ret;
		}
	private:
		WbInterface<DATA_T> *iface;

};
#endif
