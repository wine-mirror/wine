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

#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "user.h"
#include "win.h"
#include "controls.h"
#include "cursoricon.h"
#include "message.h"
#include "miscemu.h"
#include "sysmetrics.h"
#include "local.h"
#include "module.h"
#include "winternl.h"
#include "wine/debug.h"

WINE_DECLARE_DEBUG_CHANNEL(hook);
WINE_DECLARE_DEBUG_CHANNEL(local);
WINE_DECLARE_DEBUG_CHANNEL(system);
WINE_DECLARE_DEBUG_CHANNEL(win);
WINE_DECLARE_DEBUG_CHANNEL(win32);

SYSLEVEL USER_SysLevel = { CRITICAL_SECTION_INIT("USER_SysLevel"), 2 };


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


/**********************************************************************
 *           USER_ModuleUnload
 */
static void USER_ModuleUnload( HMODULE16 hModule )
{
    /* HOOK_FreeModuleHooks( hModule ); */
    CLASS_FreeModuleClasses( hModule );
    CURSORICON_FreeModuleIcons( hModule );
}

/***********************************************************************
 *		SignalProc (USER.314)
 */
void WINAPI USER_SignalProc( HANDLE16 hTaskOrModule, UINT16 uCode,
                             UINT16 uExitFn, HINSTANCE16 hInstance,
                             HQUEUE16 hQueue )
{
    FIXME_(win)("Win 3.1 USER signal %04x\n", uCode );
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
 * For comments about the meaning of uCode and dwFlags
 * see PROCESS_CallUserSignalProc.
 *
 */
WORD WINAPI UserSignalProc( UINT uCode, DWORD dwThreadOrProcessID,
                            DWORD dwFlags, HMODULE16 hModule )
{
    /* FIXME: Proper reaction to most signals still missing. */

    switch ( uCode )
    {
    case USIG_DLL_UNLOAD_WIN16:
    case USIG_DLL_UNLOAD_WIN32:
        USER_ModuleUnload( hModule );
        break;

    case USIG_DLL_UNLOAD_ORPHANS:
    case USIG_FAULT_DIALOG_PUSH:
    case USIG_FAULT_DIALOG_POP:
    case USIG_THREAD_INIT:
    case USIG_THREAD_EXIT:
    case USIG_PROCESS_CREATE:
    case USIG_PROCESS_INIT:
    case USIG_PROCESS_LOADED:
    case USIG_PROCESS_RUNNING:
    case USIG_PROCESS_EXIT:
    case USIG_PROCESS_DESTROY:
      break;

    default:
        FIXME_(win)("(%04x, %08lx, %04lx, %04x)\n",
                    uCode, dwThreadOrProcessID, dwFlags, hModule );
        break;
    }

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
    return FALSE;
}

static void _dump_CDS_flags(DWORD flags) {
#define X(x) if (flags & CDS_##x) MESSAGE(""#x ",");
	X(UPDATEREGISTRY);X(TEST);X(FULLSCREEN);X(GLOBAL);
	X(SET_PRIMARY);X(RESET);X(SETRECT);X(NORESET);
#undef X
}

/***********************************************************************
 *		ChangeDisplaySettingsA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsA( LPDEVMODEA devmode, DWORD flags )
{
  FIXME_(system)("(%p,0x%08lx), stub\n",devmode,flags);
  MESSAGE("\tflags=");_dump_CDS_flags(flags);MESSAGE("\n");
  if (devmode==NULL)
    FIXME_(system)("   devmode=NULL (return to default mode)\n");
  else if ( (devmode->dmBitsPerPel != GetSystemMetrics(SM_WINE_BPP))
	    || (devmode->dmPelsHeight != GetSystemMetrics(SM_CYSCREEN))
	    || (devmode->dmPelsWidth != GetSystemMetrics(SM_CXSCREEN)) )

  {

    if (devmode->dmFields & DM_BITSPERPEL)
      FIXME_(system)("   bpp=%ld\n",devmode->dmBitsPerPel);
    if (devmode->dmFields & DM_PELSWIDTH)
      FIXME_(system)("   width=%ld\n",devmode->dmPelsWidth);
    if (devmode->dmFields & DM_PELSHEIGHT)
      FIXME_(system)("   height=%ld\n",devmode->dmPelsHeight);
    FIXME_(system)(" (Putting X in this mode beforehand might help)\n");
    /* we don't, but the program ... does not need to know */
    return DISP_CHANGE_SUCCESSFUL;
  }
  return DISP_CHANGE_SUCCESSFUL;
}

