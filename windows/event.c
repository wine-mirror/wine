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
#include "debug.h"
#include "dde_proc.h"
#include "winsock.h"


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

  /* X context to associate a hwnd to an X window */
static XContext winContext = 0;

static INT16  captureHT = HTCLIENT;
static HWND32 captureWnd = 0;
static BOOL32 InputEnabled = TRUE;
static BOOL32 SwappedButtons = FALSE;

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
static void EVENT_Expose( WND *pWnd, XExposeEvent *event );
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
    
    if (TSXFindContext( display, event->xany.window, winContext,
                      (char **)&pWnd ) != 0)
        return;  /* Not for a registered window */

    TRACE(event, "Got event %s for hwnd %04x\n",
                   event_names[event->type], pWnd->hwndSelf );

    switch(event->type)
    {
    case KeyPress:
    case KeyRelease:
        if (InputEnabled) EVENT_Key( pWnd, (XKeyEvent*)event );
	break;
	
    case ButtonPress:
        if (InputEnabled)
            EVENT_ButtonPress( pWnd, (XButtonEvent*)event );
	break;

    case ButtonRelease:
        if (InputEnabled)
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
        if (InputEnabled)
	{
            while (TSXCheckTypedWindowEvent(display,((XAnyEvent *)event)->window,
                                          MotionNotify, event));    
            EVENT_MotionNotify( pWnd, (XMotionEvent*)event );
	}
	break;

    case FocusIn:
        EVENT_FocusIn( pWnd, (XFocusChangeEvent*)event );
	break;

    case FocusOut:
	EVENT_FocusOut( pWnd, (XFocusChangeEvent*)event );
	break;

    case Expose:
	EVENT_Expose( pWnd, (XExposeEvent *)event );
	break;

    case GraphicsExpose:
	EVENT_GraphicsExpose( pWnd, (XGraphicsExposeEvent *)event );
        break;

    case ConfigureNotify:
	EVENT_ConfigureNotify( pWnd, (XConfigureEvent*)event );
	break;

    case SelectionRequest:
	EVENT_SelectionRequest( pWnd, (XSelectionRequestEvent *)event );
	break;

    case SelectionNotify:
	EVENT_SelectionNotify( (XSelectionEvent *)event );
	break;

    case SelectionClear:
	EVENT_SelectionClear( pWnd, (XSelectionClearEvent*) event );
	break;

    case ClientMessage:
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
	EVENT_MapNotify( pWnd->hwndSelf, (XMapEvent *)event );
	break;

    default:    
	WARN(event, "Unprocessed event %s for hwnd %04x\n",
	        event_names[event->type], pWnd->hwndSelf );
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

    while (TSXPending( display ))
    {

#ifdef CONFIG_IPC
        if (DDE_GetRemoteMessage())
        {
            while(DDE_GetRemoteMessage()) ;
            return TRUE;
        }
#endif  /* CONFIG_IPC */

	TSXNextEvent( display, &event );

        if( peek )
        {
	  WND*		pWnd;
	  MESSAGEQUEUE* pQ;

          if (TSXFindContext( display, ((XAnyEvent *)&event)->window, winContext,
                            (char **)&pWnd ) || (event.type == NoExpose))
              continue;

	  /* Check only for those events which can be processed
	   * internally. */

	  if( event.type == MotionNotify ||
	      event.type == ButtonPress || event.type == ButtonRelease ||
	      event.type == KeyPress || event.type == KeyRelease ||
	      event.type == SelectionRequest || event.type == SelectionClear )
	  {
	       EVENT_ProcessEvent( &event );
               continue;
	  }

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
    }
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

    TSXSync( display, False );
    while (TSXPending( display ))
    {
	TSXNextEvent( display, &event );
	EVENT_ProcessEvent( &event );
    }    
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
 *           EVENT_Expose
 */
static void EVENT_Expose( WND *pWnd, XExposeEvent *event )
{
    RECT32 rect;

    /* Make position relative to client area instead of window */
    rect.left   = event->x - (pWnd->rectClient.left - pWnd->rectWindow.left);
    rect.top    = event->y - (pWnd->rectClient.top - pWnd->rectWindow.top);
    rect.right  = rect.left + event->width;
    rect.bottom = rect.top + event->height;

    PAINT_RedrawWindow( pWnd->hwndSelf, &rect, 0,
                        RDW_INVALIDATE | RDW_FRAME | RDW_ALLCHILDREN | RDW_ERASE |
                        (event->count ? 0 : RDW_ERASENOW), 0 );
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
    hardware_event( WM_MOUSEMOVE, EVENT_XStateToKeyState( event->state ), 0L,
                    pWnd->rectWindow.left + event->x,
                    pWnd->rectWindow.top + event->y,
                    event->time - MSG_WineStartTicks, pWnd->hwndSelf );
}


/***********************************************************************
 *           EVENT_DummyMotionNotify
 *
 * Generate a dummy MotionNotify event. Used to force a WM_SETCURSOR message.
 */
void EVENT_DummyMotionNotify(void)
{
    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int state;

    if (TSXQueryPointer( display, rootWindow, &root, &child,
                       &rootX, &rootY, &winX, &winY, &state ))
    {
        hardware_event( WM_MOUSEMOVE, EVENT_XStateToKeyState( state ), 0L,
                        winX, winY, GetTickCount(), 0 );
    }
}


/***********************************************************************
 *           EVENT_ButtonPress
 */
static void EVENT_ButtonPress( WND *pWnd, XButtonEvent *event )
{
    static WORD messages[NB_BUTTONS] = 
        { WM_LBUTTONDOWN, WM_MBUTTONDOWN, WM_RBUTTONDOWN };
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;
    if (SwappedButtons) buttonNum = NB_BUTTONS - 1 - buttonNum;
    MouseButtonsStates[buttonNum] = TRUE;
    AsyncMouseButtonsStates[buttonNum] = TRUE;
    hardware_event( messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
                    pWnd->rectWindow.left + event->x,
                    pWnd->rectWindow.top + event->y,
		    event->time - MSG_WineStartTicks, pWnd->hwndSelf );
}


/***********************************************************************
 *           EVENT_ButtonRelease
 */
static void EVENT_ButtonRelease( WND *pWnd, XButtonEvent *event )
{
    static const WORD messages[NB_BUTTONS] = 
        { WM_LBUTTONUP, WM_MBUTTONUP, WM_RBUTTONUP };
    int buttonNum = event->button - 1;

    if (buttonNum >= NB_BUTTONS) return;    
    if (SwappedButtons) buttonNum = NB_BUTTONS - 1 - buttonNum;
    MouseButtonsStates[buttonNum] = FALSE;
    hardware_event( messages[buttonNum],
		    EVENT_XStateToKeyState( event->state ), 0L,
                    pWnd->rectWindow.left + event->x,
                    pWnd->rectWindow.top + event->y,
		    event->time - MSG_WineStartTicks, pWnd->hwndSelf );
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
	    WINPOS_ChangeActiveWindow( hwnd, FALSE );
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
 *           EVENT_ClientMessage
 */
static void EVENT_ClientMessage( WND *pWnd, XClientMessageEvent *event )
{
    if (event->message_type != None && event->format == 32)
    {
       if ((event->message_type == wmProtocols) && 
           (((Atom) event->data.l[0]) == wmDeleteWindow))
	  SendMessage16( pWnd->hwndSelf, WM_SYSCOMMAND, SC_CLOSE, 0 );
       else if ( event->message_type == dndProtocol &&
		(event->data.l[0] == DndFile || event->data.l[0] == DndFiles) )
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
		  LPDROPFILESTRUCT        lpDrop;

		  aux_long += sizeof(DROPFILESTRUCT) + 1; 
		  hDrop = (HDROP16)GlobalAlloc16( GMEM_SHARE, aux_long );
		  lpDrop = (LPDROPFILESTRUCT) GlobalLock16( hDrop );

		  if( lpDrop )
		  {
		    lpDrop->wSize = sizeof(DROPFILESTRUCT);
		    lpDrop->ptMousePos.x = (INT16)x;
		    lpDrop->ptMousePos.y = (INT16)y;
		    lpDrop->fInNonClientArea = (BOOL16) 
			( x < (pDropWnd->rectClient.left - pDropWnd->rectWindow.left)  ||
			  y < (pDropWnd->rectClient.top - pDropWnd->rectWindow.top)    ||
			  x > (pDropWnd->rectClient.right - pDropWnd->rectWindow.left) ||
			  y > (pDropWnd->rectClient.bottom - pDropWnd->rectWindow.top) );
		    p_drop = ((char*)lpDrop) + sizeof(DROPFILESTRUCT);
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
       } /* dndProtocol */
       else
	  TRACE(event, "unrecognized ClientMessage\n" );
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

/**********************************************************************
 *              EVENT_Capture
 * 
 * We need this to be able to generate double click messages
 * when menu code captures mouse in the window without CS_DBLCLK style.
 */
HWND32 EVENT_Capture(HWND32 hwnd, INT16 ht)
{
    Window win;
    HWND32 capturePrev = captureWnd;

    if (!hwnd)
    {
        TSXUngrabPointer(display, CurrentTime );
        captureWnd = NULL; captureHT = 0; 
    }
    else if ((win = WIN_GetXWindow( hwnd )))
    {
	WND* wndPtr = WIN_FindWndPtr( hwnd );

        if ( wndPtr && 
	     (TSXGrabPointer(display, win, False, 
			   ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                           GrabModeAsync, GrabModeAsync,
                           None, None, CurrentTime ) == GrabSuccess) )
	{
            TRACE(win, "(0x%04x)\n", hwnd );
            captureWnd   = hwnd;
	    captureHT    = ht;
        }
    }

    if( capturePrev && capturePrev != captureWnd )
    {
	WND* wndPtr = WIN_FindWndPtr( capturePrev );
        if( wndPtr && (wndPtr->flags & WIN_ISWIN32) )
            SendMessage32A( capturePrev, WM_CAPTURECHANGED, 0L, hwnd);
    }
    return capturePrev;
}

/**********************************************************************
 *              EVENT_GetCaptureInfo
 */
INT16 EVENT_GetCaptureInfo()
{
    return captureHT;
}

/**********************************************************************
 *		SetCapture16   (USER.18)
 */
HWND16 WINAPI SetCapture16( HWND16 hwnd )
{
    return (HWND16)EVENT_Capture( hwnd, HTCLIENT );
}


/**********************************************************************
 *		SetCapture32   (USER32.464)
 */
HWND32 WINAPI SetCapture32( HWND32 hwnd )
{
    return EVENT_Capture( hwnd, HTCLIENT );
}


/**********************************************************************
 *		ReleaseCapture   (USER.19) (USER32.439)
 */
void WINAPI ReleaseCapture(void)
{
    TRACE(win, "captureWnd=%04x\n", captureWnd );
    if( captureWnd ) EVENT_Capture( 0, 0 );
}


/**********************************************************************
 *		GetCapture16   (USER.236)
 */
HWND16 WINAPI GetCapture16(void)
{
    return captureWnd;
}


/**********************************************************************
 *		GetCapture32   (USER32.208)
 */
HWND32 WINAPI GetCapture32(void)
{
    return captureWnd;
}


/***********************************************************************
 *           GetMouseEventProc   (USER.337)
 */
FARPROC16 WINAPI GetMouseEventProc(void)
{
    HMODULE16 hmodule = GetModuleHandle16("USER");
    return NE_GetEntryPoint( hmodule, NE_GetOrdinal( hmodule, "Mouse_Event" ));
}


/***********************************************************************
 *           Mouse_Event   (USER.299)
 */
void WINAPI Mouse_Event( CONTEXT *context )
{
    /* Register values:
     * AX = mouse event
     * BX = horizontal displacement if AX & ME_MOVE
     * CX = vertical displacement if AX & ME_MOVE
     * DX = button state (?)
     * SI = mouse event flags (?)
     */
    Window root, child;
    int rootX, rootY, winX, winY;
    unsigned int state;

    if (AX_reg(context) & ME_MOVE)
    {
        /* We have to actually move the cursor */
        TSXWarpPointer( display, rootWindow, None, 0, 0, 0, 0,
                      (short)BX_reg(context), (short)CX_reg(context) );
        return;
    }
    if (!TSXQueryPointer( display, rootWindow, &root, &child,
                        &rootX, &rootY, &winX, &winY, &state )) return;
    if (AX_reg(context) & ME_LDOWN)
        hardware_event( WM_LBUTTONDOWN, EVENT_XStateToKeyState( state ),
                        0L, winX, winY, GetTickCount(), 0 );
    if (AX_reg(context) & ME_LUP)
        hardware_event( WM_LBUTTONUP, EVENT_XStateToKeyState( state ),
                        0L, winX, winY, GetTickCount(), 0 );
    if (AX_reg(context) & ME_RDOWN)
        hardware_event( WM_RBUTTONDOWN, EVENT_XStateToKeyState( state ),
                        0L, winX, winY, GetTickCount(), 0 );
    if (AX_reg(context) & ME_RUP)
        hardware_event( WM_RBUTTONUP, EVENT_XStateToKeyState( state ),
                        0L, winX, winY, GetTickCount(), 0 );
}


/**********************************************************************
 *			EnableHardwareInput   (USER.331)
 */
BOOL16 WINAPI EnableHardwareInput(BOOL16 bEnable)
{
  BOOL16 bOldState = InputEnabled;
  FIXME(event,"(%d) - stub\n", bEnable);
  InputEnabled = bEnable;
  return bOldState;
}


/***********************************************************************
 *	     SwapMouseButton16   (USER.186)
 */
BOOL16 WINAPI SwapMouseButton16( BOOL16 fSwap )
{
    BOOL16 ret = SwappedButtons;
    SwappedButtons = fSwap;
    return ret;
}


/***********************************************************************
 *	     SwapMouseButton32   (USER32.537)
 */
BOOL32 WINAPI SwapMouseButton32( BOOL32 fSwap )
{
    BOOL32 ret = SwappedButtons;
    SwappedButtons = fSwap;
    return ret;
}
