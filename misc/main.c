/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
/* #include <locale.h> */
#ifdef MALLOC_DEBUGGING
#include <malloc.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/Xlocale.h>
#include <X11/cursorfont.h>
#include "winsock.h"
#include "heap.h"
#include "message.h"
#include "msdos.h"
#include "windows.h"
#include "color.h"
#include "options.h"
#include "desktop.h"
#include "process.h"
#include "shell.h"
#include "winbase.h"
#define DEBUG_DEFINE_VARIABLES
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"
#include "version.h"


const WINE_LANGUAGE_DEF Languages[] =
{
    {"En",0x0409},	/* LANG_En */
    {"Es",0x040A},	/* LANG_Es */
    {"De",0x0407},	/* LANG_De */
    {"No",0x0414},	/* LANG_No */
    {"Fr",0x0400},	/* LANG_Fr */
    {"Fi",0x040B},	/* LANG_Fi */
    {"Da",0x0406},	/* LANG_Da */
    {"Cz",0x0405},	/* LANG_Cz */
    {"Eo",     0},	/* LANG_Eo */ /* FIXME languageid */
    {"It",0x0410},	/* LANG_It */
    {"Ko",0x0412},	/* LANG_Ko */
    {"Hu",0x0436},	/* LANG_Hu */
    {"Pl",0x0415},      /* LANG_Pl */
    {"Po",0x0416},      /* LANG_Po */
    {NULL,0}
};

WORD WINE_LanguageId = 0;

#define WINE_CLASS    "Wine"    /* Class name for resources */

#define WINE_APP_DEFAULTS "/usr/lib/X11/app-defaults/Wine"

Display *display;
Screen *screen;
Window rootWindow;
int screenWidth = 0, screenHeight = 0;  /* Desktop window dimensions */
int screenDepth = 0;  /* Screen depth to use */

struct options Options =
{  /* default options */
    NULL,           /* desktopGeometry */
    NULL,           /* programName */
    NULL,           /* argv0 */
    NULL,           /* dllFlags */
    FALSE,          /* usePrivateMap */
    FALSE,          /* useFixedMap */
    FALSE,          /* synchronous */
    FALSE,          /* backing store */
    SW_SHOWNORMAL,  /* cmdShow */
    FALSE,
    FALSE,          /* failReadOnly */
    MODE_ENHANCED,  /* Enhanced mode */
#ifdef DEFAULT_LANG
    DEFAULT_LANG,   /* Default language */
#else
    LANG_En,
#endif
    FALSE,          /* Managed windows */
    FALSE           /* Perfect graphics */
};


static XrmOptionDescRec optionsTable[] =
{
    { "-backingstore",  ".backingstore",    XrmoptionNoArg,  (caddr_t)"on" },
    { "-desktop",       ".desktop",         XrmoptionSepArg, (caddr_t)NULL },
    { "-depth",         ".depth",           XrmoptionSepArg, (caddr_t)NULL },
    { "-display",       ".display",         XrmoptionSepArg, (caddr_t)NULL },
    { "-iconic",        ".iconic",          XrmoptionNoArg,  (caddr_t)"on" },
    { "-language",      ".language",        XrmoptionSepArg, (caddr_t)"En" },
    { "-name",          ".name",            XrmoptionSepArg, (caddr_t)NULL },
    { "-perfect",       ".perfect",         XrmoptionNoArg,  (caddr_t)"on" },
    { "-privatemap",    ".privatemap",      XrmoptionNoArg,  (caddr_t)"on" },
    { "-fixedmap",      ".fixedmap",        XrmoptionNoArg,  (caddr_t)"on" },
    { "-synchronous",   ".synchronous",     XrmoptionNoArg,  (caddr_t)"on" },
    { "-debug",         ".debug",           XrmoptionNoArg,  (caddr_t)"on" },
    { "-debugmsg",      ".debugmsg",        XrmoptionSepArg, (caddr_t)NULL },
    { "-dll",           ".dll",             XrmoptionSepArg, (caddr_t)NULL },
    { "-failreadonly",  ".failreadonly",    XrmoptionNoArg,  (caddr_t)"on" },
    { "-mode",          ".mode",            XrmoptionSepArg, (caddr_t)NULL },
    { "-managed",       ".managed",         XrmoptionNoArg,  (caddr_t)"off"},
    { "-winver",        ".winver",          XrmoptionSepArg, (caddr_t)NULL }
};

