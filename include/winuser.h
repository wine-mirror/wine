#ifndef __INCLUDE_WINUSER_H
#define __INCLUDE_WINUSER_H

#ifndef RC_INVOKED
#include <stdarg.h>
#endif
#include "wintypes.h"
#include "wingdi.h"

#pragma pack(1)

#define WM_USER             0x0400

typedef struct
{
    UINT32   length;
    UINT32   flags;
    UINT32   showCmd;
    POINT32  ptMinPosition WINE_PACKED;
    POINT32  ptMaxPosition WINE_PACKED;
    RECT32   rcNormalPosition WINE_PACKED;
} WINDOWPLACEMENT32, *LPWINDOWPLACEMENT32;

DECL_WINELIB_TYPE(WINDOWPLACEMENT)
DECL_WINELIB_TYPE(LPWINDOWPLACEMENT)

  /* WINDOWPLACEMENT flags */
#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002

/****** Window classes ******/

typedef struct
{
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
} WNDCLASS32A, *LPWNDCLASS32A;

typedef struct
{
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCWSTR     lpszMenuName;
    LPCWSTR     lpszClassName;
} WNDCLASS32W, *LPWNDCLASS32W;

DECL_WINELIB_TYPE_AW(WNDCLASS)
DECL_WINELIB_TYPE_AW(LPWNDCLASS)

typedef struct
{
    UINT32      cbSize;
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCSTR      lpszMenuName;
    LPCSTR      lpszClassName;
    HICON32     hIconSm;
} WNDCLASSEX32A, *LPWNDCLASSEX32A;

typedef struct
{
    UINT32      cbSize;
    UINT32      style;
    WNDPROC32   lpfnWndProc;
    INT32       cbClsExtra;
    INT32       cbWndExtra;
    HINSTANCE32 hInstance;
    HICON32     hIcon;
    HCURSOR32   hCursor;
    HBRUSH32    hbrBackground;
    LPCWSTR     lpszMenuName;
    LPCWSTR     lpszClassName;
    HICON32     hIconSm;
} WNDCLASSEX32W, *LPWNDCLASSEX32W;

DECL_WINELIB_TYPE_AW(WNDCLASSEX)
DECL_WINELIB_TYPE_AW(LPWNDCLASSEX)

typedef struct
{
    HWND32    hwnd;
    UINT32    message;
    WPARAM32  wParam;
    LPARAM    lParam;
    DWORD     time;
    POINT32   pt;
} MSG32, *LPMSG32;

DECL_WINELIB_TYPE(MSG)
DECL_WINELIB_TYPE(LPMSG)
	
/* Cursors / Icons */

typedef struct {
	BOOL32		fIcon;
	DWORD		xHotspot;
	DWORD		yHotspot;
	HBITMAP32	hbmMask;
	HBITMAP32	hbmColor;
} ICONINFO,*LPICONINFO;

typedef struct
{
    BYTE   fVirt;
    BYTE   pad0;
    WORD   key;
    WORD   cmd;
    WORD   pad1;
} ACCEL32, *LPACCEL32;

DECL_WINELIB_TYPE(ACCEL)
DECL_WINELIB_TYPE(LPACCEL)

/* Flags for TrackPopupMenu */
#define TPM_LEFTBUTTON    0x0000
#define TPM_RIGHTBUTTON   0x0002
#define TPM_LEFTALIGN     0x0000
#define TPM_CENTERALIGN   0x0004
#define TPM_RIGHTALIGN    0x0008
#define TPM_TOPALIGN      0x0000
#define TPM_VCENTERALIGN  0x0010
#define TPM_BOTTOMALIGN   0x0020
#define TPM_HORIZONTAL    0x0000
#define TPM_VERTICAL      0x0040
#define TPM_NONOTIFY      0x0080
#define TPM_RETURNCMD     0x0100

typedef struct 
{
    UINT32   cbSize;
    RECT32   rcExclude;
} TPMPARAMS, *LPTPMPARAMS;

/* FIXME: not sure this one is correct */
typedef struct {
  UINT32    cbSize;
  UINT32    fMask;
  UINT32    fType;
  UINT32    fState;
  UINT32    wID;
  HMENU32   hSubMenu;
  HBITMAP32 hbmpChecked;
  HBITMAP32 hbmpUnchecked;
  DWORD     dwItemData;
  LPSTR     dwTypeData;
  UINT32    cch;
} MENUITEMINFO32A, *LPMENUITEMINFO32A;

typedef struct {
  UINT32    cbSize;
  UINT32    fMask;
  UINT32    fType;
  UINT32    fState;
  UINT32    wID;
  HMENU32   hSubMenu;
  HBITMAP32 hbmpChecked;
  HBITMAP32 hbmpUnchecked;
  DWORD     dwItemData;
  LPWSTR    dwTypeData;
  UINT32    cch;
} MENUITEMINFO32W, *LPMENUITEMINFO32W;

DECL_WINELIB_TYPE_AW(MENUITEMINFO)
DECL_WINELIB_TYPE_AW(LPMENUITEMINFO)

/* Field specifiers for MENUITEMINFO[AW] type.  */
#define MIIM_STATE       0x00000001
#define MIIM_ID          0x00000002
#define MIIM_SUBMENU     0x00000004
#define MIIM_CHECKMARKS  0x00000008
#define MIIM_TYPE        0x00000010
#define MIIM_DATA        0x00000020

/* DrawState defines ... */
typedef BOOL32 (CALLBACK *DRAWSTATEPROC32)(HDC32,LPARAM,WPARAM32,INT32,INT32);
DECL_WINELIB_TYPE(DRAWSTATEPROC)

/* WM_H/VSCROLL commands */
#define SB_LINEUP           0
#define SB_LINELEFT         0
#define SB_LINEDOWN         1
#define SB_LINERIGHT        1
#define SB_PAGEUP           2
#define SB_PAGELEFT         2
#define SB_PAGEDOWN         3
#define SB_PAGERIGHT        3
#define SB_THUMBPOSITION    4
#define SB_THUMBTRACK       5
#define SB_TOP              6
#define SB_LEFT             6
#define SB_BOTTOM           7
#define SB_RIGHT            7
#define SB_ENDSCROLL        8

/* Scroll bar selection constants */
#define SB_HORZ             0
#define SB_VERT             1
#define SB_CTL              2
#define SB_BOTH             3

/* Scrollbar styles */
#define SBS_HORZ                    0x0000L
#define SBS_VERT                    0x0001L
#define SBS_TOPALIGN                0x0002L
#define SBS_LEFTALIGN               0x0002L
#define SBS_BOTTOMALIGN             0x0004L
#define SBS_RIGHTALIGN              0x0004L
#define SBS_SIZEBOXTOPLEFTALIGN     0x0002L
#define SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L
#define SBS_SIZEBOX                 0x0008L
#define SBS_SIZEGRIP                0x0010L

/* EnableScrollBar() flags */
#define ESB_ENABLE_BOTH     0x0000
#define ESB_DISABLE_BOTH    0x0003

#define ESB_DISABLE_LEFT    0x0001
#define ESB_DISABLE_RIGHT   0x0002

#define ESB_DISABLE_UP      0x0001
#define ESB_DISABLE_DOWN    0x0002

#define ESB_DISABLE_LTUP    ESB_DISABLE_LEFT
#define ESB_DISABLE_RTDN    ESB_DISABLE_RIGHT

/* Scrollbar messages */
#define SBM_SETPOS16             (WM_USER+0)
#define SBM_SETPOS32             0x00e0
#define SBM_SETPOS               WINELIB_NAME(SBM_SETPOS)
#define SBM_GETPOS16             (WM_USER+1)
#define SBM_GETPOS32             0x00e1
#define SBM_GETPOS               WINELIB_NAME(SBM_GETPOS)
#define SBM_SETRANGE16           (WM_USER+2)
#define SBM_SETRANGE32           0x00e2
#define SBM_SETRANGE             WINELIB_NAME(SBM_SETRANGE)
#define SBM_GETRANGE16           (WM_USER+3)
#define SBM_GETRANGE32           0x00e3
#define SBM_GETRANGE             WINELIB_NAME(SBM_GETRANGE)
#define SBM_ENABLE_ARROWS16      (WM_USER+4)
#define SBM_ENABLE_ARROWS32      0x00e4
#define SBM_ENABLE_ARROWS        WINELIB_NAME(SBM_ENABLE_ARROWS)
#define SBM_SETRANGEREDRAW16     WM_NULL  /* Not in Win16 */
#define SBM_SETRANGEREDRAW32     0x00e6
#define SBM_SETRANGEREDRAW       WINELIB_NAME(SBM_SETRANGEREDRAW)
#define SBM_SETSCROLLINFO16      WM_NULL  /* Not in Win16 */
#define SBM_SETSCROLLINFO32      0x00e9
#define SBM_SETSCROLLINFO        WINELIB_NAME(SBM_SETSCROLLINFO)
#define SBM_GETSCROLLINFO16      WM_NULL  /* Not in Win16 */
#define SBM_GETSCROLLINFO32      0x00ea
#define SBM_GETSCROLLINFO        WINELIB_NAME(SBM_GETSCROLLINFO)

/* Scrollbar info */
typedef struct
{
    UINT32    cbSize;
    UINT32    fMask;
    INT32     nMin;
    INT32     nMax;
    UINT32    nPage;
    INT32     nPos;
    INT32     nTrackPos;
} SCROLLINFO, *LPSCROLLINFO;
 
/* GetScrollInfo() flags */ 
#define SIF_RANGE           0x0001
#define SIF_PAGE            0x0002
#define SIF_POS             0x0004
#define SIF_DISABLENOSCROLL 0x0008
#define SIF_TRACKPOS        0x0010
#define SIF_ALL             (SIF_RANGE | SIF_PAGE | SIF_POS | SIF_TRACKPOS)

