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
 * dt.c--
 *	  Functions for the built-in type "dt".
 *
 * Copyright (c) 1994, Regents of the University of California
 *
 *
 *-------------------------------------------------------------------------
 */
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <limits.h>
#include <sys/timeb.h>

#include "parsedt.h"

static datetkn *datebsearch(char *key, datetkn *base, unsigned int nel);
static int	DecodeDate(char *str, int fmask, int *tmask, struct tm * tm);
static int DecodeNumber(int flen, char *field,
			 int fmask, int *tmask, struct tm * tm, double *fsec);
static int DecodeNumberField(int len, char *str,
				  int fmask, int *tmask, struct tm * tm, double *fsec);
static int	DecodeSpecial(int field, char *lowtoken, int *val);
static int DecodeTime(char *str, int fmask, int *tmask,
		   struct tm * tm, double *fsec);
static int	DecodeTimezone(char *str, int *tzp);

#define USE_DATE_CACHE 1
#define ROUND_ALL 0

static const int mdays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31, 0};

static const char * const months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
"Jul", "Aug", "Sep", "Oct", "Nov", "Dec", NULL};

static const char * const days[] = {"Sunday", "Monday", "Tuesday", "Wednesday",
"Thursday", "Friday", "Saturday", NULL};

/* those three vars are useless, and not even initialized, so 
 * I'd rather remove them all (EPP)
 */
int	DateStyle; 
bool	EuroDates;
int	CTimeZone;

#define UTIME_MINYEAR (1901)
#define UTIME_MINMONTH (12)
#define UTIME_MINDAY (14)
#define UTIME_MAXYEAR (2038)
#define UTIME_MAXMONTH (01)
#define UTIME_MAXDAY (18)

#define IS_VALID_UTIME(y,m,d) (((y > UTIME_MINYEAR) \
 || ((y == UTIME_MINYEAR) && ((m > UTIME_MINMONTH) \
  || ((m == UTIME_MINMONTH) && (d >= UTIME_MINDAY))))) \
 && ((y < UTIME_MAXYEAR) \
 || ((y == UTIME_MAXYEAR) && ((m < UTIME_MAXMONTH) \
  || ((m == UTIME_MAXMONTH) && (d <= UTIME_MAXDAY))))))




/*****************************************************************************
 *	 PRIVATE ROUTINES														 *
 *****************************************************************************/

/* definitions for squeezing values into "value" */
#define ABS_SIGNBIT		(char) 0200
#define VALMASK			(char) 0177
#define NEG(n)			((n)|ABS_SIGNBIT)
#define SIGNEDCHAR(c)	((c)&ABS_SIGNBIT? -((c)&VALMASK): (c))
#define FROMVAL(tp)		(-SIGNEDCHAR((tp)->value) * 10) /* uncompress */
#define TOVAL(tp, v)	((tp)->value = ((v) < 0? NEG((-(v))/10): (v)/10))

/*
 * to keep this table reasonably small, we divide the lexval for TZ and DTZ
 * entries by 10 and truncate the text field at MAXTOKLEN characters.
 * the text field is not guaranteed to be NULL-terminated.
 */
