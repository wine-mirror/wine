/*
 * Locale support
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
 * Copyright 2002, 2019 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#define WINNORMALIZEAPI
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winnls.h"
#include "winuser.h"
#include "winternl.h"
#include "kernelbase.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

#define CALINFO_MAX_YEAR 2029

extern const unsigned int collation_table[] DECLSPEC_HIDDEN;

static HANDLE kernel32_handle;

static const struct registry_value
{
    DWORD           lctype;
    const WCHAR    *name;
} registry_values[] =
{
    { LOCALE_ICALENDARTYPE, L"iCalendarType" },
    { LOCALE_ICURRDIGITS, L"iCurrDigits" },
    { LOCALE_ICURRENCY, L"iCurrency" },
    { LOCALE_IDIGITS, L"iDigits" },
    { LOCALE_IFIRSTDAYOFWEEK, L"iFirstDayOfWeek" },
    { LOCALE_IFIRSTWEEKOFYEAR, L"iFirstWeekOfYear" },
    { LOCALE_ILZERO, L"iLZero" },
    { LOCALE_IMEASURE, L"iMeasure" },
    { LOCALE_INEGCURR, L"iNegCurr" },
    { LOCALE_INEGNUMBER, L"iNegNumber" },
    { LOCALE_IPAPERSIZE, L"iPaperSize" },
    { LOCALE_ITIME, L"iTime" },
    { LOCALE_S1159, L"s1159" },
    { LOCALE_S2359, L"s2359" },
    { LOCALE_SCURRENCY, L"sCurrency" },
    { LOCALE_SDATE, L"sDate" },
    { LOCALE_SDECIMAL, L"sDecimal" },
    { LOCALE_SGROUPING, L"sGrouping" },
    { LOCALE_SLIST, L"sList" },
    { LOCALE_SLONGDATE, L"sLongDate" },
    { LOCALE_SMONDECIMALSEP, L"sMonDecimalSep" },
    { LOCALE_SMONGROUPING, L"sMonGrouping" },
    { LOCALE_SMONTHOUSANDSEP, L"sMonThousandSep" },
    { LOCALE_SNEGATIVESIGN, L"sNegativeSign" },
    { LOCALE_SPOSITIVESIGN, L"sPositiveSign" },
    { LOCALE_SSHORTDATE, L"sShortDate" },
    { LOCALE_STHOUSAND, L"sThousand" },
    { LOCALE_STIME, L"sTime" },
    { LOCALE_STIMEFORMAT, L"sTimeFormat" },
    { LOCALE_SYEARMONTH, L"sYearMonth" },
    /* The following are not listed under MSDN as supported,
     * but seem to be used and also stored in the registry.
     */
    { LOCALE_SNAME, L"LocaleName" },
    { LOCALE_ICOUNTRY, L"iCountry" },
    { LOCALE_IDATE, L"iDate" },
    { LOCALE_ILDATE, L"iLDate" },
    { LOCALE_ITLZERO, L"iTLZero" },
    { LOCALE_SCOUNTRY, L"sCountry" },
    { LOCALE_SABBREVLANGNAME, L"sLanguage" },
    { LOCALE_IDIGITSUBSTITUTION, L"Numshape" },
    { LOCALE_SNATIVEDIGITS, L"sNativeDigits" },
    { LOCALE_ITIMEMARKPOSN, L"iTimePrefix" },
};

static WCHAR *registry_cache[ARRAY_SIZE(registry_values)];

static const struct { UINT cp; const WCHAR *name; } codepage_names[] =
{
    { 37,    L"IBM EBCDIC US Canada" },
    { 424,   L"IBM EBCDIC Hebrew" },
    { 437,   L"OEM United States" },
    { 500,   L"IBM EBCDIC International" },
    { 708,   L"Arabic ASMO" },
    { 720,   L"Arabic (Transparent ASMO)" },
    { 737,   L"OEM Greek 437G" },
    { 775,   L"OEM Baltic" },
    { 850,   L"OEM Multilingual Latin 1" },
    { 852,   L"OEM Slovak Latin 2" },
    { 855,   L"OEM Cyrillic" },
    { 856,   L"Hebrew PC" },
    { 857,   L"OEM Turkish" },
    { 860,   L"OEM Portuguese" },
    { 861,   L"OEM Icelandic" },
    { 862,   L"OEM Hebrew" },
    { 863,   L"OEM Canadian French" },
    { 864,   L"OEM Arabic" },
    { 865,   L"OEM Nordic" },
    { 866,   L"OEM Russian" },
    { 869,   L"OEM Greek" },
    { 874,   L"ANSI/OEM Thai" },
    { 875,   L"IBM EBCDIC Greek" },
    { 878,   L"Russian KOI8" },
    { 932,   L"ANSI/OEM Japanese Shift-JIS" },
    { 936,   L"ANSI/OEM Simplified Chinese GBK" },
    { 949,   L"ANSI/OEM Korean Unified Hangul" },
    { 950,   L"ANSI/OEM Traditional Chinese Big5" },
    { 1006,  L"IBM Arabic" },
    { 1026,  L"IBM EBCDIC Latin 5 Turkish" },
    { 1250,  L"ANSI Eastern Europe" },
    { 1251,  L"ANSI Cyrillic" },
    { 1252,  L"ANSI Latin 1" },
    { 1253,  L"ANSI Greek" },
    { 1254,  L"ANSI Turkish" },
    { 1255,  L"ANSI Hebrew" },
    { 1256,  L"ANSI Arabic" },
    { 1257,  L"ANSI Baltic" },
    { 1258,  L"ANSI/OEM Viet Nam" },
    { 1361,  L"Korean Johab" },
    { 10000, L"Mac Roman" },
    { 10001, L"Mac Japanese" },
    { 10002, L"Mac Traditional Chinese" },
    { 10003, L"Mac Korean" },
    { 10004, L"Mac Arabic" },
    { 10005, L"Mac Hebrew" },
    { 10006, L"Mac Greek" },
    { 10007, L"Mac Cyrillic" },
    { 10008, L"Mac Simplified Chinese" },
    { 10010, L"Mac Romanian" },
    { 10017, L"Mac Ukrainian" },
    { 10021, L"Mac Thai" },
    { 10029, L"Mac Latin 2" },
    { 10079, L"Mac Icelandic" },
    { 10081, L"Mac Turkish" },
    { 10082, L"Mac Croatian" },
    { 20127, L"US-ASCII (7bit)" },
    { 20866, L"Russian KOI8" },
    { 20932, L"EUC-JP" },
    { 20949, L"Korean Wansung" },
    { 21866, L"Ukrainian KOI8" },
    { 28591, L"ISO 8859-1 Latin 1" },
    { 28592, L"ISO 8859-2 Latin 2 (East European)" },
    { 28593, L"ISO 8859-3 Latin 3 (South European)" },
    { 28594, L"ISO 8859-4 Latin 4 (Baltic old)" },
    { 28595, L"ISO 8859-5 Cyrillic" },
    { 28596, L"ISO 8859-6 Arabic" },
    { 28597, L"ISO 8859-7 Greek" },
    { 28598, L"ISO 8859-8 Hebrew" },
    { 28599, L"ISO 8859-9 Latin 5 (Turkish)" },
    { 28600, L"ISO 8859-10 Latin 6 (Nordic)" },
    { 28601, L"ISO 8859-11 Latin (Thai)" },
    { 28603, L"ISO 8859-13 Latin 7 (Baltic)" },
    { 28604, L"ISO 8859-14 Latin 8 (Celtic)" },
    { 28605, L"ISO 8859-15 Latin 9 (Euro)" },
    { 28606, L"ISO 8859-16 Latin 10 (Balkan)" },
    { 65000, L"Unicode (UTF-7)" },
    { 65001, L"Unicode (UTF-8)" }
};

/* Unicode expanded ligatures */
static const WCHAR ligatures[][5] =
{
    { 0x00c6,  'A','E',0 },
    { 0x00de,  'T','H',0 },
    { 0x00df,  's','s',0 },
    { 0x00e6,  'a','e',0 },
    { 0x00fe,  't','h',0 },
    { 0x0132,  'I','J',0 },
    { 0x0133,  'i','j',0 },
    { 0x0152,  'O','E',0 },
    { 0x0153,  'o','e',0 },
    { 0x01c4,  'D',0x017d,0 },
    { 0x01c5,  'D',0x017e,0 },
    { 0x01c6,  'd',0x017e,0 },
    { 0x01c7,  'L','J',0 },
    { 0x01c8,  'L','j',0 },
    { 0x01c9,  'l','j',0 },
    { 0x01ca,  'N','J',0 },
    { 0x01cb,  'N','j',0 },
    { 0x01cc,  'n','j',0 },
    { 0x01e2,  0x0100,0x0112,0 },
    { 0x01e3,  0x0101,0x0113,0 },
    { 0x01f1,  'D','Z',0 },
    { 0x01f2,  'D','z',0 },
    { 0x01f3,  'd','z',0 },
    { 0x01fc,  0x00c1,0x00c9,0 },
    { 0x01fd,  0x00e1,0x00e9,0 },
    { 0x05f0,  0x05d5,0x05d5,0 },
    { 0x05f1,  0x05d5,0x05d9,0 },
    { 0x05f2,  0x05d9,0x05d9,0 },
    { 0xfb00,  'f','f',0 },
    { 0xfb01,  'f','i',0 },
    { 0xfb02,  'f','l',0 },
    { 0xfb03,  'f','f','i',0 },
    { 0xfb04,  'f','f','l',0 },
    { 0xfb05,  0x017f,'t',0 },
    { 0xfb06,  's','t',0 },
};

enum locationkind { LOCATION_NATION = 0, LOCATION_REGION, LOCATION_BOTH };

struct geoinfo
{
    GEOID id;
    WCHAR iso2W[3];
    WCHAR iso3W[4];
    GEOID parent;
    int   uncode;
    enum locationkind kind;
};

static const struct geoinfo geoinfodata[] =
{
    { 2, L"AG", L"ATG", 10039880,  28 }, /* Antigua and Barbuda */
    { 3, L"AF", L"AFG", 47614,   4 }, /* Afghanistan */
    { 4, L"DZ", L"DZA", 42487,  12 }, /* Algeria */
    { 5, L"AZ", L"AZE", 47611,  31 }, /* Azerbaijan */
    { 6, L"AL", L"ALB", 47610,   8 }, /* Albania */
    { 7, L"AM", L"ARM", 47611,  51 }, /* Armenia */
    { 8, L"AD", L"AND", 47610,  20 }, /* Andorra */
    { 9, L"AO", L"AGO", 42484,  24 }, /* Angola */
    { 10, L"AS", L"ASM", 26286,  16 }, /* American Samoa */
    { 11, L"AR", L"ARG", 31396,  32 }, /* Argentina */
    { 12, L"AU", L"AUS", 10210825,  36 }, /* Australia */
    { 14, L"AT", L"AUT", 10210824,  40 }, /* Austria */
    { 17, L"BH", L"BHR", 47611,  48 }, /* Bahrain */
    { 18, L"BB", L"BRB", 10039880,  52 }, /* Barbados */
    { 19, L"BW", L"BWA", 10039883,  72 }, /* Botswana */
    { 20, L"BM", L"BMU", 23581,  60 }, /* Bermuda */
    { 21, L"BE", L"BEL", 10210824,  56 }, /* Belgium */
    { 22, L"BS", L"BHS", 10039880,  44 }, /* Bahamas, The */
    { 23, L"BD", L"BGD", 47614,  50 }, /* Bangladesh */
    { 24, L"BZ", L"BLZ", 27082,  84 }, /* Belize */
    { 25, L"BA", L"BIH", 47610,  70 }, /* Bosnia and Herzegovina */
    { 26, L"BO", L"BOL", 31396,  68 }, /* Bolivia */
    { 27, L"MM", L"MMR", 47599, 104 }, /* Myanmar */
    { 28, L"BJ", L"BEN", 42483, 204 }, /* Benin */
    { 29, L"BY", L"BLR", 47609, 112 }, /* Belarus */
    { 30, L"SB", L"SLB", 20900,  90 }, /* Solomon Islands */
    { 32, L"BR", L"BRA", 31396,  76 }, /* Brazil */
    { 34, L"BT", L"BTN", 47614,  64 }, /* Bhutan */
    { 35, L"BG", L"BGR", 47609, 100 }, /* Bulgaria */
    { 37, L"BN", L"BRN", 47599,  96 }, /* Brunei */
    { 38, L"BI", L"BDI", 47603, 108 }, /* Burundi */
    { 39, L"CA", L"CAN", 23581, 124 }, /* Canada */
    { 40, L"KH", L"KHM", 47599, 116 }, /* Cambodia */
    { 41, L"TD", L"TCD", 42484, 148 }, /* Chad */
    { 42, L"LK", L"LKA", 47614, 144 }, /* Sri Lanka */
    { 43, L"CG", L"COG", 42484, 178 }, /* Congo */
    { 44, L"CD", L"COD", 42484, 180 }, /* Congo (DRC) */
    { 45, L"CN", L"CHN", 47600, 156 }, /* China */
    { 46, L"CL", L"CHL", 31396, 152 }, /* Chile */
    { 49, L"CM", L"CMR", 42484, 120 }, /* Cameroon */
    { 50, L"KM", L"COM", 47603, 174 }, /* Comoros */
    { 51, L"CO", L"COL", 31396, 170 }, /* Colombia */
    { 54, L"CR", L"CRI", 27082, 188 }, /* Costa Rica */
    { 55, L"CF", L"CAF", 42484, 140 }, /* Central African Republic */
    { 56, L"CU", L"CUB", 10039880, 192 }, /* Cuba */
    { 57, L"CV", L"CPV", 42483, 132 }, /* Cape Verde */
    { 59, L"CY", L"CYP", 47611, 196 }, /* Cyprus */
    { 61, L"DK", L"DNK", 10039882, 208 }, /* Denmark */
    { 62, L"DJ", L"DJI", 47603, 262 }, /* Djibouti */
    { 63, L"DM", L"DMA", 10039880, 212 }, /* Dominica */
    { 65, L"DO", L"DOM", 10039880, 214 }, /* Dominican Republic */
    { 66, L"EC", L"ECU", 31396, 218 }, /* Ecuador */
    { 67, L"EG", L"EGY", 42487, 818 }, /* Egypt */
    { 68, L"IE", L"IRL", 10039882, 372 }, /* Ireland */
    { 69, L"GQ", L"GNQ", 42484, 226 }, /* Equatorial Guinea */
    { 70, L"EE", L"EST", 10039882, 233 }, /* Estonia */
    { 71, L"ER", L"ERI", 47603, 232 }, /* Eritrea */
    { 72, L"SV", L"SLV", 27082, 222 }, /* El Salvador */
    { 73, L"ET", L"ETH", 47603, 231 }, /* Ethiopia */
    { 75, L"CZ", L"CZE", 47609, 203 }, /* Czech Republic */
    { 77, L"FI", L"FIN", 10039882, 246 }, /* Finland */
    { 78, L"FJ", L"FJI", 20900, 242 }, /* Fiji Islands */
    { 80, L"FM", L"FSM", 21206, 583 }, /* Micronesia */
    { 81, L"FO", L"FRO", 10039882, 234 }, /* Faroe Islands */
    { 84, L"FR", L"FRA", 10210824, 250 }, /* France */
    { 86, L"GM", L"GMB", 42483, 270 }, /* Gambia, The */
    { 87, L"GA", L"GAB", 42484, 266 }, /* Gabon */
    { 88, L"GE", L"GEO", 47611, 268 }, /* Georgia */
    { 89, L"GH", L"GHA", 42483, 288 }, /* Ghana */
    { 90, L"GI", L"GIB", 47610, 292 }, /* Gibraltar */
    { 91, L"GD", L"GRD", 10039880, 308 }, /* Grenada */
    { 93, L"GL", L"GRL", 23581, 304 }, /* Greenland */
    { 94, L"DE", L"DEU", 10210824, 276 }, /* Germany */
    { 98, L"GR", L"GRC", 47610, 300 }, /* Greece */
    { 99, L"GT", L"GTM", 27082, 320 }, /* Guatemala */
    { 100, L"GN", L"GIN", 42483, 324 }, /* Guinea */
    { 101, L"GY", L"GUY", 31396, 328 }, /* Guyana */
    { 103, L"HT", L"HTI", 10039880, 332 }, /* Haiti */
    { 104, L"HK", L"HKG", 47600, 344 }, /* Hong Kong S.A.R. */
    { 106, L"HN", L"HND", 27082, 340 }, /* Honduras */
    { 108, L"HR", L"HRV", 47610, 191 }, /* Croatia */
    { 109, L"HU", L"HUN", 47609, 348 }, /* Hungary */
    { 110, L"IS", L"ISL", 10039882, 352 }, /* Iceland */
    { 111, L"ID", L"IDN", 47599, 360 }, /* Indonesia */
    { 113, L"IN", L"IND", 47614, 356 }, /* India */
    { 114, L"IO", L"IOT", 39070,  86 }, /* British Indian Ocean Territory */
    { 116, L"IR", L"IRN", 47614, 364 }, /* Iran */
    { 117, L"IL", L"ISR", 47611, 376 }, /* Israel */
    { 118, L"IT", L"ITA", 47610, 380 }, /* Italy */
    { 119, L"CI", L"CIV", 42483, 384 }, /* Côte d'Ivoire */
    { 121, L"IQ", L"IRQ", 47611, 368 }, /* Iraq */
    { 122, L"JP", L"JPN", 47600, 392 }, /* Japan */
    { 124, L"JM", L"JAM", 10039880, 388 }, /* Jamaica */
    { 125, L"SJ", L"SJM", 10039882, 744 }, /* Jan Mayen */
    { 126, L"JO", L"JOR", 47611, 400 }, /* Jordan */
    { 127, L"XX", L"XX", 161832256 }, /* Johnston Atoll */
    { 129, L"KE", L"KEN", 47603, 404 }, /* Kenya */
    { 130, L"KG", L"KGZ", 47590, 417 }, /* Kyrgyzstan */
    { 131, L"KP", L"PRK", 47600, 408 }, /* North Korea */
    { 133, L"KI", L"KIR", 21206, 296 }, /* Kiribati */
    { 134, L"KR", L"KOR", 47600, 410 }, /* Korea */
    { 136, L"KW", L"KWT", 47611, 414 }, /* Kuwait */
    { 137, L"KZ", L"KAZ", 47590, 398 }, /* Kazakhstan */
    { 138, L"LA", L"LAO", 47599, 418 }, /* Laos */
    { 139, L"LB", L"LBN", 47611, 422 }, /* Lebanon */
    { 140, L"LV", L"LVA", 10039882, 428 }, /* Latvia */
    { 141, L"LT", L"LTU", 10039882, 440 }, /* Lithuania */
    { 142, L"LR", L"LBR", 42483, 430 }, /* Liberia */
    { 143, L"SK", L"SVK", 47609, 703 }, /* Slovakia */
    { 145, L"LI", L"LIE", 10210824, 438 }, /* Liechtenstein */
    { 146, L"LS", L"LSO", 10039883, 426 }, /* Lesotho */
    { 147, L"LU", L"LUX", 10210824, 442 }, /* Luxembourg */
    { 148, L"LY", L"LBY", 42487, 434 }, /* Libya */
    { 149, L"MG", L"MDG", 47603, 450 }, /* Madagascar */
    { 151, L"MO", L"MAC", 47600, 446 }, /* Macao S.A.R. */
    { 152, L"MD", L"MDA", 47609, 498 }, /* Moldova */
    { 154, L"MN", L"MNG", 47600, 496 }, /* Mongolia */
    { 156, L"MW", L"MWI", 47603, 454 }, /* Malawi */
    { 157, L"ML", L"MLI", 42483, 466 }, /* Mali */
    { 158, L"MC", L"MCO", 10210824, 492 }, /* Monaco */
    { 159, L"MA", L"MAR", 42487, 504 }, /* Morocco */
    { 160, L"MU", L"MUS", 47603, 480 }, /* Mauritius */
    { 162, L"MR", L"MRT", 42483, 478 }, /* Mauritania */
    { 163, L"MT", L"MLT", 47610, 470 }, /* Malta */
    { 164, L"OM", L"OMN", 47611, 512 }, /* Oman */
    { 165, L"MV", L"MDV", 47614, 462 }, /* Maldives */
    { 166, L"MX", L"MEX", 27082, 484 }, /* Mexico */
    { 167, L"MY", L"MYS", 47599, 458 }, /* Malaysia */
    { 168, L"MZ", L"MOZ", 47603, 508 }, /* Mozambique */
    { 173, L"NE", L"NER", 42483, 562 }, /* Niger */
    { 174, L"VU", L"VUT", 20900, 548 }, /* Vanuatu */
    { 175, L"NG", L"NGA", 42483, 566 }, /* Nigeria */
    { 176, L"NL", L"NLD", 10210824, 528 }, /* Netherlands */
    { 177, L"NO", L"NOR", 10039882, 578 }, /* Norway */
    { 178, L"NP", L"NPL", 47614, 524 }, /* Nepal */
    { 180, L"NR", L"NRU", 21206, 520 }, /* Nauru */
    { 181, L"SR", L"SUR", 31396, 740 }, /* Suriname */
    { 182, L"NI", L"NIC", 27082, 558 }, /* Nicaragua */
    { 183, L"NZ", L"NZL", 10210825, 554 }, /* New Zealand */
    { 184, L"PS", L"PSE", 47611, 275 }, /* Palestinian Authority */
    { 185, L"PY", L"PRY", 31396, 600 }, /* Paraguay */
    { 187, L"PE", L"PER", 31396, 604 }, /* Peru */
    { 190, L"PK", L"PAK", 47614, 586 }, /* Pakistan */
    { 191, L"PL", L"POL", 47609, 616 }, /* Poland */
    { 192, L"PA", L"PAN", 27082, 591 }, /* Panama */
    { 193, L"PT", L"PRT", 47610, 620 }, /* Portugal */
    { 194, L"PG", L"PNG", 20900, 598 }, /* Papua New Guinea */
    { 195, L"PW", L"PLW", 21206, 585 }, /* Palau */
    { 196, L"GW", L"GNB", 42483, 624 }, /* Guinea-Bissau */
    { 197, L"QA", L"QAT", 47611, 634 }, /* Qatar */
    { 198, L"RE", L"REU", 47603, 638 }, /* Reunion */
    { 199, L"MH", L"MHL", 21206, 584 }, /* Marshall Islands */
    { 200, L"RO", L"ROU", 47609, 642 }, /* Romania */
    { 201, L"PH", L"PHL", 47599, 608 }, /* Philippines */
    { 202, L"PR", L"PRI", 10039880, 630 }, /* Puerto Rico */
    { 203, L"RU", L"RUS", 47609, 643 }, /* Russia */
    { 204, L"RW", L"RWA", 47603, 646 }, /* Rwanda */
    { 205, L"SA", L"SAU", 47611, 682 }, /* Saudi Arabia */
    { 206, L"PM", L"SPM", 23581, 666 }, /* St. Pierre and Miquelon */
    { 207, L"KN", L"KNA", 10039880, 659 }, /* St. Kitts and Nevis */
    { 208, L"SC", L"SYC", 47603, 690 }, /* Seychelles */
    { 209, L"ZA", L"ZAF", 10039883, 710 }, /* South Africa */
    { 210, L"SN", L"SEN", 42483, 686 }, /* Senegal */
    { 212, L"SI", L"SVN", 47610, 705 }, /* Slovenia */
    { 213, L"SL", L"SLE", 42483, 694 }, /* Sierra Leone */
    { 214, L"SM", L"SMR", 47610, 674 }, /* San Marino */
    { 215, L"SG", L"SGP", 47599, 702 }, /* Singapore */
    { 216, L"SO", L"SOM", 47603, 706 }, /* Somalia */
    { 217, L"ES", L"ESP", 47610, 724 }, /* Spain */
    { 218, L"LC", L"LCA", 10039880, 662 }, /* St. Lucia */
    { 219, L"SD", L"SDN", 42487, 736 }, /* Sudan */
    { 220, L"SJ", L"SJM", 10039882, 744 }, /* Svalbard */
    { 221, L"SE", L"SWE", 10039882, 752 }, /* Sweden */
    { 222, L"SY", L"SYR", 47611, 760 }, /* Syria */
    { 223, L"CH", L"CHE", 10210824, 756 }, /* Switzerland */
    { 224, L"AE", L"ARE", 47611, 784 }, /* United Arab Emirates */
    { 225, L"TT", L"TTO", 10039880, 780 }, /* Trinidad and Tobago */
    { 227, L"TH", L"THA", 47599, 764 }, /* Thailand */
    { 228, L"TJ", L"TJK", 47590, 762 }, /* Tajikistan */
    { 231, L"TO", L"TON", 26286, 776 }, /* Tonga */
    { 232, L"TG", L"TGO", 42483, 768 }, /* Togo */
    { 233, L"ST", L"STP", 42484, 678 }, /* São Tomé and Príncipe */
    { 234, L"TN", L"TUN", 42487, 788 }, /* Tunisia */
    { 235, L"TR", L"TUR", 47611, 792 }, /* Turkey */
    { 236, L"TV", L"TUV", 26286, 798 }, /* Tuvalu */
    { 237, L"TW", L"TWN", 47600, 158 }, /* Taiwan */
    { 238, L"TM", L"TKM", 47590, 795 }, /* Turkmenistan */
    { 239, L"TZ", L"TZA", 47603, 834 }, /* Tanzania */
    { 240, L"UG", L"UGA", 47603, 800 }, /* Uganda */
    { 241, L"UA", L"UKR", 47609, 804 }, /* Ukraine */
    { 242, L"GB", L"GBR", 10039882, 826 }, /* United Kingdom */
    { 244, L"US", L"USA", 23581, 840 }, /* United States */
    { 245, L"BF", L"BFA", 42483, 854 }, /* Burkina Faso */
    { 246, L"UY", L"URY", 31396, 858 }, /* Uruguay */
    { 247, L"UZ", L"UZB", 47590, 860 }, /* Uzbekistan */
    { 248, L"VC", L"VCT", 10039880, 670 }, /* St. Vincent and the Grenadines */
    { 249, L"VE", L"VEN", 31396, 862 }, /* Bolivarian Republic of Venezuela */
    { 251, L"VN", L"VNM", 47599, 704 }, /* Vietnam */
    { 252, L"VI", L"VIR", 10039880, 850 }, /* Virgin Islands */
    { 253, L"VA", L"VAT", 47610, 336 }, /* Vatican City */
    { 254, L"NA", L"NAM", 10039883, 516 }, /* Namibia */
    { 257, L"EH", L"ESH", 42487, 732 }, /* Western Sahara (disputed) */
    { 258, L"XX", L"XX", 161832256 }, /* Wake Island */
    { 259, L"WS", L"WSM", 26286, 882 }, /* Samoa */
    { 260, L"SZ", L"SWZ", 10039883, 748 }, /* Swaziland */
    { 261, L"YE", L"YEM", 47611, 887 }, /* Yemen */
    { 263, L"ZM", L"ZMB", 47603, 894 }, /* Zambia */
    { 264, L"ZW", L"ZWE", 47603, 716 }, /* Zimbabwe */
    { 269, L"CS", L"SCG", 47610, 891 }, /* Serbia and Montenegro (Former) */
    { 270, L"ME", L"MNE", 47610, 499 }, /* Montenegro */
    { 271, L"RS", L"SRB", 47610, 688 }, /* Serbia */
    { 273, L"CW", L"CUW", 10039880, 531 }, /* Curaçao */
    { 276, L"SS", L"SSD", 42487, 728 }, /* South Sudan */
    { 300, L"AI", L"AIA", 10039880, 660 }, /* Anguilla */
    { 301, L"AQ", L"ATA", 39070,  10 }, /* Antarctica */
    { 302, L"AW", L"ABW", 10039880, 533 }, /* Aruba */
    { 303, L"XX", L"XX", 343 }, /* Ascension Island */
    { 304, L"XX", L"XX", 10210825 }, /* Ashmore and Cartier Islands */
    { 305, L"XX", L"XX", 161832256 }, /* Baker Island */
    { 306, L"BV", L"BVT", 39070,  74 }, /* Bouvet Island */
    { 307, L"KY", L"CYM", 10039880, 136 }, /* Cayman Islands */
    { 308, L"XX", L"XX", 10210824, 830, LOCATION_BOTH }, /* Channel Islands */
    { 309, L"CX", L"CXR", 12, 162 }, /* Christmas Island */
    { 310, L"XX", L"XX", 27114 }, /* Clipperton Island */
    { 311, L"CC", L"CCK", 10210825, 166 }, /* Cocos (Keeling) Islands */
    { 312, L"CK", L"COK", 26286, 184 }, /* Cook Islands */
    { 313, L"XX", L"XX", 10210825 }, /* Coral Sea Islands */
    { 314, L"XX", L"XX", 114 }, /* Diego Garcia */
    { 315, L"FK", L"FLK", 31396, 238 }, /* Falkland Islands (Islas Malvinas) */
    { 317, L"GF", L"GUF", 31396, 254 }, /* French Guiana */
    { 318, L"PF", L"PYF", 26286, 258 }, /* French Polynesia */
    { 319, L"TF", L"ATF", 39070, 260 }, /* French Southern and Antarctic Lands */
    { 321, L"GP", L"GLP", 10039880, 312 }, /* Guadeloupe */
    { 322, L"GU", L"GUM", 21206, 316 }, /* Guam */
    { 323, L"XX", L"XX", 39070 }, /* Guantanamo Bay */
    { 324, L"GG", L"GGY", 308, 831 }, /* Guernsey */
    { 325, L"HM", L"HMD", 39070, 334 }, /* Heard Island and McDonald Islands */
    { 326, L"XX", L"XX", 161832256 }, /* Howland Island */
    { 327, L"XX", L"XX", 161832256 }, /* Jarvis Island */
    { 328, L"JE", L"JEY", 308, 832 }, /* Jersey */
    { 329, L"XX", L"XX", 161832256 }, /* Kingman Reef */
    { 330, L"MQ", L"MTQ", 10039880, 474 }, /* Martinique */
    { 331, L"YT", L"MYT", 47603, 175 }, /* Mayotte */
    { 332, L"MS", L"MSR", 10039880, 500 }, /* Montserrat */
    { 333, L"AN", L"ANT", 10039880, 530, LOCATION_BOTH }, /* Netherlands Antilles (Former) */
    { 334, L"NC", L"NCL", 20900, 540 }, /* New Caledonia */
    { 335, L"NU", L"NIU", 26286, 570 }, /* Niue */
    { 336, L"NF", L"NFK", 10210825, 574 }, /* Norfolk Island */
    { 337, L"MP", L"MNP", 21206, 580 }, /* Northern Mariana Islands */
    { 338, L"XX", L"XX", 161832256 }, /* Palmyra Atoll */
    { 339, L"PN", L"PCN", 26286, 612 }, /* Pitcairn Islands */
    { 340, L"XX", L"XX", 337 }, /* Rota Island */
    { 341, L"XX", L"XX", 337 }, /* Saipan */
    { 342, L"GS", L"SGS", 39070, 239 }, /* South Georgia and the South Sandwich Islands */
    { 343, L"SH", L"SHN", 42483, 654 }, /* St. Helena */
    { 346, L"XX", L"XX", 337 }, /* Tinian Island */
    { 347, L"TK", L"TKL", 26286, 772 }, /* Tokelau */
    { 348, L"XX", L"XX", 343 }, /* Tristan da Cunha */
    { 349, L"TC", L"TCA", 10039880, 796 }, /* Turks and Caicos Islands */
    { 351, L"VG", L"VGB", 10039880,  92 }, /* Virgin Islands, British */
    { 352, L"WF", L"WLF", 26286, 876 }, /* Wallis and Futuna */
    { 742, L"XX", L"XX", 39070, 2, LOCATION_REGION }, /* Africa */
    { 2129, L"XX", L"XX", 39070, 142, LOCATION_REGION }, /* Asia */
    { 10541, L"XX", L"XX", 39070, 150, LOCATION_REGION }, /* Europe */
    { 15126, L"IM", L"IMN", 10039882, 833 }, /* Man, Isle of */
    { 19618, L"MK", L"MKD", 47610, 807 }, /* Macedonia, Former Yugoslav Republic of */
    { 20900, L"XX", L"XX", 27114, 54, LOCATION_REGION }, /* Melanesia */
    { 21206, L"XX", L"XX", 27114, 57, LOCATION_REGION }, /* Micronesia */
    { 21242, L"XX", L"XX", 161832256 }, /* Midway Islands */
    { 23581, L"XX", L"XX", 10026358, 21, LOCATION_REGION }, /* Northern America */
    { 26286, L"XX", L"XX", 27114, 61, LOCATION_REGION }, /* Polynesia */
    { 27082, L"XX", L"XX", 161832257, 13, LOCATION_REGION }, /* Central America */
    { 27114, L"XX", L"XX", 39070, 9, LOCATION_REGION }, /* Oceania */
    { 30967, L"SX", L"SXM", 10039880, 534 }, /* Sint Maarten (Dutch part) */
    { 31396, L"XX", L"XX", 161832257, 5, LOCATION_REGION }, /* South America */
    { 31706, L"MF", L"MAF", 10039880, 663 }, /* Saint Martin (French part) */
    { 39070, L"XX", L"XX", 39070, 1, LOCATION_REGION }, /* World */
    { 42483, L"XX", L"XX", 742, 11, LOCATION_REGION }, /* Western Africa */
    { 42484, L"XX", L"XX", 742, 17, LOCATION_REGION }, /* Middle Africa */
    { 42487, L"XX", L"XX", 742, 15, LOCATION_REGION }, /* Northern Africa */
    { 47590, L"XX", L"XX", 2129, 143, LOCATION_REGION }, /* Central Asia */
    { 47599, L"XX", L"XX", 2129, 35, LOCATION_REGION }, /* South-Eastern Asia */
    { 47600, L"XX", L"XX", 2129, 30, LOCATION_REGION }, /* Eastern Asia */
    { 47603, L"XX", L"XX", 742, 14, LOCATION_REGION }, /* Eastern Africa */
    { 47609, L"XX", L"XX", 10541, 151, LOCATION_REGION }, /* Eastern Europe */
    { 47610, L"XX", L"XX", 10541, 39, LOCATION_REGION }, /* Southern Europe */
    { 47611, L"XX", L"XX", 2129, 145, LOCATION_REGION }, /* Middle East */
    { 47614, L"XX", L"XX", 2129, 34, LOCATION_REGION }, /* Southern Asia */
    { 7299303, L"TL", L"TLS", 47599, 626 }, /* Democratic Republic of Timor-Leste */
    { 9914689, L"XK", L"XKS", 47610, 906 }, /* Kosovo */
    { 10026358, L"XX", L"XX", 39070, 19, LOCATION_REGION }, /* Americas */
    { 10028789, L"AX", L"ALA", 10039882, 248 }, /* Åland Islands */
    { 10039880, L"XX", L"XX", 161832257, 29, LOCATION_REGION }, /* Caribbean */
    { 10039882, L"XX", L"XX", 10541, 154, LOCATION_REGION }, /* Northern Europe */
    { 10039883, L"XX", L"XX", 742, 18, LOCATION_REGION }, /* Southern Africa */
    { 10210824, L"XX", L"XX", 10541, 155, LOCATION_REGION }, /* Western Europe */
    { 10210825, L"XX", L"XX", 27114, 53, LOCATION_REGION }, /* Australia and New Zealand */
    { 161832015, L"BL", L"BLM", 10039880, 652 }, /* Saint Barthélemy */
    { 161832256, L"UM", L"UMI", 27114, 581 }, /* U.S. Minor Outlying Islands */
    { 161832257, L"XX", L"XX", 10026358, 419, LOCATION_REGION }, /* Latin America and the Caribbean */
    { 161832258, L"BG", L"BES", 10039880, 535 }, /* Bonaire, Sint Eustatius and Saba */
};

