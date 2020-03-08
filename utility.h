#ifndef UTILITY_H
#define UTILITY_H

#include <iostream>
#include <vector>
#include <iterator>

static inline void print_data(std::vector<uint8_t> data)
{
	std::cout << std::hex;
	for(unsigned int i = 0; i < data.size(); i++)
	{
		if(i > 0)
		{
			std::cout << ", ";
		}

		std::cout << (int)data[i];
	}

	std::cout << std::dec << std::endl;
};

static inline void print_vec(std::vector<uint8_t> in)
{
	std::cout << "Size " << in.size() << ". Data(hex): ";
	print_data(in);
	std::cout  << std::endl;
};

//template<typename DATA_T> (typename std::vector<DATA_T>::iterator) chunk(std::vector<DATA_T>::iterator start, std::vector<DATA_T>::iterator end, size_t size)
template<typename DATA_T> typename std::vector<DATA_T>::iterator chunk(typename std::vector<DATA_T>::iterator start, typename std::vector<DATA_T>::iterator end, int size)
{
	if(end-start > size)
	{
		return start+size;
	} else {
		return end;
	}
};

#endif
