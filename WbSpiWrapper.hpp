#ifndef WB_SPI_WRAPPER_HPP
#define WB_SPI_WRAPPER_HPP

// Class to talk to the Wishbone SPI peripheral

// Register map:
// 0x00 : Reserved
// 0x01 : Config
//      : Bit 0 : CS
//      : Bit 1 : Discard RX
// 0x02 : data
// 0x03 : Inject bytes

#include "WbInterface.hpp"
#include "SpiInterface.hpp"

class WbSpiWrapper : public SpiInterface
{
	public:
		WbSpiWrapper(WbInterface<uint8_t> *iface, uintptr_t base_addr);

		std::vector<uint8_t> transfer(std::vector<uint8_t> data) override;
		void setCs(bool val) override;
		void send(std::vector<uint8_t> data) override;
		std::vector<uint8_t> receive(int num) override;


	private:
		WbInterface<uint8_t> *iface;

		// Temporary
		// Copied and pasted from WbUart.hpp
		std::vector<std::vector<uint8_t>> split_vector(std::vector<uint8_t> in);

};
#endif
