/*
 *  Wine Clipboard Server
 *
 *      Copyright 1999  Noel Borthwick
 *
 * USAGE:
 *       wineclipsrv [selection_mask] [debugClass_mask] [clearAllSelections]
 *
 * The optional selection-mask argument is a bit mask of the selection
 * types to be acquired. Currently two selections are supported:
 *   1. PRIMARY (mask value 1)
 *   2. CLIPBOARD (mask value 2).
 *
 * debugClass_mask is a bit mask of all debugging classes for which messages
 * are to be output. The standard Wine debug class set FIXME(1), ERR(2),
 * WARN(4) and TRACE(8) are supported.
 *
 * If clearAllSelections == 1 *all* selections are lost whenever a SelectionClear
 * event is received.
 *
 * If no arguments are supplied the server aquires all selections. (mask value 3)
 * and defaults to output of only FIXME(1) and ERR(2) messages. The default for
 * clearAllSelections is 0.
 *
 * NOTES:
 *
 *    The Wine Clipboard Server is a standalone XLib application whose 
 * purpose is to manage the X selection when Wine exits.
 * The server itself is started automatically with the appropriate
 * selection masks, whenever Wine exits after acquiring the PRIMARY and/or
 * CLIPBOARD selection. (See X11DRV_CLIPBOARD_ResetOwner)
 * When the server starts, it first proceeds to capture the selection data from
 * Wine and then takes over the selection ownership. It does this by querying
 * the current selection owner(of the specified selections) for the TARGETS
 * selection target. It then proceeds to cache all the formats exposed by
 * TARGETS. If the selection does not support the TARGETS target, or if no
 * target formats are exposed, the server simply exits.
 * Once the cache has been filled, the server then actually acquires ownership
 * of the respective selection and begins fielding selection requests.
 * Selection requests are serviced from the cache. If a selection is lost the
 * server flushes its internal cache, destroying all data previously saved.
 * Once ALL selections have been lost the server terminates.
 *
 * TODO:
 */

#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

/*
 *  Lightweight debug definitions for Wine Clipboard Server.
 *  The standard FIXME, ERR, WARN & TRACE classes are supported
 *  without debug channels.
 *  The standard defines NO_TRACE_MSGS and NO_DEBUG_MSGS will compile out
 *  TRACE, WARN and ERR and FIXME message displays.
 */

/* Internal definitions (do not use these directly) */

enum __DEBUG_CLASS { __DBCL_FIXME, __DBCL_ERR, __DBCL_WARN, __DBCL_TRACE, __DBCL_COUNT };

extern char __debug_msg_enabled[__DBCL_COUNT];

extern const char * const debug_cl_name[__DBCL_COUNT];

#define DEBUG_CLASS_COUNT __DBCL_COUNT

#define __GET_DEBUGGING(dbcl)    (__debug_msg_enabled[(dbcl)])
#define __SET_DEBUGGING(dbcl,on) (__debug_msg_enabled[(dbcl)] = (on))


#define __DPRINTF(dbcl) \
    (!__GET_DEBUGGING(dbcl) || \
    (printf("%s:%s:%s ", debug_cl_name[(dbcl)], progname, __FUNCTION__),0)) \
    ? 0 : printf

#define __DUMMY_DPRINTF 1 ? (void)0 : (void)((int (*)(char *, ...)) NULL)

/* use configure to allow user to compile out debugging messages */
#ifndef NO_TRACE_MSGS
  #define TRACE        __DPRINTF(__DBCL_TRACE)
#else
  #define TRACE        __DUMMY_DPRINTF
#endif /* NO_TRACE_MSGS */

#ifndef NO_DEBUG_MSGS
  #define WARN         __DPRINTF(__DBCL_WARN)
  #define FIXME        __DPRINTF(__DBCL_FIXME)
#else
  #define WARN         __DUMMY_DPRINTF
  #define FIXME        __DUMMY_DPRINTF
#endif /* NO_DEBUG_MSGS */

/* define error macro regardless of what is configured */
#define ERR        __DPRINTF(__DBCL_ERR)


#define TRUE 1
#define FALSE 0
typedef int BOOL;

/* Internal definitions for debugging messages(do not use these directly) */
const char * const debug_cl_name[] = { "fixme", "err", "warn", "trace" };
char __debug_msg_enabled[DEBUG_CLASS_COUNT] = {1, 1, 0, 0};


/* Selection masks */

#define S_NOSELECTION    0
#define S_PRIMARY        1
#define S_CLIPBOARD      2

/* Debugging class masks */

#define C_FIXME          1
#define C_ERR            2
#define C_WARN           4
#define C_TRACE          8