#define NB_OPTIONS  (sizeof(optionsTable) / sizeof(optionsTable[0]))

#define USAGE \
  "Usage:  %s [options] \"program_name [arguments]\"\n" \
  "\n" \
  "Options:\n" \
  "    -backingstore   Turn on backing store\n" \
  "    -debug          Enter debugger before starting application\n" \
  "    -debugmsg name  Turn debugging-messages on or off\n" \
  "    -depth n        Change the depth to use for multiple-depth screens\n" \
  "    -desktop geom   Use a desktop window of the given geometry\n" \
  "    -display name   Use the specified display\n" \
  "    -dll name       Enable or disable built-in DLLs\n" \
  "    -failreadonly   Read only files may not be opened in write mode\n" \
  "    -fixedmap       Use a \"standard\" color map\n" \
  "    -iconic         Start as an icon\n" \
  "    -language xx    Set the language (one of En,Es,De,No,Fr,Fi,Da,Cz,Eo,It,Ko,\n                    Hu,Pl,Po)\n" \
  "    -managed        Allow the window manager to manage created windows\n" \
  "    -mode mode      Start Wine in a particular mode (standard or enhanced)\n" \
  "    -name name      Set the application name\n" \
  "    -perfect        Favor correctness over speed for graphical operations\n" \
  "    -privatemap     Use a private color map\n" \
  "    -synchronous    Turn on synchronous display mode\n" \
  "    -version        Display the Wine version\n" \
  "    -winver         Version to imitate (one of win31,win95,nt351,nt40)\n"



/***********************************************************************
 *           MAIN_Usage
 */
void MAIN_Usage( char *name )
{
    fprintf( stderr, USAGE, name );
    exit(1);
}


/***********************************************************************
 *           MAIN_GetProgramName
 *
 * Get the program name. The name is specified by (in order of precedence):
 * - the option '-name'.
 * - the environment variable 'WINE_NAME'.
 * - the last component of argv[0].
 */
static char *MAIN_GetProgramName( int argc, char *argv[] )
{
    int i;
    char *p;

    for (i = 1; i < argc-1; i++)
	if (!strcmp( argv[i], "-name" )) return argv[i+1];
    if ((p = getenv( "WINE_NAME" )) != NULL) return p;
    if ((p = strrchr( argv[0], '/' )) != NULL) return p+1;
    return argv[0];
}


/***********************************************************************
 *           MAIN_GetResource
 *
 * Fetch the value of resource 'name' using the correct instance name.
 * 'name' must begin with '.' or '*'
 */
static int MAIN_GetResource( XrmDatabase db, char *name, XrmValue *value )
{
    char *buff_instance, *buff_class;
    char *dummy;
    int retval;

    buff_instance = (char *)xmalloc(strlen(Options.programName)+strlen(name)+1);
    buff_class    = (char *)xmalloc( strlen(WINE_CLASS) + strlen(name) + 1 );

    strcpy( buff_instance, Options.programName );
    strcat( buff_instance, name );
    strcpy( buff_class, WINE_CLASS );
    strcat( buff_class, name );
    retval = XrmGetResource( db, buff_instance, buff_class, &dummy, value );
    free( buff_instance );
    free( buff_class );
    return retval;
}


/***********************************************************************
 *                    ParseDebugOptions
 *
 *  Turns specific debug messages on or off, according to "options".
 *  Returns TRUE if parsing was successful
 */
#ifdef DEBUG_RUNTIME

BOOL32 ParseDebugOptions(char *options)
{
  int l;
  if (strlen(options)<3)
    return FALSE;
  do
  {
    if ((*options!='+')&&(*options!='-'))
      return FALSE;
    if (strchr(options,','))
      l=strchr(options,',')-options;
    else
      l=strlen(options);
    if (!lstrncmpi32A(options+1,"all",l-1))
      {
	int i;
	for (i=0;i<sizeof(debug_msg_enabled)/sizeof(short);i++)
	  debug_msg_enabled[i]=(*options=='+');
      }
    else
      {
	int i;
	for (i=0;i<sizeof(debug_msg_enabled)/sizeof(short);i++)
	  if (debug_msg_name && (!lstrncmpi32A(options+1,debug_msg_name[i],l-1)))
	    {
	      debug_msg_enabled[i]=(*options=='+');
	      break;
	    }
	if (i==sizeof(debug_msg_enabled)/sizeof(short))
	  return FALSE;
      }
    options+=l;
  }
  while((*options==',')&&(*(++options)));
  if (*options)
    return FALSE;
  else
    return TRUE;
}

