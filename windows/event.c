/*
 * X events handling functions
 * 
 * Copyright 1993 Alexandre Julliard
 * 
 */

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <X11/keysym.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"
#include <X11/Xatom.h>

#include "windows.h"
#include "winnt.h"
#include "gdi.h"
#include "heap.h"
#include "queue.h"
#include "win.h"
#include "class.h"
#include "clipboard.h"
#include "dce.h"
#include "message.h"
#include "module.h"
#include "options.h"
#include "queue.h"
#include "winpos.h"
#include "drive.h"
#include "shell.h"
#include "keyboard.h"
#include "mouse.h"
#include "debug.h"
#include "dde_proc.h"
#include "winsock.h"
#include "mouse.h"


#define NB_BUTTONS      3     /* Windows can handle 3 buttons */

#define DndNotDnd       -1    /* OffiX drag&drop */
#define DndUnknown      0
#define DndRawData      1
#define DndFile         2
#define DndFiles        3
#define DndText         4
#define DndDir          5
#define DndLink         6
#define DndExe          7

#define DndEND          8

#define DndURL          128   /* KDE drag&drop */


  /* X context to associate a hwnd to an X window */
static XContext winContext = 0;

static Atom wmProtocols = None;
static Atom wmDeleteWindow = None;
static Atom dndProtocol = None;
static Atom dndSelection = None;

/* EVENT_WaitNetEvent() master fd sets */

static fd_set __event_io_set[3];
static int    __event_max_fd = 0;
static int    __event_x_connection = 0;

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

  /* Event handlers */
static void EVENT_Key( WND *pWnd, XKeyEvent *event );
static void EVENT_ButtonPress( WND *pWnd, XButtonEvent *event );
static void EVENT_ButtonRelease( WND *pWnd, XButtonEvent *event );
static void EVENT_MotionNotify( WND *pWnd, XMotionEvent *event );
static void EVENT_FocusIn( WND *pWnd, XFocusChangeEvent *event );
static void EVENT_FocusOut( WND *pWnd, XFocusChangeEvent *event );
static int  EVENT_Expose( WND *pWnd, XExposeEvent *event );
static void EVENT_GraphicsExpose( WND *pWnd, XGraphicsExposeEvent *event );
static void EVENT_ConfigureNotify( WND *pWnd, XConfigureEvent *event );
static void EVENT_SelectionRequest( WND *pWnd, XSelectionRequestEvent *event);
static void EVENT_SelectionNotify( XSelectionEvent *event);
static void EVENT_SelectionClear( WND *pWnd, XSelectionClearEvent *event);
static void EVENT_ClientMessage( WND *pWnd, XClientMessageEvent *event );
static void EVENT_MapNotify( HWND32 hwnd, XMapEvent *event );

/* Usable only with OLVWM - compile option perhaps?
static void EVENT_EnterNotify( WND *pWnd, XCrossingEvent *event );
*/

static void EVENT_GetGeometry( Window win, int *px, int *py,
                               unsigned int *pwidth, unsigned int *pheight );

/***********************************************************************
 *           EVENT_Init
 *
 * Initialize network IO.
 */
BOOL32 EVENT_Init(void)
{
    int  i;
    for( i = 0; i < 3; i++ )
	FD_ZERO( __event_io_set + i );

    __event_max_fd = __event_x_connection = ConnectionNumber(display);
    FD_SET( __event_x_connection, &__event_io_set[EVENT_IO_READ] );
    __event_max_fd++;
    return TRUE;
}

/***********************************************************************
 *          EVENT_AddIO 
 */
void EVENT_AddIO( int fd, unsigned io_type )
{
    FD_SET( fd, &__event_io_set[io_type] );
    if( __event_max_fd <= fd ) __event_max_fd = fd + 1;
}

void EVENT_DeleteIO( int fd, unsigned io_type )
{
    FD_CLR( fd, &__event_io_set[io_type] );
}

/***********************************************************************
 *           EVENT_ProcessEvent
 *
 * Process an X event.
 */
