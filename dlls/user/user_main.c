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
#include "winreg.h"
#include "tlhelp32.h"

#include "controls.h"
#include "user_private.h"
#include "win.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(graphics);

#define DESKTOP_ALL_ACCESS 0x01ff

WORD USER_HeapSel = 0;  /* USER heap selector */
HMODULE user32_module = 0;

static SYSLEVEL USER_SysLevel;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &USER_SysLevel.crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": USER_SysLevel") }
};
static SYSLEVEL USER_SysLevel = { { &critsect_debug, -1, 0, 0, 0, 0 }, 2 };

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
    _EnterSysLevel( &USER_SysLevel );
}


/***********************************************************************
 *           USER_Unlock
 */
void USER_Unlock(void)
{
    _LeaveSysLevel( &USER_SysLevel );
}


/***********************************************************************
 *           USER_CheckNotLock
 *
 * Make sure that we don't hold the user lock.
 */
void USER_CheckNotLock(void)
{
    _CheckNotSysLevel( &USER_SysLevel );
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
 *           winstation_init
 *
 * Connect to the process window station and desktop.
 */
static void winstation_init(void)
{
    static const WCHAR WinSta0[] = {'W','i','n','S','t','a','0',0};
    static const WCHAR Default[] = {'D','e','f','a','u','l','t',0};

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
        if (handle) SetProcessWindowStation( handle );
    }
    if (buffer || !GetThreadDesktop( GetCurrentThreadId() ))
    {
        handle = CreateDesktopW( desktop ? desktop : Default, NULL, NULL, 0, DESKTOP_ALL_ACCESS, NULL );
        if (handle) SetThreadDesktop( handle );
    }
    HeapFree( GetProcessHeap(), 0, buffer );
}


/***********************************************************************
 *           USER initialisation routine
 */
static BOOL process_attach(void)
{
    HINSTANCE16 instance;

    /* Create USER heap */
    if ((instance = LoadLibrary16( "USER.EXE" )) >= 32) USER_HeapSel = instance | 7;
    else
    {
        USER_HeapSel = GlobalAlloc16( GMEM_FIXED, 65536 );
        LocalInit16( USER_HeapSel, 32, 65534 );
    }

    /* some Win9x dlls expect keyboard to be loaded */
    if (GetVersion() & 0x80000000) LoadLibrary16( "keyboard.drv" );

    winstation_init();

    /* Initialize system colors and metrics */
    SYSPARAMS_Init();

    /* Setup palette function pointers */
    palette_init();

    /* Initialize built-in window classes */
    CLASS_RegisterBuiltinClasses();

    /* Initialize message spying */
    if (!SPY_Init()) return FALSE;

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
    exiting_thread_id = GetCurrentThreadId();

    WDML_NotifyThreadDetach();

    WIN_DestroyThreadWindows( get_user_thread_info()->desktop );
    CloseHandle( get_user_thread_info()->server_queue );

    exiting_thread_id = 0;
}


/***********************************************************************
 *           UserClientDllInitialize  (USER32.@)
 *
 * USER dll initialisation routine (exported as UserClientDllInitialize for compatibility).
 */
BOOL WINAPI DllMain( HINSTANCE inst, DWORD reason, LPVOID reserved )
{
    BOOL ret = TRUE;
    switch(reason)
    {
    case DLL_PROCESS_ATTACH:
        user32_module = inst;
        ret = process_attach();
        break;
    case DLL_THREAD_DETACH:
        thread_detach();
        break;
    case DLL_PROCESS_DETACH:
        USER_unload_driver();
        break;
    }
    return ret;
}


/***********************************************************************
 *           USER_GetProcessHandleList(Internal)
 */
