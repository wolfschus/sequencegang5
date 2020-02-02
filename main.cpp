//============================================================================
// Name        : Sequencegang5.cpp
// Author      : Wolfgang Schuster
// Version     : 0.7 25.01.2020
// Copyright   : Wolfgang Schuster
// Description : MIDI-Sequencer for Linux
// License     : GNU General Public License v3.0
//============================================================================

#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_ttf.h>
#include <SDL/SDL_gfxPrimitives.h>
#include <SDL/SDL_mixer.h>
#include <SDL/SDL_image.h>
#include <SDL/SDL_rotozoom.h>
#include <string>
#include <vector>
#include <pthread.h>
#include "rtmidi/RtMidi.h"
#include <sqlite3.h>
#include <unistd.h>
#include <algorithm>
#include <sys/sysinfo.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <fluidsynth.h>


#include "images/media-playback-start.xpm"
#include "images/media-playback-pause.xpm"
#include "images/media-playback-stop.xpm"
#include "images/media-skip-backward.xpm"
#include "images/media-skip-forward.xpm"
#include "images/media-seek-backward.xpm"
#include "images/media-seek-forward.xpm"
#include "images/gtk-edit.xpm"
#include "images/help-info.xpm"
#include "images/system-shutdown.xpm"
#include "images/dialog-ok-apply.xpm"
#include "images/document-save.xpm"
#include "images/folder-drag-accept.xpm"
#include "images/window-close.xpm"
#include "images/document-properties.xpm"
#include "images/help-about.xpm"
#include "images/go-previous.xpm"
#include "images/go-next.xpm"
#include "images/go-first.xpm"
#include "images/go-last.xpm"
#include "images/go-up.xpm"
#include "images/go-down.xpm"
#include "images/plug.xpm"
#include "images/songs.xpm"
#include "images/pattern.xpm"

using namespace std;

char cstring[512];
char playmode = 0;
char pattern[5][16][16][5][3];

struct seqsettings{
	string name;
	int mididevice;
	int midichannel;
	int minprog;
	int maxprog;
	int minbank;
	int maxbank;
	int aktbank;
	bool master;
	int midiinchannel;
	int type; // 0=midiout 1=midiin 2=midiinclock
	bool aktiv=false;
};

seqsettings asettmp;
vector <seqsettings> aset;

struct songsettings{
	string name;
	int bpm;
};

vector <songsettings> songset;

int aktstep=15;
int oldstep=0;
int miditick=0;
int oldmiditick=0;
int timedivision=16;
int maxstep = 15;
int bpm = 60;
float bpmcorrect=1.00;
bool timerrun=false;
bool clockmodeext=false;
bool exttimerrun = false;
int mode = 0;
int submode = 0;
int settingsmode = 0;
int selpattdevice = 0;
int selpattern[5] = {0,0,0,0,0};
int oldselpattern[5] = {0,0,0,0,0};
int nextselpattern[5] = {0,0,0,0,0};
char sql[512];
string songname = "New Song";
string songnametmp = "New Song";
int aktsong=0;
int aktpatt=0;
int selpatt=0;

bool beatstep = false;

struct cpuwerte{
	float idle;
	float usage;
};

cpuwerte oldcpu;
cpuwerte newcpu;
float cpuusage;
int anzahlcpu;
int cputimer = 0;
struct sysinfo memInfo;

// Fluidsynth
fluid_synth_t* fluid_synth;

RtMidiOut *midiout = new RtMidiOut();
RtMidiIn *midiin = new RtMidiIn();

class WSMidi
{
public:
//   void run()
//   {
//	  pthread_t thread;
//	  pthread_create(&thread, NULL, entry, this);
//   }

	void NoteOn(uint mididevice, int midichannel, int note, int volume)
	{
		vector<unsigned char> message;

		if(mididevice==255)
		{
			fluid_synth_noteon(fluid_synth, midichannel, note, volume);
		}
		else if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(144+midichannel);
			message.push_back(note);
			message.push_back(volume);
			midiout->sendMessage( &message );
//			cout << int(message[0]) << " " << int(message[1]) << " " << int(message[2]) << endl;
			midiout->closePort();
		}
		return;
	}

	void NoteOff(uint mididevice, int midichannel, int note)
	{
		vector<unsigned char> message;

		if(mididevice==255)
		{
			fluid_synth_noteoff(fluid_synth, midichannel, note);
		}
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(128+midichannel);
			message.push_back(note);
			message.push_back(0);
			midiout->sendMessage( &message );
//			cout << int(message[0]) << " " << int(message[1]) << " " << int(message[2]) << endl;
			midiout->closePort();
		}
		return;
	}

	void ProgramChange(uint mididevice, int midichannel, int program)
	{
		vector<unsigned char> message;

		if(mididevice==255)
		{
			fluid_synth_program_change(fluid_synth, midichannel,program);
		}
		else if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(192+midichannel);
			message.push_back(program);
			midiout->sendMessage( &message );
//			cout << int(message[0]) << " " << int(message[1]) << " " << endl;
			midiout->closePort();
		}
		return;
	}

	void AllSoundsOff(uint mididevice, int midichannel)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(176+midichannel);
			message.push_back(120);
			message.push_back(0);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
	}

	void AllNotesOff(uint mididevice, int midichannel)
	{
		vector<unsigned char> message;

		if(mididevice==255)
		{
			fluid_synth_all_notes_off(fluid_synth, midichannel);
		}
		else if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(176+midichannel);
			message.push_back(123);
			message.push_back(0);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
	}
	void Clock_Start(uint mididevice)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(0xFA);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void Clock_Cont(uint mididevice)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(0xFB);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void Clock_Stop(uint mididevice)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(0xFC);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void Clock_Tick(uint mididevice)
	{
		vector<unsigned char> message;
		if(mididevice<midiout->getPortCount())
		{
			midiout->openPort(mididevice);
			message.clear();
			message.push_back(0xF8);
			midiout->sendMessage( &message );
			midiout->closePort();
		}
		return;
	}

	void Play()
	{
		playmode=1;
		timerrun=true;
		aktstep=15;
	}

	void Pause()
	{
		playmode=2;
		timerrun=false;

	}

	void Stop()
	{
		playmode=0;
		timerrun=false;
		aktstep=15;
		for(int i=0;i<5;i++)
		{
			AllNotesOff(aset[i].mididevice,aset[i].midichannel);
		}
	}

	void NextTick()
	{
	  oldmiditick=miditick;
	  if(miditick<5)
	  {
		  miditick++;
	  }
	  else
	  {
		  miditick=0;
		  oldstep=aktstep;
		  aktstep++;
		  if(aktstep>maxstep)
		  {
			  aktstep=0;
			  for(int i=0;i<5;i++)
			  {
				  selpattern[i]=nextselpattern[i];
			  }

		  }

		  if(mode==0)
		  {
			  if(submode==1)
			  {
				  MidiCommand(selpattdevice, aktstep);
			  }
			  else
			  {
				  for(int i=0;i<5;i++)
				  {
					  MidiCommand(i, aktstep);
				  }
			  }
		  }
	  }
	}

	void MidiCommand(int aktdev, int step)
	{
//		pattern[5][16][16][5][3];
		for(int i=0;i<5;i++)
		{
			if(pattern[aktdev][selpattern[aktdev]][oldstep][i][0]==1)
			{
				NoteOff(aset[aktdev].mididevice,aset[aktdev].midichannel,pattern[aktdev][selpattern[aktdev]][oldstep][i][1]);
			}
			if(pattern[aktdev][selpattern[aktdev]][step][i][0]==1)
			{
				NoteOn(aset[aktdev].mididevice,aset[aktdev].midichannel, pattern[aktdev][selpattern[aktdev]][step][i][1], pattern[aktdev][selpattern[aktdev]][step][i][2]);
			}
			if(pattern[aktdev][selpattern[aktdev]][step][i][0]==2)
			{
				NoteOn(aset[aktdev].mididevice,aset[aktdev].midichannel, pattern[aktdev][selpattern[aktdev]][step][i][1], pattern[aktdev][selpattern[aktdev]][step][i][2]);
			}
			if(pattern[aktdev][selpattern[aktdev]][step][i][0]==3)
			{
				NoteOff(aset[aktdev].mididevice,aset[aktdev].midichannel,pattern[aktdev][selpattern[aktdev]][step][i][1]);
			}
			if(pattern[aktdev][selpattern[aktdev]][step][i][0]==4)
			{
				ProgramChange(aset[aktdev].mididevice, aset[aktdev].midichannel, pattern[aktdev][selpattern[aktdev]][step][i][1]);
			}
		}
	}

//private:
//   static void * entry(void *ptr)
//   {
//      WSMidi *wsm = reinterpret_cast<WSMidi*>(ptr);
//      return 0;
//   }
};

WSMidi wsmidi;

class ThreadClass
{
public:
   void run()
   {
      pthread_t thread;

      pthread_create(&thread, NULL, entry, this);
   }

   void UpdateMidiTimer()
   {
      while(1)
	  {
    	  usleep(60000000/(24*(bpm*bpmcorrect+60)));
    	  if(timerrun==true and clockmodeext==false)
    	  {
    		  wsmidi.NextTick();
     	  }
	  }
   }

private:
   static void * entry(void *ptr)
   {
      ThreadClass *tc = reinterpret_cast<ThreadClass*>(ptr);
      tc->UpdateMidiTimer();
      return 0;
   }
};

class ThreadCPUClass
{
public:
   void run()
   {
      pthread_t thread;

      pthread_create(&thread, NULL, entry, this);
   }

   void UpdateCPUTimer()
   {
      while(1)
	  {
    	  usleep(200000);
    	  oldcpu.idle=newcpu.idle;
    	  oldcpu.usage=newcpu.usage;
    	  newcpu=get_cpuusage();
    	  cpuusage=((newcpu.usage - oldcpu.usage)/(newcpu.idle + newcpu.usage - oldcpu.idle - oldcpu.usage))*100;
    	  sysinfo (&memInfo);
	  }
   }

   cpuwerte get_cpuusage()
   {
		ifstream fileStat("/proc/stat");
		string line;

		cpuwerte cpuusage;

		while(getline(fileStat,line))
		{
			if(line.find("cpu ")==0)
			{
				istringstream ss(line);
				string token;

				vector<string> result;
				while(std::getline(ss, token, ' '))
				{
					result.push_back(token);
				}
				cpuusage.idle=atof(result[5].c_str()) + atof(result[6].c_str());
				cpuusage.usage=atof(result[2].c_str()) + atof(result[3].c_str()) + atof(result[4].c_str()) + atof(result[7].c_str()) + atof(result[8].c_str()) + atof(result[9].c_str()) + atof(result[10].c_str()) + atof(result[11].c_str());
			}
		}
		return cpuusage;

   }

private:
   static void * entry(void *ptr)
   {
      ThreadCPUClass *tcc = reinterpret_cast<ThreadCPUClass*>(ptr);
      tcc->UpdateCPUTimer();
      return 0;
   }
};

class WSButton
{

public:
	bool aktiv;
	SDL_Rect button_rect;
	SDL_Rect image_rect;
	SDL_Rect text_rect;
	SDL_Surface* button_image;
	string button_text;
	int button_width;
	int button_height;

	WSButton()
	{
		button_text="";
		button_image=NULL;
		button_width=2;
		button_height=2;
		aktiv = false;
		button_rect.x = 0+3;
		button_rect.y = 0+3;
		button_rect.w = 2*48-6;
		button_rect.h = 2*48-6;
	}

	WSButton(int posx, int posy, int width, int height, int scorex, int scorey, SDL_Surface* image, string text)
	{
		button_text=text;
		button_image=image;
		button_width=width;
		button_height=height;
		aktiv = false;
		button_rect.x = posx*scorex+3;
		button_rect.y = posy*scorey+3;
		button_rect.w = button_width*scorex-6;
		button_rect.h = 2*scorey-6;
	}

