#ifndef SPI_INTERFACE_HPP
#define SPI_INTERFACE_HPP

// Interface for SPI

class SpiInterface
{
public:
	virtual ~SpiInterface() {};
	// Transfer an array
	virtual std::vector<uint8_t> transfer(std::vector<uint8_t> data) = 0;
	// Send an array - discard returned data
	virtual void send(std::vector<uint8_t> data) = 0;
	// Receive an array - send dummy data
	virtual std::vector<uint8_t> receive(int num) = 0;
	// Assert or de-assert CS manually
	virtual void setCs(bool val) = 0;

};

#endif
