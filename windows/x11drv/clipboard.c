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

#include "windows.h"
#include "wintypes.h"

#include "clipboard.h"
#include "debug.h"
#include "message.h"
#include "win.h"
#include "x11drv.h"

extern HWND32 hWndClipOwner;
extern HWND32 hWndClipWindow;
extern CLIPFORMAT ClipFormats[];

static Bool   selectionWait = False;
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
    TRACE(clipboard,"\tchecking %08x\n",
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

	TRACE(clipboard,"\tswitching selection from %08x to %08x\n", 
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
 *
 * Called from the SelectionNotify event handler. 
 */
void X11DRV_CLIPBOARD_ReadSelection(Window w,Atom prop)
{
    HANDLE32 	 hText = 0;
    LPCLIPFORMAT lpFormat = ClipFormats; 

    TRACE(clipboard,"ReadSelection callback\n");

    if(prop != None)
    {
	Atom		atype=AnyPropertyType;
	int		aformat;
	unsigned long 	nitems,remain;
	unsigned char*	val=NULL;

        TRACE(clipboard,"\tgot property %s\n",TSXGetAtomName(display,prop));

        /* TODO: Properties longer than 64K */

	if(TSXGetWindowProperty(display,w,prop,0,0x3FFF,True,XA_STRING,
	    &atype, &aformat, &nitems, &remain, &val) != Success)
	    WARN(clipboard, "\tcouldn't read property\n");
	else
	{
           TRACE(clipboard,"\tType %s,Format %d,nitems %ld,value %s\n",
		             TSXGetAtomName(display,atype),aformat,nitems,val);

	   if(atype == XA_STRING && aformat == 8)
	   {
	      int 	i,inlcount = 0;
	      char*	lpstr;

	      TRACE(clipboard,"\tselection is '%s'\n",val);

	      for(i=0; i <= nitems; i++)
		  if( val[i] == '\n' ) inlcount++;

	      if( nitems )
	      {
	        hText=GlobalAlloc32(GMEM_MOVEABLE, nitems + inlcount + 1);
	        if( (lpstr = (char*)GlobalLock32(hText)) )
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

   selectionWait=False;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ReleaseSelection
 *
 * Wine might have lost XA_PRIMARY selection because of
 * EmptyClipboard() or other client. 
 */
void X11DRV_CLIPBOARD_ReleaseSelection(Window w, HWND32 hwnd)
{
    /* w is the window that lost selection,
     * 
     * selectionPrevWindow is nonzero if CheckSelection() was called. 
     */

    TRACE(clipboard,"\tevent->window = %08x (sw = %08x, spw=%08x)\n", 
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
 *		X11DRV_CLIPBOARD_EmptyClipboard
 */
void X11DRV_CLIPBOARD_EmptyClipboard()
{
  if(selectionAcquired)
    {
      selectionAcquired	= False;
      selectionPrevWindow 	= selectionWindow;
      selectionWindow 	= None;
      
      TRACE(clipboard, "\tgiving up selection (spw = %08x)\n", 
	    (unsigned)selectionPrevWindow);
      
      TSXSetSelectionOwner(display, XA_PRIMARY, None, CurrentTime);
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_SetClipboardData
 */
void X11DRV_CLIPBOARD_SetClipboardData(UINT32 wFormat)
{
    Window       owner;

    /* Acquire X selection if text format */

    if( !selectionAcquired && 
	(wFormat == CF_TEXT || wFormat == CF_OEMTEXT) )
    {
	owner = X11DRV_WND_FindXWindow( 
	    WIN_FindWndPtr( hWndClipWindow ? hWndClipWindow : AnyPopup32() ) 
	);

	TSXSetSelectionOwner(display,XA_PRIMARY, owner, CurrentTime);
	if( TSXGetSelectionOwner(display,XA_PRIMARY) == owner )
	{
	    selectionAcquired = True;
	    selectionWindow = owner;

	    TRACE(clipboard,"Grabbed X selection, owner=(%08x)\n", 
						(unsigned) owner);
	}
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_RequestSelection
 */
BOOL32 X11DRV_CLIPBOARD_RequestSelection()
{
    HWND32 hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow32();

    if( selectionAcquired )
      return TRUE;

    if( !hWnd ) return FALSE;

    TRACE(clipboard,"Requesting selection...\n");

  /* request data type XA_STRING, later
   * CLIPBOARD_ReadSelection() will be invoked 
   * from the SelectionNotify event handler */

    TSXConvertSelection(display, XA_PRIMARY, XA_STRING,
			TSXInternAtom(display, "PRIMARY_TEXT", False),
			X11DRV_WND_FindXWindow( WIN_FindWndPtr( hWnd ) ),
			CurrentTime);

  /* wait until SelectionNotify is processed 
   *
   * FIXME: Use TSXCheckTypedWindowEvent() instead ( same in the 
   *	    CLIPBOARD_CheckSelection() ). 
   */

    selectionWait=True;
    while(selectionWait) EVENT_WaitNetEvent( TRUE, FALSE );

  /* we treat Unix text as CF_OEMTEXT */
    TRACE(clipboard,"\tgot CF_OEMTEXT = %i\n", 
		      ClipFormats[CF_OEMTEXT-1].wDataPresent);

    return (BOOL32)ClipFormats[CF_OEMTEXT-1].wDataPresent;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ResetOwner
 *
 * Called from DestroyWindow().
 */
void X11DRV_CLIPBOARD_ResetOwner(WND *pWnd, BOOL32 bFooBar)
{
    LPCLIPFORMAT lpFormat = ClipFormats;

    if(bFooBar && X11DRV_WND_GetXWindow(pWnd))
      return;

    if(!bFooBar && !X11DRV_WND_GetXWindow(pWnd))
      return;

    TRACE(clipboard,"clipboard owner = %04x, selection = %08x\n", 
				hWndClipOwner, (unsigned)selectionWindow);

    if( pWnd->hwndSelf == hWndClipOwner)
    {
	SendMessage16(hWndClipOwner,WM_RENDERALLFORMATS,0,0L);

	/* check if all formats were rendered */

	while(lpFormat)
	{
	    if( lpFormat->wDataPresent && !lpFormat->hData16 && !lpFormat->hData32 )
	    {
		TRACE(clipboard,"\tdata missing for clipboard format %i\n", 
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