static datetkn datetktbl[] = {
/*		text			token	lexval */
	{EARLY, RESERV, DTK_EARLY}, /* "-infinity" reserved for "early time" */
	{"acsst", DTZ, 63},			/* Cent. Australia */
	{"acst", TZ, 57},			/* Cent. Australia */
	{DA_D, ADBC, AD},			/* "ad" for years >= 0 */
	{"abstime", IGNOREFIELD, 0},		/* "abstime" for pre-v6.1 "Invalid
								 * Abstime" */
	{"adt", DTZ, NEG(18)},		/* Atlantic Daylight Time */
	{"aesst", DTZ, 66},			/* E. Australia */
	{"aest", TZ, 60},			/* Australia Eastern Std Time */
	{"ahst", TZ, 60},			/* Alaska-Hawaii Std Time */
	{"allballs", RESERV, DTK_ZULU},		/* 00:00:00 */
	{"am", AMPM, AM},
	{"apr", MONTH, 4},
	{"april", MONTH, 4},
	{"ast", TZ, NEG(24)},		/* Atlantic Std Time (Canada) */
	{"at", IGNOREFIELD, 0},			/* "at" (throwaway) */
	{"aug", MONTH, 8},
	{"august", MONTH, 8},
	{"awsst", DTZ, 54},			/* W. Australia */
	{"awst", TZ, 48},			/* W. Australia */
	{DB_C, ADBC, BC},			/* "bc" for years < 0 */
	{"bst", TZ, 6},				/* British Summer Time */
	{"bt", TZ, 18},				/* Baghdad Time */
	{"cadt", DTZ, 63},			/* Central Australian DST */
	{"cast", TZ, 57},			/* Central Australian ST */
	{"cat", TZ, NEG(60)},		/* Central Alaska Time */
	{"cct", TZ, 48},			/* China Coast */
	{"cdt", DTZ, NEG(30)},		/* Central Daylight Time */
	{"cet", TZ, 6},				/* Central European Time */
	{"cetdst", DTZ, 12},		/* Central European Dayl.Time */
	{"cst", TZ, NEG(36)},		/* Central Standard Time */
	{DCURRENT, RESERV, DTK_CURRENT},	/* "current" is always now */
	{"dec", MONTH, 12},
	{"december", MONTH, 12},
	{"dnt", TZ, 6},				/* Dansk Normal Tid */
	{"dow", RESERV, DTK_DOW},	/* day of week */
	{"doy", RESERV, DTK_DOY},	/* day of year */
	{"dst", DTZMOD, 6},
	{"east", TZ, NEG(60)},		/* East Australian Std Time */
	{"edt", DTZ, NEG(24)},		/* Eastern Daylight Time */
	{"eet", TZ, 12},			/* East. Europe, USSR Zone 1 */
	{"eetdst", DTZ, 18},		/* Eastern Europe */
	{EPOCH, RESERV, DTK_EPOCH}, /* "epoch" reserved for system epoch time */
#if USE_AUSTRALIAN_RULES
	{"est", TZ, 60},			/* Australia Eastern Std Time */
#else
	{"est", TZ, NEG(30)},		/* Eastern Standard Time */
#endif
	{"feb", MONTH, 2},
	{"february", MONTH, 2},
	{"fri", DOW, 5},
	{"friday", DOW, 5},
	{"fst", TZ, 6},				/* French Summer Time */
	{"fwt", DTZ, 12},			/* French Winter Time  */
	{"gmt", TZ, 0},				/* Greenwish Mean Time */
	{"gst", TZ, 60},			/* Guam Std Time, USSR Zone 9 */
	{"hdt", DTZ, NEG(54)},		/* Hawaii/Alaska */
	{"hmt", DTZ, 18},			/* Hellas ? ? */
	{"hst", TZ, NEG(60)},		/* Hawaii Std Time */
	{"idle", TZ, 72},			/* Intl. Date Line, East */
	{"idlw", TZ, NEG(72)},		/* Intl. Date Line,,	est */
	{LATE, RESERV, DTK_LATE},	/* "infinity" reserved for "late time" */
	{INVALID, RESERV, DTK_INVALID},		/* "invalid" reserved for invalid
										 * time */
	{"ist", TZ, 12},			/* Israel */
	{"it", TZ, 22},				/* Iran Time */
	{"jan", MONTH, 1},
	{"january", MONTH, 1},
	{"jst", TZ, 54},			/* Japan Std Time,USSR Zone 8 */
	{"jt", TZ, 45},				/* Java Time */
	{"jul", MONTH, 7},
	{"july", MONTH, 7},
	{"jun", MONTH, 6},
	{"june", MONTH, 6},
	{"kst", TZ, 54},			/* Korea Standard Time */
	{"ligt", TZ, 60},			/* From Melbourne, Australia */
	{"mar", MONTH, 3},
	{"march", MONTH, 3},
	{"may", MONTH, 5},
	{"mdt", DTZ, NEG(36)},		/* Mountain Daylight Time */
	{"mest", DTZ, 12},			/* Middle Europe Summer Time */
	{"met", TZ, 6},				/* Middle Europe Time */
	{"metdst", DTZ, 12},		/* Middle Europe Daylight Time */
	{"mewt", TZ, 6},			/* Middle Europe Winter Time */
	{"mez", TZ, 6},				/* Middle Europe Zone */
	{"mon", DOW, 1},
	{"monday", DOW, 1},
	{"mst", TZ, NEG(42)},		/* Mountain Standard Time */
	{"mt", TZ, 51},				/* Moluccas Time */
	{"ndt", DTZ, NEG(15)},		/* Nfld. Daylight Time */
	{"nft", TZ, NEG(21)},		/* Newfoundland Standard Time */
	{"nor", TZ, 6},				/* Norway Standard Time */
	{"nov", MONTH, 11},
	{"november", MONTH, 11},
	{NOW, RESERV, DTK_NOW},		/* current transaction time */
	{"nst", TZ, NEG(21)},		/* Nfld. Standard Time */
	{"nt", TZ, NEG(66)},		/* Nome Time */
	{"nzdt", DTZ, 78},			/* New Zealand Daylight Time */
	{"nzst", TZ, 72},			/* New Zealand Standard Time */
	{"nzt", TZ, 72},			/* New Zealand Time */
	{"oct", MONTH, 10},
	{"october", MONTH, 10},
	{"on", IGNOREFIELD, 0},			/* "on" (throwaway) */
	{"pdt", DTZ, NEG(42)},		/* Pacific Daylight Time */
	{"pm", AMPM, PM},
	{"pst", TZ, NEG(48)},		/* Pacific Standard Time */
	{"sadt", DTZ, 63},			/* S. Australian Dayl. Time */
	{"sast", TZ, 57},			/* South Australian Std Time */
	{"sat", DOW, 6},
	{"saturday", DOW, 6},
	{"sep", MONTH, 9},
	{"sept", MONTH, 9},
	{"september", MONTH, 9},
	{"set", TZ, NEG(6)},		/* Seychelles Time ?? */
	{"sst", DTZ, 12},			/* Swedish Summer Time */
	{"sun", DOW, 0},
	{"sunday", DOW, 0},
	{"swt", TZ, 6},				/* Swedish Winter Time	*/
	{"thu", DOW, 4},
	{"thur", DOW, 4},
	{"thurs", DOW, 4},
	{"thursday", DOW, 4},
	{TODAY, RESERV, DTK_TODAY}, /* midnight */
	{TOMORROW, RESERV, DTK_TOMORROW},	/* tomorrow midnight */
	{"tue", DOW, 2},
	{"tues", DOW, 2},
	{"tuesday", DOW, 2},
	{"undefined", RESERV, DTK_INVALID}, /* "undefined" pre-v6.1 invalid
										 * time */
	{"ut", TZ, 0},
	{"utc", TZ, 0},
	{"wadt", DTZ, 48},			/* West Australian DST */
	{"wast", TZ, 42},			/* West Australian Std Time */
	{"wat", TZ, NEG(6)},		/* West Africa Time */
	{"wdt", DTZ, 54},			/* West Australian DST */
	{"wed", DOW, 3},
	{"wednesday", DOW, 3},
	{"weds", DOW, 3},
	{"wet", TZ, 0},				/* Western Europe */
	{"wetdst", DTZ, 6},			/* Western Europe */
	{"wst", TZ, 48},			/* West Australian Std Time */
	{"ydt", DTZ, NEG(48)},		/* Yukon Daylight Time */
	{YESTERDAY, RESERV, DTK_YESTERDAY}, /* yesterday midnight */
	{"yst", TZ, NEG(54)},		/* Yukon Standard Time */
	{"zp4", TZ, NEG(24)},		/* GMT +4  hours. */
	{"zp5", TZ, NEG(30)},		/* GMT +5  hours. */
	{"zp6", TZ, NEG(36)},		/* GMT +6  hours. */
	{"z", RESERV, DTK_ZULU},	/* 00:00:00 */
	{ZULU, RESERV, DTK_ZULU},	/* 00:00:00 */
};

