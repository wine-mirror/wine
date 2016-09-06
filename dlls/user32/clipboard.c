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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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

#include <assert.h>
#include <stdarg.h>
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
#include "user_private.h"
#include "win.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

/*
 * Indicates if data has changed since open.
 */
static BOOL bCBHasChanged = FALSE;


/* formats that can be synthesized are: CF_TEXT, CF_OEMTEXT, CF_UNICODETEXT,
   CF_BITMAP, CF_DIB, CF_DIBV5, CF_ENHMETAFILE, CF_METAFILEPICT */

static UINT synthesized_formats[CF_MAX];

/* add a synthesized format to the list */
static void add_synthesized_format( UINT format, UINT from )
{
    assert( format < CF_MAX );
    SetClipboardData( format, 0 );
    synthesized_formats[format] = from;
}

/* store the current locale in the CF_LOCALE format */
static void set_clipboard_locale(void)
{
    HANDLE data = GlobalAlloc( GMEM_FIXED, sizeof(LCID) );

    if (!data) return;
    *(LCID *)data = GetUserDefaultLCID();
    SetClipboardData( CF_LOCALE, data );
    TRACE( "added CF_LOCALE\n" );
}

/* get the clipboard locale stored in the CF_LOCALE format */
static LCID get_clipboard_locale(void)
{
    HANDLE data;
    LCID lcid = GetUserDefaultLCID();

    if ((data = GetClipboardData( CF_LOCALE )))
    {
        LCID *ptr = GlobalLock( data );
        if (ptr && GlobalSize( data ) >= sizeof(*ptr)) lcid = *ptr;
        GlobalUnlock( data );
    }
    return lcid;
}

/* get the codepage to use for text conversions in the specified format (CF_TEXT or CF_OEMTEXT) */
static UINT get_format_codepage( LCID lcid, UINT format )
{
    LCTYPE type = (format == CF_TEXT) ? LOCALE_IDEFAULTANSICODEPAGE : LOCALE_IDEFAULTCODEPAGE;
    UINT ret;

    if (!GetLocaleInfoW( lcid, type | LOCALE_RETURN_NUMBER, (LPWSTR)&ret, sizeof(ret)/sizeof(WCHAR) ))
        ret = (format == CF_TEXT) ? CP_ACP : CP_OEMCP;
    return ret;
}

/* add synthesized text formats based on what is already in the clipboard */
static void add_synthesized_text(void)
{
    BOOL has_text = IsClipboardFormatAvailable( CF_TEXT );
    BOOL has_oemtext = IsClipboardFormatAvailable( CF_OEMTEXT );
    BOOL has_unicode = IsClipboardFormatAvailable( CF_UNICODETEXT );

    if (!has_text && !has_oemtext && !has_unicode) return;  /* no text, nothing to do */

    if (!IsClipboardFormatAvailable( CF_LOCALE )) set_clipboard_locale();

    if (has_unicode)
    {
        if (!has_text) add_synthesized_format( CF_TEXT, CF_UNICODETEXT );
        if (!has_oemtext) add_synthesized_format( CF_OEMTEXT, CF_UNICODETEXT );
    }
    else if (has_text)
    {
        if (!has_oemtext) add_synthesized_format( CF_OEMTEXT, CF_TEXT );
        if (!has_unicode) add_synthesized_format( CF_UNICODETEXT, CF_TEXT );
    }
    else
    {
        if (!has_text) add_synthesized_format( CF_TEXT, CF_OEMTEXT );
        if (!has_unicode) add_synthesized_format( CF_UNICODETEXT, CF_OEMTEXT );
    }
}