#endif

/***********************************************************************
 *           MAIN_ParseLanguageOption
 *
 * Parse -language option.
 */
static void MAIN_ParseLanguageOption( char *arg )
{
    const WINE_LANGUAGE_DEF *p = Languages;

    Options.language = LANG_En;  /* First language */
    for (;p->name;p++)
    {
        if (!lstrcmpi32A( p->name, arg ))
        {
	    WINE_LanguageId = p->langid;
	    return;
	}
        Options.language++;
    }
    fprintf( stderr, "Invalid language specified '%s'. Supported languages are: ", arg );
    for (p = Languages; p->name; p++) fprintf( stderr, "%s ", p->name );
    fprintf( stderr, "\n" );
    exit(1);
}


/***********************************************************************
 *           MAIN_ParseModeOption
 *
 * Parse -mode option.
 */
static void MAIN_ParseModeOption( char *arg )
{
    if (!lstrcmpi32A("enhanced", arg)) Options.mode = MODE_ENHANCED;
    else if (!lstrcmpi32A("standard", arg)) Options.mode = MODE_STANDARD;
    else
    {
        fprintf(stderr, "Invalid mode '%s' specified.\n", arg);
        fprintf(stderr, "Valid modes are: 'standard', 'enhanced' (default).\n");
	exit(1);
    }
}

/***********************************************************************
 *           MAIN_ParseOptions
 *
 * Parse command line options and open display.
 */
static void MAIN_ParseOptions( int *argc, char *argv[] )
{
    char *display_name = NULL;
    XrmValue value;
    XrmDatabase db = XrmGetFileDatabase(WINE_APP_DEFAULTS);
    int i;
    char *xrm_string;

    Options.programName = MAIN_GetProgramName( *argc, argv );
    Options.argv0 = argv[0];

      /* Get display name from command line */
    for (i = 1; i < *argc; i++)
    {
        if (!strcmp( argv[i], "-display" )) display_name = argv[i+1];
        if (!strcmp( argv[i], "-v" ) || !strcmp( argv[i], "-version" ))
        {
            printf( "%s\n", WINE_RELEASE_INFO );
            exit(0);
        }
    }

      /* Open display */

    if (display_name == NULL &&
	MAIN_GetResource( db, ".display", &value )) display_name = value.addr;

    if (!(display = XOpenDisplay( display_name )))
    {
	fprintf( stderr, "%s: Can't open display: %s\n",
		 argv[0], display_name ? display_name : "(none specified)" );
	exit(1);
    }

      /* Merge file and screen databases */
    if ((xrm_string = XResourceManagerString( display )) != NULL)
    {
        XrmDatabase display_db = XrmGetStringDatabase( xrm_string );
        XrmMergeDatabases( display_db, &db );
    }

      /* Parse command line */
    XrmParseCommand( &db, optionsTable, NB_OPTIONS,
		     Options.programName, argc, argv );

      /* Get all options */
    if (MAIN_GetResource( db, ".iconic", &value ))
	Options.cmdShow = SW_SHOWMINIMIZED;
    if (MAIN_GetResource( db, ".privatemap", &value ))
	Options.usePrivateMap = TRUE;
    if (MAIN_GetResource( db, ".fixedmap", &value ))
	Options.useFixedMap = TRUE;
    if (MAIN_GetResource( db, ".synchronous", &value ))
	Options.synchronous = TRUE;
    if (MAIN_GetResource( db, ".backingstore", &value ))
	Options.backingstore = TRUE;	
    if (MAIN_GetResource( db, ".debug", &value ))
	Options.debug = TRUE;
    if (MAIN_GetResource( db, ".failreadonly", &value ))
        Options.failReadOnly = TRUE;
    if (MAIN_GetResource( db, ".perfect", &value ))
	Options.perfectGraphics = TRUE;
    if (MAIN_GetResource( db, ".depth", &value))
	screenDepth = atoi( value.addr );
    if (MAIN_GetResource( db, ".desktop", &value))
	Options.desktopGeometry = value.addr;
    if (MAIN_GetResource( db, ".language", &value))
        MAIN_ParseLanguageOption( (char *)value.addr );
    if (MAIN_GetResource( db, ".managed", &value))
        Options.managed = TRUE;
    if (MAIN_GetResource( db, ".mode", &value))
        MAIN_ParseModeOption( (char *)value.addr );

#ifdef DEBUG_RUNTIME
    if (MAIN_GetResource( db, ".debugoptions", &value))
	ParseDebugOptions((char*)value.addr);
#endif
    if (MAIN_GetResource( db, ".debugmsg", &value))
      {
#ifndef DEBUG_RUNTIME
	fprintf(stderr,"%s: Option \"-debugmsg\" not implemented.\n" \
          "    Recompile with DEBUG_RUNTIME in include/stddebug.h defined.\n",
	  argv[0]);
	exit(1);
#else
	if (ParseDebugOptions((char*)value.addr)==FALSE)
	  {
	    int i;
	    fprintf(stderr,"%s: Syntax: -debugmsg +xxx,...  or -debugmsg -xxx,...\n",argv[0]);
	    fprintf(stderr,"Example: -debugmsg +all,-heap    turn on all messages except heap messages\n");
	    fprintf(stderr,"Available message types:\n");
	    fprintf(stderr,"%-9s ","all");
	    for(i=0;i<sizeof(debug_msg_enabled)/sizeof(short);i++)
	      if(debug_msg_name[i])
		fprintf(stderr,"%-9s%c",debug_msg_name[i],
			(((i+2)%8==0)?'\n':' '));
	    fprintf(stderr,"\n\n");
	    exit(1);
	  }
#endif
      }

      if (MAIN_GetResource( db, ".dll", &value))
      {
          /* Hack: store option value in Options to be retrieved */
          /* later on inside the emulator code. */
          if (!__winelib) Options.dllFlags = xstrdup((char *)value.addr);
          else
          {
              fprintf( stderr, "-dll not supported in Winelib\n" );
              exit(1);
          }
      }

      if (MAIN_GetResource( db, ".winver", &value))
          VERSION_ParseVersion( (char*)value.addr );
}


