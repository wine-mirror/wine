/*
 * Callback functions
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef __WINE_CALLBACK_H
#define __WINE_CALLBACK_H

#include "wintypes.h"
#include "winnt.h"

extern
int CallTo32_LargeStack( int (*func)(), int nbargs, ... );

/* List of the 16-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto16.S */

#ifndef WINELIB

extern void CallTo16_regs_     ( const CONTEXT *context );
extern WORD CallTo16_word_     ( FARPROC16 );
extern WORD CallTo16_word_w    ( FARPROC16, WORD );
extern WORD CallTo16_word_ww   ( FARPROC16, WORD, WORD );
extern WORD CallTo16_word_wl   ( FARPROC16, WORD, LONG );
extern WORD CallTo16_word_ll   ( FARPROC16, LONG, LONG );
extern WORD CallTo16_word_www  ( FARPROC16, WORD, WORD, WORD );
extern WORD CallTo16_word_wwl  ( FARPROC16, WORD, WORD, LONG );
extern WORD CallTo16_word_wlw  ( FARPROC16, WORD, LONG, WORD );
extern LONG CallTo16_long_wwl  ( FARPROC16, WORD, WORD, LONG );
extern WORD CallTo16_word_llwl ( FARPROC16, LONG, LONG, WORD, LONG );
extern LONG CallTo16_long_wwwl ( FARPROC16, WORD, WORD, WORD, LONG );
extern WORD CallTo16_word_lwww ( FARPROC16, LONG, WORD, WORD, WORD );
extern WORD CallTo16_word_wwll ( FARPROC16, WORD, WORD, LONG, LONG );
extern WORD CallTo16_word_wllwl( FARPROC16, WORD, LONG, LONG, WORD, LONG );
extern LONG CallTo16_long_lwwll( FARPROC16, LONG, WORD, WORD, LONG, LONG );
extern WORD CallTo16_word_wwlll( FARPROC16, WORD, WORD, LONG, LONG, LONG );
extern LONG CallTo16_long_lllllllwlwwwl( FARPROC16, LONG, LONG, LONG,
                                         LONG, LONG, LONG, LONG, WORD, LONG,
                                         WORD, WORD, WORD, LONG );
extern WORD CallTo16_word_lwll ( FARPROC16, LONG, WORD, LONG, LONG);
extern WORD CallTo16_word_lwlll( FARPROC16, LONG, WORD, LONG, LONG, LONG );
extern WORD CallTo16_word_llll ( FARPROC16, LONG, LONG, LONG, LONG);
extern LONG CallTo16_long_lwlll( FARPROC16, LONG, WORD, LONG, LONG, LONG );
extern LONG CallTo16_long_lwwllwlllllw( FARPROC16, LONG, WORD, WORD, LONG, 
				       LONG, WORD, LONG, LONG, LONG, LONG, 
				       LONG, WORD);

#define CallDriverProc( func, dwId, msg, hdrvr, lparam1, lparam2 ) \
    CallTo16_long_lwwll( func, dwId, msg, hdrvr, lparam1, lparam2 )
#define CallDriverCallback( func, hdev, msg, user, lparam1, lparam2 ) \
    CallTo16_word_wwlll( func, hdev, msg, user, lparam1, lparam2 )
#define CallTimeFuncProc( func, id, msg, dwUser, dw1, dw2 ) \
    CallTo16_word_wwlll( func, id, msg, dwUser, dw1, dw2 )
#define CallWindowsExitProc( func, nExitType ) \
    CallTo16_word_w( func, nExitType )
#define CallWordBreakProc16( func, lpch, ichCurrent, cch, code ) \
    CallTo16_word_lwww( func, lpch, ichCurrent, cch, code )


/* List of the 32-bit callback functions. This list is used  */
/* by the build program to generate the file if1632/callto32.S */

extern LONG CallTo32_0( FARPROC32 );
extern LONG CallTo32_1( FARPROC32, DWORD );
extern LONG CallTo32_2( FARPROC32, DWORD, DWORD );
extern LONG CallTo32_3( FARPROC32, DWORD, DWORD, DWORD );
extern LONG CallTo32_4( FARPROC32, DWORD, DWORD, DWORD, DWORD );
extern LONG CallTo32_5( FARPROC32, DWORD, DWORD, DWORD, DWORD, DWORD );

#define CallTaskStart32( func ) \
    CallTo32_0( func )
#define CallDLLEntryProc32( func, hmodule, a, b ) \
    CallTo32_3( func, hmodule, a, b )
#define CallWordBreakProc32A( func, lpch, ichCurrent, cch, code ) \
    CallTo32_4( func, (DWORD)lpch, ichCurrent, cch, code )


#else  /* WINELIB */

#define CallDriverProc( func, dwId, msg, hdrvr, lparam1, lparam2 ) \
    (*func)( dwId, msg, hdrvr, lparam1, lparam2 )
#define CallDriverCallback( func, hdev, msg, user, lparam1, lparam2 ) \
    (*func)( hdev, msg, user, lparam1, lparam2 )
#define CallTimeFuncProc( func, id, msg, dwUser, dw1, dw2 ) \
    (*func)( id, msg, dwUser, dw1, dw2 )
#define CallWindowsExitProc( func, nExitType ) \
    (*func)( nExitType )
#define CallWordBreakProc16( func, lpch, ichCurrent, cch, code ) \
    (*func)( lpch, ichCurrent, cch, code )
#define CallWordBreakProc32A( func, lpch, ichCurrent, cch, code ) \
    (*func)( lpch, ichCurrent, cch, code )

#endif  /* WINELIB */


#endif /* __WINE_CALLBACK_H */
