/*
 * X toolkit functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>
#include <X11/Shell.h>

#include "message.h"
#include "callback.h"
#include "win.h"


Widget XT_topLevelWidget;

static XtAppContext app_context;


/***********************************************************************
 *           main
 */
void main(int argc, char **argv)
{    
    XT_topLevelWidget = XtVaAppInitialize(&app_context,
					  "XWine",     /* Application class */
					  NULL, 0,     /* Option list */
					  &argc, argv, /* Command line args */
					  NULL,        /* Fallback resources */
					  NULL );
    _WinMain( argc, argv );
}


/***********************************************************************
 *           GetMessage   (USER.108)
 */
BOOL GetMessage( LPMSG msg, HWND hwnd, WORD first, WORD last ) 
{
    XEvent event;
    
    while(1)
    {
	if (PeekMessage( msg, hwnd, first, last, PM_REMOVE )) break;
	XtAppNextEvent( app_context, &event );
	XtDispatchEvent( &event );
    }

    return (msg->message != WM_QUIT);
}


/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LONG DefWindowProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    PAINTSTRUCT paintstruct;
    
    printf( "DefWindowProc: %d %d %d %d\n", hwnd, msg, wParam, lParam );

    switch(msg)
    {
    case WM_PAINT:
	BeginPaint( hwnd, &paintstruct );
	EndPaint( hwnd, &paintstruct );
	return 0;

    case WM_CREATE:
	return 0;

    }
    return 0;
}


/********************************************************************
 *
 * Miscellaneous partially implemented functions.
 * 
 */


HDC BeginPaint( HWND hwnd, LPPAINTSTRUCT lps ) 
{
    return hwnd;
}


void EndPaint( HWND hwnd, LPPAINTSTRUCT lps )
{
    MSG_EndPaint();
}

int DrawText( HDC hdc, LPSTR str, int count, LPRECT rect, WORD flags )
{
    WND * wndPtr = WIN_FindWndPtr( hdc );
    int x = rect->left, y = rect->top;
        
    if (flags & DT_CENTER) x = (rect->left + rect->right) / 2;
    if (flags & DT_VCENTER) y = (rect->top + rect->bottom) / 2;
    if (count == -1) count = strlen(str);

    printf( "DrawText: %d,%d '%s'\n", x, y, str );
    if (wndPtr)
    {
	XDrawString( XtDisplay(wndPtr->winWidget),
		    XtWindow(wndPtr->winWidget),
		    DefaultGCOfScreen(XtScreen(wndPtr->winWidget)),
		    x, y, str, count );    
	GlobalUnlock( hdc );
    }
}

int MessageBox( HWND hwnd, LPSTR str, LPSTR title, WORD type )
{
    printf( "MessageBox: '%s'\n", str );
}

void MessageBeep( WORD i )
{
    printf( "MessageBeep: %d\n", i );
}

HDC GetDC( HWND hwnd ) { }

HMENU CreateMenu() { }

HMENU GetMenu( HWND hwnd ) { }

BOOL SetMenu( HWND hwnd, HMENU hmenu ) { }

BOOL AppendMenu( HMENU hmenu, WORD flags, WORD id, LPSTR text ) { }

BOOL Rectangle( HDC hdc, int left, int top, int right, int bottom ) { }

HANDLE GetStockObject( int obj ) { }