	void show(SDL_Surface* screen, TTF_Font* font)
	{
		if(aktiv==true)
		{
			boxColor(screen, button_rect.x,button_rect.y,button_rect.x+button_rect.w,button_rect.y+button_rect.h,0x008F00FF);
		}
		else
		{
			boxColor(screen, button_rect.x,button_rect.y,button_rect.x+button_rect.w,button_rect.y+button_rect.h,0x8F8F8FFF);
		}

		if(button_image!=NULL)
		{
			image_rect.x = button_rect.x+2;
			image_rect.y = button_rect.y+1;
			SDL_BlitSurface(button_image, 0, screen, &image_rect);
		}
		if(button_text!="")
		{
			SDL_Surface* button_text_image;
			SDL_Color blackColor = {0, 0, 0};
			button_text_image = TTF_RenderText_Blended(font, button_text.c_str(), blackColor);
			text_rect.x = button_rect.x+button_rect.w/2-button_text_image->w/2;
			text_rect.y = button_rect.y+button_rect.h/2-button_text_image->h/2;
			SDL_BlitSurface(button_text_image, 0, screen, &text_rect);
//			SDL_FreeSurface(button_text_image);
		}
		return;
	}

	~WSButton()
	{
		return;
	}
};

vector <WSButton> load_song;
vector <WSButton> pattern_aktpatt;

static int settingscallback(void* data, int argc, char** argv, char** azColName)
{
	asettmp.name=argv[1];
	asettmp.mididevice=atoi(argv[2]);
	asettmp.midichannel=atoi(argv[3]);
	asettmp.maxbank=atoi(argv[4]);
	asettmp.maxprog=atoi(argv[5]);
	asettmp.minbank=atoi(argv[6]);
	asettmp.minprog=atoi(argv[7]);
	asettmp.master=atoi(argv[8]);
	asettmp.midiinchannel=atoi(argv[9]);
	aset.push_back(asettmp);

	return 0;
}

static int songpatterncallback(void* data, int argc, char** argv, char** azColName)
{
	pattern[atoi(argv[1])][atoi(argv[2])][atoi(argv[3])][atoi(argv[4])][0]=atoi(argv[5]);
	pattern[atoi(argv[1])][atoi(argv[2])][atoi(argv[3])][atoi(argv[4])][1]=atoi(argv[6]);
	pattern[atoi(argv[1])][atoi(argv[2])][atoi(argv[3])][atoi(argv[4])][2]=atoi(argv[7]);
	return 0;
}

static int songpatternnamecallback(void* data, int argc, char** argv, char** azColName)
{
	songsettings songsettmp;
	songsettmp.name=argv[1];
	songsettmp.bpm=atoi(argv[2]);
	songset.push_back(songsettmp);
	return 0;
}

void midiincallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
	unsigned int nBytes = message->size();

	for(int i=0;i<nBytes;i++)
	{
		cout << (int)message->at(i) << " ";
	}
	cout << endl;

	if((int)message->at(0)==176)
	{
		if((int)message->at(1)==7)
		{
			bpm = 2* (int)message->at(2);
		}
		if((int)message->at(1)==114)
		{
		}
	}
	else if((int)message->at(0)==144)
	{
		if((int)message->at(1)>=44 and (int)message->at(1)<=51)
		{
			aktpatt=(int)message->at(1)-44;
		}		
		else if((int)message->at(1)>=36 and (int)message->at(1)<=43)
		{
			aktpatt=(int)message->at(1)-28;
		}		
	}
	else if((int)message->at(0)>=192 and (int)message->at(0)<208)
	{
		for(int i=0;i<5;i++)
		{
			if((int)message->at(0)-192==aset[i].midiinchannel)
			{
				cout << aset[i].name << " " << (int)message->at(1) << endl;
				if((int)message->at(1)<16)
				{
					nextselpattern[i]=(int)message->at(1);
				}
			}
		}
	}

	if(clockmodeext==true)
	{
		if((int)message->at(0)==248 and playmode==1)
		{
  		  wsmidi.NextTick();
		}

		if((int)message->at(0)==240)
		{
			if((int)message->at(1)==127 and (int)message->at(2)==127 and (int)message->at(3)==6 and (int)message->at(4)==2 and (int)message->at(5)==247)
			{
				wsmidi.Play();
			}
		}
		if((int)message->at(0)==240)
		{
			if((int)message->at(1)==127 and (int)message->at(2)==127 and (int)message->at(3)==6 and (int)message->at(4)==1 and (int)message->at(5)==247)
			{
				wsmidi.Stop();
			}
		}
		if((int)message->at(0)==240)
		{
			if((int)message->at(1)==127 and (int)message->at(2)==127 and (int)message->at(3)==6 and (int)message->at(4)==9 and (int)message->at(5)==247)
			{
				wsmidi.Pause();
			}
		}
	}
	return;
}


