CC=g++
CXXFLAGS=-c -Wall -Wextra  -g --std=c++17
LDFLAGS=-lftdi -lpthread
SOURCES=$(wildcard src/*.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=spi_prog


all: .depend $(SOURCES) $(EXECUTABLE)

.depend: $(SOURCES)
	rm -f ./.depend
	$(CC) $(CXXFLAGS) -MM $^ >>./.depend;

include .depend


$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS)  $(LDFLAGS) -o $@

.cc.o:
	$(CC) $(CXXFLAGS) $< -o $@

clean:
	/bin/rm -f $(OBJECTS) $(EXECUTABLE) .depend
