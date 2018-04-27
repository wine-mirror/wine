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
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

#define DESKTOP_ALL_ACCESS 0x01ff

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
 *           get_default_desktop
 *
 * Get the name of the desktop to use for this app if not specified explicitly.
 */
static const WCHAR *get_default_desktop(void)
{
    static const WCHAR defaultW[] = {'D','e','f','a','u','l','t',0};
    static const WCHAR desktopW[] = {'D','e','s','k','t','o','p',0};
    static const WCHAR explorerW[] = {'\\','E','x','p','l','o','r','e','r',0};
    static const WCHAR app_defaultsW[] = {'S','o','f','t','w','a','r','e','\\',
                                          'W','i','n','e','\\',
                                          'A','p','p','D','e','f','a','u','l','t','s',0};
    static WCHAR buffer[MAX_PATH + sizeof(explorerW)/sizeof(WCHAR)];
    WCHAR *p, *appname = buffer;
    const WCHAR *ret = defaultW;
    DWORD len;
    HKEY tmpkey, appkey;

    len = (GetModuleFileNameW( 0, buffer, MAX_PATH ));
    if (!len || len >= MAX_PATH) return ret;
    if ((p = strrchrW( appname, '/' ))) appname = p + 1;
    if ((p = strrchrW( appname, '\\' ))) appname = p + 1;
    p = appname + strlenW(appname);
    strcpyW( p, explorerW );

    /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Explorer */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, app_defaultsW, &tmpkey ))
    {
        if (RegOpenKeyW( tmpkey, appname, &appkey )) appkey = 0;
        RegCloseKey( tmpkey );
        if (appkey)
        {
            len = sizeof(buffer);
            if (!RegQueryValueExW( appkey, desktopW, 0, NULL, (LPBYTE)buffer, &len )) ret = buffer;
            RegCloseKey( appkey );
            if (ret && strcmpiW( ret, defaultW )) return ret;
            ret = defaultW;
        }
    }

    memcpy( buffer, app_defaultsW, 13 * sizeof(WCHAR) );  /* copy only software\\wine */
    strcpyW( buffer + 13, explorerW );

    /* @@ Wine registry key: HKCU\Software\Wine\Explorer */
    if (!RegOpenKeyW( HKEY_CURRENT_USER, buffer, &appkey ))
    {
        len = sizeof(buffer);
        if (!RegQueryValueExW( appkey, desktopW, 0, NULL, (LPBYTE)buffer, &len )) ret = buffer;
        RegCloseKey( appkey );
    }
    return ret;
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
    static const WCHAR dpiAwareW[] = {'d','p','i','A','w','a','r','e',0};
    static const WCHAR dpiAwarenessW[] = {'d','p','i','A','w','a','r','e','n','e','s','s',0};
    static const WCHAR namespace2005W[] = {'h','t','t','p',':','/','/','s','c','h','e','m','a','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','S','M','I','/','2','0','0','5','/','W','i','n','d','o','w','s','S','e','t','t','i','n','g','s',0};
    static const WCHAR namespace2016W[] = {'h','t','t','p',':','/','/','s','c','h','e','m','a','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','S','M','I','/','2','0','1','6','/','W','i','n','d','o','w','s','S','e','t','t','i','n','g','s',0};

    if (!LdrQueryImageFileExecutionOptions( &NtCurrentTeb()->Peb->ProcessParameters->ImagePathName,
                                            dpiAwarenessW, REG_DWORD, &option, sizeof(option), NULL ))
    {
        TRACE( "got option %x\n", option );
        if (option <= 2)
        {
            SetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~(ULONG_PTR)option );
            return;
        }
    }

    if (QueryActCtxSettingsW( 0, NULL, namespace2016W, dpiAwarenessW, buffer, ARRAY_SIZE(buffer), NULL ))
    {
        static const WCHAR unawareW[] = {'u','n','a','w','a','r','e',0};
        static const WCHAR systemW[] = {'s','y','s','t','e','m',0};
        static const WCHAR permonW[] = {'p','e','r','m','o','n','i','t','o','r',0};
        static const WCHAR permonv2W[] = {'p','e','r','m','o','n','i','t','o','r','v','2',0};
        static const WCHAR spacesW[] = {' ','\t','\r','\n',0};
        static const WCHAR * const types[] = { unawareW, systemW, permonW, permonv2W };
        WCHAR *p, *start = buffer, *end;
        ULONG_PTR i;

        TRACE( "got dpiAwareness=%s\n", debugstr_w(buffer) );
        for (start = buffer; *start; start = end)
        {
            start += strspnW( start, spacesW );
            if (!(end = strchrW( start, ',' ))) end = start + strlenW(start);
            else *end++ = 0;
            if ((p = strpbrkW( start, spacesW ))) *p = 0;
            for (i = 0; i < ARRAY_SIZE(types); i++)
            {
                if (strcmpiW( start, types[i] )) continue;
                SetProcessDpiAwarenessContext( (DPI_AWARENESS_CONTEXT)~i );
                return;
            }
        }
    }
    else if (QueryActCtxSettingsW( 0, NULL, namespace2005W, dpiAwareW, buffer, ARRAY_SIZE(buffer), NULL ))
    {
        static const WCHAR trueW[] = {'t','r','u','e',0};
        static const WCHAR truepmW[] = {'t','r','u','e','/','p','m',0};
        static const WCHAR permonW[] = {'p','e','r',' ','m','o','n','i','t','o','r',0};

        TRACE( "got dpiAware=%s\n", debugstr_w(buffer) );
        if (!strcmpiW( buffer, trueW ))
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_SYSTEM_AWARE );
        else if (!strcmpiW( buffer, truepmW ) || !strcmpiW( buffer, permonW ))
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE );
        else
            SetProcessDpiAwarenessContext( DPI_AWARENESS_CONTEXT_UNAWARE );
    }
}


