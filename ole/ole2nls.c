/*
 *	OLE2NLS library
 *
 *	Copyright 1995	Martin von Loewis
 *      Copyright 1998  David Lee Lambert
 *      Copyright 2000  Julio CÈsar G·zquez
 */

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <locale.h>
#include "windef.h"
#include "wingdi.h"
#include "winuser.h"
#include "heap.h"
#include "options.h"
#include "winver.h"
#include "winnls.h"
#include "winreg.h"
#include "winerror.h"
#include "debugtools.h"
#include "crtdll.h"
#include "main.h"

DEFAULT_DEBUG_CHANNEL(ole);
DECLARE_DEBUG_CHANNEL(string);
DECLARE_DEBUG_CHANNEL(win32);

struct NLS_langlocale {
	const int lang;
	struct NLS_localevar {
		const int	type;
		const char	*val;
	} locvars[120];
};

static LPVOID lpNLSInfo = NULL;

#define LANG_BEGIN(l,s)	{	MAKELANGID(l,s), {

#define LOCVAL(type,value)					{type,value},

#define LANG_END					 }},

static const struct NLS_langlocale langlocales[] = {
/* add languages in numerical order of main language (last two digits)
 * it is much easier to find the missing holes that way */

LANG_BEGIN (LANG_CATALAN, SUBLANG_DEFAULT)	/*0x0403*/
#include "nls/cat.nls"
LANG_END

LANG_BEGIN (LANG_CZECH, SUBLANG_DEFAULT)	/*0x0405*/
#include "nls/cze.nls"
LANG_END

LANG_BEGIN (LANG_DANISH, SUBLANG_DEFAULT)	/*0x0406*/
#include "nls/dan.nls"
LANG_END

LANG_BEGIN (LANG_GERMAN, SUBLANG_GERMAN)		/*0x0407*/
#include "nls/deu.nls"
LANG_END
LANG_BEGIN (LANG_GERMAN, SUBLANG_GERMAN_SWISS)		/*0x0807*/
#include "nls/des.nls"
LANG_END
LANG_BEGIN (LANG_GERMAN, SUBLANG_GERMAN_AUSTRIAN)	/*0x0C07*/
#include "nls/dea.nls"
LANG_END
LANG_BEGIN (LANG_GERMAN, SUBLANG_GERMAN_LUXEMBOURG)	/*0x1007*/
#include "nls/del.nls"
LANG_END
LANG_BEGIN (LANG_GERMAN, SUBLANG_GERMAN_LIECHTENSTEIN)	/*0x1407*/
#include "nls/dec.nls"
LANG_END

LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_US)		/*0x0409*/
#include "nls/enu.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_UK)		/*0x0809*/
#include "nls/eng.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_AUS)		/*0x0C09*/
#include "nls/ena.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_CAN)		/*0x1009*/
#include "nls/enc.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_NZ)		/*0x1409*/
#include "nls/enz.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_EIRE)		/*0x1809*/
#include "nls/eni.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_SAFRICA)	/*0x1C09*/
#include "nls/ens.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_JAMAICA)	/*0x2009*/
#include "nls/enj.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_CARRIBEAN)	/*0x2409*/
#include "nls/enb.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_BELIZE)	/*0x2809*/
#include "nls/enl.nls"
LANG_END
LANG_BEGIN (LANG_ENGLISH, SUBLANG_ENGLISH_TRINIDAD)    	/*0x2C09*/
#include "nls/ent.nls"
LANG_END

LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH)		/*0x040a*/
#include "nls/esp.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_MEXICAN)	/*0x080a*/
#include "nls/esm.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_MODERN)	/*0x0C0a*/
#include "nls/esn.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_GUATEMALA)	/*0x100a*/
#include "nls/esg.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_COSTARICA)	/*0x140a*/
#include "nls/esc.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_PANAMA)	/*0x180a*/
#include "nls/esa.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_DOMINICAN)	/*0x1C0A*/
#include "nls/esd.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_VENEZUELA)	/*0x200a*/
#include "nls/esv.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_COLOMBIA)	/*0x240a*/
#include "nls/eso.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_PERU)		/*0x280a*/
#include "nls/esr.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_ARGENTINA)	/*0x2c0a*/
#include "nls/ess.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_ECUADOR)	/*0x300a*/
#include "nls/esf.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_CHILE)	/*0x340a*/
#include "nls/esl.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_URUGUAY)	/*0x380a*/
#include "nls/esy.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_PARAGUAY)	/*0x3c0a*/
#include "nls/esz.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_BOLIVIA)	/*0x400a*/
#include "nls/esb.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_EL_SALVADOR)	/*0x440a*/
#include "nls/ese.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_HONDURAS)	/*0x480a*/
#include "nls/esh.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_NICARAGUA)	/*0x4c0a*/
#include "nls/esi.nls"
LANG_END
LANG_BEGIN (LANG_SPANISH, SUBLANG_SPANISH_PUERTO_RICO)	/*0x500a*/
#include "nls/esu.nls"
LANG_END

LANG_BEGIN (LANG_FINNISH, SUBLANG_DEFAULT)	/*0x040B*/
#include "nls/fin.nls"
LANG_END

LANG_BEGIN (LANG_FRENCH, SUBLANG_FRENCH)		/*0x040C*/
#include "nls/fra.nls"
LANG_END
LANG_BEGIN (LANG_FRENCH, SUBLANG_FRENCH_BELGIAN)	/*0x080C*/
#include "nls/frb.nls"
LANG_END
LANG_BEGIN (LANG_FRENCH, SUBLANG_FRENCH_CANADIAN)	/*0x0C0C*/
#include "nls/frc.nls"
LANG_END
LANG_BEGIN (LANG_FRENCH, SUBLANG_FRENCH_SWISS)		/*0x100C*/
#include "nls/frs.nls"
LANG_END
LANG_BEGIN (LANG_FRENCH, SUBLANG_FRENCH_LUXEMBOURG)	/*0x140C*/
#include "nls/frl.nls"
LANG_END

LANG_BEGIN (LANG_HUNGARIAN, SUBLANG_DEFAULT)	/*0x040e*/
#include "nls/hun.nls"
LANG_END

LANG_BEGIN (LANG_ITALIAN, SUBLANG_ITALIAN)		/*0x0410*/
#include "nls/ita.nls"
LANG_END
LANG_BEGIN (LANG_ITALIAN, SUBLANG_ITALIAN_SWISS)	/*0x0810*/
#include "nls/its.nls"
LANG_END

LANG_BEGIN (LANG_JAPANESE, SUBLANG_DEFAULT)	/*0x0411*/
#include "nls/jpn.nls"
LANG_END

LANG_BEGIN (LANG_KOREAN, SUBLANG_KOREAN)	/*0x0412*/
#include "nls/kor.nls"
LANG_END

LANG_BEGIN (LANG_DUTCH, SUBLANG_DUTCH)		/*0x0413*/
#include "nls/nld.nls"
LANG_END
LANG_BEGIN (LANG_DUTCH, SUBLANG_DUTCH_BELGIAN)	/*0x0813*/
#include "nls/nlb.nls"
LANG_END
LANG_BEGIN (LANG_DUTCH, SUBLANG_DUTCH_SURINAM)	/*0x0C13*/
#include "nls/nls.nls"
LANG_END

LANG_BEGIN (LANG_NORWEGIAN, SUBLANG_NORWEGIAN_BOKMAL)	/*0x0414*/
#include "nls/nor.nls"
LANG_END
LANG_BEGIN (LANG_NORWEGIAN, SUBLANG_NORWEGIAN_NYNORSK)	/*0x0814*/
#include "nls/non.nls"
LANG_END

LANG_BEGIN (LANG_POLISH, SUBLANG_DEFAULT)	/*0x0415*/
#include "nls/plk.nls"
LANG_END

LANG_BEGIN (LANG_PORTUGUESE ,SUBLANG_PORTUGUESE_BRAZILIAN)	/*0x0416*/
#include "nls/ptb.nls"
LANG_END
LANG_BEGIN (LANG_PORTUGUESE ,SUBLANG_PORTUGUESE)		/*0x0816*/
#include "nls/ptg.nls"
LANG_END

LANG_BEGIN (LANG_RUSSIAN, SUBLANG_DEFAULT)	/*0x419*/
#include "nls/rus.nls"
LANG_END

LANG_BEGIN (LANG_SLOVAK, SUBLANG_DEFAULT)	/*0x041b*/
#include "nls/sky.nls"
LANG_END

LANG_BEGIN (LANG_SWEDISH, SUBLANG_SWEDISH)		/*0x041d*/
#include "nls/sve.nls"
LANG_END
LANG_BEGIN (LANG_SWEDISH, SUBLANG_SWEDISH_FINLAND)	/*0x081d*/
#include "nls/svf.nls"
LANG_END

LANG_BEGIN (LANG_THAI, SUBLANG_DEFAULT)	/*0x41e*/
#include "nls/tha.nls"
LANG_END
LANG_BEGIN (LANG_GAELIC, SUBLANG_DEFAULT) /* 0x043c */
#include "nls/gae.nls"
LANG_END

LANG_BEGIN (LANG_GAELIC, SUBLANG_GAELIC_SCOTTISH)
#include "nls/gdh.nls"
LANG_END

LANG_BEGIN (LANG_GAELIC, SUBLANG_GAELIC_MANX)
#include "nls/gdv.nls"
LANG_END

LANG_BEGIN (LANG_ESPERANTO, SUBLANG_DEFAULT)	/*0x048f*/
#include "nls/esperanto.nls"
LANG_END

LANG_BEGIN (LANG_WALON, SUBLANG_DEFAULT)	/*0x0490*/
#include "nls/wal.nls"
LANG_END

LANG_BEGIN (LANG_CORNISH, SUBLANG_DEFAULT) /* 0x0491  */
#include "nls/cor.nls"
LANG_END

LANG_BEGIN (LANG_WELSH, SUBLANG_DEFAULT) /* 0x0492 */
#include "nls/cym.nls"
LANG_END

LANG_BEGIN (LANG_BRETON, SUBLANG_DEFAULT) /* 0x0x93  */
#include "nls/brf.nls"
LANG_END

	    };


/* Locale name to id map. used by EnumSystemLocales, GetLocaleInfoA 
 * MUST contain all #defines from winnls.h
 * last entry has NULL name, 0 id.
 */ 
#define LOCALE_ENTRY(x)	{#x,LOCALE_##x}
static const struct tagLOCALE_NAME2ID {
    const char	*name;
	DWORD	id;
} locale_name2id[]= {
	LOCALE_ENTRY(ILANGUAGE),
	LOCALE_ENTRY(SLANGUAGE),
	LOCALE_ENTRY(SENGLANGUAGE),
	LOCALE_ENTRY(SABBREVLANGNAME),
	LOCALE_ENTRY(SNATIVELANGNAME),
	LOCALE_ENTRY(ICOUNTRY),
	LOCALE_ENTRY(SCOUNTRY),
	LOCALE_ENTRY(SENGCOUNTRY),
	LOCALE_ENTRY(SABBREVCTRYNAME),
	LOCALE_ENTRY(SNATIVECTRYNAME),
	LOCALE_ENTRY(IDEFAULTLANGUAGE),
	LOCALE_ENTRY(IDEFAULTCOUNTRY),
	LOCALE_ENTRY(IDEFAULTCODEPAGE),
	LOCALE_ENTRY(IDEFAULTANSICODEPAGE),
	LOCALE_ENTRY(IDEFAULTMACCODEPAGE),
	LOCALE_ENTRY(SLIST),
	LOCALE_ENTRY(IMEASURE),
	LOCALE_ENTRY(SDECIMAL),
	LOCALE_ENTRY(STHOUSAND),
	LOCALE_ENTRY(SGROUPING),
	LOCALE_ENTRY(IDIGITS),
	LOCALE_ENTRY(ILZERO),
	LOCALE_ENTRY(INEGNUMBER),
	LOCALE_ENTRY(SNATIVEDIGITS),
	LOCALE_ENTRY(SCURRENCY),
	LOCALE_ENTRY(SINTLSYMBOL),
	LOCALE_ENTRY(SMONDECIMALSEP),
	LOCALE_ENTRY(SMONTHOUSANDSEP),
	LOCALE_ENTRY(SMONGROUPING),
	LOCALE_ENTRY(ICURRDIGITS),
	LOCALE_ENTRY(IINTLCURRDIGITS),
	LOCALE_ENTRY(ICURRENCY),
	LOCALE_ENTRY(INEGCURR),
	LOCALE_ENTRY(SDATE),
	LOCALE_ENTRY(STIME),
	LOCALE_ENTRY(SSHORTDATE),
	LOCALE_ENTRY(SLONGDATE),
	LOCALE_ENTRY(STIMEFORMAT),
	LOCALE_ENTRY(IDATE),
	LOCALE_ENTRY(ILDATE),
	LOCALE_ENTRY(ITIME),
	LOCALE_ENTRY(ITIMEMARKPOSN),
	LOCALE_ENTRY(ICENTURY),
	LOCALE_ENTRY(ITLZERO),
	LOCALE_ENTRY(IDAYLZERO),
	LOCALE_ENTRY(IMONLZERO),
	LOCALE_ENTRY(S1159),
	LOCALE_ENTRY(S2359),
	LOCALE_ENTRY(ICALENDARTYPE),
	LOCALE_ENTRY(IOPTIONALCALENDAR),
	LOCALE_ENTRY(IFIRSTDAYOFWEEK),
	LOCALE_ENTRY(IFIRSTWEEKOFYEAR),
	LOCALE_ENTRY(SDAYNAME1),
	LOCALE_ENTRY(SDAYNAME2),
	LOCALE_ENTRY(SDAYNAME3),
	LOCALE_ENTRY(SDAYNAME4),
	LOCALE_ENTRY(SDAYNAME5),
	LOCALE_ENTRY(SDAYNAME6),
	LOCALE_ENTRY(SDAYNAME7),
	LOCALE_ENTRY(SABBREVDAYNAME1),
	LOCALE_ENTRY(SABBREVDAYNAME2),
	LOCALE_ENTRY(SABBREVDAYNAME3),
	LOCALE_ENTRY(SABBREVDAYNAME4),
	LOCALE_ENTRY(SABBREVDAYNAME5),
	LOCALE_ENTRY(SABBREVDAYNAME6),
	LOCALE_ENTRY(SABBREVDAYNAME7),
	LOCALE_ENTRY(SMONTHNAME1),
	LOCALE_ENTRY(SMONTHNAME2),
	LOCALE_ENTRY(SMONTHNAME3),
	LOCALE_ENTRY(SMONTHNAME4),
	LOCALE_ENTRY(SMONTHNAME5),
	LOCALE_ENTRY(SMONTHNAME6),
	LOCALE_ENTRY(SMONTHNAME7),
	LOCALE_ENTRY(SMONTHNAME8),
	LOCALE_ENTRY(SMONTHNAME9),
	LOCALE_ENTRY(SMONTHNAME10),
	LOCALE_ENTRY(SMONTHNAME11),
	LOCALE_ENTRY(SMONTHNAME12),
	LOCALE_ENTRY(SMONTHNAME13),
	LOCALE_ENTRY(SABBREVMONTHNAME1),
	LOCALE_ENTRY(SABBREVMONTHNAME2),
	LOCALE_ENTRY(SABBREVMONTHNAME3),
	LOCALE_ENTRY(SABBREVMONTHNAME4),
	LOCALE_ENTRY(SABBREVMONTHNAME5),
	LOCALE_ENTRY(SABBREVMONTHNAME6),
	LOCALE_ENTRY(SABBREVMONTHNAME7),
	LOCALE_ENTRY(SABBREVMONTHNAME8),
	LOCALE_ENTRY(SABBREVMONTHNAME9),
	LOCALE_ENTRY(SABBREVMONTHNAME10),
	LOCALE_ENTRY(SABBREVMONTHNAME11),
	LOCALE_ENTRY(SABBREVMONTHNAME12),
	LOCALE_ENTRY(SABBREVMONTHNAME13),
	LOCALE_ENTRY(SPOSITIVESIGN),
	LOCALE_ENTRY(SNEGATIVESIGN),
	LOCALE_ENTRY(IPOSSIGNPOSN),
	LOCALE_ENTRY(INEGSIGNPOSN),
	LOCALE_ENTRY(IPOSSYMPRECEDES),
	LOCALE_ENTRY(IPOSSEPBYSPACE),
	LOCALE_ENTRY(INEGSYMPRECEDES),
	LOCALE_ENTRY(INEGSEPBYSPACE),
	LOCALE_ENTRY(FONTSIGNATURE),
	LOCALE_ENTRY(SISO639LANGNAME),
	LOCALE_ENTRY(SISO3166CTRYNAME),
	{NULL,0},
};

