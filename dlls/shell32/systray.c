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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winnls.h"
#include "winuser.h"
#include "shellapi.h"
#include "shell32_main.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(systray);

struct notify_data_icon
{
    /* data for the icon bitmap */
    UINT width;
    UINT height;
    UINT planes;
    UINT bpp;
};

struct notify_data  /* platform-independent format for NOTIFYICONDATA */
{
    LONG  hWnd;
    UINT  uID;
    UINT  uFlags;
    UINT  uCallbackMessage;
    struct notify_data_icon icon_info; /* systray icon bitmap info */
    WCHAR szTip[128];
    DWORD dwState;
    DWORD dwStateMask;
    WCHAR szInfo[256];
    union {
        UINT uTimeout;
        UINT uVersion;
    } u;
    WCHAR szInfoTitle[64];
    DWORD dwInfoFlags;
    GUID  guidItem;
    struct notify_data_icon balloon_icon_info; /* balloon icon bitmap info */
    BYTE icon_data[];
};


/*************************************************************************
 * Shell_NotifyIcon			[SHELL32.296]
 * Shell_NotifyIconA			[SHELL32.297]
 */
BOOL WINAPI Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA pnid)
{
    NOTIFYICONDATAW nidW;
    INT cbSize;

    /* Validate the cbSize as Windows XP does */
    if (pnid->cbSize != NOTIFYICONDATAA_V1_SIZE &&
        pnid->cbSize != NOTIFYICONDATAA_V2_SIZE &&
        pnid->cbSize != NOTIFYICONDATAA_V3_SIZE &&
        pnid->cbSize != sizeof(NOTIFYICONDATAA))
    {
        WARN("Invalid cbSize (%ld) - using only Win95 fields (size=%ld)\n",
            pnid->cbSize, NOTIFYICONDATAA_V1_SIZE);
        cbSize = NOTIFYICONDATAA_V1_SIZE;
    }
    else
        cbSize = pnid->cbSize;

    ZeroMemory(&nidW, sizeof(nidW));
    nidW.cbSize = sizeof(nidW);
    nidW.hWnd   = pnid->hWnd;
    nidW.uID    = pnid->uID;
    nidW.uFlags = pnid->uFlags;
    nidW.uCallbackMessage = pnid->uCallbackMessage;
    nidW.hIcon  = pnid->hIcon;

    /* szTip */
    if (pnid->uFlags & NIF_TIP)
        MultiByteToWideChar(CP_ACP, 0, pnid->szTip, -1, nidW.szTip, ARRAY_SIZE(nidW.szTip));

    if (cbSize >= NOTIFYICONDATAA_V2_SIZE)
    {
        nidW.dwState      = pnid->dwState;
        nidW.dwStateMask  = pnid->dwStateMask;

        /* szInfo, szInfoTitle */
        if (pnid->uFlags & NIF_INFO)
        {
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfo, -1,  nidW.szInfo, ARRAY_SIZE(nidW.szInfo));
            MultiByteToWideChar(CP_ACP, 0, pnid->szInfoTitle, -1, nidW.szInfoTitle, ARRAY_SIZE(nidW.szInfoTitle));
        }

        nidW.uTimeout = pnid->uTimeout;
        nidW.dwInfoFlags = pnid->dwInfoFlags;
    }
    
    if (cbSize >= NOTIFYICONDATAA_V3_SIZE)
        nidW.guidItem = pnid->guidItem;

    if (cbSize >= sizeof(NOTIFYICONDATAA))
        nidW.hBalloonIcon = pnid->hBalloonIcon;
    return Shell_NotifyIconW(dwMessage, &nidW);
}


/*************************************************************************
 * get_bitmap_info Helper function for filling BITMAP structs and calculating buffer size in bits
 */
static void get_bitmap_info( ICONINFO *icon_info, BITMAP *mask, BITMAP *color, LONG *mask_bits, LONG *color_bits )
{
    if ((icon_info->hbmMask && !GetObjectW( icon_info->hbmMask, sizeof(*mask), mask )) ||
        (icon_info->hbmColor && !GetObjectW( icon_info->hbmColor, sizeof(*color), color )))
    {
        if (icon_info->hbmMask) DeleteObject( icon_info->hbmMask );
        if (icon_info->hbmColor) DeleteObject( icon_info->hbmColor );
        memset( icon_info, 0, sizeof(*icon_info) );
        return;
    }

    if (icon_info->hbmMask)
        *mask_bits = (mask->bmPlanes * mask->bmWidth * mask->bmHeight * mask->bmBitsPixel + 15) / 16 * 2;
    if (icon_info->hbmColor)
        *color_bits = (color->bmPlanes * color->bmWidth * color->bmHeight * color->bmBitsPixel + 15) / 16 * 2;
}


/*************************************************************************
 * fill_icon_info Helper function for filling struct image metadata and buffer with bitmap data
 */
static void fill_icon_info( const BITMAP *mask, const BITMAP *color, LONG mask_bits, LONG color_bits,
                            ICONINFO *icon_info, struct notify_data_icon *msg_icon_info, BYTE *image_data_buffer )
{
    if (icon_info->hbmColor)
    {
        msg_icon_info->width  = color->bmWidth;
        msg_icon_info->height = color->bmHeight;
        msg_icon_info->planes = color->bmPlanes;
        msg_icon_info->bpp    = color->bmBitsPixel;
        GetBitmapBits( icon_info->hbmColor, color_bits, image_data_buffer + mask_bits );
        DeleteObject( icon_info->hbmColor );
    }
    else
    {
        msg_icon_info->width  = mask->bmWidth;
        msg_icon_info->height = mask->bmHeight / 2;
        msg_icon_info->planes = 1;
        msg_icon_info->bpp    = 1;
    }

    GetBitmapBits( icon_info->hbmMask, mask_bits, image_data_buffer );
    DeleteObject( icon_info->hbmMask );
}

