/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 */

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#ifdef MALLOC_DEBUGGING
# include <malloc.h>
#endif
#include "ts_xlib.h"
#include "ts_xresource.h"
#include "ts_xutil.h"
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
#include "debug.h"
#include "debugdefs.h"
#include "xmalloc.h"
#include "version.h"
#include "winnls.h"
#include "x11drv.h"
#include "console.h"

/* when adding new languages look at ole/ole2nls.c 
 * for proper iso name and Windows code (add 0x0400 
 * to the code listed there)
 */
const WINE_LANGUAGE_DEF Languages[] =
{
    {"En",0x0409},	/* LANG_En */
    {"Es",0x040A},	/* LANG_Es */
    {"De",0x0407},	/* LANG_De */
    {"No",0x0414},	/* LANG_No */
    {"Fr",0x040C},	/* LANG_Fr */
    {"Fi",0x040B},	/* LANG_Fi */
    {"Da",0x0406},	/* LANG_Da */
    {"Cs",0x0405},	/* LANG_Cs */
    {"Eo",0x048f},	/* LANG_Eo */
    {"It",0x0410},	/* LANG_It */
    {"Ko",0x0412},	/* LANG_Ko */
    {"Hu",0x040e},	/* LANG_Hu */
    {"Pl",0x0415},	/* LANG_Pl */
    {"Pt",0x0416},	/* LANG_Pt */
    {"Sv",0x041d},	/* LANG_Sv */
    {"Ca",0x0403},	/* LANG_Ca */
    {NULL,0}
};

WORD WINE_LanguageId = 0x409;	/* english as default */

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
    FALSE,          /* Perfect graphics */
    FALSE,          /* No DGA */
    NULL,           /* Alternate config file name */
    NULL	    /* Console driver list */
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
    { "-winver",        ".winver",          XrmoptionSepArg, (caddr_t)NULL },
    { "-config",        ".config",          XrmoptionSepArg, (caddr_t)NULL },
    { "-nodga",         ".nodga",           XrmoptionNoArg,  (caddr_t)"off"},
    { "-console",       ".console",         XrmoptionSepArg, (caddr_t)NULL },
    { "-dosver",        ".dosver",          XrmoptionSepArg, (caddr_t)NULL }
};

#define NB_OPTIONS  (sizeof(optionsTable) / sizeof(optionsTable[0]))

#define USAGE \
  "%s\n" \
  "Usage:  %s [options] \"program_name [arguments]\"\n" \
  "\n" \
  "Options:\n" \
  "    -backingstore   Turn on backing store\n" \
  "    -config name    Specify config file to use\n" \
  "    -console driver Select which driver(s) to use for the console\n" \
  "    -debug          Enter debugger before starting application\n" \
  "    -debugmsg name  Turn debugging-messages on or off\n" \
  "    -depth n        Change the depth to use for multiple-depth screens\n" \
  "    -desktop geom   Use a desktop window of the given geometry\n" \
  "    -display name   Use the specified display\n" \
  "    -dll name       Enable or disable built-in DLLs\n" \
  "    -failreadonly   Read only files may not be opened in write mode\n" \
  "    -fixedmap       Use a \"standard\" color map\n" \
  "    -help           Show this help message\n" \
  "    -iconic         Start as an icon\n" \
  "    -language xx    Set the language (one of Ca,Cs,Da,De,En,Eo,Es,Fi,Fr,Hu,It,\n                    Ko,No,Pl,Pt,Sv)\n" \
  "    -managed        Allow the window manager to manage created windows\n" \
  "    -mode mode      Start Wine in a particular mode (standard or enhanced)\n" \
  "    -name name      Set the application name\n" \
  "    -nodga          Disable XFree86 DGA extensions\n" \
  "    -perfect        Favor correctness over speed for graphical operations\n" \
  "    -privatemap     Use a private color map\n" \
  "    -synchronous    Turn on synchronous display mode\n" \
  "    -version        Display the Wine version\n" \
  "    -winver         Windows version to imitate (one of win31,win95,nt351,nt40)\n" \
  "    -dosver         DOS version to imitate (x.xx, e.g. 6.22). Only valid with -winver win31\n"



/***********************************************************************
 *           MAIN_Usage
 */
void MAIN_Usage( char *name )
{
    MSG( USAGE, WINE_RELEASE_INFO, name );
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
    retval = TSXrmGetResource( db, buff_instance, buff_class, &dummy, value );
    free( buff_instance );
    free( buff_class );
    return retval;
}


/***********************************************************************
 *          MAIN_ParseDebugOptions
 *
 *  Turns specific debug messages on or off, according to "options".
 *  
 *  RETURNS
 *    TRUE if parsing was successful
 */