/***********************************************************************
 *           MAIN_CreateDesktop
 */
static void MAIN_CreateDesktop( int argc, char *argv[] )
{
    int x, y, flags;
    unsigned int width = 640, height = 480;  /* Default size = 640x480 */
    char *name = "Wine desktop";
    XSizeHints *size_hints;
    XWMHints   *wm_hints;
    XClassHint *class_hints;
    XSetWindowAttributes win_attr;
    XTextProperty window_name;
    Atom XA_WM_DELETE_WINDOW;

    flags = XParseGeometry( Options.desktopGeometry, &x, &y, &width, &height );
    screenWidth  = width;
    screenHeight = height;

      /* Create window */

    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	                 PointerMotionMask | ButtonPressMask |
			 ButtonReleaseMask | EnterWindowMask;
    win_attr.cursor = XCreateFontCursor( display, XC_top_left_arrow );

    rootWindow = XCreateWindow( display, DefaultRootWindow(display),
			        x, y, width, height, 0,
			        CopyFromParent, InputOutput, CopyFromParent,
			        CWEventMask | CWCursor, &win_attr );

      /* Set window manager properties */

    size_hints  = XAllocSizeHints();
    wm_hints    = XAllocWMHints();
    class_hints = XAllocClassHint();
    if (!size_hints || !wm_hints || !class_hints)
    {
        fprintf( stderr, "Not enough memory for window manager hints.\n" );
        exit(1);
    }
    size_hints->min_width = size_hints->max_width = width;
    size_hints->min_height = size_hints->max_height = height;
    size_hints->flags = PMinSize | PMaxSize;
    if (flags & (XValue | YValue)) size_hints->flags |= USPosition;
    if (flags & (WidthValue | HeightValue)) size_hints->flags |= USSize;
    else size_hints->flags |= PSize;

    wm_hints->flags = InputHint | StateHint;
    wm_hints->input = True;
    wm_hints->initial_state = NormalState;
    class_hints->res_name = argv[0];
    class_hints->res_class = "Wine";

    XStringListToTextProperty( &name, 1, &window_name );
    XSetWMProperties( display, rootWindow, &window_name, &window_name,
                      argv, argc, size_hints, wm_hints, class_hints );
    XA_WM_DELETE_WINDOW = XInternAtom( display, "WM_DELETE_WINDOW", False );
    XSetWMProtocols( display, rootWindow, &XA_WM_DELETE_WINDOW, 1 );
    XFree( size_hints );
    XFree( wm_hints );
    XFree( class_hints );

      /* Map window */

    XMapWindow( display, rootWindow );
}


XKeyboardState keyboard_state;

