/*
 * X11 clipboard windows driver
 *
 * Copyright 1994 Martin Ayotte
 *	     1996 Alex Korobka
 *	     1999 Noel Borthwick
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
 *    In our implementation, the CLIPBOARD selection takes precedence over PRIMARY,
 *    i.e. if a CLIPBOARD selection is available, it is used instead of PRIMARY.
 *    When Wine takes ownership of the clipboard, it takes ownership of BOTH selections.
 *    While giving up selection ownership, if the CLIPBOARD selection is lost,
 *    it will lose both PRIMARY and CLIPBOARD and empty the clipboard.
 *    However if only PRIMARY is lost, it will continue to hold the CLIPBOARD selection
 *    (leaving the clipboard cache content unaffected).
 *
 *      Every format exposed via a windows clipboard format is also exposed through
 *    a corresponding X selection target. A selection target atom is synthesized
 *    whenever a new Windows clipboard format is registered via RegisterClipboardFormat,
 *    or when a built-in format is used for the first time.
 *    Windows native format are exposed by prefixing the format name with "<WCF>"
 *    This allows us to uniquely identify windows native formats exposed by other
 *    running WINE apps.
 *
 *      In order to allow external applications to query WINE for supported formats,
 *    we respond to the "TARGETS" selection target. (See EVENT_SelectionRequest
 *    for implementation) We use the same mechanism to query external clients for
 *    availability of a particular format, by caching the list of available targets
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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fcntl.h>
#include <time.h>

#include "ts_xlib.h"
#include "winreg.h"
#include "clipboard.h"
#include "win.h"
#include "x11drv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(clipboard);

/* Maximum wait time for slection notify */
#define MAXSELECTIONNOTIFYWAIT 5

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

typedef struct tagPROPERTY
{
    struct tagPROPERTY *next;
    Atom                atom;
    Pixmap              pixmap;
} PROPERTY;

static PROPERTY *prop_head;


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
        return CF_UNICODETEXT;
    else if ( 0 == strcmp(itemFmtName, "PIXMAP")
                ||  0 == strcmp(itemFmtName, "BITMAP") )
    {
        /*
         * Return CF_DIB as first preference, if WINE is the selection owner
         * and if CF_DIB exists in the cache.
         * If wine dowsn't own the selection we always return CF_DIB
         */
        if ( !X11DRV_IsSelectionOwner() )
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
	/* We support only CF_UNICODETEXT, other formats are synthesized */
        case CF_OEMTEXT:
        case CF_TEXT:
	    return None;

        case CF_UNICODETEXT:
            prop = XA_STRING;
            break;

        case CF_DIB:
        case CF_BITMAP:
        {
            /*
             * Request a PIXMAP, only if WINE is NOT the selection owner,
             * AND the requested format is not in the cache.
             */
            if ( !X11DRV_IsSelectionOwner() && !CLIPBOARD_IsPresent(wFormat) )
            {
              prop = XA_PIXMAP;
              break;
            }
            /* Fall through to the default case in order to use the native format */
        }

        default:
        {
            /*
             * If an X atom is registered for this format, return that
             * Otherwise register a new atom.
             */
            char str[256];
            int plen = strlen(FMT_PREFIX);

            strcpy(str, FMT_PREFIX);

            if (CLIPBOARD_GetFormatName(wFormat, str + plen, sizeof(str) - plen))
                prop = TSXInternAtom(thread_display(), str, False);

            break;
        }
    }

    if (prop == None)
        TRACE("\tNo mapping to X property for Windows clipboard format %d(%s)\n",
              wFormat, CLIPBOARD_GetFormatName(wFormat, NULL, 0));

    return prop;
}

/**************************************************************************
 *	        X11DRV_CLIPBOARD_IsNativeProperty
 *
 *  Checks if a property is a native Wine property type
 */
