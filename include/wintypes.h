#ifndef __WINE_WINTYPES_H
#define __WINE_WINTYPES_H

#ifdef WINELIB
# ifdef WINELIB16
#  undef WINELIB32
# else
#  ifndef WINELIB32
#   define WINELIB32
#  endif
# endif
#else
# ifdef WINELIB32
#  undef WINELIB16
#  define WINELIB
# endif
# ifdef WINELIB16
#  define WINELIB
# endif
#endif

typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned short BOOL;
typedef unsigned char BYTE;
typedef long LONG;
#ifdef WINELIB32
typedef int INT;
typedef unsigned int UINT;
typedef short SHORT;
typedef LONG WPARAM;
typedef void* HANDLE;
typedef void* NPVOID;
typedef void* SEGPTR;
#define UIFMT "%u"
#define NPFMT "%p"
#define SPFMT "%p"
#else
typedef short INT;
typedef unsigned short UINT;
typedef UINT WPARAM;
typedef WORD HANDLE;
typedef WORD NPVOID;
typedef DWORD SEGPTR;
#define UIFMT "%hu"
#define NPFMT "%04x"
#define SPFMT "%08lx"
#endif
typedef LONG LPARAM;
typedef LONG LRESULT;
typedef DWORD HHOOK;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef LPCSTR LPCTSTR;
typedef char *NPSTR;
typedef INT *LPINT;
typedef UINT *LPUINT;
typedef WORD *LPWORD;
typedef DWORD *LPDWORD;
typedef LONG *LPLONG;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef WORD CATCHBUF[9];
typedef WORD *LPCATCHBUF;

#define DECLARE_HANDLE(a) typedef HANDLE a;

DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HCLASS);
DECLARE_HANDLE(HCURSOR);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HDROP);
DECLARE_HANDLE(HDRVR);
DECLARE_HANDLE(HDWP);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HGLOBAL);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HMODULE);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HQUEUE);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(LOCALHANDLE);

#ifdef WINELIB
typedef long (*FARPROC)();
typedef LRESULT (*WNDPROC)(HWND,UINT,WPARAM,LPARAM);
#else
typedef SEGPTR FARPROC;
typedef SEGPTR WNDPROC;
#endif
typedef FARPROC DLGPROC;
typedef FARPROC HOOKPROC;

#define TRUE 1
#define FALSE 0
#define CW_USEDEFAULT ((INT)0x8000)
#define FAR
#define NEAR
#define PASCAL
#define VOID                void
#define WINAPI              PASCAL
#define CALLBACK            PASCAL

#undef NULL
#define NULL 0

#ifdef WINELIB
#define WINE_PACKED
#else
#define WINE_PACKED __attribute__ ((packed))
#endif

#define LOBYTE(w)           ((BYTE)(UINT)(w))
#define HIBYTE(w)           ((BYTE)((UINT)(w) >> 8))

#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))

#define SLOWORD(l)           ((INT)(LONG)(l))
#define SHIWORD(l)           ((INT)((LONG)(l) >> 16))

#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | \
				    (((DWORD)((WORD)(high))) << 16)))

#define SELECTOROF(ptr)     (HIWORD(ptr))
#define OFFSETOF(ptr)       (LOWORD(ptr))

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef DONT_DEFINE_min_AND_max
#ifndef min
#define min(a,b) MIN(a,b)
#endif
#ifndef max
#define max(a,b) MAX(a,b)
#endif
#endif

#endif /* __WINE_WINTYPES_H */
