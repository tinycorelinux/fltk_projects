// flit.cpp -- Flit is a "tray" with clock, sound, and battery applets

// Version 1.3.2 Added: saved_volume variable
//				Updated: get_config and save_config to allow saving and restoring
//						 volume level between sessions.
//				Rewrote: ALSA and newvol Audio level calculations to round instead of truncate.
//						 Old versions are commented out with //RR, new versions in following line.
//				Updated: Lowered volume stepsize to 3% from 5% in do_sound_adj to fit better in
//						 ALSAs 5 bit (0-31) volume level range.

// Version 1.3.1 - M. Losh - Fix to make file to preserve root perms in /usr when installed   

// Version 1.3.0 - M. Losh - Released with changes to default values of custom colors
//                           and to the "About" text    

/* Version pre-1.3.0  Jakob Bysewski <jbysewski@googlemail.com> 
 *              * ALSA is now working with USB soundcards without a hardware mixer using an alsa virtual softvol device
 *              * flit prevents itself from starting multiple instances using a pidfile
 *              * flit can be made dockable
 *              * flit can be made to appear on all desktops (experimental - to be tested)
 *              * flit got command line parameters using argp
 *              * flit can be passed an optional config file to be used
 *              * support for custom fg and bg colors in config
 */
// 

// Version 1.2.2 Fix ALSA bug when selected parameter is Mono

// Version 1.2.1 Fix OSS setsound bug and diag message for OSS setup

// Version 1.2.0 Rounded up version number for public release

// Version 1.1.7 Made size scaling work with floating-point factor

// Version 1.1.6 Support size scaling, fixed hide/show bug

// Version 1.1.5 Compile-time option for OLPC hardware battery support

// Version 1.1.0 ALSA sound control support

// Version 1.0.0 Let's call this major level done!  No real code changes.

// Version 0.9.9 Handle case where battery rate is "unknown" (or some other non-numeric)
//               by allowing calc to proceed using derived rate (as introduced in ver 0.9.3/0.9.4)

// Version 0.9.8 Made the automatic pop-open of menu with Ctrl optional, 
//               Start in unfocused input state

// Version 0.9.7 Removed Esc keybinding to open menu, added '%' to batt. icon

// Version 0.9.6 Changed format of % statements in tooltips and diag. output
//               Changed Alt+(key) bindings to Ctrl+(key)

// Version 0.9.5 Prevent unhiding sound control if no sound system available,
//               Alt key automatically opens menu (cheap way to improve key-binding)
//               Surpress charge/discharge time estimates for 3 minutes to build up data

// Version 0.9.4 Fixed scaling factors used in effective charging/discharging rate

// Version 0.9.3 Added effective charging/discharging rate calculation in cases 
//               when ACPI does not provide one.

// Version 0.9.2 added support for keyboard shortcuts to open right-click menu
//               and to control sound using arror keys, or +/- keys, or through menu.
//               Also, the Pause key will mute/unmute sound.

// Version 0.9.1 added support for all (hopefully) likely OSS volume parameter types
//               and a new .flit.conf param: oss_control_name to favor a specific
//               OSS control parameter, (e.g. pcm, vol, or vmix0-outvol)
//               Note: vmix controls will not work unless the vmix device is fully 
//               "attached" to your hardware.

// Version 0.9.0 added support for MIXT_STEREODB OSS parameter type

// Version 0.8.9 rearranged some code to make transparent style work better.

// Version 0.8.8 switches to direct OSS API (ioctl() stuff) for sound management.
//      Hopefully, it will support a wider variety of sound hardware.
//      Also, this version peeks under the actual intended location
//      of the flit window to determine the "transparent" color.


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
#include <errno.h>
#include <argp.h>

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
#include "pidfile.h"

// UNCOMMENT TO SUPPORT OLPC-1 BATTERY MONITOR
#define OLPC

// UNCOMMENT TO PRINT DIAGNOSTIC MESSAGES
//#define DIAG

// This adds a bit of gloss to the icons
#define VANITY 1

#ifndef MIN
#define MIN(a, b)   (((a) < (b)) ? (a):(b))
#endif

#define APP_VER "1.3.2" // Last update 2019-03-08
#define PROG    "flit"
#define METER_W     28
#define SOUND_W     28
#define CLOCK_W     64
#define SIZE_H      20
#define SIZE_W      (METER_W + SOUND_W + CLOCK_W)

#define SECONDS_PER_HOUR                3600
#define SAMPLES_PER_HOUR                720
#define MIN_SAMPLES_BEFORE_ESTIMATION   36

#define BATTERY_BASE_DIR "/proc/acpi/battery/"
#define PATH_TO_FLIT_HELP "/usr/local/share/doc/flit/flit_help.htm"

// argp stuff -----------------------------------------------------------------
#define PROGNAME_VERSION PROG " " APP_VER

const char *argp_program_version = PROGNAME_VERSION;
const char *argp_program_bug_address = "<mlockmoore@gmail.com>; <jbysewski@googlemail.com>";

// Volume level saved in config file.
int saved_volume = -1;

// This structure is used by main to communicate with parse_opt
struct arguments
{
    bool on_all_desktops;       // should the application appear on all desktops
    bool dock;                  // should the application be docked
    bool multiple_instances;    // allow multiple instances
    char *config_file;          // alternative config file to use
};

/*
 * Options. Field 1 in ARGP
 * Order of fields: {NAME, KEY, ARG, FLAGS, DOC}
 */
static struct argp_option options[] =
{
    {"alldesktops", 'a', 0,             0, "Make flit appear on all desktops (experimental)"},
    {"dock",        'd', 0,             0, "Make flit dock by setting _NET_WM_WINDOW_TYPE_DOCK"},
    {"multiple",    'm', 0,             0, "Allow multiple instances of flit"},
    {"config",      'c', "CONFIGFILE",  0, "Use CONFIGFILE instead of ~/.flit.conf"},
    {0}
};

