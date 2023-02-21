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

#include <assert.h>
#include <pthread.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "ntgdi_private.h"
#include "ntuser_private.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(win);

struct dce
{
    struct list entry;         /* entry in global DCE list */
    HDC         hdc;
    HWND        hwnd;
    HRGN        clip_rgn;
    UINT        flags;
    LONG        count;         /* usage count; 0 or 1 for cache DCEs, always 1 for window DCEs,
                                  always >= 1 for class DCEs */
};

static struct list dce_list = LIST_INIT(dce_list);

#define DCE_CACHE_SIZE 64

static struct list window_surfaces = LIST_INIT( window_surfaces );
static pthread_mutex_t surfaces_lock = PTHREAD_MUTEX_INITIALIZER;

/*******************************************************************
 * Dummy window surface for windows that shouldn't get painted.
 */

static void dummy_surface_lock( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void dummy_surface_unlock( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void *dummy_surface_get_bitmap_info( struct window_surface *window_surface, BITMAPINFO *info )
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

static RECT *dummy_surface_get_bounds( struct window_surface *window_surface )
{
    static RECT dummy_bounds;
    return &dummy_bounds;
}

static void dummy_surface_set_region( struct window_surface *window_surface, HRGN region )
{
    /* nothing to do */
}

static void dummy_surface_flush( struct window_surface *window_surface )
{
    /* nothing to do */
}

static void dummy_surface_destroy( struct window_surface *window_surface )
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
 * Off-screen window surface.
 */

struct offscreen_window_surface
{
    struct window_surface header;
    pthread_mutex_t mutex;
    RECT bounds;
    char *bits;
    BITMAPINFO info;
};

static const struct window_surface_funcs offscreen_window_surface_funcs;

static struct offscreen_window_surface *impl_from_window_surface( struct window_surface *base )
{
    if (!base || base->funcs != &offscreen_window_surface_funcs) return NULL;
    return CONTAINING_RECORD( base, struct offscreen_window_surface, header );
}

static void offscreen_window_surface_lock( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    pthread_mutex_lock( &impl->mutex );
}

static void offscreen_window_surface_unlock( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    pthread_mutex_unlock( &impl->mutex );
}

static RECT *offscreen_window_surface_get_bounds( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    return &impl->bounds;
}

static void *offscreen_window_surface_get_bitmap_info( struct window_surface *base, BITMAPINFO *info )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    info->bmiHeader = impl->info.bmiHeader;
    return impl->bits;
}

static void offscreen_window_surface_set_region( struct window_surface *base, HRGN region )
{
}

static void offscreen_window_surface_flush( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    base->funcs->lock( base );
    reset_bounds( &impl->bounds );
    base->funcs->unlock( base );
}

static void offscreen_window_surface_destroy( struct window_surface *base )
{
    struct offscreen_window_surface *impl = impl_from_window_surface( base );
    free( impl );
}

static const struct window_surface_funcs offscreen_window_surface_funcs =
{
    offscreen_window_surface_lock,
    offscreen_window_surface_unlock,
    offscreen_window_surface_get_bitmap_info,
    offscreen_window_surface_get_bounds,
    offscreen_window_surface_set_region,
    offscreen_window_surface_flush,
    offscreen_window_surface_destroy
};

void create_offscreen_window_surface( const RECT *visible_rect, struct window_surface **surface )
{
    struct offscreen_window_surface *impl;
    SIZE_T size;
    RECT surface_rect = *visible_rect;
    pthread_mutexattr_t attr;

    TRACE( "visible_rect %s, surface %p.\n", wine_dbgstr_rect( visible_rect ), surface );

    OffsetRect( &surface_rect, -surface_rect.left, -surface_rect.top );
    surface_rect.right  = (surface_rect.right + 0x1f) & ~0x1f;
    surface_rect.bottom = (surface_rect.bottom + 0x1f) & ~0x1f;

    /* check that old surface is an offscreen_window_surface, or release it */
    if ((impl = impl_from_window_surface( *surface )))
    {
        /* if the rect didn't change, keep the same surface */
        if (EqualRect( &surface_rect, &impl->header.rect )) return;
        window_surface_release( &impl->header );
    }
    else if (*surface) window_surface_release( *surface );

    /* create a new window surface */
    *surface = NULL;
    size = surface_rect.right * surface_rect.bottom * 4;
    if (!(impl = calloc(1, offsetof( struct offscreen_window_surface, info.bmiColors[0] ) + size))) return;

    impl->header.funcs = &offscreen_window_surface_funcs;
    impl->header.ref = 1;
    impl->header.rect = surface_rect;

    pthread_mutexattr_init( &attr );
    pthread_mutexattr_settype( &attr, PTHREAD_MUTEX_RECURSIVE );
    pthread_mutex_init( &impl->mutex, &attr );
    pthread_mutexattr_destroy( &attr );

    reset_bounds( &impl->bounds );

    impl->bits = (char *)&impl->info.bmiColors[0];
    impl->info.bmiHeader.biSize        = sizeof( impl->info );
    impl->info.bmiHeader.biWidth       = surface_rect.right;
    impl->info.bmiHeader.biHeight      = surface_rect.bottom;
    impl->info.bmiHeader.biPlanes      = 1;
    impl->info.bmiHeader.biBitCount    = 32;
    impl->info.bmiHeader.biCompression = BI_RGB;
    impl->info.bmiHeader.biSizeImage   = size;

    TRACE( "created window surface %p\n", &impl->header );

    *surface = &impl->header;
}

/* window surface used to implement the DIB.DRV driver */

struct dib_window_surface
{
    struct window_surface header;
    RECT                  bounds;
    void                 *bits;
    UINT                  info_size;
    BITMAPINFO            info;   /* variable size, must be last */
};

static struct dib_window_surface *get_dib_surface( struct window_surface *surface )
{
    return (struct dib_window_surface *)surface;
}

/***********************************************************************
 *           dib_surface_lock
 */
static void dib_surface_lock( struct window_surface *window_surface )
{
    /* nothing to do */
}

/***********************************************************************
 *           dib_surface_unlock
 */
static void dib_surface_unlock( struct window_surface *window_surface )
{
    /* nothing to do */
}

/***********************************************************************
 *           dib_surface_get_bitmap_info
 */
static void *dib_surface_get_bitmap_info( struct window_surface *window_surface, BITMAPINFO *info )
{
    struct dib_window_surface *surface = get_dib_surface( window_surface );

    memcpy( info, &surface->info, surface->info_size );
    return surface->bits;
}

/***********************************************************************
 *           dib_surface_get_bounds
 */
static RECT *dib_surface_get_bounds( struct window_surface *window_surface )
{
    struct dib_window_surface *surface = get_dib_surface( window_surface );

    return &surface->bounds;
}

/***********************************************************************
 *           dib_surface_set_region
 */
static void dib_surface_set_region( struct window_surface *window_surface, HRGN region )
{
    /* nothing to do */
}

/***********************************************************************
 *           dib_surface_flush
 */
static void dib_surface_flush( struct window_surface *window_surface )
{
    /* nothing to do */
}

/***********************************************************************
 *           dib_surface_destroy
 */
static void dib_surface_destroy( struct window_surface *window_surface )
{
    struct dib_window_surface *surface = get_dib_surface( window_surface );

    TRACE( "freeing %p\n", surface );
    free( surface );
}

static const struct window_surface_funcs dib_surface_funcs =
{
    dib_surface_lock,
    dib_surface_unlock,
    dib_surface_get_bitmap_info,
    dib_surface_get_bounds,
    dib_surface_set_region,
    dib_surface_flush,
    dib_surface_destroy
};

BOOL create_dib_surface( HDC hdc, const BITMAPINFO *info )
{
    struct dib_window_surface *surface;
    int color = 0;
    HRGN region;
    RECT rect;

    if (info->bmiHeader.biBitCount <= 8)
        color = info->bmiHeader.biClrUsed ? info->bmiHeader.biClrUsed : (1 << info->bmiHeader.biBitCount);
    else if (info->bmiHeader.biCompression == BI_BITFIELDS)
        color = 3;

    surface = calloc( 1, offsetof( struct dib_window_surface, info.bmiColors[color] ));
    if (!surface) return FALSE;

    rect.left   = 0;
    rect.top    = 0;
    rect.right  = info->bmiHeader.biWidth;
    rect.bottom = abs(info->bmiHeader.biHeight);

    surface->header.funcs = &dib_surface_funcs;
    surface->header.rect  = rect;
    surface->header.ref   = 1;
    surface->info_size    = offsetof( BITMAPINFO, bmiColors[color] );
    surface->bits         = (char *)info + surface->info_size;
    memcpy( &surface->info, info, surface->info_size );

    TRACE( "created %p %ux%u for info %p bits %p\n",
           surface, (int)rect.right, (int)rect.bottom, info, surface->bits );

    region = NtGdiCreateRectRgn( rect.left, rect.top, rect.right, rect.bottom );
    set_visible_region( hdc, region, &rect, &rect, &surface->header );
    TRACE( "using hdc %p surface %p\n", hdc, surface );
    window_surface_release( &surface->header );
    return TRUE;
}

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

/***********************************************************************
 *           dump_rdw_flags
 */
static void dump_rdw_flags(UINT flags)
{
    TRACE("flags:");
    if (flags & RDW_INVALIDATE) TRACE(" RDW_INVALIDATE");
    if (flags & RDW_INTERNALPAINT) TRACE(" RDW_INTERNALPAINT");
    if (flags & RDW_ERASE) TRACE(" RDW_ERASE");
    if (flags & RDW_VALIDATE) TRACE(" RDW_VALIDATE");
    if (flags & RDW_NOINTERNALPAINT) TRACE(" RDW_NOINTERNALPAINT");
    if (flags & RDW_NOERASE) TRACE(" RDW_NOERASE");
    if (flags & RDW_NOCHILDREN) TRACE(" RDW_NOCHILDREN");
    if (flags & RDW_ALLCHILDREN) TRACE(" RDW_ALLCHILDREN");
    if (flags & RDW_UPDATENOW) TRACE(" RDW_UPDATENOW");
    if (flags & RDW_ERASENOW) TRACE(" RDW_ERASENOW");
    if (flags & RDW_FRAME) TRACE(" RDW_FRAME");
    if (flags & RDW_NOFRAME) TRACE(" RDW_NOFRAME");

#define RDW_FLAGS \
    (RDW_INVALIDATE | \
     RDW_INTERNALPAINT | \
     RDW_ERASE | \
     RDW_VALIDATE | \
     RDW_NOINTERNALPAINT | \
     RDW_NOERASE | \
     RDW_NOCHILDREN | \
     RDW_ALLCHILDREN | \
     RDW_UPDATENOW | \
     RDW_ERASENOW | \
     RDW_FRAME | \
     RDW_NOFRAME)

    if (flags & ~RDW_FLAGS) TRACE(" %04x", flags & ~RDW_FLAGS);
    TRACE("\n");
#undef RDW_FLAGS
}

/***********************************************************************
 *           update_visible_region
 *
 * Set the visible region and X11 drawable for the DC associated to
 * a given window.
 */
static void update_visible_region( struct dce *dce )
{
    struct window_surface *surface = NULL;
    NTSTATUS status;
    HRGN vis_rgn = 0;
    HWND top_win = 0;
    DWORD flags = dce->flags;
    DWORD paint_flags = 0;
    size_t size = 256;
    RECT win_rect, top_rect;
    WND *win;

    /* don't clip siblings if using parent clip region */
    if (flags & DCX_PARENTCLIP) flags &= ~DCX_CLIPSIBLINGS;

    /* fetch the visible region from the server */
    do
    {
        RGNDATA *data = malloc( sizeof(*data) + size - 1 );
        if (!data) return;

        SERVER_START_REQ( get_visible_region )
        {
            req->window  = wine_server_user_handle( dce->hwnd );
            req->flags   = flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                vis_rgn = NtGdiExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );

                top_win         = wine_server_ptr_handle( reply->top_win );
                win_rect.left   = reply->win_rect.left;
                win_rect.top    = reply->win_rect.top;
                win_rect.right  = reply->win_rect.right;
                win_rect.bottom = reply->win_rect.bottom;
                top_rect.left   = reply->top_rect.left;
                top_rect.top    = reply->top_rect.top;
                top_rect.right  = reply->top_rect.right;
                top_rect.bottom = reply->top_rect.bottom;
                paint_flags     = reply->paint_flags;
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        free( data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status || !vis_rgn) return;

    user_driver->pGetDC( dce->hdc, dce->hwnd, top_win, &win_rect, &top_rect, flags );

    if (dce->clip_rgn) NtGdiCombineRgn( vis_rgn, vis_rgn, dce->clip_rgn,
                                        (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );

    /* don't use a surface to paint the client area of OpenGL windows */
    if (!(paint_flags & SET_WINPOS_PIXEL_FORMAT) || (flags & DCX_WINDOW))
    {
        win = get_win_ptr( top_win );
        if (win && win != WND_DESKTOP && win != WND_OTHER_PROCESS)
        {
            surface = win->surface;
            if (surface) window_surface_add_ref( surface );
            release_win_ptr( win );
        }
    }

    if (!surface) SetRectEmpty( &top_rect );
    set_visible_region( dce->hdc, vis_rgn, &win_rect, &top_rect, surface );
    if (surface) window_surface_release( surface );
}

/***********************************************************************
 *           release_dce
 */
static void release_dce( struct dce *dce )
{
    if (!dce->hwnd) return;  /* already released */

    set_visible_region( dce->hdc, 0, &dummy_surface.rect, &dummy_surface.rect, &dummy_surface );
    user_driver->pReleaseDC( dce->hwnd, dce->hdc );

    if (dce->clip_rgn) NtGdiDeleteObjectApp( dce->clip_rgn );
    dce->clip_rgn = 0;
    dce->hwnd     = 0;
    dce->flags   &= DCX_CACHE;
}

/***********************************************************************
 *           delete_clip_rgn
 */
static void delete_clip_rgn( struct dce *dce )
{
    if (!dce->clip_rgn) return;  /* nothing to do */

    dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
    NtGdiDeleteObjectApp( dce->clip_rgn );
    dce->clip_rgn = 0;

    /* make it dirty so that the vis rgn gets recomputed next time */
    set_dce_flags( dce->hdc, DCHF_INVALIDATEVISRGN );
}

/***********************************************************************
 *           delete_dce
 */
BOOL delete_dce( struct dce *dce )
{
    BOOL ret = TRUE;

    TRACE( "hdc = %p\n", dce->hdc );

    user_lock();
    if (!(dce->flags & DCX_CACHE))
    {
        WARN("Application trying to delete an owned DC %p\n", dce->hdc);
        ret = FALSE;
    }
    else
    {
        list_remove( &dce->entry );
        if (dce->clip_rgn) NtGdiDeleteObjectApp( dce->clip_rgn );
        free( dce );
    }
    user_unlock();
    return ret;
}

/***********************************************************************
 *           update_dc
 *
 * Make sure the DC vis region is up to date.
 * This function may need user lock so the GDI lock should _not_
 * be held when calling it.
 */
void update_dc( DC *dc )
{
    if (!dc->dirty) return;
    dc->dirty = 0;
    if (dc->dce)
    {
        if (dc->dce->count) update_visible_region( dc->dce );
        else /* non-fatal but shouldn't happen */
            WARN("DC is not in use!\n");
    }
}

/***********************************************************************
 *           alloc_dce
 *
 * Allocate a new DCE.
 */
static struct dce *alloc_dce(void)
{
    struct dce *dce;

    if (!(dce = malloc( sizeof(*dce) ))) return NULL;
    if (!(dce->hdc = NtGdiOpenDCW( NULL, NULL, NULL, 0, TRUE, 0, NULL, NULL )))
    {
        free( dce );
        return 0;
    }
    dce->hwnd      = 0;
    dce->clip_rgn  = 0;
    dce->flags     = 0;
    dce->count     = 1;

    set_dc_dce( dce->hdc, dce );
    return dce;
}

/***********************************************************************
 *           get_window_dce
 */
static struct dce *get_window_dce( HWND hwnd )
{
    struct dce *dce;
    WND *win = get_win_ptr( hwnd );

    if (!win || win == WND_OTHER_PROCESS || win == WND_DESKTOP) return NULL;

    dce = win->dce;
    if (!dce && (dce = get_class_dce( win->class )))
    {
        win->dce = dce;
        dce->count++;
    }
    release_win_ptr( win );

    if (!dce)  /* try to allocate one */
    {
        struct dce *dce_to_free = NULL;
        LONG class_style = get_class_long( hwnd, GCL_STYLE, FALSE );

        if (class_style & CS_CLASSDC)
        {
            if (!(dce = alloc_dce())) return NULL;

            win = get_win_ptr( hwnd );
            if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
            {
                if (win->dce)  /* another thread beat us to it */
                {
                    dce_to_free = dce;
                    dce = win->dce;
                }
                else if ((win->dce = set_class_dce( win->class, dce )) != dce)
                {
                    dce_to_free = dce;
                    dce = win->dce;
                    dce->count++;
                }
                else
                {
                    dce->count++;
                    list_add_tail( &dce_list, &dce->entry );
                }
                release_win_ptr( win );
            }
            else dce_to_free = dce;
        }
        else if (class_style & CS_OWNDC)
        {
            if (!(dce = alloc_dce())) return NULL;

            win = get_win_ptr( hwnd );
            if (win && win != WND_OTHER_PROCESS && win != WND_DESKTOP)
            {
                if (win->dwStyle & WS_CLIPCHILDREN) dce->flags |= DCX_CLIPCHILDREN;
                if (win->dwStyle & WS_CLIPSIBLINGS) dce->flags |= DCX_CLIPSIBLINGS;
                if (win->dce)  /* another thread beat us to it */
                {
                    dce_to_free = dce;
                    dce = win->dce;
                }
                else
                {
                    win->dce = dce;
                    dce->hwnd = hwnd;
                    list_add_tail( &dce_list, &dce->entry );
                }
                release_win_ptr( win );
            }
            else dce_to_free = dce;
        }

        if (dce_to_free)
        {
            set_dc_dce( dce_to_free->hdc, NULL );
            NtGdiDeleteObjectApp( dce_to_free->hdc );
            free( dce_to_free );
            if (dce_to_free == dce)
                dce = NULL;
        }
    }
    return dce;
}

/***********************************************************************
 *           free_dce
 *
 * Free a class or window DCE.
 */
void free_dce( struct dce *dce, HWND hwnd )
{
    struct dce *dce_to_free = NULL;

    user_lock();

    if (dce)
    {
        if (!--dce->count)
        {
            release_dce( dce );
            list_remove( &dce->entry );
            dce_to_free = dce;
        }
        else if (dce->hwnd == hwnd)
        {
            release_dce( dce );
        }
    }

    /* now check for cache DCEs */

    if (hwnd)
    {
        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if (dce->hwnd != hwnd) continue;
            if (!(dce->flags & DCX_CACHE)) break;

            release_dce( dce );
            if (dce->count)
            {
                WARN( "GetDC() without ReleaseDC() for window %p\n", hwnd );
                dce->count = 0;
                set_dce_flags( dce->hdc, DCHF_DISABLEDC );
            }
        }
    }

    user_unlock();

    if (dce_to_free)
    {
        set_dc_dce( dce_to_free->hdc, NULL );
        NtGdiDeleteObjectApp( dce_to_free->hdc );
        free( dce_to_free );
    }
}

/***********************************************************************
 *           make_dc_dirty
 *
 * Mark the associated DC as dirty to force a refresh of the visible region
 */
static void make_dc_dirty( struct dce *dce )
{
    if (!dce->count)
    {
        /* Don't bother with visible regions of unused DCEs */
        TRACE("purged %p hwnd %p\n", dce->hdc, dce->hwnd);
        release_dce( dce );
    }
    else
    {
        /* Set dirty bits in the hDC and DCE structs */
        TRACE("fixed up %p hwnd %p\n", dce->hdc, dce->hwnd);
        set_dce_flags( dce->hdc, DCHF_INVALIDATEVISRGN );
    }
}

/***********************************************************************
 *           invalidate_dce
 *
 * It is called from SetWindowPos() - we have to
 * mark as dirty all busy DCEs for windows that have pWnd->parent as
 * an ancestor and whose client rect intersects with specified update
 * rectangle. In addition, pWnd->parent DCEs may need to be updated if
 * DCX_CLIPCHILDREN flag is set.
 */
void invalidate_dce( WND *win, const RECT *extra_rect )
{
    DPI_AWARENESS_CONTEXT context;
    RECT window_rect;
    struct dce *dce;

    if (!win->parent) return;

    context = SetThreadDpiAwarenessContext( get_window_dpi_awareness_context( win->obj.handle ));
    get_window_rect( win->obj.handle, &window_rect, get_thread_dpi() );

    TRACE("%p parent %p %s (%s)\n",
          win->obj.handle, win->parent, wine_dbgstr_rect(&window_rect), wine_dbgstr_rect(extra_rect) );

    /* walk all DCEs and fixup non-empty entries */

    LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
    {
        if (!dce->hwnd) continue;

        TRACE( "%p: hwnd %p dcx %08x %s %s\n", dce->hdc, dce->hwnd, dce->flags,
               (dce->flags & DCX_CACHE) ? "Cache" : "Owned", dce->count ? "InUse" : "" );

        if ((dce->hwnd == win->parent) && !(dce->flags & DCX_CLIPCHILDREN))
            continue;  /* child window positions don't bother us */

        /* if DCE window is a child of hwnd, it has to be invalidated */
        if (dce->hwnd == win->obj.handle || is_child( win->obj.handle, dce->hwnd ))
        {
            make_dc_dirty( dce );
            continue;
        }

        /* otherwise check if the window rectangle intersects this DCE window */
        if (win->parent == dce->hwnd || is_child( win->parent, dce->hwnd ))
        {
            RECT dce_rect, tmp;
            get_window_rect( dce->hwnd, &dce_rect, get_thread_dpi() );
            if (intersect_rect( &tmp, &dce_rect, &window_rect ) ||
                (extra_rect && intersect_rect( &tmp, &dce_rect, extra_rect )))
                make_dc_dirty( dce );
        }
    }
    SetThreadDpiAwarenessContext( context );
}

/***********************************************************************
 *           release_dc
 */
static INT release_dc( HWND hwnd, HDC hdc, BOOL end_paint )
{
    struct dce *dce;
    BOOL ret = FALSE;

    TRACE( "%p %p\n", hwnd, hdc );

    user_lock();
    dce = get_dc_dce( hdc );
    if (dce && dce->count && dce->hwnd)
    {
        if (!(dce->flags & DCX_NORESETATTRS)) set_dce_flags( dce->hdc, DCHF_RESETDC );
        if (end_paint || (dce->flags & DCX_CACHE)) delete_clip_rgn( dce );
        if (dce->flags & DCX_CACHE)
        {
            dce->count = 0;
            set_dce_flags( dce->hdc, DCHF_DISABLEDC );
        }
        ret = TRUE;
    }
    user_unlock();
    return ret;
}

/***********************************************************************
 *           NtUserGetDCEx (win32u.@)
 */
HDC WINAPI NtUserGetDCEx( HWND hwnd, HRGN clip_rgn, DWORD flags )
{
    const DWORD clip_flags = DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW;
    const DWORD user_flags = clip_flags | DCX_NORESETATTRS; /* flags that can be set by user */
    BOOL update_vis_rgn = TRUE;
    struct dce *dce;
    HWND parent;
    LONG window_style = get_window_long( hwnd, GWL_STYLE );

    if (!hwnd) hwnd = get_desktop_window();
    else hwnd = get_full_window_handle( hwnd );

    TRACE( "hwnd %p, clip_rgn %p, flags %08x\n", hwnd, clip_rgn, (int)flags );

    if (!is_window(hwnd)) return 0;

    /* fixup flags */

    if (flags & (DCX_WINDOW | DCX_PARENTCLIP)) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
        flags &= ~(DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if (window_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;

        if (!(flags & DCX_WINDOW))
        {
            if (get_class_long( hwnd, GCL_STYLE, FALSE ) & CS_PARENTDC) flags |= DCX_PARENTCLIP;

            if (window_style & WS_CLIPCHILDREN && !(window_style & WS_MINIMIZE))
                flags |= DCX_CLIPCHILDREN;
        }
    }

    if (flags & DCX_WINDOW) flags &= ~DCX_CLIPCHILDREN;

    parent = NtUserGetAncestor( hwnd, GA_PARENT );
    if (!parent || (parent == get_desktop_window()))
        flags = (flags & ~DCX_PARENTCLIP) | DCX_CLIPSIBLINGS;

    /* it seems parent clip is ignored when clipping siblings or children */
    if (flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) flags &= ~DCX_PARENTCLIP;

    if( flags & DCX_PARENTCLIP )
    {
        LONG parent_style = get_window_long( parent, GWL_STYLE );
        if( (window_style & WS_VISIBLE) && (parent_style & WS_VISIBLE) )
        {
            flags &= ~DCX_CLIPCHILDREN;
            if (parent_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
        }
    }

    /* find a suitable DCE */

    if ((flags & DCX_CACHE) || !(dce = get_window_dce( hwnd )))
    {
        struct dce *dceEmpty = NULL, *dceUnused = NULL, *found = NULL;
        unsigned int count = 0;

        /* Strategy: First, we attempt to find a non-empty but unused DCE with
         * compatible flags. Next, we look for an empty entry. If the cache is
         * full we have to purge one of the unused entries.
         */
        user_lock();
        LIST_FOR_EACH_ENTRY( dce, &dce_list, struct dce, entry )
        {
            if (!(dce->flags & DCX_CACHE)) break;
            count++;
            if (dce->count) continue;
            dceUnused = dce;
            if (!dce->hwnd) dceEmpty = dce;
            else if ((dce->hwnd == hwnd) && !((dce->flags ^ flags) & clip_flags))
            {
                TRACE( "found valid %p hwnd %p, flags %08x\n", dce->hdc, hwnd, dce->flags );
                found = dce;
                update_vis_rgn = FALSE;
                break;
            }
        }
        if (!found) found = dceEmpty;
        if (!found && count >= DCE_CACHE_SIZE) found = dceUnused;

        dce = found;
        if (dce)
        {
            dce->count = 1;
            set_dce_flags( dce->hdc, DCHF_ENABLEDC );
        }
        user_unlock();

        /* if there's no dce empty or unused, allocate a new one */
        if (!dce)
        {
            if (!(dce = alloc_dce())) return 0;
            dce->flags = DCX_CACHE;
            user_lock();
            list_add_head( &dce_list, &dce->entry );
            user_unlock();
        }
    }
    else
    {
        flags |= DCX_NORESETATTRS;
        if (dce->hwnd != hwnd)
        {
            /* we should free dce->clip_rgn here, but Windows apparently doesn't */
            dce->flags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN);
            dce->clip_rgn = 0;
        }
        else update_vis_rgn = FALSE; /* updated automatically, via DCHook() */
    }

    if (flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))
    {
        /* if the extra clip region has changed, get rid of the old one */
        if (dce->clip_rgn != clip_rgn || ((flags ^ dce->flags) & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)))
            delete_clip_rgn( dce );
        dce->clip_rgn = clip_rgn;
        if (!dce->clip_rgn) dce->clip_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        dce->flags |= flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN);
        update_vis_rgn = TRUE;
    }

    if (get_window_long( hwnd, GWL_EXSTYLE ) & WS_EX_LAYOUTRTL)
        NtGdiSetLayout( dce->hdc, -1, LAYOUT_RTL );

    dce->hwnd = hwnd;
    dce->flags = (dce->flags & ~user_flags) | (flags & user_flags);

    /* cross-process invalidation is not supported yet, so always update the vis rgn */
    if (!is_current_process_window( hwnd )) update_vis_rgn = TRUE;

    if (set_dce_flags( dce->hdc, DCHF_VALIDATEVISRGN )) update_vis_rgn = TRUE;  /* DC was dirty */

    if (update_vis_rgn) update_visible_region( dce );

    TRACE( "(%p,%p,0x%x): returning %p%s\n", hwnd, clip_rgn, (int)flags, dce->hdc,
           update_vis_rgn ? " (updated)" : "" );
    return dce->hdc;
}

