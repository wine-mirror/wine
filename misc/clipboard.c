/*
 * 'Wine' Clipboard function handling
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
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "windows.h"
#include "win.h"
#include "message.h"
#include "clipboard.h"
#include "xmalloc.h"
#include "stddebug.h"
#include "debug.h"

#define  CF_REGFORMATBASE 	0xC000

typedef struct tagCLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    WORD	wDataPresent;
    LPSTR	Name;
    HANDLE	hData;
    DWORD	BufSize;
    void	*PrevFormat;
    void	*NextFormat;
} CLIPFORMAT, *LPCLIPFORMAT;

/* *************************************************************************
 *			internal variables
 */

static HWND hWndClipOwner  = 0;		/* current clipboard owner */
static HWND hWndClipWindow = 0;		/* window that opened clipboard */
static HWND hWndViewer     = 0;		/* start of viewers chain */

static BOOL bClipChanged  = FALSE;
static WORD LastRegFormat = CF_REGFORMATBASE;

static Bool wait_for_selection = False;
static Bool wineOwnsSelection = False;

CLIPFORMAT ClipFormats[16]  = {
    { CF_TEXT, 1, 0, "Text", (HANDLE)NULL, 0, NULL, &ClipFormats[1] },
    { CF_BITMAP, 1, 0, "Bitmap", (HANDLE)NULL, 0, &ClipFormats[0], &ClipFormats[2] },
    { CF_METAFILEPICT, 1, 0, "MetaFile Picture", (HANDLE)NULL, 0, &ClipFormats[1], &ClipFormats[3] },
    { CF_SYLK, 1, 0, "Sylk", (HANDLE)NULL, 0, &ClipFormats[2], &ClipFormats[4] },
    { CF_DIF, 1, 0, "DIF", (HANDLE)NULL, 0, &ClipFormats[3], &ClipFormats[5] },
    { CF_TIFF, 1, 0, "TIFF", (HANDLE)NULL, 0, &ClipFormats[4], &ClipFormats[6] },
    { CF_OEMTEXT, 1, 0, "OEM Text", (HANDLE)NULL, 0, &ClipFormats[5], &ClipFormats[7] },
    { CF_DIB, 1, 0, "DIB", (HANDLE)NULL, 0, &ClipFormats[6], &ClipFormats[8] },
    { CF_PALETTE, 1, 0, "Palette", (HANDLE)NULL, 0, &ClipFormats[7], &ClipFormats[9] },
    { CF_PENDATA, 1, 0, "PenData", (HANDLE)NULL, 0, &ClipFormats[8], &ClipFormats[10] },
    { CF_RIFF, 1, 0, "RIFF", (HANDLE)NULL, 0, &ClipFormats[9], &ClipFormats[11] },
    { CF_WAVE, 1, 0, "Wave", (HANDLE)NULL, 0, &ClipFormats[10], &ClipFormats[12] },
    { CF_OWNERDISPLAY, 1, 0, "Owner Display", (HANDLE)NULL, 0, &ClipFormats[11], &ClipFormats[13] },
    { CF_DSPTEXT, 1, 0, "DSPText", (HANDLE)NULL, 0, &ClipFormats[12], &ClipFormats[14] },
    { CF_DSPMETAFILEPICT, 1, 0, "DSPMetaFile Picture", (HANDLE)NULL, 0, &ClipFormats[13], &ClipFormats[15] },
    { CF_DSPBITMAP, 1, 0, "DSPBitmap", (HANDLE)NULL, 0, &ClipFormats[14], NULL }
    };

/**************************************************************************
 *			CLIPBOARD_DisOwn
 */
void CLIPBOARD_DisOwn(HWND hWnd)
{
  LPCLIPFORMAT lpFormat = ClipFormats;

  if( hWnd != hWndClipOwner || !hWndClipOwner ) return;

  SendMessage16(hWndClipOwner,WM_RENDERALLFORMATS,0,0L);

  /* check if all formats were rendered */

  while(lpFormat)
    { 
       if( lpFormat->wDataPresent && !lpFormat->hData )
	 {
	   dprintf_clipboard(stddeb,"\tdata missing for clipboard format %i\n", lpFormat->wFormatID); 
	   lpFormat->wDataPresent = 0;
	 }
       lpFormat = lpFormat->NextFormat;
    }

  hWndClipOwner = 0;
}

/**************************************************************************
 *			CLIPBOARD_DeleteRecord
 */
