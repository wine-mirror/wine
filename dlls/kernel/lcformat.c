/*
 * Locale-dependent format handling
 *
 * Copyright 1995 Martin von Loewis
 * Copyright 1998 David Lee Lambert
 * Copyright 2000 Julio César Gázquez
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "windef.h"
#include "winbase.h"
#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(nls);


/******************************************************************************
 *		get_date_time_formatW   [Internal]
 *
 * dateformat is set TRUE if being called for a date, false for a time
 *
 * This function implements stuff for GetDateFormat() and
 * GetTimeFormat().
 *
 *  d    single-digit (no leading zero) day (of month)
 *  dd   two-digit day (of month)
 *  ddd  short day-of-week name
 *  dddd long day-of-week name
 *  M    single-digit month
 *  MM   two-digit month
 *  MMM  short month name
 *  MMMM full month name
 *  y    two-digit year, no leading 0
 *  yy   two-digit year
 *  yyyy four-digit year
 *  gg   era string
 *  h    hours with no leading zero (12-hour)
 *  hh   hours with full two digits
 *  H    hours with no leading zero (24-hour)
 *  HH   hours with full two digits
 *  m    minutes with no leading zero
 *  mm   minutes with full two digits
 *  s    seconds with no leading zero
 *  ss   seconds with full two digits
 *  t    time marker (A or P)
 *  tt   time marker (AM, PM)
 *  ''   used to quote literal characters
 *  ''   (within a quoted string) indicates a literal '
 *
 * If TIME_NOMINUTESORSECONDS or TIME_NOSECONDS is specified, the function
 *  removes the separator(s) preceding the minutes and/or seconds element(s).
 *
 * If TIME_NOTIMEMARKER is specified, the function removes the separator(s)
 *  preceding and following the time marker.
 *
 * If TIME_FORCE24HOURFORMAT is specified, the function displays any existing
 *  time marker, unless the TIME_NOTIMEMARKER flag is also set.
 *
 * These functions REQUIRE valid locale, date,  and format.
 *
 * If the time or date is invalid, return 0 and set ERROR_INVALID_PARAMETER
 *
 * Return value is the number of characters written, or if outlen is zero
 *	it is the number of characters required for the output including
 *	the terminating null.
 */