/*
 * Global variables 
 */

static Display *g_display = NULL;
static int screen_num;
static char *progname;                 /* name this program was invoked by */
static Window g_win = 0;               /* the hidden clipboard server window */
static GC g_gc = 0;

static char *g_szOutOfMemory = "Insufficient memory!\n";

/* X selection context info */
static char _CLIPBOARD[] = "CLIPBOARD";        /* CLIPBOARD atom name */
static int  g_selectionToAcquire = 0;          /* Masks for the selection to be acquired */
static int  g_selectionAcquired = 0;           /* Contains the current selection masks */
static int  g_clearAllSelections = 0;          /* If TRUE *all* selections are lost on SelectionClear */
    
/* Selection cache */
typedef struct tag_CACHEENTRY
{
    Atom target;
    Atom type;
    int nFormat;
    int nElements;
    void *pData;
} CACHEENTRY, *PCACHEENTRY;

static PCACHEENTRY g_pPrimaryCache = NULL;     /* Primary selection cache */
static PCACHEENTRY g_pClipboardCache = NULL;   /* Clipboard selection cache */
static unsigned long g_cPrimaryTargets = 0;    /* Number of TARGETS reported by PRIMARY selection */
static unsigned long g_cClipboardTargets = 0;  /* Number of TARGETS reported by CLIPBOARD selection */

/* Event names */
static const char * const event_names[] =
{
  "", "", "KeyPress", "KeyRelease", "ButtonPress", "ButtonRelease",
  "MotionNotify", "EnterNotify", "LeaveNotify", "FocusIn", "FocusOut",
  "KeymapNotify", "Expose", "GraphicsExpose", "NoExpose", "VisibilityNotify",
  "CreateNotify", "DestroyNotify", "UnmapNotify", "MapNotify", "MapRequest",
  "ReparentNotify", "ConfigureNotify", "ConfigureRequest", "GravityNotify",
  "ResizeRequest", "CirculateNotify", "CirculateRequest", "PropertyNotify",
  "SelectionClear", "SelectionRequest", "SelectionNotify", "ColormapNotify",
  "ClientMessage", "MappingNotify"
};


/*
 * Prototypes 
 */

BOOL Init(int argc, char **argv);
void TerminateServer( int ret );
int AcquireSelection();
int CacheDataFormats( Atom SelectionSrc, PCACHEENTRY *ppCache );
void EmptyCache(PCACHEENTRY pCache, int nItems);
BOOL FillCacheEntry( Atom SelectionSrc, Atom target, PCACHEENTRY pCacheEntry );
BOOL LookupCacheItem( Atom selection, Atom target, PCACHEENTRY *ppCacheEntry );
void EVENT_ProcessEvent( XEvent *event );
Atom EVENT_SelectionRequest_MULTIPLE( XSelectionRequestEvent *pevent );
void EVENT_SelectionRequest( XSelectionRequestEvent *event, BOOL bIsMultiple );
void EVENT_SelectionClear( XSelectionClearEvent *event );
void EVENT_PropertyNotify( XPropertyEvent *event );
Pixmap DuplicatePixmap(Pixmap pixmap);
void TextOut(Window win, GC gc, char *pStr);
void getGC(Window win, GC *gc);


void main(int argc, char **argv)
{
    XEvent event;

    if ( !Init(argc, argv) )
        exit(0);
    
    /* Acquire the selection after retrieving all clipboard data
     * owned by the current selection owner. If we were unable to
     * Acquire any selection, terminate right away.
     */
    if ( AcquireSelection() == S_NOSELECTION )
        TerminateServer(0);

    TRACE("Clipboard server running...\n");
    
    /* Start an X event loop */
    while (1)
    {
        XNextEvent(g_display, &event);
        
        EVENT_ProcessEvent( &event );
    }
}


/**************************************************************************
 *		Init()
 *  Initialize the clipboard server
 */
