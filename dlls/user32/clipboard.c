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

/* get a debug string for a format id */
static const char *debugstr_format( UINT id )
{
    WCHAR buffer[256];

    if (GetClipboardFormatNameW( id, buffer, 256 ))
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
        if (ptr)
        {
            if (GlobalSize( data ) >= sizeof(*ptr)) lcid = *ptr;
            GlobalUnlock( data );
        }
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

/* add synthesized bitmap formats based on what is already in the clipboard */
static void add_synthesized_bitmap(void)
{
    BOOL has_dib = IsClipboardFormatAvailable( CF_DIB );
    BOOL has_dibv5 = IsClipboardFormatAvailable( CF_DIBV5 );
    BOOL has_bitmap = IsClipboardFormatAvailable( CF_BITMAP );

    if (!has_bitmap && !has_dib && !has_dibv5) return;  /* nothing to do */
    if (has_bitmap && has_dib && has_dibv5) return;  /* nothing to synthesize */

    if (has_bitmap)
    {
        if (!has_dib) add_synthesized_format( CF_DIB, CF_BITMAP );
        if (!has_dibv5) add_synthesized_format( CF_DIBV5, CF_BITMAP );
    }
    else if (has_dib)
    {
        if (!has_bitmap) add_synthesized_format( CF_BITMAP, CF_DIB );
        if (!has_dibv5) add_synthesized_format( CF_DIBV5, CF_DIB );
    }
    else
    {
        if (!has_bitmap) add_synthesized_format( CF_BITMAP, CF_DIBV5 );
        if (!has_dib) add_synthesized_format( CF_DIB, CF_DIBV5 );
    }
}

/* add synthesized metafile formats based on what is already in the clipboard */
static void add_synthesized_metafile(void)
{
    BOOL has_mf = IsClipboardFormatAvailable( CF_METAFILEPICT );
    BOOL has_emf = IsClipboardFormatAvailable( CF_ENHMETAFILE );

    if (!has_mf && has_emf) add_synthesized_format( CF_METAFILEPICT, CF_ENHMETAFILE );
    else if (!has_emf && has_mf) add_synthesized_format( CF_ENHMETAFILE, CF_METAFILEPICT );
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

/* render a synthesized bitmap based on the DIB clipboard data */
static HANDLE render_synthesized_bitmap( HANDLE data, UINT from )
{
    BITMAPINFO *bmi;
    HANDLE ret = 0;
    HDC hdc = GetDC( 0 );

    if ((bmi = GlobalLock( data )))
    {
        /* FIXME: validate data size */
        ret = CreateDIBitmap( hdc, &bmi->bmiHeader, CBM_INIT,
                              (char *)bmi + bitmap_info_size( bmi, DIB_RGB_COLORS ),
                              bmi, DIB_RGB_COLORS );
        GlobalUnlock( data );
    }
    ReleaseDC( 0, hdc );
    return ret;
}

/* render a synthesized DIB based on the clipboard data */
static HANDLE render_synthesized_dib( HANDLE data, UINT format, UINT from )
{
    BITMAPINFO *bmi, *src;
    DWORD src_size, header_size, bits_size;
    HANDLE ret = 0;
    HDC hdc = GetDC( 0 );

    if (from == CF_BITMAP)
    {
        BITMAP bmp;

        if (!GetObjectW( data, sizeof(bmp), &bmp )) goto done;

        bits_size = abs( bmp.bmHeight ) * (((bmp.bmWidth * bmp.bmBitsPixel + 31) / 8) & ~3);
        if (bmp.bmBitsPixel <= 8)
            header_size = offsetof( BITMAPINFO, bmiColors[1 << bmp.bmBitsPixel] );
        else
            header_size = (format == CF_DIBV5) ? sizeof(BITMAPV5HEADER) : sizeof(BITMAPINFOHEADER);

        if (!(ret = GlobalAlloc( GMEM_FIXED, header_size + bits_size ))) goto done;
        bmi = (BITMAPINFO *)ret;
        memset( bmi, 0, header_size );
        bmi->bmiHeader.biSize        = header_size;
        bmi->bmiHeader.biWidth       = bmp.bmWidth;
        bmi->bmiHeader.biHeight      = bmp.bmHeight;
        bmi->bmiHeader.biPlanes      = 1;
        bmi->bmiHeader.biBitCount    = bmp.bmBitsPixel;
        bmi->bmiHeader.biCompression = BI_RGB;
        GetDIBits( hdc, data, 0, bmp.bmHeight, (char *)bmi + header_size, bmi, DIB_RGB_COLORS );
    }
    else
    {
        if (!(src = GlobalLock( data ))) goto done;

        src_size = bitmap_info_size( src, DIB_RGB_COLORS );
        bits_size = GlobalSize( data ) - src_size;
        header_size = (format == CF_DIBV5) ? sizeof(BITMAPV5HEADER) :
            offsetof( BITMAPINFO, bmiColors[src->bmiHeader.biCompression == BI_BITFIELDS ? 3 : 0] );

        if ((ret = GlobalAlloc( GMEM_FIXED, header_size + bits_size )))
        {
            bmi = (BITMAPINFO *)ret;
            memset( bmi, 0, header_size );
            memcpy( bmi, src, min( header_size, src_size ));
            bmi->bmiHeader.biSize = header_size;
            /* FIXME: convert colors according to DIBv5 color profile */
            memcpy( (char *)bmi + header_size, (char *)src + src_size, bits_size );
        }
        GlobalUnlock( data );
    }

done:
    ReleaseDC( 0, hdc );
    return ret;
}

/* render a synthesized metafile based on the enhmetafile clipboard data */
static HANDLE render_synthesized_metafile( HANDLE data )
{
    HANDLE ret = 0;
    UINT size;
    void *bits;
    METAFILEPICT *pict;
    ENHMETAHEADER header;
    HDC hdc = GetDC( 0 );

    size = GetWinMetaFileBits( data, 0, NULL, MM_ISOTROPIC, hdc );
    if ((bits = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        if (GetEnhMetaFileHeader( data, sizeof(header), &header ) &&
            GetWinMetaFileBits( data, size, bits, MM_ISOTROPIC, hdc ))
        {
            if ((ret = GlobalAlloc( GMEM_FIXED, sizeof(*pict) )))
            {
                pict = (METAFILEPICT *)ret;
                pict->mm   = MM_ISOTROPIC;
                pict->xExt = header.rclFrame.right - header.rclFrame.left;
                pict->yExt = header.rclFrame.bottom - header.rclFrame.top;
                pict->hMF  = SetMetaFileBitsEx( size, bits );
            }
        }
        HeapFree( GetProcessHeap(), 0, bits );
    }
    ReleaseDC( 0, hdc );
    return ret;
}

/* render a synthesized enhmetafile based on the metafile clipboard data */
static HANDLE render_synthesized_enhmetafile( HANDLE data )
{
    METAFILEPICT *pict;
    HANDLE ret = 0;
    UINT size;
    void *bits;

    if (!(pict = GlobalLock( data ))) return 0;

    size = GetMetaFileBitsEx( pict->hMF, 0, NULL );
    if ((bits = HeapAlloc( GetProcessHeap(), 0, size )))
    {
        GetMetaFileBitsEx( pict->hMF, size, bits );
        ret = SetWinMetaFileBits( size, bits, NULL, pict );
        HeapFree( GetProcessHeap(), 0, bits );
    }

    GlobalUnlock( data );
    return ret;
}

/* render a synthesized format */
static HANDLE render_synthesized_format( UINT format, UINT from )
{
    HANDLE data = GetClipboardData( from );

    if (!data) return 0;
    TRACE( "rendering %s from %s\n", debugstr_format( format ), debugstr_format( from ));

    switch (format)
    {
    case CF_TEXT:
    case CF_OEMTEXT:
        data = render_synthesized_textA( data, format, from );
        break;
    case CF_UNICODETEXT:
        data = render_synthesized_textW( data, from );
        break;
    case CF_BITMAP:
        data = render_synthesized_bitmap( data, from );
        break;
    case CF_DIB:
    case CF_DIBV5:
        data = render_synthesized_dib( data, format, from );
        break;
    case CF_METAFILEPICT:
        data = render_synthesized_metafile( data );
        break;
    case CF_ENHMETAFILE:
        data = render_synthesized_enhmetafile( data );
        break;
    default:
        assert( 0 );
    }
    if (data)
    {
        TRACE( "adding %s %p\n", debugstr_format( format ), data );
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
    HWND viewer = 0, owner = 0;

    SendMessageW( hwnd, WM_RENDERALLFORMATS, 0, 0 );

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

    if (viewer) SendNotifyMessageW( viewer, WM_DRAWCLIPBOARD, (WPARAM)owner, 0 );
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
INT WINAPI GetClipboardFormatNameW( UINT format, LPWSTR buffer, INT maxlen )
{
    if (format < MAXINTATOM || format > 0xffff) return 0;
    return GlobalGetAtomNameW( format, buffer, maxlen );
}


/**************************************************************************
 *		GetClipboardFormatNameA (USER32.@)
 */
INT WINAPI GetClipboardFormatNameA( UINT format, LPSTR buffer, INT maxlen )
{
    if (format < MAXINTATOM || format > 0xffff) return 0;
    return GlobalGetAtomNameA( format, buffer, maxlen );
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
    HWND viewer = 0, owner = 0;
    BOOL ret;

    TRACE("() Changed=%d\n", bCBHasChanged);

    if (bCBHasChanged)
    {
        memset( synthesized_formats, 0, sizeof(synthesized_formats) );
        add_synthesized_text();
        add_synthesized_bitmap();
        add_synthesized_metafile();
    }

    SERVER_START_REQ( close_clipboard )
    {
        req->changed = bCBHasChanged;
        if ((ret = !wine_server_call_err( req )))
        {
            viewer = wine_server_ptr_handle( reply->viewer );
            owner = wine_server_ptr_handle( reply->owner );
        }
    }
    SERVER_END_REQ;

    if (!ret) return FALSE;

    if (bCBHasChanged)
    {
        USER_Driver->pEndClipboardUpdate();
        bCBHasChanged = FALSE;
    }
    if (viewer) SendNotifyMessageW( viewer, WM_DRAWCLIPBOARD, (WPARAM)owner, 0 );
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

    TRACE( "owner %p\n", owner );

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
 */
HWND WINAPI GetClipboardOwner(void)
{
    HWND hWndOwner = 0;

    SERVER_START_REQ( get_clipboard_info )
    {
        if (!wine_server_call_err( req )) hWndOwner = wine_server_ptr_handle( reply->owner );
    }
    SERVER_END_REQ;

    TRACE( "returning %p\n", hWndOwner );

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

    TRACE( "returning %p\n", hWndOpen );

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

    TRACE( "%p returning %p\n", hwnd, prev );
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

    TRACE( "returning %p\n", hWndViewer );

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

    TRACE( "%s %p\n", debugstr_format( format ), data );

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
UINT WINAPI EnumClipboardFormats( UINT format )
{
    UINT ret = 0;

    if (!(get_clipboard_flags() & CB_OPEN))
    {
        WARN("Clipboard not opened by calling task.\n");
        SetLastError(ERROR_CLIPBOARD_NOT_OPEN);
        return 0;
    }
    SetLastError( 0 );
    ret = USER_Driver->pEnumClipboardFormats( format );
    TRACE( "%s -> %s\n", debugstr_format( format ), debugstr_format( ret ));
    return ret;
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER32.@)
 */
BOOL WINAPI IsClipboardFormatAvailable( UINT format )
{
    BOOL ret = USER_Driver->pIsClipboardFormatAvailable( format );
    TRACE( "%s -> %u\n", debugstr_format( format ), ret );
    return ret;
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

    TRACE( "%s\n", debugstr_format( format ));

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

    TRACE( "%p %u\n", list, nCount );

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

    TRACE( "returning %u\n", seqno );
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
