/*
 * Static-class extra info
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef STATIC_H
#define STATIC_H

#include "windows.h"

  /* Extra info for STATIC windows */
typedef struct
{
    HFONT  hFont;   /* Control font (or 0 for system font) */
    WORD   dummy;   /* Don't know what MS-Windows puts in there */
    HICON  hIcon;   /* Icon handle for SS_ICON controls */ 
} STATICINFO;

extern LONG StaticWndProc( HWND hWnd, WORD uMsg, WORD wParam, LONG lParam );

#endif  /* STATIC_H */
