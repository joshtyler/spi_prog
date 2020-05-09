
#include <string>
#include <istream>
#include <fstream>

#include "FileUtility.h"

size_t FileUtility::getSize(std::string filename)
{
	std::ifstream is(filename, std::ios::binary);
	is.seekg(0, std::ios::end);
	return is.tellg();
}

std::vector<uint8_t> FileUtility::readToVector(std::string filename)
{
	std::vector<uint8_t> dat;
	size_t filesize = getSize(filename);
	std::ifstream is(filename, std::ios::binary);
	dat.resize(filesize);
	is.read((char *)dat.data(), filesize);
	return dat;
}

void FileUtility::writeFromVector(std::string filename, std::vector<uint8_t> data)
{
	std::ofstream of(filename, std::ios::out | std::ios::binary);
	of.write((char *)&data[0],data.size());
}
