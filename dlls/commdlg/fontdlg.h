/*
 * COMMDLG - Font Dialog
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
 *
 * DO NOT EXPORT TO APPLICATIONS -
 * This file only defines the internal functions that are shared to 
 * implement the Win16 and Win32 font dialog support. 
 *
 */

#ifndef _WINE_FONTDLG_H
#define _WINE_FONTDLG_H

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "commdlg.h"

typedef struct
{
  HWND hWnd1;
  HWND hWnd2;
  LPCHOOSEFONTA lpcf32a;
  int  added;
} CFn_ENUMSTRUCT, *LPCFn_ENUMSTRUCT;

INT AddFontFamily(const LOGFONTA *lplf, UINT nFontType, LPCHOOSEFONTA lpcf, 
					HWND hwnd, LPCFn_ENUMSTRUCT e);
INT AddFontStyle(const LOGFONTA *lplf, UINT nFontType, LPCHOOSEFONTA lpcf, 
					HWND hcmb2, HWND hcmb3, HWND hDlg);
void _dump_cf_flags(DWORD cflags);

LRESULT CFn_WMInitDialog(HWND hDlg, WPARAM wParam, LPARAM lParam,
                         LPCHOOSEFONTA lpcf);
LRESULT CFn_WMMeasureItem(HWND hDlg, WPARAM wParam, LPARAM lParam);
LRESULT CFn_WMDrawItem(HWND hDlg, WPARAM wParam, LPARAM lParam);
LRESULT CFn_WMCommand(HWND hDlg, WPARAM wParam, LPARAM lParam,
                      LPCHOOSEFONTA lpcf);
LRESULT CFn_WMDestroy(HWND hwnd, WPARAM wParam, LPARAM lParam);

#endif /* _WINE_FONTDLG_H */
