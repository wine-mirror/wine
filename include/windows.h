/* Initial draft attempt of windows.h, by Peter MacDonald, pmacdona@sanjuan.uvic.ca */

#ifndef WINDOWS_H
#define WINDOWS_H

#include "wintypes.h"

#ifndef WINELIB
#pragma pack(1)
#endif

typedef struct { 
	INT x;
	INT y; 
} POINT;
typedef POINT *PPOINT;
typedef POINT *NPPOINT;
typedef POINT *LPPOINT;

typedef struct 
{
    INT cx;
    INT cy;
} SIZE, *LPSIZE;

#define MAKEPOINT(l) (*((POINT *)&(l)))

#define MAKELPARAM(low, high) ((LONG)(((WORD)(low)) | \
			      (((DWORD)((WORD)(high))) << 16)))

typedef struct { INT left, top, right, bottom; } RECT;
typedef RECT *LPRECT;
typedef RECT *NPRECT;
typedef RECT *PRECT;

typedef struct {
	HDC hdc;
	BOOL	fErase;
	RECT	rcPaint;
	BOOL	fRestore, fIncUpdate;
	BYTE	rgbReserved[16];
} PAINTSTRUCT;

typedef PAINTSTRUCT *PPAINTSTRUCT;
typedef PAINTSTRUCT *NPPAINTSTRUCT;
typedef PAINTSTRUCT *LPPAINTSTRUCT;


  /* Window classes */

typedef struct {
	WORD	style;
	WNDPROC	lpfnWndProc WINE_PACKED;
	INT	cbClsExtra, cbWndExtra;
	HANDLE	hInstance;
	HICON	hIcon;
	HCURSOR	hCursor;
	HBRUSH	hbrBackground;
	SEGPTR  lpszMenuName WINE_PACKED;
	SEGPTR  lpszClassName WINE_PACKED;
} WNDCLASS, *LPWNDCLASS;

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
#define GCW_HCURSOR         (-12)
#define GCW_HICON           (-14)
#define GCW_HMODULE         (-16)
#define GCW_CBWNDEXTRA      (-18)
#define GCW_CBCLSEXTRA      (-20)
#define GCL_WNDPROC         (-24)
#define GCW_STYLE           (-26)
#define GCW_ATOM            (-32)

  /* Windows */

typedef struct {
    void *    lpCreateParams;
    HINSTANCE hInstance;
    HMENU     hMenu;
    HWND      hwndParent;
    INT	      cy;
    INT       cx;
    INT       y;
    INT       x;
    LONG      style WINE_PACKED;
    SEGPTR    lpszName WINE_PACKED;
    SEGPTR    lpszClass WINE_PACKED;
    DWORD     dwExStyle WINE_PACKED;
} CREATESTRUCT, *LPCREATESTRUCT;

typedef struct 
{
    HMENU     hWindowMenu;
    WORD      idFirstChild;
} CLIENTCREATESTRUCT, *LPCLIENTCREATESTRUCT;

typedef struct
{
    SEGPTR    szClass;
    SEGPTR    szTitle;
    HANDLE    hOwner;
    INT       x;
    INT       y;
    INT       cx;
    INT	      cy;
    LONG      style WINE_PACKED;
    LONG      lParam WINE_PACKED;
} MDICREATESTRUCT, *LPMDICREATESTRUCT;

  /* Offsets for GetWindowLong() and GetWindowWord() */
#define GWL_EXSTYLE         (-20)
#define GWL_STYLE           (-16)
#define GWW_ID              (-12)
#define GWW_HWNDPARENT      (-8)
#define GWW_HINSTANCE       (-6)
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
    POINT   ptReserved;
    POINT   ptMaxSize;
    POINT   ptMaxPosition;
    POINT   ptMinTrackSize;
    POINT   ptMaxTrackSize;
} MINMAXINFO;

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
    HWND    hwnd;
    HWND    hwndInsertAfter;
    int     x;
    int     y;
    int     cx;
    int     cy;
    UINT    flags;
} WINDOWPOS;

  /* SetWindowPlacement() struct */
typedef struct
{
    UINT   length;
    UINT   flags;
    UINT   showCmd;
    POINT  ptMinPosition WINE_PACKED;
    POINT  ptMaxPosition WINE_PACKED;
    RECT   rcNormalPosition WINE_PACKED;
} WINDOWPLACEMENT, *LPWINDOWPLACEMENT;

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
    RECT	   rgrc[3];
    WINDOWPOS FAR* lppos;
} NCCALCSIZE_PARAMS;

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
#define WH_SHELL           10
#define WH_MSGFILTER	    (-1)

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
#define MSGF_MENU           2
#define MSGF_MOVE           3
#define MSGF_SIZE           4
#define MSGF_SCROLLBAR      5
#define MSGF_NEXTWINDOW     6
#define MSGF_MAINLOOP       8
#define MSGF_USER        4096

  /* Journalling hook values */
#define HC_GETNEXT	    1
#define HC_SKIP 	    2
#define HC_NOREMOVE	    3
#define HC_NOREM	    HC_NOREMOVE
#define HC_SYSMODALON       4
#define HC_SYSMODALOFF	    5

  /* Journalling hook structure */
typedef struct tagEVENTMSG
{
    UINT    message;
    UINT    paramL;
    UINT    paramH;
    DWORD   time WINE_PACKED;
} EVENTMSG, *LPEVENTMSG;

  /* Mouse hook structure */
typedef struct tagMOUSEHOOKSTRUCT
{
    POINT   pt;
    HWND    hwnd;
    WORD    wHitTestCode;
    DWORD   dwExtraInfo;
} MOUSEHOOKSTRUCT, *LPMOUSEHOOKSTRUCT;

  /* Hardware hook structure */
typedef struct tagHARDWAREHOOKSTRUCT
{
    HWND    hWnd;
    UINT    wMessage;
    WPARAM  wParam;
    LPARAM  lParam WINE_PACKED;
} HARDWAREHOOKSTRUCT;

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
typedef struct tagCBT_CREATEWND
{
    CREATESTRUCT *lpcs;
    HWND          hwndInsertAfter;
} CBT_CREATEWND, *LPCBT_CREATEWND;

typedef struct tagCBTACTIVATESTRUCT
{
    BOOL    fMouse;
    HWND    hWndActive;
} CBTACTIVATESTRUCT;

  /* Shell hook values */
#define HSHELL_WINDOWCREATED       1
#define HSHELL_WINDOWDESTROYED     2
#define HSHELL_ACTIVATESHELLWINDOW 3

  /* Debug hook structure */
typedef struct tagDEBUGHOOKINFO
{
    HANDLE	hModuleHook;
    LPARAM	reserved WINE_PACKED;
    LPARAM	lParam WINE_PACKED;
    WPARAM	wParam;
    short       code;
} DEBUGHOOKINFO, *LPDEBUGHOOKINFO;


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


typedef struct tagMSG
{
  HWND    hwnd;
  WORD    message;
  WORD    wParam;
  DWORD   lParam WINE_PACKED;
  DWORD   time WINE_PACKED;
  POINT	  pt WINE_PACKED;
} MSG, *LPMSG;
	
typedef WORD ATOM;

#define MAKEINTATOM(i)   ((SEGPTR)MAKELONG((i),0))

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

typedef struct tagBITMAP
{
    INT  bmType;
    INT  bmWidth;
    INT  bmHeight;
    INT  bmWidthBytes;
    BYTE   bmPlanes;
    BYTE   bmBitsPixel;
    void * bmBits WINE_PACKED;
} BITMAP;

typedef BITMAP *PBITMAP;
typedef BITMAP *NPBITMAP;
typedef BITMAP *LPBITMAP;

  /* Brushes */

typedef struct tagLOGBRUSH
{ 
    WORD       lbStyle; 
    COLORREF   lbColor WINE_PACKED;
    INT      lbHatch; 
} LOGBRUSH, *PLOGBRUSH, *NPLOGBRUSH, *LPLOGBRUSH;

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

typedef struct tagLOGFONT
{
    INT lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
    BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
    BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
    BYTE lfFaceName[LF_FACESIZE] WINE_PACKED;
} LOGFONT, *PLOGFONT, *NPLOGFONT, *LPLOGFONT;

typedef struct tagENUMLOGFONT
{
  LOGFONT elfLogFont;
  BYTE elfFullName[LF_FULLFACESIZE] WINE_PACKED;
  BYTE elfStyle[LF_FACESIZE] WINE_PACKED;
} ENUMLOGFONT,*PENUMLOGFONT,*NPENUMLOGFONT,*LPENUMLOGFONT;

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

typedef struct tagTEXTMETRIC
{
    INT     tmHeight;
    INT     tmAscent;
    INT     tmDescent;
    INT     tmInternalLeading;
    INT     tmExternalLeading;
    INT     tmAveCharWidth;
    INT     tmMaxCharWidth;
    INT     tmWeight;
    BYTE    tmItalic;
    BYTE    tmUnderlined;
    BYTE    tmStruckOut;
    BYTE    tmFirstChar;
    BYTE    tmLastChar;
    BYTE    tmDefaultChar;
    BYTE    tmBreakChar;
    BYTE    tmPitchAndFamily;
    BYTE    tmCharSet;
    INT     tmOverhang WINE_PACKED;
    INT     tmDigitizedAspectX WINE_PACKED;
    INT     tmDigitizedAspectY WINE_PACKED;
} TEXTMETRIC, *PTEXTMETRIC, *NPTEXTMETRIC, *LPTEXTMETRIC;

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

typedef struct tagLOGPALETTE
{ 
    WORD           palVersion;
    WORD           palNumEntries;
    PALETTEENTRY   palPalEntry[1] WINE_PACKED;
} LOGPALETTE, *PLOGPALETTE, *NPLOGPALETTE, *LPLOGPALETTE;


  /* Pens */

typedef struct tagLOGPEN
{
    WORD     lopnStyle; 
    POINT    lopnWidth WINE_PACKED;
    COLORREF lopnColor WINE_PACKED;
} LOGPEN, *PLOGPEN, *NPLOGPEN, *LPLOGPEN;

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
    UINT    bfType;
    DWORD   bfSize WINE_PACKED;
    UINT    bfReserved1 WINE_PACKED;
    UINT    bfReserved2 WINE_PACKED;
    DWORD   bfOffBits WINE_PACKED;
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{
    DWORD 	biSize;
    DWORD 	biWidth;
    DWORD 	biHeight;
    WORD 	biPlanes;
    WORD 	biBitCount;
    DWORD 	biCompression;
    DWORD 	biSizeImage;
    DWORD 	biXPelsPerMeter;
    DWORD 	biYPelsPerMeter;
    DWORD 	biClrUsed;
    DWORD 	biClrImportant;
} BITMAPINFOHEADER;