BOOL32 MAIN_ParseDebugOptions(char *options)
{
  /* defined in relay32/relay386.c */
  extern char **debug_relay_includelist;
  extern char **debug_relay_excludelist;
  /* defined in relay32/snoop.c */
  extern char **debug_snoop_includelist;
  extern char **debug_snoop_excludelist;

  int l, cls;

  l = strlen(options);
  if (l<3)
    return FALSE;
  if (options[l-1]=='\n') options[l-1]='\0';
  do
  {
    if ((*options!='+')&&(*options!='-')){
      int j;

      for(j=0; j<DEBUG_CLASS_COUNT; j++)
	if(!lstrncmpi32A(options, debug_cl_name[j], strlen(debug_cl_name[j])))
	  break;
      if(j==DEBUG_CLASS_COUNT)
	return FALSE;
      options += strlen(debug_cl_name[j]);
      if ((*options!='+')&&(*options!='-'))
	return FALSE;
      cls = j;
    }
    else
      cls = -1; /* all classes */

    if (strchr(options,','))
      l=strchr(options,',')-options;
    else
      l=strlen(options);

    if (!lstrncmpi32A(options+1,"all",l-1))
      {
	int i, j;
	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  for(j=0; j<DEBUG_CLASS_COUNT; j++)
	    if(cls == -1 || cls == j)
	      debug_msg_enabled[i][j]=(*options=='+');
      }
    else if (!lstrncmpi32A(options+1, "relay=", 6) ||
	     !lstrncmpi32A(options+1, "snoop=", 6))
      {
	int i, j;
	char *s, *s2, ***output, c;

	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  if (debug_ch_name && (!lstrncmpi32A(debug_ch_name[i],options+1,5))){
	    for(j=0; j<DEBUG_CLASS_COUNT; j++)
	      if(cls == -1 || cls == j)
		debug_msg_enabled[i][j]=TRUE;
	    break;
	  }
	/* should never happen, maybe assert(i!=DEBUG_CHANNEL_COUNT)? */
	if (i==DEBUG_CHANNEL_COUNT)
	  return FALSE;
	output = (*options == '+') ?
			((*(options+1) == 'r') ?
				&debug_relay_includelist :
				&debug_snoop_includelist) :
			((*(options+1) == 'r') ?
				&debug_relay_excludelist :
				&debug_snoop_excludelist);
	s = options + 7;
	i = 1;
	while((s = strchr(s, ':'))) i++, s++;
	*output = malloc(sizeof(char **) * i + 1);
	i = 0;
	s = options + 7;
	while((s2 = strchr(s, ':'))) {
          c = *s2;
          *s2 = '\0';
	  *((*output)+i) = strdup(s);
          *s2 = c;
	  s = s2 + 1;
	  i++;
	}
	c = *(options + l);
	*(options + l) = '\0';
	*((*output)+i) = strdup(s);
	*(options + l) = c;
	*((*output)+i+1) = NULL;
      }
    else
      {
	int i, j;
	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  if (debug_ch_name && (!lstrncmpi32A(options+1,debug_ch_name[i],l-1))){
	    for(j=0; j<DEBUG_CLASS_COUNT; j++)
	      if(cls == -1 || cls == j)
		debug_msg_enabled[i][j]=(*options=='+');
	    break;
	  }
	if (i==DEBUG_CHANNEL_COUNT)
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

/***********************************************************************
 *           MAIN_GetLanguageID
 *
 * INPUT:
 *	Lang: a string whose two first chars are the iso name of a language.
 *	Country: a string whose two first chars are the iso name of country
 *	Charset: a string defining the chossen charset encoding
 *	Dialect: a string defining a variation of the locale
 *
 *	all those values are from the standardized format of locale
 *	name in unix which is: Lang[_Country][.Charset][@Dialect]
 *
 * RETURNS:
 *	the numeric code of the language used by Windows (or 0x00)
 */
int MAIN_GetLanguageID(LPCSTR Lang,LPCSTR Country,LPCSTR Charset,LPCSTR Dialect)
{
    char lang[3]="??", country[3]={0,0,0};
    char *charset=NULL, *dialect=NULL;
    int i,j,ret=0;

    if (Lang==NULL) return 0x00;
    if (Lang[0]) lang[0]=tolower(Lang[0]);
    if (Lang[1]) lang[1]=tolower(Lang[1]);

    if (Country!=NULL) {
	if (Country[0]) country[0]=toupper(Country[0]);
	if (Country[1]) country[1]=toupper(Country[1]);
    }

    if (Charset!=NULL) {
	j=strlen(Charset);
	charset=(char*)malloc(j+1);
	for (i=0;i<j;i++)
	    charset[i]=toupper(Charset[i]);
	charset[i]='\0';
    }

    if (Dialect!=NULL) {
	j=strlen(Dialect);
	dialect=(char*)malloc(j+1);
	for (i=0;i<j;i++)
	    dialect[i]=tolower(Dialect[i]);
	dialect[i]='\0';
    }

#define LANG_ENTRY_BEGIN(x,y)	if(!strcmp(lang, x )) { \
				    if (!country[0]) { \
					ret=LANG_##y ; \
					goto end_MAIN_GetLanguageID; \
				    }
#define LANG_SUB_ENTRY(x,y,z)	    if (!strcmp(country, x )) \
					ret = MAKELANGID( LANG_##y , SUBLANG_##z ); \
					goto end_MAIN_GetLanguageID;
#define LANG_DIALECT_ENTRY(x,y)	    { ret = MAKELANGID(LANG_##x , SUBLANG_##y ); \
				    goto end_MAIN_GetLanguageID; }
#define LANG_ENTRY_END(x)	    ret = MAKELANGID(LANG_##x , SUBLANG_DEFAULT); \
				    goto end_MAIN_GetLanguageID; \
				}

/*x01*/ LANG_ENTRY_BEGIN( "ar", ARABIC )
	LANG_SUB_ENTRY( "SA", ARABIC, ARABIC)
	LANG_SUB_ENTRY( "IQ", ARABIC, ARABIC_IRAQ )
	LANG_SUB_ENTRY( "EG", ARABIC, ARABIC_EGYPT )
	LANG_SUB_ENTRY( "LY", ARABIC, ARABIC_LIBYA )
	LANG_SUB_ENTRY( "DZ", ARABIC, ARABIC_ALGERIA )
	LANG_SUB_ENTRY( "MA", ARABIC, ARABIC_MOROCCO )
	LANG_SUB_ENTRY( "TN", ARABIC, ARABIC_TUNISIA )
	LANG_SUB_ENTRY( "OM", ARABIC, ARABIC_OMAN )
	LANG_SUB_ENTRY( "YE", ARABIC, ARABIC_YEMEN )
	LANG_SUB_ENTRY( "SY", ARABIC, ARABIC_SYRIA )
	LANG_SUB_ENTRY( "JO", ARABIC, ARABIC_JORDAN )
	LANG_SUB_ENTRY( "LB", ARABIC, ARABIC_LEBANON )
	LANG_SUB_ENTRY( "KW", ARABIC, ARABIC_KUWAIT )
	LANG_SUB_ENTRY( "AE", ARABIC, ARABIC_UAE )
	LANG_SUB_ENTRY( "BH", ARABIC, ARABIC_BAHRAIN )
	LANG_SUB_ENTRY( "QA", ARABIC, ARABIC_QATAR )
	LANG_ENTRY_END( ARABIC )
/*x02*/ LANG_ENTRY_BEGIN( "bu", BULGARIAN )
	LANG_ENTRY_END( BULGARIAN )
/*x03*/ LANG_ENTRY_BEGIN( "ca", CATALAN )
	LANG_ENTRY_END( CATALAN )
/*x04*/ LANG_ENTRY_BEGIN( "zh", CHINESE )
	LANG_SUB_ENTRY( "TW", CHINESE, CHINESE_TRADITIONAL )
	LANG_SUB_ENTRY( "CN", CHINESE, CHINESE_SIMPLIFIED )
	LANG_SUB_ENTRY( "HK", CHINESE, CHINESE_HONGKONG )
	LANG_SUB_ENTRY( "SG", CHINESE, CHINESE_SINGAPORE )
	LANG_SUB_ENTRY( "MO", CHINESE, CHINESE_MACAU )
	LANG_ENTRY_END( CHINESE )
/*x05*/ LANG_ENTRY_BEGIN( "cs", CZECH )
	LANG_ENTRY_END( CZECH )
/*x06*/ LANG_ENTRY_BEGIN( "da", DANISH )
	LANG_ENTRY_END( DANISH )
/*x07*/ LANG_ENTRY_BEGIN( "de", GERMAN )
	LANG_SUB_ENTRY( "DE", GERMAN, GERMAN )
	LANG_SUB_ENTRY( "CH", GERMAN, GERMAN_SWISS )
	LANG_SUB_ENTRY( "AT", GERMAN, GERMAN_AUSTRIAN )
	LANG_SUB_ENTRY( "LU", GERMAN, GERMAN_LUXEMBOURG )
	LANG_SUB_ENTRY( "LI", GERMAN, GERMAN_LIECHTENSTEIN )
	LANG_ENTRY_END( GERMAN )
/*x08*/ LANG_ENTRY_BEGIN( "el", GREEK )
	LANG_ENTRY_END( GREEK )
/*x09*/ LANG_ENTRY_BEGIN( "en", ENGLISH )
	LANG_SUB_ENTRY( "US", ENGLISH, ENGLISH_US )
	LANG_SUB_ENTRY( "UK", ENGLISH, ENGLISH_UK )
	LANG_SUB_ENTRY( "AU", ENGLISH, ENGLISH_AUS )
	LANG_SUB_ENTRY( "CA", ENGLISH, ENGLISH_CAN )
	LANG_SUB_ENTRY( "NZ", ENGLISH, ENGLISH_NZ )
	LANG_SUB_ENTRY( "EI", ENGLISH, ENGLISH_EIRE )
	LANG_SUB_ENTRY( "ZA", ENGLISH, ENGLISH_SAFRICA )
	LANG_SUB_ENTRY( "JM", ENGLISH, ENGLISH_JAMAICA )
     /*	LANG_SUB_ENTRY( "AG", ENGLISH, ENGLISH_CARIBBEAN ) */
	LANG_SUB_ENTRY( "BZ", ENGLISH, ENGLISH_BELIZE )
	LANG_SUB_ENTRY( "TT", ENGLISH, ENGLISH_TRINIDAD )
	LANG_SUB_ENTRY( "ZW", ENGLISH, ENGLISH_ZIMBABWE )
	LANG_SUB_ENTRY( "PH", ENGLISH, ENGLISH_PHILIPPINES )
	LANG_ENTRY_END( ENGLISH )
/*x0a*/ LANG_ENTRY_BEGIN( "es", SPANISH )
	/* traditional sorting */
	if (!strcmp(dialect,"tradicional"))
		LANG_DIALECT_ENTRY( SPANISH, SPANISH )
	LANG_SUB_ENTRY( "MX", SPANISH, SPANISH_MEXICAN )
	LANG_SUB_ENTRY( "ES", SPANISH, SPANISH_MODERN )
	LANG_SUB_ENTRY( "GT", SPANISH, SPANISH_GUATEMALA )
	LANG_SUB_ENTRY( "CR", SPANISH, SPANISH_COSTARICA )
	LANG_SUB_ENTRY( "PA", SPANISH, SPANISH_PANAMA )
	LANG_SUB_ENTRY( "DO", SPANISH, SPANISH_DOMINICAN )
	LANG_SUB_ENTRY( "VE", SPANISH, SPANISH_VENEZUELA )
	LANG_SUB_ENTRY( "CO", SPANISH, SPANISH_COLOMBIA )
	LANG_SUB_ENTRY( "PE", SPANISH, SPANISH_PERU )
	LANG_SUB_ENTRY( "AR", SPANISH, SPANISH_ARGENTINA )
	LANG_SUB_ENTRY( "EC", SPANISH, SPANISH_ECUADOR )
	LANG_SUB_ENTRY( "CL", SPANISH, SPANISH_CHILE )
	LANG_SUB_ENTRY( "UY", SPANISH, SPANISH_URUGUAY )
	LANG_SUB_ENTRY( "PY", SPANISH, SPANISH_PARAGUAY )
	LANG_SUB_ENTRY( "BO", SPANISH, SPANISH_BOLIVIA )
	LANG_SUB_ENTRY( "HN", SPANISH, SPANISH_HONDURAS )
	LANG_SUB_ENTRY( "NI", SPANISH, SPANISH_NICARAGUA )
	LANG_SUB_ENTRY( "PR", SPANISH, SPANISH_PUERTO_RICO )
	LANG_ENTRY_END( SPANISH )
/*x0b*/ LANG_ENTRY_BEGIN( "fi", FINNISH )
	LANG_ENTRY_END( FINNISH )
/*x0c*/ LANG_ENTRY_BEGIN( "fr", FRENCH )
	LANG_SUB_ENTRY( "FR", FRENCH, FRENCH )
	LANG_SUB_ENTRY( "BE", FRENCH, FRENCH_BELGIAN )
	LANG_SUB_ENTRY( "CA", FRENCH, FRENCH_CANADIAN )
	LANG_SUB_ENTRY( "CH", FRENCH, FRENCH_SWISS )
	LANG_SUB_ENTRY( "LU", FRENCH, FRENCH_LUXEMBOURG )
	LANG_SUB_ENTRY( "MC", FRENCH, FRENCH_MONACO )
	LANG_ENTRY_END( FRENCH )
/*x0d*/ LANG_ENTRY_BEGIN( "iw", HEBREW )
	LANG_ENTRY_END( HEBREW )
/*x0e*/ LANG_ENTRY_BEGIN( "hu", HUNGARIAN )
	LANG_ENTRY_END( HUNGARIAN )
/*x0f*/ LANG_ENTRY_BEGIN( "ic", ICELANDIC )
	LANG_ENTRY_END( ICELANDIC )
/*x10*/ LANG_ENTRY_BEGIN( "it", ITALIAN )
	LANG_SUB_ENTRY( "IT", ITALIAN, ITALIAN )
	LANG_SUB_ENTRY( "CH", ITALIAN, ITALIAN_SWISS )
	LANG_ENTRY_END( ITALIAN )
/*x11*/ LANG_ENTRY_BEGIN( "ja", JAPANESE )
	LANG_ENTRY_END( JAPANESE )
/*x12*/ LANG_ENTRY_BEGIN( "ko", KOREAN )
	/* JOHAB encoding */
	if (!strcmp(charset,"JOHAB"))
		LANG_DIALECT_ENTRY( KOREAN, KOREAN_JOHAB )
	else
		LANG_DIALECT_ENTRY( KOREAN, KOREAN )
	LANG_ENTRY_END( KOREAN )
/*x13*/ LANG_ENTRY_BEGIN( "nl", DUTCH )
	LANG_SUB_ENTRY( "NL", DUTCH, DUTCH )
	LANG_SUB_ENTRY( "BE", DUTCH, DUTCH_BELGIAN )
	LANG_SUB_ENTRY( "SR", DUTCH, DUTCH_SURINAM )
	LANG_ENTRY_END( DUTCH )
/*x14*/ LANG_ENTRY_BEGIN( "no", NORWEGIAN )
	/* nynorsk */
	if (!strcmp(dialect,"nynorsk"))
		LANG_DIALECT_ENTRY( NORWEGIAN, NORWEGIAN_NYNORSK )
	else
		LANG_DIALECT_ENTRY( NORWEGIAN, NORWEGIAN_BOKMAL )
	LANG_ENTRY_END( NORWEGIAN )
/*x15*/ LANG_ENTRY_BEGIN( "pl", POLISH )
	LANG_ENTRY_END( POLISH )
/*x16*/ LANG_ENTRY_BEGIN( "pt", PORTUGUESE )
	LANG_SUB_ENTRY( "BR", PORTUGUESE, PORTUGUESE_BRAZILIAN )
	LANG_SUB_ENTRY( "PT", PORTUGUESE, PORTUGUESE )
	LANG_ENTRY_END( PORTUGUESE )
/*x17*/ LANG_ENTRY_BEGIN( "rm", RHAETO_ROMANCE )
	LANG_ENTRY_END( RHAETO_ROMANCE )
/*x18*/ LANG_ENTRY_BEGIN( "ro", ROMANIAN )
	LANG_SUB_ENTRY( "RO", ROMANIAN, ROMANIAN )
	LANG_SUB_ENTRY( "MD", ROMANIAN, ROMANIAN_MOLDAVIA )
	LANG_ENTRY_END( ROMANIAN )
/*x19*/ LANG_ENTRY_BEGIN( "ru", RUSSIAN )
	LANG_SUB_ENTRY( "RU", RUSSIAN, RUSSIAN )
	LANG_SUB_ENTRY( "MD", RUSSIAN, RUSSIAN_MOLDAVIA )
	LANG_ENTRY_END( RUSSIAN )
/*x1a*/ if (!strcmp(lang,"sh") || !strcmp(lang,"hr") || !strcmp(lang,"sr")) {
	    if (!country[0]) 
		LANG_DIALECT_ENTRY( SERBO_CROATIAN, NEUTRAL)
	    if (!strcmp(charset,"ISO-8859-5"))
		LANG_DIALECT_ENTRY( SERBO_CROATIAN, SERBIAN )
	    LANG_SUB_ENTRY( "HR", SERBO_CROATIAN, CROATIAN )
	    if (!strcmp(country,"YU") && !strcmp(charset,"ISO-8859-2"))
		LANG_DIALECT_ENTRY( SERBO_CROATIAN, SERBIAN_LATIN )
	    LANG_SUB_ENTRY( "YU", SERBO_CROATIAN, SERBIAN )
	    LANG_DIALECT_ENTRY( SERBO_CROATIAN, SERBIAN_LATIN )
	}
/*x1b*/ LANG_ENTRY_BEGIN( "sk", SLOVAK )
	LANG_ENTRY_END( SLOVAK )
/*x1c*/ LANG_ENTRY_BEGIN( "sq", ALBANIAN )
	LANG_ENTRY_END( ALBANIAN )
/*x1d*/ LANG_ENTRY_BEGIN( "sv", SWEDISH )
	LANG_SUB_ENTRY( "SE", SWEDISH, SWEDISH )
	LANG_SUB_ENTRY( "FI", SWEDISH, SWEDISH_FINLAND )
	LANG_ENTRY_END( SWEDISH )
/*x1e*/ LANG_ENTRY_BEGIN( "th", THAI )
	LANG_ENTRY_END( THAI )
/*x1f*/ LANG_ENTRY_BEGIN( "tr", TURKISH )
	LANG_ENTRY_END( TURKISH )
/*x20*/ LANG_ENTRY_BEGIN( "ur", URDU )
	LANG_ENTRY_END( URDU )
/*x21*/ LANG_ENTRY_BEGIN( "in", INDONESIAN )
	LANG_ENTRY_END( INDONESIAN )
/*x22*/ LANG_ENTRY_BEGIN( "uk", UKRAINIAN )
	LANG_ENTRY_END( UKRAINIAN )
/*x23*/ LANG_ENTRY_BEGIN( "be", BYELORUSSIAN )
	LANG_ENTRY_END( BYELORUSSIAN )
/*x24*/ LANG_ENTRY_BEGIN( "sl", SLOVENIAN )
	LANG_ENTRY_END( SLOVENIAN )
/*x25*/ LANG_ENTRY_BEGIN( "et", ESTONIAN )
	LANG_ENTRY_END( ESTONIAN )
/*x26*/ LANG_ENTRY_BEGIN( "lv", LATVIAN )
	LANG_ENTRY_END( LATVIAN )
/*x27*/ LANG_ENTRY_BEGIN( "lt", LITHUANIAN )
	/* traditional sorting ? */
	if (!strcmp(dialect,"classic") || !strcmp(dialect,"traditional"))
		LANG_DIALECT_ENTRY( LITHUANIAN, LITHUANIAN_CLASSIC )
	else
		LANG_DIALECT_ENTRY( LITHUANIAN, LITHUANIAN )
	LANG_ENTRY_END( LITHUANIAN )
/*x28*/ LANG_ENTRY_BEGIN( "mi", MAORI )
	LANG_ENTRY_END( MAORI )
/*x29*/ LANG_ENTRY_BEGIN( "fa", FARSI )
	LANG_ENTRY_END( FARSI )
/*x2a*/ LANG_ENTRY_BEGIN( "vi", VIETNAMESE )
	LANG_ENTRY_END( VIETNAMESE )
/*x2b*/ LANG_ENTRY_BEGIN( "hy", ARMENIAN )
	LANG_ENTRY_END( ARMENIAN )
/*x2c*/ LANG_ENTRY_BEGIN( "az", AZERI )
	/* Cyrillic */
	if (strstr(charset,"KOI8") || !strcmp(charset,"ISO-8859-5"))
		LANG_DIALECT_ENTRY( AZERI, AZERI_CYRILLIC )
	else
		LANG_DIALECT_ENTRY( AZERI, AZERI )
	LANG_ENTRY_END( AZERI )
/*x2d*/ LANG_ENTRY_BEGIN( "eu", BASQUE )
	LANG_ENTRY_END( BASQUE )
/*x2e*/ /*LANG_ENTRY_BEGIN( "??", SORBIAN )
	LANG_ENTRY_END( SORBIAN ) */
/*x2f*/ LANG_ENTRY_BEGIN( "mk", MACEDONIAN )
	LANG_ENTRY_END( MACEDONIAN )
/*x30*/ /*LANG_ENTRY_BEGIN( "??", SUTU )
	LANG_ENTRY_END( SUTU ) */
/*x31*/ LANG_ENTRY_BEGIN( "ts", TSONGA )
	LANG_ENTRY_END( TSONGA )
/*x32*/ /*LANG_ENTRY_BEGIN( "??", TSWANA )
	LANG_ENTRY_END( TSWANA ) */
/*x33*/ /*LANG_ENTRY_BEGIN( "??", VENDA )
	LANG_ENTRY_END( VENDA ) */
/*x34*/ LANG_ENTRY_BEGIN( "xh", XHOSA )
	LANG_ENTRY_END( XHOSA )
/*x35*/ LANG_ENTRY_BEGIN( "zu", ZULU )
	LANG_ENTRY_END( ZULU )
/*x36*/ LANG_ENTRY_BEGIN( "af", AFRIKAANS )
	LANG_ENTRY_END( AFRIKAANS )
/*x37*/ LANG_ENTRY_BEGIN( "ka", GEORGIAN )
	LANG_ENTRY_END( GEORGIAN )
/*x38*/ LANG_ENTRY_BEGIN( "fo", FAEROESE )
	LANG_ENTRY_END( FAEROESE )
/*x39*/ LANG_ENTRY_BEGIN( "hi", HINDI )
	LANG_ENTRY_END( HINDI )
/*x3a*/ LANG_ENTRY_BEGIN( "mt", MALTESE )
	LANG_ENTRY_END( MALTESE )
/*x3b*/ /*LANG_ENTRY_BEGIN( "??", SAAMI )
	LANG_ENTRY_END( SAAMI ) */
/*x3c*/ LANG_ENTRY_BEGIN( "ga", GAELIC )
	LANG_DIALECT_ENTRY( GAELIC, GAELIC )
	LANG_ENTRY_END( GAELIC )
/*x3c*/ LANG_ENTRY_BEGIN( "gd", GAELIC )
	LANG_DIALECT_ENTRY( GAELIC, GAELIC_SCOTTISH )
	LANG_ENTRY_END( GAELIC )
/* 0x3d */
/*x3e*/ LANG_ENTRY_BEGIN( "ms", MALAY )
	LANG_SUB_ENTRY( "MY", MALAY, MALAY )
	LANG_SUB_ENTRY( "BN", MALAY, MALAY_BRUNEI_DARUSSALAM )
	LANG_ENTRY_END( MALAY )
/*x3f*/ LANG_ENTRY_BEGIN( "kk", KAZAKH )
	LANG_ENTRY_END( KAZAKH )
/* 0x40 */
/*x41*/ LANG_ENTRY_BEGIN( "sw", SWAHILI )
	LANG_ENTRY_END( SWAHILI )
/* 0x42 */
/*x43*/ LANG_ENTRY_BEGIN( "uz", UZBEK )
        /* Cyrillic */
	if (strstr(charset,"KOI8") || !strcmp(charset,"ISO-8859-5"))
		LANG_DIALECT_ENTRY( UZBEK, UZBEK_CYRILLIC )
	else
		LANG_DIALECT_ENTRY( UZBEK, UZBEK )
	LANG_ENTRY_END( UZBEK )
/*x44*/ LANG_ENTRY_BEGIN( "tt", TATAR )
	LANG_ENTRY_END( TATAR )
/*x45*/ LANG_ENTRY_BEGIN( "bn", BENGALI )
	LANG_ENTRY_END( BENGALI )
/*x46*/ LANG_ENTRY_BEGIN( "pa", PUNJABI )
	LANG_ENTRY_END( PUNJABI )
/*x47*/ LANG_ENTRY_BEGIN( "gu", GUJARATI )
	LANG_ENTRY_END( GUJARATI )
/*x48*/ LANG_ENTRY_BEGIN( "or", ORIYA )
	LANG_ENTRY_END( ORIYA )
/*x49*/ LANG_ENTRY_BEGIN( "ta", TAMIL )
	LANG_ENTRY_END( TAMIL )
/*x4a*/ LANG_ENTRY_BEGIN( "te", TELUGU )
	LANG_ENTRY_END( TELUGU )
/*x4b*/ LANG_ENTRY_BEGIN( "kn", KANNADA )
	LANG_ENTRY_END( KANNADA )
/*x4c*/ LANG_ENTRY_BEGIN( "ml", MALAYALAM )
	LANG_ENTRY_END( MALAYALAM )
/*x4d*/ LANG_ENTRY_BEGIN( "as", ASSAMESE )
	LANG_ENTRY_END( ASSAMESE )
/*x4e*/ LANG_ENTRY_BEGIN( "mr", MARATHI )
	LANG_ENTRY_END( MARATHI )
/*x4f*/ LANG_ENTRY_BEGIN( "sa", SANSKRIT )
	LANG_ENTRY_END( SANSKRIT )
/* 0x50 -> 0x56 */
/*x57*/ /*LANG_ENTRY_BEGIN( "??", KONKANI )
	LANG_ENTRY_END( KONKANI ) */
/* 0x58 -> ... */
	LANG_ENTRY_BEGIN( "eo", ESPERANTO ) /* not official */
	LANG_ENTRY_END( ESPERANTO )

	ret = LANG_ENGLISH;

end_MAIN_GetLanguageID:
	if (Charset) free(charset);
	if (Dialect) free(dialect);

	return ret;
}

/***********************************************************************
 *           MAIN_ParseLanguageOption
 *
 * Parse -language option.
 */
static void MAIN_ParseLanguageOption( char *arg )
{
    const WINE_LANGUAGE_DEF *p = Languages;

/* for compatibility whith non-iso names previously used */
    if (!strcmp("Sw",arg)) { strcpy(arg,"Sv"); FIXME(system,"use 'Sv' instead of 'Sw'\n");}
    if (!strcmp("Cz",arg)) { strcpy(arg,"Cs"); FIXME(system,"use 'Cs' instead of 'Cz'\n");}
    if (!strcmp("Po",arg)) { strcpy(arg,"Pt"); FIXME(system,"use 'Pt' instead of 'Po'\n");}

    Options.language = LANG_Xx;  /* First (dummy) language */
    for (;p->name;p++)
    {
        if (!lstrcmpi32A( p->name, arg ))
        {
	    WINE_LanguageId = p->langid;
	    return;
	}
        Options.language++;
    }
    MSG( "Invalid language specified '%s'. Supported languages are: ", arg );
    for (p = Languages; p->name; p++) MSG( "%s ", p->name );
    MSG( "\n" );
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
        MSG( "Invalid mode '%s' specified.\n", arg);
        MSG( "Valid modes are: 'standard', 'enhanced' (default).\n");
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
    XrmDatabase db = TSXrmGetFileDatabase(WINE_APP_DEFAULTS);
    int i;
    char *xrm_string;

    Options.programName = MAIN_GetProgramName( *argc, argv );
    Options.argv0 = argv[0];

    /* initialise Options.language to 0 to tell "no language choosen yet" */
    Options.language = 0;
  
      /* Get display name from command line */
    for (i = 1; i < *argc; i++)
    {
        if (!strcmp( argv[i], "-display" )) display_name = argv[i+1];
        if (!strcmp( argv[i], "-v" ) || !strcmp( argv[i], "-version" ))
        {
            MSG( "%s\n", WINE_RELEASE_INFO );
            exit(0);
        }
        if (!strcmp( argv[i], "-h" ) || !strcmp( argv[i], "-help" ))
        {
            MAIN_Usage(argv[0]);
            exit(0);
        }
    }

      /* Open display */

    if (display_name == NULL &&
	MAIN_GetResource( db, ".display", &value )) display_name = value.addr;

    if (!(display = TSXOpenDisplay( display_name )))
    {
	MSG( "%s: Can't open display: %s\n",
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
    if (MAIN_GetResource( db, ".debugoptions", &value))
	MAIN_ParseDebugOptions((char*)value.addr);
    if (MAIN_GetResource( db, ".debugmsg", &value))
      {
#ifndef DEBUG_RUNTIME
	MSG("%s: Option \"-debugmsg\" not implemented.\n" \
          "    Recompile with DEBUG_RUNTIME in include/debugtools.h defined.\n",
	  argv[0]);
	exit(1);
#else
	if (MAIN_ParseDebugOptions((char*)value.addr)==FALSE)
	  {
	    int i;
	    MSG("%s: Syntax: -debugmsg [class]+xxx,...  or "
		    "-debugmsg [class]-xxx,...\n",argv[0]);
	    MSG("Example: -debugmsg +all,warn-heap\n"
		    "  turn on all messages except warning heap messages\n");
	    MSG("Special case: -debugmsg +relay=DLL:DLL.###:FuncName\n"
		"  turn on -debugmsg +relay only as specified\n"
		"Special case: -debugmsg -relay=DLL:DLL.###:FuncName\n"
		"  turn on -debugmsg +relay except as specified\n"
		"Also permitted, +snoop=..., -snoop=... as with relay.\n\n");

	    MSG("Available message classes:\n");
	    for(i=0;i<DEBUG_CLASS_COUNT;i++)
	      MSG( "%-9s", debug_cl_name[i]);
	    MSG("\n\n");

	    MSG("Available message types:\n");
	    MSG("%-9s ","all");
	    for(i=0;i<DEBUG_CHANNEL_COUNT;i++)
	      if(debug_ch_name[i])
		MSG("%-9s%c",debug_ch_name[i],
			(((i+2)%8==0)?'\n':' '));
	    MSG("\n\n");
	    exit(1);
	  }
#endif
      }

      if (MAIN_GetResource( db, ".dll", &value))
      {
          /* Hack: store option value in Options to be retrieved */
          /* later on inside the emulator code. */
          if (!__winelib)
                        {
                                if (Options.dllFlags)
                                {
                                        /* don't overwrite previous value. Should we
                                         * automatically add the ',' between multiple DLLs ?
                                         */
                                        MSG("Only one -dll flag is allowed. Use ',' between multiple DLLs\n");
                                }
                                else
                                {
                                        Options.dllFlags = xstrdup((char *)value.addr);
                                }
                        }
          else
          {
              MSG("-dll not supported in Winelib\n" );
              exit(1);
          }
      }

      if (MAIN_GetResource( db, ".winver", &value))
          VERSION_ParseWinVersion( (char*)value.addr );
      if (MAIN_GetResource( db, ".dosver", &value))
          VERSION_ParseDosVersion( (char*)value.addr );
      if (MAIN_GetResource( db, ".config", &value))
         Options.configFileName = xstrdup((char *)value.addr);
      if (MAIN_GetResource( db, ".nodga", &value))
	 Options.noDGA = TRUE;
      if (MAIN_GetResource( db, ".console", &value))
         Options.consoleDrivers = xstrdup((char *)value.addr);
      else
         Options.consoleDrivers = CONSOLE_DEFAULT_DRIVER;

      CONSOLE_Init(Options.consoleDrivers);
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

    flags = TSXParseGeometry( Options.desktopGeometry, &x, &y, &width, &height );
    screenWidth  = width;
    screenHeight = height;

      /* Create window */

    win_attr.background_pixel = BlackPixel(display,0);
    win_attr.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
	                 PointerMotionMask | ButtonPressMask |
			 ButtonReleaseMask | EnterWindowMask;
    win_attr.cursor = TSXCreateFontCursor( display, XC_top_left_arrow );

    rootWindow = TSXCreateWindow( display, DefaultRootWindow(display),
			        x, y, width, height, 0,
			        CopyFromParent, InputOutput, CopyFromParent,
			        CWBackPixel | CWEventMask | CWCursor, &win_attr );

      /* Set window manager properties */

    size_hints  = TSXAllocSizeHints();
    wm_hints    = TSXAllocWMHints();
    class_hints = TSXAllocClassHint();
    if (!size_hints || !wm_hints || !class_hints)
    {
        MSG("Not enough memory for window manager hints.\n" );
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

    TSXStringListToTextProperty( &name, 1, &window_name );
    TSXSetWMProperties( display, rootWindow, &window_name, &window_name,
                      argv, argc, size_hints, wm_hints, class_hints );
    XA_WM_DELETE_WINDOW = TSXInternAtom( display, "WM_DELETE_WINDOW", False );
    TSXSetWMProtocols( display, rootWindow, &XA_WM_DELETE_WINDOW, 1 );
    TSXFree( size_hints );
    TSXFree( wm_hints );
    TSXFree( class_hints );

      /* Map window */

    TSXMapWindow( display, rootWindow );
}


XKeyboardState keyboard_state;

/***********************************************************************
 *           MAIN_SaveSetup
 */
static void MAIN_SaveSetup(void)
{
    TSXGetKeyboardControl(display, &keyboard_state);
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


/***********************************************************************
 *           called_at_exit
 */
static void called_at_exit(void)
{
    MAIN_RestoreSetup();
    COLOR_Cleanup();
    WINSOCK_Shutdown();
    /* FIXME: should check for other processes or threads */
    DeleteCriticalSection( HEAP_SystemLock );
    CONSOLE_Close();
}

static int WINE_X11_ErrorHandler(Display *display,XErrorEvent *error_evt)
{
    kill( getpid(), SIGHUP ); /* force an entry in the debugger */
    return 0;
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
        MSG( "MALLOC_TRACE not set. No trace generated\n" );
    }
    else
    {
        MSG( "malloc trace goes to %s\n", trace );
        mtrace();
    }
#endif

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    setlocale(LC_CTYPE,"");
    gettimeofday( &tv, NULL);
    MSG_WineStartTicks = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);

    /* We need this before calling any Xlib function */
    InitializeCriticalSection( &X11DRV_CritSection );
    MakeCriticalSectionGlobal( &X11DRV_CritSection );

    TSXrmInitialize();

    putenv("XKB_DISABLE="); /* Disable XKB extension if present. */

    MAIN_ParseOptions( argc, argv );

    if (Options.synchronous) XSetErrorHandler( WINE_X11_ErrorHandler );

    if (Options.desktopGeometry && Options.managed)
    {
#if 0
        MSG( "%s: -managed and -desktop options cannot be used together\n",
                 Options.programName );
        exit(1);
#else
        Options.managed = FALSE;
#endif
    }

    screen       = DefaultScreenOfDisplay( display );
    screenWidth  = WidthOfScreen( screen );
    screenHeight = HeightOfScreen( screen );
    if (screenDepth)  /* -depth option specified */
    {
	depth_list = TSXListDepths(display,DefaultScreen(display),&depth_count);
	for (i = 0; i < depth_count; i++)
	    if (depth_list[i] == screenDepth) break;
	TSXFree( depth_list );
	if (i >= depth_count)
	{
	    MSG( "%s: Depth %d not supported on this screen.\n",
		              Options.programName, screenDepth );
	    exit(1);
	}
    }
    else screenDepth  = DefaultDepthOfScreen( screen );
    if (Options.synchronous) TSXSynchronize( display, True );
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
 *           MessageBeep32   (USER32.390)
 */
BOOL32 WINAPI MessageBeep32( UINT32 i )
{
    TSXBell( display, 0 );
    return TRUE;
}


/***********************************************************************
 *           Beep   (KERNEL32.11)
 */
BOOL32 WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    /* dwFreq and dwDur are ignored by Win95 */
    TSXBell(display, 0);
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
 *	SystemParametersInfo32A   (USER32.540)
 */
BOOL32 WINAPI SystemParametersInfo32A( UINT32 uAction, UINT32 uParam,
                                       LPVOID lpvParam, UINT32 fuWinIni )
{
	int timeout, temp;
	XKeyboardState		keyboard_state;

	switch (uAction) {
	case SPI_GETBEEP:
		TSXGetKeyboardControl(display, &keyboard_state);
		if (keyboard_state.bell_percent == 0)
			*(BOOL32 *) lpvParam = FALSE;
		else
			*(BOOL32 *) lpvParam = TRUE;
		break;

	case SPI_GETBORDER:
		*(INT32 *)lpvParam = GetSystemMetrics32( SM_CXFRAME );
		break;

	case SPI_GETDRAGFULLWINDOWS:
		*(BOOL32 *) lpvParam = FALSE;
		break;

	case SPI_SETDRAGFULLWINDOWS:
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
		TSXGetScreenSaver(display, &timeout, &temp,&temp,&temp);
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
		    lpnm->lfCaptionFont.lfWeight = FW_BOLD;
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfSmCaptionFont),0);
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMenuFont),0);
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfStatusFont),0);
		    SystemParametersInfo32A(SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfMessageFont),0);
		}
