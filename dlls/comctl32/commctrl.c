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
#include "animate.h"
#include "comboex.h"
#include "datetime.h"
#include "flatsb.h"
#include "header.h"
#include "hotkey.h"
#include "ipaddress.h"
#include "listview.h"
#include "monthcal.h"
#include "nativefont.h"
#include "pager.h"
#include "progress.h"
#include "propsheet.h"
#include "rebar.h"
#include "status.h"
#include "tab.h"
#include "toolbar.h"
#include "tooltips.h"
#include "trackbar.h"
#include "treeview.h"
#include "updown.h"
#include "debug.h"
#include "winerror.h"


HANDLE COMCTL32_hHeap = (HANDLE)NULL;
DWORD    COMCTL32_dwProcessesAttached = 0;
LPSTR    COMCTL32_aSubclass = (LPSTR)NULL;


/***********************************************************************
 * COMCTL32_LibMain [Internal] Initializes the internal 'COMCTL32.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the 'dlls' instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserverd, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI
COMCTL32_LibMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE (commctrl, "%x,%lx,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
	    if (COMCTL32_dwProcessesAttached == 0) {
		/* create private heap */
		COMCTL32_hHeap = HeapCreate (0, 0x10000, 0);
		TRACE (commctrl, "Heap created: 0x%x\n", COMCTL32_hHeap);

		/* add global subclassing atom (used by 'tooltip' and 'updown') */
		COMCTL32_aSubclass = (LPSTR)(DWORD)GlobalAddAtomA ("CC32SubclassInfo");
		TRACE (commctrl, "Subclassing atom added: %p\n",
		       COMCTL32_aSubclass);

		/* register all Win95 common control classes */
		ANIMATE_Register ();
		FLATSB_Register ();
		HEADER_Register ();
		HOTKEY_Register ();
		LISTVIEW_Register ();
		PROGRESS_Register ();
		PROPSHEET_Register ();
		STATUS_Register ();
		TAB_Register ();
		TOOLBAR_Register ();
		TOOLTIPS_Register ();
		TRACKBAR_Register ();
		TREEVIEW_Register ();
		UPDOWN_Register ();
	    }
	    COMCTL32_dwProcessesAttached++;
	    break;

	case DLL_PROCESS_DETACH:
	    COMCTL32_dwProcessesAttached--;
	    if (COMCTL32_dwProcessesAttached == 0) {
		/* unregister all common control classes */
		ANIMATE_Unregister ();
		COMBOEX_Unregister ();
		DATETIME_Unregister ();
		FLATSB_Unregister ();
		HEADER_Unregister ();
		HOTKEY_Unregister ();
		IPADDRESS_Unregister ();
		LISTVIEW_Unregister ();
		MONTHCAL_Unregister ();
		NATIVEFONT_Unregister ();
		PAGER_Unregister ();
		PROGRESS_Unregister ();
		PROPSHEET_UnRegister ();
		REBAR_Unregister ();
		STATUS_Unregister ();
		TAB_Unregister ();
		TOOLBAR_Unregister ();
		TOOLTIPS_Unregister ();
		TRACKBAR_Unregister ();
		TREEVIEW_Unregister ();
		UPDOWN_Unregister ();

		/* delete global subclassing atom */
		GlobalDeleteAtom (LOWORD(COMCTL32_aSubclass));
		TRACE (commctrl, "Subclassing atom deleted: %p\n",
		       COMCTL32_aSubclass);
		COMCTL32_aSubclass = (LPSTR)NULL;

		/* destroy private heap */
		HeapDestroy (COMCTL32_hHeap);
		TRACE (commctrl, "Heap destroyed: 0x%x\n", COMCTL32_hHeap);
		COMCTL32_hHeap = (HANDLE)NULL;
	    }
	    break;
    }

    return TRUE;
}


/***********************************************************************
 * MenuHelp [COMCTL32.2]
 *
 * PARAMS
 *     uMsg       [I] message (WM_MENUSELECT) (see NOTES)
 *     wParam     [I] wParam of the message uMsg
 *     lParam     [I] lParam of the message uMsg
 *     hMainMenu  [I] handle to the application's main menu
 *     hInst      [I] handle to the module that contains string resources
 *     hwndStatus [I] handle to the status bar window
 *     lpwIDs     [I] pointer to an array of intergers (see NOTES)
 *
 * RETURNS
 *     No return value
 *
 * NOTES
 *     The official documentation is incomplete!
 *     This is the correct documentation:
 *
 *     uMsg:
 *     MenuHelp() does NOT handle WM_COMMAND messages! It only handes
 *     WM_MENUSELECT messages.
 *
 *     lpwIDs:
 *     (will be written ...)
 */

