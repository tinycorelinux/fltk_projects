// generated by Fast Light User Interface Designer (fluid) version 1.0110

#include <libintl.h>
#include "appsaudit.h"
// (c) Robert Shingledecker 2009-2011
#include <iostream>
#include <fstream>
#include <string>
#include <FL/fl_message.H>
#include <FL/Fl_File_Chooser.H>
#include <locale.h>
using namespace std;
static string tcedir, target_dir, desktop; 
static string command; 
static string select_extn, select_results; 
static string option_type, report_type, update_type; 
static ifstream ifaberr; 
static string aberr, msg, hilite; 
static int results, locales_set=0; 
static string copy2fsList, copy2fsFlag, onbootName, onbootList, onbootTitle; 

void cursor_normal() {
  window->cursor(FL_CURSOR_DEFAULT);
Fl::flush();
}

void cursor_wait() {
  window->cursor(FL_CURSOR_WAIT);
Fl::flush();
}

void menu_activate() {
  menuDepends->activate();
menuInstall->activate();
menuUpdates->activate();
menuMd5s->activate();
menuOnBoot->activate();
menuOnDemand->activate();
menuBar->redraw();
}

void menu_deactivate() {
  menuDepends->deactivate();
menuInstall->deactivate();
menuUpdates->deactivate();
menuMd5s->deactivate();
menuOnBoot->deactivate();
menuOnDemand->deactivate();
menuBar->redraw();
}

static char * mygettext(const char *msgid) {
  if (!locales_set) {

setlocale(LC_ALL, "");
bindtextdomain("tinycore","/usr/local/share/locale");
textdomain("tinycore");

locales_set=1;

}

return gettext(msgid);
}

void depends_callback(Fl_Widget *, void* userdata) {
  report_type = (const char*) userdata;
option_type = "";
menu_deactivate();
menuDepends->activate();

if (userdata == "updatedeps")
{
   cursor_wait();
   command = "/usr/bin/tce-audit updatedeps " + target_dir +"/";
   system(command.c_str());
   cursor_normal();
} else if (userdata == "builddb")
{
   cursor_wait();
   command = "/usr/bin/tce-audit builddb " + target_dir +"/";
   system(command.c_str());
   string listfile = target_dir + "/tce.lst";
   brw_extn->load(listfile.c_str());
   brw_extn->remove(brw_extn->size());
   brw_results->load("/tmp/audit_results.txt");
   brw_results->remove(brw_results->size());
   menu_nodepends->activate();
   menu_notrequired->activate();
   menu_auditall->activate();
   menu_marked->activate();
   menu_clearlst->activate();
// box_extn->label("Select");
   box_results->label("Results");
   cursor_normal();
} else if (userdata == "dependson" or userdata == "requiredby" or userdata == "audit") 
{
   report_type = (const char*) userdata;
   string loadit = "tce-audit " + report_type + " " + target_dir + "/" + select_extn;
   int results = system(loadit.c_str());
   if (results == 0 )
   {
      brw_results->load("/tmp/audit_results.txt");
      brw_results->remove(brw_results->size());
   } else {
      fl_message("error detected!");
   }
} else if (userdata == "auditall" or userdata == "nodepends" or userdata == "notrequired")
{
   box_results->label("Results");
   menu_delete->deactivate();
   menu_dependson->deactivate();
   menu_requiredby->deactivate();
   menu_missing->deactivate();
   command = "tce-audit " + report_type + " " + target_dir + "/";
   int results = system(command.c_str());
   if (results == 0 )
   {
      brw_results->load("/tmp/audit_results.txt");
      brw_results->remove(brw_results->size());
   } else {
      ifstream ifaberr("/tmp/aberr");
      msg = "Error: ";
      getline(ifaberr,aberr);
      while (ifaberr) 
      {
         msg = msg + aberr + "\n";
         getline(ifaberr,aberr);
      }
      ifaberr.close();
      fl_message(msg.c_str());
   }
} else if (userdata == "delete")
{
   report_type = (const char*) userdata;
   command = "tce-audit " + report_type + " " + target_dir + "/" + select_extn;
   int results = system(command.c_str());
   if (results == 0 )
   {
      brw_results->load("/tmp/audit_results.txt");
      brw_results->remove(brw_results->size());
   } else {
      fl_message("error detected!");
   }
} else if (userdata == "display_marked")
{
     box_results->label("Results");
     brw_results->load("/tmp/audit_marked.lst");
     brw_results->remove(brw_results->size());
} else if (userdata == "clearlst")
{
     report_type = (const char*) userdata;
     command = "tce-audit " + report_type + " " + target_dir + "/" + select_extn;
     int results = system(command.c_str());
     if (results == 0 )
     { 
        brw_results->clear();
        box_results->label("Marked for deletion cleared");
     }
} else if (userdata == "exit_depends")
{
    menu_activate();
    brw_extn->clear();
    brw_results->clear();
    box_results->label("Results");
}
}

