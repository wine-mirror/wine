/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 *
static char Copyright[] = "Copyright  Alexandre Julliard, 1993";
*/

#include "dce.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "user.h"
#include "sysmetrics.h"
#include "stddebug.h"
/* #define DEBUG_DC */
#include "debug.h"

#define NB_DCE    5  /* Number of DCEs created at startup */

static HANDLE firstDCE = 0;
static HDC defaultDCstate = 0;


/***********************************************************************
 *           DCE_AllocDCE
 *
 * Allocate a new DCE.
 */
HANDLE DCE_AllocDCE( DCE_TYPE type )
{
    DCE * dce;
    HANDLE handle = USER_HEAP_ALLOC( sizeof(DCE) );
    if (!handle) return 0;
    dce = (DCE *) USER_HEAP_LIN_ADDR( handle );
    if (!(dce->hdc = CreateDC( "DISPLAY", NULL, NULL, NULL )))
    {
	USER_HEAP_FREE( handle );
	return 0;
    }
    dce->hwndCurrent = 0;
    dce->type  = type;
    dce->inUse = (type != DCE_CACHE_DC);
    dce->xOrigin = 0;
    dce->yOrigin = 0;
    dce->hNext = firstDCE;
    firstDCE = handle;
    return handle;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
void DCE_FreeDCE( HANDLE hdce )
{
    DCE * dce;
    HANDLE *handle = &firstDCE;

    if (!(dce = (DCE *) USER_HEAP_LIN_ADDR( hdce ))) return;
    while (*handle && (*handle != hdce))
    {
	DCE * prev = (DCE *) USER_HEAP_LIN_ADDR( *handle );	
	handle = &prev->hNext;
    }
    if (*handle == hdce) *handle = dce->hNext;
    DeleteDC( dce->hdc );
    USER_HEAP_FREE( hdce );
}


/***********************************************************************
 *           DCE_Init
 */
void DCE_Init()
{
    int i;
    HANDLE handle;
    DCE * dce;
        
    for (i = 0; i < NB_DCE; i++)
    {
	if (!(handle = DCE_AllocDCE( DCE_CACHE_DC ))) return;
	dce = (DCE *) USER_HEAP_LIN_ADDR( handle );	
	if (!defaultDCstate) defaultDCstate = GetDCState( dce->hdc );
    }
}


/***********************************************************************
 *           DCE_GetVisRect
 *
 * Calc the visible rectangle of a window, i.e. the client or
 * window area clipped by the client area of all ancestors.
 * Return FALSE if the visible region is empty.
 */
static BOOL DCE_GetVisRect( WND *wndPtr, BOOL clientArea, RECT *lprect )
{
    int xoffset, yoffset;

    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;
    xoffset = lprect->left;
    yoffset = lprect->top;

    if (!(wndPtr->dwStyle & WS_VISIBLE) || (wndPtr->flags & WIN_NO_REDRAW))
    {
        SetRectEmpty( lprect );  /* Clip everything */
        return FALSE;
    }

    while (wndPtr->parent)
    {
        wndPtr = wndPtr->parent;
        if (!(wndPtr->dwStyle & WS_VISIBLE) ||
            (wndPtr->flags & WIN_NO_REDRAW) ||
            (wndPtr->dwStyle & WS_ICONIC))
        {
            SetRectEmpty( lprect );  /* Clip everything */
            return FALSE;
        }
	xoffset += wndPtr->rectClient.left;
	yoffset += wndPtr->rectClient.top;
	OffsetRect( lprect, wndPtr->rectClient.left,
		    wndPtr->rectClient.top );

	  /* Warning!! we assume that IntersectRect() handles the case */
	  /* where the destination is the same as one of the sources.  */
	if (!IntersectRect( lprect, lprect, &wndPtr->rectClient ))
            return FALSE;  /* Visible rectangle is empty */
    }
    OffsetRect( lprect, -xoffset, -yoffset );
    return TRUE;
}


/***********************************************************************
 *           DCE_ClipWindows
 *
 * Go through the linked list of windows from hwndStart to hwndEnd,
 * removing from the given region the rectangle of each window offset
 * by a given amount.  The new region is returned, and the original one
 * is destroyed.  Used to implement DCX_CLIPSIBLINGS and
 * DCX_CLIPCHILDREN styles.
 */
static HRGN DCE_ClipWindows( WND *pWndStart, WND *pWndEnd,
                             HRGN hrgn, int xoffset, int yoffset )
{
    HRGN hrgnNew;

    if (!pWndStart) return hrgn;
    if (!(hrgnNew = CreateRectRgn( 0, 0, 0, 0 )))
    {
        DeleteObject( hrgn );
        return 0;
    }
    for (; pWndStart != pWndEnd; pWndStart = pWndStart->next)
    {
        if (!(pWndStart->dwStyle & WS_VISIBLE)) continue;
        SetRectRgn( hrgnNew, pWndStart->rectWindow.left + xoffset,
                    pWndStart->rectWindow.top + yoffset,
                    pWndStart->rectWindow.right + xoffset,
                    pWndStart->rectWindow.bottom + yoffset );
        if (!CombineRgn( hrgn, hrgn, hrgnNew, RGN_DIFF )) break;
    }
    DeleteObject( hrgnNew );
    if (pWndStart != pWndEnd)  /* something went wrong */
    {
        DeleteObject( hrgn );
        return 0;
    }
    return hrgn;
}


/***********************************************************************
 *           DCE_GetVisRgn
 *
 * Return the visible region of a window, i.e. the client or window area
 * clipped by the client area of all ancestors, and then optionally
 * by siblings and children.
 */
HRGN DCE_GetVisRgn( HWND hwnd, WORD flags )
{
    RECT rect;
    HRGN hrgn;
    int xoffset, yoffset;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

      /* Get visible rectangle and create a region with it */

    if (!DCE_GetVisRect( wndPtr, !(flags & DCX_WINDOW), &rect ))
    {
        return CreateRectRgn( 0, 0, 0, 0 );  /* Visible region is empty */
    }
    if (!(hrgn = CreateRectRgnIndirect( &rect ))) return 0;

      /* Clip all children from the visible region */

    if (flags & DCX_CLIPCHILDREN)
    {
        if (flags & DCX_WINDOW)
        {
            xoffset = wndPtr->rectClient.left - wndPtr->rectWindow.left;
            yoffset = wndPtr->rectClient.top - wndPtr->rectWindow.top;
        }
        else xoffset = yoffset = 0;
        hrgn = DCE_ClipWindows( wndPtr->child, NULL, hrgn, xoffset, yoffset );
        if (!hrgn) return 0;
    }

      /* Clip siblings placed above this window */

    if (flags & DCX_WINDOW)
    {
        xoffset = -wndPtr->rectWindow.left;
        yoffset = -wndPtr->rectWindow.top;
    }
    else
    {
        xoffset = -wndPtr->rectClient.left;
        yoffset = -wndPtr->rectClient.top;
    }
    if (flags & DCX_CLIPSIBLINGS)
    {
        hrgn = DCE_ClipWindows( wndPtr->parent ? wndPtr->parent->child : NULL,
                                wndPtr, hrgn, xoffset, yoffset );
        if (!hrgn) return 0;
    }

      /* Clip siblings of all ancestors that have the WS_CLIPSIBLINGS style */

    while (wndPtr->dwStyle & WS_CHILD)
    {
        wndPtr = wndPtr->parent;
        xoffset -= wndPtr->rectClient.left;
        yoffset -= wndPtr->rectClient.top;
        hrgn = DCE_ClipWindows( wndPtr->parent->child, wndPtr,
                                hrgn, xoffset, yoffset );
        if (!hrgn) return 0;
    }
    return hrgn;
}


/***********************************************************************
 *           DCE_SetDrawable
 *
 * Set the drawable, origin and dimensions for the DC associated to
 * a given window.
 */
static void DCE_SetDrawable( WND *wndPtr, DC *dc, WORD flags )
{
    if (!wndPtr)  /* Get a DC for the whole screen */
    {
        dc->w.DCOrgX = 0;
        dc->w.DCOrgY = 0;
        dc->u.x.drawable = rootWindow;
        XSetSubwindowMode( display, dc->u.x.gc, IncludeInferiors );
    }
    else
    {
        if (flags & DCX_WINDOW)
        {
            dc->w.DCOrgX  = wndPtr->rectWindow.left;
            dc->w.DCOrgY  = wndPtr->rectWindow.top;
        }
        else
        {
            dc->w.DCOrgX  = wndPtr->rectClient.left;
            dc->w.DCOrgY  = wndPtr->rectClient.top;
        }
        while (!wndPtr->window)
        {
            wndPtr = wndPtr->parent;
            dc->w.DCOrgX += wndPtr->rectClient.left;
            dc->w.DCOrgY += wndPtr->rectClient.top;
        }
        dc->w.DCOrgX -= wndPtr->rectWindow.left;
        dc->w.DCOrgY -= wndPtr->rectWindow.top;
        dc->u.x.drawable = wndPtr->window;
    }
}


/***********************************************************************
 *           GetDCEx    (USER.359)
 */
/* Unimplemented flags: DCX_LOCKWINDOWUPDATE
 */
HDC GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    HANDLE hdce;
    HRGN hrgnVisible;
    HDC hdc = 0;
    DCE * dce;
    DC * dc;
    WND * wndPtr;
    
    if (hwnd)
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    }
    else wndPtr = NULL;

    if (flags & DCX_USESTYLE)
    {
        /* Set the flags according to the window style. */
	/* Not sure if this is the real meaning of the DCX_USESTYLE flag... */
	flags &= ~(DCX_CACHE | DCX_CLIPCHILDREN |
                   DCX_CLIPSIBLINGS | DCX_PARENTCLIP);
	if (wndPtr)
	{
            if (!(wndPtr->class->wc.style & (CS_OWNDC | CS_CLASSDC)))
		flags |= DCX_CACHE;
            if (wndPtr->class->wc.style & CS_PARENTDC) flags |= DCX_PARENTCLIP;
	    if (wndPtr->dwStyle & WS_CLIPCHILDREN) flags |= DCX_CLIPCHILDREN;
	    if (wndPtr->dwStyle & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
	}
	else flags |= DCX_CACHE;
    }

      /* Can only use PARENTCLIP on child windows */
    if (!wndPtr || !(wndPtr->dwStyle & WS_CHILD)) flags &= ~DCX_PARENTCLIP;

      /* Whole window DC implies using cache DC and not clipping children */
    if (flags & DCX_WINDOW) flags = (flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;

    if (flags & DCX_CACHE)
    {
	for (hdce = firstDCE; (hdce); hdce = dce->hNext)
	{
	    if (!(dce = (DCE *) USER_HEAP_LIN_ADDR( hdce ))) return 0;
	    if ((dce->type == DCE_CACHE_DC) && (!dce->inUse)) break;
	}
    }
    else hdce = wndPtr->hdce;

    if (!hdce) return 0;
    dce = (DCE *) USER_HEAP_LIN_ADDR( hdce );
    dce->hwndCurrent = hwnd;
    dce->inUse       = TRUE;
    hdc = dce->hdc;
    
      /* Initialize DC */
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;

    DCE_SetDrawable( wndPtr, dc, flags );
    if (hwnd)
    {
        if (flags & DCX_PARENTCLIP)  /* Get a VisRgn for the parent */
        {
            WND *parentPtr = wndPtr->parent;
            DWORD newflags = flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                                       DCX_WINDOW);
            if (parentPtr->dwStyle & WS_CLIPSIBLINGS)
                newflags |= DCX_CLIPSIBLINGS;
            hrgnVisible = DCE_GetVisRgn( parentPtr->hwndSelf, newflags );
            if (flags & DCX_WINDOW)
                OffsetRgn( hrgnVisible, -wndPtr->rectWindow.left,
                                        -wndPtr->rectWindow.top );
            else OffsetRgn( hrgnVisible, -wndPtr->rectClient.left,
                                         -wndPtr->rectClient.top );
        }
        else hrgnVisible = DCE_GetVisRgn( hwnd, flags );
    }
    else  /* Get a VisRgn for the whole screen */
    {
        hrgnVisible = CreateRectRgn( 0, 0, SYSMETRICS_CXSCREEN,
                                     SYSMETRICS_CYSCREEN);
    }

      /* Intersect VisRgn with the given region */

    if ((flags & DCX_INTERSECTRGN) || (flags & DCX_EXCLUDERGN))
    {
        CombineRgn( hrgnVisible, hrgnVisible, hrgnClip,
                    (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
    }
    SelectVisRgn( hdc, hrgnVisible );
    DeleteObject( hrgnVisible );

    dprintf_dc(stddeb, "GetDCEx(%04x,%04x,0x%lx): returning %04x\n", 
	       hwnd, hrgnClip, flags, hdc);
    return hdc;
}


/***********************************************************************
 *           GetDC    (USER.66)
 */
HDC GetDC( HWND hwnd )
{
    return GetDCEx( hwnd, 0, DCX_USESTYLE );
}


/***********************************************************************
 *           GetWindowDC    (USER.67)
 */
HDC GetWindowDC( HWND hwnd )
{
    int flags = DCX_CACHE | DCX_WINDOW;
    if (hwnd)
    {
	WND * wndPtr;
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
/*	if (wndPtr->dwStyle & WS_CLIPCHILDREN) flags |= DCX_CLIPCHILDREN; */
	if (wndPtr->dwStyle & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
    }
    return GetDCEx( hwnd, 0, flags );
}


/***********************************************************************
 *           ReleaseDC    (USER.68)
 */
int ReleaseDC( HWND hwnd, HDC hdc )
{
    HANDLE hdce;
    DCE * dce = NULL;
    
    dprintf_dc(stddeb, "ReleaseDC: %04x %04x\n", hwnd, hdc );
        
    for (hdce = firstDCE; (hdce); hdce = dce->hNext)
    {
	if (!(dce = (DCE *) USER_HEAP_LIN_ADDR( hdce ))) return 0;
	if (dce->inUse && (dce->hdc == hdc)) break;
    }
    if (!hdce) return 0;

    if (dce->type == DCE_CACHE_DC)
    {
	SetDCState( dce->hdc, defaultDCstate );
	dce->inUse = FALSE;
    }
    return 1;
}
