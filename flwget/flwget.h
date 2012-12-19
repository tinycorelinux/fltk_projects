/* flbbww.H */

/* assumes unofficial busybox wget patch */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Progress.H>

#define BBWW_DEFAULT_BASE_CMD_FMT   "cd %s && exec busybox wget -c %s 2>&1"
#define BBWW_EXPECT_STR_FMTS        2
#define BBWW_PROGRESS_WIDTH         4  /* fits 100% */

enum
{
  BBWW_SPEC_ERROR = -1,
  BBWW_FULL_SUCCESS = 0,
  BBWW_INIT_ERROR,
  BBWW_NOSZ_ERROR,
  BBWW_CALL_ERROR,
  BBWW_NOTFOUND_ERROR
};

/* assumes uri is proto://host/path/to/file */
int flbbwgetwrap(char *uri, const char *save_dir, const char *base_cmd_fmt,
  Fl_Progress *progress);

