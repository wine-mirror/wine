#ifndef __WINE_WINDOWS_H
#define __WINE_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#include "wintypes.h"

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

DECL_WINELIB_TYPE(SIZE)
DECL_WINELIB_TYPE(LPSIZE)

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

DECL_WINELIB_TYPE(POINT)
DECL_WINELIB_TYPE(LPPOINT)

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

DECL_WINELIB_TYPE(RECT)
DECL_WINELIB_TYPE(LPRECT)

#define CONV_RECT16TO32(r16,r32) \
    ((r32)->left  = (INT32)(r16)->left,  (r32)->top    = (INT32)(r16)->top, \
     (r32)->right = (INT32)(r16)->right, (r32)->bottom = (INT32)(r16)->bottom)
#define CONV_RECT32TO16(r32,r16) \
    ((r16)->left  = (INT16)(r32)->left,  (r16)->top    = (INT16)(r32)->top, \
     (r16)->right = (INT16)(r32)->right, (r16)->bottom = (INT16)(r32)->bottom)


typedef struct tagCOORD {
    INT16 x;
    INT16 y;
} COORD, *LPCOORD;


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

DECL_WINELIB_TYPE(KERNINGPAIR)
DECL_WINELIB_TYPE(LPKERNINGPAIR)

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

DECL_WINELIB_TYPE(PAINTSTRUCT)
DECL_WINELIB_TYPE(LPPAINTSTRUCT)


typedef struct tagPIXELFORMATDESCRIPTOR {
    WORD  nSize;
    WORD  nVersion;
    DWORD dwFlags;
    BYTE  iPixelType;
    BYTE  cColorBits;
    BYTE  cRedBits;
    BYTE  cRedShift;
    BYTE  cGreenBits;
    BYTE  cGreenShift;
    BYTE  cBlueBits;
    BYTE  cBlueShift;
    BYTE  cAlphaBits;
    BYTE  cAlphaShift;
    BYTE  cAccumBits;
    BYTE  cAccumRedBits;
    BYTE  cAccumGreenBits;
    BYTE  cAccumBlueBits;
    BYTE  cAccumAlphaBits;
    BYTE  cDepthBits;
    BYTE  cStencilBits;
    BYTE  cAuxBuffers;
    BYTE  iLayerType;
    BYTE  bReserved;
    DWORD dwLayerMask;
    DWORD dwVisibleMask;
    DWORD dwDamageMask;
} PIXELFORMATDESCRIPTOR, *LPPIXELFORMATDESCRIPTOR;

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

DECL_WINELIB_TYPE_AW(CREATESTRUCT)
DECL_WINELIB_TYPE_AW(LPCREATESTRUCT)

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

DECL_WINELIB_TYPE(CLIENTCREATESTRUCT)
DECL_WINELIB_TYPE(LPCLIENTCREATESTRUCT)

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

DECL_WINELIB_TYPE_AW(MDICREATESTRUCT)
DECL_WINELIB_TYPE_AW(LPMDICREATESTRUCT)

#define MDITILE_VERTICAL     0x0000   
#define MDITILE_HORIZONTAL   0x0001
#define MDITILE_SKIPDISABLED 0x0002

#define MDIS_ALLCHILDSTYLES  0x0001

typedef struct {
    DWORD   styleOld;
    DWORD   styleNew;
} STYLESTRUCT, *LPSTYLESTRUCT;

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

DECL_WINELIB_TYPE(MINMAXINFO)

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

/* debug flags */
#define DBGFILL_ALLOC  0xfd
#define DBGFILL_FREE   0xfb
#define DBGFILL_BUFFER 0xf9
#define DBGFILL_STACK  0xf7

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

DECL_WINELIB_TYPE(WINDOWPOS)

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

DECL_WINELIB_TYPE(WINDOWPLACEMENT)
DECL_WINELIB_TYPE(LPWINDOWPLACEMENT)

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

DECL_WINELIB_TYPE(NCCALCSIZE_PARAMS)
DECL_WINELIB_TYPE(LPNCCALCSIZE_PARAMS)

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

/****** Window classes ******/

typedef struct
{
    UINT16      style;
    WNDPROC16   lpfnWndProc WINE_PACKED;
    INT16       cbClsExtra;
    INT16       cbWndExtra;
    HANDLE16    hInstance;
    HICON16     hIcon;
    HCURSOR16   hCursor;
    HBRUSH16    hbrBackground;
    SEGPTR      lpszMenuName WINE_PACKED;
    SEGPTR      lpszClassName WINE_PACKED;
} WNDCLASS16, *LPWNDCLASS16;

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

#define CS_VREDRAW          0x0001
#define CS_HREDRAW          0x0002
#define CS_KEYCVTWINDOW     0x0004
#define CS_DBLCLKS          0x0008
#define CS_OWNDC            0x0020
#define CS_CLASSDC          0x0040
#define CS_PARENTDC         0x0080
#define CS_NOKEYCVT         0x0100
#define CS_NOCLOSE          0x0200
#define CS_SAVEBITS         0x0800
#define CS_BYTEALIGNCLIENT  0x1000
#define CS_BYTEALIGNWINDOW  0x2000
#define CS_GLOBALCLASS      0x4000

  /* Offsets for GetClassLong() and GetClassWord() */
#define GCL_MENUNAME        (-8)
#define GCW_HBRBACKGROUND   (-10)
#define GCL_HBRBACKGROUND   GCW_HBRBACKGROUND
#define GCW_HCURSOR         (-12)
#define GCL_HCURSOR         GCW_HCURSOR
#define GCW_HICON           (-14)
#define GCL_HICON           GCW_HICON
#define GCW_HMODULE         (-16)
#define GCL_HMODULE         GCW_HMODULE
#define GCW_CBWNDEXTRA      (-18)
#define GCL_CBWNDEXTRA      GCW_CBWNDEXTRA
#define GCW_CBCLSEXTRA      (-20)
#define GCL_CBCLSEXTRA      GCW_CBCLSEXTRA
#define GCL_WNDPROC         (-24)
#define GCW_STYLE           (-26)
#define GCL_STYLE           GCW_STYLE
#define GCW_ATOM            (-32)
#define GCW_HICONSM         (-34)
#define GCL_HICONSM         GCW_HICONSM

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

DECL_WINELIB_TYPE(EVENTMSG)
DECL_WINELIB_TYPE(LPEVENTMSG)

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

DECL_WINELIB_TYPE(MOUSEHOOKSTRUCT)
DECL_WINELIB_TYPE(LPMOUSEHOOKSTRUCT)

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

DECL_WINELIB_TYPE(HARDWAREHOOKSTRUCT)
DECL_WINELIB_TYPE(LPHARDWAREHOOKSTRUCT)

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

DECL_WINELIB_TYPE_AW(CBT_CREATEWND)
DECL_WINELIB_TYPE_AW(LPCBT_CREATEWND)

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

DECL_WINELIB_TYPE(CBTACTIVATESTRUCT)
DECL_WINELIB_TYPE(LPCBTACTIVATESTRUCT)

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

DECL_WINELIB_TYPE(DEBUGHOOKINFO)
DECL_WINELIB_TYPE(LPDEBUGHOOKINFO)

typedef DWORD (CALLBACK *LPTHREAD_START_ROUTINE)(LPVOID);

/* This is also defined in winnt.h */
/* typedef struct _EXCEPTION_RECORD {
    DWORD   ExceptionCode;
    DWORD   ExceptionFlags;
    struct  _EXCEPTION_RECORD *ExceptionRecord;
    LPVOID  ExceptionAddress;
    DWORD   NumberParameters;
    DWORD   ExceptionInformation[15];
} EXCEPTION_RECORD; */

typedef struct _EXCEPTION_DEBUG_INFO {
/*    EXCEPTION_RECORD ExceptionRecord; */
    DWORD dwFirstChange;
} EXCEPTION_DEBUG_INFO;

typedef struct _CREATE_THREAD_DEBUG_INFO {
    HANDLE32 hThread;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
} CREATE_THREAD_DEBUG_INFO;

