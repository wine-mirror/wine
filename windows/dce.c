/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 *	     1996,1997 Alex Korobka
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
 * DCX_DCEEMPTY    - dce is uninitialized
 * DCX_DCEBUSY     - dce is in use
 * DCX_DCEDIRTY    - ReleaseDC() should wipe instead of caching
 * DCX_KEEPCLIPRGN - ReleaseDC() should not delete the clipping region
 * DCX_WINDOWPAINT - BeginPaint() is in effect
 */

#include <assert.h>
#include "dce.h"
#include "win.h"
#include "gdi.h"
#include "user.h"
#include "wine/debug.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

static DCE *firstDCE;
static HDC defaultDCstate;

static void DCE_DeleteClipRgn( DCE* );
static INT DCE_ReleaseDC( DCE* );


/***********************************************************************
 *           DCE_DumpCache
 */
static void DCE_DumpCache(void)
{
    DCE *dce;

    USER_Lock();
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

    USER_Unlock();
}

/***********************************************************************
 *           DCE_AllocDCE
 *
 * Allocate a new DCE.
 */
DCE *DCE_AllocDCE( HWND hWnd, DCE_TYPE type )
{
    DCE * dce;

    if (!(dce = HeapAlloc( GetProcessHeap(), 0, sizeof(DCE) ))) return NULL;
    if (!(dce->hDC = CreateDCA( "DISPLAY", NULL, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, dce );
	return 0;
    }
    if (!defaultDCstate) defaultDCstate = GetDCState16( dce->hDC );

    /* store DCE handle in DC hook data field */

    SetDCHook( dce->hDC, DCHook16, (DWORD)dce );

    dce->hwndCurrent = WIN_GetFullHandle( hWnd );
    dce->hClipRgn    = 0;

    if( type != DCE_CACHE_DC ) /* owned or class DC */
    {
	dce->DCXflags = DCX_DCEBUSY;
	if( hWnd )
	{
            LONG style = GetWindowLongW( hWnd, GWL_STYLE );
            if (style & WS_CLIPCHILDREN) dce->DCXflags |= DCX_CLIPCHILDREN;
            if (style & WS_CLIPSIBLINGS) dce->DCXflags |= DCX_CLIPSIBLINGS;
	}
	SetHookFlags16(dce->hDC,DCHF_INVALIDATEVISRGN);
    }
    else dce->DCXflags = DCX_CACHE | DCX_DCEEMPTY;

    USER_Lock();
    dce->next = firstDCE;
    firstDCE = dce;
    USER_Unlock();
    return dce;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
DCE* DCE_FreeDCE( DCE *dce )
{
    DCE **ppDCE, *ret;

    if (!dce) return NULL;

    USER_Lock();

    ppDCE = &firstDCE;

    while (*ppDCE && (*ppDCE != dce)) ppDCE = &(*ppDCE)->next;
    if (*ppDCE == dce) *ppDCE = dce->next;
    ret = *ppDCE;
    USER_Unlock();

    SetDCHook(dce->hDC, NULL, 0L);

    DeleteDC( dce->hDC );
    if( dce->hClipRgn && !(dce->DCXflags & DCX_KEEPCLIPRGN) )
	DeleteObject(dce->hClipRgn);
    HeapFree( GetProcessHeap(), 0, dce );

    return ret;
}

/***********************************************************************
 *           DCE_FreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void DCE_FreeWindowDCE( HWND hwnd )
{
    DCE *pDCE;
    WND *pWnd = WIN_GetPtr( hwnd );

    pDCE = firstDCE;
    while( pDCE )
    {
	if( pDCE->hwndCurrent == hwnd )
	{
	    if( pDCE == pWnd->dce ) /* owned or Class DCE*/
	    {
                if (pWnd->clsStyle & CS_OWNDC)	/* owned DCE*/
		{
                    pDCE = DCE_FreeDCE( pDCE );
                    pWnd->dce = NULL;
                    continue;
                }
		else if( pDCE->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN) )	/* Class DCE*/
		{
                    if (USER_Driver.pReleaseDC)
                        USER_Driver.pReleaseDC( pDCE->hwndCurrent, pDCE->hDC );
                    DCE_DeleteClipRgn( pDCE );
                    pDCE->hwndCurrent = 0;
		}
	    }
	    else
	    {
		if( pDCE->DCXflags & DCX_DCEBUSY ) /* shared cache DCE */
		{
                    /* FIXME: AFAICS we are doing the right thing here so
                     * this should be a WARN. But this is best left as an ERR
                     * because the 'application error' is likely to come from
                     * another part of Wine (i.e. it's our fault after all).
                     * We should change this to WARN when Wine is more stable
                     * (for 1.0?).
                     */
		    ERR("[%08x] GetDC() without ReleaseDC()!\n",hwnd);
		    DCE_ReleaseDC( pDCE );
		}

                if (pDCE->hwndCurrent && USER_Driver.pReleaseDC)
                    USER_Driver.pReleaseDC( pDCE->hwndCurrent, pDCE->hDC );
		pDCE->DCXflags &= DCX_CACHE;
		pDCE->DCXflags |= DCX_DCEEMPTY;
		pDCE->hwndCurrent = 0;
	    }
	}
	pDCE = pDCE->next;
    }
    WIN_ReleasePtr( pWnd );
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

    /* make it dirty so that the vis rgn gets recomputed next time */
    dce->DCXflags |= DCX_DCEDIRTY;
    SetHookFlags16( dce->hDC, DCHF_INVALIDATEVISRGN );
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
        /* make the DC clean so that SetDCState doesn't try to update the vis rgn */
        SetHookFlags16( dce->hDC, DCHF_VALIDATEVISRGN );
        SetDCState16( dce->hDC, defaultDCstate );
        dce->DCXflags &= ~DCX_DCEBUSY;
	if (dce->DCXflags & DCX_DCEDIRTY)
	{
	    /* don't keep around invalidated entries
	     * because SetDCState() disables hVisRgn updates
	     * by removing dirty bit. */
            if (dce->hwndCurrent && USER_Driver.pReleaseDC)
                USER_Driver.pReleaseDC( dce->hwndCurrent, dce->hDC );
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
 * It is called from SetWindowPos() and EVENT_MapNotify - we have to
 * mark as dirty all busy DCEs for windows that have pWnd->parent as
 * an ancestor and whose client rect intersects with specified update
 * rectangle. In addition, pWnd->parent DCEs may need to be updated if
 * DCX_CLIPCHILDREN flag is set.  */
BOOL DCE_InvalidateDCE(HWND hwnd, const RECT* pRectUpdate)
{
    HWND hwndScope = GetAncestor( hwnd, GA_PARENT );
    BOOL bRet = FALSE;

    if( hwndScope )
    {
	DCE *dce;

	TRACE("scope hwnd = %04x, (%i,%i - %i,%i)\n",
		     hwndScope, pRectUpdate->left,pRectUpdate->top,
		     pRectUpdate->right,pRectUpdate->bottom);
	if(TRACE_ON(dc))
	  DCE_DumpCache();

 	/* walk all DCEs and fixup non-empty entries */

	for (dce = firstDCE; (dce); dce = dce->next)
	{
            if (dce->DCXflags & DCX_DCEEMPTY) continue;
            if ((dce->hwndCurrent == hwndScope) && !(dce->DCXflags & DCX_CLIPCHILDREN))
                continue;  /* child window positions don't bother us */

            /* check if DCE window is within the z-order scope */

            if (hwndScope == dce->hwndCurrent || IsChild( hwndScope, dce->hwndCurrent ))
            {
                if (hwnd != dce->hwndCurrent)
                {
                    /* check if the window rectangle intersects this DCE window */
                    RECT rect;
                    GetWindowRect( dce->hwndCurrent, &rect );
                    MapWindowPoints( 0, hwndScope, (POINT *)&rect, 2 );
                    if (!IntersectRect( &rect, &rect, pRectUpdate )) continue;

                }
                if( !(dce->DCXflags & DCX_DCEBUSY) )
                {
                    /* Don't bother with visible regions of unused DCEs */

                    TRACE("\tpurged %p dce [%04x]\n", dce, dce->hwndCurrent);
                    if (dce->hwndCurrent && USER_Driver.pReleaseDC)
                        USER_Driver.pReleaseDC( dce->hwndCurrent, dce->hDC );
                    dce->hwndCurrent = 0;
                    dce->DCXflags &= DCX_CACHE;
                    dce->DCXflags |= DCX_DCEEMPTY;
                }
                else
                {
                    /* Set dirty bits in the hDC and DCE structs */

                    TRACE("\tfixed up %p dce [%04x]\n", dce, dce->hwndCurrent);
                    dce->DCXflags |= DCX_DCEDIRTY;
                    SetHookFlags16(dce->hDC, DCHF_INVALIDATEVISRGN);
                    bRet = TRUE;
                }
            }
	} /* dce list */
    }
    return bRet;
}


/***********************************************************************
 *           DCE_ExcludeRgn
 *
 *  Translate given region from the wnd client to the DC coordinates
 *  and add it to the clipping region.
 */
INT DCE_ExcludeRgn( HDC hDC, HWND hwnd, HRGN hRgn )
{
  POINT  pt = {0, 0};
  DCE     *dce = firstDCE;

  while (dce && (dce->hDC != hDC)) dce = dce->next;
  if (!dce) return ERROR;

  MapWindowPoints( hwnd, dce->hwndCurrent, &pt, 1);
  if( dce->DCXflags & DCX_WINDOW )
  {
      WND *wnd = WIN_FindWndPtr(dce->hwndCurrent);
      pt.x += wnd->rectClient.left - wnd->rectWindow.left;
      pt.y += wnd->rectClient.top - wnd->rectWindow.top;
      WIN_ReleaseWndPtr(wnd);
  }
  OffsetRgn(hRgn, pt.x, pt.y);

  return ExtSelectClipRgn( hDC, hRgn, RGN_DIFF );
}


/***********************************************************************
 *		GetDCEx (USER32.@)
 *
 * Unimplemented flags: DCX_LOCKWINDOWUPDATE
 *
 * FIXME: Full support for hrgnClip == 1 (alias for entire window).
 */
HDC WINAPI GetDCEx( HWND hwnd, HRGN hrgnClip, DWORD flags )
{
    HDC 	hdc = 0;
    DCE * 	dce;
    WND * 	wndPtr;
    DWORD 	dcxFlags = 0;
    BOOL	bUpdateVisRgn = TRUE;
    BOOL	bUpdateClipOrigin = FALSE;
    HWND parent, full;

    TRACE("hwnd %04x, hrgnClip %04x, flags %08x\n",
          hwnd, hrgnClip, (unsigned)flags);

    if (!hwnd) hwnd = GetDesktopWindow();
    if (!(full = WIN_IsCurrentProcess( hwnd )))
    {
        FIXME( "not supported yet on other process window %x\n", hwnd );
        return 0;
    }
    hwnd = full;
    if (!(wndPtr = WIN_GetPtr( hwnd ))) return 0;

    /* fixup flags */

    if (flags & (DCX_WINDOW | DCX_PARENTCLIP)) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
	flags &= ~( DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if( wndPtr->dwStyle & WS_CLIPSIBLINGS )
            flags |= DCX_CLIPSIBLINGS;

	if ( !(flags & DCX_WINDOW) )
	{
            if (wndPtr->clsStyle & CS_PARENTDC) flags |= DCX_PARENTCLIP;

	    if (wndPtr->dwStyle & WS_CLIPCHILDREN &&
                     !(wndPtr->dwStyle & WS_MINIMIZE) ) flags |= DCX_CLIPCHILDREN;
            if (!wndPtr->dce) flags |= DCX_CACHE;
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
        if( (wndPtr->dwStyle & WS_VISIBLE) && (parent_style & WS_VISIBLE) )
        {
            flags &= ~DCX_CLIPCHILDREN;
            if (parent_style & WS_CLIPSIBLINGS) flags |= DCX_CLIPSIBLINGS;
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
        dce = wndPtr->dce;
        if (dce && dce->hwndCurrent == hwnd)
	{
	    TRACE("\tskipping hVisRgn update\n");
	    bUpdateVisRgn = FALSE; /* updated automatically, via DCHook() */
	}
    }
    if (!dce)
    {
        hdc = 0;
        goto END;
    }

    if (!(flags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN))) hrgnClip = 0;

    if (((flags ^ dce->DCXflags) & (DCX_INTERSECTRGN | DCX_EXCLUDERGN)) &&
        (dce->hClipRgn != hrgnClip))
    {
        /* if the extra clip region has changed, get rid of the old one */
        DCE_DeleteClipRgn( dce );
    }

    dce->hwndCurrent = hwnd;
    dce->hClipRgn = hrgnClip;
    dce->DCXflags = flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                             DCX_CACHE | DCX_WINDOW | DCX_WINDOWPAINT |
                             DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_EXCLUDERGN);
    dce->DCXflags |= DCX_DCEBUSY;
    dce->DCXflags &= ~DCX_DCEDIRTY;
    hdc = dce->hDC;

    if (bUpdateVisRgn) SetHookFlags16( hdc, DCHF_INVALIDATEVISRGN ); /* force update */

    if (!USER_Driver.pGetDC( hwnd, hdc, hrgnClip, flags )) hdc = 0;

    TRACE("(%04x,%04x,0x%lx): returning %04x\n", hwnd, hrgnClip, flags, hdc);
END:
    WIN_ReleasePtr(wndPtr);
    return hdc;
}


/***********************************************************************
 *		GetDC (USER32.@)
 * RETURNS
 *	:Handle to DC
 *	NULL: Failure
 */
HDC WINAPI GetDC(
	     HWND hwnd /* [in] handle of window */
) {
    if (!hwnd)
        return GetDCEx( 0, 0, DCX_CACHE | DCX_WINDOW );
    return GetDCEx( hwnd, 0, DCX_USESTYLE );
}


/***********************************************************************
 *		GetWindowDC (USER32.@)
 */
HDC WINAPI GetWindowDC( HWND hwnd )
{
    return GetDCEx( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *		ReleaseDC (USER32.@)
 *
 * RETURNS
 *	1: Success
 *	0: Failure
 */
INT WINAPI ReleaseDC(
             HWND hwnd /* [in] Handle of window - ignored */,
             HDC hdc   /* [in] Handle of device context */
) {
    DCE * dce;
    INT nRet = 0;

    USER_Lock();
    dce = firstDCE;

    TRACE("%04x %04x\n", hwnd, hdc );

    while (dce && (dce->hDC != hdc)) dce = dce->next;

    if ( dce )
	if ( dce->DCXflags & DCX_DCEBUSY )
            nRet = DCE_ReleaseDC( dce );

    USER_Unlock();

    return nRet;
}

/***********************************************************************
 *		DCHook (USER.362)
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..
 */
BOOL16 WINAPI DCHook16( HDC16 hDC, WORD code, DWORD data, LPARAM lParam )
{
    BOOL retv = TRUE;
    DCE *dce = (DCE *)data;

    TRACE("hDC = %04x, %i\n", hDC, code);

    if (!dce) return 0;
    assert(dce->hDC == hDC);

    /* Grab the windows lock before doing anything else  */
    USER_Lock();

    switch( code )
    {
      case DCHC_INVALIDVISRGN:
	   /* GDI code calls this when it detects that the
	    * DC is dirty (usually after SetHookFlags()). This
	    * means that we have to recompute the visible region.
	    */
           if( dce->DCXflags & DCX_DCEBUSY )
           {
               /* Dirty bit has been cleared by caller, set it again so that
                * pGetDC recomputes the visible region. */
               SetHookFlags16( dce->hDC, DCHF_INVALIDATEVISRGN );
               USER_Driver.pGetDC( dce->hwndCurrent, dce->hDC, dce->hClipRgn, dce->DCXflags );
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
           else DCE_FreeDCE( dce );
	   break;

      default:
	   FIXME("unknown code\n");
    }

  USER_Unlock();  /* Release the wnd lock */
  return retv;
}


/**********************************************************************
 *		WindowFromDC (USER32.@)
 */
HWND WINAPI WindowFromDC( HDC hDC )
{
    DCE *dce;
    HWND hwnd;

    USER_Lock();
    dce = firstDCE;

    while (dce && (dce->hDC != hDC)) dce = dce->next;

    hwnd = dce ? dce->hwndCurrent : 0;
    USER_Unlock();

    return hwnd;
}


/***********************************************************************
 *		LockWindowUpdate (USER32.@)
 */
BOOL WINAPI LockWindowUpdate( HWND hwnd )
{
    static HWND lockedWnd;

    FIXME("(%x), partial stub!\n",hwnd);

    USER_Lock();
    if (lockedWnd)
    {
        if (!hwnd)
        {
            /* Unlock lockedWnd */
            /* FIXME: Do something */
        }
        else
        {
            /* Attempted to lock a second window */
            /* Return FALSE and do nothing */
            USER_Unlock();
            return FALSE;
        }
    }
    lockedWnd = hwnd;
    USER_Unlock();
    return TRUE;
}