void options_callback(Fl_Widget *, void* userdata) {
  option_type = (const char*) userdata;
report_type = "";

menu_deactivate();
menuInstall->activate();

if (userdata == "default")
{
   command = "ls "+ copy2fsFlag + " >/dev/null 2>&1";
   int results = system(command.c_str());
   if (results == 0)
   {
     command = "rm -f "+ copy2fsFlag;
     system(command.c_str());
     box_results->label("copy2fs.flg removed.");
   } else
   {  
     command = "touch " + tcedir + "/copy2fs.flg";
     system(command.c_str());
     box_results->label("copy2fs.flg set");
   }
} else if (userdata == "select") 
{
// box_extn->label(target_dir.c_str());
   command = "ls " + target_dir + "|grep -E .tcz$ > tce.lst";
   system(command.c_str());
   brw_extn->load("tce.lst");
   brw_extn->remove(brw_extn->size());
   brw_results->load(copy2fsList.c_str());
   brw_results->remove(brw_results->size());
   box_results->label("Current copy2fs.lst");
} else if (userdata == "exit_copy")
{
    menu_activate();
    option_type = "";
    box_results->label("Results");
    brw_extn->clear();
    brw_results->clear();
}
}

void updates_callback(Fl_Widget *, void* userdata) {
  report_type = (const char*) userdata;

if (report_type == "select_mirror")
{
   system("cat /opt/localmirrors /usr/local/share/mirrors > /tmp/mirrors 2>/dev/null");
   brw_extn->load("/tmp/mirrors");
   if ( brw_extn->size() == 1)
      fl_message("Must load mirrors.tcz extension or have /opt/localmirrors in order to use this feature.");
   else {
      brw_extn->remove(brw_extn->size());
      box_extn->activate();
   }
} else if (report_type == "update") 
{
   menu_deactivate();
   menuUpdates->activate();
   brw_extn->hide();
   grp_updates->show();
   grp_updates->activate();
   brw_multi->clear();
   brw_results->clear();
   Fl::flush();
   cursor_wait();
   command = "tce-update list " + target_dir + " > /tmp/apps_upd.lst";
   system(command.c_str());
   box_extn->label(target_dir.c_str());
   brw_multi->load("/tmp/apps_upd.lst");
   brw_multi->remove(brw_multi->size());
   if ( brw_multi->size() >= 1 )
      btn_multi->activate();
    else
       brw_results->add("System is Current. No updates required.");
   cursor_normal();
} else if (report_type == "exit_updates")
{
    menu_activate();
    grp_updates->hide();
    brw_extn->show();
    report_type = "";
    box_results->label("Results");
    brw_extn->clear();
    brw_results->clear();
}
}