void CLIPBOARD_DeleteRecord(LPCLIPFORMAT lpFormat)
{
  if( lpFormat->wFormatID >= CF_GDIOBJFIRST &&
      lpFormat->wFormatID <= CF_GDIOBJLAST )
      DeleteObject(lpFormat->hData);
  else if( lpFormat->hData )
           GlobalFree16(lpFormat->hData);

  lpFormat->wDataPresent = 0; 
  lpFormat->hData = 0;

  bClipChanged = TRUE;
}

/**************************************************************************
 *			CLIPBOARD_RequestXSelection
 */
BOOL CLIPBOARD_RequestXSelection()
{
  wait_for_selection=True;
  dprintf_clipboard(stddeb,"Requesting selection\n");

  XConvertSelection(display,XA_PRIMARY,XA_STRING,
                    XInternAtom(display,"PRIMARY_TEXT",False),
                    WIN_GetXWindow(hWndClipWindow),CurrentTime);

  /* TODO: need time-out for broken clients */
  while(wait_for_selection) EVENT_WaitXEvent(-1);

  return (BOOL)ClipFormats[0].wDataPresent;
}

/**************************************************************************
 *			CLIPBOARD_IsPresent
 */
BOOL CLIPBOARD_IsPresent(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    while(TRUE) {
        if (lpFormat == NULL) return FALSE;
        if (lpFormat->wFormatID == wFormat) break;
        lpFormat = lpFormat->NextFormat;
        }
    return (lpFormat->wDataPresent);
}

/**************************************************************************
 *			OpenClipboard		[USER.137]
 */
BOOL OpenClipboard(HWND hWnd)
{
    BOOL bRet = FALSE;
    dprintf_clipboard(stddeb,"OpenClipboard(%04x) = ", hWnd);

    if (!hWndClipWindow)
       {
    	 hWndClipWindow = hWnd;
	 bRet = TRUE;
       }
    bClipChanged = FALSE;

    dprintf_clipboard(stddeb,"%i\n", bRet);
    return bRet;
}


/**************************************************************************
 *			CloseClipboard		[USER.138]
 */
BOOL CloseClipboard()
{
    dprintf_clipboard(stddeb,"CloseClipboard(); !\n");

    if (hWndClipWindow == 0) return FALSE;
    hWndClipWindow = 0;

    if (bClipChanged && hWndViewer) SendMessage16(hWndViewer,WM_DRAWCLIPBOARD,0,0L);

    return TRUE;
}


/**************************************************************************
 *			EmptyClipboard		[USER.139]
 */
BOOL EmptyClipboard()
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    dprintf_clipboard(stddeb,"EmptyClipboard()\n");

    if (hWndClipWindow == 0) return FALSE;

    /* destroy private objects */

    if (hWndClipOwner)
	SendMessage16(hWndClipOwner,WM_DESTROYCLIPBOARD,0,0L);
  
    while(lpFormat) 
      {
	if ( lpFormat->wDataPresent )
	     CLIPBOARD_DeleteRecord( lpFormat );

	lpFormat = lpFormat->NextFormat;
      }

    hWndClipOwner = hWndClipWindow;

    if(wineOwnsSelection){
        dprintf_clipboard(stddeb,"Losing selection\n");
	wineOwnsSelection=False;
	XSetSelectionOwner(display,XA_PRIMARY,None,CurrentTime);
    }
    return TRUE;
}


/**************************************************************************
 *			GetClipboardOwner	[USER.140]
 */
HWND GetClipboardOwner()
{
    dprintf_clipboard(stddeb,
		"GetClipboardOwner() = %04x !\n", hWndClipOwner);
    return hWndClipOwner;
}


/**************************************************************************
 *			SetClipboardData	[USER.141]
 */
HANDLE SetClipboardData(WORD wFormat, HANDLE hData)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    Window       owner;

    dprintf_clipboard(stddeb,
		"SetClipboardDate(%04X, %04x) !\n", wFormat, hData);

    while(TRUE) 
      {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
      }

    /* Acquire X selection:
     *
     * doc says we shouldn't use CurrentTime 
     * should we become owner of CLIPBOARD as well? 
     */

    owner = WIN_GetXWindow(hWndClipWindow);

    XSetSelectionOwner(display,XA_PRIMARY,owner,CurrentTime);
    if( XGetSelectionOwner(display,XA_PRIMARY) == owner )
      {
        wineOwnsSelection = True;
        dprintf_clipboard(stddeb,"Getting selection\n");
      }

    if ( lpFormat->wDataPresent ) 
         CLIPBOARD_DeleteRecord(lpFormat);

    bClipChanged = TRUE;
    lpFormat->wDataPresent = TRUE;
    lpFormat->hData = hData;

    return lpFormat->hData;
}