/***********************************************************************
 *		ChangeDisplaySettings (USER.620)
 */
LONG WINAPI ChangeDisplaySettings16( LPDEVMODEA devmode, DWORD flags )
{
	TRACE_(system)("(%p,0x%08lx), stub\n",devmode,flags);
	return ChangeDisplaySettingsA(devmode, flags);
}

/***********************************************************************
 *		ChangeDisplaySettingsExA (USER32.@)
 */
LONG WINAPI ChangeDisplaySettingsExA(
	LPCSTR devname, LPDEVMODEA devmode, HWND hwnd, DWORD flags,
	LPARAM lparam
) {
  FIXME_(system)("(%s,%p,0x%04x,0x%08lx,0x%08lx), stub\n",devname,devmode,hwnd,flags,lparam);
  MESSAGE("\tflags=");_dump_CDS_flags(flags);MESSAGE("\n");
  if (devmode==NULL)
    FIXME_(system)("   devmode=NULL (return to default mode)\n");
  else if ( (devmode->dmBitsPerPel != GetSystemMetrics(SM_WINE_BPP))
	    || (devmode->dmPelsHeight != GetSystemMetrics(SM_CYSCREEN))
	    || (devmode->dmPelsWidth != GetSystemMetrics(SM_CXSCREEN)) )

  {

    if (devmode->dmFields & DM_BITSPERPEL)
      FIXME_(system)("   bpp=%ld\n",devmode->dmBitsPerPel);
    if (devmode->dmFields & DM_PELSWIDTH)
      FIXME_(system)("   width=%ld\n",devmode->dmPelsWidth);
    if (devmode->dmFields & DM_PELSHEIGHT)
      FIXME_(system)("   height=%ld\n",devmode->dmPelsHeight);
    FIXME_(system)(" (Putting X in this mode beforehand might help)\n");
    /* we don't, but the program ... does not need to know */
    return DISP_CHANGE_SUCCESSFUL;
  }
  return DISP_CHANGE_SUCCESSFUL;
}

/***********************************************************************
 *		EnumDisplaySettingsW (USER32.@)
 * FIXME: Currently uses static list of modes.
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
#define NRMODES 5
#define NRDEPTHS 4
	struct {
		int w,h;
	} modes[NRMODES]={{512,384},{640,400},{640,480},{800,600},{1024,768}};
	int depths[4] = {8,16,24,32};

	TRACE_(system)("(%s,%ld,%p)\n",debugstr_w(name),n,devmode);
	devmode->dmDisplayFlags = 0;
	devmode->dmDisplayFrequency = 85;
	if (n==0 || n == (DWORD)-1 || n == (DWORD)-2) {
		devmode->dmBitsPerPel = GetSystemMetrics(SM_WINE_BPP);
		devmode->dmPelsHeight = GetSystemMetrics(SM_CYSCREEN);
		devmode->dmPelsWidth  = GetSystemMetrics(SM_CXSCREEN);
		return TRUE;
	}
	if ((n-1)<NRMODES*NRDEPTHS) {
		devmode->dmBitsPerPel	= depths[(n-1)/NRMODES];
		devmode->dmPelsHeight	= modes[(n-1)%NRMODES].h;
		devmode->dmPelsWidth	= modes[(n-1)%NRMODES].w;
		return TRUE;
	}
	return FALSE;
}

/***********************************************************************
 *		EnumDisplaySettingsA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsA(LPCSTR name,DWORD n,LPDEVMODEA devmode)
{
    DEVMODEW devmodeW;
    BOOL ret;
    UNICODE_STRING nameW;

    if (name) RtlCreateUnicodeStringFromAsciiz(&nameW, name);
    else nameW.Buffer = NULL;

    ret = EnumDisplaySettingsW(nameW.Buffer,n,&devmodeW);
    if (ret)
    {
        devmode->dmBitsPerPel       = devmodeW.dmBitsPerPel;
        devmode->dmPelsHeight       = devmodeW.dmPelsHeight;
        devmode->dmPelsWidth        = devmodeW.dmPelsWidth;
        devmode->dmDisplayFlags     = devmodeW.dmDisplayFlags;
        devmode->dmDisplayFrequency = devmodeW.dmDisplayFrequency;
        /* FIXME: convert rest too, if they are ever returned */
    }
    RtlFreeUnicodeString(&nameW);
    return ret;
}

