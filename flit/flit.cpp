// flit.cpp -- Flit is a "tray" with clock, sound, and battery applets

// Version 0.9.4 Fixed scaling factors used in effective charging/discharging rate

// Version 0.9.3 Added effective charging/discharging rate calculation in cases 
//               when ACPI does not provide one.

// Version 0.9.2 added support for keyboard shortcuts to open right-click menu
//               and to control sound using arror keys, or +/- keys, or through menu.
//				 Also, the Pause key will mute/unmute sound.

// Version 0.9.1 added support for all (hopefully) likely OSS volume parameter types
//               and a new .flit.conf param: oss_control_name to favor a specific
//               OSS control parameter, (e.g. pcm, vol, or vmix0-outvol)
//				 Note: vmix controls will not work unless the vmix device is fully 
//				 "attached" to your hardware.

// Version 0.9.0 added support for MIXT_STEREODB OSS parameter type

// Version 0.8.9 rearranged some code to make transparent style work better.

// Version 0.8.8 switches to direct OSS API (ioctl() stuff) for sound management.
//  	Hopefully, it will support a wider variety of sound hardware.
//		Also, this version peeks under the actual intended location
//		of the flit window to determine the "transparent" color.


#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/types.h>
#include <dirent.h>

#include <FL/Fl.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Tooltip.H>
#include <FL/Enumerations.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Help_Dialog.H>
#include <FL/x.H>
#include <X11/Xlib.h>

#include "soundcard.h"

#ifndef MIN
#define MIN(a, b)	(((a) < (b)) ? (a):(b))
#endif

#define APP_VER "0.9.4"	// Last update 2009-09-10
#define METER_W 	28
#define SOUND_W 	28
#define CLOCK_W 	64
#define SIZE_H 		20
#define SIZE_W 		(METER_W + SOUND_W + CLOCK_W)

#define BATTERY_BASE_DIR "/proc/acpi/battery/"
#define PATH_TO_FLIT_HELP "/usr/share/doc/tc/flit_help.htm"

// This adds a bit of gloss to the icons
#define VANITY 1

// Uncomment this to get status info on the stdout stream
//#define DIAG 1

const char About_text[] = 
"Flit version %s\n"
"copyright 2009 by Michael A. Losh\n"
"\n"
"Flit is an applet 'tray', with clock, sound control, and battery monitor.\n"
"\n"
"Flit is free software: you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation, version 3.\n"
"\n"
"Flit is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"See http://www.gnu.org/licenses/ for more details.";

int Base_t_sec;
int Running = 1;
enum {	MI_ABOUT, MI_HELP,  MI_CLOCK, MI_SHOW24HR, 
		MI_SOUND, MI_SOUND_MUTE, MI_SOUND_UP, MI_SOUND_DOWN, MI_BATTERY, 
		MI_NORMAL_STYLE, MI_INVERSE_STYLE, MI_TRANSPARENT_STYLE, 
		MI_SAVE_CONFIG, MI_QUIT};

static void MenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item right_click_menu[18] = {
	{"&About",    				 FL_ALT+'a', MenuCB, (void*)MI_ABOUT},
	{"&Help",     				 FL_ALT+'h', MenuCB, (void*)MI_HELP,FL_MENU_DIVIDER},
	{"Show/hide &clock",		 FL_ALT+'c', MenuCB, (void*)MI_CLOCK},
	{"&Toggle 12/24 hr. disp.",  FL_ALT+'t', MenuCB, (void*)MI_SHOW24HR, FL_MENU_DIVIDER},
	{"Show/hide soun&d",         FL_ALT+'d', MenuCB, (void*)MI_SOUND},
	{"&Mute/unmute",             FL_ALT+'m', MenuCB, (void*)MI_SOUND_MUTE},
	{"&Up the sound",            FL_ALT+'u', MenuCB, (void*)MI_SOUND_UP},
	{"&Lower the sound",         FL_ALT+'l', MenuCB, (void*)MI_SOUND_DOWN, FL_MENU_DIVIDER},
	{"Show/hide &battery meter", FL_ALT+'b', MenuCB, (void*)MI_BATTERY, FL_MENU_DIVIDER},
	{"&Normal style",  			 FL_ALT+'n', MenuCB, (void*)MI_NORMAL_STYLE},	
	{"&Inverse style",  	     FL_ALT+'i', MenuCB, (void*)MI_INVERSE_STYLE},	
	{"Trans&parent style",  	 FL_ALT+'p', MenuCB, (void*)MI_TRANSPARENT_STYLE, FL_MENU_DIVIDER},	
	{"&Save configuration",  	 FL_ALT+'s', MenuCB, (void*)MI_SAVE_CONFIG, FL_MENU_DIVIDER},	
	{"&Quit",  					 FL_ALT+'q', MenuCB, (void*)MI_QUIT},
	{0}
};

uchar  Fg_r;	// Normal forground color, typically black
uchar  Fg_g;
uchar  Fg_b;

uchar  Bg_r;	// Normal background color, often gray
uchar  Bg_g;
uchar  Bg_b;

uchar  RW_r = 0x40;	// X11 root window background color
uchar  RW_g = 0x50;
uchar  RW_b = 0x80;

enum {BATT_STATE_UNK, BATT_STATE_CHARGED, BATT_STATE_CHARGING, BATT_STATE_DISCHARGING};
enum {NORMAL_STYLE = 0, INVERSE_STYLE = 1, TRANSPARENT_STYLE = 2};
enum {LOC_SE, LOC_SW, LOC_NW, LOC_NE, LOC_CUST};

