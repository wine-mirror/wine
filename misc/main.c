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

char *ProgramName;		/* Used by resource.c with WINELIB */

struct options Options =
{  /* default options */
    NULL,           /* spyFilename */
    FALSE,          /* usePrivateMap */
    FALSE,          /* synchronous */
    SW_SHOWNORMAL,  /* cmdShow */
    FALSE
};


static XrmOptionDescRec optionsTable[] =
{
    { "-display",     ".display",     XrmoptionSepArg, (caddr_t)NULL },
    { "-iconic",      ".iconic",      XrmoptionNoArg,  (caddr_t)"on" },
    { "-privatemap",  ".privatemap",  XrmoptionNoArg,  (caddr_t)"on" },
    { "-synchronous", ".synchronous", XrmoptionNoArg,  (caddr_t)"on" },
    { "-spy",         ".spy",         XrmoptionSepArg, (caddr_t)NULL },
    { "-relaydbg",    ".relaydbg",    XrmoptionNoArg,  (caddr_t)"on" }
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
#ifdef WINELIB
    /* Need to assemble command line and pass it to WinMain */
#else
    if (*argc < 2) MAIN_Usage( argv[0] );
#endif
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
    if (XrmGetResource(db,"wine.relaydbg","Wine.relaydbg",&dummy,&value))
	Options.relay_debug = TRUE;
    if (XrmGetResource(db,"wine.spy","Wine.spy",&dummy,&value))
	Options.spyFilename = value.addr;
}


/***********************************************************************
 *           main
 */
int main( int argc, char *argv[] )
{    
    int ret_val;
    XKeyboardState keyboard_state;
    XKeyboardControl keyboard_value;
    
    XrmInitialize();
    
    MAIN_ParseOptions( &argc, argv );

    screen = DefaultScreenOfDisplay( display );
    XT_display = display;
    XT_screen = screen;
    if (Options.synchronous) XSynchronize( display, True );

    XGetKeyboardControl(display, &keyboard_state);

    ProgramName = argv [0];
    DOS_InitFS();
    Comm_Init();

    ret_val = _WinMain( argc, argv );
    
    Comm_DeInit ();
    DOS_DeInitFS ();
    sync_profiles ();

    /* restore sounds/keyboard settings */

    keyboard_value.key_click_percent	= keyboard_state.key_click_percent;
    keyboard_value.bell_percent 	= keyboard_state.bell_percent;
    keyboard_value.bell_pitch		= keyboard_state.bell_pitch;
    keyboard_value.bell_duration	= keyboard_state.bell_duration;
    keyboard_value.auto_repeat_mode	= keyboard_state.global_auto_repeat;

    XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent | 
    	KBBellPitch | KBBellDuration | KBAutoRepeatMode, &keyboard_value);

    return ret_val;
}

/***********************************************************************
 *           MessageBeep    (USER.104)
 */
void MessageBeep( WORD i )
{
	XBell(display, 100);
}

/***********************************************************************
 *      GetVersion (KERNEL.3)
 */
LONG GetVersion(void)
{
	return (0x04001003); /* dos version 4.00, win ver 3.1 */
}

/***********************************************************************
 *	GetWinFlags (KERNEL.132)
 */
LONG GetWinFlags(void)
{
	return (WF_STANDARD | WF_CPU286 | WF_PMODE | WF_80x87);
}

/***********************************************************************
 *	GetTimerResolution (USER.14)
 */
LONG GetTimerResolution(void)
{
	return (1000);
}

/***********************************************************************
 *	SystemParametersInfo (USER.483)
 */
BOOL SystemParametersInfo (UINT uAction, UINT uParam, void FAR *lpvParam, UINT fuWinIni)
{
	XKeyboardState		keyboard_state;
	XKeyboardControl	keyboard_value;

	fprintf(stderr, "SystemParametersInfo: action %d, param %x, flag %x\n", 
			uAction, uParam, fuWinIni);

	switch (uAction) {
		case SPI_GETBEEP:
			XGetKeyboardControl(display, &keyboard_state);
			if (keyboard_state.bell_percent == 0)
				*(BOOL *) lpvParam = FALSE;
			else
				*(BOOL *) lpvParam = TRUE;
			break;
		
		case SPI_GETBORDER:
			*(int *) lpvParam = 1;
			break;

		case SPI_GETFASTTASKSWITCH:
			*(BOOL *) lpvParam = FALSE;
			break;

		case SPI_GETGRIDGRANULARITY:
			*(int *) lpvParam = 1;
			break;

		case SPI_GETICONTITLEWRAP:
			*(BOOL *) lpvParam = FALSE;
			break;

		case SPI_GETKEYBOARDDELAY:
			*(int *) lpvParam = 1;
			break;

		case SPI_GETKEYBOARDSPEED:
			*(WORD *) lpvParam = 30;
			break;

		case SPI_GETMENUDROPALIGNMENT:
			*(BOOL *) lpvParam = FALSE;
			break;

		case SPI_GETSCREENSAVEACTIVE:
			*(WORD *) lpvParam = FALSE;
			break;

		case SPI_GETSCREENSAVETIMEOUT:
			*(int *) lpvParam = 0;
			break;

		case SPI_ICONHORIZONTALSPACING:
			if (lpvParam == NULL)
				fprintf(stderr, "SystemParametersInfo: Horizontal icon spacing set to %d\n.", uParam);
			else
				*(int *) lpvParam = 50;
			break;

		case SPI_ICONVERTICALSPACING:
			if (lpvParam == NULL)
				fprintf(stderr, "SystemParametersInfo: Vertical icon spacing set to %d\n.", uParam);
			else
				*(int *) lpvParam = 50;
			break;

		case SPI_SETBEEP:
			if (uParam == TRUE)
				keyboard_value.bell_percent = -1;
			else
				keyboard_value.bell_percent = 0;			
   			XChangeKeyboardControl(display, KBBellPercent, 
   							&keyboard_value);
			break;

		case SPI_SETSCREENSAVEACTIVE:
			if (uParam == TRUE)
				XActivateScreenSaver(display);
			else
				XResetScreenSaver(display);
			break;

		case SPI_SETSCREENSAVETIMEOUT:
			XSetScreenSaver(display, uParam, 60, DefaultBlanking, 
						DefaultExposures);
			break;

		case SPI_LANGDRIVER:
		case SPI_SETBORDER:
		case SPI_SETDESKPATTERN:
		case SPI_SETDESKWALLPAPER:
		case SPI_SETDOUBLECLKHEIGHT:
		case SPI_SETDOUBLECLICKTIME:
		case SPI_SETDOUBLECLKWIDTH:
		case SPI_SETFASTTASKSWITCH:
		case SPI_SETKEYBOARDDELAY:
		case SPI_SETKEYBOARDSPEED:
			fprintf(stderr, "SystemParametersInfo: option %d ignored.\n", uParam);
			break;

		default:
			fprintf(stderr, "SystemParametersInfo: unknown option %d.\n", uParam);
			break;		
	}
	return 1;
}