static INT get_date_time_formatW(LCID locale, DWORD flags, DWORD tflags,
                                 const SYSTEMTIME* xtime, LPCWSTR format,
                                 LPWSTR output, INT outlen, int dateformat)
{
   INT     outpos;
   INT	   lastFormatPos; /* the position in the output buffer of */
			  /* the end of the output from the last formatting */
			  /* character */
   BOOL	   dropUntilNextFormattingChar = FALSE; /* TIME_NOTIMEMARKER drops
				all of the text around the dropped marker,
				eg. "h@!t@!m" becomes "hm" */

   /* make a debug report */
   TRACE("args: 0x%lx, 0x%lx, 0x%lx, time(d=%d,h=%d,m=%d,s=%d), fmt:%s (at %p), "
   	      "%p with max len %d\n",
	 locale, flags, tflags,
	 xtime->wDay, xtime->wHour, xtime->wMinute, xtime->wSecond,
	 debugstr_w(format), format, output, outlen);

   /* initialize state variables */
   outpos = 0;
   lastFormatPos = 0;

   while (*format) {
      /* Literal string: Maybe terminated early by a \0 */
      if (*format == (WCHAR) '\'')
      {
         format++;

	 /* We loop while we haven't reached the end of the format string */
	 /* and until we haven't found another "'" character */
	 while (*format)
         {
	    /* we found what might be the close single quote mark */
	    /* we need to make sure there isn't another single quote */
	    /* after it, if there is we need to skip over this quote mark */
	    /* as the user is trying to put a single quote mark in their output */
            if (*format == (WCHAR) '\'')
            {
               format++;
               if (*format != '\'')
               {
                  break; /* It was a terminating quote */
               }
            }

	    /* if outlen is zero then we are couting the number of */
	    /* characters of space we need to output this text, don't */
	    /* modify the output buffer */
            if (!outlen)
            {
               outpos++;   /* We are counting */;
            }  else if (outpos >= outlen)
            {
               goto too_short;
            } else
            {
               /* even drop literal strings */
	       if(!dropUntilNextFormattingChar)
               {
                 output[outpos] = *format;
                 outpos++;
               }
            }
            format++;
         }
      } else if ( (dateformat &&  (*format=='d' ||
				   *format=='M' ||
				   *format=='y' ||
				   *format=='g')  ) ||
		  (!dateformat && (*format=='H' ||
				   *format=='h' ||
				   *format=='m' ||
				   *format=='s' ||
				   *format=='t') )    )
     /* if processing a date and we have a date formatting character, OR */
     /* if we are processing a time and we have a time formatting character */
     {
         int type, count;
         char    tmp[16];
         WCHAR   buf[40];
         int     buflen=0;
         type = *format;
         format++;

	 /* clear out the drop text flag if we are in here */
	 dropUntilNextFormattingChar = FALSE;

         /* count up the number of the same letter values in a row that */
	 /* we get, this lets us distinguish between "s" and "ss" and it */
	 /* eliminates the duplicate to simplify the below case statement */
         for (count = 1; *format == type; format++)
            count++;

	 buf[0] = 0; /* always null terminate the buffer */

         switch(type)
         {
          case 'd':
	    if        (count >= 4) {
	       GetLocaleInfoW(locale,
			     LOCALE_SDAYNAME1 + (xtime->wDayOfWeek +6)%7,
			     buf, sizeof(buf)/sizeof(WCHAR) );
	    } else if (count == 3) {
	       GetLocaleInfoW(locale,
				LOCALE_SABBREVDAYNAME1 +
				(xtime->wDayOfWeek +6)%7,
				buf, sizeof(buf)/sizeof(WCHAR) );
	    } else {
                sprintf( tmp, "%.*d", count, xtime->wDay );
                MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
	    }
            break;

          case 'M':
	    if        (count >= 4) {
	       GetLocaleInfoW(locale,  LOCALE_SMONTHNAME1 +
				xtime->wMonth -1, buf,
				sizeof(buf)/sizeof(WCHAR) );
	    } else if (count == 3) {
	       GetLocaleInfoW(locale,  LOCALE_SABBREVMONTHNAME1 +
				xtime->wMonth -1, buf,
				sizeof(buf)/sizeof(WCHAR) );
	    } else {
                sprintf( tmp, "%.*d", count, xtime->wMonth );
                MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
	    }
            break;
          case 'y':
	    if        (count >= 4) {
                sprintf( tmp, "%d", xtime->wYear );
	    } else {
                sprintf( tmp, "%.*d", count > 2 ? 2 : count, xtime->wYear % 100 );
	    }
            MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
            break;

          case 'g':
	    if        (count == 2) {
	       FIXME("LOCALE_ICALENDARTYPE unimplemented\n");
               strcpy( tmp, "AD" );
	    } else {
	       /* Win API sez we copy it verbatim */
                strcpy( tmp, "g" );
	    }
            MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
            break;

          case 'h':
	      /* fallthrough if we are forced to output in 24 hour format */
	      if(!(tflags & TIME_FORCE24HOURFORMAT))
              {
                /* hours 1:00-12:00 --- is this right? */
		/* NOTE: 0000 hours is also 12 midnight */
                sprintf( tmp, "%.*d", count > 2 ? 2 : count,
			xtime->wHour == 0 ? 12 : (xtime->wHour-1)%12 +1);
                MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
                break;
              }
          case 'H':
              sprintf( tmp, "%.*d", count > 2 ? 2 : count, xtime->wHour );
              MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
              break;

          case 'm':
	      /* if TIME_NOMINUTESORSECONDS don't display minutes */
	      if(!(tflags & TIME_NOMINUTESORSECONDS))
              {
                sprintf( tmp, "%.*d", count > 2 ? 2 : count, xtime->wMinute );
                MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
	      } else
	      {
		outpos = lastFormatPos;
	      }
              break;

          case 's':
	      /* if we have a TIME_NOSECONDS or TIME_NOMINUTESORSECONDS
			flag then don't display seconds */
	      if(!(tflags & TIME_NOSECONDS) && !(tflags &
			TIME_NOMINUTESORSECONDS))
              {
                sprintf( tmp, "%.*d", count > 2 ? 2 : count, xtime->wSecond );
                MultiByteToWideChar( CP_ACP, 0, tmp, -1, buf, sizeof(buf)/sizeof(WCHAR) );
	      } else
	      {
		outpos = lastFormatPos;
	      }
              break;

          case 't':
	    if(!(tflags & TIME_NOTIMEMARKER))
	    {
  	      GetLocaleInfoW(locale, (xtime->wHour < 12) ?
			     LOCALE_S1159 : LOCALE_S2359,
			     buf, sizeof(buf) );
  	      if (count == 1)
	      {
	         buf[1] = 0;
	      }
            } else
	    {
		outpos = lastFormatPos; /* remove any prior text up until
			the output due to formatting characters */
 		dropUntilNextFormattingChar = TRUE; /* drop everything
			until we hit the next formatting character */
	    }
            break;
         }

	 /* cat buf onto the output */
	 buflen = strlenW(buf);

	 /* we are counting how many characters we need for output */
	 /* don't modify the output buffer... */
         if (!outlen)
            /* We are counting */;
         else if (outpos + buflen < outlen) {
            strcpyW( output + outpos, buf );
	 } else {
            lstrcpynW( output + outpos, buf, outlen - outpos );
            /* Is this an undocumented feature we are supporting? */
            goto too_short;
	 }
	 outpos += buflen;
	 lastFormatPos = outpos; /* record the end of the formatting text we just output */
      } else /* we are processing a NON formatting character */
      {
         /* a literal character */
         if (!outlen)
         {
            outpos++;   /* We are counting */;
         }
         else if (outpos >= outlen)
         {
            goto too_short;
         }
         else /* just copy the character into the output buffer */
         {
	    /* unless we are dropping characters */
	    if(!dropUntilNextFormattingChar)
	    {
	       output[outpos] = *format;
               outpos++;
            }
         }
         format++;
      }
   }

   /* final string terminator and sanity check */
   if (!outlen)
      /* We are counting */;
   else if (outpos >= outlen)
      goto too_short;
   else
      output[outpos] = '\0';

   outpos++; /* add one for the terminating null character */

   TRACE(" returning %d %s\n", outpos, debugstr_w(output));
   return outpos;

too_short:
   SetLastError(ERROR_INSUFFICIENT_BUFFER);
   WARN(" buffer overflow\n");
   return 0;
}