class Flit_Batt_Meter : public Fl_Box {
	int level;
	int toggle;
	public:
	int show_plug;
		
	Flit_Batt_Meter(int x, int y, int w, int h, const char * l = 0) :
		Fl_Box(x, y, w, h, l), show_plug(1), level(0), toggle(0)
		{};
	int value(void) { return level;};
	void value(int n) {
		level = n; 
		toggle = !toggle;
		redraw(); 
		}
	void  draw() {
		fl_color(FL_FOREGROUND_COLOR);
		if (show_plug) {
			label("");
		}
		Fl_Box::draw();
		if (show_plug) {
			fl_rectf(4, 4, 10, 2);
			fl_polygon(12,3, 15,2, 15,7, 12,6 );
			fl_line(15, 3, 18, 3);
			fl_line(15, 6, 18, 6);
		}
		if (level < 13  && toggle % 2 == 0) {
			fl_color(FL_RED);	// outline in red every other update
		}
		fl_rectf(2, 10, 22, 8);
		fl_rectf(24, 13, 2, 2);
		fl_line	(24, 12, 24, 15);
		fl_color(FL_BLUE);
		fl_rectf(3, 11, 20, 6);
		fl_line	(23, 13, 23, 14);
#ifdef VANITY
		// A concession to vanity: a bit of highlight
		fl_color(0x80, 0x80, 0xFF); // light blue
		fl_line(4, 12, 22, 12);
#endif		
		if (level > 40) {
			fl_color(FL_GREEN);
		}
		else if (level > 20) {
			fl_color(FL_YELLOW);
		}
		else {
			fl_color(FL_RED);
		}
		int charge_w = (level + 2) / 5; 	// + 2 does a little rounding
		fl_rectf(3, 11, charge_w, 6);

#ifdef VANITY
		if (level > 40) {
			fl_color(0x80, 0xFF, 0x80); // light green
		}
		else if (level > 20) {
			fl_color(0xFF, 0xFF, 0x80); // light yellow
		}
		else {
			fl_color(0xFF, 0x80, 0x80); // light red

		}
		fl_line(3, 12, 2 + charge_w, 12);
#endif
	}

};

class Flit_Sound_Control : public Fl_Box {
	int level;
	int toggle;
	char tip_txt[16];
	public:
	int show_plug;
		
	Flit_Sound_Control(int x, int y, int w, int h, const char * l = 0) :
		Fl_Box(x, y, w, h, l), level(91)
		{};
	int value(void) { return level;};
	void value(int n) { 
		level = n; 
		redraw(); 
		sprintf(tip_txt, "Vol. %d", value());
		tooltip(tip_txt);
#ifdef DIAG
		printf("Sound ctrl. new value is %d\n", level);
#endif
	};
	void  draw() {		
		Fl_Box::draw();
		
		fl_color(0xA0, 0xA0, 0xB0); // Medium blue-gray
		fl_rectf(x() + 5, 7, 3, 6);	// coil color
		fl_polygon(x()+8,6, x()+12,2, x()+12,18, x()+8,14 );	// horn color
		
		fl_color(FL_FOREGROUND_COLOR);
		fl_rect(x() + 4, 6, 4, 8);	// coil outline
		fl_loop(x()+8,6, x()+12,2, x()+12,18, x()+8,14 ); 		// horn outline
		fl_loop(x()+13,7, x()+14,9, x()+14,11, x()+13,13 ); 	// dome outline
		
		fl_color(0x40, 0x40, 0x40); // dark gray
		fl_loop(x()+13,7, x()+14,9, x()+14,11, x()+13,13 ); 	// dome outline
		
		if (level < 1 ) {
			fl_color(FL_RED);	
			fl_arc(x()+4, 2, 16, 16, 0.0, 359.9);
			fl_arc(x()+5, 3, 14, 14, 0.0, 359.9);
			fl_line(x()+7, 5, x()+16, 14);
			fl_line(x()+8, 5, x()+17, 14);
		}
#ifdef VANITY
		// A concession to vanity: a bit of highlight
		fl_color(0xE0, 0xE0, 0xE0); // light gray
		fl_line(x()+5, 9, x()+7, 9);
		fl_line(x()+9, 7, x()+11, 5); 
#endif		
		fl_color(Bg_r + Fg_r / 2, Bg_g + Fg_g / 2, Bg_b + Fg_b / 2);
		if (level > 0) {
			fl_color(FL_FOREGROUND_COLOR);
			fl_line(x() + 16, 15, x() + 18, 17);
		}
		if (level >= 10) {
			fl_line(x() + 18, 17, x() + 19, 19);
		}
		if (level >= 20) {
			fl_line(x() + 17, 13, x() + 19, 14);
		}
		if (level >= 30) {
			fl_line(x() + 19, 14, x() + 21, 15);
		}		
		if (level >= 40) {
			fl_line(x() + 17, 11, x() + 19, 11);
		}
		if (level >= 50) {
			fl_line(x() + 19, 11, x() + 22, 11);
		}
		if (level >= 60) {
			fl_line(x() + 17, 9, x() + 19, 8);
		}
		if (level >= 70) {
			fl_line(x() + 19, 8, x() + 21, 7);
		}
		if (level >= 80) {
			fl_line(x() + 16, 6, x() + 18, 4);
		}
		if (level >= 90) {
			fl_line(x() + 18, 4, x() + 20, 2);
		}

	}
};

