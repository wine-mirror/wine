/*
 * WIN32 clipboard implementation
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
 *
 * NOTES:
 *    This file contains the implementation for the WIN32 Clipboard API
 * and Wine's internal clipboard cache.
 * The actual contents of the clipboard are held in the clipboard cache.
 * The internal implementation talks to a "clipboard driver" to fill or
 * expose the cache to the native device. (Currently only the X11 and
 * TTY clipboard  driver are available)
 *
 * TODO: 
 *
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "winuser.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "message.h"
#include "task.h"
#include "queue.h"
#include "clipboard.h"
#include "xmalloc.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(clipboard)

#define  CF_REGFORMATBASE 	0xC000

/**************************************************************************
 *	  Clipboard context global variables
 */

CLIPBOARD_DRIVER *CLIPBOARD_Driver = NULL;

static HANDLE hClipLock   = 0;
static BOOL bCBHasChanged  = FALSE;

HWND hWndClipWindow = 0;          /* window that last opened clipboard */
HWND hWndClipOwner  = 0;          /* current clipboard owner */
HANDLE16 hTaskClipOwner = 0;      /* clipboard owner's task  */
static HWND hWndViewer     = 0;   /* start of viewers chain */

static WORD LastRegFormat = CF_REGFORMATBASE;

/* Clipboard cache initial data.
 * WARNING: This data ordering is dependendent on the WINE_CLIPFORMAT structure
 * declared in clipboard.h
 */