void md5s_callback(Fl_Widget *, void* userdata) {
  report_type = (const char*) userdata;

if (report_type == "md5s") 
{
   menu_deactivate();
   menuMd5s->activate();
   brw_extn->hide();
   grp_updates->show();
   grp_updates->activate();
   brw_multi->clear();
   brw_results->clear();
   box_results->label("Results");
   Fl::flush();
   cursor_wait();
   command = "cd " + target_dir + " && ls *.md5.txt > /tmp/apps_upd.lst";
   system(command.c_str());
   box_extn->label(target_dir.c_str());
   brw_multi->load("/tmp/apps_upd.lst");
   brw_multi->remove(brw_multi->size());
   cursor_normal();
   btn_multi->activate();
} else if (report_type == "exit_md5s")
{
    menu_activate();
    grp_updates->hide();
    brw_extn->show();
    report_type = "";
    box_results->label("Results");
    brw_extn->clear();
    brw_results->clear();
}
}

void onboot_callback(Fl_Widget *, void* userdata) {
  if (userdata == "onboot" )
{
  menu_deactivate();
  menuOnBoot->activate();
  target_dir = tcedir + "/optional/";
  report_type = "onboot";
  brw_extn->clear();
  cursor_wait();
  
  command = "ondemand -l ";
  results = system(command.c_str());
  if (results == 0 ) {
     brw_extn->load("/tmp/ondemand.tmp");
     brw_extn->remove(brw_extn->size());
     unlink("/tmp/ondemand.tmp");
  }
  
  onbootTitle = "On Boot Items (" + onbootName +")";
  box_results->label(onbootTitle.c_str());
  brw_results->load(onbootList.c_str());
  brw_results->remove(brw_results->size());
  
  cursor_normal();
}
 
if (userdata == "exit_onboot")
{
  menu_activate();
  report_type.empty();
  box_results->label("Results");
  brw_extn->clear();
  brw_results->clear();
}
}

void ondemand_callback(Fl_Widget *, void* userdata) {
  if (userdata == "ondemand" )
{
  menu_deactivate();
  menuOnDemand->activate();
  report_type = "ondemand";
  brw_extn->clear();
  cursor_wait();
  command = "ondemand -l";
  system(command.c_str());
  
//box_extn->label("Select for OnDemand");
  brw_extn->load("/tmp/ondemand.tmp");
  brw_extn->remove(brw_extn->size());
  
  brw_results->clear();
  box_results->label("Current OnDemand Items");
  command = "ls -1 "+ tcedir + "/ondemand 2>/dev/null | grep -v \".img$\" | sort -f > /tmp/ondemand.tmp";
  results = system(command.c_str());
  if (results == 0 ) {
    brw_results->load("/tmp/ondemand.tmp");
    brw_results->remove(brw_results->size());
    unlink("/tmp/ondemand.tmp");
  }  
  cursor_normal();
}
 
if (userdata == "exit_ondemand")
{
  menu_activate();
  report_type.empty();
//box_extn->label(target_dir.c_str());
  box_results->label("Results");
  brw_extn->clear();
  brw_results->clear();
} 
 
if (userdata == "quit") 
  exit(0);
}

void brw_extn_callback(Fl_Widget *, void *) {
  if (brw_extn->value())
{
   select_extn = brw_extn->text(brw_extn->value());
   bool not_duplicate = true;
   for ( int x=1; x <= brw_results->size(); x++ ) {
     if (!strcmp(brw_results->text(x),select_extn.c_str())) {
       not_duplicate = false;
     }
   }   
   if ( report_type.length() > 0 )
   {
     if ( not_duplicate ) { box_results->label(select_extn.c_str()); }
     menu_dependson->activate();
     menu_requiredby->activate();
     menu_missing->activate();
     menu_delete->activate();
     if ( not_duplicate ) { brw_results->load(""); }
   } 
   if ( option_type == "select" && not_duplicate )
   {
     command = "echo " + select_extn + " >> " + copy2fsList;
     system(command.c_str());
     brw_results->load(copy2fsList.c_str());
     brw_results->remove(brw_results->size());
   }
   
   if ( report_type == "select_mirror" )
   {
     string mirror = select_extn;
     box_results->label(mirror.c_str());
     ofstream fout("/opt/tcemirror", ios::out|ios::out);
     if (! fout.is_open())
     {
       cerr << "Can't open /opt/tcemirror for output!" << endl;
       exit(EXIT_FAILURE);
     }
     fout << mirror << endl;
     fout.close();      
   }
   
   if ( report_type == "onboot" && not_duplicate )
   {
     command = "echo " + select_extn + " >> " + onbootList;
     system(command.c_str());
     brw_extn->remove(brw_extn->value());
     box_results->label(onbootTitle.c_str());
     brw_results->load(onbootList.c_str());
     brw_results->remove(brw_results->size());
   }
      
   if ( report_type == "ondemand" )
   {
     cursor_wait();
     box_results->label("Current OnDemand Items");
     command = "ondemand " + select_extn;
     brw_results->load("");
     results = system(command.c_str());
     if ( results == 0 ) 
     {
       command = "ls -1 " + tcedir + "/ondemand | grep -v \".img$\" | sort -f > /tmp/ondemand.tmp";
       results = system(command.c_str());
       if (results == 0 ) {
         brw_extn->remove(brw_extn->value());
         brw_results->load("/tmp/ondemand.tmp");
         brw_results->remove(brw_results->size());
       }
     } else { 
         brw_results->load("/tmp/ondemand.tmp");
         brw_results->remove(brw_results->size());
     }    
     cursor_normal();
   }
}
}

