/*
 * Win32 relay functions
 * The implementations here perform only parameter conversions, and
 * call the Win16 counterparts
 *
 * Copyright 1996 Martin von Loewis
 */

#include <stdio.h>
#include "windows.h"
#include "winerror.h"
#include "struct32.h"
#include "stddebug.h"
#include "debug.h"

void PARAM32_POINT32to16(const POINT32* p32,POINT* p16)
{
	p16->x = p32->x;
	p16->y = p32->y;
}

void PARAM32_SIZE16to32(const SIZE* p16, SIZE32* p32) 
  
{
        p32->cx = p16->cx;
        p32->cy = p16->cy;
}

/****************************************************************
 *           MoveToEx          (GDI32.254)
 */
BOOL WIN32_MoveToEx(HDC hdc,int x,int y,POINT32* p32)
{
	POINT p;
	if (p32 == NULL)
		return MoveToEx(hdc,x,y,(POINT *)NULL);
	else {
		PARAM32_POINT32to16(p32,&p);
		return MoveToEx(hdc,x,y,&p);
	}
}

BOOL WIN32_GetTextExtentPointA(HDC hdc, LPCTSTR str, int length, SIZE32* lpsize)

{
        SIZE s;
        BOOL retval;

        retval = GetTextExtentPoint(hdc, str, length, &s);
        PARAM32_SIZE16to32(&s, lpsize);

        return retval;
}