void EVENT_ProcessEvent( XEvent *event )
{
    WND *pWnd;
    
    if ( TSXFindContext( display, event->xany.window, winContext,
			 (char **)&pWnd ) != 0) {
      if ( event->type == ClientMessage) {
	/* query window (drag&drop event contains only drag window) */
	Window   	root, child;
	int      	root_x, root_y, child_x, child_y;
	unsigned	u;
	TSXQueryPointer( display, rootWindow, &root, &child,
			 &root_x, &root_y, &child_x, &child_y, &u);
	if (TSXFindContext( display, child, winContext, (char **)&pWnd ) != 0)
	  return;
      } else {
	pWnd = NULL;  /* Not for a registered window */
      }
    }

    TRACE(event, "Got event %s for hwnd %04x\n",
	  event_names[event->type], pWnd? pWnd->hwndSelf : 0 );

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
        EVENT_Key( pWnd, (XKeyEvent*)event );
	break;
	
    case ButtonPress:
        EVENT_ButtonPress( pWnd, (XButtonEvent*)event );
	break;

    case ButtonRelease:
        EVENT_ButtonRelease( pWnd, (XButtonEvent*)event );
	break;

    case MotionNotify:
        /* Wine between two fast machines across the overloaded campus
	   ethernet gets very boged down in MotionEvents. The following
	   simply finds the last motion event in the queue and drops
	   the rest. On a good link events are servered before they build
	   up so this doesn't take place. On a slow link this may cause
	   problems if the event order is important. I'm not yet seen
	   of any problems. Jon 7/6/96.
	 */
        while (TSXCheckTypedWindowEvent(display,((XAnyEvent *)event)->window,
                                        MotionNotify, event));    
        EVENT_MotionNotify( pWnd, (XMotionEvent*)event );
	break;

    case FocusIn:
        if (!pWnd) return;
        EVENT_FocusIn( pWnd, (XFocusChangeEvent*)event );
	break;

    case FocusOut:
        if (!pWnd) return;
	EVENT_FocusOut( pWnd, (XFocusChangeEvent*)event );
	break;

    case Expose:
        if (!pWnd) return;
	if (EVENT_Expose( pWnd, (XExposeEvent *)event )) {
	    /* need to process ConfigureNotify first */
	    XEvent new_event;

	    /* attempt to find and process the ConfigureNotify event now */
            if (TSXCheckTypedWindowEvent(display,((XAnyEvent *)event)->window,
                                          ConfigureNotify, &new_event)) {
                EVENT_ProcessEvent( &new_event );
                EVENT_Expose( pWnd, (XExposeEvent *)event );
                break;
            }

	    /* no luck at this time, defer Expose event for later */
	    if (!pWnd->expose_event) pWnd->expose_event = malloc( sizeof(XExposeEvent) );
	    else { FIXME(x11,"can't handle more than one deferred Expose events\n"); }
	    *(pWnd->expose_event) = *(XExposeEvent *)event;
	}
	break;

    case GraphicsExpose:
        if (!pWnd) return;
	EVENT_GraphicsExpose( pWnd, (XGraphicsExposeEvent *)event );
        break;

    case ConfigureNotify:
        if (!pWnd) return;
	EVENT_ConfigureNotify( pWnd, (XConfigureEvent*)event );
	if (pWnd->expose_event) {
	    /* process deferred Expose event */
	    EVENT_Expose( pWnd, pWnd->expose_event );
	    free( pWnd->expose_event );
	    pWnd->expose_event = NULL;
	}
	break;

    case SelectionRequest:
        if (!pWnd) return;
	EVENT_SelectionRequest( pWnd, (XSelectionRequestEvent *)event );
	break;

    case SelectionNotify:
        if (!pWnd) return;
	EVENT_SelectionNotify( (XSelectionEvent *)event );
	break;

    case SelectionClear:
        if (!pWnd) return;
	EVENT_SelectionClear( pWnd, (XSelectionClearEvent*) event );
	break;

    case ClientMessage:
        if (!pWnd) return;
	EVENT_ClientMessage( pWnd, (XClientMessageEvent *) event );
	break;

/*  case EnterNotify:
 *       EVENT_EnterNotify( pWnd, (XCrossingEvent *) event );
 *       break;
 */
    case NoExpose:
	break;

    /* We get all these because of StructureNotifyMask. */
    case UnmapNotify:
    case CirculateNotify:
    case CreateNotify:
    case DestroyNotify:
    case GravityNotify:
    case ReparentNotify:
	break;

    case MapNotify:
        if (!pWnd) return;
	EVENT_MapNotify( pWnd->hwndSelf, (XMapEvent *)event );
	break;

    default:    
	WARN(event, "Unprocessed event %s for hwnd %04x\n",
	        event_names[event->type], pWnd? pWnd->hwndSelf : 0 );
	break;
    }
}


/***********************************************************************
 *           EVENT_RegisterWindow
 *
 * Associate an X window to a HWND.
 */
void EVENT_RegisterWindow( WND *pWnd )
{
    if (wmProtocols == None)
        wmProtocols = TSXInternAtom( display, "WM_PROTOCOLS", True );
    if (wmDeleteWindow == None)
        wmDeleteWindow = TSXInternAtom( display, "WM_DELETE_WINDOW", True );
    if( dndProtocol == None )
	dndProtocol = TSXInternAtom( display, "DndProtocol" , False );
    if( dndSelection == None )
	dndSelection = TSXInternAtom( display, "DndSelection" , False );

    TSXSetWMProtocols( display, pWnd->window, &wmDeleteWindow, 1 );

    if (!winContext) winContext = TSXUniqueContext();
    TSXSaveContext( display, pWnd->window, winContext, (char *)pWnd );
}

/***********************************************************************
 *           EVENT_DestroyWindow
 */
void EVENT_DestroyWindow( WND *pWnd )
{
   XEvent xe;

   if (pWnd->expose_event) {
       free( pWnd->expose_event );
       pWnd->expose_event = NULL;
   }
   TSXDeleteContext( display, pWnd->window, winContext );
   TSXDestroyWindow( display, pWnd->window );
   while( TSXCheckWindowEvent(display, pWnd->window, NoEventMask, &xe) );
}


/***********************************************************************
 *           IsUserIdle		(USER.333)
 *
 * Check if we have pending X events.
 */
BOOL16 WINAPI IsUserIdle(void)
{
    struct timeval timeout = {0, 0};
    fd_set check_set;

    FD_ZERO(&check_set);
    FD_SET(__event_x_connection, &check_set);
    if( select(__event_x_connection + 1, &check_set, NULL, NULL, &timeout) > 0 )
	return TRUE;
    return FALSE;
}


/***********************************************************************
 *           EVENT_WaitNetEvent
 *
 * Wait for a network event, optionally sleeping until one arrives.
 * Return TRUE if an event is pending, FALSE on timeout or error
 * (for instance lost connection with the server).
 */
