/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 *	     1996,1997 Alex Korobka
 *
 *
 * Note: Visible regions of CS_OWNDC/CS_CLASSDC window DCs 
 * have to be updated dynamically. 
 * 
 * Internal DCX flags:
 *
 * DCX_DCEEMPTY    - dce is uninitialized
 * DCX_DCEBUSY     - dce is in use
 * DCX_DCEDIRTY    - ReleaseDC() should wipe instead of caching
 * DCX_KEEPCLIPRGN - ReleaseDC() should not delete the clipping region
 * DCX_WINDOWPAINT - BeginPaint() is in effect
 */

#include "options.h"
#include "dce.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "region.h"
#include "heap.h"
#include "sysmetrics.h"
#include "debug.h"

#define NB_DCE    5  /* Number of DCEs created at startup */

static DCE *firstDCE = 0;
static HDC32 defaultDCstate = 0;

static void DCE_DeleteClipRgn( DCE* );
static INT32 DCE_ReleaseDC( DCE* );


/***********************************************************************
 *           DCE_DumpCache
 */
static void DCE_DumpCache(void)
{
    DCE* dce = firstDCE;
    
    DUMP("DCE:\n");
    while( dce )
    {
	DUMP("\t[0x%08x] hWnd 0x%04x, dcx %08x, %s %s\n",
	     (unsigned)dce, dce->hwndCurrent, (unsigned)dce->DCXflags, 
	     (dce->DCXflags & DCX_CACHE) ? "Cache" : "Owned", 
	     (dce->DCXflags & DCX_DCEBUSY) ? "InUse" : "" );
	dce = dce->next;
    }
}

/***********************************************************************
 *           DCE_AllocDCE
 *
 * Allocate a new DCE.
 */
DCE *DCE_AllocDCE( HWND32 hWnd, DCE_TYPE type )
{
    DCE * dce;
    if (!(dce = HeapAlloc( SystemHeap, 0, sizeof(DCE) ))) return NULL;
    if (!(dce->hDC = CreateDC16( "DISPLAY", NULL, NULL, NULL )))
    {
        HeapFree( SystemHeap, 0, dce );
	return 0;
    }

    /* store DCE handle in DC hook data field */

    SetDCHook( dce->hDC, (FARPROC16)DCHook, (DWORD)dce );

    dce->hwndCurrent = hWnd;
    dce->hClipRgn    = 0;
    dce->next        = firstDCE;
    firstDCE = dce;

    if( type != DCE_CACHE_DC ) /* owned or class DC */
    {
	dce->DCXflags = DCX_DCEBUSY;
	if( hWnd )
	{
	    WND* wnd = WIN_FindWndPtr(hWnd);
	
	    if( wnd->dwStyle & WS_CLIPCHILDREN ) dce->DCXflags |= DCX_CLIPCHILDREN;
	    if( wnd->dwStyle & WS_CLIPSIBLINGS ) dce->DCXflags |= DCX_CLIPSIBLINGS;
	}
	SetHookFlags(dce->hDC,DCHF_INVALIDATEVISRGN);
    }
    else dce->DCXflags = DCX_CACHE | DCX_DCEEMPTY;

    return dce;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
DCE* DCE_FreeDCE( DCE *dce )
{
    DCE **ppDCE = &firstDCE;

    if (!dce) return NULL;
    while (*ppDCE && (*ppDCE != dce)) ppDCE = &(*ppDCE)->next;
    if (*ppDCE == dce) *ppDCE = dce->next;

    SetDCHook(dce->hDC, NULL, 0L);

    DeleteDC32( dce->hDC );
    if( dce->hClipRgn && !(dce->DCXflags & DCX_KEEPCLIPRGN) )
	DeleteObject32(dce->hClipRgn);
    HeapFree( SystemHeap, 0, dce );
    return *ppDCE;
}

/***********************************************************************
 *           DCE_FreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void DCE_FreeWindowDCE( WND* pWnd )
{
    DCE *pDCE = firstDCE;

    while( pDCE )
    {
	if( pDCE->hwndCurrent == pWnd->hwndSelf )
	{
	    if( pDCE == pWnd->dce ) /* owned DCE */
	    {
		pDCE = DCE_FreeDCE( pDCE );
		pWnd->dce = NULL;
		continue;
	    }
	    else
	    {
		if(!(pDCE->DCXflags & DCX_CACHE) ) /* class DCE */
		{
		    if( pDCE->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN) )
			DCE_DeleteClipRgn( pDCE );
		}
		else if( pDCE->DCXflags & DCX_DCEBUSY ) /* shared cache DCE */
		{
		    ERR(dc,"[%04x] GetDC() without ReleaseDC()!\n", 
			pWnd->hwndSelf);
		    DCE_ReleaseDC( pDCE );
		}

		pDCE->DCXflags &= DCX_CACHE;
		pDCE->DCXflags |= DCX_DCEEMPTY;
		pDCE->hwndCurrent = 0;
	    }
	}
	pDCE = pDCE->next;
    }
}


