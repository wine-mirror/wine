/* Copyright (C) 1999 Corel Corporation (Paul Quinn) */

#ifndef __WINE_WINDOWSX_H
#define __WINE_WINDOWSX_H

#ifdef __cplusplus
extern "C" {
#endif

#define _INC_WINDOWSX
	
#define GET_WM_VSCROLL_CODE(wp, lp)         LOWORD(wp)
#define GET_WM_VSCROLL_POS(wp, lp)          HIWORD(wp)
#define GET_WM_VSCROLL_HWND(wp, lp)         (HWND)(lp)
#define GET_WM_VSCROLL_MPS(code, pos, hwnd) (WPARAM)MAKELONG(code, pos), (LONG)(hwnd)


#define GET_WM_COMMAND_ID(wp, lp)           LOWORD(wp)
#define GET_WM_COMMAND_HWND(wp, lp)         (HWND)(lp)
#define GET_WM_COMMAND_CMD(wp, lp)          HIWORD(wp)
#define GET_WM_COMMAND_MPS(id, hwnd, cmd)   (WPARAM)MAKELONG(id, cmd), (LONG)(hwnd)

#define WM_CTLCOLOR                             0x0019

#define GET_WM_CTLCOLOR_HDC(wp, lp, msg)        (HDC)(wp)
#define GET_WM_CTLCOLOR_HWND(wp, lp, msg)       (HWND)(lp)
#define GET_WM_CTLCOLOR_TYPE(wp, lp, msg)       (WORD)(msg - WM_CTLCOLORMSGBOX)
#define GET_WM_CTLCOLOR_MSG(type)               (WORD)(WM_CTLCOLORMSGBOX+(type))
#define GET_WM_CTLCOLOR_MPS(hdc, hwnd, type)    (WPARAM)(hdc), (LONG)(hwnd)

#define GET_WM_VKEYTOITEM_CODE(wp, lp)          (int)(short)LOWORD(wp)
#define GET_WM_VKEYTOITEM_ITEM(wp, lp)          HIWORD(wp)
#define GET_WM_VKEYTOITEM_HWND(wp, lp)          (HWND)(lp)
#define GET_WM_VKEYTOITEM_MPS(code, item, hwnd) (WPARAM)MAKELONG(item, code), (LONG)(hwnd)

/****** USER Macro APIs ******************************************************/

#define     GetWindowInstance(hwnd) ((HMODULE)GetWindowLong(hwnd, GWL_HINSTANCE))

#define     GetWindowStyle(hwnd)    ((DWORD)GetWindowLong(hwnd, GWL_STYLE))
#define     GetWindowExStyle(hwnd)  ((DWORD)GetWindowLong(hwnd, GWL_EXSTYLE))

#define     GetWindowOwner(hwnd)    GetWindow(hwnd, GW_OWNER)

#define     GetFirstChild(hwnd)     GetTopWindow(hwnd)
#define     GetFirstSibling(hwnd)   GetWindow(hwnd, GW_HWNDFIRST)
#define     GetLastSibling(hwnd)    GetWindow(hwnd, GW_HWNDLAST)
#define     GetNextSibling(hwnd)    GetWindow(hwnd, GW_HWNDNEXT)
#define     GetPrevSibling(hwnd)    GetWindow(hwnd, GW_HWNDPREV)

#define     GetWindowID(hwnd)       GetDlgCtrlID(hwnd)

#define     SetWindowRedraw(hwnd, fRedraw)  \
	        ((void)SendMessage(hwnd, WM_SETREDRAW, (WPARAM)(BOOL)(fRedraw), 0L))
#define     SubclassWindow(hwnd, lpfn)      \
		((WNDPROC)SetWindowLong((hwnd), GWL_WNDPROC, (LPARAM)(WNDPROC)(lpfn)))

#define     IsMinimized(hwnd)        IsIconic(hwnd)
#define     IsMaximized(hwnd)        IsZoomed(hwnd)
#define     IsRestored(hwnd)    ((GetWindowStyle(hwnd) & (WS_MINIMIZE | WS_MAXIMIZE)) == 0L)
#define     SetWindowFont(hwnd, hfont, fRedraw) \
		FORWARD_WM_SETFONT((hwnd), (hfont), (fRedraw), SendMessage)

#define     GetWindowFont(hwnd)      FORWARD_WM_GETFONT((hwnd), SendMessage)

#define     MapWindowRect(hwndFrom, hwndTo, lprc) \
                MapWindowPoints((hwndFrom), (hwndTo), (POINT *)(lprc), 2)

#define     IsLButtonDown()  (GetKeyState(VK_LBUTTON) < 0)
#define     IsRButtonDown()  (GetKeyState(VK_RBUTTON) < 0)
#define     IsMButtonDown()  (GetKeyState(VK_MBUTTON) < 0)

#define     SubclassDialog(hwndDlg, lpfn) \
		((DLGPROC)SetWindowLong(hwndDlg, DWL_DLGPROC, (LPARAM)(DLGPROC)(lpfn)))

#define     DeletePen(hpen)      DeleteObject((HGDIOBJ)(HPEN)(hpen))
#define     SelectPen(hdc, hpen)    ((HPEN)SelectObject((hdc), (HGDIOBJ)(HPEN)(hpen)))
#define     GetStockPen(i)       ((HPEN)GetStockObject(i))

#define     DeleteBrush(hbr)     DeleteObject((HGDIOBJ)(HBRUSH)(hbr))
#define     SelectBrush(hdc, hbr)   ((HBRUSH)SelectObject((hdc), (HGDIOBJ)(HBRUSH)(hbr)))
#define     GetStockBrush(i)     ((HBRUSH)GetStockObject(i))

#define     DeletePalette(hpal)     DeleteObject((HGDIOBJ)(HPALETTE)(hpal))

#define     DeleteFont(hfont)       DeleteObject((HGDIOBJ)(HFONT)(hfont))
#define     SelectFont(hdc, hfont)  ((HFONT)SelectObject((hdc), (HGDIOBJ)(HFONT) (hfont)))
#define     GetStockFont(i)         ((HFONT)GetStockObject(i))
#define     DeleteBitmap(hbm)       DeleteObject((HGDIOBJ)(HBITMAP)(hbm))
#define     SelectBitmap(hdc, hbm)  ((HBITMAP)SelectObject((hdc), (HGDIOBJ)(HBITMAP)(hbm)))
#define     InsetRect(lprc, dx, dy) InflateRect((lprc), -(dx), -(dy))

/* COMBOBOX Message APIs */
#define ComboBox_GetCount(hwndCtl)            \
		((int)(DWORD)SendMessage((hwndCtl), CB_GETCOUNT, 0L, 0L))

#define ComboBox_GetCurSel(hwndCtl)           \
		((int)(DWORD)SendMessage((hwndCtl), CB_GETCURSEL, 0L, 0L))

#define ComboBox_SetCurSel(hwndCtl, index)    \
		((int)(DWORD)SendMessage((hwndCtl), CB_SETCURSEL, (WPARAM)(int)(index), 0L))

#define ComboBox_GetLBTextLen(hwndCtl, index) \
		((int)(DWORD)SendMessage((hwndCtl), CB_GETLBTEXTLEN, (WPARAM)(int)(index), 0L))

#define ComboBox_DeleteString(hwndCtl, index) \
		((int)(DWORD)SendMessage((hwndCtl), CB_DELETESTRING, (WPARAM)(int)(index), 0L))

#define ComboBox_FindString(hwndCtl, indexStart, lpszFind) \
		((int)(DWORD)SendMessage((hwndCtl), CB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind))) 

