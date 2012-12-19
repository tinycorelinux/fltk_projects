#include <stdio.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Progress.H>

#include "flbbww.H"


/* quickly hacked from http://seriss.com/people/erco/fltk/#Progress */
char *save, *uri;

void butt_cb(Fl_Widget *butt, void *data)
{
//ML: new code to close app on another button click if download was a success
  static int can_leave = 0;
  int rc;
  if (can_leave) {
	  exit(0);
  }
  // Deactivate the button
  butt->deactivate();                 // prevent button from being pressed again                   
  Fl::check();                        // give fltk some cpu to gray out button

  // Make the progress bar
  Fl_Window *w = (Fl_Window*)data;    // access parent window
  w->begin();                         // add progress bar to it..
  Fl_Progress *progress = new Fl_Progress(10, 10, 200, 30);
  progress->minimum(0);               // set progress bar attribs..
  progress->maximum(1);
  w->end();                           // end of adding to window

  // starting value
  progress->value(0.0f);

  // Computation loop..
  rc = flbbwgetwrap(uri, save, NULL, w, progress);

  // Cleanup
  w->remove(progress);                // remove progress bar from window
  delete(progress);                   // deallocate it
  butt->activate();                   // reactivate button
//ML: change button to "Close" if download was good, also update the window title
  can_leave = 1;
  switch(rc) {
	case BBWW_FULL_SUCCESS:
	  butt->label("Close");
	  w->label("Download complete");
	  break;
	case BBWW_PART_SUCCESS:
	  butt->label("Resume");
	  w->label("Download interrupted");
	  can_leave = 0;
	  break;
	case BBWW_EXISTS_WARNING:
	  butt->label("Close");
	  w->label("File already exists locally");
	  break;
	case BBWW_NOTFOUND_ERROR:
	  butt->label("Quit");
	  w->label("File not found");
	  break;
	case BBWW_NOSZ_ERROR:
	  butt->label("Quit");
	  w->label("No download specified");
	  break;
	case BBWW_INIT_ERROR:
	  butt->label("Quit");
	  w->label("Initialization problem");
	  break;
	case BBWW_CALL_ERROR:
	default:
	  butt->label("Quit");
	  w->label("Unknown error");
	  break;
	  
  }
  w->redraw();                        // tell window to redraw now that progress removed
}

int main(int argc, char *argv[])
{
  char window_label[256];
  if (argc != 3)
  {
    printf("Usage: %s <uri> <output file>\n", argv[0]);
    return 1;
  }

  uri = argv[1];
  save = argv[2];

  Fl_Window flwin(330, 45);
  //ML: "Begin" sounds nicer to me :-)
  Fl_Button butt(220, 10, 100, 25, "Begin");
  butt.callback(butt_cb, &flwin);
  flwin.resizable(flwin);
  sprintf(window_label, "Download %s", basename(uri)); 
  flwin.label(window_label);

  flwin.show();
  return Fl::run();
}

