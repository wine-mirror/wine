/*
 * VARIANT
 *
 * Copyright 1998 Jean-Claude Cote
 *
 * NOTES
 *   This implements the low-level and hi-level APIs for manipulating VARIANTs.
 *   The low-level APIs are used to do data coercion between different data types.
 *   The hi-level APIs are built on top of these low-level APIs and handle
 *   initialization, copying, destroying and changing the type of VARIANTs.
 *
 * TODO:
 *   - The Variant APIs are do not support international languages, currency
 *     types, number formating and calendar.  They only support U.S. English format.
 *   - The Variant APIs do not the following types: IUknown, IDispatch, DECIMAL and SafeArray.
 *     The prototypes for these are commented out in the oleauto.h file.  They need
 *     to be implemented and cases need to be added to the switches of the  existing APIs.
 *   - The parsing of date for the VarDateFromStr is not complete.
 *   - The date manipulations do not support date prior to 1900.
 *   - The parsing does not accept has many formats has the Windows implementation.
 */
 
#include "wintypes.h"
#include "oleauto.h"
#include "heap.h"
#include "debug.h"
#include "winerror.h"
#include "mapidefs.h"
#include "parsedt.h"

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#ifndef FLT_MAX
# ifdef MAXFLOAT
#  define FLT_MAX MAXFLOAT
# else
#  error "Can't find #define for MAXFLOAT/FLT_MAX"
# endif
#endif

#undef CHAR_MAX
#undef CHAR_MIN
static const char CHAR_MAX = 127;
static const char CHAR_MIN = -128;
static const BYTE UI1_MAX = 255;
static const BYTE UI1_MIN = 0;
static const unsigned short UI2_MAX = 65535;
static const unsigned short UI2_MIN = 0;
static const short I2_MAX = 32767;
static const short I2_MIN =  -32768;
static const unsigned long UI4_MAX = 4294967295U;
static const unsigned long UI4_MIN = 0;
static const long I4_MAX = 2147483647;
static const long I4_MIN = -(2147483648U);
static const DATE DATE_MIN = -657434;
static const DATE DATE_MAX = 2958465;


/* This mask is used to set a flag in wReserved1 of
 * the VARIANTARG structure. The flag indicates if
 * the API function is using an inner variant or not.
 */
#define PROCESSING_INNER_VARIANT 0x0001

/* General use buffer.
 */
#define BUFFER_MAX 1024
static char pBuffer[BUFFER_MAX];

/*
 * Note a leap year is one that is a multiple of 4
 * but not of a 100.  Except if it is a multiple of
 * 400 then it is a leap year.
 */
/* According to postgeSQL date parsing functions there is
 * a leap year when this expression is true.
 * (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))
 * So according to this there is 365.2515 days in one year.
 * One + every four years: 1/4 -> 365.25
 * One - every 100 years: 1/100 -> 365.001
 * One + every 400 years: 1/400 -> 365.0025
 */
static const double DAYS_IN_ONE_YEAR = 365.2515;



/******************************************************************************
 *	   DateTimeStringToTm	[INTERNAL]
 *
 * Converts a string representation of a date and/or time to a tm structure.
 *
 * Note this function uses the postgresql date parsing functions found
 * in the parsedt.c file.
 *
 * Returns TRUE if successfull.
 *
 * Note: This function does not parse the day of the week,
 * daylight savings time. It will only fill the followin fields in
 * the tm struct, tm_sec, tm_min, tm_hour, tm_year, tm_day, tm_mon.
 *
 ******************************************************************************/
static BOOL DateTimeStringToTm( OLECHAR* strIn, LCID lcid, struct tm* pTm )
{
	BOOL res = FALSE;
	double		fsec;
	int 		tzp;
	int 		dtype;
	int 		nf;
	char	   *field[MAXDATEFIELDS];
	int 		ftype[MAXDATEFIELDS];
	char		lowstr[MAXDATELEN + 1];
	char* strDateTime = NULL;

	/* Convert the string to ASCII since this is the only format
	 * postgesql can handle.
	 */
	strDateTime = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );

	if( strDateTime != NULL )
	{
		/* Make sure we don't go over the maximum length
		 * accepted by postgesql.
		 */
		if( strlen( strDateTime ) <= MAXDATELEN )
		{
			if( ParseDateTime( strDateTime, lowstr, field, ftype, MAXDATEFIELDS, &nf) == 0 )
			{
				if( lcid & VAR_DATEVALUEONLY )
				{
					/* Get the date information.
					 * It returns 0 if date information was
					 * present and 1 if only time information was present.
					 * -1 if an error occures.
					 */
					if( DecodeDateTime(field, ftype, nf, &dtype, pTm, &fsec, &tzp) == 0 )
					{
						/* Eliminate the time information since we
						 * were asked to get date information only.
						 */
						pTm->tm_sec = 0;
						pTm->tm_min = 0;
						pTm->tm_hour = 0;
						res = TRUE;
					}
				}
				if( lcid & VAR_TIMEVALUEONLY )
				{
					/* Get time information only.
					 */
					if( DecodeTimeOnly(field, ftype, nf, &dtype, pTm, &fsec) == 0 )
					{
						res = TRUE;
					}
				}
				else
				{
					/* Get both date and time information.
					 * It returns 0 if date information was
					 * present and 1 if only time information was present.
					 * -1 if an error occures.
					 */
					if( DecodeDateTime(field, ftype, nf, &dtype, pTm, &fsec, &tzp) != -1 )
					{
						res = TRUE;
					}
				}
			}
		}
		HeapFree( GetProcessHeap(), 0, strDateTime );
	}

	return res;
}






/******************************************************************************
 *	   TmToDATE 	[INTERNAL]
 *
 * The date is implemented using an 8 byte floating-point number.
 * Days are represented by whole numbers increments starting with 0.00 has
 * being December 30 1899, midnight.
 * The hours are expressed as the fractional part of the number.
 * December 30 1899 at midnight = 0.00
 * January 1 1900 at midnight = 2.00
 * January 4 1900 at 6 AM = 5.25
 * January 4 1900 at noon = 5.50
 * December 29 1899 at midnight = -1.00
 * December 18 1899 at midnight = -12.00
 * December 18 1899 at 6AM = -12.25
 * December 18 1899 at 6PM = -12.75
 * December 19 1899 at midnight = -11.00
 * The tm structure is as follows:
 * struct tm {
 *		  int tm_sec;	   seconds after the minute - [0,59]
 *		  int tm_min;	   minutes after the hour - [0,59]
 *		  int tm_hour;	   hours since midnight - [0,23]
 *		  int tm_mday;	   day of the month - [1,31]
 *		  int tm_mon;	   months since January - [0,11]
 *		  int tm_year;	   years
 *		  int tm_wday;	   days since Sunday - [0,6]
 *		  int tm_yday;	   days since January 1 - [0,365]
 *		  int tm_isdst;    daylight savings time flag
 *		  };
 *
 * Note: This function does not use the tm_wday, tm_yday, tm_wday,
 * and tm_isdst fields of the tm structure. And only converts years
 * after 1900.
 *
 * Returns TRUE if successfull.
 */
static BOOL TmToDATE( struct tm* pTm, DATE *pDateOut )
{
	if( (pTm->tm_year - 1900) >= 0 )
	{
		int leapYear = 0;
		
		/* Start at 1. This is the way DATE is defined.
		 * January 1, 1900 at Midnight is 1.00.
		 * January 1, 1900 at 6AM is 1.25.
		 * and so on.
		 */
		*pDateOut = 1;

		/* Add the number of days corresponding to
		 * tm_year.
		 */
		*pDateOut += (pTm->tm_year - 1900) * 365;

		/* Add the leap days in the previous years between now and 1900.
		 * Note a leap year is one that is a multiple of 4
		 * but not of a 100.  Except if it is a multiple of
		 * 400 then it is a leap year.
		 */
		*pDateOut += ( (pTm->tm_year - 1) / 4 ) - ( 1900 / 4 );
		*pDateOut -= ( (pTm->tm_year - 1) / 100 ) - ( 1900 / 100 );
		*pDateOut += ( (pTm->tm_year - 1) / 400 ) - ( 1900 / 400 );

		/* Set the leap year flag if the
		 * current year specified by tm_year is a
		 * leap year. This will be used to add a day
		 * to the day count.
		 */
		if( isleap( pTm->tm_year ) )
			leapYear = 1;
		
		/* Add the number of days corresponding to
		 * the month.
		 */
		switch( pTm->tm_mon )
		{
		case 2:
			*pDateOut += 31;
			break;
		case 3:
			*pDateOut += ( 59 + leapYear );
			break;
		case 4:
			*pDateOut += ( 90 + leapYear );
			break;
		case 5:
			*pDateOut += ( 120 + leapYear );
			break;
		case 6:
			*pDateOut += ( 151 + leapYear );
			break;
		case 7:
			*pDateOut += ( 181 + leapYear );
			break;
		case 8:
			*pDateOut += ( 212 + leapYear );
			break;
		case 9:
			*pDateOut += ( 243 + leapYear );
			break;
		case 10:
			*pDateOut += ( 273 + leapYear );
			break;
		case 11:
			*pDateOut += ( 304 + leapYear );
			break;
		case 12:
			*pDateOut += ( 334 + leapYear );
			break;
		}
		/* Add the number of days in this month.
		 */
		*pDateOut += pTm->tm_mday;
	
		/* Add the number of seconds, minutes, and hours
		 * to the DATE. Note these are the fracionnal part
		 * of the DATE so seconds / number of seconds in a day.
		 */
		*pDateOut += pTm->tm_hour / 24.0;
		*pDateOut += pTm->tm_min / 1440.0;
		*pDateOut += pTm->tm_sec / 86400.0;
		return TRUE;
	}
	return FALSE;
}

/******************************************************************************
 *	   DateToTm 	[INTERNAL]
 *
 * This function converst a windows DATE to a tm structure.
 *
 * It does not fill all the fields of the tm structure.
 * Here is a list of the fields that are filled:
 * tm_sec, tm_min, tm_hour, tm_year, tm_day, tm_mon.
 *
 * Note this function does not support dates before the January 1, 1900
 * or ( dateIn < 2.0 ).
 *
 * Returns TRUE if successfull.
 */
static BOOL DateToTm( DATE dateIn, LCID lcid, struct tm* pTm )
{
	/* Do not process dates smaller than January 1, 1900.
	 * Which corresponds to 2.0 in the windows DATE format.
	 */
	if( dateIn >= 2.0 )
	{
		double decimalPart = 0.0;
		double wholePart = 0.0;

		memset(pTm,0,sizeof(*pTm));
	
		/* Because of the nature of DATE format witch
		 * associates 2.0 to January 1, 1900. We will
		 * remove 1.0 from the whole part of the DATE
		 * so that in the following code 1.0
		 * will correspond to January 1, 1900.
		 * This simplyfies the processing of the DATE value.
		 */
		dateIn -= 1.0;

		wholePart = (double) floor( dateIn );
		decimalPart = fmod( dateIn, wholePart );

		if( !(lcid & VAR_TIMEVALUEONLY) )
		{
			int nDay = 0;
			int leapYear = 0;
			double yearsSince1900 = 0;
			/* Start at 1900, this where the DATE time 0.0 starts.
			 */
			pTm->tm_year = 1900;
			/* find in what year the day in the "wholePart" falls into.
			 * add the value to the year field.
			 */
			yearsSince1900 = floor( wholePart / DAYS_IN_ONE_YEAR );
			pTm->tm_year += yearsSince1900;
			/* determine if this is a leap year.
			 */
			if( isleap( pTm->tm_year ) )
				leapYear = 1;
			/* find what day of that year does the "wholePart" corresponds to.
			 * Note: nDay is in [1-366] format
			 */
			nDay = (int) ( wholePart - floor( yearsSince1900 * DAYS_IN_ONE_YEAR ) );
			/* Set the tm_yday value.
			 * Note: The day is must be converted from [1-366] to [0-365]
			 */
			/*pTm->tm_yday = nDay - 1;*/
			/* find which mount this day corresponds to.
			 */
			if( nDay <= 31 )
			{
				pTm->tm_mday = nDay;
				pTm->tm_mon = 0;
			}
			else if( nDay <= ( 59 + leapYear ) )
			{
				pTm->tm_mday = nDay - 31;
				pTm->tm_mon = 1;
			}
			else if( nDay <= ( 90 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 59 + leapYear );
				pTm->tm_mon = 2;
			}
			else if( nDay <= ( 120 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 90 + leapYear );
				pTm->tm_mon = 3;
			}
			else if( nDay <= ( 151 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 120 + leapYear );
				pTm->tm_mon = 4;
			}
			else if( nDay <= ( 181 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 151 + leapYear );
				pTm->tm_mon = 5;
			}
			else if( nDay <= ( 212 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 181 + leapYear );
				pTm->tm_mon = 6;
			}
			else if( nDay <= ( 243 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 212 + leapYear );
				pTm->tm_mon = 7;
			}
			else if( nDay <= ( 273 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 243 + leapYear );
				pTm->tm_mon = 8;
			}
			else if( nDay <= ( 304 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 273 + leapYear );
				pTm->tm_mon = 9;
			}
			else if( nDay <= ( 334 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 304 + leapYear );
				pTm->tm_mon = 10;
			}
			else if( nDay <= ( 365 + leapYear ) )
			{
				pTm->tm_mday = nDay - ( 334 + leapYear );
				pTm->tm_mon = 11;
			}
		}
		if( !(lcid & VAR_DATEVALUEONLY) )
		{
			/* find the number of seconds in this day.
			 * fractional part times, hours, minutes, seconds.
			 */
			pTm->tm_hour = (int) ( decimalPart * 24 );
			pTm->tm_min = (int) ( ( ( decimalPart * 24 ) - pTm->tm_hour ) * 60 );
			pTm->tm_sec = (int) ( ( ( decimalPart * 24 * 60 ) - ( pTm->tm_hour * 60 ) - pTm->tm_min ) * 60 );
		}
		return TRUE;
	}
	return FALSE;
}



