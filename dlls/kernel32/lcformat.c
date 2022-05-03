/*
 * Locale-dependent format handling
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
 * Copyright 2003 Jon Griffiths
 * Copyright 2005 Dmitry Timoshkov
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

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
#include "winternl.h"

#include "kernel_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);

#define DATE_DATEVARSONLY 0x0100  /* only date stuff: yMdg */
#define TIME_TIMEVARSONLY 0x0200  /* only time stuff: hHmst */

/* Since calculating the formatting data for each locale is time-consuming,
 * we get the format data for each locale only once and cache it in memory.
 * We cache both the system default and user overridden data, after converting
 * them into the formats that the functions here expect. Since these functions
 * will typically be called with only a small number of the total locales
 * installed, the memory overhead is minimal while the speedup is significant.
 *
 * Our cache takes the form of a singly linked list, whose node is below:
 */
#define NLS_NUM_CACHED_STRINGS 57

typedef struct _NLS_FORMAT_NODE
{
  LCID  lcid;         /* Locale Id */
  DWORD dwFlags;      /* 0 or LOCALE_NOUSEROVERRIDE */
  DWORD dwCodePage;   /* Default code page (if LOCALE_USE_ANSI_CP not given) */
  LPWSTR lppszStrings[NLS_NUM_CACHED_STRINGS]; /* Default formats,day/month names */
  WCHAR szShortAM[2]; /* Short 'AM' marker */
  WCHAR szShortPM[2]; /* Short 'PM' marker */
  struct _NLS_FORMAT_NODE *next;
} NLS_FORMAT_NODE;

/* Macros to get particular data strings from a format node */
#define GetNegative(fmt)  fmt->lppszStrings[0]
#define GetLongDate(fmt)  fmt->lppszStrings[1]
#define GetShortDate(fmt) fmt->lppszStrings[2]
#define GetTime(fmt)      fmt->lppszStrings[3]
#define GetAM(fmt)        fmt->lppszStrings[54]
#define GetPM(fmt)        fmt->lppszStrings[55]
#define GetYearMonth(fmt) fmt->lppszStrings[56]

#define GetLongDay(fmt,day)       fmt->lppszStrings[4 + day]
#define GetShortDay(fmt,day)      fmt->lppszStrings[11 + day]
#define GetLongMonth(fmt,mth)     fmt->lppszStrings[18 + mth]
#define GetGenitiveMonth(fmt,mth) fmt->lppszStrings[30 + mth]
#define GetShortMonth(fmt,mth)    fmt->lppszStrings[42 + mth]

/* Write access to the cache is protected by this critical section */
static CRITICAL_SECTION NLS_FormatsCS;
static CRITICAL_SECTION_DEBUG NLS_FormatsCS_debug =
{
    0, 0, &NLS_FormatsCS,
    { &NLS_FormatsCS_debug.ProcessLocksList,
      &NLS_FormatsCS_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": NLS_Formats") }
};
static CRITICAL_SECTION NLS_FormatsCS = { &NLS_FormatsCS_debug, -1, 0, 0, 0, 0 };

/**************************************************************************
 * NLS_GetLocaleNumber <internal>
 *
 * Get a numeric locale format value.
 */
static DWORD NLS_GetLocaleNumber(LCID lcid, DWORD dwFlags)
{
  WCHAR szBuff[80];
  DWORD dwVal = 0;

  szBuff[0] = '\0';
  GetLocaleInfoW(lcid, dwFlags, szBuff, ARRAY_SIZE(szBuff));

  if (szBuff[0] && szBuff[1] == ';' && szBuff[2] != '0')
    dwVal = (szBuff[0] - '0') * 10 + (szBuff[2] - '0');
  else
  {
    const WCHAR* iter = szBuff;
    dwVal = 0;
    while(*iter >= '0' && *iter <= '9')
      dwVal = dwVal * 10 + (*iter++ - '0');
  }
  return dwVal;
}

/**************************************************************************
 * NLS_GetLocaleString <internal>
 *
 * Get a string locale format value.
 */
