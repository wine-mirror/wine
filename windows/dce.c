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

#include <assert.h>
#include "desktop.h"
#include "options.h"
#include "dce.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "region.h"
#include "heap.h"
#include "local.h"
#include "module.h"
#include "debugtools.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

DEFAULT_DEBUG_CHANNEL(dc)

#define NB_DCE    5  /* Number of DCEs created at startup */

static DCE *firstDCE = 0;
static HDC defaultDCstate = 0;

static void DCE_DeleteClipRgn( DCE* );
static INT DCE_ReleaseDC( DCE* );


/***********************************************************************
 *           DCE_DumpCache
 */
static void DCE_DumpCache(void)
{
    DCE *dce;
    
    WIN_LockWnds();
    dce = firstDCE;
    
    DPRINTF("DCE:\n");
    while( dce )
    {
	DPRINTF("\t[0x%08x] hWnd 0x%04x, dcx %08x, %s %s\n",
	     (unsigned)dce, dce->hwndCurrent, (unsigned)dce->DCXflags, 
	     (dce->DCXflags & DCX_CACHE) ? "Cache" : "Owned", 
	     (dce->DCXflags & DCX_DCEBUSY) ? "InUse" : "" );
	dce = dce->next;
    }

    WIN_UnlockWnds();
}

/***********************************************************************
 *           DCE_AllocDCE
 *
 * Allocate a new DCE.
 */
