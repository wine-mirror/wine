/*
 * PURPOSE: Register OLE components in the registry
 *
 * Copyright 2001 ReactOS project
 * Copyright 2001 Jurgen Van Gael [jurgen.vangael@student.kuleuven.ac.be]
 * Copyright 2002 Andriy Palamarchuk
 * Copyright 2014, 2015 Hugh McMaster
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ole2.h>
#include "regsvr32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(regsvr32);

typedef HRESULT (WINAPI *DLLREGISTER)   (void);
typedef HRESULT (WINAPI *DLLUNREGISTER) (void);
typedef HRESULT (WINAPI *DLLINSTALL)    (BOOL,LPCWSTR);

static BOOL Silent = FALSE;

static void WINAPIV output_write(UINT id, ...)
{
    WCHAR fmt[1024];
    __ms_va_list va_args;
    WCHAR *str;
    DWORD len, nOut, ret;

    if (Silent) return;

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        return;
    }

    __ms_va_start(va_args, id);
    SetLastError(NO_ERROR);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (LPWSTR)&str, 0, &va_args);
    __ms_va_end(va_args);
    if (len == 0 && GetLastError() != NO_ERROR)
    {
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        return;
    }

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), str, len, &nOut, NULL);

    /* WriteConsole fails if its output is redirected to a file.
     * If this occurs, we should use an OEM codepage and call WriteFile.
     */
    if (!ret)
    {
        DWORD lenA;
        char *strA;

        lenA = WideCharToMultiByte(GetConsoleOutputCP(), 0, str, len, NULL, 0, NULL, NULL);
        strA = HeapAlloc(GetProcessHeap(), 0, lenA);
        if (strA)
        {
            WideCharToMultiByte(GetConsoleOutputCP(), 0, str, len, strA, lenA, NULL, NULL);
            WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), strA, lenA, &nOut, FALSE);
            HeapFree(GetProcessHeap(), 0, strA);
        }
    }
    LocalFree(str);
}

static LPCWSTR find_arg_start(LPCWSTR cmdline)
{
    LPCWSTR s;
    BOOL in_quotes;
    int bcount;

    bcount=0;
    in_quotes=FALSE;
    s=cmdline;
    while (1) {
        if (*s==0 || ((*s=='\t' || *s==' ') && !in_quotes)) {
            /* end of this command line argument */
            break;
        } else if (*s=='\\') {
            /* '\', count them */
            bcount++;
        } else if ((*s=='"') && ((bcount & 1)==0)) {
            /* unescaped '"' */
            in_quotes=!in_quotes;
            bcount=0;
        } else {
            /* a regular character */
            bcount=0;
        }
        s++;
    }
    return s;
}

static void reexec_self(void)
{
    /* restart current process as 32-bit or 64-bit with same command line */
    static const WCHAR exe_name[] = {'\\','r','e','g','s','v','r','3','2','.','e','x','e',0};
#ifndef _WIN64
    static const WCHAR sysnative[] = {'\\','S','y','s','N','a','t','i','v','e',0};
    BOOL wow64;
#endif
    WCHAR systemdir[MAX_PATH];
    LPCWSTR args;
    WCHAR *cmdline;
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;

#ifdef _WIN64
    TRACE("restarting as 32-bit\n");
    GetSystemWow64DirectoryW(systemdir, MAX_PATH);
#else
    TRACE("restarting as 64-bit\n");

    if (!IsWow64Process(GetCurrentProcess(), &wow64) || !wow64)
    {
        TRACE("not running in wow64, can't restart as 64-bit\n");
        return;
    }

    GetWindowsDirectoryW(systemdir, MAX_PATH);
    wcscat(systemdir, sysnative);
#endif

    args = find_arg_start(GetCommandLineW());

    cmdline = HeapAlloc(GetProcessHeap(), 0,
        (wcslen(systemdir)+wcslen(exe_name)+wcslen(args)+1)*sizeof(WCHAR));

    wcscpy(cmdline, systemdir);
    wcscat(cmdline, exe_name);
    wcscat(cmdline, args);

    si.cb = sizeof(si);

    if (CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        DWORD exit_code;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        ExitProcess(exit_code);
    }
    else
    {
        WINE_TRACE("failed to restart, err=%d\n", GetLastError());
    }

    HeapFree(GetProcessHeap(), 0, cmdline);
}

#ifdef _WIN64
# define ALT_BINARY_TYPE SCS_32BIT_BINARY
#else
# define ALT_BINARY_TYPE SCS_64BIT_BINARY
#endif

/**
 * Loads procedure.
 *
 * Parameters:
 * strDll - name of the dll.
 * procName - name of the procedure to load from the dll.
 * DllHandle - a variable that receives the handle of the loaded dll.
 * firstDll - true if this is the first dll in the command line.
 */
static VOID *LoadProc(const WCHAR* strDll, const char* procName, HMODULE* DllHandle, BOOL firstDll)
{
    VOID* (*proc)(void);

    *DllHandle = LoadLibraryExW(strDll, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!*DllHandle)
    {
        DWORD binary_type;
        if (firstDll && GetLastError() == ERROR_BAD_EXE_FORMAT &&
            GetBinaryTypeW(strDll, &binary_type) &&
            binary_type == ALT_BINARY_TYPE)
        {
            reexec_self();
        }
        output_write(STRING_DLL_LOAD_FAILED, strDll);
        ExitProcess(LOADLIBRARY_FAILED);
    }
    proc = (VOID *) GetProcAddress(*DllHandle, procName);
    if(!proc)
    {
        output_write(STRING_PROC_NOT_IMPLEMENTED, procName, strDll);
        FreeLibrary(*DllHandle);
        return NULL;
    }
    return proc;
}

