#ifndef __WINE_WINDOWS_H
#define __WINE_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "wintypes.h"
#include "winuser.h"

#pragma pack(1)

/* The SIZE structure */

typedef struct
{
    INT16  cx;
    INT16  cy;
} SIZE16, *LPSIZE16;

typedef struct
{
    INT32  cx;
    INT32  cy;
} SIZE32, *LPSIZE32;

DECL_WINELIB_TYPE(SIZE);
DECL_WINELIB_TYPE(LPSIZE);

#define CONV_SIZE16TO32(s16,s32) \
            ((s32)->cx = (INT32)(s16)->cx, (s32)->cy = (INT32)(s16)->cy)
#define CONV_SIZE32TO16(s32,s16) \
            ((s16)->cx = (INT16)(s32)->cx, (s16)->cy = (INT16)(s32)->cy)

/* The POINT structure */

typedef struct
{
    INT16  x;
    INT16  y;
} POINT16, *LPPOINT16;

typedef struct
{
    INT32  x;
    INT32  y;
} POINT32, *LPPOINT32;

DECL_WINELIB_TYPE(POINT);
DECL_WINELIB_TYPE(LPPOINT);

#define CONV_POINT16TO32(p16,p32) \
            ((p32)->x = (INT32)(p16)->x, (p32)->y = (INT32)(p16)->y)
#define CONV_POINT32TO16(p32,p16) \
            ((p16)->x = (INT16)(p32)->x, (p16)->y = (INT16)(p32)->y)

#define MAKEPOINT16(l) (*((POINT16 *)&(l)))
#define MAKEPOINT WINELIB_NAME(MAKEPOINT)

/* The RECT structure */

typedef struct
{
    INT16  left;
    INT16  top;
    INT16  right;
    INT16  bottom;
} RECT16, *LPRECT16;

typedef struct
{
    INT32  left;
    INT32  top;
    INT32  right;
    INT32  bottom;
} RECT32, *LPRECT32;

DECL_WINELIB_TYPE(RECT);
DECL_WINELIB_TYPE(LPRECT);

#define CONV_RECT16TO32(r16,r32) \
    ((r32)->left  = (INT32)(r16)->left,  (r32)->top    = (INT32)(r16)->top, \
     (r32)->right = (INT32)(r16)->right, (r32)->bottom = (INT32)(r16)->bottom)
#define CONV_RECT32TO16(r32,r16) \
    ((r16)->left  = (INT16)(r32)->left,  (r16)->top    = (INT16)(r32)->top, \
     (r16)->right = (INT16)(r32)->right, (r16)->bottom = (INT16)(r32)->bottom)

typedef struct
{
    WORD   wFirst;
    WORD   wSecond;
    INT16  iKernAmount;
} KERNINGPAIR16, *LPKERNINGPAIR16;

typedef struct
{
    WORD   wFirst;
    WORD   wSecond;
    INT32  iKernAmount;
} KERNINGPAIR32, *LPKERNINGPAIR32;

DECL_WINELIB_TYPE(KERNINGPAIR);
DECL_WINELIB_TYPE(LPKERNINGPAIR);

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
    HDC32   hdc;
    BOOL32  fErase;
    RECT32  rcPaint;
    BOOL32  fRestore;
    BOOL32  fIncUpdate;
    BYTE    rgbReserved[32];
} PAINTSTRUCT32, *LPPAINTSTRUCT32;

DECL_WINELIB_TYPE(PAINTSTRUCT);
DECL_WINELIB_TYPE(LPPAINTSTRUCT);


  /* Windows */

typedef struct
{
    LPVOID      lpCreateParams;
    HINSTANCE16 hInstance;
    HMENU16     hMenu;
    HWND16      hwndParent;
    INT16       cy;
    INT16       cx;
    INT16       y;
    INT16       x;
    LONG        style WINE_PACKED;
    SEGPTR      lpszName WINE_PACKED;
    SEGPTR      lpszClass WINE_PACKED;
    DWORD       dwExStyle WINE_PACKED;
} CREATESTRUCT16, *LPCREATESTRUCT16;