class Flit_Frame : public Fl_Window {
	protected:
		int		size_w;
		int		size_h;
		int		win_loc;
		int		style;
		int		win_x;
		int		win_y;
		char 	configfilename[256];

		int		show_sound;
		Flit_Sound_Control* sound_p;
		int		prev_sound_vol;
		int		mixer_fd;
		int		mix_ctrl;
		int		mix_min;
		int		mix_max;
		char    oss_control_name[64];
	
		int		show_clock;
		Fl_Box* clock_p;
		int     show24hr;
	    char 	timestr[16];
		char 	ampm[4];
		time_t	last_update_time;
		
		
		int		show_battery;
		Flit_Batt_Meter* meter_p;
		int 	samples;
		float	capacity_mAh;
		float	charge_mAh;
		float	orig_charge_mAh;
		float	avgcur_mA;
		float   charge_pct;
		float   remaining_hrs;
		int		batt_state;
		char    meter_str[8];
        char    meter_tip[64];
        char    time_tip[64];
        char    batt_state_filename[128];

	public:
		void set_color(Fl_Color c) {
			if (meter_p) 	meter_p->labelcolor(c);
			if (sound_p) 	sound_p->labelcolor(c);
			if (clock_p)	clock_p->labelcolor(c);
		}
		
		void set_boxtype(Fl_Boxtype boxtype) {
			if (meter_p)	meter_p->box(boxtype);
			if (sound_p)	sound_p->box(boxtype);
			if (clock_p)	clock_p->box(boxtype);
			redraw();
		}
		
		Flit_Frame(const char *title = 0) :
		  Fl_Window(10,  10,  100,  20, title),
		  show24hr(0),
		  capacity_mAh(999999.0),
		  charge_mAh(999999.0),
		  orig_charge_mAh(99999.0),
		  avgcur_mA(1),
		  charge_pct(100.0),
		  remaining_hrs(9999.0),
		  batt_state(BATT_STATE_UNK),
		  samples(0), 
		  show_clock(1),
		  show_sound(1),
		  show_battery(1),
		  meter_p(NULL),
		  sound_p(NULL),
		  clock_p(NULL),
		  size_w(0),
		  size_h(SIZE_H),
		  win_loc(LOC_SE),
		  style(NORMAL_STYLE),
		  win_x(10),
		  win_y(10),
		  prev_sound_vol(-1),
		  mixer_fd(0),
		  mix_min(0),
		  mix_max(100),
		  mix_ctrl(-1)
		{
			Base_t_sec = time(NULL);
			strcpy(oss_control_name, "autosel");
		};
		
		void get_time(void) {
		  time_t now 		= time(NULL);
		  struct tm* timep 	= localtime(&now);
		  if (show24hr) {
			  strcpy(ampm, "");
		  }
		  else {
			  if (timep->tm_hour>=12) {
				  strcpy(ampm, " PM");
			  }
			  else {
				  strcpy(ampm, " AM");
			  }
		  }
		  int hour = timep->tm_hour;
		  if (!show24hr) {
			  hour = hour % 12;
			  if (hour < 1) hour = 12;
		  }
		  
		  sprintf(timestr, "%d:%02d%s",	hour, timep->tm_min, ampm);
		  clock_p->label(timestr);
          strcpy(time_tip, ctime(&now));
          clock_p->tooltip(time_tip);
		  clock_p->redraw();
		};
		
		void get_sound_info(void) {
			int c;
			int success = 0;
			int autosel = 0;	// Whether Flit should pick a control
			oss_mixext info;
			mix_ctrl = 0;
			
			if(!mixer_fd) {
				mixer_fd = open("/dev/mixer", O_RDWR);
				if (-1 != mixer_fd) {
					oss_mixerinfo mi;
					mi.dev = -1;
					if (-1 != ioctl(mixer_fd, SNDCTL_MIXERINFO, &mi)) {
#ifdef DIAG						
						printf("Mixer device is '%s', priority %d\n", mi.name, mi.priority);	
#endif				
					}
					else {
						perror("SNDCTL_MIXERINFO ioctl()");
					}
					
					int mix_ctls = 0; 
					if (-1 != ioctl(mixer_fd, SNDCTL_MIX_NREXT, &mix_ctls)) {
#ifdef DIAG						
						printf("There are %d controls in the mixer\n", mix_ctls);
#endif				
					}		
					autosel = 0;
					if (!strcmp(oss_control_name, "autosel") ) {
						autosel = 1;
					}
					for (c = 0 ; c < mix_ctls ; c++ ) {
						info.dev = 0;
						info.ctrl = c;
						if (-1 != ioctl(mixer_fd, SNDCTL_MIX_EXTINFO, &info)) {
#ifdef DIAG						
							printf("--->\t%d\tConsidering volume control item: '%s', type %d, flags 0x%X\n", 
								c, info.extname, info.type, info.flags);
#endif				
							if (	!success 
								&&  (	!strcmp(info.extname, oss_control_name)  // found the specified OSS control?
									||  (	autosel == 1
										&&	(	(info.flags & MIXF_MAINVOL) 			// OSS thinks it can control volume
											||  (info.flags & MIXF_PCMVOL) 				// ''
											)
										)
									)
								) 
							{
								success = 1;
								mix_ctrl = c;	
								mix_max = info.maxvalue;
								mix_min = info.minvalue;
#ifdef DIAG						
								printf("===>\t%d\tSelected volume control item: '%s'\n", c, info.extname);
								printf("===>\t%d\tmin %d, max %d,\n", c, mix_min, mix_min);
#else								
								break; // quit after the first one that seems to match
#endif				
							}
#ifdef DIAG						
							else { 
								printf("\t\t\t(not selected)\n");
							}
#endif				
						}
						else 
							perror("SNDCTL_MIX_EXTINFO ioctl()");
						
					} // end for
					
				}
#ifdef DIAG
				else {
					perror("/dev/mixer open()");
					printf("/n");
				}
#endif
				
			}
				
			if (!success) {
				show_sound = 0;
				arrange();
			}
		}

