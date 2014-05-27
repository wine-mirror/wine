/*
 * X11 clipboard windows driver
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
 *           2003 Ulrich Czekalla for CodeWeavers
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

typedef struct
{
    HWND hWndOpen;
    HWND hWndOwner;
    HWND hWndViewer;
    UINT seqno;
    UINT flags;
} CLIPBOARDINFO, *LPCLIPBOARDINFO;

struct tagWINE_CLIPDATA; /* Forward */

typedef HANDLE (*DRVEXPORTFUNC)(Display *display, Window requestor, Atom aTarget, Atom rprop,
    struct tagWINE_CLIPDATA* lpData, LPDWORD lpBytes);
typedef HANDLE (*DRVIMPORTFUNC)(Display *d, Window w, Atom prop);

typedef struct tagWINE_CLIPFORMAT {
    struct list entry;
    UINT        wFormatID;
    UINT        drvData;
    DRVIMPORTFUNC  lpDrvImportFunc;
    DRVEXPORTFUNC  lpDrvExportFunc;
} WINE_CLIPFORMAT, *LPWINE_CLIPFORMAT;

typedef struct tagWINE_CLIPDATA {
    struct list entry;
    UINT        wFormatID;
    HANDLE      hData;
    UINT        wFlags;
    UINT        drvData;
    LPWINE_CLIPFORMAT lpFormat;
} WINE_CLIPDATA, *LPWINE_CLIPDATA;

#define CF_FLAG_UNOWNED      0x0001 /* cached data is not owned */
#define CF_FLAG_SYNTHESIZED  0x0002 /* Implicitly converted data */

static int selectionAcquired = 0;              /* Contains the current selection masks */
static Window selectionWindow = None;          /* The top level X window which owns the selection */
static Atom selectionCacheSrc = XA_PRIMARY;    /* The selection source from which the clipboard cache was filled */

