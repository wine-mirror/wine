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
#include <locale.h>
#ifdef MALLOC_DEBUGGING
#include <malloc.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xresource.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "message.h"
#include "module.h"
#include "msdos.h"
#include "windows.h"
#include "winsock.h"
#include "options.h"
#include "desktop.h"
#include "registers.h"
#include "shell.h"
#include "winbase.h"
#include "string32.h"
#define DEBUG_DEFINE_VARIABLES
#include "stddebug.h"
#include "debug.h"
#include "xmalloc.h"

const char people[] = "Wine is available thanks to the work of "
"Bob Amstadt, Dag Asheim, Martin Ayotte, Ross Biro, Uwe Bonnes, Erik Bos, "
"Fons Botman, John Brezak, Andrew Bulhak, John Burton, "
"Niels de Carpentier, Roman Dolejsi, Frans van Dorsselaer, Paul Falstad, "
"Olaf Flebbe, Peter Galbavy, Ramon Garcia, Hans de Graaff, "
"Charles M. Hannum, John Harvey, Cameron Heide, Jochen Hoenicke, "
"Onno Hovers, Jeffrey Hsu, Miguel de Icaza, Jukka Iivonen, "
"Alexandre Julliard, Jochen Karrer, Andreas Kirschbaum, Albrecht Kleine, "
"Jon Konrath, Alex Korobka, Greg Kreider, Anand Kumria, Scott A. Laird, "
"Martin von Loewis, Kenneth MacDonald, Peter MacDonald, William Magro, "
"Juergen Marquardt, Marcus Meissner, Graham Menhennitt, David Metcalfe, "
"Steffen Moeller, Philippe De Muyter, Itai Nahshon, Michael Patra, "
"Jim Peterson, Robert Pouliot, Keith Reynolds, John Richardson, "
"Johannes Ruscheinski, Thomas Sandford, Constantine Sapuntzakis, "
"Daniel Schepler, Ulrich Schmid, Bernd Schmidt, Yngvi Sigurjonsson, "
"Rick Sladkey, William Smith, Erik Svendsen, Tristan Tarrant, "
"Andrew Taylor, Duncan C Thomson, Goran Thyni, Jimmy Tirtawangsa, "
"Jon Tombs, Linus Torvalds, Gregory Trubetskoy, Michael Veksler, "
"Sven Verdoolaege, Eric Warnke, Manfred Weichel, Morten Welinder, "
"Jan Willamowius, Carl Williams, Karl Guenter Wuensch, Eric Youngdale, "
"and James Youngman. ";

const char * langNames[] =
{
    "En",  /* LANG_En */
    "Es",  /* LANG_Es */
    "De",  /* LANG_De */
    "No",  /* LANG_No */
    "Fr",  /* LANG_Fr */
    "Fi",  /* LANG_Fi */
    "Da",  /* LANG_Da */
    "Cz",  /* LANG_Cz */
    "Eo",  /* LANG_Eo */
    "It",  /* LANG_It */
    "Ko",  /* LANG_Ko */
    NULL
};

#define WINE_CLASS    "Wine"    /* Class name for resources */

typedef struct tagENVENTRY {
  LPSTR	       	        Name;
  LPSTR	       	        Value;
  WORD	       	        wSize;
  struct tagENVENTRY    *Prev;
  struct tagENVENTRY    *Next;
} ENVENTRY, *LPENVENTRY;

LPENVENTRY	lpEnvList = NULL;

Display *display;
Screen *screen;
Window rootWindow;
int screenWidth = 0, screenHeight = 0;  /* Desktop window dimensions */
int screenDepth = 0;  /* Screen depth to use */
int desktopX = 0, desktopY = 0;  /* Desktop window position (if any) */
int getVersion16 = 0;
int getVersion32 = 0;
OSVERSIONINFO32A getVersionEx;

