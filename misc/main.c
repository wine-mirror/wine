/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 */

#include "config.h"

#include <locale.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "ntddk.h"
#include "winnls.h"
#include "winerror.h"
#include "options.h"
#include "debugtools.h"

DECLARE_DEBUG_CHANNEL(file);


/***********************************************************************
 *           Beep   (KERNEL32.11)
 */
BOOL WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    static char beep = '\a';
    /* dwFreq and dwDur are ignored by Win95 */
    if (isatty(2)) write( 2, &beep, 1 );
    return TRUE;
}


/***********************************************************************
*	FileCDR (KERNEL.130)
*/
FARPROC16 WINAPI FileCDR16(FARPROC16 x)
{
	FIXME_(file)("(0x%8x): stub\n", (int) x);
	return (FARPROC16)TRUE;
}

/***********************************************************************
 *           GetTickCount   (USER.13) (KERNEL32.299)
 *
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the wineserver.
 */
DWORD WINAPI GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - server_startticks;
}
