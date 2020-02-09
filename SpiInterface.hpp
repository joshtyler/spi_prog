#ifndef SPI_INTERFACE_HPP
#define SPI_INTERFACE_HPP
// Interface for SPI
// For now just support transfer
// But this could be split into send, receive, and transferred
// This would give performance increase if you didn't care about the result when sending

class SpiInterface
{
public:
	virtual ~SpiInterface() {};
	virtual std::vector<uint8_t> xferSpi(std::vector<uint8_t> data) = 0;
};

#endif
