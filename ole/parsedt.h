/*
PostgreSQL Data Base Management System (formerly known as Postgres, then
as Postgres95).

Copyright (c) 1994-7 Regents of the University of California

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose, without fee, and without a written agreement
is hereby granted, provided that the above copyright notice and this
paragraph and the following two paragraphs appear in all copies.

IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING
LOST PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS
DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATIONS TO
PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*/
/*-------------------------------------------------------------------------
 *
 * dt.h--
 *	  Definitions for the date/time and other date/time support code.
 *	  The support code is shared with other date data types,
 *	   including abstime, reltime, date, and time.
 *
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 * $Id$
 *
 *-------------------------------------------------------------------------
 */
#ifndef DT_H
#define DT_H

#include <time.h>
#include <math.h>

/* We have to include stdlib.h here because it defines many of these macros
   on some platforms, and we only want our definitions used if stdlib.h doesn't
   have its own.
*/

#include <stdlib.h>

/* ----------------------------------------------------------------
 *				Section 1:	bool, true, false, TRUE, FALSE
 * ----------------------------------------------------------------
 */
/*
 * bool --
 *		Boolean value, either true or false.
 *
 */
#define false	((char) 0)
#define true	((char) 1)
#ifndef __cplusplus
#ifndef bool
typedef char bool;
#endif   /* ndef bool */
#endif	 /* not C++ */
typedef bool *BoolPtr;

#ifndef TRUE
#define TRUE	1
#endif	 /* TRUE */

#ifndef FALSE
#define FALSE	0
#endif	 /* FALSE */



/* ----------------------------------------------------------------
 *				Section 3:	standard system types
 * ----------------------------------------------------------------
 */

/*
 * intN --
 *		Signed integer, EXACTLY N BITS IN SIZE,
 *		used for numerical computations and the
 *		frontend/backend protocol.
 */
typedef signed char int8;		/* == 8 bits */
typedef signed short int16;		/* == 16 bits */
typedef signed int int32;		/* == 32 bits */

/*
 * uintN --
 *		Unsigned integer, EXACTLY N BITS IN SIZE,
 *		used for numerical computations and the
 *		frontend/backend protocol.
 */
typedef unsigned char uint8;	/* == 8 bits */
typedef unsigned short uint16;	/* == 16 bits */
typedef unsigned int uint32;	/* == 32 bits */

/*
 * floatN --
 *		Floating point number, AT LEAST N BITS IN SIZE,
 *		used for numerical computations.
 *
 *		Since sizeof(floatN) may be > sizeof(char *), always pass
 *		floatN by reference.
 */
typedef float float32data;
typedef double float64data;
typedef float *float32;
typedef double *float64;

/*
 * boolN --
 *		Boolean value, AT LEAST N BITS IN SIZE.
 */
typedef uint8 bool8;			/* >= 8 bits */
typedef uint16 bool16;			/* >= 16 bits */
typedef uint32 bool32;			/* >= 32 bits */


/* Date/Time Configuration
 *
 * Constants to pass info from runtime environment:
 *	USE_POSTGRES_DATES specifies traditional postgres format for output.
 *	USE_ISO_DATES specifies ISO-compliant format for output.
 *	USE_SQL_DATES specified Oracle/Ingres-compliant format for output.
 *	USE_GERMAN_DATES specifies German-style dd.mm/yyyy date format.
 *
 * DateStyle specifies preference for date formatting for output.
 * EuroDates if client prefers dates interpreted and written w/European conventions.
 *
 * HasCTZSet if client timezone is specified by client.
 * CDayLight is the apparent daylight savings time status.
 * CTimeZone is the timezone offset in seconds.
 * CTZName is the timezone label.
 */

#define USE_POSTGRES_DATES		0
#define USE_ISO_DATES			1
#define USE_SQL_DATES			2
#define USE_GERMAN_DATES		3

int	DateStyle;
bool EuroDates;
int	CTimeZone;

typedef double float8;

struct varlena
{
	int		vl_len;
	char		vl_dat[1];
};

typedef struct varlena text;



typedef int AbsoluteTime;
typedef int RelativeTime;

/*
 * Note a leap year is one that is a multiple of 4
 * but not of a 100.  Except if it is a multiple of
 * 400 then it is a leap year.
 */
#define isleap(y) (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))

/*
 * DateTime represents absolute time.
 * TimeSpan represents delta time. Keep track of months (and years)
 *	separately since the elapsed time spanned is unknown until instantiated
 *	relative to an absolute time.
 *
 * Note that Postgres uses "time interval" to mean a bounded interval,
 *	consisting of a beginning and ending time, not a time span - thomas 97/03/20
 */

typedef double DateTime;

typedef struct
{
	double		time;			/* all time units other than months and
								 * years */
	int		month;			/* months and years, after time for
								 * alignment */
} TimeSpan;


/* ----------------------------------------------------------------
 *				time types + support macros
 *
 * String definitions for standard time quantities.
 *
 * These strings are the defaults used to form output time strings.
 * Other alternate forms are hardcoded into token tables in dt.c.
 * ----------------------------------------------------------------
 */