/******************************************************************************
 *	   SizeOfVariantData   	[INTERNAL]
 *
 * This function finds the size of the data referenced by a Variant based
 * the type "vt" of the Variant.
 */
static int SizeOfVariantData( VARIANT* parg )
{
    int size = 0;
    switch( parg->vt & VT_TYPEMASK )
    {
    case( VT_I2 ):
        size = sizeof(short);
        break;
    case( VT_INT ):
        size = sizeof(int);
        break;
    case( VT_I4 ):
        size = sizeof(long);
        break;
    case( VT_UI1 ):
        size = sizeof(BYTE);
        break;
    case( VT_UI2 ):
        size = sizeof(unsigned short);
        break;
    case( VT_UINT ):
        size = sizeof(unsigned int);
        break;
    case( VT_UI4 ):
        size = sizeof(unsigned long);
        break;
    case( VT_R4 ):
        size = sizeof(float);
        break;
    case( VT_R8 ):
        size = sizeof(double);
        break;
    case( VT_DATE ):
        size = sizeof(DATE);
        break;
    case( VT_BOOL ):
        size = sizeof(VARIANT_BOOL);
        break;
    case( VT_BSTR ):
        size = sizeof(void*);
        break;
    case( VT_CY ):
    case( VT_DISPATCH ):
    case( VT_UNKNOWN ):
    case( VT_DECIMAL ):
    default:
        FIXME(ole,"Add size information for type vt=%d\n", parg->vt & VT_TYPEMASK );
        break;
    }

    return size;
}
/******************************************************************************
 *	   StringDupAtoBstr		[INTERNAL]
 * 
 */
static BSTR StringDupAtoBstr( char* strIn )
{
	BSTR bstr = NULL;
	OLECHAR* pNewString = NULL;
	pNewString = HEAP_strdupAtoW( GetProcessHeap(), 0, strIn );
	bstr = SysAllocString( pNewString );
	HeapFree( GetProcessHeap(), 0, pNewString );
	return bstr;
}

/******************************************************************************
 *		round		[INTERNAL]
 *
 * Round the double value to the nearest integer value.
 */
static double round( double d )
{
   double decimals = 0.0, integerValue = 0.0, roundedValue = 0.0;
    BOOL bEvenNumber = FALSE;
    int nSign = 0;

    /* Save the sign of the number
     */
   nSign = (d >= 0.0) ? 1 : -1;
    d = fabs( d );
    
	/* Remove the decimals.
	 */
   integerValue = floor( d );

    /* Set the Even flag.  This is used to round the number when
     * the decimals are exactly 1/2.  If the integer part is
     * odd the number is rounded up. If the integer part
     * is even the number is rounded down.  Using this method
     * numbers are rounded up|down half the time.
     */
   bEvenNumber = (((short)fmod(integerValue, 2)) == 0) ? TRUE : FALSE;

    /* Remove the integral part of the number.
     */
    decimals = d - integerValue;

	/* Note: Ceil returns the smallest integer that is greater that x.
	 * and floor returns the largest integer that is less than or equal to x.
	 */
    if( decimals > 0.5 )
    {
        /* If the decimal part is greater than 1/2
         */
        roundedValue = ceil( d );
    }
    else if( decimals < 0.5 )
    {
        /* If the decimal part is smaller than 1/2
         */
        roundedValue = floor( d );
    }
    else
    {
        /* the decimals are exactly 1/2 so round according to
         * the bEvenNumber flag.
         */
        if( bEvenNumber )
        {
            roundedValue = floor( d );
        }
        else
        {
            roundedValue = ceil( d );
        }
    }

	return roundedValue * nSign;
}

/******************************************************************************
 *		RemoveCharacterFromString		[INTERNAL]
 *
 * Removes any of the characters in "strOfCharToRemove" from the "str" argument.
 */
static void RemoveCharacterFromString( LPSTR str, LPSTR strOfCharToRemove )
{
	LPSTR pNewString = NULL;
	LPSTR strToken = NULL;


	/* Check if we have a valid argument
	 */
	if( str != NULL )
	{
		pNewString = strdup( str );
		str[0] = '\0';
		strToken = strtok( pNewString, strOfCharToRemove );
		while( strToken != NULL ) { 
			strcat( str, strToken );
			strToken = strtok( NULL, strOfCharToRemove );
		}
		free( pNewString );
	}
	return;
}

/******************************************************************************
 *		GetValidRealString		[INTERNAL]
 *
 * Checks if the string is of proper format to be converted to a real value.
 */
static BOOL IsValidRealString( LPSTR strRealString )
{
	/* Real values that have a decimal point are required to either have
	 * digits before or after the decimal point.  We will assume that
	 * we do not have any digits at either position. If we do encounter
	 * some we will disable this flag.
	 */
	BOOL bDigitsRequired = TRUE;
	/* Processed fields in the string representation of the real number.
	 */
	BOOL bWhiteSpaceProcessed = FALSE;
	BOOL bFirstSignProcessed = FALSE;
	BOOL bFirstDigitsProcessed = FALSE;
	BOOL bDecimalPointProcessed = FALSE;
	BOOL bSecondDigitsProcessed = FALSE;
	BOOL bExponentProcessed = FALSE;
	BOOL bSecondSignProcessed = FALSE;
	BOOL bThirdDigitsProcessed = FALSE;
	/* Assume string parameter "strRealString" is valid and try to disprove it.
	 */
	BOOL bValidRealString = TRUE;

	/* Used to count the number of tokens in the "strRealString".
	 */
	LPSTR strToken = NULL;
	int nTokens = 0;
	LPSTR pChar = NULL;
	
	/* Check if we have a valid argument
	 */
	if( strRealString == NULL )
	{
		bValidRealString = FALSE;
	}

	if( bValidRealString == TRUE )
	{
		/* Make sure we only have ONE token in the string.
		 */
		strToken = strtok( strRealString, " " );
		while( strToken != NULL ) { 
			nTokens++;		
			strToken = strtok( NULL, " " );	
		}

		if( nTokens != 1 )
		{
			bValidRealString = FALSE;
		}
	}


	/* Make sure this token contains only valid characters.
	 * The string argument to atof has the following form:
	 * [whitespace] [sign] [digits] [.digits] [ {d | D | e | E }[sign]digits]
	 * Whitespace consists of space and|or <TAB> characters, which are ignored.
     * Sign is either plus '+' or minus '-'.
     * Digits are one or more decimal digits.
     * Note: If no digits appear before the decimal point, at least one must
     * appear after the decimal point.
     * The decimal digits may be followed by an exponent.
     * An Exponent consists of an introductory letter ( D, d, E, or e) and
	 * an optionally signed decimal integer.
	 */
	pChar = strRealString;
	while( bValidRealString == TRUE && *pChar != '\0' )
	{
		switch( *pChar )
		{
		/* If whitespace...
		 */
		case ' ':
		case '\t':
			if( bWhiteSpaceProcessed ||
				bFirstSignProcessed ||
				bFirstDigitsProcessed ||
				bDecimalPointProcessed ||
				bSecondDigitsProcessed ||
				bExponentProcessed ||
				bSecondSignProcessed ||
				bThirdDigitsProcessed )
			{
				bValidRealString = FALSE;
			}
			break;
		/* If sign...
		 */
		case '+':
		case '-':
			if( bFirstSignProcessed == FALSE )
			{
				if( bFirstDigitsProcessed ||
					bDecimalPointProcessed ||
					bSecondDigitsProcessed ||
					bExponentProcessed ||
					bSecondSignProcessed ||
					bThirdDigitsProcessed )
				{
					bValidRealString = FALSE;
				}
				bWhiteSpaceProcessed = TRUE;
				bFirstSignProcessed = TRUE;
			}
			else if( bSecondSignProcessed == FALSE )
			{
                /* Note: The exponent must be present in
				 * order to accept the second sign...
				 */
				if( bExponentProcessed == FALSE ||
					bThirdDigitsProcessed ||
					bDigitsRequired )
				{
					bValidRealString = FALSE;
				}
				bFirstSignProcessed = TRUE;
				bWhiteSpaceProcessed = TRUE;
				bFirstDigitsProcessed = TRUE;
				bDecimalPointProcessed = TRUE;
				bSecondDigitsProcessed = TRUE;
				bSecondSignProcessed = TRUE;
			}
			break;

		/* If decimals...
		 */
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9': 
			if( bFirstDigitsProcessed == FALSE )
			{
				if( bDecimalPointProcessed ||
					bSecondDigitsProcessed ||
					bExponentProcessed ||
					bSecondSignProcessed ||
					bThirdDigitsProcessed )
				{
					bValidRealString = FALSE;
				}
				bFirstSignProcessed = TRUE;
				bWhiteSpaceProcessed = TRUE;
				/* We have found some digits before the decimal point
				 * so disable the "Digits required" flag.
				 */
				bDigitsRequired = FALSE;
			}
			else if( bSecondDigitsProcessed == FALSE )
			{
				if( bExponentProcessed ||
					bSecondSignProcessed ||
					bThirdDigitsProcessed )
				{
					bValidRealString = FALSE;
				}
				bFirstSignProcessed = TRUE;
				bWhiteSpaceProcessed = TRUE;
				bFirstDigitsProcessed = TRUE;
				bDecimalPointProcessed = TRUE;
				/* We have found some digits after the decimal point
				 * so disable the "Digits required" flag.
				 */
				bDigitsRequired = FALSE;
			}
			else if( bThirdDigitsProcessed == FALSE )
			{
				/* Getting here means everything else should be processed.
                 * If we get anything else than a decimal following this
                 * digit it will be flagged by the other cases, so
				 * we do not really need to do anything in here.
				 */
			}
			break;
		/* If DecimalPoint...
		 */
		case '.': 
			if( bDecimalPointProcessed ||
				bSecondDigitsProcessed ||
				bExponentProcessed ||
				bSecondSignProcessed ||
				bThirdDigitsProcessed )
			{
				bValidRealString = FALSE;
			}
			bFirstSignProcessed = TRUE;
			bWhiteSpaceProcessed = TRUE;
			bFirstDigitsProcessed = TRUE;
			bDecimalPointProcessed = TRUE;
			break;
		/* If Exponent...
		 */
		case 'e':
		case 'E':
		case 'd':
		case 'D':
			if( bExponentProcessed ||
				bSecondSignProcessed ||
				bThirdDigitsProcessed ||
				bDigitsRequired )
			{
				bValidRealString = FALSE;
			}
			bFirstSignProcessed = TRUE;
			bWhiteSpaceProcessed = TRUE;
			bFirstDigitsProcessed = TRUE;
			bDecimalPointProcessed = TRUE;
			bSecondDigitsProcessed = TRUE;
			bExponentProcessed = TRUE;
			break;
		default:
			bValidRealString = FALSE;
			break;
		}
		/* Process next character.
		 */
		pChar++;
	}

	/* If the required digits were not present we have an invalid
	 * string representation of a real number.
	 */
	if( bDigitsRequired == TRUE )
	{
		bValidRealString = FALSE;
	}

	return bValidRealString;
}