/***********************************************************************
 *           winstation_init
 *
 * Connect to the process window station and desktop.
 */
static void winstation_init(void)
{
    static const WCHAR WinSta0[] = {'W','i','n','S','t','a','0',0};

    STARTUPINFOW info;
    WCHAR *winstation = NULL, *desktop = NULL, *buffer = NULL;
    HANDLE handle;

    GetStartupInfoW( &info );
    if (info.lpDesktop && *info.lpDesktop)
    {
        buffer = HeapAlloc( GetProcessHeap(), 0, (strlenW(info.lpDesktop) + 1) * sizeof(WCHAR) );
        strcpyW( buffer, info.lpDesktop );
        if ((desktop = strchrW( buffer, '\\' )))
        {
            *desktop++ = 0;
            winstation = buffer;
        }
        else desktop = buffer;
    }

    /* set winstation if explicitly specified, or if we don't have one yet */
    if (buffer || !GetProcessWindowStation())
    {
        handle = CreateWindowStationW( winstation ? winstation : WinSta0, 0, WINSTA_ALL_ACCESS, NULL );
        if (handle)
        {
            SetProcessWindowStation( handle );
            /* only WinSta0 is visible */
            if (!winstation || !strcmpiW( winstation, WinSta0 ))
            {
                USEROBJECTFLAGS flags;
                flags.fInherit  = FALSE;
                flags.fReserved = FALSE;
                flags.dwFlags   = WSF_VISIBLE;
                SetUserObjectInformationW( handle, UOI_FLAGS, &flags, sizeof(flags) );
            }
        }
    }
    if (buffer || !GetThreadDesktop( GetCurrentThreadId() ))
    {
        handle = CreateDesktopW( desktop ? desktop : get_default_desktop(),
                                 NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
        if (handle) SetThreadDesktop( handle );
    }
    HeapFree( GetProcessHeap(), 0, buffer );

    register_desktop_class();
}


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    dpiaware_init();
    winstation_init();

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
    static const WCHAR imm32_dllW[] = {'i','m','m','3','2','.','d','l','l',0};
    static HMODULE imm32_module;
    BOOL ret = TRUE;

    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        user32_module = inst;
        ret = process_attach();
        if(ret)
            imm32_module = LoadLibraryW(imm32_dllW);
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
    static const WCHAR winebootW[]    = { '\\','w','i','n','e','b','o','o','t','.','e','x','e',0 };
    static const WCHAR killW[]        = { ' ','-','-','k','i','l','l',0 };
    static const WCHAR end_sessionW[] = { ' ','-','-','e','n','d','-','s','e','s','s','i','o','n',0 };
    static const WCHAR forceW[]       = { ' ','-','-','f','o','r','c','e',0 };
    static const WCHAR shutdownW[]    = { ' ','-','-','s','h','u','t','d','o','w','n',0 };

    WCHAR app[MAX_PATH];
    WCHAR cmdline[MAX_PATH + 64];
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    void *redir;

    GetSystemDirectoryW( app, MAX_PATH - sizeof(winebootW)/sizeof(WCHAR) );
    strcatW( app, winebootW );
    strcpyW( cmdline, app );

    if (flags & EWX_FORCE) lstrcatW( cmdline, killW );
    else
    {
        lstrcatW( cmdline, end_sessionW );
        if (flags & EWX_FORCEIFHUNG) lstrcatW( cmdline, forceW );
    }
    if (!(flags & EWX_REBOOT)) lstrcatW( cmdline, shutdownW );

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
