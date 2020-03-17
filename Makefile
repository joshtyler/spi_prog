
CC=g++
CXXFLAGS=-c -Wall -Wextra  -g --std=c++17
LDFLAGS=-lftdi -lserial
SOURCES=FileUtility.cpp ParseUtility.cpp VectorUtility.cpp WbSpiWrapper.cpp SpiWrapper.cpp EepromProg.cpp spi_prog.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=spi_prog


all: .depend $(SOURCES) $(EXECUTABLE)

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CXXFLAGS) -MM $^>>./.depend;

include .depend


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS)  $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	/bin/rm -rf $(OBJECTS) $(EXECUTABLE)
