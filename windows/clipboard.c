/*
 * WIN32 clipboard implementation
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
 *	     2003 Ulrich Czekalla for CodeWeavers
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
 *
 * NOTES:
 *    This file contains the implementation for the WIN32 Clipboard API
 * and Wine's internal clipboard cache.
 * The actual contents of the clipboard are held in the clipboard cache.
 * The internal implementation talks to a "clipboard driver" to fill or
 * expose the cache to the native device. (Currently only the X11 and
 * TTY clipboard  driver are available)
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "user.h"
#include "win.h"
#include "clipboard.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

#define  CF_REGFORMATBASE 	0xC000


/*
 * Indicates if data has changed since open.
 */
static BOOL bCBHasChanged = FALSE;


/**************************************************************************
 *                      CLIPBOARD_SetClipboardOwner
 */
BOOL CLIPBOARD_SetClipboardOwner(HWND hWnd)
{
    BOOL bRet = FALSE;

    TRACE(" hWnd(%p)\n", hWnd);

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = SET_CB_OWNER;
        req->owner = WIN_GetFullHandle( hWnd );

        if (wine_server_call_err( req ))
        {
            ERR("Failed to set clipboard.\n");
        }
        else
        {
            bRet = TRUE;
        }
    }
    SERVER_END_REQ;

    return bRet;
}


/**************************************************************************
 *                      CLIPBOARD_GetClipboardInfo
 */
BOOL CLIPBOARD_GetClipboardInfo(LPCLIPBOARDINFO cbInfo)
{
    BOOL bRet = FALSE;

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = 0;

        if (wine_server_call_err( req ))
        {
            ERR("Failed to get clipboard owner.\n");
        }
        else
        {
            cbInfo->hWndOpen = reply->old_clipboard;
            cbInfo->hWndOwner = reply->old_owner;
            cbInfo->hWndViewer = reply->old_viewer;
            cbInfo->seqno = reply->seqno;
            cbInfo->flags = reply->flags;

            bRet = TRUE;
        }
    }
    SERVER_END_REQ;

    return bRet;
}


/**************************************************************************
 *	CLIPBOARD_ReleaseOwner
 */
BOOL CLIPBOARD_ReleaseOwner(void)
{
    BOOL bRet = FALSE;

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = SET_CB_RELOWNER | SET_CB_SEQNO;

        if (wine_server_call_err( req ))
        {
            ERR("Failed to set clipboard.\n");
        }
        else
        {
            bRet = TRUE;
        }
    }
    SERVER_END_REQ;

    return bRet;
}


/**************************************************************************
 *		CLIPBOARD_OpenClipboard
 */
static BOOL CLIPBOARD_OpenClipboard(HWND hWnd)
{
    BOOL bRet = FALSE;

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = SET_CB_OPEN;
        req->clipboard = WIN_GetFullHandle( hWnd );

        if (wine_server_call_err( req ))
        {
            ERR("Failed to set clipboard.\n");
        }
        else
        {
            bRet = TRUE;
        }
    }
    SERVER_END_REQ;

    return bRet;
}


/**************************************************************************
 *		CLIPBOARD_CloseClipboard
 */
static BOOL CLIPBOARD_CloseClipboard(void)
{
    BOOL bRet = FALSE;

    TRACE(" Changed=%d\n", bCBHasChanged);

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = SET_CB_CLOSE;

        if (bCBHasChanged)
        {
            req->flags |= SET_CB_SEQNO;
            TRACE("Clipboard data changed\n");
        }

        if (wine_server_call_err( req ))
        {
            ERR("Failed to set clipboard.\n");
        }
        else
        {
            bRet = TRUE;
        }
    }
    SERVER_END_REQ;

    return bRet;
}


/**************************************************************************
 *                WIN32 Clipboard implementation
 **************************************************************************/

/**************************************************************************
 *		RegisterClipboardFormatA (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatA(LPCSTR FormatName)
{
    UINT wFormatID = 0;

    TRACE("%s\n", debugstr_a(FormatName));

    if (USER_Driver.pRegisterClipboardFormat)
        wFormatID = USER_Driver.pRegisterClipboardFormat(FormatName);

    return wFormatID;
}


/**************************************************************************
 *		RegisterClipboardFormat (USER.145)
 */
UINT16 WINAPI RegisterClipboardFormat16(LPCSTR FormatName)
{
    UINT wFormatID = 0;

    TRACE("%s\n", debugstr_a(FormatName));

    if (USER_Driver.pRegisterClipboardFormat)
        wFormatID = USER_Driver.pRegisterClipboardFormat(FormatName);

    return wFormatID;
}