/***********************************************************************
 *           MAIN_SaveSetup
 */
static void MAIN_SaveSetup(void)
{
    XGetKeyboardControl(display, &keyboard_state);
}

/***********************************************************************
 *           MAIN_RestoreSetup
 */
static void MAIN_RestoreSetup(void)
{
    XKeyboardControl keyboard_value;

    keyboard_value.key_click_percent	= keyboard_state.key_click_percent;
    keyboard_value.bell_percent 	= keyboard_state.bell_percent;
    keyboard_value.bell_pitch		= keyboard_state.bell_pitch;
    keyboard_value.bell_duration	= keyboard_state.bell_duration;
    keyboard_value.auto_repeat_mode	= keyboard_state.global_auto_repeat;

    XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent | 
    	KBBellPitch | KBBellDuration | KBAutoRepeatMode, &keyboard_value);
}


static void called_at_exit(void)
{
    MAIN_RestoreSetup();
    COLOR_Cleanup();
    WINSOCK_Shutdown();
    /* FIXME: should check for other processes or threads */
    DeleteCriticalSection( HEAP_SystemLock );
}

/***********************************************************************
 *           MAIN_WineInit
 *
 * Wine initialisation and command-line parsing
 */
BOOL32 MAIN_WineInit( int *argc, char *argv[] )
{    
    int depth_count, i;
    int *depth_list;
    struct timeval tv;

#ifdef MALLOC_DEBUGGING
    char *trace;

    mcheck(NULL);
    if (!(trace = getenv("MALLOC_TRACE")))
    {       
        fprintf( stderr, "MALLOC_TRACE not set. No trace generated\n" );
    }
    else
    {
        fprintf( stderr, "malloc trace goes to %s\n", trace );
        mtrace();
    }
#endif

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    setlocale(LC_CTYPE,"");
    gettimeofday( &tv, NULL);
    MSG_WineStartTicks = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    XrmInitialize();

    putenv("XKB_DISABLE="); /* Disable XKB extension if present. */

    MAIN_ParseOptions( argc, argv );

    if (Options.desktopGeometry && Options.managed)
    {
        fprintf( stderr, "%s: -managed and -desktop options cannot be used together\n",
                 Options.programName );
        exit(1);
    }

    screen       = DefaultScreenOfDisplay( display );
    screenWidth  = WidthOfScreen( screen );
    screenHeight = HeightOfScreen( screen );
    if (screenDepth)  /* -depth option specified */
    {
	depth_list = XListDepths(display,DefaultScreen(display),&depth_count);
	for (i = 0; i < depth_count; i++)
	    if (depth_list[i] == screenDepth) break;
	XFree( depth_list );
	if (i >= depth_count)
	{
	    fprintf( stderr, "%s: Depth %d not supported on this screen.\n",
		              Options.programName, screenDepth );
	    exit(1);
	}
    }
    else screenDepth  = DefaultDepthOfScreen( screen );
    if (Options.synchronous) XSynchronize( display, True );
    if (Options.desktopGeometry) MAIN_CreateDesktop( *argc, argv );
    else rootWindow = DefaultRootWindow( display );

    MAIN_SaveSetup();
    atexit(called_at_exit);
    return TRUE;
}


/***********************************************************************
 *           MessageBeep16   (USER.104)
 */
void WINAPI MessageBeep16( UINT16 i )
{
    MessageBeep32( i );
}


/***********************************************************************
 *           MessageBeep32   (USER32.389)
 */
BOOL32 WINAPI MessageBeep32( UINT32 i )
{
    XBell( display, 0 );
    return TRUE;
}


/***********************************************************************
 *           Beep   (KERNEL32.11)
 */
BOOL32 WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    /* dwFreq and dwDur are ignored by Win95 */
    XBell(display, 0);
    return TRUE;
}


/***********************************************************************
 *	GetTimerResolution (USER.14)
 */
LONG WINAPI GetTimerResolution(void)
{
	return (1000);
}

/***********************************************************************
 *	SystemParametersInfo32A   (USER32.539)
 */