/*
 * Parser. Field 2 in ARGP
 */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments*)state->input;

    switch(key)
    {
        case 'a':
            arguments->on_all_desktops = true;            
            break;
        case 'd':
            arguments->dock = true;
            break;
        case 'm':
            arguments->multiple_instances = true;
            break;
        case 'c':
            arguments->config_file = arg;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

/*
 * ARGS_DOC. Field 3 in ARGP
 * Only needed for additional parameters
 */
static char args_doc[] = "";


/*
 * DOC. Field 4 in ARGP
 */
static char doc[] =
"flit -- A 'systray' with clock, sound and battery applets";

/*
 * The ARGP structure
 */
static struct argp argp = {options, parse_opt, args_doc, doc};

// end argp stuff -------------------------------------------------------------

const char About_text[] = 
"Flit version %s\n"
"copyright 2010 by Michael A. Losh and Jakob Bysewski\n"
"\n"
"Flit is an applet 'tray', with clock, sound control, and battery monitor.\n"
"Flit is free software: you can redistribute it and/or\n"
"modify it under the terms of the GNU General Public License\n"
"as published by the Free Software Foundation, version 3.\n"
"\n"
"Flit is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
"See http://www.gnu.org/licenses/ for more details.\n"
"Send bug reports to: mlockmore@@gmail.com";

int Base_t_sec;
int Running = 1;
enum {  MI_ABOUT, MI_HELP,  MI_CLOCK, MI_SHOW24HR, 
        MI_SOUND, MI_SOUND_MUTE, MI_SOUND_UP, MI_SOUND_DOWN, MI_BATTERY, 
        MI_HOTKEY_TOGGLE, MI_NORMAL_STYLE, MI_INVERSE_STYLE, MI_TRANSPARENT_STYLE, MI_CUSTOM_STYLE, 
        MI_SAVE_CONFIG, MI_QUIT};

static void MenuCB(Fl_Widget* window_p, void *userdata);
Fl_Menu_Item right_click_menu[18] = {
    {"&About",                   FL_CTRL+'a', MenuCB, (void*)MI_ABOUT},
    {"&Help",                    FL_CTRL+'h', MenuCB, (void*)MI_HELP,FL_MENU_DIVIDER},
    {"Show/hide cloc&k",         FL_CTRL+'k', MenuCB, (void*)MI_CLOCK},
    {"&Toggle 12/24 hr. disp.",  FL_CTRL+'t', MenuCB, (void*)MI_SHOW24HR, FL_MENU_DIVIDER},
    {"Show/hide soun&d",         FL_CTRL+'d', MenuCB, (void*)MI_SOUND},
    {"&Mute/unmute",             FL_CTRL+'m', MenuCB, (void*)MI_SOUND_MUTE},
    {"&Up the sound",            FL_CTRL+'u', MenuCB, (void*)MI_SOUND_UP},
    {"&Lower the sound",         FL_CTRL+'l', MenuCB, (void*)MI_SOUND_DOWN, FL_MENU_DIVIDER},
    {"Show/hide &battery meter", FL_CTRL+'b', MenuCB, (void*)MI_BATTERY, FL_MENU_DIVIDER},
    {"Toggle Ctrl key menu activation", FL_CTRL+'y', MenuCB, (void*)MI_HOTKEY_TOGGLE},
    {"&Normal style",            FL_CTRL+'n', MenuCB, (void*)MI_NORMAL_STYLE},  
    {"&Inverse style",           FL_CTRL+'i', MenuCB, (void*)MI_INVERSE_STYLE}, 
    {"Trans&parent style",       FL_CTRL+'p', MenuCB, (void*)MI_TRANSPARENT_STYLE},    
    {"&Custom style",            FL_CTRL+'c', MenuCB, (void*)MI_CUSTOM_STYLE, FL_MENU_DIVIDER},    
    {"&Save configuration",      FL_CTRL+'s', MenuCB, (void*)MI_SAVE_CONFIG, FL_MENU_DIVIDER},  
    {"&Quit",                    FL_CTRL+'q', MenuCB, (void*)MI_QUIT},
    {0}
};

float       Z = 1.0;   // Zoom - Size Scaling factor
uchar  Fg_r;    // Normal forground color, typically black
uchar  Fg_g;
uchar  Fg_b;

uchar  Bg_r;    // Normal background color, often gray
uchar  Bg_g;
uchar  Bg_b;

uchar C_Fg_r = 0x00;    // Custom foreground color from config file
uchar C_Fg_g = 0x00;
uchar C_Fg_b = 0x1f;

uchar C_Bg_r = 0x58;    // Custom background color from config file
uchar C_Bg_g = 0x7d;
uchar C_Bg_b = 0xaa;

uchar  RW_r = 0x40; // X11 root window background color
uchar  RW_g = 0x50;
uchar  RW_b = 0x80;

enum {BATT_STATE_UNK, BATT_STATE_CHARGED, BATT_STATE_CHARGING, BATT_STATE_DISCHARGING};
enum {NORMAL_STYLE = 0, INVERSE_STYLE = 1, TRANSPARENT_STYLE = 2, CUSTOM_STYLE = 3};
enum {LOC_SE, LOC_SW, LOC_NW, LOC_NE, LOC_CUST};

#ifdef OLPC
enum {BATT_SYSTEM_ACPI, BATT_SYSTEM_OLPC};
int BatterySystemType = BATT_SYSTEM_ACPI;

#define OLPC_BATT_CAP_DEVFILE   "/sys/devices/platform/olpc-battery.0/power_supply/olpc-battery/capacity" 
//#define OLPC_BATT_CAP_DEVFILE   "/home/tc/projects/flit/capacity" 

#define OLPC_BATT_AC_DEVFILE   "/sys/devices/platform/olpc-battery.0/power_supply/olpc-ac/online"
//#define OLPC_BATT_AC_DEVFILE   "/home/tc/projects/flit/online"
#endif

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
            fl_rectf(4 * Z, 4 *Z, 10 * Z, 2 * Z);
            fl_polygon(12*Z,3*Z, 15*Z,2*Z, 15*Z,7*Z, 12*Z,6*Z );
            fl_line(15*Z, 3*Z, 18*Z, 3*Z);
            fl_line(15*Z, 6*Z, 18*Z, 6*Z);
        }
        if (level < 13  && toggle % 2 == 0 && !show_plug) {
            fl_color(FL_RED);   // outline in red every other update
        }
        fl_rectf(2*Z, 10*Z, 22*Z, 8*Z);
        fl_line (24*Z, 12*Z, 24*Z, 15*Z);
        fl_color(FL_BLUE);
        fl_rectf(3*Z, 11*Z, 20*Z, 6*Z);
        fl_line (23*Z, 13*Z, 23*Z, 14*Z);
