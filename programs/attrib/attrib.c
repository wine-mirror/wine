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
#include "attrib.h"

WINE_DEFAULT_DEBUG_CHANNEL(attrib);

static const WCHAR starW[]  = {'*','\0'};

/* =========================================================================
 * Load a string from the resource file, handling any error
 * Returns string retrieved from resource file
 * ========================================================================= */
static WCHAR *ATTRIB_LoadMessage(UINT id)
{
    static WCHAR msg[MAXSTRING];
    const WCHAR failedMsg[]  = {'F', 'a', 'i', 'l', 'e', 'd', '!', 0};

    if (!LoadStringW(GetModuleHandleW(NULL), id, msg, ARRAY_SIZE(msg))) {
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
static int WINAPIV ATTRIB_wprintf(const WCHAR *format, ...)
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
     * attrib ends
     */

    if (!output_bufW) output_bufW = HeapAlloc(GetProcessHeap(), 0,
                                              MAX_WRITECONSOLE_SIZE*sizeof(WCHAR));
    if (!output_bufW) {
        WINE_FIXME("Out of memory - could not allocate 2 x 64 KB buffers\n");
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

    /* Try to write as unicode all the time we think it's a console */
    if (toConsole) {
        res = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE),
                            output_bufW, len, &nOut, NULL);
    }

    /* If writing to console has failed (ever) we assume it's file
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
          WINE_FIXME("Out of memory - could not allocate 2 x 64 KB buffers\n");
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

/* =========================================================================
 * Handle the processing for a single directory, optionally recursing into
 *  subdirectories if needed.
 * Parameters:
 *  rootdir      [I]   The directory to search in
 *  filespec     [I]   The filespec to search for
 *  recurse      [I]   Whether to recurse (search subdirectories before
 *                          current directory)
 *  includedirs  [I]   Whether to set directory attributes as well
 *  attrib_set   [I]   Attributes to set
 *  attrib_clear [I]   Attributes to clear
 *
 * Returns TRUE if at least one file displayed / modified
 * ========================================================================= */
static BOOL ATTRIB_processdirectory(const WCHAR *rootdir, const WCHAR *filespec,
                                    BOOL recurse, BOOL includedirs,
                                    DWORD attrib_set, DWORD attrib_clear)
{
    BOOL found = FALSE;
    WCHAR buffer[MAX_PATH];
    HANDLE hff;
    WIN32_FIND_DATAW fd;
    WCHAR flags[] = {' ',' ',' ',' ',' ',' ',' ',' ','\0'};
    static const WCHAR slashW[] = {'\\','\0'};

    WINE_TRACE("Processing dir '%s', spec '%s', %d,%x,%x\n",
               wine_dbgstr_w(rootdir), wine_dbgstr_w(filespec),
               recurse, attrib_set, attrib_clear);

    if (recurse) {

      /* Build spec to search for */
      lstrcpyW(buffer, rootdir);
      lstrcatW(buffer, starW);

      /* Search for directories in the location and recurse if necessary */
      WINE_TRACE("Searching for directories with '%s'\n", wine_dbgstr_w(buffer));
      hff = FindFirstFileW(buffer, &fd);
      if (hff != INVALID_HANDLE_VALUE) {
          do {
              const WCHAR dot[] = {'.', 0};
              const WCHAR dotdot[] = {'.', '.', 0};

              /* Only interested in directories, and not . nor .. */
              if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
                  !lstrcmpW(fd.cFileName, dot) || !lstrcmpW(fd.cFileName, dotdot))
                  continue;

              /* Build new root dir to go searching in */
              lstrcpyW(buffer, rootdir);
              lstrcatW(buffer, fd.cFileName);
              lstrcatW(buffer, slashW);
              ATTRIB_processdirectory(buffer, filespec, recurse, includedirs,
                                      attrib_set, attrib_clear);

          } while (FindNextFileW(hff, &fd) != 0);
      }
      FindClose (hff);
    }

    /* Build spec to search for */
    lstrcpyW(buffer, rootdir);
    lstrcatW(buffer, filespec);
    WINE_TRACE("Searching for files as '%s'\n", wine_dbgstr_w(buffer));

    /* Search for files in the location with the filespec supplied */
    hff = FindFirstFileW(buffer, &fd);
    if (hff != INVALID_HANDLE_VALUE) {
        do {
            const WCHAR dot[] = {'.', 0};
            const WCHAR dotdot[] = {'.', '.', 0};
            DWORD count;
            WINE_TRACE("Found '%s'\n", wine_dbgstr_w(fd.cFileName));

            if (!lstrcmpW(fd.cFileName, dot) || !lstrcmpW(fd.cFileName, dotdot))
                continue;

            if (!includedirs && (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
              continue;

            if (attrib_set || attrib_clear) {
                fd.dwFileAttributes &= ~attrib_clear;
                fd.dwFileAttributes |= attrib_set;
                if (!fd.dwFileAttributes)
                    fd.dwFileAttributes |= FILE_ATTRIBUTE_NORMAL;
                lstrcpyW(buffer, rootdir);
                lstrcatW(buffer, fd.cFileName);
                SetFileAttributesW(buffer, fd.dwFileAttributes);
                found = TRUE;
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
                lstrcpyW(buffer, rootdir);
                lstrcatW(buffer, fd.cFileName);
                ATTRIB_wprintf(fmt, flags, buffer);
                for (count = 0; count < (ARRAY_SIZE(flags) - 1); count++) flags[count] = ' ';
                found = TRUE;
            }
        } while (FindNextFileW(hff, &fd) != 0);
    }
    FindClose (hff);
    return found;
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    WCHAR name[MAX_PATH];
    WCHAR *namepart;
    WCHAR curdir[MAX_PATH];
    WCHAR originalname[MAX_PATH];
    DWORD attrib_set = 0;
    DWORD attrib_clear = 0;
    BOOL  attrib_recurse = FALSE;
    BOOL  attrib_includedirs = FALSE;
    static const WCHAR help_option[] = {'/','?','\0'};
    static const WCHAR wildcardsW[] = {'*','?','\0'};
    int i = 1;
    BOOL  found = FALSE;

    if ((argc >= 2) && !lstrcmpW(argv[1], help_option)) {
        ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_HELP));
        return 0;
    }

    /* By default all files from current directory are taken into account */
    lstrcpyW(name, starW);

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
                attrib_includedirs = TRUE;
            } else if (((param[1] == 'S') || (param[1] == 's')) && !param[2]) {
                attrib_recurse = TRUE;
            } else {
                WINE_FIXME("Unknown option %s\n", debugstr_w(param));
            }
        } else if (param[0]) {
            lstrcpyW(originalname, param);
        }
    }

    /* Name may be a relative or explicit path, so calculate curdir based on
       current locations, stripping off the filename                         */
    WINE_TRACE("Supplied name: '%s'\n", wine_dbgstr_w(originalname));
    GetFullPathNameW(originalname, ARRAY_SIZE(curdir), curdir, &namepart);
    WINE_TRACE("Result: '%s'\n", wine_dbgstr_w(curdir));
    if (namepart) {
        lstrcpyW(name, namepart);
        *namepart = 0;
    } else {
        name[0] = 0;
    }

    /* If a directory is explicitly supplied on the command line, and no
       wildcards are in the name, then allow it to be changed/displayed  */
    if (wcspbrk(originalname, wildcardsW) == NULL) attrib_includedirs = TRUE;

    /* Do all the processing based on the filename arg */
    found = ATTRIB_processdirectory(curdir, name, attrib_recurse,
                                    attrib_includedirs, attrib_set, attrib_clear);
    if (!found) {
      ATTRIB_wprintf(ATTRIB_LoadMessage(STRING_FILENOTFOUND), originalname);
    }
    return 0;
}
