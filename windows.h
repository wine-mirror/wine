/* Initial draft attempt of windows.h, by Peter MacDonald, pmacdona@sanjuan.uvic.ca */

#ifndef _WINARGS

typedef unsigned short WORD;
typedef unsigned long DWORD;
#ifndef _WINMAIN
typedef unsigned short BOOL;
typedef unsigned char BYTE;
#endif
typedef long LONG;
typedef WORD HANDLE;
typedef HANDLE HWND;
typedef HANDLE HDC;
typedef HANDLE HCURSOR;
typedef HANDLE HFONT;
typedef HANDLE HPEN;
typedef HANDLE HRGN;
typedef HANDLE HPALETTE;
typedef HANDLE HICON;
typedef HANDLE HMENU;
typedef HANDLE HBITMAP;
typedef HANDLE HBRUSH;
typedef HANDLE LOCALHANDLE;
typedef char *LPSTR;
typedef char *NPSTR;
typedef char *LPMSG;
typedef int *LPINT;
typedef void *LPVOID;
typedef long (*FARPROC)();
typedef int CATCHBUF[9];
typedef int *LPCATCHBUF;

#define TRUE 1
#define FALSE 0
#define CW_USEDEFAULT ((int)0x8000)
#define FAR
#define NEAR
#define PASCAL
#ifndef NULL
#define NULL (void *)0
#endif

typedef struct { short left, top, right, bottom; } RECT;
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

typedef struct {
	WORD	style;
	LONG	(*lpfnWndProc)() __attribute__ ((packed));
	short	cbClsExtra, cbWndExtra;
	HANDLE	hInstance;
	HICON	hIcon;
	HCURSOR	hCursor;
	HBRUSH	hbrBackground;
	LPSTR	lpszMenuName, lpszClassName;
} WNDCLASS;

typedef  WNDCLASS * PWNDCLASS;
typedef  WNDCLASS * NPWNDCLASS;
typedef  WNDCLASS * LPWNDCLASS;

typedef struct { int x, y; } POINT;
typedef POINT *PPOINT;
typedef POINT *NPPOINT;
typedef POINT *LPPOINT;

typedef struct { 
	HWND	hwnd;
	WORD	message, wParam;
	long	lParam;
	DWORD	time;
	POINT	pt;
} MSG;
	
typedef WORD ATOM;

typedef struct tagBITMAP
{
    unsigned short bmType;
    unsigned short bmWidth;
    unsigned short bmHeight;
    unsigned short bmWidthBytes;
    unsigned char  bmPlanes;
    unsigned char  bmBitsPixel;
    unsigned long  bmBits __attribute__ ((packed));
} BITMAP;

typedef BITMAP *PBITMAP;
typedef BITMAP *NPBITMAP;
typedef BITMAP *LPBITMAP;

typedef struct { WORD lbStyle; DWORD lbColor; int lbHatch; } LOGBRUSH;
typedef LOGBRUSH *PLOGBRUSH;
typedef LOGBRUSH *NPLOGBRUSH;
typedef LOGBRUSH *LPLOGBRUSH;

#define LF_FACESIZE 32
typedef struct {
	int lfHeight, lfWidth, lfEscapement, lfOrientation, lfWeight;
	BYTE lfItalic, lfUnderline, lfStrikeOut, lfCharSet;
	BYTE lfOutPrecision, lfClipPrecision, lfQuality, lfPitchAndFamily;
	BYTE lfFaceName[LF_FACESIZE];
} LOGFONT;
typedef LOGFONT *PLOGFONT;
typedef LOGFONT *NPLOGFONT;
typedef LOGFONT *LPLOGFONT;

typedef struct PALETTEENTRY {
	BYTE peRed, peGreen, peBlue, peFlags;
} PALETTEENTRY;
typedef PALETTEENTRY *LPPALETTEENTRY;

typedef struct { 
	WORD palVersion, palNumEntries;
	PALETTEENTRY palPalEntries[1];
} LOGPALETTE;
typedef LOGPALETTE *PLOGPALETTE;
typedef LOGPALETTE *NPLOGPALETTE;
typedef LOGPALETTE *LPLOGPALETTE;

typedef struct { WORD lopnStyle; POINT lopnWidth; DWORD lopnColor; } LOGPEN;
typedef LOGPEN *PLOGPEN;
typedef LOGPEN *NPLOGPEN;
typedef LOGPEN *LPLOGPEN;

typedef struct {
	DWORD rdSize;
	WORD rdFunction, rdParam[1];
} METARECORD;
typedef METARECORD *LPMETARECORD;
typedef METARECORD *NPMETARECORD;
typedef METARECORD *PMETARECORD;

/* Unimplemented structs */
typedef struct { BYTE rgbBlue, rgbGreen, rgbRed, rgbReserved; } RGBQUAD;
typedef struct { BYTE rgbtBlue, rgbtGreen, rgbtRed; } RGBTRIPLE;

typedef struct tagBITMAPINFOHEADER
{
    unsigned long biSize;
    unsigned long biWidth;
    unsigned long biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned long biCompression;
    unsigned long biSizeImage;
    unsigned long biXPelsPerMeter;
    unsigned long biYPelsPerMeter;
    unsigned long biClrUsed;
    unsigned long biClrImportant;
} BITMAPINFOHEADER;

typedef BITMAPINFOHEADER * LPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER * NPBITMAPINFOHEADER;
typedef BITMAPINFOHEADER * PBITMAPINFOHEADER;

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
    unsigned short bcWidth;
    unsigned short bcHeight;
    unsigned short bcPlanes;
    unsigned short bcBitCount;
} BITMAPCOREHEADER;

typedef struct {
	BYTE Id;  /* much more .... */
} DCB;

typedef struct {
	BYTE i;  /* much more .... */
} COMSTAT;

typedef struct {
	BYTE i;  /* much more .... */
} TEXTMETRIC;
typedef TEXTMETRIC *PTEXTMETRIC;
typedef TEXTMETRIC *LPTEXTMETRIC;
typedef TEXTMETRIC *NPTEXTMETRIC;

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

typedef struct 
{
	HANDLE objectHandle[1];
}HANDLETABLE;
typedef HANDLETABLE *LPHANDLETABLE;

#define CS_VREDRAW 0x0001
#define CS_HREDRAW 0x0002
#define CS_KEYCVTWINDOW 0x0004
#define CS_KBLCLKS 0x0008

#define MAKEINTRESOURCE(i) (LPSTR)((DWORD)((WORD)(i)))

#define IDI_APPLICATION MAKEINTRESOURCE(32512)
#define IDI_HAND MAKEINTRESOURCE(32513)
#define IDI_QUESTION MAKEINTRESOURCE(32514)
#define IDI_EXCLAMATION MAKEINTRESOURCE(32515)
#define IDI_ASTERISK MAKEINTRESOURCE(32516)

#define IDC_ARROW MAKEINTRESOURCE(32512)
#define IDC_IBEAM MAKEINTRESOURCE(32513)
#define IDC_WAIT MAKEINTRESOURCE(32514)
#define IDC_CROSS MAKEINTRESOURCE(32515)
#define IDC_UPARROW MAKEINTRESOURCE(32516)
#define IDC_SIZE MAKEINTRESOURCE(32540)
#define IDC_ICON MAKEINTRESOURCE(32541)
#define IDC_SIZENWSE MAKEINTRESOURCE(32542)
#define IDC_SIZENESW MAKEINTRESOURCE(32543)
#define IDC_SIZEWE MAKEINTRESOURCE(32544)
#define IDC_SIZENS MAKEINTRESOURCE(32545)