#define DAGO			"ago"
#define DCURRENT		"current"
#define EPOCH			"epoch"
#define INVALID			"invalid"
#define EARLY			"-infinity"
#define LATE			"infinity"
#define NOW				"now"
#define TODAY			"today"
#define TOMORROW		"tomorrow"
#define YESTERDAY		"yesterday"
#define ZULU			"zulu"

#define DMICROSEC		"usecond"
#define DMILLISEC		"msecond"
#define DSECOND			"second"
#define DMINUTE			"minute"
#define DHOUR			"hour"
#define DDAY			"day"
#define DWEEK			"week"
#define DMONTH			"month"
#define DQUARTER		"quarter"
#define DYEAR			"year"
#define DDECADE			"decade"
#define DCENTURY		"century"
#define DMILLENIUM		"millenium"
#define DA_D			"ad"
#define DB_C			"bc"
#define DTIMEZONE		"timezone"

/*
 * Fundamental time field definitions for parsing.
 *
 *	Meridian:  am, pm, or 24-hour style.
 *	Millenium: ad, bc
 */

#define AM		0
#define PM		1
#define HR24	2

#define AD		0
#define BC		1

/*
 * Fields for time decoding.
 * Can't have more of these than there are bits in an unsigned int
 *	since these are turned into bit masks during parsing and decoding.
 */

#define RESERV	0
#define MONTH	1
#define YEAR	2
#define DAY		3
#define TIMES	4				/* not used - thomas 1997-07-14 */
#define TZ		5
#define DTZ		6
#define DTZMOD	7
#define IGNOREFIELD	8
#define AMPM	9
#define HOUR	10
#define MINUTE	11
#define SECOND	12
#define DOY		13
#define DOW		14
#define UNITS	15
#define ADBC	16
/* these are only for relative dates */
#define AGO		17
#define ABS_BEFORE		18
#define ABS_AFTER		19

/*
 * Token field definitions for time parsing and decoding.
 * These need to fit into the datetkn table type.
 * At the moment, that means keep them within [-127,127].
 * These are also used for bit masks in DecodeDateDelta()
 *	so actually restrict them to within [0,31] for now.
 * - thomas 97/06/19
 * Not all of these fields are used for masks in DecodeDateDelta
 *	so allow some larger than 31. - thomas 1997-11-17
 */

#define DTK_NUMBER		0
#define DTK_STRING		1

#define DTK_DATE		2
#define DTK_TIME		3
#define DTK_TZ			4
#define DTK_AGO			5

#define DTK_SPECIAL		6
#define DTK_INVALID		7
#define DTK_CURRENT		8
#define DTK_EARLY		9
#define DTK_LATE		10
#define DTK_EPOCH		11
#define DTK_NOW			12
#define DTK_YESTERDAY	13
#define DTK_TODAY		14
#define DTK_TOMORROW	15
#define DTK_ZULU		16

#define DTK_DELTA		17
#define DTK_SECOND		18
#define DTK_MINUTE		19
#define DTK_HOUR		20
#define DTK_DAY			21
#define DTK_WEEK		22
#define DTK_MONTH		23
#define DTK_QUARTER		24
#define DTK_YEAR		25
#define DTK_DECADE		26
#define DTK_CENTURY		27
#define DTK_MILLENIUM	28
#define DTK_MILLISEC	29
#define DTK_MICROSEC	30

#define DTK_DOW			32
#define DTK_DOY			33
#define DTK_TZ_HOUR		34
#define DTK_TZ_MINUTE	35

/*
 * Bit mask definitions for time parsing.
 */

#define DTK_M(t)		(0x01 << (t))

#define DTK_DATE_M		(DTK_M(YEAR) | DTK_M(MONTH) | DTK_M(DAY))
#define DTK_TIME_M		(DTK_M(HOUR) | DTK_M(MINUTE) | DTK_M(SECOND))

#define MAXDATELEN		47		/* maximum possible length of an input
								 * date string */
#define MAXDATEFIELDS	25		/* maximum possible number of fields in a
								 * date string */
#define TOKMAXLEN		10		/* only this many chars are stored in
								 * datetktbl */

/* keep this struct small; it gets used a lot */
typedef struct
{
#if defined(_AIX)
	char	   *token;
#else
	char		token[TOKMAXLEN];
#endif	 /* _AIX */
	char		type;
	char		value;			/* this may be unsigned, alas */
} datetkn;



/*
 * dt.c prototypes
 */


void j2date(int jd, int *year, int *month, int *day);
int	date2j(int year, int month, int day);

int ParseDateTime(char *timestr, char *lowstr,
			  char **field, int *ftype, int maxfields, int *numfields);
int DecodeDateTime(char **field, int *ftype,
			 int nf, int *dtype, struct tm * tm, double *fsec, int *tzp);

int DecodeTimeOnly(char **field, int *ftype, int nf,
			   int *dtype, struct tm * tm, double *fsec);


#endif	 /* DT_H */
