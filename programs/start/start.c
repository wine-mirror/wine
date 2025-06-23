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

#include <wine/debug.h>

#include "resources.h"

WINE_DEFAULT_DEBUG_CHANNEL(start);

/**
 Output given message to stdout without formatting.
*/
static void output(const WCHAR *message)
{
	DWORD count;
	int    wlen = lstrlenW(message);

	if (!wlen) return;

	/* If writing to console fails, assume it's file
         * i/o so convert to OEM codepage and output
         */
	if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), message, wlen, &count, NULL))
	{
		DWORD len;
		char  *mesA;
		/* Convert to OEM, then output */
		len = WideCharToMultiByte( GetOEMCP(), 0, message, wlen, NULL, 0, NULL, NULL );
		mesA = malloc(len);
		if (!mesA) return;
		WideCharToMultiByte( GetOEMCP(), 0, message, wlen, mesA, len, NULL, NULL );
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), mesA, len, &count, FALSE);
		free(mesA);
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

    output(msg);
    output(L": ");
    args[0] = (DWORD_PTR)filename;
    status = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ARGUMENT_ARRAY,
                            NULL, error_code, 0, (LPWSTR)&lpMsgBuf, 0, (va_list *)args );
    if (!status)
    {
        WINE_ERR("FormatMessage failed\n");
    } else
    {
        output(lpMsgBuf);
        LocalFree((HLOCAL) lpMsgBuf);
        output(L"\n");
    }
    ExitProcess(1);
}

static void fatal_string_error(int which, DWORD error_code, const WCHAR *filename)
{
	WCHAR msg[2048];

	if (!LoadStringW(GetModuleHandleW(NULL), which, msg, ARRAY_SIZE(msg)))
		WINE_ERR("LoadString failed, error %ld\n", GetLastError());

	fatal_error(msg, error_code, filename);
}

static void fatal_string(int which)
{
	WCHAR msg[2048];

	if (!LoadStringW(GetModuleHandleW(NULL), which, msg, ARRAY_SIZE(msg)))
		WINE_ERR("LoadString failed, error %ld\n", GetLastError());

	output(msg);
	ExitProcess(1);
}

static void usage(void)
{
	fatal_string(STRING_USAGE);
}

/***********************************************************************
 *           build_command_line
 *
 * Build the command line of a process from the argv array.
 *
 * We must quote and escape characters so that the argv array can be rebuilt
 * from the command line:
 * - spaces and tabs must be quoted
 *   'a b'   -> '"a b"'
 * - quotes must be escaped
 *   '"'     -> '\"'
 * - if '\'s are followed by a '"', they must be doubled and followed by '\"',
 *   resulting in an odd number of '\' followed by a '"'
 *   '\"'    -> '\\\"'
 *   '\\"'   -> '\\\\\"'
 * - '\'s are followed by the closing '"' must be doubled,
 *   resulting in an even number of '\' followed by a '"'
 *   ' \'    -> '" \\"'
 *   ' \\'    -> '" \\\\"'
 * - '\'s that are not followed by a '"' can be left as is
 *   'a\b'   == 'a\b'
 *   'a\\b'  == 'a\\b'
 */
static WCHAR *build_command_line( WCHAR **wargv )
{
    int len;
    WCHAR **arg, *ret;
    LPWSTR p;

    len = 1;
    for (arg = wargv; *arg; arg++) len += 3 + 2 * wcslen( *arg );
    if (!(ret = malloc( len * sizeof(WCHAR) ))) return NULL;

    p = ret;
    for (arg = wargv; *arg; arg++)
    {
        BOOL has_space, has_quote;
        int i, bcount;
        WCHAR *a;

        /* check for quotes and spaces in this argument */
        has_space = !**arg || wcschr( *arg, ' ' ) || wcschr( *arg, '\t' );
        has_quote = wcschr( *arg, '"' ) != NULL;

        /* now transfer it to the command line */
        if (has_space) *p++ = '"';
        if (has_quote || has_space)
        {
            bcount = 0;
            for (a = *arg; *a; a++)
            {
                if (*a == '\\') bcount++;
                else
                {
                    if (*a == '"') /* double all the '\\' preceding this '"', plus one */
                        for (i = 0; i <= bcount; i++) *p++ = '\\';
                    bcount = 0;
                }
                *p++ = *a;
            }
        }
        else
        {
            wcscpy( p, *arg );
            p += wcslen( p );
        }
        if (has_space)
        {
            /* Double all the '\' preceding the closing quote */
            for (i = 0; i < bcount; i++) *p++ = '\\';
            *p++ = '"';
        }
        *p++ = ' ';
    }
    if (p > ret) p--;  /* remove last space */
    *p = 0;
    return ret;
}

