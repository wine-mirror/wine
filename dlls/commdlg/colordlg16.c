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

#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
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

WINE_DEFAULT_DEBUG_CHANNEL(commdlg);

#include "cdlg.h"
#include "colordlg.h"

/***********************************************************************
 *           ColorDlgProc   (COMMDLG.8)
 */
BOOL16 CALLBACK ColorDlgProc16( HWND16 hDlg16, UINT16 message,
                            WPARAM16 wParam, LONG lParam )
{
    BOOL16 res;
    HWND hDlg = HWND_32(hDlg16);

    LCCPRIV lpp = (LCCPRIV)GetWindowLongA(hDlg, DWL_USER);
    if (message != WM_INITDIALOG)
    {
        if (!lpp)
            return FALSE;
        res=0;
        if (CC_HookCallChk(lpp->lpcc))
            res = CallWindowProc16( (WNDPROC16)lpp->lpcc16->lpfnHook, hDlg16, message, wParam, lParam);
        if (res)
            return res;
    }

    /* FIXME: SetRGB message
       if (message && message == msetrgb)
       return HandleSetRGB(hDlg, lParam);
    */

    switch (message)
    {
	  case WM_INITDIALOG:
	                return CC_WMInitDialog(hDlg, wParam, lParam, TRUE);
	  case WM_NCDESTROY:
	                DeleteDC(lpp->hdcMem);
	                DeleteObject(lpp->hbmMem);
                        HeapFree(GetProcessHeap(), 0, lpp->lpcc);
                        HeapFree(GetProcessHeap(), 0, lpp);
	                SetWindowLongA(hDlg, DWL_USER, 0L); /* we don't need it anymore */
	                break;
	  case WM_COMMAND:
	                if (CC_WMCommand(hDlg, wParam, lParam,
					 HIWORD(lParam), HWND_32(LOWORD(lParam))))
	                   return TRUE;
	                break;
	  case WM_PAINT:
	                if (CC_WMPaint(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
	  case WM_LBUTTONDBLCLK:
	                if (CC_MouseCheckResultWindow(hDlg,lParam))
			  return TRUE;
			break;
	  case WM_MOUSEMOVE:
	                if (CC_WMMouseMove(hDlg, lParam))
			  return TRUE;
			break;
	  case WM_LBUTTONUP:  /* FIXME: ClipCursor off (if in color graph)*/
                        if (CC_WMLButtonUp(hDlg, wParam, lParam))
                           return TRUE;
			break;
	  case WM_LBUTTONDOWN:/* FIXME: ClipCursor on  (if in color graph)*/
	                if (CC_WMLButtonDown(hDlg, wParam, lParam))
	                   return TRUE;
	                break;
    }
    return FALSE ;
}



/***********************************************************************
 *            ChooseColor  (COMMDLG.5)
 */
BOOL16 WINAPI ChooseColor16( LPCHOOSECOLOR16 lpChCol )
{
    HINSTANCE16 hInst;
    HANDLE16 hDlgTmpl16 = 0, hResource16 = 0;
    HGLOBAL16 hGlobal16 = 0;
    BOOL16 bRet = FALSE;
    LPCVOID template;
    FARPROC16 ptr;

    TRACE("ChooseColor\n");
    if (!lpChCol) return FALSE;

    if (lpChCol->Flags & CC_ENABLETEMPLATEHANDLE)
        hDlgTmpl16 = lpChCol->hInstance;
    else if (lpChCol->Flags & CC_ENABLETEMPLATE)
    {
        HANDLE16 hResInfo;
        if (!(hResInfo = FindResource16(lpChCol->hInstance,
                                        MapSL(lpChCol->lpTemplateName),
                                        (LPSTR)RT_DIALOG)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
            return FALSE;
        }
        if (!(hDlgTmpl16 = LoadResource16(lpChCol->hInstance, hResInfo)))
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
            return FALSE;
        }
        hResource16 = hDlgTmpl16;
    }
    else
    {
	HRSRC hResInfo;
	HGLOBAL hDlgTmpl32;
        LPCVOID template32;
        DWORD size;
	if (!(hResInfo = FindResourceA(COMDLG32_hInstance, "CHOOSE_COLOR", (LPSTR)RT_DIALOG)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_FINDRESFAILURE);
	    return FALSE;
	}
	if (!(hDlgTmpl32 = LoadResource(COMDLG32_hInstance, hResInfo)) ||
	    !(template32 = LockResource(hDlgTmpl32)))
	{
	    COMDLG32_SetCommDlgExtendedError(CDERR_LOADRESFAILURE);
	    return FALSE;
	}
        size = SizeofResource(GetModuleHandleA("COMDLG32"), hResInfo);
        hGlobal16 = GlobalAlloc16(0, size);
        if (!hGlobal16)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMALLOCFAILURE);
            ERR("alloc failure for %ld bytes\n", size);
            return FALSE;
        }
        template = GlobalLock16(hGlobal16);
        if (!template)
        {
            COMDLG32_SetCommDlgExtendedError(CDERR_MEMLOCKFAILURE);
            ERR("global lock failure for %x handle\n", hDlgTmpl16);
            GlobalFree16(hGlobal16);
            return FALSE;
        }
        ConvertDialog32To16((LPVOID)template32, size, (LPVOID)template);
        hDlgTmpl16 = hGlobal16;
    }

    ptr = GetProcAddress16(GetModuleHandle16("COMMDLG"), (LPCSTR) 8);
    hInst = GetWindowLongA(HWND_32(lpChCol->hwndOwner), GWL_HINSTANCE);
    bRet = DialogBoxIndirectParam16(hInst, hDlgTmpl16, lpChCol->hwndOwner,
                     (DLGPROC16) ptr, (DWORD)lpChCol);
    if (hResource16) FreeResource16(hDlgTmpl16);
    if (hGlobal16)
    {
        GlobalUnlock16(hGlobal16);
        GlobalFree16(hGlobal16);
    }
    return bRet;
}