		void get_sound_level(void) {
			oss_mixext info;
			oss_mixer_value vr;
			int pct;
			int rawval;
			if (mixer_fd && mix_ctrl) {
				info.dev = 0;
				info.ctrl = mix_ctrl;				
				if (-1 != ioctl(mixer_fd, SNDCTL_MIX_EXTINFO, &info)) {
					vr.dev=info.dev;
					vr.ctrl=info.ctrl;
					vr.timestamp=info.timestamp;
					if (-1 != ioctl(mixer_fd, SNDCTL_MIX_READ, &vr)) {
						switch(info.type) {
							case MIXT_MONOSLIDER:
							case MIXT_STEREOSLIDER:
								rawval = vr.value & 0xFF;
								break;
							case MIXT_MONOSLIDER16:
							case MIXT_STEREOSLIDER16:
								rawval = vr.value & 0x7FFF;
								break;								
							case MIXT_MUTE:
								rawval = (vr.value) ? mix_max : mix_min;
								break;
							case MIXT_SLIDER:	// 32-bit
							default:			// Maybe MIXT_VALUE, MIXT_HEXVALUE, MIXT_MONODB, MIXT_STEREODB
								rawval = vr.value;
								break;
						}
								
						pct = (int)(100.0 * (float)(rawval - mix_min) / (float)mix_max);  						
#ifdef DIAG						
						printf("Read 0x%08X from control, raw volume is %d or %d%%\n", 
							vr.value, rawval, pct);
#endif				
						sound_p->value(pct);
					}
					else 
						perror("SNDCTL_MIX_READ ioctl()");
				}
				else 
					perror("SNDCTL_MIX_EXTINFO ioctl()");
			}
#ifdef DIAG						
			else 
				printf("No sound device or suitable control\n");
#endif				
		};
		
		void set_sound_level(int vol) {
			oss_mixext info;
			oss_mixer_value vr;
			int newvol;
			int intended;
			if (mixer_fd && mix_ctrl) {
				info.dev = 0;
				info.ctrl = mix_ctrl;				
				if (-1 != ioctl(mixer_fd, SNDCTL_MIX_EXTINFO, &info)) {
					vr.dev=info.dev;
					vr.ctrl=info.ctrl;
					newvol = (int)((float)(vol * mix_max) / 100.0) - mix_min;
					vr.timestamp=info.timestamp;
					switch(info.type) {
						case MIXT_MONOSLIDER:
						case MIXT_STEREOSLIDER:
							vr.value = (newvol << 8) | newvol;
							break;
						case MIXT_MONOSLIDER16:
						case MIXT_STEREOSLIDER16:
							vr.value = (newvol << 16) | newvol;
							break;								
						case MIXT_MUTE:
							vr.value = (newvol) ? mix_max : mix_min;
							break;
						case MIXT_SLIDER:	// 32-bit
						default:			// Maybe MIXT_VALUE, MIXT_HEXVALUE, MIXT_MONODB, MIXT_STEREODB
							vr.value = newvol;
							break;
					}
					intended = vr.value;		
					if (-1 != ioctl(mixer_fd, SNDCTL_MIX_WRITE, &vr)) {
#ifdef DIAG						
						printf("Written value is 0x%08X, intended value 0x%08X (for %d%%)\n", 
								vr.value, intended, vol);
#endif				
						if (vr.value == intended) {
							sound_p->value(vol);
						}
						else {
							// find out what the value is, actually
							get_sound_level();
						}
					}
					else 
						perror("SNDCTL_MIX_WRITE ioctl()");
				}
				else 
					perror("SNDCTL_MIX_EXTINFO ioctl()");
			}
#ifdef DIAG						
			else 
				printf("No sound device or suitable control\n");
#endif				
		};
		
		void get_battery_info(void) {
            char dirname[32] = "";
			char buf[80] = "";
			DIR* bd = NULL;   // batt. directory file ptr
			FILE* bif = NULL;   // batt. info file ptr
            struct dirent* dp;
            bd = opendir(BATTERY_BASE_DIR);
            while (bd && (dp = readdir(bd))) {
                if (*(dp->d_name) == '.') continue;
                if (strcmp(dp->d_name, dirname) < 0 || strlen(dirname) == 0) {
                    strcpy(dirname, dp->d_name); // keep first alphabetically if more than one
                }
            }
            closedir(bd);
            if (!strlen(dirname)) {
                meter_p->label("\nAC");
                return;
            }
            sprintf(batt_state_filename, "%s%s/state", BATTERY_BASE_DIR, dirname);  // save for later
            sprintf(buf, "%s%s/info", BATTERY_BASE_DIR, dirname); // format batt. info filename
			bif = fopen(buf, "r");
			float cap_mAh = 0.0;
			if (bif) {
				while(fgets(buf, 79, bif)) {
					if (1 == sscanf(buf, "last full capacity: %f mAh\n", &cap_mAh)) {
						capacity_mAh = cap_mAh;
#ifdef DIAG 
						printf("Battery capacity is %1.1f mAh\n", capacity_mAh); 
#endif
						break;
					}
				}
				fclose(bif);
			}
			else {
				meter_p->label("\nAC");
				meter_p->show_plug = 1;
			}
		};

