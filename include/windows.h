#ifndef __WINE_WINDOWS_H
#define __WINE_WINDOWS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#include "wintypes.h"
#include "winbase.h"  

/* FIXME: Maybe MAX_PATH and _MAX_PATH should be computed from the Unix headers instead
 * and by the way, _MAX_PATH should be defined in stdlib.h and MAX_PATH in windef.h 
 * and mapiwin.h
 */
#define _MAX_PATH 260
#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifndef DONT_INCLUDE_WINGDI
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#endif /* DONT_INCLUDE_WINGDI */

#pragma pack(1)

typedef struct tagCOORD {
    INT16 x;
    INT16 y;
} COORD, *LPCOORD;


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
} WINDOWPOS16, *LPWINDOWPOS16;

typedef struct
{
    HWND32  hwnd;
    HWND32  hwndInsertAfter;
    INT32   x;
    INT32   y;
    INT32   cx;
    INT32   cy;
    UINT32  flags;
} WINDOWPOS32, *LPWINDOWPOS32;

DECL_WINELIB_TYPE(WINDOWPOS)
DECL_WINELIB_TYPE(LPWINDOWPOS)

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
#define HTOBJECT            19
#define HTCLOSE             20
#define HTHELP              21
#define HTSIZEFIRST         HTLEFT
#define HTSIZELAST          HTBOTTOMRIGHT

  /* WM_SYSCOMMAND parameters */
#ifdef SC_SIZE /* at least HP-UX: already defined in /usr/include/sys/signal.h */
#undef SC_SIZE
#endif
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

#ifdef FSHIFT
/* Gcc on Solaris has a version of this that we don't care about.  */
#undef FSHIFT
#endif

#define	FVIRTKEY	TRUE          /* Assumed to be == TRUE */
#define	FNOINVERT	0x02
#define	FSHIFT		0x04
#define	FCONTROL	0x08
#define	FALT		0x10

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

/* CopyFileEx flags */
#define COPY_FILE_FAIL_IF_EXISTS        0x00000001
#define COPY_FILE_RESTARTABLE           0x00000002
#define COPY_FILE_OPEN_SOURCE_FOR_WRITE 0x00000004

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
} SECURITY_DESCRIPTOR, *PSECURITY_DESCRIPTOR, *LPSECURITY_DESCRIPTOR;

typedef DWORD SECURITY_INFORMATION;


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

/* Purge functions for Comm Port */
#define PURGE_TXABORT       0x0001  /* Kill the pending/current writes to the 
				       comm port */
#define PURGE_RXABORT       0x0002  /*Kill the pending/current reads to 
				     the comm port */
#define PURGE_TXCLEAR       0x0004  /* Kill the transmit queue if there*/
#define PURGE_RXCLEAR       0x0008  /* Kill the typeahead buffer if there*/


/* Modem Status Flags */
#define MS_CTS_ON           ((DWORD)0x0010)
#define MS_DSR_ON           ((DWORD)0x0020)
#define MS_RING_ON          ((DWORD)0x0040)
#define MS_RLSD_ON          ((DWORD)0x0080)

#define	RTS_CONTROL_DISABLE	0
#define	RTS_CONTROL_ENABLE	1
#define	RTS_CONTROL_HANDSHAKE	2
#define	RTS_CONTROL_TOGGLE	3

#define	DTR_CONTROL_DISABLE	0
#define	DTR_CONTROL_ENABLE	1
#define	DTR_CONTROL_HANDSHAKE	2

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
#define SPI_SETDRAGFULLWINDOWS    37
#define SPI_GETDRAGFULLWINDOWS	  38

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
#define IDC_SIZEALL16      MAKEINTRESOURCE16(32646)
#define IDC_SIZEALL32A     MAKEINTRESOURCE32A(32646)
#define IDC_SIZEALL32W     MAKEINTRESOURCE32W(32646)
#define IDC_SIZEALL        WINELIB_NAME_AW(IDC_SIZEALL)
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
#define WM_SETTINGCHANGE        WM_WININICHANGE
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
#define WM_DISPLAYCHANGE        0x007e
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
#define WM_MDIREFRESHMENU   0x0234

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

  /* MFC messages [370-37f] */

#define WM_QUERYAFXWNDPROC  0x0360
#define WM_SIZEPARENT       0x0361
#define WM_SETMESSAGESTRING 0x0362
#define WM_IDLEUPDATECMDUI  0x0363 
#define WM_INITIALUPDATE    0x0364
#define WM_COMMANDHELP      0x0365
#define WM_HELPHITTEST      0x0366
#define WM_EXITHELPMODE     0x0367
#define WM_RECALCPARENT     0x0368
#define WM_SIZECHILD        0x0369
#define WM_KICKIDLE         0x036A 
#define WM_QUERYCENTERWND   0x036B
#define WM_DISABLEMODAL     0x036C
#define WM_FLOATSTATUS      0x036D 
#define WM_ACTIVATETOPLEVEL 0x036E 
#define WM_QUERY3DCONTROLS  0x036F 
#define WM_SOCKET_NOTIFY    0x0373
#define WM_SOCKET_DEAD      0x0374
#define WM_POPMESSAGESTRING 0x0375
#define WM_OCC_LOADFROMSTREAM           0x0376
#define WM_OCC_LOADFROMSTORAGE          0x0377
#define WM_OCC_INITNEW                  0x0378
#define WM_OCC_LOADFROMSTREAM_EX        0x037A
#define WM_OCC_LOADFROMSTORAGE_EX       0x037B
#define WM_QUEUE_SENTINEL   0x0379

/* end of MFC messages */

#define WM_COALESCE_FIRST    0x0390
#define WM_COALESCE_LAST     0x039F

  /* misc messages */
#define WM_NULL             0x0000
#define WM_CPL_LAUNCH       (WM_USER + 1000)
#define WM_CPL_LAUNCHED     (WM_USER + 1001)

/* WM_NOTIFYFORMAT commands and return values */
#define NFR_ANSI	    1
#define NFR_UNICODE	    2
#define NF_QUERY	    3
#define NF_REQUERY	    4

  /* Key status flags for mouse events */
#define MK_LBUTTON	    0x0001
#define MK_RBUTTON	    0x0002
#define MK_SHIFT	    0x0004
#define MK_CONTROL	    0x0008
#define MK_MBUTTON	    0x0010

  /* keybd_event flags */
#define KEYEVENTF_EXTENDEDKEY        0x0001
#define KEYEVENTF_KEYUP              0x0002
#define KEYEVENTF_WINE_FORCEEXTENDED 0x8000

  /* mouse_event flags */
#define MOUSEEVENTF_MOVE        0x0001
#define MOUSEEVENTF_LEFTDOWN    0x0002
#define MOUSEEVENTF_LEFTUP      0x0004
#define MOUSEEVENTF_RIGHTDOWN   0x0008
#define MOUSEEVENTF_RIGHTUP     0x0010
#define MOUSEEVENTF_MIDDLEDOWN  0x0020
#define MOUSEEVENTF_MIDDLEUP    0x0040
#define MOUSEEVENTF_ABSOLUTE    0x8000

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

/* DrawCaption()/DrawCaptionTemp() flags */
#define DC_ACTIVE		0x0001
#define DC_SMALLCAP		0x0002
#define DC_ICON			0x0004
#define DC_TEXT			0x0008
#define DC_INBUTTON		0x0010

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

/* New Win95/WinNT4 styles */
#define WS_EX_MDICHILD         0x00000040L
#define WS_EX_TOOLWINDOW       0x00000080L
#define WS_EX_WINDOWEDGE       0x00000100L
#define WS_EX_CLIENTEDGE       0x00000200L
#define WS_EX_CONTEXTHELP      0x00000400L
#define WS_EX_RIGHT            0x00001000L
#define WS_EX_LEFT             0x00000000L
#define WS_EX_RTLREADING       0x00002000L
#define WS_EX_LTRREADING       0x00000000L
#define WS_EX_LEFTSCROLLBAR    0x00004000L
#define WS_EX_RIGHTSCROLLBAR   0x00000000L
#define WS_EX_CONTROLPARENT    0x00010000L
#define WS_EX_STATICEDGE       0x00020000L
#define WS_EX_APPWINDOW        0x00040000L

#define WS_EX_OVERLAPPEDWINDOW (WS_EX_WINDOWEDGE|WS_EX_CLIENTEDGE)
#define WS_EX_PALETTEWINDOW    (WS_EX_WINDOWEDGE|WS_EX_TOOLWINDOW|WS_EX_TOPMOST)

/* Window scrolling */
#define SW_SCROLLCHILDREN      0x0001
#define SW_INVALIDATE          0x0002
#define SW_ERASE               0x0004

