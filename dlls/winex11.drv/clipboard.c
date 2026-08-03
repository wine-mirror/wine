/*
 * X11 clipboard windows driver
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Alex Korobka
 * Copyright 1999 Noel Borthwick
 * Copyright 2003 Ulrich Czekalla for CodeWeavers
 * Copyright 2014 Damjan Jovanovic
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
 * NOTES:
 *    This file contains the X specific implementation for the windows
 *    Clipboard API.
 *
 *    Wine's internal clipboard is exposed to external apps via the X
 *    selection mechanism.
 *    Currently the driver asserts ownership via two selection atoms:
 *    1. PRIMARY(XA_PRIMARY)
 *    2. CLIPBOARD
 *
 *    In our implementation, the CLIPBOARD selection takes precedence over PRIMARY,
 *    i.e. if a CLIPBOARD selection is available, it is used instead of PRIMARY.
 *    When Wine takes ownership of the clipboard, it takes ownership of BOTH selections.
 *    While giving up selection ownership, if the CLIPBOARD selection is lost,
 *    it will lose both PRIMARY and CLIPBOARD and empty the clipboard.
 *    However if only PRIMARY is lost, it will continue to hold the CLIPBOARD selection
 *    (leaving the clipboard cache content unaffected).
 *
 *      Every format exposed via a windows clipboard format is also exposed through
 *    a corresponding X selection target. A selection target atom is synthesized
 *    whenever a new Windows clipboard format is registered via RegisterClipboardFormat,
 *    or when a built-in format is used for the first time.
 *    Windows native format are exposed by prefixing the format name with "<WCF>"
 *    This allows us to uniquely identify windows native formats exposed by other
 *    running WINE apps.
 *
 *      In order to allow external applications to query WINE for supported formats,
 *    we respond to the "TARGETS" selection target. (See EVENT_SelectionRequest
 *    for implementation) We use the same mechanism to query external clients for
 *    availability of a particular format, by caching the list of available targets
 *    by using the clipboard cache's "delayed render" mechanism. If a selection client
 *    does not support the "TARGETS" selection target, we actually attempt to retrieve
 *    the format requested as a fallback mechanism.
 *
 *      Certain Windows native formats are automatically converted to X native formats
 *    and vice versa. If a native format is available in the selection, it takes
 *    precedence, in order to avoid unnecessary conversions.
 *
 * FIXME: global format list needs a critical section
 */

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <limits.h>
#include <time.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "x11drv.h"

#ifdef HAVE_X11_EXTENSIONS_XFIXES_H
#include <X11/extensions/Xfixes.h>
#endif

#include "shlobj.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

/* Maximum wait time for selection notify */
#define SELECTION_RETRIES 500  /* wait for .5 seconds */
#define SELECTION_WAIT    1    /* ms */

#define SELECTION_UPDATE_DELAY 2000   /* delay between checks of the X11 selection */

