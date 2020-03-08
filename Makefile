
CC=g++
CXXFLAGS=-c -Wall  -g
LDFLAGS=-lftdi -lserial
SOURCES=WbSpiWrapper.cpp SpiWrapper.cpp EepromProg.cpp spi_prog.cpp
HEADERS= EepromProg.hpp SpiInterface.hpp SpiWrapper.hpp WbInterface.hpp WbSpiWrapper.hpp WbUart.hpp utility
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=spi_prog


all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS)  $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	/bin/rm -rf $(OBJECTS) $(EXECUTABLE)
