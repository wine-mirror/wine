/*
*     Windows Exec & Help
*
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "neexe.h"
#include "dlls.h"
#include "shell.h"
#include "windows.h"
#include "callback.h"
#include "stddebug.h"
#include "debug.h"
#include "win.h"

#define HELP_CONTEXT      0x0001
#define HELP_QUIT         0x0002
#define HELP_INDEX        0x0003
#define HELP_CONTENTS     0x0003
#define HELP_HELPONHELP   0x0004
#define HELP_SETINDEX     0x0005
#define HELP_SETCONTENTS  0x0005
#define HELP_CONTEXTPOPUP 0x0008
#define HELP_FORCEFILE    0x0009
#define HELP_KEY          0x0101
#define HELP_COMMAND      0x0102
#define HELP_PARTIALKEY   0x0105
#define HELP_MULTIKEY     0x0201
#define HELP_SETWINPOS    0x0203


/***********************************************************************
 *           EXEC_ExitWindows
 *
 * Clean-up everything and exit the Wine process.
 * This is the back-end of ExitWindows(), called when all windows
 * have agreed to be terminated.
 */
void EXEC_ExitWindows( int retCode )
{
    /* Do the clean-up stuff */

    WriteOutProfiles();
    SHELL_SaveRegistry();

    exit( retCode );
}


/***********************************************************************
 *           ExitWindows   (USER.7)
 */
BOOL ExitWindows( DWORD dwReturnCode, WORD wReserved )
{
    HWND hwnd, hwndDesktop;
    WND *wndPtr;
    HWND *list, *pWnd;
    int count, i;
    BOOL result;
        
    api_assert("ExitWindows", wReserved == 0);
    api_assert("ExitWindows", HIWORD(dwReturnCode) == 0);

    /* We have to build a list of all windows first, as in EnumWindows */

    /* First count the windows */

    hwndDesktop = GetDesktopWindow();
    count = 0;
    for (hwnd = GetTopWindow(hwndDesktop); hwnd != 0; hwnd = wndPtr->hwndNext)
    {
        if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return FALSE;
        count++;
    }
    if (!count) /* No windows, we can exit at once */
        EXEC_ExitWindows( LOWORD(dwReturnCode) );

      /* Now build the list of all windows */

    if (!(list = (HWND *)malloc( sizeof(HWND) * count ))) return FALSE;
    for (hwnd = GetTopWindow(hwndDesktop), pWnd = list; hwnd != 0; hwnd = wndPtr->hwndNext)
    {
        wndPtr = WIN_FindWndPtr( hwnd );
        *pWnd++ = hwnd;
    }

      /* Now send a WM_QUERYENDSESSION message to every window */

    for (pWnd = list, i = 0; i < count; i++, pWnd++)
    {
          /* Make sure that window still exists */
        if (!IsWindow(*pWnd)) continue;
	if (!SendMessage( *pWnd, WM_QUERYENDSESSION, 0, 0 )) break;
    }
    result = (i == count);

    /* Now notify all windows that got a WM_QUERYENDSESSION of the result */

    for (pWnd = list; i > 0; i--, pWnd++)
    {
	if (!IsWindow(*pWnd)) continue;
	SendMessage( *pWnd, WM_ENDSESSION, result, 0 );
    }
    free( list );

    if (result) EXEC_ExitWindows( LOWORD(dwReturnCode) );
    return FALSE;
}


/**********************************************************************
 *				WinHelp		[USER.171]
 */
BOOL WinHelp(HWND hWnd, LPSTR lpHelpFile, WORD wCommand, DWORD dwData)
{
	static WORD WM_WINHELP=0;
	HWND hDest;
	char szBuf[20];
	LPWINHELP lpwh;
	HANDLE hwh;
	void *data=0;
	int size,dsize,nlen;
        if (wCommand != HELP_QUIT)  /* FIXME */
            if(WinExec("winhelp.exe -x",SW_SHOWNORMAL)<=32)
		return FALSE;
	/* FIXME: Should be directed yield, to let winhelp open the window */
	Yield();
	if(!WM_WINHELP) {
		strcpy(szBuf,"WM_WINHELP");
		WM_WINHELP=RegisterWindowMessage(MAKE_SEGPTR(szBuf));
		if(!WM_WINHELP)
			return FALSE;
	}
	strcpy(szBuf,"MS_WINHELP");
	hDest = FindWindow(MAKE_SEGPTR(szBuf),0);
	if(!hDest)
		return FALSE;
	switch(wCommand)
	{
		case HELP_CONTEXT:
		case HELP_CONTENTS:
		case HELP_SETCONTENTS:
		case HELP_CONTEXTPOPUP:
		case HELP_FORCEFILE:
		case HELP_HELPONHELP:
		case HELP_QUIT:
			dsize=0;
			break;
		case HELP_KEY:
		case HELP_PARTIALKEY:
		case HELP_COMMAND:
			data = PTR_SEG_TO_LIN(dwData);
			dsize = strlen(data)+1;
			break;
		case HELP_MULTIKEY:
			data = PTR_SEG_TO_LIN(dwData);
			dsize = ((LPMULTIKEYHELP)data) -> mkSize;
			break;
		case HELP_SETWINPOS:
			data = PTR_SEG_TO_LIN(dwData);
			dsize = ((LPHELPWININFO)data) -> wStructSize;
			break;
		default:
			fprintf(stderr,"Unknown help command %d\n",wCommand);
			return FALSE;
	}
	if(lpHelpFile)
		nlen =  strlen(lpHelpFile)+1;
	else
		nlen = 0;
	size = sizeof(WINHELP) + nlen + dsize;
	hwh = GlobalAlloc(0,size);
	lpwh = GlobalLock(hwh);
	lpwh->size = size;
	lpwh->command = wCommand;
	if(nlen) {
		lpwh->ofsFilename = sizeof(WINHELP);
		strcpy(((char*)lpwh) + sizeof(WINHELP),lpHelpFile);
	}
	if(dsize) {
		memcpy(((char*)lpwh)+sizeof(WINHELP)+nlen,data,dsize);
		lpwh->ofsData = sizeof(WINHELP)+nlen;
	} else
		lpwh->ofsData = 0;
	GlobalUnlock(hwh);
	return SendMessage(hDest,WM_WINHELP,hWnd,hwh);
}