/*************************************************************************
 * Shell_NotifyIconW			[SHELL32.298]
 */
BOOL WINAPI Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW nid)
{
    HWND tray;
    COPYDATASTRUCT cds;
    struct notify_data data_buffer;
    struct notify_data *data = &data_buffer;
    ICONINFO icon_info = { 0 }, balloon_icon_info = { 0 };
    BITMAP mask, color, balloon_mask, balloon_color;
    LONG mask_size = 0, color_size = 0, balloon_mask_size = 0, balloon_color_size = 0;
    BOOL ret;

    TRACE("dwMessage = %ld, nid->cbSize=%ld\n", dwMessage, nid->cbSize);

    /* Validate the cbSize so that WM_COPYDATA doesn't crash the application */
    if (nid->cbSize != NOTIFYICONDATAW_V1_SIZE &&
        nid->cbSize != NOTIFYICONDATAW_V2_SIZE &&
        nid->cbSize != NOTIFYICONDATAW_V3_SIZE &&
        nid->cbSize != sizeof(NOTIFYICONDATAW))
    {
        NOTIFYICONDATAW newNid;

        WARN("Invalid cbSize (%ld) - using only Win95 fields (size=%ld)\n",
            nid->cbSize, NOTIFYICONDATAW_V1_SIZE);
        CopyMemory(&newNid, nid, NOTIFYICONDATAW_V1_SIZE);
        newNid.cbSize = NOTIFYICONDATAW_V1_SIZE;
        return Shell_NotifyIconW(dwMessage, &newNid);
    }

    tray = FindWindowExW(0, NULL, L"Shell_TrayWnd", NULL);
    if (!tray)
    {
        SetLastError(E_FAIL);
        return FALSE;
    }

    cds.dwData = dwMessage;
    cds.cbData = sizeof(*data);
    memset( data, 0, sizeof(*data) );

    /* FIXME: if statement only needed because we don't support interprocess
     * icon handles */
    if (nid->uFlags & NIF_ICON)
        GetIconInfo( nid->hIcon, &icon_info );

    if ((nid->uFlags & NIF_INFO) && (nid->dwInfoFlags & NIIF_ICONMASK) == NIIF_USER)
        GetIconInfo( nid->hBalloonIcon, &balloon_icon_info );

    get_bitmap_info( &icon_info, &mask, &color, &mask_size, &color_size );
    cds.cbData += mask_size + color_size;

    get_bitmap_info( &balloon_icon_info, &balloon_mask, &balloon_color, &balloon_mask_size, &balloon_color_size );
    cds.cbData += balloon_mask_size + balloon_color_size;

    if (cds.cbData > sizeof(*data))
    {
        BYTE *buffer;
        buffer = malloc(cds.cbData);
        if (!buffer)
        {
            if (icon_info.hbmMask) DeleteObject( icon_info.hbmMask );
            if (icon_info.hbmColor) DeleteObject( icon_info.hbmColor );
            if (balloon_icon_info.hbmMask) DeleteObject( balloon_icon_info.hbmMask );
            if (balloon_icon_info.hbmColor) DeleteObject( balloon_icon_info.hbmColor );
            SetLastError(E_OUTOFMEMORY);
            return FALSE;
        }

        data = (struct notify_data *)buffer;
        buffer = data->icon_data;
        memset( data, 0, sizeof(*data) );

        if (icon_info.hbmMask)
        {
            fill_icon_info( &mask, &color, mask_size, color_size, &icon_info, &data->icon_info, buffer );
            buffer += mask_size + color_size;
        }

        if (balloon_icon_info.hbmMask)
        {
            fill_icon_info( &balloon_mask, &balloon_color, balloon_mask_size, balloon_color_size,
                            &balloon_icon_info, &data->balloon_icon_info, buffer );
            buffer += mask_size + color_size;
        }
    }

    data->hWnd   = HandleToLong( nid->hWnd );
    data->uID    = nid->uID;
    data->uFlags = nid->uFlags;
    if (data->uFlags & NIF_MESSAGE)
        data->uCallbackMessage = nid->uCallbackMessage;
    if (data->uFlags & NIF_TIP)
        lstrcpynW( data->szTip, nid->szTip, ARRAY_SIZE(data->szTip));
    if (data->uFlags & NIF_STATE)
    {
        data->dwState     = nid->dwState;
        data->dwStateMask = nid->dwStateMask;
    }
    if (data->uFlags & NIF_INFO)
    {
        lstrcpynW( data->szInfo, nid->szInfo, ARRAY_SIZE(data->szInfo) );
        lstrcpynW( data->szInfoTitle, nid->szInfoTitle, ARRAY_SIZE(data->szInfoTitle));
        data->u.uTimeout  = nid->uTimeout;
        data->dwInfoFlags = nid->dwInfoFlags;
    }
    if (data->uFlags & NIF_GUID)
        data->guidItem = nid->guidItem;
    if (dwMessage == NIM_SETVERSION)
        data->u.uVersion = nid->uVersion;
    /* FIXME: balloon icon */

    cds.lpData = data;
    ret = SendMessageW(tray, WM_COPYDATA, (WPARAM)nid->hWnd, (LPARAM)&cds);
    if (data != &data_buffer) free(data);
    SetLastError(ret ? S_OK : E_FAIL);
    return ret;
}

/*************************************************************************
 * Shell_NotifyIconGetRect		[SHELL32.@]
 */
HRESULT WINAPI Shell_NotifyIconGetRect(const NOTIFYICONIDENTIFIER* identifier, RECT* icon_location)
{
    FIXME("stub (%p) (%p)\n", identifier, icon_location);
    return E_NOTIMPL;
}
