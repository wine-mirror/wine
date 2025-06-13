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

#define COBJMACROS
#include <assert.h>
#include "user_private.h"
#include "winnls.h"
#include "objidl.h"
#include "shlobj.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

#define MAX_ATOM_LEN 255

static CRITICAL_SECTION clipboard_cs;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &clipboard_cs,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": clipboard_cs") }
};
static CRITICAL_SECTION clipboard_cs = { &critsect_debug, -1, 0, 0, 0, 0 };
static IDataObject *dnd_data_object;


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
            if ((UINT)size != size) return 0;
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
            if ((UINT)size != size) return 0;
            if (!(ptr = GlobalLock( handle ))) return 0;
            ptr[size - 1] = 0;  /* enforce null-termination */
            GlobalUnlock( handle );
            *ret_size = size;
            return handle;
        }
    default:
        if (!(size = GlobalSize( handle ))) return 0;
        if ((UINT)size != size) return 0;
        *ret_size = size;
        return handle;
    }
}

/* rebuild the target handle from the data received in GetClipboardData */
static HANDLE unmarshal_data( UINT format, void *data, UINT size )
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
    UNICODE_STRING str;

    TRACE( "%s\n", debugstr_w(name) );

    RtlInitUnicodeString( &str, name );
    return NtUserRegisterWindowMessage( &str );
}


/**************************************************************************
 *		RegisterClipboardFormatA (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatA( LPCSTR name )
{
    WCHAR buf[MAX_ATOM_LEN + 1];
    UNICODE_STRING str = {.Buffer = buf, .MaximumLength = sizeof(buf)};
    STRING ansi;

    TRACE( "%s\n", debugstr_a(name) );

    RtlInitAnsiString( &ansi, name );
    RtlAnsiStringToUnicodeString( &str, &ansi, FALSE );
    return NtUserRegisterWindowMessage( &str );
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

static struct format_entry *next_format( struct format_entry *entry )
{
    return (struct format_entry *)&entry->data[(entry->size + 7) & ~7];
}

struct data_object
{
    IDataObject IDataObject_iface;
    LONG refcount;

    HWND target_hwnd; /* the last window the mouse was over */
    POINT target_pos;
    DWORD target_effect;
    IDropTarget *drop_target;

    struct format_entry *entries_end;
    struct format_entry entries[];
};

static struct data_object *data_object_from_IDataObject( IDataObject *iface )
{
    return CONTAINING_RECORD( iface, struct data_object, IDataObject_iface );
}

struct format_iterator
{
    IEnumFORMATETC IEnumFORMATETC_iface;
    LONG refcount;

    struct format_entry *entry;
    IDataObject *object;
};

static inline struct format_iterator *format_iterator_from_IEnumFORMATETC( IEnumFORMATETC *iface )
{
    return CONTAINING_RECORD(iface, struct format_iterator, IEnumFORMATETC_iface);
}

static HRESULT WINAPI format_iterator_QueryInterface( IEnumFORMATETC *iface, REFIID iid, void **obj )
{
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );

    TRACE( "iterator %p, iid %s, obj %p\n", iterator, debugstr_guid(iid), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) || IsEqualIID( iid, &IID_IEnumFORMATETC ))
    {
        IEnumFORMATETC_AddRef( &iterator->IEnumFORMATETC_iface );
        *obj = &iterator->IEnumFORMATETC_iface;
        return S_OK;
    }

    *obj = NULL;
    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid) );
    return E_NOINTERFACE;
}

static ULONG WINAPI format_iterator_AddRef( IEnumFORMATETC *iface )
{
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );
    ULONG ref = InterlockedIncrement( &iterator->refcount );
    TRACE( "iterator %p increasing refcount to %lu.\n", iterator, ref );
    return ref;
}

static ULONG WINAPI format_iterator_Release(IEnumFORMATETC *iface)
{
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );
    ULONG ref = InterlockedDecrement( &iterator->refcount );

    TRACE( "iterator %p increasing refcount to %lu.\n", iterator, ref );

    if (!ref)
    {
        IDataObject_Release( iterator->object );
        free( iterator );
    }

    return ref;
}

