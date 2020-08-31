CC       = gcc
CXX      = g++
CCFLAGS  = -Wall
CXXFLAGS = -Wall -Wno-unused-variable -Wno-write-strings
LDFLAGS  = -lSDL -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL_mixer -lrtmidi -lpthread -lfluidsynth -lsqlite3

TARGET  = sequencegang5
OBJECTS = main.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX)  $(OBJECTS) -o $@ $(CXXFLAGS) $(LDFLAGS)

.cpp.o:
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm $(TARGET)
	rm $(OBJECTS)

setup:
	if test -d ~/.$(TARGET); \
	then echo .$(TARGET) exists; \
	else mkdir ~/.$(TARGET); \
	cp -r db/* ~/.$(TARGET); \
	fi

install:
	cp $(TARGET) /usr/local/bin/
