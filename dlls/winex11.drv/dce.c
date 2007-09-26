/*
* USER DCE functions
 *
 * Copyright 1993, 2005 Alexandre Julliard
 * Copyright 1996, 1997 Alex Korobka
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

#include "config.h"

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "win.h"
#include "windef.h"
#include "wingdi.h"
#include "wownt32.h"
#include "x11drv.h"
#include "wine/winbase16.h"
#include "wine/wingdi16.h"
#include "wine/server.h"
#include "wine/list.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

struct dce
{
    struct list entry;         /* entry in global DCE list */
    HDC         hdc;
    HWND        hwnd;
    HRGN        clip_rgn;
    DWORD       flags;
    void       *class_ptr;     /* ptr to identify window class for class DCEs */
    ULONG       count;         /* usage count; 0 or 1 for cache DCEs, always 1 for window DCEs,
                                  always >= 1 for class DCEs */
};

static struct list dce_list = LIST_INIT(dce_list);

static BOOL CALLBACK dc_hook( HDC hDC, WORD code, DWORD_PTR data, LPARAM lParam );

static CRITICAL_SECTION dce_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &dce_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dce_section") }
};
static CRITICAL_SECTION dce_section = { &critsect_debug, -1, 0, 0, 0, 0 };

static const WCHAR displayW[] = { 'D','I','S','P','L','A','Y',0 };


/***********************************************************************
 *           dump_cache
 */
static void dump_cache(void)
{
    struct dce *dce;

    EnterCriticalSection( &dce_section );

    LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
    {
        TRACE("%p: hwnd %p dcx %08x %s %s\n",
              dce, dce->hwnd, dce->flags,
              (dce->flags & DCX_CACHE) ? "Cache" : "Owned",
              dce->count ? "InUse" : "" );
    }

    LeaveCriticalSection( &dce_section );
}


/***********************************************************************
 *		update_visible_region
 *
 * Set the visible region and X11 drawable for the DC associated to
 * a given window.
 */
