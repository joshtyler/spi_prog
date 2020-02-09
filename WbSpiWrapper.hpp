#ifndef WB_SPI_WRAPPER_HPP
#define WB_SPI_WRAPPER_HPP

// Class to talk to the Wishbone SPI peripheral

// Register map:
// 0x00 : Flags
// 0x01 : Fifo depth (log2)
// 0x01 : Receive count
// 0x02 : Send dataIn
// 0x03 : receive data

class WbSpiWrapper : public SpiInterface
{
	public:
		WbSpiWrapper(WbInterface if, uintptr_t base_addr);
		std::vector<uint8_t> xferSpi(std::vector<uint8_t> data) override;

};
#endif