/***********************************************************************
 *   DCE_DeleteClipRgn
 */
static void DCE_DeleteClipRgn( DCE* dce )
{
    dce->DCXflags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

    if( dce->DCXflags & DCX_KEEPCLIPRGN )
	dce->DCXflags &= ~DCX_KEEPCLIPRGN;
    else
	if( dce->hClipRgn > 1 )
	    DeleteObject32( dce->hClipRgn );

    dce->hClipRgn = 0;

    TRACE(dc,"\trestoring VisRgn\n");

    RestoreVisRgn(dce->hDC);
}


/***********************************************************************
 *   DCE_ReleaseDC
 */
static INT32 DCE_ReleaseDC( DCE* dce )
{
    if ((dce->DCXflags & (DCX_DCEEMPTY | DCX_DCEBUSY)) != DCX_DCEBUSY) return 0;

    /* restore previous visible region */

    if ((dce->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
        (dce->DCXflags & (DCX_CACHE | DCX_WINDOWPAINT)) )
        DCE_DeleteClipRgn( dce );

    if (dce->DCXflags & DCX_CACHE)
    {
        SetDCState( dce->hDC, defaultDCstate );
        dce->DCXflags &= ~DCX_DCEBUSY;
	if (dce->DCXflags & DCX_DCEDIRTY)
	{
	    /* don't keep around invalidated entries 
	     * because SetDCState() disables hVisRgn updates
	     * by removing dirty bit. */

	    dce->hwndCurrent = 0;
	    dce->DCXflags &= DCX_CACHE;
	    dce->DCXflags |= DCX_DCEEMPTY;
	}
    }
    return 1;
}


/***********************************************************************
 *   DCE_InvalidateDCE
 *
 * It is called from SetWindowPos() - we have to mark as dirty all busy
 * DCE's for windows that have pWnd->parent as an ansector and whose client 
 * rect intersects with specified update rectangle. 
 */
BOOL32 DCE_InvalidateDCE(WND* pWnd, const RECT32* pRectUpdate)
{
    WND* wndScope = pWnd->parent;
    BOOL32 bRet = FALSE;

    if( wndScope )
    {
	DCE *dce;

	TRACE(dc,"scope hwnd = %04x, (%i,%i - %i,%i)\n",
		     wndScope->hwndSelf, pRectUpdate->left,pRectUpdate->top,
		     pRectUpdate->right,pRectUpdate->bottom);
	if(TRACE_ON(dc)) 
	  DCE_DumpCache();

 	/* walk all DCEs and fixup non-empty entries */

	for (dce = firstDCE; (dce); dce = dce->next)
	{
	    if( !(dce->DCXflags & DCX_DCEEMPTY) )
	    {
		WND* wndCurrent = WIN_FindWndPtr(dce->hwndCurrent);

		if( wndCurrent && wndCurrent != WIN_GetDesktop() )
		{
		    WND* wnd = wndCurrent;
		    INT32 xoffset = 0, yoffset = 0;

		    if( (wndCurrent == wndScope) && !(dce->DCXflags & DCX_CLIPCHILDREN) ) continue;

		    /* check if DCE window is within the z-order scope */

		    for( wnd = wndCurrent; wnd; wnd = wnd->parent )
		    {
			if( wnd == wndScope )
		 	{
			    RECT32 wndRect;

			    wndRect = wndCurrent->rectWindow;

			    OffsetRect32( &wndRect, xoffset - wndCurrent->rectClient.left, 
						    yoffset - wndCurrent->rectClient.top);

			    if (pWnd == wndCurrent ||
				IntersectRect32( &wndRect, &wndRect, pRectUpdate ))
			    { 
				if( !(dce->DCXflags & DCX_DCEBUSY) )
				{
				    /* Don't bother with visible regions of unused DCEs */

				    TRACE(dc,"\tpurged %08x dce [%04x]\n", 
						(unsigned)dce, wndCurrent->hwndSelf);

				    dce->hwndCurrent = 0;
				    dce->DCXflags &= DCX_CACHE;
				    dce->DCXflags |= DCX_DCEEMPTY;
				}
				else
				{
				    /* Set dirty bits in the hDC and DCE structs */

				    TRACE(dc,"\tfixed up %08x dce [%04x]\n", 
						(unsigned)dce, wndCurrent->hwndSelf);

				    dce->DCXflags |= DCX_DCEDIRTY;
				    SetHookFlags(dce->hDC, DCHF_INVALIDATEVISRGN);
				    bRet = TRUE;
				}
			    }
			    break;
			}
			xoffset += wnd->rectClient.left;
			yoffset += wnd->rectClient.top;
		    }
		}
	    }
	} /* dce list */
    }
    return bRet;
}

/***********************************************************************
 *           DCE_Init
 */
void DCE_Init(void)
{
    int i;
    DCE * dce;
        
    for (i = 0; i < NB_DCE; i++)
    {
	if (!(dce = DCE_AllocDCE( 0, DCE_CACHE_DC ))) return;
	if (!defaultDCstate) defaultDCstate = GetDCState( dce->hDC );
    }
}


/***********************************************************************
 *           DCE_GetVisRect
 *
 * Calculate the visible rectangle of a window (i.e. the client or
 * window area clipped by the client area of all ancestors) in the
 * corresponding coordinates. Return FALSE if the visible region is empty.
 */
static BOOL32 DCE_GetVisRect( WND *wndPtr, BOOL32 clientArea, RECT32 *lprect )
{
    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
	INT32 xoffset = lprect->left;
	INT32 yoffset = lprect->top;

	while (wndPtr->dwStyle & WS_CHILD)
	{
	    wndPtr = wndPtr->parent;

	    if ( (wndPtr->dwStyle & (WS_ICONIC | WS_VISIBLE)) != WS_VISIBLE )
		goto fail;

	    xoffset += wndPtr->rectClient.left;
	    yoffset += wndPtr->rectClient.top;
	    OffsetRect32( lprect, wndPtr->rectClient.left,
				  wndPtr->rectClient.top );

	    if( (wndPtr->rectClient.left >= wndPtr->rectClient.right) ||
		(wndPtr->rectClient.top >= wndPtr->rectClient.bottom) ||
		(lprect->left >= wndPtr->rectClient.right) ||
		(lprect->right <= wndPtr->rectClient.left) ||
		(lprect->top >= wndPtr->rectClient.bottom) ||
		(lprect->bottom <= wndPtr->rectClient.top) )
		goto fail;

	    lprect->left = MAX( lprect->left, wndPtr->rectClient.left );
	    lprect->right = MIN( lprect->right, wndPtr->rectClient.right );
	    lprect->top = MAX( lprect->top, wndPtr->rectClient.top );
	    lprect->bottom = MIN( lprect->bottom, wndPtr->rectClient.bottom );
	}
	OffsetRect32( lprect, -xoffset, -yoffset );
	return TRUE;
    }

fail:
    SetRectEmpty32( lprect );
    return FALSE;
}