#ifdef VANITY
        // A concession to vanity: a bit of highlight
        fl_color(0x80, 0x80, 0xFF); // light blue
        fl_line(4*Z, 12*Z, 22*Z, 12*Z);
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
        int charge_w = (level + 2) / 5;     // + 2 does a little rounding
        fl_rectf(3*Z, 11*Z, charge_w*Z, 6*Z);

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
        fl_line(3*Z, 12*Z, (2 + charge_w)*Z, 12*Z);
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
        fl_rectf(x() + 5*Z, 7*Z, 3*Z, 6*Z); // coil color
        fl_polygon(x()+8*Z,6*Z, x()+12*Z,2*Z, x()+12*Z,18*Z, x()+8*Z,14*Z );    // horn color
        
        fl_color(FL_FOREGROUND_COLOR);
        fl_rect(x() + 4*Z, 6*Z, 4*Z, 8*Z);  // coil outline
        fl_loop(x()+8*Z,6*Z, x()+12*Z,2*Z, x()+12*Z,18*Z, x()+8*Z,14*Z );       // horn outline
        fl_loop(x()+13*Z,7*Z, x()+14*Z,9*Z, x()+14*Z,11*Z, x()+13*Z,13*Z );     // dome outline
        
        fl_color(0x40, 0x40, 0x40); // dark gray
        fl_loop(x()+13*Z,7*Z, x()+14*Z,9*Z, x()+14*Z,11*Z, x()+13*Z,13*Z );     // dome outline
        
        if (level < 1 ) {
            fl_color(FL_RED);   
            fl_arc(x()+4*Z, 2*Z, 16*Z, 16*Z, 0.0, 359.9);
            fl_arc(x()+5*Z, 3*Z, 14*Z, 14*Z, 0.0, 359.9);
            fl_line(x()+7*Z, 5*Z, x()+16*Z, 14*Z);
            fl_line(x()+8*Z, 5*Z, x()+17*Z, 14*Z);
        }
#ifdef VANITY
        // A concession to vanity: a bit of highlight
        fl_color(0xE0, 0xE0, 0xE0); // light gray
        fl_line(x()+5*Z, 9*Z, x()+7*Z, 9*Z);
        fl_line(x()+9*Z, 7*Z, x()+11*Z, 5*Z); 
#endif      
        fl_color(Bg_r + Fg_r / 2, Bg_g + Fg_g / 2, Bg_b + Fg_b / 2);
        if (level > 0) {
            fl_color(FL_FOREGROUND_COLOR);
            fl_line(x() + 16*Z, 15*Z, x() + 18*Z, 17*Z);
        }
        if (level >= 10) {
            fl_line(x() + 18*Z, 17*Z, x() + 19*Z, 19*Z);
        }
        if (level >= 20) {
            fl_line(x() + 17*Z, 13*Z, x() + 19*Z, 14*Z);
        }
        if (level >= 30) {
            fl_line(x() + 19*Z, 14*Z, x() + 21*Z, 15*Z);
        }       
        if (level >= 40) {
            fl_line(x() + 17*Z, 11*Z, x() + 19*Z, 11*Z);
        }
        if (level >= 50) {
            fl_line(x() + 19*Z, 11*Z, x() + 22*Z, 11*Z);
        }
        if (level >= 60) {
            fl_line(x() + 17*Z, 9*Z, x() + 19*Z, 8*Z);
        }
        if (level >= 70) {
            fl_line(x() + 19*Z, 8*Z, x() + 21*Z, 7*Z);
        }
        if (level >= 80) {
            fl_line(x() + 16*Z, 6*Z, x() + 18*Z, 4*Z);
        }
        if (level >= 90) {
            fl_line(x() + 18*Z, 4*Z, x() + 20*Z, 2*Z);
        }

    }
};

enum { SOUND_TYPE_OSS, SOUND_TYPE_ALSA};

class Flit_Frame : public Fl_Window {
    protected:
        int     size_w;
        int     size_h;
        int     win_loc;
        int     style;
        int     win_x;
        int     win_y;
        char    configfilename[256];
        const char *configfile_option;

        int     show_sound;
        int     sound_type;
        Flit_Sound_Control* sound_p;
        int     prev_sound_vol;
        int     mixer_fd;
        int     mix_ctrl;
        int     mix_min;
        int     mix_max;
        char    sound_control_name[64];
        char    alsa_amixer_path[128];
    
        int     show_clock;
        Fl_Box* clock_p;
        int     show24hr;
        char    timestr[16];
        char    ampm[4];
        time_t  last_update_time;
        
        
        int     show_battery;
        Flit_Batt_Meter* meter_p;
        int     samples;
        float   capacity_mAh;
        float   charge_mAh;
        float   orig_charge_mAh;
        float   avgcur_mA;
        float   charge_pct;
        float   remaining_hrs;
        int     batt_state;
        char    meter_str[8];
        char    meter_tip[64];
        char    time_tip[64];
        char    batt_state_filename[128];

    public:
        int     menu_hotkey_activation;
        void set_color(Fl_Color c) {
            if (meter_p)    meter_p->labelcolor(c);
            if (sound_p)    sound_p->labelcolor(c);
            if (clock_p)    clock_p->labelcolor(c);
        }
        
        void set_boxtype(Fl_Boxtype boxtype) {
            if (meter_p)    meter_p->box(boxtype);
            if (sound_p)    sound_p->box(boxtype);
            if (clock_p)    clock_p->box(boxtype);
            redraw();
        }
        