WINE_CLIPFORMAT ClipFormats[17]  = {
    { CF_TEXT, 1, 0, "Text",  (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, NULL, &ClipFormats[1]},
    { CF_BITMAP, 1, 0, "Bitmap", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[0], &ClipFormats[2]},
    { CF_METAFILEPICT, 1, 0, "MetaFile Picture", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[1], &ClipFormats[3]},
    { CF_SYLK, 1, 0, "Sylk", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[2], &ClipFormats[4]},
    { CF_DIF, 1, 0, "DIF", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[3], &ClipFormats[5]},
    { CF_TIFF, 1, 0, "TIFF", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[4], &ClipFormats[6]},
    { CF_OEMTEXT, 1, 0, "OEM Text", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[5], &ClipFormats[7]},
    { CF_DIB, 1, 0, "DIB", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[6], &ClipFormats[8]},
    { CF_PALETTE, 1, 0, "Palette", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[7], &ClipFormats[9]},
    { CF_PENDATA, 1, 0, "PenData", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[8], &ClipFormats[10]},
    { CF_RIFF, 1, 0, "RIFF", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[9], &ClipFormats[11]},
    { CF_WAVE, 1, 0, "Wave", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[10], &ClipFormats[12]},
    { CF_OWNERDISPLAY, 1, 0, "Owner Display", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[11], &ClipFormats[13]},
    { CF_DSPTEXT, 1, 0, "DSPText", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[12], &ClipFormats[14]},
    { CF_DSPMETAFILEPICT, 1, 0, "DSPMetaFile Picture", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[13], &ClipFormats[15]},
    { CF_DSPBITMAP, 1, 0, "DSPBitmap", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[14], &ClipFormats[16]},
    { CF_HDROP, 1, 0, "HDROP", (HANDLE16)NULL, (HANDLE)NULL, (HANDLE)NULL, 0, &ClipFormats[15], NULL}
    };


/**************************************************************************
 *                Internal Clipboard implementation methods
 **************************************************************************/


/**************************************************************************
 *                      CLIPBOARD_LookupFormat
 */
static LPWINE_CLIPFORMAT __lookup_format( LPWINE_CLIPFORMAT lpFormat, WORD wID )
{
    while(TRUE)
    {
	if (lpFormat == NULL ||
	    lpFormat->wFormatID == wID) break;
	lpFormat = lpFormat->NextFormat;
    }
    return lpFormat;
}

LPWINE_CLIPFORMAT CLIPBOARD_LookupFormat( WORD wID )
{
  return __lookup_format( ClipFormats, wID );
}

/**************************************************************************
 *                      CLIPBOARD_IsLocked
 *  Check if the clipboard cache is available to the caller
 */
BOOL CLIPBOARD_IsLocked()
{
  BOOL bIsLocked = TRUE;
  HANDLE16 hTaskCur = GetCurrentTask();

  /*
   * The clipboard is available:
   * 1. if the caller's task has opened the clipboard,
   * or
   * 2. if the caller is the clipboard owners task, AND is responding to a
   *    WM_RENDERFORMAT message.
   */
  if ( hClipLock == hTaskCur )
      bIsLocked = FALSE;
      
  else if ( hTaskCur == hTaskClipOwner )
  {
      /* Check if we're currently executing inside a window procedure
       * called in response to a WM_RENDERFORMAT message. A WM_RENDERFORMAT
       * handler is not permitted to open the clipboard since it has been opened
       * by another client. However the handler must have access to the
       * clipboard in order to update data in response to this message.
       */
      MESSAGEQUEUE *queue = QUEUE_Lock( GetFastQueue16() );
      
      if ( queue
           && queue->smWaiting
           && queue->smWaiting->msg == WM_RENDERFORMAT
           && queue->smWaiting->hSrcQueue
         )
  	bIsLocked = FALSE;
        
      QUEUE_Unlock( queue );
  }

  return bIsLocked;
}

/**************************************************************************
 *                      CLIPBOARD_ReleaseOwner
 *   Gives up ownership of the clipboard
 */
void CLIPBOARD_ReleaseOwner()
{
   hWndClipOwner = 0;
   hTaskClipOwner = 0;
}

/**************************************************************************
 *                      CLIPBOARD_GlobalFreeProc
 *
 * This is a callback mechanism to allow HGLOBAL data to be released in
 * the context of the process which allocated it. We post a WM_TIMER message
 * to the owner window(in CLIPBOARD_DeleteRecord) and destroy the data(in idEvent)
 * in this WndProc, which is invoked when the apps message loop calls DispatchMessage.
 * This technique is discussed in Matt Pietrek's "Under the Hood".
 * An article describing the same may be found in MSDN by searching for WM_TIMER.
 * Note that this mechanism will probably stop working when WINE supports
 * address space separation. When "queue events" are implemented in Wine we
 * should switch to using that mechanism, since it is more robust and does not
 * require a procedure address to be passed. See the SetWinEventHook API for
 * more info on this.
 */
VOID CALLBACK CLIPBOARD_GlobalFreeProc( HWND hwnd, UINT uMsg, UINT idEvent, DWORD dwTime )
{
    /* idEvent is the HGLOBAL to be deleted */
    GlobalFree( (HGLOBAL)idEvent );
}

/**************************************************************************
 *			CLIPBOARD_DeleteRecord
 */
void CLIPBOARD_DeleteRecord(LPWINE_CLIPFORMAT lpFormat, BOOL bChange)
{
    if( (lpFormat->wFormatID >= CF_GDIOBJFIRST &&
	 lpFormat->wFormatID <= CF_GDIOBJLAST) || lpFormat->wFormatID == CF_BITMAP 
	    || lpFormat->wFormatID == CF_PALETTE)
    {
      if (lpFormat->hData32)
	DeleteObject(lpFormat->hData32);
      if (lpFormat->hData16)
	DeleteObject16(lpFormat->hData16);
    }
    else if( lpFormat->wFormatID == CF_METAFILEPICT )
    {
      if (lpFormat->hData32)
      {
        DeleteMetaFile( ((METAFILEPICT *)GlobalLock( lpFormat->hData32 ))->hMF );
        PostMessageA(hWndClipOwner, WM_TIMER,
                     (WPARAM)lpFormat->hData32, (LPARAM)CLIPBOARD_GlobalFreeProc);
        if (lpFormat->hDataSrc32)
        {
          /* Release lpFormat->hData32 in the context of the process which created it.
           * See CLIPBOARD_GlobalFreeProc for more details about this technique.
           * GlobalFree(lpFormat->hDataSrc32);
           */
          PostMessageA(hWndClipOwner, WM_TIMER,
                       (WPARAM)lpFormat->hDataSrc32, (LPARAM)CLIPBOARD_GlobalFreeProc);
        }
          
	if (lpFormat->hData16)
	  /* HMETAFILE16 and HMETAFILE32 are apparently the same thing, 
	     and a shallow copy is enough to share a METAFILEPICT
	     structure between 16bit and 32bit clipboards.  The MetaFile
	     should of course only be deleted once. */
	  GlobalFree16(lpFormat->hData16);
      }
      if (lpFormat->hData16)
      {
	DeleteMetaFile16( ((METAFILEPICT16 *)GlobalLock16( lpFormat->hData16 ))->hMF );
	GlobalFree16(lpFormat->hData16);
      }
    }
    else 
    {
      if (lpFormat->hData32)
      {
        /* Release lpFormat->hData32 in the context of the process which created it.
         * See CLIPBOARD_GlobalFreeProc for more details about this technique.
         * GlobalFree( lpFormat->hData32 );
         */
        PostMessageA(hWndClipOwner, WM_TIMER,
                     (WPARAM)lpFormat->hData32, (LPARAM)CLIPBOARD_GlobalFreeProc);
      }
      if (lpFormat->hDataSrc32)
      {
        /* Release lpFormat->hData32 in the context of the process which created it.
         * See CLIPBOARD_GlobalFreeProc for more details about this technique.
         * GlobalFree(lpFormat->hDataSrc32);
         */
        PostMessageA(hWndClipOwner, WM_TIMER,
                     (WPARAM)lpFormat->hDataSrc32, (LPARAM)CLIPBOARD_GlobalFreeProc);
      }
      if (lpFormat->hData16)
	GlobalFree16(lpFormat->hData16);
    }

    lpFormat->wDataPresent = 0; 
    lpFormat->hData16 = 0;
    lpFormat->hData32 = 0;
    lpFormat->hDataSrc32 = 0;
    lpFormat->drvData = 0;

    if( bChange ) bCBHasChanged = TRUE;
}

/**************************************************************************
 *			CLIPBOARD_EmptyCache
 */
void CLIPBOARD_EmptyCache( BOOL bChange )
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats; 

    while(lpFormat)
    {
	if ( lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32 )
	     CLIPBOARD_DeleteRecord( lpFormat, bChange );

	lpFormat = lpFormat->NextFormat;
    }
}

