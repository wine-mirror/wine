/*
 * Desktop window definitions.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef DESKTOP_H
#define DESKTOP_H

#include "windows.h"

typedef struct
{
    HBRUSH   hbrushPattern;
    HBITMAP  hbitmapWallPaper;
    SIZE16   bitmapSize;
    BOOL     fTileWallPaper;
} DESKTOPINFO;

extern BOOL DESKTOP_Init();
extern BOOL DESKTOP_SetPattern(char *pattern );

extern LRESULT DesktopWndProc ( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam );

#endif  /* DESKTOP_H */
