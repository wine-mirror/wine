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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
 */

#include "config.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <time.h>

#include "ts_xlib.h"
#include "winreg.h"
#include "clipboard.h"
#include "win.h"
#include "x11drv.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/server.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

#define HGDIOBJ_32(handle16)  ((HGDIOBJ)(ULONG_PTR)(handle16))

/* Maximum wait time for selection notify */
#define SELECTION_RETRIES 500  /* wait for .1 seconds */
#define SELECTION_WAIT    1000 /* us */
/* Minimum seconds that must lapse between owner queries */
#define OWNERQUERYLAPSETIME 1

/* Selection masks */
#define S_NOSELECTION    0
#define S_PRIMARY        1
#define S_CLIPBOARD      2

/* CLIPBOARD atom names */
#define _CLIPBOARD     "CLIPBOARD"
#define _TARGETS      "TARGETS"
#define _MULTIPLE     "MULTIPLE"
#define _SELECTIONDATA "SELECTION_DATA"
#define _TEXT          "TEXT"
#define _COMPOUNDTEXT  "COMPOUND_TEXT"

Atom xaClipboard = None;
Atom xaTargets = None;
Atom xaMultiple = None;
Atom xaSelectionData = None;
Atom xaText = None;
Atom xaCompoundText = None;

static int selectionAcquired = 0;              /* Contains the current selection masks */
static Window selectionWindow = None;          /* The top level X window which owns the selection */
static BOOL clearAllSelections = FALSE;        /* Always lose all selections */
static Atom selectionCacheSrc = XA_PRIMARY;    /* The selection source from which the clipboard cache was filled */
static Window PrimarySelectionOwner = None;    /* The window which owns the primary selection */
static Window ClipboardSelectionOwner = None;  /* The window which owns the clipboard selection */

