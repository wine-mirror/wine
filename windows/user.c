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
#include "user.h"
#include "win.h"
#include "controls.h"
#include "cursoricon.h"
#include "message.h"
#include "local.h"
#include "wine/debug.h"

WINE_DECLARE_DEBUG_CHANNEL(hook);
WINE_DECLARE_DEBUG_CHANNEL(local);
WINE_DECLARE_DEBUG_CHANNEL(system);
WINE_DECLARE_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(win32);

static SYSLEVEL USER_SysLevel;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &USER_SysLevel.crst,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": USER_SysLevel") }
};
static SYSLEVEL USER_SysLevel = { { &critsect_debug, -1, 0, 0, 0, 0 }, 2 };

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


/***********************************************************************
 *		GetFreeSystemResources (USER.284)
 */
WORD WINAPI GetFreeSystemResources16( WORD resType )
{
    HINSTANCE16 gdi_inst;
    WORD gdi_heap;
    int userPercent, gdiPercent;

    if ((gdi_inst = LoadLibrary16( "GDI" )) < 32) return 0;
    gdi_heap = gdi_inst | 7;

    switch(resType)
    {
    case GFSR_USERRESOURCES:
        userPercent = (int)LOCAL_CountFree( USER_HeapSel ) * 100 /
                               LOCAL_HeapSize( USER_HeapSel );
        gdiPercent  = 100;
        break;

    case GFSR_GDIRESOURCES:
        gdiPercent  = (int)LOCAL_CountFree( gdi_inst ) * 100 /
                               LOCAL_HeapSize( gdi_inst );
        userPercent = 100;
        break;

    case GFSR_SYSTEMRESOURCES:
        userPercent = (int)LOCAL_CountFree( USER_HeapSel ) * 100 /
                               LOCAL_HeapSize( USER_HeapSel );
        gdiPercent  = (int)LOCAL_CountFree( gdi_inst ) * 100 /
                               LOCAL_HeapSize( gdi_inst );
        break;

    default:
        userPercent = gdiPercent = 0;
        break;
    }
    FreeLibrary16( gdi_inst );
    TRACE_(local)("<- userPercent %d, gdiPercent %d\n", userPercent, gdiPercent);
    return (WORD)min( userPercent, gdiPercent );
}


/**********************************************************************
 *		InitApp (USER.5)
 */
INT16 WINAPI InitApp16( HINSTANCE16 hInstance )
{
    /* Create task message queue */
    if ( !InitThreadInput16( 0, 0 ) ) return 0;

    return 1;
}


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
 *           WIN_SuspendWndsLock
 *
 * Suspend the lock on WND structures.
 * Returns the number of locks suspended
 * FIXME: should be removed
 */
int WIN_SuspendWndsLock( void )
{
    int isuspendedLocks = _ConfirmSysLevel( &USER_SysLevel );
    int count = isuspendedLocks;

    while ( count-- > 0 )
        _LeaveSysLevel( &USER_SysLevel );

    return isuspendedLocks;
}

/***********************************************************************
 *           WIN_RestoreWndsLock
 *
 * Restore the suspended locks on WND structures
 * FIXME: should be removed
 */
void WIN_RestoreWndsLock( int ipreviousLocks )
{
    while ( ipreviousLocks-- > 0 )
        _EnterSysLevel( &USER_SysLevel );
}

/***********************************************************************
 *		FinalUserInit (USER.400)
 */
void WINAPI FinalUserInit16( void )
{
    /* FIXME: Should chain to FinalGdiInit now. */
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
    FIXME_(win)("(%04x, %08lx, %04lx, %04x)\n",
                uCode, dwThreadOrProcessID, dwFlags, hModule );
    /* FIXME: Should chain to GdiSignalProc now. */
    return 0;
}

/***********************************************************************
 *		ExitWindows (USER.7)
 */