BOOL X11DRV_CLIPBOARD_IsNativeProperty(Atom prop)
{
    char *itemFmtName = TSXGetAtomName(thread_display(), prop);
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
    char clearSelection[8] = "0";
    int persistent_selection = 1;
    HKEY hkey;
    int fd[2], err;

    /* If persistant selection has been disabled in the .winerc Clipboard section,
     * don't launch the server
     */
    if(!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Clipboard", &hkey))
    {
	char buffer[20];
	DWORD type, count = sizeof(buffer);
	if(!RegQueryValueExA(hkey, "PersistentSelection", 0, &type, buffer, &count))
	    persistent_selection = atoi(buffer);

	/* Get the clear selection preference */
	count = sizeof(clearSelection);
	RegQueryValueExA(hkey, "ClearAllSelections", 0, &type, clearSelection, &count);
	RegCloseKey(hkey);
    }
    if ( !persistent_selection )
        return FALSE;

    /*  Start up persistant WINE X clipboard server process which will
     *  take ownership of the X selection and continue to service selection
     *  requests from other apps.
     */

    if(pipe(fd) == -1) return FALSE;
    fcntl(fd[1], F_SETFD, 1); /* set close on exec */

    selectionWindow = selectionPrevWindow;
    if ( !fork() )
    {
        /* NOTE: This code only executes in the context of the child process
         * Do note make any Wine specific calls here.
         */
        int dbgClasses = 0;
        char selMask[8], dbgClassMask[8];

	close(fd[0]);
        sprintf(selMask, "%d", selectionAcquired);

        /* Build the debug class mask to pass to the server, by inheriting
         * the settings for the clipboard debug channel.
         */
        dbgClasses |= FIXME_ON(clipboard) ? 1 : 0;
        dbgClasses |= ERR_ON(clipboard) ? 2 : 0;
        dbgClasses |= WARN_ON(clipboard) ? 4 : 0;
        dbgClasses |= TRACE_ON(clipboard) ? 8 : 0;
        sprintf(dbgClassMask, "%d", dbgClasses);

        /* Exec the clipboard server passing it the selection and debug class masks */
        execl( BINDIR "/wineclipsrv", "wineclipsrv",
               selMask, dbgClassMask, clearSelection, NULL );
        execlp( "wineclipsrv", "wineclipsrv", selMask, dbgClassMask, clearSelection, NULL );

        /* Exec Failed! */
        perror("Could not start Wine clipboard server");
	write(fd[1], &err, sizeof(err));
        _exit( 1 ); /* Exit the child process */
    }
    close(fd[1]);

    if(read(fd[0], &err, sizeof(err)) > 0) { /* exec failed */
        close(fd[0]);
        return FALSE;
    }
    close(fd[0]);

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
        /* Wait until we lose the selection, timing out after a minute */
        DWORD start_time, timeout, elapsed, ret;

        TRACE("Waiting for clipboard server to acquire selection\n");

        timeout = 60000;
        start_time = GetTickCount();
        elapsed=0;
        do
        {
            ret = MsgWaitForMultipleObjects( 1, &selectionClearEvent, FALSE, timeout - elapsed, QS_ALLINPUT );
            if (ret != WAIT_OBJECT_0+1)
                break;
            elapsed = GetTickCount() - start_time;
            if (elapsed > timeout)
                break;
        }
        while (1);
        if ( ret != WAIT_OBJECT_0 )
            TRACE("Server could not acquire selection, or a timeout occurred!\n");
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
    Display *display = thread_display();
    HWND           hWnd = 0;
    HWND           hWndClipWindow = GetOpenClipboardWindow();
    XEvent         xe;
    Atom           aTargets;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetList=NULL;
    Window         w;
    Window         ownerSelection = 0;
    time_t         maxtm;

    TRACE("enter\n");
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
    w = X11DRV_get_whole_window( GetAncestor(hWnd,GA_ROOT) );

    aTargets = TSXInternAtom(display, "TARGETS", False);

    TRACE("Requesting TARGETS selection for '%s' (owner=%08x)...\n",
          TSXGetAtomName(display, selectionCacheSrc), (unsigned)ownerSelection );
    wine_tsx11_lock();
    XConvertSelection(display, selectionCacheSrc, aTargets,
                    TSXInternAtom(display, "SELECTION_DATA", False),
                    w, CurrentTime);

    /*
     * Wait until SelectionNotify is received
     */
    maxtm = time(NULL) + MAXSELECTIONNOTIFYWAIT; /* Timeout after a maximum wait */
    while( maxtm - time(NULL) > 0 )
    {
       if( XCheckTypedWindowEvent(display, w, SelectionNotify, &xe) )
           if( xe.xselection.selection == selectionCacheSrc )
               break;
    }
    wine_tsx11_unlock();

    /* Verify that the selection returned a valid TARGETS property */
    if ( (xe.xselection.target != aTargets)
          || (xe.xselection.property == None) )
    {
        TRACE("\tExit, could not retrieve TARGETS\n");
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
    Display *display = thread_display();
    Atom	      atype=AnyPropertyType;
    int		      aformat;
    unsigned long     total,nitems,remain,itemSize,val_cnt;
    long              lRequestLength,bwc;
    unsigned char*    val;
    unsigned char*    buffer;
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

   bwc = aformat/8;
   /* we want to read the property, but not it too large of chunks or
      we could hang the cause problems. Lets go for 4k blocks */

    if(TSXGetWindowProperty(display,w,prop,0,4096,False,
                            AnyPropertyType/*reqType*/,
                            &atype, &aformat, &nitems, &remain, &buffer)
        != Success)
    {
        WARN("\tcouldn't read property\n");
        return bRet;
    }
   val = (char*)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,
                          nitems*bwc);
   memcpy(val,buffer,nitems*bwc);
   TSXFree(buffer);

   for (total = nitems*bwc,val_cnt=0; remain;)
   {
       val_cnt +=nitems*bwc;
       TSXGetWindowProperty(display, w, prop,
                          (total / 4), 4096, False,
                          AnyPropertyType, &atype,
                          &aformat, &nitems, &remain,
                          &buffer);

       total += nitems*bwc;
       HeapReAlloc(GetProcessHeap(),0,val, total);
       memcpy(&val[val_cnt], buffer, nitems*(aformat/8));
       TSXFree(buffer);
   }
   nitems = total;

    /*
     * Translate the X property into the appropriate Windows clipboard
     * format, if possible.
     */
    if ( (reqType == XA_STRING)
         && (atype == XA_STRING) && (aformat == 8) )
    /* convert Unix text to CF_UNICODETEXT */
    {
      int 	   i,inlcount = 0;
      char*      lpstr;

      for(i=0; i <= nitems; i++)
          if( val[i] == '\n' ) inlcount++;

      if( (lpstr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nitems + inlcount + 1)) )
      {
	  static UINT text_cp = (UINT)-1;
	  UINT count;
	  HANDLE hUnicodeText;

          for(i=0,inlcount=0; i <= nitems; i++)
          {
             if( val[i] == '\n' ) lpstr[inlcount++]='\r';
             lpstr[inlcount++]=val[i];
          }

	  if(text_cp == (UINT)-1)
	  {
	      HKEY hkey;
	      /* default value */
	      text_cp = CP_ACP;
	      if(!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\x11drv", &hkey))
	      {
	          char buf[20];
		  DWORD type, count = sizeof(buf);
		  if(!RegQueryValueExA(hkey, "TextCP", 0, &type, buf, &count))
		      text_cp = atoi(buf);
		  RegCloseKey(hkey);
	      }
	  }

	  count = MultiByteToWideChar(text_cp, 0, lpstr, -1, NULL, 0);
	  hUnicodeText = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, count * sizeof(WCHAR));
	  if(hUnicodeText)
	  {
	      WCHAR *textW = GlobalLock(hUnicodeText);
	      MultiByteToWideChar(text_cp, 0, lpstr, -1, textW, count);
	      GlobalUnlock(hUnicodeText);
	      if (!SetClipboardData(CF_UNICODETEXT, hUnicodeText))
	      {
                  ERR("Not SET! Need to free our own block\n");
                  GlobalFree(hUnicodeText);
              }
	      bRet = TRUE;
	  }
	  HeapFree(GetProcessHeap(), 0, lpstr);
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

        ReleaseDC(hwnd, hdc);
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
          if (wFormat == CF_METAFILEPICT || wFormat == CF_ENHMETAFILE)
          {
              hClipData = X11DRV_CLIPBOARD_SerializeMetafile(wFormat, (HANDLE)val, cBytes, FALSE);
          }
          else
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
    HeapFree(GetProcessHeap(),0,val);
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
    Display *display = thread_display();
    Atom xaClipboard = TSXInternAtom(display, "CLIPBOARD", False);
    int clearAllSelections = 0;
    HKEY hkey;

    if(!RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\Wine\\Wine\\Config\\Clipboard", &hkey))
    {
	char buffer[20];
	DWORD type, count = sizeof(buffer);
	if(!RegQueryValueExA(hkey, "ClearAllSelections", 0, &type, buffer, &count))
	    clearAllSelections = atoi(buffer);
        RegCloseKey(hkey);
    }

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
 *		ReleaseClipboard (X11DRV.@)
 *  Voluntarily release all currently owned X selections
 */
void X11DRV_ReleaseClipboard(void)
{
    Display *display = thread_display();
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

        wine_tsx11_lock();

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
        wine_tsx11_unlock();
    }

    /* Get rid of any Pixmap resources we may still have */
    while (prop_head)
    {
        PROPERTY *prop = prop_head;
        prop_head = prop->next;
        XFreePixmap( gdi_display, prop->pixmap );
        HeapFree( GetProcessHeap(), 0, prop );
    }
}

/**************************************************************************
 *		AcquireClipboard (X11DRV.@)
 */
void X11DRV_AcquireClipboard(void)
{
    Display *display = thread_display();
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
        owner = X11DRV_get_whole_window( GetAncestor( hWndClipWindow, GA_ROOT ) );

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
	    selectionWindow = owner;
	    TRACE("Grabbed X selection, owner=(%08x)\n", (unsigned) owner);
        }
    }
}

/**************************************************************************
 *		IsClipboardFormatAvailable (X11DRV.@)
 *
 * Checks if the specified format is available in the current selection
 * Only invoked when WINE is not the selection owner
 */
BOOL X11DRV_IsClipboardFormatAvailable(UINT wFormat)
{
    Display *display = thread_display();
    Atom xaClipboard = TSXInternAtom(display, _CLIPBOARD, False);
    Window ownerPrimary = TSXGetSelectionOwner(display,XA_PRIMARY);
    Window ownerClipboard = TSXGetSelectionOwner(display,xaClipboard);

    TRACE("enter for %d\n", wFormat);

    /*
     * If the selection has not been previously cached, or the selection has changed,
     * try and cache the list of available selection targets from the current selection.
     */
    if ( !cSelectionTargets || (PrimarySelectionOwner != ownerPrimary)
                         || (ClipboardSelectionOwner != ownerClipboard) )
    {
        /*
         * First try caching the CLIPBOARD selection.
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
    {
	TRACE("There is no selection owner\n");
        return FALSE;
    }

    /* Check if the format is available in the clipboard cache */
    if ( CLIPBOARD_IsPresent(wFormat) )
        return TRUE;

    /*
     * Many X client apps (such as XTerminal) don't support being queried
     * for the "TARGETS" target atom. To handle such clients we must actually
     * try to convert the selection to the requested type.
     */
    if ( !cSelectionTargets )
        return X11DRV_GetClipboardData( wFormat );

    TRACE("There is no selection\n");
    return FALSE;
}

/**************************************************************************
 *		RegisterClipboardFormat (X11DRV.@)
 *
 * Registers a custom X clipboard format
 * Returns: Format id or 0 on failure
 */
INT X11DRV_RegisterClipboardFormat( LPCSTR FormatName )
{
    Display *display = thread_display();
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

    return prop;
}

/**************************************************************************
 *		IsSelectionOwner (X11DRV.@)
 *
 * Returns: TRUE - We(WINE) own the selection, FALSE - Selection not owned by us
 */
BOOL X11DRV_IsSelectionOwner(void)
{
    return selectionAcquired;
}

/**************************************************************************
 *		SetClipboardData (X11DRV.@)
 *
 * We don't need to do anything special here since the clipboard code
 * maintains the cache.
 *
 */
void X11DRV_SetClipboardData(UINT wFormat)
{
    /* Make sure we have acquired the X selection */
    X11DRV_AcquireClipboard();
}

/**************************************************************************
 *		GetClipboardData (X11DRV.@)
 *
 * This method is invoked only when we DO NOT own the X selection
 *
 * NOTE: Clipboard driver get requests only for CF_UNICODETEXT data.
 * We always get the data from the selection client each time,
 * since we have no way of determining if the data in our cache is stale.
 */
BOOL X11DRV_GetClipboardData(UINT wFormat)
{
    Display *display = thread_display();
    BOOL bRet = selectionAcquired;
    HWND hWndClipWindow = GetOpenClipboardWindow();
    HWND hWnd = (hWndClipWindow) ? hWndClipWindow : GetActiveWindow();
    LPWINE_CLIPFORMAT lpFormat;

    TRACE("%d\n", wFormat);

    if (!selectionAcquired)
    {
        XEvent xe;
        Atom propRequest;
        Window w = X11DRV_get_whole_window( GetAncestor( hWnd, GA_ROOT ));
        if(!w)
        {
            FIXME("No parent win found %p %p\n", hWnd, hWndClipWindow);
            return FALSE;
        }

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
            wine_tsx11_lock();
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
            wine_tsx11_unlock();

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

        TRACE("\tpresent %s = %i\n", CLIPBOARD_GetFormatName(wFormat, NULL, 0), bRet );
    }

    TRACE("Returning %d\n", bRet);

    return bRet;
}

/**************************************************************************
 *		ResetSelectionOwner (X11DRV.@)
 *
 * Called from DestroyWindow() to prevent X selection from being lost when
 * a top level window is destroyed, by switching ownership to another top
 * level window.
 * Any top level window can own the selection. See X11DRV_CLIPBOARD_Acquire
 * for a more detailed description of this.
 */
void X11DRV_ResetSelectionOwner(HWND hwnd, BOOL bFooBar)
{
    Display *display = thread_display();
    HWND hWndClipOwner = 0;
    HWND tmp;
    Window XWnd = X11DRV_get_whole_window(hwnd);
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

    TRACE("clipboard owner = %p, selection window = %08x\n",
          hWndClipOwner, (unsigned)selectionWindow);

    /* now try to salvage current selection from being destroyed by X */

    TRACE("\tchecking %08x\n", (unsigned) XWnd);

    selectionPrevWindow = selectionWindow;
    selectionWindow = None;

    if (!(tmp = GetWindow( hwnd, GW_HWNDNEXT ))) tmp = GetWindow( hwnd, GW_HWNDFIRST );
    if (tmp && tmp != hwnd) selectionWindow = X11DRV_get_whole_window(tmp);

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
    PROPERTY *prop = HeapAlloc( GetProcessHeap(), 0, sizeof(*prop) );
    if (!prop) return FALSE;
    prop->atom = property;
    prop->pixmap = pixmap;
    prop->next = prop_head;
    prop_head = prop;
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
    PROPERTY **prop = &prop_head;

    while (*prop)
    {
        if ((*prop)->atom == property)
        {
            PROPERTY *next = (*prop)->next;
            XFreePixmap( gdi_display, (*prop)->pixmap );
            HeapFree( GetProcessHeap(), 0, *prop );
            *prop = next;
        }
        else prop = &(*prop)->next;
    }
}

/**************************************************************************
 *		X11DRV_GetClipboardFormatName
 */
BOOL X11DRV_GetClipboardFormatName( UINT wFormat, LPSTR retStr, UINT maxlen )
{
    BOOL bRet = FALSE;
    char *itemFmtName = TSXGetAtomName(thread_display(), wFormat);
    INT prefixlen = strlen(FMT_PREFIX);

    if ( 0 == strncmp(itemFmtName, FMT_PREFIX, prefixlen ) )
    {
        strncpy(retStr, itemFmtName + prefixlen, maxlen);
        bRet = TRUE;
    }

    TSXFree(itemFmtName);

    return bRet;
}


/**************************************************************************
 *		CLIPBOARD_SerializeMetafile
 */
HANDLE X11DRV_CLIPBOARD_SerializeMetafile(INT wformat, HANDLE hdata, INT cbytes, BOOL out)
{
    HANDLE h = 0;

    if (out) /* Serialize out, caller should free memory */
    {
        if (wformat == CF_METAFILEPICT)
        {
            LPMETAFILEPICT lpmfp = (LPMETAFILEPICT) GlobalLock(hdata);
            int size = GetMetaFileBitsEx(lpmfp->hMF, 0, NULL);

            h = GlobalAlloc(0, size + sizeof(METAFILEPICT));
            if (h)
            {
                LPVOID pdata = GlobalLock(h);

                memcpy(pdata, lpmfp, sizeof(METAFILEPICT));
                GetMetaFileBitsEx(lpmfp->hMF, size, pdata + sizeof(METAFILEPICT));

                GlobalUnlock(h);
            }

            GlobalUnlock(hdata);
        }
        else if (wformat == CF_ENHMETAFILE)
        {
            int size = GetEnhMetaFileBits(hdata, 0, NULL);

            h = GlobalAlloc(0, size);
            if (h)
            {
                LPVOID pdata = GlobalLock(h);
                GetEnhMetaFileBits(hdata, size, pdata);
                GlobalUnlock(h);
            }
        }
    }
    else
    {
        if (wformat == CF_METAFILEPICT)
        {
            h = GlobalAlloc(0, sizeof(METAFILEPICT));
            if (h)
            {
                LPMETAFILEPICT pmfp = (LPMETAFILEPICT) GlobalLock(h);

                memcpy(pmfp, (LPVOID)hdata, sizeof(METAFILEPICT));
                pmfp->hMF = SetMetaFileBitsEx(cbytes - sizeof(METAFILEPICT),
                                              (LPVOID)hdata + sizeof(METAFILEPICT));

                GlobalUnlock(h);
            }
        }
        else if (wformat == CF_ENHMETAFILE)
        {
            h = SetEnhMetaFileBits(cbytes, (LPVOID)hdata);
        }
    }

    return h;
}
