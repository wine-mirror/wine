/*
 * WIN32 clipboard implementation
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
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
 *    This file contains the implementation for the WIN32 Clipboard API
 * and Wine's internal clipboard cache.
 * The actual contents of the clipboard are held in the clipboard cache.
 * The internal implementation talks to a "clipboard driver" to fill or
 * expose the cache to the native device. (Currently only the X11 and
 * TTY clipboard  driver are available)
 */

#include "config.h"
#include "wine/port.h"

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
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "heap.h"
#include "user.h"
#include "win.h"
#include "clipboard.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

#define  CF_REGFORMATBASE 	0xC000

/**************************************************************************
 *	  Clipboard context global variables
 */

static HANDLE hClipLock   = 0;
static BOOL bCBHasChanged  = FALSE;

static HWND hWndClipWindow;        /* window that last opened clipboard */
static HWND hWndClipOwner;         /* current clipboard owner */
static HANDLE16 hTaskClipOwner;    /* clipboard owner's task  */
static HWND hWndViewer;            /* start of viewers chain */

/* Clipboard cache initial data.
 * WARNING: This data ordering is dependent on the WINE_CLIPFORMAT structure
 * declared in clipboard.h
 */
WINE_CLIPFORMAT ClipFormats[]  = {
    { CF_TEXT, 1, 0, "Text",  0, 0, 0, 0, NULL, &ClipFormats[1]},
    { CF_BITMAP, 1, 0, "Bitmap", 0, 0, 0, 0, &ClipFormats[0], &ClipFormats[2]},
    { CF_METAFILEPICT, 1, 0, "MetaFile Picture", 0, 0, 0, 0, &ClipFormats[1], &ClipFormats[3]},
    { CF_SYLK, 1, 0, "Sylk", 0, 0, 0, 0, &ClipFormats[2], &ClipFormats[4]},
    { CF_DIF, 1, 0, "DIF", 0, 0, 0, 0, &ClipFormats[3], &ClipFormats[5]},
    { CF_TIFF, 1, 0, "TIFF", 0, 0, 0, 0, &ClipFormats[4], &ClipFormats[6]},
    { CF_OEMTEXT, 1, 0, "OEM Text", 0, 0, 0, 0, &ClipFormats[5], &ClipFormats[7]},
    { CF_DIB, 1, 0, "DIB", 0, 0, 0, 0, &ClipFormats[6], &ClipFormats[8]},
    { CF_PALETTE, 1, 0, "Palette", 0, 0, 0, 0, &ClipFormats[7], &ClipFormats[9]},
    { CF_PENDATA, 1, 0, "PenData", 0, 0, 0, 0, &ClipFormats[8], &ClipFormats[10]},
    { CF_RIFF, 1, 0, "RIFF", 0, 0, 0, 0, &ClipFormats[9], &ClipFormats[11]},
    { CF_WAVE, 1, 0, "Wave", 0, 0, 0, 0, &ClipFormats[10], &ClipFormats[12]},
    { CF_UNICODETEXT, 1, 0, "Unicode Text", 0, 0, 0, 0, &ClipFormats[11], &ClipFormats[13]},
    { CF_OWNERDISPLAY, 1, 0, "Owner Display", 0, 0, 0, 0, &ClipFormats[12], &ClipFormats[14]},
    { CF_DSPTEXT, 1, 0, "DSPText", 0, 0, 0, 0, &ClipFormats[13], &ClipFormats[15]},
    { CF_DSPMETAFILEPICT, 1, 0, "DSPMetaFile Picture", 0, 0, 0, 0, &ClipFormats[14], &ClipFormats[16]},
    { CF_DSPBITMAP, 1, 0, "DSPBitmap", 0, 0, 0, 0, &ClipFormats[15], &ClipFormats[17]},
    { CF_HDROP, 1, 0, "HDROP", 0, 0, 0, 0, &ClipFormats[16], &ClipFormats[18]},
    { CF_ENHMETAFILE, 1, 0, "Enhmetafile", 0, 0, 0, 0, &ClipFormats[17], NULL}
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
#if 0
      MESSAGEQUEUE *queue = QUEUE_Current();

      if ( queue
           && queue->smWaiting
           && queue->smWaiting->msg == WM_RENDERFORMAT
           && queue->smWaiting->hSrcQueue
         )
  	bIsLocked = FALSE;
#else
      /* FIXME: queue check no longer possible */
      bIsLocked = FALSE;
#endif
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
	DeleteObject(lpFormat->hData16);
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

    if( wFormat == CF_TEXT || wFormat == CF_OEMTEXT || wFormat == CF_UNICODETEXT )
        return ClipFormats[CF_TEXT-1].wDataPresent ||
	       ClipFormats[CF_OEMTEXT-1].wDataPresent ||
	       ClipFormats[CF_UNICODETEXT-1].wDataPresent;
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
char * CLIPBOARD_GetFormatName(UINT wFormat, LPSTR buf, INT size)
{
    LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    if (lpFormat)
    {
        if (buf)
        {
            strncpy(buf, lpFormat->Name, size);
            CharLowerA(buf);
        }

        return lpFormat->Name;
    }
    else
        return NULL;
}


/**************************************************************************
 *                      CLIPBOARD_RenderFormat
 */
static BOOL CLIPBOARD_RenderFormat(LPWINE_CLIPFORMAT lpFormat)
{
  /*
   * If WINE is not the selection owner, and the format is available
   * we must ask the driver to render the data to the clipboard cache.
   */
  TRACE("enter format=%d\n", lpFormat->wFormatID);
  if ( !USER_Driver.pIsSelectionOwner()
       && USER_Driver.pIsClipboardFormatAvailable( lpFormat->wFormatID ) )
  {
    if ( !USER_Driver.pGetClipboardData( lpFormat->wFormatID ) )
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
      SendMessageW( hWndClipOwner, WM_RENDERFORMAT, (WPARAM)lpFormat->wFormatID, 0 );
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
    LPWINE_CLIPFORMAT lpTarget = NULL;
    BOOL foundData = FALSE;

    /* Asked for CF_TEXT */
    if( wFormat == CF_TEXT)
    {
        if(ClipFormats[CF_TEXT-1].wDataPresent)
        {
            lpSource = &ClipFormats[CF_TEXT-1];
            lpTarget = &ClipFormats[CF_TEXT-1];
            foundData = TRUE;
            TRACE("\t TEXT -> TEXT\n");
        }
	else if(ClipFormats[CF_UNICODETEXT-1].wDataPresent)
	{
	    /* Convert UNICODETEXT -> TEXT */
	    lpSource = &ClipFormats[CF_UNICODETEXT-1];
	    lpTarget = &ClipFormats[CF_TEXT-1];
            foundData = TRUE;
	    TRACE("\tUNICODETEXT -> TEXT\n");
	}
	else if(ClipFormats[CF_OEMTEXT-1].wDataPresent)
	{
	    /* Convert OEMTEXT -> TEXT */
	    lpSource = &ClipFormats[CF_OEMTEXT-1];
	    lpTarget = &ClipFormats[CF_TEXT-1];
            foundData = TRUE;
	    TRACE("\tOEMTEXT -> TEXT\n");
	}
    }
    /* Asked for CF_OEMTEXT  */
    else if( wFormat == CF_OEMTEXT)
    {
        if(ClipFormats[CF_OEMTEXT-1].wDataPresent)
        {
   	    lpSource = &ClipFormats[CF_OEMTEXT-1];
	    lpTarget = &ClipFormats[CF_OEMTEXT-1];
	    foundData = TRUE;
	    TRACE("\tOEMTEXT -> OEMTEXT\n");
        }
	else if(ClipFormats[CF_UNICODETEXT-1].wDataPresent)
	{
	    /* Convert UNICODETEXT -> OEMTEXT */
	    lpSource = &ClipFormats[CF_UNICODETEXT-1];
	    lpTarget = &ClipFormats[CF_OEMTEXT-1];
	    foundData = TRUE;
	    TRACE("\tUNICODETEXT -> OEMTEXT\n");
	}
	else if(ClipFormats[CF_TEXT-1].wDataPresent)
	{
	    /* Convert TEXT -> OEMTEXT */
	    lpSource = &ClipFormats[CF_TEXT-1];
	    lpTarget = &ClipFormats[CF_OEMTEXT-1];
            foundData = TRUE;
	    TRACE("\tTEXT -> OEMTEXT\n");
	}
    }
    /* Asked for CF_UNICODETEXT */
    else if( wFormat == CF_UNICODETEXT )
    {
	if(ClipFormats[CF_UNICODETEXT-1].wDataPresent)
	{
	    lpSource = &ClipFormats[CF_UNICODETEXT-1];
	    lpTarget = &ClipFormats[CF_UNICODETEXT-1];
            foundData = TRUE;
	    TRACE("\tUNICODETEXT -> UNICODETEXT\n");
	}
        else if(ClipFormats[CF_TEXT-1].wDataPresent)
	{
	    /* Convert TEXT -> UNICODETEXT */
	    lpSource = &ClipFormats[CF_TEXT-1];
	    lpTarget = &ClipFormats[CF_UNICODETEXT-1];
            foundData = TRUE;
	    TRACE("\tTEXT -> UNICODETEXT\n");
	}
	else if(ClipFormats[CF_OEMTEXT-1].wDataPresent)
	{
	    /* Convert OEMTEXT -> UNICODETEXT */
	    lpSource = &ClipFormats[CF_OEMTEXT-1];
	    lpTarget = &ClipFormats[CF_UNICODETEXT-1];
            foundData = TRUE;
	    TRACE("\tOEMTEXT -> UNICODETEXT\n");
	}
    }
    if (!foundData)
    {
        if ((wFormat == CF_TEXT) || (wFormat == CF_OEMTEXT))
        {
            lpSource = &ClipFormats[CF_UNICODETEXT-1];
            lpTarget = __lookup_format( ClipFormats, wFormat );
        }
        else
        {
	    lpSource = __lookup_format( ClipFormats, wFormat );
	    lpTarget = lpSource;
        }
    }

    /* First render the source text format */
    if ( !lpSource || !CLIPBOARD_RenderFormat(lpSource) ) return NULL;

    /* Convert to the desired target text format, if necessary */
    if( lpTarget != lpSource && !lpTarget->hData16 && !lpTarget->hData32 )
    {
        INT src_chars, dst_chars, alloc_size;
        LPCSTR lpstrS;
        LPSTR  lpstrT;

        if (lpSource->hData32)
        {
          lpstrS = (LPSTR)GlobalLock(lpSource->hData32);
        }
        else
        {
          lpstrS = (LPSTR)GlobalLock16(lpSource->hData16);
        }

        if( !lpstrS ) return NULL;

	/* Text always NULL terminated */
	if(lpSource->wFormatID == CF_UNICODETEXT)
	    src_chars = strlenW((LPCWSTR)lpstrS)+1;
	else
	    src_chars = strlen(lpstrS)+1;

	/* Calculate number of characters in the destination buffer */
	dst_chars = CLIPBOARD_ConvertText(lpSource->wFormatID, lpstrS, src_chars,
				     lpTarget->wFormatID, NULL, 0);
	if(!dst_chars) return NULL;

        TRACE("\tconverting from '%s' to '%s', %i chars\n",
		lpSource->Name, lpTarget->Name, src_chars);

	/* Convert characters to bytes */
	if(lpTarget->wFormatID == CF_UNICODETEXT)
	    alloc_size = dst_chars * sizeof(WCHAR);
	else
	    alloc_size = dst_chars;

        lpTarget->hData32 = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE | GMEM_DDESHARE, alloc_size);
        lpstrT = (LPSTR)GlobalLock(lpTarget->hData32);

        if( lpstrT )
        {
	    CLIPBOARD_ConvertText(lpSource->wFormatID, lpstrS, src_chars,
				  lpTarget->wFormatID, lpstrT, dst_chars);
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
 *            CLIPBOARD_EnumClipboardFormats   (internal)
 */
static UINT CLIPBOARD_EnumClipboardFormats( UINT wFormat )
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;
    BOOL bFormatPresent;

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

	if(CLIPBOARD_IsPresent(lpFormat->wFormatID))
	    break;

        /* Query the driver if not yet in the cache */
        if (!USER_Driver.pIsSelectionOwner())
        {
	    if(lpFormat->wFormatID == CF_UNICODETEXT ||
	       lpFormat->wFormatID == CF_TEXT ||
	       lpFormat->wFormatID == CF_OEMTEXT)
	    {
		if(USER_Driver.pIsClipboardFormatAvailable(CF_UNICODETEXT) ||
		   USER_Driver.pIsClipboardFormatAvailable(CF_TEXT) ||
		   USER_Driver.pIsClipboardFormatAvailable(CF_OEMTEXT))
		    bFormatPresent = TRUE;
		else
		    bFormatPresent = FALSE;
	    }
	    else
		bFormatPresent = USER_Driver.pIsClipboardFormatAvailable(lpFormat->wFormatID);

	    if(bFormatPresent)
		break;
        }

	lpFormat = lpFormat->NextFormat;
    }

    TRACE("Next available format %d\n", lpFormat->wFormatID);

    return lpFormat->wFormatID;
}


/**************************************************************************
 *                WIN32 Clipboard implementation
 **************************************************************************/

/**************************************************************************
 *		OpenClipboard (USER32.@)
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
        hWndClipWindow = WIN_GetFullHandle( hWnd );
        bCBHasChanged = FALSE;
        bRet = TRUE;
    }
    else bRet = FALSE;

    TRACE("   returning %i\n", bRet);
    return bRet;
}


/**************************************************************************
 *		CloseClipboard (USER.138)
 */
BOOL16 WINAPI CloseClipboard16(void)
{
    return CloseClipboard();
}


/**************************************************************************
 *		CloseClipboard (USER32.@)
 */
BOOL WINAPI CloseClipboard(void)
{
    TRACE("()\n");

    if (hClipLock == GetCurrentTask())
    {
	hWndClipWindow = 0;

        if (bCBHasChanged && hWndViewer) SendMessageW( hWndViewer, WM_DRAWCLIPBOARD, 0, 0 );
	hClipLock = 0;
    }
    return TRUE;
}


/**************************************************************************
 *		EmptyClipboard (USER.139)
 */
BOOL16 WINAPI EmptyClipboard16(void)
{
    return EmptyClipboard();
}


/**************************************************************************
 *		EmptyClipboard (USER32.@)
 *  Empties and acquires ownership of the clipboard
 */
BOOL WINAPI EmptyClipboard(void)
{
    TRACE("()\n");

    if (hClipLock != GetCurrentTask())
    {
        WARN("Clipboard not opened by calling task!\n");
        return FALSE;
    }

    /* destroy private objects */

    if (hWndClipOwner) SendMessageW( hWndClipOwner, WM_DESTROYCLIPBOARD, 0, 0 );

    /* empty the cache */
    CLIPBOARD_EmptyCache(TRUE);

    /* Assign ownership of the clipboard to the current client */
    hWndClipOwner = hWndClipWindow;

    /* Save the current task */
    hTaskClipOwner = GetCurrentTask();

    /* Tell the driver to acquire the selection */
    USER_Driver.pAcquireClipboard();

    return TRUE;
}


/**************************************************************************
 *		GetClipboardOwner (USER32.@)
 *  FIXME: Can't return the owner if the clipbard is owned by an external app
 */
HWND WINAPI GetClipboardOwner(void)
{
    TRACE("()\n");
    return hWndClipOwner;
}


/**************************************************************************
 *		SetClipboardData (USER.141)
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
    USER_Driver.pSetClipboardData(wFormat);

    if ( lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32 )
    {
	CLIPBOARD_DeleteRecord(lpFormat, TRUE);

	/* delete existing CF_UNICODETEXT/CF_TEXT/CF_OEMTEXT aliases */
	if(wFormat == CF_UNICODETEXT)
	{
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
	}
	else if(wFormat == CF_TEXT)
	{
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_UNICODETEXT-1], TRUE);
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
	}
	else if(wFormat == CF_OEMTEXT)
	{
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_UNICODETEXT-1], TRUE);
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
	}
    }

    bCBHasChanged = TRUE;
    lpFormat->wDataPresent = 1;
    lpFormat->hData16 = hData;          /* 0 is legal, see WM_RENDERFORMAT */
    lpFormat->hData32 = 0;

    return lpFormat->hData16;
}


