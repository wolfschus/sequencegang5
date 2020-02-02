CC       = gcc
CXX      = g++
CCFLAGS  = -Wall
CXXFLAGS = -Wall 
LDFLAGS  = -lSDL -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL_mixer -lrtmidi -lpthread -lfluidsynth -lsqlite3

TARGET  = sequencegang5
OBJECTS = main.o

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) -o $@ $(CXXFLAGS) $(LDFLAGS) $(OBJECTS)

.cpp.o:
	$(CXX) -c $< $(CXXFLAGS)

clean:
	rm $(TARGET)
	rm $(OBJECTS)

setup:
	mkdir ~/.$(TARGET)
	cp -r db/* ~/.$(TARGET)

install:
	cp $(TARGET) /usr/local/bin/
