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
	handle = USER_HEAP_ALLOC( GMEM_MOVEABLE, sizeof(DCE) );
	if (!handle) return;
	dce = (DCE *) USER_HEAP_ADDR( handle );
	dce->hdc = CreateDC( "DISPLAY", NULL, NULL, NULL );
	if (!dce->hdc)
	{
	    USER_HEAP_FREE( handle );
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
    }
}


/***********************************************************************
 *           GetDC    (USER.66)
 */
HDC GetDC( HWND hwnd )
{
    HANDLE hdce;
    HDC hdc = 0;
    DCE * dce;
    DC * dc;
    WND * wndPtr = NULL;
    CLASS * classPtr;
    
    if (hwnd)
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
	if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 0;
	if (wndPtr->hdc) hdc = wndPtr->hdc;
	else if (classPtr->hdc) hdc = classPtr->hdc;
    }
        
    if (!hdc)
    {
	for (hdce = firstDCE; (hdce); hdce = dce->hNext)
	{
	    dce = (DCE *) USER_HEAP_ADDR( hdce );
	    if (!dce) return 0;
	    if (!dce->inUse) break;
	}
	if (!hdce) return 0;
	dce->hwndCurrent = hwnd;
	dce->inUse       = TRUE;
	hdc = dce->hdc;
    }
    
      /* Initialize DC */
    
    if (!(dc = (DC *) GDI_GetObjPtr( dce->hdc, DC_MAGIC ))) return 0;

    if (wndPtr)
    {
	dc->u.x.drawable = XtWindow( wndPtr->winWidget );
    	dc->u.x.widget   = wndPtr->winWidget;
	if (wndPtr->dwStyle & WS_CLIPCHILDREN)
	    XSetSubwindowMode( XT_display, dc->u.x.gc, ClipByChildren );
	else XSetSubwindowMode( XT_display, dc->u.x.gc, IncludeInferiors);
    }
    else
    {
	dc->u.x.drawable = DefaultRootWindow( XT_display );
	dc->u.x.widget   = 0;
	XSetSubwindowMode( XT_display, dc->u.x.gc, IncludeInferiors );    
    }   

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
    CLASS * classPtr;
    
#ifdef DEBUG_WIN
    printf( "ReleaseDC: %d %d\n", hwnd, hdc );
#endif
        
    if (hwnd)
    {
	if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 0;
	if (wndPtr->hdc && (wndPtr->hdc == hdc)) return 1;
	if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 0;
	if (classPtr->hdc && (classPtr->hdc == hdc)) return 1;
    }

    for (hdce = firstDCE; (hdce); hdce = dce->hNext)
    {
	if (!(dce = (DCE *) USER_HEAP_ADDR( hdce ))) return 0;
	if (dce->inUse && (dce->hdc == hdc)) break;
    }

    if (hdce)
    {
	SetDCState( dce->hdc, defaultDCstate );
	dce->inUse = FALSE;
    }    
    return (hdce != 0);
}
