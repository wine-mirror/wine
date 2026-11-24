/*
 * WINOLDAP implementation
 *
 * Copyright 2000 Alexandre Julliard
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

#include "windef.h"
#include "winbase.h"
#include "ntuser.h"
#include "wine/winbase16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(module);


/**************************************************************************
 *           WINOLDAP entry point
 */
WORD WINAPI WinMain16( HINSTANCE16 inst, HINSTANCE16 prev, LPSTR cmdline, WORD show )
{
    PROCESS_INFORMATION info;
    STARTUPINFOA startup;
    DWORD count, exit_code = 1;

    WINE_TRACE( "%x %x %s %u\n", inst, prev, wine_dbgstr_a(cmdline), show );

    memset( &startup, 0, sizeof(startup) );
    startup.cb = sizeof(startup);

    if (CreateProcessA( NULL, cmdline, NULL, NULL, FALSE,
                        0, NULL, NULL, &startup, &info ))
    {
        /* Give 10 seconds to the app to come up */
        if (NtUserWaitForInputIdle( info.hProcess, 10000, TRUE ) == WAIT_FAILED)
            WINE_WARN("WaitForInputIdle failed: Error %ld\n", GetLastError() );
        ReleaseThunkLock( &count );
        WaitForSingleObject( info.hProcess, INFINITE );
        RestoreThunkLock( count );

        GetExitCodeProcess( info.hProcess, &exit_code );
        CloseHandle( info.hThread );
        CloseHandle( info.hProcess );
    }

    HeapFree( GetProcessHeap(), 0, cmdline );
    ExitThread( exit_code );
}
