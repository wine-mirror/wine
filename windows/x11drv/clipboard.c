/*
 * X11 windows driver
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xatom.h>
#include "ts_xlib.h"

#include "wine/winuser16.h"
#include "clipboard.h"
#include "debugtools.h"
#include "message.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"

DEFAULT_DEBUG_CHANNEL(clipboard)

extern HWND hWndClipOwner;
extern HWND hWndClipWindow;
extern WINE_CLIPFORMAT ClipFormats[];

static Bool   selectionAcquired = False;
static Window selectionWindow = None;
static Window selectionPrevWindow = None;

/**************************************************************************
 * 		X11DRV_CLIPBOARD_CheckSelection	[Internal]
 *
 * Prevent X selection from being lost when a top level window is
 * destroyed.
 */
static void X11DRV_CLIPBOARD_CheckSelection(WND* pWnd)
{
    TRACE("\tchecking %08x\n",
        (unsigned) X11DRV_WND_GetXWindow(pWnd)
    );

    if( selectionAcquired && selectionWindow != None &&
        X11DRV_WND_GetXWindow(pWnd) == selectionWindow )
    {
	selectionPrevWindow = selectionWindow;
	selectionWindow = None;

	if( pWnd->next ) 
	    selectionWindow = X11DRV_WND_GetXWindow(pWnd->next);
	else if( pWnd->parent )
             if( pWnd->parent->child != pWnd ) 
                 selectionWindow = X11DRV_WND_GetXWindow(pWnd->parent->child);

	TRACE("\tswitching selection from %08x to %08x\n", 
                    (unsigned)selectionPrevWindow, (unsigned)selectionWindow);

	if( selectionWindow != None )
	{
	    TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);
	    if( TSXGetSelectionOwner(display, XA_PRIMARY) != selectionWindow )
		selectionWindow = None;
	}
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ReadSelection
 */
static void X11DRV_CLIPBOARD_ReadSelection(Window w, Atom prop)
{
    HANDLE 	 hText = 0;
    LPWINE_CLIPFORMAT lpFormat = ClipFormats; 

    TRACE("Reading X selection...\n");

    if(prop != None)
    {
	Atom		atype=AnyPropertyType;
	int		aformat;
	unsigned long 	nitems,remain;
	unsigned char*	val=NULL;

        TRACE("\tgot property %s\n",TSXGetAtomName(display,prop));

        /* TODO: Properties longer than 64K */

	if(TSXGetWindowProperty(display,w,prop,0,0x3FFF,True,XA_STRING,
	    &atype, &aformat, &nitems, &remain, &val) != Success)
	    WARN("\tcouldn't read property\n");
	else
	{
           TRACE("\tType %s,Format %d,nitems %ld,value %s\n",
		             TSXGetAtomName(display,atype),aformat,nitems,val);

	   if(atype == XA_STRING && aformat == 8)
	   {
	      int 	i,inlcount = 0;
	      char*	lpstr;

	      TRACE("\tselection is '%s'\n",val);

	      for(i=0; i <= nitems; i++)
		  if( val[i] == '\n' ) inlcount++;

	      if( nitems )
	      {
	        hText=GlobalAlloc(GMEM_MOVEABLE, nitems + inlcount + 1);
	        if( (lpstr = (char*)GlobalLock(hText)) )
	          for(i=0,inlcount=0; i <= nitems; i++)
	          {
	  	     if( val[i] == '\n' ) lpstr[inlcount++]='\r';
		     lpstr[inlcount++]=val[i];
		  }
	        else hText = 0;
	      }
	   }
	   TSXFree(val);
	}
   }

   /* delete previous CF_TEXT and CF_OEMTEXT data */

   if( hText )
   {
       lpFormat = &ClipFormats[CF_TEXT-1];
       if (lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32) 
           CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
       lpFormat = &ClipFormats[CF_OEMTEXT-1];
       if (lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32)  
           CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
       lpFormat->wDataPresent = 1;
       lpFormat->hData32 = hText;
       lpFormat->hData16 = 0;
   }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ReleaseSelection
 *
 * Wine might have lost XA_PRIMARY selection because of
 * EmptyClipboard() or other client. 
 */
void X11DRV_CLIPBOARD_ReleaseSelection(Window w, HWND hwnd)
{
    /* w is the window that lost selection,
     * 
     * selectionPrevWindow is nonzero if CheckSelection() was called. 
     */

    TRACE("\tevent->window = %08x (sw = %08x, spw=%08x)\n", 
	  (unsigned)w, (unsigned)selectionWindow, (unsigned)selectionPrevWindow );

    if( selectionAcquired )
    {
	if( w == selectionWindow || selectionPrevWindow == None)
	{
	    /* alright, we really lost it */

	    selectionAcquired = False;
	    selectionWindow = None; 

	    /* but we'll keep existing data for internal use */
	}
	else if( w == selectionPrevWindow )
	{
	    w = TSXGetSelectionOwner(display, XA_PRIMARY);
	    if( w == None )
		TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);
	}
    }

    selectionPrevWindow = None;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_Empty
 */
void X11DRV_CLIPBOARD_Empty()
{
    if( selectionAcquired )
    {
	XEvent xe;

	selectionAcquired   = False;
	selectionPrevWindow = selectionWindow;
	selectionWindow     = None;
      
	TRACE("\tgiving up selection (spw = %08x)\n", 
	     (unsigned)selectionPrevWindow);
      
	EnterCriticalSection(&X11DRV_CritSection);
	XSetSelectionOwner(display, XA_PRIMARY, None, CurrentTime);

	if( selectionPrevWindow )
	    while( !XCheckTypedWindowEvent( display, selectionPrevWindow,
					    SelectionClear, &xe ) );
	LeaveCriticalSection(&X11DRV_CritSection);
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_SetData
 */
void X11DRV_CLIPBOARD_SetData(UINT wFormat)
{
    Window       owner;

    /* Acquire X selection if text format */

    if( !selectionAcquired && 
	(wFormat == CF_TEXT || wFormat == CF_OEMTEXT) )
    {
        WND *tmpWnd = WIN_FindWndPtr( hWndClipWindow ? hWndClipWindow : AnyPopup() );
	owner = X11DRV_WND_FindXWindow(tmpWnd );

	TSXSetSelectionOwner(display,XA_PRIMARY, owner, CurrentTime);
	if( TSXGetSelectionOwner(display,XA_PRIMARY) == owner )
	{
	    selectionAcquired = True;
	    selectionWindow = owner;

	    TRACE("Grabbed X selection, owner=(%08x)\n", 
						(unsigned) owner);
	}
        WIN_ReleaseWndPtr(tmpWnd);
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_GetData
 *
 * NOTE: Clipboard driver doesn't get requests for CF_TEXT data, only
 *	 for CF_OEMTEXT.
 */
BOOL X11DRV_CLIPBOARD_GetData(UINT wFormat)
{
    BOOL bRet = selectionAcquired;
    HWND hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow();
    WND* wnd = NULL;

    if( wFormat != CF_OEMTEXT ) return FALSE;

    if( !bRet && (wnd = WIN_FindWndPtr(hWnd)) )
    {
	XEvent xe;
	Window w = X11DRV_WND_FindXWindow(wnd);

	TRACE("Requesting XA_STRING selection...\n");

	EnterCriticalSection( &X11DRV_CritSection );
	XConvertSelection(display, XA_PRIMARY, XA_STRING,
			XInternAtom(display, "PRIMARY_TEXT", False),
			w, CurrentTime);
    
        /* wait until SelectionNotify is received */

	while( TRUE )
	{
	   if( XCheckTypedWindowEvent(display, w, SelectionNotify, &xe) )
	       if( xe.xselection.selection == XA_PRIMARY )
		   break;
	}
	LeaveCriticalSection( &X11DRV_CritSection );

	if (xe.xselection.target != XA_STRING) 
	    X11DRV_CLIPBOARD_ReadSelection( 0, None );
	else 
	    X11DRV_CLIPBOARD_ReadSelection( xe.xselection.requestor, 
					    xe.xselection.property );

	/* treat Unix text as CF_OEMTEXT */

	bRet = (BOOL)ClipFormats[CF_OEMTEXT-1].wDataPresent;

	TRACE("\tpresent CF_OEMTEXT = %i\n", bRet );
	WIN_ReleaseWndPtr(wnd);
    }
    return bRet;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ResetOwner
 *
 * Called from DestroyWindow().
 */
void X11DRV_CLIPBOARD_ResetOwner(WND *pWnd, BOOL bFooBar)
{
    LPWINE_CLIPFORMAT lpFormat = ClipFormats;

    if(bFooBar && X11DRV_WND_GetXWindow(pWnd))
      return;

    if(!bFooBar && !X11DRV_WND_GetXWindow(pWnd))
      return;

    TRACE("clipboard owner = %04x, selection = %08x\n", 
				hWndClipOwner, (unsigned)selectionWindow);

    if( pWnd->hwndSelf == hWndClipOwner)
    {
	SendMessage16(hWndClipOwner,WM_RENDERALLFORMATS,0,0L);

	/* check if all formats were rendered */

	while(lpFormat)
	{
	    if( lpFormat->wDataPresent && !lpFormat->hData16 && !lpFormat->hData32 )
	    {
		TRACE("\tdata missing for clipboard format %i\n", 
				   lpFormat->wFormatID); 
		lpFormat->wDataPresent = 0;
	    }
	    lpFormat = lpFormat->NextFormat;
	}
	hWndClipOwner = 0;
    }

    /* now try to salvage current selection from being destroyed by X */

    if( X11DRV_WND_GetXWindow(pWnd) ) X11DRV_CLIPBOARD_CheckSelection(pWnd);
}

#endif /* !defined(X_DISPLAY_MISSING) */
