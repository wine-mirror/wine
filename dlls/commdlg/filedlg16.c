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
 *                              FILEDLG_CallWindowProc16          [internal]
 *
 *      Call the appropriate hook
 */
static BOOL FILEDLG_CallWindowProc16(LFSPRIVATE lfs, UINT wMsg, WPARAM wParam,
                                   LPARAM lParam)
{
    if (lfs->ofn16)
    {
        return (BOOL16) CallWindowProc16(
          (WNDPROC16)lfs->ofn16->lpfnHook, HWND_16(lfs->hwnd),
          (UINT16)wMsg, (WPARAM16)wParam, lParam);
    }
    return FALSE;
}

/***********************************************************************
 *                              FILEDLG_WMInitDialog16            [internal]
 *      The is a duplicate of the 32bit FILEDLG_WMInitDialog function 
 *      The only differnce is that it calls FILEDLG_CallWindowProc16 
 *      for a 16 bit Window Proc.
 */

static LONG FILEDLG_WMInitDialog16(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  int i, n;
  WCHAR tmpstr[BUFFILE];
  LPWSTR pstr, old_pstr;
  LPOPENFILENAMEW ofn;
  LFSPRIVATE lfs = (LFSPRIVATE) lParam;

  if (!lfs) return FALSE;
  SetPropA(hWnd, OFN_PROP, (HANDLE)lfs);
  lfs->hwnd = hWnd;
  ofn = lfs->ofnW;

  TRACE("flags=%lx initialdir=%s\n", ofn->Flags, debugstr_w(ofn->lpstrInitialDir));

  SetWindowTextW( hWnd, ofn->lpstrTitle );
  /* read custom filter information */
  if (ofn->lpstrCustomFilter)
    {
      pstr = ofn->lpstrCustomFilter;
      n = 0;
      TRACE("lpstrCustomFilter = %p\n", pstr);
      while(*pstr)
	{
	  old_pstr = pstr;
          i = SendDlgItemMessageW(hWnd, cmb1, CB_ADDSTRING, 0,
                                   (LPARAM)(ofn->lpstrCustomFilter) + n );
          n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	  TRACE("add str=%s associated to %s\n",
                debugstr_w(old_pstr), debugstr_w(pstr));
          SendDlgItemMessageW(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
          n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	}
    }
  /* read filter information */
  if (ofn->lpstrFilter) {
	pstr = (LPWSTR) ofn->lpstrFilter;
	n = 0;
	while(*pstr) {
	  old_pstr = pstr;
	  i = SendDlgItemMessageW(hWnd, cmb1, CB_ADDSTRING, 0,
				       (LPARAM)(ofn->lpstrFilter + n) );
	  n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	  TRACE("add str=%s associated to %s\n",
                debugstr_w(old_pstr), debugstr_w(pstr));
	  SendDlgItemMessageW(hWnd, cmb1, CB_SETITEMDATA, i, (LPARAM)pstr);
	  n += lstrlenW(pstr) + 1;
	  pstr += lstrlenW(pstr) + 1;
	}
  }
  /* set default filter */
  if (ofn->nFilterIndex == 0 && ofn->lpstrCustomFilter == NULL)
  	ofn->nFilterIndex = 1;
  SendDlgItemMessageW(hWnd, cmb1, CB_SETCURSEL, ofn->nFilterIndex - 1, 0);
  lstrcpynW(tmpstr, FILEDLG_GetFileType(ofn->lpstrCustomFilter,
	     (LPWSTR)ofn->lpstrFilter, ofn->nFilterIndex - 1),BUFFILE);
  TRACE("nFilterIndex = %ld, SetText of edt1 to %s\n",
  			ofn->nFilterIndex, debugstr_w(tmpstr));
  SetDlgItemTextW( hWnd, edt1, tmpstr );
  /* get drive list */
  *tmpstr = 0;
  DlgDirListComboBoxW(hWnd, tmpstr, cmb2, 0, DDL_DRIVES | DDL_EXCLUSIVE);
  /* read initial directory */
  /* FIXME: Note that this is now very version-specific (See MSDN description of
   * the OPENFILENAME structure).  For example under 2000/XP any path in the
   * lpstrFile overrides the lpstrInitialDir, but not under 95/98/ME
   */
  if (ofn->lpstrInitialDir != NULL)
    {
      int len;
      lstrcpynW(tmpstr, ofn->lpstrInitialDir, 511);
      len = lstrlenW(tmpstr);
      if (len > 0 && tmpstr[len-1] != '\\'  && tmpstr[len-1] != ':') {
        tmpstr[len]='\\';
        tmpstr[len+1]='\0';
      }
    }
  else
    *tmpstr = 0;
  if (!FILEDLG_ScanDir(hWnd, tmpstr)) {
    *tmpstr = 0;
    if (!FILEDLG_ScanDir(hWnd, tmpstr))
      WARN("Couldn't read initial directory %s!\n", debugstr_w(tmpstr));
  }
  /* select current drive in combo 2, omit missing drives */
  {
      char dir[MAX_PATH];
      char str[4] = "a:\\";
      GetCurrentDirectoryA( sizeof(dir), dir );
      for(i = 0, n = -1; i < 26; i++)
      {
          str[0] = 'a' + i;
          if (GetDriveTypeA(str) > DRIVE_NO_ROOT_DIR) n++;
          if (toupper(str[0]) == toupper(dir[0])) break;
      }
  }
  SendDlgItemMessageW(hWnd, cmb2, CB_SETCURSEL, n, 0);
  if (!(ofn->Flags & OFN_SHOWHELP))
    ShowWindow(GetDlgItem(hWnd, pshHelp), SW_HIDE);
  if (ofn->Flags & OFN_HIDEREADONLY)
    ShowWindow(GetDlgItem(hWnd, chx1), SW_HIDE);
  if (lfs->hook)
      return (BOOL) FILEDLG_CallWindowProc16(lfs, WM_INITDIALOG, wParam, lfs->lParam);
  return TRUE;
}

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
            LRESULT lRet = (BOOL16)FILEDLG_CallWindowProc16(lfs, wMsg, wParam, lParam);
            if (lRet)
                return lRet;         /* else continue message processing */
        }
    switch (wMsg)
    {
    case WM_INITDIALOG:
        return FILEDLG_WMInitDialog16(hWnd, wParam, lParam);

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
   lRet = (BOOL16)FILEDLG_CallWindowProc16(lfs, wMsg, wParam, lParam);
   if (lRet)
    return lRet;         /* else continue message processing */
  }
  switch (wMsg) {
   case WM_INITDIALOG:
      return FILEDLG_WMInitDialog16(hWnd, wParam, lParam);

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