BOOL32 EVENT_WaitNetEvent( BOOL32 sleep, BOOL32 peek )
{
    XEvent event;
    LONG maxWait = sleep ? TIMER_GetNextExpiration() : 0;
    int pending = TSXPending(display);

    /* Wait for an event or a timeout. If maxWait is -1, we have no timeout;
     * in this case, we fall through directly to the XNextEvent loop.
     */

    if ((maxWait != -1) && !pending)
    {
	int num_pending;
        struct timeval timeout;
	fd_set io_set[3];

	memcpy( io_set, __event_io_set, sizeof(io_set) );

	timeout.tv_usec = (maxWait % 1000) * 1000;
	timeout.tv_sec = maxWait / 1000;

#ifdef CONFIG_IPC
	sigsetjmp(env_wait_x, 1);
	stop_wait_op= CONT;
	    
	if (DDE_GetRemoteMessage()) {
	    while(DDE_GetRemoteMessage())
		;
	    return TRUE;
	}
	stop_wait_op = STOP_WAIT_X;
	/* The code up to the next "stop_wait_op = CONT" must be reentrant */
	num_pending = select( __event_max_fd, &io_set[EVENT_IO_READ], 
					      &io_set[EVENT_IO_WRITE], 
					      &io_set[EVENT_IO_EXCEPT], &timeout );
	if ( num_pending == 0 )
        {
	    stop_wait_op = CONT;
            TIMER_ExpireTimers();
	    return FALSE;
	}
        else stop_wait_op = CONT;
#else  /* CONFIG_IPC */
	num_pending = select( __event_max_fd, &io_set[EVENT_IO_READ],
					      &io_set[EVENT_IO_WRITE],
					      &io_set[EVENT_IO_EXCEPT], &timeout );
	if ( num_pending == 0)
        {
            /* Timeout or error */
            TIMER_ExpireTimers();
            return FALSE;
        }
#endif  /* CONFIG_IPC */

	/*  Winsock asynchronous services */

	if( FD_ISSET( __event_x_connection, &io_set[EVENT_IO_READ]) ) 
	{
	    num_pending--;
	    if( num_pending )
		WINSOCK_HandleIO( &__event_max_fd, num_pending, io_set, __event_io_set );
	}
	else /* no X events */
	    return WINSOCK_HandleIO( &__event_max_fd, num_pending, io_set, __event_io_set );
    }
    else if(!pending)
    {				/* Wait for X11 input. */
	fd_set set;

	FD_ZERO(&set);
	FD_SET(__event_x_connection, &set);
	select(__event_x_connection + 1, &set, 0, 0, 0 );
    }

    /* Process current X event (and possibly others that occurred in the meantime) */

    EnterCriticalSection(&X11DRV_CritSection);
    while (XPending( display ))
    {

#ifdef CONFIG_IPC
        if (DDE_GetRemoteMessage())
        {
            LeaveCriticalSection(&X11DRV_CritSection);
            while(DDE_GetRemoteMessage()) ;
            return TRUE;
        }
#endif  /* CONFIG_IPC */

	XNextEvent( display, &event );

        LeaveCriticalSection(&X11DRV_CritSection);
        if( peek )
        {
	  WND*		pWnd;
	  MESSAGEQUEUE* pQ;


	  /* Check only for those events which can be processed
	   * internally. */

	  if( event.type == MotionNotify ||
	      event.type == ButtonPress || event.type == ButtonRelease ||
	      event.type == KeyPress || event.type == KeyRelease ||
	      event.type == SelectionRequest || event.type == SelectionClear ||
	      event.type == ClientMessage )
	  {
	       EVENT_ProcessEvent( &event );
               continue;
	  }

          if (TSXFindContext( display, ((XAnyEvent *)&event)->window, winContext,
                            (char **)&pWnd ) || (event.type == NoExpose))
              pWnd = NULL;

	  if( pWnd )
          {
            if( (pQ = (MESSAGEQUEUE*)GlobalLock16(pWnd->hmemTaskQ)) )
            {
              pQ->flags |= QUEUE_FLAG_XEVENT;
              PostEvent(pQ->hTask);
	      TSXPutBackEvent(display, &event);
              break;
	    }
          }
        }
        else EVENT_ProcessEvent( &event );
        EnterCriticalSection(&X11DRV_CritSection);
    }
    LeaveCriticalSection(&X11DRV_CritSection);
    return TRUE;
}


/***********************************************************************
 *           EVENT_Synchronize
 *
 * Synchronize with the X server. Should not be used too often.
 */
void EVENT_Synchronize()
{
    XEvent event;

    /* Use of the X critical section is needed or we have a small
     * race between XPending() and XNextEvent().
     */
    EnterCriticalSection( &X11DRV_CritSection );
    XSync( display, False );
    while (XPending( display ))
    {
	XNextEvent( display, &event );
	/* unlock X critsection for EVENT_ProcessEvent() might switch tasks */
	LeaveCriticalSection( &X11DRV_CritSection );
	EVENT_ProcessEvent( &event );
	EnterCriticalSection( &X11DRV_CritSection );
    }    
    LeaveCriticalSection( &X11DRV_CritSection );
}

/***********************************************************************
 *           EVENT_QueryZOrder
 *
 * Try to synchronize internal z-order with the window manager's.
 * Probably a futile endeavor.
 */
static BOOL32 __check_query_condition( WND** pWndA, WND** pWndB )
{
    /* return TRUE if we have at least two managed windows */

    for( *pWndB = NULL; *pWndA; *pWndA = (*pWndA)->next )
        if( (*pWndA)->flags & WIN_MANAGED &&
            (*pWndA)->dwStyle & WS_VISIBLE ) break;
    if( *pWndA )
        for( *pWndB = (*pWndA)->next; *pWndB; *pWndB = (*pWndB)->next )
            if( (*pWndB)->flags & WIN_MANAGED &&
                (*pWndB)->dwStyle & WS_VISIBLE ) break;
     return ((*pWndB) != NULL);
}

static Window __get_common_ancestor( Window A, Window B,
                                     Window** children, unsigned* total )
{
    /* find the real root window */

    Window      root, *childrenB;
    unsigned    totalB;

    do
    {
        if( *children ) TSXFree( *children );
        TSXQueryTree( display, A, &root, &A, children, total );
        TSXQueryTree( display, B, &root, &B, &childrenB, &totalB );
        if( childrenB ) TSXFree( childrenB );
    } while( A != B && A && B );
    return ( A && B ) ? A : 0 ;
}

