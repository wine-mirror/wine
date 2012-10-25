/*
 * ATTRIB - Wine-compatible attrib program
 *
 * Copyright 2010-2012 Christian Costa
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
static WCHAR *ATTRIB_LoadMessage(UINT id)
{
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
static int __cdecl ATTRIB_wprintf(const WCHAR *format, ...)
{
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
    WCHAR flags[] = {' ',' ',' ',' ',' ',' ',' ',' ','\0'};
    WCHAR name[MAX_PATH];
    WCHAR *namepart;
    WCHAR curdir[MAX_PATH];
    DWORD attrib_set = 0;
    DWORD attrib_clear = 0;
    const WCHAR help_option[] = {'/','?','\0'};
    const WCHAR start[]  = {'*','\0'};
    int i = 1;

    if ((argc >= 2) && !strcmpW(argv[1], help_option)) {
        ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_HELP));
        return 0;
    }

    /* By default all files from current directory are taken into account */
    strcpyW(name, start);

    while (i < argc) {
        WCHAR *param = argv[i++];
        WINE_TRACE("Processing arg: '%s'\n", wine_dbgstr_w(param));
        if ((param[0] == '+') || (param[0] == '-')) {
            DWORD attrib = 0;
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
        } else if (param[0] == '/') {
            if (((param[1] == 'D') || (param[1] == 'd')) && !param[2]) {
                WINE_FIXME("Option /D not yet supported\n");
            } else if (((param[1] == 'R') || (param[1] == 'r')) && !param[2]) {
                WINE_FIXME("Option /R not yet supported\n");
            } else {
                WINE_FIXME("Unknown option %s\n", debugstr_w(param));
            }
        } else if (param[0]) {
            strcpyW(name, param);
        }
    }

    /* Name may be a relative or explicit path, so calculate curdir based on
       current locations, stripping off the filename                         */
    WINE_TRACE("Supplied name: '%s'\n", wine_dbgstr_w(name));
    GetFullPathNameW(name, sizeof(curdir)/sizeof(WCHAR), curdir, &namepart);
    WINE_TRACE("Result: '%s'\n", wine_dbgstr_w(curdir));
    strcpyW(name, curdir);
    if (namepart) *namepart = 0x00;

    /* Search for files based on the location and filespec supplied */
    hff = FindFirstFileW(name, &fd);
    if (hff == INVALID_HANDLE_VALUE) {
        ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_FILENOTFOUND), name);
    }
    else {
        do {
            const WCHAR dot[] = {'.', 0};
            const WCHAR dotdot[] = {'.', '.', 0};

            if (!strcmpW(fd.cFileName, dot) || !strcmpW(fd.cFileName, dotdot))
                continue;

            if (attrib_set || attrib_clear) {
                fd.dwFileAttributes &= ~attrib_clear;
                fd.dwFileAttributes |= attrib_set;
                if (!fd.dwFileAttributes)
                    fd.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
                strcpyW(name, curdir);
                strcatW(name, fd.cFileName);
                SetFileAttributesW(name, fd.dwFileAttributes);
            } else {
                static const WCHAR fmt[] = {'%','1',' ',' ',' ',' ',' ','%','2','\n','\0'};
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) {
                    flags[4] = 'H';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) {
                    flags[1] = 'S';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE) {
                    flags[0] = 'A';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_READONLY) {
                    flags[5] = 'R';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_TEMPORARY) {
                    flags[6] = 'T';
                }
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_COMPRESSED) {
                    flags[7] = 'C';
                }
                strcpyW(name, curdir);
                strcatW(name, fd.cFileName);
                ATTRIB_wprintf(fmt, flags, name);
                for (count = 0; count < (sizeof(flags)/sizeof(WCHAR) - 1); count++) flags[count] = ' ';
            }
        } while (FindNextFileW(hff, &fd) != 0);
    }
    FindClose (hff);

    return 0;
}