/**************************************************************************
 *			CLIPBOARD_IsPresent
 */
BOOL CLIPBOARD_IsPresent(WORD wFormat)
{
    /* special case */

    if( wFormat == CF_TEXT || wFormat == CF_OEMTEXT )
        return ClipFormats[CF_TEXT-1].wDataPresent ||
               ClipFormats[CF_OEMTEXT-1].wDataPresent;
    else
    {
	LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );
	if( lpFormat ) return (lpFormat->wDataPresent);
    }
    return FALSE;
}

/**************************************************************************
 *			CLIPBOARD_IsCacheRendered
 *  Checks if any data needs to be rendered to the clipboard cache
 *  RETURNS:
 *    TRUE  - All clipboard data is available in the cache
 *    FALSE - Some data is marked for delayed render and needs rendering
 */
BOOL CLIPBOARD_IsCacheRendered()
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;
    
    /* check if all formats were rendered */
    while(lpFormat)
    {
        if( lpFormat->wDataPresent && !lpFormat->hData16 && !lpFormat->hData32 )
            return FALSE;
        
        lpFormat = lpFormat->NextFormat;
    }
    
    return TRUE;
}


/**************************************************************************
 *			CLIPBOARD_IsMemoryObject
 *  Tests if the clipboard format specifies a memory object
 */
BOOL CLIPBOARD_IsMemoryObject( WORD wFormat )
{
    switch(wFormat)
    {
        case CF_BITMAP:
        case CF_METAFILEPICT:
        case CF_DSPTEXT:
        case CF_ENHMETAFILE:
        case CF_HDROP:
        case CF_PALETTE:
        case CF_PENDATA:
            return FALSE;
        default:
            return TRUE;
     }
}

/***********************************************************************
 * CLIPBOARD_GlobalDupMem( HGLOBAL )
 * Helper method to duplicate an HGLOBAL chunk of memory into shared memory
 */
HGLOBAL CLIPBOARD_GlobalDupMem( HGLOBAL hGlobalSrc )
{
    HGLOBAL hGlobalDest;
    PVOID pGlobalSrc, pGlobalDest;
    DWORD cBytes;
    
    if ( !hGlobalSrc )
      return 0;

    cBytes = GlobalSize(hGlobalSrc);
    if ( 0 == cBytes )
      return 0;

    /* Turn on the DDESHARE and _MOVEABLE flags explicitly */
    hGlobalDest = GlobalAlloc( GlobalFlags(hGlobalSrc) | GMEM_DDESHARE | GMEM_MOVEABLE,
                               cBytes );
    if ( !hGlobalDest )
      return 0;
    
    pGlobalSrc = GlobalLock(hGlobalSrc);
    pGlobalDest = GlobalLock(hGlobalDest);
    if ( !pGlobalSrc || !pGlobalDest )
      return 0;

    memcpy(pGlobalDest, pGlobalSrc, cBytes);
        
    GlobalUnlock(hGlobalSrc);
    GlobalUnlock(hGlobalDest);

    return hGlobalDest;
}

/**************************************************************************
 *			CLIPBOARD_GetFormatName
 *  Gets the format name associated with an ID
 */
char * CLIPBOARD_GetFormatName(UINT wFormat)
{
    LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );
    return (lpFormat) ? lpFormat->Name : NULL;
}


/**************************************************************************
 *                      CLIPBOARD_RenderFormat
 */
static BOOL CLIPBOARD_RenderFormat(LPWINE_CLIPFORMAT lpFormat)
{
  /*
   * If WINE is not the selection owner, and the format is available
   * we must ask the the driver to render the data to the clipboard cache.
   */
  if ( !CLIPBOARD_Driver->pIsSelectionOwner() 
       && CLIPBOARD_Driver->pIsFormatAvailable( lpFormat->wFormatID ) )
  {
    if ( !CLIPBOARD_Driver->pGetData( lpFormat->wFormatID ) )
      return FALSE;
  }
  /*
   * If Wine owns the clipboard, and the data is marked for delayed render,
   * render it now.
   */
  else if( lpFormat->wDataPresent && !lpFormat->hData16 && !lpFormat->hData32 )
  {
    if( IsWindow(hWndClipOwner) )
    {
      /* Send a WM_RENDERFORMAT message to notify the owner to render the
       * data requested into the clipboard.
       */
      TRACE("Sending WM_RENDERFORMAT message\n");
      SendMessage16(hWndClipOwner,WM_RENDERFORMAT,
                    (WPARAM16)lpFormat->wFormatID,0L);
    }
    else
    {
      WARN("\thWndClipOwner (%04x) is lost!\n", 
	   hWndClipOwner);
      CLIPBOARD_ReleaseOwner();
      lpFormat->wDataPresent = 0;
      return FALSE;
    }
  }

  return (lpFormat->hData16 || lpFormat->hData32) ? TRUE : FALSE;
}


/**************************************************************************
 *                      CLIPBOARD_RenderText
 *
 * Renders text to the clipboard buffer converting between UNIX and DOS formats.
 *
 * RETURNS: pointer to the WINE_CLIPFORMAT if successful, NULL otherwise
 *
 * FIXME: Should be a pair of driver functions that convert between OEM text and Windows.
 *
 */
