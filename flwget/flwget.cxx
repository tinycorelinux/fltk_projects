/* flwget.cxx
 *  see flwget.H for more info
 */
/* Some code derived from from http://seriss.com/people/erco/fltk/#Progress */
/* Remainder by ^thehatsrule^ and Michael A. Losh (ML) */
/* Last update: 2009.07.23 by ML */

#include <stdio.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Progress.H>

#include "flwget.h"
// MACROS -------------------------------------------------------------
#define PROGRESS_COLOR FL_BLUE

// GLOBALS ------------------------------------------------------------
char *save, *uri;

/* RS: shoved in some fltk/c++ stuff here probably no need to generalize it */
int flbbwgetwrap(char *uri, const char *save_dir, const char *base_cmd_fmt,
  Fl_Progress *progress)
{
  int childrc;
  int ret = BBWW_CALL_ERROR;
  char *cmd, *save_path, *save_base;
  char default_base_cmd_fmt[] = BBWW_DEFAULT_BASE_CMD_FMT;
  char progress_percent[BBWW_PROGRESS_WIDTH];
  FILE *pfp;
  size_t cmd_len, save_path_len, base_cmd_len;
  int completion;
  char buf[256], c, *p;

  if (! (uri && save_dir))
  {
    return BBWW_SPEC_ERROR;
  }

  if (! base_cmd_fmt)
  {
    base_cmd_fmt = default_base_cmd_fmt;
    base_cmd_len = strlen(base_cmd_fmt) - 2*BBWW_EXPECT_STR_FMTS;  /* %s */
  }

  cmd_len = strlen(uri) + strlen(save_dir) + base_cmd_len;
  cmd = (char *)malloc(cmd_len + 1);

  save_base = basename(uri);  /* seems to work */
  save_path_len = strlen(save_dir) + strlen(save_base) + 1;
  save_path = (char *)malloc(save_path_len + 1);

  if ( cmd && save_path
    && sprintf(cmd, base_cmd_fmt, save_dir, uri) == (int)cmd_len
    && sprintf(save_path, "%s/%s", save_dir, save_base) == (int)save_path_len
    && (pfp = popen(cmd, "r"))
    && (setvbuf(pfp, (char *) NULL, _IONBF, 0) == 0) )
  {
	completion = 0;
	progress->label("0%");
	p = buf;
	while (EOF != (*p = (char)fgetc(pfp))) {
		if (*p == '\r' || *p == '\n' ) {
			if (2 == sscanf(buf, "%*s %d%c", &completion, &c) && c == '%') {
				//printf("for line '%s', completion is %d\n",  buf, completion);
				progress->value((float)completion / 100.0);
				//ML: also update text in progress bar
				sprintf(progress_percent, "%d%%", completion);
				progress->label(progress_percent);		
			}
			else if (strstr(buf, "HTTP/1.1 416")) {
				// we can get this error code if full file is already in the destination,
				// but we consider it a success
				ret = BBWW_FULL_SUCCESS;
				break;
			} else if (strstr(buf, "No such file")) {
				ret = BBWW_NOTFOUND_ERROR;
				break;
			}
			p = buf; // reset for scanning next line
		}
		else {
			p++;
		}
		
        // update the screen
        Fl::check();
	}
			
    childrc = pclose(pfp);
// Known cases handled above
    if (childrc == 0) {
		if (completion == 100) {
			ret = BBWW_FULL_SUCCESS;
		}
		else {
			ret = BBWW_CALL_ERROR;
		}
	}
	else if (childrc != 0 && ret == BBWW_CALL_ERROR)
    {
		ret = BBWW_CALL_ERROR;
    }

  }
  else
  {
    ret = BBWW_INIT_ERROR;
  }

  if (cmd)
  {
    free(cmd);
  }

  if (save_path)
  {
    free(save_path);
  }

  return ret;
}

int main(int argc, char *argv[])
{
  int result, ret = 1;
  char status_label[256];
  if (argc != 3)
  {
    fprintf(stderr, "Usage: %s <uri> <output file>\n", argv[0]);
    return 1;
  }

  //uri = argv[1];
  //save = argv[2];

  Fl_Window flwin(380, 60);
  flwin.begin();                     // add progress bar to it..
  Fl_Box 		status(10, 10, 360, 10, "(status)");
  status.labelsize(12);
  Fl_Progress	progress(10, 24, 360, 30);
  progress.minimum(0);               // set progress bar attribs..
  progress.maximum(1);
  progress.color(FL_WHITE, PROGRESS_COLOR);
  flwin.end();                       // end of adding to window

  // starting values
  flwin.label("wget");
  progress.value(0.0f);
  sprintf(status_label, "Downloading %s", basename(argv[1])); 
  status.label(status_label);
  flwin.show();

  // Computation loop..
  result = flbbwgetwrap(argv[1], argv[2], NULL,&progress);

  switch(result) {
	case BBWW_FULL_SUCCESS:
	  ret = 0;
	  break;
	case BBWW_NOTFOUND_ERROR:
	  fprintf(stderr, "File not found.  ");
	  ret  = 1;
	  break;
	case BBWW_NOSZ_ERROR:
	  fprintf(stderr, "Nothing specified to download.  ");
	  ret  = 1;
	  break;
	case BBWW_INIT_ERROR:
	  fprintf(stderr, "Initialization problem.  ");
	  ret  = 1;
	  break;
	case BBWW_CALL_ERROR:
	default:
	  fprintf(stderr, "Unknown problem.  ");
	  ret  = 1;
	  break;
	}
	return ret;
}