static WCHAR* NLS_GetLocaleString(LCID lcid, DWORD dwFlags)
{
  WCHAR szBuff[80], *str;
  DWORD dwLen;

  szBuff[0] = '\0';
  GetLocaleInfoW(lcid, dwFlags, szBuff, ARRAY_SIZE(szBuff));
  dwLen = lstrlenW(szBuff) + 1;
  str = HeapAlloc(GetProcessHeap(), 0, dwLen * sizeof(WCHAR));
  if (str)
    memcpy(str, szBuff, dwLen * sizeof(WCHAR));
  return str;
}

#define GET_LOCALE_NUMBER(num, type) num = NLS_GetLocaleNumber(lcid, type|dwFlags); \
  TRACE( #type ": %ld (%08lx)\n", (DWORD)num, (DWORD)num)

#define GET_LOCALE_STRING(str, type) str = NLS_GetLocaleString(lcid, type|dwFlags); \
  TRACE( #type ": %s\n", debugstr_w(str))

/**************************************************************************
 * NLS_GetFormats <internal>
 *
 * Calculate (and cache) the number formats for a locale.
 */
static const NLS_FORMAT_NODE *NLS_GetFormats(LCID lcid, DWORD dwFlags)
{
  /* GetLocaleInfo() identifiers for cached formatting strings */
  static const LCTYPE NLS_LocaleIndices[] = {
    LOCALE_SNEGATIVESIGN,
    LOCALE_SLONGDATE,   LOCALE_SSHORTDATE,
    LOCALE_STIMEFORMAT,
    LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3,
    LOCALE_SDAYNAME4, LOCALE_SDAYNAME5, LOCALE_SDAYNAME6, LOCALE_SDAYNAME7,
    LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3,
    LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6,
    LOCALE_SABBREVDAYNAME7,
    LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3,
    LOCALE_SMONTHNAME4, LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6,
    LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8, LOCALE_SMONTHNAME9,
    LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12,
    LOCALE_SMONTHNAME1  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME2  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME3  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME4  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME5  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME6  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME7  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME8  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME9  | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME10 | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME11 | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SMONTHNAME12 | LOCALE_RETURN_GENITIVE_NAMES,
    LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
    LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
    LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
    LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12,
    LOCALE_S1159, LOCALE_S2359,
    LOCALE_SYEARMONTH
  };
  static NLS_FORMAT_NODE *NLS_CachedFormats = NULL;
  NLS_FORMAT_NODE *node = NLS_CachedFormats;

  dwFlags &= LOCALE_NOUSEROVERRIDE;

  TRACE("(0x%04lx,0x%08lx)\n", lcid, dwFlags);

  /* See if we have already cached the locales number format */
  while (node && (node->lcid != lcid || node->dwFlags != dwFlags) && node->next)
    node = node->next;

  if (!node || node->lcid != lcid || node->dwFlags != dwFlags)
  {
    NLS_FORMAT_NODE *new_node;
    DWORD i;

    TRACE("Creating new cache entry\n");

    if (!(new_node = HeapAlloc(GetProcessHeap(), 0, sizeof(NLS_FORMAT_NODE))))
      return NULL;

    GET_LOCALE_NUMBER(new_node->dwCodePage, LOCALE_IDEFAULTANSICODEPAGE);

    /* Number Format */
    new_node->lcid = lcid;
    new_node->dwFlags = dwFlags;
    new_node->next = NULL;

    /* Date/Time Format info, negative character, etc */
    for (i = 0; i < ARRAY_SIZE(NLS_LocaleIndices); i++)
    {
      GET_LOCALE_STRING(new_node->lppszStrings[i], NLS_LocaleIndices[i]);
    }
    /* Save some memory if month genitive name is the same or not present */
    for (i = 0; i < 12; i++)
    {
      if (wcscmp(GetLongMonth(new_node, i), GetGenitiveMonth(new_node, i)) == 0)
      {
        HeapFree(GetProcessHeap(), 0, GetGenitiveMonth(new_node, i));
        GetGenitiveMonth(new_node, i) = NULL;
      }
    }

    new_node->szShortAM[0] = GetAM(new_node)[0]; new_node->szShortAM[1] = '\0';
    new_node->szShortPM[0] = GetPM(new_node)[0]; new_node->szShortPM[1] = '\0';

    /* Now add the computed format to the cache */
    RtlEnterCriticalSection(&NLS_FormatsCS);

    /* Search again: We may have raced to add the node */
    node = NLS_CachedFormats;
    while (node && (node->lcid != lcid || node->dwFlags != dwFlags) && node->next)
      node = node->next;

    if (!node)
    {
      node = NLS_CachedFormats = new_node; /* Empty list */
      new_node = NULL;
    }
    else if (node->lcid != lcid || node->dwFlags != dwFlags)
    {
      node->next = new_node; /* Not in the list, add to end */
      node = new_node;
      new_node = NULL;
    }

    RtlLeaveCriticalSection(&NLS_FormatsCS);

    if (new_node)
    {
      /* We raced and lost: The node was already added by another thread.
       * node points to the currently cached node, so free new_node.
       */
      for (i = 0; i < ARRAY_SIZE(NLS_LocaleIndices); i++)
        HeapFree(GetProcessHeap(), 0, new_node->lppszStrings[i]);
      HeapFree(GetProcessHeap(), 0, new_node);
    }
  }
  return node;
}

/**************************************************************************
 * NLS_IsUnicodeOnlyLcid <internal>
 *
 * Determine if a locale is Unicode only, and thus invalid in ANSI calls.
 */
static BOOL NLS_IsUnicodeOnlyLcid(LCID lcid)
{
  lcid = ConvertDefaultLocale(lcid);

  switch (PRIMARYLANGID(lcid))
  {
  case LANG_ARMENIAN:
  case LANG_DIVEHI:
  case LANG_GEORGIAN:
  case LANG_GUJARATI:
  case LANG_HINDI:
  case LANG_KANNADA:
  case LANG_KONKANI:
  case LANG_MARATHI:
  case LANG_PUNJABI:
  case LANG_SANSKRIT:
    TRACE("lcid 0x%08lx: langid 0x%04x is Unicode Only\n", lcid, PRIMARYLANGID(lcid));
    return TRUE;
  default:
    return FALSE;
  }
}

/*
 * Formatting of dates, times, numbers and currencies.
 */

#define IsLiteralMarker(p) (p == '\'')
#define IsDateFmtChar(p)   (p == 'd'||p == 'M'||p == 'y'||p == 'g')
#define IsTimeFmtChar(p)   (p == 'H'||p == 'h'||p == 'm'||p == 's'||p == 't')

/* Only the following flags can be given if a date/time format is specified */
#define DATE_FORMAT_FLAGS (DATE_DATEVARSONLY)
#define TIME_FORMAT_FLAGS (TIME_TIMEVARSONLY|TIME_FORCE24HOURFORMAT| \
                           TIME_NOMINUTESORSECONDS|TIME_NOSECONDS| \
                           TIME_NOTIMEMARKER)

/******************************************************************************
 * NLS_GetDateTimeFormatW <internal>
 *
 * Performs the formatting for GetDateFormatW/GetTimeFormatW.
 *
 * FIXME
 * DATE_USE_ALT_CALENDAR           - Requires GetCalendarInfo to work first.
 * DATE_LTRREADING/DATE_RTLREADING - Not yet implemented.
 */
static INT NLS_GetDateTimeFormatW(LCID lcid, DWORD dwFlags,
                                  const SYSTEMTIME* lpTime, LPCWSTR lpFormat,
                                  LPWSTR lpStr, INT cchOut)
{
  const NLS_FORMAT_NODE *node;
  SYSTEMTIME st;
  INT cchWritten = 0;
  INT lastFormatPos = 0;
  BOOL bSkipping = FALSE; /* Skipping text around marker? */
  BOOL d_dd_formatted = FALSE; /* previous formatted part was for d or dd */

  /* Verify our arguments */
  if ((cchOut && !lpStr) || !(node = NLS_GetFormats(lcid, dwFlags)))
    goto invalid_parameter;

  if (dwFlags & ~(DATE_DATEVARSONLY|TIME_TIMEVARSONLY))
  {
    if (lpFormat &&
        ((dwFlags & DATE_DATEVARSONLY && dwFlags & ~DATE_FORMAT_FLAGS) ||
         (dwFlags & TIME_TIMEVARSONLY && dwFlags & ~TIME_FORMAT_FLAGS)))
    {
      goto invalid_flags;
    }

    if (dwFlags & DATE_DATEVARSONLY)
    {
      if ((dwFlags & (DATE_LTRREADING|DATE_RTLREADING)) == (DATE_LTRREADING|DATE_RTLREADING))
        goto invalid_flags;
      else if (dwFlags & (DATE_LTRREADING|DATE_RTLREADING))
        FIXME("Unsupported flags: DATE_LTRREADING/DATE_RTLREADING\n");

      switch (dwFlags & (DATE_SHORTDATE|DATE_LONGDATE|DATE_YEARMONTH))
      {
      case 0:
        break;
      case DATE_SHORTDATE:
      case DATE_LONGDATE:
      case DATE_YEARMONTH:
        if (lpFormat)
          goto invalid_flags;
        break;
      default:
        goto invalid_flags;
      }
    }
  }

  if (!lpFormat)
  {
    /* Use the appropriate default format */
    if (dwFlags & DATE_DATEVARSONLY)
    {
      if (dwFlags & DATE_YEARMONTH)
        lpFormat = GetYearMonth(node);
      else if (dwFlags & DATE_LONGDATE)
        lpFormat = GetLongDate(node);
      else
        lpFormat = GetShortDate(node);
    }
    else
      lpFormat = GetTime(node);
  }

  if (!lpTime)
  {
    GetLocalTime(&st); /* Default to current time */
    lpTime = &st;
  }
  else
  {
    if (dwFlags & DATE_DATEVARSONLY)
    {
      FILETIME ftTmp;

      /* Verify the date and correct the D.O.W. if needed */
      memset(&st, 0, sizeof(st));
      st.wYear = lpTime->wYear;
      st.wMonth = lpTime->wMonth;
      st.wDay = lpTime->wDay;

      if (st.wDay > 31 || st.wMonth > 12 || !SystemTimeToFileTime(&st, &ftTmp))
        goto invalid_parameter;

      FileTimeToSystemTime(&ftTmp, &st);
      lpTime = &st;
    }

    if (dwFlags & TIME_TIMEVARSONLY)
    {
      /* Verify the time */
      if (lpTime->wHour > 24 || lpTime->wMinute > 59 || lpTime->wSecond > 59)
        goto invalid_parameter;
    }
  }

  /* Format the output */
  while (*lpFormat)
  {
    if (IsLiteralMarker(*lpFormat))
    {
      /* Start of a literal string */
      lpFormat++;

      /* Loop until the end of the literal marker or end of the string */
      while (*lpFormat)
      {
        if (IsLiteralMarker(*lpFormat))
        {
          lpFormat++;
          if (!IsLiteralMarker(*lpFormat))
            break; /* Terminating literal marker */
        }

        if (!cchOut)
          cchWritten++;   /* Count size only */
        else if (cchWritten >= cchOut)
          goto overrun;
        else if (!bSkipping)
        {
          lpStr[cchWritten] = *lpFormat;
          cchWritten++;
        }
        lpFormat++;
      }
    }
    else if ((dwFlags & DATE_DATEVARSONLY && IsDateFmtChar(*lpFormat)) ||
             (dwFlags & TIME_TIMEVARSONLY && IsTimeFmtChar(*lpFormat)))
    {
      WCHAR buff[32], fmtChar;
      LPCWSTR szAdd = NULL;
      DWORD dwVal = 0;
      int   count = 0, dwLen;

      bSkipping = FALSE;

      fmtChar = *lpFormat;
      while (*lpFormat == fmtChar)
      {
        count++;
        lpFormat++;
      }
      buff[0] = '\0';

      if (fmtChar != 'M') d_dd_formatted = FALSE;
      switch(fmtChar)
      {
      case 'd':
        if (count >= 4)
          szAdd = GetLongDay(node, (lpTime->wDayOfWeek + 6) % 7);
        else if (count == 3)
          szAdd = GetShortDay(node, (lpTime->wDayOfWeek + 6) % 7);
        else
        {
          dwVal = lpTime->wDay;
          szAdd = buff;
          d_dd_formatted = TRUE;
        }
        break;

      case 'M':
        if (count >= 4)
        {
          LPCWSTR genitive = GetGenitiveMonth(node, lpTime->wMonth - 1);
          if (genitive)
          {
            if (d_dd_formatted)
            {
              szAdd = genitive;
              break;
            }
            else
            {
              LPCWSTR format = lpFormat;
              /* Look forward now, if next format pattern is for day genitive
                 name should be used */
              while (*format)
              {
                /* Skip parts within markers */
                if (IsLiteralMarker(*format))
                {
                  ++format;
                  while (*format)
                  {
                    if (IsLiteralMarker(*format))
                    {
                      ++format;
                      if (!IsLiteralMarker(*format)) break;
                    }
                  }
                }
                if (*format != ' ') break;
                ++format;
              }
              /* Only numeric day form matters */
              if (*format == 'd')
              {
                INT dcount = 1;
                while (*++format == 'd') dcount++;
                if (dcount < 3)
                {
                  szAdd = genitive;
                  break;
                }
              }
            }
          }
          szAdd = GetLongMonth(node, lpTime->wMonth - 1);
        }
        else if (count == 3)
          szAdd = GetShortMonth(node, lpTime->wMonth - 1);
        else
        {
          dwVal = lpTime->wMonth;
          szAdd = buff;
        }
        break;

      case 'y':
        if (count >= 4)
        {
          count = 4;
          dwVal = lpTime->wYear;
        }
        else
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wYear % 100;
        }
        szAdd = buff;
        break;

      case 'g':
        if (count == 2)
        {
          /* FIXME: Our GetCalendarInfo() does not yet support CAL_SERASTRING.
           *        When it is fixed, this string should be cached in 'node'.
           */
          FIXME("Should be using GetCalendarInfo(CAL_SERASTRING), defaulting to 'AD'\n");
          buff[0] = 'A'; buff[1] = 'D'; buff[2] = '\0';
        }
        else
        {
          buff[0] = 'g'; buff[1] = '\0'; /* Add a literal 'g' */
        }
        szAdd = buff;
        break;

      case 'h':
        if (!(dwFlags & TIME_FORCE24HOURFORMAT))
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wHour == 0 ? 12 : (lpTime->wHour - 1) % 12 + 1;
          szAdd = buff;
          break;
        }
        /* .. fall through if we are forced to output in 24 hour format */

      case 'H':
        count = count > 2 ? 2 : count;
        dwVal = lpTime->wHour;
        szAdd = buff;
        break;

      case 'm':
        if (dwFlags & TIME_NOMINUTESORSECONDS)
        {
          cchWritten = lastFormatPos; /* Skip */
          bSkipping = TRUE;
        }
        else
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wMinute;
          szAdd = buff;
        }
        break;

      case 's':
        if (dwFlags & (TIME_NOSECONDS|TIME_NOMINUTESORSECONDS))
        {
          cchWritten = lastFormatPos; /* Skip */
          bSkipping = TRUE;
        }
        else
        {
          count = count > 2 ? 2 : count;
          dwVal = lpTime->wSecond;
          szAdd = buff;
        }
        break;

      case 't':
        if (dwFlags & TIME_NOTIMEMARKER)
        {
          cchWritten = lastFormatPos; /* Skip */
          bSkipping = TRUE;
        }
        else
        {
          if (count == 1)
            szAdd = lpTime->wHour < 12 ? node->szShortAM : node->szShortPM;
          else
            szAdd = lpTime->wHour < 12 ? GetAM(node) : GetPM(node);
        }
        break;
      }

      if (szAdd == buff && buff[0] == '\0')
      {
        /* We have a numeric value to add */
        swprintf(buff, ARRAY_SIZE(buff), L"%.*d", count, dwVal);
      }

      dwLen = szAdd ? lstrlenW(szAdd) : 0;

      if (cchOut && dwLen)
      {
        if (cchWritten + dwLen < cchOut)
          memcpy(lpStr + cchWritten, szAdd, dwLen * sizeof(WCHAR));
        else
        {
          memcpy(lpStr + cchWritten, szAdd, (cchOut - cchWritten) * sizeof(WCHAR));
          goto overrun;
        }
      }
      cchWritten += dwLen;
      lastFormatPos = cchWritten; /* Save position of last output format text */
    }
    else
    {
      /* Literal character */
      if (!cchOut)
        cchWritten++;   /* Count size only */
      else if (cchWritten >= cchOut)
        goto overrun;
      else if (!bSkipping || *lpFormat == ' ')
      {
        lpStr[cchWritten] = *lpFormat;
        cchWritten++;
      }
      lpFormat++;
    }
  }

  /* Final string terminator and sanity check */
  if (cchOut)
  {
   if (cchWritten >= cchOut)
     goto overrun;
   else
     lpStr[cchWritten] = '\0';
  }
  cchWritten++; /* Include terminating NUL */

  TRACE("returning length=%d, output=%s\n", cchWritten, debugstr_w(lpStr));
  return cchWritten;

