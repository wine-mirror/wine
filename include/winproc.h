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

extern HANDLE32 WINPROC_AllocWinProc( UINT32 func, WINDOWPROCTYPE type );
extern HANDLE32 WINPROC_CopyWinProc( HANDLE32 handle );
extern void WINPROC_FreeWinProc( HANDLE32 handle );
extern WINDOWPROCTYPE WINPROC_GetWinProcType( HANDLE32 handle );
extern WNDPROC16 WINPROC_GetFunc16( HANDLE32 handle );
extern WNDPROC32 WINPROC_GetFunc32( HANDLE32 handle );

#endif  /* __WINE_WINPROC_H */