/***********************************************************************
 *           NtUserReleaseDC (win32u.@)
 */
INT WINAPI NtUserReleaseDC( HWND hwnd, HDC hdc )
{
    return release_dc( hwnd, hdc, FALSE );
}

/***********************************************************************
 *           NtUserGetDC (win32u.@)
 */
HDC WINAPI NtUserGetDC( HWND hwnd )
{
    if (!hwnd) return NtUserGetDCEx( 0, 0, DCX_CACHE | DCX_WINDOW );
    return NtUserGetDCEx( hwnd, 0, DCX_USESTYLE );
}

/***********************************************************************
 *           NtUserGetWindowDC (win32u.@)
 */
HDC WINAPI NtUserGetWindowDC( HWND hwnd )
{
    return NtUserGetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}

/**********************************************************************
 *           NtUserWindowFromDC (win32u.@)
 */
HWND WINAPI NtUserWindowFromDC( HDC hdc )
{
    struct dce *dce;
    HWND hwnd = 0;

    user_lock();
    dce = get_dc_dce( hdc );
    if (dce) hwnd = dce->hwnd;
    user_unlock();
    return hwnd;
}

/***********************************************************************
 *           get_update_region
 *
 * Return update region (in screen coordinates) for a window.
 */
static HRGN get_update_region( HWND hwnd, UINT *flags, HWND *child )
{
    HRGN hrgn = 0;
    NTSTATUS status;
    RGNDATA *data;
    size_t size = 256;

    do
    {
        if (!(data = malloc( sizeof(*data) + size - 1 )))
        {
            RtlSetLastWin32Error( ERROR_OUTOFMEMORY );
            return 0;
        }

        SERVER_START_REQ( get_update_region )
        {
            req->window     = wine_server_user_handle( hwnd );
            req->from_child = wine_server_user_handle( child ? *child : 0 );
            req->flags      = *flags;
            wine_server_set_reply( req, data->Buffer, size );
            if (!(status = wine_server_call( req )))
            {
                size_t reply_size = wine_server_reply_size( reply );
                data->rdh.dwSize   = sizeof(data->rdh);
                data->rdh.iType    = RDH_RECTANGLES;
                data->rdh.nCount   = reply_size / sizeof(RECT);
                data->rdh.nRgnSize = reply_size;
                hrgn = NtGdiExtCreateRegion( NULL, data->rdh.dwSize + data->rdh.nRgnSize, data );
                if (child) *child = wine_server_ptr_handle( reply->child );
                *flags = reply->flags;
            }
            else size = reply->total_size;
        }
        SERVER_END_REQ;
        free( data );
    } while (status == STATUS_BUFFER_OVERFLOW);

    if (status) RtlSetLastWin32Error( RtlNtStatusToDosError(status) );
    return hrgn;
}

