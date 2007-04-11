/*
 * Nt time functions.
 *
 * RtlTimeToTimeFields, RtlTimeFieldsToTime and defines are taken from ReactOS and
 * adapted to wine with special permissions of the author. This code is
 * Copyright 2002 Rex Jolliff (rex@lvcablemodem.com)
 *
 * Copyright 1999 Juergen Schmied
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

static RTL_CRITICAL_SECTION TIME_GetBias_section;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &TIME_GetBias_section,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": TIME_GetBias_section") }
};
static RTL_CRITICAL_SECTION TIME_GetBias_section = { &critsect_debug, -1, 0, 0, 0, 0 };

/* TimeZone registry key values */
static const WCHAR TZInformationKeyW[] = { 'M','a','c','h','i','n','e','\\',
 'S','Y','S','T','E','M','\\','C','u','r','r','e','n','t','C','o','n','t','r',
 'o','l','S','e','t','\\','C','o','n','t','r','o','l','\\','T','i','m','e','z',
 'o','n','e','I','n','f','o','r','m','a','t','i','o','n', 0};
static const WCHAR TZStandardStartW[] = {
  'S','t','a','n','d','a','r','d','s','t','a','r','t', 0};
static const WCHAR TZDaylightStartW[] = {
  'D','a','y','l','i','g','h','t','s','t','a','r','t', 0};
static const WCHAR TZDaylightBiasW[] = {
    'D','a','y','l','i','g','h','t','B','i','a','s', 0};
static const WCHAR TZStandardBiasW[] = {
    'S','t','a','n','d','a','r','d','B','i','a','s', 0};
static const WCHAR TZBiasW[] = {'B','i','a','s', 0};
static const WCHAR TZDaylightNameW[] = {
    'D','a','y','l','i','g','h','t','N','a','m','e', 0};
static const WCHAR TZStandardNameW[] = {
    'S','t','a','n','d','a','r','d','N','a','m','e', 0};


#define SETTIME_MAX_ADJUST 120

/* This structure is used to store strings that represent all of the time zones
 * in the world. (This is used to help GetTimeZoneInformation)
 */
struct tagTZ_INFO
{
    const char *psTZFromUnix;
    WCHAR psTZWindows[32];
    int bias;
    int dst;
};

