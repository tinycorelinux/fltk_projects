// generated by Fast Light User Interface Designer (fluid) version 1.0110

#include <libintl.h>
#include "wbar_exclude.h"
// (c) Robert Shingledecker 2010
#include <iostream>
#include <fstream>
#include <string>
#include <FL/fl_message.H>
#include <FL/Fl_File_Chooser.H>
#include <locale.h>
using namespace std;
static string tcedir; 
static string command; 
static string select_onboot, select_wbar; 
static int results; 
static string onbootList, wbarList; 

void brw_onboot_callback(Fl_Widget *, void *) {
  if (brw_onboot->value())
{
   select_onboot = brw_onboot->text(brw_onboot->value());
   bool not_duplicate = true;
   for ( int x=1; x <= brw_wbar->size(); x++ ) {
     if (!strcmp(brw_wbar->text(x),select_onboot.c_str())) {
       not_duplicate = false;
     }
   }   
   if ( not_duplicate )
   {
     command = "echo " + select_onboot + " >> " + wbarList;
     system(command.c_str());
     brw_wbar->load(wbarList.c_str());
   }
}
}

void brw_wbar_callback(Fl_Widget *, void *) {
  if (brw_wbar->value())
{
   select_wbar = brw_wbar->text(brw_wbar->value());
   command = "sed -i '/" + select_wbar + "/d' " + wbarList;
   system(command.c_str());
   brw_wbar->load(wbarList.c_str());
}
}

Fl_Double_Window *window=(Fl_Double_Window *)0;

Fl_Browser *brw_onboot=(Fl_Browser *)0;

Fl_Browser *brw_wbar=(Fl_Browser *)0;

int main(int argc, char **argv) {
  setlocale(LC_ALL, "");
bindtextdomain("tinycore","/usr/local/share/locale");
textdomain("tinycore");
  { window = new Fl_Double_Window(425, 350, gettext("eXclude Wbar Icons"));
    { brw_onboot = new Fl_Browser(5, 20, 200, 325, gettext("Onboot Items"));
      brw_onboot->type(1);
      brw_onboot->textfont(4);
      brw_onboot->callback((Fl_Callback*)brw_onboot_callback);
      brw_onboot->align(FL_ALIGN_TOP_LEFT);
    } // Fl_Browser* brw_onboot
    { brw_wbar = new Fl_Browser(215, 20, 200, 325, gettext("eXclude Wbar Items"));
      brw_wbar->type(1);
      brw_wbar->textfont(4);
      brw_wbar->callback((Fl_Callback*)brw_wbar_callback);
      brw_wbar->align(FL_ALIGN_TOP_LEFT);
    } // Fl_Browser* brw_wbar
    window->end();
    window->resizable(window);
  } // Fl_Double_Window* window
  ifstream tcedir_file("/opt/.tce_dir");
getline(tcedir_file,tcedir);
tcedir_file.close();
onbootList = tcedir + "/onboot.lst";
wbarList = tcedir + "/xwbar.lst";
brw_onboot->load(onbootList.c_str());
brw_wbar->load(wbarList.c_str());
  window->show(argc, argv);
  return Fl::run();
}
