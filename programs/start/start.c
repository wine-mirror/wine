/*
 * Start a program using ShellExecuteEx, optionally wait for it to finish
 * Compatible with Microsoft's "c:\windows\command\start.exe"
 *
 * Copyright 2003 Dan Kegel
 * Copyright 2007 Lyutin Anatoly (Etersoft)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <windows.h>
#include <winuser.h>
#include <stdio.h>
#include <stdlib.h>
#include <shlobj.h>

#include <wine/unicode.h>
#include <wine/debug.h>

#include "resources.h"

WINE_DEFAULT_DEBUG_CHANNEL(start);

/**
 Output given message to stdout without formatting.
*/
static void output(const WCHAR *message)
{
	DWORD count;
	DWORD   res;
	int    wlen = strlenW(message);

	if (!wlen) return;

	res = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), message, wlen, &count, NULL);

	/* If writing to console fails, assume its file
	i/o so convert to OEM codepage and output                  */
	if (!res)
	{
		DWORD len;
		char  *mesA;
		/* Convert to OEM, then output */
		len = WideCharToMultiByte( GetConsoleOutputCP(), 0, message, wlen, NULL, 0, NULL, NULL );
		mesA = HeapAlloc(GetProcessHeap(), 0, len*sizeof(char));
		if (!mesA) return;
		WideCharToMultiByte( GetConsoleOutputCP(), 0, message, wlen, mesA, len, NULL, NULL );
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), mesA, len, &count, FALSE);
		HeapFree(GetProcessHeap(), 0, mesA);
	}
}

/**
 Output given message from string table,
 followed by ": ",
 followed by description of given GetLastError() value to stdout,
 followed by a trailing newline,
 then terminate.
*/

static void fatal_error(const WCHAR *msg, DWORD error_code)
{
    LPVOID lpMsgBuf;
    int status;
    static const WCHAR colonsW[] = { ':', ' ', 0 };
    static const WCHAR newlineW[] = { '\n', 0 };

    output(msg);
    output(colonsW);
    status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, 0, (LPWSTR) & lpMsgBuf, 0, NULL);
    if (!status)
    {
        WINE_ERR("FormatMessage failed\n");
    } else
    {
        output(lpMsgBuf);
        LocalFree((HLOCAL) lpMsgBuf);
        output(newlineW);
    }
    ExitProcess(1);
}

static void fatal_string_error(int which, DWORD error_code)
{
	WCHAR msg[2048];

	if (!LoadStringW(GetModuleHandle(NULL), which,
					msg, sizeof(msg)/sizeof(WCHAR)))
		WINE_ERR("LoadString failed, error %d\n", GetLastError());

	fatal_error(msg, error_code);
}
	
static void fatal_string(int which)
{
	WCHAR msg[2048];

	if (!LoadStringW(GetModuleHandle(NULL), which,
					msg, sizeof(msg)/sizeof(WCHAR)))
		WINE_ERR("LoadString failed, error %d\n", GetLastError());

	output(msg);
	ExitProcess(1);
}

static void usage(void)
{
	fatal_string(STRING_USAGE);
}

static void license(void)
{
	fatal_string(STRING_LICENSE);
}

static WCHAR *build_args( int argc, WCHAR **argvW )
{
	int i, wlen = 1;
	WCHAR *ret, *p;
	static const WCHAR FormatQuotesW[] = { ' ', '\"', '%', 's', '\"', 0 };
	static const WCHAR FormatW[] = { ' ', '%', 's', 0 };

	for (i = 0; i < argc; i++ )
	{
		wlen += strlenW(argvW[i]) + 1;
		if (strchrW(argvW[i], ' '))
			wlen += 2;
	}
	ret = HeapAlloc( GetProcessHeap(), 0, wlen*sizeof(WCHAR) );
	ret[0] = 0;

	for (i = 0, p = ret; i < argc; i++ )
	{
		if (strchrW(argvW[i], ' '))
			p += sprintfW(p, FormatQuotesW, argvW[i]);
		else
			p += sprintfW(p, FormatW, argvW[i]);
	}
	return ret;
}

int wmain (int argc, WCHAR *argv[])
{
	SHELLEXECUTEINFOW sei;
	WCHAR *args = NULL;
	int i;

	static const WCHAR openW[] = { 'o', 'p', 'e', 'n', 0 };

	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.lpVerb = openW;
	sei.nShow = SW_SHOWNORMAL;
	/* Dunno what these mean, but it looks like winMe's start uses them */
	sei.fMask = SEE_MASK_FLAG_DDEWAIT|
	            SEE_MASK_FLAG_NO_UI|
	            SEE_MASK_NO_CONSOLE;

	/* Canonical Microsoft commandline flag processing:
	 * flags start with /, are case insensitive,
	 * and may be run together in same word.
	 */
	for (i=1; i<argc; i++) {
		int ci;

		if (argv[i][0] != '/')
			break;

		/* Handle all options in this word */
		for (ci=0; argv[i][ci]; ) {
			/* Skip slash */
			ci++;
			switch(argv[i][ci]) {
			case 'l':
			case 'L':
				license();
				break;	/* notreached */
			case 'm':
			case 'M':
				if (argv[i][ci+1] == 'a' || argv[i][ci+1] == 'A')
					sei.nShow = SW_SHOWMAXIMIZED;
				else
					sei.nShow = SW_SHOWMINIMIZED;
				break;
			case 'r':
			case 'R':
				/* sei.nShow = SW_SHOWNORMAL; */
				break;
			case 'w':
			case 'W':
				sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
				break;
			default:
				WINE_ERR("Option '%s' not recognized\n", wine_dbgstr_w( argv[i]+ci-1));
				usage();
			}
			/* Skip to next slash */
			while (argv[i][ci] && (argv[i][ci] != '/'))
				ci++;
		}
	}

	if (i == argc)
		usage();

	sei.lpFile = argv[i++];

	args = build_args( argc - i, &argv[i] );
	sei.lpParameters = args;

	if (!ShellExecuteExW(&sei))
	    	fatal_string_error(STRING_EXECFAIL, GetLastError());

	HeapFree( GetProcessHeap(), 0, args );

	if (sei.fMask & SEE_MASK_NOCLOSEPROCESS) {
		DWORD exitcode;
		WaitForSingleObject(sei.hProcess, INFINITE);
		GetExitCodeProcess(sei.hProcess, &exitcode);
		ExitProcess(exitcode);
	}

	ExitProcess(0);
}