/**************************************************************************
 *		SetClipboardData (USER32.@)
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
    USER_Driver.pAcquireClipboard();

    if ( lpFormat->wDataPresent &&
         (lpFormat->hData16 || lpFormat->hData32) )
    {
	CLIPBOARD_DeleteRecord(lpFormat, TRUE);

	/* delete existing CF_UNICODETEXT/CF_TEXT/CF_OEMTEXT aliases */
	if(wFormat == CF_UNICODETEXT)
	{
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
	}
	else if(wFormat == CF_TEXT)
	{
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_UNICODETEXT-1], TRUE);
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_OEMTEXT-1], TRUE);
	}
	else if(wFormat == CF_OEMTEXT)
	{
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_UNICODETEXT-1], TRUE);
	    CLIPBOARD_DeleteRecord(&ClipFormats[CF_TEXT-1], TRUE);
	}
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
 *		GetClipboardData (USER.142)
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

    if( wFormat == CF_UNICODETEXT || wFormat == CF_TEXT || wFormat == CF_OEMTEXT )
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
 *		GetClipboardData (USER32.@)
 */
HANDLE WINAPI GetClipboardData( UINT wFormat )
{
    LPWINE_CLIPFORMAT lpRender = ClipFormats;

    TRACE("(%08X)\n", wFormat);

    if (CLIPBOARD_IsLocked())
    {
        WARN("Clipboard not opened by calling task!\n");
        return 0;
    }

    if( wFormat == CF_UNICODETEXT || wFormat == CF_TEXT || wFormat == CF_OEMTEXT )
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
 *		CountClipboardFormats (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats();
}


/**************************************************************************
 *		CountClipboardFormats (USER32.@)
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
               ( !USER_Driver.pIsSelectionOwner()
                 && USER_Driver.pIsClipboardFormatAvailable( lpFormat->wFormatID ) ) )
          {
              TRACE("\tdata found for format 0x%04x(%s)\n",
                    lpFormat->wFormatID, CLIPBOARD_GetFormatName(lpFormat->wFormatID, NULL, 0));
              FormatCount++;
          }
        }

	lpFormat = lpFormat->NextFormat;
    }

    /* these are equivalent, adjust the total */
    FormatCount += (ClipFormats[CF_UNICODETEXT-1].wDataPresent ||
		    ClipFormats[CF_TEXT-1].wDataPresent ||
		    ClipFormats[CF_OEMTEXT-1].wDataPresent) ? 1 : 0;

    TRACE("\ttotal %d\n", FormatCount);
    return FormatCount;
}

