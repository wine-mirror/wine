/*
 * COMMDLG - Color Dialog
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

/* BUGS : still seems to not refresh correctly
   sometimes, especially when 2 instances of the
   dialog are loaded at the same time */

#ifndef _WINE_COLORDLG_H
#define _WINE_COLORDLG_H

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "winuser.h"
#include "commdlg.h"
#include "dlgs.h"
#include "wine/debug.h"
#include "cderr.h"

#include "cdlg.h"

typedef struct CCPRIVATE
{
 LPCHOOSECOLORW lpcc;  /* points to public known data structure */
 LPCHOOSECOLOR16 lpcc16; /* save the 16 bits pointer */
 int nextuserdef;     /* next free place in user defined color array */
 HDC hdcMem;        /* color graph used for BitBlt() */
 HBITMAP hbmMem;    /* color graph bitmap */
 RECT fullsize;     /* original dialog window size */
 UINT msetrgb;        /* # of SETRGBSTRING message (today not used)  */
 RECT old3angle;    /* last position of l-marker */
 RECT oldcross;     /* last position of color/satuation marker */
 BOOL updating;     /* to prevent recursive WM_COMMAND/EN_UPDATE processing */
 int h;
 int s;
 int l;               /* for temporary storing of hue,sat,lum */
 int capturedGraph; /* control mouse captured */
 RECT focusRect;    /* rectangle last focused item */
 HWND hwndFocus;    /* handle last focused item */
} *LCCPRIV;

/*
 * Internal Functions
 * Do NOT Export to other programs and dlls
 */

BOOL CC_HookCallChk( LPCHOOSECOLORW lpcc );
int CC_MouseCheckResultWindow( HWND hDlg, LPARAM lParam );
LONG CC_WMInitDialog( HWND hDlg, WPARAM wParam, LPARAM lParam, BOOL b16 );
LRESULT CC_WMLButtonDown( HWND hDlg, WPARAM wParam, LPARAM lParam );
LRESULT CC_WMLButtonUp( HWND hDlg, WPARAM wParam, LPARAM lParam );
LRESULT CC_WMCommand( HWND hDlg, WPARAM wParam, LPARAM lParam, WORD 
						notifyCode, HWND hwndCtl );
LRESULT CC_WMMouseMove( HWND hDlg, LPARAM lParam );
LRESULT CC_WMPaint( HWND hDlg, WPARAM wParam, LPARAM lParam );

#endif /* _WINE_COLORDLG_H */