/***********************************************************************
 *		EnumDisplaySettings (USER.621)
 */
BOOL16 WINAPI EnumDisplaySettings16(
	LPCSTR name,		/* [in] huh? */
	DWORD n,		/* [in] nth entry in display settings list*/
	LPDEVMODEA devmode	/* [out] devmode for that setting */
) {
	TRACE_(system)("(%s, %ld, %p)\n", name, n, devmode);
	return (BOOL16)EnumDisplaySettingsA(name, n, devmode);
}

/***********************************************************************
 *		EnumDisplaySettingsExA (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExA(LPCSTR lpszDeviceName, DWORD iModeNum,
				   LPDEVMODEA lpDevMode, DWORD dwFlags)
{
        TRACE_(system)("(%s,%lu,%p,%08lx): stub\n",
		       debugstr_a(lpszDeviceName), iModeNum, lpDevMode, dwFlags);

	return EnumDisplaySettingsA(lpszDeviceName, iModeNum, lpDevMode);
}

/***********************************************************************
 *		EnumDisplaySettingsExW (USER32.@)
 */
BOOL WINAPI EnumDisplaySettingsExW(LPCWSTR lpszDeviceName, DWORD iModeNum,
				   LPDEVMODEW lpDevMode, DWORD dwFlags)
{
	TRACE_(system)("(%s,%lu,%p,%08lx): stub\n",
			debugstr_w(lpszDeviceName), iModeNum, lpDevMode, dwFlags);

	return EnumDisplaySettingsW(lpszDeviceName, iModeNum, lpDevMode);
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
        WARN_(local)("wReqType %04x (unknown)", wReqType);
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
	FIXME_(win32)("(%d,%d),stub!\n",hprocess,x);
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
	FIXME_(win32)("(%d),stub!\n",hWinSta);
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
	FIXME_(win32)("(0x%08x,%p,%p),stub!\n",hObj,pSIRequested,pSID);
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
  FIXME_(win32)("(0x%08x, %p, 0x%08lx), stub!\n", hDesktop, lpfn, lParam );
  return TRUE;
}


/***********************************************************************
 *		CloseWindowStation (USER32.@)
 */
BOOL WINAPI CloseWindowStation(HWINSTA hWinSta)
{
    FIXME_(win32)("(0x%08x)\n", hWinSta);
    return TRUE;
}

/***********************************************************************
 *		CloseDesktop (USER32.@)
 */
BOOL WINAPI CloseDesktop(HDESK hDesk)
{
    FIXME_(win32)("(0x%08x)\n", hDesk);
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
	FIXME_(win32)("(0x%x,%04x),stub!\n",hwinsta,hwnd);
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
{	FIXME_(win32)("(0x%x %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
	return TRUE;
}
/***********************************************************************
 *		GetUserObjectInformationW (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationW( HANDLE hObj, INT nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{	FIXME_(win32)("(0x%x %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
	return TRUE;
}
/***********************************************************************
 *		GetUserObjectSecurity (USER32.@)
 */
BOOL WINAPI GetUserObjectSecurity(HANDLE hObj, PSECURITY_INFORMATION pSIRequested,
	PSECURITY_DESCRIPTOR pSID, DWORD nLength, LPDWORD lpnLengthNeeded)
{	FIXME_(win32)("(0x%x %p %p len=%ld %p),stub!\n",  hObj, pSIRequested, pSID, nLength, lpnLengthNeeded);
	return TRUE;
}

/***********************************************************************
 *		SetSystemCursor (USER32.@)
 */
BOOL WINAPI SetSystemCursor(HCURSOR hcur, DWORD id)
{	FIXME_(win32)("(%08x,%08lx),stub!\n",  hcur, id);
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
	FIXME_(win32)("(hwnd=%08x, filter=%p,flags=0x%08lx), STUB!\n",
		hnd,notifyfilter,flags
	);
	return 0;
}
