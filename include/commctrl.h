/*
 * Common controls definitions
 *
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

#ifndef __WINE_COMMCTRL_H
#define __WINE_COMMCTRL_H

#include <prsht.h>
#include <commctrl.rh>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WINCOMMCTRLAPI
#ifdef _COMCTL32_
# define WINCOMMCTRLAPI
#else
# define WINCOMMCTRLAPI DECLSPEC_IMPORT
#endif
#endif

WINCOMMCTRLAPI BOOL WINAPI ShowHideMenuCtl (HWND, UINT_PTR, LPINT);
WINCOMMCTRLAPI VOID WINAPI GetEffectiveClientRect (HWND, LPRECT, const INT*);
WINCOMMCTRLAPI VOID WINAPI InitCommonControls (VOID);

typedef struct tagINITCOMMONCONTROLSEX {
    DWORD dwSize;
    DWORD dwICC;
} INITCOMMONCONTROLSEX, *LPINITCOMMONCONTROLSEX;

WINCOMMCTRLAPI BOOL WINAPI InitCommonControlsEx (const INITCOMMONCONTROLSEX*);

WINCOMMCTRLAPI LANGID WINAPI GetMUILanguage (VOID);
WINCOMMCTRLAPI VOID WINAPI InitMUILanguage (LANGID uiLang);

enum _LI_METRIC
{
    LIM_SMALL,
    LIM_LARGE
};

WINCOMMCTRLAPI HRESULT WINAPI LoadIconWithScaleDown(HINSTANCE, const WCHAR *, int, int, HICON *);
WINCOMMCTRLAPI HRESULT WINAPI LoadIconMetric(HINSTANCE, const WCHAR *, int, HICON *);

#define COMCTL32_VERSION                5  /* dll version */

#define ICC_LISTVIEW_CLASSES   0x00000001  /* listview, header */
#define ICC_TREEVIEW_CLASSES   0x00000002  /* treeview, tooltips */
#define ICC_BAR_CLASSES        0x00000004  /* toolbar, statusbar, trackbar, tooltips */
#define ICC_TAB_CLASSES        0x00000008  /* tab, tooltips */
#define ICC_UPDOWN_CLASS       0x00000010  /* updown */
#define ICC_PROGRESS_CLASS     0x00000020  /* progress */
#define ICC_HOTKEY_CLASS       0x00000040  /* hotkey */
#define ICC_ANIMATE_CLASS      0x00000080  /* animate */
#define ICC_WIN95_CLASSES      0x000000FF
#define ICC_DATE_CLASSES       0x00000100  /* month picker, date picker, time picker, updown */
#define ICC_USEREX_CLASSES     0x00000200  /* comboex */
#define ICC_COOL_CLASSES       0x00000400  /* rebar (coolbar) */
#define ICC_INTERNET_CLASSES   0x00000800  /* IP address, ... */
#define ICC_PAGESCROLLER_CLASS 0x00001000  /* page scroller */
#define ICC_NATIVEFNTCTL_CLASS 0x00002000  /* native font control ???*/
#define ICC_STANDARD_CLASSES   0x00004000
#define ICC_LINK_CLASS         0x00008000


/* common control shared messages */
#define CCM_FIRST            0x2000

#define CCM_SETBKCOLOR       (CCM_FIRST+0x1)     /* lParam = bkColor */
#define CCM_SETCOLORSCHEME   (CCM_FIRST+0x2)     /* lParam = COLORSCHEME struct ptr */
#define CCM_GETCOLORSCHEME   (CCM_FIRST+0x3)     /* lParam = COLORSCHEME struct ptr */
#define CCM_GETDROPTARGET    (CCM_FIRST+0x4)
#define CCM_SETUNICODEFORMAT (CCM_FIRST+0x5)
#define CCM_GETUNICODEFORMAT (CCM_FIRST+0x6)
#define CCM_SETVERSION       (CCM_FIRST+0x7)
#define CCM_GETVERSION       (CCM_FIRST+0x8)
#define CCM_SETNOTIFYWINDOW  (CCM_FIRST+0x9)     /* wParam = hwndParent */
#define CCM_SETWINDOWTHEME   (CCM_FIRST+0xb)
#define CCM_DPISCALE         (CCM_FIRST+0xc)


/* common notification codes (WM_NOTIFY)*/
#define NM_FIRST                (0U-  0U)
#define NM_LAST                 (0U- 99U)
#define NM_OUTOFMEMORY          (NM_FIRST-1)
#define NM_CLICK                (NM_FIRST-2)
#define NM_DBLCLK               (NM_FIRST-3)
#define NM_RETURN               (NM_FIRST-4)
#define NM_RCLICK               (NM_FIRST-5)
#define NM_RDBLCLK              (NM_FIRST-6)
#define NM_SETFOCUS             (NM_FIRST-7)
#define NM_KILLFOCUS            (NM_FIRST-8)
#define NM_CUSTOMDRAW           (NM_FIRST-12)
#define NM_HOVER                (NM_FIRST-13)
#define NM_NCHITTEST            (NM_FIRST-14)
#define NM_KEYDOWN              (NM_FIRST-15)
#define NM_RELEASEDCAPTURE      (NM_FIRST-16)
#define NM_SETCURSOR            (NM_FIRST-17)
#define NM_CHAR                 (NM_FIRST-18)
#define NM_TOOLTIPSCREATED      (NM_FIRST-19)
#define NM_LDOWN                (NM_FIRST-20)
#define NM_RDOWN                (NM_FIRST-21)
#define NM_THEMECHANGED         (NM_FIRST-22)
#define NM_FONTCHANGED          (NM_FIRST-23)
#define NM_CUSTOMTEXT           (NM_FIRST-24)
#define NM_TVSTATEIMAGECHANGING (NM_FIRST-24)

#define HANDLE_WM_NOTIFY(hwnd, wParam, lParam, fn) \
    (fn)((hwnd), (int)(wParam), (NMHDR*)(lParam))
#define FORWARD_WM_NOTIFY(hwnd, idFrom, pnmhdr, fn) \
    (LRESULT)(fn)((hwnd), WM_NOTIFY, (WPARAM)(int)(idFrom), (LPARAM)(NMHDR*)(pnmhdr))


/* callback constants */
#define LPSTR_TEXTCALLBACKA    ((LPSTR)-1)
#define LPSTR_TEXTCALLBACKW    ((LPWSTR)-1)
#define LPSTR_TEXTCALLBACK WINELIB_NAME_AW(LPSTR_TEXTCALLBACK)

#define I_IMAGECALLBACK          (-1)
#define I_IMAGENONE              (-2)
#define I_INDENTCALLBACK         (-1)
#define I_CHILDRENCALLBACK       (-1)
#define I_GROUPIDCALLBACK        (-1)
#define I_GROUPIDNONE            (-2)
#define I_COLUMNSCALLBACK        ((UINT)-1)

/* owner drawn types */
#define ODT_HEADER      100
#define ODT_TAB         101
#define ODT_LISTVIEW    102

/* common notification structures */
typedef struct tagNMTOOLTIPSCREATED
{
    NMHDR  hdr;
    HWND hwndToolTips;
} NMTOOLTIPSCREATED, *LPNMTOOLTIPSCREATED;

typedef struct tagNMMOUSE
{
    NMHDR   hdr;
    DWORD_PTR   dwItemSpec;
    DWORD_PTR   dwItemData;
    POINT   pt;
    DWORD   dwHitInfo;   /* info where on item or control the mouse is */
} NMMOUSE, *LPNMMOUSE;

typedef struct tagNMOBJECTNOTIFY
{
    NMHDR   hdr;
    int     iItem;
#ifdef __IID_DEFINED__
    const IID *piid;
#else
    const void *piid;
#endif
    void    *pObject;
    HRESULT hResult;
    DWORD   dwFlags;
} NMOBJECTNOTIFY, *LPNMOBJECTNOTIFY;

typedef struct tagNMKEY
{
    NMHDR   hdr;
    UINT    nVKey;
    UINT    uFlags;
} NMKEY, *LPNMKEY;

typedef struct tagNMCHAR
{
    NMHDR   hdr;
    UINT    ch;
    DWORD   dwItemPrev;           /* Item previously selected */
    DWORD   dwItemNext;           /* Item to be selected */
} NMCHAR, *LPNMCHAR;

#ifndef CCSIZEOF_STRUCT
#define CCSIZEOF_STRUCT(name, member) \
    (((INT)((LPBYTE)(&((name*)0)->member)-((LPBYTE)((name*)0))))+ \
    sizeof(((name*)0)->member))
#endif


#ifndef SNDMSG
#ifdef __cplusplus
#define SNDMSG ::SendMessage
#else   /* __cplusplus */
#define SNDMSG SendMessage
#endif  /* __cplusplus */
#endif  /* SNDMSG */


#ifdef __cplusplus
#define SNDMSGA ::SendMessageA
#define SNDMSGW ::SendMessageW
#else
#define SNDMSGA SendMessageA
#define SNDMSGW SendMessageW
#endif

/* Custom Draw messages */

#define CDRF_DODEFAULT          0x0
#define CDRF_NEWFONT            0x00000002
#define CDRF_SKIPDEFAULT        0x00000004
#define CDRF_DOERASE            0x00000008
#define CDRF_NOTIFYPOSTPAINT    0x00000010
#define CDRF_NOTIFYITEMDRAW     0x00000020
#define CDRF_NOTIFYSUBITEMDRAW  0x00000020
#define CDRF_NOTIFYPOSTERASE    0x00000040
#define CDRF_NOTIFYITEMERASE    0x00000080      /*  obsolete ??? */
#define CDRF_SKIPPOSTPAINT      0x00000100


/* drawstage flags */

#define CDDS_PREPAINT           1
#define CDDS_POSTPAINT          2
#define CDDS_PREERASE           3
#define CDDS_POSTERASE          4

#define CDDS_ITEM		0x00010000
#define CDDS_ITEMPREPAINT	(CDDS_ITEM | CDDS_PREPAINT)
#define CDDS_ITEMPOSTPAINT	(CDDS_ITEM | CDDS_POSTPAINT)
#define CDDS_ITEMPREERASE	(CDDS_ITEM | CDDS_PREERASE)
#define CDDS_ITEMPOSTERASE	(CDDS_ITEM | CDDS_POSTERASE)
#define CDDS_SUBITEM            0x00020000

/* itemState flags */

#define CDIS_SELECTED           0x0001
#define CDIS_GRAYED             0x0002
#define CDIS_DISABLED           0x0004
#define CDIS_CHECKED            0x0008
#define CDIS_FOCUS              0x0010
#define CDIS_DEFAULT            0x0020
#define CDIS_HOT                0x0040
#define CDIS_MARKED             0x0080
#define CDIS_INDETERMINATE      0x0100
#define CDIS_SHOWKEYBOARDCUES   0x0200
#define CDIS_NEARHOT            0x0400
#define CDIS_OTHERSIDEHOT       0x0800
#define CDIS_DROPHILITED        0x1000


typedef struct tagNMCUSTOMDRAWINFO
{
	NMHDR	hdr;
	DWORD	dwDrawStage;
	HDC	hdc;
	RECT	rc;
	DWORD_PTR dwItemSpec;
	UINT	uItemState;
	LPARAM	lItemlParam;
} NMCUSTOMDRAW, *LPNMCUSTOMDRAW;

typedef struct tagNMTTCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    UINT       uDrawFlags;
} NMTTCUSTOMDRAW, *LPNMTTCUSTOMDRAW;




/* StatusWindow */

#define STATUSCLASSNAMEA	"msctls_statusbar32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define STATUSCLASSNAMEW       L"msctls_statusbar32"
#else
static const WCHAR STATUSCLASSNAMEW[] = { 'm','s','c','t','l','s','_',
  's','t','a','t','u','s','b','a','r','3','2',0 };
#endif
#define STATUSCLASSNAME		WINELIB_NAME_AW(STATUSCLASSNAME)

#define SBT_NOBORDERS		0x0100
#define SBT_POPOUT		0x0200
#define SBT_RTLREADING		0x0400  /* not supported */
#define SBT_OWNERDRAW		0x1000

#define SB_SIMPLEID		0x00ff

#define SB_SETTEXTA		(WM_USER+1)
#define SB_SETTEXTW		(WM_USER+11)
#define SB_SETTEXT		WINELIB_NAME_AW(SB_SETTEXT)
#define SB_GETTEXTA		(WM_USER+2)
#define SB_GETTEXTW		(WM_USER+13)
#define SB_GETTEXT		WINELIB_NAME_AW(SB_GETTEXT)
#define SB_GETTEXTLENGTHA	(WM_USER+3)
#define SB_GETTEXTLENGTHW	(WM_USER+12)
#define SB_GETTEXTLENGTH	WINELIB_NAME_AW(SB_GETTEXTLENGTH)
#define SB_SETPARTS		(WM_USER+4)
#define SB_SETBORDERS		(WM_USER+5)
#define SB_GETPARTS		(WM_USER+6)
#define SB_GETBORDERS		(WM_USER+7)
#define SB_SETMINHEIGHT		(WM_USER+8)
#define SB_SIMPLE		(WM_USER+9)
#define SB_GETRECT		(WM_USER+10)
#define SB_ISSIMPLE		(WM_USER+14)
#define SB_SETICON		(WM_USER+15)
#define SB_SETTIPTEXTA		(WM_USER+16)
#define SB_SETTIPTEXTW		(WM_USER+17)
#define SB_SETTIPTEXT		WINELIB_NAME_AW(SB_SETTIPTEXT)
#define SB_GETTIPTEXTA		(WM_USER+18)
#define SB_GETTIPTEXTW		(WM_USER+19)
#define SB_GETTIPTEXT		WINELIB_NAME_AW(SB_GETTIPTEXT)
#define SB_GETICON		(WM_USER+20)
#define SB_SETBKCOLOR		CCM_SETBKCOLOR   /* lParam = bkColor */
#define SB_GETUNICODEFORMAT	CCM_GETUNICODEFORMAT
#define SB_SETUNICODEFORMAT	CCM_SETUNICODEFORMAT

#define SBN_FIRST		(0U-880U)
#define SBN_LAST		(0U-899U)
#define SBN_SIMPLEMODECHANGE	(SBN_FIRST-0)

WINCOMMCTRLAPI HWND WINAPI CreateStatusWindowA (LONG, LPCSTR, HWND, UINT);
WINCOMMCTRLAPI HWND WINAPI CreateStatusWindowW (LONG, LPCWSTR, HWND, UINT);
#define CreateStatusWindow WINELIB_NAME_AW(CreateStatusWindow)
WINCOMMCTRLAPI VOID WINAPI DrawStatusTextA (HDC, LPCRECT, LPCSTR, UINT);
WINCOMMCTRLAPI VOID WINAPI DrawStatusTextW (HDC, LPCRECT, LPCWSTR, UINT);
#define DrawStatusText WINELIB_NAME_AW(DrawStatusText)
WINCOMMCTRLAPI VOID WINAPI MenuHelp (UINT, WPARAM, LPARAM, HMENU,
                                     HINSTANCE, HWND, UINT*);

typedef struct tagCOLORSCHEME
{
   DWORD            dwSize;
   COLORREF         clrBtnHighlight;       /* highlight color */
   COLORREF         clrBtnShadow;          /* shadow color */
} COLORSCHEME, *LPCOLORSCHEME;

/**************************************************************************
 *  Drag List control
 */

typedef struct tagDRAGLISTINFO
{
    UINT  uNotification;
    HWND  hWnd;
    POINT ptCursor;
} DRAGLISTINFO, *LPDRAGLISTINFO;

#define DL_BEGINDRAG            (WM_USER+133)
#define DL_DRAGGING             (WM_USER+134)
#define DL_DROPPED              (WM_USER+135)
#define DL_CANCELDRAG           (WM_USER+136)

#define DL_CURSORSET            0
#define DL_STOPCURSOR           1
#define DL_COPYCURSOR           2
#define DL_MOVECURSOR           3

#define DRAGLISTMSGSTRINGA      "commctrl_DragListMsg"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define DRAGLISTMSGSTRINGW     L"commctrl_DragListMsg"
#else
static const WCHAR DRAGLISTMSGSTRINGW[] = { 'c','o','m','m','c','t','r','l',
  '_','D','r','a','g','L','i','s','t','M','s','g',0 };
#endif
#define DRAGLISTMSGSTRING       WINELIB_NAME_AW(DRAGLISTMSGSTRING)

WINCOMMCTRLAPI BOOL WINAPI MakeDragList (HWND);
WINCOMMCTRLAPI VOID WINAPI DrawInsert (HWND, HWND, INT);
WINCOMMCTRLAPI INT  WINAPI LBItemFromPt (HWND, POINT, BOOL);


/* UpDown */

#define UPDOWN_CLASSA           "msctls_updown32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define UPDOWN_CLASSW          L"msctls_updown32"
#else
static const WCHAR UPDOWN_CLASSW[] = { 'm','s','c','t','l','s','_',
  'u','p','d','o','w','n','3','2',0 };
#endif
#define UPDOWN_CLASS            WINELIB_NAME_AW(UPDOWN_CLASS)

typedef struct _UDACCEL
{
    UINT nSec;
    UINT nInc;
} UDACCEL, *LPUDACCEL;

#define UD_MAXVAL          0x7fff
#define UD_MINVAL          0x8001


#define UDN_FIRST          (0U-721)
#define UDN_LAST           (0U-740)
#define UDN_DELTAPOS       (UDN_FIRST-1)

#define UDM_SETRANGE       (WM_USER+101)
#define UDM_GETRANGE       (WM_USER+102)
#define UDM_SETPOS         (WM_USER+103)
#define UDM_GETPOS         (WM_USER+104)
#define UDM_SETBUDDY       (WM_USER+105)
#define UDM_GETBUDDY       (WM_USER+106)
#define UDM_SETACCEL       (WM_USER+107)
#define UDM_GETACCEL       (WM_USER+108)
#define UDM_SETBASE        (WM_USER+109)
#define UDM_GETBASE        (WM_USER+110)
#define UDM_SETRANGE32     (WM_USER+111)
#define UDM_GETRANGE32     (WM_USER+112)
#define UDM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define UDM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT
#define UDM_SETPOS32       (WM_USER+113)
#define UDM_GETPOS32       (WM_USER+114)


#define NMUPDOWN    NM_UPDOWN
#define LPNMUPDOWN  LPNM_UPDOWN

typedef struct tagNM_UPDOWN
{
  NMHDR hdr;
  int iPos;
  int iDelta;
} NM_UPDOWN, *LPNM_UPDOWN;

WINCOMMCTRLAPI HWND WINAPI CreateUpDownControl (DWORD, INT, INT, INT, INT, HWND, INT, HINSTANCE, HWND, INT, INT, INT);

/* Progress Bar */

#define PROGRESS_CLASSA   "msctls_progress32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define PROGRESS_CLASSW  L"msctls_progress32"
#else
static const WCHAR PROGRESS_CLASSW[] = { 'm','s','c','t','l','s','_',
  'p','r','o','g','r','e','s','s','3','2',0 };
#endif
#define PROGRESS_CLASS      WINELIB_NAME_AW(PROGRESS_CLASS)

#define PBM_SETRANGE        (WM_USER+1)
#define PBM_SETPOS          (WM_USER+2)
#define PBM_DELTAPOS        (WM_USER+3)
#define PBM_SETSTEP         (WM_USER+4)
#define PBM_STEPIT          (WM_USER+5)
#define PBM_SETRANGE32      (WM_USER+6)
#define PBM_GETRANGE        (WM_USER+7)
#define PBM_GETPOS          (WM_USER+8)
#define PBM_SETBARCOLOR     (WM_USER+9)
#define PBM_SETMARQUEE      (WM_USER+10)
#define PBM_GETSTEP         (WM_USER+13)
#define PBM_GETBKCOLOR      (WM_USER+14)
#define PBM_GETBARCOLOR     (WM_USER+15)
#define PBM_SETSTATE        (WM_USER+16)
#define PBM_GETSTATE        (WM_USER+17)
#define PBM_SETBKCOLOR      CCM_SETBKCOLOR


#define PBST_NORMAL         1
#define PBST_ERROR          2
#define PBST_PAUSED         3

typedef struct
{
    INT iLow;
    INT iHigh;
} PBRANGE, *PPBRANGE;


/* ImageList */

struct _IMAGELIST;
typedef struct _IMAGELIST *HIMAGELIST;

#define CLR_NONE         __MSABI_LONG(0xFFFFFFFF)
#define CLR_DEFAULT      __MSABI_LONG(0xFF000000)
#define CLR_HILIGHT      CLR_DEFAULT

#define ILC_MASK             0x00000001
#define ILC_COLOR            0x00000000
#define ILC_COLORDDB         0x000000fe
#define ILC_COLOR4           0x00000004
#define ILC_COLOR8           0x00000008
#define ILC_COLOR16          0x00000010
#define ILC_COLOR24          0x00000018
#define ILC_COLOR32          0x00000020
#define ILC_PALETTE          0x00000800  /* no longer supported by M$ */
#define ILC_MIRROR           0x00002000
#define ILC_PERITEMMIRROR    0x00008000
#define ILC_ORIGINALSIZE     0x00010000
#define ILC_HIGHQUALITYSCALE 0x00020000

#define ILD_NORMAL        0x0000
#define ILD_TRANSPARENT   0x0001
#define ILD_BLEND25       0x0002
#define ILD_BLEND50       0x0004
#define ILD_MASK          0x0010
#define ILD_IMAGE         0x0020
#define ILD_ROP           0x0040
#define ILD_OVERLAYMASK   0x0F00
#define ILD_PRESERVEALPHA 0x1000
#define ILD_SCALE         0x2000
#define ILD_DPISCALE      0x4000
#define ILD_ASYNC         0x8000

#define ILD_SELECTED     ILD_BLEND50
#define ILD_FOCUS        ILD_BLEND25
#define ILD_BLEND        ILD_BLEND50

#define INDEXTOOVERLAYMASK(i)  ((i)<<8)
#define INDEXTOSTATEIMAGEMASK(i) ((i)<<12)

#define ILCF_MOVE        (0x00000000)
#define ILCF_SWAP        (0x00000001)

#define ILGT_NORMAL     0x0000
#define ILGT_ASYNC      0x0001

#define ILS_NORMAL	0x0000
#define ILS_GLOW	0x0001
#define ILS_SHADOW	0x0002
#define ILS_SATURATE	0x0004
#define ILS_ALPHA	0x0008

typedef struct _IMAGEINFO
{
    HBITMAP hbmImage;
    HBITMAP hbmMask;
    INT     Unused1;
    INT     Unused2;
    RECT    rcImage;
} IMAGEINFO, *LPIMAGEINFO;


typedef struct _IMAGELISTDRAWPARAMS
{
    DWORD       cbSize;
    HIMAGELIST  himl;
    INT         i;
    HDC         hdcDst;
    INT         x;
    INT         y;
    INT         cx;
    INT         cy;
    INT         xBitmap;  /* x offset from the upperleft of bitmap */
    INT         yBitmap;  /* y offset from the upperleft of bitmap */
    COLORREF    rgbBk;
    COLORREF    rgbFg;
    UINT        fStyle;
    DWORD       dwRop;
    DWORD       fState;
    DWORD       Frame;
    COLORREF    crEffect;
} IMAGELISTDRAWPARAMS, *LPIMAGELISTDRAWPARAMS;

#define IMAGELISTDRAWPARAMS_V3_SIZE CCSIZEOF_STRUCT(IMAGELISTDRAWPARAMS, dwRop)