/* NLS normalization file */
struct norm_table
{
    WCHAR   name[13];      /* 00 file name */
    USHORT  checksum[3];   /* 1a checksum? */
    USHORT  version[4];    /* 20 Unicode version */
    USHORT  form;          /* 28 normalization form */
    USHORT  len_factor;    /* 2a factor for length estimates */
    USHORT  unknown1;      /* 2c */
    USHORT  decomp_size;   /* 2e decomposition hash size */
    USHORT  comp_size;     /* 30 composition hash size */
    USHORT  unknown2;      /* 32 */
    USHORT  classes;       /* 34 combining classes table offset */
    USHORT  props_level1;  /* 36 char properties table level 1 offset */
    USHORT  props_level2;  /* 38 char properties table level 2 offset */
    USHORT  decomp_hash;   /* 3a decomposition hash table offset */
    USHORT  decomp_map;    /* 3c decomposition character map table offset */
    USHORT  decomp_seq;    /* 3e decomposition character sequences offset */
    USHORT  comp_hash;     /* 40 composition hash table offset */
    USHORT  comp_seq;      /* 42 composition character sequences offset */
    /* BYTE[]       combining class values */
    /* BYTE[0x2200] char properties index level 1 */
    /* BYTE[]       char properties index level 2 */
    /* WORD[]       decomposition hash table */
    /* WORD[]       decomposition character map */
    /* WORD[]       decomposition character sequences */
    /* WORD[]       composition hash table */
    /* WORD[]       composition character sequences */
};

static NLSTABLEINFO nls_info;
static UINT unix_cp = CP_UTF8;
static UINT mac_cp = 10000;
static LCID system_lcid;
static LCID user_lcid;
static HKEY intl_key;
static HKEY nls_key;
static HKEY tz_key;
static const NLS_LOCALE_LCID_INDEX *lcids_index;
static const NLS_LOCALE_LCNAME_INDEX *lcnames_index;
static const NLS_LOCALE_HEADER *locale_table;
static const WCHAR *locale_strings;
static const NLS_LOCALE_DATA *system_locale;
static const NLS_LOCALE_DATA *user_locale;

static CPTABLEINFO codepages[128];
static unsigned int nb_codepages;

static struct norm_table *norm_info;

struct sortguid
{
    GUID  id;          /* sort GUID */
    DWORD flags;       /* flags */
    DWORD compr;       /* offset to compression table */
    DWORD except;      /* exception table offset in sortkey table */
    DWORD ling_except; /* exception table offset for linguistic casing */
    DWORD casemap;     /* linguistic casemap table offset */
};

#define FLAG_HAS_3_BYTE_WEIGHTS 0x01
#define FLAG_REVERSEDIACRITICS  0x10
#define FLAG_DOUBLECOMPRESSION  0x20
#define FLAG_INVERSECASING      0x40

static const struct sortguid *current_locale_sort;

static const GUID default_sort_guid = { 0x00000001, 0x57ee, 0x1e5c, { 0x00, 0xb4, 0xd0, 0x00, 0x0b, 0xb1, 0xe1, 0x1e }};

static struct
{
    DWORD           *keys;       /* sortkey table, indexed by char */
    USHORT          *casemap;    /* casemap table, in l_intl.nls format */
    WORD            *ctypes;     /* CT_CTYPE1,2,3 values */
    BYTE            *ctype_idx;  /* index to map char to ctypes array entry */
    DWORD            version;    /* NLS version */
    DWORD            guid_count; /* number of sort GUIDs */
    struct sortguid *guids;      /* table of sort GUIDs */
} sort;

static CRITICAL_SECTION locale_section;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &locale_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": locale_section") }
};
static CRITICAL_SECTION locale_section = { &critsect_debug, -1, 0, 0, 0, 0 };


static void load_locale_nls(void)
{
    struct
    {
        UINT ctypes;
        UINT unknown1;
        UINT unknown2;
        UINT unknown3;
        UINT locales;
        UINT charmaps;
        UINT geoids;
        UINT scripts;
    } *header;
    LCID lcid;
    LARGE_INTEGER dummy;

    RtlGetLocaleFileMappingAddress( (void **)&header, &lcid, &dummy );
    locale_table = (const NLS_LOCALE_HEADER *)((char *)header + header->locales);
    lcids_index = (const NLS_LOCALE_LCID_INDEX *)((char *)locale_table + locale_table->lcids_offset);
    lcnames_index = (const NLS_LOCALE_LCNAME_INDEX *)((char *)locale_table + locale_table->lcnames_offset);
    locale_strings = (const WCHAR *)((char *)locale_table + locale_table->strings_offset);
}


static void init_sortkeys( DWORD *ptr )
{
    WORD *ctype;
    DWORD *table;

    sort.keys = (DWORD *)((char *)ptr + ptr[0]);
    sort.casemap = (USHORT *)((char *)ptr + ptr[1]);

    ctype = (WORD *)((char *)ptr + ptr[2]);
    sort.ctypes = ctype + 2;
    sort.ctype_idx = (BYTE *)ctype + ctype[1] + 2;

    table = (DWORD *)((char *)ptr + ptr[3]);
    sort.version = table[0];
    sort.guid_count = table[1];
    sort.guids = (struct sortguid *)(table + 2);
}


static const struct sortguid *find_sortguid( const GUID *guid )
{
    int pos, ret, min = 0, max = sort.guid_count - 1;

    while (min <= max)
    {
        pos = (min + max) / 2;
        ret = memcmp( guid, &sort.guids[pos].id, sizeof(*guid) );
        if (!ret) return &sort.guids[pos];
        if (ret > 0) min = pos + 1;
        else max = pos - 1;
    }
    ERR( "no sort found for %s\n", debugstr_guid( guid ));
    return NULL;
}


static const struct sortguid *get_language_sort( const WCHAR *locale )
{
    WCHAR *p, *end, buffer[LOCALE_NAME_MAX_LENGTH], guidstr[39];
    const struct sortguid *ret;
    UNICODE_STRING str;
    GUID guid;
    HKEY key = 0;
    DWORD size, type;

    if (locale == LOCALE_NAME_USER_DEFAULT)
    {
        if (current_locale_sort) return current_locale_sort;
        GetUserDefaultLocaleName( buffer, ARRAY_SIZE( buffer ));
    }
    else lstrcpynW( buffer, locale, LOCALE_NAME_MAX_LENGTH );

    if (buffer[0] && !RegOpenKeyExW( nls_key, L"Sorting\\Ids", 0, KEY_READ, &key ))
    {
        for (;;)
        {
            size = sizeof(guidstr);
            if (!RegQueryValueExW( key, buffer, NULL, &type, (BYTE *)guidstr, &size ) && type == REG_SZ)
            {
                RtlInitUnicodeString( &str, guidstr );
                if (!RtlGUIDFromString( &str, &guid ))
                {
                    ret = find_sortguid( &guid );
                    goto done;
                }
                break;
            }
            for (p = end = buffer; *p; p++) if (*p == '-' || *p == '_') end = p;
            if (end == buffer) break;
            *end = 0;
        }
    }
    ret = find_sortguid( &default_sort_guid );
done:
    RegCloseKey( key );
    return ret;
}


static const NLS_LOCALE_DATA *get_locale_data( UINT idx )
{
    ULONG offset = locale_table->locales_offset + idx * locale_table->locale_size;
    return (const NLS_LOCALE_DATA *)((const char *)locale_table + offset);
}


static int compare_locale_names( const WCHAR *n1, const WCHAR *n2 )
{
    for (;;)
    {
        WCHAR ch1 = *n1++;
        WCHAR ch2 = *n2++;
        if (ch1 >= 'a' && ch1 <= 'z') ch1 -= 'a' - 'A';
        else if (ch1 == '_') ch1 = '-';
        if (ch2 >= 'a' && ch2 <= 'z') ch2 -= 'a' - 'A';
        else if (ch2 == '_') ch2 = '-';
        if (!ch1 || ch1 != ch2) return ch1 - ch2;
    }
}


static const NLS_LOCALE_LCNAME_INDEX *find_lcname_entry( const WCHAR *name )
{
    int min = 0, max = locale_table->nb_lcnames - 1;

    while (min <= max)
    {
        int res, pos = (min + max) / 2;
        const WCHAR *str = locale_strings + lcnames_index[pos].name;
        res = compare_locale_names( name, str + 1 );
        if (res < 0) max = pos - 1;
        else if (res > 0) min = pos + 1;
        else return &lcnames_index[pos];
    }
    return NULL;
}


static const NLS_LOCALE_LCID_INDEX *find_lcid_entry( LCID lcid )
{
    int min = 0, max = locale_table->nb_lcids - 1;

    while (min <= max)
    {
        int pos = (min + max) / 2;
        if (lcid < lcids_index[pos].id) max = pos - 1;
        else if (lcid > lcids_index[pos].id) min = pos + 1;
        else return &lcids_index[pos];
    }
    return NULL;
}


static const NLS_LOCALE_DATA *get_locale_by_name( const WCHAR *name, LCID *lcid )
{
    const NLS_LOCALE_LCNAME_INDEX *entry;

    if (name == LOCALE_NAME_USER_DEFAULT)
    {
        *lcid = user_lcid;
        return user_locale;
    }
    if (!(entry = find_lcname_entry( name ))) return NULL;
    *lcid = entry->id;
    return get_locale_data( entry->idx );
}


static const NLS_LOCALE_DATA *get_locale_by_id( LCID *lcid, DWORD flags )
{
    const NLS_LOCALE_LCID_INDEX *entry;
    const NLS_LOCALE_DATA *locale;

    switch (*lcid)
    {
    case LOCALE_SYSTEM_DEFAULT:
        *lcid = system_lcid;
        return system_locale;
    case LOCALE_NEUTRAL:
    case LOCALE_USER_DEFAULT:
    case LOCALE_CUSTOM_DEFAULT:
        *lcid = user_lcid;
        return user_locale;
    default:
        if (!(entry = find_lcid_entry( *lcid ))) return NULL;
        locale = get_locale_data( entry->idx );
        if (!(flags & LOCALE_ALLOW_NEUTRAL_NAMES) && !locale->inotneutral)
            locale = get_locale_by_name( locale_strings + locale->ssortlocale + 1, lcid );
        return locale;
    }
}


/***********************************************************************
 *		init_locale
 */
