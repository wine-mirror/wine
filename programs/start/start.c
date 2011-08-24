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

	/* If writing to console fails, assume it's file
         * i/o so convert to OEM codepage and output
         */
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

static void fatal_error(const WCHAR *msg, DWORD error_code, const WCHAR *filename)
{
    DWORD_PTR args[1];
    LPVOID lpMsgBuf;
    int status;
    static const WCHAR colonsW[] = { ':', ' ', 0 };
    static const WCHAR newlineW[] = { '\n', 0 };

    output(msg);
    output(colonsW);
    args[0] = (DWORD_PTR)filename;
    status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            NULL, error_code, 0, (LPWSTR)&lpMsgBuf, 0, (__ms_va_list *)args );
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

static void fatal_string_error(int which, DWORD error_code, const WCHAR *filename)
{
	WCHAR msg[2048];

	if (!LoadStringW(GetModuleHandleW(NULL), which,
					msg, sizeof(msg)/sizeof(WCHAR)))
		WINE_ERR("LoadString failed, error %d\n", GetLastError());

	fatal_error(msg, error_code, filename);
}
	
static void fatal_string(int which)
{
	WCHAR msg[2048];

	if (!LoadStringW(GetModuleHandleW(NULL), which,
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

static WCHAR *get_parent_dir(WCHAR* path)
{
	WCHAR *last_slash;
	WCHAR *result;
	int len;

	last_slash = strrchrW( path, '\\' );
	if (last_slash == NULL)
		len = 1;
	else
		len = last_slash - path + 1;

	result = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
	CopyMemory(result, path, (len-1)*sizeof(WCHAR));
	result[len-1] = '\0';

	return result;
}

int wmain (int argc, WCHAR *argv[])
{
	SHELLEXECUTEINFOW sei;
	WCHAR *args = NULL;
	int i;
	int unix_mode = 0;
	int progid_open = 0;
	WCHAR *dos_filename = NULL;
	WCHAR *parent_directory = NULL;
	DWORD binary_type;

	static const WCHAR openW[] = { 'o', 'p', 'e', 'n', 0 };
	static const WCHAR unixW[] = { 'u', 'n', 'i', 'x', 0 };
	static const WCHAR progIDOpenW[] =
		{ 'p', 'r', 'o', 'g', 'I', 'D', 'O', 'p', 'e', 'n', 0};

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

		/* Unix paths can start with / so we have to assume anything following /U is not a flag */
		if (unix_mode || progid_open)
			break;

		/* Handle all options in this word */
		for (ci=0; argv[i][ci]; ) {
			/* Skip slash */
			ci++;
			switch(argv[i][ci]) {
			case 'b':
			case 'B':
				break; /* FIXME: should stop new window from being created */
			case 'i':
			case 'I':
				break; /* FIXME: should ignore any changes to current environment */
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
			case 'u':
			case 'U':
				if (strncmpiW(&argv[i][ci], unixW, 4) == 0)
					unix_mode = 1;
				else {
					WINE_ERR("Option '%s' not recognized\n", wine_dbgstr_w( argv[i]+ci-1));
					usage();
				}
				break;
			case 'p':
			case 'P':
				if (strncmpiW(&argv[i][ci], progIDOpenW, 17) == 0)
					progid_open = 1;
				else {
					WINE_ERR("Option '%s' not recognized\n", wine_dbgstr_w( argv[i]+ci-1));
					usage();
				}
				break;
			case 'w':
			case 'W':
				sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
				break;
			case '?':
				usage();
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

	if (progid_open) {
		sei.lpClass = argv[i++];
		sei.fMask |= SEE_MASK_CLASSNAME;
	}

	sei.lpFile = argv[i++];

	args = build_args( argc - i, &argv[i] );
	sei.lpParameters = args;

	if (unix_mode || progid_open) {
		LPWSTR (*CDECL wine_get_dos_file_name_ptr)(LPCSTR);
		char* multibyte_unixpath;
		int multibyte_unixpath_len;

		wine_get_dos_file_name_ptr = (void*)GetProcAddress(GetModuleHandleA("KERNEL32"), "wine_get_dos_file_name");

		if (!wine_get_dos_file_name_ptr)
			fatal_string(STRING_UNIXFAIL);

		multibyte_unixpath_len = WideCharToMultiByte(CP_UNIXCP, 0, sei.lpFile, -1, NULL, 0, NULL, NULL);
		multibyte_unixpath = HeapAlloc(GetProcessHeap(), 0, multibyte_unixpath_len);

		WideCharToMultiByte(CP_UNIXCP, 0, sei.lpFile, -1, multibyte_unixpath, multibyte_unixpath_len, NULL, NULL);

		dos_filename = wine_get_dos_file_name_ptr(multibyte_unixpath);

		HeapFree(GetProcessHeap(), 0, multibyte_unixpath);

		if (!dos_filename)
			fatal_string(STRING_UNIXFAIL);

		sei.lpFile = dos_filename;
		sei.lpDirectory = parent_directory = get_parent_dir(dos_filename);
		sei.fMask &= ~SEE_MASK_FLAG_NO_UI;

                if (GetBinaryTypeW(sei.lpFile, &binary_type)) {
                    WCHAR *commandline;
                    STARTUPINFOW startup_info;
                    PROCESS_INFORMATION process_information;
                    static WCHAR commandlineformat[] = {'"','%','s','"','%','s',0};

                    /* explorer on windows always quotes the filename when running a binary on windows (see bug 5224) so we have to use CreateProcessW in this case */

                    commandline = HeapAlloc(GetProcessHeap(), 0, (strlenW(sei.lpFile)+3+strlenW(sei.lpParameters))*sizeof(WCHAR));
                    sprintfW(commandline, commandlineformat, sei.lpFile, sei.lpParameters);

                    ZeroMemory(&startup_info, sizeof(startup_info));
                    startup_info.cb = sizeof(startup_info);

                    if (!CreateProcessW(
                            NULL, /* lpApplicationName */
                            commandline, /* lpCommandLine */
                            NULL, /* lpProcessAttributes */
                            NULL, /* lpThreadAttributes */
                            FALSE, /* bInheritHandles */
                            CREATE_NEW_CONSOLE, /* dwCreationFlags */
                            NULL, /* lpEnvironment */
                            sei.lpDirectory, /* lpCurrentDirectory */
                            &startup_info, /* lpStartupInfo */
                            &process_information /* lpProcessInformation */ ))
                    {
			fatal_string_error(STRING_EXECFAIL, GetLastError(), sei.lpFile);
                    }
                    sei.hProcess = process_information.hProcess;
                    goto done;
                }
	}

        if (!ShellExecuteExW(&sei))
            fatal_string_error(STRING_EXECFAIL, GetLastError(), sei.lpFile);

done:
	HeapFree( GetProcessHeap(), 0, args );
	HeapFree( GetProcessHeap(), 0, dos_filename );
	HeapFree( GetProcessHeap(), 0, parent_directory );

	if (sei.fMask & SEE_MASK_NOCLOSEPROCESS) {
		DWORD exitcode;
		WaitForSingleObject(sei.hProcess, INFINITE);
		GetExitCodeProcess(sei.hProcess, &exitcode);
		ExitProcess(exitcode);
	}

	ExitProcess(0);
}