enum { WHITE_BRUSH, LTGRAY_BRUSH, GRAY_BRUSH, DKGRAY_BRUSH, 
	BLACK_BRUSH, NULL_BRUSH, HOLLOW_BRUSH, WHITE_PEN,
	BLACK_PEN, NULL_PEN, OEM_FIXED_FONT, ANSI_FIXED_FONT, 
	ANSI_VAR_FONT, SYSTEM_FONT_FONT, DEVICE_DEFAULT_FONT,
	DEFAULT_PALETTE, SYSTEM_FIXED_FONT };

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

#define WM_COMMAND 0x0111

enum { SW_HIDE, SW_SHOWNORMAL, SW_NORMAL, SW_SHOWMINIMIZED, SW_SHOWMAXIMIZED,
	SW_MAXIMIZE, SW_SHOWNOACTIVATE, SW_SHOW, SW_MINIMIZE,
	SW_SHOWMINNOACTIVE, SW_SHOWNA, SW_RESTORE };

#define MF_INSERT 0
#define MF_CHANGE 0x0080
#define MF_APPEND 0x0100
#define MF_DELETE 0x0200
#define MF_REMOVE 0x1000
#define MF_BYCOMMAND 0
#define MF_BYPOSITION 0x0400
#define MF_SEPARATOR 0x080
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

#ifndef NOWINOFFSETS
#define GCW_HBRBACKGROUND (-10)
#endif

#define MB_OK 0
#define MB_ICONINFORMATION 0x0040

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




#define WS_OVERLAPPED 0x00000000L
#define WS_POPUP 0x80000000L
#define WS_CHILD 0x40000000L
#define WS_MINIMIZE 0x20000000L
#define WS_VISIBLE 0x10000000L
#define WS_DISABLED 0x08000000L
#define WS_CLIPSIBLINGS 0x04000000L
#define WS_CLIPCHILDREN 0x02000000L
#define WS_MAXIMIZE 0x01000000L
#define WS_CAPTION 0x00C00000L
#define WS_BORDER 0x00800000L
#define WS_DLGFRAME 0x00400000L
#define WS_VSCROLL 0x00200000L
#define WS_HSCROLL 0x00100000L
#define WS_SYSMENU 0x00080000L
#define WS_THICKFRAME 0x00040000L
#define WS_GROUP 0x00020000L
#define WS_TABSTOP 0x00010000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_MINIMIZEBOX 0x00020000L
#define WS_MAXIMIZEBOX 0x00010000L
#define WS_TILED WS_OVERLAPPED
#define WS_ICONIC WS_MINIMIZE
#define WS_SIZEBOX WS_THICKFRAME
#define WS_OVERLAPPEDWINDOW (WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME| WS_MINIMIZEBOX | WS_MAXIMIZEBOX)
#define WS_POPUPWINDOW (WS_POPUP | WS_BORDER | WS_SYSMENU)
#define WS_CHILDWINDOW (WS_CHILD)
#define WS_TILEDWINDOW (WS_OVERLAPPEDWINDOW)

#define GMEM_MOVEABLE	0x0002

#ifdef _WIN_CODE_SECTION
#define F(ret,name) ret name(void) {dte(#name, 0);}
#define Fa(ret,name,t1,a1) ret name(t1 a1) {dte(#name, 1, #t1,a1); }
#define Fb(ret,name,t1,a1,t2,a2) ret name(t1 a1,t2 a2) { dte(#name, 2, #t1,a1,#t2,a2); }
#define Fc(ret,name,t1,a1,t2,a2,t3,a3) ret name(t1 a1,t2 a2,t3 a3){dte(#name,3,#t1,a1,#t2,a2,#t3,a3); }
#define Fd(ret,name,t1,a1,t2,a2,t3,a3,t4,a4) ret name(t1 a1,t2 a2,t3 a3,t4 a4){dte(#name,4,#t1,a1,#t2,a2,#t3,a3,#t4,a4);}
#define Fe(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) ret name( t1 a1,t2 a2,t3 a3,t4 a4,t5 a5) {dte(#name,5,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5);}
#define Ff(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6) {dte(#name,6,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6);}
#define Fg(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) ret name( t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7) {dte(#name,7,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7);}
#define Fh(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8){dte(#name,8,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8);}
#define Fi(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9) {dte(#name,9,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8,#t9,a9);}
#define Fj(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10){dte(#name,10,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8,#t9,a9,#t10,a10);}
#define Fk(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11) ret name (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11){dte(#name,11, #t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8,#t9,a9,#t10,a10,#t11,a11);}
#define Fl(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11,t12 a12){dte(#name,12,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8,#t9,a9,#t10,a10,#t11,a11,#t12,a12);}
#define Fm(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12,t13,a13) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11,t12 a12,t13 a13){dte(#name,13,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8,#t9,a9,#t10,a10,#t11,a11,#t12,a12,#t13,a13);}
#define Fn(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12,t13,a13,t14,a14) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11,t12 a12,t13 a13,t14 a14){dte(#name,14,#t1,a1,#t2,a2,#t3,a3,#t4,a4,#t5,a5,#t6,a6,#t7,a7,#t8,a8,#t9,a9,#t10,a10,#t11,a11,#t12,a12,#t13,a13,#t14,a14);}
#else
#define F(ret,name) ret name(void);
#define Fa(ret,name,t1,a1) ret name(t1 a1);
#define Fb(ret,name,t1,a1,t2,a2) ret name(t1 a1,t2 a2);
#define Fc(ret,name,t1,a1,t2,a2,t3,a3) ret name(t1 a1,t2 a2,t3 a3);
#define Fd(ret,name,t1,a1,t2,a2,t3,a3,t4,a4) ret name(t1 a1,t2 a2,t3 a3,t4 a4);
#define Fe(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5) ret name( t1 a1,t2 a2,t3 a3,t4 a4,t5 a5);
#define Ff(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6);
#define Fg(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7) ret name( t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7);
#define Fh(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8);
#define Fi(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9);
#define Fj(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10);
#define Fk(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11) ret name (t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11);
#define Fl(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11,t12 a12);
#define Fm(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12,t13,a13) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11,t12 a12,t13 a13);
#define Fn(ret,name,t1,a1,t2,a2,t3,a3,t4,a4,t5,a5,t6,a6,t7,a7,t8,a8,t9,a9,t10,a10,t11,a11,t12,a12,t13,a13,t14,a14) ret name(t1 a1,t2 a2,t3 a3,t4 a4,t5 a5,t6 a6,t7 a7,t8 a8,t9 a9,t10 a10,t11 a11,t12 a12,t13 a13,t14 a14);
#endif

int wsprintf(LPSTR a,LPSTR b,...);
#endif

#if !defined(_WIN_CODE_SECTION) || defined(_WINARGS)

