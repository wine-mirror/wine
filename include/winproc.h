/*
 * Window procedure callbacks definitions
 *
 * Copyright 1996 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINPROC_H
#define __WINE_WINPROC_H

#include "windef.h"
#include "wine/winbase16.h"
#include "winnls.h"

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

extern INT WINPROC_MapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM *pwparam,
                                     LPARAM *plparam );
extern INT WINPROC_MapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM *pwparam,
                                     LPARAM *plparam );
extern INT WINPROC_MapMsg16To32A( HWND hwnd, UINT16 msg16, WPARAM16 wParam16,
                                    UINT *pmsg32, WPARAM *pwparam32,
                                    LPARAM *plparam );
extern INT WINPROC_MapMsg16To32W( HWND hwnd, UINT16 msg16, WPARAM16 wParam16,
                                    UINT *pmsg32, WPARAM *pwparam32,
                                    LPARAM *plparam );
extern INT WINPROC_MapMsg32ATo16( HWND hwnd, UINT msg32,
                                    WPARAM wParam32, UINT16 *pmsg16,
                                    WPARAM16 *pwparam16, LPARAM *plparam );
extern INT WINPROC_MapMsg32WTo16( HWND hwnd, UINT msg32,
                                    WPARAM wParam32, UINT16 *pmsg16,
                                    WPARAM16 *pwparam16, LPARAM *plparam );
extern LRESULT WINPROC_UnmapMsg32ATo32W( HWND hwnd, UINT msg, WPARAM wParam,
                                         LPARAM lParam, LRESULT result );
extern void WINPROC_UnmapMsg32WTo32A( HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam );
extern LRESULT WINPROC_UnmapMsg16To32A( HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam, LRESULT result );
extern LRESULT WINPROC_UnmapMsg16To32W( HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam, LRESULT result );
extern void WINPROC_UnmapMsg32ATo16( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, MSGPARAM16* pm16 );
extern void WINPROC_UnmapMsg32WTo16( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, MSGPARAM16* pm16 );

/* map a Unicode string to a 16-bit pointer */
inline static SEGPTR map_str_32W_to_16( LPCWSTR str )
{
    LPSTR ret;
    INT len;

    if (!HIWORD(str)) return (SEGPTR)LOWORD(str);
    len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, len )))
        WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL );
    return MapLS(ret);
}

/* unmap a Unicode string that was converted to a 16-bit pointer */
inline static void unmap_str_32W_to_16( SEGPTR str )
{
    if (!HIWORD(str)) return;
    HeapFree( GetProcessHeap(), 0, MapSL(str) );
    UnMapLS( str );
}

/* map a 16-bit pointer to a Unicode string */
inline static LPWSTR map_str_16_to_32W( SEGPTR str )
{
    LPWSTR ret;
    INT len;

    if (!HIWORD(str)) return (LPWSTR)(ULONG_PTR)LOWORD(str);
    len = MultiByteToWideChar( CP_ACP, 0, MapSL(str), -1, NULL, 0 );
    if ((ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        MultiByteToWideChar( CP_ACP, 0, MapSL(str), -1, ret, len );
    return ret;
}

/* unmap a 16-bit pointer that was converted to a Unicode string */
inline static void unmap_str_16_to_32W( LPCWSTR str )
{
    if (HIWORD(str)) HeapFree( GetProcessHeap(), 0, (void *)str );
}

#endif  /* __WINE_WINPROC_H */