static HRESULT WINAPI format_iterator_Next( IEnumFORMATETC *iface, ULONG count, FORMATETC *formats, ULONG *ret )
{
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );
    struct data_object *object = data_object_from_IDataObject( iterator->object );
    struct format_entry *entry;
    UINT i;

    TRACE( "iterator %p, count %lu, formats %p, ret %p\n", iterator, count, formats, ret );

    for (entry = iterator->entry, i = 0; entry < object->entries_end && i < count; entry = next_format( entry ), i++)
    {
        formats[i].cfFormat = entry->format;
        formats[i].ptd = NULL;
        formats[i].dwAspect = DVASPECT_CONTENT;
        formats[i].lindex = -1;
        formats[i].tymed = TYMED_HGLOBAL;
    }

    iterator->entry = entry;
    if (ret) *ret = i;
    return (i == count) ? S_OK : S_FALSE;
}

static HRESULT WINAPI format_iterator_Skip( IEnumFORMATETC *iface, ULONG count )
{
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );
    struct data_object *object = data_object_from_IDataObject( iterator->object );
    struct format_entry *entry;

    TRACE( "iterator %p, count %lu\n", iterator, count );

    for (entry = iterator->entry; entry < object->entries_end; entry = next_format( entry ))
        if (!count--) break;

    iterator->entry = entry;
    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI format_iterator_Reset( IEnumFORMATETC *iface )
{
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );
    struct data_object *object = data_object_from_IDataObject( iterator->object );

    TRACE( "iterator %p\n", iterator );
    iterator->entry = object->entries;
    return S_OK;
}

static HRESULT format_iterator_create( IDataObject *object, IEnumFORMATETC **out );

static HRESULT WINAPI format_iterator_Clone( IEnumFORMATETC *iface, IEnumFORMATETC **out )
{
    HRESULT hr;
    struct format_iterator *iterator = format_iterator_from_IEnumFORMATETC( iface );
    TRACE( "iterator %p, out %p\n", iterator, out );
    hr = format_iterator_create( iterator->object, out );
    if (SUCCEEDED(hr))
        format_iterator_from_IEnumFORMATETC( *out )->entry = iterator->entry;
    return hr;
}

static const IEnumFORMATETCVtbl format_iterator_vtbl =
{
    format_iterator_QueryInterface,
    format_iterator_AddRef,
    format_iterator_Release,
    format_iterator_Next,
    format_iterator_Skip,
    format_iterator_Reset,
    format_iterator_Clone,
};

static HRESULT format_iterator_create( IDataObject *object, IEnumFORMATETC **out )
{
    struct format_iterator *iterator;

    if (!(iterator = calloc( 1, sizeof(*iterator) ))) return E_OUTOFMEMORY;
    iterator->IEnumFORMATETC_iface.lpVtbl = &format_iterator_vtbl;
    iterator->refcount = 1;
    IDataObject_AddRef( (iterator->object = object) );
    iterator->entry = data_object_from_IDataObject(object)->entries;

    *out = &iterator->IEnumFORMATETC_iface;
    TRACE( "created object %p iterator %p\n", object, iterator );
    return S_OK;
}

static HRESULT WINAPI data_object_QueryInterface( IDataObject *iface, REFIID iid, void **obj )
{
    struct data_object *object = data_object_from_IDataObject( iface );

    TRACE( "object %p, iid %s, obj %p\n", object, debugstr_guid(iid), obj );

    if (IsEqualIID( iid, &IID_IUnknown ) || IsEqualIID( iid, &IID_IDataObject ))
    {
        IDataObject_AddRef( &object->IDataObject_iface );
        *obj = &object->IDataObject_iface;
        return S_OK;
    }

    *obj = NULL;
    WARN( "%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid) );
    return E_NOINTERFACE;
}

static ULONG WINAPI data_object_AddRef( IDataObject *iface )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    ULONG ref = InterlockedIncrement( &object->refcount );
    TRACE( "object %p increasing refcount to %lu.\n", object, ref );
    return ref;
}

