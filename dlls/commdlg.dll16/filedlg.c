/*
 * COMMDLG - File Dialogs
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */
#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"
#include "cdlg16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

static UINT_PTR CALLBACK dummy_hook( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp )
{
    return FALSE;
}

/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08lx: stub\n", hWnd16, wMsg, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *           FileSaveDlgProc   (COMMDLG.7)
 */
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08lx: stub\n", hWnd16, wMsg, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *           GetOpenFileName   (COMMDLG.1)
 *
 * Creates a dialog box for the user to select a file to open.
 *
 * RETURNS
 *    TRUE on success: user selected a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown, there are some FIXMEs left.
 */
BOOL16 WINAPI GetOpenFileName16( SEGPTR ofn ) /* [in/out] address of structure with data*/
{
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    OPENFILENAMEA ofn32;
    BOOL ret;

    if (!lpofn) return FALSE;

    ofn32.lStructSize       = OPENFILENAME_SIZE_VERSION_400A;
    ofn32.hwndOwner         = HWND_32( lpofn->hwndOwner );
    ofn32.lpstrFilter       = MapSL( lpofn->lpstrFilter );
    ofn32.lpstrCustomFilter = MapSL( lpofn->lpstrCustomFilter );
    ofn32.nMaxCustFilter    = lpofn->nMaxCustFilter;
    ofn32.nFilterIndex      = lpofn->nFilterIndex;
    ofn32.lpstrFile         = MapSL( lpofn->lpstrFile );
    ofn32.nMaxFile          = lpofn->nMaxFile;
    ofn32.lpstrFileTitle    = MapSL( lpofn->lpstrFileTitle );
    ofn32.nMaxFileTitle     = lpofn->nMaxFileTitle;
    ofn32.lpstrInitialDir   = MapSL( lpofn->lpstrInitialDir );
    ofn32.lpstrTitle        = MapSL( lpofn->lpstrTitle );
    ofn32.Flags             = (lpofn->Flags & ~OFN_ENABLETEMPLATE) | OFN_ENABLEHOOK;
    ofn32.nFileOffset       = lpofn->nFileOffset;
    ofn32.nFileExtension    = lpofn->nFileExtension;
    ofn32.lpstrDefExt       = MapSL( lpofn->lpstrDefExt );
    ofn32.lCustData         = lpofn->lCustData;
    ofn32.lpfnHook          = dummy_hook;  /* this is to force old 3.1 dialog style */

    if (lpofn->Flags & OFN_ENABLETEMPLATE)
        FIXME( "custom templates no longer supported, using default\n" );
    if (lpofn->Flags & OFN_ENABLEHOOK)
        FIXME( "custom hook %p no longer supported\n", lpofn->lpfnHook );

    if ((ret = GetOpenFileNameA( &ofn32 )))
    {
	lpofn->nFilterIndex   = ofn32.nFilterIndex;
	lpofn->nFileOffset    = ofn32.nFileOffset;
	lpofn->nFileExtension = ofn32.nFileExtension;
    }
    return ret;
}

/***********************************************************************
 *           GetSaveFileName   (COMMDLG.2)
 *
 * Creates a dialog box for the user to select a file to save.
 *
 * RETURNS
 *    TRUE on success: user enters a valid file
 *    FALSE on cancel, error, close or filename-does-not-fit-in-buffer.
 *
 * BUGS
 *    unknown. There are some FIXMEs left.
 */
BOOL16 WINAPI GetSaveFileName16( SEGPTR ofn ) /* [in/out] address of structure with data*/
{
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    OPENFILENAMEA ofn32;
    BOOL ret;

    if (!lpofn) return FALSE;

    ofn32.lStructSize       = OPENFILENAME_SIZE_VERSION_400A;
    ofn32.hwndOwner         = HWND_32( lpofn->hwndOwner );
    ofn32.lpstrFilter       = MapSL( lpofn->lpstrFilter );
    ofn32.lpstrCustomFilter = MapSL( lpofn->lpstrCustomFilter );
    ofn32.nMaxCustFilter    = lpofn->nMaxCustFilter;
    ofn32.nFilterIndex      = lpofn->nFilterIndex;
    ofn32.lpstrFile         = MapSL( lpofn->lpstrFile );
    ofn32.nMaxFile          = lpofn->nMaxFile;
    ofn32.lpstrFileTitle    = MapSL( lpofn->lpstrFileTitle );
    ofn32.nMaxFileTitle     = lpofn->nMaxFileTitle;
    ofn32.lpstrInitialDir   = MapSL( lpofn->lpstrInitialDir );
    ofn32.lpstrTitle        = MapSL( lpofn->lpstrTitle );
    ofn32.Flags             = (lpofn->Flags & ~OFN_ENABLETEMPLATE) | OFN_ENABLEHOOK;
    ofn32.nFileOffset       = lpofn->nFileOffset;
    ofn32.nFileExtension    = lpofn->nFileExtension;
    ofn32.lpstrDefExt       = MapSL( lpofn->lpstrDefExt );
    ofn32.lCustData         = lpofn->lCustData;
    ofn32.lpfnHook          = dummy_hook;  /* this is to force old 3.1 dialog style */

    if (lpofn->Flags & OFN_ENABLETEMPLATE)
        FIXME( "custom templates no longer supported, using default\n" );
    if (lpofn->Flags & OFN_ENABLEHOOK)
        FIXME( "custom hook %p no longer supported\n", lpofn->lpfnHook );

    if ((ret = GetSaveFileNameA( &ofn32 )))
    {
	lpofn->nFilterIndex   = ofn32.nFilterIndex;
	lpofn->nFileOffset    = ofn32.nFileOffset;
	lpofn->nFileExtension = ofn32.nFileExtension;
    }
    return ret;
}


/***********************************************************************
 *	GetFileTitle		(COMMDLG.27)
 */
short WINAPI GetFileTitle16(LPCSTR lpFile, LPSTR lpTitle, UINT16 cbBuf)
{
	return GetFileTitleA(lpFile, lpTitle, cbBuf);
}
