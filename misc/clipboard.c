/*
 * 'Wine' Clipboard function handling
 *
 * Copyright 1994 Martin Ayotte
static char Copyright[] = "Copyright Martin Ayotte, 1994";
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include "win.h"
#include "message.h"
#include "clipboard.h"
#include "stddebug.h"
#include "debug.h"

typedef struct tagCLIPFORMAT {
    WORD	wFormatID;
    WORD	wRefCount;
    LPSTR	Name;
    HANDLE	hData;
    DWORD	BufSize;
    void	*PrevFormat;
    void	*NextFormat;
} CLIPFORMAT;
typedef CLIPFORMAT FAR* LPCLIPFORMAT;

static HWND hWndClipboardOwner = 0;
static HWND hWndViewer = 0;
static WORD LastRegFormat = 0xC000;
static Bool wait_for_selection = False;
static Bool wineOwnsSelection = False;

CLIPFORMAT ClipFormats[12]  = {
    { CF_TEXT, 1, "Text", (HANDLE)NULL, 0, NULL, &ClipFormats[1] },
    { CF_BITMAP, 1, "Bitmap", (HANDLE)NULL, 0, &ClipFormats[0], &ClipFormats[2] },
    { CF_METAFILEPICT, 1, "MetaFile Picture", (HANDLE)NULL, 0, &ClipFormats[1], &ClipFormats[3] },
    { CF_SYLK, 1, "Sylk", (HANDLE)NULL, 0, &ClipFormats[2], &ClipFormats[4] },
    { CF_DIF, 1, "DIF", (HANDLE)NULL, 0, &ClipFormats[3], &ClipFormats[5] },
    { CF_TIFF, 1, "TIFF", (HANDLE)NULL, 0, &ClipFormats[4], &ClipFormats[6] },
    { CF_OEMTEXT, 1, "OEM Text", (HANDLE)NULL, 0, &ClipFormats[5], &ClipFormats[7] },
    { CF_DIB, 1, "DIB", (HANDLE)NULL, 0, &ClipFormats[6], &ClipFormats[8] },
    { CF_PALETTE, 1, "Palette", (HANDLE)NULL, 0, &ClipFormats[7], &ClipFormats[9] },
    { CF_PENDATA, 1, "PenData", (HANDLE)NULL, 0, &ClipFormats[8], &ClipFormats[10] },
    { CF_RIFF, 1, "RIFF", (HANDLE)NULL, 0, &ClipFormats[9], &ClipFormats[11] },
    { CF_WAVE, 1, "Wave", (HANDLE)NULL, 0, &ClipFormats[10], NULL }
    };

/**************************************************************************
 *			OpenClipboard		[USER.137]
 */
BOOL OpenClipboard(HWND hWnd)
{
    if (hWndClipboardOwner != 0) return FALSE;
    hWndClipboardOwner = hWnd;
    dprintf_clipboard(stddeb,"OpenClipboard(%04X); !\n", hWnd);
    return TRUE;
}


/**************************************************************************
 *			CloseClipboard		[USER.138]
 */
BOOL CloseClipboard()
{
    if (hWndClipboardOwner == 0) return FALSE;
    hWndClipboardOwner = 0;
    dprintf_clipboard(stddeb,"CloseClipboard(); !\n");
    return TRUE;
}


/**************************************************************************
 *			EmptyClipboard		[USER.139]
 */
BOOL EmptyClipboard()
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    if (hWndClipboardOwner == 0) return FALSE;
    dprintf_clipboard(stddeb,"EmptyClipboard(); !\n");
    while(TRUE) {
	if (lpFormat == NULL) break;
	if (lpFormat->hData != 0) {
	    GlobalFree(lpFormat->hData);
	    lpFormat->hData = 0;
	    }
	lpFormat = lpFormat->NextFormat;
	}
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
		"GetClipboardOwner() = %04X !\n", hWndClipboardOwner);
    return hWndClipboardOwner;
}