#define ComboBox_ResetContent(hwndCtl)      \
		((int)(DWORD)SendMessage((hwndCtl), CB_RESETCONTENT, 0L, 0L))
		 
#define ComboBox_AddString(hwndCtl, lpsz)   \
		((int)(DWORD)SendMessage((hwndCtl), CB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))    
#define ComboBox_GetLBTextLen(hwndCtl, index)       \
		((int)(DWORD)SendMessage((hwndCtl), CB_GETLBTEXTLEN, (WPARAM)(int)(index), 0L))
#define ComboBox_GetLBText(hwndCtl, index, lpszBuffer) \
		((int)(DWORD)SendMessage((hwndCtl), CB_GETLBTEXT, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpszBuffer)))

#define ComboBox_GetDroppedState(hwndCtl)             \
           ((BOOL)(DWORD)SendMessage((hwndCtl), CB_GETDROPPEDSTATE, 0L, 0L))
#define ComboBox_GetDroppedControlRect(hwndCtl, lprc) \
	   ((void)SendMessage((hwndCtl), CB_GETDROPPEDCONTROLRECT, 0L, (LPARAM)(RECT *)(lprc)))

/****** ListBox control message APIs *****************************************/

#define ListBox_Enable(hwndCtl, fEnable)            EnableWindow((hwndCtl), (fEnable))
#define ListBox_GetCount(hwndCtl)          \
	   ((int)(DWORD)SendMessage((hwndCtl), LB_GETCOUNT, 0L, 0L))
#define ListBox_ResetContent(hwndCtl)      \
	   ((BOOL)(DWORD)SendMessage((hwndCtl), LB_RESETCONTENT, 0L, 0L))

#define ListBox_GetItemData(hwndCtl, index)     \
	((LRESULT)(DWORD)SendMessage((hwndCtl), LB_GETITEMDATA, (WPARAM)(int)(index), 0L))