void brw_multi_callback(Fl_Widget *, void *) {
  cursor_wait();
brw_results->clear();
for (int t=0; t<=brw_multi->size(); t++) {
   if (brw_multi->selected(t) ) {
      select_extn = brw_multi->text(t);
      string select_extn_file = select_extn + ".info";
      command = "tce-fetch.sh " + select_extn_file;
      int results = system(command.c_str());
      if (results == 0)
         brw_results->load(select_extn_file.c_str());
      continue;
   }
}   
cursor_normal();
}

void btn_multi_callback(Fl_Widget *, void *) {
  cursor_wait();
brw_results->clear();
for ( int t=0; t<=brw_multi->size(); t++ )
{
   if ( brw_multi->selected(t) )
   {
      select_extn = brw_multi->text(t);
      if ( report_type == "md5s" )
      {
         command = "cd " + target_dir +"/ && md5sum -c " + select_extn;
         results = system(command.c_str());
         if ( results == 0 ) {
            msg = " OK";
            hilite = "";
         } else {
            msg = " FAILED";
            hilite = "@B17";
         }   
             
         brw_results->add((hilite + select_extn + msg).c_str());
         Fl::flush();      
      
      } else {
         box_results->label(("Fetching " + select_extn).c_str());
         box_results->redraw();
         Fl::flush();

         command = "tce-update update " + target_dir +"/" + select_extn + ".md5.txt >/tmp/apps_upd.lst";
         cout << command << endl;
         results = system(command.c_str());
         if ( results == 0 ) 
            msg = " OK";
         else
            msg = " FAILED";

         brw_results->add((select_extn + msg).c_str());
         Fl::flush();      
         
      }
   }
}
brw_multi->deselect();
if (report_type == "update" )
   box_results->label("Updates complete. Reboot to effect.");
cursor_normal();
}