static WCHAR *get_parent_dir(const WCHAR* path)
{
	WCHAR *last_slash;
	WCHAR *result;
	int len;

	last_slash = wcsrchr( path, '\\' );
	if (last_slash == NULL)
		len = 1;
	else
		len = last_slash - path + 1;

	result = malloc( len * sizeof(WCHAR) );
	CopyMemory(result, path, (len-1)*sizeof(WCHAR));
	result[len-1] = '\0';

	return result;
}

static BOOL is_option(const WCHAR* arg, const WCHAR* opt)
{
    return CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE,
                          arg, -1, opt, -1) == CSTR_EQUAL;
}

static BOOL is_option_with_arg(WCHAR **argv, int *pos, const WCHAR* opt)
{
    if (!is_option( argv[*pos], opt )) return FALSE;
    (*pos)++;
    if (!argv[*pos])
    {
        WINE_ERR("missing argument for %s\n", debugstr_w(opt));
        usage();
    }
    return TRUE;
}

static WCHAR *parse_title(const WCHAR *arg)
{
    /* See:
     *     WCMD_start() in programs/cmd/builtins.c
     *     WCMD_parameter_with_delims() in programs/cmd/batch.c
     * The shell has already tokenized the command line for us.
     * All we need to do is filter out all the quotes.
     */
    int next;
    const WCHAR *p = arg;
    WCHAR *title = malloc( wcslen(arg) * sizeof(WCHAR) );

    for (next = 0; *p; p++) if (*p != '"') title[next++] = *p;
    title[next] = 0;
    return title;
}

static WCHAR *get_dos_filename( const WCHAR *unix_file )
{
    HANDLE handle;
    DWORD len, size = 8 + wcslen( unix_file ) + 1;
    WCHAR *path = malloc( size * sizeof(WCHAR) );

    wcscpy( path, L"\\\\?\\unix" );
    wcscat( path, unix_file );
    handle = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                          NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0 );
    if (handle == INVALID_HANDLE_VALUE) fatal_string( STRING_UNIXFAIL );
    len = GetFinalPathNameByHandleW( handle, path, size, VOLUME_NAME_DOS );
    CloseHandle( handle );
    if (len > 6 && len <= size && path[5] == ':') path += 4;
    return path;
}