const struct map_lcid2str {
	LCID		langid;
	const char	*langname;
} languages[]={
	{0x0401,"Arabic (Saudi Arabia)"},
	{0x0801,"Arabic (Iraq)"},
	{0x0c01,"Arabic (Egypt)"},
	{0x1001,"Arabic (Libya)"},
	{0x1401,"Arabic (Algeria)"},
	{0x1801,"Arabic (Morocco)"},
	{0x1c01,"Arabic (Tunisia)"},
	{0x2001,"Arabic (Oman)"},
	{0x2401,"Arabic (Yemen)"},
	{0x2801,"Arabic (Syria)"},
	{0x2c01,"Arabic (Jordan)"},
	{0x3001,"Arabic (Lebanon)"},
	{0x3401,"Arabic (Kuwait)"},
	{0x3801,"Arabic (United Arab Emirates)"},
	{0x3c01,"Arabic (Bahrain)"},
	{0x4001,"Arabic (Qatar)"},
	{0x0402,"Bulgarian"},
	{0x0403,"Catalan"},
	{0x0404,"Chinese (Taiwan)"},
	{0x0804,"Chinese (People's Republic of China)"},
	{0x0c04,"Chinese (Hong Kong)"},
	{0x1004,"Chinese (Singapore)"},
	{0x1404,"Chinese (Macau)"},
	{0x0405,"Czech"},
	{0x0406,"Danish"},
	{0x0407,"German (Germany)"},
	{0x0807,"German (Switzerland)"},
	{0x0c07,"German (Austria)"},
	{0x1007,"German (Luxembourg)"},
	{0x1407,"German (Liechtenstein)"},
	{0x0408,"Greek"},
	{0x0409,"English (United States)"},
	{0x0809,"English (United Kingdom)"},
	{0x0c09,"English (Australian)"},
	{0x1009,"English (Canadian)"},
	{0x1409,"English (New Zealand)"},
	{0x1809,"English (Ireland)"},
	{0x1c09,"English (South Africa)"},
	{0x2009,"English (Jamaica)"},
	{0x2409,"English (Caribbean)"},
	{0x2809,"English (Belize)"},
	{0x2c09,"English (Trinidad & Tobago)"},
	{0x3009,"English (Zimbabwe)"},
	{0x3409,"English (Philippines)"},
	{0x040a,"Spanish (Spain, traditional sorting)"},
	{0x080a,"Spanish (Mexico)"},
	{0x0c0a,"Spanish (Spain, international sorting)"},
	{0x100a,"Spanish (Guatemala)"},
	{0x140a,"Spanish (Costa Rica)"},
	{0x180a,"Spanish (Panama)"},
	{0x1c0a,"Spanish (Dominican Republic)"},
	{0x200a,"Spanish (Venezuela)"},
	{0x240a,"Spanish (Colombia)"},
	{0x280a,"Spanish (Peru)"},
	{0x2c0a,"Spanish (Argentina)"},
	{0x300a,"Spanish (Ecuador)"},
	{0x340a,"Spanish (Chile)"},
	{0x380a,"Spanish (Uruguay)"},
	{0x3c0a,"Spanish (Paraguay)"},
	{0x400a,"Spanish (Bolivia)"},
	{0x440a,"Spanish (El Salvador)"},
	{0x480a,"Spanish (Honduras)"},
	{0x4c0a,"Spanish (Nicaragua)"},
	{0x500a,"Spanish (Puerto Rico)"},
	{0x040b,"Finnish"},
	{0x040c,"French (France)"},
	{0x080c,"French (Belgium)"},
	{0x0c0c,"French (Canada)"},
	{0x100c,"French (Switzerland)"},
	{0x140c,"French (Luxembourg)"},
	{0x180c,"French (Monaco)"},
	{0x040d,"Hebrew"},
	{0x040e,"Hungarian"},
	{0x040f,"Icelandic"},
	{0x0410,"Italian (Italy)"},
	{0x0810,"Italian (Switzerland)"},
	{0x0411,"Japanese"},
	{0x0412,"Korean (Wansung)"},
	{0x0812,"Korean (Johab)"},
	{0x0413,"Dutch (Netherlands)"},
	{0x0813,"Dutch (Belgium)"},
	{0x0414,"Norwegian (Bokmal)"},
	{0x0814,"Norwegian (Nynorsk)"},
	{0x0415,"Polish"},
	{0x0416,"Portuguese (Brazil)"},
	{0x0816,"Portuguese (Portugal)"},
	{0x0417,"Rhaeto Romanic"},
	{0x0418,"Romanian"},
	{0x0818,"Moldavian"},
	{0x0419,"Russian (Russia)"},
	{0x0819,"Russian (Moldavia)"},
	{0x041a,"Croatian"},
	{0x081a,"Serbian (latin)"},
	{0x0c1a,"Serbian (cyrillic)"},
	{0x041b,"Slovak"},
	{0x041c,"Albanian"},
	{0x041d,"Swedish (Sweden)"},
	{0x081d,"Swedish (Finland)"},
	{0x041e,"Thai"},
	{0x041f,"Turkish"},
	{0x0420,"Urdu"},
	{0x0421,"Indonesian"},
	{0x0422,"Ukrainian"},
	{0x0423,"Belarusian"},
	{0x0424,"Slovene"},
	{0x0425,"Estonian"},
	{0x0426,"Latvian"},
	{0x0427,"Lithuanian (modern)"},
	{0x0827,"Lithuanian (classic)"},
	{0x0428,"Maori"},
	{0x0429,"Farsi"},
	{0x042a,"Vietnamese"},
	{0x042b,"Armenian"},
	{0x042c,"Azeri (latin)"},
	{0x082c,"Azeri (cyrillic)"},
	{0x042d,"Basque"},
	{0x042e,"Sorbian"},
	{0x042f,"Macedonian"},
	{0x0430,"Sutu"},
	{0x0431,"Tsonga"},
	{0x0432,"Tswana"},
	{0x0433,"Venda"},
	{0x0434,"Xhosa"},
	{0x0435,"Zulu"},
	{0x0436,"Afrikaans"},
	{0x0437,"Georgian"},
	{0x0438,"Faeroese"},
	{0x0439,"Hindi"},
	{0x043a,"Maltese"},
	{0x043b,"Saami"},
	{0x043c,"Irish gaelic"},
	{0x083c,"Scottish gaelic"},
	{0x0c3c,"Manx Gaelic"},
	{0x043e,"Malay (Malaysia)"},
	{0x083e,"Malay (Brunei Darussalam)"},
	{0x043f,"Kazak"},
	{0x0441,"Swahili"},
	{0x0443,"Uzbek (latin)"},
	{0x0843,"Uzbek (cyrillic)"},
	{0x0444,"Tatar"},
	{0x0445,"Bengali"},
	{0x0446,"Punjabi"},
	{0x0447,"Gujarati"},
	{0x0448,"Oriya"},
	{0x0449,"Tamil"},
	{0x044a,"Telugu"},
	{0x044b,"Kannada"},
	{0x044c,"Malayalam"},
	{0x044d,"Assamese"},
	{0x044e,"Marathi"},
	{0x044f,"Sanskrit"},
	{0x0457,"Konkani"},
	{0x048f,"Esperanto"}, /* Non official */
	{0x0490,"Walon"}, /* Non official */
	{0x0491,"Cornish"},	/* Not official */
	{0x0492,"Welsh"},	/* Not official */
	{0x0493,"Breton"},	/* Not official */
	{0x0000,"Unknown"}
    }, languages_de[]={
	{0x0401,"Arabic"},
	{0x0402,"Bulgarisch"},
	{0x0403,"Katalanisch"},
	{0x0404,"Traditionales Chinesisch"},
	{0x0405,"Tschecisch"},
	{0x0406,"D‰nisch"},
	{0x0407,"Deutsch"},
	{0x0408,"Griechisch"},
	{0x0409,"Amerikanisches Englisch"},
	{0x040A,"Kastilisches Spanisch"},
	{0x040B,"Finnisch"},
	{0x040C,"Franzvsisch"},
	{0x040D,"Hebrdisch"},
	{0x040E,"Ungarisch"},
	{0x040F,"Isldndisch"},
	{0x0410,"Italienisch"},
	{0x0411,"Japanisch"},
	{0x0412,"Koreanisch"},
	{0x0413,"Niederldndisch"},
	{0x0414,"Norwegisch-Bokmal"},
	{0x0415,"Polnisch"},
	{0x0416,"Brasilianisches Portugiesisch"},
	{0x0417,"Rdtoromanisch"},
	{0x0418,"Rumdnisch"},
	{0x0419,"Russisch"},
	{0x041A,"Kroatoserbisch (lateinisch)"},
	{0x041B,"Slowenisch"},
	{0x041C,"Albanisch"},
	{0x041D,"Schwedisch"},
	{0x041E,"Thai"},
	{0x041F,"T¸rkisch"},
	{0x0420,"Urdu"},
	{0x0421,"Bahasa"},
	{0x0804,"Vereinfachtes Chinesisch"},
	{0x0807,"Schweizerdeutsch"},
	{0x0809,"Britisches Englisch"},
	{0x080A,"Mexikanisches Spanisch"},
	{0x080C,"Belgisches Franzvsisch"},
	{0x0810,"Schweizerisches Italienisch"},
	{0x0813,"Belgisches Niederldndisch"},
	{0x0814,"Norgwegisch-Nynorsk"},
	{0x0816,"Portugiesisch"},
	{0x081A,"Serbokratisch (kyrillisch)"},
	{0x0C1C,"Kanadisches Franzvsisch"},
	{0x100C,"Schweizerisches Franzvsisch"},
	{0x0000,"Unbekannt"},
};

/***********************************************************************
 *           GetUserDefaultLCID       (OLE2NLS.1)
 */
LCID WINAPI GetUserDefaultLCID()
{
	return MAKELCID( GetUserDefaultLangID() , SORT_DEFAULT );
}

/***********************************************************************
 *         GetSystemDefaultLCID       (OLE2NLS.2)
 */
LCID WINAPI GetSystemDefaultLCID()
{
	return GetUserDefaultLCID();
}

/***********************************************************************
 *         GetUserDefaultLangID       (OLE2NLS.3)
 */
LANGID WINAPI GetUserDefaultLangID()
{
	/* caching result, if defined from environment, which should (?) not change during a WINE session */
	static	LANGID	userLCID = 0;
	if (Options.language) {
		return Languages[Options.language].langid;
	}

	if (userLCID == 0) {
		char *buf=NULL;
		char *lang,*country,*charset,*dialect,*next;
		int 	ret=0;
		
		buf=getenv("LANGUAGE");
		if (!buf) buf=getenv("LANG");
		if (!buf) buf=getenv("LC_ALL");
		if (!buf) buf=getenv("LC_MESSAGES");
		if (!buf) buf=getenv("LC_CTYPE");
		if (!buf) return userLCID = MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
		
		if (!strcmp(buf,"POSIX") || !strcmp(buf,"C")) {
			return MAKELANGID( LANG_ENGLISH, SUBLANG_DEFAULT );
		}
		
		lang=buf;
		
		do {
			next=strchr(lang,':'); if (next) *next++='\0';
			dialect=strchr(lang,'@'); if (dialect) *dialect++='\0';
			charset=strchr(lang,'.'); if (charset) *charset++='\0';
			country=strchr(lang,'_'); if (country) *country++='\0';
			
			ret=MAIN_GetLanguageID(lang, country, charset, dialect);
			
			lang=next;
			
		} while (lang && !ret);
		
		/* FIXME : are strings returned by getenv() to be free()'ed ? */
		userLCID = (LANGID)ret;
	}
	return userLCID;
}

/***********************************************************************
 *         GetSystemDefaultLangID     (OLE2NLS.4)
 */
LANGID WINAPI GetSystemDefaultLangID()
{
	return GetUserDefaultLangID();
}

/******************************************************************************
 *		GetLocaleInfo16	[OLE2NLS.5]
 * Is the last parameter really WORD for Win16?
 */
INT16 WINAPI GetLocaleInfo16(LCID lcid,LCTYPE LCType,LPSTR buf,INT16 len)
{
	return GetLocaleInfoA(lcid,LCType,buf,len);
}
/******************************************************************************
 * ConvertDefaultLocale32 [KERNEL32.147]
 */
LCID WINAPI ConvertDefaultLocale32 (LCID lcid)
{	switch (lcid)
	{  case LOCALE_SYSTEM_DEFAULT:
	     return GetSystemDefaultLCID();
	   case LOCALE_USER_DEFAULT:
	     return GetUserDefaultLCID();
	   case 0:
	     return MAKELCID (LANG_NEUTRAL, SUBLANG_NEUTRAL);
	}  
	return MAKELANGID( PRIMARYLANGID(lcid), SUBLANG_NEUTRAL);
}
/******************************************************************************
 * GetLocaleInfoA [KERNEL32.342]
 *
 * NOTES 
 *  LANG_NEUTRAL is equal to LOCALE_SYSTEM_DEFAULT
 *
 *  MS online documentation states that the string returned is NULL terminated
 *  except for LOCALE_FONTSIGNATURE  which "will return a non-NULL
 *  terminated string".
 */
INT WINAPI GetLocaleInfoA(LCID lcid,LCTYPE LCType,LPSTR buf,INT len)
{
  LPCSTR  retString;
	int	found,i;
	int	lang=0;

  TRACE("(lcid=0x%lx,lctype=0x%lx,%p,%x)\n",lcid,LCType,buf,len);

  if (len && (! buf) ) {
    SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return 0;
	}

	if (lcid ==0 || lcid == LANG_SYSTEM_DEFAULT || (LCType & LOCALE_NOUSEROVERRIDE) )	/* 0x00, 0x400 */
	{
            lcid = GetSystemDefaultLCID();
	} 
	else if (lcid == LANG_USER_DEFAULT) /*0x800*/
	{
            lcid = GetUserDefaultLCID();
	}
	LCType &= ~(LOCALE_NOUSEROVERRIDE|LOCALE_USE_CP_ACP);

	/* As an option, we could obtain the value from win.ini.
	   This would not match the Wine compile-time option.
	   Also, not all identifiers are available from win.ini */
	retString=0;
	/* If we are through all of this, retLen should not be zero anymore.
	   If it is, the value is not supported */
	i=0;
	while (locale_name2id[i].name!=NULL) {
		if (LCType == locale_name2id[i].id) {
			retString = locale_name2id[i].name;
			break;
		}
		i++;
	}
	if (!retString) {
      FIXME("Unkown LC type %lX\n",LCType);
		return 0;
	}

    found=0;lang=lcid;
    for (i=0;(i<3 && !found);i++) {
      int j;

      for (j=0;j<sizeof(langlocales)/sizeof(langlocales[0]);j++) {
	if (langlocales[j].lang == lang) {
	  int k;

	  for (k=0;k<sizeof(langlocales[j].locvars)/sizeof(langlocales[j].locvars[0]) && (langlocales[j].locvars[k].type);k++) {
	    if (langlocales[j].locvars[k].type == LCType) {
	      found = 1;
	      retString = langlocales[j].locvars[k].val;
	  break;
	    }
	  }
          if (found)
	    break;
	}
      }
	  /* language not found, try without a sublanguage*/
	  if (i==1) lang=MAKELANGID( PRIMARYLANGID(lang), SUBLANG_DEFAULT);
	  /* mask the LC Value */
	  if (i==2) LCType &= 0xfff;
    }

    if(!found) {
      ERR("'%s' not supported for your language (%04X).\n",
			retString,(WORD)lcid);
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;			
	}
    /* a FONTSIGNATURE is not a string, just 6 DWORDs  */
    if (LCType == LOCALE_FONTSIGNATURE) {
        if (len)
            memcpy(buf, retString, (len<=sizeof(FONTSIGNATURE))?len:sizeof(FONTSIGNATURE));
        return sizeof(FONTSIGNATURE);
    }
    /* if len=0 return only the length, don't touch the buffer*/
    if (len) lstrcpynA(buf,retString,len);
    return strlen(retString)+1;
}

/******************************************************************************
 *		GetLocaleInfoW	[KERNEL32.343]
 *
 * NOTES
 *  MS documentation states that len "specifies the size, in bytes (ANSI version)
 *  or characters (Unicode version), of" wbuf. Thus the number returned is
 *  the same between GetLocaleInfoW and GetLocaleInfoA.
 */
INT WINAPI GetLocaleInfoW(LCID lcid,LCTYPE LCType,LPWSTR wbuf,INT len)
{	WORD wlen;
	LPSTR abuf;
	
	if (len && (! wbuf) )
	{ SetLastError(ERROR_INSUFFICIENT_BUFFER);
	  return 0;
	}

	abuf = (LPSTR)HeapAlloc(GetProcessHeap(),0,len);
	wlen = GetLocaleInfoA(lcid, LCType, abuf, len);

	if (wlen && len)	/* if len=0 return only the length*/
	  lstrcpynAtoW(wbuf,abuf,len);

	HeapFree(GetProcessHeap(),0,abuf);
	return wlen;
}

/******************************************************************************
 *		SetLocaleInfoA	[KERNEL32.656]
 */
BOOL16 WINAPI SetLocaleInfoA(DWORD lcid, DWORD lctype, LPCSTR data)
{
    FIXME("(%ld,%ld,%s): stub\n",lcid,lctype,data);
    return TRUE;
}

/******************************************************************************
 *		IsValidLocale	[KERNEL32.489]
 */
BOOL WINAPI IsValidLocale(LCID lcid,DWORD flags)
{
	/* we support ANY language. Well, at least say that...*/
	return TRUE;
}

/******************************************************************************
 *		EnumSystemLocalesW	[KERNEL32.209]
 */
BOOL WINAPI EnumSystemLocalesW( LOCALE_ENUMPROCW lpfnLocaleEnum,
                                    DWORD flags )
{
	int	i;
	BOOL	ret;
	WCHAR	buffer[200];
	HKEY	xhkey;

	TRACE_(win32)("(%p,%08lx)\n",lpfnLocaleEnum,flags );
	/* see if we can reuse the Win95 registry entries.... */
	if (ERROR_SUCCESS==RegOpenKeyA(HKEY_LOCAL_MACHINE,"System\\CurrentControlSet\\control\\Nls\\Locale\\",&xhkey)) {
		i=0;
		while (1) {
			if (ERROR_SUCCESS!=RegEnumKeyW(xhkey,i,buffer,sizeof(buffer)))
				break;
            		if (!lpfnLocaleEnum(buffer))
				break;
			i++;
		}
		RegCloseKey(xhkey);
		return TRUE;
	}

	i=0;
	while (languages[i].langid!=0)
        {
            LPWSTR cp;
	    char   xbuffer[10];
  	
	    sprintf(xbuffer,"%08lx",(DWORD)languages[i].langid);

	    cp = HEAP_strdupAtoW( GetProcessHeap(), 0, xbuffer );
            ret = lpfnLocaleEnum(cp);
            HeapFree( GetProcessHeap(), 0, cp );
            if (!ret) break;
            i++;
	}
	return TRUE;
}

/******************************************************************************
 *		EnumSystemLocalesA	[KERNEL32.208]
 */
BOOL WINAPI EnumSystemLocalesA(LOCALE_ENUMPROCA lpfnLocaleEnum,
                                   DWORD flags)
{
	int	i;
	CHAR	buffer[200];
	HKEY	xhkey;

	TRACE_(win32)("(%p,%08lx)\n",
		lpfnLocaleEnum,flags
	);

	if ( ERROR_SUCCESS==RegOpenKeyA(HKEY_LOCAL_MACHINE,
                    "System\\CurrentControlSet\\Control\\Nls\\Locale\\",
                    &xhkey)) {
            i=0;
            while (1) {
                DWORD size=sizeof(buffer);
                if (ERROR_SUCCESS!=RegEnumValueA(xhkey,i,buffer,&size,NULL,
                            NULL, NULL,NULL))
                    break;
                if (size && !lpfnLocaleEnum(buffer))
                    break;
                i++;
            }
            RegCloseKey(xhkey);
            return TRUE;
	}
	i=0;
	while (languages[i].langid!=0) {
		sprintf(buffer,"%08lx",(DWORD)languages[i].langid);
		if (!lpfnLocaleEnum(buffer))
			break;
		i++;
	}
	return TRUE;
}