/**************************************************************************
 *			GetClipboardData	[USER.142]
 */
HANDLE GetClipboardData(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    dprintf_clipboard(stddeb,"GetClipboardData(%04X)\n", wFormat);

    if (!hWndClipWindow) return 0;

    /*  if(wFormat == CF_TEXT && !wineOwnsSelection)
        CLIPBOARD_RequestXSelection(); 
     */

    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}

    if( lpFormat->wDataPresent && !lpFormat->hData )
      if( IsWindow(hWndClipOwner) )
	  SendMessage16(hWndClipOwner,WM_RENDERFORMAT,(WPARAM)lpFormat->wFormatID,0L);
      else
	  dprintf_clipboard(stddeb,"\thWndClipOwner is lost\n");
      
    return lpFormat->hData;
}


/**************************************************************************
 *			CountClipboardFormats	[USER.143]
 */
INT CountClipboardFormats()
{
    int 	 FormatCount = 0;
    LPCLIPFORMAT lpFormat = ClipFormats; 

    dprintf_clipboard(stddeb,"CountClipboardFormats()\n");

    while(TRUE) {
	if (lpFormat == NULL) break;
	if (lpFormat->wDataPresent) 
	    {
               dprintf_clipboard(stddeb, "\tdata found for format %i\n", lpFormat->wFormatID);

	       FormatCount++;
	    }
	lpFormat = lpFormat->NextFormat;
	}

    dprintf_clipboard(stddeb,"\ttotal %d\n", FormatCount);
    return FormatCount;
}


/**************************************************************************
 *			EnumClipboardFormats	[USER.144]
 */
UINT EnumClipboardFormats(UINT wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    dprintf_clipboard(stddeb,"EnumClipboardFormats(%04X)\n", wFormat);

    if( (!wFormat || wFormat == CF_TEXT) && !wineOwnsSelection)
        CLIPBOARD_RequestXSelection();

    if (wFormat == 0) {
	if (lpFormat->wDataPresent) 
	    return lpFormat->wFormatID;
	else 
	    wFormat = lpFormat->wFormatID;
	}

    /* walk up to the specified format record */

    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}

    /* find next format with available data */

    lpFormat = lpFormat->NextFormat;
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wDataPresent ) break;
	lpFormat = lpFormat->NextFormat;
	}

    dprintf_clipboard(stddeb, "\t got not empty - Id=%04X hData=%04x !\n",
				lpFormat->wFormatID, lpFormat->hData);
    return lpFormat->wFormatID;
}


/**************************************************************************
 *			RegisterClipboardFormat	[USER.145]
 */
WORD RegisterClipboardFormat(LPCSTR FormatName)
{
    LPCLIPFORMAT lpNewFormat; 
    LPCLIPFORMAT lpFormat = ClipFormats; 

    if (FormatName == NULL) return 0;

    dprintf_clipboard(stddeb,"RegisterClipboardFormat('%s') !\n", FormatName);

    /* walk format chain to see if it's already registered */

    while(TRUE) {
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
 *			GetClipboardFormatName	[USER.146]
 */
int GetClipboardFormatName(WORD wFormat, LPSTR retStr, short maxlen)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 

    dprintf_clipboard(stddeb,
	"GetClipboardFormat(%04X, %p, %d) !\n", wFormat, retStr, maxlen);

    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}

    if (lpFormat->Name == NULL || 
	lpFormat->wFormatID < CF_REGFORMATBASE) return 0;

    dprintf_clipboard(stddeb,
		"GetClipboardFormat // Name='%s' !\n", lpFormat->Name);

    strncpy(retStr, lpFormat->Name, maxlen - 1);
    retStr[maxlen] = 0;

    return strlen(retStr);
}


/**************************************************************************
 *			SetClipboardViewer	[USER.147]
 */
HWND SetClipboardViewer(HWND hWnd)
{
    HWND hwndPrev = hWndViewer;

    dprintf_clipboard(stddeb,"SetClipboardViewer(%04x)\n", hWnd);

    hWndViewer = hWnd;
    return hwndPrev;
}


/**************************************************************************
 *			GetClipboardViewer	[USER.148]
 */