BOOL Init(int argc, char **argv)
{
    unsigned int width, height;	/* window size */
    unsigned int border_width = 4;	/* four pixels */
    unsigned int display_width, display_height;
    char *window_name = "Wine Clipboard Server";
    XSizeHints *size_hints = NULL;
    XWMHints *wm_hints = NULL;
    XClassHint *class_hints = NULL;
    XTextProperty windowName;
    char *display_name = NULL;
    
    progname = argv[0];
    
    if (!(size_hints = XAllocSizeHints()))
    {
        ERR(g_szOutOfMemory);
        return 0;
    }
    if (!(wm_hints = XAllocWMHints()))
    {
        ERR(g_szOutOfMemory);
        return 0;
    }
    if (!(class_hints = XAllocClassHint()))
    {
        ERR(g_szOutOfMemory);
        return 0;
    }
    
    /* connect to X server */
    if ( (g_display=XOpenDisplay(display_name)) == NULL )
    {
        ERR( "cannot connect to X server %s\n", XDisplayName(display_name));
        return 0;
    }
    
    /* get screen size from display structure macro */
    screen_num = DefaultScreen(g_display);
    display_width = DisplayWidth(g_display, screen_num);
    display_height = DisplayHeight(g_display, screen_num);
    
    /* size window with enough room for text */
    width = display_width/3, height = display_height/4;
    
    /* create opaque window */
    g_win = XCreateSimpleWindow(g_display, RootWindow(g_display,screen_num),
                    0, 0, width, height, border_width, BlackPixel(g_display,
                    screen_num), WhitePixel(g_display,screen_num));
    
    
    /* Set size hints for window manager.  The window manager may
     * override these settings. */
    
    /* x, y, width, and height hints are now taken from
     * the actual settings of the window when mapped. Note
     * that PPosition and PSize must be specified anyway. */
    
    size_hints->flags = PPosition | PSize | PMinSize;
    size_hints->min_width = 300;
    size_hints->min_height = 200;
    
    /* These calls store window_name into XTextProperty structures
     * and sets the other fields properly. */
    if (XStringListToTextProperty(&window_name, 1, &windowName) == 0)
    {
       ERR( "structure allocation for windowName failed.\n");
       TerminateServer(-1);
    }
            
    wm_hints->initial_state = NormalState;
    wm_hints->input = True;
    wm_hints->flags = StateHint | InputHint;
    
    class_hints->res_name = progname;
    class_hints->res_class = "WineClipSrv";

    XSetWMProperties(g_display, g_win, &windowName, NULL, 
                    argv, argc, size_hints, wm_hints, 
                    class_hints);

    /* Select event types wanted */
    XSelectInput(g_display, g_win, ExposureMask | KeyPressMask | 
                    ButtonPressMask | StructureNotifyMask | PropertyChangeMask );
    
    /* create GC for text and drawing */
    getGC(g_win, &g_gc);
    
    /* Display window */
    /* XMapWindow(g_display, g_win); */

    /* Set the selections to be acquired from the command line argument.
     * If none specified, default to all selections we understand.
     */
    if (argc > 1)
        g_selectionToAcquire = atoi(argv[1]);
    else
        g_selectionToAcquire = S_PRIMARY | S_CLIPBOARD;

    /* Set the debugging class state from the command line argument */
    if (argc > 2)
    {
        int dbgClasses = atoi(argv[2]);
        
        __SET_DEBUGGING(__DBCL_FIXME, dbgClasses & C_FIXME);
        __SET_DEBUGGING(__DBCL_ERR, dbgClasses & C_ERR);
        __SET_DEBUGGING(__DBCL_WARN, dbgClasses & C_WARN);
        __SET_DEBUGGING(__DBCL_TRACE, dbgClasses & C_TRACE);
    }
        
    /* Set the "ClearSelections" state from the command line argument */
    if (argc > 3)
        g_clearAllSelections = atoi(argv[3]);
    
    return TRUE;
}


/**************************************************************************
 *		TerminateServer()
 */
void TerminateServer( int ret )
{
    TRACE("Terminating Wine clipboard server...\n");
    
    /* Free Primary and Clipboard selection caches */
    EmptyCache(g_pPrimaryCache, g_cPrimaryTargets);
    EmptyCache(g_pClipboardCache, g_cClipboardTargets);

    if (g_gc)
        XFreeGC(g_display, g_gc);

    if (g_display)
        XCloseDisplay(g_display);

    exit(ret);
}


/**************************************************************************
 *		AcquireSelection()
 *
 * Acquire the selection after retrieving all clipboard data owned by 
 * the current selection owner.
 */
