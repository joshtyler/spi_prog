// Simple SPI Programming tool

// Requires : libftdi1, cxxopts, boost (for progress.hpp)

#include "SpiWrapper.hpp"
#include "EepromProg.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <ctype.h>

#include <cxxopts.hpp>

#include "ftdi.h"

void print_data(std::vector<uint8_t> data)
{
	for(unsigned int i = 0; i < data.size(); i++)
	{
		if(i > 0)
		{
			std::cout << ", ";
		}

		std::cout << std::hex << (int)data[i] << std::dec;
	}

	std::cout << std::endl;
}

// Return index of first non number, or decimal point in string
// Returns -1 if none
int findNonDigit(std::string str)
{
	for(auto i=0; i < (int)str.size(); i++)
	{
			if(not (isdigit(str[i]) or str[i] == '.'))
			{
				return i;
			}
	}
	return -1;
}

double parseFreq(std::string str)
{
	// If all numbers, parse directly
	int idx = findNonDigit(str);
	if(idx == -1)
	{
		return std::atof(str.c_str());
	}

	// If the first character is not a number
	if(idx == 0)
	{
		throw cxxopts::OptionException("Frequency is invalid format");
	}

	// Split string into number and multiplier
	double num = std::atof(str.substr(0,idx).c_str());
	std::string multStr = str.substr(idx);
	// Convert multipler to all uppercase
	// ( We won't be so pedantic as to reject mis-capitalisations  :) )
	for(auto& c : multStr)
	{
		c = toupper(c);
	}

	double mult;
	if(multStr.length() == 1 or multStr.length() == 3 )
	{
		if(multStr.length() == 3 and multStr.substr(1) != "HZ")
		{
			throw cxxopts::OptionException("Frequency is invalid format");
		}
		switch(multStr[0])
		{
			case 'K': mult = 1e3; break;
			case 'M': mult = 1e6; break;
			case 'G': mult = 1e9; break;
			default: throw cxxopts::OptionException("Frequency is invalid format");
		}
	} else if(multStr == "HZ") {
		mult = 1;
	} else {
		throw cxxopts::OptionException("Frequency is invalid format");
	}
	return mult*num;
}

int main(int argc, char* argv[])
{
	//Parse arguments
	std::string filename;
	std::string deviceString;
	int addr;
	bool readId;
	bool write;
	bool verify;
	enum ftdi_interface ifNum;
	uint16_t freqDivider; //Frequency divider for SPI interface
	try {
		cxxopts::Options options("spi_prog", "Simple programmer for SPI EEPROMs");
		options.add_options()
			("f,file","File to program", cxxopts::value<std::string>())
			("d,device","Device string, in ftdi_usb_open_string() format.",cxxopts::value<std::string>()->default_value("i:0x0403:0x6010"))
			("i,interface","Used for mult-interface FTDI chips: A,B,C or D",cxxopts::value<std::string>()->default_value("A"))
			("a,address","Address to write file to. Must be aligned with sector size",cxxopts::value<int>()->default_value("0"))
			("t,readid","Read the ID bytes of the EEPROM")
			("w,write","Write a file to the EEPROM")
			("v,verify","Verify against a file")
			("x,xtal", "frequency either 60MHz or 12MHz. Used for clock divider calculation",cxxopts::value<std::string>()->default_value("12MHz"))
			("q,freq","Desired programming frequency. Max 6MHz for 12MHz clock. Max 30MHz for 60MHz clock",cxxopts::value<std::string>()->default_value("6MHz"))
			("help","Print help")
			;

		auto result = options.parse(argc, argv);

		// Print help if requested
		if (result.count("help"))
		{
			std::cout << options.help({"", "Group"}) << std::endl;
			exit(0);
		}
		readId = result.count("t");
		write = result.count("w");
		verify = result.count("v");

		if(result.count("f"))
		{
			filename = result["f"].as<std::string>();
		} else {
			if(write or verify)
			{
				throw cxxopts::OptionException("Did not specify filename");
			}
		}

		deviceString = result["d"].as<std::string>();

		std::string ifStr = result["i"].as<std::string>();
		if(ifStr.size() == 1)
		{
			switch(toupper(ifStr[0]))
			{
				case 'A':
					ifNum = INTERFACE_A;
					break;
				case 'B':
					ifNum = INTERFACE_B;
					break;
				case 'C':
					ifNum = INTERFACE_C;
					break;
				case 'D':
					ifNum = INTERFACE_D;
					break;
				default:
					throw cxxopts::OptionException("Invalid interface selected");
			}
		} else {
			throw cxxopts::OptionException("Invalid interface selected");
		}

		addr = result["a"].as<int>();

		// Calculate clock divider
		double xtal = parseFreq(result["x"].as<std::string>());
		if(not (xtal == 60e6 or xtal == 12e6 ))
		{
			throw cxxopts::OptionException("Invalid xtal frequency. Only 60MHz or 12MHz is valid");
		}

		double progFreq = parseFreq(result["q"].as<std::string>());
		if(progFreq <= 0)
		{
			throw cxxopts::OptionException("Invalid programming frequency.");
		}
		// From FTDI MPSSE Basics p.9:
		// data speed = [xtal speed] / ((1+Divisor)*2)
		// divisor = ([xtal speed]/(2*[data speed]))-1
		if(progFreq >= xtal/2.0)
		{
			freqDivider = 0x0000;
		} else {
			double divider = (xtal / (2.0*progFreq)) -1.0;

			if(divider > (double)0xFFFF)
			{
				freqDivider = (double)0xFFFF;
			} else {
				freqDivider = (uint16_t)divider;
			}
	}

		double actualFreq = xtal / ((1+ ((double)freqDivider))*2.0);
		if(actualFreq != progFreq)
		{
			std::cerr << "Could not calculate divider for requested frequency. Using " << actualFreq/1e6 << "MHz" << std::endl;
		}

		if(not (readId or write or verify))
		{
			throw cxxopts::OptionException("No action selected");
		}

	} catch (const cxxopts::OptionException& e)
  {
    std::cerr << "error parsing options: " << e.what() << std::endl;
    exit(1);
  }

	std::vector<uint8_t> dataIn;
	if(write or verify)
	{
		// Read the input data into an array
		std::ifstream is;
			// Get size of input file
		is.open(filename, std::ios::binary);
		is.seekg(0, std::ios::end);
		size_t filesize=is.tellg();
		is.seekg(0, std::ios::beg);
			// Size vector to input file
		dataIn.resize(filesize/sizeof(uint8_t));
			// Read from file to array
		is.read((char *)dataIn.data(), filesize);

		is.close();
	}

	SpiWrapper spi(deviceString, ifNum, freqDivider);
	EepromProg prog(&spi);

	if(readId)
	{
		std::cout << "Read ID" << std::endl;
		std::vector<uint8_t> data = prog.readId();
		std::cout << "Received ID (len " << std::dec << data.size() << "): ";
		print_data(data);
	}

	if(write)
	{
		std::cout << "Write to " << addr << std::endl;
		prog.program(addr, dataIn);
	}

	if(verify)
	{
		std::cout << "Read from " << addr << std::endl;

		typeof(dataIn) dataOut;
		dataOut = prog.read(addr,dataIn.size());

		if(dataOut == dataIn)
		{
			std::cout << "Data verified correctly" << std::endl;
		} else {
			std::cout << "WARNING: Verifcation error" << std::endl;
			return 1;
		}
	}

	return 0;
}
