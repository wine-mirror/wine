/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 *	     1996 Alex Korobka
 *
 *
 * Note: Visible regions of CS_OWNDC/CS_CLASSDC window DCs 
 * have to be updated dynamically. 
 * 
 * Internal DCX flags:
 *
 * DCX_DCEBUSY     - dce structure is in use
 * DCX_KEEPCLIPRGN - do not delete clipping region in ReleaseDC
 * DCX_WINDOWPAINT - BeginPaint specific flag
 */

#include "dce.h"
#include "class.h"
#include "win.h"
#include "gdi.h"
#include "heap.h"
#include "sysmetrics.h"
#include "stddebug.h"
/* #define DEBUG_DC */
#include "debug.h"

#define NB_DCE    5  /* Number of DCEs created at startup */

static DCE *firstDCE = 0;
static HDC32 defaultDCstate = 0;

/***********************************************************************
 *           DCE_AllocDCE
*
 * Allocate a new DCE.
 */
DCE *DCE_AllocDCE( HWND32 hWnd, DCE_TYPE type )
{
    DCE * dce;
    if (!(dce = HeapAlloc( SystemHeap, 0, sizeof(DCE) ))) return NULL;
    if (!(dce->hDC = CreateDC( "DISPLAY", NULL, NULL, NULL )))
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

    if( type != DCE_CACHE_DC )
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
    else dce->DCXflags = DCX_CACHE;

    return dce;
}


/***********************************************************************
 *           DCE_FreeDCE
 */
void DCE_FreeDCE( DCE *dce )
{
    DCE **ppDCE = &firstDCE;

    if (!dce) return;
    while (*ppDCE && (*ppDCE != dce)) ppDCE = &(*ppDCE)->next;
    if (*ppDCE == dce) *ppDCE = dce->next;

    SetDCHook(dce->hDC, NULL, 0L);

    DeleteDC( dce->hDC );
    if( dce->hClipRgn && !(dce->DCXflags & DCX_KEEPCLIPRGN) )
	DeleteObject32(dce->hClipRgn);
    HeapFree( SystemHeap, 0, dce );
}


/**********************************************************************
 *          WindowFromDC16   (USER32.580)
 */
HWND16 WindowFromDC16( HDC16 hDC )
{
    return (HWND16)WindowFromDC32( hDC );
}


/**********************************************************************
 *          WindowFromDC32   (USER32.580)
 */
HWND32 WindowFromDC32( HDC32 hDC )
{
    DCE *dce = firstDCE;
    while (dce && (dce->hDC != hDC)) dce = dce->next;
    return dce ? dce->hwndCurrent : 0;
}


/***********************************************************************
 *   DCE_InvalidateDCE
 *
 * It is called from SetWindowPos - we have to invalidate all busy
 * DCE's for windows whose client rect intersects with update rectangle 
 */
BOOL32 DCE_InvalidateDCE(WND* wndScope, RECT16* pRectUpdate)
{
    BOOL32 bRet = FALSE;
    DCE *dce;

 if( !wndScope ) return 0;

 dprintf_dc(stddeb,"InvalidateDCE: scope hwnd = %04x, (%i,%i - %i,%i)\n",
                    wndScope->hwndSelf, pRectUpdate->left,pRectUpdate->top,
				        pRectUpdate->right,pRectUpdate->bottom);
 /* walk all DCE's */

 for (dce = firstDCE; (dce); dce = dce->next)
  { 
    if( dce->DCXflags & DCX_DCEBUSY )
      {
        WND * wndCurrent, * wnd; 

	wnd = wndCurrent = WIN_FindWndPtr(dce->hwndCurrent);

	/* desktop is not critical (DC is not owned anyway) */

	if( wnd == WIN_GetDesktop() ) continue; 

	/* check if DCE window is within z-order scope */

	for( ; wnd ; wnd = wnd->parent )
	    if( wnd == wndScope )
	      { 
	        RECT16 wndRect = wndCurrent->rectWindow;

	        dprintf_dc(stddeb,"\tgot hwnd %04x\n", wndCurrent->hwndSelf);
  
	        MapWindowPoints16(wndCurrent->parent->hwndSelf, wndScope->hwndSelf,
			 				       (LPPOINT16)&wndRect, 2);
	        if( IntersectRect16(&wndRect,&wndRect,pRectUpdate) )
	        {    
		   SetHookFlags(dce->hDC, DCHF_INVALIDATEVISRGN);
		   bRet = TRUE;
		}
	        break;
	      }
      }
  }
 return bRet;
}