static BOOL search_path(const WCHAR *firstParam, WCHAR **full_path)
{
    /* Copied from WCMD_run_program() in programs/cmd/wcmdmain.c */

#define MAXSTRING 8192

    WCHAR  temp[MAX_PATH];
    WCHAR  pathtosearch[MAXSTRING];
    WCHAR *pathposn;
    WCHAR  stemofsearch[MAX_PATH];    /* maximum allowed executable name is
                                         MAX_PATH, including null character */
    WCHAR *lastSlash;
    WCHAR  pathext[MAXSTRING];
    BOOL  extensionsupplied = FALSE;
    DWORD len;

    /* Calculate the search path and stem to search for */
    if (wcspbrk (firstParam, L"/\\:") == NULL) {  /* No explicit path given, search path */
        lstrcpyW(pathtosearch, L".;");
        len = GetEnvironmentVariableW(L"PATH", &pathtosearch[2], ARRAY_SIZE(pathtosearch)-2);
        if ((len == 0) || (len >= ARRAY_SIZE(pathtosearch) - 2)) {
            lstrcpyW (pathtosearch, L".");
        }
        if (wcschr(firstParam, '.') != NULL) extensionsupplied = TRUE;
        if (lstrlenW(firstParam) >= MAX_PATH) {
            return FALSE;
        }

        lstrcpyW(stemofsearch, firstParam);

    } else {

        /* Convert eg. ..\fred to include a directory by removing file part */
        GetFullPathNameW(firstParam, ARRAY_SIZE(pathtosearch), pathtosearch, NULL);
        lastSlash = wcsrchr(pathtosearch, '\\');
        if (lastSlash && wcschr(lastSlash, '.') != NULL) extensionsupplied = TRUE;
        lstrcpyW(stemofsearch, lastSlash+1);

        /* Reduce pathtosearch to a path with trailing '\' to support c:\a.bat and
           c:\windows\a.bat syntax                                                 */
        if (lastSlash) *(lastSlash + 1) = 0x00;
    }

    /* Now extract PATHEXT */
    len = GetEnvironmentVariableW(L"PATHEXT", pathext, ARRAY_SIZE(pathext));
    if ((len == 0) || (len >= ARRAY_SIZE(pathext))) {
        lstrcpyW (pathext, L".bat;.com;.cmd;.exe");
    }

    /* Loop through the search path, dir by dir */
    pathposn = pathtosearch;
    WINE_TRACE("Searching in '%s' for '%s'\n", wine_dbgstr_w(pathtosearch),
               wine_dbgstr_w(stemofsearch));
    while (pathposn) {
        WCHAR  thisDir[MAX_PATH] = {'\0'};
        int    length            = 0;
        WCHAR *pos               = NULL;
        BOOL  found             = FALSE;
        BOOL inside_quotes      = FALSE;

        /* Work on the first directory on the search path */
        pos = pathposn;
        while ((inside_quotes || *pos != ';') && *pos != 0)
        {
            if (*pos == '"')
                inside_quotes = !inside_quotes;
            pos++;
        }

        if (*pos) { /* Reached semicolon */
            memcpy(thisDir, pathposn, (pos-pathposn) * sizeof(WCHAR));
            thisDir[(pos-pathposn)] = 0x00;
            pathposn = pos+1;
        } else {    /* Reached string end */
            lstrcpyW(thisDir, pathposn);
            pathposn = NULL;
        }

        /* Remove quotes */
        length = lstrlenW(thisDir);
        if (thisDir[length - 1] == '"')
            thisDir[length - 1] = 0;

        if (*thisDir != '"')
            lstrcpyW(temp, thisDir);
        else
            lstrcpyW(temp, thisDir + 1);

        /* Since you can have eg. ..\.. on the path, need to expand
           to full information                                      */
        GetFullPathNameW(temp, MAX_PATH, thisDir, NULL);

        /* 1. If extension supplied, see if that file exists */
        if (thisDir[lstrlenW(thisDir) - 1] != '\\') lstrcatW(thisDir, L"\\");
        lstrcatW(thisDir, stemofsearch);
        pos = &thisDir[lstrlenW(thisDir)]; /* Pos = end of name */

        /* 1. If extension supplied, see if that file exists */
        if (extensionsupplied) {
            if (GetFileAttributesW(thisDir) != INVALID_FILE_ATTRIBUTES) {
               found = TRUE;
            }
        }

        /* 2. Any .* matches? */
        if (!found) {
            HANDLE          h;
            WIN32_FIND_DATAW finddata;

            lstrcatW(thisDir, L".*");
            h = FindFirstFileW(thisDir, &finddata);
            FindClose(h);
            if (h != INVALID_HANDLE_VALUE) {

                WCHAR *thisExt = pathext;

                /* 3. Yes - Try each path ext */
                while (thisExt) {
                    WCHAR *nextExt = wcschr(thisExt, ';');

                    if (nextExt) {
                        memcpy(pos, thisExt, (nextExt-thisExt) * sizeof(WCHAR));
                        pos[(nextExt-thisExt)] = 0x00;
                        thisExt = nextExt+1;
                    } else {
                        lstrcpyW(pos, thisExt);
                        thisExt = NULL;
                    }

                    if (GetFileAttributesW(thisDir) != INVALID_FILE_ATTRIBUTES) {
                        found = TRUE;
                        thisExt = NULL;
                    }
                }
            }
        }

        if (found) {
            *full_path = wcsdup(thisDir);
            if (!*full_path) fatal_string_error(STRING_EXECFAIL, ERROR_OUTOFMEMORY, firstParam);
            return TRUE;
        }
    }
    return FALSE;
}

static struct
{
    SHELLEXECUTEINFOW sei;
    WCHAR            *title;
    DWORD             creation_flags;
    USHORT            machine;
    BOOL              cp_inherit;
} opts;