static unsigned int szdatetktbl = sizeof datetktbl / sizeof datetktbl[0];



#if USE_DATE_CACHE
datetkn    *datecache[MAXDATEFIELDS] = {NULL};

datetkn    *deltacache[MAXDATEFIELDS] = {NULL};

#endif


/*
 * Calendar time to Julian date conversions.
 * Julian date is commonly used in astronomical applications,
 *	since it is numerically accurate and computationally simple.
 * The algorithms here will accurately convert between Julian day
 *	and calendar date for all non-negative Julian days
 *	(i.e. from Nov 23, -4713 on).
 *
 * Ref: Explanatory Supplement to the Astronomical Almanac, 1992.
 *	University Science Books, 20 Edgehill Rd. Mill Valley CA 94941.
 *
 * Use the algorithm by Henry Fliegel, a former NASA/JPL colleague
 *	now at Aerospace Corp. (hi, Henry!)
 *
 * These routines will be used by other date/time packages - tgl 97/02/25
 */

/* Set the minimum year to one greater than the year of the first valid day
 *	to avoid having to check year and day both. - tgl 97/05/08
 */

#define JULIAN_MINYEAR (-4713)
#define JULIAN_MINMONTH (11)
#define JULIAN_MINDAY (23)

#define IS_VALID_JULIAN(y,m,d) ((y > JULIAN_MINYEAR) \
 || ((y == JULIAN_MINYEAR) && ((m > JULIAN_MINMONTH) \
  || ((m == JULIAN_MINMONTH) && (d >= JULIAN_MINDAY)))))

int
date2j(int y, int m, int d)
{
	int			m12 = (m - 14) / 12;

	return ((1461 * (y + 4800 + m12)) / 4 + (367 * (m - 2 - 12 * (m12))) / 12
			- (3 * ((y + 4900 + m12) / 100)) / 4 + d - 32075);
}	/* date2j() */

void
j2date(int jd, int *year, int *month, int *day)
{
	int			j,
				y,
				m,
				d;

	int			i,
				l,
				n;

	l = jd + 68569;
	n = (4 * l) / 146097;
	l -= (146097 * n + 3) / 4;
	i = (4000 * (l + 1)) / 1461001;
	l += 31 - (1461 * i) / 4;
	j = (80 * l) / 2447;
	d = l - (2447 * j) / 80;
	l = j / 11;
	m = (j + 2) - (12 * l);
	y = 100 * (n - 49) + i + l;

	*year = y;
	*month = m;
	*day = d;
	return;
}	/* j2date() */




/*
 * parse and convert date in timestr (the normal interface)
 *
 * Returns the number of seconds since epoch (J2000)
 */

/* ParseDateTime()
 * Break string into tokens based on a date/time context.
 */
