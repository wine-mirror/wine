/*
 * X11 main driver
 *
 * Copyright 1998 Patrik Stridvall
 *
 */

#include "config.h"

#ifndef X_DISPLAY_MISSING

#include <X11/Xlocale.h>
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "clipboard.h"
#include "console.h"
#include "debugtools.h"
#include "desktop.h"
#include "keyboard.h"
#include "main.h"
#include "message.h"
#include "monitor.h"
#include "mouse.h"
#include "options.h"
#include "win.h"
#include "windef.h"
#include "x11drv.h"
#include "xmalloc.h"
#include "version.h"
#include "win.h"

/**********************************************************************/

void X11DRV_USER_ParseOptions(int *argc, char *argv[]);
void X11DRV_USER_Create(void);
void X11DRV_USER_SaveSetup(void);
void X11DRV_USER_RestoreSetup(void);

/**********************************************************************/

#define WINE_CLASS        "Wine"    /* Class name for resources */
#define WINE_APP_DEFAULTS "/usr/lib/X11/app-defaults/Wine"

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
    { "-winver",        ".winver",          XrmoptionSepArg, (caddr_t)NULL },
    { "-config",        ".config",          XrmoptionSepArg, (caddr_t)NULL },
    { "-nodga",         ".nodga",           XrmoptionNoArg,  (caddr_t)"off"},
    { "-console",       ".console",         XrmoptionSepArg, (caddr_t)NULL },
    { "-dosver",        ".dosver",          XrmoptionSepArg, (caddr_t)NULL }
};

#define NB_OPTIONS  (sizeof(optionsTable) / sizeof(optionsTable[0]))

/**********************************************************************/

static XKeyboardState X11DRV_XKeyboardState;
Display *display;

/***********************************************************************
 *              X11DRV_GetXScreen
 *
 * Return the X screen associated to the current desktop.
 */
Screen *X11DRV_GetXScreen()
{
  return X11DRV_MONITOR_GetXScreen(&MONITOR_PrimaryMonitor);
}

/***********************************************************************
 *              X11DRV_GetXRootWindow
 *
 * Return the X display associated to the current desktop.
 */
Window X11DRV_GetXRootWindow()
{
  return X11DRV_MONITOR_GetXRootWindow(&MONITOR_PrimaryMonitor);
}

/***********************************************************************
 *              X11DRV_USER_Initialize
 */
BOOL X11DRV_USER_Initialize(void)
{
  CLIPBOARD_Driver = &X11DRV_CLIPBOARD_Driver;
  DESKTOP_Driver = &X11DRV_DESKTOP_Driver;
  EVENT_Driver = &X11DRV_EVENT_Driver;
  KEYBOARD_Driver = &X11DRV_KEYBOARD_Driver;
  MONITOR_Driver = &X11DRV_MONITOR_Driver;
  MOUSE_Driver = &X11DRV_MOUSE_Driver;
  WND_Driver = &X11DRV_WND_Driver;

  /* We need this before calling any Xlib function */
  InitializeCriticalSection( &X11DRV_CritSection );
  MakeCriticalSectionGlobal( &X11DRV_CritSection );
  
  TSXrmInitialize();
  
  putenv("XKB_DISABLE="); /* Disable XKB extension if present. */

  X11DRV_USER_ParseOptions( Options.argc, Options.argv );
  X11DRV_USER_Create();
  X11DRV_USER_SaveSetup();

  return TRUE;
}

/***********************************************************************
 *              X11DRV_USER_Finalize
 */
void X11DRV_USER_Finalize(void)
{
  X11DRV_USER_RestoreSetup();
}

/**************************************************************************
 *		X11DRV_USER_BeginDebugging
 */
void X11DRV_USER_BeginDebugging(void)
{
  TSXUngrabServer(display);
  TSXFlush(display);
}

/**************************************************************************
 *		X11DRV_USER_EndDebugging
 */
void X11DRV_USER_EndDebugging(void)
{
}

/***********************************************************************
 *		 X11DRV_USER_GetResource
 *
 * Fetch the value of resource 'name' using the correct instance name.
 * 'name' must begin with '.' or '*'
 */
static int X11DRV_USER_GetResource( XrmDatabase db, char *name, XrmValue *value )
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
  retval = TSXrmGetResource( db, buff_instance, buff_class, &dummy, value );
  free( buff_instance );
  free( buff_class );
  return retval;
}

/***********************************************************************
 *              X11DRV_USER_ParseOptions
 * Parse command line options and open display.
 */
