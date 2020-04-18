#ifndef WB_INTERFACE_HPP
#define WB_INTERFACE_HPP
// Interface to send data over a Wishbone bus

#include <vector>
#include <stdint.h>

enum class AddressMode
{
	FIXED,
	INCREMENT
};

template<class DATA_T> class WbInterface
{
	public:
		virtual ~WbInterface() {};
		virtual void write(uintptr_t addr, AddressMode addr_mode, typename std::vector<DATA_T>::iterator begin, typename std::vector<DATA_T>::iterator end) = 0;
		virtual std::vector<DATA_T> read(uintptr_t addr, AddressMode addr_mode, size_t num) = 0;
};
#endif
