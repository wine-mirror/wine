/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 */

static char Copyright[] = "Copyright  Alexandre Julliard, 1994";

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/Xresource.h>

#include "windows.h"
#include "options.h"


Display * XT_display;  /* To be removed */
Screen * XT_screen;    /* To be removed */

Display * display;
Screen * screen;


struct options Options =
{  /* default options */
    NULL,           /* spyFilename */
    FALSE,          /* usePrivateMap */
    FALSE,          /* synchronous */
    SW_SHOWNORMAL   /* cmdShow */
};


static XrmOptionDescRec optionsTable[] =
{
    { "-display",     ".display",     XrmoptionSepArg, (caddr_t)NULL },
    { "-iconic",      ".iconic",      XrmoptionNoArg,  (caddr_t)"on" },
    { "-privatemap",  ".privatemap",  XrmoptionNoArg,  (caddr_t)"on" },
    { "-synchronous", ".synchronous", XrmoptionNoArg,  (caddr_t)"on" },
    { "-spy",         ".spy",         XrmoptionSepArg, (caddr_t)NULL }
};

#define NB_OPTIONS  (sizeof(optionsTable) / sizeof(optionsTable[0]))


/***********************************************************************
 *           MAIN_Usage
 */
static void MAIN_Usage( char *name )
{
    fprintf( stderr,"Usage: %s [-display name] [-iconic] [-privatemap]\n"
	            "        [-synchronous] [-spy file] program [arguments]\n",
	     name );
    exit(1);
}


/***********************************************************************
 *           MAIN_ParseOptions
 *
 * Parse command line options and open display.
 */
static void MAIN_ParseOptions( int *argc, char *argv[] )
{
    char *dummy, *display_name;
    XrmValue value;
    XrmDatabase db = NULL;

    XrmParseCommand( &db, optionsTable, NB_OPTIONS, "wine", argc, argv );
    if (*argc < 2) MAIN_Usage( argv[0] );

    if (XrmGetResource( db, "wine.display", "Wine.display", &dummy, &value ))
	display_name = value.addr;
    else display_name = NULL;

    if (!(display = XOpenDisplay( display_name )))
    {
	fprintf( stderr, "%s: Can't open display: %s\n",
		 argv[0], display_name ? display_name : "" );
	exit(1);
    }

    if (XrmGetResource(db,"wine.iconic","Wine.iconic",&dummy,&value))
	Options.cmdShow = SW_SHOWMINIMIZED;
    if (XrmGetResource(db,"wine.privatemap","Wine.privatemap",&dummy,&value))
	Options.usePrivateMap = TRUE;
    if (XrmGetResource(db,"wine.synchronous","Wine.synchronous",&dummy,&value))
	Options.synchronous = TRUE;
    if (XrmGetResource(db,"wine.spy","Wine.spy",&dummy,&value))
	Options.spyFilename = value.addr;
}


/***********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{    

    XrmInitialize();
    
    MAIN_ParseOptions( &argc, argv );

    screen = DefaultScreenOfDisplay( display );
    XT_display = display;
    XT_screen = screen;
    if (Options.synchronous) XSynchronize( display, True );

    DOS_InitFS();
    Comm_Init();
    return _WinMain( argc, argv );
}


/***********************************************************************
 *           MessageBeep    (USER.104)
 */
void MessageBeep( WORD i )
{
    XBell( display, 100 );
}