static int RegisterDll(const WCHAR* strDll, BOOL firstDll)
{
    HRESULT hr;
    DLLREGISTER pfRegister;
    HMODULE DllHandle = NULL;

    pfRegister = LoadProc(strDll, "DllRegisterServer", &DllHandle, firstDll);
    if (!pfRegister)
        return GETPROCADDRESS_FAILED;

    hr = pfRegister();
    if(FAILED(hr))
    {
        output_write(STRING_REGISTER_FAILED, strDll);
        return DLLSERVER_FAILED;
    }
    output_write(STRING_REGISTER_SUCCESSFUL, strDll);

    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}

static int UnregisterDll(const WCHAR* strDll, BOOL firstDll)
{
    HRESULT hr;
    DLLUNREGISTER pfUnregister;
    HMODULE DllHandle = NULL;

    pfUnregister = LoadProc(strDll, "DllUnregisterServer", &DllHandle, firstDll);
    if (!pfUnregister)
        return GETPROCADDRESS_FAILED;

    hr = pfUnregister();
    if(FAILED(hr))
    {
        output_write(STRING_UNREGISTER_FAILED, strDll);
        return DLLSERVER_FAILED;
    }
    output_write(STRING_UNREGISTER_SUCCESSFUL, strDll);

    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}

static int InstallDll(BOOL install, const WCHAR *strDll, const WCHAR *command_line, BOOL firstDll)
{
    HRESULT hr;
    DLLINSTALL pfInstall;
    HMODULE DllHandle = NULL;

    pfInstall = LoadProc(strDll, "DllInstall", &DllHandle, firstDll);
    if (!pfInstall)
        return GETPROCADDRESS_FAILED;

    hr = pfInstall(install, command_line);
    if(FAILED(hr))
    {
        if (install)
            output_write(STRING_INSTALL_FAILED, strDll);
        else
            output_write(STRING_UNINSTALL_FAILED, strDll);
        return DLLSERVER_FAILED;
    }
    if (install)
        output_write(STRING_INSTALL_SUCCESSFUL, strDll);
    else
        output_write(STRING_UNINSTALL_SUCCESSFUL, strDll);

    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}

static WCHAR *parse_command_line(WCHAR *command_line)
{
    if (command_line[0] == ':' && command_line[1])
    {
        int len = lstrlenW(command_line);

        command_line++;
        len--;
        /* remove double quotes */
        if (command_line[0] == '"')
        {
            command_line++;
            len--;
            if (command_line[0])
            {
                len--;
                command_line[len] = 0;
            }
        }
        if (command_line[0])
            return command_line;
    }
    return NULL;
}

int __cdecl wmain(int argc, WCHAR* argv[])
{
    int             i, res, ret = 0;
    BOOL            CallRegister = TRUE;
    BOOL            CallInstall = FALSE;
    BOOL            Unregister = FALSE;
    BOOL            DllFound = FALSE;
    WCHAR*          wsCommandLine = NULL;
    WCHAR           EmptyLine[1] = {0};

    OleInitialize(NULL);

    /* We mirror the Microsoft version by processing all of the flags before
     * the files (e.g. regsvr32 file1 /s file2 is silent even for file1).
     *
     * Note the complication that this version may be passed Unix format filenames
     * which could be mistaken for flags. The Windows version conveniently
     * requires each flag to be separate (e.g. no /su), so we will simply
     * assume that anything longer than /. is a filename.
     */
    for(i = 1; i < argc; i++)
    {
        if (argv[i][0] == '/' || argv[i][0] == '-')
        {
            if (!argv[i][1])
                return INVALID_ARG;

            if (argv[i][2] && argv[i][2] != ':')
                continue;

            switch (towlower(argv[i][1]))
            {
            case 'u':
                Unregister = TRUE;
                break;
            case 's':
                Silent = TRUE;
                break;
            case 'i':
                CallInstall = TRUE;
                wsCommandLine = parse_command_line(argv[i] + 2); /* argv[i] + strlen("/i") */
                if (!wsCommandLine)
                    wsCommandLine = EmptyLine;
                break;
            case 'n':
                CallRegister = FALSE;
                break;
            case 'c':
                /* console output */;
                break;
            default:
                output_write(STRING_UNRECOGNIZED_SWITCH, argv[i]);
                output_write(STRING_USAGE);
                return INVALID_ARG;
            }
            argv[i] = NULL;
        }
    }

    if (!CallInstall && !CallRegister) /* flags: /n or /u /n */
        return INVALID_ARG;

    for (i = 1; i < argc; i++)
    {
        if (argv[i])
        {
            WCHAR *DllName = argv[i];
            BOOL firstDll = !DllFound;
            res = 0;

            DllFound = TRUE;
            if (CallInstall && Unregister)
                res = InstallDll(!Unregister, DllName, wsCommandLine, firstDll);

            /* The Windows version stops processing the current file on the first error. */
            if (res)
            {
                ret = res;
                continue;
            }

            if (!CallInstall || CallRegister)
            {
                if(Unregister)
                    res = UnregisterDll(DllName, firstDll);
                else
                    res = RegisterDll(DllName, firstDll);
            }

            if (res)
            {
                ret = res;
                continue;
            }

            if (CallInstall && !Unregister)
                res = InstallDll(!Unregister, DllName, wsCommandLine, firstDll);

            if (res)
            {
                ret = res;
		continue;
            }
        }
    }

    if (!DllFound)
    {
        output_write(STRING_HEADER);
        output_write(STRING_USAGE);
        return INVALID_ARG;
    }

    OleUninitialize();

    /* return the most recent error code, even if later DLLs succeed */
    return ret;
}
