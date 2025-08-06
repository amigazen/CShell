/*
 *  get window bounds (dimensions)
 *  Copyright 1994-1995, Andreas M. Kirchwitz
 */


#include <exec/types.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <proto/dos.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned char aWSR[] = {0x9b, 0x30, 0x20, 0x71, 0};		/* Window Status Request */
static unsigned char aWBR[] = {0x9b, 0x31, 0x3b, 0x31, 0x3b, 0};	/* Window Bounds Report */

#define BUF_LEN			40



void get_WindowBounds_def(int *width, int *height)
{
	*width  = 80;
	*height = 24;
}



long get_WindowBounds_env(int *width, int *height)
{
	long got_wb = 0;
	unsigned char iline[BUF_LEN + 1];

	if (GetVar ("COLUMNS", iline, BUF_LEN, NULL) > 0L) {
		*width = atoi(iline);
		got_wb = 1;
	}

	if (GetVar ("LINES", iline, BUF_LEN, NULL) > 0L) {
		*height = atoi(iline);
		got_wb = 1;
	}

	return(got_wb);
}



long get_WindowBounds_con(BPTR con, int *width, int *height,long timeout)
{
	long got_wb = 0;
	unsigned char *p1, *p2, *p3;
	int i = 0, start = 0;
	unsigned char iline[BUF_LEN + 1];

	if (!con)
		return(got_wb);

	if (!IsInteractive(con))
		return(got_wb);

	Write (con, aWSR, strlen (aWSR));

	/* now fill buffer, start with beginning of WBR */

	while (i < BUF_LEN && WaitForChar (con, timeout)) {
		Read (con, &iline[i], 1);
		if (start)
			i++;
		else if (iline[i] == aWBR[0]) {
			i++;
			start = 1;
		}
	}

	iline[i] = 0;

	if (i > 0) {
		if (p1 = strstr (iline, aWBR)) {
			p1 += strlen (aWBR);
			if (p2 = strchr (p1, ';')) {
				*p2 = 0;
				++p2;
				if (p3 = strchr (p2, 'r')) {
					*p3 = 0;

					*height = atoi (p1);
					*width = atoi (p2);

					got_wb = 1;
				}
			}
		}
	}

	/*
	    9b 31 3b 31 3b <height> 3b <width> 20 72
	 */

	return(got_wb);
}



long get_WindowBounds_Output(int *width, int *height,long timeout)
{
	long got_wb = 0;

	if (Output()) {
		if (IsInteractive(Output())) {
			SetMode(Output(),1);	/* raw */
			got_wb = get_WindowBounds_con(Output(),width,height,timeout);
			SetMode(Output(),0);	/* cooked */
		}
	}

	return(got_wb);
}