/***********************************************************************
 *           DCE_AddClipRects
 *
 * Go through the linked list of windows from pWndStart to pWndEnd,
 * adding to the clip region the intersection of the target rectangle
 * with an offset window rectangle.
 */
static BOOL32 DCE_AddClipRects( WND *pWndStart, WND *pWndEnd, 
			        HRGN32 hrgnClip, LPRECT32 lpRect, int x, int y )
{
    RECT32 rect;

    if( pWndStart->pDriver->pIsSelfClipping( pWndStart ) )
        return TRUE; /* The driver itself will do the clipping */

    for (; pWndStart != pWndEnd; pWndStart = pWndStart->next)
    {
	if( !(pWndStart->dwStyle & WS_VISIBLE) ) continue;
	    
	rect.left = pWndStart->rectWindow.left + x;
	rect.top = pWndStart->rectWindow.top + y;
	rect.right = pWndStart->rectWindow.right + x;
	rect.bottom = pWndStart->rectWindow.bottom + y;

	if( IntersectRect32( &rect, &rect, lpRect ))
	    if(!REGION_UnionRectWithRgn( hrgnClip, &rect ))
		break;
    }
    return (pWndStart == pWndEnd);
}


/***********************************************************************
 *           DCE_GetVisRgn
 *
 * Return the visible region of a window, i.e. the client or window area
 * clipped by the client area of all ancestors, and then optionally
 * by siblings and children.
 */
