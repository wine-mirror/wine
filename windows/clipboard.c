/*
 * WINE clipboard function handling
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "ts_xlib.h"
#include <X11/Xatom.h>
#include "windows.h"
#include "win.h"
#include "heap.h"
#include "message.h"
#include "clipboard.h"
#include "xmalloc.h"
#include "debug.h"

#define  CF_REGFORMATBASE 	0xC000

typedef struct tagCLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    WORD	wDataPresent;
    LPSTR	Name;
    HANDLE16	hData;
    DWORD	BufSize;
    struct tagCLIPFORMAT *PrevFormat;
    struct tagCLIPFORMAT *NextFormat;
} CLIPFORMAT, *LPCLIPFORMAT;

/* *************************************************************************
 *			internal variables
 */

static HQUEUE16 hqClipLock   = 0;
static BOOL32 bCBHasChanged  = FALSE;

static HWND32 hWndClipOwner  = 0;   /* current clipboard owner */
static HWND32 hWndClipWindow = 0;   /* window that opened clipboard */
static HWND32 hWndViewer     = 0;   /* start of viewers chain */

static WORD LastRegFormat = CF_REGFORMATBASE;

static Bool   selectionWait = False;
static Bool   selectionAcquired = False;
static Window selectionWindow = None;
static Window selectionPrevWindow = None;

static CLIPFORMAT ClipFormats[16]  = {
    { CF_TEXT, 1, 0, "Text", (HANDLE16)NULL, 0, NULL, &ClipFormats[1] },
    { CF_BITMAP, 1, 0, "Bitmap", (HANDLE16)NULL, 0, &ClipFormats[0], &ClipFormats[2] },
    { CF_METAFILEPICT, 1, 0, "MetaFile Picture", (HANDLE16)NULL, 0, &ClipFormats[1], &ClipFormats[3] },
    { CF_SYLK, 1, 0, "Sylk", (HANDLE16)NULL, 0, &ClipFormats[2], &ClipFormats[4] },
    { CF_DIF, 1, 0, "DIF", (HANDLE16)NULL, 0, &ClipFormats[3], &ClipFormats[5] },
    { CF_TIFF, 1, 0, "TIFF", (HANDLE16)NULL, 0, &ClipFormats[4], &ClipFormats[6] },
    { CF_OEMTEXT, 1, 0, "OEM Text", (HANDLE16)NULL, 0, &ClipFormats[5], &ClipFormats[7] },
    { CF_DIB, 1, 0, "DIB", (HANDLE16)NULL, 0, &ClipFormats[6], &ClipFormats[8] },
    { CF_PALETTE, 1, 0, "Palette", (HANDLE16)NULL, 0, &ClipFormats[7], &ClipFormats[9] },
    { CF_PENDATA, 1, 0, "PenData", (HANDLE16)NULL, 0, &ClipFormats[8], &ClipFormats[10] },
    { CF_RIFF, 1, 0, "RIFF", (HANDLE16)NULL, 0, &ClipFormats[9], &ClipFormats[11] },
    { CF_WAVE, 1, 0, "Wave", (HANDLE16)NULL, 0, &ClipFormats[10], &ClipFormats[12] },
    { CF_OWNERDISPLAY, 1, 0, "Owner Display", (HANDLE16)NULL, 0, &ClipFormats[11], &ClipFormats[13] },
    { CF_DSPTEXT, 1, 0, "DSPText", (HANDLE16)NULL, 0, &ClipFormats[12], &ClipFormats[14] },
    { CF_DSPMETAFILEPICT, 1, 0, "DSPMetaFile Picture", (HANDLE16)NULL, 0, &ClipFormats[13], &ClipFormats[15] },
    { CF_DSPBITMAP, 1, 0, "DSPBitmap", (HANDLE16)NULL, 0, &ClipFormats[14], NULL }
    };

static LPCLIPFORMAT __lookup_format( LPCLIPFORMAT lpFormat, WORD wID )
{
    while(TRUE)
    {
	if (lpFormat == NULL ||
	    lpFormat->wFormatID == wID) break;
	lpFormat = lpFormat->NextFormat;
    }
    return lpFormat;
}

