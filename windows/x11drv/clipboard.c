/*
 * X11 clipboard windows driver
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
 *
 * NOTES:
 *    This file contains the X specific implementation for the windows
 *    Clipboard API.
 *
 *    Wine's internal clipboard is exposed to external apps via the X
 *    selection mechanism.
 *    Currently the driver asserts ownership via two selection atoms:
 *    1. PRIMARY(XA_PRIMARY)
 *    2. CLIPBOARD
 *
 *    In our implementation, the CLIPBOARD selection takes precedence over PRIMARY.
 *    i.e. if a CLIPBOARD selection is available, it is used instead of PRIMARY.
 *    When Wine taks ownership of the clipboard, it takes ownership of BOTH selections.
 *    While giving up selection ownership, if the CLIPBOARD selection is lost,
 *    it will lose both PRIMARY and CLIPBOARD and empty the clipboard.
 *    However if only PRIMARY is lost, it will continue to hold the CLIPBOARD selection
 *    (leaving the clipboard cache content unaffected).
 *
 *      Every format exposed via a windows clipboard format is also exposed through
 *    a corresponding X selection target. A selection target atom is synthesized
 *    whenever a new Windows clipboard format is registered via RegisterClipboardFormat,
 *    or when a built in format is used for the first time.
 *    Windows native format are exposed by prefixing the format name with "<WCF>"
 *    This allows us to uniquely identify windows native formats exposed by other
 *    running WINE apps.
 *
 *      In order to allow external applications to query WINE for supported formats,
 *    we respond to the "TARGETS" selection target. (See EVENT_SelectionRequest
 *    for implementation) We use the same mechanism to query external clients for
 *    availability of a particular format, by cacheing the list of available targets
 *    by using the clipboard cache's "delayed render" mechanism. If a selection client
 *    does not support the "TARGETS" selection target, we actually attempt to retrieve
 *    the format requested as a fallback mechanism.
 *
 *      Certain Windows native formats are automatically converted to X native formats
 *    and vice versa. If a native format is available in the selection, it takes
 *    precedence, in order to avoid unnecessary conversions.
 *
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <errno.h>
#include <X11/Xatom.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "ts_xlib.h"

#include "wine/winuser16.h"
#include "clipboard.h"
#include "message.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"
#include "bitmap.h"
#include "commctrl.h"
#include "heap.h"
#include "options.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(clipboard)

/* Selection masks */

#define S_NOSELECTION    0
#define S_PRIMARY        1
#define S_CLIPBOARD      2

/* X selection context info */

static char _CLIPBOARD[] = "CLIPBOARD";        /* CLIPBOARD atom name */
static char FMT_PREFIX[] = "<WCF>";            /* Prefix for windows specific formats */
static int selectionAcquired = 0;              /* Contains the current selection masks */
static Window selectionWindow = None;          /* The top level X window which owns the selection */
static Window selectionPrevWindow = None;      /* The last X window that owned the selection */
static Window PrimarySelectionOwner = None;    /* The window which owns the primary selection */
static Window ClipboardSelectionOwner = None;  /* The window which owns the clipboard selection */
static unsigned long cSelectionTargets = 0;    /* Number of target formats reported by TARGETS selection */
static Atom selectionCacheSrc = XA_PRIMARY;    /* The selection source from which the clipboard cache was filled */
static HANDLE selectionClearEvent = 0;/* Synchronization object used to block until server is started */

/*
 * Dynamic pointer arrays to manage destruction of Pixmap resources
 */
static HDPA PropDPA = NULL;
static HDPA PixmapDPA = NULL;



/**************************************************************************
 *		X11DRV_CLIPBOARD_MapPropertyToFormat
 *
 *  Map an X selection property type atom name to a windows clipboard format ID
 */