/* render synthesized ANSI text based on the contents of the 'from' format */
static HANDLE render_synthesized_textA( HANDLE data, UINT format, UINT from )
{
    void *src;
    WCHAR *srcW = NULL;
    HANDLE ret = 0;
    LCID lcid = get_clipboard_locale();
    UINT codepage = get_format_codepage( lcid, format );
    UINT len, size = GlobalSize( data );

    if (!(src = GlobalLock( data ))) return 0;

    if (from != CF_UNICODETEXT)  /* first convert incoming format to Unicode */
    {
        UINT from_codepage = get_format_codepage( lcid, from );
        len = MultiByteToWideChar( from_codepage, 0, src, size, NULL, 0 );
        if (!(srcW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) goto done;
        MultiByteToWideChar( from_codepage, 0, src, size, srcW, len );
        src = srcW;
        size = len * sizeof(WCHAR);
    }

    len = WideCharToMultiByte( codepage, 0, src, size / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if ((ret = GlobalAlloc( GMEM_FIXED, len )))
        WideCharToMultiByte( codepage, 0, src, size / sizeof(WCHAR), ret, len, NULL, NULL );

done:
    HeapFree( GetProcessHeap(), 0, srcW );
    GlobalUnlock( data );
    return ret;
}

/* render synthesized Unicode text based on the contents of the 'from' format */
static HANDLE render_synthesized_textW( HANDLE data, UINT from )
{
    char *src;
    HANDLE ret;
    UINT len, size = GlobalSize( data );
    UINT codepage = get_format_codepage( get_clipboard_locale(), from );

    if (!(src = GlobalLock( data ))) return 0;

    len = MultiByteToWideChar( codepage, 0, src, size, NULL, 0 );
    if ((ret = GlobalAlloc( GMEM_FIXED, len * sizeof(WCHAR) )))
        MultiByteToWideChar( codepage, 0, src, size, ret, len );

    GlobalUnlock( data );
    return ret;
}

/* render a synthesized format */
static HANDLE render_synthesized_format( UINT format, UINT from )
{
    HANDLE data = GetClipboardData( from );

    if (!data) return 0;
    TRACE( "rendering %04x from %04x\n", format, from );

    switch (format)
    {
    case CF_TEXT:
    case CF_OEMTEXT:
        data = render_synthesized_textA( data, format, from );
        break;
    case CF_UNICODETEXT:
        data = render_synthesized_textW( data, from );
        break;
    default:
        assert( 0 );
    }
    if (data)
    {
        TRACE( "adding %04x %p\n", format, data );
        SetClipboardData( format, data );
    }
    return data;
}

/**************************************************************************
 *                      get_clipboard_flags
 */
static UINT get_clipboard_flags(void)
{
    UINT ret = 0;

    SERVER_START_REQ( set_clipboard_info )
    {
        if (!wine_server_call_err( req )) ret = reply->flags;
    }
    SERVER_END_REQ;

    return ret;
}


/**************************************************************************
 *	CLIPBOARD_ReleaseOwner
 */
void CLIPBOARD_ReleaseOwner( HWND hwnd )
{
    HWND viewer = 0;

    SendMessageW( hwnd, WM_RENDERALLFORMATS, 0, 0 );

    SERVER_START_REQ( release_clipboard )
    {
        req->owner = wine_server_user_handle( hwnd );
        if (!wine_server_call( req )) viewer = wine_server_ptr_handle( reply->viewer );
    }
    SERVER_END_REQ;

    if (viewer) SendNotifyMessageW( viewer, WM_DRAWCLIPBOARD, (WPARAM)GetClipboardOwner(), 0 );
}


/**************************************************************************
 *                WIN32 Clipboard implementation
 **************************************************************************/

/**************************************************************************
 *		RegisterClipboardFormatW (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatW( LPCWSTR name )
{
    return GlobalAddAtomW( name );
}


/**************************************************************************
 *		RegisterClipboardFormatA (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatA( LPCSTR name )
{
    return GlobalAddAtomA( name );
}


/**************************************************************************
 *		GetClipboardFormatNameW (USER32.@)
 */
INT WINAPI GetClipboardFormatNameW(UINT wFormat, LPWSTR retStr, INT maxlen)
{
    if (wFormat < MAXINTATOM) return 0;
    return GlobalGetAtomNameW( wFormat, retStr, maxlen );
}


/**************************************************************************
 *		GetClipboardFormatNameA (USER32.@)
 */
INT WINAPI GetClipboardFormatNameA(UINT wFormat, LPSTR retStr, INT maxlen)
{
    if (wFormat < MAXINTATOM) return 0;
    return GlobalGetAtomNameA( wFormat, retStr, maxlen );
}


/**************************************************************************
 *		OpenClipboard (USER32.@)
 */
BOOL WINAPI OpenClipboard( HWND hwnd )
{
    BOOL ret;

    TRACE( "%p\n", hwnd );

    SERVER_START_REQ( open_clipboard )
    {
        req->window = wine_server_user_handle( hwnd );
        if ((ret = !wine_server_call_err( req )))
        {
            if (!reply->owner)
            {
                bCBHasChanged = FALSE;
                memset( synthesized_formats, 0, sizeof(synthesized_formats) );
            }
        }
    }
    SERVER_END_REQ;

    return ret;
}


/**************************************************************************
 *		CloseClipboard (USER32.@)
 */
BOOL WINAPI CloseClipboard(void)
{
    HWND viewer = 0;
    BOOL ret, owner = FALSE;

    TRACE("() Changed=%d\n", bCBHasChanged);

    if (bCBHasChanged)
    {
        memset( synthesized_formats, 0, sizeof(synthesized_formats) );
        add_synthesized_text();
    }

    SERVER_START_REQ( close_clipboard )
    {
        req->changed = bCBHasChanged;
        if ((ret = !wine_server_call_err( req )))
        {
            viewer = wine_server_ptr_handle( reply->viewer );
            owner = reply->owner;
        }
    }
    SERVER_END_REQ;

    if (!ret) return FALSE;

    if (bCBHasChanged)
    {
        if (owner) USER_Driver->pEndClipboardUpdate();
        bCBHasChanged = FALSE;
    }
    if (viewer) SendNotifyMessageW( viewer, WM_DRAWCLIPBOARD, (WPARAM)GetClipboardOwner(), 0 );
    return TRUE;
}


/**************************************************************************
 *		EmptyClipboard (USER32.@)
 * Empties and acquires ownership of the clipboard
 */
BOOL WINAPI EmptyClipboard(void)
{
    BOOL ret;
    HWND owner = GetClipboardOwner();

    TRACE("()\n");

    if (owner) SendMessageTimeoutW( owner, WM_DESTROYCLIPBOARD, 0, 0, SMTO_ABORTIFHUNG, 5000, NULL );

    SERVER_START_REQ( empty_clipboard )
    {
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (ret)
    {
        USER_Driver->pEmptyClipboard();
        bCBHasChanged = TRUE;
        memset( synthesized_formats, 0, sizeof(synthesized_formats) );
    }
    return ret;
}


/**************************************************************************
 *		GetClipboardOwner (USER32.@)
 *  FIXME: Can't return the owner if the clipboard is owned by an external X-app
 */
HWND WINAPI GetClipboardOwner(void)
{
    HWND hWndOwner = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) hWndOwner = wine_server_ptr_handle( reply->owner );
    }
    SERVER_END_REQ;

    TRACE(" hWndOwner(%p)\n", hWndOwner);

    return hWndOwner;
}


/**************************************************************************
 *		GetOpenClipboardWindow (USER32.@)
 */
HWND WINAPI GetOpenClipboardWindow(void)
{
    HWND hWndOpen = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) hWndOpen = wine_server_ptr_handle( reply->window );
    }
    SERVER_END_REQ;

    TRACE(" hWndClipWindow(%p)\n", hWndOpen);

    return hWndOpen;
}


