/*
 * X events handling functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>

#include "windows.h"
#include "win.h"


#define NB_BUTTONS      3     /* Windows can handle 3 buttons */
#define DBLCLICK_TIME   300   /* Max. time for a double click (milliseconds) */


  /* Event handlers */
static void EVENT_expose();
static void EVENT_key();
static void EVENT_mouse_motion();
static void EVENT_mouse_button();
static void EVENT_structure();

  /* State variables */
static HWND captureWnd = 0;

/***********************************************************************
 *           EVENT_AddHandlers
 *
 * Add the event handlers to the given window
 */
void EVENT_AddHandlers( Widget w, int hwnd )
{
    XtAddEventHandler(w, ExposureMask, FALSE,
		      EVENT_expose, (XtPointer)hwnd );
    XtAddEventHandler(w, KeyPressMask | KeyReleaseMask, FALSE, 
		      EVENT_key, (XtPointer)hwnd );
    XtAddEventHandler(w, PointerMotionMask, FALSE,
		      EVENT_mouse_motion, (XtPointer)hwnd );
    XtAddEventHandler(w, ButtonPressMask | ButtonReleaseMask, FALSE,
		      EVENT_mouse_button, (XtPointer)hwnd );
    XtAddEventHandler(w, StructureNotifyMask, FALSE,
		      EVENT_structure, (XtPointer)hwnd );
}


/***********************************************************************
 *           EVENT_RemoveHandlers
 *
 * Remove the event handlers of the given window
 */