int
ParseDateTime(char *timestr, char *lowstr,
			  char **field, int *ftype, int maxfields, int *numfields)
{
	int			nf = 0;
	char	   *cp = timestr;
	char	   *lp = lowstr;

#ifdef DATEDEBUG
	printf("ParseDateTime- input string is %s\n", timestr);
#endif
	/* outer loop through fields */
	while (*cp != '\0')
	{
		field[nf] = lp;

		/* leading digit? then date or time */
		if (isdigit(*cp) || (*cp == '.'))
		{
			*lp++ = *cp++;
			while (isdigit(*cp))
				*lp++ = *cp++;
			/* time field? */
			if (*cp == ':')
			{
				ftype[nf] = DTK_TIME;
				while (isdigit(*cp) || (*cp == ':') || (*cp == '.'))
					*lp++ = *cp++;

			}
			/* date field? allow embedded text month */
			else if ((*cp == '-') || (*cp == '/') || (*cp == '.'))
			{
				ftype[nf] = DTK_DATE;
				while (isalnum(*cp) || (*cp == '-') || (*cp == '/') || (*cp == '.'))
					*lp++ = tolower(*cp++);

			}

			/*
			 * otherwise, number only and will determine year, month, or
			 * day later
			 */
			else
				ftype[nf] = DTK_NUMBER;

		}

		/*
		 * text? then date string, month, day of week, special, or
		 * timezone
		 */
		else if (isalpha(*cp))
		{
			ftype[nf] = DTK_STRING;
			*lp++ = tolower(*cp++);
			while (isalpha(*cp))
				*lp++ = tolower(*cp++);

			/* full date string with leading text month? */
			if ((*cp == '-') || (*cp == '/') || (*cp == '.'))
			{
				ftype[nf] = DTK_DATE;
				while (isdigit(*cp) || (*cp == '-') || (*cp == '/') || (*cp == '.'))
					*lp++ = tolower(*cp++);
			}

			/* skip leading spaces */
		}
		else if (isspace(*cp))
		{
			cp++;
			continue;

			/* sign? then special or numeric timezone */
		}
		else if ((*cp == '+') || (*cp == '-'))
		{
			*lp++ = *cp++;
			/* soak up leading whitespace */
			while (isspace(*cp))
				cp++;
			/* numeric timezone? */
			if (isdigit(*cp))
			{
				ftype[nf] = DTK_TZ;
				*lp++ = *cp++;
				while (isdigit(*cp) || (*cp == ':'))
					*lp++ = *cp++;

				/* special? */
			}
			else if (isalpha(*cp))
			{
				ftype[nf] = DTK_SPECIAL;
				*lp++ = tolower(*cp++);
				while (isalpha(*cp))
					*lp++ = tolower(*cp++);

				/* otherwise something wrong... */
			}
			else
				return -1;

			/* ignore punctuation but use as delimiter */
		}
		else if (ispunct(*cp))
		{
			cp++;
			continue;

		}
		else
			return -1;

		/* force in a delimiter */
		*lp++ = '\0';
		nf++;
		if (nf > MAXDATEFIELDS)
			return -1;
#ifdef DATEDEBUG
		printf("ParseDateTime- set field[%d] to %s type %d\n", (nf - 1), field[nf - 1], ftype[nf - 1]);
#endif
	}

	*numfields = nf;

	return 0;
}	/* ParseDateTime() */


/* DecodeDateTime()
 * Interpret previously parsed fields for general date and time.
 * Return 0 if full date, 1 if only time, and -1 if problems.
 *		External format(s):
 *				"<weekday> <month>-<day>-<year> <hour>:<minute>:<second>"
 *				"Fri Feb-7-1997 15:23:27"
 *				"Feb-7-1997 15:23:27"
 *				"2-7-1997 15:23:27"
 *				"1997-2-7 15:23:27"
 *				"1997.038 15:23:27"		(day of year 1-366)
 *		Also supports input in compact time:
 *				"970207 152327"
 *				"97038 152327"
 *
 * Use the system-provided functions to get the current time zone
 *	if not specified in the input string.
 * If the date is outside the time_t system-supported time range,
 *	then assume GMT time zone. - tgl 97/05/27
 */
