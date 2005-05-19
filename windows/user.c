/*
 * Misc. USER functions
 *
 * Copyright 1993 Robert J. Amstadt
 *	     1996 Alex Korobka
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "winreg.h"
#include "winternl.h"
#include "tlhelp32.h"
#include "user_private.h"
#include "win.h"
#include "controls.h"
#include "cursoricon.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(user);

/* USER signal proc flags and codes */
/* See UserSignalProc for comments */
#define USIG_FLAGS_WIN32          0x0001
#define USIG_FLAGS_GUI            0x0002
#define USIG_FLAGS_FEEDBACK       0x0004
#define USIG_FLAGS_FAULT          0x0008

#define USIG_DLL_UNLOAD_WIN16     0x0001
#define USIG_DLL_UNLOAD_WIN32     0x0002
#define USIG_FAULT_DIALOG_PUSH    0x0003
#define USIG_FAULT_DIALOG_POP     0x0004
#define USIG_DLL_UNLOAD_ORPHANS   0x0005
#define USIG_THREAD_INIT          0x0010
#define USIG_THREAD_EXIT          0x0020
#define USIG_PROCESS_CREATE       0x0100
#define USIG_PROCESS_INIT         0x0200
#define USIG_PROCESS_EXIT         0x0300
#define USIG_PROCESS_DESTROY      0x0400
#define USIG_PROCESS_RUNNING      0x0500
#define USIG_PROCESS_LOADED       0x0600

/* UserSeeUserDo parameters */
#define USUD_LOCALALLOC        0x0001
#define USUD_LOCALFREE         0x0002
#define USUD_LOCALCOMPACT      0x0003
#define USUD_LOCALHEAP         0x0004
#define USUD_FIRSTCLASS        0x0005

/***********************************************************************
 *		GetFreeSystemResources (USER.284)
 */
WORD WINAPI GetFreeSystemResources16( WORD resType )
{
    STACK16FRAME* stack16 = MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved);
    HANDLE16 oldDS = stack16->ds;
    HINSTANCE16 gdi_inst;
    WORD gdi_heap;
    int userPercent, gdiPercent;

    if ((gdi_inst = LoadLibrary16( "GDI" )) < 32) return 0;
    gdi_heap = gdi_inst | 7;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        stack16->ds = USER_HeapSel;
        userPercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        gdiPercent  = 100;
        stack16->ds = oldDS;
        break;

    case GFSR_GDIRESOURCES:
        stack16->ds = gdi_inst;
        gdiPercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        userPercent = 100;
        stack16->ds = oldDS;
        break;

    case GFSR_SYSTEMRESOURCES:
        stack16->ds = USER_HeapSel;
        userPercent = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        stack16->ds = gdi_inst;
        gdiPercent  = (int)LocalCountFree16() * 100 / LocalHeapSize16();
        stack16->ds = oldDS;
        break;

    default:
        userPercent = gdiPercent = 0;
        break;
    }
    FreeLibrary16( gdi_inst );
    TRACE("<- userPercent %d, gdiPercent %d\n", userPercent, gdiPercent);
    return (WORD)min( userPercent, gdiPercent );
}