static const struct tagTZ_INFO TZ_INFO[] =
{
   {"MHT",
    {'D','a','t','e','l','i','n','e',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e','\0'}, -720, 0},
   {"SST",
    {'S','a','m','o','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, 660, 0},
   {"HST",
    {'H','a','w','a','i','i','a','n',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e','\0'}, 600, 0},
   {"AKST",
    {'A','l','a','s','k','a','n',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e',0}, 540, 0 },
   {"AKDT",
    {'A','l','a','s','k','a','n',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, 480, 1},
   {"PST",
    {'P','a','c','i','f','i','c',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, 480, 0},
   {"PDT",
    {'P','a','c','i','f','i','c',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, 420, 1},
   {"MST",
    {'U','S',' ','M','o','u','n','t','a','i','n',' ','S','t','a','n','d','a',
     'r','d',' ','T','i','m','e','\0'}, 420, 0},
   {"MDT",
    {'M','o','u','n','t','a','i','n',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e','\0'}, 360, 1},
   {"CST",
    {'C','e','n','t','r','a','l',' ','A','m','e','r','i','c','a',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e','\0'}, 360, 0},
   {"CDT",
    {'C','e','n','t','r','a','l',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, 300, 1},
   {"COT",
    {'S','A',' ','P','a','c','i','f','i','c',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 300, 0},
   {"PET",
    {'S','A',' ','P','a','c','i','f','i','c',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e',0}, 300, 0 },
   {"EDT",
    {'E','a','s','t','e','r','n',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, 240, 1},
   {"EST",
    {'U','S',' ','E','a','s','t','e','r','n',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 300, 0},
   {"ECT",
    {'E','a','s','t','e','r','n',' ','C','e','n','t','r','a','l',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 300, 0},
   {"ADT",
    {'A','t','l','a','n','t','i','c',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e','\0'}, 180, 1},
   {"VET",
    {'S','A',' ','W','e','s','t','e','r','n',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 240, 0},
   {"CLT",
    {'P','a','c','i','f','i','c',' ','S','A',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 240, 0},
   {"CLST",
    {'P','a','c','i','f','i','c',' ','S','A',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e',0}, 180, 1},
   {"AST",
    {'A','t','l','a','n','t','i','c',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e',0}, 240, 0 },
   {"NDT",
    {'N','e','w','f','o','u','n','d','l','a','n','d',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, 150, 1},
   {"NST",
    {'N','e','w','f','o','u','n','d','l','a','n','d',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e',0}, 210, 0 },
   {"BRT",
    {'B','r','a','z','i','l','i','a','n',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, 180, 0},
   {"BRST",
    {'B','r','a','z','i','l','i','a','n',' ','S','u','m','m','e','r',
     ' ','T','i','m','e','\0'}, 120, 1},
   {"ART",
    {'S','A',' ','E','a','s','t','e','r','n',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 180, 0},
   {"WGST",
    {'G','r','e','e','n','l','a','n','d',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, 120, 1},
   {"GST",
    {'M','i','d','-','A','t','l','a','n','t','i','c',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, 120, 0},
   {"AZOT",
    {'A','z','o','r','e','s',' ','S','t','a','n','d','a','r','d',' ','T','i',
     'm','e',0}, 60, 0},
   {"AZOST",
    {'A','z','o','r','e','s',' ','S','t','a','n','d','a','r','d',' ','T','i',
     'm','e','\0'}, 0, 1},
   {"CVT",
    {'C','a','p','e',' ','V','e','r','d','e',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, 60, 0},
   {"WEST",
    {'W','e','s','t','e','r','n',' ','E','u','r','o','p','e','a','n',' ','S','u','m','m','e','r',' ','T','i','m','e','\0'}, -60, 1},
   {"WET",
    {'G','r','e','e','n','w','i','c','h',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, 0, 0},
   {"BST",
    {'G','M','T',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e','\0'},
    -60, 1},
   {"IST",
    {'I','r','i','s','h',' ','S','u','m','m','e','r',' ','T','i','m','e','\0'},
    -60, 1},
   {"GMT",
    {'G','M','T',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e','\0'},
    0, 0},
   {"UTC",
    {'G','M','T',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e','\0'},
    0, 0},
   {"CET",
    {'C','e','n','t','r','a','l',' ','E','u','r','o','p','e','a','n',' ',
     'T','i','m','e','\0'}, -60, 0},
   {"CEST",
    {'C','e','n','t','r','a','l',' ','E','u','r','o','p','e',' ','S','t','a',
     'n','d','a','r','d',' ','T','i','m','e','\0'}, -120, 1},
   {"MET",
    {'C','e','n','t','r','a','l',' ','E','u','r','o','p','e',' ','S','t','a',
     'n','d','a','r','d',' ','T','i','m','e','\0'}, -60, 0},
   {"MEST",
    {'C','e','n','t','r','a','l',' ','E','u','r','o','p','e',' ','D','a','y',
     'l','i','g','h','t',' ','T','i','m','e','\0'}, -120, 1},
   {"WAT",
    {'W','.',' ','C','e','n','t','r','a','l',' ','A','f','r','i','c','a',' ',
     'S','t','a','n','d','a','r','d',' ','T','i','m','e','\0'}, -60, 0},
   {"EEST",
    {'E','.',' ','E','u','r','o','p','e',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, -180, 1},
   {"EET",
    {'E','g','y','p','t',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -120, 0},
   {"CAT",
    {'C','e','n','t','r','a','l',' ','A','f','r','i','c','a','n',' '
    ,'T','i','m','e','\0'}, -120, 0},
   {"SAST",
    {'S','o','u','t','h',' ','A','f','r','i','c','a',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, -120, 0},
   {"IST",
    {'I','s','r','a','e','l',' ','S','t','a','n','d','a','r','d',' ','T','i',
     'm','e','\0'}, -120, 0},
   {"MSK",
    {'R','u','s','s','i','a','n',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, -180, 0},
   {"ADT",
    {'A','r','a','b','i','c',' ','S','t','a','n','d','a','r','d',' ','T','i',
     'm','e','\0'}, -240, 1},
   {"AST",
    {'A','r','a','b',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',
     '\0'}, -180, 0},
   {"MSD",
    {'R','u','s','s','i','a','n',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, -240, 1},
   {"EAT",
    {'E','.',' ','A','f','r','i','c','a',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, -180, 0},
   {"IRST",
    {'I','r','a','n',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',
     0},-210, 0 },
   {"IRST",
    {'I','r','a','n',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',
     '\0'}, -270, 1},
   {"GST",
    {'A','r','a','b','i','a','n',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, -240, 0},
   {"AZT",
    {'C','a','u','c','a','s','u','s',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e',0}, -240, 0 },
   {"AZST",
    {'C','a','u','c','a','s','u','s',' ','S','t','a','n','d','a','r','d',' ',
     'T','i','m','e','\0'}, -300, 1},
   {"AFT",
    {'A','f','g','h','a','n','i','s','t','a','n',' ','S','t','a','n','d','a',
     'r','d',' ','T','i','m','e','\0'}, -270, 0},
   {"SAMT",
    {'S','a','m','a','r','a',' ','S','t','a','n','d','a','r','d',' ','T','i',
     'm','e','\0'}, -270, 1},
   {"YEKT",
    {'U','r','a','l','s',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e',' ','(','W','i','n','t','e','r',')','\0'}, -300, 0},
   {"YEKST",
    {'U','r','a','l','s',' ','D','a','y','l','i','g','h','t',
     ' ','T','i','m','e',' ','(','S','u','m','m','e','r',')','\0'}, -360, 1},
   {"OMST",
    {'O','m','s','k',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e',' ','(','W','i','n','t','e','r',')','\0'}, -360, 0},
   {"OMSST",
    {'O','m','s','k',' ','D','a','y','l','i','g','h','t',
     ' ','T','i','m','e',' ','(','S','u','m','m','e','r',')','\0'}, -420, 1},
   {"PKT",
    {'W','e','s','t',' ','A','s','i','a',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, -300, 0},
   {"IST",
    {'I','n','d','i','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -330, 0},
   {"NPT",
    {'N','e','p','a','l',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -345, 0},
   {"ALMST",
    {'N','.',' ','C','e','n','t','r','a','l',' ','A','s','i','a',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e','\0'}, -420, 1},
   {"BDT",
    {'C','e','n','t','r','a','l',' ','A','s','i','a',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, -360, 0},
   {"LKT",
    {'S','r','i',' ','L','a','n','k','a',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, -360, 0},
   {"MMT",
    {'M','y','a','n','m','a','r',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, -390, 0},
   {"ICT",
    {'S','E',' ','A','s','i','a',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, -420, 0},
   {"KRAT",
    {'N','o','r','t','h',' ','A','s','i','a',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e',0}, -420, 0},
   {"KRAST",
    {'N','o','r','t','h',' ','A','s','i','a',' ','S','t','a','n','d','a','r',
     'd',' ','T','i','m','e','\0'}, -480, 1},
   {"IRKT",
    {'N','o','r','t','h',' ','A','s','i','a',' ','E','a','s','t',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e',0}, -480, 0},
   {"CST",
    {'C','h','i','n','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -480, 0},
   {"IRKST",
    {'N','o','r','t','h',' ','A','s','i','a',' ','E','a','s','t',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e','\0'}, -540, 1},
   {"SGT",
    {'M','a','l','a','y',' ','P','e','n','i','n','s','u','l','a',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e','\0'}, -480, 0},
   {"WST",
    {'W','.',' ','A','u','s','t','r','a','l','i','a',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, -480, 0},
   {"WST",
    {'W','.',' ','A','u','s','t','r','a','l','i','a',' ','S','u','m','m','e',
     'r',' ','T','i','m','e','\0'}, -540, 1},
   {"JST",
    {'T','o','k','y','o',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -540, 0},
   {"KST",
    {'K','o','r','e','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -540, 0},
   {"YAKST",
    {'Y','a','k','u','t','s','k',' ','S','t','a','n','d','a','r','d',' ','T',
     'i','m','e','\0'}, -600, 1},
   {"CST",
    {'C','e','n','.',' ','A','u','s','t','r','a','l','i','a',' ','S','t','a',
     'n','d','a','r','d',' ','T','i','m','e','\0'}, -570, 0},
   {"CST",
    {'C','e','n','.',' ','A','u','s','t','r','a','l','i','a',' ','D','a','y',
     'l','i','g','h','t',' ','T','i','m','e','\0'}, -630, 1},
   {"EST",
    {'E','.',' ','A','u','s','t','r','a','l','i','a',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, -600, 0},
   {"EST",
    {'E','.',' ','A','u','s','t','r','a','l','i','a',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, -660, 1},
   {"GST",
    {'W','e','s','t',' ','P','a','c','i','f','i','c',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e','\0'}, -600, 0},
   {"VLAT",
    {'V','l','a','d','i','v','o','s','t','o','k',' ','S','t','a','n','d','a',
     'r','d',' ','T','i','m','e',0}, -600, 0 },
   {"ChST",
    {'W','e','s','t',' ','P','a','c','i','f','i','c',' ','S','t','a','n','d',
     'a','r','d',' ','T','i','m','e',0}, -600, 0},
   {"VLAST",
    {'V','l','a','d','i','v','o','s','t','o','k',' ','S','t','a','n','d','a',
     'r','d',' ','T','i','m','e','\0'}, -660, 1},
   {"MAGT",
    {'C','e','n','t','r','a','l',' ','P','a','c','i','f','i','c',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e',0}, -660, 0},
   {"MAGST",
    {'C','e','n','t','r','a','l',' ','P','a','c','i','f','i','c',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e','\0'}, -720, 1},
   {"NZST",
    {'N','e','w',' ','Z','e','a','l','a','n','d',' ','S','t','a','n','d','a',
     'r','d',' ','T','i','m','e','\0'}, -720, 0},
   {"NZDT",
    {'N','e','w',' ','Z','e','a','l','a','n','d',' ','D','a','y','l','i','g',
     'h','t',' ','T','i','m','e','\0'}, -780, 1},
   {"FJT",
    {'F','i','j','i',' ','S','t','a','n','d','a','r','d',' ','T','i','m','e',
     '\0'}, -720, 0},
   {"TOT",
    {'T','o','n','g','a',' ','S','t','a','n','d','a','r','d',' ','T','i','m',
     'e','\0'}, -780, 0},
   {"NOVT",
    {'N','.',' ','C','e','n','t','r','a','l',' ','A','s','i','a',' ','S','t',
     'a','n','d','a','r','d',' ','T','i','m','e',0 }, -360, 0},
   {"NOVT",
    {'N','o','v','o','s','i','b','i','r','s','k',' ','S','t','a','n','d','a',
     'r','d',' ','T','i','m','e','\0'}, -360, 1},
   {"ANAT",
    {'A','n','a','d','y','r',' ','S','t','a','n','d','a','r','d',' ','T','i',
     'm','e','\0'}, -720, 1},
   {"HKT",
    {'H','o','n','g',' ','K','o','n','g',' ','S','t','a','n','d','a','r','d',
     ' ','T','i','m','e','\0'}, -480, 0},
   {"UYT",
    {'U','r','u','g','u','a','y','a','n',' ','T','i','m','e','\0'}, 180, 0},
   {"UYST",
    {'U','r','u','g','u','a','y','a','n',' ','S','u','m','m','e','r',' ','T',
    'i','m','e','\0'}, 120, 1},
   {"MYT",
    {'M','a','l','a','y','s','i','a','n',' ','T','i','m','e','\0'}, -480, 0},
   {"PHT",
    {'P','h','i','l','i','p','p','i','n','e',' ','T','i','m','e','\0'}, -480, 0},
   {"NOVST",
    {'N','o','v','o','s','i','b','i','r','s','k',' ','S','u','m','m','e','r',
     ' ','T','i','m','e','\0'}, -480, 1},
   {"UZT",
    {'U','z','b','e','k','i','s','t','h','a','n',' ','T','i','m','e','\0'}, -300, 0}
};

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       1  /* Jan 1, 1601 was Monday */
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12
#define DAYSPERQUADRICENTENNIUM (365 * 400 + 97)
#define DAYSPERNORMALCENTURY (365 * 100 + 24)
#define DAYSPERNORMALQUADRENNIUM (365 * 4 + 1)

/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1970 (SECS_1601_TO_1970 * TICKSPERSEC)
/* 1601 to 1980 is 379 years plus 91 leap days */
#define SECS_1601_TO_1980  ((379 * 365 + 91) * (ULONGLONG)SECSPERDAY)
#define TICKS_1601_TO_1980 (SECS_1601_TO_1980 * TICKSPERSEC)
/* max ticks that can be represented as Unix time */
#define TICKS_1601_TO_UNIX_MAX ((SECS_1601_TO_1970 + INT_MAX) * TICKSPERSEC)


static const int MonthLengths[2][MONSPERYEAR] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline int IsLeapYear(int Year)
{
	return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

/***********************************************************************
 *              NTDLL_get_server_abstime
 *
 * Convert a NTDLL time into an abs_time_t struct to send to the server.
 */
void NTDLL_get_server_abstime( abs_time_t *when, const LARGE_INTEGER *timeout )
{
    UINT remainder;

    if (!timeout)  /* infinite timeout */
    {
        when->sec = when->usec = 0;
    }
    else if (timeout->QuadPart <= 0)  /* relative timeout */
    {
        struct timeval tv;

        if (-timeout->QuadPart > (LONGLONG)INT_MAX * TICKSPERSEC)
            when->sec = when->usec = INT_MAX;
        else
        {
            ULONG sec = RtlEnlargedUnsignedDivide( -timeout->QuadPart, TICKSPERSEC, &remainder );
            gettimeofday( &tv, 0 );
            when->sec = tv.tv_sec + sec;
            if ((when->usec = tv.tv_usec + (remainder / 10)) >= 1000000)
            {
                when->usec -= 1000000;
                when->sec++;
            }
            if (when->sec < tv.tv_sec)  /* overflow */
                when->sec = when->usec = INT_MAX;
        }
    }
    else  /* absolute time */
    {
        if (timeout->QuadPart < TICKS_1601_TO_1970)
            when->sec = when->usec = 0;
        else if (timeout->QuadPart > TICKS_1601_TO_UNIX_MAX)
            when->sec = when->usec = INT_MAX;
        else
        {
            when->sec = RtlEnlargedUnsignedDivide( timeout->QuadPart - TICKS_1601_TO_1970,
                                                   TICKSPERSEC, &remainder );
            when->usec = remainder / 10;
        }
    }
}


/***********************************************************************
 *              NTDLL_from_server_abstime
 *
 * Convert a timeval struct from the server into an NTDLL time.
 */
void NTDLL_from_server_abstime( LARGE_INTEGER *time, const abs_time_t *when )
{
    time->QuadPart = when->sec * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
    time->QuadPart += when->usec * 10;
}


/******************************************************************************
 *       RtlTimeToTimeFields [NTDLL.@]
 *
 * Convert a time into a TIME_FIELDS structure.
 *
 * PARAMS
 *   liTime     [I] Time to convert.
 *   TimeFields [O] Destination for the converted time.
 *
 * RETURNS
 *   Nothing.
 */
VOID WINAPI RtlTimeToTimeFields(
	const LARGE_INTEGER *liTime,
	PTIME_FIELDS TimeFields)
{
	int SecondsInDay;
        long int cleaps, years, yearday, months;
	long int Days;
	LONGLONG Time;

	/* Extract millisecond from time and convert time into seconds */
	TimeFields->Milliseconds =
            (CSHORT) (( liTime->QuadPart % TICKSPERSEC) / TICKSPERMSEC);
	Time = liTime->QuadPart / TICKSPERSEC;

	/* The native version of RtlTimeToTimeFields does not take leap seconds
	 * into account */

	/* Split the time into days and seconds within the day */
	Days = Time / SECSPERDAY;
	SecondsInDay = Time % SECSPERDAY;

	/* compute time of day */
	TimeFields->Hour = (CSHORT) (SecondsInDay / SECSPERHOUR);
	SecondsInDay = SecondsInDay % SECSPERHOUR;
	TimeFields->Minute = (CSHORT) (SecondsInDay / SECSPERMIN);
	TimeFields->Second = (CSHORT) (SecondsInDay % SECSPERMIN);

	/* compute day of week */
	TimeFields->Weekday = (CSHORT) ((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

        /* compute year, month and day of month. */
        cleaps=( 3 * ((4 * Days + 1227) / DAYSPERQUADRICENTENNIUM) + 3 ) / 4;
        Days += 28188 + cleaps;
        years = (20 * Days - 2442) / (5 * DAYSPERNORMALQUADRENNIUM);
        yearday = Days - (years * DAYSPERNORMALQUADRENNIUM)/4;
        months = (64 * yearday) / 1959;
        /* the result is based on a year starting on March.
         * To convert take 12 from Januari and Februari and
         * increase the year by one. */
        if( months < 14 ) {
            TimeFields->Month = months - 1;
            TimeFields->Year = years + 1524;
        } else {
            TimeFields->Month = months - 13;
            TimeFields->Year = years + 1525;
        }
        /* calculation of day of month is based on the wonderful
         * sequence of INT( n * 30.6): it reproduces the 
         * 31-30-31-30-31-31 month lengths exactly for small n's */
        TimeFields->Day = yearday - (1959 * months) / 64 ;
        return;
}

/******************************************************************************
 *       RtlTimeFieldsToTime [NTDLL.@]
 *
 * Convert a TIME_FIELDS structure into a time.
 *
 * PARAMS
 *   ftTimeFields [I] TIME_FIELDS structure to convert.
 *   Time         [O] Destination for the converted time.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE.
 */
BOOLEAN WINAPI RtlTimeFieldsToTime(
	PTIME_FIELDS tfTimeFields,
	PLARGE_INTEGER Time)
{
        int month, year, cleaps, day;

	/* FIXME: normalize the TIME_FIELDS structure here */
        /* No, native just returns 0 (error) if the fields are not */
        if( tfTimeFields->Milliseconds< 0 || tfTimeFields->Milliseconds > 999 ||
                tfTimeFields->Second < 0 || tfTimeFields->Second > 59 ||
                tfTimeFields->Minute < 0 || tfTimeFields->Minute > 59 ||
                tfTimeFields->Hour < 0 || tfTimeFields->Hour > 23 ||
                tfTimeFields->Month < 1 || tfTimeFields->Month > 12 ||
                tfTimeFields->Day < 1 ||
                tfTimeFields->Day > MonthLengths
                    [ tfTimeFields->Month ==2 || IsLeapYear(tfTimeFields->Year)]
                    [ tfTimeFields->Month - 1] ||
                tfTimeFields->Year < 1601 )
            return FALSE;

        /* now calculate a day count from the date
         * First start counting years from March. This way the leap days
         * are added at the end of the year, not somewhere in the middle.
         * Formula's become so much less complicate that way.
         * To convert: add 12 to the month numbers of Jan and Feb, and 
         * take 1 from the year */
        if(tfTimeFields->Month < 3) {
            month = tfTimeFields->Month + 13;
            year = tfTimeFields->Year - 1;
        } else {
            month = tfTimeFields->Month + 1;
            year = tfTimeFields->Year;
        }
        cleaps = (3 * (year / 100) + 3) / 4;   /* nr of "century leap years"*/
        day =  (36525 * year) / 100 - cleaps + /* year * dayperyr, corrected */
                 (1959 * month) / 64 +         /* months * daypermonth */
                 tfTimeFields->Day -          /* day of the month */
                 584817 ;                      /* zero that on 1601-01-01 */
        /* done */
        
        Time->QuadPart = (((((LONGLONG) day * HOURSPERDAY +
            tfTimeFields->Hour) * MINSPERHOUR +
            tfTimeFields->Minute) * SECSPERMIN +
            tfTimeFields->Second ) * 1000 +
            tfTimeFields->Milliseconds ) * TICKSPERMSEC;

        return TRUE;
}

/***********************************************************************
 *       TIME_GetBias [internal]
 *
 * Helper function calculates delta local time from UTC. 
 *
 * PARAMS
 *   utc [I] The current utc time.
 *   pdaylight [I] Local daylight.
 *
 * RETURNS
 *   The bias for the current timezone.
 */
static int TIME_GetBias(time_t utc, int *pdaylight)
{
    struct tm *ptm;
    static time_t last_utc;
    static int last_bias;
    static int last_daylight;
    int ret;

    RtlEnterCriticalSection( &TIME_GetBias_section );
    if(utc == last_utc)
    {
        *pdaylight = last_daylight;
        ret = last_bias;	
    } else
    {
        ptm = localtime(&utc);
	*pdaylight = last_daylight =
            ptm->tm_isdst; /* daylight for local timezone */
	ptm = gmtime(&utc);
	ptm->tm_isdst = *pdaylight; /* use local daylight, not that of Greenwich */
	last_utc = utc;
	ret = last_bias = (int)(utc-mktime(ptm));
    }
    RtlLeaveCriticalSection( &TIME_GetBias_section );
    return ret;
}

/******************************************************************************
 *        RtlLocalTimeToSystemTime [NTDLL.@]
 *
 * Convert a local time into system time.
 *
 * PARAMS
 *   LocalTime  [I] Local time to convert.
 *   SystemTime [O] Destination for the converted time.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: An NTSTATUS error code indicating the problem.
 */
NTSTATUS WINAPI RtlLocalTimeToSystemTime( const LARGE_INTEGER *LocalTime,
                                          PLARGE_INTEGER SystemTime)
{
    time_t gmt;
    int bias, daylight;

    TRACE("(%p, %p)\n", LocalTime, SystemTime);

    gmt = time(NULL);
    bias = TIME_GetBias(gmt, &daylight);

    SystemTime->QuadPart = LocalTime->QuadPart - bias * (LONGLONG)TICKSPERSEC;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *       RtlSystemTimeToLocalTime [NTDLL.@]
 *
 * Convert a system time into a local time.
 *
 * PARAMS
 *   SystemTime [I] System time to convert.
 *   LocalTime  [O] Destination for the converted time.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: An NTSTATUS error code indicating the problem.
 */
NTSTATUS WINAPI RtlSystemTimeToLocalTime( const LARGE_INTEGER *SystemTime,
                                          PLARGE_INTEGER LocalTime )
{
    time_t gmt;
    int bias, daylight;

    TRACE("(%p, %p)\n", SystemTime, LocalTime);

    gmt = time(NULL);
    bias = TIME_GetBias(gmt, &daylight);

    LocalTime->QuadPart = SystemTime->QuadPart + bias * (LONGLONG)TICKSPERSEC;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *       RtlTimeToSecondsSince1970 [NTDLL.@]
 *
 * Convert a time into a count of seconds since 1970.
 *
 * PARAMS
 *   Time    [I] Time to convert.
 *   Seconds [O] Destination for the converted time.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE, if the resulting value will not fit in a DWORD.
 */
BOOLEAN WINAPI RtlTimeToSecondsSince1970( const LARGE_INTEGER *Time, LPDWORD Seconds )
{
    ULONGLONG tmp = ((ULONGLONG)Time->u.HighPart << 32) | Time->u.LowPart;
    tmp = RtlLargeIntegerDivide( tmp, TICKSPERSEC, NULL );
    tmp -= SECS_1601_TO_1970;
    if (tmp > 0xffffffff) return FALSE;
    *Seconds = (DWORD)tmp;
    return TRUE;
}

/******************************************************************************
 *       RtlTimeToSecondsSince1980 [NTDLL.@]
 *
 * Convert a time into a count of seconds since 1980.
 *
 * PARAMS
 *   Time    [I] Time to convert.
 *   Seconds [O] Destination for the converted time.
 *
 * RETURNS
 *   Success: TRUE.
 *   Failure: FALSE, if the resulting value will not fit in a DWORD.
 */
BOOLEAN WINAPI RtlTimeToSecondsSince1980( const LARGE_INTEGER *Time, LPDWORD Seconds )
{
    ULONGLONG tmp = ((ULONGLONG)Time->u.HighPart << 32) | Time->u.LowPart;
    tmp = RtlLargeIntegerDivide( tmp, TICKSPERSEC, NULL );
    tmp -= SECS_1601_TO_1980;
    if (tmp > 0xffffffff) return FALSE;
    *Seconds = (DWORD)tmp;
    return TRUE;
}

/******************************************************************************
 *       RtlSecondsSince1970ToTime [NTDLL.@]
 *
 * Convert a count of seconds since 1970 to a time.
 *
 * PARAMS
 *   Seconds [I] Time to convert.
 *   Time    [O] Destination for the converted time.
 *
 * RETURNS
 *   Nothing.
 */
void WINAPI RtlSecondsSince1970ToTime( DWORD Seconds, LARGE_INTEGER *Time )
{
    ULONGLONG secs = Seconds * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
    Time->u.LowPart  = (DWORD)secs;
    Time->u.HighPart = (DWORD)(secs >> 32);
}

/******************************************************************************
 *       RtlSecondsSince1980ToTime [NTDLL.@]
 *
 * Convert a count of seconds since 1980 to a time.
 *
 * PARAMS
 *   Seconds [I] Time to convert.
 *   Time    [O] Destination for the converted time.
 *
 * RETURNS
 *   Nothing.
 */
void WINAPI RtlSecondsSince1980ToTime( DWORD Seconds, LARGE_INTEGER *Time )
{
    ULONGLONG secs = Seconds * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1980;
    Time->u.LowPart  = (DWORD)secs;
    Time->u.HighPart = (DWORD)(secs >> 32);
}

/******************************************************************************
 *       RtlTimeToElapsedTimeFields [NTDLL.@]
 *
 * Convert a time to a count of elapsed seconds.
 *
 * PARAMS
 *   Time       [I] Time to convert.
 *   TimeFields [O] Destination for the converted time.
 *
 * RETURNS
 *   Nothing.
 */
void WINAPI RtlTimeToElapsedTimeFields( const LARGE_INTEGER *Time, PTIME_FIELDS TimeFields )
{
    LONGLONG time;
    INT rem;

    time = RtlExtendedLargeIntegerDivide( Time->QuadPart, TICKSPERSEC, &rem );
    TimeFields->Milliseconds = rem / TICKSPERMSEC;

    /* time is now in seconds */
    TimeFields->Year  = 0;
    TimeFields->Month = 0;
    TimeFields->Day   = RtlExtendedLargeIntegerDivide( time, SECSPERDAY, &rem );

    /* rem is now the remaining seconds in the last day */
    TimeFields->Second = rem % 60;
    rem /= 60;
    TimeFields->Minute = rem % 60;
    TimeFields->Hour = rem / 60;
}

/***********************************************************************
 *       NtQuerySystemTime [NTDLL.@]
 *       ZwQuerySystemTime [NTDLL.@]
 *
 * Get the current system time.
 *
 * PARAMS
 *   Time [O] Destination for the current system time.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: An NTSTATUS error code indicating the problem.
 */
NTSTATUS WINAPI NtQuerySystemTime( PLARGE_INTEGER Time )
{
    struct timeval now;

    gettimeofday( &now, 0 );
    Time->QuadPart = now.tv_sec * (ULONGLONG)TICKSPERSEC + TICKS_1601_TO_1970;
    Time->QuadPart += now.tv_usec * 10;
    return STATUS_SUCCESS;
}

/******************************************************************************
 *  NtQueryPerformanceCounter	[NTDLL.@]
 *
 *  Note: Windows uses a timer clocked at a multiple of 1193182 Hz. There is a
 *  good number of applications that crash when the returned frequency is either
 *  lower or higher then what Windows gives. Also too high counter values are
 *  reported to give problems.
 */
NTSTATUS WINAPI NtQueryPerformanceCounter( PLARGE_INTEGER Counter, PLARGE_INTEGER Frequency )
{
    struct timeval now;

    if (!Counter) return STATUS_ACCESS_VIOLATION;
    gettimeofday( &now, 0 );
    /* convert a counter that increments at a rate of 1 MHz
     * to one of 1.193182 MHz, with some care for arithmetic
     * overflow ( will not overflow for 5000 years ) and
     * good accuracy ( 105/88 = 1.19318182) */
    Counter->QuadPart = (((now.tv_sec - server_start_time.sec) * (ULONGLONG)1000000 +
                          (now.tv_usec - server_start_time.usec)) * 105) / 88;
    if (Frequency) Frequency->QuadPart = 1193182;
    return STATUS_SUCCESS;
}


/******************************************************************************
 * NtGetTickCount   (NTDLL.@)
 * ZwGetTickCount   (NTDLL.@)
 */
ULONG WINAPI NtGetTickCount(void)
{
    struct timeval current_time;

    gettimeofday(&current_time, NULL);
    return (current_time.tv_sec - server_start_time.sec) * 1000 +
           (current_time.tv_usec - server_start_time.usec) / 1000;
}


/***********************************************************************
 *        TIME_GetTZAsStr [internal]
 *
 * Helper function that returns the given timezone as a string.
 *
 * PARAMS
 *   utc [I] The current utc time.
 *   bias [I] The bias of the current timezone.
 *   dst [I] ??
 *
 * RETURNS
 *   Timezone name.
 *
 * NOTES:
 *   This could be done with a hash table instead of merely iterating through a
 *   table, however with the small amount of entries (60 or so) I didn't think
 *   it was worth it.
 */
static const WCHAR* TIME_GetTZAsStr (time_t utc, int bias, int dst)
{
   char psTZName[7];
   struct tm *ptm = localtime(&utc);
   unsigned int i;

   if (!strftime (psTZName, 7, "%Z", ptm))
      return (NULL);

   for (i=0; i<(sizeof(TZ_INFO) / sizeof(struct tagTZ_INFO)); i++)
   {
      if ( strcmp(TZ_INFO[i].psTZFromUnix, psTZName) == 0 &&
           TZ_INFO[i].bias == bias &&
           TZ_INFO[i].dst == dst
         )
            return TZ_INFO[i].psTZWindows;
   }
   FIXME("Can't match system time zone name \"%s\", bias=%d and dst=%d "
         "to an entry in TZ_INFO. Please add appropriate entry to "
         "TZ_INFO and submit as patch to wine-patches\n",psTZName,bias,dst);
   return NULL;
}

/***  TIME_GetTimeZoneInfoFromReg: helper for GetTimeZoneInformation ***/


static int TIME_GetTimeZoneInfoFromReg(RTL_TIME_ZONE_INFORMATION *tzinfo)
{
    BYTE buf[90];
    KEY_VALUE_PARTIAL_INFORMATION * KpInfo =
        (KEY_VALUE_PARTIAL_INFORMATION *) buf;
    HANDLE hkey;
    DWORD size;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, TZInformationKeyW);
    if (!NtOpenKey( &hkey, KEY_ALL_ACCESS, &attr )) {

#define GTZIFR_N( valkey, tofield) \
        RtlInitUnicodeString( &nameW, valkey );\
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, KpInfo,\
                    sizeof(buf), &size )) { \
            if( size >= (sizeof((tofield)) + \
                    offsetof(KEY_VALUE_PARTIAL_INFORMATION,Data))) { \
                memcpy(&(tofield), \
                        KpInfo->Data, sizeof(tofield)); \
            } \
        }
#define GTZIFR_S( valkey, tofield) \
        RtlInitUnicodeString( &nameW, valkey );\
        if (!NtQueryValueKey( hkey, &nameW, KeyValuePartialInformation, KpInfo,\
                    sizeof(buf), &size )) { \
            size_t len = (strlenW( (WCHAR*)KpInfo->Data ) + 1) * sizeof(WCHAR); \
            if (len > sizeof(tofield)) len = sizeof(tofield); \
            memcpy( tofield, KpInfo->Data, len ); \
            tofield[(len/sizeof(WCHAR))-1] = 0; \
        }

        GTZIFR_N( TZStandardStartW,  tzinfo->StandardDate)
        GTZIFR_N( TZDaylightStartW,  tzinfo->DaylightDate)
        GTZIFR_N( TZBiasW,          tzinfo->Bias)
        GTZIFR_N( TZStandardBiasW,  tzinfo->StandardBias)
        GTZIFR_N( TZDaylightBiasW,  tzinfo->DaylightBias)
        GTZIFR_S( TZStandardNameW, tzinfo->StandardName)
        GTZIFR_S( TZDaylightNameW, tzinfo->DaylightName)

#undef GTZIFR_N
#undef GTZIFR_S
        NtClose( hkey );
        return 1;
    }
    return 0;
}

/***********************************************************************
 *      RtlQueryTimeZoneInformation [NTDLL.@]
 *
 * Get information about the current timezone.
 *
 * PARAMS
 *   tzinfo [O] Destination for the retrieved timezone info.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: An NTSTATUS error code indicating the problem.
 */
NTSTATUS WINAPI RtlQueryTimeZoneInformation(RTL_TIME_ZONE_INFORMATION *tzinfo)
{
    time_t gmt;
    int bias, daylight;
    const WCHAR *psTZ;

    memset(tzinfo, 0, sizeof(RTL_TIME_ZONE_INFORMATION));

    if( !TIME_GetTimeZoneInfoFromReg(tzinfo)) {

        gmt = time(NULL);
        bias = TIME_GetBias(gmt, &daylight);

        tzinfo->Bias = -bias / 60;
        tzinfo->StandardBias = 0;
        tzinfo->DaylightBias = -60;
        tzinfo->StandardName[0]='\0';
        tzinfo->DaylightName[0]='\0';
        psTZ = TIME_GetTZAsStr (gmt, (-bias/60), daylight);
        if (psTZ) strcpyW( tzinfo->StandardName, psTZ );
        }
    return STATUS_SUCCESS;
}

/***********************************************************************
 *       RtlSetTimeZoneInformation [NTDLL.@]
 *
 * Set the current time zone information.
 *
 * PARAMS
 *   tzinfo [I] Timezone information to set.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: An NTSTATUS error code indicating the problem.
 *
 */
NTSTATUS WINAPI RtlSetTimeZoneInformation( const RTL_TIME_ZONE_INFORMATION *tzinfo )
{
    return STATUS_PRIVILEGE_NOT_HELD;
}

/***********************************************************************
 *        NtSetSystemTime [NTDLL.@]
 *        ZwSetSystemTime [NTDLL.@]
 *
 * Set the system time.
 *
 * PARAMS
 *   NewTime [I] The time to set.
 *   OldTime [O] Optional destination for the previous system time.
 *
 * RETURNS
 *   Success: STATUS_SUCCESS.
 *   Failure: An NTSTATUS error code indicating the problem.
 */
NTSTATUS WINAPI NtSetSystemTime(const LARGE_INTEGER *NewTime, LARGE_INTEGER *OldTime)
{
    TIME_FIELDS tf;
    struct timeval tv;
    struct timezone tz;
    struct tm t;
    time_t sec, oldsec;
    int dst, bias;
    int err;

    /* Return the old time if necessary */
    if(OldTime)
        NtQuerySystemTime(OldTime);

    RtlTimeToTimeFields(NewTime, &tf);

    /* call gettimeofday to get the current timezone */
    gettimeofday(&tv, &tz);
    oldsec = tv.tv_sec;
    /* get delta local time from utc */
    bias = TIME_GetBias(oldsec, &dst);

    /* get the number of seconds */
    t.tm_sec = tf.Second;
    t.tm_min = tf.Minute;
    t.tm_hour = tf.Hour;
    t.tm_mday = tf.Day;
    t.tm_mon = tf.Month - 1;
    t.tm_year = tf.Year - 1900;
    t.tm_isdst = dst;
    sec = mktime (&t);
    /* correct for timezone and daylight */
    sec += bias;

    /* set the new time */
    tv.tv_sec = sec;
    tv.tv_usec = tf.Milliseconds * 1000;

    /* error and sanity check*/
    if(sec == (time_t)-1 || abs((int)(sec-oldsec)) > SETTIME_MAX_ADJUST) {
        err = 2;
    } else {
#ifdef HAVE_SETTIMEOFDAY
        err = settimeofday(&tv, NULL); /* 0 is OK, -1 is error */
        if(err == 0)
            return STATUS_SUCCESS;
#else
        err = 1;
#endif
    }

    ERR("Cannot set time to %d/%d/%d %d:%d:%d Time adjustment %ld %s\n",
            tf.Year, tf.Month, tf.Day, tf.Hour, tf.Minute, tf.Second,
            (long)(sec-oldsec),
            err == -1 ? "No Permission"
                      : sec == (time_t)-1 ? "" : "is too large." );

    if(err == 2)
        return STATUS_INVALID_PARAMETER;
    else if(err == -1)
        return STATUS_PRIVILEGE_NOT_HELD;
    else
        return STATUS_NOT_IMPLEMENTED;
}
