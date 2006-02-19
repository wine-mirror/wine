/*
 * Systray handling
 *
 * Copyright 1999 Kai Morich	<kai.morich@bigfoot.de>
 * Copyright 2004 Mike Hearn, for CodeWeavers
 * Copyright 2005 Robert Shearman
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "shellapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

const static WCHAR classname[] = /* Shell_TrayWnd */ {'S','h','e','l','l','_','T','r','a','y','W','n','d','\0'};

/* start timeout of 1 second */
#define SYSTRAY_START_TIMEOUT 1000

static BOOL start_systray_process(void)
{
    STARTUPINFOW          sinfo;
    PROCESS_INFORMATION   pinfo;
    WCHAR                 command_line[] = {'e','x','p','l','o','r','e','r',' ','/','s','y','s','t','r','a','y',0};
    static const WCHAR    event_name[] = {'W','i','n','e','S','y','s','t','r','a','y','I','n','i','t','e','d',0};
    HANDLE                systray_ready_event;
    DWORD                 wait;

    TRACE("No tray window found, starting %s\n", debugstr_w(command_line));

    ZeroMemory(&sinfo, sizeof(sinfo));
    sinfo.cb = sizeof(sinfo);

    if (CreateProcessW(NULL, command_line, NULL, NULL, FALSE, 0, NULL, NULL, &sinfo, &pinfo) == 0)
    {
        ERR("Could not start %s, error 0x%lx\n", debugstr_w(command_line), GetLastError());
        return FALSE;
    }
 
    CloseHandle(pinfo.hThread);
    CloseHandle(pinfo.hProcess);

    systray_ready_event = CreateEventW(NULL, TRUE, FALSE, event_name);
    if (!systray_ready_event) return FALSE;

    /* don't guess how long to wait, just wait for process to signal to us
     * that it has created the Shell_TrayWnd class before continuing */
    wait = WaitForSingleObject(systray_ready_event, SYSTRAY_START_TIMEOUT);
    CloseHandle(systray_ready_event);

    if (wait == WAIT_TIMEOUT)
    {
        ERR("timeout waiting for %s to start\n", debugstr_w(command_line));
        return FALSE;
    }

    return TRUE;
}

/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 * Shell_NotifyIconA			[SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
    NOTIFYICONDATAW nidW;
    
    nidW.cbSize = sizeof(nidW);
    nidW.hWnd   = pnid->hWnd;
    nidW.uID    = pnid->uID;
    nidW.uFlags = pnid->uFlags;
    nidW.uCallbackMessage = pnid->uCallbackMessage;
    nidW.hIcon  = pnid->hIcon;

    /* szTip */
    MultiByteToWideChar(CP_ACP, 0, pnid->szTip, sizeof(pnid->szTip), nidW.szTip, sizeof(nidW.szTip));

    nidW.dwState      = pnid->dwState;
    nidW.dwStateMask  = pnid->dwStateMask;

    /* szInfo */
    MultiByteToWideChar(CP_ACP, 0, pnid->szInfo, sizeof(pnid->szInfo),  nidW.szInfo, sizeof(nidW.szInfo));

    nidW.u.uTimeout = pnid->u.uTimeout;

    /* szInfoTitle */
    MultiByteToWideChar(CP_ACP, 0, pnid->szInfoTitle, sizeof(pnid->szInfoTitle), nidW.szInfoTitle, sizeof(nidW.szInfoTitle));
    
    nidW.dwInfoFlags = pnid->dwInfoFlags;

    return Shell_NotifyIconW(dwMessage, &nidW);
}

/*************************************************************************
 * Shell_NotifyIconW			[SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW nid)
{
    HWND tray;
    COPYDATASTRUCT cds;

    TRACE("dwMessage = %ld\n", dwMessage);

    tray = FindWindowExW(0, NULL, classname, NULL);

    /* this isn't how native does it - it assumes that Explorer is always
     * running */
    if (!tray)
    {
        if (!start_systray_process())
            return FALSE;
        tray = FindWindowExW(0, NULL, classname, NULL);
    }

    if (!tray) return FALSE;

    cds.dwData = dwMessage;

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if (nid->uFlags & NIF_ICON)
    {
        ICONINFO iconinfo;
        char *buffer;
        BITMAP bmMask;
        BITMAP bmColour;
        LONG cbMaskBits;
        LONG cbColourBits;

        if (!GetIconInfo(nid->hIcon, &iconinfo))
            return FALSE;

        if (!GetObjectW(iconinfo.hbmMask, sizeof(bmMask), &bmMask) ||
            !GetObjectW(iconinfo.hbmColor, sizeof(bmColour), &bmColour))
        {
            DeleteObject(iconinfo.hbmMask);
            DeleteObject(iconinfo.hbmColor);
            return FALSE;
        }

        cbMaskBits = (bmMask.bmPlanes * bmMask.bmWidth * bmMask.bmHeight * bmMask.bmBitsPixel) / 8;
        cbColourBits = (bmColour.bmPlanes * bmColour.bmWidth * bmColour.bmHeight * bmColour.bmBitsPixel) / 8;
        cds.cbData = sizeof(*nid) + 2*sizeof(BITMAP) + cbMaskBits + cbColourBits;
        buffer = HeapAlloc(GetProcessHeap(), 0, cds.cbData);
        if (!buffer) return FALSE;
        cds.lpData = buffer;

        memcpy(buffer, nid, sizeof(*nid));
        buffer += sizeof(*nid);
        memcpy(buffer, &bmMask, sizeof(bmMask));
        buffer += sizeof(bmMask);
        memcpy(buffer, &bmColour, sizeof(bmColour));
        buffer += sizeof(bmColour);
        GetBitmapBits(iconinfo.hbmMask, cbMaskBits, buffer);
        buffer += cbMaskBits;
        GetBitmapBits(iconinfo.hbmColor, cbColourBits, buffer);
        buffer += cbColourBits;

        DeleteObject(iconinfo.hbmMask);
        DeleteObject(iconinfo.hbmColor);
    }
    else
    {
        cds.cbData = sizeof(*nid);
        cds.lpData = nid;
    }

    SendMessageW(tray, WM_COPYDATA, (WPARAM)nid->hWnd, (LPARAM)&cds);

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if (nid->uFlags & NIF_ICON)
        HeapFree(GetProcessHeap(), 0, cds.lpData);

    return TRUE;
}