struct options Options =
{  /* default options */
    NULL,           /* desktopGeometry */
    NULL,           /* programName */
    FALSE,          /* usePrivateMap */
    FALSE,          /* useFixedMap */
    FALSE,          /* synchronous */
    FALSE,          /* backing store */
    SW_SHOWNORMAL,  /* cmdShow */
    FALSE,
    FALSE,          /* AllowReadOnly */
    MODE_ENHANCED,  /* Enhanced mode */
    FALSE,          /* IPC enabled */
#ifdef DEFAULT_LANG
    DEFAULT_LANG,   /* Default language */
#else
    LANG_En,
#endif
    FALSE	    /* Managed windows */
};


static XrmOptionDescRec optionsTable[] =
{
    { "-backingstore",  ".backingstore",    XrmoptionNoArg,  (caddr_t)"on" },
    { "-desktop",       ".desktop",         XrmoptionSepArg, (caddr_t)NULL },
    { "-depth",         ".depth",           XrmoptionSepArg, (caddr_t)NULL },
    { "-display",       ".display",         XrmoptionSepArg, (caddr_t)NULL },
    { "-iconic",        ".iconic",          XrmoptionNoArg,  (caddr_t)"on" },
    { "-ipc",           ".ipc",             XrmoptionNoArg,  (caddr_t)"off"},
    { "-language",      ".language",        XrmoptionSepArg, (caddr_t)"En" },
    { "-name",          ".name",            XrmoptionSepArg, (caddr_t)NULL },
    { "-privatemap",    ".privatemap",      XrmoptionNoArg,  (caddr_t)"on" },
    { "-fixedmap",      ".fixedmap",        XrmoptionNoArg,  (caddr_t)"on" },
    { "-synchronous",   ".synchronous",     XrmoptionNoArg,  (caddr_t)"on" },
    { "-debug",         ".debug",           XrmoptionNoArg,  (caddr_t)"on" },
    { "-debugmsg",      ".debugmsg",        XrmoptionSepArg, (caddr_t)NULL },
    { "-dll",           ".dll",             XrmoptionSepArg, (caddr_t)NULL },
    { "-allowreadonly", ".allowreadonly",   XrmoptionNoArg,  (caddr_t)"on" },
    { "-mode",          ".mode",            XrmoptionSepArg, (caddr_t)NULL },
    { "-managed",       ".managed",         XrmoptionNoArg,  (caddr_t)"off"},
    { "-winver",        ".winver",          XrmoptionSepArg, (caddr_t)NULL }
};

#define NB_OPTIONS  (sizeof(optionsTable) / sizeof(optionsTable[0]))

#define USAGE \
  "Usage:  %s [options] program_name [arguments]\n" \
  "\n" \
  "Options:\n" \
  "    -allowreadonly  Read only files may be opened in write mode\n" \
  "    -backingstore   Turn on backing store\n" \
  "    -debug          Enter debugger before starting application\n" \
  "    -debugmsg name  Turn debugging-messages on or off\n" \
  "    -depth n        Change the depth to use for multiple-depth screens\n" \
  "    -desktop geom   Use a desktop window of the given geometry\n" \
  "    -display name   Use the specified display\n" \
  "    -dll name       Enable or disable built-in DLLs\n" \
  "    -fixedmap       Use a \"standard\" color map\n" \
  "    -iconic         Start as an icon\n" \
  "    -ipc            Enable IPC facilities\n" \
  "    -language xx    Set the language (one of En,Es,De,No,Fr,Fi,Da,Cz,Eo,It,Ko)\n" \
  "    -managed        Allow the window manager to manage created windows\n" \
  "    -mode mode      Start Wine in a particular mode (standard or enhanced)\n" \
  "    -name name      Set the application name\n" \
  "    -privatemap     Use a private color map\n" \
  "    -synchronous    Turn on synchronous display mode\n" \
  "    -winver         Version to imitate (one of win31,win95,nt351)\n"



/***********************************************************************
 *           MAIN_Usage
 */