static Window __get_top_decoration( Window w, Window ancestor )
{
    Window*     children, root, prev = w, parent = w;
    unsigned    total;

    do
    {
        w = parent;
        TSXQueryTree( display, w, &root, &parent, &children, &total );
        if( children ) TSXFree( children );
    } while( parent && parent != ancestor );
    TRACE(event, "\t%08x -> %08x\n", (unsigned)prev, (unsigned)w );
    return ( parent ) ? w : 0 ;
}

static unsigned __td_lookup( Window w, Window* list, unsigned max )
{
    unsigned    i;
    for( i = 0; i < max; i++ ) if( list[i] == w ) break;
    return i;
}

static BOOL32 EVENT_QueryZOrder( WND* pWndCheck )
{
    BOOL32      bRet = FALSE;
    HWND32      hwndInsertAfter = HWND_TOP;
    WND*        pWnd, *pWndZ = WIN_GetDesktop()->child;
    Window      w, parent, *children = NULL;
    unsigned    total, check, pos, best;

    if( !__check_query_condition(&pWndZ, &pWnd) ) return TRUE;

    parent = __get_common_ancestor( pWndZ->window, pWnd->window,
                                      &children, &total );
    if( parent && children )
    {
        w = __get_top_decoration( pWndCheck->window, parent );
        if( w != children[total - 1] )
        {
            check = __td_lookup( w, children, total );
            best = total;
            for( pWnd = pWndZ; pWnd; pWnd = pWnd->next )
            {
                if( pWnd != pWndCheck )
                {
                    if( !(pWnd->flags & WIN_MANAGED) ||
                        !(w = __get_top_decoration( pWnd->window, parent )) )
                        continue;
                    pos = __td_lookup( w, children, total );
                    if( pos < best && pos > check )
                    {
                        best = pos;
                        hwndInsertAfter = pWnd->hwndSelf;
                    }
                    if( check - best == 1 ) break;
                }
            }
            WIN_UnlinkWindow( pWndCheck->hwndSelf );
            WIN_LinkWindow( pWndCheck->hwndSelf, hwndInsertAfter);
        }
    }
    if( children ) TSXFree( children );
    return bRet;
}


/***********************************************************************
 *           EVENT_XStateToKeyState
 *
 * Translate a X event state (Button1Mask, ShiftMask, etc...) to
 * a Windows key state (MK_SHIFT, MK_CONTROL, etc...)
 */
static WORD EVENT_XStateToKeyState( int state )
{
    int kstate = 0;
    
    if (state & Button1Mask) kstate |= MK_LBUTTON;
    if (state & Button2Mask) kstate |= MK_MBUTTON;
    if (state & Button3Mask) kstate |= MK_RBUTTON;
    if (state & ShiftMask)   kstate |= MK_SHIFT;
    if (state & ControlMask) kstate |= MK_CONTROL;
    return kstate;
}

/***********************************************************************
 *           EVENT_QueryPointer
 */
BOOL32 EVENT_QueryPointer( DWORD *posX, DWORD *posY, DWORD *state )
{
    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int xstate;

    if (!TSXQueryPointer( display, rootWindow, &root, &child,
                          &rootX, &rootY, &winX, &winY, &xstate )) 
        return FALSE;

    *posX  = (DWORD)winX;
    *posY  = (DWORD)winY;
    *state = EVENT_XStateToKeyState( xstate );

    return TRUE;
}

/***********************************************************************
 *           EVENT_Expose
 */
static int EVENT_Expose( WND *pWnd, XExposeEvent *event )
{
    RECT32 rect;
    int x, y;
    unsigned int width, height;

    /* When scrolling, many (fvwm2-based) window managers send the Expose
     * event before sending the ConfigureNotify event, and we don't like
     * that, so before processing the Expose event, we check whether the
     * geometry has changed, and if so, we defer the Expose event until
     * we get the ConfigureNotify event.  -Ove Kåven */
    EVENT_GetGeometry( event->window, &x, &y, &width, &height );

    if ( x != pWnd->rectWindow.left || y != pWnd->rectWindow.top ||
         (width != pWnd->rectWindow.right - pWnd->rectWindow.left) ||
         (height != pWnd->rectWindow.bottom - pWnd->rectWindow.top))
        return 1; /* tell EVENT_ProcessEvent() to defer it */

    /* Make position relative to client area instead of window */
    rect.left   = event->x - (pWnd->rectClient.left - pWnd->rectWindow.left);
    rect.top    = event->y - (pWnd->rectClient.top - pWnd->rectWindow.top);
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    PAINT_RedrawWindow( pWnd->hwndSelf, &rect, 0,
                        RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE |
                        (event->count ? 0 : RDW_ERASENOW), 0 );
    return 0;
}


/***********************************************************************
 *           EVENT_GraphicsExpose
 *
 * This is needed when scrolling area is partially obscured
 * by non-Wine X window.
 */
static void EVENT_GraphicsExpose( WND *pWnd, XGraphicsExposeEvent *event )
{
    RECT32 rect;

    /* Make position relative to client area instead of window */
    rect.left   = event->x - (pWnd->rectClient.left - pWnd->rectWindow.left);
    rect.top    = event->y - (pWnd->rectClient.top - pWnd->rectWindow.top);
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    PAINT_RedrawWindow( pWnd->hwndSelf, &rect, 0,
                        RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_ERASE |
                        (event->count ? 0 : RDW_ERASENOW), 0 );
}


/***********************************************************************
 *           EVENT_Key
 *
 * Handle a X key event
 */
static void EVENT_Key( WND *pWnd, XKeyEvent *event )
{
    KEYBOARD_HandleEvent( pWnd, event );
}


/***********************************************************************
 *           EVENT_MotionNotify
 */