void init_locale(void)
{
    UINT ansi_cp = 0, oem_cp = 0;
    USHORT *ansi_ptr, *oem_ptr;
    void *sort_ptr;
    WCHAR bufferW[LOCALE_NAME_MAX_LENGTH];
    DYNAMIC_TIME_ZONE_INFORMATION timezone;
    GEOID geoid = GEOID_NOT_AVAILABLE;
    DWORD count, dispos, i;
    SIZE_T size;
    HKEY hkey;

    load_locale_nls();
    NtQueryDefaultLocale( FALSE, &system_lcid );
    NtQueryDefaultLocale( FALSE, &user_lcid );
    system_locale = get_locale_by_id( &system_lcid, 0 );
    user_locale = get_locale_by_id( &user_lcid, 0 );

    if (GetEnvironmentVariableW( L"WINEUNIXCP", bufferW, ARRAY_SIZE(bufferW) ))
        unix_cp = wcstoul( bufferW, NULL, 10 );

    kernel32_handle = GetModuleHandleW( L"kernel32.dll" );

    GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&ansi_cp, sizeof(ansi_cp)/sizeof(WCHAR) );
    GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTMACCODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&mac_cp, sizeof(mac_cp)/sizeof(WCHAR) );
    GetLocaleInfoW( LOCALE_SYSTEM_DEFAULT, LOCALE_IDEFAULTCODEPAGE | LOCALE_RETURN_NUMBER,
                    (WCHAR *)&oem_cp, sizeof(oem_cp)/sizeof(WCHAR) );

    NtGetNlsSectionPtr( 9, 0, NULL, &sort_ptr, &size );
    NtGetNlsSectionPtr( 12, NormalizationC, NULL, (void **)&norm_info, &size );
    init_sortkeys( sort_ptr );

    if (!ansi_cp || NtGetNlsSectionPtr( 11, ansi_cp, NULL, (void **)&ansi_ptr, &size ))
        NtGetNlsSectionPtr( 11, 1252, NULL, (void **)&ansi_ptr, &size );
    if (!oem_cp || NtGetNlsSectionPtr( 11, oem_cp, 0, (void **)&oem_ptr, &size ))
        NtGetNlsSectionPtr( 11, 437, NULL, (void **)&oem_ptr, &size );
    NtCurrentTeb()->Peb->AnsiCodePageData = ansi_ptr;
    NtCurrentTeb()->Peb->OemCodePageData = oem_ptr;
    NtCurrentTeb()->Peb->UnicodeCaseTableData = sort.casemap;
    RtlInitNlsTables( ansi_ptr, oem_ptr, sort.casemap, &nls_info );
    RtlResetRtlTranslations( &nls_info );

    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\Nls",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &nls_key, NULL );
    RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &tz_key, NULL );
    RegCreateKeyExW( HKEY_CURRENT_USER, L"Control Panel\\International",
                     0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &intl_key, NULL );

    current_locale_sort = get_language_sort( LOCALE_NAME_USER_DEFAULT );

    if (GetDynamicTimeZoneInformation( &timezone ) != TIME_ZONE_ID_INVALID &&
        !RegCreateKeyExW( HKEY_LOCAL_MACHINE, L"System\\CurrentControlSet\\Control\\TimeZoneInformation",
                          0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        RegSetValueExW( hkey, L"StandardName", 0, REG_SZ, (BYTE *)timezone.StandardName,
                        (lstrlenW(timezone.StandardName) + 1) * sizeof(WCHAR) );
        RegSetValueExW( hkey, L"TimeZoneKeyName", 0, REG_SZ, (BYTE *)timezone.TimeZoneKeyName,
                        (lstrlenW(timezone.TimeZoneKeyName) + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }

    if (!RegCreateKeyExW( intl_key, L"Geo", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, &dispos ))
    {
        if (dispos == REG_CREATED_NEW_KEY)
        {
            GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IGEOID | LOCALE_RETURN_NUMBER,
                            (WCHAR *)&geoid, sizeof(geoid) / sizeof(WCHAR) );
            SetUserGeoID( geoid );
        }
        RegCloseKey( hkey );
    }

    /* Update registry contents if the user locale has changed.
     * This simulates the action of the Windows control panel. */

    count = sizeof(bufferW);
    if (!RegQueryValueExW( intl_key, L"Locale", NULL, NULL, (BYTE *)bufferW, &count ))
    {
        if (wcstoul( bufferW, NULL, 16 ) == user_lcid) return;  /* already set correctly */
        TRACE( "updating registry, locale changed %s -> %08lx\n", debugstr_w(bufferW), user_lcid );
    }
    else TRACE( "updating registry, locale changed none -> %08lx\n", user_lcid );
    swprintf( bufferW, ARRAY_SIZE(bufferW), L"%08x", user_lcid );
    RegSetValueExW( intl_key, L"Locale", 0, REG_SZ,
                    (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );

    for (i = 0; i < ARRAY_SIZE(registry_values); i++)
    {
        GetLocaleInfoW( LOCALE_USER_DEFAULT, registry_values[i].lctype | LOCALE_NOUSEROVERRIDE,
                        bufferW, ARRAY_SIZE( bufferW ));
        RegSetValueExW( intl_key, registry_values[i].name, 0, REG_SZ,
                        (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );
    }

    if (geoid == GEOID_NOT_AVAILABLE)
    {
        GetLocaleInfoW( LOCALE_USER_DEFAULT, LOCALE_IGEOID | LOCALE_RETURN_NUMBER,
                        (WCHAR *)&geoid, sizeof(geoid) / sizeof(WCHAR) );
        SetUserGeoID( geoid );
    }

    if (!RegCreateKeyExW( nls_key, L"Codepage",
                          0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", ansi_cp );
        RegSetValueExW( hkey, L"ACP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", oem_cp );
        RegSetValueExW( hkey, L"OEMCP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        count = swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03d", mac_cp );
        RegSetValueExW( hkey, L"MACCP", 0, REG_SZ, (BYTE *)bufferW, (count + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }
}


static inline USHORT get_table_entry( const USHORT *table, WCHAR ch )
{
    return table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
}


static inline WCHAR casemap( const USHORT *table, WCHAR ch )
{
    return ch + table[table[table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0x0f)];
}


static inline WORD get_char_type( DWORD type, WCHAR ch )
{
    const BYTE *ptr = sort.ctype_idx + ((const WORD *)sort.ctype_idx)[ch >> 8];
    ptr = sort.ctype_idx + ((const WORD *)ptr)[(ch >> 4) & 0x0f] + (ch & 0x0f);
    return sort.ctypes[*ptr * 3 + type / 2];
}


static BYTE rol( BYTE val, BYTE count )
{
    return (val << count) | (val >> (8 - count));
}


static BYTE get_char_props( const struct norm_table *info, unsigned int ch )
{
    const BYTE *level1 = (const BYTE *)((const USHORT *)info + info->props_level1);
    const BYTE *level2 = (const BYTE *)((const USHORT *)info + info->props_level2);
    BYTE off = level1[ch / 128];

    if (!off || off >= 0xfb) return rol( off, 5 );
    return level2[(off - 1) * 128 + ch % 128];
}


static const WCHAR *get_decomposition( WCHAR ch, unsigned int *ret_len )
{
    const struct pair { WCHAR src; USHORT dst; } *pairs;
    const USHORT *hash_table = (const USHORT *)norm_info + norm_info->decomp_hash;
    const WCHAR *ret;
    unsigned int i, pos, end, len, hash;

    *ret_len = 1;
    hash = ch % norm_info->decomp_size;
    pos = hash_table[hash];
    if (pos >> 13)
    {
        if (get_char_props( norm_info, ch ) != 0xbf) return NULL;
        ret = (const USHORT *)norm_info + norm_info->decomp_seq + (pos & 0x1fff);
        len = pos >> 13;
    }
    else
    {
        pairs = (const struct pair *)((const USHORT *)norm_info + norm_info->decomp_map);

        /* find the end of the hash bucket */
        for (i = hash + 1; i < norm_info->decomp_size; i++) if (!(hash_table[i] >> 13)) break;
        if (i < norm_info->decomp_size) end = hash_table[i];
        else for (end = pos; pairs[end].src; end++) ;

        for ( ; pos < end; pos++)
        {
            if (pairs[pos].src != (WCHAR)ch) continue;
            ret = (const USHORT *)norm_info + norm_info->decomp_seq + (pairs[pos].dst & 0x1fff);
            len = pairs[pos].dst >> 13;
            break;
        }
        if (pos >= end) return NULL;
    }

    if (len == 7) while (ret[len]) len++;
    if (!ret[0]) len = 0;  /* ignored char */
    *ret_len = len;
    return ret;
}


static WCHAR compose_chars( WCHAR ch1, WCHAR ch2 )
{
    const USHORT *table = (const USHORT *)norm_info + norm_info->comp_hash;
    const WCHAR *chars = (const USHORT *)norm_info + norm_info->comp_seq;
    unsigned int hash, start, end, i;
    WCHAR ch[3];

    hash = (ch1 + 95 * ch2) % norm_info->comp_size;
    start = table[hash];
    end = table[hash + 1];
    while (start < end)
    {
        for (i = 0; i < 3; i++, start++)
        {
            ch[i] = chars[start];
            if (IS_HIGH_SURROGATE( ch[i] )) start++;
        }
        if (ch[0] == ch1 && ch[1] == ch2) return ch[2];
    }
    return 0;
}


static UINT get_lcid_codepage( LCID lcid, ULONG flags )
{
    UINT ret = GetACP();

    if (!(flags & LOCALE_USE_CP_ACP) && lcid != GetSystemDefaultLCID())
        GetLocaleInfoW( lcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                        (WCHAR *)&ret, sizeof(ret)/sizeof(WCHAR) );
    return ret;
}


static BOOL is_genitive_name_supported( LCTYPE lctype )
{
    switch (LOWORD(lctype))
    {
    case LOCALE_SMONTHNAME1:
    case LOCALE_SMONTHNAME2:
    case LOCALE_SMONTHNAME3:
    case LOCALE_SMONTHNAME4:
    case LOCALE_SMONTHNAME5:
    case LOCALE_SMONTHNAME6:
    case LOCALE_SMONTHNAME7:
    case LOCALE_SMONTHNAME8:
    case LOCALE_SMONTHNAME9:
    case LOCALE_SMONTHNAME10:
    case LOCALE_SMONTHNAME11:
    case LOCALE_SMONTHNAME12:
    case LOCALE_SMONTHNAME13:
         return TRUE;
    default:
         return FALSE;
    }
}


static int get_value_base_by_lctype( LCTYPE lctype )
{
    return lctype == LOCALE_ILANGUAGE || lctype == LOCALE_IDEFAULTLANGUAGE ? 16 : 10;
}


static const struct registry_value *get_locale_registry_value( DWORD lctype )
{
    unsigned int i;

    for (i = 0; i < ARRAY_SIZE( registry_values ); i++)
        if (registry_values[i].lctype == lctype) return &registry_values[i];
    return NULL;
}


static INT get_registry_locale_info( const struct registry_value *registry_value, LPWSTR buffer, INT len )
{
    DWORD size, index = registry_value - registry_values;
    INT ret;

    RtlEnterCriticalSection( &locale_section );

    if (!registry_cache[index])
    {
        size = len * sizeof(WCHAR);
        ret = RegQueryValueExW( intl_key, registry_value->name, NULL, NULL, (BYTE *)buffer, &size );
        if (!ret)
        {
            if (buffer && (registry_cache[index] = HeapAlloc( GetProcessHeap(), 0, size + sizeof(WCHAR) )))
            {
                memcpy( registry_cache[index], buffer, size );
                registry_cache[index][size / sizeof(WCHAR)] = 0;
            }
            RtlLeaveCriticalSection( &locale_section );
            return size / sizeof(WCHAR);
        }
        else
        {
            RtlLeaveCriticalSection( &locale_section );
            if (ret == ERROR_FILE_NOT_FOUND) return -1;
            if (ret == ERROR_MORE_DATA) SetLastError( ERROR_INSUFFICIENT_BUFFER );
            else SetLastError( ret );
            return 0;
        }
    }

    ret = lstrlenW( registry_cache[index] ) + 1;
    if (buffer)
    {
        if (ret > len)
        {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            ret = 0;
        }
        else lstrcpyW( buffer, registry_cache[index] );
    }
    RtlLeaveCriticalSection( &locale_section );
    return ret;
}


static const CPTABLEINFO *get_codepage_table( UINT codepage )
{
    unsigned int i;
    USHORT *ptr;
    SIZE_T size;

    switch (codepage)
    {
    case CP_ACP:
        return &nls_info.AnsiTableInfo;
    case CP_OEMCP:
        return &nls_info.OemTableInfo;
    case CP_MACCP:
        codepage = mac_cp;
        break;
    case CP_THREAD_ACP:
        if (NtCurrentTeb()->CurrentLocale == GetUserDefaultLCID()) return &nls_info.AnsiTableInfo;
        codepage = get_lcid_codepage( NtCurrentTeb()->CurrentLocale, 0 );
        if (!codepage) return &nls_info.AnsiTableInfo;
        break;
    default:
        if (codepage == nls_info.AnsiTableInfo.CodePage) return &nls_info.AnsiTableInfo;
        if (codepage == nls_info.OemTableInfo.CodePage) return &nls_info.OemTableInfo;
        break;
    }

    RtlEnterCriticalSection( &locale_section );

    for (i = 0; i < nb_codepages; i++) if (codepages[i].CodePage == codepage) goto done;

    if (i == ARRAY_SIZE( codepages ))
    {
        RtlLeaveCriticalSection( &locale_section );
        ERR( "too many codepages\n" );
        return NULL;
    }
    if (NtGetNlsSectionPtr( 11, codepage, NULL, (void **)&ptr, &size ))
    {
        RtlLeaveCriticalSection( &locale_section );
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    RtlInitCodePageTable( ptr, &codepages[i] );
    nb_codepages++;
done:
    RtlLeaveCriticalSection( &locale_section );
    return &codepages[i];
}


static const WCHAR *get_ligature( WCHAR wc )
{
    int low = 0, high = ARRAY_SIZE( ligatures ) -1;
    while (low <= high)
    {
        int pos = (low + high) / 2;
        if (ligatures[pos][0] < wc) low = pos + 1;
        else if (ligatures[pos][0] > wc) high = pos - 1;
        else return ligatures[pos] + 1;
    }
    return NULL;
}


static NTSTATUS expand_ligatures( const WCHAR *src, int srclen, WCHAR *dst, int *dstlen )
{
    int i, len, pos = 0;
    NTSTATUS ret = STATUS_SUCCESS;
    const WCHAR *expand;

    for (i = 0; i < srclen; i++)
    {
        if (!(expand = get_ligature( src[i] )))
        {
            expand = src + i;
            len = 1;
        }
        else len = lstrlenW( expand );

        if (*dstlen && ret == STATUS_SUCCESS)
        {
            if (pos + len <= *dstlen) memcpy( dst + pos, expand, len * sizeof(WCHAR) );
            else ret = STATUS_BUFFER_TOO_SMALL;
        }
        pos += len;
    }
    *dstlen = pos;
    return ret;
}


static NTSTATUS fold_digits( const WCHAR *src, int srclen, WCHAR *dst, int *dstlen )
{
    extern const WCHAR wine_digitmap[] DECLSPEC_HIDDEN;
    int i, len = *dstlen;

    *dstlen = srclen;
    if (!len) return STATUS_SUCCESS;
    if (srclen > len) return STATUS_BUFFER_TOO_SMALL;
    for (i = 0; i < srclen; i++)
    {
        WCHAR digit = get_table_entry( wine_digitmap, src[i] );
        dst[i] = digit ? digit : src[i];
    }
    return STATUS_SUCCESS;
}


static NTSTATUS fold_string( DWORD flags, const WCHAR *src, int srclen, WCHAR *dst, int *dstlen )
{
    NTSTATUS ret;
    WCHAR *tmp;

    switch (flags)
    {
    case MAP_PRECOMPOSED:
        return RtlNormalizeString( NormalizationC, src, srclen, dst, dstlen );
    case MAP_FOLDCZONE:
    case MAP_PRECOMPOSED | MAP_FOLDCZONE:
        return RtlNormalizeString( NormalizationKC, src, srclen, dst, dstlen );
    case MAP_COMPOSITE:
        return RtlNormalizeString( NormalizationD, src, srclen, dst, dstlen );
    case MAP_COMPOSITE | MAP_FOLDCZONE:
        return RtlNormalizeString( NormalizationKD, src, srclen, dst, dstlen );
    case MAP_FOLDDIGITS:
        return fold_digits( src, srclen, dst, dstlen );
    case MAP_EXPAND_LIGATURES:
    case MAP_EXPAND_LIGATURES | MAP_FOLDCZONE:
        return expand_ligatures( src, srclen, dst, dstlen );
    case MAP_FOLDDIGITS | MAP_PRECOMPOSED:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationC, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_FOLDCZONE:
    case MAP_FOLDDIGITS | MAP_PRECOMPOSED | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationKC, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_COMPOSITE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationD, tmp, srclen, dst, dstlen );
        break;
    case MAP_FOLDDIGITS | MAP_COMPOSITE | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = RtlNormalizeString( NormalizationKD, tmp, srclen, dst, dstlen );
        break;
    case MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS:
    case MAP_EXPAND_LIGATURES | MAP_FOLDDIGITS | MAP_FOLDCZONE:
        if (!(tmp = RtlAllocateHeap( GetProcessHeap(), 0, srclen * sizeof(WCHAR) )))
            return STATUS_NO_MEMORY;
        fold_digits( src, srclen, tmp, &srclen );
        ret = expand_ligatures( tmp, srclen, dst, dstlen );
        break;
    default:
        return STATUS_INVALID_PARAMETER_1;
    }
    RtlFreeHeap( GetProcessHeap(), 0, tmp );
    return ret;
}


static int mbstowcs_cpsymbol( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    int len, i;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) return srclen;
    len = min( srclen, dstlen );
    for (i = 0; i < len; i++)
    {
        unsigned char c = src[i];
        dst[i] = (c < 0x20) ? c : c + 0xf000;
    }
    if (len < srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return len;
}


static int mbstowcs_utf7( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    static const signed char base64_decoding_table[] =
    {
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x00-0x0F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, /* 0x10-0x1F */
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, /* 0x20-0x2F */
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, /* 0x30-0x3F */
        -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, /* 0x40-0x4F */
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, /* 0x50-0x5F */
        -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, /* 0x60-0x6F */
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1  /* 0x70-0x7F */
    };

    const char *source_end = src + srclen;
    int offset = 0, pos = 0;
    DWORD byte_pair = 0;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
#define OUTPUT(ch) \
    do { \
        if (dstlen > 0) \
        { \
            if (pos >= dstlen) goto overflow; \
            dst[pos] = (ch); \
        } \
        pos++; \
    } while(0)

    while (src < source_end)
    {
        if (*src == '+')
        {
            src++;
            if (src >= source_end) break;
            if (*src == '-')
            {
                /* just a plus sign escaped as +- */
                OUTPUT( '+' );
                src++;
                continue;
            }

            do
            {
                signed char sextet = *src;
                if (sextet == '-')
                {
                    /* skip over the dash and end base64 decoding
                     * the current, unfinished byte pair is discarded */
                    src++;
                    offset = 0;
                    break;
                }
                if (sextet < 0)
                {
                    /* the next character of src is < 0 and therefore not part of a base64 sequence
                     * the current, unfinished byte pair is NOT discarded in this case
                     * this is probably a bug in Windows */
                    break;
                }
                sextet = base64_decoding_table[sextet];
                if (sextet == -1)
                {
                    /* -1 means that the next character of src is not part of a base64 sequence
                     * in other words, all sextets in this base64 sequence have been processed
                     * the current, unfinished byte pair is discarded */
                    offset = 0;
                    break;
                }

                byte_pair = (byte_pair << 6) | sextet;
                offset += 6;
                if (offset >= 16)
                {
                    /* this byte pair is done */
                    OUTPUT( byte_pair >> (offset - 16) );
                    offset -= 16;
                }
                src++;
            }
            while (src < source_end);
        }
        else
        {
            OUTPUT( (unsigned char)*src );
            src++;
        }
    }
    return pos;

overflow:
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return 0;
#undef OUTPUT
}


static int mbstowcs_utf8( DWORD flags, const char *src, int srclen, WCHAR *dst, int dstlen )
{
    DWORD reslen;
    NTSTATUS status;

    if (flags & ~(MB_PRECOMPOSED | MB_COMPOSITE | MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) dst = NULL;
    status = RtlUTF8ToUnicodeN( dst, dstlen * sizeof(WCHAR), &reslen, src, srclen );
    if (status == STATUS_SOME_NOT_MAPPED)
    {
        if (flags & MB_ERR_INVALID_CHARS)
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    else if (!set_ntstatus( status )) reslen = 0;

    return reslen / sizeof(WCHAR);
}


static inline int is_private_use_area_char( WCHAR code )
{
    return (code >= 0xe000 && code <= 0xf8ff);
}


static int check_invalid_chars( const CPTABLEINFO *info, const unsigned char *src, int srclen )
{
    if (info->DBCSOffsets)
    {
        for ( ; srclen; src++, srclen-- )
        {
            USHORT off = info->DBCSOffsets[*src];
            if (off)
            {
                if (srclen == 1) break;  /* partial char, error */
                if (info->DBCSOffsets[off + src[1]] == info->UniDefaultChar &&
                    ((src[0] << 8) | src[1]) != info->TransUniDefaultChar) break;
                src++;
                srclen--;
                continue;
            }
            if (info->MultiByteTable[*src] == info->UniDefaultChar && *src != info->TransUniDefaultChar)
                break;
            if (is_private_use_area_char( info->MultiByteTable[*src] )) break;
        }
    }
    else
    {
        for ( ; srclen; src++, srclen-- )
        {
            if (info->MultiByteTable[*src] == info->UniDefaultChar && *src != info->TransUniDefaultChar)
                break;
            if (is_private_use_area_char( info->MultiByteTable[*src] )) break;
        }
    }
    return !!srclen;

}


static int mbstowcs_decompose( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                               WCHAR *dst, int dstlen )
{
    WCHAR ch;
    USHORT off;
    int len;
    const WCHAR *decomp;
    unsigned int decomp_len;

    if (info->DBCSOffsets)
    {
        if (!dstlen)  /* compute length */
        {
            for (len = 0; srclen; srclen--, src++, len += decomp_len)
            {
                if ((off = info->DBCSOffsets[*src]))
                {
                    if (srclen > 1 && src[1])
                    {
                        src++;
                        srclen--;
                        ch = info->DBCSOffsets[off + *src];
                    }
                    else ch = info->UniDefaultChar;
                }
                else ch = info->MultiByteTable[*src];
                get_decomposition( ch, &decomp_len );
            }
            return len;
        }

        for (len = dstlen; srclen && len; srclen--, src++, dst += decomp_len, len -= decomp_len)
        {
            if ((off = info->DBCSOffsets[*src]))
            {
                if (srclen > 1 && src[1])
                {
                    src++;
                    srclen--;
                    ch = info->DBCSOffsets[off + *src];
                }
                else ch = info->UniDefaultChar;
            }
            else ch = info->MultiByteTable[*src];

            if ((decomp = get_decomposition( ch, &decomp_len )))
            {
                if (len < decomp_len) break;
                memcpy( dst, decomp, decomp_len * sizeof(WCHAR) );
            }
            else *dst = ch;
        }
    }
    else
    {
        if (!dstlen)  /* compute length */
        {
            for (len = 0; srclen; srclen--, src++, len += decomp_len)
                get_decomposition( info->MultiByteTable[*src], &decomp_len );
            return len;
        }

        for (len = dstlen; srclen && len; srclen--, src++, dst += decomp_len, len -= decomp_len)
        {
            ch = info->MultiByteTable[*src];
            if ((decomp = get_decomposition( ch, &decomp_len )))
            {
                if (len < decomp_len) break;
                memcpy( dst, decomp, decomp_len * sizeof(WCHAR) );
            }
            else *dst = ch;
        }
    }

    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - len;
}


static int mbstowcs_sbcs( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                          WCHAR *dst, int dstlen )
{
    const USHORT *table = info->MultiByteTable;
    int ret = srclen;

    if (!dstlen) return srclen;

    if (dstlen < srclen)  /* buffer too small: fill it up to dstlen and return error */
    {
        srclen = dstlen;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }

    while (srclen >= 16)
    {
        dst[0]  = table[src[0]];
        dst[1]  = table[src[1]];
        dst[2]  = table[src[2]];
        dst[3]  = table[src[3]];
        dst[4]  = table[src[4]];
        dst[5]  = table[src[5]];
        dst[6]  = table[src[6]];
        dst[7]  = table[src[7]];
        dst[8]  = table[src[8]];
        dst[9]  = table[src[9]];
        dst[10] = table[src[10]];
        dst[11] = table[src[11]];
        dst[12] = table[src[12]];
        dst[13] = table[src[13]];
        dst[14] = table[src[14]];
        dst[15] = table[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle the remaining characters */
    src += srclen;
    dst += srclen;
    switch (srclen)
    {
    case 15: dst[-15] = table[src[-15]];
    case 14: dst[-14] = table[src[-14]];
    case 13: dst[-13] = table[src[-13]];
    case 12: dst[-12] = table[src[-12]];
    case 11: dst[-11] = table[src[-11]];
    case 10: dst[-10] = table[src[-10]];
    case 9:  dst[-9]  = table[src[-9]];
    case 8:  dst[-8]  = table[src[-8]];
    case 7:  dst[-7]  = table[src[-7]];
    case 6:  dst[-6]  = table[src[-6]];
    case 5:  dst[-5]  = table[src[-5]];
    case 4:  dst[-4]  = table[src[-4]];
    case 3:  dst[-3]  = table[src[-3]];
    case 2:  dst[-2]  = table[src[-2]];
    case 1:  dst[-1]  = table[src[-1]];
    case 0: break;
    }
    return ret;
}


static int mbstowcs_dbcs( const CPTABLEINFO *info, const unsigned char *src, int srclen,
                          WCHAR *dst, int dstlen )
{
    USHORT off;
    int i;

    if (!dstlen)
    {
        for (i = 0; srclen; i++, src++, srclen--)
            if (info->DBCSOffsets[*src] && srclen > 1 && src[1]) { src++; srclen--; }
        return i;
    }

    for (i = dstlen; srclen && i; i--, srclen--, src++, dst++)
    {
        if ((off = info->DBCSOffsets[*src]))
        {
            if (srclen > 1 && src[1])
            {
                src++;
                srclen--;
                *dst = info->DBCSOffsets[off + *src];
            }
            else *dst = info->UniDefaultChar;
        }
        else *dst = info->MultiByteTable[*src];
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int mbstowcs_codepage( UINT codepage, DWORD flags, const char *src, int srclen,
                              WCHAR *dst, int dstlen )
{
    CPTABLEINFO local_info;
    const CPTABLEINFO *info = get_codepage_table( codepage );
    const unsigned char *str = (const unsigned char *)src;

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags & ~(MB_PRECOMPOSED | MB_COMPOSITE | MB_USEGLYPHCHARS | MB_ERR_INVALID_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if ((flags & MB_USEGLYPHCHARS) && info->MultiByteTable[256] == 256)
    {
        local_info = *info;
        local_info.MultiByteTable += 257;
        info = &local_info;
    }
    if ((flags & MB_ERR_INVALID_CHARS) && check_invalid_chars( info, str, srclen ))
    {
        SetLastError( ERROR_NO_UNICODE_TRANSLATION );
        return 0;
    }

    if (flags & MB_COMPOSITE) return mbstowcs_decompose( info, str, srclen, dst, dstlen );

    if (info->DBCSOffsets)
        return mbstowcs_dbcs( info, str, srclen, dst, dstlen );
    else
        return mbstowcs_sbcs( info, str, srclen, dst, dstlen );
}


static int wcstombs_cpsymbol( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                              const char *defchar, BOOL *used )
{
    int len, i;

    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!dstlen) return srclen;
    len = min( srclen, dstlen );
    for (i = 0; i < len; i++)
    {
        if (src[i] < 0x20) dst[i] = src[i];
        else if (src[i] >= 0xf020 && src[i] < 0xf100) dst[i] = src[i] - 0xf000;
        else
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    if (srclen > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return len;
}


static int wcstombs_utf7( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                          const char *defchar, BOOL *used )
{
    static const char directly_encodable[] =
    {
        1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, /* 0x00 - 0x0f */
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 - 0x1f */
        1, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, /* 0x20 - 0x2f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 0x30 - 0x3f */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x40 - 0x4f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 0x50 - 0x5f */
        0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 0x60 - 0x6f */
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1                 /* 0x70 - 0x7a */
    };
#define ENCODABLE(ch) ((ch) <= 0x7a && directly_encodable[(ch)])

    static const char base64_encoding_table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    const WCHAR *source_end = src + srclen;
    int pos = 0;

    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

#define OUTPUT(ch) \
    do { \
        if (dstlen > 0) \
        { \
            if (pos >= dstlen) goto overflow; \
            dst[pos] = (ch); \
        } \
        pos++; \
    } while (0)

    while (src < source_end)
    {
        if (*src == '+')
        {
            OUTPUT( '+' );
            OUTPUT( '-' );
            src++;
        }
        else if (ENCODABLE(*src))
        {
            OUTPUT( *src );
            src++;
        }
        else
        {
            unsigned int offset = 0, byte_pair = 0;

            OUTPUT( '+' );
            while (src < source_end && !ENCODABLE(*src))
            {
                byte_pair = (byte_pair << 16) | *src;
                offset += 16;
                while (offset >= 6)
                {
                    offset -= 6;
                    OUTPUT( base64_encoding_table[(byte_pair >> offset) & 0x3f] );
                }
                src++;
            }
            if (offset)
            {
                /* Windows won't create a padded base64 character if there's no room for the - sign
                 * as well ; this is probably a bug in Windows */
                if (dstlen > 0 && pos + 1 >= dstlen) goto overflow;
                byte_pair <<= (6 - offset);
                OUTPUT( base64_encoding_table[byte_pair & 0x3f] );
            }
            /* Windows always explicitly terminates the base64 sequence
               even though RFC 2152 (page 3, rule 2) does not require this */
            OUTPUT( '-' );
        }
    }
    return pos;

overflow:
    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return 0;
#undef OUTPUT
#undef ENCODABLE
}


static int wcstombs_utf8( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen,
                          const char *defchar, BOOL *used )
{
    DWORD reslen;
    NTSTATUS status;

    if (defchar || used)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags & ~(WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR | WC_ERR_INVALID_CHARS |
                  WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!dstlen) dst = NULL;
    status = RtlUnicodeToUTF8N( dst, dstlen, &reslen, src, srclen * sizeof(WCHAR) );
    if (status == STATUS_SOME_NOT_MAPPED)
    {
        if (flags & WC_ERR_INVALID_CHARS)
        {
            SetLastError( ERROR_NO_UNICODE_TRANSLATION );
            return 0;
        }
    }
    else if (!set_ntstatus( status )) reslen = 0;
    return reslen;
}


static int wcstombs_sbcs( const CPTABLEINFO *info, const WCHAR *src, unsigned int srclen,
                          char *dst, unsigned int dstlen )
{
    const char *table = info->WideCharTable;
    int ret = srclen;

    if (!dstlen) return srclen;

    if (dstlen < srclen)
    {
        /* buffer too small: fill it up to dstlen and return error */
        srclen = dstlen;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        ret = 0;
    }

    while (srclen >= 16)
    {
        dst[0]  = table[src[0]];
        dst[1]  = table[src[1]];
        dst[2]  = table[src[2]];
        dst[3]  = table[src[3]];
        dst[4]  = table[src[4]];
        dst[5]  = table[src[5]];
        dst[6]  = table[src[6]];
        dst[7]  = table[src[7]];
        dst[8]  = table[src[8]];
        dst[9]  = table[src[9]];
        dst[10] = table[src[10]];
        dst[11] = table[src[11]];
        dst[12] = table[src[12]];
        dst[13] = table[src[13]];
        dst[14] = table[src[14]];
        dst[15] = table[src[15]];
        src += 16;
        dst += 16;
        srclen -= 16;
    }

    /* now handle remaining characters */
    src += srclen;
    dst += srclen;
    switch(srclen)
    {
    case 15: dst[-15] = table[src[-15]];
    case 14: dst[-14] = table[src[-14]];
    case 13: dst[-13] = table[src[-13]];
    case 12: dst[-12] = table[src[-12]];
    case 11: dst[-11] = table[src[-11]];
    case 10: dst[-10] = table[src[-10]];
    case 9:  dst[-9]  = table[src[-9]];
    case 8:  dst[-8]  = table[src[-8]];
    case 7:  dst[-7]  = table[src[-7]];
    case 6:  dst[-6]  = table[src[-6]];
    case 5:  dst[-5]  = table[src[-5]];
    case 4:  dst[-4]  = table[src[-4]];
    case 3:  dst[-3]  = table[src[-3]];
    case 2:  dst[-2]  = table[src[-2]];
    case 1:  dst[-1]  = table[src[-1]];
    case 0: break;
    }
    return ret;
}


static int wcstombs_dbcs( const CPTABLEINFO *info, const WCHAR *src, unsigned int srclen,
                          char *dst, unsigned int dstlen )
{
    const USHORT *table = info->WideCharTable;
    int i;

    if (!dstlen)
    {
        for (i = 0; srclen; src++, srclen--, i++) if (table[*src] & 0xff00) i++;
        return i;
    }

    for (i = dstlen; srclen && i; i--, srclen--, src++)
    {
        if (table[*src] & 0xff00)
        {
            if (i == 1) break;  /* do not output a partial char */
            i--;
            *dst++ = table[*src] >> 8;
        }
        *dst++ = (char)table[*src];
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static inline int is_valid_sbcs_mapping( const CPTABLEINFO *info, DWORD flags, unsigned int wch )
{
    const unsigned char *table = info->WideCharTable;

    if (wch >= 0x10000) return 0;
    if ((flags & WC_NO_BEST_FIT_CHARS) || table[wch] == info->DefaultChar)
        return (info->MultiByteTable[table[wch]] == wch);
    return 1;
}


static inline int is_valid_dbcs_mapping( const CPTABLEINFO *info, DWORD flags, unsigned int wch )
{
    const unsigned short *table = info->WideCharTable;
    unsigned short ch;

    if (wch >= 0x10000) return 0;
    ch = table[wch];
    if ((flags & WC_NO_BEST_FIT_CHARS) || ch == info->DefaultChar)
    {
        if (ch >> 8) return info->DBCSOffsets[info->DBCSOffsets[ch >> 8] + (ch & 0xff)] == wch;
        return info->MultiByteTable[ch] == wch;
    }
    return 1;
}


static int wcstombs_sbcs_slow( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen, const char *defchar, BOOL *used )
{
    const char *table = info->WideCharTable;
    const char def = defchar ? *defchar : (char)info->DefaultChar;
    int i;
    BOOL tmp;
    WCHAR wch;
    unsigned int composed;

    if (!used) used = &tmp;  /* avoid checking on every char */
    *used = FALSE;

    if (!dstlen)
    {
        for (i = 0; srclen; i++, src++, srclen--)
        {
            wch = *src;
            if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
            {
                /* now check if we can use the composed char */
                if (is_valid_sbcs_mapping( info, flags, composed ))
                {
                    /* we have a good mapping, use it */
                    src++;
                    srclen--;
                    continue;
                }
                /* no mapping for the composed char, check the other flags */
                if (flags & WC_DEFAULTCHAR) /* use the default char instead */
                {
                    *used = TRUE;
                    src++;  /* skip the non-spacing char */
                    srclen--;
                    continue;
                }
                if (flags & WC_DISCARDNS) /* skip the second char of the composition */
                {
                    src++;
                    srclen--;
                }
                /* WC_SEPCHARS is the default */
            }
            if (!*used) *used = !is_valid_sbcs_mapping( info, flags, wch );
        }
        return i;
    }

    for (i = dstlen; srclen && i; dst++, i--, src++, srclen--)
    {
        wch = *src;
        if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
        {
            /* now check if we can use the composed char */
            if (is_valid_sbcs_mapping( info, flags, composed ))
            {
                /* we have a good mapping, use it */
                *dst = table[composed];
                src++;
                srclen--;
                continue;
            }
            /* no mapping for the composed char, check the other flags */
            if (flags & WC_DEFAULTCHAR) /* use the default char instead */
            {
                *dst = def;
                *used = TRUE;
                src++;  /* skip the non-spacing char */
                srclen--;
                continue;
            }
            if (flags & WC_DISCARDNS) /* skip the second char of the composition */
            {
                src++;
                srclen--;
            }
            /* WC_SEPCHARS is the default */
        }

        *dst = table[wch];
        if (!is_valid_sbcs_mapping( info, flags, wch ))
        {
            *dst = def;
            *used = TRUE;
        }
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int wcstombs_dbcs_slow( const CPTABLEINFO *info, DWORD flags, const WCHAR *src, unsigned int srclen,
                               char *dst, unsigned int dstlen, const char *defchar, BOOL *used )
{
    const USHORT *table = info->WideCharTable;
    WCHAR wch, defchar_value;
    unsigned int composed;
    unsigned short res;
    BOOL tmp;
    int i;

    if (!defchar[1]) defchar_value = (unsigned char)defchar[0];
    else defchar_value = ((unsigned char)defchar[0] << 8) | (unsigned char)defchar[1];

    if (!used) used = &tmp;  /* avoid checking on every char */
    *used = FALSE;

    if (!dstlen)
    {
        if (!defchar && !used && !(flags & WC_COMPOSITECHECK))
        {
            for (i = 0; srclen; srclen--, src++, i++) if (table[*src] & 0xff00) i++;
            return i;
        }
        for (i = 0; srclen; srclen--, src++, i++)
        {
            wch = *src;
            if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
            {
                /* now check if we can use the composed char */
                if (is_valid_dbcs_mapping( info, flags, composed ))
                {
                    /* we have a good mapping for the composed char, use it */
                    res = table[composed];
                    if (res & 0xff00) i++;
                    src++;
                    srclen--;
                    continue;
                }
                /* no mapping for the composed char, check the other flags */
                if (flags & WC_DEFAULTCHAR) /* use the default char instead */
                {
                    if (defchar_value & 0xff00) i++;
                    *used = TRUE;
                    src++;  /* skip the non-spacing char */
                    srclen--;
                    continue;
                }
                if (flags & WC_DISCARDNS) /* skip the second char of the composition */
                {
                    src++;
                    srclen--;
                }
                /* WC_SEPCHARS is the default */
            }

            res = table[wch];
            if (!is_valid_dbcs_mapping( info, flags, wch ))
            {
                res = defchar_value;
                *used = TRUE;
            }
            if (res & 0xff00) i++;
        }
        return i;
    }


    for (i = dstlen; srclen && i; i--, srclen--, src++)
    {
        wch = *src;
        if ((flags & WC_COMPOSITECHECK) && (srclen > 1) && (composed = compose_chars( src[0], src[1] )))
        {
            /* now check if we can use the composed char */
            if (is_valid_dbcs_mapping( info, flags, composed ))
            {
                /* we have a good mapping for the composed char, use it */
                res = table[composed];
                src++;
                srclen--;
                goto output_char;
            }
            /* no mapping for the composed char, check the other flags */
            if (flags & WC_DEFAULTCHAR) /* use the default char instead */
            {
                res = defchar_value;
                *used = TRUE;
                src++;  /* skip the non-spacing char */
                srclen--;
                goto output_char;
            }
            if (flags & WC_DISCARDNS) /* skip the second char of the composition */
            {
                src++;
                srclen--;
            }
            /* WC_SEPCHARS is the default */
        }

        res = table[wch];
        if (!is_valid_dbcs_mapping( info, flags, wch ))
        {
            res = defchar_value;
            *used = TRUE;
        }

    output_char:
        if (res & 0xff00)
        {
            if (i == 1) break;  /* do not output a partial char */
            i--;
            *dst++ = res >> 8;
        }
        *dst++ = (char)res;
    }
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    return dstlen - i;
}


static int wcstombs_codepage( UINT codepage, DWORD flags, const WCHAR *src, int srclen,
                              char *dst, int dstlen, const char *defchar, BOOL *used )
{
    const CPTABLEINFO *info = get_codepage_table( codepage );

    if (!info)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (flags & ~(WC_DISCARDNS | WC_SEPCHARS | WC_DEFAULTCHAR | WC_ERR_INVALID_CHARS |
                  WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (flags || defchar || used)
    {
        if (!defchar) defchar = (const char *)&info->DefaultChar;
        if (info->DBCSOffsets)
            return wcstombs_dbcs_slow( info, flags, src, srclen, dst, dstlen, defchar, used );
        else
            return wcstombs_sbcs_slow( info, flags, src, srclen, dst, dstlen, defchar, used );
    }
    if (info->DBCSOffsets)
        return wcstombs_dbcs( info, src, srclen, dst, dstlen );
    else
        return wcstombs_sbcs( info, src, srclen, dst, dstlen );
}


static int get_sortkey( DWORD flags, const WCHAR *src, int srclen, char *dst, int dstlen )
{
    WCHAR dummy[4]; /* no decomposition is larger than 4 chars */
    int key_len[4];
    char *key_ptr[4];
    const WCHAR *src_save = src;
    int srclen_save = srclen;

    key_len[0] = key_len[1] = key_len[2] = key_len[3] = 0;
    for (; srclen; srclen--, src++)
    {
        unsigned int i, decomposed_len = 1;/*wine_decompose(*src, dummy, 4);*/
        dummy[0] = *src;
        if (decomposed_len)
        {
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                if ((flags & NORM_IGNORESYMBOLS) &&
                    (get_char_type( CT_CTYPE1, wch ) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = casemap( nls_info.LowerCaseTable, wch );

                ce = collation_table[collation_table[collation_table[wch >> 8] + ((wch >> 4) & 0x0f)] + (wch & 0xf)];
                if (ce != (unsigned int)-1)
                {
                    if (ce >> 16) key_len[0] += 2;
                    if ((ce >> 8) & 0xff) key_len[1]++;
                    if ((ce >> 4) & 0x0f) key_len[2]++;
                    if (ce & 1)
                    {
                        if (wch >> 8) key_len[3]++;
                        key_len[3]++;
                    }
                }
                else
                {
                    key_len[0] += 2;
                    if (wch >> 8) key_len[0]++;
                    if (wch & 0xff) key_len[0]++;
		}
            }
        }
    }

    if (!dstlen) /* compute length */
        /* 4 * '\1' + key length */
        return key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4;

    if (dstlen < key_len[0] + key_len[1] + key_len[2] + key_len[3] + 4 + 1)
        return 0; /* overflow */

    src = src_save;
    srclen = srclen_save;

    key_ptr[0] = dst;
    key_ptr[1] = key_ptr[0] + key_len[0] + 1;
    key_ptr[2] = key_ptr[1] + key_len[1] + 1;
    key_ptr[3] = key_ptr[2] + key_len[2] + 1;

    for (; srclen; srclen--, src++)
    {
        unsigned int i, decomposed_len = 1;/*wine_decompose(*src, dummy, 4);*/
        dummy[0] = *src;
        if (decomposed_len)
        {
            for (i = 0; i < decomposed_len; i++)
            {
                WCHAR wch = dummy[i];
                unsigned int ce;

                if ((flags & NORM_IGNORESYMBOLS) &&
                    (get_char_type( CT_CTYPE1, wch ) & (C1_PUNCT | C1_SPACE)))
                    continue;

                if (flags & NORM_IGNORECASE) wch = casemap( nls_info.LowerCaseTable, wch );

                ce = collation_table[collation_table[collation_table[wch >> 8] + ((wch >> 4) & 0x0f)] + (wch & 0xf)];
                if (ce != (unsigned int)-1)
                {
                    WCHAR key;
                    if ((key = ce >> 16))
                    {
                        *key_ptr[0]++ = key >> 8;
                        *key_ptr[0]++ = key & 0xff;
                    }
                    /* make key 1 start from 2 */
                    if ((key = (ce >> 8) & 0xff)) *key_ptr[1]++ = key + 1;
                    /* make key 2 start from 2 */
                    if ((key = (ce >> 4) & 0x0f)) *key_ptr[2]++ = key + 1;
                    /* key 3 is always a character code */
                    if (ce & 1)
                    {
                        if (wch >> 8) *key_ptr[3]++ = wch >> 8;
                        if (wch & 0xff) *key_ptr[3]++ = wch & 0xff;
                    }
                }
                else
                {
                    *key_ptr[0]++ = 0xff;
                    *key_ptr[0]++ = 0xfe;
                    if (wch >> 8) *key_ptr[0]++ = wch >> 8;
                    if (wch & 0xff) *key_ptr[0]++ = wch & 0xff;
                }
            }
        }
    }

    *key_ptr[0] = 1;
    *key_ptr[1] = 1;
    *key_ptr[2] = 1;
    *key_ptr[3]++ = 1;
    *key_ptr[3] = 0;
    return key_ptr[3] - dst;
}


/* compose a full-width katakana. return consumed source characters. */
static int compose_katakana( const WCHAR *src, int srclen, WCHAR *dst )
{
    static const BYTE katakana_map[] =
    {
        /* */ 0x02, 0x0c, 0x0d, 0x01, 0xfb, 0xf2, 0xa1, /* U+FF61- */
        0xa3, 0xa5, 0xa7, 0xa9, 0xe3, 0xe5, 0xe7, 0xc3, /* U+FF68- */
        0xfc, 0xa2, 0xa4, 0xa6, 0xa8, 0xaa, 0xab, 0xad, /* U+FF70- */
        0xaf, 0xb1, 0xb3, 0xb5, 0xb7, 0xb9, 0xbb, 0xbd, /* U+FF78- */
        0xbf, 0xc1, 0xc4, 0xc6, 0xc8, 0xca, 0xcb, 0xcc, /* U+FF80- */
        0xcd, 0xce, 0xcf, 0xd2, 0xd5, 0xd8, 0xdb, 0xde, /* U+FF88- */
        0xdf, 0xe0, 0xe1, 0xe2, 0xe4, 0xe6, 0xe8, 0xe9, /* U+FF90- */
        0xea, 0xeb, 0xec, 0xed, 0xef, 0xf3, 0x99, 0x9a, /* U+FF98- */
    };
    WCHAR dummy;
    int shift;

    if (!dst) dst = &dummy;

    switch (*src)
    {
    case 0x309b:
    case 0x309c:
        *dst = *src - 2;
        return 1;
    case 0x30f0:
    case 0x30f1:
    case 0x30fd:
        *dst = *src;
        break;
    default:
        shift = *src - 0xff61;
        if (shift < 0 || shift >= ARRAY_SIZE( katakana_map )) return 0;
        *dst = katakana_map[shift] | 0x3000;
        break;
    }

    if (srclen <= 1) return 1;

    switch (src[1])
    {
    case 0xff9e:  /* datakuten (voiced sound) */
        if ((*src >= 0xff76 && *src <= 0xff84) || (*src >= 0xff8a && *src <= 0xff8e) || *src == 0x30fd)
            *dst += 1;
        else if (*src == 0xff73)
            *dst = 0x30f4; /* KATAKANA LETTER VU */
        else if (*src == 0xff9c)
            *dst = 0x30f7; /* KATAKANA LETTER VA */
        else if (*src == 0x30f0)
            *dst = 0x30f8; /* KATAKANA LETTER VI */
        else if (*src == 0x30f1)
            *dst = 0x30f9; /* KATAKANA LETTER VE */
        else if (*src == 0xff66)
            *dst = 0x30fa; /* KATAKANA LETTER VO */
        else
            return 1;
        break;
    case 0xff9f:  /* handakuten (semi-voiced sound) */
        if (*src >= 0xff8a && *src <= 0xff8e)
            *dst += 2;
        else
            return 1;
        break;
    default:
        return 1;
    }
    return 2;
}

/* map one or two half-width characters to one full-width character */
static int map_to_fullwidth( const WCHAR *src, int srclen, WCHAR *dst )
{
    INT n;

    if (*src <= '~' && *src > ' ' && *src != '\\')
        *dst = *src - 0x20 + 0xff00;
    else if (*src == ' ')
        *dst = 0x3000;
    else if (*src <= 0x00af && *src >= 0x00a2)
    {
        static const BYTE misc_symbols_table[] =
        {
            0xe0, 0xe1, 0x00, 0xe5, 0xe4, 0x00, 0x00, /* U+00A2- */
            0x00, 0x00, 0x00, 0xe2, 0x00, 0x00, 0xe3  /* U+00A9- */
        };
        if (misc_symbols_table[*src - 0x00a2])
            *dst = misc_symbols_table[*src - 0x00a2] | 0xff00;
        else
            *dst = *src;
    }
    else if (*src == 0x20a9) /* WON SIGN */
        *dst = 0xffe6;
    else if ((n = compose_katakana(src, srclen, dst)) > 0)
        return n;
    else if (*src >= 0xffa0 && *src <= 0xffdc)
    {
        static const BYTE hangul_mapping_table[] =
        {
            0x64, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,  /* U+FFA0- */
            0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,  /* U+FFA8- */
            0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47,  /* U+FFB0- */
            0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x00,  /* U+FFB8- */
            0x00, 0x00, 0x4f, 0x50, 0x51, 0x52, 0x53, 0x54,  /* U+FFC0- */
            0x00, 0x00, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a,  /* U+FFC8- */
            0x00, 0x00, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60,  /* U+FFD0- */
            0x00, 0x00, 0x61, 0x62, 0x63                     /* U+FFD8- */
        };

        if (hangul_mapping_table[*src - 0xffa0])
            *dst = hangul_mapping_table[*src - 0xffa0] | 0x3100;
        else
            *dst = *src;
    }
    else
        *dst = *src;

    return 1;
}

/* decompose a full-width katakana character into one or two half-width characters. */
static int decompose_katakana( WCHAR c, WCHAR *dst, int dstlen )
{
    static const BYTE katakana_map[] =
    {
        /* */ 0x9e, 0x9f, 0x9e, 0x9f, 0x00, 0x00, 0x00, /* U+3099- */
        0x00, 0x67, 0x71, 0x68, 0x72, 0x69, 0x73, 0x6a, /* U+30a1- */
        0x74, 0x6b, 0x75, 0x76, 0x01, 0x77, 0x01, 0x78, /* U+30a8- */
        0x01, 0x79, 0x01, 0x7a, 0x01, 0x7b, 0x01, 0x7c, /* U+30b0- */
        0x01, 0x7d, 0x01, 0x7e, 0x01, 0x7f, 0x01, 0x80, /* U+30b8- */
        0x01, 0x81, 0x01, 0x6f, 0x82, 0x01, 0x83, 0x01, /* U+30c0- */
        0x84, 0x01, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, /* U+30c8- */
        0x01, 0x02, 0x8b, 0x01, 0x02, 0x8c, 0x01, 0x02, /* U+30d0- */
        0x8d, 0x01, 0x02, 0x8e, 0x01, 0x02, 0x8f, 0x90, /* U+30d8- */
        0x91, 0x92, 0x93, 0x6c, 0x94, 0x6d, 0x95, 0x6e, /* U+30e0- */
        0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x00, 0x9c, /* U+30e8- */
        0x00, 0x00, 0x66, 0x9d, 0x4e, 0x00, 0x00, 0x08, /* U+30f0- */
        0x58, 0x58, 0x08, 0x65, 0x70, 0x00, 0x51        /* U+30f8- */
    };
    int len = 0, shift = c - 0x3099;
    BYTE k;

    if (shift < 0 || shift >= ARRAY_SIZE( katakana_map )) return 0;

    k = katakana_map[shift];
    if (!k)
    {
        if (dstlen > 0) *dst = c;
        len++;
    }
    else if (k > 0x60)
    {
        if (dstlen > 0) *dst = k | 0xff00;
        len++;
    }
    else
    {
        if (dstlen >= 2)
        {
            dst[0] = (k > 0x50) ? (c - (k & 0xf)) : (katakana_map[shift - k] | 0xff00);
            dst[1] = (k == 2) ? 0xff9f : 0xff9e;
        }
        len += 2;
    }
    return len;
}

/* map single full-width character to single or double half-width characters. */
static int map_to_halfwidth( WCHAR c, WCHAR *dst, int dstlen )
{
    int n = decompose_katakana( c, dst, dstlen );
    if (n > 0) return n;

    if (c == 0x3000)
        *dst = ' ';
    else if (c == 0x3001)
        *dst = 0xff64;
    else if (c == 0x3002)
        *dst = 0xff61;
    else if (c == 0x300c || c == 0x300d)
        *dst = (c - 0x300c) + 0xff62;
    else if (c >= 0x3131 && c <= 0x3163)
    {
        *dst = c - 0x3131 + 0xffa1;
        if (*dst >= 0xffbf) *dst += 3;
        if (*dst >= 0xffc8) *dst += 2;
        if (*dst >= 0xffd0) *dst += 2;
        if (*dst >= 0xffd8) *dst += 2;
    }
    else if (c == 0x3164)
        *dst = 0xffa0;
    else if (c == 0x2019)
        *dst = '\'';
    else if (c == 0x201d)
        *dst = '"';
    else if (c > 0xff00 && c < 0xff5f && c != 0xff3c)
        *dst = c - 0xff00 + 0x20;
    else if (c >= 0xffe0 && c <= 0xffe6)
    {
        static const WCHAR misc_symbol_map[] = { 0x00a2, 0x00a3, 0x00ac, 0x00af, 0x00a6, 0x00a5, 0x20a9 };
        *dst = misc_symbol_map[c - 0xffe0];
    }
    else
        *dst = c;

    return 1;
}


/* 32-bit collation element table format:
 * unicode weight - high 16 bit, diacritic weight - high 8 bit of low 16 bit,
 * case weight - high 4 bit of low 8 bit.
 */

enum weight { UNICODE_WEIGHT, DIACRITIC_WEIGHT, CASE_WEIGHT };

static unsigned int get_weight( WCHAR ch, enum weight type )
{
    unsigned int ret;

    ret = collation_table[collation_table[collation_table[ch >> 8] + ((ch >> 4) & 0x0f)] + (ch & 0xf)];
    if (ret == ~0u) return ch;

    switch (type)
    {
    case UNICODE_WEIGHT:   return ret >> 16;
    case DIACRITIC_WEIGHT: return (ret >> 8) & 0xff;
    case CASE_WEIGHT:      return (ret >> 4) & 0x0f;
    default:               return 0;
    }
}


static void inc_str_pos( const WCHAR **str, int *len, unsigned int *dpos, unsigned int *dlen )
{
    (*dpos)++;
    if (*dpos == *dlen)
    {
        *dpos = *dlen = 0;
        (*str)++;
        (*len)--;
    }
}


static int compare_weights(int flags, const WCHAR *str1, int len1,
                           const WCHAR *str2, int len2, enum weight type )
{
    unsigned int ce1, ce2, dpos1 = 0, dpos2 = 0, dlen1 = 0, dlen2 = 0;
    const WCHAR *dstr1 = NULL, *dstr2 = NULL;

    while (len1 > 0 && len2 > 0)
    {
        if (!dlen1 && !(dstr1 = get_decomposition( *str1, &dlen1 ))) dstr1 = str1;
        if (!dlen2 && !(dstr2 = get_decomposition( *str2, &dlen2 ))) dstr2 = str2;

        if (flags & NORM_IGNORESYMBOLS)
        {
            int skip = 0;
            /* FIXME: not tested */
            if (get_char_type( CT_CTYPE1, dstr1[dpos1] ) & (C1_PUNCT | C1_SPACE))
            {
                inc_str_pos( &str1, &len1, &dpos1, &dlen1 );
                skip = 1;
            }
            if (get_char_type( CT_CTYPE1, dstr2[dpos2] ) & (C1_PUNCT | C1_SPACE))
            {
                inc_str_pos( &str2, &len2, &dpos2, &dlen2 );
                skip = 1;
            }
            if (skip) continue;
        }

       /* hyphen and apostrophe are treated differently depending on
        * whether SORT_STRINGSORT specified or not
        */
        if (type == UNICODE_WEIGHT && !(flags & SORT_STRINGSORT))
        {
            if (dstr1[dpos1] == '-' || dstr1[dpos1] == '\'')
            {
                if (dstr2[dpos2] != '-' && dstr2[dpos2] != '\'')
                {
                    inc_str_pos( &str1, &len1, &dpos1, &dlen1 );
                    continue;
                }
            }
            else if (dstr2[dpos2] == '-' || dstr2[dpos2] == '\'')
            {
                inc_str_pos( &str2, &len2, &dpos2, &dlen2 );
                continue;
            }
        }

        ce1 = get_weight( dstr1[dpos1], type );
        if (!ce1)
        {
            inc_str_pos( &str1, &len1, &dpos1, &dlen1 );
            continue;
        }
        ce2 = get_weight( dstr2[dpos2], type );
        if (!ce2)
        {
            inc_str_pos( &str2, &len2, &dpos2, &dlen2 );
            continue;
        }

        if (ce1 - ce2) return ce1 - ce2;

        inc_str_pos( &str1, &len1, &dpos1, &dlen1 );
        inc_str_pos( &str2, &len2, &dpos2, &dlen2 );
    }
    while (len1)
    {
        if (!dlen1 && !(dstr1 = get_decomposition( *str1, &dlen1 ))) dstr1 = str1;
        ce1 = get_weight( dstr1[dpos1], type );
        if (ce1) break;
        inc_str_pos( &str1, &len1, &dpos1, &dlen1 );
    }
    while (len2)
    {
        if (!dlen2 && !(dstr2 = get_decomposition( *str2, &dlen2 ))) dstr2 = str2;
        ce2 = get_weight( dstr2[dpos2], type );
        if (ce2) break;
        inc_str_pos( &str2, &len2, &dpos2, &dlen2 );
    }
    return len1 - len2;
}


static const struct geoinfo *get_geoinfo_ptr( GEOID geoid )
{
    int min = 0, max = ARRAY_SIZE( geoinfodata )-1;

    while (min <= max)
    {
        int n = (min + max)/2;
        const struct geoinfo *ptr = &geoinfodata[n];
        if (geoid == ptr->id) /* we don't need empty entries */
            return *ptr->iso2W ? ptr : NULL;
        if (ptr->id > geoid) max = n-1;
        else min = n+1;
    }
    return NULL;
}


static int compare_tzdate( const TIME_FIELDS *tf, const SYSTEMTIME *compare )
{
    static const int month_lengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    int first, last, limit, dayinsecs;

    if (tf->Month < compare->wMonth) return -1; /* We are in a month before the date limit. */
    if (tf->Month > compare->wMonth) return 1; /* We are in a month after the date limit. */

    /* if year is 0 then date is in day-of-week format, otherwise
     * it's absolute date.
     */
    if (!compare->wYear)
    {
        /* wDay is interpreted as number of the week in the month
         * 5 means: the last week in the month */
        /* calculate the day of the first DayOfWeek in the month */
        first = (6 + compare->wDayOfWeek - tf->Weekday + tf->Day) % 7 + 1;
        /* check needed for the 5th weekday of the month */
        last = month_lengths[tf->Month - 1] +
            (tf->Month == 2 && (!(tf->Year % 4) && (tf->Year % 100 || !(tf->Year % 400))));
        limit = first + 7 * (compare->wDay - 1);
        if (limit > last) limit -= 7;
    }
    else limit = compare->wDay;

    limit = ((limit * 24 + compare->wHour) * 60 + compare->wMinute) * 60;
    dayinsecs = ((tf->Day * 24  + tf->Hour) * 60 + tf->Minute) * 60 + tf->Second;
    return dayinsecs - limit;
}


static DWORD get_timezone_id( const TIME_ZONE_INFORMATION *info, LARGE_INTEGER time, BOOL is_local )
{
    int year;
    BOOL before_standard_date, after_daylight_date;
    LARGE_INTEGER t2;
    TIME_FIELDS tf;

    if (!info->DaylightDate.wMonth) return TIME_ZONE_ID_UNKNOWN;

    /* if year is 0 then date is in day-of-week format, otherwise it's absolute date */
    if (info->StandardDate.wMonth == 0 ||
        (info->StandardDate.wYear == 0 &&
         (info->StandardDate.wDay < 1 || info->StandardDate.wDay > 5 ||
          info->DaylightDate.wDay < 1 || info->DaylightDate.wDay > 5)))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return TIME_ZONE_ID_INVALID;
    }

    if (!is_local) time.QuadPart -= info->Bias * (LONGLONG)600000000;
    RtlTimeToTimeFields( &time, &tf );
    year = tf.Year;
    if (!is_local)
    {
        t2.QuadPart = time.QuadPart - info->DaylightBias * (LONGLONG)600000000;
        RtlTimeToTimeFields( &t2, &tf );
    }
    if (tf.Year == year)
        before_standard_date = compare_tzdate( &tf, &info->StandardDate ) < 0;
    else
        before_standard_date = tf.Year < year;

    if (!is_local)
    {
        t2.QuadPart = time.QuadPart - info->StandardBias * (LONGLONG)600000000;
        RtlTimeToTimeFields( &t2, &tf );
    }
    if (tf.Year == year)
        after_daylight_date = compare_tzdate( &tf, &info->DaylightDate ) >= 0;
    else
        after_daylight_date = tf.Year > year;

    if (info->DaylightDate.wMonth < info->StandardDate.wMonth) /* Northern hemisphere */
    {
        if (before_standard_date && after_daylight_date) return TIME_ZONE_ID_DAYLIGHT;
    }
    else /* Down south */
    {
        if (before_standard_date || after_daylight_date) return TIME_ZONE_ID_DAYLIGHT;
    }
    return TIME_ZONE_ID_STANDARD;
}


/* Note: the Internal_ functions are not documented. The number of parameters
 * should be correct, but their exact meaning may not.
 */

/******************************************************************************
 *	Internal_EnumCalendarInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumCalendarInfo( CALINFO_ENUMPROCW proc, LCID lcid, CALID id,
                                                         CALTYPE type, BOOL unicode, BOOL ex,
                                                         BOOL exex, LPARAM lparam )
{
    WCHAR buffer[256];
    CALID calendars[2] = { id };
    INT ret, i;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (id == ENUM_ALL_CALENDARS)
    {
        if (!GetLocaleInfoW( lcid, LOCALE_ICALENDARTYPE | LOCALE_RETURN_NUMBER,
                             (WCHAR *)&calendars[0], sizeof(calendars[0]) / sizeof(WCHAR) )) return FALSE;
        if (!GetLocaleInfoW( lcid, LOCALE_IOPTIONALCALENDAR | LOCALE_RETURN_NUMBER,
                             (WCHAR *)&calendars[1], sizeof(calendars[1]) / sizeof(WCHAR) )) calendars[1] = 0;
    }

    for (i = 0; i < ARRAY_SIZE(calendars) && calendars[i]; i++)
    {
        id = calendars[i];
        if (type & CAL_RETURN_NUMBER)
            ret = GetCalendarInfoW( lcid, id, type, NULL, 0, (LPDWORD)buffer );
        else if (unicode)
            ret = GetCalendarInfoW( lcid, id, type, buffer, ARRAY_SIZE(buffer), NULL );
        else
        {
            WCHAR bufW[256];
            ret = GetCalendarInfoW( lcid, id, type, bufW, ARRAY_SIZE(bufW), NULL );
            if (ret) WideCharToMultiByte( CP_ACP, 0, bufW, -1, (char *)buffer, sizeof(buffer), NULL, NULL );
        }

        if (ret)
        {
            if (exex) ret = ((CALINFO_ENUMPROCEXEX)proc)( buffer, id, NULL, lparam );
            else if (ex) ret = ((CALINFO_ENUMPROCEXW)proc)( buffer, id );
            else ret = proc( buffer );
        }
        if (!ret) break;
    }
    return TRUE;
}


/**************************************************************************
 *	Internal_EnumDateFormats   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumDateFormats( DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags,
                                                        BOOL unicode, BOOL ex, BOOL exex, LPARAM lparam )
{
    WCHAR buffer[256];
    LCTYPE lctype;
    CALID cal_id;
    INT ret;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!GetLocaleInfoW( lcid, LOCALE_ICALENDARTYPE|LOCALE_RETURN_NUMBER,
                         (LPWSTR)&cal_id, sizeof(cal_id)/sizeof(WCHAR) ))
        return FALSE;

    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
    case DATE_SHORTDATE:
        lctype = LOCALE_SSHORTDATE;
        break;
    case DATE_LONGDATE:
        lctype = LOCALE_SLONGDATE;
        break;
    case DATE_YEARMONTH:
        lctype = LOCALE_SYEARMONTH;
        break;
    default:
        FIXME( "unknown date format 0x%08lx\n", flags );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lctype |= flags & LOCALE_USE_CP_ACP;
    if (unicode)
        ret = GetLocaleInfoW( lcid, lctype, buffer, ARRAY_SIZE(buffer) );
    else
        ret = GetLocaleInfoA( lcid, lctype, (char *)buffer, sizeof(buffer) );

    if (ret)
    {
        if (exex) ((DATEFMT_ENUMPROCEXEX)proc)( buffer, cal_id, lparam );
        else if (ex) ((DATEFMT_ENUMPROCEXW)proc)( buffer, cal_id );
        else proc( buffer );
    }
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumLanguageGroupLocales   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumLanguageGroupLocales( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                                DWORD flags, LONG_PTR param, BOOL unicode )
{
    WCHAR name[10], value[10];
    DWORD name_len, value_len, type, index = 0, alt = 0;
    HKEY key, altkey;
    LCID lcid;

    if (!proc || id < LGRPID_WESTERN_EUROPE || id > LGRPID_ARMENIAN)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (RegOpenKeyExW( nls_key, L"Locale", 0, KEY_READ, &key )) return FALSE;
    if (RegOpenKeyExW( key, L"Alternate Sorts", 0, KEY_READ, &altkey )) altkey = 0;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        if (RegEnumValueW( alt ? altkey : key, index++, name, &name_len, NULL,
                           &type, (BYTE *)value, &value_len ))
        {
            if (alt++) break;
            index = 0;
            continue;
        }
        if (type != REG_SZ) continue;
        if (id != wcstoul( value, NULL, 16 )) continue;
        lcid = wcstoul( name, NULL, 16 );
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((LANGGROUPLOCALE_ENUMPROCA)proc)( id, lcid, nameA, param )) break;
        }
        else if (!proc( id, lcid, name, param )) break;
    }
    RegCloseKey( altkey );
    RegCloseKey( key );
    return TRUE;
}


/***********************************************************************
 *	Internal_EnumSystemCodePages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumSystemCodePages( CODEPAGE_ENUMPROCW proc, DWORD flags,
                                                            BOOL unicode )
{
    WCHAR name[10];
    DWORD name_len, type, index = 0;
    HKEY key;

    if (RegOpenKeyExW( nls_key, L"Codepage", 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, NULL, NULL )) break;
        if (type != REG_SZ) continue;
        if (!wcstoul( name, NULL, 10 )) continue;
        if (!unicode)
        {
            char nameA[10];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            if (!((CODEPAGE_ENUMPROCA)proc)( nameA )) break;
        }
        else if (!proc( name )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumSystemLanguageGroups   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumSystemLanguageGroups( LANGUAGEGROUP_ENUMPROCW proc,
                                                                DWORD flags, LONG_PTR param, BOOL unicode )
{
    WCHAR name[10], value[10], descr[80];
    DWORD name_len, value_len, type, index = 0;
    HKEY key;
    LGRPID id;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (flags)
    {
    case 0:
        flags = LGRPID_INSTALLED;
        break;
    case LGRPID_INSTALLED:
    case LGRPID_SUPPORTED:
        break;
    default:
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    if (RegOpenKeyExW( nls_key, L"Language Groups", 0, KEY_READ, &key )) return FALSE;

    for (;;)
    {
        name_len = ARRAY_SIZE(name);
        value_len = sizeof(value);
        if (RegEnumValueW( key, index++, name, &name_len, NULL, &type, (BYTE *)value, &value_len )) break;
        if (type != REG_SZ) continue;

        id = wcstoul( name, NULL, 16 );

        if (!(flags & LGRPID_SUPPORTED) && !wcstoul( value, NULL, 10 )) continue;
        if (!LoadStringW( kernel32_handle, 0x2000 + id, descr, ARRAY_SIZE(descr) )) descr[0] = 0;
        TRACE( "%p: %lu %s %s %lx %Ix\n", proc, id, debugstr_w(name), debugstr_w(descr), flags, param );
        if (!unicode)
        {
            char nameA[10], descrA[80];
            WideCharToMultiByte( CP_ACP, 0, name, -1, nameA, sizeof(nameA), NULL, NULL );
            WideCharToMultiByte( CP_ACP, 0, descr, -1, descrA, sizeof(descrA), NULL, NULL );
            if (!((LANGUAGEGROUP_ENUMPROCA)proc)( id, nameA, descrA, flags, param )) break;
        }
        else if (!proc( id, name, descr, flags, param )) break;
    }
    RegCloseKey( key );
    return TRUE;
}


/**************************************************************************
 *	Internal_EnumTimeFormats   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumTimeFormats( TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags,
                                                        BOOL unicode, BOOL ex, LPARAM lparam )
{
    WCHAR buffer[256];
    LCTYPE lctype;
    INT ret;

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (flags & ~LOCALE_USE_CP_ACP)
    {
    case 0:
        lctype = LOCALE_STIMEFORMAT;
        break;
    case TIME_NOSECONDS:
        lctype = LOCALE_SSHORTTIME;
        break;
    default:
        FIXME( "Unknown time format %lx\n", flags );
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    lctype |= flags & LOCALE_USE_CP_ACP;
    if (unicode)
        ret = GetLocaleInfoW( lcid, lctype, buffer, ARRAY_SIZE(buffer) );
    else
        ret = GetLocaleInfoA( lcid, lctype, (char *)buffer, sizeof(buffer) );

    if (ret)
    {
        if (ex) ((TIMEFMT_ENUMPROCEX)proc)( buffer, lparam );
        else proc( buffer );
    }
    return TRUE;
}


/******************************************************************************
 *	Internal_EnumUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH Internal_EnumUILanguages( UILANGUAGE_ENUMPROCW proc, DWORD flags,
                                                        LONG_PTR param, BOOL unicode )
{
    WCHAR nameW[LOCALE_NAME_MAX_LENGTH];
    char nameA[LOCALE_NAME_MAX_LENGTH];
    DWORD i;

    if (!proc)
    {
	SetLastError( ERROR_INVALID_PARAMETER );
	return FALSE;
    }
    if (flags & ~(MUI_LANGUAGE_ID | MUI_LANGUAGE_NAME))
    {
	SetLastError( ERROR_INVALID_FLAGS );
	return FALSE;
    }

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        if (!lcnames_index[i].name) continue;  /* skip invariant locale */
        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        if (!get_locale_data( lcnames_index[i].idx )->inotneutral) continue;  /* skip neutral locales */
        if (!SORTIDFROMLCID( lcnames_index[i].id ) != !(flags & LCID_ALTERNATE_SORTS))
            continue;  /* skip alternate sorts */
        if (flags & MUI_LANGUAGE_NAME)
        {
            const WCHAR *str = locale_strings + lcnames_index[i].name;

            if (unicode)
            {
                memcpy( nameW, str + 1, (*str + 1) * sizeof(WCHAR) );
                if (!proc( nameW, param )) break;
            }
            else
            {
                WideCharToMultiByte( CP_ACP, 0, str + 1, -1, nameA, sizeof(nameA), NULL, NULL );
                if (!((UILANGUAGE_ENUMPROCA)proc)( nameA, param )) break;
            }
        }
        else
        {
            if (lcnames_index[i].id == LOCALE_CUSTOM_UNSPECIFIED) continue;  /* skip locales with no lcid */
            if (unicode)
            {
                swprintf( nameW, ARRAY_SIZE(nameW), L"%04lx", lcnames_index[i].id );
                if (!proc( nameW, param )) break;
            }
            else
            {
                sprintf( nameA, "%04x", lcnames_index[i].id );
                if (!((UILANGUAGE_ENUMPROCA)proc)( nameA, param )) break;
            }
        }
    }
    return TRUE;
}


/******************************************************************************
 *	CompareStringEx   (kernelbase.@)
 */
INT WINAPI CompareStringEx( const WCHAR *locale, DWORD flags, const WCHAR *str1, int len1,
                            const WCHAR *str2, int len2, NLSVERSIONINFO *version,
                            void *reserved, LPARAM handle )
{
    DWORD supported_flags = NORM_IGNORECASE | NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS | SORT_STRINGSORT |
                            NORM_IGNOREKANATYPE | NORM_IGNOREWIDTH | LOCALE_USE_CP_ACP;
    DWORD semistub_flags = NORM_LINGUISTIC_CASING | LINGUISTIC_IGNORECASE | LINGUISTIC_IGNOREDIACRITIC |
                           SORT_DIGITSASNUMBERS | 0x10000000;
    /* 0x10000000 is related to diacritics in Arabic, Japanese, and Hebrew */
    INT ret;
    static int once;

    if (version) FIXME( "unexpected version parameter\n" );
    if (reserved) FIXME( "unexpected reserved value\n" );
    if (handle) FIXME( "unexpected handle\n" );

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (flags & ~(supported_flags | semistub_flags))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (flags & semistub_flags)
    {
        if (!once++) FIXME( "semi-stub behavior for flag(s) 0x%lx\n", flags & semistub_flags );
    }

    if (len1 < 0) len1 = lstrlenW(str1);
    if (len2 < 0) len2 = lstrlenW(str2);

    ret = compare_weights( flags, str1, len1, str2, len2, UNICODE_WEIGHT );
    if (!ret)
    {
        if (!(flags & NORM_IGNORENONSPACE))
            ret = compare_weights( flags, str1, len1, str2, len2, DIACRITIC_WEIGHT );
        if (!ret && !(flags & NORM_IGNORECASE))
            ret = compare_weights( flags, str1, len1, str2, len2, CASE_WEIGHT );
    }
    if (!ret) return CSTR_EQUAL;
    return (ret < 0) ? CSTR_LESS_THAN : CSTR_GREATER_THAN;
}


/******************************************************************************
 *	CompareStringA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringA( LCID lcid, DWORD flags, const char *str1, int len1,
                                             const char *str2, int len2 )
{
    WCHAR *buf1W = NtCurrentTeb()->StaticUnicodeBuffer;
    WCHAR *buf2W = buf1W + 130;
    LPWSTR str1W, str2W;
    INT len1W = 0, len2W = 0, ret;
    UINT locale_cp = CP_ACP;

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (flags & SORT_DIGITSASNUMBERS)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (len1 < 0) len1 = strlen(str1);
    if (len2 < 0) len2 = strlen(str2);

    locale_cp = get_lcid_codepage( lcid, flags );
    if (len1)
    {
        if (len1 <= 130) len1W = MultiByteToWideChar( locale_cp, 0, str1, len1, buf1W, 130 );
        if (len1W) str1W = buf1W;
        else
        {
            len1W = MultiByteToWideChar( locale_cp, 0, str1, len1, NULL, 0 );
            str1W = HeapAlloc( GetProcessHeap(), 0, len1W * sizeof(WCHAR) );
            if (!str1W)
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return 0;
            }
            MultiByteToWideChar( locale_cp, 0, str1, len1, str1W, len1W );
        }
    }
    else
    {
        len1W = 0;
        str1W = buf1W;
    }

    if (len2)
    {
        if (len2 <= 130) len2W = MultiByteToWideChar( locale_cp, 0, str2, len2, buf2W, 130 );
        if (len2W) str2W = buf2W;
        else
        {
            len2W = MultiByteToWideChar( locale_cp, 0, str2, len2, NULL, 0 );
            str2W = HeapAlloc( GetProcessHeap(), 0, len2W * sizeof(WCHAR) );
            if (!str2W)
            {
                if (str1W != buf1W) HeapFree( GetProcessHeap(), 0, str1W );
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                return 0;
            }
            MultiByteToWideChar( locale_cp, 0, str2, len2, str2W, len2W );
        }
    }
    else
    {
        len2W = 0;
        str2W = buf2W;
    }

    ret = CompareStringW( lcid, flags, str1W, len1W, str2W, len2W );

    if (str1W != buf1W) HeapFree( GetProcessHeap(), 0, str1W );
    if (str2W != buf2W) HeapFree( GetProcessHeap(), 0, str2W );
    return ret;
}


/******************************************************************************
 *	CompareStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringW( LCID lcid, DWORD flags, const WCHAR *str1, int len1,
                                             const WCHAR *str2, int len2 )
{
    return CompareStringEx( NULL, flags, str1, len1, str2, len2, NULL, NULL, 0 );
}


/******************************************************************************
 *	CompareStringOrdinal   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH CompareStringOrdinal( const WCHAR *str1, INT len1,
                                                   const WCHAR *str2, INT len2, BOOL ignore_case )
{
    int ret;

    if (!str1 || !str2)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (len1 < 0) len1 = lstrlenW( str1 );
    if (len2 < 0) len2 = lstrlenW( str2 );

    ret = RtlCompareUnicodeStrings( str1, len1, str2, len2, ignore_case );
    if (ret < 0) return CSTR_LESS_THAN;
    if (ret > 0) return CSTR_GREATER_THAN;
    return CSTR_EQUAL;
}


/******************************************************************************
 *	ConvertDefaultLocale   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH ConvertDefaultLocale( LCID lcid )
{
    get_locale_by_id( &lcid, 0 );
    return lcid;
}


/******************************************************************************
 *	EnumCalendarInfoW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoW( CALINFO_ENUMPROCW proc, LCID lcid,
                                                 CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( proc, lcid, id, type, TRUE, FALSE, FALSE, 0 );
}


/******************************************************************************
 *	EnumCalendarInfoExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoExW( CALINFO_ENUMPROCEXW proc, LCID lcid,
                                                   CALID id, CALTYPE type )
{
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, lcid, id, type, TRUE, TRUE, FALSE, 0 );
}

/******************************************************************************
 *	EnumCalendarInfoExEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumCalendarInfoExEx( CALINFO_ENUMPROCEXEX proc, LPCWSTR locale, CALID id,
                                                    LPCWSTR reserved, CALTYPE type, LPARAM lparam )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return Internal_EnumCalendarInfo( (CALINFO_ENUMPROCW)proc, lcid, id, type, TRUE, TRUE, TRUE, lparam );
}


/**************************************************************************
 *	EnumDateFormatsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsW( DATEFMT_ENUMPROCW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumDateFormats( proc, lcid, flags, TRUE, FALSE, FALSE, 0 );
}


/**************************************************************************
 *	EnumDateFormatsExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsExW( DATEFMT_ENUMPROCEXW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, lcid, flags, TRUE, TRUE, FALSE, 0 );
}


/**************************************************************************
 *	EnumDateFormatsExEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumDateFormatsExEx( DATEFMT_ENUMPROCEXEX proc, const WCHAR *locale,
                                                   DWORD flags, LPARAM lparam )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return Internal_EnumDateFormats( (DATEFMT_ENUMPROCW)proc, lcid, flags, TRUE, TRUE, TRUE, lparam );
}



/******************************************************************************
 *	EnumDynamicTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH EnumDynamicTimeZoneInformation( DWORD index,
                                                               DYNAMIC_TIME_ZONE_INFORMATION *info )
{
    DYNAMIC_TIME_ZONE_INFORMATION tz;
    LSTATUS ret;
    DWORD size;

    if (!info) return ERROR_INVALID_PARAMETER;

    size = ARRAY_SIZE(tz.TimeZoneKeyName);
    ret = RegEnumKeyExW( tz_key, index, tz.TimeZoneKeyName, &size, NULL, NULL, NULL, NULL );
    if (ret) return ret;

    tz.DynamicDaylightTimeDisabled = TRUE;
    if (!GetTimeZoneInformationForYear( 0, &tz, (TIME_ZONE_INFORMATION *)info )) return GetLastError();

    lstrcpyW( info->TimeZoneKeyName, tz.TimeZoneKeyName );
    info->DynamicDaylightTimeDisabled = FALSE;
    return 0;
}


/******************************************************************************
 *	EnumLanguageGroupLocalesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumLanguageGroupLocalesW( LANGGROUPLOCALE_ENUMPROCW proc, LGRPID id,
                                                         DWORD flags, LONG_PTR param )
{
    return Internal_EnumLanguageGroupLocales( proc, id, flags, param, TRUE );
}


/******************************************************************************
 *	EnumUILanguagesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumUILanguagesW( UILANGUAGE_ENUMPROCW proc, DWORD flags, LONG_PTR param )
{
    return Internal_EnumUILanguages( proc, flags, param, TRUE );
}


/***********************************************************************
 *	EnumSystemCodePagesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemCodePagesW( CODEPAGE_ENUMPROCW proc, DWORD flags )
{
    return Internal_EnumSystemCodePages( proc, flags, TRUE );
}


/******************************************************************************
 *	EnumSystemGeoID   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemGeoID( GEOCLASS class, GEOID parent, GEO_ENUMPROC proc )
{
    INT i;

    TRACE( "(%ld, %ld, %p)\n", class, parent, proc );

    if (!proc)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (class != GEOCLASS_NATION && class != GEOCLASS_REGION && class != GEOCLASS_ALL)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    for (i = 0; i < ARRAY_SIZE(geoinfodata); i++)
    {
        const struct geoinfo *ptr = &geoinfodata[i];

        if (class == GEOCLASS_NATION && (ptr->kind != LOCATION_NATION)) continue;
        /* LOCATION_BOTH counts as region */
        if (class == GEOCLASS_REGION && (ptr->kind == LOCATION_NATION)) continue;
        if (parent && ptr->parent != parent) continue;
        if (!proc( ptr->id )) break;
    }
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLanguageGroupsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLanguageGroupsW( LANGUAGEGROUP_ENUMPROCW proc,
                                                         DWORD flags, LONG_PTR param )
{
    return Internal_EnumSystemLanguageGroups( proc, flags, param, TRUE );
}


/******************************************************************************
 *	EnumSystemLocalesA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesA( LOCALE_ENUMPROCA proc, DWORD flags )
{
    char name[10];
    DWORD i;

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        if (!lcnames_index[i].name) continue;  /* skip invariant locale */
        if (lcnames_index[i].id == LOCALE_CUSTOM_UNSPECIFIED) continue;  /* skip locales with no lcid */
        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        if (!get_locale_data( lcnames_index[i].idx )->inotneutral) continue;  /* skip neutral locales */
        if (!SORTIDFROMLCID( lcnames_index[i].id ) != !(flags & LCID_ALTERNATE_SORTS))
            continue;  /* skip alternate sorts */
        sprintf( name, "%08x", lcnames_index[i].id );
        if (!proc( name )) break;
    }
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLocalesW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesW( LOCALE_ENUMPROCW proc, DWORD flags )
{
    WCHAR name[10];
    DWORD i;

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        if (!lcnames_index[i].name) continue;  /* skip invariant locale */
        if (lcnames_index[i].id == LOCALE_CUSTOM_UNSPECIFIED) continue;  /* skip locales with no lcid */
        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        if (!get_locale_data( lcnames_index[i].idx )->inotneutral) continue;  /* skip neutral locales */
        if (!SORTIDFROMLCID( lcnames_index[i].id ) != !(flags & LCID_ALTERNATE_SORTS))
            continue;  /* skip alternate sorts */
        swprintf( name, ARRAY_SIZE(name), L"%08lx", lcnames_index[i].id );
        if (!proc( name )) break;
    }
    return TRUE;
}


/******************************************************************************
 *	EnumSystemLocalesEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumSystemLocalesEx( LOCALE_ENUMPROCEX proc, DWORD wanted_flags,
                                                   LPARAM param, void *reserved )
{
    WCHAR buffer[LOCALE_NAME_MAX_LENGTH];
    DWORD i, flags;

    if (reserved)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (i = 0; i < locale_table->nb_lcnames; i++)
    {
        const NLS_LOCALE_DATA *locale = get_locale_data( lcnames_index[i].idx );
        const WCHAR *str = locale_strings + lcnames_index[i].name;

        if (lcnames_index[i].id & 0x80000000) continue;  /* skip aliases */
        memcpy( buffer, str + 1, (*str + 1) * sizeof(WCHAR) );
        if (SORTIDFROMLCID( lcnames_index[i].id ) || wcschr( str + 1, '_' ))
            flags = LOCALE_ALTERNATE_SORTS;
        else
            flags = LOCALE_WINDOWS | (locale->inotneutral ? LOCALE_SPECIFICDATA : LOCALE_NEUTRALDATA);
        if (wanted_flags && !(flags & wanted_flags)) continue;
        if (!proc( buffer, flags, param )) break;
    }
    return TRUE;
}


/**************************************************************************
 *	EnumTimeFormatsW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumTimeFormatsW( TIMEFMT_ENUMPROCW proc, LCID lcid, DWORD flags )
{
    return Internal_EnumTimeFormats( proc, lcid, flags, TRUE, FALSE, 0 );
}


/**************************************************************************
 *	EnumTimeFormatsEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH EnumTimeFormatsEx( TIMEFMT_ENUMPROCEX proc, const WCHAR *locale,
                                                 DWORD flags, LPARAM lparam )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return Internal_EnumTimeFormats( (TIMEFMT_ENUMPROCW)proc, lcid, flags, TRUE, TRUE, lparam );
}


/**************************************************************************
 *	FindNLSString   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindNLSString( LCID lcid, DWORD flags, const WCHAR *src,
                                            int srclen, const WCHAR *value, int valuelen, int *found )
{
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];

    LCIDToLocaleName( lcid, locale, ARRAY_SIZE(locale), 0 );
    return FindNLSStringEx( locale, flags, src, srclen, value, valuelen, found, NULL, NULL, 0 );
}


/**************************************************************************
 *	FindNLSStringEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindNLSStringEx( const WCHAR *locale, DWORD flags, const WCHAR *src,
                                              int srclen, const WCHAR *value, int valuelen, int *found,
                                              NLSVERSIONINFO *version, void *reserved, LPARAM handle )
{
    /* FIXME: this function should normalize strings before calling CompareStringEx() */
    DWORD mask = flags;
    int offset, inc, count;

    TRACE( "%s %lx %s %d %s %d %p %p %p %Id\n", wine_dbgstr_w(locale), flags,
           wine_dbgstr_w(src), srclen, wine_dbgstr_w(value), valuelen, found,
           version, reserved, handle );

    if (version || reserved || handle || !IsValidLocaleName(locale) ||
        !src || !srclen || srclen < -1 || !value || !valuelen || valuelen < -1)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }
    if (srclen == -1) srclen = lstrlenW(src);
    if (valuelen == -1) valuelen = lstrlenW(value);

    srclen -= valuelen;
    if (srclen < 0) return -1;

    mask = flags & ~(FIND_FROMSTART | FIND_FROMEND | FIND_STARTSWITH | FIND_ENDSWITH);
    count = flags & (FIND_FROMSTART | FIND_FROMEND) ? srclen + 1 : 1;
    offset = flags & (FIND_FROMSTART | FIND_STARTSWITH) ? 0 : srclen;
    inc = flags & (FIND_FROMSTART | FIND_STARTSWITH) ? 1 : -1;
    while (count--)
    {
        if (CompareStringEx( locale, mask, src + offset, valuelen,
                             value, valuelen, NULL, NULL, 0 ) == CSTR_EQUAL)
        {
            if (found) *found = valuelen;
            return offset;
        }
        offset += inc;
    }
    return -1;
}


/******************************************************************************
 *	FindStringOrdinal   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FindStringOrdinal( DWORD flag, const WCHAR *src, INT src_size,
                                                const WCHAR *val, INT val_size, BOOL ignore_case )
{
    INT offset, inc, count;

    TRACE( "%#lx %s %d %s %d %d\n", flag, wine_dbgstr_w(src), src_size,
           wine_dbgstr_w(val), val_size, ignore_case );

    if (!src || !val)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return -1;
    }

    if (flag != FIND_FROMSTART && flag != FIND_FROMEND && flag != FIND_STARTSWITH && flag != FIND_ENDSWITH)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return -1;
    }

    if (src_size == -1) src_size = lstrlenW( src );
    if (val_size == -1) val_size = lstrlenW( val );

    SetLastError( ERROR_SUCCESS );
    src_size -= val_size;
    if (src_size < 0) return -1;

    count = flag & (FIND_FROMSTART | FIND_FROMEND) ? src_size + 1 : 1;
    offset = flag & (FIND_FROMSTART | FIND_STARTSWITH) ? 0 : src_size;
    inc = flag & (FIND_FROMSTART | FIND_STARTSWITH) ? 1 : -1;
    while (count--)
    {
        if (CompareStringOrdinal( src + offset, val_size, val, val_size, ignore_case ) == CSTR_EQUAL)
            return offset;
        offset += inc;
    }
    return -1;
}


/******************************************************************************
 *	FoldStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH FoldStringW( DWORD flags, LPCWSTR src, INT srclen, LPWSTR dst, INT dstlen )
{
    NTSTATUS status;
    WCHAR *buf = dst;
    int len = dstlen;

    if (!src || !srclen || dstlen < 0 || (dstlen && !dst) || src == dst)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (srclen == -1) srclen = lstrlenW(src) + 1;

    if (!dstlen && (flags & (MAP_PRECOMPOSED | MAP_FOLDCZONE | MAP_COMPOSITE)))
    {
        len = srclen * 4;
        if (!(buf = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
    }

    for (;;)
    {
        status = fold_string( flags, src, srclen, buf, &len );
        if (buf != dst) RtlFreeHeap( GetProcessHeap(), 0, buf );
        if (status != STATUS_BUFFER_TOO_SMALL) break;
        if (!(buf = RtlAllocateHeap( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
        {
            SetLastError( ERROR_OUTOFMEMORY );
            return 0;
        }
    }
    if (status == STATUS_INVALID_PARAMETER_1)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (!set_ntstatus( status )) return 0;

    if (dstlen && dstlen < len) SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return len;
}


static const WCHAR *get_message( DWORD flags, const void *src, UINT id, UINT lang,
                                 BOOL ansi, WCHAR **buffer )
{
    DWORD len;

    if (!(flags & FORMAT_MESSAGE_FROM_STRING))
    {
        const MESSAGE_RESOURCE_ENTRY *entry;
        NTSTATUS status = STATUS_INVALID_PARAMETER;

        if (flags & FORMAT_MESSAGE_FROM_HMODULE)
        {
            HMODULE module = (HMODULE)src;
            if (!module) module = GetModuleHandleW( 0 );
            status = RtlFindMessage( module, RT_MESSAGETABLE, lang, id, &entry );
        }
        if (status && (flags & FORMAT_MESSAGE_FROM_SYSTEM))
        {
            /* Fold win32 hresult to its embedded error code. */
            if (HRESULT_SEVERITY(id) == SEVERITY_ERROR && HRESULT_FACILITY(id) == FACILITY_WIN32)
                id = HRESULT_CODE( id );
            status = RtlFindMessage( kernel32_handle, RT_MESSAGETABLE, lang, id, &entry );
        }
        if (!set_ntstatus( status )) return NULL;

        src = entry->Text;
        ansi = !(entry->Flags & MESSAGE_RESOURCE_UNICODE);
    }

    if (!ansi) return src;
    len = MultiByteToWideChar( CP_ACP, 0, src, -1, NULL, 0 );
    if (!(*buffer = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) ))) return NULL;
    MultiByteToWideChar( CP_ACP, 0, src, -1, *buffer, len );
    return *buffer;
}


/***********************************************************************
 *	FormatMessageA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH FormatMessageA( DWORD flags, const void *source, DWORD msgid, DWORD langid,
                                               char *buffer, DWORD size, va_list *args )
{
    DWORD ret = 0;
    ULONG len, retsize = 0;
    ULONG width = (flags & FORMAT_MESSAGE_MAX_WIDTH_MASK);
    const WCHAR *src;
    WCHAR *result, *message = NULL;
    NTSTATUS status;

    TRACE( "(0x%lx,%p,%#lx,0x%lx,%p,%lu,%p)\n", flags, source, msgid, langid, buffer, size, args );

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        if (!buffer)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        *(char **)buffer = NULL;
    }
    if (size >= 32768)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (width == 0xff) width = ~0u;

    if (!(src = get_message( flags, source, msgid, langid, TRUE, &message ))) return 0;

    if (!(result = HeapAlloc( GetProcessHeap(), 0, 65536 )))
        status = STATUS_NO_MEMORY;
    else
        status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                   TRUE, !!(flags & FORMAT_MESSAGE_ARGUMENT_ARRAY), args,
                                   result, 65536, &retsize );

    HeapFree( GetProcessHeap(), 0, message );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto done;
    }
    if (!set_ntstatus( status )) goto done;

    len = WideCharToMultiByte( CP_ACP, 0, result, retsize / sizeof(WCHAR), NULL, 0, NULL, NULL );
    if (len <= 1)
    {
        SetLastError( ERROR_NO_WORK_DONE );
        goto done;
    }

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        char *buf = LocalAlloc( LMEM_ZEROINIT, max( size, len ));
        if (!buf)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            goto done;
        }
        *(char **)buffer = buf;
        WideCharToMultiByte( CP_ACP, 0, result, retsize / sizeof(WCHAR), buf, max( size, len ), NULL, NULL );
    }
    else if (len > size)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        goto done;
    }
    else WideCharToMultiByte( CP_ACP, 0, result, retsize / sizeof(WCHAR), buffer, size, NULL, NULL );

    ret = len - 1;

done:
    HeapFree( GetProcessHeap(), 0, result );
    return ret;
}


/***********************************************************************
 *	FormatMessageW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH FormatMessageW( DWORD flags, const void *source, DWORD msgid, DWORD langid,
                                               WCHAR *buffer, DWORD size, va_list *args )
{
    ULONG retsize = 0;
    ULONG width = (flags & FORMAT_MESSAGE_MAX_WIDTH_MASK);
    const WCHAR *src;
    WCHAR *message = NULL;
    NTSTATUS status;

    TRACE( "(0x%lx,%p,%#lx,0x%lx,%p,%lu,%p)\n", flags, source, msgid, langid, buffer, size, args );

    if (!buffer)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (width == 0xff) width = ~0u;

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER) *(LPWSTR *)buffer = NULL;

    if (!(src = get_message( flags, source, msgid, langid, FALSE, &message ))) return 0;

    if (flags & FORMAT_MESSAGE_ALLOCATE_BUFFER)
    {
        WCHAR *result;
        ULONG alloc = max( size * sizeof(WCHAR), 65536 );

        for (;;)
        {
            if (!(result = HeapAlloc( GetProcessHeap(), 0, alloc )))
            {
                status = STATUS_NO_MEMORY;
                break;
            }
            status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                       FALSE, !!(flags & FORMAT_MESSAGE_ARGUMENT_ARRAY), args,
                                       result, alloc, &retsize );
            if (!status)
            {
                if (retsize <= sizeof(WCHAR)) HeapFree( GetProcessHeap(), 0, result );
                else *(WCHAR **)buffer = HeapReAlloc( GetProcessHeap(), HEAP_REALLOC_IN_PLACE_ONLY,
                                                      result, max( retsize, size * sizeof(WCHAR) ));
                break;
            }
            HeapFree( GetProcessHeap(), 0, result );
            if (status != STATUS_BUFFER_OVERFLOW) break;
            alloc *= 2;
        }
    }
    else status = RtlFormatMessage( src, width, !!(flags & FORMAT_MESSAGE_IGNORE_INSERTS),
                                    FALSE, !!(flags & FORMAT_MESSAGE_ARGUMENT_ARRAY), args,
                                    buffer, size * sizeof(WCHAR), &retsize );

    HeapFree( GetProcessHeap(), 0, message );

    if (status == STATUS_BUFFER_OVERFLOW)
    {
        if (size) buffer[size - 1] = 0;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    if (!set_ntstatus( status )) return 0;
    if (retsize <= sizeof(WCHAR)) SetLastError( ERROR_NO_WORK_DONE );
    return retsize / sizeof(WCHAR) - 1;
}


/******************************************************************************
 *	GetACP   (kernelbase.@)
 */
UINT WINAPI GetACP(void)
{
    return nls_info.AnsiTableInfo.CodePage;
}


/***********************************************************************
 *	GetCPInfo   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetCPInfo( UINT codepage, CPINFO *cpinfo )
{
    const CPTABLEINFO *table;

    if (!cpinfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (codepage)
    {
    case CP_UTF7:
    case CP_UTF8:
        cpinfo->DefaultChar[0] = 0x3f;
        cpinfo->DefaultChar[1] = 0;
        memset( cpinfo->LeadByte, 0, sizeof(cpinfo->LeadByte) );
        cpinfo->MaxCharSize = (codepage == CP_UTF7) ? 5 : 4;
        break;
    default:
        if (!(table = get_codepage_table( codepage ))) return FALSE;
        cpinfo->MaxCharSize = table->MaximumCharacterSize;
        memcpy( cpinfo->DefaultChar, &table->DefaultChar, sizeof(cpinfo->DefaultChar) );
        memcpy( cpinfo->LeadByte, table->LeadByte, sizeof(cpinfo->LeadByte) );
        break;
    }
    return TRUE;
}


/***********************************************************************
 *	GetCPInfoExW   (kernelbase.@)
 */
BOOL WINAPI GetCPInfoExW( UINT codepage, DWORD flags, CPINFOEXW *cpinfo )
{
    const CPTABLEINFO *table;
    int min, max, pos;

    if (!cpinfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    switch (codepage)
    {
    case CP_UTF7:
        cpinfo->DefaultChar[0] = 0x3f;
        cpinfo->DefaultChar[1] = 0;
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;
        cpinfo->MaxCharSize = 5;
        cpinfo->CodePage = CP_UTF7;
        cpinfo->UnicodeDefaultChar = 0x3f;
        break;
    case CP_UTF8:
        cpinfo->DefaultChar[0] = 0x3f;
        cpinfo->DefaultChar[1] = 0;
        cpinfo->LeadByte[0] = cpinfo->LeadByte[1] = 0;
        cpinfo->MaxCharSize = 4;
        cpinfo->CodePage = CP_UTF8;
        cpinfo->UnicodeDefaultChar = 0x3f;
        break;
    default:
        if (!(table = get_codepage_table( codepage ))) return FALSE;
        cpinfo->MaxCharSize = table->MaximumCharacterSize;
        memcpy( cpinfo->DefaultChar, &table->DefaultChar, sizeof(cpinfo->DefaultChar) );
        memcpy( cpinfo->LeadByte, table->LeadByte, sizeof(cpinfo->LeadByte) );
        cpinfo->CodePage = table->CodePage;
        cpinfo->UnicodeDefaultChar = table->UniDefaultChar;
        break;
    }

    min = 0;
    max = ARRAY_SIZE(codepage_names) - 1;
    cpinfo->CodePageName[0] = 0;
    while (min <= max)
    {
        pos = (min + max) / 2;
        if (codepage_names[pos].cp < cpinfo->CodePage) min = pos + 1;
        else if (codepage_names[pos].cp > cpinfo->CodePage) max = pos - 1;
        else
        {
            wcscpy( cpinfo->CodePageName, codepage_names[pos].name );
            break;
        }
    }
    return TRUE;
}


/***********************************************************************
 *	GetCalendarInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetCalendarInfoW( LCID lcid, CALID calendar, CALTYPE type,
                                               WCHAR *data, INT count, DWORD *value )
{
    static const LCTYPE lctype_map[] =
    {
        0, /* not used */
        0, /* CAL_ICALINTVALUE */
        0, /* CAL_SCALNAME */
        0, /* CAL_IYEAROFFSETRANGE */
        0, /* CAL_SERASTRING */
        LOCALE_SSHORTDATE,
        LOCALE_SLONGDATE,
        LOCALE_SDAYNAME1,
        LOCALE_SDAYNAME2,
        LOCALE_SDAYNAME3,
        LOCALE_SDAYNAME4,
        LOCALE_SDAYNAME5,
        LOCALE_SDAYNAME6,
        LOCALE_SDAYNAME7,
        LOCALE_SABBREVDAYNAME1,
        LOCALE_SABBREVDAYNAME2,
        LOCALE_SABBREVDAYNAME3,
        LOCALE_SABBREVDAYNAME4,
        LOCALE_SABBREVDAYNAME5,
        LOCALE_SABBREVDAYNAME6,
        LOCALE_SABBREVDAYNAME7,
        LOCALE_SMONTHNAME1,
        LOCALE_SMONTHNAME2,
        LOCALE_SMONTHNAME3,
        LOCALE_SMONTHNAME4,
        LOCALE_SMONTHNAME5,
        LOCALE_SMONTHNAME6,
        LOCALE_SMONTHNAME7,
        LOCALE_SMONTHNAME8,
        LOCALE_SMONTHNAME9,
        LOCALE_SMONTHNAME10,
        LOCALE_SMONTHNAME11,
        LOCALE_SMONTHNAME12,
        LOCALE_SMONTHNAME13,
        LOCALE_SABBREVMONTHNAME1,
        LOCALE_SABBREVMONTHNAME2,
        LOCALE_SABBREVMONTHNAME3,
        LOCALE_SABBREVMONTHNAME4,
        LOCALE_SABBREVMONTHNAME5,
        LOCALE_SABBREVMONTHNAME6,
        LOCALE_SABBREVMONTHNAME7,
        LOCALE_SABBREVMONTHNAME8,
        LOCALE_SABBREVMONTHNAME9,
        LOCALE_SABBREVMONTHNAME10,
        LOCALE_SABBREVMONTHNAME11,
        LOCALE_SABBREVMONTHNAME12,
        LOCALE_SABBREVMONTHNAME13,
        LOCALE_SYEARMONTH,
        0, /* CAL_ITWODIGITYEARMAX */
        LOCALE_SSHORTESTDAYNAME1,
        LOCALE_SSHORTESTDAYNAME2,
        LOCALE_SSHORTESTDAYNAME3,
        LOCALE_SSHORTESTDAYNAME4,
        LOCALE_SSHORTESTDAYNAME5,
        LOCALE_SSHORTESTDAYNAME6,
        LOCALE_SSHORTESTDAYNAME7,
        LOCALE_SMONTHDAY,
        0, /* CAL_SABBREVERASTRING */
    };
    DWORD flags = 0;
    CALTYPE calinfo = type & 0xffff;

    if (type & CAL_NOUSEROVERRIDE) FIXME("flag CAL_NOUSEROVERRIDE used, not fully implemented\n");
    if (type & CAL_USE_CP_ACP) FIXME("flag CAL_USE_CP_ACP used, not fully implemented\n");

    if ((type & CAL_RETURN_NUMBER) && !value)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (type & CAL_RETURN_GENITIVE_NAMES) flags |= LOCALE_RETURN_GENITIVE_NAMES;

    switch (calinfo)
    {
    case CAL_ICALINTVALUE:
        if (type & CAL_RETURN_NUMBER)
            return GetLocaleInfoW( lcid, LOCALE_RETURN_NUMBER | LOCALE_ICALENDARTYPE,
                                   (WCHAR *)value, sizeof(*value) / sizeof(WCHAR) );
        return GetLocaleInfoW( lcid, LOCALE_ICALENDARTYPE, data, count );

    case CAL_SCALNAME:
        FIXME( "Unimplemented caltype %ld\n", calinfo );
        if (data) *data = 0;
        return 1;

    case CAL_IYEAROFFSETRANGE:
    case CAL_SERASTRING:
    case CAL_SABBREVERASTRING:
        FIXME( "Unimplemented caltype %ld\n", calinfo );
        return 0;

    case CAL_SSHORTDATE:
    case CAL_SLONGDATE:
    case CAL_SDAYNAME1:
    case CAL_SDAYNAME2:
    case CAL_SDAYNAME3:
    case CAL_SDAYNAME4:
    case CAL_SDAYNAME5:
    case CAL_SDAYNAME6:
    case CAL_SDAYNAME7:
    case CAL_SABBREVDAYNAME1:
    case CAL_SABBREVDAYNAME2:
    case CAL_SABBREVDAYNAME3:
    case CAL_SABBREVDAYNAME4:
    case CAL_SABBREVDAYNAME5:
    case CAL_SABBREVDAYNAME6:
    case CAL_SABBREVDAYNAME7:
    case CAL_SMONTHNAME1:
    case CAL_SMONTHNAME2:
    case CAL_SMONTHNAME3:
    case CAL_SMONTHNAME4:
    case CAL_SMONTHNAME5:
    case CAL_SMONTHNAME6:
    case CAL_SMONTHNAME7:
    case CAL_SMONTHNAME8:
    case CAL_SMONTHNAME9:
    case CAL_SMONTHNAME10:
    case CAL_SMONTHNAME11:
    case CAL_SMONTHNAME12:
    case CAL_SMONTHNAME13:
    case CAL_SABBREVMONTHNAME1:
    case CAL_SABBREVMONTHNAME2:
    case CAL_SABBREVMONTHNAME3:
    case CAL_SABBREVMONTHNAME4:
    case CAL_SABBREVMONTHNAME5:
    case CAL_SABBREVMONTHNAME6:
    case CAL_SABBREVMONTHNAME7:
    case CAL_SABBREVMONTHNAME8:
    case CAL_SABBREVMONTHNAME9:
    case CAL_SABBREVMONTHNAME10:
    case CAL_SABBREVMONTHNAME11:
    case CAL_SABBREVMONTHNAME12:
    case CAL_SABBREVMONTHNAME13:
    case CAL_SMONTHDAY:
    case CAL_SYEARMONTH:
    case CAL_SSHORTESTDAYNAME1:
    case CAL_SSHORTESTDAYNAME2:
    case CAL_SSHORTESTDAYNAME3:
    case CAL_SSHORTESTDAYNAME4:
    case CAL_SSHORTESTDAYNAME5:
    case CAL_SSHORTESTDAYNAME6:
    case CAL_SSHORTESTDAYNAME7:
        return GetLocaleInfoW( lcid, lctype_map[calinfo] | flags, data, count );

    case CAL_ITWODIGITYEARMAX:
        if (type & CAL_RETURN_NUMBER)
        {
            *value = CALINFO_MAX_YEAR;
            return sizeof(DWORD) / sizeof(WCHAR);
        }
        else
        {
            WCHAR buffer[10];
            int ret = swprintf( buffer, ARRAY_SIZE(buffer), L"%u", CALINFO_MAX_YEAR ) + 1;
            if (!data) return ret;
            if (ret <= count)
            {
                lstrcpyW( data, buffer );
                return ret;
            }
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }
        break;
    default:
        FIXME( "Unknown caltype %ld\n", calinfo );
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    return 0;
}


/***********************************************************************
 *	GetCalendarInfoEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetCalendarInfoEx( const WCHAR *locale, CALID calendar, const WCHAR *reserved,
                                                CALTYPE type, WCHAR *data, INT count, DWORD *value )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );
    return GetCalendarInfoW( lcid, calendar, type, data, count, value );
}

static CRITICAL_SECTION tzname_section;
static CRITICAL_SECTION_DEBUG tzname_section_debug =
{
    0, 0, &tzname_section,
    { &tzname_section_debug.ProcessLocksList, &tzname_section_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": tzname_section") }
};
static CRITICAL_SECTION tzname_section = { &tzname_section_debug, -1, 0, 0, 0, 0 };
static struct {
    LCID lcid;
    WCHAR key_name[128];
    WCHAR standard_name[32];
    WCHAR daylight_name[32];
} cached_tzname;

/***********************************************************************
 *	GetDynamicTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDynamicTimeZoneInformation( DYNAMIC_TIME_ZONE_INFORMATION *info )
{
    HKEY key;
    LARGE_INTEGER now;

    if (!set_ntstatus( RtlQueryDynamicTimeZoneInformation( (RTL_DYNAMIC_TIME_ZONE_INFORMATION *)info )))
        return TIME_ZONE_ID_INVALID;

    RtlEnterCriticalSection( &tzname_section );
    if (cached_tzname.lcid == GetThreadLocale() &&
        !wcscmp( info->TimeZoneKeyName, cached_tzname.key_name ))
    {
        wcscpy( info->StandardName, cached_tzname.standard_name );
        wcscpy( info->DaylightName, cached_tzname.daylight_name );
        RtlLeaveCriticalSection( &tzname_section );
    }
    else
    {
        RtlLeaveCriticalSection( &tzname_section );
        if (!RegOpenKeyExW( tz_key, info->TimeZoneKeyName, 0, KEY_ALL_ACCESS, &key ))
        {
            RegLoadMUIStringW( key, L"MUI_Std", info->StandardName,
                               sizeof(info->StandardName), NULL, 0, system_dir );
            RegLoadMUIStringW( key, L"MUI_Dlt", info->DaylightName,
                               sizeof(info->DaylightName), NULL, 0, system_dir );
            RegCloseKey( key );
        }
        else return TIME_ZONE_ID_INVALID;

        RtlEnterCriticalSection( &tzname_section );
        cached_tzname.lcid = GetThreadLocale();
        wcscpy( cached_tzname.key_name, info->TimeZoneKeyName );
        wcscpy( cached_tzname.standard_name, info->StandardName );
        wcscpy( cached_tzname.daylight_name, info->DaylightName );
        RtlLeaveCriticalSection( &tzname_section );
    }

    NtQuerySystemTime( &now );
    return get_timezone_id( (TIME_ZONE_INFORMATION *)info, now, FALSE );
}


/******************************************************************************
 *	GetDynamicTimeZoneInformationEffectiveYears   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetDynamicTimeZoneInformationEffectiveYears( const DYNAMIC_TIME_ZONE_INFORMATION *info,
                                                                            DWORD *first, DWORD *last )
{
    HKEY key, dst_key = 0;
    DWORD type, count, ret = ERROR_FILE_NOT_FOUND;

    if (RegOpenKeyExW( tz_key, info->TimeZoneKeyName, 0, KEY_ALL_ACCESS, &key )) return ret;

    if (RegOpenKeyExW( key, L"Dynamic DST", 0, KEY_ALL_ACCESS, &dst_key )) goto done;
    count = sizeof(DWORD);
    if (RegQueryValueExW( dst_key, L"FirstEntry", NULL, &type, (BYTE *)first, &count )) goto done;
    if (type != REG_DWORD) goto done;
    count = sizeof(DWORD);
    if (RegQueryValueExW( dst_key, L"LastEntry", NULL, &type, (BYTE *)last, &count )) goto done;
    if (type != REG_DWORD) goto done;
    ret = 0;

done:
    RegCloseKey( dst_key );
    RegCloseKey( key );
    return ret;
}


/******************************************************************************
 *	GetFileMUIInfo   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ GetFileMUIInfo( DWORD flags, const WCHAR *path,
                                                    FILEMUIINFO *info, DWORD *size )
{
    FIXME( "stub: %lu, %s, %p, %p\n", flags, debugstr_w(path), info, size );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/******************************************************************************
 *	GetFileMUIPath   (kernelbase.@)
 */
BOOL WINAPI /* DECLSPEC_HOTPATCH */ GetFileMUIPath( DWORD flags, const WCHAR *filepath,
                                                    WCHAR *language, ULONG *languagelen,
                                                    WCHAR *muipath, ULONG *muipathlen,
                                                    ULONGLONG *enumerator )
{
    FIXME( "stub: 0x%lx, %s, %s, %p, %p, %p, %p\n", flags, debugstr_w(filepath),
           debugstr_w(language), languagelen, muipath, muipathlen, enumerator );
    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return FALSE;
}


/******************************************************************************
 *	GetGeoInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetGeoInfoW( GEOID id, GEOTYPE type, WCHAR *data, int count, LANGID lang )
{
    const struct geoinfo *ptr = get_geoinfo_ptr( id );
    WCHAR bufferW[12];
    const WCHAR *str = bufferW;
    int len;

    TRACE( "%ld %ld %p %d %d\n", id, type, data, count, lang );

    if (!ptr)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    switch (type)
    {
    case GEO_NATION:
        if (ptr->kind != LOCATION_NATION) return 0;
        /* fall through */
    case GEO_ID:
        swprintf( bufferW, ARRAY_SIZE(bufferW), L"%u", ptr->id );
        break;
    case GEO_ISO_UN_NUMBER:
        swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03u", ptr->uncode );
        break;
    case GEO_PARENT:
        swprintf( bufferW, ARRAY_SIZE(bufferW), L"%u", ptr->parent );
        break;
    case GEO_ISO2:
        str = ptr->iso2W;
        break;
    case GEO_ISO3:
        str = ptr->iso3W;
        break;
    case GEO_RFC1766:
    case GEO_LCID:
    case GEO_FRIENDLYNAME:
    case GEO_OFFICIALNAME:
    case GEO_TIMEZONES:
    case GEO_OFFICIALLANGUAGES:
    case GEO_LATITUDE:
    case GEO_LONGITUDE:
    case GEO_DIALINGCODE:
    case GEO_CURRENCYCODE:
    case GEO_CURRENCYSYMBOL:
    case GEO_NAME:
        FIXME( "type %ld is not supported\n", type );
        SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
        return 0;
    default:
        WARN( "unrecognized type %ld\n", type );
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    len = lstrlenW(str) + 1;
    if (!data || !count) return len;

    memcpy( data, str, min(len, count) * sizeof(WCHAR) );
    if (count < len) SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return count < len ? 0 : len;
}


/******************************************************************************
 *	GetLocaleInfoA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoA( LCID lcid, LCTYPE lctype, char *buffer, INT len )
{
    WCHAR *bufferW;
    INT lenW, ret;

    TRACE( "lcid=0x%lx lctype=0x%lx %p %d\n", lcid, lctype, buffer, len );

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (LOWORD(lctype) == LOCALE_SSHORTTIME || (lctype & LOCALE_RETURN_GENITIVE_NAMES))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (LOWORD(lctype) == LOCALE_FONTSIGNATURE || (lctype & LOCALE_RETURN_NUMBER))
        return GetLocaleInfoW( lcid, lctype, (WCHAR *)buffer, len / sizeof(WCHAR) ) * sizeof(WCHAR);

    if (!(lenW = GetLocaleInfoW( lcid, lctype, NULL, 0 ))) return 0;

    if (!(bufferW = RtlAllocateHeap( GetProcessHeap(), 0, lenW * sizeof(WCHAR) )))
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }
    ret = GetLocaleInfoW( lcid, lctype, bufferW, lenW );
    if (ret) ret = WideCharToMultiByte( get_lcid_codepage( lcid, lctype ), 0,
                                        bufferW, ret, buffer, len, NULL, NULL );
    RtlFreeHeap( GetProcessHeap(), 0, bufferW );
    return ret;
}


/******************************************************************************
 *	GetLocaleInfoW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoW( LCID lcid, LCTYPE lctype, WCHAR *buffer, INT len )
{
    HRSRC hrsrc;
    HGLOBAL hmem;
    INT ret;
    UINT lcflags = lctype;
    const WCHAR *p;
    unsigned int i;

    if (len < 0 || (len && !buffer))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (lctype & LOCALE_RETURN_GENITIVE_NAMES && !is_genitive_name_supported( lctype ))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (!len) buffer = NULL;

    lcid = ConvertDefaultLocale( lcid );
    lctype = LOWORD(lctype);

    TRACE( "(lcid=0x%lx,lctype=0x%lx,%p,%d)\n", lcid, lctype, buffer, len );

    /* first check for overrides in the registry */

    if (!(lcflags & LOCALE_NOUSEROVERRIDE) && lcid == ConvertDefaultLocale( LOCALE_USER_DEFAULT ))
    {
        const struct registry_value *value = get_locale_registry_value( lctype );

        if (value)
        {
            if (lcflags & LOCALE_RETURN_NUMBER)
            {
                WCHAR tmp[16];
                ret = get_registry_locale_info( value, tmp, ARRAY_SIZE( tmp ));
                if (ret > 0)
                {
                    WCHAR *end;
                    UINT number = wcstol( tmp, &end, get_value_base_by_lctype( lctype ) );
                    if (*end)  /* invalid number */
                    {
                        SetLastError( ERROR_INVALID_FLAGS );
                        return 0;
                    }
                    ret = sizeof(UINT) / sizeof(WCHAR);
                    if (!len) return ret;
                    if (ret > len)
                    {
                        SetLastError( ERROR_INSUFFICIENT_BUFFER );
                        return 0;
                    }
                    memcpy( buffer, &number, sizeof(number) );
                }
            }
            else ret = get_registry_locale_info( value, buffer, len );

            if (ret != -1) return ret;
        }
    }

    /* now load it from kernel resources */

    if (!(hrsrc = FindResourceExW( kernel32_handle, (LPWSTR)RT_STRING,
                                   ULongToPtr((lctype >> 4) + 1), lcid )))
    {
        SetLastError( ERROR_INVALID_FLAGS );  /* no such lctype */
        return 0;
    }
    if (!(hmem = LoadResource( kernel32_handle, hrsrc ))) return 0;

    p = LockResource( hmem );
    for (i = 0; i < (lctype & 0x0f); i++) p += *p + 1;

    if (lcflags & LOCALE_RETURN_NUMBER) ret = sizeof(UINT) / sizeof(WCHAR);
    else if (is_genitive_name_supported( lctype ) && *p)
    {
        /* genitive form is stored after a null separator from a nominative */
        for (i = 1; i <= *p; i++) if (!p[i]) break;

        if (i <= *p && (lcflags & LOCALE_RETURN_GENITIVE_NAMES))
        {
            ret = *p - i + 1;
            p += i;
        }
        else ret = i;
    }
    else
        ret = (lctype == LOCALE_FONTSIGNATURE) ? *p : *p + 1;

    if (!len) return ret;

    if (ret > len)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    if (lcflags & LOCALE_RETURN_NUMBER)
    {
        UINT number;
        WCHAR *end, *tmp = HeapAlloc( GetProcessHeap(), 0, (*p + 1) * sizeof(WCHAR) );
        if (!tmp) return 0;
        memcpy( tmp, p + 1, *p * sizeof(WCHAR) );
        tmp[*p] = 0;
        number = wcstol( tmp, &end, get_value_base_by_lctype( lctype ) );
        if (!*end)
            memcpy( buffer, &number, sizeof(number) );
        else  /* invalid number */
        {
            SetLastError( ERROR_INVALID_FLAGS );
            ret = 0;
        }
        HeapFree( GetProcessHeap(), 0, tmp );

        TRACE( "(lcid=0x%lx,lctype=0x%lx,%p,%d) returning number %d\n",
               lcid, lctype, buffer, len, number );
    }
    else
    {
        memcpy( buffer, p + 1, ret * sizeof(WCHAR) );
        if (lctype != LOCALE_FONTSIGNATURE) buffer[ret-1] = 0;

        TRACE( "(lcid=0x%lx,lctype=0x%lx,%p,%d) returning %d %s\n",
               lcid, lctype, buffer, len, ret, debugstr_w(buffer) );
    }
    return ret;
}


/******************************************************************************
 *	GetLocaleInfoEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetLocaleInfoEx( const WCHAR *locale, LCTYPE info, WCHAR *buffer, INT len )
{
    LCID lcid = LocaleNameToLCID( locale, 0 );

    TRACE( "%s lcid=0x%lx 0x%lx\n", debugstr_w(locale), lcid, info );

    if (!lcid) return 0;

    /* special handling for neutral locale names */
    if (locale && lstrlenW( locale ) == 2)
    {
        switch (LOWORD( info ))
        {
        case LOCALE_SNAME:
            if (len && len < 3)
            {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }
            if (len) lstrcpyW( buffer, locale );
            return 3;
        case LOCALE_SPARENT:
            if (len) buffer[0] = 0;
            return 1;
        }
    }
    return GetLocaleInfoW( lcid, info, buffer, len );
}


/******************************************************************************
 *	GetNLSVersion   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNLSVersion( NLS_FUNCTION func, LCID lcid, NLSVERSIONINFO *info )
{
    WCHAR locale[LOCALE_NAME_MAX_LENGTH];

    if (info->dwNLSVersionInfoSize < offsetof( NLSVERSIONINFO, dwEffectiveId ))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }
    if (!LCIDToLocaleName( lcid, locale, LOCALE_NAME_MAX_LENGTH, LOCALE_ALLOW_NEUTRAL_NAMES ))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return GetNLSVersionEx( func, locale, (NLSVERSIONINFOEX *)info );
}


/******************************************************************************
 *	GetNLSVersionEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetNLSVersionEx( NLS_FUNCTION func, const WCHAR *locale,
                                               NLSVERSIONINFOEX *info )
{
    LCID lcid = 0;

    if (func != COMPARE_STRING)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }
    if (info->dwNLSVersionInfoSize < sizeof(*info) &&
        (info->dwNLSVersionInfoSize != offsetof( NLSVERSIONINFO, dwEffectiveId )))
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    if (!(lcid = LocaleNameToLCID( locale, 0 ))) return FALSE;

    info->dwNLSVersion = info->dwDefinedVersion = sort.version;
    if (info->dwNLSVersionInfoSize >= sizeof(*info))
    {
        const struct sortguid *sortid = get_language_sort( locale );
        info->dwEffectiveId = lcid;
        info->guidCustomVersion = sortid ? sortid->id : default_sort_guid;
    }
    return TRUE;
}


/******************************************************************************
 *	GetOEMCP   (kernelbase.@)
 */
UINT WINAPI GetOEMCP(void)
{
    return nls_info.OemTableInfo.CodePage;
}


/***********************************************************************
 *      GetProcessPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetProcessPreferredUILanguages( DWORD flags, ULONG *count,
                                                              WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetProcessPreferredUILanguages( flags, count, buffer, size ));
}


/***********************************************************************
 *	GetStringTypeA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetStringTypeA( LCID locale, DWORD type, const char *src, int count,
                                              WORD *chartype )
{
    UINT cp;
    INT countW;
    LPWSTR srcW;
    BOOL ret = FALSE;

    if (count == -1) count = strlen(src) + 1;

    cp = get_lcid_codepage( locale, 0 );
    countW = MultiByteToWideChar(cp, 0, src, count, NULL, 0);
    if((srcW = HeapAlloc(GetProcessHeap(), 0, countW * sizeof(WCHAR))))
    {
        MultiByteToWideChar(cp, 0, src, count, srcW, countW);
    /*
     * NOTE: the target buffer has 1 word for each CHARACTER in the source
     * string, with multibyte characters there maybe be more bytes in count
     * than character space in the buffer!
     */
        ret = GetStringTypeW(type, srcW, countW, chartype);
        HeapFree(GetProcessHeap(), 0, srcW);
    }
    return ret;
}


/***********************************************************************
 *	GetStringTypeW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetStringTypeW( DWORD type, const WCHAR *src, INT count, WORD *chartype )
{
    if (!src)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (type != CT_CTYPE1 && type != CT_CTYPE2 && type != CT_CTYPE3)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (count == -1) count = lstrlenW(src) + 1;

    while (count--) *chartype++ = get_char_type( type, *src++ );

    return TRUE;
}


/***********************************************************************
 *	GetStringTypeExW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetStringTypeExW( LCID locale, DWORD type, const WCHAR *src, int count,
                                                WORD *chartype )
{
    /* locale is ignored for Unicode */
    return GetStringTypeW( type, src, count, chartype );
}


