/*
 * PURPOSE: Load a DLL and run an entry point with the specified parameters
 *
 * Copyright 2002 Alberto Massari
 * Copyright 2001-2003 Aric Stewart for CodeWeavers
 * Copyright 2003 Mike McCormack for CodeWeavers
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
 */

/*
 *
 *  rundll32 dllname,entrypoint [arguments]
 *
 *  Documentation for this utility found on KB Q164787
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Exclude rarely-used stuff from Windows headers */
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#include "wine/winbase16.h"
#include "wine/asm.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(rundll32);


#ifdef __i386__
/* wrapper for dlls that declare the entry point incorrectly */
extern void call_entry_point( void *func, HWND hwnd, HINSTANCE inst, void *cmdline, int show );
__ASM_GLOBAL_FUNC( call_entry_point,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-12\n\t")
                   "subl $12,%esp\n\t"
                   "pushl 24(%ebp)\n\t"
                   "pushl 20(%ebp)\n\t"
                   "pushl 16(%ebp)\n\t"
                   "pushl 12(%ebp)\n\t"
                   "call *8(%ebp)\n\t"
                   "leal -12(%ebp),%esp\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "leave\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret" )
#else
static void call_entry_point( void *func, HWND hwnd, HINSTANCE inst, void *cmdline, int show )
{
    void (WINAPI *entry_point)( HWND hwnd, HINSTANCE inst, void *cmdline, int show ) = func;
    entry_point( hwnd, inst, cmdline, show );
}
#endif

static void (WINAPI *pRunDLL_CallEntry16)( FARPROC proc, HWND hwnd, HINSTANCE inst,
                                           LPCSTR cmdline, INT cmdshow );

/*
 * Control_RunDLL needs to have a window. So lets make us a very simple window class.
 */
static ATOM register_class(void)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEXW);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = DefWindowProcW;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = NULL;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = L"class_rundll32";
    wcex.hIconSm        = NULL;

    return RegisterClassExW(&wcex);
}

#ifdef __i386__

static HINSTANCE16 load_dll16( LPCWSTR dll )
{
    HINSTANCE16 (WINAPI *pLoadLibrary16)(LPCSTR libname);
    HINSTANCE16 ret = 0;
    DWORD len = WideCharToMultiByte( CP_ACP, 0, dll, -1, NULL, 0, NULL, NULL );
    char *dllA = malloc( len );

    if (dllA)
    {
        WideCharToMultiByte( CP_ACP, 0, dll, -1, dllA, len, NULL, NULL );
        pLoadLibrary16 = (void *)GetProcAddress( GetModuleHandleW(L"kernel32.dll"), (LPCSTR)35 );
        if (pLoadLibrary16) ret = pLoadLibrary16( dllA );
        free( dllA );
    }
    return ret;
}

static FARPROC16 get_entry_point16( HINSTANCE16 inst, LPCWSTR entry )
{
    FARPROC16 (WINAPI *pGetProcAddress16)(HMODULE16 hModule, LPCSTR name);
    FARPROC16 ret = 0;
    DWORD len = WideCharToMultiByte( CP_ACP, 0, entry, -1, NULL, 0, NULL, NULL );
    char *entryA = malloc( len );

    if (entryA)
    {
        WideCharToMultiByte( CP_ACP, 0, entry, -1, entryA, len, NULL, NULL );
        pGetProcAddress16 = (void *)GetProcAddress( GetModuleHandleW(L"kernel32.dll"), (LPCSTR)37 );
        if (pGetProcAddress16) ret = pGetProcAddress16( inst, entryA );
        free( entryA );
    }
    return ret;
}
#endif

