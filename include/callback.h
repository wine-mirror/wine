/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef WINE_CALLBACK_H
#define WINE_CALLBACK_H

#include <stdlib.h>
#include <stdarg.h>

#include "stackframe.h"

extern
int CallTo32_LargeStack( int (*func)(), int nbargs, ... );


/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

                               /* func     ds    parameters */
extern WORD CallTo16_word_     ( FARPROC16, WORD );

#ifndef WINELIB

extern WORD CallTo16_word_w    (FARPROC16, WORD, WORD);
extern WORD CallTo16_word_ww   (FARPROC16, WORD, WORD, WORD);
extern WORD CallTo16_word_wl   (FARPROC16, WORD, WORD, LONG);
extern WORD CallTo16_word_ll   (FARPROC16, WORD, LONG, LONG);
extern WORD CallTo16_word_www  (FARPROC16, WORD, WORD, WORD, WORD);
extern WORD CallTo16_word_wwl  (FARPROC16, WORD, WORD, WORD, LONG);
extern WORD CallTo16_word_wlw  (FARPROC16, WORD, WORD, LONG, WORD);
extern LONG CallTo16_long_wwl  (FARPROC16, WORD, WORD, WORD, LONG);
extern WORD CallTo16_word_llwl (FARPROC16, WORD, LONG, LONG, WORD, LONG);
extern LONG CallTo16_long_wwwl (FARPROC16, WORD, WORD, WORD, WORD, LONG);
extern WORD CallTo16_word_lwww (FARPROC16, WORD, LONG, WORD, WORD, WORD);
extern WORD CallTo16_word_wwll (FARPROC16, WORD, WORD, WORD, LONG, LONG);
extern WORD CallTo16_word_wllwl(FARPROC16, WORD, WORD, LONG, LONG, WORD, LONG);
extern LONG CallTo16_long_lwwll(FARPROC16, WORD, LONG, WORD, WORD, LONG, LONG);
extern WORD CallTo16_word_wwlll(FARPROC16, WORD, WORD, WORD, LONG, LONG, LONG);
extern LONG CallTo16_long_lllllllwlwwwl( FARPROC16, WORD, LONG, LONG, LONG,
                                         LONG, LONG, LONG, LONG, WORD, LONG,
                                         WORD, WORD, WORD, LONG );

extern WORD CallTo16_regs_( FARPROC16 func, WORD ds, WORD es, WORD bp, WORD ax,
                            WORD bx, WORD cx, WORD dx, WORD si, WORD di );

#define CallDCHookProc( func, hdc, code, data, lparam) \
    CallTo16_word_wwll( func, CURRENT_DS, hdc, code, data, lparam )
#define CallDriverProc( func, dwId, msg, hdrvr, lparam1, lparam2 ) \
    CallTo16_long_lwwll( func, CURRENT_DS, dwId, msg, hdrvr, lparam1, lparam2 )
#define CallEnumChildProc( func, hwnd, lParam ) \
    CallTo16_word_wl( func, CURRENT_DS, hwnd, lParam )
#define CallEnumFontFamProc( func, lpfont, lpmetric, type, lParam ) \
    CallTo16_word_llwl( func, CURRENT_DS, lpfont, lpmetric, type, lParam )
#define CallEnumFontsProc( func, lpfont, lpmetric, type, lParam ) \
    CallTo16_word_llwl( func, CURRENT_DS, lpfont, lpmetric, type, lParam )
#define CallEnumMetafileProc( func, hdc, lptable, lprecord, objs, lParam ) \
    CallTo16_word_wllwl(func, CURRENT_DS, hdc, lptable, lprecord, objs, lParam)
#define CallEnumObjectsProc( func, lpobj, lParam ) \
    CallTo16_word_ll( func, CURRENT_DS, lpobj, lParam )
#define CallEnumPropProc16( func, hwnd, lpstr, handle ) \
    CallTo16_word_wlw( (FARPROC16)(func), CURRENT_DS, hwnd, lpstr, handle )
#define CallEnumTaskWndProc( func, hwnd, lParam ) \
    CallTo16_word_wl( func, CURRENT_DS, hwnd, lParam )
#define CallEnumWindowsProc16( func, hwnd, lParam ) \
    CallTo16_word_wl( func, CURRENT_DS, hwnd, lParam )
#define CallLineDDAProc( func, xPos, yPos, lParam ) \
    CallTo16_word_wwl( func, CURRENT_DS, xPos, yPos, lParam )
#define CallGrayStringProc( func, hdc, lParam, cch ) \
    CallTo16_word_wlw( func, CURRENT_DS, hdc, lParam, cch )
#define CallHookProc( func, code, wParam, lParam ) \
    CallTo16_long_wwl( func, CURRENT_DS, code, wParam, lParam )
#define CallTimeFuncProc( func, id, msg, dwUser, dw1, dw2 ) \
    CallTo16_word_wwlll( func, CURRENT_DS, id, msg, dwUser, dw1, dw2 )