#ifndef WINELIB32
static void MAIN_Usage( char *name )
{
    fprintf( stderr, USAGE, name );
    exit(1);
}
#endif


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

BOOL ParseDebugOptions(char *options)
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
    const char **p = langNames;

    Options.language = LANG_En;  /* First language */
    for (p = langNames; *p; p++)
    {
        if (!lstrcmpi32A( *p, arg )) return;
        Options.language++;
    }
    fprintf( stderr, "Invalid language specified '%s'. Supported languages are: ", arg );
    for (p = langNames; *p; p++) fprintf( stderr, "%s ", *p );
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

/**********************************************************************
 *           MAIN_ParseVersion
 */
static void MAIN_ParseVersion( char *arg )
{
    /* If you add any other options, 
       verify the values you return on the real thing */
    if(strcmp(arg,"win31")==0) 
    {
        getVersion16 = 0x06160A03;
        /* FIXME: My Win32s installation failed to execute the
           MSVC 4 test program. So check these values */
        getVersion32 = 0x80000A03;
        getVersionEx.dwMajorVersion=3;
        getVersionEx.dwMinorVersion=10;
        getVersionEx.dwBuildNumber=0;
        getVersionEx.dwPlatformId=VER_PLATFORM_WIN32s;
        strcpy(getVersionEx.szCSDVersion,"Win32s 1.3");
    }
    else if(strcmp(arg, "win95")==0)
    {
        getVersion16 = 0x07005F03;
        getVersion32 = 0xC0000004;
        getVersionEx.dwMajorVersion=4;
        getVersionEx.dwMinorVersion=0;
        getVersionEx.dwBuildNumber=0x40003B6;
        getVersionEx.dwPlatformId=VER_PLATFORM_WIN32_WINDOWS;
        strcpy(getVersionEx.szCSDVersion,"");
    }
    else if(strcmp(arg, "nt351")==0)
    {
        getVersion16 = 0x05000A03;
        getVersion32 = 0x04213303;
        getVersionEx.dwMajorVersion=3;
        getVersionEx.dwMinorVersion=51;
        getVersionEx.dwBuildNumber=0x421;
        getVersionEx.dwPlatformId=VER_PLATFORM_WIN32_NT;
        strcpy(getVersionEx.szCSDVersion,"Service Pack 2");
    }
    else fprintf(stderr, "Unknown winver system code - ignored\n");
}

/***********************************************************************
 *           MAIN_ParseOptions
 *
 * Parse command line options and open display.
 */
static void MAIN_ParseOptions( int *argc, char *argv[] )
{
    char *display_name;
    XrmValue value;
    XrmDatabase db = XrmGetFileDatabase("/usr/lib/X11/app-defaults/Wine");

      /* Parse command line */
    Options.programName = MAIN_GetProgramName( *argc, argv );
    XrmParseCommand( &db, optionsTable, NB_OPTIONS,
		     Options.programName, argc, argv );

#ifdef WINELIB
    /* Need to assemble command line and pass it to WinMain */
#else
    if (*argc < 2 || lstrcmpi32A(argv[1], "-h") == 0) 
    	MAIN_Usage( argv[0] );
#endif

      /* Open display */

    if (MAIN_GetResource( db, ".display", &value )) display_name = value.addr;
    else display_name = NULL;

    if (!(display = XOpenDisplay( display_name )))
    {
	fprintf( stderr, "%s: Can't open display: %s\n",
		 argv[0], display_name ? display_name : "(none specified)" );
	exit(1);
    }

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
    if (MAIN_GetResource( db, ".allowreadonly", &value ))
        Options.allowReadOnly = TRUE;
    if (MAIN_GetResource( db, ".ipc", &value ))
        Options.ipc = TRUE;
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

      if(MAIN_GetResource( db, ".dll", &value))
      {
#ifndef WINELIB
          if (!BUILTIN_ParseDLLOptions( (char*)value.addr ))
          {
              fprintf(stderr,"%s: Syntax: -dll +xxx,... or -dll -xxx,...\n",argv[0]);
              fprintf(stderr,"Example: -dll -ole2    Do not use emulated OLE2.DLL\n");
              fprintf(stderr,"Available DLLs:\n");
              BUILTIN_PrintDLLs();
              exit(1);
          }
#else
          fprintf(stderr,"-dll not supported in libwine\n");
#endif
      }

      if(MAIN_GetResource( db, ".winver", &value))
          MAIN_ParseVersion( (char*)value.addr );
}


