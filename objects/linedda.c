/*
 * LineDDA
 *
 * Copyright 1993 Bob Amstadt
 *
static char Copyright[] = "Copyright  Bob Amstadt, 1993";
*/

#include <stdlib.h>
#include "windows.h"
#include "callback.h"

/**********************************************************************
 *		LineDDA		(GDI.100)
 */
void LineDDA(short nXStart, short nYStart, short nXEnd, short nYEnd,
	     FARPROC callback, long lParam)
{
    int xadd = 1, yadd = 1;
    int err,erradd;
    int cnt;
    int dx = nXEnd - nXStart;
    int dy = nYEnd - nYStart;

    if (dx < 0)  {
      dx = -dx; xadd = -1;
    }
    if (dy < 0)  {
      dy = -dy; yadd = -1;
    }
    if (dx > dy) { /* line is "more horizontal" */
      err = 2*dy - dx; erradd = 2*dy - 2*dx;
      for(cnt = 0;cnt <= dx; cnt++) {
	CallLineDDAProc(callback,nXStart,nYStart,lParam);
	if (err > 0) {
	  nYStart += yadd;
	  err += erradd;
	} else  {
	  err += 2*dy;
	}
	nXStart += xadd;
      }
    } else  { /* line is "more vertical" */
      err = 2*dx - dy; erradd = 2*dx - 2*dy;
      for(cnt = 0;cnt <= dy; cnt++) {
	CallLineDDAProc(callback,nXStart,nYStart,lParam);
	if (err > 0) {
	  nXStart += xadd;
	  err += erradd;
	} else  {
	  err += 2*dx;
	}
	nYStart += yadd;
      }
    }
}
