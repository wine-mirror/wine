/*
 * X11 clipboard windows driver
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
 *           2003 Ulrich Czekalla for CodeWeavers
 *           2014 Damjan Jovanovic
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

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <limits.h>
#include <time.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "shlobj.h"
#include "shellapi.h"
#include "shlwapi.h"
#include "x11drv.h"
#include "wine/list.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

/* Maximum wait time for selection notify */
#define SELECTION_RETRIES 500  /* wait for .5 seconds */
#define SELECTION_WAIT    1000 /* us */

/* Selection masks */
#define S_NOSELECTION    0
#define S_PRIMARY        1
#define S_CLIPBOARD      2

struct tagWINE_CLIPDATA; /* Forward */

typedef BOOL (*EXPORTFUNC)( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
typedef HANDLE (*IMPORTFUNC)( Atom type, const void *data, size_t size );

typedef struct clipboard_format
{
    struct list entry;
    UINT        id;
    Atom        atom;
    IMPORTFUNC  import;
    EXPORTFUNC  export;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

typedef struct tagWINE_CLIPDATA {
    struct list entry;
    UINT        wFormatID;
    HANDLE      hData;
    UINT        drvData;
    LPWINE_CLIPFORMAT lpFormat;
} WINE_CLIPDATA, *LPWINE_CLIPDATA;

static int selectionAcquired = 0;              /* Contains the current selection masks */
static Window selectionWindow = None;          /* The top level X window which owns the selection */
static Atom selectionCacheSrc = XA_PRIMARY;    /* The selection source from which the clipboard cache was filled */

static HANDLE import_data( Atom type, const void *data, size_t size );
static HANDLE import_enhmetafile( Atom type, const void *data, size_t size );
static HANDLE import_metafile( Atom type, const void *data, size_t size );
static HANDLE import_pixmap( Atom type, const void *data, size_t size );
static HANDLE import_image_bmp( Atom type, const void *data, size_t size );
static HANDLE import_string( Atom type, const void *data, size_t size );
static HANDLE import_utf8_string( Atom type, const void *data, size_t size );
static HANDLE import_compound_text( Atom type, const void *data, size_t size );
static HANDLE import_text_uri_list( Atom type, const void *data, size_t size );

static BOOL export_data( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_string( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_utf8_string( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_compound_text( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_pixmap( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_image_bmp( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_metafile( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_enhmetafile( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_text_html( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_hdrop( Display *display, Window win, Atom prop, Atom target, HANDLE handle );
static BOOL export_multiple( Display *display, Window win, Atom prop, Atom target, HANDLE handle );

static void X11DRV_CLIPBOARD_FreeData(LPWINE_CLIPDATA lpData);
static int X11DRV_CLIPBOARD_QueryAvailableData(Display *display);
static BOOL X11DRV_CLIPBOARD_ReadSelectionData(Display *display, LPWINE_CLIPDATA lpData);
static BOOL X11DRV_CLIPBOARD_ReadProperty( Display *display, Window w, Atom prop,
                                           Atom *type, unsigned char **data, unsigned long *datasize );
static BOOL X11DRV_CLIPBOARD_RenderFormat(Display *display, LPWINE_CLIPDATA lpData);
static void empty_clipboard(void);

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
    { 0, CF_TEXT,            XA_STRING,                 import_string,        export_string },
    { 0, CF_TEXT,            XATOM_text_plain,          import_string,        export_string },
    { 0, CF_BITMAP,          XATOM_WCF_BITMAP,          import_data,          NULL },
    { 0, CF_METAFILEPICT,    XATOM_WCF_METAFILEPICT,    import_metafile,      export_metafile },
    { 0, CF_SYLK,            XATOM_WCF_SYLK,            import_data,          export_data },
    { 0, CF_DIF,             XATOM_WCF_DIF,             import_data,          export_data },
    { 0, CF_TIFF,            XATOM_WCF_TIFF,            import_data,          export_data },
    { 0, CF_OEMTEXT,         XATOM_WCF_OEMTEXT,         import_data,          export_data },
    { 0, CF_DIB,             XA_PIXMAP,                 import_pixmap,        export_pixmap },
    { 0, CF_PALETTE,         XATOM_WCF_PALETTE,         import_data,          export_data },
    { 0, CF_PENDATA,         XATOM_WCF_PENDATA,         import_data,          export_data },
    { 0, CF_RIFF,            XATOM_WCF_RIFF,            import_data,          export_data },
    { 0, CF_WAVE,            XATOM_WCF_WAVE,            import_data,          export_data },
    { 0, CF_UNICODETEXT,     XATOM_UTF8_STRING,         import_utf8_string,   export_utf8_string },
    { 0, CF_UNICODETEXT,     XATOM_COMPOUND_TEXT,       import_compound_text, export_compound_text },
    { 0, CF_ENHMETAFILE,     XATOM_WCF_ENHMETAFILE,     import_enhmetafile,   export_enhmetafile },
    { 0, CF_HDROP,           XATOM_text_uri_list,       import_text_uri_list, export_hdrop },
    { 0, CF_LOCALE,          XATOM_WCF_LOCALE,          import_data,          export_data },
    { 0, CF_DIBV5,           XATOM_WCF_DIBV5,           import_data,          export_data },
    { 0, CF_OWNERDISPLAY,    XATOM_WCF_OWNERDISPLAY,    import_data,          export_data },
    { 0, CF_DSPTEXT,         XATOM_WCF_DSPTEXT,         import_data,          export_data },
    { 0, CF_DSPBITMAP,       XATOM_WCF_DSPBITMAP,       import_data,          export_data },
    { 0, CF_DSPMETAFILEPICT, XATOM_WCF_DSPMETAFILEPICT, import_data,          export_data },
    { 0, CF_DSPENHMETAFILE,  XATOM_WCF_DSPENHMETAFILE,  import_data,          export_data },
    { 0, CF_DIB,             XATOM_image_bmp,           import_image_bmp,     export_image_bmp },
    { RichTextFormatW, 0,    XATOM_text_rtf,            import_data,          export_data },
    { RichTextFormatW, 0,    XATOM_text_richtext,       import_data,          export_data },
    { GIFW, 0,               XATOM_image_gif,           import_data,          export_data },
    { JFIFW, 0,              XATOM_image_jpeg,          import_data,          export_data },
    { PNGW, 0,               XATOM_image_png,           import_data,          export_data },
    { HTMLFormatW, 0,        XATOM_HTML_Format,         import_data,          export_data },
    { HTMLFormatW, 0,        XATOM_text_html,           import_data,          export_text_html },
    { 0, 0,                  XATOM_MULTIPLE,            NULL,                 export_multiple },
};

static struct list format_list = LIST_INIT( format_list );

#define GET_ATOM(prop)  (((prop) < FIRST_XATOM) ? (Atom)(prop) : X11DRV_Atoms[(prop) - FIRST_XATOM])


/*
 * Cached clipboard data.
 */
static struct list data_list = LIST_INIT( data_list );
static UINT ClipDataCount = 0;

/*
 * Clipboard sequence number
 */
static UINT wSeqNo = 0;

/**************************************************************************
 *                Internal Clipboard implementation methods
 **************************************************************************/

static Window thread_selection_wnd(void)
{
    struct x11drv_thread_data *thread_data = x11drv_init_thread_data();
    Window w = thread_data->selection_wnd;

    if (!w)
    {
        w = XCreateWindow(thread_data->display, root_window, 0, 0, 1, 1, 0, CopyFromParent,
                          InputOnly, CopyFromParent, 0, NULL);
        if (w)
        {
            thread_data->selection_wnd = w;

            XSelectInput(thread_data->display, w, PropertyChangeMask);
        }
        else
            FIXME("Failed to create window. Fetching selection data will fail.\n");
    }

    return w;
}

static const char *debugstr_format( UINT id )
{
    WCHAR buffer[256];

    if (GetClipboardFormatNameW( id, buffer, 256 ))
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

/**************************************************************************
 *		X11DRV_InitClipboard
 */
void X11DRV_InitClipboard(void)
{
    static const unsigned int count = sizeof(builtin_formats) / sizeof(builtin_formats[0]);
    struct clipboard_format *formats;
    UINT i;

    if (!(formats = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*formats)))) return;

    for (i = 0; i < count; i++)
    {
        if (builtin_formats[i].name)
            formats[i].id = RegisterClipboardFormatW( builtin_formats[i].name );
        else
            formats[i].id = builtin_formats[i].id;

        formats[i].atom   = GET_ATOM(builtin_formats[i].data);
        formats[i].import = builtin_formats[i].import;
        formats[i].export = builtin_formats[i].export;
        list_add_tail( &format_list, &formats[i].entry );
    }
}


/**************************************************************************
 *                intern_atoms
 *
 * Intern atoms for formats that don't have one yet.
 */
static void intern_atoms(void)
{
    LPWINE_CLIPFORMAT format;
    int i, count, len;
    char **names;
    Atom *atoms;
    Display *display;
    WCHAR buffer[256];

    count = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
        if (!format->atom) count++;
    if (!count) return;

    display = thread_init_display();

    names = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*names) );
    atoms = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*atoms) );

    i = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
        if (!format->atom) {
            if (GetClipboardFormatNameW( format->id, buffer, 256 ) > 0)
            {
                /* use defined format name */
                len = WideCharToMultiByte(CP_UNIXCP, 0, buffer, -1, NULL, 0, NULL, NULL);
            }
            else
            {
                /* create a name in the same way as ntdll/atom.c:integral_atom_name
                 * which is normally used by GetClipboardFormatNameW
                 */
                static const WCHAR fmt[] = {'#','%','u',0};
                len = sprintfW(buffer, fmt, format->id) + 1;
            }
            names[i] = HeapAlloc(GetProcessHeap(), 0, len);
            WideCharToMultiByte(CP_UNIXCP, 0, buffer, -1, names[i++], len, NULL, NULL);
        }

    XInternAtoms( display, names, count, False, atoms );

    i = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
        if (!format->atom) {
            HeapFree(GetProcessHeap(), 0, names[i]);
            format->atom = atoms[i++];
        }

    HeapFree( GetProcessHeap(), 0, names );
    HeapFree( GetProcessHeap(), 0, atoms );
}


/**************************************************************************
 *		register_format
 *
 * Register a custom X clipboard format.
 */
static struct clipboard_format *register_format( UINT id, Atom prop )
{
    struct clipboard_format *format;

    /* walk format chain to see if it's already registered */
    LIST_FOR_EACH_ENTRY( format, &format_list, struct clipboard_format, entry )
        if (format->id == id) return format;

    if (!(format = HeapAlloc( GetProcessHeap(), 0, sizeof(*format) ))) return NULL;
    format->id     = id;
    format->atom   = prop;
    format->import = import_data;
    format->export = export_data;
    list_add_tail( &format_list, &format->entry );

    TRACE( "Registering format %s atom %ld\n", debugstr_format(id), prop );
    return format;
}


/**************************************************************************
 *                X11DRV_CLIPBOARD_LookupProperty
 */
static struct clipboard_format *X11DRV_CLIPBOARD_LookupProperty( struct clipboard_format *current, Atom prop )
{
    for (;;)
    {
        struct list *ptr = current ? &current->entry : &format_list;
        BOOL need_intern = FALSE;

        while ((ptr = list_next( &format_list, ptr )))
        {
            struct clipboard_format *format = LIST_ENTRY( ptr, struct clipboard_format, entry );
            if (format->atom == prop) return format;
            if (!format->atom) need_intern = TRUE;
        }
        if (!need_intern) return NULL;
        intern_atoms();
        /* restart the search for the new atoms */
    }
}


/**************************************************************************
 *               X11DRV_CLIPBOARD_LookupData
 */
static LPWINE_CLIPDATA X11DRV_CLIPBOARD_LookupData(DWORD wID)
{
    WINE_CLIPDATA *data;

    LIST_FOR_EACH_ENTRY( data, &data_list, WINE_CLIPDATA, entry )
        if (data->wFormatID == wID) return data;

    return NULL;
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_IsProcessOwner
 */
static BOOL X11DRV_CLIPBOARD_IsProcessOwner( HWND *owner )
{
    BOOL ret = FALSE;

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = 0;
        if (!wine_server_call_err( req ))
        {
            *owner = wine_server_ptr_handle( reply->old_owner );
            ret = (reply->flags & CB_PROCESS);
        }
    }
    SERVER_END_REQ;

    return ret;
}


/**************************************************************************
 *	X11DRV_CLIPBOARD_ReleaseOwnership
 */
static BOOL X11DRV_CLIPBOARD_ReleaseOwnership(void)
{
    BOOL ret;

    SERVER_START_REQ( set_clipboard_info )
    {
        req->flags = SET_CB_RELOWNER | SET_CB_SEQNO;
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;

    return ret;
}



/**************************************************************************
 *                      X11DRV_CLIPBOARD_InsertClipboardData
 *
 * Caller *must* have the clipboard open and be the owner.
 */
static BOOL X11DRV_CLIPBOARD_InsertClipboardData(UINT wFormatID, HANDLE hData,
                                                 LPWINE_CLIPFORMAT lpFormat, BOOL override)
{
    LPWINE_CLIPDATA lpData = X11DRV_CLIPBOARD_LookupData(wFormatID);

    TRACE("format=%04x lpData=%p hData=%p lpFormat=%p override=%d\n",
        wFormatID, lpData, hData, lpFormat, override);

    /* make sure the format exists */
    if (!lpFormat) register_format( wFormatID, 0 );

    if (lpData && !override)
        return TRUE;

    if (lpData)
    {
        X11DRV_CLIPBOARD_FreeData(lpData);

        lpData->hData = hData;
    }
    else
    {
        lpData = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_CLIPDATA));

        lpData->wFormatID = wFormatID;
        lpData->hData = hData;
        lpData->lpFormat = lpFormat;
        lpData->drvData = 0;

        list_add_tail( &data_list, &lpData->entry );
        ClipDataCount++;
    }

    return TRUE;
}


/**************************************************************************
 *			X11DRV_CLIPBOARD_FreeData
 *
 * Free clipboard data handle.
 */
static void X11DRV_CLIPBOARD_FreeData(LPWINE_CLIPDATA lpData)
{
    TRACE("%04x\n", lpData->wFormatID);

    if (!lpData->hData) return;

    switch (lpData->wFormatID)
    {
    case CF_BITMAP:
    case CF_DSPBITMAP:
    case CF_PALETTE:
        DeleteObject(lpData->hData);
        break;
    case CF_DIB:
        if (lpData->drvData) XFreePixmap(gdi_display, lpData->drvData);
        GlobalFree(lpData->hData);
        break;
    case CF_METAFILEPICT:
    case CF_DSPMETAFILEPICT:
        DeleteMetaFile(((METAFILEPICT *)GlobalLock( lpData->hData ))->hMF );
        GlobalFree(lpData->hData);
        break;
    case CF_ENHMETAFILE:
    case CF_DSPENHMETAFILE:
        DeleteEnhMetaFile(lpData->hData);
        break;
    default:
        GlobalFree(lpData->hData);
        break;
    }
    lpData->hData = 0;
    lpData->drvData = 0;
}


/**************************************************************************
 *			X11DRV_CLIPBOARD_UpdateCache
 */
static BOOL X11DRV_CLIPBOARD_UpdateCache(void)
{
    BOOL bret = TRUE;

    if (!selectionAcquired)
    {
        DWORD seqno = GetClipboardSequenceNumber();

        if (!seqno)
        {
            ERR("Failed to retrieve clipboard information.\n");
            bret = FALSE;
        }
        else if (wSeqNo < seqno)
        {
            empty_clipboard();

            if (X11DRV_CLIPBOARD_QueryAvailableData(thread_init_display()) < 0)
            {
                ERR("Failed to cache clipboard data owned by another process.\n");
                bret = FALSE;
            }
            wSeqNo = seqno;
        }
    }

    return bret;
}


/**************************************************************************
 *			X11DRV_CLIPBOARD_RenderFormat
 */
static BOOL X11DRV_CLIPBOARD_RenderFormat(Display *display, LPWINE_CLIPDATA lpData)
{
    BOOL bret = TRUE;

    TRACE(" 0x%04x hData(%p)\n", lpData->wFormatID, lpData->hData);

    if (lpData->hData) return bret; /* Already rendered */

    if (!selectionAcquired)
    {
        if (!X11DRV_CLIPBOARD_ReadSelectionData(display, lpData))
        {
            ERR("Failed to cache clipboard data owned by another process. Format=%04x\n",
                lpData->wFormatID);
            bret = FALSE;
        }
    }
    else
    {
        HWND owner = GetClipboardOwner();

        if (owner)
        {
            /* Send a WM_RENDERFORMAT message to notify the owner to render the
             * data requested into the clipboard.
             */
            TRACE("Sending WM_RENDERFORMAT message to hwnd(%p)\n", owner);
            SendMessageW(owner, WM_RENDERFORMAT, lpData->wFormatID, 0);

            if (!lpData->hData) bret = FALSE;
        }
        else
        {
            ERR("hWndClipOwner is lost!\n");
            bret = FALSE;
        }
    }

    return bret;
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


/**************************************************************************
 *		convert_selection
 */
static Atom convert_selection( Display *display, Window win, Atom selection, Atom target )
{
    int i;
    XEvent event;

    XConvertSelection( display, selection, target, x11drv_atom(SELECTION_DATA), win, CurrentTime );

    for (i = 0; i < SELECTION_RETRIES; i++)
    {
        Bool res = XCheckTypedWindowEvent( display, win, SelectionNotify, &event );
        if (res && event.xselection.selection == selection && event.xselection.target == target)
            return event.xselection.property;
        usleep( SELECTION_WAIT );
    }
    ERR( "Timed out waiting for SelectionNotify event\n" );
    return None;
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
static HGLOBAL create_dib_from_bitmap(HBITMAP hBmp)
{
    BITMAP bmp;
    HDC hdc;
    HGLOBAL hPackedDIB;
    LPBYTE pPackedDIB;
    LPBITMAPINFOHEADER pbmiHeader;
    unsigned int cDataSize, cPackedSize, OffsetBits;
    int nLinesCopied;

    if (!GetObjectW( hBmp, sizeof(bmp), &bmp )) return 0;

    /*
     * A packed DIB contains a BITMAPINFO structure followed immediately by
     * an optional color palette and the pixel data.
     */

    /* Calculate the size of the packed DIB */
    cDataSize = abs( bmp.bmHeight ) * (((bmp.bmWidth * bmp.bmBitsPixel + 31) / 8) & ~3);
    cPackedSize = sizeof(BITMAPINFOHEADER)
                  + ( (bmp.bmBitsPixel <= 8) ? (sizeof(RGBQUAD) * (1 << bmp.bmBitsPixel)) : 0 )
                  + cDataSize;
    /* Get the offset to the bits */
    OffsetBits = cPackedSize - cDataSize;

    /* Allocate the packed DIB */
    TRACE("\tAllocating packed DIB of size %d\n", cPackedSize);
    hPackedDIB = GlobalAlloc( GMEM_FIXED, cPackedSize );
    if ( !hPackedDIB )
    {
        WARN("Could not allocate packed DIB!\n");
        return 0;
    }

    /* A packed DIB starts with a BITMAPINFOHEADER */
    pPackedDIB = GlobalLock(hPackedDIB);
    pbmiHeader = (LPBITMAPINFOHEADER)pPackedDIB;

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
    hdc = GetDC( 0 );
    nLinesCopied = GetDIBits(hdc,                       /* Handle to device context */
                             hBmp,                      /* Handle to bitmap */
                             0,                         /* First scan line to set in dest bitmap */
                             bmp.bmHeight,              /* Number of scan lines to copy */
                             pPackedDIB + OffsetBits,   /* [out] Address of array for bitmap bits */
                             (LPBITMAPINFO) pbmiHeader, /* [out] Address of BITMAPINFO structure */
                             0);                        /* RGB or palette index */
    GlobalUnlock(hPackedDIB);
    ReleaseDC( 0, hdc );

    /* Cleanup if GetDIBits failed */
    if (nLinesCopied != bmp.bmHeight)
    {
        TRACE("\tGetDIBits returned %d. Actual lines=%d\n", nLinesCopied, bmp.bmHeight);
        GlobalFree(hPackedDIB);
        hPackedDIB = 0;
    }
    return hPackedDIB;
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
    char *uri = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strlen(encodedURI) + 1);
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
                HeapFree(GetProcessHeap(), 0, uri);
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
                ret = wine_get_dos_file_name(&uri[7]);
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
                        ret = wine_get_dos_file_name(path);
                    }
                    else if (gethostname(hostname, sizeof(hostname)) == 0)
                    {
                        if (strcmp(hostname, &uri[7]) == 0)
                        {
                            *path = '/';
                            ret = wine_get_dos_file_name(path);
                        }
                    }
                }
            }
        }
        else if (uri[6])
        {
            /* file:/path/to/file (konqueror) */
            ret = wine_get_dos_file_name(&uri[5]);
        }
    }
    HeapFree(GetProcessHeap(), 0, uri);
    return ret;
}


