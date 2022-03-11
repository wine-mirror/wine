/*
 * USER driver support
 *
 * Copyright 2000, 2005 Alexandre Julliard
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

#include "user_private.h"
#include "winnls.h"
#include "wine/debug.h"
#include "controls.h"

WINE_DEFAULT_DEBUG_CHANNEL(user);

static struct user_driver_funcs null_driver, lazy_load_driver;

const struct user_driver_funcs *USER_Driver = &lazy_load_driver;

/* load the graphics driver */
static const struct user_driver_funcs *load_driver(void)
{
    wait_graphics_driver_ready();
    if (USER_Driver == &lazy_load_driver)
    {
        static struct user_driver_funcs empty_funcs;
        WARN( "failed to load the display driver, falling back to null driver\n" );
        __wine_set_user_driver( &empty_funcs, WINE_GDI_DRIVER_VERSION );
    }

    return USER_Driver;
}

/* unload the graphics driver on process exit */
void USER_unload_driver(void)
{
    struct user_driver_funcs *prev;
    __wine_set_display_driver( &null_driver, WINE_GDI_DRIVER_VERSION );
    /* make sure we don't try to call the driver after it has been detached */
    prev = InterlockedExchangePointer( (void **)&USER_Driver, &null_driver );
    if (prev != &lazy_load_driver && prev != &null_driver)
        HeapFree( GetProcessHeap(), 0, prev );
}


/**********************************************************************
 * Null user driver
 *
 * These are fallbacks for entry points that are not implemented in the real driver.
 */

static BOOL CDECL nulldrv_SetCursorPos( INT x, INT y )
{
    return TRUE;
}

static void CDECL nulldrv_UpdateClipboard(void)
{
}

static BOOL CDECL nulldrv_CreateWindow( HWND hwnd )
{
    return TRUE;
}

static void CDECL nulldrv_DestroyWindow( HWND hwnd )
{
}

static void CDECL nulldrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                                 const RECT *top_rect, DWORD flags )
{
}

static DWORD CDECL nulldrv_MsgWaitForMultipleObjectsEx( DWORD count, const HANDLE *handles, DWORD timeout,
                                                        DWORD mask, DWORD flags )
{
    if (!count && !timeout) return WAIT_TIMEOUT;
    return WaitForMultipleObjectsEx( count, handles, flags & MWMO_WAITALL,
                                     timeout, flags & MWMO_ALERTABLE );
}

static void CDECL nulldrv_ReleaseDC( HWND hwnd, HDC hdc )
{
}

static void CDECL nulldrv_SetParent( HWND hwnd, HWND parent, HWND old_parent )
{
}

static void CDECL nulldrv_SetWindowIcon( HWND hwnd, UINT type, HICON icon )
{
}

static void CDECL nulldrv_SetWindowStyle( HWND hwnd, INT offset, STYLESTRUCT *style )
{
}

static void CDECL nulldrv_SetWindowText( HWND hwnd, LPCWSTR text )
{
}

static UINT CDECL nulldrv_ShowWindow( HWND hwnd, INT cmd, RECT *rect, UINT swp )
{
    return ~0; /* use default implementation */
}

static LRESULT CDECL nulldrv_SysCommand( HWND hwnd, WPARAM wparam, LPARAM lparam )
{
    return -1;
}

static BOOL CDECL nulldrv_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                             const RECT *window_rect, const RECT *client_rect,
                                             RECT *visible_rect, struct window_surface **surface )
{
    return FALSE;
}

static void CDECL nulldrv_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                            const RECT *window_rect, const RECT *client_rect,
                                            const RECT *visible_rect, const RECT *valid_rects,
                                            struct window_surface *surface )
{
}


/**********************************************************************
 * Lazy loading user driver
 *
 * Initial driver used before another driver is loaded.
 * Each entry point simply loads the real driver and chains to it.
 */

static BOOL CDECL loaderdrv_SetCursorPos( INT x, INT y )
{
    return load_driver()->pSetCursorPos( x, y );
}

static void CDECL loaderdrv_UpdateClipboard(void)
{
    load_driver()->pUpdateClipboard();
}

static BOOL CDECL loaderdrv_CreateWindow( HWND hwnd )
{
    return load_driver()->pCreateWindow( hwnd );
}

static void CDECL loaderdrv_GetDC( HDC hdc, HWND hwnd, HWND top_win, const RECT *win_rect,
                                   const RECT *top_rect, DWORD flags )
{
    load_driver()->pGetDC( hdc, hwnd, top_win, win_rect, top_rect, flags );
}

static struct user_driver_funcs lazy_load_driver =
{
    { NULL },
    /* keyboard functions */
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    /* cursor/icon functions */
    NULL,
    NULL,
    NULL,
    loaderdrv_SetCursorPos,
    NULL,
    /* clipboard functions */
    loaderdrv_UpdateClipboard,
    /* display modes */
    NULL,
    NULL,
    NULL,
    /* windowing functions */
    NULL,
    loaderdrv_CreateWindow,
    nulldrv_DestroyWindow,
    NULL,
    loaderdrv_GetDC,
    nulldrv_MsgWaitForMultipleObjectsEx,
    nulldrv_ReleaseDC,
    NULL,
    NULL,
    NULL,
    NULL,
    nulldrv_SetParent,
    NULL,
    nulldrv_SetWindowIcon,
    nulldrv_SetWindowStyle,
    nulldrv_SetWindowText,
    nulldrv_ShowWindow,
    nulldrv_SysCommand,
    NULL,
    NULL,
    nulldrv_WindowPosChanging,
    nulldrv_WindowPosChanged,
    /* system parameters */
    NULL,
    /* vulkan support */
    NULL,
    /* opengl support */
    NULL,
    /* thread management */
    NULL,
};

void CDECL __wine_set_user_driver( const struct user_driver_funcs *funcs, UINT version )
{
    struct user_driver_funcs *driver, *prev;

    if (version != WINE_GDI_DRIVER_VERSION)
    {
        ERR( "version mismatch, driver wants %u but user32 has %u\n", version, WINE_GDI_DRIVER_VERSION );
        return;
    }

    driver = HeapAlloc( GetProcessHeap(), 0, sizeof(*driver) );
    *driver = *funcs;

#define SET_USER_FUNC(name) \
    do { if (!driver->p##name) driver->p##name = nulldrv_##name; } while(0)

    SET_USER_FUNC(SetCursorPos);
    SET_USER_FUNC(UpdateClipboard);
    SET_USER_FUNC(CreateWindow);
    SET_USER_FUNC(DestroyWindow);
    SET_USER_FUNC(GetDC);
    SET_USER_FUNC(MsgWaitForMultipleObjectsEx);
    SET_USER_FUNC(ReleaseDC);
    SET_USER_FUNC(SetParent);
    SET_USER_FUNC(SetWindowIcon);
    SET_USER_FUNC(SetWindowStyle);
    SET_USER_FUNC(SetWindowText);
    SET_USER_FUNC(ShowWindow);
    SET_USER_FUNC(SysCommand);
    SET_USER_FUNC(WindowPosChanging);
    SET_USER_FUNC(WindowPosChanged);
#undef SET_USER_FUNC

    prev = InterlockedCompareExchangePointer( (void **)&USER_Driver, driver, &lazy_load_driver );
    if (prev != &lazy_load_driver)
    {
        /* another thread beat us to it */
        HeapFree( GetProcessHeap(), 0, driver );
        driver = prev;
    }

    __wine_set_display_driver( driver, version );
}
