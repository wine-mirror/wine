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
#include "updown.h"
#include "debug.h"


/***********************************************************************
 *           DrawStatusText32A   (COMCTL32.5)
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
 *           DrawStatusText16   (COMCTL32.27)
 */
void WINAPI DrawStatusText16( HDC16 hdc, LPRECT16 lprc, LPCSTR text,
			      UINT16 style )
{
  if(!lprc)
    DrawStatusText32A((HDC32)hdc, 0, text, (UINT32)style);
  else{    
    RECT32 rect32;
    CONV_RECT16TO32( lprc, &rect32 );
    DrawStatusText32A((HDC32)hdc, &rect32, text, (UINT32)style);
  }
}

/***********************************************************************
 *           CreateStatusWindow32A   (COMCTL32.6)
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
 *           CreateStatusWindow16   (COMCTL32.21)
 */
HWND16 WINAPI CreateStatusWindow16( INT16 style, LPCSTR text, HWND16 parent,
				    UINT16 wid )
{
    return CreateWindow16(STATUSCLASSNAME16, text, style, 
			   CW_USEDEFAULT16, CW_USEDEFAULT16,
			   CW_USEDEFAULT16, CW_USEDEFAULT16, 
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

  if (lpInitCtrls == NULL) return (FALSE);
  if (lpInitCtrls->dwSize < sizeof(INITCOMMONCONTROLSEX)) return (FALSE);

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
        TRACE (commctrl, "No toolbar class implemented!\n");
	STATUS_Register ();
        TRACE (commctrl, "No trackbar class implemented!\n");
        TRACE (commctrl, "No tooltip class implemented!\n");
        break;

      case ICC_TAB_CLASSES:
        TRACE (commctrl, "No tab class implemented!\n");
        TRACE (commctrl, "No tooltip class implemented!\n");
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

      case ICC_DATE_CLASSES:
        TRACE (commctrl, "No month calendar class implemented!\n");
        TRACE (commctrl, "No date picker class implemented!\n");
        TRACE (commctrl, "No time picker class implemented!\n");
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

  return (TRUE);
}


/***********************************************************************
 * MenuHelp (COMCTL32.2)
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
                    TRACE (commctrl, "Popup menu selected!\n");
                    FIXME (commctrl, "No popup menu texts!\n");

                    szStatusText[0] = 0;
                }
                else {
                    TRACE (commctrl, "Menu item selected!\n");
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
