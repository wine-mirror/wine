#ifndef __WINE_WINNLS_H
#define __WINE_WINNLS_H
#ifndef NONLS

#include "windef.h"
#ifdef __cplusplus
extern "C" {
#endif


/* Define a bunch of callback types */

#if defined(STRICT) || defined(__WINE__)
typedef BOOL    CALLBACK (*CALINFO_ENUMPROCA)(LPSTR);
typedef BOOL    CALLBACK (*CALINFO_ENUMPROCW)(LPWSTR);
typedef BOOL    CALLBACK (*CODEPAGE_ENUMPROCA)(LPSTR);
typedef BOOL    CALLBACK (*CODEPAGE_ENUMPROCW)(LPWSTR);
typedef BOOL    CALLBACK (*DATEFMT_ENUMPROCA)(LPSTR);
typedef BOOL    CALLBACK (*DATEFMT_ENUMPROCW)(LPWSTR);
typedef BOOL    CALLBACK (*LOCALE_ENUMPROCA)(LPSTR);
typedef BOOL    CALLBACK (*LOCALE_ENUMPROCW)(LPWSTR);
typedef BOOL    CALLBACK (*TIMEFMT_ENUMPROCA)(LPSTR);
typedef BOOL    CALLBACK (*TIMEFMT_ENUMPROCW)(LPWSTR);
#else
typedef FARPROC CALINFO_ENUMPROCA;
typedef FARPROC CALINFO_ENUMPROCW;
typedef FARPROC CODEPAGE_ENUMPROCA;
typedef FARPROC CODEPAGE_ENUMPROCW;
typedef FARPROC DATEFMT_ENUMPROCA;
typedef FARPROC DATEFMT_ENUMPROCW;
typedef FARPROC LOCALE_ENUMPROCA;
typedef FARPROC LOCALE_ENUMPROCW;
typedef FARPROC TIMEFMT_ENUMPROCA;
typedef FARPROC TIMEFMT_ENUMPROCW;
#endif /* STRICT || __WINE__ */

DECL_WINELIB_TYPE_AW(CALINFO_ENUMPROC)
DECL_WINELIB_TYPE_AW(CODEPAGE_ENUMPROC)
DECL_WINELIB_TYPE_AW(DATEFMT_ENUMPROC)
DECL_WINELIB_TYPE_AW(LOCALE_ENUMPROC)
DECL_WINELIB_TYPE_AW(TIMEFMT_ENUMPROC)


#define MB_PRECOMPOSED              0x00000001 
#define MB_COMPOSITE                0x00000002
#define MB_USEGLYPHCHARS            0x00000004
#define MB_ERR_INVALID_CHARS        0x00000008

#define LCID_INSTALLED              0x00000001 

/* flags to GetLocaleInfo */
#define	LOCALE_NOUSEROVERRIDE	    0x80000000
#define	LOCALE_USE_CP_ACP	    0x40000000

#define LOCALE_LOCALEINFOFLAGSMASK  0xC0000000

/* When adding new defines, don't forget to add an entry to the
 * locale2id map in misc/ole2nls.c
 */
#define LOCALE_ILANGUAGE            0x00000001   
#define LOCALE_SLANGUAGE            0x00000002   
#define LOCALE_SENGLANGUAGE         0x00001001   
#define LOCALE_SABBREVLANGNAME      0x00000003   
#define LOCALE_SNATIVELANGNAME      0x00000004   
#define LOCALE_ICOUNTRY             0x00000005   
#define LOCALE_SCOUNTRY             0x00000006   
#define LOCALE_SENGCOUNTRY          0x00001002   
#define LOCALE_SABBREVCTRYNAME      0x00000007   
#define LOCALE_SNATIVECTRYNAME      0x00000008   
#define LOCALE_IDEFAULTLANGUAGE     0x00000009   
#define LOCALE_IDEFAULTCOUNTRY      0x0000000A   
#define LOCALE_IDEFAULTCODEPAGE     0x0000000B   
#define LOCALE_IDEFAULTANSICODEPAGE 0x00001004   
#define LOCALE_IDEFAULTMACCODEPAGE  0x00001011
#define LOCALE_SLIST                0x0000000C   
#define LOCALE_IMEASURE             0x0000000D   
#define LOCALE_SDECIMAL             0x0000000E   
#define LOCALE_STHOUSAND            0x0000000F   
#define LOCALE_SGROUPING            0x00000010   
#define LOCALE_IDIGITS              0x00000011   
#define LOCALE_ILZERO               0x00000012   
#define LOCALE_INEGNUMBER           0x00001010   
#define LOCALE_SNATIVEDIGITS        0x00000013   
#define LOCALE_SCURRENCY            0x00000014   
#define LOCALE_SINTLSYMBOL          0x00000015   
#define LOCALE_SMONDECIMALSEP       0x00000016   
#define LOCALE_SMONTHOUSANDSEP      0x00000017   
#define LOCALE_SMONGROUPING         0x00000018   
#define LOCALE_ICURRDIGITS          0x00000019   
#define LOCALE_IINTLCURRDIGITS      0x0000001A   
#define LOCALE_ICURRENCY            0x0000001B   
#define LOCALE_INEGCURR             0x0000001C   
#define LOCALE_SDATE                0x0000001D   
#define LOCALE_STIME                0x0000001E   
#define LOCALE_SSHORTDATE           0x0000001F   
#define LOCALE_SLONGDATE            0x00000020   
#define LOCALE_STIMEFORMAT          0x00001003   
#define LOCALE_IDATE                0x00000021   
#define LOCALE_ILDATE               0x00000022   
#define LOCALE_ITIME                0x00000023   
#define LOCALE_ITIMEMARKPOSN        0x00001005   
#define LOCALE_ICENTURY             0x00000024   
#define LOCALE_ITLZERO              0x00000025   
#define LOCALE_IDAYLZERO            0x00000026   
#define LOCALE_IMONLZERO            0x00000027   
#define LOCALE_S1159                0x00000028   
#define LOCALE_S2359                0x00000029   
#define LOCALE_ICALENDARTYPE        0x00001009   
#define LOCALE_IOPTIONALCALENDAR    0x0000100B   
#define LOCALE_IFIRSTDAYOFWEEK      0x0000100C   
#define LOCALE_IFIRSTWEEKOFYEAR     0x0000100D   
#define LOCALE_SDAYNAME1            0x0000002A   
#define LOCALE_SDAYNAME2            0x0000002B   
#define LOCALE_SDAYNAME3            0x0000002C   
#define LOCALE_SDAYNAME4            0x0000002D   
#define LOCALE_SDAYNAME5            0x0000002E   
#define LOCALE_SDAYNAME6            0x0000002F   
#define LOCALE_SDAYNAME7            0x00000030   
#define LOCALE_SABBREVDAYNAME1      0x00000031   
#define LOCALE_SABBREVDAYNAME2      0x00000032   
#define LOCALE_SABBREVDAYNAME3      0x00000033   
#define LOCALE_SABBREVDAYNAME4      0x00000034   
#define LOCALE_SABBREVDAYNAME5      0x00000035   
#define LOCALE_SABBREVDAYNAME6      0x00000036   
#define LOCALE_SABBREVDAYNAME7      0x00000037   
#define LOCALE_SMONTHNAME1          0x00000038   
#define LOCALE_SMONTHNAME2          0x00000039   
#define LOCALE_SMONTHNAME3          0x0000003A   
#define LOCALE_SMONTHNAME4          0x0000003B   
#define LOCALE_SMONTHNAME5          0x0000003C   
#define LOCALE_SMONTHNAME6          0x0000003D   
#define LOCALE_SMONTHNAME7          0x0000003E   
#define LOCALE_SMONTHNAME8          0x0000003F   
#define LOCALE_SMONTHNAME9          0x00000040   
#define LOCALE_SMONTHNAME10         0x00000041   
#define LOCALE_SMONTHNAME11         0x00000042   
#define LOCALE_SMONTHNAME12         0x00000043   
#define LOCALE_SMONTHNAME13         0x0000100E   
#define LOCALE_SABBREVMONTHNAME1    0x00000044   
#define LOCALE_SABBREVMONTHNAME2    0x00000045   
#define LOCALE_SABBREVMONTHNAME3    0x00000046   
#define LOCALE_SABBREVMONTHNAME4    0x00000047   
#define LOCALE_SABBREVMONTHNAME5    0x00000048   
#define LOCALE_SABBREVMONTHNAME6    0x00000049   
#define LOCALE_SABBREVMONTHNAME7    0x0000004A   
#define LOCALE_SABBREVMONTHNAME8    0x0000004B   
#define LOCALE_SABBREVMONTHNAME9    0x0000004C   
#define LOCALE_SABBREVMONTHNAME10   0x0000004D   
#define LOCALE_SABBREVMONTHNAME11   0x0000004E   
#define LOCALE_SABBREVMONTHNAME12   0x0000004F   
#define LOCALE_SABBREVMONTHNAME13   0x0000100F   
#define LOCALE_SPOSITIVESIGN        0x00000050   
#define LOCALE_SNEGATIVESIGN        0x00000051   
#define LOCALE_IPOSSIGNPOSN         0x00000052   
#define LOCALE_INEGSIGNPOSN         0x00000053   
#define LOCALE_IPOSSYMPRECEDES      0x00000054   
#define LOCALE_IPOSSEPBYSPACE       0x00000055   
#define LOCALE_INEGSYMPRECEDES      0x00000056   
#define LOCALE_INEGSEPBYSPACE       0x00000057   
#define	LOCALE_FONTSIGNATURE        0x00000058
#define LOCALE_SISO639LANGNAME      0x00000059
#define LOCALE_SISO3166CTRYNAME     0x0000005A

#define LOCALE_IDEFAULTEBCDICCODEPAGE 0x00001012
#define LOCALE_IPAPERSIZE             0x0000100A
#define LOCALE_SENGCURRNAME           0x00001007
#define LOCALE_SNATIVECURRNAME        0x00001008
#define LOCALE_SYEARMONTH             0x00001006
#define LOCALE_SSORTNAME              0x00001013
#define LOCALE_IDIGITSUBSTITUTION     0x00001014

#define NORM_IGNORECASE				1
#define NORM_IGNORENONSPACE			2
#define NORM_IGNORESYMBOLS			4
#define NORM_STRINGSORT				0x1000
#define NORM_IGNOREKANATYPE                     0x00010000
#define NORM_IGNOREWIDTH                        0x00020000

#define CP_ACP					0
#define CP_OEMCP				1
#define CP_MACCP				2
#define CP_THREAD_ACP				3
#define CP_SYMBOL				42
#define CP_UTF7					65000
#define CP_UTF8					65001

#define WC_DISCARDNS                0x00000010
#define WC_SEPCHARS                 0x00000020
#define WC_DEFAULTCHAR              0x00000040
#define WC_COMPOSITECHECK           0x00000200
#define WC_NO_BEST_FIT_CHARS        0x00000400


/* Locale Dependent Mapping Flags */
#define LCMAP_LOWERCASE	0x00000100	/* lower case letters */
#define LCMAP_UPPERCASE	0x00000200	/* upper case letters */
#define LCMAP_SORTKEY	0x00000400	/* WC sort key (normalize) */
#define LCMAP_BYTEREV	0x00000800	/* byte reversal */

#define SORT_STRINGSORT 0x00001000      /* take punctuation into account */

#define LCMAP_HIRAGANA	0x00100000	/* map katakana to hiragana */
#define LCMAP_KATAKANA	0x00200000	/* map hiragana to katakana */
#define LCMAP_HALFWIDTH	0x00400000	/* map double byte to single byte */
#define LCMAP_FULLWIDTH	0x00800000	/* map single byte to double byte */

/* Date Flags for GetDateFormat. */

#define DATE_SHORTDATE         0x00000001  /* use short date picture */
#define DATE_LONGDATE          0x00000002  /* use long date picture */
#define DATE_USE_ALT_CALENDAR  0x00000004  /* use alternate calendar */
                          /* alt. calendar support is broken anyway */

#define TIME_FORCE24HOURFORMAT 0x00000008  /* force 24 hour format*/
#define TIME_NOTIMEMARKER      0x00000004  /* show no AM/PM */
#define TIME_NOSECONDS         0x00000002  /* show no seconds */
#define TIME_NOMINUTESORSECONDS 0x0000001  /* show no minutes either */

/* internal flags for GetDateFormat system */
#define DATE_DATEVARSONLY      0x00000100  /* only date stuff: yMdg */
#define TIME_TIMEVARSONLY      0x00000200  /* only time stuff: hHmst */
/* use this in a Winelib program if you really want all types */
#define LOCALE_TIMEDATEBOTH    0x00000300  /* full set */

/* Tests that we currently implement */
#define ITU_IMPLEMENTED_TESTS \
	IS_TEXT_UNICODE_SIGNATURE| \
	IS_TEXT_UNICODE_ODD_LENGTH

        
/* Character Type Flags */
#define	CT_CTYPE1		0x00000001	/* usual ctype */
#define	CT_CTYPE2		0x00000002	/* bidirectional layout info */
#define	CT_CTYPE3		0x00000004	/* textprocessing info */

/* CType 1 Flag Bits */
#define C1_UPPER		0x0001
#define C1_LOWER		0x0002
#define C1_DIGIT		0x0004
#define C1_SPACE		0x0008
#define C1_PUNCT		0x0010
#define C1_CNTRL		0x0020
#define C1_BLANK		0x0040
#define C1_XDIGIT		0x0080
#define C1_ALPHA		0x0100

/* CType 2 Flag Bits */
#define	C2_LEFTTORIGHT		0x0001
#define	C2_RIGHTTOLEFT		0x0002
#define	C2_EUROPENUMBER		0x0003
#define	C2_EUROPESEPARATOR	0x0004
#define	C2_EUROPETERMINATOR	0x0005
#define	C2_ARABICNUMBER		0x0006
#define	C2_COMMONSEPARATOR	0x0007
#define	C2_BLOCKSEPARATOR	0x0008
#define	C2_SEGMENTSEPARATOR	0x0009
#define	C2_WHITESPACE		0x000A
#define	C2_OTHERNEUTRAL		0x000B
#define	C2_NOTAPPLICABLE	0x0000

/* CType 3 Flag Bits */
#define	C3_NONSPACING		0x0001
#define	C3_DIACRITIC		0x0002
#define	C3_VOWELMARK		0x0004
#define	C3_SYMBOL		0x0008
#define	C3_KATAKANA		0x0010
#define	C3_HIRAGANA		0x0020
#define	C3_HALFWIDTH		0x0040
#define	C3_FULLWIDTH		0x0080
#define	C3_IDEOGRAPH		0x0100
#define	C3_KASHIDA		0x0200
#define	C3_LEXICAL		0x0400
#define	C3_ALPHA		0x8000
#define	C3_NOTAPPLICABLE	0x0000

/* Code page information.
 */
#define MAX_LEADBYTES     12
#define MAX_DEFAULTCHAR   2


/* Types
 */

typedef DWORD LCTYPE;
typedef DWORD CALTYPE;
typedef DWORD CALID;

typedef struct
{
    UINT MaxCharSize;
    BYTE   DefaultChar[MAX_DEFAULTCHAR];
    BYTE   LeadByte[MAX_LEADBYTES];
} CPINFO, *LPCPINFO;

typedef struct
{
    UINT MaxCharSize;
    BYTE DefaultChar[MAX_DEFAULTCHAR];
    BYTE LeadByte[MAX_LEADBYTES];
    WCHAR UnicodeDefaultChar;
    UINT CodePage;
    CHAR CodePageName[MAX_PATH];
} CPINFOEXA, *LPCPINFOEXA;

typedef struct
{
    UINT MaxCharSize;
    BYTE DefaultChar[MAX_DEFAULTCHAR];
    BYTE LeadByte[MAX_LEADBYTES];
    WCHAR UnicodeDefaultChar;
    UINT CodePage;
    WCHAR CodePageName[MAX_PATH];
} CPINFOEXW, *LPCPINFOEXW;

DECL_WINELIB_TYPE_AW(CPINFOEX)
DECL_WINELIB_TYPE_AW(LPCPINFOEX)

typedef struct _numberfmtA {
    UINT NumDigits;
    UINT LeadingZero;
    UINT Grouping;
    LPCSTR lpDecimalSep;
    LPCSTR lpThousandSep;
    UINT NegativeOrder;
} NUMBERFMTA, *LPNUMBERFMTA;

typedef struct _numberfmtW {
    UINT NumDigits;
    UINT LeadingZero;
    UINT Grouping;
    LPCWSTR lpDecimalSep;
    LPCWSTR lpThousandSep;
    UINT NegativeOrder;
} NUMBERFMTW, *LPNUMBERFMTW;

DECL_WINELIB_TYPE_AW(NUMBERFMT)
DECL_WINELIB_TYPE_AW(LPNUMBERFMT)

typedef struct _currencyfmtA
{   
	UINT      NumDigits;   
	UINT      LeadingZero; 
	UINT      Grouping;   
	LPCSTR    lpDecimalSep;   
	LPCSTR    lpThousandSep; 
	UINT      NegativeOrder;   
	UINT      PositiveOrder; 
	LPCSTR    lpCurrencySymbol;
} CURRENCYFMTA, *LPCURRENCYFMTA; 

typedef struct _currencyfmtW
{   
	UINT      NumDigits;   
	UINT      LeadingZero; 
	UINT      Grouping;   
	LPCWSTR   lpDecimalSep;   
	LPCWSTR   lpThousandSep; 
	UINT      NegativeOrder;   
	UINT      PositiveOrder; 
	LPCWSTR   lpCurrencySymbol;
} CURRENCYFMTW, *LPCURRENCYFMTW; 

DECL_WINELIB_TYPE_AW(CURRENCYFMT)
DECL_WINELIB_TYPE_AW(LPCURRENCYFMT)


/* APIs
 */

LCID        WINAPI ConvertDefaultLocale(LCID);
BOOL        WINAPI EnumCalendarInfoA(CALINFO_ENUMPROCA,LCID,CALID,CALTYPE);
BOOL        WINAPI EnumCalendarInfoW(CALINFO_ENUMPROCW,LCID,CALID,CALTYPE);
#define     EnumCalendarInfo WINELIB_NAME_AW(EnumCalendarInfo)
BOOL        WINAPI EnumDateFormatsA(DATEFMT_ENUMPROCA,LCID,DWORD);
BOOL        WINAPI EnumDateFormatsW(DATEFMT_ENUMPROCW,LCID,DWORD);
#define     EnumDateFormats WINELIB_NAME_AW(EnumDateFormats)
BOOL        WINAPI EnumSystemCodePagesA(CODEPAGE_ENUMPROCA,DWORD);
BOOL        WINAPI EnumSystemCodePagesW(CODEPAGE_ENUMPROCW,DWORD);
#define     EnumSystemCodePages WINELIB_NAME_AW(EnumSystemCodePages)
BOOL        WINAPI EnumSystemLocalesA(LOCALE_ENUMPROCA,DWORD);
BOOL        WINAPI EnumSystemLocalesW(LOCALE_ENUMPROCW,DWORD);
#define     EnumSystemLocales WINELIB_NAME_AW(EnumSystemLocales)
BOOL        WINAPI EnumTimeFormatsA(TIMEFMT_ENUMPROCA,LCID,DWORD);
BOOL        WINAPI EnumTimeFormatsW(TIMEFMT_ENUMPROCW,LCID,DWORD);
#define     EnumTimeFormats WINELIB_NAME_AW(EnumTimeFormats)
BOOL        WINAPI GetCPInfo(UINT,LPCPINFO);
BOOL        WINAPI GetCPInfoExA(UINT,DWORD,LPCPINFOEXA);
BOOL        WINAPI GetCPInfoExW(UINT,DWORD,LPCPINFOEXW);
#define     GetCPInfoEx WINELIB_NAME_AW(GetCPInfoEx)
INT         WINAPI GetLocaleInfoA(LCID,LCTYPE,LPSTR,INT);
INT         WINAPI GetLocaleInfoW(LCID,LCTYPE,LPWSTR,INT);
#define     GetLocaleInfo WINELIB_NAME_AW(GetLocaleInfo)
INT         WINAPI GetNumberFormatA(LCID,DWORD,LPCSTR,const NUMBERFMTA*,LPSTR,int);
INT         WINAPI GetNumberFormatW(LCID,DWORD,LPCWSTR,const NUMBERFMTW*,LPWSTR,int);
#define     GetNumberFormat WINELIB_NAME_AW(GetNumberFormat)
BOOL        WINAPI IsValidCodePage(UINT);
INT         WINAPI LCMapStringA(LCID,DWORD,LPCSTR,INT,LPSTR,INT);
INT         WINAPI LCMapStringW(LCID,DWORD,LPCWSTR,INT,LPWSTR,INT);
#define     LCMapString WINELIB_NAME_AW(LCMapString)

#ifdef __cplusplus
}
#endif

#endif /* !NONLS */
#endif  /* __WINE_WINNLS_H */