BOOL32 WINAPI SystemParametersInfo32A( UINT32 uAction, UINT32 uParam,
                                       LPVOID lpvParam, UINT32 fuWinIni )
{
	int timeout, temp;
	XKeyboardState		keyboard_state;

	switch (uAction) {
	case SPI_GETBEEP:
		XGetKeyboardControl(display, &keyboard_state);
		if (keyboard_state.bell_percent == 0)
			*(BOOL32 *) lpvParam = FALSE;
		else
			*(BOOL32 *) lpvParam = TRUE;
		break;

	case SPI_GETBORDER:
		*(INT32 *)lpvParam = GetSystemMetrics32( SM_CXFRAME );
		break;

	case SPI_GETFASTTASKSWITCH:
		if ( GetProfileInt32A( "windows", "CoolSwitch", 1 ) == 1 )
			*(BOOL32 *) lpvParam = TRUE;
		else
			*(BOOL32 *) lpvParam = FALSE;
		break;

	case SPI_GETGRIDGRANULARITY:
		*(INT32*)lpvParam=GetProfileInt32A("desktop","GridGranularity",1);
		break;

	case SPI_GETICONTITLEWRAP:
		*(BOOL32*)lpvParam=GetProfileInt32A("desktop","IconTitleWrap",TRUE);
		break;

	case SPI_GETKEYBOARDDELAY:
		*(INT32*)lpvParam=GetProfileInt32A("keyboard","KeyboardDelay",1);
		break;

	case SPI_GETKEYBOARDSPEED:
		*(DWORD*)lpvParam=GetProfileInt32A("keyboard","KeyboardSpeed",30);
		break;

	case SPI_GETMENUDROPALIGNMENT:
		*(BOOL32*)lpvParam=GetSystemMetrics32(SM_MENUDROPALIGNMENT); /* XXX check this */
		break;

	case SPI_GETSCREENSAVEACTIVE:
		if ( GetProfileInt32A( "windows", "ScreenSaveActive", 1 ) == 1 )
			*(BOOL32*)lpvParam = TRUE;
		else
			*(BOOL32*)lpvParam = FALSE;
		break;

	case SPI_GETSCREENSAVETIMEOUT:
	/* FIXME GetProfileInt( "windows", "ScreenSaveTimeout", 300 ); */
		XGetScreenSaver(display, &timeout, &temp,&temp,&temp);
		*(INT32 *) lpvParam = timeout * 1000;
		break;

	case SPI_ICONHORIZONTALSPACING:
		/* FIXME Get/SetProfileInt */
		if (lpvParam == NULL)
			/*SetSystemMetrics( SM_CXICONSPACING, uParam )*/ ;
		else
			*(INT32*)lpvParam=GetSystemMetrics32(SM_CXICONSPACING);
		break;

	case SPI_ICONVERTICALSPACING:
		/* FIXME Get/SetProfileInt */
		if (lpvParam == NULL)
			/*SetSystemMetrics( SM_CYICONSPACING, uParam )*/ ;
		else
			*(INT32*)lpvParam=GetSystemMetrics32(SM_CYICONSPACING);
		break;

	case SPI_GETICONTITLELOGFONT: {
		LPLOGFONT32A lpLogFont = (LPLOGFONT32A)lpvParam;

		/* from now on we always have an alias for MS Sans Serif */

		GetProfileString32A("Desktop", "IconTitleFaceName", "MS Sans Serif", 
			lpLogFont->lfFaceName, LF_FACESIZE );
		lpLogFont->lfHeight = -GetProfileInt32A("Desktop","IconTitleSize", 8);

		lpLogFont->lfWidth = 0;
		lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
		lpLogFont->lfWeight = FW_NORMAL;
		lpLogFont->lfItalic = FALSE;
		lpLogFont->lfStrikeOut = FALSE;
		lpLogFont->lfUnderline = FALSE;
		lpLogFont->lfCharSet = ANSI_CHARSET;
		lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
		lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
		break;
	}
	case SPI_GETWORKAREA:
		SetRect32( (RECT32 *)lpvParam, 0, 0,
			GetSystemMetrics32( SM_CXSCREEN ),
			GetSystemMetrics32( SM_CYSCREEN )
		);
		break;
	case SPI_GETNONCLIENTMETRICS: 

#define lpnm ((LPNONCLIENTMETRICS32A)lpvParam)
		
		if( lpnm->cbSize == sizeof(NONCLIENTMETRICS32A) )
		{
		    /* FIXME: initialize geometry entries */

		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfCaptionFont),0);
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMenuFont),0);
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfStatusFont),0);
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMessageFont),0);
		}
#undef lpnm
		break;

	default:
		return SystemParametersInfo16(uAction,uParam,lpvParam,fuWinIni);
	}
	return TRUE;
}


/***********************************************************************
 *	SystemParametersInfo16   (USER.483)
 */
