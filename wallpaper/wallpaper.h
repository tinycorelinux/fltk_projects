// generated by Fast Light User Interface Designer (fluid) version 1.0110

#ifndef wallpaper_h
#define wallpaper_h
#include <FL/Fl.H>
char * mygettext(const char *msgid);
void imageBrowserCallback(Fl_Widget*, void*);
void btnCallback(Fl_Widget*, void* userdata);
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Color_Chooser.H>
extern Fl_Double_Window *GradientWindow;
extern Fl_Color_Chooser *colorChooserGradient1;
extern Fl_Color_Chooser *colorChooserGradient2;
#include <FL/Fl_Round_Button.H>
extern Fl_Round_Button *btnVertical;
extern Fl_Round_Button *btnHorizontal;
extern Fl_Round_Button *btnDiagonal;
#include <FL/Fl_Box.H>
extern Fl_Box *boxColor1;
extern Fl_Box *boxColor2;
#include <FL/Fl_Button.H>
#include <FL/Fl_Return_Button.H>
extern Fl_Double_Window *colorChooserWindow;
extern Fl_Color_Chooser *colorChooser;
extern Fl_Box *boxColor;
extern Fl_Double_Window *Wallpaper;
#include <FL/Fl_File_Browser.H>
extern Fl_File_Browser *imageBrowser;
extern Fl_Round_Button *btnFull;
extern Fl_Round_Button *btnTile;
extern Fl_Round_Button *btnCenter;
extern Fl_Round_Button *btnFill;
extern Fl_Button *colorBtn;
extern Fl_Button *gradientBtn;
#include <FL/Fl_Check_Button.H>
extern Fl_Check_Button *logoBtn;
extern Fl_Button *installBtn;
extern Fl_Button *doneBtn;
#endif