static ULONG WINAPI data_object_Release( IDataObject *iface )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    ULONG ref = InterlockedDecrement( &object->refcount );

    TRACE( "object %p decreasing refcount to %lu.\n", object, ref );

    if (!ref)
    {
        if (object->drop_target)
        {
            HRESULT hr = IDropTarget_DragLeave( object->drop_target );
            if (FAILED(hr)) WARN( "IDropTarget_DragLeave returned %#lx\n", hr );
            IDropTarget_Release( object->drop_target );
        }

        free( object );
    }

    return ref;
}

static HRESULT WINAPI data_object_GetData( IDataObject *iface, FORMATETC *format, STGMEDIUM *medium )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    struct format_entry *iter;
    HRESULT hr;

    TRACE( "object %p, format %p (%s), medium %p\n", object, format, debugstr_format(format->cfFormat), medium );

    if (FAILED(hr = IDataObject_QueryGetData( iface, format ))) return hr;

    for (iter = object->entries; iter < object->entries_end; iter = next_format( iter ))
    {
        if (iter->format == format->cfFormat)
        {
            medium->tymed = TYMED_HGLOBAL;
            medium->hGlobal = GlobalAlloc( GMEM_FIXED | GMEM_ZEROINIT, iter->size );
            if (medium->hGlobal == NULL) return E_OUTOFMEMORY;
            memcpy( GlobalLock( medium->hGlobal ), iter->data, iter->size );
            GlobalUnlock( medium->hGlobal );
            medium->pUnkForRelease = 0;
            return S_OK;
        }
    }

    return DATA_E_FORMATETC;
}

static HRESULT WINAPI data_object_GetDataHere( IDataObject *iface, FORMATETC *format, STGMEDIUM *medium )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, medium %p stub!\n", object, format, medium );
    return DATA_E_FORMATETC;
}

static HRESULT WINAPI data_object_QueryGetData( IDataObject *iface, FORMATETC *format )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    struct format_entry *iter;

    TRACE( "object %p, format %p (%s)\n", object, format, debugstr_format(format->cfFormat) );

    if (format->tymed && !(format->tymed & TYMED_HGLOBAL))
    {
        FIXME("only HGLOBAL medium types supported right now\n");
        return DV_E_TYMED;
    }
    /* Windows Explorer ignores .dwAspect and .lindex for CF_HDROP,
     * and we have no way to implement them on XDnD anyway, so ignore them too.
     */

    for (iter = object->entries; iter < object->entries_end; iter = next_format( iter ))
    {
        if (iter->format == format->cfFormat)
        {
            TRACE("application found %s\n", debugstr_format(format->cfFormat));
            return S_OK;
        }
    }
    TRACE("application didn't find %s\n", debugstr_format(format->cfFormat));
    return DV_E_FORMATETC;
}

static HRESULT WINAPI data_object_GetCanonicalFormatEtc( IDataObject *iface, FORMATETC *format, FORMATETC *out )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, out %p stub!\n", object, format, out );
    out->ptd = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI data_object_SetData( IDataObject *iface, FORMATETC *format, STGMEDIUM *medium, BOOL release )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, medium %p, release %u stub!\n", object, format, medium, release );
    return E_NOTIMPL;
}

static HRESULT WINAPI data_object_EnumFormatEtc( IDataObject *iface, DWORD direction, IEnumFORMATETC **out )
{
    struct data_object *object = data_object_from_IDataObject( iface );

    TRACE( "object %p, direction %lu, out %p\n", object, direction, out );

    if (direction != DATADIR_GET)
    {
        FIXME("only the get direction is implemented\n");
        return E_NOTIMPL;
    }

    return format_iterator_create( iface, out );
}

static HRESULT WINAPI data_object_DAdvise( IDataObject *iface, FORMATETC *format, DWORD flags,
                                           IAdviseSink *sink, DWORD *connection )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, format %p, flags %#lx, sink %p, connection %p stub!\n",
           object, format, flags, sink, connection );
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI data_object_DUnadvise( IDataObject *iface, DWORD connection )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, connection %lu stub!\n", object, connection );
    return OLE_E_ADVISENOTSUPPORTED;
}