/***********************************************************************
 *           MAIN_CreateDesktop
 */
static void MAIN_CreateDesktop( int argc, char *argv[] )
{
    int flags;
    unsigned int width = 640, height = 480;  /* Default size = 640x480 */
    char *name = "Wine desktop";
    XSizeHints *size_hints;
    XWMHints   *wm_hints;
    XClassHint *class_hints;
    XSetWindowAttributes win_attr;
    XTextProperty window_name;
    Atom XA_WM_DELETE_WINDOW;

    flags = XParseGeometry( Options.desktopGeometry,
			    &desktopX, &desktopY, &width, &height );
    screenWidth  = width;
    screenHeight = height;

      /* Create window */

    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	                 PointerMotionMask | ButtonPressMask |
			 ButtonReleaseMask | EnterWindowMask | 
			 StructureNotifyMask;
    win_attr.cursor = XCreateFontCursor( display, XC_top_left_arrow );

    rootWindow = XCreateWindow( display, DefaultRootWindow(display),
			        desktopX, desktopY, width, height, 0,
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
    WSACleanup();
}

/***********************************************************************
 *           main
 */
#if defined(WINELIB) && defined(WINELIBDLL)
int _wine_main (int argc, char *argv[])
#else
int main( int argc, char *argv[] )
#endif
{    
    int ret_val;
    int depth_count, i;
    int *depth_list;
    struct timeval tv;

    extern int _WinMain(int argc, char **argv);

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
    
    MAIN_ParseOptions( &argc, argv );

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
    if (Options.desktopGeometry) MAIN_CreateDesktop( argc, argv );
    else rootWindow = DefaultRootWindow( display );

    MAIN_SaveSetup();
    atexit(called_at_exit);

    ret_val = _WinMain( argc, argv );

    return ret_val;
}


/***********************************************************************
 *           MessageBeep    (USER.104)
 */
void MessageBeep(WORD i)
{
	XBell(display, 100);
}


/***********************************************************************
 *           Beep   (KERNEL32.11)
 */
BOOL32 Beep( DWORD dwFreq, DWORD dwDur )
{
    /* dwFreq and dwDur are ignored by Win95 */
    XBell(display, 100);
    return TRUE;
}


/***********************************************************************
 *      GetVersion (KERNEL.3)
 */
LONG GetVersion(void)
{
    if (getVersion16) return getVersion16;
    return MAKELONG( WINVERSION, WINDOSVER );
}


/***********************************************************************
 *      GetVersion32
 */
LONG GetVersion32(void)
{
    if (getVersion32) return getVersion32;
    return MAKELONG( 4, DOSVERSION);
}


/***********************************************************************
 *      GetVersionExA
 */
BOOL32 GetVersionEx32A(OSVERSIONINFO32A *v)
{
    if(v->dwOSVersionInfoSize!=sizeof(OSVERSIONINFO32A))
    {
        fprintf(stddeb,"wrong OSVERSIONINFO size from app");
        return FALSE;
    }
    if(!getVersion32)
    {
        /* Return something like NT 3.5 */
        v->dwMajorVersion = 3;
        v->dwMinorVersion = 5;
        v->dwBuildNumber = 42;
        v->dwPlatformId = VER_PLATFORM_WIN32_NT;
        strcpy(v->szCSDVersion, "Wine is not an emulator");
        return TRUE;
    }
    v->dwMajorVersion = getVersionEx.dwMajorVersion;
    v->dwMinorVersion = getVersionEx.dwMinorVersion;
    v->dwBuildNumber = getVersionEx.dwBuildNumber;
    v->dwPlatformId = getVersionEx.dwPlatformId;
    strcpy(v->szCSDVersion, getVersionEx.szCSDVersion);
    return TRUE;
}


/***********************************************************************
 *     GetVersionExW
 */
BOOL32 GetVersionEx32W(OSVERSIONINFO32W *v)
{
	OSVERSIONINFO32A v1;
	if(v->dwOSVersionInfoSize!=sizeof(OSVERSIONINFO32W))
	{
		fprintf(stddeb,"wrong OSVERSIONINFO size from app");
		return FALSE;
	}
	v1.dwOSVersionInfoSize=sizeof(v1);
	GetVersionEx32A(&v1);
	v->dwMajorVersion = v1.dwMajorVersion;
	v->dwMinorVersion = v1.dwMinorVersion;
	v->dwBuildNumber = v1.dwBuildNumber;
	v->dwPlatformId = v1.dwPlatformId;
	STRING32_AnsiToUni(v->szCSDVersion, v1.szCSDVersion);
	return TRUE;
}

/***********************************************************************
 *	GetWinFlags (KERNEL.132)
 */
LONG GetWinFlags(void)
{
  static const long cpuflags[5] =
    { WF_CPU086, WF_CPU186, WF_CPU286, WF_CPU386, WF_CPU486 };

  long result = 0;

  /* There doesn't seem to be any Pentium flag.  */
#ifndef WINELIB
  long cpuflag = cpuflags[MIN (runtime_cpu (), 4)];
#else
  long cpuflag = cpuflags[4];
#endif

  switch(Options.mode) {
  case MODE_STANDARD:
    result = (WF_STANDARD | cpuflag | WF_PMODE | WF_80x87);
    break;

  case MODE_ENHANCED:
    result = (WF_ENHANCED | cpuflag | WF_PMODE | WF_80x87 | WF_PAGING);
    break;

  default:
    fprintf(stderr, "Unknown mode set? This shouldn't happen. Check GetWinFlags()!\n");
    break;
  }

  return result;
}

/***********************************************************************
 *	SetEnvironment (GDI.132)
 */
int SetEnvironment(LPCSTR lpPortName, LPCSTR lpEnviron, WORD nCount)
{
    LPENVENTRY	lpNewEnv;
    LPENVENTRY	lpEnv = lpEnvList;
    dprintf_env(stddeb, "SetEnvironment('%s', '%s', %d) !\n", 
		lpPortName, lpEnviron, nCount);
    if (lpPortName == NULL) return -1;
    while (lpEnv != NULL) {
	if (lpEnv->Name != NULL && strcmp(lpEnv->Name, lpPortName) == 0) {
	    if (nCount == 0 || lpEnviron == NULL) {
		if (lpEnv->Prev != NULL) lpEnv->Prev->Next = lpEnv->Next;
		if (lpEnv->Next != NULL) lpEnv->Next->Prev = lpEnv->Prev;
		free(lpEnv->Value);
		free(lpEnv->Name);
		free(lpEnv);
		dprintf_env(stddeb, "SetEnvironment() // entry deleted !\n");
		return -1;
	    }
	    free(lpEnv->Value);
	    lpEnv->Value = malloc(nCount);
	    if (lpEnv->Value == NULL) {
		dprintf_env(stddeb, "SetEnvironment() // Error allocating entry value !\n");
		return 0;
	    }
	    memcpy(lpEnv->Value, lpEnviron, nCount);
	    lpEnv->wSize = nCount;
	    dprintf_env(stddeb, "SetEnvironment() // entry modified !\n");
	    return nCount;
	}
	if (lpEnv->Next == NULL) break;
	lpEnv = lpEnv->Next;
    }
    if (nCount == 0 || lpEnviron == NULL) return -1;
    dprintf_env(stddeb, "SetEnvironment() // new entry !\n");
    lpNewEnv = malloc(sizeof(ENVENTRY));
    if (lpNewEnv == NULL) {
	dprintf_env(stddeb, "SetEnvironment() // Error allocating new entry !\n");
	return 0;
    }
    if (lpEnvList == NULL) {
	lpEnvList = lpNewEnv;
	lpNewEnv->Prev = NULL;
    }
    else 
    {
	lpEnv->Next = lpNewEnv;
	lpNewEnv->Prev = lpEnv;
    }
    lpNewEnv->Next = NULL;
    lpNewEnv->Name = malloc(strlen(lpPortName) + 1);
    if (lpNewEnv->Name == NULL) {
	dprintf_env(stddeb, "SetEnvironment() // Error allocating entry name !\n");
	return 0;
    }
    strcpy(lpNewEnv->Name, lpPortName);
    lpNewEnv->Value = malloc(nCount);
    if (lpNewEnv->Value == NULL) {
	dprintf_env(stddeb, "SetEnvironment() // Error allocating entry value !\n");
	return 0;
    }
    memcpy(lpNewEnv->Value, lpEnviron, nCount);
    lpNewEnv->wSize = nCount;
    return nCount;
}


/***********************************************************************
 *      SetEnvironmentVariable32A   (KERNEL32.484)
 */
BOOL32 SetEnvironmentVariable32A( LPCSTR lpName, LPCSTR lpValue )
{
    int rc;

    rc = SetEnvironment(lpName, lpValue, strlen(lpValue) + 1);
    return (rc > 0) ? 1 : 0;
}


/***********************************************************************
 *      SetEnvironmentVariable32W   (KERNEL32.485)
 */
BOOL32 SetEnvironmentVariable32W( LPCWSTR lpName, LPCWSTR lpValue )
{
    LPSTR lpAName, lpAValue;
    BOOL ret;

    lpAName = STRING32_DupUniToAnsi( lpName );
    lpAValue = STRING32_DupUniToAnsi ( lpValue );
    ret = SetEnvironment(lpAName, lpAValue, strlen(lpAValue) + 1);
    free (lpAName);
    free (lpAValue);
    return (ret > 0) ? 1 : 0;    
}


/***********************************************************************
 *	GetEnvironment (GDI.134)
 */
int GetEnvironment(LPSTR lpPortName, LPSTR lpEnviron, WORD nMaxSiz)
{
    WORD       nCount;
    LPENVENTRY lpEnv = lpEnvList;
    dprintf_env(stddeb, "GetEnvironment('%s', '%s', %d) !\n",
		lpPortName, lpEnviron, nMaxSiz);
    while (lpEnv != NULL) {
	if (lpEnv->Name != NULL && strcmp(lpEnv->Name, lpPortName) == 0) {
	    nCount = MIN(nMaxSiz, lpEnv->wSize);
	    memcpy(lpEnviron, lpEnv->Value, nCount);
	    dprintf_env(stddeb, "GetEnvironment() // found '%s' !\n", lpEnviron);
	    return nCount;
	}
	lpEnv = lpEnv->Next;
    }
    dprintf_env(stddeb, "GetEnvironment() // not found !\n");
    return 0;
}

/***********************************************************************
 *      GetEnvironmentVariableA (KERNEL32.213)
 */
DWORD GetEnvironmentVariableA(LPSTR lpName, LPSTR lpValue, DWORD size)
{
    return GetEnvironment(lpName, lpValue, size);
}

/***********************************************************************
 *      GetEnvironmentStrings (KERNEL32.210)
 */
LPVOID GetEnvironmentStrings(void)
{
    int count;
    LPENVENTRY lpEnv;
    char *envtable, *envptr;

    /* Count the total number of bytes we'll need for the string
     * table.  Include the trailing nuls and the final double nul.
     */
    count = 1;
    lpEnv = lpEnvList;
    while(lpEnv != NULL)
    {
        if(lpEnv->Name != NULL)
        {
            count += strlen(lpEnv->Name) + 1;
            count += strlen(lpEnv->Value) + 1;
        }
        lpEnv = lpEnv->Next;
    }

    envtable = malloc(count);
    if(envtable)
    {
        lpEnv = lpEnvList;
        envptr = envtable;

        while(lpEnv != NULL)
        {
            if(lpEnv->Name != NULL)
            {
                count = sprintf(envptr, "%s=%s", lpEnv->Name, lpEnv->Value);
                envptr += count + 1;
            }
            lpEnv = lpEnv->Next;
        }
        *envptr = '\0';
    }

    return envtable;
}


LPVOID GetEnvironmentStringsW(void)
{
    int count,len;
    LPENVENTRY lpEnv;
    char *envtable, *envptr;
	WCHAR *wenvtable;

    /* Count the total number of bytes we'll need for the string
     * table.  Include the trailing nuls and the final double nul.
     */
    count = 1;
    lpEnv = lpEnvList;
    while(lpEnv != NULL)
    {
        if(lpEnv->Name != NULL)
        {
            count += strlen(lpEnv->Name) + 1;
            count += strlen(lpEnv->Value) + 1;
        }
        lpEnv = lpEnv->Next;
    }

	len=count;
    envtable = malloc(count);
    if(envtable)
    {
        lpEnv = lpEnvList;
        envptr = envtable;

        while(lpEnv != NULL)
        {
            if(lpEnv->Name != NULL)
            {
                count = sprintf(envptr, "%s=%s", lpEnv->Name, lpEnv->Value);
                envptr += count + 1;
            }
            lpEnv = lpEnv->Next;
        }
        *envptr = '\0';
    }

	wenvtable = malloc(2*len);
	for(count=0;count<len;count++)
		wenvtable[count]=(WCHAR)envtable[count];
	free(envtable);

    return envtable;
}

void FreeEnvironmentStringsA(void *e)
{
	free(e);
}

void FreeEnvironmentStringsW(void* e)
{
	free(e);
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
BOOL SystemParametersInfo (UINT uAction, UINT uParam, LPVOID lpvParam, UINT fuWinIni)
{
	int timeout, temp;
	char buffer[256];
	XKeyboardState		keyboard_state;
	XKeyboardControl	keyboard_value;


	switch (uAction) {
		case SPI_GETBEEP:
			XGetKeyboardControl(display, &keyboard_state);
			if (keyboard_state.bell_percent == 0)
				*(BOOL *) lpvParam = FALSE;
			else
				*(BOOL *) lpvParam = TRUE;
			break;
		
		case SPI_GETBORDER:
			*(INT *)lpvParam = GetSystemMetrics( SM_CXFRAME );
			break;

		case SPI_GETFASTTASKSWITCH:
			*(BOOL *) lpvParam = FALSE;
			/* FIXME GetProfileInt( "windows", "CoolSwitch", 1 ) */
			break;

		case SPI_GETGRIDGRANULARITY:
			*(INT *) lpvParam = 1;
		/* FIXME GetProfileInt( "desktop", "GridGranularity", 1 ) */
			break;

		case SPI_GETICONTITLEWRAP:
			*(BOOL *) lpvParam = FALSE;
			/* FIXME GetProfileInt( "desktop", "?", True ) */
			break;

		case SPI_GETKEYBOARDDELAY:
			*(INT *) lpvParam = 1;
			/* FIXME */
			break;

		case SPI_GETKEYBOARDSPEED:
			*(WORD *) lpvParam = 30;
			/* FIXME */
			break;

		case SPI_GETMENUDROPALIGNMENT:
			*(BOOL *) lpvParam = GetSystemMetrics( SM_MENUDROPALIGNMENT ); /* XXX check this */
			break;

		case SPI_GETSCREENSAVEACTIVE:
			/* FIXME GetProfileInt( "windows", "ScreenSaveActive", 1 ); */
			*(BOOL *) lpvParam = FALSE;
			break;

		case SPI_GETSCREENSAVETIMEOUT:
			/* FIXME GetProfileInt( "windows", "ScreenSaveTimeout", 300 ); */
			XGetScreenSaver(display, &timeout, &temp,&temp,&temp);
			*(INT *) lpvParam = timeout * 1000;
			break;

		case SPI_ICONHORIZONTALSPACING:
			if (lpvParam == NULL)
				fprintf(stderr, "SystemParametersInfo: Horizontal icon spacing set to %d\n.", uParam);
			else
				*(INT *) lpvParam = GetSystemMetrics( SM_CXICONSPACING );
			break;

		case SPI_ICONVERTICALSPACING:
			if (lpvParam == NULL)
				fprintf(stderr, "SystemParametersInfo: Vertical icon spacing set to %d\n.", uParam);
			else
				*(INT *) lpvParam = GetSystemMetrics( SM_CYICONSPACING );
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
			return (SetDeskWallPaper((LPSTR) lpvParam));
			break;

		case SPI_SETDESKPATTERN:
			if ((INT) uParam == -1) {
				GetProfileString("Desktop", "Pattern", 
						"170 85 170 85 170 85 170 85", 
						buffer, sizeof(buffer) );
				return (DESKTOP_SetPattern((LPSTR) buffer));
			} else
				return (DESKTOP_SetPattern((LPSTR) lpvParam));
			break;

	        case SPI_GETICONTITLELOGFONT: 
	        {
		    /* FIXME GetProfileString( "?", "?", "?" ) */
		  LPLOGFONT16 lpLogFont = (LPLOGFONT16)lpvParam;
		  lpLogFont->lfHeight = 10;
		  lpLogFont->lfWidth = 0;
		  lpLogFont->lfEscapement = lpLogFont->lfOrientation = 0;
		  lpLogFont->lfWeight = FW_NORMAL;
		  lpLogFont->lfItalic = lpLogFont->lfStrikeOut = lpLogFont->lfUnderline = FALSE;
		  lpLogFont->lfCharSet = ANSI_CHARSET;
		  lpLogFont->lfOutPrecision = OUT_DEFAULT_PRECIS;
		  lpLogFont->lfClipPrecision = CLIP_DEFAULT_PRECIS;
		  lpLogFont->lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;
		  break;
		}
		case SPI_LANGDRIVER:
		case SPI_SETBORDER:
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

/***********************************************************************
*	COPY (GDI.250)
*/
void Copy(LPVOID lpSource, LPVOID lpDest, WORD nBytes)
{
	memcpy(lpDest, lpSource, nBytes);
}

/***********************************************************************
*	SWAPMOUSEBUTTON (USER.186)
*/
BOOL SwapMouseButton(BOOL fSwap)
{
	return 0;	/* don't swap */
}

/***********************************************************************
*	FileCDR (KERNEL.130)
*/
void FileCDR(FARPROC x)
{
	printf("FileCDR(%8x)\n", (int) x);
}

/***********************************************************************
*	GetWinDebugInfo (KERNEL.355)
*/
BOOL GetWinDebugInfo(WINDEBUGINFO *lpwdi, UINT flags)
{
	printf("GetWinDebugInfo(%8lx,%d) stub returning 0\n", (unsigned long)lpwdi, flags);
	/* 0 means not in debugging mode/version */
	/* Can this type of debugging be used in wine ? */
	/* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
	return 0;
}

/***********************************************************************
*	GetWinDebugInfo (KERNEL.355)
*/
BOOL SetWinDebugInfo(WINDEBUGINFO *lpwdi)
{
	printf("SetWinDebugInfo(%8lx) stub returning 0\n", (unsigned long)lpwdi);
	/* 0 means not in debugging mode/version */
	/* Can this type of debugging be used in wine ? */
	/* Constants: WDI_OPTIONS WDI_FILTER WDI_ALLOCBREAK */
	return 0;
}