static const unsigned char CT_CType2_LUT[] = {
  C2_NOTAPPLICABLE, /*   -   0 */
  C2_NOTAPPLICABLE, /*   -   1 */
  C2_NOTAPPLICABLE, /*   -   2 */
  C2_NOTAPPLICABLE, /*   -   3 */
  C2_NOTAPPLICABLE, /*   -   4 */
  C2_NOTAPPLICABLE, /*   -   5 */
  C2_NOTAPPLICABLE, /*   -   6 */
  C2_NOTAPPLICABLE, /*   -   7 */
  C2_NOTAPPLICABLE, /*   -   8 */
  C2_SEGMENTSEPARATOR, /*   -   9 */
  C2_NOTAPPLICABLE, /*   -  10 */
  C2_NOTAPPLICABLE, /*   -  11 */
  C2_NOTAPPLICABLE, /*   -  12 */
  C2_NOTAPPLICABLE, /*   -  13 */
  C2_NOTAPPLICABLE, /*   -  14 */
  C2_NOTAPPLICABLE, /*   -  15 */
  C2_NOTAPPLICABLE, /*   -  16 */
  C2_NOTAPPLICABLE, /*   -  17 */
  C2_NOTAPPLICABLE, /*   -  18 */
  C2_NOTAPPLICABLE, /*   -  19 */
  C2_NOTAPPLICABLE, /*   -  20 */
  C2_NOTAPPLICABLE, /*   -  21 */
  C2_NOTAPPLICABLE, /*   -  22 */
  C2_NOTAPPLICABLE, /*   -  23 */
  C2_NOTAPPLICABLE, /*   -  24 */
  C2_NOTAPPLICABLE, /*   -  25 */
  C2_NOTAPPLICABLE, /*   -  26 */
  C2_NOTAPPLICABLE, /*   -  27 */
  C2_NOTAPPLICABLE, /*   -  28 */
  C2_NOTAPPLICABLE, /*   -  29 */
  C2_NOTAPPLICABLE, /*   -  30 */
  C2_NOTAPPLICABLE, /*   -  31 */
  C2_WHITESPACE, /*   -  32 */
  C2_OTHERNEUTRAL, /* ! -  33 */
  C2_OTHERNEUTRAL, /* " -  34 */ /* " */
  C2_EUROPETERMINATOR, /* # -  35 */
  C2_EUROPETERMINATOR, /* $ -  36 */
  C2_EUROPETERMINATOR, /* % -  37 */
  C2_LEFTTORIGHT, /* & -  38 */
  C2_OTHERNEUTRAL, /* ' -  39 */
  C2_OTHERNEUTRAL, /* ( -  40 */
  C2_OTHERNEUTRAL, /* ) -  41 */
  C2_OTHERNEUTRAL, /* * -  42 */
  C2_EUROPETERMINATOR, /* + -  43 */
  C2_COMMONSEPARATOR, /* , -  44 */
  C2_EUROPETERMINATOR, /* - -  45 */
  C2_EUROPESEPARATOR, /* . -  46 */
  C2_EUROPESEPARATOR, /* / -  47 */
  C2_EUROPENUMBER, /* 0 -  48 */
  C2_EUROPENUMBER, /* 1 -  49 */
  C2_EUROPENUMBER, /* 2 -  50 */
  C2_EUROPENUMBER, /* 3 -  51 */
  C2_EUROPENUMBER, /* 4 -  52 */
  C2_EUROPENUMBER, /* 5 -  53 */
  C2_EUROPENUMBER, /* 6 -  54 */
  C2_EUROPENUMBER, /* 7 -  55 */
  C2_EUROPENUMBER, /* 8 -  56 */
  C2_EUROPENUMBER, /* 9 -  57 */
  C2_COMMONSEPARATOR, /* : -  58 */
  C2_OTHERNEUTRAL, /* ; -  59 */
  C2_OTHERNEUTRAL, /* < -  60 */
  C2_OTHERNEUTRAL, /* = -  61 */
  C2_OTHERNEUTRAL, /* > -  62 */
  C2_OTHERNEUTRAL, /* ? -  63 */
  C2_LEFTTORIGHT, /* @ -  64 */
  C2_LEFTTORIGHT, /* A -  65 */
  C2_LEFTTORIGHT, /* B -  66 */
  C2_LEFTTORIGHT, /* C -  67 */
  C2_LEFTTORIGHT, /* D -  68 */
  C2_LEFTTORIGHT, /* E -  69 */
  C2_LEFTTORIGHT, /* F -  70 */
  C2_LEFTTORIGHT, /* G -  71 */
  C2_LEFTTORIGHT, /* H -  72 */
  C2_LEFTTORIGHT, /* I -  73 */
  C2_LEFTTORIGHT, /* J -  74 */
  C2_LEFTTORIGHT, /* K -  75 */
  C2_LEFTTORIGHT, /* L -  76 */
  C2_LEFTTORIGHT, /* M -  77 */
  C2_LEFTTORIGHT, /* N -  78 */
  C2_LEFTTORIGHT, /* O -  79 */
  C2_LEFTTORIGHT, /* P -  80 */
  C2_LEFTTORIGHT, /* Q -  81 */
  C2_LEFTTORIGHT, /* R -  82 */
  C2_LEFTTORIGHT, /* S -  83 */
  C2_LEFTTORIGHT, /* T -  84 */
  C2_LEFTTORIGHT, /* U -  85 */
  C2_LEFTTORIGHT, /* V -  86 */
  C2_LEFTTORIGHT, /* W -  87 */
  C2_LEFTTORIGHT, /* X -  88 */
  C2_LEFTTORIGHT, /* Y -  89 */
  C2_LEFTTORIGHT, /* Z -  90 */
  C2_OTHERNEUTRAL, /* [ -  91 */
  C2_OTHERNEUTRAL, /* \ -  92 */
  C2_OTHERNEUTRAL, /* ] -  93 */
  C2_OTHERNEUTRAL, /* ^ -  94 */
  C2_OTHERNEUTRAL, /* _ -  95 */
  C2_OTHERNEUTRAL, /* ` -  96 */
  C2_LEFTTORIGHT, /* a -  97 */
  C2_LEFTTORIGHT, /* b -  98 */
  C2_LEFTTORIGHT, /* c -  99 */
  C2_LEFTTORIGHT, /* d - 100 */
  C2_LEFTTORIGHT, /* e - 101 */
  C2_LEFTTORIGHT, /* f - 102 */
  C2_LEFTTORIGHT, /* g - 103 */
  C2_LEFTTORIGHT, /* h - 104 */
  C2_LEFTTORIGHT, /* i - 105 */
  C2_LEFTTORIGHT, /* j - 106 */
  C2_LEFTTORIGHT, /* k - 107 */
  C2_LEFTTORIGHT, /* l - 108 */
  C2_LEFTTORIGHT, /* m - 109 */
  C2_LEFTTORIGHT, /* n - 110 */
  C2_LEFTTORIGHT, /* o - 111 */
  C2_LEFTTORIGHT, /* p - 112 */
  C2_LEFTTORIGHT, /* q - 113 */
  C2_LEFTTORIGHT, /* r - 114 */
  C2_LEFTTORIGHT, /* s - 115 */
  C2_LEFTTORIGHT, /* t - 116 */
  C2_LEFTTORIGHT, /* u - 117 */
  C2_LEFTTORIGHT, /* v - 118 */
  C2_LEFTTORIGHT, /* w - 119 */
  C2_LEFTTORIGHT, /* x - 120 */
  C2_LEFTTORIGHT, /* y - 121 */
  C2_LEFTTORIGHT, /* z - 122 */
  C2_OTHERNEUTRAL, /* { - 123 */
  C2_OTHERNEUTRAL, /* | - 124 */
  C2_OTHERNEUTRAL, /* } - 125 */
  C2_OTHERNEUTRAL, /* ~ - 126 */
  C2_NOTAPPLICABLE, /*  - 127 */
  C2_NOTAPPLICABLE, /* Ä - 128 */
  C2_NOTAPPLICABLE, /* Å - 129 */
  C2_OTHERNEUTRAL, /* Ç - 130 */
  C2_LEFTTORIGHT, /* É - 131 */
  C2_OTHERNEUTRAL, /* Ñ - 132 */
  C2_OTHERNEUTRAL, /* Ö - 133 */
  C2_OTHERNEUTRAL, /* Ü - 134 */
  C2_OTHERNEUTRAL, /* á - 135 */
  C2_LEFTTORIGHT, /* à - 136 */
  C2_EUROPETERMINATOR, /* â - 137 */
  C2_LEFTTORIGHT, /* ä - 138 */
  C2_OTHERNEUTRAL, /* ã - 139 */
  C2_LEFTTORIGHT, /* å - 140 */
  C2_NOTAPPLICABLE, /* ç - 141 */
  C2_NOTAPPLICABLE, /* é - 142 */
  C2_NOTAPPLICABLE, /* è - 143 */
  C2_NOTAPPLICABLE, /* ê - 144 */
  C2_OTHERNEUTRAL, /* ë - 145 */
  C2_OTHERNEUTRAL, /* í - 146 */
  C2_OTHERNEUTRAL, /* ì - 147 */
  C2_OTHERNEUTRAL, /* î - 148 */
  C2_OTHERNEUTRAL, /* ï - 149 */
  C2_OTHERNEUTRAL, /* ñ - 150 */
  C2_OTHERNEUTRAL, /* ó - 151 */
  C2_LEFTTORIGHT, /* ò - 152 */
  C2_OTHERNEUTRAL, /* ô - 153 */
  C2_LEFTTORIGHT, /* ö - 154 */
  C2_OTHERNEUTRAL, /* õ - 155 */
  C2_LEFTTORIGHT, /* ú - 156 */
  C2_NOTAPPLICABLE, /* ù - 157 */
  C2_NOTAPPLICABLE, /* û - 158 */
  C2_LEFTTORIGHT, /* ü - 159 */
  C2_WHITESPACE, /* † - 160 */
  C2_OTHERNEUTRAL, /* ° - 161 */
  C2_EUROPETERMINATOR, /* ¢ - 162 */
  C2_EUROPETERMINATOR, /* £ - 163 */
  C2_EUROPETERMINATOR, /* § - 164 */
  C2_EUROPETERMINATOR, /* • - 165 */
  C2_OTHERNEUTRAL, /* ¶ - 166 */
  C2_OTHERNEUTRAL, /* ß - 167 */
  C2_OTHERNEUTRAL, /* ® - 168 */
  C2_OTHERNEUTRAL, /* © - 169 */
  C2_OTHERNEUTRAL, /* ™ - 170 */
  C2_OTHERNEUTRAL, /* ´ - 171 */
  C2_OTHERNEUTRAL, /* ¨ - 172 */
  C2_OTHERNEUTRAL, /* ≠ - 173 */
  C2_OTHERNEUTRAL, /* Æ - 174 */
  C2_OTHERNEUTRAL, /* Ø - 175 */
  C2_EUROPETERMINATOR, /* ∞ - 176 */
  C2_EUROPETERMINATOR, /* ± - 177 */
  C2_EUROPENUMBER, /* ≤ - 178 */
  C2_EUROPENUMBER, /* ≥ - 179 */
  C2_OTHERNEUTRAL, /* ¥ - 180 */
  C2_OTHERNEUTRAL, /* µ - 181 */
  C2_OTHERNEUTRAL, /* ∂ - 182 */
  C2_OTHERNEUTRAL, /* ∑ - 183 */
  C2_OTHERNEUTRAL, /* ∏ - 184 */
  C2_EUROPENUMBER, /* π - 185 */
  C2_OTHERNEUTRAL, /* ∫ - 186 */
  C2_OTHERNEUTRAL, /* ª - 187 */
  C2_OTHERNEUTRAL, /* º - 188 */
  C2_OTHERNEUTRAL, /* Ω - 189 */
  C2_OTHERNEUTRAL, /* æ - 190 */
  C2_OTHERNEUTRAL, /* ø - 191 */
  C2_LEFTTORIGHT, /* ¿ - 192 */
  C2_LEFTTORIGHT, /* ¡ - 193 */
  C2_LEFTTORIGHT, /* ¬ - 194 */
  C2_LEFTTORIGHT, /* √ - 195 */
  C2_LEFTTORIGHT, /* ƒ - 196 */
  C2_LEFTTORIGHT, /* ≈ - 197 */
  C2_LEFTTORIGHT, /* ∆ - 198 */
  C2_LEFTTORIGHT, /* « - 199 */
  C2_LEFTTORIGHT, /* » - 200 */
  C2_LEFTTORIGHT, /* … - 201 */
  C2_LEFTTORIGHT, /*   - 202 */
  C2_LEFTTORIGHT, /* À - 203 */
  C2_LEFTTORIGHT, /* Ã - 204 */
  C2_LEFTTORIGHT, /* Õ - 205 */
  C2_LEFTTORIGHT, /* Œ - 206 */
  C2_LEFTTORIGHT, /* œ - 207 */
  C2_LEFTTORIGHT, /* – - 208 */
  C2_LEFTTORIGHT, /* — - 209 */
  C2_LEFTTORIGHT, /* “ - 210 */
  C2_LEFTTORIGHT, /* ” - 211 */
  C2_LEFTTORIGHT, /* ‘ - 212 */
  C2_LEFTTORIGHT, /* ’ - 213 */
  C2_LEFTTORIGHT, /* ÷ - 214 */
  C2_OTHERNEUTRAL, /* ◊ - 215 */
  C2_LEFTTORIGHT, /* ÿ - 216 */
  C2_LEFTTORIGHT, /* Ÿ - 217 */
  C2_LEFTTORIGHT, /* ⁄ - 218 */
  C2_LEFTTORIGHT, /* € - 219 */
  C2_LEFTTORIGHT, /* ‹ - 220 */
  C2_LEFTTORIGHT, /* › - 221 */
  C2_LEFTTORIGHT, /* ﬁ - 222 */
  C2_LEFTTORIGHT, /* ﬂ - 223 */
  C2_LEFTTORIGHT, /* ‡ - 224 */
  C2_LEFTTORIGHT, /* · - 225 */
  C2_LEFTTORIGHT, /* ‚ - 226 */
  C2_LEFTTORIGHT, /* „ - 227 */
  C2_LEFTTORIGHT, /* ‰ - 228 */
  C2_LEFTTORIGHT, /* Â - 229 */
  C2_LEFTTORIGHT, /* Ê - 230 */
  C2_LEFTTORIGHT, /* Á - 231 */
  C2_LEFTTORIGHT, /* Ë - 232 */
  C2_LEFTTORIGHT, /* È - 233 */
  C2_LEFTTORIGHT, /* Í - 234 */
  C2_LEFTTORIGHT, /* Î - 235 */
  C2_LEFTTORIGHT, /* Ï - 236 */
  C2_LEFTTORIGHT, /* Ì - 237 */
  C2_LEFTTORIGHT, /* Ó - 238 */
  C2_LEFTTORIGHT, /* Ô - 239 */
  C2_LEFTTORIGHT, /*  - 240 */
  C2_LEFTTORIGHT, /* Ò - 241 */
  C2_LEFTTORIGHT, /* Ú - 242 */
  C2_LEFTTORIGHT, /* Û - 243 */
  C2_LEFTTORIGHT, /* Ù - 244 */
  C2_LEFTTORIGHT, /* ı - 245 */
  C2_LEFTTORIGHT, /* ˆ - 246 */
  C2_OTHERNEUTRAL, /* ˜ - 247 */
  C2_LEFTTORIGHT, /* ¯ - 248 */
  C2_LEFTTORIGHT, /* ˘ - 249 */
  C2_LEFTTORIGHT, /* ˙ - 250 */
  C2_LEFTTORIGHT, /* ˚ - 251 */
  C2_LEFTTORIGHT, /* ¸ - 252 */
  C2_LEFTTORIGHT, /* ˝ - 253 */
  C2_LEFTTORIGHT, /* ˛ - 254 */
  C2_LEFTTORIGHT /* ˇ - 255 */
};

const WORD OLE2NLS_CT_CType3_LUT[] = { 
  0x0000, /*   -   0 */
  0x0000, /*   -   1 */
  0x0000, /*   -   2 */
  0x0000, /*   -   3 */
  0x0000, /*   -   4 */
  0x0000, /*   -   5 */
  0x0000, /*   -   6 */
  0x0000, /*   -   7 */
  0x0000, /*   -   8 */
  0x0008, /*   -   9 */
  0x0008, /*   -  10 */
  0x0008, /*   -  11 */
  0x0008, /*   -  12 */
  0x0008, /*   -  13 */
  0x0000, /*   -  14 */
  0x0000, /*   -  15 */
  0x0000, /*   -  16 */
  0x0000, /*   -  17 */
  0x0000, /*   -  18 */
  0x0000, /*   -  19 */
  0x0000, /*   -  20 */
  0x0000, /*   -  21 */
  0x0000, /*   -  22 */
  0x0000, /*   -  23 */
  0x0000, /*   -  24 */
  0x0000, /*   -  25 */
  0x0000, /*   -  26 */
  0x0000, /*   -  27 */
  0x0000, /*   -  28 */
  0x0000, /*   -  29 */
  0x0000, /*   -  30 */
  0x0000, /*   -  31 */
  0x0048, /*   -  32 */
  0x0048, /* ! -  33 */
  0x0448, /* " -  34 */ /* " */
  0x0048, /* # -  35 */
  0x0448, /* $ -  36 */
  0x0048, /* % -  37 */
  0x0048, /* & -  38 */
  0x0440, /* ' -  39 */
  0x0048, /* ( -  40 */
  0x0048, /* ) -  41 */
  0x0048, /* * -  42 */
  0x0048, /* + -  43 */
  0x0048, /* , -  44 */
  0x0440, /* - -  45 */
  0x0048, /* . -  46 */
  0x0448, /* / -  47 */
  0x0040, /* 0 -  48 */
  0x0040, /* 1 -  49 */
  0x0040, /* 2 -  50 */
  0x0040, /* 3 -  51 */
  0x0040, /* 4 -  52 */
  0x0040, /* 5 -  53 */
  0x0040, /* 6 -  54 */
  0x0040, /* 7 -  55 */
  0x0040, /* 8 -  56 */
  0x0040, /* 9 -  57 */
  0x0048, /* : -  58 */
  0x0048, /* ; -  59 */
  0x0048, /* < -  60 */
  0x0448, /* = -  61 */
  0x0048, /* > -  62 */
  0x0048, /* ? -  63 */
  0x0448, /* @ -  64 */
  0x8040, /* A -  65 */
  0x8040, /* B -  66 */
  0x8040, /* C -  67 */
  0x8040, /* D -  68 */
  0x8040, /* E -  69 */
  0x8040, /* F -  70 */
  0x8040, /* G -  71 */
  0x8040, /* H -  72 */
  0x8040, /* I -  73 */
  0x8040, /* J -  74 */
  0x8040, /* K -  75 */
  0x8040, /* L -  76 */
  0x8040, /* M -  77 */
  0x8040, /* N -  78 */
  0x8040, /* O -  79 */
  0x8040, /* P -  80 */
  0x8040, /* Q -  81 */
  0x8040, /* R -  82 */
  0x8040, /* S -  83 */
  0x8040, /* T -  84 */
  0x8040, /* U -  85 */
  0x8040, /* V -  86 */
  0x8040, /* W -  87 */
  0x8040, /* X -  88 */
  0x8040, /* Y -  89 */
  0x8040, /* Z -  90 */
  0x0048, /* [ -  91 */
  0x0448, /* \ -  92 */
  0x0048, /* ] -  93 */
  0x0448, /* ^ -  94 */
  0x0448, /* _ -  95 */
  0x0448, /* ` -  96 */
  0x8040, /* a -  97 */
  0x8040, /* b -  98 */
  0x8040, /* c -  99 */
  0x8040, /* d - 100 */
  0x8040, /* e - 101 */
  0x8040, /* f - 102 */
  0x8040, /* g - 103 */
  0x8040, /* h - 104 */
  0x8040, /* i - 105 */
  0x8040, /* j - 106 */
  0x8040, /* k - 107 */
  0x8040, /* l - 108 */
  0x8040, /* m - 109 */
  0x8040, /* n - 110 */
  0x8040, /* o - 111 */
  0x8040, /* p - 112 */
  0x8040, /* q - 113 */
  0x8040, /* r - 114 */
  0x8040, /* s - 115 */
  0x8040, /* t - 116 */
  0x8040, /* u - 117 */
  0x8040, /* v - 118 */
  0x8040, /* w - 119 */
  0x8040, /* x - 120 */
  0x8040, /* y - 121 */
  0x8040, /* z - 122 */
  0x0048, /* { - 123 */
  0x0048, /* | - 124 */
  0x0048, /* } - 125 */
  0x0448, /* ~ - 126 */
  0x0000, /*  - 127 */
  0x0000, /* Ä - 128 */
  0x0000, /* Å - 129 */
  0x0008, /* Ç - 130 */
  0x8000, /* É - 131 */
  0x0008, /* Ñ - 132 */
  0x0008, /* Ö - 133 */
  0x0008, /* Ü - 134 */
  0x0008, /* á - 135 */
  0x0001, /* à - 136 */
  0x0008, /* â - 137 */
  0x8003, /* ä - 138 */
  0x0008, /* ã - 139 */
  0x8000, /* å - 140 */
  0x0000, /* ç - 141 */
  0x0000, /* é - 142 */
  0x0000, /* è - 143 */
  0x0000, /* ê - 144 */
  0x0088, /* ë - 145 */
  0x0088, /* í - 146 */
  0x0088, /* ì - 147 */
  0x0088, /* î - 148 */
  0x0008, /* ï - 149 */
  0x0400, /* ñ - 150 */
  0x0400, /* ó - 151 */
  0x0408, /* ò - 152 */
  0x0000, /* ô - 153 */
  0x8003, /* ö - 154 */
  0x0008, /* õ - 155 */
  0x8000, /* ú - 156 */
  0x0000, /* ù - 157 */
  0x0000, /* û - 158 */
  0x8003, /* ü - 159 */
  0x0008, /* † - 160 */
  0x0008, /* ° - 161 */
  0x0048, /* ¢ - 162 */
  0x0048, /* £ - 163 */
  0x0008, /* § - 164 */
  0x0048, /* • - 165 */
  0x0048, /* ¶ - 166 */
  0x0008, /* ß - 167 */
  0x0408, /* ® - 168 */
  0x0008, /* © - 169 */
  0x0400, /* ™ - 170 */
  0x0008, /* ´ - 171 */
  0x0048, /* ¨ - 172 */
  0x0408, /* ≠ - 173 */
  0x0008, /* Æ - 174 */
  0x0448, /* Ø - 175 */
  0x0008, /* ∞ - 176 */
  0x0008, /* ± - 177 */
  0x0000, /* ≤ - 178 */
  0x0000, /* ≥ - 179 */
  0x0408, /* ¥ - 180 */
  0x0008, /* µ - 181 */
  0x0008, /* ∂ - 182 */
  0x0008, /* ∑ - 183 */
  0x0408, /* ∏ - 184 */
  0x0000, /* π - 185 */
  0x0400, /* ∫ - 186 */
  0x0008, /* ª - 187 */
  0x0000, /* º - 188 */
  0x0000, /* Ω - 189 */
  0x0000, /* æ - 190 */
  0x0008, /* ø - 191 */
  0x8003, /* ¿ - 192 */
  0x8003, /* ¡ - 193 */
  0x8003, /* ¬ - 194 */
  0x8003, /* √ - 195 */
  0x8003, /* ƒ - 196 */
  0x8003, /* ≈ - 197 */
  0x8000, /* ∆ - 198 */
  0x8003, /* « - 199 */
  0x8003, /* » - 200 */
  0x8003, /* … - 201 */
  0x8003, /*   - 202 */
  0x8003, /* À - 203 */
  0x8003, /* Ã - 204 */
  0x8003, /* Õ - 205 */
  0x8003, /* Œ - 206 */
  0x8003, /* œ - 207 */
  0x8000, /* – - 208 */
  0x8003, /* — - 209 */
  0x8003, /* “ - 210 */
  0x8003, /* ” - 211 */
  0x8003, /* ‘ - 212 */
  0x8003, /* ’ - 213 */
  0x8003, /* ÷ - 214 */
  0x0008, /* ◊ - 215 */
  0x8003, /* ÿ - 216 */
  0x8003, /* Ÿ - 217 */
  0x8003, /* ⁄ - 218 */
  0x8003, /* € - 219 */
  0x8003, /* ‹ - 220 */
  0x8003, /* › - 221 */
  0x8000, /* ﬁ - 222 */
  0x8000, /* ﬂ - 223 */
  0x8003, /* ‡ - 224 */
  0x8003, /* · - 225 */
  0x8003, /* ‚ - 226 */
  0x8003, /* „ - 227 */
  0x8003, /* ‰ - 228 */
  0x8003, /* Â - 229 */
  0x8000, /* Ê - 230 */
  0x8003, /* Á - 231 */
  0x8003, /* Ë - 232 */
  0x8003, /* È - 233 */
  0x8003, /* Í - 234 */
  0x8003, /* Î - 235 */
  0x8003, /* Ï - 236 */
  0x8003, /* Ì - 237 */
  0x8003, /* Ó - 238 */
  0x8003, /* Ô - 239 */
  0x8000, /*  - 240 */
  0x8003, /* Ò - 241 */
  0x8003, /* Ú - 242 */
  0x8003, /* Û - 243 */
  0x8003, /* Ù - 244 */
  0x8003, /* ı - 245 */
  0x8003, /* ˆ - 246 */
  0x0008, /* ˜ - 247 */
  0x8003, /* ¯ - 248 */
  0x8003, /* ˘ - 249 */
  0x8003, /* ˙ - 250 */
  0x8003, /* ˚ - 251 */
  0x8003, /* ¸ - 252 */
  0x8003, /* ˝ - 253 */
  0x8000, /* ˛ - 254 */
  0x8003  /* ˇ - 255 */
};

