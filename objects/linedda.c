/*
 * LineDDA
 *
 * Copyright 1993 Bob Amstadt
 *
static char Copyright[] = "Copyright  Bob Amstadt, 1993";
*/

#include <stdlib.h>
#include "windows.h"
#include "if1632.h"

/**********************************************************************
 *		LineDDA		(GDI.100)
 */
void LineDDA(short nXStart, short nYStart, short nXEnd, short nYEnd,
	     FARPROC callback, long lParam)
{
    int x, y;
    int b;
    int x_diff = nXEnd - nXStart;
    int y_diff = nYEnd - nYStart;

    if (x_diff == 0 && y_diff == 0)
	return;
    
    if ((abs(x_diff) < abs(y_diff) && x_diff != 0) || y_diff == 0)
    {
	b = (nXStart * y_diff) / x_diff - nYStart;

	for (x = nXStart; x <= nXEnd; x++)
	{
	    y = (x * y_diff) / x_diff + b;
	    CallLineDDAProc(callback, x, y, lParam);
	}
    }
    else
    {
	b = (nYStart * x_diff) / y_diff - nXStart;

	for (y = nYStart; y <= nYEnd; y++)
	{
	    x = (y * x_diff) / y_diff + b;
	    CallLineDDAProc(callback, x, y, lParam);
	}
    }
}