void CDECL X11DRV_EmptyClipboard(BOOL keepunowned);
void CDECL X11DRV_EndClipboardUpdate(void);
static HANDLE X11DRV_CLIPBOARD_ImportClipboardData(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportEnhMetaFile(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportMetaFilePict(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportXAPIXMAP(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportImageBmp(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportXAString(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportUTF8(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ImportCompoundText(Display *d, Window w, Atom prop);
static HANDLE X11DRV_CLIPBOARD_ExportClipboardData(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpData, LPDWORD lpBytes);
static HANDLE X11DRV_CLIPBOARD_ExportString(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpData, LPDWORD lpBytes);
static HANDLE X11DRV_CLIPBOARD_ExportXAPIXMAP(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
static HANDLE X11DRV_CLIPBOARD_ExportImageBmp(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
static HANDLE X11DRV_CLIPBOARD_ExportMetaFilePict(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
static HANDLE X11DRV_CLIPBOARD_ExportEnhMetaFile(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
static HANDLE X11DRV_CLIPBOARD_ExportTextHtml(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
static WINE_CLIPFORMAT *X11DRV_CLIPBOARD_InsertClipboardFormat(UINT id, Atom prop);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedText(Display *display, UINT wFormatID);
static void X11DRV_CLIPBOARD_FreeData(LPWINE_CLIPDATA lpData);
static BOOL X11DRV_CLIPBOARD_IsSelectionOwner(void);
static int X11DRV_CLIPBOARD_QueryAvailableData(Display *display, LPCLIPBOARDINFO lpcbinfo);
static BOOL X11DRV_CLIPBOARD_ReadSelectionData(Display *display, LPWINE_CLIPDATA lpData);
static BOOL X11DRV_CLIPBOARD_ReadProperty(Display *display, Window w, Atom prop,
    unsigned char** data, unsigned long* datasize);
static BOOL X11DRV_CLIPBOARD_RenderFormat(Display *display, LPWINE_CLIPDATA lpData);
static HANDLE X11DRV_CLIPBOARD_SerializeMetafile(INT wformat, HANDLE hdata, LPDWORD lpcbytes, BOOL out);
static BOOL X11DRV_CLIPBOARD_SynthesizeData(UINT wFormatID);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedFormat(Display *display, LPWINE_CLIPDATA lpData);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedDIB(Display *display);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedBitmap(Display *display);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedEnhMetaFile(Display *display);
static void X11DRV_HandleSelectionRequest( HWND hWnd, XSelectionRequestEvent *event, BOOL bIsMultiple );

/* Clipboard formats */

static const struct
{
    UINT          id;
    UINT          data;
    DRVIMPORTFUNC import;
    DRVEXPORTFUNC export;
} builtin_formats[] =
{
    { CF_TEXT, XA_STRING, X11DRV_CLIPBOARD_ImportXAString, X11DRV_CLIPBOARD_ExportString},
    { CF_BITMAP, XATOM_WCF_BITMAP, X11DRV_CLIPBOARD_ImportClipboardData, NULL},
    { CF_METAFILEPICT, XATOM_WCF_METAFILEPICT, X11DRV_CLIPBOARD_ImportMetaFilePict, X11DRV_CLIPBOARD_ExportMetaFilePict },
    { CF_SYLK, XATOM_WCF_SYLK, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DIF, XATOM_WCF_DIF, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_TIFF, XATOM_WCF_TIFF, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_OEMTEXT, XATOM_WCF_OEMTEXT, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DIB, XA_PIXMAP, X11DRV_CLIPBOARD_ImportXAPIXMAP, X11DRV_CLIPBOARD_ExportXAPIXMAP },
    { CF_PALETTE, XATOM_WCF_PALETTE, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_PENDATA, XATOM_WCF_PENDATA, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_RIFF, XATOM_WCF_RIFF, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_WAVE, XATOM_WCF_WAVE, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_UNICODETEXT, XATOM_UTF8_STRING, X11DRV_CLIPBOARD_ImportUTF8, X11DRV_CLIPBOARD_ExportString },
    /* If UTF8_STRING is not available, attempt COMPOUND_TEXT */
    { CF_UNICODETEXT, XATOM_COMPOUND_TEXT, X11DRV_CLIPBOARD_ImportCompoundText, X11DRV_CLIPBOARD_ExportString },
    { CF_ENHMETAFILE, XATOM_WCF_ENHMETAFILE, X11DRV_CLIPBOARD_ImportEnhMetaFile, X11DRV_CLIPBOARD_ExportEnhMetaFile },
    { CF_HDROP, XATOM_WCF_HDROP, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_LOCALE, XATOM_WCF_LOCALE, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DIBV5, XATOM_WCF_DIBV5, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_OWNERDISPLAY, XATOM_WCF_OWNERDISPLAY, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DSPTEXT, XATOM_WCF_DSPTEXT, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DSPBITMAP, XATOM_WCF_DSPBITMAP, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DSPMETAFILEPICT, XATOM_WCF_DSPMETAFILEPICT, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DSPENHMETAFILE, XATOM_WCF_DSPENHMETAFILE, X11DRV_CLIPBOARD_ImportClipboardData, X11DRV_CLIPBOARD_ExportClipboardData },
    { CF_DIB, XATOM_image_bmp, X11DRV_CLIPBOARD_ImportImageBmp, X11DRV_CLIPBOARD_ExportImageBmp },
};

static struct list format_list = LIST_INIT( format_list );

#define GET_ATOM(prop)  (((prop) < FIRST_XATOM) ? (Atom)(prop) : X11DRV_Atoms[(prop) - FIRST_XATOM])

/* Maps X properties to Windows formats */
static const WCHAR wszRichTextFormat[] = {'R','i','c','h',' ','T','e','x','t',' ','F','o','r','m','a','t',0};
static const WCHAR wszGIF[] = {'G','I','F',0};
static const WCHAR wszJFIF[] = {'J','F','I','F',0};
static const WCHAR wszPNG[] = {'P','N','G',0};
static const WCHAR wszHTMLFormat[] = {'H','T','M','L',' ','F','o','r','m','a','t',0};
static const struct
{
    LPCWSTR lpszFormat;
    UINT   prop;
} PropertyFormatMap[] =
{
    { wszRichTextFormat, XATOM_text_rtf },
    { wszRichTextFormat, XATOM_text_richtext },
    { wszGIF, XATOM_image_gif },
    { wszJFIF, XATOM_image_jpeg },
    { wszPNG, XATOM_image_png },
    { wszHTMLFormat, XATOM_HTML_Format }, /* prefer this to text/html */
};


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
 *		X11DRV_InitClipboard
 */
void X11DRV_InitClipboard(void)
{
    UINT i;
    WINE_CLIPFORMAT *format;

    /* Register built-in formats */
    for (i = 0; i < sizeof(builtin_formats)/sizeof(builtin_formats[0]); i++)
    {
        if (!(format = HeapAlloc( GetProcessHeap(), 0, sizeof(*format )))) break;
        format->wFormatID       = builtin_formats[i].id;
        format->drvData         = GET_ATOM(builtin_formats[i].data);
        format->lpDrvImportFunc = builtin_formats[i].import;
        format->lpDrvExportFunc = builtin_formats[i].export;
        list_add_tail( &format_list, &format->entry );
    }

    /* Register known mapping between window formats and X properties */
    for (i = 0; i < sizeof(PropertyFormatMap)/sizeof(PropertyFormatMap[0]); i++)
        X11DRV_CLIPBOARD_InsertClipboardFormat( RegisterClipboardFormatW(PropertyFormatMap[i].lpszFormat),
                                                GET_ATOM(PropertyFormatMap[i].prop));

    /* Set up a conversion function from "HTML Format" to "text/html" */
    format = X11DRV_CLIPBOARD_InsertClipboardFormat( RegisterClipboardFormatW(wszHTMLFormat),
                                                     GET_ATOM(XATOM_text_html));
    format->lpDrvExportFunc = X11DRV_CLIPBOARD_ExportTextHtml;
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
        if (!format->drvData) count++;
    if (!count) return;

    display = thread_init_display();

    names = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*names) );
    atoms = HeapAlloc( GetProcessHeap(), 0, count * sizeof(*atoms) );

    i = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
        if (!format->drvData) {
            GetClipboardFormatNameW( format->wFormatID, buffer, 256 );
            len = WideCharToMultiByte(CP_UNIXCP, 0, buffer, -1, NULL, 0, NULL, NULL);
            names[i] = HeapAlloc(GetProcessHeap(), 0, len);
            WideCharToMultiByte(CP_UNIXCP, 0, buffer, -1, names[i++], len, NULL, NULL);
        }

    XInternAtoms( display, names, count, False, atoms );

    i = 0;
    LIST_FOR_EACH_ENTRY( format, &format_list, WINE_CLIPFORMAT, entry )
        if (!format->drvData) {
            HeapFree(GetProcessHeap(), 0, names[i]);
            format->drvData = atoms[i++];
        }

    HeapFree( GetProcessHeap(), 0, names );
    HeapFree( GetProcessHeap(), 0, atoms );
}


/**************************************************************************
 *		register_format
 *
 * Register a custom X clipboard format.
 */
static WINE_CLIPFORMAT *register_format( UINT id, Atom prop )
{
    LPWINE_CLIPFORMAT lpFormat;

    /* walk format chain to see if it's already registered */
    LIST_FOR_EACH_ENTRY( lpFormat, &format_list, WINE_CLIPFORMAT, entry )
        if (lpFormat->wFormatID == id) return lpFormat;

    return X11DRV_CLIPBOARD_InsertClipboardFormat(id, prop);
}


/**************************************************************************
 *                X11DRV_CLIPBOARD_LookupProperty
 */
static LPWINE_CLIPFORMAT X11DRV_CLIPBOARD_LookupProperty(LPWINE_CLIPFORMAT current, UINT drvData)
{
    for (;;)
    {
        struct list *ptr = current ? &current->entry : &format_list;
        BOOL need_intern = FALSE;

        while ((ptr = list_next( &format_list, ptr )))
        {
            LPWINE_CLIPFORMAT lpFormat = LIST_ENTRY( ptr, WINE_CLIPFORMAT, entry );
            if (lpFormat->drvData == drvData) return lpFormat;
            if (!lpFormat->drvData) need_intern = TRUE;
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
 *		InsertClipboardFormat
 */
static WINE_CLIPFORMAT *X11DRV_CLIPBOARD_InsertClipboardFormat( UINT id, Atom prop )
{
    LPWINE_CLIPFORMAT lpNewFormat;

    /* allocate storage for new format entry */
    lpNewFormat = HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_CLIPFORMAT));

    if(lpNewFormat == NULL) 
    {
        WARN("No more memory for a new format!\n");
        return NULL;
    }
    lpNewFormat->wFormatID = id;
    lpNewFormat->drvData = prop;
    lpNewFormat->lpDrvImportFunc = X11DRV_CLIPBOARD_ImportClipboardData;
    lpNewFormat->lpDrvExportFunc = X11DRV_CLIPBOARD_ExportClipboardData;

    list_add_tail( &format_list, &lpNewFormat->entry );

    TRACE("Registering format %s drvData %d\n",
          debugstr_format(lpNewFormat->wFormatID), lpNewFormat->drvData);

    return lpNewFormat;
}




/**************************************************************************
 *                      X11DRV_CLIPBOARD_GetClipboardInfo
 */
static BOOL X11DRV_CLIPBOARD_GetClipboardInfo(LPCLIPBOARDINFO cbInfo)
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
            cbInfo->hWndOpen = wine_server_ptr_handle( reply->old_clipboard );
            cbInfo->hWndOwner = wine_server_ptr_handle( reply->old_owner );
            cbInfo->hWndViewer = wine_server_ptr_handle( reply->old_viewer );
            cbInfo->seqno = reply->seqno;
            cbInfo->flags = reply->flags;

            bRet = TRUE;
        }
    }
    SERVER_END_REQ;

    return bRet;
}


/**************************************************************************
 *	X11DRV_CLIPBOARD_ReleaseOwnership
 */
static BOOL X11DRV_CLIPBOARD_ReleaseOwnership(void)
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
 *                      X11DRV_CLIPBOARD_InsertClipboardData
 *
 * Caller *must* have the clipboard open and be the owner.
 */
static BOOL X11DRV_CLIPBOARD_InsertClipboardData(UINT wFormatID, HANDLE hData, DWORD flags,
                                                 LPWINE_CLIPFORMAT lpFormat, BOOL override)
{
    LPWINE_CLIPDATA lpData = X11DRV_CLIPBOARD_LookupData(wFormatID);

    TRACE("format=%04x lpData=%p hData=%p flags=0x%08x lpFormat=%p override=%d\n",
        wFormatID, lpData, hData, flags, lpFormat, override);

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

    lpData->wFlags = flags;

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

    if ((lpData->wFormatID >= CF_GDIOBJFIRST &&
        lpData->wFormatID <= CF_GDIOBJLAST) || 
        lpData->wFormatID == CF_BITMAP || 
        lpData->wFormatID == CF_DIB || 
        lpData->wFormatID == CF_PALETTE)
    {
      if (lpData->hData)
	DeleteObject(lpData->hData);

      if ((lpData->wFormatID == CF_DIB) && lpData->drvData)
          XFreePixmap(gdi_display, lpData->drvData);
    }
    else if (lpData->wFormatID == CF_METAFILEPICT)
    {
      if (lpData->hData)
      {
        DeleteMetaFile(((METAFILEPICT *)GlobalLock( lpData->hData ))->hMF );
        GlobalFree(lpData->hData);
      }
    }
    else if (lpData->wFormatID == CF_ENHMETAFILE)
    {
        if (lpData->hData)
            DeleteEnhMetaFile(lpData->hData);
    }
    else if (lpData->wFormatID < CF_PRIVATEFIRST ||
             lpData->wFormatID > CF_PRIVATELAST)
    {
      if (lpData->hData)
        GlobalFree(lpData->hData);
    }

    lpData->hData = 0;
    lpData->drvData = 0;
}


/**************************************************************************
 *			X11DRV_CLIPBOARD_UpdateCache
 */
static BOOL X11DRV_CLIPBOARD_UpdateCache(LPCLIPBOARDINFO lpcbinfo)
{
    BOOL bret = TRUE;

    if (!X11DRV_CLIPBOARD_IsSelectionOwner())
    {
        if (!X11DRV_CLIPBOARD_GetClipboardInfo(lpcbinfo))
        {
            ERR("Failed to retrieve clipboard information.\n");
            bret = FALSE;
        }
        else if (wSeqNo < lpcbinfo->seqno)
        {
            X11DRV_EmptyClipboard(TRUE);

            if (X11DRV_CLIPBOARD_QueryAvailableData(thread_init_display(), lpcbinfo) < 0)
            {
                ERR("Failed to cache clipboard data owned by another process.\n");
                bret = FALSE;
            }
            else
            {
                X11DRV_EndClipboardUpdate();
            }

            wSeqNo = lpcbinfo->seqno;
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

    if (lpData->wFlags & CF_FLAG_SYNTHESIZED)
        bret = X11DRV_CLIPBOARD_RenderSynthesizedFormat(display, lpData);
    else if (!X11DRV_CLIPBOARD_IsSelectionOwner())
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
        CLIPBOARDINFO cbInfo;

        if (X11DRV_CLIPBOARD_GetClipboardInfo(&cbInfo) && cbInfo.hWndOwner)
        {
            /* Send a WM_RENDERFORMAT message to notify the owner to render the
             * data requested into the clipboard.
             */
            TRACE("Sending WM_RENDERFORMAT message to hwnd(%p)\n", cbInfo.hWndOwner);
            SendMessageW(cbInfo.hWndOwner, WM_RENDERFORMAT, lpData->wFormatID, 0);

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
 *                      CLIPBOARD_ConvertText
 * Returns number of required/converted characters - not bytes!
 */
static INT CLIPBOARD_ConvertText(WORD src_fmt, void const *src, INT src_size,
				 WORD dst_fmt, void *dst, INT dst_size)
{
    UINT cp;

    if(src_fmt == CF_UNICODETEXT)
    {
	switch(dst_fmt)
	{
	case CF_TEXT:
	    cp = CP_ACP;
	    break;
	case CF_OEMTEXT:
	    cp = CP_OEMCP;
	    break;
	default:
	    return 0;
	}
	return WideCharToMultiByte(cp, 0, src, src_size, dst, dst_size, NULL, NULL);
    }

    if(dst_fmt == CF_UNICODETEXT)
    {
	switch(src_fmt)
	{
	case CF_TEXT:
	    cp = CP_ACP;
	    break;
	case CF_OEMTEXT:
	    cp = CP_OEMCP;
	    break;
	default:
	    return 0;
	}
	return MultiByteToWideChar(cp, 0, src, src_size, dst, dst_size);
    }

    if(!dst_size) return src_size;

    if(dst_size > src_size) dst_size = src_size;

    if(src_fmt == CF_TEXT )
	CharToOemBuffA(src, dst, dst_size);
    else
	OemToCharBuffA(src, dst, dst_size);

    return dst_size;
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_RenderSynthesizedFormat
 */
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedFormat(Display *display, LPWINE_CLIPDATA lpData)
{
    BOOL bret = FALSE;

    TRACE("\n");

    if (lpData->wFlags & CF_FLAG_SYNTHESIZED)
    {
        UINT wFormatID = lpData->wFormatID;

        if (wFormatID == CF_UNICODETEXT || wFormatID == CF_TEXT || wFormatID == CF_OEMTEXT)
            bret = X11DRV_CLIPBOARD_RenderSynthesizedText(display, wFormatID);
        else 
        {
            switch (wFormatID)
	    {
                case CF_DIB:
                    bret = X11DRV_CLIPBOARD_RenderSynthesizedDIB( display );
                    break;

                case CF_BITMAP:
                    bret = X11DRV_CLIPBOARD_RenderSynthesizedBitmap( display );
                    break;

                case CF_ENHMETAFILE:
                    bret = X11DRV_CLIPBOARD_RenderSynthesizedEnhMetaFile( display );
                    break;

                case CF_METAFILEPICT:
                    FIXME("Synthesizing CF_METAFILEPICT not implemented\n");
                    break;

                default:
                    FIXME("Called to synthesize unknown format 0x%08x\n", wFormatID);
                    break;
            }
        }

        lpData->wFlags &= ~CF_FLAG_SYNTHESIZED;
    }

    return bret;
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_RenderSynthesizedText
 *
 * Renders synthesized text
 */
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedText(Display *display, UINT wFormatID)
{
    LPCSTR lpstrS;
    LPSTR  lpstrT;
    HANDLE hData;
    INT src_chars, dst_chars, alloc_size;
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE("%04x\n", wFormatID);

    if ((lpSource = X11DRV_CLIPBOARD_LookupData(wFormatID)) &&
        lpSource->hData)
        return TRUE;

    /* Look for rendered source or non-synthesized source */
    if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_UNICODETEXT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData))
    {
        TRACE("UNICODETEXT -> %04x\n", wFormatID);
    }
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_TEXT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData))
    {
        TRACE("TEXT -> %04x\n", wFormatID);
    }
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_OEMTEXT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData))
    {
        TRACE("OEMTEXT -> %04x\n", wFormatID);
    }

    if (!lpSource || (lpSource->wFlags & CF_FLAG_SYNTHESIZED &&
        !lpSource->hData))
        return FALSE;

    /* Ask the clipboard owner to render the source text if necessary */
    if (!lpSource->hData && !X11DRV_CLIPBOARD_RenderFormat(display, lpSource))
        return FALSE;

    lpstrS = GlobalLock(lpSource->hData);
    if (!lpstrS)
        return FALSE;

    /* Text always NULL terminated */
    if(lpSource->wFormatID == CF_UNICODETEXT)
        src_chars = strlenW((LPCWSTR)lpstrS) + 1;
    else
        src_chars = strlen(lpstrS) + 1;

    /* Calculate number of characters in the destination buffer */
    dst_chars = CLIPBOARD_ConvertText(lpSource->wFormatID, lpstrS, 
        src_chars, wFormatID, NULL, 0);

    if (!dst_chars)
        return FALSE;

    TRACE("Converting from '%04x' to '%04x', %i chars\n",
    	lpSource->wFormatID, wFormatID, src_chars);

    /* Convert characters to bytes */
    if(wFormatID == CF_UNICODETEXT)
        alloc_size = dst_chars * sizeof(WCHAR);
    else
        alloc_size = dst_chars;

    hData = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE |
        GMEM_DDESHARE, alloc_size);

    lpstrT = GlobalLock(hData);

    if (lpstrT)
    {
        CLIPBOARD_ConvertText(lpSource->wFormatID, lpstrS, src_chars,
            wFormatID, lpstrT, dst_chars);
        GlobalUnlock(hData);
    }

    GlobalUnlock(lpSource->hData);

    return X11DRV_CLIPBOARD_InsertClipboardData(wFormatID, hData, 0, NULL, TRUE);
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
    hPackedDIB = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE /*| GMEM_ZEROINIT*/,
                             cPackedSize );
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


/**************************************************************************
 *                      X11DRV_CLIPBOARD_RenderSynthesizedDIB
 *
 * Renders synthesized DIB
 */
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedDIB(Display *display)
{
    BOOL bret = FALSE;
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE("\n");

    if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_DIB)) && lpSource->hData)
    {
        bret = TRUE;
    }
    /* If we have a bitmap and it's not synthesized or it has been rendered */
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_BITMAP)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData))
    {
        /* Render source if required */
        if (lpSource->hData || X11DRV_CLIPBOARD_RenderFormat(display, lpSource))
        {
            HGLOBAL hData = create_dib_from_bitmap( lpSource->hData );
            if (hData)
            {
                X11DRV_CLIPBOARD_InsertClipboardData(CF_DIB, hData, 0, NULL, TRUE);
                bret = TRUE;
            }
        }
    }

    return bret;
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_RenderSynthesizedBitmap
 *
 * Renders synthesized bitmap
 */
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedBitmap(Display *display)
{
    BOOL bret = FALSE;
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE("\n");

    if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_BITMAP)) && lpSource->hData)
    {
        bret = TRUE;
    }
    /* If we have a dib and it's not synthesized or it has been rendered */
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_DIB)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData))
    {
        /* Render source if required */
        if (lpSource->hData || X11DRV_CLIPBOARD_RenderFormat(display, lpSource))
        {
            HDC hdc;
            HBITMAP hData = NULL;
            unsigned int offset;
            LPBITMAPINFOHEADER lpbmih;

            hdc = GetDC(NULL);
            lpbmih = GlobalLock(lpSource->hData);
            if (lpbmih)
            {
                offset = sizeof(BITMAPINFOHEADER)
                      + ((lpbmih->biBitCount <= 8) ? (sizeof(RGBQUAD) *
                        (1 << lpbmih->biBitCount)) : 0);

                hData = CreateDIBitmap(hdc, lpbmih, CBM_INIT, (LPBYTE)lpbmih +
                    offset, (LPBITMAPINFO) lpbmih, DIB_RGB_COLORS);

                GlobalUnlock(lpSource->hData);
            }
            ReleaseDC(NULL, hdc);

            if (hData)
            {
                X11DRV_CLIPBOARD_InsertClipboardData(CF_BITMAP, hData, 0, NULL, TRUE);
                bret = TRUE;
            }
        }
    }

    return bret;
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_RenderSynthesizedEnhMetaFile
 */
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedEnhMetaFile(Display *display)
{
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE("\n");

    if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_ENHMETAFILE)) && lpSource->hData)
        return TRUE;
    /* If we have a MF pict and it's not synthesized or it has been rendered */
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_METAFILEPICT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData))
    {
        /* Render source if required */
        if (lpSource->hData || X11DRV_CLIPBOARD_RenderFormat(display, lpSource))
        {
            METAFILEPICT *pmfp;
            HENHMETAFILE hData = NULL;

            pmfp = GlobalLock(lpSource->hData);
            if (pmfp)
            {
                UINT size_mf_bits = GetMetaFileBitsEx(pmfp->hMF, 0, NULL);
                void *mf_bits = HeapAlloc(GetProcessHeap(), 0, size_mf_bits);
                if (mf_bits)
                {
                    GetMetaFileBitsEx(pmfp->hMF, size_mf_bits, mf_bits);
                    hData = SetWinMetaFileBits(size_mf_bits, mf_bits, NULL, pmfp);
                    HeapFree(GetProcessHeap(), 0, mf_bits);
                }
                GlobalUnlock(lpSource->hData);
            }

            if (hData)
            {
                X11DRV_CLIPBOARD_InsertClipboardData(CF_ENHMETAFILE, hData, 0, NULL, TRUE);
                return TRUE;
            }
        }
    }

    return FALSE;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportXAString
 *
 *  Import XA_STRING, converting the string to CF_TEXT.
 */
