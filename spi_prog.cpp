// Simple SPI Programming tool

// Requires : libftdi1, cxxopts, boost (for progress.hpp)

#include <fstream>
#include <iostream>
#include <iomanip>
#include <string>
#include <ctype.h>

#include <cxxopts.hpp>

#include "ftdi.h"

#include "ParseUtility.h"
#include "FileUtility.h"
#include "VectorUtility.h"

#include "SpiWrapper.hpp"
#include "EepromProg.hpp"
#include "WbUart.hpp"
#include "WbSpiWrapper.hpp"

// Try and parse an argument
// If present, return argument
// If not present:
	// If required, return default constructed object
	// If requiredt, throw OptionException
template <typename T> T tryParse(const cxxopts::ParseResult &result, std::string arg, bool required=true)
{
	T ret;
	if(result.count(arg))
	{
		ret = result[arg].as<T>();
	} else {
		if(required)
		{
			throw cxxopts::OptionException("Did not specify "+arg);
		}
	}
	return ret;
}

// Parse an FTDI interface string
// Not a very C++ way of doing it, but we have to parse to the ftdi.h format
enum ftdi_interface parseFtdiInterface(std::string str)
{
	enum ftdi_interface ret;
	if(str.size() == 1)
	{
		switch(toupper(str[0]))
		{
			case 'A':
				ret = INTERFACE_A;
				break;
			case 'B':
				ret = INTERFACE_B;
				break;
			case 'C':
				ret = INTERFACE_C;
				break;
			case 'D':
				ret = INTERFACE_D;
				break;
			default:
				throw cxxopts::OptionException("Invalid FTDI interface selected");
		}
	} else {
		throw cxxopts::OptionException("Invalid FTDI interface selected");
	}
	return ret;
}