/******************************************************************************
 *		GetDateFormatA	[KERNEL32.@]
 * Makes an ASCII string of the date
 *
 * Acts the same as GetDateFormatW(),  except that it's ASCII.
 */
INT WINAPI GetDateFormatA( LCID locale, DWORD flags, const SYSTEMTIME* xtime,
                           LPCSTR format, LPSTR date, INT datelen )
{
  INT ret;
  LPWSTR wformat = NULL;
  LPWSTR wdate = NULL;

  if (format)
  {
    wformat = HeapAlloc(GetProcessHeap(), 0,
                        (strlen(format) + 1) * sizeof(wchar_t));

    MultiByteToWideChar(CP_ACP, 0, format, -1, wformat, strlen(format) + 1);
  }

  if (date && datelen)
  {
    wdate = HeapAlloc(GetProcessHeap(), 0,
                      (datelen + 1) * sizeof(wchar_t));
  }

  ret = GetDateFormatW(locale, flags, xtime, wformat, wdate, datelen);

  if (wdate)
  {
    WideCharToMultiByte(CP_ACP, 0, wdate, ret, date, datelen, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wdate);
  }

  if (wformat)
  {
    HeapFree(GetProcessHeap(), 0, wformat);
  }

  return ret;
}


/******************************************************************************
 *		GetDateFormatW	[KERNEL32.@]
 * Makes a Unicode string of the date
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
 * Accepts & returns sizes as counts of Unicode characters.
 */