/***********************************************************************
 *           DCE_Init
 */
void DCE_Init()
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
 * Calc the visible rectangle of a window, i.e. the client or
 * window area clipped by the client area of all ancestors.
 * Return FALSE if the visible region is empty.
 */
static BOOL DCE_GetVisRect( WND *wndPtr, BOOL clientArea, RECT16 *lprect )
{
    int xoffset, yoffset;

    *lprect = clientArea ? wndPtr->rectClient : wndPtr->rectWindow;
    xoffset = lprect->left;
    yoffset = lprect->top;

    if (!(wndPtr->dwStyle & WS_VISIBLE) || (wndPtr->flags & WIN_NO_REDRAW))
    {
        SetRectEmpty16( lprect );  /* Clip everything */
        return FALSE;
    }

    while (wndPtr->parent)
    {
        wndPtr = wndPtr->parent;
        if (!(wndPtr->dwStyle & WS_VISIBLE) ||
            (wndPtr->flags & WIN_NO_REDRAW) ||
            (wndPtr->dwStyle & WS_ICONIC))
        {
            SetRectEmpty16( lprect );  /* Clip everything */
            return FALSE;
        }
	xoffset += wndPtr->rectClient.left;
	yoffset += wndPtr->rectClient.top;
	OffsetRect16( lprect, wndPtr->rectClient.left,
                      wndPtr->rectClient.top );

	  /* Warning!! we assume that IntersectRect() handles the case */
	  /* where the destination is the same as one of the sources.  */
	if (!IntersectRect16( lprect, lprect, &wndPtr->rectClient ))
            return FALSE;  /* Visible rectangle is empty */
    }
    OffsetRect16( lprect, -xoffset, -yoffset );
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
static HRGN32 DCE_ClipWindows( WND *pWndStart, WND *pWndEnd,
                               HRGN32 hrgn, int xoffset, int yoffset )
{
    HRGN32 hrgnNew;

    if (!pWndStart) return hrgn;
    if (!(hrgnNew = CreateRectRgn32( 0, 0, 0, 0 )))
    {
        DeleteObject32( hrgn );
        return 0;
    }
    for (; pWndStart != pWndEnd; pWndStart = pWndStart->next)
    {
        if (!(pWndStart->dwStyle & WS_VISIBLE)) continue;
        SetRectRgn( hrgnNew, pWndStart->rectWindow.left + xoffset,
                    pWndStart->rectWindow.top + yoffset,
                    pWndStart->rectWindow.right + xoffset,
                    pWndStart->rectWindow.bottom + yoffset );
        if (!CombineRgn32( hrgn, hrgn, hrgnNew, RGN_DIFF )) break;
    }
    DeleteObject32( hrgnNew );
    if (pWndStart != pWndEnd)  /* something went wrong */
    {
        DeleteObject32( hrgn );
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
HRGN32 DCE_GetVisRgn( HWND hwnd, WORD flags )
{
    RECT16 rect;
    HRGN32 hrgn;
    int xoffset, yoffset;
    WND *wndPtr = WIN_FindWndPtr( hwnd );

      /* Get visible rectangle and create a region with it. 
       * do we really need to calculate vis rgns for X windows? 
       * - yes, to clip child windows but we should skip 
       *   siblings in this case.
       */

    if (!wndPtr || !DCE_GetVisRect( wndPtr, !(flags & DCX_WINDOW), &rect ))
    {
        return CreateRectRgn32( 0, 0, 0, 0 );  /* Visible region is empty */
    }
    if (!(hrgn = CreateRectRgnIndirect16( &rect ))) return 0;

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
        hrgn = DCE_ClipWindows( wndPtr->parent ? wndPtr->parent->child : NULL,
                                wndPtr, hrgn, xoffset, yoffset );
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
 *           DCE_ExcludeRgn
 * 
 *  Translate given region from the wnd client to the DC coordinates
 *  and add it to the clipping region.
 */
INT16 DCE_ExcludeRgn( HDC32 hDC, WND* wnd, HRGN32 hRgn )
{
  INT16	   ret;
  POINT32  pt = {0, 0};
  HRGN32   hRgnClip = GetClipRgn16( hDC );
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
  if( hRgnClip ) ret = CombineRgn32( hRgnClip, hRgnClip, hRgn, RGN_DIFF );
  else 
  {
      hRgnClip = InquireVisRgn( hDC );
      ret = CombineRgn32( hRgn, hRgnClip, hRgn, RGN_DIFF );
      SelectClipRgn32( hDC, hRgn );
  }
  return ret;
}

/***********************************************************************
 *           GetDCEx16    (USER.359)
 */
HDC16 GetDCEx16( HWND16 hwnd, HRGN16 hrgnClip, DWORD flags )
{
    return (HDC16)GetDCEx32( hwnd, hrgnClip, flags );
}


/***********************************************************************
 *           GetDCEx32    (USER32.230)
 *
 * Unimplemented flags: DCX_LOCKWINDOWUPDATE
 */
HDC32 GetDCEx32( HWND32 hwnd, HRGN32 hrgnClip, DWORD flags )
{
    HRGN32 	hrgnVisible;
    HDC32 	hdc = 0;
    DCE * 	dce;
    DC * 	dc;
    WND * 	wndPtr;
    DWORD 	dcx_flags = 0;
    BOOL	need_update = TRUE;

    dprintf_dc(stddeb,"GetDCEx: hwnd %04x, hrgnClip %04x, flags %08x\n", hwnd, hrgnClip, (unsigned)flags);
    
    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;

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

    if (hwnd == GetDesktopWindow32() || !(wndPtr->dwStyle & WS_CHILD))
        flags &= ~DCX_PARENTCLIP;

    if (flags & DCX_WINDOW) flags = (flags & ~DCX_CLIPCHILDREN) | DCX_CACHE;

    if( flags & DCX_PARENTCLIP )
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

    if (flags & DCX_CACHE)
    {
	for (dce = firstDCE; (dce); dce = dce->next)
	{
	    if ((dce->DCXflags & DCX_CACHE) && !(dce->DCXflags & DCX_DCEBUSY)) break;
	}
    }
    else 
    {
        dce = (wndPtr->class->style & CS_OWNDC)?wndPtr->dce:wndPtr->class->dce;
	if( dce->hwndCurrent == hwnd )
	  {
	    dprintf_dc(stddeb,"\tskipping hVisRgn update\n");
	    need_update = FALSE;
	  }

	if( hrgnClip && dce->hClipRgn && !(dce->DCXflags & DCX_KEEPCLIPRGN))
	  {
	    fprintf(stdnimp,"GetDCEx: hClipRgn collision!\n");
            DeleteObject32( dce->hClipRgn ); 
	    need_update = TRUE;
	  }
    }

    dcx_flags = flags & ( DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN | DCX_CACHE | DCX_WINDOW | DCX_WINDOWPAINT);

    if (!dce) return 0;
    dce->hwndCurrent = hwnd;
    dce->hClipRgn = 0;
    dce->DCXflags = dcx_flags | DCX_DCEBUSY;
    hdc = dce->hDC;
    
    if (!(dc = (DC *) GDI_GetObjPtr( hdc, DC_MAGIC ))) return 0;

    DCE_SetDrawable( wndPtr, dc, flags );
    if( need_update || dc->w.flags & DC_DIRTY )
    {
      dprintf_dc(stddeb,"updating hDC anyway\n");

      if (flags & DCX_PARENTCLIP)
        {
            WND *parentPtr = wndPtr->parent;
            dcx_flags = flags & ~(DCX_CLIPSIBLINGS | DCX_CLIPCHILDREN |
                                       DCX_WINDOW);
            if (parentPtr->dwStyle & WS_CLIPSIBLINGS)
                dcx_flags |= DCX_CLIPSIBLINGS;
            hrgnVisible = DCE_GetVisRgn( parentPtr->hwndSelf, dcx_flags );
            if (flags & DCX_WINDOW)
                OffsetRgn32( hrgnVisible, -wndPtr->rectWindow.left,
                                          -wndPtr->rectWindow.top );
            else OffsetRgn32( hrgnVisible, -wndPtr->rectClient.left,
                                           -wndPtr->rectClient.top );
        }
           /* optimize away GetVisRgn for desktop if it isn't there */

      else if ((hwnd == GetDesktopWindow32()) &&
               (rootWindow == DefaultRootWindow(display)))
	       hrgnVisible = CreateRectRgn32( 0, 0, SYSMETRICS_CXSCREEN,
                                                    SYSMETRICS_CYSCREEN );
      else hrgnVisible = DCE_GetVisRgn( hwnd, flags );

      if( wndPtr->parent && wndPtr->window )
      {
        WND* 	wnd = wndPtr->parent->child;
	RECT16  rect;
	
        for( ; wnd != wndPtr; wnd = wnd->next )
           if( wnd->class->style & CS_SAVEBITS &&
               wnd->dwStyle & WS_VISIBLE &&
	       IntersectRect16(&rect, &wndPtr->rectClient, &wnd->rectClient) )
               wnd->flags |= WIN_SAVEUNDER_OVERRIDE;
      }

      dc->w.flags &= ~DC_DIRTY;
      SelectVisRgn( hdc, hrgnVisible );
    }
    else hrgnVisible = CreateRectRgn32( 0, 0, 0, 0 );

    if ((flags & DCX_INTERSECTRGN) || (flags & DCX_EXCLUDERGN))
    {
	dce->DCXflags |= flags & (DCX_KEEPCLIPRGN | DCX_INTERSECTRGN | DCX_EXCLUDERGN);
	dce->hClipRgn = hrgnClip;

	dprintf_dc(stddeb, "\tsaved VisRgn, clipRgn = %04x\n", hrgnClip);

	SaveVisRgn( hdc );
        CombineRgn32( hrgnVisible, InquireVisRgn( hdc ), hrgnClip,
                      (flags & DCX_INTERSECTRGN) ? RGN_AND : RGN_DIFF );
	SelectVisRgn( hdc, hrgnVisible );
    }
    DeleteObject32( hrgnVisible );

    dprintf_dc(stddeb, "GetDCEx(%04x,%04x,0x%lx): returning %04x\n", 
	       hwnd, hrgnClip, flags, hdc);
    return hdc;
}


/***********************************************************************
 *           GetDC16    (USER.66)
 */
HDC16 GetDC16( HWND16 hwnd )
{
    return (HDC16)GetDC32( hwnd );
}


/***********************************************************************
 *           GetDC32    (USER32.229)
 */
HDC32 GetDC32( HWND32 hwnd )
{
    if (!hwnd)
        return GetDCEx32( GetDesktopWindow32(), 0, DCX_CACHE | DCX_WINDOW );
    return GetDCEx32( hwnd, 0, DCX_USESTYLE );
}


/***********************************************************************
 *           GetWindowDC16    (USER.67)
 */
HDC16 GetWindowDC16( HWND16 hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow16();
    return GetDCEx16( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *           GetWindowDC32    (USER32.)
 */
HDC32 GetWindowDC32( HWND32 hwnd )
{
    if (!hwnd) hwnd = GetDesktopWindow32();
    return GetDCEx32( hwnd, 0, DCX_USESTYLE | DCX_WINDOW );
}


/***********************************************************************
 *           ReleaseDC16    (USER.68)
 */
INT16 ReleaseDC16( HWND16 hwnd, HDC16 hdc )
{
    return (INT32)ReleaseDC32( hwnd, hdc );
}


/***********************************************************************
 *           ReleaseDC32    (USER32.439)
 */
INT32 ReleaseDC32( HWND32 hwnd, HDC32 hdc )
{
    DCE * dce = firstDCE;
    
    dprintf_dc(stddeb, "ReleaseDC: %04x %04x\n", hwnd, hdc );
        
    while (dce && (dce->hDC != hdc)) dce = dce->next;
    if (!dce) return 0;
    if (!(dce->DCXflags & DCX_DCEBUSY) ) return 0;

    /* restore previous visible region */

    if ( dce->DCXflags & (DCX_INTERSECTRGN | DCX_EXCLUDERGN) &&
	(dce->DCXflags & DCX_CACHE || dce->DCXflags & DCX_WINDOWPAINT) )
    {
	dprintf_dc(stddeb,"\tcleaning up visrgn...\n");
	dce->DCXflags &= ~(DCX_EXCLUDERGN | DCX_INTERSECTRGN | DCX_WINDOWPAINT);

	if( dce->DCXflags & DCX_KEEPCLIPRGN )
	    dce->DCXflags &= ~DCX_KEEPCLIPRGN;
	else
	    if( dce->hClipRgn > 1 )
	        DeleteObject32( dce->hClipRgn );

        dce->hClipRgn = 0;
	RestoreVisRgn(dce->hDC);
    }

    if (dce->DCXflags & DCX_CACHE)
    {
	SetDCState( dce->hDC, defaultDCstate );
	dce->DCXflags = DCX_CACHE;
	dce->hwndCurrent = 0;
    }
    return 1;
}

/***********************************************************************
 *           DCHook    (USER.362)
 *
 * See "Undoc. Windows" for hints (DC, SetDCHook, SetHookFlags)..  
 */
BOOL16 DCHook( HDC16 hDC, WORD code, DWORD data, LPARAM lParam )
{
    HRGN32 hVisRgn;
    DCE *dce = firstDCE;;

    dprintf_dc(stddeb,"DCHook: hDC = %04x, %i\n", hDC, code);

    while (dce && (dce->hDC != hDC)) dce = dce->next;
    if (!dce) return 0;

  switch( code )
    {
      case DCHC_INVALIDVISRGN:
         {
           if( dce->DCXflags & DCX_DCEBUSY )
 	     {
	       SetHookFlags(hDC, DCHF_VALIDATEVISRGN);
	       hVisRgn = DCE_GetVisRgn(dce->hwndCurrent, dce->DCXflags);

	       dprintf_dc(stddeb,"\tapplying saved clipRgn\n");
  
	       /* clip this region with saved clipping region */

               if ( (dce->DCXflags & DCX_INTERSECTRGN && dce->hClipRgn != 1) ||
                  (  dce->DCXflags & DCX_EXCLUDERGN && dce->hClipRgn) )
                  {

                    if( (!dce->hClipRgn && dce->DCXflags & DCX_INTERSECTRGN) ||
                         (dce->hClipRgn == 1 && dce->DCXflags & DCX_EXCLUDERGN) )            
                         SetRectRgn(hVisRgn,0,0,0,0);
                    else
                         CombineRgn32(hVisRgn, hVisRgn, dce->hClipRgn, 
                                      (dce->DCXflags & DCX_EXCLUDERGN)? RGN_DIFF:RGN_AND);
	          }  
	       SelectVisRgn(hDC, hVisRgn);
	       DeleteObject32( hVisRgn );
	     }
           else
	     dprintf_dc(stddeb,"DCHook: DC is not in use!\n");
         }
	 break;

      case DCHC_DELETEDC: /* FIXME: ?? */
	 break;

      default:
	 fprintf(stdnimp,"DCHook: unknown code\n");
    }
  return 0;
}