static void update_visible_region( struct dce *dce )
{
    NTSTATUS status;
    HRGN vis_rgn = 0;
    HWND top = 0;
    struct x11drv_escape_set_drawable escape;
    struct x11drv_win_data *data;
    DWORD flags = dce->flags;
    size_t size = 256;

    /* don't clip siblings if using parent clip region */
    if (flags & DCX_PARENTCLIP) flags &= ~DCX_CLIPSIBLINGS;

    /* fetch the visible region from the server */
    do
    {
        RGNDATA *data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 );
        if (!data) return;

        SERVER_START_REQ( get_visible_region )
        {
            req->window  = dce->hwnd;
            req->flags   = flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                vis_rgn = ExtCreateRegion( NULL, size, data );

                top = reply->top_win;
                escape.dc_rect.left = reply->win_rect.left - reply->top_rect.left;
                escape.dc_rect.top = reply->win_rect.top - reply->top_rect.top;
                escape.dc_rect.right = reply->win_rect.right - reply->top_rect.left;
                escape.dc_rect.bottom = reply->win_rect.bottom - reply->top_rect.top;
                escape.drawable_rect.left = reply->top_rect.left;
                escape.drawable_rect.top = reply->top_rect.top;
                escape.drawable_rect.right = reply->top_rect.right;
                escape.drawable_rect.bottom = reply->top_rect.bottom;
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status || !vis_rgn) return;

    if (dce->clip_rgn) CombineRgn( vis_rgn, vis_rgn, dce->clip_rgn,
                                   (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

    if (top == dce->hwnd && ((data = X11DRV_get_win_data( dce->hwnd )) != NULL) &&
         IsIconic( dce->hwnd ) && data->icon_window)
    {
        escape.drawable = data->icon_window;
        escape.fbconfig_id = 0;
        escape.gl_drawable = 0;
        escape.pixmap = 0;
    }
    else
    {
        escape.drawable = X11DRV_get_whole_window( top );
        escape.fbconfig_id = X11DRV_get_fbconfig_id( dce->hwnd );
        escape.gl_drawable = X11DRV_get_gl_drawable( dce->hwnd );
        escape.pixmap = X11DRV_get_gl_pixmap( dce->hwnd );
    }

    escape.code = X11DRV_SET_DRAWABLE;
    escape.mode = IncludeInferiors;
    ExtEscape( dce->hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    /* map region to DC coordinates */
    OffsetRgn( vis_rgn,
               -(escape.drawable_rect.left + escape.dc_rect.left),
               -(escape.drawable_rect.top + escape.dc_rect.top) );
    SelectVisRgn16( HDC_16(dce->hdc), HRGN_16(vis_rgn) );
    DeleteObject( vis_rgn );
}


/***********************************************************************
 *		release_dce
 */
static void release_dce( struct dce *dce )
{
    struct x11drv_escape_set_drawable escape;

    if (!dce->hwnd) return;  /* already released */

    if (dce->clip_rgn) DeleteObject( dce->clip_rgn );
    dce->clip_rgn = 0;
    dce->hwnd     = 0;
    dce->flags   &= DCX_CACHE;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = root_window;
    escape.mode = IncludeInferiors;
    escape.drawable_rect = virtual_screen_rect;
    SetRect( &escape.dc_rect, 0, 0, virtual_screen_rect.right - virtual_screen_rect.left,
             virtual_screen_rect.bottom - virtual_screen_rect.top );
    escape.fbconfig_id = 0;
    escape.gl_drawable = 0;
    escape.pixmap = 0;
    ExtEscape( dce->hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
}


/***********************************************************************
 *   delete_clip_rgn
 */
static void delete_clip_rgn( struct dce *dce )
{
    if (!dce->clip_rgn) return;  /* nothing to do */

    dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
    DeleteObject( dce->clip_rgn );
    dce->clip_rgn = 0;

    /* make it dirty so that the vis rgn gets recomputed next time */
    SetHookFlags16( HDC_16(dce->hdc), DCHF_INVALIDATEVISRGN );
}


/***********************************************************************
 *           alloc_cache_dce
 *
 * Allocate a new cache DCE.
 */
static struct dce *alloc_cache_dce(void)
{
    struct x11drv_escape_set_dce escape;
    struct dce *dce;

    if (!(dce = HeapAlloc( GetProcessHeap(), 0, sizeof(*dce) ))) return NULL;
    if (!(dce->hdc = CreateDCW( displayW, NULL, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, dce );
        return 0;
    }
    SaveDC( dce->hdc );

    /* store DCE handle in DC hook data field */
    SetDCHook( dce->hdc, dc_hook, (DWORD_PTR)dce );

    dce->hwnd      = 0;
    dce->clip_rgn  = 0;
    dce->flags     = DCX_CACHE;
    dce->class_ptr = NULL;
    dce->count     = 1;

    EnterCriticalSection( &dce_section );
    list_add_head( &dce_list, &dce->entry );
    LeaveCriticalSection( &dce_section );

    escape.code = X11DRV_SET_DCE;
    escape.dce  = dce;
    ExtEscape( dce->hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape, 0, NULL );

    return dce;
}


/***********************************************************************
 *           alloc_window_dce
 *
 * Allocate a DCE for a newly created window if necessary.
 */
void alloc_window_dce( struct x11drv_win_data *data )
{
    struct x11drv_escape_set_dce escape;
    struct dce *dce;
    void *class_ptr = NULL;
    LONG style = GetClassLongW( data->hwnd, GCL_STYLE );

    if (!(style & (CS_CLASSDC|CS_OWNDC))) return;  /* nothing to do */

    if (!(style & CS_OWNDC))  /* class dc */
    {
        /* hack: get the class pointer from the window structure */
        WND *win = WIN_GetPtr( data->hwnd );
        class_ptr = win->class;
        WIN_ReleasePtr( win );

        EnterCriticalSection( &dce_section );
        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if (dce->class_ptr == class_ptr)
            {
                dce->count++;
                data->dce = dce;
                LeaveCriticalSection( &dce_section );
                return;
            }
        }
        LeaveCriticalSection( &dce_section );
    }

    /* now allocate a new one */

    if (!(dce = HeapAlloc( GetProcessHeap(), 0, sizeof(*dce) ))) return;
    if (!(dce->hdc = CreateDCW( displayW, NULL, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, dce );
        return;
    }

    /* store DCE handle in DC hook data field */

    SetDCHook( dce->hdc, dc_hook, (DWORD_PTR)dce );

    dce->hwnd      = data->hwnd;
    dce->clip_rgn  = 0;
    dce->flags     = 0;
    dce->class_ptr = class_ptr;
    dce->count     = 1;

    if (style & CS_OWNDC)
    {
        LONG win_style = GetWindowLongW( data->hwnd, GWL_STYLE );
        if (win_style & WS_CLIPCHILDREN) dce->flags |= DCX_CLIPCHILDREN;
        if (win_style & WS_CLIPSIBLINGS) dce->flags |= DCX_CLIPSIBLINGS;
    }
    SetHookFlags16( HDC_16(dce->hdc), DCHF_INVALIDATEVISRGN );

    EnterCriticalSection( &dce_section );
    list_add_tail( &dce_list, &dce->entry );
    LeaveCriticalSection( &dce_section );
    data->dce = dce;

    escape.code = X11DRV_SET_DCE;
    escape.dce  = dce;
    ExtEscape( dce->hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape, 0, NULL );
}


/***********************************************************************
 *           free_window_dce
 *
 * Free a class or window DCE.
 */
void free_window_dce( struct x11drv_win_data *data )
{
    struct dce *dce = data->dce;

    if (dce)
    {
        EnterCriticalSection( &dce_section );
        if (!--dce->count)
        {
            list_remove( &dce->entry );
            SetDCHook(dce->hdc, NULL, 0L);
            DeleteDC( dce->hdc );
            if (dce->clip_rgn) DeleteObject( dce->clip_rgn );
            HeapFree( GetProcessHeap(), 0, dce );
        }
        else if (dce->hwnd == data->hwnd)
        {
            release_dce( dce );
        }
        LeaveCriticalSection( &dce_section );
        data->dce = NULL;
    }

    /* now check for cache DCEs */

    EnterCriticalSection( &dce_section );
    LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
    {
        if (dce->hwnd != data->hwnd) continue;
        if (!(dce->flags & DCX_CACHE)) continue;

        if (dce->count) WARN( "GetDC() without ReleaseDC() for window %p\n", data->hwnd );
        release_dce( dce );
        dce->count = 0;
    }
    LeaveCriticalSection( &dce_section );
}


/***********************************************************************
 *   invalidate_dce
 *
 * It is called from SetWindowPos() - we have to
 * mark as dirty all busy DCEs for windows that have pWnd->parent as
 * an ancestor and whose client rect intersects with specified update
 * rectangle. In addition, pWnd->parent DCEs may need to be updated if
 * DCX_CLIPCHILDREN flag is set.
 */
void invalidate_dce( HWND hwnd, const RECT *rect )
{
    HWND hwndScope = GetAncestor( hwnd, GA_PARENT );

    if( hwndScope )
    {
        struct dce *dce;

        TRACE("scope hwnd = %p %s\n", hwndScope, wine_dbgstr_rect(rect) );
        if (TRACE_ON(dc)) dump_cache();

        /* walk all DCEs and fixup non-empty entries */

        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if (!dce->hwnd) continue;
            if ((dce->hwnd == hwndScope) && !(dce->flags & DCX_CLIPCHILDREN))
                continue;  /* child window positions don't bother us */

            /* check if DCE window is within the z-order scope */

            if (hwndScope == dce->hwnd || hwndScope == GetDesktopWindow() || IsChild( hwndScope, dce->hwnd ))
            {
                if (hwnd != dce->hwnd)
                {
                    /* check if the window rectangle intersects this DCE window */
                    RECT tmp;
                    GetWindowRect( dce->hwnd, &tmp );
                    MapWindowPoints( 0, hwndScope, (POINT *)&tmp, 2 );
                    if (!IntersectRect( &tmp, &tmp, rect )) continue;

                }
                if (!dce->count)
                {
                    /* Don't bother with visible regions of unused DCEs */

                    TRACE("\tpurged %p dce [%p]\n", dce, dce->hwnd);
                    release_dce( dce );
                }
                else
                {
                    /* Set dirty bits in the hDC and DCE structs */

                    TRACE("\tfixed up %p dce [%p]\n", dce, dce->hwnd);
                    SetHookFlags16( HDC_16(dce->hdc), DCHF_INVALIDATEVISRGN );
                }
            }
        } /* dce list */
    }
}


/***********************************************************************
 *		X11DRV_GetDCEx   (X11DRV.@)
 *
 * Unimplemented flags: DCX_LOCKWINDOWUPDATE
 */
HDC X11DRV_GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    static const DWORD clip_flags = DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW;

    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );
    struct dce *dce;
    BOOL bUpdateVisRgn = TRUE;
    HWND parent;
    LONG window_style = GetWindowLongW( hwnd, GWL_STYLE );

    TRACE("hwnd %p, hrgnClip %p, flags %08x\n", hwnd, hrgnClip, flags);

    /* fixup flags */

    if (flags & (DCX_WINDOW | DCX_PARENTCLIP)) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
        flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if (window_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;

        if (!(flags & DCX_WINDOW))
        {
            if (GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC) flags |= DCX_PARENTCLIP;

            if (window_style & WS_CLIPCHILDREN && !(window_style & WS_MINIMIZE))
                flags |= DCX_CLIPCHILDREN;
            if (!data || !data->dce) flags |= DCX_CACHE;
        }
    }

    if (flags & DCX_WINDOW) flags &= ~DCX_CLIPCHILDREN;

    parent = GetAncestor( hwnd, GA_PARENT );
    if (!parent || (parent == GetDesktopWindow()))
        flags = (flags & ~DCX_PARENTCLIP) | DCX_CLIPSIBLINGS;

    /* it seems parent clip is ignored when clipping siblings or children */
    if (flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) flags &= ~DCX_PARENTCLIP;

    if( flags & DCX_PARENTCLIP )
    {
        LONG parent_style = GetWindowLongW( parent, GWL_STYLE );
        if( (window_style & WS_VISIBLE) && (parent_style & WS_VISIBLE) )
        {
            flags &= ~DCX_CLIPCHILDREN;
            if (parent_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
        }
    }

    /* find a suitable DCE */

    if (flags & DCX_CACHE)
    {
        struct dce *dceEmpty = NULL, *dceUnused = NULL;

        /* Strategy: First, we attempt to find a non-empty but unused DCE with
         * compatible flags. Next, we look for an empty entry. If the cache is
         * full we have to purge one of the unused entries.
         */
        EnterCriticalSection( &dce_section );
        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if ((dce->flags & DCX_CACHE) && !dce->count)
            {
                dceUnused = dce;

                if (!dce->hwnd) dceEmpty = dce;
                else if ((dce->hwnd == hwnd) && !((dce->flags ^ flags) & clip_flags))
                {
                    TRACE("\tfound valid %p dce [%p], flags %08x\n",
                          dce, hwnd, dce->flags );
                    bUpdateVisRgn = FALSE;
                    break;
                }
            }
        }

        if (&dce->entry == &dce_list)  /* nothing found */
            dce = dceEmpty ? dceEmpty : dceUnused;

        if (dce) dce->count = 1;

        LeaveCriticalSection( &dce_section );

        /* if there's no dce empty or unused, allocate a new one */
        if (!dce) dce = alloc_cache_dce();
        if (!dce) return 0;
    }
    else
    {
        flags |= DCX_NORESETATTRS;
        dce = data->dce;
        if (dce->hwnd == hwnd)
        {
            TRACE("\tskipping hVisRgn update\n");
            bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
        }
        else
        {
            /* we should free dce->clip_rgn here, but Windows apparently doesn't */
            dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
            dce->clip_rgn = 0;
        }
    }

    if (flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))
    {
        /* if the extra clip region has changed, get rid of the old one */
        if (dce->clip_rgn != hrgnClip || ((flags ^ dce->flags) & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)))
            delete_clip_rgn( dce );
        dce->clip_rgn = hrgnClip;
        if (!dce->clip_rgn) dce->clip_rgn = CreateRectRgn( 0, 0, 0, 0 );
        dce->flags |= flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN);
        bUpdateVisRgn = TRUE;
    }

    dce->hwnd = hwnd;
    dce->flags = (dce->flags & ~clip_flags) | (flags & clip_flags);

    if (SetHookFlags16( HDC_16(dce->hdc), DCHF_VALIDATEVISRGN ))
        bUpdateVisRgn = TRUE;  /* DC was dirty */

    if (bUpdateVisRgn) update_visible_region( dce );

    if (!(flags & DCX_NORESETATTRS))
    {
        RestoreDC( dce->hdc, 1 );  /* initial save level is always 1 */
        SaveDC( dce->hdc );  /* save the state again for next time */
    }

    TRACE("(%p,%p,0x%x): returning %p\n", hwnd, hrgnClip, flags, dce->hdc);
    return dce->hdc;
}