/**************************************************************************
 *		EnumClipboardFormats (USER.144)
 */
UINT16 WINAPI EnumClipboardFormats16( UINT16 wFormat )
{
    return EnumClipboardFormats( wFormat );
}


/**************************************************************************
 *		EnumClipboardFormats (USER32.@)
 */
UINT WINAPI EnumClipboardFormats( UINT wFormat )
{
    TRACE("(%04X)\n", wFormat);

    if (CLIPBOARD_IsLocked())
    {
        WARN("Clipboard not opened by calling task!\n");
        return 0;
    }

    return CLIPBOARD_EnumClipboardFormats(wFormat);
}


/**************************************************************************
 *		RegisterClipboardFormatA (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatA( LPCSTR FormatName )
{
    LPWINE_CLIPFORMAT lpNewFormat;
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;

    if (FormatName == NULL) return 0;

    TRACE("('%s') !\n", FormatName);

    /* walk format chain to see if it's already registered */

    while(TRUE)
    {
	if ( !strcasecmp(lpFormat->Name,FormatName) )
	{
	     lpFormat->wRefCount++;
	     return lpFormat->wFormatID;
	}

	if ( lpFormat->NextFormat == NULL ) break;

	lpFormat = lpFormat->NextFormat;
    }

    /* allocate storage for new format entry */

    lpNewFormat = (LPWINE_CLIPFORMAT)HeapAlloc(GetProcessHeap(), 0, sizeof(WINE_CLIPFORMAT));
    if(lpNewFormat == NULL) {
        WARN("No more memory for a new format!\n");
        return 0;
    }
    lpFormat->NextFormat = lpNewFormat;
    lpNewFormat->wRefCount = 1;

    if (!(lpNewFormat->Name = HeapAlloc(GetProcessHeap(), 0, strlen(FormatName)+1 )))
    {
        WARN("No more memory for the new format name!\n");
        HeapFree(GetProcessHeap(), 0, lpNewFormat);
        return 0;
    }
    strcpy( lpNewFormat->Name, FormatName );
    CharLowerA(lpNewFormat->Name);

    lpNewFormat->wDataPresent = 0;
    lpNewFormat->hData16 = 0;
    lpNewFormat->hDataSrc32 = 0;
    lpNewFormat->hData32 = 0;
    lpNewFormat->drvData = 0;
    lpNewFormat->PrevFormat = lpFormat;
    lpNewFormat->NextFormat = NULL;

    /* Pass on the registration request to the driver */
    lpNewFormat->wFormatID = USER_Driver.pRegisterClipboardFormat(lpNewFormat->Name);

    TRACE("Registering format(%d): %s\n", lpNewFormat->wFormatID, FormatName);
    return lpNewFormat->wFormatID;
}