/******************************************************************************
 *		Coerce	[INTERNAL]
 *
 * This function dispatches execution to the proper conversion API
 * to do the necessary coercion.
 */
static HRESULT Coerce( VARIANTARG* pd, LCID lcid, ULONG dwFlags, VARIANTARG* ps, VARTYPE vt )
{
	HRESULT res = S_OK;
	unsigned short vtFrom = 0;
	vtFrom = ps->vt & VT_TYPEMASK;
	
    /* Note: Since "long" and "int" values both have 4 bytes and are both signed integers
     * "int" will be treated as "long" in the following code.
     * The same goes for there unsigned versions.
	 */

	switch( vt )
	{

    case( VT_EMPTY ):
        res = VariantClear( pd );
        break;
    case( VT_NULL ):
        res = VariantClear( pd );
        if( res == S_OK )
        {
            pd->vt = VT_NULL;
        }
        break;
	case( VT_I1 ):
		switch( vtFrom )
        {
        case( VT_I1 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_I2 ):
			res = VarI1FromI2( ps->u.iVal, &(pd->u.cVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarI1FromI4( ps->u.lVal, &(pd->u.cVal) );
			break;
		case( VT_UI1 ):
			res = VarI1FromUI1( ps->u.bVal, &(pd->u.cVal) );
			break;
		case( VT_UI2 ):
			res = VarI1FromUI2( ps->u.uiVal, &(pd->u.cVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarI1FromUI4( ps->u.ulVal, &(pd->u.cVal) );
			break;
		case( VT_R4 ):
			res = VarI1FromR4( ps->u.fltVal, &(pd->u.cVal) );
			break;
		case( VT_R8 ):
			res = VarI1FromR8( ps->u.dblVal, &(pd->u.cVal) );
			break;
		case( VT_DATE ):
			res = VarI1FromDate( ps->u.date, &(pd->u.cVal) );
			break;
		case( VT_BOOL ):
			res = VarI1FromBool( ps->u.boolVal, &(pd->u.cVal) );
			break;
		case( VT_BSTR ):
			res = VarI1FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.cVal) );
			break;
		case( VT_CY ):
	     res = VarI1FromCy( ps->u.cyVal, &(pd->u.cVal) );
		case( VT_DISPATCH ):
			/*res = VarI1FromDisp32( ps->u.pdispVal, lcid, &(pd->u.cVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarI1From32( ps->u.lVal, &(pd->u.cVal) );*/
		case( VT_DECIMAL ):
			/*res = VarI1FromDec32( ps->u.decVal, &(pd->u.cVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_I2 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarI2FromI1( ps->u.cVal, &(pd->u.iVal) );
			break;
        case( VT_I2 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarI2FromI4( ps->u.lVal, &(pd->u.iVal) );
			break;
		case( VT_UI1 ):
			res = VarI2FromUI1( ps->u.bVal, &(pd->u.iVal) );
			break;
		case( VT_UI2 ):
			res = VarI2FromUI2( ps->u.uiVal, &(pd->u.iVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarI2FromUI4( ps->u.ulVal, &(pd->u.iVal) );
			break;
		case( VT_R4 ):
			res = VarI2FromR4( ps->u.fltVal, &(pd->u.iVal) );
			break;
		case( VT_R8 ):
			res = VarI2FromR8( ps->u.dblVal, &(pd->u.iVal) );
			break;
		case( VT_DATE ):
			res = VarI2FromDate( ps->u.date, &(pd->u.iVal) );
			break;
		case( VT_BOOL ):
			res = VarI2FromBool( ps->u.boolVal, &(pd->u.iVal) );
			break;
		case( VT_BSTR ):
			res = VarI2FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.iVal) );
			break;
		case( VT_CY ):
	     res = VarI2FromCy( ps->u.cyVal, &(pd->u.iVal) );
		case( VT_DISPATCH ):
			/*res = VarI2FromDisp32( ps->u.pdispVal, lcid, &(pd->u.iVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarI2From32( ps->u.lVal, &(pd->u.iVal) );*/
		case( VT_DECIMAL ):
			/*res = VarI2FromDec32( ps->u.deiVal, &(pd->u.iVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_INT ):
	case( VT_I4 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarI4FromI1( ps->u.cVal, &(pd->u.lVal) );
			break;
		case( VT_I2 ):
			res = VarI4FromI2( ps->u.iVal, &(pd->u.lVal) );
            break;
        case( VT_INT ):
        case( VT_I4 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_UI1 ):
			res = VarI4FromUI1( ps->u.bVal, &(pd->u.lVal) );
			break;
		case( VT_UI2 ):
			res = VarI4FromUI2( ps->u.uiVal, &(pd->u.lVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarI4FromUI4( ps->u.ulVal, &(pd->u.lVal) );
			break;
		case( VT_R4 ):
			res = VarI4FromR4( ps->u.fltVal, &(pd->u.lVal) );
			break;
		case( VT_R8 ):
			res = VarI4FromR8( ps->u.dblVal, &(pd->u.lVal) );
			break;
		case( VT_DATE ):
			res = VarI4FromDate( ps->u.date, &(pd->u.lVal) );
			break;
		case( VT_BOOL ):
			res = VarI4FromBool( ps->u.boolVal, &(pd->u.lVal) );
			break;
		case( VT_BSTR ):
			res = VarI4FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.lVal) );
			break;
		case( VT_CY ):
	     res = VarI4FromCy( ps->u.cyVal, &(pd->u.lVal) );
		case( VT_DISPATCH ):
			/*res = VarI4FromDisp32( ps->u.pdispVal, lcid, &(pd->u.lVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarI4From32( ps->u.lVal, &(pd->u.lVal) );*/
		case( VT_DECIMAL ):
			/*res = VarI4FromDec32( ps->u.deiVal, &(pd->u.lVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_UI1 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarUI1FromI1( ps->u.cVal, &(pd->u.bVal) );
			break;
		case( VT_I2 ):
			res = VarUI1FromI2( ps->u.iVal, &(pd->u.bVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarUI1FromI4( ps->u.lVal, &(pd->u.bVal) );
			break;
        case( VT_UI1 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_UI2 ):
			res = VarUI1FromUI2( ps->u.uiVal, &(pd->u.bVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarUI1FromUI4( ps->u.ulVal, &(pd->u.bVal) );
			break;
		case( VT_R4 ):
			res = VarUI1FromR4( ps->u.fltVal, &(pd->u.bVal) );
			break;
		case( VT_R8 ):
			res = VarUI1FromR8( ps->u.dblVal, &(pd->u.bVal) );
			break;
		case( VT_DATE ):
			res = VarUI1FromDate( ps->u.date, &(pd->u.bVal) );
			break;
		case( VT_BOOL ):
			res = VarUI1FromBool( ps->u.boolVal, &(pd->u.bVal) );
			break;
		case( VT_BSTR ):
			res = VarUI1FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.bVal) );
			break;
		case( VT_CY ):
	     res = VarUI1FromCy( ps->u.cyVal, &(pd->u.bVal) );
		case( VT_DISPATCH ):
			/*res = VarUI1FromDisp32( ps->u.pdispVal, lcid, &(pd->u.bVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarUI1From32( ps->u.lVal, &(pd->u.bVal) );*/
		case( VT_DECIMAL ):
			/*res = VarUI1FromDec32( ps->u.deiVal, &(pd->u.bVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_UI2 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarUI2FromI1( ps->u.cVal, &(pd->u.uiVal) );
			break;
		case( VT_I2 ):
			res = VarUI2FromI2( ps->u.iVal, &(pd->u.uiVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarUI2FromI4( ps->u.lVal, &(pd->u.uiVal) );
			break;
		case( VT_UI1 ):
			res = VarUI2FromUI1( ps->u.bVal, &(pd->u.uiVal) );
			break;
        case( VT_UI2 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarUI2FromUI4( ps->u.ulVal, &(pd->u.uiVal) );
			break;
		case( VT_R4 ):
			res = VarUI2FromR4( ps->u.fltVal, &(pd->u.uiVal) );
			break;
		case( VT_R8 ):
			res = VarUI2FromR8( ps->u.dblVal, &(pd->u.uiVal) );
			break;
		case( VT_DATE ):
			res = VarUI2FromDate( ps->u.date, &(pd->u.uiVal) );
			break;
		case( VT_BOOL ):
			res = VarUI2FromBool( ps->u.boolVal, &(pd->u.uiVal) );
			break;
		case( VT_BSTR ):
			res = VarUI2FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.uiVal) );
			break;
		case( VT_CY ):
	     res = VarUI2FromCy( ps->u.cyVal, &(pd->u.uiVal) );
		case( VT_DISPATCH ):
			/*res = VarUI2FromDisp32( ps->u.pdispVal, lcid, &(pd->u.uiVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarUI2From32( ps->u.lVal, &(pd->u.uiVal) );*/
		case( VT_DECIMAL ):
			/*res = VarUI2FromDec32( ps->u.deiVal, &(pd->u.uiVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_UINT ):
	case( VT_UI4 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarUI4FromI1( ps->u.cVal, &(pd->u.ulVal) );
			break;
		case( VT_I2 ):
			res = VarUI4FromI2( ps->u.iVal, &(pd->u.ulVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarUI4FromI4( ps->u.lVal, &(pd->u.ulVal) );
			break;
		case( VT_UI1 ):
			res = VarUI4FromUI1( ps->u.bVal, &(pd->u.ulVal) );
			break;
		case( VT_UI2 ):
			res = VarUI4FromUI2( ps->u.uiVal, &(pd->u.ulVal) );
			break;
        case( VT_UI4 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_R4 ):
			res = VarUI4FromR4( ps->u.fltVal, &(pd->u.ulVal) );
			break;
		case( VT_R8 ):
			res = VarUI4FromR8( ps->u.dblVal, &(pd->u.ulVal) );
			break;
		case( VT_DATE ):
			res = VarUI4FromDate( ps->u.date, &(pd->u.ulVal) );
			break;
		case( VT_BOOL ):
			res = VarUI4FromBool( ps->u.boolVal, &(pd->u.ulVal) );
			break;
		case( VT_BSTR ):
			res = VarUI4FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.ulVal) );
			break;
		case( VT_CY ):
	     res = VarUI4FromCy( ps->u.cyVal, &(pd->u.ulVal) );
		case( VT_DISPATCH ):
			/*res = VarUI4FromDisp32( ps->u.pdispVal, lcid, &(pd->u.ulVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarUI4From32( ps->u.lVal, &(pd->u.ulVal) );*/
		case( VT_DECIMAL ):
			/*res = VarUI4FromDec32( ps->u.deiVal, &(pd->u.ulVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;
		
	case( VT_R4 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarR4FromI1( ps->u.cVal, &(pd->u.fltVal) );
			break;
		case( VT_I2 ):
			res = VarR4FromI2( ps->u.iVal, &(pd->u.fltVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarR4FromI4( ps->u.lVal, &(pd->u.fltVal) );
			break;
		case( VT_UI1 ):
			res = VarR4FromUI1( ps->u.bVal, &(pd->u.fltVal) );
			break;
		case( VT_UI2 ):
			res = VarR4FromUI2( ps->u.uiVal, &(pd->u.fltVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarR4FromUI4( ps->u.ulVal, &(pd->u.fltVal) );
			break;
        case( VT_R4 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_R8 ):
			res = VarR4FromR8( ps->u.dblVal, &(pd->u.fltVal) );
			break;
		case( VT_DATE ):
			res = VarR4FromDate( ps->u.date, &(pd->u.fltVal) );
			break;
		case( VT_BOOL ):
			res = VarR4FromBool( ps->u.boolVal, &(pd->u.fltVal) );
			break;
		case( VT_BSTR ):
			res = VarR4FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.fltVal) );
			break;
		case( VT_CY ):
	     res = VarR4FromCy( ps->u.cyVal, &(pd->u.fltVal) );
		case( VT_DISPATCH ):
			/*res = VarR4FromDisp32( ps->u.pdispVal, lcid, &(pd->u.fltVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarR4From32( ps->u.lVal, &(pd->u.fltVal) );*/
		case( VT_DECIMAL ):
			/*res = VarR4FromDec32( ps->u.deiVal, &(pd->u.fltVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_R8 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarR8FromI1( ps->u.cVal, &(pd->u.dblVal) );
			break;
		case( VT_I2 ):
			res = VarR8FromI2( ps->u.iVal, &(pd->u.dblVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarR8FromI4( ps->u.lVal, &(pd->u.dblVal) );
			break;
		case( VT_UI1 ):
			res = VarR8FromUI1( ps->u.bVal, &(pd->u.dblVal) );
			break;
		case( VT_UI2 ):
			res = VarR8FromUI2( ps->u.uiVal, &(pd->u.dblVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarR8FromUI4( ps->u.ulVal, &(pd->u.dblVal) );
			break;
		case( VT_R4 ):
			res = VarR8FromR4( ps->u.fltVal, &(pd->u.dblVal) );
			break;
        case( VT_R8 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_DATE ):
			res = VarR8FromDate( ps->u.date, &(pd->u.dblVal) );
			break;
		case( VT_BOOL ):
			res = VarR8FromBool( ps->u.boolVal, &(pd->u.dblVal) );
			break;
		case( VT_BSTR ):
			res = VarR8FromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.dblVal) );
			break;
		case( VT_CY ):
	     res = VarR8FromCy( ps->u.cyVal, &(pd->u.dblVal) );
		case( VT_DISPATCH ):
			/*res = VarR8FromDisp32( ps->u.pdispVal, lcid, &(pd->u.dblVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarR8From32( ps->u.lVal, &(pd->u.dblVal) );*/
		case( VT_DECIMAL ):
			/*res = VarR8FromDec32( ps->u.deiVal, &(pd->u.dblVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_DATE ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarDateFromI1( ps->u.cVal, &(pd->u.date) );
			break;
		case( VT_I2 ):
			res = VarDateFromI2( ps->u.iVal, &(pd->u.date) );
			break;
		case( VT_INT ):
			res = VarDateFromInt( ps->u.intVal, &(pd->u.date) );
			break;
		case( VT_I4 ):
			res = VarDateFromI4( ps->u.lVal, &(pd->u.date) );
			break;
		case( VT_UI1 ):
			res = VarDateFromUI1( ps->u.bVal, &(pd->u.date) );
			break;
		case( VT_UI2 ):
			res = VarDateFromUI2( ps->u.uiVal, &(pd->u.date) );
			break;
		case( VT_UINT ):
			res = VarDateFromUint( ps->u.uintVal, &(pd->u.date) );
			break;
		case( VT_UI4 ):
			res = VarDateFromUI4( ps->u.ulVal, &(pd->u.date) );
			break;
		case( VT_R4 ):
			res = VarDateFromR4( ps->u.fltVal, &(pd->u.date) );
			break;
		case( VT_R8 ):
			res = VarDateFromR8( ps->u.dblVal, &(pd->u.date) );
			break;
        case( VT_DATE ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_BOOL ):
			res = VarDateFromBool( ps->u.boolVal, &(pd->u.date) );
			break;
		case( VT_BSTR ):
			res = VarDateFromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.date) );
			break;
		case( VT_CY ):
	     res = VarDateFromCy( ps->u.cyVal, &(pd->u.date) );
		case( VT_DISPATCH ):
			/*res = VarDateFromDisp32( ps->u.pdispVal, lcid, &(pd->u.date) );*/
		case( VT_UNKNOWN ):
			/*res = VarDateFrom32( ps->u.lVal, &(pd->u.date) );*/
		case( VT_DECIMAL ):
			/*res = VarDateFromDec32( ps->u.deiVal, &(pd->u.date) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_BOOL ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarBoolFromI1( ps->u.cVal, &(pd->u.boolVal) );
			break;
		case( VT_I2 ):
			res = VarBoolFromI2( ps->u.iVal, &(pd->u.boolVal) );
			break;
		case( VT_INT ):
			res = VarBoolFromInt( ps->u.intVal, &(pd->u.boolVal) );
			break;
		case( VT_I4 ):
			res = VarBoolFromI4( ps->u.lVal, &(pd->u.boolVal) );
			break;
		case( VT_UI1 ):
			res = VarBoolFromUI1( ps->u.bVal, &(pd->u.boolVal) );
			break;
		case( VT_UI2 ):
			res = VarBoolFromUI2( ps->u.uiVal, &(pd->u.boolVal) );
			break;
		case( VT_UINT ):
			res = VarBoolFromUint( ps->u.uintVal, &(pd->u.boolVal) );
			break;
		case( VT_UI4 ):
			res = VarBoolFromUI4( ps->u.ulVal, &(pd->u.boolVal) );
			break;
		case( VT_R4 ):
			res = VarBoolFromR4( ps->u.fltVal, &(pd->u.boolVal) );
			break;
		case( VT_R8 ):
			res = VarBoolFromR8( ps->u.dblVal, &(pd->u.boolVal) );
			break;
		case( VT_DATE ):
			res = VarBoolFromDate( ps->u.date, &(pd->u.boolVal) );
			break;
        case( VT_BOOL ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_BSTR ):
			res = VarBoolFromStr( ps->u.bstrVal, lcid, dwFlags, &(pd->u.boolVal) );
			break;
		case( VT_CY ):
	     res = VarBoolFromCy( ps->u.cyVal, &(pd->u.boolVal) );
		case( VT_DISPATCH ):
			/*res = VarBoolFromDisp32( ps->u.pdispVal, lcid, &(pd->u.boolVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarBoolFrom32( ps->u.lVal, &(pd->u.boolVal) );*/
		case( VT_DECIMAL ):
			/*res = VarBoolFromDec32( ps->u.deiVal, &(pd->u.boolVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

	case( VT_BSTR ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarBstrFromI1( ps->u.cVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_I2 ):
			res = VarBstrFromI2( ps->u.iVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_INT ):
			res = VarBstrFromInt( ps->u.intVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_I4 ):
			res = VarBstrFromI4( ps->u.lVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_UI1 ):
			res = VarBstrFromUI1( ps->u.bVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_UI2 ):
			res = VarBstrFromUI2( ps->u.uiVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_UINT ):
			res = VarBstrFromUint( ps->u.uintVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_UI4 ):
			res = VarBstrFromUI4( ps->u.ulVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_R4 ):
			res = VarBstrFromR4( ps->u.fltVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_R8 ):
			res = VarBstrFromR8( ps->u.dblVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_DATE ):
			res = VarBstrFromDate( ps->u.date, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
		case( VT_BOOL ):
			res = VarBstrFromBool( ps->u.boolVal, lcid, dwFlags, &(pd->u.bstrVal) );
			break;
        case( VT_BSTR ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_CY ):
	     /*res = VarBstrFromCy32( ps->u.cyVal, lcid, dwFlags, &(pd->u.bstrVal) );*/
		case( VT_DISPATCH ):
			/*res = VarBstrFromDisp32( ps->u.pdispVal, lcid, lcid, dwFlags, &(pd->u.bstrVal) );*/
		case( VT_UNKNOWN ):
			/*res = VarBstrFrom32( ps->u.lVal, lcid, dwFlags, &(pd->u.bstrVal) );*/
		case( VT_DECIMAL ):
			/*res = VarBstrFromDec32( ps->u.deiVal, lcid, dwFlags, &(pd->u.bstrVal) );*/
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
			break;
		}
		break;

     case( VT_CY ):
	switch( vtFrom )
	  {
	  case( VT_I1 ):
	     res = VarCyFromI1( ps->u.cVal, &(pd->u.cyVal) );
	     break;
	  case( VT_I2 ):
	     res = VarCyFromI2( ps->u.iVal, &(pd->u.cyVal) );
	     break;
	  case( VT_INT ):
	     res = VarCyFromInt( ps->u.intVal, &(pd->u.cyVal) );
	     break;
	  case( VT_I4 ):
	     res = VarCyFromI4( ps->u.lVal, &(pd->u.cyVal) );
	     break;
	  case( VT_UI1 ):
	     res = VarCyFromUI1( ps->u.bVal, &(pd->u.cyVal) );
	     break;
	  case( VT_UI2 ):
	     res = VarCyFromUI2( ps->u.uiVal, &(pd->u.cyVal) );
	     break;
	  case( VT_UINT ):
	     res = VarCyFromUint( ps->u.uintVal, &(pd->u.cyVal) );
	     break;
	  case( VT_UI4 ):
	     res = VarCyFromUI4( ps->u.ulVal, &(pd->u.cyVal) );
	     break;
	  case( VT_R4 ):
	     res = VarCyFromR4( ps->u.fltVal, &(pd->u.cyVal) );
	     break;
	  case( VT_R8 ):
	     res = VarCyFromR8( ps->u.dblVal, &(pd->u.cyVal) );
	     break;
	  case( VT_DATE ):
	     res = VarCyFromDate( ps->u.date, &(pd->u.cyVal) );
	     break;
	  case( VT_BOOL ):
	     res = VarCyFromBool( ps->u.date, &(pd->u.cyVal) );
	     break;
	  case( VT_CY ):
	     res = VariantCopy( pd, ps );
	     break;
	  case( VT_BSTR ):
	     /*res = VarCyFromStr32( ps->u.bstrVal, lcid, dwFlags, &(pd->u.cyVal) );*/
	  case( VT_DISPATCH ):
	     /*res = VarCyFromDisp32( ps->u.pdispVal, lcid, &(pd->u.boolVal) );*/
	  case( VT_UNKNOWN ):
	     /*res = VarCyFrom32( ps->u.lVal, &(pd->u.boolVal) );*/
	  case( VT_DECIMAL ):
	     /*res = VarCyFromDec32( ps->u.deiVal, &(pd->u.boolVal) );*/
	  default:
	     res = DISP_E_TYPEMISMATCH;
	     FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
	     break;
	  }
	break;

	default:
		res = DISP_E_TYPEMISMATCH;
		FIXME(ole,"Coercion from %d to %d\n", vtFrom, vt );
		break;
	}
	
	return res;
}

/******************************************************************************
 *		ValidateVtRange	[INTERNAL]
 *
 * Used internally by the hi-level Variant API to determine
 * if the vartypes are valid.
 */
static HRESULT WINAPI ValidateVtRange( VARTYPE vt )
{
    /* if by value we must make sure it is in the
     * range of the valid types.
     */
    if( ( vt & VT_TYPEMASK ) > VT_MAXVALIDTYPE )
    {
        return DISP_E_BADVARTYPE;
    }
    return S_OK;
}


/******************************************************************************
 *		ValidateVartype	[INTERNAL]
 *
 * Used internally by the hi-level Variant API to determine
 * if the vartypes are valid.
 */
static HRESULT WINAPI ValidateVariantType( VARTYPE vt )
{
	HRESULT res = S_OK;

	/* check if we have a valid argument.
	 */
	if( vt & VT_BYREF )
    {
        /* if by reference check that the type is in
         * the valid range and that it is not of empty or null type
         */
        if( ( vt & VT_TYPEMASK ) == VT_EMPTY ||
            ( vt & VT_TYPEMASK ) == VT_NULL ||
			( vt & VT_TYPEMASK ) > VT_MAXVALIDTYPE )
		{
			res = E_INVALIDARG;
		}
			
    }
    else
    {
        res = ValidateVtRange( vt );
    }
		
	return res;
}

/******************************************************************************
 *		ValidateVt	[INTERNAL]
 *
 * Used internally by the hi-level Variant API to determine
 * if the vartypes are valid.
 */
static HRESULT WINAPI ValidateVt( VARTYPE vt )
{
	HRESULT res = S_OK;

	/* check if we have a valid argument.
	 */
	if( vt & VT_BYREF )
    {
        /* if by reference check that the type is in
         * the valid range and that it is not of empty or null type
         */
        if( ( vt & VT_TYPEMASK ) == VT_EMPTY ||
            ( vt & VT_TYPEMASK ) == VT_NULL ||
			( vt & VT_TYPEMASK ) > VT_MAXVALIDTYPE )
		{
			res = DISP_E_BADVARTYPE;
		}
			
    }
    else
    {
        res = ValidateVtRange( vt );
    }
		
	return res;
}





/******************************************************************************
 *		VariantInit32	[OLEAUT32.8]
 *
 * Initializes the Variant.  Unlike VariantClear it does not interpret the current
 * contents of the Variant.
 */
void WINAPI VariantInit(VARIANTARG* pvarg)
{
  TRACE(ole,"(%p),stub\n",pvarg);

  memset(pvarg, 0, sizeof (VARIANTARG));
  pvarg->vt = VT_EMPTY;

  return;
}

/******************************************************************************
 *		VariantClear32	[OLEAUT32.9]
 *
 * This function clears the VARIANT by setting the vt field to VT_EMPTY. It also
 * sets the wReservedX field to 0.	The current contents of the VARIANT are
 * freed.  If the vt is VT_BSTR the string is freed. If VT_DISPATCH the object is
 * released. If VT_ARRAY the array is freed.
 */
HRESULT WINAPI VariantClear(VARIANTARG* pvarg)
{
  HRESULT res = S_OK;
  TRACE(ole,"(%p)\n",pvarg);

  res = ValidateVariantType( pvarg->vt );
  if( res == S_OK )
  {
    if( !( pvarg->vt & VT_BYREF ) )
    {
      /*
       * The VT_ARRAY flag is a special case of a safe array.
       */
      if ( (pvarg->vt & VT_ARRAY) != 0)
      {
	SafeArrayDestroy(pvarg->u.parray);
      }
      else
      {
	switch( pvarg->vt & VT_TYPEMASK )
	{
	  case( VT_BSTR ):
	    SysFreeString( pvarg->u.bstrVal );
	    break;
	  case( VT_DISPATCH ):
	    break;
	  case( VT_VARIANT ):
	    break;
	  case( VT_UNKNOWN ):
	    break;
	  case( VT_SAFEARRAY ):
	    SafeArrayDestroy(pvarg->u.parray);
	    break;
	  default:
	    break;
	}
      }
    }
	
    /*
     * Empty all the fields and mark the type as empty.
     */
    memset(pvarg, 0, sizeof (VARIANTARG));
    pvarg->vt = VT_EMPTY;
  }

  return res;
}

/******************************************************************************
 *		VariantCopy32	[OLEAUT32.10]
 *
 * Frees up the designation variant and makes a copy of the source.
 */
HRESULT WINAPI VariantCopy(VARIANTARG* pvargDest, VARIANTARG* pvargSrc)
{
  HRESULT res = S_OK;

  TRACE(ole,"(%p, %p)\n", pvargDest, pvargSrc);

  res = ValidateVariantType( pvargSrc->vt );

  /* If the pointer are to the same variant we don't need
   * to do anything.
   */
  if( pvargDest != pvargSrc && res == S_OK )
  {
    res = VariantClear( pvargDest );
		
    if( res == S_OK )
    {
      if( pvargSrc->vt & VT_BYREF )
      {
	/* In the case of byreference we only need
	 * to copy the pointer.
	 */
	pvargDest->u = pvargSrc->u;
	pvargDest->vt = pvargSrc->vt;
      }
      else
      {
	/*
	 * The VT_ARRAY flag is another way to designate a safe array.
	 */
	if (pvargSrc->vt & VT_ARRAY)
	{
	  SafeArrayCopy(pvargSrc->u.parray, &pvargDest->u.parray);
	}
	else
	{
	  /* In the case of by value we need to
	   * copy the actuall value. In the case of
	   * VT_BSTR a copy of the string is made,
	   * if VT_DISPATCH or VT_IUNKNOWN AddReff is
	   * called to increment the object's reference count.
	   */
	  switch( pvargSrc->vt & VT_TYPEMASK )
	  {
	    case( VT_BSTR ):
	      pvargDest->u.bstrVal = SysAllocString( pvargSrc->u.bstrVal );
	      break;
	    case( VT_DISPATCH ):
	      break;
	    case( VT_VARIANT ):
	      break;
	    case( VT_UNKNOWN ):
	      break;
	    case( VT_SAFEARRAY ):
	      SafeArrayCopy(pvargSrc->u.parray, &pvargDest->u.parray);
	      break;
	    default:
	      pvargDest->u = pvargSrc->u;
	      break;
	  }
	}
	
	pvargDest->vt = pvargSrc->vt;
      }      
    }
  }

  return res;
}


/******************************************************************************
 *		VariantCopyInd32	[OLEAUT32.11]
 *
 * Frees up the destination variant and makes a copy of the source.  If
 * the source is of type VT_BYREF it performs the necessary indirections.
 */
HRESULT WINAPI VariantCopyInd(VARIANT* pvargDest, VARIANTARG* pvargSrc)
{
  HRESULT res = S_OK;

  TRACE(ole,"(%p, %p)\n", pvargDest, pvargSrc);

  res = ValidateVariantType( pvargSrc->vt );

  if( res != S_OK )
    return res;
  
  if( pvargSrc->vt & VT_BYREF )
  {
    VARIANTARG varg;
    VariantInit( &varg );

    /* handle the in place copy.
     */
    if( pvargDest == pvargSrc )
    {
      /* we will use a copy of the source instead.
       */
      res = VariantCopy( &varg, pvargSrc );
      pvargSrc = &varg;
    }

    if( res == S_OK )
    {
      res = VariantClear( pvargDest );

      if( res == S_OK )
      {
	/*
	 * The VT_ARRAY flag is another way to designate a safearray variant.
	 */
	if ( pvargSrc->vt & VT_ARRAY)
	{
	  SafeArrayCopy(*pvargSrc->u.pparray, &pvargDest->u.parray);
	}
	else
	{
	  /* In the case of by reference we need
	   * to copy the date pointed to by the variant.
	   */

	  /* Get the variant type.
	   */
	  switch( pvargSrc->vt & VT_TYPEMASK )
	  {
	    case( VT_BSTR ):
	      pvargDest->u.bstrVal = SysAllocString( *(pvargSrc->u.pbstrVal) );
	      break;
	    case( VT_DISPATCH ):
	      break;
	    case( VT_VARIANT ):
	      {
		/* Prevent from cycling.  According to tests on
		 * VariantCopyInd in Windows and the documentation
		 * this API dereferences the inner Variants to only one depth.
		 * If the inner Variant itself contains an
		 * other inner variant the E_INVALIDARG error is
		 * returned. 
		 */
		if( pvargSrc->wReserved1 & PROCESSING_INNER_VARIANT )
		{
		  /* If we get here we are attempting to deference
		   * an inner variant that that is itself contained
		   * in an inner variant so report E_INVALIDARG error.
		   */
		  res = E_INVALIDARG;
		}
		else
		{
		  /* Set the processing inner variant flag.
		   * We will set this flag in the inner variant
		   * that will be passed to the VariantCopyInd function.
		   */
		  (pvargSrc->u.pvarVal)->wReserved1 |= PROCESSING_INNER_VARIANT;
		  
		  /* Dereference the inner variant.
		   */
		  res = VariantCopyInd( pvargDest, pvargSrc->u.pvarVal );
		}
	      }
	      break;
	    case( VT_UNKNOWN ):
	      break;
	    case( VT_SAFEARRAY ):
	      SafeArrayCopy(*pvargSrc->u.pparray, &pvargDest->u.parray);
	      break;
	    default:
	      /* This is a by reference Variant which means that the union
	       * part of the Variant contains a pointer to some data of
	       * type "pvargSrc->vt & VT_TYPEMASK".
	       * We will deference this data in a generic fashion using
	       * the void pointer "Variant.u.byref".
	       * We will copy this data into the union of the destination
	       * Variant.
	       */
	      memcpy( &pvargDest->u, pvargSrc->u.byref, SizeOfVariantData( pvargSrc ) );
	      break;
	  }
	}
	
	pvargDest->vt = pvargSrc->vt & VT_TYPEMASK;
      }
    }

    /* this should not fail.
     */
    VariantClear( &varg );
  }
  else
  {
    res = VariantCopy( pvargDest, pvargSrc );
  }

  return res;
}

/******************************************************************************
 *		VariantChangeType32	[OLEAUT32.12]
 */
HRESULT WINAPI VariantChangeType(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
							USHORT wFlags, VARTYPE vt)
{
	return VariantChangeTypeEx( pvargDest, pvargSrc, 0, wFlags, vt );
}

/******************************************************************************
 *		VariantChangeTypeEx32	[OLEAUT32.147]
 */
HRESULT WINAPI VariantChangeTypeEx(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
							  LCID lcid, USHORT wFlags, VARTYPE vt)
{
	HRESULT res = S_OK;
	VARIANTARG varg;
	VariantInit( &varg );
	
	TRACE(ole,"(%p, %p, %ld, %u, %u),stub\n", pvargDest, pvargSrc, lcid, wFlags, vt);

	/* validate our source argument.
	 */
	res = ValidateVariantType( pvargSrc->vt );

	/* validate the vartype.
	 */
	if( res == S_OK )
	{
		res = ValidateVt( vt );
	}

	/* if we are doing an in-place conversion make a copy of the source.
	 */
	if( res == S_OK && pvargDest == pvargSrc )
	{
		res = VariantCopy( &varg, pvargSrc );
		pvargSrc = &varg;
	}

	if( res == S_OK )
	{
		/* free up the destination variant.
		 */
		res = VariantClear( pvargDest );
	}

	if( res == S_OK )
	{
		if( pvargSrc->vt & VT_BYREF )
		{
			/* Convert the source variant to a "byvalue" variant.
			 */
			VARIANTARG Variant;
			VariantInit( &Variant );
			res = VariantCopyInd( &Variant, pvargSrc );
			if( res == S_OK )
			{
				res = Coerce( pvargDest, lcid, wFlags, &Variant, vt );
				/* this should not fail.
				 */
				VariantClear( &Variant );
			}
	
		}
		else
		{
			/* Use the current "byvalue" source variant.
			 */
			res = Coerce( pvargDest, lcid, wFlags, pvargSrc, vt );
		}
	}
	/* this should not fail.
	 */
	VariantClear( &varg );
	
	return res;
}




/******************************************************************************
 *		VarUI1FromI232		[OLEAUT32.130]
 */
HRESULT WINAPI VarUI1FromI2(short sIn, BYTE* pbOut)
{
	TRACE( ole, "( %d, %p ), stub\n", sIn, pbOut );

	/* Check range of value.
	 */
	if( sIn < UI1_MIN || sIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) sIn;
	
	return S_OK;
}

/******************************************************************************
 *		VarUI1FromI432		[OLEAUT32.131]
 */
HRESULT WINAPI VarUI1FromI4(LONG lIn, BYTE* pbOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, pbOut );

	/* Check range of value.
	 */
	if( lIn < UI1_MIN || lIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) lIn;
	
	return S_OK;
}


/******************************************************************************
 *		VarUI1FromR432		[OLEAUT32.132]
 */
HRESULT WINAPI VarUI1FromR4(FLOAT fltIn, BYTE* pbOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, pbOut );

	/* Check range of value.
     */
    fltIn = round( fltIn );
	if( fltIn < UI1_MIN || fltIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) fltIn;
	
	return S_OK;
}