int AcquireSelection()
{
    Atom xaClipboard = XInternAtom(g_display, _CLIPBOARD, False);

    /*
     *  For all selections we need to acquire, get a list of all targets
     *  supplied by the current selection owner.
     */
    if (g_selectionToAcquire & S_PRIMARY)
    {
        TRACE("Acquiring PRIMARY selection...\n");
        g_cPrimaryTargets = CacheDataFormats( XA_PRIMARY, &g_pPrimaryCache );
        TRACE("Cached %ld formats...\n", g_cPrimaryTargets);
    }
    if (g_selectionToAcquire & S_CLIPBOARD)
    {
        TRACE("Acquiring CLIPBOARD selection...\n");
        g_cClipboardTargets = CacheDataFormats( xaClipboard, &g_pClipboardCache );
        TRACE("Cached %ld formats...\n", g_cClipboardTargets);
    }

    /*
     * Now that we have cached the data, we proceed to acquire the selections
     */
    if (g_cPrimaryTargets)
    {
        /* Acquire the PRIMARY selection */
        while (XGetSelectionOwner(g_display,XA_PRIMARY) != g_win)
            XSetSelectionOwner(g_display, XA_PRIMARY, g_win, CurrentTime);
        
        g_selectionAcquired |= S_PRIMARY;
    }
    else
        TRACE("No PRIMARY targets - ownership not acquired.\n");
    
    if (g_cClipboardTargets)
    {
        /* Acquire the CLIPBOARD selection */
        while (XGetSelectionOwner(g_display,xaClipboard) != g_win)
            XSetSelectionOwner(g_display, xaClipboard, g_win, CurrentTime);

        g_selectionAcquired |= S_CLIPBOARD;
    }
    else
        TRACE("No CLIPBOARD targets - ownership not acquired.\n");

    return g_selectionAcquired;
}

/**************************************************************************
 *		CacheDataFormats
 *
 * Allocates and caches the list of data formats available from the current selection.
 * This queries the selection owner for the TARGETS property and saves all
 * reported property types.
 */
int CacheDataFormats( Atom SelectionSrc, PCACHEENTRY *ppCache )
{
    XEvent         xe;
    Atom           aTargets;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    unsigned long  cSelectionTargets = 0;
    Atom*	   targetList=NULL;
    Window         ownerSelection = 0;

    if (!ppCache)
        return 0;
    *ppCache = NULL;

    /* Get the selection owner */
    ownerSelection = XGetSelectionOwner(g_display, SelectionSrc);
    if ( ownerSelection == None )
        return cSelectionTargets;

    /*
     * Query the selection owner for the TARGETS property
     */
    aTargets = XInternAtom(g_display, "TARGETS", False);

    TRACE("Requesting TARGETS selection for '%s' (owner=%08x)...\n",
          XGetAtomName(g_display, SelectionSrc), (unsigned)ownerSelection );
          
    XConvertSelection(g_display, SelectionSrc, aTargets,
                    XInternAtom(g_display, "SELECTION_DATA", False),
                    g_win, CurrentTime);

    /*
     * Wait until SelectionNotify is received
     */
    while( TRUE )
    {
       if( XCheckTypedWindowEvent(g_display, g_win, SelectionNotify, &xe) )
           if( xe.xselection.selection == SelectionSrc )
               break;
    }

    /* Verify that the selection returned a valid TARGETS property */
    if ( (xe.xselection.target != aTargets)
          || (xe.xselection.property == None) )
    {
        TRACE("\tCould not retrieve TARGETS\n");
        return cSelectionTargets;
    }

    /* Read the TARGETS property contents */
    if(XGetWindowProperty(g_display, xe.xselection.requestor, xe.xselection.property,
                            0, 0x3FFF, True, AnyPropertyType/*XA_ATOM*/, &atype, &aformat,
                            &cSelectionTargets, &remain, (unsigned char**)&targetList) != Success)
        TRACE("\tCouldn't read TARGETS property\n");
    else
    {
       TRACE("\tType %s,Format %d,nItems %ld, Remain %ld\n",
             XGetAtomName(g_display,atype),aformat,cSelectionTargets, remain);
       /*
        * The TARGETS property should have returned us a list of atoms
        * corresponding to each selection target format supported.
        */
       if( (atype == XA_ATOM || atype == aTargets) && aformat == 32 )
       {
          int i;

          /* Allocate the selection cache */
          *ppCache = (PCACHEENTRY)calloc(cSelectionTargets, sizeof(CACHEENTRY));
          
          /* Cache these formats in the selection cache */
          for (i = 0; i < cSelectionTargets; i++)
          {
              char *itemFmtName = XGetAtomName(g_display, targetList[i]);
          
              TRACE("\tAtom# %d: '%s'\n", i, itemFmtName);
              
              /* Populate the cache entry */
              if (!FillCacheEntry( SelectionSrc, targetList[i], &((*ppCache)[i])))
                  ERR("Failed to fill cache entry!\n");

              XFree(itemFmtName);
          }
       }

       /* Free the list of targets */
       XFree(targetList);
    }
    
    return cSelectionTargets;
}

/***********************************************************************
 *           FillCacheEntry
 *
 *   Populates the specified cache entry
 */