		void get_battery_state(void) {
			int need_state = 1;
			int need_charge = 1;
			int need_rate = 1;
			char buf[80];
			char status[20];
			float chg_mAh = 0.0;
			float rate_mA = 0.0;
			float mA = 1;
			float ratio = 0.0;
			FILE* bsf = NULL;
			bsf = fopen(batt_state_filename, "r");
			if (bsf) {
				while(fgets(buf, 79, bsf)) {					
					if (need_state && 1 == sscanf(buf, "charging state: %s\n", status)) {
						need_state = 0;
						if (!strcmp(status, "discharging")) {
							if (batt_state != BATT_STATE_DISCHARGING) samples = 0;
							batt_state = BATT_STATE_DISCHARGING;
						}
						else if (!strcmp(status, "charging")) {
							if (batt_state != BATT_STATE_CHARGING) samples = 0;
							batt_state = BATT_STATE_CHARGING;
						}
						else if (!strcmp(status, "charged")) {
							batt_state = BATT_STATE_CHARGED;
						}
						else {
							batt_state = BATT_STATE_UNK;
						}
					}
					else if (need_rate && 1 == sscanf(buf, "present rate: %f mA\n", &rate_mA)) {
						need_rate = 0;
						if (rate_mA > 32767.0) {
							rate_mA = 65536.- rate_mA;	// handle negative for charging
						}
					}
					else if (need_charge && 1 == sscanf(buf, "remaining capacity: %f mAh\n", &chg_mAh)) {
						need_charge = 0;
						charge_mAh = chg_mAh;
						charge_pct = 100.0 * charge_mAh / capacity_mAh;
						if (samples == 0) orig_charge_mAh = charge_mAh;
						break;
					}
					
				}
				fclose(bsf);
                if (!need_rate && !need_charge && !need_state) {
					samples++;
#ifdef DIAG
					printf("Sample %d at time %d sec after start\n", samples,
							time(NULL) - Base_t_sec);
#endif
					if (samples == 1) {
#ifdef DIAG
						printf("Setting avg. current to just this sample\n"); 
#endif
						avgcur_mA = rate_mA;
					}
					else if (samples < 30) {
						// For about the first 3 mins, favor new data
#ifdef DIAG
						printf("Favoring new data 2/3 to avg 1/3\n"); 
#endif
						avgcur_mA = ((2.0*rate_mA) + avgcur_mA ) / 3.0;  
					}
					else { 
						// Weighted average of past (15/16) and now (1/16), acts as a low-pass filter
#ifdef DIAG
						printf("Favoring old data 15/16 to new 1/16\n"); 
#endif
						avgcur_mA = ((15.0 * avgcur_mA) + rate_mA) / 16.0;
					}
					meter_p->show_plug = 1; // assume we need to show it
                    if (batt_state == BATT_STATE_CHARGING)
                    {
						// Calculate charging rate since ACPI did not provide
						if (rate_mA < 1.0) {
							rate_mA = (charge_mAh - orig_charge_mAh) * 720.0 / (float)samples;
							if (rate_mA < 1) {
								rate_mA = 1;
							}
						}
						remaining_hrs = (capacity_mAh - charge_mAh) / rate_mA;
						// Charging time is not very accurate because charge
						// rate slows down as the battery fills, so to 
						// provide a rough approximation, we use a couple 
						// of correction factors
						// TODO: Replace with a self-calibrating function?
						if (charge_pct< 0.90) {
							remaining_hrs *= 2.6;
						}
						else {
							remaining_hrs *= 1.6;
						}
                    } 
                    if (batt_state == BATT_STATE_DISCHARGING)
                    {
						if (fabs(avgcur_mA) > 1.0) {
							remaining_hrs = charge_mAh / avgcur_mA;
						}
						else {
							// Calculate discharging rate since ACPI did not provide
							rate_mA = (orig_charge_mAh - charge_mAh) * 720.0 / (float)samples;
							if (rate_mA < 1) {
								rate_mA = 1;
							}
							remaining_hrs = charge_mAh / rate_mA;
						}
						meter_p->show_plug = 0; // don't show if we are discharging!
                    } 
                    int hours = (int)floor(remaining_hrs);
                    int mins  = (int)((remaining_hrs - hours)*60.0);
                    sprintf(meter_str, "%1.f%s\n---",charge_pct, (batt_state == BATT_STATE_CHARGING)?"^":"");
                    if (batt_state == BATT_STATE_CHARGED) {
                        strcpy(meter_tip, "Fully charged");
                    }
                    else {
                        sprintf(meter_tip, "%1.f%%, about %dhr%s, %dmin%s %s", 
                                charge_pct, hours, (hours==1) ? "":"s", 
                                mins,  (mins==1) ? "":"s", 
                                (batt_state == BATT_STATE_CHARGING) ? "to charge":"remaining"
                               );
                    }
                    meter_p->label(meter_str);
                    meter_p->tooltip(meter_tip);
					meter_p->value((int)floor(charge_pct));

#ifdef DIAG 
					printf("Charge/discharge state is %s (flit code %d)\n", status, batt_state);
					printf("Current is %1.f mA, average current is %1.1f mA\n", rate_mA, avgcur_mA);
					printf("Charge level was %1.f mAh at sample zero\n", orig_charge_mAh);
					printf("Charge level is %1.f mAh or %1.1f%% of %1.f possible\n", charge_mAh, charge_pct, capacity_mAh);
					printf("Remaining hours: %1.2f\n", remaining_hrs); 
					printf("---------------------------------------------------\n"); 
#endif
                }
			}
			else {
				meter_p->label("??");
			}
            meter_p->redraw();
		};
		
