#ifndef __WINE_WINTYPES_H
#define __WINE_WINTYPES_H

typedef short	INT;
typedef unsigned short UINT;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef unsigned short BOOL;
typedef unsigned char BYTE;
typedef long LONG;
typedef UINT WPARAM;
typedef LONG LPARAM;
typedef LONG LRESULT;
typedef WORD HANDLE;
typedef DWORD HHOOK;
typedef DWORD SEGPTR;
typedef char *LPSTR;
typedef const char *LPCSTR;
typedef char *NPSTR;
typedef INT *LPINT;
typedef UINT *LPUINT;
typedef WORD *LPWORD;
typedef DWORD *LPDWORD;
typedef LONG *LPLONG;
typedef void *LPVOID;
#ifdef WINELIB
typedef long (*FARPROC)();
#else
typedef SEGPTR FARPROC;
#endif
typedef FARPROC DLGPROC;
typedef int CATCHBUF[9];
typedef int *LPCATCHBUF;
typedef FARPROC HOOKPROC;

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
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(LOCALHANDLE);

#define TRUE 1
#define FALSE 0
#define CW_USEDEFAULT ((INT)0x8000)
#define FAR
#define NEAR
#define PASCAL
#define VOID                void
#define WINAPI              PASCAL
#define CALLBACK            PASCAL

#ifndef NULL
#define NULL (0)
#endif

#ifdef WINELIB
#define WINE_PACKED
#else
#define WINE_PACKED __attribute__ ((packed))
#endif

#define LOBYTE(w)           ((BYTE)(UINT)(w))
#define HIBYTE(w)           ((BYTE)((UINT)(w) >> 8))

#define LOWORD(l)           ((WORD)(DWORD)(l))
#define HIWORD(l)           ((WORD)((DWORD)(l) >> 16))

#define MAKELONG(low, high) ((LONG)(((WORD)(low)) | \
				    (((DWORD)((WORD)(high))) << 16)))

#define SELECTOROF(ptr)     (HIWORD(ptr))
#define OFFSETOF(ptr)       (LOWORD(ptr))

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#endif /* __WINE_WINTYPES_H */
