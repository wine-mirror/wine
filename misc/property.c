/*
 * 				Windows Properties Functions
 *
static char Copyright[] = "Copyright Martin Ayotte, 1994";
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "heap.h"
#include "win.h"
#include "callback.h"
#include "stddebug.h"
/* #define DEBUG_PROP */
/* #undef  DEBUG_PROP */
#include "debug.h"

typedef struct tagPROPENTRY {
	LPSTR		PropName;
	WORD		Atom;
	HANDLE		hData;
	struct tagPROPENTRY	*lpPrevProp;
	struct tagPROPENTRY	*lpNextProp;
} PROPENTRY;
typedef PROPENTRY *LPPROPENTRY;


/*************************************************************************
 *				RemoveProp			[USER.24]
 */
HANDLE RemoveProp(HWND hWnd, LPSTR lpStr)
{
    WND 		*wndPtr;
	LPPROPENTRY lpProp;
	HANDLE		hOldData;
	if (((DWORD)lpStr & 0xFFFF0000) == 0L)
		dprintf_prop(stddeb, "RemoveProp(%04X, Atom %04X)\n", hWnd, LOWORD((DWORD)lpStr));
	else
		dprintf_prop(stddeb, "RemoveProp(%04X, '%s')\n", hWnd, lpStr);
	wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) {
    	printf("RemoveProp // Bad Window handle !\n");
    	return FALSE;
    	}
	lpProp = (LPPROPENTRY) GlobalLock(wndPtr->hProp);
	if (lpProp == NULL) {
		dprintf_prop(stddeb, "Property List Empty !\n");
		return 0;
		}
	while (TRUE) {
		if ((((DWORD)lpStr & 0xFFFF0000) == 0L && 
			lpProp->Atom == LOWORD((DWORD)lpStr)) ||
			(((DWORD)lpStr & 0xFFFF0000) != 0L && 
			lpProp->PropName != NULL &&
			strcmp(lpProp->PropName, lpStr) == 0)) {
		   	dprintf_prop(stddeb, "RemoveProp // Property found ! hData=%04X\n", lpProp->hData);
			hOldData = lpProp->hData;
			if (lpProp->lpPrevProp != NULL) 
				lpProp->lpPrevProp->lpNextProp = lpProp->lpNextProp;
			if (lpProp->lpNextProp != NULL) 
				lpProp->lpNextProp->lpPrevProp = lpProp->lpPrevProp;
			if (lpProp->PropName != NULL) free(lpProp->PropName);
			GlobalFree(lpProp);
			GlobalUnlock(wndPtr->hProp);
			return hOldData;
			}
		if (lpProp->lpNextProp == NULL) break;
		lpProp = lpProp->lpNextProp;
		}
	GlobalUnlock(wndPtr->hProp);
   	dprintf_prop(stddeb, "RemoveProp // Property not found !\n");
	return 0;
}


/*************************************************************************
 *				GetProp			[USER.25]
 */
HANDLE GetProp(HWND hWnd, LPSTR lpStr)
{
    WND 		*wndPtr;
	LPPROPENTRY lpProp;
	if (((DWORD)lpStr & 0xFFFF0000) == 0L)
		dprintf_prop(stddeb, "GetProp(%04X, Atom %04X)\n", hWnd, LOWORD((DWORD)lpStr));
	else
		dprintf_prop(stddeb, "GetProp(%04X, '%s')\n", hWnd, lpStr);
	wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) {
    	printf("GetProp // Bad Window handle !\n");
    	return 0;
    	}
	lpProp = (LPPROPENTRY) GlobalLock(wndPtr->hProp);
	if (lpProp == NULL) {
		dprintf_prop(stddeb, "Property List Empty !\n");
		return 0;
		}
	while (TRUE) {
		if ((((DWORD)lpStr & 0xFFFF0000) == 0L && 
			lpProp->Atom == LOWORD((DWORD)lpStr)) ||
			(((DWORD)lpStr & 0xFFFF0000) != 0L && 
			lpProp->PropName != NULL &&
			strcmp(lpProp->PropName, lpStr) == 0)) {
		   	dprintf_prop(stddeb, "GetProp // Property found ! hData=%04X\n", lpProp->hData);
			GlobalUnlock(wndPtr->hProp);
			return lpProp->hData;
			}
		if (lpProp->lpNextProp == NULL) break;
		lpProp = lpProp->lpNextProp;
		}
   	dprintf_prop(stddeb, "GetProp // Property not found !\n");
	GlobalUnlock(wndPtr->hProp);
	return 0;
}


/*************************************************************************
 *				SetProp			[USER.26]
 */