VOID WINAPI
MenuHelp (UINT uMsg, WPARAM wParam, LPARAM lParam, HMENU hMainMenu,
	  HINSTANCE hInst, HWND hwndStatus, LPUINT lpwIDs)
{
    UINT uMenuID = 0;

    if (!IsWindow (hwndStatus))
	return;

    switch (uMsg) {
	case WM_MENUSELECT:
	    TRACE (commctrl, "WM_MENUSELECT wParam=0x%X lParam=0x%lX\n",
		   wParam, lParam);

            if ((HIWORD(wParam) == 0xFFFF) && (lParam == 0)) {
                /* menu was closed */
		TRACE (commctrl, "menu was closed!\n");
                SendMessageA (hwndStatus, SB_SIMPLE, FALSE, 0);
            }
	    else {
		/* menu item was selected */
		if (HIWORD(wParam) & MF_POPUP)
		    uMenuID = (UINT)*(lpwIDs+1);
		else
		    uMenuID = (UINT)LOWORD(wParam);
		TRACE (commctrl, "uMenuID = %u\n", uMenuID);

		if (uMenuID) {
		    CHAR szText[256];

		    if (!LoadStringA (hInst, uMenuID, szText, 256))
			szText[0] = '\0';

		    SendMessageA (hwndStatus, SB_SETTEXTA,
				    255 | SBT_NOBORDERS, (LPARAM)szText);
		    SendMessageA (hwndStatus, SB_SIMPLE, TRUE, 0);
		}
	    }
	    break;

	default:
	    FIXME (commctrl, "Invalid Message 0x%x!\n", uMsg);
	    break;
    }
}


/***********************************************************************
 * ShowHideMenuCtl [COMCTL32.3] 
 *
 * Shows or hides controls and updates the corresponding menu item.
 *
 * PARAMS
 *     hwnd   [I] handle to the client window.
 *     uFlags [I] menu command id.
 *     lpInfo [I] pointer to an array of integers. (See NOTES.)
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     The official documentation is incomplete!
 *     This is the correct documentation:
 *
 *     hwnd
 *     Handle to the window that contains the menu and controls.
 *
 *     uFlags
 *     Identifier of the menu item to receive or loose a check mark.
 *
 *     lpInfo
 *     The array of integers contains pairs of values. BOTH values of
 *     the first pair must be the handles to the application's main menu.
 *     Each subsequent pair consists of a menu id and control id.
 */

BOOL WINAPI
ShowHideMenuCtl (HWND hwnd, UINT uFlags, LPINT lpInfo)
{
    LPINT lpMenuId;

    TRACE (commctrl, "%x, %x, %p\n", hwnd, uFlags, lpInfo);

    if (lpInfo == NULL)
	return FALSE;

    if (!(lpInfo[0]) || !(lpInfo[1]))
	return FALSE;

    /* search for control */
    lpMenuId = &lpInfo[2];
    while (*lpMenuId != uFlags)
	lpMenuId += 2;

    if (GetMenuState (lpInfo[1], uFlags, MF_BYCOMMAND) & MFS_CHECKED) {
	/* uncheck menu item */
	CheckMenuItem (lpInfo[0], *lpMenuId, MF_BYCOMMAND | MF_UNCHECKED);

	/* hide control */
	lpMenuId++;
	SetWindowPos (GetDlgItem (hwnd, *lpMenuId), 0, 0, 0, 0, 0,
			SWP_HIDEWINDOW);
    }
    else {
	/* check menu item */
	CheckMenuItem (lpInfo[0], *lpMenuId, MF_BYCOMMAND | MF_CHECKED);

	/* show control */
	lpMenuId++;
	SetWindowPos (GetDlgItem (hwnd, *lpMenuId), 0, 0, 0, 0, 0,
			SWP_SHOWWINDOW);
    }

    return TRUE;
}


/***********************************************************************
 * GetEffectiveClientRect [COMCTL32.4]
 *
 * PARAMS
 *     hwnd   [I] handle to the client window.
 *     lpRect [O] pointer to the rectangle of the client window
 *     lpInfo [I] pointer to an array of integers (see NOTES)
 *
 * RETURNS
 *     No return value.
 *
 * NOTES
 *     The official documentation is incomplete!
 *     This is the correct documentation:
 *
 *     lpInfo
 *     (will be written...)
 */