typedef BOOL (*EXPORTFUNC)( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
typedef void *(*IMPORTFUNC)( Atom type, const void *data, size_t size, size_t *ret_size );

struct clipboard_format
{
    struct list entry;
    UINT        id;
    Atom        atom;
    IMPORTFUNC  import;
    EXPORTFUNC  export;
};

static void *import_data( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_pixmap( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_image_bmp( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_string( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_utf8_string( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_compound_text( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_text( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_text_html( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_text_uri_list( Atom type, const void *data, size_t size, size_t *ret_size );
static void *import_targets( Atom type, const void *data, size_t size, size_t *ret_size );

static BOOL export_data( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_string( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_utf8_string( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_text( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_compound_text( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_pixmap( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_image_bmp( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_text_html( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_hdrop( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_targets( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_multiple( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );
static BOOL export_timestamp( Display *display, Window win, Atom prop, Atom target, void *data, size_t size );

static BOOL read_property( Display *display, Window w, Atom prop,
                           Atom *type, unsigned char **data, size_t *datasize );

/* Clipboard formats */

static const WCHAR RichTextFormatW[] = {'R','i','c','h',' ','T','e','x','t',' ','F','o','r','m','a','t',0};
static const WCHAR GIFW[] = {'G','I','F',0};
static const WCHAR JFIFW[] = {'J','F','I','F',0};
static const WCHAR PNGW[] = {'P','N','G',0};
static const WCHAR HTMLFormatW[] = {'H','T','M','L',' ','F','o','r','m','a','t',0};

static const struct
{
    const WCHAR  *name;
    UINT          id;
    UINT          data;
    IMPORTFUNC    import;
    EXPORTFUNC    export;
} builtin_formats[] =
{
    { 0, CF_UNICODETEXT,     XATOM_UTF8_STRING,         import_utf8_string,   export_utf8_string },
    { 0, CF_UNICODETEXT,     XATOM_COMPOUND_TEXT,       import_compound_text, export_compound_text },
    { 0, CF_UNICODETEXT,     XA_STRING,                 import_string,        export_string },
    { 0, CF_UNICODETEXT,     XATOM_text_plain,          import_string,        export_string },
    { 0, CF_UNICODETEXT,     XATOM_TEXT,                import_text,          export_text },
    { 0, CF_SYLK,            XATOM_WCF_SYLK,            import_data,          export_data },
    { 0, CF_DIF,             XATOM_WCF_DIF,             import_data,          export_data },
    { 0, CF_TIFF,            XATOM_WCF_TIFF,            import_data,          export_data },
    { 0, CF_DIB,             XA_PIXMAP,                 import_pixmap,        export_pixmap },
    { 0, CF_PENDATA,         XATOM_WCF_PENDATA,         import_data,          export_data },
    { 0, CF_RIFF,            XATOM_WCF_RIFF,            import_data,          export_data },
    { 0, CF_WAVE,            XATOM_WCF_WAVE,            import_data,          export_data },
    { 0, CF_ENHMETAFILE,     XATOM_WCF_ENHMETAFILE,     import_data,          export_data },
    { 0, CF_HDROP,           XATOM_text_uri_list,       import_text_uri_list, export_hdrop },
    { 0, CF_DIB,             XATOM_image_bmp,           import_image_bmp,     export_image_bmp },
    { RichTextFormatW, 0,    XATOM_text_rtf,            import_data,          export_data },
    { RichTextFormatW, 0,    XATOM_text_richtext,       import_data,          export_data },
    { GIFW, 0,               XATOM_image_gif,           import_data,          export_data },
    { JFIFW, 0,              XATOM_image_jpeg,          import_data,          export_data },
    { PNGW, 0,               XATOM_image_png,           import_data,          export_data },
    { HTMLFormatW, 0,        XATOM_HTML_Format,         import_data,          export_data },
    { HTMLFormatW, 0,        XATOM_text_html,           import_text_html,     export_text_html },
    { 0, 0,                  XATOM_TARGETS,             import_targets,       export_targets },
    { 0, 0,                  XATOM_MULTIPLE,            NULL,                 export_multiple },
    { 0, 0,                  XATOM_TIMESTAMP,           NULL,                 export_timestamp },
};

static struct list format_list = LIST_INIT( format_list );

#define GET_ATOM(prop)  (((prop) < FIRST_XATOM) ? (Atom)(prop) : X11DRV_Atoms[(prop) - FIRST_XATOM])

static DWORD clipboard_thread_id;
static HWND clipboard_hwnd;
static BOOL is_clipboard_owner;
static Window selection_window;
static Window import_window;
static Atom current_selection;
static UINT rendered_formats;
static ULONG last_clipboard_update;
static struct clipboard_format **current_x11_formats;
static unsigned int nb_current_x11_formats;
static BOOL use_xfixes;

Display *clipboard_display = NULL;

static const char *debugstr_format( UINT id )
{
    WCHAR buffer[256];

    if (NtUserGetClipboardFormatName( id, buffer, ARRAYSIZE(buffer) ))
        return wine_dbg_sprintf( "%04x %s", id, debugstr_w(buffer) );

    switch (id)
    {
    case 0: return "(none)";
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

static const char *debugstr_xatom( Atom atom )
{
    const char *ret;
    char *name;

    if (!atom) return "(None)";
    name = XGetAtomName( thread_display(), atom );
    ret = debugstr_a( name );
    XFree( name );
    return ret;
}


static int is_atom_error( Display *display, XErrorEvent *event, void *arg )
{
    return (event->error_code == BadAtom);
}


/**************************************************************************
 *		find_win32_format
 */
static struct clipboard_format *find_win32_format( UINT id )
{
    struct clipboard_format *format;

    LIST_FOR_EACH_ENTRY( format, &format_list, struct clipboard_format, entry )
        if (format->id == id) return format;
    return NULL;
}


/**************************************************************************
 *		find_x11_format
 */
static struct clipboard_format *find_x11_format( Atom atom )
{
    struct clipboard_format *format;

    LIST_FOR_EACH_ENTRY( format, &format_list, struct clipboard_format, entry )
        if (format->atom == atom) return format;
    return NULL;
}


static ATOM register_clipboard_format( const WCHAR *name )
{
    ATOM atom;
    if (NtAddAtom( name, lstrlenW( name ) * sizeof(WCHAR), &atom )) return 0;
    return atom;
}


/**************************************************************************
 *		register_builtin_formats
 */
static void register_builtin_formats(void)
{
    struct clipboard_format *formats;
    unsigned int i;

    if (!(formats = malloc( ARRAY_SIZE(builtin_formats) * sizeof(*formats)))) return;

    for (i = 0; i < ARRAY_SIZE(builtin_formats); i++)
    {
        if (builtin_formats[i].name)
            formats[i].id = register_clipboard_format( builtin_formats[i].name );
        else
            formats[i].id = builtin_formats[i].id;

        formats[i].atom   = GET_ATOM(builtin_formats[i].data);
        formats[i].import = builtin_formats[i].import;
        formats[i].export = builtin_formats[i].export;
        list_add_tail( &format_list, &formats[i].entry );
    }
}


/**************************************************************************
 *		register_formats
 */
static void register_formats( const UINT *ids, const Atom *atoms, unsigned int count )
{
    struct clipboard_format *formats;
    unsigned int i;

    if (!(formats = malloc( count * sizeof(*formats)))) return;

    for (i = 0; i < count; i++)
    {
        formats[i].id     = ids[i];
        formats[i].atom   = atoms[i];
        formats[i].import = import_data;
        formats[i].export = export_data;
        list_add_tail( &format_list, &formats[i].entry );
        TRACE( "registered %s atom %s\n", debugstr_format( ids[i] ), debugstr_xatom( atoms[i] ));
    }
}


/**************************************************************************
 *		register_win32_formats
 *
 * Register Win32 clipboard formats the first time we encounter them.
 */
static void register_win32_formats( const UINT *ids, UINT size )
{
    unsigned int count, len;
    UINT new_ids[256];
    char *names[256];
    Atom atoms[256];
    WCHAR buffer[256];

    if (list_empty( &format_list)) register_builtin_formats();

    while (size)
    {
        for (count = 0; count < 256 && size; ids++, size--)
        {
            if (find_win32_format( *ids )) continue;  /* it already exists */
            if (!NtUserGetClipboardFormatName( *ids, buffer, ARRAYSIZE(buffer) ))
                continue;  /* not a named format */
            len = lstrlenW( buffer );
            if (!(names[count] = malloc( len * 3 + 1 ))) continue;
            ntdll_wcstoumbs( buffer, len + 1, names[count], len * 3 + 1, FALSE );
            new_ids[count++] = *ids;
        }
        if (!count) return;

        XInternAtoms( thread_display(), names, count, False, atoms );
        register_formats( new_ids, atoms, count );
        while (count) free( names[--count] );
    }
}


/**************************************************************************
 *		register_x11_formats
 *
 * Register X11 atom formats the first time we encounter them.
 */
static void register_x11_formats( const Atom *atoms, UINT size )
{
    Display *display = thread_display();
    unsigned int i, pos, count;
    char *names[256];
    UINT ids[256];
    Atom new_atoms[256];
    WCHAR buffer[256];

    if (list_empty( &format_list)) register_builtin_formats();

    while (size)
    {
        for (count = 0; count < 256 && size; atoms++, size--)
            if (!find_x11_format( *atoms )) new_atoms[count++] = *atoms;

        if (!count) return;

        X11DRV_expect_error( display, is_atom_error, NULL );
        if (!XGetAtomNames( display, new_atoms, count, names )) count = 0;
        if (X11DRV_check_error())
        {
            WARN( "got some bad atoms, ignoring\n" );
            count = 0;
        }

        for (i = pos = 0; i < count; i++)
        {
            if (ntdll_umbstowcs( names[i], strlen( names[i] ) + 1, buffer, ARRAYSIZE(buffer) ) &&
                (ids[pos] = register_clipboard_format( buffer )))
                new_atoms[pos++] = new_atoms[i];
            XFree( names[i] );
        }
        register_formats( ids, new_atoms, pos );
    }
}


/**************************************************************************
 *		put_property
 *
 * Put data as a property on the specified window.
 */
static void put_property( Display *display, Window win, Atom prop, Atom type, int format,
                          const void *ptr, size_t size )
{
    const unsigned char *data = ptr;
    int mode = PropModeReplace;
    size_t width = (format == 32) ? sizeof(long) : format / 8;
    size_t max_size = XExtendedMaxRequestSize( display ) * 4;

    if (!max_size) max_size = XMaxRequestSize( display ) * 4;
    max_size -= 64; /* request overhead */

    do
    {
        size_t count = min( size, max_size / width );
        XChangeProperty( display, win, prop, type, format, mode, data, count );
        mode = PropModeAppend;
        size -= count;
        data += count * width;
    } while (size > 0);
}


static void selection_sleep(void)
{
    LARGE_INTEGER timeout;
    timeout.QuadPart = (ULONGLONG)SELECTION_WAIT * -10000;
    NtDelayExecution( FALSE, &timeout );
}

/**************************************************************************
 *		convert_selection
 */
static BOOL convert_selection( Display *display, Window win, Atom selection,
                               struct clipboard_format *format, Atom *type,
                               unsigned char **data, size_t *size )
{
    int i;
    XEvent event;

    TRACE( "import %s from %s win %lx to format %s\n",
           debugstr_xatom( format->atom ), debugstr_xatom( selection ),
           win, debugstr_format( format->id ) );

    XConvertSelection( display, selection, format->atom, x11drv_atom(SELECTION_DATA), win, CurrentTime );

    for (i = 0; i < SELECTION_RETRIES; i++)
    {
        Bool res = XCheckTypedWindowEvent( display, win, SelectionNotify, &event );
        if (res && event.xselection.selection == selection && event.xselection.target == format->atom)
            return read_property( display, win, event.xselection.property, type, data, size );
        selection_sleep();
    }
    ERR( "Timed out waiting for SelectionNotify event\n" );
    return FALSE;
}


/***********************************************************************
 *           bitmap_info_size
 *
 * Return the size of the bitmap info structure including color table.
 */
static int bitmap_info_size( const BITMAPINFO * info, WORD coloruse )
{
    unsigned int colors, size, masks = 0;

    if (info->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
    {
        const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)info;
        colors = (core->bcBitCount <= 8) ? 1 << core->bcBitCount : 0;
        return sizeof(BITMAPCOREHEADER) + colors *
             ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBTRIPLE) : sizeof(WORD));
    }
    else  /* assume BITMAPINFOHEADER */
    {
        colors = info->bmiHeader.biClrUsed;
        if (!colors && (info->bmiHeader.biBitCount <= 8))
            colors = 1 << info->bmiHeader.biBitCount;
        if (info->bmiHeader.biCompression == BI_BITFIELDS) masks = 3;
        size = max( info->bmiHeader.biSize, sizeof(BITMAPINFOHEADER) + masks * sizeof(DWORD) );
        return size + colors * ((coloruse == DIB_RGB_COLORS) ? sizeof(RGBQUAD) : sizeof(WORD));
    }
}


/***********************************************************************
 *           create_dib_from_bitmap
 *
 *  Allocates a packed DIB and copies the bitmap data into it.
 */
static void *create_dib_from_bitmap( HBITMAP hBmp, size_t *size )
{
    BITMAP bmp;
    HDC hdc;
    LPBITMAPINFOHEADER pbmiHeader;
    unsigned int cDataSize, OffsetBits;
    int nLinesCopied;
    char *ret;

    if (!NtGdiExtGetObjectW( hBmp, sizeof(bmp), &bmp )) return 0;

    /*
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */

    /* Calculate the size of the packed DIB */
    cDataSize = abs( bmp.bmHeight ) * (((bmp.bmWidth * bmp.bmBitsPixel + 31) / 8) & ~3);
    *size = sizeof(BITMAPINFOHEADER)
                  + ( (bmp.bmBitsPixel <= 8) ? (sizeof(RGBQUAD) * (1 << bmp.bmBitsPixel)) : 0 )
                  + cDataSize;
    /* Get the offset to the bits */
    OffsetBits = *size - cDataSize;

    /* Allocate the packed DIB */
    TRACE( "\tAllocating packed DIB\n" );
    if (!(ret = malloc( *size )))
    {
        WARN( "Could not allocate packed DIB!\n" );
        return 0;
    }

    /* A packed DIB starts with a BITMAPINFOHEADER */
    pbmiHeader = (LPBITMAPINFOHEADER)ret;

    /* Init the BITMAPINFOHEADER */
    pbmiHeader->biSize = sizeof(BITMAPINFOHEADER);
    pbmiHeader->biWidth = bmp.bmWidth;
    pbmiHeader->biHeight = bmp.bmHeight;
    pbmiHeader->biPlanes = 1;
    pbmiHeader->biBitCount = bmp.bmBitsPixel;
    pbmiHeader->biCompression = BI_RGB;
    pbmiHeader->biSizeImage = 0;
    pbmiHeader->biXPelsPerMeter = pbmiHeader->biYPelsPerMeter = 0;
    pbmiHeader->biClrUsed = 0;
    pbmiHeader->biClrImportant = 0;

    /* Retrieve the DIB bits from the bitmap and fill in the
     * DIB color table if present */
    hdc = NtUserGetDC( 0 );
    nLinesCopied = NtGdiGetDIBitsInternal( hdc, hBmp, 0, bmp.bmHeight,  ret + OffsetBits,
                                           (LPBITMAPINFO) pbmiHeader, 0, 0, 0 );
    NtUserReleaseDC( 0, hdc );

    /* Cleanup if GetDIBits failed */
    if (nLinesCopied != bmp.bmHeight)
    {
        TRACE("\tGetDIBits returned %d. Actual lines=%d\n", nLinesCopied, bmp.bmHeight);
        free( ret );
        ret = NULL;
    }
    return ret;
}


/* based on wine_get_dos_file_name */
static WCHAR *get_dos_file_name( const char *path )
{
    ULONG len = strlen( path ) + 9; /* \??\unix prefix */
    WCHAR *ret;

    if (!(ret = malloc( len * sizeof(WCHAR) ))) return NULL;
    if (wine_unix_to_nt_file_name( path, ret, &len ))
    {
        free( ret );
        return NULL;
    }

    if (ret[5] == ':')
    {
        /* get rid of the \??\ prefix */
        memmove( ret, ret + 4, (len - 4) * sizeof(WCHAR) );
    }
    else ret[1] = '\\';
    return ret;
}


/***********************************************************************
 *           get_nt_pathname
 *
 * Simplified version of RtlDosPathNameToNtPathName_U.
 */
static BOOL get_nt_pathname( const WCHAR *name, UNICODE_STRING *nt_name )
{
    static const WCHAR ntprefixW[] = {'\\','?','?','\\'};
    static const WCHAR uncprefixW[] = {'U','N','C','\\'};
    size_t len = lstrlenW( name );
    WCHAR *ptr;

    nt_name->MaximumLength = (len + 8) * sizeof(WCHAR);
    if (!(ptr = malloc( nt_name->MaximumLength ))) return FALSE;
    nt_name->Buffer = ptr;

    memcpy( ptr, ntprefixW, sizeof(ntprefixW) );
    ptr += ARRAYSIZE(ntprefixW);
    if (name[0] == '\\' && name[1] == '\\')
    {
        if ((name[2] == '.' || name[2] == '?') && name[3] == '\\')
        {
            name += 4;
            len -= 4;
        }
        else
        {
            memcpy( ptr, uncprefixW, sizeof(uncprefixW) );
            ptr += ARRAYSIZE(uncprefixW);
            name += 2;
            len -= 2;
        }
    }
    memcpy( ptr, name, (len + 1) * sizeof(WCHAR) );
    ptr += len;
    nt_name->Length = (ptr - nt_name->Buffer) * sizeof(WCHAR);
    return TRUE;
}


/* based on wine_get_unix_file_name */
static char *get_unix_file_name( const WCHAR *dosW )
{
    UNICODE_STRING nt_name;
    OBJECT_ATTRIBUTES attr;
    NTSTATUS status;
    ULONG size = 256;
    char *buffer;

    if (!get_nt_pathname( dosW, &nt_name )) return NULL;
    InitializeObjectAttributes( &attr, &nt_name, 0, 0, NULL );
    for (;;)
    {
        if (!(buffer = malloc( size )))
        {
            free( nt_name.Buffer );
            return NULL;
        }
        status = wine_nt_to_unix_file_name( &attr, buffer, &size, FILE_OPEN_IF );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        free( buffer );
    }
    free( nt_name.Buffer );
    if (status)
    {
        free( buffer );
        return NULL;
    }
    return buffer;
}


static CPTABLEINFO *get_xstring_cp(void)
{
    static CPTABLEINFO cp;
    if (!cp.CodePage)
    {
        USHORT *ptr;
        SIZE_T nls_size;
        if (NtGetNlsSectionPtr( 11, 28591, NULL, (void **)&ptr, &nls_size )) return NULL;
        RtlInitCodePageTable( ptr, &cp );
    }
    return &cp;
}


static CPTABLEINFO *get_ansi_cp(void)
{
    USHORT utf8_hdr[2] = { 0, CP_UTF8 };
    static CPTABLEINFO cp;
    if (!cp.CodePage)
    {
        if (NtCurrentTeb()->Peb->AnsiCodePageData)
            RtlInitCodePageTable( NtCurrentTeb()->Peb->AnsiCodePageData, &cp );
        else
            RtlInitCodePageTable( utf8_hdr, &cp );
    }
    return &cp;
}


/***********************************************************************
 *           uri_to_dos
 *
 *  Converts a text/uri-list URI to DOS filename.
 */
static WCHAR* uri_to_dos(char *encodedURI)
{
    WCHAR *ret = NULL;
    int i;
    int j = 0;
    char *uri = calloc( 1, strlen(encodedURI) + 1 );
    if (uri == NULL)
        return NULL;
    for (i = 0; encodedURI[i]; ++i)
    {
        if (encodedURI[i] == '%')
        {
            if (encodedURI[i+1] && encodedURI[i+2])
            {
                char buffer[3];
                int number;
                buffer[0] = encodedURI[i+1];
                buffer[1] = encodedURI[i+2];
                buffer[2] = '\0';
                sscanf(buffer, "%x", &number);
                uri[j++] = number;
                i += 2;
            }
            else
            {
                WARN("invalid URI encoding in %s\n", debugstr_a(encodedURI));
                free( uri );
                return NULL;
            }
        }
        else
            uri[j++] = encodedURI[i];
    }

    /* Read http://www.freedesktop.org/wiki/Draganddropwarts and cry... */
    if (strncmp(uri, "file:/", 6) == 0)
    {
        if (uri[6] == '/')
        {
            if (uri[7] == '/')
            {
                /* file:///path/to/file (nautilus, thunar) */
                ret = get_dos_file_name( &uri[7] );
            }
            else if (uri[7])
            {
                /* file://hostname/path/to/file (X file drag spec) */
                char hostname[256];
                char *path = strchr(&uri[7], '/');
                if (path)
                {
                    *path = '\0';
                    if (strcmp(&uri[7], "localhost") == 0)
                    {
                        *path = '/';
                        ret = get_dos_file_name( path );
                    }
                    else if (gethostname(hostname, sizeof(hostname)) == 0)
                    {
                        if (strcmp(hostname, &uri[7]) == 0)
                        {
                            *path = '/';
                            ret = get_dos_file_name( path );
                        }
                    }
                }
            }
        }
        else if (uri[6])
        {
            /* file:/path/to/file (konqueror) */
            ret = get_dos_file_name( &uri[5] );
        }
    }
    free( uri );
    return ret;
}


/**************************************************************************
 *		unicode_text_from_string
 *
 * Convert a string in the specified encoding to CF_UNICODETEXT format.
 */
static void *unicode_text_from_string( WCHAR *ret, const WCHAR *string, DWORD count, size_t *size )
{
    DWORD i, j;

    for (i = j = 0; i < count; i++)
    {
        if (string[i] == '\n' && (!i || string[i - 1] != '\r')) ret[j++] = '\r';
        ret[j++] = string[i];
    }
    ret[j++] = 0;
    *size = j * sizeof(WCHAR);
    TRACE( "returning %s\n", debugstr_wn( ret, j - 1 ));
    return ret;
}


/**************************************************************************
 *		import_string
 *
 * Import XA_STRING, converting the string to CF_UNICODETEXT.
 */
static void *import_string( Atom type, const void *data, size_t size, size_t *ret_size )
{
    DWORD str_size;
    WCHAR *ret;

    if (!(ret = malloc( (size * 2 + 1) * sizeof(WCHAR) ))) return NULL;
    RtlCustomCPToUnicodeN( get_xstring_cp(), ret + size, size * sizeof(WCHAR), &str_size, data, size );
    return unicode_text_from_string( ret, ret + size, str_size / sizeof(WCHAR), ret_size );
}


/**************************************************************************
 *		import_utf8_string
 *
 * Import XA_UTF8_STRING, converting the string to CF_UNICODETEXT.
 */
static void *import_utf8_string( Atom type, const void *data, size_t size, size_t *ret_size )
{
    DWORD str_size;
    WCHAR *ret;

    RtlUTF8ToUnicodeN( NULL, 0, &str_size, data, size );
    if (!(ret = malloc( str_size * 2 + sizeof(WCHAR) ))) return NULL;
    RtlUTF8ToUnicodeN( ret + str_size / sizeof(WCHAR), str_size, &str_size, data, size );
    return unicode_text_from_string( ret, ret + str_size / sizeof(WCHAR),
                                     str_size / sizeof(WCHAR), ret_size );
}


/**************************************************************************
 *		import_compound_text
 *
 * Import COMPOUND_TEXT to CF_UNICODETEXT.
 */
static void *import_compound_text( Atom type, const void *data, size_t size, size_t *ret_size )
{
    char** srcstr;
    int count;
    XTextProperty txtprop;
    DWORD len;
    WCHAR *ret;

    txtprop.value = (BYTE *)data;
    txtprop.nitems = size;
    txtprop.encoding = x11drv_atom(COMPOUND_TEXT);
    txtprop.format = 8;
    if (XmbTextPropertyToTextList( thread_display(), &txtprop, &srcstr, &count ) != Success) return 0;
    if (!count) return 0;

    len = strlen(srcstr[0]) + 1;
    if (!(ret = malloc( (len * 2 + 1) * sizeof(WCHAR) ))) return NULL;

    count = ntdll_umbstowcs( srcstr[0], len, ret + len, len );
    ret = unicode_text_from_string( ret, ret + len, count, ret_size );

    XFreeStringList(srcstr);
    return ret;
}


/**************************************************************************
 *		import_text
 *
 * Import XA_TEXT, converting the string to CF_UNICODETEXT.
 */
static void *import_text( Atom type, const void *data, size_t size, size_t *ret_size )
{
    if (type == XA_STRING) return import_string( type, data, size, ret_size );
    if (type == x11drv_atom(UTF8_STRING)) return import_utf8_string( type, data, size, ret_size );
    if (type == x11drv_atom(COMPOUND_TEXT)) return import_compound_text( type, data, size, ret_size );
    FIXME( "unsupported TEXT type %s\n", debugstr_xatom( type ));
    return 0;
}


/**************************************************************************
 *		import_pixmap
 *
 *  Import XA_PIXMAP, converting the image to CF_DIB.
 */
static void *import_pixmap( Atom type, const void *data, size_t size, size_t *ret_size )
{
    const Pixmap *pPixmap = (const Pixmap *)data;
    BYTE *ptr = NULL;
    XVisualInfo vis = default_visual;
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    struct gdi_image_bits bits;
    Window root;
    int x,y;               /* Unused */
    unsigned border_width; /* Unused */
    unsigned int depth, width, height;

    /* Get the Pixmap dimensions and bit depth */
    if (!XGetGeometry(gdi_display, *pPixmap, &root, &x, &y, &width, &height,
                      &border_width, &depth)) depth = 0;
    if (!pixmap_formats[depth]) return 0;

    TRACE( "pixmap properties: width=%d, height=%d, depth=%d\n", width, height, depth );

    if (depth != vis.depth) switch (pixmap_formats[depth]->bits_per_pixel)
    {
        case 1:
        case 4:
        case 8:
            break;
        case 16:  /* assume R5G5B5 */
            vis.red_mask   = 0x7c00;
            vis.green_mask = 0x03e0;
            vis.blue_mask  = 0x001f;
            break;
        case 24:  /* assume R8G8B8 */
        case 32:  /* assume A8R8G8B8 */
            vis.red_mask   = 0xff0000;
            vis.green_mask = 0x00ff00;
            vis.blue_mask  = 0x0000ff;
            break;
        default:
            return 0;
    }

    if (!get_pixmap_image( *pPixmap, width, height, &vis, info, &bits ))
    {
        DWORD info_size = bitmap_info_size( info, DIB_RGB_COLORS );

        ptr = malloc( info_size + info->bmiHeader.biSizeImage );
        if (ptr)
        {
            memcpy( ptr, info, info_size );
            memcpy( ptr + info_size, bits.ptr, info->bmiHeader.biSizeImage );
            *ret_size = info_size + info->bmiHeader.biSizeImage;
        }
        if (bits.free) bits.free( &bits );
    }
    return ptr;
}


/**************************************************************************
 *		import_image_bmp
 *
 *  Import image/bmp, converting the image to CF_DIB.
 */
static void *import_image_bmp( Atom type, const void *data, size_t size, size_t *ret_size )
{
    const BITMAPFILEHEADER *bfh = data;
    void *ret = NULL;

    if (size >= sizeof(BITMAPFILEHEADER)+sizeof(BITMAPCOREHEADER) &&
        bfh->bfType == 0x4d42 /* "BM" */)
    {
        const BITMAPINFO *bmi = (const BITMAPINFO *)(bfh + 1);
        int width, height;
        HBITMAP hbmp;
        HDC hdc;

        if (bmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
        {
            const BITMAPCOREHEADER *core = (const BITMAPCOREHEADER *)bmi;
            width  = core->bcWidth;
            height = core->bcHeight;
        }
        else if (bmi->bmiHeader.biSize >= sizeof(BITMAPINFOHEADER))
        {
            const BITMAPINFOHEADER *header = &bmi->bmiHeader;
            if (header->biCompression == BI_JPEG || header->biCompression == BI_PNG) return 0;
            width  = header->biWidth;
            height = header->biHeight;
        }
        else return NULL;
        if (!width || !height) return NULL;

        hdc = NtUserGetDC( 0 );

        if ((hbmp = NtGdiCreateDIBitmapInternal( hdc, width, height, CBM_INIT,
                                                 (const BYTE *)data + bfh->bfOffBits, bmi,
                                                 DIB_RGB_COLORS, 0, 0, 0, 0 )))
        {
            ret = create_dib_from_bitmap( hbmp, ret_size );
            NtGdiDeleteObjectApp( hbmp );
        }
        NtUserReleaseDC(0, hdc);
    }
    return ret;
}


/**************************************************************************
 *              import_text_html
 */
static void *import_text_html( Atom type, const void *data, size_t size, size_t *ret_size )
{
    static const char header[] =
        "Version:0.9\n"
        "StartHTML:0000000100\n"
        "EndHTML:%010lu\n"
        "StartFragment:%010lu\n"
        "EndFragment:%010lu\n"
        "<!--StartFragment-->";
    static const char trailer[] = "\n<!--EndFragment-->";
    char *text = NULL;
    HANDLE ret;
    SIZE_T len, total;

    /* Firefox uses UTF-16LE with byte order mark. Convert to UTF-8 without the BOM. */
    if (size >= sizeof(WCHAR) && ((const WCHAR *)data)[0] == 0xfeff)
    {
        DWORD str_len;
        RtlUnicodeToUTF8N( NULL, 0, &str_len, (const WCHAR *)data + 1, size - sizeof(WCHAR) );
        if (!(text = malloc( str_len ))) return NULL;
        RtlUnicodeToUTF8N( text, str_len, &str_len, (const WCHAR *)data + 1, size - sizeof(WCHAR) );
        size = str_len;
        data = text;
    }

    len = strlen( header ) + 12;  /* 3 * 4 extra chars for %010lu */
    total = len + size + sizeof(trailer);
    if ((ret = malloc( total )))
    {
        char *p = ret;
        p += sprintf( p, header, total - 1, len, len + size + 1 /* include the final \n in the data */ );
        memcpy( p, data, size );
        strcpy( p + size, trailer );
        *ret_size = total;
        TRACE( "returning %s\n", debugstr_a( ret ));
    }
    free( text );
    return ret;
}


/**************************************************************************
 *      file_list_to_drop_files
 */
DROPFILES *file_list_to_drop_files( const void *data, size_t size, size_t *ret_size )
{
    size_t buf_size = 4096, path_size;
    DROPFILES *drop = NULL;
    const char *ptr;
    WCHAR *path;

    for (ptr = data; ptr < (const char *)data + size; ptr += strlen( ptr ) + 1)
    {
        path = get_dos_file_name( ptr );

        TRACE( "converted URI %s to DOS path %s\n", debugstr_a(ptr), debugstr_w(path) );

        if (!path) continue;

        if (!drop)
        {
            if (!(drop = malloc( buf_size ))) return NULL;
            drop->pFiles = sizeof(*drop);
            drop->pt.x = drop->pt.y = 0;
            drop->fNC = FALSE;
            drop->fWide = TRUE;
            *ret_size = sizeof(*drop);
        }

        path_size = (lstrlenW( path ) + 1) * sizeof(WCHAR);
        if (*ret_size + path_size > buf_size - sizeof(WCHAR))
        {
            void *new_buf;
            if (!(new_buf = realloc( drop, buf_size * 2 + path_size )))
            {
                free( path );
                continue;
            }
            buf_size = buf_size * 2 + path_size;
            drop = new_buf;
        }

        memcpy( (char *)drop + *ret_size, path, path_size );
        *ret_size += path_size;
    }

    if (!drop) return NULL;
    *(WCHAR *)((char *)drop + *ret_size) = 0;
    *ret_size += sizeof(WCHAR);
    return drop;
}


/**************************************************************************
 *      uri_list_to_drop_files
 */
DROPFILES *uri_list_to_drop_files( const void *data, size_t size, size_t *ret_size )
{
    const char *uriList = data;
    char *uri;
    WCHAR *path;
    WCHAR *out = NULL;
    int total = 0;
    int capacity = 4096;
    int start = 0;
    int end = 0;
    DROPFILES *dropFiles = NULL;

    if (!(out = malloc( capacity * sizeof(WCHAR) ))) return 0;

    while (end < size)
    {
        while (end < size && uriList[end] != '\r')
            ++end;
        if (end < (size - 1) && uriList[end+1] != '\n')
        {
            WARN("URI list line doesn't end in \\r\\n\n");
            break;
        }

        uri = malloc( end - start + 1 );
        if (uri == NULL)
            break;
        lstrcpynA(uri, &uriList[start], end - start + 1);
        path = uri_to_dos(uri);
        TRACE("converted URI %s to DOS path %s\n", debugstr_a(uri), debugstr_w(path));
        free( uri );

        if (path)
        {
            int pathSize = wcslen( path ) + 1;
            if (pathSize > capacity - total)
            {
                WCHAR *new_out;
                capacity = 2*capacity + pathSize;
                new_out = realloc( out, (capacity + 1) * sizeof(WCHAR) );
                if (!new_out)
                    goto done;
                out = new_out;
            }
            memcpy(&out[total], path, pathSize * sizeof(WCHAR));
            total += pathSize;
        done:
            free( path );
            if (out == NULL)
                break;
        }

        start = end + 2;
        end = start;
    }
    if (out && end >= size)
    {
        *ret_size = sizeof(DROPFILES) + (total + 1) * sizeof(WCHAR);
        if ((dropFiles = malloc( *ret_size )))
        {
            dropFiles->pFiles = sizeof(DROPFILES);
            dropFiles->pt.x = 0;
            dropFiles->pt.y = 0;
            dropFiles->fNC = 0;
            dropFiles->fWide = TRUE;
            out[total] = '\0';
            memcpy( (char*)dropFiles + dropFiles->pFiles, out, (total + 1) * sizeof(WCHAR) );
        }
    }
    free( out );
    return dropFiles;
}


/**************************************************************************
 *      import_text_uri_list
 *
 *  Import text/uri-list.
 */
static void *import_text_uri_list( Atom type, const void *data, size_t size, size_t *ret_size )
{
    return uri_list_to_drop_files( data, size, ret_size );
}


/**************************************************************************
 *		import_targets
 *
 *  Import TARGETS and mark the corresponding clipboard formats as available.
 */
static void *import_targets( Atom type, const void *data, size_t size, size_t *ret_size )
{
    UINT i, pos, count = size / sizeof(Atom);
    const Atom *properties = data;
    struct clipboard_format *format, **formats;

    if (type != XA_ATOM && type != x11drv_atom(TARGETS)) return 0;

    register_x11_formats( properties, count );

    /* the builtin formats contain duplicates, so allocate some extra space */
    if (!(formats = malloc( (count + ARRAY_SIZE(builtin_formats)) * sizeof(*formats ))))
        return 0;

    pos = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, struct clipboard_format, entry )
    {
        for (i = 0; i < count; i++) if (properties[i] == format->atom) break;
        if (i == count) continue;
        if (format->import && format->id)
        {
            struct set_clipboard_params params = { .data = NULL };

            TRACE( "property %s -> format %s\n",
                   debugstr_xatom( properties[i] ), debugstr_format( format->id ));

            NtUserSetClipboardData( format->id, 0, &params );
            formats[pos++] = format;
        }
        else TRACE( "property %s (ignoring)\n", debugstr_xatom( properties[i] ));
    }

    free( current_x11_formats );
    current_x11_formats = formats;
    nb_current_x11_formats = pos;
    *ret_size = 0;
    return (void *)1;
}


/**************************************************************************
 *		import_data
 *
 *  Generic import clipboard data routine.
 */
static void *import_data( Atom type, const void *data, size_t size, size_t *ret_size )
{
    void *ret = malloc( size );
    if (ret)
    {
        memcpy( ret, data, size );
        *ret_size = size;
    }
    return ret;
}


/**************************************************************************
 *		import_selection
 *
 * Import the specified format from the selection and return a global handle to the data.
 */
static void *import_selection( Display *display, Window win, Atom selection,
                               struct clipboard_format *format, size_t *ret_size )
{
    unsigned char *data;
    size_t size;
    Atom type;
    HANDLE ret;

    if (!format->import) return 0;

    if (!convert_selection( display, win, selection, format, &type, &data, &size ))
    {
        TRACE( "failed to convert selection\n" );
        return 0;
    }
    ret = format->import( type, data, size, ret_size );
    free( data );
    return ret;
}


/**************************************************************************
 *      import_xdnd_selection
 *
 *  Import the X selection into the clipboard format registered for the given X target.
 */
struct format_entry *import_xdnd_selection( Display *display, Window win, Atom selection,
                                            Atom *targets, UINT count, size_t *ret_size )
{
    size_t size, buf_size = 0, entry_size;
    UINT i;
    void *data;
    struct clipboard_format *format;
    struct format_entry *ret = NULL, *tmp, *entry;
    BOOL have_hdrop = FALSE;

    register_x11_formats( targets, count );
    *ret_size = 0;

    for (i = 0; i < count; i++)
    {
        if (!(format = find_x11_format( targets[i] ))) continue;
        if (format->id != CF_HDROP) continue;
        have_hdrop = TRUE;
        break;
    }

    for (i = 0; i < count; i++)
    {
        if (!(format = find_x11_format( targets[i] ))) continue;
        if (!format->id) continue;
        if (have_hdrop && format->id != CF_HDROP && format->id < CF_MAX) continue;

        if (!(data = import_selection( display, win, selection, format, &size ))) continue;

        entry_size = (FIELD_OFFSET( struct format_entry, data[size] ) + 7) & ~7;
        if (buf_size < *ret_size + entry_size)
        {
            if (!(tmp = realloc( ret, *ret_size + entry_size + 1024 ))) continue;
            ret = tmp;
            buf_size = *ret_size + entry_size + 1024; /* extra space for following entries */
        }
        entry = (struct format_entry *)((char *)ret + *ret_size);
        entry->format = format->id;
        entry->size = size;
        if (size) memcpy( entry->data, data, size );
        *ret_size += entry_size;
        free( data );
    }

    return ret;
}


/**************************************************************************
 *		render_format
 */
static BOOL render_format( UINT id )
{
    Display *display = thread_display();
    unsigned int i;

    if (!current_selection) return 0;

    for (i = 0; i < nb_current_x11_formats; i++)
    {
        struct set_clipboard_params params = { 0 };
        if (current_x11_formats[i]->id != id) continue;
        if (!(params.data = import_selection( display, import_window, current_selection,
                                              current_x11_formats[i], &params.size ))) continue;
        NtUserSetClipboardData( id, 0, &params );
        if (params.size) free( params.data );
        return TRUE;
    }
    return FALSE;
}


/**************************************************************************
 *		export_data
 *
 *  Generic export clipboard data routine.
 */
static BOOL export_data( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    put_property( display, win, prop, target, 8, data, size );
    return TRUE;
}


/**************************************************************************
 *		string_from_unicode_text
 *
 * Convert CF_UNICODETEXT data to a string in the specified codepage.
 */
static void string_from_unicode_text( char *str, size_t len, DWORD *size )
{
    DWORD i, j;

    /* remove carriage returns */
    for (i = j = 0; i < len; i++)
    {
        if (str[i] == '\r' && (i == len - 1 || str[i + 1] == '\n')) continue;
        str[j++] = str[i];
    }
    while (j && !str[j - 1]) j--;  /* remove trailing nulls */
    TRACE( "returning %s\n", debugstr_an( str, j ));
    *size = j;
}


/**************************************************************************
 *		export_string
 *
 * Export CF_UNICODETEXT converting the string to XA_STRING.
 */
static BOOL export_string( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    DWORD len;
    char *text;

    if (!(text = malloc( size ))) return FALSE;
    RtlUnicodeToCustomCPN( get_xstring_cp(), text, size, &len, data, size );
    string_from_unicode_text( text, len, &len );

    put_property( display, win, prop, target, 8, text, len );
    free( text );
    return TRUE;
}


/**************************************************************************
 *		export_utf8_string
 *
 *  Export CF_UNICODE converting the string to UTF8.
 */
static BOOL export_utf8_string( Display *display, Window win, Atom prop, Atom target,
                                void *data, size_t size )
{
    DWORD len;
    char *text;

    if (!(text = malloc( size / sizeof(WCHAR) * 3 ))) return FALSE;
    RtlUnicodeToUTF8N( text, size / sizeof(WCHAR) * 3, &len, data, size );
    string_from_unicode_text( text, len, &len );

    put_property( display, win, prop, target, 8, text, len );
    free( text );
    return TRUE;
}


/**************************************************************************
 *		export_text
 *
 *  Export CF_UNICODE to the polymorphic TEXT type, using UTF8.
 */
static BOOL export_text( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    return export_utf8_string( display, win, prop, x11drv_atom(UTF8_STRING), data, size );
}


/**************************************************************************
 *		export_compound_text
 *
 *  Export CF_UNICODE to COMPOUND_TEXT
 */
static BOOL export_compound_text( Display *display, Window win, Atom prop, Atom target,
                                  void *data, size_t size )
{
    XTextProperty textprop;
    XICCEncodingStyle style;
    DWORD len;
    char *text;


    if (!(text = malloc( size / sizeof(WCHAR) * 3 ))) return FALSE;
    len = ntdll_wcstoumbs( data, size / sizeof(WCHAR), text, size / sizeof(WCHAR) * 3, FALSE );
    string_from_unicode_text( text, len, &len );

    if (target == x11drv_atom(COMPOUND_TEXT))
        style = XCompoundTextStyle;
    else
        style = XStdICCTextStyle;

    /* Update the X property */
    if (XmbTextListToTextProperty( display, &text, 1, style, &textprop ) == Success)
    {
        XSetTextProperty( display, win, &textprop, prop );
        XFree( textprop.value );
    }

    free( text );
    return TRUE;
}


/**************************************************************************
 *		export_pixmap
 *
 *  Export CF_DIB to XA_PIXMAP.
 */
static BOOL export_pixmap( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    Pixmap pixmap;
    BITMAPINFO *pbmi = data;
    struct gdi_image_bits bits;

    bits.ptr = (LPBYTE)pbmi + bitmap_info_size( pbmi, DIB_RGB_COLORS );
    bits.free = NULL;
    bits.is_copy = FALSE;
    pixmap = create_pixmap_from_image( 0, &default_visual, pbmi, &bits, DIB_RGB_COLORS );

    put_property( display, win, prop, target, 32, &pixmap, 1 );
    /* FIXME: free the pixmap when the property is deleted */
    return TRUE;
}


/**************************************************************************
 *		export_image_bmp
 *
 *  Export CF_DIB to image/bmp.
 */
static BOOL export_image_bmp( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    LPBYTE dibdata = data;
    UINT bmpsize;
    BITMAPFILEHEADER *bfh;

    bmpsize = sizeof(BITMAPFILEHEADER) + size;
    bfh = malloc( bmpsize );
    if (bfh)
    {
        /* bitmap file header */
        bfh->bfType = 0x4d42; /* "BM" */
        bfh->bfSize = bmpsize;
        bfh->bfReserved1 = 0;
        bfh->bfReserved2 = 0;
        bfh->bfOffBits = sizeof(BITMAPFILEHEADER) + bitmap_info_size((BITMAPINFO*)dibdata, DIB_RGB_COLORS);

        /* rest of bitmap is the same as the packed dib */
        memcpy(bfh+1, dibdata, bmpsize-sizeof(BITMAPFILEHEADER));
    }
    put_property( display, win, prop, target, 8, bfh, bmpsize );
    free( bfh );
    return TRUE;
}


/**************************************************************************
 *		export_text_html
 *
 *  Export HTML Format to text/html.
 *
 * FIXME: We should attempt to add an <a base> tag and convert windows paths.
 */
static BOOL export_text_html( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    const char *p;
    UINT start = 0, end = 0;
    BOOL ret = TRUE;

    p = data;
    while (*p && *p != '<')
    {
        if (!strncmp( p, "StartFragment:", 14 )) start = atoi( p + 14 );
        else if (!strncmp( p, "EndFragment:", 12 )) end = atoi( p + 12 );
        if (!(p = strpbrk( p, "\r\n" ))) break;
        while (*p == '\r' || *p == '\n') p++;
    }
    if (start && start < end && end <= size)
        put_property( display, win, prop, target, 8, (char *)data + start, end - start );
    else
        ret = FALSE;

    return ret;
}


/**************************************************************************
 *      export_hdrop
 *
 *  Export CF_HDROP format to text/uri-list.
 */
static BOOL export_hdrop( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    char *textUriList = NULL;
    UINT textUriListSize = 32;
    UINT next = 0;
    const WCHAR *ptr;
    WCHAR *unicode_data = NULL;
    DROPFILES *drop_files = data;

    if (!drop_files->fWide)
    {
        char *files = (char *)data + drop_files->pFiles;
        CPTABLEINFO *cp = get_ansi_cp();
        DWORD len = 0;

        while (files[len]) len += strlen( files + len ) + 1;
        len++;

        if (!(ptr = unicode_data = malloc( len * sizeof(WCHAR) ))) goto failed;

        if (cp->CodePage == CP_UTF8)
            RtlUTF8ToUnicodeN( unicode_data, len * sizeof(WCHAR), &len, files, len );
        else
            RtlCustomCPToUnicodeN( cp, unicode_data, len * sizeof(WCHAR), &len, files, len );
    }
    else ptr = (const WCHAR *)((char *)data + drop_files->pFiles);

    if (!(textUriList = malloc( textUriListSize ))) goto failed;

    while (*ptr)
    {
        char *unixFilename = NULL;
        UINT uriSize;
        UINT u;

        unixFilename = get_unix_file_name( ptr );
        if (unixFilename == NULL) goto failed;
        ptr += lstrlenW( ptr ) + 1;

        uriSize = 8 + /* file:/// */
                3 * (lstrlenA(unixFilename) - 1) + /* "%xy" per char except first '/' */
                2; /* \r\n */
        if ((next + uriSize) > textUriListSize)
        {
            UINT biggerSize = max( 2 * textUriListSize, next + uriSize );
            void *bigger = realloc( textUriList, biggerSize );
            if (bigger)
            {
                textUriList = bigger;
                textUriListSize = biggerSize;
            }
            else
            {
                free( unixFilename );
                goto failed;
            }
        }
        lstrcpyA(&textUriList[next], "file:///");
        next += 8;
        /* URL encode everything - unnecessary, but easier/lighter than linking in shlwapi, and can't hurt */
        for (u = 1; unixFilename[u]; u++)
        {
            static const char hex_table[] = "0123456789abcdef";
            textUriList[next++] = '%';
            textUriList[next++] = hex_table[(unsigned char)unixFilename[u] >> 4];
            textUriList[next++] = hex_table[unixFilename[u] & 0xf];
        }
        textUriList[next++] = '\r';
        textUriList[next++] = '\n';
        free( unixFilename );
    }
    put_property( display, win, prop, target, 8, textUriList, next );
    free( textUriList );
    return TRUE;

failed:
    free( unicode_data );
    free( textUriList );
    return FALSE;
}


/***********************************************************************
 *           get_clipboard_formats
 *
 * Return a list of all formats currently available on the Win32 clipboard.
 * Helper for export_targets.
 */
static UINT *get_clipboard_formats( UINT *size )
{
    UINT *ids;

    *size = 256;
    for (;;)
    {
        if (!(ids = malloc( *size * sizeof(*ids) ))) return NULL;
        if (NtUserGetUpdatedClipboardFormats( ids, *size, size )) break;
        free( ids );
        if (RtlGetLastWin32Error() != ERROR_INSUFFICIENT_BUFFER) return NULL;
    }
    register_win32_formats( ids, *size );
    return ids;
}


/***********************************************************************
 *           is_format_available
 *
 * Check if a clipboard format is included in the list.
 * Helper for export_targets.
 */
static BOOL is_format_available( UINT format, const UINT *ids, unsigned int count )
{
    while (count--) if (*ids++ == format) return TRUE;
    return FALSE;
}


/***********************************************************************
 *           export_targets
 *
 *  Service a TARGETS selection request event
 */
static BOOL export_targets( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    struct clipboard_format *format;
    UINT pos, count, *formats;
    Atom *targets;

    if (!(formats = get_clipboard_formats( &count ))) return FALSE;

    /* the builtin formats contain duplicates, so allocate some extra space */
    if (!(targets = malloc( (count + ARRAY_SIZE(builtin_formats)) * sizeof(*targets) )))
    {
        free( formats );
        return FALSE;
    }

    pos = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, struct clipboard_format, entry )
    {
        if (!format->export) continue;
        /* formats with id==0 are always exported */
        if (format->id && !is_format_available( format->id, formats, count )) continue;
        TRACE( "%d: %s -> %s\n", pos, debugstr_format( format->id ), debugstr_xatom( format->atom ));
        targets[pos++] = format->atom;
    }

    put_property( display, win, prop, XA_ATOM, 32, targets, pos );
    free( targets );
    free( formats );
    return TRUE;
}


/**************************************************************************
 *      export_selection
 *
 * Export selection data, depending on the target type.
 */
static BOOL export_selection( Display *display, Window win, Atom prop, Atom target )
{
    struct get_clipboard_params params = { .data_only = TRUE };
    struct clipboard_format *format;
    BOOL open = FALSE, ret = FALSE;
    size_t buffer_size = 0;

    LIST_FOR_EACH_ENTRY( format, &format_list, struct clipboard_format, entry )
    {
        if (format->atom != target) continue;
        if (!format->export) continue;
        if (!format->id)
        {
            TRACE( "win %lx prop %s target %s\n", win, debugstr_xatom( prop ), debugstr_xatom( target ));
            ret = format->export( display, win, prop, target, NULL, 0 );
            break;
        }
        if (!open && !(open = NtUserOpenClipboard( clipboard_hwnd, 0 )))
        {
            ERR( "failed to open clipboard for %s\n", debugstr_xatom( target ));
            return FALSE;
        }

        if (!buffer_size)
        {
            buffer_size = 1024;
            if (!(params.data = malloc( buffer_size ))) break;
        }

        for (;;)
        {
            params.size = buffer_size;
            if (NtUserGetClipboardData( format->id, &params ))
            {
                TRACE( "win %lx prop %s target %s exporting %s\n",
                       win, debugstr_xatom( prop ), debugstr_xatom( target ),
                       debugstr_format( format->id ) );

                ret = format->export( display, win, prop, target, params.data, params.size );
                goto done;
            }
            if (!params.data_size) break;
            free( params.data );
            if (!(params.data = malloc( params.data_size ))) goto done;
            buffer_size = params.data_size;
            params.data_size = 0;
        }
        /* keep looking for another Win32 format mapping to the same target */
    }
done:
    free( params.data );
    if (open) NtUserCloseClipboard();
    return ret;
}


/***********************************************************************
 *           export_multiple
 *
 *  Service a MULTIPLE selection request event
 *  prop contains a list of (target,property) atom pairs.
 *  The first atom names a target and the second names a property.
 *  The effect is as if we have received a sequence of SelectionRequest events
 *  (one for each atom pair) except that:
 *  1. We reply with a SelectionNotify only when all the requested conversions
 *  have been performed.
 *  2. If we fail to convert the target named by an atom in the MULTIPLE property,
 *  we replace the atom in the property by None.
 */
static BOOL export_multiple( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    Atom atype;
    int aformat;
    Atom *list;
    unsigned long i, count, failed, remain;

    /* Read the MULTIPLE property contents. This should contain a list of
     * (target,property) atom pairs.
     */
    if (XGetWindowProperty( display, win, prop, 0, 0x3FFF, False, AnyPropertyType, &atype, &aformat,
                            &count, &remain, (unsigned char**)&list ))
        return FALSE;

    TRACE( "type %s format %d count %ld remain %ld\n",
           debugstr_xatom( atype ), aformat, count, remain );

    /*
     * Make sure we got what we expect.
     * NOTE: According to the X-ICCCM Version 2.0 documentation the property sent
     * in a MULTIPLE selection request should be of type ATOM_PAIR.
     * However some X apps(such as XPaint) are not compliant with this and return
     * a user defined atom in atype when XGetWindowProperty is called.
     * The data *is* an atom pair but is not denoted as such.
     */
    if (aformat == 32 /* atype == xAtomPair */ )
    {
        for (i = failed = 0; i < count; i += 2)
        {
            if (list[i+1] == None) continue;
            if (export_selection( display, win, list[i + 1], list[i] )) continue;
            failed++;
            list[i + 1] = None;
        }
        if (failed) put_property( display, win, prop, atype, 32, list, count );
    }
    XFree( list );
    return TRUE;
}


/***********************************************************************
 *           export_timestamp
 *
 * Export the timestamp that was used to acquire the selection
 */
static BOOL export_timestamp( Display *display, Window win, Atom prop, Atom target, void *data, size_t size )
{
    Time time = CurrentTime;  /* FIXME */
    put_property( display, win, prop, XA_INTEGER, 32, &time, 1 );
    return TRUE;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_GetProperty
 *  Gets type, data and size.
 */
static BOOL X11DRV_CLIPBOARD_GetProperty(Display *display, Window w, Atom prop,
    Atom *atype, unsigned char **data, size_t *datasize)
{
    int aformat;
    unsigned long pos = 0, nitems, remain, count;
    unsigned char *val = NULL, *new_val, *buffer;

    for (;;)
    {
        if (XGetWindowProperty(display, w, prop, pos, INT_MAX / 4, False,
                               AnyPropertyType, atype, &aformat, &nitems, &remain, &buffer))
        {
            WARN("Failed to read property\n");
            free( val );
            return FALSE;
        }

        count = get_property_size( aformat, nitems );
        if (!(new_val = realloc( val, pos * sizeof(int) + count + 1 )))
        {
            XFree( buffer );
            free( val );
            return FALSE;
        }
        val = new_val;
        memcpy( (int *)val + pos, buffer, count );
        XFree( buffer );
        if (!remain)
        {
            *datasize = pos * sizeof(int) + count;
            val[*datasize] = 0;
            break;
        }
        pos += count / sizeof(int);
    }

    TRACE( "got property %s type %s format %u len %zu from window %lx\n",
           debugstr_xatom( prop ), debugstr_xatom( *atype ), aformat, *datasize, w );

    /* Delete the property on the window now that we are done
     * This will send a PropertyNotify event to the selection owner. */
    XDeleteProperty(display, w, prop);
    *data = val;
    return TRUE;
}


struct clipboard_data_packet {
    struct list entry;
    unsigned long size;
    unsigned char *data;
};

/**************************************************************************
 *		read_property
 *
 *  Reads the contents of the X selection property.
 */
static BOOL read_property( Display *display, Window w, Atom prop,
                           Atom *type, unsigned char **data, size_t *datasize )
{
    XEvent xe;

    if (prop == None)
        return FALSE;

    while (XCheckTypedWindowEvent(display, w, PropertyNotify, &xe))
        ;

    if (!X11DRV_CLIPBOARD_GetProperty(display, w, prop, type, data, datasize))
        return FALSE;

    if (*type == x11drv_atom(INCR))
    {
        unsigned char *buf;
        unsigned long bufsize = 0;
        struct list packets;
        struct clipboard_data_packet *packet, *packet2;
        BOOL res;

        free( *data );
        *data = NULL;

        list_init(&packets);

        for (;;)
        {
            int i;
            unsigned char *prop_data;
            size_t prop_size;

            /* Wait until PropertyNotify is received */
            for (i = 0; i < SELECTION_RETRIES; i++)
            {
                Bool res;

                res = XCheckTypedWindowEvent(display, w, PropertyNotify, &xe);
                if (res && xe.xproperty.atom == prop &&
                    xe.xproperty.state == PropertyNewValue)
                    break;
                selection_sleep();
            }

            if (i >= SELECTION_RETRIES ||
                !X11DRV_CLIPBOARD_GetProperty(display, w, prop, type, &prop_data, &prop_size))
            {
                res = FALSE;
                break;
            }

            /* Retrieved entire data. */
            if (prop_size == 0)
            {
                free( prop_data );
                res = TRUE;
                break;
            }

            packet = malloc( sizeof(*packet) );
            if (!packet)
            {
                free( prop_data );
                res = FALSE;
                break;
            }

            packet->size = prop_size;
            packet->data = prop_data;
            list_add_tail(&packets, &packet->entry);
            bufsize += prop_size;
        }

        if (res)
        {
            buf = malloc( bufsize + 1 );
            if (buf)
            {
                unsigned long bytes_copied = 0;
                *datasize = bufsize;
                LIST_FOR_EACH_ENTRY( packet, &packets, struct clipboard_data_packet, entry)
                {
                    memcpy(&buf[bytes_copied], packet->data, packet->size);
                    bytes_copied += packet->size;
                }
                buf[bufsize] = 0;
                *data = buf;
            }
            else
                res = FALSE;
        }

        LIST_FOR_EACH_ENTRY_SAFE( packet, packet2, &packets, struct clipboard_data_packet, entry)
        {
            free( packet->data );
            free( packet );
        }

        return res;
    }

    return TRUE;
}


/**************************************************************************
 *		acquire_selection
 *
 * Acquire the X11 selection when the Win32 clipboard has changed.
 */
static void acquire_selection( Display *display )
{
    if (selection_window) XDestroyWindow( display, selection_window );

    selection_window = XCreateWindow( display, root_window, 0, 0, 1, 1, 0, CopyFromParent,
                                      InputOutput, CopyFromParent, 0, NULL );
    if (!selection_window) return;

    XSetSelectionOwner( display, x11drv_atom(CLIPBOARD), selection_window, CurrentTime );
    if (use_primary_selection) XSetSelectionOwner( display, XA_PRIMARY, selection_window, CurrentTime );
    TRACE( "win %lx\n", selection_window );
}


/**************************************************************************
 *		release_selection
 *
 * Release the X11 selection when some other X11 app has grabbed it.
 */
static void release_selection( Display *display, Time time )
{
    assert( selection_window );

    TRACE( "win %lx\n", selection_window );

    /* release PRIMARY if we still own it */
    if (use_primary_selection && XGetSelectionOwner( display, XA_PRIMARY ) == selection_window)
        XSetSelectionOwner( display, XA_PRIMARY, None, time );

    XDestroyWindow( display, selection_window );
    selection_window = 0;
}


/**************************************************************************
 *		request_selection_contents
 *
 * Retrieve the contents of the X11 selection when it's owned by an X11 app.
 */
static BOOL request_selection_contents( Display *display, BOOL changed )
{
    struct clipboard_format *targets = find_x11_format( x11drv_atom(TARGETS) );
    struct clipboard_format *string = find_x11_format( XA_STRING );
    struct clipboard_format *format = NULL;
    Window owner = 0;
    unsigned char *data = NULL;
    size_t import_size, size = 0;
    Atom type = 0;

    static Atom last_selection;
    static Window last_owner;
    static struct clipboard_format *last_format;
    static Atom last_type;
    static unsigned char *last_data;
    static unsigned long last_size;

    assert( targets );
    assert( string );

    current_selection = 0;
    if (use_primary_selection)
    {
        if ((owner = XGetSelectionOwner( display, XA_PRIMARY )))
            current_selection = XA_PRIMARY;
    }
    if (!current_selection)
    {
        if ((owner = XGetSelectionOwner( display, x11drv_atom(CLIPBOARD) )))
            current_selection = x11drv_atom(CLIPBOARD);
    }

    if (current_selection)
    {
        if (convert_selection( display, import_window, current_selection, targets, &type, &data, &size ))
            format = targets;
        else if (convert_selection( display, import_window, current_selection, string, &type, &data, &size ))
            format = string;
    }

    changed = (changed ||
               rendered_formats ||
               last_selection != current_selection ||
               last_owner != owner ||
               last_format != format ||
               last_type != type ||
               last_size != size ||
               memcmp( last_data, data, size ));

    if (!changed || !NtUserOpenClipboard( clipboard_hwnd, 0 ))
    {
        free( data );
        return FALSE;
    }

    TRACE( "selection changed, importing\n" );
    NtUserEmptyClipboard();
    is_clipboard_owner = TRUE;
    rendered_formats = 0;

    if (format) format->import( type, data, size, &import_size );

    free( last_data );
    last_selection = current_selection;
    last_owner = owner;
    last_format = format;
    last_type = type;
    last_data = data;
    last_size = size;
    last_clipboard_update = NtGetTickCount();
    NtUserCloseClipboard();
    if (!use_xfixes)
        NtUserSetTimer( clipboard_hwnd, 1, SELECTION_UPDATE_DELAY, NULL, TIMERV_DEFAULT_COALESCING );
    return TRUE;
}


/**************************************************************************
 *		update_clipboard
 *
 * Periodically update the clipboard while the selection is owned by an X11 app.
 */
BOOL update_clipboard( HWND hwnd )
{
    if (use_xfixes) return TRUE;
    if (hwnd != clipboard_hwnd) return TRUE;
    if (!is_clipboard_owner) return TRUE;
    if (NtGetTickCount() - last_clipboard_update <= SELECTION_UPDATE_DELAY) return TRUE;
    return request_selection_contents( thread_display(), FALSE );
}


/**************************************************************************
 *              selection_notify_event
 *
 * Called when x11 clipboard content changes
 */
#ifdef SONAME_LIBXFIXES
static BOOL selection_notify_event( HWND hwnd, XEvent *event )
{
    XFixesSelectionNotifyEvent *req = (XFixesSelectionNotifyEvent*)event;

    if (!is_clipboard_owner) return FALSE;
    if (req->owner == selection_window) return FALSE;
    request_selection_contents( req->display, TRUE );
    return FALSE;
}
#endif

/**************************************************************************
 *		xfixes_init
 *
 * Initialize xfixes to receive clipboard update notifications
 */
static void xfixes_init(void)
{
#ifdef SONAME_LIBXFIXES
    typeof(XFixesSelectSelectionInput) *pXFixesSelectSelectionInput;
    typeof(XFixesQueryExtension) *pXFixesQueryExtension;
    typeof(XFixesQueryVersion) *pXFixesQueryVersion;

    int event_base, error_base;
    int major = 3, minor = 0;
    void *handle;

    handle = dlopen(SONAME_LIBXFIXES, RTLD_NOW);
    if (!handle) return;

    pXFixesQueryExtension = dlsym(handle, "XFixesQueryExtension");
    if (!pXFixesQueryExtension) return;
    pXFixesQueryVersion = dlsym(handle, "XFixesQueryVersion");
    if (!pXFixesQueryVersion) return;
    pXFixesSelectSelectionInput = dlsym(handle, "XFixesSelectSelectionInput");
    if (!pXFixesSelectSelectionInput) return;

    if (!pXFixesQueryExtension(clipboard_display, &event_base, &error_base))
        return;
    pXFixesQueryVersion(clipboard_display, &major, &minor);
    use_xfixes = (major >= 1);
    if (!use_xfixes) return;

    pXFixesSelectSelectionInput(clipboard_display, import_window, x11drv_atom(CLIPBOARD),
            XFixesSetSelectionOwnerNotifyMask |
            XFixesSelectionWindowDestroyNotifyMask |
            XFixesSelectionClientCloseNotifyMask);
    if (use_primary_selection)
    {
        pXFixesSelectSelectionInput(clipboard_display, import_window, XA_PRIMARY,
                XFixesSetSelectionOwnerNotifyMask |
                XFixesSelectionWindowDestroyNotifyMask |
                XFixesSelectionClientCloseNotifyMask);
    }
    X11DRV_register_event_handler(event_base + XFixesSelectionNotify,
            selection_notify_event, "XFixesSelectionNotify");
    TRACE("xfixes succesully initialized\n");
#else
    WARN("xfixes not supported\n");
#endif
}


/**************************************************************************
 *		clipboard_init
 *
 * Thread running inside the desktop process to manage the clipboard
 */
static BOOL clipboard_init( HWND hwnd )
{
    XSetWindowAttributes attr;

    clipboard_hwnd = hwnd;
    clipboard_display = thread_init_display();
    attr.event_mask = PropertyChangeMask;
    import_window = XCreateWindow( clipboard_display, root_window, 0, 0, 1, 1, 0, CopyFromParent,
                                   InputOutput, CopyFromParent, CWEventMask, &attr );
    if (!import_window)
    {
        ERR( "failed to create import window\n" );
        return FALSE;
    }

    clipboard_thread_id = GetCurrentThreadId();
    NtUserAddClipboardFormatListener( hwnd );
    register_builtin_formats();
    xfixes_init();
    request_selection_contents( clipboard_display, TRUE );

    TRACE( "clipboard thread running\n" );
    return TRUE;
}




/**************************************************************************
 *           x11drv_clipboard_message
 */
LRESULT X11DRV_ClipboardWindowProc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam )
{
    switch (msg)
    {
    case WM_NCCREATE:
        return clipboard_init( hwnd );
    case WM_CLIPBOARDUPDATE:
        if (is_clipboard_owner) break;  /* ignore our own changes */
        acquire_selection( thread_init_display() );
        break;
    case WM_RENDERFORMAT:
        if (render_format( wparam )) rendered_formats++;
        break;
    case WM_TIMER:
        if (!is_clipboard_owner) break;
        request_selection_contents( thread_display(), FALSE );
        break;
    case WM_DESTROYCLIPBOARD:
        TRACE( "WM_DESTROYCLIPBOARD: lost ownership\n" );
        is_clipboard_owner = FALSE;
        NtUserKillTimer( hwnd, 1 );
        break;
    }

    return NtUserMessageCall( hwnd, msg, wparam, lparam, NULL, NtUserDefWindowProc, FALSE );
}


/**************************************************************************
 *		X11DRV_UpdateClipboard
 */
void X11DRV_UpdateClipboard(void)
{
    static ULONG last_update;
    ULONG now;
    DWORD_PTR ret;

    if (use_xfixes) return;
    if (GetCurrentThreadId() == clipboard_thread_id) return;
    now = NtGetTickCount();
    if ((int)(now - last_update) <= SELECTION_UPDATE_DELAY) return;
    if (send_message_timeout( NtUserGetClipboardOwner(), WM_X11DRV_UPDATE_CLIPBOARD, 0, 0,
                              SMTO_ABORTIFHUNG, 5000, &ret ) && ret)
        last_update = now;
}


/***********************************************************************
 *           X11DRV_HandleSelectionRequest
 */
BOOL X11DRV_SelectionRequest( HWND hwnd, XEvent *xev )
{
    XSelectionRequestEvent *event = &xev->xselectionrequest;
    Display *display = event->display;
    XEvent result;
    Atom rprop = None;

    TRACE( "got request on %lx for selection %s target %s win %lx prop %s\n",
           event->owner, debugstr_xatom( event->selection ), debugstr_xatom( event->target ),
           event->requestor, debugstr_xatom( event->property ));

    if (event->owner != selection_window) goto done;
    if ((event->selection != x11drv_atom(CLIPBOARD)) &&
        (!use_primary_selection || event->selection != XA_PRIMARY)) goto done;

    /* If the specified property is None the requestor is an obsolete client.
     * We support these by using the specified target atom as the reply property.
     */
    rprop = event->property;
    if( rprop == None )
        rprop = event->target;

    if (!export_selection( display, event->requestor, rprop, event->target ))
        rprop = None;  /* report failure to client */

done:
    result.xselection.type = SelectionNotify;
    result.xselection.display = display;
    result.xselection.requestor = event->requestor;
    result.xselection.selection = event->selection;
    result.xselection.property = rprop;
    result.xselection.target = event->target;
    result.xselection.time = event->time;
    TRACE( "sending SelectionNotify for %s to %lx\n", debugstr_xatom( rprop ), event->requestor );
    XSendEvent( display, event->requestor, False, NoEventMask, &result );
    return FALSE;
}


/***********************************************************************
 *           X11DRV_SelectionClear
 */
BOOL X11DRV_SelectionClear( HWND hwnd, XEvent *xev )
{
    XSelectionClearEvent *event = &xev->xselectionclear;

    if (event->window != selection_window) return FALSE;
    if (event->selection != x11drv_atom(CLIPBOARD)) return FALSE;

    release_selection( event->display, event->time );
    request_selection_contents( event->display, TRUE );
    return FALSE;
}