/**************************************************************************
 *			SetClipboardData	[USER.141]
 */
HANDLE SetClipboardData(WORD wFormat, HANDLE hData)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    dprintf_clipboard(stddeb,
		"SetClipboardDate(%04X, %04X) !\n", wFormat, hData);
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}
    /* doc says we shouldn't use CurrentTime */
    /* should we become owner of CLIPBOARD as well? */
    XSetSelectionOwner(display,XA_PRIMARY,WIN_GetXWindow(hWndClipboardOwner),CurrentTime);
    wineOwnsSelection = True;
    dprintf_clipboard(stddeb,"Getting selection\n");
    if (lpFormat->hData != 0) GlobalFree(lpFormat->hData);
    lpFormat->hData = hData;
    return lpFormat->hData;
}


/**************************************************************************
 *			GetClipboardData	[USER.142]
 */
HANDLE GetClipboardData(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    dprintf_clipboard(stddeb,"GetClipboardData(%04X) !\n", wFormat);
    if(wFormat == CF_TEXT && !wineOwnsSelection)
    {	wait_for_selection=True;
        dprintf_clipboard(stddeb,"Requesting selection\n");
	XConvertSelection(display,XA_PRIMARY,XA_STRING,
		XInternAtom(display,"PRIMARY_TEXT",False),
		WIN_GetXWindow(hWndClipboardOwner),CurrentTime);
	/* TODO: need time-out for broken clients */
	while(wait_for_selection)MSG_WaitXEvent(-1);
    }
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}
    return lpFormat->hData;
}


/**************************************************************************
 *			CountClipboardFormats	[USER.143]
 */
int CountClipboardFormats()
{
    int FormatCount = 0;
    LPCLIPFORMAT lpFormat = ClipFormats; 
    while(TRUE) {
	if (lpFormat == NULL) break;
	if (lpFormat->hData != 0) {
        dprintf_clipboard(stddeb,
		"CountClipboardFormats // Find Not Empty (%04X) !\n",
					lpFormat->hData);
	    FormatCount++;
	    }
	lpFormat = lpFormat->NextFormat;
	}
    dprintf_clipboard(stddeb,"CountClipboardFormats() = %d !\n", FormatCount);
    return FormatCount;
}


/**************************************************************************
 *			EnumClipboardFormats	[USER.144]
 */
WORD EnumClipboardFormats(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    dprintf_clipboard(stddeb,"EnumClipboardFormats(%04X) !\n", wFormat);
    if (wFormat == 0) {
	if (lpFormat->hData != 0) 
	    return lpFormat->wFormatID;
	else 
	    wFormat = lpFormat->wFormatID;
	}
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}
    dprintf_clipboard(stddeb,"EnumClipboardFormats // Find Last (%04X) !\n",
				lpFormat->wFormatID);
    lpFormat = lpFormat->NextFormat;
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->hData != 0) break;
	lpFormat = lpFormat->NextFormat;
	}
    dprintf_clipboard(stddeb,
		"EnumClipboardFormats // Find Not Empty Id=%04X hData=%04X !\n",
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
    while(TRUE) {
	if (lpFormat->NextFormat == NULL) break;
	lpFormat = lpFormat->NextFormat;
	}
    lpNewFormat = (LPCLIPFORMAT)malloc(sizeof(CLIPFORMAT));
    if (lpNewFormat == NULL) return 0;
    lpFormat->NextFormat = lpNewFormat;
    dprintf_clipboard(stddeb,"RegisterClipboardFormat('%s') !\n", FormatName);
    lpNewFormat->wFormatID = LastRegFormat;
    lpNewFormat->wRefCount = 1;
    lpNewFormat->Name = (LPSTR)malloc(strlen(FormatName) + 1);
    if (lpNewFormat->Name == NULL) {
	free(lpNewFormat);
    	return 0;
    	}
    strcpy(lpNewFormat->Name, FormatName);
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
    if (lpFormat->Name == NULL) return 0;
    dprintf_clipboard(stddeb,
		"GetClipboardFormat // Name='%s' !\n", lpFormat->Name);
    maxlen = min(maxlen - 1, strlen(lpFormat->Name));
    dprintf_clipboard(stddeb,"GetClipboardFormat // maxlen=%d !\n", maxlen);
    memcpy(retStr, lpFormat->Name, maxlen);
    retStr[maxlen] = 0;
    return maxlen;
}


