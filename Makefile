CC=g++ -std=c++20
CXXFLAGS= -std=c++20 -Wall -pthread
CFLAGS= -c -Wall -pthread
LDFLAGS= -lpthread -lcurl
SOURCE_DIR=src
OBJECT_DIR=obj
BULD_DIR=bin
EXECUTABLE=$(BULD_DIR)/ircbot

# Список всех .cpp файлов
SOURCES = $(wildcard $(SOURCE_DIR)/*.cpp)

# Замена .cpp на .o и замена пути src/ на obj/
OBJECTS = $(SOURCES:$(SOURCE_DIR)/%.cpp=$(OBJECT_DIR)/%.o)

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@mkdir -p $(dir $@)
	$(CC) -o $@ $^ $(LDFLAGS)

# Правило для компиляции .cpp -> .o в директории obj/
$(OBJECT_DIR)/%.o: $(SOURCE_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJECT_DIR)/*.o $(EXECUTABLE)

.PHONY: all clean
