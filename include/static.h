/*
 * Static-class extra info
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_STATIC_H
#define __WINE_STATIC_H

#include "wintypes.h"

  /* Extra info for STATIC windows */
typedef struct
{
    HFONT16  hFont;   /* Control font (or 0 for system font) */
    WORD     dummy;   /* Don't know what MS-Windows puts in there */
    HICON16  hIcon;   /* Icon handle for SS_ICON controls */ 
} STATICINFO;

extern LRESULT WINAPI StaticWndProc( HWND hWnd, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam );

#endif  /* __WINE_STATIC_H */