/***********************************************************************
 *	GetSystemDefaultLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLCID(void)
{
    return system_lcid;
}


/***********************************************************************
 *	GetSystemDefaultLangID   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLangID(void)
{
    return LANGIDFROMLCID( GetSystemDefaultLCID() );
}


/***********************************************************************
 *	GetSystemDefaultLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetSystemDefaultLocaleName( LPWSTR name, INT len )
{
    return LCIDToLocaleName( GetSystemDefaultLCID(), name, len, 0 );
}


/***********************************************************************
 *	GetSystemDefaultUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetSystemDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryInstallUILanguage( &lang );
    return lang;
}


/***********************************************************************
 *      GetSystemPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetSystemPreferredUILanguages( DWORD flags, ULONG *count,
                                                             WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetSystemPreferredUILanguages( flags, 0, count, buffer, size ));
}


/***********************************************************************
 *      GetThreadPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetThreadPreferredUILanguages( DWORD flags, ULONG *count,
                                                             WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetThreadPreferredUILanguages( flags, count, buffer, size ));
}


/***********************************************************************
 *	GetTimeZoneInformation   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH GetTimeZoneInformation( TIME_ZONE_INFORMATION *info )
{
    DYNAMIC_TIME_ZONE_INFORMATION tzinfo;
    DWORD ret = GetDynamicTimeZoneInformation( &tzinfo );

    memcpy( info, &tzinfo, sizeof(*info) );
    return ret;
}


/***********************************************************************
 *	GetTimeZoneInformationForYear   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetTimeZoneInformationForYear( USHORT year,
                                                             DYNAMIC_TIME_ZONE_INFORMATION *dynamic,
                                                             TIME_ZONE_INFORMATION *info )
{
    DYNAMIC_TIME_ZONE_INFORMATION local_info;
    HKEY key = 0, dst_key;
    DWORD count;
    LRESULT ret;
    struct
    {
        LONG bias;
        LONG std_bias;
        LONG dlt_bias;
        SYSTEMTIME std_date;
        SYSTEMTIME dlt_date;
    } data;

    TRACE( "(%u,%p)\n", year, info );

    if (!dynamic)
    {
        if (GetDynamicTimeZoneInformation( &local_info ) == TIME_ZONE_ID_INVALID) return FALSE;
        dynamic = &local_info;
    }

    if ((ret = RegOpenKeyExW( tz_key, dynamic->TimeZoneKeyName, 0, KEY_ALL_ACCESS, &key ))) goto done;
    if (RegLoadMUIStringW( key, L"MUI_Std", info->StandardName,
                           sizeof(info->StandardName), NULL, 0, system_dir ))
    {
        count = sizeof(info->StandardName);
        if ((ret = RegQueryValueExW( key, L"Std", NULL, NULL, (BYTE *)info->StandardName, &count )))
            goto done;
    }
    if (RegLoadMUIStringW( key, L"MUI_Dlt", info->DaylightName,
                           sizeof(info->DaylightName), NULL, 0, system_dir ))
    {
        count = sizeof(info->DaylightName);
        if ((ret = RegQueryValueExW( key, L"Dlt", NULL, NULL, (BYTE *)info->DaylightName, &count )))
            goto done;
    }

    ret = ERROR_FILE_NOT_FOUND;
    if (!dynamic->DynamicDaylightTimeDisabled &&
        !RegOpenKeyExW( key, L"Dynamic DST", 0, KEY_ALL_ACCESS, &dst_key ))
    {
        WCHAR yearW[16];
        swprintf( yearW, ARRAY_SIZE(yearW), L"%u", year );
        count = sizeof(data);
        ret = RegQueryValueExW( dst_key, yearW, NULL, NULL, (BYTE *)&data, &count );
        RegCloseKey( dst_key );
    }
    if (ret)
    {
        count = sizeof(data);
        ret = RegQueryValueExW( key, L"TZI", NULL, NULL, (BYTE *)&data, &count );
    }

    if (!ret)
    {
        info->Bias = data.bias;
        info->StandardBias = data.std_bias;
        info->DaylightBias = data.dlt_bias;
        info->StandardDate = data.std_date;
        info->DaylightDate = data.dlt_date;
    }

done:
    RegCloseKey( key );
    if (ret) SetLastError( ret );
    return !ret;
}


/***********************************************************************
 *	GetUserDefaultLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH GetUserDefaultLCID(void)
{
    return user_lcid;
}


/***********************************************************************
 *	GetUserDefaultLangID   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetUserDefaultLangID(void)
{
    return LANGIDFROMLCID( GetUserDefaultLCID() );
}


/***********************************************************************
 *	GetUserDefaultLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH GetUserDefaultLocaleName( LPWSTR name, INT len )
{
    return LCIDToLocaleName( GetUserDefaultLCID(), name, len, 0 );
}


/***********************************************************************
 *	GetUserDefaultUILanguage   (kernelbase.@)
 */