typedef struct _CREATE_PROCESS_DEBUG_INFO {
    HANDLE32 hFile;
    HANDLE32 hProcess;
    HANDLE32 hThread;
    LPVOID lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO;

typedef struct _EXIT_THREAD_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO;

typedef struct _EXIT_PROCESS_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO;

typedef struct _LOAD_DLL_DEBUG_INFO {
    HANDLE32 hFile;
    LPVOID   lpBaseOfDll;
    DWORD    dwDebugInfoFileOffset;
    DWORD    nDebugInfoSize;
    LPVOID   lpImageName;
    WORD     fUnicode;
} LOAD_DLL_DEBUG_INFO;

typedef struct _UNLOAD_DLL_DEBUG_INFO {
    LPVOID lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO;

typedef struct _OUTPUT_DEBUG_STRING_INFO {
    LPSTR lpDebugStringData;
    WORD  fUnicode;
    WORD  nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO;

typedef struct _RIP_INFO {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO;

typedef struct _DEBUG_EVENT {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO      Exception;
        CREATE_THREAD_DEBUG_INFO  CreateThread;
        CREATE_PROCESS_DEBUG_INFO CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO    ExitThread;
        EXIT_PROCESS_DEBUG_INFO   ExitProcess;
        LOAD_DLL_DEBUG_INFO       LoadDll;
        UNLOAD_DLL_DEBUG_INFO     UnloadDll;
        OUTPUT_DEBUG_STRING_INFO  DebugString;
        RIP_INFO                  RipInfo;
    } u;
} DEBUG_EVENT, *LPDEBUG_EVENT;


/***** Dialogs *****/

  /* cbWndExtra bytes for dialog class */
#define DLGWINDOWEXTRA      30

  /* Dialog styles */
#define DS_ABSALIGN		0x0001
#define DS_SYSMODAL		0x0002
#define DS_3DLOOK		0x0004	/* win95 */
#define DS_FIXEDSYS		0x0008	/* win95 */
#define DS_NOFAILCREATE		0x0010	/* win95 */
#define DS_LOCALEDIT		0x0020
#define DS_SETFONT		0x0040
#define DS_MODALFRAME		0x0080
#define DS_NOIDLEMSG		0x0100
#define DS_SETFOREGROUND	0x0200	/* win95 */
#define DS_CONTROL		0x0400	/* win95 */
#define DS_CENTER		0x0800	/* win95 */
#define DS_CENTERMOUSE		0x1000	/* win95 */
#define DS_CONTEXTHELP		0x2000	/* win95 */


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
#define IDCLOSE             8
#define IDHELP              9      


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

DECL_WINELIB_TYPE(MSG)
DECL_WINELIB_TYPE(LPMSG)
	
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
#define COLOR_3DDKSHADOW           21
#define COLOR_3DLIGHT              22
#define COLOR_INFOTEXT             23
#define COLOR_INFOBK               24
#define COLOR_DESKTOP              COLOR_BACKGROUND
#define COLOR_3DFACE               COLOR_BTNFACE
#define COLOR_3DSHADOW             COLOR_BTNSHADOW
#define COLOR_3DHIGHLIGHT          COLOR_BTNHIGHLIGHT
#define COLOR_3DHILIGHT            COLOR_BTNHIGHLIGHT
#define COLOR_BTNHILIGHT           COLOR_BTNHIGHLIGHT

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

DECL_WINELIB_TYPE(BITMAP)
DECL_WINELIB_TYPE(LPBITMAP)

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

DECL_WINELIB_TYPE(LOGBRUSH)
DECL_WINELIB_TYPE(LPLOGBRUSH)

  /* Brush styles */
#define BS_SOLID	    0
#define BS_NULL		    1
#define BS_HOLLOW	    1
#define BS_HATCHED	    2
#define BS_PATTERN	    3
#define BS_INDEXED	    4
#define	BS_DIBPATTERN	    5
#define	BS_DIBPATTERNPT	    6
#define BS_PATTERN8X8	    7
#define	BS_DIBPATTERN8X8    8
#define BS_MONOPATTERN      9

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

#define RASTER_FONTTYPE     0x0001
#define DEVICE_FONTTYPE     0x0002
#define TRUETYPE_FONTTYPE   0x0004

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

DECL_WINELIB_TYPE_AW(LOGFONT)
DECL_WINELIB_TYPE_AW(LPLOGFONT)

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

DECL_WINELIB_TYPE_AW(ENUMLOGFONT)
DECL_WINELIB_TYPE_AW(LPENUMLOGFONT)
DECL_WINELIB_TYPE_AW(LPENUMLOGFONTEX)

typedef struct
{
  DWORD fsUsb[4];
  DWORD fsCsb[2];
} FONTSIGNATURE,*LPFONTSIGNATURE;

typedef struct 
{
  UINT32	ciCharset;
  UINT32	ciACP;
  FONTSIGNATURE	fs;
} CHARSETINFO,*LPCHARSETINFO;

/* Flags for ModifyWorldTransform */
#define MWT_IDENTITY      1
#define MWT_LEFTMULTIPLY  2
#define MWT_RIGHTMULTIPLY 3

typedef struct
{
    FLOAT  eM11;
    FLOAT  eM12;
    FLOAT  eM21;
    FLOAT  eM22;
    FLOAT  eDx;
    FLOAT  eDy;
} XFORM, *LPXFORM;

typedef struct 
{
    INT16  txfHeight;
    INT16  txfWidth;
    INT16  txfEscapement;
    INT16  txfOrientation;
    INT16  txfWeight;
    CHAR   txfItalic;
    CHAR   txfUnderline;
    CHAR   txfStrikeOut;
    CHAR   txfOutPrecision;
    CHAR   txfClipPrecision;
    INT16  txfAccelerator WINE_PACKED;
    INT16  txfOverhang WINE_PACKED;
} TEXTXFORM16, *LPTEXTXFORM16;

typedef struct
{
    INT16 dfType;
    INT16 dfPoints;
    INT16 dfVertRes;
    INT16 dfHorizRes;
    INT16 dfAscent;
    INT16 dfInternalLeading;
    INT16 dfExternalLeading;
    CHAR  dfItalic;
    CHAR  dfUnderline;
    CHAR  dfStrikeOut;
    INT16 dfWeight;
    CHAR  dfCharSet;
    INT16 dfPixWidth;
    INT16 dfPixHeight;
    CHAR  dfPitchAndFamily;
    INT16 dfAvgWidth;
    INT16 dfMaxWidth;
    CHAR  dfFirstChar;
    CHAR  dfLastChar;
    CHAR  dfDefaultChar;
    CHAR  dfBreakChar;
    INT16 dfWidthBytes;
    LONG  dfDevice;
    LONG  dfFace;
    LONG  dfBitsPointer;
    LONG  dfBitsOffset;
    CHAR  dfReserved;
    LONG  dfFlags;
    INT16 dfAspace;
    INT16 dfBspace;
    INT16 dfCspace;
    LONG  dfColorPointer;
    LONG  dfReserved1[4];
} FONTINFO16, *LPFONTINFO16;

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
#define GB2313_CHARSET        134
#define CHINESEBIG5_CHARSET   136
#define GREEK_CHARSET         161	/* CP1253 */
#define TURKISH_CHARSET       162	/* CP1254, -iso8859-9 */
#define HEBREW_CHARSET        177	/* CP1255 */
#define ARABIC_CHARSET        178	/* CP1256 */
#define BALTIC_CHARSET        186	/* CP1257 */
#define RUSSIAN_CHARSET       204	/* CP1251 */
#define EE_CHARSET	      238	/* CP1250, -iso8859-2 */
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

DECL_WINELIB_TYPE_AW(TEXTMETRIC)
DECL_WINELIB_TYPE_AW(LPTEXTMETRIC)

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

DECL_WINELIB_TYPE_AW(NEWTEXTMETRIC)
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRIC)

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

DECL_WINELIB_TYPE_AW(NEWTEXTMETRICEX)
DECL_WINELIB_TYPE_AW(LPNEWTEXTMETRICEX)


typedef INT16 (CALLBACK *FONTENUMPROC16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROC32A)(LPENUMLOGFONT32A,LPNEWTEXTMETRIC32A,
                                          UINT32,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROC32W)(LPENUMLOGFONT32W,LPNEWTEXTMETRIC32W,
                                          UINT32,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROC)

typedef INT16 (CALLBACK *FONTENUMPROCEX16)(SEGPTR,SEGPTR,UINT16,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROCEX32A)(LPENUMLOGFONTEX32A,LPNEWTEXTMETRICEX32A,UINT32,LPARAM);
typedef INT32 (CALLBACK *FONTENUMPROCEX32W)(LPENUMLOGFONTEX32W,LPNEWTEXTMETRICEX32W,UINT32,LPARAM);
DECL_WINELIB_TYPE_AW(FONTENUMPROCEX)

  /* tmPitchAndFamily bits */
#define TMPF_FIXED_PITCH    1		/* means variable pitch */
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

typedef struct
{
    UINT16	gmBlackBoxX;
    UINT16	gmBlackBoxY;
    POINT16	gmptGlyphOrigin;
    INT16	gmCellIncX;
    INT16	gmCellIncY;
} GLYPHMETRICS16, *LPGLYPHMETRICS16;

typedef struct
{
    UINT32	gmBlackBoxX;
    UINT32	gmBlackBoxY;
    POINT32	gmptGlyphOrigin;
    INT16	gmCellIncX;
    INT16	gmCellIncY;
} GLYPHMETRICS32, *LPGLYPHMETRICS32;

DECL_WINELIB_TYPE(GLYPHMETRICS)
DECL_WINELIB_TYPE(LPGLYPHMETRICS)

#define GGO_METRICS         0
#define GGO_BITMAP          1
#define GGO_NATIVE          2

typedef struct
{
    UINT16  fract;
    INT16   value;
} FIXED;

typedef struct
{
     FIXED  eM11;
     FIXED  eM12;
     FIXED  eM21;
     FIXED  eM22;
} MAT2, *LPMAT2;

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

DECL_WINELIB_TYPE(ABC)
DECL_WINELIB_TYPE(LPABC)

  /* Rasterizer status */
typedef struct
{
    INT16 nSize;
    INT16 wFlags;
    INT16 nLanguageID;
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

DECL_WINELIB_TYPE(LOGPEN)
DECL_WINELIB_TYPE(LPLOGPEN)

#define PS_SOLID         0x00000000
#define PS_DASH          0x00000001
#define PS_DOT           0x00000002
#define PS_DASHDOT       0x00000003
#define PS_DASHDOTDOT    0x00000004
#define PS_NULL          0x00000005
#define PS_INSIDEFRAME   0x00000006
#define PS_USERSTYLE     0x00000007
#define PS_ALTERNATE     0x00000008
#define PS_STYLE_MASK    0x0000000f

#define PS_ENDCAP_ROUND  0x00000000
#define PS_ENDCAP_SQUARE 0x00000100
#define PS_ENDCAP_FLAT   0x00000200
#define PS_ENDCAP_MASK   0x00000f00

#define PS_JOIN_ROUND    0x00000000
#define PS_JOIN_BEVEL    0x00001000
#define PS_JOIN_MITER    0x00002000
#define PS_JOIN_MASK     0x0000f000

#define PS_COSMETIC      0x00000000
#define PS_GEOMETRIC     0x00010000
#define PS_TYPE_MASK     0x000f0000

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


  /* Graphics Modes */
#define GM_COMPATIBLE     1
#define GM_ADVANCED       2
#define GM_LAST           2

  /* Arc direction modes */
#define AD_COUNTERCLOCKWISE 1
#define AD_CLOCKWISE        2

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
#define SM_CMOUSEBUTTONS      43
#define SM_CXFIXEDFRAME	      SM_CXDLGFRAME
#define SM_CYFIXEDFRAME	      SM_CYDLGFRAME
#define SM_CXSIZEFRAME	      SM_CXFRAME
#define SM_CYSIZEFRAME	      SM_CYFRAME
#define SM_SECURE	      44
#define SM_CXEDGE	      45
#define SM_CYEDGE	      46
#define SM_CXMINSPACING	      47
#define SM_CYMINSPACING	      48
#define SM_CXSMICON	      49
#define SM_CYSMICON	      50
#define SM_CYSMCAPTION	      51
#define SM_CXSMSIZE	      52
#define SM_CYSMSIZE	      53
#define SM_CXMENUSIZE	      54
#define SM_CYMENUSIZE	      55
#define SM_ARRANGE	      56
#define SM_CXMINIMIZED	      57
#define SM_CYMINIMIZED	      58
#define SM_CXMAXTRACK	      59
#define SM_CYMAXTRACK	      60
#define SM_CXMAXIMIZED	      61
#define SM_CYMAXIMIZED	      62
#define SM_NETWORK	      63
#define SM_CLEANBOOT	      67
#define SM_CXDRAG	      68
#define SM_CYDRAG	      69
#define SM_SHOWSOUNDS	      70
#define SM_CXMENUCHECK	      71
#define SM_CYMENUCHECK	      72
#define SM_SLOWMACHINE	      73
#define SM_MIDEASTENABLED     74
#define SM_MOUSEWHEELPRESENT  75

#define SM_CMETRICS           76

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

typedef struct 
{
	BITMAP32		dsBm;
	BITMAPINFOHEADER	dsBmih;
	DWORD			dsBitfields[3];
	HANDLE32		dshSection;
	DWORD			dsOffset;
} DIBSECTION,*LPDIBSECTION;


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

/* only used by Win32 functions */
typedef struct {
	BOOL32		fIcon;
	DWORD		xHotspot;
	DWORD		yHotspot;
	HBITMAP32	hbmMask;
	HBITMAP32	hbmColor;
} ICONINFO,*LPICONINFO;

#ifdef FSHIFT
/* Gcc on Solaris has a version of this that we don't care about.  */
#undef FSHIFT
#endif

#define	FVIRTKEY	TRUE          /* Assumed to be == TRUE */
#define	FNOINVERT	0x02
#define	FSHIFT		0x04
#define	FCONTROL	0x08
#define	FALT		0x10

typedef struct
{
    BYTE   fVirt;
    WORD   key;
    WORD   cmd;
} ACCEL16, *LPACCEL16;

typedef struct
{
    BYTE   fVirt;
    BYTE   pad0;
    WORD   key;
    WORD   cmd;
} ACCEL32, *LPACCEL32;

DECL_WINELIB_TYPE(ACCEL)
DECL_WINELIB_TYPE(LPACCEL)

/* modifiers for RegisterHotKey */
#define	MOD_ALT		0x0001
#define	MOD_CONTROL	0x0002
#define	MOD_SHIFT	0x0004
#define	MOD_WIN		0x0008

/* ids for RegisterHotKey */
#define	IDHOT_SNAPWINDOW	(-1)    /* SHIFT-PRINTSCRN  */
#define	IDHOT_SNAPDESKTOP	(-2)    /* PRINTSCRN        */

/* Flags for DrawIconEx.  */
#define DI_MASK                 1
#define DI_IMAGE                2
#define DI_NORMAL               (DI_MASK | DI_IMAGE)
#define DI_COMPAT               4
#define DI_DEFAULTSIZE          8

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

/* SetErrorMode values */
#define SEM_FAILCRITICALERRORS      0x0001
#define SEM_NOGPFAULTERRORBOX       0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004
#define SEM_NOOPENFILEERRORBOX      0x8000

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

#define HFILE_ERROR16   ((HFILE16)-1)
#define HFILE_ERROR32   ((HFILE32)-1)
#define HFILE_ERROR     WINELIB_NAME(HFILE_ERROR)

#define DDL_READWRITE	0x0000
#define DDL_READONLY	0x0001
#define DDL_HIDDEN	0x0002
#define DDL_SYSTEM	0x0004
#define DDL_DIRECTORY	0x0010
#define DDL_ARCHIVE	0x0020

#define DDL_POSTMSGS	0x2000
#define DDL_DRIVES	0x4000
#define DDL_EXCLUSIVE	0x8000

typedef struct _ACL {
    BYTE AclRevision;
    BYTE Sbz1;
    WORD AclSize;
    WORD AceCount;
    WORD Sbz2;
} ACL, *LPACL;

typedef struct {
    BYTE Value[6];
} SID_IDENTIFIER_AUTHORITY,*PSID_IDENTIFIER_AUTHORITY,*LPSID_IDENTIFIER_AUTHORITY;

typedef struct _SID {
    BYTE Revision;
    BYTE SubAuthorityCount;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority;
    DWORD SubAuthority[1];
} SID,*PSID,*LPSID;

/* The security attributes structure */
typedef struct
{
    DWORD   nLength;
    LPVOID  lpSecurityDescriptor;
    BOOL32  bInheritHandle;
} SECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;

typedef WORD SECURITY_DESCRIPTOR_CONTROL;

/* The security descriptor structure */
typedef struct {
    BYTE Revision;
    BYTE Sbz1;
    SECURITY_DESCRIPTOR_CONTROL Control;
    LPSID Owner;
    LPSID Group;
    LPACL Sacl;
    LPACL Dacl;
} SECURITY_DESCRIPTOR, *LPSECURITY_DESCRIPTOR;

typedef DWORD   SECURITY_INFORMATION;


/* 64 bit number of 100 nanoseconds intervals since January 1, 1601 */
typedef struct
{
  DWORD  dwLowDateTime;
  DWORD  dwHighDateTime;
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

DECL_WINELIB_TYPE_AW(WIN32_FIND_DATA)
DECL_WINELIB_TYPE_AW(LPWIN32_FIND_DATA)

#define INVALID_HANDLE_VALUE16  ((HANDLE16) -1)
#define INVALID_HANDLE_VALUE32  ((HANDLE32) -1)
#define INVALID_HANDLE_VALUE WINELIB_NAME(INVALID_HANDLE_VALUE)

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

DECL_WINELIB_TYPE(DCB)
DECL_WINELIB_TYPE(LPDCB)

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
/* defines below are for all win versions */
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

#define SPI_GETSOUNDSENTRY        64
#define SPI_SETSOUNDSENTRY        65

/* defines below are for all win versions WINVER >= 0x0400 */
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

#define SPI_GETSERIALKEYS         62
#define SPI_SETSERIALKEYS         63
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
#define SPI_SETSCREENSAVERRUNNING 97
#define SPI_SCREENSAVERRUNNING    SPI_SETSCREENSAVERRUNNING

/* defines below are for all win versions (_WIN32_WINNT >= 0x0400) ||
 *                                        (_WIN32_WINDOWS > 0x0400) */
#define SPI_GETMOUSEHOVERWIDTH    98
#define SPI_SETMOUSEHOVERWIDTH    99
#define SPI_GETMOUSEHOVERHEIGHT   100
#define SPI_SETMOUSEHOVERHEIGHT   101
#define SPI_GETMOUSEHOVERTIME     102
#define SPI_SETMOUSEHOVERTIME     103
#define SPI_GETWHEELSCROLLLINES   104
#define SPI_SETWHEELSCROLLLINES   105

#define SPI_GETSHOWIMEUI          110
#define SPI_SETSHOWIMEUI          111

/* defines below are for all win versions WINVER >= 0x0500 */
#define SPI_GETMOUSESPEED         112
#define SPI_SETMOUSESPEED         113
#define SPI_GETSCREENSAVERRUNNING 114

#define SPI_GETACTIVEWINDOWTRACKING    0x1000
#define SPI_SETACTIVEWINDOWTRACKING    0x1001
#define SPI_GETMENUANIMATION           0x1002
#define SPI_SETMENUANIMATION           0x1003
#define SPI_GETCOMBOBOXANIMATION       0x1004
#define SPI_SETCOMBOBOXANIMATION       0x1005
#define SPI_GETLISTBOXSMOOTHSCROLLING  0x1006
#define SPI_SETLISTBOXSMOOTHSCROLLING  0x1007
#define SPI_GETGRADIENTCAPTIONS        0x1008
#define SPI_SETGRADIENTCAPTIONS        0x1009
#define SPI_GETMENUUNDERLINES          0x100A
#define SPI_SETMENUUNDERLINES          0x100B
#define SPI_GETACTIVEWNDTRKZORDER      0x100C
#define SPI_SETACTIVEWNDTRKZORDER      0x100D
#define SPI_GETHOTTRACKING             0x100E
#define SPI_SETHOTTRACKING             0x100F
#define SPI_GETFOREGROUNDLOCKTIMEOUT   0x2000
#define SPI_SETFOREGROUNDLOCKTIMEOUT   0x2001
#define SPI_GETACTIVEWNDTRKTIMEOUT     0x2002
#define SPI_SETACTIVEWNDTRKTIMEOUT     0x2003
#define SPI_GETFOREGROUNDFLASHCOUNT    0x2004
#define SPI_SETFOREGROUNDFLASHCOUNT    0x2005

/* SystemParametersInfo flags */

#define SPIF_UPDATEINIFILE              1
#define SPIF_SENDWININICHANGE           2
#define SPIF_SENDCHANGE                 SPIF_SENDWININICHANGE

/* flags for HIGHCONTRAST dwFlags field */
#define HCF_HIGHCONTRASTON  0x00000001
#define HCF_AVAILABLE       0x00000002
#define HCF_HOTKEYACTIVE    0x00000004
#define HCF_CONFIRMHOTKEY   0x00000008
#define HCF_HOTKEYSOUND     0x00000010
#define HCF_INDICATOR       0x00000020
#define HCF_HOTKEYAVAILABLE 0x00000040
typedef struct tagHIGHCONTRASTA
{
    UINT32  cbSize;
    DWORD   dwFlags;
    LPSTR   lpszDefaultScheme;
}   HIGHCONTRASTA, *LPHIGHCONTRASTA;

typedef struct tagHIGHCONTRASTW
{
    UINT32  cbSize;
    DWORD   dwFlags;
    LPWSTR  lpszDefaultScheme;
}   HIGHCONTRASTW, *LPHIGHCONTRASTW;

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
#define	WF_HASCPUID     0x2000
#define	WF_WIN32WOW     0x4000	/* undoc */
#define	WF_WLO          0x8000

#define MAKEINTRESOURCE16(i)  (SEGPTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCE32A(i) (LPSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCE32W(i) (LPWSTR)((DWORD)((WORD)(i)))
#define MAKEINTRESOURCE WINELIB_NAME_AW(MAKEINTRESOURCE)

/* Predefined resource types */
#define RT_CURSOR16          MAKEINTRESOURCE16(1)
#define RT_CURSOR32A         MAKEINTRESOURCE32A(1)
#define RT_CURSOR32W         MAKEINTRESOURCE32W(1)
#define RT_CURSOR            WINELIB_NAME_AW(RT_CURSOR)
#define RT_BITMAP16          MAKEINTRESOURCE16(2)
#define RT_BITMAP32A         MAKEINTRESOURCE32A(2)
#define RT_BITMAP32W         MAKEINTRESOURCE32W(2)
#define RT_BITMAP            WINELIB_NAME_AW(RT_BITMAP)
#define RT_ICON16            MAKEINTRESOURCE16(3)
#define RT_ICON32A           MAKEINTRESOURCE32A(3)
#define RT_ICON32W           MAKEINTRESOURCE32W(3)
#define RT_ICON              WINELIB_NAME_AW(RT_ICON)
#define RT_MENU16            MAKEINTRESOURCE16(4)
#define RT_MENU32A           MAKEINTRESOURCE32A(4)
#define RT_MENU32W           MAKEINTRESOURCE32W(4)
#define RT_MENU              WINELIB_NAME_AW(RT_MENU)
#define RT_DIALOG16          MAKEINTRESOURCE16(5)
#define RT_DIALOG32A         MAKEINTRESOURCE32A(5)
#define RT_DIALOG32W         MAKEINTRESOURCE32W(5)
#define RT_DIALOG            WINELIB_NAME_AW(RT_DIALOG)
#define RT_STRING16          MAKEINTRESOURCE16(6)
#define RT_STRING32A         MAKEINTRESOURCE32A(6)
#define RT_STRING32W         MAKEINTRESOURCE32W(6)
#define RT_STRING            WINELIB_NAME_AW(RT_STRING)
#define RT_FONTDIR16         MAKEINTRESOURCE16(7)
#define RT_FONTDIR32A        MAKEINTRESOURCE32A(7)
#define RT_FONTDIR32W        MAKEINTRESOURCE32W(7)
#define RT_FONTDIR           WINELIB_NAME_AW(RT_FONTDIR)
#define RT_FONT16            MAKEINTRESOURCE16(8)
#define RT_FONT32A           MAKEINTRESOURCE32A(8)
#define RT_FONT32W           MAKEINTRESOURCE32W(8)
#define RT_FONT              WINELIB_NAME_AW(RT_FONT)
#define RT_ACCELERATOR16     MAKEINTRESOURCE16(9)
#define RT_ACCELERATOR32A    MAKEINTRESOURCE32A(9)
#define RT_ACCELERATOR32W    MAKEINTRESOURCE32W(9)
#define RT_ACCELERATOR       WINELIB_NAME_AW(RT_ACCELERATOR)
#define RT_RCDATA16          MAKEINTRESOURCE16(10)
#define RT_RCDATA32A         MAKEINTRESOURCE32A(10)
#define RT_RCDATA32W         MAKEINTRESOURCE32W(10)
#define RT_RCDATA            WINELIB_NAME_AW(RT_RCDATA)
#define RT_MESSAGELIST16     MAKEINTRESOURCE16(11)
#define RT_MESSAGELIST32A    MAKEINTRESOURCE32A(11)
#define RT_MESSAGELIST32W    MAKEINTRESOURCE32W(11)
#define RT_MESSAGELIST       WINELIB_NAME_AW(RT_MESSAGELIST)
#define RT_GROUP_CURSOR16    MAKEINTRESOURCE16(12)
#define RT_GROUP_CURSOR32A   MAKEINTRESOURCE32A(12)
#define RT_GROUP_CURSOR32W   MAKEINTRESOURCE32W(12)
#define RT_GROUP_CURSOR      WINELIB_NAME_AW(RT_GROUP_CURSOR)
#define RT_GROUP_ICON16      MAKEINTRESOURCE16(14)
#define RT_GROUP_ICON32A     MAKEINTRESOURCE32A(14)
#define RT_GROUP_ICON32W     MAKEINTRESOURCE32W(14)
#define RT_GROUP_ICON        WINELIB_NAME_AW(RT_GROUP_ICON)

/* Predefined resources */
#define IDI_APPLICATION16  MAKEINTRESOURCE16(32512)
#define IDI_APPLICATION32A MAKEINTRESOURCE32A(32512)
#define IDI_APPLICATION32W MAKEINTRESOURCE32W(32512)
#define IDI_APPLICATION    WINELIB_NAME_AW(IDI_APPLICATION)
#define IDI_HAND16         MAKEINTRESOURCE16(32513)
#define IDI_HAND32A        MAKEINTRESOURCE32A(32513)
#define IDI_HAND32W        MAKEINTRESOURCE32W(32513)
#define IDI_HAND           WINELIB_NAME_AW(IDI_HAND)
#define IDI_QUESTION16     MAKEINTRESOURCE16(32514)
#define IDI_QUESTION32A    MAKEINTRESOURCE32A(32514)
#define IDI_QUESTION32W    MAKEINTRESOURCE32W(32514)
#define IDI_QUESTION       WINELIB_NAME_AW(IDI_QUESTION)
#define IDI_EXCLAMATION16  MAKEINTRESOURCE16(32515)
#define IDI_EXCLAMATION32A MAKEINTRESOURCE32A(32515)
#define IDI_EXCLAMATION32W MAKEINTRESOURCE32W(32515)
#define IDI_EXCLAMATION    WINELIB_NAME_AW(IDI_EXCLAMATION)
#define IDI_ASTERISK16     MAKEINTRESOURCE16(32516)
#define IDI_ASTERISK32A    MAKEINTRESOURCE32A(32516)
#define IDI_ASTERISK32W    MAKEINTRESOURCE32W(32516)
#define IDI_ASTERISK       WINELIB_NAME_AW(IDI_ASTERISK)

#define IDC_BUMMER16       MAKEINTRESOURCE16(100)
#define IDC_BUMMER32A      MAKEINTRESOURCE32A(100)
#define IDC_BUMMER32W      MAKEINTRESOURCE32W(100)
#define IDC_BUMMER         WINELIB_NAME_AW(IDC_BUMMER)
#define IDC_ARROW16        MAKEINTRESOURCE16(32512)
#define IDC_ARROW32A       MAKEINTRESOURCE32A(32512)
#define IDC_ARROW32W       MAKEINTRESOURCE32W(32512)
#define IDC_ARROW          WINELIB_NAME_AW(IDC_ARROW)
#define IDC_IBEAM16        MAKEINTRESOURCE16(32513)
#define IDC_IBEAM32A       MAKEINTRESOURCE32A(32513)
#define IDC_IBEAM32W       MAKEINTRESOURCE32W(32513)
#define IDC_IBEAM          WINELIB_NAME_AW(IDC_IBEAM)
#define IDC_WAIT16         MAKEINTRESOURCE16(32514)
#define IDC_WAIT32A        MAKEINTRESOURCE32A(32514)
#define IDC_WAIT32W        MAKEINTRESOURCE32W(32514)
#define IDC_WAIT           WINELIB_NAME_AW(IDC_WAIT)
#define IDC_CROSS16        MAKEINTRESOURCE16(32515)
#define IDC_CROSS32A       MAKEINTRESOURCE32A(32515)
#define IDC_CROSS32W       MAKEINTRESOURCE32W(32515)
#define IDC_CROSS          WINELIB_NAME_AW(IDC_CROSS)
#define IDC_UPARROW16      MAKEINTRESOURCE16(32516)
#define IDC_UPARROW32A     MAKEINTRESOURCE32A(32516)
#define IDC_UPARROW32W     MAKEINTRESOURCE32W(32516)
#define IDC_UPARROW        WINELIB_NAME_AW(IDC_UPARROW)
#define IDC_SIZE16         MAKEINTRESOURCE16(32640)
#define IDC_SIZE32A        MAKEINTRESOURCE32A(32640)
#define IDC_SIZE32W        MAKEINTRESOURCE32W(32640)
#define IDC_SIZE           WINELIB_NAME_AW(IDC_SIZE)
#define IDC_ICON16         MAKEINTRESOURCE16(32641)
#define IDC_ICON32A        MAKEINTRESOURCE32A(32641)
#define IDC_ICON32W        MAKEINTRESOURCE32W(32641)
#define IDC_ICON           WINELIB_NAME_AW(IDC_ICON)
#define IDC_SIZENWSE16     MAKEINTRESOURCE16(32642)
#define IDC_SIZENWSE32A    MAKEINTRESOURCE32A(32642)
#define IDC_SIZENWSE32W    MAKEINTRESOURCE32W(32642)
#define IDC_SIZENWSE       WINELIB_NAME_AW(IDC_SIZENWSE)
#define IDC_SIZENESW16     MAKEINTRESOURCE16(32643)
#define IDC_SIZENESW32A    MAKEINTRESOURCE32A(32643)
#define IDC_SIZENESW32W    MAKEINTRESOURCE32W(32643)
#define IDC_SIZENESW       WINELIB_NAME_AW(IDC_SIZENESW)
#define IDC_SIZEWE16       MAKEINTRESOURCE16(32644)
#define IDC_SIZEWE32A      MAKEINTRESOURCE32A(32644)
#define IDC_SIZEWE32W      MAKEINTRESOURCE32W(32644)
#define IDC_SIZEWE         WINELIB_NAME_AW(IDC_SIZEWE)
#define IDC_SIZENS16       MAKEINTRESOURCE16(32645)
#define IDC_SIZENS32A      MAKEINTRESOURCE32A(32645)
#define IDC_SIZENS32W      MAKEINTRESOURCE32W(32645)
#define IDC_SIZENS         WINELIB_NAME_AW(IDC_SIZENS)
#define IDC_NO16           MAKEINTRESOURCE16(32648)
#define IDC_NO32A          MAKEINTRESOURCE32A(32648)
#define IDC_NO32W          MAKEINTRESOURCE32W(32648)
#define IDC_NO             WINELIB_NAME_AW(IDC_NO)
#define IDC_APPSTARTING16  MAKEINTRESOURCE16(32650)
#define IDC_APPSTARTING32A MAKEINTRESOURCE32A(32650)
#define IDC_APPSTARTING32W MAKEINTRESOURCE32W(32650)
#define IDC_APPSTARTING    WINELIB_NAME_AW(IDC_APPSTARTING)
#define IDC_HELP16         MAKEINTRESOURCE16(32651)
#define IDC_HELP32A        MAKEINTRESOURCE32A(32651)
#define IDC_HELP32W        MAKEINTRESOURCE32W(32651)
#define IDC_HELP           WINELIB_NAME_AW(IDC_HELP)

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

/* Wine extension, I think.  */
#define OBM_RADIOCHECK      32727

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
#define OCR_NO              32648
#define OCR_APPSTARTING     32650
#define OCR_HELP            32651  /* only defined in wine */

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
#define DEFAULT_GUI_FONT    17

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

#define WM_COMPACTING		0x0041

#define WM_COMMNOTIFY		0x0044
#define WM_WINDOWPOSCHANGING 	0x0046
#define WM_WINDOWPOSCHANGED 	0x0047
#define WM_POWER		0x0048

  /* Win32 4.0 messages */
#define WM_COPYDATA		0x004a
#define WM_CANCELJOURNAL	0x004b
#define WM_NOTIFY		0x004e
#define WM_HELP			0x0053
#define WM_NOTIFYFORMAT		0x0055

#define WM_CONTEXTMENU		0x007b
#define WM_STYLECHANGING 	0x007c
#define WM_STYLECHANGED		0x007d

#define WM_GETICON		0x007f
#define WM_SETICON		0x0080

  /* Non-client system messages */
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

  /* Win32 4.0 messages */
#define WM_SIZING	    0x0214
#define WM_CAPTURECHANGED   0x0215
#define WM_MOVING	    0x0216

  /* MDI messages */
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

  /* D&D messages */
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
#define WM_HOTKEY	     0x0312

#define WM_PRINT             0x0317
#define WM_PRINTCLIENT       0x0318

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

#define HWND_DESKTOP        ((HWND32)0)
#define HWND_BROADCAST      ((HWND32)0xffff)

/* SetWindowPos() hwndInsertAfter field values */
#define HWND_TOP            ((HWND32)0)
#define HWND_BOTTOM         ((HWND32)1)
#define HWND_TOPMOST        ((HWND32)-1)
#define HWND_NOTOPMOST      ((HWND32)-2)

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
#define MF_DEFAULT         0x1000
#define MF_SYSMENU         0x2000
#define MF_HELP            0x4000
#define MF_RIGHTJUSTIFY    0x4000
#define MF_MOUSESELECT     0x8000

/* Flags for extended menu item types.  */
#define MFT_STRING         MF_STRING
#define MFT_BITMAP         MF_BITMAP
#define MFT_MENUBARBREAK   MF_MENUBARBREAK
#define MFT_MENUBREAK      MF_MENUBREAK
#define MFT_OWNERDRAW      MF_OWNERDRAW
#define MFT_RADIOCHECK     0x00000200L
#define MFT_SEPARATOR      MF_SEPARATOR
#define MFT_RIGHTORDER     0x00002000L
#define MFT_RIGHTJUSTIFY   MF_RIGHTJUSTIFY

/* Flags for extended menu item states.  */
#define MFS_GRAYED          0x00000003L
#define MFS_DISABLED        MFS_GRAYED
#define MFS_CHECKED         MF_CHECKED
#define MFS_HILITE          MF_HILITE
#define MFS_ENABLED         MF_ENABLED
#define MFS_UNCHECKED       MF_UNCHECKED
#define MFS_UNHILITE        MF_UNHILITE
#define MFS_DEFAULT         MF_DEFAULT

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
  LPSTR     dwTypeData;
  UINT16    cch;
} MENUITEMINFO16, *LPMENUITEMINFO16;

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


#ifndef NOWINOFFSETS
#define GCW_HBRBACKGROUND (-10)
#endif

#define MB_OK			0x00000000
#define MB_OKCANCEL		0x00000001
#define MB_ABORTRETRYIGNORE	0x00000002
#define MB_YESNOCANCEL		0x00000003
#define MB_YESNO		0x00000004
#define MB_RETRYCANCEL		0x00000005
#define MB_TYPEMASK		0x0000000F

#define MB_ICONHAND		0x00000010
#define MB_ICONQUESTION		0x00000020
#define MB_ICONEXCLAMATION	0x00000030
#define MB_ICONASTERISK		0x00000040
#define	MB_USERICON		0x00000080
#define MB_ICONMASK		0x000000F0

#define MB_ICONINFORMATION	MB_ICONASTERISK
#define MB_ICONSTOP		MB_ICONHAND
#define MB_ICONWARNING		MB_ICONEXCLAMATION
#define MB_ICONERROR		MB_ICONHAND

#define MB_DEFBUTTON1		0x00000000
#define MB_DEFBUTTON2		0x00000100
#define MB_DEFBUTTON3		0x00000200
#define MB_DEFBUTTON4		0x00000300
#define MB_DEFMASK		0x00000F00

#define MB_APPLMODAL		0x00000000
#define MB_SYSTEMMODAL		0x00001000
#define MB_TASKMODAL		0x00002000
#define MB_MODEMASK		0x00003000

#define MB_HELP			0x00004000
#define MB_NOFOCUS		0x00008000
#define MB_MISCMASK		0x0000C000

#define MB_SETFOREGROUND	0x00010000
#define MB_DEFAULT_DESKTOP_ONLY	0x00020000
#define MB_SERVICE_NOTIFICATION	0x00040000
#define MB_TOPMOST		0x00040000
#define MB_RIGHT		0x00080000
#define MB_RTLREADING		0x00100000


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

/* DrawFrameControl() uType's */

#define DFC_CAPTION             1
#define DFC_MENU                2
#define DFC_SCROLL              3
#define DFC_BUTTON              4

/* uState's */

#define DFCS_CAPTIONCLOSE       0x0000
#define DFCS_CAPTIONMIN         0x0001
#define DFCS_CAPTIONMAX         0x0002
#define DFCS_CAPTIONRESTORE     0x0003
#define DFCS_CAPTIONHELP        0x0004		/* Windows 95 only */

#define DFCS_MENUARROW          0x0000
#define DFCS_MENUCHECK          0x0001
#define DFCS_MENUBULLET         0x0002
#define DFCS_MENUARROWRIGHT     0x0004

#define DFCS_SCROLLUP            0x0000
#define DFCS_SCROLLDOWN          0x0001
#define DFCS_SCROLLLEFT          0x0002
#define DFCS_SCROLLRIGHT         0x0003
#define DFCS_SCROLLCOMBOBOX      0x0005
#define DFCS_SCROLLSIZEGRIP      0x0008
#define DFCS_SCROLLSIZEGRIPRIGHT 0x0010

#define DFCS_BUTTONCHECK        0x0000
#define DFCS_BUTTONRADIOIMAGE   0x0001
#define DFCS_BUTTONRADIOMASK    0x0002		/* to draw nonsquare button */
#define DFCS_BUTTONRADIO        0x0004
#define DFCS_BUTTON3STATE       0x0008
#define DFCS_BUTTONPUSH         0x0010

/* additional state of the control */

#define DFCS_INACTIVE           0x0100
#define DFCS_PUSHED             0x0200
#define DFCS_CHECKED            0x0400
#define DFCS_ADJUSTRECT         0x2000		/* exclude surrounding edge */
#define DFCS_FLAT               0x4000
#define DFCS_MONO               0x8000

/* DrawState defines ... */
typedef BOOL16 (CALLBACK *DRAWSTATEPROC16)(HDC16,LPARAM,WPARAM16,INT16,INT16);
typedef BOOL32 (CALLBACK *DRAWSTATEPROC32)(HDC32,LPARAM,WPARAM32,INT32,INT32);
DECL_WINELIB_TYPE(DRAWSTATEPROC)

/* Image type */
#define	DST_COMPLEX	0x0000
#define	DST_TEXT	0x0001
#define	DST_PREFIXTEXT	0x0002
#define	DST_ICON	0x0003
#define	DST_BITMAP	0x0004

/* State type */
#define	DSS_NORMAL	0x0000
#define	DSS_UNION	0x0010  /* Gray string appearance */
#define	DSS_DISABLED	0x0020
#define	DSS_DEFAULT	0x0040  /* Make it bold */
#define	DSS_MONO	0x0080
#define	DSS_RIGHT	0x8000

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

#define SS_OWNERDRAW        0x0000000DL
#define SS_BITMAP           0x0000000EL
#define SS_ENHMETAFILE      0x0000000FL

#define SS_ETCHEDHORZ       0x00000010L
#define SS_ETCHEDVERT       0x00000011L
#define SS_ETCHEDFRAME      0x00000012L
#define SS_TYPEMASK         0x0000001FL

#define SS_NOPREFIX         0x00000080L
#define SS_NOTIFY           0x00000100L
#define SS_CENTERIMAGE      0x00000200L
#define SS_RIGHTJUST        0x00000400L
#define SS_REALSIZEIMAGE    0x00000800L
#define SS_SUNKEN           0x00001000L

/* Static Control Messages */
#define STM_SETICON16       (WM_USER+0)
#define STM_SETICON32       0x0170
#define STM_SETICON	    WINELIB_NAME(STM_SETICON)
#define STM_GETICON16       (WM_USER+1)
#define STM_GETICON32       0x0171
#define STM_GETICON	    WINELIB_NAME(STM_GETICON)
#define STM_SETIMAGE        0x0172
#define STM_GETIMAGE        0x0173

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

#define CBS_UPPERCASE	      0x2000L
#define CBS_LOWERCASE	      0x4000L


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
#define ES_LEFT         0x00000000
#define ES_CENTER       0x00000001
#define ES_RIGHT        0x00000002
#define ES_MULTILINE    0x00000004
#define ES_UPPERCASE    0x00000008
#define ES_LOWERCASE    0x00000010
#define ES_PASSWORD     0x00000020
#define ES_AUTOVSCROLL  0x00000040
#define ES_AUTOHSCROLL  0x00000080
#define ES_NOHIDESEL    0x00000100
#define ES_OEMCONVERT   0x00000400
#define ES_READONLY     0x00000800
#define ES_WANTRETURN   0x00001000
#define ES_NUMBER       0x00002000

/* Edit control messages */
#define EM_GETSEL16                (WM_USER+0)
#define EM_GETSEL32                0x00b0
#define EM_GETSEL                  WINELIB_NAME(EM_GETSEL)
#define EM_SETSEL16                (WM_USER+1)
#define EM_SETSEL32                0x00b1
#define EM_SETSEL                  WINELIB_NAME(EM_SETSEL)
#define EM_GETRECT16               (WM_USER+2)
#define EM_GETRECT32               0x00b2
#define EM_GETRECT                 WINELIB_NAME(EM_GETRECT)
#define EM_SETRECT16               (WM_USER+3)
#define EM_SETRECT32               0x00b3
#define EM_SETRECT                 WINELIB_NAME(EM_SETRECT)
#define EM_SETRECTNP16             (WM_USER+4)
#define EM_SETRECTNP32             0x00b4
#define EM_SETRECTNP               WINELIB_NAME(EM_SETRECTNP)
#define EM_SCROLL16                (WM_USER+5)
#define EM_SCROLL32                0x00b5
#define EM_SCROLL                  WINELIB_NAME(EM_SCROLL)
#define EM_LINESCROLL16            (WM_USER+6)
#define EM_LINESCROLL32            0x00b6
#define EM_LINESCROLL              WINELIB_NAME(EM_LINESCROLL)
#define EM_SCROLLCARET16           (WM_USER+7)
#define EM_SCROLLCARET32           0x00b7
#define EM_SCROLLCARET             WINELIB_NAME(EM_SCROLLCARET)
#define EM_GETMODIFY16             (WM_USER+8)
#define EM_GETMODIFY32             0x00b8
#define EM_GETMODIFY               WINELIB_NAME(EM_GETMODIFY)
#define EM_SETMODIFY16             (WM_USER+9)
#define EM_SETMODIFY32             0x00b9
#define EM_SETMODIFY               WINELIB_NAME(EM_SETMODIFY)
#define EM_GETLINECOUNT16          (WM_USER+10)
#define EM_GETLINECOUNT32          0x00ba
#define EM_GETLINECOUNT            WINELIB_NAME(EM_GETLINECOUNT)
#define EM_LINEINDEX16             (WM_USER+11)
#define EM_LINEINDEX32             0x00bb
#define EM_LINEINDEX               WINELIB_NAME(EM_LINEINDEX)
#define EM_SETHANDLE16             (WM_USER+12)
#define EM_SETHANDLE32             0x00bc
#define EM_SETHANDLE               WINELIB_NAME(EM_SETHANDLE)
#define EM_GETHANDLE16             (WM_USER+13)
#define EM_GETHANDLE32             0x00bd
#define EM_GETHANDLE               WINELIB_NAME(EM_GETHANDLE)
#define EM_GETTHUMB16              (WM_USER+14)
#define EM_GETTHUMB32              0x00be
#define EM_GETTHUMB                WINELIB_NAME(EM_GETTHUMB)
/* FIXME : missing from specs 0x00bf and 0x00c0 */
#define EM_LINELENGTH16            (WM_USER+17)
#define EM_LINELENGTH32            0x00c1
#define EM_LINELENGTH              WINELIB_NAME(EM_LINELENGTH)
#define EM_REPLACESEL16            (WM_USER+18)
#define EM_REPLACESEL32            0x00c2
#define EM_REPLACESEL              WINELIB_NAME(EM_REPLACESEL)
/* FIXME : missing from specs 0x00c3 */
#define EM_GETLINE16               (WM_USER+20)
#define EM_GETLINE32               0x00c4
#define EM_GETLINE                 WINELIB_NAME(EM_GETLINE)
#define EM_LIMITTEXT16             (WM_USER+21)
#define EM_LIMITTEXT32             0x00c5
#define EM_LIMITTEXT               WINELIB_NAME(EM_LIMITTEXT)
#define EM_CANUNDO16               (WM_USER+22)
#define EM_CANUNDO32               0x00c6
#define EM_CANUNDO                 WINELIB_NAME(EM_CANUNDO)
#define EM_UNDO16                  (WM_USER+23)
#define EM_UNDO32                  0x00c7
#define EM_UNDO                    WINELIB_NAME(EM_UNDO)
#define EM_FMTLINES16              (WM_USER+24)
#define EM_FMTLINES32              0x00c8
#define EM_FMTLINES                WINELIB_NAME(EM_FMTLINES)
#define EM_LINEFROMCHAR16          (WM_USER+25)
#define EM_LINEFROMCHAR32          0x00c9
#define EM_LINEFROMCHAR            WINELIB_NAME(EM_LINEFROMCHAR)
/* FIXME : missing from specs 0x00ca */
#define EM_SETTABSTOPS16           (WM_USER+27)
#define EM_SETTABSTOPS32           0x00cb
#define EM_SETTABSTOPS             WINELIB_NAME(EM_SETTABSTOPS)
#define EM_SETPASSWORDCHAR16       (WM_USER+28)
#define EM_SETPASSWORDCHAR32       0x00cc
#define EM_SETPASSWORDCHAR         WINELIB_NAME(EM_SETPASSWORDCHAR)
#define EM_EMPTYUNDOBUFFER16       (WM_USER+29)
#define EM_EMPTYUNDOBUFFER32       0x00cd
#define EM_EMPTYUNDOBUFFER         WINELIB_NAME(EM_EMPTYUNDOBUFFER)
#define EM_GETFIRSTVISIBLELINE16   (WM_USER+30)
#define EM_GETFIRSTVISIBLELINE32   0x00ce
#define EM_GETFIRSTVISIBLELINE     WINELIB_NAME(EM_GETFIRSTVISIBLELINE)
#define EM_SETREADONLY16           (WM_USER+31)
#define EM_SETREADONLY32           0x00cf
#define EM_SETREADONLY             WINELIB_NAME(EM_SETREADONLY)
#define EM_SETWORDBREAKPROC16      (WM_USER+32)
#define EM_SETWORDBREAKPROC32      0x00d0
#define EM_SETWORDBREAKPROC        WINELIB_NAME(EM_SETWORDBREAKPROC)
#define EM_GETWORDBREAKPROC16      (WM_USER+33)
#define EM_GETWORDBREAKPROC32      0x00d1
#define EM_GETWORDBREAKPROC        WINELIB_NAME(EM_GETWORDBREAKPROC)
#define EM_GETPASSWORDCHAR16       (WM_USER+34)
#define EM_GETPASSWORDCHAR32       0x00d2
#define EM_GETPASSWORDCHAR         WINELIB_NAME(EM_GETPASSWORDCHAR)
#define EM_SETMARGINS16            WM_NULL /* not in win16 */
#define EM_SETMARGINS32            0x00d3
#define EM_SETMARGINS              WINELIB_NAME(EM_SETMARGINS)
#define EM_GETMARGINS16            WM_NULL /* not in win16 */
#define EM_GETMARGINS32            0x00d4
#define EM_GETMARGINS              WINELIB_NAME(EM_GETMARGINS)
#define EM_GETLIMITTEXT16          WM_NULL /* not in win16 */
#define EM_GETLIMITTEXT32          0x00d5
#define EM_GETLIMITTEXT            WINELIB_NAME(EM_GETLIMITTEXT)
#define EM_POSFROMCHAR16           WM_NULL /* not in win16 */
#define EM_POSFROMCHAR32           0x00d6
#define EM_POSFROMCHAR             WINELIB_NAME(EM_POSFROMCHAR)
#define EM_CHARFROMPOS16           WM_NULL /* not in win16 */
#define EM_CHARFROMPOS32           0x00d7
#define EM_CHARFROMPOS             WINELIB_NAME(EM_CHARFROMPOS)
/* a name change since win95 */
#define EM_SETLIMITTEXT16          WM_NULL /* no name change in win16 */
#define EM_SETLIMITTEXT32          EM_LIMITTEXT32
#define EM_SETLIMITTEXT            WINELIB_NAME(EM_SETLIMITTEXT)

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

/* New since win95 : EM_SETMARGIN parameters */
#define EC_LEFTMARGIN	0x0001
#define EC_RIGHTMARGIN	0x0002
#define EC_USEFONTINFO	0xffff


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

DECL_WINELIB_TYPE(DRAWITEMSTRUCT)
DECL_WINELIB_TYPE(PDRAWITEMSTRUCT)
DECL_WINELIB_TYPE(LPDRAWITEMSTRUCT)

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

DECL_WINELIB_TYPE(MEASUREITEMSTRUCT)
DECL_WINELIB_TYPE(PMEASUREITEMSTRUCT)
DECL_WINELIB_TYPE(LPMEASUREITEMSTRUCT)

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

DECL_WINELIB_TYPE(DELETEITEMSTRUCT)
DECL_WINELIB_TYPE(LPDELETEITEMSTRUCT)

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

DECL_WINELIB_TYPE(COMPAREITEMSTRUCT)
DECL_WINELIB_TYPE(LPCOMPAREITEMSTRUCT)

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

#define VK_LWIN             0x5B
#define VK_RWIN             0x5C
#define VK_APPS             0x5D
/*                          0x5E-0x5F Unassigned */
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
/*                          0x92-0x9F  Unassigned */
/*
 * differencing between right and left shift/control/alt key.
 * Used only by GetAsyncKeyState() and GetKeyState().
 */
#define VK_LSHIFT           0xA0
#define VK_RSHIFT           0xA1
#define VK_LCONTROL         0xA2
#define VK_RCONTROL         0xA3
#define VK_LMENU            0xA4
#define VK_RMENU            0xA5
/*                          0xA6-0xB9  Unassigned */
#define VK_OEM_1            0xBA
#define VK_OEM_PLUS         0xBB
#define VK_OEM_COMMA        0xBC
#define VK_OEM_MINUS        0xBD
#define VK_OEM_PERIOD       0xBE
#define VK_OEM_2            0xBF
#define VK_OEM_3            0xC0
/*                          0xC1-0xDA  Unassigned */
#define VK_OEM_4            0xDB
#define VK_OEM_5            0xDC
#define VK_OEM_6            0xDD
#define VK_OEM_7            0xDE
/*                          0xDF-0xE4  OEM specific */

#define VK_PROCESSKEY       0xE5

/*                          0xE6       OEM specific */
/*                          0xE7-0xE8  Unassigned */
/*                          0xE9-0xF5  OEM specific */

#define VK_ATTN             0xF6
#define VK_CRSEL            0xF7
#define VK_EXSEL            0xF8
#define VK_EREOF            0xF9
#define VK_PLAY             0xFA
#define VK_ZOOM             0xFB
#define VK_NONAME           0xFC
#define VK_PA1              0xFD
#define VK_OEM_CLEAR        0xFE
  
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
#define CF_ENHMETAFILE      14

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

DECL_WINELIB_TYPE(HANDLETABLE)
DECL_WINELIB_TYPE(LPHANDLETABLE)

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

DECL_WINELIB_TYPE(METAFILEPICT)
DECL_WINELIB_TYPE(LPMETAFILEPICT)

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
#define META_UNKNOWN                 0x0529  /* FIXME: unknown meta magic */

typedef INT16 (CALLBACK *MFENUMPROC16)(HDC16,HANDLETABLE16*,METARECORD*,
                                       INT16,LPARAM);
typedef INT32 (CALLBACK *MFENUMPROC32)(HDC32,HANDLETABLE32*,METARECORD*,
                                       INT32,LPARAM);
DECL_WINELIB_TYPE(MFENUMPROC)

/* enhanced metafile structures and functions */

/* note that ENHMETAHEADER is just a particular kind of ENHMETARECORD,
   ie. the header is just the first record in the metafile */
typedef struct {
    DWORD iType; 
    DWORD nSize; 
    RECT32 rclBounds; 
    RECT32 rclFrame; 
    DWORD dSignature; 
    DWORD nVersion; 
    DWORD nBytes; 
    DWORD nRecords; 
    WORD  nHandles; 
    WORD  sReserved; 
    DWORD nDescription; 
    DWORD offDescription; 
    DWORD nPalEntries; 
    SIZE32 szlDevice; 
    SIZE32 szlMillimeters;
    DWORD cbPixelFormat;
    DWORD offPixelFormat;
    DWORD bOpenGL;
} ENHMETAHEADER, *LPENHMETAHEADER; 

typedef struct {
    DWORD iType; 
    DWORD nSize; 
    DWORD dParm[1]; 
} ENHMETARECORD, *LPENHMETARECORD; 

typedef INT32 (CALLBACK *ENHMFENUMPROC32)(HDC32, LPHANDLETABLE32, 
					  LPENHMETARECORD, INT32, LPVOID);

#define EMR_HEADER	1
#define EMR_POLYBEZIER	2
#define EMR_POLYGON	3
#define EMR_POLYLINE	4
#define EMR_POLYBEZIERTO	5
#define EMR_POLYLINETO	6
#define EMR_POLYPOLYLINE	7
#define EMR_POLYPOLYGON	8
#define EMR_SETWINDOWEXTEX	9
#define EMR_SETWINDOWORGEX	10
#define EMR_SETVIEWPORTEXTEX	11
#define EMR_SETVIEWPORTORGEX	12
#define EMR_SETBRUSHORGEX	13
#define EMR_EOF	14
#define EMR_SETPIXELV	15
#define EMR_SETMAPPERFLAGS	16
#define EMR_SETMAPMODE	17
#define EMR_SETBKMODE	18
#define EMR_SETPOLYFILLMODE	19
#define EMR_SETROP2	20
#define EMR_SETSTRETCHBLTMODE	21
#define EMR_SETTEXTALIGN	22
#define EMR_SETCOLORADJUSTMENT	23
#define EMR_SETTEXTCOLOR	24
#define EMR_SETBKCOLOR	25
#define EMR_OFFSETCLIPRGN	26
#define EMR_MOVETOEX	27
#define EMR_SETMETARGN	28
#define EMR_EXCLUDECLIPRECT	29
#define EMR_INTERSECTCLIPRECT	30
#define EMR_SCALEVIEWPORTEXTEX	31
#define EMR_SCALEWINDOWEXTEX	32
#define EMR_SAVEDC	33
#define EMR_RESTOREDC	34
#define EMR_SETWORLDTRANSFORM	35
#define EMR_MODIFYWORLDTRANSFORM	36
#define EMR_SELECTOBJECT	37
#define EMR_CREATEPEN	38
#define EMR_CREATEBRUSHINDIRECT	39
#define EMR_DELETEOBJECT	40
#define EMR_ANGLEARC	41
#define EMR_ELLIPSE	42
#define EMR_RECTANGLE	43
#define EMR_ROUNDRECT	44
#define EMR_ARC	45
#define EMR_CHORD	46
#define EMR_PIE	47
#define EMR_SELECTPALETTE	48
#define EMR_CREATEPALETTE	49
#define EMR_SETPALETTEENTRIES	50
#define EMR_RESIZEPALETTE	51
#define EMR_REALIZEPALETTE	52
#define EMR_EXTFLOODFILL	53
#define EMR_LINETO	54
#define EMR_ARCTO	55
#define EMR_POLYDRAW	56
#define EMR_SETARCDIRECTION	57
#define EMR_SETMITERLIMIT	58
#define EMR_BEGINPATH	59
#define EMR_ENDPATH	60
#define EMR_CLOSEFIGURE	61
#define EMR_FILLPATH	62
#define EMR_STROKEANDFILLPATH	63
#define EMR_STROKEPATH	64
#define EMR_FLATTENPATH	65
#define EMR_WIDENPATH	66
#define EMR_SELECTCLIPPATH	67
#define EMR_ABORTPATH	68
#define EMR_GDICOMMENT	70
#define EMR_FILLRGN	71
#define EMR_FRAMERGN	72
#define EMR_INVERTRGN	73
#define EMR_PAINTRGN	74
#define EMR_EXTSELECTCLIPRGN	75
#define EMR_BITBLT	76
#define EMR_STRETCHBLT	77
#define EMR_MASKBLT	78
#define EMR_PLGBLT	79
#define EMR_SETDIBITSTODEVICE	80
#define EMR_STRETCHDIBITS	81
#define EMR_EXTCREATEFONTINDIRECTW	82
#define EMR_EXTTEXTOUTA	83
#define EMR_EXTTEXTOUTW	84
#define EMR_POLYBEZIER16	85
#define EMR_POLYGON16	86
#define EMR_POLYLINE16	87
#define EMR_POLYBEZIERTO16	88
#define EMR_POLYLINETO16	89
#define EMR_POLYPOLYLINE16	90
#define EMR_POLYPOLYGON16	91
#define EMR_POLYDRAW16	92
#define EMR_CREATEMONOBRUSH	93
#define EMR_CREATEDIBPATTERNBRUSHPT	94
#define EMR_EXTCREATEPEN	95
#define EMR_POLYTEXTOUTA	96
#define EMR_POLYTEXTOUTW	97
#define EMR_SETICMMODE	98
#define EMR_CREATECOLORSPACE	99
#define EMR_SETCOLORSPACE	100
#define EMR_DELETECOLORSPACE	101
#define EMR_GLSRECORD	102
#define EMR_GLSBOUNDEDRECORD	103
#define EMR_PIXELFORMAT 104

#define ENHMETA_SIGNATURE	1179469088


#ifndef NOLOGERROR

/* LogParamError and LogError values */

/* Error modifier bits */
#define ERR_WARNING             0x8000
#define ERR_PARAM               0x4000

#define ERR_SIZE_MASK           0x3000
#define ERR_BYTE                0x1000
#define ERR_WORD                0x2000
#define ERR_DWORD               0x3000

/* LogParamError() values */

/* Generic parameter values */
#define ERR_BAD_VALUE           0x6001
#define ERR_BAD_FLAGS           0x6002
#define ERR_BAD_INDEX           0x6003
#define ERR_BAD_DVALUE          0x7004
#define ERR_BAD_DFLAGS          0x7005
#define ERR_BAD_DINDEX          0x7006
#define ERR_BAD_PTR             0x7007
#define ERR_BAD_FUNC_PTR        0x7008
#define ERR_BAD_SELECTOR        0x6009
#define ERR_BAD_STRING_PTR      0x700a
#define ERR_BAD_HANDLE          0x600b

/* KERNEL parameter errors */
#define ERR_BAD_HINSTANCE       0x6020
#define ERR_BAD_HMODULE         0x6021
#define ERR_BAD_GLOBAL_HANDLE   0x6022
#define ERR_BAD_LOCAL_HANDLE    0x6023
#define ERR_BAD_ATOM            0x6024
#define ERR_BAD_HFILE           0x6025

/* USER parameter errors */
#define ERR_BAD_HWND            0x6040
#define ERR_BAD_HMENU           0x6041
#define ERR_BAD_HCURSOR         0x6042
#define ERR_BAD_HICON           0x6043
#define ERR_BAD_HDWP            0x6044
#define ERR_BAD_CID             0x6045
#define ERR_BAD_HDRVR           0x6046

/* GDI parameter errors */
#define ERR_BAD_COORDS          0x7060
#define ERR_BAD_GDI_OBJECT      0x6061
#define ERR_BAD_HDC             0x6062
#define ERR_BAD_HPEN            0x6063
#define ERR_BAD_HFONT           0x6064
#define ERR_BAD_HBRUSH          0x6065
#define ERR_BAD_HBITMAP         0x6066
#define ERR_BAD_HRGN            0x6067
#define ERR_BAD_HPALETTE        0x6068
#define ERR_BAD_HMETAFILE       0x6069


/* LogError() values */

/* KERNEL errors */
#define ERR_GALLOC              0x0001
#define ERR_GREALLOC            0x0002
#define ERR_GLOCK               0x0003
#define ERR_LALLOC              0x0004
#define ERR_LREALLOC            0x0005
#define ERR_LLOCK               0x0006
#define ERR_ALLOCRES            0x0007
#define ERR_LOCKRES             0x0008
#define ERR_LOADMODULE          0x0009

/* USER errors */
#define ERR_CREATEDLG           0x0040
#define ERR_CREATEDLG2          0x0041
#define ERR_REGISTERCLASS       0x0042
#define ERR_DCBUSY              0x0043
#define ERR_CREATEWND           0x0044
#define ERR_STRUCEXTRA          0x0045
#define ERR_LOADSTR             0x0046
#define ERR_LOADMENU            0x0047
#define ERR_NESTEDBEGINPAINT    0x0048
#define ERR_BADINDEX            0x0049
#define ERR_CREATEMENU          0x004a

/* GDI errors */
#define ERR_CREATEDC            0x0080
#define ERR_CREATEMETA          0x0081
#define ERR_DELOBJSELECTED      0x0082
#define ERR_SELBITMAP           0x0083



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

#endif /* NOLOGERROR */

/* Win32-specific structures */

typedef struct {
    DWORD dwData;
    DWORD cbData;
    LPVOID lpData;
} COPYDATASTRUCT, *PCOPYDATASTRUCT, *LPCOPYDATASTRUCT;

typedef struct {
    HMENU32 hmenuIn;
    HMENU32 hmenuNext;
    HWND32  hwndNext;
} MDINEXTMENU, *PMDINEXTMENU, *LPMDINEXTMENU;

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
#define MAX_LEADBYTES     12
#define MAX_DEFAULTCHAR   2

typedef struct
{
    UINT32 MaxCharSize;
    BYTE   DefaultChar[MAX_DEFAULTCHAR];
    BYTE   LeadByte[MAX_LEADBYTES];
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

DECL_WINELIB_TYPE_AW(STARTUPINFO)
DECL_WINELIB_TYPE_AW(LPSTARTUPINFO)

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
#define FILE_ATTRIBUTE_READONLY         0x0001
#define FILE_ATTRIBUTE_HIDDEN           0x0002
#define FILE_ATTRIBUTE_SYSTEM           0x0004
#define FILE_ATTRIBUTE_LABEL            0x0008  /* Not in Windows API */
#define FILE_ATTRIBUTE_DIRECTORY        0x0010
#define FILE_ATTRIBUTE_ARCHIVE          0x0020
#define FILE_ATTRIBUTE_NORMAL           0x0080
#define FILE_ATTRIBUTE_TEMPORARY        0x0100
#define FILE_ATTRIBUTE_ATOMIC_WRITE     0x0200
#define FILE_ATTRIBUTE_XACTION_WRITE    0x0400
#define FILE_ATTRIBUTE_COMPRESSED       0x0800

/* File alignments (NT) */
#define	FILE_BYTE_ALIGNMENT		0x00000000
#define	FILE_WORD_ALIGNMENT		0x00000001
#define	FILE_LONG_ALIGNMENT		0x00000003
#define	FILE_QUAD_ALIGNMENT		0x00000007
#define	FILE_OCTA_ALIGNMENT		0x0000000f
#define	FILE_32_BYTE_ALIGNMENT		0x0000001f
#define	FILE_64_BYTE_ALIGNMENT		0x0000003f
#define	FILE_128_BYTE_ALIGNMENT		0x0000007f
#define	FILE_256_BYTE_ALIGNMENT		0x000000ff
#define	FILE_512_BYTE_ALIGNMENT		0x000001ff

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
#define HELP_CONTEXTMENU    0x000a
#define HELP_FINDER	    0x000b
#define HELP_WM_HELP	    0x000c
#define HELP_SETPOPUP_POS   0x000d

#define HELP_TCARD	    0x8000
#define HELP_TCARD_DATA	    0x0010
#define HELP_TCARD_OTHER_CALLER 0x0011


/* ExitWindows() flags */
#define EW_RESTARTWINDOWS   0x0042
#define EW_REBOOTSYSTEM     0x0043
#define EW_EXITANDEXECAPP   0x0044

/* ExitWindowsEx() flags */
#define EWX_LOGOFF           0
#define EWX_SHUTDOWN         1
#define EWX_REBOOT           2
#define EWX_FORCE            4
#define EWX_POWEROFF         8


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

DECL_WINELIB_TYPE_AW(DEVMODE)
DECL_WINELIB_TYPE_AW(LPDEVMODE)

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

/* Flags for PolyDraw and GetPath */
#define PT_CLOSEFIGURE          0x0001
#define PT_LINETO               0x0002
#define PT_BEZIERTO             0x0004
#define PT_MOVETO               0x0006

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

typedef LARGE_INTEGER LUID,*LPLUID; /* locally unique ids */

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


typedef BOOL32 (CALLBACK *CODEPAGE_ENUMPROC32A)(LPSTR);
typedef BOOL32 (CALLBACK *CODEPAGE_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(CODEPAGE_ENUMPROC)
typedef BOOL32 (CALLBACK *LOCALE_ENUMPROC32A)(LPSTR);
typedef BOOL32 (CALLBACK *LOCALE_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(LOCALE_ENUMPROC)

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
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTION32A)(DWORD,LPSTR);
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTION32W)(DWORD,LPWSTR);
DECL_WINELIB_TYPE_AW(LPSERVICE_MAIN_FUNCTION)

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

DECL_WINELIB_TYPE_AW(SERVICE_TABLE_ENTRY)
DECL_WINELIB_TYPE_AW(LPSERVICE_TABLE_ENTRY)

/* Used by: ControlService */
typedef struct _SERVICE_STATUS {
    DWORD dwServiceType;
    DWORD dwCurrentState;
    DWORD dwControlsAccepted;
    DWORD dwWin32ExitCode;
    DWORD dwServiceSpecificExitCode;
    DWORD dwCheckPoint;
    DWORD dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;


/* {G,S}etPriorityClass */
#define	NORMAL_PRIORITY_CLASS	0x00000020
#define	IDLE_PRIORITY_CLASS	0x00000040
#define	HIGH_PRIORITY_CLASS	0x00000080
#define	REALTIME_PRIORITY_CLASS	0x00000100

/* GDI Escape commands */
#define	NEWFRAME		1
#define	ABORTDOC		2
#define	NEXTBAND		3
#define	SETCOLORTABLE		4
#define	GETCOLORTABLE		5
#define	FLUSHOUTPUT		6
#define	DRAFTMODE		7
#define	QUERYESCSUPPORT		8
#define	SETABORTPROC		9
#define	STARTDOC		10
#define	ENDDOC			11
#define	GETPHYSPAGESIZE		12
#define	GETPRINTINGOFFSET	13
#define	GETSCALINGFACTOR	14
#define	MFCOMMENT		15
#define	GETPENWIDTH		16
#define	SETCOPYCOUNT		17
#define	SELECTPAPERSOURCE	18
#define	DEVICEDATA		19
#define	PASSTHROUGH		19
#define	GETTECHNOLGY		20
#define	GETTECHNOLOGY		20 /* yes, both of them */
#define	SETLINECAP		21
#define	SETLINEJOIN		22
#define	SETMITERLIMIT		23
#define	BANDINFO		24
#define	DRAWPATTERNRECT		25
#define	GETVECTORPENSIZE	26
#define	GETVECTORBRUSHSIZE	27
#define	ENABLEDUPLEX		28
#define	GETSETPAPERBINS		29
#define	GETSETPRINTORIENT	30
#define	ENUMPAPERBINS		31
#define	SETDIBSCALING		32
#define	EPSPRINTING		33
#define	ENUMPAPERMETRICS	34
#define	GETSETPAPERMETRICS	35
#define	POSTSCRIPT_DATA		37
#define	POSTSCRIPT_IGNORE	38
#define	MOUSETRAILS		39
#define	GETDEVICEUNITS		42

#define	GETEXTENDEDTEXTMETRICS	256
#define	GETEXTENTTABLE		257
#define	GETPAIRKERNTABLE	258
#define	GETTRACKKERNTABLE	259
#define	EXTTEXTOUT		512
#define	GETFACENAME		513
#define	DOWNLOADFACE		514
#define	ENABLERELATIVEWIDTHS	768
#define	ENABLEPAIRKERNING	769
#define	SETKERNTRACK		770
#define	SETALLJUSTVALUES	771
#define	SETCHARSET		772

#define	STRETCHBLT		2048
#define	GETSETSCREENPARAMS	3072
#define	QUERYDIBSUPPORT		3073
#define	BEGIN_PATH		4096
#define	CLIP_TO_PATH		4097
#define	END_PATH		4098
#define	EXT_DEVICE_CAPS		4099
#define	RESTORE_CTM		4100
#define	SAVE_CTM		4101
#define	SET_ARC_DIRECTION	4102
#define	SET_BACKGROUND_COLOR	4103
#define	SET_POLY_MODE		4104
#define	SET_SCREEN_ANGLE	4105
#define	SET_SPREAD		4106
#define	TRANSFORM_CTM		4107
#define	SET_CLIP_BOX		4108
#define	SET_BOUNDS		4109
#define	SET_MIRROR_MODE		4110
#define	OPENCHANNEL		4110
#define	DOWNLOADHEADER		4111
#define CLOSECHANNEL		4112
#define	POSTSCRIPT_PASSTHROUGH	4115
#define	ENCAPSULATED_POSTSCRIPT	4116

/* Flag returned from Escape QUERYDIBSUPPORT */
#define	QDI_SETDIBITS		1
#define	QDI_GETDIBITS		2
#define	QDI_DIBTOSCREEN		4
#define	QDI_STRETCHDIB		8

/* Spooler Error Codes */
#define	SP_NOTREPORTED	0x4000
#define	SP_ERROR	(-1)
#define	SP_APPABORT	(-2)
#define	SP_USERABORT	(-3)
#define	SP_OUTOFDISK	(-4)
#define	SP_OUTOFMEMORY	(-5)

#define PR_JOBSTATUS	0x0000

typedef BOOL32 (CALLBACK *ENUMRESTYPEPROC32A)(HMODULE32,LPSTR,LONG);
typedef BOOL32 (CALLBACK *ENUMRESTYPEPROC32W)(HMODULE32,LPWSTR,LONG);
typedef BOOL32 (CALLBACK *ENUMRESNAMEPROC32A)(HMODULE32,LPCSTR,LPSTR,LONG);
typedef BOOL32 (CALLBACK *ENUMRESNAMEPROC32W)(HMODULE32,LPCWSTR,LPWSTR,LONG);
typedef BOOL32 (CALLBACK *ENUMRESLANGPROC32A)(HMODULE32,LPCSTR,LPCSTR,WORD,LONG);
typedef BOOL32 (CALLBACK *ENUMRESLANGPROC32W)(HMODULE32,LPCWSTR,LPCWSTR,WORD,LONG);

DECL_WINELIB_TYPE_AW(ENUMRESTYPEPROC)
DECL_WINELIB_TYPE_AW(ENUMRESNAMEPROC)
DECL_WINELIB_TYPE_AW(ENUMRESLANGPROC)

/* Character Type Flags */
#define	CT_CTYPE1		0x00000001	/* usual ctype */
#define	CT_CTYPE2		0x00000002	/* bidirectional layout info */
#define	CT_CTYPE3		0x00000004	/* textprocessing info */

/* CType 1 Flag Bits */
#define C1_UPPER		0x0001
#define C1_LOWER		0x0002
#define C1_DIGIT		0x0004
#define C1_SPACE		0x0008
#define C1_PUNCT		0x0010
#define C1_CNTRL		0x0020
#define C1_BLANK		0x0040
#define C1_XDIGIT		0x0080
#define C1_ALPHA		0x0100

/* CType 2 Flag Bits */
#define	C2_LEFTTORIGHT		0x0001
#define	C2_RIGHTTOLEFT		0x0002
#define	C2_EUROPENUMBER		0x0003
#define	C2_EUROPESEPARATOR	0x0004
#define	C2_EUROPETERMINATOR	0x0005
#define	C2_ARABICNUMBER		0x0006
#define	C2_COMMONSEPARATOR	0x0007
#define	C2_BLOCKSEPARATOR	0x0008
#define	C2_SEGMENTSEPARATOR	0x0009
#define	C2_WHITESPACE		0x000A
#define	C2_OTHERNEUTRAL		0x000B
#define	C2_NOTAPPLICABLE	0x0000

/* CType 3 Flag Bits */
#define	C3_NONSPACING		0x0001
#define	C3_DIACRITIC		0x0002
#define	C3_VOWELMARK		0x0004
#define	C3_SYMBOL		0x0008
#define	C3_KATAKANA		0x0010
#define	C3_HIRAGANA		0x0020
#define	C3_HALFWIDTH		0x0040
#define	C3_FULLWIDTH		0x0080
#define	C3_IDEOGRAPH		0x0100
#define	C3_KASHIDA		0x0200
#define	C3_LEXICAL		0x0400
#define	C3_ALPHA		0x8000
#define	C3_NOTAPPLICABLE	0x0000

/* flags that can be passed to LoadLibraryEx */
#define	DONT_RESOLVE_DLL_REFERENCES	0x00000001
#define	LOAD_LIBRARY_AS_DATAFILE	0x00000002
#define	LOAD_WITH_ALTERED_SEARCH_PATH	0x00000008

typedef struct 
{
    INT16    cbSize;
    SEGPTR   lpszDocName WINE_PACKED;
    SEGPTR   lpszOutput WINE_PACKED;
} DOCINFO16, *LPDOCINFO16;

typedef struct 
{
    INT32    cbSize;
    LPCSTR   lpszDocName;
    LPCSTR   lpszOutput;
    LPCSTR   lpszDatatype;
    DWORD    fwType;
} DOCINFO32A, *LPDOCINFO32A;

typedef struct 
{
    INT32    cbSize;
    LPCWSTR  lpszDocName;
    LPCWSTR  lpszOutput;
    LPCWSTR  lpszDatatype;
    DWORD    fwType;
} DOCINFO32W, *LPDOCINFO32W;

DECL_WINELIB_TYPE_AW(DOCINFO)
DECL_WINELIB_TYPE_AW(LPDOCINFO)

typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPSTR	lpLocalName;
	LPSTR	lpRemoteName;
	LPSTR	lpComment ;
	LPSTR	lpProvider;
} NETRESOURCE32A,*LPNETRESOURCE32A;

typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPWSTR	lpLocalName;
	LPWSTR	lpRemoteName;
	LPWSTR	lpComment ;
	LPWSTR	lpProvider;
} NETRESOURCE32W,*LPNETRESOURCE32W;

DECL_WINELIB_TYPE_AW(NETRESOURCE)
DECL_WINELIB_TYPE_AW(LPNETRESOURCE)

typedef struct {
	DWORD	cbStructure;
	DWORD	dwFlags;
	DWORD	dwSpeed;
	DWORD	dwDelay;
	DWORD	dwOptDataSize;
} NETCONNECTINFOSTRUCT,*LPNETCONNECTINFOSTRUCT;

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

typedef struct {
	UINT32		cbSize;
	INT32		iBorderWidth;
	INT32		iScrollWidth;
	INT32		iScrollHeight;
	INT32		iCaptionWidth;
	INT32		iCaptionHeight;
	LOGFONT32A	lfCaptionFont;
	INT32		iSmCaptionWidth;
	INT32		iSmCaptionHeight;
	LOGFONT32A	lfSmCaptionFont;
	INT32		iMenuWidth;
	INT32		iMenuHeight;
	LOGFONT32A	lfMenuFont;
	LOGFONT32A	lfStatusFont;
	LOGFONT32A	lfMessageFont;
} NONCLIENTMETRICS32A,*LPNONCLIENTMETRICS32A;

typedef struct {
	UINT32		cbSize;
	INT32		iBorderWidth;
	INT32		iScrollWidth;
	INT32		iScrollHeight;
	INT32		iCaptionWidth;
	INT32		iCaptionHeight;
	LOGFONT32W	lfCaptionFont;
	INT32		iSmCaptionWidth;
	INT32		iSmCaptionHeight;
	LOGFONT32W	lfSmCaptionFont;
	INT32		iMenuWidth;
	INT32		iMenuHeight;
	LOGFONT32W	lfMenuFont;
	LOGFONT32W	lfStatusFont;
	LOGFONT32W	lfMessageFont;
} NONCLIENTMETRICS32W,*LPNONCLIENTMETRICS32W;

DECL_WINELIB_TYPE_AW(NONCLIENTMETRICS)
DECL_WINELIB_TYPE_AW(LPNONCLIENTMETRICS)

typedef struct tagANIMATIONINFO
{
       UINT32          cbSize;
       INT32           iMinAnimate;
} ANIMATIONINFO, *LPANIMATIONINFO;

typedef struct tagNMHDR
{
    HWND32  hwndFrom;
    UINT32  idFrom;
    UINT32  code;
} NMHDR, *LPNMHDR;

typedef struct
{
	UINT32	cbSize;
	INT32	iTabLength;
	INT32	iLeftMargin;
	INT32	iRightMargin;
	UINT32	uiLengthDrawn;
} DRAWTEXTPARAMS,*LPDRAWTEXTPARAMS;

/* ifdef _x86_ ... */
typedef struct _LDT_ENTRY {
    WORD	LimitLow;
    WORD	BaseLow;
    union {
	struct {
	    BYTE	BaseMid;
	    BYTE	Flags1;/*Declare as bytes to avoid alignment problems */
	    BYTE	Flags2; 
	    BYTE	BaseHi;
	} Bytes;
	struct {
	    DWORD	BaseMid		: 8;
	    DWORD	Type		: 5;
	    DWORD	Dpl		: 2;
	    DWORD	Pres		: 1;
	    DWORD	LimitHi		: 4;
	    DWORD	Sys		: 1;
	    DWORD	Reserved_0	: 1;
	    DWORD	Default_Big	: 1;
	    DWORD	Granularity	: 1;
	    DWORD	BaseHi		: 8;
	} Bits;
    } HighWord;
} LDT_ENTRY, *LPLDT_ENTRY;

#define	RDH_RECTANGLES  1

typedef struct _RGNDATAHEADER {
    DWORD	dwSize;
    DWORD	iType;
    DWORD	nCount;
    DWORD	nRgnSize;
    RECT32	rcBound;
} RGNDATAHEADER,*LPRGNDATAHEADER;

typedef struct _RGNDATA {
    RGNDATAHEADER	rdh;
    char		Buffer[1];
} RGNDATA,*PRGNDATA,*LPRGNDATA;

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

typedef struct /* not sure if the 16bit version is correct */
{
    UINT32	cbSize;
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

/* for WOWHandle{16,32} */
typedef enum _WOW_HANDLE_TYPE { /* WOW */
    WOW_TYPE_HWND,
    WOW_TYPE_HMENU,
    WOW_TYPE_HDWP,
    WOW_TYPE_HDROP,
    WOW_TYPE_HDC,
    WOW_TYPE_HFONT,
    WOW_TYPE_HMETAFILE,
    WOW_TYPE_HRGN,
    WOW_TYPE_HBITMAP,
    WOW_TYPE_HBRUSH,
    WOW_TYPE_HPALETTE,
    WOW_TYPE_HPEN,
    WOW_TYPE_HACCEL,
    WOW_TYPE_HTASK,
    WOW_TYPE_FULLHWND
} WOW_HANDLE_TYPE;

/* WOWCallback16Ex defines */
#define WCB16_MAX_CBARGS	16
/* ... dwFlags */
#define WCB16_PASCAL		0x0
#define WCB16_CDECL		0x1

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

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard
} GET_FILEEX_INFO_LEVELS;

typedef struct _WIN32_FILE_ATTRIBUTES_DATA {
    DWORD    dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD    nFileSizeHigh;
    DWORD    nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;




#pragma pack(4)

/* Declarations for functions that exist only in Win16 */

#ifdef __WINE__
WORD        WINAPI AllocCStoDSAlias(WORD);
WORD        WINAPI AllocDStoCSAlias(WORD);
HGLOBAL16   WINAPI AllocResource(HINSTANCE16,HRSRC16,DWORD);
WORD        WINAPI AllocSelector(WORD);
WORD        WINAPI AllocSelectorArray(WORD);
VOID        WINAPI CalcChildScroll(HWND16,WORD);
VOID        WINAPI CascadeChildWindows(HWND16,WORD);
INT16       WINAPI CloseComm(INT16);
HGLOBAL16   WINAPI CreateCursorIconIndirect(HINSTANCE16,CURSORICONINFO*,
                                            LPCVOID,LPCVOID);
WORD        WINAPI CreateSystemTimer(WORD,FARPROC16);
BOOL16      WINAPI DCHook(HDC16,WORD,DWORD,LPARAM);
VOID        WINAPI DirectedYield(HTASK16);
HGLOBAL16   WINAPI DirectResAlloc(HINSTANCE16,WORD,UINT16);
VOID        WINAPI DisableSystemTimers(void);
BOOL16      WINAPI DlgDirSelect(HWND16,LPSTR,INT16);
BOOL16      WINAPI DlgDirSelectComboBox(HWND16,LPSTR,INT16);
DWORD       WINAPI DumpIcon(SEGPTR,WORD*,SEGPTR*,SEGPTR*);
BOOL16      WINAPI EnableCommNotification(INT16,HWND16,INT16,INT16);
BOOL16      WINAPI EnableHardwareInput(BOOL16);
VOID        WINAPI EnableSystemTimers(void);
INT16       WINAPI ExcludeVisRect(HDC16,INT16,INT16,INT16,INT16);
HANDLE16    WINAPI FarGetOwner(HGLOBAL16);
VOID        WINAPI FarSetOwner(HGLOBAL16,HANDLE16);
BOOL16      WINAPI FastWindowFrame(HDC16,const RECT16*,INT16,INT16,DWORD);
VOID        WINAPI FileCDR(FARPROC16);
VOID        WINAPI FillWindow(HWND16,HWND16,HDC16,HBRUSH16);
INT16       WINAPI FlushComm(INT16,INT16);
WORD        WINAPI FreeSelector(WORD);
UINT16      WINAPI GDIRealizePalette(HDC16);
HPALETTE16  WINAPI GDISelectPalette(HDC16,HPALETTE16,WORD);
HANDLE16    WINAPI GetAtomHandle(ATOM);
DWORD       WINAPI GetBitmapDimension(HBITMAP16);
DWORD       WINAPI GetBrushOrg(HDC16);
HANDLE16    WINAPI GetCodeHandle(FARPROC16);
INT16       WINAPI GetCommError(INT16,LPCOMSTAT);
UINT16      WINAPI GetCommEventMask(INT16,UINT16);
HBRUSH16    WINAPI GetControlBrush(HWND16,HDC16,UINT16);
VOID        WINAPI GetCodeInfo(FARPROC16,LPVOID);
HANDLE16    WINAPI GetCurrentPDB(void);
DWORD       WINAPI GetCurrentPosition(HDC16);
HTASK16     WINAPI GetCurrentTask(void);
DWORD       WINAPI GetDCHook(HDC16,FARPROC16*);
DWORD       WINAPI GetDCOrg(HDC16);
HDC16       WINAPI GetDCState(HDC16);
HWND16      WINAPI GetDesktopHwnd(void);
SEGPTR      WINAPI GetDOSEnvironment(void);
INT16       WINAPI GetEnvironment(LPCSTR,LPDEVMODE16,UINT16);
HMODULE16   WINAPI GetExePtr(HANDLE16);
WORD        WINAPI GetExeVersion(void);
WORD        WINAPI GetExpWinVer(HMODULE16);
DWORD       WINAPI GetFileResourceSize(LPCSTR,SEGPTR,SEGPTR,LPDWORD);
DWORD       WINAPI GetFileResource(LPCSTR,SEGPTR,SEGPTR,DWORD,DWORD,LPVOID);
DWORD       WINAPI GetHeapSpaces(HMODULE16);
WORD        WINAPI GetIconID(HGLOBAL16,DWORD);
INT16       WINAPI GetInstanceData(HINSTANCE16,WORD,INT16);
HGLOBAL16   WINAPI GetMetaFileBits(HMETAFILE16);
BOOL16      WINAPI GetModuleName(HINSTANCE16,LPSTR,INT16);
INT16       WINAPI GetModuleUsage(HINSTANCE16);
FARPROC16   WINAPI GetMouseEventProc(void);
UINT16      WINAPI GetNumTasks(void);
DWORD       WINAPI GetSelectorBase(WORD);
DWORD       WINAPI GetSelectorLimit(WORD);
HINSTANCE16 WINAPI GetTaskDS(void);
HQUEUE16    WINAPI GetTaskQueue(HTASK16);
BYTE        WINAPI GetTempDrive(BYTE);
DWORD       WINAPI GetTextExtent(HDC16,LPCSTR,INT16);
DWORD       WINAPI GetViewportExt(HDC16);
DWORD       WINAPI GetViewportOrg(HDC16);
BOOL16      WINAPI GetWinDebugInfo(LPWINDEBUGINFO,UINT16);
DWORD       WINAPI GetWindowExt(HDC16);
DWORD       WINAPI GetWindowOrg(HDC16);
DWORD       WINAPI GetWinFlags(void);
DWORD       WINAPI GlobalDOSAlloc(DWORD);
WORD        WINAPI GlobalDOSFree(WORD);
void        WINAPI GlobalFreeAll(HGLOBAL16);
DWORD       WINAPI GlobalHandleNoRIP(WORD);
HGLOBAL16   WINAPI GlobalLRUNewest(HGLOBAL16);
HGLOBAL16   WINAPI GlobalLRUOldest(HGLOBAL16);
VOID        WINAPI GlobalNotify(FARPROC16);
WORD        WINAPI GlobalPageLock(HGLOBAL16);
WORD        WINAPI GlobalPageUnlock(HGLOBAL16);
INT16       WINAPI InitApp(HINSTANCE16);
HRGN16      WINAPI InquireVisRgn(HDC16);
INT16       WINAPI IntersectVisRect(HDC16,INT16,INT16,INT16,INT16);
BOOL16      WINAPI IsDCCurrentPalette(HDC16);
BOOL16      WINAPI IsGDIObject(HGDIOBJ16);
BOOL16      WINAPI IsSharedSelector(HANDLE16);
BOOL16      WINAPI IsTask(HTASK16);
HTASK16     WINAPI IsTaskLocked(void);
BOOL16      WINAPI IsUserIdle(void);
BOOL16      WINAPI IsValidMetaFile(HMETAFILE16);
VOID        WINAPI LogError(UINT16, LPVOID);
VOID        WINAPI LogParamError(UINT16,FARPROC16,LPVOID);
HGLOBAL16   WINAPI LoadCursorIconHandler(HGLOBAL16,HMODULE16,HRSRC16);
HGLOBAL16   WINAPI LoadDIBCursorHandler(HGLOBAL16,HMODULE16,HRSRC16);
HGLOBAL16   WINAPI LoadDIBIconHandler(HGLOBAL16,HMODULE16,HRSRC16);
WORD        WINAPI LocalCountFree(void);
WORD        WINAPI LocalHandleDelta(WORD);
WORD        WINAPI LocalHeapSize(void);
HICON16     WINAPI LoadIconHandler(HGLOBAL16,BOOL16);
BOOL16      WINAPI LocalInit(HANDLE16,WORD,WORD);
FARPROC16   WINAPI LocalNotify(FARPROC16);
HTASK16     WINAPI LockCurrentTask(BOOL16);
HMENU16     WINAPI LookupMenuHandle(HMENU16,INT16);
DWORD       WINAPI MoveTo(HDC16,INT16,INT16);
DWORD       WINAPI OffsetViewportOrg(HDC16,INT16,INT16);
INT16       WINAPI OffsetVisRgn(HDC16,INT16,INT16);
DWORD       WINAPI OffsetWindowOrg(HDC16,INT16,INT16);
VOID        WINAPI OldYield(void);
INT16       WINAPI OpenComm(LPCSTR,UINT16,UINT16);
VOID        WINAPI PaintRect(HWND16,HWND16,HDC16,HBRUSH16,const RECT16*);
VOID        WINAPI PostEvent(HTASK16);
WORD        WINAPI PrestoChangoSelector(WORD,WORD);
INT16       WINAPI ReadComm(INT16,LPSTR,INT16);
UINT16      WINAPI RealizeDefaultPalette(HDC16);
BOOL32      WINAPI RegisterShellHook(HWND16,UINT16);
INT16       WINAPI RestoreVisRgn(HDC16);
HRGN16      WINAPI SaveVisRgn(HDC16);
DWORD       WINAPI ScaleViewportExt(HDC16,INT16,INT16,INT16,INT16);
DWORD       WINAPI ScaleWindowExt(HDC16,INT16,INT16,INT16,INT16);
WORD        WINAPI SelectorAccessRights(WORD,WORD,WORD);
INT16       WINAPI SelectVisRgn(HDC16,HRGN16);
DWORD       WINAPI SetBitmapDimension(HBITMAP16,INT16,INT16);
DWORD       WINAPI SetBrushOrg(HDC16,INT16,INT16);
SEGPTR      WINAPI SetCommEventMask(INT16,UINT16);
BOOL16      WINAPI SetDCHook(HDC16,FARPROC16,DWORD);
DWORD       WINAPI SetDCOrg(HDC16,INT16,INT16);
VOID        WINAPI SetDCState(HDC16,HDC16);
BOOL16      WINAPI SetDeskPattern(void);
INT16       WINAPI SetEnvironment(LPCSTR,LPDEVMODE16,UINT16);
WORD        WINAPI SetHookFlags(HDC16,WORD);
HMETAFILE16 WINAPI SetMetaFileBits(HGLOBAL16);
VOID        WINAPI SetPriority(HTASK16,INT16);
FARPROC16   WINAPI SetResourceHandler(HINSTANCE16,SEGPTR,FARPROC16);
WORD        WINAPI SetSelectorBase(WORD,DWORD);
WORD        WINAPI SetSelectorLimit(WORD,DWORD);
HQUEUE16    WINAPI SetTaskQueue(HTASK16,HQUEUE16);
FARPROC16   WINAPI SetTaskSignalProc(HTASK16,FARPROC16);
DWORD       WINAPI SetViewportExt(HDC16,INT16,INT16);
DWORD       WINAPI SetViewportOrg(HDC16,INT16,INT16);
BOOL16      WINAPI SetWinDebugInfo(LPWINDEBUGINFO);
DWORD       WINAPI SetWindowExt(HDC16,INT16,INT16);
DWORD       WINAPI SetWindowOrg(HDC16,INT16,INT16);
VOID        WINAPI SwitchStackTo(WORD,WORD,WORD);
VOID        WINAPI TileChildWindows(HWND16,WORD);
INT16       WINAPI UngetCommChar(INT16,CHAR);
VOID        WINAPI UserYield(void);
BOOL16      WINAPI WaitEvent(HTASK16);
INT16       WINAPI WriteComm(INT16,LPSTR,INT16);
VOID        WINAPI WriteOutProfiles(VOID);
VOID        WINAPI hmemcpy(LPVOID,LPCVOID,LONG);
#endif  /* __WINE__ */

/* Declarations for functions that exist only in Win32 */

BOOL32      WINAPI AllocConsole(void);
BOOL32      WINAPI AreFileApisANSI(void);
BOOL32      WINAPI Beep(DWORD,DWORD);
BOOL32      WINAPI ClearCommError(INT32,LPDWORD,LPCOMSTAT);
BOOL32      WINAPI CloseHandle(HANDLE32);
BOOL32      WINAPI CloseServiceHandle(HANDLE32);
BOOL32      WINAPI CombineTransform(LPXFORM,const XFORM *,const XFORM *);
INT32       WINAPI CopyAcceleratorTable32A(HACCEL32,LPACCEL32,INT32);
INT32       WINAPI CopyAcceleratorTable32W(HACCEL32,LPACCEL32,INT32);
#define     CopyAcceleratorTable WINELIB_NAME_AW(CopyAcceleratorTable)
HENHMETAFILE32 WINAPI CopyEnhMetaFile32A(HENHMETAFILE32,LPCSTR);
HENHMETAFILE32 WINAPI CopyEnhMetaFile32W(HENHMETAFILE32,LPCWSTR);
#define     CopyEnhMetaFile WINELIB_NAME_AW(CopyEnhMetaFile)
BOOL32      WINAPI CopyFile32A(LPCSTR,LPCSTR,BOOL32);
BOOL32      WINAPI CopyFile32W(LPCWSTR,LPCWSTR,BOOL32);
#define     CopyFile WINELIB_NAME_AW(CopyFile)
INT32       WINAPI CompareFileTime(LPFILETIME,LPFILETIME);
BOOL32      WINAPI ControlService(HANDLE32,DWORD,LPSERVICE_STATUS);
HANDLE32    WINAPI CreateEvent32A(LPSECURITY_ATTRIBUTES,BOOL32,BOOL32,LPCSTR);
HANDLE32    WINAPI CreateEvent32W(LPSECURITY_ATTRIBUTES,BOOL32,BOOL32,LPCWSTR);
#define     CreateEvent WINELIB_NAME_AW(CreateEvent)
HFILE32     WINAPI CreateFile32A(LPCSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                                 DWORD,DWORD,HANDLE32);
HFILE32     WINAPI CreateFile32W(LPCWSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,
                                 DWORD,DWORD,HANDLE32);
#define     CreateFile WINELIB_NAME_AW(CreateFile)
HPALETTE32  WINAPI CreateHalftonePalette(HDC32);
HANDLE32    WINAPI CreateFileMapping32A(HANDLE32,LPSECURITY_ATTRIBUTES,DWORD,
                                        DWORD,DWORD,LPCSTR);
HANDLE32    WINAPI CreateFileMapping32W(HANDLE32,LPSECURITY_ATTRIBUTES,DWORD,
                                        DWORD,DWORD,LPCWSTR);
#define     CreateFileMapping WINELIB_NAME_AW(CreateFileMapping)
HICON32     WINAPI CreateIconIndirect(LPICONINFO);
HANDLE32    WINAPI CreateMutex32A(LPSECURITY_ATTRIBUTES,BOOL32,LPCSTR);
HANDLE32    WINAPI CreateMutex32W(LPSECURITY_ATTRIBUTES,BOOL32,LPCWSTR);
#define     CreateMutex WINELIB_NAME_AW(CreateMutex)
HANDLE32    WINAPI CreateSemaphore32A(LPSECURITY_ATTRIBUTES,LONG,LONG,LPCSTR);
HANDLE32    WINAPI CreateSemaphore32W(LPSECURITY_ATTRIBUTES,LONG,LONG,LPCWSTR);
#define     CreateSemaphore WINELIB_NAME_AW(CreateSemaphore)
HANDLE32    WINAPI CreateThread(LPSECURITY_ATTRIBUTES,DWORD,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
BOOL32      WINAPI DeleteEnhMetaFile(HENHMETAFILE32);
BOOL32      WINAPI DeleteService(HANDLE32);
BOOL32      WINAPI DeregisterEventSource(HANDLE32);
BOOL32      WINAPI DestroyAcceleratorTable(HACCEL32);
BOOL32      WINAPI DisableThreadLibraryCalls(HMODULE32);
BOOL32      WINAPI DosDateTimeToFileTime(WORD,WORD,LPFILETIME);
BOOL32      WINAPI DuplicateHandle(HANDLE32,HANDLE32,HANDLE32,HANDLE32*,DWORD,BOOL32,DWORD);
INT32       WINAPI EnumPropsEx32A(HWND32,PROPENUMPROCEX32A,LPARAM);
INT32       WINAPI EnumPropsEx32W(HWND32,PROPENUMPROCEX32W,LPARAM);
#define     EnumPropsEx WINELIB_NAME_AW(EnumPropsEx)
BOOL32      WINAPI EnumResourceLanguages32A(HMODULE32,LPCSTR,LPCSTR,
                                            ENUMRESLANGPROC32A,LONG);
BOOL32      WINAPI EnumResourceLanguages32W(HMODULE32,LPCWSTR,LPCWSTR,
                                            ENUMRESLANGPROC32W,LONG);
#define     EnumResourceLanguages WINELIB_NAME_AW(EnumResourceLanguages)
BOOL32      WINAPI EnumResourceNames32A(HMODULE32,LPCSTR,ENUMRESNAMEPROC32A,
                                        LONG);
BOOL32      WINAPI EnumResourceNames32W(HMODULE32,LPCWSTR,ENUMRESNAMEPROC32W,
                                        LONG);
#define     EnumResourceNames WINELIB_NAME_AW(EnumResourceNames)
BOOL32      WINAPI EnumResourceTypes32A(HMODULE32,ENUMRESTYPEPROC32A,LONG);
BOOL32      WINAPI EnumResourceTypes32W(HMODULE32,ENUMRESTYPEPROC32W,LONG);
#define     EnumResourceTypes WINELIB_NAME_AW(EnumResourceTypes)
BOOL32      WINAPI EnumSystemCodePages32A(CODEPAGE_ENUMPROC32A,DWORD);
BOOL32      WINAPI EnumSystemCodePages32W(CODEPAGE_ENUMPROC32W,DWORD);
#define     EnumSystemCodePages WINELIB_NAME_AW(EnumSystemCodePages)
BOOL32      WINAPI EnumSystemLocales32A(LOCALE_ENUMPROC32A,DWORD);
BOOL32      WINAPI EnumSystemLocales32W(LOCALE_ENUMPROC32W,DWORD);
#define     EnumSystemLocales WINELIB_NAME_AW(EnumSystemLocales)
BOOL32      WINAPI EnumThreadWindows(DWORD,WNDENUMPROC32,LPARAM);
VOID        WINAPI ExitProcess(DWORD);
VOID        WINAPI ExitThread(DWORD);
BOOL32      WINAPI ExitWindowsEx(UINT32,DWORD);
DWORD       WINAPI ExpandEnvironmentStrings32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI ExpandEnvironmentStrings32W(LPCWSTR,LPWSTR,DWORD);
#define     ExpandEnvironmentStrings WINELIB_NAME_AW(ExpandEnvironmentStrings)
HRGN32      WINAPI ExtCreateRegion(LPXFORM,DWORD,LPRGNDATA);
BOOL32      WINAPI FileTimeToDosDateTime(const FILETIME*,LPWORD,LPWORD);
BOOL32      WINAPI FileTimeToLocalFileTime(const FILETIME*,LPFILETIME);
BOOL32      WINAPI FileTimeToSystemTime(const FILETIME*,LPSYSTEMTIME);
HRSRC32     WINAPI FindResourceEx32A(HMODULE32,LPCSTR,LPCSTR,WORD);
HRSRC32     WINAPI FindResourceEx32W(HMODULE32,LPCWSTR,LPCWSTR,WORD);
#define     FindResourceEx WINELIB_NAME_AW(FindResourceEx)
BOOL32      WINAPI FlushConsoleInputBuffer(HANDLE32);
BOOL32      WINAPI FlushFileBuffers(HFILE32);
DWORD       WINAPI FormatMessage32A(DWORD,LPCVOID,DWORD,DWORD,LPSTR,
				    DWORD,LPDWORD);
#define     FormatMessage WINELIB_NAME_AW(FormatMessage)
BOOL32      WINAPI FreeConsole(void);
BOOL32      WINAPI FreeEnvironmentStrings32A(LPSTR);
BOOL32      WINAPI FreeEnvironmentStrings32W(LPWSTR);
#define     FreeEnvironmentStrings WINELIB_NAME_AW(FreeEnvironmentStrings)
UINT32      WINAPI GetACP(void);
LPCSTR      WINAPI GetCommandLine32A();
LPCWSTR     WINAPI GetCommandLine32W();
#define     GetCommandLine WINELIB_NAME_AW(GetCommandLine)
BOOL32      WINAPI GetCommTimeouts(INT32,LPCOMMTIMEOUTS);
BOOL32      WINAPI GetComputerName32A(LPSTR,LPDWORD);
BOOL32      WINAPI GetComputerName32W(LPWSTR,LPDWORD);
#define     GetComputerName WINELIB_NAME_AW(GetComputerName)
UINT32      WINAPI GetConsoleCP();
BOOL32      WINAPI GetConsoleMode(HANDLE32,LPDWORD);
UINT32      WINAPI GetConsoleOutputCP();
DWORD       WINAPI GetConsoleTitle32A(LPSTR,DWORD);
DWORD       WINAPI GetConsoleTitle32W(LPWSTR,DWORD);
#define     GetConsoleTitle WINELIB_NAME_AW(GetConsoleTitle)
BOOL32      WINAPI GetCPInfo(UINT32,LPCPINFO);
HANDLE32    WINAPI GetCurrentObject(HDC32,UINT32);
HANDLE32    WINAPI GetCurrentProcess(void);
DWORD       WINAPI GetCurrentProcessId(void);
HANDLE32    WINAPI GetCurrentThread(void);
DWORD       WINAPI GetCurrentThreadId(void);
INT32       WINAPI GetDateFormat32A(LCID,DWORD,LPSYSTEMTIME,LPCSTR,LPSTR,INT32);
INT32       WINAPI GetDateFormat32W(LCID,DWORD,LPSYSTEMTIME,LPCWSTR,LPWSTR,INT32);
#define     GetDateFormat WINELIB_NAME_AW(GetDateFormat)
BOOL32      WINAPI GetDCOrgEx(HDC32,LPPOINT32);
HENHMETAFILE32 WINAPI GetEnhMetaFile32A(LPCSTR);
HENHMETAFILE32 WINAPI GetEnhMetaFile32W(LPCWSTR);
#define     GetEnhMetaFile WINELIB_NAME_AW(GetEnhMetaFile)
UINT32      WINAPI GetEnhMetaFileBits(HENHMETAFILE32,UINT32,LPBYTE);
UINT32      WINAPI GetEnhMetaFileHeader(HENHMETAFILE32,UINT32,LPENHMETAHEADER);
LPSTR       WINAPI GetEnvironmentStrings32A(void);
LPWSTR      WINAPI GetEnvironmentStrings32W(void);
#define     GetEnvironmentStrings WINELIB_NAME_AW(GetEnvironmentStrings)
DWORD       WINAPI GetEnvironmentVariable32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetEnvironmentVariable32W(LPCWSTR,LPWSTR,DWORD);
#define     GetEnvironmentVariable WINELIB_NAME_AW(GetEnvironmentVariable)
BOOL32      WINAPI GetFileAttributesEx32A(LPCSTR,GET_FILEEX_INFO_LEVELS,LPVOID);
BOOL32      WINAPI GetFileAttributesEx32W(LPCWSTR,GET_FILEEX_INFO_LEVELS,LPVOID);
#define     GetFileattributesEx WINELIB_NAME_AW(GetFileAttributesEx)
DWORD       WINAPI GetFileInformationByHandle(HFILE32,BY_HANDLE_FILE_INFORMATION*);
DWORD       WINAPI GetFileSize(HFILE32,LPDWORD);
BOOL32      WINAPI GetFileTime(HFILE32,LPFILETIME,LPFILETIME,LPFILETIME);
DWORD       WINAPI GetFileType(HFILE32);
DWORD       WINAPI GetFullPathName32A(LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD       WINAPI GetFullPathName32W(LPCWSTR,DWORD,LPWSTR,LPWSTR*);
#define     GetFullPathName WINELIB_NAME_AW(GetFullPathName)
INT32       WINAPI GetGraphicsMode(HDC32);
BOOL32      WINAPI GetHandleInformation(HANDLE32,LPDWORD);
DWORD       WINAPI GetLargestConsoleWindowSize(HANDLE32);
VOID        WINAPI GetLocalTime(LPSYSTEMTIME);
DWORD       WINAPI GetLogicalDrives(void);
DWORD       WINAPI GetLongPathName32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetLongPathName32W(LPCWSTR,LPWSTR,DWORD);
#define     GetLongPathName WINELIB_NAME_AW(GetLongPathName)
BOOL32      WINAPI GetMenuItemInfo32A(HMENU32,UINT32,BOOL32,MENUITEMINFO32A*);
BOOL32      WINAPI GetMenuItemInfo32W(HMENU32,UINT32,BOOL32,MENUITEMINFO32W*);
#define     GetMenuItemInfo WINELIB_NAME_AW(GetMenuItemInfo)
UINT32      WINAPI GetMetaFileBitsEx(HMETAFILE32,UINT32,LPVOID);
BOOL32      WINAPI GetNumberOfConsoleInputEvents(HANDLE32,LPDWORD);
BOOL32      WINAPI GetNumberOfConsoleMouseButtons(LPDWORD);
DWORD       WINAPI GetObjectType(HANDLE32);
UINT32      WINAPI GetOEMCP(void);
DWORD       WINAPI GetPriorityClass(HANDLE32);
INT32       WINAPI GetPrivateProfileSection32A(LPCSTR,LPSTR,INT32,LPCSTR);
INT32       WINAPI GetPrivateProfileSection32W(LPCWSTR,LPWSTR,INT32,LPCWSTR);
#define     GetPrivateProfileSection WINELIB_NAME_AW(GetPrivateProfileSection)
HANDLE32    WINAPI GetProcessHeap(void);
DWORD       WINAPI GetRegionData(HRGN32,DWORD,LPRGNDATA);
DWORD       WINAPI GetShortPathName32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetShortPathName32W(LPCWSTR,LPWSTR,DWORD);
#define     GetShortPathName WINELIB_NAME_AW(GetShortPathName)
HFILE32     WINAPI GetStdHandle(DWORD);
BOOL32      WINAPI GetStringTypeEx32A(LCID,DWORD,LPCSTR,INT32,LPWORD);
BOOL32      WINAPI GetStringTypeEx32W(LCID,DWORD,LPCWSTR,INT32,LPWORD);
#define     GetStringTypeEx WINELIB_NAME_AW(GetStringTypeEx)
VOID        WINAPI GetSystemInfo(LPSYSTEM_INFO);
BOOL32      WINAPI GetSystemPowerStatus(LPSYSTEM_POWER_STATUS);
VOID        WINAPI GetSystemTime(LPSYSTEMTIME);
INT32       WINAPI GetTextCharsetInfo(HDC32,LPCHARSETINFO,DWORD);
BOOL32      WINAPI GetTextExtentExPoint32A(HDC32,LPCSTR,INT32,INT32,
                                           LPINT32,LPINT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentExPoint32W(HDC32,LPCWSTR,INT32,INT32,
                                           LPINT32,LPINT32,LPSIZE32);
#define     GetTextExtentExPoint WINELIB_NAME_AW(GetTextExtentExPoint)
INT32       WINAPI GetTimeFormat32A(LCID,DWORD,LPSYSTEMTIME,LPCSTR,LPSTR,INT32);
INT32       WINAPI GetTimeFormat32W(LCID,DWORD,LPSYSTEMTIME,LPCWSTR,LPWSTR,INT32);
#define     GetTimeFormat WINELIB_NAME_AW(GetTimeFormat)
LCID        WINAPI GetThreadLocale();
INT32       WINAPI GetThreadPriority(HANDLE32);
BOOL32      WINAPI GetThreadSelectorEntry(HANDLE32,DWORD,LPLDT_ENTRY);
BOOL32      WINAPI GetUserName32A(LPSTR,LPDWORD);
BOOL32      WINAPI GetUserName32W(LPWSTR,LPDWORD);
#define     GetUserName WINELIB_NAME_AW(GetUserName)
DWORD       WINAPI GetWindowContextHelpId(HWND32);
DWORD       WINAPI GetWindowThreadProcessId(HWND32,LPDWORD);
BOOL32      WINAPI GetWorldTransform(HDC32,LPXFORM);
VOID        WINAPI GlobalMemoryStatus(LPMEMORYSTATUS);
LPVOID      WINAPI HeapAlloc(HANDLE32,DWORD,DWORD);
DWORD       WINAPI HeapCompact(HANDLE32,DWORD);
HANDLE32    WINAPI HeapCreate(DWORD,DWORD,DWORD);
BOOL32      WINAPI HeapDestroy(HANDLE32);
BOOL32      WINAPI HeapFree(HANDLE32,DWORD,LPVOID);
BOOL32      WINAPI HeapLock(HANDLE32);
LPVOID      WINAPI HeapReAlloc(HANDLE32,DWORD,LPVOID,DWORD);
DWORD       WINAPI HeapSize(HANDLE32,DWORD,LPVOID);
BOOL32      WINAPI HeapUnlock(HANDLE32);
BOOL32      WINAPI HeapValidate(HANDLE32,DWORD,LPCVOID);
LONG        WINAPI InterlockedDecrement(LPLONG);
LONG        WINAPI InterlockedExchange(LPLONG,LONG);
LONG        WINAPI InterlockedIncrement(LPLONG);
BOOL32      WINAPI IsDBCSLeadByteEx(UINT32,BYTE);
BOOL32      WINAPI IsProcessorFeaturePresent(DWORD);
BOOL32      WINAPI IsWindowUnicode(HWND32);
BOOL32      WINAPI IsValidLocale(DWORD,DWORD);
BOOL32      WINAPI LocalFileTimeToFileTime(const FILETIME*,LPFILETIME);
BOOL32      WINAPI LockFile(HFILE32,DWORD,DWORD,DWORD,DWORD);
BOOL32      WINAPI LookupPrivilegeValue32A(LPCSTR,LPCSTR,LPVOID);
BOOL32      WINAPI LookupPrivilegeValue32W(LPCWSTR,LPCWSTR,LPVOID);
#define     LookupPrivilegeValue WINELIB_NAME_AW(LookupPrivilegeValue)
SEGPTR      WINAPI MapLS(LPVOID);
LPVOID      WINAPI MapSL(SEGPTR);
LPVOID      WINAPI MapViewOfFile(HANDLE32,DWORD,DWORD,DWORD,DWORD);
LPVOID      WINAPI MapViewOfFileEx(HANDLE32,DWORD,DWORD,DWORD,DWORD,LPVOID);
INT32       WINAPI MessageBoxEx32A(HWND32,LPCSTR,LPCSTR,UINT32,WORD);
INT32       WINAPI MessageBoxEx32W(HWND32,LPCWSTR,LPCWSTR,UINT32,WORD);
#define     MessageBoxEx WINELIB_NAME_AW(MessageBoxEx)
BOOL32      WINAPI ModifyWorldTransform(HDC32,const XFORM *, DWORD);
BOOL32      WINAPI MoveFile32A(LPCSTR,LPCSTR);
BOOL32      WINAPI MoveFile32W(LPCWSTR,LPCWSTR);
#define     MoveFile WINELIB_NAME_AW(MoveFile)
BOOL32      WINAPI MoveFileEx32A(LPCSTR,LPCSTR,DWORD);
BOOL32      WINAPI MoveFileEx32W(LPCWSTR,LPCWSTR,DWORD);
#define     MoveFileEx WINELIB_NAME_AW(MoveFileEx)
DWORD       WINAPI MsgWaitForMultipleObjects(DWORD,HANDLE32*,BOOL32,DWORD,DWORD);
INT32       WINAPI MultiByteToWideChar(UINT32,DWORD,LPCSTR,INT32,LPWSTR,INT32);
INT32       WINAPI WideCharToMultiByte(UINT32,DWORD,LPCWSTR,INT32,LPSTR,INT32,LPCSTR,BOOL32*);
HANDLE32    WINAPI OpenEvent32A(DWORD,BOOL32,LPCSTR);
HANDLE32    WINAPI OpenEvent32W(DWORD,BOOL32,LPCWSTR);
#define     OpenEvent WINELIB_NAME_AW(OpenEvent)
HANDLE32    WINAPI OpenFileMapping32A(DWORD,BOOL32,LPCSTR);
HANDLE32    WINAPI OpenFileMapping32W(DWORD,BOOL32,LPCWSTR);
#define     OpenFileMapping WINELIB_NAME_AW(OpenFileMapping)
HANDLE32    WINAPI OpenMutex32A(DWORD,BOOL32,LPCSTR);
HANDLE32    WINAPI OpenMutex32W(DWORD,BOOL32,LPCWSTR);
#define     OpenMutex WINELIB_NAME_AW(OpenMutex)
HANDLE32    WINAPI OpenProcess(DWORD,BOOL32,DWORD);
BOOL32      WINAPI OpenProcessToken(HANDLE32,DWORD,HANDLE32*);
HANDLE32    WINAPI OpenSCManager32A(LPCSTR,LPCSTR,DWORD);
HANDLE32    WINAPI OpenSCManager32W(LPCWSTR,LPCWSTR,DWORD);
#define     OpenSCManager WINELIB_NAME_AW(OpenSCManager)
HANDLE32    WINAPI OpenSemaphore32A(DWORD,BOOL32,LPCSTR);
HANDLE32    WINAPI OpenSemaphore32W(DWORD,BOOL32,LPCWSTR);
#define     OpenSemaphore WINELIB_NAME_AW(OpenSemaphore)
HANDLE32    WINAPI OpenService32A(HANDLE32,LPCSTR,DWORD);
HANDLE32    WINAPI OpenService32W(HANDLE32,LPCWSTR,DWORD);
#define     OpenService WINELIB_NAME_AW(OpenService)
BOOL32      WINAPI PlayEnhMetaFile(HDC32,HENHMETAFILE32,const RECT32*);
BOOL32      WINAPI PlayEnhMetaFileRecord(HDC32,LPHANDLETABLE32,const ENHMETARECORD*,UINT32);
BOOL32      WINAPI PulseEvent(HANDLE32);
DWORD       WINAPI QueryDosDevice32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI QueryDosDevice32W(LPCWSTR,LPWSTR,DWORD);
#define     QueryDosDevice WINELIB_NAME_AW(QueryDosDevice)
BOOL32      WINAPI QueryPerformanceCounter(LPLARGE_INTEGER);
BOOL32      WINAPI ReadConsole32A(HANDLE32,LPVOID,DWORD,LPDWORD,LPVOID);
BOOL32      WINAPI ReadConsole32W(HANDLE32,LPVOID,DWORD,LPDWORD,LPVOID);
#define     ReadConsole WINELIB_NAME_AW(ReadConsole)
BOOL32      WINAPI ReadConsoleOutputCharacter32A(HANDLE32,LPSTR,DWORD,
						 COORD,LPDWORD);
#define     ReadConsoleOutputCharacter WINELIB_NAME_AW(ReadConsoleOutputCharacter)
BOOL32      WINAPI ReadFile(HANDLE32,LPVOID,DWORD,LPDWORD,LPOVERLAPPED);
LONG        WINAPI RegConnectRegistry32A(LPCSTR,HKEY,LPHKEY);
LONG        WINAPI RegConnectRegistry32W(LPCWSTR,HKEY,LPHKEY);
#define     RegConnectRegistry WINELIB_NAME_AW(RegConnectRegistry)
DWORD       WINAPI RegCreateKeyEx32A(HKEY,LPCSTR,DWORD,LPSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
DWORD       WINAPI RegCreateKeyEx32W(HKEY,LPCWSTR,DWORD,LPWSTR,DWORD,REGSAM,
                                     LPSECURITY_ATTRIBUTES,LPHKEY,LPDWORD);
#define     RegCreateKeyEx WINELIB_NAME_AW(RegCreateKeyEx)
DWORD       WINAPI RegEnumKeyEx32A(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPSTR,
                                   LPDWORD,LPFILETIME);
DWORD       WINAPI RegEnumKeyEx32W(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPWSTR,
                                   LPDWORD,LPFILETIME);
#define     RegEnumKeyEx WINELIB_NAME_AW(RegEnumKeyEx)
LONG        WINAPI RegGetKeySecurity(HKEY,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR,LPDWORD);
HANDLE32    WINAPI RegisterEventSource32A(LPCSTR,LPCSTR);
HANDLE32    WINAPI RegisterEventSource32W(LPCWSTR,LPCWSTR);
#define     RegisterEventSource WINELIB_NAME_AW(RegisterEventSource)
LONG        WINAPI RegLoadKey32A(HKEY,LPCSTR,LPCSTR);
LONG        WINAPI RegLoadKey32W(HKEY,LPCWSTR,LPCWSTR);
#define     RegLoadKey WINELIB_NAME_AW(RegLoadKey)
LONG        WINAPI RegNotifyChangeKeyValue(HKEY,BOOL32,DWORD,HANDLE32,BOOL32);
DWORD       WINAPI RegOpenKeyEx32W(HKEY,LPCWSTR,DWORD,REGSAM,LPHKEY);
DWORD       WINAPI RegOpenKeyEx32A(HKEY,LPCSTR,DWORD,REGSAM,LPHKEY);
#define     RegOpenKeyEx WINELIB_NAME_AW(RegOpenKeyEx)
DWORD       WINAPI RegQueryInfoKey32W(HKEY,LPWSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
DWORD       WINAPI RegQueryInfoKey32A(HKEY,LPSTR,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPDWORD,LPDWORD,LPDWORD,LPDWORD,
                                      LPDWORD,LPFILETIME);
#define     RegQueryInfoKey WINELIB_NAME_AW(RegQueryInfoKey)
LONG        WINAPI RegReplaceKey32A(HKEY,LPCSTR,LPCSTR,LPCSTR);
LONG        WINAPI RegReplaceKey32W(HKEY,LPCWSTR,LPCWSTR,LPCWSTR);
#define     RegReplaceKey WINELIB_NAME_AW(RegReplaceKey)
LONG        WINAPI RegRestoreKey32A(HKEY,LPCSTR,DWORD);
LONG        WINAPI RegRestoreKey32W(HKEY,LPCWSTR,DWORD);
#define     RegRestoreKey WINELIB_NAME_AW(RegRestoreKey)
LONG        WINAPI RegSaveKey32A(HKEY,LPCSTR,LPSECURITY_ATTRIBUTES);
LONG        WINAPI RegSaveKey32W(HKEY,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     RegSaveKey WINELIB_NAME_AW(RegSaveKey)
LONG        WINAPI RegSetKeySecurity(HKEY,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR);
LONG        WINAPI RegUnLoadKey32A(HKEY,LPCSTR);
LONG        WINAPI RegUnLoadKey32W(HKEY,LPCWSTR);
#define     RegUnLoadKey WINELIB_NAME_AW(RegUnLoadKey)
BOOL32      WINAPI ReleaseSemaphore(HANDLE32,LONG,LPLONG);
BOOL32      WINAPI ResetEvent(HANDLE32);
VOID        WINAPI RtlFillMemory(LPVOID,UINT32,UINT32);
VOID        WINAPI RtlMoveMemory(LPVOID,LPCVOID,UINT32);
VOID        WINAPI RtlZeroMemory(LPVOID,UINT32);
DWORD       WINAPI SearchPath32A(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD       WINAPI SearchPath32W(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,LPWSTR,LPWSTR*);
#define     SearchPath WINELIB_NAME(SearchPath)
BOOL32      WINAPI SetBrushOrgEx(HDC32,INT32,INT32,LPPOINT32);
BOOL32      WINAPI SetCommMask(INT32,DWORD);
BOOL32      WINAPI SetCommTimeouts(INT32,LPCOMMTIMEOUTS);
BOOL32      WINAPI SetComputerName32A(LPCSTR);
BOOL32      WINAPI SetComputerName32W(LPCWSTR);
#define     SetComputerName WINELIB_NAME_AW(SetComputerName)
BOOL32      WINAPI SetConsoleCursorPosition(HANDLE32,COORD);
BOOL32      WINAPI SetConsoleMode(HANDLE32,DWORD);
BOOL32      WINAPI SetConsoleTitle32A(LPCSTR);
BOOL32      WINAPI SetConsoleTitle32W(LPCWSTR);
#define     SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
VOID        WINAPI SetDebugErrorLevel(DWORD);
BOOL32      WINAPI SetEndOfFile(HFILE32);
HENHMETAFILE32 WINAPI SetEnhMetaFileBits(UINT32,const BYTE *);
BOOL32      WINAPI SetEnvironmentVariable32A(LPCSTR,LPCSTR);
BOOL32      WINAPI SetEnvironmentVariable32W(LPCWSTR,LPCWSTR);
#define     SetEnvironmentVariable WINELIB_NAME_AW(SetEnvironmentVariable)
BOOL32      WINAPI SetEvent(HANDLE32);
VOID        WINAPI SetFileApisToANSI(void);
VOID        WINAPI SetFileApisToOEM(void);
DWORD       WINAPI SetFilePointer(HFILE32,LONG,LPLONG,DWORD);
BOOL32      WINAPI SetFileTime(HFILE32,const FILETIME*,const FILETIME*,
                               const FILETIME*);
INT32       WINAPI SetGraphicsMode(HDC32,INT32);
BOOL32      WINAPI SetHandleInformation(HANDLE32,DWORD,DWORD);
VOID        WINAPI SetLastErrorEx(DWORD,DWORD);
BOOL32      WINAPI SetMenuItemInfo32A(HMENU32,UINT32,BOOL32,const MENUITEMINFO32A*);
BOOL32      WINAPI SetMenuItemInfo32W(HMENU32,UINT32,BOOL32,const MENUITEMINFO32W*);
#define     SetMenuItemInfo WINELIB_NAME_AW(SetMenuItemInfo)
HMETAFILE32 WINAPI SetMetaFileBitsEx(UINT32,const BYTE*);
BOOL32      WINAPI SetPriorityClass(HANDLE32,DWORD);
BOOL32      WINAPI SetStdHandle(DWORD,HANDLE32);
BOOL32      WINAPI SetSystemPowerState(BOOL32,BOOL32);
BOOL32      WINAPI SetSystemTime(const SYSTEMTIME*);
BOOL32      WINAPI SetThreadPriority(HANDLE32,INT32);
BOOL32      WINAPI SetTimeZoneInformation(const LPTIME_ZONE_INFORMATION);
BOOL32      WINAPI SetWindowContextHelpId(HWND32,DWORD);
BOOL32      WINAPI SetWorldTransform(HDC32,const XFORM*);
VOID        WINAPI Sleep(DWORD);
BOOL32      WINAPI StartService32A(HANDLE32,DWORD,LPCSTR*);
BOOL32      WINAPI StartService32W(HANDLE32,DWORD,LPCWSTR*);
#define     StartService WINELIB_NAME_AW(StartService)
BOOL32      WINAPI SystemTimeToFileTime(const SYSTEMTIME*,LPFILETIME);
BOOL32      WINAPI TrackPopupMenuEx(HMENU32,UINT32,INT32,INT32,HWND32,
                                    LPTPMPARAMS);
DWORD       WINAPI TlsAlloc(void);
BOOL32      WINAPI TlsFree(DWORD);
LPVOID      WINAPI TlsGetValue(DWORD);
BOOL32      WINAPI TlsSetValue(DWORD,LPVOID);
VOID        WINAPI UnMapLS(SEGPTR);
BOOL32      WINAPI UnlockFile(HFILE32,DWORD,DWORD,DWORD,DWORD);
BOOL32      WINAPI UnmapViewOfFile(LPVOID);
LPVOID      WINAPI VirtualAlloc(LPVOID,DWORD,DWORD,DWORD);
BOOL32      WINAPI VirtualFree(LPVOID,DWORD,DWORD);
BOOL32      WINAPI VirtualLock(LPVOID,DWORD);
BOOL32      WINAPI VirtualProtect(LPVOID,DWORD,DWORD,LPDWORD);
BOOL32      WINAPI VirtualProtectEx(HANDLE32,LPVOID,DWORD,DWORD,LPDWORD);
DWORD       WINAPI VirtualQuery(LPCVOID,LPMEMORY_BASIC_INFORMATION,DWORD);
DWORD       WINAPI VirtualQueryEx(HANDLE32,LPCVOID,LPMEMORY_BASIC_INFORMATION,DWORD);
BOOL32      WINAPI VirtualUnlock(LPVOID,DWORD);
BOOL32      WINAPI WaitForDebugEvent(LPDEBUG_EVENT,DWORD);
DWORD       WINAPI WaitForInputIdle(HANDLE32,DWORD);
DWORD       WINAPI WaitForMultipleObjects(DWORD,const HANDLE32*,BOOL32,DWORD);
DWORD       WINAPI WaitForMultipleObjectsEx(DWORD,const HANDLE32*,BOOL32,DWORD,BOOL32);
DWORD       WINAPI WaitForSingleObject(HANDLE32,DWORD);
DWORD       WINAPI WaitForSingleObjectEx(HANDLE32,DWORD,BOOL32);
UINT32      WINAPI WNetAddConnection2_32A(LPNETRESOURCE32A,LPCSTR,LPCSTR,DWORD);
UINT32      WINAPI WNetAddConnection2_32W(LPNETRESOURCE32W,LPCWSTR,LPCWSTR,DWORD);
#define     WNetAddConnection2 WINELIB_NAME_AW(WNetAddConnection2_)
UINT32      WINAPI WNetAddConnection3_32A(HWND32,LPNETRESOURCE32A,LPCSTR,LPCSTR,DWORD);
UINT32      WINAPI WNetAddConnection3_32W(HWND32,LPNETRESOURCE32W,LPCWSTR,LPCWSTR,DWORD);
#define     WNetAddConnection3 WINELIB_NAME_AW(WNetAddConnection3_)
SEGPTR      WINAPI WOWGlobalAllocLock16(DWORD,DWORD,HGLOBAL16*);
DWORD       WINAPI WOWCallback16(FARPROC16,DWORD);
BOOL32      WINAPI WOWCallback16Ex(FARPROC16,DWORD,DWORD,LPVOID,LPDWORD);
HANDLE32    WINAPI WOWHandle32(WORD,WOW_HANDLE_TYPE);
WORD        WINAPI WOWHandle16(HANDLE32,WOW_HANDLE_TYPE);
BOOL32      WINAPI WriteConsole32A(HANDLE32,LPCVOID,DWORD,LPDWORD,LPVOID);
BOOL32      WINAPI WriteConsole32W(HANDLE32,LPCVOID,DWORD,LPDWORD,LPVOID);
#define     WriteConsole WINELIB_NAME_AW(WriteConsole)
BOOL32      WINAPI WriteFile(HANDLE32,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);
VOID        WINAPI ZeroMemory(LPVOID,UINT32);
#define     ZeroMemory RtlZeroMemory

/* Declarations for functions that are the same in Win16 and Win32 */

VOID        WINAPI CloseSound(VOID);
VOID        WINAPI EndMenu(void);
DWORD       WINAPI GetDialogBaseUnits(void);
VOID        WINAPI GetKeyboardState(LPBYTE);
DWORD       WINAPI GetLastError(void);
DWORD       WINAPI GetMenuCheckMarkDimensions(void);
LONG        WINAPI GetMessageExtraInfo(void);
DWORD       WINAPI GetMessagePos(void);
LONG        WINAPI GetMessageTime(void);
LANGID      WINAPI GetSystemDefaultLangID(void);
LCID        WINAPI GetSystemDefaultLCID(void);
DWORD       WINAPI GetTickCount(void);
LANGID      WINAPI GetUserDefaultLangID(void);
LCID        WINAPI GetUserDefaultLCID(void);
ATOM        WINAPI GlobalDeleteAtom(ATOM);
VOID        WINAPI LZDone(void);
DWORD       WINAPI OemKeyScan(WORD);
DWORD       WINAPI RegCloseKey(HKEY);
DWORD       WINAPI RegFlushKey(HKEY);
VOID        WINAPI ReleaseCapture(void);
VOID        WINAPI SetKeyboardState(LPBYTE);
VOID        WINAPI SetLastError(DWORD);
VOID        WINAPI WaitMessage(VOID);


/* Declarations for functions that change between Win16 and Win32 */

BOOL16      WINAPI AbortPath16(HDC16);
BOOL32      WINAPI AbortPath32(HDC32);
#define     AbortPath WINELIB_NAME(AbortPath)
LRESULT     WINAPI AboutDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI AboutDlgProc32(HWND32,UINT32,WPARAM32,LPARAM);
#define     AboutDlgProc WINELIB_NAME(AboutDlgProc)
INT16       WINAPI AccessResource16(HINSTANCE16,HRSRC16);
INT32       WINAPI AccessResource32(HMODULE32,HRSRC32);
#define     AccessResource WINELIB_NAME(AccessResource)
ATOM        WINAPI AddAtom16(SEGPTR);
ATOM        WINAPI AddAtom32A(LPCSTR);
ATOM        WINAPI AddAtom32W(LPCWSTR);
#define     AddAtom WINELIB_NAME_AW(AddAtom)
INT16       WINAPI AddFontResource16(LPCSTR);
INT32       WINAPI AddFontResource32A(LPCSTR);
INT32       WINAPI AddFontResource32W(LPCWSTR);
#define     AddFontResource WINELIB_NAME_AW(AddFontResource)
BOOL16      WINAPI AdjustWindowRect16(LPRECT16,DWORD,BOOL16);
BOOL32      WINAPI AdjustWindowRect32(LPRECT32,DWORD,BOOL32);
#define     AdjustWindowRect WINELIB_NAME(AdjustWindowRect)
BOOL16      WINAPI AdjustWindowRectEx16(LPRECT16,DWORD,BOOL16,DWORD);
BOOL32      WINAPI AdjustWindowRectEx32(LPRECT32,DWORD,BOOL32,DWORD);
#define     AdjustWindowRectEx WINELIB_NAME(AdjustWindowRectEx)
void        WINAPI AnimatePalette16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
BOOL32      WINAPI AnimatePalette32(HPALETTE32,UINT32,UINT32,LPPALETTEENTRY);
#define     AnimatePalette WINELIB_NAME(AnimatePalette)
SEGPTR      WINAPI AnsiLower16(SEGPTR);
#define     AnsiLower32A CharLower32A
#define     AnsiLower32W CharLower32W
#define     AnsiLower WINELIB_NAME_AW(AnsiLower)
UINT16      WINAPI AnsiLowerBuff16(LPSTR,UINT16);
#define     AnsiLowerBuff32A CharLowerBuff32A
#define     AnsiLowerBuff32W CharLowerBuff32W
#define     AnsiLowerBuff WINELIB_NAME_AW(AnsiLowerBuff)
SEGPTR      WINAPI AnsiNext16(SEGPTR);
#define     AnsiNext32A CharNext32A
#define     AnsiNext32W CharNext32W
#define     AnsiNext WINELIB_NAME_AW(AnsiNext)
SEGPTR      WINAPI AnsiPrev16(SEGPTR,SEGPTR);
#define     AnsiPrev32A CharPrev32A
#define     AnsiPrev32W CharPrev32W
#define     AnsiPrev WINELIB_NAME_AW(AnsiPrev)
INT16       WINAPI AnsiToOem16(LPCSTR,LPSTR);
#define     AnsiToOem32A CharToOem32A
#define     AnsiToOem32W CharToOem32W
#define     AnsiToOem WINELIB_NAME_AW(AnsiToOem)
VOID        WINAPI AnsiToOemBuff16(LPCSTR,LPSTR,UINT16);
#define     AnsiToOemBuff32A CharToOemBuff32A
#define     AnsiToOemBuff32W CharToOemBuff32W
#define     AnsiToOemBuff WINELIB_NAME_AW(AnsiToOemBuff)
SEGPTR      WINAPI AnsiUpper16(SEGPTR);
#define     AnsiUpper32A CharUpper32A
#define     AnsiUpper32W CharUpper32W
#define     AnsiUpper WINELIB_NAME_AW(AnsiUpper)
UINT16      WINAPI AnsiUpperBuff16(LPSTR,UINT16);
#define     AnsiUpperBuff32A CharUpperBuff32A
#define     AnsiUpperBuff32W CharUpperBuff32W
#define     AnsiUpperBuff WINELIB_NAME_AW(AnsiUpperBuff)
BOOL16      WINAPI AnyPopup16(void);
BOOL32      WINAPI AnyPopup32(void);
#define     AnyPopup WINELIB_NAME(AnyPopup)
BOOL16      WINAPI AppendMenu16(HMENU16,UINT16,UINT16,SEGPTR);
BOOL32      WINAPI AppendMenu32A(HMENU32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI AppendMenu32W(HMENU32,UINT32,UINT32,LPCWSTR);
#define     AppendMenu WINELIB_NAME_AW(AppendMenu)
BOOL16      WINAPI Arc16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Arc32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     Arc WINELIB_NAME(Arc)
UINT16      WINAPI ArrangeIconicWindows16(HWND16);
UINT32      WINAPI ArrangeIconicWindows32(HWND32);
#define     ArrangeIconicWindows WINELIB_NAME(ArrangeIconicWindows)
HDWP16      WINAPI BeginDeferWindowPos16(INT16);
HDWP32      WINAPI BeginDeferWindowPos32(INT32);
#define     BeginDeferWindowPos WINELIB_NAME(BeginDeferWindowPos)
HDC16       WINAPI BeginPaint16(HWND16,LPPAINTSTRUCT16);
HDC32       WINAPI BeginPaint32(HWND32,LPPAINTSTRUCT32);
#define     BeginPaint WINELIB_NAME(BeginPaint)
BOOL16      WINAPI BeginPath16(HDC16);
BOOL32      WINAPI BeginPath32(HDC32);
#define     BeginPath WINELIB_NAME(BeginPath)
BOOL16      WINAPI BitBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,INT16,DWORD);
BOOL32      WINAPI BitBlt32(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,INT32,DWORD);
#define     BitBlt WINELIB_NAME(BitBlt)
BOOL16      WINAPI BringWindowToTop16(HWND16);
BOOL32      WINAPI BringWindowToTop32(HWND32);
#define     BringWindowToTop WINELIB_NAME(BringWindowToTop)
BOOL16      WINAPI BuildCommDCB16(LPCSTR,LPDCB16);
BOOL32      WINAPI BuildCommDCB32A(LPCSTR,LPDCB32);
BOOL32      WINAPI BuildCommDCB32W(LPCWSTR,LPDCB32);
#define     BuildCommDCB WINELIB_NAME_AW(BuildCommDCB)
BOOL32      WINAPI BuildCommDCBAndTimeouts32A(LPCSTR,LPDCB32,LPCOMMTIMEOUTS);
BOOL32      WINAPI BuildCommDCBAndTimeouts32W(LPCWSTR,LPDCB32,LPCOMMTIMEOUTS);
#define     BuildCommDCBAndTimeouts WINELIB_NAME_AW(BuildCommDCBAndTimeouts)
BOOL16      WINAPI CallMsgFilter16(SEGPTR,INT16);
BOOL32      WINAPI CallMsgFilter32A(LPMSG32,INT32);
BOOL32      WINAPI CallMsgFilter32W(LPMSG32,INT32);
#define     CallMsgFilter WINELIB_NAME_AW(CallMsgFilter)
LRESULT     WINAPI CallNextHookEx16(HHOOK,INT16,WPARAM16,LPARAM);
LRESULT     WINAPI CallNextHookEx32(HHOOK,INT32,WPARAM32,LPARAM);
#define     CallNextHookEx WINELIB_NAME(CallNextHookEx)
LRESULT     WINAPI CallWindowProc16(WNDPROC16,HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI CallWindowProc32A(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI CallWindowProc32W(WNDPROC32,HWND32,UINT32,WPARAM32,LPARAM);
#define     CallWindowProc WINELIB_NAME_AW(CallWindowProc)
BOOL16      WINAPI ChangeClipboardChain16(HWND16,HWND16);
BOOL32      WINAPI ChangeClipboardChain32(HWND32,HWND32);
#define     ChangeClipboardChain WINELIB_NAME(ChangeClipboardChain)
BOOL16      WINAPI ChangeMenu16(HMENU16,UINT16,SEGPTR,UINT16,UINT16);
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
BOOL16      WINAPI CheckDlgButton16(HWND16,INT16,UINT16);
BOOL32      WINAPI CheckDlgButton32(HWND32,INT32,UINT32);
#define     CheckDlgButton WINELIB_NAME(CheckDlgButton)
BOOL16      WINAPI CheckMenuItem16(HMENU16,UINT16,UINT16);
DWORD       WINAPI CheckMenuItem32(HMENU32,UINT32,UINT32);
#define     CheckMenuItem WINELIB_NAME(CheckMenuItem)
BOOL16      WINAPI CheckMenuRadioButton16(HMENU16,UINT16,UINT16,UINT16,BOOL16);
BOOL32      WINAPI CheckMenuRadioButton32(HMENU32,UINT32,UINT32,UINT32,BOOL32);
#define     CheckMenuRadioButton WINELIB_NAME(CheckMenuRadioButton)
BOOL16      WINAPI CheckRadioButton16(HWND16,UINT16,UINT16,UINT16);
BOOL32      WINAPI CheckRadioButton32(HWND32,UINT32,UINT32,UINT32);
#define     CheckRadioButton WINELIB_NAME(CheckRadioButton)
HWND16      WINAPI ChildWindowFromPoint16(HWND16,POINT16);
HWND32      WINAPI ChildWindowFromPoint32(HWND32,POINT32);
#define     ChildWindowFromPoint WINELIB_NAME(ChildWindowFromPoint)
INT32       ChoosePixelFormat(HDC32,PIXELFORMATDESCRIPTOR*);
BOOL16      WINAPI Chord16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Chord32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     Chord WINELIB_NAME(Chord)
INT16       WINAPI ClearCommBreak16(INT16);
BOOL32      WINAPI ClearCommBreak32(INT32);
#define     ClearCommBreak WINELIB_NAME(ClearCommBreak)
VOID        WINAPI ClientToScreen16(HWND16,LPPOINT16);
BOOL32      WINAPI ClientToScreen32(HWND32,LPPOINT32);
#define     ClientToScreen WINELIB_NAME(ClientToScreen)
BOOL16      WINAPI ClipCursor16(const RECT16*);
BOOL32      WINAPI ClipCursor32(const RECT32*);
#define     ClipCursor WINELIB_NAME(ClipCursor)
BOOL16      WINAPI CloseClipboard16(void);
BOOL32      WINAPI CloseClipboard32(void);
#define     CloseClipboard WINELIB_NAME(CloseClipboard)
BOOL16      WINAPI CloseFigure16(HDC16);
BOOL32      WINAPI CloseFigure32(HDC32);
#define     CloseFigure WINELIB_NAME(CloseFigure)
HMETAFILE16 WINAPI CloseMetaFile16(HDC16);
HMETAFILE32 WINAPI CloseMetaFile32(HDC32);
#define     CloseMetaFile WINELIB_NAME(CloseMetaFile)
BOOL16      WINAPI CloseWindow16(HWND16);
BOOL32      WINAPI CloseWindow32(HWND32);
#define     CloseWindow WINELIB_NAME(CloseWindow)
INT16       WINAPI CombineRgn16(HRGN16,HRGN16,HRGN16,INT16);
INT32       WINAPI CombineRgn32(HRGN32,HRGN32,HRGN32,INT32);
#define     CombineRgn WINELIB_NAME(CombineRgn)
UINT16      WINAPI CompareString16(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT32      WINAPI CompareString32A(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT32      WINAPI CompareString32W(DWORD,DWORD,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     CompareString WINELIB_NAME_AW(CompareString)
HCURSOR16   WINAPI CopyCursor16(HINSTANCE16,HCURSOR16);
#define     CopyCursor32(cur) ((HCURSOR32)CopyIcon32((HICON32)(cur)))
#define     CopyCursor WINELIB_NAME(CopyCursor)
HICON16     WINAPI CopyIcon16(HINSTANCE16,HICON16);
HICON32     WINAPI CopyIcon32(HICON32);
#define     CopyIcon WINELIB_NAME(CopyIcon)
LONG        WINAPI CopyLZFile16(HFILE16,HFILE16);
LONG        WINAPI CopyLZFile32(HFILE32,HFILE32);
#define     CopyLZFile WINELIB_NAME(CopyLZFile)
HMETAFILE16 WINAPI CopyMetaFile16(HMETAFILE16,LPCSTR);
HMETAFILE32 WINAPI CopyMetaFile32A(HMETAFILE32,LPCSTR);
HMETAFILE32 WINAPI CopyMetaFile32W(HMETAFILE32,LPCWSTR);
#define     CopyMetaFile WINELIB_NAME_AW(CopyMetaFile)
BOOL16      WINAPI CopyRect16(RECT16*,const RECT16*);
BOOL32      WINAPI CopyRect32(RECT32*,const RECT32*);
#define     CopyRect WINELIB_NAME(CopyRect)
INT16       WINAPI CountClipboardFormats16(void);
INT32       WINAPI CountClipboardFormats32(void);
#define     CountClipboardFormats WINELIB_NAME(CountClipboardFormats)
INT16       WINAPI CountVoiceNotes16(INT16);
DWORD       WINAPI CountVoiceNotes32(DWORD);
#define     CountVoiceNotes WINELIB_NAME(CountVoiceNotes)
HBITMAP16   WINAPI CreateBitmap16(INT16,INT16,UINT16,UINT16,LPCVOID);
HBITMAP32   WINAPI CreateBitmap32(INT32,INT32,UINT32,UINT32,LPCVOID);
#define     CreateBitmap WINELIB_NAME(CreateBitmap)
HBITMAP16   WINAPI CreateBitmapIndirect16(const BITMAP16*);
HBITMAP32   WINAPI CreateBitmapIndirect32(const BITMAP32*);
#define     CreateBitmapIndirect WINELIB_NAME(CreateBitmapIndirect)
HBRUSH16    WINAPI CreateBrushIndirect16(const LOGBRUSH16*);
HBRUSH32    WINAPI CreateBrushIndirect32(const LOGBRUSH32*);
#define     CreateBrushIndirect WINELIB_NAME(CreateBrushIndirect)
VOID        WINAPI CreateCaret16(HWND16,HBITMAP16,INT16,INT16);
BOOL32      WINAPI CreateCaret32(HWND32,HBITMAP32,INT32,INT32);
#define     CreateCaret WINELIB_NAME(CreateCaret)
HBITMAP16   WINAPI CreateCompatibleBitmap16(HDC16,INT16,INT16);
HBITMAP32   WINAPI CreateCompatibleBitmap32(HDC32,INT32,INT32);
#define     CreateCompatibleBitmap WINELIB_NAME(CreateCompatibleBitmap)
HDC16       WINAPI CreateCompatibleDC16(HDC16);
HDC32       WINAPI CreateCompatibleDC32(HDC32);
#define     CreateCompatibleDC WINELIB_NAME(CreateCompatibleDC)
HCURSOR16   WINAPI CreateCursor16(HINSTANCE16,INT16,INT16,INT16,INT16,LPCVOID,LPCVOID);
HCURSOR32   WINAPI CreateCursor32(HINSTANCE32,INT32,INT32,INT32,INT32,LPCVOID,LPCVOID);
#define     CreateCursor WINELIB_NAME(CreateCursor)
HDC16       WINAPI CreateDC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC32       WINAPI CreateDC32A(LPCSTR,LPCSTR,LPCSTR,const DEVMODE32A*);
HDC32       WINAPI CreateDC32W(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODE32W*);
#define     CreateDC WINELIB_NAME_AW(CreateDC)
HWND16      WINAPI CreateDialog16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16);
#define     CreateDialog32A(inst,ptr,hwnd,dlg) \
           CreateDialogParam32A(inst,ptr,hwnd,dlg,0)
#define     CreateDialog32W(inst,ptr,hwnd,dlg) \
           CreateDialogParam32W(inst,ptr,hwnd,dlg,0)
#define     CreateDialog WINELIB_NAME_AW(CreateDialog)
HWND16      WINAPI CreateDialogIndirect16(HINSTANCE16,LPCVOID,HWND16,DLGPROC16);
#define     CreateDialogIndirect32A(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32A(inst,ptr,hwnd,dlg,0)
#define     CreateDialogIndirect32W(inst,ptr,hwnd,dlg) \
           CreateDialogIndirectParam32W(inst,ptr,hwnd,dlg,0)
#define     CreateDialogIndirect WINELIB_NAME_AW(CreateDialogIndirect)
HWND16      WINAPI CreateDialogIndirectParam16(HINSTANCE16,LPCVOID,HWND16,
                                               DLGPROC16,LPARAM);
HWND32      WINAPI CreateDialogIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,
                                                DLGPROC32,LPARAM);
HWND32      WINAPI CreateDialogIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,
                                                DLGPROC32,LPARAM);
#define     CreateDialogIndirectParam WINELIB_NAME_AW(CreateDialogIndirectParam)
HWND16      WINAPI CreateDialogParam16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16,LPARAM);
HWND32      WINAPI CreateDialogParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
HWND32      WINAPI CreateDialogParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define     CreateDialogParam WINELIB_NAME_AW(CreateDialogParam)
HBITMAP16   WINAPI CreateDIBitmap16(HDC16,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT16);
HBITMAP32   WINAPI CreateDIBitmap32(HDC32,const BITMAPINFOHEADER*,DWORD,
                                    LPCVOID,const BITMAPINFO*,UINT32);
#define     CreateDIBitmap WINELIB_NAME(CreateDIBitmap)
HBRUSH16    WINAPI CreateDIBPatternBrush16(HGLOBAL16,UINT16);
HBRUSH32    WINAPI CreateDIBPatternBrush32(HGLOBAL32,UINT32);
#define     CreateDIBPatternBrush WINELIB_NAME(CreateDIBPatternBrush)
HBITMAP16   WINAPI CreateDIBSection16 (HDC16, BITMAPINFO *, UINT16,
				       LPVOID **, HANDLE16, DWORD offset);
HBITMAP32   WINAPI CreateDIBSection32 (HDC32, BITMAPINFO *, UINT32,
				       LPVOID **, HANDLE32, DWORD offset);
#define     CreateDIBSection WINELIB_NAME(CreateDIBSection)
BOOL16      WINAPI CreateDirectory16(LPCSTR,LPVOID);
BOOL32      WINAPI CreateDirectory32A(LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL32      WINAPI CreateDirectory32W(LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     CreateDirectory WINELIB_NAME_AW(CreateDirectory)
BOOL32      WINAPI CreateDirectoryEx32A(LPCSTR,LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL32      WINAPI CreateDirectoryEx32W(LPCWSTR,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     CreateDirectoryEx WINELIB_NAME_AW(CreateDirectoryEx)
HBITMAP16   WINAPI CreateDiscardableBitmap16(HDC16,INT16,INT16);
HBITMAP32   WINAPI CreateDiscardableBitmap32(HDC32,INT32,INT32);
#define     CreateDiscardableBitmap WINELIB_NAME(CreateDiscardableBitmap)
HRGN16      WINAPI CreateEllipticRgn16(INT16,INT16,INT16,INT16);
HRGN32      WINAPI CreateEllipticRgn32(INT32,INT32,INT32,INT32);
#define     CreateEllipticRgn WINELIB_NAME(CreateEllipticRgn)
HRGN16      WINAPI CreateEllipticRgnIndirect16(const RECT16 *);
HRGN32      WINAPI CreateEllipticRgnIndirect32(const RECT32 *);
#define     CreateEllipticRgnIndirect WINELIB_NAME(CreateEllipticRgnIndirect)
HFONT16     WINAPI CreateFont16(INT16,INT16,INT16,INT16,INT16,BYTE,BYTE,BYTE,
                                BYTE,BYTE,BYTE,BYTE,BYTE,LPCSTR);
HFONT32     WINAPI CreateFont32A(INT32,INT32,INT32,INT32,INT32,DWORD,DWORD,
                                 DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCSTR);
HFONT32     WINAPI CreateFont32W(INT32,INT32,INT32,INT32,INT32,DWORD,DWORD,
                                 DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,LPCWSTR);
#define     CreateFont WINELIB_NAME_AW(CreateFont)
HFONT16     WINAPI CreateFontIndirect16(const LOGFONT16*);
HFONT32     WINAPI CreateFontIndirect32A(const LOGFONT32A*);
HFONT32     WINAPI CreateFontIndirect32W(const LOGFONT32W*);
#define     CreateFontIndirect WINELIB_NAME_AW(CreateFontIndirect)
HBRUSH16    WINAPI CreateHatchBrush16(INT16,COLORREF);
HBRUSH32    WINAPI CreateHatchBrush32(INT32,COLORREF);
#define     CreateHatchBrush WINELIB_NAME(CreateHatchBrush)
HDC16       WINAPI CreateIC16(LPCSTR,LPCSTR,LPCSTR,const DEVMODE16*);
HDC32       WINAPI CreateIC32A(LPCSTR,LPCSTR,LPCSTR,const DEVMODE32A*);
HDC32       WINAPI CreateIC32W(LPCWSTR,LPCWSTR,LPCWSTR,const DEVMODE32W*);
#define     CreateIC WINELIB_NAME_AW(CreateIC)
HICON16     WINAPI CreateIcon16(HINSTANCE16,INT16,INT16,BYTE,BYTE,LPCVOID,LPCVOID);
HICON32     WINAPI CreateIcon32(HINSTANCE32,INT32,INT32,BYTE,BYTE,LPCVOID,LPCVOID);
#define     CreateIcon WINELIB_NAME(CreateIcon)
HICON16     WINAPI CreateIconFromResourceEx16(LPBYTE,UINT16,BOOL16,DWORD,INT16,INT16,UINT16);
HICON32     WINAPI CreateIconFromResourceEx32(LPBYTE,UINT32,BOOL32,DWORD,INT32,INT32,UINT32);
#define     CreateIconFromResourceEx WINELIB_NAME(CreateIconFromResourceEx)
HMENU16     WINAPI CreateMenu16(void);
HMENU32     WINAPI CreateMenu32(void);
#define     CreateMenu WINELIB_NAME(CreateMenu)
HDC16       WINAPI CreateMetaFile16(LPCSTR);
HDC32       WINAPI CreateMetaFile32A(LPCSTR);
HDC32       WINAPI CreateMetaFile32W(LPCWSTR);
#define     CreateMetaFile WINELIB_NAME_AW(CreateMetaFile)
HPALETTE16  WINAPI CreatePalette16(const LOGPALETTE*);
HPALETTE32  WINAPI CreatePalette32(const LOGPALETTE*);
#define     CreatePalette WINELIB_NAME(CreatePalette)
HBRUSH16    WINAPI CreatePatternBrush16(HBITMAP16);
HBRUSH32    WINAPI CreatePatternBrush32(HBITMAP32);
#define     CreatePatternBrush WINELIB_NAME(CreatePatternBrush)
HPEN16      WINAPI CreatePen16(INT16,INT16,COLORREF);
HPEN32      WINAPI CreatePen32(INT32,INT32,COLORREF);
#define     CreatePen WINELIB_NAME(CreatePen)
HPEN16      WINAPI CreatePenIndirect16(const LOGPEN16*);
HPEN32      WINAPI CreatePenIndirect32(const LOGPEN32*);
#define     CreatePenIndirect WINELIB_NAME(CreatePenIndirect)
HRGN16      WINAPI CreatePolyPolygonRgn16(const POINT16*,const INT16*,INT16,INT16);
HRGN32      WINAPI CreatePolyPolygonRgn32(const POINT32*,const INT32*,INT32,INT32);
#define     CreatePolyPolygonRgn WINELIB_NAME(CreatePolyPolygonRgn)
HRGN16      WINAPI CreatePolygonRgn16(const POINT16*,INT16,INT16);
HRGN32      WINAPI CreatePolygonRgn32(const POINT32*,INT32,INT32);
#define     CreatePolygonRgn WINELIB_NAME(CreatePolygonRgn)
HMENU16     WINAPI CreatePopupMenu16(void);
HMENU32     WINAPI CreatePopupMenu32(void);
#define     CreatePopupMenu WINELIB_NAME(CreatePopupMenu)
HRGN16      WINAPI CreateRectRgn16(INT16,INT16,INT16,INT16);
HRGN32      WINAPI CreateRectRgn32(INT32,INT32,INT32,INT32);
#define     CreateRectRgn WINELIB_NAME(CreateRectRgn)
HRGN16      WINAPI CreateRectRgnIndirect16(const RECT16*);
HRGN32      WINAPI CreateRectRgnIndirect32(const RECT32*);
#define     CreateRectRgnIndirect WINELIB_NAME(CreateRectRgnIndirect)
HRGN16      WINAPI CreateRoundRectRgn16(INT16,INT16,INT16,INT16,INT16,INT16);
HRGN32      WINAPI CreateRoundRectRgn32(INT32,INT32,INT32,INT32,INT32,INT32);
#define     CreateRoundRectRgn WINELIB_NAME(CreateRoundRectRgn)
BOOL16      WINAPI CreateScalableFontResource16(UINT16,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI CreateScalableFontResource32A(DWORD,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI CreateScalableFontResource32W(DWORD,LPCWSTR,LPCWSTR,LPCWSTR);
#define     CreateScalableFontResource WINELIB_NAME_AW(CreateScalableFontResource)
HBRUSH16    WINAPI CreateSolidBrush16(COLORREF);
HBRUSH32    WINAPI CreateSolidBrush32(COLORREF);
#define     CreateSolidBrush WINELIB_NAME(CreateSolidBrush)
HWND16      WINAPI CreateWindow16(LPCSTR,LPCSTR,DWORD,INT16,INT16,INT16,INT16,
                                  HWND16,HMENU16,HINSTANCE16,LPVOID);
#define     CreateWindow32A(className,titleName,style,x,y,width,height,\
                            parent,menu,instance,param) \
            CreateWindowEx32A(0,className,titleName,style,x,y,width,height,\
                              parent,menu,instance,param)
#define     CreateWindow32W(className,titleName,style,x,y,width,height,\
                            parent,menu,instance,param) \
            CreateWindowEx32W(0,className,titleName,style,x,y,width,height,\
                              parent,menu,instance,param)
#define     CreateWindow WINELIB_NAME_AW(CreateWindow)
HWND16      WINAPI CreateWindowEx16(DWORD,LPCSTR,LPCSTR,DWORD,INT16,INT16,
                                INT16,INT16,HWND16,HMENU16,HINSTANCE16,LPVOID);
HWND32      WINAPI CreateWindowEx32A(DWORD,LPCSTR,LPCSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
HWND32      WINAPI CreateWindowEx32W(DWORD,LPCWSTR,LPCWSTR,DWORD,INT32,INT32,
                                INT32,INT32,HWND32,HMENU32,HINSTANCE32,LPVOID);
#define     CreateWindowEx WINELIB_NAME_AW(CreateWindowEx)
LRESULT     WINAPI DefDlgProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI DefDlgProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefDlgProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefDlgProc WINELIB_NAME_AW(DefDlgProc)
HDWP16      WINAPI DeferWindowPos16(HDWP16,HWND16,HWND16,INT16,INT16,INT16,INT16,UINT16);
HDWP32      WINAPI DeferWindowPos32(HDWP32,HWND32,HWND32,INT32,INT32,INT32,INT32,UINT32);
#define     DeferWindowPos WINELIB_NAME(DeferWindowPos)
LRESULT     WINAPI DefFrameProc16(HWND16,HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI DefFrameProc32A(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefFrameProc32W(HWND32,HWND32,UINT32,WPARAM32,LPARAM);
#define     DefFrameProc WINELIB_NAME_AW(DefFrameProc)
LRESULT     WINAPI DefHookProc16(INT16,WPARAM16,LPARAM,HHOOK*);
#define     DefHookProc32(code,wparam,lparam,phhook) \
            CallNextHookEx32(*(phhook),code,wparam,lparam)
#define     DefHookProc WINELIB_NAME(DefHookProc)
LRESULT     WINAPI DefMDIChildProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI DefMDIChildProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefMDIChildProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefMDIChildProc WINELIB_NAME_AW(DefMDIChildProc)
LRESULT     WINAPI DefWindowProc16(HWND16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI DefWindowProc32A(HWND32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI DefWindowProc32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     DefWindowProc WINELIB_NAME_AW(DefWindowProc)
BOOL16      WINAPI DefineHandleTable16(WORD);
#define     DefineHandleTable32(w) ((w),TRUE)
#define     DefineHandleTable WINELIB_NAME(DefineHandleTable)
ATOM        WINAPI DeleteAtom16(ATOM);
ATOM        WINAPI DeleteAtom32(ATOM);
#define     DeleteAtom WINELIB_NAME(DeleteAtom)
BOOL16      WINAPI DeleteDC16(HDC16);
BOOL32      WINAPI DeleteDC32(HDC32);
#define     DeleteDC WINELIB_NAME(DeleteDC)
BOOL16      WINAPI DeleteFile16(LPCSTR);
BOOL32      WINAPI DeleteFile32A(LPCSTR);
BOOL32      WINAPI DeleteFile32W(LPCWSTR);
#define     DeleteFile WINELIB_NAME_AW(DeleteFile)
BOOL16      WINAPI DeleteMenu16(HMENU16,UINT16,UINT16);
BOOL32      WINAPI DeleteMenu32(HMENU32,UINT32,UINT32);
#define     DeleteMenu WINELIB_NAME(DeleteMenu)
BOOL16      WINAPI DeleteMetaFile16(HMETAFILE16);
BOOL32      WINAPI DeleteMetaFile32(HMETAFILE32);
#define     DeleteMetaFile WINELIB_NAME(DeleteMetaFile)
BOOL16      WINAPI DeleteObject16(HGDIOBJ16);
BOOL32      WINAPI DeleteObject32(HGDIOBJ32);
#define     DeleteObject WINELIB_NAME(DeleteObject)
INT32       WINAPI DescribePixelFormat(HDC32,int,UINT32,
                                       LPPIXELFORMATDESCRIPTOR);
VOID        WINAPI DestroyCaret16(void);
BOOL32      WINAPI DestroyCaret32(void);
#define     DestroyCaret WINELIB_NAME(DestroyCaret)
BOOL16      WINAPI DestroyCursor16(HCURSOR16);
BOOL32      WINAPI DestroyCursor32(HCURSOR32);
#define     DestroyCursor WINELIB_NAME(DestroyCursor)
BOOL16      WINAPI DestroyIcon16(HICON16);
BOOL32      WINAPI DestroyIcon32(HICON32);
#define     DestroyIcon WINELIB_NAME(DestroyIcon)
BOOL16      WINAPI DestroyMenu16(HMENU16);
BOOL32      WINAPI DestroyMenu32(HMENU32);
#define     DestroyMenu WINELIB_NAME(DestroyMenu)
BOOL16      WINAPI DestroyWindow16(HWND16);
BOOL32      WINAPI DestroyWindow32(HWND32);
#define     DestroyWindow WINELIB_NAME(DestroyWindow)
INT16       WINAPI DialogBox16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16);
#define     DialogBox32A(inst,template,owner,func) \
            DialogBoxParam32A(inst,template,owner,func,0)
#define     DialogBox32W(inst,template,owner,func) \
            DialogBoxParam32W(inst,template,owner,func,0)
#define     DialogBox WINELIB_NAME_AW(DialogBox)
INT16       WINAPI DialogBoxIndirect16(HINSTANCE16,HANDLE16,HWND16,DLGPROC16);
#define     DialogBoxIndirect32A(inst,template,owner,func) \
            DialogBoxIndirectParam32A(inst,template,owner,func,0)
#define     DialogBoxIndirect32W(inst,template,owner,func) \
            DialogBoxIndirectParam32W(inst,template,owner,func,0)
#define     DialogBoxIndirect WINELIB_NAME_AW(DialogBoxIndirect)
INT16       WINAPI DialogBoxIndirectParam16(HINSTANCE16,HANDLE16,HWND16,DLGPROC16,LPARAM);
INT32       WINAPI DialogBoxIndirectParam32A(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
INT32       WINAPI DialogBoxIndirectParam32W(HINSTANCE32,LPCVOID,HWND32,DLGPROC32,LPARAM);
#define     DialogBoxIndirectParam WINELIB_NAME_AW(DialogBoxIndirectParam)
INT16       WINAPI DialogBoxParam16(HINSTANCE16,SEGPTR,HWND16,DLGPROC16,LPARAM);
INT32       WINAPI DialogBoxParam32A(HINSTANCE32,LPCSTR,HWND32,DLGPROC32,LPARAM);
INT32       WINAPI DialogBoxParam32W(HINSTANCE32,LPCWSTR,HWND32,DLGPROC32,LPARAM);
#define     DialogBoxParam WINELIB_NAME_AW(DialogBoxParam)
LONG        WINAPI DispatchMessage16(const MSG16*);
LONG        WINAPI DispatchMessage32A(const MSG32*);
LONG        WINAPI DispatchMessage32W(const MSG32*);
#define     DispatchMessage WINELIB_NAME_AW(DispatchMessage)
INT16       WINAPI DlgDirList16(HWND16,LPSTR,INT16,INT16,UINT16);
INT32       WINAPI DlgDirList32A(HWND32,LPSTR,INT32,INT32,UINT32);
INT32       WINAPI DlgDirList32W(HWND32,LPWSTR,INT32,INT32,UINT32);
#define     DlgDirList WINELIB_NAME_AW(DlgDirList)
INT16       WINAPI DlgDirListComboBox16(HWND16,LPSTR,INT16,INT16,UINT16);
INT32       WINAPI DlgDirListComboBox32A(HWND32,LPSTR,INT32,INT32,UINT32);
INT32       WINAPI DlgDirListComboBox32W(HWND32,LPWSTR,INT32,INT32,UINT32);
#define     DlgDirListComboBox WINELIB_NAME_AW(DlgDirListComboBox)
BOOL16      WINAPI DlgDirSelectComboBoxEx16(HWND16,LPSTR,INT16,INT16);
BOOL32      WINAPI DlgDirSelectComboBoxEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32      WINAPI DlgDirSelectComboBoxEx32W(HWND32,LPWSTR,INT32,INT32);
#define     DlgDirSelectComboBoxEx WINELIB_NAME_AW(DlgDirSelectComboBoxEx)
BOOL16      WINAPI DlgDirSelectEx16(HWND16,LPSTR,INT16,INT16);
BOOL32      WINAPI DlgDirSelectEx32A(HWND32,LPSTR,INT32,INT32);
BOOL32      WINAPI DlgDirSelectEx32W(HWND32,LPWSTR,INT32,INT32);
#define     DlgDirSelectEx WINELIB_NAME_AW(DlgDirSelectEx)
BOOL16      WINAPI DPtoLP16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI DPtoLP32(HDC32,LPPOINT32,INT32);
#define     DPtoLP WINELIB_NAME(DPtoLP)
BOOL16      WINAPI DragDetect16(HWND16,POINT16);
BOOL32      WINAPI DragDetect32(HWND32,POINT32);
#define     DragDetect WINELIB_NAME(DragDetect)
DWORD       WINAPI DragObject16(HWND16,HWND16,UINT16,HANDLE16,WORD,HCURSOR16);
DWORD       WINAPI DragObject32(HWND32,HWND32,UINT32,DWORD,HCURSOR32);
#define     DragObject WINELIB_NAME(DragObject)
BOOL16      WINAPI DrawEdge16(HDC16,LPRECT16,UINT16,UINT16);
BOOL32      WINAPI DrawEdge32(HDC32,LPRECT32,UINT32,UINT32);
#define     DrawEdge WINELIB_NAME(DrawEdge)
void        WINAPI DrawFocusRect16(HDC16,const RECT16*);
void        WINAPI DrawFocusRect32(HDC32,const RECT32*);
#define     DrawFocusRect WINELIB_NAME(DrawFocusRect)
BOOL16      WINAPI DrawFrameControl16(HDC16,LPRECT16,UINT16,UINT16);
BOOL32      WINAPI DrawFrameControl32(HDC32,LPRECT32,UINT32,UINT32);
#define     DrawFrameControl WINELIB_NAME(DrawFrameControl)
BOOL16      WINAPI DrawIcon16(HDC16,INT16,INT16,HICON16);
BOOL32      WINAPI DrawIcon32(HDC32,INT32,INT32,HICON32);
#define     DrawIcon WINELIB_NAME(DrawIcon)
BOOL16      WINAPI DrawIconEx16(HDC16,INT16,INT16,HICON16,INT16,INT16,
				UINT16,HBRUSH16,UINT16);
BOOL32      WINAPI DrawIconEx32(HDC32,INT32,INT32,HICON32,INT32,INT32,
				UINT32,HBRUSH32,UINT32);
#define     DrawIconEx WINELIB_NAME(DrawIconEx)
VOID        WINAPI DrawMenuBar16(HWND16);
BOOL32      WINAPI DrawMenuBar32(HWND32);
#define     DrawMenuBar WINELIB_NAME(DrawMenuBar)
INT16       WINAPI DrawText16(HDC16,LPCSTR,INT16,LPRECT16,UINT16);
INT32       WINAPI DrawText32A(HDC32,LPCSTR,INT32,LPRECT32,UINT32);
INT32       WINAPI DrawText32W(HDC32,LPCWSTR,INT32,LPRECT32,UINT32);
#define     DrawText WINELIB_NAME_AW(DrawText)
BOOL16      WINAPI Ellipse16(HDC16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Ellipse32(HDC32,INT32,INT32,INT32,INT32);
#define     Ellipse WINELIB_NAME(Ellipse)
BOOL16      WINAPI EmptyClipboard16(void);
BOOL32      WINAPI EmptyClipboard32(void);
#define     EmptyClipboard WINELIB_NAME(EmptyClipboard)
BOOL16      WINAPI EnableMenuItem16(HMENU16,UINT16,UINT16);
BOOL32      WINAPI EnableMenuItem32(HMENU32,UINT32,UINT32);
#define     EnableMenuItem WINELIB_NAME(EnableMenuItem)
BOOL16      WINAPI EnableScrollBar16(HWND16,INT16,UINT16);
BOOL32      WINAPI EnableScrollBar32(HWND32,INT32,UINT32);
#define     EnableScrollBar WINELIB_NAME(EnableScrollBar)
BOOL16      WINAPI EnableWindow16(HWND16,BOOL16);
BOOL32      WINAPI EnableWindow32(HWND32,BOOL32);
#define     EnableWindow WINELIB_NAME(EnableWindow)
BOOL16      WINAPI EndDeferWindowPos16(HDWP16);
BOOL32      WINAPI EndDeferWindowPos32(HDWP32);
#define     EndDeferWindowPos WINELIB_NAME(EndDeferWindowPos)
BOOL16      WINAPI EndDialog16(HWND16,INT16);
BOOL32      WINAPI EndDialog32(HWND32,INT32);
#define     EndDialog WINELIB_NAME(EndDialog)
INT16       WINAPI EndDoc16(HDC16);
INT32       WINAPI EndDoc32(HDC32);
#define     EndDoc WINELIB_NAME(EndDoc)
BOOL16      WINAPI EndPaint16(HWND16,const PAINTSTRUCT16*);
BOOL32      WINAPI EndPaint32(HWND32,const PAINTSTRUCT32*);
#define     EndPaint WINELIB_NAME(EndPaint)
BOOL16      WINAPI EndPath16(HDC16);
BOOL32      WINAPI EndPath32(HDC32);
#define     EndPath WINELIB_NAME(EndPath)
BOOL16      WINAPI EnumChildWindows16(HWND16,WNDENUMPROC16,LPARAM);
BOOL32      WINAPI EnumChildWindows32(HWND32,WNDENUMPROC32,LPARAM);
#define     EnumChildWindows WINELIB_NAME(EnumChildWindows)
UINT16      WINAPI EnumClipboardFormats16(UINT16);
UINT32      WINAPI EnumClipboardFormats32(UINT32);
#define     EnumClipboardFormats WINELIB_NAME(EnumClipboardFormats)
INT16       WINAPI EnumFontFamilies16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT32       WINAPI EnumFontFamilies32A(HDC32,LPCSTR,FONTENUMPROC32A,LPARAM);
INT32       WINAPI EnumFontFamilies32W(HDC32,LPCWSTR,FONTENUMPROC32W,LPARAM);
#define     EnumFontFamilies WINELIB_NAME_AW(EnumFontFamilies)
INT16       WINAPI EnumFontFamiliesEx16(HDC16,LPLOGFONT16,FONTENUMPROCEX16,LPARAM,DWORD);
INT32       WINAPI EnumFontFamiliesEx32A(HDC32,LPLOGFONT32A,FONTENUMPROCEX32A,LPARAM,DWORD);
INT32       WINAPI EnumFontFamiliesEx32W(HDC32,LPLOGFONT32W,FONTENUMPROCEX32W,LPARAM,DWORD);
#define     EnumFontFamiliesEx WINELIB_NAME_AW(EnumFontFamiliesEx)
INT16       WINAPI EnumFonts16(HDC16,LPCSTR,FONTENUMPROC16,LPARAM);
INT32       WINAPI EnumFonts32A(HDC32,LPCSTR,FONTENUMPROC32A,LPARAM);
INT32       WINAPI EnumFonts32W(HDC32,LPCWSTR,FONTENUMPROC32W,LPARAM);
#define     EnumFonts WINELIB_NAME_AW(EnumFonts)
BOOL16      WINAPI EnumMetaFile16(HDC16,HMETAFILE16,MFENUMPROC16,LPARAM);
BOOL32      WINAPI EnumMetaFile32(HDC32,HMETAFILE32,MFENUMPROC32,LPARAM);
#define     EnumMetaFile WINELIB_NAME(EnumMetaFile)
INT16       WINAPI EnumObjects16(HDC16,INT16,GOBJENUMPROC16,LPARAM);
INT32       WINAPI EnumObjects32(HDC32,INT32,GOBJENUMPROC32,LPARAM);
#define     EnumObjects WINELIB_NAME(EnumObjects)
INT16       WINAPI EnumProps16(HWND16,PROPENUMPROC16);
INT32       WINAPI EnumProps32A(HWND32,PROPENUMPROC32A);
INT32       WINAPI EnumProps32W(HWND32,PROPENUMPROC32W);
#define     EnumProps WINELIB_NAME_AW(EnumProps)
BOOL16      WINAPI EnumTaskWindows16(HTASK16,WNDENUMPROC16,LPARAM);
#define     EnumTaskWindows32(handle,proc,lparam) \
            EnumThreadWindows(handle,proc,lparam)
#define     EnumTaskWindows WINELIB_NAME(EnumTaskWindows)
BOOL16      WINAPI EnumWindows16(WNDENUMPROC16,LPARAM);
BOOL32      WINAPI EnumWindows32(WNDENUMPROC32,LPARAM);
#define     EnumWindows WINELIB_NAME(EnumWindows)
BOOL16      WINAPI EqualRect16(const RECT16*,const RECT16*);
BOOL32      WINAPI EqualRect32(const RECT32*,const RECT32*);
#define     EqualRect WINELIB_NAME(EqualRect)
BOOL16      WINAPI EqualRgn16(HRGN16,HRGN16);
BOOL32      WINAPI EqualRgn32(HRGN32,HRGN32);
#define     EqualRgn WINELIB_NAME(EqualRgn)
INT16       WINAPI Escape16(HDC16,INT16,INT16,SEGPTR,SEGPTR);
INT32       WINAPI Escape32(HDC32,INT32,INT32,LPVOID,LPVOID);
#define     Escape WINELIB_NAME(Escape)
LONG        WINAPI EscapeCommFunction16(UINT16,UINT16);
BOOL32      WINAPI EscapeCommFunction32(INT32,UINT32);
#define     EscapeCommFunction WINELIB_NAME(EscapeCommFunction)
INT16       WINAPI ExcludeClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT32       WINAPI ExcludeClipRect32(HDC32,INT32,INT32,INT32,INT32);
#define     ExcludeClipRect WINELIB_NAME(ExcludeClipRect)
INT16       WINAPI ExcludeUpdateRgn16(HDC16,HWND16);
INT32       WINAPI ExcludeUpdateRgn32(HDC32,HWND32);
#define     ExcludeUpdateRgn WINELIB_NAME(ExcludeUpdateRgn)
BOOL16      WINAPI ExitWindows16(DWORD,UINT16);
#define     ExitWindows32(a,b) ExitWindowsEx(EWX_LOGOFF,0xffffffff)
#define     ExitWindows WINELIB_NAME(ExitWindows)
HPEN16      WINAPI ExtCreatePen16(DWORD,DWORD,const LOGBRUSH16*,DWORD,const DWORD*);
HPEN32      WINAPI ExtCreatePen32(DWORD,DWORD,const LOGBRUSH32*,DWORD,const DWORD*);
#define     ExtCreatePen WINELIB_NAME(ExtCreatePen)
BOOL16      WINAPI ExtFloodFill16(HDC16,INT16,INT16,COLORREF,UINT16);
BOOL32      WINAPI ExtFloodFill32(HDC32,INT32,INT32,COLORREF,UINT32);
#define     ExtFloodFill WINELIB_NAME(ExtFloodFill)
HICON16     WINAPI ExtractIcon16(HINSTANCE16,LPCSTR,UINT16);
HICON32     WINAPI ExtractIcon32A(HINSTANCE32,LPCSTR,UINT32);
HICON32     WINAPI ExtractIcon32W(HINSTANCE32,LPCWSTR,UINT32);
#define     ExtractIcon WINELIB_NAME_AW(ExtractIcon)
HICON16     WINAPI ExtractAssociatedIcon16(HINSTANCE16,LPSTR,LPWORD);
HICON32     WINAPI ExtractAssociatedIcon32A(HINSTANCE32,LPSTR,LPWORD);
HICON32     WINAPI ExtractAssociatedIcon32W(HINSTANCE32,LPWSTR,LPWORD);
#define     ExtractAssociatedIcon WINELIB_NAME_AW(ExtractAssociatedIcon)
BOOL16      WINAPI ExtTextOut16(HDC16,INT16,INT16,UINT16,const RECT16*,
                                LPCSTR,UINT16,const INT16*);
BOOL32      WINAPI ExtTextOut32A(HDC32,INT32,INT32,UINT32,const RECT32*,
                                 LPCSTR,UINT32,const INT32*);
BOOL32      WINAPI ExtTextOut32W(HDC32,INT32,INT32,UINT32,const RECT32*,
                                 LPCWSTR,UINT32,const INT32*);
#define     ExtTextOut WINELIB_NAME_AW(ExtTextOut)
void        WINAPI FatalAppExit16(UINT16,LPCSTR);
void        WINAPI FatalAppExit32A(UINT32,LPCSTR);
void        WINAPI FatalAppExit32W(UINT32,LPCWSTR);
#define     FatalAppExit WINELIB_NAME_AW(FatalAppExit)
BOOL16      WINAPI FillPath16(HDC16);
BOOL32      WINAPI FillPath32(HDC32);
#define     FillPath WINELIB_NAME(FillPath)
INT16       WINAPI FillRect16(HDC16,const RECT16*,HBRUSH16);
INT32       WINAPI FillRect32(HDC32,const RECT32*,HBRUSH32);
#define     FillRect WINELIB_NAME(FillRect)
BOOL16      WINAPI FillRgn16(HDC16,HRGN16,HBRUSH16);
BOOL32      WINAPI FillRgn32(HDC32,HRGN32,HBRUSH32);
#define     FillRgn WINELIB_NAME(FillRgn)
ATOM        WINAPI FindAtom16(SEGPTR);
ATOM        WINAPI FindAtom32A(LPCSTR);
ATOM        WINAPI FindAtom32W(LPCWSTR);
#define     FindAtom WINELIB_NAME_AW(FindAtom)
BOOL16      WINAPI FindClose16(HANDLE16);
BOOL32      WINAPI FindClose32(HANDLE32);
#define     FindClose WINELIB_NAME(FindClose)
HINSTANCE16 WINAPI FindExecutable16(LPCSTR,LPCSTR,LPSTR);
HINSTANCE32 WINAPI FindExecutable32A(LPCSTR,LPCSTR,LPSTR);
HINSTANCE32 WINAPI FindExecutable32W(LPCWSTR,LPCWSTR,LPWSTR);
#define     FindExecutable WINELIB_NAME_AW(FindExecutable)
HANDLE16    WINAPI FindFirstFile16(LPCSTR,LPWIN32_FIND_DATA32A);
HANDLE32    WINAPI FindFirstFile32A(LPCSTR,LPWIN32_FIND_DATA32A);
HANDLE32    WINAPI FindFirstFile32W(LPCWSTR,LPWIN32_FIND_DATA32W);
#define     FindFirstFile WINELIB_NAME_AW(FindFirstFile)
BOOL16      WINAPI FindNextFile16(HANDLE16,LPWIN32_FIND_DATA32A);
BOOL32      WINAPI FindNextFile32A(HANDLE32,LPWIN32_FIND_DATA32A);
BOOL32      WINAPI FindNextFile32W(HANDLE32,LPWIN32_FIND_DATA32W);
#define     FindNextFile WINELIB_NAME_AW(FindNextFile)
HRSRC16     WINAPI FindResource16(HINSTANCE16,SEGPTR,SEGPTR);
HRSRC32     WINAPI FindResource32A(HMODULE32,LPCSTR,LPCSTR);
HRSRC32     WINAPI FindResource32W(HMODULE32,LPCWSTR,LPCWSTR);
#define     FindResource WINELIB_NAME_AW(FindResource)
HWND16      WINAPI FindWindow16(SEGPTR,LPCSTR);
HWND32      WINAPI FindWindow32A(LPCSTR,LPCSTR);
HWND32      WINAPI FindWindow32W(LPCWSTR,LPCWSTR);
#define     FindWindow WINELIB_NAME_AW(FindWindow)
HWND16      WINAPI FindWindowEx16(HWND16,HWND16,SEGPTR,LPCSTR);
HWND32      WINAPI FindWindowEx32A(HWND32,HWND32,LPCSTR,LPCSTR);
HWND32      WINAPI FindWindowEx32W(HWND32,HWND32,LPCWSTR,LPCWSTR);
#define     FindWindowEx WINELIB_NAME_AW(FindWindowEx)
BOOL16      WINAPI FlashWindow16(HWND16,BOOL16);
BOOL32      WINAPI FlashWindow32(HWND32,BOOL32);
#define     FlashWindow WINELIB_NAME(FlashWindow)
BOOL16      WINAPI FloodFill16(HDC16,INT16,INT16,COLORREF);
BOOL32      WINAPI FloodFill32(HDC32,INT32,INT32,COLORREF);
#define     FloodFill WINELIB_NAME(FloodFill)
INT16       WINAPI FrameRect16(HDC16,const RECT16*,HBRUSH16);
INT32       WINAPI FrameRect32(HDC32,const RECT32*,HBRUSH32);
#define     FrameRect WINELIB_NAME(FrameRect)
BOOL16      WINAPI FrameRgn16(HDC16,HRGN16,HBRUSH16,INT16,INT16);
BOOL32      WINAPI FrameRgn32(HDC32,HRGN32,HBRUSH32,INT32,INT32);
#define     FrameRgn WINELIB_NAME(FrameRgn)
VOID        WINAPI FreeLibrary16(HINSTANCE16);
BOOL32      WINAPI FreeLibrary32(HMODULE32);
#define     FreeLibrary WINELIB_NAME(FreeLibrary)
BOOL16      WINAPI FreeModule16(HMODULE16);
#define     FreeModule32(handle) FreeLibrary32(handle)
#define     FreeModule WINELIB_NAME(FreeModule)
void        WINAPI FreeProcInstance16(FARPROC16);
#define     FreeProcInstance32(proc) /*nothing*/
#define     FreeProcInstance WINELIB_NAME(FreeProcInstance)
BOOL16      WINAPI FreeResource16(HGLOBAL16);
BOOL32      WINAPI FreeResource32(HGLOBAL32);
#define     FreeResource WINELIB_NAME(FreeResource)
HWND16      WINAPI GetActiveWindow16(void);
HWND32      WINAPI GetActiveWindow32(void);
#define     GetActiveWindow WINELIB_NAME(GetActiveWindow)
DWORD       WINAPI GetAppCompatFlags16(HTASK16);
DWORD       WINAPI GetAppCompatFlags32(HTASK32);
#define     GetAppCompatFlags WINELIB_NAME(GetAppCompatFlags)
INT16       WINAPI GetArcDirection16(HDC16);
INT32       WINAPI GetArcDirection32(HDC32);
#define     GetArcDirection WINELIB_NAME(GetArcDirection)
WORD        WINAPI GetAsyncKeyState16(INT16);
WORD        WINAPI GetAsyncKeyState32(INT32);
#define     GetAsyncKeyState WINELIB_NAME(GetAsyncKeyState)
UINT16      WINAPI GetAtomName16(ATOM,LPSTR,INT16);
UINT32      WINAPI GetAtomName32A(ATOM,LPSTR,INT32);
UINT32      WINAPI GetAtomName32W(ATOM,LPWSTR,INT32);
#define     GetAtomName WINELIB_NAME_AW(GetAtomName)
LONG        WINAPI GetBitmapBits16(HBITMAP16,LONG,LPVOID);
LONG        WINAPI GetBitmapBits32(HBITMAP32,LONG,LPVOID);
#define     GetBitmapBits WINELIB_NAME(GetBitmapBits)
BOOL16      WINAPI GetBitmapDimensionEx16(HBITMAP16,LPSIZE16);
BOOL32      WINAPI GetBitmapDimensionEx32(HBITMAP32,LPSIZE32);
#define     GetBitmapDimensionEx WINELIB_NAME(GetBitmapDimensionEx)
BOOL16      WINAPI GetBrushOrgEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetBrushOrgEx32(HDC32,LPPOINT32);
#define     GetBrushOrgEx WINELIB_NAME(GetBrushOrgEx)
COLORREF    WINAPI GetBkColor16(HDC16);
COLORREF    WINAPI GetBkColor32(HDC32);
#define     GetBkColor WINELIB_NAME(GetBkColor)
INT16       WINAPI GetBkMode16(HDC16);
INT32       WINAPI GetBkMode32(HDC32);
#define     GetBkMode WINELIB_NAME(GetBkMode)
HWND16      WINAPI GetCapture16(void);
HWND32      WINAPI GetCapture32(void);
#define     GetCapture WINELIB_NAME(GetCapture)
UINT16      WINAPI GetCaretBlinkTime16(void);
UINT32      WINAPI GetCaretBlinkTime32(void);
#define     GetCaretBlinkTime WINELIB_NAME(GetCaretBlinkTime)
VOID        WINAPI GetCaretPos16(LPPOINT16);
BOOL32      WINAPI GetCaretPos32(LPPOINT32);
#define     GetCaretPos WINELIB_NAME(GetCaretPos)
BOOL16      WINAPI GetCharABCWidths16(HDC16,UINT16,UINT16,LPABC16);
BOOL32      WINAPI GetCharABCWidths32A(HDC32,UINT32,UINT32,LPABC32);
BOOL32      WINAPI GetCharABCWidths32W(HDC32,UINT32,UINT32,LPABC32);
#define     GetCharABCWidths WINELIB_NAME_AW(GetCharABCWidths)
BOOL16      WINAPI GetCharWidth16(HDC16,UINT16,UINT16,LPINT16);
BOOL32      WINAPI GetCharWidth32A(HDC32,UINT32,UINT32,LPINT32);
BOOL32      WINAPI GetCharWidth32W(HDC32,UINT32,UINT32,LPINT32);
#define     GetCharWidth WINELIB_NAME_AW(GetCharWidth)
BOOL16      WINAPI GetClassInfo16(HINSTANCE16,SEGPTR,WNDCLASS16 *);
BOOL32      WINAPI GetClassInfo32A(HINSTANCE32,LPCSTR,WNDCLASS32A *);
BOOL32      WINAPI GetClassInfo32W(HINSTANCE32,LPCWSTR,WNDCLASS32W *);
#define     GetClassInfo WINELIB_NAME_AW(GetClassInfo)
BOOL16      WINAPI GetClassInfoEx16(HINSTANCE16,SEGPTR,WNDCLASSEX16 *);
BOOL32      WINAPI GetClassInfoEx32A(HINSTANCE32,LPCSTR,WNDCLASSEX32A *);
BOOL32      WINAPI GetClassInfoEx32W(HINSTANCE32,LPCWSTR,WNDCLASSEX32W *);
#define     GetClassInfoEx WINELIB_NAME_AW(GetClassInfoEx)
LONG        WINAPI GetClassLong16(HWND16,INT16);
LONG        WINAPI GetClassLong32A(HWND32,INT32);
LONG        WINAPI GetClassLong32W(HWND32,INT32);
#define     GetClassLong WINELIB_NAME_AW(GetClassLong)
INT16       WINAPI GetClassName16(HWND16,LPSTR,INT16);
INT32       WINAPI GetClassName32A(HWND32,LPSTR,INT32);
INT32       WINAPI GetClassName32W(HWND32,LPWSTR,INT32);
#define     GetClassName WINELIB_NAME_AW(GetClassName)
WORD        WINAPI GetClassWord16(HWND16,INT16);
WORD        WINAPI GetClassWord32(HWND32,INT32);
#define     GetClassWord WINELIB_NAME(GetClassWord)
void        WINAPI GetClientRect16(HWND16,LPRECT16);
void        WINAPI GetClientRect32(HWND32,LPRECT32);
#define     GetClientRect WINELIB_NAME(GetClientRect)
HANDLE16    WINAPI GetClipboardData16(UINT16);
HANDLE32    WINAPI GetClipboardData32(UINT32);
#define     GetClipboardData WINELIB_NAME(GetClipboardData)
INT16       WINAPI GetClipboardFormatName16(UINT16,LPSTR,INT16);
INT32       WINAPI GetClipboardFormatName32A(UINT32,LPSTR,INT32);
INT32       WINAPI GetClipboardFormatName32W(UINT32,LPWSTR,INT32);
#define     GetClipboardFormatName WINELIB_NAME_AW(GetClipboardFormatName)
HWND16      WINAPI GetClipboardOwner16(void);
HWND32      WINAPI GetClipboardOwner32(void);
#define     GetClipboardOwner WINELIB_NAME(GetClipboardOwner)
HWND16      WINAPI GetClipboardViewer16(void);
HWND32      WINAPI GetClipboardViewer32(void);
#define     GetClipboardViewer WINELIB_NAME(GetClipboardViewer)
INT16       WINAPI GetClipBox16(HDC16,LPRECT16);
INT32       WINAPI GetClipBox32(HDC32,LPRECT32);
#define     GetClipBox WINELIB_NAME(GetClipBox)
void        WINAPI GetClipCursor16(LPRECT16);
void        WINAPI GetClipCursor32(LPRECT32);
#define     GetClipCursor WINELIB_NAME(GetClipCursor)
HRGN16      WINAPI GetClipRgn16(HDC16);
INT32       WINAPI GetClipRgn32(HDC32,HRGN32);
#define     GetClipRgn WINELIB_NAME(GetClipRgn)
INT16       WINAPI GetCommState16(INT16,LPDCB16);
BOOL32      WINAPI GetCommState32(INT32,LPDCB32);
#define     GetCommState WINELIB_NAME(GetCommState)
UINT16      WINAPI GetCurrentDirectory16(UINT16,LPSTR);
UINT32      WINAPI GetCurrentDirectory32A(UINT32,LPSTR);
UINT32      WINAPI GetCurrentDirectory32W(UINT32,LPWSTR);
#define     GetCurrentDirectory WINELIB_NAME_AW(GetCurrentDirectory)
BOOL16      WINAPI GetCurrentPositionEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetCurrentPositionEx32(HDC32,LPPOINT32);
#define     GetCurrentPositionEx WINELIB_NAME(GetCurrentPositionEx)
DWORD       WINAPI GetCurrentTime16(void);
#define     GetCurrentTime32() GetTickCount()
#define     GetCurrentTime WINELIB_NAME(GetCurrentTime)
HCURSOR16   WINAPI GetCursor16(void);
HCURSOR32   WINAPI GetCursor32(void);
#define     GetCursor WINELIB_NAME(GetCursor)
void        WINAPI GetCursorPos16(LPPOINT16);
void        WINAPI GetCursorPos32(LPPOINT32);
#define     GetCursorPos WINELIB_NAME(GetCursorPos)
HDC16       WINAPI GetDC16(HWND16);
HDC32       WINAPI GetDC32(HWND32);
#define     GetDC WINELIB_NAME(GetDC)
HDC16       WINAPI GetDCEx16(HWND16,HRGN16,DWORD);
HDC32       WINAPI GetDCEx32(HWND32,HRGN32,DWORD);
#define     GetDCEx WINELIB_NAME(GetDCEx)
HWND16      WINAPI GetDesktopWindow16(void);
HWND32      WINAPI GetDesktopWindow32(void);
#define     GetDesktopWindow WINELIB_NAME(GetDesktopWindow)
INT16       WINAPI GetDeviceCaps16(HDC16,INT16);
INT32       WINAPI GetDeviceCaps32(HDC32,INT32);
#define     GetDeviceCaps WINELIB_NAME(GetDeviceCaps)
UINT16      WINAPI GetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
UINT32      WINAPI GetDIBColorTable32(HDC32,UINT32,UINT32,RGBQUAD*);
#define     GetDIBColorTable WINELIB_NAME(GetDIBColorTable)
INT16       WINAPI GetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPSTR,LPBITMAPINFO,UINT16);
INT32       WINAPI GetDIBits32(HDC32,HBITMAP32,UINT32,UINT32,LPSTR,LPBITMAPINFO,UINT32);
#define     GetDIBits WINELIB_NAME(GetDIBits)
BOOL16      WINAPI GetDiskFreeSpace16(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL32      WINAPI GetDiskFreeSpace32A(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL32      WINAPI GetDiskFreeSpace32W(LPCWSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
#define     GetDiskFreeSpace WINELIB_NAME_AW(GetDiskFreeSpace)
INT16       WINAPI GetDlgCtrlID16(HWND16);
INT32       WINAPI GetDlgCtrlID32(HWND32);
#define     GetDlgCtrlID WINELIB_NAME(GetDlgCtrlID)
HWND16      WINAPI GetDlgItem16(HWND16,INT16);
HWND32      WINAPI GetDlgItem32(HWND32,INT32);
#define     GetDlgItem WINELIB_NAME(GetDlgItem)
UINT16      WINAPI GetDlgItemInt16(HWND16,INT16,BOOL16*,BOOL16);
UINT32      WINAPI GetDlgItemInt32(HWND32,INT32,BOOL32*,BOOL32);
#define     GetDlgItemInt WINELIB_NAME(GetDlgItemInt)
INT16       WINAPI GetDlgItemText16(HWND16,INT16,SEGPTR,UINT16);
INT32       WINAPI GetDlgItemText32A(HWND32,INT32,LPSTR,UINT32);
INT32       WINAPI GetDlgItemText32W(HWND32,INT32,LPWSTR,UINT32);
#define     GetDlgItemText WINELIB_NAME_AW(GetDlgItemText)
UINT16      WINAPI GetDoubleClickTime16(void);
UINT32      WINAPI GetDoubleClickTime32(void);
#define     GetDoubleClickTime WINELIB_NAME(GetDoubleClickTime)
UINT16      WINAPI GetDriveType16(UINT16); /* yes, the arguments differ */
UINT32      WINAPI GetDriveType32A(LPCSTR);
UINT32      WINAPI GetDriveType32W(LPCWSTR);
#define     GetDriveType WINELIB_NAME_AW(GetDriveType)
INT16       WINAPI GetExpandedName16(LPCSTR,LPSTR);
INT32       WINAPI GetExpandedName32A(LPCSTR,LPSTR);
INT32       WINAPI GetExpandedName32W(LPCWSTR,LPWSTR);
#define     GetExpandedName WINELIB_NAME_AW(GetExpandedName)
DWORD       WINAPI GetFileAttributes16(LPCSTR);
DWORD       WINAPI GetFileAttributes32A(LPCSTR);
DWORD       WINAPI GetFileAttributes32W(LPCWSTR);
#define     GetFileAttributes WINELIB_NAME_AW(GetFileAttributes)
DWORD       WINAPI GetFileVersionInfoSize16(LPCSTR,LPDWORD);
DWORD       WINAPI GetFileVersionInfoSize32A(LPCSTR,LPDWORD);
DWORD       WINAPI GetFileVersionInfoSize32W(LPCWSTR,LPDWORD);
#define     GetFileVersionInfoSize WINELIB_NAME_AW(GetFileVersionInfoSize)
DWORD       WINAPI GetFileVersionInfo16(LPCSTR,DWORD,DWORD,LPVOID);
DWORD       WINAPI GetFileVersionInfo32A(LPCSTR,DWORD,DWORD,LPVOID);
DWORD       WINAPI GetFileVersionInfo32W(LPCWSTR,DWORD,DWORD,LPVOID);
#define     GetFileVersionInfo WINELIB_NAME_AW(GetFileVersionInfo)
HWND16      WINAPI GetFocus16(void);
HWND32      WINAPI GetFocus32(void);
#define     GetFocus WINELIB_NAME(GetFocus)
HWND16      WINAPI GetForegroundWindow16(void);
HWND32      WINAPI GetForegroundWindow32(void);
#define     GetForegroundWindow WINELIB_NAME(GetForegroundWindow)
DWORD       WINAPI GetFreeSpace16(UINT16);
#define     GetFreeSpace32(w) (0x100000L)
#define     GetFreeSpace WINELIB_NAME(GetFreeSpace)
DWORD       WINAPI GetGlyphOutline16(HDC16,UINT16,UINT16,LPGLYPHMETRICS16,DWORD,LPVOID,const MAT2*);
DWORD       WINAPI GetGlyphOutline32A(HDC32,UINT32,UINT32,LPGLYPHMETRICS32,DWORD,LPVOID,const MAT2*);
DWORD       WINAPI GetGlyphOutline32W(HDC32,UINT32,UINT32,LPGLYPHMETRICS32,DWORD,LPVOID,const MAT2*);
#define     GetGlyphOutline WINELIB_NAME_AW(GetGlyphOutline)
BOOL16      WINAPI GetInputState16(void);
BOOL32      WINAPI GetInputState32(void);
#define     GetInputState WINELIB_NAME(GetInputState)
UINT16      WINAPI GetInternalWindowPos16(HWND16,LPRECT16,LPPOINT16);
UINT32      WINAPI GetInternalWindowPos32(HWND32,LPRECT32,LPPOINT32);
#define     GetInternalWindowPos WINELIB_NAME(GetInternalWindowPos)
INT16       WINAPI GetKBCodePage16(void);
UINT32      WINAPI GetKBCodePage32(void);
#define     GetKBCodePage WINELIB_NAME(GetKBCodePage)
INT16       WINAPI GetKerningPairs16(HDC16,INT16,LPKERNINGPAIR16);
DWORD       WINAPI GetKerningPairs32A(HDC32,DWORD,LPKERNINGPAIR32);
DWORD       WINAPI GetKerningPairs32W(HDC32,DWORD,LPKERNINGPAIR32);
#define     GetKerningPairs WINELIB_NAME_AW(GetKerningPairs)
INT16       WINAPI GetKeyboardType16(INT16);
INT32       WINAPI GetKeyboardType32(INT32);
#define     GetKeyboardType WINELIB_NAME(GetKeyboardType)
INT16       WINAPI GetKeyNameText16(LONG,LPSTR,INT16);
INT32       WINAPI GetKeyNameText32A(LONG,LPSTR,INT32);
INT32       WINAPI GetKeyNameText32W(LONG,LPWSTR,INT32);
#define     GetKeyNameText WINELIB_NAME_AW(GetKeyNameText)
WORD        WINAPI GetKeyState16(INT16);
WORD        WINAPI GetKeyState32(INT32);
#define     GetKeyState WINELIB_NAME(GetKeyState)
HWND16      WINAPI GetLastActivePopup16(HWND16);
HWND32      WINAPI GetLastActivePopup32(HWND32);
#define     GetLastActivePopup WINELIB_NAME(GetLastActivePopup)
UINT32      WINAPI GetLogicalDriveStrings32A(UINT32,LPSTR);
UINT32      WINAPI GetLogicalDriveStrings32W(UINT32,LPWSTR);
#define     GetLogicalDriveStrings WINELIB_NAME_AW(GetLogicalDriveStrings)
INT16       WINAPI GetLocaleInfo16(LCID,LCTYPE,LPSTR,INT16);
INT32       WINAPI GetLocaleInfo32A(LCID,LCTYPE,LPSTR,INT32);
INT32       WINAPI GetLocaleInfo32W(LCID,LCTYPE,LPWSTR,INT32);
#define     GetLocaleInfo WINELIB_NAME_AW(GetLocaleInfo)
INT16       WINAPI GetMapMode16(HDC16);
INT32       WINAPI GetMapMode32(HDC32);
#define     GetMapMode WINELIB_NAME(GetMapMode)
HMENU16     WINAPI GetMenu16(HWND16);
HMENU32     WINAPI GetMenu32(HWND32);
#define     GetMenu WINELIB_NAME(GetMenu)
INT16       WINAPI GetMenuItemCount16(HMENU16);
INT32       WINAPI GetMenuItemCount32(HMENU32);
#define     GetMenuItemCount WINELIB_NAME(GetMenuItemCount)
UINT16      WINAPI GetMenuItemID16(HMENU16,INT16);
UINT32      WINAPI GetMenuItemID32(HMENU32,INT32);
#define     GetMenuItemID WINELIB_NAME(GetMenuItemID)
BOOL16      WINAPI GetMenuItemRect16(HWND16,HMENU16,UINT16,LPRECT16);
BOOL32      WINAPI GetMenuItemRect32(HWND32,HMENU32,UINT32,LPRECT32);
#define     GetMenuItemRect WINELIB_NAME(GetMenuItemRect)
UINT16      WINAPI GetMenuState16(HMENU16,UINT16,UINT16);
UINT32      WINAPI GetMenuState32(HMENU32,UINT32,UINT32);
#define     GetMenuState WINELIB_NAME(GetMenuState)
INT16       WINAPI GetMenuString16(HMENU16,UINT16,LPSTR,INT16,UINT16);
INT32       WINAPI GetMenuString32A(HMENU32,UINT32,LPSTR,INT32,UINT32);
INT32       WINAPI GetMenuString32W(HMENU32,UINT32,LPWSTR,INT32,UINT32);
#define     GetMenuString WINELIB_NAME_AW(GetMenuString)
BOOL16      WINAPI GetMessage16(SEGPTR,HWND16,UINT16,UINT16);
BOOL32      WINAPI GetMessage32A(LPMSG32,HWND32,UINT32,UINT32);
BOOL32      WINAPI GetMessage32W(LPMSG32,HWND32,UINT32,UINT32);
#define     GetMessage WINELIB_NAME_AW(GetMessage)
HMETAFILE16 WINAPI GetMetaFile16(LPCSTR);
HMETAFILE32 WINAPI GetMetaFile32A(LPCSTR);
HMETAFILE32 WINAPI GetMetaFile32W(LPCWSTR);
#define     GetMetaFile WINELIB_NAME_AW(GetMetaFile)
INT16       WINAPI GetModuleFileName16(HINSTANCE16,LPSTR,INT16);
DWORD       WINAPI GetModuleFileName32A(HMODULE32,LPSTR,DWORD);
DWORD       WINAPI GetModuleFileName32W(HMODULE32,LPWSTR,DWORD);
#define     GetModuleFileName WINELIB_NAME_AW(GetModuleFileName)
HMODULE16   WINAPI GetModuleHandle16(LPCSTR);
HMODULE32   WINAPI GetModuleHandle32A(LPCSTR);
HMODULE32   WINAPI GetModuleHandle32W(LPCWSTR);
#define     GetModuleHandle WINELIB_NAME_AW(GetModuleHandle)
DWORD       WINAPI GetNearestColor16(HDC16,DWORD);
DWORD       WINAPI GetNearestColor32(HDC32,DWORD);
#define     GetNearestColor WINELIB_NAME(GetNearestColor)
UINT16      WINAPI GetNearestPaletteIndex16(HPALETTE16,COLORREF);
UINT32      WINAPI GetNearestPaletteIndex32(HPALETTE32,COLORREF);
#define     GetNearestPaletteIndex WINELIB_NAME(GetNearestPaletteIndex)
HWND16      WINAPI GetNextDlgGroupItem16(HWND16,HWND16,BOOL16);
HWND32      WINAPI GetNextDlgGroupItem32(HWND32,HWND32,BOOL32);
#define     GetNextDlgGroupItem WINELIB_NAME(GetNextDlgGroupItem)
HWND16      WINAPI GetNextDlgTabItem16(HWND16,HWND16,BOOL16);
HWND32      WINAPI GetNextDlgTabItem32(HWND32,HWND32,BOOL32);
#define     GetNextDlgTabItem WINELIB_NAME(GetNextDlgTabItem)
HWND16      WINAPI GetNextWindow16(HWND16,WORD);
#define     GetNextWindow32 GetWindow32
#define     GetNextWindow WINELIB_NAME(GetNextWindow)
INT16       WINAPI GetObject16(HANDLE16,INT16,LPVOID);
INT32       WINAPI GetObject32A(HANDLE32,INT32,LPVOID);
INT32       WINAPI GetObject32W(HANDLE32,INT32,LPVOID);
#define     GetObject WINELIB_NAME_AW(GetObject)
HWND16      WINAPI GetOpenClipboardWindow16(void);
HWND32      WINAPI GetOpenClipboardWindow32(void);
#define     GetOpenClipboardWindow WINELIB_NAME(GetOpenClipboardWindow)
UINT16      WINAPI GetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
UINT32      WINAPI GetPaletteEntries32(HPALETTE32,UINT32,UINT32,LPPALETTEENTRY);
#define     GetPaletteEntries WINELIB_NAME(GetPaletteEntries)
HWND16      WINAPI GetParent16(HWND16);
HWND32      WINAPI GetParent32(HWND32);
#define     GetParent WINELIB_NAME(GetParent)
INT16       WINAPI GetPath16(HDC16,LPPOINT16,LPBYTE,INT16);
INT32       WINAPI GetPath32(HDC32,LPPOINT32,LPBYTE,INT32);
#define     GetPath WINELIB_NAME(GetPath)
COLORREF    WINAPI GetPixel16(HDC16,INT16,INT16);
COLORREF    WINAPI GetPixel32(HDC32,INT32,INT32);
#define     GetPixel WINELIB_NAME(GetPixel)
INT32       WINAPI GetPixelFormat(HDC32);
INT16       WINAPI GetPolyFillMode16(HDC16);
INT32       WINAPI GetPolyFillMode32(HDC32);
#define     GetPolyFillMode WINELIB_NAME(GetPolyFillMode)
INT16       WINAPI GetPriorityClipboardFormat16(UINT16*,INT16);
INT32       WINAPI GetPriorityClipboardFormat32(UINT32*,INT32);
#define     GetPriorityClipboardFormat WINELIB_NAME(GetPriorityClipboardFormat)
UINT16      WINAPI GetPrivateProfileInt16(LPCSTR,LPCSTR,INT16,LPCSTR);
UINT32      WINAPI GetPrivateProfileInt32A(LPCSTR,LPCSTR,INT32,LPCSTR);
UINT32      WINAPI GetPrivateProfileInt32W(LPCWSTR,LPCWSTR,INT32,LPCWSTR);
#define     GetPrivateProfileInt WINELIB_NAME_AW(GetPrivateProfileInt)
INT16       WINAPI GetPrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT16,LPCSTR);
INT32       WINAPI GetPrivateProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT32,LPCSTR);
INT32       WINAPI GetPrivateProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,INT32,LPCWSTR);
#define     GetPrivateProfileString WINELIB_NAME_AW(GetPrivateProfileString)
FARPROC16   WINAPI GetProcAddress16(HMODULE16,SEGPTR);
FARPROC32   WINAPI GetProcAddress32(HMODULE32,LPCSTR);
#define     GetProcAddress WINELIB_NAME(GetProcAddress)
UINT16      WINAPI GetProfileInt16(LPCSTR,LPCSTR,INT16);
UINT32      WINAPI GetProfileInt32A(LPCSTR,LPCSTR,INT32);
UINT32      WINAPI GetProfileInt32W(LPCWSTR,LPCWSTR,INT32);
#define     GetProfileInt WINELIB_NAME_AW(GetProfileInt)
INT16       WINAPI GetProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT16);
INT32       WINAPI GetProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPSTR,INT32);
INT32       WINAPI GetProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,INT32);
#define     GetProfileString WINELIB_NAME_AW(GetProfileString)
HANDLE16    WINAPI GetProp16(HWND16,LPCSTR);
HANDLE32    WINAPI GetProp32A(HWND32,LPCSTR);
HANDLE32    WINAPI GetProp32W(HWND32,LPCWSTR);
#define     GetProp WINELIB_NAME_AW(GetProp)
DWORD       WINAPI GetQueueStatus16(UINT16);
DWORD       WINAPI GetQueueStatus32(UINT32);
#define     GetQueueStatus WINELIB_NAME(GetQueueStatus)
BOOL16      WINAPI GetRasterizerCaps16(LPRASTERIZER_STATUS,UINT16);
BOOL32      WINAPI GetRasterizerCaps32(LPRASTERIZER_STATUS,UINT32);
#define     GetRasterizerCaps WINELIB_NAME(GetRasterizerCaps)
INT16       WINAPI GetRelAbs16(HDC16);
INT32       WINAPI GetRelAbs32(HDC32);
#define     GetRelAbs WINELIB_NAME(GetRelAbs)
INT16       WINAPI GetRgnBox16(HRGN16,LPRECT16);
INT32       WINAPI GetRgnBox32(HRGN32,LPRECT32);
#define     GetRgnBox WINELIB_NAME(GetRgnBox)
INT16       WINAPI GetROP216(HDC16);
INT32       WINAPI GetROP232(HDC32);
#define     GetROP2 WINELIB_NAME(GetROP2)
BOOL16      WINAPI GetScrollInfo16(HWND16,INT16,LPSCROLLINFO);
BOOL32      WINAPI GetScrollInfo32(HWND32,INT32,LPSCROLLINFO);
#define     GetScrollInfo WINELIB_NAME(GetScrollInfo)
INT16       WINAPI GetScrollPos16(HWND16,INT16);
INT32       WINAPI GetScrollPos32(HWND32,INT32);
#define     GetScrollPos WINELIB_NAME(GetScrollPos)
BOOL16      WINAPI GetScrollRange16(HWND16,INT16,LPINT16,LPINT16);
BOOL32      WINAPI GetScrollRange32(HWND32,INT32,LPINT32,LPINT32);
#define     GetScrollRange WINELIB_NAME(GetScrollRange)
HWND16      WINAPI GetShellWindow16(void);
HWND32      WINAPI GetShellWindow32(void);
#define     GetShellWindow WINELIB_NAME(GetShellWindow)
HGDIOBJ16   WINAPI GetStockObject16(INT16);
HGDIOBJ32   WINAPI GetStockObject32(INT32);
#define     GetStockObject WINELIB_NAME(GetStockObject)
INT16       WINAPI GetStretchBltMode16(HDC16);
INT32       WINAPI GetStretchBltMode32(HDC32);
#define     GetStretchBltMode WINELIB_NAME(GetStretchBltMode)
BOOL16      WINAPI GetStringType16(LCID,DWORD,LPCSTR,INT16,LPWORD);
BOOL32      WINAPI GetStringType32A(LCID,DWORD,LPCSTR,INT32,LPWORD);
BOOL32      WINAPI GetStringType32W(DWORD,LPCWSTR,INT32,LPWORD);
#define     GetStringType WINELIB_NAME_AW(GetStringType)
HMENU16     WINAPI GetSubMenu16(HMENU16,INT16);
HMENU32     WINAPI GetSubMenu32(HMENU32,INT32);
#define     GetSubMenu WINELIB_NAME(GetSubMenu)
COLORREF    WINAPI GetSysColor16(INT16);
COLORREF    WINAPI GetSysColor32(INT32);
#define     GetSysColor WINELIB_NAME(GetSysColor)
HBRUSH16    WINAPI GetSysColorBrush16(INT16);
HBRUSH32    WINAPI GetSysColorBrush32(INT32);
#define     GetSysColorBrush WINELIB_NAME(GetSysColorBrush)
HWND16      WINAPI GetSysModalWindow16(void);
#define     GetSysModalWindow32() ((HWND32)0)
#define     GetSysModalWindow WINELIB_NAME(GetSysModalWindow)
UINT16      WINAPI GetSystemDirectory16(LPSTR,UINT16);
UINT32      WINAPI GetSystemDirectory32A(LPSTR,UINT32);
UINT32      WINAPI GetSystemDirectory32W(LPWSTR,UINT32);
#define     GetSystemDirectory WINELIB_NAME_AW(GetSystemDirectory)
HMENU16     WINAPI GetSystemMenu16(HWND16,BOOL16);
HMENU32     WINAPI GetSystemMenu32(HWND32,BOOL32);
#define     GetSystemMenu WINELIB_NAME(GetSystemMenu)
INT16       WINAPI GetSystemMetrics16(INT16);
INT32       WINAPI GetSystemMetrics32(INT32);
#define     GetSystemMetrics WINELIB_NAME(GetSystemMetrics)
UINT16      WINAPI GetSystemPaletteEntries16(HDC16,UINT16,UINT16,LPPALETTEENTRY);
UINT32      WINAPI GetSystemPaletteEntries32(HDC32,UINT32,UINT32,LPPALETTEENTRY);
#define     GetSystemPaletteEntries WINELIB_NAME(GetSystemPaletteEntries)
UINT16      WINAPI GetSystemPaletteUse16(HDC16);
UINT32      WINAPI GetSystemPaletteUse32(HDC32);
#define     GetSystemPaletteUse WINELIB_NAME(GetSystemPaletteUse)
DWORD       WINAPI GetTabbedTextExtent16(HDC16,LPCSTR,INT16,INT16,const INT16*);
DWORD       WINAPI GetTabbedTextExtent32A(HDC32,LPCSTR,INT32,INT32,const INT32*);
DWORD       WINAPI GetTabbedTextExtent32W(HDC32,LPCWSTR,INT32,INT32,const INT32*);
#define     GetTabbedTextExtent WINELIB_NAME_AW(GetTabbedTextExtent)
UINT16      WINAPI GetTempFileName16(BYTE,LPCSTR,UINT16,LPSTR);
UINT32      WINAPI GetTempFileName32A(LPCSTR,LPCSTR,UINT32,LPSTR);
UINT32      WINAPI GetTempFileName32W(LPCWSTR,LPCWSTR,UINT32,LPWSTR);
#define     GetTempFileName WINELIB_NAME_AW(GetTempFileName)
UINT32      WINAPI GetTempPath32A(UINT32,LPSTR);
UINT32      WINAPI GetTempPath32W(UINT32,LPWSTR);
#define     GetTempPath WINELIB_NAME_AW(GetTempPath)
UINT16      WINAPI GetTextAlign16(HDC16);
UINT32      WINAPI GetTextAlign32(HDC32);
#define     GetTextAlign WINELIB_NAME(GetTextAlign)
INT16       WINAPI GetTextCharacterExtra16(HDC16);
INT32       WINAPI GetTextCharacterExtra32(HDC32);
#define     GetTextCharacterExtra WINELIB_NAME(GetTextCharacterExtra)
INT16       WINAPI GetTextCharset16(HDC16);
INT32       WINAPI GetTextCharset32(HDC32);
#define     GetTextCharset WINELIB_NAME(GetTextCharset)
COLORREF    WINAPI GetTextColor16(HDC16);
COLORREF    WINAPI GetTextColor32(HDC32);
#define     GetTextColor WINELIB_NAME(GetTextColor)
/* this one is different, because Win32 has *both* 
 * GetTextExtentPoint and GetTextExtentPoint32 !
 */
BOOL16      WINAPI GetTextExtentPoint16(HDC16,LPCSTR,INT16,LPSIZE16);
BOOL32      WINAPI GetTextExtentPoint32A(HDC32,LPCSTR,INT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentPoint32W(HDC32,LPCWSTR,INT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentPoint32ABuggy(HDC32,LPCSTR,INT32,LPSIZE32);
BOOL32      WINAPI GetTextExtentPoint32WBuggy(HDC32,LPCWSTR,INT32,LPSIZE32);
#ifdef UNICODE
#define     GetTextExtentPoint GetTextExtentPoint32WBuggy
#define     GetTextExtentPoint32 GetTextExtentPoint32W
#else
#define     GetTextExtentPoint GetTextExtentPoint32ABuggy
#define     GetTextExtentPoint32 GetTextExtentPoint32A
#endif
INT16       WINAPI GetTextFace16(HDC16,INT16,LPSTR);
INT32       WINAPI GetTextFace32A(HDC32,INT32,LPSTR);
INT32       WINAPI GetTextFace32W(HDC32,INT32,LPWSTR);
#define     GetTextFace WINELIB_NAME_AW(GetTextFace)
BOOL16      WINAPI GetTextMetrics16(HDC16,LPTEXTMETRIC16);
BOOL32      WINAPI GetTextMetrics32A(HDC32,LPTEXTMETRIC32A);
BOOL32      WINAPI GetTextMetrics32W(HDC32,LPTEXTMETRIC32W);
#define     GetTextMetrics WINELIB_NAME_AW(GetTextMetrics)
LPINT16     WINAPI GetThresholdEvent16(void);
LPDWORD     WINAPI GetThresholdEvent32(void);
#define     GetThresholdEvent WINELIB_NAME(GetThresholdEvent)
INT16       WINAPI GetThresholdStatus16(void);
DWORD       WINAPI GetThresholdStatus32(void);
#define     GetThresholdStatus WINELIB_NAME(GetThresholdStatus)
HWND16      WINAPI GetTopWindow16(HWND16);
HWND32      WINAPI GetTopWindow32(HWND32);
#define     GetTopWindow WINELIB_NAME(GetTopWindow)
BOOL16      WINAPI GetUpdateRect16(HWND16,LPRECT16,BOOL16);
BOOL32      WINAPI GetUpdateRect32(HWND32,LPRECT32,BOOL32);
#define     GetUpdateRect WINELIB_NAME(GetUpdateRect)
INT16       WINAPI GetUpdateRgn16(HWND16,HRGN16,BOOL16);
INT32       WINAPI GetUpdateRgn32(HWND32,HRGN32,BOOL32);
#define     GetUpdateRgn WINELIB_NAME(GetUpdateRgn)
LONG        WINAPI GetVersion16(void);
LONG        WINAPI GetVersion32(void);
#define     GetVersion WINELIB_NAME(GetVersion)
BOOL16      WINAPI GetViewportExtEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetViewportExtEx32(HDC32,LPPOINT32);
#define     GetViewportExtEx WINELIB_NAME(GetViewportExtEx)
BOOL16      WINAPI GetViewportOrgEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetViewportOrgEx32(HDC32,LPPOINT32);
#define     GetViewportOrgEx WINELIB_NAME(GetViewportOrgEx)
BOOL32      WINAPI GetVolumeInformation32A(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
BOOL32      WINAPI GetVolumeInformation32W(LPCWSTR,LPWSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPWSTR,DWORD);
#define     GetVolumeInformation WINELIB_NAME_AW(GetVolumeInformation)
HWND16      WINAPI GetWindow16(HWND16,WORD);
HWND32      WINAPI GetWindow32(HWND32,WORD);
#define     GetWindow WINELIB_NAME(GetWindow)
HDC16       WINAPI GetWindowDC16(HWND16);
HDC32       WINAPI GetWindowDC32(HWND32);
#define     GetWindowDC WINELIB_NAME(GetWindowDC)
BOOL16      WINAPI GetWindowExtEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetWindowExtEx32(HDC32,LPPOINT32);
#define     GetWindowExtEx WINELIB_NAME(GetWindowExtEx)
LONG        WINAPI GetWindowLong16(HWND16,INT16);
LONG        WINAPI GetWindowLong32A(HWND32,INT32);
LONG        WINAPI GetWindowLong32W(HWND32,INT32);
#define     GetWindowLong WINELIB_NAME_AW(GetWindowLong)
BOOL16      WINAPI GetWindowOrgEx16(HDC16,LPPOINT16);
BOOL32      WINAPI GetWindowOrgEx32(HDC32,LPPOINT32);
#define     GetWindowOrgEx WINELIB_NAME(GetWindowOrgEx)
BOOL16      WINAPI GetWindowPlacement16(HWND16,LPWINDOWPLACEMENT16);
BOOL32      WINAPI GetWindowPlacement32(HWND32,LPWINDOWPLACEMENT32);
#define     GetWindowPlacement WINELIB_NAME(GetWindowPlacement)
void        WINAPI GetWindowRect16(HWND16,LPRECT16);
void        WINAPI GetWindowRect32(HWND32,LPRECT32);
#define     GetWindowRect WINELIB_NAME(GetWindowRect)
UINT16      WINAPI GetWindowsDirectory16(LPSTR,UINT16);
UINT32      WINAPI GetWindowsDirectory32A(LPSTR,UINT32);
UINT32      WINAPI GetWindowsDirectory32W(LPWSTR,UINT32);
#define     GetWindowsDirectory WINELIB_NAME_AW(GetWindowsDirectory)
HTASK16     WINAPI GetWindowTask16(HWND16);
#define     GetWindowTask32(hwnd) ((HTASK32)GetWindowThreadProcessId(hwnd,NULL))
#define     GetWindowTask WINELIB_NAME(GetWindowTask)
INT16       WINAPI GetWindowText16(HWND16,SEGPTR,INT16);
INT32       WINAPI GetWindowText32A(HWND32,LPSTR,INT32);
INT32       WINAPI GetWindowText32W(HWND32,LPWSTR,INT32);
#define     GetWindowText WINELIB_NAME_AW(GetWindowText)
INT16       WINAPI GetWindowTextLength16(HWND16);
INT32       WINAPI GetWindowTextLength32A(HWND32);
INT32       WINAPI GetWindowTextLength32W(HWND32);
#define     GetWindowTextLength WINELIB_NAME_AW(GetWindowTextLength)
WORD        WINAPI GetWindowWord16(HWND16,INT16);
WORD        WINAPI GetWindowWord32(HWND32,INT32);
#define     GetWindowWord WINELIB_NAME(GetWindowWord)
ATOM        WINAPI GlobalAddAtom16(SEGPTR);
ATOM        WINAPI GlobalAddAtom32A(LPCSTR);
ATOM        WINAPI GlobalAddAtom32W(LPCWSTR);
#define     GlobalAddAtom WINELIB_NAME_AW(GlobalAddAtom)
HGLOBAL16   WINAPI GlobalAlloc16(UINT16,DWORD);
HGLOBAL32   WINAPI GlobalAlloc32(UINT32,DWORD);
#define     GlobalAlloc WINELIB_NAME(GlobalAlloc)
DWORD       WINAPI GlobalCompact16(DWORD);
DWORD       WINAPI GlobalCompact32(DWORD);
#define     GlobalCompact WINELIB_NAME(GlobalCompact)
UINT16      WINAPI GlobalFlags16(HGLOBAL16);
UINT32      WINAPI GlobalFlags32(HGLOBAL32);
#define     GlobalFlags WINELIB_NAME(GlobalFlags)
ATOM        WINAPI GlobalFindAtom16(SEGPTR);
ATOM        WINAPI GlobalFindAtom32A(LPCSTR);
ATOM        WINAPI GlobalFindAtom32W(LPCWSTR);
#define     GlobalFindAtom WINELIB_NAME_AW(GlobalFindAtom)
HGLOBAL16   WINAPI GlobalFree16(HGLOBAL16);
HGLOBAL32   WINAPI GlobalFree32(HGLOBAL32);
#define     GlobalFree WINELIB_NAME(GlobalFree)
UINT16      WINAPI GlobalGetAtomName16(ATOM,LPSTR,INT16);
UINT32      WINAPI GlobalGetAtomName32A(ATOM,LPSTR,INT32);
UINT32      WINAPI GlobalGetAtomName32W(ATOM,LPWSTR,INT32);
#define     GlobalGetAtomName WINELIB_NAME_AW(GlobalGetAtomName)
DWORD       WINAPI GlobalHandle16(WORD);
HGLOBAL32   WINAPI GlobalHandle32(LPCVOID);
#define     GlobalHandle WINELIB_NAME(GlobalHandle)
VOID        WINAPI GlobalFix16(HGLOBAL16);
VOID        WINAPI GlobalFix32(HGLOBAL32);
#define     GlobalFix WINELIB_NAME(GlobalFix)
LPVOID      WINAPI GlobalLock16(HGLOBAL16);
LPVOID      WINAPI GlobalLock32(HGLOBAL32);
#define     GlobalLock WINELIB_NAME(GlobalLock)
HGLOBAL16   WINAPI GlobalReAlloc16(HGLOBAL16,DWORD,UINT16);
HGLOBAL32   WINAPI GlobalReAlloc32(HGLOBAL32,DWORD,UINT32);
#define     GlobalReAlloc WINELIB_NAME(GlobalReAlloc)
DWORD       WINAPI GlobalSize16(HGLOBAL16);
DWORD       WINAPI GlobalSize32(HGLOBAL32);
#define     GlobalSize WINELIB_NAME(GlobalSize)
VOID        WINAPI GlobalUnfix16(HGLOBAL16);
VOID        WINAPI GlobalUnfix32(HGLOBAL32);
#define     GlobalUnfix WINELIB_NAME(GlobalUnfix)
BOOL16      WINAPI GlobalUnlock16(HGLOBAL16);
BOOL32      WINAPI GlobalUnlock32(HGLOBAL32);
#define     GlobalUnlock WINELIB_NAME(GlobalUnlock)
BOOL16      WINAPI GlobalUnWire16(HGLOBAL16);
BOOL32      WINAPI GlobalUnWire32(HGLOBAL32);
#define     GlobalUnWire WINELIB_NAME(GlobalUnWire)
SEGPTR      WINAPI GlobalWire16(HGLOBAL16);
LPVOID      WINAPI GlobalWire32(HGLOBAL32);
#define     GlobalWire WINELIB_NAME(GlobalWire)
BOOL16      WINAPI GrayString16(HDC16,HBRUSH16,GRAYSTRINGPROC16,LPARAM,
                                INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI GrayString32A(HDC32,HBRUSH32,GRAYSTRINGPROC32,LPARAM,
                                 INT32,INT32,INT32,INT32,INT32);
BOOL32      WINAPI GrayString32W(HDC32,HBRUSH32,GRAYSTRINGPROC32,LPARAM,
                                 INT32,INT32,INT32,INT32,INT32);
#define     GrayString WINELIB_NAME_AW(GrayString)
VOID        WINAPI HideCaret16(HWND16);
BOOL32      WINAPI HideCaret32(HWND32);
#define     HideCaret WINELIB_NAME(HideCaret)
BOOL16      WINAPI HiliteMenuItem16(HWND16,HMENU16,UINT16,UINT16);
BOOL32      WINAPI HiliteMenuItem32(HWND32,HMENU32,UINT32,UINT32);
#define     HiliteMenuItem WINELIB_NAME(HiliteMenuItem)
void        WINAPI InflateRect16(LPRECT16,INT16,INT16);
void        WINAPI InflateRect32(LPRECT32,INT32,INT32);
#define     InflateRect WINELIB_NAME(InflateRect)
WORD        WINAPI InitAtomTable16(WORD);
BOOL32      WINAPI InitAtomTable32(DWORD);
#define     InitAtomTable WINELIB_NAME(InitAtomTable)
BOOL16      WINAPI InSendMessage16(void);
BOOL32      WINAPI InSendMessage32(void);
#define     InSendMessage WINELIB_NAME(InSendMessage)
BOOL16      WINAPI InsertMenu16(HMENU16,UINT16,UINT16,UINT16,SEGPTR);
BOOL32      WINAPI InsertMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI InsertMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define     InsertMenu WINELIB_NAME_AW(InsertMenu)
BOOL16      WINAPI InsertMenuItem16(HMENU16,UINT16,BOOL16,const MENUITEMINFO16*);
BOOL32      WINAPI InsertMenuItem32A(HMENU32,UINT32,BOOL32,const MENUITEMINFO32A*);
BOOL32      WINAPI InsertMenuItem32W(HMENU32,UINT32,BOOL32,const MENUITEMINFO32W*);
#define     InsertMenuItem WINELIB_NAME_AW(InsertMenuItem)
INT16       WINAPI IntersectClipRect16(HDC16,INT16,INT16,INT16,INT16);
INT32       WINAPI IntersectClipRect32(HDC32,INT32,INT32,INT32,INT32);
#define     IntersectClipRect WINELIB_NAME(IntersectClipRect)
BOOL16      WINAPI IntersectRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL32      WINAPI IntersectRect32(LPRECT32,const RECT32*,const RECT32*);
#define     IntersectRect WINELIB_NAME(IntersectRect)
void        WINAPI InvalidateRect16(HWND16,const RECT16*,BOOL16);
void        WINAPI InvalidateRect32(HWND32,const RECT32*,BOOL32);
#define     InvalidateRect WINELIB_NAME(InvalidateRect)
void        WINAPI InvalidateRgn16(HWND16,HRGN16,BOOL16);
void        WINAPI InvalidateRgn32(HWND32,HRGN32,BOOL32);
#define     InvalidateRgn WINELIB_NAME(InvalidateRgn)
void        WINAPI InvertRect16(HDC16,const RECT16*);
void        WINAPI InvertRect32(HDC32,const RECT32*);
#define     InvertRect WINELIB_NAME(InvertRect)
BOOL16      WINAPI InvertRgn16(HDC16,HRGN16);
BOOL32      WINAPI InvertRgn32(HDC32,HRGN32);
#define     InvertRgn WINELIB_NAME(InvertRgn)
BOOL16      WINAPI IsBadCodePtr16(SEGPTR);
BOOL32      WINAPI IsBadCodePtr32(FARPROC32);
#define     IsBadCodePtr WINELIB_NAME(IsBadCodePtr)
BOOL16      WINAPI IsBadHugeReadPtr16(SEGPTR,DWORD);
BOOL32      WINAPI IsBadHugeReadPtr32(LPCVOID,UINT32);
#define     IsBadHugeReadPtr WINELIB_NAME(IsBadHugeReadPtr)
BOOL16      WINAPI IsBadHugeWritePtr16(SEGPTR,DWORD);
BOOL32      WINAPI IsBadHugeWritePtr32(LPVOID,UINT32);
#define     IsBadHugeWritePtr WINELIB_NAME(IsBadHugeWritePtr)
BOOL16      WINAPI IsBadReadPtr16(SEGPTR,UINT16);
BOOL32      WINAPI IsBadReadPtr32(LPCVOID,UINT32);
#define     IsBadReadPtr WINELIB_NAME(IsBadReadPtr)
BOOL16      WINAPI IsBadStringPtr16(SEGPTR,UINT16);
BOOL32      WINAPI IsBadStringPtr32A(LPCSTR,UINT32);
BOOL32      WINAPI IsBadStringPtr32W(LPCWSTR,UINT32);
#define     IsBadStringPtr WINELIB_NAME_AW(IsBadStringPtr)
BOOL16      WINAPI IsBadWritePtr16(SEGPTR,UINT16);
BOOL32      WINAPI IsBadWritePtr32(LPVOID,UINT32);
#define     IsBadWritePtr WINELIB_NAME(IsBadWritePtr)
BOOL16      WINAPI IsCharAlpha16(CHAR);
BOOL32      WINAPI IsCharAlpha32A(CHAR);
BOOL32      WINAPI IsCharAlpha32W(WCHAR);
#define     IsCharAlpha WINELIB_NAME_AW(IsCharAlpha)
BOOL16      WINAPI IsCharAlphaNumeric16(CHAR);
BOOL32      WINAPI IsCharAlphaNumeric32A(CHAR);
BOOL32      WINAPI IsCharAlphaNumeric32W(WCHAR);
#define     IsCharAlphaNumeric WINELIB_NAME_AW(IsCharAlphaNumeric)
BOOL16      WINAPI IsCharLower16(CHAR);
BOOL32      WINAPI IsCharLower32A(CHAR);
BOOL32      WINAPI IsCharLower32W(WCHAR);
#define     IsCharLower WINELIB_NAME_AW(IsCharLower)
BOOL16      WINAPI IsCharUpper16(CHAR);
BOOL32      WINAPI IsCharUpper32A(CHAR);
BOOL32      WINAPI IsCharUpper32W(WCHAR);
#define     IsCharUpper WINELIB_NAME_AW(IsCharUpper)
BOOL16      WINAPI IsChild16(HWND16,HWND16);
BOOL32      WINAPI IsChild32(HWND32,HWND32);
#define     IsChild WINELIB_NAME(IsChild)
BOOL16      WINAPI IsClipboardFormatAvailable16(UINT16);
BOOL32      WINAPI IsClipboardFormatAvailable32(UINT32);
#define     IsClipboardFormatAvailable WINELIB_NAME(IsClipboardFormatAvailable)
BOOL16      WINAPI IsDBCSLeadByte16(BYTE);
BOOL32      WINAPI IsDBCSLeadByte32(BYTE);
#define     IsDBCSLeadByte WINELIB_NAME(IsDBCSLeadByte)
BOOL16      WINAPI IsDialogMessage16(HWND16,LPMSG16);
BOOL32      WINAPI IsDialogMessage32A(HWND32,LPMSG32);
BOOL32      WINAPI IsDialogMessage32W(HWND32,LPMSG32);
#define     IsDialogMessage WINELIB_NAME(IsDialogMessage)
UINT16      WINAPI IsDlgButtonChecked16(HWND16,UINT16);
UINT32      WINAPI IsDlgButtonChecked32(HWND32,UINT32);
#define     IsDlgButtonChecked WINELIB_NAME(IsDlgButtonChecked)
BOOL16      WINAPI IsIconic16(HWND16);
BOOL32      WINAPI IsIconic32(HWND32);
#define     IsIconic WINELIB_NAME(IsIconic)
BOOL16      WINAPI IsMenu16(HMENU16);
BOOL32      WINAPI IsMenu32(HMENU32);
#define     IsMenu WINELIB_NAME(IsMenu)
BOOL16      WINAPI IsRectEmpty16(const RECT16*);
BOOL32      WINAPI IsRectEmpty32(const RECT32*);
#define     IsRectEmpty WINELIB_NAME(IsRectEmpty)
BOOL16      WINAPI IsWindow16(HWND16);
BOOL32      WINAPI IsWindow32(HWND32);
#define     IsWindow WINELIB_NAME(IsWindow)
BOOL16      WINAPI IsWindowEnabled16(HWND16);
BOOL32      WINAPI IsWindowEnabled32(HWND32);
#define     IsWindowEnabled WINELIB_NAME(IsWindowEnabled)
BOOL16      WINAPI IsWindowVisible16(HWND16);
BOOL32      WINAPI IsWindowVisible32(HWND32);
#define     IsWindowVisible WINELIB_NAME(IsWindowVisible)
BOOL16      WINAPI IsZoomed16(HWND16);
BOOL32      WINAPI IsZoomed32(HWND32);
#define     IsZoomed WINELIB_NAME(IsZoomed)
BOOL16      WINAPI KillSystemTimer16(HWND16,UINT16);
BOOL32      WINAPI KillSystemTimer32(HWND32,UINT32);
#define     KillSystemTimer WINELIB_NAME(KillSystemTimer)
BOOL16      WINAPI KillTimer16(HWND16,UINT16);
BOOL32      WINAPI KillTimer32(HWND32,UINT32);
#define     KillTimer WINELIB_NAME(KillTimer)
HFILE16     WINAPI LZOpenFile16(LPCSTR,LPOFSTRUCT,UINT16);
HFILE32     WINAPI LZOpenFile32A(LPCSTR,LPOFSTRUCT,UINT32);
HFILE32     WINAPI LZOpenFile32W(LPCWSTR,LPOFSTRUCT,UINT32);
#define     LZOpenFile WINELIB_NAME_AW(LZOpenFile)
INT16       WINAPI LZRead16(HFILE16,LPVOID,UINT16); 
INT32       WINAPI LZRead32(HFILE32,LPVOID,UINT32); 
#define     LZRead WINELIB_NAME(LZRead)
INT16       WINAPI LZStart16(void);
INT32       WINAPI LZStart32(void);
#define     LZStart WINELIB_NAME(LZStart)
VOID        WINAPI LineDDA16(INT16,INT16,INT16,INT16,LINEDDAPROC16,LPARAM);
BOOL32      WINAPI LineDDA32(INT32,INT32,INT32,INT32,LINEDDAPROC32,LPARAM);
#define     LineDDA WINELIB_NAME(LineDDA)
BOOL16      WINAPI LineTo16(HDC16,INT16,INT16);
BOOL32      WINAPI LineTo32(HDC32,INT32,INT32);
#define     LineTo WINELIB_NAME(LineTo)
HACCEL16    WINAPI LoadAccelerators16(HINSTANCE16,SEGPTR);
HACCEL32    WINAPI LoadAccelerators32A(HINSTANCE32,LPCSTR);
HACCEL32    WINAPI LoadAccelerators32W(HINSTANCE32,LPCWSTR);
#define     LoadAccelerators WINELIB_NAME_AW(LoadAccelerators)
HBITMAP16   WINAPI LoadBitmap16(HANDLE16,SEGPTR);
HBITMAP32   WINAPI LoadBitmap32A(HANDLE32,LPCSTR);
HBITMAP32   WINAPI LoadBitmap32W(HANDLE32,LPCWSTR);
#define     LoadBitmap WINELIB_NAME(LoadBitmap)
HCURSOR16   WINAPI LoadCursor16(HINSTANCE16,SEGPTR);
HCURSOR32   WINAPI LoadCursor32A(HINSTANCE32,LPCSTR);
HCURSOR32   WINAPI LoadCursor32W(HINSTANCE32,LPCWSTR);
#define     LoadCursor WINELIB_NAME_AW(LoadCursor)
HICON16     WINAPI LoadIcon16(HINSTANCE16,SEGPTR);
HICON32     WINAPI LoadIcon32A(HINSTANCE32,LPCSTR);
HICON32     WINAPI LoadIcon32W(HINSTANCE32,LPCWSTR);
#define     LoadIcon WINELIB_NAME_AW(LoadIcon)
HANDLE16    WINAPI LoadImage16(HINSTANCE16,LPCSTR,UINT16,INT16,INT16,UINT16);
HANDLE32    WINAPI LoadImage32A(HINSTANCE32,LPCSTR,UINT32,INT32,INT32,UINT32);
HANDLE32    WINAPI LoadImage32W(HINSTANCE32,LPCWSTR,UINT32,INT32,INT32,UINT32);
#define     LoadImage WINELIB_NAME_AW(LoadImage)
HINSTANCE16 WINAPI LoadLibrary16(LPCSTR);
HMODULE32   WINAPI LoadLibrary32A(LPCSTR);
HMODULE32   WINAPI LoadLibrary32W(LPCWSTR);
#define     LoadLibrary WINELIB_NAME_AW(LoadLibrary)
HMODULE32   WINAPI LoadLibraryEx32A(LPCSTR,HFILE32,DWORD);
HMODULE32   WINAPI LoadLibraryEx32W(LPCWSTR,HFILE32,DWORD);
#define     LoadLibraryEx WINELIB_NAME_AW(LoadLibraryEx)
HMENU16     WINAPI LoadMenu16(HINSTANCE16,SEGPTR);
HMENU32     WINAPI LoadMenu32A(HINSTANCE32,LPCSTR);
HMENU32     WINAPI LoadMenu32W(HINSTANCE32,LPCWSTR);
#define     LoadMenu WINELIB_NAME_AW(LoadMenu)
HMENU16     WINAPI LoadMenuIndirect16(LPCVOID);
HMENU32     WINAPI LoadMenuIndirect32A(LPCVOID);
HMENU32     WINAPI LoadMenuIndirect32W(LPCVOID);
#define     LoadMenuIndirect WINELIB_NAME_AW(LoadMenuIndirect)
HINSTANCE16 WINAPI LoadModule16(LPCSTR,LPVOID);
DWORD       WINAPI LoadModule32(LPCSTR,LPVOID);
#define     LoadModule WINELIB_NAME(LoadModule)
HGLOBAL16   WINAPI LoadResource16(HINSTANCE16,HRSRC16);
HGLOBAL32   WINAPI LoadResource32(HMODULE32,HRSRC32);
#define     LoadResource WINELIB_NAME(LoadResource)
INT16       WINAPI LoadString16(HINSTANCE16,UINT16,LPSTR,INT16);
INT32       WINAPI LoadString32A(HINSTANCE32,UINT32,LPSTR,INT32);
INT32       WINAPI LoadString32W(HINSTANCE32,UINT32,LPWSTR,INT32);
#define     LoadString WINELIB_NAME_AW(LoadString)
HLOCAL16    WINAPI LocalAlloc16(UINT16,WORD);
HLOCAL32    WINAPI LocalAlloc32(UINT32,DWORD);
#define     LocalAlloc WINELIB_NAME(LocalAlloc)
UINT16      WINAPI LocalCompact16(UINT16);
UINT32      WINAPI LocalCompact32(UINT32);
#define     LocalCompact WINELIB_NAME(LocalCompact)
UINT16      WINAPI LocalFlags16(HLOCAL16);
UINT32      WINAPI LocalFlags32(HLOCAL32);
#define     LocalFlags WINELIB_NAME(LocalFlags)
HLOCAL16    WINAPI LocalFree16(HLOCAL16);
HLOCAL32    WINAPI LocalFree32(HLOCAL32);
#define     LocalFree WINELIB_NAME(LocalFree)
HLOCAL16    WINAPI LocalHandle16(WORD);
HLOCAL32    WINAPI LocalHandle32(LPCVOID);
#define     LocalHandle WINELIB_NAME(LocalHandle)
SEGPTR      WINAPI LocalLock16(HLOCAL16);
LPVOID      WINAPI LocalLock32(HLOCAL32);
#define     LocalLock WINELIB_NAME(LocalLock)
HLOCAL16    WINAPI LocalReAlloc16(HLOCAL16,WORD,UINT16);
HLOCAL32    WINAPI LocalReAlloc32(HLOCAL32,DWORD,UINT32);
#define     LocalReAlloc WINELIB_NAME(LocalReAlloc)
UINT16      WINAPI LocalShrink16(HGLOBAL16,UINT16);
UINT32      WINAPI LocalShrink32(HGLOBAL32,UINT32);
#define     LocalShrink WINELIB_NAME(LocalShrink)
UINT16      WINAPI LocalSize16(HLOCAL16);
UINT32      WINAPI LocalSize32(HLOCAL32);
#define     LocalSize WINELIB_NAME(LocalSize)
BOOL16      WINAPI LocalUnlock16(HLOCAL16);
BOOL32      WINAPI LocalUnlock32(HLOCAL32);
#define     LocalUnlock WINELIB_NAME(LocalUnlock)
LPVOID      WINAPI LockResource16(HGLOBAL16);
LPVOID      WINAPI LockResource32(HGLOBAL32);
#define     LockResource WINELIB_NAME(LockResource)
HGLOBAL16   WINAPI LockSegment16(HGLOBAL16);
#define     LockSegment32(handle) GlobalFix32((HANDLE32)(handle))
#define     LockSegment WINELIB_NAME(LockSegment)
BOOL16      WINAPI LockWindowUpdate16(HWND16);
BOOL32      WINAPI LockWindowUpdate32(HWND32);
#define     LockWindowUpdate WINELIB_NAME(LockWindowUpdate)
INT16       WINAPI LookupIconIdFromDirectory16(LPBYTE,BOOL16);
INT32       WINAPI LookupIconIdFromDirectory32(LPBYTE,BOOL32);
#define     LookupIconIdFromDirectory WINELIB_NAME(LookupIconIdFromDirectory)
INT16       WINAPI LookupIconIdFromDirectoryEx16(LPBYTE,BOOL16,INT16,INT16,UINT16);
INT32       WINAPI LookupIconIdFromDirectoryEx32(LPBYTE,BOOL32,INT32,INT32,UINT32);
#define     LookupIconIdFromDirectoryEx WINELIB_NAME(LookupIconIdFromDirectoryEx)
BOOL16      WINAPI LPtoDP16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI LPtoDP32(HDC32,LPPOINT32,INT32);
#define     LPtoDP WINELIB_NAME(LPtoDP)
void        WINAPI LZClose16(HFILE16);
void        WINAPI LZClose32(HFILE32);
#define     LZClose WINELIB_NAME(LZClose)
LONG        WINAPI LZCopy16(HFILE16,HFILE16);
LONG        WINAPI LZCopy32(HFILE32,HFILE32);
#define     LZCopy WINELIB_NAME(LZCopy)
HFILE16     WINAPI LZInit16(HFILE16);
HFILE32     WINAPI LZInit32(HFILE32);
#define     LZInit WINELIB_NAME(LZInit)
LONG        WINAPI LZSeek16(HFILE16,LONG,INT16);
LONG        WINAPI LZSeek32(HFILE32,LONG,INT32);
#define     LZSeek WINELIB_NAME(LZSeek)
UINT16      WINAPI MapVirtualKey16(UINT16,UINT16);
UINT32      WINAPI MapVirtualKey32A(UINT32,UINT32);
UINT32      WINAPI MapVirtualKey32W(UINT32,UINT32);
#define     MapVirtualKey WINELIB_NAME_AW(MapVirtualKey)
FARPROC16   WINAPI MakeProcInstance16(FARPROC16,HANDLE16);
#define     MakeProcInstance32(proc,inst) (proc)
#define     MakeProcInstance WINELIB_NAME(MakeProcInstance)
void        WINAPI MapDialogRect16(HWND16,LPRECT16);
void        WINAPI MapDialogRect32(HWND32,LPRECT32);
#define     MapDialogRect WINELIB_NAME(MapDialogRect)
void        WINAPI MapWindowPoints16(HWND16,HWND16,LPPOINT16,UINT16);
void        WINAPI MapWindowPoints32(HWND32,HWND32,LPPOINT32,UINT32);
#define     MapWindowPoints WINELIB_NAME(MapWindowPoints)
VOID        WINAPI MessageBeep16(UINT16);
BOOL32      WINAPI MessageBeep32(UINT32);
#define     MessageBeep WINELIB_NAME(MessageBeep)
INT16       WINAPI MessageBox16(HWND16,LPCSTR,LPCSTR,UINT16);
INT32       WINAPI MessageBox32A(HWND32,LPCSTR,LPCSTR,UINT32);
INT32       WINAPI MessageBox32W(HWND32,LPCWSTR,LPCWSTR,UINT32);
#define     MessageBox WINELIB_NAME_AW(MessageBox)
INT16       WINAPI MessageBoxIndirect16(LPMSGBOXPARAMS16);
INT32       WINAPI MessageBoxIndirect32A(LPMSGBOXPARAMS32A);
INT32       WINAPI MessageBoxIndirect32W(LPMSGBOXPARAMS32W);
#define     MessageBoxIndirect WINELIB_NAME_AW(MessageBoxIndirect)
BOOL16      WINAPI ModifyMenu16(HMENU16,UINT16,UINT16,UINT16,SEGPTR);
BOOL32      WINAPI ModifyMenu32A(HMENU32,UINT32,UINT32,UINT32,LPCSTR);
BOOL32      WINAPI ModifyMenu32W(HMENU32,UINT32,UINT32,UINT32,LPCWSTR);
#define     ModifyMenu WINELIB_NAME_AW(ModifyMenu)
BOOL16      WINAPI MoveToEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI MoveToEx32(HDC32,INT32,INT32,LPPOINT32);
#define     MoveToEx WINELIB_NAME(MoveToEx)
BOOL16      WINAPI MoveWindow16(HWND16,INT16,INT16,INT16,INT16,BOOL16);
BOOL32      WINAPI MoveWindow32(HWND32,INT32,INT32,INT32,INT32,BOOL32);
#define     MoveWindow WINELIB_NAME(MoveWindow)
INT16       WINAPI MulDiv16(INT16,INT16,INT16);
INT32       WINAPI MulDiv32(INT32,INT32,INT32);
#define     MulDiv WINELIB_NAME(MulDiv)
INT16       WINAPI OemToAnsi16(LPCSTR,LPSTR);
#define     OemToAnsi32A OemToChar32A
#define     OemToAnsi32W OemToChar32W
#define     OemToAnsi WINELIB_NAME_AW(OemToAnsi)
VOID        WINAPI OemToAnsiBuff16(LPCSTR,LPSTR,UINT16);
#define     OemToAnsiBuff32A OemToCharBuff32A
#define     OemToAnsiBuff32W OemToCharBuff32W
#define     OemToAnsiBuff WINELIB_NAME_AW(OemToAnsiBuff)
BOOL32      WINAPI OemToChar32A(LPCSTR,LPSTR);
BOOL32      WINAPI OemToChar32W(LPCSTR,LPWSTR);
#define     OemToChar WINELIB_NAME_AW(OemToChar)
BOOL32      WINAPI OemToCharBuff32A(LPCSTR,LPSTR,DWORD);
BOOL32      WINAPI OemToCharBuff32W(LPCSTR,LPWSTR,DWORD);
#define     OemToCharBuff WINELIB_NAME_AW(OemToCharBuff)
INT16       WINAPI OffsetClipRgn16(HDC16,INT16,INT16);
INT32       WINAPI OffsetClipRgn32(HDC32,INT32,INT32);
#define     OffsetClipRgn WINELIB_NAME(OffsetClipRgn)
void        WINAPI OffsetRect16(LPRECT16,INT16,INT16);
void        WINAPI OffsetRect32(LPRECT32,INT32,INT32);
#define     OffsetRect WINELIB_NAME(OffsetRect)
INT16       WINAPI OffsetRgn16(HRGN16,INT16,INT16);
INT32       WINAPI OffsetRgn32(HRGN32,INT32,INT32);
#define     OffsetRgn WINELIB_NAME(OffsetRgn)
BOOL16      WINAPI OffsetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI OffsetViewportOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     OffsetViewportOrgEx WINELIB_NAME(OffsetViewportOrgEx)
BOOL16      WINAPI OffsetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI OffsetWindowOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     OffsetWindowOrgEx WINELIB_NAME(OffsetWindowOrgEx)
BOOL16      WINAPI OpenClipboard16(HWND16);
BOOL32      WINAPI OpenClipboard32(HWND32);
#define     OpenClipboard WINELIB_NAME(OpenClipboard)
HFILE16     WINAPI OpenFile16(LPCSTR,OFSTRUCT*,UINT16);
HFILE32     WINAPI OpenFile32(LPCSTR,OFSTRUCT*,UINT32);
#define     OpenFile WINELIB_NAME(OpenFile)
BOOL16      WINAPI OpenIcon16(HWND16);
BOOL32      WINAPI OpenIcon32(HWND32);
#define     OpenIcon WINELIB_NAME(OpenIcon)
INT16       WINAPI OpenSound16(void);
VOID        WINAPI OpenSound32(void);
#define     OpenSound WINELIB_NAME(OpenSound)
VOID        WINAPI OutputDebugString16(LPCSTR);
VOID        WINAPI OutputDebugString32A(LPCSTR);
VOID        WINAPI OutputDebugString32W(LPCWSTR);
#define     OutputDebugString WINELIB_NAME_AW(OutputDebugString)
BOOL16      WINAPI PaintRgn16(HDC16,HRGN16);
BOOL32      WINAPI PaintRgn32(HDC32,HRGN32);
#define     PaintRgn WINELIB_NAME(PaintRgn)
BOOL16      WINAPI PatBlt16(HDC16,INT16,INT16,INT16,INT16,DWORD);
BOOL32      WINAPI PatBlt32(HDC32,INT32,INT32,INT32,INT32,DWORD);
#define     PatBlt WINELIB_NAME(PatBlt)
HRGN16      WINAPI PathToRegion16(HDC16);
HRGN32      WINAPI PathToRegion32(HDC32);
#define     PathToRegion WINELIB_NAME(PathToRegion)
BOOL16      WINAPI PeekMessage16(LPMSG16,HWND16,UINT16,UINT16,UINT16);
BOOL32      WINAPI PeekMessage32A(LPMSG32,HWND32,UINT32,UINT32,UINT32);
BOOL32      WINAPI PeekMessage32W(LPMSG32,HWND32,UINT32,UINT32,UINT32);
#define     PeekMessage WINELIB_NAME_AW(PeekMessage)
BOOL16      WINAPI Pie16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Pie32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     Pie WINELIB_NAME(Pie)
BOOL16      WINAPI PlayMetaFile16(HDC16,HMETAFILE16);
BOOL32      WINAPI PlayMetaFile32(HDC32,HMETAFILE32);
#define     PlayMetaFile WINELIB_NAME(PlayMetaFile)
VOID        WINAPI PlayMetaFileRecord16(HDC16,LPHANDLETABLE16,LPMETARECORD,UINT16);
BOOL32      WINAPI PlayMetaFileRecord32(HDC32,LPHANDLETABLE32,LPMETARECORD,UINT32);
#define     PlayMetaFileRecord WINELIB_NAME(PlayMetaFileRecord)
BOOL16      WINAPI PolyBezier16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI PolyBezier32(HDC32,LPPOINT32,DWORD);
#define     PolyBezier WINELIB_NAME(PolyBezier)
BOOL16      WINAPI PolyPolygon16(HDC16,LPPOINT16,LPINT16,UINT16);
BOOL32      WINAPI PolyPolygon32(HDC32,LPPOINT32,LPINT32,UINT32);
#define     PolyPolygon WINELIB_NAME(PolyPolygon)
BOOL16      WINAPI Polygon16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI Polygon32(HDC32,LPPOINT32,INT32);
#define     Polygon WINELIB_NAME(Polygon)
BOOL16      WINAPI Polyline16(HDC16,LPPOINT16,INT16);
BOOL32      WINAPI Polyline32(HDC32,LPPOINT32,INT32);
#define     Polyline WINELIB_NAME(Polyline)
BOOL16      WINAPI PostAppMessage16(HTASK16,UINT16,WPARAM16,LPARAM);
#define     PostAppMessage32A(thread,msg,wparam,lparam) \
            PostThreadMessage32A((DWORD)(thread),msg,wparam,lparam)
#define     PostAppMessage32W(thread,msg,wparam,lparam) \
            PostThreadMessage32W((DWORD)(thread),msg,wparam,lparam)
#define     PostAppMessage WINELIB_NAME_AW(PostAppMessage)
BOOL16      WINAPI PostMessage16(HWND16,UINT16,WPARAM16,LPARAM);
BOOL32      WINAPI PostMessage32A(HWND32,UINT32,WPARAM32,LPARAM);
BOOL32      WINAPI PostMessage32W(HWND32,UINT32,WPARAM32,LPARAM);
#define     PostMessage WINELIB_NAME_AW(PostMessage)
void        WINAPI PostQuitMessage16(INT16);
void        WINAPI PostQuitMessage32(INT32);
#define     PostQuitMessage WINELIB_NAME(PostQuitMessage)
BOOL16      WINAPI PtInRect16(const RECT16*,POINT16);
BOOL32      WINAPI PtInRect32(const RECT32*,POINT32);
#define     PtInRect WINELIB_NAME(PtInRect)
BOOL16      WINAPI PtInRegion16(HRGN16,INT16,INT16);
BOOL32      WINAPI PtInRegion32(HRGN32,INT32,INT32);
#define     PtInRegion WINELIB_NAME(PtInRegion)
BOOL16      WINAPI PtVisible16(HDC16,INT16,INT16);
BOOL32      WINAPI PtVisible32(HDC32,INT32,INT32);
#define     PtVisible WINELIB_NAME(PtVisible)
UINT16      WINAPI RealizePalette16(HDC16);
UINT32      WINAPI RealizePalette32(HDC32);
#define     RealizePalette WINELIB_NAME(RealizePalette)
BOOL16      WINAPI Rectangle16(HDC16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI Rectangle32(HDC32,INT32,INT32,INT32,INT32);
#define     Rectangle WINELIB_NAME(Rectangle)
BOOL16      WINAPI RectInRegion16(HRGN16,const RECT16 *);
BOOL32      WINAPI RectInRegion32(HRGN32,const RECT32 *);
#define     RectInRegion WINELIB_NAME(RectInRegion)
BOOL16      WINAPI RectVisible16(HDC16,LPRECT16);
BOOL32      WINAPI RectVisible32(HDC32,LPRECT32);
#define     RectVisible WINELIB_NAME(RectVisible)
BOOL16      WINAPI RedrawWindow16(HWND16,const RECT16*,HRGN16,UINT16);
BOOL32      WINAPI RedrawWindow32(HWND32,const RECT32*,HRGN32,UINT32);
#define     RedrawWindow WINELIB_NAME(RedrawWindow)
DWORD       WINAPI RegCreateKey16(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegCreateKey32A(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegCreateKey32W(HKEY,LPCWSTR,LPHKEY);
#define     RegCreateKey WINELIB_NAME_AW(RegCreateKey)
DWORD       WINAPI RegDeleteKey16(HKEY,LPCSTR);
DWORD       WINAPI RegDeleteKey32A(HKEY,LPCSTR);
DWORD       WINAPI RegDeleteKey32W(HKEY,LPWSTR);
#define     RegDeleteKey WINELIB_NAME_AW(RegDeleteKey)
DWORD       WINAPI RegDeleteValue16(HKEY,LPSTR);
DWORD       WINAPI RegDeleteValue32A(HKEY,LPSTR);
DWORD       WINAPI RegDeleteValue32W(HKEY,LPWSTR);
#define     RegDeleteValue WINELIB_NAME_AW(RegDeleteValue)
DWORD       WINAPI RegEnumKey16(HKEY,DWORD,LPSTR,DWORD);
DWORD       WINAPI RegEnumKey32A(HKEY,DWORD,LPSTR,DWORD);
DWORD       WINAPI RegEnumKey32W(HKEY,DWORD,LPWSTR,DWORD);
#define     RegEnumKey WINELIB_NAME_AW(RegEnumKey)
DWORD       WINAPI RegEnumValue16(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegEnumValue32A(HKEY,DWORD,LPSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegEnumValue32W(HKEY,DWORD,LPWSTR,LPDWORD,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegEnumValue WINELIB_NAME_AW(RegEnumValue)
ATOM        WINAPI RegisterClass16(const WNDCLASS16*);
ATOM        WINAPI RegisterClass32A(const WNDCLASS32A *);
ATOM        WINAPI RegisterClass32W(const WNDCLASS32W *);
#define     RegisterClass WINELIB_NAME_AW(RegisterClass)
ATOM        WINAPI RegisterClassEx16(const WNDCLASSEX16*);
ATOM        WINAPI RegisterClassEx32A(const WNDCLASSEX32A *);
ATOM        WINAPI RegisterClassEx32W(const WNDCLASSEX32W *);
#define     RegisterClassEx WINELIB_NAME_AW(RegisterClassEx)
UINT16      WINAPI RegisterClipboardFormat16(LPCSTR);
UINT32      WINAPI RegisterClipboardFormat32A(LPCSTR);
UINT32      WINAPI RegisterClipboardFormat32W(LPCWSTR);
#define     RegisterClipboardFormat WINELIB_NAME_AW(RegisterClipboardFormat)
WORD        WINAPI RegisterWindowMessage16(SEGPTR);
WORD        WINAPI RegisterWindowMessage32A(LPCSTR);
WORD        WINAPI RegisterWindowMessage32W(LPCWSTR);
#define     RegisterWindowMessage WINELIB_NAME_AW(RegisterWindowMessage)
DWORD       WINAPI RegOpenKey16(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegOpenKey32A(HKEY,LPCSTR,LPHKEY);
DWORD       WINAPI RegOpenKey32W(HKEY,LPCWSTR,LPHKEY);
#define     RegOpenKey WINELIB_NAME_AW(RegOpenKey)
DWORD       WINAPI RegQueryValue16(HKEY,LPSTR,LPSTR,LPDWORD);
DWORD       WINAPI RegQueryValue32A(HKEY,LPSTR,LPSTR,LPDWORD);
DWORD       WINAPI RegQueryValue32W(HKEY,LPWSTR,LPWSTR,LPDWORD);
#define     RegQueryValue WINELIB_NAME_AW(RegQueryValue)
DWORD       WINAPI RegQueryValueEx16(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegQueryValueEx32A(HKEY,LPSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
DWORD       WINAPI RegQueryValueEx32W(HKEY,LPWSTR,LPDWORD,LPDWORD,LPBYTE,LPDWORD);
#define     RegQueryValueEx WINELIB_NAME_AW(RegQueryValueEx)
DWORD       WINAPI RegSetValue16(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD       WINAPI RegSetValue32A(HKEY,LPCSTR,DWORD,LPCSTR,DWORD);
DWORD       WINAPI RegSetValue32W(HKEY,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     RegSetValue WINELIB_NAME_AW(RegSetValue)
DWORD       WINAPI RegSetValueEx16(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
DWORD       WINAPI RegSetValueEx32A(HKEY,LPSTR,DWORD,DWORD,LPBYTE,DWORD);
DWORD       WINAPI RegSetValueEx32W(HKEY,LPWSTR,DWORD,DWORD,LPBYTE,DWORD);
#define     RegSetValueEx WINELIB_NAME_AW(RegSetValueEx)
INT16       WINAPI ReleaseDC16(HWND16,HDC16);
INT32       WINAPI ReleaseDC32(HWND32,HDC32);
#define     ReleaseDC WINELIB_NAME(ReleaseDC)
BOOL16      WINAPI RemoveDirectory16(LPCSTR);
BOOL32      WINAPI RemoveDirectory32A(LPCSTR);
BOOL32      WINAPI RemoveDirectory32W(LPCWSTR);
#define     RemoveDirectory WINELIB_NAME_AW(RemoveDirectory)
BOOL16      WINAPI RemoveFontResource16(SEGPTR);
BOOL32      WINAPI RemoveFontResource32A(LPCSTR);
BOOL32      WINAPI RemoveFontResource32W(LPCWSTR);
#define     RemoveFontResource WINELIB_NAME_AW(RemoveFontResource)
BOOL16      WINAPI RemoveMenu16(HMENU16,UINT16,UINT16);
BOOL32      WINAPI RemoveMenu32(HMENU32,UINT32,UINT32);
#define     RemoveMenu WINELIB_NAME(RemoveMenu)
HANDLE16    WINAPI RemoveProp16(HWND16,LPCSTR);
HANDLE32    WINAPI RemoveProp32A(HWND32,LPCSTR);
HANDLE32    WINAPI RemoveProp32W(HWND32,LPCWSTR);
#define     RemoveProp WINELIB_NAME_AW(RemoveProp)
VOID        WINAPI ReplyMessage16(LRESULT);
BOOL32      WINAPI ReplyMessage32(LRESULT);
#define     ReplyMessage WINELIB_NAME(ReplyMessage)
HDC16       WINAPI ResetDC16(HDC16,const DEVMODE16 *);
HDC32       WINAPI ResetDC32A(HDC32,const DEVMODE32A *);
HDC32       WINAPI ResetDC32W(HDC32,const DEVMODE32W *);
#define     ResetDC WINELIB_NAME_AW(ResetDC)
BOOL16      WINAPI ResizePalette16(HPALETTE16,UINT16);
BOOL32      WINAPI ResizePalette32(HPALETTE32,UINT32);
#define     ResizePalette WINELIB_NAME(ResizePalette)
BOOL16      WINAPI RestoreDC16(HDC16,INT16);
BOOL32      WINAPI RestoreDC32(HDC32,INT32);
#define     RestoreDC WINELIB_NAME(RestoreDC)
BOOL16      WINAPI RoundRect16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16);
BOOL32      WINAPI RoundRect32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32);
#define     RoundRect WINELIB_NAME(RoundRect)
INT16       WINAPI SaveDC16(HDC16);
INT32       WINAPI SaveDC32(HDC32);
#define     SaveDC WINELIB_NAME(SaveDC)
BOOL16      WINAPI ScaleViewportExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI ScaleViewportExtEx32(HDC32,INT32,INT32,INT32,INT32,LPSIZE32);
#define     ScaleViewportExtEx WINELIB_NAME(ScaleViewportExtEx)
BOOL16      WINAPI ScaleWindowExtEx16(HDC16,INT16,INT16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI ScaleWindowExtEx32(HDC32,INT32,INT32,INT32,INT32,LPSIZE32);
#define     ScaleWindowExtEx WINELIB_NAME(ScaleWindowExtEx)
void        WINAPI ScreenToClient16(HWND16,LPPOINT16);
void        WINAPI ScreenToClient32(HWND32,LPPOINT32);
#define     ScreenToClient WINELIB_NAME(ScreenToClient)
VOID        WINAPI ScrollChildren16(HWND16,UINT16,WPARAM16,LPARAM);
VOID        WINAPI ScrollChildren32(HWND32,UINT32,WPARAM32,LPARAM);
#define     ScrollChildren WINELIB_NAME(ScrollChildren)
BOOL16      WINAPI ScrollDC16(HDC16,INT16,INT16,const RECT16*,const RECT16*,
                      HRGN16,LPRECT16);
BOOL32      WINAPI ScrollDC32(HDC32,INT32,INT32,const RECT32*,const RECT32*,
                      HRGN32,LPRECT32);
#define     ScrollDC WINELIB_NAME(ScrollDC)
void        WINAPI ScrollWindow16(HWND16,INT16,INT16,const RECT16*,const RECT16*);
BOOL32      WINAPI ScrollWindow32(HWND32,INT32,INT32,const RECT32*,const RECT32*);
#define     ScrollWindow WINELIB_NAME(ScrollWindow)
INT16       WINAPI ScrollWindowEx16(HWND16,INT16,INT16,const RECT16*,
                                    const RECT16*,HRGN16,LPRECT16,UINT16);
INT32       WINAPI ScrollWindowEx32(HWND32,INT32,INT32,const RECT32*,
                                    const RECT32*,HRGN32,LPRECT32,UINT32);
#define     ScrollWindowEx WINELIB_NAME(ScrollWindowEx)
BOOL16      WINAPI SelectClipPath16(HDC16,INT16);
BOOL32      WINAPI SelectClipPath32(HDC32,INT32);
#define     SelectClipPath WINELIB_NAME(SelectClipPath)
INT16       WINAPI SelectClipRgn16(HDC16,HRGN16);
INT32       WINAPI SelectClipRgn32(HDC32,HRGN32);
#define     SelectClipRgn WINELIB_NAME(SelectClipRgn)
HGDIOBJ16   WINAPI SelectObject16(HDC16,HGDIOBJ16);
HGDIOBJ32   WINAPI SelectObject32(HDC32,HGDIOBJ32);
#define     SelectObject WINELIB_NAME(SelectObject)
HPALETTE16  WINAPI SelectPalette16(HDC16,HPALETTE16,BOOL16);
HPALETTE32  WINAPI SelectPalette32(HDC32,HPALETTE32,BOOL32);
#define     SelectPalette WINELIB_NAME(SelectPalette)
LRESULT     WINAPI SendDlgItemMessage16(HWND16,INT16,UINT16,WPARAM16,LPARAM);
LRESULT     WINAPI SendDlgItemMessage32A(HWND32,INT32,UINT32,WPARAM32,LPARAM);
LRESULT     WINAPI SendDlgItemMessage32W(HWND32,INT32,UINT32,WPARAM32,LPARAM);
#define     SendDlgItemMessage WINELIB_NAME_AW(SendDlgItemMessage)
LRESULT     WINAPI SendMessage16(HWND16,UINT16,WPARAM16,LPARAM);
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
HWND16      WINAPI SetActiveWindow16(HWND16);
HWND32      WINAPI SetActiveWindow32(HWND32);
#define     SetActiveWindow WINELIB_NAME(SetActiveWindow)
INT16       WINAPI SetArcDirection16(HDC16,INT16);
INT32       WINAPI SetArcDirection32(HDC32,INT32);
#define     SetArcDirection WINELIB_NAME(SetArcDirection)
LONG        WINAPI SetBitmapBits16(HBITMAP16,LONG,LPCVOID);
LONG        WINAPI SetBitmapBits32(HBITMAP32,LONG,LPCVOID);
#define     SetBitmapBits WINELIB_NAME(SetBitmapBits)
BOOL16      WINAPI SetBitmapDimensionEx16(HBITMAP16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI SetBitmapDimensionEx32(HBITMAP32,INT32,INT32,LPSIZE32);
#define     SetBitmapDimensionEx WINELIB_NAME(SetBitmapDimensionEx)
COLORREF    WINAPI SetBkColor16(HDC16,COLORREF);
COLORREF    WINAPI SetBkColor32(HDC32,COLORREF);
#define     SetBkColor WINELIB_NAME(SetBkColor)
INT16       WINAPI SetBkMode16(HDC16,INT16);
INT32       WINAPI SetBkMode32(HDC32,INT32);
#define     SetBkMode WINELIB_NAME(SetBkMode)
HWND16      WINAPI SetCapture16(HWND16);
HWND32      WINAPI SetCapture32(HWND32);
#define     SetCapture WINELIB_NAME(SetCapture)
VOID        WINAPI SetCaretBlinkTime16(UINT16);
BOOL32      WINAPI SetCaretBlinkTime32(UINT32);
#define     SetCaretBlinkTime WINELIB_NAME(SetCaretBlinkTime)
VOID        WINAPI SetCaretPos16(INT16,INT16);
BOOL32      WINAPI SetCaretPos32(INT32,INT32);
#define     SetCaretPos WINELIB_NAME(SetCaretPos)
LONG        WINAPI SetClassLong16(HWND16,INT16,LONG);
LONG        WINAPI SetClassLong32A(HWND32,INT32,LONG);
LONG        WINAPI SetClassLong32W(HWND32,INT32,LONG);
#define     SetClassLong WINELIB_NAME_AW(SetClassLong)
WORD        WINAPI SetClassWord16(HWND16,INT16,WORD);
WORD        WINAPI SetClassWord32(HWND32,INT32,WORD);
#define     SetClassWord WINELIB_NAME(SetClassWord)
HANDLE16    WINAPI SetClipboardData16(UINT16,HANDLE16);
HANDLE32    WINAPI SetClipboardData32(UINT32,HANDLE32);
#define     SetClipboardData WINELIB_NAME(SetClipboardData)
HWND16      WINAPI SetClipboardViewer16(HWND16);
HWND32      WINAPI SetClipboardViewer32(HWND32);
#define     SetClipboardViewer WINELIB_NAME(SetClipboardViewer)
INT16       WINAPI SetCommBreak16(INT16);
BOOL32      WINAPI SetCommBreak32(INT32);
#define     SetCommBreak WINELIB_NAME(SetCommBreak)
INT16       WINAPI SetCommState16(LPDCB16);
BOOL32      WINAPI SetCommState32(INT32,LPDCB32);
#define     SetCommState WINELIB_NAME(SetCommState)
BOOL16      WINAPI SetCurrentDirectory16(LPCSTR);
BOOL32      WINAPI SetCurrentDirectory32A(LPCSTR);
BOOL32      WINAPI SetCurrentDirectory32W(LPCWSTR);
#define     SetCurrentDirectory WINELIB_NAME_AW(SetCurrentDirectory)
HCURSOR16   WINAPI SetCursor16(HCURSOR16);
HCURSOR32   WINAPI SetCursor32(HCURSOR32);
#define     SetCursor WINELIB_NAME(SetCursor)
void        WINAPI SetCursorPos16(INT16,INT16);
BOOL32      WINAPI SetCursorPos32(INT32,INT32);
#define     SetCursorPos WINELIB_NAME(SetCursorPos)
BOOL16      WINAPI SetDeskWallPaper16(LPCSTR);
BOOL32      WINAPI SetDeskWallPaper32(LPCSTR);
#define     SetDeskWallPaper WINELIB_NAME(SetDeskWallPaper)
UINT16      WINAPI SetDIBColorTable16(HDC16,UINT16,UINT16,RGBQUAD*);
UINT32      WINAPI SetDIBColorTable32(HDC32,UINT32,UINT32,RGBQUAD*);
#define     SetDIBColorTable WINELIB_NAME(SetDIBColorTable)
INT16       WINAPI SetDIBits16(HDC16,HBITMAP16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT32       WINAPI SetDIBits32(HDC32,HBITMAP32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
#define     SetDIBits WINELIB_NAME(SetDIBits)
INT16       WINAPI SetDIBitsToDevice16(HDC16,INT16,INT16,INT16,INT16,INT16,
                         INT16,UINT16,UINT16,LPCVOID,const BITMAPINFO*,UINT16);
INT32       WINAPI SetDIBitsToDevice32(HDC32,INT32,INT32,DWORD,DWORD,INT32,
                         INT32,UINT32,UINT32,LPCVOID,const BITMAPINFO*,UINT32);
#define     SetDIBitsToDevice WINELIB_NAME(SetDIBitsToDevice)
void        WINAPI SetDlgItemInt16(HWND16,INT16,UINT16,BOOL16);
void        WINAPI SetDlgItemInt32(HWND32,INT32,UINT32,BOOL32);
#define     SetDlgItemInt WINELIB_NAME(SetDlgItemInt)
void        WINAPI SetDlgItemText16(HWND16,INT16,SEGPTR);
void        WINAPI SetDlgItemText32A(HWND32,INT32,LPCSTR);
void        WINAPI SetDlgItemText32W(HWND32,INT32,LPCWSTR);
#define     SetDlgItemText WINELIB_NAME_AW(SetDlgItemText)
VOID        WINAPI SetDoubleClickTime16(UINT16);
BOOL32      WINAPI SetDoubleClickTime32(UINT32);
#define     SetDoubleClickTime WINELIB_NAME(SetDoubleClickTime)
UINT16      WINAPI SetErrorMode16(UINT16);
UINT32      WINAPI SetErrorMode32(UINT32);
#define     SetErrorMode WINELIB_NAME(SetErrorMode)
BOOL16      WINAPI SetFileAttributes16(LPCSTR,DWORD);
BOOL32      WINAPI SetFileAttributes32A(LPCSTR,DWORD);
BOOL32      WINAPI SetFileAttributes32W(LPCWSTR,DWORD);
#define     SetFileAttributes WINELIB_NAME_AW(SetFileAttributes)
HWND16      WINAPI SetFocus16(HWND16);
HWND32      WINAPI SetFocus32(HWND32);
#define     SetFocus WINELIB_NAME(SetFocus)
BOOL16      WINAPI SetForegroundWindow16(HWND16);
BOOL32      WINAPI SetForegroundWindow32(HWND32);
#define     SetForegroundWindow WINELIB_NAME(SetForegroundWindow)
UINT16      WINAPI SetHandleCount16(UINT16);
UINT32      WINAPI SetHandleCount32(UINT32);
#define     SetHandleCount WINELIB_NAME(SetHandleCount)
void        WINAPI SetInternalWindowPos16(HWND16,UINT16,LPRECT16,LPPOINT16);
void        WINAPI SetInternalWindowPos32(HWND32,UINT32,LPRECT32,LPPOINT32);
#define     SetInternalWindowPos WINELIB_NAME(SetInternalWindowPos)
INT16       WINAPI SetMapMode16(HDC16,INT16);
INT32       WINAPI SetMapMode32(HDC32,INT32);
#define     SetMapMode WINELIB_NAME(SetMapMode)
DWORD       WINAPI SetMapperFlags16(HDC16,DWORD);
DWORD       WINAPI SetMapperFlags32(HDC32,DWORD);
#define     SetMapperFlags WINELIB_NAME(SetMapperFlags)
BOOL16      WINAPI SetMenu16(HWND16,HMENU16);
BOOL32      WINAPI SetMenu32(HWND32,HMENU32);
#define     SetMenu WINELIB_NAME(SetMenu)
BOOL16      WINAPI SetMenuItemBitmaps16(HMENU16,UINT16,UINT16,HBITMAP16,HBITMAP16);
BOOL32      WINAPI SetMenuItemBitmaps32(HMENU32,UINT32,UINT32,HBITMAP32,HBITMAP32);
#define     SetMenuItemBitmaps WINELIB_NAME(SetMenuItemBitmaps)
BOOL16      WINAPI SetMessageQueue16(INT16);
BOOL32      WINAPI SetMessageQueue32(INT32);
#define     SetMessageQueue WINELIB_NAME(SetMessageQueue)
UINT16      WINAPI SetPaletteEntries16(HPALETTE16,UINT16,UINT16,LPPALETTEENTRY);
UINT32      WINAPI SetPaletteEntries32(HPALETTE32,UINT32,UINT32,LPPALETTEENTRY);
#define     SetPaletteEntries WINELIB_NAME(SetPaletteEntries)
HWND16      WINAPI SetParent16(HWND16,HWND16);
HWND32      WINAPI SetParent32(HWND32,HWND32);
#define     SetParent WINELIB_NAME(SetParent)
COLORREF    WINAPI SetPixel16(HDC16,INT16,INT16,COLORREF);
COLORREF    WINAPI SetPixel32(HDC32,INT32,INT32,COLORREF);
#define     SetPixel WINELIB_NAME(SetPixel)
BOOL32      WINAPI SetPixelFormat(HDC32,int,PIXELFORMATDESCRIPTOR*);
INT16       WINAPI SetPolyFillMode16(HDC16,INT16);
INT32       WINAPI SetPolyFillMode32(HDC32,INT32);
#define     SetPolyFillMode WINELIB_NAME(SetPolyFillMode)
BOOL16      WINAPI SetProp16(HWND16,LPCSTR,HANDLE16);
BOOL32      WINAPI SetProp32A(HWND32,LPCSTR,HANDLE32);
BOOL32      WINAPI SetProp32W(HWND32,LPCWSTR,HANDLE32);
#define     SetProp WINELIB_NAME_AW(SetProp)
void        WINAPI SetRect16(LPRECT16,INT16,INT16,INT16,INT16);
void        WINAPI SetRect32(LPRECT32,INT32,INT32,INT32,INT32);
#define     SetRect WINELIB_NAME(SetRect)
void        WINAPI SetRectEmpty16(LPRECT16);
void        WINAPI SetRectEmpty32(LPRECT32);
#define     SetRectEmpty WINELIB_NAME(SetRectEmpty)
VOID        WINAPI SetRectRgn16(HRGN16,INT16,INT16,INT16,INT16);
VOID        WINAPI SetRectRgn32(HRGN32,INT32,INT32,INT32,INT32);
#define     SetRectRgn WINELIB_NAME(SetRectRgn)
INT16       WINAPI SetRelAbs16(HDC16,INT16);
INT32       WINAPI SetRelAbs32(HDC32,INT32);
#define     SetRelAbs WINELIB_NAME(SetRelAbs)
INT16       WINAPI SetROP216(HDC16,INT16);
INT32       WINAPI SetROP232(HDC32,INT32);
#define     SetROP2 WINELIB_NAME(SetROP2)
INT16       WINAPI SetScrollInfo16(HWND16,INT16,const SCROLLINFO*,BOOL16);
INT32       WINAPI SetScrollInfo32(HWND32,INT32,const SCROLLINFO*,BOOL32);
#define     SetScrollInfo WINELIB_NAME(SetScrollInfo)
INT16       WINAPI SetScrollPos16(HWND16,INT16,INT16,BOOL16);
INT32       WINAPI SetScrollPos32(HWND32,INT32,INT32,BOOL32);
#define     SetScrollPos WINELIB_NAME(SetScrollPos)
void        WINAPI SetScrollRange16(HWND16,INT16,INT16,INT16,BOOL16);
BOOL32      WINAPI SetScrollRange32(HWND32,INT32,INT32,INT32,BOOL32);
#define     SetScrollRange WINELIB_NAME(SetScrollRange)
INT16       WINAPI SetSoundNoise16(INT16,INT16);
DWORD       WINAPI SetSoundNoise32(DWORD,DWORD);
#define     SetSoundNoise WINELIB_NAME(SetSoundNoise)
INT16       WINAPI SetStretchBltMode16(HDC16,INT16);
INT32       WINAPI SetStretchBltMode32(HDC32,INT32);
#define     SetStretchBltMode WINELIB_NAME(SetStretchBltMode)
LONG        WINAPI SetSwapAreaSize16(WORD);
#define     SetSwapAreaSize32(w) (w)
#define     SetSwapAreaSize WINELIB_NAME(SetSwapAreaSize)
VOID        WINAPI SetSysColors16(INT16,const INT16*,const COLORREF*);
BOOL32      WINAPI SetSysColors32(INT32,const INT32*,const COLORREF*);
#define     SetSysColors WINELIB_NAME(SetSysColors)
HWND16      WINAPI SetSysModalWindow16(HWND16);
#define     SetSysModalWindow32(hwnd) ((HWND32)0)
#define     SetSysModalWindow WINELIB_NAME(SetSysModalWindow)
BOOL16      WINAPI SetSystemMenu16(HWND16,HMENU16);
BOOL32      WINAPI SetSystemMenu32(HWND32,HMENU32);
#define     SetSystemMenu WINELIB_NAME(SetSystemMenu)
UINT16      WINAPI SetSystemPaletteUse16(HDC16,UINT16);
UINT32      WINAPI SetSystemPaletteUse32(HDC32,UINT32);
#define     SetSystemPaletteUse WINELIB_NAME(SetSystemPaletteUse)
UINT16      WINAPI SetSystemTimer16(HWND16,UINT16,UINT16,TIMERPROC16);
UINT32      WINAPI SetSystemTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define     SetSystemTimer WINELIB_NAME(SetSystemTimer)
UINT16      WINAPI SetTextAlign16(HDC16,UINT16);
UINT32      WINAPI SetTextAlign32(HDC32,UINT32);
#define     SetTextAlign WINELIB_NAME(SetTextAlign)
INT16       WINAPI SetTextCharacterExtra16(HDC16,INT16);
INT32       WINAPI SetTextCharacterExtra32(HDC32,INT32);
#define     SetTextCharacterExtra WINELIB_NAME(SetTextCharacterExtra)
COLORREF    WINAPI SetTextColor16(HDC16,COLORREF);
COLORREF    WINAPI SetTextColor32(HDC32,COLORREF);
#define     SetTextColor WINELIB_NAME(SetTextColor)
INT16       WINAPI SetTextJustification16(HDC16,INT16,INT16);
BOOL32      WINAPI SetTextJustification32(HDC32,INT32,INT32);
#define     SetTextJustification WINELIB_NAME(SetTextJustification)
UINT16      WINAPI SetTimer16(HWND16,UINT16,UINT16,TIMERPROC16);
UINT32      WINAPI SetTimer32(HWND32,UINT32,UINT32,TIMERPROC32);
#define     SetTimer WINELIB_NAME(SetTimer)
BOOL16      WINAPI SetViewportExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI SetViewportExtEx32(HDC32,INT32,INT32,LPSIZE32);
#define     SetViewportExtEx WINELIB_NAME(SetViewportExtEx)
BOOL16      WINAPI SetViewportOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI SetViewportOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     SetViewportOrgEx WINELIB_NAME(SetViewportOrgEx)
INT16       WINAPI SetVoiceAccent16(INT16,INT16,INT16,INT16,INT16);
DWORD       WINAPI SetVoiceAccent32(DWORD,DWORD,DWORD,DWORD,DWORD);
#define     SetVoiceAccent WINELIB_NAME(SetVoiceAccent)
INT16       WINAPI SetVoiceEnvelope16(INT16,INT16,INT16);
DWORD       WINAPI SetVoiceEnvelope32(DWORD,DWORD,DWORD);
#define     SetVoiceEnvelope WINELIB_NAME(SetVoiceEnvelope)
INT16       WINAPI SetVoiceNote16(INT16,INT16,INT16,INT16);
DWORD       WINAPI SetVoiceNote32(DWORD,DWORD,DWORD,DWORD);
#define     SetVoiceNote WINELIB_NAME(SetVoiceNote)
INT16       WINAPI SetVoiceQueueSize16(INT16,INT16);
DWORD       WINAPI SetVoiceQueueSize32(DWORD,DWORD);
#define     SetVoiceQueueSize WINELIB_NAME(SetVoiceQueueSize)
INT16       WINAPI SetVoiceSound16(INT16,DWORD,INT16);
DWORD       WINAPI SetVoiceSound32(DWORD,DWORD,DWORD);
#define     SetVoiceSound WINELIB_NAME(SetVoiceSound)
INT16       WINAPI SetVoiceThreshold16(INT16,INT16);
DWORD       WINAPI SetVoiceThreshold32(DWORD,DWORD);
#define     SetVoiceThreshold WINELIB_NAME(SetVoiceThreshold)
BOOL16      WINAPI SetWindowExtEx16(HDC16,INT16,INT16,LPSIZE16);
BOOL32      WINAPI SetWindowExtEx32(HDC32,INT32,INT32,LPSIZE32);
#define     SetWindowExtEx WINELIB_NAME(SetWindowExtEx)
LONG        WINAPI SetWindowLong16(HWND16,INT16,LONG);
LONG        WINAPI SetWindowLong32A(HWND32,INT32,LONG);
LONG        WINAPI SetWindowLong32W(HWND32,INT32,LONG);
#define     SetWindowLong WINELIB_NAME_AW(SetWindowLong)
BOOL16      WINAPI SetWindowOrgEx16(HDC16,INT16,INT16,LPPOINT16);
BOOL32      WINAPI SetWindowOrgEx32(HDC32,INT32,INT32,LPPOINT32);
#define     SetWindowOrgEx WINELIB_NAME(SetWindowOrgEx)
BOOL16      WINAPI SetWindowPlacement16(HWND16,const WINDOWPLACEMENT16*);
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
BOOL16      WINAPI SetWindowPos16(HWND16,HWND16,INT16,INT16,INT16,INT16,WORD);
BOOL32      WINAPI SetWindowPos32(HWND32,HWND32,INT32,INT32,INT32,INT32,WORD);
#define     SetWindowPos WINELIB_NAME(SetWindowPos)
void        WINAPI SetWindowText16(HWND16,SEGPTR);
void        WINAPI SetWindowText32A(HWND32,LPCSTR);
void        WINAPI SetWindowText32W(HWND32,LPCWSTR);
#define     SetWindowText WINELIB_NAME_AW(SetWindowText)
WORD        WINAPI SetWindowWord16(HWND16,INT16,WORD);
WORD        WINAPI SetWindowWord32(HWND32,INT32,WORD);
#define     SetWindowWord WINELIB_NAME(SetWindowWord)
BOOL16      WINAPI ShellAbout16(HWND16,LPCSTR,LPCSTR,HICON16);
BOOL32      WINAPI ShellAbout32A(HWND32,LPCSTR,LPCSTR,HICON32);
BOOL32      WINAPI ShellAbout32W(HWND32,LPCWSTR,LPCWSTR,HICON32);
#define     ShellAbout WINELIB_NAME_AW(ShellAbout)
HINSTANCE16 WINAPI ShellExecute16(HWND16,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT16);
HINSTANCE32 WINAPI ShellExecute32A(HWND32,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT32);
HINSTANCE32 WINAPI ShellExecute32W(HWND32,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT32);
#define     ShellExecute WINELIB_NAME_AW(ShellExecute)
VOID        WINAPI ShowCaret16(HWND16);
BOOL32      WINAPI ShowCaret32(HWND32);
#define     ShowCaret WINELIB_NAME(ShowCaret)
INT16       WINAPI ShowCursor16(BOOL16);
INT32       WINAPI ShowCursor32(BOOL32);
#define     ShowCursor WINELIB_NAME(ShowCursor)
void        WINAPI ShowScrollBar16(HWND16,INT16,BOOL16);
BOOL32      WINAPI ShowScrollBar32(HWND32,INT32,BOOL32);
#define     ShowScrollBar WINELIB_NAME(ShowScrollBar)
VOID        WINAPI ShowOwnedPopups16(HWND16,BOOL16);
BOOL32      WINAPI ShowOwnedPopups32(HWND32,BOOL32);
#define     ShowOwnedPopups WINELIB_NAME(ShowOwnedPopups)
BOOL16      WINAPI ShowWindow16(HWND16,INT16);
BOOL32      WINAPI ShowWindow32(HWND32,INT32);
#define     ShowWindow WINELIB_NAME(ShowWindow)
DWORD       WINAPI SizeofResource16(HMODULE16,HRSRC16);
DWORD       WINAPI SizeofResource32(HMODULE32,HRSRC32);
#define     SizeofResource WINELIB_NAME(SizeofResource)
INT16       WINAPI StartDoc16(HDC16,const DOCINFO16*);
INT32       WINAPI StartDoc32A(HDC32,const DOCINFO32A*);
INT32       WINAPI StartDoc32W(HDC32,const DOCINFO32W*);
#define     StartDoc WINELIB_NAME_AW(StartDoc)
INT16       WINAPI StartSound16(void);
VOID        WINAPI StartSound32(void);
#define     StartSound WINELIB_NAME(StartSound)
INT16       WINAPI StopSound16(void);
VOID        WINAPI StopSound32(void);
#define     StopSound WINELIB_NAME(StopSound)
BOOL16      WINAPI StretchBlt16(HDC16,INT16,INT16,INT16,INT16,HDC16,INT16,
                                INT16,INT16,INT16,DWORD);
BOOL32      WINAPI StretchBlt32(HDC32,INT32,INT32,INT32,INT32,HDC32,INT32,
                                INT32,INT32,INT32,DWORD);
#define     StretchBlt WINELIB_NAME(StretchBlt)
INT16       WINAPI StretchDIBits16(HDC16,INT16,INT16,INT16,INT16,INT16,INT16,
                       INT16,INT16,const VOID*,const BITMAPINFO*,UINT16,DWORD);
INT32       WINAPI StretchDIBits32(HDC32,INT32,INT32,INT32,INT32,INT32,INT32,
                       INT32,INT32,const VOID*,const BITMAPINFO*,UINT32,DWORD);
#define     StretchDIBits WINELIB_NAME(StretchDIBits)
BOOL16      WINAPI SubtractRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL32      WINAPI SubtractRect32(LPRECT32,const RECT32*,const RECT32*);
#define     SubtractRect WINELIB_NAME(SubtractRect)
BOOL32      WINAPI SwapBuffers(HDC32);
BOOL16      WINAPI SwapMouseButton16(BOOL16);
BOOL32      WINAPI SwapMouseButton32(BOOL32);
#define     SwapMouseButton WINELIB_NAME(SwapMouseButton)
VOID        WINAPI SwitchToThisWindow16(HWND16,BOOL16);
VOID        WINAPI SwitchToThisWindow32(HWND32,BOOL32);
#define     SwitchToThisWindow WINELIB_NAME(SwitchToThisWindow)
INT16       WINAPI SyncAllVoices16(void);
DWORD       WINAPI SyncAllVoices32(void);
#define     SyncAllVoices WINELIB_NAME(SyncAllVoices)
BOOL16      WINAPI SystemParametersInfo16(UINT16,UINT16,LPVOID,UINT16);
BOOL32      WINAPI SystemParametersInfo32A(UINT32,UINT32,LPVOID,UINT32);
BOOL32      WINAPI SystemParametersInfo32W(UINT32,UINT32,LPVOID,UINT32);
#define     SystemParametersInfo WINELIB_NAME_AW(SystemParametersInfo)
LONG        WINAPI TabbedTextOut16(HDC16,INT16,INT16,LPCSTR,INT16,INT16,const INT16*,INT16);
LONG        WINAPI TabbedTextOut32A(HDC32,INT32,INT32,LPCSTR,INT32,INT32,const INT32*,INT32);
LONG        WINAPI TabbedTextOut32W(HDC32,INT32,INT32,LPCWSTR,INT32,INT32,const INT32*,INT32);
#define     TabbedTextOut WINELIB_NAME_AW(TabbedTextOut)
BOOL16      WINAPI TextOut16(HDC16,INT16,INT16,LPCSTR,INT16);
BOOL32      WINAPI TextOut32A(HDC32,INT32,INT32,LPCSTR,INT32);
BOOL32      WINAPI TextOut32W(HDC32,INT32,INT32,LPCWSTR,INT32);
#define     TextOut WINELIB_NAME_AW(TextOut)
INT16       WINAPI ToAscii16(UINT16,UINT16,LPBYTE,LPVOID,UINT16);
INT32       WINAPI ToAscii32(UINT32,UINT32,LPBYTE,LPWORD,UINT32);
#define     ToAscii WINELIB_NAME(ToAscii)
BOOL16      WINAPI TrackPopupMenu16(HMENU16,UINT16,INT16,INT16,INT16,HWND16,const RECT16*);
BOOL32      WINAPI TrackPopupMenu32(HMENU32,UINT32,INT32,INT32,INT32,HWND32,const RECT32*);
#define     TrackPopupMenu WINELIB_NAME(TrackPopupMenu)
INT16       WINAPI TranslateAccelerator16(HWND16,HACCEL16,LPMSG16);
INT32       WINAPI TranslateAccelerator32(HWND32,HACCEL32,LPMSG32);
#define     TranslateAccelerator WINELIB_NAME(TranslateAccelerator)
BOOL16      WINAPI TranslateMDISysAccel16(HWND16,LPMSG16);
BOOL32      WINAPI TranslateMDISysAccel32(HWND32,LPMSG32);
#define     TranslateMDISysAccel WINELIB_NAME(TranslateMDISysAccel)
BOOL16      WINAPI TranslateMessage16(const MSG16*);
BOOL32      WINAPI TranslateMessage32(const MSG32*);
#define     TranslateMessage WINELIB_NAME(TranslateMessage)
INT16       WINAPI TransmitCommChar16(INT16,CHAR);
BOOL32      WINAPI TransmitCommChar32(INT32,CHAR);
#define     TransmitCommChar WINELIB_NAME(TransmitCommChar)
BOOL16      WINAPI UnhookWindowsHook16(INT16,HOOKPROC16);
BOOL32      WINAPI UnhookWindowsHook32(INT32,HOOKPROC32);
#define     UnhookWindowsHook WINELIB_NAME(UnhookWindowsHook)
BOOL16      WINAPI UnhookWindowsHookEx16(HHOOK);
BOOL32      WINAPI UnhookWindowsHookEx32(HHOOK);
#define     UnhookWindowsHookEx WINELIB_NAME(UnhookWindowsHookEx)
BOOL16      WINAPI UnionRect16(LPRECT16,const RECT16*,const RECT16*);
BOOL32      WINAPI UnionRect32(LPRECT32,const RECT32*,const RECT32*);
#define     UnionRect WINELIB_NAME(UnionRect)
void        WINAPI UnlockSegment16(HGLOBAL16);
#define     UnlockSegment32(handle) GlobalUnfix((HANDLE32)(handle))
#define     UnlockSegment WINELIB_NAME(UnlockSegment)
BOOL16      WINAPI UnrealizeObject16(HGDIOBJ16);
BOOL32      WINAPI UnrealizeObject32(HGDIOBJ32);
#define     UnrealizeObject WINELIB_NAME(UnrealizeObject)
BOOL16      WINAPI UnregisterClass16(SEGPTR,HINSTANCE16);
BOOL32      WINAPI UnregisterClass32A(LPCSTR,HINSTANCE32);
BOOL32      WINAPI UnregisterClass32W(LPCWSTR,HINSTANCE32);
#define     UnregisterClass WINELIB_NAME_AW(UnregisterClass)
INT16       WINAPI UpdateColors16(HDC16);
BOOL32      WINAPI UpdateColors32(HDC32);
#define     UpdateColors WINELIB_NAME(UpdateColors)
VOID        WINAPI UpdateWindow16(HWND16);
VOID        WINAPI UpdateWindow32(HWND32);
#define     UpdateWindow WINELIB_NAME(UpdateWindow)
VOID        WINAPI ValidateRect16(HWND16,const RECT16*);
VOID        WINAPI ValidateRect32(HWND32,const RECT32*);
#define     ValidateRect WINELIB_NAME(ValidateRect)
VOID        WINAPI ValidateRgn16(HWND16,HRGN16);
VOID        WINAPI ValidateRgn32(HWND32,HRGN32);
#define     ValidateRgn WINELIB_NAME(ValidateRgn)
DWORD       WINAPI VerFindFile16(UINT16,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16*,LPSTR,UINT16*);
DWORD       WINAPI VerFindFile32A(UINT32,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT32*,LPSTR,UINT32*);
DWORD       WINAPI VerFindFile32W(UINT32,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT32*,LPWSTR,UINT32*);
#define     VerFindFile WINELIB_NAME_AW(VerFindFile)
DWORD       WINAPI VerInstallFile16(UINT16,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16*);
DWORD       WINAPI VerInstallFile32A(UINT32,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT32*);
DWORD       WINAPI VerInstallFile32W(UINT32,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT32*);
#define     VerInstallFile WINELIB_NAME_AW(VerInstallFile)
DWORD       WINAPI VerLanguageName16(UINT16,LPSTR,UINT16);
DWORD       WINAPI VerLanguageName32A(UINT32,LPSTR,UINT32);
DWORD       WINAPI VerLanguageName32W(UINT32,LPWSTR,UINT32);
#define     VerLanguageName WINELIB_NAME_AW(VerLanguageName)
DWORD       WINAPI VerQueryValue16(SEGPTR,LPCSTR,SEGPTR*,UINT16*);
DWORD       WINAPI VerQueryValue32A(LPVOID,LPCSTR,LPVOID*,UINT32*);
DWORD       WINAPI VerQueryValue32W(LPVOID,LPCWSTR,LPVOID*,UINT32*);
#define     VerQueryValue WINELIB_NAME_AW(VerQueryValue)
WORD        WINAPI VkKeyScan16(CHAR);
WORD        WINAPI VkKeyScan32A(CHAR);
WORD        WINAPI VkKeyScan32W(WCHAR);
#define     VkKeyScan WINELIB_NAME_AW(VkKeyScan)
INT16       WINAPI WaitSoundState16(INT16);
DWORD       WINAPI WaitSoundState32(DWORD);
#define     WaitSoundState WINELIB_NAME(WaitSoundState)
HWND16      WINAPI WindowFromDC16(HDC16);
HWND32      WINAPI WindowFromDC32(HDC32);
#define     WindowFromDC WINELIB_NAME(WindowFromDC)
HWND16      WINAPI WindowFromPoint16(POINT16);
HWND32      WINAPI WindowFromPoint32(POINT32);
#define     WindowFromPoint WINELIB_NAME(WindowFromPoint)
BOOL16      WINAPI WritePrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WritePrivateProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WritePrivateProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
#define     WritePrivateProfileString WINELIB_NAME_AW(WritePrivateProfileString)
BOOL16      WINAPI WriteProfileString16(LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WriteProfileString32A(LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WriteProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR);
#define     WriteProfileString WINELIB_NAME_AW(WriteProfileString)
VOID        WINAPI Yield16(void);
#define     Yield32()
#define     Yield WINELIB_NAME(Yield)
SEGPTR      WINAPI lstrcat16(SEGPTR,LPCSTR);
LPSTR       WINAPI lstrcat32A(LPSTR,LPCSTR);
LPWSTR      WINAPI lstrcat32W(LPWSTR,LPCWSTR);
#define     lstrcat WINELIB_NAME_AW(lstrcat)
SEGPTR      WINAPI lstrcatn16(SEGPTR,LPCSTR,INT16);
LPSTR       WINAPI lstrcatn32A(LPSTR,LPCSTR,INT32);
LPWSTR      WINAPI lstrcatn32W(LPWSTR,LPCWSTR,INT32);
#define     lstrcatn WINELIB_NAME_AW(lstrcatn)
INT16       WINAPI lstrcmp16(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmp32A(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmp32W(LPCWSTR,LPCWSTR);
#define     lstrcmp WINELIB_NAME_AW(lstrcmp)
INT16       WINAPI lstrcmpi16(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmpi32A(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmpi32W(LPCWSTR,LPCWSTR);
#define     lstrcmpi WINELIB_NAME_AW(lstrcmpi)
SEGPTR      WINAPI lstrcpy16(SEGPTR,LPCSTR);
LPSTR       WINAPI lstrcpy32A(LPSTR,LPCSTR);
LPWSTR      WINAPI lstrcpy32W(LPWSTR,LPCWSTR);
#define     lstrcpy WINELIB_NAME_AW(lstrcpy)
SEGPTR      WINAPI lstrcpyn16(SEGPTR,LPCSTR,INT16);
LPSTR       WINAPI lstrcpyn32A(LPSTR,LPCSTR,INT32);
LPWSTR      WINAPI lstrcpyn32W(LPWSTR,LPCWSTR,INT32);
#define     lstrcpyn WINELIB_NAME_AW(lstrcpyn)
INT16       WINAPI lstrlen16(LPCSTR);
INT32       WINAPI lstrlen32A(LPCSTR);
INT32       WINAPI lstrlen32W(LPCWSTR);
#define     lstrlen WINELIB_NAME_AW(lstrlen)
HINSTANCE16 WINAPI WinExec16(LPCSTR,UINT16);
HINSTANCE32 WINAPI WinExec32(LPCSTR,UINT32);
#define     WinExec WINELIB_NAME(WinExec)
BOOL16      WINAPI WinHelp16(HWND16,LPCSTR,UINT16,DWORD);
BOOL32      WINAPI WinHelp32A(HWND32,LPCSTR,UINT32,DWORD);
BOOL32      WINAPI WinHelp32W(HWND32,LPCWSTR,UINT32,DWORD);
#define     WinHelp WINELIB_NAME_AW(WinHelp)
UINT16      WINAPI WNetAddConnection16(LPCSTR,LPCSTR,LPCSTR);
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
INT16       WINAPI wvsprintf16(LPSTR,LPCSTR,LPCVOID);
INT32       WINAPI wvsprintf32A(LPSTR,LPCSTR,va_list);
INT32       WINAPI wvsprintf32W(LPWSTR,LPCWSTR,va_list);
#define     wvsprintf WINELIB_NAME_AW(wvsprintf)
LONG        WINAPI _hread16(HFILE16,LPVOID,LONG);
LONG        WINAPI _hread32(HFILE32,LPVOID,LONG);
#define     _hread WINELIB_NAME(_hread)
LONG        WINAPI _hwrite16(HFILE16,LPCSTR,LONG);
LONG        WINAPI _hwrite32(HFILE32,LPCSTR,LONG);
#define     _hwrite WINELIB_NAME(_hwrite)
HFILE16     WINAPI _lcreat16(LPCSTR,INT16);
HFILE32     WINAPI _lcreat32(LPCSTR,INT32);
#define     _lcreat WINELIB_NAME(_lcreat)
HFILE16     WINAPI _lclose16(HFILE16);
HFILE32     WINAPI _lclose32(HFILE32);
#define     _lclose WINELIB_NAME(_lclose)
LONG        WINAPI _llseek16(HFILE16,LONG,INT16);
LONG        WINAPI _llseek32(HFILE32,LONG,INT32);
#define     _llseek WINELIB_NAME(_llseek)
HFILE16     WINAPI _lopen16(LPCSTR,INT16);
HFILE32     WINAPI _lopen32(LPCSTR,INT32);
#define     _lopen WINELIB_NAME(_lopen)
UINT16      WINAPI _lread16(HFILE16,LPVOID,UINT16);
UINT32      WINAPI _lread32(HFILE32,LPVOID,UINT32);
#define     _lread WINELIB_NAME(_lread)
UINT16      WINAPI _lwrite16(HFILE16,LPCSTR,UINT16);
UINT32      WINAPI _lwrite32(HFILE32,LPCSTR,UINT32);
#define     _lwrite WINELIB_NAME(_lwrite)

/* Extra functions that don't exist in the Windows API */

HPEN16      WINAPI GetSysColorPen16(INT16);
HPEN32      WINAPI GetSysColorPen32(INT32);
INT32       WINAPI LoadMessage32A(HMODULE32,UINT32,WORD,LPSTR,INT32);
INT32       WINAPI LoadMessage32W(HMODULE32,UINT32,WORD,LPWSTR,INT32);
SEGPTR      WINAPI WIN16_GlobalLock16(HGLOBAL16);
SEGPTR      WINAPI WIN16_LockResource16(HGLOBAL16);
LONG        WINAPI WIN16_hread(HFILE16,SEGPTR,LONG);
INT32       WINAPI lstrncmp32A(LPCSTR,LPCSTR,INT32);
INT32       WINAPI lstrncmp32W(LPCWSTR,LPCWSTR,INT32);
INT32       WINAPI lstrncmpi32A(LPCSTR,LPCSTR,INT32);
INT32       WINAPI lstrncmpi32W(LPCWSTR,LPCWSTR,INT32);
LPWSTR      WINAPI lstrcpyAtoW(LPWSTR,LPCSTR);
LPSTR       WINAPI lstrcpyWtoA(LPSTR,LPCWSTR);
LPWSTR      WINAPI lstrcpynAtoW(LPWSTR,LPCSTR,INT32);
LPSTR       WINAPI lstrcpynWtoA(LPSTR,LPCWSTR,INT32);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINDOWS_H */
