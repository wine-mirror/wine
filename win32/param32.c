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
#include "stackframe.h"
#include "stddebug.h"
#include "debug.h"

/****************************************************************
 *           MoveToEx          (GDI32.254)
 */
BOOL WIN32_MoveToEx(HDC hdc,int x,int y,POINT32* p32)
{
	POINT p;
	if (p32 == NULL)
		return MoveToEx(hdc,x,y,(POINT *)NULL);
	else {
		STRUCT32_POINT32to16(p32,&p);
		return MoveToEx(hdc,x,y,&p);
	}
}

BOOL WIN32_GetTextExtentPointA(HDC hdc, LPCTSTR str, int length, SIZE32* lpsize)

{
        SIZE s;
        BOOL retval;

        retval = GetTextExtentPoint(hdc, str, length, &s);
        STRUCT32_SIZE16to32(&s, lpsize);

        return retval;
}

ATOM WIN32_GlobalAddAtomA(LPCTSTR str)
{
    char buffer[256];  /* 16-bit atoms are limited to 255 anyway */
    lstrcpyn( buffer, str, sizeof(buffer) );
    return GlobalAddAtom(MAKE_SEGPTR(buffer));
}