/***********************************************************************
 *		X11DRV_ReleaseDC  (X11DRV.@)
 */
INT X11DRV_ReleaseDC( HWND hwnd, HDC hdc, BOOL end_paint )
{
    enum x11drv_escape_codes escape = X11DRV_GET_DCE;
    struct dce *dce;
    BOOL ret = FALSE;

    TRACE("%p %p\n", hwnd, hdc );

    EnterCriticalSection( &dce_section );
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(dce), (LPSTR)&dce )) dce = NULL;
    if (dce && dce->count)
    {
        if (end_paint || (dce->flags & DCX_CACHE)) delete_clip_rgn( dce );
        if (dce->flags & DCX_CACHE) dce->count = 0;
        ret = TRUE;
    }
    LeaveCriticalSection( &dce_section );
    return ret;
}

/***********************************************************************
 *		dc_hook
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..
 */
static BOOL CALLBACK dc_hook( HDC hDC, WORD code, DWORD_PTR data, LPARAM lParam )
{
    BOOL retv = TRUE;
    struct dce *dce = (struct dce *)data;

    TRACE("hDC = %p, %u\n", hDC, code);

    if (!dce) return 0;
    assert( dce->hdc == hDC );

    switch( code )
    {
    case DCHC_INVALIDVISRGN:
        /* GDI code calls this when it detects that the
         * DC is dirty (usually after SetHookFlags()). This
         * means that we have to recompute the visible region.
         */
        if (dce->count) update_visible_region( dce );
        else /* non-fatal but shouldn't happen */
            WARN("DC is not in use!\n");
        break;
    case DCHC_DELETEDC:
        /*
         * Windows will not let you delete a DC that is busy
         * (between GetDC and ReleaseDC)
         */
        if (dce->count)
        {
            WARN("Application trying to delete a busy DC %p\n", dce->hdc);
            retv = FALSE;
        }
        else
        {
            EnterCriticalSection( &dce_section );
            list_remove( &dce->entry );
            LeaveCriticalSection( &dce_section );
            if (dce->clip_rgn) DeleteObject( dce->clip_rgn );
            HeapFree( GetProcessHeap(), 0, dce );
        }
        break;
    }
    return retv;
}


/**********************************************************************
 *		WindowFromDC   (X11DRV.@)
 */
HWND X11DRV_WindowFromDC( HDC hdc )
{
    enum x11drv_escape_codes escape = X11DRV_GET_DCE;
    struct dce *dce;
    HWND hwnd = 0;

    EnterCriticalSection( &dce_section );
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(dce), (LPSTR)&dce )) dce = NULL;
    if (dce) hwnd = dce->hwnd;
    LeaveCriticalSection( &dce_section );
    return hwnd;
}