BOOL SetProp(HWND hWnd, LPSTR lpStr, HANDLE hData)
{
    WND 		*wndPtr;
    HANDLE 		hNewProp;
	LPPROPENTRY lpNewProp;
	LPPROPENTRY lpProp;
	if (((DWORD)lpStr & 0xFFFF0000) == 0L)
		dprintf_prop(stddeb, "SetProp(%04X, Atom %04X, %04X)\n", 
				hWnd, LOWORD((DWORD)lpStr), hData);
	else
		dprintf_prop(stddeb, "SetProp(%04X, '%s', %04X)\n", hWnd, lpStr, hData);
	wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) {
    	printf("SetProp // Bad Window handle !\n");
    	return FALSE;
    	}
	lpProp = (LPPROPENTRY) GlobalLock(wndPtr->hProp);
	if (lpProp != NULL) {
		while (TRUE) {
			if ((((DWORD)lpStr & 0xFFFF0000) == 0L && 
				lpProp->Atom == LOWORD((DWORD)lpStr)) ||
				(((DWORD)lpStr & 0xFFFF0000) != 0L && 
				lpProp->PropName != NULL &&
				strcmp(lpProp->PropName, lpStr) == 0)) {
			    dprintf_prop(stddeb, "SetProp // change already exinsting property !\n");
				lpProp->hData = hData;
				GlobalUnlock(wndPtr->hProp);
				return TRUE;
				}
			if (lpProp->lpNextProp == NULL) break;
			lpProp = lpProp->lpNextProp;
			}
		}
	hNewProp = GlobalAlloc(GMEM_MOVEABLE, sizeof(PROPENTRY));
	lpNewProp = (LPPROPENTRY) GlobalLock(hNewProp);
	if (lpNewProp == NULL) {
    	printf("SetProp // Can't allocate Property entry !\n");
		GlobalUnlock(wndPtr->hProp);
    	return FALSE;
		}
	dprintf_prop(stddeb, "SetProp // entry allocated %08X\n", 
    		(unsigned int) lpNewProp);
	if (lpProp == NULL) {
		wndPtr->hProp = hNewProp;
		lpNewProp->lpPrevProp = NULL;
	    dprintf_prop(stddeb, "SetProp // first entry \n");
		}
	else {
		lpProp->lpNextProp = lpNewProp;
		lpNewProp->lpPrevProp = lpProp;
		}
	lpNewProp->lpNextProp = NULL;
	lpNewProp->hData = hData;
	if (((DWORD)lpStr & 0xFFFF0000) == 0L) {
		lpNewProp->PropName = NULL;
		lpNewProp->Atom = LOWORD((DWORD)lpStr);
		}
	else {
		lpNewProp->Atom = 0;
		lpNewProp->PropName = malloc(strlen(lpStr) + 1);
		if (lpNewProp->PropName == NULL) {
	    	printf("SetProp // Can't allocate memory for Property Name !\n");
			GlobalUnlock(wndPtr->hProp);
    		return FALSE;
			}
		strcpy(lpNewProp->PropName, lpStr);
		}
	GlobalUnlock(hNewProp);
	GlobalUnlock(wndPtr->hProp);
	return TRUE;
}


/*************************************************************************
 *				EnumProps			[USER.27]
 */
int EnumProps(HWND hWnd, FARPROC lpEnumFunc)
{
    WND 		*wndPtr;
	LPPROPENTRY lpProp;
	LPSTR		str;
	int			nRet;
	printf("EnumProps(%04X, %08X)\n", hWnd, (unsigned int) lpEnumFunc);
	wndPtr = WIN_FindWndPtr(hWnd);
    if (wndPtr == NULL) {
    	printf("EnumProps // Bad Window handle !\n");
    	return 0;
    	}
	lpProp = (LPPROPENTRY) GlobalLock(wndPtr->hProp);
	if (lpProp == NULL) {
		printf("Property List Empty !\n");
		return 0;
		}
	if (lpEnumFunc == NULL)	return 0;
	while (TRUE) {
    	printf("EnumProps // lpProp->Atom=%04X !\n", lpProp->Atom);
		str = (LPSTR)MAKELONG(lpProp->Atom, 0); 
		if (lpProp->PropName != NULL) {
	    	printf("EnumProps // lpProp->PropName='%s' !\n", lpProp->PropName);
			str = lpProp->PropName; 
			}
#ifdef WINELIB
		nRet = (*lpEnumFunc)((HWND)hWnd, (WORD)0, 
			(LPSTR)str, (HANDLE)lpProp->hData);
#else
		nRet = CallBack16(lpEnumFunc, 3,
		    CALLBACK_SIZE_WORD, (HWND)hWnd,
		    CALLBACK_SIZE_LONG, (LPSTR)str,
		    CALLBACK_SIZE_WORD, (HANDLE)lpProp->hData);
#endif
		if (nRet == 0) break;
		if (lpProp->lpNextProp == NULL) break;
		lpProp = lpProp->lpNextProp;
		}
	GlobalUnlock(wndPtr->hProp);
	return 0;
}


