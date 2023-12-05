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
#include <winternl.h>
#include <winnls.h>
#include <ole2.h>
#include "regsvr32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(regsvr32);

typedef HRESULT (WINAPI *DLLREGISTER)   (void);
typedef HRESULT (WINAPI *DLLUNREGISTER) (void);
typedef HRESULT (WINAPI *DLLINSTALL)    (BOOL,LPCWSTR);

static BOOL Silent;

static void WINAPIV output_write(BOOL with_usage, UINT id, ...)
{
    WCHAR buffer[4096];
    WCHAR fmt[1024];
    va_list va_args;
    DWORD len;
    LCID current_lcid;

    current_lcid = GetThreadLocale();
    if (Silent) /* force en-US not to have localized strings */
        SetThreadLocale(MAKELCID(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));

    if (!LoadStringW(GetModuleHandleW(NULL), id, fmt, ARRAY_SIZE(fmt)))
    {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        SetThreadLocale(current_lcid);
        return;
    }

    va_start(va_args, id);
    len = FormatMessageW(FORMAT_MESSAGE_FROM_STRING,
                         fmt, 0, 0, buffer, ARRAY_SIZE(buffer), &va_args);
    va_end(va_args);
    if (len == 0 && GetLastError() != ERROR_NO_WORK_DONE)
    {
        WINE_FIXME("Could not format string: le=%lu, fmt=%s\n", GetLastError(), wine_dbgstr_w(fmt));
        SetThreadLocale(current_lcid);
        return;
    }
    if (with_usage &&
        !LoadStringW(GetModuleHandleW(NULL), STRING_USAGE,
                     &buffer[wcslen(buffer)], ARRAY_SIZE(buffer) - wcslen(buffer)))
    {
        WINE_FIXME("LoadString failed with %ld\n", GetLastError());
        SetThreadLocale(current_lcid);
        return;
    }
    if (Silent)
        MESSAGE("%ls", buffer);
    else
        MessageBoxW(NULL, buffer, L"RegSvr32", MB_OK);
    SetThreadLocale(current_lcid);
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

static void reexec_self( WORD machine )
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    WCHAR app[MAX_PATH];
    LPCWSTR args;
    WCHAR *cmdline;
    HANDLE process = 0;
    STARTUPINFOW si = {0};
    PROCESS_INFORMATION pi;
    void *cookie;
    ULONG i;

    NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                machines, sizeof(machines), NULL );
    for (i = 0; machines[i].Machine; i++) if (machines[i].Machine == machine) break;
    if (!machines[i].Machine) return;
    if (machines[i].Native) machine = IMAGE_FILE_MACHINE_TARGET_HOST;
    if (!GetSystemWow64Directory2W( app, MAX_PATH, machine )) return;
    wcscat( app, L"\\regsvr32.exe" );

    TRACE( "restarting as %s\n", debugstr_w(app) );

    args = find_arg_start(GetCommandLineW());

    cmdline = HeapAlloc(GetProcessHeap(), 0,
                        (wcslen(app)+wcslen(args)+1)*sizeof(WCHAR));

    wcscpy(cmdline, app);
    wcscat(cmdline, args);

    si.cb = sizeof(si);

    Wow64DisableWow64FsRedirection(&cookie);
    if (CreateProcessW(app, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
    {
        DWORD exit_code;
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exit_code);
        ExitProcess(exit_code);
    }
    else
    {
        WINE_TRACE("failed to restart, err=%ld\n", GetLastError());
    }
    Wow64RevertWow64FsRedirection(cookie);
    HeapFree(GetProcessHeap(), 0, cmdline);
}

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
        HMODULE module;
        if (firstDll && GetLastError() == ERROR_BAD_EXE_FORMAT &&
            (module = LoadLibraryExW(strDll, 0, LOAD_LIBRARY_AS_IMAGE_RESOURCE)))
        {
            IMAGE_NT_HEADERS *nt = RtlImageNtHeader( (HMODULE)((ULONG_PTR)module & ~3) );
            reexec_self( nt->FileHeader.Machine );
        }
        output_write(FALSE, STRING_DLL_LOAD_FAILED, strDll);
        ExitProcess(LOADLIBRARY_FAILED);
    }
    proc = (VOID *) GetProcAddress(*DllHandle, procName);
    if(!proc)
    {
        output_write(FALSE, STRING_PROC_NOT_IMPLEMENTED, procName, strDll);
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
        output_write(FALSE, STRING_REGISTER_FAILED, strDll);
        return DLLSERVER_FAILED;
    }
    output_write(FALSE, STRING_REGISTER_SUCCESSFUL, strDll);

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
        output_write(FALSE, STRING_UNREGISTER_FAILED, strDll);
        return DLLSERVER_FAILED;
    }
    output_write(FALSE, STRING_UNREGISTER_SUCCESSFUL, strDll);

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
            output_write(FALSE, STRING_INSTALL_FAILED, strDll);
        else
            output_write(FALSE, STRING_UNINSTALL_FAILED, strDll);
        return DLLSERVER_FAILED;
    }
    if (install)
        output_write(FALSE, STRING_INSTALL_SUCCESSFUL, strDll);
    else
        output_write(FALSE, STRING_UNINSTALL_SUCCESSFUL, strDll);

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
    WCHAR           EmptyLine[] = L"";

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
            default:
                output_write(TRUE, STRING_UNRECOGNIZED_SWITCH, argv[i]);
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
        output_write(TRUE, STRING_HEADER);
        return INVALID_ARG;
    }

    OleUninitialize();

    /* return the most recent error code, even if later DLLs succeed */
    return ret;
}