/******************************************************************************
 *		GetStringType16	[OLE2NLS.7]
 */
BOOL16 WINAPI GetStringType16(LCID locale,DWORD dwInfoType,LPCSTR src,
                              INT16 cchSrc,LPWORD chartype)
{
	return GetStringTypeExA(locale,dwInfoType,src,cchSrc,chartype);
}
/******************************************************************************
 *		GetStringTypeA	[KERNEL32.396]
 */
BOOL WINAPI GetStringTypeA(LCID locale,DWORD dwInfoType,LPCSTR src,
                               INT cchSrc,LPWORD chartype)
{
	return GetStringTypeExA(locale,dwInfoType,src,cchSrc,chartype);
}

/******************************************************************************
 *		GetStringTypeExA	[KERNEL32.397]
 *
 * FIXME: Ignores the locale.
 */
BOOL WINAPI GetStringTypeExA(LCID locale,DWORD dwInfoType,LPCSTR src,
                                 INT cchSrc,LPWORD chartype)
{
	int	i;
	
	if ((src==NULL) || (chartype==NULL) || (src==(LPSTR)chartype))
	{
	  SetLastError(ERROR_INVALID_PARAMETER);
	  return FALSE;
	}

	if (cchSrc==-1)
	  cchSrc=lstrlenA(src)+1;
	  
	switch (dwInfoType) {
	case CT_CTYPE1:
	  for (i=0;i<cchSrc;i++) 
	  {
	    chartype[i] = 0;
	    if (isdigit(src[i])) chartype[i]|=C1_DIGIT;
	    if (isalpha(src[i])) chartype[i]|=C1_ALPHA;
	    if (islower(src[i])) chartype[i]|=C1_LOWER;
	    if (isupper(src[i])) chartype[i]|=C1_UPPER;
	    if (isspace(src[i])) chartype[i]|=C1_SPACE;
	    if (ispunct(src[i])) chartype[i]|=C1_PUNCT;
	    if (iscntrl(src[i])) chartype[i]|=C1_CNTRL;
/* FIXME: isblank() is a GNU extension */
/*		if (isblank(src[i])) chartype[i]|=C1_BLANK; */
	    if ((src[i] == ' ') || (src[i] == '\t')) chartype[i]|=C1_BLANK;
	    /* C1_XDIGIT */
	}
	return TRUE;

	case CT_CTYPE2:
	  for (i=0;i<cchSrc;i++) 
	  {
	    chartype[i]=(WORD)CT_CType2_LUT[i];
	  }
	  return TRUE;

	case CT_CTYPE3:
	  for (i=0;i<cchSrc;i++) 
	  {
	    chartype[i]=OLE2NLS_CT_CType3_LUT[i];
	  }
	  return TRUE;

	default:
	  ERR("Unknown dwInfoType:%ld\n",dwInfoType);
	  return FALSE;
	}
}

/******************************************************************************
 *		GetStringTypeW	[KERNEL32.399]
 *
 * NOTES
 * Yes, this is missing LCID locale. MS fault.
 */
BOOL WINAPI GetStringTypeW(DWORD dwInfoType,LPCWSTR src,INT cchSrc,
                               LPWORD chartype)
{
	return GetStringTypeExW(0/*defaultlocale*/,dwInfoType,src,cchSrc,chartype);
}

/******************************************************************************
 *		GetStringTypeExW	[KERNEL32.398]
 *
 * FIXME: unicode chars are assumed chars
 */
BOOL WINAPI GetStringTypeExW(LCID locale,DWORD dwInfoType,LPCWSTR src,
                                 INT cchSrc,LPWORD chartype)
{
	int	i;


	if (cchSrc==-1)
	  cchSrc=lstrlenW(src)+1;
	
	switch (dwInfoType) {
	case CT_CTYPE2:
		FIXME("CT_CTYPE2 not supported.\n");
		return FALSE;
	case CT_CTYPE3:
		FIXME("CT_CTYPE3 not supported.\n");
		return FALSE;
	default:break;
	}
	for (i=0;i<cchSrc;i++) {
		chartype[i] = 0;
		if (isdigit(src[i])) chartype[i]|=C1_DIGIT;
		if (isalpha(src[i])) chartype[i]|=C1_ALPHA;
		if (islower(src[i])) chartype[i]|=C1_LOWER;
		if (isupper(src[i])) chartype[i]|=C1_UPPER;
		if (isspace(src[i])) chartype[i]|=C1_SPACE;
		if (ispunct(src[i])) chartype[i]|=C1_PUNCT;
		if (iscntrl(src[i])) chartype[i]|=C1_CNTRL;
/* FIXME: isblank() is a GNU extension */
/*		if (isblank(src[i])) chartype[i]|=C1_BLANK; */
                if ((src[i] == ' ') || (src[i] == '\t')) chartype[i]|=C1_BLANK;
		/* C1_XDIGIT */
	}
	return TRUE;
}

/*****************************************************************
 * WINE_GetLanguageName   [internal] 
 */
static LPCSTR WINE_GetLanguageName( UINT langid )
{
    int i;
    for ( i = 0; languages[i].langid != 0; i++ )
        if ( langid == languages[i].langid )
            break;

    return languages[i].langname;
}

/***********************************************************************
 *           VerLanguageNameA              [KERNEL32.709][VERSION.9]
 */
DWORD WINAPI VerLanguageNameA( UINT wLang, LPSTR szLang, UINT nSize )
{
    char    buffer[80];
    LPCSTR  name;
    DWORD   result;

    /*
     * First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
     * from the registry.
     */

    sprintf( buffer,
             "\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",
             wLang );

    result = RegQueryValueA( HKEY_LOCAL_MACHINE, buffer, szLang, (LPDWORD)&nSize );
    if ( result == ERROR_SUCCESS || result == ERROR_MORE_DATA )
        return nSize;

    /*
     * If that fails, use the internal table
     */

    name = WINE_GetLanguageName( wLang );
    lstrcpynA( szLang, name, nSize );
    return lstrlenA( name );
}

/***********************************************************************
 *           VerLanguageNameW              [KERNEL32.710][VERSION.10]
 */
DWORD WINAPI VerLanguageNameW( UINT wLang, LPWSTR szLang, UINT nSize )
{
    char    buffer[80];
    LPWSTR  keyname;
    LPCSTR  name;
    DWORD   result;

    /*
     * First, check \System\CurrentControlSet\control\Nls\Locale\<langid>
     * from the registry.
     */

    sprintf( buffer,
             "\\System\\CurrentControlSet\\control\\Nls\\Locale\\%08x",
             wLang );

    keyname = HEAP_strdupAtoW( GetProcessHeap(), 0, buffer );
    result = RegQueryValueW( HKEY_LOCAL_MACHINE, keyname, szLang, (LPDWORD)&nSize );
    HeapFree( GetProcessHeap(), 0, keyname );

    if ( result == ERROR_SUCCESS || result == ERROR_MORE_DATA )
        return nSize;

    /*
     * If that fails, use the internal table
     */

    name = WINE_GetLanguageName( wLang );
    lstrcpynAtoW( szLang, name, nSize );
    return lstrlenA( name );
}

 
static const unsigned char LCM_Unicode_LUT[] = {
  6      ,   3, /*   -   1 */  
  6      ,   4, /*   -   2 */  
  6      ,   5, /*   -   3 */  
  6      ,   6, /*   -   4 */  
  6      ,   7, /*   -   5 */  
  6      ,   8, /*   -   6 */  
  6      ,   9, /*   -   7 */  
  6      ,  10, /*   -   8 */  
  7      ,   5, /*   -   9 */  
  7      ,   6, /*   -  10 */  
  7      ,   7, /*   -  11 */  
  7      ,   8, /*   -  12 */  
  7      ,   9, /*   -  13 */  
  6      ,  11, /*   -  14 */  
  6      ,  12, /*   -  15 */  
  6      ,  13, /*   -  16 */  
  6      ,  14, /*   -  17 */  
  6      ,  15, /*   -  18 */  
  6      ,  16, /*   -  19 */  
  6      ,  17, /*   -  20 */  
  6      ,  18, /*   -  21 */  
  6      ,  19, /*   -  22 */  
  6      ,  20, /*   -  23 */  
  6      ,  21, /*   -  24 */  
  6      ,  22, /*   -  25 */  
  6      ,  23, /*   -  26 */  
  6      ,  24, /*   -  27 */  
  6      ,  25, /*   -  28 */  
  6      ,  26, /*   -  29 */  
  6      ,  27, /*   -  30 */  
  6      ,  28, /*   -  31 */  
  7      ,   2, /*   -  32 */
  7      ,  28, /* ! -  33 */
  7      ,  29, /* " -  34 */ /* " */
  7      ,  31, /* # -  35 */
  7      ,  33, /* $ -  36 */
  7      ,  35, /* % -  37 */
  7      ,  37, /* & -  38 */
  6      , 128, /* ' -  39 */
  7      ,  39, /* ( -  40 */
  7      ,  42, /* ) -  41 */
  7      ,  45, /* * -  42 */
  8      ,   3, /* + -  43 */
  7      ,  47, /* , -  44 */
  6      , 130, /* - -  45 */
  7      ,  51, /* . -  46 */
  7      ,  53, /* / -  47 */
 12      ,   3, /* 0 -  48 */
 12      ,  33, /* 1 -  49 */
 12      ,  51, /* 2 -  50 */
 12      ,  70, /* 3 -  51 */
 12      ,  88, /* 4 -  52 */
 12      , 106, /* 5 -  53 */
 12      , 125, /* 6 -  54 */
 12      , 144, /* 7 -  55 */
 12      , 162, /* 8 -  56 */
 12      , 180, /* 9 -  57 */
  7      ,  55, /* : -  58 */
  7      ,  58, /* ; -  59 */
  8      ,  14, /* < -  60 */
  8      ,  18, /* = -  61 */
  8      ,  20, /* > -  62 */
  7      ,  60, /* ? -  63 */
  7      ,  62, /* @ -  64 */
 14      ,   2, /* A -  65 */
 14      ,   9, /* B -  66 */
 14      ,  10, /* C -  67 */
 14      ,  26, /* D -  68 */
 14      ,  33, /* E -  69 */
 14      ,  35, /* F -  70 */
 14      ,  37, /* G -  71 */
 14      ,  44, /* H -  72 */
 14      ,  50, /* I -  73 */
 14      ,  53, /* J -  74 */
 14      ,  54, /* K -  75 */
 14      ,  72, /* L -  76 */
 14      ,  81, /* M -  77 */
 14      , 112, /* N -  78 */
 14      , 124, /* O -  79 */
 14      , 126, /* P -  80 */
 14      , 137, /* Q -  81 */
 14      , 138, /* R -  82 */
 14      , 145, /* S -  83 */
 14      , 153, /* T -  84 */
 14      , 159, /* U -  85 */
 14      , 162, /* V -  86 */
 14      , 164, /* W -  87 */
 14      , 166, /* X -  88 */
 14      , 167, /* Y -  89 */
 14      , 169, /* Z -  90 */
  7      ,  63, /* [ -  91 */
  7      ,  65, /* \ -  92 */
  7      ,  66, /* ] -  93 */
  7      ,  67, /* ^ -  94 */
  7      ,  68, /* _ -  95 */
  7      ,  72, /* ` -  96 */
 14      ,   2, /* a -  97 */
 14      ,   9, /* b -  98 */
 14      ,  10, /* c -  99 */
 14      ,  26, /* d - 100 */
 14      ,  33, /* e - 101 */
 14      ,  35, /* f - 102 */
 14      ,  37, /* g - 103 */
 14      ,  44, /* h - 104 */
 14      ,  50, /* i - 105 */
 14      ,  53, /* j - 106 */
 14      ,  54, /* k - 107 */
 14      ,  72, /* l - 108 */
 14      ,  81, /* m - 109 */
 14      , 112, /* n - 110 */
 14      , 124, /* o - 111 */
 14      , 126, /* p - 112 */
 14      , 137, /* q - 113 */
 14      , 138, /* r - 114 */
 14      , 145, /* s - 115 */
 14      , 153, /* t - 116 */
 14      , 159, /* u - 117 */
 14      , 162, /* v - 118 */
 14      , 164, /* w - 119 */
 14      , 166, /* x - 120 */
 14      , 167, /* y - 121 */
 14      , 169, /* z - 122 */
  7      ,  74, /* { - 123 */
  7      ,  76, /* | - 124 */
  7      ,  78, /* } - 125 */
  7      ,  80, /* ~ - 126 */
  6      ,  29, /*  - 127 */
  6      ,  30, /* Ä - 128 */
  6      ,  31, /* Å - 129 */
  7      , 123, /* Ç - 130 */
 14      ,  35, /* É - 131 */
  7      , 127, /* Ñ - 132 */
 10      ,  21, /* Ö - 133 */
 10      ,  15, /* Ü - 134 */
 10      ,  16, /* á - 135 */
  7      ,  67, /* à - 136 */
 10      ,  22, /* â - 137 */
 14      , 145, /* ä - 138 */
  7      , 136, /* ã - 139 */
 14 + 16 , 124, /* å - 140 */
  6      ,  43, /* ç - 141 */
  6      ,  44, /* é - 142 */
  6      ,  45, /* è - 143 */
  6      ,  46, /* ê - 144 */
  7      , 121, /* ë - 145 */
  7      , 122, /* í - 146 */
  7      , 125, /* ì - 147 */
  7      , 126, /* î - 148 */
 10      ,  17, /* ï - 149 */
  6      , 137, /* ñ - 150 */
  6      , 139, /* ó - 151 */
  7      ,  93, /* ò - 152 */
 14      , 156, /* ô - 153 */
 14      , 145, /* ö - 154 */
  7      , 137, /* õ - 155 */
 14 + 16 , 124, /* ú - 156 */
  6      ,  59, /* ù - 157 */
  6      ,  60, /* û - 158 */
 14      , 167, /* ü - 159 */
  7      ,   4, /* † - 160 */
  7      ,  81, /* ° - 161 */
 10      ,   2, /* ¢ - 162 */
 10      ,   3, /* £ - 163 */
 10      ,   4, /* § - 164 */
 10      ,   5, /* • - 165 */
  7      ,  82, /* ¶ - 166 */
 10      ,   6, /* ß - 167 */
  7      ,  83, /* ® - 168 */
 10      ,   7, /* © - 169 */
 14      ,   2, /* ™ - 170 */
  8      ,  24, /* ´ - 171 */
 10      ,   8, /* ¨ - 172 */
  6      , 131, /* ≠ - 173 */
 10      ,   9, /* Æ - 174 */
  7      ,  84, /* Ø - 175 */
 10      ,  10, /* ∞ - 176 */
  8      ,  23, /* ± - 177 */
 12      ,  51, /* ≤ - 178 */
 12      ,  70, /* ≥ - 179 */
  7      ,  85, /* ¥ - 180 */
 10      ,  11, /* µ - 181 */
 10      ,  12, /* ∂ - 182 */
 10      ,  13, /* ∑ - 183 */
  7      ,  86, /* ∏ - 184 */
 12      ,  33, /* π - 185 */
 14      , 124, /* ∫ - 186 */
  8      ,  26, /* ª - 187 */
 12      ,  21, /* º - 188 */
 12      ,  25, /* Ω - 189 */
 12      ,  29, /* æ - 190 */
  7      ,  87, /* ø - 191 */
 14      ,   2, /* ¿ - 192 */
 14      ,   2, /* ¡ - 193 */
 14      ,   2, /* ¬ - 194 */
 14      ,   2, /* √ - 195 */
 14      ,   2, /* ƒ - 196 */
 14      ,   2, /* ≈ - 197 */
 14 + 16 ,   2, /* ∆ - 198 */
 14      ,  10, /* « - 199 */
 14      ,  33, /* » - 200 */
 14      ,  33, /* … - 201 */
 14      ,  33, /*   - 202 */
 14      ,  33, /* À - 203 */
 14      ,  50, /* Ã - 204 */
 14      ,  50, /* Õ - 205 */
 14      ,  50, /* Œ - 206 */
 14      ,  50, /* œ - 207 */
 14      ,  26, /* – - 208 */
 14      , 112, /* — - 209 */
 14      , 124, /* “ - 210 */
 14      , 124, /* ” - 211 */
 14      , 124, /* ‘ - 212 */
 14      , 124, /* ’ - 213 */
 14      , 124, /* ÷ - 214 */
  8      ,  28, /* ◊ - 215 */
 14      , 124, /* ÿ - 216 */
 14      , 159, /* Ÿ - 217 */
 14      , 159, /* ⁄ - 218 */
 14      , 159, /* € - 219 */
 14      , 159, /* ‹ - 220 */
 14      , 167, /* › - 221 */
 14 + 32 , 153, /* ﬁ - 222 */
 14 + 48 , 145, /* ﬂ - 223 */
 14      ,   2, /* ‡ - 224 */
 14      ,   2, /* · - 225 */
 14      ,   2, /* ‚ - 226 */
 14      ,   2, /* „ - 227 */
 14      ,   2, /* ‰ - 228 */
 14      ,   2, /* Â - 229 */
 14 + 16 ,   2, /* Ê - 230 */
 14      ,  10, /* Á - 231 */
 14      ,  33, /* Ë - 232 */
 14      ,  33, /* È - 233 */
 14      ,  33, /* Í - 234 */
 14      ,  33, /* Î - 235 */
 14      ,  50, /* Ï - 236 */
 14      ,  50, /* Ì - 237 */
 14      ,  50, /* Ó - 238 */
 14      ,  50, /* Ô - 239 */
 14      ,  26, /*  - 240 */
 14      , 112, /* Ò - 241 */
 14      , 124, /* Ú - 242 */
 14      , 124, /* Û - 243 */
 14      , 124, /* Ù - 244 */
 14      , 124, /* ı - 245 */
 14      , 124, /* ˆ - 246 */
  8      ,  29, /* ˜ - 247 */
 14      , 124, /* ¯ - 248 */
 14      , 159, /* ˘ - 249 */
 14      , 159, /* ˙ - 250 */
 14      , 159, /* ˚ - 251 */
 14      , 159, /* ¸ - 252 */
 14      , 167, /* ˝ - 253 */
 14 + 32 , 153, /* ˛ - 254 */
 14      , 167  /* ˇ - 255 */ };