		void toggle_show24hr(void) {
		  show24hr = !show24hr;
		  get_time();
		  redraw();
		};
		
		void do_menu(void) {
			const Fl_Menu_Item *m = right_click_menu->popup(Fl::event_x(), Fl::event_y(), "Flit", 0, 0);
			if ( m ) m->do_callback(this, m->user_data());
		};
		
		// -1 for quieter, +1 for louder, 0 for toggle
		void do_sound_adj(int direction) {
			int vol;
			vol = sound_p->value();
			if (direction < 0) {
				prev_sound_vol = vol;
				vol -= 5;
				if (vol < 0) vol = 0;
			}
			else if (direction > 0) {
				prev_sound_vol = vol;
				vol += 5;
				if (vol > 100) vol = 100;
			}
			else {
				// direction is 0, so toggle
				if (sound_p->value() > 0) {
					prev_sound_vol = vol;
					vol = 0;
				}
				else {
					vol = prev_sound_vol;
				}
			}
			set_sound_level(vol);
			sound_p->redraw();
			Fl_Tooltip::delay(0);		// force tooltip on
			Fl_Tooltip::exit(sound_p);
			Fl_Tooltip::enter(sound_p);
			Fl_Tooltip::delay(1);
		}
		
		int handle(int e) {
			static int off_x = 0;
			static int off_y = 0;
			int ret, key;
			switch ( e ) {
			case FL_KEYDOWN:
				key = Fl::event_key();
				if (key == FL_Escape) {
					do_menu();
					return 1;
				}
				break;
			case FL_KEYUP:
				key = Fl::event_key();
				//printf("Keyup for 0x%02X\n", key);
				if (show_sound &&  (key == FL_Up || key == '=')) {	// '=' is also + key				
					do_sound_adj(+1);
					return 1;
				}
				else if (show_sound && (key == FL_Down || key == '-')) {
					do_sound_adj(-1);
					return 1;
				}
				else if (show_sound && key == FL_Pause) {
					do_sound_adj(0);
					return 1;
				}
				break;
			case FL_PUSH:
				if (Fl::event_button() == FL_LEFT_MOUSE) {
					if (show_sound && Fl::event_clicks() 
						&& Fl::event_x() > sound_p->x() 
						&&  Fl::event_x() < (sound_p->x() + SOUND_W)) 
					{
						do_sound_adj(0); // zero means toggle	
						return 1;
					}
					else {
						off_x = Fl::event_x();
						off_y = Fl::event_y();
					}
				}
				else if (Fl::event_button() == FL_RIGHT_MOUSE ) {
					do_menu();
					return(1);          // (tells caller we handled this event)
				}	
				break;
			case FL_DRAG:
				if ( Fl::event_button() == FL_LEFT_MOUSE ) {
					win_x = Fl::event_x_root() - off_x;
					win_y = Fl::event_y_root() - off_y;
					position(win_x, win_y);
					win_loc = LOC_CUST;
					redraw();
					return 1;
				}  
			case FL_MOUSEWHEEL:
				if (show_sound && Fl::event_x() > sound_p->x() 
					&&  Fl::event_x() < (sound_p->x() + SOUND_W)) {
					do_sound_adj(-1 * Fl::event_dy());
					return 1;
				}
			default:
				ret = Fl_Window::handle(e);
				break;
			}
			return(ret);
		};
		void update(void) {
			time_t now 		= time(NULL);
			if (now - last_update_time >= 5) {
				if (show_clock) 	get_time();
				if (show_sound)		get_sound_level();
				if (show_battery)	get_battery_state();
				last_update_time = now;
			}
			if (show_sound && prev_sound_vol < 0) {
				prev_sound_vol = sound_p->value();
			}
			redraw();
		}
		void get_config() {
			char confline[128];
			char s[64];
			int n;
			int xpos, ypos;
			char* env_home_p = getenv("HOME");
			sprintf(configfilename, "%s/.flit.conf", env_home_p);
			FILE* cfgf = NULL;
			
			cfgf = fopen(configfilename, "r");
			if (cfgf) {
				while (fgets(confline, 127, cfgf)) {
					if (1 == sscanf(confline, "show_clock = %d\n", &n)) {
#ifdef DIAG		
						printf("show_clock is %d\n", n);
#endif
						show_clock = n;
					}
					else if (1 == sscanf(confline, "show_sound = %d\n", &n)) {
#ifdef DIAG		
						printf("show_sound is %d\n", n);
#endif
						show_sound = n;
					}
					else if (1 == sscanf(confline, "show_battery = %d\n", &n)) {
#ifdef DIAG		
						printf("show_battery is %d\n", n);
#endif
						show_battery = n;
					}
					else if (1 == sscanf(confline, "style = %d\n", &n)) {
#ifdef DIAG		
						printf("style is %d\n", n);
#endif
						style = n;
					}
					else if (1 == sscanf(confline, "oss_control_name = %s\n", s)) {
						if (strlen(s) > 0 && strlen(s) < 63) {
							strcpy(oss_control_name, s);
						}
#ifdef DIAG		
						printf("oss_control_name is %s\n", oss_control_name);
#endif
						
					}
					else if (1 == sscanf(confline, "show24hr = %d\n", &n)) {
#ifdef DIAG		
						printf("show24hr is %d\n", n);
#endif
						show24hr = n;
					}
					else if (1 == sscanf(confline, "location = %s\n", s)) {
#ifdef DIAG		
						printf("Location spec is %s\n", s);
#endif
						if (!strcmp(s, "se")) {
							win_loc = LOC_SE;
						}
						else if (!strcmp(s, "sw")) {
							win_loc = LOC_SW;
						}
						else if (!strcmp(s, "nw")) {
							win_loc = LOC_NW;
						}
						else if (!strcmp(s, "ne")) {
							win_loc = LOC_NE;
						}
						else {
							if (2 == sscanf(s, "%d,%d", &xpos, &ypos)) {
								win_x = xpos;
								win_y = ypos;
								win_loc = LOC_CUST;
							}
						} 
					} // end location		
				} // end while more lines.
			}
			else {
#ifdef DIAG
				printf("No config file found: using defaults.\n");
#endif				
			}
		};  // end get_config()