VOID WINAPI
GetEffectiveClientRect (HWND hwnd, LPRECT lpRect, LPINT lpInfo)
{
    RECT rcCtrl;
    INT  *lpRun;
    HWND hwndCtrl;

    TRACE (commctrl, "(0x%08lx 0x%08lx 0x%08lx)\n",
	   (DWORD)hwnd, (DWORD)lpRect, (DWORD)lpInfo);

    GetClientRect (hwnd, lpRect);
    lpRun = lpInfo;

    do {
	lpRun += 2;
	if (*lpRun == 0)
	    return;
	lpRun++;
	hwndCtrl = GetDlgItem (hwnd, *lpRun);
	if (GetWindowLongA (hwndCtrl, GWL_STYLE) & WS_VISIBLE) {
	    TRACE (commctrl, "control id 0x%x\n", *lpRun);
	    GetWindowRect (hwndCtrl, &rcCtrl);
	    MapWindowPoints ((HWND)0, hwnd, (LPPOINT)&rcCtrl, 2);
	    SubtractRect (lpRect, lpRect, &rcCtrl);
	}
	lpRun++;
    } while (*lpRun);
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
 *     style [I] drawing style
 *
 * RETURNS
 *     No return value.
 *
 * NOTES
 *     The style variable can have one of the following values:
 *     (will be written ...)
 */

VOID WINAPI
DrawStatusTextA (HDC hdc, LPRECT lprc, LPCSTR text, UINT style)
{
    RECT r = *lprc;
    UINT border = BDR_SUNKENOUTER;

    if (style == SBT_POPOUT)
      border = BDR_RAISEDOUTER;
    else if (style == SBT_NOBORDERS)
      border = 0;

    DrawEdge (hdc, &r, border, BF_RECT|BF_ADJUST|BF_MIDDLE);

    /* now draw text */
    if (text) {
      int oldbkmode = SetBkMode (hdc, TRANSPARENT);
      r.left += 3;
      DrawTextA (hdc, text, lstrlenA(text),
		   &r, DT_LEFT|DT_VCENTER|DT_SINGLELINE);  
      if (oldbkmode != TRANSPARENT)
	SetBkMode(hdc, oldbkmode);
    }
}


/***********************************************************************
 * DrawStatusText32W [COMCTL32.28]
 *
 * Draws text with borders, like in a status bar.
 *
 * PARAMS
 *     hdc   [I] handle to the window's display context
 *     lprc  [I] pointer to a rectangle
 *     text  [I] pointer to the text
 *     style [I] drawing style
 *
 * RETURNS
 *     No return value.
 */

VOID WINAPI
DrawStatusTextW (HDC hdc, LPRECT lprc, LPCWSTR text, UINT style)
{
    LPSTR p = HEAP_strdupWtoA (GetProcessHeap (), 0, text);
    DrawStatusTextA (hdc, lprc, p, style);
    HeapFree (GetProcessHeap (), 0, p );
}


/***********************************************************************
 * CreateStatusWindow32A [COMCTL32.6][COMCTL32.21]
 *
 * Creates a status bar
 *
 * PARAMS
 *     style  [I] window style
 *     text   [I] pointer to the window text
 *     parent [I] handle to the parent window
 *     wid    [I] control id of the status bar
 *
 * RETURNS
 *     Success: handle to the status window
 *     Failure: 0
 */

HWND WINAPI
CreateStatusWindowA (INT style, LPCSTR text, HWND parent, UINT wid)
{
    return CreateWindowA(STATUSCLASSNAMEA, text, style, 
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   CW_USEDEFAULT, CW_USEDEFAULT, 
			   parent, wid, 0, 0);
}


/***********************************************************************
 * CreateStatusWindow32W [COMCTL32.22] Creates a status bar control
 *
 * PARAMS
 *     style  [I] window style
 *     text   [I] pointer to the window text
 *     parent [I] handle to the parent window
 *     wid    [I] control id of the status bar
 *
 * RETURNS
 *     Success: handle to the status window
 *     Failure: 0
 */

HWND WINAPI
CreateStatusWindowW (INT style, LPCWSTR text, HWND parent, UINT wid)
{
    return CreateWindowW(STATUSCLASSNAMEW, text, style,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   CW_USEDEFAULT, CW_USEDEFAULT,
			   parent, wid, 0, 0);
}


/***********************************************************************
 * CreateUpDownControl [COMCTL32.16] Creates an up-down control
 *
 * PARAMS
 *     style  [I] window styles
 *     x      [I] horizontal position of the control
 *     y      [I] vertical position of the control
 *     cx     [I] with of the control
 *     cy     [I] height of the control
 *     parent [I] handle to the parent window
 *     id     [I] the control's identifier
 *     inst   [I] handle to the application's module instance
 *     buddy  [I] handle to the buddy window, can be NULL
 *     maxVal [I] upper limit of the control
 *     minVal [I] lower limit of the control
 *     curVal [I] current value of the control
 *
 * RETURNS
 *     Success: handle to the updown control
 *     Failure: 0
 */

HWND WINAPI
CreateUpDownControl (DWORD style, INT x, INT y, INT cx, INT cy,
		     HWND parent, INT id, HINSTANCE inst,
		     HWND buddy, INT maxVal, INT minVal, INT curVal)
{
    HWND hUD =
	CreateWindowA (UPDOWN_CLASSA, 0, style, x, y, cx, cy,
			 parent, id, inst, 0);
    if (hUD) {
	SendMessageA (hUD, UDM_SETBUDDY, buddy, 0);
	SendMessageA (hUD, UDM_SETRANGE16, 0, MAKELONG(maxVal, minVal));
	SendMessageA (hUD, UDM_SETPOS, 0, MAKELONG(curVal, 0));     
    }

    return hUD;
}


/***********************************************************************
 * InitCommonControls [COMCTL32.17]
 *
 * Registers the common controls.
 *
 * PARAMS
 *     No parameters.
 *
 * RETURNS
 *     No return values.
 *
 * NOTES
 *     This function is just a dummy.
 *     The Win95 controls are registered at the DLL's initialization.
 *     To register other controls InitCommonControlsEx() must be used.
 */

VOID WINAPI
InitCommonControls (VOID)
{
}


/***********************************************************************
 * InitCommonControlsEx [COMCTL32.81]
 *
 * Registers the common controls.
 *
 * PARAMS
 *     lpInitCtrls [I] pointer to an INITCOMMONCONTROLS structure.
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 *
 * NOTES
 *     Only the additinal common controls are registered by this function.
 *     The Win95 controls are registered at the DLL's initialization.
 */

BOOL WINAPI
InitCommonControlsEx (LPINITCOMMONCONTROLSEX lpInitCtrls)
{
    INT cCount;
    DWORD dwMask;

    if (!lpInitCtrls)
	return FALSE;
    if (lpInitCtrls->dwSize != sizeof(INITCOMMONCONTROLSEX))
	return FALSE;

    TRACE(commctrl,"(0x%08lx)\n", lpInitCtrls->dwICC);

    for (cCount = 0; cCount < 32; cCount++) {
	dwMask = 1 << cCount;
	if (!(lpInitCtrls->dwICC & dwMask))
	    continue;

	switch (lpInitCtrls->dwICC & dwMask) {
	    /* dummy initialization */
	    case ICC_ANIMATE_CLASS:
	    case ICC_BAR_CLASSES:
	    case ICC_LISTVIEW_CLASSES:
	    case ICC_TREEVIEW_CLASSES:
	    case ICC_TAB_CLASSES:
	    case ICC_UPDOWN_CLASS:
	    case ICC_PROGRESS_CLASS:
	    case ICC_HOTKEY_CLASS:
		break;

	    /* advanced classes - not included in Win95 */
	    case ICC_DATE_CLASSES:
		MONTHCAL_Register ();
		DATETIME_Register ();
		break;

	    case ICC_USEREX_CLASSES:
		COMBOEX_Register ();
		break;

	    case ICC_COOL_CLASSES:
		REBAR_Register ();
		break;

	    case ICC_INTERNET_CLASSES:
		IPADDRESS_Register ();
		break;

	    case ICC_PAGESCROLLER_CLASS:
		PAGER_Register ();
		break;

	    case ICC_NATIVEFNTCTL_CLASS:
		NATIVEFONT_Register ();
		break;

	    default:
		FIXME (commctrl, "Unknown class! dwICC=0x%lX\n", dwMask);
		break;
	}
    }

    return TRUE;
}


/***********************************************************************
 * CreateToolbarEx [COMCTL32.32] Creates a tool bar window
 *
 * PARAMS
 *     hwnd
 *     style
 *     wID
 *     nBitmaps
 *     hBMInst
 *     wBMID
 *     lpButtons
 *     iNumButtons
 *     dxButton
 *     dyButton
 *     dxBitmap
 *     dyBitmap
 *     uStructSize
 *
 * RETURNS
 *     Success: handle to the tool bar control
 *     Failure: 0
 */

HWND WINAPI
CreateToolbarEx (HWND hwnd, DWORD style, UINT wID, INT nBitmaps,
                 HINSTANCE hBMInst, UINT wBMID, LPCTBBUTTON lpButtons,
                 INT iNumButtons, INT dxButton, INT dyButton,
                 INT dxBitmap, INT dyBitmap, UINT uStructSize)
{
    HWND hwndTB =
        CreateWindowExA (0, TOOLBARCLASSNAMEA, "", style, 0, 0, 0, 0,
			   hwnd, (HMENU)wID, 0, NULL);
    if(hwndTB) {
	TBADDBITMAP tbab;

        SendMessageA (hwndTB, TB_BUTTONSTRUCTSIZE,
			(WPARAM)uStructSize, 0);

	/* set bitmap and button size */
	if (hBMInst == HINST_COMMCTRL) {
	    if (wBMID & 1) {
		SendMessageA (hwndTB, TB_SETBITMAPSIZE, 0,
				MAKELPARAM(26, 25));
		SendMessageA (hwndTB, TB_SETBUTTONSIZE, 0,
				MAKELPARAM(33, 32));
	    }
	    else {
		SendMessageA (hwndTB, TB_SETBITMAPSIZE, 0,
				MAKELPARAM(16, 15));
		SendMessageA (hwndTB, TB_SETBUTTONSIZE, 0,
				MAKELPARAM(23, 22));
	    }
	}
	else {
	    SendMessageA (hwndTB, TB_SETBITMAPSIZE, 0,
			    MAKELPARAM((WORD)dyBitmap, (WORD)dxBitmap));
	    SendMessageA (hwndTB, TB_SETBUTTONSIZE, 0,
			    MAKELPARAM((WORD)dyButton, (WORD)dxButton));
	}

	/* add bitmaps */
	tbab.hInst = hBMInst;
	tbab.nID   = wBMID;
	SendMessageA (hwndTB, TB_ADDBITMAP,
			(WPARAM)nBitmaps, (LPARAM)&tbab);

	/* add buttons */
	SendMessageA (hwndTB, TB_ADDBUTTONSA,
			(WPARAM)iNumButtons, (LPARAM)lpButtons);
    }

    return hwndTB;
}


/***********************************************************************
 * CreateMappedBitmap [COMCTL32.8]
 *
 * PARAMS
 *     hInstance  [I]
 *     idBitmap   [I]
 *     wFlags     [I]
 *     lpColorMap [I]
 *     iNumMaps   [I]
 *
 * RETURNS
 *     Success: handle to the new bitmap
 *     Failure: 0
 */

HBITMAP WINAPI
CreateMappedBitmap (HINSTANCE hInstance, INT idBitmap, UINT wFlags,
		    LPCOLORMAP lpColorMap, INT iNumMaps)
{
    HGLOBAL hglb;
    HRSRC hRsrc;
    LPBITMAPINFOHEADER lpBitmap, lpBitmapInfo;
    UINT nSize, nColorTableSize;
    DWORD *pColorTable;
    INT iColor, i, iMaps, nWidth, nHeight;
    HDC hdcScreen;
    HBITMAP hbm;
    LPCOLORMAP sysColorMap;
    COLORMAP internalColorMap[4] =
	{{0x000000, 0}, {0x808080, 0}, {0xC0C0C0, 0}, {0xFFFFFF, 0}};

    /* initialize pointer to colortable and default color table */
    if (lpColorMap) {
	iMaps = iNumMaps;
	sysColorMap = lpColorMap;
    }
    else {
	internalColorMap[0].to = GetSysColor (COLOR_BTNTEXT);
	internalColorMap[1].to = GetSysColor (COLOR_BTNSHADOW);
	internalColorMap[2].to = GetSysColor (COLOR_BTNFACE);
	internalColorMap[3].to = GetSysColor (COLOR_BTNHIGHLIGHT);
	iMaps = 4;
	sysColorMap = (LPCOLORMAP)internalColorMap;
    }

    hRsrc = FindResourceA (hInstance, (LPSTR)idBitmap, RT_BITMAPA);
    if (hRsrc == 0)
	return 0;
    hglb = LoadResource (hInstance, hRsrc);
    if (hglb == 0)
	return 0;
    lpBitmap = (LPBITMAPINFOHEADER)LockResource (hglb);
    if (lpBitmap == NULL)
	return 0;

    nColorTableSize = (1 << lpBitmap->biBitCount);
    nSize = lpBitmap->biSize + nColorTableSize * sizeof(RGBQUAD);
    lpBitmapInfo = (LPBITMAPINFOHEADER)GlobalAlloc (GMEM_FIXED, nSize);
    if (lpBitmapInfo == NULL)
	return 0;
    RtlMoveMemory (lpBitmapInfo, lpBitmap, nSize);

    pColorTable = (DWORD*)(((LPBYTE)lpBitmapInfo)+(UINT)lpBitmapInfo->biSize);

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

    nWidth  = (INT)lpBitmapInfo->biWidth;
    nHeight = (INT)lpBitmapInfo->biHeight;
    hdcScreen = GetDC ((HWND)0);
    hbm = CreateCompatibleBitmap (hdcScreen, nWidth, nHeight);
    if (hbm) {
	HDC hdcDst = CreateCompatibleDC (hdcScreen);
	HBITMAP hbmOld = SelectObject (hdcDst, hbm);
	LPBYTE lpBits = (LPBYTE)(lpBitmap + 1);
	lpBits += (1 << (lpBitmapInfo->biBitCount)) * sizeof(RGBQUAD);
	StretchDIBits (hdcDst, 0, 0, nWidth, nHeight, 0, 0, nWidth, nHeight,
		         lpBits, (LPBITMAPINFO)lpBitmapInfo, DIB_RGB_COLORS,
		         SRCCOPY);
	SelectObject (hdcDst, hbmOld);
	DeleteDC (hdcDst);
    }
    ReleaseDC ((HWND)0, hdcScreen);
    GlobalFree ((HGLOBAL)lpBitmapInfo);
    FreeResource (hglb);

    return hbm;
}


/***********************************************************************
 * CreateToolbar [COMCTL32.7] Creates a tool bar control
 *
 * PARAMS
 *     hwnd
 *     style
 *     wID
 *     nBitmaps
 *     hBMInst
 *     wBMID
 *     lpButtons
 *     iNumButtons
 *
 * RETURNS
 *     Success: handle to the tool bar control
 *     Failure: 0
 *
 * NOTES
 *     Do not use this functions anymore. Use CreateToolbarEx instead.
 */

HWND WINAPI
CreateToolbar (HWND hwnd, DWORD style, UINT wID, INT nBitmaps,
	       HINSTANCE hBMInst, UINT wBMID,
	       LPCOLDTBBUTTON lpButtons,INT iNumButtons)
{
    return CreateToolbarEx (hwnd, style | CCS_NODIVIDER, wID, nBitmaps,
			    hBMInst, wBMID, (LPCTBBUTTON)lpButtons,
			    iNumButtons, 0, 0, 0, 0, sizeof (OLDTBBUTTON));
}


/***********************************************************************
 * DllGetVersion [COMCTL32.25]
 *
 * Retrieves version information of the 'COMCTL32.DLL'
 *
 * PARAMS
 *     pdvi [O] pointer to version information structure.
 *
 * RETURNS
 *     Success: S_OK
 *     Failure: E_INVALIDARG
 *
 * NOTES
 *     Returns version of a comctl32.dll from IE4.01 SP1.
 */

HRESULT WINAPI
COMCTL32_DllGetVersion (DLLVERSIONINFO *pdvi)
{
    if (pdvi->cbSize != sizeof(DLLVERSIONINFO)) {
        WARN (commctrl, "wrong DLLVERSIONINFO size from app");
	return E_INVALIDARG;
    }

    pdvi->dwMajorVersion = 4;
    pdvi->dwMinorVersion = 72;
    pdvi->dwBuildNumber = 3110;
    pdvi->dwPlatformID = 1;

    TRACE (commctrl, "%lu.%lu.%lu.%lu\n",
	   pdvi->dwMajorVersion, pdvi->dwMinorVersion,
	   pdvi->dwBuildNumber, pdvi->dwPlatformID);

    return S_OK;
}
