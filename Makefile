CC=g++ -std=c++20
CXXFLAGS= -std=c++20
CFLAGS=-c -Wall
LDFLAGS=-lpthread -lcurl
SOURCE_DIR=src
SOURCE_FILES=$(wildcard $(SOURCE_DIR)/*.cpp)
OBJECTS=$(SOURCE_FILES:.cpp=.o)
BUILD_DIR=bin
EXECUTABLE=ircbot

all: $(SOURCE_FILES) $(EXECUTABLE)
	
$(EXECUTABLE): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(SOURCE_DIR)/*.o $(EXECUTABLE)
