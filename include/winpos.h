/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_WINPOS_H
#define __WINE_WINPOS_H

#include "win.h"
#include "wine/winuser16.h" /* for WINDOWPOS16 */

#define DWP_MAGIC  ((INT32)('W' | ('P' << 8) | ('O' << 16) | ('S' << 24)))

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

typedef struct
{
    INT32       actualCount;
    INT32       suggestedCount;
    BOOL32      valid;
    INT32       wMagic;
    HWND32      hwndParent;
    WINDOWPOS32 winPos[1];
} DWP;

extern BOOL32 WINPOS_RedrawIconTitle( HWND32 hWnd );
extern BOOL32 WINPOS_ShowIconTitle( WND* pWnd, BOOL32 bShow );
extern void   WINPOS_GetMinMaxInfo( WND* pWnd, POINT32 *maxSize,
                                    POINT32 *maxPos, POINT32 *minTrack,
                                    POINT32 *maxTrack );
extern UINT16 WINPOS_MinMaximize( WND* pWnd, UINT16 cmd, LPRECT16 lpPos);
extern BOOL32 WINPOS_SetActiveWindow( HWND32 hWnd, BOOL32 fMouse,
                                      BOOL32 fChangeFocus );
extern BOOL32 WINPOS_ChangeActiveWindow( HWND32 hwnd, BOOL32 mouseMsg );
extern LONG WINPOS_SendNCCalcSize(HWND32 hwnd, BOOL32 calcValidRect,
                                  RECT32 *newWindowRect, RECT32 *oldWindowRect,
                                  RECT32 *oldClientRect, WINDOWPOS32 *winpos,
                                  RECT32 *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging16(WND *wndPtr, WINDOWPOS16 *winpos);
extern LONG WINPOS_HandleWindowPosChanging32(WND *wndPtr, WINDOWPOS32 *winpos);
extern INT16 WINPOS_WindowFromPoint( WND* scopeWnd, POINT16 pt, WND **ppWnd );
extern void WINPOS_CheckInternalPos( WND* wndPtr );
extern BOOL32 WINPOS_ActivateOtherWindow(WND* pWnd);
extern BOOL32 WINPOS_CreateInternalPosAtom(void);

#endif  /* __WINE_WINPOS_H */