typedef BITMAPINFOHEADER * LPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER * NPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER * PBITMAPINFOHEADER;

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

typedef struct tagBITMAPCOREHEADER
{
    unsigned long bcSize;
    UINT bcWidth;
    UINT bcHeight;
    UINT bcPlanes;
    UINT bcBitCount;
} BITMAPCOREHEADER;

#define DIB_RGB_COLORS   0
#define DIB_PAL_COLORS   1
#define CBM_INIT         4

typedef struct {
	BYTE i;  /* much more .... */
} KANJISTRUCT;
typedef KANJISTRUCT *LPKANJISTRUCT;
typedef KANJISTRUCT *NPKANJISTRUCT;
typedef KANJISTRUCT *PKANJISTRUCT;

typedef struct {
	BYTE cBytes, fFixedDisk;
	WORD nErrCode;
	BYTE reserved[4], szPathName[128];
} OFSTRUCT;
typedef OFSTRUCT *POFSTRUCT;
typedef OFSTRUCT *NPOFSTRUCT;
typedef OFSTRUCT *LPOFSTRUCT;

#define OF_READ 0x0000
#define OF_WRITE 0x0001
#define OF_READWRITE 0x0002
#define OF_CANCEL 0x0800
#define OF_CREATE 0x1000
#define OF_DELETE 0x0200
#define OF_EXIST 0x4000
#define OF_PARSE 0x0100
#define OF_PROMPT 0x2000
#define OF_REOPEN 0x8000
#define OF_SHARE_COMPAT 0x0000
#define OF_SHARE_DENY_NONE 0x0040
#define OF_SHARE_DENY_READ 0x0030
#define OF_SHARE_DENY_WRITE 0x0020
#define OF_SHARE_EXCLUSIVE 0x0010
#define OF_VERIFY 0x0400

#define DRIVE_CANNOTDETERMINE	0
#define DRIVE_DOESNOTEXIST	1
#define DRIVE_REMOVABLE	2
#define DRIVE_FIXED	3
#define DRIVE_REMOTE	4

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
#define INFINITE	0xFFFF

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
#define GETMAXLPT	8
#define GETMAXCOM	9
#define GETBASEIRQ	10

#define CN_RECEIVE	0x0001
#define CN_TRANSMIT	0x0002
#define CN_EVENT	0x0004

typedef struct tagDCB
{
    BYTE Id;
    UINT BaudRate WINE_PACKED;
    BYTE ByteSize;
    BYTE Parity;
    BYTE StopBits;
    UINT RlsTimeout;
    UINT CtsTimeout;
    UINT DsrTimeout;

    UINT fBinary        :1;
    UINT fRtsDisable    :1;
    UINT fParity        :1;
    UINT fOutxCtsFlow   :1;
    UINT fOutxDsrFlow   :1;
    UINT fDummy         :2;
    UINT fDtrDisable    :1;

    UINT fOutX          :1;
    UINT fInX           :1;
    UINT fPeChar        :1;
    UINT fNull          :1;
    UINT fChEvt         :1;
    UINT fDtrflow       :1;
    UINT fRtsflow       :1;
    UINT fDummy2        :1;

    char XonChar;
    char XoffChar;
    UINT XonLim;
    UINT XoffLim;
    char PeChar;
    char EofChar;
    char EvtChar;
    UINT TxDelay WINE_PACKED;
} DCB;
typedef DCB FAR* LPDCB;

typedef struct tagCOMSTAT
{
    BYTE status;
    UINT cbInQue WINE_PACKED;
    UINT cbOutQue WINE_PACKED;
} COMSTAT;

#define CSTF_CTSHOLD	0x01
#define CSTF_DSRHOLD	0x02
#define CSTF_RLSDHOLD	0x04
#define CSTF_XOFFHOLD	0x08
#define CSTF_XOFFSENT	0x10
#define CSTF_EOF	0x20
#define CSTF_TXIM	0x40

/* SystemParametersInfo */

#define	SPI_GETBEEP			1
#define	SPI_SETBEEP			2
#define	SPI_GETMOUSE			3
#define	SPI_SETMOUSE			4
#define	SPI_GETBORDER			5
#define	SPI_SETBORDER			6
#define	SPI_GETKEYBOARDSPEED		10
#define	SPI_SETKEYBOARDSPEED		11
#define	SPI_LANGDRIVER			12
#define SPI_ICONHORIZONTALSPACING	13
#define SPI_GETSCREENSAVETIMEOUT	14
#define SPI_SETSCREENSAVETIMEOUT	15
#define SPI_GETSCREENSAVEACTIVE		16
#define SPI_SETSCREENSAVEACTIVE		17
#define SPI_GETGRIDGRANULARITY		18
#define SPI_SETGRIDGRANULARITY		19
#define SPI_SETDESKWALLPAPER		20
#define SPI_SETDESKPATTERN		21
#define SPI_GETKEYBOARDDELAY		22
#define SPI_SETKEYBOARDDELAY		23
#define SPI_ICONVERTICALSPACING		24
#define SPI_GETICONTITLEWRAP		25
#define SPI_SETICONTITLEWRAP		26
#define SPI_GETMENUDROPALIGNMENT	27
#define SPI_SETMENUDROPALIGNMENT	28
#define SPI_SETDOUBLECLKWIDTH		29
#define SPI_SETDOUBLECLKHEIGHT		30
#define SPI_GETICONTITLELOGFONT		31
#define SPI_SETDOUBLECLICKTIME		32
#define SPI_SETMOUSEBUTTONSWAP		33
#define SPI_SETICONTITLELOGFONT		34
#define SPI_GETFASTTASKSWITCH		35
#define SPI_SETFASTTASKSWITCH		36

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
#define RT_GROUP_CURSOR     MAKEINTRESOURCE(12)
#define RT_GROUP_ICON	    MAKEINTRESOURCE(14)

/* Predefined resources */
#define IDI_APPLICATION  MAKEINTRESOURCE(32512)
#define IDI_HAND         MAKEINTRESOURCE(32513)
#define IDI_QUESTION     MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION  MAKEINTRESOURCE(32515)
#define IDI_ASTERISK     MAKEINTRESOURCE(32516)

#define IDC_ARROW        MAKEINTRESOURCE(32512)
#define IDC_IBEAM        MAKEINTRESOURCE(32513)
#define IDC_WAIT         MAKEINTRESOURCE(32514)
#define IDC_CROSS        MAKEINTRESOURCE(32515)
#define IDC_UPARROW      MAKEINTRESOURCE(32516)
#define IDC_SIZE         MAKEINTRESOURCE(32540)
#define IDC_ICON         MAKEINTRESOURCE(32541)
#define IDC_SIZENWSE     MAKEINTRESOURCE(32542)
#define IDC_SIZENESW     MAKEINTRESOURCE(32543)
#define IDC_SIZEWE       MAKEINTRESOURCE(32544)
#define IDC_SIZENS       MAKEINTRESOURCE(32545)

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


enum { WM_NULL, WM_CREATE, WM_DESTROY, WM_MOVE, WM_UNUSED0, WM_SIZE, WM_ACTIVATE,
	WM_SETFOCUS, WM_KILLFOCUS, WM_UNUSED1, WM_ENABLE, WM_SETREDRAW, 
	WM_SETTEXT, WM_GETTEXT, WM_GETTEXTLENGTH, WM_PAINT, WM_CLOSE, 
	WM_QUERYENDSESSION, WM_QUIT, WM_QUERYOPEN, WM_ERASEBKGND, 
	WM_SYSCOLORCHANGE, WM_ENDSESSION, WM_UNUSED2,
	WM_SHOWWINDOW, WM_CTLCOLOR, WM_WININICHANGE, WM_DEVMODECHANGE,
	WM_ACTIVATEAPP, WM_FONTCHANGE, WM_TIMECHANGE, WM_CANCELMODE, WM_SETCURSOR,
	WM_MOUSEACTIVATE, WM_CHILDACTIVATE, WM_QUEUESYNC, WM_GETMINMAXINFO,
	WM_UNUSED3, WM_PAINTICON, WM_ICONERASEBKGND, WM_NEXTDLGCTL, 
	WM_UNUSED4, WM_SPOOLERSTATUS, WM_DRAWITEM, WM_MEASUREITEM, 
	WM_DELETEITEM, WM_VKEYTOITEM,
	WM_CHARTOITEM, WM_SETFONT, WM_GETFONT };

#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED 0x0047

#define WM_NCCREATE         0x0081
#define WM_NCDESTROY        0x0082
#define WM_NCCALCSIZE       0x0083
#define WM_NCHITTEST        0x0084
#define WM_NCPAINT          0x0085
#define WM_NCACTIVATE       0x0086

#define WM_GETDLGCODE	    0x0087

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

#define WM_ENTERIDLE        0x0121

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
#define WM_MDISETMENU	    0x0230

#define WM_ENTERSIZEMOVE    0x0231
#define WM_EXITSIZEMOVE     0x0232

  /* misc messages */
#define WM_NULL             0x0000
#define WM_USER             0x0400


  /* Key status flags for mouse events */
#define MK_LBUTTON	    0x0001
#define MK_RBUTTON	    0x0002
#define MK_SHIFT	    0x0004
#define MK_CONTROL	    0x0008
#define MK_MBUTTON	    0x0010

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

  /* WM_SIZE message wParam values */
#define SIZE_RESTORED        0
#define SIZE_MINIMIZED       1
#define SIZE_MAXIMIZED       2
#define SIZE_MAXSHOW         3
#define SIZE_MAXHIDE         4

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

/* Menu messages */
#define WM_INITMENU         0x0116
#define WM_INITMENUPOPUP    0x0117

#define WM_MENUSELECT       0x011F
#define WM_MENUCHAR         0x0120

#define MF_INSERT 0
#define MF_CHANGE 0x0080
#define MF_APPEND 0x0100
#define MF_DELETE 0x0200
#define MF_REMOVE 0x1000
#define MF_BYCOMMAND 0
#define MF_BYPOSITION 0x0400
#define MF_SEPARATOR 0x0800
#define MF_ENABLED 0
#define MF_GRAYED 0x0001
#define MF_DISABLED 0x0002
#define MF_UNCHECKED 0
#define MF_CHECKED 0x0008
#define MF_USECHECKBITMAPS 0x0200
#define MF_STRING 0
#define MF_BITMAP 0x0004
#define MF_OWNERDRAW 0x0100
#define MF_POPUP 0x0010
#define MF_MENUBARBREAK 0x0020
#define MF_MENUBREAK 0x0040
#define MF_UNHILITE 0
#define MF_HILITE 0x0080
#define MF_SYSMENU 0x2000
#define MF_HELP 0x4000
#define MF_MOUSESELECT 0x8000
#define MF_END 0x0080

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
#define WS_EX_NOPARENTNOTIFY   0x00000004L
#define WS_EX_TOPMOST          0x00000008L
#define WS_EX_ACCEPTFILES      0x00000010L
#define WS_EX_TRANSPARENT      0x00000020L

