/*
 * Basic types definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

/* Misc. constants. */

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE  1

#ifdef NULL
#undef NULL
#endif
#define NULL  0

#ifndef __WINE_WINTYPES_H
#define __WINE_WINTYPES_H

#ifdef __WINE__
# include "config.h"
# undef UNICODE
#endif  /* __WINE__ */

#ifdef __cplusplus
extern "C" {
#endif

/* Macros to map Winelib names to the correct implementation name */
/* depending on __WINE__ and UNICODE macros.                      */
/* Note that Winelib is purely Win32.                             */

#ifdef __WINE__
# define WINELIB_NAME_AW(func)   this_is_a_syntax_error this_is_a_syntax_error
#else  /* __WINE__ */
# ifdef UNICODE
#  define WINELIB_NAME_AW(func) func##W
# else
#  define WINELIB_NAME_AW(func) func##A
# endif  /* UNICODE */
#endif  /* __WINE__ */

#ifdef __WINE__
# define DECL_WINELIB_TYPE_AW(type)  /* nothing */
#else   /* __WINE__ */
# define DECL_WINELIB_TYPE_AW(type)  typedef WINELIB_NAME_AW(type) type;
#endif  /* __WINE__ */


/* Calling conventions definitions */

#ifdef __i386__
# if defined(__GNUC__) && (__GNUC__ == 2) && (__GNUC_MINOR__ >= 7)
#  define __stdcall __attribute__((__stdcall__))
#  define __cdecl   __attribute__((__cdecl__))
#  define __RESTORE_ES  __asm__ __volatile__("pushl %ds\n\tpopl %es")
# else
#  error You need gcc >= 2.7 to build Wine on a 386
# endif  /* __GNUC__ */
#else  /* __i386__ */
# define __stdcall
# define __cdecl
# define __RESTORE_ES
#endif  /* __i386__ */

#define CALLBACK    __stdcall
#define WINAPI      __stdcall
#define APIPRIVATE  __stdcall
#define PASCAL      __stdcall
#define _pascal     __stdcall
#define __export    __stdcall
#define WINAPIV     __cdecl
#define APIENTRY    WINAPI

#define CONST       const

/* Standard data types. These are the same for emulator and library. */

typedef void            VOID;
typedef short           INT16;
typedef unsigned short  UINT16;
typedef int             INT;
typedef unsigned int    UINT;
typedef unsigned short  WORD;
typedef unsigned long   DWORD;
typedef unsigned long   ULONG;
typedef unsigned char   BYTE;
typedef long            LONG;
typedef short           SHORT;
typedef unsigned short  USHORT;
typedef char            CHAR;
typedef unsigned char   UCHAR;
/* Some systems might have wchar_t, but we really need 16 bit characters */
typedef unsigned short  WCHAR;
typedef unsigned short  BOOL16;
typedef int             BOOL;
typedef double          DATE;
typedef long            LONG_PTR;
typedef unsigned long   ULONG_PTR;
typedef double          DOUBLE;
typedef double          LONGLONG;
typedef double          ULONGLONG;

/* Integer types. These are the same for emulator and library. */

typedef UINT16          HANDLE16;
typedef UINT            HANDLE;
typedef UINT16         *LPHANDLE16;
typedef UINT           *LPHANDLE;
typedef UINT16          WPARAM16;
typedef UINT            WPARAM;
typedef LONG            LPARAM;
typedef LONG            HRESULT;
typedef LONG            LRESULT;
typedef WORD            ATOM;
typedef WORD            CATCHBUF[9];
typedef WORD           *LPCATCHBUF;
typedef DWORD           ACCESS_MASK;
typedef ACCESS_MASK     REGSAM;
typedef HANDLE          HHOOK;
typedef HANDLE          HKEY;
typedef HANDLE          HMONITOR;
typedef DWORD           LCID;
typedef WORD            LANGID;
typedef DWORD           LCTYPE;
typedef float           FLOAT;
typedef long long       __int64;

/* Pointers types. These are the same for emulator and library. */
/* winnt types */
typedef void           *PVOID;
typedef const void     *PCVOID;
typedef CHAR           *PCHAR;
typedef UCHAR          *PUCHAR;
typedef BYTE           *PBYTE;
typedef ULONG          *PULONG;
typedef LONG           *PLONG;
typedef DWORD          *PDWORD;
/* common win32 types */
typedef CHAR           *LPSTR;
typedef CHAR           *PSTR;
typedef const CHAR     *LPCSTR;
typedef const CHAR     *PCSTR;
typedef WCHAR          *LPWSTR;
typedef WCHAR          *PWSTR;
typedef const WCHAR    *LPCWSTR;
typedef const WCHAR    *PCWSTR;
typedef BYTE           *LPBYTE;
typedef WORD           *LPWORD;
typedef DWORD          *LPDWORD;
typedef LONG           *LPLONG;
typedef VOID           *LPVOID;
typedef const VOID     *LPCVOID;
typedef INT16          *LPINT16;
typedef UINT16         *LPUINT16;
typedef INT            *PINT;
typedef INT            *LPINT;
typedef UINT           *PUINT;
typedef UINT           *LPUINT;
typedef HKEY           *LPHKEY;
typedef HKEY           *PHKEY;
typedef FLOAT          *PFLOAT;
typedef FLOAT          *LPFLOAT;
typedef BOOL           *PBOOL;
typedef BOOL           *LPBOOL;

/* Special case: a segmented pointer is just a pointer in the user's code. */

#ifdef __WINE__
typedef DWORD SEGPTR;
#else
typedef void* SEGPTR;
#endif /* __WINE__ */

/* Handle types that exist both in Win16 and Win32. */

#define DECLARE_HANDLE(a)  typedef HANDLE16 a##16; typedef HANDLE a
DECLARE_HANDLE(HACCEL);
DECLARE_HANDLE(HBITMAP);
DECLARE_HANDLE(HBRUSH);
DECLARE_HANDLE(HCOLORSPACE);
DECLARE_HANDLE(HCURSOR);
DECLARE_HANDLE(HDC);
DECLARE_HANDLE(HDROP);
DECLARE_HANDLE(HDRVR);
DECLARE_HANDLE(HDWP);
DECLARE_HANDLE(HENHMETAFILE);
DECLARE_HANDLE(HFILE);
DECLARE_HANDLE(HFONT);
DECLARE_HANDLE(HGDIOBJ);
DECLARE_HANDLE(HGLOBAL);
DECLARE_HANDLE(HICON);
DECLARE_HANDLE(HINSTANCE);
DECLARE_HANDLE(HLOCAL);
DECLARE_HANDLE(HMENU);
DECLARE_HANDLE(HMETAFILE);
DECLARE_HANDLE(HMIDI);
DECLARE_HANDLE(HMIDIIN);
DECLARE_HANDLE(HMIDIOUT);
DECLARE_HANDLE(HMIDISTRM);
DECLARE_HANDLE(HMIXER);
DECLARE_HANDLE(HMIXEROBJ);
DECLARE_HANDLE(HMMIO);
DECLARE_HANDLE(HMODULE);
DECLARE_HANDLE(HPALETTE);
DECLARE_HANDLE(HPEN);
DECLARE_HANDLE(HQUEUE);
DECLARE_HANDLE(HRGN);
DECLARE_HANDLE(HRSRC);
DECLARE_HANDLE(HTASK);
DECLARE_HANDLE(HWAVE);
DECLARE_HANDLE(HWAVEIN);
DECLARE_HANDLE(HWAVEOUT);
DECLARE_HANDLE(HWINSTA);
DECLARE_HANDLE(HDESK);
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HKL);
DECLARE_HANDLE(HIC);
DECLARE_HANDLE(HRASCONN);
#undef DECLARE_HANDLE

