// CPP Wrapper for ftdi library 
// Uses same pinout as iceprog (Normal FTDI SPI pins + GPIO for SS)

#include "SpiWrapper.hpp"
#include <vector>
#include <string>
#include <iostream>

SpiWrapper::SpiWrapper(std::string devstr, enum ftdi_interface ifnum)
{
	ftdi_init(&ftdic);
	ftdi_set_interface(&ftdic, ifnum);

	if (devstr.c_str() != NULL) {
		if (ftdi_usb_open_string(&ftdic, devstr.c_str())) {
			fprintf(stderr, "Can't find iCE FTDI USB device (device string %s).\n", devstr.c_str());
			error(2);
		}
	} else {
		if (ftdi_usb_open(&ftdic, 0x0403, 0x6010) && ftdi_usb_open(&ftdic, 0x0403, 0x6014)) {
			fprintf(stderr, "Can't find iCE FTDI USB device (vendor_id 0x0403, device_id 0x6010 or 0x6014).\n");
			error(2);
		}
	}

	ftdic_open = true;

	if (ftdi_usb_reset(&ftdic)) {
		fprintf(stderr, "Failed to reset iCE FTDI USB device.\n");
		error(2);
	}

	if (ftdi_usb_purge_buffers(&ftdic)) {
		fprintf(stderr, "Failed to purge buffers on iCE FTDI USB device.\n");
		error(2);
	}

	if (ftdi_get_latency_timer(&ftdic, &ftdi_latency) < 0) {
		fprintf(stderr, "Failed to get latency timer (%s).\n", ftdi_get_error_string(&ftdic));
		error(2);
	}

	/* 1 is the fastest polling, it means 1 kHz polling */
	if (ftdi_set_latency_timer(&ftdic, 1) < 0) {
		fprintf(stderr, "Failed to set latency timer (%s).\n", ftdi_get_error_string(&ftdic));
		error(2);
	}

	ftdic_latency_set = true;

	/* Enter MPSSE (Multi-Protocol Synchronous Serial Engine) mode. Set all pins to output. */
	if (ftdi_set_bitmode(&ftdic, 0xff, BITMODE_MPSSE) < 0) {
		fprintf(stderr, "Failed to set BITMODE_MPSSE on iCE FTDI USB device.\n");
		error(2);
	}

	// enable clock divide by 5
	sendByte(MC_TCK_D5);

	// set 6 MHz clock
	sendByte(MC_SET_CLK_DIV);
	sendByte(0x00);
	sendByte(0x02);

	gpio_data = 0x20; // Power on SCK low
	setSS(true); // Make slave select high
}

SpiWrapper::~SpiWrapper()
{
	fprintf(stderr, "Bye.\n");
	gpio_data = 0; // All lines off
	setSS(false);

	ftdi_set_latency_timer(&ftdic, ftdi_latency);
	ftdi_disable_bitbang(&ftdic);
	ftdi_usb_close(&ftdic);
	ftdi_deinit(&ftdic);
}

void SpiWrapper::sendByte(uint8_t data)
{
	int rc = ftdi_write_data(&ftdic, &data, 1);
	if (rc != 1) {
		fprintf(stderr, "Write error (single byte, rc=%d, expected %d).\n", rc, 1);
		error(2);
	}
}

uint8_t SpiWrapper::recvByte(void)
{
	uint8_t data;
	while (1) {
		int rc = ftdi_read_data(&ftdic, &data, 1);
		if (rc < 0) {
			fprintf(stderr, "Read error.\n");
			error(2);
		}
		if (rc == 1)
			break;
//		usleep(100);
	}
	return data;
}

void SpiWrapper::setSS(bool data)
{
	uint8_t gpio = gpio_data;

	if(data)
	{
		// ADBUS4 (GPIOL0)
		gpio |= 0x10;
	}
	//std::cout << "Setting GPIO: " << std::hex <<(int)gpio << std::endl;
	sendByte(MC_SETB_LOW);
	sendByte(gpio); /* Value */
	sendByte(0x93); /* Direction */
}

std::vector<uint8_t> SpiWrapper::xferSpi(std::vector<uint8_t> data)
{
	std::vector<uint8_t> retVal;

	if (data.size() > 0)
	{
		setSS(false);

		/* Input and output, update data on negative edge read on positive. */
		sendByte(MC_DATA_IN | MC_DATA_OUT | MC_DATA_OCN);
		sendByte(data.size() - 1);
		sendByte((data.size() - 1) >> 8);

		unsigned int rc = ftdi_write_data(&ftdic, data.data(), data.size());
		if (rc != data.size()) {
			fprintf(stderr, "Write error (chunk, rc=%d, expected %d).\n", rc, (int)data.size());
			error(2);
		}
		for (unsigned int i = 0; i < data.size(); i++)
			retVal.push_back(recvByte());

		setSS(true);
	}
	return retVal;
}

void SpiWrapper::error(int status)
{
	checkRx();
	fprintf(stderr, "ABORT.\n");
	if (ftdic_open) {
		if (ftdic_latency_set)
			ftdi_set_latency_timer(&ftdic, ftdi_latency);
		ftdi_usb_close(&ftdic);
	}
	ftdi_deinit(&ftdic);
	exit(status);
}

void SpiWrapper::checkRx(void)
{
	while (1) {
		uint8_t data;
		int rc = ftdi_read_data(&ftdic, &data, 1);
		if (rc <= 0)
			break;
		fprintf(stderr, "unexpected rx byte: %02X\n", data);
	}
}
