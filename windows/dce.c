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
 * DCX_WINDOWPAINT - BeginPaint() is in effect
 */

#include <assert.h>
#include "dce.h"
#include "win.h"
#include "user_private.h"
#include "windef.h"
#include "wingdi.h"
#include "wownt32.h"
#include "wine/winbase16.h"
#include "wine/winuser16.h"
#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(dc);

typedef struct tagDCE
{
    struct list entry;
    HDC         hDC;
    HWND        hwndCurrent;
    HRGN        hClipRgn;
    DCE_TYPE    type;
    DWORD       DCXflags;
} DCE;

static struct list dce_list = LIST_INIT(dce_list);

static void DCE_DeleteClipRgn( DCE* );
static INT DCE_ReleaseDC( DCE* );


/***********************************************************************
 *           DCE_DumpCache
 */
static void DCE_DumpCache(void)
{
    DCE *dce;

    USER_Lock();

    LIST_FOR_EACH_ENTRY( dce, &dce_list, DCE, entry )
    {
	TRACE("\t[0x%08x] hWnd %p, dcx %08x, %s %s\n",
	      (unsigned)dce, dce->hwndCurrent, (unsigned)dce->DCXflags,
	      (dce->DCXflags & DCX_CACHE) ? "Cache" : "Owned",
	      (dce->DCXflags & DCX_DCEBUSY) ? "InUse" : "" );
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
    static const WCHAR szDisplayW[] = { 'D','I','S','P','L','A','Y','\0' };
    DCE * dce;

    TRACE("(%p,%d)\n", hWnd, type);

    if (!(dce = HeapAlloc( GetProcessHeap(), 0, sizeof(DCE) ))) return NULL;
    if (!(dce->hDC = CreateDCW( szDisplayW, NULL, NULL, NULL )))
    {
        HeapFree( GetProcessHeap(), 0, dce );
	return 0;
    }
    SaveDC( dce->hDC );

    /* store DCE handle in DC hook data field */

    SetDCHook( dce->hDC, DCHook16, (DWORD)dce );

    dce->hwndCurrent = hWnd;
    dce->hClipRgn    = 0;
    dce->type        = type;

    if( type != DCE_CACHE_DC ) /* owned or class DC */
    {
	dce->DCXflags = DCX_DCEBUSY;
	if( hWnd )
	{
            LONG style = GetWindowLongW( hWnd, GWL_STYLE );
            if (style & WS_CLIPCHILDREN) dce->DCXflags |= DCX_CLIPCHILDREN;
            if (style & WS_CLIPSIBLINGS) dce->DCXflags |= DCX_CLIPSIBLINGS;
	}
        SetHookFlags16( HDC_16(dce->hDC), DCHF_INVALIDATEVISRGN );
    }
    else dce->DCXflags = DCX_CACHE | DCX_DCEEMPTY;

    USER_Lock();
    list_add_head( &dce_list, &dce->entry );
    USER_Unlock();
    return dce;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
void DCE_FreeDCE( DCE *dce )
{
    if (!dce) return;

    USER_Lock();
    list_remove( &dce->entry );
    USER_Unlock();

    SetDCHook(dce->hDC, NULL, 0L);

    DeleteDC( dce->hDC );
    if (dce->hClipRgn) DeleteObject(dce->hClipRgn);
    HeapFree( GetProcessHeap(), 0, dce );
}

/***********************************************************************
 *           DCE_FreeWindowDCE
 *
 * Remove owned DCE and reset unreleased cache DCEs.
 */
void DCE_FreeWindowDCE( HWND hwnd )
{
    struct list *ptr, *next;
    WND *pWnd = WIN_GetPtr( hwnd );

    LIST_FOR_EACH_SAFE( ptr, next, &dce_list )
    {
        DCE *pDCE = LIST_ENTRY( ptr, DCE, entry );

        if (pDCE->hwndCurrent != hwnd) continue;

        switch( pDCE->type )
        {
        case DCE_WINDOW_DC:
            DCE_FreeDCE( pDCE );
            pWnd->dce = NULL;
            break;
        case DCE_CLASS_DC:
            if( pDCE->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN) )
            {
                if (USER_Driver.pReleaseDC)
                    USER_Driver.pReleaseDC( pDCE->hwndCurrent, pDCE->hDC );
                DCE_DeleteClipRgn( pDCE );
                pDCE->hwndCurrent = 0;
            }
            break;
        case DCE_CACHE_DC:
            if( pDCE->DCXflags & DCX_DCEBUSY ) /* shared cache DCE */
            {
                WARN("[%p] GetDC() without ReleaseDC()!\n",hwnd);
                DCE_ReleaseDC( pDCE );
            }
            if (pDCE->hwndCurrent && USER_Driver.pReleaseDC)
                USER_Driver.pReleaseDC( pDCE->hwndCurrent, pDCE->hDC );
            pDCE->DCXflags &= DCX_CACHE;
            pDCE->DCXflags |= DCX_DCEEMPTY;
            pDCE->hwndCurrent = 0;
            break;
        }
    }
    WIN_ReleasePtr( pWnd );
}


/***********************************************************************
 *   DCE_DeleteClipRgn
 */
static void DCE_DeleteClipRgn( DCE* dce )
{
    dce->DCXflags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

    if (dce->hClipRgn) DeleteObject( dce->hClipRgn );
    dce->hClipRgn = 0;

    /* make it dirty so that the vis rgn gets recomputed next time */
    dce->DCXflags |= DCX_DCEDIRTY;
    SetHookFlags16( HDC_16(dce->hDC), DCHF_INVALIDATEVISRGN );
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
        /* make the DC clean so that RestoreDC doesn't try to update the vis rgn */
        SetHookFlags16( HDC_16(dce->hDC), DCHF_VALIDATEVISRGN );
        RestoreDC( dce->hDC, 1 );  /* initial save level is always 1 */
        SaveDC( dce->hDC );  /* save the state again for next time */
        dce->DCXflags &= ~DCX_DCEBUSY;
	if (dce->DCXflags & DCX_DCEDIRTY)
	{
	    /* don't keep around invalidated entries
	     * because RestoreDC() disables hVisRgn updates
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

        TRACE("scope hwnd = %p, (%ld,%ld - %ld,%ld)\n",
              hwndScope, pRectUpdate->left,pRectUpdate->top,
              pRectUpdate->right,pRectUpdate->bottom);
	if(TRACE_ON(dc))
	  DCE_DumpCache();

 	/* walk all DCEs and fixup non-empty entries */

        LIST_FOR_EACH_ENTRY( dce, &dce_list, DCE, entry )
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

                    TRACE("\tpurged %p dce [%p]\n", dce, dce->hwndCurrent);
                    if (dce->hwndCurrent && USER_Driver.pReleaseDC)
                        USER_Driver.pReleaseDC( dce->hwndCurrent, dce->hDC );
                    dce->hwndCurrent = 0;
                    dce->DCXflags &= DCX_CACHE;
                    dce->DCXflags |= DCX_DCEEMPTY;
                }
                else
                {
                    /* Set dirty bits in the hDC and DCE structs */

                    TRACE("\tfixed up %p dce [%p]\n", dce, dce->hwndCurrent);
                    dce->DCXflags |= DCX_DCEDIRTY;
                    SetHookFlags16( HDC_16(dce->hDC), DCHF_INVALIDATEVISRGN );
                    bRet = TRUE;
                }
            }
	} /* dce list */
    }
    return bRet;
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
    HWND parent;
    LONG window_style;

    TRACE("hwnd %p, hrgnClip %p, flags %08lx\n", hwnd, hrgnClip, flags);

    if (flags & (DCX_LOCKWINDOWUPDATE)) {
       FIXME("not yet supported - see source\n");
       /* See the comment in LockWindowUpdate for more explanation. This flag is not implemented
        * by that patch, but we need LockWindowUpdate implemented correctly before this can be done.
        */
    }

    if (!hwnd) hwnd = GetDesktopWindow();
    else hwnd = WIN_GetFullHandle( hwnd );

    if (!(wndPtr = WIN_GetPtr( hwnd ))) return 0;
    if (wndPtr == WND_OTHER_PROCESS)
    {
        wndPtr = NULL;
        USER_Lock();
    }

    window_style = GetWindowLongW( hwnd, GWL_STYLE );

    /* fixup flags */

    if (flags & (DCX_WINDOW | DCX_PARENTCLIP)) flags |= DCX_CACHE;

    if (flags & DCX_USESTYLE)
    {
	flags &= ~( DCX_CLIPCHILDREN | DCX_CLIPSIBLINGS | DCX_PARENTCLIP);

        if( window_style & WS_CLIPSIBLINGS )
            flags |= DCX_CLIPSIBLINGS;

	if ( !(flags & DCX_WINDOW) )
	{
            if (GetClassLongW( hwnd, GCL_STYLE ) & CS_PARENTDC) flags |= DCX_PARENTCLIP;

            if (window_style & WS_CLIPCHILDREN && !(window_style & WS_MINIMIZE))
                flags |= DCX_CLIPCHILDREN;
            if (!wndPtr || !wndPtr->dce) flags |= DCX_CACHE;
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

    dcxFlags = flags & (DCX_PARENTCLIP | DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
		        DCX_CACHE | DCX_WINDOW);

    if (flags & DCX_CACHE)
    {
        DCE *dceEmpty = NULL, *dceUnused = NULL;

	/* Strategy: First, we attempt to find a non-empty but unused DCE with
	 * compatible flags. Next, we look for an empty entry. If the cache is
	 * full we have to purge one of the unused entries.
	 */

        LIST_FOR_EACH_ENTRY( dce, &dce_list, DCE, entry )
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
		    TRACE("\tfound valid %p dce [%p], flags %08lx\n",
                          dce, hwnd, dcxFlags );
		    bUpdateVisRgn = FALSE;
		    bUpdateClipOrigin = TRUE;
		    break;
		}
	    }
	}

        if (&dce->entry == &dce_list)  /* nothing found */
            dce = dceEmpty ? dceEmpty : dceUnused;

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
                             DCX_INTERSECTRGN | DCX_EXCLUDERGN);
    dce->DCXflags |= DCX_DCEBUSY;
    dce->DCXflags &= ~DCX_DCEDIRTY;
    hdc = dce->hDC;

    if (bUpdateVisRgn) SetHookFlags16( HDC_16(hdc), DCHF_INVALIDATEVISRGN ); /* force update */

    if (!USER_Driver.pGetDC || !USER_Driver.pGetDC( hwnd, hdc, hrgnClip, flags ))
        hdc = 0;

    TRACE("(%p,%p,0x%lx): returning %p\n", hwnd, hrgnClip, flags, hdc);
