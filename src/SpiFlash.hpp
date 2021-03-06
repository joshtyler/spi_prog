// Wrapper to contain all the commands needed to write to the SPI Flash

#ifndef SPI_FLASH_HPP
#define SPI_FLASH_HPP


#include <vector>
#include <exception>

#include "SpiWrapper.hpp"

class SpiFlashException : public std::exception
{
	public:
		SpiFlashException(std::string ss) : s(ss) {}
		~SpiFlashException() throw () {}
		const char* what() const throw() { return s.c_str(); }

	private:
		std::string s;
};

class SpiFlash
{
	public:
		SpiFlash(SpiInterface *spi) :spi(spi) {};
		~SpiFlash() {};

		std::vector<uint8_t> read(int addr, int num);
		std::vector<uint8_t> readId(void);
		void write(int addr, std::vector<uint8_t>::iterator start, std::vector<uint8_t>::iterator end);
		void chipErase(void);
		void sectorErase(int addr);
		void program(int addr, std::vector<uint8_t> data);
		void releasePowerDown(void);
		uint8_t readStatusRegister(int reg=1);

	private:
		void waitUntilReady(void);
		void enableWriting(void);
		void checkAndDisableWriteProection(void);

		SpiInterface *spi;

		const int pageSize = 256; //Page size in bytes
		const int sectorSize = 64*1024; //Sector size in bytes

		enum class SpiCmd : uint8_t
		{
			read = 0x03,
			chipErase = 0xC7,
			byteProgram = 0x02,
			readStatusRegister1 = 0x05,
			readStatusRegister2 = 0x35,
			readStatusRegister3 = 0x15,
			enableWriteStatusRegister = 0x50,
			writeStatusRegister = 0x01,
			writeEnable = 0x06,
			readId = 0x9F,
			sectorErase = 0xD8
		};
};

#endif