static void *get_entry_point32( HMODULE module, LPCWSTR entry, BOOL *unicode )
{
    void *ret;

    /* determine if the entry point is an ordinal */
    if (entry[0] == '#')
    {
        INT_PTR ordinal = wcstol( entry + 1, NULL, 10 );
        if (ordinal <= 0)
            return NULL;

        *unicode = TRUE;
        ret = GetProcAddress( module, (LPCSTR)ordinal );
    }
    else
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, entry, -1, NULL, 0, NULL, NULL );
        char *entryA = malloc( len + 1 );

        if (!entryA)
            return NULL;

        WideCharToMultiByte( CP_ACP, 0, entry, -1, entryA, len, NULL, NULL );

        /* first try the W version */
        *unicode = TRUE;
        strcat( entryA, "W" );
        if (!(ret = GetProcAddress( module, entryA )))
        {
            /* now the A version */
            *unicode = FALSE;
            entryA[strlen(entryA)-1] = 'A';
            if (!(ret = GetProcAddress( module, entryA )))
            {
                /* now the version without suffix */
                entryA[strlen(entryA)-1] = 0;
                ret = GetProcAddress( module, entryA );
            }
        }
        free( entryA );
    }
    return ret;
}

static WCHAR *parse_arguments( WCHAR *cmdline, WCHAR **dll, WCHAR **entrypoint )
{
    WCHAR *p;

    if (cmdline[0] == '"')
        p = wcschr( ++cmdline, '"' );
    else
        p = wcspbrk( cmdline, L" ,\t" );

    if (!p) return NULL;
    *p++ = 0;
    *dll = cmdline;
    /* skip spaces and commas */
    p += wcsspn( p, L" ,\t" );
    if (!*p) return NULL;
    *entrypoint = p;
    /* find end of entry point string */
    p += wcscspn( p, L" \t" );
    if (*p) *p++ = 0;
    p += wcsspn( p, L" \t" );
    return p;
}

