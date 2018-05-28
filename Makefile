
CC=g++
CXXFLAGS=-c -Wall -Werror -g
LDFLAGS=-lftdi
SOURCES=SpiWrapper.cpp EepromProg.cpp spi_prog.cpp
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=spi_prog


all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(OBJECTS)  $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean: 
	/bin/rm -rf $(OBJECTS) $(EXECUTABLE)