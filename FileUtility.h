#ifndef FILE_UTILITY_H
#define FILE_UTILITY_H

#include <string>
#include <vector>

namespace FileUtility
{

	size_t getSize(std::string filename);

	std::vector<uint8_t> readToVector(std::string filename);

	void writeFromVector(std::string filename, std::vector<uint8_t> data);

};

#endif
