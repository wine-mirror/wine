/*
 * Main function.
 *
 * Copyright 1994 Alexandre Julliard
 */

#include "config.h"

#include <locale.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef MALLOC_DEBUGGING
# include <malloc.h>
#endif

#include "winbase.h"
#include "winsock.h"
#include "heap.h"
#include "message.h"
#include "msdos.h"
#include "color.h"
#include "options.h"
#include "builtin32.h"
#include "debugtools.h"
#include "debugdefs.h"
#include "module.h"
#include "version.h"
#include "winnls.h"
#include "console.h"
#include "monitor.h"
#include "keyboard.h"
#include "gdi.h"
#include "user.h"
#include "windef.h"
#include "wingdi.h"
#include "wine/winuser16.h"
#include "tweak.h"
#include "winerror.h"

/**********************************************************************/

USER_DRIVER *USER_Driver = NULL;

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
    {"Sk",0x041b},	/* LANG_Sk */
    {"Sv",0x041d},	/* LANG_Sv */
    {"Ca",0x0403},	/* LANG_Ca */
    {"Nl",0x0413},	/* LANG_Nl */
    {"Ru",0x0419},	/* LANG_Ru */
    {"Wa",0x0490},      /* LANG_Wa */
    {"Ga",0x043c},	/* LANG_Ga */
    {"Gd",0x083c},	/* LANG_Gd */
    {"Gv",0x0c3c},	/* LANG_Gv */
    {"Kw",0x0491},	/* LANG_Kw */
    {"Cy",0x0492},	/* LANG_Cy */
    {"Br",0x0493},	/* LANG_Br  */
    {"Ja",0x0411},	/* LANG_Ja */
    {NULL,0}
};

WORD WINE_LanguageId = 0x409;	/* english as default */

struct options Options =
{  /* default options */
    0,              /* argc */
    NULL,           /* argv */
    NULL,           /* desktopGeometry */
    NULL,           /* display */
    NULL,           /* dllFlags */
    FALSE,          /* synchronous */
    0,              /* language */
    FALSE,          /* Managed windows */
    NULL            /* Alternate config file name */
};

const char *argv0;

/***********************************************************************
 *          MAIN_ParseDebugOptions
 *
 *  Turns specific debug messages on or off, according to "options".
 */
void MAIN_ParseDebugOptions( const char *arg )
{
  /* defined in relay32/relay386.c */
  extern char **debug_relay_includelist;
  extern char **debug_relay_excludelist;
  /* defined in relay32/snoop.c */
  extern char **debug_snoop_includelist;
  extern char **debug_snoop_excludelist;

  int i;
  int l, cls, dotracerelay = TRACE_ON(relay);

  char *options = strdup(arg);

  l = strlen(options);
  if (l<2) goto error;

  if (options[l-1]=='\n') options[l-1]='\0';
  do
  {
    if ((*options!='+')&&(*options!='-')){
      int j;

      for(j=0; j<DEBUG_CLASS_COUNT; j++)
	if(!lstrncmpiA(options, debug_cl_name[j], strlen(debug_cl_name[j])))
	  break;
      if(j==DEBUG_CLASS_COUNT)
	goto error;
      options += strlen(debug_cl_name[j]);
      if ((*options!='+')&&(*options!='-'))
	goto error;
      cls = j;
    }
    else
      cls = -1; /* all classes */

    if (strchr(options,','))
      l=strchr(options,',')-options;
    else
      l=strlen(options);

    if (!lstrncmpiA(options+1,"all",l-1))
      {
	int i, j;
	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  for(j=0; j<DEBUG_CLASS_COUNT; j++)
	    if(cls == -1 || cls == j)
                __SET_DEBUGGING( j, debug_channels[i], (*options=='+') );
      }
    else if (!lstrncmpiA(options+1, "relay=", 6) ||
	     !lstrncmpiA(options+1, "snoop=", 6))
      {
	int i, j;
	char *s, *s2, ***output, c;

	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
	  if (!strncasecmp( debug_channels[i] + 1, options + 1, 5))
          {
	    for(j=0; j<DEBUG_CLASS_COUNT; j++)
	      if(cls == -1 || cls == j)
                  __SET_DEBUGGING( j, debug_channels[i], 1 );
	    break;
	  }
	/* should never happen, maybe assert(i!=DEBUG_CHANNEL_COUNT)? */
	if (i==DEBUG_CHANNEL_COUNT)
	  goto error;
	output = (*options == '+') ?
			((*(options+1) == 'r') ?
				&debug_relay_includelist :
				&debug_snoop_includelist) :
			((*(options+1) == 'r') ?
				&debug_relay_excludelist :
				&debug_snoop_excludelist);
	s = options + 7;
	/* if there are n ':', there are n+1 modules, and we need n+2 slots
	 * last one being for the sentinel (NULL) */
	i = 2;	
	while((s = strchr(s, ':'))) i++, s++;
	*output = malloc(sizeof(char **) * i);
	i = 0;
	s = options + 7;
	while((s2 = strchr(s, ':'))) {
          c = *s2;
          *s2 = '\0';
	  *((*output)+i) = CharUpperA(strdup(s));
          *s2 = c;
	  s = s2 + 1;
	  i++;
	}
	c = *(options + l);
	*(options + l) = '\0';
	*((*output)+i) = CharUpperA(strdup(s));
	*(options + l) = c;
	*((*output)+i+1) = NULL;
      }
    else
      {
	int i, j;
	for (i=0; i<DEBUG_CHANNEL_COUNT; i++)
          if (!strncasecmp( debug_channels[i] + 1, options + 1, l - 1) && !debug_channels[i][l])
          {
	    for(j=0; j<DEBUG_CLASS_COUNT; j++)
	      if(cls == -1 || cls == j)
                  __SET_DEBUGGING( j, debug_channels[i], (*options=='+') );
	    break;
	  }
	if (i==DEBUG_CHANNEL_COUNT)
	  goto error;
      }
    options+=l;
  }
  while((*options==',')&&(*(++options)));

  /* special handling for relay debugging */
  if (dotracerelay != TRACE_ON(relay))
  	BUILTIN32_SwitchRelayDebug( TRACE_ON(relay) );

  if (!*options) return;

 error:  
  MESSAGE("%s: Syntax: -debugmsg [class]+xxx,...  or "
      "-debugmsg [class]-xxx,...\n",argv0);
  MESSAGE("Example: -debugmsg +all,warn-heap\n"
      "  turn on all messages except warning heap messages\n");
  MESSAGE("Special case: -debugmsg +relay=DLL:DLL.###:FuncName\n"
      "  turn on -debugmsg +relay only as specified\n"
      "Special case: -debugmsg -relay=DLL:DLL.###:FuncName\n"
      "  turn on -debugmsg +relay except as specified\n"
      "Also permitted, +snoop=..., -snoop=... as with relay.\n\n");
  
  MESSAGE("Available message classes:\n");
  for(i=0;i<DEBUG_CLASS_COUNT;i++)
    MESSAGE( "%-9s", debug_cl_name[i]);
  MESSAGE("\n\n");
  
  MESSAGE("Available message types:\n");
  MESSAGE("%-9s ","all");
  for(i=0;i<DEBUG_CHANNEL_COUNT;i++)
      MESSAGE("%-9s%c",debug_channels[i] + 1,
	  (((i+2)%8==0)?'\n':' '));
  MESSAGE("\n\n");
  ExitProcess(1);
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
    } else {
    	dialect = malloc(1);
	dialect[0] = '\0';
    }