static void reexec_self( WCHAR *args, WORD machine )
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    WCHAR app[MAX_PATH];
    WCHAR *cmdline;
    HANDLE process = 0;
    STARTUPINFOEXW si = {{ sizeof(si.StartupInfo) }};
    PROCESS_INFORMATION pi;
    SIZE_T len, size = 1024;
    struct _PROC_THREAD_ATTRIBUTE_LIST *list = malloc( size );
    void *cookie;
    ULONG i;

    NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
                                machines, sizeof(machines), NULL );
    for (i = 0; machines[i].Machine; i++) if (machines[i].Machine == machine) break;
    if (!GetSystemWow64Directory2W( app, MAX_PATH, machines[i].WoW64Container ?
                                    machine : IMAGE_FILE_MACHINE_TARGET_HOST )) return;
    wcscat( app, L"\\rundll32.exe" );

    TRACE( "restarting as %s cmdline %s\n", debugstr_w(app), debugstr_w(args) );

    len = wcslen(app) + wcslen(args) + 2;
    cmdline = malloc( len * sizeof(WCHAR) );
    swprintf( cmdline, len, L"%s %s", app, args );

    InitializeProcThreadAttributeList( list, 1, 0, &size );
    UpdateProcThreadAttribute( list, 0, PROC_THREAD_ATTRIBUTE_MACHINE_TYPE, &machine, sizeof(machine), NULL, NULL );
    si.StartupInfo.cb = sizeof(si);
    si.lpAttributeList = list;

    Wow64DisableWow64FsRedirection(&cookie);
    if (CreateProcessW( app, cmdline, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
                        NULL, NULL, &si.StartupInfo, &pi ))
    {
        DWORD exit_code;
        WaitForSingleObject( pi.hProcess, INFINITE );
        GetExitCodeProcess( pi.hProcess, &exit_code );
        ExitProcess( exit_code );
    }
    Wow64RevertWow64FsRedirection(cookie);
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE hOldInstance, LPWSTR szCmdLine, int nCmdShow)
{
    HWND hWnd;
    LPWSTR szDllName, szEntryPoint, args;
    void *entry_point = NULL;
    BOOL unicode = FALSE, win16 = FALSE;
    HMODULE hDll, hCtx;
    WCHAR path[MAX_PATH];
    STARTUPINFOW info;
    ULONG_PTR cookie;
    ACTCTXW ctx;

    /* Initialize the rundll32 class */
    register_class();
    hWnd = CreateWindowW(L"class_rundll32", L"rundll32", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
          CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, NULL, NULL);

    /* Get the dll name and API EntryPoint */
    args = parse_arguments( wcsdup(szCmdLine), &szDllName, &szEntryPoint );
    if (!args) return 0;

    TRACE( "dll %s entry %s cmdline %s\n", wine_dbgstr_w(szDllName),
           wine_dbgstr_w(szEntryPoint), wine_dbgstr_w(args) );

    /* Activate context before DllMain() is called */
    if (SearchPathW(NULL, szDllName, NULL, ARRAY_SIZE(path), path, NULL))
    {
        memset(&ctx, 0, sizeof(ctx));
        ctx.cbSize = sizeof(ctx);
        ctx.lpSource = path;
        ctx.lpResourceName = MAKEINTRESOURCEW(123);
        ctx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
        hCtx = CreateActCtxW(&ctx);
        if (hCtx != INVALID_HANDLE_VALUE)
            ActivateActCtx(hCtx, &cookie);
        else
            WINE_TRACE("No manifest at ID 123 in %s\n", wine_dbgstr_w(path));
    }

    /* Load the library */
    hDll=LoadLibraryW(szDllName);
    if (hDll) entry_point = get_entry_point32( hDll, szEntryPoint, &unicode );
    else
    {
        HMODULE module;
        if (GetLastError() == ERROR_BAD_EXE_FORMAT &&
            (module = LoadLibraryExW( szDllName, 0, LOAD_LIBRARY_AS_IMAGE_RESOURCE )))
        {
            IMAGE_NT_HEADERS *nt = RtlImageNtHeader( (HMODULE)((ULONG_PTR)module & ~3) );
            reexec_self( szCmdLine, nt->FileHeader.Machine );
        }
#ifdef __i386__
        else
        {
            HINSTANCE16 dll = load_dll16( szDllName );
            if (dll <= 32)
            {
                /* Windows has a MessageBox here... */
                WINE_ERR("Unable to load %s\n",wine_dbgstr_w(szDllName));
                return 0;
            }
            win16 = TRUE;
            entry_point = get_entry_point16( dll, szEntryPoint );
        }
#endif
    }

    if (!entry_point)
    {
        /* Windows has a MessageBox here... */
        WINE_ERR( "Unable to find the entry point %s in %s\n",
                  wine_dbgstr_w(szEntryPoint), wine_dbgstr_w(szDllName) );
        return 0;
    }

    GetStartupInfoW( &info );
    if (!(info.dwFlags & STARTF_USESHOWWINDOW)) info.wShowWindow = SW_SHOWDEFAULT;

    if (unicode)
    {
        WINE_TRACE( "Calling %s (%p,%p,%s,%d)\n", wine_dbgstr_w(szEntryPoint),
                    hWnd, instance, wine_dbgstr_w(args), info.wShowWindow );

        call_entry_point( entry_point, hWnd, instance, args, info.wShowWindow );
    }
    else
    {
        DWORD len = WideCharToMultiByte( CP_ACP, 0, args, -1, NULL, 0, NULL, NULL );
        char *cmdline = malloc( len );

        if (!cmdline)
            return 0;

        WideCharToMultiByte( CP_ACP, 0, args, -1, cmdline, len, NULL, NULL );

        WINE_TRACE( "Calling %s (%p,%p,%s,%d)\n", wine_dbgstr_w(szEntryPoint),
                    hWnd, instance, wine_dbgstr_a(cmdline), info.wShowWindow );

        if (win16)
        {
            HMODULE shell = LoadLibraryW( L"shell32.dll" );
            if (shell) pRunDLL_CallEntry16 = (void *)GetProcAddress( shell, (LPCSTR)122 );
            if (pRunDLL_CallEntry16)
                pRunDLL_CallEntry16( entry_point, hWnd, instance, cmdline, info.wShowWindow );
        }
        else call_entry_point( entry_point, hWnd, instance, cmdline, info.wShowWindow );
    }

    return 0; /* rundll32 always returns 0! */
}
