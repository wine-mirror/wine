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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *
 * Note: Visible regions of CS_OWNDC/CS_CLASSDC window DCs
 * have to be updated dynamically.
 *
 * Internal DCX flags:
 *
 * DCX_WINDOWPAINT - BeginPaint() is in effect
 */

#include "config.h"

#include <assert.h>
#include "win.h"
#include "windef.h"
#include "wingdi.h"
#include "wownt32.h"
#include "ntstatus.h"
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
    int         empty : 1;     /* DCE is uninitialized */
    int         inuse : 1;     /* DCE is in use */
    int         dirty : 1;     /* ReleaseDC should wipe instead of caching */
    void       *class_ptr;     /* ptr to identify window class for class DCEs */
    ULONG       count;         /* reference count for class DCEs */
};

static struct list dce_list = LIST_INIT(dce_list);

static void delete_clip_rgn( struct dce * );
static INT release_dc( struct dce * );
static BOOL16 CALLBACK dc_hook( HDC16 hDC, WORD code, DWORD data, LPARAM lParam );

static CRITICAL_SECTION dce_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &dce_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { 0, (DWORD)(__FILE__ ": dce_section") }
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
        TRACE("%p: hwnd %p dcx %08lx %s %s\n",
              dce, dce->hwnd, dce->flags,
              (dce->flags & DCX_CACHE) ? "Cache" : "Owned",
              dce->inuse ? "InUse" : "" );
    }

    LeaveCriticalSection( &dce_section );
}


/***********************************************************************
 *		get_server_visible_region
 */
static HRGN get_server_visible_region( HWND hwnd, UINT flags )
{
    RGNDATA *data;
    NTSTATUS status;
    HRGN ret = 0;
    size_t size = 256;

    do
    {
        if (!(data = HeapAlloc( GetProcessHeap(), 0, sizeof(*data) + size - 1 ))) return 0;
        SERVER_START_REQ( get_visible_region )
        {
            req->window  = hwnd;
            req->flags   = flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                ret = ExtCreateRegion( NULL, size, data );
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        HeapFree( GetProcessHeap(), 0, data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) SetLastError( RtlNtStatusToDosError(status) );
    return ret;
}


/***********************************************************************
 *           get_top_clipping_window
 *
 * Get the top window to clip against (i.e. the top parent that has
 * an associated X window).
 */
static HWND get_top_clipping_window( HWND hwnd )
{
    HWND ret = GetAncestor( hwnd, GA_ROOT );
    if (!ret) ret = GetDesktopWindow();
    return ret;
}


/***********************************************************************
 *		set_drawable
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
static void set_drawable( HWND hwnd, HDC hdc, HRGN hrgn, DWORD flags )
{
    HWND top = get_top_clipping_window( hwnd );
    struct x11drv_escape_set_drawable escape;
    struct x11drv_win_data *data;

    escape.mode = IncludeInferiors;
    /* don't clip siblings if using parent clip region */
    if (flags & DCX_PARENTCLIP) flags &= ~DCX_CLIPSIBLINGS;

    if (top != hwnd || !(data = X11DRV_get_win_data( hwnd )))
    {
        POINT client_offset;

        if (flags & DCX_WINDOW)
        {
            RECT rect;
            GetWindowRect( hwnd, &rect );
            escape.org.x = rect.left;
            escape.org.y = rect.top;
            MapWindowPoints( 0, top, &escape.org, 1 );
            escape.drawable_org.x = rect.left - escape.org.x;
            escape.drawable_org.y = rect.top - escape.org.y;
        }
        else
        {
            escape.org.x = escape.org.y = 0;
            escape.drawable_org.x = escape.drawable_org.y = 0;
            MapWindowPoints( hwnd, top, &escape.org, 1 );
            MapWindowPoints( top, 0, &escape.drawable_org, 1 );
        }

        /* now make origins relative to the X window and not the client area */
        client_offset = X11DRV_get_client_area_offset( top );
        escape.org.x += client_offset.x;
        escape.org.y += client_offset.y;
        escape.drawable_org.x -= client_offset.x;
        escape.drawable_org.y -= client_offset.y;
        escape.drawable = X11DRV_get_whole_window( top );
    }
    else
    {
        if (IsIconic( hwnd ))
        {
            escape.drawable = data->icon_window ? data->icon_window : data->whole_window;
            escape.org.x = 0;
            escape.org.y = 0;
            escape.drawable_org = escape.org;
            MapWindowPoints( hwnd, 0, &escape.drawable_org, 1 );
        }
        else
        {
            escape.drawable = data->whole_window;
            escape.drawable_org.x = data->whole_rect.left;
            escape.drawable_org.y = data->whole_rect.top;
            if (flags & DCX_WINDOW)
            {
                escape.org.x = data->window_rect.left - data->whole_rect.left;
                escape.org.y = data->window_rect.top - data->whole_rect.top;
            }
            else
            {
                escape.org.x = data->client_rect.left;
                escape.org.y = data->client_rect.top;
            }
        }
    }

    escape.code = X11DRV_SET_DRAWABLE;
    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );

    if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) ||
        SetHookFlags16( HDC_16(hdc), DCHF_VALIDATEVISRGN ))  /* DC was dirty */
    {
        /* need to recompute the visible region */
        HRGN visRgn = get_server_visible_region( hwnd, flags );

        if (flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
            CombineRgn( visRgn, visRgn, hrgn, (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

        SelectVisRgn16( HDC_16(hdc), HRGN_16(visRgn) );
        DeleteObject( visRgn );
    }
}


/***********************************************************************
 *		release_drawable
 */
static void release_drawable( HWND hwnd, HDC hdc )
{
    struct x11drv_escape_set_drawable escape;

    escape.code = X11DRV_SET_DRAWABLE;
    escape.drawable = root_window;
    escape.mode = IncludeInferiors;
    escape.org.x = escape.org.y = 0;
    escape.drawable_org.x = escape.drawable_org.y = 0;

    ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPSTR)&escape, 0, NULL );
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
    SetDCHook( dce->hdc, dc_hook, (DWORD)dce );

    dce->hwnd      = 0;
    dce->clip_rgn  = 0;
    dce->flags     = DCX_CACHE;
    dce->empty     = 1;
    dce->inuse     = 0;
    dce->dirty     = 0;
    dce->class_ptr = NULL;
    dce->count     = 0;

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
    SaveDC( dce->hdc );

    /* store DCE handle in DC hook data field */

    SetDCHook( dce->hdc, dc_hook, (DWORD)dce );

    dce->hwnd      = data->hwnd;
    dce->clip_rgn  = 0;
    dce->flags     = 0;
    dce->empty     = 0;
    dce->inuse     = 1;
    dce->dirty     = 0;
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
            if (dce->flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))
            {
                release_drawable( dce->hwnd, dce->hdc );
                delete_clip_rgn( dce );
                dce->hwnd = 0;
            }
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

        if (dce->inuse) /* shared cache DCE */
        {
            WARN( "GetDC() without ReleaseDC() for window %p\n", data->hwnd );
            release_dc( dce );
        }
        if (dce->hwnd) release_drawable( dce->hwnd, dce->hdc );
        dce->hwnd  = 0;
        dce->flags = DCX_CACHE;
        dce->inuse = 0;
        dce->empty = 1;
        dce->dirty = 0;
    }
    LeaveCriticalSection( &dce_section );
}


