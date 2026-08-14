/*
 * Copyright (C) the Wine project
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINE_WINUSER16_H
#define __WINE_WINE_WINUSER16_H

#include <wine/wingdi16.h> /* wingdi.h needed for COLORREF */
#include <winuser.h> /* winuser.h needed for MSGBOXCALLBACK */

#pragma pack(push,1)

typedef struct tagCOMSTAT16
{
    BYTE   status;
    UINT16 cbInQue;
    UINT16 cbOutQue;
} COMSTAT16,*LPCOMSTAT16;

typedef struct tagDCB16
{
    BYTE   Id;
    UINT16 BaudRate;
    BYTE   ByteSize;
    BYTE   Parity;
    BYTE   StopBits;
    UINT16 RlsTimeout;
    UINT16 CtsTimeout;
    UINT16 DsrTimeout;

    unsigned fBinary        :1;
    unsigned fRtsDisable    :1;
    unsigned fParity        :1;
    unsigned fOutxCtsFlow   :1;
    unsigned fOutxDsrFlow   :1;
    unsigned fDummy         :2;
    unsigned fDtrDisable    :1;

    unsigned fOutX          :1;
    unsigned fInX           :1;
    unsigned fPeChar        :1;
    unsigned fNull          :1;
    unsigned fChEvt         :1;
    unsigned fDtrflow       :1;
    unsigned fRtsflow       :1;
    unsigned fDummy2        :1;

    CHAR   XonChar;
    CHAR   XoffChar;
    UINT16 XonLim;
    UINT16 XoffLim;
    CHAR   PeChar;
    CHAR   EofChar;
    CHAR   EvtChar;
    UINT16 TxDelay;
} DCB16, *LPDCB16;


typedef struct
{
   LPARAM   lParam;
   WPARAM16 wParam;
   UINT16   message;
   HWND16   hwnd;
} CWPSTRUCT16, *LPCWPSTRUCT16;

typedef struct
{
  LRESULT       lResult;
  LPARAM        lParam;
  WPARAM16      wParam;
  DWORD         message;
  HWND16        hwnd;
} CWPRETSTRUCT16, *LPCWPRETSTRUCT16;

  /* SetWindowPlacement() struct */
typedef struct
{
    UINT16   length;
    UINT16   flags;
    UINT16   showCmd;
    POINT16  ptMinPosition;
    POINT16  ptMaxPosition;
    RECT16   rcNormalPosition;
} WINDOWPLACEMENT16, *LPWINDOWPLACEMENT16;

/****** Window classes ******/

typedef struct
{
    UINT16      style;
    WNDPROC16   lpfnWndProc;
    INT16       cbClsExtra;
    INT16       cbWndExtra;
    HANDLE16    hInstance;
    HICON16     hIcon;
    HCURSOR16   hCursor;
    HBRUSH16    hbrBackground;
    SEGPTR      lpszMenuName;
    SEGPTR      lpszClassName;
} WNDCLASS16, *LPWNDCLASS16;

typedef struct
{
    UINT      cbSize;
    UINT      style;
    WNDPROC16   lpfnWndProc;
    INT16       cbClsExtra;
    INT16       cbWndExtra;
    HANDLE16    hInstance;
    HICON16     hIcon;
    HCURSOR16   hCursor;
    HBRUSH16    hbrBackground;
    SEGPTR      lpszMenuName;
    SEGPTR      lpszClassName;
    HICON16     hIconSm;
} WNDCLASSEX16, *LPWNDCLASSEX16;

typedef struct
{
    HWND16    hwnd;
    UINT16    message;
    WPARAM16  wParam;
    LPARAM    lParam;
    DWORD     time;
    POINT16   pt;
} MSG16, *LPMSG16;

typedef struct
{
    MSG16 msg;
    WORD wParamHigh;
} MSG32_16, *LPMSG16_32;

/* Cursors / Icons */

typedef struct tagCURSORICONINFO
{
    POINT16 ptHotSpot;
    WORD    nWidth;
    WORD    nHeight;
    WORD    nWidthBytes;
    BYTE    bPlanes;
    BYTE    bBitsPerPixel;
} CURSORICONINFO;

typedef struct {
	BOOL16		fIcon;
	WORD		xHotspot;
	WORD		yHotspot;
	HBITMAP16	hbmMask;
	HBITMAP16	hbmColor;
} ICONINFO16,*LPICONINFO16;

typedef struct
{
    BYTE   fVirt;
    WORD   key;
    WORD   cmd;
} ACCEL16, *LPACCEL16;

/* FIXME: not sure this one is correct */
typedef struct {
  UINT16    cbSize;
  UINT16    fMask;
  UINT16    fType;
  UINT16    fState;
  UINT16    wID;
  HMENU16   hSubMenu;
  HBITMAP16 hbmpChecked;
  HBITMAP16 hbmpUnchecked;
  DWORD     dwItemData;
  SEGPTR    dwTypeData;
  UINT16    cch;
} MENUITEMINFO16, *LPMENUITEMINFO16;

/* DrawState defines ... */
typedef BOOL16 (CALLBACK *DRAWSTATEPROC16)(HDC16,LPARAM,WPARAM16,INT16,INT16);

/* Listbox messages */
#define LB_ADDSTRING16           (WM_USER+1)
#define LB_INSERTSTRING16        (WM_USER+2)
#define LB_DELETESTRING16        (WM_USER+3)
#define LB_SELITEMRANGEEX16      (WM_USER+4)
#define LB_RESETCONTENT16        (WM_USER+5)
#define LB_SETSEL16              (WM_USER+6)
#define LB_SETCURSEL16           (WM_USER+7)
#define LB_GETSEL16              (WM_USER+8)
#define LB_GETCURSEL16           (WM_USER+9)
#define LB_GETTEXT16             (WM_USER+10)
#define LB_GETTEXTLEN16          (WM_USER+11)
#define LB_GETCOUNT16            (WM_USER+12)
#define LB_SELECTSTRING16        (WM_USER+13)
#define LB_DIR16                 (WM_USER+14)
#define LB_GETTOPINDEX16         (WM_USER+15)
#define LB_FINDSTRING16          (WM_USER+16)
#define LB_GETSELCOUNT16         (WM_USER+17)
#define LB_GETSELITEMS16         (WM_USER+18)
#define LB_SETTABSTOPS16         (WM_USER+19)
#define LB_GETHORIZONTALEXTENT16 (WM_USER+20)
#define LB_SETHORIZONTALEXTENT16 (WM_USER+21)
#define LB_SETCOLUMNWIDTH16      (WM_USER+22)
#define LB_ADDFILE16             (WM_USER+23)
#define LB_SETTOPINDEX16         (WM_USER+24)
#define LB_GETITEMRECT16         (WM_USER+25)
#define LB_GETITEMDATA16         (WM_USER+26)
#define LB_SETITEMDATA16         (WM_USER+27)
#define LB_SELITEMRANGE16        (WM_USER+28)
#define LB_SETANCHORINDEX16      (WM_USER+29)
#define LB_GETANCHORINDEX16      (WM_USER+30)
#define LB_SETCARETINDEX16       (WM_USER+31)
#define LB_GETCARETINDEX16       (WM_USER+32)
#define LB_SETITEMHEIGHT16       (WM_USER+33)
#define LB_GETITEMHEIGHT16       (WM_USER+34)
#define LB_FINDSTRINGEXACT16     (WM_USER+35)
#define LB_CARETON16             (WM_USER+36)
#define LB_CARETOFF16            (WM_USER+37)

