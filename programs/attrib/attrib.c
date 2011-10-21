/*
 * ATTRIB - Wine-compatible attrib program
 *
 * Copyright 2010-2011 Christian Costa
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

#include <windows.h>
#include <wine/debug.h>
#include <wine/unicode.h>
#include "attrib.h"

WINE_DEFAULT_DEBUG_CHANNEL(attrib);

/* =========================================================================
 * Load a string from the resource file, handling any error
 * Returns string retrieved from resource file
 * ========================================================================= */
static WCHAR *ATTRIB_LoadMessage(UINT id) {
    static WCHAR msg[MAXSTRING];
    const WCHAR failedMsg[]  = {'F', 'a', 'i', 'l', 'e', 'd', '!', 0};

    if (!LoadStringW(GetModuleHandleW(NULL), id, msg, sizeof(msg)/sizeof(WCHAR))) {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        lstrcpyW(msg, failedMsg);
    }
    return msg;
}

/* =========================================================================
 * Output a formatted unicode string. Ideally this will go to the console
 *  and hence required WriteConsoleW to output it, however if file i/o is
 *  redirected, it needs to be WriteFile'd using OEM (not ANSI) format
 * ========================================================================= */
static int __cdecl ATTRIB_wprintf(const WCHAR *format, ...) {

    static WCHAR *output_bufW = NULL;
    static char  *output_bufA = NULL;
    static BOOL  toConsole    = TRUE;
    static BOOL  traceOutput  = FALSE;
#define MAX_WRITECONSOLE_SIZE 65535

    __ms_va_list parms;
    DWORD   nOut;
    int len;
    DWORD   res = 0;

    /*
     * Allocate buffer to use when writing to console
     * Note: Not freed - memory will be allocated once and released when
     *         xcopy ends
     */

    if (!output_bufW) output_bufW = HeapAlloc(GetProcessHeap(), 0,
                                              MAX_WRITECONSOLE_SIZE);
    if (!output_bufW) {
        WINE_FIXME("Out of memory - could not allocate 2 x 64K buffers\n");
        return 0;
    }

    __ms_va_start(parms, format);
    SetLastError(NO_ERROR);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, format, 0, 0, output_bufW,
                   MAX_WRITECONSOLE_SIZE/sizeof(*output_bufW), &parms);
    __ms_va_end(parms);
    if (len == 0 && GetLastError() != NO_ERROR) {
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(format));
        return 0;
    }

    /* Try to write as unicode all the time we think its a console */
    if (toConsole) {
        res = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                            output_bufW, len, &nOut, NULL);
    }

    /* If writing to console has failed (ever) we assume its file
       i/o so convert to OEM codepage and output                  */
    if (!res) {
        BOOL usedDefaultChar = FALSE;
        DWORD convertedChars;

        toConsole = FALSE;

        /*
         * Allocate buffer to use when writing to file. Not freed, as above
         */
        if (!output_bufA) output_bufA = HeapAlloc(GetProcessHeap(), 0,
                                                MAX_WRITECONSOLE_SIZE);
        if (!output_bufA) {
          WINE_FIXME("Out of memory - could not allocate 2 x 64K buffers\n");
          return 0;
        }

        /* Convert to OEM, then output */
        convertedChars = WideCharToMultiByte(GetConsoleOutputCP(), 0, output_bufW,
                            len, output_bufA, MAX_WRITECONSOLE_SIZE,
                            "?", &usedDefaultChar);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), output_bufA, convertedChars,
                  &nOut, FALSE);
    }

    /* Trace whether screen or console */
    if (!traceOutput) {
        WINE_TRACE("Writing to console? (%d)\n", toConsole);
        traceOutput = TRUE;
    }
    return nOut;
}

int wmain(int argc, WCHAR *argv[])
{
    DWORD count;
    HANDLE hff;
    WIN32_FIND_DATAW fd;
    WCHAR flags[9] = {' ',' ',' ',' ',' ',' ',' ',' ','\0'};
    WCHAR name[128];
    WCHAR *param = argc >= 2 ? argv[1] : NULL;
    DWORD attrib_set = 0;
    DWORD attrib_clear = 0;
    WCHAR help_option[] = {'/','?','\0'};

    if (param && !strcmpW(param, help_option))
    {
        ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_HELP));
        return 0;
    }

    if (param && (param[0] == '+' || param[0] == '-')) {
        DWORD attrib = 0;
        /* FIXME: the real cmd can handle many more than two args; this should be in a loop */
        switch (param[1]) {
        case 'H': case 'h': attrib |= FILE_ATTRIBUTE_HIDDEN; break;
        case 'S': case 's': attrib |= FILE_ATTRIBUTE_SYSTEM; break;
        case 'R': case 'r': attrib |= FILE_ATTRIBUTE_READONLY; break;
        case 'A': case 'a': attrib |= FILE_ATTRIBUTE_ARCHIVE; break;
        default:
            ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_NYI));
            return 0;
        }
        switch (param[0]) {
        case '+': attrib_set = attrib; break;
        case '-': attrib_clear = attrib; break;
        }
        param = argc >= 3 ? argv[2] : NULL;
    }

    if (!param || strlenW(param) == 0) {
        static const WCHAR slashStarW[]  = {'\\','*','\0'};

        GetCurrentDirectoryW(sizeof(name)/sizeof(WCHAR), name);
        strcatW (name, slashStarW);
    } else {
        strcpyW(name, param);
    }

    hff = FindFirstFileW(name, &fd);
    if (hff == INVALID_HANDLE_VALUE) {
        ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_FILENOTFOUND), name);
    }
    else {
        do {
            if (attrib_set || attrib_clear) {
                fd.dwFileAttributes &= ~attrib_clear;
                fd.dwFileAttributes |= attrib_set;
                if (!fd.dwFileAttributes)
                    fd.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
                SetFileAttributesW(name, fd.dwFileAttributes);
            } else {
                static const WCHAR fmt[] = {'%','1',' ',' ',' ','%','2','\n','\0'};
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
                    flags[0] = 'H';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
                    flags[1] = 'S';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
                    flags[2] = 'A';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                    flags[3] = 'R';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                    flags[4] = 'T';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                    flags[5] = 'C';
                }
                ATTRIB_wprintf(fmt, flags, fd.cFileName);
                for (count=0; count < 8; count++) flags[count] = ' ';
            }
        } while (FindNextFileW(hff, &fd) != 0);
    }
    FindClose (hff);

    return 0;
}