static LPWINE_CLIPFORMAT CLIPBOARD_RenderText( UINT wFormat )
{
    LPWINE_CLIPFORMAT lpSource = ClipFormats; 
    LPWINE_CLIPFORMAT lpTarget;
    
    /* Asked for CF_TEXT and not available - always attempt to convert from CF_OEM_TEXT */
    if( wFormat == CF_TEXT && !ClipFormats[CF_TEXT-1].wDataPresent )
    {
        /* Convert OEMTEXT -> TEXT */
        lpSource = &ClipFormats[CF_OEMTEXT-1];
        lpTarget = &ClipFormats[CF_TEXT-1];

        TRACE("\tOEMTEXT -> TEXT\n");
    }
    /* Asked for CF_OEM_TEXT, and CF_TEXT available */
    else if( wFormat == CF_OEMTEXT && !ClipFormats[CF_OEMTEXT-1].wDataPresent
				   &&  ClipFormats[CF_TEXT-1].wDataPresent )
    {
	/* Convert TEXT -> OEMTEXT */
        lpSource = &ClipFormats[CF_TEXT-1];
	lpTarget = &ClipFormats[CF_OEMTEXT-1];
	
	TRACE("\tTEXT -> OEMTEXT\n");
    }
    /* Text format requested is available - no conversion necessary */
    else
    {
	lpSource = __lookup_format( ClipFormats, wFormat );
	lpTarget = lpSource;
    }

    /* First render the source text format */
    if ( !lpSource || !CLIPBOARD_RenderFormat(lpSource) ) return NULL;

    /* Convert to the desired target text format, if necessary */
    if( lpTarget != lpSource && !lpTarget->hData16 && !lpTarget->hData32 )
    {
        UINT16 size;
        LPCSTR lpstrS; 
        LPSTR  lpstrT;
    
        if (lpSource->hData32)
        {
          size = GlobalSize( lpSource->hData32 );
          lpstrS = (LPSTR)GlobalLock(lpSource->hData32);
        }
        else
        {
          size = GlobalSize16( lpSource->hData16 );
          lpstrS = (LPSTR)GlobalLock16(lpSource->hData16);
        }
    
        if( !lpstrS ) return NULL;
        TRACE("\tconverting from '%s' to '%s', %i chars\n",
                                          lpSource->Name, lpTarget->Name, size);
    
        lpTarget->hData32 = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, size);
        lpstrT = (LPSTR)GlobalLock(lpTarget->hData32);
    
        if( lpstrT )
        {
            if( lpSource->wFormatID == CF_TEXT )
                CharToOemBuffA(lpstrS, lpstrT, size);
            else
                OemToCharBuffA(lpstrS, lpstrT, size);
            TRACE("\tgot %s\n", lpstrT);
            GlobalUnlock(lpTarget->hData32);
        }
        else
            lpTarget->hData32 = 0;

        /* Unlock source */
        if (lpSource->hData32)
          GlobalUnlock(lpSource->hData32);
        else
          GlobalUnlock16(lpSource->hData16);
    }

    return (lpTarget->hData16 || lpTarget->hData32) ? lpTarget : NULL;
}

/**************************************************************************
 *                WIN32 Clipboard implementation 
 **************************************************************************/

/**************************************************************************
 *            OpenClipboard16   (USER.137)
 */
BOOL16 WINAPI OpenClipboard16( HWND16 hWnd )
{
    return OpenClipboard( hWnd );
}


/**************************************************************************
 *            OpenClipboard32   (USER32.407)
 *
 * Note: Netscape uses NULL hWnd to open the clipboard.
 */
BOOL WINAPI OpenClipboard( HWND hWnd )
{
    BOOL bRet;

    TRACE("(%04x)...\n", hWnd);

    if (!hClipLock)
    {
        hClipLock = GetCurrentTask();

        /* Save current user of the clipboard */
        hWndClipWindow = hWnd;
        bCBHasChanged = FALSE;
        bRet = TRUE;
    }
    else bRet = FALSE;

    TRACE("   returning %i\n", bRet);
    return bRet;
}


/**************************************************************************
 *            CloseClipboard16   (USER.138)
 */
BOOL16 WINAPI CloseClipboard16(void)
{
    return CloseClipboard();
}


/**************************************************************************
 *            CloseClipboard32   (USER32.54)
 */
BOOL WINAPI CloseClipboard(void)
{
    TRACE("()\n");

    if (hClipLock == GetCurrentTask())
    {
	hWndClipWindow = 0;

        if (bCBHasChanged && hWndViewer) 
	    SendMessage16(hWndViewer, WM_DRAWCLIPBOARD, 0, 0L);
	hClipLock = 0;
    }
    return TRUE;
}


/**************************************************************************
 *            EmptyClipboard16   (USER.139)
 */
BOOL16 WINAPI EmptyClipboard16(void)
{
    return EmptyClipboard();
}


/**************************************************************************
 *            EmptyClipboard32   (USER32.169)
 *  Empties and acquires ownership of the clipboard
 */
