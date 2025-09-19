/*
 * an application for displaying Win32 console
 *
 * Copyright 2001, 2002 Eric Pouech
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

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wincon.h"
#include "winternl.h"

#include "wineconsole_res.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(console);

static WCHAR *lookup_executable(WCHAR *cmdline)
{
    WCHAR path[MAX_PATH];
    WCHAR *p;
    BOOL status;

    if (!(cmdline = wcsdup(cmdline))) return NULL;
    if ((p = wcspbrk(cmdline, L" \t"))) *p = L'\0';
    status = SearchPathW(NULL, cmdline, L".exe", ARRAY_SIZE(path), path, NULL);
    free(cmdline);
    return status ? wcsdup(path) : NULL;
}

static BOOL setup_target_console(WCHAR *title)
{
    BOOL ret;

    if (!FreeConsole()) return FALSE;
    /* Zero out std handles so that AllocConsole() sets the newly allocated handles as std,
     * and will be inherited by child process.
     */
    SetStdHandle(STD_INPUT_HANDLE, NULL);
    SetStdHandle(STD_OUTPUT_HANDLE, NULL);
    SetStdHandle(STD_ERROR_HANDLE, NULL);
    /* HACK: tweak process parameters to set the title to target executable
     * so that conhost will take config from that target process (and not wineconsole)
     */
    if (title)
    {
        UNICODE_STRING old = RtlGetCurrentPeb()->ProcessParameters->WindowTitle;
        RtlInitUnicodeString(&RtlGetCurrentPeb()->ProcessParameters->WindowTitle, title);
        ret = AllocConsole();
        RtlGetCurrentPeb()->ProcessParameters->WindowTitle = old;
        free(title);
    }
    else
        ret = AllocConsole();
    return ret;
}

int WINAPI wWinMain( HINSTANCE inst, HINSTANCE prev, WCHAR *cmdline, INT show )
{
    STARTUPINFOW startup = { sizeof(startup) };
    PROCESS_INFORMATION info;
    WCHAR *cmd = cmdline;
    DWORD exit_code;

    static WCHAR default_cmd[] = L"cmd";

    if (!*cmd) cmd = default_cmd;

    if (!setup_target_console(lookup_executable(cmdline)))
    {
        ERR( "failed to allocate console: %lu\n", GetLastError() );
        return 1;
    }

    if (!CreateProcessW( NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info ))
    {
        WCHAR format[256], *buf;
        INPUT_RECORD ir;
        DWORD len;

        exit_code = GetLastError();
        WARN( "CreateProcess '%ls' failed: %lu\n", cmd, exit_code );

        LoadStringW( GetModuleHandleW( NULL ), IDS_CMD_LAUNCH_FAILED, format, ARRAY_SIZE(format) );
        len = wcslen( format ) + wcslen( cmd );
        if ((buf = malloc( len * sizeof(WCHAR) )))
        {
            swprintf( buf, len, format, cmd );
            WriteConsoleW( startup.hStdOutput, buf, wcslen(buf), &len, NULL);
            while (ReadConsoleInputW( startup.hStdInput, &ir, 1, &len ) && ir.EventType == MOUSE_EVENT);
        }
        return exit_code;
    }

    /* detach from created console */
    FreeConsole();
    CloseHandle( info.hThread );
    WaitForSingleObject( info.hProcess, INFINITE );
    return GetExitCodeProcess( info.hProcess, &exit_code ) ? exit_code : GetLastError();
}