DCE *DCE_AllocDCE( HWND hWnd, DCE_TYPE type )
{
    FARPROC16 hookProc;
    DCE * dce;
    WND* wnd;
    
    if (!(dce = HeapAlloc( SystemHeap, 0, sizeof(DCE) ))) return NULL;
    if (!(dce->hDC = CreateDC16( "DISPLAY", NULL, NULL, NULL )))
    {
        HeapFree( SystemHeap, 0, dce );
	return 0;
    }

    wnd = WIN_FindWndPtr(hWnd);
    
    /* store DCE handle in DC hook data field */

    hookProc = (FARPROC16)NE_GetEntryPoint( GetModuleHandle16("USER"), 362 );
    SetDCHook( dce->hDC, hookProc, (DWORD)dce );

    dce->hwndCurrent = hWnd;
    dce->hClipRgn    = 0;
    dce->next        = firstDCE;
    firstDCE = dce;

    if( type != DCE_CACHE_DC ) /* owned or class DC */
    {
	dce->DCXflags = DCX_DCEBUSY;
	if( hWnd )
	{
	    if( wnd->dwStyle & WS_CLIPCHILDREN ) dce->DCXflags |= DCX_CLIPCHILDREN;
	    if( wnd->dwStyle & WS_CLIPSIBLINGS ) dce->DCXflags |= DCX_CLIPSIBLINGS;
	}
	SetHookFlags16(dce->hDC,DCHF_INVALIDATEVISRGN);
    }
    else dce->DCXflags = DCX_CACHE | DCX_DCEEMPTY;

    WIN_ReleaseWndPtr(wnd);
    
    return dce;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
DCE* DCE_FreeDCE( DCE *dce )
{
    DCE **ppDCE;

    if (!dce) return NULL;

    WIN_LockWnds();

    ppDCE = &firstDCE;

    while (*ppDCE && (*ppDCE != dce)) ppDCE = &(*ppDCE)->next;
    if (*ppDCE == dce) *ppDCE = dce->next;

    SetDCHook(dce->hDC, NULL, 0L);

    DeleteDC( dce->hDC );
    if( dce->hClipRgn && !(dce->DCXflags & DCX_KEEPCLIPRGN) )
	DeleteObject(dce->hClipRgn);
    HeapFree( SystemHeap, 0, dce );

    WIN_UnlockWnds();
    
    return *ppDCE;
}

/***********************************************************************
 *           DCE_FreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void DCE_FreeWindowDCE( WND* pWnd )
{
    DCE *pDCE;

    WIN_LockWnds();
    pDCE = firstDCE;

    while( pDCE )
    {
	if( pDCE->hwndCurrent == pWnd->hwndSelf )
	{
	    if( pDCE == pWnd->dce ) /* owned or Class DCE*/
	    {
                if (pWnd->class->style & CS_OWNDC)	/* owned DCE*/
		{
                    pDCE = DCE_FreeDCE( pDCE );
                    pWnd->dce = NULL;
                    continue;
                }
		else if( pDCE->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN) )	/* Class DCE*/
		{
                    DCE_DeleteClipRgn( pDCE );
                    pDCE->hwndCurrent = 0;
		}
	    }
	    else
	    {
		if( pDCE->DCXflags & DCX_DCEBUSY ) /* shared cache DCE */
		{
		    ERR("[%04x] GetDC() without ReleaseDC()!\n", 
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
    
    WIN_UnlockWnds();
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
	    DeleteObject( dce->hClipRgn );

    dce->hClipRgn = 0;

    TRACE("\trestoring VisRgn\n");

    RestoreVisRgn16(dce->hDC);
}


/***********************************************************************
 *   DCE_ReleaseDC
 */
static INT DCE_ReleaseDC( DCE* dce )
{
    if ((dce->DCXflags & (DCX_DCEEMPTY | DCX_DCEBUSY)) != DCX_DCEBUSY) return 0;

    /* restore previous visible region */

    if ((dce->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
        (dce->DCXflags & (DCX_CACHE | DCX_WINDOWPAINT)) )
        DCE_DeleteClipRgn( dce );

    if (dce->DCXflags & DCX_CACHE)
    {
        SetDCState16( dce->hDC, defaultDCstate );
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
 * DCEs for windows that have pWnd->parent as an ansector and whose client 
 * rect intersects with specified update rectangle. In addition, pWnd->parent
 * DCEs may need to be updated if DCX_CLIPCHILDREN flag is set.
 */
BOOL DCE_InvalidateDCE(WND* pWnd, const RECT* pRectUpdate)
{
    WND* wndScope = WIN_LockWndPtr(pWnd->parent);
    WND *pDesktop = WIN_GetDesktop();
    BOOL bRet = FALSE;

    if( wndScope )
    {
	DCE *dce;

	TRACE("scope hwnd = %04x, (%i,%i - %i,%i)\n",
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

		if( wndCurrent )
		{
		    WND* wnd = NULL;
		    INT xoffset = 0, yoffset = 0;

                    if( (wndCurrent == wndScope) && !(dce->DCXflags & DCX_CLIPCHILDREN) )
                    {
			/* child window positions don't bother us */
                        WIN_ReleaseWndPtr(wndCurrent);
                        continue;
                    }

		    if( !Options.desktopGeometry && wndCurrent == pDesktop )
		    {
			/* don't bother with fake desktop */
                        WIN_ReleaseWndPtr(wndCurrent);
			continue;
		    }

		    /* check if DCE window is within the z-order scope */

		    for( wnd = WIN_LockWndPtr(wndCurrent); wnd; WIN_UpdateWndPtr(&wnd,wnd->parent))
		    {
			if( wnd == wndScope )
		 	{
			    RECT wndRect;

			    wndRect = wndCurrent->rectWindow;

			    OffsetRect( &wndRect, xoffset - wndCurrent->rectClient.left, 
						    yoffset - wndCurrent->rectClient.top);

			    if (pWnd == wndCurrent ||
				IntersectRect( &wndRect, &wndRect, pRectUpdate ))
			    { 
				if( !(dce->DCXflags & DCX_DCEBUSY) )
				{
				    /* Don't bother with visible regions of unused DCEs */

				    TRACE("\tpurged %08x dce [%04x]\n", 
						(unsigned)dce, wndCurrent->hwndSelf);

				    dce->hwndCurrent = 0;
				    dce->DCXflags &= DCX_CACHE;
				    dce->DCXflags |= DCX_DCEEMPTY;
				}
				else
				{
				    /* Set dirty bits in the hDC and DCE structs */

				    TRACE("\tfixed up %08x dce [%04x]\n", 
						(unsigned)dce, wndCurrent->hwndSelf);

				    dce->DCXflags |= DCX_DCEDIRTY;
				    SetHookFlags16(dce->hDC, DCHF_INVALIDATEVISRGN);
				    bRet = TRUE;
				}
			    }
                            WIN_ReleaseWndPtr(wnd);
			    break;
			}
			xoffset += wnd->rectClient.left;
			yoffset += wnd->rectClient.top;
		    }
		}
                WIN_ReleaseWndPtr(wndCurrent);
	    }
	} /* dce list */
        WIN_ReleaseWndPtr(wndScope);
    }
    WIN_ReleaseDesktop();
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
	if (!defaultDCstate) defaultDCstate = GetDCState16( dce->hDC );
    }
}


/***********************************************************************
 *           DCE_GetVisRect
 *
 * Calculate the visible rectangle of a window (i.e. the client or
 * window area clipped by the client area of all ancestors) in the
 * corresponding coordinates. Return FALSE if the visible region is empty.
 */
static BOOL DCE_GetVisRect( WND *wndPtr, BOOL clientArea, RECT *lprect )
{
    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;

    if (wndPtr->dwStyle & WS_VISIBLE)
    {
	INT xoffset = lprect->left;
	INT yoffset = lprect->top;

	while( !(wndPtr->flags & WIN_NATIVE) &&
	   ( wndPtr = WIN_LockWndPtr(wndPtr->parent)) )
	{
	    if ( (wndPtr->dwStyle & (WS_ICONIC | WS_VISIBLE)) != WS_VISIBLE )
            {
                WIN_ReleaseWndPtr(wndPtr);
		goto fail;
            }

	    xoffset += wndPtr->rectClient.left;
	    yoffset += wndPtr->rectClient.top;
	    OffsetRect( lprect, wndPtr->rectClient.left,
				  wndPtr->rectClient.top );

	    if( (wndPtr->rectClient.left >= wndPtr->rectClient.right) ||
		(wndPtr->rectClient.top >= wndPtr->rectClient.bottom) ||
		(lprect->left >= wndPtr->rectClient.right) ||
		(lprect->right <= wndPtr->rectClient.left) ||
		(lprect->top >= wndPtr->rectClient.bottom) ||
		(lprect->bottom <= wndPtr->rectClient.top) )
            {
                WIN_ReleaseWndPtr(wndPtr);
		goto fail;
            }

	    lprect->left = MAX( lprect->left, wndPtr->rectClient.left );
	    lprect->right = MIN( lprect->right, wndPtr->rectClient.right );
	    lprect->top = MAX( lprect->top, wndPtr->rectClient.top );
	    lprect->bottom = MIN( lprect->bottom, wndPtr->rectClient.bottom );

            WIN_ReleaseWndPtr(wndPtr);
	}
	OffsetRect( lprect, -xoffset, -yoffset );
	return TRUE;
    }

fail:
    SetRectEmpty( lprect );
    return FALSE;
}


/***********************************************************************
 *           DCE_AddClipRects
 *
 * Go through the linked list of windows from pWndStart to pWndEnd,
 * adding to the clip region the intersection of the target rectangle
 * with an offset window rectangle.
 */
static BOOL DCE_AddClipRects( WND *pWndStart, WND *pWndEnd, 
			        HRGN hrgnClip, LPRECT lpRect, int x, int y )
{
    RECT rect;

    if( pWndStart->pDriver->pIsSelfClipping( pWndStart ) )
        return TRUE; /* The driver itself will do the clipping */

    for (WIN_LockWndPtr(pWndStart); pWndStart != pWndEnd; WIN_UpdateWndPtr(&pWndStart,pWndStart->next))
    {
        if( !(pWndStart->dwStyle & WS_VISIBLE) ) continue;
	    
	rect.left = pWndStart->rectWindow.left + x;
	rect.top = pWndStart->rectWindow.top + y;
	rect.right = pWndStart->rectWindow.right + x;
	rect.bottom = pWndStart->rectWindow.bottom + y;

	if( IntersectRect( &rect, &rect, lpRect ))
        {
	    if(!REGION_UnionRectWithRgn( hrgnClip, &rect )) break;
        }
    }
    WIN_ReleaseWndPtr(pWndStart);
    return (pWndStart == pWndEnd);
}


/***********************************************************************
 *           DCE_GetVisRgn
 *
 * Return the visible region of a window, i.e. the client or window area
 * clipped by the client area of all ancestors, and then optionally
 * by siblings and children.
 */
HRGN DCE_GetVisRgn( HWND hwnd, WORD flags, HWND hwndChild, WORD cflags )
{
    HRGN hrgnVis = 0;
    RECT rect;
    WND *wndPtr = WIN_FindWndPtr( hwnd );
    WND *childWnd = WIN_FindWndPtr( hwndChild );

    /* Get visible rectangle and create a region with it. */

    if (wndPtr && DCE_GetVisRect(wndPtr, !(flags & DCX_WINDOW), &rect))
    {
	if((hrgnVis = CreateRectRgnIndirect( &rect )))
        {
	    HRGN hrgnClip = CreateRectRgn( 0, 0, 0, 0 );
	    INT xoffset, yoffset;

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

		/* We may need to clip children of child window, if a window with PARENTDC
		 * class style and CLIPCHILDREN window style (like in Free Agent 16
		 * preference dialogs) gets here, we take the region for the parent window
		 * but apparently still need to clip the children of the child window... */

		if( (cflags & DCX_CLIPCHILDREN) && childWnd && childWnd->child )
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

		    /* client coordinates of child window */
		    xoffset += childWnd->rectClient.left;
		    yoffset += childWnd->rectClient.top;

		    DCE_AddClipRects( childWnd->child, NULL, hrgnClip, 
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
		    WIN_UpdateWndPtr(&wndPtr,wndPtr->parent);
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

		CombineRgn( hrgnVis, hrgnVis, hrgnClip, RGN_DIFF );
		DeleteObject( hrgnClip );
	    }
	    else
	    {
		DeleteObject( hrgnVis );
		hrgnVis = 0;
	    }
	}
    }
    else
	hrgnVis = CreateRectRgn(0, 0, 0, 0); /* empty */
    WIN_ReleaseWndPtr(wndPtr);
    WIN_ReleaseWndPtr(childWnd);
    return hrgnVis;
}

/***********************************************************************
 *           DCE_OffsetVisRgn
 *
 * Change region from DC-origin relative coordinates to screen coords.
 */

static void DCE_OffsetVisRgn( HDC hDC, HRGN hVisRgn )
{
    DC *dc;
    if (!(dc = (DC *) GDI_GetObjPtr( hDC, DC_MAGIC ))) return;

    OffsetRgn( hVisRgn, dc->w.DCOrgX, dc->w.DCOrgY );

    GDI_HEAP_UNLOCK( hDC );
}

/***********************************************************************
 *           DCE_ExcludeRgn
 * 
 *  Translate given region from the wnd client to the DC coordinates
 *  and add it to the clipping region.
 */
INT16 DCE_ExcludeRgn( HDC hDC, WND* wnd, HRGN hRgn )
{
  POINT  pt = {0, 0};
  DCE     *dce = firstDCE;

  while (dce && (dce->hDC != hDC)) dce = dce->next;
  if( dce )
  {
      MapWindowPoints( wnd->hwndSelf, dce->hwndCurrent, &pt, 1);
      if( dce->DCXflags & DCX_WINDOW )
      { 
	  wnd = WIN_FindWndPtr(dce->hwndCurrent);
	  pt.x += wnd->rectClient.left - wnd->rectWindow.left;
	  pt.y += wnd->rectClient.top - wnd->rectWindow.top;
          WIN_ReleaseWndPtr(wnd);
      }
  }
  else return ERROR;
  OffsetRgn(hRgn, pt.x, pt.y);

  return ExtSelectClipRgn( hDC, hRgn, RGN_DIFF );
}


/***********************************************************************
 *           GetDCEx16    (USER.359)
 */
HDC16 WINAPI GetDCEx16( HWND16 hwnd, HRGN16 hrgnClip, DWORD flags )
{
    return (HDC16)GetDCEx( hwnd, hrgnClip, flags );
}


/***********************************************************************
 *           GetDCEx32    (USER32.231)
 *
 * Unimplemented flags: DCX_LOCKWINDOWUPDATE
 *
 * FIXME: Full support for hrgnClip == 1 (alias for entire window).
 */
HDC WINAPI GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    HRGN 	hrgnVisible = 0;
    HDC 	hdc = 0;
    DCE * 	dce;
    DC * 	dc;
    WND * 	wndPtr;
    DWORD 	dcxFlags = 0;
    BOOL	bUpdateVisRgn = TRUE;
    BOOL	bUpdateClipOrigin = FALSE;

    TRACE("hwnd %04x, hrgnClip %04x, flags %08x\n", 
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
		    TRACE("\tfound valid %08x dce [%04x], flags %08x\n", 
					(unsigned)dce, hwnd, (unsigned)dcxFlags );
		    bUpdateVisRgn = FALSE; 
		    bUpdateClipOrigin = TRUE;
		    break;
		}
	    }
	}

	if (!dce) dce = (dceEmpty) ? dceEmpty : dceUnused;
        
        /* if there's no dce empty or unused, allocate a new one */
        if (!dce)
        {
            dce = DCE_AllocDCE( 0, DCE_CACHE_DC );
        }
    }
    else 
    {
        dce = (wndPtr->class->style & CS_OWNDC) ? wndPtr->dce : wndPtr->class->dce;
	if( dce->hwndCurrent == hwnd )
	{
	    TRACE("\tskipping hVisRgn update\n");
	    bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */

            /* Abey - 16Jul99. to take care of the nested GetDC. first one
               with DCX_EXCLUDERGN or DCX_INTERSECTRGN flags and the next
               one with or without these flags. */

	    if(dce->DCXflags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
	    {
		/* This is likely to be a nested BeginPaint().
                   or a BeginPaint() followed by a GetDC()*/

		if( dce->hClipRgn != hrgnClip )
		{
		    FIXME("new hrgnClip[%04x] smashes the previous[%04x]\n",
			  hrgnClip, dce->hClipRgn );
		    DCE_DeleteClipRgn( dce );
		}
		else 
		    RestoreVisRgn16(dce->hDC);
	    }
	}
    }
    if (!dce)
    {
        hdc = 0;
        goto END;
    }

    dce->hwndCurrent = hwnd;
    dce->hClipRgn = 0;
    dce->DCXflags = dcxFlags | (flags & DCX_WINDOWPAINT) | DCX_DCEBUSY;
    hdc = dce->hDC;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC )))
    {
        hdc = 0;
        goto END;
    }
    bUpdateVisRgn = bUpdateVisRgn || (dc->w.flags & DC_DIRTY);

    /* recompute visible region */
    wndPtr->pDriver->pSetDrawable( wndPtr, dc, flags, bUpdateClipOrigin );

    if( bUpdateVisRgn )
    {
	TRACE("updating visrgn for %08x dce, hwnd [%04x]\n", (unsigned)dce, hwnd);

	if (flags & DCX_PARENTCLIP)
        {
            WND *parentPtr = WIN_LockWndPtr(wndPtr->parent);

	    if( wndPtr->dwStyle & WS_VISIBLE && !(parentPtr->dwStyle & WS_MINIMIZE) )
	    {
		if( parentPtr->dwStyle & WS_CLIPSIBLINGS ) 
		    dcxFlags = DCX_CLIPSIBLINGS | (flags & ~(DCX_CLIPCHILDREN | DCX_WINDOW));
		else
		    dcxFlags = flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_WINDOW);

                hrgnVisible = DCE_GetVisRgn( parentPtr->hwndSelf, dcxFlags,
                                             wndPtr->hwndSelf, flags );
		if( flags & DCX_WINDOW )
		    OffsetRgn( hrgnVisible, -wndPtr->rectWindow.left,
					      -wndPtr->rectWindow.top );
		else
		    OffsetRgn( hrgnVisible, -wndPtr->rectClient.left,
					      -wndPtr->rectClient.top );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
	    }
	    else
		hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );
            WIN_ReleaseWndPtr(parentPtr);
        }
        else
	    if ((hwnd == GetDesktopWindow()) && !DESKTOP_IsSingleWindow())
                 hrgnVisible = CreateRectRgn( 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) );
	    else 
            {
                hrgnVisible = DCE_GetVisRgn( hwnd, flags, 0, 0 );
                DCE_OffsetVisRgn( hdc, hrgnVisible );
            }

	dc->w.flags &= ~DC_DIRTY;
	dce->DCXflags &= ~DCX_DCEDIRTY;
	SelectVisRgn16( hdc, hrgnVisible );
    }
    else
	TRACE("no visrgn update %08x dce, hwnd [%04x]\n", (unsigned)dce, hwnd);

    /* apply additional region operation (if any) */

    if( flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN) )
    {
	if( !hrgnVisible ) hrgnVisible = CreateRectRgn( 0, 0, 0, 0 );

	dce->DCXflags |= flags & (DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_EXCLUDERGN);
	dce->hClipRgn = hrgnClip;

	TRACE("\tsaved VisRgn, clipRgn = %04x\n", hrgnClip);

	SaveVisRgn16( hdc );
        CombineRgn( hrgnVisible, hrgnClip, 0, RGN_COPY );
        DCE_OffsetVisRgn( hdc, hrgnVisible );
        CombineRgn( hrgnVisible, InquireVisRgn16( hdc ), hrgnVisible,
                      (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
	SelectVisRgn16( hdc, hrgnVisible );
    }

    if( hrgnVisible ) DeleteObject( hrgnVisible );

    TRACE("(%04x,%04x,0x%lx): returning %04x\n", 
	       hwnd, hrgnClip, flags, hdc);
END:
    WIN_ReleaseWndPtr(wndPtr);
    return hdc;
}


/***********************************************************************
 *           GetDC16    (USER.66)
 */
HDC16 WINAPI GetDC16( HWND16 hwnd )
{
    return (HDC16)GetDC( hwnd );
}


/***********************************************************************
 *           GetDC32    (USER32.230)
 * RETURNS
 *	:Handle to DC
 *	NULL: Failure
 */
HDC WINAPI GetDC(
	     HWND hwnd /* handle of window */
) {
    if (!hwnd)
        return GetDCEx( GetDesktopWindow(), 0, DCX_CACHE | DCX_WINDOW );
    return GetDCEx( hwnd, 0, DCX_USESTYLE );
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
HDC WINAPI GetWindowDC( HWND hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow();
    return GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *           ReleaseDC16    (USER.68)
 */
INT16 WINAPI ReleaseDC16( HWND16 hwnd, HDC16 hdc )
{
    return (INT16)ReleaseDC( hwnd, hdc );
}


/***********************************************************************
 *           ReleaseDC32    (USER32.440)
 *
 * RETURNS
 *	1: Success
 *	0: Failure
 */
INT WINAPI ReleaseDC( 
             HWND hwnd /* Handle of window - ignored */, 
             HDC hdc   /* Handle of device context */
) {
    DCE * dce;
    INT nRet = 0;

    WIN_LockWnds();
    dce = firstDCE;
    
    TRACE("%04x %04x\n", hwnd, hdc );
        
    while (dce && (dce->hDC != hdc)) dce = dce->next;

    if ( dce ) 
	if ( dce->DCXflags & DCX_DCEBUSY )
            nRet = DCE_ReleaseDC( dce );

    WIN_UnlockWnds();

    return nRet;
}

/***********************************************************************
 *           DCHook    (USER.362)
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..  
 */
BOOL16 WINAPI DCHook16( HDC16 hDC, WORD code, DWORD data, LPARAM lParam )
{
    BOOL retv = TRUE;
    HRGN hVisRgn;
    DCE *dce = (DCE *)data;
    DC  *dc;
    WND *wndPtr;

    TRACE("hDC = %04x, %i\n", hDC, code);

    if (!dce) return 0;
    assert(dce->hDC == hDC);

    /* Grab the windows lock before doing anything else  */
    WIN_LockWnds();

    switch( code )
    {
      case DCHC_INVALIDVISRGN:

	   /* GDI code calls this when it detects that the
	    * DC is dirty (usually after SetHookFlags()). This
	    * means that we have to recompute the visible region.
	    */

           if( dce->DCXflags & DCX_DCEBUSY )
 	   {

               /* Update stale DC in DCX */
               wndPtr = WIN_FindWndPtr( dce->hwndCurrent);
	       dc = (DC *) GDI_GetObjPtr( dce->hDC, DC_MAGIC);
	       if( dc && wndPtr)
		 wndPtr->pDriver->pSetDrawable( wndPtr, dc,dce->DCXflags,TRUE);

	       SetHookFlags16(hDC, DCHF_VALIDATEVISRGN);
	       hVisRgn = DCE_GetVisRgn(dce->hwndCurrent, dce->DCXflags, 0, 0);

	       TRACE("\tapplying saved clipRgn\n");
  
	       /* clip this region with saved clipping region */

               if ( (dce->DCXflags & DCX_INTERSECTRGN && dce->hClipRgn != 1) ||
                  (  dce->DCXflags & DCX_EXCLUDERGN && dce->hClipRgn) )
               {

                    if( (!dce->hClipRgn && dce->DCXflags & DCX_INTERSECTRGN) ||
                         (dce->hClipRgn == 1 && dce->DCXflags & DCX_EXCLUDERGN) )            
                         SetRectRgn(hVisRgn,0,0,0,0);
                    else
                         CombineRgn(hVisRgn, hVisRgn, dce->hClipRgn, 
                                      (dce->DCXflags & DCX_EXCLUDERGN)? RGN_DIFF:RGN_AND);
	       }
	       dce->DCXflags &= ~DCX_DCEDIRTY;
               DCE_OffsetVisRgn( hDC, hVisRgn );
	       SelectVisRgn16(hDC, hVisRgn);
	       DeleteObject( hVisRgn );
              WIN_ReleaseWndPtr( wndPtr );  /* Release WIN_FindWndPtr lock */
	   }
           else /* non-fatal but shouldn't happen */
	     WARN("DC is not in use!\n");
	   break;

      case DCHC_DELETEDC:
           /*
            * Windows will not let you delete a DC that is busy
            * (between GetDC and ReleaseDC)
            */

           if ( dce->DCXflags & DCX_DCEBUSY )
           {
               WARN("Application trying to delete a busy DC\n");
               retv = FALSE;
           }
	   break;

      default:
	   FIXME("unknown code\n");
    }

  WIN_UnlockWnds();  /* Release the wnd lock */
  return retv;
}


/**********************************************************************
 *          WindowFromDC16   (USER.117)
 */
HWND16 WINAPI WindowFromDC16( HDC16 hDC )
{
    return (HWND16)WindowFromDC( hDC );
}


/**********************************************************************
 *          WindowFromDC32   (USER32.581)
 */
HWND WINAPI WindowFromDC( HDC hDC )
{
    DCE *dce;
    HWND hwnd;

    WIN_LockWnds();
    dce = firstDCE;
    
    while (dce && (dce->hDC != hDC)) dce = dce->next;

    hwnd = dce ? dce->hwndCurrent : 0;
    WIN_UnlockWnds();
    
    return hwnd;
}


/***********************************************************************
 *           LockWindowUpdate16   (USER.294)
 */
BOOL16 WINAPI LockWindowUpdate16( HWND16 hwnd )
{
    return LockWindowUpdate( hwnd );
}


/***********************************************************************
 *           LockWindowUpdate32   (USER32.378)
 */
BOOL WINAPI LockWindowUpdate( HWND hwnd )
{
    /* FIXME? DCX_LOCKWINDOWUPDATE is unimplemented */
    return TRUE;
}