static HANDLE X11DRV_CLIPBOARD_ImportXAString(Display *display, Window w, Atom prop)
{
    LPBYTE lpdata;
    unsigned long cbytes;
    LPSTR lpstr;
    unsigned long i, inlcount = 0;
    HANDLE hText = 0;

    if (!X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
        return 0;

    for (i = 0; i <= cbytes; i++)
    {
        if (lpdata[i] == '\n')
            inlcount++;
    }

    if ((hText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbytes + inlcount + 1)))
    {
        lpstr = GlobalLock(hText);

        for (i = 0, inlcount = 0; i <= cbytes; i++)
        {
            if (lpdata[i] == '\n')
                lpstr[inlcount++] = '\r';

            lpstr[inlcount++] = lpdata[i];
        }

        GlobalUnlock(hText);
    }

    /* Free the retrieved property data */
    HeapFree(GetProcessHeap(), 0, lpdata);

    return hText;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportUTF8
 *
 *  Import XA_STRING, converting the string to CF_UNICODE.
 */
static HANDLE X11DRV_CLIPBOARD_ImportUTF8(Display *display, Window w, Atom prop)
{
    LPBYTE lpdata;
    unsigned long cbytes;
    LPSTR lpstr;
    unsigned long i, inlcount = 0;
    HANDLE hUnicodeText = 0;

    if (!X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
        return 0;

    for (i = 0; i <= cbytes; i++)
    {
        if (lpdata[i] == '\n')
            inlcount++;
    }

    if ((lpstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cbytes + inlcount + 1)))
    {
        UINT count;

        for (i = 0, inlcount = 0; i <= cbytes; i++)
        {
            if (lpdata[i] == '\n')
                lpstr[inlcount++] = '\r';

            lpstr[inlcount++] = lpdata[i];
        }

        count = MultiByteToWideChar(CP_UTF8, 0, lpstr, -1, NULL, 0);
        hUnicodeText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, count * sizeof(WCHAR));

        if (hUnicodeText)
        {
            WCHAR *textW = GlobalLock(hUnicodeText);
            MultiByteToWideChar(CP_UTF8, 0, lpstr, -1, textW, count);
            GlobalUnlock(hUnicodeText);
        }

        HeapFree(GetProcessHeap(), 0, lpstr);
    }

    /* Free the retrieved property data */
    HeapFree(GetProcessHeap(), 0, lpdata);

    return hUnicodeText;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportCompoundText
 *
 *  Import COMPOUND_TEXT to CF_UNICODE
 */
static HANDLE X11DRV_CLIPBOARD_ImportCompoundText(Display *display, Window w, Atom prop)
{
    int i, j, ret;
    char** srcstr;
    int count, lcount;
    int srclen, destlen;
    HANDLE hUnicodeText;
    XTextProperty txtprop;

    if (!X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &txtprop.value, &txtprop.nitems))
    {
        return 0;
    }

    txtprop.encoding = x11drv_atom(COMPOUND_TEXT);
    txtprop.format = 8;
    ret = XmbTextPropertyToTextList(display, &txtprop, &srcstr, &count);
    HeapFree(GetProcessHeap(), 0, txtprop.value);
    if (ret != Success || !count) return 0;

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

    if ((hUnicodeText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (destlen + lcount + 1) * sizeof(WCHAR))))
    {
        WCHAR *deststr = GlobalLock(hUnicodeText);
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

        GlobalUnlock(hUnicodeText);
    }

    XFreeStringList(srcstr);

    return hUnicodeText;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportXAPIXMAP
 *
 *  Import XA_PIXMAP, converting the image to CF_DIB.
 */