/**************************************************************************
 *                      CLIPBOARD_CheckSelection
 *
 * Prevent X selection from being lost when a top level window is
 * destroyed.
 */
static void CLIPBOARD_CheckSelection(WND* pWnd)
{
    TRACE(clipboard,"\tchecking %08x\n", (unsigned)pWnd->window);

    if( selectionAcquired && selectionWindow != None &&
        pWnd->window == selectionWindow )
    {
	selectionPrevWindow = selectionWindow;
	selectionWindow = None;

	if( pWnd->next ) 
	    selectionWindow = pWnd->next->window;
	else if( pWnd->parent )
             if( pWnd->parent->child != pWnd ) 
                 selectionWindow = pWnd->parent->child->window;

	TRACE(clipboard,"\tswitching selection from %08x to %08x\n", 
                    (unsigned)selectionPrevWindow, (unsigned)selectionWindow);

	if( selectionWindow != None )
	{
	    TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);
	    if( TSXGetSelectionOwner(display, XA_PRIMARY) != selectionWindow )
		selectionWindow = None;
	}
    }
}

/**************************************************************************
 *                      CLIPBOARD_ResetLock
 */
void CLIPBOARD_ResetLock( HQUEUE16 hqCurrent, HQUEUE16 hqNew )
{
    if( hqClipLock == hqCurrent )
    {
	if( hqNew )
	    hqClipLock = hqNew;
	else
	{
 	    hWndClipOwner = 0;
	    hWndClipWindow = 0;
	    EmptyClipboard32();
	    hqClipLock = 0;
	}
    }
}

/**************************************************************************
 *			CLIPBOARD_ResetOwner
 *
 * Called from DestroyWindow().
 */
void CLIPBOARD_ResetOwner(WND* pWnd)
{
    LPCLIPFORMAT lpFormat = ClipFormats;

    TRACE(clipboard,"clipboard owner = %04x, selection = %08x\n", 
				hWndClipOwner, (unsigned)selectionWindow);

    if( pWnd->hwndSelf == hWndClipOwner)
    {
	SendMessage16(hWndClipOwner,WM_RENDERALLFORMATS,0,0L);

	/* check if all formats were rendered */

	while(lpFormat)
	{
	    if( lpFormat->wDataPresent && !lpFormat->hData )
	    {
		TRACE(clipboard,"\tdata missing for clipboard format %i\n", 
				   lpFormat->wFormatID); 
		lpFormat->wDataPresent = 0;
	    }
	    lpFormat = lpFormat->NextFormat;
	}
	hWndClipOwner = 0;
    }

    /* now try to salvage current selection from being destroyed by X */

    if( pWnd->window ) CLIPBOARD_CheckSelection(pWnd);
}

/**************************************************************************
 *			CLIPBOARD_DeleteRecord
 */
static void CLIPBOARD_DeleteRecord(LPCLIPFORMAT lpFormat, BOOL32 bChange)
{
    if( (lpFormat->wFormatID >= CF_GDIOBJFIRST &&
	 lpFormat->wFormatID <= CF_GDIOBJLAST) || lpFormat->wFormatID == CF_BITMAP )
	DeleteObject32(lpFormat->hData);
    else if( lpFormat->wFormatID == CF_METAFILEPICT && lpFormat->hData )
    {
        DeleteMetaFile16( ((METAFILEPICT16 *)GlobalLock16( lpFormat->hData ))->hMF );
	GlobalFree16(lpFormat->hData);
    }
    else if( lpFormat->hData )
	GlobalFree16(lpFormat->hData);

    lpFormat->wDataPresent = 0; 
    lpFormat->hData = 0;

    if( bChange ) bCBHasChanged = TRUE;
}

/**************************************************************************
 *			CLIPBOARD_RequestXSelection
 */