/* Callback function pointers types */

typedef BOOL    (CALLBACK* DATEFMT_ENUMPROCA)(LPSTR);
typedef BOOL    (CALLBACK* DATEFMT_ENUMPROCW)(LPWSTR);
DECL_WINELIB_TYPE_AW(DATEFMT_ENUMPROC)
typedef BOOL16  (CALLBACK *DLGPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef BOOL    (CALLBACK *DLGPROC)(HWND,UINT,WPARAM,LPARAM);
typedef LRESULT (CALLBACK *DRIVERPROC16)(DWORD,HDRVR16,UINT16,LPARAM,LPARAM);
typedef LRESULT (CALLBACK *DRIVERPROC)(DWORD,HDRVR,UINT,LPARAM,LPARAM);
typedef INT16   (CALLBACK *EDITWORDBREAKPROC16)(LPSTR,INT16,INT16,INT16);
typedef INT     (CALLBACK *EDITWORDBREAKPROCA)(LPSTR,INT,INT,INT);
typedef INT     (CALLBACK *EDITWORDBREAKPROCW)(LPWSTR,INT,INT,INT);
DECL_WINELIB_TYPE_AW(EDITWORDBREAKPROC)
typedef LRESULT (CALLBACK *FARPROC16)();
typedef LRESULT (CALLBACK *FARPROC)();
typedef INT16   (CALLBACK *PROC16)();
typedef INT     (CALLBACK *PROC)();
typedef INT16   (CALLBACK *GOBJENUMPROC16)(SEGPTR,LPARAM);
typedef INT     (CALLBACK *GOBJENUMPROC)(LPVOID,LPARAM);
typedef BOOL16  (CALLBACK *GRAYSTRINGPROC16)(HDC16,LPARAM,INT16);
typedef BOOL    (CALLBACK *GRAYSTRINGPROC)(HDC,LPARAM,INT);
typedef LRESULT (CALLBACK *HOOKPROC16)(INT16,WPARAM16,LPARAM);
typedef LRESULT (CALLBACK *HOOKPROC)(INT,WPARAM,LPARAM);
typedef VOID    (CALLBACK *LINEDDAPROC16)(INT16,INT16,LPARAM);
typedef VOID    (CALLBACK *LINEDDAPROC)(INT,INT,LPARAM);
typedef BOOL16  (CALLBACK *PROPENUMPROC16)(HWND16,SEGPTR,HANDLE16);
typedef BOOL    (CALLBACK *PROPENUMPROCA)(HWND,LPCSTR,HANDLE);
typedef BOOL    (CALLBACK *PROPENUMPROCW)(HWND,LPCWSTR,HANDLE);
DECL_WINELIB_TYPE_AW(PROPENUMPROC)
typedef BOOL    (CALLBACK *PROPENUMPROCEXA)(HWND,LPCSTR,HANDLE,LPARAM);
typedef BOOL    (CALLBACK *PROPENUMPROCEXW)(HWND,LPCWSTR,HANDLE,LPARAM);
DECL_WINELIB_TYPE_AW(PROPENUMPROCEX)
typedef BOOL    (CALLBACK* TIMEFMT_ENUMPROCA)(LPSTR);
typedef BOOL    (CALLBACK* TIMEFMT_ENUMPROCW)(LPWSTR);
DECL_WINELIB_TYPE_AW(TIMEFMT_ENUMPROC)
typedef VOID    (CALLBACK *TIMERPROC16)(HWND16,UINT16,UINT16,DWORD);
typedef VOID    (CALLBACK *TIMERPROC)(HWND,UINT,UINT,DWORD);
typedef LRESULT (CALLBACK *WNDENUMPROC16)(HWND16,LPARAM);
typedef LRESULT (CALLBACK *WNDENUMPROC)(HWND,LPARAM);
typedef LRESULT (CALLBACK *WNDPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef LRESULT (CALLBACK *WNDPROC)(HWND,UINT,WPARAM,LPARAM);

/* TCHAR data types definitions for Winelib. */
/* These types are _not_ defined for the emulator, because they */
/* depend on the UNICODE macro that only exists in user's code. */

#ifndef __WINE__
# ifdef UNICODE
typedef WCHAR TCHAR;
typedef LPWSTR LPTSTR;
typedef LPCWSTR LPCTSTR;
#define __TEXT(string) L##string /*probably wrong */
# else  /* UNICODE */
typedef CHAR TCHAR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
#define __TEXT(string) string
# endif /* UNICODE */
#endif   /* __WINE__ */
#define TEXT(quote) __TEXT(quote)

/* Data types specific to the library. These do _not_ exist in the emulator. */



/* Define some empty macros for compatibility with Windows code. */

#ifndef __WINE__
#define NEAR
#define FAR
#define _far
#define _near
#define IN
#define OUT
#define OPTIONAL
#endif  /* __WINE__ */

/* Macro for structure packing. */

#ifdef __GNUC__
#define WINE_PACKED __attribute__ ((packed))
#define WINE_UNUSED __attribute__ ((unused))
#else
#define WINE_PACKED  /* nothing */
#define WINE_UNUSED  /* nothing */
#endif

/* Macros to split words and longs. */

#define LOBYTE(w)              ((BYTE)(WORD)(w))
#define HIBYTE(w)              ((BYTE)((WORD)(w) >> 8))

#define LOWORD(l)              ((WORD)(DWORD)(l))
#define HIWORD(l)              ((WORD)((DWORD)(l) >> 16))

#define SLOWORD(l)             ((INT16)(LONG)(l))
#define SHIWORD(l)             ((INT16)((LONG)(l) >> 16))

#define MAKEWORD(low,high)     ((WORD)(((BYTE)(low)) | ((WORD)((BYTE)(high))) << 8))
#define MAKELONG(low,high)     ((LONG)(((WORD)(low)) | (((DWORD)((WORD)(high))) << 16)))
#define MAKELPARAM(low,high)   ((LPARAM)MAKELONG(low,high))
#define MAKEWPARAM(low,high)   ((WPARAM)MAKELONG(low,high))
#define MAKELRESULT(low,high)  ((LRESULT)MAKELONG(low,high))
#define MAKEINTATOM(atom)      ((LPCSTR)MAKELONG((atom),0))

#define SELECTOROF(ptr)     (HIWORD(ptr))
#define OFFSETOF(ptr)       (LOWORD(ptr))

/* Macros to access unaligned or wrong-endian WORDs and DWORDs. */
/* Note: These macros are semantically broken, at least for wrc.  wrc
   spits out data in the platform's current binary format, *not* in 
   little-endian format.  These macros are used throughout the resource
   code to load and store data to the resources.  Since it is unlikely 
   that we'll ever be dealing with little-endian resource data, the 
   byte-swapping nature of these macros has been disabled.  Rather than 
   remove the use of these macros from the resource loading code, the
   macros have simply been disabled.  In the future, someone may want 
   to reactivate these macros for other purposes.  In that case, the
   resource code will have to be modified to use different macros. */ 

#if 1
#define PUT_WORD(ptr,w)   (*(WORD *)(ptr) = (w))
#define GET_WORD(ptr)     (*(WORD *)(ptr))
#define PUT_DWORD(ptr,dw) (*(DWORD *)(ptr) = (dw))
#define GET_DWORD(ptr)    (*(DWORD *)(ptr))
#else
#define PUT_WORD(ptr,w)   (*(BYTE *)(ptr) = LOBYTE(w), \
                           *((BYTE *)(ptr) + 1) = HIBYTE(w))
#define GET_WORD(ptr)     ((WORD)(*(BYTE *)(ptr) | \
                                  (WORD)(*((BYTE *)(ptr)+1) << 8)))