static const unsigned char LCM_Unicode_LUT_2[] = { 33, 44, 145 };

#define LCM_Diacritic_Start 131

static const unsigned char LCM_Diacritic_LUT[] = { 
123,  /* É - 131 */
  2,  /* Ñ - 132 */
  2,  /* Ö - 133 */
  2,  /* Ü - 134 */
  2,  /* á - 135 */
  3,  /* à - 136 */
  2,  /* â - 137 */
 20,  /* ä - 138 */
  2,  /* ã - 139 */
  2,  /* å - 140 */
  2,  /* ç - 141 */
  2,  /* é - 142 */
  2,  /* è - 143 */
  2,  /* ê - 144 */
  2,  /* ë - 145 */
  2,  /* í - 146 */
  2,  /* ì - 147 */
  2,  /* î - 148 */
  2,  /* ï - 149 */
  2,  /* ñ - 150 */
  2,  /* ó - 151 */
  2,  /* ò - 152 */
  2,  /* ô - 153 */
 20,  /* ö - 154 */
  2,  /* õ - 155 */
  2,  /* ú - 156 */
  2,  /* ù - 157 */
  2,  /* û - 158 */
 19,  /* ü - 159 */
  2,  /* † - 160 */
  2,  /* ° - 161 */
  2,  /* ¢ - 162 */
  2,  /* £ - 163 */
  2,  /* § - 164 */
  2,  /* • - 165 */
  2,  /* ¶ - 166 */
  2,  /* ß - 167 */
  2,  /* ® - 168 */
  2,  /* © - 169 */
  3,  /* ™ - 170 */
  2,  /* ´ - 171 */
  2,  /* ¨ - 172 */
  2,  /* ≠ - 173 */
  2,  /* Æ - 174 */
  2,  /* Ø - 175 */
  2,  /* ∞ - 176 */
  2,  /* ± - 177 */
  2,  /* ≤ - 178 */
  2,  /* ≥ - 179 */
  2,  /* ¥ - 180 */
  2,  /* µ - 181 */
  2,  /* ∂ - 182 */
  2,  /* ∑ - 183 */
  2,  /* ∏ - 184 */
  2,  /* π - 185 */
  3,  /* ∫ - 186 */
  2,  /* ª - 187 */
  2,  /* º - 188 */
  2,  /* Ω - 189 */
  2,  /* æ - 190 */
  2,  /* ø - 191 */
 15,  /* ¿ - 192 */
 14,  /* ¡ - 193 */
 18,  /* ¬ - 194 */
 25,  /* √ - 195 */
 19,  /* ƒ - 196 */
 26,  /* ≈ - 197 */
  2,  /* ∆ - 198 */
 28,  /* « - 199 */
 15,  /* » - 200 */
 14,  /* … - 201 */
 18,  /*   - 202 */
 19,  /* À - 203 */
 15,  /* Ã - 204 */
 14,  /* Õ - 205 */
 18,  /* Œ - 206 */
 19,  /* œ - 207 */
104,  /* – - 208 */
 25,  /* — - 209 */
 15,  /* “ - 210 */
 14,  /* ” - 211 */
 18,  /* ‘ - 212 */
 25,  /* ’ - 213 */
 19,  /* ÷ - 214 */
  2,  /* ◊ - 215 */
 33,  /* ÿ - 216 */
 15,  /* Ÿ - 217 */
 14,  /* ⁄ - 218 */
 18,  /* € - 219 */
 19,  /* ‹ - 220 */
 14,  /* › - 221 */
  2,  /* ﬁ - 222 */
  2,  /* ﬂ - 223 */
 15,  /* ‡ - 224 */
 14,  /* · - 225 */
 18,  /* ‚ - 226 */
 25,  /* „ - 227 */
 19,  /* ‰ - 228 */
 26,  /* Â - 229 */
  2,  /* Ê - 230 */
 28,  /* Á - 231 */
 15,  /* Ë - 232 */
 14,  /* È - 233 */
 18,  /* Í - 234 */
 19,  /* Î - 235 */
 15,  /* Ï - 236 */
 14,  /* Ì - 237 */
 18,  /* Ó - 238 */
 19,  /* Ô - 239 */
104,  /*  - 240 */
 25,  /* Ò - 241 */
 15,  /* Ú - 242 */
 14,  /* Û - 243 */
 18,  /* Ù - 244 */
 25,  /* ı - 245 */
 19,  /* ˆ - 246 */
  2,  /* ˜ - 247 */
 33,  /* ¯ - 248 */
 15,  /* ˘ - 249 */
 14,  /* ˙ - 250 */
 18,  /* ˚ - 251 */
 19,  /* ¸ - 252 */
 14,  /* ˝ - 253 */
  2,  /* ˛ - 254 */
 19,  /* ˇ - 255 */
} ;

/******************************************************************************
 * OLE2NLS_isPunctuation [INTERNAL]
 */
static int OLE2NLS_isPunctuation(unsigned char c) 
{
  /* "punctuation character" in this context is a character which is 
     considered "less important" during word sort comparison.
     See LCMapString implementation for the precise definition 
     of "less important". */

  return (LCM_Unicode_LUT[-2+2*c]==6);
}

/******************************************************************************
 * OLE2NLS_isNonSpacing [INTERNAL]
 */
static int OLE2NLS_isNonSpacing(unsigned char c) 
{
  /* This function is used by LCMapStringA.  Characters 
     for which it returns true are ignored when mapping a
     string with NORM_IGNORENONSPACE */
  return ((c==136) || (c==170) || (c==186));
}

/******************************************************************************
 * OLE2NLS_isSymbol [INTERNAL]
 */
static int OLE2NLS_isSymbol(unsigned char c) 
{
  /* This function is used by LCMapStringA.  Characters 
     for which it returns true are ignored when mapping a
     string with NORM_IGNORESYMBOLS */
  return ( (c!=0) && !IsCharAlphaNumericA(c) );
}

/******************************************************************************
 *		identity	[Internal]
 */
static int identity(int c)
{
  return c;
}

/*************************************************************************
 *              LCMapStringA                [KERNEL32.492]
 *
 * Convert a string, or generate a sort key from it.
 *
 * If (mapflags & LCMAP_SORTKEY), the function will generate
 * a sort key for the source string.  Else, it will convert it
 * accordingly to the flags LCMAP_UPPERCASE, LCMAP_LOWERCASE,...
 *
 * RETURNS
 *    Error : 0.
 *    Success : length of the result string.
 *
 * NOTES
 *    If called with scrlen = -1, the function will compute the length
 *      of the 0-terminated string strsrc by itself.      
 * 
 *    If called with dstlen = 0, returns the buffer length that 
 *      would be required.
 *
 *    NORM_IGNOREWIDTH means to compare ASCII and wide characters
 *    as if they are equal.  
 *    In the only code page implemented so far, there may not be
 *    wide characters in strings passed to LCMapStringA,
 *    so there is nothing to be done for this flag.
 */
INT WINAPI LCMapStringA(
	LCID lcid /* locale identifier created with MAKELCID; 
		     LOCALE_SYSTEM_DEFAULT and LOCALE_USER_DEFAULT are 
                     predefined values. */,
	DWORD mapflags /* flags */,
	LPCSTR srcstr  /* source buffer */,
	INT srclen   /* source length */,
	LPSTR dststr   /* destination buffer */,
	INT dstlen   /* destination buffer length */) 
{
  int i;

  TRACE_(string)("(0x%04lx,0x%08lx,%s,%d,%p,%d)\n",
	lcid,mapflags,srcstr,srclen,dststr,dstlen);

  if ( ((dstlen!=0) && (dststr==NULL)) || (srcstr==NULL) )
  {
    ERR("(src=%s,dest=%s): Invalid NULL string\n", srcstr, dststr);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  if (srclen == -1) 
    srclen = lstrlenA(srcstr) + 1 ;    /* (include final '\0') */

#define LCMAPSTRINGA_SUPPORTED_FLAGS (LCMAP_UPPERCASE     | \
                                        LCMAP_LOWERCASE     | \
                                        LCMAP_SORTKEY       | \
                                        NORM_IGNORECASE     | \
                                        NORM_IGNORENONSPACE | \
                                        SORT_STRINGSORT     | \
                                        NORM_IGNOREWIDTH    | \
                                        NORM_IGNOREKANATYPE)
  /* FIXME: as long as we don't support Kanakana nor Hirigana 
   * characters, we can support NORM_IGNOREKANATYPE
   */
  if (mapflags & ~LCMAPSTRINGA_SUPPORTED_FLAGS)
  {
    FIXME_(string)("(0x%04lx,0x%08lx,%p,%d,%p,%d): "
	  "unimplemented flags: 0x%08lx\n",
	  lcid,
	  mapflags,
	  srcstr,
	  srclen,
	  dststr,
	  dstlen,
	  mapflags & ~LCMAPSTRINGA_SUPPORTED_FLAGS
     );
  }

  if ( !(mapflags & LCMAP_SORTKEY) )
  {
    int i,j;
    int (*f)(int) = identity; 
    int flag_ignorenonspace = mapflags & NORM_IGNORENONSPACE;
    int flag_ignoresymbols = mapflags & NORM_IGNORESYMBOLS;

    if (flag_ignorenonspace || flag_ignoresymbols)
    {
      /* For some values of mapflags, the length of the resulting 
	 string is not known at this point.  Windows does map the string
	 and does not SetLastError ERROR_INSUFFICIENT_BUFFER in
	 these cases. */
      if (dstlen==0)
      {
	/* Compute required length */
	for (i=j=0; i < srclen; i++)
	{
	  if ( !(flag_ignorenonspace && OLE2NLS_isNonSpacing(srcstr[i]))
	       && !(flag_ignoresymbols && OLE2NLS_isSymbol(srcstr[i])) )
	    j++;
	}
	return j;
      }
    }
    else
    {
      if (dstlen==0)
	return srclen;  
      if (dstlen<srclen) 
	   {
	     SetLastError(ERROR_INSUFFICIENT_BUFFER);
	     return 0;
	   }
    }
    if (mapflags & LCMAP_UPPERCASE)
      f = toupper;
    else if (mapflags & LCMAP_LOWERCASE)
      f = tolower;
    /* FIXME: NORM_IGNORENONSPACE requires another conversion */
    for (i=j=0; (i<srclen) && (j<dstlen) ; i++)
    {
      if ( !(flag_ignorenonspace && OLE2NLS_isNonSpacing(srcstr[i]))
	   && !(flag_ignoresymbols && OLE2NLS_isSymbol(srcstr[i])) )
      {
	dststr[j] = (CHAR) f(srcstr[i]);
	j++;
      }
    }
    return j;
  }

  /* FIXME: This function completely ignores the "lcid" parameter. */
  /* else ... (mapflags & LCMAP_SORTKEY)  */
  {
    int unicode_len=0;
    int case_len=0;
    int diacritic_len=0;
    int delayed_punctuation_len=0;
    char *case_component;
    char *diacritic_component;
    char *delayed_punctuation_component;
    int room,count;
    int flag_stringsort = mapflags & SORT_STRINGSORT;

    /* compute how much room we will need */
    for (i=0;i<srclen;i++)
    {
      int ofs;
      unsigned char source_char = srcstr[i];
      if (source_char!='\0') 
      {
	if (flag_stringsort || !OLE2NLS_isPunctuation(source_char))
	{
	  unicode_len++;
	  if ( LCM_Unicode_LUT[-2+2*source_char] & ~15 )
	    unicode_len++;             /* double letter */
	}
	else
	{
	  delayed_punctuation_len++;
	}	  
      }
	  
      if (isupper(source_char))
	case_len=unicode_len; 

      ofs = source_char - LCM_Diacritic_Start;
      if ((ofs>=0) && (LCM_Diacritic_LUT[ofs]!=2))
	diacritic_len=unicode_len;
    }

    if (mapflags & NORM_IGNORECASE)
      case_len=0;                   
    if (mapflags & NORM_IGNORENONSPACE)
      diacritic_len=0;

    room =  2 * unicode_len              /* "unicode" component */
      +     diacritic_len                /* "diacritic" component */
      +     case_len                     /* "case" component */
      +     4 * delayed_punctuation_len  /* punctuation in word sort mode */
      +     4                            /* four '\1' separators */
      +     1  ;                         /* terminal '\0' */
    if (dstlen==0)
      return room;      
    else if (dstlen<room)
    {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }

    /*FIXME the Pointercheck should not be nessesary */
    if (IsBadWritePtr (dststr,room))
    { ERR_(string)("bad destination buffer (dststr) : %p,%d\n",dststr,dstlen);
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      return 0;
    }

    /* locate each component, write separators */
    diacritic_component = dststr + 2*unicode_len ;
    *diacritic_component++ = '\1'; 
    case_component = diacritic_component + diacritic_len ;
    *case_component++ = '\1'; 
    delayed_punctuation_component = case_component + case_len ;
    *delayed_punctuation_component++ = '\1';
    *delayed_punctuation_component++ = '\1';

    /* read source string char by char, write 
       corresponding weight in each component. */
    for (i=0,count=0;i<srclen;i++)
    {
      unsigned char source_char=srcstr[i];
      if (source_char!='\0') 
      {
	int type,longcode;
	type = LCM_Unicode_LUT[-2+2*source_char];
	longcode = type >> 4;
	type &= 15;
	if (!flag_stringsort && OLE2NLS_isPunctuation(source_char)) 
	{
	  UINT16 encrypted_location = (1<<15) + 7 + 4*count;
	  *delayed_punctuation_component++ = (unsigned char) (encrypted_location>>8);
	  *delayed_punctuation_component++ = (unsigned char) (encrypted_location&255);
                     /* big-endian is used here because it lets string comparison be
			compatible with numerical comparison */

	  *delayed_punctuation_component++ = type;
	  *delayed_punctuation_component++ = LCM_Unicode_LUT[-1+2*source_char];  
                     /* assumption : a punctuation character is never a 
			double or accented letter */
	}
	else
	{
	  dststr[2*count] = type;
	  dststr[2*count+1] = LCM_Unicode_LUT[-1+2*source_char];  
	  if (longcode)
	  {
	    if (count<case_len)
	      case_component[count] = ( isupper(source_char) ? 18 : 2 ) ;
	    if (count<diacritic_len)
	      diacritic_component[count] = 2; /* assumption: a double letter
						 is never accented */
	    count++;
	    
	    dststr[2*count] = type;
	    dststr[2*count+1] = *(LCM_Unicode_LUT_2 - 1 + longcode); 
	    /* 16 in the first column of LCM_Unicode_LUT  -->  longcode = 1 
	       32 in the first column of LCM_Unicode_LUT  -->  longcode = 2 
	       48 in the first column of LCM_Unicode_LUT  -->  longcode = 3 */
	  }

	  if (count<case_len)
	    case_component[count] = ( isupper(source_char) ? 18 : 2 ) ;
	  if (count<diacritic_len)
	  {
	    int ofs = source_char - LCM_Diacritic_Start;
	    diacritic_component[count] = (ofs>=0 ? LCM_Diacritic_LUT[ofs] : 2);
	  }
	  count++;
	}
      }
    }
    dststr[room-1] = '\0';
    return room;
  }
}
		     
/*************************************************************************
 *              LCMapStringW                [KERNEL32.493]
 *
 * Convert a string, or generate a sort key from it.
 *
 * NOTE
 *
 * See LCMapStringA for documentation
 */