LANGID WINAPI DECLSPEC_HOTPATCH GetUserDefaultUILanguage(void)
{
    LANGID lang;
    NtQueryDefaultUILanguage( &lang );
    return lang;
}


/******************************************************************************
 *	GetUserGeoID   (kernelbase.@)
 */
GEOID WINAPI DECLSPEC_HOTPATCH GetUserGeoID( GEOCLASS geoclass )
{
    GEOID ret = 39070;
    const WCHAR *name;
    WCHAR bufferW[40];
    HKEY hkey;

    switch (geoclass)
    {
    case GEOCLASS_NATION:
        name = L"Nation";
        break;
    case GEOCLASS_REGION:
        name = L"Region";
        break;
    default:
        WARN("Unknown geoclass %ld\n", geoclass);
        return GEOID_NOT_AVAILABLE;
    }
    if (!RegOpenKeyExW( intl_key, L"Geo", 0, KEY_ALL_ACCESS, &hkey ))
    {
        DWORD count = sizeof(bufferW);
        if (!RegQueryValueExW( hkey, name, NULL, NULL, (BYTE *)bufferW, &count ))
            ret = wcstol( bufferW, NULL, 10 );
        RegCloseKey( hkey );
    }
    return ret;
}


/******************************************************************************
 *      GetUserPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH GetUserPreferredUILanguages( DWORD flags, ULONG *count,
                                                           WCHAR *buffer, ULONG *size )
{
    return set_ntstatus( RtlGetUserPreferredUILanguages( flags, 0, count, buffer, size ));
}


/******************************************************************************
 *	IdnToAscii   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH IdnToAscii( DWORD flags, const WCHAR *src, INT srclen,
                                         WCHAR *dst, INT dstlen )
{
    NTSTATUS status = RtlIdnToAscii( flags, src, srclen, dst, &dstlen );
    if (!set_ntstatus( status )) return 0;
    return dstlen;
}


/******************************************************************************
 *	IdnToNameprepUnicode   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH IdnToNameprepUnicode( DWORD flags, const WCHAR *src, INT srclen,
                                                   WCHAR *dst, INT dstlen )
{
    NTSTATUS status = RtlIdnToNameprepUnicode( flags, src, srclen, dst, &dstlen );
    if (!set_ntstatus( status )) return 0;
    return dstlen;
}


/******************************************************************************
 *	IdnToUnicode   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH IdnToUnicode( DWORD flags, const WCHAR *src, INT srclen,
                                           WCHAR *dst, INT dstlen )
{
    NTSTATUS status = RtlIdnToUnicode( flags, src, srclen, dst, &dstlen );
    if (!set_ntstatus( status )) return 0;
    return dstlen;
}


/******************************************************************************
 *	IsCharAlphaA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaA( CHAR c )
{
    WCHAR wc = nls_info.AnsiTableInfo.MultiByteTable[(unsigned char)c];
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_ALPHA);
}


/******************************************************************************
 *	IsCharAlphaW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_ALPHA);
}


/******************************************************************************
 *	IsCharAlphaNumericA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaNumericA( CHAR c )
{
    WCHAR wc = nls_info.AnsiTableInfo.MultiByteTable[(unsigned char)c];
    return !!(get_char_type( CT_CTYPE1, wc ) & (C1_ALPHA | C1_DIGIT));
}


/******************************************************************************
 *	IsCharAlphaNumericW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharAlphaNumericW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & (C1_ALPHA | C1_DIGIT));
}


/******************************************************************************
 *	IsCharBlankW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharBlankW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_BLANK);
}


/******************************************************************************
 *	IsCharCntrlW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharCntrlW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_CNTRL);
}


/******************************************************************************
 *	IsCharDigitW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharDigitW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_DIGIT);
}


/******************************************************************************
 *	IsCharLowerA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharLowerA( CHAR c )
{
    WCHAR wc = nls_info.AnsiTableInfo.MultiByteTable[(unsigned char)c];
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_LOWER);
}


/******************************************************************************
 *	IsCharLowerW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharLowerW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_LOWER);
}


/******************************************************************************
 *	IsCharPunctW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharPunctW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_PUNCT);
}


/******************************************************************************
 *	IsCharSpaceA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharSpaceA( CHAR c )
{
    WCHAR wc = nls_info.AnsiTableInfo.MultiByteTable[(unsigned char)c];
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_SPACE);
}


/******************************************************************************
 *	IsCharSpaceW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharSpaceW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_SPACE);
}


/******************************************************************************
 *	IsCharUpperA   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharUpperA( CHAR c )
{
    WCHAR wc = nls_info.AnsiTableInfo.MultiByteTable[(unsigned char)c];
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_UPPER);
}


/******************************************************************************
 *	IsCharUpperW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharUpperW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_UPPER);
}


/******************************************************************************
 *	IsCharXDigitW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsCharXDigitW( WCHAR wc )
{
    return !!(get_char_type( CT_CTYPE1, wc ) & C1_XDIGIT);
}


/******************************************************************************
 *	IsDBCSLeadByte   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsDBCSLeadByte( BYTE testchar )
{
    return nls_info.AnsiTableInfo.DBCSCodePage && nls_info.AnsiTableInfo.DBCSOffsets[testchar];
}


/******************************************************************************
 *	IsDBCSLeadByteEx   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsDBCSLeadByteEx( UINT codepage, BYTE testchar )
{
    const CPTABLEINFO *table = get_codepage_table( codepage );
    return table && table->DBCSCodePage && table->DBCSOffsets[testchar];
}


/******************************************************************************
 *	IsNormalizedString   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsNormalizedString( NORM_FORM form, const WCHAR *str, INT len )
{
    BOOLEAN res;
    if (!set_ntstatus( RtlIsNormalizedString( form, str, len, &res ))) res = FALSE;
    return res;
}


/******************************************************************************
 *	IsValidCodePage   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidCodePage( UINT codepage )
{
    switch (codepage)
    {
    case CP_ACP:
    case CP_OEMCP:
    case CP_MACCP:
    case CP_THREAD_ACP:
        return FALSE;
    case CP_UTF7:
    case CP_UTF8:
        return TRUE;
    default:
        return get_codepage_table( codepage ) != NULL;
    }
}


/******************************************************************************
 *	IsValidLanguageGroup   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLanguageGroup( LGRPID id, DWORD flags )
{
    WCHAR name[10], value[10];
    DWORD type, value_len = sizeof(value);
    BOOL ret = FALSE;
    HKEY key;

    if (RegOpenKeyExW( nls_key, L"Language Groups", 0, KEY_READ, &key )) return FALSE;

    swprintf( name, ARRAY_SIZE(name), L"%x", id );
    if (!RegQueryValueExW( key, name, NULL, &type, (BYTE *)value, &value_len ) && type == REG_SZ)
        ret = (flags & LGRPID_SUPPORTED) || wcstoul( value, NULL, 10 );

    RegCloseKey( key );
    return ret;
}


/******************************************************************************
 *	IsValidLocale   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLocale( LCID lcid, DWORD flags )
{
    return !!get_locale_by_id( &lcid, LOCALE_ALLOW_NEUTRAL_NAMES );
}


/******************************************************************************
 *	IsValidLocaleName   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH IsValidLocaleName( const WCHAR *locale )
{
    if (locale == LOCALE_NAME_USER_DEFAULT) return FALSE;
    return !!find_lcname_entry( locale );
}


/******************************************************************************
 *	IsValidNLSVersion   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH IsValidNLSVersion( NLS_FUNCTION func, const WCHAR *locale,
                                                  NLSVERSIONINFOEX *info )
{
    static const GUID GUID_NULL;
    NLSVERSIONINFOEX infoex;
    DWORD ret;

    if (func != COMPARE_STRING)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (info->dwNLSVersionInfoSize < sizeof(*info) &&
        (info->dwNLSVersionInfoSize != offsetof( NLSVERSIONINFO, dwEffectiveId )))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    infoex.dwNLSVersionInfoSize = sizeof(infoex);
    if (!GetNLSVersionEx( func, locale, &infoex )) return FALSE;

    ret = (infoex.dwNLSVersion & ~0xff) == (info->dwNLSVersion & ~0xff);
    if (ret && !IsEqualGUID( &info->guidCustomVersion, &GUID_NULL ))
        ret = find_sortguid( &info->guidCustomVersion ) != NULL;

    if (!ret) SetLastError( ERROR_SUCCESS );
    return ret;
}


/***********************************************************************
 *	LCIDToLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCIDToLocaleName( LCID lcid, WCHAR *name, INT count, DWORD flags )
{
    static int once;
    if (flags && !once++) FIXME( "unsupported flags %lx\n", flags );

    return GetLocaleInfoW( lcid, LOCALE_SNAME | LOCALE_NOUSEROVERRIDE, name, count );
}


/***********************************************************************
 *	LCMapStringEx   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCMapStringEx( const WCHAR *locale, DWORD flags, const WCHAR *src, int srclen,
                                            WCHAR *dst, int dstlen, NLSVERSIONINFO *version,
                                            void *reserved, LPARAM handle )
{
    const struct sortguid *sortid = NULL;
    LPWSTR dst_ptr;
    INT len;

    if (version) FIXME( "unsupported version structure %p\n", version );
    if (reserved) FIXME( "unsupported reserved pointer %p\n", reserved );
    if (handle)
    {
        static int once;
        if (!once++) FIXME( "unsupported lparam %Ix\n", handle );
    }

    if (!src || !srclen || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    /* mutually exclusive flags */
    if ((flags & (LCMAP_LOWERCASE | LCMAP_UPPERCASE)) == (LCMAP_LOWERCASE | LCMAP_UPPERCASE) ||
        (flags & (LCMAP_HIRAGANA | LCMAP_KATAKANA)) == (LCMAP_HIRAGANA | LCMAP_KATAKANA) ||
        (flags & (LCMAP_HALFWIDTH | LCMAP_FULLWIDTH)) == (LCMAP_HALFWIDTH | LCMAP_FULLWIDTH) ||
        (flags & (LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE)) == (LCMAP_TRADITIONAL_CHINESE | LCMAP_SIMPLIFIED_CHINESE) ||
        !flags)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (!dstlen) dst = NULL;

    if (flags & LCMAP_LINGUISTIC_CASING && !(sortid = get_language_sort( locale )))
    {
        FIXME( "unknown locale %s\n", debugstr_w(locale) );
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (flags & LCMAP_SORTKEY)
    {
        INT ret;

        if (src == dst)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            return 0;
        }
        if (srclen < 0) srclen = lstrlenW(src);

        TRACE( "(%s,0x%08lx,%s,%d,%p,%d)\n",
               debugstr_w(locale), flags, debugstr_wn(src, srclen), srclen, dst, dstlen );

        if ((ret = get_sortkey( flags, src, srclen, (char *)dst, dstlen ))) ret++;
        else SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return ret;
    }

    /* SORT_STRINGSORT must be used exclusively with LCMAP_SORTKEY */
    if (flags & SORT_STRINGSORT)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }
    if (((flags & (NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS)) &&
         (flags & ~(NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS))) ||
        ((flags & (LCMAP_HIRAGANA | LCMAP_KATAKANA | LCMAP_HALFWIDTH | LCMAP_FULLWIDTH)) &&
         (flags & (LCMAP_SIMPLIFIED_CHINESE | LCMAP_TRADITIONAL_CHINESE))))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (srclen < 0) srclen = lstrlenW(src) + 1;

    TRACE( "(%s,0x%08lx,%s,%d,%p,%d)\n",
           debugstr_w(locale), flags, debugstr_wn(src, srclen), srclen, dst, dstlen );

    if (!dst) /* return required string length */
    {
        if (flags & NORM_IGNORESYMBOLS)
        {
            for (len = 0; srclen; src++, srclen--)
                if (!(get_char_type( CT_CTYPE1, *src ) & (C1_PUNCT | C1_SPACE))) len++;
        }
        else if (flags & LCMAP_FULLWIDTH)
        {
            for (len = 0; srclen; src++, srclen--, len++)
            {
                if (compose_katakana( src, srclen, NULL ) == 2)
                {
                    src++;
                    srclen--;
                }
            }
        }
        else if (flags & LCMAP_HALFWIDTH)
        {
            for (len = 0; srclen; src++, srclen--, len++)
            {
                WCHAR wch = *src;
                /* map Hiragana to Katakana before decomposition if needed */
                if ((flags & LCMAP_KATAKANA) &&
                    ((wch >= 0x3041 && wch <= 0x3096) || wch == 0x309D || wch == 0x309E))
                    wch += 0x60;

                if (decompose_katakana( wch, NULL, 0 ) == 2) len++;
            }
        }
        else len = srclen;
        return len;
    }

    if (src == dst && (flags & ~(LCMAP_LOWERCASE | LCMAP_UPPERCASE)))
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return 0;
    }

    if (flags & (NORM_IGNORENONSPACE | NORM_IGNORESYMBOLS))
    {
        for (len = dstlen, dst_ptr = dst; srclen && len; src++, srclen--)
        {
            if ((flags & NORM_IGNORESYMBOLS) && (get_char_type( CT_CTYPE1, *src ) & (C1_PUNCT | C1_SPACE)))
                continue;
            *dst_ptr++ = *src;
            len--;
        }
        goto done;
    }

    if (flags & (LCMAP_FULLWIDTH | LCMAP_HALFWIDTH | LCMAP_HIRAGANA | LCMAP_KATAKANA))
    {
        for (len = dstlen, dst_ptr = dst; len && srclen; src++, srclen--, len--, dst_ptr++)
        {
            WCHAR wch;
            if (flags & LCMAP_FULLWIDTH)
            {
                /* map half-width character to full-width one,
                   e.g. U+FF71 -> U+30A2, U+FF8C U+FF9F -> U+30D7. */
                if (map_to_fullwidth( src, srclen, &wch ) == 2)
                {
                    src++;
                    srclen--;
                }
            }
            else wch = *src;

            if (flags & LCMAP_KATAKANA)
            {
                /* map hiragana to katakana, e.g. U+3041 -> U+30A1.
                   we can't use C3_HIRAGANA as some characters can't map to katakana */
                if ((wch >= 0x3041 && wch <= 0x3096) || wch == 0x309D || wch == 0x309E) wch += 0x60;
            }
            else if (flags & LCMAP_HIRAGANA)
            {
                /* map katakana to hiragana, e.g. U+30A1 -> U+3041.
                   we can't use C3_KATAKANA as some characters can't map to hiragana */
                if ((wch >= 0x30A1 && wch <= 0x30F6) || wch == 0x30FD || wch == 0x30FE) wch -= 0x60;
            }

            if (flags & LCMAP_HALFWIDTH)
            {
                /* map full-width character to half-width one,
                   e.g. U+30A2 -> U+FF71, U+30D7 -> U+FF8C U+FF9F. */
                if (map_to_halfwidth(wch, dst_ptr, len) == 2)
                {
                    len--;
                    dst_ptr++;
                    if (!len) break;
                }
            }
            else *dst_ptr = wch;
        }
        if (!(flags & (LCMAP_UPPERCASE | LCMAP_LOWERCASE)) || srclen) goto done;

        srclen = dst_ptr - dst;
        src = dst;
    }

    if (flags & (LCMAP_UPPERCASE | LCMAP_LOWERCASE))
    {
        const USHORT *table = sort.casemap + (flags & LCMAP_LINGUISTIC_CASING ? sortid->casemap : 0);
        table = table + 2 + (flags & LCMAP_LOWERCASE ? table[1] : 0);
        for (len = dstlen, dst_ptr = dst; srclen && len; src++, srclen--, len--)
            *dst_ptr++ = casemap( table, *src );
    }
    else
    {
        len = min( srclen, dstlen );
        memcpy( dst, src, len * sizeof(WCHAR) );
        dst_ptr = dst + len;
        srclen -= len;
    }

