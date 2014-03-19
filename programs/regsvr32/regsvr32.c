/*
 * PURPOSE: Register OLE components in the registry
 *
 * Copyright 2001 ReactOS project
 * Copyright 2001 Jurgen Van Gael [jurgen.vangael@student.kuleuven.ac.be]
 * Copyright 2002 Andriy Palamarchuk
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
 *
 * This version deliberately differs in error handling compared to the
 * windows version.
 */

/*
 *
 *  regsvr32 [/u] [/s] [/n] [/i[:cmdline]] dllname ...
 *  [/u]    unregister server
 *  [/s]    silent (no message boxes)
 *  [/i]    Call DllInstall passing it an optional [cmdline];
 *          when used with /u calls dll uninstall.
 *  [/n]    Do not call DllRegisterServer; this option must be used with [/i]
 *  [/c]    Console output (seems to be deprecated and ignored)
 *
 *  Note the complication that this version may be passed unix format file names
 *  which might be mistaken for flags.  Conveniently the Windows version
 *  requires each flag to be separate (e.g. no /su ) and so we will simply
 *  assume that anything longer than /. is a filename.
 */

/**
 * FIXME - currently receives command-line parameters in ASCII only and later
 * converts to Unicode. Ideally the function should have wWinMain entry point
 * and then work in Unicode only, but it seems Wine does not have necessary
 * support.
 */

#define WIN32_LEAN_AND_MEAN

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <windows.h>
#include <ole2.h>
#include "regsvr32.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(regsvr32);

typedef HRESULT (*DLLREGISTER)          (void);
typedef HRESULT (*DLLUNREGISTER)        (void);
typedef HRESULT (*DLLINSTALL)           (BOOL,LPCWSTR);

static BOOL Silent = FALSE;

static void __cdecl output_write(UINT id, ...)
{
    char fmt[1024];
    __ms_va_list va_args;
    char *str;
    DWORD len, nOut, ret;

    if (!LoadStringA(GetModuleHandleA(NULL), id, fmt, sizeof(fmt)/sizeof(fmt[0])))
    {
        WINE_FIXME("LoadString failed with %d\n", GetLastError());
        return;
    }

    __ms_va_start(va_args, id);
    SetLastError(NO_ERROR);
    len = FormatMessageA(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ALLOCATE_BUFFER,
                         fmt, 0, 0, (LPSTR)&str, 0, &va_args);
    __ms_va_end(va_args);
    if (len == 0 && GetLastError() != NO_ERROR)
    {
        WINE_FIXME("Could not format string: le=%u, fmt=%s\n", GetLastError(), wine_dbgstr_a(fmt));
        return;
    }

    ret = WriteConsoleA(GetStdHandle(STD_OUTPUT_HANDLE), str, len, &nOut, NULL);

    if (!ret)
        WINE_WARN("regsvr32: WriteConsoleA() failed.\n");

    LocalFree(str);
}

/**
 * Loads procedure.
 *
 * Parameters:
 * strDll - name of the dll.
 * procName - name of the procedure to load from dll
 * pDllHanlde - output variable receives handle of the loaded dll.
 */
