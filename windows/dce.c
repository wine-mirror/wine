/*
 * USER DCE functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include "dce.h"
#include "win.h"
#include "gdi.h"


#define NB_DCE    5  /* Number of DCEs created at startup */

extern Display * XT_display;
extern Screen * XT_screen;

static HANDLE firstDCE = 0;
static HDC defaultDCstate = 0;


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
	handle = GlobalAlloc( GMEM_MOVEABLE, sizeof(DCE) );
	if (!handle) return;
	dce = (DCE *) GlobalLock( handle );
	dce->hdc = CreateDC( "DISPLAY", NULL, NULL, NULL );
	if (!dce->hdc)
	{
	    GlobalUnlock( handle );
	    GlobalFree( handle );
	    return;
	}
	dce->hwndCurrent = 0;
	dce->flags = 0;
	dce->inUse = FALSE;
	dce->xOrigin = 0;
	dce->yOrigin = 0;
	dce->hNext = firstDCE;
	firstDCE = handle;
	if (!defaultDCstate) defaultDCstate = GetDCState( dce->hdc );
	GlobalUnlock( handle );
    }
}


/***********************************************************************
 *           GetDC    (USER.66)
 */
HDC GetDC( HWND hwnd )
{
    HANDLE hdce, next;
    HDC hdc;
    DCE * dce;
    DC * dc;
    WND * wndPtr = NULL;
    
    if (hwnd)
    {
	wndPtr = WIN_FindWndPtr( hwnd );
	if (!wndPtr) return 0;
    }
        
    for (hdce = firstDCE; (hdce); hdce = next)
    {
	dce = (DCE *) GlobalLock( hdce );
	if (!dce) return 0;
	if (!dce->inUse) break;
	next = dce->hNext;
	GlobalUnlock( hdce );
    }

    if (!hdce)
    {
	if (hwnd) GlobalUnlock( hwnd );
	return 0;
    }
    
      /* Initialize DC */
    
    dc = (DC *) GDI_GetObjPtr( dce->hdc, DC_MAGIC );
    if (!dc)
    {
	if (hwnd) GlobalUnlock( hwnd );
	return 0;
    }
    if (wndPtr)
    {
	dc->u.x.drawable = XtWindow( wndPtr->winWidget );
    	dc->u.x.widget   = wndPtr->winWidget;
    }
    else
    {
	dc->u.x.drawable = DefaultRootWindow( XT_display );
	dc->u.x.widget   = 0;
    }   
    SetDCState( dce->hdc, defaultDCstate );

    dce->hwndCurrent = hwnd;
    dce->inUse       = TRUE;
    hdc = dce->hdc;
    GlobalUnlock( hdce );
    if (hwnd) GlobalUnlock( hwnd );
#ifdef DEBUG_WIN
    printf( "GetDC(%d): returning %d\n", hwnd, hdc );
#endif
    return hdc;
}


/***********************************************************************
 *           ReleaseDC    (USER.68)
 */
int ReleaseDC( HWND hwnd, HDC hdc )
{
    HANDLE hdce, next;
    DCE * dce;
    WND * wndPtr = NULL;
    
#ifdef DEBUG_WIN
    printf( "ReleaseDC: %d %d\n", hwnd, hdc );
#endif
        
    if (hwnd)
    {
	wndPtr = WIN_FindWndPtr( hwnd );
	if (!wndPtr) return 0;
    }

    for (hdce = firstDCE; (hdce); hdce = next)
    {
	dce = (DCE *) GlobalLock( hdce );
	if (!dce) return 0;
	if (dce->inUse && (dce->hdc == hdc)) break;
	next = dce->hNext;
	GlobalUnlock( hdce );
    }

    if (hdce) 
    {
	dce->inUse = FALSE;
    	GlobalUnlock( hdce );
    }

    if (hwnd) GlobalUnlock( hwnd );
    return (hdce != 0);
}
