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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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

static void usage()
{
	fatal_string(STRING_USAGE);
}

static void license()
{
	fatal_string(STRING_LICENSE);
}

int main(int argc, char *argv[])
{
	char arguments[MAX_PATH];
	char *p;
	SHELLEXECUTEINFO sei;
	int argi;

	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.lpVerb = "open";
	sei.nShow = SW_SHOWNORMAL;
	/* Dunno what these mean, but it looks like winMe's start uses them */
	sei.fMask = SEE_MASK_FLAG_DDEWAIT|SEE_MASK_FLAG_NO_UI;

	/* Canonical Microsoft commandline flag processing:
	 * flags start with /, are case insensitive,
	 * and may be run together in same word.
	 */
	for (argi=1; argi<argc; argi++) {
		int ci;

		if (argv[argi][0] != '/')
			break;

		/* Handle all options in this word */
		for (ci=0; argv[argi][ci]; ) {
			/* Skip slash */
			ci++;
			switch(argv[argi][ci]) {
			case 'l':
			case 'L':
				license();
				break;	/* notreached */
			case 'm':
			case 'M':
				if (argv[argi][ci+1] == 'a' || argv[argi][ci+1] == 'A')
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
				printf("Option '%s' not recognized\n", argv[argi]+ci-1);
				usage();
			}
			/* Skip to next slash */
			while (argv[argi][ci] && (argv[argi][ci] != '/'))
				ci++;
		}
	}

	if (argi == argc)
		usage();

	sei.lpFile = argv[argi++];

	/* FIXME - prone to overflow */
	arguments[0] = 0;
	for (p = arguments; argi < argc; argi++)
		p += sprintf(p, " %s", argv[argi]);

	sei.lpParameters = arguments;

	if (!ShellExecuteEx(&sei))
	    	fatal_string_error(STRING_EXECFAIL, GetLastError());

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