#define CallWindowsExitProc( func, nExitType ) \
    CallTo16_word_w( func, CURRENT_DS, nExitType )
#define CallWndProc16( func, ds, hwnd, msg, wParam, lParam ) \
    CallTo16_long_wwwl( (FARPROC16)(func), ds, hwnd, msg, wParam, lParam )
#define CallWordBreakProc( func, lpch, ichCurrent, cch, code ) \
    CallTo16_word_lwww( func, CURRENT_DS, lpch, ichCurrent, cch, code )
#define CallWndProcNCCREATE16( func, ds, exStyle, clsName, winName, style, \
                               x, y, cx, cy, hparent, hmenu, instance, \
                               params, hwnd, msg, wParam, lParam ) \
    CallTo16_long_lllllllwlwwwl( (FARPROC16)(func), ds, exStyle, clsName, \
                              winName, style, MAKELONG(y,x), MAKELONG(cy,cx), \
                              MAKELONG(hmenu,hparent), instance, params, \
                              hwnd, msg, wParam, lParam )

/* List of the 32-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto32.S */

extern LONG CallTo32_0( FARPROC32 );
extern LONG CallTo32_2( FARPROC32, DWORD, DWORD );
extern LONG CallTo32_3( FARPROC32, DWORD, DWORD, DWORD );
extern LONG CallTo32_4( FARPROC32, DWORD, DWORD, DWORD, DWORD );

#define CallTaskStart32( func ) \
    CallTo32_0( func )
#define CallDLLEntryProc32( func, hmodule, a, b ) \
    CallTo32_3( func, hmodule, a, b )
#define CallEnumPropProc32( func, hwnd, lpstr, handle ) \
    CallTo32_3( (FARPROC32)(func), hwnd, (DWORD)(lpstr), handle )
#define CallEnumPropProcEx32( func, hwnd, lpstr, handle, data ) \
    CallTo32_4( (FARPROC32)(func), hwnd, (DWORD)(lpstr), handle, data )
#define CallEnumWindowsProc32( func, hwnd, lParam ) \
    CallTo32_2( func, hwnd, lParam )
#define CallWndProc32( func, hwnd, msg, wParam, lParam ) \
    CallTo32_4( func, hwnd, msg, wParam, lParam )


#else  /* WINELIB */

#define CallDCHookProc( func, hdc, code, data, lparam ) \
    (*func)( hdc, code, data, lparam )
#define CallDriverProc( func, dwId, msg, hdrvr, lparam1, lparam2 ) \
    (*func)( dwId, msg, hdrvr, lparam1, lparam2 )
#define CallEnumChildProc( func, hwnd, lParam ) \
    (*func)( hwnd, lParam )
#define CallEnumFontFamProc( func, lpfont, lpmetric, type, lParam ) \
    (*func)( lpfont, lpmetric, type, lParam )
#define CallEnumFontsProc( func, lpfont, lpmetric, type, lParam ) \
    (*func)( lpfont, lpmetric, type, lParam )
#define CallEnumMetafileProc( func, hdc, lptable, lprecord, objs, lParam ) \
    (*func)( hdc, lptable, lprecord, objs, lParam)
#define CallEnumObjectsProc( func, lpobj, lParam ) \
    (*func)( lpobj, lParam )
#define CallEnumPropProc16( func, hwnd, lpstr, handle ) \
    (*func)( hwnd, lpstr, handle )
#define CallEnumPropProc32( func, hwnd, lpstr, handle ) \
    (*func)( hwnd, lpstr, handle )
#define CallEnumPropProcEx32( func, hwnd, lpstr, handle, data ) \
    (*func)( hwnd, lpstr, handle, data )
#define CallEnumTaskWndProc( func, hwnd, lParam ) \
    (*func)( hwnd, lParam )
#define CallEnumWindowsProc16( func, hwnd, lParam ) \
    (*func)( hwnd, lParam )
#define CallEnumWindowsProc32( func, hwnd, lParam ) \
    (*func)( hwnd, lParam )
#define CallLineDDAProc( func, xPos, yPos, lParam ) \
    (*func)( xPos, yPos, lParam )
#define CallGrayStringProc( func, hdc, lParam, cch ) \
    (*func)( hdc, lParam, cch )
#define CallHookProc( func, code, wParam, lParam ) \
    (*func)( code, wParam, lParam )
#define CallTimeFuncProc( func, id, msg, dwUser, dw1, dw2 ) \
    (*func)( id, msg, dwUser, dw1, dw2 )
#define CallWindowsExitProc( func, nExitType ) \
    (*func)( nExitType )
#define CallWndProc16( func, ds, hwnd, msg, wParam, lParam ) \
    (*func)( hwnd, msg, wParam, lParam )
#define CallWndProc32( func, hwnd, msg, wParam, lParam ) \
    (*func)( hwnd, msg, wParam, lParam )
#define CallWordBreakProc( func, lpch, ichCurrent, cch, code ) \
    (*func)( lpch, ichCurrent, cch, code )

#endif  /* WINELIB */


#endif /* WINE_CALLBACK_H */