BOOL WINAPI EmptyClipboard(void)
{
    TRACE("()\n");

    if (hClipLock != GetCurrentTask())
    {
        WARN("Clipboard not opened by calling task!");
        return FALSE;
    }
    
    /* destroy private objects */

    if (hWndClipOwner)
	SendMessage16(hWndClipOwner, WM_DESTROYCLIPBOARD, 0, 0L);

    /* empty the cache */
    CLIPBOARD_EmptyCache(TRUE);

    /* Assign ownership of the clipboard to the current client */
    hWndClipOwner = hWndClipWindow;

    /* Save the current task */
    hTaskClipOwner = GetCurrentTask();
    
    /* Tell the driver to acquire the selection */
    CLIPBOARD_Driver->pAcquire();

    return TRUE;
}


/**************************************************************************
 *            GetClipboardOwner16   (USER.140)
 *  FIXME: Can't return the owner if the clipbard is owned by an external app
 */
HWND16 WINAPI GetClipboardOwner16(void)
{
    TRACE("()\n");
    return hWndClipOwner;
}


/**************************************************************************
 *            GetClipboardOwner32   (USER32.225)
 *  FIXME: Can't return the owner if the clipbard is owned by an external app
 */
HWND WINAPI GetClipboardOwner(void)
{
    TRACE("()\n");
    return hWndClipOwner;
}


/**************************************************************************
 *            SetClipboardData16   (USER.141)
 */
HANDLE16 WINAPI SetClipboardData16( UINT16 wFormat, HANDLE16 hData )
{
    LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    TRACE("(%04X, %04x) !\n", wFormat, hData);

    /* NOTE: If the hData is zero and current owner doesn't match
     * the window that opened the clipboard then this application
     * is screwed because WM_RENDERFORMAT will go to the owner.
     * (to become the owner it must call EmptyClipboard() before
     *  adding new data).
     */

    if( CLIPBOARD_IsLocked() || !lpFormat ||
        (!hData && (!hWndClipOwner || (hWndClipOwner != hWndClipWindow))) )
    {
        WARN("Invalid hData or clipboard not opened by calling task!\n");
        return 0;
    }

    /* Pass on the request to the driver */
    CLIPBOARD_Driver->pSetData(wFormat);

    if ( lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32 ) 
    {
	CLIPBOARD_DeleteRecord(lpFormat, TRUE);

	/* delete existing CF_TEXT/CF_OEMTEXT aliases */

	if( wFormat == CF_TEXT 
	    && ( ClipFormats[CF_OEMTEXT-1].hData16 
		 ||  ClipFormats[CF_OEMTEXT-1].hData32 )
	    && !ClipFormats[CF_OEMTEXT-1].wDataPresent )
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
        if( wFormat == CF_OEMTEXT 
	    && ( ClipFormats[CF_OEMTEXT-1].hData16 
		 ||  ClipFormats[CF_OEMTEXT-1].hData32 )
	    && !ClipFormats[CF_TEXT-1].wDataPresent )
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
    }

    bCBHasChanged = TRUE;
    lpFormat->wDataPresent = 1;
    lpFormat->hData16 = hData;          /* 0 is legal, see WM_RENDERFORMAT */
    lpFormat->hData32 = 0;

    return lpFormat->hData16;
}


/**************************************************************************
 *            SetClipboardData   (USER32.470)
 */
HANDLE WINAPI SetClipboardData( UINT wFormat, HANDLE hData )
{
    LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    TRACE("(%08X, %08x) !\n", wFormat, hData);

    /* NOTE: If the hData is zero and current owner doesn't match
     * the window that opened the clipboard then this application
     * is screwed because WM_RENDERFORMAT will go to the owner.
     * (to become the owner it must call EmptyClipboard() before
     *  adding new data).
     */

    if( CLIPBOARD_IsLocked() || !lpFormat ||
        (!hData && (!hWndClipOwner || (hWndClipOwner != hWndClipWindow))) )
    {
        WARN("Invalid hData or clipboard not opened by calling task!\n");
        return 0;
    }

    /* Tell the driver to acquire the selection */
    CLIPBOARD_Driver->pAcquire();

    if ( lpFormat->wDataPresent &&
         (lpFormat->hData16 || lpFormat->hData32) )
    {
	CLIPBOARD_DeleteRecord(lpFormat, TRUE);

	/* delete existing CF_TEXT/CF_OEMTEXT aliases */

	if( wFormat == CF_TEXT 
	    && ( ClipFormats[CF_OEMTEXT-1].hData16 
		 ||  ClipFormats[CF_OEMTEXT-1].hData32 )
	    && !ClipFormats[CF_OEMTEXT-1].wDataPresent )
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
        if( wFormat == CF_OEMTEXT 
	    && ( ClipFormats[CF_OEMTEXT-1].hData16 
		 ||  ClipFormats[CF_OEMTEXT-1].hData32 )
	    && !ClipFormats[CF_TEXT-1].wDataPresent )
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
    }

    bCBHasChanged = TRUE;
    lpFormat->wDataPresent = 1;
    lpFormat->hDataSrc32 = hData;  /* Save the source handle */

    /*
     * Make a shared duplicate if the memory is not shared
     * TODO: What should be done for non-memory objects
     */
    if ( CLIPBOARD_IsMemoryObject(wFormat) && hData && !(GlobalFlags(hData) & GMEM_DDESHARE) )
        lpFormat->hData32 = CLIPBOARD_GlobalDupMem( hData );
    else
        lpFormat->hData32 = hData;          /* 0 is legal, see WM_RENDERFORMAT */
    
    lpFormat->hData16 = 0;

    return lpFormat->hData32;   /* Should we return lpFormat->hDataSrc32 */
}


