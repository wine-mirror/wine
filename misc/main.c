/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <locale.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winternl.h"
#include "winnls.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(file);


/***********************************************************************
 *           Beep   (KERNEL32.@)
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
	FIXME("(0x%8x): stub\n", (int) x);
	return (FARPROC16)TRUE;
}
