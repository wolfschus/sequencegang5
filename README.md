# sequencegang5
A Midi Sequencer for Linux / SDL 

This is a midi sequencer which runs on a raspberry pi with touch screen.
Perfect as a supplement to the Arturia BeatStep Pro. Switching the pattern works with the same midi commands.
Works also with arturiagang

To use binary:

sudo apt install libsdl-gfx1.2-5

To compile:

sudo apt install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev librtmidi-dev librtaudio-dev libsqlite3-dev

make

To install:

make setup (Directory ~/.sequencegang will created, db-files copied, only first time needed)

sudo make install

Info:
I started developing a MIDI sequencer in 1987 on a Commodore 64. Then I ported the program to an Amiga. After I became a big Raspberry Pi fan a few years ago, I ported the program to Linux / SDL.
