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

#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "user_private.h"
#include "win.h"

#include "wine/list.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);


struct cached_format
{
    struct list entry;       /* entry in cache list */
    UINT        format;      /* format id */
    UINT        seqno;       /* sequence number when the data was set */
    HANDLE      handle;      /* original data handle */
};

static struct list cached_formats = LIST_INIT( cached_formats );
static struct list formats_to_free = LIST_INIT( formats_to_free );

static CRITICAL_SECTION clipboard_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &clipboard_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": clipboard_cs") }
};
static CRITICAL_SECTION clipboard_cs = { &critsect_debug, -1, 0, 0, 0, 0 };

/* platform-independent version of METAFILEPICT */
struct metafile_pict
{
    LONG       mm;
    LONG       xExt;
    LONG       yExt;
    BYTE       bits[1];
};

/* get a debug string for a format id */
static const char *debugstr_format( UINT id )
{
    WCHAR buffer[256];
    DWORD le = GetLastError();
    BOOL r = NtUserGetClipboardFormatName( id, buffer, 256 );
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

/* build the data to send to the server in SetClipboardData */
static HANDLE marshal_data( UINT format, HANDLE handle, data_size_t *ret_size )
{
    SIZE_T size;

    switch (format)
    {
    case CF_BITMAP:
    case CF_DSPBITMAP:
        {
            BITMAP bitmap, *bm;
            if (!GetObjectW( handle, sizeof(bitmap), &bitmap )) return 0;
            size = abs( bitmap.bmHeight ) * ((((bitmap.bmWidth * bitmap.bmBitsPixel) + 15) >> 3) & ~1);
            *ret_size = sizeof(bitmap) + size;
            if (!(bm = GlobalAlloc( GMEM_FIXED, *ret_size ))) return 0;
            *bm = bitmap;
            GetBitmapBits( handle, size, bm + 1 );
            return bm;
        }
    case CF_PALETTE:
        {
            LOGPALETTE *pal;
            if (!(size = GetPaletteEntries( handle, 0, 0, NULL ))) return 0;
            *ret_size = offsetof( LOGPALETTE, palPalEntry[size] );
            if (!(pal = GlobalAlloc( GMEM_FIXED, *ret_size ))) return 0;
            pal->palVersion = 0x300;
            pal->palNumEntries = size;
            GetPaletteEntries( handle, 0, size, pal->palPalEntry );
            return pal;
        }
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        {
            BYTE *ret;
            if (!(size = GetEnhMetaFileBits( handle, 0, NULL ))) return 0;
            if (!(ret = GlobalAlloc( GMEM_FIXED, size ))) return 0;
            GetEnhMetaFileBits( handle, size, ret );
            *ret_size = size;
            return ret;
        }
    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        {
            METAFILEPICT *mf;
            struct metafile_pict *mfbits;
            if (!(mf = GlobalLock( handle ))) return 0;
            if (!(size = GetMetaFileBitsEx( mf->hMF, 0, NULL )))
            {
                GlobalUnlock( handle );
                return 0;
            }
            *ret_size = offsetof( struct metafile_pict, bits[size] );
            if (!(mfbits = GlobalAlloc( GMEM_FIXED, *ret_size )))
            {
                GlobalUnlock( handle );
                return 0;
            }
            mfbits->mm   = mf->mm;
            mfbits->xExt = mf->xExt;
            mfbits->yExt = mf->yExt;
            GetMetaFileBitsEx( mf->hMF, size, mfbits->bits );
            GlobalUnlock( handle );
            return mfbits;
        }
    case CF_UNICODETEXT:
        {
            WCHAR *ptr;
            if (!(size = GlobalSize( handle ))) return 0;
            if ((data_size_t)size != size) return 0;
            if (!(ptr = GlobalLock( handle ))) return 0;
            ptr[(size + 1) / sizeof(WCHAR) - 1] = 0;  /* enforce null-termination */
            GlobalUnlock( handle );
            *ret_size = size;
            return handle;
        }
    case CF_TEXT:
    case CF_OEMTEXT:
        {
            char *ptr;
            if (!(size = GlobalSize( handle ))) return 0;
            if ((data_size_t)size != size) return 0;
            if (!(ptr = GlobalLock( handle ))) return 0;
            ptr[size - 1] = 0;  /* enforce null-termination */
            GlobalUnlock( handle );
            *ret_size = size;
            return handle;
        }
    default:
        if (!(size = GlobalSize( handle ))) return 0;
        if ((data_size_t)size != size) return 0;
        *ret_size = size;
        return handle;
    }
}

/* rebuild the target handle from the data received in GetClipboardData */
static HANDLE unmarshal_data( UINT format, void *data, data_size_t size )
{
    HANDLE handle = GlobalReAlloc( data, size, 0 );  /* release unused space */

    switch (format)
    {
    case CF_BITMAP:
        {
            BITMAP *bm = handle;
            if (size < sizeof(*bm)) break;
            if (size < bm->bmWidthBytes * abs( bm->bmHeight )) break;
            if (bm->bmBits) break;  /* DIB sections are not supported across processes */
            bm->bmBits = bm + 1;
            return CreateBitmapIndirect( bm );
        }
    case CF_DSPBITMAP:  /* not supported across processes */
        break;
    case CF_PALETTE:
        {
            LOGPALETTE *pal = handle;
            if (size < sizeof(*pal)) break;
            if (size < offsetof( LOGPALETTE, palPalEntry[pal->palNumEntries] )) break;
            return CreatePalette( pal );
        }
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        return SetEnhMetaFileBits( size, handle );
    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        {
            METAFILEPICT mf;
            struct metafile_pict *mfbits = handle;
            if (size <= sizeof(*mfbits)) break;
            mf.mm   = mfbits->mm;
            mf.xExt = mfbits->xExt;
            mf.yExt = mfbits->yExt;
            mf.hMF  = SetMetaFileBitsEx( size - offsetof( struct metafile_pict, bits ), mfbits->bits );
            *(METAFILEPICT *)handle = mf;
            return handle;
        }
    }
    return handle;
}

/* retrieve a data format from the cache */
static struct cached_format *get_cached_format( UINT format )
{
    struct cached_format *cache;

    LIST_FOR_EACH_ENTRY( cache, &cached_formats, struct cached_format, entry )
        if (cache->format == format) return cache;
    return NULL;
}

/* store data in the cache, or reuse the existing one if available */
static HANDLE cache_data( UINT format, HANDLE data, data_size_t size, UINT seqno,
                          struct cached_format *cache )
{
    if (cache)
    {
        if (seqno == cache->seqno)  /* we can reuse the cached data */
        {
            GlobalFree( data );
            return cache->handle;
        }
        /* cache entry is stale, remove it */
        list_remove( &cache->entry );
        list_add_tail( &formats_to_free, &cache->entry );
    }

    /* allocate new cache entry */
    if (!(cache = HeapAlloc( GetProcessHeap(), 0, sizeof(*cache) )))
    {
        GlobalFree( data );
        return 0;
    }
    cache->format = format;
    cache->seqno  = seqno;
    cache->handle = unmarshal_data( format, data, size );
    list_add_tail( &cached_formats, &cache->entry );
    return cache->handle;
}

/* free a single cached format */
static void free_cached_data( struct cached_format *cache )
{
    void *ptr;

    switch (cache->format)
    {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        DeleteObject( cache->handle );
        break;
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        DeleteEnhMetaFile( cache->handle );
        break;
    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        if ((ptr = GlobalLock( cache->handle )))
        {
            DeleteMetaFile( ((METAFILEPICT *)ptr)->hMF );
            GlobalUnlock( cache->handle );
        }
        GlobalFree( cache->handle );
        break;
    default:
        GlobalFree( cache->handle );
        break;
    }
    list_remove( &cache->entry );
    HeapFree( GetProcessHeap(), 0, cache );
}

/* clear global memory formats; special types are freed on EmptyClipboard */
static void invalidate_memory_formats(void)
{
    struct cached_format *cache, *next;

    LIST_FOR_EACH_ENTRY_SAFE( cache, next, &cached_formats, struct cached_format, entry )
    {
        switch (cache->format)
        {
        case CF_BITMAP:
        case CF_DSPBITMAP:
        case CF_PALETTE:
        case CF_ENHMETAFILE:
        case CF_DSPENHMETAFILE:
        case CF_METAFILEPICT:
        case CF_DSPMETAFILEPICT:
            continue;
        default:
            free_cached_data( cache );
            break;
        }
    }
}

/* free all the data in the cache */
static void free_cached_formats(void)
{
    struct list *ptr;

    list_move_tail( &formats_to_free, &cached_formats );
    while ((ptr = list_head( &formats_to_free )))
        free_cached_data( LIST_ENTRY( ptr, struct cached_format, entry ));
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

    if ((len = WideCharToMultiByte( codepage, 0, src, size / sizeof(WCHAR), NULL, 0, NULL, NULL )))
    {
        if ((ret = GlobalAlloc( GMEM_FIXED, len )))
            WideCharToMultiByte( codepage, 0, src, size / sizeof(WCHAR), ret, len, NULL, NULL );
    }

done:
    HeapFree( GetProcessHeap(), 0, srcW );
    GlobalUnlock( data );
    return ret;
}

/* render synthesized Unicode text based on the contents of the 'from' format */
static HANDLE render_synthesized_textW( HANDLE data, UINT from )
{
    char *src;
    HANDLE ret = 0;
    UINT len, size = GlobalSize( data );
    UINT codepage = get_format_codepage( get_clipboard_locale(), from );

    if (!(src = GlobalLock( data ))) return 0;

    if ((len = MultiByteToWideChar( codepage, 0, src, size, NULL, 0 )))
    {
        if ((ret = GlobalAlloc( GMEM_FIXED, len * sizeof(WCHAR) )))
            MultiByteToWideChar( codepage, 0, src, size, ret, len );
    }
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
    NtUserReleaseDC( 0, hdc );
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
        BITMAPV5HEADER header;

        memset( &header, 0, sizeof(header) );
        header.bV5Size = (format == CF_DIBV5) ? sizeof(BITMAPV5HEADER) : sizeof(BITMAPINFOHEADER);
        if (!GetDIBits( hdc, data, 0, 0, NULL, (BITMAPINFO *)&header, DIB_RGB_COLORS )) goto done;

        header_size = bitmap_info_size( (BITMAPINFO *)&header, DIB_RGB_COLORS );
        if (!(ret = GlobalAlloc( GMEM_FIXED, header_size + header.bV5SizeImage ))) goto done;
        bmi = (BITMAPINFO *)ret;
        memset( bmi, 0, header_size );
        memcpy( bmi, &header, header.bV5Size );
        GetDIBits( hdc, data, 0, abs(header.bV5Height), (char *)bmi + header_size, bmi, DIB_RGB_COLORS );
    }
    else
    {
        SIZE_T size = GlobalSize( data );

        if (size < sizeof(*bmi)) goto done;
        if (!(src = GlobalLock( data ))) goto done;

        src_size = bitmap_info_size( src, DIB_RGB_COLORS );
        if (size <= src_size)
        {
            GlobalUnlock( data );
            goto done;
        }
        bits_size = size - src_size;
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
    NtUserReleaseDC( 0, hdc );
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
    NtUserReleaseDC( 0, hdc );
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
    HWND owner;

    TRACE( "%p\n", hwnd );

    NtUserCallNoParam( NtUserUpdateClipboard );

    EnterCriticalSection( &clipboard_cs );

    SERVER_START_REQ( open_clipboard )
    {
        req->window = wine_server_user_handle( hwnd );
        ret = !wine_server_call_err( req );
        owner = wine_server_ptr_handle( reply->owner );
    }
    SERVER_END_REQ;

    if (ret && !WIN_IsCurrentProcess( owner )) invalidate_memory_formats();

    LeaveCriticalSection( &clipboard_cs );
    return ret;
}


/**************************************************************************
 *		EmptyClipboard (USER32.@)
 * Empties and acquires ownership of the clipboard
 */
BOOL WINAPI EmptyClipboard(void)
{
    BOOL ret;
    HWND owner = NtUserGetClipboardOwner();

    TRACE( "owner %p\n", owner );

    if (owner) SendMessageTimeoutW( owner, WM_DESTROYCLIPBOARD, 0, 0, SMTO_ABORTIFHUNG, 5000, NULL );

    EnterCriticalSection( &clipboard_cs );

    SERVER_START_REQ( empty_clipboard )
    {
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    if (ret) free_cached_formats();

    LeaveCriticalSection( &clipboard_cs );
    return ret;
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
    struct cached_format *cache = NULL;
    void *ptr = NULL;
    data_size_t size = 0;
    HANDLE handle = data, retval = 0;
    NTSTATUS status = STATUS_SUCCESS;

    TRACE( "%s %p\n", debugstr_format( format ), data );

    if (data)
    {
        if (!(handle = marshal_data( format, data, &size ))) return 0;
        if (!(ptr = GlobalLock( handle ))) goto done;
        if (!(cache = HeapAlloc( GetProcessHeap(), 0, sizeof(*cache) ))) goto done;
        cache->format = format;
        cache->handle = data;
    }

    EnterCriticalSection( &clipboard_cs );

    SERVER_START_REQ( set_clipboard_data )
    {
        req->format = format;
        req->lcid = GetUserDefaultLCID();
        wine_server_add_data( req, ptr, size );
        if (!(status = wine_server_call( req )))
        {
            if (cache) cache->seqno = reply->seqno;
        }
    }
    SERVER_END_REQ;

    if (!status)
    {
        /* free the previous entry if any */
        struct cached_format *prev;

        if ((prev = get_cached_format( format ))) free_cached_data( prev );
        if (cache) list_add_tail( &cached_formats, &cache->entry );
        retval = data;
    }
    else HeapFree( GetProcessHeap(), 0, cache );

    LeaveCriticalSection( &clipboard_cs );

done:
    if (ptr) GlobalUnlock( handle );
    if (handle != data) GlobalFree( handle );
    if (status) SetLastError( RtlNtStatusToDosError( status ));
    return retval;
}


/**************************************************************************
 *		EnumClipboardFormats (USER32.@)
 */
UINT WINAPI EnumClipboardFormats( UINT format )
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
 *		GetClipboardData (USER32.@)
 */
HANDLE WINAPI GetClipboardData( UINT format )
{
    struct cached_format *cache;
    NTSTATUS status;
    UINT from, data_seqno;
    HWND owner;
    HANDLE data;
    UINT size = 1024;
    BOOL render = TRUE;

    for (;;)
    {
        if (!(data = GlobalAlloc( GMEM_FIXED, size ))) return 0;

        EnterCriticalSection( &clipboard_cs );
        cache = get_cached_format( format );

        SERVER_START_REQ( get_clipboard_data )
        {
            req->format = format;
            req->render = render;
            if (cache)
            {
                req->cached = 1;
                req->seqno = cache->seqno;
            }
            wine_server_set_reply( req, data, size );
            status = wine_server_call( req );
            from = reply->from;
            size = reply->total;
            data_seqno = reply->seqno;
            owner = wine_server_ptr_handle( reply->owner );
        }
        SERVER_END_REQ;

        if (!status && size)
        {
            data = cache_data( format, data, size, data_seqno, cache );
            LeaveCriticalSection( &clipboard_cs );
            TRACE( "%s returning %p\n", debugstr_format( format ), data );
            return data;
        }
        LeaveCriticalSection( &clipboard_cs );
        GlobalFree( data );

        if (status == STATUS_BUFFER_OVERFLOW) continue;  /* retry with the new size */
        if (status == STATUS_OBJECT_NAME_NOT_FOUND) return 0; /* no such format */
        if (status)
        {
            SetLastError( RtlNtStatusToDosError( status ));
            TRACE( "%s error %08x\n", debugstr_format( format ), status );
            return 0;
        }
        if (render)  /* try rendering it */
        {
            render = FALSE;
            if (from)
            {
                render_synthesized_format( format, from );
                continue;
            }
            else if (owner)
            {
                TRACE( "%s sending WM_RENDERFORMAT to %p\n", debugstr_format( format ), owner );
                SendMessageW( owner, WM_RENDERFORMAT, format, 0 );
                continue;
            }
        }
        TRACE( "%s returning 0\n", debugstr_format( format ));
        return 0;
    }
}