BOOL FillCacheEntry( Atom SelectionSrc, Atom target, PCACHEENTRY pCacheEntry )
{
    XEvent            xe;
    Window            w;
    Atom              prop, reqType;
    Atom	      atype=AnyPropertyType;
    int		      aformat;
    unsigned long     nitems,remain,itemSize;
    long              lRequestLength;
    unsigned char*    val=NULL;
    BOOL              bRet = FALSE;

    TRACE("Requesting %s selection from %s...\n",
          XGetAtomName(g_display, target),
          XGetAtomName(g_display, SelectionSrc) );

    /* Ask the selection owner to convert the selection to the target format */
    XConvertSelection(g_display, SelectionSrc, target,
                    XInternAtom(g_display, "SELECTION_DATA", False),
                    g_win, CurrentTime);

    /* wait until SelectionNotify is received */
    while( TRUE )
    {
       if( XCheckTypedWindowEvent(g_display, g_win, SelectionNotify, &xe) )
           if( xe.xselection.selection == SelectionSrc )
               break;
    }

    /* Now proceed to retrieve the actual converted property from
     * the SELECTION_DATA atom */

    w = xe.xselection.requestor;
    prop = xe.xselection.property;
    reqType = xe.xselection.target;
    
    if(prop == None)
    {
        TRACE("\tOwner failed to convert selection!\n");
        return bRet;
    }
       
    TRACE("\tretrieving property %s from window %ld into %s\n",
          XGetAtomName(g_display,reqType), (long)w, XGetAtomName(g_display,prop) );

    /*
     * First request a zero length in order to figure out the request size.
     */
    if(XGetWindowProperty(g_display,w,prop,0,0,False, AnyPropertyType/*reqType*/,
                            &atype, &aformat, &nitems, &itemSize, &val) != Success)
    {
        WARN("\tcouldn't get property size\n");
        return bRet;
    }

    /* Free zero length return data if any */
    if ( val )
    {
       XFree(val);
       val = NULL;
    }
    
    TRACE("\tretrieving %ld bytes...\n", itemSize * aformat/8);
    lRequestLength = (itemSize * aformat/8)/4  + 1;
    
    /*
     * Retrieve the actual property in the required X format.
     */
    if(XGetWindowProperty(g_display,w,prop,0,lRequestLength,False,AnyPropertyType/*reqType*/,
                          &atype, &aformat, &nitems, &remain, &val) != Success)
    {
        WARN("\tcouldn't read property\n");
        return bRet;
    }

    TRACE("\tType %s,Format %d,nitems %ld,remain %ld,value %s\n",
          atype ? XGetAtomName(g_display,atype) : NULL, aformat,nitems,remain,val);
    
    if (remain)
    {
        WARN("\tCouldn't read entire property- selection may be too large! Remain=%ld\n", remain);
        goto END;
    }

    /*
     * Populate the cache entry
     */
    pCacheEntry->target = target;
    pCacheEntry->type = atype;
    pCacheEntry->nFormat = aformat;
    pCacheEntry->nElements = nitems;

    if (atype == XA_PIXMAP)
    {
        Pixmap *pPixmap = (Pixmap *)val;
        Pixmap newPixmap = DuplicatePixmap( *pPixmap );
        pPixmap = (Pixmap*)calloc(1, sizeof(Pixmap));
        *pPixmap = newPixmap;
        pCacheEntry->pData = pPixmap;
    }
    else
        pCacheEntry->pData = val;

END:
    /* Delete the property on the window now that we are done
     * This will send a PropertyNotify event to the selection owner. */
    XDeleteProperty(g_display,w,prop);
    
    return TRUE;
}


/***********************************************************************
 *           LookupCacheItem
 *
 *   Lookup a target atom in the cache and get the matching cache entry
 */
BOOL LookupCacheItem( Atom selection, Atom target, PCACHEENTRY *ppCacheEntry )
{
    int i;
    int             nCachetargets = 0;
    PCACHEENTRY     pCache = NULL;
    Atom            xaClipboard = XInternAtom(g_display, _CLIPBOARD, False);

    /* Locate the cache to be used based on the selection type */
    if ( selection == XA_PRIMARY )
    {
        pCache = g_pPrimaryCache;
        nCachetargets = g_cPrimaryTargets;
    }
    else if ( selection == xaClipboard )
    {
        pCache = g_pClipboardCache;
        nCachetargets = g_cClipboardTargets;
    }

    if (!pCache || !ppCacheEntry)
        return FALSE;

    *ppCacheEntry = NULL;
    
    /* Look for the target item in the cache */
    for (i = 0; i < nCachetargets; i++)
    {
        if (pCache[i].target == target)
        {
            *ppCacheEntry = &pCache[i];
            return TRUE;
        }
    }

    return FALSE;
}