static HANDLE *USER_GetProcessHandleList(void)
{
    DWORD count, i, n;
    HANDLE *list;
    PROCESSENTRY32 pe;
    HANDLE hSnapshot;
    BOOL r;

    hSnapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    if (!hSnapshot)
    {
        ERR("cannot create snapshot\n");
        return FALSE;
    }

    /* count the number of processes plus one */
    for (count=0; ;count++)
    {
        pe.dwSize = sizeof pe;
        if (count)
            r = Process32Next( hSnapshot, &pe );
        else
            r = Process32First( hSnapshot, &pe );
        if (!r)
            break;
    }

    /* allocate memory make a list of the process handles */
    list = HeapAlloc( GetProcessHeap(), 0, (count+1)*sizeof(HANDLE) );
    n=0;
    for (i=0; i<count; i++)
    {
        pe.dwSize = sizeof pe;
        if (i)
            r = Process32Next( hSnapshot, &pe );
        else
            r = Process32First( hSnapshot, &pe );
        if (!r)
            break;

        /* don't kill ourselves */
        if (GetCurrentProcessId() == pe.th32ProcessID )
            continue;

        /* open the process so we don't can track it */
        list[n] = OpenProcess( PROCESS_QUERY_INFORMATION|
                                  PROCESS_TERMINATE,
                                  FALSE, pe.th32ProcessID );

        /* check it didn't terminate already */
        if( list[n] )
            n++;
    }
    list[n]=0;
    CloseHandle( hSnapshot );

    if (!r)
        ERR("Error enumerating processes\n");

    TRACE("return %lu processes\n", n);

    return list;
}


/***********************************************************************
 *		USER_KillProcesses (Internal)
 */
static DWORD USER_KillProcesses(void)
{
    DWORD n, r, i;
    HANDLE *handles;
    const DWORD dwShutdownTimeout = 10000;

    TRACE("terminating other processes\n");

    /* kill it and add it to our list of object to wait on */
    handles = USER_GetProcessHandleList();
    for (n=0; handles && handles[n]; n++)
        TerminateProcess( handles[n], 0 );

    /* wait for processes to exit */
    for (i=0; i<n; i+=MAXIMUM_WAIT_OBJECTS)
    {
        int n_objs = ((n-i)>MAXIMUM_WAIT_OBJECTS) ? MAXIMUM_WAIT_OBJECTS : (n-i);
        r = WaitForMultipleObjects( n_objs, &handles[i], TRUE, dwShutdownTimeout );
        if (r==WAIT_TIMEOUT)
            ERR("wait failed!\n");
    }

    /* close the handles */
    for (i=0; i<n; i++)
        CloseHandle( handles[i] );

    HeapFree( GetProcessHeap(), 0, handles );

    return n;
}


/***********************************************************************
 *		USER_DoShutdown (Internal)
 */
static void USER_DoShutdown(void)
{
    DWORD i, n;
    const DWORD nRetries = 10;

    for (i=0; i<nRetries; i++)
    {
        n = USER_KillProcesses();
        TRACE("Killed %ld processes, attempt %ld\n", n, i);
        if(!n)
            break;
    }
}


/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT flags, DWORD reason )
{
    TRACE("(%x,%lx)\n", flags, reason);

    if ((flags & EWX_FORCE) == 0)
    {
        HWND *list;

        /* We have to build a list of all windows first, as in EnumWindows */
        list = WIN_ListChildren( GetDesktopWindow() );
        if (list)
        {
            HWND *phwnd;
            UINT send_flags;
            DWORD_PTR result=1;

            /* Send a WM_QUERYENDSESSION / WM_ENDSESSION message pair to
             * each window. Note: it might be better to send all the
             * WM_QUERYENDSESSION messages, aggregate the results and then
             * send all the WM_ENDSESSION messages with the results but
             * that's not what Windows does.
             */
            send_flags=(flags & EWX_FORCEIFHUNG) ? SMTO_ABORTIFHUNG : SMTO_NORMAL;
            for (phwnd = list; *phwnd; phwnd++)
            {
                /* Make sure that the window still exists */
                if (!IsWindow( *phwnd )) continue;
                if (SendMessageTimeoutW( *phwnd, WM_QUERYENDSESSION, 0, 0, send_flags, 0, &result))
                {
                    DWORD_PTR dummy;
                    SendMessageTimeoutW( *phwnd, WM_ENDSESSION, result, 0, send_flags, 0, &dummy );
                    if (!result) break;
                }
            }
            HeapFree( GetProcessHeap(), 0, list );

            if (!result)
                return TRUE;
        }
    }

    /* USER_DoShutdown will kill all processes except the current process */
    USER_DoShutdown();

    if (flags & EWX_REBOOT)
    {
        WCHAR winebootW[] = { 'w','i','n','e','b','o','o','t',0 };
        PROCESS_INFORMATION pi;
        STARTUPINFOW si;

        memset( &si, 0, sizeof si );
        si.cb = sizeof si;
        if (CreateProcessW( NULL, winebootW, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ))
        {
            CloseHandle( pi.hProcess );
            CloseHandle( pi.hThread );
        }
        else
            MESSAGE("wine: Failed to start wineboot\n");
    }

    ExitProcess(0);
    return TRUE;
}