int
DecodeDateTime(char **field, int *ftype, int nf,
			   int *dtype, struct tm * tm, double *fsec, int *tzp)
{
	int			fmask = 0,
				tmask,
				type;
	int			i;
	int			flen,
				val;
	int			mer = HR24;
	int			bc = FALSE;

	*dtype = DTK_DATE;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	*fsec = 0;
	tm->tm_isdst = -1;			/* don't know daylight savings time status
								 * apriori */
	if (tzp != NULL)
		*tzp = 0;

	for (i = 0; i < nf; i++)
	{
#ifdef DATEDEBUG
		printf("DecodeDateTime- field[%d] is %s (type %d)\n", i, field[i], ftype[i]);
#endif
		switch (ftype[i])
		{
			case DTK_DATE:
				if (DecodeDate(field[i], fmask, &tmask, tm) != 0)
					return -1;
				break;

			case DTK_TIME:
				if (DecodeTime(field[i], fmask, &tmask, tm, fsec) != 0)
					return -1;

				/*
				 * check upper limit on hours; other limits checked in
				 * DecodeTime()
				 */
				if (tm->tm_hour > 23)
					return -1;
				break;

			case DTK_TZ:
				if (tzp == NULL)
					return -1;
				if (DecodeTimezone(field[i], tzp) != 0)
					return -1;
				tmask = DTK_M(TZ);
				break;

			case DTK_NUMBER:
				flen = strlen(field[i]);

				if (flen > 4)
				{
					if (DecodeNumberField(flen, field[i], fmask, &tmask, tm, fsec) != 0)
						return -1;

				}
				else
				{
					if (DecodeNumber(flen, field[i], fmask, &tmask, tm, fsec) != 0)
						return -1;
				}
				break;

			case DTK_STRING:
			case DTK_SPECIAL:
				type = DecodeSpecial(i, field[i], &val);
#ifdef DATEDEBUG
				printf("DecodeDateTime- special field[%d] %s type=%d value=%d\n", i, field[i], type, val);
#endif
				if (type == IGNOREFIELD)
					continue;

				tmask = DTK_M(type);
				switch (type)
				{
					case RESERV:
#ifdef DATEDEBUG
						printf("DecodeDateTime- RESERV field %s value is %d\n", field[i], val);
#endif
						switch (val)
						{

							default:
								*dtype = val;
						}

						break;

					case MONTH:
#ifdef DATEDEBUG
						printf("DecodeDateTime- month field %s value is %d\n", field[i], val);
#endif
						tm->tm_mon = val;
						break;

						/*
						 * daylight savings time modifier (solves "MET
						 * DST" syntax)
						 */
					case DTZMOD:
						tmask |= DTK_M(DTZ);
						tm->tm_isdst = 1;
						if (tzp == NULL)
							return -1;
						*tzp += val * 60;
						break;

					case DTZ:

						/*
						 * set mask for TZ here _or_ check for DTZ later
						 * when getting default timezone
						 */
						tmask |= DTK_M(TZ);
						tm->tm_isdst = 1;
						if (tzp == NULL)
							return -1;
						*tzp = val * 60;
						break;

					case TZ:
						tm->tm_isdst = 0;
						if (tzp == NULL)
							return -1;
						*tzp = val * 60;
						break;

					case IGNOREFIELD:
						break;

					case AMPM:
						mer = val;
						break;

					case ADBC:
						bc = (val == BC);
						break;

					case DOW:
						tm->tm_wday = val;
						break;

					default:
						return -1;
				}
				break;

			default:
				return -1;
		}

#ifdef DATEDEBUG
		printf("DecodeDateTime- field[%d] %s (%08x/%08x) value is %d\n",
			   i, field[i], fmask, tmask, val);
#endif

		if (tmask & fmask)
			return -1;
		fmask |= tmask;
	}

	/* there is no year zero in AD/BC notation; i.e. "1 BC" == year 0 */
	if (bc)
		tm->tm_year = -(tm->tm_year - 1);

	if ((mer != HR24) && (tm->tm_hour > 12))
		return -1;
	if ((mer == AM) && (tm->tm_hour == 12))
		tm->tm_hour = 0;
	else if ((mer == PM) && (tm->tm_hour != 12))
		tm->tm_hour += 12;

#ifdef DATEDEBUG
	printf("DecodeDateTime- mask %08x (%08x)", fmask, DTK_DATE_M);
	printf(" set y%04d m%02d d%02d", tm->tm_year, tm->tm_mon, tm->tm_mday);
	printf(" %02d:%02d:%02d\n", tm->tm_hour, tm->tm_min, tm->tm_sec);
#endif

	if ((*dtype == DTK_DATE) && ((fmask & DTK_DATE_M) != DTK_DATE_M))
		return ((fmask & DTK_TIME_M) == DTK_TIME_M) ? 1 : -1;

	/* timezone not specified? then find local timezone if possible */
	if ((*dtype == DTK_DATE) && ((fmask & DTK_DATE_M) == DTK_DATE_M)
		&& (tzp != NULL) && (!(fmask & DTK_M(TZ))))
	{

		/*
		 * daylight savings time modifier but no standard timezone? then
		 * error
		 */
		if (fmask & DTK_M(DTZMOD))
			return -1;

		if (IS_VALID_UTIME(tm->tm_year, tm->tm_mon, tm->tm_mday))
		{
#ifdef USE_POSIX_TIME
			tm->tm_year -= 1900;
			tm->tm_mon -= 1;
			tm->tm_isdst = -1;
			mktime(tm);
			tm->tm_year += 1900;
			tm->tm_mon += 1;

#ifdef HAVE_INT_TIMEZONE
			*tzp = ((tm->tm_isdst > 0) ? (timezone - 3600) : timezone);

#else							/* !HAVE_INT_TIMEZONE */
			*tzp = -(tm->tm_gmtoff);	/* tm_gmtoff is Sun/DEC-ism */
#endif

#else							/* !USE_POSIX_TIME */
			*tzp = CTimeZone;
#endif
		}
		else
		{
			tm->tm_isdst = 0;
			*tzp = 0;
		}
	}

	return 0;
}	/* DecodeDateTime() */


/* DecodeTimeOnly()
 * Interpret parsed string as time fields only.
 */