UINT X11DRV_CLIPBOARD_MapPropertyToFormat(char *itemFmtName)
{
    /*
     * If the property name starts with FMT_PREFIX strip this off and
     * get the ID for a custom Windows registered format with this name.
     * We can also understand STRING, PIXMAP and BITMAP.
     */
    if ( NULL == itemFmtName )
        return 0;
    else if ( 0 == strncmp(itemFmtName, FMT_PREFIX, strlen(FMT_PREFIX)) )
        return RegisterClipboardFormatA(itemFmtName + strlen(FMT_PREFIX));
    else if ( 0 == strcmp(itemFmtName, "STRING") )
        return CF_OEMTEXT;
    else if ( 0 == strcmp(itemFmtName, "PIXMAP")
                ||  0 == strcmp(itemFmtName, "BITMAP") )
    {
        /*
         * Return CF_DIB as first preference, if WINE is the selection owner
         * and if CF_DIB exists in the cache.
         * If wine dowsn't own the selection we always return CF_DIB
         */
        if ( !X11DRV_CLIPBOARD_IsSelectionowner() )
            return CF_DIB;
        else if ( CLIPBOARD_IsPresent(CF_DIB) )
            return CF_DIB;
        else
            return CF_BITMAP;
    }

    WARN("\tNo mapping to Windows clipboard format for property %s\n", itemFmtName);
    return 0;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_MapFormatToProperty
 *
 *  Map a windows clipboard format ID to an X selection property atom
 */
Atom X11DRV_CLIPBOARD_MapFormatToProperty(UINT wFormat)
{
    Atom prop = None;
    
    switch (wFormat)
    {
        case CF_OEMTEXT:
        case CF_TEXT:
            prop = XA_STRING;
            break;

        case CF_DIB:
        case CF_BITMAP:
        {
            /*
             * Request a PIXMAP, only if WINE is NOT the selection owner,
             * AND the requested format is not in the cache.
             */
            if ( !X11DRV_CLIPBOARD_IsSelectionowner() && !CLIPBOARD_IsPresent(wFormat) )
            {
              prop = XA_PIXMAP;
              break;
            }
            /* Fall thru to the default case in order to use the native format */
        }
        
        default:
        {
            /*
             * If an X atom is registered for this format, return that
             * Otherwise register a new atom.
             */
            char str[256];
            char *fmtName = CLIPBOARD_GetFormatName(wFormat);
            strcpy(str, FMT_PREFIX);

            if (fmtName)
            {
                strncat(str, fmtName, sizeof(str) - strlen(FMT_PREFIX));
                prop = TSXInternAtom(display, str, False);
            }
            break;
        }
    }

    if (prop == None)
        TRACE("\tNo mapping to X property for Windows clipboard format %d(%s)\n",
              wFormat, CLIPBOARD_GetFormatName(wFormat));
    
    return prop;
}

/**************************************************************************
 *	        X11DRV_CLIPBOARD_IsNativeProperty
 *
 *  Checks if a property is a native property type
 */
BOOL X11DRV_CLIPBOARD_IsNativeProperty(Atom prop)
{
    char *itemFmtName = TSXGetAtomName(display, prop);
    BOOL bRet = FALSE;
    
    if ( 0 == strncmp(itemFmtName, FMT_PREFIX, strlen(FMT_PREFIX)) )
        bRet = TRUE;
    
    TSXFree(itemFmtName);
    return bRet;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_LaunchServer
 * Launches the clipboard server. This is called from X11DRV_CLIPBOARD_ResetOwner
 * when the selection can no longer be recyled to another top level window.
 * In order to make the selection persist after Wine shuts down a server
 * process is launched which services subsequent selection requests.
 */
BOOL X11DRV_CLIPBOARD_LaunchServer()
{
    int iWndsLocks;

    /* If persistant selection has been disabled in the .winerc Clipboard section,
     * don't launch the server
     */
    if ( !PROFILE_GetWineIniInt("Clipboard", "PersistentSelection", 1) )
        return FALSE;

    /*  Start up persistant WINE X clipboard server process which will
     *  take ownership of the X selection and continue to service selection
     *  requests from other apps.
     */
    selectionWindow = selectionPrevWindow;
    if ( !fork() )
    {
        /* NOTE: This code only executes in the context of the child process
         * Do note make any Wine specific calls here.
         */
        
        int dbgClasses = 0;
        char selMask[8], dbgClassMask[8], clearSelection[8];
	int i;

	/* Don't inherit wine's X sockets to the wineclipsrv, otherwise
	 * windows stay around when you have to kill a hanging wine...
	 */
	for (i = 3; i < 256; ++i)
		fcntl(i, F_SETFD, 1);

        sprintf(selMask, "%d", selectionAcquired);

        /* Build the debug class mask to pass to the server, by inheriting
         * the settings for the clipboard debug channel.
         */
        dbgClasses |= __GET_DEBUGGING(__DBCL_FIXME, dbch_clipboard) ? 1 : 0;
        dbgClasses |= __GET_DEBUGGING(__DBCL_ERR, dbch_clipboard) ? 2 : 0;
        dbgClasses |= __GET_DEBUGGING(__DBCL_WARN, dbch_clipboard) ? 4 : 0;
        dbgClasses |= __GET_DEBUGGING(__DBCL_TRACE, dbch_clipboard) ? 8 : 0;
        sprintf(dbgClassMask, "%d", dbgClasses);

        /* Get the clear selection preference */
        sprintf(clearSelection, "%d",
                PROFILE_GetWineIniInt("Clipboard", "ClearAllSelections", 0));

        /* Exec the clipboard server passing it the selection and debug class masks */
        execl( BINDIR "/wineclipsrv", "wineclipsrv",
               selMask, dbgClassMask, clearSelection, NULL );
        execlp( "wineclipsrv", "wineclipsrv", selMask, dbgClassMask, clearSelection, NULL );
        execl( "./windows/x11drv/wineclipsrv", "wineclipsrv",
               selMask, dbgClassMask, clearSelection, NULL );

        /* Exec Failed! */
        perror("Could not start Wine clipboard server");
        exit( 1 ); /* Exit the child process */
    }

    /* Wait until the clipboard server acquires the selection.
     * We must release the windows lock to enable Wine to process
     * selection messages in response to the servers requests.
     */
    
    iWndsLocks = WIN_SuspendWndsLock();

    /* We must wait until the server finishes acquiring the selection,
     * before proceeding, otherwise the window which owns the selection
     * will be destroyed prematurely!
     * Create a non-signalled, auto-reset event which will be set by
     * X11DRV_CLIPBOARD_ReleaseSelection, and wait until this gets
     * signalled before proceeding.
     */

    if ( !(selectionClearEvent = CreateEventA(NULL, FALSE, FALSE, NULL)) )
        ERR("Could not create wait object. Clipboard server won't start!\n");
    else
    {
        /* Make the event object's handle global */
        selectionClearEvent = ConvertToGlobalHandle(selectionClearEvent);
        
        /* Wait until we lose the selection, timing out after a minute */

        TRACE("Waiting for clipboard server to acquire selection\n");

        if ( WaitForSingleObject( selectionClearEvent, 60000 ) != WAIT_OBJECT_0 )
            TRACE("Server could not acquire selection, or a time out occured!\n");
        else
            TRACE("Server successfully acquired selection\n");

        /* Release the event */
        CloseHandle(selectionClearEvent);
        selectionClearEvent = 0;
    }

    WIN_RestoreWndsLock(iWndsLocks);
    
    return TRUE;
}


/**************************************************************************
 *		X11DRV_CLIPBOARD_CacheDataFormats
 *
 * Caches the list of data formats available from the current selection.
 * This queries the selection owner for the TARGETS property and saves all
 * reported property types.
 */
int X11DRV_CLIPBOARD_CacheDataFormats( Atom SelectionName )
{
    HWND           hWnd = 0;
    HWND           hWndClipWindow = GetOpenClipboardWindow();
    WND*           wnd = NULL;
    XEvent         xe;
    Atom           aTargets;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetList=NULL;
    Window         w;
    Window         ownerSelection = 0;
        
    /*
     * Empty the clipboard cache 
     */
    CLIPBOARD_EmptyCache(TRUE);

    cSelectionTargets = 0;
    selectionCacheSrc = SelectionName;
    
    hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow();

    ownerSelection = TSXGetSelectionOwner(display, SelectionName);
    if ( !hWnd || (ownerSelection == None) )
        return cSelectionTargets;

    /*
     * Query the selection owner for the TARGETS property
     */
    wnd = WIN_FindWndPtr(hWnd);
    w = X11DRV_WND_FindXWindow(wnd);
    WIN_ReleaseWndPtr(wnd);
    wnd = NULL;

    aTargets = TSXInternAtom(display, "TARGETS", False);

    TRACE("Requesting TARGETS selection for '%s' (owner=%08x)...\n",
          TSXGetAtomName(display, selectionCacheSrc), (unsigned)ownerSelection );
          
    EnterCriticalSection( &X11DRV_CritSection );
    XConvertSelection(display, selectionCacheSrc, aTargets,
                    TSXInternAtom(display, "SELECTION_DATA", False),
                    w, CurrentTime);

    /*
     * Wait until SelectionNotify is received
     */
    while( TRUE )
    {
       if( XCheckTypedWindowEvent(display, w, SelectionNotify, &xe) )
           if( xe.xselection.selection == selectionCacheSrc )
               break;
    }
    LeaveCriticalSection( &X11DRV_CritSection );

    /* Verify that the selection returned a valid TARGETS property */
    if ( (xe.xselection.target != aTargets)
          || (xe.xselection.property == None) )
    {
        TRACE("\tCould not retrieve TARGETS\n");
        return cSelectionTargets;
    }

    /* Read the TARGETS property contents */
    if(TSXGetWindowProperty(display, xe.xselection.requestor, xe.xselection.property,
                            0, 0x3FFF, True, AnyPropertyType/*XA_ATOM*/, &atype, &aformat,
                            &cSelectionTargets, &remain, (unsigned char**)&targetList) != Success)
        TRACE("\tCouldn't read TARGETS property\n");
    else
    {
       TRACE("\tType %s,Format %d,nItems %ld, Remain %ld\n",
             TSXGetAtomName(display,atype),aformat,cSelectionTargets, remain);
       /*
        * The TARGETS property should have returned us a list of atoms
        * corresponding to each selection target format supported.
        */
       if( (atype == XA_ATOM || atype == aTargets) && aformat == 32 )
       {
          int i;
          LPWINE_CLIPFORMAT lpFormat;
          
          /* Cache these formats in the clipboard cache */

          for (i = 0; i < cSelectionTargets; i++)
          {
              char *itemFmtName = TSXGetAtomName(display, targetList[i]);
              UINT wFormat = X11DRV_CLIPBOARD_MapPropertyToFormat(itemFmtName);

              /*
               * If the clipboard format maps to a Windows format, simply store
               * the atom identifier and record its availablity status
               * in the clipboard cache.
               */
              if (wFormat)
              {
                  lpFormat = CLIPBOARD_LookupFormat( wFormat );
                  
                  /* Don't replace if the property already cached is a native format,
                   * or if a PIXMAP is being replaced by a BITMAP.
                   */
                  if (lpFormat->wDataPresent &&
                        ( X11DRV_CLIPBOARD_IsNativeProperty(lpFormat->drvData)
                          || (lpFormat->drvData == XA_PIXMAP && targetList[i] == XA_BITMAP) )
                     )
                  {
                      TRACE("\tAtom# %d: '%s' --> FormatID(%d) %s (Skipped)\n",
                            i, itemFmtName, wFormat, lpFormat->Name);
                  }
                  else
                  {
                      lpFormat->wDataPresent = 1;
                      lpFormat->drvData = targetList[i];
                      TRACE("\tAtom# %d: '%s' --> FormatID(%d) %s\n",
                            i, itemFmtName, wFormat, lpFormat->Name);
                  }
              }
              
              TSXFree(itemFmtName);
          }
       }

       /* Free the list of targets */
       TSXFree(targetList);
    }
    
    return cSelectionTargets;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ReadSelection
 *  Reads the contents of the X selection property into the WINE clipboard cache
 *  converting the selection into a format compatible with the windows clipboard
 *  if possible.
 *  This method is invoked only to read the contents of a the selection owned
 *  by an external application. i.e. when we do not own the X selection.
 */
static BOOL X11DRV_CLIPBOARD_ReadSelection(UINT wFormat, Window w, Atom prop, Atom reqType)
{
    Atom	      atype=AnyPropertyType;
    int		      aformat;
    unsigned long     nitems,remain,itemSize;
    long              lRequestLength;
    unsigned char*    val=NULL;
    LPWINE_CLIPFORMAT lpFormat;
    BOOL              bRet = FALSE;
    HWND              hWndClipWindow = GetOpenClipboardWindow();

    
    if(prop == None)
        return bRet;

    TRACE("Reading X selection...\n");

    TRACE("\tretrieving property %s from window %ld into %s\n",
          TSXGetAtomName(display,reqType), (long)w, TSXGetAtomName(display,prop) );

    /*
     * First request a zero length in order to figure out the request size.
     */
    if(TSXGetWindowProperty(display,w,prop,0,0,False, AnyPropertyType/*reqType*/,
                            &atype, &aformat, &nitems, &itemSize, &val) != Success)
    {
        WARN("\tcouldn't get property size\n");
        return bRet;
    }

    /* Free zero length return data if any */
    if ( val )
    {
       TSXFree(val);
       val = NULL;
    }
    
    TRACE("\tretrieving %ld bytes...\n", itemSize * aformat/8);
    lRequestLength = (itemSize * aformat/8)/4  + 1;
    
    /*
     * Retrieve the actual property in the required X format.
     */
    if(TSXGetWindowProperty(display,w,prop,0,lRequestLength,False,AnyPropertyType/*reqType*/,
                            &atype, &aformat, &nitems, &remain, &val) != Success)
    {
        WARN("\tcouldn't read property\n");
        return bRet;
    }

    TRACE("\tType %s,Format %d,nitems %ld,remain %ld,value %s\n",
          atype ? TSXGetAtomName(display,atype) : NULL, aformat,nitems,remain,val);
    
    if (remain)
    {
        WARN("\tCouldn't read entire property- selection may be too large! Remain=%ld\n", remain);
        goto END;
    }
    
    /*
     * Translate the X property into the appropriate Windows clipboard
     * format, if possible.
     */
    if ( (reqType == XA_STRING)
         && (atype == XA_STRING) && (aformat == 8) ) /* treat Unix text as CF_OEMTEXT */
    {
      HANDLE16   hText = 0;
      int 	   i,inlcount = 0;
      char*      lpstr;
 
      TRACE("\tselection is '%s'\n",val);
 
      for(i=0; i <= nitems; i++)
          if( val[i] == '\n' ) inlcount++;
 
      if( nitems )
      {
        hText=GlobalAlloc16(GMEM_MOVEABLE, nitems + inlcount + 1);
        if( (lpstr = (char*)GlobalLock16(hText)) )
        {
          for(i=0,inlcount=0; i <= nitems; i++)
          {
             if( val[i] == '\n' ) lpstr[inlcount++]='\r';
             lpstr[inlcount++]=val[i];
          }
          GlobalUnlock16(hText);
        }
        else
            hText = 0;
      }
      
      if( hText )
      {
          /* delete previous CF_TEXT and CF_OEMTEXT data */
          lpFormat = CLIPBOARD_LookupFormat(CF_TEXT);
          if (lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32) 
              CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
          
          lpFormat = CLIPBOARD_LookupFormat(CF_OEMTEXT);
          if (lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32)  
              CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
 
          /* Update the CF_OEMTEXT record */
          lpFormat->wDataPresent = 1;
          lpFormat->hData32 = 0;
          lpFormat->hData16 = hText;
 
          bRet = TRUE;
      }
    }
    else if ( reqType == XA_PIXMAP || reqType == XA_BITMAP ) /* treat PIXMAP as CF_DIB or CF_BITMAP */
    {
      /* Get the first pixmap handle passed to us */
      Pixmap *pPixmap = (Pixmap *)val;
      HANDLE hTargetImage = 0;  /* Handle to store the converted bitmap or DIB */
      
      if (aformat != 32 || nitems < 1 || atype != XA_PIXMAP
          || (wFormat != CF_BITMAP && wFormat != CF_DIB))
      {
          WARN("\tUnimplemented format conversion request\n");
          goto END;
      }
          
      if ( wFormat == CF_BITMAP )
      {
        /* For CF_BITMAP requests we must return an HBITMAP */
        hTargetImage = X11DRV_BITMAP_CreateBitmapFromPixmap(*pPixmap, TRUE);
      }
      else if (wFormat == CF_DIB)
      {
        HWND hwnd = GetOpenClipboardWindow();
        HDC hdc = GetDC(hwnd);
        
        /* For CF_DIB requests we must return an HGLOBAL storing a packed DIB */
        hTargetImage = X11DRV_DIB_CreateDIBFromPixmap(*pPixmap, hdc, TRUE);
        
        ReleaseDC(hdc, hwnd);
      }

      if (!hTargetImage)
      {
          WARN("PIXMAP conversion failed!\n" );
          goto END;
      }
      
      /* Delete previous clipboard data */
      lpFormat = CLIPBOARD_LookupFormat(wFormat);
      if (lpFormat->wDataPresent && (lpFormat->hData16 || lpFormat->hData32))
          CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
      
      /* Update the clipboard record */
      lpFormat->wDataPresent = 1;
      lpFormat->hData32 = hTargetImage;
      lpFormat->hData16 = 0;

      bRet = TRUE;
    }
 
    /* For native properties simply copy the X data without conversion */
    else if (X11DRV_CLIPBOARD_IsNativeProperty(reqType)) /* <WCF>* */
    {
      HANDLE hClipData = 0;
      void*  lpClipData;
      int cBytes = nitems * aformat/8;

      if( cBytes )
      {
        /* Turn on the DDESHARE flag to enable shared 32 bit memory */
        hClipData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cBytes );
        if( (lpClipData = GlobalLock(hClipData)) )
        {
            memcpy(lpClipData, val, cBytes);
            GlobalUnlock(hClipData);
        }
        else
            hClipData = 0;
      }
      
      if( hClipData )
      {
          /* delete previous clipboard record if any */
          lpFormat = CLIPBOARD_LookupFormat(wFormat);
          if (lpFormat->wDataPresent || lpFormat->hData16 || lpFormat->hData32) 
              CLIPBOARD_DeleteRecord(lpFormat, !(hWndClipWindow));
          
          /* Update the clipboard record */
          lpFormat->wDataPresent = 1;
          lpFormat->hData32 = hClipData;
          lpFormat->hData16 = 0;

          bRet = TRUE;
      }
    }
    else
    {
        WARN("\tUnimplemented format conversion request\n");
        goto END;
    }

END:
    /* Delete the property on the window now that we are done
     * This will send a PropertyNotify event to the selection owner. */
    TSXDeleteProperty(display,w,prop);
    
    /* Free the retrieved property data */
    if (val)
       TSXFree(val);
    
    return bRet;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ReleaseSelection
 *
 * Release an XA_PRIMARY or XA_CLIPBOARD selection that we own, in response
 * to a SelectionClear event.
 * This can occur in response to another client grabbing the X selection.
 * If the XA_CLIPBOARD selection is lost, we relinquish XA_PRIMARY as well.
 */
void X11DRV_CLIPBOARD_ReleaseSelection(Atom selType, Window w, HWND hwnd)
{
    Atom xaClipboard = TSXInternAtom(display, "CLIPBOARD", False);
    int clearAllSelections = PROFILE_GetWineIniInt("Clipboard", "ClearAllSelections", 0);
    
    /* w is the window that lost the selection
     * selectionPrevWindow is nonzero if CheckSelection() was called. 
     */

    TRACE("\tevent->window = %08x (sw = %08x, spw=%08x)\n", 
	  (unsigned)w, (unsigned)selectionWindow, (unsigned)selectionPrevWindow );

    if( selectionAcquired )
    {
	if( w == selectionWindow || selectionPrevWindow == None)
	{
            /* If we're losing the CLIPBOARD selection, or if the preferences in .winerc
             * dictate that *all* selections should be cleared on loss of a selection,
             * we must give up all the selections we own.
             */
            if ( clearAllSelections || (selType == xaClipboard) )
            {
              /* completely give up the selection */
              TRACE("Lost CLIPBOARD (+PRIMARY) selection\n");

              /* We are completely giving up the selection.
               * Make sure we can open the windows clipboard first. */
              
              if ( !OpenClipboard(hwnd) )
              {
                  /*
                   * We can't empty the clipboard if we cant open it so abandon.
                   * Wine will think that it still owns the selection but this is
                   * safer than losing the selection without properly emptying
                   * the clipboard. Perhaps we should forcibly re-assert ownership
                   * of the CLIPBOARD selection in this case...
                   */
                  ERR("\tClipboard is busy. Could not give up selection!\n");
                  return;
              }

              /* We really lost CLIPBOARD but want to voluntarily lose PRIMARY */
              if ( (selType == xaClipboard)
                   && (selectionAcquired & S_PRIMARY) )
              {
                  XSetSelectionOwner(display, XA_PRIMARY, None, CurrentTime);
              }
              
              /* We really lost PRIMARY but want to voluntarily lose CLIPBOARD  */
              if ( (selType == XA_PRIMARY)
                   && (selectionAcquired & S_CLIPBOARD) )
              {
                  XSetSelectionOwner(display, xaClipboard, None, CurrentTime);
              }
              
              selectionWindow = None;
              PrimarySelectionOwner = ClipboardSelectionOwner = 0;
              
              /* Empty the windows clipboard.
               * We should pretend that we still own the selection BEFORE calling
               * EmptyClipboard() since otherwise this has the side effect of
               * triggering X11DRV_CLIPBOARD_Acquire() and causing the X selection
               * to be re-acquired by us!
               */
              selectionAcquired = (S_PRIMARY | S_CLIPBOARD);
              EmptyClipboard();
              CloseClipboard();

              /* Give up ownership of the windows clipboard */
              CLIPBOARD_ReleaseOwner();

              /* Reset the selection flags now that we are done */
              selectionAcquired = S_NOSELECTION;
            }
            else if ( selType == XA_PRIMARY ) /* Give up only PRIMARY selection */
            {
                TRACE("Lost PRIMARY selection\n");
                PrimarySelectionOwner = 0;
                selectionAcquired &= ~S_PRIMARY;  /* clear S_PRIMARY mask */
            }

            cSelectionTargets = 0;
	}
        /* but we'll keep existing data for internal use */
	else if( w == selectionPrevWindow )
	{
            Atom xaClipboard = TSXInternAtom(display, _CLIPBOARD, False);
            
	    w = TSXGetSelectionOwner(display, XA_PRIMARY);
	    if( w == None )
		TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);

	    w = TSXGetSelectionOwner(display, xaClipboard);
	    if( w == None )
		TSXSetSelectionOwner(display, xaClipboard, selectionWindow, CurrentTime);
        }
    }

    /* Signal to a selectionClearEvent listener if the selection is completely lost */
    if (selectionClearEvent && !selectionAcquired)
    {
        TRACE("Lost all selections, signalling to selectionClearEvent listener\n");
        SetEvent(selectionClearEvent);
    }
    
    selectionPrevWindow = None;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_Empty
 *  Voluntarily release all currently owned X selections
 */
void X11DRV_CLIPBOARD_Release()
{
    if( selectionAcquired )
    {
	XEvent xe;
        Window savePrevWindow = selectionWindow;
        Atom xaClipboard = TSXInternAtom(display, _CLIPBOARD, False);
        BOOL bHasPrimarySelection = selectionAcquired & S_PRIMARY;

	selectionAcquired   = S_NOSELECTION;
	selectionPrevWindow = selectionWindow;
	selectionWindow     = None;
      
	TRACE("\tgiving up selection (spw = %08x)\n", 
	     (unsigned)selectionPrevWindow);
      
	EnterCriticalSection(&X11DRV_CritSection);

        TRACE("Releasing CLIPBOARD selection\n");
        XSetSelectionOwner(display, xaClipboard, None, CurrentTime);
	if( selectionPrevWindow )
	    while( !XCheckTypedWindowEvent( display, selectionPrevWindow,
                                            SelectionClear, &xe ) );

        if ( bHasPrimarySelection )
        {
            TRACE("Releasing XA_PRIMARY selection\n");
            selectionPrevWindow = savePrevWindow; /* May be cleared in X11DRV_CLIPBOARD_ReleaseSelection */
            XSetSelectionOwner(display, XA_PRIMARY, None, CurrentTime);
    
            if( selectionPrevWindow )
                while( !XCheckTypedWindowEvent( display, selectionPrevWindow,
                                                SelectionClear, &xe ) );
        }
        
	LeaveCriticalSection(&X11DRV_CritSection);
    }

    /* Get rid of any Pixmap resources we may still have */
    if (PropDPA)
        DPA_Destroy( PropDPA );
    if (PixmapDPA)
    {
      int i;
      Pixmap pixmap;
      for( i = 0; ; i++ )
      {
        if ( (pixmap = ((Pixmap)DPA_GetPtr(PixmapDPA, i))) )
          XFreePixmap(display, pixmap);
        else
          break;
      }
      DPA_Destroy( PixmapDPA );
    }
    PixmapDPA = PropDPA = NULL;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_Acquire()
 */
void X11DRV_CLIPBOARD_Acquire()
{
    Window       owner;
    HWND         hWndClipWindow = GetOpenClipboardWindow();

    /*
     * Acquire X selection if we don't already own it.
     * Note that we only acquire the selection if it hasn't been already
     * acquired by us, and ignore the fact that another X window may be
     * asserting ownership. The reason for this is we need *any* top level
     * X window to hold selection ownership. The actual clipboard data requests
     * are made via GetClipboardData from EVENT_SelectionRequest and this
     * ensures that the real HWND owner services the request.
     * If the owning X window gets destroyed the selection ownership is
     * re-cycled to another top level X window in X11DRV_CLIPBOARD_ResetOwner.
     *
     */

    if ( !(selectionAcquired == (S_PRIMARY | S_CLIPBOARD)) )
    {
        Atom xaClipboard = TSXInternAtom(display, _CLIPBOARD, False);
        WND *tmpWnd = WIN_FindWndPtr( hWndClipWindow ? hWndClipWindow : AnyPopup() );
        owner = X11DRV_WND_FindXWindow(tmpWnd );
        WIN_ReleaseWndPtr(tmpWnd);

        /* Grab PRIMARY selection if not owned */
        if ( !(selectionAcquired & S_PRIMARY) )
            TSXSetSelectionOwner(display, XA_PRIMARY, owner, CurrentTime);
        
        /* Grab CLIPBOARD selection if not owned */
        if ( !(selectionAcquired & S_CLIPBOARD) )
            TSXSetSelectionOwner(display, xaClipboard, owner, CurrentTime);

        if( TSXGetSelectionOwner(display,XA_PRIMARY) == owner )
	    selectionAcquired |= S_PRIMARY;

        if( TSXGetSelectionOwner(display,xaClipboard) == owner)
	    selectionAcquired |= S_CLIPBOARD;

        if (selectionAcquired)
        {
            /* Create dynamic pointer arrays to manage Pixmap resources we may expose */
            if (!PropDPA)
                PropDPA = DPA_CreateEx( 2, SystemHeap );
            if (!PixmapDPA)
                PixmapDPA = DPA_CreateEx( 2, SystemHeap );
            
	    selectionWindow = owner;
	    TRACE("Grabbed X selection, owner=(%08x)\n", (unsigned) owner);
        }
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_IsFormatAvailable
 *
 * Checks if the specified format is available in the current selection
 * Only invoked when WINE is not the selection owner
 */
BOOL X11DRV_CLIPBOARD_IsFormatAvailable(UINT wFormat)
{
    Atom xaClipboard = TSXInternAtom(display, _CLIPBOARD, False);
    Window ownerPrimary = TSXGetSelectionOwner(display,XA_PRIMARY);
    Window ownerClipboard = TSXGetSelectionOwner(display,xaClipboard);

    /*
     * If the selection has not been previously cached, or the selection has changed,
     * try and cache the list of available selection targets from the current selection.
     */
    if ( !cSelectionTargets || (PrimarySelectionOwner != ownerPrimary)
                         || (ClipboardSelectionOwner != ownerClipboard) )
    {
        /*
         * First try cacheing the CLIPBOARD selection.
         * If unavailable try PRIMARY.
         */
        if ( X11DRV_CLIPBOARD_CacheDataFormats(xaClipboard) == 0 )
        {
            X11DRV_CLIPBOARD_CacheDataFormats(XA_PRIMARY);
        }

        ClipboardSelectionOwner = ownerClipboard;
        PrimarySelectionOwner = ownerPrimary;
    }

    /* Exit if there is no selection */
    if ( !ownerClipboard && !ownerPrimary )
        return FALSE;

    if ( wFormat == CF_TEXT )
        wFormat = CF_OEMTEXT;
    
    /* Check if the format is available in the clipboard cache */
    if ( CLIPBOARD_IsPresent(wFormat) )
        return TRUE;

    /*
     * Many X client apps (such as XTerminal) don't support being queried
     * for the "TARGETS" target atom. To handle such clients we must actually
     * try to convert the selection to the requested type.
     */
    if ( !cSelectionTargets )
        return X11DRV_CLIPBOARD_GetData( wFormat );
        
    return FALSE;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_RegisterFormat
 *
 * Registers a custom X clipboard format
 * Returns: TRUE - success,  FALSE - failure
 */
BOOL X11DRV_CLIPBOARD_RegisterFormat( LPCSTR FormatName )
{
    Atom prop = None;
    char str[256];
    
    /*
     * If an X atom is registered for this format, return that
     * Otherwise register a new atom.
     */
    if (FormatName)
    {
        /* Add a WINE specific prefix to the format */
        strcpy(str, FMT_PREFIX);
        strncat(str, FormatName, sizeof(str) - strlen(FMT_PREFIX));
        prop = TSXInternAtom(display, str, False);
    }
    
    return (prop) ? TRUE : FALSE;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_IsSelectionowner
 *
 * Returns: TRUE - We(WINE) own the selection, FALSE - Selection not owned by us
 */
BOOL X11DRV_CLIPBOARD_IsSelectionowner()
{
    return selectionAcquired;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_SetData
 *
 * We don't need to do anything special here since the clipboard code
 * maintains the cache. 
 *
 */
void X11DRV_CLIPBOARD_SetData(UINT wFormat)
{
    /* Make sure we have acquired the X selection */
    X11DRV_CLIPBOARD_Acquire();
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_GetData
 *
 * This method is invoked only when we DO NOT own the X selection
 *
 * NOTE: Clipboard driver doesn't get requests for CF_TEXT data, only
 * for CF_OEMTEXT.
 * We always get the data from the selection client each time,
 * since we have no way of determining if the data in our cache is stale.
 */
BOOL X11DRV_CLIPBOARD_GetData(UINT wFormat)
{
    BOOL bRet = selectionAcquired;
    HWND hWndClipWindow = GetOpenClipboardWindow();
    HWND hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow();
    WND* wnd = NULL;
    LPWINE_CLIPFORMAT lpFormat;

    if( !selectionAcquired && (wnd = WIN_FindWndPtr(hWnd)) )
    {
	XEvent xe;
        Atom propRequest;
	Window w = X11DRV_WND_FindXWindow(wnd);
        WIN_ReleaseWndPtr(wnd);
        wnd = NULL;

        /* Map the format ID requested to an X selection property.
         * If the format is in the cache, use the atom associated
         * with it.
         */
        
        lpFormat = CLIPBOARD_LookupFormat( wFormat );
        if (lpFormat && lpFormat->wDataPresent && lpFormat->drvData)
            propRequest = (Atom)lpFormat->drvData;
        else
            propRequest = X11DRV_CLIPBOARD_MapFormatToProperty(wFormat);

        if (propRequest)
        {
            TRACE("Requesting %s selection from %s...\n",
                  TSXGetAtomName(display, propRequest),
                  TSXGetAtomName(display, selectionCacheSrc) );
    
            EnterCriticalSection( &X11DRV_CritSection );
            XConvertSelection(display, selectionCacheSrc, propRequest,
                            TSXInternAtom(display, "SELECTION_DATA", False),
                            w, CurrentTime);
        
            /* wait until SelectionNotify is received */
    
            while( TRUE )
            {
               if( XCheckTypedWindowEvent(display, w, SelectionNotify, &xe) )
                   if( xe.xselection.selection == selectionCacheSrc )
                       break;
            }
            LeaveCriticalSection( &X11DRV_CritSection );
        
            /*
             *  Read the contents of the X selection property into WINE's
             *  clipboard cache converting the selection to be compatible if possible.
             */
            bRet = X11DRV_CLIPBOARD_ReadSelection( wFormat,
                                                   xe.xselection.requestor,
                                                   xe.xselection.property,
                                                   xe.xselection.target);
        }
        else
            bRet = FALSE;

        TRACE("\tpresent %s = %i\n", CLIPBOARD_GetFormatName(wFormat), bRet );
    }
    
    return bRet;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_ResetOwner
 *
 * Called from DestroyWindow() to prevent X selection from being lost when
 * a top level window is destroyed, by switching ownership to another top
 * level window.
 * Any top level window can own the selection. See X11DRV_CLIPBOARD_Acquire
 * for a more detailed description of this.
 */
void X11DRV_CLIPBOARD_ResetOwner(WND *pWnd, BOOL bFooBar)
{
    HWND hWndClipOwner = 0;
    Window XWnd = X11DRV_WND_GetXWindow(pWnd);
    Atom xaClipboard;
    BOOL bLostSelection = FALSE;

    /* There is nothing to do if we don't own the selection,
     * or if the X window which currently owns the selecion is different
     * from the one passed in.
     */
    if ( !selectionAcquired || XWnd != selectionWindow
         || selectionWindow == None )
       return;

    if ( (bFooBar && XWnd) || (!bFooBar && !XWnd) )
       return;

    hWndClipOwner = GetClipboardOwner();
    xaClipboard = TSXInternAtom(display, _CLIPBOARD, False);
    
    TRACE("clipboard owner = %04x, selection window = %08x\n",
          hWndClipOwner, (unsigned)selectionWindow);

    /* now try to salvage current selection from being destroyed by X */

    TRACE("\tchecking %08x\n", (unsigned) XWnd);

    selectionPrevWindow = selectionWindow;
    selectionWindow = None;

    if( pWnd->next ) 
        selectionWindow = X11DRV_WND_GetXWindow(pWnd->next);
    else if( pWnd->parent )
         if( pWnd->parent->child != pWnd ) 
             selectionWindow = X11DRV_WND_GetXWindow(pWnd->parent->child);

    if( selectionWindow != None )
    {
        /* We must pretend that we don't own the selection while making the switch
         * since a SelectionClear event will be sent to the last owner.
         * If there is no owner X11DRV_CLIPBOARD_ReleaseSelection will do nothing.
         */
        int saveSelectionState = selectionAcquired;
        selectionAcquired = False;

        TRACE("\tswitching selection from %08x to %08x\n", 
                    (unsigned)selectionPrevWindow, (unsigned)selectionWindow);
    
        /* Assume ownership for the PRIMARY and CLIPBOARD selection */
        if ( saveSelectionState & S_PRIMARY )
            TSXSetSelectionOwner(display, XA_PRIMARY, selectionWindow, CurrentTime);
        
        TSXSetSelectionOwner(display, xaClipboard, selectionWindow, CurrentTime);

        /* Restore the selection masks */
        selectionAcquired = saveSelectionState;

        /* Lose the selection if something went wrong */
        if ( ( (saveSelectionState & S_PRIMARY) &&
               (TSXGetSelectionOwner(display, XA_PRIMARY) != selectionWindow) )
             || (TSXGetSelectionOwner(display, xaClipboard) != selectionWindow) )
        {
            bLostSelection = TRUE;
            goto END;
        }
        else
        {
            /* Update selection state */
            if (saveSelectionState & S_PRIMARY)
               PrimarySelectionOwner = selectionWindow;
            
            ClipboardSelectionOwner = selectionWindow;
        }
    }
    else
    {
        bLostSelection = TRUE;
        goto END;
    }

END:
    if (bLostSelection)
    {
      /* Launch the clipboard server if the selection can no longer be recyled
       * to another top level window. */
  
      if ( !X11DRV_CLIPBOARD_LaunchServer() )
      {
         /* Empty the windows clipboard if the server was not launched.
          * We should pretend that we still own the selection BEFORE calling
          * EmptyClipboard() since otherwise this has the side effect of
          * triggering X11DRV_CLIPBOARD_Acquire() and causing the X selection
          * to be re-acquired by us!
          */
  
         TRACE("\tLost the selection! Emptying the clipboard...\n");
      
         OpenClipboard( 0 );
         selectionAcquired = (S_PRIMARY | S_CLIPBOARD);
         EmptyClipboard();
         
         CloseClipboard();
   
         /* Give up ownership of the windows clipboard */
         CLIPBOARD_ReleaseOwner();
      }

      selectionAcquired = S_NOSELECTION;
      ClipboardSelectionOwner = PrimarySelectionOwner = 0;
      selectionWindow = 0;
    }
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_RegisterPixmapResource
 * Registers a Pixmap resource which is to be associated with a property Atom.
 * When the property is destroyed we also destroy the Pixmap through the
 * PropertyNotify event.
 */
BOOL X11DRV_CLIPBOARD_RegisterPixmapResource( Atom property, Pixmap pixmap )
{
  if ( -1 == DPA_InsertPtr( PropDPA, 0, (void*)property ) )
    return FALSE;
    
  if ( -1 == DPA_InsertPtr( PixmapDPA, 0, (void*)pixmap ) )
    return FALSE;

  return TRUE;
}

/**************************************************************************
 *		X11DRV_CLIPBOARD_FreeResources
 *
 * Called from EVENT_PropertyNotify() to give us a chance to destroy
 * any resources associated with this property.
 */
void X11DRV_CLIPBOARD_FreeResources( Atom property )
{
    /* Do a simple linear search to see if we have a Pixmap resource
     * associated with this property and release it.
     */
    int i;
    Pixmap pixmap;
    Atom cacheProp = 0;
    for( i = 0; ; i++ )
    {
        if ( !(cacheProp = ((Atom)DPA_GetPtr(PropDPA, i))) )
            break;
        
        if ( cacheProp == property )
        {
            /* Lookup the associated Pixmap and free it */
            pixmap = (Pixmap)DPA_GetPtr(PixmapDPA, i);
  
            TRACE("Releasing pixmap %ld for Property %s\n",
                  (long)pixmap, TSXGetAtomName(display, cacheProp));
            
            XFreePixmap(display, pixmap);

            /* Free the entries from the table */
            DPA_DeletePtr(PropDPA, i);
            DPA_DeletePtr(PixmapDPA, i);
        }
    }
}

#endif /* !defined(X_DISPLAY_MISSING) */