static BOOL32 CLIPBOARD_RequestXSelection()
{
    HWND32 hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow32();

    if( !hWnd ) return FALSE;

    TRACE(clipboard,"Requesting selection...\n");

  /* request data type XA_STRING, later
   * CLIPBOARD_ReadSelection() will be invoked 
   * from the SelectionNotify event handler */

    TSXConvertSelection(display,XA_PRIMARY,XA_STRING,
                      TSXInternAtom(display,"PRIMARY_TEXT",False),
                      WIN_GetXWindow(hWnd),CurrentTime);

  /* wait until SelectionNotify is processed 
   *
   * FIXME: Use TSXCheckTypedWindowEvent() instead ( same in the 
   *	    CLIPBOARD_CheckSelection() ). 
   */

    selectionWait=True;
    while(selectionWait) EVENT_WaitNetEvent( TRUE, FALSE );

  /* we treat Unix text as CF_OEMTEXT */
    TRACE(clipboard,"\tgot CF_OEMTEXT = %i\n", 
		      ClipFormats[CF_OEMTEXT-1].wDataPresent);

    return (BOOL32)ClipFormats[CF_OEMTEXT-1].wDataPresent;
}

/**************************************************************************
 *			CLIPBOARD_IsPresent
 */
BOOL32 CLIPBOARD_IsPresent(WORD wFormat)
{
    /* special case */

    if( wFormat == CF_TEXT || wFormat == CF_OEMTEXT )
        return ClipFormats[CF_TEXT-1].wDataPresent ||
               ClipFormats[CF_OEMTEXT-1].wDataPresent;
    else
    {
	LPCLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );
	if( lpFormat ) return (lpFormat->wDataPresent);
    }
    return FALSE;
}

/**************************************************************************
 *            OpenClipboard16   (USER.137)
 */
BOOL16 WINAPI OpenClipboard16( HWND16 hWnd )
{
    return OpenClipboard32( hWnd );
}


/**************************************************************************
 *            OpenClipboard32   (USER32.406)
 *
 * Note: Netscape uses NULL hWnd to open the clipboard.
 */
BOOL32 WINAPI OpenClipboard32( HWND32 hWnd )
{
    BOOL32 bRet;

    TRACE(clipboard,"(%04x)...\n", hWnd);

    if (!hqClipLock)
    {
	 hqClipLock = GetTaskQueue(0);
    	 hWndClipWindow = hWnd;
	 bCBHasChanged = FALSE;
	 bRet = TRUE;
    }
    else bRet = FALSE;

    TRACE(clipboard,"   returning %i\n", bRet);
    return bRet;
}


/**************************************************************************
 *            CloseClipboard16   (USER.138)
 */
BOOL16 WINAPI CloseClipboard16(void)
{
    return CloseClipboard32();
}


/**************************************************************************
 *            CloseClipboard32   (USER32.53)
 */
BOOL32 WINAPI CloseClipboard32(void)
{
    TRACE(clipboard,"!\n");

    if (hqClipLock == GetTaskQueue(0))
    {
	hWndClipWindow = 0;

        if (bCBHasChanged && hWndViewer) 
	    SendMessage16(hWndViewer, WM_DRAWCLIPBOARD, 0, 0L);
	hqClipLock = 0;
    }
    return TRUE;
}


/**************************************************************************
 *            EmptyClipboard16   (USER.139)
 */
BOOL16 WINAPI EmptyClipboard16(void)
{
    return EmptyClipboard32();
}


/**************************************************************************
 *            EmptyClipboard32   (USER32.168)
 */
BOOL32 WINAPI EmptyClipboard32(void)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"(void)\n");

    if (hqClipLock != GetTaskQueue(0)) return FALSE;

    /* destroy private objects */

    if (hWndClipOwner)
	SendMessage16(hWndClipOwner, WM_DESTROYCLIPBOARD, 0, 0L);
  
    while(lpFormat) 
    {
	if ( lpFormat->wDataPresent || lpFormat->hData )
	     CLIPBOARD_DeleteRecord( lpFormat, TRUE );

	lpFormat = lpFormat->NextFormat;
    }

    hWndClipOwner = hWndClipWindow;

    if(selectionAcquired)
    {
	selectionAcquired	= False;
	selectionPrevWindow 	= selectionWindow;
	selectionWindow 	= None;

	TRACE(clipboard, "\tgiving up selection (spw = %08x)\n", 
				 	(unsigned)selectionPrevWindow);

	TSXSetSelectionOwner(display, XA_PRIMARY, None, CurrentTime);
    }
    return TRUE;
}


