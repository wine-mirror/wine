/*
 * Windows Help
 *
 * Copyright 1996 Martin von Loewis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "wine/debug.h"
#include "windef.h"
#include "wingdi.h"
#include "wownt32.h"
#include "wine/winuser16.h"
#include "wine/winbase16.h"
#include "win.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);


/* WinHelp internal structure */
typedef struct
{
    WORD size;
    WORD command;
    LONG data;
    LONG reserved;
    WORD ofsFilename;
    WORD ofsData;
} WINHELP,*LPWINHELP;

/**********************************************************************
 *		WinHelp (USER.171)
 */
BOOL16 WINAPI WinHelp16( HWND16 hWnd, LPCSTR lpHelpFile, UINT16 wCommand,
                         DWORD dwData )
{
  BOOL ret;
  DWORD mutex_count;

  /* We might call WinExec() */
  ReleaseThunkLock( &mutex_count );

  if (!(ret = WinHelpA( WIN_Handle32(hWnd), lpHelpFile, wCommand, (DWORD)MapSL(dwData) )))
  {
      /* try to start the 16-bit winhelp */
      if (WinExec( "winhelp.exe -x", SW_SHOWNORMAL ) >= 32)
      {
          K32WOWYield16();
          ret = WinHelpA( WIN_Handle32(hWnd), lpHelpFile, wCommand, (DWORD)MapSL(dwData) );
      }
  }

  RestoreThunkLock( mutex_count );
  return ret;
}


/**********************************************************************
 *		WinHelpA (USER32.@)
 */
BOOL WINAPI WinHelpA( HWND hWnd, LPCSTR lpHelpFile, UINT wCommand,
                          DWORD dwData )
{
	static WORD WM_WINHELP = 0;
	HWND hDest;
	LPWINHELP lpwh;
	HGLOBAL16 hwh;
	int size,dsize,nlen;


	if(!WM_WINHELP)
	  {
	    WM_WINHELP=RegisterWindowMessageA("WM_WINHELP");
	    if(!WM_WINHELP)
	      return FALSE;
	  }

	hDest = FindWindowA( "MS_WINHELP", NULL );
	if(!hDest) {
	  if(wCommand == HELP_QUIT) return TRUE;
          if (WinExec ( "winhlp32.exe -x", SW_SHOWNORMAL ) < 32) {
	      ERR("can't start winhlp32.exe -x ?\n");
	      return FALSE;
	  }
	  if ( ! ( hDest = FindWindowA ( "MS_WINHELP", NULL ) )) {
	      FIXME("did not find MS_WINHELP (FindWindow() failed, maybe global window handling still unimplemented)\n");
	      return FALSE;
	  }
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
			dsize = dwData ? strlen( (LPSTR)dwData )+1: 0;
			break;
		case HELP_MULTIKEY:
			dsize = ((LPMULTIKEYHELPA)dwData)->mkSize;
			break;
		case HELP_SETWINPOS:
			dsize = ((LPHELPWININFOA)dwData)->wStructSize;
			break;
		default:
			FIXME("Unknown help command %d\n",wCommand);
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
	return SendMessage16(HWND_16(hDest),WM_WINHELP,HWND_16(hWnd),hwh);
}


/**********************************************************************
 *		WinHelpW (USER32.@)
 */
BOOL WINAPI WinHelpW( HWND hWnd, LPCWSTR helpFile, UINT command, DWORD dwData )
{
    INT len;
    LPSTR file;
    BOOL ret = FALSE;

    if (!helpFile) return WinHelpA( hWnd, NULL, command, dwData );

    len = WideCharToMultiByte( CP_ACP, 0, helpFile, -1, NULL, 0, NULL, NULL );
    if ((file = HeapAlloc( GetProcessHeap(), 0, len )))
    {
        WideCharToMultiByte( CP_ACP, 0, helpFile, -1, file, len, NULL, NULL );
        ret = WinHelpA( hWnd, file, command, dwData );
        HeapFree( GetProcessHeap(), 0, file );
    }
    return ret;
}