bool CheckMouse(int mousex, int mousey, SDL_Rect Position)
{
	if( ( mousex > Position.x ) && ( mousex < Position.x + Position.w ) && ( mousey > Position.y ) && ( mousey < Position.y + Position.h ) )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

bool Clearpattern()
{
	for(int i=0;i<5;i++)
	{
		for(int j=0;j<15;j++)
		{
			for(int k=0;k<15;k++)
			{
				for(int l=0;l<5;l++)
				{
					for(int m=0;m<3;m++)
					{
						pattern[i][j][k][l][m]=0;
					}
				}
			}
		}
	}
	return true;
}

int LoadScene(int nr)
{
	// Load SongDB
	Clearpattern();
	sqlite3 *songsdb;
	char dbpath[512];
	sprintf(dbpath, "%s/.sequencegang5/songs.db", getenv("HOME"));
	if(sqlite3_open(dbpath, &songsdb) != SQLITE_OK)
	{
		cout << "Fehler beim Öffnen: " << sqlite3_errmsg(songsdb) << endl;
		return 1;
	}
	cout << "Songsdatenbank erfolgreich geöffnet!" << endl;
	sprintf(sql, "SELECT * FROM Song%d",nr);
	if( sqlite3_exec(songsdb,sql,songpatterncallback,0,0) != SQLITE_OK)
	{
		cout << "Fehler beim SELECT: " << sqlite3_errmsg(songsdb) << endl;
		return 1;
	}
	sqlite3_close(songsdb);
	cout << "Load Song " << nr << endl;
	songname=load_song[nr].button_text;
	aktsong=nr+1;
	bpm=songset[nr].bpm-60;

	return 0;
}

int main(int argc, char* argv[])
{
	bool debug=false;
	bool fullscreen=false;
	
	// Argumentverarbeitung
	for (int i = 0; i < argc; ++i)
	{
		if(string(argv[i])=="--help")
		{
			cout << "Sequencegang5" << endl;
			cout << "(c) 1997 - 2020 by Wolfgang Schuster" << endl;
			cout << "sequencegang5 --fullscreen = fullscreen" << endl;
			cout << "sequencegang5 --debug = debug" << endl;
			cout << "sequencegang5 --help = this screen" << endl;
			SDL_Quit();
			exit(0);
		}
		if(string(argv[i])=="--fullscreen")
		{
			fullscreen=true;
		}
		if(string(argv[i])=="--debug")
		{
			debug=true;
		}
	}

	ThreadClass tc;
	tc.run();
	ThreadCPUClass tcc;
	tcc.run();

	if(SDL_Init(SDL_INIT_VIDEO) == -1)
	{
		std::cerr << "Konnte SDL nicht initialisieren! Fehler: " << SDL_GetError() << std::endl;
		return -1;
	}
	SDL_Surface* screen;
	if(fullscreen==true)
	{
		screen = SDL_SetVideoMode(1024, 600 , 32, SDL_DOUBLEBUF|SDL_FULLSCREEN);
	}
	else
	{
		screen = SDL_SetVideoMode(1024, 600 , 32, SDL_DOUBLEBUF);
	}
	if(!screen)
	{
	    std::cerr << "Konnte SDL-Fenster nicht erzeugen! Fehler: " << SDL_GetError() << std::endl;
	    return -1;
	}
	int scorex = screen->w/36;
	int scorey = screen->h/21;

	if(TTF_Init() == -1)
	{
	    std::cerr << "Konnte SDL_ttf nicht initialisieren! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* fontbold = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 22);
	if(!fontbold)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 22);
	if(!font)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* fontsmall = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 16);
	if(!fontsmall)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* fontsmallbold = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", 16);
	if(!fontsmallbold)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	TTF_Font* fontextrasmall = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
	if(!fontsmall)
	{
	    std::cerr << "Konnte Schriftart nicht laden! Fehler: " << TTF_GetError() << std::endl;
	    return -1;
	}
	SDL_WM_SetCaption("Sequencegang5", "Sequencegang5");

	bool run = true;

	// Fluidsynth
    fluid_settings_t* fluid_settings = new_fluid_settings();
    fluid_audio_driver_t* adriver;
	struct fluid_program {
    	unsigned int channel;
		unsigned int bank;
		unsigned int program;
	};
	fluid_program fptmp;
	vector<fluid_program> fluid_program_state;
    int sf2id;
    char* fluid_alsa_device;
	unsigned int sfid = 0;
	unsigned int fsbank = 0;
	unsigned int fsprogram = 0;
	vector<SDL_Rect> fluid_settings_bank_rect;
	vector<SDL_Rect> fluid_settings_program_rect;

    fluid_settings_setint(fluid_settings, "synth.polyphony", 128);
    fluid_synth = new_fluid_synth(fluid_settings);
    fluid_settings_setstr(fluid_settings, "audio.driver", "alsa");
    adriver = new_fluid_audio_driver(fluid_settings, fluid_synth);
    sf2id = fluid_synth_sfload(fluid_synth,"/usr/share/sounds/sf2/FluidR3_GM.sf2",true);
    fluid_synth_program_change(fluid_synth, 0 , 0);

    int fluid_nmid_chan = fluid_synth_count_midi_channels(fluid_synth);
    fluid_settings_getstr(fluid_settings, "audio.alsa.device", &fluid_alsa_device);

    for(int i=0;i<16;i++)
    {
		if (FLUID_OK == fluid_synth_get_program (fluid_synth, i, &sfid, &fsbank, &fsprogram))
		{
			fptmp.channel=i;
			fptmp.bank=fsbank;
			fptmp.program=fsprogram;
			fluid_program_state.push_back(fptmp);
		}
    }

// GM Instruments
	vector<char *> gm_program_name;
	gm_program_name.push_back("Acoustic Piano");
	gm_program_name.push_back("Bright Piano");
	gm_program_name.push_back("Electric Grand Piano");
	gm_program_name.push_back("Honky-tonk Piano");
	gm_program_name.push_back("Electric Piano 1");
	gm_program_name.push_back("Electric Piano 2");
	gm_program_name.push_back("Harpsichord");
	gm_program_name.push_back("Clavi");
	gm_program_name.push_back("Celesta");
	gm_program_name.push_back("Glockenspiel");
	gm_program_name.push_back("Musical box");
	gm_program_name.push_back("Vibraphone");
	gm_program_name.push_back("Marimba");
	gm_program_name.push_back("Xylophone");
	gm_program_name.push_back("Tubular Bell");
	gm_program_name.push_back("Dulcimer");
	gm_program_name.push_back("Drawbar Organ");
	gm_program_name.push_back("Percussive Organ");
	gm_program_name.push_back("Rock Organ");
	gm_program_name.push_back("Church organ");
	gm_program_name.push_back("Reed organ");
	gm_program_name.push_back("Accordion");
	gm_program_name.push_back("Harmonica");
	gm_program_name.push_back("Tango Accordion");
	gm_program_name.push_back("Acoustic Guitar (nylon)");
	gm_program_name.push_back("Acoustic Guitar (steel)");
	gm_program_name.push_back("Electric Guitar (jazz)");
	gm_program_name.push_back("Electric Guitar (clean)");
	gm_program_name.push_back("Electric Guitar (muted)");
	gm_program_name.push_back("Overdriven Guitar");
	gm_program_name.push_back("Distortion Guitar");
	gm_program_name.push_back("Guitar harmonics");
	gm_program_name.push_back("Acoustic Bass");
	gm_program_name.push_back("Electric Bass (finger)");
	gm_program_name.push_back("Electric Bass (pick)");
	gm_program_name.push_back("Fretless Bass");
	gm_program_name.push_back("Slap Bass 1");
	gm_program_name.push_back("Slap Bass 2");
	gm_program_name.push_back("Synth Bass 1");
	gm_program_name.push_back("Synth Bass 2");
	gm_program_name.push_back("Violin");
	gm_program_name.push_back("Viola");
	gm_program_name.push_back("Cello");
	gm_program_name.push_back("Double bass");
	gm_program_name.push_back("Tremolo Strings");
	gm_program_name.push_back("Pizzicato Strings");
	gm_program_name.push_back("Orchestral Harp");
	gm_program_name.push_back("Timpani");
	gm_program_name.push_back("String Ensemble 1");
	gm_program_name.push_back("String Ensemble 2");
	gm_program_name.push_back("Synth Strings 1");
	gm_program_name.push_back("Synth Strings 2");
	gm_program_name.push_back("Voice Aahs");
	gm_program_name.push_back("Voice Oohs");
	gm_program_name.push_back("Synth Voice");
	gm_program_name.push_back("Orchestra Hit");
	gm_program_name.push_back("Trumpet");
	gm_program_name.push_back("Trombone");
	gm_program_name.push_back("Tuba");
	gm_program_name.push_back("Muted Trumpet");
	gm_program_name.push_back("French horn");
	gm_program_name.push_back("Brass Section");
	gm_program_name.push_back("Synth Brass 1");
	gm_program_name.push_back("Synth Brass 2");
	gm_program_name.push_back("Soprano Sax");
	gm_program_name.push_back("Alto Sax");
	gm_program_name.push_back("Tenor Sax");
	gm_program_name.push_back("Baritone Sax");
	gm_program_name.push_back("Oboe");
	gm_program_name.push_back("English Horn");
	gm_program_name.push_back("Bassoon");
	gm_program_name.push_back("Clarinet");
	gm_program_name.push_back("Piccolo");
	gm_program_name.push_back("Flute");
	gm_program_name.push_back("Recorder");
	gm_program_name.push_back("Pan Flute");
	gm_program_name.push_back("Blown Bottle");
	gm_program_name.push_back("Shakuhachi");
	gm_program_name.push_back("Whistle");
	gm_program_name.push_back("Ocarina");
	gm_program_name.push_back("Lead 1 (square)");
	gm_program_name.push_back("Lead 2 (sawtooth)");
	gm_program_name.push_back("Lead 3 (calliope)");
	gm_program_name.push_back("Lead 4 (chiff)");
	gm_program_name.push_back("Lead 5 (charang)");
	gm_program_name.push_back("Lead 6 (voice)");
	gm_program_name.push_back("Lead 7 (fifths)");
	gm_program_name.push_back("Lead 8 (bass + lead)");
	gm_program_name.push_back("Pad 1 (Fantasia)");
	gm_program_name.push_back("Pad 2 (warm)");
	gm_program_name.push_back("Pad 3 (polysynth)");
	gm_program_name.push_back("Pad 4 (choir)");
	gm_program_name.push_back("Pad 5 (bowed)");
	gm_program_name.push_back("Pad 6 (metallic)");
	gm_program_name.push_back("Pad 7 (halo)");
	gm_program_name.push_back("Pad 8 (sweep)");
	gm_program_name.push_back("FX 1 (rain)");
	gm_program_name.push_back("FX 2 (soundtrack)");
	gm_program_name.push_back("FX 3 (crystal)");
	gm_program_name.push_back("FX 4 (atmosphere)");
	gm_program_name.push_back("FX 5 (brightness)");
	gm_program_name.push_back("FX 6 (goblins)");
	gm_program_name.push_back("FX 7 (echoes)");
	gm_program_name.push_back("FX 8 (sci-fi)");
	gm_program_name.push_back("Sitar");
	gm_program_name.push_back("Banjo");
	gm_program_name.push_back("Shamisen");
	gm_program_name.push_back("Koto");
	gm_program_name.push_back("Kalimba");
	gm_program_name.push_back("Bagpipe");
	gm_program_name.push_back("Fiddle");
	gm_program_name.push_back("Shanai");
	gm_program_name.push_back("Tinkle Bell");
	gm_program_name.push_back("Agogo");
	gm_program_name.push_back("Steel Drums");
	gm_program_name.push_back("Woodblock");
	gm_program_name.push_back("Taiko Drum");
	gm_program_name.push_back("Melodic Tom");
	gm_program_name.push_back("Synth Drum");
	gm_program_name.push_back("Reverse Cymbal");
	gm_program_name.push_back("Guitar Fret Noise");
	gm_program_name.push_back("Breath Noise");
	gm_program_name.push_back("Seashore");
	gm_program_name.push_back("Bird Tweet");
	gm_program_name.push_back("Telephone Ring");
	gm_program_name.push_back("Helicopter");
	gm_program_name.push_back("Applause");
	gm_program_name.push_back("Gunshot");

// GM Drums
	vector<char *> gm_drum_name;
	gm_drum_name.push_back("Bass Drum 2");
	gm_drum_name.push_back("Bass Drum 1");
	gm_drum_name.push_back("Side Stick");
	gm_drum_name.push_back("Snare Drum 1");
	gm_drum_name.push_back("Hand Clap");
	gm_drum_name.push_back("Snare Drum 2");
	gm_drum_name.push_back("Low Tom 2");
	gm_drum_name.push_back("Closed Hi-hat");
	gm_drum_name.push_back("Low Tom 1");
	gm_drum_name.push_back("Pedal Hi-hat");
	gm_drum_name.push_back("Mid Tom 2");
	gm_drum_name.push_back("Open Hi-hat");
	gm_drum_name.push_back("Mid Tom 1");
	gm_drum_name.push_back("High Tom 2");
	gm_drum_name.push_back("Crash Cymbal 1");
	gm_drum_name.push_back("High Tom 1");
	gm_drum_name.push_back("Ride Cymbal 1");
	gm_drum_name.push_back("Chinese Cymbal");
	gm_drum_name.push_back("Ride Bell");
	gm_drum_name.push_back("Tambourine");
	gm_drum_name.push_back("Splash Cymbal");
	gm_drum_name.push_back("Cowbell");
	gm_drum_name.push_back("Crash Cymbal 2");
	gm_drum_name.push_back("Vibra Slap");
	gm_drum_name.push_back("Ride Cymbal 2");
	gm_drum_name.push_back("High Bongo");
	gm_drum_name.push_back("Low Bongo");
	gm_drum_name.push_back("Mute High Conga");
	gm_drum_name.push_back("Open High Conga");
	gm_drum_name.push_back("Low Conga");
	gm_drum_name.push_back("High Timbale");
	gm_drum_name.push_back("Low Timbale");
	gm_drum_name.push_back("High Agogo");
	gm_drum_name.push_back("Low Agogo");
	gm_drum_name.push_back("Cabasa");
	gm_drum_name.push_back("Maracas");
	gm_drum_name.push_back("Short Whistle");
	gm_drum_name.push_back("Long Whistle");
	gm_drum_name.push_back("Short Guiro");
	gm_drum_name.push_back("Long Guiro");
	gm_drum_name.push_back("Claves");
	gm_drum_name.push_back("High Wood Block");
	gm_drum_name.push_back("Low Wood Block");
	gm_drum_name.push_back("Mute Cuica");
	gm_drum_name.push_back("Open Cuica");
	gm_drum_name.push_back("Mute Triangle");
	gm_drum_name.push_back("Open Triangle");
	
	// [vor der Event-Schleife] In diesem Array merken wir uns, welche Tasten gerade gedrückt sind.
	bool keyPressed[SDLK_LAST];
	memset(keyPressed, 0, sizeof(keyPressed));
	SDL_EnableUNICODE(1);
	SDL_Rect textPosition;
	textPosition.x = 5;
	textPosition.y = 0;
	SDL_Surface* text = NULL;
	SDL_Color textColor = {225, 225, 225};
	SDL_Color blackColor = {0, 0, 0};

	SDL_Rect rahmen;
	rahmen.x = 4*scorex;
	rahmen.y = 3*scorey;
	rahmen.w = 32*scorex;
	rahmen.h = 10*scorey;

	SDL_Surface* start_image = IMG_ReadXPMFromArray(media_playback_start_xpm);
	SDL_Surface* stop_image = IMG_ReadXPMFromArray(media_playback_stop_xpm);
	SDL_Surface* pause_image = IMG_ReadXPMFromArray(media_playback_pause_xpm);
	SDL_Surface* next_image = IMG_ReadXPMFromArray(media_skip_forward_xpm);
	SDL_Surface* prev_image = IMG_ReadXPMFromArray(media_skip_backward_xpm);
	SDL_Surface* ff_image = IMG_ReadXPMFromArray(media_seek_forward_xpm);
	SDL_Surface* fb_image = IMG_ReadXPMFromArray(media_seek_backward_xpm);
	SDL_Surface* edit_image = IMG_ReadXPMFromArray(gtk_edit_xpm);
	SDL_Surface* info_image = IMG_ReadXPMFromArray(help_info_xpm);
	SDL_Surface* exit_image = IMG_ReadXPMFromArray(system_shutdown_xpm);
	SDL_Surface* ok_image = IMG_ReadXPMFromArray(dialog_ok_apply_xpm);
	SDL_Surface* save_image = IMG_ReadXPMFromArray(document_save_xpm);
	SDL_Surface* open_image = IMG_ReadXPMFromArray(folder_drag_accept_xpm);
	SDL_Surface* cancel_image = IMG_ReadXPMFromArray(window_close_xpm);
	SDL_Surface* settings_image = IMG_ReadXPMFromArray(document_properties_xpm);
	SDL_Surface* manuell_image = IMG_ReadXPMFromArray(help_about_xpm);
	SDL_Surface* left_image = IMG_ReadXPMFromArray(go_previous_xpm);
	SDL_Surface* right_image = IMG_ReadXPMFromArray(go_next_xpm);
	SDL_Surface* first_image = IMG_ReadXPMFromArray(go_first_xpm);
	SDL_Surface* last_image = IMG_ReadXPMFromArray(go_last_xpm);
	SDL_Surface* up_image = IMG_ReadXPMFromArray(go_up_xpm);
	SDL_Surface* down_image = IMG_ReadXPMFromArray(go_down_xpm);
	SDL_Surface* plug_image = IMG_ReadXPMFromArray(plug_xpm);
	SDL_Surface* songs_image = IMG_ReadXPMFromArray(songs_xpm);
	SDL_Surface* pattern_image = IMG_ReadXPMFromArray(pattern_xpm);

	char tmp[256];

	// Load SettingsDB
	char dbpath[512];
	sprintf(dbpath, "%s/.sequencegang5/settings.db", getenv("HOME"));
	sqlite3 *settingsdb;
	if(sqlite3_open(dbpath, &settingsdb) != SQLITE_OK)
	{
		cout << "Fehler beim Öffnen: " << sqlite3_errmsg(settingsdb) << endl;
		return 1;
	}
	cout << "Settingsdatenbank erfolgreich geöffnet!" << endl;
	sprintf(sql, "SELECT * FROM settings");
	if( sqlite3_exec(settingsdb,sql,settingscallback,0,0) != SQLITE_OK)
	{
		cout << "Fehler beim SELECT: " << sqlite3_errmsg(settingsdb) << endl;
		return 1;
	}
	sqlite3_close(settingsdb);

	int aktbank[5] = {0,0,0,0,0};
	int selsetmididevice = 0;
	int seleditstep = 0;
	int akteditnote = 60;
	int akteditvolume = 127;
	int seleditcommand = 0;
	int akteditprogram = 0;
	SDL_Rect selsetmidideviceRect;
	selsetmidideviceRect.x = 5.5*scorex;
	selsetmidideviceRect.y = 4.5*scorey;
	selsetmidideviceRect.w = 24*scorex;
	selsetmidideviceRect.h = 12*scorey;
	int selsetmidichannel = 0;
	SDL_Rect selsetmidichannelRect;
	selsetmidichannelRect.x = 30*scorex;
	selsetmidichannelRect.y = 4.5*scorey;
	selsetmidichannelRect.w = 6*scorex;
	selsetmidichannelRect.h = 12*scorey;

	char keyboard_letters[40][2] = {"1","2","3","4","5","6","7","8","9","0","q","w","e","r","t","z","u","i","o","p","a","s","d","f","g","h","j","k","l","+","y","x","c","v","b","n","m",",",".","-"};
	char shkeyboard_letters[40][2] = {"1","2","3","4","5","6","7","8","9","0","Q","W","E","R","T","Z","U","I","O","P","A","S","D","F","G","H","J","K","L","*","Y","X","C","V","B","N","M",";",":","_"};

	anzahlcpu=get_nprocs();
	sysinfo (&memInfo);

	int aktprog[5] = {aset[0].minprog,aset[1].minprog,aset[2].minprog,aset[3].minprog,aset[4].minprog};

	SDL_Rect imagePosition;

	WSButton start(32,19,2,2,scorex,scorey,start_image,"");
	WSButton pause(32,19,2,2,scorex,scorey,pause_image,"");
	WSButton stop(30,19,2,2,scorex,scorey,stop_image,"");
	WSButton exit(0,19,2,2,scorex,scorey,exit_image,"");
	WSButton info(2,19,2,2,scorex,scorey,info_image,"");
	WSButton settings(4,19,2,2,scorex,scorey,settings_image,"");
	WSButton open(6,19,2,2,scorex,scorey,open_image,"");
	WSButton save(8,19,2,2,scorex,scorey,save_image,"");
	WSButton program(16,19,2,2,scorex,scorey,NULL,"Prog");
	WSButton noteonoff(18,19,2,2,scorex,scorey,NULL,"OnOff");
	WSButton noteon(20,19,2,2,scorex,scorey,NULL,"On");
	WSButton noteoff(22,19,2,2,scorex,scorey,NULL,"Off");
	WSButton clear(24,19,2,2,scorex,scorey,NULL,"Clear");
	WSButton edit(26,19,2,2,scorex,scorey,edit_image,"");
	WSButton bpm10ff(20,17,2,2,scorex,scorey,last_image,"");
	WSButton bpmff(18,17,2,2,scorex,scorey,right_image,"");
	WSButton bpmfb(14,17,2,2,scorex,scorey,left_image,"");
	WSButton bpm10fb(12,17,2,2,scorex,scorey,first_image,"");
	WSButton bpmcor10ff(32,17,2,2,scorex,scorey,last_image,"");
	WSButton bpmcorff(30,17,2,2,scorex,scorey,right_image,"");
	WSButton bpmcorfb(26,17,2,2,scorex,scorey,left_image,"");
	WSButton bpmcor10fb(24,17,2,2,scorex,scorey,first_image,"");
	WSButton ok(16,19,2,2,scorex,scorey,ok_image,"");
	WSButton cancel(18,19,2,2,scorex,scorey,cancel_image,"");
	WSButton settings_next(34,19,2,2,scorex,scorey,next_image,"");
	WSButton settings_prev(34,19,2,2,scorex,scorey,prev_image,"");
	WSButton settings_up(24,19,2,2,scorex,scorey,up_image,"");
	WSButton settings_down(26,19,2,2,scorex,scorey,down_image,"");
	WSButton extmidi(10,17,2,2,scorex,scorey,plug_image,"");
	WSButton songs(0,17,2,2,scorex,scorey,songs_image,"");
	WSButton showpattern(2,17,2,2,scorex,scorey,pattern_image,"");
	WSButton noteup(18,17,2,2,scorex,scorey,right_image,"");
	WSButton notedown(14,17,2,2,scorex,scorey,left_image,"");
	WSButton oktaveup(20,17,2,2,scorex,scorey,last_image,"");
	WSButton oktavedown(12,17,2,2,scorex,scorey,first_image,"");
	WSButton volumeup(30,17,2,2,scorex,scorey,right_image,"");
	WSButton volumedown(26,17,2,2,scorex,scorey,left_image,"");
	WSButton volume10up(32,17,2,2,scorex,scorey,last_image,"");
	WSButton volume10down(24,17,2,2,scorex,scorey,first_image,"");
	WSButton programup(18,17,2,2,scorex,scorey,right_image,"");
	WSButton programdown(14,17,2,2,scorex,scorey,left_image,"");
	WSButton program10up(20,17,2,2,scorex,scorey,last_image,"");
	WSButton program10down(12,17,2,2,scorex,scorey,first_image,"");

	songs.aktiv=true;

	vector <WSButton> pattern_up;
	vector <WSButton> pattern_down;

	for(int i=0;i<5;i++)
	{
		WSButton tmp1(8+(6*i),14,2,2,scorex,scorey,right_image,"");
		pattern_up.push_back(tmp1);
	}
	for(int i=0;i<5;i++)
	{
		WSButton tmp1(6+(6*i),14,2,2,scorex,scorey,NULL,"");
		pattern_aktpatt.push_back(tmp1);
	}
	for(int i=0;i<5;i++)
	{
		WSButton tmp1(4+(6*i),14,2,2,scorex,scorey,left_image,"");
		pattern_down.push_back(tmp1);
	}

	for(int i=0;i<4;i++)
	{
		for(int j=0;j<4;j++)
		{
			sprintf(tmp, "%d %s", ((i+1)+4*j), "test");
			WSButton tmp1(1+9*i,5+(2*j),8,2,scorex,scorey,NULL,tmp);
			load_song.push_back(tmp1);
		}
	}

	int save_song=0;

	vector <WSButton> keyboard;
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,8,2,2,scorex,scorey,NULL,keyboard_letters[i]);
		keyboard.push_back(tmp);
	}
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,10,2,2,scorex,scorey,NULL,keyboard_letters[i+10]);
		keyboard.push_back(tmp);
	}
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,12,2,2,scorex,scorey,NULL,keyboard_letters[i+20]);
		keyboard.push_back(tmp);
	}
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,14,2,2,scorex,scorey,NULL,keyboard_letters[i+30]);
		keyboard.push_back(tmp);
	}

	vector <WSButton> shkeyboard;
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,8,2,2,scorex,scorey,NULL,shkeyboard_letters[i]);
		shkeyboard.push_back(tmp);
	}
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,10,2,2,scorex,scorey,NULL,shkeyboard_letters[i+10]);
		shkeyboard.push_back(tmp);
	}
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,12,2,2,scorex,scorey,NULL,shkeyboard_letters[i+20]);
		shkeyboard.push_back(tmp);
	}
	for(int i=0;i<10;i++)
	{
		WSButton tmp(8+2*i,14,2,2,scorex,scorey,NULL,shkeyboard_letters[i+30]);
		shkeyboard.push_back(tmp);
	}

	WSButton key_shift(8,16,4,2,scorex,scorey,NULL,"Shift");
	WSButton key_space(12,16,12,2,scorex,scorey,NULL,"Space");
	WSButton key_backspace(26,16,2,2,scorex,scorey,left_image,"");

	char note[12][3] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "B", "C"};

	Clearpattern();

	int mousex = 0;
	int mousey = 0;

	bool anzeige = true;
	bool blinkmode = false;
	int blink = 0;

	bool changesettings = false;
	bool changesong = false;

	SDL_Rect songnamePosition;

	SDL_Rect patterndevicenamePosition;

	vector<unsigned char> message;
	vector<unsigned char> inmessage;
	vector<unsigned char> cc;
	message.push_back(0);
	message.push_back(0);
	message.push_back(0);

	vector<string> midioutname;

	// Check available Midi Out ports.
	cout << "Midi Out" << endl;
	int onPorts = midiout->getPortCount();
	if ( onPorts == 0 )
	{
		cout << "No ports available!" << endl;
	}
	else
	{
		for(int i=0;i<onPorts;i++)
		{
			midioutname.push_back(midiout->getPortName(i));
			cout << i << ": " << midiout->getPortName(i) << endl;
		}
	}

	vector<string> midiinname;

	// Check available Midi In ports.
	cout << "Midi In" << endl;
	int inPorts = midiin->getPortCount();
	if ( inPorts == 0 )
	{
		cout << "No ports available!" << endl;
	}
	else
	{
		for(int i=0;i<inPorts;i++)
		{
			midiinname.push_back(midiin->getPortName(i));
			cout << i << ": " << midiin->getPortName(i) << endl;
		}
	}

	// MIDI IN Device
	if(aset[5].mididevice<inPorts)
	{
		midiin->openPort( aset[5].mididevice );
		midiin->setCallback( &midiincallback );
		// Don't ignore sysex, timing, or active sensing messages.
		midiin->ignoreTypes( false, false, false );
	}

	while(run)
	{
		if(oldstep!=aktstep)
		{
			anzeige=true;
		}
		blink++;
		if(blink>10)
		{
			if(blinkmode==true)
			{
				blinkmode=false;
			}
			else
			{
				blinkmode=true;
			}
			anzeige=true;
			blink=0;
		}

		if(anzeige==true)
		{
			SDL_FillRect(screen, NULL, 0x000000);
			boxColor(screen, 0,0,screen->w,1.5*scorey,0x00008FFF);
			SDL_FreeSurface(text);
			text = TTF_RenderText_Blended(fontbold, "*** SEQUENCEGANG 5.0 ***", textColor);
			textPosition.x = screen->w/2-text->w/2;
			textPosition.y = 0.75*scorey-text->h/2;
			SDL_BlitSurface(text, 0, screen, &textPosition);
			SDL_FreeSurface(text);
			text = TTF_RenderText_Blended(fontsmall, songname.c_str(), textColor);
			songnamePosition.x = screen->w-text->w-0.5*scorex;
			songnamePosition.y = 0.75*scorey-text->h/2;
			SDL_BlitSurface(text, 0, screen, &songnamePosition);

			if(mode==0)
			{
				SDL_FreeSurface(text);
				if(submode==0)
				{
					text = TTF_RenderText_Blended(fontsmall, "Song", textColor);
				}
				if(submode==1)
				{
					sprintf(tmp, "%s",aset[selpattdevice].name.c_str());
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
				}
				patterndevicenamePosition.x = 2*scorex;
				patterndevicenamePosition.y = 0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &patterndevicenamePosition);
				patterndevicenamePosition.h = scorey;
				patterndevicenamePosition.w = 10*scorex;

// CPU und RAM
				boxColor(screen, 0.2*scorex,0.25*scorey,0.4*scorex,1.25*scorey,0x2F2F2FFF);
				if(cpuusage>90)
				{
					boxColor(screen, 0.2*scorex,(0.25+(100-cpuusage)/100)*scorey,0.4*scorex,1.25*scorey,0xFF0000FF);
				}
				else if(cpuusage>80)
				{
					boxColor(screen, 0.2*scorex,(0.25+(100-cpuusage)/100)*scorey,0.4*scorex,1.25*scorey,0xFFFF00FF);
				}
				else
				{
					boxColor(screen, 0.2*scorex,(0.25+(100-cpuusage)/100)*scorey,0.4*scorex,1.25*scorey,0x00FF00FF);
				}

				boxColor(screen, 0.6*scorex,0.25*scorey,0.8*scorex,1.25*scorey,0x2F2F2FFF);
				if(float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram)<0.1)
				{
					boxColor(screen, 0.6*scorex,(0.25+float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram))*scorey,0.8*scorex,1.25*scorey,0xFF0000FF);
				}
				else if(float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram)<0.2)
				{
					boxColor(screen, 0.6*scorex,(0.25+float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram))*scorey,0.8*scorex,1.25*scorey,0xFFFF00FF);
				}
				else
				{
					boxColor(screen, 0.6*scorex,(0.25+float(memInfo.freeram+memInfo.bufferram+memInfo.sharedram)/float(memInfo.totalram))*scorey,0.8*scorex,1.25*scorey,0x00FF00FF);
				}

				for (int i=0;i<16;i++)
				{
// Taktanzeige
					if(aktstep==i and playmode!=0)
					{
						boxColor(screen, 4*scorex+(2*i)*scorex+3,1.5*scorey+3,6*scorex+(2*i)*scorex-3,2*scorey-3,0x00FF00FF);
					}
					else
					{
						boxColor(screen, 4*scorex+(2*i)*scorex+3,1.5*scorey+3,6*scorex+(2*i)*scorex-3,2*scorey-3,0x8F8F8FFF);
					}

// Postext
					SDL_FreeSurface(text);
					sprintf(tmp, "%d",i+1);
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
					textPosition.y = 3*scorey-text->h;
					SDL_BlitSurface(text, 0, screen, &textPosition);

// Pattern Raster
					if(submode==0)
					{
						for(int j=0;j<5;j++)
						{
							boxColor(screen, 4*scorex+(2*i)*scorex+3,3*scorey+(2*j)*scorey+3,6*scorex+(2*i)*scorex-3,5*scorey+(2*j)*scorey-3,0x8F8F8FFF);

							for(int k=0;k<5;k++)
							{
								if(pattern[j][selpattern[j]][i][k][0]==1)
								{
									boxColor(screen, 4*scorex+(2*i)*scorex+(2*scorex)*k/5+3,(5*scorey+(2*j)*scorey-3)-(((2*scorey-6)*pattern[j][selpattern[j]][i][k][2])/127), 4*scorex+(2*i)*scorex+(2*scorex)*(k+1)/5-3,5*scorey+(2*j)*scorey-3,0xFFFF8FFF);
								}
								if(pattern[j][selpattern[j]][i][k][0]==2)
								{
									boxColor(screen, 4*scorex+(2*i)*scorex+(2*scorex)*k/5+3,(5*scorey+(2*j)*scorey-3)-(((2*scorey-6)*pattern[j][selpattern[j]][i][k][2])/127), 4*scorex+(2*i)*scorex+(2*scorex)*(k+1)/5-3,5*scorey+(2*j)*scorey-3,0x8FFF8FFF);
								}
								if(pattern[j][selpattern[j]][i][k][0]==3)
								{
									boxColor(screen, 4*scorex+(2*i)*scorex+(2*scorex)*k/5+3,3*scorey+(2*j)*scorey+3, 4*scorex+(2*i)*scorex+(2*scorex)*(k+1)/5-3,5*scorey+(2*j)*scorey-3,0xFF8F8FFF);
								}
								if(pattern[j][selpattern[j]][i][k][0]==4)
								{
									boxColor(screen, 4*scorex+(2*i)*scorex+(2*scorex)*k/5+3,3*scorey+(2*j)*scorey+3, 4*scorex+(2*i)*scorex+(2*scorex)*(k+1)/5-3,5*scorey+(2*j)*scorey-3,0x8F8FFFFF);
								}
							}
						}
					}
// Pattern Details
					if(submode==1)
					{
						for(int j=0;j<5;j++)
						{
							if(edit.aktiv==true)
							{
								if(i==seleditstep and j==seleditcommand)
								{
									boxColor(screen, 4*scorex+(2*i)*scorex,3*scorey+(2*j)*scorey,6*scorex+(2*i)*scorex,5*scorey+(2*j)*scorey,0x00AF00FF);
								}
							}
							boxColor(screen, 4*scorex+(2*i)*scorex+3,3*scorey+(2*j)*scorey+3,6*scorex+(2*i)*scorex-3,5*scorey+(2*j)*scorey-3,0x8F8F8FFF);

							if(pattern[selpattdevice][selpattern[selpattdevice]][i][j][0]==1)
							{
								boxColor(screen, 4*scorex+(2*i)*scorex+3,5*scorey+(2*j)*scorey-3-(((2*scorey-6)*pattern[selpattdevice][selpattern[selpattdevice]][i][j][2])/127),6*scorex+(2*i)*scorex-3,5*scorey+(2*j)*scorey-3,0xFFFF8FFF);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "Note", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+4;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "OnOff", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+6+text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								sprintf(tmp, "%s%d",note[pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]-((pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]/12)*12)],pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]/12-2);
								text = TTF_RenderText_Blended(fontextrasmall, tmp, blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+8+2*text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
							}
							if(pattern[selpattdevice][selpattern[selpattdevice]][i][j][0]==2)
							{
								boxColor(screen, 4*scorex+(2*i)*scorex+3,5*scorey+(2*j)*scorey-3-(((2*scorey-6)*pattern[selpattdevice][selpattern[selpattdevice]][i][j][2])/127),6*scorex+(2*i)*scorex-3,5*scorey+(2*j)*scorey-3,0x8FFF8FFF);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "Note", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+4;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "On", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+6+text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								sprintf(tmp, "%s%d",note[pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]-((pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]/12)*12)],pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]/12-2);
								text = TTF_RenderText_Blended(fontextrasmall, tmp, blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+8+2*text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
							}
							if(pattern[selpattdevice][selpattern[selpattdevice]][i][j][0]==3)
							{
								boxColor(screen, 4*scorex+(2*i)*scorex+3,3*scorey+(2*j)*scorey+3,6*scorex+(2*i)*scorex-3,5*scorey+(2*j)*scorey-3,0xFF8F8FFF);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "Note", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+4;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "Off", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+6+text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								sprintf(tmp, "%s%d",note[pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]-((pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]/12)*12)],pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]/12-2);
								text = TTF_RenderText_Blended(fontextrasmall, tmp, blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+8+2*text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
							}
							if(pattern[selpattdevice][selpattern[selpattdevice]][i][j][0]==4)
							{
								boxColor(screen, 4*scorex+(2*i)*scorex+3,3*scorey+(2*j)*scorey+3,6*scorex+(2*i)*scorex-3,5*scorey+(2*j)*scorey-3,0x8F8FFFFF);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "Program", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+4;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								text = TTF_RenderText_Blended(fontextrasmall, "Change", blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+6+text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
								SDL_FreeSurface(text);
								sprintf(tmp, "%d",pattern[selpattdevice][selpattern[selpattdevice]][i][j][1]+1);
								text = TTF_RenderText_Blended(fontextrasmall, tmp, blackColor);
								textPosition.x = 5*scorex+(2*i)*scorex-text->w/2;
								textPosition.y = 3*scorey+(2*j)*scorey+8+2*text->h;
								SDL_BlitSurface(text, 0, screen, &textPosition);
							}
						}
					}
				}