static HRESULT WINAPI data_object_EnumDAdvise( IDataObject *iface, IEnumSTATDATA **advise )
{
    struct data_object *object = data_object_from_IDataObject( iface );
    FIXME( "object %p, advise %p stub!\n", object, advise );
    return OLE_E_ADVISENOTSUPPORTED;
}

static IDataObjectVtbl data_object_vtbl =
{
    data_object_QueryInterface,
    data_object_AddRef,
    data_object_Release,
    data_object_GetData,
    data_object_GetDataHere,
    data_object_QueryGetData,
    data_object_GetCanonicalFormatEtc,
    data_object_SetData,
    data_object_EnumFormatEtc,
    data_object_DAdvise,
    data_object_DUnadvise,
    data_object_EnumDAdvise,
};

static HRESULT data_object_create( UINT entries_size, const struct format_entry *entries, IDataObject **out )
{
    struct data_object *object;

    if (!(object = calloc( 1, sizeof(*object) + entries_size ))) return E_OUTOFMEMORY;
    object->IDataObject_iface.lpVtbl = &data_object_vtbl;
    object->refcount = 1;

    object->entries_end = (struct format_entry *)((char *)object->entries + entries_size);
    memcpy( object->entries, entries, entries_size );
    *out = &object->IDataObject_iface;

    return S_OK;
}

static struct data_object *get_data_object( BOOL clear )
{
    IDataObject *iface;

    EnterCriticalSection( &clipboard_cs );
    if ((iface = dnd_data_object))
    {
        if (clear) dnd_data_object = NULL;
        else IDataObject_AddRef( iface );
    }
    LeaveCriticalSection( &clipboard_cs );

    if (!iface) return NULL;
    return data_object_from_IDataObject( iface );
}

/* Based on functions in dlls/ole32/ole2.c */
static HANDLE get_droptarget_local_handle(HWND hwnd)
{
    static const WCHAR prop_marshalleddroptarget[] =
        {'W','i','n','e','M','a','r','s','h','a','l','l','e','d','D','r','o','p','T','a','r','g','e','t',0};
    HANDLE handle;
    HANDLE local_handle = 0;

    handle = GetPropW(hwnd, prop_marshalleddroptarget);
    if (handle)
    {
        DWORD pid;
        HANDLE process;

        GetWindowThreadProcessId(hwnd, &pid);
        process = OpenProcess(PROCESS_DUP_HANDLE, FALSE, pid);
        if (process)
        {
            DuplicateHandle(process, handle, GetCurrentProcess(), &local_handle, 0, FALSE, DUPLICATE_SAME_ACCESS);
            CloseHandle(process);
        }
    }
    return local_handle;
}

static HRESULT create_stream_from_map(HANDLE map, IStream **stream)
{
    HRESULT hr = E_OUTOFMEMORY;
    HGLOBAL hmem;
    void *data;
    MEMORY_BASIC_INFORMATION info;

    data = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    if(!data) return hr;

    VirtualQuery(data, &info, sizeof(info));
    TRACE("size %d\n", (int)info.RegionSize);

    hmem = GlobalAlloc(GMEM_MOVEABLE, info.RegionSize);
    if(hmem)
    {
        memcpy(GlobalLock(hmem), data, info.RegionSize);
        GlobalUnlock(hmem);
        hr = CreateStreamOnHGlobal(hmem, TRUE, stream);
    }
    UnmapViewOfFile(data);
    return hr;
}

static IDropTarget* get_droptarget_pointer(HWND hwnd)
{
    IDropTarget *droptarget = NULL;
    HANDLE map;
    IStream *stream;

    map = get_droptarget_local_handle(hwnd);
    if(!map) return NULL;

    if(SUCCEEDED(create_stream_from_map(map, &stream)))
    {
        CoUnmarshalInterface(stream, &IID_IDropTarget, (void**)&droptarget);
        IStream_Release(stream);
    }
    CloseHandle(map);
    return droptarget;
}

/* Recursively searches for a window on given coordinates in a drag&drop specific manner.
 *
 * Don't use WindowFromPoint instead, because it omits the STATIC and transparent
 * windows, but they can be a valid drop targets if have WS_EX_ACCEPTFILES set.
 */