/* Combo box messages */
#define CB_GETEDITSEL16            (WM_USER+0)
#define CB_LIMITTEXT16             (WM_USER+1)
#define CB_SETEDITSEL16            (WM_USER+2)
#define CB_ADDSTRING16             (WM_USER+3)
#define CB_DELETESTRING16          (WM_USER+4)
#define CB_DIR16                   (WM_USER+5)
#define CB_GETCOUNT16              (WM_USER+6)
#define CB_GETCURSEL16             (WM_USER+7)
#define CB_GETLBTEXT16             (WM_USER+8)
#define CB_GETLBTEXTLEN16          (WM_USER+9)
#define CB_INSERTSTRING16          (WM_USER+10)
#define CB_RESETCONTENT16          (WM_USER+11)
#define CB_FINDSTRING16            (WM_USER+12)
#define CB_SELECTSTRING16          (WM_USER+13)
#define CB_SETCURSEL16             (WM_USER+14)
#define CB_SHOWDROPDOWN16          (WM_USER+15)
#define CB_GETITEMDATA16           (WM_USER+16)
#define CB_SETITEMDATA16           (WM_USER+17)
#define CB_GETDROPPEDCONTROLRECT16 (WM_USER+18)
#define CB_SETITEMHEIGHT16         (WM_USER+19)
#define CB_GETITEMHEIGHT16         (WM_USER+20)
#define CB_SETEXTENDEDUI16         (WM_USER+21)
#define CB_GETEXTENDEDUI16         (WM_USER+22)
#define CB_GETDROPPEDSTATE16       (WM_USER+23)
#define CB_FINDSTRINGEXACT16       (WM_USER+24)

typedef struct /* not sure if the 16bit version is correct */
{
    UINT	cbSize;
    HWND16	hwndOwner;
    HINSTANCE16	hInstance;
    SEGPTR	lpszText;
    SEGPTR	lpszCaption;
    DWORD	dwStyle;
    SEGPTR	lpszIcon;
    DWORD	dwContextHelpId;
    MSGBOXCALLBACK	lpfnMsgBoxCallback;
    DWORD	dwLanguageId;
} MSGBOXPARAMS16,*LPMSGBOXPARAMS16;

  /* Windows */

