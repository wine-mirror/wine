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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winnls.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/unicode.h"
#include "commdlg.h"
#include "wine/debug.h"
#include "cderr.h"
#include "winternl.h"

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "filedlg.h"

/***********************************************************************
 *                              FILEDLG_WMMeasureItem16         [internal]
 */
static LONG FILEDLG_WMMeasureItem16(HWND16 hWnd, WPARAM16 wParam, LPARAM lParam)
{
    LPMEASUREITEMSTRUCT16 lpmeasure;

    lpmeasure = MapSL(lParam);
    lpmeasure->itemHeight = fldrHeight;
    return TRUE;
}

/* ------------------ Dialog procedures ---------------------- */

/***********************************************************************
 *           FileOpenDlgProc   (COMMDLG.6)
 */
BOOL16 CALLBACK FileOpenDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{
    HWND hWnd = HWND_32(hWnd16);
    LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);
    DRAWITEMSTRUCT dis;

    TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
    if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
        {
            LRESULT lRet = (BOOL16)FILEDLG_CallWindowProc(lfs, wMsg, wParam, lParam);
            if (lRet)
                return lRet;         /* else continue message processing */
        }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FILEDLG_WMInitDialog(hWnd, wParam, lParam);

    case WM_MEASUREITEM:
        return FILEDLG_WMMeasureItem16(hWnd16, wParam, lParam);

    case WM_DRAWITEM:
        FILEDLG_MapDrawItemStruct(MapSL(lParam), &dis);
        return FILEDLG_WMDrawItem(hWnd, wParam, lParam, FALSE, &dis);

    case WM_COMMAND:
        return FILEDLG_WMCommand(hWnd, lParam, HIWORD(lParam),wParam, lfs);
#if 0
    case WM_CTLCOLOR:
         SetBkColor((HDC16)wParam, 0x00C0C0C0);
         switch (HIWORD(lParam))
         {
	 case CTLCOLOR_BTN:
	     SetTextColor((HDC16)wParam, 0x00000000);
             return hGRAYBrush;
	case CTLCOLOR_STATIC:
             SetTextColor((HDC16)wParam, 0x00000000);
             return hGRAYBrush;
	}
      break;
#endif
    }
    return FALSE;
}

/***********************************************************************
 *           FileSaveDlgProc   (COMMDLG.7)
 */
BOOL16 CALLBACK FileSaveDlgProc16(HWND16 hWnd16, UINT16 wMsg, WPARAM16 wParam,
                               LPARAM lParam)
{
 HWND hWnd = HWND_32(hWnd16);
 LFSPRIVATE lfs = (LFSPRIVATE)GetPropA(hWnd,OFN_PROP);
 DRAWITEMSTRUCT dis;

 TRACE("msg=%x wparam=%x lParam=%lx\n", wMsg, wParam, lParam);
 if ((wMsg != WM_INITDIALOG) && lfs && lfs->hook)
  {
   LRESULT  lRet;
   lRet = (BOOL16)FILEDLG_CallWindowProc(lfs, wMsg, wParam, lParam);
   if (lRet)
    return lRet;         /* else continue message processing */
  }
  switch (wMsg) {
   case WM_INITDIALOG:
      return FILEDLG_WMInitDialog(hWnd, wParam, lParam);

   case WM_MEASUREITEM:
      return FILEDLG_WMMeasureItem16(hWnd16, wParam, lParam);

   case WM_DRAWITEM:
      FILEDLG_MapDrawItemStruct(MapSL(lParam), &dis);
      return FILEDLG_WMDrawItem(hWnd, wParam, lParam, TRUE, &dis);

   case WM_COMMAND:
      return FILEDLG_WMCommand(hWnd, lParam, HIWORD(lParam), wParam, lfs);
  }

  /*
  case WM_CTLCOLOR:
   SetBkColor((HDC16)wParam, 0x00C0C0C0);
   switch (HIWORD(lParam))
   {
    case CTLCOLOR_BTN:
     SetTextColor((HDC16)wParam, 0x00000000);
     return hGRAYBrush;
    case CTLCOLOR_STATIC:
     SetTextColor((HDC16)wParam, 0x00000000);
     return hGRAYBrush;
   }
   return FALSE;

   */
  return FALSE;
}

/* ------------------ APIs ---------------------- */

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
 *    unknown, there are some FIXME's left.
 */
BOOL16 WINAPI GetOpenFileName16(
				SEGPTR ofn /* [in/out] address of structure with data*/
				)
{
    HINSTANCE16 hInst;
    BOOL bRet = FALSE;
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    LFSPRIVATE lfs;
    FARPROC16 ptr;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) ofn, LFS16, OPEN_DIALOG);
    if (lfs)
    {
        hInst = GetWindowWord( HWND_32(lpofn->hwndOwner), GWL_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 6);
        bRet = DialogBoxIndirectParam16( hInst, lfs->hDlgTmpl16, lpofn->hwndOwner,
                                         (DLGPROC16) ptr, (LPARAM) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", (char *)MapSL(lpofn->lpstrFile));
    return bRet;
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
 *    unknown. There are some FIXME's left.
 */
BOOL16 WINAPI GetSaveFileName16(
				SEGPTR ofn /* [in/out] addess of structure with data*/
				)
{
    HINSTANCE16 hInst;
    BOOL bRet = FALSE;
    LPOPENFILENAME16 lpofn = MapSL(ofn);
    LFSPRIVATE lfs;
    FARPROC16 ptr;

    if (!lpofn || !FileDlg_Init()) return FALSE;

    lfs = FILEDLG_AllocPrivate((LPARAM) ofn, LFS16, SAVE_DIALOG);
    if (lfs)
    {
        hInst = GetWindowWord( HWND_32(lpofn->hwndOwner), GWL_HINSTANCE );
        ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 7);
        bRet = DialogBoxIndirectParam16( hInst, lfs->hDlgTmpl16, lpofn->hwndOwner,
                                         (DLGPROC16) ptr, (LPARAM) lfs);
        FILEDLG_DestroyPrivate(lfs);
    }

    TRACE("return lpstrFile='%s' !\n", (char *)MapSL(lpofn->lpstrFile));
    return bRet;
}
