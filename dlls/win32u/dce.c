/*
 * Window painting functions
 *
 * Copyright 1993, 1994, 1995, 2001, 2004, 2005, 2008 Alexandre Julliard
 * Copyright 1996, 1997, 1999 Alex Korobka
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

#if 0
#pragma makedep unix
#endif

#include <pthread.h>
#include "ntgdi_private.h"
#include "ntuser_private.h"


static struct list window_surfaces = LIST_INIT( window_surfaces );
static pthread_mutex_t surfaces_lock = PTHREAD_MUTEX_INITIALIZER;

/*******************************************************************
 * Dummy window surface for windows that shouldn't get painted.
 */

static void CDECL dummy_surface_lock( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void CDECL dummy_surface_unlock( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void *CDECL dummy_surface_get_bitmap_info( struct window_surface *window_surface, BITMAPINFO *info )
{
    static DWORD dummy_data;

    info->bmiHeader.biSize          = sizeof( info->bmiHeader );
    info->bmiHeader.biWidth         = dummy_surface.rect.right;
    info->bmiHeader.biHeight        = dummy_surface.rect.bottom;
    info->bmiHeader.biPlanes        = 1;
    info->bmiHeader.biBitCount      = 32;
    info->bmiHeader.biCompression   = BI_RGB;
    info->bmiHeader.biSizeImage     = 0;
    info->bmiHeader.biXPelsPerMeter = 0;
    info->bmiHeader.biYPelsPerMeter = 0;
    info->bmiHeader.biClrUsed       = 0;
    info->bmiHeader.biClrImportant  = 0;
    return &dummy_data;
}

static RECT *CDECL dummy_surface_get_bounds( struct window_surface *window_surface )
{
    static RECT dummy_bounds;
    return &dummy_bounds;
}

static void CDECL dummy_surface_set_region( struct window_surface *window_surface, HRGN region )
{
    /* nothing to do */
}

static void CDECL dummy_surface_flush( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void CDECL dummy_surface_destroy( struct window_surface *window_surface )
{
    /* nothing to do */
}

static const struct window_surface_funcs dummy_surface_funcs =
{
    dummy_surface_lock,
    dummy_surface_unlock,
    dummy_surface_get_bitmap_info,
    dummy_surface_get_bounds,
    dummy_surface_set_region,
    dummy_surface_flush,
    dummy_surface_destroy
};

struct window_surface dummy_surface = { &dummy_surface_funcs, { NULL, NULL }, 1, { 0, 0, 1, 1 } };

/*******************************************************************
 *           register_window_surface
 *
 * Register a window surface in the global list, possibly replacing another one.
 */
void register_window_surface( struct window_surface *old, struct window_surface *new )
{
    if (old == &dummy_surface) old = NULL;
    if (new == &dummy_surface) new = NULL;
    if (old == new) return;
    pthread_mutex_lock( &surfaces_lock );
    if (old) list_remove( &old->entry );
    if (new) list_add_tail( &window_surfaces, &new->entry );
    pthread_mutex_unlock( &surfaces_lock );
}

/*******************************************************************
 *           flush_window_surfaces
 *
 * Flush pending output from all window surfaces.
 */
void flush_window_surfaces( BOOL idle )
{
    static DWORD last_idle;
    DWORD now;
    struct window_surface *surface;

    pthread_mutex_lock( &surfaces_lock );
    now = NtGetTickCount();
    if (idle) last_idle = now;
    /* if not idle, we only flush if there's evidence that the app never goes idle */
    else if ((int)(now - last_idle) < 50) goto done;

    LIST_FOR_EACH_ENTRY( surface, &window_surfaces, struct window_surface, entry )
        surface->funcs->flush( surface );
done:
    pthread_mutex_unlock( &surfaces_lock );
}

/**********************************************************************
 *           NtUserWindowFromDC (win32u.@)
 */
HWND WINAPI NtUserWindowFromDC( HDC hdc )
{
    struct dce *dce;
    HWND hwnd = 0;

    user_lock();
    dce = (struct dce *)GetDCHook( hdc, NULL );
    if (dce) hwnd = dce->hwnd;
    user_unlock();
    return hwnd;
}