/***********************************************************************
 *   delete_clip_rgn
 */
static void delete_clip_rgn( struct dce *dce )
{
    dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

    if (dce->clip_rgn) DeleteObject( dce->clip_rgn );
    dce->clip_rgn = 0;

    /* make it dirty so that the vis rgn gets recomputed next time */
    dce->dirty = 1;
    SetHookFlags16( HDC_16(dce->hdc), DCHF_INVALIDATEVISRGN );
}


/***********************************************************************
 *   release_dc
 */
static BOOL release_dc( struct dce *dce )
{
    if (dce->empty) return FALSE;
    if (!dce->inuse) return FALSE;

    /* restore previous visible region */

    if ((dce->flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
        (dce->flags & (DCX_CACHE | DCX_WINDOWPAINT)) )
        delete_clip_rgn( dce );

    if (dce->flags & DCX_CACHE)
    {
        /* make the DC clean so that RestoreDC doesn't try to update the vis rgn */
        SetHookFlags16( HDC_16(dce->hdc), DCHF_VALIDATEVISRGN );
        RestoreDC( dce->hdc, 1 );  /* initial save level is always 1 */
        SaveDC( dce->hdc );  /* save the state again for next time */
        dce->inuse = 0;
        if (dce->dirty)
        {
            /* don't keep around invalidated entries
	     * because RestoreDC() disables hVisRgn updates
	     * by removing dirty bit. */
            if (dce->hwnd) release_drawable( dce->hwnd, dce->hdc );
            dce->hwnd  = 0;
            dce->flags = DCX_CACHE;
            dce->empty = 1;
            dce->dirty = 0;
        }
    }
    return TRUE;
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
            if (dce->empty) continue;
            if ((dce->hwnd == hwndScope) && !(dce->flags & DCX_CLIPCHILDREN))
                continue;  /* child window positions don't bother us */

            /* check if DCE window is within the z-order scope */

            if (hwndScope == dce->hwnd || IsChild( hwndScope, dce->hwnd ))
            {
                if (hwnd != dce->hwnd)
                {
                    /* check if the window rectangle intersects this DCE window */
                    RECT tmp;
                    GetWindowRect( dce->hwnd, &tmp );
                    MapWindowPoints( 0, hwndScope, (POINT *)&tmp, 2 );
                    if (!IntersectRect( &tmp, &tmp, rect )) continue;

                }
                if (!dce->inuse)
                {
                    /* Don't bother with visible regions of unused DCEs */

                    TRACE("\tpurged %p dce [%p]\n", dce, dce->hwnd);
                    if (dce->hwnd) release_drawable( dce->hwnd, dce->hdc );
                    dce->hwnd = 0;
                    dce->flags &= DCX_CACHE;
                    dce->empty = 1;
                    dce->dirty = 0;
                }
                else
                {
                    /* Set dirty bits in the hDC and DCE structs */

                    TRACE("\tfixed up %p dce [%p]\n", dce, dce->hwnd);
                    dce->dirty = 1;
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
    struct x11drv_win_data *data = X11DRV_get_win_data( hwnd );
    struct dce *dce;
    HDC hdc = 0;
    DWORD dcxFlags = 0;
    BOOL bUpdateVisRgn = TRUE;
    HWND parent;
    LONG window_style = GetWindowLongW( hwnd, GWL_STYLE );

    TRACE("hwnd %p, hrgnClip %p, flags %08lx\n", hwnd, hrgnClip, flags);


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

    if (!(flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))) hrgnClip = 0;

    /* find a suitable DCE */

    dcxFlags = flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                        DCX_CACHE | DCX_WINDOW);

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
            if ((dce->flags & DCX_CACHE) && !dce->inuse)
            {
                dceUnused = dce;

                if (dce->empty) dceEmpty = dce;
                else if ((dce->hwnd == hwnd) &&
                         ((dce->flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                                         DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)) == dcxFlags))
                {
                    TRACE("\tfound valid %p dce [%p], flags %08lx\n",
                          dce, hwnd, dcxFlags );
                    bUpdateVisRgn = FALSE;
                    break;
                }
            }
        }

        if (&dce->entry == &dce_list)  /* nothing found */
            dce = dceEmpty ? dceEmpty : dceUnused;

        if (dce)
        {
            dce->empty = 0;
            dce->inuse = 1;
        }

        LeaveCriticalSection( &dce_section );

        /* if there's no dce empty or unused, allocate a new one */
        if (!dce) dce = alloc_cache_dce();
        if (!dce) return 0;
    }
    else
    {
        dce = data->dce;
        if (dce->hwnd == hwnd)
        {
            TRACE("\tskipping hVisRgn update\n");
            bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
        }
    }

    if (((flags ^ dce->flags) & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
        (dce->clip_rgn != hrgnClip))
    {
        /* if the extra clip region has changed, get rid of the old one */
        delete_clip_rgn( dce );
    }

    dce->hwnd = hwnd;
    dce->clip_rgn = hrgnClip;
    dce->flags = flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                          DCX_CACHE | DCX_WINDOW | DCX_WINDOWPAINT |
                          DCX_INTERSECTRGN | DCX_EXCLUDERGN);
    dce->empty = 0;
    dce->inuse = 1;
    dce->dirty = 0;
    hdc = dce->hdc;

    if (bUpdateVisRgn) SetHookFlags16( HDC_16(hdc), DCHF_INVALIDATEVISRGN ); /* force update */

    set_drawable( hwnd, hdc, hrgnClip, flags );

    TRACE("(%p,%p,0x%lx): returning %p\n", hwnd, hrgnClip, flags, hdc);
    return hdc;
}