/**************************************************************************
 *            GetClipboardOwner16   (USER.140)
 */
HWND16 WINAPI GetClipboardOwner16(void)
{
    return hWndClipOwner;
}


/**************************************************************************
 *            GetClipboardOwner32   (USER32.224)
 */
HWND32 WINAPI GetClipboardOwner32(void)
{
    return hWndClipOwner;
}


/**************************************************************************
 *            SetClipboardData16   (USER.141)
 */
HANDLE16 WINAPI SetClipboardData16( UINT16 wFormat, HANDLE16 hData )
{
    LPCLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );
    Window       owner;

    TRACE(clipboard, "(%04X, %04x) !\n", wFormat, hData);

    /* NOTE: If the hData is zero and current owner doesn't match
     * the window that opened the clipboard then this application
     * is screwed because WM_RENDERFORMAT will go to the owner
     * (to become the owner it must call EmptyClipboard() before
     *  adding new data).
     */

    if( (hqClipLock != GetTaskQueue(0)) || !lpFormat ||
	(!hData && (!hWndClipOwner || (hWndClipOwner != hWndClipWindow))) ) return 0; 

    /* Acquire X selection if text format */

    if( !selectionAcquired && 
	(wFormat == CF_TEXT || wFormat == CF_OEMTEXT) )
    {
	owner = WIN_GetXWindow( hWndClipWindow ? hWndClipWindow : AnyPopup32() );
	TSXSetSelectionOwner(display,XA_PRIMARY,owner,CurrentTime);
	if( TSXGetSelectionOwner(display,XA_PRIMARY) == owner )
	{
	    selectionAcquired = True;
	    selectionWindow = owner;

	    TRACE(clipboard,"Grabbed X selection, owner=(%08x)\n", 
						(unsigned) owner);
	}
    }

    if ( lpFormat->wDataPresent || lpFormat->hData ) 
    {
	CLIPBOARD_DeleteRecord(lpFormat, TRUE);

	/* delete existing CF_TEXT/CF_OEMTEXT aliases */

	if( wFormat == CF_TEXT && ClipFormats[CF_OEMTEXT-1].hData
	    && !ClipFormats[CF_OEMTEXT-1].wDataPresent )
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
        if( wFormat == CF_OEMTEXT && ClipFormats[CF_TEXT-1].hData
	    && !ClipFormats[CF_TEXT-1].wDataPresent )
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
    }

    bCBHasChanged = TRUE;
    lpFormat->wDataPresent = 1;
    lpFormat->hData = hData;          /* 0 is legal, see WM_RENDERFORMAT */

    return lpFormat->hData;
}


/**************************************************************************
 *            SetClipboardData32   (USER32.469)
 */
HANDLE32 WINAPI SetClipboardData32( UINT32 wFormat, HANDLE32 hData )
{
    fprintf( stderr, "SetClipboardData: empty stub\n" );
    return 0;
}


/**************************************************************************
 *                      CLIPBOARD_RenderFormat
 */
static BOOL32 CLIPBOARD_RenderFormat(LPCLIPFORMAT lpFormat)
{
   if( lpFormat->wDataPresent && !lpFormat->hData )
   if( IsWindow32(hWndClipOwner) )
       SendMessage16(hWndClipOwner,WM_RENDERFORMAT,
                     (WPARAM16)lpFormat->wFormatID,0L);
   else
   {
       WARN(clipboard, "\thWndClipOwner (%04x) is lost!\n", 
                                      hWndClipOwner);
       hWndClipOwner = 0; lpFormat->wDataPresent = 0;
       return FALSE;
   }
   return (lpFormat->hData) ? TRUE : FALSE;
}

/**************************************************************************
 *                      CLIPBOARD_RenderText
 *
 * Convert text between UNIX and DOS formats.
 */