INT WINAPI LCMapStringW(
	LCID lcid,DWORD mapflags,LPCWSTR srcstr,INT srclen,LPWSTR dststr,
	INT dstlen)
{
  int i;
 
  TRACE_(string)("(0x%04lx,0x%08lx,%p,%d,%p,%d)\n",
                 lcid, mapflags, srcstr, srclen, dststr, dstlen);
  
  if ( ((dstlen!=0) && (dststr==NULL)) || (srcstr==NULL) )
  {
    ERR("(src=%p,dst=%p): Invalid NULL string\n", srcstr, dststr);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }
  if (srclen==-1) 
    srclen = lstrlenW(srcstr)+1;

  /* FIXME: Both this function and it's companion LCMapStringA()
   * completely ignore the "lcid" parameter.  In place of the "lcid"
   * parameter the application must set the "LC_COLLATE" or "LC_ALL"
   * environment variable prior to invoking this function.  */
  if (mapflags & LCMAP_SORTKEY) 
  {
      /* Possible values of LC_COLLATE. */
      char *lc_collate_default = 0; /* value prior to this function */
      char *lc_collate_env = 0;     /* value retrieved from the environment */

      /* General purpose index into strings of any type. */
      int str_idx = 0;

      /* Lengths of various strings where the length is measured in
       * wide characters for wide character strings and in bytes for
       * native strings.  The lengths include the NULL terminator.  */
      size_t returned_len    = 0;
      size_t src_native_len  = 0;
      size_t dst_native_len  = 0;
      size_t dststr_libc_len = 0;

      /* Native (character set determined by locale) versions of the
       * strings source and destination strings.  */
      LPSTR src_native = 0;
      LPSTR dst_native = 0;

      /* Version of the source and destination strings using the
       * "wchar_t" Unicode data type needed by various libc functions.  */
      wchar_t *srcstr_libc = 0;
      wchar_t *dststr_libc = 0;

      if(!(srcstr_libc = (wchar_t *)HeapAlloc(GetProcessHeap(), 0,
                                       srclen * sizeof(wchar_t))))
      {
          ERR("Unable to allocate %d bytes for srcstr_libc\n",
              srclen * sizeof(wchar_t));
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          return 0;
      }

      /* Convert source string to a libc Unicode string. */
      for(str_idx = 0; str_idx < srclen; str_idx++)
      {
          srcstr_libc[str_idx] = srcstr[str_idx];
      }

      /* src_native should contain at most 3 bytes for each
       * multibyte characters in the original srcstr string.  */
      src_native_len = 3 * srclen;
      if(!(src_native = (LPSTR)HeapAlloc(GetProcessHeap(), 0,
                                          src_native_len)))
      {
          ERR("Unable to allocate %d bytes for src_native\n", src_native_len);
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          return 0;
      }

      /* FIXME: Prior to to setting the LC_COLLATE locale category the
       * current value is backed up so it can be restored after the
       * last LC_COLLATE sensitive function returns.
       * 
       * Even though the locale is adjusted for a minimum amount of
       * time a race condition exists where other threads may be
       * affected if they invoke LC_COLLATE sensitive functions.  One
       * possible solution is to wrap all LC_COLLATE sensitive Wine
       * functions, like LCMapStringW(), in a mutex.
       *
       * Another enhancement to the following would be to set the
       * LC_COLLATE locale category as a function of the "lcid"
       * parameter instead of the "LC_COLLATE" environment variable. */
      if(!(lc_collate_default = setlocale(LC_COLLATE, NULL)))
      {
          ERR("Unable to query the LC_COLLATE catagory\n");
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          return 0;
      }

      if(!(lc_collate_env = setlocale(LC_COLLATE, "")))
      {
          ERR("Unable to inherit the LC_COLLATE locale category from the "
              "environment.  The \"LC_COLLATE\" environment variable is "
              "\"%s\".\n", getenv("LC_COLLATE") ?
              getenv("LC_COLLATE") : "<unset>");
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          return 0;
      }

      TRACE_(string)("lc_collate_default = %s\n", lc_collate_default);
      TRACE_(string)("lc_collate_env = %s\n", lc_collate_env);

      /* Convert the libc Unicode string to a native multibyte character
       * string. */
      returned_len = wcstombs(src_native, srcstr_libc, src_native_len) + 1;
      if(returned_len == 0)
      {
          LPSTR srcstr_ascii = (LPSTR)HEAP_strdupWtoA(GetProcessHeap(),
					   0, srcstr);
          ERR("wcstombs failed.  The string specified (%s) may contains an "
              "invalid character.\n", srcstr_ascii);
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_ascii) HeapFree(GetProcessHeap(), 0, srcstr_ascii);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }
      else if(returned_len > src_native_len)
      {
          src_native[src_native_len - 1] = 0;
          ERR("wcstombs returned a string (%s) that was longer (%d bytes) " 
              "than expected (%d bytes).\n", src_native, returned_len,
              dst_native_len);

          /* Since this is an internal error I'm not sure what the correct
           * error code is.  */
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);

          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }
      src_native_len = returned_len;

      TRACE_(string)("src_native = %s  src_native_len = %d\n",
             src_native, src_native_len);

      /* dst_native seems to contain at most 4 bytes for each byte in
       * the original src_native string.  Change if need be since this
       * isn't documented by the strxfrm() man page. */
      dst_native_len = 4 * src_native_len;
      if(!(dst_native = (LPSTR)HeapAlloc(GetProcessHeap(), 0, dst_native_len)))
      {
          ERR("Unable to allocate %d bytes for dst_native\n", dst_native_len);
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }

      /* The actual translation is done by the following call to
       * strxfrm().  The surrounding code could have been simplified
       * by calling wcsxfrm() instead except that wcsxfrm() is not
       * available on older Linux systems (RedHat 4.1 with
       * libc-5.3.12-17).
       *
       * Also, it is possible that the translation could be done by
       * various tables as it is done in LCMapStringA().  However, I'm
       * not sure what those tables are. */
      returned_len = strxfrm(dst_native, src_native, dst_native_len) + 1;
      
      if(returned_len > dst_native_len)
      {
          dst_native[dst_native_len - 1] = 0;
          ERR("strxfrm returned a string (%s) that was longer (%d bytes) " 
              "than expected (%d bytes).\n", dst_native, returned_len,
              dst_native_len);

          /* Since this is an internal error I'm not sure what the correct
           * error code is.  */
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);

          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }
      dst_native_len = returned_len;
      
      TRACE_(string)("dst_native = %s  dst_native_len = %d\n",
             dst_native, dst_native_len);

      dststr_libc_len = dst_native_len;
      if(!(dststr_libc = (wchar_t *)HeapAlloc(GetProcessHeap(), 0,
                                       dststr_libc_len * sizeof(wchar_t))))
      {
          ERR("Unable to allocate %d bytes for dststr_libc\n",
              dststr_libc_len * sizeof(wchar_t));
          SetLastError(ERROR_NOT_ENOUGH_MEMORY);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
          setlocale(LC_COLLATE, lc_collate_default);
          return 0;
      }

      /* Convert the native multibyte string to a libc Unicode string. */
      returned_len = mbstowcs(dststr_libc, dst_native, dst_native_len) + 1;

      /* Restore LC_COLLATE now that the last LC_COLLATE sensitive
       * function has returned. */
      setlocale(LC_COLLATE, lc_collate_default); 

      if(returned_len == 0)
      {
          ERR("mbstowcs failed.  The native version of the translated string "
              "(%s) may contain an invalid character.\n", dst_native);
          SetLastError(ERROR_INVALID_PARAMETER);
          if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
          if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
          if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
          if(dststr_libc) HeapFree(GetProcessHeap(), 0, dststr_libc);
          return 0;
      }
      if(dstlen)
      {
          if(returned_len > dstlen)
          {
              ERR("mbstowcs returned a string that was longer (%d chars) " 
                  "than the buffer provided (%d chars).\n", returned_len,
                  dstlen);
              SetLastError(ERROR_INSUFFICIENT_BUFFER);
              if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
              if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
              if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
              if(dststr_libc) HeapFree(GetProcessHeap(), 0, dststr_libc);
              return 0;          
          }
          dstlen = returned_len;

          /* Convert a libc Unicode string to the destination string. */
          for(str_idx = 0; str_idx < dstlen; str_idx++)
          {
              dststr[str_idx] = dststr_libc[str_idx];
          }
          TRACE_(string)("1st 4 int sized chunks of dststr = %x %x %x %x\n",
                         *(((int *)dststr) + 0),
                         *(((int *)dststr) + 1),
                         *(((int *)dststr) + 2),
                         *(((int *)dststr) + 3));
      }
      else
      {
          dstlen = returned_len;
      }
      TRACE_(string)("dstlen (return) = %d\n", dstlen);
      if(srcstr_libc) HeapFree(GetProcessHeap(), 0, srcstr_libc);
      if(src_native) HeapFree(GetProcessHeap(), 0, src_native);
      if(dst_native) HeapFree(GetProcessHeap(), 0, dst_native);
      if(dststr_libc) HeapFree(GetProcessHeap(), 0, dststr_libc);
      return dstlen;
  }
  else
  {
    int (*f)(int)=identity; 

    if (dstlen==0)
        return srclen;  
    if (dstlen<srclen) 
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return 0;
    }

    if (mapflags & LCMAP_UPPERCASE)
      f = toupper;
    else if (mapflags & LCMAP_LOWERCASE)
      f = tolower;
    for (i=0; i < srclen; i++)
      dststr[i] = (WCHAR) f(srcstr[i]);
    return srclen;
  }
}

/***********************************************************************
 *           CompareString16       (OLE2NLS.8)
 */
UINT16 WINAPI CompareString16(DWORD lcid,DWORD fdwStyle,
                              LPCSTR s1,DWORD l1,LPCSTR s2,DWORD l2)
{
	return (UINT16)CompareStringA(lcid,fdwStyle,s1,l1,s2,l2);
}

/***********************************************************************
 *           OLE2NLS_EstimateMappingLength
 *
 * Estimates the number of characters required to hold the string
 * computed by LCMapStringA.
 *
 * The size is always over-estimated, with a fixed limit on the
 * amount of estimation error.
 *
 * Note that len == -1 is not permitted.
 */
static inline int OLE2NLS_EstimateMappingLength(LCID lcid, DWORD dwMapFlags,
						LPCSTR str, DWORD len)
{
    /* Estimate only for small strings to keep the estimation error from
     * becoming too large. */
    if (len < 128) return len * 8 + 5;
    else return LCMapStringA(lcid, dwMapFlags, str, len, NULL, 0);
}

/******************************************************************************
 *		CompareStringA	[KERNEL32.143]
 * Compares two strings using locale
 *
 * RETURNS
 *
 * success: CSTR_LESS_THAN, CSTR_EQUAL, CSTR_GREATER_THAN
 * failure: 0
 *
 * NOTES
 *
 * Defaults to a word sort, but uses a string sort if
 * SORT_STRINGSORT is set.
 * Calls SetLastError for ERROR_INVALID_FLAGS, ERROR_INVALID_PARAMETER.
 * 
 * BUGS
 *
 * This implementation ignores the locale
 *
 * FIXME
 * 
 * Quite inefficient.
 */
UINT WINAPI CompareStringA(
    DWORD lcid,     /* locale ID */
    DWORD fdwStyle, /* comparison-style options */
    LPCSTR s1,      /* first string */
    DWORD l1,       /* length of first string */
    LPCSTR s2,      /* second string */
    DWORD l2)       /* length of second string */
{
  int mapstring_flags;
  int len1,len2;
  int result;
  LPSTR sk1,sk2;
  TRACE("%s and %s\n",
	debugstr_a (s1), debugstr_a (s2));

  if ( (s1==NULL) || (s2==NULL) )
  {    
    ERR("(s1=%s,s2=%s): Invalid NULL string\n", s1, s2);
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if(fdwStyle & NORM_IGNORESYMBOLS)
    FIXME("IGNORESYMBOLS not supported\n");

  if (l1 == -1) l1 = lstrlenA(s1);
  if (l2 == -1) l2 = lstrlenA(s2);
  	
  mapstring_flags = LCMAP_SORTKEY | fdwStyle ;
  len1 = OLE2NLS_EstimateMappingLength(lcid, mapstring_flags, s1, l1);
  len2 = OLE2NLS_EstimateMappingLength(lcid, mapstring_flags, s2, l2);

  if ((len1==0)||(len2==0))
    return 0;     /* something wrong happened */

  sk1 = (LPSTR)HeapAlloc(GetProcessHeap(), 0, len1 + len2);
  sk2 = sk1 + len1;
  if ( (!LCMapStringA(lcid,mapstring_flags,s1,l1,sk1,len1))
	 || (!LCMapStringA(lcid,mapstring_flags,s2,l2,sk2,len2)) )
  {
    ERR("Bug in LCmapStringA.\n");
    result = 0;
  }
  else
  {
    /* strcmp doesn't necessarily return -1, 0, or 1 */
    result = strcmp(sk1,sk2);
  }
  HeapFree(GetProcessHeap(),0,sk1);

  if (result < 0)
    return 1;
  if (result == 0)
    return 2;

  /* must be greater, if we reach this point */
  return 3;
}

/******************************************************************************
 *		CompareStringW	[KERNEL32.144]
 * This implementation ignores the locale
 * FIXME :  Does only string sort.  Should
 * be reimplemented the same way as CompareStringA.
 */
UINT WINAPI CompareStringW(DWORD lcid, DWORD fdwStyle, 
                               LPCWSTR s1, DWORD l1, LPCWSTR s2,DWORD l2)
{
	int len,ret;
	if(fdwStyle & NORM_IGNORENONSPACE)
		FIXME("IGNORENONSPACE not supprted\n");
	if(fdwStyle & NORM_IGNORESYMBOLS)
		FIXME("IGNORESYMBOLS not supported\n");

	/* Is strcmp defaulting to string sort or to word sort?? */
	/* FIXME: Handle NORM_STRINGSORT */
	l1 = (l1==-1)?lstrlenW(s1):l1;
	l2 = (l2==-1)?lstrlenW(s2):l2;
	len = l1<l2 ? l1:l2;
	ret = (fdwStyle & NORM_IGNORECASE) ?
		CRTDLL__wcsnicmp(s1,s2,len) : CRTDLL_wcsncmp(s1,s2,len);
	/* not equal, return 1 or 3 */
	if(ret!=0) return ret+2;
	/* same len, return 2 */
	if(l1==l2) return 2;
	/* the longer one is lexically greater */
	return (l1<l2)? 1 : 3;
}

/******************************************************************************
 *      RegisterNLSInfoChanged  [OLE2NLS.10]
 */
BOOL16 WINAPI RegisterNLSInfoChanged16(LPVOID/*FIXME*/ lpNewNLSInfo)
{
	FIXME("Fully implemented, but doesn't effect anything.\n");

	if (!lpNewNLSInfo)
	{
		lpNLSInfo = NULL;
		return TRUE;
	}
	else
	{
		if (!lpNLSInfo)
		{
			lpNLSInfo = lpNewNLSInfo;
			return TRUE;
		}
	}

	return FALSE; /* ptr not set */
} 

/******************************************************************************
 *		OLE_GetFormatA	[Internal]
 *
 * FIXME
 *    If datelen == 0, it should return the reguired string length.
 *
 This function implements stuff for GetDateFormat() and 
 GetTimeFormat().

  d    single-digit (no leading zero) day (of month)
  dd   two-digit day (of month)
  ddd  short day-of-week name
  dddd long day-of-week name
  M    single-digit month
  MM   two-digit month
  MMM  short month name
  MMMM full month name
  y    two-digit year, no leading 0
  yy   two-digit year
  yyyy four-digit year
  gg   era string
  h    hours with no leading zero (12-hour)
  hh   hours with full two digits
  H    hours with no leading zero (24-hour)
  HH   hours with full two digits
  m    minutes with no leading zero
  mm   minutes with full two digits
  s    seconds with no leading zero
  ss   seconds with full two digits
  t    time marker (A or P)
  tt   time marker (AM, PM)
  ''   used to quote literal characters
  ''   (within a quoted string) indicates a literal '

 These functions REQUIRE valid locale, date,  and format. 
 */
static INT OLE_GetFormatA(LCID locale,
			    DWORD flags,
			    DWORD tflags,
			    LPSYSTEMTIME xtime,
			    LPCSTR _format, 	/*in*/
			    LPSTR date,		/*out*/
			    INT datelen)
{
   INT inpos, outpos;
   int count, type, inquote, Overflow;
   char buf[40];
   char format[40];
   char * pos;
   int buflen;

   const char * _dgfmt[] = { "%d", "%02d" };
   const char ** dgfmt = _dgfmt - 1; 

   /* report, for debugging */
   TRACE("(0x%lx,0x%lx, 0x%lx, time(d=%d,h=%d,m=%d,s=%d), fmt=%p \'%s\' , %p, len=%d)\n",
   	 locale, flags, tflags,
	 xtime->wDay, xtime->wHour, xtime->wMinute, xtime->wSecond,
	 _format, _format, date, datelen);
  
   if(datelen == 0) {
     FIXME("datelen = 0, returning 255\n");
     return 255;
   }

   /* initalize state variables and output buffer */
   inpos = outpos = 0;
   count = 0; inquote = 0; Overflow = 0;
   type = '\0';
   date[0] = buf[0] = '\0';
      
   strcpy(format,_format);

   /* alter the formatstring, while it works for all languages now in wine
   its possible that it fails when the time looks like ss:mm:hh as example*/   
   if (tflags & (TIME_NOMINUTESORSECONDS))
   { if ((pos = strstr ( format, ":mm")))
     { memcpy ( pos, pos+3, strlen(format)-(pos-format)-2 );
     }
   }
   if (tflags & (TIME_NOSECONDS))
   { if ((pos = strstr ( format, ":ss")))
     { memcpy ( pos, pos+3, strlen(format)-(pos-format)-2 );
     }
   }
   
   for (inpos = 0;; inpos++) {
      /* TRACE(ole, "STATE inpos=%2d outpos=%2d count=%d inquote=%d type=%c buf,date = %c,%c\n", inpos, outpos, count, inquote, type, buf[inpos], date[outpos]); */
      if (inquote) {
	 if (format[inpos] == '\'') {
	    if (format[inpos+1] == '\'') {
	       inpos += 1;
	       date[outpos++] = '\'';
	    } else {
	       inquote = 0;
	       continue; /* we did nothing to the output */
	    }
	 } else if (format[inpos] == '\0') {
	    date[outpos++] = '\0';
	    if (outpos > datelen) Overflow = 1;
	    break;
	 } else {
	    date[outpos++] = format[inpos];
	    if (outpos > datelen) {
	       Overflow = 1;
	       date[outpos-1] = '\0'; /* this is the last place where
					 it's safe to write */
	       break;
	    }
	 }
      } else if (  (count && (format[inpos] != type))
		   || count == 4
		   || (count == 2 && strchr("ghHmst", type)) )
       {
	    if         (type == 'd') {
	       if        (count == 4) {
		  GetLocaleInfoA(locale,
				   LOCALE_SDAYNAME1
				   + xtime->wDayOfWeek - 1,
				   buf, sizeof(buf));
	       } else if (count == 3) {
			   GetLocaleInfoA(locale, 
					    LOCALE_SABBREVDAYNAME1 
					    + xtime->wDayOfWeek - 1,
					    buf, sizeof(buf));
		      } else {
		  sprintf(buf, dgfmt[count], xtime->wDay);
	       }
	    } else if (type == 'M') {
	       if (count == 3) {
		  GetLocaleInfoA(locale, 
				   LOCALE_SABBREVMONTHNAME1
				   + xtime->wMonth - 1,
				   buf, sizeof(buf));
	       } else if (count == 4) {
		  GetLocaleInfoA(locale,
				   LOCALE_SMONTHNAME1
				   + xtime->wMonth - 1,
				   buf, sizeof(buf));
		 } else {
		  sprintf(buf, dgfmt[count], xtime->wMonth);
	       }
	    } else if (type == 'y') {
	       if (count == 4) {
		      sprintf(buf, "%d", xtime->wYear);
	       } else if (count == 3) {
		  strcpy(buf, "yyy");
		  WARN("unknown format, c=%c, n=%d\n",  type, count);
		 } else {
		  sprintf(buf, dgfmt[count], xtime->wYear % 100);
	       }
	    } else if (type == 'g') {
	       if        (count == 2) {
		  FIXME("LOCALE_ICALENDARTYPE unimp.\n");
		  strcpy(buf, "AD");
	    } else {
		  strcpy(buf, "g");
		  WARN("unknown format, c=%c, n=%d\n", type, count);
	       }
	    } else if (type == 'h') {
	       /* gives us hours 1:00 -- 12:00 */
	       sprintf(buf, dgfmt[count], (xtime->wHour-1)%12 +1);
	    } else if (type == 'H') {
	       /* 24-hour time */
	       sprintf(buf, dgfmt[count], xtime->wHour);
	    } else if ( type == 'm') {
	       sprintf(buf, dgfmt[count], xtime->wMinute);
	    } else if ( type == 's') {
	       sprintf(buf, dgfmt[count], xtime->wSecond);
	    } else if (type == 't') {
	       if        (count == 1) {
		  sprintf(buf, "%c", (xtime->wHour < 12) ? 'A' : 'P');
	       } else if (count == 2) {
		  /* sprintf(buf, "%s", (xtime->wHour < 12) ? "AM" : "PM"); */
		  GetLocaleInfoA(locale,
				   (xtime->wHour<12) 
				   ? LOCALE_S1159 : LOCALE_S2359,
				   buf, sizeof(buf));
	       }
	    };

	    /* we need to check the next char in the format string 
	       again, no matter what happened */
	    inpos--;
	    
	    /* add the contents of buf to the output */
	    buflen = strlen(buf);
	    if (outpos + buflen < datelen) {
	       date[outpos] = '\0'; /* for strcat to hook onto */
		 strcat(date, buf);
	       outpos += buflen;
	    } else {
	       date[outpos] = '\0';
	       strncat(date, buf, datelen - outpos);
		 date[datelen - 1] = '\0';
		 SetLastError(ERROR_INSUFFICIENT_BUFFER);
	       WARN("insufficient buffer\n");
		 return 0;
	    }

	    /* reset the variables we used to keep track of this item */
	    count = 0;
	    type = '\0';
	 } else if (format[inpos] == '\0') {
	    /* we can't check for this at the loop-head, because
	       that breaks the printing of the last format-item */
	    date[outpos] = '\0';
	    break;
         } else if (count) {
	    /* continuing a code for an item */
	    count +=1;
	    continue;
	 } else if (strchr("hHmstyMdg", format[inpos])) {
	    type = format[inpos];
	    count = 1;
	    continue;
	 } else if (format[inpos] == '\'') {
	    inquote = 1;
	    continue;
       } else {
	    date[outpos++] = format[inpos];
	 }
      /* now deal with a possible buffer overflow */
      if (outpos >= datelen) {
       date[datelen - 1] = '\0';
       SetLastError(ERROR_INSUFFICIENT_BUFFER);
       return 0;
      }
   }
   
   if (Overflow) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
   };

   /* finish it off with a string terminator */
   outpos++;
   /* sanity check */
   if (outpos > datelen-1) outpos = datelen-1;
   date[outpos] = '\0';
   
   TRACE("OLE_GetFormatA returns string '%s', len %d\n",
	       date, outpos);
   return outpos;
}

/******************************************************************************
 * OLE_GetFormatW [INTERNAL]
 */