/**************************************************************************
 *		import_string
 *
 *  Import XA_STRING, converting the string to CF_TEXT.
 */
static HANDLE import_string( Atom type, const void *data, size_t size )
{
    const char *lpdata = data;
    LPSTR lpstr;
    size_t i, inlcount = 0;

    for (i = 0; i < size; i++)
    {
        if (lpdata[i] == '\n')
            inlcount++;
    }

    if ((lpstr = GlobalAlloc( GMEM_FIXED, size + inlcount + 1 )))
    {
        for (i = 0, inlcount = 0; i < size; i++)
        {
            if (lpdata[i] == '\n')
                lpstr[inlcount++] = '\r';

            lpstr[inlcount++] = lpdata[i];
        }
        lpstr[inlcount] = 0;
    }
    return lpstr;
}


/**************************************************************************
 *		import_utf8_string
 *
 *  Import XA_UTF8_STRING, converting the string to CF_UNICODE.
 */
static HANDLE import_utf8_string( Atom type, const void *data, size_t size )
{
    const char *lpdata = data;
    LPSTR lpstr;
    size_t i, inlcount = 0;
    WCHAR *textW = NULL;

    for (i = 0; i < size; i++)
    {
        if (lpdata[i] == '\n')
            inlcount++;
    }

    if ((lpstr = HeapAlloc( GetProcessHeap(), 0, size + inlcount + 1 )))
    {
        UINT count;

        for (i = 0, inlcount = 0; i < size; i++)
        {
            if (lpdata[i] == '\n')
                lpstr[inlcount++] = '\r';

            lpstr[inlcount++] = lpdata[i];
        }
        lpstr[inlcount] = 0;

        count = MultiByteToWideChar(CP_UTF8, 0, lpstr, -1, NULL, 0);
        textW = GlobalAlloc( GMEM_FIXED, count * sizeof(WCHAR) );
        if (textW) MultiByteToWideChar( CP_UTF8, 0, lpstr, -1, textW, count );
        HeapFree(GetProcessHeap(), 0, lpstr);
    }
    return textW;
}


