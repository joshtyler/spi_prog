// Wrapper to contain all the commands needed to write to the EEPROM

#ifndef EEPROM_PROG_HPP
#define EEPROM_PROG_HPP


#include <vector>
#include <exception>

#include "SpiWrapper.hpp"

class EepromException : public std::exception
{
	public:
		EepromException(std::string ss) : s(ss) {}
		~EepromException() throw () {}
		const char* what() const throw() { return s.c_str(); }

	private:
		std::string s;
};

class EepromProg
{
	public:
		EepromProg(SpiInterface *spi) :spi(spi) {};
		~EepromProg() {};

		std::vector<uint8_t> read(int addr, int num);
		std::vector<uint8_t> readId(void);
		void write(int addr, std::vector<uint8_t>::iterator start, std::vector<uint8_t>::iterator end);
		void chipErase(void);
		void sectorErase(int addr);
		void program(int addr, std::vector<uint8_t> data);

	private:
		uint8_t readStatusRegister(void);
		void waitUntilReady(void);
		void enableWriting(void);

		SpiInterface *spi;

		const int pageSize = 256; //Page size in bytes
		const int sectorSize = 64*1024; //Sector size in bytes

		enum class SpiCmd : uint8_t
		{
			read = 0x03,
			chipErase = 0xC7,
			byteProgram = 0x02,
			readStatusRegister = 0x05,
			enableWriteStatusRegister = 0x50,
			writeStatusRegister = 0x01,
			writeEnable = 0x06,
			readId = 0x9F,
			sectorErase = 0xD8
		};
};

#endif