static void EVENT_MotionNotify( WND *pWnd, XMotionEvent *event )
{
    int xOffset = pWnd? pWnd->rectWindow.left : 0;
    int yOffset = pWnd? pWnd->rectWindow.top  : 0;

    MOUSE_SendEvent( MOUSEEVENTF_MOVE, 
                     xOffset + event->x, yOffset + event->y,
                     EVENT_XStateToKeyState( event->state ), 
                     event->time - MSG_WineStartTicks,
                     pWnd? pWnd->hwndSelf : 0 );
}


/***********************************************************************
 *           EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void EVENT_DummyMotionNotify(void)
{
    DWORD winX, winY, state;

    if ( EVENT_QueryPointer( &winX, &winY, &state ) )
    {
        MOUSE_SendEvent( MOUSEEVENTF_MOVE, winX, winY, state,
                         GetTickCount(), 0 );
    }
}


/***********************************************************************
 *           EVENT_ButtonPress
 */
static void EVENT_ButtonPress( WND *pWnd, XButtonEvent *event )
{
    static WORD statusCodes[NB_BUTTONS] = 
        { MOUSEEVENTF_LEFTDOWN, MOUSEEVENTF_MIDDLEDOWN, MOUSEEVENTF_RIGHTDOWN };
    int buttonNum = event->button - 1;

    int xOffset = pWnd? pWnd->rectWindow.left : 0;
    int yOffset = pWnd? pWnd->rectWindow.top  : 0;

    if (buttonNum >= NB_BUTTONS) return;

    MOUSE_SendEvent( statusCodes[buttonNum], 
                     xOffset + event->x, yOffset + event->y,
                     EVENT_XStateToKeyState( event->state ), 
                     event->time - MSG_WineStartTicks,
                     pWnd? pWnd->hwndSelf : 0 );
}


/***********************************************************************
 *           EVENT_ButtonRelease
 */
static void EVENT_ButtonRelease( WND *pWnd, XButtonEvent *event )
{
    static WORD statusCodes[NB_BUTTONS] = 
        { MOUSEEVENTF_LEFTUP, MOUSEEVENTF_MIDDLEUP, MOUSEEVENTF_RIGHTUP };
    int buttonNum = event->button - 1;

    int xOffset = pWnd? pWnd->rectWindow.left : 0;
    int yOffset = pWnd? pWnd->rectWindow.top  : 0;

    if (buttonNum >= NB_BUTTONS) return;    

    MOUSE_SendEvent( statusCodes[buttonNum], 
                     xOffset + event->x, yOffset + event->y,
                     EVENT_XStateToKeyState( event->state ), 
                     event->time - MSG_WineStartTicks,
                     pWnd? pWnd->hwndSelf : 0 );
}


/**********************************************************************
 *              EVENT_FocusIn
 */
static void EVENT_FocusIn( WND *pWnd, XFocusChangeEvent *event )
{
    if (Options.managed) EVENT_QueryZOrder( pWnd );

    if (event->detail != NotifyPointer)
    { 
	HWND32	hwnd = pWnd->hwndSelf;

	if (hwnd != GetActiveWindow32()) 
        {
	    WINPOS_ChangeActiveWindow( hwnd, FALSE );
	    KEYBOARD_UpdateState();
        }
	if ((hwnd != GetFocus32()) && !IsChild32( hwnd, GetFocus32()))
            SetFocus32( hwnd );
    }
}


/**********************************************************************
 *              EVENT_FocusOut
 *
 * Note: only top-level override-redirect windows get FocusOut events.
 */
static void EVENT_FocusOut( WND *pWnd, XFocusChangeEvent *event )
{
    if (event->detail != NotifyPointer)
    {
	HWND32	hwnd = pWnd->hwndSelf;

	if (hwnd == GetActiveWindow32()) 
	    WINPOS_ChangeActiveWindow( 0, FALSE );
	if ((hwnd == GetFocus32()) || IsChild32( hwnd, GetFocus32()))
	    SetFocus32( 0 );
    }
}

/**********************************************************************
 *              EVENT_CheckFocus
 */
BOOL32 EVENT_CheckFocus(void)
{
    WND*   pWnd;
    Window xW;
    int	   state;

    TSXGetInputFocus(display, &xW, &state);
    if( xW == None ||
        TSXFindContext(display, xW, winContext, (char **)&pWnd) ) 
        return FALSE;
    return TRUE;
}


/**********************************************************************
 *              EVENT_GetGeometry
 *
 * Helper function for ConfigureNotify handling.
 * Get the new geometry of a window relative to the root window.
 */
static void EVENT_GetGeometry( Window win, int *px, int *py,
                               unsigned int *pwidth, unsigned int *pheight )
{
    Window root, parent, *children;
    int xpos, ypos;
    unsigned int width, height, border, depth, nb_children;

    if (!TSXGetGeometry( display, win, &root, px, py, pwidth, pheight,
                       &border, &depth )) return;
    if (win == rootWindow)
    {
        *px = *py = 0;
        return;
    }

    for (;;)
    {
        if (!TSXQueryTree(display, win, &root, &parent, &children, &nb_children))
            return;
        TSXFree( children );
        if (parent == rootWindow) break;
        win = parent;
        if (!TSXGetGeometry( display, win, &root, &xpos, &ypos,
                           &width, &height, &border, &depth )) return;
        *px += xpos;
        *py += ypos;
    }
}


/**********************************************************************
 *              EVENT_ConfigureNotify
 *
 * The ConfigureNotify event is only selected on top-level windows
 * when the -managed flag is used.
 */