static BOOL32 CLIPBOARD_RenderText(LPCLIPFORMAT lpTarget, LPCLIPFORMAT lpSource)
{
    UINT16 size = GlobalSize16( lpSource->hData );
    LPCSTR lpstrS = (LPSTR)GlobalLock16(lpSource->hData);
    LPSTR  lpstrT;

    if( !lpstrS ) return FALSE;
    TRACE(clipboard,"\tconverting from '%s' to '%s', %i chars\n",
			   	      lpSource->Name, lpTarget->Name, size);

    lpTarget->hData = GlobalAlloc16(GMEM_ZEROINIT, size); 
    lpstrT = (LPSTR)GlobalLock16(lpTarget->hData);

    if( lpstrT )
    {
	if( lpSource->wFormatID == CF_TEXT )
	    CharToOemBuff32A(lpstrS, lpstrT, size);
	else
	    OemToCharBuff32A(lpstrS, lpstrT, size);
	TRACE(clipboard,"\tgot %s\n", lpstrT);
	return TRUE;
    }

    lpTarget->hData = 0;
    return FALSE;
}

/**************************************************************************
 *             GetClipboardData16   (USER.142)
 */
HANDLE16 WINAPI GetClipboardData16( UINT16 wFormat )
{
    LPCLIPFORMAT lpRender = ClipFormats; 
    LPCLIPFORMAT lpUpdate = NULL;

    if (hqClipLock != GetTaskQueue(0)) return 0;

    TRACE(clipboard,"(%04X)\n", wFormat);

    if( wFormat == CF_TEXT && !lpRender[CF_TEXT-1].wDataPresent 
			   &&  lpRender[CF_OEMTEXT-1].wDataPresent )
    {
	lpRender = &ClipFormats[CF_OEMTEXT-1];
	lpUpdate = &ClipFormats[CF_TEXT-1];

	TRACE(clipboard,"\tOEMTEXT -> TEXT\n");
    }
    else if( wFormat == CF_OEMTEXT && !lpRender[CF_OEMTEXT-1].wDataPresent
				   &&  lpRender[CF_TEXT-1].wDataPresent )
    {
        lpRender = &ClipFormats[CF_TEXT-1];
	lpUpdate = &ClipFormats[CF_OEMTEXT-1];
	
	TRACE(clipboard,"\tTEXT -> OEMTEXT\n");
    }
    else
    {
	lpRender = __lookup_format( ClipFormats, wFormat );
	lpUpdate = lpRender;
    }
   
    if( !lpRender || !CLIPBOARD_RenderFormat(lpRender) ) return 0; 
    if( lpUpdate != lpRender && !lpUpdate->hData ) 
	CLIPBOARD_RenderText(lpUpdate, lpRender);

    TRACE(clipboard,"\treturning %04x (type %i)\n", 
			      lpUpdate->hData, lpUpdate->wFormatID);
    return lpUpdate->hData;
}


/**************************************************************************
 *             GetClipboardData32   (USER32.221)
 */
HANDLE32 WINAPI GetClipboardData32( UINT32 wFormat )
{
    fprintf( stderr, "GetClipboardData32: empty stub\n" );
    return 0;
}

/**************************************************************************
 *           CountClipboardFormats16   (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats32();
}


/**************************************************************************
 *           CountClipboardFormats32   (USER32.62)
 */
INT32 WINAPI CountClipboardFormats32(void)
{
    INT32 FormatCount = 0;
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"(void)\n");

    if( !selectionAcquired ) CLIPBOARD_RequestXSelection();

    FormatCount += abs(lpFormat[CF_TEXT-1].wDataPresent -
		       lpFormat[CF_OEMTEXT-1].wDataPresent); 

    while(TRUE) 
    {
	if (lpFormat == NULL) break;
	if (lpFormat->wDataPresent) 
	{
            TRACE(clipboard, "\tdata found for format %i\n", lpFormat->wFormatID);
	    FormatCount++;
	}
	lpFormat = lpFormat->NextFormat;
    }

    TRACE(clipboard,"\ttotal %d\n", FormatCount);
    return FormatCount;
}


/**************************************************************************
 *            EnumClipboardFormats16   (USER.144)
 */
UINT16 WINAPI EnumClipboardFormats16( UINT16 wFormat )
{
    return EnumClipboardFormats32( wFormat );
}