HWND GetClipboardViewer()
{
    dprintf_clipboard(stddeb,"GetClipboardFormat() = %04x\n", hWndViewer);

    return hWndViewer;
}


/**************************************************************************
 *			ChangeClipboardChain	[USER.149]
 */
BOOL ChangeClipboardChain(HWND hWnd, HWND hWndNext)
{
    BOOL bRet = 0;

    dprintf_clipboard(stdnimp, "ChangeClipboardChain(%04x, %04x)\n", hWnd, hWndNext);

    if( hWndViewer )
      bRet = !SendMessage16( hWndViewer, WM_CHANGECBCHAIN, (WPARAM)hWnd, (LPARAM)hWndNext);   
    else
      dprintf_clipboard(stddeb,"ChangeClipboardChain: hWndViewer is lost\n");

    if( hWnd == hWndViewer ) hWndViewer = hWndNext;

    return 0;
}


/**************************************************************************
 *			IsClipboardFormatAvailable	[USER.193]
 */
BOOL IsClipboardFormatAvailable(WORD wFormat)
{
    dprintf_clipboard(stddeb,"IsClipboardFormatAvailable(%04X) !\n", wFormat);

    if(wFormat == CF_TEXT && !wineOwnsSelection)
	CLIPBOARD_RequestXSelection();

    return CLIPBOARD_IsPresent(wFormat);
}


/**************************************************************************
 *			GetOpenClipboardWindow	[USER.248]
 */
HWND GetOpenClipboardWindow()
{
    dprintf_clipboard(stddeb,
		"GetOpenClipboardWindow() = %04x\n", hWndClipWindow);
    return hWndClipWindow;
}


/**************************************************************************
 *			GetPriorityClipboardFormat	[USER.402]
 */
int GetPriorityClipboardFormat(WORD *lpPriorityList, short nCount)
{
    dprintf_clipboard(stdnimp,
	"GetPriorityClipboardFormat(%p, %d) !\n", lpPriorityList, nCount);

    return 0;
}


/**************************************************************************
 *			CLIPBOARD_ReadSelection
 *
 *	The current selection owner has set prop at our window w
 *	Transfer the property contents into the Clipboard
 */
void CLIPBOARD_ReadSelection(Window w,Atom prop)
{
    HANDLE hText;
    LPCLIPFORMAT lpFormat = ClipFormats; 
    if(prop==None)
        hText=0;
    else
      {
	Atom atype=None;
	int aformat;
	unsigned long nitems,remain;
	unsigned char *val=NULL;

        dprintf_clipboard(stddeb,"Received prop %s\n",XGetAtomName(display,prop));

        /* TODO: Properties longer than 64K */

	if(XGetWindowProperty(display,w,prop,0,0x3FFF,True,XA_STRING,
	    &atype, &aformat, &nitems, &remain, &val)!=Success)
		fprintf(stderr,"couldn't read property\n");

        dprintf_clipboard(stddeb,"Type %s,Format %d,nitems %ld,value %s\n",
		XGetAtomName(display,atype),aformat,nitems,val);

	if(atype!=XA_STRING || aformat!=8){
	    fprintf(stderr,"Property not set\n");
	    hText=0;
	} else {
	    dprintf_clipboard(stddeb,"Selection is %s\n",val);
	    hText=GlobalAlloc16(GMEM_MOVEABLE, nitems+1);
	    memcpy(GlobalLock16(hText),val,nitems+1);
	    GlobalUnlock16(hText);
	}
	XFree(val);
      }

    while(TRUE) {
	if (lpFormat == NULL) return;
	if (lpFormat->wFormatID == CF_TEXT) break;
	lpFormat = lpFormat->NextFormat;
	}

    if (lpFormat->wDataPresent) 
       CLIPBOARD_DeleteRecord(lpFormat);

    wait_for_selection=False;

    lpFormat->wDataPresent = TRUE;
    lpFormat->hData = hText;
    dprintf_clipboard(stddeb,"Received selection\n");
}

/**************************************************************************
 *			CLIPBOARD_ReleaseSelection
 *
 *	Wine lost the primary selection.
 *	Empty the clipboard, but don't set the current owner to None.
 *	Make sure current get/put attempts fail.
 */
void CLIPBOARD_ReleaseSelection(HWND hwnd)
{
    wineOwnsSelection=False;
    OpenClipboard(hwnd);
    EmptyClipboard();
    CloseClipboard();
}
