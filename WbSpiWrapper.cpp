#include "WbSpiWrapper.hpp"

WbSpiWrapper::WbSpiWrapper(WbInterface<uint8_t> *iface, uintptr_t base_addr)
:iface(iface)
{

}

std::vector<uint8_t> WbSpiWrapper::transfer(std::vector<uint8_t> data)
{
	std::vector<uint8_t> ret;
	// For now, do 255 at a time
	#warning "This is a stupid place to split it. The splitting logic is all broken"
	// If a larger vector is passed into write, it will just try and write too much (e.g. a 258 sized vector)
	// Splitting to 252 is a hack that gives us leg room for the three byte header
	auto vecs = split_vector(data);
	for(auto vec : vecs)
	{
		iface->write(2,vec);
		auto temp = iface->read(2,vec.size());
		ret.insert(ret.end(), temp.begin(), temp.end());
	}
	return ret;
}

void WbSpiWrapper::setCs(bool val)
{
	auto readData = iface->read(1,1);
	uint8_t reg = readData[0];
	if(val)
	{
		reg |= 0x01;
	} else {
		reg &= 0xFE;
	}
	iface->write(1,{reg});
}

void WbSpiWrapper::send(std::vector<uint8_t> data)
{
	iface->write(1,{0x2}); // We know CS must be low. Set to discard data
	iface->write(2,data);
	iface->write(1,{0x0}); // We know CS must be low.
}

std::vector<uint8_t> WbSpiWrapper::receive(int num)
{
	std::vector<uint8_t> ret;
	constexpr unsigned int size = 252;
	// For now, do 255 at a time
	#warning "This is a stupid place to split it. The splitting logic is all broken"
	// If a larger vector is passed into write, it will just try and write too much (e.g. a 258 sized vector)
	// Splitting to 252 is a hack that gives us leg room for the three byte header
	unsigned int numTxns = num/size + (num%size != 0);
	for(auto i=0u; i<numTxns; i++)
	{
		unsigned int cur_size = (i == numTxns-1)? (num%size? num%size : 255 ) : size;
		iface->write(3,{cur_size}); // Set to inject bytes
		auto temp = iface->read(2,cur_size);
		ret.insert(ret.end(), temp.begin(), temp.end());
	}
	return ret;
}

// Temporary
// Copied and pasted from WbUart.hpp
std::vector<std::vector<uint8_t>> WbSpiWrapper::split_vector(std::vector<uint8_t> in)
{
	constexpr unsigned int size = 252;

	std::vector<std::vector<uint8_t>> ret;

	unsigned int num = in.size()/size + (in.size()%size != 0);

	for(auto i=0u; i<num; i++)
	{
		unsigned int cur_size = (i == num-1)? (in.size()%size? in.size()%size : 255 ) : size;
		ret.push_back(std::vector<uint8_t>(in.begin()+i*size,in.begin()+(i*size)+cur_size));
	}
	return ret;
}