/**************************************************************************
 *		import_compound_text
 *
 *  Import COMPOUND_TEXT to CF_UNICODE
 */
static HANDLE import_compound_text( Atom type, const void *data, size_t size )
{
    int i, j;
    char** srcstr;
    int count, lcount;
    int srclen, destlen;
    WCHAR *deststr;
    XTextProperty txtprop;

    txtprop.value = (BYTE *)data;
    txtprop.nitems = size;
    txtprop.encoding = x11drv_atom(COMPOUND_TEXT);
    txtprop.format = 8;
    if (XmbTextPropertyToTextList( thread_display(), &txtprop, &srcstr, &count ) != Success) return 0;
    if (!count) return 0;

    TRACE("Importing %d line(s)\n", count);

    /* Compute number of lines */
    srclen = strlen(srcstr[0]);
    for (i = 0, lcount = 0; i <= srclen; i++)
    {
        if (srcstr[0][i] == '\n')
            lcount++;
    }

    destlen = MultiByteToWideChar(CP_UNIXCP, 0, srcstr[0], -1, NULL, 0);

    TRACE("lcount = %d, destlen=%d, srcstr %s\n", lcount, destlen, srcstr[0]);

    if ((deststr = GlobalAlloc( GMEM_FIXED, (destlen + lcount + 1) * sizeof(WCHAR) )))
    {
        MultiByteToWideChar(CP_UNIXCP, 0, srcstr[0], -1, deststr, destlen);

        if (lcount)
        {
            for (i = destlen - 1, j = destlen + lcount - 1; i >= 0; i--, j--)
            {
                deststr[j] = deststr[i];

                if (deststr[i] == '\n')
                    deststr[--j] = '\r';
            }
        }
    }

    XFreeStringList(srcstr);

    return deststr;
}