/* Window scrolling */
#define SW_SCROLLCHILDREN      0x0001
#define SW_INVALIDATE          0x0002
#define SW_ERASE               0x0003

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

/* Button control messages */
#define BM_GETCHECK            (WM_USER+0)
#define BM_SETCHECK            (WM_USER+1)
#define BM_GETSTATE            (WM_USER+2)
#define BM_SETSTATE            (WM_USER+3)
#define BM_SETSTYLE            (WM_USER+4)

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

/* Static Control Mesages */
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

/* Listbox styles */
#define LBS_NOTIFY            0x0001L
#define LBS_SORT              0x0002L
#define LBS_NOREDRAW          0x0004L
#define LBS_MULTIPLESEL       0x0008L
#define LBS_OWNERDRAWFIXED    0x0010L
#define LBS_OWNERDRAWVARIABLE 0x0020L
#define LBS_HASSTRINGS        0x0040L
#define LBS_USETABSTOPS       0x0080L
#define LBS_NOINTEGRALHEIGHT  0x0100L
#define LBS_MULTICOLUMN       0x0200L
#define LBS_WANTKEYBOARDINPUT 0x0400L
#define LBS_EXTENDEDSEL       0x0800L
#define LBS_DISABLENOSCROLL   0x1000L
#define LBS_STANDARD          (LBS_NOTIFY | LBS_SORT | WS_VSCROLL | WS_BORDER)

/* Listbox messages */
#define LB_ADDSTRING           (WM_USER+1)
#define LB_INSERTSTRING        (WM_USER+2)
#define LB_DELETESTRING        (WM_USER+3)
#define LB_RESETCONTENT        (WM_USER+5)
#define LB_SETSEL              (WM_USER+6)
#define LB_SETCURSEL           (WM_USER+7)
#define LB_GETSEL              (WM_USER+8)
#define LB_GETCURSEL           (WM_USER+9)
#define LB_GETTEXT             (WM_USER+10)
#define LB_GETTEXTLEN          (WM_USER+11)
#define LB_GETCOUNT            (WM_USER+12)
#define LB_SELECTSTRING        (WM_USER+13)
#define LB_DIR                 (WM_USER+14)
#define LB_GETTOPINDEX         (WM_USER+15)
#define LB_FINDSTRING          (WM_USER+16)
#define LB_GETSELCOUNT         (WM_USER+17)
#define LB_GETSELITEMS         (WM_USER+18)
#define LB_SETTABSTOPS         (WM_USER+19)
#define LB_GETHORIZONTALEXTENT (WM_USER+20)
#define LB_SETHORIZONTALEXTENT (WM_USER+21)
#define LB_SETCOLUMNWIDTH      (WM_USER+22)
#define LB_SETTOPINDEX         (WM_USER+24)
#define LB_GETITEMRECT         (WM_USER+25)
#define LB_GETITEMDATA         (WM_USER+26)
#define LB_SETITEMDATA         (WM_USER+27)
#define LB_SELITEMRANGE        (WM_USER+28)
#define LB_SETCARETINDEX       (WM_USER+31)
#define LB_GETCARETINDEX       (WM_USER+32)
#define LB_SETITEMHEIGHT       (WM_USER+33)
#define LB_GETITEMHEIGHT       (WM_USER+34)
#define LB_FINDSTRINGEXACT     (WM_USER+35)

/* Listbox notification codes */
#define LBN_ERRSPACE        (-2)
#define LBN_SELCHANGE       1
#define LBN_DBLCLK          2
#define LBN_SELCANCEL       3
#define LBN_SETFOCUS        4
#define LBN_KILLFOCUS       5

/* Listbox notification messages */
#define WM_VKEYTOITEM       0x002E
#define WM_CHARTOITEM       0x002F

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
#define CB_GETEDITSEL            (WM_USER+0)
#define CB_LIMITTEXT             (WM_USER+1)
#define CB_SETEDITSEL            (WM_USER+2)
#define CB_ADDSTRING             (WM_USER+3)
#define CB_DELETESTRING          (WM_USER+4)
#define CB_DIR                   (WM_USER+5)
#define CB_GETCOUNT              (WM_USER+6)
#define CB_GETCURSEL             (WM_USER+7)
#define CB_GETLBTEXT             (WM_USER+8)
#define CB_GETLBTEXTLEN          (WM_USER+9)
#define CB_INSERTSTRING          (WM_USER+10)
#define CB_RESETCONTENT          (WM_USER+11)
#define CB_FINDSTRING            (WM_USER+12)
#define CB_SELECTSTRING          (WM_USER+13)
#define CB_SETCURSEL             (WM_USER+14)
#define CB_SHOWDROPDOWN          (WM_USER+15)
#define CB_GETITEMDATA           (WM_USER+16)
#define CB_SETITEMDATA           (WM_USER+17)
#define CB_GETDROPPEDCONTROLRECT (WM_USER+18)
#define CB_SETITEMHEIGHT         (WM_USER+19)
#define CB_GETITEMHEIGHT         (WM_USER+20)
#define CB_SETEXTENDEDUI         (WM_USER+21)
#define CB_GETEXTENDEDUI         (WM_USER+22)
#define CB_GETDROPPEDSTATE       (WM_USER+23)
#define CB_FINDSTRINGEXACT       (WM_USER+24)

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
#define ES_NOHISESEL    0x00000100L
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

typedef int (CALLBACK *EDITWORDBREAKPROC)(LPSTR lpch, int ichCurrent,
					  int cch, int code);

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


#define WM_DRAWITEM         0x002B

typedef struct tagDRAWITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemAction;
    UINT        itemState;
    HWND        hwndItem;
    HDC         hDC;
    RECT        rcItem WINE_PACKED;
    DWORD       itemData WINE_PACKED;
} DRAWITEMSTRUCT;
typedef DRAWITEMSTRUCT NEAR* PDRAWITEMSTRUCT;
typedef DRAWITEMSTRUCT FAR* LPDRAWITEMSTRUCT;

#define WM_MEASUREITEM      0x002C

typedef struct tagMEASUREITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    UINT        itemID;
    UINT        itemWidth;
    UINT        itemHeight;
    DWORD       itemData WINE_PACKED;
} MEASUREITEMSTRUCT;
typedef MEASUREITEMSTRUCT NEAR* PMEASUREITEMSTRUCT;
typedef MEASUREITEMSTRUCT FAR* LPMEASUREITEMSTRUCT;

#define WM_DELETEITEM       0x002D

typedef struct tagDELETEITEMSTRUCT
{
    UINT       CtlType;
    UINT       CtlID;
    UINT       itemID;
    HWND       hwndItem;
    DWORD      itemData;
} DELETEITEMSTRUCT;
typedef DELETEITEMSTRUCT NEAR* PDELETEITEMSTRUCT;
typedef DELETEITEMSTRUCT FAR* LPDELETEITEMSTRUCT;

#define WM_COMPAREITEM      0x0039

typedef struct tagCOMPAREITEMSTRUCT
{
    UINT        CtlType;
    UINT        CtlID;
    HWND        hwndItem;
    UINT        itemID1;
    DWORD       itemData1;
    UINT        itemID2;
    DWORD       itemData2 WINE_PACKED;
} COMPAREITEMSTRUCT;
typedef COMPAREITEMSTRUCT NEAR* PCOMPAREITEMSTRUCT;
typedef COMPAREITEMSTRUCT FAR* LPCOMPAREITEMSTRUCT;

/* Virtual key codes */
#define VK_LBUTTON          0x01
#define VK_RBUTTON          0x02
#define VK_CANCEL           0x03
#define VK_MBUTTON          0x04
#define VK_BACK             0x08
#define VK_TAB              0x09
#define VK_CLEAR            0x0C
#define VK_RETURN           0x0D
#define VK_SHIFT            0x10
#define VK_CONTROL          0x11
#define VK_MENU             0x12
#define VK_PAUSE            0x13
#define VK_CAPITAL          0x14
#define VK_ESCAPE           0x1B
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
#define VK_PRINT            0x2A
#define VK_EXECUTE          0x2B
#define VK_SNAPSHOT         0x2C
#define VK_INSERT           0x2D
#define VK_DELETE           0x2E
#define VK_HELP             0x2F
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
#define VK_NUMLOCK          0x90
#define VK_SCROLL           0x91

  
#define LMEM_FIXED          0   
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00
#define LMEM_WINE_ALIGN     0x1000

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

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)


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
typedef struct tagMETAHEADER
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
typedef struct tagMETARECORD
{
    DWORD      rdSize;
    WORD       rdFunction;
    WORD       rdParam[1];
} METARECORD;
typedef METARECORD *PMETARECORD;
typedef METARECORD *LPMETARECORD;

/* Handle table structure */
typedef struct tagHANDLETABLE
{
    HANDLE     objectHandle[1];
} HANDLETABLE;
typedef HANDLETABLE *PHANDLETABLE;
typedef HANDLETABLE *LPHANDLETABLE;

/* Clipboard metafile picture structure */
typedef struct tagMETAFILEPICT
{
    INT        mm;
    INT        xExt;
    INT        yExt;
    HMETAFILE  hMF;
} METAFILEPICT;
typedef METAFILEPICT *LPMETAFILEPICT;

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
#define META_SETTEXTOUT              0x0A32
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

/* Debugging support (DEBUG SYSTEM ONLY) */
typedef struct tagWINDEBUGINFO
{
    UINT    flags;
    DWORD   dwOptions;
    DWORD   dwFilter;
    char    achAllocModule[8];
    DWORD   dwAllocBreak;
    DWORD   dwAllocCount;
} WINDEBUGINFO;
typedef WINDEBUGINFO FAR* LPWINDEBUGINFO;

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


#ifndef WINELIB
#pragma pack(4)
#endif