INT X11DRV_RegisterClipboardFormat(LPCSTR FormatName);
void X11DRV_EmptyClipboard(void);
void X11DRV_EndClipboardUpdate(void);
HANDLE X11DRV_CLIPBOARD_ImportClipboardData(LPBYTE lpdata, UINT cBytes);
HANDLE X11DRV_CLIPBOARD_ImportEnhMetaFile(LPBYTE lpdata, UINT cBytes);
HANDLE X11DRV_CLIPBOARD_ImportMetaFilePict(LPBYTE lpdata, UINT cBytes);
HANDLE X11DRV_CLIPBOARD_ImportXAPIXMAP(LPBYTE lpdata, UINT cBytes);
HANDLE X11DRV_CLIPBOARD_ImportXAString(LPBYTE lpdata, UINT cBytes);
HANDLE X11DRV_CLIPBOARD_ExportClipboardData(Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpData, LPDWORD lpBytes);
HANDLE X11DRV_CLIPBOARD_ExportString(Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpData, LPDWORD lpBytes);
HANDLE X11DRV_CLIPBOARD_ExportXAPIXMAP(Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
HANDLE X11DRV_CLIPBOARD_ExportMetaFilePict(Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
HANDLE X11DRV_CLIPBOARD_ExportEnhMetaFile(Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpdata, LPDWORD lpBytes);
UINT X11DRV_CLIPBOARD_InsertClipboardFormat(LPCSTR FormatName, LPCSTR PropertyName);
static BOOL X11DRV_CLIPBOARD_ReadSelection(LPWINE_CLIPFORMAT lpData, Window w, Atom prop);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedText(UINT wFormatID);
static void X11DRV_CLIPBOARD_FreeData(LPWINE_CLIPDATA lpData);
static BOOL X11DRV_CLIPBOARD_IsSelectionOwner(void);
static int X11DRV_CLIPBOARD_QueryAvailableData(LPCLIPBOARDINFO lpcbinfo);
static BOOL X11DRV_CLIPBOARD_ReadClipboardData(UINT wFormat);
static BOOL X11DRV_CLIPBOARD_RenderFormat(LPWINE_CLIPDATA lpData);
static HANDLE X11DRV_CLIPBOARD_SerializeMetafile(INT wformat, HANDLE hdata, LPDWORD lpcbytes, BOOL out);
static BOOL X11DRV_CLIPBOARD_SynthesizeData(UINT wFormatID);
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedFormat(LPWINE_CLIPDATA lpData);

/* Clipboard formats
 * WARNING: This data ordering is dependent on the WINE_CLIPFORMAT structure
 * declared in clipboard.h
 */
WINE_CLIPFORMAT ClipFormats[]  =
{
    { CF_TEXT, "WCF_TEXT",  0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, NULL, &ClipFormats[1]},

    { CF_BITMAP, "WCF_BITMAP", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[0], &ClipFormats[2]},

    { CF_METAFILEPICT, "WCF_METAFILEPICT", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportMetaFilePict,
        X11DRV_CLIPBOARD_ExportMetaFilePict, &ClipFormats[1], &ClipFormats[3]},

    { CF_SYLK, "WCF_SYLK", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[2], &ClipFormats[4]},

    { CF_DIF, "WCF_DIF", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[3], &ClipFormats[5]},

    { CF_TIFF, "WCF_TIFF", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[4], &ClipFormats[6]},

    { CF_OEMTEXT, "WCF_OEMTEXT", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[5], &ClipFormats[7]},

    { CF_DIB, "WCF_DIB", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportXAPIXMAP,
	X11DRV_CLIPBOARD_ExportXAPIXMAP, &ClipFormats[6], &ClipFormats[8]},

    { CF_PALETTE, "WCF_PALETTE", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[7], &ClipFormats[9]},

    { CF_PENDATA, "WCF_PENDATA", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[8], &ClipFormats[10]},

    { CF_RIFF, "WCF_RIFF", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[9], &ClipFormats[11]},

    { CF_WAVE, "WCF_WAVE", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[10], &ClipFormats[12]},

    { CF_UNICODETEXT, "WCF_UNICODETEXT", XA_STRING, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportXAString,
	X11DRV_CLIPBOARD_ExportString, &ClipFormats[11], &ClipFormats[13]},

    { CF_ENHMETAFILE, "WCF_ENHMETAFILE", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportEnhMetaFile,
        X11DRV_CLIPBOARD_ExportEnhMetaFile, &ClipFormats[12], &ClipFormats[14]},

    { CF_HDROP, "WCF_HDROP", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[13], &ClipFormats[15]},

    { CF_LOCALE, "WCF_LOCALE", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[14], &ClipFormats[16]},

    { CF_DIBV5, "WCF_DIBV5", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[15], &ClipFormats[17]},

    { CF_OWNERDISPLAY, "WCF_OWNERDISPLAY", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[16], &ClipFormats[18]},

    { CF_DSPTEXT, "WCF_DSPTEXT", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[17], &ClipFormats[19]},

    { CF_DSPBITMAP, "WCF_DSPBITMAP", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[18], &ClipFormats[20]},

    { CF_DSPMETAFILEPICT, "WCF_DSPMETAFILEPICT", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[19], &ClipFormats[21]},

    { CF_DSPENHMETAFILE, "WCF_DSPENHMETAFILE", 0, CF_FLAG_BUILTINFMT, X11DRV_CLIPBOARD_ImportClipboardData,
        X11DRV_CLIPBOARD_ExportClipboardData, &ClipFormats[20], NULL}
};


/* Maps X properties to Windows formats */
PROPERTYFORMATMAP PropertyFormatMap[] =
{ 
    { "text/rtf", "Rich Text Format" },
    /* Temporarily disable text/html because Evolution incorrectly pastes strings with extra nulls */
    /*{ "text/html", "HTML Format" },*/
    { "image/gif", "GIF" },
};


/* Maps equivalent X properties. It is assumed that lpszProperty must already 
   be in ClipFormats or PropertyFormatMap. */
PROPERTYALIASMAP PropertyAliasMap[] =
{ 
    /* lpszProperty, Alias */
    { "text/rtf", 0, "text/richtext", 0 },
    { "XAString", XA_STRING, _COMPOUNDTEXT, 0 },
    { "XAString", XA_STRING, _TEXT, 0 },
};


/*
 * Cached clipboard data.
 */
static LPWINE_CLIPDATA ClipData = NULL;
static UINT ClipDataCount = 0;

/*
 * Clipboard sequence number
 */
UINT wSeqNo = 0;

/**************************************************************************
 *                Internal Clipboard implementation methods
 **************************************************************************/

/**************************************************************************
 *		X11DRV_InitClipboard
 */
BOOL X11DRV_InitClipboard(Display *display)
{
    INT i;
    HKEY hkey;
    PROPERTYALIASMAP *lpalias;
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;

    xaClipboard = TSXInternAtom( display, _CLIPBOARD, FALSE );
    xaTargets = TSXInternAtom( display, _TARGETS, FALSE );
    xaMultiple = TSXInternAtom(display, _MULTIPLE, False);
    xaSelectionData = TSXInternAtom(display, _SELECTIONDATA, False);
    xaText = TSXInternAtom(display, _TEXT, False);
    xaCompoundText = TSXInternAtom(display, _COMPOUNDTEXT, False);

    if(!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Clipboard", &hkey))
    {
	char buffer[20];
	DWORD type, count = sizeof(buffer);
	if(!RegQueryValueExA(hkey, "ClearAllSelections", 0, &type, buffer, &count))
	    clearAllSelections = atoi(buffer);
        RegCloseKey(hkey);
    }

    /* Register known formats */
    while (lpFormat)
    {
        if (!lpFormat->wFormatID)
            lpFormat->wFormatID = GlobalAddAtomA(lpFormat->Name);

        if (!lpFormat->drvData)
            lpFormat->drvData = TSXInternAtom(display, lpFormat->Name, False);

	lpFormat = lpFormat->NextFormat;
    }

    /* Register known mapping between window formats and X properties */
    for (i = 0; i < sizeof(PropertyFormatMap)/sizeof(PROPERTYFORMATMAP); i++)
        X11DRV_CLIPBOARD_InsertClipboardFormat(PropertyFormatMap[i].lpszFormat, 
            PropertyFormatMap[i].lpszProperty);

    /* Register known mapping between X properties */
    for (i = 0; i < sizeof(PropertyAliasMap)/sizeof(PROPERTYALIASMAP); i++)
    {
        lpalias = &PropertyAliasMap[i];
        if (!lpalias->drvDataProperty)
        lpalias->drvDataProperty = TSXInternAtom(display, lpalias->lpszProperty, False);
        if (!lpalias->drvDataAlias)
        lpalias->drvDataAlias = TSXInternAtom(display, lpalias->lpszAlias, False);
    }

    return TRUE;
}


/**************************************************************************
 *                X11DRV_CLIPBOARD_LookupFormat
 */
LPWINE_CLIPFORMAT X11DRV_CLIPBOARD_LookupFormat(WORD wID)
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;

    while(lpFormat)
    {
        if (lpFormat->wFormatID == wID) 
            break;

	lpFormat = lpFormat->NextFormat;
    }

    return lpFormat;
}


/**************************************************************************
 *                X11DRV_CLIPBOARD_LookupProperty
 */
LPWINE_CLIPFORMAT X11DRV_CLIPBOARD_LookupProperty(UINT drvData)
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;

    while(lpFormat)
    {
        if (lpFormat->drvData == drvData) 
            break;

	lpFormat = lpFormat->NextFormat;
    }

    return lpFormat;
}


/**************************************************************************
 *                X11DRV_CLIPBOARD_LookupAliasProperty
 */
LPWINE_CLIPFORMAT X11DRV_CLIPBOARD_LookupAliasProperty(UINT drvDataAlias)
{
    unsigned int i;
    LPWINE_CLIPFORMAT lpFormat = NULL;

    for (i = 0; i < sizeof(PropertyAliasMap)/sizeof(PROPERTYALIASMAP); i++)
    {
        if (PropertyAliasMap[i].drvDataAlias == drvDataAlias)
        {
            lpFormat = X11DRV_CLIPBOARD_LookupProperty(PropertyAliasMap[i].drvDataProperty);
            break;
        }
   }

    return lpFormat;
}


/**************************************************************************
 *                X11DRV_CLIPBOARD_LookupPropertyAlias
 */
UINT  X11DRV_CLIPBOARD_LookupPropertyAlias(UINT drvDataProperty)
{
    unsigned int i;
    UINT alias = 0;

    for (i = 0; i < sizeof(PropertyAliasMap)/sizeof(PROPERTYALIASMAP); i++)
    {
        if (PropertyAliasMap[i].drvDataProperty == drvDataProperty)
        {
            alias = PropertyAliasMap[i].drvDataAlias;
            break;
        }
   }

    return alias;
}


/**************************************************************************
 *               X11DRV_CLIPBOARD_LookupData
 */
LPWINE_CLIPDATA X11DRV_CLIPBOARD_LookupData(DWORD wID)
{
    LPWINE_CLIPDATA lpData = ClipData;

    if (lpData)
    {
        do
        {
            if (lpData->wFormatID == wID) 
                break;

	    lpData = lpData->NextData;
        }
        while(lpData != ClipData);

        if (lpData->wFormatID != wID)
            lpData = NULL;
    }

    return lpData;
}


/**************************************************************************
 *		InsertClipboardFormat
 */
UINT X11DRV_CLIPBOARD_InsertClipboardFormat(LPCSTR FormatName, LPCSTR PropertyName)
{
    LPWINE_CLIPFORMAT lpFormat;
    LPWINE_CLIPFORMAT lpNewFormat;
   
    /* allocate storage for new format entry */
    lpNewFormat = (LPWINE_CLIPFORMAT) HeapAlloc(GetProcessHeap(), 
        0, sizeof(WINE_CLIPFORMAT));

    if(lpNewFormat == NULL) 
    {
        WARN("No more memory for a new format!\n");
        return 0;
    }

    if (!(lpNewFormat->Name = HeapAlloc(GetProcessHeap(), 0, strlen(FormatName)+1)))
    {
        WARN("No more memory for the new format name!\n");
        HeapFree(GetProcessHeap(), 0, lpNewFormat);
        return 0;
    }

    strcpy(lpNewFormat->Name, FormatName);
    lpNewFormat->wFlags = 0;
    lpNewFormat->wFormatID = GlobalAddAtomA(lpNewFormat->Name);
    lpNewFormat->drvData = TSXInternAtom(thread_display(), PropertyName, False);
    lpNewFormat->lpDrvImportFunc = X11DRV_CLIPBOARD_ImportClipboardData;
    lpNewFormat->lpDrvExportFunc = X11DRV_CLIPBOARD_ExportClipboardData;

    /* Link Format */
    lpFormat = ClipFormats;

    while(lpFormat->NextFormat) /* Move to last entry */
	lpFormat = lpFormat->NextFormat;

    lpNewFormat->NextFormat = NULL;
    lpFormat->NextFormat = lpNewFormat;
    lpNewFormat->PrevFormat = lpFormat;

    TRACE("Registering format(%d): %s drvData(%d): %s\n", 
        lpNewFormat->wFormatID, 
        FormatName,
        lpNewFormat->drvData, 
        PropertyName);

    return lpNewFormat->wFormatID;
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
            cbInfo->hWndOpen = reply->old_clipboard;
            cbInfo->hWndOwner = reply->old_owner;
            cbInfo->hWndViewer = reply->old_viewer;
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
static BOOL X11DRV_CLIPBOARD_InsertClipboardData(UINT wFormat, HANDLE16 hData16, HANDLE hData32, DWORD flags)
{
    LPWINE_CLIPDATA lpData = X11DRV_CLIPBOARD_LookupData(wFormat);

    TRACE("format=%d lpData=%p hData16=%08x hData32=%08x flags=0x%08lx\n", 
        wFormat, lpData, hData16, (unsigned int)hData32, flags);

    if (lpData)
    {
        X11DRV_CLIPBOARD_FreeData(lpData);

        lpData->hData16 = hData16;  /* 0 is legal, see WM_RENDERFORMAT */
        lpData->hData32 = hData32;
    }
    else
    {
        lpData = (LPWINE_CLIPDATA) HeapAlloc(GetProcessHeap(), 
            0, sizeof(WINE_CLIPDATA));

        lpData->wFormatID = wFormat;
        lpData->hData16 = hData16;  /* 0 is legal, see WM_RENDERFORMAT */
        lpData->hData32 = hData32;

	if (ClipData)
        {
            LPWINE_CLIPDATA lpPrevData = ClipData->PrevData;

            lpData->PrevData = lpPrevData;
            lpData->NextData = ClipData;

            lpPrevData->NextData = lpData;
            ClipData->PrevData = lpData;
        }
        else
        {
            lpData->NextData = lpData;
            lpData->PrevData = lpData;
            ClipData = lpData;
        }

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
    TRACE("%d\n", lpData->wFormatID);

    if ((lpData->wFormatID >= CF_GDIOBJFIRST &&
        lpData->wFormatID <= CF_GDIOBJLAST) || 
        lpData->wFormatID == CF_BITMAP || 
        lpData->wFormatID == CF_DIB || 
        lpData->wFormatID == CF_PALETTE)
    {
      if (lpData->hData32)
	DeleteObject(lpData->hData32);

      if (lpData->hData16)
	DeleteObject(HGDIOBJ_32(lpData->hData16));
    }
    else if (lpData->wFormatID == CF_METAFILEPICT)
    {
      if (lpData->hData32)
      {
        DeleteMetaFile(((METAFILEPICT *)GlobalLock( lpData->hData32 ))->hMF );
        GlobalFree(lpData->hData32);

	if (lpData->hData16)
	  /* HMETAFILE16 and HMETAFILE32 are apparently the same thing,
	     and a shallow copy is enough to share a METAFILEPICT
	     structure between 16bit and 32bit clipboards.  The MetaFile
	     should of course only be deleted once. */
	  GlobalFree16(lpData->hData16);
      }

      if (lpData->hData16)
      {
        METAFILEPICT16* lpMetaPict = (METAFILEPICT16 *) GlobalLock16(lpData->hData16);

        if (lpMetaPict)
        {
            DeleteMetaFile16(lpMetaPict->hMF);
            lpMetaPict->hMF = 0;
        }

	GlobalFree16(lpData->hData16);
      }
    }
    else if (lpData->wFormatID == CF_ENHMETAFILE)
    {
        if (lpData->hData32)
            DeleteEnhMetaFile(lpData->hData32);
    }
    else if (lpData->wFormatID < CF_PRIVATEFIRST ||
             lpData->wFormatID > CF_PRIVATELAST)
    {
      if (lpData->hData32)
        GlobalFree(lpData->hData32);

      if (lpData->hData16)
	GlobalFree16(lpData->hData16);
    }

    lpData->hData16 = 0;
    lpData->hData32 = 0;
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
            X11DRV_EmptyClipboard();

            if (X11DRV_CLIPBOARD_QueryAvailableData(lpcbinfo) < 0)
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
static BOOL X11DRV_CLIPBOARD_RenderFormat(LPWINE_CLIPDATA lpData)
{
    BOOL bret = TRUE;

    TRACE(" 0x%04x hData32(0x%08x) hData16(0x%08x)\n", 
        lpData->wFormatID, (unsigned int)lpData->hData32, lpData->hData16);

    if (lpData->hData32 || lpData->hData16)
        return bret; /* Already rendered */

    if (lpData->wFlags & CF_FLAG_SYNTHESIZED)
        bret = X11DRV_CLIPBOARD_RenderSynthesizedFormat(lpData);
    else if (!X11DRV_CLIPBOARD_IsSelectionOwner())
    {
        if (!X11DRV_CLIPBOARD_ReadClipboardData(lpData->wFormatID))
        {
            ERR("Failed to cache clipboard data owned by another process. Format=%d\n", 
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
                SendMessageW(cbInfo.hWndOwner, WM_RENDERFORMAT, (WPARAM)lpData->wFormatID, 0);

	        if (!lpData->hData32 && !lpData->hData16)
                    bret = FALSE;
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
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedFormat(LPWINE_CLIPDATA lpData)
{
    BOOL bret = FALSE;

    TRACE("\n");

    if (lpData->wFlags & CF_FLAG_SYNTHESIZED)
    {
        UINT wFormatID = lpData->wFormatID;

        if (wFormatID == CF_UNICODETEXT || wFormatID == CF_TEXT || wFormatID == CF_OEMTEXT)
            bret = X11DRV_CLIPBOARD_RenderSynthesizedText(wFormatID);
        else 
        {
            switch (wFormatID)
	    {
                case CF_ENHMETAFILE:
                case CF_METAFILEPICT:
                case CF_DIB:
                case CF_BITMAP:
		    FIXME("Synthesizing wFormatID(0x%08x) not implemented\n", wFormatID);
                    break;

                default:
		    FIXME("Called to synthesize unknown format\n");
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
static BOOL X11DRV_CLIPBOARD_RenderSynthesizedText(UINT wFormatID)
{
    LPCSTR lpstrS;
    LPSTR  lpstrT;
    HANDLE hData32;
    INT src_chars, dst_chars, alloc_size;
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE(" %d\n", wFormatID);

    if ((lpSource = X11DRV_CLIPBOARD_LookupData(wFormatID)) &&
        lpSource->hData32)
        return TRUE;

    /* Look for rendered source or non-synthesized source */
    if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_UNICODETEXT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData32))
    {
        TRACE("UNICODETEXT -> %d\n", wFormatID);
    }
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_TEXT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData32))
    {
        TRACE("TEXT -> %d\n", wFormatID);
    }
    else if ((lpSource = X11DRV_CLIPBOARD_LookupData(CF_OEMTEXT)) &&
        (!(lpSource->wFlags & CF_FLAG_SYNTHESIZED) || lpSource->hData32))
    {
        TRACE("OEMTEXT -> %d\n", wFormatID);
    }

    if (!lpSource || (lpSource->wFlags & CF_FLAG_SYNTHESIZED &&
        !lpSource->hData32))
        return FALSE;

    /* Ask the clipboard owner to render the source text if necessary */
    if (!lpSource->hData32 && !X11DRV_CLIPBOARD_RenderFormat(lpSource))
        return FALSE;

    if (lpSource->hData32)
    {
        lpstrS = (LPSTR)GlobalLock(lpSource->hData32);
    }
    else
    {
        lpstrS = (LPSTR)GlobalLock16(lpSource->hData16);
    }

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

    TRACE("Converting from '%d' to '%d', %i chars\n",
    	lpSource->wFormatID, wFormatID, src_chars);

    /* Convert characters to bytes */
    if(wFormatID == CF_UNICODETEXT)
        alloc_size = dst_chars * sizeof(WCHAR);
    else
        alloc_size = dst_chars;

    hData32 = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | 
        GMEM_DDESHARE, alloc_size);

    lpstrT = (LPSTR)GlobalLock(hData32);

    if (lpstrT)
    {
        CLIPBOARD_ConvertText(lpSource->wFormatID, lpstrS, src_chars,
            wFormatID, lpstrT, dst_chars);
        GlobalUnlock(hData32);
    }

    /* Unlock source */
    if (lpSource->hData32)
        GlobalUnlock(lpSource->hData32);
    else
        GlobalUnlock16(lpSource->hData16);

    return X11DRV_CLIPBOARD_InsertClipboardData(wFormatID, 0, hData32, 0);
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportXAString
 *
 *  Import XA_STRING, converting the string to CF_UNICODE.
 */
HANDLE X11DRV_CLIPBOARD_ImportXAString(LPBYTE lpdata, UINT cBytes)
{
    LPSTR lpstr;
    UINT i, inlcount = 0;
    HANDLE hUnicodeText = 0;

    for (i = 0; i <= cBytes; i++)
    {
        if (lpdata[i] == '\n')
            inlcount++;
    }

    if ((lpstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cBytes + inlcount + 1)))
    {
        UINT count;

        for (i = 0, inlcount = 0; i <= cBytes; i++)
        {
            if (lpdata[i] == '\n')
                lpstr[inlcount++] = '\r';

            lpstr[inlcount++] = lpdata[i];
        }

	count = MultiByteToWideChar(CP_UNIXCP, 0, lpstr, -1, NULL, 0);
	hUnicodeText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, count * sizeof(WCHAR));

	if(hUnicodeText)
	{
            WCHAR *textW = GlobalLock(hUnicodeText);
            MultiByteToWideChar(CP_UNIXCP, 0, lpstr, -1, textW, count);
            GlobalUnlock(hUnicodeText);
        }

        HeapFree(GetProcessHeap(), 0, lpstr);
    }

    return hUnicodeText;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportXAPIXMAP
 *
 *  Import XA_PIXMAP, converting the image to CF_DIB.
 */
HANDLE X11DRV_CLIPBOARD_ImportXAPIXMAP(LPBYTE lpdata, UINT cBytes)
{
    HANDLE hTargetImage = 0;  /* Handle to store the converted DIB */
    Pixmap *pPixmap = (Pixmap *) lpdata;
    HWND hwnd = GetOpenClipboardWindow();
    HDC hdc = GetDC(hwnd);

    hTargetImage = X11DRV_DIB_CreateDIBFromPixmap(*pPixmap, hdc, TRUE);

    ReleaseDC(hwnd, hdc);

    return hTargetImage;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ImportMetaFilePict
 *
 *  Import MetaFilePict.
 */
HANDLE X11DRV_CLIPBOARD_ImportMetaFilePict(LPBYTE lpdata, UINT cBytes)
{
    return X11DRV_CLIPBOARD_SerializeMetafile(CF_METAFILEPICT, (HANDLE)lpdata, (LPDWORD)&cBytes, FALSE);
}


/**************************************************************************
 *		X11DRV_ImportEnhMetaFile
 *
 *  Import EnhMetaFile.
 */
HANDLE X11DRV_CLIPBOARD_ImportEnhMetaFile(LPBYTE lpdata, UINT cBytes)
{
    return X11DRV_CLIPBOARD_SerializeMetafile(CF_ENHMETAFILE, (HANDLE)lpdata, (LPDWORD)&cBytes, FALSE);
}


/**************************************************************************
 *		X11DRV_ImportClipbordaData
 *
 *  Generic import clipboard data routine.
 */
HANDLE X11DRV_CLIPBOARD_ImportClipboardData(LPBYTE lpdata, UINT cBytes)
{
    LPVOID lpClipData;
    HANDLE hClipData = 0;

    if (cBytes)
    {
        /* Turn on the DDESHARE flag to enable shared 32 bit memory */
        hClipData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cBytes);
        if ((lpClipData = GlobalLock(hClipData)))
        {
            memcpy(lpClipData, lpdata, cBytes);
            GlobalUnlock(hClipData);
        }
        else
            hClipData = 0;
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportClipboardData
 *
 *  Generic export clipboard data routine.
 */
HANDLE X11DRV_CLIPBOARD_ExportClipboardData(Window requestor, Atom aTarget,
    Atom rprop, LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    LPVOID lpClipData;
    UINT cBytes = 0;
    HANDLE hClipData = 0;

    *lpBytes = 0; /* Assume failure */

    if (!X11DRV_CLIPBOARD_RenderFormat(lpData))
        ERR("Failed to export %d format\n", lpData->wFormatID);
    else
    {
        cBytes = GlobalSize(lpData->hData32);

        hClipData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cBytes);

        if ((lpClipData = GlobalLock(hClipData)))
        {
            LPVOID lpdata = GlobalLock(lpData->hData32);

            memcpy(lpClipData, lpdata, cBytes);
	    *lpBytes = cBytes;

            GlobalUnlock(lpData->hData32);
            GlobalUnlock(hClipData);
        }
    }

    return hClipData;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportXAString
 *
 *  Export CF_UNICODE converting the string to XA_STRING.
 *  Helper function for X11DRV_CLIPBOARD_ExportString.
 */
HANDLE X11DRV_CLIPBOARD_ExportXAString(LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    INT i, j;
    UINT size;
    LPWSTR uni_text;
    LPSTR text, lpstr;

    *lpBytes = 0; /* Assume return has zero bytes */

    uni_text = GlobalLock(lpData->hData32);

    size = WideCharToMultiByte(CP_UNIXCP, 0, uni_text, -1, NULL, 0, NULL, NULL);

    text = HeapAlloc(GetProcessHeap(), 0, size);
    if (!text)
       return None;
    WideCharToMultiByte(CP_UNIXCP, 0, uni_text, -1, text, size, NULL, NULL);

    /* remove carriage returns */

    lpstr = (char*)HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, size-- );
    if(lpstr == NULL) return None;
    for(i = 0,j = 0; i < size && text[i]; i++ )
    {
        if( text[i] == '\r' &&
            (text[i+1] == '\n' || text[i+1] == '\0') ) continue;
        lpstr[j++] = text[i];
    }
    lpstr[j]='\0';

    *lpBytes = j; /* Number of bytes in string */

    HeapFree(GetProcessHeap(), 0, text);
    GlobalUnlock(lpData->hData32);

    return lpstr;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportCompoundText
 *
 *  Export CF_UNICODE to COMPOUND_TEXT or TEXT
 *  Helper function for X11DRV_CLIPBOARD_ExportString.
 */
HANDLE X11DRV_CLIPBOARD_ExportCompoundText(Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    Display *display = thread_display();
    char* lpstr = 0;
    XTextProperty prop;
    XICCEncodingStyle style;

    lpstr = (char*) X11DRV_CLIPBOARD_ExportXAString(lpData, lpBytes);

    if (lpstr)
    {
        if (aTarget == xaCompoundText)
           style = XCompoundTextStyle;
        else
           style = XStdICCTextStyle;

        /* Update the X property */
        wine_tsx11_lock();
        if (XmbTextListToTextProperty(display, &lpstr, 1, style, &prop) == Success)
        {
           XSetTextProperty(display, requestor, &prop, rprop);
           XFree(prop.value);
        }
        wine_tsx11_unlock();

        HeapFree( GetProcessHeap(), 0, lpstr );
    }

    return 0;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportString
 *
 *  Export CF_UNICODE string
 */
HANDLE X11DRV_CLIPBOARD_ExportString(Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpData, LPDWORD lpBytes)
{
    if (X11DRV_CLIPBOARD_RenderFormat(lpData))
    {
        if (aTarget == XA_STRING)
            return X11DRV_CLIPBOARD_ExportXAString(lpData, lpBytes);
        else if (aTarget == xaCompoundText || aTarget == xaText)
            return X11DRV_CLIPBOARD_ExportCompoundText(requestor, aTarget,
                rprop, lpData, lpBytes);
        else
            ERR("Unknown target %ld to %d format\n", aTarget, lpData->wFormatID);
    }
    else
        ERR("Failed to render %d format\n", lpData->wFormatID);

    return 0;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportXAPIXMAP
 *
 *  Export CF_DIB to XA_PIXMAP.
 */
HANDLE X11DRV_CLIPBOARD_ExportXAPIXMAP(Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    HDC hdc;
    Pixmap pixmap;

    if (!X11DRV_CLIPBOARD_RenderFormat(lpdata))
    {
        ERR("Failed to export %d format\n", lpdata->wFormatID);
        return 0;
    }

    hdc = GetDC(0);

    /* For convert from packed DIB to Pixmap */
    pixmap = X11DRV_DIB_CreatePixmapFromDIB(lpdata, hdc);
    *lpBytes = 4; /* pixmap is a 32bit value */

    ReleaseDC(0, hdc);

    return (HANDLE) pixmap;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportMetaFilePict
 *
 *  Export MetaFilePict.
 */
HANDLE X11DRV_CLIPBOARD_ExportMetaFilePict(Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    if (!X11DRV_CLIPBOARD_RenderFormat(lpdata))
    {
        ERR("Failed to export %d format\n", lpdata->wFormatID);
        return 0;
    }

    return X11DRV_CLIPBOARD_SerializeMetafile(CF_METAFILEPICT, (HANDLE)lpdata->hData32, 
        lpBytes, TRUE);
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ExportEnhMetaFile
 *
 *  Export EnhMetaFile.
 */
HANDLE X11DRV_CLIPBOARD_ExportEnhMetaFile(Window requestor, Atom aTarget, Atom rprop,
    LPWINE_CLIPDATA lpdata, LPDWORD lpBytes)
{
    if (!X11DRV_CLIPBOARD_RenderFormat(lpdata))
    {
        ERR("Failed to export %d format\n", lpdata->wFormatID);
        return 0;
    }

    return X11DRV_CLIPBOARD_SerializeMetafile(CF_ENHMETAFILE, (HANDLE)lpdata->hData32, 
        lpBytes, TRUE);
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_QueryTargets
 */
static BOOL X11DRV_CLIPBOARD_QueryTargets(Display *display, Window w, Atom selection, XEvent *xe)
{
    INT i;
    Bool res;

    wine_tsx11_lock();
    XConvertSelection(display, selection, xaTargets, xaSelectionData, w, CurrentTime);
    wine_tsx11_unlock();

    /*
     * Wait until SelectionNotify is received
     */
    for (i = 0; i < SELECTION_RETRIES; i++)
    {
        wine_tsx11_lock();
        res = XCheckTypedWindowEvent(display, w, SelectionNotify, xe);
        wine_tsx11_unlock();
        if (res && xe->xselection.selection == selection) break;

        usleep(SELECTION_WAIT);
    }

    /* Verify that the selection returned a valid TARGETS property */
    if ((xe->xselection.target != xaTargets) || (xe->xselection.property == None))
    {
        /* Selection owner failed to respond or we missed the SelectionNotify */
        WARN("Failed to retrieve TARGETS for selection %ld.\n", selection);
        return FALSE;
    }

    return TRUE;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_QueryAvailableData
 *
 * Caches the list of data formats available from the current selection.
 * This queries the selection owner for the TARGETS property and saves all
 * reported property types.
 */
static int X11DRV_CLIPBOARD_QueryAvailableData(LPCLIPBOARDINFO lpcbinfo)
{
    Display *display = thread_display();
    XEvent         xe;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetList=NULL;
    Window         w;
    HWND           hWndClipWindow; 
    unsigned long  cSelectionTargets = 0;

    if (selectionAcquired & (S_PRIMARY | S_CLIPBOARD))
    {
        ERR("Received request to cache selection but process is owner=(%08x)\n", 
            (unsigned) selectionWindow);

        selectionAcquired  = S_NOSELECTION;

        if (TSXGetSelectionOwner(display,XA_PRIMARY) == selectionWindow)
	    selectionAcquired |= S_PRIMARY;

        if (TSXGetSelectionOwner(display,xaClipboard) == selectionWindow)
	    selectionAcquired |= S_CLIPBOARD;

        if (!(selectionAcquired == (S_PRIMARY | S_CLIPBOARD)))
        {
            WARN("Lost selection but process didn't process SelectClear\n");
            selectionWindow = None;
        }
	else
	{
            return -1; /* Prevent self request */
	}
    }

    if (lpcbinfo->flags & CB_OWNER)
        hWndClipWindow = lpcbinfo->hWndOwner;
    else if (lpcbinfo->flags & CB_OPEN)
        hWndClipWindow = lpcbinfo->hWndOpen;
    else
        hWndClipWindow = GetActiveWindow();

    if (!hWndClipWindow)
    {
        WARN("No window available to retrieve selection!n");
        return -1;
    }

    w = X11DRV_get_whole_window(GetAncestor(hWndClipWindow, GA_ROOT));

    /*
     * Query the selection owner for the TARGETS property
     */
    if (TSXGetSelectionOwner(display,XA_PRIMARY) ||
        TSXGetSelectionOwner(display,xaClipboard))
    {
    if (X11DRV_CLIPBOARD_QueryTargets(display, w, XA_PRIMARY, &xe))
        selectionCacheSrc = XA_PRIMARY;
    else if (X11DRV_CLIPBOARD_QueryTargets(display, w, xaClipboard, &xe))
        selectionCacheSrc = xaClipboard;
    else
            return -1;
    }
    else /* No selection owner so report 0 targets available */
        return 0;

    /* Read the TARGETS property contents */
    if(TSXGetWindowProperty(display, xe.xselection.requestor, xe.xselection.property,
        0, 0x3FFF, True, AnyPropertyType/*XA_ATOM*/, &atype, &aformat, &cSelectionTargets, 
        &remain, (unsigned char**)&targetList) != Success)
    {
        WARN("Failed to read TARGETS property\n");
    }
    else
    {
       TRACE("Type %s,Format %d,nItems %ld, Remain %ld\n",
             TSXGetAtomName(display,atype),aformat,cSelectionTargets, remain);
       /*
        * The TARGETS property should have returned us a list of atoms
        * corresponding to each selection target format supported.
        */
       if ((atype == XA_ATOM || atype == xaTargets) && aformat == 32)
       {
          INT i;

          /* Cache these formats in the clipboard cache */
          for (i = 0; i < cSelectionTargets; i++)
          {
              LPWINE_CLIPFORMAT lpFormat = X11DRV_CLIPBOARD_LookupProperty(targetList[i]);

              if (!lpFormat)
                  lpFormat = X11DRV_CLIPBOARD_LookupAliasProperty(targetList[i]);

              if (!lpFormat)
              {
                  LPSTR lpName = TSXGetAtomName(display, targetList[i]); 
                  X11DRV_RegisterClipboardFormat(lpName);

                  lpFormat = X11DRV_CLIPBOARD_LookupProperty(targetList[i]);

		  if (!lpFormat)
                  {
                      ERR("Failed to cache %s property\n", lpName);
                      continue;
                  }

                  TSXFree(lpName);
              }

              TRACE("Atom#%d Property(%d): --> FormatID(%d) %s\n",
                  i, lpFormat->drvData, lpFormat->wFormatID, lpFormat->Name);

              X11DRV_CLIPBOARD_InsertClipboardData(lpFormat->wFormatID, 0, 0, 0);
          }
       }

       /* Free the list of targets */
       TSXFree(targetList);
    }

    return cSelectionTargets;
}


/**************************************************************************
 *	X11DRV_CLIPBOARD_ReadClipboardData
 *
 * This method is invoked only when we DO NOT own the X selection
 *
 * We always get the data from the selection client each time,
 * since we have no way of determining if the data in our cache is stale.
 */
static BOOL X11DRV_CLIPBOARD_ReadClipboardData(UINT wFormat)
{
    Display *display = thread_display();
    BOOL bRet = FALSE;
    Bool res;

    HWND hWndClipWindow = GetOpenClipboardWindow();
    HWND hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow();

    LPWINE_CLIPFORMAT lpFormat;

    TRACE("%d\n", wFormat);

    if (!selectionAcquired)
    {
        Window w = X11DRV_get_whole_window(GetAncestor(hWnd, GA_ROOT));
        if(!w)
        {
            FIXME("No parent win found %p %p\n", hWnd, hWndClipWindow);
            return FALSE;
        }

        lpFormat = X11DRV_CLIPBOARD_LookupFormat(wFormat);

        if (lpFormat->drvData)
        {
    	    DWORD i;
	    UINT alias;
            XEvent xe;

            TRACE("Requesting %s selection (%d) from win(%08x)\n", 
                lpFormat->Name, lpFormat->drvData, (UINT)selectionCacheSrc);

            wine_tsx11_lock();
            XConvertSelection(display, selectionCacheSrc, lpFormat->drvData,
                              xaSelectionData, w, CurrentTime);
            wine_tsx11_unlock();

            /* wait until SelectionNotify is received */
            for (i = 0; i < SELECTION_RETRIES; i++)
            {
                wine_tsx11_lock();
                res = XCheckTypedWindowEvent(display, w, SelectionNotify, &xe);
                wine_tsx11_unlock();
                if (res && xe.xselection.selection == selectionCacheSrc) break;

                usleep(SELECTION_WAIT);
            }

            /* If the property wasn't available check for aliases */
	    if (xe.xselection.property == None &&
                (alias = X11DRV_CLIPBOARD_LookupPropertyAlias(lpFormat->drvData)))
	    {
                wine_tsx11_lock();
                XConvertSelection(display, selectionCacheSrc, alias,
                                  xaSelectionData, w, CurrentTime);
                wine_tsx11_unlock();

                /* wait until SelectionNotify is received */
                for (i = 0; i < SELECTION_RETRIES; i++)
                {
                    wine_tsx11_lock();
                    res = XCheckTypedWindowEvent(display, w, SelectionNotify, &xe);
                    wine_tsx11_unlock();
                    if (res && xe.xselection.selection == selectionCacheSrc) break;

                    usleep(SELECTION_WAIT);
                }
	    }

            /* Verify that the selection returned a valid TARGETS property */
            if (xe.xselection.property != None)
            {
                /*
                 *  Read the contents of the X selection property 
                 *  into WINE's clipboard cache converting the 
                 *  selection to be compatible if possible.
                 */
                bRet = X11DRV_CLIPBOARD_ReadSelection(lpFormat, xe.xselection.requestor, 
                    xe.xselection.property);
            }
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
 *		X11DRV_CLIPBOARD_ReadSelection
 *  Reads the contents of the X selection property into the WINE clipboard cache
 *  converting the selection into a format compatible with the windows clipboard
 *  if possible.
 *  This method is invoked only to read the contents of a the selection owned
 *  by an external application. i.e. when we do not own the X selection.
 */
static BOOL X11DRV_CLIPBOARD_ReadSelection(LPWINE_CLIPFORMAT lpData, Window w, Atom prop)
{
    Display *display = thread_display();
    Atom	      atype=AnyPropertyType;
    int		      aformat;
    unsigned long     total,nitems,remain,itemSize,val_cnt;
    long              lRequestLength,bwc;
    unsigned char*    val;
    unsigned char*    buffer;
    BOOL              bRet = FALSE;

    if(prop == None)
        return bRet;

    TRACE("Reading X selection type %s\n", lpData->Name);

    /*
     * First request a zero length in order to figure out the request size.
     */
    if(TSXGetWindowProperty(display,w,prop,0,0,False, AnyPropertyType,
        &atype, &aformat, &nitems, &itemSize, &val) != Success)
    {
        WARN("Failed to get property size\n");
        return bRet;
    }

    /* Free zero length return data if any */
    if (val)
    {
       TSXFree(val);
       val = NULL;
    }

    TRACE("Retrieving %ld bytes\n", itemSize * aformat/8);

    lRequestLength = (itemSize * aformat/8)/4  + 1;
    bwc = aformat/8;

    /* Read property in 4K blocks */
    if (TSXGetWindowProperty(display,w,prop,0,4096,False, AnyPropertyType/*reqType*/, 
        &atype, &aformat, &nitems, &remain, &buffer) != Success)
    {
        WARN("Failed to read property\n");
        return bRet;
    }

    val = (char*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nitems*bwc);
    memcpy(val,buffer,nitems*bwc);

    TSXFree(buffer);

    for (total = nitems*bwc, val_cnt = 0; remain;)
    {
       val_cnt +=nitems*bwc;
       if (TSXGetWindowProperty(display, w, prop, (total / 4), 4096, False,
           AnyPropertyType, &atype, &aformat, &nitems, &remain, &buffer) != Success)
       {
           WARN("Failed to read property\n");
           HeapFree(GetProcessHeap(), 0, val);
           return bRet;
       }

       total += nitems*bwc;
       HeapReAlloc(GetProcessHeap(),0,val, total);
       memcpy(&val[val_cnt], buffer, nitems*(aformat/8));
       TSXFree(buffer);
    }

    bRet = X11DRV_CLIPBOARD_InsertClipboardData(lpData->wFormatID, 0, lpData->lpDrvImportFunc(val, total), 0);

    /* Delete the property on the window now that we are done
     * This will send a PropertyNotify event to the selection owner. */
    TSXDeleteProperty(display,w,prop);

    /* Free the retrieved property data */
    HeapFree(GetProcessHeap(),0,val);

    return bRet;
}


/**************************************************************************
 *		CLIPBOARD_SerializeMetafile
 */
static HANDLE X11DRV_CLIPBOARD_SerializeMetafile(INT wformat, HANDLE hdata, LPDWORD lpcbytes, BOOL out)
{
    HANDLE h = 0;

    TRACE(" wFormat=%d hdata=%08x out=%d\n", wformat, (unsigned int) hdata, out);

    if (out) /* Serialize out, caller should free memory */
    {
        *lpcbytes = 0; /* Assume failure */

        if (wformat == CF_METAFILEPICT)
        {
            LPMETAFILEPICT lpmfp = (LPMETAFILEPICT) GlobalLock(hdata);
            int size = GetMetaFileBitsEx(lpmfp->hMF, 0, NULL);

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
                LPMETAFILEPICT pmfp = (LPMETAFILEPICT) GlobalLock(h);

                memcpy(pmfp, (LPVOID)hdata, sizeof(METAFILEPICT));
                pmfp->hMF = SetMetaFileBitsEx(*lpcbytes - sizeof(METAFILEPICT),
                                              (char *)hdata + sizeof(METAFILEPICT));

                GlobalUnlock(h);
            }
        }
        else if (wformat == CF_ENHMETAFILE)
        {
            h = SetEnhMetaFileBits(*lpcbytes, (LPVOID)hdata);
        }
    }

    return h;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_ReleaseSelection
 *
 * Release an XA_PRIMARY or XA_CLIPBOARD selection that we own, in response
 * to a SelectionClear event.
 * This can occur in response to another client grabbing the X selection.
 * If the XA_CLIPBOARD selection is lost, we relinquish XA_PRIMARY as well.
 */
void X11DRV_CLIPBOARD_ReleaseSelection(Atom selType, Window w, HWND hwnd)
{
    Display *display = thread_display();

    /* w is the window that lost the selection
     */
    TRACE("event->window = %08x (selectionWindow = %08x) selectionAcquired=0x%08x\n",
	  (unsigned)w, (unsigned)selectionWindow, (unsigned)selectionAcquired);

    if (selectionAcquired)
    {
	if (w == selectionWindow)
        {
            /* If we're losing the CLIPBOARD selection, or if the preferences in .winerc
             * dictate that *all* selections should be cleared on loss of a selection,
             * we must give up all the selections we own.
             */
            if (clearAllSelections || (selType == xaClipboard))
            {
                CLIPBOARDINFO cbinfo;

                /* completely give up the selection */
                TRACE("Lost CLIPBOARD (+PRIMARY) selection\n");

              /* We are completely giving up the selection. There is a
               * potential race condition where the apps that now owns
	       * the selection has already grabbed both selections. In
	       * this case, if we clear any selection we may clear the
	       * new owners selection. To prevent this common case we
	       * try to open the clipboard. If we can't, we assume it
	       * was a wine apps that took it and has taken both selections.
	       * In this case, don't bother releasing the other selection.
	       * Otherwise only release the selection if we still own it.
               */
                X11DRV_CLIPBOARD_GetClipboardInfo(&cbinfo);

                if (cbinfo.flags & CB_OWNER)
                {
                    /* Since we're still the owner, this wasn't initiated by 
		       another Wine process */
                    if (OpenClipboard(hwnd))
                    {
                      /* We really lost CLIPBOARD but want to voluntarily lose PRIMARY */
                      if ((selType == xaClipboard) && (selectionAcquired & S_PRIMARY))
                      {
		          TRACE("Lost clipboard. Check if we need to release PRIMARY\n");
                          if (selectionWindow == TSXGetSelectionOwner(display,XA_PRIMARY))
                          {
		             TRACE("We still own PRIMARY. Releasing PRIMARY.\n");
                             XSetSelectionOwner(display, XA_PRIMARY, None, CurrentTime);
                          }
		          else
		             TRACE("We no longer own PRIMARY\n");
                      }

                      /* We really lost PRIMARY but want to voluntarily lose CLIPBOARD  */
                      if ((selType == XA_PRIMARY) && (selectionAcquired & S_CLIPBOARD))
                      {
		          TRACE("Lost PRIMARY. Check if we need to release CLIPBOARD\n");
                          if (selectionWindow == TSXGetSelectionOwner(display,xaClipboard))
                          {
		              TRACE("We still own CLIPBOARD. Releasing CLIPBOARD.\n");
                              XSetSelectionOwner(display, xaClipboard, None, CurrentTime);
                          }
		          else
		              TRACE("We no longer own CLIPBOARD\n");
                      }

                      /* Destroy private objects */
                      SendMessageW(cbinfo.hWndOwner, WM_DESTROYCLIPBOARD, 0, 0);
   
                      /* Give up ownership of the windows clipboard */
                      X11DRV_CLIPBOARD_ReleaseOwnership();

                      CloseClipboard();
                    }
                }
	        else
                {
                    TRACE("Lost selection to other Wine process.\n");
                }

                selectionWindow = None;
                PrimarySelectionOwner = ClipboardSelectionOwner = 0;

                X11DRV_EmptyClipboard();

                /* Reset the selection flags now that we are done */
                selectionAcquired = S_NOSELECTION;
            }
            else if ( selType == XA_PRIMARY ) /* Give up only PRIMARY selection */
            {
                TRACE("Lost PRIMARY selection\n");
                PrimarySelectionOwner = 0;
                selectionAcquired &= ~S_PRIMARY;  /* clear S_PRIMARY mask */
            }
	}
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


/**************************************************************************
 *		RegisterClipboardFormat (X11DRV.@)
 *
 * Registers a custom X clipboard format
 * Returns: Format id or 0 on failure
 */
INT X11DRV_RegisterClipboardFormat(LPCSTR FormatName)
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;

    if (FormatName == NULL) 
        return 0;

    TRACE("('%s') !\n", FormatName);

    /* walk format chain to see if it's already registered */
    while(TRUE)
    {
	if ( !strcasecmp(lpFormat->Name, FormatName) && 
	     (lpFormat->wFlags & CF_FLAG_BUILTINFMT) == 0)
	     return lpFormat->wFormatID;

        if (!lpFormat->NextFormat)
            break;

	lpFormat = lpFormat->NextFormat;
    }

    return X11DRV_CLIPBOARD_InsertClipboardFormat(FormatName, FormatName);
}


/**************************************************************************
 *		X11DRV_GetClipboardFormatName
 */
INT X11DRV_GetClipboardFormatName(UINT wFormat, LPSTR retStr, INT maxlen)
{
    INT len = 0;
    LPWINE_CLIPFORMAT lpFormat = X11DRV_CLIPBOARD_LookupFormat(wFormat);

    TRACE("(%04X, %p, %d) !\n", wFormat, retStr, maxlen);

    if (!lpFormat || (lpFormat->wFlags & CF_FLAG_BUILTINFMT))
    {
        TRACE("Unknown format 0x%08x!\n", wFormat);
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        strncpy(retStr, lpFormat->Name, maxlen - 1);
        retStr[maxlen - 1] = 0;

        len = strlen(retStr);
    }

    return len;
}


/**************************************************************************
 *		AcquireClipboard (X11DRV.@)
 */
void X11DRV_AcquireClipboard(HWND hWndClipWindow)
{
    Display *display = thread_display();

    /*
     * Acquire X selection if we don't already own it.
     * Note that we only acquire the selection if it hasn't been already
     * acquired by us, and ignore the fact that another X window may be
     * asserting ownership. The reason for this is we need *any* top level
     * X window to hold selection ownership. The actual clipboard data requests
     * are made via GetClipboardData from EVENT_SelectionRequest and this
     * ensures that the real HWND owner services the request.
     * If the owning X window gets destroyed the selection ownership is
     * re-cycled to another top level X window in X11DRV_CLIPBOARD_ResetOwner.
     *
     */
    if (!(selectionAcquired == (S_PRIMARY | S_CLIPBOARD)))
    {
        Window owner;

        if (!hWndClipWindow)
            hWndClipWindow = GetActiveWindow();

        owner = X11DRV_get_whole_window(GetAncestor(hWndClipWindow, GA_ROOT));

        /* Grab PRIMARY selection if not owned */
        if (!(selectionAcquired & S_PRIMARY))
            TSXSetSelectionOwner(display, XA_PRIMARY, owner, CurrentTime);

        /* Grab CLIPBOARD selection if not owned */
        if (!(selectionAcquired & S_CLIPBOARD))
            TSXSetSelectionOwner(display, xaClipboard, owner, CurrentTime);

        if (TSXGetSelectionOwner(display,XA_PRIMARY) == owner)
	    selectionAcquired |= S_PRIMARY;

        if (TSXGetSelectionOwner(display,xaClipboard) == owner)
	    selectionAcquired |= S_CLIPBOARD;

        if (selectionAcquired)
        {
	    selectionWindow = owner;
	    TRACE("Grabbed X selection, owner=(%08x)\n", (unsigned) owner);
        }
    }
    else
    {
        WARN("Received request to acquire selection but process is already owner=(%08x)\n", (unsigned) selectionWindow);

        selectionAcquired  = S_NOSELECTION;

        if (TSXGetSelectionOwner(display,XA_PRIMARY) == selectionWindow)
	    selectionAcquired |= S_PRIMARY;

        if (TSXGetSelectionOwner(display,xaClipboard) == selectionWindow)
	    selectionAcquired |= S_CLIPBOARD;

        if (!(selectionAcquired == (S_PRIMARY | S_CLIPBOARD)))
	{
            WARN("Lost selection but process didn't process SelectClear\n");
            selectionWindow = None;
	}
    }
}


/**************************************************************************
 *	X11DRV_EmptyClipboard
 */
void X11DRV_EmptyClipboard(void)
{
    if (ClipData)
    {
        LPWINE_CLIPDATA lpData;
        LPWINE_CLIPDATA lpNext = ClipData;

        do
        {
            lpData = lpNext;
            lpNext = lpData->NextData;
            lpData->PrevData->NextData = lpData->NextData;
            lpData->NextData->PrevData = lpData->PrevData;
            X11DRV_CLIPBOARD_FreeData(lpData);
            HeapFree(GetProcessHeap(), 0, lpData);
        } while (lpNext != lpData);
    }

    TRACE(" %d entries deleted from cache.\n", ClipDataCount);

    ClipData = NULL;
    ClipDataCount = 0;
}



/**************************************************************************
 *		X11DRV_SetClipboardData
 */
BOOL X11DRV_SetClipboardData(UINT wFormat, HANDLE16 hData16, HANDLE hData32)
{
    BOOL bResult = FALSE;

    if (X11DRV_CLIPBOARD_InsertClipboardData(wFormat, hData16, hData32, 0))
        bResult = TRUE;

    return bResult;
}


/**************************************************************************
 *		CountClipboardFormats
 */
INT X11DRV_CountClipboardFormats(void)
{
    CLIPBOARDINFO cbinfo;

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

    TRACE(" count=%d\n", ClipDataCount);

    return ClipDataCount;
}


/**************************************************************************
 *		X11DRV_EnumClipboardFormats
 */
UINT X11DRV_EnumClipboardFormats(UINT wFormat)
{
    CLIPBOARDINFO cbinfo;
    UINT wNextFormat = 0;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

    if (!wFormat)
    {
        if (ClipData)
            wNextFormat = ClipData->wFormatID;
    }
    else
    {
        LPWINE_CLIPDATA lpData = X11DRV_CLIPBOARD_LookupData(wFormat);

	if (lpData && lpData->NextData != ClipData)
            wNextFormat = lpData->NextData->wFormatID;
    }

    return wNextFormat;
}


/**************************************************************************
 *		X11DRV_IsClipboardFormatAvailable
 */
BOOL X11DRV_IsClipboardFormatAvailable(UINT wFormat)
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
BOOL X11DRV_GetClipboardData(UINT wFormat, HANDLE16* phData16, HANDLE* phData32)
{
    CLIPBOARDINFO cbinfo;
    LPWINE_CLIPDATA lpRender;

    TRACE("(%04X)\n", wFormat);

    X11DRV_CLIPBOARD_UpdateCache(&cbinfo);

    if ((lpRender = X11DRV_CLIPBOARD_LookupData(wFormat)))
    {
        if ( !lpRender->hData32 )
            X11DRV_CLIPBOARD_RenderFormat(lpRender);

        /* Convert between 32 -> 16 bit data, if necessary */
        if (lpRender->hData32 && !lpRender->hData16)
        {
            int size;

            if (lpRender->wFormatID == CF_METAFILEPICT)
                size = sizeof(METAFILEPICT16);
            else
                size = GlobalSize(lpRender->hData32);

            lpRender->hData16 = GlobalAlloc16(GMEM_ZEROINIT, size);

            if (!lpRender->hData16)
                ERR("(%04X) -- not enough memory in 16b heap\n", wFormat);
            else
            {
                if (lpRender->wFormatID == CF_METAFILEPICT)
                {
                    FIXME("\timplement function CopyMetaFilePict32to16\n");
                    FIXME("\tin the appropriate file.\n");
  #ifdef SOMEONE_IMPLEMENTED_ME
                    CopyMetaFilePict32to16(GlobalLock16(lpRender->hData16),
                        GlobalLock(lpRender->hData32));
  #endif
                }
                else
                {
                    memcpy(GlobalLock16(lpRender->hData16),
                        GlobalLock(lpRender->hData32), size);
                }

                GlobalUnlock16(lpRender->hData16);
                GlobalUnlock(lpRender->hData32);
            }
        }

        /* Convert between 32 -> 16 bit data, if necessary */
        if (lpRender->hData16 && !lpRender->hData32)
        {
            int size;

            if (lpRender->wFormatID == CF_METAFILEPICT)
                size = sizeof(METAFILEPICT16);
            else
                size = GlobalSize(lpRender->hData32);

            lpRender->hData32 = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | 
                GMEM_DDESHARE, size);

            if (lpRender->wFormatID == CF_METAFILEPICT)
            {
                FIXME("\timplement function CopyMetaFilePict16to32\n");
                FIXME("\tin the appropriate file.\n");
#ifdef SOMEONE_IMPLEMENTED_ME
                CopyMetaFilePict16to32(GlobalLock16(lpRender->hData32),
                    GlobalLock(lpRender->hData16));
#endif
            }
            else
            {
                memcpy(GlobalLock(lpRender->hData32),
                    GlobalLock16(lpRender->hData16), size);
            }

            GlobalUnlock(lpRender->hData32);
            GlobalUnlock16(lpRender->hData16);
        }

        if (phData16)
            *phData16 = lpRender->hData16;

        if (phData32)
            *phData32 = lpRender->hData32;

        TRACE(" returning hData16(%04x) hData32(%04x) (type %d)\n", 
            lpRender->hData16, (unsigned int) lpRender->hData32, lpRender->wFormatID);

        return lpRender->hData16 || lpRender->hData32;
    }

    return 0;
}


/**************************************************************************
 *		ResetSelectionOwner (X11DRV.@)
 *
 * Called from DestroyWindow() to prevent X selection from being lost when
 * a top level window is destroyed, by switching ownership to another top
 * level window.
 * Any top level window can own the selection. See X11DRV_CLIPBOARD_Acquire
 * for a more detailed description of this.
 */
void X11DRV_ResetSelectionOwner(HWND hwnd, BOOL bFooBar)
{
    Display *display = thread_display();
    HWND hWndClipOwner = 0;
    HWND tmp;
    Window XWnd = X11DRV_get_whole_window(hwnd);
    BOOL bLostSelection = FALSE;
    Window selectionPrevWindow;

    /* There is nothing to do if we don't own the selection,
     * or if the X window which currently owns the selection is different
     * from the one passed in.
     */
    if (!selectionAcquired || XWnd != selectionWindow
         || selectionWindow == None )
       return;

    if ((bFooBar && XWnd) || (!bFooBar && !XWnd))
       return;

    hWndClipOwner = GetClipboardOwner();

    TRACE("clipboard owner = %p, selection window = %08x\n",
          hWndClipOwner, (unsigned)selectionWindow);

    /* now try to salvage current selection from being destroyed by X */
    TRACE("checking %08x\n", (unsigned) XWnd);

    selectionPrevWindow = selectionWindow;
    selectionWindow = None;

    if (!(tmp = GetWindow(hwnd, GW_HWNDNEXT)))
        tmp = GetWindow(hwnd, GW_HWNDFIRST);

    if (tmp && tmp != hwnd) 
        selectionWindow = X11DRV_get_whole_window(tmp);

    if (selectionWindow != None)
    {
        /* We must pretend that we don't own the selection while making the switch
         * since a SelectionClear event will be sent to the last owner.
         * If there is no owner X11DRV_CLIPBOARD_ReleaseSelection will do nothing.
         */
        int saveSelectionState = selectionAcquired;
        selectionAcquired = S_NOSELECTION;

        TRACE("\tswitching selection from %08x to %08x\n",
                    (unsigned)selectionPrevWindow, (unsigned)selectionWindow);

        /* Assume ownership for the PRIMARY and CLIPBOARD selection */
        if (saveSelectionState & S_PRIMARY)
            TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);

        TSXSetSelectionOwner(display, xaClipboard, selectionWindow, CurrentTime);

        /* Restore the selection masks */
        selectionAcquired = saveSelectionState;

        /* Lose the selection if something went wrong */
        if (((saveSelectionState & S_PRIMARY) &&
           (TSXGetSelectionOwner(display, XA_PRIMARY) != selectionWindow)) || 
           (TSXGetSelectionOwner(display, xaClipboard) != selectionWindow))
        {
            bLostSelection = TRUE;
        }
        else
        {
            /* Update selection state */
            if (saveSelectionState & S_PRIMARY)
               PrimarySelectionOwner = selectionWindow;

            ClipboardSelectionOwner = selectionWindow;
        }
    }
    else
    {
        bLostSelection = TRUE;
    }

    if (bLostSelection)
    {
        TRACE("Lost the selection!\n");

        X11DRV_CLIPBOARD_ReleaseOwnership();
        selectionAcquired = S_NOSELECTION;
        ClipboardSelectionOwner = PrimarySelectionOwner = 0;
        selectionWindow = 0;
    }
}


/**************************************************************************
 *                      X11DRV_CLIPBOARD_SynthesizeData
 */
static BOOL X11DRV_CLIPBOARD_SynthesizeData(UINT wFormatID)
{
    BOOL bsyn = TRUE;
    LPWINE_CLIPDATA lpSource = NULL;

    TRACE(" %d\n", wFormatID);

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
        bsyn = (lpSource = X11DRV_CLIPBOARD_LookupData(CF_METAFILEPICT)) &&
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
        X11DRV_CLIPBOARD_InsertClipboardData(wFormatID, 0, 0, CF_FLAG_SYNTHESIZED);

    return bsyn;
}



/**************************************************************************
 *              X11DRV_EndClipboardUpdate
 * TODO:
 *  Add locale if it hasn't already been added
 */
void X11DRV_EndClipboardUpdate(void)
{
    INT count = ClipDataCount;

    /* Do Unicode <-> Text <-> OEM mapping */
    X11DRV_CLIPBOARD_SynthesizeData(CF_UNICODETEXT);
    X11DRV_CLIPBOARD_SynthesizeData(CF_TEXT);
    X11DRV_CLIPBOARD_SynthesizeData(CF_OEMTEXT);

    /* Enhmetafile <-> MetafilePict mapping */
    X11DRV_CLIPBOARD_SynthesizeData(CF_ENHMETAFILE);
    X11DRV_CLIPBOARD_SynthesizeData(CF_METAFILEPICT);

    /* DIB <-> Bitmap mapping */
    X11DRV_CLIPBOARD_SynthesizeData(CF_DIB);
    X11DRV_CLIPBOARD_SynthesizeData(CF_BITMAP);

    TRACE("%d formats added to cached data\n", ClipDataCount - count);
}