/***********************************************************************
 *           redraw_window_rects
 *
 * Redraw part of a window.
 */
static BOOL redraw_window_rects( HWND hwnd, UINT flags, const RECT *rects, UINT count )
{
    BOOL ret;

    if (!(flags & (RDW_INVALIDATE|RDW_VALIDATE|RDW_INTERNALPAINT|RDW_NOINTERNALPAINT)))
        return TRUE;  /* nothing to do */

    SERVER_START_REQ( redraw_window )
    {
        req->window = wine_server_user_handle( hwnd );
        req->flags  = flags;
        wine_server_add_data( req, rects, count * sizeof(RECT) );
        ret = !wine_server_call_err( req );
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           get_update_flags
 *
 * Get only the update flags, not the update region.
 */
static BOOL get_update_flags( HWND hwnd, HWND *child, UINT *flags )
{
    BOOL ret;

    SERVER_START_REQ( get_update_region )
    {
        req->window     = wine_server_user_handle( hwnd );
        req->from_child = wine_server_user_handle( child ? *child : 0 );
        req->flags      = *flags | UPDATE_NOREGION;
        if ((ret = !wine_server_call_err( req )))
        {
            if (child) *child = wine_server_ptr_handle( reply->child );
            *flags = reply->flags;
        }
    }
    SERVER_END_REQ;
    return ret;
}

/***********************************************************************
 *           send_ncpaint
 *
 * Send a WM_NCPAINT message if needed, and return the resulting update region (in screen coords).
 * Helper for erase_now and BeginPaint.
 */
static HRGN send_ncpaint( HWND hwnd, HWND *child, UINT *flags )
{
    HRGN whole_rgn = get_update_region( hwnd, flags, child );
    HRGN client_rgn = 0;
    DWORD style;

    if (child) hwnd = *child;

    if (hwnd == get_desktop_window()) return whole_rgn;

    if (whole_rgn)
    {
        DPI_AWARENESS_CONTEXT context;
        RECT client, window, update;
        INT type;

        context = SetThreadDpiAwarenessContext( get_window_dpi_awareness_context( hwnd ));

        /* check if update rgn overlaps with nonclient area */
        type = NtGdiGetRgnBox( whole_rgn, &update );
        get_window_rects( hwnd, COORDS_SCREEN, &window, &client, get_thread_dpi() );

        if ((*flags & UPDATE_NONCLIENT) ||
            update.left < client.left || update.top < client.top ||
            update.right > client.right || update.bottom > client.bottom)
        {
            client_rgn = NtGdiCreateRectRgn( client.left, client.top, client.right, client.bottom );
            NtGdiCombineRgn( client_rgn, client_rgn, whole_rgn, RGN_AND );

            /* check if update rgn contains complete nonclient area */
            if (type == SIMPLEREGION && EqualRect( &window, &update ))
            {
                NtGdiDeleteObjectApp( whole_rgn );
                whole_rgn = (HRGN)1;
            }
        }
        else
        {
            client_rgn = whole_rgn;
            whole_rgn = 0;
        }

        if (whole_rgn) /* NOTE: WM_NCPAINT allows wParam to be 1 */
        {
            if (*flags & UPDATE_NONCLIENT)
            {
                /* Mark standard scroll bars as not painted before sending WM_NCPAINT */
                style = get_window_long( hwnd, GWL_STYLE );
                if (style & WS_HSCROLL)
                    set_standard_scroll_painted( hwnd, SB_HORZ, FALSE );
                if (style & WS_VSCROLL)
                    set_standard_scroll_painted( hwnd, SB_VERT, FALSE );

                send_message( hwnd, WM_NCPAINT, (WPARAM)whole_rgn, 0 );
            }
            if (whole_rgn > (HRGN)1) NtGdiDeleteObjectApp( whole_rgn );
        }
        SetThreadDpiAwarenessContext( context );
    }
    return client_rgn;
}

/***********************************************************************
 *           send_erase
 *
 * Send a WM_ERASEBKGND message if needed, and optionally return the DC for painting.
 * If a DC is requested, the region is selected into it. In all cases the region is deleted.
 * Helper for erase_now and BeginPaint.
 */
static BOOL send_erase( HWND hwnd, UINT flags, HRGN client_rgn,
                        RECT *clip_rect, HDC *hdc_ret )
{
    BOOL need_erase = (flags & UPDATE_DELAYED_ERASE) != 0;
    HDC hdc = 0;
    RECT dummy;

    if (!clip_rect) clip_rect = &dummy;
    if (hdc_ret || (flags & UPDATE_ERASE))
    {
        UINT dcx_flags = DCX_INTERSECTRGN | DCX_USESTYLE;
        if (is_iconic(hwnd)) dcx_flags |= DCX_WINDOW;

        if ((hdc = NtUserGetDCEx( hwnd, client_rgn, dcx_flags )))
        {
            INT type = NtGdiGetAppClipBox( hdc, clip_rect );

            if (flags & UPDATE_ERASE)
            {
                /* don't erase if the clip box is empty */
                if (type != NULLREGION)
                    need_erase = !send_message( hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0 );
            }
            if (!hdc_ret) release_dc( hwnd, hdc, TRUE );
        }

        if (hdc_ret) *hdc_ret = hdc;
    }
    if (!hdc) NtGdiDeleteObjectApp( client_rgn );
    return need_erase;
}

/***********************************************************************
 *           copy_bits_from_surface
 *
 * Copy bits from a window surface; helper for move_window_bits and move_window_bits_parent.
 */
static void copy_bits_from_surface( HWND hwnd, struct window_surface *surface,
                                    const RECT *dst, const RECT *src )
{
    char buffer[FIELD_OFFSET( BITMAPINFO, bmiColors[256] )];
    BITMAPINFO *info = (BITMAPINFO *)buffer;
    void *bits;
    UINT flags = UPDATE_NOCHILDREN | UPDATE_CLIPCHILDREN;
    HRGN rgn = get_update_region( hwnd, &flags, NULL );
    HDC hdc = NtUserGetDCEx( hwnd, rgn, DCX_CACHE | DCX_WINDOW | DCX_EXCLUDERGN );

    bits = surface->funcs->get_info( surface, info );
    surface->funcs->lock( surface );
    NtGdiSetDIBitsToDeviceInternal( hdc, dst->left, dst->top, dst->right - dst->left, dst->bottom - dst->top,
                                    src->left - surface->rect.left, surface->rect.bottom - src->bottom,
                                    0, surface->rect.bottom - surface->rect.top,
                                    bits, info, DIB_RGB_COLORS, 0, 0, FALSE, NULL );
    surface->funcs->unlock( surface );
    NtUserReleaseDC( hwnd, hdc );
}

/***********************************************************************
 *           move_window_bits
 *
 * Move the window bits when a window is resized or its surface recreated.
 */
void move_window_bits( HWND hwnd, struct window_surface *old_surface,
                       struct window_surface *new_surface,
                       const RECT *visible_rect, const RECT *old_visible_rect,
                       const RECT *window_rect, const RECT *valid_rects )
{
    RECT dst = valid_rects[0];
    RECT src = valid_rects[1];

    if (new_surface != old_surface ||
        src.left - old_visible_rect->left != dst.left - visible_rect->left ||
        src.top - old_visible_rect->top != dst.top - visible_rect->top)
    {
        TRACE( "copying %s -> %s\n", wine_dbgstr_rect( &src ), wine_dbgstr_rect( &dst ));
        OffsetRect( &src, -old_visible_rect->left, -old_visible_rect->top );
        OffsetRect( &dst, -window_rect->left, -window_rect->top );
        copy_bits_from_surface( hwnd, old_surface, &dst, &src );
    }
}


/***********************************************************************
 *           move_window_bits_parent
 *
 * Move the window bits in the parent surface when a child is moved.
 */
void move_window_bits_parent( HWND hwnd, HWND parent, const RECT *window_rect, const RECT *valid_rects )
{
    struct window_surface *surface;
    RECT dst = valid_rects[0];
    RECT src = valid_rects[1];
    WND *win;

    if (src.left == dst.left && src.top == dst.top) return;

    if (!(win = get_win_ptr( parent ))) return;
    if (win == WND_DESKTOP || win == WND_OTHER_PROCESS) return;
    if (!(surface = win->surface))
    {
        release_win_ptr( win );
        return;
    }

    TRACE( "copying %s -> %s\n", wine_dbgstr_rect( &src ), wine_dbgstr_rect( &dst ));
    map_window_points( NtUserGetAncestor( hwnd, GA_PARENT ), parent, (POINT *)&src, 2, get_thread_dpi() );
    OffsetRect( &src, win->client_rect.left - win->visible_rect.left,
                win->client_rect.top - win->visible_rect.top );
    OffsetRect( &dst, -window_rect->left, -window_rect->top );
    window_surface_add_ref( surface );
    release_win_ptr( win );

    copy_bits_from_surface( hwnd, surface, &dst, &src );
    window_surface_release( surface );
}

/***********************************************************************
 *           NtUserBeginPaint (win32u.@)
 */
HDC WINAPI NtUserBeginPaint( HWND hwnd, PAINTSTRUCT *ps )
{
    HRGN hrgn;
    HDC hdc;
    BOOL erase;
    RECT rect;
    UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE | UPDATE_PAINT | UPDATE_INTERNALPAINT | UPDATE_NOCHILDREN;

    NtUserHideCaret( hwnd );

    if (!(hrgn = send_ncpaint( hwnd, NULL, &flags ))) return 0;

    erase = send_erase( hwnd, flags, hrgn, &rect, &hdc );

    TRACE( "hdc = %p box = (%s), fErase = %d\n", hdc, wine_dbgstr_rect(&rect), erase );

    if (!ps)
    {
        release_dc( hwnd, hdc, TRUE );
        return 0;
    }
    ps->fErase = erase;
    ps->rcPaint = rect;
    ps->hdc = hdc;
    return hdc;
}

/***********************************************************************
 *           NtUserEndPaint (win32u.@)
 */
BOOL WINAPI NtUserEndPaint( HWND hwnd, const PAINTSTRUCT *ps )
{
    NtUserShowCaret( hwnd );
    flush_window_surfaces( FALSE );
    if (!ps) return FALSE;
    release_dc( hwnd, ps->hdc, TRUE );
    return TRUE;
}

/***********************************************************************
 *           erase_now
 *
 * Implementation of RDW_ERASENOW behavior.
 */
void erase_now( HWND hwnd, UINT rdw_flags )
{
    HWND child = 0;
    HRGN hrgn;
    BOOL need_erase = FALSE;

    /* loop while we find a child to repaint */
    for (;;)
    {
        UINT flags = UPDATE_NONCLIENT | UPDATE_ERASE;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;
        if (need_erase) flags |= UPDATE_DELAYED_ERASE;

        if (!(hrgn = send_ncpaint( hwnd, &child, &flags ))) break;
        need_erase = send_erase( child, flags, hrgn, NULL, NULL );

        if (!flags) break;  /* nothing more to do */
        if ((rdw_flags & RDW_NOCHILDREN) && !need_erase) break;
    }
}

/***********************************************************************
 *           update_now
 *
 * Implementation of RDW_UPDATENOW behavior.
 */
static void update_now( HWND hwnd, UINT rdw_flags )
{
    HWND child = 0;

    /* desktop window never gets WM_PAINT, only WM_ERASEBKGND */
    if (hwnd == get_desktop_window()) erase_now( hwnd, rdw_flags | RDW_NOCHILDREN );

    /* loop while we find a child to repaint */
    for (;;)
    {
        UINT flags = UPDATE_PAINT | UPDATE_INTERNALPAINT;

        if (rdw_flags & RDW_NOCHILDREN) flags |= UPDATE_NOCHILDREN;
        else if (rdw_flags & RDW_ALLCHILDREN) flags |= UPDATE_ALLCHILDREN;

        if (!get_update_flags( hwnd, &child, &flags )) break;
        if (!flags) break;  /* nothing more to do */

        send_message( child, WM_PAINT, 0, 0 );
        if (rdw_flags & RDW_NOCHILDREN) break;
    }
}

/***********************************************************************
 *           NtUserRedrawWindow (win32u.@)
 */
BOOL WINAPI NtUserRedrawWindow( HWND hwnd, const RECT *rect, HRGN hrgn, UINT flags )
{
    static const RECT empty;
    BOOL ret;

    if (TRACE_ON(win))
    {
        if (hrgn)
        {
            RECT r;
            NtGdiGetRgnBox( hrgn, &r );
            TRACE( "%p region %p box %s ", hwnd, hrgn, wine_dbgstr_rect(&r) );
        }
        else if (rect)
            TRACE( "%p rect %s ", hwnd, wine_dbgstr_rect(rect) );
        else
            TRACE( "%p whole window ", hwnd );

        dump_rdw_flags(flags);
    }

    /* process pending expose events before painting */
    if (flags & RDW_UPDATENOW) user_driver->pProcessEvents( QS_PAINT );

    if (rect && !hrgn)
    {
        RECT ordered = *rect;

        order_rect( &ordered );
        if (IsRectEmpty( &ordered )) ordered = empty;
        ret = redraw_window_rects( hwnd, flags, &ordered, 1 );
    }
    else if (!hrgn)
    {
        ret = redraw_window_rects( hwnd, flags, NULL, 0 );
    }
    else  /* need to build a list of the region rectangles */
    {
        DWORD size;
        RGNDATA *data;

        if (!(size = NtGdiGetRegionData( hrgn, 0, NULL ))) return FALSE;
        if (!(data = malloc( size ))) return FALSE;
        NtGdiGetRegionData( hrgn, size, data );
        if (!data->rdh.nCount)  /* empty region -> use a single all-zero rectangle */
            ret = redraw_window_rects( hwnd, flags, &empty, 1 );
        else
            ret = redraw_window_rects( hwnd, flags, (const RECT *)data->Buffer, data->rdh.nCount );
        free( data );
    }

    if (!hwnd) hwnd = get_desktop_window();

    if (flags & RDW_UPDATENOW) update_now( hwnd, flags );
    else if (flags & RDW_ERASENOW) erase_now( hwnd, flags );

    return ret;
}

/***********************************************************************
 *           NtUserValidateRect (win32u.@)
 */
BOOL WINAPI NtUserValidateRect( HWND hwnd, const RECT *rect )
{
    UINT flags = RDW_VALIDATE;

    if (!hwnd)
    {
        flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
        rect = NULL;
    }

    return NtUserRedrawWindow( hwnd, rect, 0, flags );
}

/***********************************************************************
 *           NtUserGetUpdateRgn (win32u.@)
 */
INT WINAPI NtUserGetUpdateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    DPI_AWARENESS_CONTEXT context;
    INT retval = ERROR;
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;

    context = SetThreadDpiAwarenessContext( get_window_dpi_awareness_context( hwnd ));

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if ((update_rgn = send_ncpaint( hwnd, NULL, &flags )))
    {
        retval = NtGdiCombineRgn( hrgn, update_rgn, 0, RGN_COPY );
        if (send_erase( hwnd, flags, update_rgn, NULL, NULL ))
        {
            flags = UPDATE_DELAYED_ERASE;
            get_update_flags( hwnd, NULL, &flags );
        }
        /* map region to client coordinates */
        map_window_region( 0, hwnd, hrgn );
    }
    SetThreadDpiAwarenessContext( context );
    return retval;
}

/***********************************************************************
 *           NtUserGetUpdateRect (win32u.@)
 */
BOOL WINAPI NtUserGetUpdateRect( HWND hwnd, RECT *rect, BOOL erase )
{
    UINT flags = UPDATE_NOCHILDREN;
    HRGN update_rgn;
    BOOL need_erase;

    if (erase) flags |= UPDATE_NONCLIENT | UPDATE_ERASE;

    if (!(update_rgn = send_ncpaint( hwnd, NULL, &flags ))) return FALSE;

    if (rect && NtGdiGetRgnBox( update_rgn, rect ) != NULLREGION)
    {
        HDC hdc = NtUserGetDCEx( hwnd, 0, DCX_USESTYLE );
        DWORD layout = NtGdiSetLayout( hdc, -1, 0 );  /* map_window_points mirrors already */
        UINT win_dpi = get_dpi_for_window( hwnd );
        map_window_points( 0, hwnd, (POINT *)rect, 2, win_dpi );
        *rect = map_dpi_rect( *rect, win_dpi, get_thread_dpi() );
        NtGdiTransformPoints( hdc, (POINT *)rect, (POINT *)rect, 2, NtGdiDPtoLP );
        NtGdiSetLayout( hdc, -1, layout );
        NtUserReleaseDC( hwnd, hdc );
    }
    need_erase = send_erase( hwnd, flags, update_rgn, NULL, NULL );

    /* check if we still have an update region */
    flags = UPDATE_PAINT | UPDATE_NOCHILDREN;
    if (need_erase) flags |= UPDATE_DELAYED_ERASE;
    return get_update_flags( hwnd, NULL, &flags ) && (flags & UPDATE_PAINT);
}

/***********************************************************************
 *           NtUserExcludeUpdateRgn (win32u.@)
 */
INT WINAPI NtUserExcludeUpdateRgn( HDC hdc, HWND hwnd )
{
    HRGN update_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
    INT ret = NtUserGetUpdateRgn( hwnd, update_rgn, FALSE );

    if (ret != ERROR)
    {
        DPI_AWARENESS_CONTEXT context;
        POINT pt;

        context = SetThreadDpiAwarenessContext( get_window_dpi_awareness_context( hwnd ));
        NtGdiGetDCPoint( hdc, NtGdiGetDCOrg, &pt );
        map_window_points( 0, hwnd, &pt, 1, get_thread_dpi() );
        NtGdiOffsetRgn( update_rgn, -pt.x, -pt.y );
        ret = NtGdiExtSelectClipRgn( hdc, update_rgn, RGN_DIFF );
        SetThreadDpiAwarenessContext( context );
    }
    NtGdiDeleteObjectApp( update_rgn );
    return ret;
}

/***********************************************************************
 *           NtUserInvalidateRgn (win32u.@)
 */
BOOL WINAPI NtUserInvalidateRgn( HWND hwnd, HRGN hrgn, BOOL erase )
{
    if (!hwnd)
    {
        RtlSetLastWin32Error( ERROR_INVALID_WINDOW_HANDLE );
        return FALSE;
    }

    return NtUserRedrawWindow( hwnd, NULL, hrgn, RDW_INVALIDATE | (erase ? RDW_ERASE : 0) );
}

/***********************************************************************
 *           NtUserInvalidateRect (win32u.@)
 */
BOOL WINAPI NtUserInvalidateRect( HWND hwnd, const RECT *rect, BOOL erase )
{
    UINT flags = RDW_INVALIDATE | (erase ? RDW_ERASE : 0);

    if (!hwnd)
    {
        flags = RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_FRAME | RDW_ERASE | RDW_ERASENOW;
        rect = NULL;
    }

    return NtUserRedrawWindow( hwnd, rect, 0, flags );
}

/***********************************************************************
 *           NtUserLockWindowUpdate (win32u.@)
 */
BOOL WINAPI NtUserLockWindowUpdate( HWND hwnd )
{
    static HWND locked_hwnd;

    FIXME( "(%p), partial stub!\n", hwnd );

    if (!hwnd)
    {
        locked_hwnd = NULL;
        return TRUE;
    }
    return !InterlockedCompareExchangePointer( (void **)&locked_hwnd, hwnd, 0 );
}

/*************************************************************************
 *             fix_caret
 *
 * Helper for NtUserScrollWindowEx:
 * If the return value is 0, no special caret handling is necessary.
 * Otherwise the return value is the handle of the window that owns the
 * caret. Its caret needs to be hidden during the scroll operation and
 * moved to new_caret_pos if move_caret is TRUE.
 */
static HWND fix_caret( HWND hwnd, const RECT *scroll_rect, INT dx, INT dy,
                       UINT flags, BOOL *move_caret, POINT *new_caret_pos )
{
    RECT rect, mapped_caret;
    GUITHREADINFO info;

    info.cbSize = sizeof(info);
    if (!NtUserGetGUIThreadInfo( GetCurrentThreadId(), &info )) return 0;
    if (!info.hwndCaret) return 0;

    mapped_caret = info.rcCaret;
    if (info.hwndCaret == hwnd)
    {
        /* The caret needs to be moved along with scrolling even if it's
         * outside the visible area. Otherwise, when the caret is scrolled
         * out from the view, the position won't get updated anymore and
         * the caret will never scroll back again. */
        *move_caret = TRUE;
        new_caret_pos->x = info.rcCaret.left + dx;
        new_caret_pos->y = info.rcCaret.top + dy;
    }
    else
    {
        *move_caret = FALSE;
        if (!(flags & SW_SCROLLCHILDREN) || !is_child( hwnd, info.hwndCaret ))
            return 0;
        map_window_points( info.hwndCaret, hwnd, (POINT *)&mapped_caret, 2, get_thread_dpi() );
    }

    /* If the caret is not in the src/dest rects, all is fine done. */
    if (!intersect_rect( &rect, scroll_rect, &mapped_caret ))
    {
        rect = *scroll_rect;
        OffsetRect( &rect, dx, dy );
        if (!intersect_rect( &rect, &rect, &mapped_caret ))
            return 0;
    }

    /* Indicate that the caret needs to be updated during the scrolling. */
    return info.hwndCaret;
}

/*************************************************************************
 *           NtUserScrollWindowEx (win32u.@)
 *
 * Note: contrary to what the doc says, pixels that are scrolled from the
 *      outside of clipRect to the inside are NOT painted.
 */
INT WINAPI NtUserScrollWindowEx( HWND hwnd, INT dx, INT dy, const RECT *rect,
                                 const RECT *clip_rect, HRGN update_rgn,
                                 RECT *update_rect, UINT flags )
{
    BOOL update = update_rect || update_rgn || flags & (SW_INVALIDATE | SW_ERASE);
    BOOL own_rgn = TRUE, move_caret = FALSE;
    HRGN temp_rgn, winupd_rgn = 0;
    INT retval = NULLREGION;
    HWND caret_hwnd = NULL;
    POINT new_caret_pos;
    RECT rc, cliprc;
    int rdw_flags;
    HDC hdc;

    TRACE( "%p, %d,%d update_rgn=%p update_rect = %p %s %04x\n",
           hwnd, dx, dy, update_rgn, update_rect, wine_dbgstr_rect(rect), flags );
    TRACE( "clip_rect = %s\n", wine_dbgstr_rect(clip_rect) );
    if (flags & ~(SW_SCROLLCHILDREN | SW_INVALIDATE | SW_ERASE | SW_NODCCACHE))
        FIXME( "some flags (%04x) are unhandled\n", flags );

    rdw_flags = (flags & SW_ERASE) && (flags & SW_INVALIDATE) ?
        RDW_INVALIDATE | RDW_ERASE  : RDW_INVALIDATE;

    if (!is_window_drawable( hwnd, TRUE )) return ERROR;
    hwnd = get_full_window_handle( hwnd );

    get_client_rect( hwnd, &rc );
    if (clip_rect) intersect_rect( &cliprc, &rc, clip_rect );
    else cliprc = rc;

    if (rect) intersect_rect( &rc, &rc, rect );
    if (update_rgn) own_rgn = FALSE;
    else if (update) update_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );

    new_caret_pos.x = new_caret_pos.y = 0;

    if (!IsRectEmpty( &cliprc ) && (dx || dy))
    {
        DWORD style = get_window_long( hwnd, GWL_STYLE );
        DWORD dcxflags = 0;

        caret_hwnd = fix_caret( hwnd, &rc, dx, dy, flags, &move_caret, &new_caret_pos );
        if (caret_hwnd) NtUserHideCaret( caret_hwnd );

        if (!(flags & SW_NODCCACHE)) dcxflags |= DCX_CACHE;
        if (style & WS_CLIPSIBLINGS) dcxflags |= DCX_CLIPSIBLINGS;
        if (get_class_long( hwnd, GCL_STYLE, FALSE ) & CS_PARENTDC) dcxflags |= DCX_PARENTCLIP;
        if (!(flags & SW_SCROLLCHILDREN) && (style & WS_CLIPCHILDREN))
            dcxflags |= DCX_CLIPCHILDREN;
        hdc = NtUserGetDCEx( hwnd, 0, dcxflags);
        if (hdc)
        {
            NtUserScrollDC( hdc, dx, dy, &rc, &cliprc, update_rgn, update_rect );
            NtUserReleaseDC( hwnd, hdc );
            if (!update) NtUserRedrawWindow( hwnd, NULL, update_rgn, rdw_flags );
        }

        /* If the windows has an update region, this must be scrolled as well.
         * Keep a copy in winupd_rgn to be added to hrngUpdate at the end. */
        temp_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0 );
        retval = NtUserGetUpdateRgn( hwnd, temp_rgn, FALSE );
        if (retval != NULLREGION)
        {
            HRGN clip_rgn = NtGdiCreateRectRgn( cliprc.left, cliprc.top,
                                                cliprc.right, cliprc.bottom );
            if (!own_rgn)
            {
                winupd_rgn = NtGdiCreateRectRgn( 0, 0, 0, 0);
                NtGdiCombineRgn( winupd_rgn, temp_rgn, 0, RGN_COPY);
            }
            NtGdiOffsetRgn( temp_rgn, dx, dy );
            NtGdiCombineRgn( temp_rgn, temp_rgn, clip_rgn, RGN_AND );
            if (!own_rgn) NtGdiCombineRgn( winupd_rgn, winupd_rgn, temp_rgn, RGN_OR );
            NtUserRedrawWindow( hwnd, NULL, temp_rgn, rdw_flags );

           /*
            * Catch the case where the scrolling amount exceeds the size of the
            * original window. This generated a second update area that is the
            * location where the original scrolled content would end up.
            * This second region is not returned by the ScrollDC and sets
            * ScrollWindowEx apart from just a ScrollDC.
            *
            * This has been verified with testing on windows.
            */
            if (abs( dx ) > abs( rc.right - rc.left ) || abs( dy ) > abs( rc.bottom - rc.top ))
            {
                NtGdiSetRectRgn( temp_rgn, rc.left + dx, rc.top + dy, rc.right+dx, rc.bottom + dy );
                NtGdiCombineRgn( temp_rgn, temp_rgn, clip_rgn, RGN_AND );
                NtGdiCombineRgn( update_rgn, update_rgn, temp_rgn, RGN_OR );

                if (update_rect)
                {
                    RECT temp_rect;
                    NtGdiGetRgnBox( temp_rgn, &temp_rect );
                    union_rect( update_rect, update_rect, &temp_rect );
                }

                if (!own_rgn) NtGdiCombineRgn( winupd_rgn, winupd_rgn, temp_rgn, RGN_OR );
            }
            NtGdiDeleteObjectApp( clip_rgn );
        }
        NtGdiDeleteObjectApp( temp_rgn );
    }
    else
    {
        /* nothing was scrolled */
        if (!own_rgn) NtGdiSetRectRgn( update_rgn, 0, 0, 0, 0 );
        SetRectEmpty( update_rect );
    }

    if (flags & SW_SCROLLCHILDREN)
    {
        HWND *list = list_window_children( 0, hwnd, NULL, 0 );
        if (list)
        {
            RECT r, dummy;
            int i;

            for (i = 0; list[i]; i++)
            {
                get_window_rects( list[i], COORDS_PARENT, &r, NULL, get_thread_dpi() );
                if (!rect || intersect_rect( &dummy, &r, rect ))
                    NtUserSetWindowPos( list[i], 0, r.left + dx, r.top  + dy, 0, 0,
                                        SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE |
                                        SWP_NOREDRAW | SWP_DEFERERASE );
            }
            free( list );
        }
    }

    if (flags & (SW_INVALIDATE | SW_ERASE))
        NtUserRedrawWindow( hwnd, NULL, update_rgn, rdw_flags |
                            ((flags & SW_SCROLLCHILDREN) ? RDW_ALLCHILDREN : 0 ) );

    if (winupd_rgn)
    {
        NtGdiCombineRgn( update_rgn, update_rgn, winupd_rgn, RGN_OR );
        NtGdiDeleteObjectApp( winupd_rgn );
    }

    if (move_caret) set_caret_pos( new_caret_pos.x, new_caret_pos.y );
    if (caret_hwnd) NtUserShowCaret( caret_hwnd );
    if (own_rgn && update_rgn) NtGdiDeleteObjectApp( update_rgn );

    return retval;
}

/************************************************************************
 *           NtUserPrintWindow (win32u.@)
 */
BOOL WINAPI NtUserPrintWindow( HWND hwnd, HDC hdc, UINT flags )
{
    UINT prf_flags = PRF_CHILDREN | PRF_ERASEBKGND | PRF_OWNED | PRF_CLIENT;
    if (!(flags & PW_CLIENTONLY)) prf_flags |= PRF_NONCLIENT;
    send_message( hwnd, WM_PRINT, (WPARAM)hdc, prf_flags );
    return TRUE;
}