done:
    if (srclen)
    {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    return dst_ptr - dst;
}


/***********************************************************************
 *	LCMapStringA   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCMapStringA( LCID lcid, DWORD flags, const char *src, int srclen,
                                           char *dst, int dstlen )
{
    WCHAR *bufW = NtCurrentTeb()->StaticUnicodeBuffer;
    LPWSTR srcW, dstW;
    INT ret = 0, srclenW, dstlenW;
    UINT locale_cp = CP_ACP;

    if (!src || !srclen || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    locale_cp = get_lcid_codepage( lcid, flags );

    srclenW = MultiByteToWideChar( locale_cp, 0, src, srclen, bufW, 260 );
    if (srclenW) srcW = bufW;
    else
    {
        srclenW = MultiByteToWideChar( locale_cp, 0, src, srclen, NULL, 0 );
        srcW = HeapAlloc( GetProcessHeap(), 0, srclenW * sizeof(WCHAR) );
        if (!srcW)
        {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return 0;
        }
        MultiByteToWideChar( locale_cp, 0, src, srclen, srcW, srclenW );
    }

    if (flags & LCMAP_SORTKEY)
    {
        if (src == dst)
        {
            SetLastError( ERROR_INVALID_FLAGS );
            goto done;
        }
        ret = LCMapStringEx( NULL, flags, srcW, srclenW, (WCHAR *)dst, dstlen, NULL, NULL, 0 );
        goto done;
    }

    if (flags & SORT_STRINGSORT)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        goto done;
    }

    dstlenW = LCMapStringEx( NULL, flags, srcW, srclenW, NULL, 0, NULL, NULL, 0 );
    if (!dstlenW) goto done;

    dstW = HeapAlloc( GetProcessHeap(), 0, dstlenW * sizeof(WCHAR) );
    if (!dstW)
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        goto done;
    }
    LCMapStringEx( NULL, flags, srcW, srclenW, dstW, dstlenW, NULL, NULL, 0 );
    ret = WideCharToMultiByte( locale_cp, 0, dstW, dstlenW, dst, dstlen, NULL, NULL );
    HeapFree( GetProcessHeap(), 0, dstW );

done:
    if (srcW != bufW) HeapFree( GetProcessHeap(), 0, srcW );
    return ret;
}


/***********************************************************************
 *	LCMapStringW   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH LCMapStringW( LCID lcid, DWORD flags, const WCHAR *src, int srclen,
                                           WCHAR *dst, int dstlen )
{
    return LCMapStringEx( NULL, flags, src, srclen, dst, dstlen, NULL, NULL, 0 );
}


/***********************************************************************
 *	LocaleNameToLCID   (kernelbase.@)
 */
