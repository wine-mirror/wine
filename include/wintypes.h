/*
 * Basic types definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

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
# define WINELIB_NAME(func)      this_is_a_syntax_error this_is_a_syntax_error
# define WINELIB_NAME_AW(func)   this_is_a_syntax_error this_is_a_syntax_error
#else  /* __WINE__ */
# define WINELIB_NAME(func)     func##32
# ifdef UNICODE
#  define WINELIB_NAME_AW(func) func##32W
# else
#  define WINELIB_NAME_AW(func) func##32A
# endif  /* UNICODE */
#endif  /* __WINE__ */

#ifdef __WINE__
# define DECL_WINELIB_TYPE(type)     /* nothing */
# define DECL_WINELIB_TYPE_AW(type)  /* nothing */
#else   /* __WINE__ */
# define DECL_WINELIB_TYPE(type)     typedef WINELIB_NAME(type) type;
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
typedef int             INT32;
typedef unsigned int    UINT32;
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
typedef int             BOOL32;
typedef double          DATE;
#ifdef __i386__
typedef double          LONGLONG;
typedef double          ULONGLONG;
#endif /*__i386__*/

/* Integer types. These are the same for emulator and library. */

typedef UINT16          HANDLE16;
typedef UINT32          HANDLE32;
typedef UINT16          WPARAM16;
typedef UINT32          WPARAM32;
typedef LONG            LPARAM;
typedef LONG            HRESULT;
typedef LONG            LRESULT;
typedef WORD            ATOM;
typedef WORD            CATCHBUF[9];
typedef WORD           *LPCATCHBUF;
typedef DWORD           ACCESS_MASK;
typedef ACCESS_MASK     REGSAM;
typedef HANDLE32        HHOOK;
typedef HANDLE32        HKEY;
typedef HANDLE32        HMONITOR;
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
typedef const CHAR     *LPCSTR;
typedef WCHAR          *LPWSTR;
typedef const WCHAR    *LPCWSTR;
typedef BYTE           *LPBYTE;
typedef WORD           *LPWORD;
typedef DWORD          *LPDWORD;
typedef LONG           *LPLONG;
typedef VOID           *LPVOID;
typedef const VOID     *LPCVOID;
typedef INT16          *LPINT16;
typedef UINT16         *LPUINT16;
typedef INT32          *PINT32;
typedef INT32          *LPINT32;
typedef UINT32         *PUINT32;
typedef UINT32         *LPUINT32;
typedef HKEY           *LPHKEY;
typedef FLOAT          *PFLOAT;
typedef FLOAT          *LPFLOAT;
typedef BOOL32         *PBOOL32;
typedef BOOL32         *LPBOOL32;

/* Special case: a segmented pointer is just a pointer in the user's code. */

#ifdef __WINE__
typedef DWORD SEGPTR;
#else
typedef void* SEGPTR;
#endif /* __WINE__ */

/* Handle types that exist both in Win16 and Win32. */

#define DECLARE_HANDLE(a)  typedef HANDLE16 a##16; typedef HANDLE32 a##32
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