overrun:
  TRACE("returning 0, (ERROR_INSUFFICIENT_BUFFER)\n");
  SetLastError(ERROR_INSUFFICIENT_BUFFER);
  return 0;

invalid_parameter:
  SetLastError(ERROR_INVALID_PARAMETER);
  return 0;

invalid_flags:
  SetLastError(ERROR_INVALID_FLAGS);
  return 0;
}

/******************************************************************************
 * NLS_GetDateTimeFormatA <internal>
 *
 * ANSI wrapper for GetDateFormatA/GetTimeFormatA.
 */
static INT NLS_GetDateTimeFormatA(LCID lcid, DWORD dwFlags,
                                  const SYSTEMTIME* lpTime,
                                  LPCSTR lpFormat, LPSTR lpStr, INT cchOut)
{
  DWORD cp = CP_ACP;
  WCHAR szFormat[128], szOut[128];
  INT iRet, cchOutW;

  TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n", lcid, dwFlags, lpTime,
        debugstr_a(lpFormat), lpStr, cchOut);

  if ((cchOut && !lpStr) || NLS_IsUnicodeOnlyLcid(lcid))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  if (!(dwFlags & LOCALE_USE_CP_ACP))
  {
    const NLS_FORMAT_NODE *node = NLS_GetFormats(lcid, dwFlags);
    if (!node)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
    }

    cp = node->dwCodePage;
  }

  if (lpFormat)
    MultiByteToWideChar(cp, 0, lpFormat, -1, szFormat, ARRAY_SIZE(szFormat));

  /* If cchOut == 0 we need the full string to get the accurate ANSI size */
  cchOutW = (cchOut && cchOut <= ARRAY_SIZE(szOut)) ? cchOut : ARRAY_SIZE(szOut);
  iRet = NLS_GetDateTimeFormatW(lcid, dwFlags, lpTime, lpFormat ? szFormat : NULL,
                                szOut, cchOutW);
  if (iRet)
      iRet = WideCharToMultiByte(cp, 0, szOut, -1, lpStr, cchOut, NULL, NULL);
  return iRet;
}