/***********************************************************************
 *		SignalProc32 (USER.391)
 *		UserSignalProc (USER32.@)
 *
 * The exact meaning of the USER signals is undocumented, but this
 * should cover the basic idea:
 *
 * USIG_DLL_UNLOAD_WIN16
 *     This is sent when a 16-bit module is unloaded.
 *
 * USIG_DLL_UNLOAD_WIN32
 *     This is sent when a 32-bit module is unloaded.
 *
 * USIG_DLL_UNLOAD_ORPHANS
 *     This is sent after the last Win3.1 module is unloaded,
 *     to allow removal of orphaned menus.
 *
 * USIG_FAULT_DIALOG_PUSH
 * USIG_FAULT_DIALOG_POP
 *     These are called to allow USER to prepare for displaying a
 *     fault dialog, even though the fault might have happened while
 *     inside a USER critical section.
 *
 * USIG_THREAD_INIT
 *     This is called from the context of a new thread, as soon as it
 *     has started to run.
 *
 * USIG_THREAD_EXIT
 *     This is called, still in its context, just before a thread is
 *     about to terminate.
 *
 * USIG_PROCESS_CREATE
 *     This is called, in the parent process context, after a new process
 *     has been created.
 *
 * USIG_PROCESS_INIT
 *     This is called in the new process context, just after the main thread
 *     has started execution (after the main thread's USIG_THREAD_INIT has
 *     been sent).
 *
 * USIG_PROCESS_LOADED
 *     This is called after the executable file has been loaded into the
 *     new process context.
 *
 * USIG_PROCESS_RUNNING
 *     This is called immediately before the main entry point is called.
 *
 * USIG_PROCESS_EXIT
 *     This is called in the context of a process that is about to
 *     terminate (but before the last thread's USIG_THREAD_EXIT has
 *     been sent).
 *
 * USIG_PROCESS_DESTROY
 *     This is called after a process has terminated.
 *
 *
 * The meaning of the dwFlags bits is as follows:
 *
 * USIG_FLAGS_WIN32
 *     Current process is 32-bit.
 *
 * USIG_FLAGS_GUI
 *     Current process is a (Win32) GUI process.
 *
 * USIG_FLAGS_FEEDBACK
 *     Current process needs 'feedback' (determined from the STARTUPINFO
 *     flags STARTF_FORCEONFEEDBACK / STARTF_FORCEOFFFEEDBACK).
 *
 * USIG_FLAGS_FAULT
 *     The signal is being sent due to a fault.
 */
WORD WINAPI UserSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                            DWORD dwFlags, HMODULE16 hModule )
{
    FIXME("(%04x, %08lx, %04lx, %04x)\n",
          uCode, dwThreadOrProcessID, dwFlags, hModule );
    /* FIXME: Should chain to GdiSignalProc now. */
    return 0;
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
 *		USER_StartRebootProcess (Internal)
 */
static BOOL USER_StartRebootProcess(void)
{
    WCHAR winebootW[] = { 'w','i','n','e','b','o','o','t',0 };
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    BOOL r;

    memset( &si, 0, sizeof si );
    si.cb = sizeof si;

    r = CreateProcessW( NULL, winebootW, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi );
    if (r)
    {
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }
    else
        MESSAGE("wine: Failed to start wineboot\n");

    return r;
}

/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT flags, DWORD reserved )
{
    int i;
    BOOL result = FALSE;
    HWND *list, *phwnd;

    /* We have to build a list of all windows first, as in EnumWindows */
    TRACE("(%x,%lx)\n", flags, reserved);

    list = WIN_ListChildren( GetDesktopWindow() );
    if (list)
    {
        /* Send a WM_QUERYENDSESSION message to every window */

        for (i = 0; list[i]; i++)
        {
            /* Make sure that the window still exists */
            if (!IsWindow( list[i] )) continue;
            if (!SendMessageW( list[i], WM_QUERYENDSESSION, 0, 0 )) break;
        }
        result = !list[i];

        /* Now notify all windows that got a WM_QUERYENDSESSION of the result */

        for (phwnd = list; i > 0; i--, phwnd++)
        {
            if (!IsWindow( *phwnd )) continue;
            SendMessageW( *phwnd, WM_ENDSESSION, result, 0 );
        }
        HeapFree( GetProcessHeap(), 0, list );

        if ( !(result || (flags & EWX_FORCE) ))
            return FALSE;
    }

    /* USER_DoShutdown will kill all processes except the current process */
    USER_DoShutdown();

    if (flags & EWX_REBOOT)
        USER_StartRebootProcess();

    if (result) ExitProcess(0);
    return TRUE;
}