INT WINAPI GetDateFormatW( LCID locale, DWORD flags, const SYSTEMTIME* xtime,
                           LPCWSTR format, LPWSTR date, INT datelen)
{
    WCHAR format_buf[40];
    LPCWSTR thisformat;
    SYSTEMTIME t;
    LCID thislocale;
    INT ret;
    FILETIME ft;
    BOOL res;

    TRACE("(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",
	  locale,flags,xtime,debugstr_w(format),date,datelen);

    /* Tests */
    if (flags && format) /* if lpFormat is non-null then flags must be zero */
    {
        SetLastError (ERROR_INVALID_FLAGS);
	return 0;
    }
    if (datelen && !date)
    {
        SetLastError (ERROR_INVALID_PARAMETER);
	return 0;
    }
    if (!locale)
    {
	locale = LOCALE_SYSTEM_DEFAULT;
    }

    if (locale == LOCALE_SYSTEM_DEFAULT)
    {
	thislocale = GetSystemDefaultLCID();
    } else if (locale == LOCALE_USER_DEFAULT)
    {
	thislocale = GetUserDefaultLCID();
    } else
    {
	thislocale = locale;
    }

    /* check for invalid flag combinations */
    if((flags & DATE_LTRREADING) && (flags & DATE_RTLREADING))
    {
        SetLastError (ERROR_INVALID_FLAGS);
	return 0;
    }

    /* DATE_SHORTDATE, DATE_LONGDATE and DATE_YEARMONTH are mutually */
    /* exclusive */
    if((flags & (DATE_SHORTDATE|DATE_LONGDATE|DATE_YEARMONTH))
	 && !((flags & DATE_SHORTDATE) ^ (flags &
        DATE_LONGDATE) ^ (flags & DATE_YEARMONTH)))
    {
        SetLastError (ERROR_INVALID_FLAGS);
	return 0;
    }

    /* if the user didn't pass in a pointer to the current time we read it */
    /* here */
    if (xtime == NULL)
    {
	GetSystemTime(&t);
    } else
    {
        /* NOTE: check here before we perform the SystemTimeToFileTime conversion */
        /*  because this conversion will fix invalid time values */
        /* check to see if the time/date is valid */
        /* set ERROR_INVALID_PARAMETER and return 0 if invalid */
        if((xtime->wDay > 31) || (xtime->wMonth > 12))
        {
          SetLastError(ERROR_INVALID_PARAMETER);
          return 0;
        }
        /* For all we know the day of week and the time may be absolute
         * rubbish.  Therefore if we are going to use conversion through
         * FileTime we had better use a clean time (and hopefully we won't
         * fall over any timezone complications).
         * If we go with an alternative method of correcting the day of week
         * (e.g. Zeller's congruence) then we won't need to, but we will need
         * to check the date.
         */
        memset (&t, 0, sizeof(t));
        t.wYear = xtime->wYear;
        t.wMonth = xtime->wMonth;
        t.wDay = xtime->wDay;

	/* Silently correct wDayOfWeek by transforming to FileTime and back again */
	res=SystemTimeToFileTime(&t,&ft);

	/* Check year(?)/month and date for range and set ERROR_INVALID_PARAMETER  on error */
	if(!res)
	{
	    SetLastError(ERROR_INVALID_PARAMETER);
	    return 0;
	}
	FileTimeToSystemTime(&ft,&t);
    }

    if (format == NULL)
    {
	GetLocaleInfoW(thislocale, ((flags&DATE_LONGDATE)
				    ? LOCALE_SLONGDATE
				    : LOCALE_SSHORTDATE),
		       format_buf, sizeof(format_buf)/sizeof(*format_buf));
	thisformat = format_buf;
    } else
    {
	thisformat = format;
    }

    ret = get_date_time_formatW(thislocale, flags, 0, &t, thisformat, date, datelen, 1);

    TRACE("GetDateFormatW() returning %d, with data=%s\n",
	  ret, debugstr_w(date));
    return ret;
}


