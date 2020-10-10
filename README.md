# sequencegang5
A Midi Sequencer for Linux / SDL 

This is a midi sequencer which runs on a raspberry pi with touch screen.
Perfect as a supplement to the Arturia BeatStep Pro. Switching the pattern works with the same midi commands.
Works also with arturiagang

To use binary:

sudo apt install libsdl-gfx1.2-5

To compile:

sudo apt install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev libsdl-image1.2-dev librtmidi-dev librtaudio-dev libsqlite3-dev

For compile on Linux (x86/a64):

ln Makefile.linux Makefile
make

For compile on Raspberry Pi (arm):

ln Makefile.raspi Makefile
make

To install:

cp db/sequencegang5.settings ~/.sequencegang.settings
mkdir ~/Dokumente/Sequencegang5
or
mkdir ~/Documents/Sequencegang5
cp db/songs.seq5 ~/Documents/Sequencegang5

sudo make install

Info:
I started developing a MIDI sequencer in 1987 on a Commodore 64. Then I ported the program to an Amiga. After I became a big Raspberry Pi fan a few years ago, I ported the program to Linux / SDL.

Databases:
I have coosen SQLite for storing Song data and settings, because of possibility to port the program to Android easily ;-)
Song database is flexible, but the settings database is still rigid. I will be flexible about this over time. But it is not the highest priority. Please do not change the order. Adding entries does not work yet. Changes to the device names must still be made in the database.

Midi Clock Out ist possible over device 12, but not exact. The best way ist to use Midi Clock In over device 12.
Button Clock out is deaktivated, because of bad timing.

ToDo:
- additional MidiIn devices
- Documentation