static void parse_command_line( int argc, WCHAR *argv[] )
{
    int i;
    const WCHAR *file = NULL;
    BOOL unix_mode = FALSE;
    BOOL progid_open = FALSE;

    opts.sei.cbSize = sizeof(opts.sei);
    opts.sei.lpVerb = L"open";
    opts.sei.nShow = SW_SHOWNORMAL;
    /* Dunno what these mean, but it looks like winMe's start uses them */
    opts.sei.fMask = SEE_MASK_FLAG_DDEWAIT | SEE_MASK_FLAG_NO_UI;
    opts.creation_flags = CREATE_NEW_CONSOLE;
    opts.cp_inherit = FALSE;

    /* Canonical Microsoft commandline flag processing:
     * flags start with / and are case insensitive.
     */
    for (i = 1; i < argc; i++)
    {
        /* parse first quoted argument as console title */
        if (!opts.title && argv[i][0] == '"') {
            opts.title = parse_title(argv[i]);
            continue;
        }
        if (argv[i][0] != '/')
            break;

        if (argv[i][1] == 'd' || argv[i][1] == 'D') {
            if (argv[i][2])
                /* The start directory was concatenated to the option */
                opts.sei.lpDirectory = argv[i]+2;
            else if (i+1 == argc) {
                WINE_ERR("you must specify a directory path for the /d option\n");
                usage();
            } else
                opts.sei.lpDirectory = argv[++i];
        }
        else if (is_option(argv[i], L"/b"))
            opts.creation_flags &= ~CREATE_NEW_CONSOLE;
        else if (argv[i][0] == '/' && (argv[i][1] == 'i' || argv[i][1] == 'I'))
            TRACE("/i is ignored\n"); /* FIXME */
        else if (is_option(argv[i], L"/min"))
            opts.sei.nShow = SW_SHOWMINIMIZED;
        else if (is_option(argv[i], L"/max"))
            opts.sei.nShow = SW_SHOWMAXIMIZED;
        else if (is_option(argv[i], L"/low"))
            opts.creation_flags |= IDLE_PRIORITY_CLASS;
        else if (is_option(argv[i], L"/normal"))
            opts.creation_flags |= NORMAL_PRIORITY_CLASS;
        else if (is_option(argv[i], L"/high"))
            opts.creation_flags |= HIGH_PRIORITY_CLASS;
        else if (is_option(argv[i], L"/realtime"))
            opts.creation_flags |= REALTIME_PRIORITY_CLASS;
        else if (is_option(argv[i], L"/abovenormal"))
            opts.creation_flags |= ABOVE_NORMAL_PRIORITY_CLASS;
        else if (is_option(argv[i], L"/belownormal"))
            opts.creation_flags |= BELOW_NORMAL_PRIORITY_CLASS;
        else if (is_option(argv[i], L"/separate"))
            opts.creation_flags |= CREATE_SEPARATE_WOW_VDM;
        else if (is_option(argv[i], L"/shared"))
            opts.creation_flags |= CREATE_SHARED_WOW_VDM;
        else if (is_option(argv[i], L"/w") || is_option(argv[i], L"/wait"))
            opts.sei.fMask |= SEE_MASK_NOCLOSEPROCESS;
        else if (is_option(argv[i], L"/?"))
            usage();
        else if (is_option_with_arg(argv, &i, L"/node"))
            TRACE("/node is ignored\n"); /* FIXME */
        else if (is_option_with_arg(argv, &i, L"/affinity"))
            TRACE("/affinity is ignored\n"); /* FIXME */
        else if (is_option_with_arg(argv, &i, L"/machine"))
        {
            if (is_option( argv[i], L"x86" )) opts.machine = IMAGE_FILE_MACHINE_I386;
            else if (is_option( argv[i], L"amd64" )) opts.machine = IMAGE_FILE_MACHINE_AMD64;
            else if (is_option( argv[i], L"arm" )) opts.machine = IMAGE_FILE_MACHINE_ARMNT;
            else if (is_option( argv[i], L"arm64" )) opts.machine = IMAGE_FILE_MACHINE_ARM64;
            else usage();
        }

        /* Wine extensions */

        else if (is_option_with_arg(argv, &i, L"/unix")) {
            unix_mode = TRUE;
            break;
        }
        else if (is_option(argv[i], L"/exec")) {
            opts.creation_flags = 0;
            opts.sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NO_CONSOLE | SEE_MASK_FLAG_NO_UI;
            opts.cp_inherit = TRUE;
            i++;
            break;
        }
        else if (is_option_with_arg(argv, &i, L"/progIDOpen")) {
            progid_open = TRUE;
            opts.sei.lpClass = argv[i++];
            opts.sei.fMask |= SEE_MASK_CLASSNAME;
            break;
        }
        else
        {
            WINE_ERR("Unknown option '%s'\n", wine_dbgstr_w(argv[i]));
            usage();
        }
    }

    if (i == argc)
    {
        if (progid_open) usage();
        file = L"cmd.exe";
    }
    else file = argv[i++];

    opts.sei.lpParameters = build_command_line( &argv[i] );

    if ((unix_mode || progid_open) && file[0] == '/')
    {
        opts.sei.lpFile = get_dos_filename( file );
        if (!opts.sei.lpDirectory) opts.sei.lpDirectory = get_parent_dir(opts.sei.lpFile);
        opts.sei.fMask &= ~SEE_MASK_FLAG_NO_UI;
    }
    else
    {
	WCHAR *fullpath;

        if (search_path(file, &fullpath)) opts.sei.lpFile = fullpath;
        else opts.sei.lpFile = file;
    }
}