/***********************************************************************
 *		ChangeDisplaySettingsA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsA( LPDEVMODEA devmode, DWORD flags )
{
  return ChangeDisplaySettingsExA(NULL,devmode,NULL,flags,NULL);
}

/***********************************************************************
 *		ChangeDisplaySettingsW (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsW( LPDEVMODEW devmode, DWORD flags )
{
  return ChangeDisplaySettingsExW(NULL,devmode,NULL,flags,NULL);
}

/***********************************************************************
 *		ChangeDisplaySettingsExA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExA(
	LPCSTR devname, LPDEVMODEA devmode, HWND hwnd, DWORD flags,
	LPVOID lparam
) {
    DEVMODEW devmodeW;
    LONG ret;
    UNICODE_STRING nameW;

    if (devname) RtlCreateUnicodeStringFromAsciiz(&nameW, devname);
    else nameW.Buffer = NULL;

    if (devmode)
    {
        devmodeW.dmBitsPerPel       = devmode->dmBitsPerPel;
        devmodeW.dmPelsHeight       = devmode->dmPelsHeight;
        devmodeW.dmPelsWidth        = devmode->dmPelsWidth;
        devmodeW.dmDisplayFlags     = devmode->dmDisplayFlags;
        devmodeW.dmDisplayFrequency = devmode->dmDisplayFrequency;
        devmodeW.dmFields           = devmode->dmFields;
        ret = ChangeDisplaySettingsExW(nameW.Buffer, &devmodeW, hwnd, flags, lparam);
    }
    else
    {
        ret = ChangeDisplaySettingsExW(nameW.Buffer, NULL, hwnd, flags, lparam);
    }

    if (devname) RtlFreeUnicodeString(&nameW);
    return ret;
}

/***********************************************************************
 *		ChangeDisplaySettingsExW (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExW( LPCWSTR devname, LPDEVMODEW devmode, HWND hwnd,
                                      DWORD flags, LPVOID lparam )
{
    /* Pass the request on to the driver */
    if (!USER_Driver.pChangeDisplaySettingsExW) return DISP_CHANGE_FAILED;
    return USER_Driver.pChangeDisplaySettingsExW( devname, devmode, hwnd, flags, lparam );
}

/***********************************************************************
 *		EnumDisplaySettingsW (USER32.@)
 *
 * RETURNS
 *	TRUE if nth setting exists found (described in the LPDEVMODEW struct)
 *	FALSE if we do not have the nth setting
 */
BOOL WINAPI EnumDisplaySettingsW(
	LPCWSTR name,		/* [in] huh? */
	DWORD n,		/* [in] nth entry in display settings list*/
	LPDEVMODEW devmode	/* [out] devmode for that setting */
) {
    return EnumDisplaySettingsExW(name, n, devmode, 0);
}

/***********************************************************************
 *		EnumDisplaySettingsA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsA(LPCSTR name,DWORD n,LPDEVMODEA devmode)
{
    return EnumDisplaySettingsExA(name, n, devmode, 0);
}

/***********************************************************************
 *		EnumDisplaySettingsExA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExA(LPCSTR lpszDeviceName, DWORD iModeNum,
				   LPDEVMODEA lpDevMode, DWORD dwFlags)
{
    DEVMODEW devmodeW;
    BOOL ret;
    UNICODE_STRING nameW;

    if (lpszDeviceName) RtlCreateUnicodeStringFromAsciiz(&nameW, lpszDeviceName);
    else nameW.Buffer = NULL;

    ret = EnumDisplaySettingsExW(nameW.Buffer,iModeNum,&devmodeW,dwFlags);
    if (ret)
    {
        lpDevMode->dmBitsPerPel       = devmodeW.dmBitsPerPel;
        lpDevMode->dmPelsHeight       = devmodeW.dmPelsHeight;
        lpDevMode->dmPelsWidth        = devmodeW.dmPelsWidth;
        lpDevMode->dmDisplayFlags     = devmodeW.dmDisplayFlags;
        lpDevMode->dmDisplayFrequency = devmodeW.dmDisplayFrequency;
        lpDevMode->dmFields           = devmodeW.dmFields;
    }
    if (lpszDeviceName) RtlFreeUnicodeString(&nameW);
    return ret;
}

/***********************************************************************
 *		EnumDisplaySettingsExW (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum,
				   LPDEVMODEW lpDevMode, DWORD dwFlags)
{
    /* Pass the request on to the driver */
    if (!USER_Driver.pEnumDisplaySettingsExW) return FALSE;
    return USER_Driver.pEnumDisplaySettingsExW(lpszDeviceName, iModeNum, lpDevMode, dwFlags);
}