static void EVENT_ConfigureNotify( WND *pWnd, XConfigureEvent *event )
{
    WINDOWPOS32 winpos;
    RECT32 newWindowRect, newClientRect;
    HRGN32 hrgnOldPos, hrgnNewPos;
    Window above = event->above;
    int x, y;
    unsigned int width, height;

    assert (pWnd->flags & WIN_MANAGED);

    /* We don't rely on the event geometry info, because it is relative
     * to parent and not to root, and it may be wrong (XFree sets x,y to 0,0
     * if the window hasn't moved).
     */
    EVENT_GetGeometry( event->window, &x, &y, &width, &height );

    /* Fill WINDOWPOS struct */
    winpos.flags = SWP_NOACTIVATE | SWP_NOZORDER;
    winpos.hwnd = pWnd->hwndSelf;
    winpos.x = x;
    winpos.y = y;
    winpos.cx = width;
    winpos.cy = height;

    /* Check for unchanged attributes */
    if (winpos.x == pWnd->rectWindow.left && winpos.y == pWnd->rectWindow.top)
        winpos.flags |= SWP_NOMOVE;
    if ((winpos.cx == pWnd->rectWindow.right - pWnd->rectWindow.left) &&
        (winpos.cy == pWnd->rectWindow.bottom - pWnd->rectWindow.top))
        winpos.flags |= SWP_NOSIZE;
    else
    {
        RECT32 rect = { 0, 0, pWnd->rectWindow.right - pWnd->rectWindow.left,
                        pWnd->rectWindow.bottom - pWnd->rectWindow.top };
        DCE_InvalidateDCE( pWnd, &rect );
    }

    /* Send WM_WINDOWPOSCHANGING */
    SendMessage32A( winpos.hwnd, WM_WINDOWPOSCHANGING, 0, (LPARAM)&winpos );

    /* Calculate new position and size */
    newWindowRect.left = x;
    newWindowRect.right = x + width;
    newWindowRect.top = y;
    newWindowRect.bottom = y + height;

    WINPOS_SendNCCalcSize( winpos.hwnd, TRUE, &newWindowRect,
                           &pWnd->rectWindow, &pWnd->rectClient,
                           &winpos, &newClientRect );

    hrgnOldPos = CreateRectRgnIndirect32( &pWnd->rectWindow );
    hrgnNewPos = CreateRectRgnIndirect32( &newWindowRect );
    CombineRgn32( hrgnOldPos, hrgnOldPos, hrgnNewPos, RGN_DIFF );
    DeleteObject32(hrgnOldPos);
    DeleteObject32(hrgnNewPos);
 
    /* Set new size and position */
    pWnd->rectWindow = newWindowRect;
    pWnd->rectClient = newClientRect;
    SendMessage32A( winpos.hwnd, WM_WINDOWPOSCHANGED, 0, (LPARAM)&winpos );

    if (!IsWindow32( winpos.hwnd )) return;
    if( above == None )			/* absolute bottom */
    {
        WIN_UnlinkWindow( winpos.hwnd );
        WIN_LinkWindow( winpos.hwnd, HWND_BOTTOM);
    }
    else EVENT_QueryZOrder( pWnd );	/* try to outsmart window manager */
}


/***********************************************************************
 *           EVENT_SelectionRequest
 */
static void EVENT_SelectionRequest( WND *pWnd, XSelectionRequestEvent *event )
{
    XSelectionEvent result;
    Atom 	    rprop = None;
    Window 	    request = event->requestor;

    if(event->target == XA_STRING)
    {
	HANDLE16 hText;
	LPSTR  text;
	int    size,i,j;

        rprop = event->property;

	if(rprop == None) rprop = event->target;

        if(event->selection!=XA_PRIMARY) rprop = None;
        else if(!CLIPBOARD_IsPresent(CF_OEMTEXT)) rprop = None;
	else
	{
            /* open to make sure that clipboard is available */

	    BOOL32 couldOpen = OpenClipboard32( pWnd->hwndSelf );
	    char* lpstr = 0;

	    hText = GetClipboardData16(CF_TEXT);
	    text = GlobalLock16(hText);
	    size = GlobalSize16(hText);

	    /* remove carriage returns */

	    lpstr = (char*)HEAP_xalloc( GetProcessHeap(), 0, size-- );
	    for(i=0,j=0; i < size && text[i]; i++ )
	    {
	       if( text[i] == '\r' && 
		  (text[i+1] == '\n' || text[i+1] == '\0') ) continue;
	       lpstr[j++] = text[i];
	    }
	    lpstr[j]='\0';

	    TSXChangeProperty(display, request, rprop, 
			    XA_STRING, 8, PropModeReplace, 
			    lpstr, j);
	    HeapFree( GetProcessHeap(), 0, lpstr );

	    /* close only if we opened before */

	    if(couldOpen) CloseClipboard32();
	}
    }

    if(rprop == None) 
       TRACE(event,"Request for %s ignored\n", TSXGetAtomName(display,event->target));

    result.type = SelectionNotify;
    result.display = display;
    result.requestor = request;
    result.selection = event->selection;
    result.property = rprop;
    result.target = event->target;
    result.time = event->time;
    TSXSendEvent(display,event->requestor,False,NoEventMask,(XEvent*)&result);
}


/***********************************************************************
 *           EVENT_SelectionNotify
 */
static void EVENT_SelectionNotify( XSelectionEvent *event )
{
    if (event->selection != XA_PRIMARY) return;

    if (event->target != XA_STRING) CLIPBOARD_ReadSelection( 0, None );
    else CLIPBOARD_ReadSelection( event->requestor, event->property );

    TRACE(clipboard,"\tSelectionNotify done!\n");
}


/***********************************************************************
 *           EVENT_SelectionClear
 */
static void EVENT_SelectionClear( WND *pWnd, XSelectionClearEvent *event )
{
    if (event->selection != XA_PRIMARY) return;
    CLIPBOARD_ReleaseSelection( event->window, pWnd->hwndSelf ); 
}


/**********************************************************************
 *           EVENT_DropFromOffix
 *
 * don't know if it still works (last Changlog is from 96/11/04)
 */
