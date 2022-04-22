/*
 * WIN32 clipboard implementation
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Alex Korobka
 * Copyright 1999 Noel Borthwick
 * Copyright 2003 Ulrich Czekalla for CodeWeavers
 * Copyright 2016 Alexandre Julliard
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
 */

#if 0
#pragma makedep unix
#endif

#include "win32u_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);


/* get a debug string for a format id */
static const char *debugstr_format( UINT id )
{
    WCHAR buffer[256];
    DWORD le = GetLastError();
    BOOL r = NtUserGetClipboardFormatName( id, buffer, ARRAYSIZE(buffer) );
    SetLastError(le);

    if (r)
        return wine_dbg_sprintf( "%04x %s", id, debugstr_w(buffer) );

    switch (id)
    {
#define BUILTIN(id) case id: return #id;
    BUILTIN(CF_TEXT)
    BUILTIN(CF_BITMAP)
    BUILTIN(CF_METAFILEPICT)
    BUILTIN(CF_SYLK)
    BUILTIN(CF_DIF)
    BUILTIN(CF_TIFF)
    BUILTIN(CF_OEMTEXT)
    BUILTIN(CF_DIB)
    BUILTIN(CF_PALETTE)
    BUILTIN(CF_PENDATA)
    BUILTIN(CF_RIFF)
    BUILTIN(CF_WAVE)
    BUILTIN(CF_UNICODETEXT)
    BUILTIN(CF_ENHMETAFILE)
    BUILTIN(CF_HDROP)
    BUILTIN(CF_LOCALE)
    BUILTIN(CF_DIBV5)
    BUILTIN(CF_OWNERDISPLAY)
    BUILTIN(CF_DSPTEXT)
    BUILTIN(CF_DSPBITMAP)
    BUILTIN(CF_DSPMETAFILEPICT)
    BUILTIN(CF_DSPENHMETAFILE)
#undef BUILTIN
    default: return wine_dbg_sprintf( "%04x", id );
    }
}

/**************************************************************************
 *           NtUserCloseClipboard    (win32u.@)
 */
BOOL WINAPI NtUserCloseClipboard(void)
{
    HWND viewer = 0, owner = 0;
    BOOL ret;

    TRACE( "\n" );

    SERVER_START_REQ( close_clipboard )
    {
        if ((ret = !wine_server_call_err( req )))
        {
            viewer = wine_server_ptr_handle( reply->viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (viewer) NtUserMessageCall( viewer, WM_DRAWCLIPBOARD, (WPARAM)owner, 0,
                                   0, NtUserSendNotifyMessage, FALSE );
    return ret;
}

/**************************************************************************
 *           NtUserCountClipboardFormats    (win32u.@)
 */
INT WINAPI NtUserCountClipboardFormats(void)
{
    INT count = 0;

    user_driver->pUpdateClipboard();

    SERVER_START_REQ( get_clipboard_formats )
    {
        wine_server_call( req );
        count = reply->count;
    }
    SERVER_END_REQ;

    TRACE( "returning %d\n", count );
    return count;
}

/**************************************************************************
 *	     NtUserIsClipboardFormatAvailable    (win32u.@)
 */
BOOL WINAPI NtUserIsClipboardFormatAvailable( UINT format )
{
    BOOL ret = FALSE;

    if (!format) return FALSE;

    user_driver->pUpdateClipboard();

    SERVER_START_REQ( get_clipboard_formats )
    {
        req->format = format;
        if (!wine_server_call_err( req )) ret = (reply->count > 0);
    }
    SERVER_END_REQ;
    TRACE( "%s -> %u\n", debugstr_format( format ), ret );
    return ret;
}

/**************************************************************************
 *	     NtUserGetUpdatedClipboardFormats    (win32u.@)
 */
BOOL WINAPI NtUserGetUpdatedClipboardFormats( UINT *formats, UINT size, UINT *out_size )
{
    BOOL ret;

    if (!out_size)
    {
        SetLastError( ERROR_NOACCESS );
        return FALSE;
    }

    user_driver->pUpdateClipboard();

    SERVER_START_REQ( get_clipboard_formats )
    {
        if (formats) wine_server_set_reply( req, formats, size * sizeof(*formats) );
        ret = !wine_server_call_err( req );
        *out_size = reply->count;
    }
    SERVER_END_REQ;

    TRACE( "%p %u returning %u formats, ret %u\n", formats, size, *out_size, ret );
    if (!ret && !formats && *out_size) SetLastError( ERROR_NOACCESS );
    return ret;
}

/**************************************************************************
 *	     NtUserGetPriorityClipboardFormat    (win32u.@)
 */
INT WINAPI NtUserGetPriorityClipboardFormat( UINT *list, INT count )
{
    int i;

    TRACE( "%p %u\n", list, count );

    if (NtUserCountClipboardFormats() == 0)
        return 0;

    for (i = 0; i < count; i++)
        if (NtUserIsClipboardFormatAvailable( list[i] ))
            return list[i];

    return -1;
}

/**************************************************************************
 *	     NtUserGetClipboardFormatName    (win32u.@)
 */
INT WINAPI NtUserGetClipboardFormatName( UINT format, WCHAR *buffer, INT maxlen )
{
    char buf[sizeof(ATOM_BASIC_INFORMATION) + MAX_ATOM_LEN * sizeof(WCHAR)];
    ATOM_BASIC_INFORMATION *abi = (ATOM_BASIC_INFORMATION *)buf;
    UINT length = 0;

    if (format < MAXINTATOM || format > 0xffff) return 0;
    if (maxlen <= 0)
    {
        SetLastError( ERROR_MORE_DATA );
        return 0;
    }
    if (!set_ntstatus( NtQueryInformationAtom( format, AtomBasicInformation,
                                               buf, sizeof(buf), NULL )))
        return 0;

    length = min( abi->NameLength / sizeof(WCHAR), maxlen - 1 );
    if (length) memcpy( buffer, abi->Name, length * sizeof(WCHAR) );
    buffer[length] = 0;
    return length;
}

/**************************************************************************
 *	     NtUserGetClipboardOwner    (win32u.@)
 */
HWND WINAPI NtUserGetClipboardOwner(void)
{
    HWND owner = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) owner = wine_server_ptr_handle( reply->owner );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", owner );
    return owner;
}

/**************************************************************************
 *           NtUserSetClipboardViewer    (win32u.@)
 */
HWND WINAPI NtUserSetClipboardViewer( HWND hwnd )
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

    if (hwnd)
        NtUserMessageCall( hwnd, WM_DRAWCLIPBOARD, (WPARAM)owner, 0,
                           NULL, NtUserSendNotifyMessage, FALSE );

    TRACE( "%p returning %p\n", hwnd, prev );
    return prev;
}

/**************************************************************************
 *           NtUserGetClipboardViewer    (win32u.@)
 */
HWND WINAPI NtUserGetClipboardViewer(void)
{
    HWND viewer = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) viewer = wine_server_ptr_handle( reply->viewer );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", viewer );
    return viewer;
}

