/*
 * Window related functions
 *
 * Copyright 1993, 1994, 1995, 1996, 2001, 2013-2017 Alexandre Julliard
 * Copyright 1993 David Metcalfe
 * Copyright 1995, 1996 Alex Korobka
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

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "wine/unicode.h"

#include "android.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(android);

/* private window data */
struct android_win_data
{
    HWND           hwnd;           /* hwnd that this private data belongs to */
    HWND           parent;         /* parent hwnd for child windows */
    RECT           window_rect;    /* USER window rectangle relative to parent */
    RECT           whole_rect;     /* X window rectangle for the whole window relative to parent */
    RECT           client_rect;    /* client area relative to parent */
};

#define SWP_AGG_NOPOSCHANGE (SWP_NOSIZE | SWP_NOMOVE | SWP_NOCLIENTSIZE | SWP_NOCLIENTMOVE | SWP_NOZORDER)

static CRITICAL_SECTION win_data_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &win_data_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": win_data_section") }
};
static CRITICAL_SECTION win_data_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static struct android_win_data *win_data_context[32768];

static inline int context_idx( HWND hwnd )
{
    return LOWORD( hwnd ) >> 1;
}


/***********************************************************************
 *           alloc_win_data
 */
static struct android_win_data *alloc_win_data( HWND hwnd )
{
    struct android_win_data *data;

    if ((data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*data))))
    {
        data->hwnd = hwnd;
        EnterCriticalSection( &win_data_section );
        win_data_context[context_idx(hwnd)] = data;
    }
    return data;
}


/***********************************************************************
 *           free_win_data
 */
static void free_win_data( struct android_win_data *data )
{
    win_data_context[context_idx( data->hwnd )] = NULL;
    LeaveCriticalSection( &win_data_section );
    HeapFree( GetProcessHeap(), 0, data );
}


/***********************************************************************
 *           get_win_data
 *
 * Lock and return the data structure associated with a window.
 */
static struct android_win_data *get_win_data( HWND hwnd )
{
    struct android_win_data *data;

    if (!hwnd) return NULL;
    EnterCriticalSection( &win_data_section );
    if ((data = win_data_context[context_idx(hwnd)]) && data->hwnd == hwnd) return data;
    LeaveCriticalSection( &win_data_section );
    return NULL;
}


/***********************************************************************
 *           release_win_data
 *
 * Release the data returned by get_win_data.
 */
static void release_win_data( struct android_win_data *data )
{
    if (data) LeaveCriticalSection( &win_data_section );
}


/***********************************************************************
 *           ANDROID_DestroyWindow
 */
void CDECL ANDROID_DestroyWindow( HWND hwnd )
{
    struct android_win_data *data;

    if (!(data = get_win_data( hwnd ))) return;

    free_win_data( data );
}


/***********************************************************************
 *           create_win_data
 *
 * Create a data window structure for an existing window.
 */
static struct android_win_data *create_win_data( HWND hwnd, const RECT *window_rect,
                                                 const RECT *client_rect )
{
    struct android_win_data *data;
    HWND parent;

    if (!(parent = GetAncestor( hwnd, GA_PARENT ))) return NULL;  /* desktop or HWND_MESSAGE */

    if (parent != GetDesktopWindow())
    {
        if (!(data = get_win_data( parent )) &&
            !(data = create_win_data( parent, NULL, NULL )))
            return NULL;
        release_win_data( data );
    }

    if (!(data = alloc_win_data( hwnd ))) return NULL;

    data->parent = (parent == GetDesktopWindow()) ? 0 : parent;

    if (window_rect)
    {
        data->whole_rect = data->window_rect = *window_rect;
        data->client_rect = *client_rect;
    }
    else
    {
        GetWindowRect( hwnd, &data->window_rect );
        MapWindowPoints( 0, parent, (POINT *)&data->window_rect, 2 );
        data->whole_rect = data->window_rect;
        GetClientRect( hwnd, &data->client_rect );
        MapWindowPoints( hwnd, parent, (POINT *)&data->client_rect, 2 );
    }
    return data;
}


/***********************************************************************
 *           ANDROID_WindowPosChanging
 */
void CDECL ANDROID_WindowPosChanging( HWND hwnd, HWND insert_after, UINT swp_flags,
                                     const RECT *window_rect, const RECT *client_rect, RECT *visible_rect,
                                     struct window_surface **surface )
{
    struct android_win_data *data = get_win_data( hwnd );

    TRACE( "win %p window %s client %s style %08x flags %08x\n",
           hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect),
           GetWindowLongW( hwnd, GWL_STYLE ), swp_flags );

    if (!data && !(data = create_win_data( hwnd, window_rect, client_rect ))) return;

    *visible_rect = *window_rect;

    release_win_data( data );
}


/***********************************************************************
 *           ANDROID_WindowPosChanged
 */
void CDECL ANDROID_WindowPosChanged( HWND hwnd, HWND insert_after, UINT swp_flags,
                                    const RECT *window_rect, const RECT *client_rect,
                                    const RECT *visible_rect, const RECT *valid_rects,
                                    struct window_surface *surface )
{
    struct android_win_data *data;
    DWORD new_style = GetWindowLongW( hwnd, GWL_STYLE );

    if (!(data = get_win_data( hwnd ))) return;

    data->window_rect = *window_rect;
    data->whole_rect  = *visible_rect;
    data->client_rect = *client_rect;

    release_win_data( data );

    TRACE( "win %p window %s client %s style %08x flags %08x\n",
           hwnd, wine_dbgstr_rect(window_rect), wine_dbgstr_rect(client_rect), new_style, swp_flags );
}