HRGN32 DCE_GetVisRgn( HWND32 hwnd, WORD flags )
{
    HRGN32 hrgnVis = 0;
    RECT32 rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

    /* Get visible rectangle and create a region with it. */

    if (wndPtr && DCE_GetVisRect(wndPtr, !(flags & DCX_WINDOW), &rect))
    {
	if((hrgnVis = CreateRectRgnIndirect32( &rect )))
        {
	    HRGN32 hrgnClip = CreateRectRgn32( 0, 0, 0, 0 );
	    INT32 xoffset, yoffset;

	    if( hrgnClip )
	    {
		/* Compute obscured region for the visible rectangle by 
		 * clipping children, siblings, and ancestors. Note that
		 * DCE_GetVisRect() returns a rectangle either in client
		 * or in window coordinates (for DCX_WINDOW request). */

		if( (flags & DCX_CLIPCHILDREN) && wndPtr->child )
		{
		    if( flags & DCX_WINDOW )
		    {
			/* adjust offsets since child window rectangles are 
			 * in client coordinates */

			xoffset = wndPtr->rectClient.left - wndPtr->rectWindow.left;
			yoffset = wndPtr->rectClient.top - wndPtr->rectWindow.top;
		    }
		    else 
			xoffset = yoffset = 0;

		    DCE_AddClipRects( wndPtr->child, NULL, hrgnClip, 
				      &rect, xoffset, yoffset );
		}

		/* sibling window rectangles are in client 
		 * coordinates of the parent window */

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

		if (flags & DCX_CLIPSIBLINGS && wndPtr->parent )
		    DCE_AddClipRects( wndPtr->parent->child,
				      wndPtr, hrgnClip, &rect, xoffset, yoffset );

		/* Clip siblings of all ancestors that have the
                 * WS_CLIPSIBLINGS style
		 */

		while (wndPtr->dwStyle & WS_CHILD)
		{
		    wndPtr = wndPtr->parent;
		    xoffset -= wndPtr->rectClient.left;
		    yoffset -= wndPtr->rectClient.top;
		    if(wndPtr->dwStyle & WS_CLIPSIBLINGS && wndPtr->parent)
		    {
			DCE_AddClipRects( wndPtr->parent->child, wndPtr,
					  hrgnClip, &rect, xoffset, yoffset );
		    }
		}

		/* Now once we've got a jumbo clip region we have
		 * to substract it from the visible rectangle.
	         */

		CombineRgn32( hrgnVis, hrgnVis, hrgnClip, RGN_DIFF );
		DeleteObject32( hrgnClip );
	    }
	    else
	    {
		DeleteObject32( hrgnVis );
		hrgnVis = 0;
	    }
	}
    }
    else
	hrgnVis = CreateRectRgn32(0, 0, 0, 0); /* empty */
    return hrgnVis;
}

