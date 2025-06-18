/*
 * COMMDLG - Print Dialog
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1996 Albrecht Kleine
 * Copyright 1999 Klaas van Gend
 * Copyright 2000 Huw D M Davies
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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "winuser.h"
#include "wine/winuser16.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"
#include "winspool.h"
#include "cdlg16.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

static void global_handle_to_16( HGLOBAL16 *h16, HGLOBAL handle )
{
    DWORD size;
    HGLOBAL16 ret;

    if (!handle) return;
    size = GlobalSize( handle );
    if (*h16) ret = GlobalReAlloc16( *h16, size, GMEM_MOVEABLE );
    else ret = GlobalAlloc16( GMEM_MOVEABLE, size );
    if (ret)
    {
        void *src = GlobalLock( handle );
        void *dst = GlobalLock16( ret );
        memcpy( dst, src, size );
        GlobalUnlock( handle );
        GlobalUnlock16( ret );
        *h16 = ret;
    }
}

static HGLOBAL global_handle_from_16( HGLOBAL16 handle )
{
    DWORD size;
    HGLOBAL ret;

    if (!handle) return 0;
    size = GlobalSize16( handle );
    if ((ret = GlobalAlloc( GMEM_MOVEABLE, size )))
    {
        void *src = GlobalLock16( handle );
        void *dst = GlobalLock( ret );
        memcpy( dst, src, size );
        GlobalUnlock16( handle );
        GlobalUnlock( ret );
    }
    return ret;
}

/**********************************************************************
 *
 *      16 bit commdlg
 */

/***********************************************************************
 *           PrintDlg   (COMMDLG.20)
 *
 *  Displays the PRINT dialog box, which enables the user to specify
 *  specific properties of the print job.
 *
 * RETURNS
 *  nonzero if the user pressed the OK button
 *  zero    if the user cancelled the window or an error occurred
 *
 * BUGS
 *  * calls up to the 32-bit versions of the Dialogs, which look different
 *  * Customizing is *not* implemented.
 */

BOOL16 WINAPI PrintDlg16( LPPRINTDLG16 lppd )
{
    PRINTDLGA pd32;
    BOOL ret;

    if (!lppd) return PrintDlgA(NULL); /* generate failure with CDERR_INITIALIZATION */

    pd32.lStructSize = sizeof(pd32);
    pd32.Flags       = lppd->Flags & ~(PD_ENABLEPRINTTEMPLATE | PD_ENABLEPRINTTEMPLATEHANDLE |
                                       PD_ENABLESETUPTEMPLATE | PD_ENABLESETUPTEMPLATEHANDLE |
                                       PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK);
    pd32.hwndOwner   = HWND_32(lppd->hwndOwner);
    pd32.hDevMode    = global_handle_from_16( lppd->hDevMode );
    pd32.hDevNames   = global_handle_from_16( lppd->hDevNames );
    pd32.nFromPage   = lppd->nFromPage;
    pd32.nToPage     = lppd->nToPage;
    pd32.nMinPage    = lppd->nMinPage;
    pd32.nMaxPage    = lppd->nMaxPage;
    pd32.nCopies     = lppd->nCopies;

    if (lppd->Flags & (PD_ENABLEPRINTTEMPLATE | PD_ENABLEPRINTTEMPLATEHANDLE |
                       PD_ENABLESETUPTEMPLATE | PD_ENABLESETUPTEMPLATEHANDLE))
        FIXME( "custom templates no longer supported, using default\n" );
    if (lppd->Flags & PD_ENABLEPRINTHOOK)
        FIXME( "custom print hook %p no longer supported\n", lppd->lpfnPrintHook );
    if (lppd->Flags & PD_ENABLESETUPHOOK)
        FIXME( "custom setup hook %p no longer supported\n", lppd->lpfnSetupHook );

    /* Generate failure with CDERR_STRUCTSIZE, when needed */
    if (lppd->lStructSize != sizeof(PRINTDLG16)) pd32.lStructSize--;

    if ((ret = PrintDlgA( &pd32 )))
    {
        lppd->hDC = HDC_16( pd32.hDC );
        global_handle_to_16( &lppd->hDevNames, pd32.hDevNames );
        global_handle_to_16( &lppd->hDevMode, pd32.hDevMode );

        lppd->nFromPage   = pd32.nFromPage;
        lppd->nToPage     = pd32.nToPage;
        lppd->nMinPage    = pd32.nMinPage;
        lppd->nMaxPage    = pd32.nMaxPage;
        lppd->nCopies     = pd32.nCopies;
    }
    GlobalFree( pd32.hDevNames );
    GlobalFree( pd32.hDevMode );
    return ret;
}

/***********************************************************************
 *           PrintDlgProc   (COMMDLG.21)
 */
BOOL16 CALLBACK PrintDlgProc16(HWND16 hDlg16, UINT16 uMsg, WPARAM16 wParam, LPARAM lParam)
{
    FIXME( "%04x %04x %04x %08Ix: stub\n", hDlg16, uMsg, wParam, lParam );
    return FALSE;
}

/***********************************************************************
 *           PrintSetupDlgProc   (COMMDLG.22)
 */
BOOL16 CALLBACK PrintSetupDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
				   LPARAM lParam)
{
  HWND hWnd = HWND_32(hWnd16);
  switch (wMsg)
    {
    case WM_INITDIALOG:
      TRACE("WM_INITDIALOG lParam=%08IX\n", lParam);
      ShowWindow(hWnd, SW_SHOWNORMAL);
      return (TRUE);
    case WM_COMMAND:
      switch (wParam) {
      case IDOK:
	EndDialog(hWnd, TRUE);
	return(TRUE);
      case IDCANCEL:
	EndDialog(hWnd, FALSE);
	return(TRUE);
      }
      return(FALSE);
    }
  return FALSE;
}
