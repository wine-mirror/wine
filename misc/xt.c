/*
 * X toolkit functions
 *
 * Copyright 1993 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1993";

#include <sys/param.h>
#include <sys/times.h>

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Core.h>
#include <X11/Shell.h>

#include "message.h"
#include "callback.h"
#include "win.h"
#include "class.h"
#include "gdi.h"


Display * XT_display;
Screen * XT_screen;
XtAppContext XT_app_context;

static Widget topLevelWidget;


/***********************************************************************
 *           main
 */
void main(int argc, char **argv)
{    
    topLevelWidget = XtVaAppInitialize(&XT_app_context,
				       "XWine",     /* Application class */
				       NULL, 0,     /* Option list */
				       &argc, argv, /* Command line args */
				       NULL,        /* Fallback resources */
				       NULL );
    XT_display = XtDisplay( topLevelWidget );
    XT_screen  = XtScreen( topLevelWidget );
    
    _WinMain( argc, argv );
}


/***********************************************************************
 *           DefWindowProc   (USER.107)
 */
LONG DefWindowProc( HWND hwnd, WORD msg, WORD wParam, LONG lParam )
{
    WND * wndPtr;
    CLASS * classPtr;
    
#ifdef DEBUG_MESSAGE
    printf( "DefWindowProc: %d %d %d %08x\n", hwnd, msg, wParam, lParam );
#endif

    switch(msg)
    {
    case WM_PAINT:
	{
	    PAINTSTRUCT paintstruct;
	    BeginPaint( hwnd, &paintstruct );
	    EndPaint( hwnd, &paintstruct );
	    return 0;
	}

    case WM_CREATE:
	return 0;

    case WM_CLOSE:
	DestroyWindow( hwnd );
	return 0;

    case WM_ERASEBKGND:
    case WM_ICONERASEBKGND:
	{
	    if (!(wndPtr = WIN_FindWndPtr( hwnd ))) return 1;
	    if (!(classPtr = CLASS_FindClassPtr( wndPtr->hClass ))) return 1;
	    if (!classPtr->wc.hbrBackground) return 1;
	    FillWindow( wndPtr->hwndParent, hwnd, (HDC)wParam,
		        classPtr->wc.hbrBackground );
	    GlobalUnlock( hwnd );
	    return 0;
	}
    }
    return 0;
}


/********************************************************************
 *
 * Miscellaneous partially implemented functions.
 * 
 */


int MessageBox( HWND hwnd, LPSTR str, LPSTR title, WORD type )
{
    printf( "MessageBox: '%s'\n", str );
}

void MessageBeep( WORD i )
{
    printf( "MessageBeep: %d\n", i );
}

WORD RegisterWindowMessage( LPSTR str )
{
    printf( "RegisterWindowMessage: '%s'\n", str );
    return 0xc000;
}

/***********************************************************************
 *           GetTickCount    (USER.13)
 */
DWORD GetTickCount()
{
    struct tms dummy;
    return times(&dummy) / (1000 / HZ);
}


int GetSystemMetrics( short index )
{
    printf( "GetSystemMetrics: %d\n", index );
    switch(index)
    {
      case SM_CXSCREEN:
	return DisplayWidth( XT_display, DefaultScreen( XT_display ));

      case SM_CYSCREEN:
	return DisplayHeight( XT_display, DefaultScreen( XT_display ));

      default:
	  return 0;
    }
}

void AdjustWindowRect( LPRECT rect, DWORD style, BOOL menu )
{
    printf( "AdjustWindowRect: (%d,%d)-(%d,%d) %d %d\n", rect->left, rect->top,
	   rect->right, rect->bottom, style, menu );
}

WORD GetPrivateProfileInt( LPSTR section, LPSTR entry,
			   short defval, LPSTR filename )
{
    printf( "GetPrivateProfileInt: %s %s %d %s\n", section, entry, defval, filename );
    return defval;
}

short GetPrivateProfileString( LPSTR section, LPSTR entry, LPSTR defval,
			       LPSTR buffer, short count, LPSTR filename )
{
    printf( "GetPrivateProfileString: %s %s %s %d %s\n", section, entry, defval, count, filename );
    strncpy( buffer, defval, count );
    buffer[count-1] = 0;
    return strlen(buffer);
}

BOOL IsIconic( HWND hwnd )
{
    printf( "IsIconic: returning FALSE\n" );
    return FALSE;
}

HMENU CreateMenu() { }

BOOL AppendMenu( HMENU hmenu, WORD flags, WORD id, LPSTR text ) { }

