/*
 * Start a program using ShellExecuteEx, optionally wait for it to finish
 * Compatible with Microsoft's "c:\windows\command\start.exe"
 *
 * Copyright 2003 Dan Kegel
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

#include "resources.h"

/**
 Output given message to stdout without formatting.
*/
static void output(const char *message)
{
	DWORD count;
	WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), message, strlen(message), &count, NULL);
}

/**
 Output given message,
 followed by ": ",
 followed by description of given GetLastError() value to stdout,
 followed by a trailing newline,
 then terminate.
*/
static void fatal_error(const char *msg, DWORD error_code)
{
	LPVOID lpMsgBuf;
	int status;

	output(msg);
	output(": ");
	status = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, 0, (LPTSTR) & lpMsgBuf, 0, NULL);
	if (!status) {
		output("FormatMessage failed\n");
	} else {
		output(lpMsgBuf);
		LocalFree((HLOCAL) lpMsgBuf);
		output("\n");
	}
	ExitProcess(1);
}

/**
 Output given message from string table,
 followed by ": ",
 followed by description of given GetLastError() value to stdout,
 followed by a trailing newline,
 then terminate.
*/
static void fatal_string_error(int which, DWORD error_code)
{
	char msg[2048];

	if (!LoadString(GetModuleHandle(NULL), which, 
					msg, sizeof(msg)))
		fatal_error("LoadString failed", GetLastError());

	fatal_error(msg, error_code);
}
	
static void fatal_string(int which)
{
	char msg[2048];

	if (!LoadString(GetModuleHandle(NULL), which, 
					msg, sizeof(msg)))
		fatal_error("LoadString failed", GetLastError());

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

static char *build_args( int argc, char **argv )
{
	int i, len = 1;
	char *ret, *p;

	for (i = 0; i < argc; i++ )
	{
		len += strlen(argv[i]) + 1;
		if (strchr(argv[i], ' '))
			len += 2;
	}
	ret = HeapAlloc( GetProcessHeap(), 0, len );
	ret[0] = 0;

	for (i = 0, p = ret; i < argc; i++ )
	{
		if (strchr(argv[i], ' '))
			p += sprintf(p, " \"%s\"", argv[i]);
		else
			p += sprintf(p, " %s", argv[i]);
	}
	return ret;
}

int main(int argc, char *argv[])
{
	SHELLEXECUTEINFO sei;
	char *args;
	int i;

	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.lpVerb = "open";
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
				printf("Option '%s' not recognized\n", argv[i]+ci-1);
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

	if (!ShellExecuteEx(&sei))
	    	fatal_string_error(STRING_EXECFAIL, GetLastError());

	HeapFree( GetProcessHeap(), 0, args );

	if (sei.fMask & SEE_MASK_NOCLOSEPROCESS) {
		DWORD exitcode;
		DWORD waitcode;
		waitcode = WaitForSingleObject(sei.hProcess, INFINITE);
		if (waitcode)
			fatal_error("WaitForSingleObject", GetLastError());
		if (!GetExitCodeProcess(sei.hProcess, &exitcode))
			fatal_error("GetExitCodeProcess", GetLastError());
		/* fixme: haven't tested whether exit code works properly */
		ExitProcess(exitcode);
	}

	ExitProcess(0);
}