/**************************************************************************
 *		import_pixmap
 *
 *  Import XA_PIXMAP, converting the image to CF_DIB.
 */
static HANDLE import_pixmap( Atom type, const void *data, size_t size )
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

        ptr = GlobalAlloc( GMEM_FIXED, info_size + info->bmiHeader.biSizeImage );
        if (ptr)
        {
            memcpy( ptr, info, info_size );
            memcpy( ptr + info_size, bits.ptr, info->bmiHeader.biSizeImage );
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
static HANDLE import_image_bmp( Atom type, const void *data, size_t size )
{
    HANDLE hClipData = 0;
    const BITMAPFILEHEADER *bfh = data;

    if (size >= sizeof(BITMAPFILEHEADER)+sizeof(BITMAPCOREHEADER) &&
        bfh->bfType == 0x4d42 /* "BM" */)
    {
        const BITMAPINFO *bmi = (const BITMAPINFO *)(bfh + 1);
        HBITMAP hbmp;
        HDC hdc = GetDC(0);

        if ((hbmp = CreateDIBitmap( hdc, &bmi->bmiHeader, CBM_INIT,
                                    (const BYTE *)data + bfh->bfOffBits, bmi, DIB_RGB_COLORS )))
        {
            hClipData = create_dib_from_bitmap( hbmp );
            DeleteObject(hbmp);
        }
        ReleaseDC(0, hdc);
    }
    return hClipData;
}


/**************************************************************************
 *		import_metafile
 */
static HANDLE import_metafile( Atom type, const void *data, size_t size )
{
    METAFILEPICT *mf;

    if (size < sizeof(METAFILEPICT)) return 0;
    if ((mf = GlobalAlloc( GMEM_FIXED, sizeof(METAFILEPICT) )))
    {
        memcpy( mf, data, sizeof(METAFILEPICT) );
        mf->hMF = SetMetaFileBitsEx( size - sizeof(METAFILEPICT),
                                     (const BYTE *)data + sizeof(METAFILEPICT) );
    }
    return mf;
}


/**************************************************************************
 *		import_enhmetafile
 */
static HANDLE import_enhmetafile( Atom type, const void *data, size_t size )
{
    return SetEnhMetaFileBits( size, data );
}


/**************************************************************************
 *      import_text_uri_list
 *
 *  Import text/uri-list.
 */
static HANDLE import_text_uri_list( Atom type, const void *data, size_t size )
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

    if (!(out = HeapAlloc(GetProcessHeap(), 0, capacity * sizeof(WCHAR)))) return 0;

    while (end < size)
    {
        while (end < size && uriList[end] != '\r')
            ++end;
        if (end < (size - 1) && uriList[end+1] != '\n')
        {
            WARN("URI list line doesn't end in \\r\\n\n");
            break;
        }

        uri = HeapAlloc(GetProcessHeap(), 0, end - start + 1);
        if (uri == NULL)
            break;
        lstrcpynA(uri, &uriList[start], end - start + 1);
        path = uri_to_dos(uri);
        TRACE("converted URI %s to DOS path %s\n", debugstr_a(uri), debugstr_w(path));
        HeapFree(GetProcessHeap(), 0, uri);

        if (path)
        {
            int pathSize = strlenW(path) + 1;
            if (pathSize > capacity - total)
            {
                capacity = 2*capacity + pathSize;
                out = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, out, (capacity + 1)*sizeof(WCHAR));
                if (out == NULL)
                    goto done;
            }
            memcpy(&out[total], path, pathSize * sizeof(WCHAR));
            total += pathSize;
        done:
            HeapFree(GetProcessHeap(), 0, path);
            if (out == NULL)
                break;
        }

        start = end + 2;
        end = start;
    }
    if (out && end >= size)
    {
        if ((dropFiles = GlobalAlloc( GMEM_FIXED, sizeof(DROPFILES) + (total + 1) * sizeof(WCHAR) )))
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
    HeapFree(GetProcessHeap(), 0, out);
    return dropFiles;
}


/**************************************************************************
 *		import_data
 *
 *  Generic import clipboard data routine.
 */
static HANDLE import_data( Atom type, const void *data, size_t size )
{
    void *ret = GlobalAlloc( GMEM_FIXED, size );

    if (ret) memcpy( ret, data, size );
    return ret;
}


/**************************************************************************
 *		import_selection
 *
 * Import the specified format from the selection and return a global handle to the data.
 */
static HANDLE import_selection( Display *display, Window win, Atom selection,
                                struct clipboard_format *format )
{
    unsigned char *data;
    unsigned long size;
    Atom prop, type;
    HANDLE ret;

    if (!format->import) return 0;

    TRACE( "import %s from %s win %lx to format %s\n",
           debugstr_xatom( format->atom ), debugstr_xatom( selection ),
           win, debugstr_format( format->id ) );

    if ((prop = convert_selection( display, win, selection, format->atom )) == None)
    {
        TRACE( "failed to convert selection\n" );
        return 0;
    }
    if (!X11DRV_CLIPBOARD_ReadProperty( display, win, prop, &type, &data, &size ))
    {
        TRACE( "failed to read property\n" );
        return 0;
    }
    ret = format->import( type, data, size );
    HeapFree( GetProcessHeap(), 0, data );
    return ret;
}


/**************************************************************************
 *      X11DRV_CLIPBOARD_ImportSelection
 *
 *  Import the X selection into the clipboard format registered for the given X target.
 */
HANDLE X11DRV_CLIPBOARD_ImportSelection( Display *display, Window win, Atom selection,
                                         Atom target, UINT *windowsFormat )
{
    struct clipboard_format *format = X11DRV_CLIPBOARD_LookupProperty( NULL, target );
    if (!format) return 0;
    *windowsFormat = format->id;
    return import_selection( display, win, selection, format );
}


/**************************************************************************
 *		export_data
 *
 *  Generic export clipboard data routine.
 */
static BOOL export_data( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    void *ptr = GlobalLock( handle );

    if (!ptr) return FALSE;
    put_property( display, win, prop, target, 8, ptr, GlobalSize( handle ));
    GlobalUnlock( handle );
    return TRUE;
}


/**************************************************************************
 *		export_string
 *
 *  Export CF_TEXT converting the string to XA_STRING.
 */
static BOOL export_string( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    UINT i, j;
    UINT size;
    LPSTR text, lpstr = NULL;

    text = GlobalLock( handle );
    size = strlen(text);

    /* remove carriage returns */
    lpstr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
    if (lpstr == NULL)
    {
        GlobalUnlock( handle );
        return FALSE;
    }
    for (i = 0,j = 0; i < size && text[i]; i++)
    {
        if (text[i] == '\r' && (text[i+1] == '\n' || text[i+1] == '\0'))
            continue;
        lpstr[j++] = text[i];
    }
    put_property( display, win, prop, target, 8, lpstr, j );
    HeapFree( GetProcessHeap(), 0, lpstr );
    GlobalUnlock( handle );
    return TRUE;
}


/**************************************************************************
 *		export_utf8_string
 *
 *  Export CF_UNICODE converting the string to UTF8.
 */
static BOOL export_utf8_string( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    UINT i, j;
    UINT size;
    LPSTR text, lpstr;
    LPWSTR uni_text = GlobalLock( handle );

    size = WideCharToMultiByte(CP_UTF8, 0, uni_text, -1, NULL, 0, NULL, NULL);
    text = HeapAlloc(GetProcessHeap(), 0, size);
    if (!text)
    {
        GlobalUnlock( handle );
        return FALSE;
    }

    WideCharToMultiByte(CP_UTF8, 0, uni_text, -1, text, size, NULL, NULL);
    GlobalUnlock( handle );

    /* remove carriage returns */
    lpstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size--);
    if (lpstr == NULL)
    {
        HeapFree(GetProcessHeap(), 0, text);
        return FALSE;
    }
    for (i = 0,j = 0; i < size && text[i]; i++)
    {
        if (text[i] == '\r' && (text[i+1] == '\n' || text[i+1] == '\0'))
            continue;
        lpstr[j++] = text[i];
    }
    put_property( display, win, prop, target, 8, lpstr, j );
    HeapFree(GetProcessHeap(), 0, lpstr);
    HeapFree(GetProcessHeap(), 0, text);
    GlobalUnlock( handle );
    return TRUE;
}