/**************************************************************************
 *		RegisterClipboardFormatW (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatW(LPCWSTR formatName)
{
    LPSTR aFormat = HEAP_strdupWtoA( GetProcessHeap(), 0, formatName );
    UINT ret = RegisterClipboardFormatA( aFormat );
    HeapFree( GetProcessHeap(), 0, aFormat );
    return ret;
}


/**************************************************************************
 *		GetClipboardFormatName (USER.146)
 */
INT16 WINAPI GetClipboardFormatName16(UINT16 wFormat, LPSTR retStr, INT16 maxlen)
{
    TRACE("%04x,%p,%d\n", wFormat, retStr, maxlen);

    return GetClipboardFormatNameA(wFormat, retStr, maxlen);
}


/**************************************************************************
 *		GetClipboardFormatNameA (USER32.@)
 */
INT WINAPI GetClipboardFormatNameA(UINT wFormat, LPSTR retStr, INT maxlen)
{
    INT len = 0;

    TRACE("%04x,%p,%d\n", wFormat, retStr, maxlen);

    if (USER_Driver.pGetClipboardFormatName)
        len = USER_Driver.pGetClipboardFormatName(wFormat, retStr, maxlen);

    return len;
}


/**************************************************************************
 *		GetClipboardFormatNameW (USER32.@)
 */
INT WINAPI GetClipboardFormatNameW(UINT wFormat, LPWSTR retStr, INT maxlen)
{
    INT ret;
    LPSTR p = HeapAlloc( GetProcessHeap(), 0, maxlen );
    if(p == NULL) return 0; /* FIXME: is this the correct failure value? */

    ret = GetClipboardFormatNameA( wFormat, p, maxlen );

    if (maxlen > 0 && !MultiByteToWideChar( CP_ACP, 0, p, -1, retStr, maxlen ))
        retStr[maxlen-1] = 0;
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/**************************************************************************
 *		OpenClipboard (USER32.@)
 *
 * Note: Netscape uses NULL hWnd to open the clipboard.
 */
BOOL WINAPI OpenClipboard( HWND hWnd )
{
    BOOL bRet;

    TRACE("(%p)...\n", hWnd);

    bRet = CLIPBOARD_OpenClipboard(hWnd);

    TRACE(" returning %i\n", bRet);

    return bRet;
}


/**************************************************************************
 *		CloseClipboard (USER.138)
 */
BOOL16 WINAPI CloseClipboard16(void)
{
    return CloseClipboard();
}


/**************************************************************************
 *		CloseClipboard (USER32.@)
 */
BOOL WINAPI CloseClipboard(void)
{
    BOOL bRet = FALSE;

    TRACE("(%d)\n", bCBHasChanged);

    if (CLIPBOARD_CloseClipboard())
    {
 	if (bCBHasChanged)
 	{
            HWND hWndViewer = GetClipboardViewer();

            if (USER_Driver.pEndClipboardUpdate)
                USER_Driver.pEndClipboardUpdate();
 
 	    if (hWndViewer)
 	        SendMessageW(hWndViewer, WM_DRAWCLIPBOARD, 0, 0);
 
            bCBHasChanged = FALSE;
        }

	bRet = TRUE;
    }

    return bRet;
}


/**************************************************************************
 *		EmptyClipboard (USER.139)
 */
BOOL16 WINAPI EmptyClipboard16(void)
{
    return EmptyClipboard();
}


/**************************************************************************
 *		EmptyClipboard (USER32.@)
 * Empties and acquires ownership of the clipboard
 */
BOOL WINAPI EmptyClipboard(void)
{
    CLIPBOARDINFO cbinfo;
 
    TRACE("()\n");

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo) ||
        ~cbinfo.flags & CB_OPEN)
    { 
        WARN("Clipboard not opened by calling task!\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return FALSE;
    }

    /* Destroy private objects */
    if (cbinfo.hWndOwner) 
        SendMessageW(cbinfo.hWndOwner, WM_DESTROYCLIPBOARD, 0, 0);

    /* Tell the driver to acquire the selection. The current owner
     * will be signaled to delete it's own cache. */
    if (~cbinfo.flags & CB_OWNER)
    {
        /* Assign ownership of the clipboard to the current client. We do
	 * this before acquiring the selection so that when we do acquire the
	 * selection and the selection loser gets notified, it can check if 
	 * it has lost the Wine clipboard ownership. If it did then it knows
	 * that a WM_DESTORYCLIPBOARD has already been sent. Otherwise it
	 * lost the selection to a X app and it should send the 
	 * WM_DESTROYCLIPBOARD itself. */
        CLIPBOARD_SetClipboardOwner(cbinfo.hWndOpen);

	/* Acquire the selection. This will notify the previous owner
	 * to clear it's cache. */ 
        if (USER_Driver.pAcquireClipboard) 
            USER_Driver.pAcquireClipboard(cbinfo.hWndOpen);
    }

    /* Empty the local cache */
    if (USER_Driver.pEmptyClipboard) 
        USER_Driver.pEmptyClipboard();
 
    bCBHasChanged = TRUE;

    return TRUE;
}