int __cdecl wmain (int argc, WCHAR *argv[])
{
	DWORD binary_type;

        parse_command_line( argc, argv );

        if (GetBinaryTypeW(opts.sei.lpFile, &binary_type)) {
                    WCHAR *commandline;
                    STARTUPINFOEXW si = {{ sizeof(si.StartupInfo) }};
                    PROCESS_INFORMATION process_information;
                    int len = lstrlenW(opts.sei.lpFile) + 4 + lstrlenW(opts.sei.lpParameters);

                    /* explorer on windows always quotes the filename when running a binary on windows (see bug 5224) so we have to use CreateProcessW in this case */

                    if (opts.machine)
                    {
                        SIZE_T size = 1024;
                        struct _PROC_THREAD_ATTRIBUTE_LIST *list = malloc( size );

                        InitializeProcThreadAttributeList( list, 1, 0, &size );
                        UpdateProcThreadAttribute( list, 0, PROC_THREAD_ATTRIBUTE_MACHINE_TYPE, &opts.machine, sizeof(opts.machine), NULL, NULL );
                        si.StartupInfo.cb = sizeof(si);
                        si.lpAttributeList = list;
                        opts.creation_flags |= EXTENDED_STARTUPINFO_PRESENT;
                    }

                    commandline = malloc(len * sizeof(WCHAR));
                    swprintf(commandline, len, L"\"%s\" %s", opts.sei.lpFile, opts.sei.lpParameters);

                    si.StartupInfo.wShowWindow = opts.sei.nShow;
                    si.StartupInfo.dwFlags |= STARTF_USESHOWWINDOW;
                    si.StartupInfo.lpTitle = opts.title;

                    if (!CreateProcessW( opts.sei.lpFile, commandline, NULL, NULL, opts.cp_inherit,
                                         opts.creation_flags, NULL, opts.sei.lpDirectory,
                                         &si.StartupInfo, &process_information ))
                    {
			fatal_string_error(STRING_EXECFAIL, GetLastError(), opts.sei.lpFile);
                    }
                    opts.sei.hProcess = process_information.hProcess;
                    goto done;
        }

        if (!ShellExecuteExW(&opts.sei))
        {
            const WCHAR *filename = opts.sei.lpFile;
            DWORD size, filename_len;
            WCHAR *name, *env;

            size = GetEnvironmentVariableW(L"PATHEXT", NULL, 0);
            if (size)
            {
                WCHAR *start, *ptr;

                env = malloc(size * sizeof(WCHAR));
                if (!env)
                    fatal_string_error(STRING_EXECFAIL, ERROR_OUTOFMEMORY, opts.sei.lpFile);
                GetEnvironmentVariableW(L"PATHEXT", env, size);

                filename_len = lstrlenW(filename);
                name = malloc((filename_len + size) * sizeof(WCHAR));
                if (!name)
                    fatal_string_error(STRING_EXECFAIL, ERROR_OUTOFMEMORY, opts.sei.lpFile);

                opts.sei.lpFile = name;
                start = env;
                while ((ptr = wcschr(start, ';')))
                {
                    if (start == ptr)
                    {
                        start = ptr + 1;
                        continue;
                    }

                    lstrcpyW(name, filename);
                    memcpy(&name[filename_len], start, (ptr - start) * sizeof(WCHAR));
                    name[filename_len + (ptr - start)] = 0;

                    if (ShellExecuteExW(&opts.sei)) goto done;
                    start = ptr + 1;
                }

            }

            fatal_string_error(STRING_EXECFAIL, GetLastError(), filename);
        }

done:
	if (opts.sei.fMask & SEE_MASK_NOCLOSEPROCESS) {
		DWORD exitcode;
		HANDLE hJob;
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;

		SetConsoleCtrlHandler(NULL, TRUE);
		hJob = CreateJobObjectA(NULL, NULL);
		/* Create a job where the child is associated... if the start.exe terminates
		 * before the child, the job will be terminated, and the child will be terminated as well.
		 * (The idea is to allow to kill (from a Unix standpoint) a created Windows
		 * process (here start.exe), and that the unix-kill of start.exe will be also terminate
		 * start.exe's child process).
		 */
		memset(&info, 0, sizeof(info));
		info.BasicLimitInformation.LimitFlags =
                    JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE |
                    JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
		SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &info, sizeof(info));
		AssignProcessToJobObject(hJob, opts.sei.hProcess);

		WaitForSingleObject(opts.sei.hProcess, INFINITE);
		GetExitCodeProcess(opts.sei.hProcess, &exitcode);
		ExitProcess(exitcode);
	}

	ExitProcess(0);
}
