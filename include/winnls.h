#ifndef __WINE_WINNLS_H
#define __WINE_WINNLS_H

/* flags to GetLocaleInfo */
#define	LOCALE_NOUSEROVERRIDE	    0x80000000
#define	LOCALE_USE_CP_ACP	    0x40000000

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


#define NORM_IGNORECASE				1
#define NORM_IGNORENONSPACE			2
#define NORM_IGNORESYMBOLS			4
#define NORM_STRINGSORT				0x1000

#define CP_ACP						0
#define CP_OEMCP					1

#define WC_DEFAULTCHECK				0x00000100
#define WC_COMPOSITECHECK			0x00000200
#define WC_DISCARDNS				0x00000010
#define WC_SEPCHARS					0x00000020
#define WC_DEFAULTCHAR				0x00000040

#define MAKELCID(l, s)    (MAKELONG(l, s))
#define MAKELANGID(p, s)  ((((WORD)(s))<<10) | (WORD)(p))

/* Language IDs (were in winnt.h,  for some reason) */


/* Language IDs */

#define LANG_NEUTRAL                     0x00
#define LANG_ARABIC                      0x01
#define LANG_AFRIKAANS                   0x36
#define LANG_ALBANIAN                    0x1c
#define LANG_BASQUE                      0x2d
#define LANG_BULGARIAN                   0x02
#define LANG_BYELORUSSIAN                0x23
#define LANG_CATALAN                     0x03
#define LANG_CHINESE                     0x04
#define LANG_CROATIAN                    0x1a
#define LANG_CZECH                       0x05
#define LANG_DANISH                      0x06
#define LANG_DUTCH                       0x13
#define LANG_ENGLISH                     0x09
#define LANG_ESTONIAN                    0x25
#define LANG_FAEROESE                    0x38
#define LANG_FARSI                       0x29
#define LANG_FINNISH                     0x0b
#define LANG_FRENCH                      0x0c
#define LANG_GERMAN                      0x07
#define LANG_GREEK                       0x08
#define LANG_HEBREW                      0x0D
#define LANG_HUNGARIAN                   0x0e
#define LANG_ICELANDIC                   0x0f
#define LANG_INDONESIAN                  0x21
#define LANG_ITALIAN                     0x10
#define LANG_JAPANESE                    0x11
#define LANG_KOREAN                      0x12
#define LANG_LATVIAN                     0x26
#define LANG_LITHUANIAN                  0x27
#define LANG_NORWEGIAN                   0x14
#define LANG_POLISH                      0x15
#define LANG_PORTUGUESE                  0x16
#define LANG_ROMANIAN                    0x18
#define LANG_RUSSIAN                     0x19
#define LANG_SLOVAK                      0x1b
#define LANG_SLOVENIAN                   0x24
#define LANG_SORBIAN                     0x2e
#define LANG_SPANISH                     0x0a
#define LANG_SWEDISH                     0x1d
#define LANG_THAI                        0x1e
#define LANG_TURKISH                     0x1f
#define LANG_UKRAINIAN                   0x22

/* "default"ish values */

#define SORT_DEFAULT      0
#define SUBLANG_DEFAULT   1
#define SUBLANG_SYS_DEFAULT 1 /* FIXME: I don't know if this is right */


#define LANG_SYSTEM_DEFAULT   (MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT))
#define LANG_USER_DEFAULT     (MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))
#define LOCALE_SYSTEM_DEFAULT (MAKELCID(LANG_SYSTEM_DEFAULT, SORT_DEFAULT))
#define LOCALE_USER_DEFAULT   (MAKELCID(LANG_USER_DEFAULT, SORT_DEFAULT)) 

/* Locale Dependent Mapping Flags */
#define LCMAP_LOWERCASE	0x00000100	/* lower case letters */
#define LCMAP_UPPERCASE	0x00000200	/* upper case letters */
#define LCMAP_SORTKEY	0x00000400	/* WC sort key (normalize) */
#define LCMAP_BYTEREV	0x00000800	/* byte reversal */

#define LCMAP_HIRAGANA	0x00100000	/* map katakana to hiragana */
#define LCMAP_KATAKANA	0x00200000	/* map hiragana to katakana */
#define LCMAP_HALFWIDTH	0x00400000	/* map double byte to single byte */
#define LCMAP_FULLWIDTH	0x00800000	/* map single byte to double byte */

/* Date Flags for GetDateFormat. */

#define DATE_SHORTDATE         0x00000001  /* use short date picture */
#define DATE_LONGDATE          0x00000002  /* use long date picture */
#define DATE_USE_ALT_CALENDAR  0x00000004  /* use alternate calendar */
                          /* alt. calendar support is broken anyway */


#endif  /* __WINE_WINNLS_H */