int
DecodeTimeOnly(char **field, int *ftype, int nf, int *dtype, struct tm * tm, double *fsec)
{
	int			fmask,
				tmask,
				type;
	int			i;
	int			flen,
				val;
	int			mer = HR24;

	*dtype = DTK_TIME;
	tm->tm_hour = 0;
	tm->tm_min = 0;
	tm->tm_sec = 0;
	tm->tm_isdst = -1;			/* don't know daylight savings time status
								 * apriori */
	*fsec = 0;

	fmask = DTK_DATE_M;

	for (i = 0; i < nf; i++)
	{
#ifdef DATEDEBUG
		printf("DecodeTimeOnly- field[%d] is %s (type %d)\n", i, field[i], ftype[i]);
#endif
		switch (ftype[i])
		{
			case DTK_TIME:
				if (DecodeTime(field[i], fmask, &tmask, tm, fsec) != 0)
					return -1;
				break;

			case DTK_NUMBER:
				flen = strlen(field[i]);

				if (DecodeNumberField(flen, field[i], fmask, &tmask, tm, fsec) != 0)
					return -1;
				break;

			case DTK_STRING:
			case DTK_SPECIAL:
				type = DecodeSpecial(i, field[i], &val);
#ifdef DATEDEBUG
				printf("DecodeTimeOnly- special field[%d] %s type=%d value=%d\n", i, field[i], type, val);
#endif
				if (type == IGNOREFIELD)
					continue;

				tmask = DTK_M(type);
				switch (type)
				{
					case RESERV:
#ifdef DATEDEBUG
						printf("DecodeTimeOnly- RESERV field %s value is %d\n", field[i], val);
#endif
						switch (val)
						{

							default:
								return -1;
						}

						break;

					case IGNOREFIELD:
						break;

					case AMPM:
						mer = val;
						break;

					default:
						return -1;
				}
				break;

			default:
				return -1;
		}

		if (tmask & fmask)
			return -1;
		fmask |= tmask;

#ifdef DATEDEBUG
		printf("DecodeTimeOnly- field[%d] %s value is %d\n", i, field[i], val);
#endif
	}

#ifdef DATEDEBUG
	printf("DecodeTimeOnly- mask %08x (%08x)", fmask, DTK_TIME_M);
	printf(" %02d:%02d:%02d (%f)\n", tm->tm_hour, tm->tm_min, tm->tm_sec, *fsec);
#endif

	if ((mer != HR24) && (tm->tm_hour > 12))
		return -1;
	if ((mer == AM) && (tm->tm_hour == 12))
		tm->tm_hour = 0;
	else if ((mer == PM) && (tm->tm_hour != 12))
		tm->tm_hour += 12;

	if ((fmask & DTK_TIME_M) != DTK_TIME_M)
		return -1;

	return 0;
}	/* DecodeTimeOnly() */


/* DecodeDate()
 * Decode date string which includes delimiters.
 * Insist on a complete set of fields.
 */
static int
DecodeDate(char *str, int fmask, int *tmask, struct tm * tm)
{
	double		fsec;

	int			nf = 0;
	int			i,
				len;
	int			type,
				val,
				dmask = 0;
	char	   *field[MAXDATEFIELDS];

	/* parse this string... */
	while ((*str != '\0') && (nf < MAXDATEFIELDS))
	{
		/* skip field separators */
		while (!isalnum(*str))
			str++;

		field[nf] = str;
		if (isdigit(*str))
		{
			while (isdigit(*str))
				str++;
		}
		else if (isalpha(*str))
		{
			while (isalpha(*str))
				str++;
		}

		if (*str != '\0')
			*str++ = '\0';
		nf++;
	}

	/* don't allow too many fields */
	if (nf > 3)
		return -1;

	*tmask = 0;

	/* look first for text fields, since that will be unambiguous month */
	for (i = 0; i < nf; i++)
	{
		if (isalpha(*field[i]))
		{
			type = DecodeSpecial(i, field[i], &val);
			if (type == IGNOREFIELD)
				continue;

			dmask = DTK_M(type);
			switch (type)
			{
				case MONTH:
#ifdef DATEDEBUG
					printf("DecodeDate- month field %s value is %d\n", field[i], val);
#endif
					tm->tm_mon = val;
					break;

				default:
#ifdef DATEDEBUG
					printf("DecodeDate- illegal field %s value is %d\n", field[i], val);
#endif
					return -1;
			}
			if (fmask & dmask)
				return -1;

			fmask |= dmask;
			*tmask |= dmask;

			/* mark this field as being completed */
			field[i] = NULL;
		}
	}

	/* now pick up remaining numeric fields */
	for (i = 0; i < nf; i++)
	{
		if (field[i] == NULL)
			continue;

		if ((len = strlen(field[i])) <= 0)
			return -1;

		if (DecodeNumber(len, field[i], fmask, &dmask, tm, &fsec) != 0)
			return -1;

		if (fmask & dmask)
			return -1;

		fmask |= dmask;
		*tmask |= dmask;
	}

	return 0;
}	/* DecodeDate() */


/* DecodeTime()
 * Decode time string which includes delimiters.
 * Only check the lower limit on hours, since this same code
 *	can be used to represent time spans.
 */
