/*
 *	common shell dialogs
 *
 * Copyright 2000 Juergen Schmied
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

#include <string.h>
#include <stdio.h>
#include "winerror.h"
#include "wine/debug.h"

#include "shellapi.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "undocshell.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);


/*************************************************************************
 * PickIconDlg					[SHELL32.62]
 *
 */
BOOL WINAPI PickIconDlg(
	HWND hwndOwner,
	LPSTR lpstrFile,
	DWORD nMaxFile,
	LPDWORD lpdwIconIndex)
{
	FIXME("(%08x,%s,%08lx,%p):stub.\n",
	  hwndOwner, lpstrFile, nMaxFile,lpdwIconIndex);
	return 0xffffffff;
}

/*************************************************************************
 * RunFileDlg					[SHELL32.61]
 *
 * NOTES
 *     Original name: RunFileDlg (exported by ordinal)
 */
void WINAPI RunFileDlg(
	HWND hwndOwner,
	HICON hIcon,
	LPCSTR lpstrDirectory,
	LPCSTR lpstrTitle,
	LPCSTR lpstrDescription,
	UINT uFlags)
{
	FIXME("(0x%04x 0x%04x %s %s %s 0x%08x):stub.\n",
	   hwndOwner, hIcon, lpstrDirectory, lpstrTitle, lpstrDescription, uFlags);
}

/*************************************************************************
 * ExitWindowsDialog				[SHELL32.60]
 *
 * NOTES
 *     exported by ordinal
 */
void WINAPI ExitWindowsDialog (HWND hWndOwner)
{
	TRACE("(0x%08x)\n", hWndOwner);
	if (MessageBoxA( hWndOwner, "Do you want to exit WINE?", "Shutdown", MB_YESNO|MB_ICONQUESTION) == IDYES)
	{
	  SendMessageA ( hWndOwner, WM_QUIT, 0, 0);
	}
}