/* Implemented functions */
F(HMENU,CreateMenu)
Fa(BOOL,IsCharAlpha,char,ch)
Fa(BOOL,IsCharAlphaNumeric,char,ch)
Fa(BOOL,IsCharLower,char,ch)
Fa(BOOL,IsCharUpper,char,ch)
Fa(BOOL,RegisterClass,LPWNDCLASS,a) 
Fa(BOOL,TranslateMessage,LPMSG,a)
Fa(int,_lclose,int,a)
Fb(int,_lopen,LPSTR,a,int,b)
Fa(int,lstrlen,LPSTR,a)
Fa(long,DispatchMessage,MSG *,msg)
Fa(void,UpdateWindow,HWND,a)
Fb(BOOL,ExitWindows,DWORD,dwReserved,WORD,wReturnCode)
Fb(BOOL,ShowWindow,HWND,a,int,b) 
Fb(HDC,BeginPaint,HWND,a,LPPAINTSTRUCT,b) 
Fb(LPSTR,lstrcat,LPSTR,a,LPSTR,b )
Fb(LPSTR,lstrcpy,LPSTR,a,LPSTR,b )
Fb(int,_lcreat,LPSTR,a,int,b)
Fb(int,lstrcmp,LPSTR,a,LPSTR,b )
Fb(int,lstrcmpi,LPSTR,a,LPSTR,b )
Fb(void,EndPaint,HWND,a,LPPAINTSTRUCT,b)
Fb(void,GetClientRect,HWND,a,LPRECT,b)
Fc(BOOL,LineTo,HDC,a,int,b,int,c)
Fc(LONG,_llseek,int,a,long,b,int,c)
Fc(WORD,_lread,int,a,LPSTR,b,int,c)
Fc(WORD,_lwrite,int,a,LPSTR,b,int,c)
Fc(int,FillRect,HDC,a,LPRECT,b,HBRUSH,c)
Fc(DWORD,MoveTo,HDC,a,int,b,int,c)
Fd(BOOL,AppendMenu,HMENU,a,WORD,b,WORD,c,LPSTR,d)
Fd(BOOL,GetMessage,LPMSG,msg,HWND,b,WORD,c,WORD,d)
Fe(BOOL,Rectangle,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom)
Fe(int,DrawText,HDC,a,LPSTR,str,int,c,LPRECT,d,WORD,flag)
Fi(BOOL,Arc,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom,int,xStart,int,yStart,int,xEnd,int,yEnd)
Fi(BOOL,Chord,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom,int,xStart,int,yStart,int,xEnd,int,yEnd)
Fi(BOOL,Pie,HDC,a,int,xLeft,int,yTop,int,xRight,int,yBottom,int,xStart,int,yStart,int,xEnd,int,yEnd)
Fk(HWND,CreateWindow,LPSTR,szAppName,LPSTR,Label,DWORD,ol,int,x,int,y,int,w,int,h,HWND,d,HMENU,e,,HANDLE i,LPSTR,g)
#endif

