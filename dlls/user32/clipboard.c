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
#include "user_private.h"
#include "winnls.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);


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
static HANDLE marshal_data( UINT format, HANDLE handle, size_t *ret_size )
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
            char *ptr;
            if (!(size = GlobalSize( handle ))) return 0;
            if ((data_size_t)size != size) return 0;
            if (size < sizeof(WCHAR)) return 0;
            if (!(ptr = GlobalLock( handle ))) return 0;
            /* enforce nul-termination the Windows way: ignoring alignment */
            *((WCHAR *)(ptr + size) - 1) = 0;
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
    HANDLE handle = GlobalReAlloc( data, size, GMEM_MOVEABLE );  /* release unused space */

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

/* free a single cached format */
void free_cached_data( UINT format, HANDLE handle )
{
    void *ptr;

    switch (format)
    {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        DeleteObject( handle );
        break;
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        DeleteEnhMetaFile( handle );
        break;
    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        if ((ptr = GlobalLock( handle )))
        {
            DeleteMetaFile( ((METAFILEPICT *)ptr)->hMF );
            GlobalUnlock( handle );
        }
        GlobalFree( handle );
        break;
    default:
        GlobalFree( handle );
        break;
    }
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
    HDC hdc = NtUserGetDC( 0 );

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
    HDC hdc = NtUserGetDC( 0 );

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
    HDC hdc = NtUserGetDC( 0 );

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
HANDLE render_synthesized_format( UINT format, UINT from )
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
    return NtUserOpenClipboard( hwnd, 0 );
}


/**************************************************************************
 *		SetClipboardData (USER32.@)
 */
HANDLE WINAPI SetClipboardData( UINT format, HANDLE data )
{
    struct set_clipboard_params params = { .size = 0 };
    HANDLE handle = data;
    NTSTATUS status;

    TRACE( "%s %p\n", debugstr_format( format ), data );

    if (data)
    {
        if (!(handle = marshal_data( format, data, &params.size ))) return 0;
        if (!(params.data = GlobalLock( handle ))) return 0;
    }

    status = NtUserSetClipboardData( format, data, &params );

    if (params.data) GlobalUnlock( handle );
    if (handle != data) GlobalFree( handle );
    if (status)
    {
        SetLastError( RtlNtStatusToDosError( status ));
        return 0;
    }
    return data;
}


/**************************************************************************
 *		EnumClipboardFormats (USER32.@)
 */
UINT WINAPI EnumClipboardFormats( UINT format )
{
    return NtUserEnumClipboardFormats( format );
}


/**************************************************************************
 *		GetClipboardData (USER32.@)
 */
HANDLE WINAPI GetClipboardData( UINT format )
{
    struct get_clipboard_params params = { .data_size = 1024 };
    HANDLE ret = 0;

    EnterCriticalSection( &clipboard_cs );

    while (params.data_size)
    {
        params.size = params.data_size;
        params.data_size = 0;
        if (!(params.data = GlobalAlloc( GMEM_FIXED, params.size ))) break;
        ret = NtUserGetClipboardData( format, &params );
        if (ret) break;
        if (params.data_size == ~0) /* needs unmarshaling */
        {
            struct set_clipboard_params set_params = { .cache_only = TRUE, .seqno = params.seqno };

            ret = unmarshal_data( format, params.data, params.size );
            if (!NtUserSetClipboardData( format, ret, &set_params )) break;

            /* data changed, retry */
            free_cached_data( format, ret );
            ret = 0;
            params.data_size = 1024;
            continue;
        }
        GlobalFree( params.data );
    }

    LeaveCriticalSection( &clipboard_cs );
    return ret;
}
