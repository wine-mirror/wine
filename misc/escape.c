/*
 * Escape() function.
 *
 * Copyright 1994  Bob Amstadt
 */

static char Copyright[] = "Copyright  Bob Amstadt, 1994";

#include <stdlib.h>
#include <stdio.h>
#include "windows.h"

int Escape(HDC hdc, int nEscape, int cbInput, 
	   LPSTR lpszInData, LPSTR lpvOutData)
{
    fprintf(stderr, "Escape(nEscape = %04x)\n", nEscape);
    return 0;
}