BOOL16 WINAPI ExitWindows16( DWORD dwReturnCode, UINT16 wReserved )
{
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *		ExitWindowsExec (USER.246)
 */
BOOL16 WINAPI ExitWindowsExec16( LPCSTR lpszExe, LPCSTR lpszParams )
{
    TRACE_(system)("Should run the following in DOS-mode: \"%s %s\"\n",
	lpszExe, lpszParams);
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *		ExitWindowsEx (USER32.@)
 */
BOOL WINAPI ExitWindowsEx( UINT flags, DWORD reserved )
{
    int i;
    BOOL result;
    HWND *list, *phwnd;

    /* We have to build a list of all windows first, as in EnumWindows */

    if (!(list = WIN_ListChildren( GetDesktopWindow() ))) return FALSE;

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

    if (result) ExitKernel16();
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
 *		ChangeDisplaySettings (USER.620)
 */
LONG WINAPI ChangeDisplaySettings16( LPDEVMODEA devmode, DWORD flags )
{
	return ChangeDisplaySettingsA(devmode, flags);
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
 *		EnumDisplaySettings (USER.621)
 */
BOOL16 WINAPI EnumDisplaySettings16(
	LPCSTR name,		/* [in] huh? */
	DWORD n,		/* [in] nth entry in display settings list*/
	LPDEVMODEA devmode	/* [out] devmode for that setting */
) {
	return (BOOL16)EnumDisplaySettingsA(name, n, devmode);
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
	FIXME_(system)("(%p,%ld,%p,0x%08lx), stub!\n",unused,i,lpDisplayDevice,dwFlags);
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
	FIXME_(system)("(%p,%ld,%p,0x%08lx), stub!\n",unused,i,lpDisplayDevice,dwFlags);
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
 *		SetEventHook (USER.321)
 *
 *	Used by Turbo Debugger for Windows
 */
FARPROC16 WINAPI SetEventHook16(FARPROC16 lpfnEventHook)
{
	FIXME_(hook)("(lpfnEventHook=%08x): stub\n", (UINT)lpfnEventHook);
	return NULL;
}

/***********************************************************************
 *		UserSeeUserDo (USER.216)
 */
DWORD WINAPI UserSeeUserDo16(WORD wReqType, WORD wParam1, WORD wParam2, WORD wParam3)
{
    switch (wReqType)
    {
    case USUD_LOCALALLOC:
        return LOCAL_Alloc(USER_HeapSel, wParam1, wParam3);
    case USUD_LOCALFREE:
        return LOCAL_Free(USER_HeapSel, wParam1);
    case USUD_LOCALCOMPACT:
        return LOCAL_Compact(USER_HeapSel, wParam3, 0);
    case USUD_LOCALHEAP:
        return USER_HeapSel;
    case USUD_FIRSTCLASS:
        FIXME_(local)("return a pointer to the first window class.\n");
        return (DWORD)-1;
    default:
        WARN_(local)("wReqType %04x (unknown)\n", wReqType);
        return (DWORD)-1;
    }
}

/***********************************************************************
 *		GetSystemDebugState (USER.231)
 */
WORD WINAPI GetSystemDebugState16(void)
{
    return 0;  /* FIXME */
}

/***********************************************************************
 *		RegisterLogonProcess (USER32.@)
 */
DWORD WINAPI RegisterLogonProcess(HANDLE hprocess,BOOL x) {
	FIXME_(win32)("(%p,%d),stub!\n",hprocess,x);
	return 1;
}

/***********************************************************************
 *		CreateWindowStationW (USER32.@)
 */
HWINSTA WINAPI CreateWindowStationW(
	LPWSTR winstation,DWORD res1,DWORD desiredaccess,
	LPSECURITY_ATTRIBUTES lpsa
) {
	FIXME_(win32)("(%s,0x%08lx,0x%08lx,%p),stub!\n",debugstr_w(winstation),
		res1,desiredaccess,lpsa
	);
	return (HWINSTA)0xdeadcafe;
}

/***********************************************************************
 *		SetProcessWindowStation (USER32.@)
 */
BOOL WINAPI SetProcessWindowStation(HWINSTA hWinSta) {
	FIXME_(win32)("(%p),stub!\n",hWinSta);
	return TRUE;
}

/***********************************************************************
 *		SetUserObjectSecurity (USER32.@)
 */
BOOL WINAPI SetUserObjectSecurity(
	HANDLE hObj,
	PSECURITY_INFORMATION pSIRequested,
	PSECURITY_DESCRIPTOR pSID
) {
	FIXME_(win32)("(%p,%p,%p),stub!\n",hObj,pSIRequested,pSID);
	return TRUE;
}

/***********************************************************************
 *		CreateDesktopA (USER32.@)
 */
HDESK WINAPI CreateDesktopA(
	LPSTR lpszDesktop,LPSTR lpszDevice,LPDEVMODEA pDevmode,
	DWORD dwFlags,DWORD dwDesiredAccess,LPSECURITY_ATTRIBUTES lpsa
) {
	FIXME_(win32)("(%s,%s,%p,0x%08lx,0x%08lx,%p),stub!\n",
		lpszDesktop,lpszDevice,pDevmode,
		dwFlags,dwDesiredAccess,lpsa
	);
	return (HDESK)0xcafedead;
}

/***********************************************************************
 *		CreateDesktopW (USER32.@)
 */
HDESK WINAPI CreateDesktopW(
	LPWSTR lpszDesktop,LPWSTR lpszDevice,LPDEVMODEW pDevmode,
	DWORD dwFlags,DWORD dwDesiredAccess,LPSECURITY_ATTRIBUTES lpsa
) {
	FIXME_(win32)("(%s,%s,%p,0x%08lx,0x%08lx,%p),stub!\n",
		debugstr_w(lpszDesktop),debugstr_w(lpszDevice),pDevmode,
		dwFlags,dwDesiredAccess,lpsa
	);
	return (HDESK)0xcafedead;
}

/***********************************************************************
 *		EnumDesktopWindows (USER32.@)
 */
BOOL WINAPI EnumDesktopWindows( HDESK hDesktop, WNDENUMPROC lpfn, LPARAM lParam ) {
  FIXME_(win32)("(%p, %p, 0x%08lx), stub!\n", hDesktop, lpfn, lParam );
  return TRUE;
}


/***********************************************************************
 *		CloseWindowStation (USER32.@)
 */
BOOL WINAPI CloseWindowStation(HWINSTA hWinSta)
{
    FIXME_(win32)("(%p)\n", hWinSta);
    return TRUE;
}

/***********************************************************************
 *		CloseDesktop (USER32.@)
 */
BOOL WINAPI CloseDesktop(HDESK hDesk)
{
    FIXME_(win32)("(%p)\n", hDesk);
    return TRUE;
}

/***********************************************************************
 *		SetWindowStationUser (USER32.@)
 */
DWORD WINAPI SetWindowStationUser(DWORD x1,DWORD x2) {
	FIXME_(win32)("(0x%08lx,0x%08lx),stub!\n",x1,x2);
	return 1;
}

/***********************************************************************
 *		SetLogonNotifyWindow (USER32.@)
 */
DWORD WINAPI SetLogonNotifyWindow(HWINSTA hwinsta,HWND hwnd) {
	FIXME_(win32)("(%p,%p),stub!\n",hwinsta,hwnd);
	return 1;
}

/***********************************************************************
 *		LoadLocalFonts (USER32.@)
 */
VOID WINAPI LoadLocalFonts(VOID) {
	/* are loaded. */
	return;
}
/***********************************************************************
 *		GetUserObjectInformationA (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationA( HANDLE hObj, INT nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{	FIXME_(win32)("(%p %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
	return TRUE;
}
/***********************************************************************
 *		GetUserObjectInformationW (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationW( HANDLE hObj, INT nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{	FIXME_(win32)("(%p %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
	return TRUE;
}
/***********************************************************************
 *		GetUserObjectSecurity (USER32.@)
 */
BOOL WINAPI GetUserObjectSecurity(HANDLE hObj, PSECURITY_INFORMATION pSIRequested,
	PSECURITY_DESCRIPTOR pSID, DWORD nLength, LPDWORD lpnLengthNeeded)
{	FIXME_(win32)("(%p %p %p len=%ld %p),stub!\n",  hObj, pSIRequested, pSID, nLength, lpnLengthNeeded);
	return TRUE;
}

/***********************************************************************
 *		SetSystemCursor (USER32.@)
 */
BOOL WINAPI SetSystemCursor(HCURSOR hcur, DWORD id)
{	FIXME_(win32)("(%p,%08lx),stub!\n",  hcur, id);
	return TRUE;
}

/***********************************************************************
 *		RegisterSystemThread (USER32.@)
 */
void WINAPI RegisterSystemThread(DWORD flags, DWORD reserved)
{
	FIXME_(win32)("(%08lx, %08lx)\n", flags, reserved);
}

/***********************************************************************
 *		RegisterDeviceNotificationA (USER32.@)
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationA(
	HANDLE hnd, LPVOID notifyfilter, DWORD flags
) {
    FIXME_(win32)("(hwnd=%p, filter=%p,flags=0x%08lx), STUB!\n", hnd,notifyfilter,flags );
    return 0;
}
