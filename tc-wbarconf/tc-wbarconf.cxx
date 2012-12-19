// generated by Fast Light User Interface Designer (fluid) version 1.0110

#include <libintl.h>
#include "tc-wbarconf.h"
// (c) Robert Shingledecker 2010-2012
// Zoom, Icon Size, Icon - Brian Smith
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <FL/fl_message.H>
#include <FL/Fl_File_Chooser.H>
#include <locale.h>
#include <unistd.h>
#include <string.h>
using namespace std;
static string home, tcedir, command, selected; 
static int chosen,results; 
static string widgetArray[4]; 
static string onbootList, wbarXlist, options; 
static string wbarIcons = "/usr/local/tce.icons"; 

void genConf() {
  options = "c: wbar --bpress --pos ";
int pos = posChoice->value();
switch(pos)
{
   case 0  : options += "top";
             break;;
   case 1  : options += "bottom";
             break;;
   case 2  : options += "left --vbar";
             break;;
   case 3  : options += "right --vbar";
             break;;
   case 4  : options += "top-left";
             break;;
   case 5  : options += "top-right";
             break;;
   case 6  : options += "bot-left";
             break;;
   case 7  : options += "bot-right";
             break;;
   case 8  : options += "top-left --vbar";
             break;;
   case 9  : options += "bot-left --vbar";
             break;;
   case 10 : options += "top-right --vbar";
             break;;
   case 11 : options += "bot-right --vbar";
             break;;
   default : options += "bottom";
             break;;
}
double zoomf_value = zoomChoice->value();
if (zoomf_value == 1) {zoomf_value = 1.00001;}
string zoomf;
ostringstream stream;
stream << (double)zoomf_value;
zoomf = stream.str();
options += " --zoomf " + zoomf;

stream.str("");
string isize;
stream << (double)isizeChoice->value();
isize = stream.str();
options += " --isize " + isize;
txtFromGUI->value(options.c_str());
options += " ";
}

void brwWbarCB(Fl_Widget *, void *) {
  if (brwWbar->value())
{
   chosen = brwWbar->value();
   selected = brwWbar->text(chosen);
   command = "wbar_mv_icon " + selected.substr(0,selected.length()-1) + " " + wbarIcons + " " + wbarXlist;
   system(command.c_str());
   brwXlist->add(selected.c_str());
   brwWbar->remove(chosen);
}
}

void brwXlistCB(Fl_Widget *, void *) {
  if (brwXlist->value())
{
   chosen = brwXlist->value();
   selected = brwXlist->text(chosen);
   command = "wbar_mv_icon " + selected.substr(0,selected.length()-1) + " " + wbarXlist + " " + wbarIcons;
   system(command.c_str());
   brwXlist->remove(chosen);
   brwWbar->add(selected.c_str());
}
}

void btnApplyCB(Fl_Widget *, void *) {
  genConf();
options += txtOptions->value();

string fname = home + "/.wbarconf";
ofstream fout(fname.c_str(), ios::out|ios::out);
if (! fout.is_open())
{
   cerr << "Can't open .wbarconf for output." << endl;
   exit(EXIT_FAILURE);
}
fout << options << endl;
fout.close();
Fl::flush();
command = "wbar.sh";
system(command.c_str());
}

Fl_Double_Window *window=(Fl_Double_Window *)0;

Fl_Group *grpGUI=(Fl_Group *)0;

Fl_Browser *brwWbar=(Fl_Browser *)0;

Fl_Browser *brwXlist=(Fl_Browser *)0;

Fl_Choice *posChoice=(Fl_Choice *)0;

