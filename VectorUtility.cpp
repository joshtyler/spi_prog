#include "VectorUtility.h"


void VectorUtility::print(std::vector<uint8_t> data, bool printMeta)
{
	if(printMeta)
	{
		std::cout << "Size " << data.size() << ". Data(hex): ";
	}

	std::cout << std::hex;
	for(unsigned int i = 0; i < data.size(); i++)
	{
		if(i > 0)
		{
			std::cout << ", ";
		}

		std::cout << "0x" << (int)data[i];
	}

	#warning "Lazy preservation of flags, should preserve all"
	std::cout << std::dec << std::endl;
};