static INT OLE_GetFormatW(LCID locale, DWORD flags, DWORD tflags,
			    LPSYSTEMTIME xtime,
			    LPCWSTR format,
			    LPWSTR output, INT outlen)
{
   INT   inpos, outpos;
   int     count, type=0, inquote;
   int     Overflow; /* loop check */
   WCHAR   buf[40];
   int     buflen=0;
   WCHAR   arg0[] = {0}, arg1[] = {'%','d',0};
   WCHAR   arg2[] = {'%','0','2','d',0};
   WCHAR  *argarr[3];
   int     datevars=0, timevars=0;

   argarr[0] = arg0;
   argarr[1] = arg1;
   argarr[2] = arg2;

   /* make a debug report */
   TRACE("args: 0x%lx, 0x%lx, 0x%lx, time(d=%d,h=%d,m=%d,s=%d), fmt:%s (at %p), "
   	      "%p with max len %d\n",
	 locale, flags, tflags,
	 xtime->wDay, xtime->wHour, xtime->wMinute, xtime->wSecond,
	 debugstr_w(format), format, output, outlen);
   
   if(outlen == 0) {
     FIXME("outlen = 0, returning 255\n");
     return 255;
   }

   /* initialize state variables */
   inpos = outpos = 0;
   count = 0;
   inquote = Overflow = 0;
   /* this is really just a sanity check */
   output[0] = buf[0] = 0;
   
   /* this loop is the core of the function */
   for (inpos = 0; /* we have several break points */ ; inpos++) {
      if (inquote) {
	 if (format[inpos] == (WCHAR) '\'') {
	    if (format[inpos+1] == '\'') {
	       inpos++;
	       output[outpos++] = '\'';
	    } else {
	       inquote = 0;
	       continue;
	    }
	 } else if (format[inpos] == 0) {
	    output[outpos++] = 0;
	    if (outpos > outlen) Overflow = 1;
	    break;  /*  normal exit (within a quote) */
	 } else {
	    output[outpos++] = format[inpos]; /* copy input */
	    if (outpos > outlen) {
	       Overflow = 1;
	       output[outpos-1] = 0; 
	       break;
	    }
	 }
      } else if (  (count && (format[inpos] != type))
		   || ( (count==4 && type =='y') ||
			(count==4 && type =='M') ||
			(count==4 && type =='d') ||
			(count==2 && type =='g') ||
			(count==2 && type =='h') ||
			(count==2 && type =='H') ||
			(count==2 && type =='m') ||
			(count==2 && type =='s') ||
			(count==2 && type =='t') )  ) {
	 if        (type == 'd') {
	    if        (count == 3) {
	       GetLocaleInfoW(locale,
			     LOCALE_SDAYNAME1 + xtime->wDayOfWeek -1,
			     buf, sizeof(buf)/sizeof(WCHAR) );
	    } else if (count == 3) {
	       GetLocaleInfoW(locale,
				LOCALE_SABBREVDAYNAME1 +
				xtime->wDayOfWeek -1,
				buf, sizeof(buf)/sizeof(WCHAR) );
	    } else {
	       wsnprintfW(buf, 5, argarr[count], xtime->wDay );
	    };
	 } else if (type == 'M') {
	    if        (count == 4) {
	       GetLocaleInfoW(locale,  LOCALE_SMONTHNAME1 +
				xtime->wMonth -1, buf,
				sizeof(buf)/sizeof(WCHAR) );
	    } else if (count == 3) {
	       GetLocaleInfoW(locale,  LOCALE_SABBREVMONTHNAME1 +
				xtime->wMonth -1, buf,
				sizeof(buf)/sizeof(WCHAR) );
	    } else {
	       wsnprintfW(buf, 5, argarr[count], xtime->wMonth);
	    }
	 } else if (type == 'y') {
	    if        (count == 4) {
	       wsnprintfW(buf, 6, argarr[1] /* "%d" */,
			 xtime->wYear);
	    } else if (count == 3) {
	       lstrcpynAtoW(buf, "yyy", 5);
	    } else {
	       wsnprintfW(buf, 6, argarr[count],
			    xtime->wYear % 100);
	    }
	 } else if (type == 'g') {
	    if        (count == 2) {
	       FIXME("LOCALE_ICALENDARTYPE unimplemented\n");
	       lstrcpynAtoW(buf, "AD", 5);
	    } else {
	       /* Win API sez we copy it verbatim */
	       lstrcpynAtoW(buf, "g", 5);
	    }
	 } else if (type == 'h') {
	    /* hours 1:00-12:00 --- is this right? */
	    wsnprintfW(buf, 5, argarr[count], 
			 (xtime->wHour-1)%12 +1);
	 } else if (type == 'H') {
	    wsnprintfW(buf, 5, argarr[count], 
			 xtime->wHour);
	 } else if (type == 'm' ) {
	    wsnprintfW(buf, 5, argarr[count],
			 xtime->wMinute);
	 } else if (type == 's' ) {
	    wsnprintfW(buf, 5, argarr[count],
			 xtime->wSecond);
	 } else if (type == 't') {
	    GetLocaleInfoW(locale, (xtime->wHour < 12) ?
			     LOCALE_S1159 : LOCALE_S2359,
			     buf, sizeof(buf) );
	    if        (count == 1) {
	       buf[1] = 0;
	    }
}

	 /* no matter what happened,  we need to check this next 
	    character the next time we loop through */
	 inpos--;

	 /* cat buf onto the output */
	 outlen = lstrlenW(buf);
	 if (outpos + buflen < outlen) {
            lstrcpyW( output + outpos, buf );
	    outpos += buflen;
	 } else {
            lstrcpynW( output + outpos, buf, outlen - outpos );
	    Overflow = 1;
	    break; /* Abnormal exit */
	 }

	 /* reset the variables we used this time */
	 count = 0;
	 type = '\0';
      } else if (format[inpos] == 0) {
	 /* we can't check for this at the beginning,  because that 
	 would keep us from printing a format spec that ended the 
	 string */
	 output[outpos] = 0;
	 break;  /*  NORMAL EXIT  */
      } else if (count) {
	 /* how we keep track of the middle of a format spec */
	 count++;
	 continue;
      } else if ( (datevars && (format[inpos]=='d' ||
				format[inpos]=='M' ||
				format[inpos]=='y' ||
				format[inpos]=='g')  ) ||
		  (timevars && (format[inpos]=='H' ||
				format[inpos]=='h' ||
				format[inpos]=='m' ||
				format[inpos]=='s' ||
				format[inpos]=='t') )    ) {
	 type = format[inpos];
	 count = 1;
	 continue;
      } else if (format[inpos] == '\'') {
	 inquote = 1;
	 continue;
      } else {
	 /* unquoted literals */
	 output[outpos++] = format[inpos];
      }
   }

   if (Overflow) {
      SetLastError(ERROR_INSUFFICIENT_BUFFER);
      WARN(" buffer overflow\n");
   };

   /* final string terminator and sanity check */
   outpos++;
   if (outpos > outlen-1) outpos = outlen-1;
   output[outpos] = '0';

   TRACE(" returning %s\n", debugstr_w(output));
	
   return (!Overflow) ? outlen : 0;
   
}


/******************************************************************************
 *		GetDateFormatA	[KERNEL32.310]
 * Makes an ASCII string of the date
 *
 * This function uses format to format the date,  or,  if format
 * is NULL, uses the default for the locale.  format is a string
 * of literal fields and characters as follows:
 *
 * - d    single-digit (no leading zero) day (of month)
 * - dd   two-digit day (of month)
 * - ddd  short day-of-week name
 * - dddd long day-of-week name
 * - M    single-digit month
 * - MM   two-digit month
 * - MMM  short month name
 * - MMMM full month name
 * - y    two-digit year, no leading 0
 * - yy   two-digit year
 * - yyyy four-digit year
 * - gg   era string
 *
 */
INT WINAPI GetDateFormatA(LCID locale,DWORD flags,
			      LPSYSTEMTIME xtime,
			      LPCSTR format, LPSTR date,INT datelen) 
{
   
  char format_buf[40];
  LPCSTR thisformat;
  SYSTEMTIME t;
  LPSYSTEMTIME thistime;
  LCID thislocale;
  INT ret;

  TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",
	      locale,flags,xtime,format,date,datelen);
  
  if (!locale) {
     locale = LOCALE_SYSTEM_DEFAULT;
     };
  
  if (locale == LOCALE_SYSTEM_DEFAULT) {
     thislocale = GetSystemDefaultLCID();
  } else if (locale == LOCALE_USER_DEFAULT) {
     thislocale = GetUserDefaultLCID();
  } else {
     thislocale = locale;
   };

  if (xtime == NULL) {
     GetSystemTime(&t);
     thistime = &t;
  } else {
     thistime = xtime;
  };

  if (format == NULL) {
     GetLocaleInfoA(thislocale, ((flags&DATE_LONGDATE) 
				   ? LOCALE_SLONGDATE
				   : LOCALE_SSHORTDATE),
		      format_buf, sizeof(format_buf));
     thisformat = format_buf;
  } else {
     thisformat = format;
  };

  
  ret = OLE_GetFormatA(thislocale, flags, 0, thistime, thisformat, 
		       date, datelen);
  

   TRACE(
	       "GetDateFormatA() returning %d, with data=%s\n",
	       ret, date);
  return ret;
}

/******************************************************************************
 *		GetDateFormatW	[KERNEL32.311]
 * Makes a Unicode string of the date
 *
 * Acts the same as GetDateFormatA(),  except that it's Unicode.
 * Accepts & returns sizes as counts of Unicode characters.
 *
 */
INT WINAPI GetDateFormatW(LCID locale,DWORD flags,
			      LPSYSTEMTIME xtime,
			      LPCWSTR format,
			      LPWSTR date, INT datelen)
{
   unsigned short datearr[] = {'1','9','9','4','-','1','-','1',0};

   FIXME("STUB (should call OLE_GetFormatW)\n");   
   lstrcpynW(date, datearr, datelen);
   return (  datelen < 9) ? datelen : 9;
   
   
}

/**************************************************************************
 *              EnumDateFormatsA	(KERNEL32.198)
 */