static int
DecodeTime(char *str, int fmask, int *tmask, struct tm * tm, double *fsec)
{
	char	   *cp;

	*tmask = DTK_TIME_M;

	tm->tm_hour = strtol(str, &cp, 10);
	if (*cp != ':')
		return -1;
	str = cp + 1;
	tm->tm_min = strtol(str, &cp, 10);
	if (*cp == '\0')
	{
		tm->tm_sec = 0;
		*fsec = 0;

	}
	else if (*cp != ':')
	{
		return -1;

	}
	else
	{
		str = cp + 1;
		tm->tm_sec = strtol(str, &cp, 10);
		if (*cp == '\0')
			*fsec = 0;
		else if (*cp == '.')
		{
			str = cp;
			*fsec = strtod(str, &cp);
			if (cp == str)
				return -1;
		}
		else
			return -1;
	}

	/* do a sanity check */
	if ((tm->tm_hour < 0)
		|| (tm->tm_min < 0) || (tm->tm_min > 59)
		|| (tm->tm_sec < 0) || (tm->tm_sec > 59))
		return -1;

	return 0;
}	/* DecodeTime() */


/* DecodeNumber()
 * Interpret numeric field as a date value in context.
 */
static int
DecodeNumber(int flen, char *str, int fmask, int *tmask, struct tm * tm, double *fsec)
{
	int			val;
	char	   *cp;

	*tmask = 0;

	val = strtol(str, &cp, 10);
	if (cp == str)
		return -1;
	if (*cp == '.')
	{
		*fsec = strtod(cp, &cp);
		if (*cp != '\0')
			return -1;
	}

#ifdef DATEDEBUG
	printf("DecodeNumber- %s is %d fmask=%08x tmask=%08x\n", str, val, fmask, *tmask);
#endif

	/* enough digits to be unequivocal year? */
	if (flen == 4)
	{
#ifdef DATEDEBUG
		printf("DecodeNumber- match %d (%s) as year\n", val, str);
#endif
		*tmask = DTK_M(YEAR);

		/* already have a year? then see if we can substitute... */
		if (fmask & DTK_M(YEAR))
		{
			if ((!(fmask & DTK_M(DAY)))
				&& ((tm->tm_year >= 1) && (tm->tm_year <= 31)))
			{
#ifdef DATEDEBUG
				printf("DecodeNumber- misidentified year previously; swap with day %d\n", tm->tm_mday);
#endif
				tm->tm_mday = tm->tm_year;
				*tmask = DTK_M(DAY);
			}
		}

		tm->tm_year = val;

		/* special case day of year? */
	}
	else if ((flen == 3) && (fmask & DTK_M(YEAR))
			 && ((val >= 1) && (val <= 366)))
	{
		*tmask = (DTK_M(DOY) | DTK_M(MONTH) | DTK_M(DAY));
		tm->tm_yday = val;
		j2date((date2j(tm->tm_year, 1, 1) + tm->tm_yday - 1),
			   &tm->tm_year, &tm->tm_mon, &tm->tm_mday);

		/* already have year? then could be month */
	}
	else if ((fmask & DTK_M(YEAR)) && (!(fmask & DTK_M(MONTH)))
			 && ((val >= 1) && (val <= 12)))
	{
#ifdef DATEDEBUG
		printf("DecodeNumber- match %d (%s) as month\n", val, str);
#endif
		*tmask = DTK_M(MONTH);
		tm->tm_mon = val;

		/* no year and EuroDates enabled? then could be day */
	}
	else if ((EuroDates || (fmask & DTK_M(MONTH)))
			 && (!(fmask & DTK_M(YEAR)) && !(fmask & DTK_M(DAY)))
			 && ((val >= 1) && (val <= 31)))
	{
#ifdef DATEDEBUG
		printf("DecodeNumber- match %d (%s) as day\n", val, str);
#endif
		*tmask = DTK_M(DAY);
		tm->tm_mday = val;

	}
	else if ((!(fmask & DTK_M(MONTH)))
			 && ((val >= 1) && (val <= 12)))
	{
#ifdef DATEDEBUG
		printf("DecodeNumber- (2) match %d (%s) as month\n", val, str);
#endif
		*tmask = DTK_M(MONTH);
		tm->tm_mon = val;

	}
	else if ((!(fmask & DTK_M(DAY)))
			 && ((val >= 1) && (val <= 31)))
	{
#ifdef DATEDEBUG
		printf("DecodeNumber- (2) match %d (%s) as day\n", val, str);
#endif
		*tmask = DTK_M(DAY);
		tm->tm_mday = val;

	}
	else if (!(fmask & DTK_M(YEAR)))
	{
#ifdef DATEDEBUG
		printf("DecodeNumber- (2) match %d (%s) as year\n", val, str);
#endif
		*tmask = DTK_M(YEAR);
		tm->tm_year = val;
		if (tm->tm_year < 70)
			tm->tm_year += 2000;
		else if (tm->tm_year < 100)
			tm->tm_year += 1900;

	}
	else
		return -1;

	return 0;
}	/* DecodeNumber() */


/* DecodeNumberField()
 * Interpret numeric string as a concatenated date field.
 */