/**************************************************************************
 *            EnumClipboardFormats32   (USER32.178)
 */
UINT32 WINAPI EnumClipboardFormats32( UINT32 wFormat )
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"(%04X)\n", wFormat);

    if( hqClipLock != GetTaskQueue(0) ) return 0;

    if( (!wFormat || wFormat == CF_TEXT || wFormat == CF_OEMTEXT) 
	 && !selectionAcquired) CLIPBOARD_RequestXSelection();

    if (wFormat == 0)
	if (lpFormat->wDataPresent || ClipFormats[CF_OEMTEXT-1].wDataPresent) 
	    return lpFormat->wFormatID;
	else 
	    wFormat = lpFormat->wFormatID; /* and CF_TEXT is not available */

    /* walk up to the specified format record */

    if( !(lpFormat = __lookup_format( lpFormat, wFormat )) ) return 0;

    /* find next format with available data */

    lpFormat = lpFormat->NextFormat;
    while(TRUE) 
    {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wDataPresent || (lpFormat->wFormatID == CF_OEMTEXT && 
				       ClipFormats[CF_TEXT-1].wDataPresent)) 
	    break;
	lpFormat = lpFormat->NextFormat;
    }

    return lpFormat->wFormatID;
}


/**************************************************************************
 *            RegisterClipboardFormat16  (USER.145)
 */
UINT16 WINAPI RegisterClipboardFormat16( LPCSTR FormatName )
{
    LPCLIPFORMAT lpNewFormat; 
    LPCLIPFORMAT lpFormat = ClipFormats; 

    if (FormatName == NULL) return 0;

    TRACE(clipboard,"('%s') !\n", FormatName);

    /* walk format chain to see if it's already registered */

    while(TRUE) 
    {
	if ( !strcmp(lpFormat->Name,FormatName) )
	{
	     lpFormat->wRefCount++;
	     return lpFormat->wFormatID;
	}
 
	if ( lpFormat->NextFormat == NULL ) break;

	lpFormat = lpFormat->NextFormat;
    }

    /* allocate storage for new format entry */

    lpNewFormat = (LPCLIPFORMAT)xmalloc(sizeof(CLIPFORMAT));
    lpFormat->NextFormat = lpNewFormat;
    lpNewFormat->wFormatID = LastRegFormat;
    lpNewFormat->wRefCount = 1;

    lpNewFormat->Name = (LPSTR)xmalloc(strlen(FormatName) + 1);
    strcpy(lpNewFormat->Name, FormatName);

    lpNewFormat->wDataPresent = 0;
    lpNewFormat->hData = 0;
    lpNewFormat->BufSize = 0;
    lpNewFormat->PrevFormat = lpFormat;
    lpNewFormat->NextFormat = NULL;

    return LastRegFormat++;
}


/**************************************************************************
 *            RegisterClipboardFormat32A   (USER32.430)
 */
UINT32 WINAPI RegisterClipboardFormat32A( LPCSTR formatName )
{
    return RegisterClipboardFormat16( formatName );
}


/**************************************************************************
 *            RegisterClipboardFormat32W   (USER32.431)
 */
UINT32 WINAPI RegisterClipboardFormat32W( LPCWSTR formatName )
{
    LPSTR aFormat = HEAP_strdupWtoA( GetProcessHeap(), 0, formatName );
    UINT32 ret = RegisterClipboardFormat32A( aFormat );
    HeapFree( GetProcessHeap(), 0, aFormat );
    return ret;
}

/**************************************************************************
 *            GetClipboardFormatName16   (USER.146)
 */
INT16 WINAPI GetClipboardFormatName16( UINT16 wFormat, LPSTR retStr, INT16 maxlen )
{
    return GetClipboardFormatName32A( wFormat, retStr, maxlen );
}


/**************************************************************************
 *            GetClipboardFormatName32A   (USER32.222)
 */
INT32 WINAPI GetClipboardFormatName32A( UINT32 wFormat, LPSTR retStr, INT32 maxlen )
{
    LPCLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    TRACE(clipboard, "(%04X, %p, %d) !\n", wFormat, retStr, maxlen);

    if (lpFormat == NULL || lpFormat->Name == NULL || 
	lpFormat->wFormatID < CF_REGFORMATBASE) return 0;

    TRACE(clipboard, "Name='%s' !\n", lpFormat->Name);

    lstrcpyn32A( retStr, lpFormat->Name, maxlen );
    return strlen(retStr);
}


