/*
 * Windows Help
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "debugtools.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "winversion.h"
#include "heap.h"
#include "ldt.h"
#include "syslevel.h"

DEFAULT_DEBUG_CHANNEL(win)


/**********************************************************************
 *             WinHelp16   (USER.171)
 */
BOOL16 WINAPI WinHelp16( HWND16 hWnd, LPCSTR lpHelpFile, UINT16 wCommand,
                         DWORD dwData )
{
  BOOL ret;
  /* We might call WinExec() */
  SYSLEVEL_ReleaseWin16Lock();
  ret = WinHelpA( hWnd, lpHelpFile, wCommand, (DWORD)PTR_SEG_TO_LIN(dwData) );
  SYSLEVEL_RestoreWin16Lock();
  return ret;
}


/**********************************************************************
 *             WinHelpA   (USER32.579)
 */
BOOL WINAPI WinHelpA( HWND hWnd, LPCSTR lpHelpFile, UINT wCommand,
                          DWORD dwData )
{
	static WORD WM_WINHELP = 0;
	HWND hDest;
	LPWINHELP lpwh;
	HGLOBAL16 hwh;
	HINSTANCE winhelp;
	int size,dsize,nlen;


	if(!WM_WINHELP) 
	  {
	    WM_WINHELP=RegisterWindowMessageA("WM_WINHELP");
	    if(!WM_WINHELP)
	      return FALSE;
	  }

	hDest = FindWindowA( "MS_WINHELP", NULL );
	if(!hDest) {
	  if(wCommand == HELP_QUIT)
	    return TRUE;
	  else
	    if ( VERSION_GetVersion() == WIN31 ) {
	      winhelp = WinExec ( "winhelp.exe -x", SW_SHOWNORMAL );
	      Yield16();
	    }
	    else {
	      winhelp = WinExec ( "winhlp32.exe -x", SW_SHOWNORMAL );
	    }	  
	  if ( winhelp <= 32 ) return FALSE;
	  if ( ! ( hDest = FindWindowA ( "MS_WINHELP", NULL ) )) return FALSE;
        }


	switch(wCommand)
	{
		case HELP_CONTEXT:
		case HELP_SETCONTENTS:
		case HELP_CONTENTS:
		case HELP_CONTEXTPOPUP:
		case HELP_FORCEFILE:
		case HELP_HELPONHELP:
		case HELP_FINDER:
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
			WARN("Unknown help command %d\n",wCommand);
			return FALSE;
	}
	if(lpHelpFile)
		nlen = strlen(lpHelpFile)+1;
	else
		nlen = 0;
	size = sizeof(WINHELP) + nlen + dsize;
	hwh = GlobalAlloc16(0,size);
	lpwh = GlobalLock16(hwh);
	lpwh->size = size;
	lpwh->command = wCommand;
	lpwh->data = dwData;
	if(nlen) {
		strcpy(((char*)lpwh) + sizeof(WINHELP),lpHelpFile);
		lpwh->ofsFilename = sizeof(WINHELP);
 	} else
		lpwh->ofsFilename = 0;
	if(dsize) {
		memcpy(((char*)lpwh)+sizeof(WINHELP)+nlen,(LPSTR)dwData,dsize);
		lpwh->ofsData = sizeof(WINHELP)+nlen;
	} else
		lpwh->ofsData = 0;
	GlobalUnlock16(hwh);
	return SendMessage16(hDest,WM_WINHELP,hWnd,hwh);
}


/**********************************************************************
 *             WinHelpW   (USER32.580)
 */
BOOL WINAPI WinHelpW( HWND hWnd, LPCWSTR helpFile, UINT command,
                          DWORD dwData )
{
    LPSTR file = HEAP_strdupWtoA( GetProcessHeap(), 0, helpFile );
    BOOL ret = WinHelpA( hWnd, file, command, dwData );
    HeapFree( GetProcessHeap(), 0, file );
    return ret;
}