#define ListBox_GetCurSel(hwndCtl)        \
	((int)(DWORD)SendMessage((hwndCtl), LB_GETCURSEL, 0L, 0L))
#define ListBox_SetCurSel(hwndCtl, index) \
	((int)(DWORD)SendMessage((hwndCtl), LB_SETCURSEL, (WPARAM)(int)(index), 0L))

#define ListBox_AddString(hwndCtl, lpsz)           \
	((int)(DWORD)SendMessage((hwndCtl), LB_ADDSTRING, 0L, (LPARAM)(LPCTSTR)(lpsz)))
#define ListBox_InsertString(hwndCtl, index, lpsz) \
	((int)(DWORD)SendMessage((hwndCtl), LB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(LPCTSTR)(lpsz)))
	 
#define ListBox_AddItemData(hwndCtl, data) \
	((int)(DWORD)SendMessage((hwndCtl), LB_ADDSTRING, 0L, (LPARAM)(data)))
#define ListBox_InsertItemData(hwndCtl, index, data) \
	((int)(DWORD)SendMessage((hwndCtl), LB_INSERTSTRING, (WPARAM)(int)(index), (LPARAM)(data)))

#define ListBox_FindString(hwndCtl, indexStart, lpszFind) \
	((int)(DWORD)SendMessage((hwndCtl), LB_FINDSTRING, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))
#define ListBox_FindStringExact(hwndCtl, indexStart, lpszFind) \
	((int)(DWORD)SendMessage((hwndCtl), LB_FINDSTRINGEXACT, (WPARAM)(int)(indexStart), (LPARAM)(LPCTSTR)(lpszFind)))

#define Edit_LineFromChar(hwndCtl, ich)   \
	((int)(DWORD)SendMessage((hwndCtl), EM_LINEFROMCHAR, (WPARAM)(int)(ich), 0L))
#define Edit_LineIndex(hwndCtl, line)     \
	((int)(DWORD)SendMessage((hwndCtl), EM_LINEINDEX, (WPARAM)(int)(line), 0L))
#define Edit_LineLength(hwndCtl, line)    \
	((int)(DWORD)SendMessage((hwndCtl), EM_LINELENGTH, (WPARAM)(int)(line), 0L))
	 
/****** Edit control message APIs ********************************************/

#define Edit_SetSel(hwndCtl, ichStart, ichEnd)   \
	((void)SendMessage((hwndCtl), EM_SETSEL, (ichStart), (ichEnd)))

#define Edit_GetText(hwndCtl, lpch, cchMax)     GetWindowText((hwndCtl), (lpch), (cchMax))
#define Edit_GetTextLength(hwndCtl)             GetWindowTextLength(hwndCtl)
#define Edit_SetText(hwndCtl, lpsz)             SetWindowText((hwndCtl), (lpsz))
	 
#define Edit_GetModify(hwndCtl)            \
	((BOOL)(DWORD)SendMessage((hwndCtl), EM_GETMODIFY, 0L, 0L))
#define Edit_SetModify(hwndCtl, fModified) \
	((void)SendMessage((hwndCtl), EM_SETMODIFY, (WPARAM)(UINT)(fModified), 0L))
	 
/* void Cls_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT * lpMeasureItem) */
#define HANDLE_WM_MEASUREITEM(hwnd, wParam, lParam, fn) \
	    ((fn)((hwnd), (MEASUREITEMSTRUCT *)(lParam)), 0L)
#define FORWARD_WM_MEASUREITEM(hwnd, lpMeasureItem, fn) \
	    (void)(fn)((hwnd), WM_MEASUREITEM, (WPARAM)(((MEASUREITEMSTRUCT *)lpMeasureItem)->CtlID), (LPARAM)(MEASUREITEMSTRUCT *)(lpMeasureItem))

/* void Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) */
#define HANDLE_WM_COMMAND(hwnd, wParam, lParam, fn) \
	    ((fn)((hwnd), (int)(LOWORD(wParam)), (HWND)(lParam), (UINT)HIWORD(wParam)), 0L)
#define FORWARD_WM_COMMAND(hwnd, id, hwndCtl, codeNotify, fn) \
	    (void)(fn)((hwnd), WM_COMMAND, MAKEWPARAM((UINT)(id),(UINT)(codeNotify)), (LPARAM)(HWND)(hwndCtl))

/* void Cls_OnTimer(HWND hwnd, UINT id) */
#define HANDLE_WM_TIMER(hwnd, wParam, lParam, fn) ((fn)((hwnd), (UINT)(wParam)), 0L)
#define FORWARD_WM_TIMER(hwnd, id, fn) (void)(fn)((hwnd), WM_TIMER, (WPARAM)(UINT)(id), 0L)