#define F(ret,name) ret name(void);
#define Fa(ret,name,t1,a1) ret name(t1);
#define Fb(ret,name,t1,a1,t2,a2) ret name(t1,t2);
#define Fc(ret,name,t1,a1,t2,a2,t3,a3) ret name(t1,t2,t3);
#define Fd(ret,name,t1,a1,t2,a2,t3,a3,t4,a4) ret name(t1,t2,t3,t4);
#define Fe(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) ret name(t1,t2,t3,t4,t5);
#define Ff(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) ret name(t1,t2,t3,t4,t5,t6);
#define Fg(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) ret name(t1,t2,t3,t4,t5,t6,t7);
#define Fh(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) ret name(t1,t2,t3,t4,t5,t6,t7,t8);
#define Fi(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9) ret name(t1,t2,t3,t4,t5,t6,t7,t8,t9);
#define Fj(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10) ret name(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10);
#define Fk(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11) ret name(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11);
#define Fl(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12) ret name(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12);
#define Fm(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12,t13,a13) ret name(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13);
#define Fn(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12,t13,a13,t14,a14) ret name(t1,t2,t3,t4,t5,t6,t7,t8,t9,t10,t11,t12,t13,t14);

int wsprintf(LPSTR a,LPSTR b,...);

/* Implemented functions */