#undef lpnm
		break;

        case SPI_GETANIMATION: {
                LPANIMATIONINFO lpAnimInfo = (LPANIMATIONINFO)lpvParam;
 
                /* Tell it "disabled" */
                lpAnimInfo->cbSize = sizeof(ANIMATIONINFO);
                uParam = sizeof(ANIMATIONINFO);
                lpAnimInfo->iMinAnimate = 0; /* Minimise and restore animation is disabled (nonzero == enabled) */
                break;
        }
 
        case SPI_SETANIMATION: {
                LPANIMATIONINFO lpAnimInfo = (LPANIMATIONINFO)lpvParam;
 
                /* Do nothing */
                WARN(system, "SPI_SETANIMATION ignored.\n");
                lpAnimInfo->cbSize = sizeof(ANIMATIONINFO);
                uParam = sizeof(ANIMATIONINFO);
                break;
        }

        case SPI_GETHIGHCONTRAST:
        {
                LPHIGHCONTRASTA lpHighContrastA = (LPHIGHCONTRASTA)lpvParam;

                FIXME(system,"SPI_GETHIGHCONTRAST not fully implemented\n");

                if ( lpHighContrastA->cbSize == sizeof( HIGHCONTRASTA ) )
                {
                        /* Indicate that there is no high contrast available */
                        lpHighContrastA->dwFlags = 0;
                        lpHighContrastA->lpszDefaultScheme = NULL;
                }
                else
                {
                        return FALSE;
                }

                break;
        }

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
			TSXGetKeyboardControl(display, &keyboard_state);
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
			TSXGetScreenSaver(display, &timeout, &temp,&temp,&temp);
			*(INT16 *) lpvParam = timeout;
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
   			TSXChangeKeyboardControl(display, KBBellPercent, 
   							&keyboard_value);
			break;

		case SPI_SETSCREENSAVEACTIVE:
			if (uParam == TRUE)
				TSXActivateScreenSaver(display);
			else
				TSXResetScreenSaver(display);
			break;

		case SPI_SETSCREENSAVETIMEOUT:
			TSXSetScreenSaver(display, uParam, 60, DefaultBlanking, 
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
			lpnm->lfCaptionFont.lfWeight = FW_BOLD;
			SystemParametersInfo16( SPI_GETICONTITLELOGFONT, 0,
							(LPVOID)&(lpnm->lfSmCaptionFont),0);
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
			WARN(system, "Option %d ignored.\n", uAction);
			break;

                case SPI_GETWORKAREA:
                    SetRect16( (RECT16 *)lpvParam, 0, 0,
                               GetSystemMetrics16( SM_CXSCREEN ),
                               GetSystemMetrics16( SM_CYSCREEN ) );
                    break;

		default:
			WARN(system, "Unknown option %d.\n", uAction);
			break;
	}
	return 1;
}

