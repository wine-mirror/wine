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
    HBRUSH32   hbrushPattern;
    HBITMAP32  hbitmapWallPaper;
    SIZE32     bitmapSize;
    BOOL32     fTileWallPaper;
} DESKTOPINFO;

extern BOOL32 DESKTOP_SetPattern( LPCSTR pattern );
extern LRESULT DesktopWndProc( HWND32 hwnd, UINT32 message,
                               WPARAM32 wParam, LPARAM lParam );

#endif  /* DESKTOP_H */