/***********************************************************************
 *           EmptyCache
 *
 *   Empties the specified cache
 */
void EmptyCache(PCACHEENTRY pCache, int nItems)
{
    int i;
    
    if (!pCache)
        return;

    /* Release all items in the cache */
    for (i = 0; i < nItems; i++)
    {
        if (pCache[i].target && pCache[i].pData)
        {
            /* If we have a Pixmap, free it first */
            if (pCache[i].target == XA_PIXMAP || pCache[i].target == XA_BITMAP)
            {
                Pixmap *pPixmap = (Pixmap *)pCache[i].pData;
                
                TRACE("Freeing %s (handle=%ld)...\n",
                      XGetAtomName(g_display, pCache[i].target), *pPixmap);
                
                XFreePixmap(g_display, *pPixmap);

                /* Free the cached data item (allocated by us) */
                free(pCache[i].pData);
            }
            else
            {
                TRACE("Freeing %s (%p)...\n",
                      XGetAtomName(g_display, pCache[i].target), pCache[i].pData);
            
                /* Free the cached data item (allocated by X) */
                XFree(pCache[i].pData);
            }
        }
    }

    /* Destroy the cache */
    free(pCache);
}


/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
void EVENT_ProcessEvent( XEvent *event )
{
  /*
  TRACE(" event %s for Window %08lx\n", event_names[event->type], event->xany.window );
  */
    
  switch (event->type)
  {
      case Expose:
          /* don't draw the window */
          if (event->xexpose.count != 0)
                  break;

          /* Output something */
          TextOut(g_win, g_gc, "Click here to terminate");
          break;
          
      case ConfigureNotify:
          break;
          
      case ButtonPress:
              /* fall into KeyPress (no break) */
      case KeyPress:
          TerminateServer(1);
          break;

      case SelectionRequest:
          EVENT_SelectionRequest( (XSelectionRequestEvent *)event, FALSE );
          break;
  
      case SelectionClear:
          EVENT_SelectionClear( (XSelectionClearEvent*)event );
          break;
        
      case PropertyNotify:
          // EVENT_PropertyNotify( (XPropertyEvent *)event );
          break;

      default: /* ignore all other events */
          break;
          
  } /* end switch */

}


/***********************************************************************
 *           EVENT_SelectionRequest_MULTIPLE
 *  Service a MULTIPLE selection request event
 *  rprop contains a list of (target,property) atom pairs.
 *  The first atom names a target and the second names a property.
 *  The effect is as if we have received a sequence of SelectionRequest events
 *  (one for each atom pair) except that:
 *  1. We reply with a SelectionNotify only when all the requested conversions
 *  have been performed.
 *  2. If we fail to convert the target named by an atom in the MULTIPLE property,
 *  we replace the atom in the property by None.
 */
Atom EVENT_SelectionRequest_MULTIPLE( XSelectionRequestEvent *pevent )
{
    Atom           rprop;
    Atom           atype=AnyPropertyType;
    int		   aformat;
    unsigned long  remain;
    Atom*	   targetPropList=NULL;
    unsigned long  cTargetPropList = 0;
/*  Atom           xAtomPair = XInternAtom(g_display, "ATOM_PAIR", False); */
    
   /* If the specified property is None the requestor is an obsolete client.
    * We support these by using the specified target atom as the reply property.
    */
    rprop = pevent->property;
    if( rprop == None ) 
        rprop = pevent->target;
    if (!rprop)
        goto END;

    /* Read the MULTIPLE property contents. This should contain a list of
     * (target,property) atom pairs.
     */
    if(XGetWindowProperty(g_display, pevent->requestor, rprop,
                            0, 0x3FFF, False, AnyPropertyType, &atype, &aformat,
                            &cTargetPropList, &remain, (unsigned char**)&targetPropList) != Success)
        TRACE("\tCouldn't read MULTIPLE property\n");
    else
    {
       TRACE("\tType %s,Format %d,nItems %ld, Remain %ld\n",
                     XGetAtomName(g_display,atype),aformat,cTargetPropList,remain);

       /*
        * Make sure we got what we expect.
        * NOTE: According to the X-ICCCM Version 2.0 documentation the property sent
        * in a MULTIPLE selection request should be of type ATOM_PAIR.
        * However some X apps(such as XPaint) are not compliant with this and return
        * a user defined atom in atype when XGetWindowProperty is called.
        * The data *is* an atom pair but is not denoted as such.
        */
       if(aformat == 32 /* atype == xAtomPair */ )
       {
          int i;
          
          /* Iterate through the ATOM_PAIR list and execute a SelectionRequest
           * for each (target,property) pair */

          for (i = 0; i < cTargetPropList; i+=2)
          {
              char *targetName = XGetAtomName(g_display, targetPropList[i]);
              char *propName = XGetAtomName(g_display, targetPropList[i+1]);
              XSelectionRequestEvent event;

              TRACE("MULTIPLE(%d): Target='%s' Prop='%s'\n", i/2, targetName, propName);
              XFree(targetName);
              XFree(propName);
              
              /* We must have a non "None" property to service a MULTIPLE target atom */
              if ( !targetPropList[i+1] )
              {
                  TRACE("\tMULTIPLE(%d): Skipping target with empty property!", i);
                  continue;
              }
              
              /* Set up an XSelectionRequestEvent for this (target,property) pair */
              memcpy( &event, pevent, sizeof(XSelectionRequestEvent) );
              event.target = targetPropList[i];
              event.property = targetPropList[i+1];
                  
              /* Fire a SelectionRequest, informing the handler that we are processing
               * a MULTIPLE selection request event.
               */
              EVENT_SelectionRequest( &event, TRUE );
          }
       }

       /* Free the list of targets/properties */
       XFree(targetPropList);
    }

END:
    return rprop;
}


