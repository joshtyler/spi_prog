// Wrapper to contain all the commands needed to write to the EEPROM

#ifndef EEPROM_PROG_HPP
#define EEPROM_PROG_HPP


#include <vector>
#include "SpiWrapper.hpp"

class EepromProg
{
	public:
		EepromProg(SpiWrapper &spiIn) :spi(spiIn) {};
		~EepromProg() {};

		std::vector<uint8_t> read(int addr, int num);
		std::vector<uint8_t> readId(void);
		void write(int addr, uint8_t data);
		void chipErase(void);
		void sectorErase(int addr);

	private:
		uint8_t readStatusRegister(void);
		void waitUntilReady(void);
		void enableWriting(void);

		SpiWrapper &spi;

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