BOOL WINAPI EnumDateFormatsA(
  DATEFMT_ENUMPROCA lpDateFmtEnumProc, LCID Locale,  DWORD dwFlags)
{
  LCID Loc = GetUserDefaultLCID(); 
  FIXME("Only different English Locales supported \n");

  if(!lpDateFmtEnumProc)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  switch( Loc )
 {

   case 0x00000809:  /* (Loc,"en_UK") */ 
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd/MM/yyyy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d/M/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d.M.yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dd MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00000c09:  /* (Loc,"en_AU") */   
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("d/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d/M/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dddd,d MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00001009:  /* (Loc,"en_CA") */ 
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d/M/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("yy-MM-dd")) return TRUE;
	if(!(*lpDateFmtEnumProc)("M/dd/yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("d-MMM-yy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("MMMM d, yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00001409:  /* (Loc,"en_NZ") */ 
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("d/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d.MM.yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("d MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("dddd, d MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00001809:  /* (Loc,"en_IE") */   
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d/M/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d.M.yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dd MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00001c09:  /* (Loc,"en_ZA") */   
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("yy/MM/dd")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dd MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00002009:  /* (Loc,"en_JM") */  
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd/MM/yyyy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dddd,MMMM dd,yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("MMMM dd,yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("dddd,dd MMMM,yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("dd MMMM,yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   case 0x00002809:  /* (Loc,"en_BZ") */
   case 0x00002c09:  /* (Loc,"en_TT") */  
  {    
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd/MM/yyyy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dddd,dd MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }

   default:  /* default to US English "en_US" */
  {
   switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("M/d/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("M/d/yyyy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("MM/dd/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("MM/dd/yyyy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("yy/MM/dd")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd-MMM-yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dddd, MMMM dd, yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("MMMM dd, yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("dddd, dd MMMM, yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("dd MMMM, yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags); 
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
    }
  }
 }
}

/**************************************************************************
 *              EnumDateFormatsW	(KERNEL32.199)
 */
BOOL WINAPI EnumDateFormatsW(
  DATEFMT_ENUMPROCW lpDateFmtEnumProc, LCID Locale, DWORD dwFlags)
{
  FIXME("(%p, %ld, %ld): stub\n", lpDateFmtEnumProc, Locale, dwFlags);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/**************************************************************************
 *              EnumTimeFormatsA	(KERNEL32.210)
 */
BOOL WINAPI EnumTimeFormatsA(
  TIMEFMT_ENUMPROCA lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags)
{
  LCID Loc = GetUserDefaultLCID(); 
  if(!lpTimeFmtEnumProc)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  if(dwFlags)
    {
      FIXME("Unknown time format (%ld)\n", dwFlags); 
    }

  switch( Loc )
 {
   case 0x00000407:  /* (Loc,"de_DE") */
   {
    if(!(*lpTimeFmtEnumProc)("HH.mm")) return TRUE; 
    if(!(*lpTimeFmtEnumProc)("HH:mm:ss")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("H:mm:ss")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("H.mm")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("H.mm'Uhr'")) return TRUE;
    return TRUE;
   }       

   case 0x0000040c:  /* (Loc,"fr_FR") */
   case 0x00000c0c:  /* (Loc,"fr_CA") */
   {
    if(!(*lpTimeFmtEnumProc)("H:mm")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("HH:mm:ss")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("H:mm:ss")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("HH.mm")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("HH'h'mm")) return TRUE;
    return TRUE;
   }

   case 0x00000809:  /* (Loc,"en_UK") */
   case 0x00000c09:  /* (Loc,"en_AU") */ 
   case 0x00001409:  /* (Loc,"en_NZ") */
   case 0x00001809:  /* (Loc,"en_IE") */ 
   {
    if(!(*lpTimeFmtEnumProc)("h:mm:ss tt")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("HH:mm:ss")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("H:mm:ss")) return TRUE;
    return TRUE;
   }

   case 0x00001c09:  /* (Loc,"en_ZA") */   
   case 0x00002809:  /* (Loc,"en_BZ") */
   case 0x00002c09:  /* (Loc,"en_TT") */ 
   {
    if(!(*lpTimeFmtEnumProc)("h:mm:ss tt")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("hh:mm:ss tt")) return TRUE; 
    return TRUE;  
   }

   default:  /* default to US style "en_US" */   
   {
    if(!(*lpTimeFmtEnumProc)("h:mm:ss tt")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("hh:mm:ss tt")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("H:mm:ss")) return TRUE;
    if(!(*lpTimeFmtEnumProc)("HH:mm:ss")) return TRUE;
    return TRUE;
   }
 }
}

/**************************************************************************
 *              EnumTimeFormatsW	(KERNEL32.211)
 */
BOOL WINAPI EnumTimeFormatsW(
  TIMEFMT_ENUMPROCW lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags)
{
  FIXME("(%p,%ld,%ld): stub\n", lpTimeFmtEnumProc, Locale, dwFlags);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/**************************************************************************
 *           This function is used just locally !
 *  Description: Inverts a string.
 */ 
static void OLE_InvertString(char* string)
{    
    char    sTmpArray[128];
    INT     counter, i = 0;

    for (counter = strlen(string); counter > 0; counter--)
    {
        memcpy(sTmpArray + i, string + counter-1, 1);
        i++;
    }
    memcpy(sTmpArray + i, "\0", 1);
    strcpy(string, sTmpArray);
}

/***************************************************************************************
 *           This function is used just locally !
 *  Description: Test if the given string (psNumber) is valid or not.
 *               The valid characters are the following:
 *               - Characters '0' through '9'. 
 *               - One decimal point (dot) if the number is a floating-point value. 
 *               - A minus sign in the first character position if the number is 
 *                 a negative value.
 *              If the function succeeds, psBefore/psAfter will point to the string
 *              on the right/left of the decimal symbol. pbNegative indicates if the 
 *              number is negative.
 */
static INT OLE_GetNumberComponents(char* pInput, char* psBefore, char* psAfter, BOOL* pbNegative)
{
char	sNumberSet[] = "0123456789";
BOOL	bInDecimal = FALSE;

	/* Test if we do have a minus sign */
	if ( *pInput == '-' )
	{
		*pbNegative = TRUE;
		pInput++; /* Jump to the next character. */
	}
	
	while(*pInput != '\0')
	{
		/* Do we have a valid numeric character */
		if ( strchr(sNumberSet, *pInput) != NULL )
		{
			if (bInDecimal == TRUE)
                *psAfter++ = *pInput;
			else
                *psBefore++ = *pInput;
		}
		else
		{
			/* Is this a decimal point (dot) */
			if ( *pInput == '.' )
			{
				/* Is it the first time we find it */
				if ((bInDecimal == FALSE))
					bInDecimal = TRUE;
				else
					return -1; /* ERROR: Invalid parameter */
			}
			else
			{
				/* It's neither a numeric character, nor a decimal point.
				 * Thus, return an error.
                 */
				return -1;
			}
		}
        pInput++;
	}
	
	/* Add an End of Line character to the output buffers */
	*psBefore = '\0';
	*psAfter = '\0';

	return 0; 
}

/**************************************************************************
 *           This function is used just locally !
 *  Description: A number could be formatted using different numbers 
 *               of "digits in group" (example: 4;3;2;0).
 *               The first parameter of this function is an array
 *               containing the rule to be used. It's format is the following:
 *               |NDG|DG1|DG2|...|0|
 *               where NDG is the number of used "digits in group" and DG1, DG2,
 *               are the corresponding "digits in group".
 *               Thus, this function returns the grouping value in the array
 *               pointed by the second parameter.
 */
static INT OLE_GetGrouping(char* sRule, INT index)
{
    char    sData[2], sRuleSize[2];
    INT     nData, nRuleSize;

    memcpy(sRuleSize, sRule, 1);
    memcpy(sRuleSize+1, "\0", 1);
    nRuleSize = atoi(sRuleSize);

    if (index > 0 && index < nRuleSize)
    {
        memcpy(sData, sRule+index, 1);
        memcpy(sData+1, "\0", 1);
        nData = atoi(sData);            
    }
        
    else
    {
        memcpy(sData, sRule+nRuleSize-1, 1);
        memcpy(sData+1, "\0", 1);
        nData = atoi(sData);            
    }
    
    return nData;
}

/**************************************************************************
 *              GetNumberFormatA	(KERNEL32.355)
 */
INT WINAPI GetNumberFormatA(LCID locale, DWORD dwflags,
			       LPCSTR lpvalue,   const NUMBERFMTA * lpFormat,
			       LPSTR lpNumberStr, int cchNumber)
{
    char   sNumberDigits[3], sDecimalSymbol[5], sDigitsInGroup[11], sDigitGroupSymbol[5], sILZero[2];
    INT    nNumberDigits, nNumberDecimal, i, j, nCounter, nStep, nRuleIndex, nGrouping, nDigits, retVal, nLZ;
    char   sNumber[128], sDestination[128], sDigitsAfterDecimal[10], sDigitsBeforeDecimal[128];
    char   sRule[10], sSemiColumn[]=";", sBuffer[5], sNegNumber[2];    
    char   *pStr = NULL, *pTmpStr = NULL;
    LCID   systemDefaultLCID;
    BOOL   bNegative = FALSE;
    enum   Operations 
    {
        USE_PARAMETER,
        USE_LOCALEINFO,
        USE_SYSTEMDEFAULT,
        RETURN_ERROR
    } used_operation;

    strncpy(sNumber, lpvalue, 128);
    sNumber[127] = '\0';

    /* Make sure we have a valid input string, get the number
     * of digits before and after the decimal symbol, and check
     * if this is a negative number.
     */
    if ( OLE_GetNumberComponents(sNumber, sDigitsBeforeDecimal, sDigitsAfterDecimal, &bNegative) != -1)
    {
        nNumberDecimal = strlen(sDigitsBeforeDecimal);
        nDigits = strlen(sDigitsAfterDecimal);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* Which source will we use to format the string */
    used_operation = RETURN_ERROR;
    if (lpFormat != NULL)
    {
        if (dwflags == 0)
            used_operation = USE_PARAMETER; 
    }
    else
    {
        if (dwflags & LOCALE_NOUSEROVERRIDE)
            used_operation = USE_LOCALEINFO;
        else
            used_operation = USE_SYSTEMDEFAULT;
    }

    /* Load the fields we need */
    switch(used_operation)
    {
    case USE_LOCALEINFO:
        GetLocaleInfoA(locale, LOCALE_IDIGITS, sNumberDigits, sizeof(sNumberDigits));
        GetLocaleInfoA(locale, LOCALE_SDECIMAL, sDecimalSymbol, sizeof(sDecimalSymbol));
        GetLocaleInfoA(locale, LOCALE_SGROUPING, sDigitsInGroup, sizeof(sDigitsInGroup));
        GetLocaleInfoA(locale, LOCALE_STHOUSAND, sDigitGroupSymbol, sizeof(sDigitGroupSymbol));
        GetLocaleInfoA(locale, LOCALE_ILZERO, sILZero, sizeof(sILZero));
        GetLocaleInfoA(locale, LOCALE_INEGNUMBER, sNegNumber, sizeof(sNegNumber));
        break;
    case USE_PARAMETER:
        sprintf(sNumberDigits, "%d",lpFormat->NumDigits);
        strcpy(sDecimalSymbol, lpFormat->lpDecimalSep);
        sprintf(sDigitsInGroup, "%d;0",lpFormat->Grouping);
        strcpy(sDigitGroupSymbol, lpFormat->lpThousandSep);
        sprintf(sILZero, "%d",lpFormat->LeadingZero);
        sprintf(sNegNumber, "%d",lpFormat->NegativeOrder);
        break;
    case USE_SYSTEMDEFAULT:
        systemDefaultLCID = GetSystemDefaultLCID();
        GetLocaleInfoA(systemDefaultLCID, LOCALE_IDIGITS, sNumberDigits, sizeof(sNumberDigits));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_SDECIMAL, sDecimalSymbol, sizeof(sDecimalSymbol));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_SGROUPING, sDigitsInGroup, sizeof(sDigitsInGroup));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_STHOUSAND, sDigitGroupSymbol, sizeof(sDigitGroupSymbol));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_ILZERO, sILZero, sizeof(sILZero));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_INEGNUMBER, sNegNumber, sizeof(sNegNumber));
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    nNumberDigits = atoi(sNumberDigits);
    
    /* Remove the ";" */
    i=0;
    j = 1;
    for (nCounter=0; nCounter<strlen(sDigitsInGroup); nCounter++)
    {
        if ( memcmp(sDigitsInGroup + nCounter, sSemiColumn, 1) != 0 )
        {
            memcpy(sRule + j, sDigitsInGroup + nCounter, 1);
            i++;
            j++;
        }
    }
    sprintf(sBuffer, "%d", i);
    memcpy(sRule, sBuffer, 1); /* Number of digits in the groups ( used by OLE_GetGrouping() ) */
    memcpy(sRule + j, "\0", 1);
    
    /* First, format the digits before the decimal. */
    if ((nNumberDecimal>0) && (atoi(sDigitsBeforeDecimal) != 0)) 
    {
        /* Working on an inverted string is easier ! */
        OLE_InvertString(sDigitsBeforeDecimal);

        nStep = nCounter = i = j = 0;
        nRuleIndex = 1;
        nGrouping = OLE_GetGrouping(sRule, nRuleIndex);
        
        /* Here, we will loop until we reach the end of the string.
         * An internal counter (j) is used in order to know when to 
         * insert the "digit group symbol".
         */
        while (nNumberDecimal > 0)
        {
            i = nCounter + nStep;
            memcpy(sDestination + i, sDigitsBeforeDecimal + nCounter, 1);
            nCounter++;
            j++;
            if (j >= nGrouping)
            {
                j = 0;
                if (nRuleIndex < sRule[0])
                    nRuleIndex++;
                nGrouping = OLE_GetGrouping(sRule, nRuleIndex);
                memcpy(sDestination + i+1, sDigitGroupSymbol, strlen(sDigitGroupSymbol));
                nStep+= strlen(sDigitGroupSymbol);
            }

            nNumberDecimal--;
        }

        memcpy(sDestination + i+1, "\0", 1);
        /* Get the string in the right order ! */
        OLE_InvertString(sDestination);
     }
     else
     {
        nLZ = atoi(sILZero);
        if (nLZ != 0)
        {        
            /* Use 0.xxx instead of .xxx */
            memcpy(sDestination, "0", 1);
            memcpy(sDestination+1, "\0", 1);
        }
        else
            memcpy(sDestination, "\0", 1);

     }

    /* Second, format the digits after the decimal. */
    j = 0;
    nCounter = nNumberDigits;
    if ( (nDigits>0) && (pStr = strstr (sNumber, ".")) )
    {
        i = strlen(sNumber) - strlen(pStr) + 1;        
        strncpy ( sDigitsAfterDecimal, sNumber + i, nNumberDigits);
        j = strlen(sDigitsAfterDecimal);
        if (j < nNumberDigits)
            nCounter = nNumberDigits-j;            
    }
    for (i=0;i<nCounter;i++)
         memcpy(sDigitsAfterDecimal+i+j, "0", 1);    
    memcpy(sDigitsAfterDecimal + nNumberDigits, "\0", 1);    

    i = strlen(sDestination);
    j = strlen(sDigitsAfterDecimal);
    /* Finally, construct the resulting formatted string. */
        
    for (nCounter=0; nCounter<i; nCounter++)
        memcpy(sNumber + nCounter, sDestination + nCounter, 1);
       
    memcpy(sNumber + nCounter, sDecimalSymbol, strlen(sDecimalSymbol));

    for (i=0; i<j; i++)
        memcpy(sNumber + nCounter+i+strlen(sDecimalSymbol), sDigitsAfterDecimal + i, 1);
    memcpy(sNumber + nCounter+i+strlen(sDecimalSymbol), "\0", 1);
        
    /* Is it a negative number */
    if (bNegative == TRUE)
    {
        i = atoi(sNegNumber);
        pStr = sDestination;
        pTmpStr = sNumber;
        switch (i)
        {          
        case 0:
            *pStr++ = '(';
            while (*sNumber != '\0')
                *pStr++ =  *pTmpStr++;
            *pStr++ = ')';                
            break;  
        case 1:
            *pStr++ = '-';
            while (*pTmpStr != '\0')
                *pStr++ =  *pTmpStr++;
            break;
        case 2:
            *pStr++ = '-';
            *pStr++ = ' ';
            while (*pTmpStr != '\0')
                *pStr++ =  *pTmpStr++;
            break;
        case 3:
            while (*pTmpStr != '\0')
                *pStr++ =  *pTmpStr++;
            *pStr++ = '-';
            break;
        case 4:
            while (*pTmpStr != '\0')
                *pStr++ =  *pTmpStr++;
            *pStr++ = ' ';
            *pStr++ = '-';
            break;
        default:
            while (*pTmpStr != '\0')
                *pStr++ =  *pTmpStr++;
            break;
        }
    }
    else
        strcpy(sDestination, sNumber);

    /* If cchNumber is zero, then returns the number of bytes or characters 
     * required to hold the formatted number string
     */
    if (cchNumber==0)
        retVal = strlen(sDestination) + 1;
    else           
    {
        strncpy (lpNumberStr, sDestination, cchNumber-1);
        *(lpNumberStr+cchNumber-1) = '\0';   /* ensure we got a NULL at the end */
        retVal = strlen(lpNumberStr);
    }
          
    return retVal;
}

/**************************************************************************
 *              GetNumberFormatW	(KERNEL32.xxx)
 */
INT WINAPI GetNumberFormatW(LCID locale, DWORD dwflags,
			       LPCWSTR lpvalue,  const NUMBERFMTW * lpFormat,
			       LPWSTR lpNumberStr, int cchNumber)
{
 FIXME("%s: stub, no reformating done\n",debugstr_w(lpvalue));

 lstrcpynW( lpNumberStr, lpvalue, cchNumber );
 return cchNumber? lstrlenW( lpNumberStr ) : 0;
}

/**************************************************************************
 *              GetCurrencyFormatA	(KERNEL32.302)
 */
INT WINAPI GetCurrencyFormatA(LCID locale, DWORD dwflags,
			       LPCSTR lpvalue,   const CURRENCYFMTA * lpFormat,
			       LPSTR lpCurrencyStr, int cchCurrency)
{
    UINT   nPosOrder, nNegOrder;
    INT    retVal;
    char   sDestination[128], sNegOrder[8], sPosOrder[8], sCurrencySymbol[8];
    char   *pDestination = sDestination;
    char   sNumberFormated[128];
    char   *pNumberFormated = sNumberFormated;
    LCID   systemDefaultLCID;
    BOOL   bIsPositive = FALSE, bValidFormat = FALSE;
    enum   Operations 
    {
        USE_PARAMETER,
        USE_LOCALEINFO,
        USE_SYSTEMDEFAULT,
        RETURN_ERROR
    } used_operation;

    NUMBERFMTA  numberFmt;

    /* Which source will we use to format the string */
    used_operation = RETURN_ERROR;
    if (lpFormat != NULL)
    {
        if (dwflags == 0)
            used_operation = USE_PARAMETER; 
    }
    else
    {
        if (dwflags & LOCALE_NOUSEROVERRIDE)
            used_operation = USE_LOCALEINFO;
        else
            used_operation = USE_SYSTEMDEFAULT;
    }

    /* Load the fields we need */
    switch(used_operation)
    {
    case USE_LOCALEINFO:        
        /* Specific to CURRENCYFMTA*/
        GetLocaleInfoA(locale, LOCALE_INEGCURR, sNegOrder, sizeof(sNegOrder));
        GetLocaleInfoA(locale, LOCALE_ICURRENCY, sPosOrder, sizeof(sPosOrder));
        GetLocaleInfoA(locale, LOCALE_SCURRENCY, sCurrencySymbol, sizeof(sCurrencySymbol));
        
        nPosOrder = atoi(sPosOrder);
        nNegOrder = atoi(sNegOrder);
        break;
    case USE_PARAMETER:        
        /* Specific to CURRENCYFMTA*/
        nNegOrder = lpFormat->NegativeOrder;
        nPosOrder = lpFormat->PositiveOrder;
        strcpy(sCurrencySymbol, lpFormat->lpCurrencySymbol);
        break;
    case USE_SYSTEMDEFAULT:
        systemDefaultLCID = GetSystemDefaultLCID();        
        /* Specific to CURRENCYFMTA*/
        GetLocaleInfoA(systemDefaultLCID, LOCALE_INEGCURR, sNegOrder, sizeof(sNegOrder));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_ICURRENCY, sPosOrder, sizeof(sPosOrder));
        GetLocaleInfoA(systemDefaultLCID, LOCALE_SCURRENCY, sCurrencySymbol, sizeof(sCurrencySymbol));
        
        nPosOrder = atoi(sPosOrder);
        nNegOrder = atoi(sNegOrder);
        break;
    default:
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    
    /* Construct a temporary number format structure */
    if (lpFormat != NULL)
    {
        numberFmt.NumDigits     = lpFormat->NumDigits;
        numberFmt.LeadingZero   = lpFormat->LeadingZero;
        numberFmt.Grouping      = lpFormat->Grouping;
        numberFmt.NegativeOrder = 0;
        numberFmt.lpDecimalSep = lpFormat->lpDecimalSep;
        numberFmt.lpThousandSep = lpFormat->lpThousandSep;
        bValidFormat = TRUE;
    }

    /* Make a call to GetNumberFormatA() */
    if (*lpvalue == '-')
    {
        bIsPositive = FALSE;
        retVal = GetNumberFormatA(locale,0,lpvalue+1,(bValidFormat)?&numberFmt:NULL,pNumberFormated,128);
    }
    else
    {
        bIsPositive = TRUE;
        retVal = GetNumberFormatA(locale,0,lpvalue,(bValidFormat)?&numberFmt:NULL,pNumberFormated,128);
    }
        
    if (retVal == 0)
        return 0;

    /* construct the formatted string */
    if (bIsPositive)
    {
        switch (nPosOrder)
        {
        case 0:   /* Prefix, no separation */
            strcpy (pDestination, sCurrencySymbol);
            strcat (pDestination, pNumberFormated);
            break;
        case 1:   /* Suffix, no separation */
            strcpy (pDestination, pNumberFormated);
            strcat (pDestination, sCurrencySymbol);
            break;
        case 2:   /* Prefix, 1 char separation */
            strcpy (pDestination, sCurrencySymbol);
            strcat (pDestination, " ");
            strcat (pDestination, pNumberFormated);
            break;
        case 3:   /* Suffix, 1 char separation */
            strcpy (pDestination, pNumberFormated);
            strcat (pDestination, " ");
            strcat (pDestination, sCurrencySymbol);
            break;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }
    else  /* negative number */
    {
        switch (nNegOrder)
        {
        case 0:   /* format: ($1.1) */
            strcpy (pDestination, "(");
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, ")");
            break;
        case 1:   /* format: -$1.1 */
            strcpy (pDestination, "-");
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, pNumberFormated);
            break;
        case 2:   /* format: $-1.1 */
            strcpy (pDestination, sCurrencySymbol);
            strcat (pDestination, "-");
            strcat (pDestination, pNumberFormated);
            break;
        case 3:   /* format: $1.1- */
            strcpy (pDestination, sCurrencySymbol);
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, "-");
            break;
        case 4:   /* format: (1.1$) */
            strcpy (pDestination, "(");
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, ")");
            break;
        case 5:   /* format: -1.1$ */
            strcpy (pDestination, "-");
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, sCurrencySymbol);
            break;
        case 6:   /* format: 1.1-$ */
            strcpy (pDestination, pNumberFormated);
            strcat (pDestination, "-");
            strcat (pDestination, sCurrencySymbol);
            break;
        case 7:   /* format: 1.1$- */
            strcpy (pDestination, pNumberFormated);
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, "-");
            break;
        case 8:   /* format: -1.1 $ */
            strcpy (pDestination, "-");
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, " ");
            strcat (pDestination, sCurrencySymbol);
            break;
        case 9:   /* format: -$ 1.1 */
            strcpy (pDestination, "-");
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, " ");
            strcat (pDestination, pNumberFormated);
            break;
        case 10:   /* format: 1.1 $- */
            strcpy (pDestination, pNumberFormated);
            strcat (pDestination, " ");
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, "-");
            break;
        case 11:   /* format: $ 1.1- */
            strcpy (pDestination, sCurrencySymbol);
            strcat (pDestination, " ");
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, "-");
            break;
        case 12:   /* format: $ -1.1 */
            strcpy (pDestination, sCurrencySymbol);
            strcat (pDestination, " ");
            strcat (pDestination, "-");
            strcat (pDestination, pNumberFormated);
            break;
        case 13:   /* format: 1.1- $ */
            strcpy (pDestination, pNumberFormated);
            strcat (pDestination, "-");
            strcat (pDestination, " ");
            strcat (pDestination, sCurrencySymbol);
            break;
        case 14:   /* format: ($ 1.1) */
            strcpy (pDestination, "("); 
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, " ");
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, ")");
            break;
        case 15:   /* format: (1.1 $) */
            strcpy (pDestination, "("); 
            strcat (pDestination, pNumberFormated);
            strcat (pDestination, " ");
            strcat (pDestination, sCurrencySymbol);
            strcat (pDestination, ")");
            break;
        default:
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }
    }

    if (cchCurrency == 0)
        return strlen(pDestination) + 1;

    else
    {
        strncpy (lpCurrencyStr, pDestination, cchCurrency-1);
        *(lpCurrencyStr+cchCurrency-1) = '\0';   /* ensure we got a NULL at the end */
        return  strlen(lpCurrencyStr);
    }    
}

/**************************************************************************
 *              GetCurrencyFormatW	(KERNEL32.303)
 */
INT WINAPI GetCurrencyFormatW(LCID locale, DWORD dwflags,
			       LPCWSTR lpvalue,   const CURRENCYFMTW * lpFormat,
			       LPWSTR lpCurrencyStr, int cchCurrency)
{
    FIXME("This API function is NOT implemented !\n");
    return 0;
}

/******************************************************************************
 *		OLE2NLS_CheckLocale	[intern]
 */ 
static LCID OLE2NLS_CheckLocale (LCID locale)
{
	if (!locale) 
	{ locale = LOCALE_SYSTEM_DEFAULT;
	}
  
	if (locale == LOCALE_SYSTEM_DEFAULT) 
  	{ return GetSystemDefaultLCID();
	} 
	else if (locale == LOCALE_USER_DEFAULT) 
	{ return GetUserDefaultLCID();
	}
	else
	{ return locale;
	}
}
/******************************************************************************
 *		GetTimeFormatA	[KERNEL32.422]
 * Makes an ASCII string of the time
 *
 * Formats date according to format,  or locale default if format is
 * NULL. The format consists of literal characters and fields as follows:
 *
 * h  hours with no leading zero (12-hour)
 * hh hours with full two digits
 * H  hours with no leading zero (24-hour)
 * HH hours with full two digits
 * m  minutes with no leading zero
 * mm minutes with full two digits
 * s  seconds with no leading zero
 * ss seconds with full two digits
 * t  time marker (A or P)
 * tt time marker (AM, PM)
 *
 */
INT WINAPI 
GetTimeFormatA(LCID locale,        /* in  */
		 DWORD flags,        /* in  */
		 LPSYSTEMTIME xtime, /* in  */ 
		 LPCSTR format,      /* in  */
		 LPSTR timestr,      /* out */
		 INT timelen       /* in  */) 
{ char format_buf[40];
  LPCSTR thisformat;
  SYSTEMTIME t;
  LPSYSTEMTIME thistime;
  LCID thislocale=0;
  DWORD thisflags=LOCALE_STIMEFORMAT; /* standart timeformat */
  INT ret;
    
  TRACE("GetTimeFormat(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",locale,flags,xtime,format,timestr,timelen);

  thislocale = OLE2NLS_CheckLocale ( locale );

  if ( flags & (TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT ))
  { FIXME("TIME_NOTIMEMARKER or TIME_FORCE24HOURFORMAT not implemented\n");
  }
  
  flags &= (TIME_NOSECONDS | TIME_NOMINUTESORSECONDS); /* mask for OLE_GetFormatA*/

  if (format == NULL) 
  { if (flags & LOCALE_NOUSEROVERRIDE)  /*use system default*/
    { thislocale = GetSystemDefaultLCID();
    }
    GetLocaleInfoA(thislocale, thisflags, format_buf, sizeof(format_buf));
    thisformat = format_buf;
  }
  else 
  { thisformat = format;
  }
  
  if (xtime == NULL) /* NULL means use the current local time*/
  { GetLocalTime(&t);
    thistime = &t;
  } 
  else
  { thistime = xtime;
  }
  ret = OLE_GetFormatA(thislocale, thisflags, flags, thistime, thisformat,
  			 timestr, timelen);
  return ret;
}


/******************************************************************************
 *		GetTimeFormatW	[KERNEL32.423]
 * Makes a Unicode string of the time
 */
INT WINAPI 
GetTimeFormatW(LCID locale,        /* in  */
		 DWORD flags,        /* in  */
		 LPSYSTEMTIME xtime, /* in  */ 
		 LPCWSTR format,     /* in  */
		 LPWSTR timestr,     /* out */
		 INT timelen       /* in  */) 
{	WCHAR format_buf[40];
	LPCWSTR thisformat;
	SYSTEMTIME t;
	LPSYSTEMTIME thistime;
	LCID thislocale=0;
	DWORD thisflags=LOCALE_STIMEFORMAT; /* standart timeformat */
	INT ret;
	    
	TRACE("GetTimeFormat(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",locale,flags,
	xtime,debugstr_w(format),timestr,timelen);

	thislocale = OLE2NLS_CheckLocale ( locale );

	if ( flags & (TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT ))
	{ FIXME("TIME_NOTIMEMARKER or TIME_FORCE24HOURFORMAT not implemented\n");
	}
  
	flags &= (TIME_NOSECONDS | TIME_NOMINUTESORSECONDS); /* mask for OLE_GetFormatA*/

	if (format == NULL) 
	{ if (flags & LOCALE_NOUSEROVERRIDE)  /*use system default*/
	  { thislocale = GetSystemDefaultLCID();
	  }
	  GetLocaleInfoW(thislocale, thisflags, format_buf, 40);
	  thisformat = format_buf;
	}	  
	else 
	{ thisformat = format;
	}
 
	if (xtime == NULL) /* NULL means use the current local time*/
	{ GetSystemTime(&t);
	  thistime = &t;
	} 
	else
	{ thistime = xtime;
	}

	ret = OLE_GetFormatW(thislocale, thisflags, flags, thistime, thisformat,
  			 timestr, timelen);
	return ret;
}

/******************************************************************************
 *		EnumCalendarInfoA	[KERNEL32.196]
 */
BOOL WINAPI EnumCalendarInfoA(
	CALINFO_ENUMPROCA calinfoproc,LCID locale,CALID calendar,CALTYPE caltype
) {
	FIXME("(%p,0x%04lx,0x%08lx,0x%08lx),stub!\n",calinfoproc,locale,calendar,caltype);
	return FALSE;
}
