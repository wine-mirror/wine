/*
 *	common shell dialogs
 */
#include <string.h>
#include <stdio.h>
#include "winerror.h"
#include "debugtools.h"

#include "shellapi.h"
#include "shlobj.h"
#include "shell32_main.h"
#include "wine/undocshell.h"

DEFAULT_DEBUG_CHANNEL(shell);


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
	if (MessageBoxA( hWndOwner, "Do you want to exit WINE?", "Shutdown", MB_YESNO|MB_ICONQUESTION) == IDOK)
	{
	  SendMessageA ( hWndOwner, WM_QUIT, 0, 0);
	}
}