typedef struct
{
    LPVOID      lpCreateParams;
    HINSTANCE32 hInstance;
    HMENU32     hMenu;
    HWND32      hwndParent;
    INT32       cy;
    INT32       cx;
    INT32       y;
    INT32       x;
    LONG        style;
    LPCSTR      lpszName;
    LPCSTR      lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCT32A, *LPCREATESTRUCT32A;

typedef struct
{
    LPVOID      lpCreateParams;
    HINSTANCE32 hInstance;
    HMENU32     hMenu;
    HWND32      hwndParent;
    INT32       cy;
    INT32       cx;
    INT32       y;
    INT32       x;
    LONG        style;
    LPCWSTR     lpszName;
    LPCWSTR     lpszClass;
    DWORD       dwExStyle;
} CREATESTRUCT32W, *LPCREATESTRUCT32W;

DECL_WINELIB_TYPE_AW(CREATESTRUCT);
DECL_WINELIB_TYPE_AW(LPCREATESTRUCT);

typedef struct 
{
    HMENU16   hWindowMenu;
    UINT16    idFirstChild;
} CLIENTCREATESTRUCT16, *LPCLIENTCREATESTRUCT16;

typedef struct 
{
    HMENU32   hWindowMenu;
    UINT32    idFirstChild;
} CLIENTCREATESTRUCT32, *LPCLIENTCREATESTRUCT32;

DECL_WINELIB_TYPE(CLIENTCREATESTRUCT);
DECL_WINELIB_TYPE(LPCLIENTCREATESTRUCT);

typedef struct
{
    SEGPTR       szClass;
    SEGPTR       szTitle;
    HINSTANCE16  hOwner;
    INT16        x;
    INT16        y;
    INT16        cx;
    INT16        cy;
    DWORD        style WINE_PACKED;
    LPARAM       lParam WINE_PACKED;
} MDICREATESTRUCT16, *LPMDICREATESTRUCT16;

typedef struct
{
    LPCSTR       szClass;
    LPCSTR       szTitle;
    HINSTANCE32  hOwner;
    INT32        x;
    INT32        y;
    INT32        cx;
    INT32        cy;
    DWORD        style;
    LPARAM       lParam;
} MDICREATESTRUCT32A, *LPMDICREATESTRUCT32A;

typedef struct
{
    LPCWSTR      szClass;
    LPCWSTR      szTitle;
    HINSTANCE32  hOwner;
    INT32        x;
    INT32        y;
    INT32        cx;
    INT32        cy;
    DWORD        style;
    LPARAM       lParam;
} MDICREATESTRUCT32W, *LPMDICREATESTRUCT32W;

DECL_WINELIB_TYPE_AW(MDICREATESTRUCT);
DECL_WINELIB_TYPE_AW(LPMDICREATESTRUCT);

#define MDITILE_VERTICAL    0    
#define MDITILE_HORIZONTAL  1
#define MDIS_ALLCHILDSTYLES 0x0001

  /* Offsets for GetWindowLong() and GetWindowWord() */
#define GWL_USERDATA        (-21)
#define GWL_EXSTYLE         (-20)
#define GWL_STYLE           (-16)
#define GWW_ID              (-12)
#define GWL_ID              GWW_ID
#define GWW_HWNDPARENT      (-8)
#define GWL_HWNDPARENT      GWW_HWNDPARENT
#define GWW_HINSTANCE       (-6)
#define GWL_HINSTANCE       GWW_HINSTANCE
#define GWL_WNDPROC         (-4)
#define DWL_MSGRESULT	    0
#define DWL_DLGPROC	    4
#define DWL_USER	    8

  /* GetWindow() constants */
#define GW_HWNDFIRST	0
#define GW_HWNDLAST	1
#define GW_HWNDNEXT	2
#define GW_HWNDPREV	3
#define GW_OWNER	4
#define GW_CHILD	5

  /* WM_GETMINMAXINFO struct */
typedef struct
{
    POINT16   ptReserved;
    POINT16   ptMaxSize;
    POINT16   ptMaxPosition;
    POINT16   ptMinTrackSize;
    POINT16   ptMaxTrackSize;
} MINMAXINFO16;

typedef struct
{
    POINT32   ptReserved;
    POINT32   ptMaxSize;
    POINT32   ptMaxPosition;
    POINT32   ptMinTrackSize;
    POINT32   ptMaxTrackSize;
} MINMAXINFO32;

DECL_WINELIB_TYPE(MINMAXINFO);

  /* RedrawWindow() flags */
#define RDW_INVALIDATE       0x0001
#define RDW_INTERNALPAINT    0x0002
#define RDW_ERASE            0x0004
#define RDW_VALIDATE         0x0008
#define RDW_NOINTERNALPAINT  0x0010
#define RDW_NOERASE          0x0020
#define RDW_NOCHILDREN       0x0040
#define RDW_ALLCHILDREN      0x0080
#define RDW_UPDATENOW        0x0100
#define RDW_ERASENOW         0x0200
#define RDW_FRAME            0x0400
#define RDW_NOFRAME          0x0800

  /* WM_WINDOWPOSCHANGING/CHANGED struct */
typedef struct
{
    HWND16  hwnd;
    HWND16  hwndInsertAfter;
    INT16   x;
    INT16   y;
    INT16   cx;
    INT16   cy;
    UINT16  flags;
} WINDOWPOS16;

typedef struct
{
    HWND32  hwnd;
    HWND32  hwndInsertAfter;
    INT32   x;
    INT32   y;
    INT32   cx;
    INT32   cy;
    UINT32  flags;
} WINDOWPOS32;

DECL_WINELIB_TYPE(WINDOWPOS);

  /* SetWindowPlacement() struct */
typedef struct
{
    UINT16   length;
    UINT16   flags;
    UINT16   showCmd;
    POINT16  ptMinPosition WINE_PACKED;
    POINT16  ptMaxPosition WINE_PACKED;
    RECT16   rcNormalPosition WINE_PACKED;
} WINDOWPLACEMENT16, *LPWINDOWPLACEMENT16;

typedef struct
{
    UINT32   length;
    UINT32   flags;
    UINT32   showCmd;
    POINT32  ptMinPosition WINE_PACKED;
    POINT32  ptMaxPosition WINE_PACKED;
    RECT32   rcNormalPosition WINE_PACKED;
} WINDOWPLACEMENT32, *LPWINDOWPLACEMENT32;

DECL_WINELIB_TYPE(WINDOWPLACEMENT);
DECL_WINELIB_TYPE(LPWINDOWPLACEMENT);

  /* WINDOWPLACEMENT flags */
#define WPF_SETMINPOSITION      0x0001
#define WPF_RESTORETOMAXIMIZED  0x0002

  /* WM_MOUSEACTIVATE return values */
#define MA_ACTIVATE             1
#define MA_ACTIVATEANDEAT       2
#define MA_NOACTIVATE           3
#define MA_NOACTIVATEANDEAT     4

  /* WM_ACTIVATE wParam values */
#define WA_INACTIVE             0
#define WA_ACTIVE               1
#define WA_CLICKACTIVE          2

  /* WM_NCCALCSIZE parameter structure */
typedef struct
{
    RECT16  rgrc[3];
    SEGPTR  lppos;
} NCCALCSIZE_PARAMS16, *LPNCCALCSIZE_PARAMS16;

typedef struct
{
    RECT32       rgrc[3];
    WINDOWPOS32 *lppos;
} NCCALCSIZE_PARAMS32, *LPNCCALCSIZE_PARAMS32;

DECL_WINELIB_TYPE(NCCALCSIZE_PARAMS);
DECL_WINELIB_TYPE(LPNCCALCSIZE_PARAMS);

  /* WM_NCCALCSIZE return flags */
#define WVR_ALIGNTOP        0x0010
#define WVR_ALIGNLEFT       0x0020
#define WVR_ALIGNBOTTOM     0x0040
#define WVR_ALIGNRIGHT      0x0080
#define WVR_HREDRAW         0x0100
#define WVR_VREDRAW         0x0200
#define WVR_REDRAW          (WVR_HREDRAW | WVR_VREDRAW)
#define WVR_VALIDRECTS      0x0400

  /* WM_NCHITTEST return codes */
#define HTERROR             (-2)
#define HTTRANSPARENT       (-1)
#define HTNOWHERE           0
#define HTCLIENT            1
#define HTCAPTION           2
#define HTSYSMENU           3
#define HTSIZE              4
#define HTMENU              5
#define HTHSCROLL           6
#define HTVSCROLL           7
#define HTMINBUTTON         8
#define HTMAXBUTTON         9
#define HTLEFT              10
#define HTRIGHT             11
#define HTTOP               12
#define HTTOPLEFT           13
#define HTTOPRIGHT          14
#define HTBOTTOM            15
#define HTBOTTOMLEFT        16
#define HTBOTTOMRIGHT       17
#define HTBORDER            18
#define HTGROWBOX           HTSIZE
#define HTREDUCE            HTMINBUTTON
#define HTZOOM              HTMAXBUTTON

  /* WM_SYSCOMMAND parameters */
#define SC_SIZE         0xf000
#define SC_MOVE         0xf010
#define SC_MINIMIZE     0xf020
#define SC_MAXIMIZE     0xf030
#define SC_NEXTWINDOW   0xf040
#define SC_PREVWINDOW   0xf050
#define SC_CLOSE        0xf060
#define SC_VSCROLL      0xf070
#define SC_HSCROLL      0xf080
#define SC_MOUSEMENU    0xf090
#define SC_KEYMENU      0xf100
#define SC_ARRANGE      0xf110
#define SC_RESTORE      0xf120
#define SC_TASKLIST     0xf130
#define SC_SCREENSAVE   0xf140
#define SC_HOTKEY       0xf150

/***** Window hooks *****/

  /* Hook values */
#define WH_MIN		    (-1)
#define WH_MSGFILTER	    (-1)
#define WH_JOURNALRECORD    0
#define WH_JOURNALPLAYBACK  1
#define WH_KEYBOARD	    2
#define WH_GETMESSAGE	    3
#define WH_CALLWNDPROC	    4
#define WH_CBT		    5
#define WH_SYSMSGFILTER	    6
#define WH_MOUSE	    7
#define WH_HARDWARE	    8
#define WH_DEBUG	    9
#define WH_SHELL            10
#define WH_FOREGROUNDIDLE   11
#define WH_CALLWNDPROCRET   12
#define WH_MAX              12

#define WH_MINHOOK          WH_MIN
#define WH_MAXHOOK          WH_MAX
#define WH_NB_HOOKS         (WH_MAXHOOK-WH_MINHOOK+1)

  /* Hook action codes */
#define HC_ACTION           0
#define HC_GETNEXT          1
#define HC_SKIP             2
#define HC_NOREMOVE         3
#define HC_NOREM            HC_NOREMOVE
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF      5

  /* CallMsgFilter() values */
#define MSGF_DIALOGBOX      0
#define MSGF_MESSAGEBOX     1
#define MSGF_MENU           2
#define MSGF_MOVE           3
#define MSGF_SIZE           4
#define MSGF_SCROLLBAR      5
#define MSGF_NEXTWINDOW     6
#define MSGF_MAINLOOP       8
#define MSGF_USER        4096

  /* Windows Exit Procedure flag values */
#define	WEP_FREE_DLL        0
#define	WEP_SYSTEM_EXIT     1

  /* Journalling hook structure */

typedef struct
{
    UINT16  message;
    UINT16  paramL;
    UINT16  paramH;
    DWORD   time WINE_PACKED;
} EVENTMSG16, *LPEVENTMSG16;

typedef struct
{
    UINT32  message;
    UINT32  paramL;
    UINT32  paramH;
    DWORD   time;
    HWND32  hwnd;
} EVENTMSG32, *LPEVENTMSG32;

DECL_WINELIB_TYPE(EVENTMSG);
DECL_WINELIB_TYPE(LPEVENTMSG);

  /* Mouse hook structure */

typedef struct
{
    POINT16 pt;
    HWND16  hwnd;
    UINT16  wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT16, *LPMOUSEHOOKSTRUCT16;

typedef struct
{
    POINT32 pt;
    HWND32  hwnd;
    UINT32  wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT32, *LPMOUSEHOOKSTRUCT32;

DECL_WINELIB_TYPE(MOUSEHOOKSTRUCT);
DECL_WINELIB_TYPE(LPMOUSEHOOKSTRUCT);

  /* Hardware hook structure */

typedef struct
{
    HWND16    hWnd;
    UINT16    wMessage;
    WPARAM16  wParam;
    LPARAM    lParam WINE_PACKED;
} HARDWAREHOOKSTRUCT16, *LPHARDWAREHOOKSTRUCT16;

typedef struct
{
    HWND32    hWnd;
    UINT32    wMessage;
    WPARAM32  wParam;
    LPARAM    lParam;
} HARDWAREHOOKSTRUCT32, *LPHARDWAREHOOKSTRUCT32;

DECL_WINELIB_TYPE(HARDWAREHOOKSTRUCT);
DECL_WINELIB_TYPE(LPHARDWAREHOOKSTRUCT);

  /* CBT hook values */
#define HCBT_MOVESIZE	    0
#define HCBT_MINMAX	    1
#define HCBT_QS 	    2
#define HCBT_CREATEWND	    3
#define HCBT_DESTROYWND	    4
#define HCBT_ACTIVATE	    5
#define HCBT_CLICKSKIPPED   6
#define HCBT_KEYSKIPPED     7
#define HCBT_SYSCOMMAND	    8
#define HCBT_SETFOCUS	    9

  /* CBT hook structures */

typedef struct
{
    CREATESTRUCT16  *lpcs;
    HWND16           hwndInsertAfter;
} CBT_CREATEWND16, *LPCBT_CREATEWND16;

typedef struct
{
    CREATESTRUCT32A *lpcs;
    HWND32           hwndInsertAfter;
} CBT_CREATEWND32A, *LPCBT_CREATEWND32A;

typedef struct
{
    CREATESTRUCT32W *lpcs;
    HWND32           hwndInsertAfter;
} CBT_CREATEWND32W, *LPCBT_CREATEWND32W;

DECL_WINELIB_TYPE_AW(CBT_CREATEWND);
DECL_WINELIB_TYPE_AW(LPCBT_CREATEWND);

typedef struct
{
    BOOL16    fMouse;
    HWND16    hWndActive;
} CBTACTIVATESTRUCT16, *LPCBTACTIVATESTRUCT16;

typedef struct
{
    BOOL32    fMouse;
    HWND32    hWndActive;
} CBTACTIVATESTRUCT32, *LPCBTACTIVATESTRUCT32;

DECL_WINELIB_TYPE(CBTACTIVATESTRUCT);
DECL_WINELIB_TYPE(LPCBTACTIVATESTRUCT);

  /* Shell hook values */
#define HSHELL_WINDOWCREATED       1
#define HSHELL_WINDOWDESTROYED     2
#define HSHELL_ACTIVATESHELLWINDOW 3

  /* Debug hook structure */

typedef struct
{
    HMODULE16   hModuleHook;
    LPARAM	reserved WINE_PACKED;
    LPARAM	lParam WINE_PACKED;
    WPARAM16    wParam;
    INT16       code;
} DEBUGHOOKINFO16, *LPDEBUGHOOKINFO16;

typedef struct
{
    DWORD       idThread;
    DWORD       idThreadInstaller;
    LPARAM      lParam;
    WPARAM32    wParam;
    INT32       code;
} DEBUGHOOKINFO32, *LPDEBUGHOOKINFO32;

DECL_WINELIB_TYPE(DEBUGHOOKINFO);
DECL_WINELIB_TYPE(LPDEBUGHOOKINFO);

/***** Dialogs *****/

  /* cbWndExtra bytes for dialog class */
#define DLGWINDOWEXTRA      30

  /* Dialog styles */
#define DS_ABSALIGN         0x001
#define DS_SYSMODAL         0x002
#define DS_LOCALEDIT        0x020
#define DS_SETFONT          0x040
#define DS_MODALFRAME       0x080
#define DS_NOIDLEMSG        0x100

  /* Dialog messages */
#define DM_GETDEFID         (WM_USER+0)
#define DM_SETDEFID         (WM_USER+1)

#define DC_HASDEFID         0x534b

  /* WM_GETDLGCODE values */
#define DLGC_WANTARROWS      0x0001
#define DLGC_WANTTAB         0x0002
#define DLGC_WANTALLKEYS     0x0004
#define DLGC_WANTMESSAGE     0x0004
#define DLGC_HASSETSEL       0x0008
#define DLGC_DEFPUSHBUTTON   0x0010
#define DLGC_UNDEFPUSHBUTTON 0x0020
#define DLGC_RADIOBUTTON     0x0040
#define DLGC_WANTCHARS       0x0080
#define DLGC_STATIC          0x0100
#define DLGC_BUTTON          0x2000

/* Standard dialog button IDs */
#define IDOK                1
#define IDCANCEL            2
#define IDABORT             3
#define IDRETRY             4
#define IDIGNORE            5
#define IDYES               6
#define IDNO                7


typedef struct
{
    HWND16    hwnd;
    UINT16    message;
    WPARAM16  wParam;
    LPARAM    lParam WINE_PACKED;
    DWORD     time WINE_PACKED;
    POINT16   pt WINE_PACKED;
} MSG16, *LPMSG16;

typedef struct
{
    HWND32    hwnd;
    UINT32    message;
    WPARAM32  wParam;
    LPARAM    lParam;
    DWORD     time;
    POINT32   pt;
} MSG32, *LPMSG32;

DECL_WINELIB_TYPE(MSG);
DECL_WINELIB_TYPE(LPMSG);
	
  /* Raster operations */

#define R2_BLACK         1
#define R2_NOTMERGEPEN   2
#define R2_MASKNOTPEN    3
#define R2_NOTCOPYPEN    4
#define R2_MASKPENNOT    5
#define R2_NOT           6
#define R2_XORPEN        7
#define R2_NOTMASKPEN    8
#define R2_MASKPEN       9
#define R2_NOTXORPEN    10
#define R2_NOP          11
#define R2_MERGENOTPEN  12
#define R2_COPYPEN      13
#define R2_MERGEPENNOT  14
#define R2_MERGEPEN     15
#define R2_WHITE        16

#define SRCCOPY         0xcc0020
#define SRCPAINT        0xee0086
#define SRCAND          0x8800c6
#define SRCINVERT       0x660046
#define SRCERASE        0x440328
#define NOTSRCCOPY      0x330008
#define NOTSRCERASE     0x1100a6
#define MERGECOPY       0xc000ca
#define MERGEPAINT      0xbb0226
#define PATCOPY         0xf00021
#define PATPAINT        0xfb0a09
#define PATINVERT       0x5a0049
#define DSTINVERT       0x550009
#define BLACKNESS       0x000042
#define WHITENESS       0xff0062

  /* StretchBlt() modes */
#define BLACKONWHITE         1
#define WHITEONBLACK         2
#define COLORONCOLOR	     3

#define STRETCH_ANDSCANS     BLACKONWHITE
#define STRETCH_ORSCANS      WHITEONBLACK
#define STRETCH_DELETESCANS  COLORONCOLOR


  /* Colors */

typedef DWORD COLORREF;

#define RGB(r,g,b)          ((COLORREF)((r) | ((g) << 8) | ((b) << 16)))
#define PALETTERGB(r,g,b)   (0x02000000 | RGB(r,g,b))
#define PALETTEINDEX(i)     ((COLORREF)(0x01000000 | (WORD)(i)))

#define GetRValue(rgb)	    ((rgb) & 0xff)
#define GetGValue(rgb)      (((rgb) >> 8) & 0xff)
#define GetBValue(rgb)	    (((rgb) >> 16) & 0xff)

#define COLOR_SCROLLBAR		    0
#define COLOR_BACKGROUND	    1
#define COLOR_ACTIVECAPTION	    2
#define COLOR_INACTIVECAPTION	    3
#define COLOR_MENU		    4
#define COLOR_WINDOW		    5
#define COLOR_WINDOWFRAME	    6
#define COLOR_MENUTEXT		    7
#define COLOR_WINDOWTEXT	    8
#define COLOR_CAPTIONTEXT  	    9
#define COLOR_ACTIVEBORDER	   10
#define COLOR_INACTIVEBORDER	   11
#define COLOR_APPWORKSPACE	   12
#define COLOR_HIGHLIGHT		   13
#define COLOR_HIGHLIGHTTEXT	   14
#define COLOR_BTNFACE              15
#define COLOR_BTNSHADOW            16
#define COLOR_GRAYTEXT             17
#define COLOR_BTNTEXT		   18
#define COLOR_INACTIVECAPTIONTEXT  19
#define COLOR_BTNHIGHLIGHT         20

  /* WM_CTLCOLOR values */
#define CTLCOLOR_MSGBOX             0
#define CTLCOLOR_EDIT               1
#define CTLCOLOR_LISTBOX            2
#define CTLCOLOR_BTN                3
#define CTLCOLOR_DLG                4
#define CTLCOLOR_SCROLLBAR          5
#define CTLCOLOR_STATIC             6

  /* Bitmaps */

typedef struct
{
    INT16  bmType;
    INT16  bmWidth;
    INT16  bmHeight;
    INT16  bmWidthBytes;
    BYTE   bmPlanes;
    BYTE   bmBitsPixel;
    SEGPTR bmBits WINE_PACKED;
} BITMAP16, *LPBITMAP16;

typedef struct
{
    INT32  bmType;
    INT32  bmWidth;
    INT32  bmHeight;
    INT32  bmWidthBytes;
    WORD   bmPlanes;
    WORD   bmBitsPixel;
    LPVOID bmBits WINE_PACKED;
} BITMAP32, *LPBITMAP32;

DECL_WINELIB_TYPE(BITMAP);
DECL_WINELIB_TYPE(LPBITMAP);

  /* Brushes */

typedef struct
{ 
    UINT16     lbStyle;
    COLORREF   lbColor WINE_PACKED;
    INT16      lbHatch;
} LOGBRUSH16, *LPLOGBRUSH16;

typedef struct
{ 
    UINT32     lbStyle;
    COLORREF   lbColor;
    INT32      lbHatch;
} LOGBRUSH32, *LPLOGBRUSH32;

DECL_WINELIB_TYPE(LOGBRUSH);
DECL_WINELIB_TYPE(LPLOGBRUSH);

  /* Brush styles */
#define BS_SOLID	    0
#define BS_NULL		    1
#define BS_HOLLOW	    1
#define BS_HATCHED	    2
#define BS_PATTERN	    3
#define BS_INDEXED	    4
#define	BS_DIBPATTERN	    5

  /* Hatch styles */
#define HS_HORIZONTAL       0
#define HS_VERTICAL         1
#define HS_FDIAGONAL        2
#define HS_BDIAGONAL        3
#define HS_CROSS            4
#define HS_DIAGCROSS        5

  /* Fonts */

#define LF_FACESIZE     32
#define LF_FULLFACESIZE 64

typedef struct
{
    INT16  lfHeight;
    INT16  lfWidth;
    INT16  lfEscapement;
    INT16  lfOrientation;
    INT16  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE] WINE_PACKED;
} LOGFONT16, *LPLOGFONT16;

typedef struct
{
    INT32  lfHeight;
    INT32  lfWidth;
    INT32  lfEscapement;
    INT32  lfOrientation;
    INT32  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    CHAR   lfFaceName[LF_FACESIZE];
} LOGFONT32A, *LPLOGFONT32A;

typedef struct
{
    INT32  lfHeight;
    INT32  lfWidth;
    INT32  lfEscapement;
    INT32  lfOrientation;
    INT32  lfWeight;
    BYTE   lfItalic;
    BYTE   lfUnderline;
    BYTE   lfStrikeOut;
    BYTE   lfCharSet;
    BYTE   lfOutPrecision;
    BYTE   lfClipPrecision;
    BYTE   lfQuality;
    BYTE   lfPitchAndFamily;
    WCHAR  lfFaceName[LF_FACESIZE];
} LOGFONT32W, *LPLOGFONT32W;

DECL_WINELIB_TYPE_AW(LOGFONT);
DECL_WINELIB_TYPE_AW(LPLOGFONT);

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT16, *LPENUMLOGFONT16;

typedef struct
{
  LOGFONT32A elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT32A, *LPENUMLOGFONT32A;

typedef struct
{
  LOGFONT32W elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  WCHAR      elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT32W, *LPENUMLOGFONT32W;

typedef struct
{
  LOGFONT16  elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
  BYTE       elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX16, *LPENUMLOGFONTEX16;

typedef struct
{
  LOGFONT32A elfLogFont;
  BYTE       elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE       elfStyle[LF_FACESIZE] WINE_PACKED;
  BYTE       elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX32A,*LPENUMLOGFONTEX32A;

typedef struct
{
  LOGFONT32W elfLogFont;
  WCHAR      elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  WCHAR      elfStyle[LF_FACESIZE] WINE_PACKED;
  WCHAR      elfScript[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONTEX32W,*LPENUMLOGFONTEX32W;

DECL_WINELIB_TYPE_AW(ENUMLOGFONT);
DECL_WINELIB_TYPE_AW(LPENUMLOGFONT);
DECL_WINELIB_TYPE_AW(LPENUMLOGFONTEX);

typedef struct
{
  DWORD fsUsb[4];
  DWORD fsCsb[2];
} FONTSIGNATURE,*LPFONTSIGNATURE;


  /* lfWeight values */
#define FW_DONTCARE	    0
#define FW_THIN 	    100
#define FW_EXTRALIGHT	    200
#define FW_ULTRALIGHT	    200
#define FW_LIGHT	    300
#define FW_NORMAL	    400
#define FW_REGULAR	    400
#define FW_MEDIUM	    500
#define FW_SEMIBOLD	    600
#define FW_DEMIBOLD	    600
#define FW_BOLD 	    700
#define FW_EXTRABOLD	    800
#define FW_ULTRABOLD	    800
#define FW_HEAVY	    900
#define FW_BLACK	    900

  /* lfCharSet values */
#define ANSI_CHARSET	      0
#define DEFAULT_CHARSET       1
#define SYMBOL_CHARSET	      2
#define SHIFTJIS_CHARSET      128
#define HANGEUL_CHARSET       129
#define CHINESEBIG5_CHARSET   136
#define OEM_CHARSET	      255

  /* lfOutPrecision values */
#define OUT_DEFAULT_PRECIS	0
#define OUT_STRING_PRECIS	1
#define OUT_CHARACTER_PRECIS	2
#define OUT_STROKE_PRECIS	3
#define OUT_TT_PRECIS		4
#define OUT_DEVICE_PRECIS	5
#define OUT_RASTER_PRECIS	6
#define OUT_TT_ONLY_PRECIS	7

  /* lfClipPrecision values */
#define CLIP_DEFAULT_PRECIS     0x00
#define CLIP_CHARACTER_PRECIS   0x01
#define CLIP_STROKE_PRECIS      0x02
#define CLIP_MASK		0x0F
#define CLIP_LH_ANGLES		0x10
#define CLIP_TT_ALWAYS		0x20
#define CLIP_EMBEDDED		0x80

  /* lfQuality values */
#define DEFAULT_QUALITY     0
#define DRAFT_QUALITY       1
#define PROOF_QUALITY       2

  /* lfPitchAndFamily pitch values */
#define DEFAULT_PITCH       0x00
#define FIXED_PITCH         0x01
#define VARIABLE_PITCH      0x02
#define FF_DONTCARE         0x00
#define FF_ROMAN            0x10
#define FF_SWISS            0x20
#define FF_MODERN           0x30
#define FF_SCRIPT           0x40
#define FF_DECORATIVE       0x50

typedef struct
{
    INT16     tmHeight;
    INT16     tmAscent;
    INT16     tmDescent;
    INT16     tmInternalLeading;
    INT16     tmExternalLeading;
    INT16     tmAveCharWidth;
    INT16     tmMaxCharWidth;
    INT16     tmWeight;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    INT16     tmOverhang WINE_PACKED;
    INT16     tmDigitizedAspectX WINE_PACKED;
    INT16     tmDigitizedAspectY WINE_PACKED;
} TEXTMETRIC16, *LPTEXTMETRIC16;

typedef struct
{
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRIC32A, *LPTEXTMETRIC32A;

typedef struct
{
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
} TEXTMETRIC32W, *LPTEXTMETRIC32W;

DECL_WINELIB_TYPE_AW(TEXTMETRIC);
DECL_WINELIB_TYPE_AW(LPTEXTMETRIC);

/* ntmFlags field flags */
#define NTM_REGULAR     0x00000040L
#define NTM_BOLD        0x00000020L
#define NTM_ITALIC      0x00000001L

typedef struct
{
    INT16     tmHeight;
    INT16     tmAscent;
    INT16     tmDescent;
    INT16     tmInternalLeading;
    INT16     tmExternalLeading;
    INT16     tmAveCharWidth;
    INT16     tmMaxCharWidth;
    INT16     tmWeight;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    INT16     tmOverhang WINE_PACKED;
    INT16     tmDigitizedAspectX WINE_PACKED;
    INT16     tmDigitizedAspectY WINE_PACKED;
    DWORD     ntmFlags;
    UINT16    ntmSizeEM;
    UINT16    ntmCellHeight;
    UINT16    ntmAvgWidth;
} NEWTEXTMETRIC16,*LPNEWTEXTMETRIC16;

typedef struct
{
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
    BYTE      tmFirstChar;
    BYTE      tmLastChar;
    BYTE      tmDefaultChar;
    BYTE      tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    DWORD     ntmFlags;
    UINT32    ntmSizeEM;
    UINT32    ntmCellHeight;
    UINT32    ntmAvgWidth;
} NEWTEXTMETRIC32A, *LPNEWTEXTMETRIC32A;

typedef struct
{
    INT32     tmHeight;
    INT32     tmAscent;
    INT32     tmDescent;
    INT32     tmInternalLeading;
    INT32     tmExternalLeading;
    INT32     tmAveCharWidth;
    INT32     tmMaxCharWidth;
    INT32     tmWeight;
    INT32     tmOverhang;
    INT32     tmDigitizedAspectX;
    INT32     tmDigitizedAspectY;
    WCHAR     tmFirstChar;
    WCHAR     tmLastChar;
    WCHAR     tmDefaultChar;
    WCHAR     tmBreakChar;
    BYTE      tmItalic;
    BYTE      tmUnderlined;
    BYTE      tmStruckOut;
    BYTE      tmPitchAndFamily;
    BYTE      tmCharSet;
    DWORD     ntmFlags;
    UINT32    ntmSizeEM;
    UINT32    ntmCellHeight;
    UINT32    ntmAvgWidth;
} NEWTEXTMETRIC32W, *LPNEWTEXTMETRIC32W;

DECL_WINELIB_TYPE_AW(NEWTEXTMETRIC);
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRIC);

typedef struct
{
    NEWTEXTMETRIC16	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX16,*LPNEWTEXTMETRICEX16;

typedef struct
{
    NEWTEXTMETRIC32A	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX32A,*LPNEWTEXTMETRICEX32A;

typedef struct
{
    NEWTEXTMETRIC32W	ntmetm;
    FONTSIGNATURE       ntmeFontSignature;
} NEWTEXTMETRICEX32W,*LPNEWTEXTMETRICEX32W;

DECL_WINELIB_TYPE_AW(NEWTEXTMETRICEX);
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRICEX);


typedef INT16 (*FONTENUMPROC16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT32 (*FONTENUMPROC32A)(LPENUMLOGFONT32A,LPNEWTEXTMETRIC32A,UINT32,LPARAM);
typedef INT32 (*FONTENUMPROC32W)(LPENUMLOGFONT32W,LPNEWTEXTMETRIC32W,UINT32,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROC);

typedef INT16 (*FONTENUMPROCEX16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT32 (*FONTENUMPROCEX32A)(LPENUMLOGFONTEX32A,LPNEWTEXTMETRICEX32A,UINT32,LPARAM);
typedef INT32 (*FONTENUMPROCEX32W)(LPENUMLOGFONTEX32W,LPNEWTEXTMETRICEX32W,UINT32,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROCEX);

  /* tmPitchAndFamily values */
#define TMPF_FIXED_PITCH    1
#define TMPF_VECTOR	    2
#define TMPF_TRUETYPE	    4
#define TMPF_DEVICE	    8

  /* Text alignment */
#define TA_NOUPDATECP       0x00
#define TA_UPDATECP         0x01
#define TA_LEFT             0x00
#define TA_RIGHT            0x02
#define TA_CENTER           0x06
#define TA_TOP              0x00
#define TA_BOTTOM           0x08
#define TA_BASELINE         0x18

  /* ExtTextOut() parameters */
#define ETO_GRAYED          0x01
#define ETO_OPAQUE          0x02
#define ETO_CLIPPED         0x04

  /* for GetCharABCWidths() */
typedef struct
{
    INT16   abcA;
    UINT16  abcB;
    INT16   abcC;
} ABC16, *LPABC16;

typedef struct
{
    INT32   abcA;
    UINT32  abcB;
    INT32   abcC;
} ABC32, *LPABC32;

DECL_WINELIB_TYPE(ABC);
DECL_WINELIB_TYPE(LPABC);

  /* Rasterizer status */
typedef struct
{
    WORD nSize;
    WORD wFlags;
    WORD nLanguageID;
} RASTERIZER_STATUS, *LPRASTERIZER_STATUS;

#define TT_AVAILABLE        0x0001
#define TT_ENABLED          0x0002

/* Get/SetSystemPaletteUse() values */
#define SYSPAL_STATIC   1
#define SYSPAL_NOSTATIC 2

typedef struct tagPALETTEENTRY
{
	BYTE peRed, peGreen, peBlue, peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

/* Logical palette entry flags */
#define PC_RESERVED     0x01
#define PC_EXPLICIT     0x02
#define PC_NOCOLLAPSE   0x04

typedef struct
{ 
    WORD           palVersion;
    WORD           palNumEntries;
    PALETTEENTRY   palPalEntry[1] WINE_PACKED;
} LOGPALETTE, *LPLOGPALETTE;

  /* Pens */

typedef struct
{
    UINT16   lopnStyle; 
    POINT16  lopnWidth WINE_PACKED;
    COLORREF lopnColor WINE_PACKED;
} LOGPEN16, *LPLOGPEN16;

typedef struct
{
    UINT32   lopnStyle; 
    POINT32  lopnWidth WINE_PACKED;
    COLORREF lopnColor WINE_PACKED;
} LOGPEN32, *LPLOGPEN32;

DECL_WINELIB_TYPE(LOGPEN);
DECL_WINELIB_TYPE(LPLOGPEN);

#define PS_SOLID	  0
#define PS_DASH           1
#define PS_DOT            2
#define PS_DASHDOT        3
#define PS_DASHDOTDOT     4
#define PS_NULL 	  5
#define PS_INSIDEFRAME 	  6

  /* Regions */

#define ERROR             0
#define NULLREGION        1
#define SIMPLEREGION      2
#define COMPLEXREGION     3

#define RGN_AND           1
#define RGN_OR            2
#define RGN_XOR           3
#define RGN_DIFF          4
#define RGN_COPY          5

  /* Device contexts */

/* GetDCEx flags */
#define DCX_WINDOW           0x00000001
#define DCX_CACHE            0x00000002
#define DCX_CLIPCHILDREN     0x00000008
#define DCX_CLIPSIBLINGS     0x00000010
#define DCX_PARENTCLIP       0x00000020
#define DCX_EXCLUDERGN       0x00000040
#define DCX_INTERSECTRGN     0x00000080
#define DCX_LOCKWINDOWUPDATE 0x00000400
#define DCX_USESTYLE         0x00010000

  /* Polygon modes */
#define ALTERNATE         1
#define WINDING           2

  /* Background modes */
#ifdef TRANSPARENT  /*Apparently some broken svr4 includes define TRANSPARENT*/
#undef TRANSPARENT
#endif
#define TRANSPARENT       1
#define OPAQUE            2

  /* Map modes */
#define MM_TEXT		  1
#define MM_LOMETRIC	  2
#define MM_HIMETRIC	  3
#define MM_LOENGLISH	  4
#define MM_HIENGLISH	  5
#define MM_TWIPS	  6
#define MM_ISOTROPIC	  7
#define MM_ANISOTROPIC	  8

  /* Coordinate modes */
#define ABSOLUTE          1
#define RELATIVE          2

  /* Flood fill modes */
#define FLOODFILLBORDER   0
#define FLOODFILLSURFACE  1

  /* Device parameters for GetDeviceCaps() */
#define DRIVERVERSION     0
#define TECHNOLOGY        2
#define HORZSIZE          4
#define VERTSIZE          6
#define HORZRES           8
#define VERTRES           10
#define BITSPIXEL         12
#define PLANES            14
#define NUMBRUSHES        16
#define NUMPENS           18
#define NUMMARKERS        20
#define NUMFONTS          22
#define NUMCOLORS         24
#define PDEVICESIZE       26
#define CURVECAPS         28
#define LINECAPS          30
#define POLYGONALCAPS     32
#define TEXTCAPS          34
#define CLIPCAPS          36
#define RASTERCAPS        38
#define ASPECTX           40
#define ASPECTY           42
#define ASPECTXY          44
#define LOGPIXELSX        88
#define LOGPIXELSY        90
#define SIZEPALETTE       104
#define NUMRESERVED       106
#define COLORRES          108

/* TECHNOLOGY */
#define DT_PLOTTER        0
#define DT_RASDISPLAY     1
#define DT_RASPRINTER     2
#define DT_RASCAMERA      3
#define DT_CHARSTREAM     4
#define DT_METAFILE       5
#define DT_DISPFILE       6

/* CURVECAPS */
#define CC_NONE           0x0000
#define CC_CIRCLES        0x0001
#define CC_PIE            0x0002
#define CC_CHORD          0x0004
#define CC_ELLIPSES       0x0008
#define CC_WIDE           0x0010
#define CC_STYLED         0x0020
#define CC_WIDESTYLED     0x0040
#define CC_INTERIORS      0x0080
#define CC_ROUNDRECT      0x0100

/* LINECAPS */
#define LC_NONE           0x0000
#define LC_POLYLINE       0x0002
#define LC_MARKER         0x0004
#define LC_POLYMARKER     0x0008
#define LC_WIDE           0x0010
#define LC_STYLED         0x0020
#define LC_WIDESTYLED     0x0040
#define LC_INTERIORS      0x0080

/* POLYGONALCAPS */
#define PC_NONE           0x0000
#define PC_POLYGON        0x0001
#define PC_RECTANGLE      0x0002
#define PC_WINDPOLYGON    0x0004
#define PC_SCANLINE       0x0008
#define PC_WIDE           0x0010
#define PC_STYLED         0x0020
#define PC_WIDESTYLED     0x0040
#define PC_INTERIORS      0x0080

/* TEXTCAPS */
#define TC_OP_CHARACTER   0x0001
#define TC_OP_STROKE      0x0002
#define TC_CP_STROKE      0x0004
#define TC_CR_90          0x0008
#define TC_CR_ANY         0x0010
#define TC_SF_X_YINDEP    0x0020
#define TC_SA_DOUBLE      0x0040
#define TC_SA_INTEGER     0x0080
#define TC_SA_CONTIN      0x0100
#define TC_EA_DOUBLE      0x0200
#define TC_IA_ABLE        0x0400
#define TC_UA_ABLE        0x0800
#define TC_SO_ABLE        0x1000
#define TC_RA_ABLE        0x2000
#define TC_VA_ABLE        0x4000
#define TC_RESERVED       0x8000

/* CLIPCAPS */
#define CP_NONE           0x0000
#define CP_RECTANGLE      0x0001
#define CP_REGION         0x0002

/* RASTERCAPS */
#define RC_NONE           0x0000
#define RC_BITBLT         0x0001
#define RC_BANDING        0x0002
#define RC_SCALING        0x0004
#define RC_BITMAP64       0x0008
#define RC_GDI20_OUTPUT   0x0010
#define RC_GDI20_STATE    0x0020
#define RC_SAVEBITMAP     0x0040
#define RC_DI_BITMAP      0x0080
#define RC_PALETTE        0x0100
#define RC_DIBTODEV       0x0200
#define RC_BIGFONT        0x0400
#define RC_STRETCHBLT     0x0800
#define RC_FLOODFILL      0x1000
#define RC_STRETCHDIB     0x2000
#define RC_OP_DX_OUTPUT   0x4000
#define RC_DEVBITS        0x8000

  /* GetSystemMetrics() codes */
#define SM_CXSCREEN	       0
#define SM_CYSCREEN            1
#define SM_CXVSCROLL           2
#define SM_CYHSCROLL	       3
#define SM_CYCAPTION	       4
#define SM_CXBORDER	       5
#define SM_CYBORDER	       6
#define SM_CXDLGFRAME	       7
#define SM_CYDLGFRAME	       8
#define SM_CYVTHUMB	       9
#define SM_CXHTHUMB	      10
#define SM_CXICON	      11
#define SM_CYICON	      12
#define SM_CXCURSOR	      13
#define SM_CYCURSOR	      14
#define SM_CYMENU	      15
#define SM_CXFULLSCREEN       16
#define SM_CYFULLSCREEN       17
#define SM_CYKANJIWINDOW      18
#define SM_MOUSEPRESENT       19
#define SM_CYVSCROLL	      20
#define SM_CXHSCROLL	      21
#define SM_DEBUG	      22
#define SM_SWAPBUTTON	      23
#define SM_RESERVED1	      24
#define SM_RESERVED2	      25
#define SM_RESERVED3	      26
#define SM_RESERVED4	      27
#define SM_CXMIN	      28
#define SM_CYMIN	      29
#define SM_CXSIZE	      30
#define SM_CYSIZE	      31
#define SM_CXFRAME	      32
#define SM_CYFRAME	      33
#define SM_CXMINTRACK	      34
#define SM_CYMINTRACK	      35
#define SM_CXDOUBLECLK        36
#define SM_CYDOUBLECLK        37
#define SM_CXICONSPACING      38
#define SM_CYICONSPACING      39
#define SM_MENUDROPALIGNMENT  40
#define SM_PENWINDOWS         41
#define SM_DBCSENABLED        42

#define SM_CMETRICS           43

  /* Device-independent bitmaps */

typedef struct { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct { BYTE rgbtBlue, rgbtGreen, rgbtRed; } RGBTRIPLE;

typedef struct
{
    UINT16  bfType;
    DWORD   bfSize WINE_PACKED;
    UINT16  bfReserved1 WINE_PACKED;
    UINT16  bfReserved2 WINE_PACKED;
    DWORD   bfOffBits WINE_PACKED;
} BITMAPFILEHEADER;

typedef struct
{
    DWORD 	biSize;
    LONG  	biWidth;
    LONG  	biHeight;
    WORD 	biPlanes;
    WORD 	biBitCount;
    DWORD 	biCompression;
    DWORD 	biSizeImage;
    LONG  	biXPelsPerMeter;
    LONG  	biYPelsPerMeter;
    DWORD 	biClrUsed;
    DWORD 	biClrImportant;
} BITMAPINFOHEADER, *LPBITMAPINFOHEADER;

  /* biCompression */
#define BI_RGB           0
#define BI_RLE8          1
#define BI_RLE4          2

typedef struct {
	BITMAPINFOHEADER bmiHeader;
	RGBQUAD	bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO *LPBITMAPINFO;
typedef BITMAPINFO *NPBITMAPINFO;
typedef BITMAPINFO *PBITMAPINFO;

typedef struct
{
    DWORD   bcSize;
    UINT16  bcWidth;
    UINT16  bcHeight;
    UINT16  bcPlanes;
    UINT16  bcBitCount;
} BITMAPCOREHEADER;

typedef struct
{
    BITMAPCOREHEADER bmciHeader;
    RGBTRIPLE        bmciColors[1];
} BITMAPCOREINFO, *LPBITMAPCOREINFO;

#define DIB_RGB_COLORS   0
#define DIB_PAL_COLORS   1
#define CBM_INIT         4


  /* Cursors / Icons */

typedef struct
{
    POINT16 ptHotSpot;
    WORD    nWidth;
    WORD    nHeight;
    WORD    nWidthBytes;
    BYTE    bPlanes;
    BYTE    bBitsPerPixel;
} CURSORICONINFO;



typedef struct {
	BYTE i;  /* much more .... */
} KANJISTRUCT;
typedef KANJISTRUCT *LPKANJISTRUCT;
typedef KANJISTRUCT *NPKANJISTRUCT;
typedef KANJISTRUCT *PKANJISTRUCT;

#define OFS_MAXPATHNAME 128
typedef struct
{
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    BYTE reserved[4];
    BYTE szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT;

#define OF_READ               0x0000
#define OF_WRITE              0x0001
#define OF_READWRITE          0x0002
#define OF_SHARE_COMPAT       0x0000
#define OF_SHARE_EXCLUSIVE    0x0010
#define OF_SHARE_DENY_WRITE   0x0020
#define OF_SHARE_DENY_READ    0x0030
#define OF_SHARE_DENY_NONE    0x0040
#define OF_PARSE              0x0100
#define OF_DELETE             0x0200
#define OF_VERIFY             0x0400   /* Used with OF_REOPEN */
#define OF_SEARCH             0x0400   /* Used without OF_REOPEN */
#define OF_CANCEL             0x0800
#define OF_CREATE             0x1000
#define OF_PROMPT             0x2000
#define OF_EXIST              0x4000
#define OF_REOPEN             0x8000

/* GetTempFileName() Flags */
#define TF_FORCEDRIVE	        0x80

#define DRIVE_CANNOTDETERMINE      0
#define DRIVE_DOESNOTEXIST         1
#define DRIVE_REMOVABLE            2
#define DRIVE_FIXED                3
#define DRIVE_REMOTE               4
/* Win32 additions */
#define DRIVE_CDROM                5
#define DRIVE_RAMDISK              6

#define HFILE_ERROR	-1

#define DDL_READWRITE	0x0000
#define DDL_READONLY	0x0001
#define DDL_HIDDEN	0x0002
#define DDL_SYSTEM	0x0004
#define DDL_DIRECTORY	0x0010
#define DDL_ARCHIVE	0x0020

#define DDL_POSTMSGS	0x2000
#define DDL_DRIVES	0x4000
#define DDL_EXCLUSIVE	0x8000

/* The security attributes structure 
 */
typedef struct
{
    DWORD   nLength;
    LPVOID  lpSecurityDescriptor;
    BOOL32  bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
/* FIXME: currently dwLowDateTime is equivalent to the UNIX time(),
 * and dwHighDateTime 0
 */
typedef struct
{
  INT32  dwLowDateTime;
  INT32  dwHighDateTime;
} FILETIME, *LPFILETIME;

/* Find* structures */
typedef struct
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    CHAR      cFileName[260];
    CHAR      cAlternateFileName[14];
} WIN32_FIND_DATA32A, *LPWIN32_FIND_DATA32A;

typedef struct
{
    DWORD     dwFileAttributes;
    FILETIME  ftCreationTime;
    FILETIME  ftLastAccessTime;
    FILETIME  ftLastWriteTime;
    DWORD     nFileSizeHigh;
    DWORD     nFileSizeLow;
    DWORD     dwReserved0;
    DWORD     dwReserved1;
    WCHAR     cFileName[260];
    WCHAR     cAlternateFileName[14];
} WIN32_FIND_DATA32W, *LPWIN32_FIND_DATA32W;

DECL_WINELIB_TYPE_AW(WIN32_FIND_DATA);
DECL_WINELIB_TYPE_AW(LPWIN32_FIND_DATA);

/* comm */

#define CBR_110	0xFF10
#define CBR_300	0xFF11
#define CBR_600	0xFF12
#define CBR_1200	0xFF13
#define CBR_2400	0xFF14
#define CBR_4800	0xFF15
#define CBR_9600	0xFF16
#define CBR_14400	0xFF17
#define CBR_19200	0xFF18
#define CBR_38400	0xFF1B
#define CBR_56000	0xFF1F
#define CBR_128000	0xFF23
#define CBR_256000	0xFF27

#define NOPARITY	0
#define ODDPARITY	1
#define EVENPARITY	2
#define MARKPARITY	3
#define SPACEPARITY	4
#define ONESTOPBIT	0
#define ONE5STOPBITS	1
#define TWOSTOPBITS	2

#define IGNORE		0
#define INFINITE16      0xFFFF
#define INFINITE32      0xFFFFFFFF
#define INFINITE WINELIB_NAME(INFINITE)

#define CE_RXOVER	0x0001
#define CE_OVERRUN	0x0002
#define CE_RXPARITY	0x0004
#define CE_FRAME	0x0008
#define CE_BREAK	0x0010
#define CE_CTSTO	0x0020
#define CE_DSRTO	0x0040
#define CE_RLSDTO	0x0080
#define CE_TXFULL	0x0100
#define CE_PTO		0x0200
#define CE_IOE		0x0400
#define CE_DNS		0x0800
#define CE_OOP		0x1000
#define CE_MODE	0x8000

#define IE_BADID	-1
#define IE_OPEN	-2
#define IE_NOPEN	-3
#define IE_MEMORY	-4
#define IE_DEFAULT	-5
#define IE_HARDWARE	-10
#define IE_BYTESIZE	-11
#define IE_BAUDRATE	-12

#define EV_RXCHAR	0x0001
#define EV_RXFLAG	0x0002
#define EV_TXEMPTY	0x0004
#define EV_CTS		0x0008
#define EV_DSR		0x0010
#define EV_RLSD	0x0020
#define EV_BREAK	0x0040
#define EV_ERR		0x0080
#define EV_RING	0x0100
#define EV_PERR	0x0200
#define EV_CTSS	0x0400
#define EV_DSRS	0x0800
#define EV_RLSDS	0x1000
#define EV_RINGTE	0x2000
#define EV_RingTe	EV_RINGTE

#define SETXOFF	1
#define SETXON		2
#define SETRTS		3
#define CLRRTS		4
#define SETDTR		5
#define CLRDTR		6
#define RESETDEV	7
/* win16 only */
#define GETMAXLPT	8
#define GETMAXCOM	9
/* win32 only */
#define SETBREAK	8
#define CLRBREAK	9

#define GETBASEIRQ	10

#define CN_RECEIVE	0x0001
#define CN_TRANSMIT	0x0002
#define CN_EVENT	0x0004

typedef struct tagDCB16
{
    BYTE   Id;
    UINT16 BaudRate WINE_PACKED;
    BYTE   ByteSize;
    BYTE   Parity;
    BYTE   StopBits;
    UINT16 RlsTimeout;
    UINT16 CtsTimeout;
    UINT16 DsrTimeout;

    UINT16 fBinary        :1;
    UINT16 fRtsDisable    :1;
    UINT16 fParity        :1;
    UINT16 fOutxCtsFlow   :1;
    UINT16 fOutxDsrFlow   :1;
    UINT16 fDummy         :2;
    UINT16 fDtrDisable    :1;

    UINT16 fOutX          :1;
    UINT16 fInX           :1;
    UINT16 fPeChar        :1;
    UINT16 fNull          :1;
    UINT16 fChEvt         :1;
    UINT16 fDtrflow       :1;
    UINT16 fRtsflow       :1;
    UINT16 fDummy2        :1;

    CHAR   XonChar;
    CHAR   XoffChar;
    UINT16 XonLim;
    UINT16 XoffLim;
    CHAR   PeChar;
    CHAR   EofChar;
    CHAR   EvtChar;
    UINT16 TxDelay WINE_PACKED;
} DCB16, *LPDCB16;

typedef struct tagDCB32
{
    DWORD DCBlength;
    DWORD BaudRate;
    DWORD fBinary		:1;
    DWORD fParity		:1;
    DWORD fOutxCtsFlow		:1;
    DWORD fOutxDsrFlow		:1;
    DWORD fDtrControl		:2;
    DWORD fDsrSensitivity	:1;
    DWORD fTXContinueOnXoff	:1;
    DWORD fOutX			:1;
    DWORD fInX			:1;
    DWORD fErrorChar		:1;
    DWORD fNull			:1;
    DWORD fRtsControl		:2;
    DWORD fAbortOnError		:1;
    DWORD fDummy2		:17;
    WORD wReserved;
    WORD XonLim;
    WORD XoffLim;
    BYTE ByteSize;
    BYTE Parity;
    BYTE StopBits;
    char XonChar;
    char XoffChar;
    char ErrorChar;
    char EofChar;
    char EvtChar;
} DCB32, *LPDCB32;

DECL_WINELIB_TYPE(DCB);
DECL_WINELIB_TYPE(LPDCB);

#define	RTS_CONTROL_DISABLE	0
#define	RTS_CONTROL_ENABLE	1
#define	RTS_CONTROL_HANDSHAKE	2
#define	RTS_CONTROL_TOGGLE	3

#define	DTR_CONTROL_DISABLE	0
#define	DTR_CONTROL_ENABLE	1
#define	DTR_CONTROL_HANDSHAKE	2

typedef struct tagCOMMTIMEOUTS {
	DWORD	ReadIntervalTimeout;
	DWORD	ReadTotalTimeoutMultiplier;
	DWORD	ReadTotalTimeoutConstant;
	DWORD	WriteTotalTimeoutMultiplier;
	DWORD	WriteTotalTimeoutConstant;
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;

typedef struct tagCOMSTAT
{
    BYTE   status;
    UINT16 cbInQue WINE_PACKED;
    UINT16 cbOutQue WINE_PACKED;
} COMSTAT,*LPCOMSTAT;

#define CSTF_CTSHOLD	0x01
#define CSTF_DSRHOLD	0x02
#define CSTF_RLSDHOLD	0x04
#define CSTF_XOFFHOLD	0x08
#define CSTF_XOFFSENT	0x10
#define CSTF_EOF	0x20
#define CSTF_TXIM	0x40

/* SystemParametersInfo */

#define SPI_GETBEEP               1
#define SPI_SETBEEP               2
#define SPI_GETMOUSE              3
#define SPI_SETMOUSE              4
#define SPI_GETBORDER             5
#define SPI_SETBORDER             6
#define SPI_GETKEYBOARDSPEED      10
#define SPI_SETKEYBOARDSPEED      11
#define SPI_LANGDRIVER            12
#define SPI_ICONHORIZONTALSPACING 13
#define SPI_GETSCREENSAVETIMEOUT  14
#define SPI_SETSCREENSAVETIMEOUT  15
#define SPI_GETSCREENSAVEACTIVE   16
#define SPI_SETSCREENSAVEACTIVE   17
#define SPI_GETGRIDGRANULARITY    18
#define SPI_SETGRIDGRANULARITY    19
#define SPI_SETDESKWALLPAPER      20
#define SPI_SETDESKPATTERN        21
#define SPI_GETKEYBOARDDELAY      22
#define SPI_SETKEYBOARDDELAY      23
#define SPI_ICONVERTICALSPACING   24
#define SPI_GETICONTITLEWRAP      25
#define SPI_SETICONTITLEWRAP      26
#define SPI_GETMENUDROPALIGNMENT  27
#define SPI_SETMENUDROPALIGNMENT  28
#define SPI_SETDOUBLECLKWIDTH     29
#define SPI_SETDOUBLECLKHEIGHT    30
#define SPI_GETICONTITLELOGFONT   31
#define SPI_SETDOUBLECLICKTIME    32
#define SPI_SETMOUSEBUTTONSWAP    33
#define SPI_SETICONTITLELOGFONT   34
#define SPI_GETFASTTASKSWITCH     35
#define SPI_SETFASTTASKSWITCH     36
#define SPI_SETDRAGFULLWINDOWS    37
#define SPI_GETDRAGFULLWINDOWS    38
#define SPI_GETNONCLIENTMETRICS   41
#define SPI_SETNONCLIENTMETRICS   42
#define SPI_GETMINIMIZEDMETRICS   43
#define SPI_SETMINIMIZEDMETRICS   44
#define SPI_GETICONMETRICS        45
#define SPI_SETICONMETRICS        46
#define SPI_SETWORKAREA           47
#define SPI_GETWORKAREA           48
#define SPI_SETPENWINDOWS         49
#define SPI_GETFILTERKEYS         50
#define SPI_SETFILTERKEYS         51
#define SPI_GETTOGGLEKEYS         52
#define SPI_SETTOGGLEKEYS         53
#define SPI_GETMOUSEKEYS          54
#define SPI_SETMOUSEKEYS          55
#define SPI_GETSHOWSOUNDS         56
#define SPI_SETSHOWSOUNDS         57
#define SPI_GETSTICKYKEYS         58
#define SPI_SETSTICKYKEYS         59
#define SPI_GETACCESSTIMEOUT      60
#define SPI_SETACCESSTIMEOUT      61
#define SPI_GETSERIALKEYS         62
#define SPI_SETSERIALKEYS         63
#define SPI_GETSOUNDSENTRY        64
#define SPI_SETSOUNDSENTRY        65
#define SPI_GETHIGHCONTRAST       66
#define SPI_SETHIGHCONTRAST       67
#define SPI_GETKEYBOARDPREF       68
#define SPI_SETKEYBOARDPREF       69
#define SPI_GETSCREENREADER       70
#define SPI_SETSCREENREADER       71
#define SPI_GETANIMATION          72
#define SPI_SETANIMATION          73
#define SPI_GETFONTSMOOTHING      74
#define SPI_SETFONTSMOOTHING      75
#define SPI_SETDRAGWIDTH          76
#define SPI_SETDRAGHEIGHT         77
#define SPI_SETHANDHELD           78
#define SPI_GETLOWPOWERTIMEOUT    79
#define SPI_GETPOWEROFFTIMEOUT    80
#define SPI_SETLOWPOWERTIMEOUT    81
#define SPI_SETPOWEROFFTIMEOUT    82
#define SPI_GETLOWPOWERACTIVE     83
#define SPI_GETPOWEROFFACTIVE     84
#define SPI_SETLOWPOWERACTIVE     85
#define SPI_SETPOWEROFFACTIVE     86
#define SPI_SETCURSORS            87
#define SPI_SETICONS              88
#define SPI_GETDEFAULTINPUTLANG   89
#define SPI_SETDEFAULTINPUTLANG   90
#define SPI_SETLANGTOGGLE         91
#define SPI_GETWINDOWSEXTENSION   92
#define SPI_SETMOUSETRAILS        93
#define SPI_GETMOUSETRAILS        94
#define SPI_SCREENSAVERRUNNING    97

/* SystemParametersInfo flags */

#define SPIF_UPDATEINIFILE		1
#define SPIF_SENDWININICHANGE		2

/* GetFreeSystemResources() parameters */

#define GFSR_SYSTEMRESOURCES   0x0000
#define GFSR_GDIRESOURCES      0x0001
#define GFSR_USERRESOURCES     0x0002

/* GetWinFlags */

#define WF_PMODE 	0x0001
#define WF_CPU286 	0x0002
#define	WF_CPU386	0x0004
#define	WF_CPU486 	0x0008
#define	WF_STANDARD	0x0010
#define	WF_WIN286 	0x0010
#define	WF_ENHANCED	0x0020
#define	WF_WIN386	0x0020
#define	WF_CPU086	0x0040
#define	WF_CPU186	0x0080
#define	WF_LARGEFRAME	0x0100
#define	WF_SMALLFRAME	0x0200
#define	WF_80x87	0x0400
#define	WF_PAGING	0x0800
#define	WF_WLO          0x8000

#define MAKEINTRESOURCE(i) (SEGPTR)((DWORD)((WORD)(i)))

/* Predefined resource types */
#define RT_CURSOR	    MAKEINTRESOURCE(1)
#define RT_BITMAP	    MAKEINTRESOURCE(2)
#define RT_ICON 	    MAKEINTRESOURCE(3)
#define RT_MENU 	    MAKEINTRESOURCE(4)
#define RT_DIALOG	    MAKEINTRESOURCE(5)
#define RT_STRING	    MAKEINTRESOURCE(6)
#define RT_FONTDIR	    MAKEINTRESOURCE(7)
#define RT_FONT 	    MAKEINTRESOURCE(8)
#define RT_ACCELERATOR	    MAKEINTRESOURCE(9)
#define RT_RCDATA	    MAKEINTRESOURCE(10)
#define RT_MESSAGELIST      MAKEINTRESOURCE(11)
#define RT_GROUP_CURSOR     MAKEINTRESOURCE(12)
#define RT_GROUP_ICON	    MAKEINTRESOURCE(14)

/* Predefined resources */
#define IDI_APPLICATION  MAKEINTRESOURCE(32512)
#define IDI_HAND         MAKEINTRESOURCE(32513)
#define IDI_QUESTION     MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION  MAKEINTRESOURCE(32515)
#define IDI_ASTERISK     MAKEINTRESOURCE(32516)

#define IDC_BUMMER	 MAKEINTRESOURCE(100)
#define IDC_ARROW        MAKEINTRESOURCE(32512)
#define IDC_IBEAM        MAKEINTRESOURCE(32513)
#define IDC_WAIT         MAKEINTRESOURCE(32514)
#define IDC_CROSS        MAKEINTRESOURCE(32515)
#define IDC_UPARROW      MAKEINTRESOURCE(32516)
#define IDC_SIZE         MAKEINTRESOURCE(32640)
#define IDC_ICON         MAKEINTRESOURCE(32641)
#define IDC_SIZENWSE     MAKEINTRESOURCE(32642)
#define IDC_SIZENESW     MAKEINTRESOURCE(32643)
#define IDC_SIZEWE       MAKEINTRESOURCE(32644)
#define IDC_SIZENS       MAKEINTRESOURCE(32645)

/* OEM Resource Ordinal Numbers */
#define OBM_CLOSE           32754
#define OBM_UPARROW         32753
#define OBM_DNARROW         32752
#define OBM_RGARROW         32751
#define OBM_LFARROW         32750
#define OBM_REDUCE          32749
#define OBM_ZOOM            32748
#define OBM_RESTORE         32747
#define OBM_REDUCED         32746
#define OBM_ZOOMD           32745
#define OBM_RESTORED        32744
#define OBM_UPARROWD        32743
#define OBM_DNARROWD        32742
#define OBM_RGARROWD        32741
#define OBM_LFARROWD        32740
#define OBM_MNARROW         32739
#define OBM_COMBO           32738
#define OBM_UPARROWI        32737
#define OBM_DNARROWI        32736
#define OBM_RGARROWI        32735
#define OBM_LFARROWI        32734

#define OBM_FOLDER          32733
#define OBM_FOLDER2         32732
#define OBM_FLOPPY          32731
#define OBM_HDISK           32730
#define OBM_CDROM           32729
#define OBM_TRTYPE          32728

#define OBM_OLD_CLOSE       32767
#define OBM_SIZE            32766
#define OBM_OLD_UPARROW     32765
#define OBM_OLD_DNARROW     32764
#define OBM_OLD_RGARROW     32763
#define OBM_OLD_LFARROW     32762
#define OBM_BTSIZE          32761
#define OBM_CHECK           32760
#define OBM_CHECKBOXES      32759
#define OBM_BTNCORNERS      32758
#define OBM_OLD_REDUCE      32757
#define OBM_OLD_ZOOM        32756
#define OBM_OLD_RESTORE     32755

#define OCR_BUMMER	    100
#define OCR_DRAGOBJECT	    101

#define OCR_NORMAL          32512
#define OCR_IBEAM           32513
#define OCR_WAIT            32514
#define OCR_CROSS           32515
#define OCR_UP              32516
#define OCR_SIZE            32640
#define OCR_ICON            32641
#define OCR_SIZENWSE        32642
#define OCR_SIZENESW        32643
#define OCR_SIZEWE          32644
#define OCR_SIZENS          32645
#define OCR_SIZEALL         32646
#define OCR_ICOCUR          32647

#define OIC_SAMPLE          32512
#define OIC_HAND            32513
#define OIC_QUES            32514
#define OIC_BANG            32515
#define OIC_NOTE            32516
#define OIC_PORTRAIT        32517
#define OIC_LANDSCAPE       32518
#define OIC_WINEICON        32519

  /* Stock GDI objects for GetStockObject() */

#define WHITE_BRUSH	    0
#define LTGRAY_BRUSH	    1
#define GRAY_BRUSH	    2
#define DKGRAY_BRUSH	    3
#define BLACK_BRUSH	    4
#define NULL_BRUSH	    5
#define HOLLOW_BRUSH	    5
#define WHITE_PEN	    6
#define BLACK_PEN	    7
#define NULL_PEN	    8
#define OEM_FIXED_FONT	    10
#define ANSI_FIXED_FONT     11
#define ANSI_VAR_FONT	    12
#define SYSTEM_FONT	    13
#define DEVICE_DEFAULT_FONT 14
#define DEFAULT_PALETTE     15
#define SYSTEM_FIXED_FONT   16

/* DragObject stuff */

typedef struct
{
    HWND16     hWnd;
    HANDLE16   hScope;
    WORD       wFlags;
    HANDLE16   hList;
    HANDLE16   hOfStruct;
    POINT16 pt WINE_PACKED;
    LONG       l WINE_PACKED;
} DRAGINFO, *LPDRAGINFO;

#define DRAGOBJ_PROGRAM		0x0001
#define DRAGOBJ_DATA		0x0002
#define DRAGOBJ_DIRECTORY	0x0004
#define DRAGOBJ_MULTIPLE	0x0008
#define DRAGOBJ_EXTERNAL	0x8000

#define DRAG_PRINT		0x544E5250
#define DRAG_FILE		0x454C4946

/* Messages */

#define WM_NULL                 0x0000
#define WM_CREATE               0x0001
#define WM_DESTROY              0x0002
#define WM_MOVE                 0x0003
#define WM_SIZEWAIT             0x0004
#define WM_SIZE                 0x0005
#define WM_ACTIVATE             0x0006
#define WM_SETFOCUS             0x0007
#define WM_KILLFOCUS            0x0008
#define WM_SETVISIBLE           0x0009
#define WM_ENABLE               0x000a
#define WM_SETREDRAW            0x000b
#define WM_SETTEXT              0x000c
#define WM_GETTEXT              0x000d
#define WM_GETTEXTLENGTH        0x000e
#define WM_PAINT                0x000f
#define WM_CLOSE                0x0010
#define WM_QUERYENDSESSION      0x0011
#define WM_QUIT                 0x0012
#define WM_QUERYOPEN            0x0013
#define WM_ERASEBKGND           0x0014
#define WM_SYSCOLORCHANGE       0x0015
#define WM_ENDSESSION           0x0016
#define WM_SYSTEMERROR          0x0017
#define WM_SHOWWINDOW           0x0018
#define WM_CTLCOLOR             0x0019
#define WM_WININICHANGE         0x001a
#define WM_DEVMODECHANGE        0x001b
#define WM_ACTIVATEAPP          0x001c
#define WM_FONTCHANGE           0x001d
#define WM_TIMECHANGE           0x001e
#define WM_CANCELMODE           0x001f
#define WM_SETCURSOR            0x0020
#define WM_MOUSEACTIVATE        0x0021
#define WM_CHILDACTIVATE        0x0022
#define WM_QUEUESYNC            0x0023
#define WM_GETMINMAXINFO        0x0024

#define WM_PAINTICON            0x0026
#define WM_ICONERASEBKGND       0x0027
#define WM_NEXTDLGCTL           0x0028
#define WM_ALTTABACTIVE         0x0029
#define WM_SPOOLERSTATUS        0x002a
#define WM_DRAWITEM             0x002b
#define WM_MEASUREITEM          0x002c
#define WM_DELETEITEM           0x002d
#define WM_VKEYTOITEM           0x002e
#define WM_CHARTOITEM           0x002f
#define WM_SETFONT              0x0030
#define WM_GETFONT              0x0031
#define WM_SETHOTKEY            0x0032
#define WM_GETHOTKEY            0x0033
#define WM_FILESYSCHANGE        0x0034
#define WM_ISACTIVEICON         0x0035
#define WM_QUERYPARKICON        0x0036
#define WM_QUERYDRAGICON        0x0037
#define WM_QUERYSAVESTATE       0x0038
#define WM_COMPAREITEM          0x0039
#define WM_TESTING              0x003a

#define WM_OTHERWINDOWCREATED	0x003c
#define WM_OTHERWINDOWDESTROYED	0x003d
#define WM_ACTIVATESHELLWINDOW	0x003e

#define WM_COMPACTING	    0x0041

#define WM_COMMNOTIFY	    0x0044
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED  0x0047
#define WM_POWER	    0x0048

#define WM_NCCREATE         0x0081
#define WM_NCDESTROY        0x0082
#define WM_NCCALCSIZE       0x0083
#define WM_NCHITTEST        0x0084
#define WM_NCPAINT          0x0085
#define WM_NCACTIVATE       0x0086

#define WM_GETDLGCODE	    0x0087
#define WM_SYNCPAINT	    0x0088
#define WM_SYNCTASK	    0x0089

  /* Non-client mouse messages */
#define WM_NCMOUSEMOVE      0x00a0
#define WM_NCLBUTTONDOWN    0x00a1
#define WM_NCLBUTTONUP      0x00a2
#define WM_NCLBUTTONDBLCLK  0x00a3
#define WM_NCRBUTTONDOWN    0x00a4
#define WM_NCRBUTTONUP      0x00a5
#define WM_NCRBUTTONDBLCLK  0x00a6
#define WM_NCMBUTTONDOWN    0x00a7
#define WM_NCMBUTTONUP      0x00a8
#define WM_NCMBUTTONDBLCLK  0x00a9

  /* Keyboard messages */
#define WM_KEYDOWN          0x0100
#define WM_KEYUP            0x0101
#define WM_CHAR             0x0102
#define WM_DEADCHAR         0x0103
#define WM_SYSKEYDOWN       0x0104
#define WM_SYSKEYUP         0x0105
#define WM_SYSCHAR          0x0106
#define WM_SYSDEADCHAR      0x0107
#define WM_KEYFIRST         WM_KEYDOWN
#define WM_KEYLAST          0x0108

#define WM_INITDIALOG       0x0110 
#define WM_COMMAND          0x0111
#define WM_SYSCOMMAND       0x0112
#define WM_TIMER	    0x0113
#define WM_SYSTIMER	    0x0118

  /* scroll messages */
#define WM_HSCROLL          0x0114
#define WM_VSCROLL          0x0115

/* Menu messages */
#define WM_INITMENU         0x0116
#define WM_INITMENUPOPUP    0x0117

#define WM_MENUSELECT       0x011F
#define WM_MENUCHAR         0x0120
#define WM_ENTERIDLE        0x0121

#define WM_LBTRACKPOINT     0x0131

  /* Win32 CTLCOLOR messages */
#define WM_CTLCOLORMSGBOX    0x0132
#define WM_CTLCOLOREDIT      0x0133
#define WM_CTLCOLORLISTBOX   0x0134
#define WM_CTLCOLORBTN       0x0135
#define WM_CTLCOLORDLG       0x0136
#define WM_CTLCOLORSCROLLBAR 0x0137
#define WM_CTLCOLORSTATIC    0x0138

  /* Mouse messages */
#define WM_MOUSEMOVE	    0x0200
#define WM_LBUTTONDOWN	    0x0201
#define WM_LBUTTONUP	    0x0202
#define WM_LBUTTONDBLCLK    0x0203
#define WM_RBUTTONDOWN	    0x0204
#define WM_RBUTTONUP	    0x0205
#define WM_RBUTTONDBLCLK    0x0206
#define WM_MBUTTONDOWN	    0x0207
#define WM_MBUTTONUP	    0x0208
#define WM_MBUTTONDBLCLK    0x0209
#define WM_MOUSEFIRST	    WM_MOUSEMOVE
#define WM_MOUSELAST	    WM_MBUTTONDBLCLK

#define WM_PARENTNOTIFY     0x0210
#define WM_ENTERMENULOOP    0x0211
#define WM_EXITMENULOOP     0x0212
#define WM_NEXTMENU	    0x0213

#define WM_MDICREATE	    0x0220
#define WM_MDIDESTROY	    0x0221
#define WM_MDIACTIVATE	    0x0222
#define WM_MDIRESTORE	    0x0223
#define WM_MDINEXT	    0x0224
#define WM_MDIMAXIMIZE	    0x0225
#define WM_MDITILE	    0x0226
#define WM_MDICASCADE	    0x0227
#define WM_MDIICONARRANGE   0x0228
#define WM_MDIGETACTIVE     0x0229
#define WM_DROPOBJECT	    0x022A
#define WM_QUERYDROPOBJECT  0x022B
#define WM_BEGINDRAG	    0x022C
#define WM_DRAGLOOP	    0x022D
#define WM_DRAGSELECT	    0x022E
#define WM_DRAGMOVE	    0x022F
#define WM_MDISETMENU	    0x0230

#define WM_ENTERSIZEMOVE    0x0231
#define WM_EXITSIZEMOVE     0x0232
#define WM_DROPFILES	    0x0233

#define WM_CUT               0x0300
#define WM_COPY              0x0301
#define WM_PASTE             0x0302
#define WM_CLEAR             0x0303
#define WM_UNDO              0x0304
#define WM_RENDERFORMAT      0x0305
#define WM_RENDERALLFORMATS  0x0306
#define WM_DESTROYCLIPBOARD  0x0307
#define WM_DRAWCLIPBOARD     0x0308
#define WM_PAINTCLIPBOARD    0x0309
#define WM_VSCROLLCLIPBOARD  0x030A
#define WM_SIZECLIPBOARD     0x030B
#define WM_ASKCBFORMATNAME   0x030C
#define WM_CHANGECBCHAIN     0x030D
#define WM_HSCROLLCLIPBOARD  0x030E
#define WM_QUERYNEWPALETTE   0x030F
#define WM_PALETTEISCHANGING 0x0310
#define WM_PALETTECHANGED    0x0311

#define WM_COALESCE_FIRST    0x0390
#define WM_COALESCE_LAST     0x039F

  /* misc messages */
#define WM_NULL             0x0000
#define WM_USER             0x0400
#define WM_CPL_LAUNCH       (WM_USER + 1000)
#define WM_CPL_LAUNCHED     (WM_USER + 1001)

  /* Key status flags for mouse events */
#define MK_LBUTTON	    0x0001
#define MK_RBUTTON	    0x0002
#define MK_SHIFT	    0x0004
#define MK_CONTROL	    0x0008
#define MK_MBUTTON	    0x0010

  /* Mouse_Event flags */
#define ME_MOVE             0x01
#define ME_LDOWN            0x02
#define ME_LUP              0x04
#define ME_RDOWN            0x08
#define ME_RUP              0x10

  /* Queue status flags */
#define QS_KEY		0x0001
#define QS_MOUSEMOVE	0x0002
#define QS_MOUSEBUTTON	0x0004
#define QS_MOUSE	(QS_MOUSEMOVE | QS_MOUSEBUTTON)
#define QS_POSTMESSAGE	0x0008
#define QS_TIMER	0x0010
#define QS_PAINT	0x0020
#define QS_SENDMESSAGE	0x0040
#define QS_ALLINPUT     0x007f

  /* PeekMessage() options */
#define PM_NOREMOVE	0x0000
#define PM_REMOVE	0x0001
#define PM_NOYIELD	0x0002

#define WM_SHOWWINDOW       0x0018

/* WM_SHOWWINDOW wParam codes */
#define SW_PARENTCLOSING    1
#define SW_OTHERMAXIMIZED   2
#define SW_PARENTOPENING    3
#define SW_OTHERRESTORED    4

  /* ShowWindow() codes */
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_NORMAL           1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define SW_MAXIMIZE         3
#define SW_SHOWNOACTIVATE   4
#define SW_SHOW             5
#define SW_MINIMIZE         6
#define SW_SHOWMINNOACTIVE  7
#define SW_SHOWNA           8
#define SW_RESTORE          9
#define SW_SHOWDEFAULT	    10
#define SW_MAX		    10
#define SW_NORMALNA	    0xCC	/* undoc. flag in MinMaximize */

  /* WM_SIZE message wParam values */
#define SIZE_RESTORED        0
#define SIZE_MINIMIZED       1
#define SIZE_MAXIMIZED       2
#define SIZE_MAXSHOW         3
#define SIZE_MAXHIDE         4
#define SIZENORMAL           SIZE_RESTORED
#define SIZEICONIC           SIZE_MINIMIZED
#define SIZEFULLSCREEN       SIZE_MAXIMIZED
#define SIZEZOOMSHOW         SIZE_MAXSHOW
#define SIZEZOOMHIDE         SIZE_MAXHIDE

/* SetWindowPos() and WINDOWPOS flags */
#define SWP_NOSIZE          0x0001
#define SWP_NOMOVE          0x0002
#define SWP_NOZORDER        0x0004
#define SWP_NOREDRAW        0x0008
#define SWP_NOACTIVATE      0x0010
#define SWP_FRAMECHANGED    0x0020  /* The frame changed: send WM_NCCALCSIZE */
#define SWP_SHOWWINDOW      0x0040
#define SWP_HIDEWINDOW      0x0080
#define SWP_NOCOPYBITS      0x0100
#define SWP_NOOWNERZORDER   0x0200  /* Don't do owner Z ordering */

#define SWP_DRAWFRAME       SWP_FRAMECHANGED
#define SWP_NOREPOSITION    SWP_NOOWNERZORDER

#define SWP_NOSENDCHANGING  0x0400
#define SWP_DEFERERASE      0x2000

#define HWND_BROADCAST	    ((HWND)0xffff)

/* SetWindowPos() hwndInsertAfter field values */
#define HWND_TOP            ((HWND)0)
#define HWND_BOTTOM         ((HWND)1)
#define HWND_TOPMOST        ((HWND)-1)
#define HWND_NOTOPMOST      ((HWND)-2)

/* Flags for TrackPopupMenu */
#define TPM_LEFTBUTTON  0x0000
#define TPM_RIGHTBUTTON 0x0002
#define TPM_LEFTALIGN   0x0000
#define TPM_CENTERALIGN 0x0004
#define TPM_RIGHTALIGN  0x0008

#define MF_INSERT          0x0000
#define MF_CHANGE          0x0080
#define MF_APPEND          0x0100
#define MF_DELETE          0x0200
#define MF_REMOVE          0x1000
#define MF_END             0x0080

#define MF_ENABLED         0x0000
#define MF_GRAYED          0x0001
#define MF_DISABLED        0x0002
#define MF_STRING          0x0000
#define MF_BITMAP          0x0004
#define MF_UNCHECKED       0x0000
#define MF_CHECKED         0x0008
#define MF_POPUP           0x0010
#define MF_MENUBARBREAK    0x0020
#define MF_MENUBREAK       0x0040
#define MF_UNHILITE        0x0000
#define MF_HILITE          0x0080
#define MF_OWNERDRAW       0x0100
#define MF_USECHECKBITMAPS 0x0200
#define MF_BYCOMMAND       0x0000
#define MF_BYPOSITION      0x0400
#define MF_SEPARATOR       0x0800
#define MF_SYSMENU         0x2000
#define MF_HELP            0x4000
#define MF_MOUSESELECT     0x8000

#ifndef NOWINOFFSETS
#define GCW_HBRBACKGROUND (-10)
#endif

#define MB_OK               0x0000
#define MB_OKCANCEL         0x0001
#define MB_ABORTRETRYIGNORE 0x0002
#define MB_YESNOCANCEL      0x0003
#define MB_YESNO            0x0004
#define MB_RETRYCANCEL      0x0005
#define MB_TYPEMASK         0x000F

#define MB_ICONHAND         0x0010
#define MB_ICONQUESTION     0x0020
#define MB_ICONEXCLAMATION  0x0030
#define MB_ICONASTERISK     0x0040
#define MB_ICONMASK         0x00F0

#define MB_ICONINFORMATION  MB_ICONASTERISK
#define MB_ICONSTOP         MB_ICONHAND

#define MB_DEFBUTTON1       0x0000
#define MB_DEFBUTTON2       0x0100
#define MB_DEFBUTTON3       0x0200
#define MB_DEFMASK          0x0F00

#define MB_APPLMODAL        0x0000
#define MB_SYSTEMMODAL      0x1000
#define MB_TASKMODAL        0x2000

#define MB_NOFOCUS          0x8000


#define DT_TOP 0
#define DT_LEFT 0
#define DT_CENTER 1
#define DT_RIGHT 2
#define DT_VCENTER 4
#define DT_BOTTOM 8
#define DT_WORDBREAK 16
#define DT_SINGLELINE 32
#define DT_EXPANDTABS 64
#define DT_TABSTOP 128
#define DT_NOCLIP 256
#define DT_EXTERNALLEADING 512
#define DT_CALCRECT 1024
#define DT_NOPREFIX 2048
#define DT_INTERNAL 4096

/* DrawEdge() flags */
#define BDR_RAISEDOUTER    0x0001
#define BDR_SUNKENOUTER    0x0002
#define BDR_RAISEDINNER    0x0004
#define BDR_SUNKENINNER    0x0008

#define BDR_OUTER          0x0003
#define BDR_INNER          0x000c
#define BDR_RAISED         0x0005
#define BDR_SUNKEN         0x000a

#define EDGE_RAISED        (BDR_RAISEDOUTER | BDR_RAISEDINNER)
#define EDGE_SUNKEN        (BDR_SUNKENOUTER | BDR_SUNKENINNER)
#define EDGE_ETCHED        (BDR_SUNKENOUTER | BDR_RAISEDINNER)
#define EDGE_BUMP          (BDR_RAISEDOUTER | BDR_SUNKENINNER)

/* border flags */
#define BF_LEFT            0x0001
#define BF_TOP             0x0002
#define BF_RIGHT           0x0004
#define BF_BOTTOM          0x0008
#define BF_DIAGONAL        0x0010
#define BF_MIDDLE          0x0800  /* Fill in the middle */
#define BF_SOFT            0x1000  /* For softer buttons */
#define BF_ADJUST          0x2000  /* Calculate the space left over */
#define BF_FLAT            0x4000  /* For flat rather than 3D borders */
#define BF_MONO            0x8000  /* For monochrome borders */
#define BF_TOPLEFT         (BF_TOP | BF_LEFT)
#define BF_TOPRIGHT        (BF_TOP | BF_RIGHT)
#define BF_BOTTOMLEFT      (BF_BOTTOM | BF_LEFT)
#define BF_BOTTOMRIGHT     (BF_BOTTOM | BF_RIGHT)
#define BF_RECT            (BF_LEFT | BF_TOP | BF_RIGHT | BF_BOTTOM)
#define BF_DIAGONAL_ENDTOPRIGHT     (BF_DIAGONAL | BF_TOP | BF_RIGHT)
#define BF_DIAGONAL_ENDTOPLEFT      (BF_DIAGONAL | BF_TOP | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMLEFT   (BF_DIAGONAL | BF_BOTTOM | BF_LEFT)
#define BF_DIAGONAL_ENDBOTTOMRIGHT  (BF_DIAGONAL | BF_BOTTOM | BF_RIGHT)

/* Window Styles */
#define WS_OVERLAPPED    0x00000000L
#define WS_POPUP         0x80000000L
#define WS_CHILD         0x40000000L
#define WS_MINIMIZE      0x20000000L
#define WS_VISIBLE       0x10000000L
#define WS_DISABLED      0x08000000L
#define WS_CLIPSIBLINGS  0x04000000L
#define WS_CLIPCHILDREN  0x02000000L
#define WS_MAXIMIZE      0x01000000L
#define WS_CAPTION       0x00C00000L
#define WS_BORDER        0x00800000L
#define WS_DLGFRAME      0x00400000L
#define WS_VSCROLL       0x00200000L
#define WS_HSCROLL       0x00100000L
#define WS_SYSMENU       0x00080000L
#define WS_THICKFRAME    0x00040000L
#define WS_GROUP         0x00020000L
#define WS_TABSTOP       0x00010000L
#define WS_MINIMIZEBOX   0x00020000L
#define WS_MAXIMIZEBOX   0x00010000L
#define WS_TILED         WS_OVERLAPPED
#define WS_ICONIC        WS_MINIMIZE
#define WS_SIZEBOX       WS_THICKFRAME
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME| WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW (WS_CHILD)
#define WS_TILEDWINDOW (WS_OVERLAPPEDWINDOW)

/* Window extended styles */
#define WS_EX_DLGMODALFRAME    0x00000001L
#define WS_EX_DRAGDETECT       0x00000002L
#define WS_EX_NOPARENTNOTIFY   0x00000004L
#define WS_EX_TOPMOST          0x00000008L
#define WS_EX_ACCEPTFILES      0x00000010L
#define WS_EX_TRANSPARENT      0x00000020L

/* Window scrolling */
#define SW_SCROLLCHILDREN      0x0001
#define SW_INVALIDATE          0x0002
#define SW_ERASE               0x0004

/* CreateWindow() coordinates */
#define CW_USEDEFAULT16 ((INT16)0x8000)
#define CW_USEDEFAULT32 ((INT32)0x80000000)
#define CW_USEDEFAULT   WINELIB_NAME(CW_USEDEFAULT)

/* Button control styles */
#define BS_PUSHBUTTON          0x00000000L
#define BS_DEFPUSHBUTTON       0x00000001L
#define BS_CHECKBOX            0x00000002L
#define BS_AUTOCHECKBOX        0x00000003L
#define BS_RADIOBUTTON         0x00000004L
#define BS_3STATE              0x00000005L
#define BS_AUTO3STATE          0x00000006L
#define BS_GROUPBOX            0x00000007L
#define BS_USERBUTTON          0x00000008L
#define BS_AUTORADIOBUTTON     0x00000009L
#define BS_OWNERDRAW           0x0000000BL
#define BS_LEFTTEXT            0x00000020L

/* Win16 button control messages */
#define BM_GETCHECK16          (WM_USER+0)
#define BM_SETCHECK16          (WM_USER+1)
#define BM_GETSTATE16          (WM_USER+2)
#define BM_SETSTATE16          (WM_USER+3)
#define BM_SETSTYLE16          (WM_USER+4)
#define BM_CLICK16             WM_NULL  /* Does not exist in Win16 */
#define BM_GETIMAGE16          WM_NULL  /* Does not exist in Win16 */
#define BM_SETIMAGE16          WM_NULL  /* Does not exist in Win16 */
/* Win32 button control messages */
#define BM_GETCHECK32          0x00f0
#define BM_SETCHECK32          0x00f1
#define BM_GETSTATE32          0x00f2
#define BM_SETSTATE32          0x00f3
#define BM_SETSTYLE32          0x00f4
#define BM_CLICK32             0x00f5
#define BM_GETIMAGE32          0x00f6
#define BM_SETIMAGE32          0x00f7
/* Winelib button control messages */
#define BM_GETCHECK            WINELIB_NAME(BM_GETCHECK)
#define BM_SETCHECK            WINELIB_NAME(BM_SETCHECK)
#define BM_GETSTATE            WINELIB_NAME(BM_GETSTATE)
#define BM_SETSTATE            WINELIB_NAME(BM_SETSTATE)
#define BM_SETSTYLE            WINELIB_NAME(BM_SETSTYLE)
#define BM_CLICK               WINELIB_NAME(BM_CLICK)
#define BM_GETIMAGE            WINELIB_NAME(BM_GETIMAGE)
#define BM_SETIMAGE            WINELIB_NAME(BM_SETIMAGE)

/* Button notification codes */
#define BN_CLICKED             0
#define BN_PAINT               1
#define BN_HILITE              2
#define BN_UNHILITE            3
#define BN_DISABLE             4
#define BN_DOUBLECLICKED       5

/* Static Control Styles */
#define SS_LEFT             0x00000000L
#define SS_CENTER           0x00000001L
#define SS_RIGHT            0x00000002L
#define SS_ICON             0x00000003L
#define SS_BLACKRECT        0x00000004L
#define SS_GRAYRECT         0x00000005L
#define SS_WHITERECT        0x00000006L
#define SS_BLACKFRAME       0x00000007L
#define SS_GRAYFRAME        0x00000008L
#define SS_WHITEFRAME       0x00000009L
#define SS_SIMPLE           0x0000000BL
#define SS_LEFTNOWORDWRAP   0x0000000CL
#define SS_NOPREFIX         0x00000080L

/* Static Control Messages */
#define STM_SETICON         (WM_USER+0)
#define STM_GETICON         (WM_USER+1)

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
#define LB_ADDSTRING16           (WM_USER+1)
#define LB_ADDSTRING32           0x0180
#define LB_ADDSTRING             WINELIB_NAME(LB_ADDSTRING)
#define LB_INSERTSTRING16        (WM_USER+2)
#define LB_INSERTSTRING32        0x0181
#define LB_INSERTSTRING          WINELIB_NAME(LB_INSERTSTRING)
#define LB_DELETESTRING16        (WM_USER+3)
#define LB_DELETESTRING32        0x0182
#define LB_DELETESTRING          WINELIB_NAME(LB_DELETESTRING)
#define LB_SELITEMRANGEEX16      (WM_USER+4)
#define LB_SELITEMRANGEEX32      0x0183
#define LB_SELITEMRANGEEX        WINELIB_NAME(LB_SELITEMRANGEEX)
#define LB_RESETCONTENT16        (WM_USER+5)
#define LB_RESETCONTENT32        0x0184
#define LB_RESETCONTENT          WINELIB_NAME(LB_RESETCONTENT)
#define LB_SETSEL16              (WM_USER+6)
#define LB_SETSEL32              0x0185
#define LB_SETSEL                WINELIB_NAME(LB_SETSEL)
#define LB_SETCURSEL16           (WM_USER+7)
#define LB_SETCURSEL32           0x0186
#define LB_SETCURSEL             WINELIB_NAME(LB_SETCURSEL)
#define LB_GETSEL16              (WM_USER+8)
#define LB_GETSEL32              0x0187
#define LB_GETSEL                WINELIB_NAME(LB_GETSEL)
#define LB_GETCURSEL16           (WM_USER+9)
#define LB_GETCURSEL32           0x0188
#define LB_GETCURSEL             WINELIB_NAME(LB_GETCURSEL)
#define LB_GETTEXT16             (WM_USER+10)
#define LB_GETTEXT32             0x0189
#define LB_GETTEXT               WINELIB_NAME(LB_GETTEXT)
#define LB_GETTEXTLEN16          (WM_USER+11)
#define LB_GETTEXTLEN32          0x018a
#define LB_GETTEXTLEN            WINELIB_NAME(LB_GETTEXTLEN)
#define LB_GETCOUNT16            (WM_USER+12)
#define LB_GETCOUNT32            0x018b
#define LB_GETCOUNT              WINELIB_NAME(LB_GETCOUNT)
#define LB_SELECTSTRING16        (WM_USER+13)
#define LB_SELECTSTRING32        0x018c
#define LB_SELECTSTRING          WINELIB_NAME(LB_SELECTSTRING)
#define LB_DIR16                 (WM_USER+14)
#define LB_DIR32                 0x018d
#define LB_DIR                   WINELIB_NAME(LB_DIR)
#define LB_GETTOPINDEX16         (WM_USER+15)
#define LB_GETTOPINDEX32         0x018e
#define LB_GETTOPINDEX           WINELIB_NAME(LB_GETTOPINDEX)
#define LB_FINDSTRING16          (WM_USER+16)
#define LB_FINDSTRING32          0x018f
#define LB_FINDSTRING            WINELIB_NAME(LB_FINDSTRING)
#define LB_GETSELCOUNT16         (WM_USER+17)
#define LB_GETSELCOUNT32         0x0190
#define LB_GETSELCOUNT           WINELIB_NAME(LB_GETSELCOUNT)
#define LB_GETSELITEMS16         (WM_USER+18)
#define LB_GETSELITEMS32         0x0191
#define LB_GETSELITEMS           WINELIB_NAME(LB_GETSELITEMS)
#define LB_SETTABSTOPS16         (WM_USER+19)
#define LB_SETTABSTOPS32         0x0192
#define LB_SETTABSTOPS           WINELIB_NAME(LB_SETTABSTOPS)
#define LB_GETHORIZONTALEXTENT16 (WM_USER+20)
#define LB_GETHORIZONTALEXTENT32 0x0193
#define LB_GETHORIZONTALEXTENT   WINELIB_NAME(LB_GETHORIZONTALEXTENT)
#define LB_SETHORIZONTALEXTENT16 (WM_USER+21)
#define LB_SETHORIZONTALEXTENT32 0x0194
#define LB_SETHORIZONTALEXTENT   WINELIB_NAME(LB_SETHORIZONTALEXTENT)
#define LB_SETCOLUMNWIDTH16      (WM_USER+22)
#define LB_SETCOLUMNWIDTH32      0x0195
#define LB_SETCOLUMNWIDTH        WINELIB_NAME(LB_SETCOLUMNWIDTH)
#define LB_ADDFILE16             (WM_USER+23)
#define LB_ADDFILE32             0x0196
#define LB_ADDFILE               WINELIB_NAME(LB_ADDFILE)
#define LB_SETTOPINDEX16         (WM_USER+24)
#define LB_SETTOPINDEX32         0x0197
#define LB_SETTOPINDEX           WINELIB_NAME(LB_SETTOPINDEX)
#define LB_GETITEMRECT16         (WM_USER+25)
#define LB_GETITEMRECT32         0x0198
#define LB_GETITEMRECT           WINELIB_NAME(LB_GETITEMRECT)
#define LB_GETITEMDATA16         (WM_USER+26)
#define LB_GETITEMDATA32         0x0199
#define LB_GETITEMDATA           WINELIB_NAME(LB_GETITEMDATA)
#define LB_SETITEMDATA16         (WM_USER+27)
#define LB_SETITEMDATA32         0x019a
#define LB_SETITEMDATA           WINELIB_NAME(LB_SETITEMDATA)
#define LB_SELITEMRANGE16        (WM_USER+28)
#define LB_SELITEMRANGE32        0x019b
#define LB_SELITEMRANGE          WINELIB_NAME(LB_SELITEMRANGE)
#define LB_SETANCHORINDEX16      (WM_USER+29)
#define LB_SETANCHORINDEX32      0x019c
#define LB_SETANCHORINDEX        WINELIB_NAME(LB_SETANCHORINDEX)
#define LB_GETANCHORINDEX16      (WM_USER+30)
#define LB_GETANCHORINDEX32      0x019d
#define LB_GETANCHORINDEX        WINELIB_NAME(LB_GETANCHORINDEX)
#define LB_SETCARETINDEX16       (WM_USER+31)
#define LB_SETCARETINDEX32       0x019e
#define LB_SETCARETINDEX         WINELIB_NAME(LB_SETCARETINDEX)
#define LB_GETCARETINDEX16       (WM_USER+32)
#define LB_GETCARETINDEX32       0x019f
#define LB_GETCARETINDEX         WINELIB_NAME(LB_GETCARETINDEX)
#define LB_SETITEMHEIGHT16       (WM_USER+33)
#define LB_SETITEMHEIGHT32       0x01a0
#define LB_SETITEMHEIGHT         WINELIB_NAME(LB_SETITEMHEIGHT)
#define LB_GETITEMHEIGHT16       (WM_USER+34)
#define LB_GETITEMHEIGHT32       0x01a1
#define LB_GETITEMHEIGHT         WINELIB_NAME(LB_GETITEMHEIGHT)
#define LB_FINDSTRINGEXACT16     (WM_USER+35)
#define LB_FINDSTRINGEXACT32     0x01a2
#define LB_FINDSTRINGEXACT       WINELIB_NAME(LB_FINDSTRINGEXACT)
#define LB_CARETON16             (WM_USER+36)
#define LB_CARETON32             0x01a3
#define LB_CARETON               WINELIB_NAME(LB_CARETON)
#define LB_CARETOFF16            (WM_USER+37)
#define LB_CARETOFF32            0x01a4
#define LB_CARETOFF              WINELIB_NAME(LB_CARETOFF)
#define LB_SETLOCALE16           WM_NULL  /* Not in Win16 */
#define LB_SETLOCALE32           0x01a5
#define LB_SETLOCALE             WINELIB_NAME(LB_SETLOCALE)
#define LB_GETLOCALE16           WM_NULL  /* Not in Win16 */
#define LB_GETLOCALE32           0x01a6
#define LB_GETLOCALE             WINELIB_NAME(LB_GETLOCALE)
#define LB_SETCOUNT16            WM_NULL  /* Not in Win16 */
#define LB_SETCOUNT32            0x01a7
#define LB_SETCOUNT              WINELIB_NAME(LB_SETCOUNT)
#define LB_INITSTORAGE16         WM_NULL  /* Not in Win16 */
#define LB_INITSTORAGE32         0x01a8
#define LB_INITSTORAGE           WINELIB_NAME(LB_INITSTORAGE)
#define LB_ITEMFROMPOINT16       WM_NULL  /* Not in Win16 */
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

/* Combo box messages */
#define CB_GETEDITSEL16            (WM_USER+0)
#define CB_GETEDITSEL32            0x0140
#define CB_GETEDITSEL              WINELIB_NAME(CB_GETEDITSEL)
#define CB_LIMITTEXT16             (WM_USER+1)
#define CB_LIMITTEXT32             0x0141
#define CB_LIMITTEXT               WINELIB_NAME(CB_LIMITTEXT)
#define CB_SETEDITSEL16            (WM_USER+2)
#define CB_SETEDITSEL32            0x0142
#define CB_SETEDITSEL              WINELIB_NAME(CB_SETEDITSEL)
#define CB_ADDSTRING16             (WM_USER+3)
#define CB_ADDSTRING32             0x0143
#define CB_ADDSTRING               WINELIB_NAME(CB_ADDSTRING)
#define CB_DELETESTRING16          (WM_USER+4)
#define CB_DELETESTRING32          0x0144
#define CB_DELETESTRING            WINELIB_NAME(CB_DELETESTRING)
#define CB_DIR16                   (WM_USER+5)
#define CB_DIR32                   0x0145
#define CB_DIR                     WINELIB_NAME(CB_DIR)
#define CB_GETCOUNT16              (WM_USER+6)
#define CB_GETCOUNT32              0x0146
#define CB_GETCOUNT                WINELIB_NAME(CB_GETCOUNT)
#define CB_GETCURSEL16             (WM_USER+7)
#define CB_GETCURSEL32             0x0147
#define CB_GETCURSEL               WINELIB_NAME(CB_GETCURSEL)
#define CB_GETLBTEXT16             (WM_USER+8)
#define CB_GETLBTEXT32             0x0148
#define CB_GETLBTEXT               WINELIB_NAME(CB_GETLBTEXT)
#define CB_GETLBTEXTLEN16          (WM_USER+9)
#define CB_GETLBTEXTLEN32          0x0149
#define CB_GETLBTEXTLEN            WINELIB_NAME(CB_GETLBTEXTLEN)
#define CB_INSERTSTRING16          (WM_USER+10)
#define CB_INSERTSTRING32          0x014a
#define CB_INSERTSTRING            WINELIB_NAME(CB_INSERTSTRING)
#define CB_RESETCONTENT16          (WM_USER+11)
#define CB_RESETCONTENT32          0x014b
#define CB_RESETCONTENT            WINELIB_NAME(CB_RESETCONTENT)
#define CB_FINDSTRING16            (WM_USER+12)
#define CB_FINDSTRING32            0x014c
#define CB_FINDSTRING              WINELIB_NAME(CB_FINDSTRING)
#define CB_SELECTSTRING16          (WM_USER+13)
#define CB_SELECTSTRING32          0x014d
#define CB_SELECTSTRING            WINELIB_NAME(CB_SELECTSTRING)
#define CB_SETCURSEL16             (WM_USER+14)
#define CB_SETCURSEL32             0x014e
#define CB_SETCURSEL               WINELIB_NAME(CB_SETCURSEL)
#define CB_SHOWDROPDOWN16          (WM_USER+15)
#define CB_SHOWDROPDOWN32          0x014f
#define CB_SHOWDROPDOWN            WINELIB_NAME(CB_SHOWDROPDOWN)
#define CB_GETITEMDATA16           (WM_USER+16)
#define CB_GETITEMDATA32           0x0150
#define CB_GETITEMDATA             WINELIB_NAME(CB_GETITEMDATA)
#define CB_SETITEMDATA16           (WM_USER+17)
#define CB_SETITEMDATA32           0x0151
#define CB_SETITEMDATA             WINELIB_NAME(CB_SETITEMDATA)
#define CB_GETDROPPEDCONTROLRECT16 (WM_USER+18)
#define CB_GETDROPPEDCONTROLRECT32 0x0152
#define CB_GETDROPPEDCONTROLRECT   WINELIB_NAME(CB_GETDROPPEDCONTROLRECT)
#define CB_SETITEMHEIGHT16         (WM_USER+19)
#define CB_SETITEMHEIGHT32         0x0153
#define CB_SETITEMHEIGHT           WINELIB_NAME(CB_SETITEMHEIGHT)
#define CB_GETITEMHEIGHT16         (WM_USER+20)
#define CB_GETITEMHEIGHT32         0x0154
#define CB_GETITEMHEIGHT           WINELIB_NAME(CB_GETITEMHEIGHT)
#define CB_SETEXTENDEDUI16         (WM_USER+21)
#define CB_SETEXTENDEDUI32         0x0155
#define CB_SETEXTENDEDUI           WINELIB_NAME(CB_SETEXTENDEDUI)
#define CB_GETEXTENDEDUI16         (WM_USER+22)
#define CB_GETEXTENDEDUI32         0x0156
#define CB_GETEXTENDEDUI           WINELIB_NAME(CB_GETEXTENDEDUI)
#define CB_GETDROPPEDSTATE16       (WM_USER+23)
#define CB_GETDROPPEDSTATE32       0x0157
#define CB_GETDROPPEDSTATE         WINELIB_NAME(CB_GETDROPPEDSTATE)
#define CB_FINDSTRINGEXACT16       (WM_USER+24)
#define CB_FINDSTRINGEXACT32       0x0158
#define CB_FINDSTRINGEXACT         WINELIB_NAME(CB_FINDSTRINGEXACT)
#define CB_SETLOCALE16             WM_NULL  /* Not in Win16 */
#define CB_SETLOCALE32             0x0159
#define CB_SETLOCALE               WINELIB_NAME(CB_SETLOCALE)
#define CB_GETLOCALE16             WM_NULL  /* Not in Win16 */
#define CB_GETLOCALE32             0x015a
#define CB_GETLOCALE               WINELIB_NAME(CB_GETLOCALE)
#define CB_GETTOPINDEX16           WM_NULL  /* Not in Win16 */
#define CB_GETTOPINDEX32           0x015b
#define CB_GETTOPINDEX             WINELIB_NAME(CB_GETTOPINDEX)
#define CB_SETTOPINDEX16           WM_NULL  /* Not in Win16 */
#define CB_SETTOPINDEX32           0x015c
#define CB_SETTOPINDEX             WINELIB_NAME(CB_SETTOPINDEX)
#define CB_GETHORIZONTALEXTENT16   WM_NULL  /* Not in Win16 */
#define CB_GETHORIZONTALEXTENT32   0x015d
#define CB_GETHORIZONTALEXTENT     WINELIB_NAME(CB_GETHORIZONTALEXTENT)
#define CB_SETHORIZONTALEXTENT16   WM_NULL  /* Not in Win16 */
#define CB_SETHORIZONTALEXTENT32   0x015e
#define CB_SETHORIZONTALEXTENT     WINELIB_NAME(CB_SETHORIZONTALEXTENT)
#define CB_GETDROPPEDWIDTH16       WM_NULL  /* Not in Win16 */
#define CB_GETDROPPEDWIDTH32       0x015f
#define CB_GETDROPPEDWIDTH         WINELIB_NAME(CB_GETDROPPEDWIDTH)
#define CB_SETDROPPEDWIDTH16       WM_NULL  /* Not in Win16 */
#define CB_SETDROPPEDWIDTH32       0x0160
#define CB_SETDROPPEDWIDTH         WINELIB_NAME(CB_SETDROPPEDWIDTH)
#define CB_INITSTORAGE16           WM_NULL  /* Not in Win16 */
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


/* Owner draw control types */
#define ODT_MENU        1
#define ODT_LISTBOX     2
#define ODT_COMBOBOX    3
#define ODT_BUTTON      4

/* Owner draw actions */
#define ODA_DRAWENTIRE  0x0001
#define ODA_SELECT      0x0002
#define ODA_FOCUS       0x0004

/* Owner draw state */
#define ODS_SELECTED    0x0001
#define ODS_GRAYED      0x0002
#define ODS_DISABLED    0x0004
#define ODS_CHECKED     0x0008
#define ODS_FOCUS       0x0010

/* Edit control styles */
#define ES_LEFT         0x00000000L
#define ES_CENTER       0x00000001L
#define ES_RIGHT        0x00000002L
#define ES_MULTILINE    0x00000004L
#define ES_UPPERCASE    0x00000008L
#define ES_LOWERCASE    0x00000010L
#define ES_PASSWORD     0x00000020L
#define ES_AUTOVSCROLL  0x00000040L
#define ES_AUTOHSCROLL  0x00000080L
#define ES_NOHIDESEL    0x00000100L
#define ES_OEMCONVERT   0x00000400L
#define ES_READONLY     0x00000800L
#define ES_WANTRETURN   0x00001000L

/* Edit control messages */
#define EM_GETSEL              (WM_USER+0)
#define EM_SETSEL              (WM_USER+1)
#define EM_GETRECT             (WM_USER+2)
#define EM_SETRECT             (WM_USER+3)
#define EM_SETRECTNP           (WM_USER+4)
#define EM_LINESCROLL          (WM_USER+6)
#define EM_GETMODIFY           (WM_USER+8)
#define EM_SETMODIFY           (WM_USER+9)
#define EM_GETLINECOUNT        (WM_USER+10)
#define EM_LINEINDEX           (WM_USER+11)
#define EM_SETHANDLE           (WM_USER+12)
#define EM_GETHANDLE           (WM_USER+13)
#define EM_LINELENGTH          (WM_USER+17)
#define EM_REPLACESEL          (WM_USER+18)
#define EM_GETLINE             (WM_USER+20)
#define EM_LIMITTEXT           (WM_USER+21)
#define EM_CANUNDO             (WM_USER+22)
#define EM_UNDO                (WM_USER+23)
#define EM_FMTLINES            (WM_USER+24)
#define EM_LINEFROMCHAR        (WM_USER+25)
#define EM_SETTABSTOPS         (WM_USER+27)
#define EM_SETPASSWORDCHAR     (WM_USER+28)
#define EM_EMPTYUNDOBUFFER     (WM_USER+29)
#define EM_GETFIRSTVISIBLELINE (WM_USER+30)
#define EM_SETREADONLY         (WM_USER+31)
#define EM_SETWORDBREAKPROC    (WM_USER+32)
#define EM_GETWORDBREAKPROC    (WM_USER+33)
#define EM_GETPASSWORDCHAR     (WM_USER+34)
/* Edit control undocumented messages */
#define EM_SCROLL              (WM_USER+5)
#define EM_GETTHUMB            (WM_USER+14)

/* EDITWORDBREAKPROC code values */
#define WB_LEFT         0
#define WB_RIGHT        1
#define WB_ISDELIMITER  2

/* Edit control notification codes */
#define EN_SETFOCUS     0x0100
#define EN_KILLFOCUS    0x0200
#define EN_CHANGE       0x0300
#define EN_UPDATE       0x0400
#define EN_ERRSPACE     0x0500
#define EN_MAXTEXT      0x0501
#define EN_HSCROLL      0x0601
#define EN_VSCROLL      0x0602


typedef struct
{
    UINT16      CtlType;
    UINT16      CtlID;
    UINT16      itemID;
    UINT16      itemAction;
    UINT16      itemState;
    HWND16      hwndItem;
    HDC16       hDC;
    RECT16      rcItem WINE_PACKED;
    DWORD       itemData WINE_PACKED;
} DRAWITEMSTRUCT16, *PDRAWITEMSTRUCT16, *LPDRAWITEMSTRUCT16;

typedef struct
{
    UINT32      CtlType;
    UINT32      CtlID;
    UINT32      itemID;
    UINT32      itemAction;
    UINT32      itemState;
    HWND32      hwndItem;
    HDC32       hDC;
    RECT32      rcItem WINE_PACKED;
    DWORD       itemData WINE_PACKED;
} DRAWITEMSTRUCT32, *PDRAWITEMSTRUCT32, *LPDRAWITEMSTRUCT32;

DECL_WINELIB_TYPE(DRAWITEMSTRUCT);
DECL_WINELIB_TYPE(PDRAWITEMSTRUCT);
DECL_WINELIB_TYPE(LPDRAWITEMSTRUCT);

typedef struct
{
    UINT16      CtlType;
    UINT16      CtlID;
    UINT16      itemID;
    UINT16      itemWidth;
    UINT16      itemHeight;
    DWORD       itemData WINE_PACKED;
} MEASUREITEMSTRUCT16, *PMEASUREITEMSTRUCT16, *LPMEASUREITEMSTRUCT16;

typedef struct
{
    UINT32      CtlType;
    UINT32      CtlID;
    UINT32      itemID;
    UINT32      itemWidth;
    UINT32      itemHeight;
    DWORD       itemData;
} MEASUREITEMSTRUCT32, *PMEASUREITEMSTRUCT32, *LPMEASUREITEMSTRUCT32;

DECL_WINELIB_TYPE(MEASUREITEMSTRUCT);
DECL_WINELIB_TYPE(PMEASUREITEMSTRUCT);
DECL_WINELIB_TYPE(LPMEASUREITEMSTRUCT);

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
    UINT32     CtlType;
    UINT32     CtlID;
    UINT32     itemID;
    HWND32     hwndItem;
    DWORD      itemData;
} DELETEITEMSTRUCT32, *LPDELETEITEMSTRUCT32;

DECL_WINELIB_TYPE(DELETEITEMSTRUCT);
DECL_WINELIB_TYPE(LPDELETEITEMSTRUCT);

typedef struct
{
    UINT16      CtlType;
    UINT16      CtlID;
    HWND16      hwndItem;
    UINT16      itemID1;
    DWORD       itemData1;
    UINT16      itemID2;
    DWORD       itemData2 WINE_PACKED;
} COMPAREITEMSTRUCT16, *LPCOMPAREITEMSTRUCT16;

typedef struct
{
    UINT32      CtlType;
    UINT32      CtlID;
    HWND32      hwndItem;
    UINT32      itemID1;
    DWORD       itemData1;
    UINT32      itemID2;
    DWORD       itemData2;
    DWORD       dwLocaleId;
} COMPAREITEMSTRUCT32, *LPCOMPAREITEMSTRUCT32;

DECL_WINELIB_TYPE(COMPAREITEMSTRUCT);
DECL_WINELIB_TYPE(LPCOMPAREITEMSTRUCT);

/* WM_KEYUP/DOWN/CHAR HIWORD(lParam) flags */
#define KF_EXTENDED         0x0100
#define KF_DLGMODE          0x0800
#define KF_MENUMODE         0x1000
#define KF_ALTDOWN          0x2000
#define KF_REPEAT           0x4000
#define KF_UP               0x8000

/* Virtual key codes */
#define VK_LBUTTON          0x01
#define VK_RBUTTON          0x02
#define VK_CANCEL           0x03
#define VK_MBUTTON          0x04
/*                          0x05-0x07  Undefined */
#define VK_BACK             0x08
#define VK_TAB              0x09
/*                          0x0A-0x0B  Undefined */
#define VK_CLEAR            0x0C
#define VK_RETURN           0x0D
/*                          0x0E-0x0F  Undefined */
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_PAUSE            0x13
#define VK_CAPITAL          0x14
/*                          0x15-0x19  Reserved for Kanji systems */
/*                          0x1A       Undefined */
#define VK_ESCAPE           0x1B
/*                          0x1C-0x1F  Reserved for Kanji systems */
#define VK_SPACE            0x20
#define VK_PRIOR            0x21
#define VK_NEXT             0x22
#define VK_END              0x23
#define VK_HOME             0x24
#define VK_LEFT             0x25
#define VK_UP               0x26
#define VK_RIGHT            0x27
#define VK_DOWN             0x28
#define VK_SELECT           0x29
#define VK_PRINT            0x2A /* OEM specific in Windows 3.1 SDK */
#define VK_EXECUTE          0x2B
#define VK_SNAPSHOT         0x2C
#define VK_INSERT           0x2D
#define VK_DELETE           0x2E
#define VK_HELP             0x2F
#define VK_0                0x30
#define VK_1                0x31
#define VK_2                0x32
#define VK_3                0x33
#define VK_4                0x34
#define VK_5                0x35
#define VK_6                0x36
#define VK_7                0x37
#define VK_8                0x38
#define VK_9                0x39
/*                          0x3A-0x40  Undefined */
#define VK_A                0x41
#define VK_B                0x42
#define VK_C                0x43
#define VK_D                0x44
#define VK_E                0x45
#define VK_F                0x46
#define VK_G                0x47
#define VK_H                0x48
#define VK_I                0x49
#define VK_J                0x4A
#define VK_K                0x4B
#define VK_L                0x4C
#define VK_M                0x4D
#define VK_N                0x4E
#define VK_O                0x4F
#define VK_P                0x50
#define VK_Q                0x51
#define VK_R                0x52
#define VK_S                0x53
#define VK_T                0x54
#define VK_U                0x55
#define VK_V                0x56
#define VK_W                0x57
#define VK_X                0x58
#define VK_Y                0x59
#define VK_Z                0x5A
/*                          0x5B-0x5F  Undefined */
#define VK_NUMPAD0          0x60
#define VK_NUMPAD1          0x61
#define VK_NUMPAD2          0x62
#define VK_NUMPAD3          0x63
#define VK_NUMPAD4          0x64
#define VK_NUMPAD5          0x65
#define VK_NUMPAD6          0x66
#define VK_NUMPAD7          0x67
#define VK_NUMPAD8          0x68
#define VK_NUMPAD9          0x69
#define VK_MULTIPLY         0x6A
#define VK_ADD              0x6B
#define VK_SEPARATOR        0x6C
#define VK_SUBTRACT         0x6D
#define VK_DECIMAL          0x6E
#define VK_DIVIDE           0x6F
#define VK_F1               0x70
#define VK_F2               0x71
#define VK_F3               0x72
#define VK_F4               0x73
#define VK_F5               0x74
#define VK_F6               0x75
#define VK_F7               0x76
#define VK_F8               0x77
#define VK_F9               0x78
#define VK_F10              0x79
#define VK_F11              0x7A
#define VK_F12              0x7B
#define VK_F13              0x7C
#define VK_F14              0x7D
#define VK_F15              0x7E
#define VK_F16              0x7F
#define VK_F17              0x80
#define VK_F18              0x81
#define VK_F19              0x82
#define VK_F20              0x83
#define VK_F21              0x84
#define VK_F22              0x85
#define VK_F23              0x86
#define VK_F24              0x87
/*                          0x88-0x8F  Unassigned */
#define VK_NUMLOCK          0x90
#define VK_SCROLL           0x91
/*                          0x92-0xB9  Unassigned */
/*                          0xBA-0xC0  OEM specific */
/*                          0xC1-0xDA  Unassigned */
/*                          0xDB-0xE4  OEM specific */
/*                          0xE5       Unassigned */
/*                          0xE6       OEM specific */
/*                          0xE7-0xE8  Unassigned */
/*                          0xE9-0xF5  OEM specific */
/*                          0xF6-0xFE  Unassigned */

  
#define LMEM_FIXED          0   
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00
#define LMEM_DISCARDED	    0x4000
#define LMEM_LOCKCOUNT	    0x00FF

#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED
#define GMEM_DISCARDED      0x4000
#define GMEM_LOCKCOUNT      0x00ff
#define GMEM_INVALID_HANDLE 0x8000

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)


typedef struct tagMEMORYSTATUS
{
    DWORD    dwLength;
    DWORD    dwMemoryLoad;
    DWORD    dwTotalPhys;
    DWORD    dwAvailPhys;
    DWORD    dwTotalPageFile;
    DWORD    dwAvailPageFile;
    DWORD    dwTotalVirtual;
    DWORD    dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

/* Predefined Clipboard Formats */
#define CF_TEXT              1
#define CF_BITMAP            2
#define CF_METAFILEPICT      3
#define CF_SYLK              4
#define CF_DIF               5
#define CF_TIFF              6
#define CF_OEMTEXT           7
#define CF_DIB               8
#define CF_PALETTE           9
#define CF_PENDATA          10
#define CF_RIFF             11
#define CF_WAVE             12

#define CF_OWNERDISPLAY     0x0080
#define CF_DSPTEXT          0x0081
#define CF_DSPBITMAP        0x0082
#define CF_DSPMETAFILEPICT  0x0083

/* "Private" formats don't get GlobalFree()'d */
#define CF_PRIVATEFIRST     0x0200
#define CF_PRIVATELAST      0x02FF

/* "GDIOBJ" formats do get DeleteObject()'d */
#define CF_GDIOBJFIRST      0x0300
#define CF_GDIOBJLAST       0x03FF

/* Clipboard command messages */
#define WM_CUT              0x0300
#define WM_COPY             0x0301
#define WM_PASTE            0x0302
#define WM_CLEAR            0x0303
#define WM_UNDO             0x0304

/* Clipboard owner messages */
#define WM_RENDERFORMAT     0x0305
#define WM_RENDERALLFORMATS 0x0306
#define WM_DESTROYCLIPBOARD 0x0307

/* Clipboard viewer messages */
#define WM_DRAWCLIPBOARD    0x0308
#define WM_PAINTCLIPBOARD   0x0309
#define WM_SIZECLIPBOARD    0x030B
#define WM_VSCROLLCLIPBOARD 0x030A
#define WM_HSCROLLCLIPBOARD 0x030E
#define WM_ASKCBFORMATNAME  0x030C
#define WM_CHANGECBCHAIN    0x030D

/* Metafile header structure */
typedef struct
{
    WORD       mtType;
    WORD       mtHeaderSize;
    WORD       mtVersion;
    DWORD      mtSize WINE_PACKED;
    WORD       mtNoObjects;
    DWORD      mtMaxRecord WINE_PACKED;
    WORD       mtNoParameters;
} METAHEADER;

/* Metafile typical record structure */
typedef struct
{
    DWORD      rdSize;
    WORD       rdFunction;
    WORD       rdParam[1];
} METARECORD;
typedef METARECORD *PMETARECORD;
typedef METARECORD *LPMETARECORD;

/* Handle table structure */

typedef struct
{
    HGDIOBJ16 objectHandle[1];
} HANDLETABLE16, *LPHANDLETABLE16;

typedef struct
{
    HGDIOBJ32 objectHandle[1];
} HANDLETABLE32, *LPHANDLETABLE32;

DECL_WINELIB_TYPE(HANDLETABLE);
DECL_WINELIB_TYPE(LPHANDLETABLE);

/* Clipboard metafile picture structure */
typedef struct
{
    INT16        mm;
    INT16        xExt;
    INT16        yExt;
    HMETAFILE16  hMF;
} METAFILEPICT16, *LPMETAFILEPICT16;

typedef struct
{
    INT32        mm;
    INT32        xExt;
    INT32        yExt;
    HMETAFILE32  hMF;
} METAFILEPICT32, *LPMETAFILEPICT32;

DECL_WINELIB_TYPE(METAFILEPICT);
DECL_WINELIB_TYPE(LPMETAFILEPICT);

/* Metafile functions */
#define META_SETBKCOLOR              0x0201
#define META_SETBKMODE               0x0102
#define META_SETMAPMODE              0x0103
#define META_SETROP2                 0x0104
#define META_SETRELABS               0x0105
#define META_SETPOLYFILLMODE         0x0106
#define META_SETSTRETCHBLTMODE       0x0107
#define META_SETTEXTCHAREXTRA        0x0108
#define META_SETTEXTCOLOR            0x0209
#define META_SETTEXTJUSTIFICATION    0x020A
#define META_SETWINDOWORG            0x020B
#define META_SETWINDOWEXT            0x020C
#define META_SETVIEWPORTORG          0x020D
#define META_SETVIEWPORTEXT          0x020E
#define META_OFFSETWINDOWORG         0x020F
#define META_SCALEWINDOWEXT          0x0410
#define META_OFFSETVIEWPORTORG       0x0211
#define META_SCALEVIEWPORTEXT        0x0412
#define META_LINETO                  0x0213
#define META_MOVETO                  0x0214
#define META_EXCLUDECLIPRECT         0x0415
#define META_INTERSECTCLIPRECT       0x0416
#define META_ARC                     0x0817
#define META_ELLIPSE                 0x0418
#define META_FLOODFILL               0x0419
#define META_PIE                     0x081A
#define META_RECTANGLE               0x041B
#define META_ROUNDRECT               0x061C
#define META_PATBLT                  0x061D
#define META_SAVEDC                  0x001E
#define META_SETPIXEL                0x041F
#define META_OFFSETCLIPRGN           0x0220
#define META_TEXTOUT                 0x0521
#define META_BITBLT                  0x0922
#define META_STRETCHBLT              0x0B23
#define META_POLYGON                 0x0324
#define META_POLYLINE                0x0325
#define META_ESCAPE                  0x0626
#define META_RESTOREDC               0x0127
#define META_FILLREGION              0x0228
#define META_FRAMEREGION             0x0429
#define META_INVERTREGION            0x012A
#define META_PAINTREGION             0x012B
#define META_SELECTCLIPREGION        0x012C
#define META_SELECTOBJECT            0x012D
#define META_SETTEXTALIGN            0x012E
#define META_DRAWTEXT                0x062F
#define META_CHORD                   0x0830
#define META_SETMAPPERFLAGS          0x0231
#define META_EXTTEXTOUT              0x0A32
#define META_SETDIBTODEV             0x0D33
#define META_SELECTPALETTE           0x0234
#define META_REALIZEPALETTE          0x0035
#define META_ANIMATEPALETTE          0x0436
#define META_SETPALENTRIES           0x0037
#define META_POLYPOLYGON             0x0538
#define META_RESIZEPALETTE           0x0139
#define META_DIBBITBLT               0x0940
#define META_DIBSTRETCHBLT           0x0B41
#define META_DIBCREATEPATTERNBRUSH   0x0142
#define META_STRETCHDIB              0x0F43
#define META_EXTFLOODFILL            0x0548
#define META_RESETDC                 0x014C
#define META_STARTDOC                0x014D
#define META_STARTPAGE               0x004F
#define META_ENDPAGE                 0x0050
#define META_ABORTDOC                0x0052
#define META_ENDDOC                  0x005E
#define META_DELETEOBJECT            0x01F0
#define META_CREATEPALETTE           0x00F7
#define META_CREATEBRUSH             0x00F8
#define META_CREATEPATTERNBRUSH      0x01F9
#define META_CREATEPENINDIRECT       0x02FA
#define META_CREATEFONTINDIRECT      0x02FB
#define META_CREATEBRUSHINDIRECT     0x02FC
#define META_CREATEBITMAPINDIRECT    0x02FD
#define META_CREATEBITMAP            0x06FE
#define META_CREATEREGION            0x06FF

typedef INT16 (*MFENUMPROC16)(HDC16,HANDLETABLE16*,METARECORD*,INT16,LPARAM);
typedef INT32 (*MFENUMPROC32)(HDC32,HANDLETABLE32*,METARECORD*,INT32,LPARAM);
DECL_WINELIB_TYPE(MFENUMPROC);

/* Debugging support (DEBUG SYSTEM ONLY) */
typedef struct
{
    UINT16  flags;
    DWORD   dwOptions WINE_PACKED;
    DWORD   dwFilter WINE_PACKED;
    CHAR    achAllocModule[8] WINE_PACKED;
    DWORD   dwAllocBreak WINE_PACKED;
    DWORD   dwAllocCount WINE_PACKED;
} WINDEBUGINFO, *LPWINDEBUGINFO;

/* WINDEBUGINFO flags values */
#define WDI_OPTIONS         0x0001
#define WDI_FILTER          0x0002
#define WDI_ALLOCBREAK      0x0004

/* dwOptions values */
#define DBO_CHECKHEAP       0x0001
#define DBO_BUFFERFILL      0x0004
#define DBO_DISABLEGPTRAPPING 0x0010
#define DBO_CHECKFREE       0x0020

#define DBO_SILENT          0x8000

#define DBO_TRACEBREAK      0x2000
#define DBO_WARNINGBREAK    0x1000
#define DBO_NOERRORBREAK    0x0800
#define DBO_NOFATALBREAK    0x0400
#define DBO_INT3BREAK       0x0100

/* DebugOutput flags values */
#define DBF_TRACE           0x0000
#define DBF_WARNING         0x4000
#define DBF_ERROR           0x8000
#define DBF_FATAL           0xc000

/* dwFilter values */
#define DBF_KERNEL          0x1000
#define DBF_KRN_MEMMAN      0x0001
#define DBF_KRN_LOADMODULE  0x0002
#define DBF_KRN_SEGMENTLOAD 0x0004
#define DBF_USER            0x0800
#define DBF_GDI             0x0400
#define DBF_MMSYSTEM        0x0040
#define DBF_PENWIN          0x0020
#define DBF_APPLICATION     0x0008
#define DBF_DRIVER          0x0010

/* Win32-specific structures */

typedef struct {
        WORD wYear;
        WORD wMonth;
        WORD wDayOfWeek;
        WORD wDay;
        WORD wHour;
        WORD wMinute;
        WORD wSecond;
        WORD wMilliseconds;
} SYSTEMTIME, *LPSYSTEMTIME;


/* Code page information.
 */
typedef struct {
        DWORD MaxCharSize;
        BYTE DefaultChar[2];
        BYTE LeadBytes[5];
} CPINFO, *LPCPINFO;

/* The 'overlapped' data structure used by async I/O functions.
 */
typedef struct {
        DWORD Internal;
        DWORD InternalHigh;
        DWORD Offset;
        DWORD OffsetHigh;
        HANDLE32 hEvent;
} OVERLAPPED, *LPOVERLAPPED;

/* Process startup information.
 */
typedef struct {
        DWORD cb;
        LPSTR lpReserved;
        LPSTR lpDesktop;
        LPSTR lpTitle;
        DWORD dwX;
        DWORD dwY;
        DWORD dwXSize;
        DWORD dwYSize;
        DWORD dwXCountChars;
        DWORD dwYCountChars;
        DWORD dwFillAttribute;
        DWORD dwFlags;
        WORD wShowWindow;
        WORD cbReserved2;
        BYTE *lpReserved2;
        HANDLE32 hStdInput;
        HANDLE32 hStdOutput;
        HANDLE32 hStdError;
} STARTUPINFO32A, *LPSTARTUPINFO32A;

typedef struct {
        DWORD cb;
        LPWSTR lpReserved;
        LPWSTR lpDesktop;
        LPWSTR lpTitle;
        DWORD dwX;
        DWORD dwY;
        DWORD dwXSize;
        DWORD dwYSize;
        DWORD dwXCountChars;
        DWORD dwYCountChars;
        DWORD dwFillAttribute;
        DWORD dwFlags;
        WORD wShowWindow;
        WORD cbReserved2;
        BYTE *lpReserved2;
        HANDLE32 hStdInput;
        HANDLE32 hStdOutput;
        HANDLE32 hStdError;
} STARTUPINFO32W, *LPSTARTUPINFO32W;

DECL_WINELIB_TYPE_AW(STARTUPINFO);
DECL_WINELIB_TYPE_AW(LPSTARTUPINFO);

typedef struct {
	HANDLE32	hProcess;
	HANDLE32	hThread;
	DWORD		dwProcessId;
	DWORD		dwThreadId;
} PROCESS_INFORMATION,*LPPROCESS_INFORMATION;

typedef struct {
        LONG Bias;
        WCHAR StandardName[32];
        SYSTEMTIME StandardDate;
        LONG StandardBias;
        WCHAR DaylightName[32];
        SYSTEMTIME DaylightDate;
        LONG DaylightBias;
} TIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

#define TIME_ZONE_ID_UNKNOWN    0
#define TIME_ZONE_ID_STANDARD   1
#define TIME_ZONE_ID_DAYLIGHT   2


/* File object type definitions
 */
#define FILE_TYPE_UNKNOWN       0
#define FILE_TYPE_DISK          1
#define FILE_TYPE_CHAR          2
#define FILE_TYPE_PIPE          3
#define FILE_TYPE_REMOTE        32768

/* File creation flags
 */
#define GENERIC_READ            0x80000000L
#define GENERIC_WRITE           0x40000000L
#define CREATE_NEW              1
#define CREATE_ALWAYS           2
#define OPEN_EXISTING           3
#define OPEN_ALWAYS             4
#define TRUNCATE_EXISTING       5

/* Standard handle identifiers
 */
#define STD_INPUT_HANDLE        ((DWORD) -10)
#define STD_OUTPUT_HANDLE       ((DWORD) -11)
#define STD_ERROR_HANDLE        ((DWORD) -12)

typedef struct
{
  int dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  int dwVolumeSerialNumber;
  int nFileSizeHigh;
  int nFileSizeLow;
  int nNumberOfLinks;
  int nFileIndexHigh;
  int nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION ;

/* File attribute flags
 */
#define FILE_ATTRIBUTE_ARCHIVE          0x0020
#define FILE_ATTRIBUTE_COMPRESSED       0x0800
#define FILE_ATTRIBUTE_DIRECTORY        0x0010
#define FILE_ATTRIBUTE_HIDDEN           0x0002
#define FILE_ATTRIBUTE_NORMAL           0x0080
#define FILE_ATTRIBUTE_READONLY         0x0001
#define FILE_ATTRIBUTE_SYSTEM           0x0004
#define FILE_ATTRIBUTE_TEMPORARY        0x0100
#define FILE_ATTRIBUTE_ATOMIC_WRITE     0x0200
#define FILE_ATTRIBUTE_XACTION_WRITE    0x0400


/* WinHelp internal structure */
typedef struct {
	WORD size;
	WORD command;
	LONG data;
	LONG reserved;
	WORD ofsFilename;
	WORD ofsData;
} WINHELP,*LPWINHELP;

typedef struct
{
    UINT16  mkSize;
    BYTE    mkKeyList;
    BYTE    szKeyPhrase[1];
} MULTIKEYHELP, *LPMULTIKEYHELP;

typedef struct {
	WORD wStructSize;
	WORD x;
	WORD y;
	WORD dx;
	WORD dy;
	WORD wMax;
	char rgchMember[2];
} HELPWININFO, *LPHELPWININFO;

#define HELP_CONTEXT        0x0001
#define HELP_QUIT           0x0002
#define HELP_INDEX          0x0003
#define HELP_CONTENTS       0x0003
#define HELP_HELPONHELP     0x0004
#define HELP_SETINDEX       0x0005
#define HELP_SETCONTENTS    0x0005
#define HELP_CONTEXTPOPUP   0x0008
#define HELP_FORCEFILE      0x0009
#define HELP_KEY            0x0101
#define HELP_COMMAND        0x0102
#define HELP_PARTIALKEY     0x0105
#define HELP_MULTIKEY       0x0201
#define HELP_SETWINPOS      0x0203

#define CCHDEVICENAME 32
#define CCHFORMNAME   32

typedef struct
{
    BYTE   dmDeviceName[CCHDEVICENAME];
    WORD   dmSpecVersion;
    WORD   dmDriverVersion;
    WORD   dmSize;
    WORD   dmDriverExtra;
    DWORD  dmFields;
    INT16  dmOrientation;
    INT16  dmPaperSize;
    INT16  dmPaperLength;
    INT16  dmPaperWidth;
    INT16  dmScale;
    INT16  dmCopies;
    INT16  dmDefaultSource;
    INT16  dmPrintQuality;
    INT16  dmColor;
    INT16  dmDuplex;
    INT16  dmYResolution;
    INT16  dmTTOption;
    INT16  dmCollate;
    BYTE   dmFormName[CCHFORMNAME];
    WORD   dmUnusedPadding;
    WORD   dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
} DEVMODE16, *LPDEVMODE16;

typedef struct
{
    BYTE   dmDeviceName[CCHDEVICENAME];
    WORD   dmSpecVersion;
    WORD   dmDriverVersion;
    WORD   dmSize;
    WORD   dmDriverExtra;
    DWORD  dmFields;
    INT16  dmOrientation;
    INT16  dmPaperSize;
    INT16  dmPaperLength;
    INT16  dmPaperWidth;
    INT16  dmScale;
    INT16  dmCopies;
    INT16  dmDefaultSource;
    INT16  dmPrintQuality;
    INT16  dmColor;
    INT16  dmDuplex;
    INT16  dmYResolution;
    INT16  dmTTOption;
    INT16  dmCollate;
    BYTE   dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
} DEVMODE32A, *LPDEVMODE32A;

typedef struct
{
    WCHAR  dmDeviceName[CCHDEVICENAME];
    WORD   dmSpecVersion;
    WORD   dmDriverVersion;
    WORD   dmSize;
    WORD   dmDriverExtra;
    DWORD  dmFields;
    INT16  dmOrientation;
    INT16  dmPaperSize;
    INT16  dmPaperLength;
    INT16  dmPaperWidth;
    INT16  dmScale;
    INT16  dmCopies;
    INT16  dmDefaultSource;
    INT16  dmPrintQuality;
    INT16  dmColor;
    INT16  dmDuplex;
    INT16  dmYResolution;
    INT16  dmTTOption;
    INT16  dmCollate;
    WCHAR  dmFormName[CCHFORMNAME];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
} DEVMODE32W, *LPDEVMODE32W;

DECL_WINELIB_TYPE_AW(DEVMODE);
DECL_WINELIB_TYPE_AW(LPDEVMODE);

typedef struct _SYSTEM_POWER_STATUS
{
  BOOL16  ACLineStatus;
  BYTE    BatteryFlag;
  BYTE    BatteryLifePercent;
  BYTE    reserved;
  DWORD   BatteryLifeTime;
  DWORD   BatteryFullLifeTime;
} SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

/* flags to FormatMessage */
#define	FORMAT_MESSAGE_ALLOCATE_BUFFER	0x00000100
#define	FORMAT_MESSAGE_IGNORE_INSERTS	0x00000200
#define	FORMAT_MESSAGE_FROM_STRING	0x00000400
#define	FORMAT_MESSAGE_FROM_HMODULE	0x00000800
#define	FORMAT_MESSAGE_FROM_SYSTEM	0x00001000
#define	FORMAT_MESSAGE_ARGUMENT_ARRAY	0x00002000
#define	FORMAT_MESSAGE_MAX_WIDTH_MASK	0x000000FF

/* types of LoadImage */
#define IMAGE_BITMAP	0
#define IMAGE_ICON	1
#define IMAGE_CURSOR	2
#define IMAGE_ENHMETA	3

/* loadflags to LoadImage */
#define LR_DEFAULTCOLOR		0x0000
#define LR_MONOCHROME		0x0001
#define LR_COPYRETURNONORG	0x0002
#define LR_COPYDELETEORC	0x0004
#define LR_COPYFROMRESOURCE	0x0008
#define LR_LOADFROMFILE		0x0010
#define LR_LOADREALSIZE		0x0020
#define LR_LOADMAP3DCOLORS	0x1000

typedef struct _LARGE_INTEGER
{
    DWORD    LowPart;
    LONG     HighPart;
} LARGE_INTEGER,*LPLARGE_INTEGER;

typedef struct _ULARGE_INTEGER
{
    DWORD    LowPart;
    DWORD    HighPart;
} ULARGE_INTEGER,*LPULARGE_INTEGER;

/* SetLastErrorEx types */
#define	SLE_ERROR	0x00000001
#define	SLE_MINORERROR	0x00000002
#define	SLE_WARNING	0x00000003

/* Argument 1 passed to the DllEntryProc. */
#define	DLL_PROCESS_DETACH	0	/* detach process (unload library) */
#define	DLL_PROCESS_ATTACH	1	/* attach process (load library) */
#define	DLL_THREAD_ATTACH	2	/* attach new thread */
#define	DLL_THREAD_DETACH	3	/* detach thread */

typedef struct _MEMORY_BASIC_INFORMATION
{
    LPVOID   BaseAddress;
    LPVOID   AllocationBase;
    DWORD    AllocationProtect;
    DWORD    RegionSize;
    DWORD    State;
    DWORD    Protect;
    DWORD    Type;
} MEMORY_BASIC_INFORMATION,*LPMEMORY_BASIC_INFORMATION;

typedef DWORD (*LPTHREAD_START_ROUTINE)(LPVOID);

typedef BOOL32 (*CODEPAGE_ENUMPROC32A)(LPSTR);
typedef BOOL32 (*CODEPAGE_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(CODEPAGE_ENUMPROC);

typedef BOOL32 (*LOCALE_ENUMPROC32A)(LPSTR);
typedef BOOL32 (*LOCALE_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(LOCALE_ENUMPROC);

typedef struct tagSYSTEM_INFO
{
    union {
    	DWORD	dwOemId;
	struct {
		WORD wProcessorArchitecture;
		WORD wReserved;
	} x;
    } u;
    DWORD	dwPageSize;
    LPVOID	lpMinimumApplicationAddress;
    LPVOID	lpMaximumApplicationAddress;
    DWORD	dwActiveProcessorMask;
    DWORD	dwNumberOfProcessors;
    DWORD	dwProcessorType;
    DWORD	dwAllocationGranularity;
    WORD	wProcessorLevel;
    WORD	wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

/* u.x.wProcessorArchitecture (NT) */
#define	PROCESSOR_ARCHITECTURE_INTEL	0
#define	PROCESSOR_ARCHITECTURE_MIPS	1
#define	PROCESSOR_ARCHITECTURE_ALPHA	2
#define	PROCESSOR_ARCHITECTURE_PPC	3
#define	PROCESSOR_ARCHITECTURE_UNKNOWN	0xFFFF

/* dwProcessorType */
#define	PROCESSOR_INTEL_386	386
#define	PROCESSOR_INTEL_486	486
#define	PROCESSOR_INTEL_PENTIUM	586
#define	PROCESSOR_MIPS_R4000	4000
#define	PROCESSOR_ALPHA_21064	21064

/* service main function prototype */
typedef VOID (*LPSERVICE_MAIN_FUNCTION32A)(DWORD,LPSTR);
typedef VOID (*LPSERVICE_MAIN_FUNCTION32W)(DWORD,LPWSTR);
DECL_WINELIB_TYPE_AW(LPSERVICE_MAIN_FUNCTION);

/* service start table */
typedef struct
{
    LPSTR			lpServiceName;
    LPSERVICE_MAIN_FUNCTION32A	lpServiceProc;
} *LPSERVICE_TABLE_ENTRY32A, SERVICE_TABLE_ENTRY32A;

typedef struct
{
    LPWSTR			lpServiceName;
    LPSERVICE_MAIN_FUNCTION32W	lpServiceProc;
} *LPSERVICE_TABLE_ENTRY32W, SERVICE_TABLE_ENTRY32W;

DECL_WINELIB_TYPE_AW(SERVICE_TABLE_ENTRY);
DECL_WINELIB_TYPE_AW(LPSERVICE_TABLE_ENTRY);

/* Security Ids of NT */

typedef struct {
	BYTE	Value[6];
} SID_IDENTIFIER_AUTHORITY,*LPSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
	BYTE	Revision;
	BYTE	SubAuthorityCount;
	SID_IDENTIFIER_AUTHORITY	IdentifierAuthority;
	DWORD	SubAuthority[1];	/* more than one */
} SID,*LPSID;

#define	SID_REVISION			(1)	/* Current revision */
#define	SID_MAX_SUB_AUTHORITIES		(15)	/* current max subauths */
#define	SID_RECOMMENDED_SUB_AUTHORITIES	(1)	/* recommended subauths */

/* NT lowlevel Strings (handled by Rtl* functions in NTDLL)
 * If they are zero terminated, Length does not include the terminating 0.
 */

typedef struct _STRING {
	UINT16	Length;
	UINT16	MaximumLength;
	LPSTR	Buffer;
} STRING,*LPSTRING,ANSI_STRING,*LPANSI_STRING;

typedef struct _CSTRING {
	UINT16	Length;
	UINT16	MaximumLength;
	LPCSTR	Buffer;
} CSTRING,*LPCSTRING;

typedef struct _UNICODE_STRING {
	UINT16	Length;		/* bytes ? */
	UINT16	MaximumLength;	/* bytes ? */
	LPWSTR	Buffer;
} UNICODE_STRING,*LPUNICODE_STRING;

#pragma pack(4)

/* Declarations for functions that exist only in Win16 */

WORD       AllocCStoDSAlias(WORD);
WORD       AllocDStoCSAlias(WORD);
WORD       AllocSelector(WORD);
WORD       AllocSelectorArray(WORD);
LPSTR      AnsiLower(LPSTR);
LPSTR      AnsiUpper(LPSTR);
INT16      Catch(LPCATCHBUF);
WORD       ChangeSelector(WORD,WORD);
INT16      CloseComm(INT16);
BOOL16     DCHook(HDC16,WORD,DWORD,LPARAM);
VOID       DirectedYield(HTASK16);
HGLOBAL16  DirectResAlloc(HINSTANCE16,WORD,UINT16);
BOOL16     DlgDirSelect(HWND16,LPSTR,INT16);
BOOL16     DlgDirSelectComboBox(HWND16,LPSTR,INT16);
BOOL16     EnableHardwareInput(BOOL16);
INT16      ExcludeVisRect(HDC16,INT16,INT16,INT16,INT16);
HANDLE16   FarGetOwner(HGLOBAL16);
VOID       FarSetOwner(HGLOBAL16,HANDLE16);
BOOL16     FastWindowFrame(HDC16,const RECT16*,INT16,INT16,DWORD);
VOID       FillWindow(HWND16,HWND16,HDC16,HBRUSH16);
INT16      FlushComm(INT16,INT16);
WORD       FreeSelector(WORD);
HANDLE16   GetAtomHandle(ATOM);
DWORD      GetBitmapDimension(HBITMAP16);
HANDLE16   GetCodeHandle(FARPROC16);
INT16      GetCommError(INT16,LPCOMSTAT);
UINT16     GetCommEventMask(INT16,UINT16);
VOID       GetCodeInfo(FARPROC16,LPVOID);
HANDLE16   GetCurrentPDB(void);
HTASK16    GetCurrentTask(void);
DWORD      GetDCHook(HDC16,FARPROC16*);
DWORD      GetDCOrg(HDC16);
HWND16     GetDesktopHwnd(void);
HMODULE16  GetExePtr(HANDLE16);
WORD       GetExeVersion(void);
INT16      GetInstanceData(HINSTANCE16,WORD,INT16);
BOOL16     GetModuleName(HINSTANCE16,LPSTR,INT16);
FARPROC16  GetMouseEventProc(void);
UINT16     GetNumTasks(void);
DWORD      GetSelectorBase(WORD);
DWORD      GetSelectorLimit(WORD);
HINSTANCE16 GetTaskDS(void);
HQUEUE16   GetTaskQueue(HTASK16);
DWORD      GlobalDOSAlloc(DWORD);
WORD       GlobalDOSFree(WORD);
void       GlobalFreeAll(HGLOBAL16);
HGLOBAL16  GlobalLRUNewest(HGLOBAL16);
HGLOBAL16  GlobalLRUOldest(HGLOBAL16);
VOID       GlobalNotify(FARPROC16);
WORD       GlobalPageLock(HGLOBAL16);
WORD       GlobalPageUnlock(HGLOBAL16);
INT16      InitApp(HINSTANCE16);
INT16      IntersectVisRect(HDC16,INT16,INT16,INT16,INT16);
BOOL16     IsGDIObject(HGDIOBJ16);
BOOL16     IsSharedSelector(HANDLE16);
BOOL16     IsTask(HTASK16);
HTASK16    IsTaskLocked(void);
BOOL16     LocalInit(HANDLE16,WORD,WORD);
HTASK16    LockCurrentTask(BOOL16);
DWORD      MoveTo(HDC16,INT16,INT16);
DWORD      OffsetViewportOrg(HDC16,INT16,INT16);
INT16      OffsetVisRgn(HDC16,INT16,INT16);
DWORD      OffsetWindowOrg(HDC16,INT16,INT16);
VOID       OldYield(void);
INT16      OpenComm(LPCSTR,UINT16,UINT16);
VOID       PaintRect(HWND16,HWND16,HDC16,HBRUSH16,const RECT16*);
VOID       PostEvent(HTASK16);
WORD       PrestoChangoSelector(WORD,WORD);
INT16      ReadComm(INT16,LPSTR,INT16);
INT16      RestoreVisRgn(HDC16);
HRGN16     SaveVisRgn(HDC16);
DWORD      ScaleViewportExt(HDC16,INT16,INT16,INT16,INT16);
DWORD      ScaleWindowExt(HDC16,INT16,INT16,INT16,INT16);
WORD       SelectorAccessRights(WORD,WORD,WORD);
INT16      SelectVisRgn(HDC16,HRGN16);
DWORD      SetBitmapDimension(HBITMAP16,INT16,INT16);
DWORD      SetBrushOrg(HDC16,INT16,INT16);
UINT16*    SetCommEventMask(INT16,UINT16);
BOOL16     SetDCHook(HDC16,FARPROC16,DWORD);
BOOL16     SetDeskPattern(void);
VOID       SetPriority(HTASK16,INT16);
WORD       SetSelectorBase(WORD,DWORD);
WORD       SetSelectorLimit(WORD,DWORD);
HQUEUE16   SetTaskQueue(HTASK16,HQUEUE16);
FARPROC16  SetTaskSignalProc(HTASK16,FARPROC16);
DWORD      SetViewportExt(HDC16,INT16,INT16);
DWORD      SetViewportOrg(HDC16,INT16,INT16);
DWORD      SetWindowExt(HDC16,INT16,INT16);
DWORD      SetWindowOrg(HDC16,INT16,INT16);
VOID       SwitchStackBack(void);
VOID       SwitchStackTo(WORD,WORD,WORD);
INT16      Throw(LPCATCHBUF,INT16);
INT16      UngetCommChar(INT16,CHAR);
VOID       UserYield(void);
BOOL16     WaitEvent(HTASK16);
INT16      WriteComm(INT16,LPSTR,INT16);
VOID       WriteOutProfiles(VOID);
VOID       hmemcpy(LPVOID,LPCVOID,LONG);
VOID       Yield(void);

/* Declarations for functions that exist only in Win32 */

BOOL32     Beep(DWORD,DWORD);
BOOL32     ClearCommError(INT32,LPDWORD,LPCOMSTAT);
HFILE      CreateFile32A(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE32);
HFILE      CreateFile32W(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE32);
#define    CreateFile WINELIB_NAME_AW(CreateFile)
HANDLE32   CreateFileMapping32A(HANDLE32,LPSECURITY_ATTRIBUTES,DWORD,DWORD,DWORD,LPCSTR);
HANDLE32   CreateFileMapping32W(HANDLE32,LPSECURITY_ATTRIBUTES,DWORD,DWORD,DWORD,LPCWSTR);
#define    CreateFileMapping WINELIB_NAME_AW(CreateFileMapping)
BOOL32     DosDateTimeToFileTime(WORD,WORD,LPFILETIME);
INT32      EnumPropsEx32A(HWND32,PROPENUMPROCEX32A,LPARAM);
INT32      EnumPropsEx32W(HWND32,PROPENUMPROCEX32W,LPARAM);
#define    EnumPropsEx WINELIB_NAME_AW(EnumPropsEx)
BOOL32     EnumSystemCodePages32A(CODEPAGE_ENUMPROC32A,DWORD);
BOOL32     EnumSystemCodePages32W(CODEPAGE_ENUMPROC32W,DWORD);
#define    EnumSystemCodePages WINELIB_NAME_AW(EnumSystemCodePages)
BOOL32     EnumSystemLocales32A(LOCALE_ENUMPROC32A,DWORD);
BOOL32     EnumSystemLocales32W(LOCALE_ENUMPROC32W,DWORD);
#define    EnumSystemLocales WINELIB_NAME_AW(EnumSystemLocales)
BOOL32     EnumThreadWindows(DWORD,WNDENUMPROC32,LPARAM);
void       ExitProcess(DWORD);
BOOL32     FileTimeToDosDateTime(LPFILETIME,LPWORD,LPWORD);
BOOL32     FileTimeToLocalFileTime(LPFILETIME,LPFILETIME);
BOOL32     FileTimeToSystemTime(LPFILETIME,LPSYSTEMTIME);
HRSRC32    FindResourceEx32A(HINSTANCE32,LPCSTR,LPCSTR,WORD);
HRSRC32    FindResourceEx32W(HINSTANCE32,LPCWSTR,LPCWSTR,WORD);
#define    FindResourceEx WINELIB_NAME_AW(FindResourceEx)
BOOL32     FlushFileBuffers(HFILE);
UINT32     GetACP(void);
LPCSTR     GetCommandLine32A();
LPCWSTR    GetCommandLine32W();
#define    GetCommandLine WINELIB_NAME_AW(GetCommandLine)
BOOL32     GetCommTimeouts(INT32,LPCOMMTIMEOUTS);
DWORD      GetCurrentThreadId(void);
HANDLE32   GetCurrentThread(void);
BOOL32     GetDCOrgEx(HDC32,LPPOINT32);
DWORD      GetFileInformationByHandle(HFILE,BY_HANDLE_FILE_INFORMATION*);
DWORD      GetFileSize(HFILE,LPDWORD);
DWORD      GetFileType(HFILE);
VOID       GetLocalTime(LPSYSTEMTIME);
DWORD      GetLogicalDrives(void);
UINT32     GetOEMCP(void);
HANDLE32   GetProcessHeap(void);
DWORD      GetShortPathName32A(LPCSTR,LPSTR,DWORD);
DWORD      GetShortPathName32W(LPCWSTR,LPWSTR,DWORD);
#define    GetShortPathName WINELIB_NAME_AW(GetShortPathName)
HFILE      GetStdHandle(DWORD);
VOID       GetSystemInfo(LPSYSTEM_INFO);
BOOL32     GetSystemPowerStatus(LPSYSTEM_POWER_STATUS);
VOID       GetSystemTime(LPSYSTEMTIME);
VOID       GlobalMemoryStatus(LPMEMORYSTATUS);
LPVOID     HeapAlloc(HANDLE32,DWORD,DWORD);
DWORD      HeapCompact(HANDLE32,DWORD);
HANDLE32   HeapCreate(DWORD,DWORD,DWORD);
BOOL32     HeapDestroy(HANDLE32);
BOOL32     HeapFree(HANDLE32,DWORD,LPVOID);
BOOL32     HeapLock(HANDLE32);
LPVOID     HeapReAlloc(HANDLE32,DWORD,LPVOID,DWORD);
DWORD      HeapSize(HANDLE32,DWORD,LPVOID);
BOOL32     HeapUnlock(HANDLE32);
BOOL32     HeapValidate(HANDLE32,DWORD,LPVOID);
BOOL32     IsWindowUnicode(HWND32);
BOOL32     IsValidLocale(DWORD,DWORD);
BOOL32     LocalFileTimeToFileTime(LPFILETIME,LPFILETIME);
LPVOID     MapViewOfFileEx(HANDLE32,DWORD,DWORD,DWORD,DWORD,DWORD);
BOOL32     MoveFile32A(LPCSTR,LPCSTR);
BOOL32     MoveFile32W(LPCWSTR,LPCWSTR);
#define    MoveFile WINELIB_NAME_AW(MoveFile)
BOOL32     QueryPerformanceCounter(LPLARGE_INTEGER);
BOOL32     ReadConsole32A(HANDLE32,LPVOID,DWORD,LPDWORD,LPVOID);
BOOL32     ReadConsole32W(HANDLE32,LPVOID,DWORD,LPDWORD,LPVOID);
#define    ReadConsole WINELIB_NAME_AW(ReadConsole)
BOOL32     ReadFile(HFILE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
DWORD      RegCreateKeyEx32A(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,
                             LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
DWORD      RegCreateKeyEx32W(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,
                             LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
#define    RegCreateKeyEx WINELIB_NAME_AW(RegCreateKeyEx)
DWORD      RegEnumKeyEx32A(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPSTR,LPDWORD,LPFILETIME);
DWORD      RegEnumKeyEx32W(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPWSTR,LPDWORD,LPFILETIME);
#define    RegEnumKeyEx WINELIB_NAME_AW(RegEnumKeyEx)
DWORD      RegOpenKeyEx32W(HKEY,LPCWSTR,DWORD,REGSAM,LPHKEY);
DWORD      RegOpenKeyEx32A(HKEY,LPCSTR,DWORD,REGSAM,LPHKEY);
#define    RegOpenKeyEx WINELIB_NAME_AW(RegOpenKeyEx)
DWORD      RegQueryInfoKey32W(HKEY,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                            LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPFILETIME);
DWORD      RegQueryInfoKey32A(HKEY,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                            LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPFILETIME);
#define    RegQueryInfoKey WINELIB_NAME_AW(RegQueryInfoKey)
VOID       RtlFillMemory(LPVOID,UINT32,UINT32);
VOID       RtlMoveMemory(LPVOID,LPCVOID,UINT32);
VOID       RtlZeroMemory(LPVOID,UINT32);
DWORD      SearchPath32A(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD      SearchPath32W(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,LPWSTR,LPWSTR*);
#define    SearchPath WINELIB_NAME(SearchPath)
BOOL32     SetBrushOrgEx(HDC32,INT32,INT32,LPPOINT32);
BOOL32     SetCommMask(INT32,DWORD);
BOOL32     SetCommTimeouts(INT32,LPCOMMTIMEOUTS);
BOOL32     SetConsoleTitle32A(LPCSTR);
BOOL32     SetConsoleTitle32W(LPCWSTR);
#define    SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
BOOL32     SetEndOfFile(HFILE);
DWORD      SetFilePointer(HFILE,LONG,LPLONG,DWORD);
BOOL32     SetFileTime(HFILE,LPFILETIME,LPFILETIME,LPFILETIME);
BOOL32     SetTimeZoneInformation(const LPTIME_ZONE_INFORMATION);
BOOL32     SetSystemPowerState(BOOL32,BOOL32);
BOOL32     SetSystemTime(const SYSTEMTIME*);
VOID       Sleep(DWORD);
BOOL32     SystemTimeToFileTime(LPSYSTEMTIME,LPFILETIME);
LPVOID     VirtualAlloc(LPVOID,DWORD,DWORD,DWORD);
BOOL32     VirtualFree(LPVOID,DWORD,DWORD);
BOOL32     WriteConsole32A(HANDLE32,LPVOID,DWORD,LPDWORD,LPVOID);
BOOL32     WriteConsole32W(HANDLE32,LPVOID,DWORD,LPDWORD,LPVOID);
#define    WriteConsole WINELIB_NAME_AW(WriteConsole)
BOOL32     WriteFile(HFILE,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);


/* Declarations for functions that are the same in Win16 and Win32 */

BOOL16     CheckDlgButton(HWND32,INT32,UINT32);
BOOL16     CheckRadioButton(HWND32,UINT32,UINT32,UINT32);
LONG       CopyLZFile(HFILE,HFILE);
HBITMAP16  CreateBitmap(INT32,INT32,UINT32,UINT32,LPCVOID);
BOOL16     CreateCaret(HWND32,HBITMAP32,INT32,INT32);
HBITMAP16  CreateCompatibleBitmap(HDC32,INT32,INT32);
HBITMAP16  CreateDiscardableBitmap(HDC32,INT32,INT32);
BOOL16     DestroyCaret(void);
BOOL16     EndDialog(HWND32,INT32);
INT16      ExcludeUpdateRgn(HDC32,HWND32);
DWORD      GetAppCompatFlags(HTASK32);
LONG       GetBitmapBits(HBITMAP32,LONG,LPVOID);
WORD       GetClassWord(HWND32,INT32);
INT16      GetDlgCtrlID(HWND32);
DWORD      GetLastError(void);
COLORREF   GetSysColor(INT32);
DWORD      GetTickCount(void);
INT16      GetUpdateRgn(HWND32,HRGN32,BOOL32);
WORD       GetWindowWord(HWND32,INT32);
BOOL16     HideCaret(HWND32);
BOOL16     IsWindow(HWND32);
void       LZClose(HFILE);
LONG       LZCopy(HFILE,HFILE);
void       LZDone(void);
HFILE      LZInit(HFILE);
LONG       LZSeek(HFILE,LONG,INT32);
INT16      LZStart(void);
HFILE      OpenFile(LPCSTR,OFSTRUCT*,UINT32);
UINT16     RealizePalette(HDC32);
DWORD      RegCloseKey(HKEY);
DWORD      RegFlushKey(HKEY);
VOID       ReleaseCapture(void);
LONG       SetBitmapBits(HBITMAP32,LONG,LPCVOID);
COLORREF   SetBkColor(HDC32,COLORREF);
BOOL16     SetCaretBlinkTime(UINT32);
BOOL16     SetCaretPos(INT32,INT32);
WORD       SetClassWord(HWND32,INT32,WORD);
INT16      SetDIBits(HDC32,HBITMAP32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
INT16      SetDIBitsToDevice(HDC32,INT32,INT32,DWORD,DWORD,INT32,INT32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
VOID       SetLastError(DWORD);
VOID       SetLastErrorEx(DWORD,DWORD);
VOID       SetRectRgn(HRGN32,INT32,INT32,INT32,INT32);
COLORREF   SetTextColor(HDC32,COLORREF);
WORD       SetWindowWord(HWND32,INT32,WORD);
BOOL16     ShowCaret(HWND32);
VOID       UpdateWindow(HWND32);
LONG       _hread(HFILE,LPVOID,LONG);
HFILE      _lclose(HFILE);
HFILE      _lcreat(LPCSTR,INT32);
LONG       _llseek(HFILE,LONG,INT32);
HFILE      _lopen(LPCSTR,INT32);
LONG       _hwrite(HFILE,LPCSTR,LONG);

/* Declarations for functions that change between Win16 and Win32 */

INT16      AccessResource16(HINSTANCE16,HRSRC16);
INT32      AccessResource32(HINSTANCE32,HRSRC32);
#define    AccessResource WINELIB_NAME(AccessResource)
BOOL16     AdjustWindowRect16(LPRECT16,DWORD,BOOL16);
BOOL32     AdjustWindowRect32(LPRECT32,DWORD,BOOL32);
#define    AdjustWindowRect WINELIB_NAME(AdjustWindowRect)
BOOL16     AdjustWindowRectEx16(LPRECT16,DWORD,BOOL16,DWORD);
BOOL32     AdjustWindowRectEx32(LPRECT32,DWORD,BOOL32,DWORD);
#define    AdjustWindowRectEx WINELIB_NAME(AdjustWindowRectEx)
HGLOBAL16  AllocResource16(HINSTANCE16,HRSRC16,DWORD);
HGLOBAL32  AllocResource32(HINSTANCE32,HRSRC32,DWORD);
#define    AllocResource WINELIB_NAME(AllocResource)
BOOL16     AppendMenu16(HMENU16,UINT16,UINT16,SEGPTR);
BOOL32     AppendMenu32A(HMENU32,UINT32,UINT32,LPCSTR);
BOOL32     AppendMenu32W(HMENU32,UINT32,UINT32,LPCWSTR);
#define    AppendMenu WINELIB_NAME_AW(AppendMenu)
BOOL16     Arc16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32     Arc32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define    Arc WINELIB_NAME(Arc)
HDC16      BeginPaint16(HWND16,LPPAINTSTRUCT16);
HDC32      BeginPaint32(HWND32,LPPAINTSTRUCT32);
#define    BeginPaint WINELIB_NAME(BeginPaint)
BOOL16     BitBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,INT16,DWORD);
BOOL32     BitBlt32(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,INT32,DWORD);
#define    BitBlt WINELIB_NAME(BitBlt)
BOOL16     BuildCommDCB16(LPCSTR,LPDCB16);
BOOL32     BuildCommDCB32A(LPCSTR,LPDCB32);
BOOL32     BuildCommDCB32W(LPCWSTR,LPDCB32);
#define    BuildCommDCB WINELIB_NAME_AW(BuildCommDCB)
BOOL32     BuildCommDCBAndTimeouts32A(LPCSTR,LPDCB32,LPCOMMTIMEOUTS);
BOOL32     BuildCommDCBAndTimeouts32W(LPCWSTR,LPDCB32,LPCOMMTIMEOUTS);
#define    BuildCommDCBAndTimeouts WINELIB_NAME_AW(BuildCommDCBAndTimeouts)
LRESULT    CallNextHookEx16(HHOOK,INT16,WPARAM16,LPARAM);
LRESULT    CallNextHookEx32(HHOOK,INT32,WPARAM32,LPARAM);
#define    CallNextHookEx WINELIB_NAME(CallNextHookEx)
LRESULT    CallWindowProc16(WNDPROC16,HWND16,UINT16,WPARAM16,LPARAM);
LRESULT    CallWindowProc32A(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT    CallWindowProc32W(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
#define    CallWindowProc WINELIB_NAME_AW(CallWindowProc)
BOOL16     ChangeMenu16(HMENU16,UINT16,SEGPTR,UINT16,UINT16);
BOOL32     ChangeMenu32A(HMENU32,UINT32,LPCSTR,UINT32,UINT32);
BOOL32     ChangeMenu32W(HMENU32,UINT32,LPCWSTR,UINT32,UINT32);
#define    ChangeMenu WINELIB_NAME_AW(ChangeMenu)
LPSTR      CharLower32A(LPSTR);
LPWSTR     CharLower32W(LPWSTR);
#define    CharLower WINELIB_NAME_AW(CharLower)
DWORD      CharLowerBuff32A(LPSTR,DWORD);
DWORD      CharLowerBuff32W(LPWSTR,DWORD);
#define    CharLowerBuff WINELIB_NAME_AW(CharLowerBuff)
LPSTR      CharNext32A(LPCSTR);
LPWSTR     CharNext32W(LPCWSTR);
#define    CharNext WINELIB_NAME_AW(CharNext)
LPSTR      CharNextEx32A(WORD,LPCSTR,DWORD);
LPWSTR     CharNextEx32W(WORD,LPCWSTR,DWORD);
#define    CharNextEx WINELIB_NAME_AW(CharNextEx)
LPSTR      CharPrev32A(LPCSTR,LPCSTR);
LPWSTR     CharPrev32W(LPCWSTR,LPCWSTR);
#define    CharPrev WINELIB_NAME_AW(CharPrev)
LPSTR      CharPrevEx32A(WORD,LPCSTR,LPCSTR,DWORD);
LPWSTR     CharPrevEx32W(WORD,LPCWSTR,LPCWSTR,DWORD);
#define    CharPrevEx WINELIB_NAME_AW(CharPrevEx)
LPSTR      CharUpper32A(LPSTR);
LPWSTR     CharUpper32W(LPWSTR);
#define    CharUpper WINELIB_NAME_AW(CharUpper)
DWORD      CharUpperBuff32A(LPSTR,DWORD);
DWORD      CharUpperBuff32W(LPWSTR,DWORD);
#define    CharUpperBuff WINELIB_NAME_AW(CharUpperBuff)
BOOL32     CharToOem32A(LPSTR,LPSTR);
BOOL32     CharToOem32W(LPCWSTR,LPSTR);
#define    CharToOem WINELIB_NAME_AW(CharToOem)
BOOL32     CharToOemBuff32A(LPSTR,LPSTR,DWORD);
BOOL32     CharToOemBuff32W(LPCWSTR,LPSTR,DWORD);
#define    CharToOemBuff WINELIB_NAME_AW(CharToOemBuff)
HWND16     ChildWindowFromPoint16(HWND16,POINT16);
HWND32     ChildWindowFromPoint32(HWND32,POINT32);
#define    ChildWindowFromPoint WINELIB_NAME(ChildWindowFromPoint)
BOOL16     Chord16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32     Chord32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define    Chord WINELIB_NAME(Chord)
INT16      ClearCommBreak16(INT16);
BOOL32     ClearCommBreak32(INT32);
#define    ClearCommBreak WINELIB_NAME(ClearCommBreak)
BOOL16     ClientToScreen16(HWND16,LPPOINT16);
BOOL32     ClientToScreen32(HWND32,LPPOINT32);
#define    ClientToScreen WINELIB_NAME(ClientToScreen)
BOOL16     ClipCursor16(const RECT16*);
BOOL32     ClipCursor32(const RECT32*);
#define    ClipCursor WINELIB_NAME(ClipCursor)
HMETAFILE16 CloseMetaFile16(HDC16);
HMETAFILE32 CloseMetaFile32(HDC32);
#define    CloseMetaFile WINELIB_NAME(CloseMetaFile)
INT16      CombineRgn16(HRGN16,HRGN16,HRGN16,INT16);
INT32      CombineRgn32(HRGN32,HRGN32,HRGN32,INT32);
#define    CombineRgn WINELIB_NAME(CombineRgn)
UINT16     CompareString16(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT32     CompareString32A(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT32     CompareString32W(DWORD,DWORD,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define    CompareString WINELIB_NAME_AW(CompareString)
HCURSOR16  CopyCursor16(HINSTANCE16,HCURSOR16);
#define    CopyCursor32(cur) ((HCURSOR32)CopyIcon32((HICON32)(cur)))
#define    CopyCursor WINELIB_NAME(CopyCursor)
HICON16    CopyIcon16(HINSTANCE16,HICON16);
HICON32    CopyIcon32(HICON32);
#define    CopyIcon WINELIB_NAME(CopyIcon)
BOOL16     CopyRect16(RECT16*,const RECT16*);
BOOL32     CopyRect32(RECT32*,const RECT32*);
#define    CopyRect WINELIB_NAME(CopyRect)
HBITMAP16  CreateBitmapIndirect16(const BITMAP16*);
HBITMAP32  CreateBitmapIndirect32(const BITMAP32*);
#define    CreateBitmapIndirect WINELIB_NAME(CreateBitmapIndirect)
HBRUSH16   CreateBrushIndirect16(const LOGBRUSH16*);
HBRUSH32   CreateBrushIndirect32(const LOGBRUSH32*);
#define    CreateBrushIndirect WINELIB_NAME(CreateBrushIndirect)
HDC16      CreateDC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC32      CreateDC32A(LPCSTR,LPCSTR,LPCSTR,const DEVMODE32A*);
HDC32      CreateDC32W(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODE32W*);
#define    CreateDC WINELIB_NAME_AW(CreateDC)
HWND16     CreateDialog16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16);
#define    CreateDialog32A(inst,ptr,hwnd,dlg) \
           CreateDialogParam32A(inst,ptr,hwnd,dlg,0)
#define    CreateDialog32W(inst,ptr,hwnd,dlg) \
           CreateDialogParam32W(inst,ptr,hwnd,dlg,0)
#define    CreateDialog WINELIB_NAME_AW(CreateDialog)
HWND16     CreateDialogIndirect16(HINSTANCE16,LPCVOID,HWND16,DLGPROC16);
#define    CreateDialogIndirect32A(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32A(inst,ptr,hwnd,dlg,0)
#define    CreateDialogIndirect32W(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32W(inst,ptr,hwnd,dlg,0)
#define    CreateDialogIndirect WINELIB_NAME_AW(CreateDialogIndirect)
HWND16     CreateDialogIndirectParam16(HINSTANCE16,LPCVOID,HWND16,DLGPROC16,LPARAM);
HWND32     CreateDialogIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
HWND32     CreateDialogIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
#define    CreateDialogIndirectParam WINELIB_NAME_AW(CreateDialogIndirectParam)
HWND16     CreateDialogParam16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16,LPARAM);
HWND32     CreateDialogParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
HWND32     CreateDialogParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define    CreateDialogParam WINELIB_NAME_AW(CreateDialogParam)
HBRUSH16   CreateDIBPatternBrush16(HGLOBAL16,UINT16);
HBRUSH32   CreateDIBPatternBrush32(HGLOBAL32,UINT32);
#define    CreateDIBPatternBrush WINELIB_NAME(CreateDIBPatternBrush)
BOOL16     CreateDirectory16(LPCSTR,LPVOID);
BOOL32     CreateDirectory32A(LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL32     CreateDirectory32W(LPCWSTR,LPSECURITY_ATTRIBUTES);
#define    CreateDirectory WINELIB_NAME_AW(CreateDirectory)
BOOL32     CreateDirectoryEx32A(LPCSTR,LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL32     CreateDirectoryEx32W(LPCWSTR,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define    CreateDirectoryEx WINELIB_NAME_AW(CreateDirectoryEx)
HRGN16     CreateEllipticRgn16(INT16,INT16,INT16,INT16);
HRGN32     CreateEllipticRgn32(INT32,INT32,INT32,INT32);
#define    CreateEllipticRgn WINELIB_NAME(CreateEllipticRgn)
HRGN16     CreateEllipticRgnIndirect16(const RECT16 *);
HRGN32     CreateEllipticRgnIndirect32(const RECT32 *);
#define    CreateEllipticRgnIndirect WINELIB_NAME(CreateEllipticRgnIndirect)
HFONT16    CreateFont16(INT16,INT16,INT16,INT16,INT16,BYTE,BYTE,BYTE,BYTE,BYTE,BYTE,BYTE,BYTE,LPCSTR);
HFONT32    CreateFont32A(INT32,INT32,INT32,INT32,INT32,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR);
HFONT32    CreateFont32W(INT32,INT32,INT32,INT32,INT32,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR);
#define    CreateFont WINELIB_NAME_AW(CreateFont)
HFONT16    CreateFontIndirect16(const LOGFONT16*);
HFONT32    CreateFontIndirect32A(const LOGFONT32A*);
HFONT32    CreateFontIndirect32W(const LOGFONT32W*);
#define    CreateFontIndirect WINELIB_NAME_AW(CreateFontIndirect)
HBRUSH16   CreateHatchBrush16(INT16,COLORREF);
HBRUSH32   CreateHatchBrush32(INT32,COLORREF);
#define    CreateHatchBrush WINELIB_NAME(CreateHatchBrush)
HDC16      CreateMetaFile16(LPCSTR);
HDC32      CreateMetaFile32A(LPCSTR);
HDC32      CreateMetaFile32W(LPCWSTR);
#define    CreateMetaFile WINELIB_NAME_AW(CreateMetaFile)
HBRUSH16   CreatePatternBrush16(HBITMAP16);
HBRUSH32   CreatePatternBrush32(HBITMAP32);
#define    CreatePatternBrush WINELIB_NAME(CreatePatternBrush)
HPEN16     CreatePen16(INT16,INT16,COLORREF);
HPEN32     CreatePen32(INT32,INT32,COLORREF);
#define    CreatePen WINELIB_NAME(CreatePen)
HPEN16     CreatePenIndirect16(const LOGPEN16*);
HPEN32     CreatePenIndirect32(const LOGPEN32*);
#define    CreatePenIndirect WINELIB_NAME(CreatePenIndirect)
HRGN16     CreatePolyPolygonRgn16(const POINT16*,const INT16*,INT16,INT16);
HRGN32     CreatePolyPolygonRgn32(const POINT32*,const INT32*,INT32,INT32);
#define    CreatePolyPolygonRgn WINELIB_NAME(CreatePolyPolygonRgn)
HRGN16     CreatePolygonRgn16(const POINT16*,INT16,INT16);
HRGN32     CreatePolygonRgn32(const POINT32*,INT32,INT32);
#define    CreatePolygonRgn WINELIB_NAME(CreatePolygonRgn)
HRGN16     CreateRectRgn16(INT16,INT16,INT16,INT16);
HRGN32     CreateRectRgn32(INT32,INT32,INT32,INT32);
#define    CreateRectRgn WINELIB_NAME(CreateRectRgn)
HRGN16     CreateRectRgnIndirect16(const RECT16*);
HRGN32     CreateRectRgnIndirect32(const RECT32*);
#define    CreateRectRgnIndirect WINELIB_NAME(CreateRectRgnIndirect)
HRGN16     CreateRoundRectRgn16(INT16,INT16,INT16,INT16,INT16,INT16);
HRGN32     CreateRoundRectRgn32(INT32,INT32,INT32,INT32,INT32,INT32);
#define    CreateRoundRectRgn WINELIB_NAME(CreateRoundRectRgn)
HBRUSH16   CreateSolidBrush16(COLORREF);
HBRUSH32   CreateSolidBrush32(COLORREF);
#define    CreateSolidBrush WINELIB_NAME(CreateSolidBrush)
HWND16     CreateWindow16(LPCSTR,LPCSTR,DWORD,INT16,INT16,INT16,INT16,HWND16,HMENU16,HINSTANCE16,LPVOID);
#define    CreateWindow32A(className,titleName,style,x,y,width,height,\
                           parent,menu,instance,param) \
           CreateWindowEx32A(0,className,titleName,style,x,y,width,height,\
                           parent,menu,instance,param)
#define    CreateWindow32W(className,titleName,style,x,y,width,height,\
                           parent,menu,instance,param) \
           CreateWindowEx32W(0,className,titleName,style,x,y,width,height,\
                           parent,menu,instance,param)
#define    CreateWindow WINELIB_NAME_AW(CreateWindow)
HWND16     CreateWindowEx16(DWORD,LPCSTR,LPCSTR,DWORD,INT16,INT16,INT16,INT16,HWND16,HMENU16,HINSTANCE16,LPVOID);
HWND32     CreateWindowEx32A(DWORD,LPCSTR,LPCSTR,DWORD,INT32,INT32,INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
HWND32     CreateWindowEx32W(DWORD,LPCWSTR,LPCWSTR,DWORD,INT32,INT32,INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
#define    CreateWindowEx WINELIB_NAME_AW(CreateWindowEx)
LRESULT    DefDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT    DefDlgProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT    DefDlgProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define    DefDlgProc WINELIB_NAME_AW(DefDlgProc)
LRESULT    DefFrameProc16(HWND16,HWND16,UINT16,WPARAM16,LPARAM);
LRESULT    DefFrameProc32A(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT    DefFrameProc32W(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
#define    DefFrameProc WINELIB_NAME_AW(DefFrameProc)
LRESULT    DefHookProc16(INT16,WPARAM16,LPARAM,HHOOK*);
#define    DefHookProc32(code,wparam,lparam,phhook) \
           CallNextHookEx32(*(phhook),code,wparam,lparam)
#define    DefHookProc WINELIB_NAME(DefHookProc)
LRESULT    DefMDIChildProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT    DefMDIChildProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT    DefMDIChildProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define    DefMDIChildProc WINELIB_NAME_AW(DefMDIChildProc)
LRESULT    DefWindowProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT    DefWindowProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT    DefWindowProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define    DefWindowProc WINELIB_NAME_AW(DefWindowProc)
BOOL16     DeleteFile16(LPCSTR);
BOOL32     DeleteFile32A(LPCSTR);
BOOL32     DeleteFile32W(LPCWSTR);
#define    DeleteFile WINELIB_NAME_AW(DeleteFile)
BOOL16     DeleteMetaFile16(HMETAFILE16);
BOOL32     DeleteMetaFile32(HMETAFILE32);
#define    DeleteMetaFile WINELIB_NAME(DeleteMetaFile)
BOOL16     DeleteObject16(HGDIOBJ16);
BOOL32     DeleteObject32(HGDIOBJ32);
#define    DeleteObject WINELIB_NAME(DeleteObject)
INT16      DialogBox16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16);
#define    DialogBox32A(inst,template,owner,func) \
           DialogBoxParam32A(inst,template,owner,func,0)
#define    DialogBox32W(inst,template,owner,func) \
           DialogBoxParam32W(inst,template,owner,func,0)
#define    DialogBox WINELIB_NAME_AW(DialogBox)
INT16      DialogBoxIndirect16(HINSTANCE16,HANDLE16,HWND16,DLGPROC16);
#define    DialogBoxIndirect32A(inst,template,owner,func) \
           DialogBoxIndirectParam32A(inst,template,owner,func,0)
#define    DialogBoxIndirect32W(inst,template,owner,func) \
           DialogBoxIndirectParam32W(inst,template,owner,func,0)
#define    DialogBoxIndirect WINELIB_NAME_AW(DialogBoxIndirect)
INT16      DialogBoxIndirectParam16(HINSTANCE16,HANDLE16,HWND16,DLGPROC16,LPARAM);
INT32      DialogBoxIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
INT32      DialogBoxIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
#define    DialogBoxIndirectParam WINELIB_NAME_AW(DialogBoxIndirectParam)
INT16      DialogBoxParam16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16,LPARAM);
INT32      DialogBoxParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
INT32      DialogBoxParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define    DialogBoxParam WINELIB_NAME_AW(DialogBoxParam)
INT16      DlgDirList16(HWND16,LPCSTR,INT16,INT16,UINT16);
INT32      DlgDirList32A(HWND32,LPCSTR,INT32,INT32,UINT32);
INT32      DlgDirList32W(HWND32,LPCWSTR,INT32,INT32,UINT32);
#define    DlgDirList WINELIB_NAME_AW(DlgDirList)
INT16      DlgDirListComboBox16(HWND16,LPCSTR,INT16,INT16,UINT16);
INT32      DlgDirListComboBox32A(HWND32,LPCSTR,INT32,INT32,UINT32);
INT32      DlgDirListComboBox32W(HWND32,LPCWSTR,INT32,INT32,UINT32);
#define    DlgDirListComboBox WINELIB_NAME_AW(DlgDirListComboBox)
BOOL16     DlgDirSelectComboBoxEx16(HWND16,LPSTR,INT16,INT16);
BOOL32     DlgDirSelectComboBoxEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32     DlgDirSelectComboBoxEx32W(HWND32,LPWSTR,INT32,INT32);
#define    DlgDirSelectComboBoxEx WINELIB_NAME_AW(DlgDirSelectComboBoxEx)
BOOL16     DlgDirSelectEx16(HWND16,LPSTR,INT16,INT16);
BOOL32     DlgDirSelectEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32     DlgDirSelectEx32W(HWND32,LPWSTR,INT32,INT32);
#define    DlgDirSelectEx WINELIB_NAME_AW(DlgDirSelectEx)
BOOL16     DPtoLP16(HDC16,LPPOINT16,INT16);
BOOL32     DPtoLP32(HDC32,LPPOINT32,INT32);
#define    DPtoLP WINELIB_NAME(DPtoLP)
BOOL16     DrawEdge16(HDC16,LPRECT16,UINT16,UINT16);
BOOL32     DrawEdge32(HDC32,LPRECT32,UINT32,UINT32);
#define    DrawEdge WINELIB_NAME(DrawEdge)
void       DrawFocusRect16(HDC16,const RECT16*);
void       DrawFocusRect32(HDC32,const RECT32*);
#define    DrawFocusRect WINELIB_NAME(DrawFocusRect)
BOOL16     DrawFrameControl16(HDC16,LPRECT16,UINT16,UINT16);
BOOL32     DrawFrameControl32(HDC32,LPRECT32,UINT32,UINT32);
#define    DrawFrameControl WINELIB_NAME(DrawFrameControl)
INT16      DrawText16(HDC16,LPCSTR,INT16,LPRECT16,UINT16);
INT32      DrawText32A(HDC32,LPCSTR,INT32,LPRECT32,UINT32);
INT32      DrawText32W(HDC32,LPCWSTR,INT32,LPRECT32,UINT32);
#define    DrawText WINELIB_NAME_AW(DrawText)
BOOL16     Ellipse16(HDC16,INT16,INT16,INT16,INT16);
BOOL32     Ellipse32(HDC32,INT32,INT32,INT32,INT32);
#define    Ellipse WINELIB_NAME(Ellipse)
BOOL16     EnableScrollBar16(HWND16,INT16,UINT16);
BOOL32     EnableScrollBar32(HWND32,INT32,UINT32);
#define    EnableScrollBar WINELIB_NAME(EnableScrollBar)
BOOL16     EndPaint16(HWND16,const PAINTSTRUCT16*);
BOOL32     EndPaint32(HWND32,const PAINTSTRUCT32*);
#define    EndPaint WINELIB_NAME(EndPaint)
BOOL16     EnumChildWindows16(HWND16,WNDENUMPROC16,LPARAM);
BOOL32     EnumChildWindows32(HWND32,WNDENUMPROC32,LPARAM);
#define    EnumChildWindows WINELIB_NAME(EnumChildWindows)
INT16      EnumFontFamilies16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT32      EnumFontFamilies32A(HDC32,LPCSTR,FONTENUMPROC32A,LPARAM);
INT32      EnumFontFamilies32W(HDC32,LPCWSTR,FONTENUMPROC32W,LPARAM);
#define    EnumFontFamilies WINELIB_NAME_AW(EnumFontFamilies)
INT16      EnumFontFamiliesEx16(HDC16,LPLOGFONT16,FONTENUMPROCEX16,LPARAM,DWORD);
INT32      EnumFontFamiliesEx32A(HDC32,LPLOGFONT32A,FONTENUMPROCEX32A,LPARAM,DWORD);
INT32      EnumFontFamiliesEx32W(HDC32,LPLOGFONT32W,FONTENUMPROCEX32W,LPARAM,DWORD);
#define    EnumFontFamiliesEx WINELIB_NAME_AW(EnumFontFamiliesEx)
INT16      EnumFonts16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT32      EnumFonts32A(HDC32,LPCSTR,FONTENUMPROC32A,LPARAM);
INT32      EnumFonts32W(HDC32,LPCWSTR,FONTENUMPROC32W,LPARAM);
#define    EnumFonts WINELIB_NAME_AW(EnumFonts)
INT16      EnumObjects16(HDC16,INT16,GOBJENUMPROC16,LPARAM);
INT32      EnumObjects32(HDC32,INT32,GOBJENUMPROC32,LPARAM);
#define    EnumObjects WINELIB_NAME(EnumObjects)
INT16      EnumProps16(HWND16,PROPENUMPROC16);
INT32      EnumProps32A(HWND32,PROPENUMPROC32A);
INT32      EnumProps32W(HWND32,PROPENUMPROC32W);
#define    EnumProps WINELIB_NAME_AW(EnumProps)
BOOL16     EnumTaskWindows16(HTASK16,WNDENUMPROC16,LPARAM);
#define    EnumTaskWindows32(handle,proc,lparam) \
           EnumThreadWindows(handle,proc,lparam)
#define    EnumTaskWindows WINELIB_NAME(EnumTaskWindows)
BOOL16     EnumWindows16(WNDENUMPROC16,LPARAM);
BOOL32     EnumWindows32(WNDENUMPROC32,LPARAM);
#define    EnumWindows WINELIB_NAME(EnumWindows)
BOOL16     EqualRect16(const RECT16*,const RECT16*);
BOOL32     EqualRect32(const RECT32*,const RECT32*);
#define    EqualRect WINELIB_NAME(EqualRect)
BOOL16     EqualRgn16(HRGN16,HRGN16);
BOOL32     EqualRgn32(HRGN32,HRGN32);
#define    EqualRgn WINELIB_NAME(EqualRgn)
LONG       EscapeCommFunction16(UINT16,UINT16);
BOOL32     EscapeCommFunction32(INT32,UINT32);
#define    EscapeCommFunction WINELIB_NAME(EscapeCommFunction)
INT16      ExcludeClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT32      ExcludeClipRect32(HDC32,INT32,INT32,INT32,INT32);
#define    ExcludeClipRect WINELIB_NAME(ExcludeClipRect)
BOOL16     ExtFloodFill16(HDC16,INT16,INT16,COLORREF,UINT16);
BOOL32     ExtFloodFill32(HDC32,INT32,INT32,COLORREF,UINT32);
#define    ExtFloodFill WINELIB_NAME(ExtFloodFill)
BOOL16     ExtTextOut16(HDC16,INT16,INT16,UINT16,const RECT16*,LPCSTR,UINT16,const INT16*);
BOOL32     ExtTextOut32A(HDC32,INT32,INT32,UINT32,const RECT32*,LPCSTR,UINT32,const INT32*);
BOOL32     ExtTextOut32W(HDC32,INT32,INT32,UINT32,const RECT32*,LPCWSTR,UINT32,const INT32*);
#define    ExtTextOut WINELIB_NAME_AW(ExtTextOut)
INT16      FillRect16(HDC16,const RECT16*,HBRUSH16);
INT32      FillRect32(HDC32,const RECT32*,HBRUSH32);
#define    FillRect WINELIB_NAME(FillRect)
BOOL16     FillRgn16(HDC16,HRGN16,HBRUSH16);
BOOL32     FillRgn32(HDC32,HRGN32,HBRUSH32);
#define    FillRgn WINELIB_NAME(FillRgn)
BOOL16     FindClose16(HANDLE16);
BOOL32     FindClose32(HANDLE32);
#define    FindClose WINELIB_NAME(FindClose)
HANDLE16   FindFirstFile16(LPCSTR,LPVOID);
HANDLE32   FindFirstFile32A(LPCSTR,LPWIN32_FIND_DATA32A);
HANDLE32   FindFirstFile32W(LPCWSTR,LPWIN32_FIND_DATA32W);
#define    FindFirst WINELIB_NAME_AW(FindFirst)
BOOL16     FindNextFile16(HANDLE16,LPVOID);
BOOL32     FindNextFile32A(HANDLE32,LPWIN32_FIND_DATA32A);
BOOL32     FindNextFile32W(HANDLE32,LPWIN32_FIND_DATA32W);
#define    FindNext WINELIB_NAME_AW(FindNext)
HRSRC16    FindResource16(HINSTANCE16,SEGPTR,SEGPTR);
HRSRC32    FindResource32A(HINSTANCE32,LPCSTR,LPCSTR);
HRSRC32    FindResource32W(HINSTANCE32,LPCWSTR,LPCWSTR);
#define    FindResource WINELIB_NAME_AW(FindResource)
HWND16     FindWindow16(SEGPTR,LPCSTR);
HWND32     FindWindow32A(LPCSTR,LPCSTR);
HWND32     FindWindow32W(LPCWSTR,LPCWSTR);
#define    FindWindow WINELIB_NAME_AW(FindWindow)
HWND16     FindWindowEx16(HWND16,HWND16,SEGPTR,LPCSTR);
HWND32     FindWindowEx32A(HWND32,HWND32,LPCSTR,LPCSTR);
HWND32     FindWindowEx32W(HWND32,HWND32,LPCWSTR,LPCWSTR);
#define    FindWindowEx WINELIB_NAME_AW(FindWindowEx)
BOOL16     FloodFill16(HDC16,INT16,INT16,COLORREF);
BOOL32     FloodFill32(HDC32,INT32,INT32,COLORREF);
#define    FloodFill WINELIB_NAME(FloodFill)
INT16      FrameRect16(HDC16,const RECT16*,HBRUSH16);
INT32      FrameRect32(HDC32,const RECT32*,HBRUSH32);
#define    FrameRect WINELIB_NAME(FrameRect)
BOOL16     FrameRgn16(HDC16,HRGN16,HBRUSH16,INT16,INT16);
BOOL32     FrameRgn32(HDC32,HRGN32,HBRUSH32,INT32,INT32);
#define    FrameRgn WINELIB_NAME(FrameRgn)
BOOL16     FreeModule16(HMODULE16);
#define    FreeModule32(handle) FreeLibrary32(handle)
#define    FreeModule WINELIB_NAME(FreeModule)
void       FreeProcInstance16(FARPROC16);
#define    FreeProcInstance32(proc) /*nothing*/
#define    FreeProcInstance WINELIB_NAME(FreeProcInstance)
BOOL16     FreeResource16(HGLOBAL16);
BOOL32     FreeResource32(HGLOBAL32);
#define    FreeResource WINELIB_NAME(FreeResource)
BOOL16     GetBitmapDimensionEx16(HBITMAP16,LPSIZE16);
BOOL32     GetBitmapDimensionEx32(HBITMAP32,LPSIZE32);
#define    GetBitmapDimensionEx WINELIB_NAME(GetBitmapDimensionEx)
BOOL16     GetBrushOrgEx16(HDC16,LPPOINT16);
BOOL32     GetBrushOrgEx32(HDC32,LPPOINT32);
#define    GetBrushOrgEx WINELIB_NAME(GetBrushOrgEx)
HWND16     GetCapture16(void);
HWND32     GetCapture32(void);
#define    GetCapture WINELIB_NAME(GetCapture)
UINT16     GetCaretBlinkTime16(void);
UINT32     GetCaretBlinkTime32(void);
#define    GetCaretBlinkTime WINELIB_NAME(GetCaretBlinkTime)
VOID       GetCaretPos16(LPPOINT16);
BOOL32     GetCaretPos32(LPPOINT32);
#define    GetCaretPos WINELIB_NAME(GetCaretPos)
BOOL16     GetClassInfo16(HINSTANCE16,SEGPTR,WNDCLASS16 *);
BOOL32     GetClassInfo32A(HINSTANCE32,LPCSTR,WNDCLASS32A *);
BOOL32     GetClassInfo32W(HINSTANCE32,LPCWSTR,WNDCLASS32W *);
#define    GetClassInfo WINELIB_NAME_AW(GetClassInfo)
BOOL16     GetClassInfoEx16(HINSTANCE16,SEGPTR,WNDCLASSEX16 *);
BOOL32     GetClassInfoEx32A(HINSTANCE32,LPCSTR,WNDCLASSEX32A *);
BOOL32     GetClassInfoEx32W(HINSTANCE32,LPCWSTR,WNDCLASSEX32W *);
#define    GetClassInfoEx WINELIB_NAME_AW(GetClassInfoEx)
LONG       GetClassLong16(HWND16,INT16);
LONG       GetClassLong32A(HWND32,INT32);
LONG       GetClassLong32W(HWND32,INT32);
#define    GetClassLong WINELIB_NAME_AW(GetClassLong)
INT16      GetClassName16(HWND16,LPSTR,INT16);
INT32      GetClassName32A(HWND32,LPSTR,INT32);
INT32      GetClassName32W(HWND32,LPWSTR,INT32);
#define    GetClassName WINELIB_NAME_AW(GetClassName)
void       GetClientRect16(HWND16,LPRECT16);
void       GetClientRect32(HWND32,LPRECT32);
#define    GetClientRect WINELIB_NAME(GetClientRect)
INT16      GetClipBox16(HDC16,LPRECT16);
INT32      GetClipBox32(HDC32,LPRECT32);
#define    GetClipBox WINELIB_NAME(GetClipBox)
void       GetClipCursor16(LPRECT16);
void       GetClipCursor32(LPRECT32);
#define    GetClipCursor WINELIB_NAME(GetClipCursor)
HRGN16     GetClipRgn16(HDC16);
INT32      GetClipRgn32(HDC32,HRGN32);
#define    GetClipRgn WINELIB_NAME(GetClipRgn)
INT16      GetCommState16(INT16,LPDCB16);
BOOL32     GetCommState32(INT32,LPDCB32);
#define    GetCommState WINELIB_NAME(GetCommState)
UINT16     GetCurrentDirectory16(UINT16,LPSTR);
UINT32     GetCurrentDirectory32A(UINT32,LPSTR);
UINT32     GetCurrentDirectory32W(UINT32,LPWSTR);
#define    GetCurrentDirectory WINELIB_NAME_AW(GetCurrentDirectory)
BOOL16     GetCurrentPositionEx16(HDC16,LPPOINT16);
BOOL32     GetCurrentPositionEx32(HDC32,LPPOINT32);
#define    GetCurrentPositionEx WINELIB_NAME(GetCurrentPositionEx)
void       GetCursorPos16(LPPOINT16);
void       GetCursorPos32(LPPOINT32);
#define    GetCursorPos WINELIB_NAME(GetCursorPos)
HDC16      GetDC16(HWND16);
HDC32      GetDC32(HWND32);
#define    GetDC WINELIB_NAME(GetDC)
HDC16      GetDCEx16(HWND16,HRGN16,DWORD);
HDC32      GetDCEx32(HWND32,HRGN32,DWORD);
#define    GetDCEx WINELIB_NAME(GetDCEx)
HWND16     GetDesktopWindow16(void);
HWND32     GetDesktopWindow32(void);
#define    GetDesktopWindow WINELIB_NAME(GetDesktopWindow)
BOOL16     GetDiskFreeSpace16(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL32     GetDiskFreeSpace32A(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL32     GetDiskFreeSpace32W(LPCWSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
#define    GetDiskFreeSpace WINELIB_NAME_AW(GetDiskFreeSpace)
INT16      GetDlgItemText16(HWND16,INT16,SEGPTR,UINT16);
INT32      GetDlgItemText32A(HWND32,INT32,LPSTR,UINT32);
INT32      GetDlgItemText32W(HWND32,INT32,LPWSTR,UINT32);
#define    GetDlgItemText WINELIB_NAME_AW(GetDlgItemText)
UINT16     GetDriveType16(UINT16); /* yes, the arguments differ */
UINT32     GetDriveType32A(LPCSTR);
UINT32     GetDriveType32W(LPCWSTR);
#define    GetDriveType WINELIB_NAME_AW(GetDriveType)
INT16      GetExpandedName16(LPCSTR,LPSTR);
INT32      GetExpandedName32A(LPCSTR,LPSTR);
INT32      GetExpandedName32W(LPCWSTR,LPWSTR);
#define    GetExpandedName WINELIB_NAME_AW(GetExpandedName)
DWORD      GetFileAttributes16(LPCSTR);
DWORD      GetFileAttributes32A(LPCSTR);
DWORD      GetFileAttributes32W(LPCWSTR);
#define    GetFileAttributes WINELIB_NAME_AW(GetFileAttributes)
DWORD      GetFileVersionInfoSize16(LPCSTR,LPDWORD);
DWORD      GetFileVersionInfoSize32A(LPCSTR,LPDWORD);
DWORD      GetFileVersionInfoSize32W(LPCWSTR,LPDWORD);
#define    GetFileVersionInfoSize WINELIB_NAME_AW(GetFileVersionInfoSize)
DWORD      GetFileVersionInfo16(LPCSTR,DWORD,DWORD,LPVOID);
DWORD      GetFileVersionInfo32A(LPCSTR,DWORD,DWORD,LPVOID);
DWORD      GetFileVersionInfo32W(LPCWSTR,DWORD,DWORD,LPVOID);
#define    GetFileVersionInfo WINELIB_NAME_AW(GetFileVersionInfo)
HWND16     GetFocus16(void);
HWND32     GetFocus32(void);
#define    GetFocus WINELIB_NAME(GetFocus)
UINT16     GetInternalWindowPos16(HWND16,LPRECT16,LPPOINT16);
UINT32     GetInternalWindowPos32(HWND32,LPRECT32,LPPOINT32);
#define    GetInternalWindowPos WINELIB_NAME(GetInternalWindowPos)
INT16      GetKeyNameText16(LONG,LPSTR,INT16);
INT32      GetKeyNameText32A(LONG,LPSTR,INT32);
INT32      GetKeyNameText32W(LONG,LPWSTR,INT32);
#define    GetKeyNameText WINELIB_NAME_AW(GetKeyNameText)
UINT32     GetLogicalDriveStrings32A(UINT32,LPSTR);
UINT32     GetLogicalDriveStrings32W(UINT32,LPWSTR);
#define    GetLogicalDriveStrings WINELIB_NAME_AW(GetLogicalDriveStrings)
INT16      GetModuleFileName16(HINSTANCE16,LPSTR,INT16);
DWORD      GetModuleFileName32A(HMODULE32,LPSTR,DWORD);
DWORD      GetModuleFileName32W(HMODULE32,LPWSTR,DWORD);
#define    GetModuleFileName WINELIB_NAME_AW(GetModuleFileName)
HWND16     GetNextDlgGroupItem16(HWND16,HWND16,BOOL16);
HWND32     GetNextDlgGroupItem32(HWND32,HWND32,BOOL32);
#define    GetNextDlgGroupItem WINELIB_NAME(GetNextDlgGroupItem)
HWND16     GetNextDlgTabItem16(HWND16,HWND16,BOOL16);
HWND32     GetNextDlgTabItem32(HWND32,HWND32,BOOL32);
#define    GetNextDlgTabItem WINELIB_NAME(GetNextDlgTabItem)
INT16      GetObject16(HANDLE16,INT16,LPVOID);
INT32      GetObject32A(HANDLE32,INT32,LPVOID);
INT32      GetObject32W(HANDLE32,INT32,LPVOID);
#define    GetObject WINELIB_NAME_AW(GetObject)
HWND16     GetParent16(HWND16);
HWND32     GetParent32(HWND32);
#define    GetParent WINELIB_NAME(GetParent)
COLORREF   GetPixel16(HDC16,INT16,INT16);
COLORREF   GetPixel32(HDC32,INT32,INT32);
#define    GetPixel WINELIB_NAME(GetPixel)
UINT16     GetPrivateProfileInt16(LPCSTR,LPCSTR,INT16,LPCSTR);
UINT32     GetPrivateProfileInt32A(LPCSTR,LPCSTR,INT32,LPCSTR);
UINT32     GetPrivateProfileInt32W(LPCWSTR,LPCWSTR,INT32,LPCWSTR);
#define    GetPrivateProfileInt WINELIB_NAME_AW(GetPrivateProfileInt)
INT16      GetPrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT16,LPCSTR);
INT32      GetPrivateProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT32,LPCSTR);
INT32      GetPrivateProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,INT32,LPCWSTR);
#define    GetPrivateProfileString WINELIB_NAME_AW(GetPrivateProfileString)
FARPROC16  GetProcAddress16(HMODULE16,SEGPTR);
FARPROC32  GetProcAddress32(HMODULE32,LPCSTR);
#define    GetProcAddress WINELIB_NAME(GetProcAddress)
UINT16     GetProfileInt16(LPCSTR,LPCSTR,INT16);
UINT32     GetProfileInt32A(LPCSTR,LPCSTR,INT32);
UINT32     GetProfileInt32W(LPCWSTR,LPCWSTR,INT32);
#define    GetProfileInt WINELIB_NAME_AW(GetProfileInt)
INT16      GetProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT16);
INT32      GetProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT32);
INT32      GetProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,INT32);
#define    GetProfileString WINELIB_NAME_AW(GetProfileString)
HANDLE16   GetProp16(HWND16,LPCSTR);
HANDLE32   GetProp32A(HWND32,LPCSTR);
HANDLE32   GetProp32W(HWND32,LPCWSTR);
#define    GetProp WINELIB_NAME_AW(GetProp)
INT16      GetRgnBox16(HRGN16,LPRECT16);
INT32      GetRgnBox32(HRGN32,LPRECT32);
#define    GetRgnBox WINELIB_NAME(GetRgnBox)
BOOL16     GetScrollInfo16(HWND16,INT16,LPSCROLLINFO);
BOOL32     GetScrollInfo32(HWND32,INT32,LPSCROLLINFO);
#define    GetScrollInfo WINELIB_NAME(GetScrollInfo)
INT16      GetScrollPos16(HWND16,INT16);
INT32      GetScrollPos32(HWND32,INT32);
#define    GetScrollPos WINELIB_NAME(GetScrollPos)
BOOL16     GetScrollRange16(HWND16,INT16,LPINT16,LPINT16);
BOOL32     GetScrollRange32(HWND32,INT32,LPINT32,LPINT32);
#define    GetScrollRange WINELIB_NAME(GetScrollRange)
HGDIOBJ16  GetStockObject16(INT16);
HGDIOBJ32  GetStockObject32(INT32);
#define    GetStockObject WINELIB_NAME(GetStockObject)
HBRUSH16   GetSysColorBrush16(INT16);
HBRUSH32   GetSysColorBrush32(INT32);
#define    GetSysColorBrush WINELIB_NAME(GetSysColorBrush)
HWND16     GetSysModalWindow16(void);
#define    GetSysModalWindow32() ((HWND32)0)
#define    GetSysModalWindow WINELIB_NAME(GetSysModalWindow)
UINT16     GetSystemDirectory16(LPSTR,UINT16);
UINT32     GetSystemDirectory32A(LPSTR,UINT32);
UINT32     GetSystemDirectory32W(LPWSTR,UINT32);
#define    GetSystemDirectory WINELIB_NAME_AW(GetSystemDirectory)
UINT16     GetTempFileName16(BYTE,LPCSTR,UINT16,LPSTR);
UINT32     GetTempFileName32A(LPCSTR,LPCSTR,UINT32,LPSTR);
UINT32     GetTempFileName32W(LPCWSTR,LPCWSTR,UINT32,LPWSTR);
#define    GetTempFileName WINELIB_NAME_AW(GetTempFileName)
UINT32     GetTempPath32A(UINT32,LPSTR);
UINT32     GetTempPath32W(UINT32,LPWSTR);
#define    GetTempPath WINELIB_NAME_AW(GetTempPath)
BOOL16     GetTextExtentPoint16(HDC16,LPCSTR,INT16,LPSIZE16);
BOOL32     GetTextExtentPoint32A(HDC32,LPCSTR,INT32,LPSIZE32);
BOOL32     GetTextExtentPoint32W(HDC32,LPCWSTR,INT32,LPSIZE32);
#define    GetTextExtentPoint WINELIB_NAME_AW(GetTextExtentPoint)
INT16      GetTextFace16(HDC16,INT16,LPSTR);
INT32      GetTextFace32A(HDC32,INT32,LPSTR);
INT32      GetTextFace32W(HDC32,INT32,LPWSTR);
#define    GetTextFace WINELIB_NAME_AW(GetTextFace)
BOOL16     GetTextMetrics16(HDC16,LPTEXTMETRIC16);
BOOL32     GetTextMetrics32A(HDC32,LPTEXTMETRIC32A);
BOOL32     GetTextMetrics32W(HDC32,LPTEXTMETRIC32W);
#define    GetTextMetrics WINELIB_NAME_AW(GetTextMetrics)
BOOL16     GetUpdateRect16(HWND16,LPRECT16,BOOL16);
BOOL32     GetUpdateRect32(HWND32,LPRECT32,BOOL32);
#define    GetUpdateRect WINELIB_NAME(GetUpdateRect)
LONG       GetVersion16(void);
LONG       GetVersion32(void);
#define    GetVersion WINELIB_NAME(GetVersion)
BOOL16     GetViewportExtEx16(HDC16,LPPOINT16);
BOOL32     GetViewportExtEx32(HDC32,LPPOINT32);
#define    GetViewportExtEx WINELIB_NAME(GetViewportExtEx)
BOOL16     GetViewportOrgEx16(HDC16,LPPOINT16);
BOOL32     GetViewportOrgEx32(HDC32,LPPOINT32);
#define    GetViewportOrgEx WINELIB_NAME(GetViewportOrgEx)
BOOL32     GetVolumeInformation32A(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
BOOL32     GetVolumeInformation32W(LPCWSTR,LPWSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPWSTR,DWORD);
#define    GetVolumeInformation WINELIB_NAME_AW(GetVolumeInformation)
HDC16      GetWindowDC16(HWND16);
HDC32      GetWindowDC32(HWND32);
#define    GetWindowDC WINELIB_NAME(GetWindowDC)
BOOL16     GetWindowExtEx16(HDC16,LPPOINT16);
BOOL32     GetWindowExtEx32(HDC32,LPPOINT32);
#define    GetWindowExtEx WINELIB_NAME(GetWindowExtEx)
LONG       GetWindowLong16(HWND16,INT16);
LONG       GetWindowLong32A(HWND32,INT32);
LONG       GetWindowLong32W(HWND32,INT32);
#define    GetWindowLong WINELIB_NAME_AW(GetWindowLong)
BOOL16     GetWindowOrgEx16(HDC16,LPPOINT16);
BOOL32     GetWindowOrgEx32(HDC32,LPPOINT32);
#define    GetWindowOrgEx WINELIB_NAME(GetWindowOrgEx)
BOOL16     GetWindowPlacement16(HWND16,LPWINDOWPLACEMENT16);
BOOL32     GetWindowPlacement32(HWND32,LPWINDOWPLACEMENT32);
#define    GetWindowPlacement WINELIB_NAME(GetWindowPlacement)
void       GetWindowRect16(HWND16,LPRECT16);
void       GetWindowRect32(HWND32,LPRECT32);
#define    GetWindowRect WINELIB_NAME(GetWindowRect)
UINT16     GetWindowsDirectory16(LPSTR,UINT16);
UINT32     GetWindowsDirectory32A(LPSTR,UINT32);
UINT32     GetWindowsDirectory32W(LPWSTR,UINT32);
#define    GetWindowsDirectory WINELIB_NAME_AW(GetWindowsDirectory)
HTASK16    GetWindowTask16(HWND16);
#define    GetWindowTask32(hwnd) ((HTASK32)GetWindowThreadProcessId(hwnd,NULL))
#define    GetWindowTask WINELIB_NAME(GetWindowTask)
INT16      GetWindowText16(HWND16,SEGPTR,INT16);
INT32      GetWindowText32A(HWND32,LPSTR,INT32);
INT32      GetWindowText32W(HWND32,LPWSTR,INT32);
#define    GetWindowText WINELIB_NAME_AW(GetWindowText)
INT16      GetWindowTextLength16(HWND16);
INT32      GetWindowTextLength32A(HWND32);
INT32      GetWindowTextLength32W(HWND32);
#define    GetWindowTextLength WINELIB_NAME_AW(GetWindowTextLength)
ATOM       GlobalAddAtom16(SEGPTR);
ATOM       GlobalAddAtom32A(LPCSTR);
ATOM       GlobalAddAtom32W(LPCWSTR);
#define    GlobalAddAtom WINELIB_NAME_AW(GlobalAddAtom)
HGLOBAL16  GlobalAlloc16(UINT16,DWORD);
HGLOBAL32  GlobalAlloc32(UINT32,DWORD);
#define    GlobalAlloc WINELIB_NAME(GlobalAlloc)
DWORD      GlobalCompact16(DWORD);
DWORD      GlobalCompact32(DWORD);
#define    GlobalCompact WINELIB_NAME(GlobalCompact)
UINT16     GlobalFlags16(HGLOBAL16);
UINT32     GlobalFlags32(HGLOBAL32);
#define    GlobalFlags WINELIB_NAME(GlobalFlags)
ATOM       GlobalFindAtom16(SEGPTR);
ATOM       GlobalFindAtom32A(LPCSTR);
ATOM       GlobalFindAtom32W(LPCWSTR);
#define    GlobalFindAtom WINELIB_NAME_AW(GlobalFindAtom)
HGLOBAL16  GlobalFree16(HGLOBAL16);
HGLOBAL32  GlobalFree32(HGLOBAL32);
#define    GlobalFree WINELIB_NAME(GlobalFree)
UINT16     GlobalGetAtomName16(ATOM,LPSTR,INT16);
UINT32     GlobalGetAtomName32A(ATOM,LPSTR,INT32);
UINT32     GlobalGetAtomName32W(ATOM,LPWSTR,INT32);
#define    GlobalGetAtomName WINELIB_NAME_AW(GlobalGetAtomName)
DWORD      GlobalHandle16(WORD);
HGLOBAL32  GlobalHandle32(LPCVOID);
#define    GlobalHandle WINELIB_NAME(GlobalHandle)
LPVOID     GlobalLock16(HGLOBAL16);
LPVOID     GlobalLock32(HGLOBAL32);
#define    GlobalLock WINELIB_NAME(GlobalLock)
HGLOBAL16  GlobalReAlloc16(HGLOBAL16,DWORD,UINT16);
HGLOBAL32  GlobalReAlloc32(HGLOBAL32,DWORD,UINT32);
#define    GlobalReAlloc WINELIB_NAME(GlobalReAlloc)
DWORD      GlobalSize16(HGLOBAL16);
DWORD      GlobalSize32(HGLOBAL32);
#define    GlobalSize WINELIB_NAME(GlobalSize)
BOOL16     GlobalUnlock16(HGLOBAL16);
BOOL32     GlobalUnlock32(HGLOBAL32);
#define    GlobalUnlock WINELIB_NAME(GlobalUnlock)
void       InflateRect16(LPRECT16,INT16,INT16);
void       InflateRect32(LPRECT32,INT32,INT32);
#define    InflateRect WINELIB_NAME(InflateRect)
BOOL16     InsertMenu16(HMENU16,UINT16,UINT16,UINT16,SEGPTR);
BOOL32     InsertMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32     InsertMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define    InsertMenu WINELIB_NAME_AW(InsertMenu)
INT16      IntersectClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT32      IntersectClipRect32(HDC32,INT32,INT32,INT32,INT32);
#define    IntersectClipRect WINELIB_NAME(IntersectClipRect)
BOOL16     IntersectRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL32     IntersectRect32(LPRECT32,const RECT32*,const RECT32*);
#define    IntersectRect WINELIB_NAME(IntersectRect)
void       InvalidateRect16(HWND16,const RECT16*,BOOL16);
void       InvalidateRect32(HWND32,const RECT32*,BOOL32);
#define    InvalidateRect WINELIB_NAME(InvalidateRect)
void       InvertRect16(HDC16,const RECT16*);
void       InvertRect32(HDC32,const RECT32*);
#define    InvertRect WINELIB_NAME(InvertRect)
BOOL16     InvertRgn16(HDC16,HRGN16);
BOOL32     InvertRgn32(HDC32,HRGN32);
#define    InvertRgn WINELIB_NAME(InvertRgn)
BOOL16     IsBadCodePtr16(SEGPTR);
BOOL32     IsBadCodePtr32(FARPROC32);
#define    IsBadCodePtr WINELIB_NAME(IsBadCodePtr)
BOOL16     IsBadHugeReadPtr16(SEGPTR,DWORD);
BOOL32     IsBadHugeReadPtr32(LPCVOID,UINT32);
#define    IsBadHugeReadPtr WINELIB_NAME(IsBadHugeReadPtr)
BOOL16     IsBadHugeWritePtr16(SEGPTR,DWORD);
BOOL32     IsBadHugeWritePtr32(LPCVOID,UINT32);
#define    IsBadHugeWritePtr WINELIB_NAME(IsBadHugeWritePtr)
BOOL16     IsBadReadPtr16(SEGPTR,UINT16);
BOOL32     IsBadReadPtr32(LPCVOID,UINT32);
#define    IsBadReadPtr WINELIB_NAME(IsBadReadPtr)
BOOL16     IsBadStringPtr16(SEGPTR,UINT16);
BOOL32     IsBadStringPtr32A(LPCSTR,UINT32);
BOOL32     IsBadStringPtr32W(LPCWSTR,UINT32);
#define    IsBadStringPtr WINELIB_NAME_AW(IsBadStringPtr)
BOOL16     IsBadWritePtr16(SEGPTR,UINT16);
BOOL32     IsBadWritePtr32(LPVOID,UINT32);
#define    IsBadWritePtr WINELIB_NAME(IsBadWritePtr)
BOOL16     IsCharAlpha16(CHAR);
BOOL32     IsCharAlpha32A(CHAR);
BOOL32     IsCharAlpha32W(WCHAR);
#define    IsCharAlpha WINELIB_NAME_AW(IsCharAlpha)
BOOL16     IsCharAlphaNumeric16(CHAR);
BOOL32     IsCharAlphaNumeric32A(CHAR);
BOOL32     IsCharAlphaNumeric32W(WCHAR);
#define    IsCharAlphaNumeric WINELIB_NAME_AW(IsCharAlphaNumeric)
BOOL16     IsCharLower16(CHAR);
BOOL32     IsCharLower32A(CHAR);
BOOL32     IsCharLower32W(WCHAR);
#define    IsCharLower WINELIB_NAME_AW(IsCharLower)
BOOL16     IsCharUpper16(CHAR);
BOOL32     IsCharUpper32A(CHAR);
BOOL32     IsCharUpper32W(WCHAR);
#define    IsCharUpper WINELIB_NAME_AW(IsCharUpper)
BOOL16     IsRectEmpty16(const RECT16*);
BOOL32     IsRectEmpty32(const RECT32*);
#define    IsRectEmpty WINELIB_NAME(IsRectEmpty)
BOOL16     KillSystemTimer16(HWND16,UINT16);
BOOL32     KillSystemTimer32(HWND32,UINT32);
#define    KillSystemTimer WINELIB_NAME(KillSystemTimer)
BOOL16     KillTimer16(HWND16,UINT16);
BOOL32     KillTimer32(HWND32,UINT32);
#define    KillTimer WINELIB_NAME(KillTimer)
HFILE      LZOpenFile16(LPCSTR,LPOFSTRUCT,UINT16);
HFILE      LZOpenFile32A(LPCSTR,LPOFSTRUCT,UINT32);
HFILE      LZOpenFile32W(LPCWSTR,LPOFSTRUCT,UINT32);
#define    LZOpenFile WINELIB_NAME_AW(LZOpenFile)
INT16      LZRead16(HFILE,SEGPTR,UINT16); 
INT32      LZRead32(HFILE,LPVOID,UINT32); 
#define    LZRead WINELIB_NAME(LZRead)
VOID       LineDDA16(INT16,INT16,INT16,INT16,LINEDDAPROC16,LPARAM);
BOOL32     LineDDA32(INT32,INT32,INT32,INT32,LINEDDAPROC32,LPARAM);
#define    LineDDA WINELIB_NAME(LineDDA)
BOOL16     LineTo16(HDC16,INT16,INT16);
BOOL32     LineTo32(HDC32,INT32,INT32);
#define    LineTo WINELIB_NAME(LineTo)
HACCEL16   LoadAccelerators16(HINSTANCE16,SEGPTR);
HACCEL32   LoadAccelerators32A(HINSTANCE32,LPCSTR);
HACCEL32   LoadAccelerators32W(HINSTANCE32,LPCWSTR);
#define    LoadAccelerators WINELIB_NAME_AW(LoadAccelerators)
HBITMAP16  LoadBitmap16(HANDLE16,SEGPTR);
HBITMAP32  LoadBitmap32A(HANDLE32,LPCSTR);
HBITMAP32  LoadBitmap32W(HANDLE32,LPCWSTR);
#define    LoadBitmap WINELIB_NAME(LoadBitmap)
HCURSOR16  LoadCursor16(HINSTANCE16,SEGPTR);
HCURSOR32  LoadCursor32A(HINSTANCE32,LPCSTR);
HCURSOR32  LoadCursor32W(HINSTANCE32,LPCWSTR);
#define    LoadCursor WINELIB_NAME_AW(LoadCursor)
HICON16    LoadIcon16(HINSTANCE16,SEGPTR);
HICON32    LoadIcon32A(HINSTANCE32,LPCSTR);
HICON32    LoadIcon32W(HINSTANCE32,LPCWSTR);
#define    LoadIcon WINELIB_NAME_AW(LoadIcon)
HINSTANCE16 LoadLibrary16(LPCSTR);
HINSTANCE32 LoadLibrary32A(LPCSTR);
HINSTANCE32 LoadLibrary32W(LPCWSTR);
#define    LoadLibrary WINELIB_NAME_AW(LoadLibrary)
HMENU16    LoadMenu16(HINSTANCE16,SEGPTR);
HMENU32    LoadMenu32A(HINSTANCE32,LPCSTR);
HMENU32    LoadMenu32W(HINSTANCE32,LPCWSTR);
#define    LoadMenu WINELIB_NAME_AW(LoadMenu)
HMENU16    LoadMenuIndirect16(LPCVOID);
HMENU32    LoadMenuIndirect32A(LPCVOID);
HMENU32    LoadMenuIndirect32W(LPCVOID);
#define    LoadMenuIndirect WINELIB_NAME_AW(LoadMenuIndirect)
HGLOBAL16  LoadResource16(HINSTANCE16,HRSRC16);
HGLOBAL32  LoadResource32(HINSTANCE32,HRSRC32);
#define    LoadResource WINELIB_NAME(LoadResource)
INT16      LoadString16(HINSTANCE16,UINT16,LPSTR,INT16);
INT32      LoadString32A(HINSTANCE32,UINT32,LPSTR,INT32);
INT32      LoadString32W(HINSTANCE32,UINT32,LPWSTR,INT32);
#define    LoadString WINELIB_NAME_AW(LoadString)
HLOCAL16   LocalAlloc16(UINT16,WORD);
HLOCAL32   LocalAlloc32(UINT32,DWORD);
#define    LocalAlloc WINELIB_NAME(LocalAlloc)
UINT16     LocalCompact16(UINT16);
UINT32     LocalCompact32(UINT32);
#define    LocalCompact WINELIB_NAME(LocalCompact)
UINT16     LocalFlags16(HLOCAL16);
UINT32     LocalFlags32(HLOCAL32);
#define    LocalFlags WINELIB_NAME(LocalFlags)
HLOCAL16   LocalFree16(HLOCAL16);
HLOCAL32   LocalFree32(HLOCAL32);
#define    LocalFree WINELIB_NAME(LocalFree)
HLOCAL16   LocalHandle16(WORD);
HLOCAL32   LocalHandle32(LPCVOID);
#define    LocalHandle WINELIB_NAME(LocalHandle)
SEGPTR     LocalLock16(HLOCAL16);
LPVOID     LocalLock32(HLOCAL32);
#define    LocalLock WINELIB_NAME(LocalLock)
HLOCAL16   LocalReAlloc16(HLOCAL16,WORD,UINT16);
HLOCAL32   LocalReAlloc32(HLOCAL32,DWORD,UINT32);
#define    LocalReAlloc WINELIB_NAME(LocalReAlloc)
UINT16     LocalShrink16(HGLOBAL16,UINT16);
UINT32     LocalShrink32(HGLOBAL32,UINT32);
#define    LocalShrink WINELIB_NAME(LocalShrink)
UINT16     LocalSize16(HLOCAL16);
UINT32     LocalSize32(HLOCAL32);
#define    LocalSize WINELIB_NAME(LocalSize)
BOOL16     LocalUnlock16(HLOCAL16);
BOOL32     LocalUnlock32(HLOCAL32);
#define    LocalUnlock WINELIB_NAME(LocalUnlock)
LPVOID     LockResource16(HGLOBAL16);
LPVOID     LockResource32(HGLOBAL32);
#define    LockResource WINELIB_NAME(LockResource)
HGLOBAL16  LockSegment16(HGLOBAL16);
#define    LockSegment32(handle) GlobalFix((HANDLE32)(handle))
#define    LockSegment WINELIB_NAME(LockSegment)
BOOL16     LPtoDP16(HDC16,LPPOINT16,INT16);
BOOL32     LPtoDP32(HDC32,LPPOINT32,INT32);
#define    LPtoDP WINELIB_NAME(LPtoDP)
FARPROC16  MakeProcInstance16(FARPROC16,HANDLE16);
#define    MakeProcInstance32(proc,inst) (proc)
#define    MakeProcInstance WINELIB_NAME(MakeProcInstance)
void       MapDialogRect16(HWND16,LPRECT16);
void       MapDialogRect32(HWND32,LPRECT32);
#define    MapDialogRect WINELIB_NAME(MapDialogRect)
void       MapWindowPoints16(HWND16,HWND16,LPPOINT16,UINT16);
void       MapWindowPoints32(HWND32,HWND32,LPPOINT32,UINT32);
#define    MapWindowPoints WINELIB_NAME(MapWindowPoints)
INT16      MessageBox16(HWND16,LPCSTR,LPCSTR,UINT16);
INT32      MessageBox32A(HWND32,LPCSTR,LPCSTR,UINT32);
INT32      MessageBox32W(HWND32,LPCWSTR,LPCWSTR,UINT32);
#define    MessageBox WINELIB_NAME_AW(MessageBox)
BOOL16     ModifyMenu16(HMENU16,UINT16,UINT16,UINT16,SEGPTR);
BOOL32     ModifyMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32     ModifyMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define    ModifyMenu WINELIB_NAME_AW(ModifyMenu)
BOOL16     MoveToEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32     MoveToEx32(HDC32,INT32,INT32,LPPOINT32);
#define    MoveToEx WINELIB_NAME(MoveToEx)
INT16      MulDiv16(INT16,INT16,INT16);
INT32      MulDiv32(INT32,INT32,INT32);
#define    MulDiv WINELIB_NAME(MulDiv)
BOOL32     OemToChar32A(LPSTR,LPSTR);
BOOL32     OemToChar32W(LPCSTR,LPWSTR);
#define    OemToChar WINELIB_NAME_AW(OemToChar)
BOOL32     OemToCharBuff32A(LPSTR,LPSTR,DWORD);
BOOL32     OemToCharBuff32W(LPCSTR,LPWSTR,DWORD);
#define    OemToCharBuff WINELIB_NAME_AW(OemToCharBuff)
INT16      OffsetClipRgn16(HDC16,INT16,INT16);
INT32      OffsetClipRgn32(HDC32,INT32,INT32);
#define    OffsetClipRgn WINELIB_NAME(OffsetClipRgn)
void       OffsetRect16(LPRECT16,INT16,INT16);
void       OffsetRect32(LPRECT32,INT32,INT32);
#define    OffsetRect WINELIB_NAME(OffsetRect)
INT16      OffsetRgn16(HRGN16,INT16,INT16);
INT32      OffsetRgn32(HRGN32,INT32,INT32);
#define    OffsetRgn WINELIB_NAME(OffsetRgn)
BOOL16     OffsetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32     OffsetViewportOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define    OffsetViewportOrgEx WINELIB_NAME(OffsetViewportOrgEx)
BOOL16     OffsetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32     OffsetWindowOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define    OffsetWindowOrgEx WINELIB_NAME(OffsetWindowOrgEx)
BOOL16     PaintRgn16(HDC16,HRGN16);
BOOL32     PaintRgn32(HDC32,HRGN32);
#define    PaintRgn WINELIB_NAME(PaintRgn)
BOOL16     PatBlt16(HDC16,INT16,INT16,INT16,INT16,DWORD);
BOOL32     PatBlt32(HDC32,INT32,INT32,INT32,INT32,DWORD);
#define    PatBlt WINELIB_NAME(PatBlt)
BOOL16     PeekMessage16(LPMSG16,HWND16,UINT16,UINT16,UINT16);
BOOL32     PeekMessage32A(LPMSG32,HWND32,UINT32,UINT32,UINT32);
BOOL32     PeekMessage32W(LPMSG32,HWND32,UINT32,UINT32,UINT32);
#define    PeekMessage WINELIB_NAME_AW(PeekMessage)
BOOL16     Pie16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32     Pie32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define    Pie WINELIB_NAME(Pie)
BOOL16     PolyPolygon16(HDC16,LPPOINT16,LPINT16,UINT16);
BOOL32     PolyPolygon32(HDC32,LPPOINT32,LPINT32,UINT32);
#define    PolyPolygon WINELIB_NAME(PolyPolygon)
BOOL16     Polygon16(HDC16,LPPOINT16,INT16);
BOOL32     Polygon32(HDC32,LPPOINT32,INT32);
#define    Polygon WINELIB_NAME(Polygon)
BOOL16     Polyline16(HDC16,LPPOINT16,INT16);
BOOL32     Polyline32(HDC32,LPPOINT32,INT32);
#define    Polyline WINELIB_NAME(Polyline)
BOOL16     PostAppMessage16(HTASK16,UINT16,WPARAM16,LPARAM);
#define    PostAppMessage32A(thread,msg,wparam,lparam) \
           PostThreadMessage32A((DWORD)(thread),msg,wparam,lparam)
#define    PostAppMessage32W(thread,msg,wparam,lparam) \
           PostThreadMessage32W((DWORD)(thread),msg,wparam,lparam)
#define    PostAppMessage WINELIB_NAME_AW(PostAppMessage)
BOOL16     PtInRect16(const RECT16*,POINT16);
BOOL32     PtInRect32(const RECT32*,POINT32);
#define    PtInRect WINELIB_NAME(PtInRect)
BOOL16     PtInRegion16(HRGN16,INT16,INT16);
BOOL32     PtInRegion32(HRGN32,INT32,INT32);
#define    PtInRegion WINELIB_NAME(PtInRegion)
BOOL16     PtVisible16(HDC16,INT16,INT16);
BOOL32     PtVisible32(HDC32,INT32,INT32);
#define    PtVisible WINELIB_NAME(PtVisible)
BOOL16     Rectangle16(HDC16,INT16,INT16,INT16,INT16);
BOOL32     Rectangle32(HDC32,INT32,INT32,INT32,INT32);
#define    Rectangle WINELIB_NAME(Rectangle)
BOOL16     RectInRegion16(HRGN16,const RECT16 *);
BOOL32     RectInRegion32(HRGN32,const RECT32 *);
#define    RectInRegion WINELIB_NAME(RectInRegion)
BOOL16     RectVisible16(HDC16,LPRECT16);
BOOL32     RectVisible32(HDC32,LPRECT32);
#define    RectVisible WINELIB_NAME(RectVisible)
BOOL16     RedrawWindow16(HWND16,const RECT16*,HRGN16,UINT16);
BOOL32     RedrawWindow32(HWND32,const RECT32*,HRGN32,UINT32);
#define    RedrawWindow WINELIB_NAME(RedrawWindow)
DWORD      RegCreateKey16(HKEY,LPCSTR,LPHKEY);
DWORD      RegCreateKey32A(HKEY,LPCSTR,LPHKEY);
DWORD      RegCreateKey32W(HKEY,LPCWSTR,LPHKEY);
#define    RegCreateKey WINELIB_NAME_AW(RegCreateKey)
DWORD      RegDeleteKey16(HKEY,LPCSTR);
DWORD      RegDeleteKey32A(HKEY,LPCSTR);
DWORD      RegDeleteKey32W(HKEY,LPWSTR);
#define    RegDeleteKey WINELIB_NAME_AW(RegDeleteKey)
DWORD      RegDeleteValue16(HKEY,LPSTR);
DWORD      RegDeleteValue32A(HKEY,LPSTR);
DWORD      RegDeleteValue32W(HKEY,LPWSTR);
#define    RegDeleteValue WINELIB_NAME_AW(RegDeleteValue)
DWORD      RegEnumKey16(HKEY,DWORD,LPSTR,DWORD);
DWORD      RegEnumKey32A(HKEY,DWORD,LPSTR,DWORD);
DWORD      RegEnumKey32W(HKEY,DWORD,LPWSTR,DWORD);
#define    RegEnumKey WINELIB_NAME_AW(RegEnumKey)
DWORD      RegEnumValue16(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD      RegEnumValue32A(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD      RegEnumValue32W(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define    RegEnumValue WINELIB_NAME_AW(RegEnumValue)
ATOM       RegisterClass16(const WNDCLASS16*);
ATOM       RegisterClass32A(const WNDCLASS32A *);
ATOM       RegisterClass32W(const WNDCLASS32W *);
#define    RegisterClass WINELIB_NAME_AW(RegisterClass)
ATOM       RegisterClassEx16(const WNDCLASSEX16*);
ATOM       RegisterClassEx32A(const WNDCLASSEX32A *);
ATOM       RegisterClassEx32W(const WNDCLASSEX32W *);
#define    RegisterClassEx WINELIB_NAME_AW(RegisterClassEx)
UINT16     RegisterClipboardFormat16(LPCSTR);
UINT32     RegisterClipboardFormat32A(LPCSTR);
UINT32     RegisterClipboardFormat32W(LPCWSTR);
#define    RegisterClipboardFormat WINELIB_NAME_AW(RegisterClipboardFormat)
WORD       RegisterWindowMessage16(SEGPTR);
WORD       RegisterWindowMessage32A(LPCSTR);
WORD       RegisterWindowMessage32W(LPCWSTR);
#define    RegisterWindowMessage WINELIB_NAME_AW(RegisterWindowMessage)
DWORD      RegOpenKey16(HKEY,LPCSTR,LPHKEY);
DWORD      RegOpenKey32A(HKEY,LPCSTR,LPHKEY);
DWORD      RegOpenKey32W(HKEY,LPCWSTR,LPHKEY);
#define    RegOpenKey WINELIB_NAME_AW(RegOpenKey)
DWORD      RegQueryValue16(HKEY,LPSTR,LPSTR,LPDWORD);
DWORD      RegQueryValue32A(HKEY,LPSTR,LPSTR,LPDWORD);
DWORD      RegQueryValue32W(HKEY,LPWSTR,LPWSTR,LPDWORD);
#define    RegQueryValue WINELIB_NAME_AW(RegQueryValue)
DWORD      RegQueryValueEx16(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD      RegQueryValueEx32A(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD      RegQueryValueEx32W(HKEY,LPWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define    RegQueryValueEx WINELIB_NAME_AW(RegQueryValueEx)
DWORD      RegSetValue16(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD      RegSetValue32A(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD      RegSetValue32W(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define    RegSetValue WINELIB_NAME_AW(RegSetValue)
DWORD      RegSetValueEx16(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
DWORD      RegSetValueEx32A(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
DWORD      RegSetValueEx32W(HKEY,LPWSTR,DWORD,DWORD,LPBYTE,DWORD);
#define    RegSetValueEx WINELIB_NAME_AW(RegSetValueEx)
INT16      ReleaseDC16(HWND16,HDC16);
INT32      ReleaseDC32(HWND32,HDC32);
#define    ReleaseDC WINELIB_NAME(ReleaseDC)
HANDLE16   RemoveProp16(HWND16,LPCSTR);
HANDLE32   RemoveProp32A(HWND32,LPCSTR);
HANDLE32   RemoveProp32W(HWND32,LPCWSTR);
#define    RemoveProp WINELIB_NAME_AW(RemoveProp)
BOOL16     RemoveDirectory16(LPCSTR);
BOOL32     RemoveDirectory32A(LPCSTR);
BOOL32     RemoveDirectory32W(LPCWSTR);
#define    RemoveDirectory WINELIB_NAME_AW(RemoveDirectory)
BOOL16     RoundRect16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32     RoundRect32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32);
#define    RoundRect WINELIB_NAME(RoundRect)
BOOL16     ScaleViewportExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL32     ScaleViewportExtEx32(HDC32,INT32,INT32,INT32,INT32,LPSIZE32);
#define    ScaleViewportExtEx WINELIB_NAME(ScaleViewportExtEx)
BOOL16     ScaleWindowExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL32     ScaleWindowExtEx32(HDC32,INT32,INT32,INT32,INT32,LPSIZE32);
#define    ScaleWindowExtEx WINELIB_NAME(ScaleWindowExtEx)
void       ScreenToClient16(HWND16,LPPOINT16);
void       ScreenToClient32(HWND32,LPPOINT32);
#define    ScreenToClient WINELIB_NAME(ScreenToClient)
BOOL16     ScrollDC16(HDC16,INT16,INT16,const RECT16*,const RECT16*,
                      HRGN16,LPRECT16);
BOOL32     ScrollDC32(HDC32,INT32,INT32,const RECT32*,const RECT32*,
                      HRGN32,LPRECT32);
#define    ScrollDC WINELIB_NAME(ScrollDC)
void       ScrollWindow16(HWND16,INT16,INT16,const RECT16*,const RECT16*);
BOOL32     ScrollWindow32(HWND32,INT32,INT32,const RECT32*,const RECT32*);
#define    ScrollWindow WINELIB_NAME(ScrollWindow)
INT16      ScrollWindowEx16(HWND16,INT16,INT16,const RECT16*,const RECT16*,
                            HRGN16,LPRECT16,UINT16);
INT32      ScrollWindowEx32(HWND32,INT32,INT32,const RECT32*,const RECT32*,
                            HRGN32,LPRECT32,UINT32);
#define    ScrollWindowEx WINELIB_NAME(ScrollWindowEx)
INT16      SelectClipRgn16(HDC16,HRGN16);
INT32      SelectClipRgn32(HDC32,HRGN32);
#define    SelectClipRgn WINELIB_NAME(SelectClipRgn)
HGDIOBJ16  SelectObject16(HDC16,HGDIOBJ16);
HGDIOBJ32  SelectObject32(HDC32,HGDIOBJ32);
#define    SelectObject WINELIB_NAME(SelectObject)
LRESULT    SendDlgItemMessage16(HWND16,INT16,UINT16,WPARAM16,LPARAM);
LRESULT    SendDlgItemMessage32A(HWND32,INT32,UINT32,WPARAM32,LPARAM);
LRESULT    SendDlgItemMessage32W(HWND32,INT32,UINT32,WPARAM32,LPARAM);
#define    SendDlgItemMessage WINELIB_NAME_AW(SendDlgItemMessage)
LRESULT    SendMessage16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT    SendMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT    SendMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define    SendMessage WINELIB_NAME_AW(SendMessage)
BOOL16     SetBitmapDimensionEx16(HBITMAP16,INT16,INT16,LPSIZE16);
BOOL32     SetBitmapDimensionEx32(HBITMAP32,INT32,INT32,LPSIZE32);
#define    SetBitmapDimensionEx WINELIB_NAME(SetBitmapDimensionEx)
HWND16     SetCapture16(HWND16);
HWND32     SetCapture32(HWND32);
#define    SetCapture WINELIB_NAME(SetCapture)
LONG       SetClassLong16(HWND16,INT16,LONG);
LONG       SetClassLong32A(HWND32,INT32,LONG);
LONG       SetClassLong32W(HWND32,INT32,LONG);
#define    SetClassLong WINELIB_NAME_AW(SetClassLong)
INT16      SetCommBreak16(INT16);
BOOL32     SetCommBreak32(INT32);
#define    SetCommBreak WINELIB_NAME(SetCommBreak)
INT16      SetCommState16(LPDCB16);
BOOL32     SetCommState32(INT32,LPDCB32);
#define    SetCommState WINELIB_NAME(SetCommState)
BOOL16     SetDeskWallPaper16(LPCSTR);
BOOL32     SetDeskWallPaper32(LPCSTR);
#define    SetDeskWallPaper WINELIB_NAME(SetDeskWallPaper)
void       SetDlgItemInt16(HWND16,INT16,UINT16,BOOL16);
void       SetDlgItemInt32(HWND32,INT32,UINT32,BOOL32);
#define    SetDlgItemInt WINELIB_NAME(SetDlgItemInt)
void       SetDlgItemText16(HWND16,INT16,SEGPTR);
void       SetDlgItemText32A(HWND32,INT32,LPCSTR);
void       SetDlgItemText32W(HWND32,INT32,LPCWSTR);
#define    SetDlgItemText WINELIB_NAME_AW(SetDlgItemText)
BOOL32     SetEnvironmentVariable32A(LPCSTR,LPCSTR);
BOOL32     SetEnvironmentVariable32W(LPCWSTR,LPCWSTR);
#define    SetEnvironmentVariable WINELIB_NAME_AW(SetEnvironmentVariable)
BOOL16     SetFileAttributes16(LPCSTR,DWORD);
BOOL32     SetFileAttributes32A(LPCSTR,DWORD);
BOOL32     SetFileAttributes32W(LPCWSTR,DWORD);
#define    SetFileAttributes WINELIB_NAME_AW(SetFileAttributes)
HWND16     SetFocus16(HWND16);
HWND32     SetFocus32(HWND32);
#define    SetFocus WINELIB_NAME(SetFocus)
UINT16     SetHandleCount16(UINT16);
UINT32     SetHandleCount32(UINT32);
#define    SetHandleCount WINELIB_NAME(SetHandleCount)
void       SetInternalWindowPos16(HWND16,UINT16,LPRECT16,LPPOINT16);
void       SetInternalWindowPos32(HWND32,UINT32,LPRECT32,LPPOINT32);
#define    SetInternalWindowPos WINELIB_NAME(SetInternalWindowPos)
COLORREF   SetPixel16(HDC16,INT16,INT16,COLORREF);
COLORREF   SetPixel32(HDC32,INT32,INT32,COLORREF);
#define    SetPixel WINELIB_NAME(SetPixel)
BOOL16     SetProp16(HWND16,LPCSTR,HANDLE16);
BOOL32     SetProp32A(HWND32,LPCSTR,HANDLE32);
BOOL32     SetProp32W(HWND32,LPCWSTR,HANDLE32);
#define    SetProp WINELIB_NAME_AW(SetProp)
void       SetRect16(LPRECT16,INT16,INT16,INT16,INT16);
void       SetRect32(LPRECT32,INT32,INT32,INT32,INT32);
#define    SetRect WINELIB_NAME(SetRect)
void       SetRectEmpty16(LPRECT16);
void       SetRectEmpty32(LPRECT32);
#define    SetRectEmpty WINELIB_NAME(SetRectEmpty)
INT16      SetScrollInfo16(HWND16,INT16,const SCROLLINFO*,BOOL16);
INT32      SetScrollInfo32(HWND32,INT32,const SCROLLINFO*,BOOL32);
#define    SetScrollInfo WINELIB_NAME(SetScrollInfo)
INT16      SetScrollPos16(HWND16,INT16,INT16,BOOL16);
INT32      SetScrollPos32(HWND32,INT32,INT32,BOOL32);
#define    SetScrollPos WINELIB_NAME(SetScrollPos)
void       SetScrollRange16(HWND16,INT16,INT16,INT16,BOOL16);
BOOL32     SetScrollRange32(HWND32,INT32,INT32,INT32,BOOL32);
#define    SetScrollRange WINELIB_NAME(SetScrollRange)
HWND16     SetSysModalWindow16(HWND16);
#define    SetSysModalWindow32(hwnd) ((HWND32)0)
#define    SetSysModalWindow WINELIB_NAME(SetSysModalWindow)
UINT16     SetSystemTimer16(HWND16,UINT16,UINT16,TIMERPROC16);
UINT32     SetSystemTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define    SetSystemTimer WINELIB_NAME(SetSystemTimer)
UINT16     SetTimer16(HWND16,UINT16,UINT16,TIMERPROC16);
UINT32     SetTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define    SetTimer WINELIB_NAME(SetTimer)
BOOL16     SetViewportExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL32     SetViewportExtEx32(HDC32,INT32,INT32,LPSIZE32);
#define    SetViewportExtEx WINELIB_NAME(SetViewportExtEx)
BOOL16     SetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32     SetViewportOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define    SetViewportOrgEx WINELIB_NAME(SetViewportOrgEx)
BOOL16     SetWindowExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL32     SetWindowExtEx32(HDC32,INT32,INT32,LPSIZE32);
#define    SetWindowExtEx WINELIB_NAME(SetWindowExtEx)
LONG       SetWindowLong16(HWND16,INT16,LONG);
LONG       SetWindowLong32A(HWND32,INT32,LONG);
LONG       SetWindowLong32W(HWND32,INT32,LONG);
#define    SetWindowLong WINELIB_NAME_AW(SetWindowLong)
BOOL16     SetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32     SetWindowOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define    SetWindowOrgEx WINELIB_NAME(SetWindowOrgEx)
BOOL16     SetWindowPlacement16(HWND16,const WINDOWPLACEMENT16*);
BOOL32     SetWindowPlacement32(HWND32,const WINDOWPLACEMENT32*);
#define    SetWindowPlacement WINELIB_NAME(SetWindowPlacement)
FARPROC16  SetWindowsHook16(INT16,HOOKPROC16);
HHOOK      SetWindowsHook32A(INT32,HOOKPROC32);
HHOOK      SetWindowsHook32W(INT32,HOOKPROC32);
#define    SetWindowsHook WINELIB_NAME_AW(SetWindowsHook)
HHOOK      SetWindowsHookEx16(INT16,HOOKPROC16,HINSTANCE16,HTASK16);
HHOOK      SetWindowsHookEx32A(INT32,HOOKPROC32,HINSTANCE32,DWORD);
HHOOK      SetWindowsHookEx32W(INT32,HOOKPROC32,HINSTANCE32,DWORD);
#define    SetWindowsHookEx WINELIB_NAME_AW(SetWindowsHookEx)
void       SetWindowText16(HWND16,SEGPTR);
void       SetWindowText32A(HWND32,LPCSTR);
void       SetWindowText32W(HWND32,LPCWSTR);
#define    SetWindowText WINELIB_NAME_AW(SetWindowText)
void       ShowScrollBar16(HWND16,INT16,BOOL16);
BOOL32     ShowScrollBar32(HWND32,INT32,BOOL32);
#define    ShowScrollBar WINELIB_NAME(ShowScrollBar)
BOOL16     StretchBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,INT16,
                        INT16,INT16,DWORD);
BOOL32     StretchBlt32(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,INT32,
                        INT32,INT32,DWORD);
#define    StretchBlt WINELIB_NAME(StretchBlt)
INT16      StretchDIBits16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,const VOID*,const BITMAPINFO*,UINT16,DWORD);
INT32      StretchDIBits32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,const VOID*,const BITMAPINFO*,UINT32,DWORD);
#define    StretchDIBits WINELIB_NAME(StretchDIBits)
BOOL16     SubtractRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL32     SubtractRect32(LPRECT32,const RECT32*,const RECT32*);
#define    SubtractRect WINELIB_NAME(SubtractRect)
BOOL16     SystemParametersInfo16(UINT16,UINT16,LPVOID,UINT16);
BOOL32     SystemParametersInfo32A(UINT32,UINT32,LPVOID,UINT32);
BOOL32     SystemParametersInfo32W(UINT32,UINT32,LPVOID,UINT32);
#define    SystemParametersInfo WINELIB_NAME_AW(SystemParametersInfo)
BOOL16     TextOut16(HDC16,INT16,INT16,LPCSTR,INT16);
BOOL32     TextOut32A(HDC32,INT32,INT32,LPCSTR,INT32);
BOOL32     TextOut32W(HDC32,INT32,INT32,LPCWSTR,INT32);
#define    TextOut WINELIB_NAME_AW(TextOut)
BOOL16     TrackPopupMenu16(HMENU16,UINT16,INT16,INT16,INT16,HWND16,const RECT16*);
BOOL32     TrackPopupMenu32(HMENU32,UINT32,INT32,INT32,INT32,HWND32,const RECT32*);
#define    TrackPopupMenu WINELIB_NAME(TrackPopupMenu)
INT16      TransmitCommChar16(INT16,CHAR);
BOOL32     TransmitCommChar32(INT32,CHAR);
#define    TransmitCommChar WINELIB_NAME(TransmitCommChar)
BOOL16     UnhookWindowsHook16(INT16,HOOKPROC16);
BOOL32     UnhookWindowsHook32(INT32,HOOKPROC32);
#define    UnhookWindowsHook WINELIB_NAME(UnhookWindowsHook)
BOOL16     UnhookWindowsHookEx16(HHOOK);
BOOL32     UnhookWindowsHookEx32(HHOOK);
#define    UnhookWindowsHookEx WINELIB_NAME(UnhookWindowsHookEx)
BOOL16     UnionRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL32     UnionRect32(LPRECT32,const RECT32*,const RECT32*);
#define    UnionRect WINELIB_NAME(UnionRect)
void       UnlockSegment16(HGLOBAL16);
#define    UnlockSegment32(handle) GlobalUnfix((HANDLE32)(handle))
#define    UnlockSegment WINELIB_NAME(UnlockSegment)
BOOL16     UnrealizeObject16(HGDIOBJ16);
BOOL32     UnrealizeObject32(HGDIOBJ32);
#define    UnrealizeObject WINELIB_NAME(UnrealizeObject)
BOOL16     UnregisterClass16(SEGPTR,HINSTANCE16);
BOOL32     UnregisterClass32A(LPCSTR,HINSTANCE32);
BOOL32     UnregisterClass32W(LPCWSTR,HINSTANCE32);
#define    UnregisterClass WINELIB_NAME_AW(UnregisterClass)
void       ValidateRect16(HWND16,const RECT16*);
void       ValidateRect32(HWND32,const RECT32*);
#define    ValidateRect WINELIB_NAME(ValidateRect)
DWORD      VerFindFile16(UINT16,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16*,LPSTR,UINT16*);
DWORD      VerFindFile32A(UINT32,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT32*,LPSTR,UINT32*);
DWORD      VerFindFile32W(UINT32,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT32*,LPWSTR,UINT32*);
#define    VerFindFile WINELIB_NAME_AW(VerFindFile)
DWORD      VerInstallFile16(UINT16,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16*);
DWORD      VerInstallFile32A(UINT32,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT32*);
DWORD      VerInstallFile32W(UINT32,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT32*);
#define    VerInstallFile WINELIB_NAME_AW(VerInstallFile)
DWORD      VerLanguageName16(UINT16,LPSTR,UINT16);
DWORD      VerLanguageName32A(UINT32,LPSTR,UINT32);
DWORD      VerLanguageName32W(UINT32,LPWSTR,UINT32);
#define    VerLanguageName WINELIB_NAME_AW(VerLanguageName)
DWORD      VerQueryValue16(SEGPTR,LPCSTR,SEGPTR*,UINT16*);
DWORD      VerQueryValue32A(LPVOID,LPCSTR,LPVOID*,UINT32*);
DWORD      VerQueryValue32W(LPVOID,LPCWSTR,LPVOID*,UINT32*);
#define    VerQueryValue WINELIB_NAME_AW(VerQueryValue)
HWND16     WindowFromDC16(HDC16);
HWND32     WindowFromDC32(HDC32);
#define    WindowFromDC WINELIB_NAME(WindowFromDC)
HWND16     WindowFromPoint16(POINT16);
HWND32     WindowFromPoint32(POINT32);
#define    WindowFromPoint WINELIB_NAME(WindowFromPoint)
BOOL16     WritePrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL32     WritePrivateProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL32     WritePrivateProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
#define    WritePrivateProfileString WINELIB_NAME_AW(WritePrivateProfileString)
BOOL16     WriteProfileString16(LPCSTR,LPCSTR,LPCSTR);
BOOL32     WriteProfileString32A(LPCSTR,LPCSTR,LPCSTR);
BOOL32     WriteProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR);
#define    WriteProfileString WINELIB_NAME_AW(WriteProfileString)
SEGPTR     lstrcat16(SEGPTR,SEGPTR);
LPSTR      lstrcat32A(LPSTR,LPCSTR);
LPWSTR     lstrcat32W(LPWSTR,LPCWSTR);
#define    lstrcat WINELIB_NAME_AW(lstrcat)
SEGPTR     lstrcatn16(SEGPTR,SEGPTR,INT16);
LPSTR      lstrcatn32A(LPSTR,LPCSTR,INT32);
LPWSTR     lstrcatn32W(LPWSTR,LPCWSTR,INT32);
#define    lstrcatn WINELIB_NAME_AW(lstrcatn)
INT16      lstrcmp16(LPCSTR,LPCSTR);
INT32      lstrcmp32A(LPCSTR,LPCSTR);
INT32      lstrcmp32W(LPCWSTR,LPCWSTR);
#define    lstrcmp WINELIB_NAME_AW(lstrcmp)
INT16      lstrcmpi16(LPCSTR,LPCSTR);
INT32      lstrcmpi32A(LPCSTR,LPCSTR);
INT32      lstrcmpi32W(LPCWSTR,LPCWSTR);
#define    lstrcmpi WINELIB_NAME_AW(lstrcmpi)
SEGPTR     lstrcpy16(SEGPTR,SEGPTR);
LPSTR      lstrcpy32A(LPSTR,LPCSTR);
LPWSTR     lstrcpy32W(LPWSTR,LPCWSTR);
#define    lstrcpy WINELIB_NAME_AW(lstrcpy)
SEGPTR     lstrcpyn16(SEGPTR,SEGPTR,INT16);
LPSTR      lstrcpyn32A(LPSTR,LPCSTR,INT32);
LPWSTR     lstrcpyn32W(LPWSTR,LPCWSTR,INT32);
#define    lstrcpyn WINELIB_NAME_AW(lstrcpyn)
INT16      lstrlen16(LPCSTR);
INT32      lstrlen32A(LPCSTR);
INT32      lstrlen32W(LPCWSTR);
#define    lstrlen WINELIB_NAME_AW(lstrlen)
INT16      wsnprintf16(LPSTR,UINT16,LPCSTR,...);
INT32      wsnprintf32A(LPSTR,UINT32,LPCSTR,...);
INT32      wsnprintf32W(LPWSTR,UINT32,LPCWSTR,...);
#define    wsnprintf WINELIB_NAME_AW(wsnprintf)
INT16      wsprintf16(LPSTR,LPCSTR,...);
INT32      wsprintf32A(LPSTR,LPCSTR,...);
INT32      wsprintf32W(LPWSTR,LPCWSTR,...);
#define    wsprintf WINELIB_NAME_AW(wsprintf)
INT16      wvsnprintf16(LPSTR,UINT16,LPCSTR,LPCVOID);
INT32      wvsnprintf32A(LPSTR,UINT32,LPCSTR,LPCVOID);
INT32      wvsnprintf32W(LPWSTR,UINT32,LPCWSTR,LPCVOID);
#define    wvsnprintf WINELIB_NAME_AW(wvsnprintf)
INT16      wvsprintf16(LPSTR,LPCSTR,LPCVOID);
INT32      wvsprintf32A(LPSTR,LPCSTR,LPCVOID);
INT32      wvsprintf32W(LPWSTR,LPCWSTR,LPCVOID);
#define    wvsprintf WINELIB_NAME_AW(wvsprintf)
UINT16     _lread16(HFILE,LPVOID,UINT16);
UINT32     _lread32(HFILE,LPVOID,UINT32);
#define    _lread WINELIB_NAME(_lread)
UINT16     _lwrite16(HFILE,LPCSTR,UINT16);
UINT32     _lwrite32(HFILE,LPCSTR,UINT32);
#define    _lwrite WINELIB_NAME(_lwrite)

/* Extra functions that don't exist in the Windows API */

INT32      LoadMessage32A(HINSTANCE32,UINT32,WORD,LPSTR,INT32);
INT32      LoadMessage32W(HINSTANCE32,UINT32,WORD,LPWSTR,INT32);
SEGPTR     WIN16_GlobalLock16(HGLOBAL16);
INT32      lstrncmp32A(LPCSTR,LPCSTR,INT32);
INT32      lstrncmp32W(LPCWSTR,LPCWSTR,INT32);
INT32      lstrncmpi32A(LPCSTR,LPCSTR,INT32);
INT32      lstrncmpi32W(LPCWSTR,LPCWSTR,INT32);
LPWSTR     lstrcpynAtoW(LPWSTR,LPCSTR,INT32);
LPSTR      lstrcpynWtoA(LPSTR,LPCWSTR,INT32);

/* Library data types defined as a transition aid for the emulator. */
/* These should _not_ be used in the emulator and will be removed someday. */

#ifndef NO_TRANSITION_TYPES

#ifdef __WINE__
# ifdef WINELIB32
typedef INT32 INT;
typedef UINT32 UINT;
typedef BOOL32 BOOL;
typedef HANDLE32 HWND;
# else  /* WINELIB32 */
typedef INT16 INT;
typedef UINT16 UINT;
typedef BOOL16 BOOL;
typedef HANDLE16 HWND;
# endif  /* WINELIB32 */
#endif  /* __WINE__ */

/* Callback function pointers types. */

#ifdef WINELIB
typedef LONG (*DRIVERPROC)(DWORD, HDRVR16, UINT, LPARAM, LPARAM);
typedef int (*EDITWORDBREAKPROC)(LPSTR lpch, int ichCurrent, int cch,int code);
#else
typedef SEGPTR DRIVERPROC;
typedef SEGPTR EDITWORDBREAKPROC;
#endif

ATOM       AddAtom(SEGPTR);
INT        AddFontResource(LPCSTR);
BOOL       AnimatePalette(HPALETTE16,UINT,UINT,LPPALETTEENTRY);
UINT       AnsiLowerBuff(LPSTR,UINT);
SEGPTR     AnsiNext(SEGPTR);
SEGPTR     AnsiPrev(SEGPTR,SEGPTR);
INT        AnsiToOem(LPCSTR,LPSTR);
void       AnsiToOemBuff(LPCSTR,LPSTR,UINT);
UINT       AnsiUpperBuff(LPSTR,UINT);
BOOL       AnyPopup(void);
UINT       ArrangeIconicWindows(HWND);
HDWP16     BeginDeferWindowPos(INT);
BOOL       BringWindowToTop(HWND);
void       CalcChildScroll(HWND,WORD);
BOOL       CallMsgFilter(SEGPTR,INT);
BOOL       ChangeClipboardChain(HWND,HWND);
INT        CheckMenuItem(HMENU16,UINT,UINT);
BOOL       CloseClipboard(void);
void       CloseSound(void);
BOOL       CloseWindow(HWND);
int        ConvertRequest(HWND,LPKANJISTRUCT);
HMETAFILE16 CopyMetaFile(HMETAFILE16,LPCSTR);
INT        CountClipboardFormats(void);
INT        CountVoiceNotes(INT);
HDC16      CreateCompatibleDC(HDC16);
HCURSOR16  CreateCursor(HINSTANCE16,INT,INT,INT,INT,const BYTE*,const BYTE*);
HGLOBAL16  CreateCursorIconIndirect(HINSTANCE16,CURSORICONINFO*,const BYTE*,const BYTE*);
HBITMAP16  CreateDIBitmap(HDC16,BITMAPINFOHEADER*,DWORD,LPVOID,BITMAPINFO*,UINT);
HDC16      CreateIC(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HICON16    CreateIcon(HINSTANCE16,INT,INT,BYTE,BYTE,const BYTE*,const BYTE*);
HMENU16    CreateMenu(void);
HPALETTE16 CreatePalette(const LOGPALETTE*);
HMENU16    CreatePopupMenu(void);
HDWP16     DeferWindowPos(HDWP16,HWND,HWND,INT,INT,INT,INT,UINT);
ATOM       DeleteAtom(ATOM);
BOOL       DeleteDC(HDC16);
BOOL       DeleteMenu(HMENU16,UINT,UINT);
BOOL       DestroyCursor(HCURSOR16);
BOOL       DestroyIcon(HICON16);
BOOL       DestroyMenu(HMENU16);
BOOL       DestroyWindow(HWND);
LONG       DispatchMessage(const MSG16*);
BOOL16     DragDetect(HWND16,POINT16);
DWORD      DragObject(HWND, HWND, WORD, HANDLE16, WORD, HCURSOR16);
BOOL       DrawIcon(HDC16,INT,INT,HICON16);
void       DrawMenuBar(HWND);
DWORD      DumpIcon(SEGPTR,WORD*,SEGPTR*,SEGPTR*);
BOOL       EmptyClipboard(void);
BOOL       EnableMenuItem(HMENU16,UINT,UINT);
BOOL       EnableWindow(HWND,BOOL);
BOOL       EndDeferWindowPos(HDWP16);
UINT16     EnumClipboardFormats(UINT16);
BOOL       EnumMetaFile(HDC16,HMETAFILE16,MFENUMPROC16,LPARAM);
INT        Escape(HDC16,INT,INT,SEGPTR,SEGPTR);
BOOL       ExitWindows(DWORD,WORD);
HICON16    ExtractIcon(HINSTANCE16,LPCSTR,WORD);
void       FatalAppExit(UINT,LPCSTR);
void       FatalExit(int);
ATOM       FindAtom(SEGPTR);
HINSTANCE16 FindExecutable(LPCSTR,LPCSTR,LPSTR);
BOOL       FlashWindow(HWND,BOOL);
void       FreeLibrary(HINSTANCE16);
UINT       GDIRealizePalette(HDC16);
HPALETTE16 GDISelectPalette(HDC16,HPALETTE16,WORD);
HWND       GetActiveWindow(void);
DWORD      GetAspectRatioFilter(HDC16);
int        GetAsyncKeyState(int);
WORD       GetAtomName(ATOM,LPSTR,short);
COLORREF   GetBkColor(HDC16);
WORD       GetBkMode(HDC16);
DWORD      GetBrushOrg(HDC16);
BOOL       GetCharABCWidths(HDC16,UINT,UINT,LPABC16);
BOOL       GetCharWidth(HDC16,WORD,WORD,LPINT16);
HANDLE16   GetClipboardData(WORD);
int        GetClipboardFormatName(WORD,LPSTR,short);
HWND       GetClipboardOwner(void);
HWND       GetClipboardViewer(void);
HBRUSH16   GetControlBrush(HWND,HDC16,WORD);
DWORD      GetCurrentPosition(HDC16);
DWORD      GetCurrentTime(void);
HCURSOR16  GetCursor(void);
HDC16      GetDCState(HDC16);
int        GetDIBits(HDC16,HBITMAP16,WORD,WORD,LPSTR,LPBITMAPINFO,WORD);
SEGPTR     GetDOSEnvironment(void);
int        GetDeviceCaps(HDC16,WORD);
DWORD      GetDialogBaseUnits(void);
HWND       GetDlgItem(HWND,WORD);
WORD       GetDlgItemInt(HWND,WORD,BOOL*,BOOL);
WORD       GetDoubleClickTime(void);
int        GetEnvironment(LPSTR,LPSTR,WORD);
DWORD      GetFreeSpace(UINT16);
DWORD      GetHeapSpaces(HMODULE16);
BOOL       GetInputState(void);
int        GetKBCodePage(void);
int        GetKerningPairs(HDC16,int,LPKERNINGPAIR16);
INT        GetKeyState(INT);
void       GetKeyboardState(BYTE*);
int        GetKeyboardType(int);
HWND       GetLastActivePopup(HWND);
WORD       GetMapMode(HDC16);
HMENU16    GetMenu(HWND);
DWORD      GetMenuCheckMarkDimensions(void);
INT        GetMenuItemCount(HMENU16);
UINT       GetMenuItemID(HMENU16,int);
UINT       GetMenuState(HMENU16,UINT,UINT);
int        GetMenuString(HMENU16,UINT,LPSTR,short,UINT);
BOOL       GetMessage(SEGPTR,HWND,UINT,UINT);
LONG       GetMessageExtraInfo(void);
DWORD      GetMessagePos(void);
LONG       GetMessageTime(void);
HMETAFILE16 GetMetaFile(LPSTR);
HGLOBAL16  GetMetaFileBits(HMETAFILE16);
HMODULE16  GetModuleHandle(LPCSTR);
INT16      GetModuleUsage(HINSTANCE16);
DWORD      GetNearestColor(HDC16,DWORD);
WORD       GetNearestPaletteIndex(HPALETTE16,DWORD);
HWND       GetNextWindow(HWND,WORD);
HWND       GetOpenClipboardWindow(void);
WORD       GetPaletteEntries(HPALETTE16,WORD,WORD,LPPALETTEENTRY);
WORD       GetPolyFillMode(HDC16);
int        GetPriorityClipboardFormat(WORD*,short);
DWORD      GetQueueStatus(UINT);
BOOL       GetRasterizerCaps(LPRASTERIZER_STATUS,UINT);
WORD       GetROP2(HDC16);
WORD       GetRelAbs(HDC16);
WORD       GetStretchBltMode(HDC16);
HMENU16    GetSubMenu(HMENU16,short);
HMENU16    GetSystemMenu(HWND,BOOL);
int        GetSystemMetrics(WORD);
WORD       GetSystemPaletteEntries(HDC16,WORD,WORD,LPPALETTEENTRY);
WORD       GetSystemPaletteUse(HDC16);
DWORD      GetTabbedTextExtent(HDC16,LPSTR,int,int,LPINT16);
BYTE       GetTempDrive(BYTE);
WORD       GetTextAlign(HDC16);
short      GetTextCharacterExtra(HDC16);
COLORREF   GetTextColor(HDC16);
DWORD      GetTextExtent(HDC16,LPCSTR,short);
LPINT16    GetThresholdEvent(void);
int        GetThresholdStatus(void);
HWND       GetTopWindow(HWND);
DWORD      GetViewportExt(HDC16);
DWORD      GetViewportOrg(HDC16);
BOOL       GetWinDebugInfo(LPWINDEBUGINFO,UINT);
LONG       GetWinFlags(void);
HWND       GetWindow(HWND,WORD);
DWORD      GetWindowExt(HDC16);
DWORD      GetWindowOrg(HDC16);
ATOM       GlobalDeleteAtom(ATOM);
void       GlobalFix(HGLOBAL16);
BOOL16     GlobalUnWire(HGLOBAL16);
void       GlobalUnfix(HGLOBAL16);
SEGPTR     GlobalWire(HGLOBAL16);
BOOL       GrayString(HDC16,HBRUSH16,GRAYSTRINGPROC16,LPARAM,INT,INT,INT,INT,INT);
BOOL       HiliteMenuItem(HWND,HMENU16,UINT,UINT);
BOOL       InSendMessage(void);
WORD       InitAtomTable(WORD);
HRGN32     InquireVisRgn(HDC16);
void       InvalidateRgn(HWND32,HRGN32,BOOL32);
BOOL       IsChild(HWND,HWND);
BOOL       IsClipboardFormatAvailable(WORD);
BOOL       IsDialogMessage(HWND,LPMSG16);
WORD       IsDlgButtonChecked(HWND,WORD);
BOOL       IsIconic(HWND);
BOOL       IsMenu(HMENU16);
BOOL       IsValidMetaFile(HMETAFILE16);
BOOL       IsWindowEnabled(HWND);
BOOL       IsWindowVisible(HWND);
BOOL       IsZoomed(HWND);
HINSTANCE16 LoadModule(LPCSTR,LPVOID);
FARPROC16  LocalNotify(FARPROC16);
HMENU16    LookupMenuHandle(HMENU16,INT);
WORD       MapVirtualKey(WORD,WORD);
void       MessageBeep(WORD);
BOOL       MoveWindow(HWND,short,short,short,short,BOOL);
DWORD      OemKeyScan(WORD);
BOOL       OemToAnsi(LPCSTR,LPSTR);
void       OemToAnsiBuff(LPCSTR,LPSTR,INT);
BOOL       OpenClipboard(HWND);
BOOL       OpenIcon(HWND);
int        OpenSound(void);
void       OutputDebugString(LPCSTR);
BOOL       PlayMetaFile(HDC16,HMETAFILE16);
void       PlayMetaFileRecord(HDC16,LPHANDLETABLE16,LPMETARECORD,WORD);
BOOL       PostMessage(HWND,WORD,WORD,LONG);
void       PostQuitMessage(INT);
void       ProfClear(void);
void       ProfFinish(void);
void       ProfFlush(void);
int        ProfInsChk(void);
void       ProfSampRate(int,int);
void       ProfSetup(int,int);
void       ProfStart(void);
void       ProfStop(void);
WORD       RealizeDefaultPalette(HDC16);
BOOL       RemoveFontResource(LPSTR);
BOOL       RemoveMenu(HMENU16,UINT,UINT);
void       ReplyMessage(LRESULT);
HDC16      ResetDC(HDC16,LPVOID);
BOOL       ResizePalette(HPALETTE16,UINT);
BOOL       RestoreDC(HDC16,short);
int        SaveDC(HDC16);
void       ScrollChildren(HWND,UINT,WPARAM16,LPARAM);
HPALETTE16 SelectPalette(HDC16,HPALETTE16,BOOL);
HWND       SetActiveWindow(HWND);
WORD       SetBkMode(HDC16,WORD);
HANDLE16   SetClipboardData(WORD,HANDLE16);
HWND       SetClipboardViewer(HWND);
void       SetConvertHook(BOOL);
BOOL       SetConvertParams(int,int);
BOOL32     SetCurrentDirectory(LPCSTR);
HCURSOR16  SetCursor(HCURSOR16);
void       SetCursorPos(short,short);
void       SetDCState(HDC16,HDC16);
void       SetDoubleClickTime(WORD);
int        SetEnvironment(LPCSTR,LPCSTR,WORD);
UINT       SetErrorMode(UINT);
WORD       SetHookFlags(HDC16,WORD);
void       SetKeyboardState(BYTE*);
WORD       SetMapMode(HDC16,WORD);
DWORD      SetMapperFlags(HDC16,DWORD);
BOOL       SetMenu(HWND,HMENU16);
BOOL       SetMenuItemBitmaps(HMENU16,UINT,UINT,HBITMAP16,HBITMAP16);
BOOL       SetMessageQueue(int);
HMETAFILE16 SetMetaFileBits(HGLOBAL16);
WORD       SetPaletteEntries(HPALETTE16,WORD,WORD,LPPALETTEENTRY);
HWND       SetParent(HWND,HWND);
WORD       SetPolyFillMode(HDC16,WORD);
WORD       SetROP2(HDC16,WORD);
WORD       SetRelAbs(HDC16,WORD);
FARPROC16  SetResourceHandler(HINSTANCE16,LPSTR,FARPROC16);
int        SetSoundNoise(int,int);
WORD       SetStretchBltMode(HDC16,WORD);
LONG       SetSwapAreaSize(WORD);
void       SetSysColors(int,LPINT16,COLORREF*);
WORD       SetSystemPaletteUse(HDC16,WORD);
WORD       SetTextAlign(HDC16,WORD);
short      SetTextCharacterExtra(HDC16,short);
short      SetTextJustification(HDC16,short,short);
int        SetVoiceAccent(int,int,int,int,int);
int        SetVoiceEnvelope(int,int,int);
int        SetVoiceNote(int,int,int,int);
int        SetVoiceQueueSize(int,int);
int        SetVoiceSound(int,LONG,int);
int        SetVoiceThreshold(int,int);
BOOL       SetWinDebugInfo(LPWINDEBUGINFO);
BOOL       SetWindowPos(HWND,HWND,INT,INT,INT,INT,WORD);
HINSTANCE16 ShellExecute(HWND,LPCSTR,LPCSTR,LPSTR,LPCSTR,INT);
int        ShowCursor(BOOL);
void       ShowOwnedPopups(HWND,BOOL);
BOOL       ShowWindow(HWND,int);
DWORD      SizeofResource(HMODULE16,HRSRC16);
int        StartSound(void);
int        StopSound(void);
BOOL       SwapMouseButton(BOOL);
void       SwapRecording(WORD);
int        SyncAllVoices(void);
LONG       TabbedTextOut(HDC16,short,short,LPSTR,short,short,LPINT16,short);
int        ToAscii(WORD,WORD,LPSTR,LPVOID,WORD);
INT16      TranslateAccelerator(HWND,HACCEL16,LPMSG16);
BOOL       TranslateMDISysAccel(HWND,LPMSG16);
BOOL       TranslateMessage(LPMSG16);
int        UpdateColors(HDC16);
void       ValidateCodeSegments(void);
LPSTR      ValidateFreeSpaces(void);
void       ValidateRgn(HWND32,HRGN32);
WORD       VkKeyScan(WORD);
SEGPTR     WIN16_LockResource(HGLOBAL16);
void       WaitMessage(void);
int        WaitSoundState(int);
HINSTANCE16 WinExec(LPSTR,WORD);
BOOL       WinHelp(HWND,LPSTR,WORD,DWORD);

#endif  /* NO_TRANSITION_TYPES */

#define WINELIB_UNIMP(x) fprintf (stderr, "WineLib: Unimplemented %s\n", x)

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINDOWS_H */