/***********************************************************************
 *		EnumDisplayDevicesA (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesA(
	LPVOID unused,DWORD i,LPDISPLAY_DEVICEA lpDisplayDevice,DWORD dwFlags
) {
	if (i)
		return FALSE;
	FIXME("(%p,%ld,%p,0x%08lx), stub!\n",unused,i,lpDisplayDevice,dwFlags);
	strcpy(lpDisplayDevice->DeviceName,"X11");
	strcpy(lpDisplayDevice->DeviceString,"X 11 Windowing System");
	lpDisplayDevice->StateFlags =
			DISPLAY_DEVICE_ATTACHED_TO_DESKTOP	|
			DISPLAY_DEVICE_PRIMARY_DEVICE		|
			DISPLAY_DEVICE_VGA_COMPATIBLE;
	return TRUE;
}

/***********************************************************************
 *		EnumDisplayDevicesW (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesW(
	LPVOID unused,DWORD i,LPDISPLAY_DEVICEW lpDisplayDevice,DWORD dwFlags
) {
	if (i)
		return FALSE;
	FIXME("(%p,%ld,%p,0x%08lx), stub!\n",unused,i,lpDisplayDevice,dwFlags);
        MultiByteToWideChar( CP_ACP, 0, "X11", -1, lpDisplayDevice->DeviceName,
                             sizeof(lpDisplayDevice->DeviceName)/sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, "X11 Windowing System", -1, lpDisplayDevice->DeviceString,
                             sizeof(lpDisplayDevice->DeviceString)/sizeof(WCHAR) );
	lpDisplayDevice->StateFlags =
			DISPLAY_DEVICE_ATTACHED_TO_DESKTOP	|
			DISPLAY_DEVICE_PRIMARY_DEVICE		|
			DISPLAY_DEVICE_VGA_COMPATIBLE;
	return TRUE;
}

/***********************************************************************
 *		UserSeeUserDo (USER.216)
 */
DWORD WINAPI UserSeeUserDo16(WORD wReqType, WORD wParam1, WORD wParam2, WORD wParam3)
{
    STACK16FRAME* stack16 = MapSL((SEGPTR)NtCurrentTeb()->WOW32Reserved);
    HANDLE16 oldDS = stack16->ds;
    DWORD ret = (DWORD)-1;

    stack16->ds = USER_HeapSel;
    switch (wReqType)
    {
    case USUD_LOCALALLOC:
        ret = LocalAlloc16(wParam1, wParam3);
        break;
    case USUD_LOCALFREE:
        ret = LocalFree16(wParam1);
        break;
    case USUD_LOCALCOMPACT:
        ret = LocalCompact16(wParam3);
        break;
    case USUD_LOCALHEAP:
        ret = USER_HeapSel;
        break;
    case USUD_FIRSTCLASS:
        FIXME("return a pointer to the first window class.\n");
        break;
    default:
        WARN("wReqType %04x (unknown)\n", wReqType);
    }
    stack16->ds = oldDS;
    return ret;
}

/***********************************************************************
 *		SetSystemCursor (USER32.@)
 */
BOOL WINAPI SetSystemCursor(HCURSOR hcur, DWORD id)
{	FIXME("(%p,%08lx),stub!\n",  hcur, id);
	return TRUE;
}
