/*
 * USER 16-bit thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1998       Ulrich Weigand
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "win.h"
#include "callback.h"

/* ### start build ### */
extern WORD CALLBACK THUNK_CallTo16_word_wl   (FARPROC16,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wlw  (FARPROC16,WORD,LONG,WORD);
extern WORD CALLBACK THUNK_CallTo16_word_wlwww(FARPROC16,WORD,LONG,WORD,WORD,WORD);
/* ### stop build ### */


/*******************************************************************
 *           EnumWindows   (USER.54)
 */
BOOL16 WINAPI EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumWindows( (WNDENUMPROC)&thunk, lParam );
}


/**********************************************************************
 *           EnumChildWindows   (USER.55)
 */
BOOL16 WINAPI EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumChildWindows( WIN_Handle32(parent), (WNDENUMPROC)&thunk, lParam );
}


/**********************************************************************
 *           EnumTaskWindows   (USER.225)
 */
BOOL16 WINAPI THUNK_EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                       LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumTaskWindows16( hTask, (WNDENUMPROC16)&thunk, lParam );
}


/***********************************************************************
 *           EnumProps   (USER.27)
 */
INT16 WINAPI THUNK_EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wlw );
    return EnumProps16( hwnd, (PROPENUMPROC16)&thunk );
}


/***********************************************************************
 *           GrayString   (USER.185)
 */
BOOL16 WINAPI THUNK_GrayString16( HDC16 hdc, HBRUSH16 hbr,
                                  GRAYSTRINGPROC16 func, LPARAM lParam,
                                  INT16 cch, INT16 x, INT16 y,
                                  INT16 cx, INT16 cy )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wlw );
    if (!func)
        return GrayString16( hdc, hbr, NULL, lParam, cch, x, y, cx, cy );
    else
        return GrayString16( hdc, hbr, (GRAYSTRINGPROC16)&thunk, lParam, cch,
                             x, y, cx, cy );
}


/**********************************************************************
 *	     DrawState    (USER.449)
 */
BOOL16 WINAPI DrawState16( HDC16 hdc, HBRUSH16 hbr, DRAWSTATEPROC16 func, LPARAM ldata,
                           WPARAM16 wdata, INT16 x, INT16 y, INT16 cx, INT16 cy, UINT16 flags )
{
    UINT opcode = flags & 0xf;
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wlwww );

    if (opcode == DST_TEXT || opcode == DST_PREFIXTEXT)
    {
        /* make sure DrawStateA doesn't try to use ldata as a pointer */
        if (!wdata) wdata = strlen( MapSL(ldata) );
        if (!cx || !cy)
        {
            SIZE s;
            if (!GetTextExtentPoint32A( hdc, MapSL(ldata), wdata, &s )) return FALSE;
            if (!cx) cx = s.cx;
            if (!cy) cy = s.cy;
        }
    }
    return DrawStateA( hdc, hbr, (DRAWSTATEPROC)&thunk, ldata, wdata, x, y, cx, cy, flags );
}