static void EVENT_DropFromOffiX( WND *pWnd, XClientMessageEvent *event )
{
  unsigned long		data_length;
  unsigned long		aux_long;
  unsigned char*	p_data = NULL;
  union {
    Atom		atom_aux;
    POINT32	pt_aux;
    int		i;
  }		u;
  int			x, y;
  BOOL16	        bAccept;
  HGLOBAL16		hDragInfo = GlobalAlloc16( GMEM_SHARE | GMEM_ZEROINIT, sizeof(DRAGINFO));
  LPDRAGINFO            lpDragInfo = (LPDRAGINFO) GlobalLock16(hDragInfo);
  SEGPTR		spDragInfo = (SEGPTR) WIN16_GlobalLock16(hDragInfo);
  Window		w_aux_root, w_aux_child;
  WND*			pDropWnd;
  
  if( !lpDragInfo || !spDragInfo ) return;
  
  TSXQueryPointer( display, pWnd->window, &w_aux_root, &w_aux_child, 
		   &x, &y, &u.pt_aux.x, &u.pt_aux.y, (unsigned int*)&aux_long);
  
  lpDragInfo->hScope = pWnd->hwndSelf;
  lpDragInfo->pt.x = (INT16)x; lpDragInfo->pt.y = (INT16)y;
  
  /* find out drop point and drop window */
  if( x < 0 || y < 0 ||
      x > (pWnd->rectWindow.right - pWnd->rectWindow.left) ||
      y > (pWnd->rectWindow.bottom - pWnd->rectWindow.top) )
    {   bAccept = pWnd->dwExStyle & WS_EX_ACCEPTFILES; x = y = 0; }
  else
    {
      bAccept = DRAG_QueryUpdate( pWnd->hwndSelf, spDragInfo, TRUE );
      x = lpDragInfo->pt.x; y = lpDragInfo->pt.y;
    }
  pDropWnd = WIN_FindWndPtr( lpDragInfo->hScope );
  GlobalFree16( hDragInfo );
  
  if( bAccept )
    {
      TSXGetWindowProperty( display, DefaultRootWindow(display),
			    dndSelection, 0, 65535, FALSE,
			    AnyPropertyType, &u.atom_aux, &u.pt_aux.y,
			    &data_length, &aux_long, &p_data);
      
      if( !aux_long && p_data)	/* don't bother if > 64K */
	{
	  char *p = (char*) p_data;
	  char *p_drop;
	  
	  aux_long = 0; 
	  while( *p )	/* calculate buffer size */
	    {
	      p_drop = p;
	      if((u.i = *p) != -1 ) 
		u.i = DRIVE_FindDriveRoot( (const char **)&p_drop );
	      if( u.i == -1 ) *p = -1;	/* mark as "bad" */
	      else
		{
		  INT32 len = GetShortPathName32A( p, NULL, 0 );
		  if (len) aux_long += len + 1;
		  else *p = -1;
		}
	      p += strlen(p) + 1;
	    }
	  if( aux_long && aux_long < 65535 )
	    {
	      HDROP16                 hDrop;
	      LPDROPFILESTRUCT16        lpDrop;
	      
	      aux_long += sizeof(DROPFILESTRUCT16) + 1; 
	      hDrop = (HDROP16)GlobalAlloc16( GMEM_SHARE, aux_long );
	      lpDrop = (LPDROPFILESTRUCT16) GlobalLock16( hDrop );
	      
	      if( lpDrop )
		{
		  lpDrop->wSize = sizeof(DROPFILESTRUCT16);
		  lpDrop->ptMousePos.x = (INT16)x;
		  lpDrop->ptMousePos.y = (INT16)y;
		  lpDrop->fInNonClientArea = (BOOL16) 
		    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
		      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
		      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
		      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
		  p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT16);
		  p = p_data;
		  while(*p)
		    {
		      if( *p != -1 )	/* use only "good" entries */
			{
                          GetShortPathName32A( p, p_drop, 65535 );
                          p_drop += strlen( p_drop ) + 1;
			}
		      p += strlen(p) + 1;
		    }
		  *p_drop = '\0';
		  PostMessage16( pWnd->hwndSelf, WM_DROPFILES,
				 (WPARAM16)hDrop, 0L );
		}
	    }
	}
      if( p_data ) TSXFree(p_data);  
      
    } /* WS_EX_ACCEPTFILES */
}

/**********************************************************************
 *           EVENT_DropURLs
 *
 * drop items are separated by \n 
 * each item is prefixed by its mime type
 *
 * event->data.l[3], event->data.l[4] contains drop x,y position
 */