// Devices
				if(submode==0)
				{
					for(int i=0;i<5;i++)
					{
						SDL_FreeSurface(text);
						sprintf(tmp, "%s",aset[i].name.c_str());
						text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
						textPosition.x = 0.2*scorex;
						textPosition.y = 4*scorey+(2*i)*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
					}
				}
				if(submode==1)
				{
					for(int i=0;i<5;i++)
					{
						SDL_FreeSurface(text);
						sprintf(tmp, "Command %d",i+1);
						text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
						textPosition.x = 0.2*scorex;
						textPosition.y = 4*scorey+(2*i)*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);
					}
				}

// Pattern
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmall, "Pattern", textColor);
				textPosition.x = 0.2*scorex;
				textPosition.y = 15*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				for(int i=0;i<5;i++)
				{
					pattern_aktpatt[i].show(screen, fontsmall);
					if(pattern_aktpatt[i].aktiv==true)
					{
						nextselpattern[i]=aktpatt;
					}
					
					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(fontsmall, aset[i].name.c_str(), textColor);
					textPosition.x = 7*scorex+6*i*scorex-text->w/2;
					textPosition.y = 14*scorey-text->h;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					if(selpattern[i]==nextselpattern[i])
					{
						sprintf(tmp, "%d",selpattern[i]+1);
					}
					else
					{
						if(blinkmode==true)
						{
							sprintf(tmp, "%s"," ");
						}
						else
						{
							sprintf(tmp, "%d",nextselpattern[i]+1);
						}
					}
					text = TTF_RenderText_Blended(fontbold, tmp, blackColor);
					textPosition.x = 7*scorex+6*i*scorex-text->w/2;
					textPosition.y = 15*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					pattern_down[i].show(screen, fontsmall);
					pattern_up[i].show(screen, fontsmall);
				}