/* void Cls_OnInitMenuPopup(HWND hwnd, HMENU hMenu, UINT item, BOOL fSystemMenu) */
#define HANDLE_WM_INITMENUPOPUP(hwnd, wParam, lParam, fn) \
        ((fn)((hwnd), (HMENU)(wParam), (UINT)LOWORD(lParam), (BOOL)HIWORD(lParam)), 0L)
#define FORWARD_WM_INITMENUPOPUP(hwnd, hMenu, item, fSystemMenu, fn) \
        (void)(fn)((hwnd), WM_INITMENUPOPUP, (WPARAM)(HMENU)(hMenu), MAKELPARAM((item),(fSystemMenu)))

/* UINT Cls_OnNCHitTest(HWND hwnd, int x, int y) */
#define HANDLE_WM_NCHITTEST(hwnd, wParam, lParam, fn) \
	(LRESULT)(DWORD)(UINT)(fn)((hwnd), (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam))
#define FORWARD_WM_NCHITTEST(hwnd, x, y, fn) \
	(UINT)(DWORD)(fn)((hwnd), WM_NCHITTEST, 0L, MAKELPARAM((x), (y)))

/* HFONT Cls_OnGetFont(HWND hwnd) */
#define HANDLE_WM_GETFONT(hwnd, wParam, lParam, fn)  (LRESULT)(DWORD)(UINT)(HFONT)(fn)(hwnd)
#define FORWARD_WM_GETFONT(hwnd, fn)  (HFONT)(UINT)(DWORD)(fn)((hwnd), WM_GETFONT, 0L, 0L)

/* void Cls_OnSetFont(HWND hwndCtl, HFONT hfont, BOOL fRedraw) */
#define HANDLE_WM_SETFONT(hwnd, wParam, lParam, fn) \
	    ((fn)((hwnd), (HFONT)(wParam), (BOOL)(lParam)), 0L)
#define FORWARD_WM_SETFONT(hwnd, hfont, fRedraw, fn) \
			 (void)(fn)((hwnd), WM_SETFONT, (WPARAM)(HFONT)(hfont), (LPARAM)(BOOL)(fRedraw))

/* void Cls_OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpMinMaxInfo) */
#define HANDLE_WM_GETMINMAXINFO(hwnd, wParam, lParam, fn) \
			 ((fn)((hwnd), (LPMINMAXINFO)(lParam)), 0L)
#define FORWARD_WM_GETMINMAXINFO(hwnd, lpMinMaxInfo, fn) \
			(void)(fn)((hwnd), WM_GETMINMAXINFO, 0L, (LPARAM)(LPMINMAXINFO)(lpMinMaxInfo))

/* void Cls_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT lpDrawItem) */
#define HANDLE_WM_DRAWITEM(hwnd, wParam, lParam, fn) \
	    ((fn)((hwnd), (const DRAWITEMSTRUCT *)(lParam)), 0L)
#define FORWARD_WM_DRAWITEM(hwnd, lpDrawItem, fn) \
        (void)(fn)((hwnd), WM_DRAWITEM, (WPARAM)(((const DRAWITEMSTRUCT *)lpDrawItem)->CtlID), (LPARAM)(const DRAWITEMSTRUCT *)(lpDrawItem))
				 
/****** C runtime porting macros ****************************************/

#define _ncalloc    calloc
#define _nexpand    _expand
#define _ffree      free
#define _fmalloc    malloc
#define _fmemccpy   _memccpy
#define _fmemchr    memchr
#define _fmemcmp    memcmp
#define _fmemcpy    memcpy
#define _fmemicmp   _memicmp
#define _fmemmove   memmove
#define _fmemset    memset
#define _fmsize     _msize
#define _frealloc   realloc
#define _fstrcat    strcat
#define _fstrchr    strchr
#define _fstrcmp    strcmp
#define _fstrcpy    strcpy
#define _fstrcspn   strcspn
#define _fstrdup    _strdup
#define _fstricmp   _stricmp
#define _fstrlen    strlen
#define _fstrlwr    _strlwr
#define _fstrncat   strncat
#define _fstrncmp   strncmp
#define _fstrncpy   strncpy
#define _fstrnicmp  _strnicmp
#define _fstrnset   _strnset
#define _fstrpbrk   strpbrk
#define _fstrrchr   strrchr
#define _fstrrev    _strrev
#define _fstrset    _strset
#define _fstrspn    strspn
#define _fstrstr    strstr
#define _fstrtok    strtok
#define _fstrupr    _strupr
#define _nfree      free
#define _nmalloc    malloc
#define _nmsize     _msize
#define _nrealloc   realloc
#define _nstrdup    _strdup
#define hmemcpy     MoveMemory

#ifdef __cplusplus
}
#endif
				
#endif