/******************************************************************************
 *		VarUI1FromR832		[OLEAUT32.133]
 */
HRESULT WINAPI VarUI1FromR8(double dblIn, BYTE* pbOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, pbOut );

	/* Check range of value.
     */
    dblIn = round( dblIn );
	if( dblIn < UI1_MIN || dblIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromDate32		[OLEAUT32.135]
 */
HRESULT WINAPI VarUI1FromDate(DATE dateIn, BYTE* pbOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, pbOut );

	/* Check range of value.
     */
    dateIn = round( dateIn );
	if( dateIn < UI1_MIN || dateIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromBool32		[OLEAUT32.138]
 */
HRESULT WINAPI VarUI1FromBool(VARIANT_BOOL boolIn, BYTE* pbOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, pbOut );

	*pbOut = (BYTE) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromI132		[OLEAUT32.237]
 */
HRESULT WINAPI VarUI1FromI1(CHAR cIn, BYTE* pbOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, pbOut );

	*pbOut = cIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromUI232		[OLEAUT32.238]
 */
HRESULT WINAPI VarUI1FromUI2(USHORT uiIn, BYTE* pbOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pbOut );

	/* Check range of value.
	 */
	if( uiIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromUI432		[OLEAUT32.239]
 */
HRESULT WINAPI VarUI1FromUI4(ULONG ulIn, BYTE* pbOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, pbOut );

	/* Check range of value.
	 */
	if( ulIn > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) ulIn;

	return S_OK;
}


/******************************************************************************
 *		VarUI1FromStr32		[OLEAUT32.54]
 */
HRESULT WINAPI VarUI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, BYTE* pbOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, 0x%08lx, 0x%08lx, %p ), stub\n", strIn, lcid, dwFlags, pbOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
	
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0 , pNewString );

	/* Check range of value.
     */
    dValue = round( dValue );
	if( dValue < UI1_MIN || dValue > UI1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pbOut = (BYTE) dValue;

	return S_OK;
}