/**************************************************************************
 *		export_compound_text
 *
 *  Export CF_UNICODE to COMPOUND_TEXT
 */
static BOOL export_compound_text( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    char* lpstr;
    XTextProperty textprop;
    XICCEncodingStyle style;
    UINT i, j;
    UINT size;
    LPWSTR uni_text = GlobalLock( handle );

    size = WideCharToMultiByte(CP_UNIXCP, 0, uni_text, -1, NULL, 0, NULL, NULL);
    lpstr = HeapAlloc(GetProcessHeap(), 0, size);
    if (!lpstr)
    {
        GlobalUnlock( handle );
        return FALSE;
    }

    WideCharToMultiByte(CP_UNIXCP, 0, uni_text, -1, lpstr, size, NULL, NULL);

    /* remove carriage returns */
    for (i = 0, j = 0; i < size && lpstr[i]; i++)
    {
        if (lpstr[i] == '\r' && (lpstr[i+1] == '\n' || lpstr[i+1] == '\0'))
            continue;
        lpstr[j++] = lpstr[i];
    }
    lpstr[j]='\0';

    GlobalUnlock( handle );

    if (target == x11drv_atom(COMPOUND_TEXT))
        style = XCompoundTextStyle;
    else
        style = XStdICCTextStyle;

    /* Update the X property */
    if (XmbTextListToTextProperty( display, &lpstr, 1, style, &textprop ) == Success)
    {
        XSetTextProperty( display, win, &textprop, prop );
        XFree( textprop.value );
    }

    HeapFree(GetProcessHeap(), 0, lpstr);
    return TRUE;
}


/**************************************************************************
 *		export_pixmap
 *
 *  Export CF_DIB to XA_PIXMAP.
 */
static BOOL export_pixmap( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    Pixmap pixmap;
    BITMAPINFO *pbmi;
    struct gdi_image_bits bits;

    pbmi = GlobalLock( handle );
    bits.ptr = (LPBYTE)pbmi + bitmap_info_size( pbmi, DIB_RGB_COLORS );
    bits.free = NULL;
    bits.is_copy = FALSE;
    pixmap = create_pixmap_from_image( 0, &default_visual, pbmi, &bits, DIB_RGB_COLORS );
    GlobalUnlock( handle );

    put_property( display, win, prop, target, 32, &pixmap, 1 );
    /* FIXME: free the pixmap when the property is deleted */
    return TRUE;
}


/**************************************************************************
 *		export_image_bmp
 *
 *  Export CF_DIB to image/bmp.
 */
static BOOL export_image_bmp( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    LPBYTE dibdata = GlobalLock( handle );
    UINT bmpsize;
    BITMAPFILEHEADER *bfh;

    bmpsize = sizeof(BITMAPFILEHEADER) + GlobalSize( handle );
    bfh = HeapAlloc( GetProcessHeap(), 0, bmpsize );
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
    GlobalUnlock( handle );
    put_property( display, win, prop, target, 8, bfh, bmpsize );
    HeapFree( GetProcessHeap(), 0, bfh );
    return TRUE;
}


/**************************************************************************
 *		export_metafile
 *
 *  Export MetaFilePict.
 */
static BOOL export_metafile( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    METAFILEPICT *src, *dst;
    unsigned int size;

    src = GlobalLock( handle );
    size = GetMetaFileBitsEx(src->hMF, 0, NULL);

    dst = HeapAlloc( GetProcessHeap(), 0, size + sizeof(*dst) );
    if (dst)
    {
        *dst = *src;
        GetMetaFileBitsEx( src->hMF, size, dst + 1 );
    }
    GlobalUnlock( handle );
    put_property( display, win, prop, target, 8, dst, size + sizeof(*dst) );
    HeapFree( GetProcessHeap(), 0, dst );
    return TRUE;
}


/**************************************************************************
 *		export_enhmetafile
 *
 *  Export EnhMetaFile.
 */
static BOOL export_enhmetafile( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    unsigned int size;
    void *ptr;

    if (!(size = GetEnhMetaFileBits( handle, 0, NULL ))) return FALSE;
    if (!(ptr = HeapAlloc( GetProcessHeap(), 0, size ))) return FALSE;

    GetEnhMetaFileBits( handle, size, ptr );
    put_property( display, win, prop, target, 8, ptr, size );
    HeapFree( GetProcessHeap(), 0, ptr );
    return TRUE;
}


/**************************************************************************
 *		get_html_description_field
 *
 *  Find the value of a field in an HTML Format description.
 */
static LPCSTR get_html_description_field(LPCSTR data, LPCSTR keyword)
{
    LPCSTR pos=data;

    while (pos && *pos && *pos != '<')
    {
        if (memcmp(pos, keyword, strlen(keyword)) == 0)
            return pos+strlen(keyword);

        pos = strchr(pos, '\n');
        if (pos) pos++;
    }

    return NULL;
}


/**************************************************************************
 *		export_text_html
 *
 *  Export HTML Format to text/html.
 *
 * FIXME: We should attempt to add an <a base> tag and convert windows paths.
 */
static BOOL export_text_html( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    LPCSTR data, field_value;
    UINT fragmentstart, fragmentend;

    data = GlobalLock( handle );

    /* read the important fields */
    field_value = get_html_description_field(data, "StartFragment:");
    if (!field_value)
    {
        ERR("Couldn't find StartFragment value\n");
        goto failed;
    }
    fragmentstart = atoi(field_value);

    field_value = get_html_description_field(data, "EndFragment:");
    if (!field_value)
    {
        ERR("Couldn't find EndFragment value\n");
        goto failed;
    }
    fragmentend = atoi(field_value);

    /* export only the fragment */
    put_property( display, win, prop, target, 8, &data[fragmentstart], fragmentend - fragmentstart );
    GlobalUnlock( handle );
    return TRUE;

failed:
    GlobalUnlock( handle );
    return FALSE;
}


/**************************************************************************
 *      export_hdrop
 *
 *  Export CF_HDROP format to text/uri-list.
 */