typedef struct
{
    SEGPTR      lpCreateParams;
    HINSTANCE16 hInstance;
    HMENU16     hMenu;
    HWND16      hwndParent;
    INT16       cy;
    INT16       cx;
    INT16       y;
    INT16       x;
    LONG        style;
    SEGPTR      lpszName;
    SEGPTR      lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCT16, *LPCREATESTRUCT16;

typedef struct
{
    HDC16   hdc;
    BOOL16  fErase;
    RECT16  rcPaint;
    BOOL16  fRestore;
    BOOL16  fIncUpdate;
    BYTE    rgbReserved[16];
} PAINTSTRUCT16, *LPPAINTSTRUCT16;

typedef struct
{
    HMENU16   hWindowMenu;
    UINT16    idFirstChild;
} CLIENTCREATESTRUCT16, *LPCLIENTCREATESTRUCT16;

typedef struct
{
    SEGPTR       szClass;
    SEGPTR       szTitle;
    HINSTANCE16  hOwner;
    INT16        x;
    INT16        y;
    INT16        cx;
    INT16        cy;
    DWORD        style;
    LPARAM       lParam;
} MDICREATESTRUCT16, *LPMDICREATESTRUCT16;

  /* WM_GETMINMAXINFO struct */
typedef struct
{
    POINT16   ptReserved;
    POINT16   ptMaxSize;
    POINT16   ptMaxPosition;
    POINT16   ptMinTrackSize;
    POINT16   ptMaxTrackSize;
} MINMAXINFO16;

  /* WM_WINDOWPOSCHANGING/CHANGED struct */
typedef struct tagWINDOWPOS16
{
    HWND16  hwnd;
    HWND16  hwndInsertAfter;
    INT16   x;
    INT16   y;
    INT16   cx;
    INT16   cy;
    UINT16  flags;
} WINDOWPOS16, *LPWINDOWPOS16;

  /* WM_NCCALCSIZE parameter structure */
typedef struct
{
    RECT16  rgrc[3];
    SEGPTR  lppos;
} NCCALCSIZE_PARAMS16, *LPNCCALCSIZE_PARAMS16;

typedef struct {
	UINT16		cbSize;
	INT16		iBorderWidth;
	INT16		iScrollWidth;
	INT16		iScrollHeight;
	INT16		iCaptionWidth;
	INT16		iCaptionHeight;
	LOGFONT16	lfCaptionFont;
	INT16		iSmCaptionWidth;
	INT16		iSmCaptionHeight;
	LOGFONT16	lfSmCaptionFont;
	INT16		iMenuWidth;
	INT16		iMenuHeight;
	LOGFONT16	lfMenuFont;
	LOGFONT16	lfStatusFont;
	LOGFONT16	lfMessageFont;
} NONCLIENTMETRICS16,*LPNONCLIENTMETRICS16;

  /* Journalling hook structure */

typedef struct
{
    UINT16  message;
    UINT16  paramL;
    UINT16  paramH;
    DWORD   time;
} EVENTMSG16, *LPEVENTMSG16;

  /* Mouse hook structure */

typedef struct
{
    POINT16 pt;
    HWND16  hwnd;
    UINT16  wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT16, *LPMOUSEHOOKSTRUCT16;

  /* Hardware hook structure */

typedef struct
{
    HWND16    hWnd;
    UINT16    wMessage;
    WPARAM16  wParam;
    LPARAM    lParam;
} HARDWAREHOOKSTRUCT16, *LPHARDWAREHOOKSTRUCT16;

/* Scrollbar messages */
#define SBM_SETPOS16             (WM_USER+0)
#define SBM_GETPOS16             (WM_USER+1)
#define SBM_SETRANGE16           (WM_USER+2)
#define SBM_GETRANGE16           (WM_USER+3)
#define SBM_ENABLE_ARROWS16      (WM_USER+4)

  /* CBT hook structures */

typedef struct
{
    CREATESTRUCT16  *lpcs;
    HWND16           hwndInsertAfter;
} CBT_CREATEWND16, *LPCBT_CREATEWND16;

typedef struct
{
    BOOL16    fMouse;
    HWND16    hWndActive;
} CBTACTIVATESTRUCT16, *LPCBTACTIVATESTRUCT16;

  /* Debug hook structure */

typedef struct
{
    HMODULE16   hModuleHook;
    LPARAM      reserved;
    LPARAM      lParam;
    WPARAM16    wParam;
    INT16       code;
} DEBUGHOOKINFO16, *LPDEBUGHOOKINFO16;

#define GETMAXLPT	8
#define GETMAXCOM	9
#define GETBASEIRQ	10

/* GetFreeSystemResources() parameters */

#define GFSR_SYSTEMRESOURCES   0x0000
#define GFSR_GDIRESOURCES      0x0001
#define GFSR_USERRESOURCES     0x0002

/* CreateWindow() coordinates */
#define CW_USEDEFAULT16 ((INT16)0x8000)

/* Win16 button control messages */
#define BM_GETCHECK16          (WM_USER+0)
#define BM_SETCHECK16          (WM_USER+1)
#define BM_GETSTATE16          (WM_USER+2)
#define BM_SETSTATE16          (WM_USER+3)
#define BM_SETSTYLE16          (WM_USER+4)

/* Static Control Messages */
#define STM_SETICON16       (WM_USER+0)
#define STM_GETICON16       (WM_USER+1)

/* Edit control messages */
#define EM_GETSEL16                (WM_USER+0)
#define EM_SETSEL16                (WM_USER+1)
#define EM_GETRECT16               (WM_USER+2)
#define EM_SETRECT16               (WM_USER+3)
#define EM_SETRECTNP16             (WM_USER+4)
#define EM_SCROLL16                (WM_USER+5)
#define EM_LINESCROLL16            (WM_USER+6)
#define EM_SCROLLCARET16           (WM_USER+7)
#define EM_GETMODIFY16             (WM_USER+8)
#define EM_SETMODIFY16             (WM_USER+9)
#define EM_GETLINECOUNT16          (WM_USER+10)
#define EM_LINEINDEX16             (WM_USER+11)
#define EM_SETHANDLE16             (WM_USER+12)
#define EM_GETHANDLE16             (WM_USER+13)
#define EM_GETTHUMB16              (WM_USER+14)
#define EM_LINELENGTH16            (WM_USER+17)
#define EM_REPLACESEL16            (WM_USER+18)
#define EM_GETLINE16               (WM_USER+20)
#define EM_LIMITTEXT16             (WM_USER+21)
#define EM_CANUNDO16               (WM_USER+22)
#define EM_UNDO16                  (WM_USER+23)
#define EM_FMTLINES16              (WM_USER+24)
#define EM_LINEFROMCHAR16          (WM_USER+25)
#define EM_SETTABSTOPS16           (WM_USER+27)
#define EM_SETPASSWORDCHAR16       (WM_USER+28)
#define EM_EMPTYUNDOBUFFER16       (WM_USER+29)
#define EM_GETFIRSTVISIBLELINE16   (WM_USER+30)
#define EM_SETREADONLY16           (WM_USER+31)
#define EM_SETWORDBREAKPROC16      (WM_USER+32)
#define EM_GETWORDBREAKPROC16      (WM_USER+33)
#define EM_GETPASSWORDCHAR16       (WM_USER+34)

typedef struct
{
    UINT16      CtlType;
    UINT16      CtlID;
    UINT16      itemID;
    UINT16      itemAction;
    UINT16      itemState;
    HWND16      hwndItem;
    HDC16       hDC;
    RECT16      rcItem;
    DWORD       itemData;
} DRAWITEMSTRUCT16, *PDRAWITEMSTRUCT16, *LPDRAWITEMSTRUCT16;

typedef struct
{
    UINT16      CtlType;
    UINT16      CtlID;
    UINT16      itemID;
    UINT16      itemWidth;
    UINT16      itemHeight;
    DWORD       itemData;
} MEASUREITEMSTRUCT16, *PMEASUREITEMSTRUCT16, *LPMEASUREITEMSTRUCT16;

typedef struct
{
    UINT16     CtlType;
    UINT16     CtlID;
    UINT16     itemID;
    HWND16     hwndItem;
    DWORD      itemData;
} DELETEITEMSTRUCT16, *LPDELETEITEMSTRUCT16;

typedef struct
{
    UINT16      CtlType;
    UINT16      CtlID;
    HWND16      hwndItem;
    UINT16      itemID1;
    DWORD       itemData1;
    UINT16      itemID2;
    DWORD       itemData2;
} COMPAREITEMSTRUCT16, *LPCOMPAREITEMSTRUCT16;

/* DragObject stuff */

typedef struct
{
    HWND16     hWnd;
    HANDLE16   hScope;
    WORD       wFlags;
    HANDLE16   hList;
    HANDLE16   hOfStruct;
    POINT16    pt;
    LONG       l;
} DRAGINFO16, *LPDRAGINFO16;

/* undocumented */
typedef struct tagCOPYDATASTRUCT16 {
    DWORD dwData;
    DWORD cbData;
    SEGPTR lpData;
} COPYDATASTRUCT16, *PCOPYDATASTRUCT16;

#define DRAGOBJ_PROGRAM    0x0001
#define DRAGOBJ_DATA       0x0002
#define DRAGOBJ_DIRECTORY  0x0004
#define DRAGOBJ_MULTIPLE   0x0008
#define DRAGOBJ_EXTERNAL   0x8000

#define DRAG_PRINT 0x544E5250
#define DRAG_FILE  0x454C4946

#pragma pack(pop)

/* WM_COMMNOTIFY flags */
#define CN_RECEIVE	0x0001
#define CN_TRANSMIT	0x0002
#define CN_EVENT	0x0004

BOOL16      WINAPI CheckMenuRadioItem16(HMENU16,UINT16,UINT16,UINT16,UINT16);
HICON16     WINAPI CopyImage16(HANDLE16,UINT16,INT16,INT16,UINT16);
HICON16     WINAPI CreateIconFromResource16(LPBYTE,UINT16,BOOL16,DWORD);
BOOL16      WINAPI EnumChildWindows16(HWND16,WNDENUMPROC16,LPARAM);
INT16       WINAPI EnumProps16(HWND16,PROPENUMPROC16);
BOOL16      WINAPI EnumWindows16(WNDENUMPROC16,LPARAM);
DWORD       WINAPI GetAppCompatFlags16(HTASK16);
INT16       WINAPI GetKBCodePage16(void);
INT16       WINAPI GetKeyboardType16(INT16);
INT16       WINAPI GetKeyNameText16(LONG,LPSTR,INT16);
INT16       WINAPI GetWindowRgn16(HWND16,HRGN16);
BOOL16      WINAPI IsWindow16(HWND16);
INT16       WINAPI LookupIconIdFromDirectory16(LPBYTE,BOOL16);
UINT16      WINAPI MapVirtualKey16(UINT16,UINT16);
FARPROC16   WINAPI SetWindowsHook16(INT16,HOOKPROC16);
HHOOK       WINAPI SetWindowsHookEx16(INT16,HOOKPROC16,HINSTANCE16,HTASK16);
BOOL16      WINAPI UnhookWindowsHook16(INT16,HOOKPROC16);
BOOL16      WINAPI UnhookWindowsHookEx16(HHOOK);
VOID        WINAPI CalcChildScroll16(HWND16,WORD);
VOID        WINAPI CascadeChildWindows16(HWND16,WORD);
INT16       WINAPI CloseComm16(INT16);
HGLOBAL16   WINAPI CreateCursorIconIndirect16(HINSTANCE16,CURSORICONINFO*,
                                            LPCVOID,LPCVOID);
BOOL16      WINAPI DCHook16(HDC16,WORD,DWORD,LPARAM);
BOOL16      WINAPI DlgDirSelect16(HWND16,LPSTR,INT16);
BOOL16      WINAPI DlgDirSelectComboBox16(HWND16,LPSTR,INT16);
DWORD       WINAPI DumpIcon16(SEGPTR,WORD*,SEGPTR*,SEGPTR*);
BOOL16      WINAPI EnableCommNotification16(INT16,HWND16,INT16,INT16);
BOOL16      WINAPI EnableHardwareInput16(BOOL16);
VOID        WINAPI FillWindow16(HWND16,HWND16,HDC16,HBRUSH16);
INT16       WINAPI FlushComm16(INT16,INT16);
UINT16      WINAPI GetCommEventMask16(INT16,UINT16);
HBRUSH16    WINAPI GetControlBrush16(HWND16,HDC16,UINT16);
HWND16      WINAPI GetDesktopHwnd16(void);
WORD        WINAPI GetIconID16(HGLOBAL16,DWORD);
FARPROC16   WINAPI GetMouseEventProc16(void);
INT16       WINAPI InitApp16(HINSTANCE16);
BOOL16      WINAPI IsUserIdle16(void);
HGLOBAL16   WINAPI LoadCursorIconHandler16(HGLOBAL16,HMODULE16,HRSRC16);
HGLOBAL16   WINAPI LoadDIBCursorHandler16(HGLOBAL16,HMODULE16,HRSRC16);
HGLOBAL16   WINAPI LoadDIBIconHandler16(HGLOBAL16,HMODULE16,HRSRC16);
HICON16     WINAPI LoadIconHandler16(HGLOBAL16,BOOL16);
HMENU16     WINAPI LookupMenuHandle16(HMENU16,INT16);
INT16       WINAPI OpenComm16(LPCSTR,UINT16,UINT16);
VOID        WINAPI PaintRect16(HWND16,HWND16,HDC16,HBRUSH16,const RECT16*);
INT16       WINAPI ReadComm16(INT16,LPSTR,INT16);
SEGPTR      WINAPI SetCommEventMask16(INT16,UINT16);
BOOL16      WINAPI SetDeskPattern(void);
VOID        WINAPI TileChildWindows16(HWND16,WORD);
INT16       WINAPI UngetCommChar16(INT16,CHAR);
VOID        WINAPI UserYield16(void);
INT16       WINAPI WriteComm16(INT16,LPSTR,INT16);
BOOL16      WINAPI AdjustWindowRect16(LPRECT16,DWORD,BOOL16);
BOOL16      WINAPI AdjustWindowRectEx16(LPRECT16,DWORD,BOOL16,DWORD);
SEGPTR      WINAPI AnsiLower16(SEGPTR);
UINT16      WINAPI AnsiLowerBuff16(LPSTR,UINT16);
SEGPTR      WINAPI AnsiNext16(SEGPTR);
SEGPTR      WINAPI AnsiPrev16(LPCSTR,SEGPTR);
SEGPTR      WINAPI AnsiUpper16(SEGPTR);
UINT16      WINAPI AnsiUpperBuff16(LPSTR,UINT16);
BOOL16      WINAPI AnyPopup16(void);
BOOL16      WINAPI AppendMenu16(HMENU16,UINT16,UINT16,SEGPTR);
UINT16      WINAPI ArrangeIconicWindows16(HWND16);
HDWP16      WINAPI BeginDeferWindowPos16(INT16);
HDC16       WINAPI BeginPaint16(HWND16,LPPAINTSTRUCT16);
BOOL16      WINAPI BringWindowToTop16(HWND16);
BOOL16      WINAPI CallMsgFilter16(MSG16*,INT16);
BOOL16      WINAPI CallMsgFilter32_16(MSG32_16*,INT16,BOOL16);
LRESULT     WINAPI CallNextHookEx16(HHOOK,INT16,WPARAM16,LPARAM);
LRESULT     WINAPI CallWindowProc16(WNDPROC16,HWND16,UINT16,WPARAM16,LPARAM);
BOOL16      WINAPI ChangeClipboardChain16(HWND16,HWND16);
BOOL16      WINAPI ChangeMenu16(HMENU16,UINT16,SEGPTR,UINT16,UINT16);
BOOL16      WINAPI CheckDlgButton16(HWND16,INT16,UINT16);
BOOL16      WINAPI CheckMenuItem16(HMENU16,UINT16,UINT16);
BOOL16      WINAPI CheckRadioButton16(HWND16,UINT16,UINT16,UINT16);
HWND16      WINAPI ChildWindowFromPoint16(HWND16,POINT16);
HWND16      WINAPI ChildWindowFromPointEx16(HWND16,POINT16,UINT16);
INT16       WINAPI ClearCommBreak16(INT16);
VOID        WINAPI ClientToScreen16(HWND16,LPPOINT16);
BOOL16      WINAPI ClipCursor16(const RECT16*);
BOOL16      WINAPI CloseClipboard16(void);
BOOL16      WINAPI CloseWindow16(HWND16);
void        WINAPI ControlPanelInfo16(INT16, WORD, LPSTR);
HCURSOR16   WINAPI CopyCursor16(HINSTANCE16,HCURSOR16);
HICON16     WINAPI CopyIcon16(HINSTANCE16,HICON16);
BOOL16      WINAPI CopyRect16(RECT16*,const RECT16*);
INT16       WINAPI CountClipboardFormats16(void);
VOID        WINAPI CreateCaret16(HWND16,HBITMAP16,INT16,INT16);
HCURSOR16   WINAPI CreateCursor16(HINSTANCE16,INT16,INT16,INT16,INT16,LPCVOID,LPCVOID);
HWND16      WINAPI CreateDialog16(HINSTANCE16,LPCSTR,HWND16,DLGPROC16);
HWND16      WINAPI CreateDialogIndirect16(HINSTANCE16,LPCVOID,HWND16,DLGPROC16);
HWND16      WINAPI CreateDialogIndirectParam16(HINSTANCE16,LPCVOID,HWND16,
                                               DLGPROC16,LPARAM);
HWND16      WINAPI CreateDialogParam16(HINSTANCE16,LPCSTR,HWND16,DLGPROC16,LPARAM);
HICON16     WINAPI CreateIcon16(HINSTANCE16,INT16,INT16,BYTE,BYTE,LPCVOID,LPCVOID);
HICON16     WINAPI CreateIconFromResourceEx16(LPBYTE,UINT16,BOOL16,DWORD,INT16,INT16,UINT16);
HMENU16     WINAPI CreateMenu16(void);
HMENU16     WINAPI CreatePopupMenu16(void);
HWND16      WINAPI CreateWindow16(LPCSTR,LPCSTR,DWORD,INT16,INT16,INT16,INT16,
                                  HWND16,HMENU16,HINSTANCE16,LPVOID);
HWND16      WINAPI CreateWindowEx16(DWORD,LPCSTR,LPCSTR,DWORD,INT16,INT16,
                                INT16,INT16,HWND16,HMENU16,HINSTANCE16,LPVOID);
LRESULT     WINAPI DefDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
HDWP16      WINAPI DeferWindowPos16(HDWP16,HWND16,HWND16,INT16,INT16,INT16,INT16,UINT16);
LRESULT     WINAPI DefFrameProc16(HWND16,HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI DefHookProc16(INT16,WPARAM16,LPARAM,HHOOK*);
LRESULT     WINAPI DefMDIChildProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI DefWindowProc16(HWND16,UINT16,WPARAM16,LPARAM);
BOOL16      WINAPI DeleteMenu16(HMENU16,UINT16,UINT16);
VOID        WINAPI DestroyCaret16(void);
BOOL16      WINAPI DestroyCursor16(HCURSOR16);
BOOL16      WINAPI DestroyIcon16(HICON16);
BOOL16      WINAPI DestroyMenu16(HMENU16);
BOOL16      WINAPI DestroyWindow16(HWND16);
INT16       WINAPI DialogBox16(HINSTANCE16,LPCSTR,HWND16,DLGPROC16);
INT16       WINAPI DialogBoxIndirect16(HINSTANCE16,HANDLE16,HWND16,DLGPROC16);
INT16       WINAPI DialogBoxIndirectParam16(HINSTANCE16,HANDLE16,HWND16,DLGPROC16,LPARAM);
INT16       WINAPI DialogBoxParam16(HINSTANCE16,LPCSTR,HWND16,DLGPROC16,LPARAM);
LONG        WINAPI DispatchMessage16(const MSG16*);
LONG        WINAPI DispatchMessage32_16(const MSG32_16*,BOOL16);
INT16       WINAPI DlgDirList16(HWND16,LPSTR,INT16,INT16,UINT16);
INT16       WINAPI DlgDirListComboBox16(HWND16,LPSTR,INT16,INT16,UINT16);
BOOL16      WINAPI DlgDirSelectComboBoxEx16(HWND16,LPSTR,INT16,INT16);
BOOL16      WINAPI DlgDirSelectEx16(HWND16,LPSTR,INT16,INT16);
BOOL16      WINAPI DragDetect16(HWND16,POINT16);
DWORD       WINAPI DragObject16(HWND16,HWND16,UINT16,HANDLE16,WORD,HCURSOR16);
BOOL16      WINAPI DrawAnimatedRects16(HWND16,INT16,const RECT16*,const RECT16*);
BOOL16      WINAPI DrawCaption16(HWND16,HDC16,const RECT16*,UINT16);
BOOL16      WINAPI DrawCaptionTemp16(HWND16,HDC16, const RECT16*,HFONT16,HICON16,LPCSTR,UINT16);
BOOL16      WINAPI DrawEdge16(HDC16,LPRECT16,UINT16,UINT16);
void        WINAPI DrawFocusRect16(HDC16,const RECT16*);
BOOL16      WINAPI DrawFrameControl16(HDC16,LPRECT16,UINT16,UINT16);
BOOL16      WINAPI DrawIcon16(HDC16,INT16,INT16,HICON16);
BOOL16      WINAPI DrawIconEx16(HDC16,INT16,INT16,HICON16,INT16,INT16,
				UINT16,HBRUSH16,UINT16);
VOID        WINAPI DrawMenuBar16(HWND16);
INT16       WINAPI DrawText16(HDC16,LPCSTR,INT16,LPRECT16,UINT16);
BOOL16      WINAPI EmptyClipboard16(void);
BOOL16      WINAPI EnableMenuItem16(HMENU16,UINT16,UINT16);
BOOL16      WINAPI EnableScrollBar16(HWND16,INT16,UINT16);
BOOL16      WINAPI EnableWindow16(HWND16,BOOL16);
BOOL16      WINAPI EndDeferWindowPos16(HDWP16);
BOOL16      WINAPI EndDialog16(HWND16,INT16);
BOOL16      WINAPI EndPaint16(HWND16,const PAINTSTRUCT16*);
UINT16      WINAPI EnumClipboardFormats16(UINT16);
BOOL16      WINAPI EqualRect16(const RECT16*,const RECT16*);
LONG        WINAPI EscapeCommFunction16(UINT16,UINT16);
INT16       WINAPI ExcludeUpdateRgn16(HDC16,HWND16);
BOOL16      WINAPI ExitWindows16(DWORD,UINT16);
INT16       WINAPI FillRect16(HDC16,const RECT16*,HBRUSH16);
HWND16      WINAPI FindWindow16(LPCSTR,LPCSTR);
HWND16      WINAPI FindWindowEx16(HWND16,HWND16,LPCSTR,LPCSTR);
BOOL16      WINAPI FlashWindow16(HWND16,BOOL16);
DWORD       WINAPI FormatMessage16(DWORD,SEGPTR,WORD,WORD,LPSTR,WORD,LPDWORD);
INT16       WINAPI FrameRect16(HDC16,const RECT16*,HBRUSH16);
HWND16      WINAPI GetActiveWindow16(void);
INT16       WINAPI GetAsyncKeyState16(INT16);
HWND16      WINAPI GetCapture16(void);
UINT16      WINAPI GetCaretBlinkTime16(void);
VOID        WINAPI GetCaretPos16(LPPOINT16);
BOOL16      WINAPI GetClassInfo16(HINSTANCE16,SEGPTR,WNDCLASS16 *);
BOOL16      WINAPI GetClassInfoEx16(HINSTANCE16,SEGPTR,WNDCLASSEX16 *);
LONG        WINAPI GetClassLong16(HWND16,INT16);
INT16       WINAPI GetClassName16(HWND16,LPSTR,INT16);
WORD        WINAPI GetClassWord16(HWND16,INT16);
void        WINAPI GetClientRect16(HWND16,LPRECT16);
HANDLE16    WINAPI GetClipboardData16(UINT16);
INT16       WINAPI GetClipboardFormatName16(UINT16,LPSTR,INT16);
HWND16      WINAPI GetClipboardOwner16(void);
HWND16      WINAPI GetClipboardViewer16(void);
void        WINAPI GetClipCursor16(LPRECT16);
DWORD       WINAPI GetCurrentTime16(void);
HCURSOR16   WINAPI GetCursor16(void);
BOOL16      WINAPI GetCursorPos16(LPPOINT16);
HDC16       WINAPI GetDC16(HWND16);
HDC16       WINAPI GetDCEx16(HWND16,HRGN16,DWORD);
HWND16      WINAPI GetDesktopWindow16(void);
INT16       WINAPI GetDlgCtrlID16(HWND16);
HWND16      WINAPI GetDlgItem16(HWND16,INT16);
UINT16      WINAPI GetDlgItemInt16(HWND16,INT16,BOOL16*,BOOL16);
INT16       WINAPI GetDlgItemText16(HWND16,INT16,SEGPTR,UINT16);
UINT16      WINAPI GetDoubleClickTime16(void);
HWND16      WINAPI GetFocus16(void);
HWND16      WINAPI GetForegroundWindow16(void);
BOOL16      WINAPI GetIconInfo16(HICON16,LPICONINFO16);
BOOL16      WINAPI GetInputState16(void);
UINT16      WINAPI GetInternalWindowPos16(HWND16,LPRECT16,LPPOINT16);
INT16       WINAPI GetKeyboardLayoutName16(LPSTR);
INT16       WINAPI GetKeyState16(INT16);
HWND16      WINAPI GetLastActivePopup16(HWND16);
HMENU16     WINAPI GetMenu16(HWND16);
DWORD       WINAPI GetMenuContextHelpId16(HMENU16);
INT16       WINAPI GetMenuItemCount16(HMENU16);
UINT16      WINAPI GetMenuItemID16(HMENU16,INT16);
BOOL16      WINAPI GetMenuItemRect16(HWND16,HMENU16,UINT16,LPRECT16);
UINT16      WINAPI GetMenuState16(HMENU16,UINT16,UINT16);
INT16       WINAPI GetMenuString16(HMENU16,UINT16,LPSTR,INT16,UINT16);
BOOL16      WINAPI GetMessage16(MSG16*,HWND16,UINT16,UINT16);
BOOL16      WINAPI GetMessage32_16(MSG32_16*,HWND16,UINT16,UINT16,BOOL16);
HWND16      WINAPI GetNextDlgGroupItem16(HWND16,HWND16,BOOL16);
HWND16      WINAPI GetNextDlgTabItem16(HWND16,HWND16,BOOL16);
HWND16      WINAPI GetNextWindow16(HWND16,WORD);
HWND16      WINAPI GetOpenClipboardWindow16(void);
HWND16      WINAPI GetParent16(HWND16);
INT16       WINAPI GetPriorityClipboardFormat16(UINT16*,INT16);
HANDLE16    WINAPI GetProp16(HWND16,LPCSTR);
DWORD       WINAPI GetQueueStatus16(UINT16);
BOOL16      WINAPI GetScrollInfo16(HWND16,INT16,LPSCROLLINFO);
INT16       WINAPI GetScrollPos16(HWND16,INT16);
BOOL16      WINAPI GetScrollRange16(HWND16,INT16,LPINT16,LPINT16);
HWND16      WINAPI GetShellWindow16(void);
HMENU16     WINAPI GetSubMenu16(HMENU16,INT16);
COLORREF    WINAPI GetSysColor16(INT16);
HBRUSH16    WINAPI GetSysColorBrush16(INT16);
HWND16      WINAPI GetSysModalWindow16(void);
HMENU16     WINAPI GetSystemMenu16(HWND16,BOOL16);
INT16       WINAPI GetSystemMetrics16(INT16);
DWORD       WINAPI GetTabbedTextExtent16(HDC16,LPCSTR,INT16,INT16,const INT16*);
HWND16      WINAPI GetTopWindow16(HWND16);
BOOL16      WINAPI GetUpdateRect16(HWND16,LPRECT16,BOOL16);
INT16       WINAPI GetUpdateRgn16(HWND16,HRGN16,BOOL16);
HWND16      WINAPI GetWindow16(HWND16,WORD);
HDC16       WINAPI GetWindowDC16(HWND16);
LONG        WINAPI GetWindowLong16(HWND16,INT16);
BOOL16      WINAPI GetWindowPlacement16(HWND16,LPWINDOWPLACEMENT16);
void        WINAPI GetWindowRect16(HWND16,LPRECT16);
HTASK16     WINAPI GetWindowTask16(HWND16);
INT16       WINAPI GetWindowText16(HWND16,SEGPTR,INT16);
INT16       WINAPI GetWindowTextLength16(HWND16);
WORD        WINAPI GetWindowWord16(HWND16,INT16);
ATOM        WINAPI GlobalAddAtom16(LPCSTR);
ATOM        WINAPI GlobalDeleteAtom16(ATOM);
ATOM        WINAPI GlobalFindAtom16(LPCSTR);
UINT16      WINAPI GlobalGetAtomName16(ATOM,LPSTR,INT16);
VOID        WINAPI HideCaret16(HWND16);
BOOL16      WINAPI HiliteMenuItem16(HWND16,HMENU16,UINT16,UINT16);
DWORD       WINAPI IconSize16(void);
void        WINAPI InflateRect16(LPRECT16,INT16,INT16);
HQUEUE16    WINAPI InitThreadInput16(WORD,WORD);
BOOL16      WINAPI InSendMessage16(void);
BOOL16      WINAPI InsertMenu16(HMENU16,UINT16,UINT16,UINT16,SEGPTR);
BOOL16      WINAPI InsertMenuItem16(HMENU16,UINT16,BOOL16,const MENUITEMINFO16*);
BOOL16      WINAPI IntersectRect16(LPRECT16,const RECT16*,const RECT16*);
void        WINAPI InvalidateRect16(HWND16,const RECT16*,BOOL16);
void        WINAPI InvalidateRgn16(HWND16,HRGN16,BOOL16);
void        WINAPI InvertRect16(HDC16,const RECT16*);
BOOL16      WINAPI IsCharAlpha16(CHAR);
BOOL16      WINAPI IsCharAlphaNumeric16(CHAR);
BOOL16      WINAPI IsCharLower16(CHAR);
BOOL16      WINAPI IsCharUpper16(CHAR);
BOOL16      WINAPI IsChild16(HWND16,HWND16);
BOOL16      WINAPI IsClipboardFormatAvailable16(UINT16);
UINT16      WINAPI IsDlgButtonChecked16(HWND16,UINT16);
BOOL16      WINAPI IsIconic16(HWND16);
BOOL16      WINAPI IsMenu16(HMENU16);
BOOL16      WINAPI IsRectEmpty16(const RECT16*);
BOOL16      WINAPI IsWindowEnabled16(HWND16);
BOOL16      WINAPI IsWindowVisible16(HWND16);
BOOL16      WINAPI IsZoomed16(HWND16);
BOOL16      WINAPI KillSystemTimer16(HWND16,UINT16);
BOOL16      WINAPI KillTimer16(HWND16,UINT16);
HBITMAP16   WINAPI LoadBitmap16(HANDLE16,LPCSTR);
HCURSOR16   WINAPI LoadCursor16(HINSTANCE16,LPCSTR);
HICON16     WINAPI LoadIcon16(HINSTANCE16,LPCSTR);
HANDLE16    WINAPI LoadImage16(HINSTANCE16,LPCSTR,UINT16,INT16,INT16,UINT16);
HMENU16     WINAPI LoadMenu16(HINSTANCE16,LPCSTR);
HMENU16     WINAPI LoadMenuIndirect16(LPCVOID);
INT16       WINAPI LoadString16(HINSTANCE16,UINT16,LPSTR,INT16);
BOOL16      WINAPI LockWindowUpdate16(HWND16);
INT16       WINAPI LookupIconIdFromDirectoryEx16(LPBYTE,BOOL16,INT16,INT16,UINT16);
void        WINAPI MapDialogRect16(HWND16,LPRECT16);
void        WINAPI MapWindowPoints16(HWND16,HWND16,LPPOINT16,UINT16);
VOID        WINAPI MessageBeep16(UINT16);
INT16       WINAPI MessageBox16(HWND16,LPCSTR,LPCSTR,UINT16);
INT16       WINAPI MessageBoxIndirect16(LPMSGBOXPARAMS16);
BOOL16      WINAPI ModifyMenu16(HMENU16,UINT16,UINT16,UINT16,SEGPTR);
BOOL16      WINAPI MoveWindow16(HWND16,INT16,INT16,INT16,INT16,BOOL16);
void        WINAPI OffsetRect16(LPRECT16,INT16,INT16);
BOOL16      WINAPI OpenClipboard16(HWND16);
BOOL16      WINAPI OpenIcon16(HWND16);
BOOL16      WINAPI PeekMessage16(MSG16*,HWND16,UINT16,UINT16,UINT16);
BOOL16      WINAPI PeekMessage32_16(MSG32_16*,HWND16,UINT16,UINT16,UINT16,BOOL16);
BOOL16      WINAPI PostAppMessage16(HTASK16,UINT16,WPARAM16,LPARAM);
BOOL16      WINAPI PostMessage16(HWND16,UINT16,WPARAM16,LPARAM);
void        WINAPI PostQuitMessage16(INT16);
BOOL16      WINAPI PtInRect16(const RECT16*,POINT16);
UINT16      WINAPI RealizePalette16(HDC16);
BOOL16      WINAPI RedrawWindow16(HWND16,const RECT16*,HRGN16,UINT16);
ATOM        WINAPI RegisterClass16(const WNDCLASS16*);
ATOM        WINAPI RegisterClassEx16(const WNDCLASSEX16*);
UINT16      WINAPI RegisterClipboardFormat16(LPCSTR);
BOOL        WINAPI RegisterShellHook16(HWND16,UINT16);
INT16       WINAPI ReleaseDC16(HWND16,HDC16);
BOOL16      WINAPI RemoveMenu16(HMENU16,UINT16,UINT16);
HANDLE16    WINAPI RemoveProp16(HWND16,LPCSTR);
VOID        WINAPI ReplyMessage16(LRESULT);
void        WINAPI ScreenToClient16(HWND16,LPPOINT16);
VOID        WINAPI ScrollChildren16(HWND16,UINT16,WPARAM16,LPARAM);
BOOL16      WINAPI ScrollDC16(HDC16,INT16,INT16,const RECT16*,const RECT16*,
                      HRGN16,LPRECT16);
void        WINAPI ScrollWindow16(HWND16,INT16,INT16,const RECT16*,const RECT16*);
INT16       WINAPI ScrollWindowEx16(HWND16,INT16,INT16,const RECT16*,
                                    const RECT16*,HRGN16,LPRECT16,UINT16);
HPALETTE16  WINAPI SelectPalette16(HDC16,HPALETTE16,BOOL16);
LRESULT     WINAPI SendDlgItemMessage16(HWND16,INT16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI SendMessage16(HWND16,UINT16,WPARAM16,LPARAM);
HWND16      WINAPI SetActiveWindow16(HWND16);
HWND16      WINAPI SetCapture16(HWND16);
VOID        WINAPI SetCaretBlinkTime16(UINT16);
VOID        WINAPI SetCaretPos16(INT16,INT16);
LONG        WINAPI SetClassLong16(HWND16,INT16,LONG);
WORD        WINAPI SetClassWord16(HWND16,INT16,WORD);
HANDLE16    WINAPI SetClipboardData16(UINT16,HANDLE16);
HWND16      WINAPI SetClipboardViewer16(HWND16);
INT16       WINAPI SetCommBreak16(INT16);
HCURSOR16   WINAPI SetCursor16(HCURSOR16);
void        WINAPI SetCursorPos16(INT16,INT16);
BOOL16      WINAPI SetDeskWallpaper16(const char*);
void        WINAPI SetDlgItemInt16(HWND16,INT16,UINT16,BOOL16);
void        WINAPI SetDlgItemText16(HWND16,INT16,SEGPTR);
VOID        WINAPI SetDoubleClickTime16(UINT16);
HWND16      WINAPI SetFocus16(HWND16);
BOOL16      WINAPI SetForegroundWindow16(HWND16);
void        WINAPI SetInternalWindowPos16(HWND16,UINT16,LPRECT16,LPPOINT16);
BOOL16      WINAPI SetMenu16(HWND16,HMENU16);
BOOL16      WINAPI SetMenuContextHelpId16(HMENU16,DWORD);
BOOL16      WINAPI SetMenuItemBitmaps16(HMENU16,UINT16,UINT16,HBITMAP16,HBITMAP16);
BOOL16      WINAPI SetMessageQueue16(INT16);
HWND16      WINAPI SetParent16(HWND16,HWND16);
BOOL16      WINAPI SetProp16(HWND16,LPCSTR,HANDLE16);
void        WINAPI SetRect16(LPRECT16,INT16,INT16,INT16,INT16);
void        WINAPI SetRectEmpty16(LPRECT16);
INT16       WINAPI SetScrollInfo16(HWND16,INT16,const SCROLLINFO*,BOOL16);
INT16       WINAPI SetScrollPos16(HWND16,INT16,INT16,BOOL16);
void        WINAPI SetScrollRange16(HWND16,INT16,INT16,INT16,BOOL16);
VOID        WINAPI SetSysColors16(INT16,const INT16*,const COLORREF*);
HWND16      WINAPI SetSysModalWindow16(HWND16);
BOOL16      WINAPI SetSystemMenu16(HWND16,HMENU16);
UINT16      WINAPI SetSystemTimer16(HWND16,UINT16,UINT16,TIMERPROC16);
UINT16      WINAPI SetTimer16(HWND16,UINT16,UINT16,TIMERPROC16);
LONG        WINAPI SetWindowLong16(HWND16,INT16,LONG);
BOOL16      WINAPI SetWindowPlacement16(HWND16,const WINDOWPLACEMENT16*);
BOOL16      WINAPI SetWindowPos16(HWND16,HWND16,INT16,INT16,INT16,INT16,WORD);
INT16       WINAPI SetWindowRgn16(HWND16,HRGN16,BOOL16);
BOOL16      WINAPI SetWindowText16(HWND16,SEGPTR);
WORD        WINAPI SetWindowWord16(HWND16,INT16,WORD);
VOID        WINAPI ShowCaret16(HWND16);
INT16       WINAPI ShowCursor16(BOOL16);
void        WINAPI ShowScrollBar16(HWND16,INT16,BOOL16);
VOID        WINAPI ShowOwnedPopups16(HWND16,BOOL16);
BOOL16      WINAPI ShowWindow16(HWND16,INT16);
BOOL16      WINAPI SubtractRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL16      WINAPI SwapMouseButton16(BOOL16);
VOID        WINAPI SwitchToThisWindow16(HWND16,BOOL16);
BOOL16      WINAPI SystemParametersInfo16(UINT16,UINT16,LPVOID,UINT16);
LONG        WINAPI TabbedTextOut16(HDC16,INT16,INT16,LPCSTR,INT16,INT16,const INT16*,INT16);
BOOL16      WINAPI TrackPopupMenu16(HMENU16,UINT16,INT16,INT16,INT16,HWND16,const RECT16*);
INT16       WINAPI TranslateAccelerator16(HWND16,HACCEL16,LPMSG16);
BOOL16      WINAPI TranslateMDISysAccel16(HWND16,LPMSG16);
BOOL16      WINAPI TranslateMessage16(const MSG16*);
BOOL16      WINAPI TranslateMessage32_16(const MSG32_16*,BOOL16);
INT16       WINAPI TransmitCommChar16(INT16,CHAR);
BOOL16      WINAPI UnionRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL16      WINAPI UnregisterClass16(LPCSTR,HINSTANCE16);
VOID        WINAPI UpdateWindow16(HWND16);
VOID        WINAPI ValidateRect16(HWND16,const RECT16*);
VOID        WINAPI ValidateRgn16(HWND16,HRGN16);
HWND16      WINAPI WindowFromDC16(HDC16);
HWND16      WINAPI WindowFromPoint16(POINT16);
BOOL16      WINAPI WinHelp16(HWND16,LPCSTR,UINT16,DWORD);
WORD        WINAPI WNetAddConnection16(LPCSTR,LPCSTR,LPCSTR);
INT16       WINAPI wvsprintf16(LPSTR,LPCSTR,VA_LIST16);
BOOL16      WINAPI DrawState16A(HDC16,HBRUSH16,DRAWSTATEPROC16,LPARAM,WPARAM16,INT16,INT16,INT16,INT16,UINT16);
BOOL16      WINAPI IsDialogMessage16(HWND16,MSG16*);
INT16       WINAPI GetCommError16(INT16,LPCOMSTAT16);
INT16       WINAPI BuildCommDCB16(LPCSTR,LPDCB16);
INT16       WINAPI GetCommState16(INT16,LPDCB16);
INT16       WINAPI SetCommState16(LPDCB16);
INT16       WINAPI lstrcmp16(LPCSTR,LPCSTR);
INT16       WINAPI lstrcmpi16(LPCSTR,LPCSTR);

/* undocumented functions */

void        WINAPI ConvertDialog32To16(LPCVOID,DWORD,LPVOID);
BOOL16      WINAPI EnumTaskWindows16(HTASK16,WNDENUMPROC16,LPARAM);
BOOL16      WINAPI GrayString16(HDC16,HBRUSH16,GRAYSTRINGPROC16,LPARAM,
                                INT16,INT16,INT16,INT16,INT16);
DWORD       WINAPI GetFileResourceSize16(LPCSTR,LPCSTR,LPCSTR,LPDWORD);
DWORD       WINAPI GetFileResource16(LPCSTR,LPCSTR,LPCSTR,DWORD,DWORD,LPVOID);
FARPROC16   WINAPI SetTaskSignalProc(HTASK16,FARPROC16);

#endif /* __WINE_WINE_WINUSER16_H */