/* Listbox styles */
#define LBS_NOTIFY               0x0001
#define LBS_SORT                 0x0002
#define LBS_NOREDRAW             0x0004
#define LBS_MULTIPLESEL          0x0008
#define LBS_OWNERDRAWFIXED       0x0010
#define LBS_OWNERDRAWVARIABLE    0x0020
#define LBS_HASSTRINGS           0x0040
#define LBS_USETABSTOPS          0x0080
#define LBS_NOINTEGRALHEIGHT     0x0100
#define LBS_MULTICOLUMN          0x0200
#define LBS_WANTKEYBOARDINPUT    0x0400
#define LBS_EXTENDEDSEL          0x0800
#define LBS_DISABLENOSCROLL      0x1000
#define LBS_NODATA               0x2000
#define LBS_NOSEL                0x4000
#define LBS_STANDARD  (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* Listbox messages */
#define LB_ADDSTRING32           0x0180
#define LB_ADDSTRING             WINELIB_NAME(LB_ADDSTRING)
#define LB_INSERTSTRING32        0x0181
#define LB_INSERTSTRING          WINELIB_NAME(LB_INSERTSTRING)
#define LB_DELETESTRING32        0x0182
#define LB_DELETESTRING          WINELIB_NAME(LB_DELETESTRING)
#define LB_SELITEMRANGEEX32      0x0183
#define LB_SELITEMRANGEEX        WINELIB_NAME(LB_SELITEMRANGEEX)
#define LB_RESETCONTENT32        0x0184
#define LB_RESETCONTENT          WINELIB_NAME(LB_RESETCONTENT)
#define LB_SETSEL32              0x0185
#define LB_SETSEL                WINELIB_NAME(LB_SETSEL)
#define LB_SETCURSEL32           0x0186
#define LB_SETCURSEL             WINELIB_NAME(LB_SETCURSEL)
#define LB_GETSEL32              0x0187
#define LB_GETSEL                WINELIB_NAME(LB_GETSEL)
#define LB_GETCURSEL32           0x0188
#define LB_GETCURSEL             WINELIB_NAME(LB_GETCURSEL)
#define LB_GETTEXT32             0x0189
#define LB_GETTEXT               WINELIB_NAME(LB_GETTEXT)
#define LB_GETTEXTLEN32          0x018a
#define LB_GETTEXTLEN            WINELIB_NAME(LB_GETTEXTLEN)
#define LB_GETCOUNT32            0x018b
#define LB_GETCOUNT              WINELIB_NAME(LB_GETCOUNT)
#define LB_SELECTSTRING32        0x018c
#define LB_SELECTSTRING          WINELIB_NAME(LB_SELECTSTRING)
#define LB_DIR32                 0x018d
#define LB_DIR                   WINELIB_NAME(LB_DIR)
#define LB_GETTOPINDEX32         0x018e
#define LB_GETTOPINDEX           WINELIB_NAME(LB_GETTOPINDEX)
#define LB_FINDSTRING32          0x018f
#define LB_FINDSTRING            WINELIB_NAME(LB_FINDSTRING)
#define LB_GETSELCOUNT32         0x0190
#define LB_GETSELCOUNT           WINELIB_NAME(LB_GETSELCOUNT)
#define LB_GETSELITEMS32         0x0191
#define LB_GETSELITEMS           WINELIB_NAME(LB_GETSELITEMS)
#define LB_SETTABSTOPS32         0x0192
#define LB_SETTABSTOPS           WINELIB_NAME(LB_SETTABSTOPS)
#define LB_GETHORIZONTALEXTENT32 0x0193
#define LB_GETHORIZONTALEXTENT   WINELIB_NAME(LB_GETHORIZONTALEXTENT)
#define LB_SETHORIZONTALEXTENT32 0x0194
#define LB_SETHORIZONTALEXTENT   WINELIB_NAME(LB_SETHORIZONTALEXTENT)
#define LB_SETCOLUMNWIDTH32      0x0195
#define LB_SETCOLUMNWIDTH        WINELIB_NAME(LB_SETCOLUMNWIDTH)
#define LB_ADDFILE32             0x0196
#define LB_ADDFILE               WINELIB_NAME(LB_ADDFILE)
#define LB_SETTOPINDEX32         0x0197
#define LB_SETTOPINDEX           WINELIB_NAME(LB_SETTOPINDEX)
#define LB_GETITEMRECT32         0x0198
#define LB_GETITEMRECT           WINELIB_NAME(LB_GETITEMRECT)
#define LB_GETITEMDATA32         0x0199
#define LB_GETITEMDATA           WINELIB_NAME(LB_GETITEMDATA)
#define LB_SETITEMDATA32         0x019a
#define LB_SETITEMDATA           WINELIB_NAME(LB_SETITEMDATA)
#define LB_SELITEMRANGE32        0x019b
#define LB_SELITEMRANGE          WINELIB_NAME(LB_SELITEMRANGE)
#define LB_SETANCHORINDEX32      0x019c
#define LB_SETANCHORINDEX        WINELIB_NAME(LB_SETANCHORINDEX)
#define LB_GETANCHORINDEX32      0x019d
#define LB_GETANCHORINDEX        WINELIB_NAME(LB_GETANCHORINDEX)
#define LB_SETCARETINDEX32       0x019e
#define LB_SETCARETINDEX         WINELIB_NAME(LB_SETCARETINDEX)
#define LB_GETCARETINDEX32       0x019f
#define LB_GETCARETINDEX         WINELIB_NAME(LB_GETCARETINDEX)
#define LB_SETITEMHEIGHT32       0x01a0
#define LB_SETITEMHEIGHT         WINELIB_NAME(LB_SETITEMHEIGHT)
#define LB_GETITEMHEIGHT32       0x01a1
#define LB_GETITEMHEIGHT         WINELIB_NAME(LB_GETITEMHEIGHT)
#define LB_FINDSTRINGEXACT32     0x01a2
#define LB_FINDSTRINGEXACT       WINELIB_NAME(LB_FINDSTRINGEXACT)
#define LB_CARETON32             0x01a3
#define LB_CARETON               WINELIB_NAME(LB_CARETON)
#define LB_CARETOFF32            0x01a4
#define LB_CARETOFF              WINELIB_NAME(LB_CARETOFF)
#define LB_SETLOCALE32           0x01a5
#define LB_SETLOCALE             WINELIB_NAME(LB_SETLOCALE)
#define LB_GETLOCALE32           0x01a6
#define LB_GETLOCALE             WINELIB_NAME(LB_GETLOCALE)
#define LB_SETCOUNT32            0x01a7
#define LB_SETCOUNT              WINELIB_NAME(LB_SETCOUNT)
#define LB_INITSTORAGE32         0x01a8
#define LB_INITSTORAGE           WINELIB_NAME(LB_INITSTORAGE)
#define LB_ITEMFROMPOINT32       0x01a9
#define LB_ITEMFROMPOINT         WINELIB_NAME(LB_ITEMFROMPOINT)

/* Listbox notification codes */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5

/* Listbox message return values */
#define LB_OKAY             0
#define LB_ERR              (-1)
#define LB_ERRSPACE         (-2)

#define LB_CTLCODE          0L

/* Combo box styles */
#define CBS_SIMPLE            0x0001L
#define CBS_DROPDOWN          0x0002L
#define CBS_DROPDOWNLIST      0x0003L
#define CBS_OWNERDRAWFIXED    0x0010L
#define CBS_OWNERDRAWVARIABLE 0x0020L
#define CBS_AUTOHSCROLL       0x0040L
#define CBS_OEMCONVERT        0x0080L
#define CBS_SORT              0x0100L
#define CBS_HASSTRINGS        0x0200L
#define CBS_NOINTEGRALHEIGHT  0x0400L
#define CBS_DISABLENOSCROLL   0x0800L

#define CBS_UPPERCASE	      0x2000L
#define CBS_LOWERCASE	      0x4000L


/* Combo box messages */
#define CB_GETEDITSEL32            0x0140
#define CB_GETEDITSEL              WINELIB_NAME(CB_GETEDITSEL)
#define CB_LIMITTEXT32             0x0141
#define CB_LIMITTEXT               WINELIB_NAME(CB_LIMITTEXT)
#define CB_SETEDITSEL32            0x0142
#define CB_SETEDITSEL              WINELIB_NAME(CB_SETEDITSEL)
#define CB_ADDSTRING32             0x0143
#define CB_ADDSTRING               WINELIB_NAME(CB_ADDSTRING)
#define CB_DELETESTRING32          0x0144
#define CB_DELETESTRING            WINELIB_NAME(CB_DELETESTRING)
#define CB_DIR32                   0x0145
#define CB_DIR                     WINELIB_NAME(CB_DIR)
#define CB_GETCOUNT32              0x0146
#define CB_GETCOUNT                WINELIB_NAME(CB_GETCOUNT)
#define CB_GETCURSEL32             0x0147
#define CB_GETCURSEL               WINELIB_NAME(CB_GETCURSEL)
#define CB_GETLBTEXT32             0x0148
#define CB_GETLBTEXT               WINELIB_NAME(CB_GETLBTEXT)
#define CB_GETLBTEXTLEN32          0x0149
#define CB_GETLBTEXTLEN            WINELIB_NAME(CB_GETLBTEXTLEN)
#define CB_INSERTSTRING32          0x014a
#define CB_INSERTSTRING            WINELIB_NAME(CB_INSERTSTRING)
#define CB_RESETCONTENT32          0x014b
#define CB_RESETCONTENT            WINELIB_NAME(CB_RESETCONTENT)
#define CB_FINDSTRING32            0x014c
#define CB_FINDSTRING              WINELIB_NAME(CB_FINDSTRING)
#define CB_SELECTSTRING32          0x014d
#define CB_SELECTSTRING            WINELIB_NAME(CB_SELECTSTRING)
#define CB_SETCURSEL32             0x014e
#define CB_SETCURSEL               WINELIB_NAME(CB_SETCURSEL)
#define CB_SHOWDROPDOWN32          0x014f
#define CB_SHOWDROPDOWN            WINELIB_NAME(CB_SHOWDROPDOWN)
#define CB_GETITEMDATA32           0x0150
#define CB_GETITEMDATA             WINELIB_NAME(CB_GETITEMDATA)
#define CB_SETITEMDATA32           0x0151
#define CB_SETITEMDATA             WINELIB_NAME(CB_SETITEMDATA)
#define CB_GETDROPPEDCONTROLRECT32 0x0152
#define CB_GETDROPPEDCONTROLRECT   WINELIB_NAME(CB_GETDROPPEDCONTROLRECT)
#define CB_SETITEMHEIGHT32         0x0153
#define CB_SETITEMHEIGHT           WINELIB_NAME(CB_SETITEMHEIGHT)
#define CB_GETITEMHEIGHT32         0x0154
#define CB_GETITEMHEIGHT           WINELIB_NAME(CB_GETITEMHEIGHT)
#define CB_SETEXTENDEDUI32         0x0155
#define CB_SETEXTENDEDUI           WINELIB_NAME(CB_SETEXTENDEDUI)
#define CB_GETEXTENDEDUI32         0x0156
#define CB_GETEXTENDEDUI           WINELIB_NAME(CB_GETEXTENDEDUI)
#define CB_GETDROPPEDSTATE32       0x0157
#define CB_GETDROPPEDSTATE         WINELIB_NAME(CB_GETDROPPEDSTATE)
#define CB_FINDSTRINGEXACT32       0x0158
#define CB_FINDSTRINGEXACT         WINELIB_NAME(CB_FINDSTRINGEXACT)
#define CB_SETLOCALE32             0x0159
#define CB_SETLOCALE               WINELIB_NAME(CB_SETLOCALE)
#define CB_GETLOCALE32             0x015a
#define CB_GETLOCALE               WINELIB_NAME(CB_GETLOCALE)
#define CB_GETTOPINDEX32           0x015b
#define CB_GETTOPINDEX             WINELIB_NAME(CB_GETTOPINDEX)
#define CB_SETTOPINDEX32           0x015c
#define CB_SETTOPINDEX             WINELIB_NAME(CB_SETTOPINDEX)
#define CB_GETHORIZONTALEXTENT32   0x015d
#define CB_GETHORIZONTALEXTENT     WINELIB_NAME(CB_GETHORIZONTALEXTENT)
#define CB_SETHORIZONTALEXTENT32   0x015e
#define CB_SETHORIZONTALEXTENT     WINELIB_NAME(CB_SETHORIZONTALEXTENT)
#define CB_GETDROPPEDWIDTH32       0x015f
#define CB_GETDROPPEDWIDTH         WINELIB_NAME(CB_GETDROPPEDWIDTH)
#define CB_SETDROPPEDWIDTH32       0x0160
#define CB_SETDROPPEDWIDTH         WINELIB_NAME(CB_SETDROPPEDWIDTH)
#define CB_INITSTORAGE32           0x0161
#define CB_INITSTORAGE             WINELIB_NAME(CB_INITSTORAGE)

/* Combo box notification codes */
#define CBN_ERRSPACE        (-1)
#define CBN_SELCHANGE       1
#define CBN_DBLCLK          2
#define CBN_SETFOCUS        3
#define CBN_KILLFOCUS       4
#define CBN_EDITCHANGE      5
#define CBN_EDITUPDATE      6
#define CBN_DROPDOWN        7
#define CBN_CLOSEUP         8
#define CBN_SELENDOK        9
#define CBN_SELENDCANCEL    10

/* Combo box message return values */
#define CB_OKAY             0
#define CB_ERR              (-1)
#define CB_ERRSPACE         (-2)

typedef struct _PRINTER_DEFAULTS32A {
    LPSTR        pDatatype;
    LPDEVMODE32A pDevMode;
    ACCESS_MASK  DesiredAccess;
} PRINTER_DEFAULTS32A, *LPPRINTER_DEFAULTS32A;

typedef struct _PRINTER_DEFAULTS32W {
    LPWSTR       pDatatype;
    LPDEVMODE32W pDevMode;
    ACCESS_MASK  DesiredAccess;
} PRINTER_DEFAULTS32W, *LPPRINTER_DEFAULTS32W;

DECL_WINELIB_TYPE_AW(PRINTER_DEFAULTS)
DECL_WINELIB_TYPE_AW(LPPRINTER_DEFAULTS)

#define	HELPINFO_WINDOW		0x0001
#define	HELPINFO_MENUITEM	0x0002
typedef struct			/* Structure pointed to by lParam of WM_HELP */
{
    UINT32	cbSize;		/* Size in bytes of this struct  */
    INT32	iContextType;	/* Either HELPINFO_WINDOW or HELPINFO_MENUITEM */
    INT32	iCtrlId;	/* Control Id or a Menu item Id. */
    HANDLE32	hItemHandle;	/* hWnd of control or hMenu.     */
    DWORD	dwContextId;	/* Context Id associated with this item */
    POINT32	MousePos;	/* Mouse Position in screen co-ordinates */
}  HELPINFO,*LPHELPINFO;

typedef void (CALLBACK *MSGBOXCALLBACK)(LPHELPINFO lpHelpInfo);

typedef struct
{
    UINT32	cbSize;
    HWND32	hwndOwner;
    HINSTANCE32	hInstance;
    LPCSTR	lpszText;
    LPCSTR	lpszCaption;
    DWORD	dwStyle;
    LPCSTR	lpszIcon;
    DWORD	dwContextHelpId;
    MSGBOXCALLBACK	lpfnMsgBoxCallback;
    DWORD	dwLanguageId;
} MSGBOXPARAMS32A,*LPMSGBOXPARAMS32A;

