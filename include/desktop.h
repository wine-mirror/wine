/*
 * Desktop window definitions.
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_DESKTOP_H
#define __WINE_DESKTOP_H

#include "wintypes.h"

struct tagMONITOR;

struct _DESKTOP_DRIVER;

typedef struct tagDESKTOP
{
  HBRUSH32                hbrushPattern;
  HBITMAP32               hbitmapWallPaper;
  SIZE32                  bitmapSize;
  BOOL32                  fTileWallPaper;
  struct tagMONITOR      *pPrimaryMonitor;
  struct _DESKTOP_DRIVER *pDriver;         /* Desktop driver */
  void                   *pDriverData;     /* Desktop driver data */
} DESKTOP;

typedef struct _DESKTOP_DRIVER {
  void (*pInitialize)(struct tagDESKTOP *pDesktop);
  void (*pFinalize)(struct tagDESKTOP *pDesktop);
} DESKTOP_DRIVER;

extern int DESKTOP_GetScreenWidth(void);
extern int DESKTOP_GetScreenHeight(void);
extern int DESKTOP_GetScreenDepth(void);

extern BOOL32 DESKTOP_SetPattern( LPCSTR pattern );
extern LRESULT WINAPI DesktopWndProc( HWND32 hwnd, UINT32 message,
                                      WPARAM32 wParam, LPARAM lParam );

#endif  /* __WINE_DESKTOP_H */
