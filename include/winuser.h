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
    WNDPROC     lpfnWndProc WINE_PACKED;
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

typedef void WNDCLASSEX16;  /* There's no WNDCLASSEX in Win16 */

DECL_WINELIB_TYPE_AW(WNDCLASS);
DECL_WINELIB_TYPE_AW(LPWNDCLASS);
DECL_WINELIB_TYPE_AW(WNDCLASSEX);
DECL_WINELIB_TYPE_AW(LPWNDCLASSEX);


#endif  /* __WINE_WINUSER_H */
