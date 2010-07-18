/*
 * 16-bit winhelp implementation
 *
 * Copyright 2009 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>

#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winhelp);

/**************************************************************************
 *           WINHELP entry point
 */
WORD WINAPI WinMain16( HINSTANCE16 inst, HINSTANCE16 prev, LPSTR cmdline, WORD show )
{
    int len = GetSystemDirectoryA( NULL, 0 ) + sizeof("\\winhlp32.exe ") + strlen(cmdline);
    char *buffer = HeapAlloc( GetProcessHeap(), 0, len );

    GetSystemDirectoryA( buffer, len );
    strcat( buffer, "\\winhlp32.exe " );
    strcat( buffer, cmdline );

    WINE_TRACE( "starting %s\n", wine_dbgstr_a(buffer) );

    WinExec16( buffer, show );

    HeapFree( GetProcessHeap(), 0, buffer );
    ExitThread( 0 );
}