/***********************************************************************
 *           EVENT_SelectionRequest
 *  Process an event selection request event.
 *  The bIsMultiple flag is used to signal when EVENT_SelectionRequest is called
 *  recursively while servicing a "MULTIPLE" selection target.
 *
 */
void EVENT_SelectionRequest( XSelectionRequestEvent *event, BOOL bIsMultiple )
{
  XSelectionEvent result;
  Atom 	          rprop = None;
  Window          request = event->requestor;
  Atom            xaMultiple = XInternAtom(g_display, "MULTIPLE", False);
  PCACHEENTRY     pCacheEntry = NULL;
  void            *pData = NULL;
  Pixmap          pixmap;

  /* If the specified property is None the requestor is an obsolete client.
   * We support these by using the specified target atom as the reply property.
   */
  rprop = event->property;
  if( rprop == None ) 
      rprop = event->target;

  TRACE("Request for %s in selection %s\n",
        XGetAtomName(g_display, event->target), XGetAtomName(g_display, event->selection));

  /* Handle MULTIPLE requests -  rprop contains a list of (target, property) atom pairs */
  if(event->target == xaMultiple)
  {
      /* MULTIPLE selection request - will call us back recursively */
      rprop = EVENT_SelectionRequest_MULTIPLE( event );
      goto END;
  }

  /* Lookup the requested target property in the cache */
  if ( !LookupCacheItem(event->selection, event->target, &pCacheEntry) )
  {
      TRACE("Item not available in cache!\n");
      goto END;
  }

  /* Update the X property */
  TRACE("\tUpdating property %s...\n", XGetAtomName(g_display, rprop));

  /* If we have a request for a pixmap, return a duplicate */
  
  if(event->target == XA_PIXMAP || event->target == XA_BITMAP)
  {
    Pixmap *pPixmap = (Pixmap *)pCacheEntry->pData;
    pixmap = DuplicatePixmap( *pPixmap );
    pData = &pixmap;
  }
  else
    pData = pCacheEntry->pData;
  
  XChangeProperty(g_display, request, rprop,
                    pCacheEntry->type, pCacheEntry->nFormat, PropModeReplace,
                    (unsigned char *)pData, pCacheEntry->nElements);

END:
  if( rprop == None) 
      TRACE("\tRequest ignored\n");

  /* reply to sender 
   * SelectionNotify should be sent only at the end of a MULTIPLE request
   */
  if ( !bIsMultiple )
  {
    result.type = SelectionNotify;
    result.display = g_display;
    result.requestor = request;
    result.selection = event->selection;
    result.property = rprop;
    result.target = event->target;
    result.time = event->time;
    TRACE("Sending SelectionNotify event...\n");
    XSendEvent(g_display,event->requestor,False,NoEventMask,(XEvent*)&result);
  }
}


/***********************************************************************
 *           EVENT_SelectionClear
 *   We receive this event when another client grabs the X selection.
 *   If we lost both PRIMARY and CLIPBOARD we must terminate.
 */