        Flit_Frame(const char *title = 0, const char *configfile_option = 0) :
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
          sound_type(SOUND_TYPE_OSS),
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
          mix_ctrl(-1),
          menu_hotkey_activation(1),
          last_update_time(0),
          configfile_option(configfile_option)
        {
            Base_t_sec = time(NULL);
            strcpy(sound_control_name, "autosel");
        };
        
        void get_time(void) {
          time_t now        = time(NULL);
          struct tm* timep  = localtime(&now);
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
          
          sprintf(timestr, "%d:%02d%s", hour, timep->tm_min, ampm);
          clock_p->label(timestr);
          strcpy(time_tip, ctime(&now));
          clock_p->tooltip(time_tip);
          clock_p->redraw();
        };
        
        void get_sound_info(void) {
            int c;
            int success = 0;
            int autosel = 0;    // Whether Flit should pick a control
            oss_mixext info;
            mix_ctrl = 0;
            autosel = 0;
            if (!strcmp(sound_control_name, "autosel") ) {
printf("Will autoselect sound control parameter\n");
                autosel = 1;
            }
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
                    for (c = 0 ; c < mix_ctls ; c++ ) {
                        info.dev = 0;
                        info.ctrl = c;
                        if (-1 != ioctl(mixer_fd, SNDCTL_MIX_EXTINFO, &info)) {
#ifdef DIAG                     
                            printf("--->\t%d\tConsidering volume control item: '%s', type %d, flags 0x%X\n", 
                                c, info.extname, info.type, info.flags);
#endif              
                            if (    !success 
                                &&  (   !strcmp(info.extname, sound_control_name)  // found the specified OSS control?
                                    ||  (   autosel == 1
                                        &&  (   (info.flags & MIXF_MAINVOL)             // OSS thinks it can control volume
                                            ||  (info.flags & MIXF_PCMVOL)              // ''
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
                                printf("===>\t%d\tmin %d, max %d,\n", c, mix_min, mix_max);
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
                    printf("\n");
                }
#endif                
            }
            
            if (!success) {
                // Check for ALSA sound
#ifdef DIAG
                printf("Looking for ALSA sound...\n");
#endif
                char potential_sound_control[48];
                FILE* which_pf = 0;
                *alsa_amixer_path = '\0';
                *potential_sound_control = '\0';
                which_pf = popen("which amixer", "r");
                if(fgets(alsa_amixer_path, 127, which_pf)) {
#ifdef DIAG
                    printf("AMIXER path is %s\n", alsa_amixer_path);
#endif
                }
                fclose(which_pf);

                if (strlen(alsa_amixer_path) > 1) {
                    sound_type = SOUND_TYPE_ALSA;
                                                
                    FILE* amixerpf = NULL;
                    char cmd[128];
                    char buf[512];
                    char *s;
                    int need_autosel = autosel;
                    int need_lims = 1;
                    int newval;
                    int minval = -1;
                    int maxval = -1;
                    if (need_autosel) {
                        sprintf(cmd, "amixer", sound_control_name);
                    }
                    else {
                        sprintf(cmd, "amixer sget %s", sound_control_name);
                    }
                    amixerpf = popen(cmd, "r");
                    if (amixerpf) {
                        while(!feof(amixerpf) && fgets(buf, 511, amixerpf)) {
                            // look for limits
                            s = strstr(buf, "Limits:");
                            if (need_lims && s && (2 == sscanf(s, "Limits: Playback %d - %d", &minval, &maxval) || 2 == sscanf(s, "Limits: %d - %d", &minval, &maxval))) {
                                mix_min = minval;
                                mix_max = maxval;
#ifdef DIAG
                                printf("Sound volume ranges from %d to %d\n", mix_min, mix_max);
#endif
                                success = 1;
                                need_lims = 0;

                                continue; 
                            }
                            s = strstr(buf, "Simple mixer control");
                            if (need_autosel && s && 1 == sscanf(s, "Simple mixer control '%[^']", potential_sound_control)) {
#ifdef DIAG
                                printf("Considering potential sound control '%s'\n",  potential_sound_control);
#endif
                                continue;
                            }

                            //s = strstr(buf, "pvolume");
                            s = strstr(buf, "volume");
                            if (need_autosel && s && strlen(potential_sound_control) > 0) {
                                strcpy(sound_control_name, potential_sound_control);
#ifdef DIAG
                                printf("Accepted sound countrol '%s'\n", sound_control_name);
#endif
                                need_autosel = 0;
                                continue;
                            }
                            else {
                                *potential_sound_control = '\0'; // blank any current candidate out
                                continue;
                            }

                        }
                        fclose(amixerpf);
                    }
                }
                else {
#ifdef DIAG
                    printf("ALSA amixer not found!\n");
#endif
                }
            }
                
            if (!success) {
                printf("No sound found!\n");
                show_sound = 0;
                mixer_fd = 0;
            }
        }

        void get_sound_level(void) {
            oss_mixext info;
            oss_mixer_value vr;
            int pct;
            int rawval;
            if (sound_type == SOUND_TYPE_OSS && mixer_fd && mix_ctrl) {
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
                            case MIXT_SLIDER:   // 32-bit
                            default:            // Maybe MIXT_VALUE, MIXT_HEXVALUE, MIXT_MONODB, MIXT_STEREODB
                                rawval = vr.value;
                                break;
                        }
                                
                        pct = (int)(100.0 * (float)(rawval - mix_min) / (float)mix_max);                        
#ifdef DIAG                     
                        printf("Read 0x%08X from control, raw volume is %d or %d %%\n", 
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
            else if (sound_type == SOUND_TYPE_ALSA) {
                FILE* amixerpf = NULL;
                char buf[512];
                char *s;
                int lvol = -1;
                int rvol = -1;
                int v;
                char cmd[128];
                sprintf(cmd, "amixer sget %s", sound_control_name);
                amixerpf = popen(cmd, "r");
                if (amixerpf) {
                    while(!feof(amixerpf) && fgets(buf, 511, amixerpf)) {
                        //s = strstr(buf, "Front Left: Playback");
                        s = strstr(buf, "Front Left:");
                        if (s && (1 == sscanf(s, "Front Left: Playback %d", & v) || 1 == sscanf(s, "Front Left: %d", & v))) {
                            lvol = v;
                            //continue;
                        }
                        //s = strstr(buf, "Front Right: Playback");
                        s = strstr(buf, "Front Right:");
                        if (s && (1 == sscanf(s, "Front Right: Playback %d", & v) || 1 == sscanf(s, "Front Right: %d", & v))) {
                            rvol = v;
                        }
                        if (lvol >= 0 && rvol >= 0 && (mix_max > mix_min)) {
//RR                            pct = (int)(((float)(rvol + lvol)) * 100.0 / (2.0 * (float)(mix_max - mix_min)));
						pct = (int)(roundf(((rvol + lvol) * (100.0 / ((float)(mix_max - mix_min) * 2.0)))));
#ifdef DIAG
                            printf("Sound level is %d %%\n", pct);
#endif
                            sound_p->value(pct);
                            break;
                        }
                    }
                    fclose(amixerpf);
                }
            }
            else {
#ifdef DIAG  
                printf("No sound device or suitable control\n");
#endif              
            }
        };
        
        void set_sound_level(int vol) {
            oss_mixext info;
            oss_mixer_value vr;
            int newvol;
            int intended;
//RR            newvol = (int)((float)(vol * mix_max) / 100.0) - mix_min;
            newvol = (int)(roundf((vol * ((float)(mix_max - mix_min) / 100.0))));
#ifdef DIAG
            printf("vol=%d newvol=%d mix_max=%d mix_min=%d\n", vol, newvol, mix_max, mix_min);
#endif
            if (sound_type == SOUND_TYPE_OSS && mixer_fd && mix_ctrl) {
                info.dev = 0;
                info.ctrl = mix_ctrl;               
                if (-1 != ioctl(mixer_fd, SNDCTL_MIX_EXTINFO, &info)) {
                    vr.dev=info.dev;
                    vr.ctrl=info.ctrl;
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
                        case MIXT_SLIDER:   // 32-bit
                        default:            // Maybe MIXT_VALUE, MIXT_HEXVALUE, MIXT_MONODB, MIXT_STEREODB
                            vr.value = newvol;
                            break;
                    }
                    intended = vr.value;        
                    if (-1 != ioctl(mixer_fd, SNDCTL_MIX_WRITE, &vr)) {
#ifdef DIAG                     
                        printf("Written value is 0x%08X, intended value 0x%08X (for %d %%)\n", 
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
            else if (sound_type == SOUND_TYPE_ALSA) {
                FILE* amixerpf = NULL;
                char cmd[128];
                char buf[512];
                char *s;
                int minval, maxval;
                int lvol = -1;
                int rvol = -1;
                int v;
                int actual_pct;
                sprintf(cmd, "amixer sset %s %d", sound_control_name, newvol);
#ifdef DIAG
                printf("********************\nCmd for intended %d pct: '%s'\n", vol, cmd);
#endif
                amixerpf = popen(cmd, "r");
                if (amixerpf) {
                    while(!feof(amixerpf) && fgets(buf, 511, amixerpf)) {
                        s = strstr(buf, "Mono: Playback");
                        if (s && 1 == sscanf(s, "Mono: Playback %d", & v)) {
                            lvol = v;
                            rvol = v;
                            continue;
                        }
                        //s = strstr(buf, "Front Left: Playback");
                        s = strstr(buf, "Front Left:");
                        if (s && (1 == sscanf(s, "Front Left: Playback %d", & v) || 1 == sscanf(s, "Front Left: %d", & v))) {
                            lvol = v;
                            continue;
                        }
                        //s = strstr(buf, "Front Right: Playback");
                        s = strstr(buf, "Front Right:");
                        if (s && (1 == sscanf(s, "Front Right: Playback %d", & v) || 1 == sscanf(s, "Front Right: %d", & v))) {
                            rvol = v;
                            continue;
                        }
                    }
                    if (lvol >= 0 && rvol >= 0 && (mix_max > mix_min)) {
//RR                        actual_pct = (rvol + lvol) * 100.0 / (2.0 * (mix_max - mix_min));
						actual_pct = (int)(roundf(((rvol + lvol) * (100.0 / ((float)(mix_max - mix_min) * 2.0)))));
#ifdef DIAG
                        printf("Sound level after change is actually %d %%\n", actual_pct);
#endif
                        sound_p->value(actual_pct);
                    }
                    fclose(amixerpf);
                }
               
            }
            else {

#ifdef DIAG                     
                printf("No sound device or suitable control\n");
#endif              
            }
        };
        
        void get_battery_info(void) {
            char dirname[32] = "";
            char buf[80] = "";
            DIR* bd = NULL;   // batt. directory file ptr
            FILE* bif = NULL;   // batt. info file ptr
            struct dirent* dp;
#ifdef OLPC
            bif = fopen(OLPC_BATT_CAP_DEVFILE, "r");
            if (bif) {
                BatterySystemType = BATT_SYSTEM_OLPC;
                fclose(bif);
                return;
            }
            else {
                BatterySystemType = BATT_SYSTEM_ACPI;
            }
#endif
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

        void update_battery_meter(void) {
                int hours = (int)floor(remaining_hrs);
                int mins  = (int)((remaining_hrs - hours)*60.0);
                sprintf(meter_str, "%1.f%%\n-",charge_pct);
                if (batt_state == BATT_STATE_CHARGED) {
                    strcpy(meter_tip, "Fully charged");
                }
                else if (samples < MIN_SAMPLES_BEFORE_ESTIMATION) {
                    sprintf(meter_tip, "%1.f %%", charge_pct);
                }
                else {
                    sprintf(meter_tip, "%1.f %%, about %dhr%s, %dmin%s %s", 
                            charge_pct, hours, (hours==1) ? "":"s", 
                            mins,  (mins==1) ? "":"s", 
                            (batt_state == BATT_STATE_CHARGING) ? "to charge":"remaining"
                           );
                }
                meter_p->label(meter_str);
                meter_p->tooltip(meter_tip);
                meter_p->value((int)floor(charge_pct));
                meter_p->redraw();
                return;
        }

        void get_battery_state(void) {
            int need_state = 1;
            int need_charge = 1;
            int want_rate = 1;
            char buf[80];
            char status[20];
            float chg_mAh = 0.0;
            float rate_mA = 0.0;
            float mA = 1;
            float ratio = 0.0;
            FILE* bsf = NULL;
            FILE* bif = NULL;   // batt. info file ptr
            struct dirent* dp;
#ifdef OLPC     // vvvvvvvvvvvvvvvvvvvvv
            static int cap_initialized = 0;
            static unsigned long int t_of_prev_pcnt = 0; // when the cap. percetage last changed;
            static int prev_cap_pcnt = 0;
            static float rate_pcnt = 100.0 / (3.0 * 3600.0);    // just assume 3 hrs of charging/discharging now
            static int prev_ac_present = 0;
            int cap_pcnt = -1;
            int cap_pcnt_change = 0;
            int interval_t = 0;
            if (BatterySystemType == BATT_SYSTEM_OLPC) {
                bsf = fopen(OLPC_BATT_CAP_DEVFILE, "r");
                if (bsf) {
                    fscanf(bsf, "%d", &cap_pcnt);
                    if (cap_pcnt >= 0) {

                        if (cap_initialized) {
                            cap_pcnt_change = cap_pcnt - prev_cap_pcnt;
                            if (cap_pcnt_change != 0) {
                                interval_t = time(NULL) - t_of_prev_pcnt;
                            }
                        }
                            
                        charge_pct = ((float) cap_pcnt ) * 100.0 / 97.0;
                        if (charge_pct > 100.0) {
                            charge_pct = 100.0;
                        }
#ifdef DIAG
                        printf("OLPC raw charge percentage is %d%%, rescaled to %1.0f %%\n",  
                                cap_pcnt, charge_pct);
#endif
                    }
                    samples++;
                    fclose(bsf);
                }
#ifdef DIAG
                else {
                        printf("ERROR: Cannot check battery charge status!\n");
                }
#endif
                FILE* acif = NULL;   // AC info file ptr
                int charging = -1;
                acif = fopen(OLPC_BATT_AC_DEVFILE, "r");
                if (acif) {
                    fscanf(acif, "%d", &charging);
                    meter_p->show_plug = 0; // assume discharging
                    if (charging == 1) {
                        if (prev_ac_present != 1) {
                            samples = 0;  // state change, don't report time est. for a while
                            rate_pcnt = 0.0;
                        }
                        prev_ac_present = 1;
                        meter_p->show_plug = 1; 
                        batt_state = BATT_STATE_CHARGING;
                        if (interval_t > 0) {
                            // do some low-pass filtering on rate of change
                            rate_pcnt = ((7.0 * rate_pcnt )+ ((float)cap_pcnt_change / (float)interval_t)) / 8.0;
                        }
                        if (fabs(rate_pcnt) > 0.00000) {
                            remaining_hrs = (100.0 - charge_pct)  / (3600.0 * rate_pcnt);
                        }
#ifdef DIAG
                        printf("cap_pcnt_change is %d interval_t is %d, rate_pcnt is %1.3f, cap_pcnt is %d, remaining_hrs is %1.4f\n",
                                cap_pcnt_change, interval_t, rate_pcnt, cap_pcnt, remaining_hrs); 
#endif                        
                        
                        if (charge_pct >= 99.0) {
                            batt_state = BATT_STATE_CHARGED;
                            remaining_hrs = 0.0;
                        }
                    }
                    else if (charging == 0) {
                        if (prev_ac_present != 0) {
                            samples = 0;  // state change, don't report time est. for a while
                            rate_pcnt = 0.0;
                        }
                        prev_ac_present = 0;
                        batt_state = BATT_STATE_DISCHARGING;
                        if (interval_t > 0) {
                            // Change rate slowly to match recent "instantaneous" rate
                            rate_pcnt = ((7.0 * rate_pcnt )+ ((float)cap_pcnt_change / (float)interval_t)) / 8.0;
                        }
                        if (fabs(rate_pcnt) > 0.00000) {
                            remaining_hrs = charge_pct / (-3600.0 * rate_pcnt);
                        }
#ifdef DIAG
                        printf("cap_pcnt_change is %d interval_t is %d, rate_pcnt is %1.3f, cap_pcnt is %d, remaining_hrs is %1.4f\n",
                                cap_pcnt_change, interval_t, rate_pcnt, cap_pcnt, remaining_hrs); 
#endif                        
                    }
                    else {
                        batt_state = BATT_STATE_UNK;
                        prev_ac_present = -1;
                    }
                        
                    if (cap_initialized == 0 || cap_pcnt_change != 0) {
                        t_of_prev_pcnt = time(NULL);
                        prev_cap_pcnt = cap_pcnt;
                        cap_initialized = 1;
                    }
                    fclose (acif);
                }
                
                update_battery_meter();
                return;
            }
#endif   // OLPC ^^^^^^^^^^^^^

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
                    else if (want_rate && 1 == sscanf(buf, "present rate: %f mA\n", &rate_mA)) {
                        want_rate = 0;
                        if (rate_mA > 32767.0) {
                            rate_mA = 65536.0 - rate_mA;    // handle negative for charging
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
                if (!need_charge && !need_state) {
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
                        if (rate_mA < 1.0) {
                            // Calculate charging rate since ACPI did not provide
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
                   update_battery_meter();
#ifdef DIAG 
                    printf("Charge/discharge state is %s (flit code %d)\n", status, batt_state);
                    printf("Current is %1.f mA, average current is %1.1f mA\n", rate_mA, avgcur_mA);
                    printf("Charge level was %1.f mAh at sample zero\n", orig_charge_mAh);
                    printf("Charge level is %1.f mAh or %1.1f %% of %1.f possible\n", charge_mAh, charge_pct, capacity_mAh);
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
                vol -= 3;
                if (vol < 0) vol = 0;
            }
            else if (direction > 0) {
                prev_sound_vol = vol;
                vol += 3;
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
            Fl_Tooltip::delay(0);       // force tooltip on
            Fl_Tooltip::exit(sound_p);
            Fl_Tooltip::enter(sound_p);
            Fl_Tooltip::delay(1);
        }
        
        int handle(int e) {
            static int off_x = 0;
            static int off_y = 0;
            int key;
            switch ( e ) {
            case FL_KEYDOWN:
                key = Fl::event_key();
                if (menu_hotkey_activation && (key == FL_Control_L || key == FL_Control_R)) {
                    do_menu();
                    return 1;
                }
                break;
            case FL_KEYUP:
                key = Fl::event_key();
                if (show_sound &&  (key == FL_Up || key == '=')) {  // '=' is also + key                
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
                break;
            }
            return(Fl_Window::handle(e));
        };
        void update(void) {
            time_t now      = time(NULL);
            if (now - last_update_time >= 5) {
                if (show_clock)     get_time();
                if (show_sound)     get_sound_level();
                if (show_battery)   get_battery_state();
                last_update_time = now;
        arrange();
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
            float f;
            int xpos, ypos;
        int r,g,b;

            if(configfile_option) {
                strcpy(configfilename, configfile_option);
            }
            else {
                char* env_home_p = getenv("HOME");
                sprintf(configfilename, "%s/.flit.conf", env_home_p);
            }

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
                    else if (1 == sscanf(confline, "menu_hotkey_activation = %d\n", &n)) {
#ifdef DIAG     
                        printf("menu_hotkey_activation is %d\n", n);
#endif
                        menu_hotkey_activation = n;
                    }
                    else if (1 == sscanf(confline, "style = %d\n", &n)) {
#ifdef DIAG     
                        printf("style is %d\n", n);
#endif
                        style = n;
                    }
                    else if (1 == sscanf(confline, "zoom = %f\n", &f)) {
#ifdef DIAG     
                        printf("zoom is %1.2f\n", f);
#endif
                        Z = f;
                    }
                    else if (1 == sscanf(confline, "sound_control_name = %s\n", s)) {
                        if (strlen(s) > 0 && strlen(s) < 63) {
                            strcpy(sound_control_name, s);
                        }
#ifdef DIAG     
                        printf("sound_control_name is %s\n", sound_control_name);
#endif
                        
                    }
                    else if (1 == sscanf(confline, "sound_volume_level = %d\n", &n)) {
#ifdef DIAG     
                        printf("volume is %d\n", n);
#endif
                        if((n >= 0) && (n <= 100))
							saved_volume = n;
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
            else if(1 == sscanf(confline, "custom_fg = %s\n", s)) {
                if(3 == sscanf(s, "%d,%d,%d", &r, &g, &b)) {
                    C_Fg_r = r;
                    C_Fg_g = g;
                    C_Fg_b = b;
                }
            }
            else if(1 == sscanf(confline, "custom_bg = %s\n", s)) {
                if(3 == sscanf(s, "%d,%d,%d", &r, &g, &b)) {
                    C_Bg_r = r;
                    C_Bg_g = g;
                    C_Bg_b = b;
                }
            }
                } // end while more lines.
            }
            else {
#ifdef DIAG
                printf("No config file found: using defaults.\n");
#endif              
            }
        };  // end get_config()

        void arrange(void) {
            size_h = SIZE_H * Z;
            size_w = 0;
            begin();
            if (show_battery) {
                if (!meter_p) {
                    meter_p = new Flit_Batt_Meter(0, 0, METER_W * Z, SIZE_H * Z);
                    get_battery_info();
                    meter_p->labelsize(8 * Z);
                }
                meter_p->position(size_w, 0);
                size_w += METER_W * Z;
            }
            else if (meter_p) { 
                meter_p->position(size_w, 0);
            }

            if (show_sound) {
                if (!sound_p) {
                    sound_p = new Flit_Sound_Control(0, 0, SOUND_W * Z, SIZE_H * Z);
                    sound_p->labelsize(8 * Z);
                }
                sound_p->position(size_w, 0);
                size_w += SOUND_W * Z;
            }
            else if (sound_p) {           
                sound_p->position(0, 0);
            }

            if (show_clock) {   
                if (!clock_p) {
                    clock_p = new Fl_Box(0, 0, CLOCK_W * Z, SIZE_H * Z);
                    get_time();
                    clock_p->labelsize(12 * Z);
                }
                clock_p->position(size_w, 0);
                size_w += CLOCK_W * Z;
            }
            else if (clock_p) {
                clock_p->position(0, 0);
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
                if (clock_p) clock_p->hide();
            }
            else if (!show_clock) { // can always re-enable clock
                show_clock = 1;
                if (clock_p) clock_p->show();
            }
            arrange();
            set_style(style);
        };
        
        void toggle_show_sound(void) {
            if (show_sound && (show_clock + show_battery)) // can't disable sound ctrl unless one other is showing
            {
                show_sound = 0;
                mixer_fd = 0;
                if (sound_p) sound_p->hide();
            }
            else if (!show_sound) { // can always re-enable sound
                show_sound = 1;
                if (sound_p) sound_p->show();
                get_sound_info();   // checks for availability
                if (!show_sound) {
                    if (sound_p) sound_p->hide();
                    fl_alert("No sound system (e.g. OSS) found.\n");
                }
            }
            arrange();
            set_style(style);
        };
        
        void toggle_show_batt(void) {
            if (show_battery && (show_sound + show_clock)) // can't disable battery meter unless one other is showing
            {
                show_battery = 0;
                if (meter_p) meter_p->hide();
            }
            else if (!show_battery) {   // can always re-enable battery
                show_battery = 1;
                if (meter_p) meter_p->show();
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
                    bg_r = MIN(0xFF, RW_r + 50);
                    bg_g = MIN(0xFF, RW_g + 50);
                    bg_b = MIN(0xFF, RW_b + 50);
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
                case CUSTOM_STYLE:
                    Fl::set_color(FL_BACKGROUND_COLOR, C_Bg_r, C_Bg_g, C_Bg_b);
                    Fl::set_color(FL_FOREGROUND_COLOR, C_Fg_r, C_Fg_g, C_Fg_b);
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
                RW_r = (uchar)xip->data[2]; // This may only work with true-color color modes
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
                fprintf(cfgf, "# OSS/ALSA sound control name to use, autosel means 'try to pick for me'\n");
                fprintf(cfgf, "sound_control_name = %s\n", sound_control_name);
                fprintf(cfgf, "\n");
                fprintf(cfgf, "# OSS/ALSA sound volume level (0 to 100)\n");
                fprintf(cfgf, "sound_volume_level = %d\n", sound_p->value());
                fprintf(cfgf, "\n");
                fprintf(cfgf, "# Automatically pop up right-click menu when Ctrl key is first pressed (1 = do, 0 = don't)\n");
                fprintf(cfgf, "menu_hotkey_activation = %d\n", menu_hotkey_activation);
                fprintf(cfgf, "\n");
                fprintf(cfgf, "# Color style: 0 is normal, 1 is inverted, 2 is 'transparent' (based on root window color), 3 is custom style\n");
                fprintf(cfgf, "style = %d\n", style);
                fprintf(cfgf, "\n");
                fprintf(cfgf, "# Zoom size: 1.0 is normal, 1.50 is %150, etc.\n");
                fprintf(cfgf, "zoom = %1.2f\n", Z);
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
                fprintf(cfgf, "# Custom foreground and background color color (r,g,b) 0-255:\n");
                fprintf(cfgf, "custom_fg = %u,%u,%u\n", C_Fg_r, C_Fg_g, C_Fg_b);
                fprintf(cfgf, "custom_bg = %u,%u,%u\n", C_Bg_r, C_Bg_g, C_Bg_b);
                fprintf(cfgf, "\n");
                
                fclose(cfgf);
            }
        }
        
};

static void MenuCB(Fl_Widget* window_p, void *userdata) 
{
    Fl_Help_Dialog hd;
    long choice = (long)userdata;
    Flit_Frame* mainwnd_p = (Flit_Frame *)window_p;
    
    switch (choice) {
        case MI_ABOUT:
            fl_message(About_text, APP_VER, argp_program_bug_address);
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
            mainwnd_p->do_sound_adj(0); // Toggle sound on/off
            break;
        case MI_SOUND_UP:
            mainwnd_p->do_sound_adj(+1);
            break;
        case MI_SOUND_DOWN:
            mainwnd_p->do_sound_adj(-1);
            break;
        case MI_HOTKEY_TOGGLE:
            mainwnd_p->menu_hotkey_activation = !(mainwnd_p->menu_hotkey_activation);
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
        case MI_CUSTOM_STYLE:
            mainwnd_p->set_style(CUSTOM_STYLE);
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
    
extern Display *fl_display;
extern Window fl_window;

void make_window_dock()
{
    Atom dock, win_type;
    win_type = XInternAtom(fl_display,"_NET_WM_WINDOW_TYPE",False);
    dock = XInternAtom(fl_display,"_NET_WM_WINDOW_TYPE_DOCK",False);
    XChangeProperty(fl_display, fl_window,  win_type, XA_ATOM, 32,  PropModeReplace,
          (unsigned char *) &dock, 1);
}

void show_window_on_all_desktops()
{
    // Need to test which method really works
    
    //Atom desktop, win_type;
    //win_type = XInternAtom(fl_display,"_NET_WM_WINDOW_TYPE",False);
    //desktop = XInternAtom(fl_display,"_NET_WM_DESKTOP",False);
    //XChangeProperty(fl_display, fl_window,  win_type, XA_ATOM, 32,  PropModeReplace,
    //        (unsigned char *) &desktop, 0xFFFFFFFF);

    XClientMessageEvent xev;
    xev.type = ClientMessage;
    xev.window = fl_window;
    xev.message_type = XInternAtom(fl_display, "_NET_WM_DESKTOP", False); 
    xev.format = 32;

    /* Force on all desktops! */
    xev.data.l[0] = 0xFFFFFFFF;

    XSendEvent(
        fl_display,
    fl_window, 
    False, 
    SubstructureNotifyMask|SubstructureRedirectMask, 
    (XEvent *) &xev);
}

int main(int argc, char** argv)
{
    struct arguments arguments;
    // Set argument defaults
    arguments.on_all_desktops       = false;
    arguments.dock                  = false;
    arguments.multiple_instances    = false;
    arguments.config_file           = NULL;

    // Parse our command line arguments
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    if(!arguments.multiple_instances) {
        if(is_already_running(PROG)) {
            printf("flit is already running - exiting\n");
            return 0;
        }
        create_pidfile(PROG);        
    }

    fl_open_display();

    fl_message_font(fl_font(), 12 * Z);
    Flit_Frame MainWnd("Flit", arguments.config_file);
    MainWnd.configure();
    MainWnd.update();
    MainWnd.show();
    
    // Needed for docking to work
    MainWnd.make_current();

    if(arguments.on_all_desktops) {
        show_window_on_all_desktops();
    }

    if(arguments.dock) {
        make_window_dock();    
    }

	if(saved_volume >= 0)
	{
		MainWnd.set_sound_level(saved_volume);
#ifdef DIAG
		printf("saved_volume=%d\n", saved_volume);
#endif
	}
    //XSetInputFocus(fl_display, PointerRoot, RevertToPointerRoot, CurrentTime);
    while(Running) {
        Fl::wait(5.0);
        MainWnd.update();
    }    
    //MainWnd.save_config();
    
    if(!arguments.multiple_instances) {
        delete_pidfile(PROG);
    }

    return 0; // Fl::run();
}        