/**************************************************************************
 *		RegisterClipboardFormat (USER.145)
 */
UINT16 WINAPI RegisterClipboardFormat16( LPCSTR FormatName )
{
    return RegisterClipboardFormatA( FormatName );
}


/**************************************************************************
 *		RegisterClipboardFormatW (USER32.@)
 */
UINT WINAPI RegisterClipboardFormatW( LPCWSTR formatName )
{
    LPSTR aFormat = HEAP_strdupWtoA( GetProcessHeap(), 0, formatName );
    UINT ret = RegisterClipboardFormatA( aFormat );
    HeapFree( GetProcessHeap(), 0, aFormat );
    return ret;
}


/**************************************************************************
 *		GetClipboardFormatName (USER.146)
 */
INT16 WINAPI GetClipboardFormatName16( UINT16 wFormat, LPSTR retStr, INT16 maxlen )
{
    return GetClipboardFormatNameA( wFormat, retStr, maxlen );
}


/**************************************************************************
 *		GetClipboardFormatNameA (USER32.@)
 */
INT WINAPI GetClipboardFormatNameA( UINT wFormat, LPSTR retStr, INT maxlen )
{
    LPWINE_CLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    TRACE("(%04X, %p, %d) !\n", wFormat, retStr, maxlen);

    if (lpFormat == NULL || lpFormat->Name == NULL)
    {
        /* Check if another wine process already registered the format */
        if (wFormat && !USER_Driver.pGetClipboardFormatName(wFormat, retStr, maxlen))
        {
            RegisterClipboardFormatA(retStr); /* Make a cache entry */
            return strlen(retStr);
        }
        else
        {
            TRACE("wFormat=%d not found\n", wFormat);
            return 0;
        }
    }

    TRACE("Name='%s' !\n", lpFormat->Name);

    lstrcpynA( retStr, lpFormat->Name, maxlen );
    return strlen(retStr);
}


