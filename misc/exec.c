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


/**********************************************************************
 *				ExitWindows		[USER.7]
 */
BOOL ExitWindows(DWORD dwReturnCode, WORD wReserved)
{
    api_assert("ExitWindows", wReserved == 0);
    api_assert("ExitWindows", HIWORD(dwReturnCode) == 0);

    dprintf_exec( stdnimp,"PARTIAL STUB ExitWindows(%08lX, %04X)\n", 
                  dwReturnCode, wReserved);

    /* Do the clean-up stuff */

    WriteOutProfiles();
    SHELL_SaveRegistry();

    exit( LOWORD(dwReturnCode) );
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
	default:
		return FALSE;
	}
	WinExec(str, SW_SHOWNORMAL);
	return(TRUE);
}