#define PUT_DWORD(ptr,dw) (PUT_WORD((ptr),LOWORD(dw)), \
                           PUT_WORD((WORD *)(ptr)+1,HIWORD(dw)))
#define GET_DWORD(ptr)    ((DWORD)(GET_WORD(ptr) | \
                                   ((DWORD)GET_WORD((WORD *)(ptr)+1) << 16)))
#endif  /* 1 */

/* MIN and MAX macros */

#ifdef MAX
#undef MAX
#endif
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#ifdef MIN
#undef MIN
#endif
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define __max(a,b) MAX(a,b)
#define __min(a,b) MIN(a,b)
#define max(a,b)   MAX(a,b)
#define min(a,b)   MIN(a,b)

#define _MAX_PATH  260
#define _MAX_DRIVE 3
#define _MAX_DIR   256
#define _MAX_FNAME 255
#define _MAX_EXT   256

/* Winelib run-time flag */

#ifdef __WINE__
extern int __winelib;
#endif  /* __WINE__ */

/* The SIZE structure */

typedef struct
{
    INT16  cx;
    INT16  cy;
} SIZE16, *PSIZE16, *LPSIZE16;

typedef struct tagSIZE
{
    INT  cx;
    INT  cy;
} SIZE, *PSIZE, *LPSIZE;


