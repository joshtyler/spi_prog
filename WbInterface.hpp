#ifndef WB_INTERFACE_HPP
#define WB_INTERFACE_HPP
// Interface to send data over a Wishbone bus

template<class DATA_T> class WbInterface
{
	public:
		virtual ~WbInterface() {};
		virtual void send(uintptr_t addr, DATA_T data) = 0;
		virtual DATA_T receive(uintptr_t addr) = 0;
};
#endif