/***********************************************************************
 *	SystemParametersInfo32W   (USER32.541)
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
            LPLOGFONT32W lpLogFont = (LPLOGFONT32W)lpvParam;
 	    GetProfileString32A("Desktop", "IconTitleFaceName", "MS Sans Serif", 
			 buffer, sizeof(buffer) );
	    lstrcpynAtoW(lpLogFont->lfFaceName, buffer ,LF_FACESIZE);
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
	lpnm->lfCaptionFont.lfWeight = FW_BOLD;
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfSmCaptionFont),0);
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfMenuFont),0);
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfStatusFont),0);
	SystemParametersInfo32W(SPI_GETICONTITLELOGFONT,0,(LPVOID)&(lpnm->lfMessageFont),0);
	break;
    }

    case SPI_GETHIGHCONTRAST:
    {
       LPHIGHCONTRASTW lpHighContrastW = (LPHIGHCONTRASTW)lpvParam;

       FIXME(system,"SPI_GETHIGHCONTRAST not fully implemented\n");

       if ( lpHighContrastW->cbSize == sizeof( HIGHCONTRASTW ) )
       {
          /* Indicate that there is no high contrast available */
          lpHighContrastW->dwFlags = 0;
          lpHighContrastW->lpszDefaultScheme = NULL;
       }
       else
       {
          return FALSE;
       }

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
FARPROC16 WINAPI FileCDR(FARPROC16 x)
{
	FIXME(file,"(0x%8x): stub\n", (int) x);
	return (FARPROC16)TRUE;
}