typedef SIZE SIZEL, *PSIZEL, *LPSIZEL;

#define CONV_SIZE16TO32(s16,s32) \
            ((s32)->cx = (INT)(s16)->cx, (s32)->cy = (INT)(s16)->cy)
#define CONV_SIZE32TO16(s32,s16) \
            ((s16)->cx = (INT16)(s32)->cx, (s16)->cy = (INT16)(s32)->cy)

/* The POINT structure */

typedef struct
{
    INT16  x;
    INT16  y;
} POINT16, *PPOINT16, *LPPOINT16;

typedef struct tagPOINT
{
    LONG  x;
    LONG  y;
} POINT, *PPOINT, *LPPOINT;


#define CONV_POINT16TO32(p16,p32) \
            ((p32)->x = (INT)(p16)->x, (p32)->y = (INT)(p16)->y)
#define CONV_POINT32TO16(p32,p16) \
            ((p16)->x = (INT16)(p32)->x, (p16)->y = (INT16)(p32)->y)

#define MAKEPOINT16(l) (*((POINT16 *)&(l)))

/* The POINTS structure */

typedef struct tagPOINTS
{
	SHORT x;
	SHORT y;
} POINTS, *PPOINTS, *LPPOINTS;


#define MAKEPOINTS(l)  (*((POINTS *)&(l)))


/* The RECT structure */

typedef struct
{
    INT16  left;
    INT16  top;
    INT16  right;
    INT16  bottom;
} RECT16, *LPRECT16;

typedef struct tagRECT
{
    INT  left;
    INT  top;
    INT  right;
    INT  bottom;
} RECT, *PRECT, *LPRECT;
typedef const RECT *LPCRECT32;


typedef struct tagRECTL
{
    LONG left;
    LONG top;  
    LONG right;
    LONG bottom;
} RECTL, *PRECTL, *LPRECTL;

typedef const RECTL *LPCRECTL;

#define CONV_RECT16TO32(r16,r32) \
    ((r32)->left  = (INT)(r16)->left,  (r32)->top    = (INT)(r16)->top, \
     (r32)->right = (INT)(r16)->right, (r32)->bottom = (INT)(r16)->bottom)
#define CONV_RECT32TO16(r32,r16) \
    ((r16)->left  = (INT16)(r32)->left,  (r16)->top    = (INT16)(r32)->top, \
     (r16)->right = (INT16)(r32)->right, (r16)->bottom = (INT16)(r32)->bottom)

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WINTYPES_H */
