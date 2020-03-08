#ifndef WB_INTERFACE_HPP
#define WB_INTERFACE_HPP
// Interface to send data over a Wishbone bus

#include <vector>
#include <stdint.h>

template<class DATA_T> class WbInterface
{
	public:
		virtual ~WbInterface() {};
		virtual void write(uintptr_t addr, typename std::vector<DATA_T>::iterator begin, typename std::vector<DATA_T>::iterator end) = 0;
		virtual std::vector<DATA_T> read(uintptr_t addr, size_t num) = 0;
};
#endif