/**************************************************************************
 *             GetClipboardData16   (USER.142)
 */
HANDLE16 WINAPI GetClipboardData16( UINT16 wFormat )
{
    LPWINE_CLIPFORMAT lpRender = ClipFormats; 

    TRACE("(%04X)\n", wFormat);

    if (CLIPBOARD_IsLocked())
    {
        WARN("Clipboard not opened by calling task!\n");
        return 0;
    }

    if( wFormat == CF_TEXT || wFormat == CF_OEMTEXT )
    {
	lpRender = CLIPBOARD_RenderText(wFormat);
        if ( !lpRender ) return 0;
    }
    else
    {
	lpRender = __lookup_format( ClipFormats, wFormat );
        if( !lpRender || !CLIPBOARD_RenderFormat(lpRender) ) return 0;
    }
   
    /* Convert between 32 -> 16 bit data, if necessary */
    if( lpRender->hData32 && !lpRender->hData16
        && CLIPBOARD_IsMemoryObject(wFormat) )
    {
      int size;
      if( lpRender->wFormatID == CF_METAFILEPICT )
	size = sizeof( METAFILEPICT16 );
      else
          size = GlobalSize(lpRender->hData32);
      
      lpRender->hData16 = GlobalAlloc16(GMEM_ZEROINIT, size);
      if( !lpRender->hData16 )
	ERR("(%04X) -- not enough memory in 16b heap\n", wFormat);
      else
      {
        if( lpRender->wFormatID == CF_METAFILEPICT )
        {
          FIXME("\timplement function CopyMetaFilePict32to16\n");
          FIXME("\tin the appropriate file.\n");
  #ifdef SOMEONE_IMPLEMENTED_ME
          CopyMetaFilePict32to16( GlobalLock16(lpRender->hData16), 
                                  GlobalLock(lpRender->hData32) );
  #endif
        }
        else
        {
          memcpy( GlobalLock16(lpRender->hData16), 
                  GlobalLock(lpRender->hData32), 
                  size );
        }
	GlobalUnlock16(lpRender->hData16);
	GlobalUnlock(lpRender->hData32);
      }
    }

    TRACE("\treturning %04x (type %i)\n", 
			      lpRender->hData16, lpRender->wFormatID);
    return lpRender->hData16;
}


/**************************************************************************
 *             GetClipboardData32   (USER32.222)
 */
HANDLE WINAPI GetClipboardData( UINT wFormat )
{
    LPWINE_CLIPFORMAT lpRender = ClipFormats; 

    TRACE("(%08X)\n", wFormat);

    if (CLIPBOARD_IsLocked())
    {
        WARN("Clipboard not opened by calling task!");
        return 0;
    }

    if( wFormat == CF_TEXT || wFormat == CF_OEMTEXT )
    {
	lpRender = CLIPBOARD_RenderText(wFormat);
        if ( !lpRender ) return 0;
    }
    else
    {
	lpRender = __lookup_format( ClipFormats, wFormat );
        if( !lpRender || !CLIPBOARD_RenderFormat(lpRender) ) return 0;
    }
   
    /* Convert between 16 -> 32 bit data, if necessary */
    if( lpRender->hData16 && !lpRender->hData32
        && CLIPBOARD_IsMemoryObject(wFormat) )
    {
      int size;
      if( lpRender->wFormatID == CF_METAFILEPICT )
	size = sizeof( METAFILEPICT );
      else
	size = GlobalSize16(lpRender->hData16);
      lpRender->hData32 = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE,
                                      size);
      if( lpRender->wFormatID == CF_METAFILEPICT )
      {
	FIXME("\timplement function CopyMetaFilePict16to32\n");
	FIXME("\tin the appropriate file.\n");
#ifdef SOMEONE_IMPLEMENTED_ME
	CopyMetaFilePict16to32( GlobalLock16(lpRender->hData32), 
			        GlobalLock(lpRender->hData16) );
#endif
      }
      else
      {
	memcpy( GlobalLock(lpRender->hData32), 
		GlobalLock16(lpRender->hData16), 
		size );
      }
      GlobalUnlock(lpRender->hData32);
      GlobalUnlock16(lpRender->hData16);
    }

    TRACE("\treturning %04x (type %i)\n", 
			      lpRender->hData32, lpRender->wFormatID);
    return lpRender->hData32;
}


/**************************************************************************
 *           CountClipboardFormats16   (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats();
}


/**************************************************************************
 *           CountClipboardFormats32   (USER32.63)
 */