/**************************************************************************
 *		GetClipboardFormatNameW (USER32.@)
 */
INT WINAPI GetClipboardFormatNameW( UINT wFormat, LPWSTR retStr, INT maxlen )
{
    INT ret;
    LPSTR p = HeapAlloc( GetProcessHeap(), 0, maxlen );
    if(p == NULL) return 0; /* FIXME: is this the correct failure value? */

    ret = GetClipboardFormatNameA( wFormat, p, maxlen );

    if (maxlen > 0 && !MultiByteToWideChar( CP_ACP, 0, p, -1, retStr, maxlen ))
        retStr[maxlen-1] = 0;
    HeapFree( GetProcessHeap(), 0, p );
    return ret;
}


/**************************************************************************
 *		SetClipboardViewer (USER32.@)
 */
HWND WINAPI SetClipboardViewer( HWND hWnd )
{
    HWND hwndPrev = hWndViewer;

    TRACE("(%04x): returning %04x\n", hWnd, hwndPrev);

    hWndViewer = WIN_GetFullHandle( hWnd );
    return hwndPrev;
}


/**************************************************************************
 *		GetClipboardViewer (USER32.@)
 */
HWND WINAPI GetClipboardViewer(void)
{
    TRACE("()\n");
    return hWndViewer;
}


