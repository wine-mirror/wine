/*		
 * Common controls functions
 *
 * Copyright 1997 Dimitrie O. Paun
 * Copyright 1998 Eric Kohl
 *
 */

#include "win.h"
#include "heap.h"
#include "commctrl.h"
#include "header.h"
#include "listview.h"
#include "pager.h"
#include "progress.h"
#include "rebar.h"
#include "status.h"
#include "toolbar.h"
#include "tooltips.h"
#include "trackbar.h"
#include "treeview.h"
#include "updown.h"
#include "debug.h"

/***********************************************************************
 * ComCtl32LibMain
 */

BOOL32 WINAPI ComCtl32LibMain (HINSTANCE32 hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{ TRACE(commctrl,"%x,%lx,%p\n",hinstDLL,fdwReason,lpvReserved);
  if ( fdwReason == DLL_PROCESS_ATTACH)
  { InitCommonControls();
  }
  return TRUE;

}
/***********************************************************************
 * DrawStatusText32A [COMCTL32.5][COMCTL32.27]
 *
 * Draws text with borders, like in a status bar.
 *
 * PARAMS
 *     hdc   [I] handle to the window's display context
 *     lprc  [I] pointer to a rectangle
 *     text  [I] pointer to the text
 *     style [I] 
 */

VOID WINAPI
DrawStatusText32A (HDC32 hdc, LPRECT32 lprc, LPCSTR text, UINT32 style)
{
    RECT32 r = *lprc;
    UINT32 border = BDR_SUNKENOUTER;

    if (style == SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if (style == SBT_NOBORDERS)
      border = 0;

    DrawEdge32 (hdc, &r, border, BF_RECT|BF_ADJUST|BF_MIDDLE);

    /* now draw text */
    if (text) {
      int oldbkmode = SetBkMode32 (hdc, TRANSPARENT);
      r.left += 3;
      DrawText32A (hdc, text, lstrlen32A(text),
		   &r, DT_LEFT|DT_VCENTER|DT_SINGLELINE);  
      if (oldbkmode != TRANSPARENT)
	SetBkMode32(hdc, oldbkmode);
    }
}


/***********************************************************************
 * DrawStatusText32W [COMCTL32.28]
 */
void WINAPI DrawStatusText32W( HDC32 hdc, LPRECT32 lprc, LPCWSTR text,
                               UINT32 style )
{
  LPSTR p = HEAP_strdupWtoA( GetProcessHeap(), 0, text );
  DrawStatusText32A(hdc, lprc, p, style);
  HeapFree( GetProcessHeap(), 0, p );         
}


/***********************************************************************
 * CreateStatusWindow32A [COMCTL32.6][COMCTL32.21]
 */
HWND32 WINAPI CreateStatusWindow32A( INT32 style, LPCSTR text, HWND32 parent,
                                     UINT32 wid )
{
    return CreateWindow32A(STATUSCLASSNAME32A, text, style, 
			   CW_USEDEFAULT32, CW_USEDEFAULT32,
			   CW_USEDEFAULT32, CW_USEDEFAULT32, 
			   parent, wid, 0, 0);
}


/***********************************************************************
 *           CreateStatusWindow32W   (COMCTL32.22)
 */
HWND32 WINAPI CreateStatusWindow32W( INT32 style, LPCWSTR text, HWND32 parent,
                                     UINT32 wid )
{
    return CreateWindow32W((LPCWSTR)STATUSCLASSNAME32W, text, style, 
			   CW_USEDEFAULT32, CW_USEDEFAULT32,
			   CW_USEDEFAULT32, CW_USEDEFAULT32, 
			   parent, wid, 0, 0);
}

/***********************************************************************
 *           CreateUpDownControl  (COMCTL32.16)
 */
HWND32 WINAPI CreateUpDownControl( DWORD style, INT32 x, INT32 y,
                                   INT32 cx, INT32 cy, HWND32 parent,
                                   INT32 id, HINSTANCE32 inst, HWND32 buddy,
                                   INT32 maxVal, INT32 minVal, INT32 curVal )
{
  HWND32 hUD = CreateWindow32A(UPDOWN_CLASS32A, 0, style, x, y, cx, cy,
			       parent, id, inst, 0);
  if(hUD){
    SendMessage32A(hUD, UDM_SETBUDDY, buddy, 0);
    SendMessage32A(hUD, UDM_SETRANGE, 0, MAKELONG(maxVal, minVal));
    SendMessage32A(hUD, UDM_SETPOS, 0, MAKELONG(curVal, 0));     
  }

  return hUD;
}


/***********************************************************************
 * InitCommonControls [COMCTL32.17]
 *
 * Registers the common controls.
 *
 * PARAMS
 *     None.
 *
 * NOTES
 *     Calls InitCommonControlsEx.
 *     InitCommonControlsEx should be used instead.
 */

VOID WINAPI
InitCommonControls (VOID)
{
    INITCOMMONCONTROLSEX icc;

    icc.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icc.dwICC = ICC_WIN95_CLASSES;

    InitCommonControlsEx (&icc);
}


/***********************************************************************
 * InitCommonControlsEx [COMCTL32.81]
 *
 * Registers the common controls.
 *
 * PARAMS
 *     lpInitCtrls [I] pointer to a INITCOMMONCONTROLS structure.
 */

BOOL32 WINAPI
InitCommonControlsEx (LPINITCOMMONCONTROLSEX lpInitCtrls)
{
  INT32 cCount;
  DWORD dwMask;

  TRACE(commctrl,"\n");
  
  if (lpInitCtrls == NULL) return FALSE;
  if (lpInitCtrls->dwSize < sizeof(INITCOMMONCONTROLSEX)) return FALSE;

  for (cCount = 0; cCount <= 31; cCount++) {
    dwMask = 1 << cCount;
    if (!(lpInitCtrls->dwICC & dwMask))
      continue;

    switch (lpInitCtrls->dwICC & dwMask) {
      case ICC_LISTVIEW_CLASSES:
        LISTVIEW_Register ();
        HEADER_Register ();
        break;

      case ICC_TREEVIEW_CLASSES:
        TREEVIEW_Register ();
	TOOLTIPS_Register ();
        break;

      case ICC_BAR_CLASSES:
	TOOLBAR_Register ();
	STATUS_Register ();
	TRACKBAR_Register ();
	TOOLTIPS_Register ();
        break;

      case ICC_TAB_CLASSES:
        TRACE (commctrl, "No tab class implemented!\n");
	TOOLTIPS_Register ();
        UPDOWN_Register ();
        break;

      case ICC_UPDOWN_CLASS:
        UPDOWN_Register ();
        break;

      case ICC_PROGRESS_CLASS:
        PROGRESS_Register ();
        break;

      case ICC_HOTKEY_CLASS:
        TRACE (commctrl, "No hotkey class implemented!\n");
        break;

      case ICC_ANIMATE_CLASS:
        TRACE (commctrl, "No animation class implemented!\n");
        break;

      /* advanced classes - not included in Win95 */
      case ICC_DATE_CLASSES:
        TRACE (commctrl, "No month calendar class implemented!\n");
        TRACE (commctrl, "No date picker class implemented!\n");
        TRACE (commctrl, "No time picker class implemented!\n");
        UPDOWN_Register ();
        break;

      case ICC_USEREX_CLASSES:
        TRACE (commctrl, "No comboex class implemented!\n");
        break;

      case ICC_COOL_CLASSES:
	REBAR_Register ();
        break;

      case ICC_INTERNET_CLASSES:
        TRACE (commctrl, "No IPAddress class implemented!\n");
        break;

      case ICC_PAGESCROLLER_CLASS:
	PAGER_Register ();
        break;

      case ICC_NATIVEFNTCTL_CLASS:
        TRACE (commctrl, "No native font class implemented!\n");
        break;

      default:
        WARN (commctrl, "Unknown class! dwICC=0x%lX\n", dwMask);
        break;
    }
  }

  return TRUE;
}


/***********************************************************************
 * MenuHelp [COMCTL32.2]
 *
 * PARAMS
 *     uMsg
 *     wParam
 *     lParam
 *     hMainMenu
 *     hInst
 *     hwndStatus
 *     lpwIDs
 */

VOID WINAPI
MenuHelp (UINT32 uMsg, WPARAM32 wParam, LPARAM lParam, HMENU32 hMainMenu,
	  HINSTANCE32 hInst, HWND32 hwndStatus, LPUINT32 lpwIDs)
{
    char szStatusText[128];

    if (!IsWindow32 (hwndStatus)) return;

    switch (uMsg) {
	case WM_MENUSELECT:
            TRACE (commctrl, "WM_MENUSELECT wParam=0x%X lParam=0x%lX\n",
                   wParam, lParam);

            if ((HIWORD(wParam) == 0xFFFF) && (lParam == 0)) {
                /* menu was closed */
                SendMessage32A (hwndStatus, SB_SIMPLE, FALSE, 0);
            }
            else {
                if (HIWORD(wParam) & MF_POPUP) {
		    FIXME (commctrl, "popup 0x%08x 0x%08lx\n", wParam, lParam);

                    szStatusText[0] = 0;
                }
                else {
                    TRACE (commctrl, "menu item selected!\n");
                    if (!LoadString32A (hInst, LOWORD(wParam), szStatusText, 128))
                        szStatusText[0] = 0;
                }
                SendMessage32A (hwndStatus, SB_SETTEXT32A, 255 | SBT_NOBORDERS,
                                (LPARAM)szStatusText);
                SendMessage32A (hwndStatus, SB_SIMPLE, TRUE, 0);
            }
            break;

        default:
            WARN (commctrl, "Invalid Message!\n");
            break;
    }
}


/***********************************************************************
 * CreateToolbarEx [COMCTL32.32]
 *
 *
 *
 */

HWND32 WINAPI
CreateToolbarEx (HWND32 hwnd, DWORD style, UINT32 wID, INT32 nBitmaps,
                 HINSTANCE32 hBMInst, UINT32 wBMID, LPCTBBUTTON lpButtons,
                 INT32 iNumButtons, INT32 dxButton, INT32 dyButton,
                 INT32 dxBitmap, INT32 dyBitmap, UINT32 uStructSize)
{
    HWND32 hwndTB =
        CreateWindowEx32A (0, TOOLBARCLASSNAME32A, "", style, 0, 0, 0, 0,
			   hwnd, (HMENU32)wID, 0, NULL);
    if(hwndTB) {
	TBADDBITMAP tbab;

        SendMessage32A (hwndTB, TB_BUTTONSTRUCTSIZE,
			(WPARAM32)uStructSize, 0);

	/* set bitmap and button size */
	if (hBMInst == HINST_COMMCTRL) {
	    if (wBMID & 1) {
		SendMessage32A (hwndTB, TB_SETBITMAPSIZE, 0,
				MAKELPARAM(26, 25));
		SendMessage32A (hwndTB, TB_SETBUTTONSIZE, 0,
				MAKELPARAM(33, 32));
	    }
	    else {
		SendMessage32A (hwndTB, TB_SETBITMAPSIZE, 0,
				MAKELPARAM(16, 15));
		SendMessage32A (hwndTB, TB_SETBUTTONSIZE, 0,
				MAKELPARAM(23, 22));
	    }
	}
	else {
	    SendMessage32A (hwndTB, TB_SETBITMAPSIZE, 0,
			    MAKELPARAM((WORD)dyBitmap, (WORD)dxBitmap));
	    SendMessage32A (hwndTB, TB_SETBUTTONSIZE, 0,
			    MAKELPARAM((WORD)dyButton, (WORD)dxButton));
	}

	/* add bitmaps */
	tbab.hInst = hBMInst;
	tbab.nID   = wBMID;
	SendMessage32A (hwndTB, TB_ADDBITMAP,
			(WPARAM32)nBitmaps, (LPARAM)&tbab);

	/* add buttons */
	SendMessage32A (hwndTB, TB_ADDBUTTONS32A,
			(WPARAM32)iNumButtons, (LPARAM)lpButtons);
    }

    return hwndTB;
}


/***********************************************************************
 * CreateMappedBitmap [COMCTL32.8]
 *
 * PARAMS
 *     hInstance
 *     idBitmap
 *     wFlags
 *     lpColorMap
 *     iNumMaps
 */

HBITMAP32 WINAPI
CreateMappedBitmap (HINSTANCE32 hInstance, INT32 idBitmap, UINT32 wFlags,
		    LPCOLORMAP lpColorMap, INT32 iNumMaps)
{
    HGLOBAL32 hglb;
    HRSRC32 hRsrc;
    LPBITMAPINFOHEADER lpBitmap, lpBitmapInfo;
    UINT32 nSize, nColorTableSize;
    DWORD *pColorTable;
    INT32 iColor, i, iMaps, nWidth, nHeight;
    HDC32 hdcScreen;
    HBITMAP32 hbm;
    LPCOLORMAP sysColorMap;
    COLORMAP internalColorMap[4] =
	{{0x000000, 0}, {0x808080, 0}, {0xC0C0C0, 0}, {0xFFFFFF, 0}};

    /* initialize pointer to colortable and default color table */
    if (lpColorMap) {
	iMaps = iNumMaps;
	sysColorMap = lpColorMap;
    }
    else {
	internalColorMap[0].to = GetSysColor32 (COLOR_BTNTEXT);
	internalColorMap[1].to = GetSysColor32 (COLOR_BTNSHADOW);
	internalColorMap[2].to = GetSysColor32 (COLOR_BTNFACE);
	internalColorMap[3].to = GetSysColor32 (COLOR_BTNHIGHLIGHT);
	iMaps = 4;
	sysColorMap = (LPCOLORMAP)internalColorMap;
    }

    hRsrc = FindResource32A (hInstance, (LPSTR)idBitmap, RT_BITMAP32A);
    if (hRsrc == 0)
	return 0;
    hglb = LoadResource32 (hInstance, hRsrc);
    if (hglb == 0)
	return 0;
    lpBitmap = (LPBITMAPINFOHEADER)LockResource32 (hglb);
    if (lpBitmap == NULL)
	return 0;

    nColorTableSize = (1 << lpBitmap->biBitCount);
    nSize = lpBitmap->biSize + nColorTableSize * sizeof(RGBQUAD);
    lpBitmapInfo = (LPBITMAPINFOHEADER)GlobalAlloc32 (GMEM_FIXED, nSize);
    if (lpBitmapInfo == NULL)
	return 0;
    RtlMoveMemory (lpBitmapInfo, lpBitmap, nSize);

    pColorTable = (DWORD*)(((LPBYTE)lpBitmapInfo)+(UINT32)lpBitmapInfo->biSize);

    for (iColor = 0; iColor < nColorTableSize; iColor++) {
	for (i = 0; i < iMaps; i++) {
	    if (pColorTable[iColor] == sysColorMap[i].from) {
#if 0
		if (wFlags & CBS_MASKED) {
		    if (sysColorMap[i].to != COLOR_BTNTEXT)
			pColorTable[iColor] = RGB(255, 255, 255);
		}
		else
#endif
		    pColorTable[iColor] = sysColorMap[i].to;
		break;
	    }
	}
    }

    nWidth  = (INT32)lpBitmapInfo->biWidth;
    nHeight = (INT32)lpBitmapInfo->biHeight;
    hdcScreen = GetDC32 ((HWND32)0);
    hbm = CreateCompatibleBitmap32 (hdcScreen, nWidth, nHeight);
    if (hbm) {
	HDC32 hdcDst = CreateCompatibleDC32 (hdcScreen);
	HBITMAP32 hbmOld = SelectObject32 (hdcDst, hbm);
	LPBYTE lpBits = (LPBYTE)(lpBitmap + 1);
	lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);
	StretchDIBits32 (hdcDst, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight,
		         lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS,
		         SRCCOPY);
	SelectObject32 (hdcDst, hbmOld);
	DeleteDC32 (hdcDst);
    }
    ReleaseDC32 ((HWND32)0, hdcScreen);
    GlobalFree32 ((HGLOBAL32)lpBitmapInfo);
    FreeResource32 (hglb);

    return hbm;
}


/***********************************************************************
 * CreateToolbar [COMCTL32.7]
 *
 *
 *
 */

HWND32 WINAPI
CreateToolbar (HWND32 hwnd, DWORD style, UINT32 wID, INT32 nBitmaps,
	       HINSTANCE32 hBMInst, UINT32 wBMID,
	       LPCOLDTBBUTTON lpButtons,INT32 iNumButtons)
{
    return CreateToolbarEx (hwnd, style | CCS_NODIVIDER, wID, nBitmaps,
			    hBMInst, wBMID, (LPCTBBUTTON)lpButtons,
			    iNumButtons, 0, 0, 0, 0, sizeof (OLDTBBUTTON));
}


/***********************************************************************
 * GetEffectiveClientRect [COMCTL32.4]
 *
 * PARAMS
 *     hwnd   [I] handle to the client window.
 *     lpRect [O] pointer to the rectangle of the client window
 *     lpInfo [I] pointer to an array of integers
 *
 * NOTES
 */

VOID WINAPI
GetEffectiveClientRect (HWND32 hwnd, LPRECT32 lpRect, LPINT32 lpInfo)
{
    RECT32 rcCtrl;
    INT32  *lpRun;
    HWND32 hwndCtrl;

    TRACE (commctrl, "(0x%08lx 0x%08lx 0x%08lx)\n",
	   (DWORD)hwnd, (DWORD)lpRect, (DWORD)lpInfo);

    GetClientRect32 (hwnd, lpRect);
    lpRun = lpInfo;

    do {
	lpRun += 2;
	if (*lpRun == 0)
	    return;
	lpRun++;
	hwndCtrl = GetDlgItem32 (hwnd, *lpRun);
	if (GetWindowLong32A (hwndCtrl, GWL_STYLE) & WS_VISIBLE) {
	    TRACE (commctrl, "control id 0x%x\n", *lpRun);
	    GetWindowRect32 (hwndCtrl, &rcCtrl);
	    MapWindowPoints32 ((HWND32)0, hwnd, (LPPOINT32)&rcCtrl, 2);
	    SubtractRect32 (lpRect, lpRect, &rcCtrl);
	}
	lpRun++;
    } while (*lpRun);
}


/***********************************************************************
 * ShowHideMenuCtl [COMCTL32.3]
 *
 * PARAMS
 *     hwnd   [I] handle to the client window.
 *     uFlags [I] menu command id
 *     lpInfo [I] pointer to an array of integers
 *
 * NOTES
 */

BOOL32 WINAPI
ShowHideMenuCtl (HWND32 hwnd, UINT32 uFlags, LPINT32 lpInfo)
{
    FIXME (commctrl, "(0x%08x 0x%08x %p): empty stub!\n",
	   hwnd, uFlags, lpInfo);
#if 0
    if (GetMenuState (lpInfo[1], uFlags, MF_BYCOMMAND) & MFS_CHECKED) {
	/* checked -> hide control */

    }
    else {
	/* not checked -> show control */

    }

#endif

    return FALSE;
}