void X11DRV_USER_ParseOptions(int *argc, char *argv[])
{
  int i;
  char *display_name = NULL;
  XrmValue value;
  XrmDatabase db = TSXrmGetFileDatabase(WINE_APP_DEFAULTS);
  char *xrm_string;

  /* Get display name from command line */
  for (i = 1; i < *argc; i++)
    {
      if (!strcmp( argv[i], "-display" )) display_name = argv[i+1];
    }

  /* Open display */
  
  if (display_name == NULL &&
      X11DRV_USER_GetResource( db, ".display", &value )) display_name = value.addr;
  
  if (!(display = TSXOpenDisplay( display_name )))
    {
      MESSAGE( "%s: Can't open display: %s\n",
	   argv[0], display_name ? display_name : "(none specified)" );
      exit(1);
    }
  
  /* tell the libX11 that we will do input method handling ourselves
   * that keep libX11 from doing anything whith dead keys, allowing Wine
   * to have total control over dead keys, that is this line allows
   * them to work in Wine, even whith a libX11 including the dead key
   * patches from Th.Quinot (http://Web.FdN.FR/~tquinot/dead-keys.en.html)
   */
  TSXOpenIM(display,NULL,NULL,NULL);
  
  /* Merge file and screen databases */
  if ((xrm_string = TSXResourceManagerString( display )) != NULL)
    {
      XrmDatabase display_db = TSXrmGetStringDatabase( xrm_string );
      TSXrmMergeDatabases( display_db, &db );
    }
  
  /* Parse command line */
  TSXrmParseCommand( &db, optionsTable, NB_OPTIONS,
		     Options.programName, argc, argv );
  
  /* Get all options */
  if (X11DRV_USER_GetResource( db, ".iconic", &value ))
    Options.cmdShow = SW_SHOWMINIMIZED;
  if (X11DRV_USER_GetResource( db, ".privatemap", &value ))
    Options.usePrivateMap = TRUE;
  if (X11DRV_USER_GetResource( db, ".fixedmap", &value ))
    Options.useFixedMap = TRUE;
  if (X11DRV_USER_GetResource( db, ".synchronous", &value ))
    Options.synchronous = TRUE;
  if (X11DRV_USER_GetResource( db, ".backingstore", &value ))
    Options.backingstore = TRUE;	
  if (X11DRV_USER_GetResource( db, ".debug", &value ))
    Options.debug = TRUE;
  if (X11DRV_USER_GetResource( db, ".failreadonly", &value ))
    Options.failReadOnly = TRUE;
  if (X11DRV_USER_GetResource( db, ".perfect", &value ))
    Options.perfectGraphics = TRUE;
  if (X11DRV_USER_GetResource( db, ".depth", &value))
    Options.screenDepth = atoi( value.addr );
  if (X11DRV_USER_GetResource( db, ".desktop", &value))
    Options.desktopGeometry = value.addr;
  if (X11DRV_USER_GetResource( db, ".language", &value))
    MAIN_ParseLanguageOption( (char *)value.addr );
  if (X11DRV_USER_GetResource( db, ".managed", &value))
    Options.managed = TRUE;
  if (X11DRV_USER_GetResource( db, ".mode", &value))
    MAIN_ParseModeOption( (char *)value.addr );
  if (X11DRV_USER_GetResource( db, ".debugoptions", &value))
    MAIN_ParseDebugOptions((char*)value.addr);
  if (X11DRV_USER_GetResource( db, ".debugmsg", &value))
    {
#ifndef DEBUG_RUNTIME
      MESSAGE("%s: Option \"-debugmsg\" not implemented.\n" \
          "    Recompile with DEBUG_RUNTIME in include/debugtools.h defined.\n",
	  argv[0]);
      exit(1);
#else
      MAIN_ParseDebugOptions((char*)value.addr);
#endif
    }
  
  if (X11DRV_USER_GetResource( db, ".dll", &value))
  {
      if (Options.dllFlags)
      {
          /* don't overwrite previous value. Should we
           * automatically add the ',' between multiple DLLs ?
           */
          MESSAGE("Only one -dll flag is allowed. Use ',' between multiple DLLs\n");
      }
      else Options.dllFlags = xstrdup((char *)value.addr);
  }
  
  if (X11DRV_USER_GetResource( db, ".winver", &value))
    VERSION_ParseWinVersion( (char*)value.addr );
  if (X11DRV_USER_GetResource( db, ".dosver", &value))
    VERSION_ParseDosVersion( (char*)value.addr );
  if (X11DRV_USER_GetResource( db, ".config", &value))
    Options.configFileName = xstrdup((char *)value.addr);
  if (X11DRV_USER_GetResource( db, ".nodga", &value))
    Options.noDGA = TRUE;
  if (X11DRV_USER_GetResource( db, ".console", &value))
      driver.driver_list = xstrdup((char *)value.addr);
  else
      driver.driver_list = CONSOLE_DEFAULT_DRIVER;
}

/***********************************************************************
 *		X11DRV_USER_ErrorHandler
 */
static int X11DRV_USER_ErrorHandler(Display *display, XErrorEvent *error_evt)
{
    DebugBreak();  /* force an entry in the debugger */
    return 0;
}

/***********************************************************************
 *		X11DRV_USER_Create
 */
void X11DRV_USER_Create()
{
  if (Options.synchronous) XSetErrorHandler( X11DRV_USER_ErrorHandler );
  
  if (Options.desktopGeometry && Options.managed)
    {
#if 0
      MESSAGE( "%s: -managed and -desktop options cannot be used together\n",
	   Options.programName );
      exit(1);
#else
      Options.managed = FALSE;
#endif
    }
}

/***********************************************************************
 *           X11DRV_USER_SaveSetup
 */
void X11DRV_USER_SaveSetup()
{
  TSXGetKeyboardControl(display, &X11DRV_XKeyboardState);
}

/***********************************************************************
 *           X11DRV_USER_RestoreSetup
 */
void X11DRV_USER_RestoreSetup()
{
  XKeyboardControl keyboard_value;
  
  keyboard_value.key_click_percent	= X11DRV_XKeyboardState.key_click_percent;
  keyboard_value.bell_percent 	= X11DRV_XKeyboardState.bell_percent;
  keyboard_value.bell_pitch		= X11DRV_XKeyboardState.bell_pitch;
  keyboard_value.bell_duration	= X11DRV_XKeyboardState.bell_duration;
  keyboard_value.auto_repeat_mode	= X11DRV_XKeyboardState.global_auto_repeat;
  
  XChangeKeyboardControl(display, KBKeyClickPercent | KBBellPercent | 
			 KBBellPitch | KBBellDuration | KBAutoRepeatMode, &keyboard_value);
}

#endif /* X_DISPLAY_MISSING */
