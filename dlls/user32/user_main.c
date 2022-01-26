/*
 * USER initialization code
 *
 * Copyright 2000 Alexandre Julliard
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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"

#include "controls.h"
#include "user_private.h"
#include "win.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

HMODULE user32_module = 0;

static CRITICAL_SECTION user_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &user_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": user_section") }
};
static CRITICAL_SECTION user_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static HPALETTE (WINAPI *pfnGDISelectPalette)( HDC hdc, HPALETTE hpal, WORD bkgnd );
static UINT (WINAPI *pfnGDIRealizePalette)( HDC hdc );
static HPALETTE hPrimaryPalette;

static DWORD exiting_thread_id;

extern void WDML_NotifyThreadDetach(void);

/***********************************************************************
 *           USER_Lock
 */
void USER_Lock(void)
{
    EnterCriticalSection( &user_section );
}


/***********************************************************************
 *           USER_Unlock
 */
void USER_Unlock(void)
{
    LeaveCriticalSection( &user_section );
}


/***********************************************************************
 *           USER_CheckNotLock
 *
 * Make sure that we don't hold the user lock.
 */
void USER_CheckNotLock(void)
{
    if (RtlIsCriticalSectionLockedByThread(&user_section))
    {
        ERR( "BUG: holding USER lock\n" );
        DebugBreak();
    }
}


/***********************************************************************
 *		UserSelectPalette (Not a Windows API)
 */
static HPALETTE WINAPI UserSelectPalette( HDC hDC, HPALETTE hPal, BOOL bForceBackground )
{
    WORD wBkgPalette = 1;

    if (!bForceBackground && (hPal != GetStockObject(DEFAULT_PALETTE)))
    {
        HWND hwnd = WindowFromDC( hDC );
        if (hwnd)
        {
            HWND hForeground = GetForegroundWindow();
            /* set primary palette if it's related to current active */
            if (hForeground == hwnd || IsChild(hForeground,hwnd))
            {
                wBkgPalette = 0;
                hPrimaryPalette = hPal;
            }
        }
    }
    return pfnGDISelectPalette( hDC, hPal, wBkgPalette);
}


/***********************************************************************
 *		UserRealizePalette (USER32.@)
 */
UINT WINAPI UserRealizePalette( HDC hDC )
{
    UINT realized = pfnGDIRealizePalette( hDC );

    /* do not send anything if no colors were changed */
    if (realized && GetCurrentObject( hDC, OBJ_PAL ) == hPrimaryPalette)
    {
        /* send palette change notification */
        HWND hWnd = WindowFromDC( hDC );
        if (hWnd) SendMessageTimeoutW( HWND_BROADCAST, WM_PALETTECHANGED, (WPARAM)hWnd, 0,
                                       SMTO_ABORTIFHUNG, 2000, NULL );
    }
    return realized;
}


/***********************************************************************
 *           palette_init
 *
 * Patch the function pointers in GDI for SelectPalette and RealizePalette
 */
static void palette_init(void)
{
    void **ptr;
    HMODULE module = GetModuleHandleA( "gdi32" );
    if (!module)
    {
        ERR( "cannot get GDI32 handle\n" );
        return;
    }
    if ((ptr = (void**)GetProcAddress( module, "pfnSelectPalette" )))
        pfnGDISelectPalette = InterlockedExchangePointer( ptr, UserSelectPalette );
    else ERR( "cannot find pfnSelectPalette in GDI32\n" );
    if ((ptr = (void**)GetProcAddress( module, "pfnRealizePalette" )))
        pfnGDIRealizePalette = InterlockedExchangePointer( ptr, UserRealizePalette );
    else ERR( "cannot find pfnRealizePalette in GDI32\n" );
}


/***********************************************************************
 *           dpiaware_init
 *
 * Initialize the DPI awareness style.
 */
static void dpiaware_init(void)
{
    WCHAR buffer[256];
    DWORD option;

    if (!LdrQueryImageFileExecutionOptions( &NtCurrentTeb()->Peb->ProcessParameters->ImagePathName,
                                            L"dpiAwareness", REG_DWORD, &option, sizeof(option), NULL ))
    {
        TRACE( "got option %x\n", option );
        if (option <= 2)
        {
            SetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~(ULONG_PTR)option );
            return;
        }
    }

    if (QueryActCtxSettingsW( 0, NULL, L"http://schemas.microsoft.com/SMI/2016/WindowsSettings",
                              L"dpiAwareness", buffer, ARRAY_SIZE(buffer), NULL ))
    {
        static const WCHAR * const types[] = { L"unaware", L"system", L"permonitor", L"permonitorv2" };
        WCHAR *p, *start, *end;
        ULONG_PTR i;

        TRACE( "got dpiAwareness=%s\n", debugstr_w(buffer) );
        for (start = buffer; *start; start = end)
        {
            start += wcsspn( start, L" \t\r\n" );
            if (!(end = wcschr( start, ',' ))) end = start + lstrlenW(start);
            else *end++ = 0;
            if ((p = wcspbrk( start, L" \t\r\n" ))) *p = 0;
            for (i = 0; i < ARRAY_SIZE(types); i++)
            {
                if (wcsicmp( start, types[i] )) continue;
                SetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~i );
                return;
            }
        }
    }
    else if (QueryActCtxSettingsW( 0, NULL, L"http://schemas.microsoft.com/SMI/2005/WindowsSettings",
                                   L"dpiAware", buffer, ARRAY_SIZE(buffer), NULL ))
    {
        TRACE( "got dpiAware=%s\n", debugstr_w(buffer) );
        if (!wcsicmp( buffer, L"true" ))
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
        else if (!wcsicmp( buffer, L"true/pm" ) || !wcsicmp( buffer, L"per monitor" ))
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        else
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    }
}


