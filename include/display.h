/*
 * DISPLAY driver interface
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_DISPLAY_H
#define __WINE_DISPLAY_H

#include "windef.h"

struct tagCURSORICONINFO;

#include "pshpack1.h"
typedef struct tagCURSORINFO
{
    WORD wXMickeys;
    WORD wYMickeys;
} CURSORINFO, *PCURSORINFO, *LPCURSORINFO;
#include "poppack.h"

WORD WINAPI DISPLAY_Inquire(LPCURSORINFO lpCursorInfo);
VOID WINAPI DISPLAY_SetCursor( struct tagCURSORICONINFO *lpCursor );
VOID WINAPI DISPLAY_MoveCursor( WORD wAbsX, WORD wAbsY );
VOID WINAPI DISPLAY_CheckCursor();

#endif /* __WINE_DISPLAY_H */