/**************************************************************************
 *           NtUserChangeClipboardChain    (win32u.@)
 */
BOOL WINAPI NtUserChangeClipboardChain( HWND hwnd, HWND next )
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
        return !send_message( viewer, WM_CHANGECBCHAIN, (WPARAM)hwnd, (LPARAM)next );

    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return !status;
}

/**************************************************************************
 *	     NtUserGetOpenClipboardWindow    (win32u.@)
 */
HWND WINAPI NtUserGetOpenClipboardWindow(void)
{
    HWND window = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) window = wine_server_ptr_handle( reply->window );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", window );
    return window;
}

/**************************************************************************
 *	     NtUserGetClipboardSequenceNumber    (win32u.@)
 */
DWORD WINAPI NtUserGetClipboardSequenceNumber(void)
{
    DWORD seqno = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) seqno = reply->seqno;
    }
    SERVER_END_REQ;

    TRACE( "returning %u\n", seqno );
    return seqno;
}

/* see EnumClipboardFormats */
UINT enum_clipboard_formats( UINT format )
{
    UINT ret = 0;

    SERVER_START_REQ( enum_clipboard_formats )
    {
        req->previous = format;
        if (!wine_server_call_err( req ))
        {
            ret = reply->format;
            SetLastError( ERROR_SUCCESS );
        }
    }
    SERVER_END_REQ;

    TRACE( "%s -> %s\n", debugstr_format( format ), debugstr_format( ret ));
    return ret;
}

/**************************************************************************
 *	     NtUserAddClipboardFormatListener    (win32u.@)
 */
BOOL WINAPI NtUserAddClipboardFormatListener( HWND hwnd )
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
 *	     NtUserRemoveClipboardFormatListener    (win32u.@)
 */
BOOL WINAPI NtUserRemoveClipboardFormatListener( HWND hwnd )
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

/**************************************************************************
 *           release_clipboard_owner
 */
void release_clipboard_owner( HWND hwnd )
{
    HWND viewer = 0, owner = 0;

    send_message( hwnd, WM_RENDERALLFORMATS, 0, 0 );

    SERVER_START_REQ( release_clipboard )
    {
        req->owner = wine_server_user_handle( hwnd );
        if (!wine_server_call( req ))
        {
            viewer = wine_server_ptr_handle( reply->viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (viewer)
        NtUserMessageCall( viewer, WM_DRAWCLIPBOARD, (WPARAM)owner, 0,
                           0, NtUserSendNotifyMessage, FALSE );
}