static const void *kernel_callback_table[NtUserCallCount] =
{
    User32CallEnumDisplayMonitor,
};


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    NtCurrentTeb()->Peb->KernelCallbackTable = kernel_callback_table;

    dpiaware_init();
    register_desktop_class();

    /* Initialize system colors and metrics */
    SYSPARAMS_Init();

    /* Setup palette function pointers */
    palette_init();

    return TRUE;
}


/**********************************************************************
 *           USER_IsExitingThread
 */
BOOL USER_IsExitingThread( DWORD tid )
{
    return (tid == exiting_thread_id);
}


/**********************************************************************
 *           thread_detach
 */
static void thread_detach(void)
{
    struct user_thread_info *thread_info = get_user_thread_info();

    exiting_thread_id = GetCurrentThreadId();

    WDML_NotifyThreadDetach();
    USER_Driver->pThreadDetach();

    destroy_thread_windows();
    CloseHandle( thread_info->server_queue );
    HeapFree( GetProcessHeap(), 0, thread_info->wmchar_data );
    HeapFree( GetProcessHeap(), 0, thread_info->key_state );
    HeapFree( GetProcessHeap(), 0, thread_info->rawinput );

    exiting_thread_id = 0;
}


/***********************************************************************
 *           UserClientDllInitialize  (USER32.@)
 *
 * USER dll initialisation routine (exported as UserClientDllInitialize for compatibility).
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    static HMODULE imm32_module;
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        user32_module = inst;
        ret = process_attach();
        if(ret)
            imm32_module = LoadLibraryW(L"imm32.dll");
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        USER_unload_driver();
        FreeLibrary(imm32_module);
        DeleteCriticalSection(&user_section);
        break;
    }
    return ret;
}


/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT flags, DWORD reason )
{
    WCHAR app[MAX_PATH];
    WCHAR cmdline[MAX_PATH + 64];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    void *redir;

    GetSystemDirectoryW( app, MAX_PATH - ARRAY_SIZE( L"\\wineboot.exe" ));
    lstrcatW( app, L"\\wineboot.exe" );
    lstrcpyW( cmdline, app );

    if (flags & EWX_FORCE) lstrcatW( cmdline, L" --kill" );
    else
    {
        lstrcatW( cmdline, L" --end-session" );
        if (flags & EWX_FORCEIFHUNG) lstrcatW( cmdline, L" --force" );
    }
    if (!(flags & EWX_REBOOT)) lstrcatW( cmdline, L" --shutdown" );

    memset( &si, 0, sizeof si );
    si.cb = sizeof si;
    Wow64DisableWow64FsRedirection( &redir );
    if (!CreateProcessW( app, cmdline, NULL, NULL, FALSE, DETACHED_PROCESS, NULL, NULL, &si, &pi ))
    {
        Wow64RevertWow64FsRedirection( redir );
        ERR( "Failed to run %s\n", debugstr_w(cmdline) );
        return FALSE;
    }
    Wow64RevertWow64FsRedirection( redir );
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    return TRUE;
}

/***********************************************************************
 *		LockWorkStation (USER32.@)
 */
BOOL WINAPI LockWorkStation(void)
{
    TRACE(": stub\n");
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *		RegisterServicesProcess (USER32.@)
 */
int WINAPI RegisterServicesProcess(DWORD ServicesProcessId)
{
    FIXME("(0x%x): stub\n", ServicesProcessId);
    return 0;
}

/***********************************************************************
 *		ShutdownBlockReasonCreate (USER32.@)
 */
BOOL WINAPI ShutdownBlockReasonCreate(HWND hwnd, LPCWSTR reason)
{
    FIXME("(%p, %s): stub\n", hwnd, debugstr_w(reason));
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}

/***********************************************************************
 *		ShutdownBlockReasonDestroy (USER32.@)
 */
BOOL WINAPI ShutdownBlockReasonDestroy(HWND hwnd)
{
    FIXME("(%p): stub\n", hwnd);
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}
