/*
 * Window procedure callbacks definitions
 *
 * Copyright 1996 Alexandre Julliard
 */

#ifndef __WINE_WINPROC_H
#define __WINE_WINPROC_H

#include "windef.h"

typedef enum
{
    WIN_PROC_INVALID,
    WIN_PROC_16,
    WIN_PROC_32A,
    WIN_PROC_32W
} WINDOWPROCTYPE;

typedef enum
{
    WIN_PROC_CLASS,
    WIN_PROC_WINDOW,
    WIN_PROC_TIMER
} WINDOWPROCUSER;

typedef void *HWINDOWPROC;  /* Really a pointer to a WINDOWPROC */

typedef struct
{
    WPARAM16	wParam;
    LPARAM	lParam;
    LRESULT	lResult;
} MSGPARAM16;

typedef struct
{
    WPARAM    wParam;
    LPARAM	lParam;
    LRESULT	lResult;
} MSGPARAM;

extern BOOL WINPROC_Init(void);
extern WNDPROC16 WINPROC_GetProc( HWINDOWPROC proc, WINDOWPROCTYPE type );
extern BOOL WINPROC_SetProc( HWINDOWPROC *pFirst, WNDPROC16 func,
                               WINDOWPROCTYPE type, WINDOWPROCUSER user );
extern void WINPROC_FreeProc( HWINDOWPROC proc, WINDOWPROCUSER user );
extern WINDOWPROCTYPE WINPROC_GetProcType( HWINDOWPROC proc );

extern INT WINPROC_MapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM *plparam );
extern INT WINPROC_MapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM *plparam );
extern INT WINPROC_MapMsg16To32A( UINT16 msg16, WPARAM16 wParam16,
                                    UINT *pmsg32, WPARAM *pwparam32,
                                    LPARAM *plparam );
extern INT WINPROC_MapMsg16To32W( HWND16, UINT16 msg16, WPARAM16 wParam16,
                                    UINT *pmsg32, WPARAM *pwparam32,
                                    LPARAM *plparam );
extern INT WINPROC_MapMsg32ATo16( HWND hwnd, UINT msg32,
                                    WPARAM wParam32, UINT16 *pmsg16,
                                    WPARAM16 *pwparam16, LPARAM *plparam );
extern INT WINPROC_MapMsg32WTo16( HWND hwnd, UINT msg32,
                                    WPARAM wParam32, UINT16 *pmsg16,
                                    WPARAM16 *pwparam16, LPARAM *plparam );
extern void WINPROC_UnmapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam );
extern void WINPROC_UnmapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam );
extern LRESULT WINPROC_UnmapMsg16To32A( HWND16 hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam, LRESULT result );
extern LRESULT WINPROC_UnmapMsg16To32W( HWND16 hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam, LRESULT result );
extern void WINPROC_UnmapMsg32ATo16( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, MSGPARAM16* pm16 );
extern void WINPROC_UnmapMsg32WTo16( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, MSGPARAM16* pm16 );
#endif  /* __WINE_WINPROC_H */