void brw_results_callback(Fl_Widget *, void *) {
  if (brw_results->value())
{
   select_results = brw_results->text(brw_results->value());
   if ( option_type.length() > 0 )
   {
     command = "sed -i '/" + select_results + "/d' " + copy2fsList;
     system(command.c_str());
     brw_results->load(copy2fsList.c_str());
     brw_results->remove(brw_results->size());
   }
   if (report_type == "delete" or report_type == "display_marked")
   {
     string target = select_results.substr(select_results.find_last_of("/")+1);
     command = "sed -i '/" + target + "/d' /tmp/audit_marked.lst";
     system(command.c_str());
     brw_results->load("/tmp/audit_marked.lst");
     brw_results->remove(brw_results->size());
   }
   if (report_type == "onboot")
   {
     command = "sed -i '/" + select_results + "/d' " + onbootList;
     system(command.c_str());
     target_dir = tcedir + "/optional/";
     report_type = "onboot";
     brw_extn->clear();
     cursor_wait();
  
     command = "ondemand -l ";
     results = system(command.c_str());
     if (results == 0 ) {
//      box_extn->label(target_dir.c_str());
        brw_extn->load("/tmp/ondemand.tmp");
        brw_extn->remove(brw_extn->size());
     }
  
     box_results->label(onbootTitle.c_str());
     brw_results->load(onbootList.c_str());
     brw_results->remove(brw_results->size());
  
     cursor_normal();
   }  

   if (report_type == "ondemand")
   {
     command = "ondemand -r " + select_results;
     system(command.c_str());   
     command = "ondemand -l ";
     results = system(command.c_str());
     if (results == 0 ) {
       brw_extn->load("/tmp/ondemand.tmp");
       brw_extn->remove(brw_extn->size());
     }  
     command = "ls -1 " + tcedir + "/ondemand | grep -v .img$ | sort -f > /tmp/ondemand.tmp";
     results = system(command.c_str());
     if (results == 0 ) {
       brw_results->load("/tmp/ondemand.tmp");
       brw_results->remove(brw_results->size());
     }  
     unlink("/tmp/ondemand.tmp");
   }  
}
}

Fl_Double_Window *window=(Fl_Double_Window *)0;

static void cb_window(Fl_Double_Window*, void*) {
  system("tce-remove");
exit(0);
}

Fl_Menu_Bar *menuBar=(Fl_Menu_Bar *)0;

