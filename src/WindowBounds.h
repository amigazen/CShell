/*
 *  get window bounds (dimensions)
 *  Copyright 1994-1995, Andreas M. Kirchwitz
 */


#include <dos/dos.h>

#define GWB_TIMEOUT_LOCAL	1		/* one microsecond */
#define GWB_TIMEOUT_SHORT	1000		/* one millisecond */
#define GWB_TIMEOUT_LONG	1000000		/* one full second */



void get_WindowBounds_def(int *width, int *height);
 /*
  *  get default window bounds  (80x24)
  */



long get_WindowBounds_env(int *width, int *height);
 /*
  *  get window bounds from environment  (COLUMNS, LINES),
  *  first local then global
  *
  *  width/height are only modified if COLUMNS/LINES is set
  *
  *  returns 0, if no window bound could be read
  */



long get_WindowBounds_con(BPTR con, int *width, int *height,long timeout);
 /*
  *  get window bounds from console (with user defined timeout),
  *  "con" must be in RAW mode
  *
  *  width/height are only modified if console responds to the
  *  "window status report" (special Amiga control sequence)
  *  within the specified timeout period (in microseconds)
  *
  *  returns 0, if no window bound could be read
  */



long get_WindowBounds_Output(int *width, int *height,long timeout);
 /*
  *  get window bounds from Output()
  *
  *  sets to RAW, calls get_WindowBounds_con(), sets to cooked
  *
  *  returns 0, if no window bound could be read
  */