WINCOMMCTRLAPI HRESULT    WINAPI HIMAGELIST_QueryInterface(HIMAGELIST,REFIID,void **);
WINCOMMCTRLAPI INT        WINAPI ImageList_Add(HIMAGELIST,HBITMAP,HBITMAP);
WINCOMMCTRLAPI INT        WINAPI ImageList_AddMasked(HIMAGELIST,HBITMAP,COLORREF);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_BeginDrag(HIMAGELIST,INT,INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Copy(HIMAGELIST,INT,HIMAGELIST,INT,UINT);
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Create(INT,INT,UINT,INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Destroy(HIMAGELIST);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_DragEnter(HWND,INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_DragLeave(HWND);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_DragMove(INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_DragShowNolock (BOOL);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Draw(HIMAGELIST,INT,HDC,INT,INT,UINT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_DrawEx(HIMAGELIST,INT,HDC,INT,INT,INT,INT,COLORREF,COLORREF,UINT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_DrawIndirect(IMAGELISTDRAWPARAMS*);
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Duplicate(HIMAGELIST);
WINCOMMCTRLAPI VOID       WINAPI ImageList_EndDrag(VOID);
WINCOMMCTRLAPI COLORREF   WINAPI ImageList_GetBkColor(HIMAGELIST);
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_GetDragImage(POINT*,POINT*);
WINCOMMCTRLAPI HICON      WINAPI ImageList_GetIcon(HIMAGELIST,INT,UINT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_GetIconSize(HIMAGELIST,INT*,INT*);
WINCOMMCTRLAPI INT        WINAPI ImageList_GetImageCount(HIMAGELIST);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_GetImageInfo(HIMAGELIST,INT,IMAGEINFO*);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_GetImageRect(HIMAGELIST,INT,LPRECT);
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_LoadImageA(HINSTANCE,LPCSTR,INT,INT,COLORREF,UINT,UINT);
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_LoadImageW(HINSTANCE,LPCWSTR,INT,INT,COLORREF,UINT,UINT);
#define                          ImageList_LoadImage WINELIB_NAME_AW(ImageList_LoadImage)
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Merge(HIMAGELIST,INT,HIMAGELIST,INT,INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Remove(HIMAGELIST,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Replace(HIMAGELIST,INT,HBITMAP,HBITMAP);
WINCOMMCTRLAPI INT        WINAPI ImageList_ReplaceIcon(HIMAGELIST,INT,HICON);
WINCOMMCTRLAPI COLORREF   WINAPI ImageList_SetBkColor(HIMAGELIST,COLORREF);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_SetDragCursorImage(HIMAGELIST,INT,INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_SetIconSize(HIMAGELIST,INT,INT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_SetImageCount(HIMAGELIST,UINT);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_SetOverlayImage(HIMAGELIST,INT,INT);

struct IStream;
WINCOMMCTRLAPI HIMAGELIST WINAPI ImageList_Read(struct IStream*);
WINCOMMCTRLAPI BOOL       WINAPI ImageList_Write(HIMAGELIST,struct IStream*);
WINCOMMCTRLAPI HRESULT    WINAPI ImageList_WriteEx(HIMAGELIST,DWORD,struct IStream*);

#define ILP_NORMAL    0
#define ILP_DOWNLEVEL 1

#define ImageList_AddIcon(himl,hicon) ImageList_ReplaceIcon(himl,-1,hicon)
#define ImageList_ExtractIcon(hi,himl,i) ImageList_GetIcon(himl,i,0)
#define ImageList_LoadBitmap(hi,lpbmp,cx,cGrow,crMask) \
  ImageList_LoadImage(hi,lpbmp,cx,cGrow,crMask,IMAGE_BITMAP,0)
#define ImageList_RemoveAll(himl) ImageList_Remove(himl,-1)


#ifndef WM_MOUSEHOVER
#define WM_MOUSEHOVER                   0x02A1
#define WM_MOUSELEAVE                   0x02A3
#endif

#ifndef TME_HOVER

#define TME_HOVER       0x00000001
#define TME_LEAVE       0x00000002
#define TME_NONCLIENT   0x00000010
#define TME_QUERY       0x40000000
#define TME_CANCEL      0x80000000


#define HOVER_DEFAULT   0xFFFFFFFF

typedef struct tagTRACKMOUSEEVENT {
    DWORD cbSize;
    DWORD dwFlags;
    HWND  hwndTrack;
    DWORD dwHoverTime;
} TRACKMOUSEEVENT, *LPTRACKMOUSEEVENT;

#endif

WINCOMMCTRLAPI BOOL WINAPI _TrackMouseEvent(LPTRACKMOUSEEVENT lpEventTrack);

/* Flat Scrollbar control */

#define FLATSB_CLASSA         "flatsb_class32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define FLATSB_CLASSW        L"flatsb_class32"
#else
static const WCHAR FLATSB_CLASSW[] = { 'f','l','a','t','s','b','_',
  'c','l','a','s','s','3','2',0 };
#endif
#define FLATSB_CLASS          WINELIB_NAME_AW(FLATSB_CLASS)

#define WSB_PROP_CYVSCROLL     __MSABI_LONG(0x00000001)
#define WSB_PROP_CXHSCROLL     __MSABI_LONG(0x00000002)
#define WSB_PROP_CYHSCROLL     __MSABI_LONG(0x00000004)
#define WSB_PROP_CXVSCROLL     __MSABI_LONG(0x00000008)
#define WSB_PROP_CXHTHUMB      __MSABI_LONG(0x00000010)
#define WSB_PROP_CYVTHUMB      __MSABI_LONG(0x00000020)
#define WSB_PROP_VBKGCOLOR     __MSABI_LONG(0x00000040)
#define WSB_PROP_HBKGCOLOR     __MSABI_LONG(0x00000080)
#define WSB_PROP_VSTYLE        __MSABI_LONG(0x00000100)
#define WSB_PROP_HSTYLE        __MSABI_LONG(0x00000200)
#define WSB_PROP_WINSTYLE      __MSABI_LONG(0x00000400)
#define WSB_PROP_PALETTE       __MSABI_LONG(0x00000800)
#define WSB_PROP_MASK          __MSABI_LONG(0x00000FFF)

#define FSB_REGULAR_MODE       0
#define FSB_ENCARTA_MODE       1
#define FSB_FLAT_MODE          2


WINCOMMCTRLAPI BOOL    WINAPI FlatSB_EnableScrollBar(HWND, INT, UINT);
WINCOMMCTRLAPI BOOL    WINAPI FlatSB_ShowScrollBar(HWND, INT, BOOL);
WINCOMMCTRLAPI BOOL    WINAPI FlatSB_GetScrollRange(HWND, INT, LPINT, LPINT);
WINCOMMCTRLAPI BOOL    WINAPI FlatSB_GetScrollInfo(HWND, INT, LPSCROLLINFO);
WINCOMMCTRLAPI INT     WINAPI FlatSB_GetScrollPos(HWND, INT);
WINCOMMCTRLAPI BOOL    WINAPI FlatSB_GetScrollProp(HWND, INT, LPINT);
WINCOMMCTRLAPI INT     WINAPI FlatSB_SetScrollPos(HWND, INT, INT, BOOL);
WINCOMMCTRLAPI INT     WINAPI FlatSB_SetScrollInfo(HWND, INT, LPSCROLLINFO, BOOL);
WINCOMMCTRLAPI INT     WINAPI FlatSB_SetScrollRange(HWND, INT, INT, INT, BOOL);
WINCOMMCTRLAPI BOOL    WINAPI FlatSB_SetScrollProp(HWND, UINT, INT, BOOL);
WINCOMMCTRLAPI BOOL    WINAPI InitializeFlatSB(HWND);
WINCOMMCTRLAPI HRESULT WINAPI UninitializeFlatSB(HWND);

/* Subclassing stuff */
typedef LRESULT (CALLBACK *SUBCLASSPROC)(HWND, UINT, WPARAM, LPARAM, UINT_PTR, DWORD_PTR);
WINCOMMCTRLAPI BOOL    WINAPI SetWindowSubclass(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
WINCOMMCTRLAPI BOOL    WINAPI GetWindowSubclass(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR*);
WINCOMMCTRLAPI BOOL    WINAPI RemoveWindowSubclass(HWND, SUBCLASSPROC, UINT_PTR);
WINCOMMCTRLAPI LRESULT WINAPI DefSubclassProc(HWND, UINT, WPARAM, LPARAM);

WINCOMMCTRLAPI int WINAPI DrawShadowText(HDC, LPCWSTR, UINT, RECT*, DWORD, COLORREF, COLORREF, int, int);

/* Header control */

#define WC_HEADERA		"SysHeader32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_HEADERW             L"SysHeader32"
#else
static const WCHAR WC_HEADERW[] = { 'S','y','s','H','e','a','d','e','r','3','2',0 };
#endif
#define WC_HEADER		WINELIB_NAME_AW(WC_HEADER)


#define HDI_WIDTH               0x0001
#define HDI_HEIGHT              HDI_WIDTH
#define HDI_TEXT                0x0002
#define HDI_FORMAT              0x0004
#define HDI_LPARAM              0x0008
#define HDI_BITMAP              0x0010
#define HDI_IMAGE               0x0020
#define HDI_DI_SETITEM          0x0040
#define HDI_ORDER               0x0080
#define HDI_FILTER              0x0100
#define HDI_STATE               0x0200

#define HDIS_FOCUSED        0x00000001

#define HDF_LEFT                0x0000
#define HDF_RIGHT               0x0001
#define HDF_CENTER              0x0002
#define HDF_JUSTIFYMASK         0x0003
#define HDF_RTLREADING          0x0004
#define HDF_CHECKBOX            0x0040
#define HDF_CHECKED             0x0080
#define HDF_FIXEDWIDTH          0x0100
#define HDF_SORTDOWN            0x0200
#define HDF_SORTUP              0x0400
#define HDF_IMAGE               0x0800
#define HDF_BITMAP_ON_RIGHT     0x1000
#define HDF_BITMAP              0x2000
#define HDF_STRING              0x4000
#define HDF_OWNERDRAW           0x8000
#define HDF_SPLITBUTTON      0x1000000

#define HHT_NOWHERE             0x0001
#define HHT_ONHEADER            0x0002
#define HHT_ONDIVIDER           0x0004
#define HHT_ONDIVOPEN           0x0008
#define HHT_ONFILTER            0x0010
#define HHT_ONFILTERBUTTON      0x0020
#define HHT_ABOVE               0x0100
#define HHT_BELOW               0x0200
#define HHT_TORIGHT             0x0400
#define HHT_TOLEFT              0x0800
#define HHT_ONITEMSTATEICON     0x1000
#define HHT_ONDROPDOWN          0x2000
#define HHT_ONOVERFLOW          0x4000

#define HDM_FIRST               0x1200
#define HDM_GETITEMCOUNT        (HDM_FIRST+0)
#define HDM_INSERTITEMA         (HDM_FIRST+1)
#define HDM_INSERTITEMW         (HDM_FIRST+10)
#define HDM_INSERTITEM		WINELIB_NAME_AW(HDM_INSERTITEM)
#define HDM_DELETEITEM          (HDM_FIRST+2)
#define HDM_GETITEMA            (HDM_FIRST+3)
#define HDM_GETITEMW            (HDM_FIRST+11)
#define HDM_GETITEM		WINELIB_NAME_AW(HDM_GETITEM)
#define HDM_SETITEMA            (HDM_FIRST+4)
#define HDM_SETITEMW            (HDM_FIRST+12)
#define HDM_SETITEM		WINELIB_NAME_AW(HDM_SETITEM)
#define HDM_LAYOUT              (HDM_FIRST+5)
#define HDM_HITTEST             (HDM_FIRST+6)
#define HDM_GETITEMRECT         (HDM_FIRST+7)
#define HDM_SETIMAGELIST        (HDM_FIRST+8)
#define HDM_GETIMAGELIST        (HDM_FIRST+9)

#define HDM_ORDERTOINDEX        (HDM_FIRST+15)
#define HDM_CREATEDRAGIMAGE     (HDM_FIRST+16)
#define HDM_GETORDERARRAY       (HDM_FIRST+17)
#define HDM_SETORDERARRAY       (HDM_FIRST+18)
#define HDM_SETHOTDIVIDER       (HDM_FIRST+19)
#define HDM_SETBITMAPMARGIN     (HDM_FIRST+20)
#define HDM_GETBITMAPMARGIN     (HDM_FIRST+21)
#define HDM_SETFILTERCHANGETIMEOUT (HDM_FIRST+22)
#define HDM_EDITFILTER          (HDM_FIRST+23)
#define HDM_CLEARFILTER         (HDM_FIRST+24)
#define HDM_GETITEMDROPDOWNRECT (HDM_FIRST+25)
#define HDM_GETOVERFLOWRECT     (HDM_FIRST+26)
#define HDM_GETFOCUSEDITEM      (HDM_FIRST+27)
#define HDM_SETFOCUSEDITEM      (HDM_FIRST+28)
#define HDM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT
#define HDM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT

#define HDN_FIRST               (0U-300U)
#define HDN_LAST                (0U-399U)
#define HDN_ITEMCHANGINGA       (HDN_FIRST-0)
#define HDN_ITEMCHANGINGW       (HDN_FIRST-20)
#define HDN_ITEMCHANGING        WINELIB_NAME_AW(HDN_ITEMCHANGING)
#define HDN_ITEMCHANGEDA        (HDN_FIRST-1)
#define HDN_ITEMCHANGEDW        (HDN_FIRST-21)
#define HDN_ITEMCHANGED         WINELIB_NAME_AW(HDN_ITEMCHANGED)
#define HDN_ITEMCLICKA          (HDN_FIRST-2)
#define HDN_ITEMCLICKW          (HDN_FIRST-22)
#define HDN_ITEMCLICK           WINELIB_NAME_AW(HDN_ITEMCLICK)
#define HDN_ITEMDBLCLICKA       (HDN_FIRST-3)
#define HDN_ITEMDBLCLICKW       (HDN_FIRST-23)
#define HDN_ITEMDBLCLICK        WINELIB_NAME_AW(HDN_ITEMDBLCLICK)
#define HDN_DIVIDERDBLCLICKA    (HDN_FIRST-5)
#define HDN_DIVIDERDBLCLICKW    (HDN_FIRST-25)
#define HDN_DIVIDERDBLCLICK     WINELIB_NAME_AW(HDN_DIVIDERDBLCLICK)
#define HDN_BEGINTRACKA         (HDN_FIRST-6)
#define HDN_BEGINTRACKW         (HDN_FIRST-26)
#define HDN_BEGINTRACK          WINELIB_NAME_AW(HDN_BEGINTRACK)
#define HDN_ENDTRACKA           (HDN_FIRST-7)
#define HDN_ENDTRACKW           (HDN_FIRST-27)
#define HDN_ENDTRACK            WINELIB_NAME_AW(HDN_ENDTRACK)
#define HDN_TRACKA              (HDN_FIRST-8)
#define HDN_TRACKW              (HDN_FIRST-28)
#define HDN_TRACK               WINELIB_NAME_AW(HDN_TRACK)
#define HDN_GETDISPINFOA        (HDN_FIRST-9)
#define HDN_GETDISPINFOW        (HDN_FIRST-29)
#define HDN_GETDISPINFO         WINELIB_NAME_AW(HDN_GETDISPINFO)
#define HDN_BEGINDRAG           (HDN_FIRST-10)
#define HDN_ENDDRAG             (HDN_FIRST-11)
#define HDN_FILTERCHANGE        (HDN_FIRST-12)
#define HDN_FILTERBTNCLICK      (HDN_FIRST-13)
#define HDN_BEGINFILTEREDIT     (HDN_FIRST-14)
#define HDN_ENDFILTEREDIT       (HDN_FIRST-15)
#define HDN_ITEMSTATEICONCLICK  (HDN_FIRST-16)
#define HDN_ITEMKEYDOWN         (HDN_FIRST-17)
#define HDN_DROPDOWN            (HDN_FIRST-18)
#define HDN_OVERFLOWCLICK       (HDN_FIRST-19)

typedef struct _HD_LAYOUT
{
    RECT      *prc;
    WINDOWPOS *pwpos;
} HDLAYOUT, *LPHDLAYOUT;

#define HD_LAYOUT   HDLAYOUT

typedef struct _HD_ITEMA
{
    UINT    mask;
    INT     cxy;
    LPSTR     pszText;
    HBITMAP hbm;
    INT     cchTextMax;
    INT     fmt;
    LPARAM    lParam;
    /* (_WIN32_IE >= 0x0300) */
    INT     iImage;
    INT     iOrder;
    /* (_WIN32_IE >= 0x0500) */
    UINT    type;
    LPVOID  pvFilter;
    /* (_WIN32_WINNT >= 0x0600) */
    UINT    state;
} HDITEMA, *LPHDITEMA;

typedef struct _HD_ITEMW
{
    UINT    mask;
    INT     cxy;
    LPWSTR    pszText;
    HBITMAP hbm;
    INT     cchTextMax;
    INT     fmt;
    LPARAM    lParam;
    /* (_WIN32_IE >= 0x0300) */
    INT     iImage;
    INT     iOrder;
    /* (_WIN32_IE >= 0x0500) */
    UINT    type;
    LPVOID  pvFilter;
    /* (_WIN32_WINNT >= 0x0600) */
    UINT    state;
} HDITEMW, *LPHDITEMW;

#define HDITEM   WINELIB_NAME_AW(HDITEM)
#define LPHDITEM WINELIB_NAME_AW(LPHDITEM)
#define HD_ITEM  HDITEM

#define HDITEM_V1_SIZEA CCSIZEOF_STRUCT(HDITEMA, lParam)
#define HDITEM_V1_SIZEW CCSIZEOF_STRUCT(HDITEMW, lParam)
#define HDITEM_V1_SIZE WINELIB_NAME_AW(HDITEM_V1_SIZE)

#define HDFT_ISSTRING      0x0000
#define HDFT_ISNUMBER      0x0001
#define HDFT_HASNOVALUE    0x8000

typedef struct _HD_TEXTFILTERA
{
    LPSTR pszText;
    INT cchTextMax;
} HD_TEXTFILTERA, *LPHD_TEXTFILTERA;

typedef struct _HD_TEXTFILTERW
{
    LPWSTR pszText;
    INT cchTextMax;
} HD_TEXTFILTERW, *LPHD_TEXTFILTERW;

#define HD_TEXTFILTER WINELIB_NAME_AW(HD_TEXTFILTER)
#define HDTEXTFILTER WINELIB_NAME_AW(HD_TEXTFILTER)
#define LPHD_TEXTFILTER WINELIB_NAME_AW(LPHD_TEXTFILTER)
#define LPHDTEXTFILTER WINELIB_NAME_AW(LPHD_TEXTFILTER)

typedef struct _HD_HITTESTINFO
{
    POINT pt;
    UINT  flags;
    INT   iItem;
} HDHITTESTINFO, *LPHDHITTESTINFO;

#define HD_HITTESTINFO   HDHITTESTINFO

typedef struct tagNMHEADERA
{
    NMHDR     hdr;
    INT     iItem;
    INT     iButton;
    HDITEMA *pitem;
} NMHEADERA, *LPNMHEADERA;

typedef struct tagNMHEADERW
{
    NMHDR     hdr;
    INT     iItem;
    INT     iButton;
    HDITEMW *pitem;
} NMHEADERW, *LPNMHEADERW;

#define NMHEADER		WINELIB_NAME_AW(NMHEADER)
#define LPNMHEADER		WINELIB_NAME_AW(LPNMHEADER)
#define HD_NOTIFY               NMHEADER

typedef struct tagNMHDDISPINFOA
{
    NMHDR     hdr;
    INT     iItem;
    UINT    mask;
    LPSTR     pszText;
    INT     cchTextMax;
    INT     iImage;
    LPARAM    lParam;
} NMHDDISPINFOA, *LPNMHDDISPINFOA;

typedef struct tagNMHDDISPINFOW
{
    NMHDR     hdr;
    INT     iItem;
    UINT    mask;
    LPWSTR    pszText;
    INT     cchTextMax;
    INT     iImage;
    LPARAM    lParam;
} NMHDDISPINFOW, *LPNMHDDISPINFOW;

#define NMHDDISPINFO		WINELIB_NAME_AW(NMHDDISPINFO)
#define LPNMHDDISPINFO		WINELIB_NAME_AW(LPNMHDDISPINFO)

typedef struct tagNMHDFILTERBTNCLICK
{
    NMHDR hdr;
    INT iItem;
    RECT rc;
} NMHDFILTERBTNCLICK, *LPNMHDFILTERBTNCLICK;

#define Header_GetItemCount(hwndHD) \
  (INT)SNDMSG((hwndHD), HDM_GETITEMCOUNT, 0, 0)
#define Header_InsertItemA(hwndHD,i,phdi) \
  (INT)SNDMSGA((hwndHD),HDM_INSERTITEMA,(WPARAM)(INT)(i),(LPARAM)(const HDITEMA*)(phdi))
#define Header_InsertItemW(hwndHD,i,phdi) \
  (INT)SNDMSGW((hwndHD),HDM_INSERTITEMW,(WPARAM)(INT)(i),(LPARAM)(const HDITEMW*)(phdi))
#define Header_InsertItem WINELIB_NAME_AW(Header_InsertItem)
#define Header_DeleteItem(hwndHD,i) \
  (BOOL)SNDMSG((hwndHD), HDM_DELETEITEM, (WPARAM)(INT)(i), 0)
#define Header_GetItemA(hwndHD,i,phdi) \
  (BOOL)SNDMSGA((hwndHD),HDM_GETITEMA,(WPARAM)(INT)(i),(LPARAM)(HDITEMA*)(phdi))
#define Header_GetItemW(hwndHD,i,phdi) \
  (BOOL)SNDMSGW((hwndHD),HDM_GETITEMW,(WPARAM)(INT)(i),(LPARAM)(HDITEMW*)(phdi))
#define Header_GetItem WINELIB_NAME_AW(Header_GetItem)
#define Header_SetItemA(hwndHD,i,phdi) \
  (BOOL)SNDMSGA((hwndHD),HDM_SETITEMA,(WPARAM)(INT)(i),(LPARAM)(const HDITEMA*)(phdi))
#define Header_SetItemW(hwndHD,i,phdi) \
  (BOOL)SNDMSGW((hwndHD),HDM_SETITEMW,(WPARAM)(INT)(i),(LPARAM)(const HDITEMW*)(phdi))
#define Header_SetItem WINELIB_NAME_AW(Header_SetItem)
#define Header_Layout(hwndHD,playout) \
  (BOOL)SNDMSG((hwndHD),HDM_LAYOUT,0,(LPARAM)(LPHDLAYOUT)(playout))
#define Header_GetItemRect(hwnd,iItem,lprc) \
  (BOOL)SNDMSG((hwnd),HDM_GETITEMRECT,(WPARAM)iItem,(LPARAM)lprc)
#define Header_SetImageList(hwnd,himl) \
  (HIMAGELIST)SNDMSG((hwnd),HDM_SETIMAGELIST,0,(LPARAM)himl)
#define Header_GetImageList(hwnd) \
  (HIMAGELIST)SNDMSG((hwnd),HDM_GETIMAGELIST,0,0)
#define Header_OrderToIndex(hwnd,i) \
  (INT)SNDMSG((hwnd),HDM_ORDERTOINDEX,(WPARAM)i,0)
#define Header_CreateDragImage(hwnd,i) \
  (HIMAGELIST)SNDMSG((hwnd),HDM_CREATEDRAGIMAGE,(WPARAM)i,0)
#define Header_GetOrderArray(hwnd,iCount,lpi) \
  (BOOL)SNDMSG((hwnd),HDM_GETORDERARRAY,(WPARAM)iCount,(LPARAM)lpi)
#define Header_SetOrderArray(hwnd,iCount,lpi) \
  (BOOL)SNDMSG((hwnd),HDM_SETORDERARRAY,(WPARAM)iCount,(LPARAM)lpi)
#define Header_SetHotDivider(hwnd,fPos,dw) \
  (INT)SNDMSG((hwnd),HDM_SETHOTDIVIDER,(WPARAM)fPos,(LPARAM)dw)
#define Header_SetUnicodeFormat(hwnd,fUnicode) \
  (BOOL)SNDMSG((hwnd),HDM_SETUNICODEFORMAT,(WPARAM)(fUnicode),0)
#define Header_GetUnicodeFormat(hwnd) \
  (BOOL)SNDMSG((hwnd),HDM_GETUNICODEFORMAT,0,0)
#define Header_GetItemDropDownRect(hwnd,iItem,lprc) \
  (BOOL)SNDMSG((hwnd), HDM_GETITEMDROPDOWNRECT, (WPARAM)iItem, (LPARAM)lprc)
#define Header_GetOverflowRect(hwnd, rc) \
  (BOOL)SNDMSG((hwnd), HDM_GETOVERFLOWRECT, 0, (LPARAM)(rc))
#define Header_GetFocusedItem(hwnd) \
  (INT)SNDMSG((hwnd), HDM_GETFOCUSEDITEM, (WPARAM)(0), (LPARAM)(0))
#define Header_SetFocusedItem(hwnd, item) \
  (BOOL)SNDMSG((hwnd), HDM_SETFOCUSEDITEM, (WPARAM)(0), (LPARAM)(item))

/* Win32 5.1 Button Theme */
#define WC_BUTTONA       "Button"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_BUTTONW      L"Button"
#else
static const WCHAR WC_BUTTONW[] = { 'B','u','t','t','o','n',0 };
#endif
#define WC_BUTTON WINELIB_NAME_AW(WC_BUTTON)

#define BCN_FIRST               (0U-1250U)
#define BCN_LAST                (0U-1350U)

#define BCN_HOTITEMCHANGE       (BCN_FIRST + 1)
#define BCN_DROPDOWN            (BCN_FIRST + 2)
#define NM_GETCUSTOMSPLITRECT   (BCN_FIRST + 3)

#define BCM_FIRST               0x1600
#define BCM_GETIDEALSIZE        (BCM_FIRST + 1)
#define BCM_SETIMAGELIST        (BCM_FIRST + 2)
#define BCM_GETIMAGELIST        (BCM_FIRST + 3)
#define BCM_SETTEXTMARGIN       (BCM_FIRST + 4)
#define BCM_GETTEXTMARGIN       (BCM_FIRST + 5)
#define BCM_SETDROPDOWNSTATE    (BCM_FIRST + 6)
#define BCM_SETSPLITINFO        (BCM_FIRST + 7)
#define BCM_GETSPLITINFO        (BCM_FIRST + 8)
#define BCM_SETNOTE             (BCM_FIRST + 9)
#define BCM_GETNOTE             (BCM_FIRST + 10)
#define BCM_GETNOTELENGTH       (BCM_FIRST + 11)
#define BCM_SETSHIELD           (BCM_FIRST + 12)

#define BUTTON_IMAGELIST_ALIGN_LEFT      0
#define BUTTON_IMAGELIST_ALIGN_RIGHT     1
#define BUTTON_IMAGELIST_ALIGN_TOP       2
#define BUTTON_IMAGELIST_ALIGN_BOTTOM    3
#define BUTTON_IMAGELIST_ALIGN_CENTER    4

#define BCCL_NOGLYPH    (HIMAGELIST)(-1)

typedef struct
{
    HIMAGELIST himl;
    RECT margin;
    UINT uAlign;
} BUTTON_IMAGELIST, *PBUTTON_IMAGELIST;

typedef struct tagBUTTON_SPLITINFO
{
    UINT mask;
    HIMAGELIST himlGlyph;
    UINT uSplitStyle;
    SIZE size;
} BUTTON_SPLITINFO, *PBUTTON_SPLITINFO;

typedef struct tagNMBCDROPDOWN
{
    NMHDR hdr;
    RECT rcButton;
} NMBCDROPDOWN;

typedef struct tagNMBCHOTITEM
{
  NMHDR hdr;
  DWORD dwFlags;
} NMBCHOTITEM, *LPNMBCHOTITEM;

#define BST_HOT                 0x0200
#define BST_DROPDOWNPUSHED      0x0400

/* Button control styles for _WIN32_WINNT >= 0x600 */
#define BS_SPLITBUTTON          0x0000000C
#define BS_DEFSPLITBUTTON       0x0000000D
#define BS_COMMANDLINK          0x0000000E
#define BS_DEFCOMMANDLINK       0x0000000F

#define BCSIF_GLYPH             0x00000001
#define BCSIF_IMAGE             0x00000002
#define BCSIF_STYLE             0x00000004
#define BCSIF_SIZE              0x00000008

#define BCSS_NOSPLIT            0x00000001
#define BCSS_STRETCH            0x00000002
#define BCSS_ALIGNLEFT          0x00000004
#define BCSS_IMAGE              0x00000008

/* Button macros */
#define Button_SetNote(button, note)  \
  (BOOL)SNDMSG(button, BCM_SETNOTE, 0, (LPARAM)(note))
#define Button_GetNote(button, buffer, size)  \
  (BOOL)SNDMSG(button, BCM_GETNOTE, (WPARAM)(size), (LPARAM)(buffer))
#define Button_GetNoteLength(button)  \
  (LRESULT)SNDMSG(button, BCM_GETNOTELENGTH, 0, 0)
#define Button_GetImageList(button, image_list)  \
  (BOOL)SNDMSG(button, BCM_GETIMAGELIST, 0, (LPARAM)(image_list))
#define Button_SetImageList(button, image_list)  \
  (BOOL)SNDMSG(button, BCM_SETIMAGELIST, 0, (LPARAM)(image_list))
#define Button_GetTextMargin(button, margin)  \
  (BOOL)SNDMSG(button, BCM_GETTEXTMARGIN, 0, (LPARAM)(margin))
#define Button_SetTextMargin(button, margin)  \
  (BOOL)SNDMSG(button, BCM_SETTEXTMARGIN, 0, (LPARAM)(margin))
#define Button_GetIdealSize(button, size)  \
  (BOOL)SNDMSG(button, BCM_GETIDEALSIZE, 0, (LPARAM)(size))
#define Button_SetSplitInfo(hwnd, info) \
  (BOOL)SNDMSG((hwnd), BCM_SETSPLITINFO, 0, (LPARAM)(info))
#define Button_GetSplitInfo(hwnd, info) \
  (BOOL)SNDMSG((hwnd), BCM_GETSPLITINFO, 0, (LPARAM)(info))
#define Button_SetElevationRequiredState(hwnd, required) \
  (LRESULT)SNDMSG((hwnd), BCM_SETSHIELD, 0, (LPARAM)required)
#define Button_SetDropDownState(hwnd, dropdown) \
  (BOOL)SNDMSG((hwnd), BCM_SETDROPDOWNSTATE, (WPARAM)(dropdown), 0)

/* Toolbar */

#define TOOLBARCLASSNAMEA       "ToolbarWindow32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define TOOLBARCLASSNAMEW      L"ToolbarWindow32"
#else
static const WCHAR TOOLBARCLASSNAMEW[] = { 'T','o','o','l','b','a','r',
  'W','i','n','d','o','w','3','2',0 };
#endif
#define TOOLBARCLASSNAME WINELIB_NAME_AW(TOOLBARCLASSNAME)

#define CMB_MASKED              0x02

#define TBSTATE_CHECKED         0x01
#define TBSTATE_PRESSED         0x02
#define TBSTATE_ENABLED         0x04
#define TBSTATE_HIDDEN          0x08
#define TBSTATE_INDETERMINATE   0x10
#define TBSTATE_WRAP            0x20
#define TBSTATE_ELLIPSES        0x40
#define TBSTATE_MARKED          0x80


/* as of _WIN32_IE >= 0x0500 the following symbols are obsolete,
 * "everyone" should use the BTNS_... stuff below
 */
#define TBSTYLE_BUTTON          0x00
#define TBSTYLE_SEP             0x01
#define TBSTYLE_CHECK           0x02
#define TBSTYLE_GROUP           0x04
#define TBSTYLE_CHECKGROUP      (TBSTYLE_GROUP | TBSTYLE_CHECK)
#define TBSTYLE_DROPDOWN        0x08
#define TBSTYLE_AUTOSIZE        0x10
#define TBSTYLE_NOPREFIX        0x20
#define BTNS_BUTTON             TBSTYLE_BUTTON
#define BTNS_SEP                TBSTYLE_SEP
#define BTNS_CHECK              TBSTYLE_CHECK
#define BTNS_GROUP              TBSTYLE_GROUP
#define BTNS_CHECKGROUP         TBSTYLE_CHECKGROUP
#define BTNS_DROPDOWN           TBSTYLE_DROPDOWN
#define BTNS_AUTOSIZE           TBSTYLE_AUTOSIZE
#define BTNS_NOPREFIX           TBSTYLE_NOPREFIX
#define BTNS_SHOWTEXT           0x40  /* ignored unless TBSTYLE_EX_MIXEDB set */
#define BTNS_WHOLEDROPDOWN      0x80  /* draw dropdown arrow, but without split arrow section */

#define TBSTYLE_TOOLTIPS        0x0100
#define TBSTYLE_WRAPABLE        0x0200
#define TBSTYLE_ALTDRAG         0x0400
#define TBSTYLE_FLAT            0x0800
#define TBSTYLE_LIST            0x1000
#define TBSTYLE_CUSTOMERASE     0x2000
#define TBSTYLE_REGISTERDROP    0x4000
#define TBSTYLE_TRANSPARENT     0x8000
#define TBSTYLE_EX_DRAWDDARROWS         0x00000001
#define TBSTYLE_EX_MULTICOLUMN          0x00000002
#define TBSTYLE_EX_VERTICAL             0x00000004
#define TBSTYLE_EX_MIXEDBUTTONS         0x00000008
#define TBSTYLE_EX_HIDECLIPPEDBUTTONS   0x00000010 /* don't show partially obscured buttons */
#define TBSTYLE_EX_DOUBLEBUFFER         0x00000080 /* Double Buffer the toolbar */

#define TBIF_IMAGE              0x00000001
#define TBIF_TEXT               0x00000002
#define TBIF_STATE              0x00000004
#define TBIF_STYLE              0x00000008
#define TBIF_LPARAM             0x00000010
#define TBIF_COMMAND            0x00000020
#define TBIF_SIZE               0x00000040
#define TBIF_BYINDEX            0x80000000

#define TBBF_LARGE		0x0001

#define TB_ENABLEBUTTON          (WM_USER+1)
#define TB_CHECKBUTTON           (WM_USER+2)
#define TB_PRESSBUTTON           (WM_USER+3)
#define TB_HIDEBUTTON            (WM_USER+4)
#define TB_INDETERMINATE         (WM_USER+5)
#define TB_MARKBUTTON		 (WM_USER+6)
#define TB_ISBUTTONENABLED       (WM_USER+9)
#define TB_ISBUTTONCHECKED       (WM_USER+10)
#define TB_ISBUTTONPRESSED       (WM_USER+11)
#define TB_ISBUTTONHIDDEN        (WM_USER+12)
#define TB_ISBUTTONINDETERMINATE (WM_USER+13)
#define TB_ISBUTTONHIGHLIGHTED   (WM_USER+14)
#define TB_SETSTATE              (WM_USER+17)
#define TB_GETSTATE              (WM_USER+18)
#define TB_ADDBITMAP             (WM_USER+19)
#define TB_ADDBUTTONSA           (WM_USER+20)
#define TB_ADDBUTTONSW           (WM_USER+68)
#define TB_ADDBUTTONS            WINELIB_NAME_AW(TB_ADDBUTTONS)
#define TB_HITTEST               (WM_USER+69)
#define TB_INSERTBUTTONA         (WM_USER+21)
#define TB_INSERTBUTTONW         (WM_USER+67)
#define TB_INSERTBUTTON          WINELIB_NAME_AW(TB_INSERTBUTTON)
#define TB_DELETEBUTTON          (WM_USER+22)
#define TB_GETBUTTON             (WM_USER+23)
#define TB_BUTTONCOUNT           (WM_USER+24)
#define TB_COMMANDTOINDEX        (WM_USER+25)
#define TB_SAVERESTOREA          (WM_USER+26)
#define TB_SAVERESTOREW          (WM_USER+76)
#define TB_SAVERESTORE           WINELIB_NAME_AW(TB_SAVERESTORE)
#define TB_CUSTOMIZE             (WM_USER+27)
#define TB_ADDSTRINGA            (WM_USER+28)
#define TB_ADDSTRINGW            (WM_USER+77)
#define TB_ADDSTRING             WINELIB_NAME_AW(TB_ADDSTRING)
#define TB_GETITEMRECT           (WM_USER+29)
#define TB_BUTTONSTRUCTSIZE      (WM_USER+30)
#define TB_SETBUTTONSIZE         (WM_USER+31)
#define TB_SETBITMAPSIZE         (WM_USER+32)
#define TB_AUTOSIZE              (WM_USER+33)
#define TB_GETTOOLTIPS           (WM_USER+35)
#define TB_SETTOOLTIPS           (WM_USER+36)
#define TB_SETPARENT             (WM_USER+37)
#define TB_SETROWS               (WM_USER+39)
#define TB_GETROWS               (WM_USER+40)
#define TB_GETBITMAPFLAGS        (WM_USER+41)
#define TB_SETCMDID              (WM_USER+42)
#define TB_CHANGEBITMAP          (WM_USER+43)
#define TB_GETBITMAP             (WM_USER+44)
#define TB_GETBUTTONTEXTA        (WM_USER+45)
#define TB_GETBUTTONTEXTW        (WM_USER+75)
#define TB_GETBUTTONTEXT         WINELIB_NAME_AW(TB_GETBUTTONTEXT)
#define TB_REPLACEBITMAP         (WM_USER+46)
#define TB_SETINDENT             (WM_USER+47)
#define TB_SETIMAGELIST          (WM_USER+48)
#define TB_GETIMAGELIST          (WM_USER+49)
#define TB_LOADIMAGES            (WM_USER+50)
#define TB_GETRECT               (WM_USER+51) /* wParam is the Cmd instead of index */
#define TB_SETHOTIMAGELIST       (WM_USER+52)
#define TB_GETHOTIMAGELIST       (WM_USER+53)
#define TB_SETDISABLEDIMAGELIST  (WM_USER+54)
#define TB_GETDISABLEDIMAGELIST  (WM_USER+55)
#define TB_SETSTYLE              (WM_USER+56)
#define TB_GETSTYLE              (WM_USER+57)
#define TB_GETBUTTONSIZE         (WM_USER+58)
#define TB_SETBUTTONWIDTH        (WM_USER+59)
#define TB_SETMAXTEXTROWS        (WM_USER+60)
#define TB_GETTEXTROWS           (WM_USER+61)
#define TB_GETOBJECT             (WM_USER+62)
#define TB_GETBUTTONINFOW        (WM_USER+63)
#define TB_GETBUTTONINFOA        (WM_USER+65)
#define TB_GETBUTTONINFO         WINELIB_NAME_AW(TB_GETBUTTONINFO)
#define TB_SETBUTTONINFOW        (WM_USER+64)
#define TB_SETBUTTONINFOA        (WM_USER+66)
#define TB_SETBUTTONINFO         WINELIB_NAME_AW(TB_SETBUTTONINFO)
#define TB_SETDRAWTEXTFLAGS      (WM_USER+70)
#define TB_GETHOTITEM            (WM_USER+71)
#define TB_SETHOTITEM            (WM_USER+72)
#define TB_SETANCHORHIGHLIGHT    (WM_USER+73)
#define TB_GETANCHORHIGHLIGHT    (WM_USER+74)
#define TB_MAPACCELERATORA       (WM_USER+78)
#define TB_MAPACCELERATORW       (WM_USER+90)
#define TB_MAPACCELERATOR        WINELIB_NAME_AW(TB_MAPACCELERATOR)
#define TB_GETINSERTMARK         (WM_USER+79)
#define TB_SETINSERTMARK         (WM_USER+80)
#define TB_INSERTMARKHITTEST     (WM_USER+81)
#define TB_MOVEBUTTON            (WM_USER+82)
#define TB_GETMAXSIZE            (WM_USER+83)
#define TB_SETEXTENDEDSTYLE      (WM_USER+84)
#define TB_GETEXTENDEDSTYLE      (WM_USER+85)
#define TB_GETPADDING            (WM_USER+86)
#define TB_SETPADDING            (WM_USER+87)
#define TB_SETINSERTMARKCOLOR    (WM_USER+88)
#define TB_GETINSERTMARKCOLOR    (WM_USER+89)
#define TB_SETCOLORSCHEME        CCM_SETCOLORSCHEME
#define TB_GETCOLORSCHEME        CCM_GETCOLORSCHEME
#define TB_SETUNICODEFORMAT      CCM_SETUNICODEFORMAT
#define TB_GETUNICODEFORMAT      CCM_GETUNICODEFORMAT
#define TB_GETSTRINGW            (WM_USER+91)
#define TB_GETSTRINGA            (WM_USER+92)
#define TB_GETSTRING             WINELIB_NAME_AW(TB_GETSTRING)
#define TB_SETBOUNDINGSIZE       (WM_USER+93)
#define TB_SETHOTITEM2           (WM_USER+94)
#define TB_HASACCELERATOR        (WM_USER+95)
#define TB_SETLISTGAP            (WM_USER+96)
#define TB_GETIMAGELISTCOUNT     (WM_USER+98)
#define TB_GETIDEALSIZE          (WM_USER+99)

/* undocumented messages in Toolbar */
#ifdef __WINESRC__
#define TB_UNKWN464              (WM_USER+100)
#endif

#define TB_GETMETRICS            (WM_USER+101)
#define TB_SETMETRICS            (WM_USER+102)
#define TB_GETITEMDROPDOWNRECT   (WM_USER+103)
#define TB_SETPRESSEDIMAGELIST   (WM_USER+104)
#define TB_GETPRESSEDIMAGELIST   (WM_USER+105)
#define TB_SETWINDOWTHEME        CCM_SETWINDOWTHEME

#define TBN_FIRST               (0U-700U)
#define TBN_LAST                (0U-720U)
#define TBN_GETBUTTONINFOA      (TBN_FIRST-0)
#define TBN_GETBUTTONINFOW      (TBN_FIRST-20)
#define TBN_GETBUTTONINFO       WINELIB_NAME_AW(TBN_GETBUTTONINFO)
#define TBN_BEGINDRAG		(TBN_FIRST-1)
#define TBN_ENDDRAG		(TBN_FIRST-2)
#define TBN_BEGINADJUST		(TBN_FIRST-3)
#define TBN_ENDADJUST		(TBN_FIRST-4)
#define TBN_RESET		(TBN_FIRST-5)
#define TBN_QUERYINSERT		(TBN_FIRST-6)
#define TBN_QUERYDELETE		(TBN_FIRST-7)
#define TBN_TOOLBARCHANGE	(TBN_FIRST-8)
#define TBN_CUSTHELP		(TBN_FIRST-9)
#define TBN_DROPDOWN		(TBN_FIRST-10)
#define TBN_GETOBJECT		(TBN_FIRST-12)
#define TBN_HOTITEMCHANGE	(TBN_FIRST-13)
#define TBN_DRAGOUT		(TBN_FIRST-14)
#define TBN_DELETINGBUTTON	(TBN_FIRST-15)
#define TBN_GETDISPINFOA	(TBN_FIRST-16)
#define TBN_GETDISPINFOW	(TBN_FIRST-17)
#define TBN_GETDISPINFO		WINELIB_NAME_AW(TBN_GETDISPINFO)
#define TBN_GETINFOTIPA         (TBN_FIRST-18)
#define TBN_GETINFOTIPW         (TBN_FIRST-19)
#define TBN_GETINFOTIP          WINELIB_NAME_AW(TBN_GETINFOTIP)
#define TBN_RESTORE	        (TBN_FIRST-21)
#define TBN_SAVE	        (TBN_FIRST-22)
#define TBN_INITCUSTOMIZE	(TBN_FIRST-23)
#define TBN_WRAPHOTITEM         (TBN_FIRST-24)
#define TBN_DUPACCELERATOR      (TBN_FIRST-25)
#define TBN_WRAPACCELERATOR     (TBN_FIRST-26)
#define TBN_DRAGOVER            (TBN_FIRST-27)
#define TBN_MAPACCELERATOR      (TBN_FIRST-28)
#define TBNRF_HIDEHELP		0x00000001


/* Return values from TBN_DROPDOWN */
#define TBDDRET_DEFAULT  0
#define TBDDRET_NODEFAULT  1
#define TBDDRET_TREATPRESSED  2

typedef struct _NMTBCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    HBRUSH hbrMonoDither;
    HBRUSH hbrLines;
    HPEN hpenLines;
    COLORREF clrText;
    COLORREF clrMark;
    COLORREF clrTextHighlight;
    COLORREF clrBtnFace;
    COLORREF clrBtnHighlight;
    COLORREF clrHighlightHotTrack;
    RECT rcText;
    int nStringBkMode;
    int nHLStringBkMode;
    int iListGap;
} NMTBCUSTOMDRAW, *LPNMTBCUSTOMDRAW;

/* return flags for Toolbar NM_CUSTOMDRAW notifications */
#define TBCDRF_NOEDGES        0x00010000  /* Don't draw button edges       */
#define TBCDRF_HILITEHOTTRACK 0x00020000  /* Use color of the button bkgnd */
                                          /* when hottracked               */
#define TBCDRF_NOOFFSET       0x00040000  /* No offset button if pressed   */
#define TBCDRF_NOMARK         0x00080000  /* Don't draw default highlight  */
                                          /* for TBSTATE_MARKED            */
#define TBCDRF_NOETCHEDEFFECT 0x00100000  /* No etched effect for          */
                                          /* disabled items                */
#define TBCDRF_BLENDICON      0x00200000  /* ILD_BLEND50 on the icon image */
#define TBCDRF_NOBACKGROUND   0x00400000  /* Don't draw button background  */
#define TBCDRF_USECDCOLORS    0x00800000


/* This is just for old CreateToolbar. */
/* Don't use it in new programs. */
typedef struct _OLDTBBUTTON {
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
    BYTE  bReserved[2];
    DWORD dwData;
} OLDTBBUTTON, *POLDTBBUTTON, *LPOLDTBBUTTON;
typedef const OLDTBBUTTON *LPCOLDTBBUTTON;


typedef struct _TBBUTTON {
    INT iBitmap;
    INT idCommand;
    BYTE  fsState;
    BYTE  fsStyle;
#ifdef _WIN64
    BYTE  bReserved[6];
#else
    BYTE  bReserved[2];
#endif
    DWORD_PTR dwData;
    INT_PTR iString;
} TBBUTTON, *PTBBUTTON, *LPTBBUTTON;
typedef const TBBUTTON *LPCTBBUTTON;


typedef struct _COLORMAP {
    COLORREF from;
    COLORREF to;
} COLORMAP, *LPCOLORMAP;


typedef struct tagTBADDBITMAP {
    HINSTANCE hInst;
    UINT_PTR  nID;
} TBADDBITMAP, *LPTBADDBITMAP;

#define HINST_COMMCTRL         ((HINSTANCE)-1)
#define IDB_STD_SMALL_COLOR     0
#define IDB_STD_LARGE_COLOR     1
#define IDB_VIEW_SMALL_COLOR    4
#define IDB_VIEW_LARGE_COLOR    5
#define IDB_HIST_SMALL_COLOR    8
#define IDB_HIST_LARGE_COLOR    9
#define IDB_HIST_NORMAL        12
#define IDB_HIST_HOT           13
#define IDB_HIST_DISABLED      14
#define IDB_HIST_PRESSED       15

#define STD_CUT                 0
#define STD_COPY                1
#define STD_PASTE               2
#define STD_UNDO                3
#define STD_REDOW               4
#define STD_DELETE              5
#define STD_FILENEW             6
#define STD_FILEOPEN            7
#define STD_FILESAVE            8
#define STD_PRINTPRE            9
#define STD_PROPERTIES          10
#define STD_HELP                11
#define STD_FIND                12
#define STD_REPLACE             13
#define STD_PRINT               14

#define VIEW_LARGEICONS         0
#define VIEW_SMALLICONS         1
#define VIEW_LIST               2
#define VIEW_DETAILS            3
#define VIEW_SORTNAME           4
#define VIEW_SORTSIZE           5
#define VIEW_SORTDATE           6
#define VIEW_SORTTYPE           7
#define VIEW_PARENTFOLDER       8
#define VIEW_NETCONNECT         9
#define VIEW_NETDISCONNECT      10
#define VIEW_NEWFOLDER          11
#define VIEW_VIEWMENU           12

#define HIST_BACK               0
#define HIST_FORWARD            1
#define HIST_FAVORITES          2
#define HIST_ADDTOFAVORITES     3
#define HIST_VIEWTREE           4

typedef struct tagTBSAVEPARAMSA {
    HKEY   hkr;
    LPCSTR pszSubKey;
    LPCSTR pszValueName;
} TBSAVEPARAMSA, *LPTBSAVEPARAMSA;

typedef struct tagTBSAVEPARAMSW {
    HKEY   hkr;
    LPCWSTR pszSubKey;
    LPCWSTR pszValueName;
} TBSAVEPARAMSW, *LPTBSAVEPARAMSW;

#define TBSAVEPARAMS   WINELIB_NAME_AW(TBSAVEPARAMS)
#define LPTBSAVEPARAMS WINELIB_NAME_AW(LPTBSAVEPARAMS)

typedef struct
{
    UINT cbSize;
    DWORD  dwMask;
    INT  idCommand;
    INT  iImage;
    BYTE   fsState;
    BYTE   fsStyle;
    WORD   cx;
    DWORD_PTR lParam;
    LPSTR  pszText;
    INT  cchText;
} TBBUTTONINFOA, *LPTBBUTTONINFOA;

typedef struct
{
    UINT cbSize;
    DWORD  dwMask;
    INT  idCommand;
    INT  iImage;
    BYTE   fsState;
    BYTE   fsStyle;
    WORD   cx;
    DWORD_PTR lParam;
    LPWSTR pszText;
    INT  cchText;
} TBBUTTONINFOW, *LPTBBUTTONINFOW;

#define TBBUTTONINFO   WINELIB_NAME_AW(TBBUTTONINFO)
#define LPTBBUTTONINFO WINELIB_NAME_AW(LPTBBUTTONINFO)

typedef struct tagNMTBHOTITEM
{
    NMHDR hdr;
    int idOld;
    int idNew;
    DWORD dwFlags;
} NMTBHOTITEM, *LPNMTBHOTITEM;

typedef struct tagNMTBGETINFOTIPA
{
    NMHDR  hdr;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iItem;
    LPARAM lParam;
} NMTBGETINFOTIPA, *LPNMTBGETINFOTIPA;

typedef struct tagNMTBGETINFOTIPW
{
    NMHDR  hdr;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iItem;
    LPARAM lParam;
} NMTBGETINFOTIPW, *LPNMTBGETINFOTIPW;

#define NMTBGETINFOTIP   WINELIB_NAME_AW(NMTBGETINFOTIP)
#define LPNMTBGETINFOTIP WINELIB_NAME_AW(LPNMTBGETINFOTIP)

typedef struct
{
    NMHDR hdr;
    DWORD dwMask;
    int idCommand;
    DWORD_PTR lParam;
    int iImage;
    LPSTR pszText;
    int cchText;
} NMTBDISPINFOA, *LPNMTBDISPINFOA;

typedef struct
{
    NMHDR hdr;
    DWORD dwMask;
    int idCommand;
    DWORD_PTR lParam;
    int iImage;
    LPWSTR pszText;
    int cchText;
} NMTBDISPINFOW, *LPNMTBDISPINFOW;

#define NMTBDISPINFO WINELIB_NAME_AW(NMTBDISPINFO)
#define LPNMTBDISPINFO WINELIB_NAME_AW(LPNMTBDISPINFO)

/* contents of dwMask in the NMTBDISPINFO structure */
#define TBNF_IMAGE     0x00000001
#define TBNF_TEXT      0x00000002
#define TBNF_DI_SETITEM  0x10000000


typedef struct tagNMTOOLBARA
{
    NMHDR    hdr;
    INT      iItem;
    TBBUTTON tbButton;
    INT      cchText;
    LPSTR    pszText;
    RECT     rcButton; /* Version 5.80 */
} NMTOOLBARA, *LPNMTOOLBARA, TBNOTIFYA, *LPTBNOTIFYA;

typedef struct tagNMTOOLBARW
{
    NMHDR    hdr;
    INT      iItem;
    TBBUTTON tbButton;
    INT      cchText;
    LPWSTR   pszText;
    RECT     rcButton; /* Version 5.80 */
} NMTOOLBARW, *LPNMTOOLBARW, TBNOTIFYW, *LPTBNOTIFYW;

#define NMTOOLBAR   WINELIB_NAME_AW(NMTOOLBAR)
#define LPNMTOOLBAR WINELIB_NAME_AW(LPNMTOOLBAR)
#define TBNOTIFY    WINELIB_NAME_AW(TBNOTIFY)
#define LPTBNOTIFY  WINELIB_NAME_AW(LPTBNOTIFY)

typedef struct
{
	HINSTANCE hInstOld;
	UINT_PTR  nIDOld;
	HINSTANCE hInstNew;
	UINT_PTR  nIDNew;
	INT       nButtons;
} TBREPLACEBITMAP, *LPTBREPLACEBITMAP;

#define HICF_OTHER          0x00000000
#define HICF_MOUSE          0x00000001   /* Triggered by mouse             */
#define HICF_ARROWKEYS      0x00000002   /* Triggered by arrow keys        */
#define HICF_ACCELERATOR    0x00000004   /* Triggered by accelerator       */
#define HICF_DUPACCEL       0x00000008   /* This accelerator is not unique */
#define HICF_ENTERING       0x00000010   /* idOld is invalid               */
#define HICF_LEAVING        0x00000020   /* idNew is invalid               */
#define HICF_RESELECT       0x00000040   /* hot item reselected            */
#define HICF_LMOUSE         0x00000080   /* left mouse button selected     */
#define HICF_TOGGLEDROPDOWN 0x00000100   /* Toggle button's dropdown state */

typedef struct
{
    int   iButton;
    DWORD dwFlags;
} TBINSERTMARK, *LPTBINSERTMARK;
#define TBIMHT_AFTER      0x00000001 /* TRUE = insert After iButton, otherwise before */
#define TBIMHT_BACKGROUND 0x00000002 /* TRUE if and only if missed buttons completely */

typedef struct tagNMTBSAVE
{
    NMHDR hdr;
    DWORD* pData;
    DWORD* pCurrent;
    UINT cbData;
    int iItem;
    int cButtons;
    TBBUTTON tbButton;
} NMTBSAVE, *LPNMTBSAVE;

typedef struct tagNMTBRESTORE
{
    NMHDR hdr;
    DWORD* pData;
    DWORD* pCurrent;
    UINT cbData;
    int iItem;
    int cButtons;
    int cbBytesPerRecord;
    TBBUTTON tbButton;
} NMTBRESTORE, *LPNMTBRESTORE;

#define TBMF_PAD           0x00000001
#define TBMF_BARPAD        0x00000002
#define TBMF_BUTTONSPACING 0x00000004

typedef struct
{
    UINT cbSize;
    DWORD dwMask;
    INT cxPad;
    INT cyPad;
    INT cxBarPad;
    INT cyBarPad;
    INT cxButtonSpacing;
    INT cyButtonSpacing;
} TBMETRICS, *LPTBMETRICS;

/* these are undocumented and the names are guesses */
typedef struct
{
    NMHDR hdr;
    HWND hwndDialog;
} NMTBINITCUSTOMIZE;

typedef struct
{
    NMHDR hdr;
    INT idNew;
    INT iDirection; /* left is -1, right is 1 */
    DWORD dwReason; /* HICF_* */
} NMTBWRAPHOTITEM;


HWND WINAPI
CreateToolbar(HWND, DWORD, UINT, INT, HINSTANCE,
              UINT, LPCTBBUTTON, INT);

HWND WINAPI
CreateToolbarEx(HWND, DWORD, UINT, INT,
                HINSTANCE, UINT_PTR, LPCTBBUTTON,
                INT, INT, INT, INT, INT, UINT);

HBITMAP WINAPI
CreateMappedBitmap (HINSTANCE, INT_PTR, UINT, LPCOLORMAP, INT);


/* Tool tips */

#define TOOLTIPS_CLASSA         "tooltips_class32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define TOOLTIPS_CLASSW        L"tooltips_class32"
#else
static const WCHAR TOOLTIPS_CLASSW[] = { 't','o','o','l','t','i','p','s','_',
  'c','l','a','s','s','3','2',0 };
#endif
#define TOOLTIPS_CLASS          WINELIB_NAME_AW(TOOLTIPS_CLASS)

#define INFOTIPSIZE             1024


#define TTF_IDISHWND            0x0001
#define TTF_CENTERTIP           0x0002
#define TTF_RTLREADING          0x0004
#define TTF_SUBCLASS            0x0010
#define TTF_TRACK               0x0020
#define TTF_ABSOLUTE            0x0080
#define TTF_TRANSPARENT         0x0100
#define TTF_DI_SETITEM          0x8000  /* valid only on the TTN_NEEDTEXT callback */


#define TTDT_AUTOMATIC          0
#define TTDT_RESHOW             1
#define TTDT_AUTOPOP            2
#define TTDT_INITIAL            3


#define TTI_NONE                0
#define TTI_INFO                1
#define TTI_WARNING             2
#define TTI_ERROR               3
#define TTI_INFO_LARGE          4
#define TTI_WARNING_LARGE       5
#define TTI_ERROR_LARGE         6


#define TTM_ACTIVATE            (WM_USER+1)
#define TTM_SETDELAYTIME        (WM_USER+3)
#define TTM_ADDTOOLA            (WM_USER+4)
#define TTM_ADDTOOLW            (WM_USER+50)
#define TTM_ADDTOOL             WINELIB_NAME_AW(TTM_ADDTOOL)
#define TTM_DELTOOLA            (WM_USER+5)
#define TTM_DELTOOLW            (WM_USER+51)
#define TTM_DELTOOL             WINELIB_NAME_AW(TTM_DELTOOL)
#define TTM_NEWTOOLRECTA        (WM_USER+6)
#define TTM_NEWTOOLRECTW        (WM_USER+52)
#define TTM_NEWTOOLRECT         WINELIB_NAME_AW(TTM_NEWTOOLRECT)
#define TTM_RELAYEVENT          (WM_USER+7)
#define TTM_GETTOOLINFOA        (WM_USER+8)
#define TTM_GETTOOLINFOW        (WM_USER+53)
#define TTM_GETTOOLINFO         WINELIB_NAME_AW(TTM_GETTOOLINFO)
#define TTM_SETTOOLINFOA        (WM_USER+9)
#define TTM_SETTOOLINFOW        (WM_USER+54)
#define TTM_SETTOOLINFO         WINELIB_NAME_AW(TTM_SETTOOLINFO)
#define TTM_HITTESTA            (WM_USER+10)
#define TTM_HITTESTW            (WM_USER+55)
#define TTM_HITTEST             WINELIB_NAME_AW(TTM_HITTEST)
#define TTM_GETTEXTA            (WM_USER+11)
#define TTM_GETTEXTW            (WM_USER+56)
#define TTM_GETTEXT             WINELIB_NAME_AW(TTM_GETTEXT)
#define TTM_UPDATETIPTEXTA      (WM_USER+12)
#define TTM_UPDATETIPTEXTW      (WM_USER+57)
#define TTM_UPDATETIPTEXT       WINELIB_NAME_AW(TTM_UPDATETIPTEXT)
#define TTM_GETTOOLCOUNT        (WM_USER+13)
#define TTM_ENUMTOOLSA          (WM_USER+14)
#define TTM_ENUMTOOLSW          (WM_USER+58)
#define TTM_ENUMTOOLS           WINELIB_NAME_AW(TTM_ENUMTOOLS)
#define TTM_GETCURRENTTOOLA     (WM_USER+15)
#define TTM_GETCURRENTTOOLW     (WM_USER+59)
#define TTM_GETCURRENTTOOL      WINELIB_NAME_AW(TTM_GETCURRENTTOOL)
#define TTM_WINDOWFROMPOINT     (WM_USER+16)
#define TTM_TRACKACTIVATE       (WM_USER+17)
#define TTM_TRACKPOSITION       (WM_USER+18)
#define TTM_SETTIPBKCOLOR       (WM_USER+19)
#define TTM_SETTIPTEXTCOLOR     (WM_USER+20)
#define TTM_GETDELAYTIME        (WM_USER+21)
#define TTM_GETTIPBKCOLOR       (WM_USER+22)
#define TTM_GETTIPTEXTCOLOR     (WM_USER+23)
#define TTM_SETMAXTIPWIDTH      (WM_USER+24)
#define TTM_GETMAXTIPWIDTH      (WM_USER+25)
#define TTM_SETMARGIN           (WM_USER+26)
#define TTM_GETMARGIN           (WM_USER+27)
#define TTM_POP                 (WM_USER+28)
#define TTM_UPDATE              (WM_USER+29)
#define TTM_GETBUBBLESIZE       (WM_USER+30)
#define TTM_ADJUSTRECT          (WM_USER+31)
#define TTM_SETTITLEA           (WM_USER+32)
#define TTM_SETTITLEW           (WM_USER+33)
#define TTM_SETTITLE            WINELIB_NAME_AW(TTM_SETTITLE)
#define TTM_POPUP               (WM_USER+34)
#define TTM_GETTITLE            (WM_USER+35)
#define TTM_SETWINDOWTHEME      CCM_SETWINDOWTHEME


#define TTN_FIRST               (0U-520U)
#define TTN_LAST                (0U-549U)
#define TTN_GETDISPINFOA        (TTN_FIRST-0)
#define TTN_GETDISPINFOW        (TTN_FIRST-10)
#define TTN_GETDISPINFO         WINELIB_NAME_AW(TTN_GETDISPINFO)
#define TTN_SHOW                (TTN_FIRST-1)
#define TTN_POP                 (TTN_FIRST-2)
#define TTN_LINKCLICK           (TTN_FIRST-3)

#define TTN_NEEDTEXT		TTN_GETDISPINFO
#define TTN_NEEDTEXTA 		TTN_GETDISPINFOA
#define TTN_NEEDTEXTW 		TTN_GETDISPINFOW

typedef struct tagTOOLINFOA {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT_PTR uId;
    RECT rect;
    HINSTANCE hinst;
    LPSTR lpszText;
    LPARAM lParam;
    void *lpReserved;
} TTTOOLINFOA, *LPTOOLINFOA, *PTOOLINFOA, *LPTTTOOLINFOA;

typedef struct tagTOOLINFOW {
    UINT cbSize;
    UINT uFlags;
    HWND hwnd;
    UINT_PTR uId;
    RECT rect;
    HINSTANCE hinst;
    LPWSTR lpszText;
    LPARAM lParam;
    void *lpReserved;
} TTTOOLINFOW, *LPTOOLINFOW, *PTOOLINFOW, *LPTTTOOLINFOW;

#define TTTOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define TOOLINFOA TTTOOLINFOA
#define TOOLINFOW TTTOOLINFOW
#define TOOLINFO WINELIB_NAME_AW(TTTOOLINFO)
#define PTOOLINFO WINELIB_NAME_AW(PTOOLINFO)
#define LPTTTOOLINFO WINELIB_NAME_AW(LPTTTOOLINFO)
#define LPTOOLINFO WINELIB_NAME_AW(LPTOOLINFO)

#define TTTOOLINFOA_V1_SIZE CCSIZEOF_STRUCT(TTTOOLINFOA, lpszText)
#define TTTOOLINFOW_V1_SIZE CCSIZEOF_STRUCT(TTTOOLINFOW, lpszText)
#define TTTOOLINFO_V1_SIZE  CCSIZEOF_STRUCT(WINELIB_NAME_AW(TTTOOLINFO), lpszText)
#define TTTOOLINFOA_V2_SIZE CCSIZEOF_STRUCT(TTTOOLINFOA, lParam)
#define TTTOOLINFOW_V2_SIZE CCSIZEOF_STRUCT(TTTOOLINFOW, lParam)
#define TTTOOLINFOA_V3_SIZE CCSIZEOF_STRUCT(TTTOOLINFOA, lpReserved)
#define TTTOOLINFOW_V3_SIZE CCSIZEOF_STRUCT(TTTOOLINFOW, lpReserved)

typedef struct _TT_HITTESTINFOA
{
    HWND        hwnd;
    POINT       pt;
    TTTOOLINFOA ti;
} TTHITTESTINFOA, *LPTTHITTESTINFOA;
#define LPHITTESTINFOA LPTTHITTESTINFOA

typedef struct _TT_HITTESTINFOW
{
    HWND        hwnd;
    POINT       pt;
    TTTOOLINFOW ti;
} TTHITTESTINFOW, *LPTTHITTESTINFOW;
#define LPHITTESTINFOW LPTTHITTESTINFOW

#define TTHITTESTINFO WINELIB_NAME_AW(TTHITTESTINFO)
#define LPTTHITTESTINFO WINELIB_NAME_AW(LPTTHITTESTINFO)
#define LPHITTESTINFO WINELIB_NAME_AW(LPHITTESTINFO)

typedef struct tagNMTTDISPINFOA
{
    NMHDR hdr;
    LPSTR lpszText;
    CHAR  szText[80];
    HINSTANCE hinst;
    UINT      uFlags;
    LPARAM      lParam;
} NMTTDISPINFOA, *LPNMTTDISPINFOA;

typedef struct tagNMTTDISPINFOW
{
    NMHDR       hdr;
    LPWSTR      lpszText;
    WCHAR       szText[80];
    HINSTANCE hinst;
    UINT      uFlags;
    LPARAM      lParam;
} NMTTDISPINFOW, *LPNMTTDISPINFOW;

#define NMTTDISPINFO WINELIB_NAME_AW(NMTTDISPINFO)
#define LPNMTTDISPINFO WINELIB_NAME_AW(LPNMTTDISPINFO)

#define NMTTDISPINFO_V1_SIZEA CCSIZEOF_STRUCT(NMTTDISPINFOA, uFlags)
#define NMTTDISPINFO_V1_SIZEW CCSIZEOF_STRUCT(NMTTDISPINFOW, uFlags)
#define NMTTDISPINFO_V1_SIZE WINELIB_NAME_AW(NMTTDISPINFO_V1_SIZE)

typedef struct _TTGETTITLE
{
    DWORD dwSize;
    UINT uTitleBitmap;
    UINT cch;
    WCHAR* pszTitle;
} TTGETTITLE, *PTTGETTITLE;

#define TOOLTIPTEXTW    NMTTDISPINFOW
#define TOOLTIPTEXTA    NMTTDISPINFOA
#define TOOLTIPTEXT     NMTTDISPINFO
#define LPTOOLTIPTEXTW  LPNMTTDISPINFOW
#define LPTOOLTIPTEXTA  LPNMTTDISPINFOA
#define LPTOOLTIPTEXT   LPNMTTDISPINFO


/* Rebar control */

#define REBARCLASSNAMEA         "ReBarWindow32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define REBARCLASSNAMEW        L"ReBarWindow32"
#else
static const WCHAR REBARCLASSNAMEW[] = { 'R','e','B','a','r',
  'W','i','n','d','o','w','3','2',0 };
#endif
#define REBARCLASSNAME          WINELIB_NAME_AW(REBARCLASSNAME)


#define RBIM_IMAGELIST          0x00000001

#define RBBIM_STYLE             0x00000001
#define RBBIM_COLORS            0x00000002
#define RBBIM_TEXT              0x00000004
#define RBBIM_IMAGE             0x00000008
#define RBBIM_CHILD             0x00000010
#define RBBIM_CHILDSIZE         0x00000020
#define RBBIM_SIZE              0x00000040
#define RBBIM_BACKGROUND        0x00000080
#define RBBIM_ID                0x00000100
#define RBBIM_IDEALSIZE         0x00000200
#define RBBIM_LPARAM            0x00000400
#define RBBIM_HEADERSIZE        0x00000800
#define RBBIM_CHEVRONLOCATION   0x00001000
#define RBBIM_CHEVRONSTATE      0x00002000

#define RBBS_BREAK              0x00000001
#define RBBS_FIXEDSIZE          0x00000002
#define RBBS_CHILDEDGE          0x00000004
#define RBBS_HIDDEN             0x00000008
#define RBBS_NOVERT             0x00000010
#define RBBS_FIXEDBMP           0x00000020
#define RBBS_VARIABLEHEIGHT     0x00000040
#define RBBS_GRIPPERALWAYS      0x00000080
#define RBBS_NOGRIPPER          0x00000100
#define RBBS_USECHEVRON         0x00000200
#define RBBS_HIDETITLE          0x00000400
#define RBBS_TOPALIGN           0x00000800

#define RBNM_ID                 0x00000001
#define RBNM_STYLE              0x00000002
#define RBNM_LPARAM             0x00000004

#define RBHT_NOWHERE            0x0001
#define RBHT_CAPTION            0x0002
#define RBHT_CLIENT             0x0003
#define RBHT_GRABBER            0x0004
#define RBHT_CHEVRON            0x0008
#define RBHT_SPLITTER           0x0010

#define RB_INSERTBANDA          (WM_USER+1)
#define RB_INSERTBANDW          (WM_USER+10)
#define RB_INSERTBAND           WINELIB_NAME_AW(RB_INSERTBAND)
#define RB_DELETEBAND           (WM_USER+2)
#define RB_GETBARINFO           (WM_USER+3)
#define RB_SETBARINFO           (WM_USER+4)
#define RB_SETBANDINFOA         (WM_USER+6)
#define RB_SETBANDINFOW         (WM_USER+11)
#define RB_SETBANDINFO          WINELIB_NAME_AW(RB_SETBANDINFO)
#define RB_SETPARENT            (WM_USER+7)
#define RB_HITTEST              (WM_USER+8)
#define RB_GETRECT              (WM_USER+9)
#define RB_GETBANDCOUNT         (WM_USER+12)
#define RB_GETROWCOUNT          (WM_USER+13)
#define RB_GETROWHEIGHT         (WM_USER+14)
#define RB_IDTOINDEX            (WM_USER+16)
#define RB_GETTOOLTIPS          (WM_USER+17)
#define RB_SETTOOLTIPS          (WM_USER+18)
#define RB_SETBKCOLOR           (WM_USER+19)
#define RB_GETBKCOLOR           (WM_USER+20)
#define RB_SETTEXTCOLOR         (WM_USER+21)
#define RB_GETTEXTCOLOR         (WM_USER+22)
#define RB_SIZETORECT           (WM_USER+23)
#define RB_BEGINDRAG            (WM_USER+24)
#define RB_ENDDRAG              (WM_USER+25)
#define RB_DRAGMOVE             (WM_USER+26)
#define RB_GETBARHEIGHT         (WM_USER+27)
#define RB_GETBANDINFOW         (WM_USER+28)
#define RB_GETBANDINFOA         (WM_USER+29)
#define RB_GETBANDINFO          WINELIB_NAME_AW(RB_GETBANDINFO)
#define RB_MINIMIZEBAND         (WM_USER+30)
#define RB_MAXIMIZEBAND         (WM_USER+31)
#define RB_GETBANDBORDERS       (WM_USER+34)
#define RB_SHOWBAND             (WM_USER+35)
#define RB_SETPALETTE           (WM_USER+37)
#define RB_GETPALETTE           (WM_USER+38)
#define RB_MOVEBAND             (WM_USER+39)
#define RB_GETBANDMARGINS       (WM_USER+40)
#define RB_SETEXTENDEDSTYLE     (WM_USER+41)
#define RB_GETEXTENDEDSTYLE     (WM_USER+42)
#define RB_PUSHCHEVRON          (WM_USER+43)
#define RB_SETBANDWIDTH         (WM_USER+44)

#define RB_GETDROPTARGET        CCM_GETDROPTARGET
#define RB_SETCOLORSCHEME       CCM_SETCOLORSCHEME
#define RB_GETCOLORSCHEME       CCM_GETCOLORSCHEME
#define RB_SETUNICODEFORMAT     CCM_SETUNICODEFORMAT
#define RB_GETUNICODEFORMAT     CCM_GETUNICODEFORMAT
#define RB_SETWINDOWTHEME       CCM_SETWINDOWTHEME

#define RBN_FIRST               (0U-831U)
#define RBN_LAST                (0U-859U)
#define RBN_HEIGHTCHANGE        (RBN_FIRST-0)
#define RBN_GETOBJECT           (RBN_FIRST-1)
#define RBN_LAYOUTCHANGED       (RBN_FIRST-2)
#define RBN_AUTOSIZE            (RBN_FIRST-3)
#define RBN_BEGINDRAG           (RBN_FIRST-4)
#define RBN_ENDDRAG             (RBN_FIRST-5)
#define RBN_DELETINGBAND        (RBN_FIRST-6)
#define RBN_DELETEDBAND         (RBN_FIRST-7)
#define RBN_CHILDSIZE           (RBN_FIRST-8)
#define RBN_CHEVRONPUSHED       (RBN_FIRST-10)
#define RBN_SPLITTERDRAG        (RBN_FIRST-11)
#define RBN_MINMAX              (RBN_FIRST-21)
#define RBN_AUTOBREAK           (RBN_FIRST-22)

typedef struct tagREBARINFO
{
    UINT     cbSize;
    UINT     fMask;
    HIMAGELIST himl;
} REBARINFO, *LPREBARINFO;

typedef struct tagREBARBANDINFOA
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    LPSTR     lpText;
    UINT    cch;
    INT     iImage;
    HWND    hwndChild;
    UINT    cxMinChild;
    UINT    cyMinChild;
    UINT    cx;
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;
    UINT    cyMaxChild;
    UINT    cyIntegral;
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;
    /* _WIN32_WINNT >= 0x0600 */
    RECT    rcChevronLocation;
    UINT    uChevronState;
} REBARBANDINFOA, *LPREBARBANDINFOA;

typedef REBARBANDINFOA const *LPCREBARBANDINFOA;

typedef struct tagREBARBANDINFOW
{
    UINT    cbSize;
    UINT    fMask;
    UINT    fStyle;
    COLORREF  clrFore;
    COLORREF  clrBack;
    LPWSTR    lpText;
    UINT    cch;
    INT     iImage;
    HWND    hwndChild;
    UINT    cxMinChild;
    UINT    cyMinChild;
    UINT    cx;
    HBITMAP hbmBack;
    UINT    wID;
    UINT    cyChild;
    UINT    cyMaxChild;
    UINT    cyIntegral;
    UINT    cxIdeal;
    LPARAM    lParam;
    UINT    cxHeader;
    /* _WIN32_WINNT >= 0x0600 */
    RECT    rcChevronLocation;
    UINT    uChevronState;
} REBARBANDINFOW, *LPREBARBANDINFOW;

typedef REBARBANDINFOW const *LPCREBARBANDINFOW;

#define REBARBANDINFO    WINELIB_NAME_AW(REBARBANDINFO)
#define LPREBARBANDINFO  WINELIB_NAME_AW(LPREBARBANDINFO)
#define LPCREBARBANDINFO WINELIB_NAME_AW(LPCREBARBANDINFO)

#define REBARBANDINFOA_V3_SIZE CCSIZEOF_STRUCT(REBARBANDINFOA, wID)
#define REBARBANDINFOW_V3_SIZE CCSIZEOF_STRUCT(REBARBANDINFOW, wID)
#define REBARBANDINFO_V3_SIZE  CCSIZEOF_STRUCT(WINELIB_NAME_AW(REBARBANDINFO), wID)
#define REBARBANDINFOA_V6_SIZE CCSIZEOF_STRUCT(REBARBANDINFOA, cxHeader)
#define REBARBANDINFOW_V6_SIZE CCSIZEOF_STRUCT(REBARBANDINFOW, cxHeader)
#define REBARBANDINFO_V6_SIZE  CCSIZEOF_STRUCT(WINELIB_NAME_AW(REBARBANDINFO), cxHeader)

typedef struct tagNMREBARCHILDSIZE
{
    NMHDR  hdr;
    UINT uBand;
    UINT wID;
    RECT rcChild;
    RECT rcBand;
} NMREBARCHILDSIZE, *LPNMREBARCHILDSIZE;

typedef struct tagNMREBAR
{
    NMHDR  hdr;
    DWORD  dwMask;
    UINT uBand;
    UINT fStyle;
    UINT wID;
    LPARAM lParam;
} NMREBAR, *LPNMREBAR;

typedef struct tagNMRBAUTOSIZE
{
    NMHDR hdr;
    BOOL fChanged;
    RECT rcTarget;
    RECT rcActual;
} NMRBAUTOSIZE, *LPNMRBAUTOSIZE;

typedef struct tagNMREBARCHEVRON
{
    NMHDR hdr;
    UINT uBand;
    UINT wID;
    LPARAM lParam;
    RECT rc;
    LPARAM lParamNM;
} NMREBARCHEVRON, *LPNMREBARCHEVRON;

typedef struct _RB_HITTESTINFO
{
    POINT pt;
    UINT  flags;
    INT   iBand;
} RBHITTESTINFO, *LPRBHITTESTINFO;

#define RBAB_AUTOSIZE   0x0001
#define RBAB_ADDBAND    0x0002

typedef struct tagNMREBARAUTOBREAK
{
    NMHDR hdr;
    UINT uBand;
    UINT wID;
    LPARAM lParam;
    UINT uMsg;
    UINT fStyleCurrent;
    BOOL fAutoBreak;
} NMREBARAUTOBREAK, *LPNMREBARAUTOBREAK;


/* Trackbar control */

#define TRACKBAR_CLASSA         "msctls_trackbar32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define TRACKBAR_CLASSW        L"msctls_trackbar32"
#else
static const WCHAR TRACKBAR_CLASSW[] = { 'm','s','c','t','l','s','_',
  't','r','a','c','k','b','a','r','3','2',0 };
#endif
#define TRACKBAR_CLASS  WINELIB_NAME_AW(TRACKBAR_CLASS)


#define TBTS_TOP                0
#define TBTS_LEFT               1
#define TBTS_BOTTOM             2
#define TBTS_RIGHT              3

#define TB_LINEUP               0
#define TB_LINEDOWN             1
#define TB_PAGEUP               2
#define TB_PAGEDOWN             3
#define TB_THUMBPOSITION        4
#define TB_THUMBTRACK           5
#define TB_TOP                  6
#define TB_BOTTOM               7
#define TB_ENDTRACK             8

#define TBCD_TICS               0x0001
#define TBCD_THUMB              0x0002
#define TBCD_CHANNEL            0x0003

#define TBM_GETPOS              (WM_USER)
#define TBM_GETRANGEMIN         (WM_USER+1)
#define TBM_GETRANGEMAX         (WM_USER+2)
#define TBM_GETTIC              (WM_USER+3)
#define TBM_SETTIC              (WM_USER+4)
#define TBM_SETPOS              (WM_USER+5)
#define TBM_SETRANGE            (WM_USER+6)
#define TBM_SETRANGEMIN         (WM_USER+7)
#define TBM_SETRANGEMAX         (WM_USER+8)
#define TBM_CLEARTICS           (WM_USER+9)
#define TBM_SETSEL              (WM_USER+10)
#define TBM_SETSELSTART         (WM_USER+11)
#define TBM_SETSELEND           (WM_USER+12)
#define TBM_GETPTICS            (WM_USER+14)
#define TBM_GETTICPOS           (WM_USER+15)
#define TBM_GETNUMTICS          (WM_USER+16)
#define TBM_GETSELSTART         (WM_USER+17)
#define TBM_GETSELEND           (WM_USER+18)
#define TBM_CLEARSEL            (WM_USER+19)
#define TBM_SETTICFREQ          (WM_USER+20)
#define TBM_SETPAGESIZE         (WM_USER+21)
#define TBM_GETPAGESIZE         (WM_USER+22)
#define TBM_SETLINESIZE         (WM_USER+23)
#define TBM_GETLINESIZE         (WM_USER+24)
#define TBM_GETTHUMBRECT        (WM_USER+25)
#define TBM_GETCHANNELRECT      (WM_USER+26)
#define TBM_SETTHUMBLENGTH      (WM_USER+27)
#define TBM_GETTHUMBLENGTH      (WM_USER+28)
#define TBM_SETTOOLTIPS         (WM_USER+29)
#define TBM_GETTOOLTIPS         (WM_USER+30)
#define TBM_SETTIPSIDE          (WM_USER+31)
#define TBM_SETBUDDY            (WM_USER+32)
#define TBM_GETBUDDY            (WM_USER+33)
#define TBM_SETPOSNOTIFY        (WM_USER+34)
#define TBM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define TBM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT

#define TRBN_FIRST              (0u-1501u)
#define TRBN_LAST               (0u-1519u)
#define TRBN_THUMBPOSCHANGING   (TRBN_FIRST-1)

/* Pager control */

#define WC_PAGESCROLLERA      "SysPager"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_PAGESCROLLERW     L"SysPager"
#else
static const WCHAR WC_PAGESCROLLERW[] = { 'S','y','s','P','a','g','e','r',0 };
#endif
#define WC_PAGESCROLLER  WINELIB_NAME_AW(WC_PAGESCROLLER)


#define PGF_INVISIBLE           0
#define PGF_NORMAL              1
#define PGF_GRAYED              2
#define PGF_DEPRESSED           4
#define PGF_HOT                 8

#define PGB_TOPORLEFT           0
#define PGB_BOTTOMORRIGHT       1

/* only used with PGN_SCROLL */
#define PGF_SCROLLUP            1
#define PGF_SCROLLDOWN          2
#define PGF_SCROLLLEFT          4
#define PGF_SCROLLRIGHT         8

#define PGK_SHIFT               1
#define PGK_CONTROL             2
#define PGK_MENU                4

/* only used with PGN_CALCSIZE */
#define PGF_CALCWIDTH           1
#define PGF_CALCHEIGHT          2

#define PGM_FIRST               0x1400
#define PGM_SETCHILD            (PGM_FIRST+1)
#define PGM_RECALCSIZE          (PGM_FIRST+2)
#define PGM_FORWARDMOUSE        (PGM_FIRST+3)
#define PGM_SETBKCOLOR          (PGM_FIRST+4)
#define PGM_GETBKCOLOR          (PGM_FIRST+5)
#define PGM_SETBORDER           (PGM_FIRST+6)
#define PGM_GETBORDER           (PGM_FIRST+7)
#define PGM_SETPOS              (PGM_FIRST+8)
#define PGM_GETPOS              (PGM_FIRST+9)
#define PGM_SETBUTTONSIZE       (PGM_FIRST+10)
#define PGM_GETBUTTONSIZE       (PGM_FIRST+11)
#define PGM_GETBUTTONSTATE      (PGM_FIRST+12)
#define PGM_GETDROPTARGET       CCM_GETDROPTARGET

#define PGN_FIRST               (0U-900U)
#define PGN_LAST                (0U-950U)
#define PGN_SCROLL              (PGN_FIRST-1)
#define PGN_CALCSIZE            (PGN_FIRST-2)
#define PGN_HOTITEMCHANGE       (PGN_FIRST-3)

#pragma pack(push,1)

typedef struct
{
    NMHDR hdr;
    WORD  fwKeys;
    RECT rcParent;
    INT  iDir;
    INT  iXpos;
    INT  iYpos;
    INT  iScroll;
} NMPGSCROLL, *LPNMPGSCROLL;

#pragma pack(pop)

typedef struct
{
    NMHDR hdr;
    DWORD dwFlag;
    INT iWidth;
    INT iHeight;
} NMPGCALCSIZE, *LPNMPGCALCSIZE;

#define Pager_SetChild(hwnd, child) \
    SNDMSG((hwnd), PGM_SETCHILD, 0, (LPARAM)(child))
#define Pager_RecalcSize(hwnd) \
    SNDMSG((hwnd), PGM_RECALCSIZE, 0, 0)
#define Pager_ForwardMouse(hwnd, forward) \
    SNDMSG((hwnd), PGM_FORWARDMOUSE, (WPARAM)(forward), 0)
#define Pager_SetBkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), PGM_SETBKCOLOR, 0, (LPARAM)(clr))
#define Pager_GetBkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), PGM_GETBKCOLOR, 0, 0)
#define Pager_SetBorder(hwnd, border) \
    (INT)SNDMSG((hwnd), PGM_SETBORDER, 0, (LPARAM)(border))
#define Pager_GetBorder(hwnd) \
    (INT)SNDMSG((hwnd), PGM_GETBORDER, 0, 0)
#define Pager_SetPos(hwnd, pos) \
    (INT)SNDMSG((hwnd), PGM_SETPOS, 0, (LPARAM)(pos))
#define Pager_GetPos(hwnd) \
    (INT)SNDMSG((hwnd), PGM_GETPOS, 0, 0)
#define Pager_SetButtonSize(hwnd, size) \
    (INT)SNDMSG((hwnd), PGM_SETBUTTONSIZE, 0, (LPARAM)(size))
#define Pager_GetButtonSize(hwnd) \
    (INT)SNDMSG((hwnd), PGM_GETBUTTONSIZE, 0, 0)
#define Pager_GetButtonState(hwnd, button) \
    (DWORD)SNDMSG((hwnd), PGM_GETBUTTONSTATE, 0, (LPARAM)(button))
#define Pager_GetDropTarget(hwnd, dt) \
    SNDMSG((hwnd), PGM_GETDROPTARGET, 0, (LPARAM)(dt))
#define Pager_SetScrollInfo(hwnd, timeout, lines, pixels) \
    SNDMSG((hwnd), PGM_SETSCROLLINFO, timeout, MAKELONG(lines, pixels))

/* Treeview control */

#define WC_TREEVIEWA          "SysTreeView32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_TREEVIEWW         L"SysTreeView32"
#else
static const WCHAR WC_TREEVIEWW[] = { 'S','y','s',
  'T','r','e','e','V','i','e','w','3','2',0 };
#endif
#define WC_TREEVIEW             WINELIB_NAME_AW(WC_TREEVIEW)

#define TVSIL_NORMAL            0
#define TVSIL_STATE             2

#define TVSBF_XBORDER           1
#define TVSBF_YBORDER           2

#define TV_FIRST                0x1100
#define TVM_INSERTITEMA         (TV_FIRST+0)
#define TVM_INSERTITEMW         (TV_FIRST+50)
#define TVM_INSERTITEM          WINELIB_NAME_AW(TVM_INSERTITEM)
#define TVM_DELETEITEM          (TV_FIRST+1)
#define TVM_EXPAND              (TV_FIRST+2)
#define TVM_GETITEMRECT         (TV_FIRST+4)
#define TVM_GETCOUNT            (TV_FIRST+5)
#define TVM_GETINDENT           (TV_FIRST+6)
#define TVM_SETINDENT           (TV_FIRST+7)
#define TVM_GETIMAGELIST        (TV_FIRST+8)
#define TVM_SETIMAGELIST        (TV_FIRST+9)
#define TVM_GETNEXTITEM         (TV_FIRST+10)
#define TVM_SELECTITEM          (TV_FIRST+11)
#define TVM_GETITEMA            (TV_FIRST+12)
#define TVM_GETITEMW            (TV_FIRST+62)
#define TVM_GETITEM             WINELIB_NAME_AW(TVM_GETITEM)
#define TVM_SETITEMA            (TV_FIRST+13)
#define TVM_SETITEMW            (TV_FIRST+63)
#define TVM_SETITEM             WINELIB_NAME_AW(TVM_SETITEM)
#define TVM_EDITLABELA          (TV_FIRST+14)
#define TVM_EDITLABELW          (TV_FIRST+65)
#define TVM_EDITLABEL           WINELIB_NAME_AW(TVM_EDITLABEL)
#define TVM_GETEDITCONTROL      (TV_FIRST+15)
#define TVM_GETVISIBLECOUNT     (TV_FIRST+16)
#define TVM_HITTEST             (TV_FIRST+17)
#define TVM_CREATEDRAGIMAGE     (TV_FIRST+18)
#define TVM_SORTCHILDREN        (TV_FIRST+19)
#define TVM_ENSUREVISIBLE       (TV_FIRST+20)
#define TVM_SORTCHILDRENCB      (TV_FIRST+21)
#define TVM_ENDEDITLABELNOW     (TV_FIRST+22)
#define TVM_GETISEARCHSTRINGA   (TV_FIRST+23)
#define TVM_GETISEARCHSTRINGW   (TV_FIRST+64)
#define TVM_GETISEARCHSTRING    WINELIB_NAME_AW(TVM_GETISEARCHSTRING)
#define TVM_SETTOOLTIPS         (TV_FIRST+24)
#define TVM_GETTOOLTIPS         (TV_FIRST+25)
#define TVM_SETINSERTMARK       (TV_FIRST+26)
#define TVM_SETITEMHEIGHT       (TV_FIRST+27)
#define TVM_GETITEMHEIGHT       (TV_FIRST+28)
#define TVM_SETBKCOLOR          (TV_FIRST+29)
#define TVM_SETTEXTCOLOR        (TV_FIRST+30)
#define TVM_GETBKCOLOR          (TV_FIRST+31)
#define TVM_GETTEXTCOLOR        (TV_FIRST+32)
#define TVM_SETSCROLLTIME       (TV_FIRST+33)
#define TVM_GETSCROLLTIME       (TV_FIRST+34)
#define TVM_SETBORDER           (TV_FIRST+35)
#define TVM_UNKNOWN36           (TV_FIRST+36)
#define TVM_SETINSERTMARKCOLOR  (TV_FIRST+37)
#define TVM_GETINSERTMARKCOLOR  (TV_FIRST+38)
#define TVM_GETITEMSTATE        (TV_FIRST+39)
#define TVM_SETLINECOLOR        (TV_FIRST+40)
#define TVM_GETLINECOLOR        (TV_FIRST+41)
#define TVM_SETUNICODEFORMAT    CCM_SETUNICODEFORMAT
#define TVM_GETUNICODEFORMAT    CCM_GETUNICODEFORMAT
#define TVM_MAPACCIDTOHTREEITEM (TV_FIRST + 42)
#define TVM_MAPHTREEITEMTOACCID (TV_FIRST + 43)
#define TVM_SETEXTENDEDSTYLE    (TV_FIRST + 44)
#define TVM_GETEXTENDEDSTYLE    (TV_FIRST + 45)
#define TVM_SETAUTOSCROLLINFO   (TV_FIRST + 59)
#define TVM_GETSELECTEDCOUNT    (TV_FIRST + 70)
#define TVM_SHOWINFOTIP         (TV_FIRST + 71)
#define TVM_GETITEMPARTRECT     (TV_FIRST + 72)


#define TVN_FIRST               (0U-400U)
#define TVN_LAST                (0U-499U)

#define TVN_SELCHANGINGA        (TVN_FIRST-1)
#define TVN_SELCHANGINGW        (TVN_FIRST-50)
#define TVN_SELCHANGING         WINELIB_NAME_AW(TVN_SELCHANGING)

#define TVN_SELCHANGEDA         (TVN_FIRST-2)
#define TVN_SELCHANGEDW         (TVN_FIRST-51)
#define TVN_SELCHANGED          WINELIB_NAME_AW(TVN_SELCHANGED)

#define TVN_GETDISPINFOA        (TVN_FIRST-3)
#define TVN_GETDISPINFOW        (TVN_FIRST-52)
#define TVN_GETDISPINFO         WINELIB_NAME_AW(TVN_GETDISPINFO)

#define TVN_SETDISPINFOA        (TVN_FIRST-4)
#define TVN_SETDISPINFOW        (TVN_FIRST-53)
#define TVN_SETDISPINFO         WINELIB_NAME_AW(TVN_SETDISPINFO)

#define TVN_ITEMEXPANDINGA      (TVN_FIRST-5)
#define TVN_ITEMEXPANDINGW      (TVN_FIRST-54)
#define TVN_ITEMEXPANDING       WINELIB_NAME_AW(TVN_ITEMEXPANDING)

#define TVN_ITEMEXPANDEDA       (TVN_FIRST-6)
#define TVN_ITEMEXPANDEDW       (TVN_FIRST-55)
#define TVN_ITEMEXPANDED        WINELIB_NAME_AW(TVN_ITEMEXPANDED)

#define TVN_BEGINDRAGA          (TVN_FIRST-7)
#define TVN_BEGINDRAGW          (TVN_FIRST-56)
#define TVN_BEGINDRAG           WINELIB_NAME_AW(TVN_BEGINDRAG)

#define TVN_BEGINRDRAGA         (TVN_FIRST-8)
#define TVN_BEGINRDRAGW         (TVN_FIRST-57)
#define TVN_BEGINRDRAG          WINELIB_NAME_AW(TVN_BEGINRDRAG)

#define TVN_DELETEITEMA         (TVN_FIRST-9)
#define TVN_DELETEITEMW         (TVN_FIRST-58)
#define TVN_DELETEITEM          WINELIB_NAME_AW(TVN_DELETEITEM)

#define TVN_BEGINLABELEDITA     (TVN_FIRST-10)
#define TVN_BEGINLABELEDITW     (TVN_FIRST-59)
#define TVN_BEGINLABELEDIT      WINELIB_NAME_AW(TVN_BEGINLABELEDIT)

#define TVN_ENDLABELEDITA       (TVN_FIRST-11)
#define TVN_ENDLABELEDITW       (TVN_FIRST-60)
#define TVN_ENDLABELEDIT        WINELIB_NAME_AW(TVN_ENDLABELEDIT)

#define TVN_KEYDOWN             (TVN_FIRST-12)

#define TVN_GETINFOTIPA         (TVN_FIRST-13)
#define TVN_GETINFOTIPW         (TVN_FIRST-14)
#define TVN_GETINFOTIP          WINELIB_NAME_AW(TVN_GETINFOTIP)

#define TVN_SINGLEEXPAND        (TVN_FIRST-15)

#define TVN_ITEMCHANGINGA       (TVN_FIRST-16)
#define TVN_ITEMCHANGINGW       (TVN_FIRST-17)
#define TVN_ITEMCHANGEDA        (TVN_FIRST-18)
#define TVN_ITEMCHANGEDW        (TVN_FIRST-19)
#define TVN_ASYNCDRAW           (TVN_FIRST-20)

#define TVIF_TEXT             0x0001
#define TVIF_IMAGE            0x0002
#define TVIF_PARAM            0x0004
#define TVIF_STATE            0x0008
#define TVIF_HANDLE           0x0010
#define TVIF_SELECTEDIMAGE    0x0020
#define TVIF_CHILDREN         0x0040
#define TVIF_INTEGRAL         0x0080
#define TVIF_STATEEX          0x0100
#define TVIF_EXPANDEDIMAGE    0x0200
#define TVIF_DI_SETITEM	      0x1000

#define TVI_ROOT              ((HTREEITEM)-65536)
#define TVI_FIRST             ((HTREEITEM)-65535)
#define TVI_LAST              ((HTREEITEM)-65534)
#define TVI_SORT              ((HTREEITEM)-65533)

#define TVIS_FOCUSED          0x0001
#define TVIS_SELECTED         0x0002
#define TVIS_CUT              0x0004
#define TVIS_DROPHILITED      0x0008
#define TVIS_BOLD             0x0010
#define TVIS_EXPANDED         0x0020
#define TVIS_EXPANDEDONCE     0x0040
#define TVIS_EXPANDPARTIAL    0x0080
#define TVIS_OVERLAYMASK      0x0f00
#define TVIS_STATEIMAGEMASK   0xf000
#define TVIS_USERMASK         0xf000

#define TVIS_EX_FLAT          0x0001
#define TVIS_EX_DISABLED      0x0002
/* TVIS_EX_HWND is listed on MSDN but apparently not in any header. */
#define TVIS_EX_ALL           0x0002

#define TVHT_NOWHERE          0x0001
#define TVHT_ONITEMICON       0x0002
#define TVHT_ONITEMLABEL      0x0004
#define TVHT_ONITEMINDENT     0x0008
#define TVHT_ONITEMBUTTON     0x0010
#define TVHT_ONITEMRIGHT      0x0020
#define TVHT_ONITEMSTATEICON  0x0040
#define TVHT_ONITEM           (TVHT_ONITEMICON | TVHT_ONITEMLABEL | TVHT_ONITEMSTATEICON)
#define TVHT_ABOVE            0x0100
#define TVHT_BELOW            0x0200
#define TVHT_TORIGHT          0x0400
#define TVHT_TOLEFT           0x0800

#define TVS_SHAREDIMAGELISTS  0x0000
#define TVS_PRIVATEIMAGELISTS 0x0400

#define TVS_EX_NOSINGLECOLLAPSE    0x0001
#define TVS_EX_MULTISELECT         0x0002
#define TVS_EX_DOUBLEBUFFER        0x0004
#define TVS_EX_NOINDENTSTATE       0x0008
#define TVS_EX_RICHTOOLTIP         0x0010
#define TVS_EX_AUTOHSCROLL         0x0020
#define TVS_EX_FADEINOUTEXPANDOS   0x0040
#define TVS_EX_PARTIALCHECKBOXES   0x0080
#define TVS_EX_EXCLUSIONCHECKBOXES 0x0100
#define TVS_EX_DIMMEDCHECKBOXES    0x0200
#define TVS_EX_DRAWIMAGEASYNC      0x0400

#define TVE_COLLAPSE          0x0001
#define TVE_EXPAND            0x0002
#define TVE_TOGGLE            0x0003
#define TVE_EXPANDPARTIAL     0x4000
#define TVE_COLLAPSERESET     0x8000

#define TVGN_ROOT             0
#define TVGN_NEXT             1
#define TVGN_PREVIOUS         2
#define TVGN_PARENT           3
#define TVGN_CHILD            4
#define TVGN_FIRSTVISIBLE     5
#define TVGN_NEXTVISIBLE      6
#define TVGN_PREVIOUSVISIBLE  7
#define TVGN_DROPHILITE       8
#define TVGN_CARET            9
#define TVGN_LASTVISIBLE      10
#define TVGN_NEXTSELECTED     11
#define TVSI_NOSINGLEEXPAND   0x8000

#define TVC_UNKNOWN           0x00
#define TVC_BYMOUSE           0x01
#define TVC_BYKEYBOARD        0x02


typedef struct _TREEITEM *HTREEITEM;

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPSTR  pszText;
      INT  cchTextMax;
      INT  iImage;
      INT  iSelectedImage;
      INT  cChildren;
      LPARAM lParam;
} TVITEMA, *LPTVITEMA;

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPWSTR pszText;
      INT cchTextMax;
      INT iImage;
      INT iSelectedImage;
      INT cChildren;
      LPARAM lParam;
} TVITEMW, *LPTVITEMW;

#define TV_ITEMA    TVITEMA
#define TV_ITEMW    TVITEMW
#define LPTV_ITEMA  LPTVITEMA
#define LPTV_ITEMW  LPTVITEMW

#define TVITEM      WINELIB_NAME_AW(TVITEM)
#define LPTVITEM    WINELIB_NAME_AW(LPTVITEM)
#define TV_ITEM     WINELIB_NAME_AW(TV_ITEM)
#define LPTV_ITEM   WINELIB_NAME_AW(LPTV_ITEM)

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPSTR  pszText;
      INT  cchTextMax;
      INT  iImage;
      INT  iSelectedImage;
      INT  cChildren;
      LPARAM lParam;
      INT iIntegral;
      UINT uStateEx;        /* _WIN32_IE >= 0x600 */
      HWND hwnd;            /* _WIN32_IE >= 0x600 */
      INT iExpandedImage;   /* _WIN32_IE >= 0x600 */
      INT iReserved;
} TVITEMEXA, *LPTVITEMEXA;

typedef struct {
      UINT mask;
      HTREEITEM hItem;
      UINT state;
      UINT stateMask;
      LPWSTR pszText;
      INT cchTextMax;
      INT iImage;
      INT iSelectedImage;
      INT cChildren;
      LPARAM lParam;
      INT iIntegral;
      UINT uStateEx;        /* _WIN32_IE >= 0x600 */
      HWND hwnd;            /* _WIN32_IE >= 0x600 */
      INT iExpandedImage;   /* _WIN32_IE >= 0x600 */
      INT iReserved;
} TVITEMEXW, *LPTVITEMEXW;

#define TVITEMEX   WINELIB_NAME_AW(TVITEMEX)
#define LPTVITEMEX WINELIB_NAME_AW(LPTVITEMEX)

typedef struct tagTVINSERTSTRUCTA {
        HTREEITEM hParent;
        HTREEITEM hInsertAfter;
        union {
           TVITEMEXA itemex;
           TVITEMA   item;
        } DUMMYUNIONNAME;
} TVINSERTSTRUCTA, *LPTVINSERTSTRUCTA;

typedef struct tagTVINSERTSTRUCTW {
        HTREEITEM hParent;
        HTREEITEM hInsertAfter;
        union {
           TVITEMEXW itemex;
           TVITEMW   item;
        } DUMMYUNIONNAME;
} TVINSERTSTRUCTW, *LPTVINSERTSTRUCTW;

#define TVINSERTSTRUCT    WINELIB_NAME_AW(TVINSERTSTRUCT)
#define LPTVINSERTSTRUCT  WINELIB_NAME_AW(LPTVINSERTSTRUCT)

#define TVINSERTSTRUCT_V1_SIZEA CCSIZEOF_STRUCT(TVINSERTSTRUCTA, item)
#define TVINSERTSTRUCT_V1_SIZEW CCSIZEOF_STRUCT(TVINSERTSTRUCTW, item)
#define TVINSERTSTRUCT_V1_SIZE    WINELIB_NAME_AW(TVINSERTSTRUCT_V1_SIZE)

#define TV_INSERTSTRUCT    TVINSERTSTRUCT
#define TV_INSERTSTRUCTA   TVINSERTSTRUCTA
#define TV_INSERTSTRUCTW   TVINSERTSTRUCTW
#define LPTV_INSERTSTRUCT  LPTVINSERTSTRUCT
#define LPTV_INSERTSTRUCTA LPTVINSERTSTRUCTA
#define LPTV_INSERTSTRUCTW LPTVINSERTSTRUCTW



typedef struct tagNMTREEVIEWA {
	NMHDR	hdr;
	UINT	action;
	TVITEMA	itemOld;
	TVITEMA	itemNew;
	POINT	ptDrag;
} NMTREEVIEWA, *LPNMTREEVIEWA;

typedef struct tagNMTREEVIEWW {
	NMHDR	hdr;
	UINT	action;
	TVITEMW	itemOld;
	TVITEMW	itemNew;
	POINT	ptDrag;
} NMTREEVIEWW, *LPNMTREEVIEWW;

#define NMTREEVIEW     WINELIB_NAME_AW(NMTREEVIEW)
#define NM_TREEVIEW    WINELIB_NAME_AW(NMTREEVIEW)
#define NM_TREEVIEWA   NMTREEVIEWA
#define NM_TREEVIEWW   NMTREEVIEWW
#define LPNMTREEVIEW   WINELIB_NAME_AW(LPNMTREEVIEW)

#define LPNM_TREEVIEW  LPNMTREEVIEW
#define LPNM_TREEVIEWA LPNMTREEVIEWA
#define LPNM_TREEVIEWW LPNMTREEVIEWW

typedef struct tagTVDISPINFOA {
	NMHDR	hdr;
	TVITEMA	item;
} NMTVDISPINFOA, *LPNMTVDISPINFOA;

typedef struct tagTVDISPINFOW {
	NMHDR	hdr;
	TVITEMW	item;
} NMTVDISPINFOW, *LPNMTVDISPINFOW;

typedef struct tagTVDISPINFOEXA {
	NMHDR	hdr;
	TVITEMEXA	item;
} NMTVDISPINFOEXA, *LPNMTVDISPINFOEXA;

typedef struct tagTVDISPINFOEXW {
	NMHDR	hdr;
	TVITEMEXW	item;
} NMTVDISPINFOEXW, *LPNMTVDISPINFOEXW;

#define NMTVDISPINFO            WINELIB_NAME_AW(NMTVDISPINFO)
#define LPNMTVDISPINFO          WINELIB_NAME_AW(LPNMTVDISPINFO)
#define NMTVDISPINFOEX          WINELIB_NAME_AW(NMTVDISPINFOEX)
#define LPNMTVDISPINFOEX        WINELIB_NAME_AW(LPNMTVDISPINFOEX)
#define TV_DISPINFOA            NMTVDISPINFOA
#define TV_DISPINFOW            NMTVDISPINFOW
#define TV_DISPINFO             NMTVDISPINFO

typedef INT (CALLBACK *PFNTVCOMPARE)(LPARAM, LPARAM, LPARAM);

typedef struct tagTVSORTCB
{
	HTREEITEM hParent;
	PFNTVCOMPARE lpfnCompare;
	LPARAM lParam;
} TVSORTCB, *LPTVSORTCB;

#define TV_SORTCB TVSORTCB
#define LPTV_SORTCB LPTVSORTCB

typedef struct tagTVHITTESTINFO {
        POINT pt;
        UINT flags;
        HTREEITEM hItem;
} TVHITTESTINFO, *LPTVHITTESTINFO;

#define TV_HITTESTINFO TVHITTESTINFO


/* Custom Draw Treeview */

#define NMTVCUSTOMDRAW_V3_SIZE CCSIZEOF_STRUCT(NMTVCUSTOMDRAW, clrTextBk)

#define TVCDRF_NOIMAGES     0x00010000

typedef struct tagNMTVCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    COLORREF     clrText;
    COLORREF     clrTextBk;
    INT iLevel;                 /* IE>0x0400 */
} NMTVCUSTOMDRAW, *LPNMTVCUSTOMDRAW;

/* Treeview tooltips */

typedef struct tagNMTVGETINFOTIPA
{
    NMHDR hdr;
    LPSTR pszText;
    INT cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPA, *LPNMTVGETINFOTIPA;

typedef struct tagNMTVGETINFOTIPW
{
    NMHDR hdr;
    LPWSTR pszText;
    INT cchTextMax;
    HTREEITEM hItem;
    LPARAM lParam;
} NMTVGETINFOTIPW, *LPNMTVGETINFOTIPW;

#define NMTVGETINFOTIP WINELIB_NAME_AW(NMTVGETINFOTIP)
#define LPNMTVGETINFOTIP WINELIB_NAME_AW(LPNMTVGETINFOTIP)

typedef enum _TVITEMPART
{
    TVGIPR_BUTTON = 1
} TVITEMPART;

typedef struct tagTVGETITEMPARTRECTINFO
{
    HTREEITEM hti;
    RECT *prc;
    TVITEMPART partID;
} TVGETITEMPARTRECTINFO;

typedef struct tagTVITEMCHANGE
{
    NMHDR hdr;
    UINT uChanged;
    HTREEITEM hItem;
    UINT uStateNew;
    UINT uStateOld;
    LPARAM lParam;
} NMTVITEMCHANGE;

typedef struct tagNMTVASYNCDRAW
{
    NMHDR hdr;
    IMAGELISTDRAWPARAMS *pimldp;
    HRESULT hr;
    HTREEITEM hItem;
    LPARAM lParam;
    DWORD dwRetFlags;
    int iRetImageIndex;
} NMTVASYNCDRAW;

#pragma pack(push,1)
typedef struct tagTVKEYDOWN
{
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
} NMTVKEYDOWN, *LPNMTVKEYDOWN;
#pragma pack(pop)

#define TV_KEYDOWN      NMTVKEYDOWN

#define TreeView_InsertItemA(hwnd, phdi) \
  (HTREEITEM)SNDMSGA((hwnd), TVM_INSERTITEMA, 0, \
                            (LPARAM)(LPTVINSERTSTRUCTA)(phdi))
#define TreeView_InsertItemW(hwnd,phdi) \
  (HTREEITEM)SNDMSGW((hwnd), TVM_INSERTITEMW, 0, \
                            (LPARAM)(LPTVINSERTSTRUCTW)(phdi))
#define TreeView_InsertItem WINELIB_NAME_AW(TreeView_InsertItem)

#define TreeView_DeleteItem(hwnd, hItem) \
  (BOOL)SNDMSG((hwnd), TVM_DELETEITEM, 0, (LPARAM)(HTREEITEM)(hItem))
#define TreeView_DeleteAllItems(hwnd) \
  (BOOL)SNDMSG((hwnd), TVM_DELETEITEM, 0, (LPARAM)TVI_ROOT)
#define TreeView_Expand(hwnd, hitem, code) \
 (BOOL)SNDMSG((hwnd), TVM_EXPAND, (WPARAM)code, \
	(LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetItemRect(hwnd, hitem, prc, code) \
 (*(HTREEITEM *)prc = (hitem), (BOOL)SNDMSG((hwnd), \
			TVM_GETITEMRECT, (WPARAM)(code), (LPARAM)(RECT *)(prc)))

#define TreeView_GetCount(hwnd) \
    (UINT)SNDMSG((hwnd), TVM_GETCOUNT, 0, 0)
#define TreeView_GetIndent(hwnd) \
    (UINT)SNDMSG((hwnd), TVM_GETINDENT, 0, 0)
#define TreeView_SetIndent(hwnd, indent) \
    (BOOL)SNDMSG((hwnd), TVM_SETINDENT, (WPARAM)indent, 0)

#define TreeView_GetImageList(hwnd, iImage) \
    (HIMAGELIST)SNDMSG((hwnd), TVM_GETIMAGELIST, iImage, 0)

#define TreeView_SetImageList(hwnd, himl, iImage) \
    (HIMAGELIST)SNDMSG((hwnd), TVM_SETIMAGELIST, iImage, \
 (LPARAM)(HIMAGELIST)(himl))

#define TreeView_GetNextItem(hwnd, hitem, code) \
    (HTREEITEM)SNDMSG((hwnd), TVM_GETNEXTITEM, (WPARAM)code,\
(LPARAM)(HTREEITEM) (hitem))

#define TreeView_GetChild(hwnd, hitem) \
	 	TreeView_GetNextItem(hwnd, hitem , TVGN_CHILD)
#define TreeView_GetNextSibling(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_NEXT)
#define TreeView_GetPrevSibling(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_PREVIOUS)
#define TreeView_GetParent(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_PARENT)
#define TreeView_GetFirstVisible(hwnd)  \
		TreeView_GetNextItem(hwnd, NULL, TVGN_FIRSTVISIBLE)
#define TreeView_GetLastVisible(hwnd)   \
		TreeView_GetNextItem(hwnd, NULL, TVGN_LASTVISIBLE)
#define TreeView_GetNextVisible(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_NEXTVISIBLE)
#define TreeView_GetPrevVisible(hwnd, hitem) \
		TreeView_GetNextItem(hwnd, hitem , TVGN_PREVIOUSVISIBLE)
#define TreeView_GetSelection(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_CARET)
#define TreeView_GetDropHilight(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_DROPHILITE)
#define TreeView_GetRoot(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_ROOT)
#define TreeView_GetLastVisible(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_LASTVISIBLE)
#define TreeView_GetNextSelected(hwnd) \
		TreeView_GetNextItem(hwnd, NULL, TVGN_NEXTSELECTED)


#define TreeView_Select(hwnd, hitem, code) \
 (BOOL)SNDMSG((hwnd), TVM_SELECTITEM, (WPARAM)(code), \
(LPARAM)(HTREEITEM)(hitem))


#define TreeView_SelectItem(hwnd, hitem) \
		TreeView_Select(hwnd, hitem, TVGN_CARET)
#define TreeView_SelectDropTarget(hwnd, hitem) \
       	TreeView_Select(hwnd, hitem, TVGN_DROPHILITE)
#define TreeView_SelectSetFirstVisible(hwnd, hitem) \
       	TreeView_Select(hwnd, hitem, TVGN_FIRSTVISIBLE)


#define TreeView_GetItemA(hwnd, pitem) \
 (BOOL)SNDMSGA((hwnd), TVM_GETITEMA, 0, (LPARAM) (TVITEMA *)(pitem))
#define TreeView_GetItemW(hwnd, pitem) \
 (BOOL)SNDMSGW((hwnd), TVM_GETITEMW, 0, (LPARAM) (TVITEMW *)(pitem))
#define TreeView_GetItem WINELIB_NAME_AW(TreeView_GetItem)

#define TreeView_SetItemA(hwnd, pitem) \
 (BOOL)SNDMSGA((hwnd), TVM_SETITEMA, 0, (LPARAM)(const TVITEMA *)(pitem))
#define TreeView_SetItemW(hwnd, pitem) \
 (BOOL)SNDMSGW((hwnd), TVM_SETITEMW, 0, (LPARAM)(const TVITEMW *)(pitem))
#define TreeView_SetItem WINELIB_NAME_AW(TreeView_SetItem)

#define TreeView_EditLabel(hwnd, hitem) \
    (HWND)SNDMSG((hwnd), TVM_EDITLABEL, 0, (LPARAM)(HTREEITEM)(hitem))

#define TreeView_GetEditControl(hwnd) \
    (HWND)SNDMSG((hwnd), TVM_GETEDITCONTROL, 0, 0)

#define TreeView_GetVisibleCount(hwnd) \
    (UINT)SNDMSG((hwnd), TVM_GETVISIBLECOUNT, 0, 0)

#define TreeView_HitTest(hwnd, lpht) \
    (HTREEITEM)SNDMSG((hwnd), TVM_HITTEST, 0,\
(LPARAM)(LPTVHITTESTINFO)(lpht))

#define TreeView_CreateDragImage(hwnd, hitem) \
    (HIMAGELIST)SNDMSG((hwnd), TVM_CREATEDRAGIMAGE, 0,\
(LPARAM)(HTREEITEM)(hitem))

#define TreeView_SortChildren(hwnd, hitem, recurse) \
    (BOOL)SNDMSG((hwnd), TVM_SORTCHILDREN, (WPARAM)recurse,\
(LPARAM)(HTREEITEM)(hitem))

#define TreeView_EnsureVisible(hwnd, hitem) \
    (BOOL)SNDMSG((hwnd), TVM_ENSUREVISIBLE, 0, (LPARAM)(UINT)(hitem))

#define TreeView_SortChildrenCB(hwnd, psort, recurse) \
    (BOOL)SNDMSG((hwnd), TVM_SORTCHILDRENCB, (WPARAM)recurse, \
    (LPARAM)(LPTV_SORTCB)(psort))

#define TreeView_EndEditLabelNow(hwnd, fCancel) \
    (BOOL)SNDMSG((hwnd), TVM_ENDEDITLABELNOW, (WPARAM)fCancel, 0)

#define TreeView_GetISearchString(hwnd, lpsz) \
    (BOOL)SNDMSG((hwnd), TVM_GETISEARCHSTRING, 0, \
							(LPARAM)(LPTSTR)lpsz)

#define TreeView_SetToolTips(hwnd,  hwndTT) \
    (HWND)SNDMSG((hwnd), TVM_SETTOOLTIPS, (WPARAM)(hwndTT), 0)

#define TreeView_GetToolTips(hwnd) \
    (HWND)SNDMSG((hwnd), TVM_GETTOOLTIPS, 0, 0)

#define TreeView_SetItemHeight(hwnd,  iHeight) \
    (INT)SNDMSG((hwnd), TVM_SETITEMHEIGHT, (WPARAM)iHeight, 0)

#define TreeView_GetItemHeight(hwnd) \
    (INT)SNDMSG((hwnd), TVM_GETITEMHEIGHT, 0, 0)

#define TreeView_SetBkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETBKCOLOR, 0, (LPARAM)clr)

#define TreeView_SetTextColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETTEXTCOLOR, 0, (LPARAM)clr)

#define TreeView_GetBkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETBKCOLOR, 0, 0)

#define TreeView_GetTextColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETTEXTCOLOR, 0, 0)

#define TreeView_SetScrollTime(hwnd, uTime) \
    (UINT)SNDMSG((hwnd), TVM_SETSCROLLTIME, uTime, 0)

#define TreeView_GetScrollTime(hwnd) \
    (UINT)SNDMSG((hwnd), TVM_GETSCROLLTIME, 0, 0)

#define TreeView_SetInsertMark(hwnd, hItem, fAfter) \
    (BOOL)SNDMSG((hwnd), TVM_SETINSERTMARK, (WPARAM)(fAfter), \
                       (LPARAM) (hItem))

#define TreeView_SetInsertMarkColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETINSERTMARKCOLOR, 0, (LPARAM)clr)

#define TreeView_GetInsertMarkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETINSERTMARKCOLOR, 0, 0)

#define TreeView_SetItemState(hwndTV, hti, data, _mask) \
{ TVITEM _TVi; \
  _TVi.mask = TVIF_STATE; \
  _TVi.hItem = hti; \
  _TVi.stateMask = _mask; \
  _TVi.state = data; \
  SNDMSG((hwndTV), TVM_SETITEM, 0, (LPARAM)&_TVi); \
}

#define TreeView_GetItemState(hwndTV, hti, mask) \
   (UINT)SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti), (LPARAM)(mask))
#define TreeView_GetCheckState(hwndTV, hti) \
   ((((UINT)(SNDMSG((hwndTV), TVM_GETITEMSTATE, (WPARAM)(hti),  \
                     TVIS_STATEIMAGEMASK))) >> 12) -1)
#define TreeView_SetCheckState(hwndTV, hti, check) \
    TreeView_SetItemState(hwndTV, hti, INDEXTOSTATEIMAGEMASK((check) ? 2 : 1), TVIS_STATEIMAGEMASK)

#define TreeView_SetLineColor(hwnd, clr) \
    (COLORREF)SNDMSG((hwnd), TVM_SETLINECOLOR, 0, (LPARAM)(clr))

#define TreeView_GetLineColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), TVM_GETLINECOLOR, 0, 0)

#define TreeView_MapAccIDToHTREEITEM(hwnd, id) \
    (HTREEITEM)SNDMSG((hwnd), TVM_MAPACCIDTOHTREEITEM, id, 0)

#define TreeView_MapHTREEITEMToAccID(hwnd, htreeitem) \
    (UINT)SNDMSG((hwnd), TVM_MAPHTREEITEMTOACCID, (WPARAM)htreeitem, 0)

#define TreeView_SetUnicodeFormat(hwnd, fUnicode) \
    (BOOL)SNDMSG((hwnd), TVM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)
#define TreeView_GetUnicodeFormat(hwnd) \
    (BOOL)SNDMSG((hwnd), TVM_GETUNICODEFORMAT, 0, 0)

#define TreeView_SetExtendedStyle(hwnd, style, mask) \
    (DWORD)SNDMSG((hwnd), TVM_SETEXTENDEDSTYLE, mask, style)

#define TreeView_GetExtendedStyle(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETEXTENDEDSTYLE, 0, 0)

#define TreeView_SetAutoScrollInfo(hwnd, pps, updatetime) \
    SNDMSG((hwnd), TVM_SETAUTOSCROLLINFO, (WPARAM)(pps), (LPARAM)(updatetime))

#define TreeView_SetHot(hwnd, hitem) \
    SNDMSG((hwnd), TVM_SETHOT, 0, (LPARAM)(hitem))

#define TreeView_GetSelectedCount(hwnd) \
    (DWORD)SNDMSG((hwnd), TVM_GETSELECTEDCOUNT, 0, 0)

#define TreeView_ShowInfoTip(hwnd, hitem) \
    (DWORD)SNDMSG((hwnd), TVM_SHOWINFOTIP, 0, (LPARAM)(hitem))

#define TreeView_GetItemPartRect(hwnd, hitem, rect, part) \
{ \
    TVGETITEMPARTRECTINFO info; \
    info.hti = (hitem); \
    info.prc = (rect); \
    info.partID = (part); \
    SNDMSG((hwnd), TVM_GETITEMPARTRECT, 0, (LPARAM)&info); \
}


/* Listview control */

#define WC_LISTVIEWA          "SysListView32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_LISTVIEWW         L"SysListView32"
#else
static const WCHAR WC_LISTVIEWW[] = { 'S','y','s',
  'L','i','s','t','V','i','e','w','3','2',0 };
#endif
#define WC_LISTVIEW  WINELIB_NAME_AW(WC_LISTVIEW)

#define LVSCW_AUTOSIZE -1
#define LVSCW_AUTOSIZE_USEHEADER -2


#define LVS_EX_GRIDLINES        0x0001
#define LVS_EX_SUBITEMIMAGES    0x0002
#define LVS_EX_CHECKBOXES       0x0004
#define LVS_EX_TRACKSELECT      0x0008
#define LVS_EX_HEADERDRAGDROP   0x0010
#define LVS_EX_FULLROWSELECT    0x0020
#define LVS_EX_ONECLICKACTIVATE 0x0040
#define LVS_EX_TWOCLICKACTIVATE 0x0080
#define LVS_EX_FLATSB           0x0100
#define LVS_EX_REGIONAL         0x0200
#define LVS_EX_INFOTIP          0x0400
#define LVS_EX_UNDERLINEHOT     0x0800
#define LVS_EX_UNDERLINECOLD    0x1000
#define LVS_EX_MULTIWORKAREAS   0x2000
#define LVS_EX_LABELTIP         0x4000
#define LVS_EX_BORDERSELECT     0x8000
#define LVS_EX_DOUBLEBUFFER     0x00010000
#define LVS_EX_HIDELABELS       0x00020000
#define LVS_EX_SINGLEROW        0x00040000
#define LVS_EX_SNAPTOGRID       0x00080000
#define LVS_EX_SIMPLESELECT     0x00100000
#define LVS_EX_JUSTIFYCOLUMNS   0x00200000
#define LVS_EX_TRANSPARENTBKGND 0x00400000
#define LVS_EX_TRANSPARENTSHADOWTEXT 0x00800000
#define LVS_EX_AUTOAUTOARRANGE  0x01000000
#define LVS_EX_HEADERINALLVIEWS 0x02000000
#define LVS_EX_AUTOCHECKSELECT  0x08000000
#define LVS_EX_AUTOSIZECOLUMNS  0x10000000
#define LVS_EX_COLUMNSNAPPOINTS 0x40000000
#define LVS_EX_COLUMNOVERFLOW   0x80000000

#define LVCF_FMT                0x0001
#define LVCF_WIDTH              0x0002
#define LVCF_TEXT               0x0004
#define LVCF_SUBITEM            0x0008
#define LVCF_IMAGE              0x0010
#define LVCF_ORDER              0x0020
#define LVCF_MINWIDTH           0x0040
#define LVCF_DEFAULTWIDTH       0x0080
#define LVCF_IDEALWIDTH         0x0100

#define LVCFMT_LEFT             0x0000
#define LVCFMT_RIGHT            0x0001
#define LVCFMT_CENTER           0x0002
#define LVCFMT_JUSTIFYMASK      0x0003
#define LVCFMT_FIXED_WIDTH      0x0100
#define LVCFMT_IMAGE            0x0800
#define LVCFMT_BITMAP_ON_RIGHT  0x1000
#define LVCFMT_COL_HAS_IMAGES   0x8000
#define LVCFMT_NO_DPI_SCALE     0x00040000
#define LVCFMT_FIXED_RATIO      0x00080000
#define LVCFMT_LINE_BREAK       0x00100000
#define LVCFMT_FILL             0x00200000
#define LVCFMT_WRAP             0x00400000
#define LVCFMT_NO_TITLE         0x00800000
#define LVCFMT_SPLIT_BUTTON     0x01000000
#define LVCFMT_TILE_PLACEMENTMASK (LVCFMT_LINE_BREAK | LVCFMT_FILL)

#define LVSIL_NORMAL            0
#define LVSIL_SMALL             1
#define LVSIL_STATE             2
#define LVSIL_GROUPHEADER       3

/* following 2 flags only for LVS_OWNERDATA listviews */
/* and only in report or list mode */
#define LVSICF_NOINVALIDATEALL  0x0001
#define LVSICF_NOSCROLL         0x0002


#define LVFI_PARAM              0x0001
#define LVFI_STRING             0x0002
#define LVFI_SUBSTRING          0x0004
#define LVFI_PARTIAL            0x0008
#define LVFI_WRAP               0x0020
#define LVFI_NEARESTXY          0x0040

#define LVIF_TEXT               0x0001
#define LVIF_IMAGE              0x0002
#define LVIF_PARAM              0x0004
#define LVIF_STATE              0x0008
#define LVIF_INDENT             0x0010
#define LVIF_GROUPID            0x0100
#define LVIF_COLUMNS            0x0200
#define LVIF_NORECOMPUTE        0x0800
#define LVIF_DI_SETITEM         0x1000
#define LVIF_COLFMT             0x00010000

#define LVIR_BOUNDS             0x0000
#define LVIR_ICON               0x0001
#define LVIR_LABEL              0x0002
#define LVIR_SELECTBOUNDS       0x0003

#define LVIS_FOCUSED            0x0001
#define LVIS_SELECTED           0x0002
#define LVIS_CUT                0x0004
#define LVIS_DROPHILITED        0x0008
#define LVIS_ACTIVATING         0x0020

#define LVIS_OVERLAYMASK        0x0F00
#define LVIS_STATEIMAGEMASK     0xF000

#define LVNI_ALL		0x0000
#define LVNI_FOCUSED		0x0001
#define LVNI_SELECTED		0x0002
#define LVNI_CUT		0x0004
#define LVNI_DROPHILITED	0x0008
#define LVNI_STATEMASK          (LVNI_FOCUSED | LVNI_SELECTED | LVNI_CUT | LVNI_DROPHILITED)

#define LVNI_VISIBLEORDER       0x0010
#define LVNI_PREVIOUS           0x0020
#define LVNI_VISIBLEONLY        0x0040
#define LVNI_SAMEGROUPONLY      0x0080

#define LVNI_ABOVE		0x0100
#define LVNI_BELOW		0x0200
#define LVNI_TOLEFT		0x0400
#define LVNI_TORIGHT		0x0800
#define LVNI_DIRECTIONMASK      (LVNI_ABOVE | LVNI_BELOW | LVNI_TOLEFT | LVNI_TORIGHT)

#define LVHT_NOWHERE		0x0001
#define LVHT_ONITEMICON		0x0002
#define LVHT_ONITEMLABEL	0x0004
#define LVHT_ONITEMSTATEICON	0x0008
#define LVHT_ONITEM		(LVHT_ONITEMICON|LVHT_ONITEMLABEL|LVHT_ONITEMSTATEICON)

#define LVHT_ABOVE		0x0008
#define LVHT_BELOW		0x0010
#define LVHT_TORIGHT		0x0020
#define LVHT_TOLEFT		0x0040

#define LV_VIEW_ICON            0x0000
#define LV_VIEW_DETAILS         0x0001
#define LV_VIEW_SMALLICON       0x0002
#define LV_VIEW_LIST            0x0003
#define LV_VIEW_TILE            0x0004
#define LV_VIEW_MAX             0x0004

#define LVGF_NONE               0x00000000
#define LVGF_HEADER             0x00000001
#define LVGF_FOOTER             0x00000002
#define LVGF_STATE              0x00000004
#define LVGF_ALIGN              0x00000008
#define LVGF_GROUPID            0x00000010
#define LVGF_SUBTITLE           0x00000100
#define LVGF_TASK               0x00000200
#define LVGF_DESCRIPTIONTOP     0x00000400
#define LVGF_DESCRIPTIONBOTTOM  0x00000800
#define LVGF_TITLEIMAGE         0x00001000
#define LVGF_EXTENDEDIMAGE      0x00002000
#define LVGF_ITEMS              0x00004000
#define LVGF_SUBSET             0x00008000
#define LVGF_SUBSETITEMS        0x00010000

#define LVGS_NORMAL             0x00000000
#define LVGS_COLLAPSED          0x00000001
#define LVGS_HIDDEN             0x00000002
#define LVGS_NOHEADER           0x00000004
#define LVGS_COLLAPSIBLE        0x00000008
#define LVGS_FOCUSED            0x00000010
#define LVGS_SELECTED           0x00000020
#define LVGS_SUBSETED           0x00000040
#define LVGS_SUBSETLINKFOCUSED  0x00000080

#define LVGA_HEADER_LEFT        0x00000001
#define LVGA_HEADER_CENTER      0x00000002
#define LVGA_HEADER_RIGHT       0x00000004
#define LVGA_FOOTER_LEFT        0x00000008
#define LVGA_FOOTER_CENTER      0x00000010
#define LVGA_FOOTER_RIGHT       0x00000020

#define LVGMF_NONE              0x00000000
#define LVGMF_BORDERSIZE        0x00000001
#define LVGMF_BORDERCOLOR       0x00000002
#define LVGMF_TEXTCOLOR         0x00000004

#define LVTVIF_AUTOSIZE         0x00000000
#define LVTVIF_FIXEDWIDTH       0x00000001
#define LVTVIF_FIXEDHEIGHT      0x00000002
#define LVTVIF_FIXEDSIZE        0x00000003
#define LVTVIF_EXTENDED         0x00000004

#define LVTVIM_TILESIZE         0x00000001
#define LVTVIM_COLUMNS          0x00000002
#define LVTVIM_LABELMARGIN      0x00000004

#define LVIM_AFTER              0x00000001

#define LVM_FIRST               0x1000
#define LVM_GETBKCOLOR          (LVM_FIRST+0)
#define LVM_SETBKCOLOR          (LVM_FIRST+1)
#define LVM_GETIMAGELIST        (LVM_FIRST+2)
#define LVM_SETIMAGELIST        (LVM_FIRST+3)
#define LVM_GETITEMCOUNT        (LVM_FIRST+4)
#define LVM_GETITEMA            (LVM_FIRST+5)
#define LVM_GETITEMW            (LVM_FIRST+75)
#define LVM_GETITEM             WINELIB_NAME_AW(LVM_GETITEM)
#define LVM_SETITEMA            (LVM_FIRST+6)
#define LVM_SETITEMW            (LVM_FIRST+76)
#define LVM_SETITEM             WINELIB_NAME_AW(LVM_SETITEM)
#define LVM_INSERTITEMA         (LVM_FIRST+7)
#define LVM_INSERTITEMW         (LVM_FIRST+77)
#define LVM_INSERTITEM          WINELIB_NAME_AW(LVM_INSERTITEM)
#define LVM_DELETEITEM          (LVM_FIRST+8)
#define LVM_DELETEALLITEMS      (LVM_FIRST+9)
#define LVM_GETCALLBACKMASK     (LVM_FIRST+10)
#define LVM_SETCALLBACKMASK     (LVM_FIRST+11)
#define LVM_GETNEXTITEM         (LVM_FIRST+12)
#define LVM_FINDITEMA           (LVM_FIRST+13)
#define LVM_FINDITEMW           (LVM_FIRST+83)
#define LVM_FINDITEM            WINELIB_NAME_AW(LVM_FINDITEM)
#define LVM_GETITEMRECT         (LVM_FIRST+14)
#define LVM_SETITEMPOSITION     (LVM_FIRST+15)
#define LVM_GETITEMPOSITION     (LVM_FIRST+16)
#define LVM_GETSTRINGWIDTHA     (LVM_FIRST+17)
#define LVM_GETSTRINGWIDTHW     (LVM_FIRST+87)
#define LVM_GETSTRINGWIDTH      WINELIB_NAME_AW(LVM_GETSTRINGWIDTH)
#define LVM_HITTEST             (LVM_FIRST+18)
#define LVM_ENSUREVISIBLE       (LVM_FIRST+19)
#define LVM_SCROLL              (LVM_FIRST+20)
#define LVM_REDRAWITEMS         (LVM_FIRST+21)
#define LVM_ARRANGE             (LVM_FIRST+22)
#define LVM_EDITLABELA          (LVM_FIRST+23)
#define LVM_EDITLABELW          (LVM_FIRST+118)
#define LVM_EDITLABEL           WINELIB_NAME_AW(LVM_EDITLABEL)
#define LVM_GETEDITCONTROL      (LVM_FIRST+24)
#define LVM_GETCOLUMNA          (LVM_FIRST+25)
#define LVM_GETCOLUMNW          (LVM_FIRST+95)
#define LVM_GETCOLUMN           WINELIB_NAME_AW(LVM_GETCOLUMN)
#define LVM_SETCOLUMNA          (LVM_FIRST+26)
#define LVM_SETCOLUMNW          (LVM_FIRST+96)
#define LVM_SETCOLUMN           WINELIB_NAME_AW(LVM_SETCOLUMN)
#define LVM_INSERTCOLUMNA       (LVM_FIRST+27)
#define LVM_INSERTCOLUMNW       (LVM_FIRST+97)
#define LVM_INSERTCOLUMN        WINELIB_NAME_AW(LVM_INSERTCOLUMN)
#define LVM_DELETECOLUMN        (LVM_FIRST+28)
#define LVM_GETCOLUMNWIDTH      (LVM_FIRST+29)
#define LVM_SETCOLUMNWIDTH      (LVM_FIRST+30)
#define LVM_GETHEADER           (LVM_FIRST+31)

#define LVM_CREATEDRAGIMAGE     (LVM_FIRST+33)
#define LVM_GETVIEWRECT         (LVM_FIRST+34)
#define LVM_GETTEXTCOLOR        (LVM_FIRST+35)
#define LVM_SETTEXTCOLOR        (LVM_FIRST+36)
#define LVM_GETTEXTBKCOLOR      (LVM_FIRST+37)
#define LVM_SETTEXTBKCOLOR      (LVM_FIRST+38)
#define LVM_GETTOPINDEX         (LVM_FIRST+39)
#define LVM_GETCOUNTPERPAGE     (LVM_FIRST+40)
#define LVM_GETORIGIN           (LVM_FIRST+41)
#define LVM_UPDATE              (LVM_FIRST+42)
#define LVM_SETITEMSTATE        (LVM_FIRST+43)
#define LVM_GETITEMSTATE        (LVM_FIRST+44)
#define LVM_GETITEMTEXTA        (LVM_FIRST+45)
#define LVM_GETITEMTEXTW        (LVM_FIRST+115)
#define LVM_GETITEMTEXT         WINELIB_NAME_AW(LVM_GETITEMTEXT)
#define LVM_SETITEMTEXTA        (LVM_FIRST+46)
#define LVM_SETITEMTEXTW        (LVM_FIRST+116)
#define LVM_SETITEMTEXT         WINELIB_NAME_AW(LVM_SETITEMTEXT)
#define LVM_SETITEMCOUNT        (LVM_FIRST+47)
#define LVM_SORTITEMS           (LVM_FIRST+48)
#define LVM_SORTITEMSEX         (LVM_FIRST+81)
#define LVM_SETITEMPOSITION32   (LVM_FIRST+49)
#define LVM_GETSELECTEDCOUNT    (LVM_FIRST+50)
#define LVM_GETITEMSPACING      (LVM_FIRST+51)
#define LVM_GETISEARCHSTRINGA   (LVM_FIRST+52)
#define LVM_GETISEARCHSTRINGW   (LVM_FIRST+117)
#define LVM_GETISEARCHSTRING    WINELIB_NAME_AW(LVM_GETISEARCHSTRING)
#define LVM_SETICONSPACING      (LVM_FIRST+53)
#define LVM_SETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+54)
#define LVM_GETEXTENDEDLISTVIEWSTYLE (LVM_FIRST+55)
#define LVM_GETSUBITEMRECT      (LVM_FIRST+56)
#define LVM_SUBITEMHITTEST      (LVM_FIRST+57)
#define LVM_SETCOLUMNORDERARRAY (LVM_FIRST+58)
#define LVM_GETCOLUMNORDERARRAY (LVM_FIRST+59)
#define LVM_SETHOTITEM          (LVM_FIRST+60)
#define LVM_GETHOTITEM          (LVM_FIRST+61)
#define LVM_SETHOTCURSOR        (LVM_FIRST+62)
#define LVM_GETHOTCURSOR        (LVM_FIRST+63)
#define LVM_APPROXIMATEVIEWRECT (LVM_FIRST+64)
#define LVM_SETWORKAREAS        (LVM_FIRST+65)
#define LVM_GETSELECTIONMARK    (LVM_FIRST+66)
#define LVM_SETSELECTIONMARK    (LVM_FIRST+67)
#define LVM_SETBKIMAGEA         (LVM_FIRST+68)
#define LVM_SETBKIMAGEW         (LVM_FIRST+138)
#define LVM_SETBKIMAGE          WINELIB_NAME_AW(LVM_SETBKIMAGE)
#define LVM_GETBKIMAGEA         (LVM_FIRST+69)
#define LVM_GETBKIMAGEW         (LVM_FIRST+139)
#define LVM_GETBKIMAGE          WINELIB_NAME_AW(LVM_GETBKIMAGE)
#define LVM_GETWORKAREAS        (LVM_FIRST+70)
#define LVM_SETHOVERTIME        (LVM_FIRST+71)
#define LVM_GETHOVERTIME        (LVM_FIRST+72)
#define LVM_GETNUMBEROFWORKAREAS (LVM_FIRST+73)
#define LVM_SETTOOLTIPS         (LVM_FIRST+74)
#define LVM_GETTOOLTIPS         (LVM_FIRST+78)
#define LVM_GETUNICODEFORMAT    (CCM_GETUNICODEFORMAT)
#define LVM_SETUNICODEFORMAT    (CCM_SETUNICODEFORMAT)
#define LVM_GETGROUPSTATE       (LVM_FIRST + 92)
#define LVM_GETFOCUSEDGROUP     (LVM_FIRST + 93)
#define LVM_GETGROUPRECT        (LVM_FIRST + 98)
#define LVM_SETSELECTEDCOLUMN   (LVM_FIRST + 140)
#define LVM_SETTILEWIDTH        (LVM_FIRST + 141)
#define LVM_SETVIEW             (LVM_FIRST + 142)
#define LVM_GETVIEW             (LVM_FIRST + 143)
#define LVM_INSERTGROUP         (LVM_FIRST + 145)
#define LVM_SETGROUPINFO        (LVM_FIRST + 147)
#define LVM_GETGROUPINFO        (LVM_FIRST + 149)
#define LVM_REMOVEGROUP         (LVM_FIRST + 150)
#define LVM_MOVEGROUP           (LVM_FIRST + 151)
#define LVM_GETGROUPCOUNT       (LVM_FIRST + 152)
#define LVM_GETGROUPINFOBYINDEX (LVM_FIRST + 153)
#define LVM_MOVEITEMTOGROUP     (LVM_FIRST + 154)
#define LVM_SETGROUPMETRICS     (LVM_FIRST + 155)
#define LVM_GETGROUPMETRICS     (LVM_FIRST + 156)
#define LVM_ENABLEGROUPVIEW     (LVM_FIRST + 157)
#define LVM_SORTGROUPS          (LVM_FIRST + 158)
#define LVM_INSERTGROUPSORTED   (LVM_FIRST + 159)
#define LVM_REMOVEALLGROUPS     (LVM_FIRST + 160)
#define LVM_HASGROUP            (LVM_FIRST + 161)
#define LVM_SETTILEVIEWINFO     (LVM_FIRST + 162)
#define LVM_GETTILEVIEWINFO     (LVM_FIRST + 163)
#define LVM_SETTILEINFO         (LVM_FIRST + 164)
#define LVM_GETTILEINFO         (LVM_FIRST + 165)
#define LVM_SETINSERTMARK       (LVM_FIRST + 166)
#define LVM_GETINSERTMARK       (LVM_FIRST + 167)
#define LVM_INSERTMARKHITTEST   (LVM_FIRST + 168)
#define LVM_GETINSERTMARKRECT   (LVM_FIRST + 169)
#define LVM_SETINSERTMARKCOLOR  (LVM_FIRST + 170)
#define LVM_GETINSERTMARKCOLOR  (LVM_FIRST + 171)
#define LVM_SETINFOTIP          (LVM_FIRST + 173)
#define LVM_GETSELECTEDCOLUMN   (LVM_FIRST + 174)
#define LVM_ISGROUPVIEWENABLED  (LVM_FIRST + 175)
#define LVM_GETOUTLINECOLOR     (LVM_FIRST + 176)
#define LVM_SETOUTLINECOLOR     (LVM_FIRST + 177)
#define LVM_CANCELEDITLABEL     (LVM_FIRST + 179)
#define LVM_MAPINDEXTOID        (LVM_FIRST + 180)
#define LVM_MAPIDTOINDEX        (LVM_FIRST + 181)
#define LVM_ISITEMVISIBLE       (LVM_FIRST + 182)
#define LVM_GETEMPTYTEXT        (LVM_FIRST + 204)
#define LVM_GETFOOTERRECT       (LVM_FIRST + 205)
#define LVM_GETFOOTERINFO       (LVM_FIRST + 206)
#define LVM_GETFOOTERITEMRECT   (LVM_FIRST + 207)
#define LVM_GETFOOTERITEM       (LVM_FIRST + 208)
#define LVM_GETITEMINDEXRECT    (LVM_FIRST + 209)
#define LVM_SETITEMINDEXSTATE   (LVM_FIRST + 210)
#define LVM_GETNEXTITEMINDEX    (LVM_FIRST + 211)

#define LVN_FIRST               (0U-100U)
#define LVN_LAST                (0U-199U)
#define LVN_ITEMCHANGING        (LVN_FIRST-0)
#define LVN_ITEMCHANGED         (LVN_FIRST-1)
#define LVN_INSERTITEM          (LVN_FIRST-2)
#define LVN_DELETEITEM          (LVN_FIRST-3)
#define LVN_DELETEALLITEMS      (LVN_FIRST-4)
#define LVN_BEGINLABELEDITA     (LVN_FIRST-5)
#define LVN_BEGINLABELEDITW     (LVN_FIRST-75)
#define LVN_BEGINLABELEDIT      WINELIB_NAME_AW(LVN_BEGINLABELEDIT)
#define LVN_ENDLABELEDITA       (LVN_FIRST-6)
#define LVN_ENDLABELEDITW       (LVN_FIRST-76)
#define LVN_ENDLABELEDIT        WINELIB_NAME_AW(LVN_ENDLABELEDIT)
#define LVN_COLUMNCLICK         (LVN_FIRST-8)
#define LVN_BEGINDRAG           (LVN_FIRST-9)
#define LVN_BEGINRDRAG          (LVN_FIRST-11)
#define LVN_ODCACHEHINT         (LVN_FIRST-13)
#define LVN_ITEMACTIVATE        (LVN_FIRST-14)
#define LVN_ODSTATECHANGED      (LVN_FIRST-15)
#define LVN_HOTTRACK            (LVN_FIRST-21)
#define LVN_ODFINDITEMA         (LVN_FIRST-52)
#define LVN_ODFINDITEMW         (LVN_FIRST-79)
#define LVN_ODFINDITEM          WINELIB_NAME_AW(LVN_ODFINDITEM)
#define LVN_GETDISPINFOA        (LVN_FIRST-50)
#define LVN_GETDISPINFOW        (LVN_FIRST-77)
#define LVN_GETDISPINFO         WINELIB_NAME_AW(LVN_GETDISPINFO)
#define LVN_SETDISPINFOA        (LVN_FIRST-51)
#define LVN_SETDISPINFOW        (LVN_FIRST-78)
#define LVN_SETDISPINFO         WINELIB_NAME_AW(LVN_SETDISPINFO)
#define LVN_KEYDOWN             (LVN_FIRST-55)
#define LVN_MARQUEEBEGIN        (LVN_FIRST-56)
#define LVN_GETINFOTIPA         (LVN_FIRST-57)
#define LVN_GETINFOTIPW         (LVN_FIRST-58)
#define LVN_GETINFOTIP          WINELIB_NAME_AW(LVN_GETINFOTIP)
#define LVN_INCREMENTALSEARCHA  (LVN_FIRST-62)
#define LVN_INCREMENTALSEARCHW  (LVN_FIRST-63)
#define LVN_COLUMNDROPDOWN      (LVN_FIRST-64)
#define LVN_COLUMNOVERFLOWCLICK (LVN_FIRST-66)
#define LVN_INCREMENTALSEARCH   WINELIB_NAME_AW(LVN_INCREMENTALSEARCH)
#define LVN_BEGINSCROLL         (LVN_FIRST-80)
#define LVN_ENDSCROLL           (LVN_FIRST-81)
#define LVN_LINKCLICK           (LVN_FIRST-84)
#define LVN_ASYNCDRAWN          (LVN_FIRST-86)
#define LVN_GETEMPTYMARKUP      (LVN_FIRST-87)

/* LVN_INCREMENTALSEARCH return codes */
#define LVNSCH_DEFAULT          -1
#define LVNSCH_ERROR            -2
#define LVNSCH_IGNORE           -3

#define LVA_DEFAULT             0x0000
#define LVA_ALIGNLEFT           0x0001
#define LVA_ALIGNTOP            0x0002
#define LVA_SNAPTOGRID          0x0005

typedef struct tagLVITEMA
{
    UINT mask;
    INT  iItem;
    INT  iSubItem;
    UINT state;
    UINT stateMask;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
    /* (_WIN32_IE >= 0x0300) */
    INT  iIndent;
    /* (_WIN32_IE >= 0x0560) */
    INT iGroupId;
    UINT cColumns;
    PUINT puColumns;
    /* (_WIN32_WINNT >= 0x0600) */
    PINT piColFmt;
    INT iGroup;
} LVITEMA, *LPLVITEMA;

typedef struct tagLVITEMW
{
    UINT mask;
    INT  iItem;
    INT  iSubItem;
    UINT state;
    UINT stateMask;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
    /* (_WIN32_IE >= 0x0300) */
    INT  iIndent;
    /* (_WIN32_IE >= 0x0560) */
    INT iGroupId;
    UINT cColumns;
    PUINT puColumns;
    /* (_WIN32_WINNT >= 0x0600) */
    PINT piColFmt;
    INT iGroup;
} LVITEMW, *LPLVITEMW;

#define LVITEM   WINELIB_NAME_AW(LVITEM)
#define LPLVITEM WINELIB_NAME_AW(LPLVITEM)

#define LVITEM_V1_SIZEA CCSIZEOF_STRUCT(LVITEMA, lParam)
#define LVITEM_V1_SIZEW CCSIZEOF_STRUCT(LVITEMW, lParam)
#define LVITEM_V1_SIZE WINELIB_NAME_AW(LVITEM_V1_SIZE)

#define LVITEMA_V5_SIZE CCSIZEOF_STRUCT(LVITEMA, puColumns)
#define LVITEMW_V5_SIZE CCSIZEOF_STRUCT(LVITEMW, puColumns)
#define LVITEM_V5_SIZE WINELIB_NAME_AW(LVITEM_V5_SIZE)

#define LV_ITEM  LVITEM
#define LV_ITEMA LVITEMA
#define LV_ITEMW LVITEMW

typedef struct LVSETINFOTIP
{
    UINT cbSize;
    DWORD dwFlags;
    LPWSTR pszText;
    int iItem;
    int iSubItem;
} LVSETINFOTIP, *PLVSETINFOTIP;

/* ListView background image structs and constants
   For _WIN32_IE version 0x400 and later. */

typedef struct tagLVBKIMAGEA
{
    ULONG ulFlags;
    HBITMAP hbm;
    LPSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
} LVBKIMAGEA, *LPLVBKIMAGEA;

typedef struct tagLVBKIMAGEW
{
    ULONG ulFlags;
    HBITMAP hbm;
    LPWSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
} LVBKIMAGEW, *LPLVBKIMAGEW;

#define LVBKIMAGE WINELIB_NAME_AW(LVBKIMAGE)
#define LPLVBKIMAGE WINELIB_NAME_AW(LPLVBKIMAGE)

#define LVBKIF_SOURCE_NONE      0x00000000
#define LVBKIF_SOURCE_HBITMAP   0x00000001
#define LVBKIF_SOURCE_URL       0x00000002
#define LVBKIF_SOURCE_MASK      0x00000003
#define LVBKIF_STYLE_NORMAL     0x00000000
#define LVBKIF_STYLE_TILE       0x00000010
#define LVBKIF_STYLE_MASK       0x00000010
#define LVBKIF_FLAG_TILEOFFSET  0x00000100
#define LVBKIF_TYPE_WATERMARK   0x10000000
#define LVBKIF_FLAG_ALPHABLEND  0x20000000

#define ListView_SetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_SETBKIMAGE, 0, (LPARAM)plvbki)

#define ListView_GetBkImage(hwnd, plvbki) \
    (BOOL)SNDMSG((hwnd), LVM_GETBKIMAGE, 0, (LPARAM)plvbki)

typedef struct tagLVCOLUMNA
{
    UINT mask;
    INT  fmt;
    INT  cx;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iSubItem;
    /* (_WIN32_IE >= 0x0300) */
    INT  iImage;
    INT  iOrder;
    /* (_WIN32_WINNT >= 0x0600) */
    INT  cxMin;
    INT  cxDefault;
    INT  cxIdeal;
} LVCOLUMNA, *LPLVCOLUMNA;

typedef struct tagLVCOLUMNW
{
    UINT mask;
    INT  fmt;
    INT  cx;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iSubItem;
    /* (_WIN32_IE >= 0x0300) */
    INT  iImage;
    INT  iOrder;
    /* (_WIN32_WINNT >= 0x0600) */
    INT  cxMin;
    INT  cxDefault;
    INT  cxIdeal;
} LVCOLUMNW, *LPLVCOLUMNW;

#define LVCOLUMN   WINELIB_NAME_AW(LVCOLUMN)
#define LPLVCOLUMN WINELIB_NAME_AW(LPLVCOLUMN)

#define LVCOLUMN_V1_SIZEA CCSIZEOF_STRUCT(LVCOLUMNA, iSubItem)
#define LVCOLUMN_V1_SIZEW CCSIZEOF_STRUCT(LVCOLUMNW, iSubItem)
#define LVCOLUMN_V1_SIZE WINELIB_NAME_AW(LVCOLUMN_V1_SIZE)

#define LV_COLUMN       LVCOLUMN
#define LV_COLUMNA      LVCOLUMNA
#define LV_COLUMNW      LVCOLUMNW


typedef struct tagNMLISTVIEW
{
    NMHDR hdr;
    INT iItem;
    INT iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM  lParam;
} NMLISTVIEW, *LPNMLISTVIEW;

#define NM_LISTVIEW     NMLISTVIEW
#define LPNM_LISTVIEW   LPNMLISTVIEW

typedef struct tagNMITEMACTIVATE
{
    NMHDR hdr;
    int iItem;
    int iSubItem;
    UINT uNewState;
    UINT uOldState;
    UINT uChanged;
    POINT ptAction;
    LPARAM lParam;
    UINT uKeyFlags;
} NMITEMACTIVATE, *LPNMITEMACTIVATE;

#define LVKF_ALT     0x0001
#define LVKF_CONTROL 0x0002
#define LVKF_SHIFT   0x0004

typedef struct tagLVDISPINFO
{
    NMHDR hdr;
    LVITEMA item;
} NMLVDISPINFOA, *LPNMLVDISPINFOA;

typedef struct tagLVDISPINFOW
{
    NMHDR hdr;
    LVITEMW item;
} NMLVDISPINFOW, *LPNMLVDISPINFOW;

#define NMLVDISPINFO   WINELIB_NAME_AW(NMLVDISPINFO)
#define LPNMLVDISPINFO WINELIB_NAME_AW(LPNMLVDISPINFO)

#define LV_DISPINFO	NMLVDISPINFO
#define LV_DISPINFOA	NMLVDISPINFOA
#define LV_DISPINFOW	NMLVDISPINFOW

#pragma pack(push,1)
typedef struct tagLVKEYDOWN
{
  NMHDR hdr;
  WORD  wVKey;
  UINT flags;
} NMLVKEYDOWN, *LPNMLVKEYDOWN;
#pragma pack(pop)

#define LV_KEYDOWN     NMLVKEYDOWN

typedef struct tagNMLVGETINFOTIPA
{
    NMHDR hdr;
    DWORD dwFlags;
    LPSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
} NMLVGETINFOTIPA, *LPNMLVGETINFOTIPA;

typedef struct tagNMLVGETINFOTIPW
{
    NMHDR hdr;
    DWORD dwFlags;
    LPWSTR pszText;
    int cchTextMax;
    int iItem;
    int iSubItem;
    LPARAM lParam;
} NMLVGETINFOTIPW, *LPNMLVGETINFOTIPW;

#define NMLVGETINFOTIP WINELIB_NAME_AW(NMLVGETINFOTIP)
#define LPNMLVGETINFOTIP WINELIB_NAME_AW(LPNMLVGETINFOTIP)

typedef struct tagLVHITTESTINFO
{
    POINT pt;
    UINT  flags;
    INT   iItem;
    INT   iSubItem;
    /* (_WIN32_WINNT >= 0x0600) */
    INT   iGroup;
} LVHITTESTINFO, *LPLVHITTESTINFO;

#define LV_HITTESTINFO LVHITTESTINFO
#define _LV_HITTESTINFO tagLVHITTESTINFO
#define LVHITTESTINFO_V1_SIZE CCSIZEOF_STRUCT(LVHITTESTINFO,iItem)

typedef struct tagLVFINDINFOA
{
	UINT flags;
	LPCSTR psz;
	LPARAM lParam;
	POINT pt;
	UINT vkDirection;
} LVFINDINFOA, *LPLVFINDINFOA;

typedef struct tagLVFINDINFOW
{
	UINT flags;
	LPCWSTR psz;
	LPARAM lParam;
	POINT pt;
	UINT vkDirection;
} LVFINDINFOW, *LPLVFINDINFOW;

#define LVFINDINFO WINELIB_NAME_AW(LVFINDINFO)
#define LPLVFINDINFO WINELIB_NAME_AW(LPLVFINDINFO)

#define LV_FINDINFO	LVFINDINFO
#define LV_FINDINFOA	LVFINDINFOA
#define LV_FINDINFOW	LVFINDINFOW

/* Groups relates structures */

typedef struct LVGROUP
{
	UINT cbSize;
	UINT mask;
	LPWSTR pszHeader;
	INT cchHeader;
	LPWSTR pszFooter;
	INT cchFooter;
	INT iGroupId;
	UINT stateMask;
	UINT state;
	UINT uAlign;
        /* (_WIN32_WINNT >= 0x0600) */
	LPWSTR  pszSubtitle;
	UINT    cchSubtitle;
	LPWSTR  pszTask;
	UINT    cchTask;
	LPWSTR  pszDescriptionTop;
	UINT    cchDescriptionTop;
	LPWSTR  pszDescriptionBottom;
	UINT    cchDescriptionBottom;
	INT     iTitleImage;
	INT     iExtendedImage;
	INT     iFirstItem;
	UINT    cItems;
	LPWSTR  pszSubsetTitle;
	UINT    cchSubsetTitle;
} LVGROUP, *PLVGROUP;

#define LVGROUP_V5_SIZE CCSIZEOF_STRUCT(LVGROUP, uAlign)

typedef struct LVGROUPMETRICS
{
	UINT cbSize;
	UINT mask;
	UINT Left;
	UINT Top;
	UINT Right;
	UINT Bottom;
	COLORREF crLeft;
	COLORREF crTop;
	COLORREF crRight;
	COLORREF crBottom;
	COLORREF crRightHeader;
	COLORREF crFooter;
} LVGROUPMETRICS, *PLVGROUPMETRICS;

typedef INT (*PFNLVGROUPCOMPARE)(INT, INT, VOID*);

typedef struct LVINSERTGROUPSORTED
{
	PFNLVGROUPCOMPARE pfnGroupCompare;
	LPVOID *pvData;
	LVGROUP lvGroup;
} LVINSERTGROUPSORTED, *PLVINSERTGROUPSORTED;

/* Tile related structures */

typedef struct LVTILEINFO
{
	UINT cbSize;
	int iItem;
	UINT cColumns;
	PUINT puColumns;
        /* (_WIN32_WINNT >= 0x0600) */
	int* piColFmt;
} LVTILEINFO, *PLVTILEINFO;

typedef struct LVTILEVIEWINFO
{
	UINT cbSize;
	DWORD dwMask;
	DWORD dwFlags;
	SIZE sizeTile;
	int cLines;
	RECT rcLabelMargin;
} LVTILEVIEWINFO, *PLVTILEVIEWINFO;

typedef struct LVINSERTMARK
{
	UINT cbSize;
	DWORD dwFlags;
	int iItem;
	DWORD dwReserved;
} LVINSERTMARK, *LPLVINSERTMARK;

typedef struct tagTCHITTESTINFO
{
	POINT pt;
	UINT flags;
} TCHITTESTINFO, *LPTCHITTESTINFO;

#define TC_HITTESTINFO TCHITTESTINFO

typedef INT (CALLBACK *PFNLVCOMPARE)(LPARAM, LPARAM, LPARAM);

#define NMLVCUSTOMDRAW_V3_SIZE CCSIZEOF_STRUCT(NMLCUSTOMDRW, clrTextBk)

typedef struct tagNMLVCUSTOMDRAW
{
    NMCUSTOMDRAW nmcd;
    COLORREF clrText;
    COLORREF clrTextBk;
    int iSubItem;	/* (_WIN32_IE >= 0x0400) */
    DWORD dwItemType;	/* (_WIN32_IE >= 0x560) */
    COLORREF clrFace;   /* (_WIN32_IE >= 0x560) */
    int iIconEffect;	/* (_WIN32_IE >= 0x560) */
    int iIconPhase;	/* (_WIN32_IE >= 0x560) */
    int iPartId;	/* (_WIN32_IE >= 0x560) */
    int iStateId;	/* (_WIN32_IE >= 0x560) */
    RECT rcText;	/* (_WIN32_IE >= 0x560) */
    UINT uAlign;	/* (_WIN32_IE >= 0x560) */
} NMLVCUSTOMDRAW, *LPNMLVCUSTOMDRAW;

typedef struct tagNMLVCACHEHINT
{
	NMHDR	hdr;
	INT	iFrom;
	INT	iTo;
} NMLVCACHEHINT, *LPNMLVCACHEHINT;

#define LPNM_CACHEHINT LPNMLVCACHEHINT
#define PNM_CACHEHINT  LPNMLVCACHEHINT
#define NM_CACHEHINT   NMLVCACHEHINT

typedef struct tagNMLVFINDITEMA
{
    NMHDR hdr;
    int iStart;
    LVFINDINFOA lvfi;
} NMLVFINDITEMA, *LPNMLVFINDITEMA;

typedef struct tagNMLVFINDITEMW
{
    NMHDR hdr;
    int iStart;
    LVFINDINFOW lvfi;
} NMLVFINDITEMW, *LPNMLVFINDITEMW;

#define NMLVFINDITEM   WINELIB_NAME_AW(NMLVFINDITEM)
#define LPNMLVFINDITEM WINELIB_NAME_AW(LPNMLVFINDITEM)
#define NM_FINDITEM    NMLVFINDITEM
#define LPNM_FINDITEM  LPNMLVFINDITEM
#define PNM_FINDITEM   LPNMLVFINDITEM

typedef struct tagNMLVODSTATECHANGE
{
    NMHDR hdr;
    int iFrom;
    int iTo;
    UINT uNewState;
    UINT uOldState;
} NMLVODSTATECHANGE, *LPNMLVODSTATECHANGE;

#define PNM_ODSTATECHANGE LPNMLVODSTATECHANGE
#define LPNM_ODSTATECHANGE LPNMLVODSTATECHANGE
#define NM_ODSTATECHANGE NMLVODSTATECHANGE

typedef struct NMLVSCROLL
{
    NMHDR hdr;
    int dx;
    int dy;
} NMLVSCROLL, *LPNMLVSCROLL;

typedef struct tagLVITEMINDEX
{
    int iItem;
    int iGroup;
} LVITEMINDEX, *PLVITEMINDEX;

#define LVGGR_GROUP      0
#define LVGGR_HEADER     1
#define LVGGR_LABEL      2
#define LVGGR_SUBSETLINK 3

#define ListView_SetItemCount(hwnd,count) \
    (BOOL)SNDMSG((hwnd),LVM_SETITEMCOUNT,(WPARAM)(INT)(count),0)
#define ListView_SetTextBkColor(hwnd,clrBk) \
    (BOOL)SNDMSG((hwnd),LVM_SETTEXTBKCOLOR,0,(LPARAM)(COLORREF)(clrBk))
#define ListView_SetTextColor(hwnd,clrBk) \
    (BOOL)SNDMSG((hwnd),LVM_SETTEXTCOLOR,0,(LPARAM)(COLORREF)(clrBk))
#define ListView_DeleteColumn(hwnd,col)\
    (LRESULT)SNDMSG((hwnd),LVM_DELETECOLUMN,0,(LPARAM)(INT)(col))
#define ListView_GetColumnA(hwnd,x,col)\
    (LRESULT)SNDMSGA((hwnd),LVM_GETCOLUMNA,(WPARAM)(INT)(x),(LPARAM)(LPLVCOLUMNA)(col))
#define ListView_GetColumnW(hwnd,x,col)\
    (LRESULT)SNDMSGW((hwnd),LVM_GETCOLUMNW,(WPARAM)(INT)(x),(LPARAM)(LPLVCOLUMNW)(col))
#define ListView_GetColumn WINELIB_NAME_AW(ListView_GetColumn)
#define ListView_SetColumnA(hwnd,x,col)\
    (LRESULT)SNDMSGA((hwnd),LVM_SETCOLUMNA,(WPARAM)(INT)(x),(LPARAM)(LPLVCOLUMNA)(col))
#define ListView_SetColumnW(hwnd,x,col)\
    (LRESULT)SNDMSGW((hwnd),LVM_SETCOLUMNW,(WPARAM)(INT)(x),(LPARAM)(LPLVCOLUMNW)(col))
#define ListView_SetColumn WINELIB_NAME_AW(ListView_SetColumn)
#define ListView_GetColumnWidth(hwnd,x)\
    (INT)SNDMSG((hwnd), LVM_GETCOLUMNWIDTH, (WPARAM)(INT)(x), 0)
#define ListView_SetColumnWidth(hwnd,x,width)\
    (BOOL)SNDMSG((hwnd),LVM_SETCOLUMNWIDTH,(WPARAM)(INT)(x),(LPARAM)(MAKELPARAM(width,0)))


#define ListView_GetNextItem(hwnd,nItem,flags) \
    (INT)SNDMSG((hwnd),LVM_GETNEXTITEM,(WPARAM)(INT)(nItem),(LPARAM)(MAKELPARAM(flags,0)))
#define ListView_FindItemA(hwnd,nItem,plvfi) \
    (INT)SNDMSGA((hwnd),LVM_FINDITEMA,(WPARAM)(INT)(nItem),(LPARAM)(LVFINDINFOA*)(plvfi))
#define ListView_FindItemW(hwnd,nItem,plvfi) \
    (INT)SNDMSGW((hwnd),LVM_FINDITEMW,(WPARAM)(INT)(nItem),(LPARAM)(LVFINDINFOW*)(plvfi))
#define ListView_FindItem WINELIB_NAME_AW(ListView_FindItem)

#define ListView_Arrange(hwnd,code) \
    (INT)SNDMSG((hwnd), LVM_ARRANGE, (WPARAM)(INT)(code), 0)
#define ListView_GetItemPosition(hwnd,i,ppt) \
    (INT)SNDMSG((hwnd),LVM_GETITEMPOSITION,(WPARAM)(INT)(i),(LPARAM)(LPPOINT)(ppt))
#define ListView_GetItemRect(hwnd,i,prc,code) \
	(BOOL)SNDMSG((hwnd), LVM_GETITEMRECT, (WPARAM)(int)(i), \
	((prc) ? (((RECT*)(prc))->left = (code),(LPARAM)(RECT \
	*)(prc)) : (LPARAM)(RECT*)NULL))
#define ListView_SetItemA(hwnd,pitem) \
    (INT)SNDMSGA((hwnd),LVM_SETITEMA,0,(LPARAM)(const LVITEMA *)(pitem))
#define ListView_SetItemW(hwnd,pitem) \
    (INT)SNDMSGW((hwnd),LVM_SETITEMW,0,(LPARAM)(const LVITEMW *)(pitem))
#define ListView_SetItem WINELIB_NAME_AW(ListView_SetItem)
#define ListView_SetItemState(hwnd,i,data,dataMask) \
{ LVITEM _LVi; _LVi.state = data; _LVi.stateMask = dataMask;\
  SNDMSG(hwnd, LVM_SETITEMSTATE, (WPARAM)(UINT)i, (LPARAM) (LPLVITEM)&_LVi);}
#define ListView_GetItemState(hwnd,i,mask) \
    (UINT)SNDMSG((hwnd),LVM_GETITEMSTATE,(WPARAM)(UINT)(i),(LPARAM)(UINT)(mask))
#define ListView_SetCheckState(hwndLV, i, bCheck) \
    { LVITEM _LVi; _LVi.state = INDEXTOSTATEIMAGEMASK((bCheck)?2:1); _LVi.stateMask = LVIS_STATEIMAGEMASK; \
    SNDMSG(hwndLV, LVM_SETITEMSTATE, (WPARAM)(UINT)(i), (LPARAM)(LPLVITEM)&_LVi);}
#define ListView_GetCheckState(hwndLV, i) \
    (((UINT)SNDMSG((hwndLV), LVM_GETITEMSTATE, (i), LVIS_STATEIMAGEMASK) >> 12) - 1)
#define ListView_GetCountPerPage(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_GETCOUNTPERPAGE, 0, 0)
#define ListView_GetImageList(hwnd,iImageList) \
    (HIMAGELIST)SNDMSG((hwnd), LVM_GETIMAGELIST, (WPARAM)(INT)(iImageList), 0)
#define ListView_GetStringWidthA(hwnd,pstr) \
    (INT)SNDMSGA((hwnd),LVM_GETSTRINGWIDTHA,0,(LPARAM)(LPCSTR)(pstr))
#define ListView_GetStringWidthW(hwnd,pstr) \
    (INT)SNDMSGW((hwnd),LVM_GETSTRINGWIDTHW,0,(LPARAM)(LPCWSTR)(pstr))
#define ListView_GetStringWidth WINELIB_NAME_AW(ListView_GetStringWidth)
#define ListView_GetTopIndex(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_GETTOPINDEX, 0, 0)
#define ListView_Scroll(hwnd,dx,dy) \
    (BOOL)SNDMSG((hwnd),LVM_SCROLL,(WPARAM)(INT)(dx),(LPARAM)(INT)(dy))
#define ListView_EnsureVisible(hwnd,i,fPartialOk) \
    (BOOL)SNDMSG((hwnd),LVM_ENSUREVISIBLE,(WPARAM)(INT)i,(LPARAM)(BOOL)fPartialOk)
#define ListView_SetBkColor(hwnd,clrBk) \
    (BOOL)SNDMSG((hwnd),LVM_SETBKCOLOR,0,(LPARAM)(COLORREF)(clrBk))
#define ListView_SetImageList(hwnd,himl,iImageList) \
    (HIMAGELIST)SNDMSG((hwnd),LVM_SETIMAGELIST,(WPARAM)(iImageList),(LPARAM)(HIMAGELIST)(himl))
#define ListView_GetItemCount(hwnd) \
    (INT)SNDMSG((hwnd), LVM_GETITEMCOUNT, 0, 0)
#define ListView_RedrawItems(hwnd,first,last) \
    (BOOL)SNDMSG((hwnd),LVM_REDRAWITEMS,(WPARAM)(INT)(first),(LPARAM)(INT)(last))
#define ListView_GetEditControl(hwnd) \
    (HWND)SNDMSG((hwnd), LVM_GETEDITCONTROL, 0, 0)
#define ListView_GetTextColor(hwnd)  \
    (COLORREF)SNDMSG((hwnd), LVM_GETTEXTCOLOR, 0, 0)
#define ListView_GetTextBkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), LVM_GETTEXTBKCOLOR, 0, 0)
#define ListView_GetBkColor(hwnd)  \
    (COLORREF)SNDMSG((hwnd), LVM_GETBKCOLOR, 0, 0)
#define ListView_GetItemA(hwnd,pitem) \
    (BOOL)SNDMSGA((hwnd),LVM_GETITEMA,0,(LPARAM)(LVITEMA *)(pitem))
#define ListView_GetItemW(hwnd,pitem) \
    (BOOL)SNDMSGW((hwnd),LVM_GETITEMW,0,(LPARAM)(LVITEMW *)(pitem))
#define ListView_GetItem WINELIB_NAME_AW(ListView_GetItem)
#define ListView_GetOrigin(hwnd,ppt) \
    (BOOL)SNDMSG((hwnd),LVM_GETORIGIN,0,(LPARAM)(POINT *)(ppt))

#define ListView_HitTest(hwnd,pinfo) \
    (INT)SNDMSG((hwnd),LVM_HITTEST,0,(LPARAM)(LPLVHITTESTINFO)(pinfo))

#define ListView_InsertItemA(hwnd,pitem) \
    (INT)SNDMSGA((hwnd),LVM_INSERTITEMA,0,(LPARAM)(const LVITEMA *)(pitem))
#define ListView_InsertItemW(hwnd,pitem) \
    (INT)SNDMSGW((hwnd),LVM_INSERTITEMW,0,(LPARAM)(const LVITEMW *)(pitem))
#define ListView_InsertItem WINELIB_NAME_AW(ListView_InsertItem)

#define ListView_DeleteAllItems(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_DELETEALLITEMS, 0, 0)

#define ListView_InsertColumnA(hwnd,iCol,pcol) \
    (INT)SNDMSGA((hwnd),LVM_INSERTCOLUMNA,(WPARAM)(INT)(iCol),(LPARAM)(const LVCOLUMNA *)(pcol))
#define ListView_InsertColumnW(hwnd,iCol,pcol) \
    (INT)SNDMSGW((hwnd),LVM_INSERTCOLUMNW,(WPARAM)(INT)(iCol),(LPARAM)(const LVCOLUMNW *)(pcol))
#define ListView_InsertColumn WINELIB_NAME_AW(ListView_InsertColumn)

#define ListView_SortItems(hwndLV,_pfnCompare,_lPrm) \
    (BOOL)SNDMSG((hwndLV),LVM_SORTITEMS,(WPARAM)(LPARAM)_lPrm,(LPARAM)(PFNLVCOMPARE)_pfnCompare)
#define ListView_SortItemsEx(hwndLV, _pfnCompare, _lPrm) \
  (BOOL)SNDMSG((hwndLV), LVM_SORTITEMSEX, (WPARAM)(LPARAM)(_lPrm), (LPARAM)(PFNLVCOMPARE)(_pfnCompare))

#define ListView_SetItemPosition(hwndLV, i, x, y) \
    (BOOL)SNDMSG((hwndLV),LVM_SETITEMPOSITION,(WPARAM)(INT)(i),MAKELPARAM((x),(y)))
#define ListView_GetSelectedCount(hwndLV) \
    (UINT)SNDMSG((hwndLV), LVM_GETSELECTEDCOUNT, 0, 0)

#define ListView_EditLabelA(hwndLV, i) \
    (HWND)SNDMSG((hwndLV), LVM_EDITLABELA, (WPARAM)(int)(i), 0)
#define ListView_EditLabelW(hwndLV, i) \
    (HWND)SNDMSG((hwndLV), LVM_EDITLABELW, (WPARAM)(int)(i), 0)
#define ListView_EditLabel WINELIB_NAME_AW(ListView_EditLabel)

#define ListView_GetItemTextA(hwndLV, i, _iSubItem, _pszText, _cchTextMax) \
{ \
    LVITEMA _LVi;\
    _LVi.iSubItem = _iSubItem;\
    _LVi.cchTextMax = _cchTextMax;\
    _LVi.pszText = _pszText;\
    SNDMSGA(hwndLV, LVM_GETITEMTEXTA, (WPARAM)(i), (LPARAM)&_LVi);\
}
#define ListView_GetItemTextW(hwndLV, i, _iSubItem, _pszText, _cchTextMax) \
{ \
    LVITEMW _LVi;\
    _LVi.iSubItem = _iSubItem;\
    _LVi.cchTextMax = _cchTextMax;\
    _LVi.pszText = _pszText;\
    SNDMSGW(hwndLV, LVM_GETITEMTEXTW, (WPARAM)(i), (LPARAM)&_LVi);\
}
#define ListView_GetItemText WINELIB_NAME_AW(ListView_GetItemText)
#define ListView_SetItemPosition32(hwnd,n,x1,y1) \
{ POINT ptNewPos; ptNewPos.x = (x1); ptNewPos.y = (y1); SNDMSG((hwnd), LVM_SETITEMPOSITION32, (WPARAM)(int)(n), (LPARAM)&ptNewPos); }
#define ListView_SetItemTextA(hwndLV, i, _iSubItem, _pszText) \
{ LVITEMA _LVi; _LVi.iSubItem = _iSubItem; _LVi.pszText = _pszText;\
  SNDMSGA(hwndLV, LVM_SETITEMTEXTA, (WPARAM)i, (LPARAM) (LVITEMA*)&_LVi);}
#define ListView_SetItemTextW(hwndLV, i, _iSubItem, _pszText) \
{ LVITEMW _LVi; _LVi.iSubItem = _iSubItem; _LVi.pszText = _pszText;\
  SNDMSGW(hwndLV, LVM_SETITEMTEXTW, (WPARAM)i, (LPARAM) (LVITEMW*)& _LVi);}
#define ListView_SetItemText WINELIB_NAME_AW(ListView_SetItemText)

#define ListView_DeleteItem(hwndLV, i) \
    (BOOL)SNDMSG(hwndLV, LVM_DELETEITEM, (WPARAM)(int)(i), 0)
#define ListView_Update(hwndLV, i) \
    (BOOL)SNDMSG((hwndLV), LVM_UPDATE, (WPARAM)(i), 0)
#define ListView_GetColumnOrderArray(hwndLV, iCount, pi) \
    (BOOL)SNDMSG((hwndLV), LVM_GETCOLUMNORDERARRAY, (WPARAM)iCount, (LPARAM)(LPINT)pi)
#define ListView_GetExtendedListViewStyle(hwndLV) \
    (DWORD)SNDMSG((hwndLV), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0)
#define ListView_GetHotCursor(hwndLV) \
    (HCURSOR)SNDMSG((hwndLV), LVM_GETHOTCURSOR, 0, 0)
#define ListView_GetHotItem(hwndLV) \
    (int)SNDMSG((hwndLV), LVM_GETHOTITEM, 0, 0)
#define ListView_GetItemSpacing(hwndLV, fSmall) \
    (DWORD)SNDMSG((hwndLV), LVM_GETITEMSPACING, (WPARAM)fSmall, 0)
#define ListView_GetSubItemRect(hwndLV, iItem, iSubItem, code, prc) \
    (BOOL)SNDMSG((hwndLV), LVM_GETSUBITEMRECT, (WPARAM)(int)(iItem), \
                       ((prc) ? ((((LPRECT)(prc))->top = iSubItem), (((LPRECT)(prc))->left = code), (LPARAM)(prc)) : 0))
#define ListView_GetToolTips(hwndLV) \
    (HWND)SNDMSG((hwndLV), LVM_GETTOOLTIPS, 0, 0)
#define ListView_SetColumnOrderArray(hwndLV, iCount, pi) \
    (BOOL)SNDMSG((hwndLV), LVM_SETCOLUMNORDERARRAY, (WPARAM)iCount, (LPARAM)(LPINT)pi)
#define ListView_SetExtendedListViewStyle(hwndLV, dw) \
    (DWORD)SNDMSG((hwndLV), LVM_SETEXTENDEDLISTVIEWSTYLE, 0, (LPARAM)dw)
#define ListView_SetExtendedListViewStyleEx(hwndLV, dwMask, dw) \
    (DWORD)SNDMSG((hwndLV), LVM_SETEXTENDEDLISTVIEWSTYLE, (WPARAM)dwMask, (LPARAM)dw)
#define ListView_SetHotCursor(hwndLV, hcur) \
    (HCURSOR)SNDMSG((hwndLV), LVM_SETHOTCURSOR, 0, (LPARAM)hcur)
#define ListView_SetHotItem(hwndLV, i) \
    (int)SNDMSG((hwndLV), LVM_SETHOTITEM, (WPARAM)i, 0)
#define ListView_SetIconSpacing(hwndLV, cx, cy) \
    (DWORD)SNDMSG((hwndLV), LVM_SETICONSPACING, 0, MAKELONG(cx,cy))
#define ListView_SetToolTips(hwndLV, hwndNewHwnd) \
    (HWND)SNDMSG((hwndLV), LVM_SETTOOLTIPS, (WPARAM)hwndNewHwnd, 0)
#define ListView_SubItemHitTest(hwndLV, plvhti) \
    (int)SNDMSG((hwndLV), LVM_SUBITEMHITTEST, 0, (LPARAM)(LPLVHITTESTINFO)(plvhti))
#define ListView_GetSelectionMark(hwndLV) \
    (int)SNDMSG((hwndLV), LVM_GETSELECTIONMARK, 0, 0)
#define ListView_SetSelectionMark(hwndLV, iItem) \
    (int)SNDMSG((hwndLV), LVM_SETSELECTIONMARK, 0, (LPARAM)(iItem))
#define ListView_GetViewRect(hwndLV, prc) \
    (BOOL)SNDMSG((hwndLV),LVM_GETVIEWRECT,0,(LPARAM)(LPRECT)(prc))
#define ListView_GetHeader(hwndLV) \
    (HWND)SNDMSG((hwndLV), LVM_GETHEADER, 0, 0)
#define ListView_SetSelectedColumn(hwnd, iCol) \
    SNDMSG((hwnd), LVM_SETSELECTEDCOLUMN, (WPARAM)iCol, 0)
#define ListView_SetTileWidth(hwnd, cpWidth) \
    SNDMSG((hwnd), LVM_SETTILEWIDTH, (WPARAM)cpWidth, 0)
#define ListView_SetView(hwnd, iView) \
    (DWORD)SNDMSG((hwnd), LVM_SETVIEW, (WPARAM)(DWORD)iView, 0)
#define ListView_GetView(hwnd) \
    (DWORD)SNDMSG((hwnd), LVM_GETVIEW, 0, 0)
#define ListView_InsertGroup(hwnd, index, pgrp) \
    SNDMSG((hwnd), LVM_INSERTGROUP, (WPARAM)index, (LPARAM)pgrp)
#define ListView_SetGroupHeaderImageList(hwnd, himl) \
    SNDMSG((hwnd), LVM_SETIMAGELIST, (WPARAM)LVSIL_GROUPHEADER, (LPARAM)himl)
#define ListView_GetGroupHeaderImageList(hwnd) \
    SNDMSG((hwnd), LVM_GETIMAGELIST, (WPARAM)LVSIL_GROUPHEADER, 0)
#define ListView_SetGroupInfo(hwnd, iGroupId, pgrp) \
    SNDMSG((hwnd), LVM_SETGROUPINFO, (WPARAM)iGroupId, (LPARAM)pgrp)
#define ListView_GetGroupInfo(hwnd, iGroupId, pgrp) \
    SNDMSG((hwnd), LVM_GETGROUPINFO, (WPARAM)iGroupId, (LPARAM)pgrp)
#define ListView_RemoveGroup(hwnd, iGroupId) \
    SNDMSG((hwnd), LVM_REMOVEGROUP, (WPARAM)iGroupId, 0)
#define ListView_MoveGroup(hwnd, iGroupId, toIndex) \
    SNDMSG((hwnd), LVM_MOVEGROUP, (WPARAM)iGroupId, (LPARAM)toIndex)
#define ListView_MoveItemToGroup(hwnd, idItemFrom, idGroupTo) \
    SNDMSG((hwnd), LVM_MOVEITEMTOGROUP, (WPARAM)idItemFrom, (LPARAM)idGroupTo)
#define ListView_SetGroupMetrics(hwnd, pGroupMetrics) \
    SNDMSG((hwnd), LVM_SETGROUPMETRICS, 0, (LPARAM)pGroupMetrics)
#define ListView_GetGroupMetrics(hwnd, pGroupMetrics) \
    SNDMSG((hwnd), LVM_GETGROUPMETRICS, 0, (LPARAM)pGroupMetrics)
#define ListView_EnableGroupView(hwnd, fEnable) \
    SNDMSG((hwnd), LVM_ENABLEGROUPVIEW, (WPARAM)fEnable, 0)
#define ListView_SortGroups(hwnd, _pfnGroupCompate, _plv) \
    SNDMSG((hwnd), LVM_SORTGROUPS, (WPARAM)_pfnGroupCompate, (LPARAM)_plv)
#define ListView_InsertGroupSorted(hwnd, structInsert) \
    SNDMSG((hwnd), LVM_INSERTGROUPSORTED, (WPARAM)structInsert, 0)
#define ListView_RemoveAllGroups(hwnd) \
    SNDMSG((hwnd), LVM_REMOVEALLGROUPS, 0, 0)
#define ListView_HasGroup(hwnd, dwGroupId) \
    SNDMSG((hwnd), LVM_HASGROUP, dwGroupId, 0)
#define ListView_SetTileViewInfo(hwnd, ptvi) \
    SNDMSG((hwnd), LVM_SETTILEVIEWINFO, 0, (LPARAM)ptvi)
#define ListView_GetTileViewInfo(hwnd, ptvi) \
    SNDMSG((hwnd), LVM_GETTILEVIEWINFO, 0, (LPARAM)ptvi)
#define ListView_SetTileInfo(hwnd, pti) \
    SNDMSG((hwnd), LVM_SETTILEINFO, 0, (LPARAM)pti)
#define ListView_GetTileInfo(hwnd, pti) \
    SNDMSG((hwnd), LVM_GETTILEINFO, 0, (LPARAM)pti)
#define ListView_SetInsertMark(hwnd, lvim) \
    (BOOL)SNDMSG((hwnd), LVM_SETINSERTMARK, (WPARAM) 0, (LPARAM) (lvim))
#define ListView_GetInsertMark(hwnd, lvim) \
    (BOOL)SNDMSG((hwnd), LVM_GETINSERTMARK, (WPARAM) 0, (LPARAM) (lvim))
#define ListView_InsertMarkHitTest(hwnd, point, lvim) \
    (int)SNDMSG((hwnd), LVM_INSERTMARKHITTEST, (WPARAM)(LPPOINT)(point), (LPARAM)(LPLVINSERTMARK)(lvim))
#define ListView_GetInsertMarkRect(hwnd, rc) \
    (int)SNDMSG((hwnd), LVM_GETINSERTMARKRECT, (WPARAM)0, (LPARAM)(LPRECT)(rc))
#define ListView_SetInsertMarkColor(hwnd, color) \
    (COLORREF)SNDMSG((hwnd), LVM_SETINSERTMARKCOLOR, (WPARAM)0, (LPARAM)(COLORREF)(color))
#define ListView_GetInsertMarkColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), LVM_GETINSERTMARKCOLOR, (WPARAM)0, (LPARAM)0)
#define ListView_SetInfoTip(hwndLV, plvInfoTip)\
    (BOOL)SNDMSG((hwndLV), LVM_SETINFOTIP, (WPARAM)0, (LPARAM)plvInfoTip)
#define ListView_GetSelectedColumn(hwnd) \
    (UINT)SNDMSG((hwnd), LVM_GETSELECTEDCOLUMN, 0, 0)
#define ListView_IsGroupViewEnabled(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_ISGROUPVIEWENABLED, 0, 0)
#define ListView_GetOutlineColor(hwnd) \
    (COLORREF)SNDMSG((hwnd), LVM_GETOUTLINECOLOR, 0, 0)
#define ListView_SetOutlineColor(hwnd, color) \
    (COLORREF)SNDMSG((hwnd), LVM_SETOUTLINECOLOR, (WPARAM)0, (LPARAM)(COLORREF)(color))
#define ListView_CancelEditLabel(hwnd) \
    (VOID)SNDMSG((hwnd), LVM_CANCELEDITLABEL, (WPARAM)0, (LPARAM)0)
#define ListView_MapIndexToID(hwnd, index) \
    (UINT)SNDMSG((hwnd), LVM_MAPINDEXTOID, (WPARAM)index, (LPARAM)0)
#define ListView_MapIDToIndex(hwnd, id) \
    (UINT)SNDMSG((hwnd), LVM_MAPIDTOINDEX, (WPARAM)id, (LPARAM)0)
#define ListView_SetUnicodeFormat(hwnd, fUnicode) \
    (BOOL)SNDMSG((hwnd), LVM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)
#define ListView_GetUnicodeFormat(hwnd) \
    (BOOL)SNDMSG((hwnd), LVM_GETUNICODEFORMAT, 0, 0)
#define ListView_GetGroupInfoByIndex(hwnd, index, grp) \
    SNDMSG((hwnd), LVM_GETGROUPINFOBYINDEX, (WPARAM)(index), (LPARAM)(grp))
#define ListView_SetGroupState(hwnd, group, masks, states) \
{   LVGROUP level;  level.cbSize = sizeof(level);  level.mask = LVGF_STATE; \
    level.stateMask = masks; level.state = states; \
    SNDMSG((hwnd), LVM_SETGROUPINFO, (WPARAM)(group), (LPARAM)(LVGROUP *)&level); }
#define ListView_IsItemVisible(hwnd, index) \
    (UINT)SNDMSG((hwnd), LVM_ISITEMVISIBLE, (WPARAM)(index), (LPARAM)0)
#define ListView_GetGroupState(hwnd, group, mask) \
    (UINT)SNDMSG((hwnd), LVM_GETGROUPSTATE, (WPARAM)(group), (LPARAM)(mask))
#define ListView_GetFocusedGroup(hwnd) \
    SNDMSG((hwnd), LVM_GETFOCUSEDGROUP, 0, 0)
#define ListView_GetGroupRect(hwnd, group, type, rc) \
    SNDMSG((hwnd), LVM_GETGROUPRECT, (WPARAM)(group), \
      ((rc) ? (((RECT*)(rc))->top = type), (LPARAM)(RECT*)(rc) : (LPARAM)(RECT*)NULL))
#define ListView_GetGroupCount(hwnd) \
    SNDMSG((hwnd), LVM_GETGROUPCOUNT, (WPARAM)0, (LPARAM)0)
#define ListView_GetItemIndexRect(hwnd, index, subitem, code, prc) \
    (BOOL)SNDMSG((hwnd), LVM_GETITEMINDEXRECT, (WPARAM)(LVITEMINDEX*)(index), \
      (prc ? ((((LPRECT)prc)->top = subitem), (((LPRECT)prc)->left = code), (LPARAM)prc) : (LPARAM)NULL))
#define ListView_SetItemIndexState(hwndLV, index, data, mask) \
{   LV_ITEM macro; macro.stateMask = (mask); macro.state = data; \
    SNDMSG((hwndLV), LVM_SETITEMINDEXSTATE, (WPARAM)(LVITEMINDEX*)(index), (LPARAM)(LV_ITEM *)&macro); }
#define ListView_GetNextItemIndex(hwnd, index, flags) \
    (BOOL)SNDMSG((hwnd), LVM_GETNEXTITEMINDEX, (WPARAM)(LVITEMINDEX*)(index), MAKELPARAM((flags),0))

/* Tab Control */

#define WC_TABCONTROLA		"SysTabControl32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_TABCONTROLW         L"SysTabControl32"
#else
static const WCHAR WC_TABCONTROLW[] = { 'S','y','s',
  'T','a','b','C','o','n','t','r','o','l','3','2',0 };
#endif
#define WC_TABCONTROL		WINELIB_NAME_AW(WC_TABCONTROL)

/* tab control styles */
#define TCS_EX_FLATSEPARATORS   0x00000001  /* to be used with */
#define TCS_EX_REGISTERDROP     0x00000002  /* TCM_SETEXTENDEDSTYLE */


#define TCM_FIRST		0x1300

#define TCM_GETIMAGELIST        (TCM_FIRST + 2)
#define TCM_SETIMAGELIST        (TCM_FIRST + 3)
#define TCM_GETITEMCOUNT	(TCM_FIRST + 4)
#define TCM_GETITEM		WINELIB_NAME_AW(TCM_GETITEM)
#define TCM_GETITEMA		(TCM_FIRST + 5)
#define TCM_GETITEMW		(TCM_FIRST + 60)
#define TCM_SETITEMA		(TCM_FIRST + 6)
#define TCM_SETITEMW		(TCM_FIRST + 61)
#define TCM_SETITEM		WINELIB_NAME_AW(TCM_SETITEM)
#define TCM_INSERTITEMA		(TCM_FIRST + 7)
#define TCM_INSERTITEMW		(TCM_FIRST + 62)
#define TCM_INSERTITEM		WINELIB_NAME_AW(TCM_INSERTITEM)
#define TCM_DELETEITEM          (TCM_FIRST + 8)
#define TCM_DELETEALLITEMS      (TCM_FIRST + 9)
#define TCM_GETITEMRECT         (TCM_FIRST + 10)
#define TCM_GETCURSEL		(TCM_FIRST + 11)
#define TCM_SETCURSEL           (TCM_FIRST + 12)
#define TCM_HITTEST             (TCM_FIRST + 13)
#define TCM_SETITEMEXTRA	(TCM_FIRST + 14)
#define TCM_ADJUSTRECT          (TCM_FIRST + 40)
#define TCM_SETITEMSIZE         (TCM_FIRST + 41)
#define TCM_REMOVEIMAGE         (TCM_FIRST + 42)
#define TCM_SETPADDING          (TCM_FIRST + 43)
#define TCM_GETROWCOUNT         (TCM_FIRST + 44)
#define TCM_GETTOOLTIPS         (TCM_FIRST + 45)
#define TCM_SETTOOLTIPS         (TCM_FIRST + 46)
#define TCM_GETCURFOCUS         (TCM_FIRST + 47)
#define TCM_SETCURFOCUS         (TCM_FIRST + 48)
#define TCM_SETMINTABWIDTH     (TCM_FIRST + 49)
#define TCM_DESELECTALL         (TCM_FIRST + 50)
#define TCM_HIGHLIGHTITEM		(TCM_FIRST + 51)
#define TCM_SETEXTENDEDSTYLE	(TCM_FIRST + 52)
#define TCM_GETEXTENDEDSTYLE	(TCM_FIRST + 53)
#define TCM_SETUNICODEFORMAT	CCM_SETUNICODEFORMAT
#define TCM_GETUNICODEFORMAT	CCM_GETUNICODEFORMAT


#define TCIF_TEXT		0x0001
#define TCIF_IMAGE		0x0002
#define TCIF_RTLREADING		0x0004
#define TCIF_PARAM		0x0008
#define TCIF_STATE		0x0010

#define TCIS_BUTTONPRESSED      0x0001
#define TCIS_HIGHLIGHTED        0x0002

/* TabCtrl Macros */
#define TabCtrl_GetImageList(hwnd) \
    (HIMAGELIST)SNDMSG((hwnd), TCM_GETIMAGELIST, 0, 0)
#define TabCtrl_SetImageList(hwnd, himl) \
    (HIMAGELIST)SNDMSG((hwnd), TCM_SETIMAGELIST, 0, (LPARAM)(UINT)(HIMAGELIST)(himl))
#define TabCtrl_GetItemCount(hwnd) \
    (int)SNDMSG((hwnd), TCM_GETITEMCOUNT, 0, 0)
#define TabCtrl_GetItemA(hwnd, iItem, pitem) \
    (BOOL)SNDMSGA((hwnd), TCM_GETITEMA, (WPARAM)(int)iItem, (LPARAM)(TCITEMA *)(pitem))
#define TabCtrl_GetItemW(hwnd, iItem, pitem) \
    (BOOL)SNDMSGW((hwnd), TCM_GETITEMW, (WPARAM)(int)iItem, (LPARAM)(TCITEMW *)(pitem))
#define TabCtrl_GetItem WINELIB_NAME_AW(TabCtrl_GetItem)
#define TabCtrl_SetItemA(hwnd, iItem, pitem) \
    (BOOL)SNDMSGA((hwnd), TCM_SETITEMA, (WPARAM)(int)iItem, (LPARAM)(TCITEMA *)(pitem))
#define TabCtrl_SetItemW(hwnd, iItem, pitem) \
    (BOOL)SNDMSGW((hwnd), TCM_SETITEMW, (WPARAM)(int)iItem, (LPARAM)(TCITEMW *)(pitem))
#define TabCtrl_SetItem WINELIB_NAME_AW(TabCtrl_SetItem)
#define TabCtrl_InsertItemA(hwnd, iItem, pitem)   \
    (int)SNDMSGA((hwnd), TCM_INSERTITEMA, (WPARAM)(int)iItem, (LPARAM)(const TCITEMA *)(pitem))
#define TabCtrl_InsertItemW(hwnd, iItem, pitem)   \
    (int)SNDMSGW((hwnd), TCM_INSERTITEMW, (WPARAM)(int)iItem, (LPARAM)(const TCITEMW *)(pitem))
#define TabCtrl_InsertItem WINELIB_NAME_AW(TabCtrl_InsertItem)
#define TabCtrl_DeleteItem(hwnd, i) \
    (BOOL)SNDMSG((hwnd), TCM_DELETEITEM, (WPARAM)(int)(i), 0)
#define TabCtrl_DeleteAllItems(hwnd) \
    (BOOL)SNDMSG((hwnd), TCM_DELETEALLITEMS, 0, 0)
#define TabCtrl_GetItemRect(hwnd, i, prc) \
    (BOOL)SNDMSG((hwnd), TCM_GETITEMRECT, (WPARAM)(int)(i), (LPARAM)(RECT *)(prc))
#define TabCtrl_GetCurSel(hwnd) \
    (int)SNDMSG((hwnd), TCM_GETCURSEL, 0, 0)
#define TabCtrl_SetCurSel(hwnd, i) \
    (int)SNDMSG((hwnd), TCM_SETCURSEL, (WPARAM)i, 0)
#define TabCtrl_HitTest(hwndTC, pinfo) \
    (int)SNDMSG((hwndTC), TCM_HITTEST, 0, (LPARAM)(TC_HITTESTINFO *)(pinfo))
#define TabCtrl_SetItemExtra(hwndTC, cb) \
    (BOOL)SNDMSG((hwndTC), TCM_SETITEMEXTRA, (WPARAM)(cb), 0)
#define TabCtrl_AdjustRect(hwnd, bLarger, prc) \
    (int)SNDMSG(hwnd, TCM_ADJUSTRECT, (WPARAM)(BOOL)bLarger, (LPARAM)(RECT *)prc)
#define TabCtrl_SetItemSize(hwnd, x, y) \
    (DWORD)SNDMSG((hwnd), TCM_SETITEMSIZE, 0, MAKELPARAM(x,y))
#define TabCtrl_RemoveImage(hwnd, i) \
    (void)SNDMSG((hwnd), TCM_REMOVEIMAGE, i, 0)
#define TabCtrl_SetPadding(hwnd,  cx, cy) \
    (void)SNDMSG((hwnd), TCM_SETPADDING, 0, MAKELPARAM(cx, cy))
#define TabCtrl_GetRowCount(hwnd) \
    (int)SNDMSG((hwnd), TCM_GETROWCOUNT, 0, 0)
#define TabCtrl_GetToolTips(hwnd) \
    (HWND)SNDMSG((hwnd), TCM_GETTOOLTIPS, 0, 0)
#define TabCtrl_SetToolTips(hwnd, hwndTT) \
    (void)SNDMSG((hwnd), TCM_SETTOOLTIPS, (WPARAM)hwndTT, 0)
#define TabCtrl_GetCurFocus(hwnd) \
    (int)SNDMSG((hwnd), TCM_GETCURFOCUS, 0, 0)
#define TabCtrl_SetCurFocus(hwnd, i) \
    SNDMSG((hwnd),TCM_SETCURFOCUS, i, 0)
#define TabCtrl_SetMinTabWidth(hwnd, x) \
    (int)SNDMSG((hwnd), TCM_SETMINTABWIDTH, 0, x)
#define TabCtrl_DeselectAll(hwnd, fExcludeFocus)\
    (void)SNDMSG((hwnd), TCM_DESELECTALL, fExcludeFocus, 0)
#define TabCtrl_GetUnicodeFormat(hwnd) \
    (BOOL)SNDMSG((hwnd), TCM_GETUNICODEFORMAT, 0, 0)
#define TabCtrl_SetUnicodeFormat(hwnd, fUnicode) \
    (BOOL)SNDMSG((hwnd), TCM_SETUNICODEFORMAT, (WPARAM)fUnicode, 0)
#define TabCtrl_GetExtendedStyle(hwnd) \
    (BOOL)SNDMSG((hwnd), TCM_GETEXTENDEDSTYLE, 0, 0)
#define TabCtrl_SetExtendedStyle(hwnd, dwExStyle) \
    (BOOL)SNDMSG((hwnd), TCM_GETEXTENDEDSTYLE, 0, (LPARAM)dwExStyle)
#define TabCtrl_HighlightItem(hwnd, i, fHighlight) \
    (BOOL)SNDMSG((hwnd), TCM_HIGHLIGHTITEM, (WPARAM)i, (LPARAM)MAKELONG(fHighlight, 0))

/* constants for TCHITTESTINFO */

#define TCHT_NOWHERE      0x01
#define TCHT_ONITEMICON   0x02
#define TCHT_ONITEMLABEL  0x04
#define TCHT_ONITEM       (TCHT_ONITEMICON | TCHT_ONITEMLABEL)

typedef struct tagTCITEMHEADERA
{
    UINT  mask;
    UINT  lpReserved1;
    UINT  lpReserved2;
    LPSTR pszText;
    int   cchTextMax;
    int   iImage;
} TCITEMHEADERA, *LPTCITEMHEADERA;

typedef struct tagTCITEMHEADERW
{
    UINT   mask;
    UINT   lpReserved1;
    UINT   lpReserved2;
    LPWSTR pszText;
    int    cchTextMax;
    int    iImage;
} TCITEMHEADERW, *LPTCITEMHEADERW;

#define TCITEMHEADER    WINELIB_NAME_AW(TCITEMHEADER)
#define LPTCITEMHEADER  WINELIB_NAME_AW(LPTCITEMHEADER)
#define TC_ITEMHEADER   WINELIB_NAME_AW(TCITEMHEADER)
#define LPTC_ITEMHEADER WINELIB_NAME_AW(LPTCITEMHEADER)

typedef struct tagTCITEMA
{
    UINT mask;
    UINT dwState;
    UINT dwStateMask;
    LPSTR  pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
} TCITEMA, *LPTCITEMA;

typedef struct tagTCITEMW
{
    UINT mask;
    DWORD  dwState;
    DWORD  dwStateMask;
    LPWSTR pszText;
    INT  cchTextMax;
    INT  iImage;
    LPARAM lParam;
} TCITEMW, *LPTCITEMW;

#define TCITEM    WINELIB_NAME_AW(TCITEM)
#define LPTCITEM  WINELIB_NAME_AW(LPTCITEM)
#define TC_ITEM   WINELIB_NAME_AW(TCITEM)
#define LPTC_ITEM WINELIB_NAME_AW(LPTCITEM)

#define TCN_FIRST               (0U-550U)
#define TCN_LAST                (0U-580U)
#define TCN_KEYDOWN             (TCN_FIRST - 0)
#define TCN_SELCHANGE           (TCN_FIRST - 1)
#define TCN_SELCHANGING         (TCN_FIRST - 2)
#define TCN_GETOBJECT           (TCN_FIRST - 3)
#define TCN_FOCUSCHANGE         (TCN_FIRST - 4)

#pragma pack(push,1)
typedef struct tagTCKEYDOWN
{
    NMHDR hdr;
    WORD wVKey;
    UINT flags;
} NMTCKEYDOWN;
#pragma pack(pop)

#define TC_KEYDOWN              NMTCKEYDOWN

/* ComboBoxEx control */

#define WC_COMBOBOXEXA        "ComboBoxEx32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_COMBOBOXEXW       L"ComboBoxEx32"
#else
static const WCHAR WC_COMBOBOXEXW[] = { 'C','o','m','b','o',
  'B','o','x','E','x','3','2',0 };
#endif
#define WC_COMBOBOXEX           WINELIB_NAME_AW(WC_COMBOBOXEX)

#define CBEM_INSERTITEMA        (WM_USER+1)
#define CBEM_INSERTITEMW        (WM_USER+11)
#define CBEM_INSERTITEM         WINELIB_NAME_AW(CBEM_INSERTITEM)
#define CBEM_SETIMAGELIST       (WM_USER+2)
#define CBEM_GETIMAGELIST       (WM_USER+3)
#define CBEM_GETITEMA           (WM_USER+4)
#define CBEM_GETITEMW           (WM_USER+13)
#define CBEM_GETITEM            WINELIB_NAME_AW(CBEM_GETITEM)
#define CBEM_SETITEMA           (WM_USER+5)
#define CBEM_SETITEMW           (WM_USER+12)
#define CBEM_SETITEM            WINELIB_NAME_AW(CBEM_SETITEM)
#define CBEM_DELETEITEM         CB_DELETESTRING
#define CBEM_GETCOMBOCONTROL    (WM_USER+6)
#define CBEM_GETEDITCONTROL     (WM_USER+7)
#define CBEM_SETEXSTYLE         (WM_USER+8)
#define CBEM_GETEXSTYLE         (WM_USER+9)
#define CBEM_GETEXTENDEDSTYLE   (WM_USER+9)
#define CBEM_SETEXTENDEDSTYLE   (WM_USER+14)
#define CBEM_SETUNICODEFORMAT   CCM_SETUNICODEFORMAT
#define CBEM_GETUNICODEFORMAT   CCM_GETUNICODEFORMAT
#define CBEM_HASEDITCHANGED     (WM_USER+10)
#define CBEM_SETWINDOWTHEME     CCM_SETWINDOWTHEME

#define CBEIF_TEXT              0x00000001
#define CBEIF_IMAGE             0x00000002
#define CBEIF_SELECTEDIMAGE     0x00000004
#define CBEIF_OVERLAY           0x00000008
#define CBEIF_INDENT            0x00000010
#define CBEIF_LPARAM            0x00000020
#define CBEIF_DI_SETITEM        0x10000000

#define CBEN_FIRST              (0U-800U)
#define CBEN_LAST               (0U-830U)

#define CBEN_GETDISPINFOA       (CBEN_FIRST - 0)
#define CBEN_GETDISPINFOW       (CBEN_FIRST - 7)
#define CBEN_GETDISPINFO WINELIB_NAME_AW(CBEN_GETDISPINFO)
#define CBEN_INSERTITEM         (CBEN_FIRST - 1)
#define CBEN_DELETEITEM         (CBEN_FIRST - 2)
#define CBEN_BEGINEDIT          (CBEN_FIRST - 4)
#define CBEN_ENDEDITA           (CBEN_FIRST - 5)
#define CBEN_ENDEDITW           (CBEN_FIRST - 6)
#define CBEN_ENDEDIT WINELIB_NAME_AW(CBEN_ENDEDIT)
#define CBEN_DRAGBEGINA         (CBEN_FIRST - 8)
#define CBEN_DRAGBEGINW         (CBEN_FIRST - 9)
#define CBEN_DRAGBEGIN WINELIB_NAME_AW(CBEN_DRAGBEGIN)

#define CBES_EX_NOEDITIMAGE          0x00000001
#define CBES_EX_NOEDITIMAGEINDENT    0x00000002
#define CBES_EX_PATHWORDBREAKPROC    0x00000004
#define CBES_EX_NOSIZELIMIT          0x00000008
#define CBES_EX_CASESENSITIVE        0x00000010


typedef struct tagCOMBOBOXEXITEMA
{
    UINT mask;
    INT_PTR iItem;
    LPSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} COMBOBOXEXITEMA, *PCOMBOBOXEXITEMA;
typedef COMBOBOXEXITEMA const *PCCOMBOEXITEMA; /* Yes, there's a BOX missing */

typedef struct tagCOMBOBOXEXITEMW
{
    UINT mask;
    INT_PTR iItem;
    LPWSTR pszText;
    int cchTextMax;
    int iImage;
    int iSelectedImage;
    int iOverlay;
    int iIndent;
    LPARAM lParam;
} COMBOBOXEXITEMW, *PCOMBOBOXEXITEMW;
typedef COMBOBOXEXITEMW const *PCCOMBOEXITEMW; /* Yes, there's a BOX missing */

#define COMBOBOXEXITEM WINELIB_NAME_AW(COMBOBOXEXITEM)
#define PCOMBOBOXEXITEM WINELIB_NAME_AW(PCOMBOBOXEXITEM)
#define PCCOMBOBOXEXITEM WINELIB_NAME_AW(PCCOMBOEXITEM) /* Yes, there's a BOX missing */

#define CBENF_KILLFOCUS               1
#define CBENF_RETURN                  2
#define CBENF_ESCAPE                  3
#define CBENF_DROPDOWN                4

#define CBEMAXSTRLEN 260

typedef struct tagNMCBEENDEDITW
{
    NMHDR hdr;
    BOOL fChanged;
    int iNewSelection;
    WCHAR szText[CBEMAXSTRLEN];
    int iWhy;
} NMCBEENDEDITW, *LPNMCBEENDEDITW, *PNMCBEENDEDITW;

typedef struct tagNMCBEENDEDITA
{
    NMHDR hdr;
    BOOL fChanged;
    int iNewSelection;
    char szText[CBEMAXSTRLEN];
    int iWhy;
} NMCBEENDEDITA, *LPNMCBEENDEDITA, *PNMCBEENDEDITA;

#define NMCBEENDEDIT WINELIB_NAME_AW(NMCBEENDEDIT)
#define LPNMCBEENDEDIT WINELIB_NAME_AW(LPNMCBEENDEDIT)
#define PNMCBEENDEDIT WINELIB_NAME_AW(PNMCBEENDEDIT)

typedef struct
{
    NMHDR hdr;
    COMBOBOXEXITEMA ceItem;
} NMCOMBOBOXEXA, *PNMCOMBOBOXEXA;

typedef struct
{
    NMHDR hdr;
    COMBOBOXEXITEMW ceItem;
} NMCOMBOBOXEXW, *PNMCOMBOBOXEXW;

#define NMCOMBOBOXEX WINELIB_NAME_AW(NMCOMBOBOXEX)
#define PNMCOMBOBOXEX WINELIB_NAME_AW(PNMCOMBOBOXEX)

typedef struct
{
    NMHDR hdr;
    int iItemid;
    char szText[CBEMAXSTRLEN];
} NMCBEDRAGBEGINA, *PNMCBEDRAGBEGINA, *LPNMCBEDRAGBEGINA;

typedef struct
{
    NMHDR hdr;
    int iItemid;
    WCHAR szText[CBEMAXSTRLEN];
} NMCBEDRAGBEGINW, *PNMCBEDRAGBEGINW, *LPNMCBEDRAGBEGINW;

#define NMCBEDRAGBEGIN WINELIB_NAME_AW(NMCBEDRAGBEGIN)
#define PNMCBEDRAGBEGIN WINELIB_NAME_AW(PNMCBEDRAGBEGIN)
#define LPNMCBEDRAGBEGIN WINELIB_NAME_AW(LPNMCBEDRAGBEGIN)


/* Hotkey control */

#define HOTKEY_CLASSA           "msctls_hotkey32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define HOTKEY_CLASSW          L"msctls_hotkey32"
#else
static const WCHAR HOTKEY_CLASSW[] = { 'm','s','c','t','l','s','_',
  'h','o','t','k','e','y','3','2',0 };
#endif
#define HOTKEY_CLASS            WINELIB_NAME_AW(HOTKEY_CLASS)

#define HOTKEYF_SHIFT           0x01
#define HOTKEYF_CONTROL         0x02
#define HOTKEYF_ALT             0x04
#define HOTKEYF_EXT             0x08

#define HKCOMB_NONE             0x0001
#define HKCOMB_S                0x0002
#define HKCOMB_C                0x0004
#define HKCOMB_A                0x0008
#define HKCOMB_SC               0x0010
#define HKCOMB_SA               0x0020
#define HKCOMB_CA               0x0040
#define HKCOMB_SCA              0x0080

#define HKM_SETHOTKEY           (WM_USER+1)
#define HKM_GETHOTKEY           (WM_USER+2)
#define HKM_SETRULES            (WM_USER+3)


/* animate control */

#define ANIMATE_CLASSA        "SysAnimate32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define ANIMATE_CLASSW       L"SysAnimate32"
#else
static const WCHAR ANIMATE_CLASSW[] = { 'S','y','s',
  'A','n','i','m','a','t','e','3','2',0 };
#endif
#define ANIMATE_CLASS           WINELIB_NAME_AW(ANIMATE_CLASS)


#define ACM_OPENA               (WM_USER+100)
#define ACM_OPENW               (WM_USER+103)
#define ACM_OPEN                WINELIB_NAME_AW(ACM_OPEN)
#define ACM_PLAY                (WM_USER+101)
#define ACM_STOP                (WM_USER+102)
#define ACM_ISPLAYING           (WM_USER+104)

#define ACN_START               1
#define ACN_STOP                2

#define Animate_CreateA(hwndP,id,dwStyle,hInstance) \
    CreateWindowA(ANIMATE_CLASSA,NULL,dwStyle,0,0,0,0,hwndP,(HMENU)(id),hInstance,NULL)
#define Animate_CreateW(hwndP,id,dwStyle,hInstance) \
    CreateWindowW(ANIMATE_CLASSW,NULL,dwStyle,0,0,0,0,hwndP,(HMENU)(id),hInstance,NULL)
#define Animate_Create WINELIB_NAME_AW(Animate_Create)
#define Animate_OpenA(hwnd,szName) \
    (BOOL)SNDMSGA(hwnd,ACM_OPENA,0,(LPARAM)(LPSTR)(szName))
#define Animate_OpenW(hwnd,szName) \
    (BOOL)SNDMSGW(hwnd,ACM_OPENW,0,(LPARAM)(LPWSTR)(szName))
#define Animate_Open WINELIB_NAME_AW(Animate_Open)
#define Animate_OpenExA(hwnd,hInst,szName) \
    (BOOL)SNDMSGA(hwnd,ACM_OPENA,(WPARAM)hInst,(LPARAM)(LPSTR)(szName))
#define Animate_OpenExW(hwnd,hInst,szName) \
    (BOOL)SNDMSGW(hwnd,ACM_OPENW,(WPARAM)hInst,(LPARAM)(LPWSTR)(szName))
#define Animate_OpenEx WINELIB_NAME_AW(Animate_OpenEx)
#define Animate_Play(hwnd,from,to,rep) \
    (BOOL)SNDMSG(hwnd,ACM_PLAY,(WPARAM)(UINT)(rep),(LPARAM)MAKELONG(from,to))
#define Animate_Stop(hwnd) \
    (BOOL)SNDMSG(hwnd,ACM_STOP,0,0)
#define Animate_Close(hwnd) \
    (BOOL)SNDMSG(hwnd,ACM_OPENA,0,0)
#define Animate_Seek(hwnd,frame) \
    (BOOL)SNDMSG(hwnd,ACM_PLAY,1,(LPARAM)MAKELONG(frame,frame))
#define Animate_IsPlaying(hwnd) \
    (BOOL)SNDMSG(hwnd, ACM_ISPLAYING, 0, 0)

/**************************************************************************
 * IP Address control
 */

#define WC_IPADDRESSA		"SysIPAddress32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_IPADDRESSW          L"SysIPAddress32"
#else
static const WCHAR WC_IPADDRESSW[] = { 'S','y','s',
  'I','P','A','d','d','r','e','s','s','3','2',0 };
#endif
#define WC_IPADDRESS		WINELIB_NAME_AW(WC_IPADDRESS)

#define IPM_CLEARADDRESS	(WM_USER+100)
#define IPM_SETADDRESS		(WM_USER+101)
#define IPM_GETADDRESS		(WM_USER+102)
#define IPM_SETRANGE		(WM_USER+103)
#define IPM_SETFOCUS		(WM_USER+104)
#define IPM_ISBLANK		(WM_USER+105)

#define IPN_FIRST               (0U-860U)
#define IPN_LAST                (0U-879U)
#define IPN_FIELDCHANGED        (IPN_FIRST-0)

typedef struct tagNMIPADDRESS
{
    NMHDR hdr;
    INT iField;
    INT iValue;
} NMIPADDRESS, *LPNMIPADDRESS;

#define MAKEIPRANGE(low,high) \
    ((LPARAM)(WORD)(((BYTE)(high)<<8)+(BYTE)(low)))
#define MAKEIPADDRESS(b1,b2,b3,b4) \
    ((LPARAM)(((DWORD)(b1)<<24)+((DWORD)(b2)<<16)+((DWORD)(b3)<<8)+((DWORD)(b4))))

#define FIRST_IPADDRESS(x)	(((x)>>24)&0xff)
#define SECOND_IPADDRESS(x)	(((x)>>16)&0xff)
#define THIRD_IPADDRESS(x)	(((x)>>8)&0xff)
#define FOURTH_IPADDRESS(x)	((x)&0xff)


/**************************************************************************
 * Native Font control
 */

#define WC_NATIVEFONTCTLA	"NativeFontCtl"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_NATIVEFONTCTLW      L"NativeFontCtl"
#else
static const WCHAR WC_NATIVEFONTCTLW[] = { 'N','a','t','i','v','e',
  'F','o','n','t','C','t','l',0 };
#endif
#define WC_NATIVEFONTCTL	WINELIB_NAME_AW(WC_NATIVEFONTCTL)


/**************************************************************************
 * Month calendar control
 *
 */

#define MONTHCAL_CLASSA	"SysMonthCal32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define MONTHCAL_CLASSW L"SysMonthCal32"
#else
static const WCHAR MONTHCAL_CLASSW[] = { 'S','y','s',
  'M','o','n','t','h','C','a','l','3','2',0 };
#endif
#define MONTHCAL_CLASS		WINELIB_NAME_AW(MONTHCAL_CLASS)

#define MCM_FIRST             0x1000
#define MCN_FIRST             (0U-750U)
#define MCN_LAST              (0U-759U)


#define MCM_GETCURSEL         (MCM_FIRST + 1)
#define MCM_SETCURSEL         (MCM_FIRST + 2)
#define MCM_GETMAXSELCOUNT    (MCM_FIRST + 3)
#define MCM_SETMAXSELCOUNT    (MCM_FIRST + 4)
#define MCM_GETSELRANGE       (MCM_FIRST + 5)
#define MCM_SETSELRANGE       (MCM_FIRST + 6)
#define MCM_GETMONTHRANGE     (MCM_FIRST + 7)
#define MCM_SETDAYSTATE       (MCM_FIRST + 8)
#define MCM_GETMINREQRECT     (MCM_FIRST + 9)
#define MCM_SETCOLOR          (MCM_FIRST + 10)
#define MCM_GETCOLOR          (MCM_FIRST + 11)
#define MCM_SETTODAY          (MCM_FIRST + 12)
#define MCM_GETTODAY          (MCM_FIRST + 13)
#define MCM_HITTEST           (MCM_FIRST + 14)
#define MCM_SETFIRSTDAYOFWEEK (MCM_FIRST + 15)
#define MCM_GETFIRSTDAYOFWEEK (MCM_FIRST + 16)
#define MCM_GETRANGE          (MCM_FIRST + 17)
#define MCM_SETRANGE          (MCM_FIRST + 18)
#define MCM_GETMONTHDELTA     (MCM_FIRST + 19)
#define MCM_SETMONTHDELTA     (MCM_FIRST + 20)
#define MCM_GETMAXTODAYWIDTH  (MCM_FIRST + 21)
#define MCM_GETCURRENTVIEW    (MCM_FIRST + 22)
#define MCM_GETCALENDARCOUNT  (MCM_FIRST + 23)
#define MCM_GETCALENDARGRIDINFO (MCM_FIRST + 24)
#define MCM_GETCALID          (MCM_FIRST + 27)
#define MCM_SETCALID          (MCM_FIRST + 28)
#define MCM_SIZERECTTOMIN     (MCM_FIRST + 29)
#define MCM_SETCALENDARBORDER (MCM_FIRST + 30)
#define MCM_GETCALENDARBORDER (MCM_FIRST + 31)
#define MCM_SETCURRENTVIEW    (MCM_FIRST + 32)
#define MCM_GETUNICODEFORMAT  CCM_GETUNICODEFORMAT
#define MCM_SETUNICODEFORMAT  CCM_SETUNICODEFORMAT


/* Notifications */

#define MCN_VIEWCHANGE        MCN_FIRST
#define MCN_SELCHANGE         (MCN_FIRST + 1)
#define MCN_GETDAYSTATE       (MCN_FIRST + 3)
#define MCN_SELECT            (MCN_FIRST + 4)

#define MCSC_BACKGROUND   0
#define MCSC_TEXT         1
#define MCSC_TITLEBK      2
#define MCSC_TITLETEXT    3
#define MCSC_MONTHBK      4
#define MCSC_TRAILINGTEXT 5


#define MCHT_TITLE             0x00010000
#define MCHT_CALENDAR          0x00020000
#define MCHT_TODAYLINK         0x00030000
#define MCHT_CALENDARCONTROL   0x00100000

#define MCHT_NEXT              0x01000000
#define MCHT_PREV              0x02000000
#define MCHT_NOWHERE           0x00000000
#define MCHT_TITLEBK           (MCHT_TITLE)
#define MCHT_TITLEMONTH        (MCHT_TITLE | 0x0001)
#define MCHT_TITLEYEAR         (MCHT_TITLE | 0x0002)
#define MCHT_TITLEBTNNEXT      (MCHT_TITLE | MCHT_NEXT | 0x0003)
#define MCHT_TITLEBTNPREV      (MCHT_TITLE | MCHT_PREV | 0x0003)

#define MCHT_CALENDARBK        (MCHT_CALENDAR)
#define MCHT_CALENDARDATE      (MCHT_CALENDAR | 0x0001)
#define MCHT_CALENDARDATENEXT  (MCHT_CALENDARDATE | MCHT_NEXT)
#define MCHT_CALENDARDATEPREV  (MCHT_CALENDARDATE | MCHT_PREV)
#define MCHT_CALENDARDAY       (MCHT_CALENDAR | 0x0002)
#define MCHT_CALENDARWEEKNUM   (MCHT_CALENDAR | 0x0003)
#define MCHT_CALENDARDATEMIN   (MCHT_CALENDAR | 0x0004)
#define MCHT_CALENDARDATEMAX   (MCHT_CALENDAR | 0x0005)


#define GMR_VISIBLE     0
#define GMR_DAYSTATE    1

#define MCMV_MONTH      0
#define MCMV_YEAR       1
#define MCMV_DECADE     2
#define MCMV_CENTURY    3
#define MCMV_MAX        MCMV_CENTURY

/*  Month calendar's structures */


typedef struct {
        UINT cbSize;
        POINT pt;
        UINT uHit;
        SYSTEMTIME st;
        /* Vista */
        RECT rc;
        INT iOffset;
        INT iRow;
        INT iCol;
} MCHITTESTINFO, *PMCHITTESTINFO;

#define MCHITTESTINFO_V1_SIZE CCSIZEOF_STRUCT(MCHITTESTINFO, st)

typedef struct tagNMSELCHANGE
{
    NMHDR           nmhdr;
    SYSTEMTIME      stSelStart;
    SYSTEMTIME      stSelEnd;
} NMSELCHANGE, *LPNMSELCHANGE;

typedef NMSELCHANGE NMSELECT, *LPNMSELECT;
typedef DWORD MONTHDAYSTATE, *LPMONTHDAYSTATE;

typedef struct tagNMDAYSTATE
{
    NMHDR           nmhdr;
    SYSTEMTIME      stStart;
    int             cDayState;
    LPMONTHDAYSTATE prgDayState;
} NMDAYSTATE, *LPNMDAYSTATE;

typedef struct tagMCGRIDINFO
{
    UINT       cbSize;
    DWORD      dwPart;
    DWORD      dwFlags;
    INT        iCalendar;
    INT        iRow;
    INT        iCol;
    BOOL       bSelected;
    SYSTEMTIME stStart;
    SYSTEMTIME stEnd;
    RECT       rc;
    WCHAR      *pszName;
    SIZE_T     cchName;
} MCGRIDINFO, *PMCGRIDINFO;

/* macros */

#define MonthCal_GetCurSel(hmc, pst) \
		(BOOL)SNDMSG(hmc, MCM_GETCURSEL, 0, (LPARAM)(pst))
#define MonthCal_SetCurSel(hmc, pst)  \
		(BOOL)SNDMSG(hmc, MCM_SETCURSEL, 0, (LPARAM)(pst))
#define MonthCal_GetMaxSelCount(hmc) \
                (DWORD)SNDMSG(hmc, MCM_GETMAXSELCOUNT, 0, 0)
#define MonthCal_SetMaxSelCount(hmc, n) \
                (BOOL)SNDMSG(hmc, MCM_SETMAXSELCOUNT, (WPARAM)(n), 0)
#define MonthCal_GetSelRange(hmc, rgst) \
		SNDMSG(hmc, MCM_GETSELRANGE, 0, (LPARAM) (rgst))
#define MonthCal_SetSelRange(hmc, rgst) \
		SNDMSG(hmc, MCM_SETSELRANGE, 0, (LPARAM) (rgst))
#define MonthCal_GetMonthRange(hmc, gmr, rgst) \
		(DWORD)SNDMSG(hmc, MCM_GETMONTHRANGE, (WPARAM)(gmr), (LPARAM)(rgst))
#define MonthCal_SetDayState(hmc, cbds, rgds) \
		SNDMSG(hmc, MCM_SETDAYSTATE, (WPARAM)(cbds), (LPARAM)(rgds))
#define MonthCal_GetMinReqRect(hmc, prc) \
		SNDMSG(hmc, MCM_GETMINREQRECT, 0, (LPARAM)(prc))
#define MonthCal_SetColor(hmc, iColor, clr)\
        SNDMSG(hmc, MCM_SETCOLOR, iColor, clr)
#define MonthCal_GetColor(hmc, iColor) \
		SNDMSG(hmc, MCM_SETCOLOR, iColor, 0)
#define MonthCal_GetToday(hmc, pst)\
		(BOOL)SNDMSG(hmc, MCM_GETTODAY, 0, (LPARAM)pst)
#define MonthCal_SetToday(hmc, pst)\
		SNDMSG(hmc, MCM_SETTODAY, 0, (LPARAM)pst)
#define MonthCal_HitTest(hmc, pinfo) \
        SNDMSG(hmc, MCM_HITTEST, 0, (LPARAM)(PMCHITTESTINFO)pinfo)
#define MonthCal_SetFirstDayOfWeek(hmc, iDay) \
        SNDMSG(hmc, MCM_SETFIRSTDAYOFWEEK, 0, iDay)
#define MonthCal_GetFirstDayOfWeek(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETFIRSTDAYOFWEEK, 0, 0)
#define MonthCal_GetRange(hmc, rgst) \
        (DWORD)SNDMSG(hmc, MCM_GETRANGE, 0, (LPARAM)(rgst))
#define MonthCal_SetRange(hmc, gd, rgst) \
        (BOOL)SNDMSG(hmc, MCM_SETRANGE, (WPARAM)(gd), (LPARAM)(rgst))
#define MonthCal_GetMonthDelta(hmc) \
        (int)SNDMSG(hmc, MCM_GETMONTHDELTA, 0, 0)
#define MonthCal_SetMonthDelta(hmc, n) \
        (int)SNDMSG(hmc, MCM_SETMONTHDELTA, n, 0)
#define MonthCal_GetMaxTodayWidth(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETMAXTODAYWIDTH, 0, 0)
#define MonthCal_SetUnicodeFormat(hwnd, fUnicode)  \
        (BOOL)SNDMSG((hwnd), MCM_SETUNICODEFORMAT, (WPARAM)(fUnicode), 0)
#define MonthCal_GetUnicodeFormat(hwnd)  \
        (BOOL)SNDMSG((hwnd), MCM_GETUNICODEFORMAT, 0, 0)
#define MonthCal_SetCALID(hmc, calid) \
        SNDMSG(hmc, MCM_SETCALID, (WPARAM)(calid), 0)
#define MonthCal_GetCALID(hmc) \
        (CALID)SNDMSG(hmc, MCM_GETCALID, 0, 0)
#define MonthCal_GetCalendarCount(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETCALENDARCOUNT, 0, 0)
#define MonthCal_GetCurrentView(hmc) \
        (DWORD)SNDMSG(hmc, MCM_GETCURRENTVIEW, 0, 0)
#define MonthCal_SetCurrentView(hmc, view) \
        (BOOL)SNDMSG(hmc, MCM_SETCURRENTVIEW, 0, (LPARAM)view)
#define MonthCal_SizeRectToMin(hmc, prc) \
        SNDMSG(hmc, MCM_SIZERECTTOMIN, 0, (LPARAM)prc)
#define MonthCal_GetCalendarBorder(hmc) \
        (INT)SNDMSG(hmc, MCM_GETCALENDARBORDER, 0, 0)
#define MonthCal_SetCalendarBorder(hmc, reset, xy) \
        SNDMSG(hmc, MCM_SETCALENDARBORDER, (WPARAM)reset, (LPARAM)xy)
#define MonthCal_GetCalendarGridInfo(hmc, info) \
        (BOOL)SNDMSG(hmc, MCM_GETCALENDARGRIDINFO, 0, (LPARAM)(PMCGRIDINFO)info)

/**************************************************************************
 * Date and time picker control
 */

#define DATETIMEPICK_CLASSA	"SysDateTimePick32"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define DATETIMEPICK_CLASSW    L"SysDateTimePick32"
#else
static const WCHAR DATETIMEPICK_CLASSW[] = { 'S','y','s',
  'D','a','t','e','T','i','m','e','P','i','c','k','3','2',0 };
#endif
#define DATETIMEPICK_CLASS	WINELIB_NAME_AW(DATETIMEPICK_CLASS)

#define DTM_FIRST        0x1000
#define DTN_FIRST       (0U-760U)
#define DTN_LAST        (0U-799U)


#define DTM_GETSYSTEMTIME	(DTM_FIRST+1)
#define DTM_SETSYSTEMTIME	(DTM_FIRST+2)
#define DTM_GETRANGE		(DTM_FIRST+3)
#define DTM_SETRANGE		(DTM_FIRST+4)
#define DTM_SETFORMATA	        (DTM_FIRST+5)
#define DTM_SETFORMATW	        (DTM_FIRST + 50)
#define DTM_SETFORMAT		WINELIB_NAME_AW(DTM_SETFORMAT)
#define DTM_SETMCCOLOR		(DTM_FIRST+6)
#define DTM_GETMCCOLOR		(DTM_FIRST+7)
#define DTM_GETMONTHCAL		(DTM_FIRST+8)
#define DTM_SETMCFONT		(DTM_FIRST+9)
#define DTM_GETMCFONT		(DTM_FIRST+10)
#define DTM_SETMCSTYLE		(DTM_FIRST+11)
#define DTM_GETMCSTYLE		(DTM_FIRST+12)
#define DTM_CLOSEMONTHCAL	(DTM_FIRST+13)
#define DTM_GETDATETIMEPICKERINFO (DTM_FIRST+14)
#define DTM_GETIDEALSIZE	(DTM_FIRST+15)

/* Datetime Notifications */

#define DTN_DATETIMECHANGE  (DTN_FIRST + 1)
#define DTN_USERSTRINGA     (DTN_FIRST + 2)
#define DTN_WMKEYDOWNA      (DTN_FIRST + 3)
#define DTN_FORMATA         (DTN_FIRST + 4)
#define DTN_FORMATQUERYA    (DTN_FIRST + 5)
#define DTN_DROPDOWN        (DTN_FIRST + 6)
#define DTN_CLOSEUP         (DTN_FIRST + 7)
#define DTN_USERSTRINGW     (DTN_FIRST + 15)
#define DTN_WMKEYDOWNW      (DTN_FIRST + 16)
#define DTN_FORMATW         (DTN_FIRST + 17)
#define DTN_FORMATQUERYW    (DTN_FIRST + 18)

#define DTN_USERSTRING      WINELIB_NAME_AW(DTN_USERSTRING)
#define DTN_WMKEYDOWN       WINELIB_NAME_AW(DTN_WMKEYDOWN)
#define DTN_FORMAT          WINELIB_NAME_AW(DTN_FORMAT)
#define DTN_FORMATQUERY     WINELIB_NAME_AW(DTN_FORMATQUERY)


typedef struct tagNMDATETIMECHANGE
{
    NMHDR       nmhdr;
    DWORD       dwFlags;
    SYSTEMTIME  st;
} NMDATETIMECHANGE, *LPNMDATETIMECHANGE;

typedef struct tagNMDATETIMESTRINGA
{
    NMHDR      nmhdr;
    LPCSTR     pszUserString;
    SYSTEMTIME st;
    DWORD      dwFlags;
} NMDATETIMESTRINGA, *LPNMDATETIMESTRINGA;

typedef struct tagNMDATETIMESTRINGW
{
    NMHDR      nmhdr;
    LPCWSTR    pszUserString;
    SYSTEMTIME st;
    DWORD      dwFlags;
} NMDATETIMESTRINGW, *LPNMDATETIMESTRINGW;

DECL_WINELIB_TYPE_AW(NMDATETIMESTRING)
DECL_WINELIB_TYPE_AW(LPNMDATETIMESTRING)

typedef struct tagNMDATETIMEWMKEYDOWNA
{
    NMHDR      nmhdr;
    int        nVirtKey;
    LPCSTR     pszFormat;
    SYSTEMTIME st;
} NMDATETIMEWMKEYDOWNA, *LPNMDATETIMEWMKEYDOWNA;

typedef struct tagNMDATETIMEWMKEYDOWNW
{
    NMHDR      nmhdr;
    int        nVirtKey;
    LPCWSTR    pszFormat;
    SYSTEMTIME st;
} NMDATETIMEWMKEYDOWNW, *LPNMDATETIMEWMKEYDOWNW;

DECL_WINELIB_TYPE_AW(NMDATETIMEWMKEYDOWN)
DECL_WINELIB_TYPE_AW(LPNMDATETIMEWMKEYDOWN)

typedef struct tagNMDATETIMEFORMATA
{
    NMHDR nmhdr;
    LPCSTR  pszFormat;
    SYSTEMTIME st;
    LPCSTR pszDisplay;
    CHAR szDisplay[64];
} NMDATETIMEFORMATA, *LPNMDATETIMEFORMATA;


typedef struct tagNMDATETIMEFORMATW
{
    NMHDR nmhdr;
    LPCWSTR pszFormat;
    SYSTEMTIME st;
    LPCWSTR pszDisplay;
    WCHAR szDisplay[64];
} NMDATETIMEFORMATW, *LPNMDATETIMEFORMATW;

DECL_WINELIB_TYPE_AW(NMDATETIMEFORMAT)
DECL_WINELIB_TYPE_AW(LPNMDATETIMEFORMAT)

typedef struct tagNMDATETIMEFORMATQUERYA
{
    NMHDR nmhdr;
    LPCSTR pszFormat;
    SIZE szMax;
} NMDATETIMEFORMATQUERYA, *LPNMDATETIMEFORMATQUERYA;

typedef struct tagNMDATETIMEFORMATQUERYW
{
    NMHDR nmhdr;
    LPCWSTR pszFormat;
    SIZE szMax;
} NMDATETIMEFORMATQUERYW, *LPNMDATETIMEFORMATQUERYW;

DECL_WINELIB_TYPE_AW(NMDATETIMEFORMATQUERY)
DECL_WINELIB_TYPE_AW(LPNMDATETIMEFORMATQUERY)

typedef struct tagDATETIMEPICKERINFO
{
    DWORD cbSize;
    RECT  rcCheck;
    DWORD stateCheck;
    RECT  rcButton;
    DWORD stateButton;
    HWND  hwndEdit;
    HWND  hwndUD;
    HWND  hwndDropDown;
} DATETIMEPICKERINFO, *LPDATETIMEPICKERINFO;


#define GDT_ERROR    -1
#define GDT_VALID    0
#define GDT_NONE     1

#define GDTR_MIN     0x0001
#define GDTR_MAX     0x0002


#define DateTime_GetSystemtime(hdp, pst)   \
  (DWORD)SNDMSG (hdp, DTM_GETSYSTEMTIME , 0, (LPARAM)(pst))
#define DateTime_SetSystemtime(hdp, gd, pst)   \
  (BOOL)SNDMSG (hdp, DTM_SETSYSTEMTIME, (LPARAM)(gd), (LPARAM)(pst))
#define DateTime_GetRange(hdp, rgst)  \
  (DWORD)SNDMSG (hdp, DTM_GETRANGE, 0, (LPARAM)(rgst))
#define DateTime_SetRange(hdp, gd, rgst) \
   (BOOL)SNDMSG (hdp, DTM_SETRANGE, (WPARAM)(gd), (LPARAM)(rgst))
#define DateTime_SetFormatA(hdp, sz)  \
  (BOOL)SNDMSGA (hdp, DTM_SETFORMATA, 0, (LPARAM)(sz))
#define DateTime_SetFormatW(hdp, sz)  \
  (BOOL)SNDMSGW (hdp, DTM_SETFORMATW, 0, (LPARAM)(sz))
#define DateTime_SetFormat WINELIB_NAME_AW(DateTime_SetFormat)
#define DateTime_GetMonthCalColor(hdp, iColor) \
  SNDMSG (hdp, DTM_GETMCCOLOR, iColor, 0)
#define DateTime_SetMonthCalColor(hdp, iColor, clr) \
  SNDMSG (hdp, DTM_SETMCCOLOR, iColor, clr)
#define DateTime_GetMonthCal(hdp)  \
  (HWND) SNDMSG (hdp, DTM_GETMONTHCAL, 0, 0)
#define DateTime_SetMonthCalFont(hdp, hfont, fRedraw) \
  SNDMSG (hdp, DTM_SETMCFONT, (WPARAM)hfont, (LPARAM)fRedraw)
#define DateTime_GetMonthCalFont(hdp) \
  SNDMSG (hdp, DTM_GETMCFONT, 0, 0)
#define DateTime_GetIdealSize(hdp, sz) \
  (BOOL) SNDMSG (hdp, DTM_GETIDEALSIZE, 0, (LPARAM)sz)
#define DateTime_SetMonthCalStyle(hdp, style) \
  SNDMSG(hdp, DTM_SETMCSTYLE, 0, (LPARAM)style)
#define DateTime_GetMonthCalStyle(hdp) \
  SNDMSG(hdp, DTM_GETMCSTYLE, 0, 0)
#define DateTime_GetDateTimePickerInfo(hdp, info) \
  SNDMSG(hdp, DTM_GETDATETIMEPICKERINFO, 0, (LPARAM)info)
#define DateTime_CloseMonthCal(hdp) \
  SNDMSG(hdp, DTM_CLOSEMONTHCAL, 0, 0)

#define DA_LAST         (0x7fffffff)
#define DPA_APPEND      (0x7fffffff)
#define DPA_ERR         (-1)

#define DSA_APPEND      (0x7fffffff)
#define DSA_ERR         (-1)

struct _DSA;
typedef struct _DSA *HDSA;

typedef INT (CALLBACK *PFNDSAENUMCALLBACK)(LPVOID, LPVOID);

WINCOMMCTRLAPI HDSA   WINAPI DSA_Create(INT, INT);
WINCOMMCTRLAPI BOOL   WINAPI DSA_Destroy(HDSA);
WINCOMMCTRLAPI void   WINAPI DSA_DestroyCallback(HDSA, PFNDSAENUMCALLBACK, LPVOID);
WINCOMMCTRLAPI LPVOID WINAPI DSA_GetItemPtr(HDSA, INT);
WINCOMMCTRLAPI INT    WINAPI DSA_InsertItem(HDSA, INT, LPVOID);

#define DPAS_SORTED             0x0001
#define DPAS_INSERTBEFORE       0x0002
#define DPAS_INSERTAFTER        0x0004


struct _DPA;
typedef struct _DPA *HDPA;

#define DPA_GetPtrCount(hdpa)  (*(INT*)(hdpa))

typedef INT (CALLBACK *PFNDPAENUMCALLBACK)(LPVOID, LPVOID);
typedef INT (CALLBACK *PFNDPACOMPARE)(LPVOID, LPVOID, LPARAM);
typedef PVOID (CALLBACK *PFNDPAMERGE)(UINT,PVOID,PVOID,LPARAM);

/* merge callback codes */
#define DPAMM_MERGE     1
#define DPAMM_DELETE    2
#define DPAMM_INSERT    3

/* merge options */
#define DPAM_SORTED     0x00000001
#define DPAM_NORMAL     0x00000002
#define DPAM_UNION      0x00000004
#define DPAM_INTERSECT  0x00000008

WINCOMMCTRLAPI HDPA      WINAPI DPA_Clone(const HDPA, HDPA);
WINCOMMCTRLAPI HDPA      WINAPI DPA_Create(INT);
WINCOMMCTRLAPI BOOL      WINAPI DPA_Destroy(HDPA);
WINCOMMCTRLAPI LPVOID    WINAPI DPA_DeletePtr(HDPA, INT);
WINCOMMCTRLAPI BOOL      WINAPI DPA_DeleteAllPtrs(HDPA);
WINCOMMCTRLAPI BOOL      WINAPI DPA_SetPtr(HDPA, INT, LPVOID);
WINCOMMCTRLAPI LPVOID    WINAPI DPA_GetPtr(HDPA, INT_PTR);
WINCOMMCTRLAPI INT       WINAPI DPA_GetPtrIndex(HDPA, LPCVOID);
WINCOMMCTRLAPI ULONGLONG WINAPI DPA_GetSize(HDPA);
WINCOMMCTRLAPI BOOL      WINAPI DPA_Grow(HDPA, INT);
WINCOMMCTRLAPI INT       WINAPI DPA_InsertPtr(HDPA, INT, LPVOID);
WINCOMMCTRLAPI BOOL      WINAPI DPA_Sort(HDPA, PFNDPACOMPARE, LPARAM);
WINCOMMCTRLAPI void      WINAPI DPA_EnumCallback(HDPA, PFNDPAENUMCALLBACK, LPVOID);
WINCOMMCTRLAPI void      WINAPI DPA_DestroyCallback(HDPA, PFNDPAENUMCALLBACK, LPVOID);
WINCOMMCTRLAPI INT       WINAPI DPA_Search(HDPA, LPVOID, INT, PFNDPACOMPARE, LPARAM, UINT);
WINCOMMCTRLAPI BOOL      WINAPI DPA_Merge(HDPA, HDPA, DWORD, PFNDPACOMPARE, PFNDPAMERGE, LPARAM);

/* save/load from stream */
typedef struct _DPASTREAMINFO
{
    INT    iPos;   /* item index */
    LPVOID pvItem;
} DPASTREAMINFO;

struct IStream;
typedef HRESULT (CALLBACK *PFNDPASTREAM)(DPASTREAMINFO*, struct IStream*, LPVOID);

WINCOMMCTRLAPI HRESULT WINAPI DPA_LoadStream(HDPA*, PFNDPASTREAM, struct IStream*, LPVOID);
WINCOMMCTRLAPI HRESULT WINAPI DPA_SaveStream(HDPA, PFNDPASTREAM, struct IStream*, LPVOID);

WINCOMMCTRLAPI BOOL WINAPI Str_SetPtrW (LPWSTR *, LPCWSTR);

/**************************************************************************
 * SysLink control
 */

#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_LINK             L"SysLink"
#else
static const WCHAR WC_LINK[] = { 'S','y','s','L','i','n','k',0 };
#endif

/* SysLink styles */
#define LWS_TRANSPARENT      0x0001
#define LWS_IGNORERETURN     0x0002
#define LWS_NOPREFIX         0x0004
#define LWS_USEVISUALSTYLE   0x0008
#define LWS_USECUSTOMTEXT    0x0010
#define LWS_RIGHT            0x0020

/* SysLink messages */
#define LM_HITTEST           (WM_USER + 768)
#define LM_GETIDEALHEIGHT    (WM_USER + 769)
#define LM_GETIDEALSIZE      (LM_GETIDEALHEIGHT)
#define LM_SETITEM           (WM_USER + 770)
#define LM_GETITEM           (WM_USER + 771)

/* SysLink links flags */

#define LIF_ITEMINDEX   1
#define LIF_STATE       2
#define LIF_ITEMID      4
#define LIF_URL         8

/* SysLink links states */

#define LIS_FOCUSED          0x0001
#define LIS_ENABLED          0x0002
#define LIS_VISITED          0x0004
#define LIS_HOTTRACK         0x0008
#define LIS_DEFAULTCOLORS    0x0010

/* SysLink misc. */

#define INVALID_LINK_INDEX  (-1)
#define MAX_LINKID_TEXT     48
#define L_MAX_URL_LENGTH    2084

/* SysLink structures */

typedef struct tagLITEM
{
  UINT mask;
  int iLink;
  UINT state;
  UINT stateMask;
  WCHAR szID[MAX_LINKID_TEXT];
  WCHAR szUrl[L_MAX_URL_LENGTH];
} LITEM, *PLITEM;

typedef struct tagLHITTESTINFO
{
  POINT pt;
  LITEM item;
} LHITTESTINFO, *PLHITTESTINFO;

typedef struct tagNMLINK
{
  NMHDR hdr;
  LITEM item;
} NMLINK, *PNMLINK;

typedef struct tagNMLVLINK
{
    NMHDR hdr;
    LITEM link;
    int iItem;
    int iSubItem;
} NMLVLINK, *PNMLVLINK;

/**************************************************************************
 * Static control
 */

#define WC_STATICA		"Static"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_STATICW 		L"Static"
#else
static const WCHAR WC_STATICW[] = { 'S','t','a','t','i','c',0 };
#endif
#define WC_STATIC		WINELIB_NAME_AW(WC_STATIC)

/**************************************************************************
 * Combobox control
 */

#define WC_COMBOBOXA              "ComboBox"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_COMBOBOXW             L"ComboBox"
#else
static const WCHAR WC_COMBOBOXW[] = { 'C','o','m','b','o','B','o','x',0 };
#endif
#define WC_COMBOBOX               WINELIB_NAME_AW(WC_COMBOBOX)

#define CBM_FIRST                 0x1700
#define CB_SETMINVISIBLE          (CBM_FIRST + 1)
#define CB_GETMINVISIBLE          (CBM_FIRST + 2)
#define CB_SETCUEBANNER           (CBM_FIRST + 3)
#define CB_GETCUEBANNER           (CBM_FIRST + 4)

#define ComboBox_GetMinVisible(hwnd) \
        ((INT)SNDMSG((hwnd), CB_GETMINVISIBLE, 0, 0))
#define ComboBox_SetMinVisible(hwnd, count) \
        ((BOOL)SNDMSG((hwnd), CB_SETMINVISIBLE, (WPARAM)(count), 0))
#define ComboBox_SetCueBannerText(hwnd, text) \
        ((BOOL)SNDMSG((hwnd), CB_SETCUEBANNER, 0, (LPARAM)(text)))
#define ComboBox_GetCueBannerText(hwnd, text, cnt) \
        ((BOOL)SNDMSG((hwnd), CB_GETCUEBANNER, (WPARAM)(text), (LPARAM)(cnt)))

/**************************************************************************
 * Edit control
 */

#define WC_EDITA                  "Edit"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_EDITW                 L"Edit"
#else
static const WCHAR WC_EDITW[] = { 'E','d','i','t',0 };
#endif
#define WC_EDIT                   WINELIB_NAME_AW(WC_EDIT)

typedef struct _tagEDITBALLOONTIP
{
    DWORD cbStruct;
    LPCWSTR pszTitle;
    LPCWSTR pszText;
    INT ttiIcon;
} EDITBALLOONTIP, *PEDITBALLOONTIP;

#define ECM_FIRST                 0x1500
#define EM_SETCUEBANNER           (ECM_FIRST + 1)
#define EM_GETCUEBANNER           (ECM_FIRST + 2)
#define EM_SHOWBALLOONTIP         (ECM_FIRST + 3)
#define EM_HIDEBALLOONTIP         (ECM_FIRST + 4)
#define EM_SETHILITE              (ECM_FIRST + 5)
#define EM_GETHILITE              (ECM_FIRST + 6)
#define EM_NOSETFOCUS             (ECM_FIRST + 7)
#define EM_TAKEFOCUS              (ECM_FIRST + 8)
#define EM_SETEXTENDEDSTYLE       (ECM_FIRST + 10)
#define EM_GETEXTENDEDSTYLE       (ECM_FIRST + 11)
#define EM_SETENDOFLINE           (ECM_FIRST + 12)
#define EM_GETENDOFLINE           (ECM_FIRST + 13)
#define EM_ENABLESEARCHWEB        (ECM_FIRST + 14)
#define EM_SEARCHWEB              (ECM_FIRST + 15)
#define EM_SETCARETINDEX          (ECM_FIRST + 17)
#define EM_GETCARETINDEX          (ECM_FIRST + 18)
#define EM_FILELINEFROMCHAR       (ECM_FIRST + 19)
#define EM_FILELINEINDEX          (ECM_FIRST + 20)
#define EM_FILELINELENGTH         (ECM_FIRST + 21)
#define EM_GETFILELINE            (ECM_FIRST + 22)
#define EM_GETFILELINECOUNT       (ECM_FIRST + 23)

#define EM_GETZOOM                (WM_USER + 224)
#define EM_SETZOOM                (WM_USER + 225)

#define EN_SEARCHWEB              (EN_FIRST)

#define Edit_SetCueBannerText(hwnd, text) \
        (BOOL)SNDMSG((hwnd), EM_SETCUEBANNER, 0, (LPARAM)(text))
#define Edit_SetCueBannerTextFocused(hwnd, text, drawfocused) \
        (BOOL)SNDMSG((hwnd), EM_SETCUEBANNER, (WPARAM)(drawfocused), (LPARAM)(text))
#define Edit_GetCueBannerText(hwnd, buff, buff_size) \
        (BOOL)SNDMSG((hwnd), EM_GETCUEBANNER, (WPARAM)(buff), (LPARAM)(buff_size))
#define Edit_SetHilite(hwnd, start, end) \
        SNDMSG((hwnd), EM_SETHILITE, start, end)
#define Edit_GetHilite(hwnd) \
        ((DWORD)SNDMSG((hwnd), EM_GETHILITE, 0L, 0L))
#define Edit_ShowBalloonTip(hwnd, tip) \
        (BOOL)SNDMSG((hwnd), EM_SHOWBALLOONTIP, 0, (LPARAM)(tip))
#define Edit_HideBalloonTip(hwnd) \
        (BOOL)SNDMSG((hwnd), EM_HIDEBALLOONTIP, 0, 0)

/**************************************************************************
 * Listbox control
 */

#define WC_LISTBOXA               "ListBox"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_LISTBOXW              L"ListBox"
#else
static const WCHAR WC_LISTBOXW[] = { 'L','i','s','t','B','o','x',0 };
#endif
#define WC_LISTBOX                WINELIB_NAME_AW(WC_LISTBOX)

/**************************************************************************
 * Scrollbar control
 */

#define WC_SCROLLBARA             "ScrollBar"
#if defined(_MSC_VER) || defined(__MINGW32__)
# define WC_SCROLLBARW            L"ScrollBar"
#else
static const WCHAR WC_SCROLLBARW[] = { 'S','c','r','o','l','l','B','a','r',0 };
#endif
#define WC_SCROLLBAR              WINELIB_NAME_AW(WC_SCROLLBAR)

/**************************************************************************
 * Task dialog
 */

#ifndef NOTASKDIALOG

#pragma pack(push,1)

enum _TASKDIALOG_FLAGS
{
    TDF_ENABLE_HYPERLINKS           = 0x0001,
    TDF_USE_HICON_MAIN              = 0x0002,
    TDF_USE_HICON_FOOTER            = 0x0004,
    TDF_ALLOW_DIALOG_CANCELLATION   = 0x0008,
    TDF_USE_COMMAND_LINKS           = 0x0010,
    TDF_USE_COMMAND_LINKS_NO_ICON   = 0x0020,
    TDF_EXPAND_FOOTER_AREA          = 0x0040,
    TDF_EXPANDED_BY_DEFAULT         = 0x0080,
    TDF_VERIFICATION_FLAG_CHECKED   = 0x0100,
    TDF_SHOW_PROGRESS_BAR           = 0x0200,
    TDF_SHOW_MARQUEE_PROGRESS_BAR   = 0x0400,
    TDF_CALLBACK_TIMER              = 0x0800,
    TDF_POSITION_RELATIVE_TO_WINDOW = 0x1000,
    TDF_RTL_LAYOUT                  = 0x2000,
    TDF_NO_DEFAULT_RADIO_BUTTON     = 0x4000,
    TDF_CAN_BE_MINIMIZED            = 0x8000,
    TDF_NO_SET_FOREGROUND           = 0x10000,
    TDF_SIZE_TO_CONTENT             = 0x01000000
};
typedef int TASKDIALOG_FLAGS;

typedef enum _TASKDIALOG_MESSAGES
{
    TDM_NAVIGATE_PAGE                       = WM_USER + 101,
    TDM_CLICK_BUTTON                        = WM_USER + 102,
    TDM_SET_MARQUEE_PROGRESS_BAR            = WM_USER + 103,
    TDM_SET_PROGRESS_BAR_STATE              = WM_USER + 104,
    TDM_SET_PROGRESS_BAR_RANGE              = WM_USER + 105,
    TDM_SET_PROGRESS_BAR_POS                = WM_USER + 106,
    TDM_SET_PROGRESS_BAR_MARQUEE            = WM_USER + 107,
    TDM_SET_ELEMENT_TEXT                    = WM_USER + 108,
    TDM_CLICK_RADIO_BUTTON                  = WM_USER + 110,
    TDM_ENABLE_BUTTON                       = WM_USER + 111,
    TDM_ENABLE_RADIO_BUTTON                 = WM_USER + 112,
    TDM_CLICK_VERIFICATION                  = WM_USER + 113,
    TDM_UPDATE_ELEMENT_TEXT                 = WM_USER + 114,
    TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE = WM_USER + 115,
    TDM_UPDATE_ICON                         = WM_USER + 116,
} TASKDIALOG_MESSAGES;

typedef enum _TASKDIALOG_NOTIFICATIONS
{
    TDN_CREATED,
    TDN_NAVIGATED,
    TDN_BUTTON_CLICKED,
    TDN_HYPERLINK_CLICKED,
    TDN_TIMER,
    TDN_DESTROYED,
    TDN_RADIO_BUTTON_CLICKED,
    TDN_DIALOG_CONSTRUCTED,
    TDN_VERIFICATION_CLICKED,
    TDN_HELP,
    TDN_EXPANDO_BUTTON_CLICKED,
} TASKDIALOG_NOTIFICATIONS;

typedef enum _TASKDIALOG_ELEMENTS
{
    TDE_CONTENT,
    TDE_EXPANDED_INFORMATION,
    TDE_FOOTER,
    TDE_MAIN_INSTRUCTION,
} TASKDIALOG_ELEMENTS;

typedef enum _TASKDIALOG_ICON_ELEMENTS
{
    TDIE_ICON_MAIN,
    TDIE_ICON_FOOTER,
} TASKDIALOG_ICON_ELEMENTS;

#define TD_WARNING_ICON        MAKEINTRESOURCEW(-1)
#define TD_ERROR_ICON          MAKEINTRESOURCEW(-2)
#define TD_INFORMATION_ICON    MAKEINTRESOURCEW(-3)
#define TD_SHIELD_ICON         MAKEINTRESOURCEW(-4)

enum _TASKDIALOG_COMMON_BUTTON_FLAGS
{
    TDCBF_OK_BUTTON     = 0x0001,
    TDCBF_YES_BUTTON    = 0x0002,
    TDCBF_NO_BUTTON     = 0x0004,
    TDCBF_CANCEL_BUTTON = 0x0008,
    TDCBF_RETRY_BUTTON  = 0x0010,
    TDCBF_CLOSE_BUTTON  = 0x0020
};
typedef int TASKDIALOG_COMMON_BUTTON_FLAGS;

typedef struct _TASKDIALOG_BUTTON
{
    int     nButtonID;
    PCWSTR  pszButtonText;
} TASKDIALOG_BUTTON;

typedef HRESULT (CALLBACK *PFTASKDIALOGCALLBACK)(HWND, UINT, WPARAM, LPARAM, LONG_PTR);

typedef struct _TASKDIALOGCONFIG
{
    UINT        cbSize;
    HWND        hwndParent;
    HINSTANCE   hInstance;
    TASKDIALOG_FLAGS    dwFlags;
    TASKDIALOG_COMMON_BUTTON_FLAGS  dwCommonButtons;
    PCWSTR      pszWindowTitle;
    union
    {
        HICON   hMainIcon;
        PCWSTR  pszMainIcon;
    } DUMMYUNIONNAME;
    PCWSTR      pszMainInstruction;
    PCWSTR      pszContent;
    UINT        cButtons;
    const TASKDIALOG_BUTTON *pButtons;
    int         nDefaultButton;
    UINT        cRadioButtons;
    const TASKDIALOG_BUTTON *pRadioButtons;
    int         nDefaultRadioButton;
    PCWSTR      pszVerificationText;
    PCWSTR      pszExpandedInformation;
    PCWSTR      pszExpandedControlText;
    PCWSTR      pszCollapsedControlText;
    union
    {
        HICON   hFooterIcon;
        PCWSTR  pszFooterIcon;
    } DUMMYUNIONNAME2;
    PCWSTR      pszFooter;
    PFTASKDIALOGCALLBACK pfCallback;
    LONG_PTR    lpCallbackData;
    UINT        cxWidth;
} TASKDIALOGCONFIG;

WINCOMMCTRLAPI HRESULT WINAPI TaskDialog(HWND owner, HINSTANCE hinst, const WCHAR *title, const WCHAR *main_instruction,
        const WCHAR *content, TASKDIALOG_COMMON_BUTTON_FLAGS common_buttons, const WCHAR *icon, int *button);
WINCOMMCTRLAPI HRESULT WINAPI TaskDialogIndirect(const TASKDIALOGCONFIG *, int *, int *, BOOL *);

#pragma pack(pop)

#endif /* NOTASKDIALOG */

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_COMMCTRL_H */