static BOOL export_hdrop( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
{
    UINT i;
    UINT numFiles;
    char *textUriList;
    UINT textUriListSize = 32;
    UINT next = 0;

    textUriList = HeapAlloc( GetProcessHeap(), 0, textUriListSize );
    if (!textUriList) return FALSE;
    numFiles = DragQueryFileW( handle, 0xFFFFFFFF, NULL, 0 );
    for (i = 0; i < numFiles; i++)
    {
        UINT dosFilenameSize;
        WCHAR *dosFilename = NULL;
        char *unixFilename = NULL;
        UINT uriSize;
        UINT u;

        dosFilenameSize = 1 + DragQueryFileW( handle, i, NULL, 0 );
        dosFilename = HeapAlloc(GetProcessHeap(), 0, dosFilenameSize*sizeof(WCHAR));
        if (dosFilename == NULL) goto failed;
        DragQueryFileW( handle, i, dosFilename, dosFilenameSize );
        unixFilename = wine_get_unix_file_name(dosFilename);
        HeapFree(GetProcessHeap(), 0, dosFilename);
        if (unixFilename == NULL) goto failed;
        uriSize = 8 + /* file:/// */
                3 * (lstrlenA(unixFilename) - 1) + /* "%xy" per char except first '/' */
                2; /* \r\n */
        if ((next + uriSize) > textUriListSize)
        {
            UINT biggerSize = max( 2 * textUriListSize, next + uriSize );
            void *bigger = HeapReAlloc( GetProcessHeap(), 0, textUriList, biggerSize );
            if (bigger)
            {
                textUriList = bigger;
                textUriListSize = biggerSize;
            }
            else
            {
                HeapFree(GetProcessHeap(), 0, unixFilename);
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
            textUriList[next++] = hex_table[unixFilename[u] >> 4];
            textUriList[next++] = hex_table[unixFilename[u] & 0xf];
        }
        textUriList[next++] = '\r';
        textUriList[next++] = '\n';
        HeapFree(GetProcessHeap(), 0, unixFilename);
    }
    put_property( display, win, prop, target, 8, textUriList, next );
    HeapFree( GetProcessHeap(), 0, textUriList );
    return TRUE;

failed:
    HeapFree( GetProcessHeap(), 0, textUriList );
    return FALSE;
}


/**************************************************************************
 *      export_selection
 *
 * Export selection data, depending on the target type.
 */
static BOOL export_selection( Display *display, Window win, Atom prop, Atom target )
{
    struct clipboard_format *format = X11DRV_CLIPBOARD_LookupProperty( NULL, target );
    HANDLE handle = 0;

    if (!format || !format->export) return FALSE;

    TRACE( "win %lx prop %s target %s exporting %s\n", win,
           debugstr_xatom( prop ), debugstr_xatom( target ), debugstr_format( format->id ) );

    if (format->id)
    {
        LPWINE_CLIPDATA data = X11DRV_CLIPBOARD_LookupData( format->id );
        if (!data) return FALSE;
        if (!X11DRV_CLIPBOARD_RenderFormat( display, data )) return FALSE;
        handle = data->hData;
    }
    return format->export( display, win, prop, target, handle );
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
static BOOL export_multiple( Display *display, Window win, Atom prop, Atom target, HANDLE handle )
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


static int is_atom_error( Display *display, XErrorEvent *event, void *arg )
{
    return (event->error_code == BadAtom);
}

static int is_window_error( Display *display, XErrorEvent *event, void *arg )
{
    return (event->error_code == BadWindow);
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_InsertSelectionProperties
 *
 * Mark properties available for future retrieval.
 */
static VOID X11DRV_CLIPBOARD_InsertSelectionProperties(Display *display, Atom* properties, UINT count)
{
     UINT i, nb_atoms = 0;
     Atom *atoms = NULL;

     /* Cache these formats in the clipboard cache */
     for (i = 0; i < count; i++)
     {
         LPWINE_CLIPFORMAT lpFormat = X11DRV_CLIPBOARD_LookupProperty(NULL, properties[i]);

         if (lpFormat)
         {
             /* We found at least one Window's format that mapps to the property.
              * Continue looking for more.
              *
              * If more than one property map to a Window's format then we use the first 
              * one and ignore the rest.
              */
             while (lpFormat)
             {
                 TRACE( "property %s -> format %s\n",
                        debugstr_xatom( lpFormat->atom ), debugstr_format( lpFormat->id ));
                 if (lpFormat->id)
                     X11DRV_CLIPBOARD_InsertClipboardData( lpFormat->id, 0, lpFormat, FALSE );
                 lpFormat = X11DRV_CLIPBOARD_LookupProperty(lpFormat, properties[i]);
             }
         }
         else if (properties[i])
         {
             /* add it to the list of atoms that we don't know about yet */
             if (!atoms) atoms = HeapAlloc( GetProcessHeap(), 0,
                                            (count - i) * sizeof(*atoms) );
             if (atoms) atoms[nb_atoms++] = properties[i];
         }
     }

     /* query all unknown atoms in one go */
     if (atoms)
     {
         char **names = HeapAlloc( GetProcessHeap(), 0, nb_atoms * sizeof(*names) );
         if (names)
         {
             X11DRV_expect_error( display, is_atom_error, NULL );
             if (!XGetAtomNames( display, atoms, nb_atoms, names )) nb_atoms = 0;
             if (X11DRV_check_error())
             {
                 WARN( "got some bad atoms, ignoring\n" );
                 nb_atoms = 0;
             }
             for (i = 0; i < nb_atoms; i++)
             {
                 WINE_CLIPFORMAT *lpFormat;
                 LPWSTR wname;
                 int len = MultiByteToWideChar(CP_UNIXCP, 0, names[i], -1, NULL, 0);
                 wname = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
                 MultiByteToWideChar(CP_UNIXCP, 0, names[i], -1, wname, len);

                 lpFormat = register_format( RegisterClipboardFormatW(wname), atoms[i] );
                 HeapFree(GetProcessHeap(), 0, wname);
                 if (!lpFormat)
                 {
                     ERR("Failed to register %s property. Type will not be cached.\n", names[i]);
                     continue;
                 }
                 TRACE( "property %s -> format %s\n",
                        debugstr_xatom( lpFormat->atom ), debugstr_format( lpFormat->id ));
                 X11DRV_CLIPBOARD_InsertClipboardData( lpFormat->id, 0, lpFormat, FALSE );
             }
             for (i = 0; i < nb_atoms; i++) XFree( names[i] );
             HeapFree( GetProcessHeap(), 0, names );
         }
         HeapFree( GetProcessHeap(), 0, atoms );
     }
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_QueryAvailableData
 *
 * Caches the list of data formats available from the current selection.
 * This queries the selection owner for the TARGETS property and saves all
 * reported property types.
 */
static int X11DRV_CLIPBOARD_QueryAvailableData(Display *display)
{
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetList=NULL;
    Window         w;
    unsigned long  cSelectionTargets = 0;
    Atom prop;

    if (selectionAcquired & (S_PRIMARY | S_CLIPBOARD))
    {
        ERR("Received request to cache selection but process is owner=(%08x)\n", 
            (unsigned) selectionWindow);
        return -1; /* Prevent self request */
    }

    w = thread_selection_wnd();
    if (!w)
    {
        ERR("No window available to retrieve selection!\n");
        return -1;
    }

    /*
     * Query the selection owner for the TARGETS property
     */
    if ((use_primary_selection && XGetSelectionOwner(display,XA_PRIMARY)) ||
        XGetSelectionOwner(display,x11drv_atom(CLIPBOARD)))
    {
        if (use_primary_selection && (prop = convert_selection( display, w, XA_PRIMARY, x11drv_atom(TARGETS) )))
            selectionCacheSrc = XA_PRIMARY;
        else if ((prop = convert_selection( display, w, x11drv_atom(CLIPBOARD), x11drv_atom(TARGETS) )))
            selectionCacheSrc = x11drv_atom(CLIPBOARD);
        else
        {
            Atom xstr = XA_STRING;

            /* Selection Owner doesn't understand TARGETS, try retrieving XA_STRING */
            if (convert_selection( display, w, XA_PRIMARY, XA_STRING ))
            {
                X11DRV_CLIPBOARD_InsertSelectionProperties(display, &xstr, 1);
                selectionCacheSrc = XA_PRIMARY;
                return 1;
            }
            else if (convert_selection( display, w, x11drv_atom(CLIPBOARD), XA_STRING ))
            {
                X11DRV_CLIPBOARD_InsertSelectionProperties(display, &xstr, 1);
                selectionCacheSrc = x11drv_atom(CLIPBOARD);
                return 1;
            }
            else
            {
                WARN("Failed to query selection owner for available data.\n");
                return -1;
            }
        }
    }
    else return 0; /* No selection owner so report 0 targets available */

    /* Read the TARGETS property contents */
    if (!XGetWindowProperty(display, w, prop,
        0, 0x3FFF, True, AnyPropertyType/*XA_ATOM*/, &atype, &aformat, &cSelectionTargets, 
        &remain, (unsigned char**)&targetList))
    {
       TRACE( "type %s format %d count %ld remain %ld\n",
              debugstr_xatom( atype ), aformat, cSelectionTargets, remain);
       /*
        * The TARGETS property should have returned us a list of atoms
        * corresponding to each selection target format supported.
        */
       if (atype == XA_ATOM || atype == x11drv_atom(TARGETS))
       {
           if (aformat == 32)
           {
               X11DRV_CLIPBOARD_InsertSelectionProperties(display, targetList, cSelectionTargets);
           }
           else if (aformat == 8)  /* work around quartz-wm brain damage */
           {
               unsigned long i, count = cSelectionTargets / sizeof(CARD32);
               Atom *atoms = HeapAlloc( GetProcessHeap(), 0, count * sizeof(Atom) );
               for (i = 0; i < count; i++)
                   atoms[i] = ((CARD32 *)targetList)[i];  /* FIXME: byte swapping */
               X11DRV_CLIPBOARD_InsertSelectionProperties( display, atoms, count );
               HeapFree( GetProcessHeap(), 0, atoms );
           }
       }

       /* Free the list of targets */
       XFree(targetList);
    }
    else WARN("Failed to read TARGETS property\n");

    return cSelectionTargets;
}


/**************************************************************************
 *	X11DRV_CLIPBOARD_ReadSelectionData
 *
 * This method is invoked only when we DO NOT own the X selection
 *
 * We always get the data from the selection client each time,
 * since we have no way of determining if the data in our cache is stale.
 */
static BOOL X11DRV_CLIPBOARD_ReadSelectionData(Display *display, LPWINE_CLIPDATA lpData)
{
    BOOL bRet = FALSE;
    HANDLE hData;

    if (!lpData->lpFormat)
    {
        ERR("Requesting format %04x but no source format linked to data.\n",
            lpData->wFormatID);
        return FALSE;
    }

    if (!selectionAcquired)
    {
        Window w = thread_selection_wnd();
        if(!w)
        {
            ERR("No window available to read selection data!\n");
            return FALSE;
        }

        hData = import_selection( display, w, selectionCacheSrc, lpData->lpFormat );
        if (hData)
            bRet = X11DRV_CLIPBOARD_InsertClipboardData(lpData->wFormatID, hData, lpData->lpFormat, TRUE);
    }
    else
    {
        ERR("Received request to cache selection data but process is owner\n");
    }

    TRACE("Returning %d\n", bRet);

    return bRet;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_GetProperty
 *  Gets type, data and size.
 */
static BOOL X11DRV_CLIPBOARD_GetProperty(Display *display, Window w, Atom prop,
    Atom *atype, unsigned char** data, unsigned long* datasize)
{
    int aformat;
    unsigned long pos = 0, nitems, remain, count;
    unsigned char *val = NULL, *buffer;

    for (;;)
    {
        if (XGetWindowProperty(display, w, prop, pos, INT_MAX / 4, False,
                               AnyPropertyType, atype, &aformat, &nitems, &remain, &buffer))
        {
            WARN("Failed to read property\n");
            HeapFree( GetProcessHeap(), 0, val );
            return FALSE;
        }

        count = get_property_size( aformat, nitems );
        if (!val) *data = HeapAlloc( GetProcessHeap(), 0, pos * sizeof(int) + count + 1 );
        else *data = HeapReAlloc( GetProcessHeap(), 0, val, pos * sizeof(int) + count + 1 );

        if (!*data)
        {
            XFree( buffer );
            HeapFree( GetProcessHeap(), 0, val );
            return FALSE;
        }
        val = *data;
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

    TRACE( "got property %s type %s format %u len %lu from window %lx\n",
           debugstr_xatom( prop ), debugstr_xatom( *atype ), aformat, *datasize, w );

    /* Delete the property on the window now that we are done
     * This will send a PropertyNotify event to the selection owner. */
    XDeleteProperty(display, w, prop);
    return TRUE;
}


struct clipboard_data_packet {
    struct list entry;
    unsigned long size;
    unsigned char *data;
};

/**************************************************************************
 *		X11DRV_CLIPBOARD_ReadProperty
 *  Reads the contents of the X selection property.
 */
static BOOL X11DRV_CLIPBOARD_ReadProperty( Display *display, Window w, Atom prop,
                                           Atom *type, unsigned char** data, unsigned long* datasize )
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

        HeapFree(GetProcessHeap(), 0, *data);
        *data = NULL;

        list_init(&packets);

        for (;;)
        {
            int i;
            unsigned char *prop_data;
            unsigned long prop_size;

            /* Wait until PropertyNotify is received */
            for (i = 0; i < SELECTION_RETRIES; i++)
            {
                Bool res;

                res = XCheckTypedWindowEvent(display, w, PropertyNotify, &xe);
                if (res && xe.xproperty.atom == prop &&
                    xe.xproperty.state == PropertyNewValue)
                    break;
                usleep(SELECTION_WAIT);
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
                HeapFree(GetProcessHeap(), 0, prop_data);
                res = TRUE;
                break;
            }

            packet = HeapAlloc(GetProcessHeap(), 0, sizeof(*packet));
            if (!packet)
            {
                HeapFree(GetProcessHeap(), 0, prop_data);
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
            buf = HeapAlloc(GetProcessHeap(), 0, bufsize + 1);
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
            HeapFree(GetProcessHeap(), 0, packet->data);
            HeapFree(GetProcessHeap(), 0, packet);
        }

        return res;
    }

    return TRUE;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ReleaseSelection
 *
 * Release XA_CLIPBOARD and XA_PRIMARY in response to a SelectionClear event.
 */
static void X11DRV_CLIPBOARD_ReleaseSelection(Display *display, Atom selType, Window w, HWND hwnd, Time time)
{
    /* w is the window that lost the selection
     */
    TRACE("event->window = %08x (selectionWindow = %08x) selectionAcquired=0x%08x\n",
	  (unsigned)w, (unsigned)selectionWindow, (unsigned)selectionAcquired);

    if (selectionAcquired && (w == selectionWindow))
    {
        HWND owner;

        /* completely give up the selection */
        TRACE("Lost CLIPBOARD (+PRIMARY) selection\n");

        if (X11DRV_CLIPBOARD_IsProcessOwner( &owner ))
        {
            /* Since we're still the owner, this wasn't initiated by
               another Wine process */
            if (OpenClipboard(hwnd))
            {
                /* Destroy private objects */
                SendMessageW(owner, WM_DESTROYCLIPBOARD, 0, 0);

                /* Give up ownership of the windows clipboard */
                X11DRV_CLIPBOARD_ReleaseOwnership();
                CloseClipboard();
            }
        }

        if ((selType == x11drv_atom(CLIPBOARD)) && (selectionAcquired & S_PRIMARY))
        {
            TRACE("Lost clipboard. Check if we need to release PRIMARY\n");

            if (selectionWindow == XGetSelectionOwner(display, XA_PRIMARY))
            {
                TRACE("We still own PRIMARY. Releasing PRIMARY.\n");
                XSetSelectionOwner(display, XA_PRIMARY, None, time);
            }
            else
                TRACE("We no longer own PRIMARY\n");
        }
        else if ((selType == XA_PRIMARY) && (selectionAcquired & S_CLIPBOARD))
        {
            TRACE("Lost PRIMARY. Check if we need to release CLIPBOARD\n");

            if (selectionWindow == XGetSelectionOwner(display,x11drv_atom(CLIPBOARD)))
            {
                TRACE("We still own CLIPBOARD. Releasing CLIPBOARD.\n");
                XSetSelectionOwner(display, x11drv_atom(CLIPBOARD), None, time);
            }
            else
                TRACE("We no longer own CLIPBOARD\n");
        }

        selectionWindow = None;

        empty_clipboard();

        /* Reset the selection flags now that we are done */
        selectionAcquired = S_NOSELECTION;
    }
}


/**************************************************************************
 *                X11DRV Clipboard Exports
 **************************************************************************/


static void selection_acquire(void)
{
    Window owner;
    Display *display;

    owner = thread_selection_wnd();
    display = thread_display();

    selectionAcquired = 0;
    selectionWindow = 0;

    /* Grab PRIMARY selection if not owned */
    if (use_primary_selection)
        XSetSelectionOwner(display, XA_PRIMARY, owner, CurrentTime);

    /* Grab CLIPBOARD selection if not owned */
    XSetSelectionOwner(display, x11drv_atom(CLIPBOARD), owner, CurrentTime);

    if (use_primary_selection && XGetSelectionOwner(display, XA_PRIMARY) == owner)
        selectionAcquired |= S_PRIMARY;

    if (XGetSelectionOwner(display,x11drv_atom(CLIPBOARD)) == owner)
        selectionAcquired |= S_CLIPBOARD;

    if (selectionAcquired)
    {
        selectionWindow = owner;
        TRACE("Grabbed X selection, owner=(%08x)\n", (unsigned) owner);
    }
}

static DWORD WINAPI selection_thread_proc(LPVOID p)
{
    HANDLE event = p;

    TRACE("\n");

    selection_acquire();
    SetEvent(event);

    while (selectionAcquired)
    {
        MsgWaitForMultipleObjectsEx(0, NULL, INFINITE, QS_SENDMESSAGE, 0);
    }

    return 0;
}

/**************************************************************************
 *		X11DRV_AcquireClipboard
 */
void X11DRV_AcquireClipboard(HWND hWndClipWindow)
{
    DWORD procid;
    HANDLE selectionThread;

    TRACE(" %p\n", hWndClipWindow);

    /*
     * It's important that the selection get acquired from the thread
     * that owns the clipboard window. The primary reason is that we know 
     * it is running a message loop and therefore can process the 
     * X selection events.
     */
    if (hWndClipWindow &&
        GetCurrentThreadId() != GetWindowThreadProcessId(hWndClipWindow, &procid))
    {
        if (procid != GetCurrentProcessId())
        {
            WARN("Setting clipboard owner to other process is not supported\n");
            hWndClipWindow = NULL;
        }
        else
        {
            TRACE("Thread %x is acquiring selection with thread %x's window %p\n",
                GetCurrentThreadId(),
                GetWindowThreadProcessId(hWndClipWindow, NULL), hWndClipWindow);

            SendMessageW(hWndClipWindow, WM_X11DRV_ACQUIRE_SELECTION, 0, 0);
            return;
        }
    }

    if (hWndClipWindow)
    {
        selection_acquire();
    }
    else
    {
        HANDLE event = CreateEventW(NULL, FALSE, FALSE, NULL);
        selectionThread = CreateThread(NULL, 0, selection_thread_proc, event, 0, NULL);

        if (selectionThread)
        {
            WaitForSingleObject(event, INFINITE);
            CloseHandle(selectionThread);
        }
        CloseHandle(event);
    }
}


static void empty_clipboard(void)
{
    WINE_CLIPDATA *data, *next;

    LIST_FOR_EACH_ENTRY_SAFE( data, next, &data_list, WINE_CLIPDATA, entry )
    {
        list_remove( &data->entry );
        X11DRV_CLIPBOARD_FreeData( data );
        HeapFree( GetProcessHeap(), 0, data );
        ClipDataCount--;
    }

    TRACE(" %d entries remaining in cache.\n", ClipDataCount);
}

/**************************************************************************
 *	X11DRV_EmptyClipboard
 *
 * Empty cached clipboard data.
 */
void CDECL X11DRV_EmptyClipboard(void)
{
    X11DRV_AcquireClipboard( GetOpenClipboardWindow() );
    empty_clipboard();
}

/**************************************************************************
 *		X11DRV_SetClipboardData
 */
BOOL CDECL X11DRV_SetClipboardData(UINT wFormat, HANDLE hData, BOOL owner)
{
    if (!owner) X11DRV_CLIPBOARD_UpdateCache();

    return X11DRV_CLIPBOARD_InsertClipboardData(wFormat, hData, NULL, TRUE);
}


/**************************************************************************
 *		CountClipboardFormats
 */
INT CDECL X11DRV_CountClipboardFormats(void)
{
    X11DRV_CLIPBOARD_UpdateCache();

    TRACE(" count=%d\n", ClipDataCount);

    return ClipDataCount;
}


/**************************************************************************
 *		X11DRV_EnumClipboardFormats
 */
UINT CDECL X11DRV_EnumClipboardFormats(UINT wFormat)
{
    struct list *ptr = NULL;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache();

    if (!wFormat)
    {
        ptr = list_head( &data_list );
    }
    else
    {
        LPWINE_CLIPDATA lpData = X11DRV_CLIPBOARD_LookupData(wFormat);
        if (lpData) ptr = list_next( &data_list, &lpData->entry );
    }

    if (!ptr) return 0;
    return LIST_ENTRY( ptr, WINE_CLIPDATA, entry )->wFormatID;
}


/**************************************************************************
 *		X11DRV_IsClipboardFormatAvailable
 */
BOOL CDECL X11DRV_IsClipboardFormatAvailable(UINT wFormat)
{
    BOOL bRet = FALSE;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache();

    if (wFormat != 0 && X11DRV_CLIPBOARD_LookupData(wFormat))
        bRet = TRUE;

    TRACE("(%04X)- ret(%d)\n", wFormat, bRet);

    return bRet;
}


/**************************************************************************
 *		GetClipboardData (USER.142)
 */
HANDLE CDECL X11DRV_GetClipboardData(UINT wFormat)
{
    LPWINE_CLIPDATA lpRender;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache();

    if ((lpRender = X11DRV_CLIPBOARD_LookupData(wFormat)))
    {
        if ( !lpRender->hData )
            X11DRV_CLIPBOARD_RenderFormat(thread_init_display(), lpRender);

        TRACE(" returning %p (type %04x)\n", lpRender->hData, lpRender->wFormatID);
        return lpRender->hData;
    }

    return 0;
}


/**************************************************************************
 *		ResetSelectionOwner
 *
 * Called when the thread owning the selection is destroyed and we need to
 * preserve the selection ownership. We look for another top level window
 * in this process and send it a message to acquire the selection.
 */
void X11DRV_ResetSelectionOwner(void)
{
    HWND hwnd;
    DWORD procid;

    TRACE("\n");

    if (!selectionAcquired  || thread_selection_wnd() != selectionWindow)
        return;

    selectionAcquired = S_NOSELECTION;
    selectionWindow = 0;

    hwnd = GetWindow(GetDesktopWindow(), GW_CHILD);
    do
    {
        if (GetCurrentThreadId() != GetWindowThreadProcessId(hwnd, &procid))
        {
            if (GetCurrentProcessId() == procid)
            {
                if (SendMessageW(hwnd, WM_X11DRV_ACQUIRE_SELECTION, 0, 0))
                    return;
            }
        }
    } while ((hwnd = GetWindow(hwnd, GW_HWNDNEXT)) != NULL);

    WARN("Failed to find another thread to take selection ownership. Clipboard data will be lost.\n");

    X11DRV_CLIPBOARD_ReleaseOwnership();
    empty_clipboard();
}


/***********************************************************************
 *           X11DRV_SelectionRequest_TARGETS
 *  Service a TARGETS selection request event
 */
static Atom X11DRV_SelectionRequest_TARGETS( Display *display, Window requestor,
                                             Atom target, Atom rprop )
{
    UINT i;
    Atom* targets;
    ULONG cTargets;
    LPWINE_CLIPFORMAT format;
    LPWINE_CLIPDATA lpData;

    /* Create X atoms for any clipboard types which don't have atoms yet.
     * This avoids sending bogus zero atoms.
     * Without this, copying might not have access to all clipboard types.
     * FIXME: is it safe to call this here?
     */
    intern_atoms();

    /*
     * Count the number of items we wish to expose as selection targets.
     */
    cTargets = 1; /* Include TARGETS */

    if (!list_head( &data_list )) return None;

    LIST_FOR_EACH_ENTRY( lpData, &data_list, WINE_CLIPDATA, entry )
        LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
            if (format->id == lpData->wFormatID && format->export && format->atom)
                cTargets++;

    TRACE(" found %d formats\n", cTargets);

    /* Allocate temp buffer */
    targets = HeapAlloc( GetProcessHeap(), 0, cTargets * sizeof(Atom));
    if(targets == NULL)
        return None;

    i = 0;
    targets[i++] = x11drv_atom(TARGETS);

    LIST_FOR_EACH_ENTRY( lpData, &data_list, WINE_CLIPDATA, entry )
        LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
            if (format->id == lpData->wFormatID && format->export && format->atom)
            {
                TRACE( "%d: %s -> %s\n", i, debugstr_format( format->id ),
                       debugstr_xatom( format->atom ));
                targets[i++] = format->atom;
            }

    put_property( display, requestor, rprop, XA_ATOM, 32, targets, cTargets );

    HeapFree(GetProcessHeap(), 0, targets);

    return rprop;
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
    Window request = event->requestor;

    X11DRV_expect_error( display, is_window_error, NULL );

    /*
     * We can only handle the selection request if :
     * The selection is PRIMARY or CLIPBOARD, AND we can successfully open the clipboard.
     */
    if (((event->selection != XA_PRIMARY) && (event->selection != x11drv_atom(CLIPBOARD))))
        goto done;

    /* If the specified property is None the requestor is an obsolete client.
     * We support these by using the specified target atom as the reply property.
     */
    rprop = event->property;
    if( rprop == None )
        rprop = event->target;

    if(event->target == x11drv_atom(TARGETS))  /*  Return a list of all supported targets */
    {
        /* TARGETS selection request */
        rprop = X11DRV_SelectionRequest_TARGETS( display, request, event->target, rprop );
    }
    else if (!export_selection( display, event->requestor, rprop, event->target ))
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
    XSync( display, False );
    if (X11DRV_check_error()) WARN( "requestor %lx is no longer valid\n", event->requestor );
    return FALSE;
}


/***********************************************************************
 *           X11DRV_SelectionClear
 */
BOOL X11DRV_SelectionClear( HWND hWnd, XEvent *xev )
{
    XSelectionClearEvent *event = &xev->xselectionclear;
    if (event->selection == XA_PRIMARY || event->selection == x11drv_atom(CLIPBOARD))
        X11DRV_CLIPBOARD_ReleaseSelection( event->display, event->selection,
                                           event->window, hWnd, event->time );
    return FALSE;
}