void EVENT_SelectionClear( XSelectionClearEvent *event )
{
  Atom xaClipboard = XInternAtom(g_display, _CLIPBOARD, False);
    
  TRACE("()\n");

  /* If we're losing the CLIPBOARD selection, or if the preferences in .winerc
   * dictate that *all* selections should be cleared on loss of a selection,
   * we must give up all the selections we own.
   */
  if ( g_clearAllSelections || (event->selection == xaClipboard) )
  {
      TRACE("Lost CLIPBOARD (+PRIMARY) selection\n");
      
      /* We really lost CLIPBOARD but want to voluntarily lose PRIMARY */
      if ( (event->selection == xaClipboard)
           && (g_selectionAcquired & S_PRIMARY) )
      {
          XSetSelectionOwner(g_display, XA_PRIMARY, None, CurrentTime);
      }
      
      /* We really lost PRIMARY but want to voluntarily lose CLIPBOARD  */
      if ( (event->selection == XA_PRIMARY)
           && (g_selectionAcquired & S_CLIPBOARD) )
      {
          XSetSelectionOwner(g_display, xaClipboard, None, CurrentTime);
      }
      
      g_selectionAcquired = S_NOSELECTION;   /* Clear the selection masks */
  }
  else if (event->selection == XA_PRIMARY)
  {
      TRACE("Lost PRIMARY selection...\n");
      g_selectionAcquired &= ~S_PRIMARY;     /* Clear the PRIMARY flag */
  }

  /* Once we lose all our selections we have nothing more to do */
  if (g_selectionAcquired == S_NOSELECTION)
      TerminateServer(1);
}

/***********************************************************************
 *           EVENT_PropertyNotify
 *   We use this to release resources like Pixmaps when a selection
 *   client no longer needs them.
 */
void EVENT_PropertyNotify( XPropertyEvent *event )
{
  TRACE("()\n");

  /* Check if we have any resources to free */

  switch(event->state)
  {
    case PropertyDelete:
    {
      TRACE("\tPropertyDelete for atom %s on window %ld\n",
                    XGetAtomName(event->display, event->atom), (long)event->window);
      
      /* FreeResources( event->atom ); */
      break;
    }

    case PropertyNewValue:
    {
      TRACE("\tPropertyNewValue for atom %s on window %ld\n\n",
                    XGetAtomName(event->display, event->atom), (long)event->window);
      break;
    }
    
    default:
      break;
  }
}

/***********************************************************************
 *           DuplicatePixmap
 */
Pixmap DuplicatePixmap(Pixmap pixmap)
{
    Pixmap newPixmap;
    XImage *xi;
    Window root;
    int x,y;               /* Unused */
    unsigned border_width; /* Unused */
    unsigned int depth, width, height;

    TRACE("\t() Pixmap=%ld\n", (long)pixmap);
          
    /* Get the Pixmap dimensions and bit depth */
    if ( 0 == XGetGeometry(g_display, pixmap, &root, &x, &y, &width, &height,
                             &border_width, &depth) )
        return 0;

    TRACE("\tPixmap properties: width=%d, height=%d, depth=%d\n",
          width, height, depth);
    
    newPixmap = XCreatePixmap(g_display, g_win, width, height, depth);
        
    xi = XGetImage(g_display, pixmap, 0, 0, width, height, AllPlanes, XYPixmap);

    XPutImage(g_display, newPixmap, g_gc, xi, 0, 0, 0, 0, width, height);

    XDestroyImage(xi);
    
    TRACE("\t() New Pixmap=%ld\n", (long)newPixmap);
    return newPixmap;
}

/***********************************************************************
 *           getGC
 * Get a GC to use for drawing
 */
void getGC(Window win, GC *gc)
{
	unsigned long valuemask = 0; /* ignore XGCvalues and use defaults */
	XGCValues values;
	unsigned int line_width = 6;
	int line_style = LineOnOffDash;
	int cap_style = CapRound;
	int join_style = JoinRound;
	int dash_offset = 0;
	static char dash_list[] = {12, 24};
	int list_length = 2;

	/* Create default Graphics Context */
	*gc = XCreateGC(g_display, win, valuemask, &values);

	/* specify black foreground since default window background is 
	 * white and default foreground is undefined. */
	XSetForeground(g_display, *gc, BlackPixel(g_display,screen_num));

	/* set line attributes */
	XSetLineAttributes(g_display, *gc, line_width, line_style, 
			cap_style, join_style);

	/* set dashes */
	XSetDashes(g_display, *gc, dash_offset, dash_list, list_length);
}


/***********************************************************************
 *           TextOut
 */
void TextOut(Window win, GC gc, char *pStr)
{
	int y_offset, x_offset;

	y_offset = 10;
	x_offset = 2;

	/* output text, centered on each line */
	XDrawString(g_display, win, gc, x_offset, y_offset, pStr, 
			strlen(pStr));
}
