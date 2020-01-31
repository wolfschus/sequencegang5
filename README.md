# sequencegang5
A Midi Sequencer for Linux / SDL Arturia BeatStepPro style

This is a midi sequencer which runs on a raspberry pi with touch screen.
Perfect as a supplement to the Arturia BeatStep Pro. Switching the pattern works with the same midi commands.

To use binary:

sudo apt install libsdl-gfx1.2-5

To compile:

sudo apt install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-gfx1.2-dev libsdl-mixer1.2-dev librtmidi-dev librtaudio-dev libsqlite3-dev libfluidsynth-dev fluid-soundfont-gm

./make

sudo cp sequencegang5 /usr/local/bin/

mkdir ~/.sequencegang5

cp -r db/* ~/.sequencegang5/
