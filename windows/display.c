/*
 * DISPLAY driver
 *
 * Copyright 1998 Ulrich Weigand
 *
 */

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"

#include "windows.h"
#include "win.h"
#include "gdi.h"
#include "heap.h"
#include "debug.h"
#include "debugtools.h"


/***********************************************************************
 *           DisplayInquire			(DISPLAY.101)
 */

#pragma pack(1)
typedef struct _CURSORINFO
{
    WORD wXMickeys;
    WORD wYMickeys;
} CURSORINFO;
#pragma pack(4)

WORD WINAPI DisplayInquire(CURSORINFO *cursorInfo) 
{
    cursorInfo->wXMickeys = 1;
    cursorInfo->wYMickeys = 1;

    return sizeof(CURSORINFO);
}

/***********************************************************************
 *           DisplaySetCursor			(DISPLAY.102)
 */
VOID WINAPI DisplaySetCursor( LPVOID cursorShape )
{
    FIXME(keyboard, "(%p): stub\n", cursorShape );
}

/***********************************************************************
 *           DisplayMoveCursor			(DISPLAY.103)
 */
VOID WINAPI DisplayMoveCursor( WORD wAbsX, WORD wAbsY )
{
    FIXME(keyboard, "(%d,%d): stub\n", wAbsX, wAbsY );
}

/***********************************************************************
 *           DisplayCheckCursor                  (DISPLAY.104)
 */
VOID WINAPI DisplayCheckCursor()
{
    FIXME(keyboard, "stub\n");
}

/***********************************************************************
 *           UserRepaintDisable			(DISPLAY.103)
 */
VOID WINAPI UserRepaintDisable( BOOL16 disable )
{
    FIXME(keyboard, "(%d): stub\n", disable);
}