/******************************************************************************
 * GetDateFormatA [KERNEL32.@]
 *
 * Format a date for a given locale.
 *
 * PARAMS
 *  lcid      [I] Locale to format for
 *  dwFlags   [I] LOCALE_ and DATE_ flags from "winnls.h"
 *  lpTime    [I] Date to format
 *  lpFormat  [I] Format string, or NULL to use the system defaults
 *  lpDateStr [O] Destination for formatted string
 *  cchOut    [I] Size of lpDateStr, or 0 to calculate the resulting size
 *
 * NOTES
 *  - If lpFormat is NULL, lpDateStr will be formatted according to the format
 *    details returned by GetLocaleInfoA() and modified by dwFlags.
 *  - lpFormat is a string of characters and formatting tokens. Any characters
 *    in the string are copied verbatim to lpDateStr, with tokens being replaced
 *    by the date values they represent.
 *  - The following tokens have special meanings in a date format string:
 *|  Token  Meaning
 *|  -----  -------
 *|  d      Single digit day of the month (no leading 0)
 *|  dd     Double digit day of the month
 *|  ddd    Short name for the day of the week
 *|  dddd   Long name for the day of the week
 *|  M      Single digit month of the year (no leading 0)
 *|  MM     Double digit month of the year
 *|  MMM    Short name for the month of the year
 *|  MMMM   Long name for the month of the year
 *|  y      Double digit year number (no leading 0)
 *|  yy     Double digit year number
 *|  yyyy   Four digit year number
 *|  gg     Era string, for example 'AD'.
 *  - To output any literal character that could be misidentified as a token,
 *    enclose it in single quotes.
 *  - The ANSI version of this function fails if lcid is Unicode only.
 *
 * RETURNS
 *  Success: The number of character written to lpDateStr, or that would
 *           have been written, if cchOut is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetDateFormatA( LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                           LPCSTR lpFormat, LPSTR lpDateStr, INT cchOut)
{
  TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",lcid, dwFlags, lpTime,
        debugstr_a(lpFormat), lpDateStr, cchOut);

  return NLS_GetDateTimeFormatA(lcid, dwFlags | DATE_DATEVARSONLY, lpTime,
                                lpFormat, lpDateStr, cchOut);
}

/******************************************************************************
 * GetDateFormatEx [KERNEL32.@]
 *
 * Format a date for a given locale.
 *
 * PARAMS
 *  localename [I] Locale to format for
 *  flags      [I] LOCALE_ and DATE_ flags from "winnls.h"
 *  date       [I] Date to format
 *  format     [I] Format string, or NULL to use the locale defaults
 *  outbuf     [O] Destination for formatted string
 *  bufsize    [I] Size of outbuf, or 0 to calculate the resulting size
 *  calendar   [I] Reserved, must be NULL
 *
 * See GetDateFormatA for notes.
 *
 * RETURNS
 *  Success: The number of characters written to outbuf, or that would have
 *           been written if bufsize is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetDateFormatEx(LPCWSTR localename, DWORD flags,
                           const SYSTEMTIME* date, LPCWSTR format,
                           LPWSTR outbuf, INT bufsize, LPCWSTR calendar)
{
  TRACE("(%s,0x%08lx,%p,%s,%p,%d,%s)\n", debugstr_w(localename), flags,
        date, debugstr_w(format), outbuf, bufsize, debugstr_w(calendar));

  /* Parameter is currently reserved and Windows errors if set */
  if (calendar != NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return 0;
  }

  return NLS_GetDateTimeFormatW(LocaleNameToLCID(localename, 0),
                                flags | DATE_DATEVARSONLY, date, format,
                                outbuf, bufsize);
}