INT WINAPI CountClipboardFormats(void)
{
    INT FormatCount = 0;
    LPWINE_CLIPFORMAT lpFormat = ClipFormats; 

    TRACE("()\n");

    while(TRUE) 
    {
	if (lpFormat == NULL) break;

        if( lpFormat->wFormatID != CF_TEXT ) /* Don't count CF_TEXT */
        {
          /*
           * The format is available if either:
           * 1. The data is already in the cache.
           * 2. The selection is not owned by us(WINE) and the data is
           *    available to the clipboard driver.
           */
          if ( lpFormat->wDataPresent ||
               ( !CLIPBOARD_Driver->pIsSelectionOwner()
                 && CLIPBOARD_Driver->pIsFormatAvailable( lpFormat->wFormatID ) ) )
          {
              TRACE("\tdata found for format 0x%04x(%s)\n",
                    lpFormat->wFormatID, CLIPBOARD_GetFormatName(lpFormat->wFormatID));
              FormatCount++;
          }
        }

	lpFormat = lpFormat->NextFormat;
    }

    /* these two are equivalent, adjust the total */

    FormatCount += abs(ClipFormats[CF_TEXT-1].wDataPresent -
                       ClipFormats[CF_OEMTEXT-1].wDataPresent);

    TRACE("\ttotal %d\n", FormatCount);
    return FormatCount;
}


/**************************************************************************
 *            EnumClipboardFormats16   (USER.144)
 */
UINT16 WINAPI EnumClipboardFormats16( UINT16 wFormat )
{
    return EnumClipboardFormats( wFormat );
}


/**************************************************************************
 *            EnumClipboardFormats32   (USER32.179)
 */
UINT WINAPI EnumClipboardFormats( UINT wFormat )
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;
    BOOL bFormatPresent;

    TRACE("(%04X)\n", wFormat);

    if (CLIPBOARD_IsLocked())
    {
        WARN("Clipboard not opened by calling task!");
        return 0;
    }

    if (wFormat == 0) /* start from the beginning */
	lpFormat = ClipFormats;
    else
    {
	/* walk up to the specified format record */

	if( !(lpFormat = __lookup_format( lpFormat, wFormat )) ) 
	    return 0;
	lpFormat = lpFormat->NextFormat; /* right */
    }

    while(TRUE) 
    {
	if (lpFormat == NULL) return 0;

        /* Synthesize CF_TEXT from CF_OEMTEXT and vice versa */
	bFormatPresent = (lpFormat->wDataPresent ||
           (lpFormat->wFormatID == CF_OEMTEXT && ClipFormats[CF_TEXT-1].wDataPresent) ||
	   (lpFormat->wFormatID == CF_TEXT && ClipFormats[CF_OEMTEXT-1].wDataPresent) );

        /* Query the driver if not yet in the cache */
        if (!bFormatPresent && !CLIPBOARD_Driver->pIsSelectionOwner())
        {
            bFormatPresent =
               CLIPBOARD_Driver->pIsFormatAvailable( (lpFormat->wFormatID == CF_TEXT) ?
                                                      CF_OEMTEXT : lpFormat->wFormatID );
        }

	if (bFormatPresent)
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
    LPWINE_CLIPFORMAT lpNewFormat; 
    LPWINE_CLIPFORMAT lpFormat = ClipFormats; 

    if (FormatName == NULL) return 0;

    TRACE("('%s') !\n", FormatName);

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

    lpNewFormat = (LPWINE_CLIPFORMAT)xmalloc(sizeof(WINE_CLIPFORMAT));
    lpFormat->NextFormat = lpNewFormat;
    lpNewFormat->wFormatID = LastRegFormat;
    lpNewFormat->wRefCount = 1;

    lpNewFormat->Name = (LPSTR)xmalloc(strlen(FormatName) + 1);
    strcpy(lpNewFormat->Name, FormatName);

    lpNewFormat->wDataPresent = 0;
    lpNewFormat->hData16 = 0;
    lpNewFormat->hDataSrc32 = 0;
    lpNewFormat->hData32 = 0;
    lpNewFormat->drvData = 0;
    lpNewFormat->PrevFormat = lpFormat;
    lpNewFormat->NextFormat = NULL;

    /* Pass on the registration request to the driver */
    CLIPBOARD_Driver->pRegisterFormat( FormatName );
    
    return LastRegFormat++;
}


/**************************************************************************
 *            RegisterClipboardFormat32A   (USER32.431)
 */
UINT WINAPI RegisterClipboardFormatA( LPCSTR formatName )
{
    return RegisterClipboardFormat16( formatName );
}


/**************************************************************************
 *            RegisterClipboardFormat32W   (USER32.432)
 */
UINT WINAPI RegisterClipboardFormatW( LPCWSTR formatName )
{
    LPSTR aFormat = HEAP_strdupWtoA( GetProcessHeap(), 0, formatName );
    UINT ret = RegisterClipboardFormatA( aFormat );
    HeapFree( GetProcessHeap(), 0, aFormat );
    return ret;
}


/**************************************************************************
 *            GetClipboardFormatName16   (USER.146)
 */
INT16 WINAPI GetClipboardFormatName16( UINT16 wFormat, LPSTR retStr, INT16 maxlen )
{
    return GetClipboardFormatNameA( wFormat, retStr, maxlen );
}


/**************************************************************************
 *            GetClipboardFormatName32A   (USER32.223)
 */
