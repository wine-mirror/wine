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

#include "windows.h"

#ifdef __NetBSD__
#define HZ 100
#endif

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
    XBell(XT_display, 100);
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
    return (times(&dummy) * 1000) / HZ;
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


BOOL IsIconic( HWND hwnd )
{
    printf( "IsIconic: returning FALSE\n" );
    return FALSE;
}

HMENU CreateMenu() { return 0; }

BOOL AppendMenu( HMENU hmenu, WORD flags, WORD id, LPSTR text ) { return TRUE;}

BOOL DestroyMenu( HMENU hmenu ) { return TRUE; }