typedef struct
{
    UINT32	cbSize;
    HWND32	hwndOwner;
    HINSTANCE32	hInstance;
    LPCWSTR	lpszText;
    LPCWSTR	lpszCaption;
    DWORD	dwStyle;
    LPCWSTR	lpszIcon;
    DWORD	dwContextHelpId;
    MSGBOXCALLBACK	lpfnMsgBoxCallback;
    DWORD	dwLanguageId;
} MSGBOXPARAMS32W,*LPMSGBOXPARAMS32W;

DECL_WINELIB_TYPE_AW(MSGBOXPARAMS)
DECL_WINELIB_TYPE_AW(LPMSGBOXPARAMS)

typedef struct _numberfmt32a {
    UINT32 NumDigits;
    UINT32 LeadingZero;
    UINT32 Grouping;
    LPCSTR lpDecimalSep;
    LPCSTR lpThousandSep;
    UINT32 NegativeOrder;
} NUMBERFMT32A;

typedef struct _numberfmt32w {
    UINT32 NumDigits;
    UINT32 LeadingZero;
    UINT32 Grouping;
    LPCWSTR lpDecimalSep;
    LPCWSTR lpThousandSep;
    UINT32 NegativeOrder;
} NUMBERFMT32W;

#define MONITOR_DEFAULTTONULL       0x00000000
#define MONITOR_DEFAULTTOPRIMARY    0x00000001
#define MONITOR_DEFAULTTONEAREST    0x00000002

#define MONITORINFOF_PRIMARY        0x00000001

typedef struct tagMONITORINFO
{
    DWORD   cbSize;
    RECT32  rcMonitor;
    RECT32  rcWork;
    DWORD   dwFlags;
} MONITORINFO, *LPMONITORINFO;

typedef struct tagMONITORINFOEX32A
{
    MONITORINFO dummy;
    CHAR        szDevice[CCHDEVICENAME];
} MONITORINFOEX32A, *LPMONITORINFOEX32A;

typedef struct tagMONITORINFOEX32W
{
    MONITORINFO dummy;
    WCHAR       szDevice[CCHDEVICENAME];
} MONITORINFOEX32W, *LPMONITORINFOEX32W;

DECL_WINELIB_TYPE_AW(MONITORINFOEX)
DECL_WINELIB_TYPE_AW(LPMONITORINFOEX)

typedef BOOL32  (CALLBACK *MONITORENUMPROC)(HMONITOR,HDC32,LPRECT32,LPARAM);

#pragma pack(4)
WORD        WINAPI CascadeWindows (HWND32, UINT32, const LPRECT32,
                                   UINT32, const HWND32 *);
INT32       WINAPI CopyAcceleratorTable32A(HACCEL32,LPACCEL32,INT32);
INT32       WINAPI CopyAcceleratorTable32W(HACCEL32,LPACCEL32,INT32);
#define     CopyAcceleratorTable WINELIB_NAME_AW(CopyAcceleratorTable)
HICON32     WINAPI CreateIconIndirect(LPICONINFO);
BOOL32      WINAPI DestroyAcceleratorTable(HACCEL32);
BOOL32      WINAPI EnumDisplayMonitors(HDC32,LPRECT32,MONITORENUMPROC,LPARAM);
INT32       WINAPI EnumPropsEx32A(HWND32,PROPENUMPROCEX32A,LPARAM);
INT32       WINAPI EnumPropsEx32W(HWND32,PROPENUMPROCEX32W,LPARAM);
#define     EnumPropsEx WINELIB_NAME_AW(EnumPropsEx)
BOOL32      WINAPI EnumThreadWindows(DWORD,WNDENUMPROC32,LPARAM);
BOOL32      WINAPI ExitWindowsEx(UINT32,DWORD);
BOOL32      WINAPI GetIconInfo(HICON32,LPICONINFO);
DWORD       WINAPI GetMenuContextHelpId32(HMENU32);
#define     GetMenuContextHelpId WINELIB_NAME(GetMenuContextHelpId)
UINT32      WINAPI GetMenuDefaultItem32(HMENU32,UINT32,UINT32);
#define     GetMenuDefaultItem WINELIB_NAME(GetMenuDefaultItem)
BOOL32      WINAPI GetMenuItemInfo32A(HMENU32,UINT32,BOOL32,MENUITEMINFO32A*);
BOOL32      WINAPI GetMenuItemInfo32W(HMENU32,UINT32,BOOL32,MENUITEMINFO32W*);
#define     GetMenuItemInfo WINELIB_NAME_AW(GetMenuItemInfo)
BOOL32      WINAPI GetMonitorInfo32A(HMONITOR,LPMONITORINFO);
BOOL32      WINAPI GetMonitorInfo32W(HMONITOR,LPMONITORINFO);
#define     GetMonitorInfo WINELIB_NAME_AW(GetMonitorInfo)
INT32       WINAPI GetNumberFormat32A(LCID,DWORD,LPCSTR,const NUMBERFMT32A*,LPSTR,int);
INT32       WINAPI GetNumberFormat32W(LCID,DWORD,LPCWSTR,const NUMBERFMT32W*,LPWSTR,int);
#define     GetNumberFormat WINELIB_NAME_AW(GetNumberFormat);
DWORD       WINAPI GetWindowContextHelpId(HWND32);
DWORD       WINAPI GetWindowThreadProcessId(HWND32,LPDWORD);
BOOL32      WINAPI IsWindowUnicode(HWND32);
HKL32       WINAPI LoadKeyboardLayout32A(LPCSTR,UINT32);
HKL32       WINAPI LoadKeyboardLayout32W(LPCWSTR,UINT32);
#define     LoadKeyboardLayout WINELIB_NAME_AW(LoadKeyboardLayout)
INT32       WINAPI MessageBoxEx32A(HWND32,LPCSTR,LPCSTR,UINT32,WORD);
INT32       WINAPI MessageBoxEx32W(HWND32,LPCWSTR,LPCWSTR,UINT32,WORD);
#define     MessageBoxEx WINELIB_NAME_AW(MessageBoxEx)
HMONITOR    WINAPI MonitorFromPoint(POINT32,DWORD);
HMONITOR    WINAPI MonitorFromRect(LPRECT32,DWORD);
HMONITOR    WINAPI MonitorFromWindow(HWND32,DWORD);
DWORD       WINAPI MsgWaitForMultipleObjects(DWORD,HANDLE32*,BOOL32,DWORD,DWORD);
BOOL32      WINAPI PaintDesktop(HDC32);
BOOL32      WINAPI PostThreadMessage32A(DWORD, UINT32, WPARAM32, LPARAM);
BOOL32      WINAPI PostThreadMessage32W(DWORD, UINT32, WPARAM32, LPARAM);
#define     PostThreadMessage WINELIB_NAME_AW(PostThreadMessage)
UINT32      WINAPI ReuseDDElParam(UINT32,UINT32,UINT32,UINT32,UINT32);
BOOL32      WINAPI SendNotifyMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
BOOL32      WINAPI SendNotifyMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     SendNotifyMessage WINELIB_NAME_AW(SendNotifyMessage)
VOID        WINAPI SetDebugErrorLevel(DWORD);
VOID        WINAPI SetLastErrorEx(DWORD,DWORD);
BOOL32      WINAPI SetMenuDefaultItem32(HMENU32,UINT32,UINT32);
#define     SetMenuDefaultItem WINELIB_NAME(SetMenuDefaultItem)
BOOL32      WINAPI SetMenuItemInfo32A(HMENU32,UINT32,BOOL32,const MENUITEMINFO32A*);
BOOL32      WINAPI SetMenuItemInfo32W(HMENU32,UINT32,BOOL32,const MENUITEMINFO32W*);
#define     SetMenuItemInfo WINELIB_NAME_AW(SetMenuItemInfo)
BOOL32      WINAPI SetWindowContextHelpId(HWND32,DWORD);
WORD        WINAPI TileWindows (HWND32, UINT32, const LPRECT32,
                                UINT32, const HWND32 *);
BOOL32      WINAPI TrackPopupMenuEx(HMENU32,UINT32,INT32,INT32,HWND32,
                                    LPTPMPARAMS);
UINT32      WINAPI UnpackDDElParam(UINT32,UINT32,UINT32*,UINT32*);
DWORD       WINAPI WaitForInputIdle(HANDLE32,DWORD);
VOID        WINAPI keybd_event(BYTE,BYTE,DWORD,DWORD);
VOID        WINAPI mouse_event(DWORD,DWORD,DWORD,DWORD,DWORD);

/* Declarations for functions that are the same in Win16 and Win32 */
VOID        WINAPI EndMenu(void);
DWORD       WINAPI GetDialogBaseUnits(void);
VOID        WINAPI GetKeyboardState(LPBYTE);
DWORD       WINAPI GetMenuCheckMarkDimensions(void);
LONG        WINAPI GetMessageExtraInfo(void);
DWORD       WINAPI GetMessagePos(void);
LONG        WINAPI GetMessageTime(void);
DWORD       WINAPI GetTickCount(void);
ATOM        WINAPI GlobalDeleteAtom(ATOM);
DWORD       WINAPI OemKeyScan(WORD);
VOID        WINAPI ReleaseCapture(void);
VOID        WINAPI SetKeyboardState(LPBYTE);
VOID        WINAPI WaitMessage(VOID);


/* Declarations for functions that change between Win16 and Win32 */