INT WINAPI GetClipboardFormatNameA( UINT wFormat, LPSTR retStr, INT maxlen )
{
    LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    TRACE("(%04X, %p, %d) !\n", wFormat, retStr, maxlen);

    if (lpFormat == NULL || lpFormat->Name == NULL || 
	lpFormat->wFormatID < CF_REGFORMATBASE) return 0;

    TRACE("Name='%s' !\n", lpFormat->Name);

    lstrcpynA( retStr, lpFormat->Name, maxlen );
    return strlen(retStr);
}


/**************************************************************************
 *            GetClipboardFormatName32W   (USER32.224)
 */
INT WINAPI GetClipboardFormatNameW( UINT wFormat, LPWSTR retStr, INT maxlen )
{
    LPSTR p = HEAP_xalloc( GetProcessHeap(), 0, maxlen );
    INT ret = GetClipboardFormatNameA( wFormat, p, maxlen );
    lstrcpynAtoW( retStr, p, maxlen );
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/**************************************************************************
 *            SetClipboardViewer16   (USER.147)
 */
HWND16 WINAPI SetClipboardViewer16( HWND16 hWnd )
{
    TRACE("(%04x)\n", hWnd);
    return SetClipboardViewer( hWnd );
}


/**************************************************************************
 *            SetClipboardViewer32   (USER32.471)
 */
HWND WINAPI SetClipboardViewer( HWND hWnd )
{
    HWND hwndPrev = hWndViewer;

    TRACE("(%04x): returning %04x\n", hWnd, hwndPrev);

    hWndViewer = hWnd;
    return hwndPrev;
}


/**************************************************************************
 *           GetClipboardViewer16   (USER.148)
 */
HWND16 WINAPI GetClipboardViewer16(void)
{
    TRACE("()\n");
    return hWndViewer;
}


/**************************************************************************
 *           GetClipboardViewer32   (USER32.226)
 */
HWND WINAPI GetClipboardViewer(void)
{
    TRACE("()\n");
    return hWndViewer;
}


/**************************************************************************
 *           ChangeClipboardChain16   (USER.149)
 */
BOOL16 WINAPI ChangeClipboardChain16(HWND16 hWnd, HWND16 hWndNext)
{
    return ChangeClipboardChain(hWnd, hWndNext);
}


/**************************************************************************
 *           ChangeClipboardChain32   (USER32.22)
 */
BOOL WINAPI ChangeClipboardChain(HWND hWnd, HWND hWndNext)
{
    BOOL bRet = 0;

    FIXME("(0x%04x, 0x%04x): stub?\n", hWnd, hWndNext);

    if( hWndViewer )
	bRet = !SendMessage16( hWndViewer, WM_CHANGECBCHAIN,
                             (WPARAM16)hWnd, (LPARAM)hWndNext);   
    else
	WARN("hWndViewer is lost\n");

    if( hWnd == hWndViewer ) hWndViewer = hWndNext;

    return bRet;
}


/**************************************************************************
 *           IsClipboardFormatAvailable16   (USER.193)
 */
BOOL16 WINAPI IsClipboardFormatAvailable16( UINT16 wFormat )
{
    return IsClipboardFormatAvailable( wFormat );
}


/**************************************************************************
 *           IsClipboardFormatAvailable32   (USER32.340)
 */
BOOL WINAPI IsClipboardFormatAvailable( UINT wFormat )
{
    BOOL bRet;

    if (wFormat == 0)  /* Reject this case quickly */
        bRet = FALSE;

    /* If WINE is not the clipboard selection owner ask the clipboard driver */
    else if ( !CLIPBOARD_Driver->pIsSelectionOwner() )
        bRet = CLIPBOARD_Driver->pIsFormatAvailable( (wFormat == CF_TEXT) ?
                                                     CF_OEMTEXT : wFormat );
    /* Check if the format is in the local cache */
    else 
        bRet = CLIPBOARD_IsPresent(wFormat);

    TRACE("(%04X)- ret(%d)\n", wFormat, bRet);
    return bRet;
}


/**************************************************************************
 *             GetOpenClipboardWindow16   (USER.248)
 *  FIXME: This wont work if an external app owns the selection
 */
HWND16 WINAPI GetOpenClipboardWindow16(void)
{
    TRACE("()\n");
    return hWndClipWindow;
}


/**************************************************************************
 *             GetOpenClipboardWindow32   (USER32.277)
 *  FIXME: This wont work if an external app owns the selection
 */
HWND WINAPI GetOpenClipboardWindow(void)
{
    TRACE("()\n");
    return hWndClipWindow;
}


/**************************************************************************
 *             GetPriorityClipboardFormat16   (USER.402)
 */
INT16 WINAPI GetPriorityClipboardFormat16( UINT16 *lpPriorityList, INT16 nCount)
{
    FIXME("(%p,%d): stub\n", lpPriorityList, nCount );
    return 0;
}


/**************************************************************************
 *             GetPriorityClipboardFormat32   (USER32.279)
 */
INT WINAPI GetPriorityClipboardFormat( UINT *lpPriorityList, INT nCount )
{
    int Counter;
    TRACE("()\n");

    if(CountClipboardFormats() == 0) 
    { 
        return 0;
    }

    for(Counter = 0; Counter <= nCount; Counter++)
    {
        if(IsClipboardFormatAvailable(*(lpPriorityList+sizeof(INT)*Counter)))
            return *(lpPriorityList+sizeof(INT)*Counter);
    }

    return -1;
}

