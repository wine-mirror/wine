/*
 * 'Wine' Clipboard function handling
 *
 * Copyright 1994 Martin Ayotte
 */

static char Copyright[] = "Copyright Martin Ayotte, 1994";

/*
#define DEBUG_CLIPBOARD
*/

#include <windows.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "prototypes.h"
#include "heap.h"
#include "win.h"

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
#ifdef DEBUG_CLIPBOARD
    printf("OpenClipboard(%04X); !\n", hWnd);
#endif
    return TRUE;
}


/**************************************************************************
 *			CloseClipboard		[USER.138]
 */
BOOL CloseClipboard()
{
    if (hWndClipboardOwner == 0) return FALSE;
    hWndClipboardOwner = 0;
#ifdef DEBUG_CLIPBOARD
    printf("CloseClipboard(); !\n");
#endif
    return TRUE;
}


/**************************************************************************
 *			EmptyClipboard		[USER.139]
 */
BOOL EmptyClipboard()
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
    if (hWndClipboardOwner == 0) return FALSE;
#ifdef DEBUG_CLIPBOARD
    printf("EmptyClipboard(); !\n");
#endif
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
#ifdef DEBUG_CLIPBOARD
    printf("GetClipboardOwner() = %04X !\n", hWndClipboardOwner);
#endif
    return hWndClipboardOwner;
}


/**************************************************************************
 *			SetClipboardData	[USER.141]
 */
HANDLE SetClipboardData(WORD wFormat, HANDLE hData)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
#ifdef DEBUG_CLIPBOARD
    printf("SetClipboardDate(%04X, %04X) !\n", wFormat, hData);
#endif
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
#ifdef DEBUG_CLIPBOARD
    printf("GetClipboardData(%04X) !\n", wFormat);
#endif
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
#ifdef DEBUG_CLIPBOARD
	    printf("CountClipboardFormats // Find Not Empty (%04X) !\n", 
					lpFormat->hData);
#endif
	    FormatCount++;
	    }
	lpFormat = lpFormat->NextFormat;
	}
#ifdef DEBUG_CLIPBOARD
    printf("CountClipboardFormats() = %d !\n", FormatCount);
#endif
    return FormatCount;
}


/**************************************************************************
 *			EnumClipboardFormats	[USER.144]
 */
WORD EnumClipboardFormats(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
#ifdef DEBUG_CLIPBOARD
    printf("EnumClipboardFormats(%04X) !\n", wFormat);
#endif
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
#ifdef DEBUG_CLIPBOARD
    printf("EnumClipboardFormats // Find Last (%04X) !\n", 
				lpFormat->wFormatID);
#endif
    lpFormat = lpFormat->NextFormat;
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->hData != 0) break;
	lpFormat = lpFormat->NextFormat;
	}
#ifdef DEBUG_CLIPBOARD
    printf("EnumClipboardFormats // Find Not Empty Id=%04X hData=%04X !\n",
				lpFormat->wFormatID, lpFormat->hData);
#endif
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
#ifdef DEBUG_CLIPBOARD
    printf("RegisterClipboardFormat('%s') !\n", FormatName);
#endif
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
#ifdef DEBUG_CLIPBOARD
    printf("GetClipboardFormat(%04X, %08X, %d) !\n", wFormat, retStr, maxlen);
#endif
    while(TRUE) {
	if (lpFormat == NULL) return 0;
	if (lpFormat->wFormatID == wFormat) break;
	lpFormat = lpFormat->NextFormat;
	}
    if (lpFormat->Name == NULL) return 0;
#ifdef DEBUG_CLIPBOARD
    printf("GetClipboardFormat // Name='%s' !\n", lpFormat->Name);
#endif
    maxlen = min(maxlen - 1, strlen(lpFormat->Name));
    printf("GetClipboardFormat // maxlen=%d !\n", maxlen);
    memcpy(retStr, lpFormat->Name, maxlen);
    retStr[maxlen] = 0;
    return maxlen;
}


/**************************************************************************
 *			SetClipboardViewer	[USER.147]
 */
HWND SetClipboardViewer(HWND hWnd)
{
#ifdef DEBUG_CLIPBOARD
    printf("SetClipboardFormat(%04X) !\n", hWnd);
#endif
    hWndViewer = hWnd;
}


/**************************************************************************
 *			GetClipboardViewer	[USER.148]
 */
HWND GetClipboardViewer()
{
#ifdef DEBUG_CLIPBOARD
    printf("GetClipboardFormat() = %04X !\n", hWndViewer);
#endif
}


/**************************************************************************
 *			ChangeClipboardChain	[USER.149]
 */
BOOL ChangeClipboardChain(HWND hWnd, HWND hWndNext)
{
#ifdef DEBUG_CLIPBOARD
    printf("ChangeClipboardChain(%04X, %04X) !\n", hWnd, hWndNext);
#endif
}


/**************************************************************************
 *			IsClipboardFormatAvailable	[USER.193]
 */
BOOL IsClipboardFormatAvailable(WORD wFormat)
{
    LPCLIPFORMAT lpFormat = ClipFormats; 
#ifdef DEBUG_CLIPBOARD
    printf("IsClipboardFormatAvailable(%04X) !\n", wFormat);
#endif
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
#ifdef DEBUG_CLIPBOARD
    printf("GetOpenClipboardWindow() = %04X !\n", hWndClipboardOwner);
#endif
    return hWndClipboardOwner;
}


/**************************************************************************
 *			GetPriorityClipboardFormat	[USER.402]
 */
int GetPriorityClipboardFormat(WORD FAR *lpPriorityList, short nCount)
{
#ifdef DEBUG_CLIPBOARD
    printf("GetPriorityClipboardFormat(%08X, %d) !\n", lpPriorityList, nCount);
#endif
}



