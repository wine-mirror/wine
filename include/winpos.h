/*
 * *DeferWindowPos() structure and definitions
 *
 * Copyright 1994 Alexandre Julliard
 */

#ifndef __WINE_WINPOS_H
#define __WINE_WINPOS_H

#include "windef.h"
#include "wingdi.h"
#include "winuser.h"

struct tagWND;

#define DWP_MAGIC  ((INT)('W' | ('P' << 8) | ('O' << 16) | ('S' << 24)))

/* undocumented SWP flags - from SDK 3.1 */
#define SWP_NOCLIENTSIZE	0x0800
#define SWP_NOCLIENTMOVE	0x1000

/* Wine extra SWP flag */
#define SWP_WINE_NOHOSTMOVE	0x80000000

struct tagWINDOWPOS16;

typedef struct
{
    INT       actualCount;
    INT       suggestedCount;
    BOOL      valid;
    INT       wMagic;
    HWND      hwndParent;
    WINDOWPOS winPos[1];
} DWP;

extern BOOL WINPOS_RedrawIconTitle( HWND hWnd );
extern BOOL WINPOS_ShowIconTitle( struct tagWND* pWnd, BOOL bShow );
extern void   WINPOS_GetMinMaxInfo( struct tagWND* pWnd, POINT *maxSize,
                                    POINT *maxPos, POINT *minTrack,
                                    POINT *maxTrack );
extern UINT WINPOS_MinMaximize( struct tagWND* pWnd, UINT16 cmd, LPRECT16 lpPos);
extern BOOL WINPOS_SetActiveWindow( HWND hWnd, BOOL fMouse,
                                      BOOL fChangeFocus );
extern BOOL WINPOS_ChangeActiveWindow( HWND hwnd, BOOL mouseMsg );
extern LONG WINPOS_SendNCCalcSize(HWND hwnd, BOOL calcValidRect,
                                  RECT *newWindowRect, RECT *oldWindowRect,
                                  RECT *oldClientRect, WINDOWPOS *winpos,
                                  RECT *newClientRect );
extern LONG WINPOS_HandleWindowPosChanging16(struct tagWND *wndPtr, struct tagWINDOWPOS16 *winpos);
extern LONG WINPOS_HandleWindowPosChanging(struct tagWND *wndPtr, WINDOWPOS *winpos);
extern INT16 WINPOS_WindowFromPoint( struct tagWND* scopeWnd, POINT16 pt, struct tagWND **ppWnd );
extern void WINPOS_CheckInternalPos( struct tagWND* wndPtr );
extern BOOL WINPOS_ActivateOtherWindow(struct tagWND* pWnd);
extern BOOL WINPOS_CreateInternalPosAtom(void);

#endif  /* __WINE_WINPOS_H */