static int
DecodeNumberField(int len, char *str, int fmask, int *tmask, struct tm * tm, double *fsec)
{
	char	   *cp;

	/* yyyymmdd? */
	if (len == 8)
	{
#ifdef DATEDEBUG
		printf("DecodeNumberField- %s is 8 character date fmask=%08x tmask=%08x\n", str, fmask, *tmask);
#endif

		*tmask = DTK_DATE_M;

		tm->tm_mday = atoi(str + 6);
		*(str + 6) = '\0';
		tm->tm_mon = atoi(str + 4);
		*(str + 4) = '\0';
		tm->tm_year = atoi(str + 0);

		/* yymmdd or hhmmss? */
	}
	else if (len == 6)
	{
#ifdef DATEDEBUG
		printf("DecodeNumberField- %s is 6 characters fmask=%08x tmask=%08x\n", str, fmask, *tmask);
#endif
		if (fmask & DTK_DATE_M)
		{
#ifdef DATEDEBUG
			printf("DecodeNumberField- %s is time field fmask=%08x tmask=%08x\n", str, fmask, *tmask);
#endif
			*tmask = DTK_TIME_M;
			tm->tm_sec = atoi(str + 4);
			*(str + 4) = '\0';
			tm->tm_min = atoi(str + 2);
			*(str + 2) = '\0';
			tm->tm_hour = atoi(str + 0);

		}
		else
		{
#ifdef DATEDEBUG
			printf("DecodeNumberField- %s is date field fmask=%08x tmask=%08x\n", str, fmask, *tmask);
#endif
			*tmask = DTK_DATE_M;
			tm->tm_mday = atoi(str + 4);
			*(str + 4) = '\0';
			tm->tm_mon = atoi(str + 2);
			*(str + 2) = '\0';
			tm->tm_year = atoi(str + 0);
		}

	}
	else if (strchr(str, '.') != NULL)
	{
#ifdef DATEDEBUG
		printf("DecodeNumberField- %s is time field fmask=%08x tmask=%08x\n", str, fmask, *tmask);
#endif
		*tmask = DTK_TIME_M;
		tm->tm_sec = strtod((str + 4), &cp);
		if (cp == (str + 4))
			return -1;
		if (*cp == '.')
			*fsec = strtod(cp, NULL);
		*(str + 4) = '\0';
		tm->tm_min = strtod((str + 2), &cp);
		*(str + 2) = '\0';
		tm->tm_hour = strtod((str + 0), &cp);

	}
	else
		return -1;

	return 0;
}	/* DecodeNumberField() */


/* DecodeTimezone()
 * Interpret string as a numeric timezone.
 */
static int
DecodeTimezone(char *str, int *tzp)
{
	int			tz;
	int			hr,
				min;
	char	   *cp;
	int			len;

	/* assume leading character is "+" or "-" */
	hr = strtol((str + 1), &cp, 10);

	/* explicit delimiter? */
	if (*cp == ':')
	{
		min = strtol((cp + 1), &cp, 10);

		/* otherwise, might have run things together... */
	}
	else if ((*cp == '\0') && ((len = strlen(str)) > 3))
	{
		min = strtol((str + len - 2), &cp, 10);
		*(str + len - 2) = '\0';
		hr = strtol((str + 1), &cp, 10);

	}
	else
		min = 0;

	tz = (hr * 60 + min) * 60;
	if (*str == '-')
		tz = -tz;

	*tzp = -tz;
	return *cp != '\0';
}	/* DecodeTimezone() */


/* DecodeSpecial()
 * Decode text string using lookup table.
 * Implement a cache lookup since it is likely that dates
 *	will be related in format.
 */
static int
DecodeSpecial(int field, char *lowtoken, int *val)
{
	int			type;
	datetkn    *tp;

#if USE_DATE_CACHE
	if ((datecache[field] != NULL)
		&& (strncmp(lowtoken, datecache[field]->token, TOKMAXLEN) == 0))
		tp = datecache[field];
	else
	{
#endif
		tp = datebsearch(lowtoken, datetktbl, szdatetktbl);
#if USE_DATE_CACHE
	}
	datecache[field] = tp;
#endif
	if (tp == NULL)
	{
		type = IGNOREFIELD;
		*val = 0;
	}
	else
	{
		type = tp->type;
		switch (type)
		{
			case TZ:
			case DTZ:
			case DTZMOD:
				*val = FROMVAL(tp);
				break;

			default:
				*val = tp->value;
				break;
		}
	}

	return type;
}	/* DecodeSpecial() */



/* datebsearch()
 * Binary search -- from Knuth (6.2.1) Algorithm B.  Special case like this
 * is WAY faster than the generic bsearch().
 */
static datetkn *
datebsearch(char *key, datetkn *base, unsigned int nel)
{
	datetkn    *last = base + nel - 1,
			   *position;
	int			result;

	while (last >= base)
	{
		position = base + ((last - base) >> 1);
		result = key[0] - position->token[0];
		if (result == 0)
		{
			result = strncmp(key, position->token, TOKMAXLEN);
			if (result == 0)
				return position;
		}
		if (result < 0)
			last = position - 1;
		else
			base = position + 1;
	}
	return NULL;
}