static HANDLE X11DRV_CLIPBOARD_ImportXAPIXMAP(Display *display, Window w, Atom prop)
{
    LPBYTE lpdata;
    unsigned long cbytes;
    Pixmap *pPixmap;
    HANDLE hClipData = 0;

    if (X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
    {
        XVisualInfo vis = default_visual;
        char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
        BITMAPINFO *info = (BITMAPINFO *)buffer;
        struct gdi_image_bits bits;
        Window root;
        int x,y;               /* Unused */
        unsigned border_width; /* Unused */
        unsigned int depth, width, height;

        pPixmap = (Pixmap *) lpdata;

        /* Get the Pixmap dimensions and bit depth */
        if (!XGetGeometry(gdi_display, *pPixmap, &root, &x, &y, &width, &height,
                          &border_width, &depth)) depth = 0;
        if (!pixmap_formats[depth]) return 0;

        TRACE("\tPixmap properties: width=%d, height=%d, depth=%d\n",
              width, height, depth);

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
            BYTE *ptr;

            hClipData = GlobalAlloc( GMEM_MOVEABLE | GMEM_DDESHARE,
                                     info_size + info->bmiHeader.biSizeImage );
            if (hClipData)
            {
                ptr = GlobalLock( hClipData );
                memcpy( ptr, info, info_size );
                memcpy( ptr + info_size, bits.ptr, info->bmiHeader.biSizeImage );
                GlobalUnlock( hClipData );
            }
            if (bits.free) bits.free( &bits );
        }

        HeapFree(GetProcessHeap(), 0, lpdata);
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportImageBmp
 *
 *  Import image/bmp, converting the image to CF_DIB.
 */
static HANDLE X11DRV_CLIPBOARD_ImportImageBmp(Display *display, Window w, Atom prop)
{
    LPBYTE lpdata;
    unsigned long cbytes;
    HANDLE hClipData = 0;

    if (X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
    {
        BITMAPFILEHEADER *bfh = (BITMAPFILEHEADER*)lpdata;

        if (cbytes >= sizeof(BITMAPFILEHEADER)+sizeof(BITMAPCOREHEADER) &&
            bfh->bfType == 0x4d42 /* "BM" */)
        {
            BITMAPINFO *bmi = (BITMAPINFO*)(bfh+1);
            HBITMAP hbmp;
            HDC hdc;

            hdc = GetDC(0);
            hbmp = CreateDIBitmap(
                hdc,
                &(bmi->bmiHeader),
                CBM_INIT,
                lpdata+bfh->bfOffBits,
                bmi,
                DIB_RGB_COLORS
                );

            hClipData = create_dib_from_bitmap( hbmp );

            DeleteObject(hbmp);
            ReleaseDC(0, hdc);
        }

        /* Free the retrieved property data */
        HeapFree(GetProcessHeap(), 0, lpdata);
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportMetaFilePict
 *
 *  Import MetaFilePict.
 */
static HANDLE X11DRV_CLIPBOARD_ImportMetaFilePict(Display *display, Window w, Atom prop)
{
    LPBYTE lpdata;
    unsigned long cbytes;
    HANDLE hClipData = 0;

    if (X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
    {
        if (cbytes)
            hClipData = X11DRV_CLIPBOARD_SerializeMetafile(CF_METAFILEPICT, lpdata, (LPDWORD)&cbytes, FALSE);

        /* Free the retrieved property data */
        HeapFree(GetProcessHeap(), 0, lpdata);
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_ImportEnhMetaFile
 *
 *  Import EnhMetaFile.
 */
static HANDLE X11DRV_CLIPBOARD_ImportEnhMetaFile(Display *display, Window w, Atom prop)
{
    LPBYTE lpdata;
    unsigned long cbytes;
    HANDLE hClipData = 0;

    if (X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
    {
        if (cbytes)
            hClipData = X11DRV_CLIPBOARD_SerializeMetafile(CF_ENHMETAFILE, lpdata, (LPDWORD)&cbytes, FALSE);

        /* Free the retrieved property data */
        HeapFree(GetProcessHeap(), 0, lpdata);
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_ImportClipbordaData
 *
 *  Generic import clipboard data routine.
 */
static HANDLE X11DRV_CLIPBOARD_ImportClipboardData(Display *display, Window w, Atom prop)
{
    LPVOID lpClipData;
    LPBYTE lpdata;
    unsigned long cbytes;
    HANDLE hClipData = 0;

    if (X11DRV_CLIPBOARD_ReadProperty(display, w, prop, &lpdata, &cbytes))
    {
        if (cbytes)
        {
            /* Turn on the DDESHARE flag to enable shared 32 bit memory */
            hClipData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbytes);
            if (hClipData == 0)
            {
                HeapFree(GetProcessHeap(), 0, lpdata);
                return NULL;
            }

            if ((lpClipData = GlobalLock(hClipData)))
            {
                memcpy(lpClipData, lpdata, cbytes);
                GlobalUnlock(hClipData);
            }
            else
            {
                GlobalFree(hClipData);
                hClipData = 0;
            }
        }

        /* Free the retrieved property data */
        HeapFree(GetProcessHeap(), 0, lpdata);
    }

    return hClipData;
}

/**************************************************************************
 *      X11DRV_CLIPBOARD_ImportSelection
 *
 *  Import the X selection into the clipboard format registered for the given X target.
 */
HANDLE X11DRV_CLIPBOARD_ImportSelection(Display *d, Atom target, Window w, Atom prop, UINT *windowsFormat)
{
    WINE_CLIPFORMAT *clipFormat;

    clipFormat = X11DRV_CLIPBOARD_LookupProperty(NULL, target);
    if (clipFormat)
    {
        *windowsFormat = clipFormat->wFormatID;
        return clipFormat->lpDrvImportFunc(d, w, prop);
    }
    return NULL;
}


/**************************************************************************
 		X11DRV_CLIPBOARD_ExportClipboardData
 *
 *  Generic export clipboard data routine.
 */
static HANDLE X11DRV_CLIPBOARD_ExportClipboardData(Display *display, Window requestor, Atom aTarget,
                                            Atom rprop, LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    LPVOID lpClipData;
    UINT datasize = 0;
    HANDLE hClipData = 0;

    *lpBytes = 0; /* Assume failure */

    if (!X11DRV_CLIPBOARD_RenderFormat(display, lpData))
        ERR("Failed to export %04x format\n", lpData->wFormatID);
    else
    {
        datasize = GlobalSize(lpData->hData);

        hClipData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, datasize);
        if (hClipData == 0) return NULL;

        if ((lpClipData = GlobalLock(hClipData)))
        {
            LPVOID lpdata = GlobalLock(lpData->hData);

            memcpy(lpClipData, lpdata, datasize);
            *lpBytes = datasize;

            GlobalUnlock(lpData->hData);
            GlobalUnlock(hClipData);
        } else {
            GlobalFree(hClipData);
            hClipData = 0;
        }
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportXAString
 *
 *  Export CF_TEXT converting the string to XA_STRING.
 *  Helper function for X11DRV_CLIPBOARD_ExportString.
 */
static HANDLE X11DRV_CLIPBOARD_ExportXAString(LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    UINT i, j;
    UINT size;
    LPSTR text, lpstr = NULL;

    *lpBytes = 0; /* Assume return has zero bytes */

    text = GlobalLock(lpData->hData);
    size = strlen(text);

    /* remove carriage returns */
    lpstr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size + 1);
    if (lpstr == NULL)
        goto done;

    for (i = 0,j = 0; i < size && text[i]; i++)
    {
        if (text[i] == '\r' && (text[i+1] == '\n' || text[i+1] == '\0'))
            continue;
        lpstr[j++] = text[i];
    }

    lpstr[j]='\0';
    *lpBytes = j; /* Number of bytes in string */

done:
    GlobalUnlock(lpData->hData);

    return lpstr;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportUTF8String
 *
 *  Export CF_UNICODE converting the string to UTF8.
 *  Helper function for X11DRV_CLIPBOARD_ExportString.
 */
static HANDLE X11DRV_CLIPBOARD_ExportUTF8String(LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    UINT i, j;
    UINT size;
    LPWSTR uni_text;
    LPSTR text, lpstr = NULL;

    *lpBytes = 0; /* Assume return has zero bytes */

    uni_text = GlobalLock(lpData->hData);

    size = WideCharToMultiByte(CP_UTF8, 0, uni_text, -1, NULL, 0, NULL, NULL);

    text = HeapAlloc(GetProcessHeap(), 0, size);
    if (!text)
        goto done;
    WideCharToMultiByte(CP_UTF8, 0, uni_text, -1, text, size, NULL, NULL);

    /* remove carriage returns */
    lpstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size--);
    if (lpstr == NULL)
        goto done;

    for (i = 0,j = 0; i < size && text[i]; i++)
    {
        if (text[i] == '\r' && (text[i+1] == '\n' || text[i+1] == '\0'))
            continue;
        lpstr[j++] = text[i];
    }
    lpstr[j]='\0';

    *lpBytes = j; /* Number of bytes in string */

done:
    HeapFree(GetProcessHeap(), 0, text);
    GlobalUnlock(lpData->hData);

    return lpstr;
}



/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportCompoundText
 *
 *  Export CF_UNICODE to COMPOUND_TEXT
 *  Helper function for X11DRV_CLIPBOARD_ExportString.
 */
static HANDLE X11DRV_CLIPBOARD_ExportCompoundText(Display *display, Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    char* lpstr = 0;
    XTextProperty prop;
    XICCEncodingStyle style;
    UINT i, j;
    UINT size;
    LPWSTR uni_text;

    uni_text = GlobalLock(lpData->hData);

    size = WideCharToMultiByte(CP_UNIXCP, 0, uni_text, -1, NULL, 0, NULL, NULL);
    lpstr = HeapAlloc(GetProcessHeap(), 0, size);
    if (!lpstr)
        return 0;

    WideCharToMultiByte(CP_UNIXCP, 0, uni_text, -1, lpstr, size, NULL, NULL);

    /* remove carriage returns */
    for (i = 0, j = 0; i < size && lpstr[i]; i++)
    {
        if (lpstr[i] == '\r' && (lpstr[i+1] == '\n' || lpstr[i+1] == '\0'))
            continue;
        lpstr[j++] = lpstr[i];
    }
    lpstr[j]='\0';

    GlobalUnlock(lpData->hData);

    if (aTarget == x11drv_atom(COMPOUND_TEXT))
        style = XCompoundTextStyle;
    else
        style = XStdICCTextStyle;

    /* Update the X property */
    if (XmbTextListToTextProperty(display, &lpstr, 1, style, &prop) == Success)
    {
        XSetTextProperty(display, requestor, &prop, rprop);
        XFree(prop.value);
    }

    HeapFree(GetProcessHeap(), 0, lpstr);

    return 0;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportString
 *
 *  Export string
 */
static HANDLE X11DRV_CLIPBOARD_ExportString(Display *display, Window requestor, Atom aTarget, Atom rprop,
                                     LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    if (X11DRV_CLIPBOARD_RenderFormat(display, lpData))
    {
        if (aTarget == XA_STRING)
            return X11DRV_CLIPBOARD_ExportXAString(lpData, lpBytes);
        else if (aTarget == x11drv_atom(COMPOUND_TEXT) || aTarget == x11drv_atom(TEXT))
            return X11DRV_CLIPBOARD_ExportCompoundText(display, requestor, aTarget,
                rprop, lpData, lpBytes);
        else
        {
            TRACE("Exporting target %ld to default UTF8_STRING\n", aTarget);
            return X11DRV_CLIPBOARD_ExportUTF8String(lpData, lpBytes);
        }
    }
    else
        ERR("Failed to render %04x format\n", lpData->wFormatID);

    return 0;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportXAPIXMAP
 *
 *  Export CF_DIB to XA_PIXMAP.
 */
static HANDLE X11DRV_CLIPBOARD_ExportXAPIXMAP(Display *display, Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    HANDLE hData;
    unsigned char* lpData;

    if (!X11DRV_CLIPBOARD_RenderFormat(display, lpdata))
    {
        ERR("Failed to export %04x format\n", lpdata->wFormatID);
        return 0;
    }

    if (!lpdata->drvData) /* If not already rendered */
    {
        Pixmap pixmap;
        LPBITMAPINFO pbmi;
        struct gdi_image_bits bits;

        pbmi = GlobalLock( lpdata->hData );
        bits.ptr = (LPBYTE)pbmi + bitmap_info_size( pbmi, DIB_RGB_COLORS );
        bits.free = NULL;
        bits.is_copy = FALSE;
        pixmap = create_pixmap_from_image( 0, &default_visual, pbmi, &bits, DIB_RGB_COLORS );
        GlobalUnlock( lpdata->hData );
        lpdata->drvData = pixmap;
    }

    *lpBytes = sizeof(Pixmap); /* pixmap is a 32bit value */

    /* Wrap pixmap so we can return a handle */
    hData = GlobalAlloc(0, *lpBytes);
    lpData = GlobalLock(hData);
    memcpy(lpData, &lpdata->drvData, *lpBytes);
    GlobalUnlock(hData);

    return hData;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportImageBmp
 *
 *  Export CF_DIB to image/bmp.
 */
static HANDLE X11DRV_CLIPBOARD_ExportImageBmp(Display *display, Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    HANDLE hpackeddib;
    LPBYTE dibdata;
    UINT bmpsize;
    HANDLE hbmpdata;
    LPBYTE bmpdata;
    BITMAPFILEHEADER *bfh;

    *lpBytes = 0;

    if (!X11DRV_CLIPBOARD_RenderFormat(display, lpdata))
    {
        ERR("Failed to export %04x format\n", lpdata->wFormatID);
        return 0;
    }

    hpackeddib = lpdata->hData;

    dibdata = GlobalLock(hpackeddib);
    if (!dibdata)
    {
        ERR("Failed to lock packed DIB\n");
        return 0;
    }

    bmpsize = sizeof(BITMAPFILEHEADER) + GlobalSize(hpackeddib);

    hbmpdata = GlobalAlloc(0, bmpsize);

    if (hbmpdata)
    {
        bmpdata = GlobalLock(hbmpdata);

        if (!bmpdata)
        {
            GlobalFree(hbmpdata);
            GlobalUnlock(hpackeddib);
            return 0;
        }

        /* bitmap file header */
        bfh = (BITMAPFILEHEADER*)bmpdata;
        bfh->bfType = 0x4d42; /* "BM" */
        bfh->bfSize = bmpsize;
        bfh->bfReserved1 = 0;
        bfh->bfReserved2 = 0;
        bfh->bfOffBits = sizeof(BITMAPFILEHEADER) + bitmap_info_size((BITMAPINFO*)dibdata, DIB_RGB_COLORS);

        /* rest of bitmap is the same as the packed dib */
        memcpy(bfh+1, dibdata, bmpsize-sizeof(BITMAPFILEHEADER));

        *lpBytes = bmpsize;

        GlobalUnlock(hbmpdata);
    }

    GlobalUnlock(hpackeddib);

    return hbmpdata;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportMetaFilePict
 *
 *  Export MetaFilePict.
 */
static HANDLE X11DRV_CLIPBOARD_ExportMetaFilePict(Display *display, Window requestor, Atom aTarget, Atom rprop,
                                           LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    if (!X11DRV_CLIPBOARD_RenderFormat(display, lpdata))
    {
        ERR("Failed to export %04x format\n", lpdata->wFormatID);
        return 0;
    }

    return X11DRV_CLIPBOARD_SerializeMetafile(CF_METAFILEPICT, lpdata->hData, lpBytes, TRUE);
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportEnhMetaFile
 *
 *  Export EnhMetaFile.
 */
static HANDLE X11DRV_CLIPBOARD_ExportEnhMetaFile(Display *display, Window requestor, Atom aTarget, Atom rprop,
                                          LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    if (!X11DRV_CLIPBOARD_RenderFormat(display, lpdata))
    {
        ERR("Failed to export %04x format\n", lpdata->wFormatID);
        return 0;
    }

    return X11DRV_CLIPBOARD_SerializeMetafile(CF_ENHMETAFILE, lpdata->hData, lpBytes, TRUE);
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
 *		X11DRV_CLIPBOARD_ExportTextHtml
 *
 *  Export HTML Format to text/html.
 *
 * FIXME: We should attempt to add an <a base> tag and convert windows paths.
 */
static HANDLE X11DRV_CLIPBOARD_ExportTextHtml(Display *display, Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    HANDLE hdata;
    LPCSTR data, field_value;
    UINT fragmentstart, fragmentend, htmlsize;
    HANDLE hhtmldata=NULL;
    LPSTR htmldata;

    *lpBytes = 0;

    if (!X11DRV_CLIPBOARD_RenderFormat(display, lpdata))
    {
        ERR("Failed to export %04x format\n", lpdata->wFormatID);
        return 0;
    }

    hdata = lpdata->hData;

    data = GlobalLock(hdata);
    if (!data)
    {
        ERR("Failed to lock HTML Format data\n");
        return 0;
    }

    /* read the important fields */
    field_value = get_html_description_field(data, "StartFragment:");
    if (!field_value)
    {
        ERR("Couldn't find StartFragment value\n");
        goto end;
    }
    fragmentstart = atoi(field_value);

    field_value = get_html_description_field(data, "EndFragment:");
    if (!field_value)
    {
        ERR("Couldn't find EndFragment value\n");
        goto end;
    }
    fragmentend = atoi(field_value);

    /* export only the fragment */
    htmlsize = fragmentend - fragmentstart + 1;

    hhtmldata = GlobalAlloc(0, htmlsize);

    if (hhtmldata)
    {
        htmldata = GlobalLock(hhtmldata);

        if (!htmldata)
        {
            GlobalFree(hhtmldata);
            htmldata = NULL;
            goto end;
        }

        memcpy(htmldata, &data[fragmentstart], fragmentend-fragmentstart);
        htmldata[htmlsize-1] = '\0';

        *lpBytes = htmlsize;

        GlobalUnlock(htmldata);
    }

end:

    GlobalUnlock(hdata);

    return hhtmldata;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_QueryTargets
 */
static BOOL X11DRV_CLIPBOARD_QueryTargets(Display *display, Window w, Atom selection,
    Atom target, XEvent *xe)
{
    INT i;

    XConvertSelection(display, selection, target, x11drv_atom(SELECTION_DATA), w, CurrentTime);

    /*
     * Wait until SelectionNotify is received
     */
    for (i = 0; i < SELECTION_RETRIES; i++)
    {
        Bool res = XCheckTypedWindowEvent(display, w, SelectionNotify, xe);
        if (res && xe->xselection.selection == selection) break;

        usleep(SELECTION_WAIT);
    }

    if (i == SELECTION_RETRIES)
    {
        ERR("Timed out waiting for SelectionNotify event\n");
        return FALSE;
    }
    /* Verify that the selection returned a valid TARGETS property */
    if ((xe->xselection.target != target) || (xe->xselection.property == None))
    {
        /* Selection owner failed to respond or we missed the SelectionNotify */
        WARN("Failed to retrieve TARGETS for selection %ld.\n", selection);
        return FALSE;
    }

    return TRUE;
}


static int is_atom_error( Display *display, XErrorEvent *event, void *arg )
{
    return (event->error_code == BadAtom);
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
                 TRACE("Atom#%d Property(%d): --> Format %s\n",
                       i, lpFormat->drvData, debugstr_format(lpFormat->wFormatID));
                 X11DRV_CLIPBOARD_InsertClipboardData(lpFormat->wFormatID, 0, 0, lpFormat, FALSE);
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
                 TRACE("Atom#%d Property(%d): --> Format %s\n",
                       i, lpFormat->drvData, debugstr_format(lpFormat->wFormatID));
                 X11DRV_CLIPBOARD_InsertClipboardData(lpFormat->wFormatID, 0, 0, lpFormat, FALSE);
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
static int X11DRV_CLIPBOARD_QueryAvailableData(Display *display, LPCLIPBOARDINFO lpcbinfo)
{
    XEvent         xe;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetList=NULL;
    Window         w;
    unsigned long  cSelectionTargets = 0;

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
        if (use_primary_selection && (X11DRV_CLIPBOARD_QueryTargets(display, w, XA_PRIMARY, x11drv_atom(TARGETS), &xe)))
            selectionCacheSrc = XA_PRIMARY;
        else if (X11DRV_CLIPBOARD_QueryTargets(display, w, x11drv_atom(CLIPBOARD), x11drv_atom(TARGETS), &xe))
            selectionCacheSrc = x11drv_atom(CLIPBOARD);
        else
        {
            Atom xstr = XA_STRING;

            /* Selection Owner doesn't understand TARGETS, try retrieving XA_STRING */
            if (X11DRV_CLIPBOARD_QueryTargets(display, w, XA_PRIMARY, XA_STRING, &xe))
            {
                X11DRV_CLIPBOARD_InsertSelectionProperties(display, &xstr, 1);
                selectionCacheSrc = XA_PRIMARY;
                return 1;
            }
            else if (X11DRV_CLIPBOARD_QueryTargets(display, w, x11drv_atom(CLIPBOARD), XA_STRING, &xe))
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
    if (!XGetWindowProperty(display, xe.xselection.requestor, xe.xselection.property,
        0, 0x3FFF, True, AnyPropertyType/*XA_ATOM*/, &atype, &aformat, &cSelectionTargets, 
        &remain, (unsigned char**)&targetList) != Success)
    {
       TRACE("Type %lx,Format %d,nItems %ld, Remain %ld\n",
             atype, aformat, cSelectionTargets, remain);
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
    Bool res;
    DWORD i;
    XEvent xe;
    BOOL bRet = FALSE;

    TRACE("%04x\n", lpData->wFormatID);

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

        TRACE("Requesting conversion of %s property (%d) from selection type %08x\n",
              debugstr_format(lpData->lpFormat->wFormatID), lpData->lpFormat->drvData,
              (UINT)selectionCacheSrc);

        XConvertSelection(display, selectionCacheSrc, lpData->lpFormat->drvData,
                          x11drv_atom(SELECTION_DATA), w, CurrentTime);

        /* wait until SelectionNotify is received */
        for (i = 0; i < SELECTION_RETRIES; i++)
        {
            res = XCheckTypedWindowEvent(display, w, SelectionNotify, &xe);
            if (res && xe.xselection.selection == selectionCacheSrc) break;

            usleep(SELECTION_WAIT);
        }

        if (i == SELECTION_RETRIES)
        {
            ERR("Timed out waiting for SelectionNotify event\n");
        }
        /* Verify that the selection returned a valid TARGETS property */
        else if (xe.xselection.property != None)
        {
            /*
             *  Read the contents of the X selection property 
             *  into WINE's clipboard cache and converting the 
             *  data format if necessary.
             */
             HANDLE hData = lpData->lpFormat->lpDrvImportFunc(display, xe.xselection.requestor,
                 xe.xselection.property);

             if (hData)
                 bRet = X11DRV_CLIPBOARD_InsertClipboardData(lpData->wFormatID, hData, 0, lpData->lpFormat, TRUE);
             else
                 TRACE("Import function failed\n");
        }
        else
        {
            TRACE("Failed to convert selection\n");
        }
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

    TRACE("Reading property %lu from X window %lx\n", prop, w);

    for (;;)
    {
        if (XGetWindowProperty(display, w, prop, pos, INT_MAX / 4, False,
                               AnyPropertyType, atype, &aformat, &nitems, &remain, &buffer) != Success)
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
static BOOL X11DRV_CLIPBOARD_ReadProperty(Display *display, Window w, Atom prop,
    unsigned char** data, unsigned long* datasize)
{
    Atom atype;
    XEvent xe;

    if (prop == None)
        return FALSE;

    while (XCheckTypedWindowEvent(display, w, PropertyNotify, &xe))
        ;

    if (!X11DRV_CLIPBOARD_GetProperty(display, w, prop, &atype, data, datasize))
        return FALSE;

    if (atype == x11drv_atom(INCR))
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
                !X11DRV_CLIPBOARD_GetProperty(display, w, prop, &atype, &prop_data, &prop_size))
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
 *		CLIPBOARD_SerializeMetafile
 */
static HANDLE X11DRV_CLIPBOARD_SerializeMetafile(INT wformat, HANDLE hdata, LPDWORD lpcbytes, BOOL out)
{
    HANDLE h = 0;

    TRACE(" wFormat=%d hdata=%p out=%d\n", wformat, hdata, out);

    if (out) /* Serialize out, caller should free memory */
    {
        *lpcbytes = 0; /* Assume failure */

        if (wformat == CF_METAFILEPICT)
        {
            LPMETAFILEPICT lpmfp = GlobalLock(hdata);
            unsigned int size = GetMetaFileBitsEx(lpmfp->hMF, 0, NULL);

            h = GlobalAlloc(0, size + sizeof(METAFILEPICT));
            if (h)
            {
                char *pdata = GlobalLock(h);

                memcpy(pdata, lpmfp, sizeof(METAFILEPICT));
                GetMetaFileBitsEx(lpmfp->hMF, size, pdata + sizeof(METAFILEPICT));

                *lpcbytes = size + sizeof(METAFILEPICT);

                GlobalUnlock(h);
            }

            GlobalUnlock(hdata);
        }
        else if (wformat == CF_ENHMETAFILE)
        {
            int size = GetEnhMetaFileBits(hdata, 0, NULL);

            h = GlobalAlloc(0, size);
            if (h)
            {
                LPVOID pdata = GlobalLock(h);

                GetEnhMetaFileBits(hdata, size, pdata);
                *lpcbytes = size;

                GlobalUnlock(h);
            }
        }
    }
    else
    {
        if (wformat == CF_METAFILEPICT)
        {
            h = GlobalAlloc(0, sizeof(METAFILEPICT));
            if (h)
            {
                unsigned int wiresize;
                LPMETAFILEPICT lpmfp = GlobalLock(h);

                memcpy(lpmfp, hdata, sizeof(METAFILEPICT));
                wiresize = *lpcbytes - sizeof(METAFILEPICT);
                lpmfp->hMF = SetMetaFileBitsEx(wiresize,
                    ((const BYTE *)hdata) + sizeof(METAFILEPICT));
                GlobalUnlock(h);
            }
        }
        else if (wformat == CF_ENHMETAFILE)
        {
            h = SetEnhMetaFileBits(*lpcbytes, hdata);
        }
    }

    return h;
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
        CLIPBOARDINFO cbinfo;

        /* completely give up the selection */
        TRACE("Lost CLIPBOARD (+PRIMARY) selection\n");

        X11DRV_CLIPBOARD_GetClipboardInfo(&cbinfo);

        if (cbinfo.flags & CB_PROCESS)
        {
            /* Since we're still the owner, this wasn't initiated by
               another Wine process */
            if (OpenClipboard(hwnd))
            {
                /* Destroy private objects */
                SendMessageW(cbinfo.hWndOwner, WM_DESTROYCLIPBOARD, 0, 0);

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

        X11DRV_EmptyClipboard(FALSE);

        /* Reset the selection flags now that we are done */
        selectionAcquired = S_NOSELECTION;
    }
}


/**************************************************************************
 *		IsSelectionOwner (X11DRV.@)
 *
 * Returns: TRUE if the selection is owned by this process, FALSE otherwise
 */
static BOOL X11DRV_CLIPBOARD_IsSelectionOwner(void)
{
    return selectionAcquired;
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
 *		AcquireClipboard (X11DRV.@)
 */
int CDECL X11DRV_AcquireClipboard(HWND hWndClipWindow)
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

            return SendMessageW(hWndClipWindow, WM_X11DRV_ACQUIRE_SELECTION, 0, 0);
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

        if (!selectionThread)
        {
            WARN("Could not start clipboard thread\n");
            CloseHandle(event);
            return 0;
        }

        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
        CloseHandle(selectionThread);
    }

    return 1;
}


/**************************************************************************
 *	X11DRV_EmptyClipboard
 *
 * Empty cached clipboard data. 
 */
void CDECL X11DRV_EmptyClipboard(BOOL keepunowned)
{
    WINE_CLIPDATA *data, *next;

    LIST_FOR_EACH_ENTRY_SAFE( data, next, &data_list, WINE_CLIPDATA, entry )
    {
        if (keepunowned && (data->wFlags & CF_FLAG_UNOWNED)) continue;
        list_remove( &data->entry );
        X11DRV_CLIPBOARD_FreeData( data );
        HeapFree( GetProcessHeap(), 0, data );
        ClipDataCount--;
    }

    TRACE(" %d entries remaining in cache.\n", ClipDataCount);
}



/**************************************************************************
 *		X11DRV_SetClipboardData
 */
BOOL CDECL X11DRV_SetClipboardData(UINT wFormat, HANDLE hData, BOOL owner)
{
    DWORD flags = 0;
    BOOL bResult = TRUE;

    /* If it's not owned, data can only be set if the format data is not already owned
       and its rendering is not delayed */
    if (!owner)
    {
        CLIPBOARDINFO cbinfo;
        LPWINE_CLIPDATA lpRender;

        X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

        if (!hData ||
            ((lpRender = X11DRV_CLIPBOARD_LookupData(wFormat)) &&
            !(lpRender->wFlags & CF_FLAG_UNOWNED)))
            bResult = FALSE;
        else
            flags = CF_FLAG_UNOWNED;
    }

    bResult &= X11DRV_CLIPBOARD_InsertClipboardData(wFormat, hData, flags, NULL, TRUE);

    return bResult;
}


/**************************************************************************
 *		CountClipboardFormats
 */
INT CDECL X11DRV_CountClipboardFormats(void)
{
    CLIPBOARDINFO cbinfo;

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

    TRACE(" count=%d\n", ClipDataCount);

    return ClipDataCount;
}


/**************************************************************************
 *		X11DRV_EnumClipboardFormats
 */
UINT CDECL X11DRV_EnumClipboardFormats(UINT wFormat)
{
    CLIPBOARDINFO cbinfo;
    struct list *ptr = NULL;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

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
    CLIPBOARDINFO cbinfo;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

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
    CLIPBOARDINFO cbinfo;
    LPWINE_CLIPDATA lpRender;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

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
    X11DRV_EmptyClipboard(FALSE);
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_SynthesizeData
 */
static BOOL X11DRV_CLIPBOARD_SynthesizeData(UINT wFormatID)
{
    BOOL bsyn = TRUE;
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE(" %04x\n", wFormatID);

    /* Don't need to synthesize if it already exists */
    if (X11DRV_CLIPBOARD_LookupData(wFormatID))
        return TRUE;

    if (wFormatID == CF_UNICODETEXT || wFormatID == CF_TEXT || wFormatID == CF_OEMTEXT)
    {
        bsyn = ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_UNICODETEXT)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED) ||
            ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_TEXT)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED) ||
            ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_OEMTEXT)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED);
    }
    else if (wFormatID == CF_ENHMETAFILE)
    {
        bsyn = (lpSource = X11DRV_CLIPBOARD_LookupData(CF_METAFILEPICT)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED;
    }
    else if (wFormatID == CF_METAFILEPICT)
    {
        bsyn = (lpSource = X11DRV_CLIPBOARD_LookupData(CF_ENHMETAFILE)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED;
    }
    else if (wFormatID == CF_DIB)
    {
        bsyn = (lpSource = X11DRV_CLIPBOARD_LookupData(CF_BITMAP)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED;
    }
    else if (wFormatID == CF_BITMAP)
    {
        bsyn = (lpSource = X11DRV_CLIPBOARD_LookupData(CF_DIB)) &&
            ~lpSource->wFlags & CF_FLAG_SYNTHESIZED;
    }

    if (bsyn)
        X11DRV_CLIPBOARD_InsertClipboardData(wFormatID, 0, CF_FLAG_SYNTHESIZED, NULL, TRUE);

    return bsyn;
}



/**************************************************************************
 *              X11DRV_EndClipboardUpdate
 * TODO:
 *  Add locale if it hasn't already been added
 */
void CDECL X11DRV_EndClipboardUpdate(void)
{
    INT count = ClipDataCount;

    /* Do Unicode <-> Text <-> OEM mapping */
    X11DRV_CLIPBOARD_SynthesizeData(CF_TEXT);
    X11DRV_CLIPBOARD_SynthesizeData(CF_OEMTEXT);
    X11DRV_CLIPBOARD_SynthesizeData(CF_UNICODETEXT);

    /* Enhmetafile <-> MetafilePict mapping */
    X11DRV_CLIPBOARD_SynthesizeData(CF_ENHMETAFILE);
    X11DRV_CLIPBOARD_SynthesizeData(CF_METAFILEPICT);

    /* DIB <-> Bitmap mapping */
    X11DRV_CLIPBOARD_SynthesizeData(CF_DIB);
    X11DRV_CLIPBOARD_SynthesizeData(CF_BITMAP);

    TRACE("%d formats added to cached data\n", ClipDataCount - count);
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
            if ((format->wFormatID == lpData->wFormatID) &&
                format->lpDrvExportFunc && format->drvData)
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
            if ((format->wFormatID == lpData->wFormatID) &&
                format->lpDrvExportFunc && format->drvData)
                targets[i++] = format->drvData;

    if (TRACE_ON(clipboard))
    {
        unsigned int i;
        for ( i = 0; i < cTargets; i++)
        {
            char *itemFmtName = XGetAtomName(display, targets[i]);
            TRACE("\tAtom# %d:  Property %ld Type %s\n", i, targets[i], itemFmtName);
            XFree(itemFmtName);
        }
    }

    /* We may want to consider setting the type to xaTargets instead,
     * in case some apps expect this instead of XA_ATOM */
    XChangeProperty(display, requestor, rprop, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)targets, cTargets);

    HeapFree(GetProcessHeap(), 0, targets);

    return rprop;
}


/***********************************************************************
 *           X11DRV_SelectionRequest_MULTIPLE
 *  Service a MULTIPLE selection request event
 *  rprop contains a list of (target,property) atom pairs.
 *  The first atom names a target and the second names a property.
 *  The effect is as if we have received a sequence of SelectionRequest events
 *  (one for each atom pair) except that:
 *  1. We reply with a SelectionNotify only when all the requested conversions
 *  have been performed.
 *  2. If we fail to convert the target named by an atom in the MULTIPLE property,
 *  we replace the atom in the property by None.
 */
static Atom X11DRV_SelectionRequest_MULTIPLE( HWND hWnd, XSelectionRequestEvent *pevent )
{
    Display *display = pevent->display;
    Atom           rprop;
    Atom           atype=AnyPropertyType;
    int            aformat;
    unsigned long  remain;
    Atom*          targetPropList=NULL;
    unsigned long  cTargetPropList = 0;

    /* If the specified property is None the requestor is an obsolete client.
     * We support these by using the specified target atom as the reply property.
     */
    rprop = pevent->property;
    if( rprop == None )
        rprop = pevent->target;
    if (!rprop)
        return 0;

    /* Read the MULTIPLE property contents. This should contain a list of
     * (target,property) atom pairs.
     */
    if (!XGetWindowProperty(display, pevent->requestor, rprop,
                            0, 0x3FFF, False, AnyPropertyType, &atype,&aformat,
                            &cTargetPropList, &remain,
                            (unsigned char**)&targetPropList) != Success)
    {
        if (TRACE_ON(clipboard))
        {
            char * const typeName = XGetAtomName(display, atype);
            TRACE("\tType %s,Format %d,nItems %ld, Remain %ld\n",
                  typeName, aformat, cTargetPropList, remain);
            XFree(typeName);
        }

        /*
         * Make sure we got what we expect.
         * NOTE: According to the X-ICCCM Version 2.0 documentation the property sent
         * in a MULTIPLE selection request should be of type ATOM_PAIR.
         * However some X apps(such as XPaint) are not compliant with this and return
         * a user defined atom in atype when XGetWindowProperty is called.
         * The data *is* an atom pair but is not denoted as such.
         */
        if(aformat == 32 /* atype == xAtomPair */ )
        {
            unsigned int i;

            /* Iterate through the ATOM_PAIR list and execute a SelectionRequest
             * for each (target,property) pair */

            for (i = 0; i < cTargetPropList; i+=2)
            {
                XSelectionRequestEvent event;

                if (TRACE_ON(clipboard))
                {
                    char *targetName, *propName;
                    targetName = XGetAtomName(display, targetPropList[i]);
                    propName = XGetAtomName(display, targetPropList[i+1]);
                    TRACE("MULTIPLE(%d): Target='%s' Prop='%s'\n",
                          i/2, targetName, propName);
                    XFree(targetName);
                    XFree(propName);
                }

                /* We must have a non "None" property to service a MULTIPLE target atom */
                if ( !targetPropList[i+1] )
                {
                    TRACE("\tMULTIPLE(%d): Skipping target with empty property!\n", i);
                    continue;
                }

                /* Set up an XSelectionRequestEvent for this (target,property) pair */
                event = *pevent;
                event.target = targetPropList[i];
                event.property = targetPropList[i+1];

                /* Fire a SelectionRequest, informing the handler that we are processing
                 * a MULTIPLE selection request event.
                 */
                X11DRV_HandleSelectionRequest( hWnd, &event, TRUE );
            }
        }

        /* Free the list of targets/properties */
        XFree(targetPropList);
    }
    else TRACE("Couldn't read MULTIPLE property\n");

    return rprop;
}


/***********************************************************************
 *           X11DRV_HandleSelectionRequest
 *  Process an event selection request event.
 *  The bIsMultiple flag is used to signal when EVENT_SelectionRequest is called
 *  recursively while servicing a "MULTIPLE" selection target.
 *
 *  Note: We only receive this event when WINE owns the X selection
 */
static void X11DRV_HandleSelectionRequest( HWND hWnd, XSelectionRequestEvent *event, BOOL bIsMultiple )
{
    Display *display = event->display;
    XSelectionEvent result;
    Atom rprop = None;
    Window request = event->requestor;

    TRACE("\n");

    /*
     * We can only handle the selection request if :
     * The selection is PRIMARY or CLIPBOARD, AND we can successfully open the clipboard.
     * Don't do these checks or open the clipboard while recursively processing MULTIPLE,
     * since this has been already done.
     */
    if ( !bIsMultiple )
    {
        if (((event->selection != XA_PRIMARY) && (event->selection != x11drv_atom(CLIPBOARD))))
            goto END;
    }

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
    else if(event->target == x11drv_atom(MULTIPLE))  /*  rprop contains a list of (target, property) atom pairs */
    {
        /* MULTIPLE selection request */
        rprop = X11DRV_SelectionRequest_MULTIPLE( hWnd, event );
    }
    else
    {
        LPWINE_CLIPFORMAT lpFormat = X11DRV_CLIPBOARD_LookupProperty(NULL, event->target);

        if (lpFormat && lpFormat->lpDrvExportFunc)
        {
            LPWINE_CLIPDATA lpData = X11DRV_CLIPBOARD_LookupData(lpFormat->wFormatID);

            if (lpData)
            {
                unsigned char* lpClipData;
                DWORD cBytes;
                HANDLE hClipData = lpFormat->lpDrvExportFunc(display, request, event->target,
                                                             rprop, lpData, &cBytes);

                if (hClipData && (lpClipData = GlobalLock(hClipData)))
                {
                    int mode = PropModeReplace;

                    TRACE("\tUpdating property %s, %d bytes\n",
                          debugstr_format(lpFormat->wFormatID), cBytes);
                    do
                    {
                        int nelements = min(cBytes, 65536);
                        XChangeProperty(display, request, rprop, event->target,
                                        8, mode, lpClipData, nelements);
                        mode = PropModeAppend;
                        cBytes -= nelements;
                        lpClipData += nelements;
                    } while (cBytes > 0);

                    GlobalUnlock(hClipData);
                    GlobalFree(hClipData);
                }
            }
        }
    }

END:
    /* reply to sender
     * SelectionNotify should be sent only at the end of a MULTIPLE request
     */
    if ( !bIsMultiple )
    {
        result.type = SelectionNotify;
        result.display = display;
        result.requestor = request;
        result.selection = event->selection;
        result.property = rprop;
        result.target = event->target;
        result.time = event->time;
        TRACE("Sending SelectionNotify event...\n");
        XSendEvent(display,event->requestor,False,NoEventMask,(XEvent*)&result);
    }
}


/***********************************************************************
 *           X11DRV_SelectionRequest
 */
void X11DRV_SelectionRequest( HWND hWnd, XEvent *event )
{
    X11DRV_HandleSelectionRequest( hWnd, &event->xselectionrequest, FALSE );
}


/***********************************************************************
 *           X11DRV_SelectionClear
 */
void X11DRV_SelectionClear( HWND hWnd, XEvent *xev )
{
    XSelectionClearEvent *event = &xev->xselectionclear;
    if (event->selection == XA_PRIMARY || event->selection == x11drv_atom(CLIPBOARD))
        X11DRV_CLIPBOARD_ReleaseSelection( event->display, event->selection,
                                           event->window, hWnd, event->time );
}