/**************************************************************************
 *            GetClipboardFormatName32W   (USER32.223)
 */
INT32 WINAPI GetClipboardFormatName32W( UINT32 wFormat, LPWSTR retStr, INT32 maxlen )
{
    LPSTR p = HEAP_xalloc( GetProcessHeap(), 0, maxlen );
    INT32 ret = GetClipboardFormatName32A( wFormat, p, maxlen );
    lstrcpynAtoW( retStr, p, maxlen );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/**************************************************************************
 *            SetClipboardViewer16   (USER.147)
 */
HWND16 WINAPI SetClipboardViewer16( HWND16 hWnd )
{
    return SetClipboardViewer32( hWnd );
}


/**************************************************************************
 *            SetClipboardViewer32   (USER32.470)
 */
HWND32 WINAPI SetClipboardViewer32( HWND32 hWnd )
{
    HWND32 hwndPrev = hWndViewer;

    TRACE(clipboard,"(%04x): returning %04x\n", hWnd, hwndPrev);

    hWndViewer = hWnd;
    return hwndPrev;
}


/**************************************************************************
 *           GetClipboardViewer16   (USER.148)
 */
HWND16 WINAPI GetClipboardViewer16(void)
{
    return hWndViewer;
}


/**************************************************************************
 *           GetClipboardViewer32   (USER32.225)
 */
HWND32 WINAPI GetClipboardViewer32(void)
{
    return hWndViewer;
}


/**************************************************************************
 *           ChangeClipboardChain16   (USER.149)
 */
BOOL16 WINAPI ChangeClipboardChain16(HWND16 hWnd, HWND16 hWndNext)
{
    return ChangeClipboardChain32(hWnd, hWndNext);
}

/**************************************************************************
 *           ChangeClipboardChain32   (USER32.21)
 */
BOOL32 WINAPI ChangeClipboardChain32(HWND32 hWnd, HWND32 hWndNext)
{
    BOOL32 bRet = 0;

    FIXME(clipboard, "(%04x, %04x) - stub?\n", 
		      hWnd, hWndNext);

    if( hWndViewer )
	bRet = !SendMessage16( hWndViewer, WM_CHANGECBCHAIN,
                             (WPARAM16)hWnd, (LPARAM)hWndNext);   
    else
	WARN(clipboard, "hWndViewer is lost\n");

    if( hWnd == hWndViewer ) hWndViewer = hWndNext;

    return bRet;
}



/**************************************************************************
 *           IsClipboardFormatAvailable16   (USER.193)
 */
BOOL16 WINAPI IsClipboardFormatAvailable16( UINT16 wFormat )
{
    return IsClipboardFormatAvailable32( wFormat );
}


/**************************************************************************
 *           IsClipboardFormatAvailable32   (USER32.339)
 */
BOOL32 WINAPI IsClipboardFormatAvailable32( UINT32 wFormat )
{
    TRACE(clipboard,"(%04X) !\n", wFormat);

    if( (wFormat == CF_TEXT || wFormat == CF_OEMTEXT) &&
        !selectionAcquired ) CLIPBOARD_RequestXSelection();

    return CLIPBOARD_IsPresent(wFormat);
}


/**************************************************************************
 *             GetOpenClipboardWindow16   (USER.248)
 */
HWND16 WINAPI GetOpenClipboardWindow16(void)
{
    return hWndClipWindow;
}


/**************************************************************************
 *             GetOpenClipboardWindow32   (USER32.276)
 */
HWND32 WINAPI GetOpenClipboardWindow32(void)
{
    return hWndClipWindow;
}


/**************************************************************************
 *             GetPriorityClipboardFormat16   (USER.402)
 */
INT16 WINAPI GetPriorityClipboardFormat16( UINT16 *lpPriorityList, INT16 nCount)
{
    fprintf( stderr, "GetPriorityClipboardFormat16(%p, %d): stub\n",
             lpPriorityList, nCount );
    return 0;
}


/**************************************************************************
 *             GetPriorityClipboardFormat32   (USER32.279)
 */
INT32 WINAPI GetPriorityClipboardFormat32( UINT32 *lpPriorityList, INT32 nCount )
{
    int Counter;

    if(CountClipboardFormats32() == 0) 
    { 
        return 0;
    }

    for(Counter = 0; Counter <= nCount; Counter++)
    {
        if(IsClipboardFormatAvailable32(*(lpPriorityList+sizeof(INT32)*Counter)))
            return *(lpPriorityList+sizeof(INT32)*Counter);
    }

    return -1;
}


/**************************************************************************
 *			CLIPBOARD_ReadSelection
 *
 * Called from the SelectionNotify event handler. 
 */
void CLIPBOARD_ReadSelection(Window w,Atom prop)
{
    HANDLE16 	 hText = 0;
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"ReadSelection callback\n");

    if(prop != None)
    {
	Atom		atype=AnyPropertyType;
	int		aformat;
	unsigned long 	nitems,remain;
	unsigned char*	val=NULL;

        TRACE(clipboard,"\tgot property %s\n",TSXGetAtomName(display,prop));

        /* TODO: Properties longer than 64K */

	if(TSXGetWindowProperty(display,w,prop,0,0x3FFF,True,XA_STRING,
	    &atype, &aformat, &nitems, &remain, &val) != Success)
	    WARN(clipboard, "\tcouldn't read property\n");
	else
	{
           TRACE(clipboard,"\tType %s,Format %d,nitems %ld,value %s\n",
		             TSXGetAtomName(display,atype),aformat,nitems,val);

	   if(atype == XA_STRING && aformat == 8)
	   {
	      int 	i,inlcount = 0;
	      char*	lpstr;

	      TRACE(clipboard,"\tselection is '%s'\n",val);

	      for(i=0; i <= nitems; i++)
		  if( val[i] == '\n' ) inlcount++;

	      if( nitems )
	      {
	        hText=GlobalAlloc16(GMEM_MOVEABLE, nitems + inlcount + 1);
	        if( (lpstr = (char*)GlobalLock16(hText)) )
	          for(i=0,inlcount=0; i <= nitems; i++)
	          {
	  	     if( val[i] == '\n' ) lpstr[inlcount++]='\r';
		     lpstr[inlcount++]=val[i];
		  }
	        else hText = 0;
	      }
	   }
	   TSXFree(val);
	}
   }

   /* delete previous CF_TEXT and CF_OEMTEXT data */

   if( hText )
   {
     lpFormat = &ClipFormats[CF_TEXT-1];
     if (lpFormat->wDataPresent || lpFormat->hData) 
         CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
     lpFormat = &ClipFormats[CF_OEMTEXT-1];
     if (lpFormat->wDataPresent || lpFormat->hData) 
         CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));

     lpFormat->wDataPresent = 1;
     lpFormat->hData = hText;
   }

   selectionWait=False;
}

/**************************************************************************
 *			CLIPBOARD_ReleaseSelection
 *
 * Wine might have lost XA_PRIMARY selection because of
 * EmptyClipboard() or other client. 
 */
void CLIPBOARD_ReleaseSelection(Window w, HWND32 hwnd)
{
    /* w is the window that lost selection,
     * 
     * selectionPrevWindow is nonzero if CheckSelection() was called. 
     */

    TRACE(clipboard,"\tevent->window = %08x (sw = %08x, spw=%08x)\n", 
	  (unsigned)w, (unsigned)selectionWindow, (unsigned)selectionPrevWindow );

    if( selectionAcquired )
	if( w == selectionWindow || selectionPrevWindow == None)
	{
	    /* alright, we really lost it */

	    selectionAcquired = False;
	    selectionWindow = None; 

	    /* but we'll keep existing data for internal use */
	}
	else if( w == selectionPrevWindow )
	{
	    w = TSXGetSelectionOwner(display, XA_PRIMARY);
	    if( w == None )
		TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);
	}

    selectionPrevWindow = None;
}