static HWND window_from_point_dnd( HWND hwnd, POINT point )
{
    HWND child;

    ScreenToClient( hwnd, &point );

    while ((child = ChildWindowFromPointEx( hwnd, point, CWP_SKIPDISABLED | CWP_SKIPINVISIBLE )) && child != hwnd)
    {
       MapWindowPoints( hwnd, child, &point, 1 );
       hwnd = child;
    }

    return hwnd;
}

/* Returns the first window down the hierarchy that has WS_EX_ACCEPTFILES set or
 * returns NULL, if such window does not exists.
 */
static HWND window_accepting_files( HWND hwnd )
{
    /* MUST to be GetParent, not GetAncestor, because the owner window (with WS_EX_ACCEPTFILES)
     * of a window with WS_POPUP is a valid drop target. GetParent works exactly this way! */
    while (hwnd && !(GetWindowLongW( hwnd, GWL_EXSTYLE ) & WS_EX_ACCEPTFILES)) hwnd = GetParent( hwnd );
    return hwnd;
}

BOOL drag_drop_enter( UINT entries_size, const struct format_entry *entries )
{
    IDataObject *object, *previous;

    TRACE( "entries_size %u, entries %p\n", entries_size, entries );

    if (FAILED(data_object_create( entries_size, entries, &object ))) return FALSE;

    EnterCriticalSection( &clipboard_cs );
    previous = dnd_data_object;
    dnd_data_object = object;
    LeaveCriticalSection( &clipboard_cs );

    if (previous) IDataObject_Release( previous );
    return TRUE;
}

void drag_drop_leave(void)
{
    struct data_object *object;

    TRACE("DND Operation canceled\n");

    if ((object = get_data_object( TRUE ))) IDataObject_Release( &object->IDataObject_iface );
}

DWORD drag_drop_drag( HWND hwnd, POINT point, DWORD effect )
{
    int accept = 0; /* Assume we're not accepting */
    POINTL pointl = {.x = point.x, .y = point.y};
    struct data_object *object;
    HRESULT hr;

    TRACE( "hwnd %p, point %s, effect %#lx\n", hwnd, wine_dbgstr_point(&point), effect );

    if (!(object = get_data_object( FALSE ))) return DROPEFFECT_NONE;
    object->target_pos = point;
    hwnd = window_from_point_dnd( hwnd, object->target_pos );

    if (!object->drop_target || object->target_hwnd != hwnd)
    {
        /* Notify OLE of DragEnter. Result determines if we accept */
        HWND dropTargetWindow;

        if (object->drop_target)
        {
            hr = IDropTarget_DragLeave( object->drop_target );
            if (FAILED(hr)) WARN( "IDropTarget_DragLeave returned %#lx\n", hr );
            IDropTarget_Release( object->drop_target );
            object->drop_target = NULL;
        }

        dropTargetWindow = hwnd;
        do { object->drop_target = get_droptarget_pointer( dropTargetWindow ); }
        while (!object->drop_target && !!(dropTargetWindow = GetParent( dropTargetWindow )));
        object->target_hwnd = hwnd;

        if (object->drop_target)
        {
            DWORD effect_ignore = effect;
            hr = IDropTarget_DragEnter( object->drop_target, &object->IDataObject_iface,
                                        MK_LBUTTON, pointl, &effect_ignore );
            if (hr == S_OK) TRACE( "the application accepted the drop (effect = %ld)\n", effect_ignore );
            else
            {
                WARN( "IDropTarget_DragEnter returned %#lx\n", hr );
                IDropTarget_Release( object->drop_target );
                object->drop_target = NULL;
            }
        }
    }
    else if (object->drop_target)
    {
        hr = IDropTarget_DragOver( object->drop_target, MK_LBUTTON, pointl, &effect );
        if (hr == S_OK) object->target_effect = effect;
        else WARN( "IDropTarget_DragOver returned %#lx\n", hr );
    }

    if (object->drop_target && object->target_effect != DROPEFFECT_NONE)
        accept = 1;
    else
    {
        /* fallback search for window able to accept these files. */
        FORMATETC format = {.cfFormat = CF_HDROP};

        if (window_accepting_files(hwnd) && SUCCEEDED(IDataObject_QueryGetData( &object->IDataObject_iface, &format )))
        {
            accept = 1;
            effect = DROPEFFECT_COPY;
        }
    }

    if (!accept) effect = DROPEFFECT_NONE;
    IDataObject_Release( &object->IDataObject_iface );

    return effect;
}