/******************************************************************************
 * GetDateFormatW	[KERNEL32.@]
 *
 * See GetDateFormatA.
 */
INT WINAPI GetDateFormatW(LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                          LPCWSTR lpFormat, LPWSTR lpDateStr, INT cchOut)
{
  TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n", lcid, dwFlags, lpTime,
        debugstr_w(lpFormat), lpDateStr, cchOut);

  return NLS_GetDateTimeFormatW(lcid, dwFlags|DATE_DATEVARSONLY, lpTime,
                                lpFormat, lpDateStr, cchOut);
}

/******************************************************************************
 *		GetTimeFormatA	[KERNEL32.@]
 *
 * Format a time for a given locale.
 *
 * PARAMS
 *  lcid      [I] Locale to format for
 *  dwFlags   [I] LOCALE_ and TIME_ flags from "winnls.h"
 *  lpTime    [I] Time to format
 *  lpFormat  [I] Formatting overrides
 *  lpTimeStr [O] Destination for formatted string
 *  cchOut    [I] Size of lpTimeStr, or 0 to calculate the resulting size
 *
 * NOTES
 *  - If lpFormat is NULL, lpszValue will be formatted according to the format
 *    details returned by GetLocaleInfoA() and modified by dwFlags.
 *  - lpFormat is a string of characters and formatting tokens. Any characters
 *    in the string are copied verbatim to lpTimeStr, with tokens being replaced
 *    by the time values they represent.
 *  - The following tokens have special meanings in a time format string:
 *|  Token  Meaning
 *|  -----  -------
 *|  h      Hours with no leading zero (12-hour clock)
 *|  hh     Hours with full two digits (12-hour clock)
 *|  H      Hours with no leading zero (24-hour clock)
 *|  HH     Hours with full two digits (24-hour clock)
 *|  m      Minutes with no leading zero
 *|  mm     Minutes with full two digits
 *|  s      Seconds with no leading zero
 *|  ss     Seconds with full two digits
 *|  t      Short time marker (e.g. "A" or "P")
 *|  tt     Long time marker (e.g. "AM", "PM")
 *  - To output any literal character that could be misidentified as a token,
 *    enclose it in single quotes.
 *  - The ANSI version of this function fails if lcid is Unicode only.
 *
 * RETURNS
 *  Success: The number of character written to lpTimeStr, or that would
 *           have been written, if cchOut is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetTimeFormatA(LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                          LPCSTR lpFormat, LPSTR lpTimeStr, INT cchOut)
{
  TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",lcid, dwFlags, lpTime,
        debugstr_a(lpFormat), lpTimeStr, cchOut);

  return NLS_GetDateTimeFormatA(lcid, dwFlags|TIME_TIMEVARSONLY, lpTime,
                                lpFormat, lpTimeStr, cchOut);
}

/******************************************************************************
 * GetTimeFormatEx [KERNEL32.@]
 *
 * Format a date for a given locale.
 *
 * PARAMS
 *  localename [I] Locale to format for
 *  flags      [I] LOCALE_ and TIME_ flags from "winnls.h"
 *  time       [I] Time to format
 *  format     [I] Formatting overrides
 *  outbuf     [O] Destination for formatted string
 *  bufsize    [I] Size of outbuf, or 0 to calculate the resulting size
 *
 * See GetTimeFormatA for notes.
 *
 * RETURNS
 *  Success: The number of characters written to outbuf, or that would have
 *           have been written if bufsize is 0.
 *  Failure: 0. Use GetLastError() to determine the cause.
 */
INT WINAPI GetTimeFormatEx(LPCWSTR localename, DWORD flags,
                           const SYSTEMTIME* time, LPCWSTR format,
                           LPWSTR outbuf, INT bufsize)
{
  TRACE("(%s,0x%08lx,%p,%s,%p,%d)\n", debugstr_w(localename), flags, time,
        debugstr_w(format), outbuf, bufsize);

  return NLS_GetDateTimeFormatW(LocaleNameToLCID(localename, 0),
                                flags | TIME_TIMEVARSONLY, time, format,
                                outbuf, bufsize);
}

/******************************************************************************
 *		GetTimeFormatW	[KERNEL32.@]
 *
 * See GetTimeFormatA.
 */
INT WINAPI GetTimeFormatW(LCID lcid, DWORD dwFlags, const SYSTEMTIME* lpTime,
                          LPCWSTR lpFormat, LPWSTR lpTimeStr, INT cchOut)
{
  TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",lcid, dwFlags, lpTime,
        debugstr_w(lpFormat), lpTimeStr, cchOut);

  return NLS_GetDateTimeFormatW(lcid, dwFlags|TIME_TIMEVARSONLY, lpTime,
                                lpFormat, lpTimeStr, cchOut);
}