		void arrange(void) {
			size_h = SIZE_H;
			size_w = 0;
			begin();
			if (show_battery) {
				if (!meter_p) {
					meter_p = new Flit_Batt_Meter(0, 0, METER_W, SIZE_H);
					get_battery_info();
					meter_p->labelsize(8);
				}
				meter_p->position(size_w, 0);
				size_w += METER_W;
			}

			if (show_sound) {
				if (!sound_p) {
					sound_p = new Flit_Sound_Control(0, 0, SOUND_W, SIZE_H);
					sound_p->labelsize(8);
				}
				sound_p->position(size_w, 0);
				size_w += SOUND_W;
			}

			if (show_clock) {	
				if (!clock_p) {
					clock_p = new Fl_Box(0, 0, CLOCK_W, SIZE_H);
					get_time();
					clock_p->labelsize(12);
				}
				clock_p->position(size_w, 0);
				size_w += CLOCK_W;
			}
			end();
			
			switch(win_loc) {
				case LOC_SW:
					win_x = 0;
					win_y = Fl::h() - size_h;
					break;
				case LOC_NW:
					win_x = 0;
					win_y = 0;
					break;
				case LOC_NE:
					win_x = Fl::w() - size_w; 
					win_y = 0;
					break;
				case LOC_SE:
					win_x = Fl::w() - size_w; 
					win_y = Fl::h() - size_h;
					break;
			}
			resize(win_x, win_y, size_w, size_h);
			
			redraw();
		};
		
		void toggle_show_clock(void) {
			if (show_clock && (show_battery + show_sound)) // can't disable clock unless one other is showing
			{
				show_clock = 0;
			}
			else if (!show_clock) {	// can always re-enable clock
				show_clock = 1;
			}
			arrange();
			set_style(style);
		};
		
		void toggle_show_sound(void) {
			if (show_sound && (show_clock + show_battery)) // can't disable sound ctrl unless one other is showing
			{
				show_sound = 0;
			}
			else if (!show_sound) {	// can always re-enable sound
				show_sound = 1;
			}
			arrange();
			set_style(style);
		};
		
		void toggle_show_batt(void) {
			if (show_battery && (show_sound + show_clock)) // can't disable battery meter unless one other is showing
			{
				show_battery = 0;
			}
			else if (!show_battery) {	// can always re-enable battery
				show_battery = 1;
			}
			arrange();
			set_style(style);
		};
		
		void set_style(int newstyle) {
			int bg_r, bg_g, bg_b, fg_r, fg_g, fg_b;
			style = newstyle;
			switch(style) {
				case NORMAL_STYLE:
					Fl::set_color(FL_BACKGROUND_COLOR, Bg_r, Bg_g, Bg_b);
					Fl::set_color(FL_FOREGROUND_COLOR, Fg_r, Fg_g, Fg_b);
					set_boxtype(FL_THIN_DOWN_BOX);
					break;
				case INVERSE_STYLE:
					Fl::set_color(FL_BACKGROUND_COLOR, Fg_r, Fg_g, Fg_b);
					Fl::set_color(FL_FOREGROUND_COLOR, Bg_r, Bg_g, Bg_b);
					set_boxtype(FL_THIN_DOWN_BOX);
					break;
				case TRANSPARENT_STYLE:
					bg_r = MIN(0xFF, RW_r + 20);
					bg_g = MIN(0xFF, RW_g + 20);
					bg_b = MIN(0xFF, RW_b + 20);
					fg_r = Fg_r;
					fg_g = Fg_g;
					fg_b = Fg_b;
					if (bg_r + bg_g + bg_b < 0xA0) {
						// we have a fairly dark background, so foreground color
						// needs to be fairly light
						fg_r = fg_g = fg_b = 0xA0;
					}
					Fl::set_color(FL_BACKGROUND_COLOR, bg_r, bg_g, bg_b);
					Fl::set_color(FL_FOREGROUND_COLOR, fg_r, fg_g, fg_b);
					set_boxtype(FL_FLAT_BOX);
					break;
			}
			redraw();
		};