BOOL32      WINAPI AdjustWindowRect32(LPRECT32,DWORD,BOOL32);
#define     AdjustWindowRect WINELIB_NAME(AdjustWindowRect)
BOOL32      WINAPI AdjustWindowRectEx32(LPRECT32,DWORD,BOOL32,DWORD);
#define     AdjustWindowRectEx WINELIB_NAME(AdjustWindowRectEx)
#define     AnsiLower32A CharLower32A
#define     AnsiLower32W CharLower32W
#define     AnsiLower WINELIB_NAME_AW(AnsiLower)
#define     AnsiLowerBuff32A CharLowerBuff32A
#define     AnsiLowerBuff32W CharLowerBuff32W
#define     AnsiLowerBuff WINELIB_NAME_AW(AnsiLowerBuff)
#define     AnsiNext32A CharNext32A
#define     AnsiNext32W CharNext32W
#define     AnsiNext WINELIB_NAME_AW(AnsiNext)
#define     AnsiPrev32A CharPrev32A
#define     AnsiPrev32W CharPrev32W
#define     AnsiPrev WINELIB_NAME_AW(AnsiPrev)
#define     AnsiUpper32A CharUpper32A
#define     AnsiUpper32W CharUpper32W
#define     AnsiUpper WINELIB_NAME_AW(AnsiUpper)
#define     AnsiUpperBuff32A CharUpperBuff32A
#define     AnsiUpperBuff32W CharUpperBuff32W
#define     AnsiUpperBuff WINELIB_NAME_AW(AnsiUpperBuff)
BOOL32      WINAPI AnyPopup32(void);
#define     AnyPopup WINELIB_NAME(AnyPopup)
BOOL32      WINAPI AppendMenu32A(HMENU32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI AppendMenu32W(HMENU32,UINT32,UINT32,LPCWSTR);
#define     AppendMenu WINELIB_NAME_AW(AppendMenu)
UINT32      WINAPI ArrangeIconicWindows32(HWND32);
#define     ArrangeIconicWindows WINELIB_NAME(ArrangeIconicWindows)
HDWP32      WINAPI BeginDeferWindowPos32(INT32);
#define     BeginDeferWindowPos WINELIB_NAME(BeginDeferWindowPos)
HDC32       WINAPI BeginPaint32(HWND32,LPPAINTSTRUCT32);
#define     BeginPaint WINELIB_NAME(BeginPaint)
BOOL32      WINAPI BringWindowToTop32(HWND32);
#define     BringWindowToTop WINELIB_NAME(BringWindowToTop)
BOOL32      WINAPI CallMsgFilter32A(LPMSG32,INT32);
BOOL32      WINAPI CallMsgFilter32W(LPMSG32,INT32);
#define     CallMsgFilter WINELIB_NAME_AW(CallMsgFilter)
LRESULT     WINAPI CallNextHookEx32(HHOOK,INT32,WPARAM32,LPARAM);
#define     CallNextHookEx WINELIB_NAME(CallNextHookEx)
LRESULT     WINAPI CallWindowProc32A(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI CallWindowProc32W(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
#define     CallWindowProc WINELIB_NAME_AW(CallWindowProc)
BOOL32      WINAPI ChangeClipboardChain32(HWND32,HWND32);
#define     ChangeClipboardChain WINELIB_NAME(ChangeClipboardChain)
BOOL32      WINAPI ChangeMenu32A(HMENU32,UINT32,LPCSTR,UINT32,UINT32);
BOOL32      WINAPI ChangeMenu32W(HMENU32,UINT32,LPCWSTR,UINT32,UINT32);
#define     ChangeMenu WINELIB_NAME_AW(ChangeMenu)
LPSTR       WINAPI CharLower32A(LPSTR);
LPWSTR      WINAPI CharLower32W(LPWSTR);
#define     CharLower WINELIB_NAME_AW(CharLower)
DWORD       WINAPI CharLowerBuff32A(LPSTR,DWORD);
DWORD       WINAPI CharLowerBuff32W(LPWSTR,DWORD);
#define     CharLowerBuff WINELIB_NAME_AW(CharLowerBuff)
LPSTR       WINAPI CharNext32A(LPCSTR);
LPWSTR      WINAPI CharNext32W(LPCWSTR);
#define     CharNext WINELIB_NAME_AW(CharNext)
LPSTR       WINAPI CharNextEx32A(WORD,LPCSTR,DWORD);
LPWSTR      WINAPI CharNextEx32W(WORD,LPCWSTR,DWORD);
#define     CharNextEx WINELIB_NAME_AW(CharNextEx)
LPSTR       WINAPI CharPrev32A(LPCSTR,LPCSTR);
LPWSTR      WINAPI CharPrev32W(LPCWSTR,LPCWSTR);
#define     CharPrev WINELIB_NAME_AW(CharPrev)
LPSTR       WINAPI CharPrevEx32A(WORD,LPCSTR,LPCSTR,DWORD);
LPWSTR      WINAPI CharPrevEx32W(WORD,LPCWSTR,LPCWSTR,DWORD);
#define     CharPrevEx WINELIB_NAME_AW(CharPrevEx)
LPSTR       WINAPI CharUpper32A(LPSTR);
LPWSTR      WINAPI CharUpper32W(LPWSTR);
#define     CharUpper WINELIB_NAME_AW(CharUpper)
DWORD       WINAPI CharUpperBuff32A(LPSTR,DWORD);
DWORD       WINAPI CharUpperBuff32W(LPWSTR,DWORD);
#define     CharUpperBuff WINELIB_NAME_AW(CharUpperBuff)
BOOL32      WINAPI CharToOem32A(LPCSTR,LPSTR);
BOOL32      WINAPI CharToOem32W(LPCWSTR,LPSTR);
#define     CharToOem WINELIB_NAME_AW(CharToOem)
BOOL32      WINAPI CharToOemBuff32A(LPCSTR,LPSTR,DWORD);
BOOL32      WINAPI CharToOemBuff32W(LPCWSTR,LPSTR,DWORD);
#define     CharToOemBuff WINELIB_NAME_AW(CharToOemBuff)
BOOL32      WINAPI CheckDlgButton32(HWND32,INT32,UINT32);
#define     CheckDlgButton WINELIB_NAME(CheckDlgButton)
DWORD       WINAPI CheckMenuItem32(HMENU32,UINT32,UINT32);
#define     CheckMenuItem WINELIB_NAME(CheckMenuItem)
BOOL16      WINAPI CheckMenuRadioItem16(HMENU16,UINT16,UINT16,UINT16,UINT16);
BOOL32      WINAPI CheckMenuRadioItem32(HMENU32,UINT32,UINT32,UINT32,UINT32);
#define     CheckMenuRadioItem WINELIB_NAME(CheckMenuRadioItem)
BOOL32      WINAPI CheckRadioButton32(HWND32,UINT32,UINT32,UINT32);
#define     CheckRadioButton WINELIB_NAME(CheckRadioButton)
HWND32      WINAPI ChildWindowFromPoint32(HWND32,POINT32);
#define     ChildWindowFromPoint WINELIB_NAME(ChildWindowFromPoint)
HWND32      WINAPI ChildWindowFromPointEx32(HWND32,POINT32,UINT32);
#define     ChildWindowFromPointEx WINELIB_NAME(ChildWindowFromPointEx)
BOOL32      WINAPI ClearCommBreak32(INT32);
#define     ClearCommBreak WINELIB_NAME(ClearCommBreak)
BOOL32      WINAPI ClientToScreen32(HWND32,LPPOINT32);
#define     ClientToScreen WINELIB_NAME(ClientToScreen)
BOOL32      WINAPI ClipCursor32(const RECT32*);
#define     ClipCursor WINELIB_NAME(ClipCursor)
BOOL32      WINAPI CloseClipboard32(void);
#define     CloseClipboard WINELIB_NAME(CloseClipboard)
BOOL32      WINAPI CloseWindow32(HWND32);
#define     CloseWindow WINELIB_NAME(CloseWindow)
#define     CopyCursor32(cur) ((HCURSOR32)CopyIcon32((HICON32)(cur)))
#define     CopyCursor WINELIB_NAME(CopyCursor)
HICON32     WINAPI CopyIcon32(HICON32);
#define     CopyIcon WINELIB_NAME(CopyIcon)
HICON16     WINAPI CopyImage16(HANDLE16,UINT16,INT16,INT16,UINT16);
HICON32     WINAPI CopyImage32(HANDLE32,UINT32,INT32,INT32,UINT32);
#define     CopyImage WINELIB_NAME(CopyImage)
BOOL32      WINAPI CopyRect32(RECT32*,const RECT32*);
#define     CopyRect WINELIB_NAME(CopyRect)
INT32       WINAPI CountClipboardFormats32(void);
#define     CountClipboardFormats WINELIB_NAME(CountClipboardFormats)
BOOL32      WINAPI CreateCaret32(HWND32,HBITMAP32,INT32,INT32);
#define     CreateCaret WINELIB_NAME(CreateCaret)
HCURSOR32   WINAPI CreateCursor32(HINSTANCE32,INT32,INT32,INT32,INT32,LPCVOID,LPCVOID);
#define     CreateCursor WINELIB_NAME(CreateCursor)
#define     CreateDialog32A(inst,ptr,hwnd,dlg) \
           CreateDialogParam32A(inst,ptr,hwnd,dlg,0)
#define     CreateDialog32W(inst,ptr,hwnd,dlg) \
           CreateDialogParam32W(inst,ptr,hwnd,dlg,0)
#define     CreateDialog WINELIB_NAME_AW(CreateDialog)
#define     CreateDialogIndirect32A(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32A(inst,ptr,hwnd,dlg,0)
#define     CreateDialogIndirect32W(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32W(inst,ptr,hwnd,dlg,0)
#define     CreateDialogIndirect WINELIB_NAME_AW(CreateDialogIndirect)
HWND32      WINAPI CreateDialogIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,
                                                DLGPROC32,LPARAM);
HWND32      WINAPI CreateDialogIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,
                                                DLGPROC32,LPARAM);
#define     CreateDialogIndirectParam WINELIB_NAME_AW(CreateDialogIndirectParam)
HWND32      WINAPI CreateDialogParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
HWND32      WINAPI CreateDialogParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define     CreateDialogParam WINELIB_NAME_AW(CreateDialogParam)
HICON32     WINAPI CreateIcon32(HINSTANCE32,INT32,INT32,BYTE,BYTE,LPCVOID,LPCVOID);
#define     CreateIcon WINELIB_NAME(CreateIcon)
HICON16     WINAPI CreateIconFromResource16(LPBYTE,UINT16,BOOL16,DWORD);
HICON32     WINAPI CreateIconFromResource32(LPBYTE,UINT32,BOOL32,DWORD);
#define     CreateIconFromResource WINELIB_NAME(CreateIconFromResource)
HICON32     WINAPI CreateIconFromResourceEx32(LPBYTE,UINT32,BOOL32,DWORD,INT32,INT32,UINT32);
#define     CreateIconFromResourceEx WINELIB_NAME(CreateIconFromResourceEx)
HMENU32     WINAPI CreateMenu32(void);
#define     CreateMenu WINELIB_NAME(CreateMenu)
HMENU32     WINAPI CreatePopupMenu32(void);
#define     CreatePopupMenu WINELIB_NAME(CreatePopupMenu)
#define     CreateWindow32A(className,titleName,style,x,y,width,height,\
                            parent,menu,instance,param) \
            CreateWindowEx32A(0,className,titleName,style,x,y,width,height,\
                              parent,menu,instance,param)
#define     CreateWindow32W(className,titleName,style,x,y,width,height,\
                            parent,menu,instance,param) \
            CreateWindowEx32W(0,className,titleName,style,x,y,width,height,\
                              parent,menu,instance,param)