END:
    if (wndPtr) WIN_ReleasePtr(wndPtr);
    else USER_Unlock();
    return hdc;
}


/***********************************************************************
 *		GetDC (USER32.@)
 *
 * Get a device context.
 *
 * RETURNS
 *	Success: Handle to the device context
 *	Failure: NULL.
 */
HDC WINAPI GetDC( HWND hwnd )
{
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
 * Release a device context.
 *
 * RETURNS
 *	Success: Non-zero. Resources used by hdc are released.
 *	Failure: 0.
 */
INT WINAPI ReleaseDC( HWND hwnd, HDC hdc )
{
    DCE * dce;
    INT nRet = 0;

    TRACE("%p %p\n", hwnd, hdc );

    USER_Lock();
    LIST_FOR_EACH_ENTRY( dce, &dce_list, DCE, entry )
    {
        if (dce->hDC == hdc)
        {
            if ( dce->DCXflags & DCX_DCEBUSY )
                nRet = DCE_ReleaseDC( dce );
            break;
        }

    }
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
    assert( HDC_16(dce->hDC) == hDC );

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
               SetHookFlags16( hDC, DCHF_INVALIDATEVISRGN );
               if (USER_Driver.pGetDC)
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
    HWND hwnd = 0;

    USER_Lock();
    LIST_FOR_EACH_ENTRY( dce, &dce_list, DCE, entry )
    {
        if (dce->hDC == hDC)
        {
            hwnd = dce->hwndCurrent;
            break;
        }
    }
    USER_Unlock();

    return hwnd;
}


/***********************************************************************
 *		LockWindowUpdate (USER32.@)
 */
BOOL WINAPI LockWindowUpdate( HWND hwnd )
{
    static HWND lockedWnd;

    /* This function is fully implemented by the following patch:
     *
     * http://www.winehq.org/hypermail/wine-patches/2004/01/0142.html
     *
     * but in order to work properly, it needs the ability to invalidate
     * DCEs in other processes when the lock window is changed, which
     * isn't possible yet.
     * -mike
     */

    FIXME("(%p), partial stub!\n",hwnd);

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