/***********************************************************************
 *		X11DRV_ReleaseDC  (X11DRV.@)
 */
BOOL X11DRV_ReleaseDC( HWND hwnd, HDC hdc )
{
    enum x11drv_escape_codes escape = X11DRV_GET_DCE;
    struct dce *dce;
    BOOL ret = FALSE;

    TRACE("%p %p\n", hwnd, hdc );

    EnterCriticalSection( &dce_section );
    if (!ExtEscape( hdc, X11DRV_ESCAPE, sizeof(escape), (LPCSTR)&escape,
                    sizeof(dce), (LPSTR)&dce )) dce = NULL;
    if (dce && dce->inuse) ret = release_dc( dce );
    LeaveCriticalSection( &dce_section );
    return ret;
}

/***********************************************************************
 *		dc_hook
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..
 */
static BOOL16 CALLBACK dc_hook( HDC16 hDC, WORD code, DWORD data, LPARAM lParam )
{
    BOOL retv = TRUE;
    struct dce *dce = (struct dce *)data;

    TRACE("hDC = %04x, %i\n", hDC, code);

    if (!dce) return 0;
    assert( HDC_16(dce->hdc) == hDC );

    switch( code )
    {
    case DCHC_INVALIDVISRGN:
        /* GDI code calls this when it detects that the
         * DC is dirty (usually after SetHookFlags()). This
         * means that we have to recompute the visible region.
         */
        if (dce->inuse)
        {
            /* Dirty bit has been cleared by caller, set it again so that
             * pGetDC recomputes the visible region. */
            SetHookFlags16( hDC, DCHF_INVALIDATEVISRGN );
            set_drawable( dce->hwnd, dce->hdc, dce->clip_rgn, dce->flags );
        }
        else /* non-fatal but shouldn't happen */
            WARN("DC is not in use!\n");
        break;
    case DCHC_DELETEDC:
        /*
         * Windows will not let you delete a DC that is busy
         * (between GetDC and ReleaseDC)
         */
        if (dce->inuse)
        {
            WARN("Application trying to delete a busy DC\n");
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