/**************************************************************************
 *		GetClipboardOwner (USER32.@)
 *  FIXME: Can't return the owner if the clipboard is owned by an external X-app
 */
HWND WINAPI GetClipboardOwner(void)
{
    HWND hWndOwner = 0;
    CLIPBOARDINFO cbinfo;

    if (CLIPBOARD_GetClipboardInfo(&cbinfo))
        hWndOwner = cbinfo.hWndOwner;

    TRACE(" hWndOwner(%p)\n", hWndOwner);

    return hWndOwner;
}


/**************************************************************************
 *		GetOpenClipboardWindow (USER32.@)
 */
HWND WINAPI GetOpenClipboardWindow(void)
{
    HWND hWndOpen = 0;
    CLIPBOARDINFO cbinfo;

    if (CLIPBOARD_GetClipboardInfo(&cbinfo))
        hWndOpen = cbinfo.hWndOpen;

    TRACE(" hWndClipWindow(%p)\n", hWndOpen);

    return hWndOpen;
}


/**************************************************************************
 *		SetClipboardViewer (USER32.@)
 */
HWND WINAPI SetClipboardViewer( HWND hWnd )
{
    HWND hwndPrev = 0;
 
    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = SET_CB_VIEWER;
        req->viewer = WIN_GetFullHandle(hWnd);
 
        if (wine_server_call_err( req ))
 	{
            ERR("Failed to set clipboard.\n");
 	}
 	else
        {
            hwndPrev = reply->old_viewer;
        }
    }
    SERVER_END_REQ;
 
    TRACE("(%p): returning %p\n", hWnd, hwndPrev);
  
    return hwndPrev;
}


/**************************************************************************
 *		GetClipboardViewer (USER32.@)
 */
HWND WINAPI GetClipboardViewer(void)
{
    HWND hWndViewer = 0;
    CLIPBOARDINFO cbinfo;

    if (CLIPBOARD_GetClipboardInfo(&cbinfo))
        hWndViewer = cbinfo.hWndViewer;

    TRACE(" hWndViewer=%p\n", hWndViewer);

    return hWndViewer;
}


/**************************************************************************
 *		ChangeClipboardChain (USER32.@)
 */
BOOL WINAPI ChangeClipboardChain(HWND hWnd, HWND hWndNext)
{
    BOOL bRet = TRUE;
    HWND hWndViewer = GetClipboardViewer();

    if (hWndViewer)
    {
        if (WIN_GetFullHandle(hWnd) == hWndViewer) 
            SetClipboardViewer(WIN_GetFullHandle(hWndNext));
	else
            bRet = !SendMessageW(hWndViewer, WM_CHANGECBCHAIN, (WPARAM)hWnd, (LPARAM)hWndNext);
    }
    else
	ERR("hWndViewer is lost\n");

    return bRet;
}


/**************************************************************************
 *		SetClipboardData (USER.141)
 */
HANDLE16 WINAPI SetClipboardData16(UINT16 wFormat, HANDLE16 hData)
{
    CLIPBOARDINFO cbinfo;
    HANDLE16 hResult = 0;

    TRACE("(%04X, %04x) !\n", wFormat, hData);

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo) ||
        (~cbinfo.flags & CB_OPEN) ||
        (~cbinfo.flags & CB_OWNER))
    {
        WARN("Clipboard not opened by calling task!\n");
    }
    else if (USER_Driver.pSetClipboardData &&
        USER_Driver.pSetClipboardData(wFormat, hData, 0))
    {
        hResult = hData;
        bCBHasChanged = TRUE;
    }

    return hResult;
}


/**************************************************************************
 *		SetClipboardData (USER32.@)
 */