LCID WINAPI DECLSPEC_HOTPATCH LocaleNameToLCID( const WCHAR *name, DWORD flags )
{
    LCID lcid;
    const NLS_LOCALE_DATA *locale = get_locale_by_name( name, &lcid );

    if (!locale)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(flags & LOCALE_ALLOW_NEUTRAL_NAMES) && !locale->inotneutral)
        lcid = locale->idefaultlanguage;
    return lcid;
}


/******************************************************************************
 *	MultiByteToWideChar   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH MultiByteToWideChar( UINT codepage, DWORD flags, const char *src, INT srclen,
                                                  WCHAR *dst, INT dstlen )
{
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (srclen < 0) srclen = strlen(src) + 1;

    switch (codepage)
    {
    case CP_SYMBOL:
        ret = mbstowcs_cpsymbol( flags, src, srclen, dst, dstlen );
        break;
    case CP_UTF7:
        ret = mbstowcs_utf7( flags, src, srclen, dst, dstlen );
        break;
    case CP_UTF8:
        ret = mbstowcs_utf8( flags, src, srclen, dst, dstlen );
        break;
    case CP_UNIXCP:
        if (unix_cp == CP_UTF8)
        {
            ret = mbstowcs_utf8( flags, src, srclen, dst, dstlen );
            break;
        }
        codepage = unix_cp;
        /* fall through */
    default:
        ret = mbstowcs_codepage( codepage, flags, src, srclen, dst, dstlen );
        break;
    }
    TRACE( "cp %d %s -> %s, ret = %d\n", codepage, debugstr_an(src, srclen), debugstr_wn(dst, ret), ret );
    return ret;
}


