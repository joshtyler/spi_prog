
#include <string>
#include <algorithm>

#include "ParseUtility.h"

	std::optional<size_t> ParseUtility::findNonDigit(std::string str)
	{
		for(auto i=0; i < (int)str.size(); i++)
		{
				if(not (isdigit(str[i]) or str[i] == '.'))
				{
					return i;
				}
		}
		return std::nullopt;
	}

	std::string ParseUtility::toLower(std::string in)
	{
		std::transform(in.begin(), in.end(), in.begin(), [](unsigned char c){return std::tolower(c);});
		return in;
	}

	std::string ParseUtility::toUpper(std::string in)
	{
		std::transform(in.begin(), in.end(), in.begin(), [](unsigned char c){return std::tolower(c);});
		return in;
	}

	std::optional<double> ParseUtility::parseFreq(std::string str)
	{
		std::optional<double> ret;
		auto idx = findNonDigit(str);
		if(idx)
		{
			// If the first character is not a number, it must be wrong
			if(*idx)
			{
				// Split string into number and multiplier
				double num = std::atof(str.substr(0,*idx).c_str());
				std::string multStr = str.substr(*idx);

				// Figure out the multipler
				std::optional<double> mult;
				// We only accept length 1 e.g. k,M,G
				// Length 2 i.e. Hz
				// Or length 3, e.g. kHz, MHz, GHz
				if(multStr.length() == 1 or (multStr.length() == 3 and multStr.substr(1) == "Hz"))
				{
					switch(multStr[0])
					{
						case 'k': mult = 1e3; break;
						case 'M': mult = 1e6; break;
						case 'G': mult = 1e9; break;
						default: break;
					}
				} else if(multStr == "Hz") {
					mult = 1;
				}
				if(mult)
				{
					ret = (*mult)*num;
				}
			}
		} else {
			// If all digits, parse directly
			ret =  std::atof(str.c_str());
		}
		return ret;
	}