int main(int argc, char* argv[])
{
	//Parse arguments
	// N.B. for simple verification arguments are constructed/pasesed in order

	std::vector<std::string> optionGroups = {"", "FTDI mode. Use with -t FTDI", "wbuart mode. Use with -t wbuart"};
	try {
		cxxopts::Options options(argv[0], "Simple programmer for SPI EEPROMs. Multiple operations are supported, and are executed in the order listed in -h");
		options.add_options(optionGroups[0])
			("h,help",         "Print help")
			("m,mode",         "Which device will do the programming. FTDI or wbuart",cxxopts::value<std::string>())
			("d,readid",       "Read the ID bytes of the EEPROM")
			("s,readstatregs", "Read the status registers")
			("c,customcmd",    "Execute a custom command (comma separated values, no whitespace)",cxxopts::value<std::vector<uint8_t>>())
			("w,write",        "Write a file to the EEPROM")
			("r,read",         "Read EEPROM to file")
			("v,verify",       "Verify against a file")
			("a,address",      "Address to read from/write to. Must be aligned with sector size",cxxopts::value<int>()->default_value("0"))
			("i,infile",       "File to write to flash/verify against (use with -w or -v)", cxxopts::value<std::string>())
			("o,outfile",      "File to save data read from flash to (use with -r)")
			("l,readlen",      "Length to read back from EEPROM. (use with -r, but not -w or -v. In these cases lengh is implicit)")
			;

		options.add_options(optionGroups[1])
			("ftdidev",   "Device string, in ftdi_usb_open_string() format.",cxxopts::value<std::string>()->default_value("i:0x0403:0x6010"))
			("iface",     "Used for mult-interface FTDI chips: A,B,C or D",cxxopts::value<std::string>()->default_value("A"))
			("xtalFreq",  "FTDI IC crystal frequency either 60MHz or 12MHz. Used for clock divider calculation",cxxopts::value<std::string>()->default_value("12MHz"))
			("progfreq",  "Desired programming frequency. Max 6MHz for 12MHz clock. Max 30MHz for 60MHz clock",cxxopts::value<std::string>()->default_value("6MHz"))
			;

		options.add_options(optionGroups[2])
			("uartdev",   "Serial port device string", cxxopts::value<std::string>())
			("baud",      "Serial port baud rate", cxxopts::value<int>())
			("compaddr",  "Address of wishbone SPI component", cxxopts::value<int>())
			;

		auto result = options.parse(argc, argv);

		// Print help if requested
		if (result.count("help"))
		{
			std::cout << options.help(optionGroups) << std::endl;
			exit(0);
		}

		// Check arguments
		std::string mode = tryParse<std::string>(result, "mode");

		// Get commands
		bool readId       = result.count("readid");
		bool readStatRegs = result.count("readstatregs");
		std::optional<std::vector<uint8_t>> customCmd;
		if(result.count("customcmd"))
		{
			customCmd = result["customcmd"].as<std::vector<uint8_t>>();
		}
		bool write        = result.count("write");
		bool read         = result.count("read");
		bool verify       = result.count("verify");

		int address = tryParse<int>(result, "address", read or write or verify);

		std::string inFile = tryParse<std::string>(result, "infile", write or verify);
		std::string outFile = tryParse<std::string>(result, "outfile", read);
		int readLen = tryParse<int>(result, "readlen", read and (not(write or verify)));

		if(not (readId or readStatRegs or write or read or verify))
		{
			throw cxxopts::OptionException("No action selected");
		}

		// Convert target to all lower case for more tolerant parsing
		mode = ParseUtility::toLower(mode);

		// Pointers are constructed here so they have correct scope
		std::unique_ptr<SpiInterface> spi = NULL;
		std::unique_ptr<WbUart<uint8_t,8>> uart = NULL;
		std::unique_ptr<EepromProg> prog = NULL;
		// Perform target specific arument parsing
		if(mode == "ftdi")
		{
			std::string ftdiDev = tryParse<std::string>(result, "ftdidev");
			enum ftdi_interface iface = parseFtdiInterface(tryParse<std::string>(result, "iface"));
			double xtalFreq;
			auto maybeXtalFreq = ParseUtility::parseFreq(tryParse<std::string>(result, "xtalfreq"));
			if(maybeXtalFreq)
			{
				xtalFreq = *maybeXtalFreq;
			} else {
				throw cxxopts::OptionException("Could not parse xtalfreq");
			}
			double progFreq;
			auto maybeProgFreq = ParseUtility::parseFreq(tryParse<std::string>(result, "progfreq"));
			if(maybeXtalFreq)
			{
				progFreq = *maybeProgFreq;
			} else {
				throw cxxopts::OptionException("Could not parse progfreq");
			}

			// Calculate clock divider
			if(not (xtalFreq == 60e6 or xtalFreq == 12e6 ))
			{
				throw cxxopts::OptionException("Invalid xtal frequency. Only 60MHz or 12MHz is valid");
			}

			if(progFreq <= 0)
			{
				throw cxxopts::OptionException("Invalid programming frequency.");
			}
			uint16_t freqDivider; //Frequency divider for SPI interface
			// From FTDI MPSSE Basics p.9:
			// data speed = [xtal speed] / ((1+Divisor)*2)
			// divisor = ([xtal speed]/(2*[data speed]))-1
			if(progFreq >= xtalFreq/2.0)
			{
				freqDivider = 0x0000;
			} else {
				double divider = (xtalFreq / (2.0*progFreq)) -1.0;

				if(divider > (double)0xFFFF)
				{
					freqDivider = (double)0xFFFF;
				} else {
					freqDivider = (uint16_t)divider;
				}
		}

			double actualFreq = xtalFreq / ((1+ ((double)freqDivider))*2.0);
			if(actualFreq != progFreq)
			{
				std::cerr << "WARNING: Could not calculate divider for requested frequency. Using " << actualFreq/1e6 << "MHz" << std::endl;
			}

			spi = std::make_unique<SpiWrapper>(ftdiDev, iface, freqDivider);
			prog = std::make_unique<EepromProg>(spi.get());

		} else if(mode == "wbuart") {

			std::string uartDev = tryParse<std::string>(result, "uartdev");
			int baud = tryParse<int>(result, "baud");
			int compAddr = tryParse<int>(result, "compaddr");

			uart = std::make_unique<WbUart<uint8_t,8>>(uartDev, baud);
			spi = std::make_unique<WbSpiWrapper>(uart.get(),compAddr);
			prog = std::make_unique<EepromProg>(spi.get());

		} else {
			throw cxxopts::OptionException("Invalid mode: "+mode);
		}

		// Arguments are now parsed, we can do the real work

		std::vector<uint8_t> dataIn;
		if(write or verify)
		{
			dataIn = FileUtility::readToVector(inFile);
		}

		// Release powerdown in case chip is asleep
		 prog->releasePowerDown();

		if(readId)
		{
			std::cout << "Read ID" << std::endl;
			std::vector<uint8_t> data = prog->readId();
			std::cout << "Received ID: ";
			VectorUtility::print(data);
		}

		if(readStatRegs)
		{
			std::cout << "Read Status registers" << std::endl;
			for(int i=1; i<=3; i++)
			{
				std::cout << "Status register " << i << ": 0x" << std::hex << std::setfill('0') << std::setw(2) << (int) prog->readStatusRegister(i) << std::dec << std::endl;
			}
		}

		if(customCmd)
		{
			std::cout << "Writing custom command: ";
			VectorUtility::print(*customCmd);

			spi->setCs(false);
			auto result = spi->transfer(*customCmd);
			spi->setCs(true);

			std::cout << "Result: ";
			VectorUtility::print(result);
		}


		if(write)
		{
			std::cout << "Write to " << address << std::endl;
			prog->program(address, dataIn);
		}

		std::vector<uint8_t> dataOut;
		if(read or verify)
		{
			std::cout << "Read from " << address << std::endl;

			if(write or verify)
			{
				readLen = dataIn.size();
				std::cout << "Size from read data (" << readLen << ")" << std::endl;
			} else {
				std::cout << "Size from arguments (" << readLen << ")" << std::endl;
			}

			dataOut = prog->read(address,readLen);

			if(read)
			{
				FileUtility::writeFromVector(outFile, dataOut);
			}
		}

		if(verify)
		{
			std::cout << "Verifying data" << std::endl;

			if(dataOut == dataIn)
			{
				std::cout << "Data verified correctly" << std::endl;
			} else {
				std::cout << "WARNING: Verifcation error" << std::endl;
				return -1;
			}
		}

		std::cout << "Done!" << std::endl;


	// The catch here is slightly lazy
	// It keeps the parsed arguments in scope
	// Thus avoiding separate delaration and use
	} catch (const cxxopts::OptionException& e)
	{
		std::cerr << "ERROR: Could not parse options: " << e.what() << std::endl;
		std::cerr << "Run with -h for help" << std::endl;
		exit(1);
	}

	return 0;
}