typedef BOOL32 (CALLBACK* DATEFMT_ENUMPROC32A)(LPSTR);
typedef BOOL32 (CALLBACK* DATEFMT_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(DATEFMT_ENUMPROC)
typedef BOOL16 (CALLBACK *DLGPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef BOOL32 (CALLBACK *DLGPROC32)(HWND32,UINT32,WPARAM32,LPARAM);
DECL_WINELIB_TYPE(DLGPROC)
typedef LRESULT (CALLBACK *DRIVERPROC16)(DWORD,HDRVR16,UINT16,LPARAM,LPARAM);
typedef LRESULT (CALLBACK *DRIVERPROC32)(DWORD,HDRVR32,UINT32,LPARAM,LPARAM);
DECL_WINELIB_TYPE(DRIVERPROC)
typedef INT16   (CALLBACK *EDITWORDBREAKPROC16)(LPSTR,INT16,INT16,INT16);
typedef INT32   (CALLBACK *EDITWORDBREAKPROC32A)(LPSTR,INT32,INT32,INT32);
typedef INT32   (CALLBACK *EDITWORDBREAKPROC32W)(LPWSTR,INT32,INT32,INT32);
DECL_WINELIB_TYPE_AW(EDITWORDBREAKPROC)
typedef LRESULT (CALLBACK *FARPROC16)();
typedef LRESULT (CALLBACK *FARPROC32)();
DECL_WINELIB_TYPE(FARPROC)
typedef INT16   (CALLBACK *GOBJENUMPROC16)(SEGPTR,LPARAM);
typedef INT32   (CALLBACK *GOBJENUMPROC32)(LPVOID,LPARAM);
DECL_WINELIB_TYPE(GOBJENUMPROC)
typedef BOOL16  (CALLBACK *GRAYSTRINGPROC16)(HDC16,LPARAM,INT16);
typedef BOOL32  (CALLBACK *GRAYSTRINGPROC32)(HDC32,LPARAM,INT32);
DECL_WINELIB_TYPE(GRAYSTRINGPROC)
typedef LRESULT (CALLBACK *HOOKPROC16)(INT16,WPARAM16,LPARAM);
typedef LRESULT (CALLBACK *HOOKPROC32)(INT32,WPARAM32,LPARAM);
DECL_WINELIB_TYPE(HOOKPROC)
typedef VOID    (CALLBACK *LINEDDAPROC16)(INT16,INT16,LPARAM);
typedef VOID    (CALLBACK *LINEDDAPROC32)(INT32,INT32,LPARAM);
DECL_WINELIB_TYPE(LINEDDAPROC)
typedef BOOL16  (CALLBACK *PROPENUMPROC16)(HWND16,SEGPTR,HANDLE16);
typedef BOOL32  (CALLBACK *PROPENUMPROC32A)(HWND32,LPCSTR,HANDLE32);
typedef BOOL32  (CALLBACK *PROPENUMPROC32W)(HWND32,LPCWSTR,HANDLE32);
DECL_WINELIB_TYPE_AW(PROPENUMPROC)
typedef BOOL32  (CALLBACK *PROPENUMPROCEX32A)(HWND32,LPCSTR,HANDLE32,LPARAM);
typedef BOOL32  (CALLBACK *PROPENUMPROCEX32W)(HWND32,LPCWSTR,HANDLE32,LPARAM);
DECL_WINELIB_TYPE_AW(PROPENUMPROCEX)
typedef BOOL32 (CALLBACK* TIMEFMT_ENUMPROC32A)(LPSTR);
typedef BOOL32 (CALLBACK* TIMEFMT_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(TIMEFMT_ENUMPROC)
typedef VOID    (CALLBACK *TIMERPROC16)(HWND16,UINT16,UINT16,DWORD);
typedef VOID    (CALLBACK *TIMERPROC32)(HWND32,UINT32,UINT32,DWORD);
DECL_WINELIB_TYPE(TIMERPROC)
typedef LRESULT (CALLBACK *WNDENUMPROC16)(HWND16,LPARAM);
typedef LRESULT (CALLBACK *WNDENUMPROC32)(HWND32,LPARAM);
DECL_WINELIB_TYPE(WNDENUMPROC)
typedef LRESULT (CALLBACK *WNDPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef LRESULT (CALLBACK *WNDPROC32)(HWND32,UINT32,WPARAM32,LPARAM);
DECL_WINELIB_TYPE(WNDPROC)

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

DECL_WINELIB_TYPE(INT)
DECL_WINELIB_TYPE(LPINT)
DECL_WINELIB_TYPE(PUINT)
DECL_WINELIB_TYPE(LPUINT)
DECL_WINELIB_TYPE(UINT)
DECL_WINELIB_TYPE(BOOL)
DECL_WINELIB_TYPE(WPARAM)

DECL_WINELIB_TYPE(HACCEL)
DECL_WINELIB_TYPE(HANDLE)
DECL_WINELIB_TYPE(HBITMAP)
DECL_WINELIB_TYPE(HBRUSH)
DECL_WINELIB_TYPE(HCOLORSPACE)
DECL_WINELIB_TYPE(HCURSOR)
DECL_WINELIB_TYPE(HDC)
DECL_WINELIB_TYPE(HDROP)
DECL_WINELIB_TYPE(HDRVR)
DECL_WINELIB_TYPE(HDWP)
DECL_WINELIB_TYPE(HENHMETAFILE)
DECL_WINELIB_TYPE(HFILE)
DECL_WINELIB_TYPE(HFONT)
DECL_WINELIB_TYPE(HGDIOBJ)
DECL_WINELIB_TYPE(HGLOBAL)
DECL_WINELIB_TYPE(HICON)
DECL_WINELIB_TYPE(HINSTANCE)
DECL_WINELIB_TYPE(HLOCAL)
DECL_WINELIB_TYPE(HMENU)
DECL_WINELIB_TYPE(HMETAFILE)
DECL_WINELIB_TYPE(HMIDI)
DECL_WINELIB_TYPE(HMIDIIN)
DECL_WINELIB_TYPE(HMIDIOUT)
DECL_WINELIB_TYPE(HMMIO)
DECL_WINELIB_TYPE(HMODULE)
DECL_WINELIB_TYPE(HPALETTE)
DECL_WINELIB_TYPE(HPEN)
DECL_WINELIB_TYPE(HQUEUE)
DECL_WINELIB_TYPE(HRGN)
DECL_WINELIB_TYPE(HRSRC)
DECL_WINELIB_TYPE(HTASK)
DECL_WINELIB_TYPE(HWAVE)
DECL_WINELIB_TYPE(HWAVEIN)
DECL_WINELIB_TYPE(HWAVEOUT)
DECL_WINELIB_TYPE(HWND)

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
#else
#define WINE_PACKED  /* nothing */
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
#define MAKEWPARAM(low,high)   ((WPARAM32)MAKELONG(low,high))
#define MAKELRESULT(low,high)  ((LRESULT)MAKELONG(low,high))
#define MAKEINTATOM(atom)      ((LPCSTR)MAKELONG((atom),0))

#define SELECTOROF(ptr)     (HIWORD(ptr))
#define OFFSETOF(ptr)       (LOWORD(ptr))

/* Macros to access unaligned or wrong-endian WORDs and DWORDs. */

#ifdef __i386__
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
#endif  /* __i386__ */

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
#define _MAX_FNAME 255

/* Winelib run-time flag */

#ifdef __WINE__
extern int __winelib;
#endif  /* __WINE__ */

/* The SIZE structure */

typedef struct
{
    INT16  cx;
    INT16  cy;
} SIZE16, *LPSIZE16;

typedef struct tagSIZE
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

typedef struct tagPOINT
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

typedef struct tagRECT
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

#ifdef __cplusplus
}
#endif

#endif /* __WINE_WINTYPES_H */
