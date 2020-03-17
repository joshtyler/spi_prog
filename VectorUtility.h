#ifndef VECTOR_UTILITY_H
#define VECTOR_UTILITY_H

#include <iostream>
#include <vector>
#include <iterator>
#include <string>

namespace VectorUtility
{

	void print(std::vector<uint8_t> data, bool printMeta=false);

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
