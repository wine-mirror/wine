/*
 * LineDDA
 *
 * Copyright 1993 Bob Amstadt
 */

#include <stdlib.h>
#include "windows.h"


/**********************************************************************
 *           LineDDA16   (GDI.100)
 */
void WINAPI LineDDA16( INT16 nXStart, INT16 nYStart, INT16 nXEnd, INT16 nYEnd,
                       LINEDDAPROC16 callback, LPARAM lParam )
{
    LineDDA32( nXStart, nYStart, nXEnd, nYEnd,
               (LINEDDAPROC32)callback, lParam );
}


/**********************************************************************
 *           LineDDA32   (GDI32.248)
 */
BOOL32 WINAPI LineDDA32(INT32 nXStart, INT32 nYStart, INT32 nXEnd, INT32 nYEnd,
                        LINEDDAPROC32 callback, LPARAM lParam )
{
    INT32 xadd = 1, yadd = 1;
    INT32 err,erradd;
    INT32 cnt;
    INT32 dx = nXEnd - nXStart;
    INT32 dy = nYEnd - nYStart;

    if (dx < 0)  {
      dx = -dx; xadd = -1;
    }
    if (dy < 0)  {
      dy = -dy; yadd = -1;
    }
    if (dx > dy) { /* line is "more horizontal" */
      err = 2*dy - dx; erradd = 2*dy - 2*dx;
      for(cnt = 0;cnt <= dx; cnt++) {
        callback(nXStart,nYStart,lParam);
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
	callback(nXStart,nYStart,lParam);
	if (err > 0) {
	  nXStart += xadd;
	  err += erradd;
	} else  {
	  err += 2*dx;
	}
	nYStart += yadd;
      }
    }
    return TRUE;
}