// BPM
				if(submode==0)
				{
					if(clockmodeext==false)
					{
						SDL_FreeSurface(text);
						text = TTF_RenderText_Blended(fontsmall, "BPM", textColor);
						textPosition.x = 17*scorex-text->w/2;
						textPosition.y = 17*scorey-text->h;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%d",bpm+60);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 17*scorex-text->w/2;
						textPosition.y = 18*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						bpm10ff.show(screen, fontsmall);
						bpmff.show(screen, fontsmall);
						bpmfb.show(screen, fontsmall);
						bpm10fb.show(screen, fontsmall);

						SDL_FreeSurface(text);
						text = TTF_RenderText_Blended(fontsmall, "BPM correction", textColor);
						textPosition.x = 29*scorex-text->w/2;
						textPosition.y = 17*scorey-text->h;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%.2f",bpmcorrect);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 29*scorex-text->w/2;
						textPosition.y = 18*scorey-text->h/2;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						bpmcor10ff.show(screen, fontsmall);
						bpmcorff.show(screen, fontsmall);
						bpmcorfb.show(screen, fontsmall);
						bpmcor10fb.show(screen, fontsmall);
					}
					extmidi.show(screen, fontsmall);
				}

// Pattern Note
				if(submode==1)
				{
//					noteonoff.aktiv==true;
//					noteon.aktiv==true;
//					noteoff.aktiv==true;

					if(edit.aktiv==true)
					{
						if(noteonoff.aktiv==true or noteon.aktiv==true or noteoff.aktiv)
						{
							SDL_FreeSurface(text);
							if(aset[selpattdevice].midichannel==9 and aset[selpattdevice].mididevice==255)
							{
								if(akteditnote>=35 and akteditnote<=81)
								{
									sprintf(tmp, "%s",gm_drum_name[akteditnote-35]);
									text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
								}
								else
								{
									text = TTF_RenderText_Blended(fontsmall, "Drum", textColor);
								}
							}
							else
							{
								text = TTF_RenderText_Blended(fontsmall, "Note", textColor);
							}	
							textPosition.x = 17*scorex-text->w/2;
							textPosition.y = 17*scorey-text->h;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							SDL_FreeSurface(text);
							sprintf(tmp, "%s%d",note[akteditnote-((akteditnote/12)*12)],akteditnote/12-2);
							text = TTF_RenderText_Blended(font, tmp, textColor);
							textPosition.x = 17*scorex-text->w/2;
							textPosition.y = 18*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							oktavedown.show(screen, fontsmall);
							notedown.show(screen, fontsmall);
							noteup.show(screen, fontsmall);
							oktaveup.show(screen, fontsmall);
						}
	// Pattern Volume
						if(noteonoff.aktiv==true or noteon.aktiv==true)
						{
							SDL_FreeSurface(text);
							text = TTF_RenderText_Blended(fontsmall, "Volume", textColor);
							textPosition.x = 29*scorex-text->w/2;
							textPosition.y = 17*scorey-text->h;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							SDL_FreeSurface(text);
							sprintf(tmp, "%d",akteditvolume);
							text = TTF_RenderText_Blended(font, tmp, textColor);
							textPosition.x = 29*scorex-text->w/2;
							textPosition.y = 18*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							volumedown.show(screen, fontsmall);
							volume10down.show(screen, fontsmall);
							volumeup.show(screen, fontsmall);
							volume10up.show(screen, fontsmall);
						}
// Program
						if(program.aktiv==true)
						{
							SDL_FreeSurface(text);
							text = TTF_RenderText_Blended(fontsmall, "Program", textColor);
							textPosition.x = 17*scorex-text->w/2;
							textPosition.y = 17*scorey-text->h;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							SDL_FreeSurface(text);
							sprintf(tmp, "%d",akteditprogram+1);
							text = TTF_RenderText_Blended(font, tmp, textColor);
							textPosition.x = 17*scorex-text->w/2;
							textPosition.y = 18*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							SDL_FreeSurface(text);
							if(aset[selpattdevice].midichannel==9 and aset[selpattdevice].mididevice==255)
							{
								if(akteditprogram==0)
								{
									sprintf(tmp, "%s","Standard Kit");
								}
								else if(akteditprogram==8)
								{
									sprintf(tmp, "%s","Room Kit");
								}
								else if(akteditprogram==16)
								{
									sprintf(tmp, "%s","Power Kit");
								}
								else if(akteditprogram==24)
								{
									sprintf(tmp, "%s","Electronic Kit");
								}
								else if(akteditprogram==25)
								{
									sprintf(tmp, "%s","TR-808 Kit");
								}
								else if(akteditprogram==32)
								{
									sprintf(tmp, "%s","Jazz Kit");
								}
								else if(akteditprogram==40)
								{
									sprintf(tmp, "%s","Brush Kit");
								}
								else if(akteditprogram==48)
								{
									sprintf(tmp, "%s","Orchestra Kit");
								}
								else if(akteditprogram==56)
								{
									sprintf(tmp, "%s","Sound FX Kit");
								}
								else if(akteditprogram==127)
								{
									sprintf(tmp, "%s","Percussion");
								}
								else
								{
									sprintf(tmp, "%s","Drum unknown");
								}
							}
							else
							{
								sprintf(tmp, "%s",gm_program_name[akteditprogram]);
							}
							text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
							textPosition.x = 24*scorex;
							textPosition.y = 18*scorey-text->h/2;
							SDL_BlitSurface(text, 0, screen, &textPosition);

							programdown.show(screen, fontsmall);
							program10down.show(screen, fontsmall);
							programup.show(screen, fontsmall);
							program10up.show(screen, fontsmall);
						}
					}
				}