Fl_Menu_Item menu_posChoice[] = {
 {gettext("Top"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Bottom"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Left Vertical"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Right Vertical"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Top Left"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Top Right"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Bottom Left"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Bottom Right"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Left Top Vertical"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Left Bottom Vertical"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Right Top Vertical"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {gettext("Right Bottom Vertical"), 0,  0, 0, 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Slider *isizeChoice=(Fl_Slider *)0;

Fl_Slider *zoomChoice=(Fl_Slider *)0;

static void cb_More(Fl_Button*, void*) {
  grpGUI->hide();
grpOptions->show();
genConf();
txtOptions->take_focus();
}

Fl_Group *grpOptions=(Fl_Group *)0;

Fl_Output *txtFromGUI=(Fl_Output *)0;

Fl_Browser *brwOptions=(Fl_Browser *)0;

Fl_Input *txtOptions=(Fl_Input *)0;

static void cb_Back(Fl_Button*, void*) {
  grpOptions->hide();
grpGUI->show();
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
bindtextdomain("tinycore","/usr/local/share/locale");
textdomain("tinycore");
  { window = new Fl_Double_Window(435, 333, gettext("tc-wbarconf"));
    { grpGUI = new Fl_Group(9, 32, 421, 293);
      { brwWbar = new Fl_Browser(9, 33, 200, 225, gettext("Wbar Icons"));
        brwWbar->type(1);
        brwWbar->textfont(4);
        brwWbar->callback((Fl_Callback*)brwWbarCB);
        brwWbar->align(FL_ALIGN_TOP);
      } // Fl_Browser* brwWbar
      { brwXlist = new Fl_Browser(219, 33, 210, 225, gettext("eXclude Icons"));
        brwXlist->type(1);
        brwXlist->textfont(4);
        brwXlist->callback((Fl_Callback*)brwXlistCB);
        brwXlist->align(FL_ALIGN_TOP);
      } // Fl_Browser* brwXlist
      { posChoice = new Fl_Choice(66, 273, 165, 20, gettext("Position"));
        posChoice->down_box(FL_BORDER_BOX);
        posChoice->menu(menu_posChoice);
        posChoice->value(1);
      } // Fl_Choice* posChoice
      { isizeChoice = new Fl_Slider(319, 273, 105, 20, gettext("Icon Size"));
        isizeChoice->type(1);
        isizeChoice->minimum(10);
        isizeChoice->maximum(80);
        isizeChoice->step(1);
        isizeChoice->value(32);
        isizeChoice->align(FL_ALIGN_LEFT);
      } // Fl_Slider* isizeChoice
      { zoomChoice = new Fl_Slider(115, 303, 115, 20, gettext("Zoom"));
        zoomChoice->type(1);
        zoomChoice->minimum(1);
        zoomChoice->maximum(3);
        zoomChoice->step(0.1);
        zoomChoice->value(2);
        zoomChoice->align(FL_ALIGN_LEFT);
      } // Fl_Slider* zoomChoice
      { Fl_Button* o = new Fl_Button(254, 303, 80, 20, gettext("More"));
        o->callback((Fl_Callback*)cb_More);
      } // Fl_Button* o
      { Fl_Button* o = new Fl_Button(343, 303, 80, 20, gettext("Apply"));
        o->callback((Fl_Callback*)btnApplyCB);
      } // Fl_Button* o
      grpGUI->end();
    } // Fl_Group* grpGUI
    { grpOptions = new Fl_Group(8, 9, 426, 316);
      grpOptions->hide();
      { txtFromGUI = new Fl_Output(11, 9, 415, 20);
        txtFromGUI->color((Fl_Color)29);
      } // Fl_Output* txtFromGUI
      { brwOptions = new Fl_Browser(8, 39, 420, 215, gettext("Optionally Enter Additional Unique Options from above:"));
        brwOptions->textfont(4);
      } // Fl_Browser* brwOptions
      { txtOptions = new Fl_Input(10, 271, 414, 24);
      } // Fl_Input* txtOptions
      { Fl_Button* o = new Fl_Button(222, 303, 80, 20, gettext("Back"));
        o->callback((Fl_Callback*)cb_Back);
      } // Fl_Button* o
      { Fl_Button* o = new Fl_Button(324, 303, 80, 20, gettext("Apply"));
        o->callback((Fl_Callback*)btnApplyCB);
      } // Fl_Button* o
      grpOptions->end();
    } // Fl_Group* grpOptions
    window->end();
    window->resizable(window);
  } // Fl_Double_Window* window
  home = getenv("HOME");
FILE *pipe;
char *mbuf;

char buffer[1024];
int length;
length = readlink("/etc/sysconfig/tcedir", buffer, sizeof(buffer));
buffer[length]='\0';
tcedir = strdup(buffer);

wbarXlist = tcedir + "/xwbar.lst";

command = "awk '/t: /{if (NR>4)print $2}' < " + wbarIcons;
pipe = popen(command.c_str(),"r");
mbuf = (char *) calloc(PATH_MAX,sizeof(char));

if (pipe) {
  brwWbar->clear();
  while (fgets(mbuf,PATH_MAX,pipe)) {
     string line(mbuf);
     brwWbar->add(line.c_str());
//     Fl::flush();
   }
   pclose(pipe);
   free(mbuf);
}

command = "awk '/t: /{print $2}' < " + wbarXlist + " 2>/dev/null";
pipe = popen(command.c_str(),"r");
mbuf = (char *) calloc(PATH_MAX,sizeof(char));

if (pipe) {
  brwXlist->clear();
  while (fgets(mbuf,PATH_MAX,pipe)) {
     string line(mbuf);
     brwXlist->add(line.c_str());
   }
   pclose(pipe);
   free(mbuf);
}
// Fl::flush();


command = "wbar --help 2>/dev/null | tail -23";
pipe = popen(command.c_str(),"r");
mbuf = (char *) calloc(PATH_MAX,sizeof(char));

if (pipe) {
  brwOptions->clear();
  while (fgets(mbuf,PATH_MAX,pipe)) {
     string line(mbuf);
     brwOptions->add(line.c_str());
//     Fl::flush();
   }
   pclose(pipe);
   free(mbuf);
}

// Parse .wbarconf to reload basic widgets
int i=0;
command = "wbar_parseConf";
pipe = popen(command.c_str(),"r");
mbuf = (char *) calloc(PATH_MAX,sizeof(char));

if (pipe) {
  while (fgets(mbuf,PATH_MAX,pipe)) {
     string line(mbuf);
     widgetArray[i++] = line;
   }
   pclose(pipe);
   free(mbuf);
}

i = atof(widgetArray[0].c_str());
posChoice->value(i-1);
   
double zoomf = atof(widgetArray[1].c_str());
if (zoomf == 0) { zoomf = 2.0; }
zoomChoice->value(zoomf);   

double isize = atof(widgetArray[2].c_str());
if (isize == 0) { isize = 32.0; }
isizeChoice->value(isize);

string otherOptions = widgetArray[3].substr(0,widgetArray[3].length()-1);
txtOptions->value(otherOptions.c_str());

grpGUI->show();
Fl::flush();
  window->show(argc, argv);
  return Fl::run();
}
