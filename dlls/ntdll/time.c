/*
 * Conversion between Time and TimeFields
 *
 * RtlTimeToTimeFields, RtlTimeFieldsToTime and defines are taken from ReactOS and
 * adapted to wine with special permissions of the author
 * Rex Jolliff (rex@lvcablemodem.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <string.h>
#include <time.h>
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include "winternl.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ntdll);

#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       0
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12

/* 1601 to 1970 is 369 years plus 89 leap days */
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)86400)
/* 1601 to 1980 is 379 years plus 91 leap days */
#define SECS_1601_to_1980  ((379 * 365 + 91) * (ULONGLONG)86400)


static const int YearLengths[2] = {DAYSPERNORMALYEAR, DAYSPERLEAPYEAR};
static const int MonthLengths[2][MONSPERYEAR] =
{
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static inline int IsLeapYear(int Year)
{
	return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

static inline void NormalizeTimeFields(CSHORT *FieldToNormalize, CSHORT *CarryField,int Modulus)
{
	*FieldToNormalize = (CSHORT) (*FieldToNormalize - Modulus);
	*CarryField = (CSHORT) (*CarryField + 1);
}

/******************************************************************************
 *  RtlTimeToTimeFields		[NTDLL.@]
 *
 */

VOID WINAPI RtlTimeToTimeFields(
	PLARGE_INTEGER liTime,
	PTIME_FIELDS TimeFields)
{
	const int *Months;
	int SecondsInDay, CurYear;
	int LeapYear, CurMonth;
	long int Days;
	LONGLONG Time;

	Time = liTime->s.HighPart;
	Time <<= 32;
	Time += liTime->s.LowPart;

	/* Extract millisecond from time and convert time into seconds */
	TimeFields->Milliseconds = (CSHORT) ((Time % TICKSPERSEC) / TICKSPERMSEC);
	Time = Time / TICKSPERSEC;

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

	/* compute year */
	CurYear = EPOCHYEAR;
	/* FIXME: handle calendar modifications */
	while (1)
	{ LeapYear = IsLeapYear(CurYear);
	  if (Days < (long) YearLengths[LeapYear])
	  { break;
	  }
	  CurYear++;
	  Days = Days - (long) YearLengths[LeapYear];
	}
	TimeFields->Year = (CSHORT) CurYear;

	/* Compute month of year */
	Months = MonthLengths[LeapYear];
	for (CurMonth = 0; Days >= (long) Months[CurMonth]; CurMonth++)
	  Days = Days - (long) Months[CurMonth];
	TimeFields->Month = (CSHORT) (CurMonth + 1);
	TimeFields->Day = (CSHORT) (Days + 1);
}

/******************************************************************************
 *  RtlTimeFieldsToTime		[NTDLL.@]
 *
 */
BOOLEAN WINAPI RtlTimeFieldsToTime(
	PTIME_FIELDS tfTimeFields,
	PLARGE_INTEGER Time)
{
	int CurYear, CurMonth;
	LONGLONG rcTime;
	TIME_FIELDS TimeFields = *tfTimeFields;

	rcTime = 0;

	/* FIXME: normalize the TIME_FIELDS structure here */
	while (TimeFields.Second >= SECSPERMIN)
	{ NormalizeTimeFields(&TimeFields.Second, &TimeFields.Minute, SECSPERMIN);
	}
	while (TimeFields.Minute >= MINSPERHOUR)
	{ NormalizeTimeFields(&TimeFields.Minute, &TimeFields.Hour, MINSPERHOUR);
	}
	while (TimeFields.Hour >= HOURSPERDAY)
	{ NormalizeTimeFields(&TimeFields.Hour, &TimeFields.Day, HOURSPERDAY);
	}
	while (TimeFields.Day > MonthLengths[IsLeapYear(TimeFields.Year)][TimeFields.Month - 1])
	{ NormalizeTimeFields(&TimeFields.Day, &TimeFields.Month, SECSPERMIN);
	}
	while (TimeFields.Month > MONSPERYEAR)
	{ NormalizeTimeFields(&TimeFields.Month, &TimeFields.Year, MONSPERYEAR);
	}

	/* FIXME: handle calendar corrections here */
	for (CurYear = EPOCHYEAR; CurYear < TimeFields.Year; CurYear++)
	{ rcTime += YearLengths[IsLeapYear(CurYear)];
	}
	for (CurMonth = 1; CurMonth < TimeFields.Month; CurMonth++)
	{ rcTime += MonthLengths[IsLeapYear(CurYear)][CurMonth - 1];
	}
	rcTime += TimeFields.Day - 1;
	rcTime *= SECSPERDAY;
	rcTime += TimeFields.Hour * SECSPERHOUR + TimeFields.Minute * SECSPERMIN + TimeFields.Second;
	rcTime *= TICKSPERSEC;
	rcTime += TimeFields.Milliseconds * TICKSPERMSEC;
	*Time = *(LARGE_INTEGER *)&rcTime;

	return TRUE;
}
/************* end of code by Rex Jolliff (rex@lvcablemodem.com) *******************/

/******************************************************************************
 *  RtlSystemTimeToLocalTime 	[NTDLL.@]
 */
VOID WINAPI RtlSystemTimeToLocalTime(
	IN  PLARGE_INTEGER SystemTime,
	OUT PLARGE_INTEGER LocalTime)
{
	FIXME("(%p, %p),stub!\n",SystemTime,LocalTime);

	memcpy (LocalTime, SystemTime, sizeof (PLARGE_INTEGER));
}

/******************************************************************************
 *  RtlTimeToSecondsSince1970		[NTDLL.@]
 */
BOOLEAN WINAPI RtlTimeToSecondsSince1970( const LARGE_INTEGER *time, PULONG res )
{
    ULONGLONG tmp = ((ULONGLONG)time->s.HighPart << 32) | time->s.LowPart;
    tmp = RtlLargeIntegerDivide( tmp, 10000000, NULL );
    tmp -= SECS_1601_TO_1970;
    if (tmp > 0xffffffff) return FALSE;
    *res = (DWORD)tmp;
    return TRUE;
}

/******************************************************************************
 *  RtlTimeToSecondsSince1980		[NTDLL.@]
 */
BOOLEAN WINAPI RtlTimeToSecondsSince1980( const LARGE_INTEGER *time, LPDWORD res )
{
    ULONGLONG tmp = ((ULONGLONG)time->s.HighPart << 32) | time->s.LowPart;
    tmp = RtlLargeIntegerDivide( tmp, 10000000, NULL );
    tmp -= SECS_1601_to_1980;
    if (tmp > 0xffffffff) return FALSE;
    *res = (DWORD)tmp;
    return TRUE;
}

/******************************************************************************
 *  RtlSecondsSince1970ToTime		[NTDLL.@]
 */
void WINAPI RtlSecondsSince1970ToTime( DWORD time, LARGE_INTEGER *res )
{
    ULONGLONG secs = RtlExtendedIntegerMultiply( time + SECS_1601_TO_1970, 10000000 );
    res->s.LowPart  = (DWORD)secs;
    res->s.HighPart = (DWORD)(secs >> 32);
}

/******************************************************************************
 *  RtlSecondsSince1980ToTime		[NTDLL.@]
 */
void WINAPI RtlSecondsSince1980ToTime( DWORD time, LARGE_INTEGER *res )
{
    ULONGLONG secs = RtlExtendedIntegerMultiply( time + SECS_1601_to_1980, 10000000 );
    res->s.LowPart  = (DWORD)secs;
    res->s.HighPart = (DWORD)(secs >> 32);
}

/******************************************************************************
 * RtlTimeToElapsedTimeFields [NTDLL.@]
 * FIXME: prototype guessed
 */
VOID WINAPI RtlTimeToElapsedTimeFields(
	PLARGE_INTEGER liTime,
	PTIME_FIELDS TimeFields)
{
	FIXME("(%p,%p): stub\n",liTime,TimeFields);
}

/***********************************************************************
 *      NtQuerySystemTime   (NTDLL.@)
 *      ZwQuerySystemTime   (NTDLL.@)
 */
NTSTATUS WINAPI NtQuerySystemTime( PLARGE_INTEGER time )
{
    ULONGLONG secs;
    struct timeval now;

    gettimeofday( &now, 0 );
    secs = RtlExtendedIntegerMultiply( now.tv_sec+SECS_1601_TO_1970, 10000000 ) + now.tv_usec * 10;
    time->s.LowPart  = (DWORD)secs;
    time->s.HighPart = (DWORD)(secs >> 32);
    return STATUS_SUCCESS;
}