HANDLE WINAPI SetClipboardData(UINT wFormat, HANDLE hData)
{
    CLIPBOARDINFO cbinfo;
    HANDLE hResult = 0;

    TRACE("(%04X, %p) !\n", wFormat, hData);

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo) ||
        (~cbinfo.flags & CB_OWNER))
    {
        WARN("Clipboard not owned by calling task!\n");
    }
    else if (USER_Driver.pSetClipboardData &&
        USER_Driver.pSetClipboardData(wFormat, 0, hData))
    {
        hResult = hData;
        bCBHasChanged = TRUE;
    }

    return hResult;
}


/**************************************************************************
 *		CountClipboardFormats (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats();
}


/**************************************************************************
 *		CountClipboardFormats (USER32.@)
 */
INT WINAPI CountClipboardFormats(void)
{
    INT count = 0;

    if (USER_Driver.pCountClipboardFormats)
        count = USER_Driver.pCountClipboardFormats();

    TRACE("returning %d\n", count);
    return count;
}


/**************************************************************************
 *		EnumClipboardFormats (USER.144)
 */
UINT16 WINAPI EnumClipboardFormats16(UINT16 wFormat)
{
    return EnumClipboardFormats(wFormat);
}


/**************************************************************************
 *		EnumClipboardFormats (USER32.@)
 */
UINT WINAPI EnumClipboardFormats(UINT wFormat)
{
    UINT wFmt = 0;
    CLIPBOARDINFO cbinfo;

    TRACE("(%04X)\n", wFormat);

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo) ||
        (~cbinfo.flags & CB_OPEN))
    {
        WARN("Clipboard not opened by calling task.\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return 0;
    }

    if (USER_Driver.pEnumClipboardFormats)
        wFmt = USER_Driver.pEnumClipboardFormats(wFormat);

    return wFmt;
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER.193)
 */
BOOL16 WINAPI IsClipboardFormatAvailable16(UINT16 wFormat)
{
    return IsClipboardFormatAvailable(wFormat);
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER32.@)
 */
BOOL WINAPI IsClipboardFormatAvailable(UINT wFormat)
{
    BOOL bret = FALSE;

    if (USER_Driver.pIsClipboardFormatAvailable)
        bret = USER_Driver.pIsClipboardFormatAvailable(wFormat);

    TRACE("%04x, returning %d\n", wFormat, bret);
    return bret;
}


/**************************************************************************
 *		GetClipboardData (USER.142)
 */
HANDLE16 WINAPI GetClipboardData16(UINT16 wFormat)
{
    HANDLE16 hData = 0;
    CLIPBOARDINFO cbinfo;

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo) ||
        (~cbinfo.flags & CB_OPEN))
    {
        WARN("Clipboard not opened by calling task.\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return 0;
    }

    if (USER_Driver.pGetClipboardData)
        USER_Driver.pGetClipboardData(wFormat, &hData, NULL);

    return hData;
}


/**************************************************************************
 *		GetClipboardData (USER32.@)
 */
HANDLE WINAPI GetClipboardData(UINT wFormat)
{
    HANDLE hData = 0;
    CLIPBOARDINFO cbinfo;

    TRACE("%04x\n", wFormat);

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo) ||
        (~cbinfo.flags & CB_OPEN))
    {
        WARN("Clipboard not opened by calling task.\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return 0;
    }

    if (USER_Driver.pGetClipboardData)
        USER_Driver.pGetClipboardData(wFormat, NULL, &hData);

    TRACE("returning %p\n", hData);
    return hData;
}


/**************************************************************************
 *		GetPriorityClipboardFormat (USER32.@)
 */
INT WINAPI GetPriorityClipboardFormat(UINT *list, INT nCount)
{
    int i;

    TRACE("()\n");

    if(CountClipboardFormats() == 0)
        return 0;

    for (i = 0; i < nCount; i++)
        if (IsClipboardFormatAvailable(list[i]))
            return list[i];

    return -1;
}


/**************************************************************************
 *		GetClipboardSequenceNumber (USER32.@)
 * Supported on Win2k/Win98
 * MSDN: Windows clipboard code keeps a serial number for the clipboard
 * for each window station.  The number is incremented whenever the
 * contents change or are emptied.
 * If you do not have WINSTA_ACCESSCLIPBOARD then the function returns 0
 */
DWORD WINAPI GetClipboardSequenceNumber(VOID)
{
    CLIPBOARDINFO cbinfo;

    if (!CLIPBOARD_GetClipboardInfo(&cbinfo))
    {
        ERR("Failed to get clipboard information.\n");
	return 0;
    }

    TRACE("returning %x\n", cbinfo.seqno);
    return cbinfo.seqno;
}