BOOL16 WINAPI SystemParametersInfo16( UINT16 uAction, UINT16 uParam,
                                      LPVOID lpvParam, UINT16 fuWinIni )
{
	int timeout, temp;
	char buffer[256];
	XKeyboardState		keyboard_state;
	XKeyboardControl	keyboard_value;


	switch (uAction)
        {
		case SPI_GETBEEP:
			XGetKeyboardControl(display, &keyboard_state);
			if (keyboard_state.bell_percent == 0)
				*(BOOL16 *) lpvParam = FALSE;
			else
				*(BOOL16 *) lpvParam = TRUE;
			break;
		
		case SPI_GETBORDER:
			*(INT16 *)lpvParam = GetSystemMetrics16( SM_CXFRAME );
			break;

		case SPI_GETFASTTASKSWITCH:
		    if ( GetProfileInt32A( "windows", "CoolSwitch", 1 ) == 1 )
			  *(BOOL16 *) lpvParam = TRUE;
			else
			  *(BOOL16 *) lpvParam = FALSE;
			break;

		case SPI_GETGRIDGRANULARITY:
                    *(INT16 *) lpvParam = GetProfileInt32A( "desktop", 
                                                          "GridGranularity",
                                                          1 );
                    break;

                case SPI_GETICONTITLEWRAP:
                    *(BOOL16 *) lpvParam = GetProfileInt32A( "desktop",
                                                           "IconTitleWrap",
                                                           TRUE );
                    break;

                case SPI_GETKEYBOARDDELAY:
                    *(INT16 *) lpvParam = GetProfileInt32A( "keyboard",
                                                          "KeyboardDelay", 1 );
                    break;

                case SPI_GETKEYBOARDSPEED:
                    *(WORD *) lpvParam = GetProfileInt32A( "keyboard",
                                                           "KeyboardSpeed",
                                                           30 );
                    break;

		case SPI_GETMENUDROPALIGNMENT:
			*(BOOL16 *) lpvParam = GetSystemMetrics16( SM_MENUDROPALIGNMENT ); /* XXX check this */
			break;

		case SPI_GETSCREENSAVEACTIVE:
                    if ( GetProfileInt32A( "windows", "ScreenSaveActive", 1 ) == 1 )
                        *(BOOL16 *) lpvParam = TRUE;
                    else
                        *(BOOL16 *) lpvParam = FALSE;
                    break;

		case SPI_GETSCREENSAVETIMEOUT:
			/* FIXME GetProfileInt( "windows", "ScreenSaveTimeout", 300 ); */
			XGetScreenSaver(display, &timeout, &temp,&temp,&temp);
			*(INT16 *) lpvParam = timeout * 1000;
			break;

		case SPI_ICONHORIZONTALSPACING:
                    /* FIXME Get/SetProfileInt */
			if (lpvParam == NULL)
                            /*SetSystemMetrics( SM_CXICONSPACING, uParam )*/ ;
                        else
                            *(INT16 *)lpvParam = GetSystemMetrics16( SM_CXICONSPACING );
			break;

		case SPI_ICONVERTICALSPACING:
                    /* FIXME Get/SetProfileInt */
                    if (lpvParam == NULL)
                        /*SetSystemMetrics( SM_CYICONSPACING, uParam )*/ ;
		    else
                        *(INT16 *)lpvParam = GetSystemMetrics16(SM_CYICONSPACING);
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

		case SPI_SETDESKWALLPAPER:
			return (SetDeskWallPaper32((LPSTR) lpvParam));
			break;

		case SPI_SETDESKPATTERN:
			if ((INT16)uParam == -1) {
				GetProfileString32A("Desktop", "Pattern", 
						"170 85 170 85 170 85 170 85", 
						buffer, sizeof(buffer) );
				return (DESKTOP_SetPattern((LPSTR) buffer));
			} else
				return (DESKTOP_SetPattern((LPSTR) lpvParam));
			break;

	        case SPI_GETICONTITLELOGFONT: 
	        {
                    LPLOGFONT16 lpLogFont = (LPLOGFONT16)lpvParam;

		    GetProfileString32A("Desktop", "IconTitleFaceName", "MS Sans Serif", 
					lpLogFont->lfFaceName, LF_FACESIZE );
                    lpLogFont->lfHeight = -GetProfileInt32A("Desktop","IconTitleSize", 8);

                    lpLogFont->lfWidth = 0;
                    lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
                    lpLogFont->lfWeight = FW_NORMAL;
                    lpLogFont->lfItalic = FALSE;
                    lpLogFont->lfStrikeOut = FALSE;
                    lpLogFont->lfUnderline = FALSE;
                    lpLogFont->lfCharSet = ANSI_CHARSET;
                    lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
                    lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
                    lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
                    break;
                }
		case SPI_GETNONCLIENTMETRICS:

#define lpnm ((LPNONCLIENTMETRICS16)lpvParam)
		    if( lpnm->cbSize == sizeof(NONCLIENTMETRICS16) )
		    {
			/* FIXME: initialize geometry entries */
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0, 
							(LPVOID)&(lpnm->lfCaptionFont),0);
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMenuFont),0);
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfStatusFont),0);
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMessageFont),0);
		    }
		    else /* winfile 95 sets sbSize to 340 */
		        SystemParametersInfo32A( uAction, uParam, lpvParam, fuWinIni );
