/*
 * Desktop window definitions.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_DESKTOP_H
#define __WINE_DESKTOP_H

#include "windef.h"

struct tagMONITOR;

typedef struct tagDESKTOP
{
  HBRUSH                hbrushPattern;
  HBITMAP               hbitmapWallPaper;
  SIZE                  bitmapSize;
  BOOL                  fTileWallPaper;
  struct tagMONITOR    *pPrimaryMonitor;
} DESKTOP;

extern BOOL DESKTOP_IsSingleWindow(void);
extern int DESKTOP_GetScreenWidth(void);
extern int DESKTOP_GetScreenHeight(void);
extern int DESKTOP_GetScreenDepth(void);

extern BOOL DESKTOP_SetPattern( LPCSTR pattern );
extern LRESULT WINAPI DesktopWndProc( HWND hwnd, UINT message,
                                      WPARAM wParam, LPARAM lParam );

#endif  /* __WINE_DESKTOP_H */
