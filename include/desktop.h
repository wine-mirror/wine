/*
 * Desktop window definitions.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_DESKTOP_H
#define __WINE_DESKTOP_H

#include "windef.h"

typedef struct tagDESKTOP
{
  HBRUSH                hbrushPattern;
  HBITMAP               hbitmapWallPaper;
  SIZE                  bitmapSize;
  BOOL                  fTileWallPaper;
} DESKTOP;

extern BOOL DESKTOP_SetPattern( LPCSTR pattern );
extern LRESULT WINAPI DesktopWndProc( HWND hwnd, UINT message,
                                      WPARAM wParam, LPARAM lParam );

#endif  /* __WINE_DESKTOP_H */
