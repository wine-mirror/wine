/*		
 * Common controls functions
 *
 * Copyright 1997 Dimitrie O. Paun
 *
 */

#include "win.h"
#include "heap.h"
#include "commctrl.h"
#include "header.h"
#include "progress.h"
#include "status.h"
#include "toolbar.h"
#include "updown.h"
#include "debug.h"


/***********************************************************************
 * DrawStatusText32A [COMCTL32.5][COMCTL32.27]
 */
void WINAPI DrawStatusText32A( HDC32 hdc, LPRECT32 lprc, LPCSTR text,
                               UINT32 style )
{
    RECT32 r = *lprc;
    UINT32 border = BDR_SUNKENOUTER;

    if(style==SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if(style==SBT_NOBORDERS)
      border = 0;

    DrawEdge32(hdc, &r, border, BF_RECT|BF_ADJUST|BF_MIDDLE);

    /* now draw text */
    if (text) {
      int oldbkmode = SetBkMode32(hdc, TRANSPARENT);
      r.left += 3;
      DrawText32A(hdc, text, lstrlen32A(text),
		  &r, DT_LEFT|DT_VCENTER|DT_SINGLELINE);  
      if (oldbkmode != TRANSPARENT)
	SetBkMode32(hdc, oldbkmode);
    }
    
}

/***********************************************************************
 *           DrawStatusText32W   (COMCTL32.28)
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
 *
 *
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
 *
 *
 */

BOOL32 WINAPI
InitCommonControlsEx (LPINITCOMMONCONTROLSEX lpInitCtrls)
{
  INT32 cCount;
  DWORD dwMask;

  if (lpInitCtrls == NULL) return FALSE;
  if (lpInitCtrls->dwSize < sizeof(INITCOMMONCONTROLSEX)) return FALSE;

  for (cCount = 0; cCount <= 31; cCount++) {
    dwMask = 1 << cCount;
    if (!(lpInitCtrls->dwICC & dwMask))
      continue;

    switch (lpInitCtrls->dwICC & dwMask) {
      case ICC_LISTVIEW_CLASSES:
        TRACE (commctrl, "No listview class implemented!\n");
        HEADER_Register();
        break;

      case ICC_TREEVIEW_CLASSES:
        TRACE (commctrl, "No treeview class implemented!\n");
        TRACE (commctrl, "No tooltip class implemented!\n");
        break;

      case ICC_BAR_CLASSES:
	TOOLBAR_Register ();
	STATUS_Register ();
        TRACE (commctrl, "No trackbar class implemented!\n");
        TRACE (commctrl, "No tooltip class implemented!\n");
        break;

      case ICC_TAB_CLASSES:
        TRACE (commctrl, "No tab class implemented!\n");
        TRACE (commctrl, "No tooltip class implemented!\n");
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
        TRACE (commctrl, "No rebar class implemented!\n");
        break;

      case ICC_INTERNET_CLASSES:
        TRACE (commctrl, "No internet classes implemented!\n");
        break;

      case ICC_PAGESCROLLER_CLASS:
        TRACE (commctrl, "No page scroller class implemented!\n");
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
 *
 *
 *
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
                    TRACE (commctrl, "popup menu selected!\n");

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
 *
 *
 */

HBITMAP32 WINAPI
CreateMappedBitmap (HINSTANCE32 hInstance, INT32 idBitmap, UINT32 wFlags,
		    LPCOLORMAP lpColorMap, INT32 iNumMaps)
{
    HBITMAP32 hbm;

    FIXME (commctrl, "semi-stub!\n");

    hbm = LoadBitmap32A (hInstance, MAKEINTRESOURCE32A(idBitmap));

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
    RECT32 rcClient, rcCtrl;
    INT32  idCtrl, *lpRun;

    TRACE (commctrl, "hwnd=0x%08lx lpRect=0x%08lx lpInfo=0x%08lx\n",
	   (DWORD)hwnd, (DWORD)lpRect, (DWORD)lpInfo);

    GetClientRect32 (hwnd, &rcClient);
#if 0
    lpRun = lpInfo;

    do {
	lpRun += 3;
	idCtrl = *lpRun;
	if (idCtrl) {
	    TRACE (commctrl, "control id 0x%x\n", idCtrl);
	    GetWindowRect32 (GetDlgItem32 (hwnd, idCtrl), &rcCtrl);
	    MapWindowPoints32 (NULL, hwnd, (LPPOINT32)&rcCtrl, 2);
	    SubtractRect32 (&rcClient, &rcClient, &rcCtrl);
	    lpRun++;
	}
    } while (idCtrl);
#endif
    CopyRect32 (lpRect, &rcClient);
}

