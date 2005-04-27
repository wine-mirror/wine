/*
 * Misc USER functions
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1997 Marcus Meissner
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

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

/* callback to allow EnumDesktopsA to use EnumDesktopsW */
typedef struct {
    DESKTOPENUMPROCA lpEnumFunc;
    LPARAM lParam;
} ENUMDESKTOPS_LPARAM;

/* EnumDesktopsA passes this callback function to EnumDesktopsW.
 * It simply converts the string to ASCII and calls the callback
 * function provided by the original caller
 */
static BOOL CALLBACK EnumDesktopProcWtoA(LPWSTR lpszDesktop, LPARAM lParam)
{
    LPSTR buffer;
    INT   len;
    BOOL  ret;
    ENUMDESKTOPS_LPARAM *data = (ENUMDESKTOPS_LPARAM *)lParam;

    len = WideCharToMultiByte(CP_ACP, 0, lpszDesktop, -1, NULL, 0, NULL, NULL);
    if (!(buffer = HeapAlloc( GetProcessHeap(), 0, len))) return FALSE;
    WideCharToMultiByte(CP_ACP, 0, lpszDesktop, -1, buffer, len, NULL, NULL);

    ret = data->lpEnumFunc(buffer, data->lParam);

    HeapFree(GetProcessHeap(), 0, buffer);
    return ret;
}

/**********************************************************************
 * SetLastErrorEx [USER32.@]
 *
 * Sets the last-error code.
 *
 * RETURNS
 *    None.
 */
void WINAPI SetLastErrorEx(
    DWORD error, /* [in] Per-thread error code */
    DWORD type)  /* [in] Error type */
{
    TRACE("(0x%08lx, 0x%08lx)\n", error,type);
    switch(type) {
        case 0:
            break;
        case SLE_ERROR:
        case SLE_MINORERROR:
        case SLE_WARNING:
            /* Fall through for now */
        default:
            FIXME("(error=%08lx, type=%08lx): Unhandled type\n", error,type);
            break;
    }
    SetLastError( error );
}

/******************************************************************************
 * GetAltTabInfoA [USER32.@]
 */
BOOL WINAPI GetAltTabInfoA(HWND hwnd, int iItem, PALTTABINFO pati, LPSTR pszItemText, UINT cchItemText)
{
    FIXME("(%p, 0x%08x, %p, %p, 0x%08x)\n", hwnd, iItem, pati, pszItemText, cchItemText);
    return FALSE;
}

/******************************************************************************
 * GetAltTabInfoW [USER32.@]
 */
BOOL WINAPI GetAltTabInfoW(HWND hwnd, int iItem, PALTTABINFO pati, LPWSTR pszItemText, UINT cchItemText)
{
    FIXME("(%p, 0x%08x, %p, %p, 0x%08x)\n", hwnd, iItem, pati, pszItemText, cchItemText);
    return FALSE;
}

/******************************************************************************
 * GetProcessWindowStation [USER32.@]
 *
 * Returns handle of window station
 *
 * NOTES
 *    Docs say the return value is HWINSTA
 *
 * RETURNS
 *    Success: Handle to window station associated with calling process
 *    Failure: NULL
 */
HWINSTA WINAPI GetProcessWindowStation(void)
{
    FIXME("(void): stub\n");
    return (HWINSTA)1;
}


/******************************************************************************
 * GetThreadDesktop [USER32.@]
 *
 * Returns handle to desktop
 *
 * PARAMS
 *    dwThreadId [I] Thread identifier
 *
 * RETURNS
 *    Success: Handle to desktop associated with specified thread
 *    Failure: NULL
 */
HDESK WINAPI GetThreadDesktop( DWORD dwThreadId )
{
    FIXME("(%lx): stub\n",dwThreadId);
    return (HDESK)1;
}


/******************************************************************************
 * SetDebugErrorLevel [USER32.@]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 */
VOID WINAPI SetDebugErrorLevel( DWORD dwLevel )
{
    FIXME("(%ld): stub\n", dwLevel);
}


/******************************************************************************
 *                    GetProcessDefaultLayout [USER32.@]
 *
 * Gets the default layout for parentless windows.
 * Right now, just returns 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 *
 * BUGS
 *    No RTL
 */
BOOL WINAPI GetProcessDefaultLayout( DWORD *pdwDefaultLayout )
{
    if ( !pdwDefaultLayout ) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
     }
    FIXME( "( %p ): No BiDi\n", pdwDefaultLayout );
    *pdwDefaultLayout = 0;
    return TRUE;
}