DWORD drag_drop_drop( HWND hwnd )
{
    DWORD effect;
    int accept = 0; /* Assume we're not accepting */
    struct data_object *object;
    BOOL drop_file = TRUE;

    TRACE( "hwnd %p\n", hwnd );

    if (!(object = get_data_object( TRUE ))) return DROPEFFECT_NONE;
    effect = object->target_effect;

    /* Notify OLE of Drop */
    if (object->drop_target && effect != DROPEFFECT_NONE)
    {
        POINTL pointl = {object->target_pos.x, object->target_pos.y};
        HRESULT hr;

        hr = IDropTarget_Drop( object->drop_target, &object->IDataObject_iface,
                               MK_LBUTTON, pointl, &effect );
        if (hr == S_OK)
        {
            if (effect != DROPEFFECT_NONE)
            {
                TRACE("drop succeeded\n");
                accept = 1;
                drop_file = FALSE;
            }
            else
                TRACE("the application refused the drop\n");
        }
        else if (FAILED(hr))
            WARN("drop failed, error 0x%08lx\n", hr);
        else
        {
            WARN("drop returned 0x%08lx\n", hr);
            drop_file = FALSE;
        }
    }
    else if (object->drop_target)
    {
        HRESULT hr = IDropTarget_DragLeave( object->drop_target );
        if (FAILED(hr)) WARN( "IDropTarget_DragLeave returned %#lx\n", hr );
        IDropTarget_Release( object->drop_target );
        object->drop_target = NULL;
    }

    if (drop_file)
    {
        /* Only send WM_DROPFILES if Drop didn't succeed or DROPEFFECT_NONE was set.
         * Doing both causes winamp to duplicate the dropped files (#29081) */
        HWND hwnd_drop = window_accepting_files(window_from_point_dnd( hwnd, object->target_pos ));
        FORMATETC format = {.cfFormat = CF_HDROP};
        STGMEDIUM medium;

        if (hwnd_drop && SUCCEEDED(IDataObject_GetData( &object->IDataObject_iface, &format, &medium )))
        {
            DROPFILES *drop = GlobalLock( medium.hGlobal );
            void *files = (char *)drop + drop->pFiles;
            RECT rect;

            drop->pt = object->target_pos;
            drop->fNC = !ScreenToClient( hwnd, &drop->pt ) || !GetClientRect( hwnd, &rect ) || !PtInRect( &rect, drop->pt );
            TRACE( "Sending WM_DROPFILES: hwnd %p, pt %s, fNC %u, files %p (%s)\n", hwnd,
                   wine_dbgstr_point( &drop->pt), drop->fNC, files, debugstr_w(files) );
            GlobalUnlock( medium.hGlobal );

            PostMessageW( hwnd, WM_DROPFILES, (WPARAM)medium.hGlobal, 0 );
            accept = 1;
            effect = DROPEFFECT_COPY;
        }
    }

    TRACE("effectRequested(0x%lx) accept(%d) performed(0x%lx) at x(%ld),y(%ld)\n",
          object->target_effect, accept, effect, object->target_pos.x, object->target_pos.y);

    if (!accept) effect = DROPEFFECT_NONE;
    IDataObject_Release( &object->IDataObject_iface );

    return effect;
}

void drag_drop_post( HWND hwnd, UINT drop_size, const DROPFILES *drop )
{
    POINT point = drop->pt;
    HDROP handle;

    hwnd = window_accepting_files( window_from_point_dnd( hwnd, point ) );
    if ((handle = GlobalAlloc( GMEM_SHARE, drop_size )))
    {
        DROPFILES *ptr = GlobalLock( handle );
        memcpy( ptr, drop, drop_size );
        GlobalUnlock( handle );
        PostMessageW( hwnd, WM_DROPFILES, (WPARAM)handle, 0 );
    }
}