/**************************************************************************
 *		SetClipboardViewer (USER32.@)
 */
HWND WINAPI SetClipboardViewer( HWND hwnd )
{
    HWND prev = 0, owner = 0;

    SERVER_START_REQ( set_clipboard_viewer )
    {
        req->viewer = wine_server_user_handle( hwnd );
        if (!wine_server_call_err( req ))
        {
            prev = wine_server_ptr_handle( reply->old_viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (hwnd) SendNotifyMessageW( hwnd, WM_DRAWCLIPBOARD, (WPARAM)owner, 0 );

    TRACE( "(%p): returning %p\n", hwnd, prev );
    return prev;
}


/**************************************************************************
 *              GetClipboardViewer (USER32.@)
 */
HWND WINAPI GetClipboardViewer(void)
{
    HWND hWndViewer = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) hWndViewer = wine_server_ptr_handle( reply->viewer );
    }
    SERVER_END_REQ;

    TRACE(" hWndViewer=%p\n", hWndViewer);

    return hWndViewer;
}


/**************************************************************************
 *              ChangeClipboardChain (USER32.@)
 */
BOOL WINAPI ChangeClipboardChain( HWND hwnd, HWND next )
{
    NTSTATUS status;
    HWND viewer;

    if (!hwnd) return FALSE;

    SERVER_START_REQ( set_clipboard_viewer )
    {
        req->viewer = wine_server_user_handle( next );
        req->previous = wine_server_user_handle( hwnd );
        status = wine_server_call( req );
        viewer = wine_server_ptr_handle( reply->old_viewer );
    }
    SERVER_END_REQ;

    if (status == STATUS_PENDING)
        return !SendMessageW( viewer, WM_CHANGECBCHAIN, (WPARAM)hwnd, (LPARAM)next );

    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}


/**************************************************************************
 *		SetClipboardData (USER32.@)
 */
HANDLE WINAPI SetClipboardData( UINT format, HANDLE data )
{
    HANDLE hResult = 0;
    UINT flags;

    TRACE( "%04x %p\n", format, data );

    if (!format)
    {
        SetLastError( ERROR_CLIPBOARD_NOT_OPEN );
        return 0;
    }

    flags = get_clipboard_flags();
    if (!(flags & CB_OPEN_ANY))
    {
        SetLastError( ERROR_CLIPBOARD_NOT_OPEN );
        return 0;
    }

    if (USER_Driver->pSetClipboardData( format, data, flags & CB_OWNER))
    {
        hResult = data;
        bCBHasChanged = TRUE;
        if (format < CF_MAX) synthesized_formats[format] = 0;
    }

    return hResult;
}


/**************************************************************************
 *		CountClipboardFormats (USER32.@)
 */
INT WINAPI CountClipboardFormats(void)
{
    INT count = USER_Driver->pCountClipboardFormats();
    TRACE("returning %d\n", count);
    return count;
}


/**************************************************************************
 *		EnumClipboardFormats (USER32.@)
 */
UINT WINAPI EnumClipboardFormats(UINT wFormat)
{
    TRACE("(%04X)\n", wFormat);

    if (!(get_clipboard_flags() & CB_OPEN))
    {
        WARN("Clipboard not opened by calling task.\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return 0;
    }
    return USER_Driver->pEnumClipboardFormats(wFormat);
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER32.@)
 */
BOOL WINAPI IsClipboardFormatAvailable(UINT wFormat)
{
    BOOL bret = USER_Driver->pIsClipboardFormatAvailable(wFormat);
    TRACE("%04x, returning %d\n", wFormat, bret);
    return bret;
}


/**************************************************************************
 *		GetUpdatedClipboardFormats (USER32.@)
 */
BOOL WINAPI GetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size )
{
    UINT i = 0, cf = 0;

    if (!out_size)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }
    if (!(*out_size = CountClipboardFormats())) return TRUE;  /* nothing else to do */

    if (!formats)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }
    if (size < *out_size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    /* FIXME: format list could change in the meantime */
    while ((cf = USER_Driver->pEnumClipboardFormats( cf ))) formats[i++] = cf;
    return TRUE;
}


