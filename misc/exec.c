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
	char	str[256];
    	dprintf_exec(stddeb,"WinHelp(%s, %u, %lu)\n", 
		lpHelpFile, wCommand, dwData);
	switch(wCommand) {
	case 0:
	case HELP_HELPONHELP:
		GetWindowsDirectory(str, sizeof(str));
		strcat(str, "\\winhelp.exe winhelp.hlp");
        dprintf_exec(stddeb,"'%s'\n", str);
		break;
	case HELP_INDEX:
		GetWindowsDirectory(str, sizeof(str));
		strcat(str, "\\winhelp.exe ");
		strcat(str, lpHelpFile);
        dprintf_exec(stddeb,"'%s'\n", str);
		break;
        case HELP_QUIT:
            return TRUE;
	default:
		return FALSE;
	}
	WinExec(str, SW_SHOWNORMAL);
	return(TRUE);
}
