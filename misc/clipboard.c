/*
 * 'Wine' Clipboard function handling
 *
 * Copyright 1994 Martin Ayotte
 */

static char Copyright[] = "Copyright Martin Ayotte, 1994";

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "heap.h"
#include "win.h"
#include "stddebug.h"
/* #define DEBUG_CLIPBOARD /* */
/* #undef  DEBUG_CLIPBOARD /* */
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
	"GetClipboardFormat(%04X, %08X, %d) !\n", wFormat, retStr, maxlen);
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
    dprintf_clipboard(stddeb,"SetClipboardFormat(%04X) !\n", hWnd);
    hWndViewer = hWnd;
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
}


/**************************************************************************
 *			IsClipboardFormatAvailable	[USER.193]
 */
BOOL IsClipboardFormatAvailable(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    dprintf_clipboard(stddeb,"IsClipboardFormatAvailable(%04X) !\n", wFormat);
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
	"GetPriorityClipboardFormat(%08X, %d) !\n", lpPriorityList, nCount);
}