/**************************************************************************
 *              EnumDateFormatsA	(KERNEL32.@)
 */
BOOL WINAPI EnumDateFormatsA( DATEFMT_ENUMPROCA lpDateFmtEnumProc, LCID Locale,  DWORD dwFlags)
{
  LCID Loc = GetUserDefaultLCID();
  if(!lpDateFmtEnumProc)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  switch( Loc )
 {

   case 0x00000407:  /* (Loc,"de_DE") */
   {
    switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd.MM.yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d.M.yyyy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d.MM.yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("d.M.yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dddd,d. MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d. MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d. MMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   }

   case 0x0000040c:  /* (Loc,"fr_FR") */
   {
    switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd.MM.yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd-MM-yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd/MM/yyyy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("dddd d MMMM yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d MMM yy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d MMMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   }

   case 0x00000c0c:  /* (Loc,"fr_CA") */
   {
    switch(dwFlags)
    {
      case DATE_SHORTDATE:
	if(!(*lpDateFmtEnumProc)("yy-MM-dd")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd-MM-yy")) return TRUE;
	if(!(*lpDateFmtEnumProc)("yy MM dd")) return TRUE;
	if(!(*lpDateFmtEnumProc)("dd/MM/yy")) return TRUE;
	return TRUE;
      case DATE_LONGDATE:
        if(!(*lpDateFmtEnumProc)("d MMMM, yyyy")) return TRUE;
        if(!(*lpDateFmtEnumProc)("d MMM yyyy")) return TRUE;
	return TRUE;
      default:
	FIXME("Unknown date format (%ld)\n", dwFlags);
	SetLastError(ERROR_INVALID_PARAMETER);
	return FALSE;
     }
   }

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
 *              EnumDateFormatsW	(KERNEL32.@)
 */
BOOL WINAPI EnumDateFormatsW( DATEFMT_ENUMPROCW lpDateFmtEnumProc, LCID Locale, DWORD dwFlags )
{
  FIXME("(%p, %ld, %ld): stub\n", lpDateFmtEnumProc, Locale, dwFlags);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/**************************************************************************
 *              EnumTimeFormatsA	(KERNEL32.@)
 */
BOOL WINAPI EnumTimeFormatsA( TIMEFMT_ENUMPROCA lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags )
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
 *              EnumTimeFormatsW	(KERNEL32.@)
 */
BOOL WINAPI EnumTimeFormatsW( TIMEFMT_ENUMPROCW lpTimeFmtEnumProc, LCID Locale, DWORD dwFlags )
{
  FIXME("(%p,%ld,%ld): stub\n", lpTimeFmtEnumProc, Locale, dwFlags);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/**************************************************************************
 *           This function is used just locally !
 *  Description: Inverts a string.
 */
static void invert_string(char* string)
{
    int i, j;

    for (i = 0, j = strlen(string) - 1; i < j; i++, j--)
    {
        char ch = string[i];
        string[i] = string[j];
        string[j] = ch;
    }
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
static INT get_number_components(char* pInput, char* psBefore, char* psAfter, BOOL* pbNegative)
{
    char sNumberSet[] = "0123456789";
    BOOL bInDecimal = FALSE;

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
 *               containing the rule to be used. Its format is the following:
 *               |NDG|DG1|DG2|...|0|
 *               where NDG is the number of used "digits in group" and DG1, DG2,
 *               are the corresponding "digits in group".
 *               Thus, this function returns the grouping value in the array
 *               pointed by the second parameter.
 */
static INT get_grouping(char* sRule, INT index)
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
 *              GetNumberFormatA	(KERNEL32.@)
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
    if (get_number_components(sNumber, sDigitsBeforeDecimal, sDigitsAfterDecimal, &bNegative) != -1)
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
            used_operation = USE_SYSTEMDEFAULT;
        else
            used_operation = USE_LOCALEINFO;
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
        /* Win95-WinME only allow 0-9 for grouping, no matter what MSDN says. */
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
    memcpy(sRule, sBuffer, 1); /* Number of digits in the groups ( used by get_grouping() ) */
    memcpy(sRule + j, "\0", 1);

    /* First, format the digits before the decimal. */
    if ((nNumberDecimal>0) && (atoi(sDigitsBeforeDecimal) != 0))
    {
        /* Working on an inverted string is easier ! */
        invert_string(sDigitsBeforeDecimal);

        nStep = nCounter = i = j = 0;
        nRuleIndex = 1;
        nGrouping = get_grouping(sRule, nRuleIndex);
        if (nGrouping == 0) /* If the first grouping is zero */
            nGrouping = nNumberDecimal; /* Don't do grouping */

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
                nGrouping = get_grouping(sRule, nRuleIndex);
                memcpy(sDestination + i+1, sDigitGroupSymbol, strlen(sDigitGroupSymbol));
                nStep+= strlen(sDigitGroupSymbol);
            }

            nNumberDecimal--;
        }

        memcpy(sDestination + i+1, "\0", 1);
        /* Get the string in the right order ! */
        invert_string(sDestination);
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
    memcpy(sNumber + nCounter+i+ (i ? strlen(sDecimalSymbol) : 0), "\0", 1);

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
    retVal = strlen(sDestination) + 1;
    if (cchNumber!=0)
    {
        memcpy( lpNumberStr, sDestination, min(cchNumber, retVal) );
        if (cchNumber < retVal) {
            retVal = 0;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    return retVal;
}

/**************************************************************************
 *              GetNumberFormatW	(KERNEL32.@)
 */
INT WINAPI GetNumberFormatW(LCID locale, DWORD dwflags,
                            LPCWSTR lpvalue,  const NUMBERFMTW * lpFormat,
                            LPWSTR lpNumberStr, int cchNumber)
{
 FIXME("%s: stub, no reformatting done\n",debugstr_w(lpvalue));

 lstrcpynW( lpNumberStr, lpvalue, cchNumber );
 return cchNumber? strlenW( lpNumberStr ) : 0;
}

/**************************************************************************
 *              GetCurrencyFormatA	(KERNEL32.@)
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
            used_operation = USE_SYSTEMDEFAULT;
        else
            used_operation = USE_LOCALEINFO;
    }

    /* Load the fields we need */
    switch(used_operation)
    {
    case USE_LOCALEINFO:
        /* Specific to CURRENCYFMTA */
        GetLocaleInfoA(locale, LOCALE_INEGCURR, sNegOrder, sizeof(sNegOrder));
        GetLocaleInfoA(locale, LOCALE_ICURRENCY, sPosOrder, sizeof(sPosOrder));
        GetLocaleInfoA(locale, LOCALE_SCURRENCY, sCurrencySymbol, sizeof(sCurrencySymbol));

        nPosOrder = atoi(sPosOrder);
        nNegOrder = atoi(sNegOrder);
        break;
    case USE_PARAMETER:
        /* Specific to CURRENCYFMTA */
        nNegOrder = lpFormat->NegativeOrder;
        nPosOrder = lpFormat->PositiveOrder;
        strcpy(sCurrencySymbol, lpFormat->lpCurrencySymbol);
        break;
    case USE_SYSTEMDEFAULT:
        systemDefaultLCID = GetSystemDefaultLCID();
        /* Specific to CURRENCYFMTA */
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

    retVal = strlen(pDestination) + 1;

    if (cchCurrency)
    {
        memcpy( lpCurrencyStr, pDestination, min(cchCurrency, retVal) );
        if (cchCurrency < retVal) {
            retVal = 0;
            SetLastError(ERROR_INSUFFICIENT_BUFFER);
        }
    }
    return retVal;
}

/**************************************************************************
 *              GetCurrencyFormatW	(KERNEL32.@)
 */
INT WINAPI GetCurrencyFormatW(LCID locale, DWORD dwflags,
                              LPCWSTR lpvalue,   const CURRENCYFMTW * lpFormat,
                              LPWSTR lpCurrencyStr, int cchCurrency)
{
    FIXME("This API function is NOT implemented !\n");
    return 0;
}


/******************************************************************************
 *		GetTimeFormatA	[KERNEL32.@]
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
GetTimeFormatA(LCID locale,        /* [in]  */
	       DWORD flags,        /* [in]  */
	       const SYSTEMTIME* xtime, /* [in]  */
	       LPCSTR format,      /* [in]  */
	       LPSTR timestr,      /* [out] */
	       INT timelen         /* [in]  */)
{
  INT ret;
  LPWSTR wformat = NULL;
  LPWSTR wtime = NULL;

  if (format)
  {
    wformat = HeapAlloc(GetProcessHeap(), 0,
                        (strlen(format) + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, format, -1, wformat, strlen(format) + 1);
  }

  if (timestr && timelen)
  {
    wtime = HeapAlloc(GetProcessHeap(), 0,
                      (timelen + 1) * sizeof(wchar_t));
  }

  ret = GetTimeFormatW(locale, flags, xtime, wformat, wtime, timelen);

  if (wtime)
  {
    WideCharToMultiByte(CP_ACP, 0, wtime, ret, timestr, timelen, NULL, NULL);
    HeapFree(GetProcessHeap(), 0, wtime);
  }

  if (wformat)
  {
    HeapFree(GetProcessHeap(), 0, wformat);
  }

  return ret;
}


/******************************************************************************
 *		GetTimeFormatW	[KERNEL32.@]
 * Makes a Unicode string of the time
 *
 * NOTE: See get_date_time_formatW() for further documentation
 */
INT WINAPI
GetTimeFormatW(LCID locale,        /* [in]  */
	       DWORD flags,        /* [in]  */
	       const SYSTEMTIME* xtime, /* [in]  */
	       LPCWSTR format,     /* [in]  */
	       LPWSTR timestr,     /* [out] */
	       INT timelen         /* [in]  */)
{	WCHAR format_buf[40];
	LPCWSTR thisformat;
	SYSTEMTIME t;
	const SYSTEMTIME* thistime;
	DWORD thisflags=LOCALE_STIMEFORMAT; /* standard timeformat */
	INT ret;

	TRACE("GetTimeFormat(0x%04lx,0x%08lx,%p,%s,%p,%d)\n",locale,flags,
	xtime,debugstr_w(format),timestr,timelen);

        if (!locale) locale = LOCALE_SYSTEM_DEFAULT;
        locale = ConvertDefaultLocale( locale );

	/* if the user didn't specify a format we use the default */
        /* format for this locale */
	if (format == NULL)
	{
	  if (flags & LOCALE_NOUSEROVERRIDE)  /* use system default */
	  {
            locale = GetSystemDefaultLCID();
	  }
	  GetLocaleInfoW(locale, thisflags, format_buf, 40);
	  thisformat = format_buf;
	}
	else
	{
	  /* if non-null format and LOCALE_NOUSEROVERRIDE then fail */
	  /* NOTE: this could be either invalid flags or invalid parameter */
	  /*  windows sets it to invalid flags */
	  if (flags & LOCALE_NOUSEROVERRIDE)
          {
	    SetLastError(ERROR_INVALID_FLAGS);
	    return 0;
          }

          thisformat = format;
	}

	if (xtime == NULL) /* NULL means use the current local time */
	{ GetLocalTime(&t);
	  thistime = &t;
	}
	else
	{
          /* check time values */
          if((xtime->wHour > 24) || (xtime->wMinute >= 60) || (xtime->wSecond >= 60))
          {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
          }

          thistime = xtime;
	}

	ret = get_date_time_formatW(locale, thisflags, flags, thistime, thisformat,
                                    timestr, timelen, 0);
	return ret;
}

/******************************************************************************
 *		EnumCalendarInfoA	[KERNEL32.@]
 */
BOOL WINAPI EnumCalendarInfoA( CALINFO_ENUMPROCA calinfoproc,LCID locale,
                               CALID calendar,CALTYPE caltype )
{
    FIXME("(%p,0x%04lx,0x%08lx,0x%08lx),stub!\n",calinfoproc,locale,calendar,caltype);
    return FALSE;
}
