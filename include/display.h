/*
 * DISPLAY driver interface
 *
 * Copyright 1998 Ulrich Weigand
 */

#ifndef __WINE_DISPLAY_H
#define __WINE_DISPLAY_H

#include "cursoricon.h"
#include "wine/winuser16.h"

#pragma pack(1)
typedef struct _CURSORINFO
{
    WORD wXMickeys;
    WORD wYMickeys;
} CURSORINFO, *LPCURSORINFO;
#pragma pack(4)

typedef struct _MOUSE_DRIVER {
  VOID (*pSetCursor)(CURSORICONINFO *);
  VOID (*pMoveCursor)(WORD, WORD);
} MOUSE_DRIVER;

WORD WINAPI DISPLAY_Inquire(LPCURSORINFO lpCursorInfo);
VOID WINAPI DISPLAY_SetCursor( CURSORICONINFO *lpCursor );
VOID WINAPI DISPLAY_MoveCursor( WORD wAbsX, WORD wAbsY );
VOID WINAPI DISPLAY_CheckCursor();

#endif /* __WINE_DISPLAY_H */