// Songs / Pattern

				songs.show(screen, fontsmall);
				showpattern.show(screen, fontsmall);
// Exit Info

				exit.show(screen, fontsmall);
				info.show(screen, fontsmall);
				settings.show(screen, fontsmall);
				open.show(screen, fontsmall);
				save.show(screen, fontsmall);

// NoteOn / NoteOff / Clear Umschalter / Edit
				if(submode==1)
				{
					program.show(screen, fontsmall);
					noteonoff.show(screen, fontsmall);
					noteon.show(screen, fontsmall);
					noteoff.show(screen, fontsmall);
					clear.show(screen, fontsmall);
					edit.show(screen, fontsmall);
				}

// Play Stop Pause
				if(playmode==1)
				{
					pause.show(screen, fontsmall);
				}
				else
				{
					start.show(screen, fontsmall);
				}
				stop.show(screen, fontsmall);

// Debug
				if(debug==true)
				{
				}
			}

			if(mode==1) // Info
			{
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "(c) 1987-2020 by Wolfgang Schuster", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 2*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmall, "GNU General Public License v3.0", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 3*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				int i = 0;
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmallbold, "MIDI OUT", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = (5+i)*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				i++;
				for(auto &mout: midioutname)
				{
					SDL_FreeSurface(text);
					sprintf(tmp, "%s",mout.c_str());
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 2*scorex;
					textPosition.y = (5+i)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					i++;
				}
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmallbold, "MIDI IN", textColor);
				textPosition.x = 2*scorex;
				textPosition.y = (5+i)*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				i++;
				for(auto &min: midiinname)
				{
					SDL_FreeSurface(text);
					sprintf(tmp, "%s",min.c_str());
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 2*scorex;
					textPosition.y = (5+i)*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					i++;
				}
				
				i = 0;
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmallbold, "FLUIDSYNTH", textColor);
				textPosition.x = 20*scorex;
				textPosition.y = (5+i)*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);
				
				fluid_settings_bank_rect.clear();
				fluid_settings_program_rect.clear();
			    for(auto &fptmp: fluid_program_state)
			    {
					SDL_FreeSurface(text);
					sprintf(tmp, "%d",fptmp.channel+1);
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 20*scorex-text->w/2;
					textPosition.y = (i+8)*0.75*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					sprintf(tmp, "%d",fptmp.bank);
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 22*scorex-text->w/2;
					textPosition.y = (i+8)*0.75*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					fluid_settings_bank_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(tmp, "%d",fptmp.program);
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 24*scorex-text->w/2;
					textPosition.y = (i+8)*0.75*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					fluid_settings_program_rect.push_back(textPosition);

					SDL_FreeSurface(text);
					sprintf(tmp, "%s",gm_program_name[fptmp.program]);
					text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
					textPosition.x = 26*scorex;
					textPosition.y = (i+8)*0.75*scorey-text->h/2;
					SDL_BlitSurface(text, 0, screen, &textPosition);
					fluid_settings_program_rect.push_back(textPosition);

			    	i++;
			    }
				
		    	i++;
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmallbold, "MIDI-Channels", textColor);
				textPosition.x = 20*scorex;
				textPosition.y = (i+8)*0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(tmp, "%d",fluid_nmid_chan);
				text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
				textPosition.x = 28*scorex;
				textPosition.y = (i+8)*0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

		    	i++;
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmallbold, "Audio ALSA-Device", textColor);
				textPosition.x = 20*scorex;
				textPosition.y = (i+8)*0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(tmp, "%s",fluid_alsa_device);
				text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
				textPosition.x = 28*scorex;
				textPosition.y = (i+8)*0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

		    	i++;
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontsmallbold, "CPU Load", textColor);
				textPosition.x = 20*scorex;
				textPosition.y = (i+8)*0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(tmp, "%d",int(fluid_synth_get_cpu_load(fluid_synth)));
				text = TTF_RenderText_Blended(fontsmall, tmp, textColor);
				textPosition.x = 28*scorex;
				textPosition.y = (i+8)*0.75*scorey-text->h/2;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				ok.show(screen, fontsmall);
			}

			if(mode==2) // Exit
			{
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Really Exit ?", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 10*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				ok.show(screen, fontsmall);
				cancel.show(screen, fontsmall);
			}

			if(mode==3) // Settings
			{
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Settings", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 2*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				boxColor(screen, 0,3*scorey,screen->w,4*scorey,0x00008FFF);

				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(font, "Name", textColor);
				textPosition.x = 0.2*scorex;
				textPosition.y = 3*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(settingsmode==0)
				{
					if(selsetmididevice>0)
					{
						boxColor(screen, selsetmidideviceRect.x,selsetmidideviceRect.y+2*(selsetmididevice-1)*scorey,selsetmidideviceRect.x+selsetmidideviceRect.w,selsetmidideviceRect.y+2*selsetmididevice*scorey,0x4F4F4FFF);
					}
					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "MIDI Device", textColor);
					textPosition.x = 6*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					if(selsetmidichannel>0)
					{
						boxColor(screen, selsetmidichannelRect.x,selsetmidichannelRect.y+2*(selsetmidichannel-1)*scorey,selsetmidichannelRect.x+selsetmidichannelRect.w,selsetmidichannelRect.y+2*selsetmidichannel*scorey,0x4F4F4FFF);
					}
					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "MIDI Channel", textColor);
					textPosition.x = 30*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}
				else if (settingsmode==1)
				{
					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "min Prog", textColor);
					textPosition.x = 7*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "max Prog", textColor);
					textPosition.x = 13*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "min Bank", textColor);
					textPosition.x = 19*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "max Bank", textColor);
					textPosition.x = 25*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, "MIDI In Ch", textColor);
					textPosition.x = 31*scorex;
					textPosition.y = 3*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);
				}

				for(int i=0;i<6;i++)
				{
					SDL_FreeSurface(text);
					text = TTF_RenderText_Blended(font, aset[i].name.c_str(), textColor);
					textPosition.x = 0.2*scorex;
					textPosition.y = 5*scorey+2*i*scorey;
					SDL_BlitSurface(text, 0, screen, &textPosition);

					if(settingsmode==0)
					{
						SDL_FreeSurface(text);
						if(i<5)
						{
							if(int(aset[i].mididevice)==255)
							{
								sprintf(tmp, "%s","Fluidsynth");
							}
							else if(int(aset[i].mididevice)<onPorts)
							{
								sprintf(tmp, "%s",midioutname[aset[i].mididevice].c_str());
							}
							else
							{
								sprintf(tmp, "%s","Mididevice not available");
							}
						}
						else
						{
							if(int(aset[i].mididevice)<inPorts)
							{
								sprintf(tmp, "%s",midiinname[aset[i].mididevice].c_str());
							}
							else
							{
								sprintf(tmp, "%s","Mididevice not available");
							}
						}
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 6*scorex;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%d",aset[i].midichannel+1);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 33*scorex-text->w;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);
					}
					else if (settingsmode==1 and i<5)
					{
						SDL_FreeSurface(text);
						sprintf(tmp, "%d",aset[i].minprog);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x =10*scorex-text->w;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%d",aset[i].maxprog);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 16*scorex-text->w;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%d",aset[i].minbank);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 22*scorex-text->w;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%d",aset[i].maxbank);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 28*scorex-text->w;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);

						SDL_FreeSurface(text);
						sprintf(tmp, "%d",aset[i].midiinchannel+1);
						text = TTF_RenderText_Blended(font, tmp, textColor);
						textPosition.x = 34*scorex-text->w;
						textPosition.y = 5*scorey+2*i*scorey;
						SDL_BlitSurface(text, 0, screen, &textPosition);

					}

				}

				ok.show(screen, fontsmall);
				cancel.show(screen, fontsmall);

				if(settingsmode==0)
				{
					settings_next.show(screen, fontsmall);
				}
				else
				{
					settings_prev.show(screen, fontsmall);
				}

				settings_up.show(screen, fontsmall);
				settings_down.show(screen, fontsmall);

			}

			if(mode==4)  // Open Song
			{
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Open Song", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 2*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				for(int i=0;i<16;i++)
				{
					load_song[i].show(screen, fontsmall);
				}

				ok.show(screen, fontsmall);
				cancel.show(screen, fontsmall);
			}

			if(mode==6)  // save Song
			{
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Save Song", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 2*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				for(int i=0;i<16;i++)
				{
					load_song[i].show(screen, fontsmall);
				}

				ok.show(screen, fontsmall);
				cancel.show(screen, fontsmall);
			}

			if(mode==7)  // Change Songname
			{
				SDL_FreeSurface(text);
				text = TTF_RenderText_Blended(fontbold, "Change Songname", textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 2*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				SDL_FreeSurface(text);
				sprintf(tmp, "%s",songnametmp.c_str());
				text = TTF_RenderText_Blended(font, tmp, textColor);
				textPosition.x = screen->w/2-text->w/2;
				textPosition.y = 5*scorey;
				SDL_BlitSurface(text, 0, screen, &textPosition);

				if(blinkmode==true)
				{
					SDL_Surface* texttmp;
					texttmp = TTF_RenderText_Blended(font, "_", textColor);
					textPosition.x = textPosition.x+text->w;
					textPosition.y = 5*scorey;
					SDL_BlitSurface(texttmp, 0, screen, &textPosition);
				}

				for(int i=0;i<40;i++)
        		{
        			if(key_shift.aktiv==false)
        			{
        				keyboard[i].show(screen, font);
        			}
        			else
        			{
        				shkeyboard[i].show(screen, font);
        			}

        		}
        		key_shift.show(screen, font);
        		key_space.show(screen, font);
        		key_backspace.show(screen, font);

				ok.show(screen, fontsmall);
				cancel.show(screen, fontsmall);
			}

			SDL_Flip(screen);
			anzeige=false;
		}

		// Wir holen uns so lange neue Ereignisse, bis es keine mehr gibt.
		SDL_Event event;
		while(SDL_PollEvent(&event))
		{
			// Was ist passiert?
			switch(event.type)
			{
				case SDL_QUIT:
					run = false;
					break;
				case SDL_KEYDOWN:
					keyPressed[event.key.keysym.sym] = true;
					if(keyPressed[SDLK_ESCAPE])
					{
						run = false;        // Programm beenden.
					}
					if(event.key.keysym.sym==SDLK_DOWN)
					{
						if(seleditcommand<4)
						{
							seleditcommand++;
						}
					}
					if(event.key.keysym.sym==SDLK_UP)
					{
						if(seleditcommand>0)
						{
							seleditcommand--;
						}
					}
					if(event.key.keysym.sym==SDLK_RIGHT)
					{
						if(seleditstep<15)
						{
							seleditstep++;
						}
					}
					if(event.key.keysym.sym==SDLK_LEFT)
					{
						if(seleditstep>0)
						{
							seleditstep--;
						}
					}
					
					anzeige=true;
					break;
				case SDL_MOUSEBUTTONDOWN:
			        if( event.button.button == SDL_BUTTON_LEFT )
			        {
			        	if(mode==0)
			        	{
							if(CheckMouse(mousex, mousey, songnamePosition)==true)
							{
								songnametmp=songname;
								mode=7;
							}
							else if(CheckMouse(mousex, mousey, songs.button_rect)==true)
							{
								submode=0;
								songs.aktiv=true;
								showpattern.aktiv=false;
								edit.aktiv=false;
								clear.aktiv=false;
								program.aktiv=false;
								noteonoff.aktiv=false;
								noteon.aktiv=false;
								noteoff.aktiv=false;
							}
							else if(CheckMouse(mousex, mousey, showpattern.button_rect)==true)
							{
								submode=1;
								songs.aktiv=false;
								showpattern.aktiv=true;
								edit.aktiv=false;
								program.aktiv=false;
								clear.aktiv=false;
								noteonoff.aktiv=false;
								noteon.aktiv=false;
								noteoff.aktiv=false;
							}
							else if(CheckMouse(mousex, mousey, start.button_rect)==true)
							{
								if(playmode==0)
								{
									wsmidi.Play();
								}
								else if(playmode==2)
								{
									playmode=1;
									timerrun=true;
								}
								else
								{
									playmode=2;
									timerrun=false;
								}
							}
							else if(CheckMouse(mousex, mousey, stop.button_rect)==true)
							{
								stop.aktiv = true;
								wsmidi.Stop();
							}
							else if(CheckMouse(mousex, mousey, info.button_rect)==true)
							{
								fluid_program_state.clear();
							    for(int i=0;i<16;i++)
								{
									if (FLUID_OK == fluid_synth_get_program (fluid_synth, i, &sfid, &fsbank, &fsprogram))
									{
										fptmp.channel=i;
										fptmp.bank=fsbank;
										fptmp.program=fsprogram;
										fluid_program_state.push_back(fptmp);
									}
								}

								mode=1;
							}
							else if(CheckMouse(mousex, mousey, exit.button_rect)==true)
							{
								mode=2;
							}
							else if(CheckMouse(mousex, mousey, settings.button_rect)==true)
							{
								mode=3;
							}
							else if(CheckMouse(mousex, mousey, open.button_rect)==true)
							{
								// Load SongDB
								songset.clear();
								sqlite3 *songsdb;
								sprintf(dbpath, "%s/.sequencegang5/songs.db", getenv("HOME"));
								if(sqlite3_open(dbpath, &songsdb) != SQLITE_OK)
								{
									cout << "Fehler beim Öffnen: " << sqlite3_errmsg(songsdb) << endl;
									return 1;
								}
								cout << "Songsdatenbank erfolgreich geöffnet!" << endl;
								sprintf(sql, "SELECT * FROM settings");
								if( sqlite3_exec(songsdb,sql,songpatternnamecallback,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim SELECT: " << sqlite3_errmsg(songsdb) << endl;
									return 1;
								}
								sqlite3_close(songsdb);

								for(int i=0;i<16;i++)
								{
									load_song[i].button_text = songset[i].name;
								}

								if(aktsong>0)
								{
									for(int i=0;i<16;i++)
									{
										load_song[i].aktiv=false;
										if(i+1==aktsong)
										{
											load_song[i].aktiv=true;
										}
									}
								}
								mode=4;
							}
							else if(CheckMouse(mousex, mousey, save.button_rect)==true)
							{
								// Load SongDB
								songset.clear();
								sqlite3 *songsdb;
								sprintf(dbpath, "%s/.sequencegang5/songs.db", getenv("HOME"));
								if(sqlite3_open(dbpath, &songsdb) != SQLITE_OK)
								{
									cout << "Fehler beim Öffnen: " << sqlite3_errmsg(songsdb) << endl;
									return 1;
								}
								cout << "Songsdatenbank erfolgreich geöffnet!" << endl;
								sprintf(sql, "SELECT * FROM settings");
								if( sqlite3_exec(songsdb,sql,songpatternnamecallback,0,0) != SQLITE_OK)
								{
									cout << "Fehler beim SELECT: " << sqlite3_errmsg(songsdb) << endl;
									return 1;
								}
								sqlite3_close(songsdb);

								for(int i=0;i<16;i++)
								{
									load_song[i].button_text = songset[i].name;
								}
								save_song=0;
								mode=6;
							}
							for(int i=0;i<5;i++)
							{
								if(CheckMouse(mousex, mousey, pattern_aktpatt[i].button_rect)==true)
								{
									if(pattern_aktpatt[i].aktiv==true)
									{
										for(int i=0;i<5;i++)
										{
											pattern_aktpatt[i].aktiv=false;
										}
									}
									else
									{
										for(int i=0;i<5;i++)
										{
											pattern_aktpatt[i].aktiv=false;
										}
										pattern_aktpatt[i].aktiv=true;
										aktpatt=selpattern[i];
									}

								}
								if(CheckMouse(mousex, mousey, pattern_up[i].button_rect)==true)
								{
									pattern_up[i].aktiv=true;
									if(nextselpattern[i]<15)
									{
										nextselpattern[i]++;
									}
									if(playmode==0)
									{
										selpattern[i]=nextselpattern[i];
									}
								}
								if(CheckMouse(mousex, mousey, pattern_down[i].button_rect)==true)
								{
									pattern_down[i].aktiv=true;
									if(nextselpattern[i]>0)
									{
										nextselpattern[i]--;
									}
									if(playmode==0)
									{
										selpattern[i]=nextselpattern[i];
									}
								}
							}
			        		if(submode==0)
			        		{
								if(CheckMouse(mousex, mousey, bpmff.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpmff.aktiv=true;
										if(bpm<255)
										{
											bpm++;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpmfb.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpmfb.aktiv=true;
										if(bpm>0)
										{
											bpm--;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpm10ff.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpm10ff.aktiv=true;
										if(bpm<245)
										{
											bpm=bpm+10;;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpm10fb.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpm10fb.aktiv=true;
										if(bpm>9)
										{
											bpm=bpm-10;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpmcorff.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpmcorff.aktiv=true;
										if(bpmcorrect<2.0)
										{
											bpmcorrect=bpmcorrect+0.01;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpmcorfb.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpmcorfb.aktiv=true;
										if(bpm>-2.0)
										{
											bpmcorrect=bpmcorrect-0.01;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpmcor10ff.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpmcor10ff.aktiv=true;
										if(bpmcorrect<2.0)
										{
											bpmcorrect=bpmcorrect+0.10;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, bpmcor10fb.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										bpmcor10fb.aktiv=true;
										if(bpmcorrect>-2.0)
										{
											bpmcorrect=bpmcorrect-0.10;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, extmidi.button_rect)==true)
								{
									if(clockmodeext==false)
									{
										clockmodeext=true;
										extmidi.aktiv=true;
									}
									else
									{
										clockmodeext=false;
										extmidi.aktiv=false;
									}
								}
								else if(CheckMouse(mousex, mousey, rahmen)==true)
								{
									int i = int((mousey/scorey-3)/2);
									int j = int((mousex/scorex-4)/2);

									selpattdevice = i;
									seleditstep = j;
									seleditcommand = 0;
									submode=1;
									songs.aktiv=false;
									showpattern.aktiv=true;
									edit.aktiv=false;
									program.aktiv=false;
									clear.aktiv=false;
									noteonoff.aktiv=false;
									noteon.aktiv=false;
									noteoff.aktiv=false;
								}
			        		}
			        		if(submode==1)
			        		{
								if(CheckMouse(mousex, mousey, noteup.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true or noteoff.aktiv)
									{
										noteup.aktiv=true;
										if(akteditnote<127)
										{
											akteditnote++;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, notedown.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true or noteoff.aktiv)
									{
										notedown.aktiv=true;
										if(akteditnote>0)
										{
											akteditnote--;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, oktaveup.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true or noteoff.aktiv)
									{
										oktaveup.aktiv=true;
										if(akteditnote<115)
										{
											akteditnote=akteditnote+12;
										}
										else
										{
											akteditnote=127;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, oktavedown.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true or noteoff.aktiv)
									{
										oktavedown.aktiv=true;
										if(akteditnote>12)
										{
											akteditnote=akteditnote-12;
										}
										else
										{
											akteditnote=0;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, volumeup.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true)
									{
										volumeup.aktiv=true;
										if(akteditvolume<127)
										{
											akteditvolume++;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, volumedown.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true)
									{
										volumedown.aktiv=true;
										if(akteditvolume>0)
										{
											akteditvolume--;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, volume10up.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true)
									{
										volume10up.aktiv=true;
										if(akteditvolume<117)
										{
											akteditvolume=akteditvolume+10;
										}
										else
										{
											akteditvolume=127;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, volume10down.button_rect)==true)
								{
									if(noteonoff.aktiv==true or noteon.aktiv==true)
									{
										volume10down.aktiv=true;
										if(akteditvolume>10)
										{
											akteditvolume=akteditvolume-10;
										}
										else
										{
											akteditvolume=0;
										}
									}
								}
								if(CheckMouse(mousex, mousey, programup.button_rect)==true)
								{
									if(program.aktiv==true)
									{
										programup.aktiv=true;
										if(akteditprogram<127)
										{
											akteditprogram++;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, programdown.button_rect)==true)
								{
									if(program.aktiv==true)
									{
										programdown.aktiv=true;
										if(akteditprogram>0)
										{
											akteditprogram--;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, program10up.button_rect)==true)
								{
									if(program.aktiv==true)
									{
										program10up.aktiv=true;
										if(akteditprogram<117)
										{
											akteditprogram=akteditprogram+10;
										}
										else
										{
											akteditprogram=127;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, program10down.button_rect)==true)
								{
									if(program.aktiv==true)
									{
										program10down.aktiv=true;
										if(akteditprogram>10)
										{
											akteditprogram=akteditprogram-10;
										}
										else
										{
											akteditprogram=0;
										}
									}
								}
								if(CheckMouse(mousex, mousey, patterndevicenamePosition)==true)
								{
									if(selpattdevice<4)
									{
										selpattdevice++;
									}
									else
									{
										selpattdevice=0;
									}
								}
								else if(CheckMouse(mousex, mousey, program.button_rect)==true and edit.aktiv==true)
								{
									program.aktiv=true;
									clear.aktiv=false;
									noteonoff.aktiv=false;
									noteon.aktiv=false;
									noteoff.aktiv=false;
								}
								else if(CheckMouse(mousex, mousey, noteonoff.button_rect)==true and edit.aktiv==true)
								{
									program.aktiv=false;
									clear.aktiv=false;
									noteonoff.aktiv=true;
									noteon.aktiv=false;
									noteoff.aktiv=false;
								}
								else if(CheckMouse(mousex, mousey, noteon.button_rect)==true and edit.aktiv==true)
								{
									program.aktiv=false;
									clear.aktiv=false;
									noteonoff.aktiv=false;
									noteon.aktiv=true;
									noteoff.aktiv=false;
								}
								else if(CheckMouse(mousex, mousey, noteoff.button_rect)==true and edit.aktiv==true)
								{
									program.aktiv=false;
									clear.aktiv=false;
									noteonoff.aktiv=false;
									noteon.aktiv=false;
									noteoff.aktiv=true;
								}
								else if(CheckMouse(mousex, mousey, clear.button_rect)==true and edit.aktiv==true)
								{
									program.aktiv=false;
									clear.aktiv=true;
									noteonoff.aktiv=false;
									noteon.aktiv=false;
									noteoff.aktiv=false;
								}
								else if(CheckMouse(mousex, mousey, edit.button_rect)==true)
								{
									if(edit.aktiv==true)
									{
										edit.aktiv=false;
										program.aktiv=false;
										clear.aktiv=false;
										noteonoff.aktiv=false;
										noteon.aktiv=false;
										noteoff.aktiv=false;
									}
									else
									{
										edit.aktiv=true;
									}
								}
								else if(CheckMouse(mousex, mousey, rahmen)==true)
								{
									int i = int((mousey/scorey-3)/2);
									int j = int((mousex/scorex-4)/2);
									seleditstep = j;
									seleditcommand = i;

									if(clear.aktiv==true and edit.aktiv==true)
									{
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][0]=0;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][1]=0;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][2]=0;
									}
									else if(noteonoff.aktiv==true and edit.aktiv==true)
									{
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][0]=1;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][1]=akteditnote;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][2]=akteditvolume;
									}
									else if(noteon.aktiv==true and edit.aktiv==true)
									{
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][0]=2;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][1]=akteditnote;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][2]=akteditvolume;
									}
									else if(noteoff.aktiv==true and edit.aktiv==true)
									{
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][0]=3;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][1]=akteditnote;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][2]=0;
									}
									else if(program.aktiv==true and edit.aktiv==true)
									{
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][0]=4;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][1]=akteditprogram;
										pattern[selpattdevice][selpattern[selpattdevice]][seleditstep][seleditcommand][2]=0;
									}
								}
			        		}
			        	}
			        	else if(mode==1)
			        	{
							if(CheckMouse(mousex, mousey, ok.button_rect)==true)
							{
								mode=0;
							}
			        	}
			        	else if(mode==2)
			        	{
							if(CheckMouse(mousex, mousey, ok.button_rect)==true)
							{
								run = false;        // Programm beenden.
							}
							if(CheckMouse(mousex, mousey, cancel.button_rect)==true)
							{
								mode=0;
							}
			        	}
			        	else if(mode==3)  // Settings
			        	{
							if(CheckMouse(mousex, mousey, ok.button_rect)==true)
							{
								if(changesettings==true)
								{
									cout << "Save Settings" << endl;

									sprintf(dbpath, "%s/.sequencegang5/settings.db", getenv("HOME"));
									if(sqlite3_open("settings.db", &settingsdb) != SQLITE_OK)
									{
										cout << "Fehler beim Öffnen: " << sqlite3_errmsg(settingsdb) << endl;
										return 1;
									}
									cout << "Datenbank erfolgreich geöffnet!" << endl;

									// Settings
									int i = 1;
									for(seqsettings asettemp: aset)
									{
	//									sprintf(sql, "UPDATE settings SET name = \"%s\"  WHERE id = %d",asettemp.name,i);
	//									if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
	//									{
	//										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
	//										return 1;
	//									}
										sprintf(sql, "UPDATE settings SET mididevice = \"%d\" WHERE id = %d",asettemp.mididevice,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										sprintf(sql, "UPDATE settings SET midichannel = \"%d\" WHERE id = %d",asettemp.midichannel,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										sprintf(sql, "UPDATE settings SET maxbank = \"%d\" WHERE id = %d",asettemp.maxbank,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										sprintf(sql, "UPDATE settings SET maxprog = \"%d\" WHERE id = %d",asettemp.maxprog,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										sprintf(sql, "UPDATE settings SET minbank = \"%d\" WHERE id = %d",asettemp.minbank,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										sprintf(sql, "UPDATE settings SET minprog = \"%d\" WHERE id = %d",asettemp.minprog,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										sprintf(sql, "UPDATE settings SET master = \"%d\" WHERE id = %d",asettemp.master,i);
										if( sqlite3_exec(settingsdb,sql,NULL,0,0) != SQLITE_OK)
										{
											cout << "Fehler beim UPDATE: " << sqlite3_errmsg(settingsdb) << endl;
											return 1;
										}
										i++;

									}
									sqlite3_close(settingsdb);
									changesettings=false;
								}
								mode=0;
							}
							if(CheckMouse(mousex, mousey, cancel.button_rect)==true)
							{
								mode=0;
							}

							if(settingsmode==0)
							{
								if(CheckMouse(mousex, mousey, settings_next.button_rect)==true)
								{
									settingsmode=1;
									selsetmidichannel=0;
									selsetmididevice=0;
								}
								else if(CheckMouse(mousex, mousey, selsetmidideviceRect)==true)
								{
									selsetmididevice=(mousey-selsetmidideviceRect.y)/scorey/2+1;
									selsetmidichannel=0;
								}
								else if(CheckMouse(mousex, mousey, selsetmidichannelRect)==true)
								{
									selsetmidichannel=(mousey-selsetmidichannelRect.y)/scorey/2+1;
									selsetmididevice=0;
								}
								else if(CheckMouse(mousex, mousey, settings_up.button_rect)==true)
								{
									settings_up.aktiv=true;
									if(selsetmididevice<6) // MidiOut
									{
										if(aset[selsetmididevice-1].mididevice==255)
										{
											aset[selsetmididevice-1].mididevice=0;
											changesettings=true;
										}
										else if(aset[selsetmididevice-1].mididevice<midiout->getPortCount()-1)
										{
											aset[selsetmididevice-1].mididevice++;
											changesettings=true;
										}
									}
									else // MidiIn
									{
										if(aset[selsetmididevice-1].mididevice<midiin->getPortCount()-1)
										{
											aset[selsetmididevice-1].mididevice++;
											changesettings=true;
										}
									}
									if(selsetmidichannel>0)
									{
										if(aset[selsetmidichannel-1].midichannel<15)
										{
											aset[selsetmidichannel-1].midichannel++;
											changesettings=true;
										}
									}
								}
								else if(CheckMouse(mousex, mousey, settings_down.button_rect)==true)
								{
									settings_down.aktiv=true;
									if(aset[selsetmididevice-1].mididevice==0)
									{
										aset[selsetmididevice-1].mididevice=255;
										changesettings=true;
									}
									else if(aset[selsetmididevice-1].mididevice>0 and aset[selsetmididevice-1].mididevice<255)
									{
										aset[selsetmididevice-1].mididevice--;
										changesettings=true;
									}
									if(selsetmidichannel>0)
									{
										if(aset[selsetmidichannel-1].midichannel>0)
										{
											aset[selsetmidichannel-1].midichannel--;
											changesettings=true;
										}
									}
								}
								else
								{
									selsetmididevice=0;
								}
							}
							else if(settingsmode==1)
							{
								if(CheckMouse(mousex, mousey, settings_prev.button_rect)==true)
								{
									settingsmode=0;
								}
							}
			        	}
			        	else if(mode==4)  // Load
			        	{
							if(CheckMouse(mousex, mousey, ok.button_rect)==true)
							{
								for(int i=0;i<16;i++)
								{
									if(load_song[i].aktiv==true)
									{
										LoadScene(i);
									}
								}
								mode=0;
							}
							if(CheckMouse(mousex, mousey, cancel.button_rect)==true)
							{
								mode=0;
							}
							for(int i=0;i<16;i++)
							{
								load_song[i].aktiv=false;
								if(CheckMouse(mousex, mousey, load_song[i].button_rect)==true)
								{
									load_song[i].aktiv=true;
								}

							}
			        	}
			        	else if(mode==6 ) //Save
			        	{
			        		if(CheckMouse(mousex, mousey, ok.button_rect)==true)
							{
			        			for(int i=0;i<16;i++)
			        			{
			        				if(load_song[i].aktiv==true)
			        				{
			        					save_song=i+1;
			        				}
			        			}
			        			if(save_song>0)
			        			{
									// Load SongDB
									sqlite3 *songsdb;
									sprintf(dbpath, "%s/.sequencegang5/songs.db", getenv("HOME"));
									if(sqlite3_open(dbpath, &songsdb) != SQLITE_OK)
									{
										cout << "Fehler beim Öffnen: " << sqlite3_errmsg(songsdb) << endl;
										return 1;
									}
									cout << "Songsdatenbank erfolgreich geöffnet!" << endl;

									sprintf(sql, "UPDATE settings SET name = \"%s\" WHERE id = %d",songname.c_str(),save_song);
									if( sqlite3_exec(songsdb,sql,NULL,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(songsdb) << endl;
										return 1;
									}
									sprintf(sql, "UPDATE settings SET bpm = \"%d\" WHERE id = %d",bpm+60,save_song);
									if( sqlite3_exec(songsdb,sql,NULL,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim UPDATE: " << sqlite3_errmsg(songsdb) << endl;
										return 1;
									}

									sprintf(sql, "DELETE FROM Song%d",save_song-1);
									if( sqlite3_exec(songsdb,sql,NULL,0,0) != SQLITE_OK)
									{
										cout << "Fehler beim DELETE: " << sqlite3_errmsg(songsdb) << endl;
										return 1;
									}

									cout << "Schreibe in DB Song" << save_song-1 << endl;

									for(int i=0;i<5;i++)
									{
										for(int j=0;j<16;j++)
										{
											for(int k=0;k<16;k++)
											{
												for(int l=0;l<5;l++)
												{
													if(pattern[i][j][k][l][0]>0)
													{

														sprintf(sql, "INSERT INTO Song%d (device,pattern,step,pos,command,data1,data2) VALUES (%d,%d,%d,%d,%d,%d,%d);",save_song-1,i,j,k,l,pattern[i][j][k][l][0],pattern[i][j][k][l][1],pattern[i][j][k][l][2]);
														if( sqlite3_exec(songsdb,sql,NULL,0,0) != SQLITE_OK)
														{
															cout << "Fehler beim INSERT: " << sqlite3_errmsg(songsdb) << endl;
															return 1;
														}
													}
												}
											}
										}
									}
									sqlite3_close(songsdb);
			        			}

								mode=0;
							}
			        		else if(CheckMouse(mousex, mousey, cancel.button_rect)==true)
							{
								mode=0;
							}
							for(int i=0;i<16;i++)
							{
								load_song[i].aktiv=false;
								if(CheckMouse(mousex, mousey, load_song[i].button_rect)==true)
								{
									load_song[i].aktiv=true;
								}
							}

			        	}
			        	else if(mode==7) // Change Songname
			        	{
			        		for(int i=0;i<40;i++)
			        		{
			        			if(CheckMouse(mousex, mousey, keyboard[i].button_rect)==true)
			        			{
			        				if(key_shift.aktiv==false)
			        				{
										keyboard[i].aktiv=true;
										if(songnametmp==" ")
										{
											songnametmp=keyboard[i].button_text;
										}
										else if(songnametmp.size()<22)
										{
											songnametmp = songnametmp + keyboard[i].button_text;
										}
			        				}
			        			}
			        			if(CheckMouse(mousex, mousey, shkeyboard[i].button_rect)==true)
			        			{
			        				if(key_shift.aktiv==true)
			        				{
										shkeyboard[i].aktiv=true;
										keyboard[i].aktiv=true;
										if(songnametmp==" ")
										{
											songnametmp=shkeyboard[i].button_text;
										}
										else if(songnametmp.size()<22)
										{
											songnametmp = songnametmp + shkeyboard[i].button_text;
										}
			        				}
			        				key_shift.aktiv=false;
			        			}
			        		}
			        		if(CheckMouse(mousex, mousey, key_shift.button_rect)==true)
							{
		        				if(key_shift.aktiv==true)
		        				{
		        					key_shift.aktiv=false;
		        				}
		        				else
		        				{
		        					key_shift.aktiv=true;
		        				}
							}
			        		if(CheckMouse(mousex, mousey, key_space.button_rect)==true)
							{
			        			key_space.aktiv=true;
								songnametmp = songnametmp + " ";
							}
			        		if(CheckMouse(mousex, mousey, key_backspace.button_rect)==true)
							{
			        			key_backspace.aktiv=true;
								if(songnametmp.size()>1)
								{
									songnametmp.erase(songnametmp.end()-1);
								}
								else
								{
									songnametmp=" ";
								}
							}
			        		if(CheckMouse(mousex, mousey, ok.button_rect)==true)
							{
			        			songname=songnametmp;
								mode=0;
							}
			        		else if(CheckMouse(mousex, mousey, cancel.button_rect)==true)
							{
								mode=0;
							}
			        	}
			        }
					anzeige=true;
					break;
				case SDL_MOUSEMOTION:
					mousex = event.button.x;
					mousey = event.button.y;
					anzeige=true;
					break;
				case SDL_MOUSEBUTTONUP:
			        if( event.button.button == SDL_BUTTON_LEFT )
			        {
			        	if(mode==7)
			        	{
							for(int i=0;i<40;i++)
							{
								keyboard[i].aktiv=false;
								shkeyboard[i].aktiv=false;
							}
		        			key_space.aktiv=false;
		        			key_backspace.aktiv=false;
			        	}
			        	stop.aktiv = false;
			        	bpmff.aktiv = false;
			        	bpmfb.aktiv = false;
			        	bpm10ff.aktiv = false;
			        	bpm10fb.aktiv = false;
			        	bpmcorff.aktiv = false;
			        	bpmcorfb.aktiv = false;
			        	bpmcor10ff.aktiv = false;
			        	bpmcor10fb.aktiv = false;
						settings_up.aktiv=false;
						settings_down.aktiv=false;
			        	noteup.aktiv = false;
			        	notedown.aktiv = false;
			        	oktaveup.aktiv = false;
			        	oktavedown.aktiv = false;
			        	volumeup.aktiv = false;
			        	volumedown.aktiv = false;
			        	volume10up.aktiv = false;
			        	volume10down.aktiv = false;
			        	programup.aktiv = false;
			        	programdown.aktiv = false;
			        	program10up.aktiv = false;
			        	program10down.aktiv = false;
			        	for(int i=0;i<5;i++)
			        	{
			        		pattern_up[i].aktiv = false;
			        		pattern_down[i].aktiv = false;
			        	}
			        }
					anzeige=true;
					break;
			}
		}
	}
	SDL_Quit();
}