#define     CreateWindow WINELIB_NAME_AW(CreateWindow)
HWND32      WINAPI CreateWindowEx32A(DWORD,LPCSTR,LPCSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
HWND32      WINAPI CreateWindowEx32W(DWORD,LPCWSTR,LPCWSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
#define     CreateWindowEx WINELIB_NAME_AW(CreateWindowEx)
HWND32      WINAPI CreateMDIWindow32A(LPCSTR,LPCSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HINSTANCE32,LPARAM);
HWND32      WINAPI CreateMDIWindow32W(LPCWSTR,LPCWSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HINSTANCE32,LPARAM);
#define     CreateMDIWindow WINELIB_NAME_AW(CreateMDIWindow)
LRESULT     WINAPI DefDlgProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefDlgProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefDlgProc WINELIB_NAME_AW(DefDlgProc)
HDWP32      WINAPI DeferWindowPos32(HDWP32,HWND32,HWND32,INT32,INT32,INT32,INT32,UINT32);
#define     DeferWindowPos WINELIB_NAME(DeferWindowPos)
LRESULT     WINAPI DefFrameProc32A(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefFrameProc32W(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
#define     DefFrameProc WINELIB_NAME_AW(DefFrameProc)
#define     DefHookProc32(code,wparam,lparam,phhook) \
            CallNextHookEx32(*(phhook),code,wparam,lparam)
#define     DefHookProc WINELIB_NAME(DefHookProc)
LRESULT     WINAPI DefMDIChildProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefMDIChildProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefMDIChildProc WINELIB_NAME_AW(DefMDIChildProc)
LRESULT     WINAPI DefWindowProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefWindowProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefWindowProc WINELIB_NAME_AW(DefWindowProc)
BOOL32      WINAPI DeleteMenu32(HMENU32,UINT32,UINT32);
#define     DeleteMenu WINELIB_NAME(DeleteMenu)
BOOL32      WINAPI DestroyCaret32(void);
#define     DestroyCaret WINELIB_NAME(DestroyCaret)
BOOL32      WINAPI DestroyCursor32(HCURSOR32);
#define     DestroyCursor WINELIB_NAME(DestroyCursor)
BOOL32      WINAPI DestroyIcon32(HICON32);
#define     DestroyIcon WINELIB_NAME(DestroyIcon)
BOOL32      WINAPI DestroyMenu32(HMENU32);
#define     DestroyMenu WINELIB_NAME(DestroyMenu)
BOOL32      WINAPI DestroyWindow32(HWND32);
#define     DestroyWindow WINELIB_NAME(DestroyWindow)
#define     DialogBox32A(inst,template,owner,func) \
            DialogBoxParam32A(inst,template,owner,func,0)
#define     DialogBox32W(inst,template,owner,func) \
            DialogBoxParam32W(inst,template,owner,func,0)
#define     DialogBox WINELIB_NAME_AW(DialogBox)
#define     DialogBoxIndirect32A(inst,template,owner,func) \
            DialogBoxIndirectParam32A(inst,template,owner,func,0)
#define     DialogBoxIndirect32W(inst,template,owner,func) \
            DialogBoxIndirectParam32W(inst,template,owner,func,0)
#define     DialogBoxIndirect WINELIB_NAME_AW(DialogBoxIndirect)
INT32       WINAPI DialogBoxIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
INT32       WINAPI DialogBoxIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
#define     DialogBoxIndirectParam WINELIB_NAME_AW(DialogBoxIndirectParam)
INT32       WINAPI DialogBoxParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
INT32       WINAPI DialogBoxParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define     DialogBoxParam WINELIB_NAME_AW(DialogBoxParam)
LONG        WINAPI DispatchMessage32A(const MSG32*);
LONG        WINAPI DispatchMessage32W(const MSG32*);
#define     DispatchMessage WINELIB_NAME_AW(DispatchMessage)
INT32       WINAPI DlgDirList32A(HWND32,LPSTR,INT32,INT32,UINT32);
INT32       WINAPI DlgDirList32W(HWND32,LPWSTR,INT32,INT32,UINT32);
#define     DlgDirList WINELIB_NAME_AW(DlgDirList)
INT32       WINAPI DlgDirListComboBox32A(HWND32,LPSTR,INT32,INT32,UINT32);
INT32       WINAPI DlgDirListComboBox32W(HWND32,LPWSTR,INT32,INT32,UINT32);
#define     DlgDirListComboBox WINELIB_NAME_AW(DlgDirListComboBox)
BOOL32      WINAPI DlgDirSelectComboBoxEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32      WINAPI DlgDirSelectComboBoxEx32W(HWND32,LPWSTR,INT32,INT32);
#define     DlgDirSelectComboBoxEx WINELIB_NAME_AW(DlgDirSelectComboBoxEx)
BOOL32      WINAPI DlgDirSelectEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32      WINAPI DlgDirSelectEx32W(HWND32,LPWSTR,INT32,INT32);
#define     DlgDirSelectEx WINELIB_NAME_AW(DlgDirSelectEx)
BOOL32      WINAPI DragDetect32(HWND32,POINT32);
#define     DragDetect WINELIB_NAME(DragDetect)
DWORD       WINAPI DragObject32(HWND32,HWND32,UINT32,DWORD,HCURSOR32);
#define     DragObject WINELIB_NAME(DragObject)
BOOL32      WINAPI DrawAnimatedRects32(HWND32,int,const RECT32*,const RECT32*);
#define     DrawAnimatedRects WINELIB_NAME(DrawAnimatedRects)
BOOL32      WINAPI DrawCaption32(HWND32,HDC32,const RECT32*,UINT32);
#define     DrawCaption WINELIB_NAME(DrawCaption)
BOOL32      WINAPI DrawCaptionTemp32A(HWND32,HDC32,const RECT32*,HFONT32,HICON32,LPCSTR,UINT32);
BOOL32      WINAPI DrawCaptionTemp32W(HWND32,HDC32,const RECT32*,HFONT32,HICON32,LPCWSTR,UINT32);
#define     DrawCaptionTemp WINELIB_NAME_AW(DrawCaptionTemp)
BOOL32      WINAPI DrawEdge32(HDC32,LPRECT32,UINT32,UINT32);
#define     DrawEdge WINELIB_NAME(DrawEdge)
void        WINAPI DrawFocusRect32(HDC32,const RECT32*);
#define     DrawFocusRect WINELIB_NAME(DrawFocusRect)
BOOL32      WINAPI DrawFrameControl32(HDC32,LPRECT32,UINT32,UINT32);
#define     DrawFrameControl WINELIB_NAME(DrawFrameControl)
BOOL32      WINAPI DrawIcon32(HDC32,INT32,INT32,HICON32);
#define     DrawIcon WINELIB_NAME(DrawIcon)
BOOL32      WINAPI DrawIconEx32(HDC32,INT32,INT32,HICON32,INT32,INT32,
				UINT32,HBRUSH32,UINT32);
#define     DrawIconEx WINELIB_NAME(DrawIconEx)
BOOL32      WINAPI DrawMenuBar32(HWND32);
#define     DrawMenuBar WINELIB_NAME(DrawMenuBar)
BOOL32      WINAPI DrawState32A(HDC32,HBRUSH32,DRAWSTATEPROC32,LPARAM,WPARAM32,INT32,INT32,INT32,INT32,UINT32);
BOOL32      WINAPI DrawState32W(HDC32,HBRUSH32,DRAWSTATEPROC32,LPARAM,WPARAM32,INT32,INT32,INT32,INT32,UINT32);
#define     DrawState WINELIB_NAME_AW(DrawState)
INT32       WINAPI DrawText32A(HDC32,LPCSTR,INT32,LPRECT32,UINT32);
INT32       WINAPI DrawText32W(HDC32,LPCWSTR,INT32,LPRECT32,UINT32);
#define     DrawText WINELIB_NAME_AW(DrawText)
BOOL32      WINAPI EmptyClipboard32(void);
#define     EmptyClipboard WINELIB_NAME(EmptyClipboard)
UINT32      WINAPI EnableMenuItem32(HMENU32,UINT32,UINT32);
#define     EnableMenuItem WINELIB_NAME(EnableMenuItem)
BOOL32      WINAPI EnableScrollBar32(HWND32,INT32,UINT32);
#define     EnableScrollBar WINELIB_NAME(EnableScrollBar)
BOOL32      WINAPI EnableWindow32(HWND32,BOOL32);
#define     EnableWindow WINELIB_NAME(EnableWindow)
BOOL32      WINAPI EndDeferWindowPos32(HDWP32);
#define     EndDeferWindowPos WINELIB_NAME(EndDeferWindowPos)
BOOL32      WINAPI EndDialog32(HWND32,INT32);
#define     EndDialog WINELIB_NAME(EndDialog)
BOOL32      WINAPI EndPaint32(HWND32,const PAINTSTRUCT32*);
#define     EndPaint WINELIB_NAME(EndPaint)
BOOL16      WINAPI EnumChildWindows16(HWND16,WNDENUMPROC16,LPARAM);
BOOL32      WINAPI EnumChildWindows32(HWND32,WNDENUMPROC32,LPARAM);
#define     EnumChildWindows WINELIB_NAME(EnumChildWindows)
UINT32      WINAPI EnumClipboardFormats32(UINT32);
#define     EnumClipboardFormats WINELIB_NAME(EnumClipboardFormats)
INT16       WINAPI EnumProps16(HWND16,PROPENUMPROC16);
INT32       WINAPI EnumProps32A(HWND32,PROPENUMPROC32A);
INT32       WINAPI EnumProps32W(HWND32,PROPENUMPROC32W);
#define     EnumProps WINELIB_NAME_AW(EnumProps)
BOOL16      WINAPI EnumWindows16(WNDENUMPROC16,LPARAM);
BOOL32      WINAPI EnumWindows32(WNDENUMPROC32,LPARAM);
#define     EnumWindows WINELIB_NAME(EnumWindows)
BOOL32      WINAPI EqualRect32(const RECT32*,const RECT32*);
#define     EqualRect WINELIB_NAME(EqualRect)
BOOL32      WINAPI EscapeCommFunction32(INT32,UINT32);
#define     EscapeCommFunction WINELIB_NAME(EscapeCommFunction)
INT32       WINAPI ExcludeUpdateRgn32(HDC32,HWND32);
#define     ExcludeUpdateRgn WINELIB_NAME(ExcludeUpdateRgn)
#define     ExitWindows32(a,b) ExitWindowsEx(EWX_LOGOFF,0xffffffff)
#define     ExitWindows WINELIB_NAME(ExitWindows)
INT32       WINAPI FillRect32(HDC32,const RECT32*,HBRUSH32);
#define     FillRect WINELIB_NAME(FillRect)
HWND32      WINAPI FindWindow32A(LPCSTR,LPCSTR);
HWND32      WINAPI FindWindow32W(LPCWSTR,LPCWSTR);
#define     FindWindow WINELIB_NAME_AW(FindWindow)
HWND32      WINAPI FindWindowEx32A(HWND32,HWND32,LPCSTR,LPCSTR);
HWND32      WINAPI FindWindowEx32W(HWND32,HWND32,LPCWSTR,LPCWSTR);
#define     FindWindowEx WINELIB_NAME_AW(FindWindowEx)
BOOL32      WINAPI FlashWindow32(HWND32,BOOL32);
#define     FlashWindow WINELIB_NAME(FlashWindow)
INT32       WINAPI FrameRect32(HDC32,const RECT32*,HBRUSH32);
#define     FrameRect WINELIB_NAME(FrameRect)
HWND32      WINAPI GetActiveWindow32(void);
#define     GetActiveWindow WINELIB_NAME(GetActiveWindow)
DWORD       WINAPI GetAppCompatFlags16(HTASK16);
DWORD       WINAPI GetAppCompatFlags32(HTASK32);
#define     GetAppCompatFlags WINELIB_NAME(GetAppCompatFlags)
WORD        WINAPI GetAsyncKeyState32(INT32);
#define     GetAsyncKeyState WINELIB_NAME(GetAsyncKeyState)
HWND32      WINAPI GetCapture32(void);
#define     GetCapture WINELIB_NAME(GetCapture)
UINT32      WINAPI GetCaretBlinkTime32(void);
#define     GetCaretBlinkTime WINELIB_NAME(GetCaretBlinkTime)
BOOL32      WINAPI GetCaretPos32(LPPOINT32);
#define     GetCaretPos WINELIB_NAME(GetCaretPos)
BOOL32      WINAPI GetClassInfo32A(HINSTANCE32,LPCSTR,WNDCLASS32A *);
BOOL32      WINAPI GetClassInfo32W(HINSTANCE32,LPCWSTR,WNDCLASS32W *);
#define     GetClassInfo WINELIB_NAME_AW(GetClassInfo)
BOOL32      WINAPI GetClassInfoEx32A(HINSTANCE32,LPCSTR,WNDCLASSEX32A *);
BOOL32      WINAPI GetClassInfoEx32W(HINSTANCE32,LPCWSTR,WNDCLASSEX32W *);
#define     GetClassInfoEx WINELIB_NAME_AW(GetClassInfoEx)
LONG        WINAPI GetClassLong32A(HWND32,INT32);
LONG        WINAPI GetClassLong32W(HWND32,INT32);
#define     GetClassLong WINELIB_NAME_AW(GetClassLong)
INT32       WINAPI GetClassName32A(HWND32,LPSTR,INT32);
INT32       WINAPI GetClassName32W(HWND32,LPWSTR,INT32);
#define     GetClassName WINELIB_NAME_AW(GetClassName)
WORD        WINAPI GetClassWord32(HWND32,INT32);
#define     GetClassWord WINELIB_NAME(GetClassWord)
void        WINAPI GetClientRect32(HWND32,LPRECT32);
#define     GetClientRect WINELIB_NAME(GetClientRect)
HANDLE32    WINAPI GetClipboardData32(UINT32);
#define     GetClipboardData WINELIB_NAME(GetClipboardData)
INT32       WINAPI GetClipboardFormatName32A(UINT32,LPSTR,INT32);
INT32       WINAPI GetClipboardFormatName32W(UINT32,LPWSTR,INT32);
#define     GetClipboardFormatName WINELIB_NAME_AW(GetClipboardFormatName)
HWND32      WINAPI GetClipboardOwner32(void);
#define     GetClipboardOwner WINELIB_NAME(GetClipboardOwner)
HWND32      WINAPI GetClipboardViewer32(void);
#define     GetClipboardViewer WINELIB_NAME(GetClipboardViewer)
void        WINAPI GetClipCursor32(LPRECT32);
#define     GetClipCursor WINELIB_NAME(GetClipCursor)
#define     GetCurrentTime32() GetTickCount()
#define     GetCurrentTime WINELIB_NAME(GetCurrentTime)
HCURSOR32   WINAPI GetCursor32(void);
#define     GetCursor WINELIB_NAME(GetCursor)
void        WINAPI GetCursorPos32(LPPOINT32);
#define     GetCursorPos WINELIB_NAME(GetCursorPos)
HDC32       WINAPI GetDC32(HWND32);
#define     GetDC WINELIB_NAME(GetDC)
HDC32       WINAPI GetDCEx32(HWND32,HRGN32,DWORD);
#define     GetDCEx WINELIB_NAME(GetDCEx)
HWND32      WINAPI GetDesktopWindow32(void);
#define     GetDesktopWindow WINELIB_NAME(GetDesktopWindow)
INT32       WINAPI GetDlgCtrlID32(HWND32);
#define     GetDlgCtrlID WINELIB_NAME(GetDlgCtrlID)
HWND32      WINAPI GetDlgItem32(HWND32,INT32);
#define     GetDlgItem WINELIB_NAME(GetDlgItem)
UINT32      WINAPI GetDlgItemInt32(HWND32,INT32,BOOL32*,BOOL32);
#define     GetDlgItemInt WINELIB_NAME(GetDlgItemInt)
INT32       WINAPI GetDlgItemText32A(HWND32,INT32,LPSTR,UINT32);
INT32       WINAPI GetDlgItemText32W(HWND32,INT32,LPWSTR,UINT32);
#define     GetDlgItemText WINELIB_NAME_AW(GetDlgItemText)
UINT32      WINAPI GetDoubleClickTime32(void);
#define     GetDoubleClickTime WINELIB_NAME(GetDoubleClickTime)
HWND32      WINAPI GetFocus32(void);
#define     GetFocus WINELIB_NAME(GetFocus)
HWND32      WINAPI GetForegroundWindow32(void);
#define     GetForegroundWindow WINELIB_NAME(GetForegroundWindow)
BOOL32      WINAPI GetInputState32(void);
#define     GetInputState WINELIB_NAME(GetInputState)
UINT32      WINAPI GetInternalWindowPos32(HWND32,LPRECT32,LPPOINT32);
#define     GetInternalWindowPos WINELIB_NAME(GetInternalWindowPos)
INT16       WINAPI GetKBCodePage16(void);
UINT32      WINAPI GetKBCodePage32(void);
#define     GetKBCodePage WINELIB_NAME(GetKBCodePage)
INT16       WINAPI GetKeyboardType16(INT16);
INT32       WINAPI GetKeyboardType32(INT32);
#define     GetKeyboardType WINELIB_NAME(GetKeyboardType)
INT16       WINAPI GetKeyNameText16(LONG,LPSTR,INT16);
INT32       WINAPI GetKeyNameText32A(LONG,LPSTR,INT32);
INT32       WINAPI GetKeyNameText32W(LONG,LPWSTR,INT32);
#define     GetKeyNameText WINELIB_NAME_AW(GetKeyNameText)
INT32       WINAPI GetKeyboardLayoutName32A(LPSTR);
INT32       WINAPI GetKeyboardLayoutName32W(LPWSTR);
#define     GetKeyboardLayoutName WINELIB_NAME_AW(GetKeyboardLayoutName)
INT16       WINAPI GetKeyState32(INT32);
#define     GetKeyState WINELIB_NAME(GetKeyState)
HWND32      WINAPI GetLastActivePopup32(HWND32);
#define     GetLastActivePopup WINELIB_NAME(GetLastActivePopup)
HMENU32     WINAPI GetMenu32(HWND32);
#define     GetMenu WINELIB_NAME(GetMenu)
INT32       WINAPI GetMenuItemCount32(HMENU32);
#define     GetMenuItemCount WINELIB_NAME(GetMenuItemCount)
UINT32      WINAPI GetMenuItemID32(HMENU32,INT32);
#define     GetMenuItemID WINELIB_NAME(GetMenuItemID)
BOOL32      WINAPI GetMenuItemRect32(HWND32,HMENU32,UINT32,LPRECT32);
#define     GetMenuItemRect WINELIB_NAME(GetMenuItemRect)
UINT32      WINAPI GetMenuState32(HMENU32,UINT32,UINT32);
#define     GetMenuState WINELIB_NAME(GetMenuState)
INT32       WINAPI GetMenuString32A(HMENU32,UINT32,LPSTR,INT32,UINT32);
INT32       WINAPI GetMenuString32W(HMENU32,UINT32,LPWSTR,INT32,UINT32);
#define     GetMenuString WINELIB_NAME_AW(GetMenuString)
BOOL32      WINAPI GetMessage32A(LPMSG32,HWND32,UINT32,UINT32);
BOOL32      WINAPI GetMessage32W(LPMSG32,HWND32,UINT32,UINT32);
#define     GetMessage WINELIB_NAME_AW(GetMessage)
HWND32      WINAPI GetNextDlgGroupItem32(HWND32,HWND32,BOOL32);
#define     GetNextDlgGroupItem WINELIB_NAME(GetNextDlgGroupItem)
HWND32      WINAPI GetNextDlgTabItem32(HWND32,HWND32,BOOL32);
#define     GetNextDlgTabItem WINELIB_NAME(GetNextDlgTabItem)
#define     GetNextWindow32 GetWindow32
#define     GetNextWindow WINELIB_NAME(GetNextWindow)
HWND32      WINAPI GetOpenClipboardWindow32(void);
#define     GetOpenClipboardWindow WINELIB_NAME(GetOpenClipboardWindow)
HWND32      WINAPI GetParent32(HWND32);
#define     GetParent WINELIB_NAME(GetParent)
INT32       WINAPI GetPriorityClipboardFormat32(UINT32*,INT32);
#define     GetPriorityClipboardFormat WINELIB_NAME(GetPriorityClipboardFormat)
HANDLE32    WINAPI GetProp32A(HWND32,LPCSTR);
HANDLE32    WINAPI GetProp32W(HWND32,LPCWSTR);
#define     GetProp WINELIB_NAME_AW(GetProp)
DWORD       WINAPI GetQueueStatus32(UINT32);
#define     GetQueueStatus WINELIB_NAME(GetQueueStatus)
BOOL32      WINAPI GetScrollInfo32(HWND32,INT32,LPSCROLLINFO);
#define     GetScrollInfo WINELIB_NAME(GetScrollInfo)
INT32       WINAPI GetScrollPos32(HWND32,INT32);
#define     GetScrollPos WINELIB_NAME(GetScrollPos)
BOOL32      WINAPI GetScrollRange32(HWND32,INT32,LPINT32,LPINT32);
#define     GetScrollRange WINELIB_NAME(GetScrollRange)
HWND32      WINAPI GetShellWindow32(void);
#define     GetShellWindow WINELIB_NAME(GetShellWindow)
HMENU32     WINAPI GetSubMenu32(HMENU32,INT32);
#define     GetSubMenu WINELIB_NAME(GetSubMenu)
COLORREF    WINAPI GetSysColor32(INT32);
#define     GetSysColor WINELIB_NAME(GetSysColor)
HBRUSH32    WINAPI GetSysColorBrush32(INT32);
#define     GetSysColorBrush WINELIB_NAME(GetSysColorBrush)
#define     GetSysModalWindow32() ((HWND32)0)
#define     GetSysModalWindow WINELIB_NAME(GetSysModalWindow)
HMENU32     WINAPI GetSystemMenu32(HWND32,BOOL32);
#define     GetSystemMenu WINELIB_NAME(GetSystemMenu)
INT32       WINAPI GetSystemMetrics32(INT32);
#define     GetSystemMetrics WINELIB_NAME(GetSystemMetrics)
DWORD       WINAPI GetTabbedTextExtent32A(HDC32,LPCSTR,INT32,INT32,const INT32*);
DWORD       WINAPI GetTabbedTextExtent32W(HDC32,LPCWSTR,INT32,INT32,const INT32*);
#define     GetTabbedTextExtent WINELIB_NAME_AW(GetTabbedTextExtent)
HWND32      WINAPI GetTopWindow32(HWND32);
#define     GetTopWindow WINELIB_NAME(GetTopWindow)
BOOL32      WINAPI GetUpdateRect32(HWND32,LPRECT32,BOOL32);
#define     GetUpdateRect WINELIB_NAME(GetUpdateRect)
INT32       WINAPI GetUpdateRgn32(HWND32,HRGN32,BOOL32);
#define     GetUpdateRgn WINELIB_NAME(GetUpdateRgn)
HWND32      WINAPI GetWindow32(HWND32,WORD);
#define     GetWindow WINELIB_NAME(GetWindow)
HDC32       WINAPI GetWindowDC32(HWND32);
#define     GetWindowDC WINELIB_NAME(GetWindowDC)
LONG        WINAPI GetWindowLong32A(HWND32,INT32);
LONG        WINAPI GetWindowLong32W(HWND32,INT32);
#define     GetWindowLong WINELIB_NAME_AW(GetWindowLong)
BOOL32      WINAPI GetWindowPlacement32(HWND32,LPWINDOWPLACEMENT32);
#define     GetWindowPlacement WINELIB_NAME(GetWindowPlacement)
BOOL32      WINAPI GetWindowRect32(HWND32,LPRECT32);
#define     GetWindowRect WINELIB_NAME(GetWindowRect)
INT16       WINAPI GetWindowRgn16(HWND16,HRGN16);
INT32       WINAPI GetWindowRgn32(HWND32,HRGN32);
#define     GetWindowRgn WINELIB_NAME(GetWindowRgn)
#define     GetWindowTask32(hwnd) ((HTASK32)GetWindowThreadProcessId(hwnd,NULL))
#define     GetWindowTask WINELIB_NAME(GetWindowTask)
INT32       WINAPI GetWindowText32A(HWND32,LPSTR,INT32);
INT32       WINAPI GetWindowText32W(HWND32,LPWSTR,INT32);
#define     GetWindowText WINELIB_NAME_AW(GetWindowText)
INT32       WINAPI GetWindowTextLength32A(HWND32);
INT32       WINAPI GetWindowTextLength32W(HWND32);
#define     GetWindowTextLength WINELIB_NAME_AW(GetWindowTextLength)
WORD        WINAPI GetWindowWord32(HWND32,INT32);
#define     GetWindowWord WINELIB_NAME(GetWindowWord)
ATOM        WINAPI GlobalAddAtom32A(LPCSTR);
ATOM        WINAPI GlobalAddAtom32W(LPCWSTR);
#define     GlobalAddAtom WINELIB_NAME_AW(GlobalAddAtom)
ATOM        WINAPI GlobalFindAtom32A(LPCSTR);
ATOM        WINAPI GlobalFindAtom32W(LPCWSTR);
#define     GlobalFindAtom WINELIB_NAME_AW(GlobalFindAtom)
UINT32      WINAPI GlobalGetAtomName32A(ATOM,LPSTR,INT32);
UINT32      WINAPI GlobalGetAtomName32W(ATOM,LPWSTR,INT32);
#define     GlobalGetAtomName WINELIB_NAME_AW(GlobalGetAtomName)
BOOL32      WINAPI GrayString32A(HDC32,HBRUSH32,GRAYSTRINGPROC32,LPARAM,
                                 INT32,INT32,INT32,INT32,INT32);
BOOL32      WINAPI GrayString32W(HDC32,HBRUSH32,GRAYSTRINGPROC32,LPARAM,
                                 INT32,INT32,INT32,INT32,INT32);
#define     GrayString WINELIB_NAME_AW(GrayString)
BOOL32      WINAPI HideCaret32(HWND32);
#define     HideCaret WINELIB_NAME(HideCaret)
BOOL32      WINAPI HiliteMenuItem32(HWND32,HMENU32,UINT32,UINT32);
#define     HiliteMenuItem WINELIB_NAME(HiliteMenuItem)
void        WINAPI InflateRect32(LPRECT32,INT32,INT32);
#define     InflateRect WINELIB_NAME(InflateRect)
BOOL32      WINAPI InSendMessage32(void);
#define     InSendMessage WINELIB_NAME(InSendMessage)
BOOL32      WINAPI InsertMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI InsertMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define     InsertMenu WINELIB_NAME_AW(InsertMenu)
BOOL32      WINAPI InsertMenuItem32A(HMENU32,UINT32,BOOL32,const MENUITEMINFO32A*);
BOOL32      WINAPI InsertMenuItem32W(HMENU32,UINT32,BOOL32,const MENUITEMINFO32W*);
#define     InsertMenuItem WINELIB_NAME_AW(InsertMenuItem)
BOOL32      WINAPI IntersectRect32(LPRECT32,const RECT32*,const RECT32*);
#define     IntersectRect WINELIB_NAME(IntersectRect)
void        WINAPI InvalidateRect32(HWND32,const RECT32*,BOOL32);
#define     InvalidateRect WINELIB_NAME(InvalidateRect)
void        WINAPI InvalidateRgn32(HWND32,HRGN32,BOOL32);
#define     InvalidateRgn WINELIB_NAME(InvalidateRgn)
void        WINAPI InvertRect32(HDC32,const RECT32*);
#define     InvertRect WINELIB_NAME(InvertRect)
BOOL32      WINAPI IsCharAlpha32A(CHAR);
BOOL32      WINAPI IsCharAlpha32W(WCHAR);
#define     IsCharAlpha WINELIB_NAME_AW(IsCharAlpha)
BOOL32      WINAPI IsCharAlphaNumeric32A(CHAR);
BOOL32      WINAPI IsCharAlphaNumeric32W(WCHAR);
#define     IsCharAlphaNumeric WINELIB_NAME_AW(IsCharAlphaNumeric)
BOOL32      WINAPI IsCharLower32A(CHAR);
BOOL32      WINAPI IsCharLower32W(WCHAR);
#define     IsCharLower WINELIB_NAME_AW(IsCharLower)
BOOL32      WINAPI IsCharUpper32A(CHAR);
BOOL32      WINAPI IsCharUpper32W(WCHAR);
#define     IsCharUpper WINELIB_NAME_AW(IsCharUpper)
BOOL32      WINAPI IsChild32(HWND32,HWND32);
#define     IsChild WINELIB_NAME(IsChild)
BOOL32      WINAPI IsClipboardFormatAvailable32(UINT32);
#define     IsClipboardFormatAvailable WINELIB_NAME(IsClipboardFormatAvailable)
BOOL32      WINAPI IsDialogMessage32A(HWND32,LPMSG32);
BOOL32      WINAPI IsDialogMessage32W(HWND32,LPMSG32);
#define     IsDialogMessage WINELIB_NAME_AW(IsDialogMessage)
UINT32      WINAPI IsDlgButtonChecked32(HWND32,UINT32);
#define     IsDlgButtonChecked WINELIB_NAME(IsDlgButtonChecked)
BOOL32      WINAPI IsIconic32(HWND32);
#define     IsIconic WINELIB_NAME(IsIconic)
BOOL32      WINAPI IsMenu32(HMENU32);
#define     IsMenu WINELIB_NAME(IsMenu)
BOOL32      WINAPI IsRectEmpty32(const RECT32*);
#define     IsRectEmpty WINELIB_NAME(IsRectEmpty)
BOOL16      WINAPI IsWindow16(HWND16);
BOOL32      WINAPI IsWindow32(HWND32);
#define     IsWindow WINELIB_NAME(IsWindow)
BOOL32      WINAPI IsWindowEnabled32(HWND32);
#define     IsWindowEnabled WINELIB_NAME(IsWindowEnabled)
BOOL32      WINAPI IsWindowVisible32(HWND32);
#define     IsWindowVisible WINELIB_NAME(IsWindowVisible)
BOOL32      WINAPI IsZoomed32(HWND32);
#define     IsZoomed WINELIB_NAME(IsZoomed)
BOOL32      WINAPI KillSystemTimer32(HWND32,UINT32);
#define     KillSystemTimer WINELIB_NAME(KillSystemTimer)
BOOL32      WINAPI KillTimer32(HWND32,UINT32);
#define     KillTimer WINELIB_NAME(KillTimer)
HACCEL32    WINAPI LoadAccelerators32A(HINSTANCE32,LPCSTR);
HACCEL32    WINAPI LoadAccelerators32W(HINSTANCE32,LPCWSTR);
#define     LoadAccelerators WINELIB_NAME_AW(LoadAccelerators)
HBITMAP32   WINAPI LoadBitmap32A(HANDLE32,LPCSTR);
HBITMAP32   WINAPI LoadBitmap32W(HANDLE32,LPCWSTR);
#define     LoadBitmap WINELIB_NAME_AW(LoadBitmap)
HCURSOR32   WINAPI LoadCursor32A(HINSTANCE32,LPCSTR);
HCURSOR32   WINAPI LoadCursor32W(HINSTANCE32,LPCWSTR);
#define     LoadCursor WINELIB_NAME_AW(LoadCursor)
HCURSOR32   WINAPI LoadCursorFromFile32A(LPCSTR);
HCURSOR32   WINAPI LoadCursorFromFile32W(LPCWSTR);
#define     LoadCursorFromFile WINELIB_NAME_AW(LoadCursorFromFile)
HICON32     WINAPI LoadIcon32A(HINSTANCE32,LPCSTR);
HICON32     WINAPI LoadIcon32W(HINSTANCE32,LPCWSTR);
#define     LoadIcon WINELIB_NAME_AW(LoadIcon)
HANDLE32    WINAPI LoadImage32A(HINSTANCE32,LPCSTR,UINT32,INT32,INT32,UINT32);
HANDLE32    WINAPI LoadImage32W(HINSTANCE32,LPCWSTR,UINT32,INT32,INT32,UINT32);
#define     LoadImage WINELIB_NAME_AW(LoadImage)
HMENU32     WINAPI LoadMenu32A(HINSTANCE32,LPCSTR);
HMENU32     WINAPI LoadMenu32W(HINSTANCE32,LPCWSTR);
#define     LoadMenu WINELIB_NAME_AW(LoadMenu)
HMENU32     WINAPI LoadMenuIndirect32A(LPCVOID);
HMENU32     WINAPI LoadMenuIndirect32W(LPCVOID);
#define     LoadMenuIndirect WINELIB_NAME_AW(LoadMenuIndirect)
INT32       WINAPI LoadString32A(HINSTANCE32,UINT32,LPSTR,INT32);
INT32       WINAPI LoadString32W(HINSTANCE32,UINT32,LPWSTR,INT32);
#define     LoadString WINELIB_NAME_AW(LoadString)
BOOL32      WINAPI LockWindowUpdate32(HWND32);
#define     LockWindowUpdate WINELIB_NAME(LockWindowUpdate)
INT16       WINAPI LookupIconIdFromDirectory16(LPBYTE,BOOL16);
INT32       WINAPI LookupIconIdFromDirectory32(LPBYTE,BOOL32);
#define     LookupIconIdFromDirectory WINELIB_NAME(LookupIconIdFromDirectory)
INT32       WINAPI LookupIconIdFromDirectoryEx32(LPBYTE,BOOL32,INT32,INT32,UINT32);
#define     LookupIconIdFromDirectoryEx WINELIB_NAME(LookupIconIdFromDirectoryEx)
UINT16      WINAPI MapVirtualKey16(UINT16,UINT16);
UINT32      WINAPI MapVirtualKey32A(UINT32,UINT32);
UINT32      WINAPI MapVirtualKey32W(UINT32,UINT32);
#define     MapVirtualKey WINELIB_NAME_AW(MapVirtualKey)
void        WINAPI MapDialogRect32(HWND32,LPRECT32);
#define     MapDialogRect WINELIB_NAME(MapDialogRect)
void        WINAPI MapWindowPoints32(HWND32,HWND32,LPPOINT32,UINT32);
#define     MapWindowPoints WINELIB_NAME(MapWindowPoints)
BOOL32      WINAPI MessageBeep32(UINT32);
#define     MessageBeep WINELIB_NAME(MessageBeep)
INT32       WINAPI MessageBox32A(HWND32,LPCSTR,LPCSTR,UINT32);
INT32       WINAPI MessageBox32W(HWND32,LPCWSTR,LPCWSTR,UINT32);
#define     MessageBox WINELIB_NAME_AW(MessageBox)
INT32       WINAPI MessageBoxIndirect32A(LPMSGBOXPARAMS32A);
INT32       WINAPI MessageBoxIndirect32W(LPMSGBOXPARAMS32W);
#define     MessageBoxIndirect WINELIB_NAME_AW(MessageBoxIndirect)
BOOL32      WINAPI ModifyMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI ModifyMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define     ModifyMenu WINELIB_NAME_AW(ModifyMenu)
BOOL32      WINAPI MoveWindow32(HWND32,INT32,INT32,INT32,INT32,BOOL32);
#define     MoveWindow WINELIB_NAME(MoveWindow)
BOOL32      WINAPI OemToChar32A(LPCSTR,LPSTR);
BOOL32      WINAPI OemToChar32W(LPCSTR,LPWSTR);
#define     OemToChar WINELIB_NAME_AW(OemToChar)
BOOL32      WINAPI OemToCharBuff32A(LPCSTR,LPSTR,DWORD);
BOOL32      WINAPI OemToCharBuff32W(LPCSTR,LPWSTR,DWORD);
#define     OemToCharBuff WINELIB_NAME_AW(OemToCharBuff)
void        WINAPI OffsetRect32(LPRECT32,INT32,INT32);
#define     OffsetRect WINELIB_NAME(OffsetRect)
BOOL32      WINAPI OpenClipboard32(HWND32);
#define     OpenClipboard WINELIB_NAME(OpenClipboard)
BOOL32      WINAPI OpenIcon32(HWND32);
#define     OpenIcon WINELIB_NAME(OpenIcon)
BOOL32      WINAPI PeekMessage32A(LPMSG32,HWND32,UINT32,UINT32,UINT32);
BOOL32      WINAPI PeekMessage32W(LPMSG32,HWND32,UINT32,UINT32,UINT32);
#define     PeekMessage WINELIB_NAME_AW(PeekMessage)
#define     PostAppMessage32A(thread,msg,wparam,lparam) \
            PostThreadMessage32A((DWORD)(thread),msg,wparam,lparam)
#define     PostAppMessage32W(thread,msg,wparam,lparam) \
            PostThreadMessage32W((DWORD)(thread),msg,wparam,lparam)
#define     PostAppMessage WINELIB_NAME_AW(PostAppMessage)
BOOL32      WINAPI PostMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
BOOL32      WINAPI PostMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     PostMessage WINELIB_NAME_AW(PostMessage)
void        WINAPI PostQuitMessage32(INT32);
#define     PostQuitMessage WINELIB_NAME(PostQuitMessage)
BOOL32      WINAPI PtInRect32(const RECT32*,POINT32);
#define     PtInRect WINELIB_NAME(PtInRect)
BOOL32      WINAPI RedrawWindow32(HWND32,const RECT32*,HRGN32,UINT32);
#define     RedrawWindow WINELIB_NAME(RedrawWindow)
ATOM        WINAPI RegisterClass32A(const WNDCLASS32A *);
ATOM        WINAPI RegisterClass32W(const WNDCLASS32W *);
#define     RegisterClass WINELIB_NAME_AW(RegisterClass)
ATOM        WINAPI RegisterClassEx32A(const WNDCLASSEX32A *);
ATOM        WINAPI RegisterClassEx32W(const WNDCLASSEX32W *);
#define     RegisterClassEx WINELIB_NAME_AW(RegisterClassEx)
UINT32      WINAPI RegisterClipboardFormat32A(LPCSTR);
UINT32      WINAPI RegisterClipboardFormat32W(LPCWSTR);
#define     RegisterClipboardFormat WINELIB_NAME_AW(RegisterClipboardFormat)
WORD        WINAPI RegisterWindowMessage32A(LPCSTR);
WORD        WINAPI RegisterWindowMessage32W(LPCWSTR);
#define     RegisterWindowMessage WINELIB_NAME_AW(RegisterWindowMessage)
INT32       WINAPI ReleaseDC32(HWND32,HDC32);
#define     ReleaseDC WINELIB_NAME(ReleaseDC)
BOOL32      WINAPI RemoveMenu32(HMENU32,UINT32,UINT32);
#define     RemoveMenu WINELIB_NAME(RemoveMenu)
HANDLE32    WINAPI RemoveProp32A(HWND32,LPCSTR);
HANDLE32    WINAPI RemoveProp32W(HWND32,LPCWSTR);
#define     RemoveProp WINELIB_NAME_AW(RemoveProp)
BOOL32      WINAPI ReplyMessage32(LRESULT);
#define     ReplyMessage WINELIB_NAME(ReplyMessage)
void        WINAPI ScreenToClient32(HWND32,LPPOINT32);
#define     ScreenToClient WINELIB_NAME(ScreenToClient)
VOID        WINAPI ScrollChildren32(HWND32,UINT32,WPARAM32,LPARAM);
#define     ScrollChildren WINELIB_NAME(ScrollChildren)
BOOL32      WINAPI ScrollDC32(HDC32,INT32,INT32,const RECT32*,const RECT32*,
                      HRGN32,LPRECT32);
#define     ScrollDC WINELIB_NAME(ScrollDC)
BOOL32      WINAPI ScrollWindow32(HWND32,INT32,INT32,const RECT32*,const RECT32*);
#define     ScrollWindow WINELIB_NAME(ScrollWindow)
INT32       WINAPI ScrollWindowEx32(HWND32,INT32,INT32,const RECT32*,
                                    const RECT32*,HRGN32,LPRECT32,UINT32);
#define     ScrollWindowEx WINELIB_NAME(ScrollWindowEx)
LRESULT     WINAPI SendDlgItemMessage32A(HWND32,INT32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI SendDlgItemMessage32W(HWND32,INT32,UINT32,WPARAM32,LPARAM);
#define     SendDlgItemMessage WINELIB_NAME_AW(SendDlgItemMessage)
LRESULT     WINAPI SendMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI SendMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     SendMessage WINELIB_NAME_AW(SendMessage)
LRESULT     WINAPI SendMessageTimeout16(HWND16,UINT16,WPARAM16,LPARAM,UINT16,
					UINT16,LPWORD);
LRESULT     WINAPI SendMessageTimeout32A(HWND32,UINT32,WPARAM32,LPARAM,UINT32,
					 UINT32,LPDWORD);
LRESULT     WINAPI SendMessageTimeout32W(HWND32,UINT32,WPARAM32,LPARAM,UINT32,
					 UINT32,LPDWORD);
#define     SendMessageTimeout WINELIB_NAME_AW(SendMessageTimeout)
HWND32      WINAPI SetActiveWindow32(HWND32);
#define     SetActiveWindow WINELIB_NAME(SetActiveWindow)
HWND32      WINAPI SetCapture32(HWND32);
#define     SetCapture WINELIB_NAME(SetCapture)
BOOL32      WINAPI SetCaretBlinkTime32(UINT32);
#define     SetCaretBlinkTime WINELIB_NAME(SetCaretBlinkTime)
BOOL32      WINAPI SetCaretPos32(INT32,INT32);
#define     SetCaretPos WINELIB_NAME(SetCaretPos)
LONG        WINAPI SetClassLong32A(HWND32,INT32,LONG);
LONG        WINAPI SetClassLong32W(HWND32,INT32,LONG);
#define     SetClassLong WINELIB_NAME_AW(SetClassLong)
WORD        WINAPI SetClassWord32(HWND32,INT32,WORD);
#define     SetClassWord WINELIB_NAME(SetClassWord)
HANDLE32    WINAPI SetClipboardData32(UINT32,HANDLE32);
#define     SetClipboardData WINELIB_NAME(SetClipboardData)
HWND32      WINAPI SetClipboardViewer32(HWND32);
#define     SetClipboardViewer WINELIB_NAME(SetClipboardViewer)
BOOL32      WINAPI SetCommBreak32(INT32);
#define     SetCommBreak WINELIB_NAME(SetCommBreak)
HCURSOR32   WINAPI SetCursor32(HCURSOR32);
#define     SetCursor WINELIB_NAME(SetCursor)
BOOL32      WINAPI SetCursorPos32(INT32,INT32);
#define     SetCursorPos WINELIB_NAME(SetCursorPos)
BOOL32      WINAPI SetDeskWallPaper32(LPCSTR);
#define     SetDeskWallPaper WINELIB_NAME(SetDeskWallPaper)
void        WINAPI SetDlgItemInt32(HWND32,INT32,UINT32,BOOL32);
#define     SetDlgItemInt WINELIB_NAME(SetDlgItemInt)
BOOL32      WINAPI SetDlgItemText32A(HWND32,INT32,LPCSTR);
BOOL32      WINAPI SetDlgItemText32W(HWND32,INT32,LPCWSTR);
#define     SetDlgItemText WINELIB_NAME_AW(SetDlgItemText)
BOOL32      WINAPI SetDoubleClickTime32(UINT32);
#define     SetDoubleClickTime WINELIB_NAME(SetDoubleClickTime)
HWND32      WINAPI SetFocus32(HWND32);
#define     SetFocus WINELIB_NAME(SetFocus)
BOOL32      WINAPI SetForegroundWindow32(HWND32);
#define     SetForegroundWindow WINELIB_NAME(SetForegroundWindow)
void        WINAPI SetInternalWindowPos32(HWND32,UINT32,LPRECT32,LPPOINT32);
#define     SetInternalWindowPos WINELIB_NAME(SetInternalWindowPos)
BOOL32      WINAPI SetMenu32(HWND32,HMENU32);
#define     SetMenu WINELIB_NAME(SetMenu)
BOOL32      WINAPI SetMenuContextHelpId32(HMENU32,DWORD);
#define     SetMenuContextHelpId WINELIB_NAME(SetMenuContextHelpId)
BOOL32      WINAPI SetMenuItemBitmaps32(HMENU32,UINT32,UINT32,HBITMAP32,HBITMAP32);
#define     SetMenuItemBitmaps WINELIB_NAME(SetMenuItemBitmaps)
BOOL32      WINAPI SetMessageQueue32(INT32);
#define     SetMessageQueue WINELIB_NAME(SetMessageQueue)
HWND32      WINAPI SetParent32(HWND32,HWND32);
#define     SetParent WINELIB_NAME(SetParent)
BOOL32      WINAPI SetProp32A(HWND32,LPCSTR,HANDLE32);
BOOL32      WINAPI SetProp32W(HWND32,LPCWSTR,HANDLE32);
#define     SetProp WINELIB_NAME_AW(SetProp)
void        WINAPI SetRect32(LPRECT32,INT32,INT32,INT32,INT32);
#define     SetRect WINELIB_NAME(SetRect)
void        WINAPI SetRectEmpty32(LPRECT32);
#define     SetRectEmpty WINELIB_NAME(SetRectEmpty)
INT32       WINAPI SetScrollInfo32(HWND32,INT32,const SCROLLINFO*,BOOL32);
#define     SetScrollInfo WINELIB_NAME(SetScrollInfo)
INT32       WINAPI SetScrollPos32(HWND32,INT32,INT32,BOOL32);
#define     SetScrollPos WINELIB_NAME(SetScrollPos)
BOOL32      WINAPI SetScrollRange32(HWND32,INT32,INT32,INT32,BOOL32);
#define     SetScrollRange WINELIB_NAME(SetScrollRange)
BOOL32      WINAPI SetSysColors32(INT32,const INT32*,const COLORREF*);
#define     SetSysColors WINELIB_NAME(SetSysColors)
#define     SetSysModalWindow32(hwnd) ((HWND32)0)
#define     SetSysModalWindow WINELIB_NAME(SetSysModalWindow)
BOOL32      WINAPI SetSystemMenu32(HWND32,HMENU32);
#define     SetSystemMenu WINELIB_NAME(SetSystemMenu)
UINT32      WINAPI SetSystemTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define     SetSystemTimer WINELIB_NAME(SetSystemTimer)
UINT32      WINAPI SetTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define     SetTimer WINELIB_NAME(SetTimer)
LONG        WINAPI SetWindowLong32A(HWND32,INT32,LONG);
LONG        WINAPI SetWindowLong32W(HWND32,INT32,LONG);
#define     SetWindowLong WINELIB_NAME_AW(SetWindowLong)
BOOL32      WINAPI SetWindowPlacement32(HWND32,const WINDOWPLACEMENT32*);
#define     SetWindowPlacement WINELIB_NAME(SetWindowPlacement)
FARPROC16   WINAPI SetWindowsHook16(INT16,HOOKPROC16);
HHOOK       WINAPI SetWindowsHook32A(INT32,HOOKPROC32);
HHOOK       WINAPI SetWindowsHook32W(INT32,HOOKPROC32);
#define     SetWindowsHook WINELIB_NAME_AW(SetWindowsHook)
HHOOK       WINAPI SetWindowsHookEx16(INT16,HOOKPROC16,HINSTANCE16,HTASK16);
HHOOK       WINAPI SetWindowsHookEx32A(INT32,HOOKPROC32,HINSTANCE32,DWORD);
HHOOK       WINAPI SetWindowsHookEx32W(INT32,HOOKPROC32,HINSTANCE32,DWORD);
#define     SetWindowsHookEx WINELIB_NAME_AW(SetWindowsHookEx)
BOOL32      WINAPI SetWindowPos32(HWND32,HWND32,INT32,INT32,INT32,INT32,WORD);
#define     SetWindowPos WINELIB_NAME(SetWindowPos)
INT32       WINAPI SetWindowRgn32(HWND32,HRGN32,BOOL32);
#define     SetWindowRgn WINELIB_NAME(SetWindowRgn)
BOOL32      WINAPI SetWindowText32A(HWND32,LPCSTR);
BOOL32      WINAPI SetWindowText32W(HWND32,LPCWSTR);
#define     SetWindowText WINELIB_NAME_AW(SetWindowText)
WORD        WINAPI SetWindowWord32(HWND32,INT32,WORD);
#define     SetWindowWord WINELIB_NAME(SetWindowWord)
BOOL32      WINAPI ShowCaret32(HWND32);
#define     ShowCaret WINELIB_NAME(ShowCaret)
INT32       WINAPI ShowCursor32(BOOL32);
#define     ShowCursor WINELIB_NAME(ShowCursor)
BOOL32      WINAPI ShowScrollBar32(HWND32,INT32,BOOL32);
#define     ShowScrollBar WINELIB_NAME(ShowScrollBar)
BOOL32      WINAPI ShowOwnedPopups32(HWND32,BOOL32);
#define     ShowOwnedPopups WINELIB_NAME(ShowOwnedPopups)
BOOL32      WINAPI ShowWindow32(HWND32,INT32);
#define     ShowWindow WINELIB_NAME(ShowWindow)
BOOL32      WINAPI SubtractRect32(LPRECT32,const RECT32*,const RECT32*);
#define     SubtractRect WINELIB_NAME(SubtractRect)
BOOL32      WINAPI SwapMouseButton32(BOOL32);
#define     SwapMouseButton WINELIB_NAME(SwapMouseButton)
VOID        WINAPI SwitchToThisWindow32(HWND32,BOOL32);
#define     SwitchToThisWindow WINELIB_NAME(SwitchToThisWindow)
BOOL32      WINAPI SystemParametersInfo32A(UINT32,UINT32,LPVOID,UINT32);
BOOL32      WINAPI SystemParametersInfo32W(UINT32,UINT32,LPVOID,UINT32);
#define     SystemParametersInfo WINELIB_NAME_AW(SystemParametersInfo)
LONG        WINAPI TabbedTextOut32A(HDC32,INT32,INT32,LPCSTR,INT32,INT32,const INT32*,INT32);
LONG        WINAPI TabbedTextOut32W(HDC32,INT32,INT32,LPCWSTR,INT32,INT32,const INT32*,INT32);
#define     TabbedTextOut WINELIB_NAME_AW(TabbedTextOut)
/* FIXME: the following line comes from keyboard.spec */
INT16       WINAPI ToAscii16(UINT16,UINT16,LPBYTE,LPVOID,UINT16);
INT32       WINAPI ToAscii32(UINT32,UINT32,LPBYTE,LPWORD,UINT32);
#define     ToAscii WINELIB_NAME(ToAscii)
BOOL32      WINAPI TrackPopupMenu32(HMENU32,UINT32,INT32,INT32,INT32,HWND32,const RECT32*);
#define     TrackPopupMenu WINELIB_NAME(TrackPopupMenu)
INT32       WINAPI TranslateAccelerator32(HWND32,HACCEL32,LPMSG32);
#define     TranslateAccelerator WINELIB_NAME(TranslateAccelerator)
BOOL32      WINAPI TranslateMDISysAccel32(HWND32,LPMSG32);
#define     TranslateMDISysAccel WINELIB_NAME(TranslateMDISysAccel)
BOOL32      WINAPI TranslateMessage32(const MSG32*);
#define     TranslateMessage WINELIB_NAME(TranslateMessage)
BOOL16      WINAPI UnhookWindowsHook16(INT16,HOOKPROC16);
BOOL32      WINAPI UnhookWindowsHook32(INT32,HOOKPROC32);
#define     UnhookWindowsHook WINELIB_NAME(UnhookWindowsHook)
BOOL16      WINAPI UnhookWindowsHookEx16(HHOOK);
BOOL32      WINAPI UnhookWindowsHookEx32(HHOOK);
#define     UnhookWindowsHookEx WINELIB_NAME(UnhookWindowsHookEx)
BOOL32      WINAPI UnionRect32(LPRECT32,const RECT32*,const RECT32*);
#define     UnionRect WINELIB_NAME(UnionRect)
BOOL32      WINAPI UnregisterClass32A(LPCSTR,HINSTANCE32);
BOOL32      WINAPI UnregisterClass32W(LPCWSTR,HINSTANCE32);
#define     UnregisterClass WINELIB_NAME_AW(UnregisterClass)
VOID        WINAPI UpdateWindow32(HWND32);
#define     UpdateWindow WINELIB_NAME(UpdateWindow)
VOID        WINAPI ValidateRect32(HWND32,const RECT32*);
#define     ValidateRect WINELIB_NAME(ValidateRect)
VOID        WINAPI ValidateRgn32(HWND32,HRGN32);
#define     ValidateRgn WINELIB_NAME(ValidateRgn)
/* FIXME the following line comes from keyboard.spec */
WORD        WINAPI VkKeyScan16(CHAR);
WORD        WINAPI VkKeyScan32A(CHAR);
WORD        WINAPI VkKeyScan32W(WCHAR);
#define     VkKeyScan WINELIB_NAME_AW(VkKeyScan)
HWND32      WINAPI WindowFromDC32(HDC32);
#define     WindowFromDC WINELIB_NAME(WindowFromDC)
HWND32      WINAPI WindowFromPoint32(POINT32);
#define     WindowFromPoint WINELIB_NAME(WindowFromPoint)
BOOL32      WINAPI WinHelp32A(HWND32,LPCSTR,UINT32,DWORD);
BOOL32      WINAPI WinHelp32W(HWND32,LPCWSTR,UINT32,DWORD);
#define     WinHelp WINELIB_NAME_AW(WinHelp)
UINT32      WINAPI WNetAddConnection32A(LPCSTR,LPCSTR,LPCSTR);
UINT32      WINAPI WNetAddConnection32W(LPCWSTR,LPCWSTR,LPCWSTR);
#define     WNetAddConnection WINELIB_NAME_AW(WNetAddConnection)
INT16       WINAPIV wsnprintf16(LPSTR,UINT16,LPCSTR,...);
INT32       WINAPIV wsnprintf32A(LPSTR,UINT32,LPCSTR,...);
INT32       WINAPIV wsnprintf32W(LPWSTR,UINT32,LPCWSTR,...);
#define     wsnprintf WINELIB_NAME_AW(wsnprintf)
INT16       WINAPIV wsprintf16(LPSTR,LPCSTR,...);
INT32       WINAPIV wsprintf32A(LPSTR,LPCSTR,...);
INT32       WINAPIV wsprintf32W(LPWSTR,LPCWSTR,...);
#define     wsprintf WINELIB_NAME_AW(wsprintf)
INT16       WINAPI wvsnprintf16(LPSTR,UINT16,LPCSTR,LPCVOID);
INT32       WINAPI wvsnprintf32A(LPSTR,UINT32,LPCSTR,va_list);
INT32       WINAPI wvsnprintf32W(LPWSTR,UINT32,LPCWSTR,va_list);
#define     wvsnprintf WINELIB_NAME_AW(wvsnprintf)
INT32       WINAPI wvsprintf32A(LPSTR,LPCSTR,va_list);
INT32       WINAPI wvsprintf32W(LPWSTR,LPCWSTR,va_list);
#define     wvsprintf WINELIB_NAME_AW(wvsprintf)

#endif /* __INCLUDE_WINUSER_H */
