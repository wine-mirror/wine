/*
 * Misc USER functions
 *
 * Copyright 1995 Thomas Sandford
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Turchanov Sergey
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

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "wine/windef16.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winnls.h"
#include "winternl.h"
#include "controls.h"
#include "user_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

#define IMM_INIT_MAGIC 0x19650412
static HWND (WINAPI *imm_get_ui_window)(HKL);
BOOL (WINAPI *imm_register_window)(HWND) = NULL;
void (WINAPI *imm_unregister_window)(HWND) = NULL;

/* MSIME messages */
static UINT WM_MSIME_SERVICE;
static UINT WM_MSIME_RECONVERTOPTIONS;
static UINT WM_MSIME_MOUSE;
static UINT WM_MSIME_RECONVERTREQUEST;
static UINT WM_MSIME_RECONVERT;
static UINT WM_MSIME_QUERYPOSITION;
static UINT WM_MSIME_DOCUMENTFEED;

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
    FIXME("(%04x, %08x, %04x, %04x)\n",
          uCode, dwThreadOrProcessID, dwFlags, hModule );
    /* FIXME: Should chain to GdiSignalProc now. */
    return 0;
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
    TRACE("(0x%08x, 0x%08x)\n", error,type);
    switch(type) {
        case 0:
            break;
        case SLE_ERROR:
        case SLE_MINORERROR:
        case SLE_WARNING:
            /* Fall through for now */
        default:
            FIXME("(error=%08x, type=%08x): Unhandled type\n", error,type);
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
 * SetDebugErrorLevel [USER32.@]
 * Sets the minimum error level for generating debugging events
 *
 * PARAMS
 *    dwLevel [I] Debugging error level
 *
 * RETURNS
 *    Nothing.
 */
VOID WINAPI SetDebugErrorLevel( DWORD dwLevel )
{
    FIXME("(%d): stub\n", dwLevel);
}


/***********************************************************************
 *		SetWindowStationUser (USER32.@)
 */
DWORD WINAPI SetWindowStationUser(DWORD x1,DWORD x2)
{
    FIXME("(0x%08x,0x%08x),stub!\n",x1,x2);
    return 1;
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

static const WCHAR primary_device_name[] = {'\\','\\','.','\\','D','I','S','P','L','A','Y','1',0};
static const WCHAR primary_device_string[] = {'X','1','1',' ','W','i','n','d','o','w','i','n','g',' ',
                                              'S','y','s','t','e','m',0};
static const WCHAR primary_device_deviceid[] = {'P','C','I','\\','V','E','N','_','0','0','0','0','&',
                                                'D','E','V','_','0','0','0','0',0};

/***********************************************************************
 *		EnumDisplayDevicesA (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesA( LPCSTR lpDevice, DWORD i, LPDISPLAY_DEVICEA lpDispDev,
                                 DWORD dwFlags )
{
    UNICODE_STRING deviceW;
    DISPLAY_DEVICEW ddW;
    BOOL ret;

    if(lpDevice)
        RtlCreateUnicodeStringFromAsciiz(&deviceW, lpDevice); 
    else
        deviceW.Buffer = NULL;

    ddW.cb = sizeof(ddW);
    ret = EnumDisplayDevicesW(deviceW.Buffer, i, &ddW, dwFlags);
    RtlFreeUnicodeString(&deviceW);

    if(!ret) return ret;

    WideCharToMultiByte(CP_ACP, 0, ddW.DeviceName, -1, lpDispDev->DeviceName, sizeof(lpDispDev->DeviceName), NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, ddW.DeviceString, -1, lpDispDev->DeviceString, sizeof(lpDispDev->DeviceString), NULL, NULL);
    lpDispDev->StateFlags = ddW.StateFlags;

    if(lpDispDev->cb >= offsetof(DISPLAY_DEVICEA, DeviceID) + sizeof(lpDispDev->DeviceID))
        WideCharToMultiByte(CP_ACP, 0, ddW.DeviceID, -1, lpDispDev->DeviceID, sizeof(lpDispDev->DeviceID), NULL, NULL);
    if(lpDispDev->cb >= offsetof(DISPLAY_DEVICEA, DeviceKey) + sizeof(lpDispDev->DeviceKey))
        WideCharToMultiByte(CP_ACP, 0, ddW.DeviceKey, -1, lpDispDev->DeviceKey, sizeof(lpDispDev->DeviceKey), NULL, NULL);

    return TRUE;
}

/***********************************************************************
 *		EnumDisplayDevicesW (USER32.@)
 */
BOOL WINAPI EnumDisplayDevicesW( LPCWSTR lpDevice, DWORD i, LPDISPLAY_DEVICEW lpDisplayDevice,
                                 DWORD dwFlags )
{
    FIXME("(%s,%d,%p,0x%08x), stub!\n",debugstr_w(lpDevice),i,lpDisplayDevice,dwFlags);

    if (i)
        return FALSE;

    memcpy(lpDisplayDevice->DeviceName, primary_device_name, sizeof(primary_device_name));
    memcpy(lpDisplayDevice->DeviceString, primary_device_string, sizeof(primary_device_string));
  
    lpDisplayDevice->StateFlags =
        DISPLAY_DEVICE_ATTACHED_TO_DESKTOP |
        DISPLAY_DEVICE_PRIMARY_DEVICE |
        DISPLAY_DEVICE_VGA_COMPATIBLE;

    if(lpDisplayDevice->cb >= offsetof(DISPLAY_DEVICEW, DeviceID) + sizeof(lpDisplayDevice->DeviceID))
        memcpy(lpDisplayDevice->DeviceID, primary_device_deviceid, sizeof(primary_device_deviceid));
    if(lpDisplayDevice->cb >= offsetof(DISPLAY_DEVICEW, DeviceKey) + sizeof(lpDisplayDevice->DeviceKey))
        lpDisplayDevice->DeviceKey[0] = 0;

    return TRUE;
}

struct monitor_enum_info
{
    RECT     rect;
    UINT     max_area;
    UINT     min_distance;
    HMONITOR primary;
    HMONITOR nearest;
    HMONITOR ret;
};

/* helper callback for MonitorFromRect */
static BOOL CALLBACK monitor_enum( HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lp )
{
    struct monitor_enum_info *info = (struct monitor_enum_info *)lp;
    RECT intersect;

    if (IntersectRect( &intersect, rect, &info->rect ))
    {
        /* check for larger intersecting area */
        UINT area = (intersect.right - intersect.left) * (intersect.bottom - intersect.top);
        if (area > info->max_area)
        {
            info->max_area = area;
            info->ret = monitor;
        }
    }
    else if (!info->max_area)  /* if not intersecting, check for min distance */
    {
        UINT distance;
        UINT x, y;

        if (info->rect.right <= rect->left) x = rect->left - info->rect.right;
        else if (rect->right <= info->rect.left) x = info->rect.left - rect->right;
        else x = 0;
        if (info->rect.bottom <= rect->top) y = rect->top - info->rect.bottom;
        else if (rect->bottom <= info->rect.top) y = info->rect.top - rect->bottom;
        else y = 0;
        distance = x * x + y * y;
        if (distance < info->min_distance)
        {
            info->min_distance = distance;
            info->nearest = monitor;
        }
    }
    if (!info->primary)
    {
        MONITORINFO mon_info;
        mon_info.cbSize = sizeof(mon_info);
        GetMonitorInfoW( monitor, &mon_info );
        if (mon_info.dwFlags & MONITORINFOF_PRIMARY) info->primary = monitor;
    }
    return TRUE;
}

/***********************************************************************
 *		MonitorFromRect (USER32.@)
 */
HMONITOR WINAPI MonitorFromRect( LPRECT rect, DWORD flags )
{
    struct monitor_enum_info info;

    info.rect         = *rect;
    info.max_area     = 0;
    info.min_distance = ~0u;
    info.primary      = 0;
    info.nearest      = 0;
    info.ret          = 0;

    if (IsRectEmpty(&info.rect))
    {
        info.rect.right = info.rect.left + 1;
        info.rect.bottom = info.rect.top + 1;
    }

    if (!EnumDisplayMonitors( 0, NULL, monitor_enum, (LPARAM)&info )) return 0;
    if (!info.ret)
    {
        if (flags & MONITOR_DEFAULTTOPRIMARY) info.ret = info.primary;
        else if (flags & MONITOR_DEFAULTTONEAREST) info.ret = info.nearest;
    }

    TRACE( "%s flags %x returning %p\n", wine_dbgstr_rect(rect), flags, info.ret );
    return info.ret;
}

/***********************************************************************
 *		MonitorFromPoint (USER32.@)
 */
HMONITOR WINAPI MonitorFromPoint( POINT pt, DWORD flags )
{
    RECT rect;

    SetRect( &rect, pt.x, pt.y, pt.x + 1, pt.y + 1 );
    return MonitorFromRect( &rect, flags );
}

/***********************************************************************
 *		MonitorFromWindow (USER32.@)
 */
HMONITOR WINAPI MonitorFromWindow(HWND hWnd, DWORD dwFlags)
{
    RECT rect;
    WINDOWPLACEMENT wp;

    TRACE("(%p, 0x%08x)\n", hWnd, dwFlags);

    wp.length = sizeof(wp);
    if (IsIconic(hWnd) && GetWindowPlacement(hWnd, &wp))
        return MonitorFromRect( &wp.rcNormalPosition, dwFlags );

    if (GetWindowRect( hWnd, &rect ))
        return MonitorFromRect( &rect, dwFlags );

    if (!(dwFlags & (MONITOR_DEFAULTTOPRIMARY|MONITOR_DEFAULTTONEAREST))) return 0;
    /* retrieve the primary */
    SetRect( &rect, 0, 0, 1, 1 );
    return MonitorFromRect( &rect, dwFlags );
}

/***********************************************************************
 *		GetMonitorInfoA (USER32.@)
 */
BOOL WINAPI GetMonitorInfoA(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    MONITORINFOEXW miW;
    MONITORINFOEXA *miA = (MONITORINFOEXA*)lpMonitorInfo;
    BOOL ret;

    if((miA->cbSize != sizeof(MONITORINFOEXA)) && (miA->cbSize != sizeof(MONITORINFO)))
        return FALSE;

    miW.cbSize = sizeof(miW);

    ret = GetMonitorInfoW(hMonitor, (MONITORINFO*)&miW);
    if(!ret) return ret;

    miA->rcMonitor = miW.rcMonitor;
    miA->rcWork = miW.rcWork;
    miA->dwFlags = miW.dwFlags;
    if(miA->cbSize == sizeof(MONITORINFOEXA))
        WideCharToMultiByte(CP_ACP, 0, miW.szDevice, -1, miA->szDevice, sizeof(miA->szDevice), NULL, NULL);
    return ret;
}

/***********************************************************************
 *		GetMonitorInfoW (USER32.@)
 */
BOOL WINAPI GetMonitorInfoW(HMONITOR hMonitor, LPMONITORINFO lpMonitorInfo)
{
    BOOL ret;

    if (lpMonitorInfo->cbSize != sizeof(MONITORINFOEXW) && lpMonitorInfo->cbSize != sizeof(MONITORINFO))
        return FALSE;

    ret = USER_Driver->pGetMonitorInfo( hMonitor, lpMonitorInfo );
    if (ret)
        TRACE("flags %04x, monitor %s, work %s\n", lpMonitorInfo->dwFlags,
              wine_dbgstr_rect(&lpMonitorInfo->rcMonitor),
              wine_dbgstr_rect(&lpMonitorInfo->rcWork));
    return ret;
}

#ifdef __i386__
/* Some apps pass a non-stdcall callback to EnumDisplayMonitors,
 * so we need a small assembly wrapper to call it.
 * MJ's Help Diagnostic expects that %ecx contains the address to the rect.
 */
struct enumdisplaymonitors_lparam
{
    MONITORENUMPROC proc;
    LPARAM lparam;
};

extern BOOL CALLBACK enumdisplaymonitors_callback_wrapper(HMONITOR monitor, HDC hdc, LPRECT rect, LPARAM lparam);
__ASM_STDCALL_FUNC( enumdisplaymonitors_callback_wrapper, 16,
    "pushl %ebp\n\t"
    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
    __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
    "movl %esp,%ebp\n\t"
    __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
    "subl $8,%esp\n\t"
    "movl 20(%ebp),%eax\n\t"    /* struct enumdisplaymonitors_lparam *orig = (struct enumdisplaymonitors_lparam*)lparam */
    "pushl 4(%eax)\n\t"         /* push orig->lparam */
    "pushl 16(%ebp)\n\t"
    "pushl 12(%ebp)\n\t"
    "pushl 8(%ebp)\n\t"
    "movl 16(%ebp),%ecx\n\t"
    "call *(%eax)\n\t"          /* call orig->proc */
    "leave\n\t"
    __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
    __ASM_CFI(".cfi_same_value %ebp\n\t")
    "ret $16" )
#endif /* __i386__ */

/***********************************************************************
 *		EnumDisplayMonitors (USER32.@)
 */
BOOL WINAPI EnumDisplayMonitors( HDC hdc, LPRECT rect, MONITORENUMPROC proc, LPARAM lp )
{
#ifdef __i386__
    struct enumdisplaymonitors_lparam orig = { proc, lp };
    proc = enumdisplaymonitors_callback_wrapper;
    lp = (LPARAM)&orig;
#endif
    return USER_Driver->pEnumDisplayMonitors( hdc, rect, proc, lp );
}


/***********************************************************************
 *              QueryDisplayConfig (USER32.@)
 */
LONG WINAPI QueryDisplayConfig(UINT32 flags, UINT32 *numpathelements, DISPLAYCONFIG_PATH_INFO *pathinfo,
                               UINT32 *numinfoelements, DISPLAYCONFIG_MODE_INFO *modeinfo,
                               DISPLAYCONFIG_TOPOLOGY_ID *topologyid)
{
   FIXME("(%08x %p %p %p %p %p)\n", flags, numpathelements, pathinfo, numinfoelements, modeinfo, topologyid);
   return ERROR_CALL_NOT_IMPLEMENTED;
}

/***********************************************************************
 *		RegisterSystemThread (USER32.@)
 */
void WINAPI RegisterSystemThread(DWORD flags, DWORD reserved)
{
    FIXME("(%08x, %08x)\n", flags, reserved);
}

/***********************************************************************
 *           RegisterShellHookWindow			[USER32.@]
 */
BOOL WINAPI RegisterShellHookWindow(HWND hWnd)
{
    FIXME("(%p): stub\n", hWnd);
    return FALSE;
}


/***********************************************************************
 *           DeregisterShellHookWindow			[USER32.@]
 */
BOOL WINAPI DeregisterShellHookWindow(HWND hWnd)
{
    FIXME("(%p): stub\n", hWnd);
    return FALSE;
}


/***********************************************************************
 *           RegisterTasklist   			[USER32.@]
 */
DWORD WINAPI RegisterTasklist (DWORD x)
{
    FIXME("0x%08x\n",x);
    return TRUE;
}


/***********************************************************************
 *		RegisterDeviceNotificationA (USER32.@)
 *
 * See RegisterDeviceNotificationW.
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationA(HANDLE hnd, LPVOID notifyfilter, DWORD flags)
{
    FIXME("(hwnd=%p, filter=%p,flags=0x%08x) returns a fake device notification handle!\n",
          hnd,notifyfilter,flags );
    return (HDEVNOTIFY) 0xcafecafe;
}

/***********************************************************************
 *		RegisterDeviceNotificationW (USER32.@)
 *
 * Registers a window with the system so that it will receive
 * notifications about a device.
 *
 * PARAMS
 *     hRecipient           [I] Window or service status handle that
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
 *| DEVICE_NOTIFY_WINDOW_HANDLE  - hRecipient is a window handle
 *| DEVICE_NOTIFY_SERVICE_HANDLE - hRecipient is a service status handle
 */
HDEVNOTIFY WINAPI RegisterDeviceNotificationW(HANDLE hRecipient, LPVOID pNotificationFilter, DWORD dwFlags)
{
    FIXME("(hwnd=%p, filter=%p,flags=0x%08x) returns a fake device notification handle!\n",
          hRecipient,pNotificationFilter,dwFlags );
    return (HDEVNOTIFY) 0xcafeaffe;
}

/***********************************************************************
 *		UnregisterDeviceNotification (USER32.@)
 *
 */
BOOL  WINAPI UnregisterDeviceNotification(HDEVNOTIFY hnd)
{
    FIXME("(handle=%p), STUB!\n", hnd);
    return TRUE;
}

/***********************************************************************
 *           GetAppCompatFlags   (USER32.@)
 */
DWORD WINAPI GetAppCompatFlags( HTASK hTask )
{
    FIXME("(%p) stub\n", hTask);
    return 0;
}

/***********************************************************************
 *           GetAppCompatFlags2   (USER32.@)
 */
DWORD WINAPI GetAppCompatFlags2( HTASK hTask )
{
    FIXME("(%p) stub\n", hTask);
    return 0;
}


/***********************************************************************
 *           AlignRects   (USER32.@)
 */
BOOL WINAPI AlignRects(LPRECT rect, DWORD b, DWORD c, DWORD d)
{
    FIXME("(%p, %d, %d, %d): stub\n", rect, b, c, d);
    if (rect)
        FIXME("rect: %s\n", wine_dbgstr_rect(rect));
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
 *		User32InitializeImmEntryTable
 */
BOOL WINAPI User32InitializeImmEntryTable(DWORD magic)
{
    static const WCHAR imm32_dllW[] = {'i','m','m','3','2','.','d','l','l',0};
    HMODULE imm32 = GetModuleHandleW(imm32_dllW);

    TRACE("(%x)\n", magic);

    if (!imm32 || magic != IMM_INIT_MAGIC)
        return FALSE;

    if (imm_get_ui_window)
        return TRUE;

    WM_MSIME_SERVICE = RegisterWindowMessageA("MSIMEService");
    WM_MSIME_RECONVERTOPTIONS = RegisterWindowMessageA("MSIMEReconvertOptions");
    WM_MSIME_MOUSE = RegisterWindowMessageA("MSIMEMouseOperation");
    WM_MSIME_RECONVERTREQUEST = RegisterWindowMessageA("MSIMEReconvertRequest");
    WM_MSIME_RECONVERT = RegisterWindowMessageA("MSIMEReconvert");
    WM_MSIME_QUERYPOSITION = RegisterWindowMessageA("MSIMEQueryPosition");
    WM_MSIME_DOCUMENTFEED = RegisterWindowMessageA("MSIMEDocumentFeed");

    /* this part is not compatible with native imm32.dll */
    imm_get_ui_window = (void*)GetProcAddress(imm32, "__wine_get_ui_window");
    imm_register_window = (void*)GetProcAddress(imm32, "__wine_register_window");
    imm_unregister_window = (void*)GetProcAddress(imm32, "__wine_unregister_window");
    if (!imm_get_ui_window)
        FIXME("native imm32.dll not supported\n");
    return TRUE;
}

/**********************************************************************
 * WINNLSGetIMEHotkey [USER32.@]
 *
 */
UINT WINAPI WINNLSGetIMEHotkey(HWND hwnd)
{
    FIXME("hwnd %p: stub!\n", hwnd);
    return 0; /* unknown */
}

/**********************************************************************
 * WINNLSEnableIME [USER32.@]
 *
 */
BOOL WINAPI WINNLSEnableIME(HWND hwnd, BOOL enable)
{
    FIXME("hwnd %p enable %d: stub!\n", hwnd, enable);
    return TRUE; /* success (?) */
}

/**********************************************************************
 * WINNLSGetEnableStatus [USER32.@]
 *
 */
BOOL WINAPI WINNLSGetEnableStatus(HWND hwnd)
{
    FIXME("hwnd %p: stub!\n", hwnd);
    return TRUE; /* success (?) */
}

/**********************************************************************
 * SendIMEMessageExA [USER32.@]
 *
 */
LRESULT WINAPI SendIMEMessageExA(HWND hwnd, LPARAM lparam)
{
  FIXME("(%p,%lx): stub\n", hwnd, lparam);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/**********************************************************************
 * SendIMEMessageExW [USER32.@]
 *
 */
LRESULT WINAPI SendIMEMessageExW(HWND hwnd, LPARAM lparam)
{
  FIXME("(%p,%lx): stub\n", hwnd, lparam);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/**********************************************************************
 * DisableProcessWindowsGhosting [USER32.@]
 *
 */
VOID WINAPI DisableProcessWindowsGhosting(VOID)
{
  FIXME(": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return;
}

/**********************************************************************
 * UserHandleGrantAccess [USER32.@]
 *
 */
BOOL WINAPI UserHandleGrantAccess(HANDLE handle, HANDLE job, BOOL grant)
{
    FIXME("(%p,%p,%d): stub\n", handle, job, grant);
    return TRUE;
}

/**********************************************************************
 * RegisterPowerSettingNotification [USER32.@]
 */
HPOWERNOTIFY WINAPI RegisterPowerSettingNotification(HANDLE recipient, const GUID *guid, DWORD flags)
{
    FIXME("(%p,%s,%x): stub\n", recipient, debugstr_guid(guid), flags);
    return (HPOWERNOTIFY)0xdeadbeef;
}

/**********************************************************************
 * UnregisterPowerSettingNotification [USER32.@]
 */
BOOL WINAPI UnregisterPowerSettingNotification(HPOWERNOTIFY handle)
{
    FIXME("(%p): stub\n", handle);
    return TRUE;
}

/*****************************************************************************
 * GetGestureConfig (USER32.@)
 */
BOOL WINAPI GetGestureConfig( HWND hwnd, DWORD reserved, DWORD flags, UINT *count, GESTURECONFIG *config, UINT size )
{
    FIXME("(%p %08x %08x %p %p %u): stub\n", hwnd, reserved, flags, count, config, size);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 * SetGestureConfig [USER32.@]
 */
BOOL WINAPI SetGestureConfig( HWND hwnd, DWORD reserved, UINT id, PGESTURECONFIG config, UINT size )
{
    FIXME("(%p %08x %u %p %u): stub\n", hwnd, reserved, id, config, size);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/**********************************************************************
 * IsTouchWindow [USER32.@]
 */
BOOL WINAPI IsTouchWindow( HWND hwnd, PULONG flags )
{
    FIXME("(%p %p): stub\n", hwnd, flags);
    return FALSE;
}

/**********************************************************************
 * IsWindowRedirectedForPrint [USER32.@]
 */
BOOL WINAPI IsWindowRedirectedForPrint( HWND hwnd )
{
    FIXME("(%p): stub\n", hwnd);
    return FALSE;
}

/**********************************************************************
 * GetDisplayConfigBufferSizes [USER32.@]
 */
LONG WINAPI GetDisplayConfigBufferSizes(UINT32 flags, UINT32 *num_path_info, UINT32 *num_mode_info)
{
    FIXME("(0x%x %p %p): stub\n", flags, num_path_info, num_mode_info);

    if (!num_path_info || !num_mode_info)
        return ERROR_INVALID_PARAMETER;

    *num_path_info = 0;
    *num_mode_info = 0;
    return ERROR_NOT_SUPPORTED;
}

/**********************************************************************
 * RegisterPointerDeviceNotifications [USER32.@]
 */
BOOL WINAPI RegisterPointerDeviceNotifications(HWND hwnd, BOOL notifyrange)
{
    FIXME("(%p %d): stub\n", hwnd, notifyrange);
    return TRUE;
}

/**********************************************************************
 * GetPointerDevices [USER32.@]
 */
BOOL WINAPI GetPointerDevices(UINT32 *device_count, POINTER_DEVICE_INFO *devices)
{
    FIXME("(%p %p): partial stub\n", device_count, devices);

    if (!device_count)
        return FALSE;

    if (devices)
        return FALSE;

    *device_count = 0;
    return TRUE;
}

/**********************************************************************
 * RegisterTouchHitTestingWindow [USER32.@]
 */
BOOL WINAPI RegisterTouchHitTestingWindow(HWND hwnd, ULONG value)
{
    FIXME("(%p %d): stub\n", hwnd, value);
    return TRUE;
}


static const WCHAR imeW[] = {'I','M','E',0};
const struct builtin_class_descr IME_builtin_class =
{
    imeW,               /* name */
    0,                  /* style  */
    WINPROC_IME,        /* proc */
    2*sizeof(LONG_PTR), /* extra */
    IDC_ARROW,          /* cursor */
    0                   /* brush */
};

static BOOL is_ime_ui_msg( UINT msg )
{
    switch(msg) {
    case WM_IME_STARTCOMPOSITION:
    case WM_IME_ENDCOMPOSITION:
    case WM_IME_COMPOSITION:
    case WM_IME_SETCONTEXT:
    case WM_IME_NOTIFY:
    case WM_IME_CONTROL:
    case WM_IME_COMPOSITIONFULL:
    case WM_IME_SELECT:
    case WM_IME_CHAR:
    case WM_IME_REQUEST:
    case WM_IME_KEYDOWN:
    case WM_IME_KEYUP:
        return TRUE;
    default:
        if ((msg == WM_MSIME_RECONVERTOPTIONS) ||
                (msg == WM_MSIME_SERVICE) ||
                (msg == WM_MSIME_MOUSE) ||
                (msg == WM_MSIME_RECONVERTREQUEST) ||
                (msg == WM_MSIME_RECONVERT) ||
                (msg == WM_MSIME_QUERYPOSITION) ||
                (msg == WM_MSIME_DOCUMENTFEED))
            return TRUE;

        return FALSE;
    }
}

LRESULT WINAPI ImeWndProcA( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HWND uiwnd;

    if (msg==WM_CREATE)
        return TRUE;

    if (imm_get_ui_window && is_ime_ui_msg(msg))
    {
        if ((uiwnd = imm_get_ui_window(GetKeyboardLayout(0))))
            return SendMessageA(uiwnd, msg, wParam, lParam);
        return FALSE;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT WINAPI ImeWndProcW( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    HWND uiwnd;

    if (msg==WM_CREATE)
        return TRUE;

    if (imm_get_ui_window && is_ime_ui_msg(msg))
    {
        if ((uiwnd = imm_get_ui_window(GetKeyboardLayout(0))))
            return SendMessageW(uiwnd, msg, wParam, lParam);
        return FALSE;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}