/******************************************************************************
 *                    SetProcessDefaultLayout [USER32.@]
 *
 * Sets the default layout for parentless windows.
 * Right now, only accepts 0 (left-to-right).
 *
 * RETURNS
 *    Success: Nonzero
 *    Failure: Zero
 *
 * BUGS
 *    No RTL
 */
BOOL WINAPI SetProcessDefaultLayout( DWORD dwDefaultLayout )
{
    if ( dwDefaultLayout == 0 )
        return TRUE;
    FIXME( "( %08lx ): No BiDi\n", dwDefaultLayout );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/***********************************************************************
 *		CreateDesktopA (USER32.@)
 */
HDESK WINAPI CreateDesktopA( LPSTR lpszDesktop,LPSTR lpszDevice,LPDEVMODEA pDevmode,
                             DWORD dwFlags,DWORD dwDesiredAccess,LPSECURITY_ATTRIBUTES lpsa )
{
    FIXME("(%s,%s,%p,0x%08lx,0x%08lx,%p),stub!\n", lpszDesktop,lpszDevice,pDevmode,
          dwFlags,dwDesiredAccess,lpsa );
    return (HDESK)0xcafedead;
}

/***********************************************************************
 *		CreateDesktopW (USER32.@)
 */
HDESK WINAPI CreateDesktopW( LPWSTR lpszDesktop,LPWSTR lpszDevice,LPDEVMODEW pDevmode,
                             DWORD dwFlags,DWORD dwDesiredAccess,LPSECURITY_ATTRIBUTES lpsa)
{
    FIXME("(%s,%s,%p,0x%08lx,0x%08lx,%p),stub!\n",
          debugstr_w(lpszDesktop),debugstr_w(lpszDevice),pDevmode,
          dwFlags,dwDesiredAccess,lpsa );
    return (HDESK)0xcafedead;
}

/******************************************************************************
 * OpenDesktopA [USER32.@]
 *
 *    Not supported on Win9x - returns NULL and calls SetLastError.
 */
HDESK WINAPI OpenDesktopA( LPCSTR lpszDesktop, DWORD dwFlags,
                                BOOL fInherit, DWORD dwDesiredAccess )
{
    FIXME("(%s,%lx,%i,%lx): stub\n",debugstr_a(lpszDesktop),dwFlags,
          fInherit,dwDesiredAccess);

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/******************************************************************************
 * OpenInputDesktop [USER32.@]
 *
 *    Not supported on Win9x - returns NULL and calls SetLastError.
 */
HDESK WINAPI OpenInputDesktop( DWORD dwFlags, BOOL fInherit, ACCESS_MASK dwDesiredAccess )
{
    FIXME("(%lx,%i,%lx): stub\n",dwFlags, fInherit,dwDesiredAccess);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/***********************************************************************
 *		CloseDesktop (USER32.@)
 */
BOOL WINAPI CloseDesktop(HDESK hDesk)
{
    FIXME("(%p)\n", hDesk);
    return TRUE;
}

/******************************************************************************
 *              EnumDesktopsA [USER32.@]
 */
BOOL WINAPI EnumDesktopsA( HWINSTA hwinsta, DESKTOPENUMPROCA lpEnumFunc,
                    LPARAM lParam )
{
    ENUMDESKTOPS_LPARAM caller_data;

    caller_data.lpEnumFunc = lpEnumFunc;
    caller_data.lParam     = lParam;
    
    return EnumDesktopsW(hwinsta, EnumDesktopProcWtoA, (LPARAM) &caller_data);
}

/******************************************************************************
 *              EnumDesktopsW [USER32.@]
 */
BOOL WINAPI EnumDesktopsW( HWINSTA hwinsta, DESKTOPENUMPROCW lpEnumFunc,
                    LPARAM lParam )
{
    FIXME("%p,%p,%lx): stub\n",hwinsta,lpEnumFunc,lParam);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		EnumDesktopWindows (USER32.@)
 */
BOOL WINAPI EnumDesktopWindows( HDESK hDesktop, WNDENUMPROC lpfn, LPARAM lParam )
{
  FIXME("(%p, %p, 0x%08lx), stub!\n", hDesktop, lpfn, lParam );
  return TRUE;
}

/***********************************************************************
 *		CreateWindowStationW (USER32.@)
 */
HWINSTA WINAPI CreateWindowStationW( LPWSTR winstation,DWORD res1,DWORD desiredaccess,
                                     LPSECURITY_ATTRIBUTES lpsa )
{
    FIXME("(%s,0x%08lx,0x%08lx,%p),stub!\n",debugstr_w(winstation), res1,desiredaccess,lpsa );
    return (HWINSTA)0xdeadcafe;
}

/***********************************************************************
 *		CloseWindowStation (USER32.@)
 */
BOOL WINAPI CloseWindowStation(HWINSTA hWinSta)
{
    FIXME("(%p)\n", hWinSta);
    return TRUE;
}

/***********************************************************************
 *		SetWindowStationUser (USER32.@)
 */
DWORD WINAPI SetWindowStationUser(DWORD x1,DWORD x2)
{
    FIXME("(0x%08lx,0x%08lx),stub!\n",x1,x2);
    return 1;
}

/***********************************************************************
 *		SetProcessWindowStation (USER32.@)
 */
BOOL WINAPI SetProcessWindowStation(HWINSTA hWinSta)
{
    FIXME("(%p),stub!\n",hWinSta);
    return TRUE;
}

/******************************************************************************
 *              EnumWindowStationsA [USER32.@]
 */
BOOL WINAPI EnumWindowStationsA( WINSTAENUMPROCA lpEnumFunc, LPARAM lParam)
{
    FIXME("%p,%lx): stub\n",lpEnumFunc,lParam);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/******************************************************************************
 *              EnumWindowStationsW [USER32.@]
 */
BOOL WINAPI EnumWindowStationsW( WINSTAENUMPROCW lpEnumFunc, LPARAM lParam)
{
    FIXME("%p,%lx): stub\n",lpEnumFunc,lParam);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *		GetUserObjectInformationA (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationA( HANDLE hObj, INT nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{
    FIXME("(%p %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
    return TRUE;
}

/***********************************************************************
 *		GetUserObjectInformationW (USER32.@)
 */
BOOL WINAPI GetUserObjectInformationW( HANDLE hObj, INT nIndex, LPVOID pvInfo, DWORD nLength, LPDWORD lpnLen )
{
    FIXME("(%p %i %p %ld %p),stub!\n", hObj, nIndex, pvInfo, nLength, lpnLen );
    return TRUE;
}

/******************************************************************************
 *		SetUserObjectInformationA   (USER32.@)
 */
BOOL WINAPI SetUserObjectInformationA( HANDLE hObj, INT nIndex,
				       LPVOID pvInfo, DWORD nLength )
{
    FIXME("(%p,%d,%p,%lx): stub\n",hObj,nIndex,pvInfo,nLength);
    return TRUE;
}

/***********************************************************************
 *		GetUserObjectSecurity (USER32.@)
 */
BOOL WINAPI GetUserObjectSecurity(HANDLE hObj, PSECURITY_INFORMATION pSIRequested,
                                  PSECURITY_DESCRIPTOR pSID, DWORD nLength, LPDWORD lpnLengthNeeded)
{
    FIXME("(%p %p %p len=%ld %p),stub!\n",  hObj, pSIRequested, pSID, nLength, lpnLengthNeeded);
    return TRUE;
}

/***********************************************************************
 *		SetUserObjectSecurity (USER32.@)
 */
BOOL WINAPI SetUserObjectSecurity( HANDLE hObj, PSECURITY_INFORMATION pSIRequested,
                                   PSECURITY_DESCRIPTOR pSID )
{
    FIXME("(%p,%p,%p),stub!\n",hObj,pSIRequested,pSID);
    return TRUE;
}

/******************************************************************************
 *		SetThreadDesktop   (USER32.@)
 */
BOOL WINAPI SetThreadDesktop( HANDLE hDesktop )
{
    FIXME("(%p): stub\n",hDesktop);
    return TRUE;
}

/***********************************************************************
 *		RegisterLogonProcess (USER32.@)
 */
DWORD WINAPI RegisterLogonProcess(HANDLE hprocess,BOOL x)
{
    FIXME("(%p,%d),stub!\n",hprocess,x);
    return 1;
}

/***********************************************************************
 *		SetLogonNotifyWindow (USER32.@)
 */
DWORD WINAPI SetLogonNotifyWindow(HWINSTA hwinsta,HWND hwnd)
{
    FIXME("(%p,%p),stub!\n",hwinsta,hwnd);
    return 1;
}

/***********************************************************************
 *		RegisterSystemThread (USER32.@)
 */
void WINAPI RegisterSystemThread(DWORD flags, DWORD reserved)
{
    FIXME("(%08lx, %08lx)\n", flags, reserved);
}

/***********************************************************************
 *           RegisterShellHookWindow			[USER32.@]
 */
BOOL WINAPI RegisterShellHookWindow ( HWND hWnd )
{
    FIXME("(%p): stub\n", hWnd);
    return 0;
}


/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.@]
 */
HRESULT WINAPI DeregisterShellHookWindow ( DWORD u )
{
    FIXME("0x%08lx stub\n",u);
    return 0;

}


/***********************************************************************
 *           RegisterTasklist   			[USER32.@]
 */
DWORD WINAPI RegisterTasklist (DWORD x)
{
    FIXME("0x%08lx\n",x);
    return TRUE;
}


/***********************************************************************
 *		RegisterDeviceNotificationA (USER32.@)
 *
 * See RegisterDeviceNotificationW.
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationA(HANDLE hnd, LPVOID notifyfilter, DWORD flags)
{
    FIXME("(hwnd=%p, filter=%p,flags=0x%08lx), STUB!\n", hnd,notifyfilter,flags );
    return 0;
}

/***********************************************************************
 *		RegisterDeviceNotificationW (USER32.@)
 *
 * Registers a window with the system so that it will receive
 * notifications about a device.
 *
 * PARAMS
 *     hRecepient           [I] Window or service status handle that
 *                              will receive notifications.
 *     pNotificationFilter  [I] DEV_BROADCAST_HDR followed by some
 *                              type-specific data.
 *     dwFlags              [I] See notes
 *
 * RETURNS
 *
 * A handle to the device notification.
 *
 * NOTES
 *
 * The dwFlags parameter can be one of two values:
 *| DEVICE_NOTIFY_WINDOW_HANDLE  - hRecepient is a window handle
 *| DEVICE_NOTIFY_SERVICE_HANDLE - hRecepient is a service status handle
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationW(HANDLE hRecepient, LPVOID pNotificationFilter, DWORD dwFlags)
{
    FIXME("(hwnd=%p, filter=%p,flags=0x%08lx), STUB!\n", hRecepient,pNotificationFilter,dwFlags );
    return 0;
}

/***********************************************************************
 *           GetAppCompatFlags   (USER32.@)
 */
DWORD WINAPI GetAppCompatFlags( HTASK hTask )
{
    FIXME("stub\n");
    return 0;
}


/***********************************************************************
 *           AlignRects   (USER32.@)
 */
BOOL WINAPI AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d)
{
    FIXME("(%p, %ld, %ld, %ld): stub\n", rect, b, c, d);
    if (rect)
        FIXME("rect: [[%ld, %ld], [%ld, %ld]]\n", rect->left, rect->top, rect->right, rect->bottom);
    /* Calls OffsetRect */
    return FALSE;
}


/***********************************************************************
 *		LoadLocalFonts (USER32.@)
 */
VOID WINAPI LoadLocalFonts(VOID)
{
    /* are loaded. */
    return;
}


/***********************************************************************
 *		USER_489 (USER.489)
 */
LONG WINAPI stub_USER_489(void) { FIXME("stub\n"); return 0; }

/***********************************************************************
 *		USER_490 (USER.490)
 */
LONG WINAPI stub_USER_490(void) { FIXME("stub\n"); return 0; }

/***********************************************************************
 *		USER_492 (USER.492)
 */
LONG WINAPI stub_USER_492(void) { FIXME("stub\n"); return 0; }

/***********************************************************************
 *		USER_496 (USER.496)
 */
LONG WINAPI stub_USER_496(void) { FIXME("stub\n"); return 0; }

/***********************************************************************
 *		User32InitializeImmEntryTable
 */
BOOL WINAPI User32InitializeImmEntryTable(LPVOID ptr)
{
  FIXME("(%p): stub\n", ptr);
  return TRUE;
}

/**********************************************************************
 * WINNLSGetIMEHotkey [USER32.@]
 *
 */
UINT WINAPI WINNLSGetIMEHotkey(HWND hUnknown1)
{
    FIXME("hUnknown1 %p: stub!\n", hUnknown1);
    return 0; /* unknown */
}

/**********************************************************************
 * WINNLSEnableIME [USER32.@]
 *
 */
BOOL WINAPI WINNLSEnableIME(HWND hUnknown1, BOOL bUnknown2)
{
    FIXME("hUnknown1 %p bUnknown2 %d: stub!\n", hUnknown1, bUnknown2);
    return TRUE; /* success (?) */
}

/**********************************************************************
 * WINNLSGetEnableStatus [USER32.@]
 *
 */
BOOL WINAPI WINNLSGetEnableStatus(HWND hUnknown1)
{
    FIXME("hUnknown1 %p: stub!\n", hUnknown1);
    return TRUE; /* success (?) */
}

/**********************************************************************
 * SendIMEMessageExA [USER32.@]
 *
 */
LRESULT WINAPI SendIMEMessageExA(HWND p1, LPARAM p2)
{
  FIXME("(%p,%lx): stub\n", p1, p2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/**********************************************************************
 * SendIMEMessageExW [USER32.@]
 *
 */
LRESULT WINAPI SendIMEMessageExW(HWND p1, LPARAM p2)
{
  FIXME("(%p,%lx): stub\n", p1, p2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}