static VOID *LoadProc(const char* strDll, const char* procName, HMODULE* DllHandle)
{
    VOID* (*proc)(void);

    *DllHandle = LoadLibraryExA(strDll, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if(!*DllHandle)
    {
        if(!Silent)
            output_write(STRING_DLL_LOAD_FAILED, strDll);

        ExitProcess(1);
    }
    proc = (VOID *) GetProcAddress(*DllHandle, procName);
    if(!proc)
    {
        if(!Silent)
            output_write(STRING_PROC_NOT_IMPLEMENTED, procName, strDll);
        FreeLibrary(*DllHandle);
        return NULL;
    }
    return proc;
}

static int RegisterDll(const char* strDll)
{
    HRESULT hr;
    DLLREGISTER pfRegister;
    HMODULE DllHandle = NULL;

    pfRegister = LoadProc(strDll, "DllRegisterServer", &DllHandle);
    if (!pfRegister)
        return 0;

    hr = pfRegister();
    if(FAILED(hr))
    {
        if(!Silent)
            output_write(STRING_REGISTER_FAILED, strDll);

        return -1;
    }
    if(!Silent)
        output_write(STRING_REGISTER_SUCCESSFUL, strDll);

    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}

static int UnregisterDll(char* strDll)
{
    HRESULT hr;
    DLLUNREGISTER pfUnregister;
    HMODULE DllHandle = NULL;

    pfUnregister = LoadProc(strDll, "DllUnregisterServer", &DllHandle);
    if (!pfUnregister)
        return 0;

    hr = pfUnregister();
    if(FAILED(hr))
    {
        if(!Silent)
            output_write(STRING_UNREGISTER_FAILED, strDll);

        return -1;
    }
    if(!Silent)
        output_write(STRING_UNREGISTER_SUCCESSFUL, strDll);

    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}

static int InstallDll(BOOL install, char *strDll, WCHAR *command_line)
{
    HRESULT hr;
    DLLINSTALL pfInstall;
    HMODULE DllHandle = NULL;

    pfInstall = LoadProc(strDll, "DllInstall", &DllHandle);
    if (!pfInstall)
        return 0;

    hr = pfInstall(install, command_line);
    if(FAILED(hr))
    {
        if(!Silent)
        {
            if (install)
                output_write(STRING_INSTALL_FAILED, strDll);
            else
                output_write(STRING_UNINSTALL_FAILED, strDll);
        }
        return -1;
    }
    if(!Silent)
    {
        if (install)
            output_write(STRING_INSTALL_SUCCESSFUL, strDll);
        else
            output_write(STRING_UNINSTALL_SUCCESSFUL, strDll);
    }

    if(DllHandle)
        FreeLibrary(DllHandle);
    return 0;
}

int main(int argc, char* argv[])
{
    int             i;
    BOOL            CallRegister = TRUE;
    BOOL            CallInstall = FALSE;
    BOOL            Unregister = FALSE;
    BOOL            DllFound = FALSE;
    WCHAR*          wsCommandLine = NULL;
    WCHAR           EmptyLine[1] = {0};

    OleInitialize(NULL);

    /* Strictly, the Microsoft version processes all the flags before
     * the files (e.g. regsvr32 file1 /s file2 is silent even for file1).
     * For ease, we will not replicate that and will process the arguments
     * in order.
     */
    for(i = 1; i < argc; i++)
    {
        if ((!strcasecmp(argv[i], "/u")) ||(!strcasecmp(argv[i], "-u")))
                Unregister = TRUE;
        else if ((!strcasecmp(argv[i], "/s"))||(!strcasecmp(argv[i], "-s")))
                Silent = TRUE;
        else if ((!strncasecmp(argv[i], "/i", strlen("/i")))||(!strncasecmp(argv[i], "-i", strlen("-i"))))
        {
            CHAR* command_line = argv[i] + strlen("/i");

            CallInstall = TRUE;
            if (command_line[0] == ':' && command_line[1])
            {
                int len = strlen(command_line);

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
                {
                    len = MultiByteToWideChar(CP_ACP, 0, command_line, -1,
                                              NULL, 0);
                    wsCommandLine = HeapAlloc(GetProcessHeap(), 0,
                                              len * sizeof(WCHAR));
                    if (wsCommandLine)
                        MultiByteToWideChar(CP_ACP, 0, command_line, -1,
                                            wsCommandLine, len);
                }
                else
                {
                    wsCommandLine = EmptyLine;
                }
            }
            else
            {
                wsCommandLine = EmptyLine;
            }
        }
        else if((!strcasecmp(argv[i], "/n"))||(!strcasecmp(argv[i], "-n")))
            CallRegister = FALSE;
        else if((!strcasecmp(argv[i], "/c"))||(!strcasecmp(argv[i], "-c")))
            /* console output */;
        else if (argv[i][0] == '/' && (!argv[i][2] || argv[i][2] == ':'))
        {
            output_write(STRING_UNRECOGNIZED_SWITCH, argv[i]);
            output_write(STRING_USAGE);
            return 1;
        }
        else
        {
            char *DllName = argv[i];
            int res = 0;

            DllFound = TRUE;
            if (!CallInstall || (CallInstall && CallRegister))
            {
                if(Unregister)
                    res = UnregisterDll(DllName);
                else
                    res = RegisterDll(DllName);
            }

            if (res)
                return res;
	    /* Confirmed.  The windows version does stop on the first error.*/

            if (CallInstall)
            {
                res = InstallDll(!Unregister, DllName, wsCommandLine);
            }

            if (res)
		return res;
        }
    }

    if (!DllFound)
    {
        if(!Silent)
        {
            output_write(STRING_HEADER);
            output_write(STRING_USAGE);
        }
        return 1;
    }

    OleUninitialize();

    return 0;
}