		void get_background_color(void) {
			Window rootw;
			XImage * xip = NULL;

			rootw = DefaultRootWindow(fl_display);
			// Take a peek at the X11 root window, so we can find out it's color
			xip = XGetImage(fl_display, rootw, win_x, win_y, 1, 1, -1, ZPixmap); // 1x1 pixel screen grab
			if (xip) {
				RW_r = (uchar)xip->data[2];	// This may only work with true-color color modes
				RW_g = (uchar)xip->data[1];
				RW_b = (uchar)xip->data[0];
#ifdef DIAG		
				printf("Root RGB colors are %02X:%02X:%02x\n", RW_r, RW_g, RW_b);
#endif
			}

		}
		
		void configure(void) {
			get_config();
			clear_border();
			box(FL_NO_BOX);
			if (show_sound) get_sound_info();
			arrange();
			get_background_color();
			Fl::get_color(FL_FOREGROUND_COLOR, Fg_r, Fg_g, Fg_b);
			Fl::get_color(FL_BACKGROUND_COLOR, Bg_r, Bg_g, Bg_b);
			set_style(style);
			
		};
		
		void save_config(void) {
			const char *loc_tag[] = {"se", "sw", "nw", "ne"};
			FILE* cfgf = NULL;
			cfgf = fopen(configfilename, "w");
			if (cfgf) {
				fprintf(cfgf, "# Configuration file for Flit \n\n");
				fprintf(cfgf, "# Note: this is a machine-generated file; if you change any\n");
				fprintf(cfgf, "#       content, preserve spelling, capitalization, and spacing!\n\n");
				fprintf(cfgf, "# Enable (1) or disable (0) individual applets\n");
				fprintf(cfgf, "show_clock = %d\n", show_clock);
				fprintf(cfgf, "show_sound = %d\n", show_sound);
				fprintf(cfgf, "show_battery = %d\n", show_battery);
				fprintf(cfgf, "\n");
				fprintf(cfgf, "# Show clock in 24 hour format (1 means yes, 0 means no - use AM/PM)\n");
				fprintf(cfgf, "show24hr = %d\n", show24hr);
				fprintf(cfgf, "\n");
				fprintf(cfgf, "# OSS sound control name to use, autosel means 'try to pick for me'\n");
				fprintf(cfgf, "oss_control_name = %s\n", oss_control_name);
				fprintf(cfgf, "\n");
				fprintf(cfgf, "# Color style: 0 is normal, 1 is inverted, 2 is 'transparent' (based on root window color)\n");
				fprintf(cfgf, "style = %d\n", style);
				fprintf(cfgf, "\n");
				fprintf(cfgf, "# Location: se (Southeast, i.e. lower right), sw, nw, ne, or a custom X,Y like this example:\n");
				fprintf(cfgf, "#location = %d,%d\n", win_x, win_y);
				if (win_loc == LOC_CUST) {
					fprintf(cfgf, "location = %d,%d\n", win_x, win_y);
				}
				else {
					fprintf(cfgf, "location = %s\n", loc_tag[win_loc]);
				}
				fprintf(cfgf, "\n");
				
				fclose(cfgf);
			}
		};

		
};

static void MenuCB(Fl_Widget* window_p, void *userdata)	
{
	Fl_Help_Dialog hd;
	long choice = (long)userdata;
	Flit_Frame* mainwnd_p = (Flit_Frame *)window_p;
	
	switch (choice) {
		case MI_ABOUT:
			fl_message(About_text, APP_VER);
			break;
		case MI_HELP:
			hd.load(PATH_TO_FLIT_HELP);
			hd.textsize(14);
			hd.show();
			while (hd.visible()) {
				Fl::wait(1);
			} 
			break;
		case MI_CLOCK:
			mainwnd_p->toggle_show_clock();
			break;
		case MI_SHOW24HR:
			mainwnd_p->toggle_show24hr();
			break;
		case MI_BATTERY:
			mainwnd_p->toggle_show_batt();
			break;
		case MI_SOUND:
			mainwnd_p->toggle_show_sound();
			break;
		case MI_SOUND_MUTE:
			mainwnd_p->do_sound_adj(0);	// Toggle sound on/off
			break;
		case MI_SOUND_UP:
			mainwnd_p->do_sound_adj(+1);
			break;
		case MI_SOUND_DOWN:
			mainwnd_p->do_sound_adj(-1);
			break;
		case MI_NORMAL_STYLE:
			mainwnd_p->set_style(NORMAL_STYLE);
			break;
		case MI_INVERSE_STYLE:
			mainwnd_p->set_style(INVERSE_STYLE);
			break;
		case MI_TRANSPARENT_STYLE:
			mainwnd_p->set_style(TRANSPARENT_STYLE);
			break;
		case MI_SAVE_CONFIG:
			mainwnd_p->save_config();
			break;
		case MI_QUIT:
			Running = 0;
			break;
			
		default:
			break;
	}

}

int main(int argc, char** argv)
{
	int rc;
	fl_open_display();
	fl_message_font(fl_font(), 12);
	Flit_Frame MainWnd("Flit");
	MainWnd.configure();
	MainWnd.update();
	MainWnd.show();
	while(Running) {
		Fl::wait(5.0);
		MainWnd.update();
	}
	//MainWnd.save_config();
    return 0; // Fl::run();
	
}
