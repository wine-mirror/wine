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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "winnls.h"

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

struct tagWINDOWPROC;

typedef LRESULT (*winproc_callback_t)( HWND hwnd, UINT msg, WPARAM wp, LPARAM lp,
                                       LRESULT *result, void *arg );
typedef LRESULT (*winproc_callback16_t)( HWND16 hwnd, UINT16 msg, WPARAM16 wp, LPARAM lp,
                                         LRESULT *result, void *arg );

extern WNDPROC16 WINPROC_GetProc16( WNDPROC proc, BOOL unicode );
extern WNDPROC WINPROC_AllocProc16( WNDPROC16 func );
extern WNDPROC WINPROC_GetProc( WNDPROC proc, BOOL unicode );
extern WNDPROC WINPROC_AllocProc( WNDPROC funcA, WNDPROC funcW );
extern BOOL WINPROC_IsUnicode( WNDPROC proc, BOOL def_val );

extern LRESULT WINPROC_CallProcAtoW( winproc_callback_t callback, HWND hwnd, UINT msg,
                                     WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg );
extern LRESULT WINPROC_CallProc16To32A( winproc_callback_t callback, HWND16 hwnd, UINT16 msg,
                                        WPARAM16 wParam, LPARAM lParam, LRESULT *result, void *arg );
extern LRESULT WINPROC_CallProc32ATo16( winproc_callback16_t callback, HWND hwnd, UINT msg,
                                        WPARAM wParam, LPARAM lParam, LRESULT *result, void *arg );

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
extern LRESULT WINPROC_UnmapMsg16To32A( HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam, LRESULT result );
extern LRESULT WINPROC_UnmapMsg16To32W( HWND hwnd, UINT msg, WPARAM wParam,
                                        LPARAM lParam, LRESULT result,
                                        WNDPROC dispatch );
extern void WINPROC_UnmapMsg32ATo16( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, MSGPARAM16* pm16 );
extern void WINPROC_UnmapMsg32WTo16( HWND hwnd, UINT msg, WPARAM wParam,
                                     LPARAM lParam, MSGPARAM16* pm16 );

extern INT_PTR WINPROC_CallDlgProc16( DLGPROC16 func, HWND16 hwnd, UINT16 msg, WPARAM16 wParam, LPARAM lParam );
extern INT_PTR WINPROC_CallDlgProcA( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );
extern INT_PTR WINPROC_CallDlgProcW( DLGPROC func, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam );

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


/* Class functions */
struct tagCLASS;  /* opaque structure */
struct tagWND;
extern void CLASS_RegisterBuiltinClasses(void);
extern void CLASS_AddWindow( struct tagCLASS *class, struct tagWND *win, BOOL unicode );
extern void CLASS_FreeModuleClasses( HMODULE16 hModule );

#endif  /* __WINE_WINPROC_H */