F(BOOL,AnyPopup)
F(BOOL,CloseClipboard)
F(BOOL,EmptyClipboard)
F(BOOL,GetInputState)
F(BOOL,InSendMessage)
F(DWORD,GetCurrentTime)
F(DWORD,GetMessagePos)
F(DWORD,GetTickCount)
F(HANDLE,GetCurrentTask)
F(HMENU,CreatePopupMenu)
F(HWND,GetActiveWindow)
F(HWND,GetCapture)
F(HWND,GetClipboardOwner)
F(HWND,GetClipboardViewer)
F(HWND,GetDesktopHwnd)
F(HWND,GetDesktopWindow)
F(HWND,GetFocus)
F(HWND,GetSysModalWindow)
F(LONG,GetMenuCheckMarkDimensions)
F(LONG,GetMessageTime)
F(LONG,GetWinFlags)
F(LPINT,GetThresholdEvent)
/*F(LPSTR,GetDOSEnvironment)*/
F(LPSTR,ValidateFreeSpaces)
F(void,ValidateCodeSegments)
F(WORD,GetCaretBlinkTime)
F(WORD,GetCurrentPDB)
F(WORD,GetDoubleClickTime)
F(WORD,GetNumTasks)
F(WORD,GetVersion)
F(int,CountClipboardFormats)
F(int,GetKBCodePage)
F(int,GetThresholdStatus)
F(int,OpenSound)
F(int,ProfInsChk)
F(int,StartSound)
F(int,StopSound)
F(int,SyncAllVoices)
F(long,GetDialogBaseUnits)
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
F(void,WaitMessage)
F(void,Yield)
Fa(ATOM,AddAtom,LPSTR,a)
Fa(ATOM,DeleteAtom,ATOM,a)
Fa(ATOM,FindAtom,LPSTR,a)
Fa(ATOM,GlobalAddAtom,LPSTR,a)
Fa(ATOM,GlobalDeleteAtom,ATOM,a)
Fa(ATOM,GlobalFindAtom,LPSTR,a)
Fa(BOOL,DeleteDC,HDC,a)
Fa(BOOL,DeleteMetaFile,HANDLE,a)
Fa(BOOL,DeleteObject,HANDLE,a)
Fa(BOOL,DestroyCursor,HCURSOR,a)
Fa(BOOL,DestroyIcon,HICON,a)
Fa(BOOL,DestroyMenu,HMENU,a)
Fa(BOOL,DestroyWindow,HWND,a)
Fa(BOOL,EnableHardwareInput,BOOL,a)
Fa(BOOL,FreeModule,HANDLE,a)
Fa(BOOL,FreeResource,HANDLE,a)
Fa(BOOL,GlobalUnWire,HANDLE,a)
Fa(BOOL,GlobalUnfix,HANDLE,a)
Fa(BOOL,GlobalUnlock,HANDLE,a)
Fa(BOOL,InitAtomTable,int,a)
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
Fa(BOOL,SetErrorMode,WORD,a)
Fa(BOOL,SetMessageQueue,int,a)
Fa(BOOL,SwapMouseButton,BOOL,a)
Fa(BOOL,UnrealizeObject,HBRUSH,a)
Fa(BYTE,GetTempDrive,BYTE,a)
Fa(DWORD,GetAspectRatioFilter,HDC,a)
Fa(DWORD,GetBitmapDimension,HBITMAP,a)
Fa(DWORD,GetBkColor,HDC,a)
Fa(DWORD,GetBrushOrg,HDC,a)
Fa(DWORD,GetCurrentPosition,HDC,a)
Fa(DWORD,GetDCOrg,HDC,a)
Fa(DWORD,GetFreeSpace,WORD,a)
Fa(DWORD,GetSysColor,int,a)
Fa(DWORD,GetTextColor,HDC,a)
Fa(DWORD,GetViewportExt,HDC,a)
Fa(DWORD,GetViewportOrg,HDC,a)
Fa(DWORD,GetWindowExt,HDC,a)
Fa(DWORD,GetWindowOrg,HDC,a)
Fa(DWORD,GlobalCompact,DWORD,a)
Fa(DWORD,GlobalHandle,WORD,a)
Fa(DWORD,GlobalSize,HANDLE,a)
Fa(DWORD,OemKeyScan,WORD,a)
Fa(FARPROC,LocalNotify,FARPROC,a)
Fa(HANDLE,BeginDeferWindowPos,int,nNumWindows)
Fa(HANDLE,CloseMetaFile,HANDLE,a)
Fa(HANDLE,CreateMetaFile,LPSTR,a)
Fa(HANDLE,GetAtomHandle,ATOM,a)
Fa(HANDLE,GetClipboardData,WORD,a)
Fa(HANDLE,GetCodeHandle,FARPROC,a)
Fa(HANDLE,GetMetaFile,LPSTR,a)
Fa(HANDLE,GetMetaFileBits,HANDLE,a)
Fa(HANDLE,GetModuleHandle,LPSTR,a)
Fa(HANDLE,GetStockObject,int,a)
Fa(HANDLE,GetWindowTask,HWND,a)
Fa(HANDLE,GlobalFree,HANDLE,a)
Fa(HANDLE,GlobalLRUNewest,HANDLE,a)
Fa(HANDLE,GlobalLRUOldest,HANDLE,a)
Fa(HANDLE,LoadLibrary,LPSTR,a)
Fa(HANDLE,LocalFree,HANDLE,a)
Fa(HANDLE,LocalHandle,WORD,a)
Fa(HANDLE,LockSegment,WORD,a)
Fa(HANDLE,SetMetaFileBits,HANDLE,a)
Fa(HANDLE,UnlockSegment,WORD,a)
Fa(HBITMAP,CreateBitmapIndirect,BITMAP FAR*,a)
Fa(HBRUSH,CreateBrushIndirect,LOGBRUSH FAR*,a)
Fa(HBRUSH,CreatePatternBrush,HBITMAP,a)
Fa(HBRUSH,CreateSolidBrush,DWORD,a)
Fa(HCURSOR,SetCursor,HCURSOR,a)
Fa(HDC,CreateCompatibleDC,HDC,a)
Fa(HDC,GetDC,HWND,a)
Fa(HDC,GetWindowDC,HWND,a)
Fa(HFONT,CreateFontIndirect,LOGFONT FAR*,a)
Fa(HMENU,GetMenu,HWND,a)
Fa(HMENU,LoadMenuIndirect,LPSTR,a)
Fa(HPALETTE,CreatePalette,LPLOGPALETTE,a)
Fa(HPEN,CreatePenIndirect,LOGPEN FAR*,a)
Fa(HRGN,CreateEllipticRgnIndirect,LPRECT,a)
Fa(HRGN,CreateRectRgnIndirect,LPRECT,a)
Fa(HWND,GetLastActivePopup,HWND,a)
Fa(HWND,GetParent,HWND,a)
Fa(HWND,GetTopWindow,HWND,a)
Fa(HWND,SetActiveWindow,HWND,a)
Fa(HWND,SetCapture,HWND,a)
Fa(HWND,SetClipboardViewer,HWND,a)
Fa(HWND,SetFocus,HWND,a)
Fa(HWND,SetSysModalWindow,HWND,a)
Fa(HWND,WindowFromPoint,POINT,a)
Fa(LONG,SetSwapAreaSize,WORD,a)
Fa(LPSTR,AnsiLower,LPSTR,a)
Fa(LPSTR,AnsiNext,LPSTR,a)
Fa(LPSTR,AnsiUpper,LPSTR,a)
Fa(LPSTR,GlobalLock,HANDLE,a)
Fa(LPSTR,GlobalWire,HANDLE,a)
Fa(LPSTR,LockResource,HANDLE,a)
Fa(void,GlobalFix,HANDLE,a)
Fa(void,GlobalNotify,FARPROC,a)
Fa(void,LimitEmsPages,DWORD,a)
Fa(void,SetConvertHook,BOOL,a)
Fa(WORD,AllocDStoCSAlias,WORD,a)
Fa(WORD,AllocSelector,WORD,a)
Fa(WORD,ArrangeIconicWindows,HWND,a)
Fa(WORD,EnumClipboardFormats,WORD,a)
Fa(WORD,FreeSelector,WORD,a)
Fa(WORD,GetDriveType,int,a)
Fa(WORD,GetMenuItemCount,HMENU,a)
Fa(WORD,GetTextAlign,HDC,a)
Fa(WORD,GlobalFlags,HANDLE,a)
Fa(WORD,GlobalPageLock,HANDLE,a)
Fa(WORD,GlobalPageUnlock,HANDLE,a)
Fa(WORD,LocalCompact,WORD,a)
Fa(WORD,LocalFlags,HANDLE,a)
Fa(WORD,LocalSize,HANDLE,a)
Fa(WORD,RealizePalette,HDC,a)
Fa(WORD,RegisterClipboardFormat,LPSTR,a)
Fa(WORD,RegisterWindowMessage,LPSTR,a)
Fa(WORD,SetHandleCount,WORD,a)
Fa(WORD,VkKeyScan,WORD,a)
Fa(char NEAR*,LocalLock,HANDLE,a)
Fa(int,AddFontResource,LPSTR,a)
Fa(int,Catch,LPCATCHBUF,a)
Fa(int,ClearCommBreak,int,a)
Fa(int,CloseComm,int,a)
Fa(int,CountVoiceNotes,int,a)
Fa(int,GetAsyncKeyState,int,a)
Fa(int,GetBkMode,HDC,a)
Fa(int,GetDlgCtrlID,HWND,a)
Fa(int,GetKeyState,int,a)
Fa(int,GetKeyboardType,int,a)
Fa(int,GetMapMode,HDC,a)
Fa(int,GetModuleUsage,HANDLE,a)
Fa(int,GetPolyFillMode,HDC,a)
Fa(int,GetROP2,HDC,a)
Fa(int,GetStretchBltMode,HDC,a)
Fa(int,GetSystemMetrics,int,a)
Fa(int,GetTextCharacterExtra,HDC,a)
Fa(int,GetWindowTextLength,HWND,a)
Fa(int,SaveDC,HDC,a)
Fa(int,SetCommBreak,int,a)
Fa(int,SetCommState,DCB*,a)
Fa(int,ShowCursor,BOOL,a)
Fa(int,UpdateColors,HDC,a)
Fa(int,WaitSoundState,int,a)
Fa(void,BringWindowToTop,HWND,a)
Fa(void,ClipCursor,LPRECT,a)
Fa(void,CloseWindow,HWND,a)
Fa(void,DrawMenuBar,HWND,a)
Fa(void,EndDeferWindowPos,HANDLE,hWinPosInfo)
Fa(void,FatalExit,int,a)
Fa(void,FreeLibrary,HANDLE,a)
Fa(void,FreeProcInstance,FARPROC,a)
Fa(void,GetCaretPos,LPPOINT,a)
Fa(void,GetCursorPos,LPPOINT,a)
Fa(void,GetKeyboardState,BYTE FAR*,a)
Fa(void,HideCaret,HWND,a)
Fa(void,MessageBeep,WORD,a)
Fa(void,OutputDebugString,LPSTR,a)
Fa(void,PostQuitMessage,int,a)
Fa(void,ReplyMessage,LONG,a)
Fa(void,SetCaretBlinkTime,WORD,a)
Fa(void,SetDoubleClickTime,WORD,a)
Fa(void,SetKeyboardState,BYTE FAR*,a)
Fa(void,SetRectEmpty,LPRECT,a)
Fa(void,ShowCaret,HWND,a)
Fa(void,SwapRecording,WORD,a)
Fb(BOOL,CallMsgFilter,LPMSG,a,int,b)
Fb(BOOL,ChangeClipboardChain,HWND,a,HWND,b)
Fb(BOOL,EnableWindow,HWND,a,BOOL,b)
Fb(BOOL,EnumWindows,FARPROC,a,LONG,b)
Fb(BOOL,EqualRect,LPRECT,a,LPRECT,b)
Fb(BOOL,EqualRgn,HRGN,a,HRGN,b)
Fb(BOOL,FlashWindow,HWND,a,BOOL,b)
Fb(BOOL,GetTextMetrics,HDC,a,LPTEXTMETRIC,b)
Fb(BOOL,InvertRgn,HDC,a,HRGN,b)
Fb(BOOL,IsChild,HWND,a,HWND,b)
Fb(BOOL,IsDialogMessage,HWND,a,LPMSG,b)
Fb(BOOL,KillTimer,HWND,a,int,b)
Fb(BOOL,OemToAnsi,LPSTR,a,LPSTR,b)
Fb(BOOL,PaintRgn,HDC,a,HRGN,b)
Fb(BOOL,PlayMetaFile,HDC,a,HANDLE,b)
Fb(BOOL,PtInRect,LPRECT,a,POINT,b)
Fb(BOOL,RectInRegion,HRGN,a,LPRECT,b)
Fb(BOOL,RectVisible,HDC,a,LPRECT,b)
Fb(BOOL,ResizePalette,HPALETTE,a,WORD,b)
Fb(BOOL,RestoreDC,HDC,a,int,b)
Fb(BOOL,SetConvertParams,int,a,int,b)
Fb(BOOL,SetMenu,HWND,a,HMENU,b)
Fb(BOOL,TranslateMDISysAccel,HWND,a,LPMSG,b)
Fb(BOOL,UnhookWindowsHook,int,a,FARPROC,b)
Fb(BOOL,UnregisterClass,LPSTR,a,HANDLE,b)
Fb(DWORD,GetNearestColor,HDC,a,DWORD,b)
Fb(DWORD,SetBkColor,HDC,a,DWORD,b)
Fb(DWORD,SetMapperFlags,HDC,a,DWORD,b)
Fb(DWORD,SetTextColor,HDC,a,DWORD,b)
Fb(FARPROC,GetProcAddress,HANDLE,a,LPSTR,b)
Fb(FARPROC,MakeProcInstance,FARPROC,a,HANDLE,b)
Fb(FARPROC,SetWindowsHook,int,a,FARPROC,b)
Fb(HANDLE,CopyMetaFile,HANDLE,a,LPSTR,b)
Fb(HANDLE,GetProp,HWND,a,LPSTR,b)
Fb(HANDLE,GlobalAlloc,WORD,a,DWORD,b)
Fb(HANDLE,LoadAccelerators,HANDLE,a,LPSTR,b)
Fb(HANDLE,LoadModule,LPSTR,a,LPVOID,b)
Fb(HANDLE,LoadResource,HANDLE,a,HANDLE,b)
Fb(HANDLE,LocalAlloc,WORD,a,WORD,b)
Fb(HANDLE,RemoveProp,HWND,a,LPSTR,b)
Fb(HANDLE,SelectObject,HDC,a,HANDLE,b)
Fb(HANDLE,SetClipboardData,WORD,a,HANDLE,b)
Fb(HBITMAP,LoadBitmap,HANDLE,a,LPSTR,b)
Fb(HBRUSH,CreateDIBPatternBrush,HANDLE,a,WORD,b)
Fb(HBRUSH,CreateHatchBrush,int,a,DWORD,b)
Fb(HCURSOR,LoadCursor,HANDLE,a,LPSTR,b)
Fb(HICON,LoadIcon,HANDLE,a,LPSTR,b)
Fb(HMENU,GetSubMenu,HMENU,a,int,b)
Fb(HMENU,GetSystemMenu,HWND,a,BOOL,b)
Fb(HMENU,LoadMenu,HANDLE,a,LPSTR,b)
Fb(HWND,ChildWindowFromPoint,HWND,a,POINT,b)
Fb(HWND,FindWindow,LPSTR,a,LPSTR,b)
Fb(HWND,GetDlgItem,HWND,a,int,b)
Fb(HWND,GetNextWindow,HWND,a,WORD,b)
Fb(HWND,GetWindow,HWND,a,WORD,b)
Fb(HWND,SetParent,HWND,a,HWND,b)
Fb(LONG,GetClassLong,HWND,a,int,b)
Fb(LONG,GetWindowLong,HWND,a,int,b)
Fb(LPSTR,AnsiPrev,LPSTR,a,LPSTR,b)
Fb(WORD FAR*,SetCommEventMask,int,a,WORD,b)
Fb(WORD,AnsiLowerBuff,LPSTR,a,WORD,b)
Fb(WORD,AnsiUpperBuff,LPSTR,a,WORD,b)
Fb(WORD,ChangeSelector,WORD,a,WORD,b)
Fb(WORD,GetClassWord,HWND,a,int,b)
Fb(WORD,GetCommEventMask,int,a,int,b)
Fb(WORD,GetMenuItemID,HMENU,a,int,b)
Fb(WORD,GetNearestPaletteIndex,HPALETTE,a,DWORD,b)
Fb(WORD,GetSystemDirectory,LPSTR,a,WORD,b)
Fb(WORD,GetSystemPaletteUse,HDC,a,WORD,b)
Fb(WORD,GetWindowWord,HWND,a,int,b)
Fb(WORD,GetWindowsDirectory,LPSTR,a,WORD,b)
Fb(WORD,IsDlgButtonChecked,HWND,a,int,b)
Fb(WORD,LocalShrink,HANDLE,a,WORD,b)
Fb(WORD,MapVirtualKey,WORD,a,WORD,b)
Fb(WORD,SetSystemPaletteUse,HDC,a,WORD,b)
Fb(WORD,SetTextAlign,HDC,a,WORD,b)
Fb(WORD,SizeofResource,HANDLE,a,HANDLE,b)
Fb(WORD,WinExec,LPSTR,a,WORD,b)
Fb(int,AccessResource,HANDLE,a,HANDLE,b)
Fb(int,AnsiToOem,LPSTR,a,LPSTR,b)
Fb(int,BuildCommDCB,LPSTR,a,DCB*,b)
Fb(int,ConvertRequest,HWND,a,LPKANJISTRUCT,b)
Fb(int,CopyRect,LPRECT,a,LPRECT,b)
Fb(int,EnumProps,HWND,a,FARPROC,b)
Fb(int,EscapeCommFunction,int,a,int,b)
Fb(int,ExcludeUpdateRgn,HDC,a,HWND,b)
Fb(int,FlushComm,int,a,int,b)
Fb(int,GetClipBox,HDC,a,LPRECT,b)
Fb(int,GetCommError,int,a,COMSTAT*,b)
Fb(int,GetCommState,int,a,DCB*,b)
Fb(int,GetDeviceCaps,HDC,a,int,b)
Fb(int,GetPriorityClipboardFormat,WORD FAR*,a,int,b)
Fb(int,GetRgnBox,HRGN,a,LPRECT,b)
Fb(int,GetScrollPos,HWND,a,int,b)
Fb(int,ReleaseDC,HWND,a,HDC,b)
Fb(int,SelectClipRgn,HDC,a,HRGN,b)
Fb(int,SetBkMode,HDC,a,int,b)
Fb(int,SetMapMode,HDC,a,int,b)
Fb(int,SetPolyFillMode,HDC,a,int,b)
Fb(int,SetROP2,HDC,a,int,b)
Fb(int,SetSoundNoise,int,a,int,b)
Fb(int,SetStretchBltMode,HDC,a,int,b)
Fb(int,SetTextCharacterExtra,HDC,a,int,b)
Fb(int,SetVoiceQueueSize,int,a,int,b)
Fb(int,SetVoiceThreshold,int,a,int,b)
Fb(int,TransmitCommChar,int,a,char,b)
Fb(int,UngetCommChar,int,a,char,b)
Fb(void,ClientToScreen,HWND,a,LPPOINT,b)
Fb(void,DrawFocusRect,HDC,a,LPRECT,b)
Fb(void,EndDialog,HWND,a,int,b)
Fb(void,GetCodeInfo,FARPROC,lpProc,LPVOID,lpSegInfo)
Fb(void,GetWindowRect,HWND,a,LPRECT,b)
Fb(void,InvertRect,HDC,a,LPRECT,b)
Fb(void,MapDialogRect,HWND,a,LPRECT,b)
Fb(void,ProfSampRate,int,a,int,b)
Fb(void,ProfSetup,int,a,int,b)
Fb(void,ScreenToClient,HWND,a,LPPOINT,b)
Fb(void,SetCaretPos,int,a,int,b)
Fb(void,SetCursorPos,int,a,int,b)
Fb(void,SetWindowText,HWND,a,LPSTR,b)
Fb(void,ShowOwnedPopups,HWND,a,BOOL,b)
Fb(void,Throw,LPCATCHBUF,a,int,b)
Fb(void,ValidateRect,HWND,a,LPRECT,b)
Fb(void,ValidateRgn,HWND,a,HRGN,b)
Fc(BOOL,CheckMenuItem,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,DPtoLP,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,DeleteMenu,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,DlgDirSelect,HWND,a,LPSTR,b,int,c)
Fc(BOOL,DlgDirSelectComboBox,HWND,a,LPSTR,b,int,c)
Fc(BOOL,EnableMenuItem,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,EnumChildWindows,HWND,a,FARPROC,b,LONG,c)
Fc(BOOL,EnumTaskWindows,HANDLE,a,FARPROC,b,LONG,c)
Fc(BOOL,FillRgn,HDC,a,HRGN,b,HBRUSH,c)
Fc(BOOL,GetClassInfo,HANDLE,a,LPSTR,b,LPWNDCLASS,c)
Fc(BOOL,GetUpdateRect,HWND,a,LPRECT,b,BOOL,c)
Fc(BOOL,LPtoDP,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,LocalInit,WORD,a,WORD,b,WORD,c)
Fc(BOOL,Polygon,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,Polyline,HDC,a,LPPOINT,b,int,c)
Fc(BOOL,PtInRegion,HRGN,a,int,b,int,c)
Fc(BOOL,PtVisible,HDC,a,int,b,int,c)
Fc(BOOL,RemoveMenu,HMENU,a,WORD,b,WORD,c)
Fc(BOOL,SetProp,HWND,a,LPSTR,b,HANDLE,c)
Fc(BOOL,WriteProfileString,LPSTR,a,LPSTR,b,LPSTR,c)
Fc(DWORD,GetPixel,HDC,a,int,b,int,c)
Fc(DWORD,GetTextExtent,HDC,a,LPSTR,b,int,c)
Fc(DWORD,OffsetViewportOrg,HDC,a,int,b,int,c)
Fc(DWORD,OffsetWindowOrg,HDC,a,int,b,int,c)
Fc(DWORD,SetBitmapDimension,HBITMAP,a,int,b,int,c)
Fc(DWORD,SetBrushOrg,HDC,a,int,b,int,c)
Fc(DWORD,SetViewportExt,HDC,a,int,b,int,c)
Fc(DWORD,SetViewportOrg,HDC,a,int,b,int,c)
Fc(DWORD,SetWindowExt,HDC,a,int,b,int,c)
Fc(DWORD,SetWindowOrg,HDC,a,int,b,int,c)
Fc(FARPROC,SetResourceHandler,HANDLE,a,LPSTR,b,FARPROC,c)
Fc(HANDLE,AllocResource,HANDLE,a,HANDLE,b,DWORD,c)
Fc(HANDLE,FindResource,HANDLE,a,LPSTR,b,LPSTR,c)
Fc(HANDLE,GlobalReAlloc,HANDLE,a,DWORD,b,WORD,c)
Fc(HANDLE,LocalReAlloc,HANDLE,a,WORD,b,WORD,c)
Fc(HBITMAP,CreateCompatibleBitmap,HDC,a,int,b,int,c)
Fc(HBITMAP,CreateDiscardableBitmap,HDC,a,int,b,int,c)
Fc(HPALETTE,SelectPalette,HDC,a,HPALETTE,b,BOOL,c)
Fc(HPEN,CreatePen,int,a,int,b,DWORD,c)
Fc(HRGN,CreatePolygonRgn,LPPOINT,a,int,b,int,c)
Fc(HWND,GetNextDlgGroupItem,HWND,a,HWND,b,BOOL,c)
Fc(HWND,GetNextDlgTabItem,HWND,a,HWND,b,BOOL,c)
Fc(LONG,GetBitmapBits,HBITMAP,a,LONG,b,LPSTR,c)
Fc(LONG,SetBitmapBits,HBITMAP,a,DWORD,b,LPSTR,c)
Fc(LONG,SetClassLong,HWND,a,int,b,LONG,c)
Fc(LONG,SetWindowLong,HWND,a,int,b,LONG,c)
Fc(WORD,GetAtomName,ATOM,a,LPSTR,b,int,c)
Fc(WORD,GetMenuState,HMENU,a,WORD,b,WORD,c)
Fc(WORD,GetProfileInt,LPSTR,a,LPSTR,b,int,c)
Fc(WORD,GlobalGetAtomName,ATOM,a,LPSTR,b,int,c)
Fc(WORD,SetClassWord,HWND,a,int,b,WORD,c)
Fc(WORD,SetWindowWord,HWND,a,int,b,WORD,c)
Fc(int,FrameRect,HDC,a,LPRECT,b,HBRUSH,c)
Fc(int,GetClassName,HWND,a,LPSTR,b,int,c)
Fc(int,GetClipboardFormatName,WORD,a,LPSTR,b,int,c)
Fc(int,GetEnvironment,LPSTR,a,LPSTR,b,WORD,c)
Fc(int,GetInstanceData,HANDLE,a,NPSTR,b,int,c)
Fc(int,GetKeyNameText,LONG,a,LPSTR,b,int,c)
Fc(int,GetModuleFileName,HANDLE,a,LPSTR,b,int,c)
Fc(int,GetObject,HANDLE,a,int,b,LPSTR,c)
Fc(int,GetTextFace,HDC,a,int,b,LPSTR,c)
Fc(int,GetUpdateRgn,HWND,a,HRGN,b,BOOL,c)
Fc(int,GetWindowText,HWND,a,LPSTR,b,int,c)
Fc(int,IntersectRect,LPRECT,a,LPRECT,b,LPRECT,c)
Fc(int,MulDiv,int,a,int,b,int,c)
Fc(int,OffsetClipRgn,HDC,a,int,b,int,c)
Fc(int,OffsetRgn,HRGN,a,int,b,int,c)
Fc(int,OpenComm,LPSTR,a,WORD,b,WORD,c)
Fc(int,OpenFile,LPSTR,a,LPOFSTRUCT,b,WORD,c)
Fc(int,ReadComm,int,a,LPSTR,b,int,c)
Fc(int,SetEnvironment,LPSTR,a,LPSTR,b,WORD,c)
Fc(int,SetTextJustification,HDC,a,int,b,int,c)
Fc(int,SetVoiceEnvelope,int,a,int,b,int,c)
Fc(int,SetVoiceSound,int,a,LONG,b,int,c)
Fc(int,TranslateAccelerator,HWND,a,HANDLE,b,LPMSG,c)
Fc(int,UnionRect,LPRECT,a,LPRECT,b,LPRECT,c)
Fc(int,WriteComm,int,a,LPSTR,b,int,c)
Fc(int,wvsprintf,LPSTR,a,LPSTR,b,LPSTR,c)
Fc(void,AdjustWindowRect,LPRECT,a,LONG,b,BOOL,c)
Fc(void,AnsiToOemBuff,LPSTR,a,LPSTR,b,int,c)
Fc(void,CheckDlgButton,HWND,a,int,b,WORD,c)
Fc(void,InflateRect,LPRECT,a,int,b,int,c)
Fc(void,InvalidateRect,HWND,a,LPRECT,b,BOOL,c)
Fc(void,InvalidateRgn,HWND,a,HRGN,b,BOOL,c)
Fc(void,OemToAnsiBuff,LPSTR,a,LPSTR,b,int,c)
Fc(void,OffsetRect,LPRECT,a,int,b,int,c)
Fc(void,SetDlgItemText,HWND,a,int,b,LPSTR,c)
Fc(void,SetSysColors,int,a,LPINT,b,LONG*,c)
Fc(void,ShowScrollBar,HWND,a,WORD,b,BOOL,c)
Fc(void,SwitchStackTo,WORD,a,WORD,b,WORD,c)
Fd(BOOL,DrawIcon,HDC,a,int,b,int,c,HICON,d)
Fd(BOOL,EnumMetaFile,HDC,a,LOCALHANDLE,b,FARPROC,c,BYTE FAR*,d)
Fd(BOOL,FloodFill,HDC,a,int,b,int,c,DWORD,d)
Fd(BOOL,GetCharWidth,HDC,a,WORD,b,WORD,c,LPINT,d)
Fd(BOOL,HiliteMenuItem,HWND,a,HMENU,b,WORD,c,WORD,d)
Fd(BOOL,PolyPolygon,HDC,a,LPPOINT,b,LPINT,c,int,d)
Fd(BOOL,PostAppMessage,HANDLE,a,WORD,b,WORD,c,LONG,d)
Fd(BOOL,PostMessage,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(BOOL,WinHelp,HWND,hwndMain,LPSTR,lpszHelp,WORD,usCommand,DWORD,ulData)
Fd(BOOL,WritePrivateProfileString,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d)
Fd(DWORD,DefHookProc,int,a,WORD,b,DWORD,c,FARPROC FAR*,d)
Fd(DWORD,SetPixel,HDC,a,int,b,int,c,DWORD,d)
Fd(HDC,CreateDC,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d)
Fd(HDC,CreateIC,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d)
Fd(HRGN,CreateEllipticRgn,int,a,int,b,int,c,int,d)
Fd(HRGN,CreatePolyPolygonRgn,LPPOINT,a,LPINT,b,int,c,int,d)
Fd(HRGN,CreateRectRgn,int,a,int,b,int,c,int,d)
Fd(HWND,CreateDialog,HANDLE,a,LPSTR,b,HWND,c,FARPROC,d)
Fd(HWND,CreateDialogIndirect,HANDLE,a,LPSTR,b,HWND,c,FARPROC,d)
Fd(LONG,DefDlgProc,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(LONG,DefMDIChildProc,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(LONG,DefWindowProc,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(LONG,SendMessage,HWND,a,WORD,b,WORD,c,LONG,d)
Fd(WORD,GetDlgItemInt,HWND,a,int,b,BOOL FAR*,c,BOOL,d)
Fd(WORD,GetPaletteEntries,HPALETTE,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(WORD,GetPrivateProfileInt,LPSTR,a,LPSTR,b,int,c,LPSTR,d)
Fd(WORD,GetSystemPaletteEntries,HDC,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(WORD,SetPaletteEntries,HPALETTE,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(WORD,SetTimer,HWND,a,int,d,WORD,b,FARPROC,c)
Fd(int,CombineRgn,HRGN,a,HRGN,b,HRGN,c,int,d)
Fd(int,DialogBox,HANDLE,a,LPSTR,b,HWND,c,FARPROC,d)
Fd(int,DialogBoxIndirect,HANDLE,a,HANDLE,b,HWND,c,FARPROC,d)
Fd(int,EnumFonts,HDC,a,LPSTR,b,FARPROC,c,LPSTR,d)
Fd(int,EnumObjects,HDC,a,int,b,FARPROC,c,LPSTR,d)
Fd(int,GetDlgItemText,HWND,a,int,b,LPSTR,c,int,d)
Fd(int,GetTempFileName,BYTE,a,LPSTR,b,WORD,c,LPSTR,d)
Fd(int,LoadString,HANDLE,a,WORD,b,LPSTR,c,int,d)
Fd(int,MessageBox,HWND,a,LPSTR,b,LPSTR,c,WORD,d)
Fd(int,SetScrollPos,HWND,a,int,b,int,c,BOOL,d)
Fd(int,SetVoiceNote,int,a,int,b,int,c,int,d)
Fd(void,AdjustWindowRectEx,LPRECT,a,LONG,b,BOOL,c,DWORD,d)
Fd(void,AnimatePalette,HPALETTE,a,WORD,b,WORD,c,LPPALETTEENTRY,d)
Fd(void,CheckRadioButton,HWND,a,int,b,int,c,int,d)
Fd(void,CreateCaret,HWND,a,HBITMAP,b,int,c,int,d)
Fd(void,GetScrollRange,HWND,a,int,b,LPINT,c,LPINT,d)
Fd(void,PlayMetaFileRecord,HDC,a,LPHANDLETABLE,b,LPMETARECORD,c,WORD,d)
Fd(void,SetDlgItemInt,HWND,a,int,b,WORD,c,BOOL,d)
Fe(BOOL,ChangeMenu,HMENU,a,WORD,b,LPSTR,c,WORD,d,WORD,e)
Fe(BOOL,Ellipse,HDC,a,int,b,int,c,int,d,int,e)
Fe(BOOL,ExtFloodFill,HDC,a,int,b,int,c,DWORD,d,WORD,e)
Fe(BOOL,FrameRgn,HDC,a,HRGN,b,HBRUSH,e,int,c,int,d)
Fe(BOOL,InsertMenu,HMENU,a,WORD,b,WORD,c,WORD,d,LPSTR,e)
Fe(BOOL,ModifyMenu,HMENU,a,WORD,b,WORD,c,WORD,d,LPSTR,e)
Fe(BOOL,PeekMessage,LPMSG,a,HWND,b,WORD,c,WORD,d,WORD,e)
Fe(BOOL,SetMenuItemBitmaps,HMENU,a,WORD,b,WORD,c,HBITMAP,d,HBITMAP,e)
Fe(BOOL,TextOut,HDC,a,int,b,int,c,LPSTR,d,int,e)
Fe(DWORD,GetTabbedTextExtent,HDC,a,LPSTR,b,int,c,int,d,LPINT,e)
Fe(DWORD,ScaleViewportExt,HDC,a,int,b,int,c,int,d,int,e)
Fe(DWORD,ScaleWindowExt,HDC,a,int,b,int,c,int,d,int,e)
Fe(HBITMAP,CreateBitmap,int,a,int,b,BYTE,c,BYTE,d,LPSTR,e)
Fe(HWND,CreateDialogIndirectParam,HANDLE,a,LPSTR,b,HWND,c,FARPROC,d,LONG,e)
Fe(HWND,CreateDialogParam,HANDLE,a,LPSTR,b,HWND,c,FARPROC,d,LONG,e)
Fe(LONG,CallWindowProc,FARPROC,a,HWND,b,WORD,c,WORD,d,LONG,e)
Fe(LONG,DefFrameProc,HWND,a,HWND,b,WORD,c,WORD,d,LONG,e)
Fe(LONG,SendDlgItemMessage,HWND,a,int,b,WORD,c,WORD,d,LONG,e)
Fe(int,DialogBoxIndirectParam,HANDLE,a,HANDLE,b,HWND,c,FARPROC,d,LONG,e)
Fe(int,DialogBoxParam,HANDLE,a,LPSTR,b,HWND,c,FARPROC,d,LONG,e)
Fe(int,DlgDirList,HWND,a,LPSTR,b,int,c,int,d,WORD,e)
Fe(int,DlgDirListComboBox,HWND,a,LPSTR,b,int,c,int,d,WORD,e)
Fe(int,Escape,HDC,a,int,b,int,c,LPSTR,d,LPSTR,e)
Fe(int,ExcludeClipRect,HDC,a,int,b,int,c,int,d,int,e)
Fe(int,GetMenuString,HMENU,a,WORD,b,LPSTR,c,int,d,WORD,e)
Fe(int,GetProfileString,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d,int,e)
Fe(int,IntersectClipRect,HDC,a,int,b,int,c,int,d,int,e)
Fe(int,SetVoiceAccent,int,a,int,b,int,c,int,d,int,e)
Fe(int,ToAscii,WORD,wVirtKey,WORD,wScanCode,LPSTR,lpKeyState,LPVOID,lpChar,WORD,wFlags)
Fe(void,ScrollWindow,HWND,a,int,b,int,c,LPRECT,d,LPRECT,e)
Fe(void,SetRect,LPRECT,a,int,b,int,c,int,d,int,e)
Fe(void,SetRectRgn,HRGN,a,int,b,int,c,int,d,int,e)
Fe(void,SetScrollRange,HWND,a,int,b,int,c,int,d,BOOL,e)
Ff(BOOL,PatBlt,HDC,a,int,b,int,c,int,d,int,e,DWORD,f)
Ff(HBITMAP,CreateDIBitmap,HDC,a,LPBITMAPINFOHEADER,b,DWORD,c,LPSTR,d,LPBITMAPINFO,e,WORD,f)
Ff(HRGN,CreateRoundRectRgn,int,a,int,b,int,c,int,d,int,e,int,f)
Ff(int,GetPrivateProfileString,LPSTR,a,LPSTR,b,LPSTR,c,LPSTR,d,int,e,LPSTR,f)
Ff(void,LineDDA,int,a,int,b,int,c,int,d,FARPROC,e,LPSTR,f)
Ff(void,MoveWindow,HWND,a,int,b,int,c,int,d,int,e,BOOL,f)
Fg(BOOL,RoundRect,HDC,a,int,b,int,c,int,d,int,e,int,f,int,g)
Fg(BOOL,ScrollDC,HDC,a,int,b,int,c,LPRECT,d,LPRECT,e,HRGN,f,LPRECT,g)
Fg(BOOL,TrackPopupMenu,HMENU,a,WORD,b,int,c,int,d,int,e,HWND,f,LPRECT,g)
Fg(HCURSOR,CreateCursor,HANDLE,a,int,b,int,c,int,d,int,e,LPSTR,f,LPSTR,g)
Fg(HICON,CreateIcon,HANDLE,a,int,b,int,c,BYTE,d,BYTE,e,LPSTR,f,LPSTR,g)
Fg(int,GetDIBits,HDC,a,HANDLE,a2,WORD,b,WORD,c,LPSTR,d,LPBITMAPINFO,e,WORD,f)
Fg(int,SetDIBits,HDC,a,HANDLE,a2,WORD,b,WORD,c,LPSTR,d,LPBITMAPINFO,e,WORD,f)
Fg(void,SetWindowPos,HWND,a,HWND,b,int,c,int,d,int,e,int,f,WORD,g)
Fh(BOOL,ExtTextOut,HDC,a,int,b,int,c,WORD,d,LPRECT,e,LPSTR,f,WORD,g,LPINT,h)
Fh(HANDLE,DeferWindowPos,HANDLE,hWinPosInfo,HWND,hWnd,HWND,hWndInsertAfter,int,x,int,y,int,cx,int,cy,WORD,wFlags)
Fh(LONG,TabbedTextOut,HDC,a,int,b,int,c,LPSTR,d,int,e,int,f,LPINT,g,int,h)
Fi(BOOL,BitBlt,HDC,a,int,b,int,c,int,d,int,e,HDC,f,int,g,int,h,DWORD,i)
Fi(BOOL,GrayString,HDC,a,HBRUSH,b,FARPROC,c,DWORD,d,int,e,int,f,int,g,int,h,int,i)
Fk(BOOL,StretchBlt,HDC,a,int,b,int,c,int,d,int,e,HDC,f,int,g,int,h,int,i,int,j,DWORD,k)
Fl(HWND,CreateWindowEx,DWORD,a,LPSTR,b,LPSTR,c,DWORD,d,int,e,int,f,int,g,int,h,HWND,i,HMENU,j,HANDLE,k,LPSTR,l)
Fl(int,SetDIBitsToDevice,HDC,a,WORD,b,WORD,c,WORD,d,WORD,e,WORD,f,WORD,g,WORD,h,WORD,i,LPSTR,j,LPBITMAPINFO,k,WORD,l)
Fm(int,StretchDIBits,HDC,a,WORD,b,WORD,c,WORD,d,WORD,e,WORD,f,WORD,g,WORD,h,WORD,i,LPSTR,j,LPBITMAPINFO,k,WORD,l,DWORD,m)
Fn(HFONT,CreateFont,int,a,int,b,int,c,int,d,int,e,BYTE,f,BYTE,g,BYTE,h,BYTE,i,BYTE,j,BYTE,k,BYTE,l,BYTE,m,LPSTR,n)
