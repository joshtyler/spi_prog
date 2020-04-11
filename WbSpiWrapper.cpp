#include "VectorUtility.h"

#include "WbSpiWrapper.hpp"

WbSpiWrapper::WbSpiWrapper(WbInterface<uint8_t> *iface, uintptr_t base_addr)
:iface(iface), base_addr(base_addr)
{
	// Get current config from FPGA
	auto readData = iface->read(base_addr+1,1);
	current_config = readData[0];
}

std::vector<uint8_t> WbSpiWrapper::transfer(std::vector<uint8_t> data)
{
	std::vector<uint8_t> ret;
	ret.reserve(data.size());
	// The SPI block only has 255 words of memory inside, so we need to chunk to this amount

	modifyConfig(config_bits::DISCARD_RX, false);

	auto cur_iter = data.begin();
	while(cur_iter != data.end())
	{
		auto next_iter = VectorUtility::chunk<uint8_t>(cur_iter, data.end(),255);
		auto len = next_iter-cur_iter;
		iface->write(base_addr+2, data.begin(), data.end());
		auto temp = iface->read(base_addr+2,len);
		ret.insert(ret.end(), temp.begin(), temp.end());
		cur_iter = next_iter;
	}
	return ret;
}

void WbSpiWrapper::setCs(bool val)
{
	modifyConfig(config_bits::CS, val);
}

void WbSpiWrapper::send(std::vector<uint8_t> data)
{
	modifyConfig(config_bits::DISCARD_RX, true);
	iface->write(base_addr+2,data.begin(), data.end());
}

std::vector<uint8_t> WbSpiWrapper::receive(int num)
{
	std::vector<uint8_t> ret;
	ret.reserve(num);

	modifyConfig(config_bits::DISCARD_RX, false);

	// Because we can only inject 255 dummy bytes at a time, we need to chunk here into chunks of 255
	constexpr unsigned int size = 255;
	unsigned int numTxns = num/size + (num%size != 0);
	for(auto i=0u; i<numTxns; i++)
	{
		unsigned int cur_size = (i == numTxns-1)? (num%size? num%size : 255 ) : size;
		std::vector<uint8_t> wr = {static_cast<uint8_t>(cur_size)};
		iface->write(base_addr+3,wr.begin(), wr.end());// Set to inject bytes
		auto temp = iface->read(base_addr+2,cur_size);
		ret.insert(ret.end(), temp.begin(), temp.end());
	}
	return ret;
}

void WbSpiWrapper::modifyConfig(config_bits bit, bool value)
{
	auto idx = static_cast<std::underlying_type_t<config_bits>>(bit);
	if(current_config[idx] != value)
	{
		current_config[idx] = value;
		std::vector<uint8_t> wr = {static_cast<uint8_t>(current_config.to_ulong())};
		iface->write(base_addr+1,wr.begin(), wr.end());
	}
}