static void EVENT_DropURLs( WND *pWnd, XClientMessageEvent *event )
{
  WND           *pDropWnd;
  unsigned long	data_length;
  unsigned long	aux_long, drop_len = 0;
  unsigned char	*p_data = NULL; /* property data */
  char		*p_drop = NULL;
  char          *p, *next;
  int		x, y, drop32 = FALSE ;
  union {
    Atom	atom_aux;
    POINT32	pt_aux;
    int         i;
    Window      w_aux;
  }		u; /* unused */
  union {
    HDROP16     h16;
    HDROP32     h32;
  } hDrop;

  drop32 = pWnd->flags & WIN_ISWIN32;

  if (!(pWnd->dwExStyle & WS_EX_ACCEPTFILES))
    return;

  TSXGetWindowProperty( display, DefaultRootWindow(display),
			dndSelection, 0, 65535, FALSE,
			AnyPropertyType, &u.atom_aux, &u.i,
			&data_length, &aux_long, &p_data);
  if (aux_long)
    WARN(event,"property too large, truncated!\n");
  TRACE(event,"urls=%s\n", p_data);

  if( !aux_long && p_data) {	/* don't bother if > 64K */
    /* calculate length */
    p = p_data;
    next = strchr(p, '\n');
    while (p) {
      if (next) *next=0;
      if (strncmp(p,"file:",5) == 0 ) {
	INT32 len = GetShortPathName32A( p+5, NULL, 0 );
	if (len) drop_len += len + 1;
      }
      if (next) { 
	*next = '\n'; 
	p = next + 1;
	next = strchr(p, '\n');
      } else {
	p = NULL;
      }
    }
    
    if( drop_len && drop_len < 65535 ) {
      TSXQueryPointer( display, rootWindow, &u.w_aux, &u.w_aux, 
		       &x, &y, &u.i, &u.i, &u.i);
      pDropWnd = WIN_FindWndPtr( pWnd->hwndSelf );
      
      if (drop32) {
	LPDROPFILESTRUCT32        lpDrop;
	drop_len += sizeof(DROPFILESTRUCT32) + 1; 
	hDrop.h32 = (HDROP32)GlobalAlloc32( GMEM_SHARE, drop_len );
	lpDrop = (LPDROPFILESTRUCT32) GlobalLock32( hDrop.h32 );
	
	if( lpDrop ) {
	  lpDrop->lSize = sizeof(DROPFILESTRUCT32);
	  lpDrop->ptMousePos.x = (INT32)x;
	  lpDrop->ptMousePos.y = (INT32)y;
	  lpDrop->fInNonClientArea = (BOOL32) 
	    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
	      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
	      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
	      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
	  lpDrop->fWideChar = FALSE;
	  p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT32);
	}
      } else {
	LPDROPFILESTRUCT16        lpDrop;
	drop_len += sizeof(DROPFILESTRUCT16) + 1; 
	hDrop.h16 = (HDROP16)GlobalAlloc16( GMEM_SHARE, drop_len );
	lpDrop = (LPDROPFILESTRUCT16) GlobalLock16( hDrop.h16 );
	
	if( lpDrop ) {
	  lpDrop->wSize = sizeof(DROPFILESTRUCT16);
	  lpDrop->ptMousePos.x = (INT16)x;
	  lpDrop->ptMousePos.y = (INT16)y;
	  lpDrop->fInNonClientArea = (BOOL16) 
	    ( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
	      y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
	      x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
	      y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
	  p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT16);
	}
      }
      
      /* create message content */
      if (p_drop) {
	p = p_data;
	next = strchr(p, '\n');
	while (p) {
	  if (next) *next=0;
	  if (strncmp(p,"file:",5) == 0 ) {
	    INT32 len = GetShortPathName32A( p+5, p_drop, 65535 );
	    if (len) {
	      TRACE(event, "drop file %s as %s\n", p+5, p_drop);
	      p_drop += len+1;
	    } else {
	      WARN(event, "can't convert file %s to dos name \n", p+5);
	    }
	  } else {
	    WARN(event, "unknown mime type %s\n", p);
	  }
	  if (next) { 
	    *next = '\n'; 
	    p = next + 1;
	    next = strchr(p, '\n');
	  } else {
	    p = NULL;
	  }
	  *p_drop = '\0';
	}

	if (drop32) {
	  /* can not use PostMessage32A because it is currently based on 
	   * PostMessage16 and WPARAM32 would be truncated to WPARAM16
	   */
	  GlobalUnlock32(hDrop.h32);
	  SendMessage32A( pWnd->hwndSelf, WM_DROPFILES,
			  (WPARAM32)hDrop.h32, 0L );
	} else {
	  GlobalUnlock16(hDrop.h16);
	  PostMessage16( pWnd->hwndSelf, WM_DROPFILES,
			 (WPARAM16)hDrop.h16, 0L );
	}
      }
    }
    if( p_data ) TSXFree(p_data);  
  }
}

/**********************************************************************
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage( WND *pWnd, XClientMessageEvent *event )
{
    if (event->message_type != None && event->format == 32) {
      if ((event->message_type == wmProtocols) && 
	  (((Atom) event->data.l[0]) == wmDeleteWindow))
	SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_CLOSE, 0 );
      else if ( event->message_type == dndProtocol &&
		(event->data.l[0] == DndFile || event->data.l[0] == DndFiles) )
	EVENT_DropFromOffiX(pWnd, event);
      else if ( event->message_type == dndProtocol &&
		event->data.l[0] == DndURL )
	EVENT_DropURLs(pWnd, event);
      else {
#if 0
	/* enable this if you want to see the message */
	unsigned char* p_data = NULL;
	union {
	  unsigned long	l;
	  int            	i;
	  Atom		atom;
	} u; /* unused */
	TSXGetWindowProperty( display, DefaultRootWindow(display),
			      dndSelection, 0, 65535, FALSE,
			      AnyPropertyType, &u.atom, &u.i,
			      &u.l, &u.l, &p_data);
	TRACE(event, "message_type=%ld, data=%ld,%ld,%ld,%ld,%ld, msg=%s\n",
	      event->message_type, event->data.l[0], event->data.l[1], 
	      event->data.l[2], event->data.l[3], event->data.l[4],
	      p_data);
#endif
	TRACE(event, "unrecognized ClientMessage\n" );
      }
    }
}

/**********************************************************************
 *           EVENT_EnterNotify
 *
 * Install colormap when Wine window is focused in
 * self-managed mode with private colormap
 */
/*
  void EVENT_EnterNotify( WND *pWnd, XCrossingEvent *event )
  {
   if( !Options.managed && rootWindow == DefaultRootWindow(display) &&
     (COLOR_GetSystemPaletteFlags() & COLOR_PRIVATE) && GetFocus32() )
      TSXInstallColormap( display, COLOR_GetColormap() );
  }
 */ 

/**********************************************************************
 *		EVENT_MapNotify
 */
void EVENT_MapNotify( HWND32 hWnd, XMapEvent *event )
{
    HWND32 hwndFocus = GetFocus32();

    if (hwndFocus && IsChild32( hWnd, hwndFocus ))
      FOCUS_SetXFocus( (HWND32)hwndFocus );

    return;
}