#undef lpnm
		    break;

		case SPI_LANGDRIVER:
		case SPI_SETBORDER:
		case SPI_SETDOUBLECLKHEIGHT:
		case SPI_SETDOUBLECLICKTIME:
		case SPI_SETDOUBLECLKWIDTH:
		case SPI_SETFASTTASKSWITCH:
		case SPI_SETKEYBOARDDELAY:
		case SPI_SETKEYBOARDSPEED:
	        case SPI_GETHIGHCONTRAST:
			fprintf(stderr, "SystemParametersInfo: option %d ignored.\n", uAction);
			break;

                case SPI_GETWORKAREA:
                    SetRect16( (RECT16 *)lpvParam, 0, 0,
                               GetSystemMetrics16( SM_CXSCREEN ),
                               GetSystemMetrics16( SM_CYSCREEN ) );
                    break;

		default:
			fprintf(stderr, "SystemParametersInfo: unknown option %d.\n", uAction);
			break;
	}
	return 1;
}

/***********************************************************************
 *	SystemParametersInfo32W   (USER32.540)
 */
BOOL32 WINAPI SystemParametersInfo32W( UINT32 uAction, UINT32 uParam,
                                       LPVOID lpvParam, UINT32 fuWinIni )
{
    char buffer[256];

    switch (uAction)
    {
    case SPI_SETDESKWALLPAPER:
        if (lpvParam)
        {
            lstrcpynWtoA(buffer,(LPWSTR)lpvParam,sizeof(buffer));
            return SetDeskWallPaper32(buffer);
        }
        return SetDeskWallPaper32(NULL);

    case SPI_SETDESKPATTERN:
        if ((INT32) uParam == -1)
        {
            GetProfileString32A("Desktop", "Pattern", 
                                "170 85 170 85 170 85 170 85", 
                                buffer, sizeof(buffer) );
            return (DESKTOP_SetPattern((LPSTR) buffer));
        }
        if (lpvParam)
        {
            lstrcpynWtoA(buffer,(LPWSTR)lpvParam,sizeof(buffer));
            return DESKTOP_SetPattern(buffer);
        }
        return DESKTOP_SetPattern(NULL);

    case SPI_GETICONTITLELOGFONT:
        {
            /* FIXME GetProfileString32A( "?", "?", "?" ) */
            LPLOGFONT32W lpLogFont = (LPLOGFONT32W)lpvParam;
            lpLogFont->lfHeight = 10;
            lpLogFont->lfWidth = 0;
            lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
            lpLogFont->lfWeight = FW_NORMAL;
            lpLogFont->lfItalic = lpLogFont->lfStrikeOut = lpLogFont->lfUnderline = FALSE;
            lpLogFont->lfCharSet = ANSI_CHARSET;
            lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
            lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
            lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
        }
        break;
    case SPI_GETNONCLIENTMETRICS: {
	/* FIXME: implement correctly */
	LPNONCLIENTMETRICS32W	lpnm=(LPNONCLIENTMETRICS32W)lpvParam;

	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfCaptionFont),0);
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfMenuFont),0);
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfStatusFont),0);
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfMessageFont),0);
	break;
    }

    default:
        return SystemParametersInfo32A(uAction,uParam,lpvParam,fuWinIni);
	
    }
    return TRUE;
}


/***********************************************************************
*	FileCDR (KERNEL.130)
*/
void WINAPI FileCDR(FARPROC16 x)
{
	printf("FileCDR(%8x)\n", (int) x);
}
