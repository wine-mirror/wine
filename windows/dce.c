/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "dce.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "user.h"
#include "icon.h"


#define NB_DCE    5  /* Number of DCEs created at startup */

extern Display * display;

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
    HANDLE handle = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(DCE) );
    if (!handle) return 0;
    dce = (DCE *) USER_HEAP_ADDR( handle );
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

    if (!(dce = (DCE *) USER_HEAP_ADDR( hdce ))) return;
    while (*handle && (*handle != hdce))
    {
	DCE * prev = (DCE *) USER_HEAP_ADDR( *handle );	
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
	dce = (DCE *) USER_HEAP_ADDR( handle );	
	if (!defaultDCstate) defaultDCstate = GetDCState( dce->hdc );
    }
}


/***********************************************************************
 *           DCE_GetVisRect
 *
 * Return the visible rectangle of a window, i.e. the client or
 * window area clipped by the client area of all ancestors.
 */
static void DCE_GetVisRect( WND *wndPtr, BOOL clientArea, RECT *lprect )
{
    int xoffset, yoffset;

    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;
    xoffset = lprect->left;
    yoffset = lprect->top;

    while (wndPtr->dwStyle & WS_CHILD)
    {
	WND *parentPtr = WIN_FindWndPtr( wndPtr->hwndParent );
	xoffset += parentPtr->rectClient.left;
	yoffset += parentPtr->rectClient.top;
	OffsetRect( lprect, parentPtr->rectClient.left,
		    parentPtr->rectClient.top );

	  /* Warning!! we assume that IntersectRect() handles the case */
	  /* where the destination is the same as one of the sources.  */
	IntersectRect( lprect, lprect, &parentPtr->rectClient );
	wndPtr = parentPtr;
    }
    OffsetRect( lprect, -xoffset, -yoffset );
}


/***********************************************************************
 *           GetDCEx    (USER.359)
 */
/* Unimplemented flags: DCX_CLIPSIBLINGS, DCX_LOCKWINDOWUPDATE, DCX_PARENTCLIP
 */
HDC GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    HANDLE hdce;
    RECT clipRect;
    HDC hdc = 0;
    DCE * dce;
    DC * dc;
    WND * wndPtr;
    ICONALLOC   *lpico;
    
    if (hwnd)
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
    }
    else wndPtr = NULL;

    if (flags & DCX_USESTYLE)
    {
	/* Not sure if this is the real meaning of the DCX_USESTYLE flag... */
	flags &= ~(DCX_CACHE | DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS);
	if (wndPtr)
	{
	    if (!(wndPtr->flags & (WIN_CLASS_DC | WIN_OWN_DC)))
		flags |= DCX_CACHE;
	    if (wndPtr->dwStyle & WS_CLIPCHILDREN) flags |= DCX_CLIPCHILDREN;
	    if (wndPtr->dwStyle & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
	}
	else flags |= DCX_CACHE;
    }

    if (flags & DCX_CACHE)
    {
	for (hdce = firstDCE; (hdce); hdce = dce->hNext)
	{
	    if (!(dce = (DCE *) USER_HEAP_ADDR( hdce ))) return 0;
	    if ((dce->type == DCE_CACHE_DC) && (!dce->inUse)) break;
	}
    }
    else hdce = wndPtr->hdce;

    if (!hdce) return 0;
    dce = (DCE *) USER_HEAP_ADDR( hdce );
    dce->hwndCurrent = hwnd;
    dce->inUse       = TRUE;
    hdc = dce->hdc;
    
      /* Initialize DC */
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;

    if (wndPtr)
    {
	dc->u.x.drawable = wndPtr->window;
	if (flags & DCX_WINDOW)
	{
	    dc->w.DCOrgX  = 0;
	    dc->w.DCOrgY  = 0;
	    dc->w.DCSizeX = wndPtr->rectWindow.right - wndPtr->rectWindow.left;
	    dc->w.DCSizeY = wndPtr->rectWindow.bottom - wndPtr->rectWindow.top;
	}
	else
	{
	    dc->w.DCOrgX  = wndPtr->rectClient.left - wndPtr->rectWindow.left;
	    dc->w.DCOrgY  = wndPtr->rectClient.top - wndPtr->rectWindow.top;
	    dc->w.DCSizeX = wndPtr->rectClient.right - wndPtr->rectClient.left;
	    dc->w.DCSizeY = wndPtr->rectClient.bottom - wndPtr->rectClient.top;
	}	

	DCE_GetVisRect( wndPtr, !(flags & DCX_WINDOW), &clipRect );
	IntersectVisRect( hdc, clipRect.left, clipRect.top,
			  clipRect.right, clipRect.bottom );
    }
    else dc->u.x.drawable = rootWindow;

    if (flags & DCX_CLIPCHILDREN)
	XSetSubwindowMode( display, dc->u.x.gc, ClipByChildren );
    else XSetSubwindowMode( display, dc->u.x.gc, IncludeInferiors);

    if ((flags & DCX_INTERSECTRGN) || (flags & DCX_EXCLUDERGN))
    {
	HRGN hrgn = CreateRectRgn( 0, 0, 0, 0 );
	if (hrgn)
	{
	    if (CombineRgn( hrgn, InquireVisRgn(hdc), hrgnClip,
			    (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF )) {
		SelectVisRgn( hdc, hrgn );
	    }
	    DeleteObject( hrgn );
	}
    }

#ifdef DEBUG_DC
    printf( "GetDCEx(%d,%d,0x%x): returning %d\n", hwnd, hrgnClip, flags, hdc);
#endif
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
	if (wndPtr->dwStyle & WS_CLIPCHILDREN) flags |= DCX_CLIPCHILDREN;
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
    
#ifdef DEBUG_DC
    printf( "ReleaseDC: %d %d\n", hwnd, hdc );
#endif
        
    for (hdce = firstDCE; (hdce); hdce = dce->hNext)
    {
	if (!(dce = (DCE *) USER_HEAP_ADDR( hdce ))) return 0;
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