F(BOOL,AnyPopup)
F(BOOL,CloseClipboard)
F(BOOL,EmptyClipboard)
F(BOOL,GetInputState)
F(BOOL,InSendMessage)
F(BOOL,SetDeskPattern)
F(DWORD,GetCurrentTime)
F(DWORD,GetDialogBaseUnits)
F(DWORD,GetMenuCheckMarkDimensions)
F(DWORD,GetMessagePos)
F(DWORD,GetTickCount)
F(HANDLE,GetCurrentTask)
F(HMENU,CreateMenu)
F(HMENU,CreatePopupMenu)
F(HWND,GetActiveWindow)
F(HWND,GetCapture)
F(HWND,GetClipboardOwner)
F(HWND,GetClipboardViewer)
F(HWND,GetDesktopHwnd)
F(HWND,GetDesktopWindow)
F(HWND,GetFocus)
F(HWND,GetOpenClipboardWindow)
F(HWND,GetSysModalWindow)
F(LONG,GetMessageExtraInfo)
F(LONG,GetMessageTime)
F(LONG,GetVersion)
F(LONG,GetWinFlags)
F(LPINT,GetThresholdEvent)
F(LPSTR,ValidateFreeSpaces)
F(SEGPTR,GetDOSEnvironment)
F(WORD,GetCaretBlinkTime)
F(WORD,GetCurrentPDB)
F(WORD,GetDoubleClickTime)
F(WORD,GetNumTasks)
F(WORD,GetTaskDS)
F(int,CountClipboardFormats)
F(int,GetKBCodePage)
F(int,GetThresholdStatus)
F(int,OpenSound)
F(int,ProfInsChk)
F(int,StartSound)
F(int,StopSound)
F(int,SyncAllVoices)
F(void,CloseSound)
F(void,DebugBreak)
F(void,DestroyCaret)
F(void,ProfClear)
F(void,ProfFinish)
F(void,ProfFlush)
F(void,ProfStart)
F(void,ProfStop)
F(void,ReleaseCapture)
F(void,SwitchStackBack)
F(void,ValidateCodeSegments)
F(void,WaitMessage)
F(void,Yield)
Fa(ATOM,AddAtom,SEGPTR,a)
Fa(ATOM,DeleteAtom,ATOM,a)
Fa(ATOM,FindAtom,SEGPTR,a)
Fa(ATOM,GlobalAddAtom,SEGPTR,a)
Fa(ATOM,GlobalDeleteAtom,ATOM,a)
Fa(ATOM,GlobalFindAtom,SEGPTR,a)
Fa(ATOM,RegisterClass,LPWNDCLASS,a) 
Fa(BOOL,BringWindowToTop,HWND,a)
Fa(BOOL,DeleteDC,HDC,a)
Fa(BOOL,DeleteMetaFile,HMETAFILE,a)
Fa(BOOL,DeleteObject,HANDLE,a)
Fa(BOOL,DestroyCursor,HCURSOR,a)
Fa(BOOL,DestroyIcon,HICON,a)
Fa(BOOL,DestroyMenu,HMENU,a)
Fa(BOOL,DestroyWindow,HWND,a)
Fa(BOOL,EnableHardwareInput,BOOL,a)
Fa(BOOL,EndDeferWindowPos,HDWP,hWinPosInfo)
Fa(BOOL,FreeModule,HANDLE,a)
Fa(BOOL,FreeResource,HANDLE,a)
Fa(BOOL,GlobalUnWire,HGLOBAL,a)
Fa(BOOL,GlobalUnlock,HGLOBAL,a)
Fa(BOOL,IsBadCodePtr,SEGPTR,a)
Fa(BOOL,IsCharAlpha,char,ch)
Fa(BOOL,IsCharAlphaNumeric,char,ch)
Fa(BOOL,IsCharLower,char,ch)
Fa(BOOL,IsCharUpper,char,ch)
Fa(BOOL,IsClipboardFormatAvailable,WORD,a)
Fa(BOOL,IsIconic,HWND,a)
Fa(BOOL,IsRectEmpty,LPRECT,a)
Fa(BOOL,IsTwoByteCharPrefix,char,a)
Fa(BOOL,IsWindow,HWND,a)
Fa(BOOL,IsWindowEnabled,HWND,a)
Fa(BOOL,IsWindowVisible,HWND,a)
Fa(BOOL,IsZoomed,HWND,a)
Fa(BOOL,LocalUnlock,HANDLE,a)
Fa(BOOL,OpenClipboard,HWND,a)
Fa(BOOL,OpenIcon,HWND,a)
Fa(BOOL,RemoveFontResource,LPSTR,a)
Fa(BOOL,SetDeskWallPaper,LPSTR,a)
Fa(BOOL,SetErrorMode,WORD,a)
Fa(BOOL,SetMessageQueue,int,a)
Fa(BOOL,SetWinDebugInfo,LPWINDEBUGINFO,a)
Fa(BOOL,SwapMouseButton,BOOL,a)
Fa(BOOL,TranslateMessage,LPMSG,a)
Fa(BOOL,UnhookWindowsHookEx,HHOOK,a)
Fa(BOOL,UnrealizeObject,HBRUSH,a)
Fa(BYTE,GetTempDrive,BYTE,a)
Fa(COLORREF,GetBkColor,HDC,a)
Fa(COLORREF,GetSysColor,short,a)
Fa(COLORREF,GetTextColor,HDC,a)
Fa(DWORD,GetAspectRatioFilter,HDC,a)
Fa(DWORD,GetBitmapDimension,HBITMAP,a)
Fa(DWORD,GetBrushOrg,HDC,a)
Fa(DWORD,GetCurrentPosition,HDC,a)
Fa(DWORD,GetDCOrg,HDC,a)
Fa(DWORD,GetFreeSpace,WORD,a)
Fa(DWORD,GetHeapSpaces,HMODULE,a)
Fa(DWORD,GetSelectorBase,WORD,a)
Fa(DWORD,GetSelectorLimit,WORD,a)
Fa(DWORD,GetViewportExt,HDC,a)
Fa(DWORD,GetViewportOrg,HDC,a)
Fa(DWORD,GetWindowExt,HDC,a)
Fa(DWORD,GetWindowOrg,HDC,a)
Fa(DWORD,GlobalCompact,DWORD,a)
Fa(DWORD,GlobalDOSAlloc,DWORD,a)
Fa(DWORD,GlobalHandle,WORD,a)
Fa(DWORD,GlobalSize,HGLOBAL,a)
Fa(DWORD,OemKeyScan,WORD,a)
Fa(FARPROC,LocalNotify,FARPROC,a)
Fa(HANDLE,CreateMetaFile,LPSTR,a)
Fa(HANDLE,GetAtomHandle,ATOM,a)
Fa(HANDLE,GetClipboardData,WORD,a)
Fa(HANDLE,GetCodeHandle,FARPROC,a)
Fa(HANDLE,GetMetaFile,LPSTR,a)
Fa(HANDLE,GetMetaFileBits,HANDLE,a)
Fa(HANDLE,GetModuleHandle,LPCSTR,a)
Fa(HANDLE,GetStockObject,int,a)
Fa(HANDLE,GetWindowTask,HWND,a)
Fa(HANDLE,LoadLibrary,LPCSTR,a)
Fa(HANDLE,LocalFree,HANDLE,a)
Fa(HANDLE,LocalHandle,WORD,a)
Fa(HANDLE,SetMetaFileBits,HANDLE,a)
Fa(HBITMAP,CreateBitmapIndirect,BITMAP FAR*,a)
Fa(HBRUSH,CreateBrushIndirect,LOGBRUSH FAR*,a)
Fa(HBRUSH,CreatePatternBrush,HBITMAP,a)
Fa(HBRUSH,CreateSolidBrush,DWORD,a)
Fa(HCURSOR,SetCursor,HCURSOR,a)
Fa(HDC,CreateCompatibleDC,HDC,a)
Fa(HDC,GetDC,HWND,a)
Fa(HDC,GetDCState,HDC,a)
Fa(HDC,GetWindowDC,HWND,a)
Fa(HDWP,BeginDeferWindowPos,INT,nNumWindows)
Fa(HFONT,CreateFontIndirect,LOGFONT FAR*,a)
Fa(HGLOBAL,GetTaskQueue,HTASK,a)
Fa(HGLOBAL,GlobalFree,HGLOBAL,a)
Fa(HGLOBAL,GlobalLRUNewest,HGLOBAL,a)
Fa(HGLOBAL,GlobalLRUOldest,HGLOBAL,a)
Fa(HGLOBAL,LockSegment,HGLOBAL,a)
Fa(HMENU,GetMenu,HWND,a)
Fa(HMENU,LoadMenuIndirect,LPSTR,a)
Fa(HMETAFILE,CloseMetaFile,HANDLE,a)
Fa(HMODULE, GetExePtr,HANDLE,a)
Fa(HPALETTE,CreatePalette,LPLOGPALETTE,a)
Fa(HPEN,CreatePenIndirect,LOGPEN FAR*,a)
Fa(HRGN,CreateEllipticRgnIndirect,LPRECT,a)
Fa(HRGN,CreateRectRgnIndirect,LPRECT,a)
Fa(HRGN,GetClipRgn,HDC,a)
Fa(HRGN,InquireVisRgn,HDC,a)
Fa(HRGN,SaveVisRgn,HDC,a)
Fa(HWND,GetLastActivePopup,HWND,a)
Fa(HWND,GetParent,HWND,a)
Fa(HWND,GetTopWindow,HWND,a)
Fa(HWND,SetActiveWindow,HWND,a)
Fa(HWND,SetCapture,HWND,a)
Fa(HWND,SetClipboardViewer,HWND,a)
Fa(HWND,SetFocus,HWND,a)
Fa(HWND,SetSysModalWindow,HWND,a)
Fa(HWND,WindowFromPoint,POINT,a)
Fa(INT,_lclose,INT,a)
Fa(INT,lstrlen,LPCSTR,a)
Fa(LONG,DispatchMessage,LPMSG,msg)
Fa(LONG,SetSwapAreaSize,WORD,a)
Fa(LPSTR,AnsiLower,LPSTR,a)
Fa(LPSTR,AnsiUpper,LPSTR,a)
Fa(LPSTR,GlobalLock,HGLOBAL,a)
Fa(LPSTR,LockResource,HANDLE,a)
Fa(SEGPTR,AnsiNext,SEGPTR,a)
Fa(SEGPTR,GlobalWire,HGLOBAL,a)
Fa(SEGPTR,WIN16_GlobalLock,HGLOBAL,a)
Fa(SEGPTR,WIN16_LockResource,HANDLE,a)
Fa(UINT,GDIRealizePalette,HDC,a)
Fa(UINT,RealizePalette,HDC,a)
Fa(WORD,AllocCStoDSAlias,WORD,a)
Fa(WORD,AllocDStoCSAlias,WORD,a)
Fa(WORD,AllocSelector,WORD,a)
Fa(WORD,AllocSelectorArray,WORD,a)
Fa(WORD,ArrangeIconicWindows,HWND,a)
Fa(WORD,EnumClipboardFormats,WORD,a)
Fa(WORD,FarGetOwner,HANDLE,a)
Fa(WORD,FreeSelector,WORD,a)
Fa(WORD,GetBkMode,HDC,a)
Fa(WORD,GetDriveType,INT,a)
Fa(WORD,GetMapMode,HDC,a)
Fa(WORD,GetMenuItemCount,HMENU,a)
Fa(WORD,GetPolyFillMode,HDC,a)
Fa(WORD,GetROP2,HDC,a)
Fa(WORD,GetRelAbs,HDC,a)
Fa(WORD,GetStretchBltMode,HDC,a)
Fa(WORD,GetSystemPaletteUse,HDC,a)
Fa(WORD,GetTextAlign,HDC,a)
Fa(WORD,GlobalDOSFree,WORD,a)
Fa(WORD,GlobalFlags,HGLOBAL,a)
Fa(WORD,GlobalPageLock,HGLOBAL,a)
Fa(WORD,GlobalPageUnlock,HGLOBAL,a)
Fa(WORD,InitAtomTable,WORD,a)
Fa(WORD,LocalCompact,WORD,a)
Fa(WORD,LocalFlags,HLOCAL,a)
Fa(WORD,LocalLock,HLOCAL,a)
Fa(WORD,LocalSize,HLOCAL,a)
Fa(WORD,RealizeDefaultPalette,HDC,a)
Fa(WORD,RegisterClipboardFormat,LPCSTR,a)
Fa(WORD,RegisterWindowMessage,SEGPTR,a)
Fa(WORD,SetHandleCount,WORD,a)
Fa(WORD,VkKeyScan,WORD,a)
Fa(int,AddFontResource,LPSTR,a)
Fa(int,Catch,LPCATCHBUF,a)
Fa(int,ClearCommBreak,int,a)
Fa(int,CloseComm,int,a)
Fa(int,CountVoiceNotes,int,a)
Fa(int,GetAsyncKeyState,int,a)
Fa(int,GetDlgCtrlID,HWND,a)
Fa(int,GetKeyState,int,a)
Fa(int,GetKeyboardType,int,a)
Fa(int,GetModuleUsage,HANDLE,a)
Fa(int,GetSystemMetrics,WORD,a)
Fa(int,GetWindowTextLength,HWND,a)
Fa(int,RestoreVisRgn,HDC,a)
Fa(int,SaveDC,HDC,a)
Fa(int,SetCommBreak,int,a)
Fa(int,SetCommState,DCB*,a)
Fa(int,ShowCursor,BOOL,a)
Fa(int,UpdateColors,HDC,a)
Fa(int,WaitSoundState,int,a)
Fa(short,GetTextCharacterExtra,HDC,a)
Fa(void,ClipCursor,LPRECT,a)
Fa(void,CloseWindow,HWND,a)
Fa(void,DrawMenuBar,HWND,a)
Fa(void,FatalExit,int,a)
Fa(void,FreeLibrary,HANDLE,a)
Fa(void,FreeProcInstance,FARPROC,a)
Fa(void,GetCaretPos,LPPOINT,a)
Fa(void,GetCursorPos,LPPOINT,a)
Fa(void,GetKeyboardState,BYTE FAR*,a)
Fa(void,GlobalFix,HGLOBAL,a)
Fa(void,GlobalFreeAll,HANDLE,a)
Fa(void,GlobalNotify,FARPROC,a)
Fa(void,GlobalUnfix,HGLOBAL,a)
Fa(void,HideCaret,HWND,a)
Fa(void,LimitEmsPages,DWORD,a)
Fa(void,MessageBeep,WORD,a)
Fa(void,OutputDebugString,LPSTR,a)
Fa(void,PostQuitMessage,int,a)
Fa(void,ReplyMessage,LONG,a)
Fa(void,SetCaretBlinkTime,WORD,a)
Fa(void,SetConvertHook,BOOL,a)
Fa(void,SetDoubleClickTime,WORD,a)
Fa(void,SetKeyboardState,BYTE FAR*,a)
Fa(void,SetRectEmpty,LPRECT,a)
Fa(void,ShowCaret,HWND,a)
Fa(void,SwapRecording,WORD,a)
Fa(void,UnlockSegment,HGLOBAL,a)
Fa(void,UpdateWindow,HWND,a)
Fb(BOOL,CallMsgFilter,SEGPTR,a,short,b)
Fb(BOOL,ChangeClipboardChain,HWND,a,HWND,b)
Fb(BOOL,EnableWindow,HWND,a,BOOL,b)
Fb(BOOL,EnumWindows,FARPROC,a,LONG,b)
Fb(BOOL,EqualRect,LPRECT,a,LPRECT,b)
Fb(BOOL,EqualRgn,HRGN,a,HRGN,b)
Fb(BOOL,ExitWindows,DWORD,dwReserved,WORD,wReturnCode)
Fb(BOOL,FlashWindow,HWND,a,BOOL,b)
Fb(BOOL,GetBitmapDimensionEx,HBITMAP,a,LPSIZE,b)
Fb(BOOL,GetBrushOrgEx,HDC,a,LPPOINT,b)
Fb(BOOL,GetCurrentPositionEx,HDC,a,LPPOINT,b)
Fb(BOOL,GetTextMetrics,HDC,a,LPTEXTMETRIC,b)
Fb(BOOL,GetViewportExtEx,HDC,a,LPPOINT,b)
Fb(BOOL,GetViewportOrgEx,HDC,a,LPPOINT,b)
Fb(BOOL,GetWinDebugInfo,LPWINDEBUGINFO,a,UINT,b)
Fb(BOOL,GetWindowExtEx,HDC,a,LPPOINT,b)
Fb(BOOL,GetWindowOrgEx,HDC,a,LPPOINT,b)
Fb(BOOL,GetWindowPlacement,HWND,a,LPWINDOWPLACEMENT,b)
Fb(BOOL,InvertRgn,HDC,a,HRGN,b)
Fb(BOOL,IsBadHugeReadPtr,SEGPTR,a,DWORD,b)
Fb(BOOL,IsBadHugeWritePtr,SEGPTR,a,DWORD,b)
Fb(BOOL,IsBadReadPtr,SEGPTR,a,WORD,b)
Fb(BOOL,IsBadStringPtr,SEGPTR,a,WORD,b)
Fb(BOOL,IsBadWritePtr,SEGPTR,a,WORD,b)
Fb(BOOL,IsChild,HWND,a,HWND,b)
Fb(BOOL,IsDialogMessage,HWND,a,LPMSG,b)
Fb(BOOL,KillSystemTimer,HWND,a,WORD,b)
Fb(BOOL,KillTimer,HWND,a,WORD,b)
Fb(BOOL,OemToAnsi,LPSTR,a,LPSTR,b)
Fb(BOOL,PaintRgn,HDC,a,HRGN,b)
Fb(BOOL,PlayMetaFile,HDC,a,HANDLE,b)
Fb(BOOL,PtInRect,LPRECT,a,POINT,b)
Fb(BOOL,RectInRegion,HRGN,a,LPRECT,b)
Fb(BOOL,RectVisible,HDC,a,LPRECT,b)
Fb(BOOL,ResizePalette,HPALETTE,a,WORD,b)
Fb(BOOL,RestoreDC,HDC,a,short,b)
Fb(BOOL,SetConvertParams,int,a,int,b)
Fb(BOOL,SetMenu,HWND,a,HMENU,b)
Fb(BOOL,SetWindowPlacement,HWND,a,LPWINDOWPLACEMENT,b)
Fb(BOOL,ShowWindow,HWND,a,int,b) 
Fb(BOOL,TranslateMDISysAccel,HWND,a,LPMSG,b)
Fb(BOOL,UnhookWindowsHook,short,a,FARPROC,b)
Fb(BOOL,UnregisterClass,SEGPTR,a,HANDLE,b)
Fb(DWORD,GetNearestColor,HDC,a,DWORD,b)
Fb(DWORD,SetBkColor,HDC,a,COLORREF,b)
Fb(DWORD,SetMapperFlags,HDC,a,DWORD,b)
Fb(DWORD,SetTextColor,HDC,a,DWORD,b)
Fb(DWORD,SizeofResource,HANDLE,a,HRSRC,b)
Fb(FARPROC,GetProcAddress,HANDLE,a,SEGPTR,b)
Fb(FARPROC,MakeProcInstance,FARPROC,a,HANDLE,b)
Fb(FARPROC,SetWindowsHook,short,a,FARPROC,b)
Fb(HANDLE,CopyMetaFile,HANDLE,a,LPSTR,b)
Fb(HANDLE,GetProp,HWND,a,SEGPTR,b)
Fb(HANDLE,LoadAccelerators,HANDLE,a,SEGPTR,b)
Fb(HANDLE,LoadModule,LPCSTR,a,LPVOID,b)
Fb(HANDLE,LoadResource,HANDLE,a,HANDLE,b)
Fb(HANDLE,LocalAlloc,WORD,a,WORD,b)
Fb(HANDLE,RemoveProp,HWND,a,SEGPTR,b)
Fb(HANDLE,SelectObject,HDC,a,HANDLE,b)
Fb(HANDLE,SetClipboardData,WORD,a,HANDLE,b)
Fb(HBITMAP,LoadBitmap,HANDLE,a,SEGPTR,b)
Fb(HBRUSH,CreateDIBPatternBrush,HANDLE,a,WORD,b)
Fb(HBRUSH,CreateHatchBrush,short,a,COLORREF,b)
Fb(HCURSOR,LoadCursor,HANDLE,a,SEGPTR,b)
Fb(HDC,BeginPaint,HWND,a,LPPAINTSTRUCT,b) 
Fb(HGLOBAL,GlobalAlloc,WORD,a,DWORD,b)
Fb(HGLOBAL,SetTaskQueue,HTASK,a,HGLOBAL,b)
Fb(HICON,LoadIcon,HANDLE,a,SEGPTR,b)
Fb(HMENU,GetSubMenu,HMENU,a,short,b)
Fb(HMENU,GetSystemMenu,HWND,a,BOOL,b)
Fb(HMENU,LoadMenu,HANDLE,a,SEGPTR,b)
Fb(HMENU,LookupMenuHandle,HMENU,a,INT,b)
Fb(HPALETTE,GDISelectPalette,HDC,a,HPALETTE,b)
Fb(HWND,ChildWindowFromPoint,HWND,a,POINT,b)
Fb(HWND,FindWindow,SEGPTR,a,LPSTR,b)
Fb(HWND,GetDlgItem,HWND,a,WORD,b)
Fb(HWND,GetNextWindow,HWND,a,WORD,b)
Fb(HWND,GetWindow,HWND,a,WORD,b)
Fb(HWND,SetParent,HWND,a,HWND,b)
Fb(INT,AnsiToOem,LPSTR,a,LPSTR,b)
Fb(INT,_lcreat,LPSTR,a,INT,b)
Fb(INT,_lopen,LPSTR,a,INT,b)
Fb(INT,lstrcmp,LPCSTR,a,LPCSTR,b )
Fb(INT,lstrcmpi,LPCSTR,a,LPCSTR,b )
Fb(LONG,EscapeCommFunction,int,a,int,b)
Fb(LONG,GetClassLong,HWND,a,short,b)
Fb(LONG,GetWindowLong,HWND,a,short,b)
Fb(SEGPTR,AnsiPrev,SEGPTR,a,SEGPTR,b)
Fb(SEGPTR,lstrcat,SEGPTR,a,SEGPTR,b)
Fb(SEGPTR,lstrcpy,SEGPTR,a,SEGPTR,b)
Fb(WORD FAR*,SetCommEventMask,int,a,WORD,b)
Fb(WORD,AnsiLowerBuff,LPSTR,a,WORD,b)
Fb(WORD,AnsiUpperBuff,LPSTR,a,WORD,b)
Fb(WORD,ChangeSelector,WORD,a,WORD,b)
Fb(WORD,GetClassWord,HWND,a,short,b)
Fb(WORD,GetCommEventMask,int,a,int,b)
Fb(WORD,GetMenuItemID,HMENU,a,int,b)
Fb(WORD,GetNearestPaletteIndex,HPALETTE,a,DWORD,b)
Fb(WORD,GetSystemDirectory,LPSTR,a,WORD,b)
Fb(WORD,GetWindowWord,HWND,a,short,b)
Fb(WORD,GetWindowsDirectory,LPSTR,a,WORD,b)
Fb(WORD,IsDlgButtonChecked,HWND,a,WORD,b)
Fb(WORD,LocalShrink,HANDLE,a,WORD,b)
Fb(WORD,MapVirtualKey,WORD,a,WORD,b)
Fb(WORD,PrestoChangoSelector,WORD,a,WORD,b)
Fb(WORD,SetBkMode,HDC,a,WORD,b)
Fb(WORD,SetMapMode,HDC,a,WORD,b)
Fb(WORD,SetPolyFillMode,HDC,a,WORD,b)
Fb(WORD,SetROP2,HDC,a,WORD,b)
Fb(WORD,SetRelAbs,HDC,a,WORD,b)
Fb(WORD,SetSelectorBase,WORD,a,DWORD,b)
Fb(WORD,SetSelectorLimit,WORD,a,DWORD,b)
Fb(WORD,SetStretchBltMode,HDC,a,WORD,b)
Fb(WORD,SetSystemPaletteUse,HDC,a,WORD,b)
Fb(WORD,SetTextAlign,HDC,a,WORD,b)
Fb(WORD,WinExec,LPSTR,a,WORD,b)
Fb(int,AccessResource,HANDLE,a,HANDLE,b)
Fb(int,BuildCommDCB,LPSTR,a,DCB*,b)
Fb(int,ConvertRequest,HWND,a,LPKANJISTRUCT,b)
Fb(int,EnumProps,HWND,a,FARPROC,b)
Fb(int,ExcludeUpdateRgn,HDC,a,HWND,b)
Fb(int,FlushComm,int,a,int,b)
Fb(int,GetClipBox,HDC,a,LPRECT,b)
Fb(int,GetCommError,int,a,COMSTAT*,b)
Fb(int,GetCommState,int,a,DCB*,b)
Fb(int,GetDeviceCaps,HDC,a,WORD,b)
Fb(int,GetPriorityClipboardFormat,WORD FAR*,a,short,b)
Fb(int,GetRgnBox,HRGN,a,LPRECT,b)
Fb(int,GetScrollPos,HWND,a,int,b)
Fb(int,ReleaseDC,HWND,a,HDC,b)
Fb(int,SelectClipRgn,HDC,a,HRGN,b)
Fb(int,SelectVisRgn,HDC,a,HRGN,b)
Fb(int,SetSoundNoise,int,a,int,b)
Fb(int,SetVoiceQueueSize,int,a,int,b)
Fb(int,SetVoiceThreshold,int,a,int,b)
Fb(int,Throw,LPCATCHBUF,a,int,b)
Fb(int,TransmitCommChar,int,a,char,b)
Fb(int,UngetCommChar,int,a,char,b)
Fb(short,SetTextCharacterExtra,HDC,a,short,b)
Fb(void,ClientToScreen,HWND,a,LPPOINT,b)
Fb(void,CopyRect,LPRECT,a,LPRECT,b)
Fb(void,DrawFocusRect,HDC,a,LPRECT,b)
Fb(void,EndDialog,HWND,a,short,b)
Fb(void,EndPaint,HWND,a,LPPAINTSTRUCT,b)
Fb(void,FarSetOwner,HANDLE,a,WORD,b)
Fb(void,GetClientRect,HWND,a,LPRECT,b)
Fb(void,GetCodeInfo,FARPROC,lpProc,LPVOID,lpSegInfo)
Fb(void,GetWindowRect,HWND,a,LPRECT,b)
Fb(void,InvertRect,HDC,a,LPRECT,b)
Fb(void,MapDialogRect,HWND,a,LPRECT,b)
Fb(void,ProfSampRate,int,a,int,b)
Fb(void,ProfSetup,int,a,int,b)
Fb(void,ScreenToClient,HWND,a,LPPOINT,b)
Fb(void,SetCaretPos,short,a,short,b)
Fb(void,SetCursorPos,short,a,short,b)
Fb(void,SetDCState,HDC,a,HDC,b)
Fb(void,SetWindowText,HWND,a,LPSTR,b)
Fb(void,ShowOwnedPopups,HWND,a,BOOL,b)
Fb(void,ValidateRect,HWND,a,LPRECT,b)
Fb(void,ValidateRgn,HWND,a,HRGN,b)
Fc(BOOL,CheckMenuItem,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,DPtoLP,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,DeleteMenu,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,DlgDirSelect,HWND,a,LPSTR,b,int,c)
Fc(BOOL,DlgDirSelectComboBox,HWND,a,LPSTR,b,int,c)
Fc(BOOL,EnableMenuItem,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,EnableScrollBar,HWND,a,INT,b,UINT,c)
Fc(BOOL,EnumChildWindows,HWND,a,FARPROC,b,LONG,c)
Fc(BOOL,EnumTaskWindows,HANDLE,a,FARPROC,b,LONG,c)
Fc(BOOL,FillRgn,HDC,a,HRGN,b,HBRUSH,c)
Fc(BOOL,GetClassInfo,HANDLE,a,SEGPTR,b,LPWNDCLASS,c)
Fc(BOOL,GetUpdateRect,HWND,a,LPRECT,b,BOOL,c)
Fc(BOOL,IntersectRect,LPRECT,a,LPRECT,b,LPRECT,c)
Fc(BOOL,LPtoDP,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,LineTo,HDC,a,short,b,short,c)
Fc(BOOL,LocalInit,WORD,a,WORD,b,WORD,c)
Fc(BOOL,Polygon,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,Polyline,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,PtInRegion,HRGN,a,short,b,short,c)
Fc(BOOL,PtVisible,HDC,a,short,b,short,c)
Fc(BOOL,RemoveMenu,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,SetProp,HWND,a,SEGPTR,b,HANDLE,c)
Fc(BOOL,SubtractRect,LPRECT,a,LPRECT,b,LPRECT,c)
Fc(BOOL,UnionRect,LPRECT,a,LPRECT,b,LPRECT,c)
Fc(BOOL,WriteProfileString,LPSTR,a,LPSTR,b,LPSTR,c)
Fc(DWORD,GetPixel,HDC,a,short,b,short,c)
Fc(DWORD,GetTextExtent,HDC,a,LPSTR,b,short,c)
Fc(DWORD,MoveTo,HDC,a,short,b,short,c)
Fc(DWORD,OffsetViewportOrg,HDC,a,short,b,short,c)
Fc(DWORD,OffsetWindowOrg,HDC,a,short,b,short,c)
Fc(DWORD,SetBitmapDimension,HBITMAP,a,short,b,short,c)
Fc(DWORD,SetBrushOrg,HDC,a,short,b,short,c)
Fc(DWORD,SetViewportExt,HDC,a,short,b,short,c)
Fc(DWORD,SetViewportOrg,HDC,a,short,b,short,c)
Fc(DWORD,SetWindowExt,HDC,a,short,b,short,c)
Fc(DWORD,SetWindowOrg,HDC,a,short,b,short,c)
Fc(FARPROC,SetResourceHandler,HANDLE,a,LPSTR,b,FARPROC,c)
Fc(HANDLE,AllocResource,HANDLE,a,HANDLE,b,DWORD,c)
Fc(HANDLE,FindResource,HANDLE,a,SEGPTR,b,SEGPTR,c)
Fc(HANDLE,LocalReAlloc,HANDLE,a,WORD,b,WORD,c)
Fc(HBITMAP,CreateCompatibleBitmap,HDC,a,short,b,short,c)
Fc(HBITMAP,CreateDiscardableBitmap,HDC,a,short,b,short,c)
Fc(HBRUSH,GetControlBrush,HWND,a,HDC,b,WORD,c)
Fc(HDC,GetDCEx,HWND,a,HRGN,b,DWORD,c)
Fc(HGLOBAL,GlobalReAlloc,HGLOBAL,a,DWORD,b,WORD,c)
Fc(HPALETTE,SelectPalette,HDC,a,HPALETTE,b,BOOL,c)
Fc(HPEN,CreatePen,short,a,short,b,COLORREF,c)
Fc(HRGN,CreatePolygonRgn,LPPOINT,a,short,b,short,c)
Fc(HWND,GetNextDlgGroupItem,HWND,a,HWND,b,BOOL,c)
Fc(HWND,GetNextDlgTabItem,HWND,a,HWND,b,BOOL,c)
Fc(INT,GetTextFace,HDC,a,INT,b,LPSTR,c)
Fc(INT,OpenFile,LPSTR,a,LPOFSTRUCT,b,WORD,c)
Fc(INT,_lread,INT,a,LPSTR,b,WORD,c)
Fc(INT,_lwrite,INT,a,LPCSTR,b,WORD,c)
Fc(LONG,GetBitmapBits,HBITMAP,a,LONG,b,LPSTR,c)
Fc(LONG,SetBitmapBits,HBITMAP,a,LONG,b,LPSTR,c)
Fc(LONG,SetClassLong,HWND,a,short,b,LONG,c)
Fc(LONG,SetWindowLong,HWND,a,short,b,LONG,c)
Fc(LONG,_hread,INT,a,LPSTR,b,LONG,c)
Fc(LONG,_hwrite,INT,a,LPCSTR,b,LONG,c)
Fc(LONG,_llseek,INT,a,LONG,b,INT,c)
Fc(SEGPTR,lstrcpyn,SEGPTR,a,SEGPTR,b,WORD,c)
Fc(WORD,GetAtomName,ATOM,a,LPSTR,b,short,c)
Fc(WORD,GetInternalWindowPos,HWND,a,LPRECT,b,LPPOINT,c)
Fc(WORD,GetMenuState,HMENU,a,WORD,b,WORD,c)
Fc(WORD,GetProfileInt,LPSTR,a,LPSTR,b,int,c)
Fc(WORD,GlobalGetAtomName,ATOM,a,LPSTR,b,short,c)
Fc(WORD,SelectorAccessRights,WORD,a,WORD,b,WORD,c)
Fc(WORD,SetClassWord,HWND,a,short,b,WORD,c)
Fc(WORD,SetWindowWord,HWND,a,short,b,WORD,c)
Fc(int,FillRect,HDC,a,LPRECT,b,HBRUSH,c)
Fc(int,FrameRect,HDC,a,LPRECT,b,HBRUSH,c)
Fc(int,GetClassName,HWND,a,LPSTR,b,short,c)
Fc(int,GetClipboardFormatName,WORD,a,LPSTR,b,short,c)
Fc(int,GetEnvironment,LPSTR,a,LPSTR,b,WORD,c)
Fc(int,GetInstanceData,HANDLE,a,WORD,b,int,c)
Fc(int,GetKeyNameText,LONG,a,LPSTR,b,int,c)
Fc(int,GetModuleFileName,HANDLE,a,LPSTR,b,short,c)
Fc(int,GetObject,HANDLE,a,int,b,LPSTR,c)
Fc(int,GetUpdateRgn,HWND,a,HRGN,b,BOOL,c)
Fc(int,GetWindowText,HWND,a,LPSTR,b,int,c)
Fc(int,MulDiv,int,a,int,b,int,c)
Fc(int,OffsetClipRgn,HDC,a,short,b,short,c)
Fc(int,OffsetRgn,HRGN,a,short,b,short,c)
Fc(int,OpenComm,LPSTR,a,WORD,b,WORD,c)
Fc(int,ReadComm,int,a,LPSTR,b,int,c)
Fc(int,SetEnvironment,LPSTR,a,LPSTR,b,WORD,c)
Fc(int,SetVoiceEnvelope,int,a,int,b,int,c)
Fc(int,SetVoiceSound,int,a,LONG,b,int,c)
Fc(int,TranslateAccelerator,HWND,a,HANDLE,b,LPMSG,c)
Fc(int,WriteComm,int,a,LPSTR,b,int,c)
Fc(int,wvsprintf,LPSTR,a,LPSTR,b,LPSTR,c)
Fc(short,SetTextJustification,HDC,a,short,b,short,c)
Fc(void,AdjustWindowRect,LPRECT,a,DWORD,b,BOOL,c)
Fc(void,AnsiToOemBuff,LPSTR,a,LPSTR,b,INT,c)
Fc(void,CheckDlgButton,HWND,a,WORD,b,WORD,c)
Fc(void,InflateRect,LPRECT,a,short,b,short,c)
Fc(void,InvalidateRect,HWND,a,LPRECT,b,BOOL,c)
Fc(void,InvalidateRgn,HWND,a,HRGN,b,BOOL,c)
Fc(void,OemToAnsiBuff,LPSTR,a,LPSTR,b,INT,c)
Fc(void,OffsetRect,LPRECT,a,short,b,short,c)
Fc(void,SetDlgItemText,HWND,a,WORD,b,SEGPTR,c)
Fc(void,SetSysColors,int,a,LPINT,b,COLORREF*,c)
Fc(void,ShowScrollBar,HWND,a,WORD,b,BOOL,c)
Fc(void,SwitchStackTo,WORD,a,WORD,b,WORD,c)
Fd(BOOL,AppendMenu,HMENU,a,WORD,b,WORD,c,LPSTR,d)
Fd(BOOL,DrawIcon,HDC,a,short,b,short,c,HICON,d)
Fd(BOOL,EnumMetaFile,HDC,a,LOCALHANDLE,b,FARPROC,c,BYTE FAR*,d)
Fd(BOOL,FloodFill,HDC,a,INT,b,INT,c,COLORREF,d)
Fd(BOOL,GetCharWidth,HDC,a,WORD,b,WORD,c,LPINT,d)
Fd(BOOL,GetMessage,SEGPTR,msg,HWND,b,WORD,c,WORD,d)
Fd(BOOL,GetTextExtentPoint,HDC,a,LPSTR,b,short,c,LPSIZE,d)
Fd(BOOL,HiliteMenuItem,HWND,a,HMENU,b,WORD,c,WORD,d)
Fd(BOOL,MoveToEx,HDC,a,short,b,short,c,LPPOINT,d)
Fd(BOOL,OffsetViewportOrgEx,HDC,a,short,b,short,c,LPPOINT,d)
Fd(BOOL,OffsetWindowOrgEx,HDC,a,short,b,short,c,LPPOINT,d)
Fd(BOOL,PolyPolygon,HDC,a,LPPOINT,b,LPINT,c,WORD,d)
Fd(BOOL,PostAppMessage,HANDLE,a,WORD,b,WORD,c,LONG,d)
Fd(BOOL,PostMessage,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(BOOL,RedrawWindow,HWND,a,LPRECT,b,HRGN,c,UINT,d)
Fd(BOOL,SetBitmapDimensionEx,HBITMAP,a,short,b,short,c,LPSIZE,d)
Fd(BOOL,SetViewportExtEx,HDC,a,short,b,short,c,LPSIZE,d)
Fd(BOOL,SetViewportOrgEx,HDC,a,short,b,short,c,LPPOINT,d)
Fd(BOOL,SetWindowExtEx,HDC,a,short,b,short,c,LPSIZE,d)
Fd(BOOL,SetWindowOrgEx,HDC,a,short,b,short,c,LPPOINT,d)
Fd(BOOL,WinHelp,HWND,hwndMain,LPSTR,lpszHelp,WORD,usCommand,DWORD,ulData)
Fd(BOOL,WritePrivateProfileString,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d)
Fd(COLORREF,SetPixel,HDC,a,short,b,short,c,COLORREF,d)
Fd(DWORD,CallNextHookEx,HHOOK,a,short,b,WPARAM,c,LPARAM,d)
Fd(DWORD,DefHookProc,short,a,WORD,b,DWORD,c,HHOOK FAR*,d)
Fd(HDC,CreateDC,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d)
Fd(HDC,CreateIC,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d)
Fd(HHOOK,SetWindowsHookEx,short,a,HOOKPROC,b,HINSTANCE,c,HTASK,d)
Fd(HRGN,CreateEllipticRgn,short,a,short,b,short,c,short,d)
Fd(HRGN,CreatePolyPolygonRgn,LPPOINT,a,LPINT,b,short,c,short,d)
Fd(HRGN,CreateRectRgn,short,a,short,b,short,c,short,d)
Fd(HWND,CreateDialog,HANDLE,a,SEGPTR,b,HWND,c,WNDPROC,d)
Fd(HWND,CreateDialogIndirect,HANDLE,a,SEGPTR,b,HWND,c,WNDPROC,d)
Fd(INT,GetTempFileName,BYTE,a,LPCSTR,b,UINT,c,LPSTR,d)
Fd(LONG,DefDlgProc,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(LONG,DefMDIChildProc,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(LONG,DefWindowProc,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(LONG,SendMessage,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(WORD,GetDlgItemInt,HWND,a,WORD,b,BOOL FAR*,c,BOOL,d)
Fd(WORD,GetPaletteEntries,HPALETTE,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(WORD,GetPrivateProfileInt,LPSTR,a,LPSTR,b,short,c,LPSTR,d)
Fd(WORD,GetSystemPaletteEntries,HDC,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(WORD,SetPaletteEntries,HPALETTE,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(WORD,SetSystemTimer,HWND,a,WORD,d,WORD,b,FARPROC,c)
Fd(WORD,SetTimer,HWND,a,WORD,d,WORD,b,FARPROC,c)
Fd(int,CombineRgn,HRGN,a,HRGN,b,HRGN,c,short,d)
Fd(int,DialogBox,HINSTANCE,a,SEGPTR,b,HWND,c,WNDPROC,d)
Fd(int,DialogBoxIndirect,HANDLE,a,HANDLE,b,HWND,c,WNDPROC,d)
Fd(int,EnumFonts,HDC,a,LPSTR,b,FARPROC,c,LPSTR,d)
Fd(int,EnumObjects,HDC,a,int,b,FARPROC,c,LPSTR,d)
Fd(int,GetDlgItemText,HWND,a,WORD,b,SEGPTR,c,WORD,d)
Fd(int,LoadString,HANDLE,a,WORD,b,LPSTR,c,int,d)
Fd(int,MessageBox,HWND,a,LPSTR,b,LPSTR,c,WORD,d)
Fd(int,SetScrollPos,HWND,a,int,b,int,c,BOOL,d)
Fd(int,SetVoiceNote,int,a,int,b,int,c,int,d)
Fd(void,AdjustWindowRectEx,LPRECT,a,DWORD,b,BOOL,c,DWORD,d)
Fd(void,AnimatePalette,HPALETTE,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(void,CheckRadioButton,HWND,a,WORD,b,WORD,c,WORD,d)
Fd(void,CreateCaret,HWND,a,HBITMAP,b,short,c,short,d)
Fd(void,FillWindow,HWND,a,HWND,b,HDC,c,HBRUSH,d)
Fd(void,GetScrollRange,HWND,a,int,b,LPINT,c,LPINT,d)
Fd(void,MapWindowPoints,HWND,a,HWND,b,LPPOINT,c,WORD,d)
Fd(void,PlayMetaFileRecord,HDC,a,LPHANDLETABLE,b,LPMETARECORD,c,WORD,d)
Fd(void,SetDlgItemInt,HWND,a,WORD,b,WORD,c,BOOL,d)
Fd(void,SetInternalWindowPos,HWND,a,WORD,b,LPRECT,c,LPPOINT,d)
Fe(BOOL,ChangeMenu,HMENU,a,WORD,b,LPSTR,c,WORD,d,WORD,e)
Fe(BOOL,Ellipse,HDC,a,int,b,int,c,int,d,int,e)
Fe(BOOL,ExtFloodFill,HDC,a,INT,b,INT,c,COLORREF,d,WORD,e)
Fe(BOOL,FrameRgn,HDC,a,HRGN,b,HBRUSH,e,int,c,int,d)
Fe(BOOL,InsertMenu,HMENU,a,WORD,b,WORD,c,WORD,d,LPSTR,e)
Fe(BOOL,ModifyMenu,HMENU,a,WORD,b,WORD,c,WORD,d,LPSTR,e)
Fe(BOOL,PeekMessage,LPMSG,a,HWND,b,WORD,c,WORD,d,WORD,e)
Fe(BOOL,Rectangle,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom)
Fe(BOOL,SetMenuItemBitmaps,HMENU,a,WORD,b,WORD,c,HBITMAP,d,HBITMAP,e)
Fe(BOOL,TextOut,HDC,a,short,b,short,c,LPSTR,d,short,e)
Fe(DWORD,GetTabbedTextExtent,HDC,a,LPSTR,b,int,c,int,d,LPINT,e)
Fe(DWORD,ScaleViewportExt,HDC,a,short,b,short,c,short,d,short,e)
Fe(DWORD,ScaleWindowExt,HDC,a,short,b,short,c,short,d,short,e)
Fe(HBITMAP,CreateBitmap,short,a,short,b,BYTE,c,BYTE,d,LPSTR,e)
Fe(HWND,CreateDialogIndirectParam,HANDLE,a,SEGPTR,b,HWND,c,WNDPROC,d,LPARAM,e)
Fe(HWND,CreateDialogParam,HANDLE,a,SEGPTR,b,HWND,c,WNDPROC,d,LPARAM,e)
Fe(LONG,CallWindowProc,WNDPROC,a,HWND,b,WORD,c,WORD,d,LONG,e)
Fe(LONG,DefFrameProc,HWND,a,HWND,b,WORD,c,WORD,d,LONG,e)
Fe(LONG,SendDlgItemMessage,HWND,a,WORD,b,WORD,c,WORD,d,LONG,e)
Fe(int,DialogBoxIndirectParam,HANDLE,a,HANDLE,b,HWND,c,WNDPROC,d,LONG,e)
Fe(int,DialogBoxParam,HANDLE,a,SEGPTR,b,HWND,c,WNDPROC,d,LONG,e)
Fe(int,DlgDirList,HWND,a,LPSTR,b,int,c,int,d,WORD,e)
Fe(int,DlgDirListComboBox,HWND,a,SEGPTR,b,int,c,int,d,WORD,e)
Fe(int,DrawText,HDC,a,LPSTR,str,int,c,LPRECT,d,WORD,flag)
Fe(int,Escape,HDC,a,int,b,int,c,LPSTR,d,LPSTR,e)
Fe(int,ExcludeClipRect,HDC,a,short,b,short,c,short,d,short,e)
Fe(int,ExcludeVisRect,HDC,a,short,b,short,c,short,d,short,e)
Fe(int,GetMenuString,HMENU,a,WORD,b,LPSTR,c,short,d,WORD,e)
Fe(int,GetProfileString,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d,int,e)
Fe(int,IntersectClipRect,HDC,a,short,b,short,c,short,d,short,e)
Fe(int,IntersectVisRect,HDC,a,short,b,short,c,short,d,short,e)
Fe(int,SetVoiceAccent,int,a,int,b,int,c,int,d,int,e)
Fe(int,ToAscii,WORD,wVirtKey,WORD,wScanCode,LPSTR,lpKeyState,LPVOID,lpChar,WORD,wFlags)
Fe(void,PaintRect,HWND,a,HWND,b,HDC,c,HBRUSH,d,LPRECT,e)
Fe(void,ScrollWindow,HWND,a,short,b,short,c,LPRECT,d,LPRECT,e)
Fe(void,SetRect,LPRECT,a,short,b,short,c,short,d,short,e)
Fe(void,SetRectRgn,HRGN,a,short,b,short,c,short,d,short,e)
Fe(void,SetScrollRange,HWND,a,int,b,int,c,int,d,BOOL,e)
Ff(BOOL,MoveWindow,HWND,a,short,b,short,c,short,d,short,e,BOOL,f)
Ff(BOOL,PatBlt,HDC,a,short,b,short,c,short,d,short,e,DWORD,f)
Ff(BOOL,ScaleViewportExtEx,HDC,a,short,b,short,c,short,d,short,e,LPSIZE,f)
Ff(BOOL,ScaleWindowExtEx,HDC,a,short,b,short,c,short,d,short,e,LPSIZE,f)
Ff(HBITMAP,CreateDIBitmap,HDC,a,LPBITMAPINFOHEADER,b,DWORD,c,LPSTR,d,LPBITMAPINFO,e,WORD,f)
Ff(HRGN,CreateRoundRectRgn,short,a,short,b,short,c,short,d,short,e,short,f)
Ff(short,GetPrivateProfileString,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d,short,e,LPSTR,f)
Ff(void,LineDDA,short,a,short,b,short,c,short,d,FARPROC,e,long,f)
Fg(BOOL,RoundRect,HDC,a,short,b,short,c,short,d,short,e,short,f,short,g)
Fg(BOOL,ScrollDC,HDC,a,short,b,short,c,LPRECT,d,LPRECT,e,HRGN,f,LPRECT,g)
Fg(BOOL,SetWindowPos,HWND,a,HWND,b,short,c,short,d,short,e,short,f,WORD,g)
Fg(BOOL,TrackPopupMenu,HMENU,a,WORD,b,short,c,short,d,short,e,HWND,f,LPRECT,g)
Fg(HCURSOR,CreateCursor,HANDLE,a,short,b,short,c,short,d,short,e,LPSTR,f,LPSTR,g)
Fg(HICON,CreateIcon,HANDLE,a,int,b,int,c,BYTE,d,BYTE,e,LPSTR,f,LPSTR,g)
Fg(int,GetDIBits,HDC,a,HANDLE,a2,WORD,b,WORD,c,LPSTR,d,LPBITMAPINFO,e,WORD,f)
Fg(int,SetDIBits,HDC,a,HANDLE,a2,WORD,b,WORD,c,LPSTR,d,LPBITMAPINFO,e,WORD,f)
Fh(BOOL,ExtTextOut,HDC,a,short,b,short,c,WORD,d,LPRECT,e,LPSTR,f,WORD,g,LPINT,h)
Fh(HDWP,DeferWindowPos,HDWP,hWinPosInfo,HWND,hWnd,HWND,hWndInsertAfter,INT,x,INT,y,INT,cx,INT,cy,WORD,wFlags)
Fh(LONG,TabbedTextOut,HDC,a,short,b,short,c,LPSTR,d,short,e,short,f,LPINT,g,short,h)
Fh(int,ScrollWindowEx,HWND,a,short,b,short,c,LPRECT,d,LPRECT,e,HRGN,f,LPRECT,g,WORD,h)
Fi(BOOL,Arc,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom,int,xStart,int,yStart,int,xEnd,int,yEnd)
Fi(BOOL,BitBlt,HDC,a,short,b,short,c,short,d,short,e,HDC,f,short,g,short,h,DWORD,i)
Fi(BOOL,Chord,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom,int,xStart,int,yStart,int,xEnd,int,yEnd)
Fi(BOOL,GrayString,HDC,a,HBRUSH,b,FARPROC,gsprc,LPARAM,lParam,INT,cch,INT,x,INT,y,INT,cx,INT,cy)
Fi(BOOL,Pie,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom,int,xStart,int,yStart,int,xEnd,int,yEnd)
Fk(BOOL,StretchBlt,HDC,a,short,b,short,c,short,d,short,e,HDC,f,short,g,short,h,short,i,short,j,DWORD,k)
Fk(HWND,CreateWindow,SEGPTR,a,SEGPTR,b,DWORD,c,short,d,short,e,short,f,short,g,HWND,h,HMENU,i,HANDLE,j,SEGPTR,k)
Fl(HWND,CreateWindowEx,DWORD,a,SEGPTR,b,SEGPTR,c,DWORD,d,short,e,short,f,short,g,short,h,HWND,i,HMENU,j,HANDLE,k,SEGPTR,l)
Fl(int,SetDIBitsToDevice,HDC,a,short,b,short,c,WORD,d,WORD,e,WORD,f,WORD,g,WORD,h,WORD,i,LPSTR,j,LPBITMAPINFO,k,WORD,l)
Fm(int,StretchDIBits,HDC,a,WORD,b,WORD,c,WORD,d,WORD,e,WORD,f,WORD,g,WORD,h,WORD,i,LPSTR,j,LPBITMAPINFO,k,WORD,l,DWORD,m)
Fn(HFONT,CreateFont,int,a,int,b,int,c,int,d,int,e,BYTE,f,BYTE,g,BYTE,h,BYTE,i,BYTE,j,BYTE,k,BYTE,l,BYTE,m,LPSTR,n)

#ifdef WINELIB
#define WINELIB_UNIMP(x) fprintf (stderr, "WineLib: Unimplemented %s\n", x)
#endif
#endif  /* WINDOWS_H */
