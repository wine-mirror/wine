/*
 *     Windows Exec & Help
 */

#define NO_TRANSITION_TYPES  /* This file is Win32-clean */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "windows.h"
#include "heap.h"
#include "neexe.h"
#include "shell.h"
#include "stddebug.h"
#include "debug.h"
#include "win.h"


/***********************************************************************
 *           EXEC_ExitWindows
 *
 * Clean-up everything and exit the Wine process.
 * This is the back-end of ExitWindows(), called when all windows
 * have agreed to be terminated.
 */
void EXEC_ExitWindows(void)
{
    /* Do the clean-up stuff */

    WriteOutProfiles();
    SHELL_SaveRegistry();

    exit(0);
}


/***********************************************************************
 *           ExitWindows16   (USER.7)
 */
BOOL16 ExitWindows16( DWORD dwReturnCode, UINT16 wReserved )
{
    return ExitWindowsEx( EWX_LOGOFF, 0xffffffff );
}


/***********************************************************************
 *           ExitWindowsEx   (USER32.195)
 */
BOOL32 ExitWindowsEx( UINT32 flags, DWORD reserved )
{
    int i;
    BOOL32 result;
    WND **list, **ppWnd;
        
    /* We have to build a list of all windows first, as in EnumWindows */

    if (!(list = WIN_BuildWinArray( WIN_GetDesktop() ))) return FALSE;

    /* Send a WM_QUERYENDSESSION message to every window */

    for (ppWnd = list, i = 0; *ppWnd; ppWnd++, i++)
    {
        /* Make sure that the window still exists */
        if (!IsWindow( (*ppWnd)->hwndSelf )) continue;
	if (!SendMessage16( (*ppWnd)->hwndSelf, WM_QUERYENDSESSION, 0, 0 ))
            break;
    }
    result = !(*ppWnd);

    /* Now notify all windows that got a WM_QUERYENDSESSION of the result */

    for (ppWnd = list; i > 0; i--, ppWnd++)
    {
        if (!IsWindow( (*ppWnd)->hwndSelf )) continue;
	SendMessage16( (*ppWnd)->hwndSelf, WM_ENDSESSION, result, 0 );
    }
    HeapFree( SystemHeap, 0, list );

    if (result) EXEC_ExitWindows();
    return FALSE;
}


/**********************************************************************
 *             WinHelp16   (USER.171)
 */
BOOL16 WinHelp16( HWND16 hWnd, LPCSTR lpHelpFile, UINT16 wCommand,
                  DWORD dwData )
{
    return WinHelp32A( hWnd, lpHelpFile, wCommand,
                       (DWORD)PTR_SEG_TO_LIN(dwData) );
}


/**********************************************************************
 *             WinHelp32A   (USER32.578)
 */
BOOL32 WinHelp32A( HWND32 hWnd, LPCSTR lpHelpFile, UINT32 wCommand,
                   DWORD dwData )
{
	static WORD WM_WINHELP=0;
	HWND32 hDest;
	LPWINHELP lpwh;
	HGLOBAL16 hwh;
	int size,dsize,nlen;
        if (wCommand != HELP_QUIT)  /* FIXME */
            if(WinExec32("winhelp.exe -x",SW_SHOWNORMAL)<=32)
		return FALSE;
	/* FIXME: Should be directed yield, to let winhelp open the window */
	Yield();
	if(!WM_WINHELP) {
		WM_WINHELP=RegisterWindowMessage32A("WM_WINHELP");
		if(!WM_WINHELP)
			return FALSE;
	}
	hDest = FindWindow32A( "MS_WINHELP", NULL );
	if(!hDest)
		if(wCommand == HELP_QUIT)
			return TRUE;
		else
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
			dsize = strlen( (LPSTR)dwData )+1;
			break;
		case HELP_MULTIKEY:
			dsize = ((LPMULTIKEYHELP)dwData)->mkSize;
			break;
		case HELP_SETWINPOS:
			dsize = ((LPHELPWININFO)dwData)->wStructSize;
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
	hwh = GlobalAlloc16(0,size);
	lpwh = GlobalLock16(hwh);
	lpwh->size = size;
	lpwh->command = wCommand;
	if(nlen) {
		lpwh->ofsFilename = sizeof(WINHELP);
		strcpy(((char*)lpwh) + sizeof(WINHELP),lpHelpFile);
	}
	if(dsize) {
		memcpy(((char*)lpwh)+sizeof(WINHELP)+nlen,(LPSTR)dwData,dsize);
		lpwh->ofsData = sizeof(WINHELP)+nlen;
	} else
		lpwh->ofsData = 0;
	GlobalUnlock16(hwh);
	return SendMessage16(hDest,WM_WINHELP,hWnd,hwh);
}


/**********************************************************************
 *             WinHelp32W   (USER32.579)
 */
BOOL32 WinHelp32W( HWND32 hWnd, LPCWSTR helpFile, UINT32 command,
                   DWORD dwData )
{
    LPSTR file = HEAP_strdupWtoA( GetProcessHeap(), 0, helpFile );
    BOOL32 ret = WinHelp32A( hWnd, file, command, dwData );
    HeapFree( GetProcessHeap(), 0, file );
    return ret;
}
