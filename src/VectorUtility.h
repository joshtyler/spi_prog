#ifndef VECTOR_UTILITY_H
#define VECTOR_UTILITY_H

#include <iostream>
#include <iomanip>
#include <vector>
#include <iterator>
#include <string>

namespace VectorUtility
{

	template<typename DATA_T> void print(std::vector<DATA_T> data, bool printMeta=false)
	{
		std::ios_base::fmtflags flags(std::cout.flags());

		if(printMeta)
		{
			std::cout << "Size " << data.size() << ". Data(hex): ";
		}

		std::cout << std::hex;
		for(unsigned int i = 0; i < data.size(); i++)
		{
			if(i > 0)
			{
				std::cout << ",";
			}

			std::cout << "0x" << std::setfill('0') << std::setw(sizeof(DATA_T)*2) << static_cast<uint64_t>(data[i]);
		}

		std::cout.flags(flags);
	}

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


};

#endif