/******************************************************************************
 *	NormalizeString   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH NormalizeString(NORM_FORM form, const WCHAR *src, INT src_len,
                                             WCHAR *dst, INT dst_len)
{
    NTSTATUS status = RtlNormalizeString( form, src, src_len, dst, &dst_len );

    switch (status)
    {
    case STATUS_OBJECT_NAME_NOT_FOUND:
        status = STATUS_INVALID_PARAMETER;
        break;
    case STATUS_BUFFER_TOO_SMALL:
    case STATUS_NO_UNICODE_TRANSLATION:
        dst_len = -dst_len;
        break;
    }
    SetLastError( RtlNtStatusToDosError( status ));
    return dst_len;
}


/******************************************************************************
 *	ResolveLocaleName   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH ResolveLocaleName( LPCWSTR name, LPWSTR buffer, INT len )
{
    FIXME( "stub: %s, %p, %d\n", wine_dbgstr_w(name), buffer, len );

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
    return 0;
}


/******************************************************************************
 *	SetLocaleInfoW   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetLocaleInfoW( LCID lcid, LCTYPE lctype, const WCHAR *data )
{
    const struct registry_value *value;
    DWORD index;
    LSTATUS status;

    lctype = LOWORD(lctype);
    value = get_locale_registry_value( lctype );

    if (!data || !value)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lctype == LOCALE_IDATE || lctype == LOCALE_ILDATE)
    {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    TRACE( "setting %lx (%s) to %s\n", lctype, debugstr_w(value->name), debugstr_w(data) );

    /* FIXME: should check that data to set is sane */

    status = RegSetValueExW( intl_key, value->name, 0, REG_SZ, (BYTE *)data, (lstrlenW(data)+1)*sizeof(WCHAR) );
    index = value - registry_values;

    RtlEnterCriticalSection( &locale_section );
    HeapFree( GetProcessHeap(), 0, registry_cache[index] );
    registry_cache[index] = NULL;
    RtlLeaveCriticalSection( &locale_section );

    if (lctype == LOCALE_SSHORTDATE || lctype == LOCALE_SLONGDATE)
    {
        /* Set I-value from S value */
        WCHAR *pD, *pM, *pY, buf[2];

        pD = wcschr( data, 'd' );
        pM = wcschr( data, 'M' );
        pY = wcschr( data, 'y' );

        if (pD <= pM) buf[0] = '1'; /* D-M-Y */
        else if (pY <= pM) buf[0] = '2'; /* Y-M-D */
        else buf[0] = '0'; /* M-D-Y */
        buf[1] = 0;

        lctype = (lctype == LOCALE_SSHORTDATE) ? LOCALE_IDATE : LOCALE_ILDATE;
        value = get_locale_registry_value( lctype );
        index = value - registry_values;

        RegSetValueExW( intl_key, value->name, 0, REG_SZ, (BYTE *)buf, sizeof(buf) );

        RtlEnterCriticalSection( &locale_section );
        HeapFree( GetProcessHeap(), 0, registry_cache[index] );
        registry_cache[index] = NULL;
        RtlLeaveCriticalSection( &locale_section );
    }
    return set_ntstatus( status );
}


/***********************************************************************
 *	SetCalendarInfoW   (kernelbase.@)
 */
INT WINAPI /* DECLSPEC_HOTPATCH */ SetCalendarInfoW( LCID lcid, CALID calendar, CALTYPE type, const WCHAR *data )
{
    FIXME( "(%08lx,%08lx,%08lx,%s): stub\n", lcid, calendar, type, debugstr_w(data) );
    return 0;
}


/***********************************************************************
 *      SetProcessPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetProcessPreferredUILanguages( DWORD flags, PCZZWSTR buffer, ULONG *count )
{
    return set_ntstatus( RtlSetProcessPreferredUILanguages( flags, buffer, count ));
}


/***********************************************************************
 *      SetThreadPreferredUILanguages   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetThreadPreferredUILanguages( DWORD flags, PCZZWSTR buffer, ULONG *count )
{
    return set_ntstatus( RtlSetThreadPreferredUILanguages( flags, buffer, count ));
}


/***********************************************************************
 *	SetTimeZoneInformation   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetTimeZoneInformation( const TIME_ZONE_INFORMATION *info )
{
    return set_ntstatus( RtlSetTimeZoneInformation( (const RTL_TIME_ZONE_INFORMATION *)info ));
}


/******************************************************************************
 *	SetUserGeoID   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SetUserGeoID( GEOID id )
{
    const struct geoinfo *geoinfo = get_geoinfo_ptr( id );
    WCHAR bufferW[10];
    HKEY hkey;

    if (!geoinfo)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    if (!RegCreateKeyExW( intl_key, L"Geo", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey, NULL ))
    {
        const WCHAR *name = geoinfo->kind == LOCATION_NATION ? L"Nation" : L"Region";
        swprintf( bufferW, ARRAY_SIZE(bufferW), L"%u", geoinfo->id );
        RegSetValueExW( hkey, name, 0, REG_SZ, (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );

        if (geoinfo->kind == LOCATION_NATION || geoinfo->kind == LOCATION_BOTH)
            lstrcpyW( bufferW, geoinfo->iso2W );
        else
            swprintf( bufferW, ARRAY_SIZE(bufferW), L"%03u", geoinfo->uncode );
        RegSetValueExW( hkey, L"Name", 0, REG_SZ, (BYTE *)bufferW, (lstrlenW(bufferW) + 1) * sizeof(WCHAR) );
        RegCloseKey( hkey );
    }
    return TRUE;
}


/***********************************************************************
 *	SystemTimeToTzSpecificLocalTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH SystemTimeToTzSpecificLocalTime( const TIME_ZONE_INFORMATION *info,
                                                               const SYSTEMTIME *system,
                                                               SYSTEMTIME *local )
{
    TIME_ZONE_INFORMATION tzinfo;
    LARGE_INTEGER ft;

    if (!info)
    {
        RtlQueryTimeZoneInformation( (RTL_TIME_ZONE_INFORMATION *)&tzinfo );
        info = &tzinfo;
    }

    if (!SystemTimeToFileTime( system, (FILETIME *)&ft )) return FALSE;
    switch (get_timezone_id( info, ft, FALSE ))
    {
    case TIME_ZONE_ID_UNKNOWN:
        ft.QuadPart -= info->Bias * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_STANDARD:
        ft.QuadPart -= (info->Bias + info->StandardBias) * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        ft.QuadPart -= (info->Bias + info->DaylightBias) * (LONGLONG)600000000;
        break;
    default:
        return FALSE;
    }
    return FileTimeToSystemTime( (FILETIME *)&ft, local );
}


/***********************************************************************
 *	TzSpecificLocalTimeToSystemTime   (kernelbase.@)
 */
BOOL WINAPI DECLSPEC_HOTPATCH TzSpecificLocalTimeToSystemTime( const TIME_ZONE_INFORMATION *info,
                                                               const SYSTEMTIME *local,
                                                               SYSTEMTIME *system )
{
    TIME_ZONE_INFORMATION tzinfo;
    LARGE_INTEGER ft;

    if (!info)
    {
        RtlQueryTimeZoneInformation( (RTL_TIME_ZONE_INFORMATION *)&tzinfo );
        info = &tzinfo;
    }

    if (!SystemTimeToFileTime( local, (FILETIME *)&ft )) return FALSE;
    switch (get_timezone_id( info, ft, TRUE ))
    {
    case TIME_ZONE_ID_UNKNOWN:
        ft.QuadPart += info->Bias * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_STANDARD:
        ft.QuadPart += (info->Bias + info->StandardBias) * (LONGLONG)600000000;
        break;
    case TIME_ZONE_ID_DAYLIGHT:
        ft.QuadPart += (info->Bias + info->DaylightBias) * (LONGLONG)600000000;
        break;
    default:
        return FALSE;
    }
    return FileTimeToSystemTime( (FILETIME *)&ft, system );
}


/***********************************************************************
 *	VerLanguageNameA   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH VerLanguageNameA( DWORD lang, LPSTR buffer, DWORD size )
{
    return GetLocaleInfoA( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SENGLANGUAGE, buffer, size );
}


/***********************************************************************
 *	VerLanguageNameW   (kernelbase.@)
 */
DWORD WINAPI DECLSPEC_HOTPATCH VerLanguageNameW( DWORD lang, LPWSTR buffer, DWORD size )
{
    return GetLocaleInfoW( MAKELCID( lang, SORT_DEFAULT ), LOCALE_SENGLANGUAGE, buffer, size );
}


/***********************************************************************
 *	WideCharToMultiByte   (kernelbase.@)
 */
INT WINAPI DECLSPEC_HOTPATCH WideCharToMultiByte( UINT codepage, DWORD flags, LPCWSTR src, INT srclen,
                                                  LPSTR dst, INT dstlen, LPCSTR defchar, BOOL *used )
{
    int ret;

    if (!src || !srclen || (!dst && dstlen) || dstlen < 0)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (srclen < 0) srclen = lstrlenW(src) + 1;

    switch (codepage)
    {
    case CP_SYMBOL:
        ret = wcstombs_cpsymbol( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UTF7:
        ret = wcstombs_utf7( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UTF8:
        ret = wcstombs_utf8( flags, src, srclen, dst, dstlen, defchar, used );
        break;
    case CP_UNIXCP:
        if (unix_cp == CP_UTF8)
        {
            if (used) *used = FALSE;
            ret = wcstombs_utf8( flags, src, srclen, dst, dstlen, NULL, NULL );
            break;
        }
        codepage = unix_cp;
        /* fall through */
    default:
        ret = wcstombs_codepage( codepage, flags, src, srclen, dst, dstlen, defchar, used );
        break;
    }
    TRACE( "cp %d %s -> %s, ret = %d\n", codepage, debugstr_wn(src, srclen), debugstr_an(dst, ret), ret );
    return ret;
}


/***********************************************************************
 *	GetUserDefaultGeoName  (kernelbase.@)
 */
INT WINAPI GetUserDefaultGeoName(LPWSTR geo_name, int count)
{
    const struct geoinfo *geoinfo;
    WCHAR buffer[32];
    LSTATUS status;
    DWORD size;
    HKEY key;

    TRACE( "geo_name %p, count %d.\n", geo_name, count );

    if (count && !geo_name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    if (!(status = RegOpenKeyExW( intl_key, L"Geo", 0, KEY_ALL_ACCESS, &key )))
    {
        size = sizeof(buffer);
        status = RegQueryValueExW( key, L"Name", NULL, NULL, (BYTE *)buffer, &size );
        RegCloseKey( key );
    }
    if (status)
    {
        if ((geoinfo = get_geoinfo_ptr( GetUserGeoID( GEOCLASS_NATION ))) && geoinfo->id != 39070)
            lstrcpyW( buffer, geoinfo->iso2W );
        else
            lstrcpyW( buffer, L"001" );
    }
    size = lstrlenW( buffer ) + 1;
    if (count < size)
    {
        if (!count)
            return size;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }
    lstrcpyW( geo_name, buffer );
    return size;
}


/***********************************************************************
 *	SetUserDefaultGeoName  (kernelbase.@)
 */
BOOL WINAPI SetUserGeoName(PWSTR geo_name)
{
    unsigned int i;
    WCHAR *endptr;
    int uncode;

    TRACE( "geo_name %s.\n", debugstr_w( geo_name ));

    if (!geo_name)
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (lstrlenW( geo_name ) == 3)
    {
        uncode = wcstol( geo_name, &endptr, 10 );
        if (!uncode || endptr != geo_name + 3)
        {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }
        for (i = 0; i < ARRAY_SIZE(geoinfodata); ++i)
            if (geoinfodata[i].uncode == uncode)
                break;
    }
    else
    {
        if (!lstrcmpiW( geo_name, L"XX" ))
            return SetUserGeoID( 39070 );
        for (i = 0; i < ARRAY_SIZE(geoinfodata); ++i)
            if (!lstrcmpiW( geo_name, geoinfodata[i].iso2W ))
                break;
    }
    if (i == ARRAY_SIZE(geoinfodata))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return SetUserGeoID( geoinfodata[i].id );
}
