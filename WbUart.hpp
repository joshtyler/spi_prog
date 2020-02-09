#ifndef WB_UART_HPP
#define WB_UART_HPP
// Class to talk to a wishbone over uart Interface

#include <string>

template<class DATA_T> class WbUart : public WbInterface
{
	WbUart(std::string dev_path);
	virtual void send(uintptr_t addr, DATA_T data) override;
	virtual DATA_T receive(uintptr_t addr) override;
};
#endif