/**********************************************************************
 *              VarUI1FromCy32 [OLEAUT32.134]
 * Convert currency to unsigned char
 */
HRESULT WINAPI VarUI1FromCy(CY cyIn, BYTE* pbOut) {
   double t = round((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   if (t > UI1_MAX || t < UI1_MIN) return DISP_E_OVERFLOW;
   
   *pbOut = (BYTE)t;
   return S_OK;
}

/******************************************************************************
 *		VarI2FromUI132		[OLEAUT32.48]
 */
HRESULT WINAPI VarI2FromUI1(BYTE bIn, short* psOut)
{
	TRACE( ole, "( 0x%08x, %p ), stub\n", bIn, psOut );

	*psOut = (short) bIn;
	
	return S_OK;
}

/******************************************************************************
 *		VarI2FromI432		[OLEAUT32.49]
 */
HRESULT WINAPI VarI2FromI4(LONG lIn, short* psOut)
{
	TRACE( ole, "( %lx, %p ), stub\n", lIn, psOut );

	/* Check range of value.
	 */
	if( lIn < I2_MIN || lIn > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short) lIn;
	
	return S_OK;
}

/******************************************************************************
 *		VarI2FromR432		[OLEAUT32.50]
 */
HRESULT WINAPI VarI2FromR4(FLOAT fltIn, short* psOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, psOut );

	/* Check range of value.
     */
    fltIn = round( fltIn );
	if( fltIn < I2_MIN || fltIn > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromR832		[OLEAUT32.51]
 */
HRESULT WINAPI VarI2FromR8(double dblIn, short* psOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, psOut );

	/* Check range of value.
     */
    dblIn = round( dblIn );
	if( dblIn < I2_MIN || dblIn > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromDate32		[OLEAUT32.53]
 */
HRESULT WINAPI VarI2FromDate(DATE dateIn, short* psOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, psOut );

	/* Check range of value.
     */
    dateIn = round( dateIn );
	if( dateIn < I2_MIN || dateIn > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromBool32		[OLEAUT32.56]
 */
HRESULT WINAPI VarI2FromBool(VARIANT_BOOL boolIn, short* psOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, psOut );

	*psOut = (short) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromI132		[OLEAUT32.48]
 */
HRESULT WINAPI VarI2FromI1(CHAR cIn, short* psOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, psOut );

	*psOut = (short) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromUI232		[OLEAUT32.206]
 */
HRESULT WINAPI VarI2FromUI2(USHORT uiIn, short* psOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, psOut );

	/* Check range of value.
	 */
	if( uiIn > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromUI432		[OLEAUT32.49]
 */
HRESULT WINAPI VarI2FromUI4(ULONG ulIn, short* psOut)
{
	TRACE( ole, "( %lx, %p ), stub\n", ulIn, psOut );

	/* Check range of value.
	 */
	if( ulIn < I2_MIN || ulIn > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromStr32		[OLEAUT32.54]
 */
HRESULT WINAPI VarI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, short* psOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, 0x%08lx, 0x%08lx, %p ), stub\n", strIn, lcid, dwFlags, psOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
	
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	/* Check range of value.
     */
    dValue = round( dValue );
	if( dValue < I2_MIN || dValue > I2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*psOut = (short)  dValue;

	return S_OK;
}

/**********************************************************************
 *              VarI2FromCy32 [OLEAUT32.52]
 * Convert currency to signed short
 */
HRESULT WINAPI VarI2FromCy(CY cyIn, short* psOut) {
   double t = round((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   if (t > I2_MAX || t < I2_MIN) return DISP_E_OVERFLOW;
   
   *psOut = (SHORT)t;
   return S_OK;
}

/******************************************************************************
 *		VarI4FromUI132		[OLEAUT32.58]
 */
HRESULT WINAPI VarI4FromUI1(BYTE bIn, LONG* plOut)
{
	TRACE( ole, "( %X, %p ), stub\n", bIn, plOut );

	*plOut = (LONG) bIn;

	return S_OK;
}


/******************************************************************************
 *		VarI4FromR432		[OLEAUT32.60]
 */
HRESULT WINAPI VarI4FromR4(FLOAT fltIn, LONG* plOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, plOut );

	/* Check range of value.
     */
    fltIn = round( fltIn );
	if( fltIn < I4_MIN || fltIn > I4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*plOut = (LONG) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromR832		[OLEAUT32.61]
 */
HRESULT WINAPI VarI4FromR8(double dblIn, LONG* plOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, plOut );

	/* Check range of value.
     */
    dblIn = round( dblIn );
	if( dblIn < I4_MIN || dblIn > I4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*plOut = (LONG) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromDate32		[OLEAUT32.63]
 */
HRESULT WINAPI VarI4FromDate(DATE dateIn, LONG* plOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, plOut );

	/* Check range of value.
     */
    dateIn = round( dateIn );
	if( dateIn < I4_MIN || dateIn > I4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*plOut = (LONG) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromBool32		[OLEAUT32.66]
 */
HRESULT WINAPI VarI4FromBool(VARIANT_BOOL boolIn, LONG* plOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, plOut );

	*plOut = (LONG) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromI132		[OLEAUT32.209]
 */
HRESULT WINAPI VarI4FromI1(CHAR cIn, LONG* plOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, plOut );

	*plOut = (LONG) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromUI232		[OLEAUT32.210]
 */
HRESULT WINAPI VarI4FromUI2(USHORT uiIn, LONG* plOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, plOut );

	*plOut = (LONG) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromUI432		[OLEAUT32.211]
 */
HRESULT WINAPI VarI4FromUI4(ULONG ulIn, LONG* plOut)
{
	TRACE( ole, "( %lx, %p ), stub\n", ulIn, plOut );

	/* Check range of value.
	 */
	if( ulIn < I4_MIN || ulIn > I4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*plOut = (LONG) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromI232		[OLEAUT32.59]
 */
HRESULT WINAPI VarI4FromI2(short sIn, LONG* plOut)
{
	TRACE( ole, "( %d, %p ), stub\n", sIn, plOut );

	*plOut = (LONG) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromStr32		[OLEAUT32.64]
 */
HRESULT WINAPI VarI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, LONG* plOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, 0x%08lx, 0x%08lx, %p ), stub\n", strIn, lcid, dwFlags, plOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
	
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	/* Check range of value.
     */
    dValue = round( dValue );
	if( dValue < I4_MIN || dValue > I4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*plOut = (LONG) dValue;

	return S_OK;
}

/**********************************************************************
 *              VarI4FromCy32 [OLEAUT32.62]
 * Convert currency to signed long
 */
HRESULT WINAPI VarI4FromCy(CY cyIn, LONG* plOut) {
   double t = round((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   if (t > I4_MAX || t < I4_MIN) return DISP_E_OVERFLOW;
   
   *plOut = (LONG)t;
   return S_OK;
}

/******************************************************************************
 *		VarR4FromUI132		[OLEAUT32.68]
 */
HRESULT WINAPI VarR4FromUI1(BYTE bIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %X, %p ), stub\n", bIn, pfltOut );

	*pfltOut = (FLOAT) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromI232		[OLEAUT32.69]
 */
HRESULT WINAPI VarR4FromI2(short sIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %d, %p ), stub\n", sIn, pfltOut );

	*pfltOut = (FLOAT) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromI432		[OLEAUT32.70]
 */
HRESULT WINAPI VarR4FromI4(LONG lIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %lx, %p ), stub\n", lIn, pfltOut );

	*pfltOut = (FLOAT) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromR832		[OLEAUT32.71]
 */
HRESULT WINAPI VarR4FromR8(double dblIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, pfltOut );

	/* Check range of value.
	 */
	if( dblIn < -(FLT_MAX) || dblIn > FLT_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pfltOut = (FLOAT) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromDate32		[OLEAUT32.73]
 */
HRESULT WINAPI VarR4FromDate(DATE dateIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, pfltOut );

	/* Check range of value.
	 */
	if( dateIn < -(FLT_MAX) || dateIn > FLT_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pfltOut = (FLOAT) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromBool32		[OLEAUT32.76]
 */
HRESULT WINAPI VarR4FromBool(VARIANT_BOOL boolIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, pfltOut );

	*pfltOut = (FLOAT) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromI132		[OLEAUT32.213]
 */
HRESULT WINAPI VarR4FromI1(CHAR cIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, pfltOut );

	*pfltOut = (FLOAT) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromUI232		[OLEAUT32.214]
 */
HRESULT WINAPI VarR4FromUI2(USHORT uiIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pfltOut );

	*pfltOut = (FLOAT) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromUI432		[OLEAUT32.215]
 */
HRESULT WINAPI VarR4FromUI4(ULONG ulIn, FLOAT* pfltOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, pfltOut );

	*pfltOut = (FLOAT) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromStr32		[OLEAUT32.74]
 */
HRESULT WINAPI VarR4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, FLOAT* pfltOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, pfltOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
	
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	/* Check range of value.
	 */
	if( dValue < -(FLT_MAX) || dValue > FLT_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pfltOut = (FLOAT) dValue;

	return S_OK;
}

/**********************************************************************
 *              VarR4FromCy32 [OLEAUT32.72]
 * Convert currency to float
 */
HRESULT WINAPI VarR4FromCy(CY cyIn, FLOAT* pfltOut) {
   *pfltOut = (FLOAT)((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   return S_OK;
}

/******************************************************************************
 *		VarR8FromUI132		[OLEAUT32.68]
 */
HRESULT WINAPI VarR8FromUI1(BYTE bIn, double* pdblOut)
{
	TRACE( ole, "( %d, %p ), stub\n", bIn, pdblOut );

	*pdblOut = (double) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromI232		[OLEAUT32.69]
 */
HRESULT WINAPI VarR8FromI2(short sIn, double* pdblOut)
{
	TRACE( ole, "( %d, %p ), stub\n", sIn, pdblOut );

	*pdblOut = (double) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromI432		[OLEAUT32.70]
 */
HRESULT WINAPI VarR8FromI4(LONG lIn, double* pdblOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, pdblOut );

	*pdblOut = (double) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromR432		[OLEAUT32.81]
 */
HRESULT WINAPI VarR8FromR4(FLOAT fltIn, double* pdblOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, pdblOut );

	*pdblOut = (double) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromDate32		[OLEAUT32.83]
 */
HRESULT WINAPI VarR8FromDate(DATE dateIn, double* pdblOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, pdblOut );

	*pdblOut = (double) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromBool32		[OLEAUT32.86]
 */
HRESULT WINAPI VarR8FromBool(VARIANT_BOOL boolIn, double* pdblOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, pdblOut );

	*pdblOut = (double) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromI132		[OLEAUT32.217]
 */
HRESULT WINAPI VarR8FromI1(CHAR cIn, double* pdblOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, pdblOut );

	*pdblOut = (double) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromUI232		[OLEAUT32.218]
 */
HRESULT WINAPI VarR8FromUI2(USHORT uiIn, double* pdblOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pdblOut );

	*pdblOut = (double) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromUI432		[OLEAUT32.219]
 */
HRESULT WINAPI VarR8FromUI4(ULONG ulIn, double* pdblOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, pdblOut );

	*pdblOut = (double) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromStr32		[OLEAUT32.84]
 */
HRESULT WINAPI VarR8FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, double* pdblOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, pdblOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
	
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	*pdblOut = dValue;

	return S_OK;
}

/**********************************************************************
 *              VarR8FromCy32 [OLEAUT32.82]
 * Convert currency to double
 */
HRESULT WINAPI VarR8FromCy(CY cyIn, double* pdblOut) {
   *pdblOut = (double)((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   return S_OK;
}

/******************************************************************************
 *		VarDateFromUI132		[OLEAUT32.]
 */
HRESULT WINAPI VarDateFromUI1(BYTE bIn, DATE* pdateOut)
{
	TRACE( ole, "( %d, %p ), stub\n", bIn, pdateOut );

	*pdateOut = (DATE) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromI232		[OLEAUT32.222]
 */
HRESULT WINAPI VarDateFromI2(short sIn, DATE* pdateOut)
{
	TRACE( ole, "( %d, %p ), stub\n", sIn, pdateOut );

	*pdateOut = (DATE) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromI432		[OLEAUT32.90]
 */
HRESULT WINAPI VarDateFromI4(LONG lIn, DATE* pdateOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, pdateOut );

	if( lIn < DATE_MIN || lIn > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromR432		[OLEAUT32.91]
 */
HRESULT WINAPI VarDateFromR4(FLOAT fltIn, DATE* pdateOut)
{
    TRACE( ole, "( %f, %p ), stub\n", fltIn, pdateOut );

    if( ceil(fltIn) < DATE_MIN || floor(fltIn) > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromR832		[OLEAUT32.92]
 */
HRESULT WINAPI VarDateFromR8(double dblIn, DATE* pdateOut)
{
    TRACE( ole, "( %f, %p ), stub\n", dblIn, pdateOut );

	if( ceil(dblIn) < DATE_MIN || floor(dblIn) > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromStr32		[OLEAUT32.94]
 * The string representing the date is composed of two parts, a date and time.
 *
 * The format of the time is has follows:
 * hh[:mm][:ss][AM|PM]
 * Whitespace can be inserted anywhere between these tokens.  A whitespace consists
 * of space and/or tab characters, which are ignored.
 *
 * The formats for the date part are has follows:
 * mm/[dd/][yy]yy 
 * [dd/]mm/[yy]yy
 * [yy]yy/mm/dd 
 * January dd[,] [yy]yy
 * dd January [yy]yy
 * [yy]yy January dd
 * Whitespace can be inserted anywhere between these tokens.
 *
 * The formats for the date and time string are has follows.
 * date[whitespace][time] 
 * [time][whitespace]date
 *
 * These are the only characters allowed in a string representing a date and time:
 * [A-Z] [a-z] [0-9] ':' '-' '/' ',' ' ' '\t'
 */
HRESULT WINAPI VarDateFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, DATE* pdateOut)
{
	HRESULT ret = S_OK;
    struct tm TM = { 0,0,0,0,0,0,0,0,0 };

    TRACE( ole, "( %p, %lx, %lx, %p ), stub\n", strIn, lcid, dwFlags, pdateOut );

    if( DateTimeStringToTm( strIn, lcid, &TM ) )
    {
        if( TmToDATE( &TM, pdateOut ) == FALSE )
        {
            ret = E_INVALIDARG;
        }
    }
    else
    {
        ret = DISP_E_TYPEMISMATCH;
    }


	return ret;
}

/******************************************************************************
 *		VarDateFromI132		[OLEAUT32.221]
 */
HRESULT WINAPI VarDateFromI1(CHAR cIn, DATE* pdateOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, pdateOut );

	*pdateOut = (DATE) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromUI232		[OLEAUT32.222]
 */
HRESULT WINAPI VarDateFromUI2(USHORT uiIn, DATE* pdateOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pdateOut );

	if( uiIn > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromUI432		[OLEAUT32.223]
 */
HRESULT WINAPI VarDateFromUI4(ULONG ulIn, DATE* pdateOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, pdateOut );

	if( ulIn < DATE_MIN || ulIn > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromBool32		[OLEAUT32.96]
 */
HRESULT WINAPI VarDateFromBool(VARIANT_BOOL boolIn, DATE* pdateOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, pdateOut );

	*pdateOut = (DATE) boolIn;

	return S_OK;
}

/**********************************************************************
 *              VarDateFromCy32 [OLEAUT32.93]
 * Convert currency to date
 */
HRESULT WINAPI VarDateFromCy(CY cyIn, DATE* pdateOut) {
   *pdateOut = (DATE)((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);

   if (*pdateOut > DATE_MAX || *pdateOut < DATE_MIN) return DISP_E_TYPEMISMATCH;
   return S_OK;
}

/******************************************************************************
 *		VarBstrFromUI132		[OLEAUT32.108]
 */
HRESULT WINAPI VarBstrFromUI1(BYTE bVal, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %d, %ld, %ld, %p ), stub\n", bVal, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", bVal );

	*pbstrOut =  StringDupAtoBstr( pBuffer );
	
	return S_OK;
}

/******************************************************************************
 *		VarBstrFromI232		[OLEAUT32.109]
 */
HRESULT WINAPI VarBstrFromI2(short iVal, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %d, %ld, %ld, %p ), stub\n", iVal, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", iVal );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromI432		[OLEAUT32.110]
 */
HRESULT WINAPI VarBstrFromI4(LONG lIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %ld, %ld, %ld, %p ), stub\n", lIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, "%ld", lIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromR432		[OLEAUT32.111]
 */
HRESULT WINAPI VarBstrFromR4(FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %f, %ld, %ld, %p ), stub\n", fltIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, "%.7g", fltIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromR832		[OLEAUT32.112]
 */
HRESULT WINAPI VarBstrFromR8(double dblIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %f, %ld, %ld, %p ), stub\n", dblIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, "%.15g", dblIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromDate32		[OLEAUT32.114]
 *
 * The date is implemented using an 8 byte floating-point number.
 * Days are represented by whole numbers increments starting with 0.00 has
 * being December 30 1899, midnight.
 * The hours are expressed as the fractional part of the number.
 * December 30 1899 at midnight = 0.00
 * January 1 1900 at midnight = 2.00
 * January 4 1900 at 6 AM = 5.25
 * January 4 1900 at noon = 5.50
 * December 29 1899 at midnight = -1.00
 * December 18 1899 at midnight = -12.00
 * December 18 1899 at 6AM = -12.25
 * December 18 1899 at 6PM = -12.75
 * December 19 1899 at midnight = -11.00
 * The tm structure is as follows:
 * struct tm {
 *		  int tm_sec;	   seconds after the minute - [0,59]
 *		  int tm_min;	   minutes after the hour - [0,59]
 *		  int tm_hour;	   hours since midnight - [0,23]
 *		  int tm_mday;	   day of the month - [1,31]
 *		  int tm_mon;	   months since January - [0,11]
 *		  int tm_year;	   years
 *		  int tm_wday;	   days since Sunday - [0,6]
 *		  int tm_yday;	   days since January 1 - [0,365]
 *		  int tm_isdst;    daylight savings time flag
 *		  };
 */
HRESULT WINAPI VarBstrFromDate(DATE dateIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
		struct tm TM = {0,0,0,0,0,0,0,0,0};
	 
    TRACE( ole, "( %f, %ld, %ld, %p ), stub\n", dateIn, lcid, dwFlags, pbstrOut );

    if( DateToTm( dateIn, lcid, &TM ) == FALSE )
			{
        return E_INVALIDARG;
		}

		if( lcid & VAR_DATEVALUEONLY )
			strftime( pBuffer, BUFFER_MAX, "%x", &TM );
		else if( lcid & VAR_TIMEVALUEONLY )
			strftime( pBuffer, BUFFER_MAX, "%X", &TM );
		else
        strftime( pBuffer, BUFFER_MAX, "%x %X", &TM );

		*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromBool32		[OLEAUT32.116]
 */
HRESULT WINAPI VarBstrFromBool(VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %d, %ld, %ld, %p ), stub\n", boolIn, lcid, dwFlags, pbstrOut );

	if( boolIn == VARIANT_FALSE )
	{
		sprintf( pBuffer, "False" );
	}
	else
	{
		sprintf( pBuffer, "True" );
	}

	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromI132		[OLEAUT32.229]
 */
HRESULT WINAPI VarBstrFromI1(CHAR cIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %c, %ld, %ld, %p ), stub\n", cIn, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", cIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromUI232		[OLEAUT32.230]
 */
HRESULT WINAPI VarBstrFromUI2(USHORT uiIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %d, %ld, %ld, %p ), stub\n", uiIn, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", uiIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromUI432		[OLEAUT32.231]
 */
HRESULT WINAPI VarBstrFromUI4(ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE( ole, "( %ld, %ld, %ld, %p ), stub\n", ulIn, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%ld", ulIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromUI132		[OLEAUT32.118]
 */
HRESULT WINAPI VarBoolFromUI1(BYTE bIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %d, %p ), stub\n", bIn, pboolOut );

	if( bIn == 0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromI232		[OLEAUT32.119]
 */
HRESULT WINAPI VarBoolFromI2(short sIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %d, %p ), stub\n", sIn, pboolOut );

	if( sIn == 0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromI432		[OLEAUT32.120]
 */
HRESULT WINAPI VarBoolFromI4(LONG lIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, pboolOut );

	if( lIn == 0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromR432		[OLEAUT32.121]
 */
HRESULT WINAPI VarBoolFromR4(FLOAT fltIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, pboolOut );

	if( fltIn == 0.0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromR832		[OLEAUT32.122]
 */
HRESULT WINAPI VarBoolFromR8(double dblIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, pboolOut );

	if( dblIn == 0.0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromDate32		[OLEAUT32.123]
 */
HRESULT WINAPI VarBoolFromDate(DATE dateIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, pboolOut );

	if( dateIn == 0.0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromStr32		[OLEAUT32.125]
 */
HRESULT WINAPI VarBoolFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL* pboolOut)
{
	HRESULT ret = S_OK;
	char* pNewString = NULL;

	TRACE( ole, "( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, pboolOut );

    pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );

	if( pNewString == NULL || strlen( pNewString ) == 0 )
	{
		ret = DISP_E_TYPEMISMATCH;
	}

	if( ret == S_OK )
	{
		if( strncasecmp( pNewString, "True", strlen( pNewString ) ) == 0 )
		{
			*pboolOut = VARIANT_TRUE;
		}
		else if( strncasecmp( pNewString, "False", strlen( pNewString ) ) == 0 )
		{
			*pboolOut = VARIANT_FALSE;
		}
		else
		{
			/* Try converting the string to a floating point number.
			 */
			double dValue = 0.0;
			HRESULT res = VarR8FromStr( strIn, lcid, dwFlags, &dValue );
			if( res != S_OK )
			{
				ret = DISP_E_TYPEMISMATCH;
			}
			else if( dValue == 0.0 )
			{
				*pboolOut = VARIANT_FALSE;
			}
			else
			{
				*pboolOut = VARIANT_TRUE;
			}
		}
	}

	HeapFree( GetProcessHeap(), 0, pNewString );
	
	return ret;
}

/******************************************************************************
 *		VarBoolFromI132		[OLEAUT32.233]
 */
HRESULT WINAPI VarBoolFromI1(CHAR cIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, pboolOut );

	if( cIn == 0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromUI232		[OLEAUT32.234]
 */
HRESULT WINAPI VarBoolFromUI2(USHORT uiIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pboolOut );

	if( uiIn == 0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromUI432		[OLEAUT32.235]
 */
HRESULT WINAPI VarBoolFromUI4(ULONG ulIn, VARIANT_BOOL* pboolOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, pboolOut );

	if( ulIn == 0 )
	{
		*pboolOut = VARIANT_FALSE;
	}
	else
	{
		*pboolOut = VARIANT_TRUE;
	}

	return S_OK;
}

/**********************************************************************
 *              VarBoolFromCy32 [OLEAUT32.124]
 * Convert currency to boolean
 */
HRESULT WINAPI VarBoolFromCy(CY cyIn, VARIANT_BOOL* pboolOut) {
      if (cyIn.u.Hi || cyIn.u.Lo) *pboolOut = -1;
      else *pboolOut = 0;
      
      return S_OK;
}

/******************************************************************************
 *		VarI1FromUI132		[OLEAUT32.244]
 */
HRESULT WINAPI VarI1FromUI1(BYTE bIn, CHAR* pcOut)
{
	TRACE( ole, "( %d, %p ), stub\n", bIn, pcOut );

	/* Check range of value.
	 */
	if( bIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromI232		[OLEAUT32.245]
 */
HRESULT WINAPI VarI1FromI2(short uiIn, CHAR* pcOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pcOut );

	if( uiIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromI432		[OLEAUT32.246]
 */
HRESULT WINAPI VarI1FromI4(LONG lIn, CHAR* pcOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, pcOut );

	if( lIn < CHAR_MIN || lIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromR432		[OLEAUT32.247]
 */
HRESULT WINAPI VarI1FromR4(FLOAT fltIn, CHAR* pcOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, pcOut );

    fltIn = round( fltIn );
	if( fltIn < CHAR_MIN || fltIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromR832		[OLEAUT32.248]
 */
HRESULT WINAPI VarI1FromR8(double dblIn, CHAR* pcOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, pcOut );

    dblIn = round( dblIn );
    if( dblIn < CHAR_MIN || dblIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromDate32		[OLEAUT32.249]
 */
HRESULT WINAPI VarI1FromDate(DATE dateIn, CHAR* pcOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, pcOut );

    dateIn = round( dateIn );
	if( dateIn < CHAR_MIN || dateIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromStr32		[OLEAUT32.251]
 */
HRESULT WINAPI VarI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, CHAR* pcOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, pcOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
  
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	/* Check range of value.
     */
    dValue = round( dValue );
	if( dValue < CHAR_MIN || dValue > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) dValue;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromBool32		[OLEAUT32.253]
 */
HRESULT WINAPI VarI1FromBool(VARIANT_BOOL boolIn, CHAR* pcOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, pcOut );

	*pcOut = (CHAR) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromUI232		[OLEAUT32.254]
 */
HRESULT WINAPI VarI1FromUI2(USHORT uiIn, CHAR* pcOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pcOut );

	if( uiIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromUI432		[OLEAUT32.255]
 */
HRESULT WINAPI VarI1FromUI4(ULONG ulIn, CHAR* pcOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, pcOut );

	if( ulIn > CHAR_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) ulIn;

	return S_OK;
}

/**********************************************************************
 *              VarI1FromCy32 [OLEAUT32.250]
 * Convert currency to signed char
 */
HRESULT WINAPI VarI1FromCy(CY cyIn, CHAR* pcOut) {
   double t = round((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   if (t > CHAR_MAX || t < CHAR_MIN) return DISP_E_OVERFLOW;
   
   *pcOut = (CHAR)t;
   return S_OK;
}

/******************************************************************************
 *		VarUI2FromUI132		[OLEAUT32.257]
 */
HRESULT WINAPI VarUI2FromUI1(BYTE bIn, USHORT* puiOut)
{
	TRACE( ole, "( %d, %p ), stub\n", bIn, puiOut );

	*puiOut = (USHORT) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromI232		[OLEAUT32.258]
 */
HRESULT WINAPI VarUI2FromI2(short uiIn, USHORT* puiOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, puiOut );

	if( uiIn < UI2_MIN )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromI432		[OLEAUT32.259]
 */
HRESULT WINAPI VarUI2FromI4(LONG lIn, USHORT* puiOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, puiOut );

	if( lIn < UI2_MIN || lIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromR432		[OLEAUT32.260]
 */
HRESULT WINAPI VarUI2FromR4(FLOAT fltIn, USHORT* puiOut)
{
	TRACE( ole, "( %f, %p ), stub\n", fltIn, puiOut );

    fltIn = round( fltIn );
	if( fltIn < UI2_MIN || fltIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromR832		[OLEAUT32.261]
 */
HRESULT WINAPI VarUI2FromR8(double dblIn, USHORT* puiOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, puiOut );

    dblIn = round( dblIn );
    if( dblIn < UI2_MIN || dblIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromDate32		[OLEAUT32.262]
 */
HRESULT WINAPI VarUI2FromDate(DATE dateIn, USHORT* puiOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, puiOut );

    dateIn = round( dateIn );
	if( dateIn < UI2_MIN || dateIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromStr32		[OLEAUT32.264]
 */
HRESULT WINAPI VarUI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, USHORT* puiOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, puiOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
  
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	/* Check range of value.
     */
    dValue = round( dValue );
	if( dValue < UI2_MIN || dValue > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) dValue;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromBool32		[OLEAUT32.266]
 */
HRESULT WINAPI VarUI2FromBool(VARIANT_BOOL boolIn, USHORT* puiOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, puiOut );

	*puiOut = (USHORT) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromI132		[OLEAUT32.267]
 */
HRESULT WINAPI VarUI2FromI1(CHAR cIn, USHORT* puiOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, puiOut );

	*puiOut = (USHORT) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromUI432		[OLEAUT32.268]
 */
HRESULT WINAPI VarUI2FromUI4(ULONG ulIn, USHORT* puiOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", ulIn, puiOut );

	if( ulIn < UI2_MIN || ulIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromStr32		[OLEAUT32.277]
 */
HRESULT WINAPI VarUI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, ULONG* pulOut)
{
	double dValue = 0.0;
	LPSTR pNewString = NULL;

	TRACE( ole, "( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, pulOut );

	/* Check if we have a valid argument
	 */
	pNewString = HEAP_strdupWtoA( GetProcessHeap(), 0, strIn );
	RemoveCharacterFromString( pNewString, "," );
	if( IsValidRealString( pNewString ) == FALSE )
	{
		return DISP_E_TYPEMISMATCH;
	}

	/* Convert the valid string to a floating point number.
	 */
	dValue = atof( pNewString );
  
	/* We don't need the string anymore so free it.
	 */
	HeapFree( GetProcessHeap(), 0, pNewString );

	/* Check range of value.
     */
    dValue = round( dValue );
	if( dValue < UI4_MIN || dValue > UI4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) dValue;

	return S_OK;
}

/**********************************************************************
 *              VarUI2FromCy32 [OLEAUT32.263]
 * Convert currency to unsigned short
 */
HRESULT WINAPI VarUI2FromCy(CY cyIn, USHORT* pusOut) {
   double t = round((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   if (t > UI2_MAX || t < UI2_MIN) return DISP_E_OVERFLOW;
      
   *pusOut = (USHORT)t;
   
   return S_OK;
}

/******************************************************************************
 *		VarUI4FromUI132		[OLEAUT32.270]
 */
HRESULT WINAPI VarUI4FromUI1(BYTE bIn, ULONG* pulOut)
{
	TRACE( ole, "( %d, %p ), stub\n", bIn, pulOut );

	*pulOut = (USHORT) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromI232		[OLEAUT32.271]
 */
HRESULT WINAPI VarUI4FromI2(short uiIn, ULONG* pulOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pulOut );

	if( uiIn < UI4_MIN )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromI432		[OLEAUT32.272]
 */
HRESULT WINAPI VarUI4FromI4(LONG lIn, ULONG* pulOut)
{
	TRACE( ole, "( %ld, %p ), stub\n", lIn, pulOut );

	if( lIn < UI4_MIN )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromR432		[OLEAUT32.273]
 */
HRESULT WINAPI VarUI4FromR4(FLOAT fltIn, ULONG* pulOut)
{
    fltIn = round( fltIn );
    if( fltIn < UI4_MIN || fltIn > UI4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromR832		[OLEAUT32.274]
 */
HRESULT WINAPI VarUI4FromR8(double dblIn, ULONG* pulOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dblIn, pulOut );

    dblIn = round( dblIn );
	if( dblIn < UI4_MIN || dblIn > UI4_MAX )
	{
		return DISP_E_OVERFLOW;
    }

	*pulOut = (ULONG) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromDate32		[OLEAUT32.275]
 */
HRESULT WINAPI VarUI4FromDate(DATE dateIn, ULONG* pulOut)
{
	TRACE( ole, "( %f, %p ), stub\n", dateIn, pulOut );

    dateIn = round( dateIn );
    if( dateIn < UI4_MIN || dateIn > UI4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromBool32		[OLEAUT32.279]
 */
HRESULT WINAPI VarUI4FromBool(VARIANT_BOOL boolIn, ULONG* pulOut)
{
	TRACE( ole, "( %d, %p ), stub\n", boolIn, pulOut );

	*pulOut = (ULONG) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromI132		[OLEAUT32.280]
 */
HRESULT WINAPI VarUI4FromI1(CHAR cIn, ULONG* pulOut)
{
	TRACE( ole, "( %c, %p ), stub\n", cIn, pulOut );

	*pulOut = (ULONG) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromUI232		[OLEAUT32.281]
 */
HRESULT WINAPI VarUI4FromUI2(USHORT uiIn, ULONG* pulOut)
{
	TRACE( ole, "( %d, %p ), stub\n", uiIn, pulOut );

	*pulOut = (ULONG) uiIn;

	return S_OK;
}

/**********************************************************************
 *              VarUI4FromCy32 [OLEAUT32.276]
 * Convert currency to unsigned long
 */
HRESULT WINAPI VarUI4FromCy(CY cyIn, ULONG* pulOut) {
   double t = round((((double)cyIn.u.Hi * 4294967296.0) + (double)cyIn.u.Lo) / 10000);
   
   if (t > UI4_MAX || t < UI4_MIN) return DISP_E_OVERFLOW;

   *pulOut = (ULONG)t;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromUI132 [OLEAUT32.98]
 * Convert unsigned char to currency
 */
HRESULT WINAPI VarCyFromUI1(BYTE bIn, CY* pcyOut) {
   pcyOut->u.Hi = 0;
   pcyOut->u.Lo = ((ULONG)bIn) * 10000;
   
   return S_OK;
}

/**********************************************************************
 *              VarCyFromI232 [OLEAUT32.99]
 * Convert signed short to currency
 */
HRESULT WINAPI VarCyFromI2(short sIn, CY* pcyOut) {
   if (sIn < 0) pcyOut->u.Hi = -1;
   else pcyOut->u.Hi = 0;
   pcyOut->u.Lo = ((ULONG)sIn) * 10000;
   
   return S_OK;
}

/**********************************************************************
 *              VarCyFromI432 [OLEAUT32.100]
 * Convert signed long to currency
 */
HRESULT WINAPI VarCyFromI4(LONG lIn, CY* pcyOut) {
      double t = (double)lIn * (double)10000;
      pcyOut->u.Hi = (LONG)(t / (double)4294967296.0);
      pcyOut->u.Lo = (ULONG)fmod(t, (double)4294967296.0);
      if (lIn < 0) pcyOut->u.Hi--;
   
      return S_OK;
}

/**********************************************************************
 *              VarCyFromR432 [OLEAUT32.101]
 * Convert float to currency
 */
HRESULT WINAPI VarCyFromR4(FLOAT fltIn, CY* pcyOut) {
   double t = round((double)fltIn * (double)10000);
   pcyOut->u.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->u.Lo = (ULONG)fmod(t, (double)4294967296.0);
   if (fltIn < 0) pcyOut->u.Hi--;
   
   return S_OK;
}

/**********************************************************************
 *              VarCyFromR832 [OLEAUT32.102]
 * Convert double to currency
 */
HRESULT WINAPI VarCyFromR8(double dblIn, CY* pcyOut) {
   double t = round(dblIn * (double)10000);
   pcyOut->u.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->u.Lo = (ULONG)fmod(t, (double)4294967296.0);
   if (dblIn < 0) pcyOut->u.Hi--;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromDate32 [OLEAUT32.103]
 * Convert date to currency
 */
HRESULT WINAPI VarCyFromDate(DATE dateIn, CY* pcyOut) {
   double t = round((double)dateIn * (double)10000);
   pcyOut->u.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->u.Lo = (ULONG)fmod(t, (double)4294967296.0);
   if (dateIn < 0) pcyOut->u.Hi--;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromBool32 [OLEAUT32.106]
 * Convert boolean to currency
 */
HRESULT WINAPI VarCyFromBool(VARIANT_BOOL boolIn, CY* pcyOut) {
   if (boolIn < 0) pcyOut->u.Hi = -1;
   else pcyOut->u.Hi = 0;
   pcyOut->u.Lo = (ULONG)boolIn * (ULONG)10000;
   
   return S_OK;
}

/**********************************************************************
 *              VarCyFromI132 [OLEAUT32.225]
 * Convert signed char to currency
 */
HRESULT WINAPI VarCyFromI1(CHAR cIn, CY* pcyOut) {
   if (cIn < 0) pcyOut->u.Hi = -1;
   else pcyOut->u.Hi = 0;
   pcyOut->u.Lo = (ULONG)cIn * (ULONG)10000;
   
   return S_OK;
}

/**********************************************************************
 *              VarCyFromUI232 [OLEAUT32.226]
 * Convert unsigned short to currency
 */
HRESULT WINAPI VarCyFromUI2(USHORT usIn, CY* pcyOut) {
   pcyOut->u.Hi = 0;
   pcyOut->u.Lo = (ULONG)usIn * (ULONG)10000;
   
   return S_OK;
}

/**********************************************************************
 *              VarCyFromUI432 [OLEAUT32.227]
 * Convert unsigned long to currency
 */
HRESULT WINAPI VarCyFromUI4(ULONG ulIn, CY* pcyOut) {
   double t = (double)ulIn * (double)10000;
   pcyOut->u.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->u.Lo = (ULONG)fmod(t, (double)4294967296.0);
      
   return S_OK;
}
