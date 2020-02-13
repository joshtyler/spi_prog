#ifndef WB_INTERFACE_HPP
#define WB_INTERFACE_HPP
// Interface to send data over a Wishbone bus

template<class DATA_T> class WbInterface
{
	public:
		virtual ~WbInterface() {};
		virtual void write(uintptr_t addr, std::vector<DATA_T> data) = 0;
		virtual std::vector<DATA_T> read(uintptr_t addr, size_t num) = 0;
};
#endif