/* CreateWindow() coordinates */
#define CW_USEDEFAULT16 ((INT16)0x8000)
#define CW_USEDEFAULT32 ((INT32)0x80000000)
#define CW_USEDEFAULT   WINELIB_NAME(CW_USEDEFAULT)

/* ChildWindowFromPointEx Flags */
#define CWP_ALL                0x0000
#define CWP_SKIPINVISIBLE      0x0001
#define CWP_SKIPDISABLED       0x0002
#define CWP_SKIPTRANSPARENT    0x0004

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

#define LPTR (LMEM_FIXED | LMEM_ZEROINIT)

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

/* STARTUPINFO.dwFlags */
#define	STARTF_USESHOWWINDOW	0x00000001
#define	STARTF_USESIZE		0x00000002
#define	STARTF_USEPOSITION	0x00000004
#define	STARTF_USECOUNTCHARS	0x00000008
#define	STARTF_USEFILLATTRIBUTE	0x00000010
#define	STARTF_RUNFULLSCREEN	0x00000020
#define	STARTF_FORCEONFEEDBACK	0x00000040
#define	STARTF_FORCEOFFFEEDBACK	0x00000080
#define	STARTF_USESTDHANDLES	0x00000100
#define	STARTF_USEHOTKEY	0x00000200

typedef struct {
        DWORD cb;		/* 00: size of struct */
        LPSTR lpReserved;	/* 04: */
        LPSTR lpDesktop;	/* 08: */
        LPSTR lpTitle;		/* 0c: */
        DWORD dwX;		/* 10: */
        DWORD dwY;		/* 14: */
        DWORD dwXSize;		/* 18: */
        DWORD dwYSize;		/* 1c: */
        DWORD dwXCountChars;	/* 20: */
        DWORD dwYCountChars;	/* 24: */
        DWORD dwFillAttribute;	/* 28: */
        DWORD dwFlags;		/* 2c: */
        WORD wShowWindow;	/* 30: */
        WORD cbReserved2;	/* 32: */
        BYTE *lpReserved2;	/* 34: */
        HANDLE32 hStdInput;	/* 38: */
        HANDLE32 hStdOutput;	/* 3c: */
        HANDLE32 hStdError;	/* 40: */
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

/* CreateProcess: dwCreationFlag values
 */
#define DEBUG_PROCESS               0x00000001
#define DEBUG_ONLY_THIS_PROCESS     0x00000002
#define CREATE_SUSPENDED            0x00000004
#define DETACHED_PROCESS            0x00000008
#define CREATE_NEW_CONSOLE          0x00000010
#define NORMAL_PRIORITY_CLASS       0x00000020
#define IDLE_PRIORITY_CLASS         0x00000040
#define HIGH_PRIORITY_CLASS         0x00000080
#define REALTIME_PRIORITY_CLASS     0x00000100
#define CREATE_NEW_PROCESS_GROUP    0x00000200
#define CREATE_UNICODE_ENVIRONMENT  0x00000400
#define CREATE_SEPARATE_WOW_VDM     0x00000800
#define CREATE_SHARED_WOW_VDM       0x00001000
#define CREATE_DEFAULT_ERROR_MODE   0x04000000
#define CREATE_NO_WINDOW            0x08000000
#define PROFILE_USER                0x10000000
#define PROFILE_KERNEL              0x20000000
#define PROFILE_SERVER              0x40000000


/* File object type definitions
 */
#define FILE_TYPE_UNKNOWN       0
#define FILE_TYPE_DISK          1
#define FILE_TYPE_CHAR          2
#define FILE_TYPE_PIPE          3
#define FILE_TYPE_REMOTE        32768

/* File creation flags
 */
#define FILE_FLAG_WRITE_THROUGH    0x80000000UL
#define FILE_FLAG_OVERLAPPED 	   0x40000000L
#define FILE_FLAG_NO_BUFFERING     0x20000000L
#define FILE_FLAG_RANDOM_ACCESS    0x10000000L
#define FILE_FLAG_SEQUENTIAL_SCAN  0x08000000L
#define FILE_FLAG_DELETE_ON_CLOSE  0x04000000L
#define FILE_FLAG_BACKUP_SEMANTICS 0x02000000L
#define FILE_FLAG_POSIX_SEMANTICS  0x01000000L
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
#define FILE_SHARE_READ			0x00000001L
#define FILE_SHARE_WRITE		0x00000002L
#define FILE_SHARE_DELETE		0x00000004L
#define FILE_ATTRIBUTE_READONLY         0x00000001L
#define FILE_ATTRIBUTE_HIDDEN           0x00000002L
#define FILE_ATTRIBUTE_SYSTEM           0x00000004L
#define FILE_ATTRIBUTE_LABEL            0x00000008L  /* Not in Windows API */
#define FILE_ATTRIBUTE_DIRECTORY        0x00000010L
#define FILE_ATTRIBUTE_ARCHIVE          0x00000020L
#define FILE_ATTRIBUTE_NORMAL           0x00000080L
#define FILE_ATTRIBUTE_TEMPORARY        0x00000100L
#define FILE_ATTRIBUTE_ATOMIC_WRITE     0x00000200L
#define FILE_ATTRIBUTE_XACTION_WRITE    0x00000400L
#define FILE_ATTRIBUTE_COMPRESSED       0x00000800L
#define FILE_ATTRIBUTE_OFFLINE		0x00001000L

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


#define DM_UPDATE	1
#define DM_COPY		2
#define DM_PROMPT	4
#define DM_MODIFY	8

#define DM_IN_BUFFER      DM_MODIFY
#define DM_IN_PROMPT      DM_PROMPT
#define DM_OUT_BUFFER     DM_COPY
#define DM_OUT_DEFAULT    DM_UPDATE

#define DM_ORIENTATION		0x00000001L
#define DM_PAPERSIZE		0x00000002L
#define DM_PAPERLENGTH		0x00000004L
#define DM_PAPERWIDTH		0x00000008L
#define DM_SCALE		0x00000010L
#define DM_COPIES		0x00000100L
#define DM_DEFAULTSOURCE	0x00000200L
#define DM_PRINTQUALITY		0x00000400L
#define DM_COLOR		0x00000800L
#define DM_DUPLEX		0x00001000L
#define DM_YRESOLUTION		0x00002000L
#define DM_TTOPTION		0x00004000L
#define DM_BITSPERPEL           0x00040000L
#define DM_PELSWIDTH            0x00080000L
#define DM_PELSHEIGHT           0x00100000L
#define DM_DISPLAYFLAGS         0x00200000L
#define DM_DISPLAYFREQUENCY     0x00400000L

/* etc.... */

#define DMORIENT_PORTRAIT	1
#define DMORIENT_LANDSCAPE	2

#define DMPAPER_LETTER		1
#define DMPAPER_LEGAL		5
#define DMPAPER_EXECUTIVE	7
#define DMPAPER_A3		8
#define DMPAPER_A4		9
#define DMPAPER_A5		11
#define DMPAPER_ENV_10		20
#define DMPAPER_ENV_DL		27
#define DMPAPER_ENV_C5		28
#define DMPAPER_ENV_B5		34
#define DMPAPER_ENV_MONARCH	37

#define DMBIN_UPPER		1
#define DMBIN_LOWER		2
#define DMBIN_MIDDLE		3
#define DMBIN_MANUAL		4
#define DMBIN_ENVELOPE		5
#define DMBIN_ENVMANUAL		6
#define DMBIN_AUTO		7
#define DMBIN_LARGECAPACITY	11

#define DMCOLOR_MONOCHROME	1
#define DMCOLOR_COLOR		2

#define DMTT_BITMAP		1
#define DMTT_DOWNLOAD		2
#define DMTT_SUBDEV		3


#define DC_FIELDS		1
#define DC_PAPERS		2
#define DC_PAPERSIZE		3
#define DC_MINEXTENT		4
#define DC_MAXEXTENT		5
#define DC_BINS			6
#define DC_DUPLEX		7
#define DC_SIZE			8
#define DC_EXTRA		9
#define DC_VERSION		10
#define DC_DRIVER		11
#define DC_BINNAMES		12
#define DC_ENUMRESOLUTIONS	13
#define DC_FILEDEPENDENCIES	14
#define DC_TRUETYPE		15
#define DC_PAPERNAMES		16
#define DC_ORIENTATION		17
#define DC_COPIES		18

     /* ChangeDisplaySettings return codes */

#define DISP_CHANGE_SUCCESSFUL 0
#define DISP_CHANGE_RESTART    1
#define DISP_CHANGE_FAILED     (-1)
#define DISP_CHANGE_BADMODE    (-2)
#define DISP_CHANGE_NOTUPDATED (-3)
#define DISP_CHANGE_BADFLAGS   (-4)


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
#define LR_COLOR		0x0002
#define LR_COPYRETURNORG	0x0004
#define LR_COPYDELETEORG	0x0008
#define LR_LOADFROMFILE		0x0010
#define LR_LOADTRANSPARENT	0x0020
#define LR_DEFAULTSIZE		0x0040
#define LR_VGA_COLOR		0x0080
#define LR_LOADMAP3DCOLORS	0x1000
#define	LR_CREATEDIBSECTION	0x2000
#define LR_COPYFROMRESOURCE	0x4000
#define LR_SHARED		0x8000

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
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND32 hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCE32A lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCT32A, *LPCONNECTDLGSTRUCT32A;
typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND32 hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCE32W lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCT32W, *LPCONNECTDLGSTRUCT32W;

DECL_WINELIB_TYPE_AW(CONNECTDLGSTRUCT)
DECL_WINELIB_TYPE_AW(LPCONNECTDLGSTRUCT)

/**/
#define CONNDLG_RO_PATH     0x00000001 /* Resource path should be read-only    */
#define CONNDLG_CONN_POINT  0x00000002 /* Netware -style movable connection point enabled */
#define CONNDLG_USE_MRU     0x00000004 /* Use MRU combobox  */
#define CONNDLG_HIDE_BOX    0x00000008 /* Hide persistent connect checkbox  */
#define CONNDLG_PERSIST     0x00000010 /* Force persistent connection */
#define CONNDLG_NOT_PERSIST 0x00000020 /* Force connection NOT persistent */


typedef struct {
	DWORD	cbStructure;
	DWORD	dwFlags;
	DWORD	dwSpeed;
	DWORD	dwDelay;
	DWORD	dwOptDataSize;
} NETCONNECTINFOSTRUCT,*LPNETCONNECTINFOSTRUCT;


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

typedef struct _DllVersionInfo {
    DWORD cbSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformID;
} DLLVERSIONINFO;

typedef struct _SEGINFO {
    UINT16    offSegment;
    UINT16    cbSegment;
    UINT16    flags;
    UINT16    cbAlloc;
    HGLOBAL16 h;
    UINT16    alignShift;
    UINT16    reserved[2];
} SEGINFO;

typedef struct tagDLGTEMPLATE
{
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
}DLGTEMPLATE, *LPDLGTEMPLATE;
typedef const DLGTEMPLATE *LPCDLGTEMPLATE;
/* Fixme: use this instaed of LPCVOID for CreateDialogIndirectParam and DialogBoxIndirectParam*/
typedef struct tagDLGITEMTEMPLATE
{
    DWORD style;
    DWORD dwExtendedStyle;
    WORD cdit;
    short x;
    short y;
    short cx;
    short cy;
    WORD id;
}DLGITEMTEMPLATE, *LPDLGITEMTEMPLATE;
typedef const DLGITEMTEMPLATE *LPCDLGITEMTEMPLATE;

/*
 * This one seems to be a Win32 only definition. It also is defined with
 * WINAPI instead of CALLBACK in the windows headers.
 */
typedef DWORD (WINAPI *LPPROGRESS_ROUTINE)(LARGE_INTEGER, LARGE_INTEGER, LARGE_INTEGER, 
                                           LARGE_INTEGER, DWORD, DWORD, HANDLE32,
                                           HANDLE32, LPVOID);

#pragma pack(4)

/* Declarations for functions that exist only in Win16 */

#ifdef __WINE__
typedef VOID (*SYSTEMTIMERPROC)(WORD);

WORD        WINAPI AllocCStoDSAlias(WORD);
WORD        WINAPI AllocDStoCSAlias(WORD);
HGLOBAL16   WINAPI AllocResource(HINSTANCE16,HRSRC16,DWORD);
WORD        WINAPI AllocSelector(WORD);
WORD        WINAPI AllocSelectorArray(WORD);
WORD        WINAPI CreateSystemTimer(WORD,SYSTEMTIMERPROC);
VOID        WINAPI DirectedYield(HTASK16);
HGLOBAL16   WINAPI DirectResAlloc(HINSTANCE16,WORD,UINT16);
VOID        WINAPI DisableSystemTimers(void);
VOID        WINAPI EnableSystemTimers(void);
HANDLE16    WINAPI FarGetOwner(HGLOBAL16);
VOID        WINAPI FarSetOwner(HGLOBAL16,HANDLE16);
FARPROC16   WINAPI FileCDR(FARPROC16);
WORD        WINAPI FreeSelector(WORD);
HANDLE16    WINAPI GetAtomHandle(ATOM);
HANDLE16    WINAPI GetCodeHandle(FARPROC16);
VOID        WINAPI GetCodeInfo(FARPROC16,SEGINFO*);
DWORD       WINAPI GetCurrentPDB(void);
HTASK16     WINAPI GetCurrentTask(void);
SEGPTR      WINAPI GetDOSEnvironment(void);
HMODULE16   WINAPI GetExePtr(HANDLE16);
WORD        WINAPI GetExeVersion(void);
WORD        WINAPI GetExpWinVer(HMODULE16);
DWORD       WINAPI GetFileResourceSize(LPCSTR,SEGPTR,SEGPTR,LPDWORD);
DWORD       WINAPI GetFileResource(LPCSTR,SEGPTR,SEGPTR,DWORD,DWORD,LPVOID);
DWORD       WINAPI GetHeapSpaces(HMODULE16);
INT16       WINAPI GetInstanceData(HINSTANCE16,WORD,INT16);
BOOL16      WINAPI GetModuleName(HINSTANCE16,LPSTR,INT16);
INT16       WINAPI GetModuleUsage(HINSTANCE16);
UINT16      WINAPI GetNumTasks(void);
DWORD       WINAPI GetSelectorBase(WORD);
DWORD       WINAPI GetSelectorLimit(WORD);
FARPROC16   WINAPI GetSetKernelDOSProc(FARPROC16 DosProc);
HINSTANCE16 WINAPI GetTaskDS(void);
HQUEUE16    WINAPI GetTaskQueue(HTASK16);
BYTE        WINAPI GetTempDrive(BYTE);
BOOL16      WINAPI GetWinDebugInfo(LPWINDEBUGINFO,UINT16);
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
BOOL16      WINAPI IsSharedSelector(HANDLE16);
BOOL16      WINAPI IsTask(HTASK16);
HTASK16     WINAPI IsTaskLocked(void);
VOID        WINAPI LogError(UINT16, LPVOID);
VOID        WINAPI LogParamError(UINT16,FARPROC16,LPVOID);
WORD        WINAPI LocalCountFree(void);
WORD        WINAPI LocalHandleDelta(WORD);
WORD        WINAPI LocalHeapSize(void);
BOOL16      WINAPI LocalInit(HANDLE16,WORD,WORD);
HMODULE32   WINAPI LoadLibraryEx32W16(LPCSTR,HANDLE16,DWORD);
FARPROC16   WINAPI LocalNotify(FARPROC16);
HTASK16     WINAPI LockCurrentTask(BOOL16);
VOID        WINAPI OldYield(void);
VOID        WINAPI PostEvent(HTASK16);
WORD        WINAPI PrestoChangoSelector(WORD,WORD);
BOOL32      WINAPI RegisterShellHook(HWND16,UINT16);
/* NOTE: This is SYSTEM.3, not USER.182, which is also named KillSystemTimer */
WORD        WINAPI SYSTEM_KillSystemTimer( WORD );
WORD        WINAPI SelectorAccessRights(WORD,WORD,WORD);
VOID        WINAPI SetPriority(HTASK16,INT16);
FARPROC16   WINAPI SetResourceHandler(HINSTANCE16,SEGPTR,FARPROC16);
WORD        WINAPI SetSelectorBase(WORD,DWORD);
WORD        WINAPI SetSelectorLimit(WORD,DWORD);
HQUEUE16    WINAPI SetTaskQueue(HTASK16,HQUEUE16);
FARPROC16   WINAPI SetTaskSignalProc(HTASK16,FARPROC16);
BOOL16      WINAPI SetWinDebugInfo(LPWINDEBUGINFO);
VOID        WINAPI SwitchStackTo(WORD,WORD,WORD);
BOOL16      WINAPI WaitEvent(HTASK16);
VOID        WINAPI WriteOutProfiles(VOID);
VOID        WINAPI hmemcpy(LPVOID,LPCVOID,LONG);
#endif  /* __WINE__ */

/* Declarations for functions that exist only in Win32 */

BOOL32      WINAPI AllocConsole(void);
BOOL32      WINAPI AreFileApisANSI(void);
BOOL32      WINAPI Beep(DWORD,DWORD);
BOOL32      WINAPI CloseHandle(HANDLE32);
BOOL32      WINAPI CloseServiceHandle(HANDLE32);
HANDLE32    WINAPI ConvertToGlobalHandle(HANDLE32 hSrc);
BOOL32      WINAPI CopyFile32A(LPCSTR,LPCSTR,BOOL32);
BOOL32      WINAPI CopyFile32W(LPCWSTR,LPCWSTR,BOOL32);
#define     CopyFile WINELIB_NAME_AW(CopyFile)
BOOL32      WINAPI CopyFileEx32A(LPCSTR, LPCSTR, LPPROGRESS_ROUTINE, LPVOID, LPBOOL32, DWORD);
BOOL32      WINAPI CopyFileEx32W(LPCWSTR, LPCWSTR, LPPROGRESS_ROUTINE, LPVOID, LPBOOL32, DWORD);
#define     CopyFileEx WINELIB_NAME_AW(CopyFileEx)
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
HANDLE32    WINAPI CreateFileMapping32A(HANDLE32,LPSECURITY_ATTRIBUTES,DWORD,
                                        DWORD,DWORD,LPCSTR);
HANDLE32    WINAPI CreateFileMapping32W(HANDLE32,LPSECURITY_ATTRIBUTES,DWORD,
                                        DWORD,DWORD,LPCWSTR);
#define     CreateFileMapping WINELIB_NAME_AW(CreateFileMapping)
HANDLE32    WINAPI CreateMutex32A(LPSECURITY_ATTRIBUTES,BOOL32,LPCSTR);
HANDLE32    WINAPI CreateMutex32W(LPSECURITY_ATTRIBUTES,BOOL32,LPCWSTR);
#define     CreateMutex WINELIB_NAME_AW(CreateMutex)
BOOL32      WINAPI CreateProcess32A(LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,
                                    LPSECURITY_ATTRIBUTES,BOOL32,DWORD,LPVOID,LPCSTR,
                                    LPSTARTUPINFO32A,LPPROCESS_INFORMATION);
BOOL32      WINAPI CreateProcess32W(LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,
                                    LPSECURITY_ATTRIBUTES,BOOL32,DWORD,LPVOID,LPCWSTR,
                                    LPSTARTUPINFO32W,LPPROCESS_INFORMATION);
#define     CreateProcess WINELIB_NAME_AW(CreateProcess)
HANDLE32    WINAPI CreateSemaphore32A(LPSECURITY_ATTRIBUTES,LONG,LONG,LPCSTR);
HANDLE32    WINAPI CreateSemaphore32W(LPSECURITY_ATTRIBUTES,LONG,LONG,LPCWSTR);
#define     CreateSemaphore WINELIB_NAME_AW(CreateSemaphore)
HANDLE32    WINAPI CreateThread(LPSECURITY_ATTRIBUTES,DWORD,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD);
BOOL32      WINAPI DeleteService(HANDLE32);
BOOL32      WINAPI DeregisterEventSource(HANDLE32);
BOOL32      WINAPI DisableThreadLibraryCalls(HMODULE32);
BOOL32      WINAPI DosDateTimeToFileTime(WORD,WORD,LPFILETIME);
BOOL32      WINAPI DuplicateHandle(HANDLE32,HANDLE32,HANDLE32,HANDLE32*,DWORD,BOOL32,DWORD);
BOOL32      WINAPI EnumDateFormats32A(DATEFMT_ENUMPROC32A lpDateFmtEnumProc, LCID Locale, DWORD dwFlags);
BOOL32      WINAPI EnumDateFormats32W(DATEFMT_ENUMPROC32W lpDateFmtEnumProc, LCID Locale, DWORD dwFlags);
#define     EnumDateFormats WINELIB_NAME_AW(EnumDateFormats)
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
BOOL32      WINAPI EnumTimeFormats32A(TIMEFMT_ENUMPROC32A lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags);
BOOL32      WINAPI EnumTimeFormats32W(TIMEFMT_ENUMPROC32W lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags);
#define     EnumTimeFormats WINELIB_NAME_AW(EnumTimeFormats)
VOID        WINAPI ExitProcess(DWORD);
VOID        WINAPI ExitThread(DWORD);
DWORD       WINAPI ExpandEnvironmentStrings32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI ExpandEnvironmentStrings32W(LPCWSTR,LPWSTR,DWORD);
#define     ExpandEnvironmentStrings WINELIB_NAME_AW(ExpandEnvironmentStrings)
BOOL32      WINAPI FileTimeToDosDateTime(const FILETIME*,LPWORD,LPWORD);
BOOL32      WINAPI FileTimeToLocalFileTime(const FILETIME*,LPFILETIME);
BOOL32      WINAPI FileTimeToSystemTime(const FILETIME*,LPSYSTEMTIME);
HANDLE32    WINAPI FindFirstChangeNotification32A(LPCSTR,BOOL32,DWORD);
HANDLE32    WINAPI FindFirstChangeNotification32W(LPCWSTR,BOOL32,DWORD);
#define     FindFirstChangeNotification WINELIB_NAME_AW(FindFirstChangeNotification)
BOOL32      WINAPI FindNextChangeNotification(HANDLE32);
BOOL32      WINAPI FindCloseChangeNotification(HANDLE32);
HRSRC32     WINAPI FindResourceEx32A(HMODULE32,LPCSTR,LPCSTR,WORD);
HRSRC32     WINAPI FindResourceEx32W(HMODULE32,LPCWSTR,LPCWSTR,WORD);
#define     FindResourceEx WINELIB_NAME_AW(FindResourceEx)
BOOL32      WINAPI FlushConsoleInputBuffer(HANDLE32);
BOOL32      WINAPI FlushFileBuffers(HFILE32);
BOOL32      WINAPI FlushViewOfFile(LPCVOID, DWORD);
DWORD       WINAPI FormatMessage32A(DWORD,LPCVOID,DWORD,DWORD,LPSTR,
				    DWORD,LPDWORD);
#define     FormatMessage WINELIB_NAME_AW(FormatMessage)
BOOL32      WINAPI FreeConsole(void);
BOOL32      WINAPI FreeEnvironmentStrings32A(LPSTR);
BOOL32      WINAPI FreeEnvironmentStrings32W(LPWSTR);
#define     FreeEnvironmentStrings WINELIB_NAME_AW(FreeEnvironmentStrings)
UINT32      WINAPI GetACP(void);
LPCSTR      WINAPI GetCommandLine32A(void);
LPCWSTR     WINAPI GetCommandLine32W(void);
#define     GetCommandLine WINELIB_NAME_AW(GetCommandLine)
BOOL32      WINAPI GetComputerName32A(LPSTR,LPDWORD);
BOOL32      WINAPI GetComputerName32W(LPWSTR,LPDWORD);
#define     GetComputerName WINELIB_NAME_AW(GetComputerName)
UINT32      WINAPI GetConsoleCP(void);
BOOL32      WINAPI GetConsoleMode(HANDLE32,LPDWORD);
UINT32      WINAPI GetConsoleOutputCP(void);
DWORD       WINAPI GetConsoleTitle32A(LPSTR,DWORD);
DWORD       WINAPI GetConsoleTitle32W(LPWSTR,DWORD);
#define     GetConsoleTitle WINELIB_NAME_AW(GetConsoleTitle)
BOOL32      WINAPI GetCPInfo(UINT32,LPCPINFO);
BOOL32      WINAPI GetCommMask(HANDLE32, LPDWORD);
BOOL32      WINAPI GetCommModemStatus(HANDLE32, LPDWORD);
HANDLE32    WINAPI GetCurrentProcess(void);
DWORD       WINAPI GetCurrentProcessId(void);
HANDLE32    WINAPI GetCurrentThread(void);
DWORD       WINAPI GetCurrentThreadId(void);
INT32       WINAPI GetDateFormat32A(LCID,DWORD,LPSYSTEMTIME,LPCSTR,LPSTR,INT32);
INT32       WINAPI GetDateFormat32W(LCID,DWORD,LPSYSTEMTIME,LPCWSTR,LPWSTR,INT32);
#define     GetDateFormat WINELIB_NAME_AW(GetDateFormat)
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
BOOL32      WINAPI GetFileSecurity32A(LPCSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR,DWORD,LPDWORD);
BOOL32      WINAPI GetFileSecurity32W(LPCWSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR,DWORD,LPDWORD);
#define     GetFileSecurity WINELIB_NAME_AW(GetFileSecurity)
DWORD       WINAPI GetFileSize(HFILE32,LPDWORD);
BOOL32      WINAPI GetFileTime(HFILE32,LPFILETIME,LPFILETIME,LPFILETIME);
DWORD       WINAPI GetFileType(HFILE32);
DWORD       WINAPI GetFullPathName32A(LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD       WINAPI GetFullPathName32W(LPCWSTR,DWORD,LPWSTR,LPWSTR*);
#define     GetFullPathName WINELIB_NAME_AW(GetFullPathName)
BOOL32      WINAPI GetHandleInformation(HANDLE32,LPDWORD);
DWORD       WINAPI GetLargestConsoleWindowSize(HANDLE32);
VOID        WINAPI GetLocalTime(LPSYSTEMTIME);
DWORD       WINAPI GetLogicalDrives(void);
DWORD       WINAPI GetLongPathName32A(LPCSTR,LPSTR,DWORD);
DWORD       WINAPI GetLongPathName32W(LPCWSTR,LPWSTR,DWORD);
#define     GetLongPathName WINELIB_NAME_AW(GetLongPathName)
BOOL32      WINAPI GetNumberOfConsoleInputEvents(HANDLE32,LPDWORD);
BOOL32      WINAPI GetNumberOfConsoleMouseButtons(LPDWORD);
UINT32      WINAPI GetOEMCP(void);
DWORD       WINAPI GetPriorityClass(HANDLE32);
HANDLE32    WINAPI GetProcessHeap(void);
DWORD       WINAPI GetProcessVersion(DWORD);
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
INT32       WINAPI GetTimeFormat32A(LCID,DWORD,LPSYSTEMTIME,LPCSTR,LPSTR,INT32);
INT32       WINAPI GetTimeFormat32W(LCID,DWORD,LPSYSTEMTIME,LPCWSTR,LPWSTR,INT32);
#define     GetTimeFormat WINELIB_NAME_AW(GetTimeFormat)
LCID        WINAPI GetThreadLocale(void);
INT32       WINAPI GetThreadPriority(HANDLE32);
BOOL32      WINAPI GetThreadSelectorEntry(HANDLE32,DWORD,LPLDT_ENTRY);
BOOL32      WINAPI GetUserName32A(LPSTR,LPDWORD);
BOOL32      WINAPI GetUserName32W(LPWSTR,LPDWORD);
#define     GetUserName WINELIB_NAME_AW(GetUserName)
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
BOOL32      WINAPI IsValidLocale(DWORD,DWORD);
BOOL32      WINAPI LocalFileTimeToFileTime(const FILETIME*,LPFILETIME);
BOOL32      WINAPI LockFile(HFILE32,DWORD,DWORD,DWORD,DWORD);
BOOL32      WINAPI LookupPrivilegeValue32A(LPCSTR,LPCSTR,LPVOID);
BOOL32      WINAPI LookupPrivilegeValue32W(LPCWSTR,LPCWSTR,LPVOID);
#define     LookupPrivilegeValue WINELIB_NAME_AW(LookupPrivilegeValue)
HMODULE32   WINAPI MapHModuleSL(HMODULE16);
HMODULE16   WINAPI MapHModuleLS(HMODULE32);
SEGPTR      WINAPI MapLS(LPVOID);
LPVOID      WINAPI MapSL(SEGPTR);
LPVOID      WINAPI MapViewOfFile(HANDLE32,DWORD,DWORD,DWORD,DWORD);
LPVOID      WINAPI MapViewOfFileEx(HANDLE32,DWORD,DWORD,DWORD,DWORD,LPVOID);
BOOL32      WINAPI MoveFile32A(LPCSTR,LPCSTR);
BOOL32      WINAPI MoveFile32W(LPCWSTR,LPCWSTR);
#define     MoveFile WINELIB_NAME_AW(MoveFile)
BOOL32      WINAPI MoveFileEx32A(LPCSTR,LPCSTR,DWORD);
BOOL32      WINAPI MoveFileEx32W(LPCWSTR,LPCWSTR,DWORD);
#define     MoveFileEx WINELIB_NAME_AW(MoveFileEx)
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
BOOL32      WINAPI PulseEvent(HANDLE32);
BOOL32      WINAPI PurgeComm(HANDLE32,DWORD);
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
BOOL32      WINAPI ReleaseMutex(HANDLE32);
BOOL32      WINAPI ReleaseSemaphore(HANDLE32,LONG,LPLONG);
BOOL32      WINAPI ResetEvent(HANDLE32);
DWORD       WINAPI ResumeThread(HANDLE32);
VOID        WINAPI RtlFillMemory(LPVOID,UINT32,UINT32);
VOID        WINAPI RtlMoveMemory(LPVOID,LPCVOID,UINT32);
VOID        WINAPI RtlZeroMemory(LPVOID,UINT32);
DWORD       WINAPI SearchPath32A(LPCSTR,LPCSTR,LPCSTR,DWORD,LPSTR,LPSTR*);
DWORD       WINAPI SearchPath32W(LPCWSTR,LPCWSTR,LPCWSTR,DWORD,LPWSTR,LPWSTR*);
#define     SearchPath WINELIB_NAME(SearchPath)
BOOL32      WINAPI SetCommMask(INT32,DWORD);
BOOL32      WINAPI SetComputerName32A(LPCSTR);
BOOL32      WINAPI SetComputerName32W(LPCWSTR);
#define     SetComputerName WINELIB_NAME_AW(SetComputerName)
BOOL32      WINAPI SetConsoleCursorPosition(HANDLE32,COORD);
BOOL32      WINAPI SetConsoleMode(HANDLE32,DWORD);
BOOL32      WINAPI SetConsoleTitle32A(LPCSTR);
BOOL32      WINAPI SetConsoleTitle32W(LPCWSTR);
#define     SetConsoleTitle WINELIB_NAME_AW(SetConsoleTitle)
BOOL32      WINAPI SetEndOfFile(HFILE32);
BOOL32      WINAPI SetEnvironmentVariable32A(LPCSTR,LPCSTR);
BOOL32      WINAPI SetEnvironmentVariable32W(LPCWSTR,LPCWSTR);
#define     SetEnvironmentVariable WINELIB_NAME_AW(SetEnvironmentVariable)
BOOL32      WINAPI SetEvent(HANDLE32);
VOID        WINAPI SetFileApisToANSI(void);
VOID        WINAPI SetFileApisToOEM(void);
DWORD       WINAPI SetFilePointer(HFILE32,LONG,LPLONG,DWORD);
BOOL32      WINAPI SetFileSecurity32A(LPCSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR);
BOOL32      WINAPI SetFileSecurity32W(LPCWSTR,SECURITY_INFORMATION,LPSECURITY_DESCRIPTOR);
#define     SetFileSecurity WINELIB_NAME_AW(SetFileSecurity)
BOOL32      WINAPI SetFileTime(HFILE32,const FILETIME*,const FILETIME*,
                               const FILETIME*);
BOOL32      WINAPI SetHandleInformation(HANDLE32,DWORD,DWORD);
BOOL32      WINAPI SetPriorityClass(HANDLE32,DWORD);
BOOL32      WINAPI SetStdHandle(DWORD,HANDLE32);
BOOL32      WINAPI SetSystemPowerState(BOOL32,BOOL32);
BOOL32      WINAPI SetSystemTime(const SYSTEMTIME*);
DWORD       WINAPI SetThreadAffinityMask(HANDLE32,DWORD);
BOOL32      WINAPI SetThreadPriority(HANDLE32,INT32);
BOOL32      WINAPI SetTimeZoneInformation(const LPTIME_ZONE_INFORMATION);
VOID        WINAPI Sleep(DWORD);
DWORD       WINAPI SleepEx(DWORD,BOOL32);
BOOL32      WINAPI StartService32A(HANDLE32,DWORD,LPCSTR*);
BOOL32      WINAPI StartService32W(HANDLE32,DWORD,LPCWSTR*);
#define     StartService WINELIB_NAME_AW(StartService)
DWORD       WINAPI SuspendThread(HANDLE32);
BOOL32      WINAPI SystemTimeToFileTime(const SYSTEMTIME*,LPFILETIME);
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
BOOL32      WINAPI WaitCommEvent(HANDLE32,LPDWORD,LPOVERLAPPED);
BOOL32      WINAPI WaitForDebugEvent(LPDEBUG_EVENT,DWORD);
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
UINT32      WINAPI WNetConnectionDialog1_32(HWND32,DWORD);
UINT32      WINAPI WNetConnectionDialog1_32A(LPCONNECTDLGSTRUCT32A);
UINT32      WINAPI WNetConnectionDialog1_32W(LPCONNECTDLGSTRUCT32W);
#define     WNetConnectionDialog1 WINELIB_NAME_AW(WNetConnectionDialog1_)
UINT32      WINAPI MultinetGetErrorText32A(DWORD,DWORD,DWORD);
UINT32      WINAPI MultinetGetErrorText32W(DWORD,DWORD,DWORD);
#define     MultinetGetErrorText WINELIB_NAME_AW(MultinetGetErrorText_)
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
DWORD       WINAPI GetLastError(void);
LANGID      WINAPI GetSystemDefaultLangID(void);
LCID        WINAPI GetSystemDefaultLCID(void);
LANGID      WINAPI GetUserDefaultLangID(void);
LCID        WINAPI GetUserDefaultLCID(void);
VOID        WINAPI LZDone(void);
VOID        WINAPI ScreenSwitchEnable(WORD);
DWORD       WINAPI RegCloseKey(HKEY);
DWORD       WINAPI RegFlushKey(HKEY);
VOID        WINAPI SetLastError(DWORD);


/* Declarations for functions that change between Win16 and Win32 */

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
INT16       WINAPI AnsiToOem16(LPCSTR,LPSTR);
#define     AnsiToOem32A CharToOem32A
#define     AnsiToOem32W CharToOem32W
#define     AnsiToOem WINELIB_NAME_AW(AnsiToOem)
VOID        WINAPI AnsiToOemBuff16(LPCSTR,LPSTR,UINT16);
#define     AnsiToOemBuff32A CharToOemBuff32A
#define     AnsiToOemBuff32W CharToOemBuff32W
#define     AnsiToOemBuff WINELIB_NAME_AW(AnsiToOemBuff)
BOOL16      WINAPI CheckMenuRadioButton16(HMENU16,UINT16,UINT16,UINT16,BOOL16);
BOOL32      WINAPI CheckMenuRadioButton32(HMENU32,UINT32,UINT32,UINT32,BOOL32);
#define     CheckMenuRadioButton WINELIB_NAME(CheckMenuRadioButton)
UINT16      WINAPI CompareString16(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT32      WINAPI CompareString32A(DWORD,DWORD,LPCSTR,DWORD,LPCSTR,DWORD);
UINT32      WINAPI CompareString32W(DWORD,DWORD,LPCWSTR,DWORD,LPCWSTR,DWORD);
#define     CompareString WINELIB_NAME_AW(CompareString)
LONG        WINAPI CopyLZFile16(HFILE16,HFILE16);
LONG        WINAPI CopyLZFile32(HFILE32,HFILE32);
#define     CopyLZFile WINELIB_NAME(CopyLZFile)
INT16       WINAPI CountVoiceNotes16(INT16);
DWORD       WINAPI CountVoiceNotes32(DWORD);
#define     CountVoiceNotes WINELIB_NAME(CountVoiceNotes)
BOOL16      WINAPI CreateDirectory16(LPCSTR,LPVOID);
BOOL32      WINAPI CreateDirectory32A(LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL32      WINAPI CreateDirectory32W(LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     CreateDirectory WINELIB_NAME_AW(CreateDirectory)
BOOL32      WINAPI CreateDirectoryEx32A(LPCSTR,LPCSTR,LPSECURITY_ATTRIBUTES);
BOOL32      WINAPI CreateDirectoryEx32W(LPCWSTR,LPCWSTR,LPSECURITY_ATTRIBUTES);
#define     CreateDirectoryEx WINELIB_NAME_AW(CreateDirectoryEx)
BOOL16      WINAPI DefineHandleTable16(WORD);
#define     DefineHandleTable32(w) ((w),TRUE)
#define     DefineHandleTable WINELIB_NAME(DefineHandleTable)
ATOM        WINAPI DeleteAtom16(ATOM);
ATOM        WINAPI DeleteAtom32(ATOM);
#define     DeleteAtom WINELIB_NAME(DeleteAtom)
BOOL16      WINAPI DeleteFile16(LPCSTR);
BOOL32      WINAPI DeleteFile32A(LPCSTR);
BOOL32      WINAPI DeleteFile32W(LPCWSTR);
#define     DeleteFile WINELIB_NAME_AW(DeleteFile)
BOOL16      WINAPI EnumTaskWindows16(HTASK16,WNDENUMPROC16,LPARAM);
#define     EnumTaskWindows32(handle,proc,lparam) \
            EnumThreadWindows(handle,proc,lparam)
#define     EnumTaskWindows WINELIB_NAME(EnumTaskWindows)
HICON16     WINAPI ExtractIcon16(HINSTANCE16,LPCSTR,UINT16);
HICON32     WINAPI ExtractIcon32A(HINSTANCE32,LPCSTR,UINT32);
HICON32     WINAPI ExtractIcon32W(HINSTANCE32,LPCWSTR,UINT32);
#define     ExtractIcon WINELIB_NAME_AW(ExtractIcon)
HICON16     WINAPI ExtractAssociatedIcon16(HINSTANCE16,LPSTR,LPWORD);
HICON32     WINAPI ExtractAssociatedIcon32A(HINSTANCE32,LPSTR,LPWORD);
HICON32     WINAPI ExtractAssociatedIcon32W(HINSTANCE32,LPWSTR,LPWORD);
#define     ExtractAssociatedIcon WINELIB_NAME_AW(ExtractAssociatedIcon)
void        WINAPI FatalAppExit16(UINT16,LPCSTR);
void        WINAPI FatalAppExit32A(UINT32,LPCSTR);
void        WINAPI FatalAppExit32W(UINT32,LPCWSTR);
#define     FatalAppExit WINELIB_NAME_AW(FatalAppExit)
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
UINT16      WINAPI GetAtomName16(ATOM,LPSTR,INT16);
UINT32      WINAPI GetAtomName32A(ATOM,LPSTR,INT32);
UINT32      WINAPI GetAtomName32W(ATOM,LPWSTR,INT32);
#define     GetAtomName WINELIB_NAME_AW(GetAtomName)
UINT16      WINAPI GetCurrentDirectory16(UINT16,LPSTR);
UINT32      WINAPI GetCurrentDirectory32A(UINT32,LPSTR);
UINT32      WINAPI GetCurrentDirectory32W(UINT32,LPWSTR);
#define     GetCurrentDirectory WINELIB_NAME_AW(GetCurrentDirectory)
BOOL16      WINAPI GetDiskFreeSpace16(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL32      WINAPI GetDiskFreeSpace32A(LPCSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
BOOL32      WINAPI GetDiskFreeSpace32W(LPCWSTR,LPDWORD,LPDWORD,LPDWORD,LPDWORD);
#define     GetDiskFreeSpace WINELIB_NAME_AW(GetDiskFreeSpace)
BOOL32      WINAPI GetDiskFreeSpaceEx32A(LPCSTR,LPULARGE_INTEGER,LPULARGE_INTEGER,LPULARGE_INTEGER);
BOOL32      WINAPI GetDiskFreeSpaceEx32W(LPCWSTR,LPULARGE_INTEGER,LPULARGE_INTEGER,LPULARGE_INTEGER);
#define     GetDiskFreeSpaceEx WINELIB_NAME_AW(GetDiskFreeSpaceEx)
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
DWORD       WINAPI GetFreeSpace16(UINT16);
#define     GetFreeSpace32(w) (0x100000L)
#define     GetFreeSpace WINELIB_NAME(GetFreeSpace)
UINT32      WINAPI GetLogicalDriveStrings32A(UINT32,LPSTR);
UINT32      WINAPI GetLogicalDriveStrings32W(UINT32,LPWSTR);
#define     GetLogicalDriveStrings WINELIB_NAME_AW(GetLogicalDriveStrings)
INT16       WINAPI GetLocaleInfo16(LCID,LCTYPE,LPSTR,INT16);
INT32       WINAPI GetLocaleInfo32A(LCID,LCTYPE,LPSTR,INT32);
INT32       WINAPI GetLocaleInfo32W(LCID,LCTYPE,LPWSTR,INT32);
#define     GetLocaleInfo WINELIB_NAME_AW(GetLocaleInfo)
INT16       WINAPI GetModuleFileName16(HINSTANCE16,LPSTR,INT16);
DWORD       WINAPI GetModuleFileName32A(HMODULE32,LPSTR,DWORD);
DWORD       WINAPI GetModuleFileName32W(HMODULE32,LPWSTR,DWORD);
#define     GetModuleFileName WINELIB_NAME_AW(GetModuleFileName)
HMODULE16   WINAPI GetModuleHandle16(LPCSTR);
HMODULE32   WINAPI GetModuleHandle32A(LPCSTR);
HMODULE32   WINAPI GetModuleHandle32W(LPCWSTR);
#define     GetModuleHandle WINELIB_NAME_AW(GetModuleHandle)
UINT16      WINAPI GetPrivateProfileInt16(LPCSTR,LPCSTR,INT16,LPCSTR);
UINT32      WINAPI GetPrivateProfileInt32A(LPCSTR,LPCSTR,INT32,LPCSTR);
UINT32      WINAPI GetPrivateProfileInt32W(LPCWSTR,LPCWSTR,INT32,LPCWSTR);
#define     GetPrivateProfileInt WINELIB_NAME_AW(GetPrivateProfileInt)
INT16       WINAPI GetPrivateProfileSection16(LPCSTR,LPSTR,UINT16,LPCSTR);
INT32       WINAPI GetPrivateProfileSection32A(LPCSTR,LPSTR,DWORD,LPCSTR);
INT32       WINAPI GetPrivateProfileSection32W(LPCWSTR,LPWSTR,DWORD,LPCWSTR);
#define     GetPrivateProfileSection WINELIB_NAME_AW(GetPrivateProfileSection)
WORD        WINAPI GetPrivateProfileSectionNames16(LPSTR,UINT16,LPCSTR);
DWORD       WINAPI GetPrivateProfileSectionNames32A(LPSTR,DWORD,LPCSTR);
DWORD       WINAPI GetPrivateProfileSectionNames32W(LPWSTR,DWORD,LPCWSTR);
#define     GetPrivateProfileSectionNames WINELIB_NAME_AW(GetPrivateProfileSectionNames)
INT16       WINAPI GetPrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16,LPCSTR);
INT32       WINAPI GetPrivateProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT32,LPCSTR);
INT32       WINAPI GetPrivateProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT32,LPCWSTR);
#define     GetPrivateProfileString WINELIB_NAME_AW(GetPrivateProfileString)
BOOL16      WINAPI GetPrivateProfileStruct16(LPCSTR,LPCSTR,LPVOID,UINT16,LPCSTR);
BOOL32      WINAPI GetPrivateProfileStruct32A(LPCSTR,LPCSTR,LPVOID,UINT32,LPCSTR);
BOOL32      WINAPI GetPrivateProfileStruct32W(LPCWSTR,LPCWSTR,LPVOID,UINT32,LPCWSTR);
#define     GetPrivateProfileStruct WINELIB_NAME_AW(GetPrivateProfileStruct)
FARPROC16   WINAPI GetProcAddress16(HMODULE16,SEGPTR);
FARPROC32   WINAPI GetProcAddress32(HMODULE32,LPCSTR);
#define     GetProcAddress WINELIB_NAME(GetProcAddress)
UINT16      WINAPI GetProfileInt16(LPCSTR,LPCSTR,INT16);
UINT32      WINAPI GetProfileInt32A(LPCSTR,LPCSTR,INT32);
UINT32      WINAPI GetProfileInt32W(LPCWSTR,LPCWSTR,INT32);
#define     GetProfileInt WINELIB_NAME_AW(GetProfileInt)
INT16       WINAPI GetProfileString16(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT16);
INT32       WINAPI GetProfileSection32A(LPCSTR,LPSTR,DWORD);
INT32       WINAPI GetProfileSection32W(LPCWSTR,LPWSTR,DWORD);
#define     GetProfileSection WINELIB_NAME_AW(GetProfileSection)
INT32       WINAPI GetProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPSTR,UINT32);
INT32       WINAPI GetProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPWSTR,UINT32);
#define     GetProfileString WINELIB_NAME_AW(GetProfileString)
BOOL16      WINAPI GetStringType16(LCID,DWORD,LPCSTR,INT16,LPWORD);
BOOL32      WINAPI GetStringType32A(LCID,DWORD,LPCSTR,INT32,LPWORD);
BOOL32      WINAPI GetStringType32W(DWORD,LPCWSTR,INT32,LPWORD);
#define     GetStringType WINELIB_NAME_AW(GetStringType)
UINT16      WINAPI GetSystemDirectory16(LPSTR,UINT16);
UINT32      WINAPI GetSystemDirectory32A(LPSTR,UINT32);
UINT32      WINAPI GetSystemDirectory32W(LPWSTR,UINT32);
#define     GetSystemDirectory WINELIB_NAME_AW(GetSystemDirectory)
UINT16      WINAPI GetTempFileName16(BYTE,LPCSTR,UINT16,LPSTR);
UINT32      WINAPI GetTempFileName32A(LPCSTR,LPCSTR,UINT32,LPSTR);
UINT32      WINAPI GetTempFileName32W(LPCWSTR,LPCWSTR,UINT32,LPWSTR);
#define     GetTempFileName WINELIB_NAME_AW(GetTempFileName)
UINT32      WINAPI GetTempPath32A(UINT32,LPSTR);
UINT32      WINAPI GetTempPath32W(UINT32,LPWSTR);
#define     GetTempPath WINELIB_NAME_AW(GetTempPath)
LPINT16     WINAPI GetThresholdEvent16(void);
LPDWORD     WINAPI GetThresholdEvent32(void);
#define     GetThresholdEvent WINELIB_NAME(GetThresholdEvent)
INT16       WINAPI GetThresholdStatus16(void);
DWORD       WINAPI GetThresholdStatus32(void);
#define     GetThresholdStatus WINELIB_NAME(GetThresholdStatus)
LONG        WINAPI GetVersion16(void);
LONG        WINAPI GetVersion32(void);
#define     GetVersion WINELIB_NAME(GetVersion)
BOOL32      WINAPI GetExitCodeProcess(HANDLE32,LPDWORD);
BOOL32      WINAPI GetVolumeInformation32A(LPCSTR,LPSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPSTR,DWORD);
BOOL32      WINAPI GetVolumeInformation32W(LPCWSTR,LPWSTR,DWORD,LPDWORD,LPDWORD,LPDWORD,LPWSTR,DWORD);
#define     GetVolumeInformation WINELIB_NAME_AW(GetVolumeInformation)
UINT16      WINAPI GetWindowsDirectory16(LPSTR,UINT16);
UINT32      WINAPI GetWindowsDirectory32A(LPSTR,UINT32);
UINT32      WINAPI GetWindowsDirectory32W(LPWSTR,UINT32);
#define     GetWindowsDirectory WINELIB_NAME_AW(GetWindowsDirectory)
HGLOBAL16   WINAPI GlobalAlloc16(UINT16,DWORD);
HGLOBAL32   WINAPI GlobalAlloc32(UINT32,DWORD);
#define     GlobalAlloc WINELIB_NAME(GlobalAlloc)
DWORD       WINAPI GlobalCompact16(DWORD);
DWORD       WINAPI GlobalCompact32(DWORD);
#define     GlobalCompact WINELIB_NAME(GlobalCompact)
UINT16      WINAPI GlobalFlags16(HGLOBAL16);
UINT32      WINAPI GlobalFlags32(HGLOBAL32);
#define     GlobalFlags WINELIB_NAME(GlobalFlags)
HGLOBAL16   WINAPI GlobalFree16(HGLOBAL16);
HGLOBAL32   WINAPI GlobalFree32(HGLOBAL32);
#define     GlobalFree WINELIB_NAME(GlobalFree)
DWORD       WINAPI GlobalHandle16(WORD);
HGLOBAL32   WINAPI GlobalHandle32(LPCVOID);
#define     GlobalHandle WINELIB_NAME(GlobalHandle)
WORD        WINAPI GlobalFix16(HGLOBAL16);
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
WORD        WINAPI InitAtomTable16(WORD);
BOOL32      WINAPI InitAtomTable32(DWORD);
#define     InitAtomTable WINELIB_NAME(InitAtomTable)
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
BOOL16      WINAPI IsDBCSLeadByte16(BYTE);
BOOL32      WINAPI IsDBCSLeadByte32(BYTE);
#define     IsDBCSLeadByte WINELIB_NAME(IsDBCSLeadByte)
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
HINSTANCE16 WINAPI LoadLibrary16(LPCSTR);
HMODULE32   WINAPI LoadLibrary32A(LPCSTR);
HMODULE32   WINAPI LoadLibrary32W(LPCWSTR);
#define     LoadLibrary WINELIB_NAME_AW(LoadLibrary)
HMODULE32   WINAPI LoadLibraryEx32A(LPCSTR,HFILE32,DWORD);
HMODULE32   WINAPI LoadLibraryEx32W(LPCWSTR,HFILE32,DWORD);
#define     LoadLibraryEx WINELIB_NAME_AW(LoadLibraryEx)
HINSTANCE16 WINAPI LoadModule16(LPCSTR,LPVOID);
HINSTANCE32 WINAPI LoadModule32(LPCSTR,LPVOID);
#define     LoadModule WINELIB_NAME(LoadModule)
HGLOBAL16   WINAPI LoadResource16(HINSTANCE16,HRSRC16);
HGLOBAL32   WINAPI LoadResource32(HMODULE32,HRSRC32);
#define     LoadResource WINELIB_NAME(LoadResource)
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
FARPROC16   WINAPI MakeProcInstance16(FARPROC16,HANDLE16);
#define     MakeProcInstance32(proc,inst) (proc)
#define     MakeProcInstance WINELIB_NAME(MakeProcInstance)
INT16       WINAPI OemToAnsi16(LPCSTR,LPSTR);
#define     OemToAnsi32A OemToChar32A
#define     OemToAnsi32W OemToChar32W
#define     OemToAnsi WINELIB_NAME_AW(OemToAnsi)
VOID        WINAPI OemToAnsiBuff16(LPCSTR,LPSTR,UINT16);
#define     OemToAnsiBuff32A OemToCharBuff32A
#define     OemToAnsiBuff32W OemToCharBuff32W
#define     OemToAnsiBuff WINELIB_NAME_AW(OemToAnsiBuff)
INT16       WINAPI WideCharToLocal16(LPSTR,LPWSTR,INT16);
INT32       WINAPI WideCharToLocal32(LPSTR,LPWSTR,INT32);
#define			WideCharToLocal WINELIB_NAME(WideCharToLocal)
INT16       WINAPI LocalToWideChar16(LPWSTR,LPSTR,INT16);
INT32       WINAPI LocalToWideChar32(LPWSTR,LPSTR,INT32);
#define			LocalToWideChar WINELIB_NAME(LocalToWideChar)
HFILE16     WINAPI OpenFile16(LPCSTR,OFSTRUCT*,UINT16);
HFILE32     WINAPI OpenFile32(LPCSTR,OFSTRUCT*,UINT32);
#define     OpenFile WINELIB_NAME(OpenFile)
INT16       WINAPI OpenSound16(void);
VOID        WINAPI OpenSound32(void);
#define     OpenSound WINELIB_NAME(OpenSound)
VOID        WINAPI OutputDebugString16(LPCSTR);
VOID        WINAPI OutputDebugString32A(LPCSTR);
VOID        WINAPI OutputDebugString32W(LPCWSTR);
#define     OutputDebugString WINELIB_NAME_AW(OutputDebugString)
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
HRESULT     WINAPI RegisterDragDrop16(HWND16,LPVOID);
HRESULT     WINAPI RegisterDragDrop32(HWND32,LPVOID);
#define     RegisterDragDrop WINELIB_NAME(RegisterDragDrop)
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
BOOL16      WINAPI RemoveDirectory16(LPCSTR);
BOOL32      WINAPI RemoveDirectory32A(LPCSTR);
BOOL32      WINAPI RemoveDirectory32W(LPCWSTR);
#define     RemoveDirectory WINELIB_NAME_AW(RemoveDirectory)
HRESULT     WINAPI RevokeDragDrop16(HWND16);
HRESULT     WINAPI RevokeDragDrop32(HWND32);
#define     RevokeDragDrop WINELIB_NAME(RevokeDragDrop)
BOOL16      WINAPI SetCurrentDirectory16(LPCSTR);
BOOL32      WINAPI SetCurrentDirectory32A(LPCSTR);
BOOL32      WINAPI SetCurrentDirectory32W(LPCWSTR);
#define     SetCurrentDirectory WINELIB_NAME_AW(SetCurrentDirectory)
UINT16      WINAPI SetErrorMode16(UINT16);
UINT32      WINAPI SetErrorMode32(UINT32);
#define     SetErrorMode WINELIB_NAME(SetErrorMode)
BOOL16      WINAPI SetFileAttributes16(LPCSTR,DWORD);
BOOL32      WINAPI SetFileAttributes32A(LPCSTR,DWORD);
BOOL32      WINAPI SetFileAttributes32W(LPCWSTR,DWORD);
#define     SetFileAttributes WINELIB_NAME_AW(SetFileAttributes)
UINT16      WINAPI SetHandleCount16(UINT16);
UINT32      WINAPI SetHandleCount32(UINT32);
#define     SetHandleCount WINELIB_NAME(SetHandleCount)
INT16       WINAPI SetSoundNoise16(INT16,INT16);
DWORD       WINAPI SetSoundNoise32(DWORD,DWORD);
#define     SetSoundNoise WINELIB_NAME(SetSoundNoise)
LONG        WINAPI SetSwapAreaSize16(WORD);
#define     SetSwapAreaSize32(w) (w)
#define     SetSwapAreaSize WINELIB_NAME(SetSwapAreaSize)
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
BOOL16      WINAPI ShellAbout16(HWND16,LPCSTR,LPCSTR,HICON16);
BOOL32      WINAPI ShellAbout32A(HWND32,LPCSTR,LPCSTR,HICON32);
BOOL32      WINAPI ShellAbout32W(HWND32,LPCWSTR,LPCWSTR,HICON32);
#define     ShellAbout WINELIB_NAME_AW(ShellAbout)
HINSTANCE16 WINAPI ShellExecute16(HWND16,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT16);
HINSTANCE32 WINAPI ShellExecute32A(HWND32,LPCSTR,LPCSTR,LPCSTR,LPCSTR,INT32);
HINSTANCE32 WINAPI ShellExecute32W(HWND32,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,INT32);
#define     ShellExecute WINELIB_NAME_AW(ShellExecute)
DWORD       WINAPI SizeofResource16(HMODULE16,HRSRC16);
DWORD       WINAPI SizeofResource32(HMODULE32,HRSRC32);
#define     SizeofResource WINELIB_NAME(SizeofResource)
INT16       WINAPI StartSound16(void);
VOID        WINAPI StartSound32(void);
#define     StartSound WINELIB_NAME(StartSound)
INT16       WINAPI StopSound16(void);
VOID        WINAPI StopSound32(void);
#define     StopSound WINELIB_NAME(StopSound)
INT16       WINAPI SyncAllVoices16(void);
DWORD       WINAPI SyncAllVoices32(void);
#define     SyncAllVoices WINELIB_NAME(SyncAllVoices)
void        WINAPI UnlockSegment16(HGLOBAL16);
#define     UnlockSegment32(handle) GlobalUnfix((HANDLE32)(handle))
#define     UnlockSegment WINELIB_NAME(UnlockSegment)
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
INT16       WINAPI WaitSoundState16(INT16);
DWORD       WINAPI WaitSoundState32(DWORD);
#define     WaitSoundState WINELIB_NAME(WaitSoundState)
BOOL16      WINAPI WritePrivateProfileSection16(LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WritePrivateProfileSection32A(LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WritePrivateProfileSection32W(LPCWSTR,LPCWSTR,LPCWSTR);
#define     WritePrivateProfileSection WINELIB_NAME_AW(WritePrivateProfileSection)
BOOL16      WINAPI WritePrivateProfileString16(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WritePrivateProfileString32A(LPCSTR,LPCSTR,LPCSTR,LPCSTR);
BOOL32      WINAPI WritePrivateProfileString32W(LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
BOOL32	     WINAPI WriteProfileSection32A(LPCSTR,LPCSTR);
BOOL32	     WINAPI WriteProfileSection32W(LPCWSTR,LPCWSTR);
#define     WritePrivateProfileString WINELIB_NAME_AW(WritePrivateProfileString)
BOOL16      WINAPI WritePrivateProfileStruct16(LPCSTR,LPCSTR,LPVOID,UINT16,LPCSTR);
BOOL32      WINAPI WritePrivateProfileStruct32A(LPCSTR,LPCSTR,LPVOID,UINT32,LPCSTR);
BOOL32      WINAPI WritePrivateProfileStruct32W(LPCWSTR,LPCWSTR,LPVOID,UINT32,LPCWSTR);
#define     WritePrivateProfileStruct WINELIB_NAME_AW(WritePrivateProfileStruct)
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
UINT32      WINAPI WIN16_GetTempDrive(BYTE);
SEGPTR      WINAPI WIN16_GlobalLock16(HGLOBAL16);
SEGPTR      WINAPI WIN16_LockResource16(HGLOBAL16);
LONG        WINAPI WIN16_hread(HFILE16,SEGPTR,LONG);
UINT16      WINAPI WIN16_lread(HFILE16,SEGPTR,UINT16);
INT32       WINAPI lstrncmp32A(LPCSTR,LPCSTR,INT32);
INT32       WINAPI lstrncmp32W(LPCWSTR,LPCWSTR,INT32);
INT32       WINAPI lstrncmpi32A(LPCSTR,LPCSTR,INT32);
INT32       WINAPI lstrncmpi32W(LPCWSTR,LPCWSTR,INT32);
LPWSTR      WINAPI lstrcpyAtoW(LPWSTR,LPCSTR);
LPSTR       WINAPI lstrcpyWtoA(LPSTR,LPCWSTR);
LPWSTR      WINAPI lstrcpynAtoW(LPWSTR,LPCSTR,INT32);
LPSTR       WINAPI lstrcpynWtoA(LPSTR,LPCWSTR,INT32);
INT16       WINAPI lstrcmp16(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmp32A(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmp32W(LPCWSTR,LPCWSTR);
#define     lstrcmp WINELIB_NAME_AW(lstrcmp)
INT16       WINAPI lstrcmpi16(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmpi32A(LPCSTR,LPCSTR);
INT32       WINAPI lstrcmpi32W(LPCWSTR,LPCWSTR);
#define     lstrcmpi WINELIB_NAME_AW(lstrcmpi)


#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINDOWS_H */