/***********************************************************************
 *           DCE_OffsetVisRgn
 *
 * Change region from DC-origin relative coordinates to screen coords.
 */

static void DCE_OffsetVisRgn( HDC32 hDC, HRGN32 hVisRgn )
{
    DC *dc;
    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return;

    OffsetRgn32( hVisRgn, dc->w.DCOrgX, dc->w.DCOrgY );

    GDI_HEAP_UNLOCK( hDC );
}

/***********************************************************************
 *           DCE_ExcludeRgn
 * 
 *  Translate given region from the wnd client to the DC coordinates
 *  and add it to the clipping region.
 */
INT16 DCE_ExcludeRgn( HDC32 hDC, WND* wnd, HRGN32 hRgn )
{
  POINT32  pt = {0, 0};
  DCE     *dce = firstDCE;

  while (dce && (dce->hDC != hDC)) dce = dce->next;
  if( dce )
  {
      MapWindowPoints32( wnd->hwndSelf, dce->hwndCurrent, &pt, 1);
      if( dce->DCXflags & DCX_WINDOW )
      { 
	  wnd = WIN_FindWndPtr(dce->hwndCurrent);
	  pt.x += wnd->rectClient.left - wnd->rectWindow.left;
	  pt.y += wnd->rectClient.top - wnd->rectWindow.top;
      }
  }
  else return ERROR;
  OffsetRgn32(hRgn, pt.x, pt.y);

  return ExtSelectClipRgn( hDC, hRgn, RGN_DIFF );
}

/***********************************************************************
 *           GetDCEx16    (USER.359)
 */
HDC16 WINAPI GetDCEx16( HWND16 hwnd, HRGN16 hrgnClip, DWORD flags )
{
    return (HDC16)GetDCEx32( hwnd, hrgnClip, flags );
}


/***********************************************************************
 *           GetDCEx32    (USER32.231)
 *
 * Unimplemented flags: DCX_LOCKWINDOWUPDATE
 *
 * FIXME: Full support for hrgnClip == 1 (alias for entire window).
 */
