/*
 * USER definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINUSER_H
#define __WINE_WINUSER_H

#include "wintypes.h"


/* Window classes */

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

DECL_WINELIB_TYPE_AW(WNDCLASS);
DECL_WINELIB_TYPE_AW(LPWNDCLASS);
DECL_WINELIB_TYPE_AW(WNDCLASSEX);
DECL_WINELIB_TYPE_AW(LPWNDCLASSEX);

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

#endif  /* __WINE_WINUSER_H */
