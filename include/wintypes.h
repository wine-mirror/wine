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
typedef char            CHAR;
/* Some systems might have wchar_t, but we really need 16 bit characters */
typedef unsigned short  WCHAR;
typedef unsigned short  BOOL16;
typedef int             BOOL32;

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

/* Pointers types. These are the same for emulator and library. */

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
typedef INT32          *LPINT32;
typedef UINT32         *LPUINT32;
typedef HKEY           *LPHKEY;
typedef FLOAT          *LPFLOAT;

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
DECLARE_HANDLE(HWND);
DECLARE_HANDLE(HKL);
DECLARE_HANDLE(HIC);
#undef DECLARE_HANDLE

/* Callback function pointers types */

typedef BOOL32 (CALLBACK* DATEFMT_ENUMPROC32A)(LPSTR);
typedef BOOL32 (CALLBACK* DATEFMT_ENUMPROC32W)(LPWSTR);
DECL_WINELIB_TYPE_AW(DATEFMT_ENUMPROC)
typedef LRESULT (CALLBACK *DLGPROC16)(HWND16,UINT16,WPARAM16,LPARAM);
typedef LRESULT (CALLBACK *DLGPROC32)(HWND32,UINT32,WPARAM32,LPARAM);
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
# else  /* UNICODE */
typedef CHAR TCHAR;
typedef LPSTR LPTSTR;
typedef LPCSTR LPCTSTR;
# endif /* UNICODE */
#endif   /* __WINE__ */

/* Data types specific to the library. These do _not_ exist in the emulator. */

DECL_WINELIB_TYPE(INT)
DECL_WINELIB_TYPE(LPINT)
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

#define MAKELONG(low,high)     ((LONG)(((WORD)(low)) | \
                                       (((DWORD)((WORD)(high))) << 16)))
#define MAKELPARAM(low,high)   ((LPARAM)MAKELONG(low,high))
#define MAKEWPARAM(low,high)   ((WPARAM32)MAKELONG(low,high))
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

/* Winelib run-time flag */

#ifdef __WINE__
extern int __winelib;
#endif  /* __WINE__ */

#endif /* __WINE_WINTYPES_H */