Fl_Menu_Item menu_menuBar[] = {
 {mygettext("Dependencies"), 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Update .dep files."), 0,  (Fl_Callback*)depends_callback, (void*)("updatedeps"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Build Reporting Database"), 0,  (Fl_Callback*)depends_callback, (void*)("builddb"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("List Dependencies"), 0,  (Fl_Callback*)depends_callback, (void*)("dependson"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("List Required By"), 0,  (Fl_Callback*)depends_callback, (void*)("requiredby"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("List Missing Dependencies"), 0,  (Fl_Callback*)depends_callback, (void*)("audit"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Display All with No Dependencies"), 0,  (Fl_Callback*)depends_callback, (void*)("nodepends"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Display All Not Depended On"), 0,  (Fl_Callback*)depends_callback, (void*)("notrequired"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Find Missing Dependencies"), 0,  (Fl_Callback*)depends_callback, (void*)("auditall"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Mark for Deletion"), 0,  (Fl_Callback*)depends_callback, (void*)("delete"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Display Marked for Deletion"), 0,  (Fl_Callback*)depends_callback, (void*)("display_marked"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Clear Marked for Deletion"), 0,  (Fl_Callback*)depends_callback, (void*)("clearlst"), 1, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Exit Dependencies Mode"), 0,  (Fl_Callback*)depends_callback, (void*)("exit_depends"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {mygettext("Install Options"), 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Toggle Default Copy Install"), 0,  (Fl_Callback*)options_callback, (void*)("default"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Selective Copy Install"), 0,  (Fl_Callback*)options_callback, (void*)("select"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Exit Install Options"), 0,  (Fl_Callback*)options_callback, (void*)("exit_copy"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {mygettext("Updates"), 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Select Mirror"), 0,  (Fl_Callback*)updates_callback, (void*)("select_mirror"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Check for Updates"), 0,  (Fl_Callback*)updates_callback, (void*)("update"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Exit Update Mode"), 0,  (Fl_Callback*)updates_callback, (void*)("exit_updates"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {mygettext("Md5Check"), 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Check Md5sums"), 0,  (Fl_Callback*)md5s_callback, (void*)("md5s"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Exit Md5 Check Mode"), 0,  (Fl_Callback*)md5s_callback, (void*)("exit_md5s"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {mygettext("OnBoot"), 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Maintenance"), 0,  (Fl_Callback*)onboot_callback, (void*)("onboot"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Exit OnBoot"), 0,  (Fl_Callback*)onboot_callback, (void*)("exit_onboot"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {mygettext("OnDemand"), 0,  0, 0, 64, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Maintenance"), 0,  (Fl_Callback*)ondemand_callback, (void*)("ondemand"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {mygettext("Exit OnDemand"), 0,  (Fl_Callback*)ondemand_callback, (void*)("exit_ondemand"), 0, FL_NORMAL_LABEL, 0, 14, 0},
 {0,0,0,0,0,0,0,0,0},
 {0,0,0,0,0,0,0,0,0}
};

Fl_Box *box_extn=(Fl_Box *)0;

Fl_Box *box_results=(Fl_Box *)0;

Fl_Browser *brw_extn=(Fl_Browser *)0;

Fl_Group *grp_updates=(Fl_Group *)0;

Fl_Browser *brw_multi=(Fl_Browser *)0;

Fl_Button *btn_multi=(Fl_Button *)0;

Fl_Browser *brw_results=(Fl_Browser *)0;

int main(int argc, char **argv) {
  { window = new Fl_Double_Window(675, 375, mygettext("AppsAudit"));
    window->callback((Fl_Callback*)cb_window);
    { menuBar = new Fl_Menu_Bar(0, 0, 685, 20);
      menuBar->menu(menu_menuBar);
    } // Fl_Menu_Bar* menuBar
    { box_extn = new Fl_Box(0, 24, 200, 16, mygettext("Select"));
      box_extn->labelfont(1);
    } // Fl_Box* box_extn
    { box_results = new Fl_Box(225, 24, 430, 16, mygettext("Results"));
      box_results->labelfont(1);
    } // Fl_Box* box_results
    { brw_extn = new Fl_Browser(0, 45, 200, 325);
      brw_extn->type(1);
      brw_extn->textfont(4);
      brw_extn->callback((Fl_Callback*)brw_extn_callback);
    } // Fl_Browser* brw_extn
    { grp_updates = new Fl_Group(0, 45, 200, 325);
      grp_updates->hide();
      grp_updates->deactivate();
      { brw_multi = new Fl_Browser(0, 45, 200, 300);
        brw_multi->type(3);
        brw_multi->textfont(4);
        brw_multi->callback((Fl_Callback*)brw_multi_callback);
      } // Fl_Browser* brw_multi
      { btn_multi = new Fl_Button(0, 350, 200, 20, mygettext("Process Selected Item(s)"));
        btn_multi->callback((Fl_Callback*)btn_multi_callback);
        btn_multi->deactivate();
      } // Fl_Button* btn_multi
      grp_updates->end();
    } // Fl_Group* grp_updates
    { brw_results = new Fl_Browser(225, 45, 430, 325);
      brw_results->type(1);
      brw_results->textfont(4);
      brw_results->callback((Fl_Callback*)brw_results_callback);
    } // Fl_Browser* brw_results
    window->end();
    window->resizable(window);
  } // Fl_Double_Window* window
  ifstream desktop_file("/etc/sysconfig/desktop");
getline(desktop_file,desktop);
desktop_file.close();

ifstream tcedir_file("/opt/.tce_dir");
getline(tcedir_file,tcedir);
tcedir_file.close();

target_dir = tcedir + "/optional";
window->label(target_dir.c_str());

copy2fsList = tcedir + "/copy2fs.lst";
copy2fsFlag = tcedir + "/copy2fs.flg";

string cmdline, target_boot_option;
ifstream proc_cmdline("/proc/cmdline");
getline(proc_cmdline, cmdline);
proc_cmdline.close();
target_boot_option = "lst=";
int sloc = cmdline.find(target_boot_option);
if ( sloc == string::npos ) {
   onbootName = "onboot.lst";
} else {
   int eloc = cmdline.find(" ",sloc);
   int work = eloc - (sloc + target_boot_option.length());
   onbootName = cmdline.substr(sloc+target_boot_option.length(),work);
}

onbootList = tcedir + "/" + onbootName;

option_type.empty();
report_type.empty();

command = "ls " + target_dir + "/tce.db >/dev/null 2>&1";

int results = system(command.c_str());
if (results == 0)
{
  report_type = "updatedeps";
}
  window->show(argc, argv);
  return Fl::run();
}