/**************************************************************************
 *		GetClipboardData (USER32.@)
 */
HANDLE WINAPI GetClipboardData( UINT format )
{
    HANDLE data = 0;

    TRACE( "%04x\n", format );

    if (!(get_clipboard_flags() & CB_OPEN))
    {
        WARN("Clipboard not opened by calling task.\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return 0;
    }
    if (format < CF_MAX && synthesized_formats[format])
        data = render_synthesized_format( format, synthesized_formats[format] );
    else
        data = USER_Driver->pGetClipboardData( format );

    TRACE( "returning %p\n", data );
    return data;
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
    DWORD seqno = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) seqno = reply->seqno;
    }
    SERVER_END_REQ;

    TRACE("returning %x\n", seqno);
    return seqno;
}

/**************************************************************************
 *		AddClipboardFormatListener (USER32.@)
 */
BOOL WINAPI AddClipboardFormatListener(HWND hwnd)
{
    BOOL ret;

    SERVER_START_REQ( add_clipboard_listener )
    {
        req->window = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/**************************************************************************
 *		RemoveClipboardFormatListener (USER32.@)
 */
BOOL WINAPI RemoveClipboardFormatListener(HWND hwnd)
{
    BOOL ret;

    SERVER_START_REQ( remove_clipboard_listener )
    {
        req->window = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}
