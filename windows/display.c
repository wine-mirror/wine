/*
 * DISPLAY driver
 *
 * Copyright 1998 Ulrich Weigand
 *
 */

#include "debug.h"
#include "display.h"
#include "wintypes.h"

extern MOUSE_DRIVER X11DRV_MOUSE_Driver;

/***********************************************************************
 *           MOUSE_GetDriver()
 */
MOUSE_DRIVER *MOUSE_GetDriver()
{
  return &X11DRV_MOUSE_Driver;
}

/***********************************************************************
 *           DISPLAY_Inquire			(DISPLAY.101)
 */
WORD WINAPI DISPLAY_Inquire(LPCURSORINFO lpCursorInfo) 
{
    lpCursorInfo->wXMickeys = 1;
    lpCursorInfo->wYMickeys = 1;

    return sizeof(CURSORINFO);
}

/***********************************************************************
 *           DISPLAY_SetCursor			(DISPLAY.102)
 */
VOID WINAPI DISPLAY_SetCursor( CURSORICONINFO *lpCursor )
{
   MOUSE_GetDriver()->pSetCursor(lpCursor);
}

/***********************************************************************
 *           DISPLAY_MoveCursor			(DISPLAY.103)
 */
VOID WINAPI DISPLAY_MoveCursor( WORD wAbsX, WORD wAbsY )
{
   MOUSE_GetDriver()->pMoveCursor(wAbsX, wAbsY);
}

/***********************************************************************
 *           DISPLAY_CheckCursor                  (DISPLAY.104)
 */
VOID WINAPI DISPLAY_CheckCursor()
{
    FIXME( cursor, "stub\n" );
}

/***********************************************************************
 *           UserRepaintDisable			(DISPLAY.500)
 */
VOID WINAPI UserRepaintDisable( BOOL16 disable )
{
    TRACE( cursor, "(%d): stub\n", disable );
}
