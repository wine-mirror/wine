/*
 * WINE clipboard function handling
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include "windows.h"
#include "win.h"
#include "heap.h"
#include "message.h"
#include "queue.h"
#include "task.h"
#include "clipboard.h"
#include "xmalloc.h"
#include "debug.h"

#ifndef X_DISPLAY_MISSING
extern CLIPBOARD_DRIVER X11DRV_CLIPBOARD_Driver;
#else /* X_DISPLAY_MISSING */
extern CLIPBOARD_DRIVER TTYDRV_CLIPBOARD_Driver;
#endif /* X_DISPLAY_MISSING */

#define  CF_REGFORMATBASE 	0xC000

/**************************************************************************
 *			internal variables
 */

static HQUEUE16 hqClipLock   = 0;
static BOOL32 bCBHasChanged  = FALSE;

HWND32 hWndClipOwner  = 0;   /* current clipboard owner */
HWND32 hWndClipWindow = 0;   /* window that opened clipboard */
static HWND32 hWndViewer     = 0;   /* start of viewers chain */

static WORD LastRegFormat = CF_REGFORMATBASE;

CLIPFORMAT ClipFormats[16]  = {
    { CF_TEXT, 1, 0, "Text", (HANDLE32)NULL, 0, NULL, &ClipFormats[1] , (HANDLE16)NULL},
    { CF_BITMAP, 1, 0, "Bitmap", (HANDLE32)NULL, 0, &ClipFormats[0], &ClipFormats[2] , (HANDLE16)NULL},
    { CF_METAFILEPICT, 1, 0, "MetaFile Picture", (HANDLE32)NULL, 0, &ClipFormats[1], &ClipFormats[3] , (HANDLE16)NULL},
    { CF_SYLK, 1, 0, "Sylk", (HANDLE32)NULL, 0, &ClipFormats[2], &ClipFormats[4] , (HANDLE16)NULL},
    { CF_DIF, 1, 0, "DIF", (HANDLE32)NULL, 0, &ClipFormats[3], &ClipFormats[5] , (HANDLE16)NULL},
    { CF_TIFF, 1, 0, "TIFF", (HANDLE32)NULL, 0, &ClipFormats[4], &ClipFormats[6] , (HANDLE16)NULL},
    { CF_OEMTEXT, 1, 0, "OEM Text", (HANDLE32)NULL, 0, &ClipFormats[5], &ClipFormats[7] , (HANDLE16)NULL},
    { CF_DIB, 1, 0, "DIB", (HANDLE32)NULL, 0, &ClipFormats[6], &ClipFormats[8] , (HANDLE16)NULL},
    { CF_PALETTE, 1, 0, "Palette", (HANDLE32)NULL, 0, &ClipFormats[7], &ClipFormats[9] , (HANDLE16)NULL},
    { CF_PENDATA, 1, 0, "PenData", (HANDLE32)NULL, 0, &ClipFormats[8], &ClipFormats[10] , (HANDLE16)NULL},
    { CF_RIFF, 1, 0, "RIFF", (HANDLE32)NULL, 0, &ClipFormats[9], &ClipFormats[11] , (HANDLE16)NULL},
    { CF_WAVE, 1, 0, "Wave", (HANDLE32)NULL, 0, &ClipFormats[10], &ClipFormats[12] , (HANDLE16)NULL},
    { CF_OWNERDISPLAY, 1, 0, "Owner Display", (HANDLE32)NULL, 0, &ClipFormats[11], &ClipFormats[13] , (HANDLE16)NULL},
    { CF_DSPTEXT, 1, 0, "DSPText", (HANDLE32)NULL, 0, &ClipFormats[12], &ClipFormats[14] , (HANDLE16)NULL},
    { CF_DSPMETAFILEPICT, 1, 0, "DSPMetaFile Picture", (HANDLE32)NULL, 0, &ClipFormats[13], &ClipFormats[15] , (HANDLE16)NULL},
    { CF_DSPBITMAP, 1, 0, "DSPBitmap", (HANDLE32)NULL, 0, &ClipFormats[14], NULL , (HANDLE16)NULL}
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
 *		CLIPBOARD_GetDriver
 */
CLIPBOARD_DRIVER *CLIPBOARD_GetDriver()
{
#ifndef X_DISPLAY_MISSING
  return &X11DRV_CLIPBOARD_Driver;
#else /* X_DISPLAY_MISSING */
  return &TTYDRV_CLIPBOARD_Driver;
#endif /* X_DISPLAY_MISSING */
};

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
 *			CLIPBOARD_DeleteRecord
 */
void CLIPBOARD_DeleteRecord(LPCLIPFORMAT lpFormat, BOOL32 bChange)
{
    if( (lpFormat->wFormatID >= CF_GDIOBJFIRST &&
	 lpFormat->wFormatID <= CF_GDIOBJLAST) || lpFormat->wFormatID == CF_BITMAP )
    {
      if (lpFormat->hData32)
	DeleteObject32(lpFormat->hData32);
      if (lpFormat->hData16)
	DeleteObject16(lpFormat->hData16);
    }
    else if( lpFormat->wFormatID == CF_METAFILEPICT )
    {
      if (lpFormat->hData32)
      {
        DeleteMetaFile32( ((METAFILEPICT32 *)GlobalLock32( lpFormat->hData32 ))->hMF );
	GlobalFree32(lpFormat->hData32);
	if (lpFormat->hData16)
	  /* HMETAFILE16 and HMETAFILE32 are apparently the same thing, 
	     and a shallow copy is enough to share a METAFILEPICT
	     structure between 16bit and 32bit clipboards.  The MetaFile
	     should of course only be deleted once. */
	  GlobalFree16(lpFormat->hData16);
      }
      else if (lpFormat->hData16)
      {
	DeleteMetaFile16( ((METAFILEPICT16 *)GlobalLock16( lpFormat->hData16 ))->hMF );
	GlobalFree16(lpFormat->hData16);
      }
    }
    else 
    {
      if (lpFormat->hData32)
	GlobalFree32(lpFormat->hData32);
      if (lpFormat->hData16)
	GlobalFree16(lpFormat->hData16);
    }

    lpFormat->wDataPresent = 0; 
    lpFormat->hData16 = 0;
    lpFormat->hData32 = 0;

    if( bChange ) bCBHasChanged = TRUE;
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
 *            OpenClipboard32   (USER32.407)
 *
 * Note: Netscape uses NULL hWnd to open the clipboard.
 */
BOOL32 WINAPI OpenClipboard32( HWND32 hWnd )
{
    BOOL32 bRet;

    TRACE(clipboard,"(%04x)...\n", hWnd);

    if (!hqClipLock)
    {
	 hqClipLock = GetFastQueue();
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
 *            CloseClipboard32   (USER32.54)
 */
BOOL32 WINAPI CloseClipboard32(void)
{
    TRACE(clipboard,"!\n");

    if (hqClipLock == GetFastQueue())
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
 *            EmptyClipboard32   (USER32.169)
 */
BOOL32 WINAPI EmptyClipboard32(void)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"(void)\n");

    if (hqClipLock != GetFastQueue()) return FALSE;

    /* destroy private objects */

    if (hWndClipOwner)
	SendMessage16(hWndClipOwner, WM_DESTROYCLIPBOARD, 0, 0L);
  
    while(lpFormat) 
    {
	if ( lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32 )
	     CLIPBOARD_DeleteRecord( lpFormat, TRUE );

	lpFormat = lpFormat->NextFormat;
    }

    hWndClipOwner = hWndClipWindow;

    CLIPBOARD_GetDriver()->pEmptyClipboard();

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
 *            GetClipboardOwner32   (USER32.225)
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

    TRACE(clipboard, "(%04X, %04x) !\n", wFormat, hData);

    /* NOTE: If the hData is zero and current owner doesn't match
     * the window that opened the clipboard then this application
     * is screwed because WM_RENDERFORMAT will go to the owner
     * (to become the owner it must call EmptyClipboard() before
     *  adding new data).
     */

    if( (hqClipLock != GetFastQueue()) || !lpFormat ||
	(!hData && (!hWndClipOwner || (hWndClipOwner != hWndClipWindow))) ) return 0; 

    CLIPBOARD_GetDriver()->pSetClipboardData(wFormat);

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
 *            SetClipboardData32   (USER32.470)
 */
HANDLE32 WINAPI SetClipboardData32( UINT32 wFormat, HANDLE32 hData )
{
    LPCLIPFORMAT lpFormat = __lookup_format( ClipFormats, wFormat );

    TRACE(clipboard, "(%08X, %08x) !\n", wFormat, hData);

    /* NOTE: If the hData is zero and current owner doesn't match
     * the window that opened the clipboard then this application
     * is screwed because WM_RENDERFORMAT will go to the owner
     * (to become the owner it must call EmptyClipboard() before
     *  adding new data).
     */

    if( (hqClipLock != GetFastQueue()) || !lpFormat ||
	(!hData && (!hWndClipOwner || (hWndClipOwner != hWndClipWindow))) ) return 0; 

    CLIPBOARD_GetDriver()->pSetClipboardData(wFormat);

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
    lpFormat->hData32 = hData;          /* 0 is legal, see WM_RENDERFORMAT */
    lpFormat->hData16 = 0;

    return lpFormat->hData32;
}


/**************************************************************************
 *                      CLIPBOARD_RenderFormat
 */
static BOOL32 CLIPBOARD_RenderFormat(LPCLIPFORMAT lpFormat)
{
  if( lpFormat->wDataPresent && !lpFormat->hData16 && !lpFormat->hData32 )
  {
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
  }
  return (lpFormat->hData16 || lpFormat->hData32) ? TRUE : FALSE;
}

/**************************************************************************
 *                      CLIPBOARD_RenderText
 *
 * Convert text between UNIX and DOS formats.
 */
static BOOL32 CLIPBOARD_RenderText(LPCLIPFORMAT lpTarget, LPCLIPFORMAT lpSource)
{
    UINT16 size;
    LPCSTR lpstrS; 
    LPSTR  lpstrT;

    if (lpSource->hData32)
    {
      size = GlobalSize32( lpSource->hData32 );
      lpstrS = (LPSTR)GlobalLock32(lpSource->hData32);
    }
    else
    {
      size = GlobalSize16( lpSource->hData16 );
      lpstrS = (LPSTR)GlobalLock16(lpSource->hData16);
    }

    if( !lpstrS ) return FALSE;
    TRACE(clipboard,"\tconverting from '%s' to '%s', %i chars\n",
			   	      lpSource->Name, lpTarget->Name, size);

    lpTarget->hData32 = GlobalAlloc32(GMEM_ZEROINIT, size); 
    lpstrT = (LPSTR)GlobalLock32(lpTarget->hData32);

    if( lpstrT )
    {
	if( lpSource->wFormatID == CF_TEXT )
	    CharToOemBuff32A(lpstrS, lpstrT, size);
	else
	    OemToCharBuff32A(lpstrS, lpstrT, size);
	TRACE(clipboard,"\tgot %s\n", lpstrT);
	GlobalUnlock32(lpTarget->hData32);
	if (lpSource->hData32)
	  GlobalUnlock32(lpSource->hData32);
	else
	  GlobalUnlock16(lpSource->hData16);
	return TRUE;
    }

    lpTarget->hData32 = 0;
    if (lpSource->hData32)
      GlobalUnlock32(lpSource->hData32);
    else
      GlobalUnlock16(lpSource->hData16);
    return FALSE;
}

/**************************************************************************
 *             GetClipboardData16   (USER.142)
 */
HANDLE16 WINAPI GetClipboardData16( UINT16 wFormat )
{
    LPCLIPFORMAT lpRender = ClipFormats; 
    LPCLIPFORMAT lpUpdate = NULL;

    if (hqClipLock != GetFastQueue()) return 0;

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
    if( lpUpdate != lpRender && !lpUpdate->hData16 && !lpUpdate->hData32 ) 
	CLIPBOARD_RenderText(lpUpdate, lpRender);

    if( lpUpdate->hData32 && !lpUpdate->hData16 )
    {
      int size;
      if( lpUpdate->wFormatID == CF_METAFILEPICT )
	size = sizeof( METAFILEPICT16 );
      else
	size = GlobalSize32(lpUpdate->hData32);
      lpUpdate->hData16 = GlobalAlloc16(GMEM_ZEROINIT, size);
      if( !lpUpdate->hData16 )
	ERR(clipboard, "(%04X) -- not enough memory in 16b heap\n", wFormat);
      else
      {
      if( lpUpdate->wFormatID == CF_METAFILEPICT )
      {
	FIXME(clipboard,"\timplement function CopyMetaFilePict32to16\n");
	FIXME(clipboard,"\tin the appropriate file.\n");
#ifdef SOMEONE_IMPLEMENTED_ME
	CopyMetaFilePict32to16( GlobalLock16(lpUpdate->hData16), 
			        GlobalLock32(lpUpdate->hData32) );
#endif
      }
      else
      {
	memcpy( GlobalLock16(lpUpdate->hData16), 
		GlobalLock32(lpUpdate->hData32), 
		size );
      }
	GlobalUnlock16(lpUpdate->hData16);
	GlobalUnlock32(lpUpdate->hData32);
      }
    }

    TRACE(clipboard,"\treturning %04x (type %i)\n", 
			      lpUpdate->hData16, lpUpdate->wFormatID);
    return lpUpdate->hData16;
}


/**************************************************************************
 *             GetClipboardData32   (USER32.222)
 */
HANDLE32 WINAPI GetClipboardData32( UINT32 wFormat )
{
    LPCLIPFORMAT lpRender = ClipFormats; 
    LPCLIPFORMAT lpUpdate = NULL;

    if (hqClipLock != GetFastQueue()) return 0;

    TRACE(clipboard,"(%08X)\n", wFormat);

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
    if( lpUpdate != lpRender && !lpUpdate->hData16 && !lpUpdate->hData32 ) 
	CLIPBOARD_RenderText(lpUpdate, lpRender);

    if( lpUpdate->hData16 && !lpUpdate->hData32 )
    {
      int size;
      if( lpUpdate->wFormatID == CF_METAFILEPICT )
	size = sizeof( METAFILEPICT32 );
      else
	size = GlobalSize16(lpUpdate->hData16);
      lpUpdate->hData32 = GlobalAlloc32(GMEM_ZEROINIT, size); 
      if( lpUpdate->wFormatID == CF_METAFILEPICT )
      {
	FIXME(clipboard,"\timplement function CopyMetaFilePict16to32\n");
	FIXME(clipboard,"\tin the appropriate file.\n");
#ifdef SOMEONE_IMPLEMENTED_ME
	CopyMetaFilePict16to32( GlobalLock16(lpUpdate->hData32), 
			        GlobalLock32(lpUpdate->hData16) );
#endif
      }
      else
      {
	memcpy( GlobalLock32(lpUpdate->hData32), 
		GlobalLock16(lpUpdate->hData16), 
		size );
      }
      GlobalUnlock32(lpUpdate->hData32);
      GlobalUnlock16(lpUpdate->hData16);
    }

    TRACE(clipboard,"\treturning %04x (type %i)\n", 
			      lpUpdate->hData32, lpUpdate->wFormatID);
    return lpUpdate->hData32;
}

/**************************************************************************
 *           CountClipboardFormats16   (USER.143)
 */
INT16 WINAPI CountClipboardFormats16(void)
{
    return CountClipboardFormats32();
}


/**************************************************************************
 *           CountClipboardFormats32   (USER32.63)
 */
INT32 WINAPI CountClipboardFormats32(void)
{
    INT32 FormatCount = 0;
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"(void)\n");

    /* FIXME: Returns BOOL32 */
    CLIPBOARD_GetDriver()->pRequestSelection();

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
 *            EnumClipboardFormats32   (USER32.179)
 */
UINT32 WINAPI EnumClipboardFormats32( UINT32 wFormat )
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"(%04X)\n", wFormat);

    if( hqClipLock != GetFastQueue() ) return 0;

    if( (!wFormat || wFormat == CF_TEXT || wFormat == CF_OEMTEXT) ) 
        CLIPBOARD_GetDriver()->pRequestSelection();

    if (wFormat == 0)
    {
	if (lpFormat->wDataPresent || ClipFormats[CF_OEMTEXT-1].wDataPresent) 
	    return lpFormat->wFormatID;
	else 
	    wFormat = lpFormat->wFormatID; /* and CF_TEXT is not available */
    }

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
    lpNewFormat->hData16 = 0;
    lpNewFormat->hData32 = 0;
    lpNewFormat->BufSize = 0;
    lpNewFormat->PrevFormat = lpFormat;
    lpNewFormat->NextFormat = NULL;

    return LastRegFormat++;
}


/**************************************************************************
 *            RegisterClipboardFormat32A   (USER32.431)
 */
UINT32 WINAPI RegisterClipboardFormat32A( LPCSTR formatName )
{
    return RegisterClipboardFormat16( formatName );
}


/**************************************************************************
 *            RegisterClipboardFormat32W   (USER32.432)
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
 *            GetClipboardFormatName32A   (USER32.223)
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
 *            GetClipboardFormatName32W   (USER32.224)
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
 *            SetClipboardViewer32   (USER32.471)
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
 *           GetClipboardViewer32   (USER32.226)
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
 *           ChangeClipboardChain32   (USER32.22)
 */
BOOL32 WINAPI ChangeClipboardChain32(HWND32 hWnd, HWND32 hWndNext)
{
    BOOL32 bRet = 0;

    FIXME(clipboard, "(0x%04x, 0x%04x): stub?\n", hWnd, hWndNext);

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
 *           IsClipboardFormatAvailable32   (USER32.340)
 */
BOOL32 WINAPI IsClipboardFormatAvailable32( UINT32 wFormat )
{
    TRACE(clipboard,"(%04X) !\n", wFormat);

    if( (wFormat == CF_TEXT || wFormat == CF_OEMTEXT) )
        CLIPBOARD_GetDriver()->pRequestSelection();

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
 *             GetOpenClipboardWindow32   (USER32.277)
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
    FIXME(clipboard, "(%p,%d): stub\n", lpPriorityList, nCount );
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



