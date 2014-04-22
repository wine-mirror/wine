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

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlobj.h>
#include <shellapi.h>

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

static BOOL is_option(const WCHAR* arg, const WCHAR* opt)
{
    return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          arg, -1, opt, -1) == CSTR_EQUAL;
}

int wmain (int argc, WCHAR *argv[])
{
	SHELLEXECUTEINFOW sei;
	DWORD creation_flags;
	WCHAR *args = NULL;
	int i;
        BOOL unix_mode = FALSE;
        BOOL progid_open = FALSE;
	WCHAR *title = NULL;
	WCHAR *dos_filename = NULL;
	WCHAR *parent_directory = NULL;
	DWORD binary_type;

	static const WCHAR bW[] = { '/', 'b', 0 };
	static const WCHAR minW[] = { '/', 'm', 'i', 'n', 0 };
	static const WCHAR maxW[] = { '/', 'm', 'a', 'x', 0 };
	static const WCHAR lowW[] = { '/', 'l', 'o', 'w', 0 };
	static const WCHAR normalW[] = { '/', 'n', 'o', 'r', 'm', 'a', 'l', 0 };
	static const WCHAR highW[] = { '/', 'h', 'i', 'g', 'h', 0 };
	static const WCHAR realtimeW[] = { '/', 'r', 'e', 'a', 'l', 't', 'i', 'm', 'e', 0 };
	static const WCHAR abovenormalW[] = { '/', 'a', 'b', 'o', 'v', 'e', 'n', 'o', 'r', 'm', 'a', 'l', 0 };
	static const WCHAR belownormalW[] = { '/', 'b', 'e', 'l', 'o', 'w', 'n', 'o', 'r', 'm', 'a', 'l', 0 };
	static const WCHAR separateW[] = { '/', 's', 'e', 'p', 'a', 'r', 'a', 't', 'e', 0 };
	static const WCHAR sharedW[] = { '/', 's', 'h', 'a', 'r', 'e', 'd', 0 };
	static const WCHAR nodeW[] = { '/', 'n', 'o', 'd', 'e', 0 };
	static const WCHAR affinityW[] = { '/', 'a', 'f', 'f', 'i', 'n', 'i', 't', 'y', 0 };
	static const WCHAR wW[] = { '/', 'w', 0 };
	static const WCHAR waitW[] = { '/', 'w', 'a', 'i', 't', 0 };
	static const WCHAR helpW[] = { '/', '?', 0 };
	static const WCHAR unixW[] = { '/', 'u', 'n', 'i', 'x', 0 };
	static const WCHAR progIDOpenW[] =
		{ '/', 'p', 'r', 'o', 'g', 'I', 'D', 'O', 'p', 'e', 'n', 0};
	static const WCHAR openW[] = { 'o', 'p', 'e', 'n', 0 };
	static const WCHAR cmdW[] = { 'c', 'm', 'd', '.', 'e', 'x', 'e', 0 };

	memset(&sei, 0, sizeof(sei));
	sei.cbSize = sizeof(sei);
	sei.lpVerb = openW;
	sei.nShow = SW_SHOWNORMAL;
	/* Dunno what these mean, but it looks like winMe's start uses them */
	sei.fMask = SEE_MASK_FLAG_DDEWAIT|
	            SEE_MASK_FLAG_NO_UI;
        sei.lpDirectory = NULL;
        creation_flags = CREATE_NEW_CONSOLE;

	/* Canonical Microsoft commandline flag processing:
	 * flags start with / and are case insensitive.
	 */
	for (i=1; i<argc; i++) {
		if (argv[i][0] == '"') {
			title = argv[i];
			continue;
		}
		if (argv[i][0] != '/')
			break;

		/* Unix paths can start with / so we have to assume anything following /unix is not a flag */
		if (unix_mode || progid_open)
			break;

		if (argv[i][0] == '/' && (argv[i][1] == 'd' || argv[i][1] == 'D')) {
			if (argv[i][2])
				/* The start directory was concatenated to the option */
				sei.lpDirectory = argv[i]+2;
			else if (i+1 == argc) {
				WINE_ERR("you must specify a directory path for the /d option\n");
				usage();
			} else
				sei.lpDirectory = argv[++i];
		}
		else if (is_option(argv[i], bW)) {
			creation_flags &= ~CREATE_NEW_CONSOLE;
		}
		else if (argv[i][0] == '/' && (argv[i][1] == 'i' || argv[i][1] == 'I')) {
                    TRACE("/i is ignored\n"); /* FIXME */
		}
		else if (is_option(argv[i], minW)) {
			sei.nShow = SW_SHOWMINIMIZED;
		}
		else if (is_option(argv[i], maxW)) {
			sei.nShow = SW_SHOWMAXIMIZED;
		}
		else if (is_option(argv[i], lowW)) {
			creation_flags |= IDLE_PRIORITY_CLASS;
		}
		else if (is_option(argv[i], normalW)) {
			creation_flags |= NORMAL_PRIORITY_CLASS;
		}
		else if (is_option(argv[i], highW)) {
			creation_flags |= HIGH_PRIORITY_CLASS;
		}
		else if (is_option(argv[i], realtimeW)) {
			creation_flags |= REALTIME_PRIORITY_CLASS;
		}
		else if (is_option(argv[i], abovenormalW)) {
			creation_flags |= ABOVE_NORMAL_PRIORITY_CLASS;
		}
		else if (is_option(argv[i], belownormalW)) {
			creation_flags |= BELOW_NORMAL_PRIORITY_CLASS;
		}
		else if (is_option(argv[i], separateW)) {
			TRACE("/separate is ignored\n"); /* FIXME */
		}
		else if (is_option(argv[i], sharedW)) {
			TRACE("/shared is ignored\n"); /* FIXME */
		}
		else if (is_option(argv[i], nodeW)) {
			if (i+1 == argc) {
				WINE_ERR("you must specify a numa node for the /node option\n");
				usage();
			} else
			{
				TRACE("/node is ignored\n"); /* FIXME */
				i++;
			}
		}
		else if (is_option(argv[i], affinityW))
		{
			if (i+1 == argc) {
				WINE_ERR("you must specify a numa node for the /node option\n");
				usage();
			} else
			{
				TRACE("/affinity is ignored\n"); /* FIXME */
				i++;
			}
		}
		else if (is_option(argv[i], wW) || is_option(argv[i], waitW)) {
			sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
		}
		else if (is_option(argv[i], helpW)) {
			usage();
		}

		/* Wine extensions */

		else if (is_option(argv[i], unixW)) {
                        unix_mode = TRUE;
		}
		else if (is_option(argv[i], progIDOpenW)) {
                        progid_open = TRUE;
		} else

		{
			WINE_ERR("Unknown option '%s'\n", wine_dbgstr_w(argv[i]));
			usage();
		}
	}

	if (progid_open) {
		if (i == argc)
			usage();
		sei.lpClass = argv[i++];
		sei.fMask |= SEE_MASK_CLASSNAME;
	}

	if (i == argc) {
		if (progid_open || unix_mode)
			usage();
		sei.lpFile = cmdW;
	}
	else
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
		if (!sei.lpDirectory)
			sei.lpDirectory = parent_directory = get_parent_dir(dos_filename);
		sei.fMask &= ~SEE_MASK_FLAG_NO_UI;

                if (GetBinaryTypeW(sei.lpFile, &binary_type)) {
                    WCHAR *commandline;
                    STARTUPINFOW startup_info;
                    PROCESS_INFORMATION process_information;
                    static const WCHAR commandlineformat[] = {'"','%','s','"','%','s',0};

                    /* explorer on windows always quotes the filename when running a binary on windows (see bug 5224) so we have to use CreateProcessW in this case */

                    commandline = HeapAlloc(GetProcessHeap(), 0, (strlenW(sei.lpFile)+3+strlenW(sei.lpParameters))*sizeof(WCHAR));
                    sprintfW(commandline, commandlineformat, sei.lpFile, sei.lpParameters);

                    ZeroMemory(&startup_info, sizeof(startup_info));
                    startup_info.cb = sizeof(startup_info);
                    startup_info.lpTitle = title;

                    if (!CreateProcessW(
                            NULL, /* lpApplicationName */
                            commandline, /* lpCommandLine */
                            NULL, /* lpProcessAttributes */
                            NULL, /* lpThreadAttributes */
                            FALSE, /* bInheritHandles */
                            creation_flags, /* dwCreationFlags */
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