/**************************************************************************
 *			SetClipboardViewer	[USER.147]
 */
HWND SetClipboardViewer(HWND hWnd)
{
    HWND hwndPrev = hWndViewer;
    dprintf_clipboard(stddeb,"SetClipboardViewer(%04X) !\n", hWnd);
    hWndViewer = hWnd;
    return hwndPrev;
}


/**************************************************************************
 *			GetClipboardViewer	[USER.148]
 */
HWND GetClipboardViewer()
{
    dprintf_clipboard(stddeb,"GetClipboardFormat() = %04X !\n", hWndViewer);
    return hWndViewer;
}


/**************************************************************************
 *			ChangeClipboardChain	[USER.149]
 */
BOOL ChangeClipboardChain(HWND hWnd, HWND hWndNext)
{
    dprintf_clipboard(stdnimp,
		"ChangeClipboardChain(%04X, %04X) !\n", hWnd, hWndNext);

     return 0;
}


/**************************************************************************
 *			IsClipboardFormatAvailable	[USER.193]
 */
BOOL IsClipboardFormatAvailable(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    dprintf_clipboard(stddeb,"IsClipboardFormatAvailable(%04X) !\n", wFormat);
    if(wFormat == CF_TEXT && !wineOwnsSelection) /* obtain selection as text if possible */
	return GetClipboardData(CF_TEXT)!=0;
    while(TRUE) {
	if (lpFormat == NULL) return FALSE;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}
    return (lpFormat->hData != 0);
}


/**************************************************************************
 *			GetOpenClipboardWindow	[USER.248]
 */
HWND GetOpenClipboardWindow()
{
    dprintf_clipboard(stddeb,
		"GetOpenClipboardWindow() = %04X !\n", hWndClipboardOwner);
    return hWndClipboardOwner;
}


/**************************************************************************
 *			GetPriorityClipboardFormat	[USER.402]
 */
int GetPriorityClipboardFormat(WORD FAR *lpPriorityList, short nCount)
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
    else{
	Atom atype=None;
	int aformat;
	unsigned long nitems,remain;
	unsigned char *val=NULL;
        dprintf_clipboard(stddeb,"Received prop %s\n",XGetAtomName(display,prop));
        /* TODO: Properties longer than 64K */
	if(XGetWindowProperty(display,w,prop,0,0x3FFF,True,XA_STRING,
	    &atype, &aformat, &nitems, &remain, &val)!=Success)
		printf("couldn't read property\n");
        dprintf_clipboard(stddeb,"Type %s,Format %d,nitems %ld,value %s\n",
		XGetAtomName(display,atype),aformat,nitems,val);
	if(atype!=XA_STRING || aformat!=8){
	    fprintf(stderr,"Property not set\n");
	    hText=0;
	} else {
	    dprintf_clipboard(stddeb,"Selection is %s\n",val);
	    hText=GlobalAlloc(GMEM_MOVEABLE, nitems);
	    memcpy(GlobalLock(hText),val,nitems+1);
	    GlobalUnlock(hText);
	}
	XFree(val);
    }
    while(TRUE) {
	if (lpFormat == NULL) return;
	if (lpFormat->wFormatID == CF_TEXT) break;
	lpFormat = lpFormat->NextFormat;
	}
    if (lpFormat->hData != 0) GlobalFree(lpFormat->hData);
    wait_for_selection=False;
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
