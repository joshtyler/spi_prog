#ifndef PARSE_UTILITY_H
#define PARSE_UTILITY_H

#include <optional>

namespace ParseUtility
{

	// Return index of first non number, or decimal point (in case of float) in string
	// None if there are no non digits
	std::optional<size_t> findNonDigit(std::string str);

	std::string toLower(std::string in);
	std::string toUpper(std::string in);

	std::optional<double> parseFreq(std::string str);

};

#endif
