/*
 * USER 16-bit thunks
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1998       Ulrich Weigand
 */

#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "callback.h"

/* ### start build ### */
extern WORD CALLBACK THUNK_CallTo16_word_wl   (FARPROC16,WORD,LONG);
extern WORD CALLBACK THUNK_CallTo16_word_wlw  (FARPROC16,WORD,LONG,WORD);
/* ### stop build ### */


/*******************************************************************
 *           EnumWindows16   (USER.54)
 */
BOOL16 WINAPI EnumWindows16( WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumWindows( (WNDENUMPROC)&thunk, lParam );
}


/**********************************************************************
 *           EnumChildWindows16   (USER.55)
 */
BOOL16 WINAPI EnumChildWindows16( HWND16 parent, WNDENUMPROC16 func, LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumChildWindows( parent, (WNDENUMPROC)&thunk, lParam );
}


/**********************************************************************
 *           THUNK_EnumTaskWindows16   (USER.225)
 */
BOOL16 WINAPI THUNK_EnumTaskWindows16( HTASK16 hTask, WNDENUMPROC16 func,
                                       LPARAM lParam )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wl );
    return EnumTaskWindows16( hTask, (WNDENUMPROC16)&thunk, lParam );
}


/***********************************************************************
 *           THUNK_EnumProps16   (USER.27)
 */
INT16 WINAPI THUNK_EnumProps16( HWND16 hwnd, PROPENUMPROC16 func )
{
    DECL_THUNK( thunk, func, THUNK_CallTo16_word_wlw );
    return EnumProps16( hwnd, (PROPENUMPROC16)&thunk );
}


/***********************************************************************
 *           THUNK_GrayString16   (USER.185)
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