void EVENT_RemoveHandlers( Widget w, int hwnd )
{
    XtRemoveEventHandler(w, ExposureMask, FALSE,
			 EVENT_expose, (XtPointer)hwnd );
    XtRemoveEventHandler(w, KeyPressMask | KeyReleaseMask, FALSE, 
			 EVENT_key, (XtPointer)hwnd );
    XtRemoveEventHandler(w, PointerMotionMask, FALSE,
			 EVENT_mouse_motion, (XtPointer)hwnd );
    XtRemoveEventHandler(w, ButtonPressMask | ButtonReleaseMask, FALSE,
			 EVENT_mouse_button, (XtPointer)hwnd );
    XtRemoveEventHandler(w, StructureNotifyMask, FALSE,
			 EVENT_structure, (XtPointer)hwnd );
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
 *           EVENT_expose
 *
 * Handle a X expose event
 */
static void EVENT_expose( Widget w, int hwnd, XExposeEvent *event,
			  Boolean *cont_dispatch )
{
    RECT rect;
    rect.left   = event->x;
    rect.top    = event->y;
    rect.right  = event->x + event->width;
    rect.bottom = event->y + event->height;
    
    InvalidateRect( hwnd, &rect, TRUE );
}


/***********************************************************************
 *           EVENT_key
 *
 * Handle a X key event
 */
static void EVENT_key( Widget w, int hwnd, XKeyEvent *event,
		       Boolean *cont_dispatch )
{
    MSG msg;
    
    msg.hwnd    = hwnd;
    msg.message = (event->type == KeyRelease) ? WM_KEYUP : WM_KEYDOWN;
    msg.wParam  = 0;
    msg.lParam  = (event->x & 0xffff) | (event->y << 16);
    msg.time = event->time;
    msg.pt.x = event->x & 0xffff;
    msg.pt.y = event->y & 0xffff;
    
    MSG_AddMsg( &msg );    
}


/***********************************************************************
 *           EVENT_mouse_motion
 *
 * Handle a X mouse motion event
 */
static void EVENT_mouse_motion( Widget w, int hwnd, XMotionEvent *event, 
			        Boolean *cont_dispatch )
{
    MSG msg;
    
    msg.hwnd    = hwnd;
    msg.message = WM_MOUSEMOVE;
    msg.wParam  = EVENT_XStateToKeyState( event->state );
    msg.lParam  = (event->x & 0xffff) | (event->y << 16);
    msg.time = event->time;
    msg.pt.x = event->x & 0xffff;
    msg.pt.y = event->y & 0xffff;
    
    MSG_AddMsg( &msg );    
}


/***********************************************************************
 *           EVENT_mouse_button
 *
 * Handle a X mouse button event
 */
static void EVENT_mouse_button( Widget w, int hwnd, XButtonEvent *event,
			        Boolean *cont_dispatch )
{
    static WORD messages[3][NB_BUTTONS] = 
    {
	{ WM_LBUTTONDOWN, WM_MBUTTONDOWN, WM_RBUTTONDOWN },
	{ WM_LBUTTONUP, WM_MBUTTONUP, WM_RBUTTONUP },
        { WM_LBUTTONDBLCLK, WM_MBUTTONDBLCLK, WM_RBUTTONDBLCLK }
    };
    static unsigned long lastClickTime[NB_BUTTONS] = { 0, 0, 0 };
        
    MSG msg;
    int buttonNum, prevTime, type;
    
    buttonNum = event->button-1;
    if (buttonNum >= NB_BUTTONS) return;
    if (event->type == ButtonRelease) type = 1;
    else
    {  /* Check if double-click */
	prevTime = lastClickTime[buttonNum];
	lastClickTime[buttonNum] = event->time;
	type = (event->time - prevTime < DBLCLICK_TIME) ? 2 : 0;
    }	
    
    msg.hwnd    = hwnd;
    msg.message = messages[type][buttonNum];
    msg.wParam  = EVENT_XStateToKeyState( event->state );
    msg.lParam  = (event->x & 0xffff) | (event->y << 16);
    msg.time = event->time;
    msg.pt.x = event->x & 0xffff;
    msg.pt.y = event->y & 0xffff;

    MSG_AddMsg( &msg );    
}


/***********************************************************************
 *           EVENT_structure
 *
 * Handle a X StructureNotify event
 */
static void EVENT_structure( Widget w, int hwnd, XEvent *event, 
 			     Boolean *cont_dispatch )
{
    MSG msg;
    
    msg.hwnd = hwnd;
    msg.time = GetTickCount();
    msg.pt.x = 0;
    msg.pt.y = 0;

    switch(event->type)
    {
      case ConfigureNotify:
	{
	    XConfigureEvent * evt = (XConfigureEvent *)event;
	    WND * wndPtr = WIN_FindWndPtr( hwnd );
	    if (!wndPtr) return;
	    wndPtr->rectClient.right  = wndPtr->rectClient.left + evt->width;
	    wndPtr->rectClient.bottom = wndPtr->rectClient.top + evt->height;
	    PostMessage( hwnd, WM_SIZE, SIZE_RESTORED,
			 (evt->width & 0xffff) | (evt->height << 16) );
	}
	break;
	
    }    
}

/**********************************************************************
 *		SetCapture 	(USER.18)
 */
HWND SetCapture(HWND wnd)
{
    int rv;
    HWND old_capture_wnd = captureWnd;
    WND *wnd_p = WIN_FindWndPtr(wnd);
    if (wnd_p == NULL)
	return 0;
    
    rv = XtGrabPointer(wnd_p->winWidget, False, 
		       ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
		       GrabModeAsync, GrabModeSync, None, None, CurrentTime);

    GlobalUnlock(wnd);
    if (rv == GrabSuccess)
    {
	captureWnd = wnd;
	return old_capture_wnd;
    }
    else
	return 0;
}

/**********************************************************************
 *		ReleaseCapture	(USER.19)
 */
void ReleaseCapture()
{
    WND *wnd_p;
    
    if (captureWnd == 0)
	return;
    
    wnd_p = WIN_FindWndPtr(captureWnd);
    if (wnd_p == NULL)
	return;
    
    XtUngrabPointer(wnd_p->winWidget, CurrentTime);

    GlobalUnlock(captureWnd);
    captureWnd = 0;
}
