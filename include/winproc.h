/*
 * Window procedure callbacks definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINPROC_H
#define __WINE_WINPROC_H

#include "wintypes.h"

typedef enum
{
    WIN_PROC_INVALID,
    WIN_PROC_16,
    WIN_PROC_32A,
    WIN_PROC_32W
} WINDOWPROCTYPE;

typedef void *HWINDOWPROC;  /* Really a pointer to a WINDOWPROC */

extern BOOL32 WINPROC_Init(void);
extern WNDPROC16 WINPROC_GetProc( HWINDOWPROC proc, WINDOWPROCTYPE type );
extern BOOL32 WINPROC_SetProc( HWINDOWPROC *pFirst, WNDPROC16 func,
                               WINDOWPROCTYPE type );
extern void WINPROC_FreeProc( HWINDOWPROC proc );
extern WINDOWPROCTYPE WINPROC_GetProcType( HWINDOWPROC proc );

extern INT32 WINPROC_MapMsg32ATo32W( UINT32 msg, WPARAM32 wParam,
                                     LPARAM *plparam );
extern INT32 WINPROC_MapMsg32WTo32A( UINT32 msg, WPARAM32 wParam,
                                     LPARAM *plparam );
extern INT32 WINPROC_MapMsg16To32A( UINT16 msg16, WPARAM16 wParam16,
                                    UINT32 *pmsg32, WPARAM32 *pwparam32,
                                    LPARAM *plparam );
extern INT32 WINPROC_MapMsg16To32W( UINT16 msg16, WPARAM16 wParam16,
                                    UINT32 *pmsg32, WPARAM32 *pwparam32,
                                    LPARAM *plparam );
extern INT32 WINPROC_MapMsg32ATo16( UINT32 msg32, WPARAM32 wParam32,
                                    UINT16 *pmsg16, WPARAM16 *pwparam16,
                                    LPARAM *plparam );
extern INT32 WINPROC_MapMsg32WTo16( UINT32 msg32, WPARAM32 wParam32,
                                    UINT16 *pmsg16, WPARAM16 *pwparam16,
                                    LPARAM *plparam );
extern void WINPROC_UnmapMsg32ATo32W( UINT32 msg, WPARAM32 wParam,
                                      LPARAM lParam );
extern void WINPROC_UnmapMsg32WTo32A( UINT32 msg, WPARAM32 wParam,
                                      LPARAM lParam );
extern void WINPROC_UnmapMsg16To32A( UINT32 msg, WPARAM32 wParam,
                                     LPARAM lParam );
extern void WINPROC_UnmapMsg16To32W( UINT32 msg, WPARAM32 wParam,
                                     LPARAM lParam );
extern void WINPROC_UnmapMsg32ATo16( UINT32 msg, WPARAM16 wParam,
                                     LPARAM lParam );
extern void WINPROC_UnmapMsg32WTo16( UINT32 msg, WPARAM16 wParam,
                                     LPARAM lParam );

#endif  /* __WINE_WINPROC_H */