/**************************************************************************
 *		ChangeClipboardChain (USER32.@)
 */
BOOL WINAPI ChangeClipboardChain(HWND hWnd, HWND hWndNext)
{
    BOOL bRet = 0;

    FIXME("(0x%04x, 0x%04x): stub?\n", hWnd, hWndNext);

    if( hWndViewer )
        bRet = !SendMessageW( hWndViewer, WM_CHANGECBCHAIN, (WPARAM)hWnd, (LPARAM)hWndNext );
    else
	WARN("hWndViewer is lost\n");

    if( WIN_GetFullHandle(hWnd) == hWndViewer ) hWndViewer = WIN_GetFullHandle( hWndNext );

    return bRet;
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER.193)
 */
BOOL16 WINAPI IsClipboardFormatAvailable16( UINT16 wFormat )
{
    return IsClipboardFormatAvailable( wFormat );
}


/**************************************************************************
 *		IsClipboardFormatAvailable (USER32.@)
 */
BOOL WINAPI IsClipboardFormatAvailable( UINT wFormat )
{
    BOOL bRet;

    if (wFormat == 0)  /* Reject this case quickly */
        bRet = FALSE;
    else
    {
        UINT iret = CLIPBOARD_EnumClipboardFormats(wFormat - 1);
        if ((wFormat == CF_TEXT) || (wFormat == CF_OEMTEXT) || (wFormat == CF_UNICODETEXT))
            bRet = ((iret == CF_TEXT) || (iret == CF_OEMTEXT) || (iret == CF_UNICODETEXT));
        else
	    bRet = iret == wFormat;
    }
    TRACE("(%04X)- ret(%d)\n", wFormat, bRet);
    return bRet;
}


/**************************************************************************
 *		GetOpenClipboardWindow (USER32.@)
 *  FIXME: This wont work if an external app owns the selection
 */
HWND WINAPI GetOpenClipboardWindow(void)
{
    TRACE("()\n");
    return hWndClipWindow;
}


/**************************************************************************
 *		GetPriorityClipboardFormat (USER32.@)
 */
INT WINAPI GetPriorityClipboardFormat( UINT *list, INT nCount )
{
    int i;
    TRACE("()\n");

    if(CountClipboardFormats() == 0) return 0;

    for (i = 0; i < nCount; i++)
        if (IsClipboardFormatAvailable( list[i] )) return list[i];
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
	FIXME("Returning 0, see windows/clipboard.c\n");
	/* FIXME: Use serial numbers */
	return 0;
}