HDC32 WINAPI GetDCEx32( HWND32 hwnd, HRGN32 hrgnClip, DWORD flags )
{
    HRGN32 	hrgnVisible = 0;
    HDC32 	hdc = 0;
    DCE * 	dce;
    DC * 	dc;
    WND * 	wndPtr;
    DWORD 	dcxFlags = 0;
    BOOL32	bUpdateVisRgn = TRUE;
    BOOL32	bUpdateClipOrigin = FALSE;

    TRACE(dc,"hwnd %04x, hrgnClip %04x, flags %08x\n", 
				hwnd, hrgnClip, (unsigned)flags);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;

    /* fixup flags */

    if (!(wndPtr->class->style & (CS_OWNDC | CS_CLASSDC))) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
	flags &= ~( DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if( wndPtr->dwStyle & WS_CLIPSIBLINGS )
            flags |= DCX_CLIPSIBLINGS;

	if ( !(flags & DCX_WINDOW) )
	{
            if (wndPtr->class->style & CS_PARENTDC) flags |= DCX_PARENTCLIP;

	    if (wndPtr->dwStyle & WS_CLIPCHILDREN &&
                     !(wndPtr->dwStyle & WS_MINIMIZE) ) flags |= DCX_CLIPCHILDREN;
	}
	else flags |= DCX_CACHE;
    }

    if( flags & DCX_NOCLIPCHILDREN )
    {
        flags |= DCX_CACHE;
        flags &= ~(DCX_PARENTCLIP | DCX_CLIPCHILDREN);
    }

    if (flags & DCX_WINDOW) 
	flags = (flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;

    if (!(wndPtr->dwStyle & WS_CHILD) || !wndPtr->parent ) 
	flags &= ~DCX_PARENTCLIP;
    else if( flags & DCX_PARENTCLIP )
    {
	flags |= DCX_CACHE;
	if( !(flags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN)) )
	    if( (wndPtr->dwStyle & WS_VISIBLE) && (wndPtr->parent->dwStyle & WS_VISIBLE) )
	    {
		flags &= ~DCX_CLIPCHILDREN;
		if( wndPtr->parent->dwStyle & WS_CLIPSIBLINGS )
		    flags |= DCX_CLIPSIBLINGS;
	    }
    }

    /* find a suitable DCE */

    dcxFlags = flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | 
		        DCX_CACHE | DCX_WINDOW);

    if (flags & DCX_CACHE)
    {
	DCE*	dceEmpty;
	DCE*	dceUnused;

	dceEmpty = dceUnused = NULL;

	/* Strategy: First, we attempt to find a non-empty but unused DCE with
	 * compatible flags. Next, we look for an empty entry. If the cache is
	 * full we have to purge one of the unused entries.
	 */

	for (dce = firstDCE; (dce); dce = dce->next)
	{
	    if ((dce->DCXflags & (DCX_CACHE | DCX_DCEBUSY)) == DCX_CACHE )
	    {
		dceUnused = dce;

		if (dce->DCXflags & DCX_DCEEMPTY)
		    dceEmpty = dce;
		else
		if ((dce->hwndCurrent == hwnd) &&
		   ((dce->DCXflags & (DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
				      DCX_CACHE | DCX_WINDOW | DCX_PARENTCLIP)) == dcxFlags))
		{
		    TRACE(dc,"\tfound valid %08x dce [%04x], flags %08x\n", 
					(unsigned)dce, hwnd, (unsigned)dcxFlags );
		    bUpdateVisRgn = FALSE; 
		    bUpdateClipOrigin = TRUE;
		    break;
		}
	    }
	}
	if (!dce) dce = (dceEmpty) ? dceEmpty : dceUnused;
    }
    else 
    {
        dce = (wndPtr->class->style & CS_OWNDC) ? wndPtr->dce : wndPtr->class->dce;
	if( dce->hwndCurrent == hwnd )
	{
	    TRACE(dc,"\tskipping hVisRgn update\n");
	    bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */

	    if( (dce->DCXflags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) &&
		(flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN)) )
	    {
		/* This is likely to be a nested BeginPaint(). */

		if( dce->hClipRgn != hrgnClip )
		{
		    FIXME(dc,"new hrgnClip[%04x] smashes the previous[%04x]\n",
			  hrgnClip, dce->hClipRgn );
		    DCE_DeleteClipRgn( dce );
		}
		else 
		    RestoreVisRgn(dce->hDC);
	    }
	}
    }
    if (!dce) return 0;

    dce->hwndCurrent = hwnd;
    dce->hClipRgn = 0;
    dce->DCXflags = dcxFlags | (flags & DCX_WINDOWPAINT) | DCX_DCEBUSY;
    hdc = dce->hDC;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;
    bUpdateVisRgn = bUpdateVisRgn || (dc->w.flags & DC_DIRTY);

    /* recompute visible region */

    wndPtr->pDriver->pSetDrawable( wndPtr, dc, flags, bUpdateClipOrigin );
    if( bUpdateVisRgn )
    {
	TRACE(dc,"updating visrgn for %08x dce, hwnd [%04x]\n", (unsigned)dce, hwnd);

	if (flags & DCX_PARENTCLIP)
        {
            WND *parentPtr = wndPtr->parent;

	    if( wndPtr->dwStyle & WS_VISIBLE && !(parentPtr->dwStyle & WS_MINIMIZE) )
	    {
		if( parentPtr->dwStyle & WS_CLIPSIBLINGS ) 
		    dcxFlags = DCX_CLIPSIBLINGS | (flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
		else
		    dcxFlags = flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);

                hrgnVisible = DCE_GetVisRgn( parentPtr->hwndSelf, dcxFlags );
		if( flags & DCX_WINDOW )
		    OffsetRgn32( hrgnVisible, -wndPtr->rectWindow.left,
					      -wndPtr->rectWindow.top );
		else
		    OffsetRgn32( hrgnVisible, -wndPtr->rectClient.left,
					      -wndPtr->rectClient.top );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
	    }
	    else
		hrgnVisible = CreateRectRgn32( 0, 0, 0, 0 );
        }
        else 
	    if ((hwnd == GetDesktopWindow32()) &&
                (rootWindow == DefaultRootWindow(display)))
                 hrgnVisible = CreateRectRgn32( 0, 0, SYSMETRICS_CXSCREEN,
						      SYSMETRICS_CYSCREEN );
	    else 
            {
                hrgnVisible = DCE_GetVisRgn( hwnd, flags );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
            }

	dc->w.flags &= ~DC_DIRTY;
	dce->DCXflags &= ~DCX_DCEDIRTY;
	SelectVisRgn( hdc, hrgnVisible );
    }
    else
	TRACE(dc,"no visrgn update %08x dce, hwnd [%04x]\n", (unsigned)dce, hwnd);

    /* apply additional region operation (if any) */

    if( flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) )
    {
	if( !hrgnVisible ) hrgnVisible = CreateRectRgn32( 0, 0, 0, 0 );

	dce->DCXflags |= flags & (DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_EXCLUDERGN);
	dce->hClipRgn = hrgnClip;

	TRACE(dc, "\tsaved VisRgn, clipRgn = %04x\n", hrgnClip);

	SaveVisRgn( hdc );
        CombineRgn32( hrgnVisible, hrgnClip, 0, RGN_COPY );
        DCE_OffsetVisRgn( hdc, hrgnVisible );
        CombineRgn32( hrgnVisible, InquireVisRgn( hdc ), hrgnVisible,
                      (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
	SelectVisRgn( hdc, hrgnVisible );
    }

    if( hrgnVisible ) DeleteObject32( hrgnVisible );

    TRACE(dc, "(%04x,%04x,0x%lx): returning %04x\n", 
	       hwnd, hrgnClip, flags, hdc);
    return hdc;
}


/***********************************************************************
 *           GetDC16    (USER.66)
 */
HDC16 WINAPI GetDC16( HWND16 hwnd )
{
    return (HDC16)GetDC32( hwnd );
}


/***********************************************************************
 *           GetDC32    (USER32.230)
 * RETURNS
 *	:Handle to DC
 *	NULL: Failure
 */
HDC32 WINAPI GetDC32(
	     HWND32 hwnd /* handle of window */
) {
    if (!hwnd)
        return GetDCEx32( GetDesktopWindow32(), 0, DCX_CACHE | DCX_WINDOW );
    return GetDCEx32( hwnd, 0, DCX_USESTYLE );
}


/***********************************************************************
 *           GetWindowDC16    (USER.67)
 */
HDC16 WINAPI GetWindowDC16( HWND16 hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow16();
    return GetDCEx16( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *           GetWindowDC32    (USER32.304)
 */
HDC32 WINAPI GetWindowDC32( HWND32 hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow32();
    return GetDCEx32( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *           ReleaseDC16    (USER.68)
 */
INT16 WINAPI ReleaseDC16( HWND16 hwnd, HDC16 hdc )
{
    return (INT16)ReleaseDC32( hwnd, hdc );
}


/***********************************************************************
 *           ReleaseDC32    (USER32.440)
 *
 * RETURNS
 *	1: Success
 *	0: Failure
 */
INT32 WINAPI ReleaseDC32( 
             HWND32 hwnd /* Handle of window - ignored */, 
             HDC32 hdc   /* Handle of device context */
) {
    DCE * dce = firstDCE;
    
    TRACE(dc, "%04x %04x\n", hwnd, hdc );
        
    while (dce && (dce->hDC != hdc)) dce = dce->next;

    if ( dce ) 
	if ( dce->DCXflags & DCX_DCEBUSY )
	     return DCE_ReleaseDC( dce );
    return 0;
}

/***********************************************************************
 *           DCHook    (USER.362)
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..  
 */
BOOL16 WINAPI DCHook( HDC16 hDC, WORD code, DWORD data, LPARAM lParam )
{
    HRGN32 hVisRgn;
    DCE *dce = firstDCE;;

    TRACE(dc,"hDC = %04x, %i\n", hDC, code);

    while (dce && (dce->hDC != hDC)) dce = dce->next;
    if (!dce) return 0;

    switch( code )
    {
      case DCHC_INVALIDVISRGN:

	   /* GDI code calls this when it detects that the
	    * DC is dirty (usually after SetHookFlags()). This
	    * means that we have to recompute the visible region.
	    */

           if( dce->DCXflags & DCX_DCEBUSY )
 	   {
	       SetHookFlags(hDC, DCHF_VALIDATEVISRGN);
	       hVisRgn = DCE_GetVisRgn(dce->hwndCurrent, dce->DCXflags);

	       TRACE(dc,"\tapplying saved clipRgn\n");
  
	       /* clip this region with saved clipping region */

               if ( (dce->DCXflags & DCX_INTERSECTRGN && dce->hClipRgn != 1) ||
                  (  dce->DCXflags & DCX_EXCLUDERGN && dce->hClipRgn) )
               {

                    if( (!dce->hClipRgn && dce->DCXflags & DCX_INTERSECTRGN) ||
                         (dce->hClipRgn == 1 && dce->DCXflags & DCX_EXCLUDERGN) )            
                         SetRectRgn32(hVisRgn,0,0,0,0);
                    else
                         CombineRgn32(hVisRgn, hVisRgn, dce->hClipRgn, 
                                      (dce->DCXflags & DCX_EXCLUDERGN)? RGN_DIFF:RGN_AND);
	       }
	       dce->DCXflags &= ~DCX_DCEDIRTY;
               DCE_OffsetVisRgn( hDC, hVisRgn );
	       SelectVisRgn(hDC, hVisRgn);
	       DeleteObject32( hVisRgn );
	   }
           else /* non-fatal but shouldn't happen */
	     WARN(dc, "DC is not in use!\n");
	   break;

      case DCHC_DELETEDC: /* FIXME: ?? */
	   break;

      default:
	   FIXME(dc,"unknown code\n");
    }
  return 0;
}


/**********************************************************************
 *          WindowFromDC16   (USER.117)
 */
HWND16 WINAPI WindowFromDC16( HDC16 hDC )
{
    return (HWND16)WindowFromDC32( hDC );
}


/**********************************************************************
 *          WindowFromDC32   (USER32.581)
 */
HWND32 WINAPI WindowFromDC32( HDC32 hDC )
{
    DCE *dce = firstDCE;
    while (dce && (dce->hDC != hDC)) dce = dce->next;
    return dce ? dce->hwndCurrent : 0;
}


/***********************************************************************
 *           LockWindowUpdate16   (USER.294)
 */
BOOL16 WINAPI LockWindowUpdate16( HWND16 hwnd )
{
    return LockWindowUpdate32( hwnd );
}


/***********************************************************************
 *           LockWindowUpdate32   (USER32.378)
 */
BOOL32 WINAPI LockWindowUpdate32( HWND32 hwnd )
{
    /* FIXME? DCX_LOCKWINDOWUPDATE is unimplemented */
    return TRUE;
}