#define LANG_ENTRY_BEGIN(x,y)	if(!strcmp(lang, x )) { \
				    if (!country[0]) { \
					ret=LANG_##y ; \
					goto end_MAIN_GetLanguageID; \
				    }
#define LANG_SUB_ENTRY(x,y,z)	    if (!strcmp(country, x )) { \
					ret = MAKELANGID( LANG_##y , SUBLANG_##z ); \
					goto end_MAIN_GetLanguageID; \
				    }
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
	LANG_SUB_ENTRY( "GB", ENGLISH, ENGLISH_UK )
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
	LANG_ENTRY_BEGIN( "wa", WALON )	/* not official */ 
	LANG_ENTRY_END( WALON )

	ret = LANG_ENGLISH;

end_MAIN_GetLanguageID:
	if (Charset) free(charset);
	free(dialect);

	return ret;
}

/***********************************************************************
 *           MAIN_ParseLanguageOption
 *
 * Parse -language option.
 */
void MAIN_ParseLanguageOption( const char *arg )
{
    const WINE_LANGUAGE_DEF *p = Languages;

    Options.language = LANG_Xx;  /* First (dummy) language */
    for (;p->name;p++)
    {
        if (!lstrcmpiA( p->name, arg ))
        {
	    WINE_LanguageId = p->langid;
	    return;
	}
        Options.language++;
    }
    MESSAGE( "Invalid language specified '%s'. Supported languages are: ", arg );
    for (p = Languages; p->name; p++) MESSAGE( "%s ", p->name );
    MESSAGE( "\n" );
    ExitProcess(1);
}


/***********************************************************************
 *           called_at_exit
 */
static void called_at_exit(void)
{
    CONSOLE_Close();
}

/***********************************************************************
 *           MAIN_WineInit
 *
 * Wine initialisation and command-line parsing
 */
BOOL MAIN_WineInit( int argc, char *argv[] )
{
    struct timeval tv;

#ifdef MALLOC_DEBUGGING
    char *trace;

    mcheck(NULL);
    if (!(trace = getenv("MALLOC_TRACE")))
    {       
        MESSAGE( "MALLOC_TRACE not set. No trace generated\n" );
    }
    else
    {
        MESSAGE( "malloc trace goes to %s\n", trace );
        mtrace();
    }
#endif

    setbuf(stdout,NULL);
    setbuf(stderr,NULL);

    setlocale(LC_CTYPE,"");
    gettimeofday( &tv, NULL);
    MSG_WineStartTicks = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    
    OPTIONS_ParseOptions( argc, argv );

    atexit(called_at_exit);
    return TRUE;
}

/***********************************************************************
 *           MessageBeep16   (USER.104)
 */
void WINAPI MessageBeep16( UINT16 i )
{
    MessageBeep( i );
}


/***********************************************************************
 *           MessageBeep   (USER32.390)
 */
BOOL WINAPI MessageBeep( UINT i )
{
    KEYBOARD_Beep();
    return TRUE;
}


/***********************************************************************
 *           Beep   (KERNEL32.11)
 */
BOOL WINAPI Beep( DWORD dwFreq, DWORD dwDur )
{
    /* dwFreq and dwDur are ignored by Win95 */
    KEYBOARD_Beep();
    return TRUE;
}


/***********************************************************************
*	FileCDR (KERNEL.130)
*/
FARPROC16 WINAPI FileCDR16(FARPROC16 x)
{
	FIXME_(file)("(0x%8x): stub\n", (int) x);
	return (FARPROC16)TRUE;
}

/***********************************************************************
 *           GetTickCount   (USER.13) (KERNEL32.299)
 *
 * Returns the number of milliseconds, modulo 2^32, since the start
 * of the current session.
 */
DWORD WINAPI GetTickCount(void)
{
    struct timeval t;
    gettimeofday( &t, NULL );
    return ((t.tv_sec * 1000) + (t.tv_usec / 1000)) - MSG_WineStartTicks;
}
