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
Display * display;
Screen * screen;
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
    display = XtDisplay( topLevelWidget );
    screen  = XtScreen( topLevelWidget );
    
    DOS_InitFS();
    Comm_Init();
    _WinMain( argc, argv );
}



/********************************************************************
 *
 * Miscellaneous partially implemented functions.
 * 
 */


void MessageBeep( WORD i )
{
    XBell(XT_display, 100);
}

/***********************************************************************
 *           GetTickCount    (USER.13)
 */
DWORD GetTickCount()
{
    struct tms dummy;
    return (times(&dummy) * 1000) / HZ;
}
