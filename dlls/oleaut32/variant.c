/*
 * VARIANT
 *
 * Copyright 1998 Jean-Claude Cote
 * Copyright 2003 Jon Griffiths
 * The alorithm for conversion from Julian days to day/month/year is based on
 * that devised by Henry Fliegel, as implemented in PostgreSQL, which is
 * Copyright 1994-7 Regents of the University of California
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

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#ifdef HAVE_FLOAT_H
# include <float.h>
#endif

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "oleauto.h"
#include "winreg.h"
#include "heap.h"
#include "wine/debug.h"
#include "wine/unicode.h"
#include "winerror.h"
#include "typelib.h"
#include "winternl.h"
#include "variant.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

const char* wine_vtypes[VT_CLSID] =
{
  "VT_EMPTY","VT_NULL","VT_I2","VT_I4","VT_R4","VT_R8","VT_CY","VT_DATE",
  "VT_BSTR","VT_DISPATCH","VT_ERROR","VT_BOOL","VT_VARIANT","VT_UNKNOWN",
  "VT_DECIMAL","15","VT_I1","VT_UI1","VT_UI2","VT_UI4","VT_I8","VT_UI8",
  "VT_INT","VT_UINT","VT_VOID","VT_HRESULT","VT_PTR","VT_SAFEARRAY",
  "VT_CARRAY","VT_USERDEFINED","VT_LPSTR","VT_LPWSTR""32","33","34","35",
  "VT_RECORD","VT_INT_PTR","VT_UINT_PTR","39","40","41","42","43","44","45",
  "46","47","48","49","50","51","52","53","54","55","56","57","58","59","60",
  "61","62","63","VT_FILETIME","VT_BLOB","VT_STREAM","VT_STORAGE",
  "VT_STREAMED_OBJECT","VT_STORED_OBJECT","VT_BLOB_OBJECT","VT_CF","VT_CLSID"
};

const char* wine_vflags[16] =
{
 "",
 "|VT_VECTOR",
 "|VT_ARRAY",
 "|VT_VECTOR|VT_ARRAY",
 "|VT_BYREF",
 "|VT_VECTOR|VT_ARRAY",
 "|VT_ARRAY|VT_BYREF",
 "|VT_VECTOR|VT_ARRAY|VT_BYREF",
 "|VT_HARDTYPE",
 "|VT_VECTOR|VT_HARDTYPE",
 "|VT_ARRAY|VT_HARDTYPE",
 "|VT_VECTOR|VT_ARRAY|VT_HARDTYPE",
 "|VT_BYREF|VT_HARDTYPE",
 "|VT_VECTOR|VT_ARRAY|VT_HARDTYPE",
 "|VT_ARRAY|VT_BYREF|VT_HARDTYPE",
 "|VT_VECTOR|VT_ARRAY|VT_BYREF|VT_HARDTYPE",
};

#define SYSDUPSTRING(str) SysAllocStringByteLen((LPCSTR)(str), SysStringByteLen(str))

/* the largest valid type
 */
#define VT_MAXVALIDTYPE VT_CLSID

/* This mask is used to set a flag in wReserved1 of
 * the VARIANTARG structure. The flag indicates if
 * the API function is using an inner variant or not.
 */
#define PROCESSING_INNER_VARIANT 0x0001

/* General use buffer.
 */
#define BUFFER_MAX 1024
static char pBuffer[BUFFER_MAX];

/******************************************************************************
 *	   StringDupAtoBstr		[INTERNAL]
 *
 */
static BSTR StringDupAtoBstr( char* strIn )
{
	BSTR bstr = NULL;
	OLECHAR* pNewString = NULL;
	UNICODE_STRING usBuffer;

	RtlCreateUnicodeStringFromAsciiz( &usBuffer, strIn );
	pNewString = usBuffer.Buffer;

	bstr = SysAllocString( pNewString );
	RtlFreeUnicodeString( &usBuffer );
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
 *		Coerce	[INTERNAL]
 *
 * This function dispatches execution to the proper conversion API
 * to do the necessary coercion.
 *
 * FIXME: Passing down dwFlags to the conversion functions is wrong, this
 * 	  is a different flagmask. Check MSDN.
 */
static HRESULT Coerce( VARIANTARG* pd, LCID lcid, ULONG dwFlags, VARIANTARG* ps, VARTYPE vt )
{
	HRESULT res = S_OK;
	unsigned short vtFrom = 0;
	vtFrom = V_VT(ps) & VT_TYPEMASK;


	/* Note: Since "long" and "int" values both have 4 bytes and are
	 * both signed integers "int" will be treated as "long" in the
	 * following code.
	 * The same goes for their unsigned versions.
	 */

	/* Trivial Case: If the coercion is from two types that are
	 * identical then we can blindly copy from one argument to another.*/
	if ((vt==vtFrom))
	   return VariantCopy(pd,ps);

	/* Cases requiring thought*/
	switch( vt )
	{

    case( VT_EMPTY ):
        res = VariantClear( pd );
        break;
    case( VT_NULL ):
        res = VariantClear( pd );
        if( res == S_OK )
        {
            V_VT(pd) = VT_NULL;
        }
        break;
	case( VT_I1 ):
		switch( vtFrom )
        {
		case( VT_I2 ):
			res = VarI1FromI2( V_UNION(ps,iVal), &V_UNION(pd,cVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarI1FromI4( V_UNION(ps,lVal), &V_UNION(pd,cVal) );
			break;
		case( VT_UI1 ):
			res = VarI1FromUI1( V_UNION(ps,bVal), &V_UNION(pd,cVal) );
			break;
		case( VT_UI2 ):
			res = VarI1FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,cVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarI1FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,cVal) );
			break;
		case( VT_R4 ):
			res = VarI1FromR4( V_UNION(ps,fltVal), &V_UNION(pd,cVal) );
			break;
		case( VT_R8 ):
			res = VarI1FromR8( V_UNION(ps,dblVal), &V_UNION(pd,cVal) );
			break;
		case( VT_DATE ):
			res = VarI1FromDate( V_UNION(ps,date), &V_UNION(pd,cVal) );
			break;
		case( VT_BOOL ):
			res = VarI1FromBool( V_UNION(ps,boolVal), &V_UNION(pd,cVal) );
			break;
		case( VT_BSTR ):
			res = VarI1FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,cVal) );
			break;
		case( VT_CY ):
			res = VarI1FromCy( V_UNION(ps,cyVal), &V_UNION(pd,cVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarI1FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,cVal) );*/
		case( VT_DECIMAL ):
			/*res = VarI1FromDec( V_UNION(ps,decVal), &V_UNION(pd,cVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_I1\n", vtFrom );
			break;
		}
		break;

	case( VT_I2 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarI2FromI1( V_UNION(ps,cVal), &V_UNION(pd,iVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarI2FromI4( V_UNION(ps,lVal), &V_UNION(pd,iVal) );
			break;
		case( VT_UI1 ):
			res = VarI2FromUI1( V_UNION(ps,bVal), &V_UNION(pd,iVal) );
			break;
		case( VT_UI2 ):
			res = VarI2FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,iVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarI2FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,iVal) );
			break;
		case( VT_R4 ):
			res = VarI2FromR4( V_UNION(ps,fltVal), &V_UNION(pd,iVal) );
			break;
		case( VT_R8 ):
			res = VarI2FromR8( V_UNION(ps,dblVal), &V_UNION(pd,iVal) );
			break;
		case( VT_DATE ):
			res = VarI2FromDate( V_UNION(ps,date), &V_UNION(pd,iVal) );
			break;
		case( VT_BOOL ):
			res = VarI2FromBool( V_UNION(ps,boolVal), &V_UNION(pd,iVal) );
			break;
		case( VT_BSTR ):
			res = VarI2FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,iVal) );
			break;
		case( VT_CY ):
			res = VarI2FromCy( V_UNION(ps,cyVal), &V_UNION(pd,iVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarI2FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,iVal) );*/
		case( VT_DECIMAL ):
			/*res = VarI2FromDec( V_UNION(ps,deiVal), &V_UNION(pd,iVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_I2\n", vtFrom);
			break;
		}
		break;

	case( VT_INT ):
	case( VT_I4 ):
		switch( vtFrom )
		{
		case( VT_EMPTY ):
		        V_UNION(pd,lVal) = 0;
		        res = S_OK;
		    	break;
		case( VT_I1 ):
			res = VarI4FromI1( V_UNION(ps,cVal), &V_UNION(pd,lVal) );
			break;
		case( VT_I2 ):
			res = VarI4FromI2( V_UNION(ps,iVal), &V_UNION(pd,lVal) );

            		break;
		case( VT_ERROR ):
			V_UNION(pd,lVal) = V_UNION(pd,scode);
			res = S_OK;
			break;
        case( VT_INT ):
        case( VT_I4 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_UI1 ):
			res = VarI4FromUI1( V_UNION(ps,bVal), &V_UNION(pd,lVal) );
			break;
		case( VT_UI2 ):
			res = VarI4FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,lVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarI4FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,lVal) );
			break;
		case( VT_R4 ):
			res = VarI4FromR4( V_UNION(ps,fltVal), &V_UNION(pd,lVal) );
			break;
		case( VT_R8 ):
			res = VarI4FromR8( V_UNION(ps,dblVal), &V_UNION(pd,lVal) );
			break;
		case( VT_DATE ):
			res = VarI4FromDate( V_UNION(ps,date), &V_UNION(pd,lVal) );
			break;
		case( VT_BOOL ):
			res = VarI4FromBool( V_UNION(ps,boolVal), &V_UNION(pd,lVal) );
			break;
		case( VT_BSTR ):
			res = VarI4FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,lVal) );
			break;
		case( VT_CY ):
			res = VarI4FromCy( V_UNION(ps,cyVal), &V_UNION(pd,lVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarI4FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,lVal) );*/
		case( VT_DECIMAL ):
			/*res = VarI4FromDec( V_UNION(ps,deiVal), &V_UNION(pd,lVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_INT/VT_I4\n", vtFrom);
			break;
		}
		break;

	case( VT_UI1 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarUI1FromI1( V_UNION(ps,cVal), &V_UNION(pd,bVal) );
			break;
		case( VT_I2 ):
			res = VarUI1FromI2( V_UNION(ps,iVal), &V_UNION(pd,bVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarUI1FromI4( V_UNION(ps,lVal), &V_UNION(pd,bVal) );
			break;
        case( VT_UI1 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_UI2 ):
			res = VarUI1FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,bVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarUI1FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,bVal) );
			break;
		case( VT_R4 ):
			res = VarUI1FromR4( V_UNION(ps,fltVal), &V_UNION(pd,bVal) );
			break;
		case( VT_R8 ):
			res = VarUI1FromR8( V_UNION(ps,dblVal), &V_UNION(pd,bVal) );
			break;
		case( VT_DATE ):
			res = VarUI1FromDate( V_UNION(ps,date), &V_UNION(pd,bVal) );
			break;
		case( VT_BOOL ):
			res = VarUI1FromBool( V_UNION(ps,boolVal), &V_UNION(pd,bVal) );
			break;
		case( VT_BSTR ):
			res = VarUI1FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,bVal) );
			break;
		case( VT_CY ):
			res = VarUI1FromCy( V_UNION(ps,cyVal), &V_UNION(pd,bVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarUI1FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,bVal) );*/
		case( VT_DECIMAL ):
			/*res = VarUI1FromDec( V_UNION(ps,deiVal), &V_UNION(pd,bVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_UI1\n", vtFrom);
			break;
		}
		break;

	case( VT_UI2 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarUI2FromI1( V_UNION(ps,cVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_I2 ):
			res = VarUI2FromI2( V_UNION(ps,iVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarUI2FromI4( V_UNION(ps,lVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_UI1 ):
			res = VarUI2FromUI1( V_UNION(ps,bVal), &V_UNION(pd,uiVal) );
			break;
        case( VT_UI2 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarUI2FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_R4 ):
			res = VarUI2FromR4( V_UNION(ps,fltVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_R8 ):
			res = VarUI2FromR8( V_UNION(ps,dblVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_DATE ):
			res = VarUI2FromDate( V_UNION(ps,date), &V_UNION(pd,uiVal) );
			break;
		case( VT_BOOL ):
			res = VarUI2FromBool( V_UNION(ps,boolVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_BSTR ):
			res = VarUI2FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,uiVal) );
			break;
		case( VT_CY ):
			res = VarUI2FromCy( V_UNION(ps,cyVal), &V_UNION(pd,uiVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarUI2FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,uiVal) );*/
		case( VT_DECIMAL ):
			/*res = VarUI2FromDec( V_UNION(ps,deiVal), &V_UNION(pd,uiVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_UI2\n", vtFrom);
			break;
		}
		break;

	case( VT_UINT ):
	case( VT_UI4 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarUI4FromI1( V_UNION(ps,cVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_I2 ):
			res = VarUI4FromI2( V_UNION(ps,iVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarUI4FromI4( V_UNION(ps,lVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_UI1 ):
			res = VarUI4FromUI1( V_UNION(ps,bVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_UI2 ):
			res = VarUI4FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,ulVal) );
			break;
        case( VT_UI4 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_R4 ):
			res = VarUI4FromR4( V_UNION(ps,fltVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_R8 ):
			res = VarUI4FromR8( V_UNION(ps,dblVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_DATE ):
			res = VarUI4FromDate( V_UNION(ps,date), &V_UNION(pd,ulVal) );
			break;
		case( VT_BOOL ):
			res = VarUI4FromBool( V_UNION(ps,boolVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_BSTR ):
			res = VarUI4FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,ulVal) );
			break;
		case( VT_CY ):
			res = VarUI4FromCy( V_UNION(ps,cyVal), &V_UNION(pd,ulVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarUI4FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,ulVal) );*/
		case( VT_DECIMAL ):
			/*res = VarUI4FromDec( V_UNION(ps,deiVal), &V_UNION(pd,ulVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_UINT/VT_UI4\n", vtFrom);
			break;
		}
		break;

	case( VT_R4 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarR4FromI1( V_UNION(ps,cVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_I2 ):
			res = VarR4FromI2( V_UNION(ps,iVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarR4FromI4( V_UNION(ps,lVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_UI1 ):
			res = VarR4FromUI1( V_UNION(ps,bVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_UI2 ):
			res = VarR4FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarR4FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_R4 ):
		    res = VariantCopy( pd, ps );
		    break;
		case( VT_R8 ):
			res = VarR4FromR8( V_UNION(ps,dblVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_DATE ):
			res = VarR4FromDate( V_UNION(ps,date), &V_UNION(pd,fltVal) );
			break;
		case( VT_BOOL ):
			res = VarR4FromBool( V_UNION(ps,boolVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_BSTR ):
			res = VarR4FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,fltVal) );
			break;
		case( VT_CY ):
			res = VarR4FromCy( V_UNION(ps,cyVal), &V_UNION(pd,fltVal) );
			break;
		case( VT_ERROR ):
			V_UNION(pd,fltVal) = V_UNION(ps,scode);
			res = S_OK;
			break;
		case( VT_DISPATCH ):
			/*res = VarR4FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,fltVal) );*/
		case( VT_DECIMAL ):
			/*res = VarR4FromDec( V_UNION(ps,deiVal), &V_UNION(pd,fltVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_R4\n", vtFrom);
			break;
		}
		break;

	case( VT_R8 ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarR8FromI1( V_UNION(ps,cVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_I2 ):
			res = VarR8FromI2( V_UNION(ps,iVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_INT ):
		case( VT_I4 ):
			res = VarR8FromI4( V_UNION(ps,lVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_UI1 ):
			res = VarR8FromUI1( V_UNION(ps,bVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_UI2 ):
			res = VarR8FromUI2( V_UNION(ps,uiVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_UINT ):
		case( VT_UI4 ):
			res = VarR8FromUI4( V_UNION(ps,ulVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_R4 ):
			res = VarR8FromR4( V_UNION(ps,fltVal), &V_UNION(pd,dblVal) );
			break;
        case( VT_R8 ):
            res = VariantCopy( pd, ps );
            break;
		case( VT_DATE ):
			res = VarR8FromDate( V_UNION(ps,date), &V_UNION(pd,dblVal) );
			break;
		case( VT_BOOL ):
			res = VarR8FromBool( V_UNION(ps,boolVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_BSTR ):
			res = VarR8FromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,dblVal) );
			break;
		case( VT_CY ):
			res = VarR8FromCy( V_UNION(ps,cyVal), &V_UNION(pd,dblVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarR8FromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,dblVal) );*/
		case( VT_DECIMAL ):
			/*res = VarR8FromDec( V_UNION(ps,deiVal), &V_UNION(pd,dblVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_R8\n", vtFrom);
			break;
		}
		break;

	case( VT_DATE ):
		switch( vtFrom )
		{
		case( VT_I1 ):
			res = VarDateFromI1( V_UNION(ps,cVal), &V_UNION(pd,date) );
			break;
		case( VT_I2 ):
			res = VarDateFromI2( V_UNION(ps,iVal), &V_UNION(pd,date) );
			break;
		case( VT_INT ):
			res = VarDateFromInt( V_UNION(ps,intVal), &V_UNION(pd,date) );
			break;
		case( VT_I4 ):
			res = VarDateFromI4( V_UNION(ps,lVal), &V_UNION(pd,date) );
			break;
		case( VT_UI1 ):
			res = VarDateFromUI1( V_UNION(ps,bVal), &V_UNION(pd,date) );
			break;
		case( VT_UI2 ):
			res = VarDateFromUI2( V_UNION(ps,uiVal), &V_UNION(pd,date) );
			break;
		case( VT_UINT ):
			res = VarDateFromUint( V_UNION(ps,uintVal), &V_UNION(pd,date) );
			break;
		case( VT_UI4 ):
			res = VarDateFromUI4( V_UNION(ps,ulVal), &V_UNION(pd,date) );
			break;
		case( VT_R4 ):
			res = VarDateFromR4( V_UNION(ps,fltVal), &V_UNION(pd,date) );
			break;
		case( VT_R8 ):
			res = VarDateFromR8( V_UNION(ps,dblVal), &V_UNION(pd,date) );
			break;
		case( VT_BOOL ):
			res = VarDateFromBool( V_UNION(ps,boolVal), &V_UNION(pd,date) );
			break;
		case( VT_BSTR ):
			res = VarDateFromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,date) );
			break;
		case( VT_CY ):
			res = VarDateFromCy( V_UNION(ps,cyVal), &V_UNION(pd,date) );
			break;
		case( VT_DISPATCH ):
			/*res = VarDateFromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,date) );*/
		case( VT_DECIMAL ):
			/*res = VarDateFromDec( V_UNION(ps,deiVal), &V_UNION(pd,date) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_DATE\n", vtFrom);
			break;
		}
		break;

	case( VT_BOOL ):
		switch( vtFrom )
		{
		case( VT_NULL ):
		case( VT_EMPTY ):
		    	res = S_OK;
			V_UNION(pd,boolVal) = VARIANT_FALSE;
			break;
		case( VT_I1 ):
			res = VarBoolFromI1( V_UNION(ps,cVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_I2 ):
			res = VarBoolFromI2( V_UNION(ps,iVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_INT ):
			res = VarBoolFromInt( V_UNION(ps,intVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_I4 ):
			res = VarBoolFromI4( V_UNION(ps,lVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_UI1 ):
			res = VarBoolFromUI1( V_UNION(ps,bVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_UI2 ):
			res = VarBoolFromUI2( V_UNION(ps,uiVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_UINT ):
			res = VarBoolFromUint( V_UNION(ps,uintVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_UI4 ):
			res = VarBoolFromUI4( V_UNION(ps,ulVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_R4 ):
			res = VarBoolFromR4( V_UNION(ps,fltVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_R8 ):
			res = VarBoolFromR8( V_UNION(ps,dblVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_DATE ):
			res = VarBoolFromDate( V_UNION(ps,date), &V_UNION(pd,boolVal) );
			break;
		case( VT_BSTR ):
			res = VarBoolFromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,boolVal) );
			break;
		case( VT_CY ):
			res = VarBoolFromCy( V_UNION(ps,cyVal), &V_UNION(pd,boolVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarBoolFromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,boolVal) );*/
		case( VT_DECIMAL ):
			/*res = VarBoolFromDec( V_UNION(ps,deiVal), &V_UNION(pd,boolVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_BOOL\n", vtFrom);
			break;
		}
		break;

	case( VT_BSTR ):
		switch( vtFrom )
		{
		case( VT_EMPTY ):
			if ((V_UNION(pd,bstrVal) = SysAllocStringLen(NULL, 0)))
				res = S_OK;
			else
				res = E_OUTOFMEMORY;
			break;
		case( VT_I1 ):
			res = VarBstrFromI1( V_UNION(ps,cVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_I2 ):
			res = VarBstrFromI2( V_UNION(ps,iVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_INT ):
			res = VarBstrFromInt( V_UNION(ps,intVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_I4 ):
			res = VarBstrFromI4( V_UNION(ps,lVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_UI1 ):
			res = VarBstrFromUI1( V_UNION(ps,bVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_UI2 ):
			res = VarBstrFromUI2( V_UNION(ps,uiVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_UINT ):
			res = VarBstrFromUint( V_UNION(ps,uintVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_UI4 ):
			res = VarBstrFromUI4( V_UNION(ps,ulVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_R4 ):
			res = VarBstrFromR4( V_UNION(ps,fltVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_R8 ):
			res = VarBstrFromR8( V_UNION(ps,dblVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_DATE ):
            if (dwFlags & VARIANT_NOUSEROVERRIDE)
              res = VarBstrFromDate( V_UNION(ps,date), lcid, LOCALE_NOUSEROVERRIDE, &V_UNION(pd,bstrVal) );
            else
			  res = VarBstrFromDate( V_UNION(ps,date), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_BOOL ):
			if (dwFlags & VARIANT_ALPHABOOL)
			  res = VarBstrFromBool(V_BOOL(ps), lcid, 0, &V_BSTR(pd));
			else if (dwFlags & VARIANT_LOCALBOOL)
			  res = VarBstrFromBool(V_BOOL(ps), lcid, VAR_LOCALBOOL, &V_BSTR(pd));
			else
			  res = VarBstrFromI2(V_BOOL(ps), lcid, dwFlags, &V_BSTR(pd));
			break;
		case( VT_BSTR ):
			res = VariantCopy( pd, ps );
			break;
		case( VT_CY ):
			res = VarBstrFromCy( V_UNION(ps,cyVal), lcid, 0, &V_UNION(pd,bstrVal) );
			break;
		case( VT_DISPATCH ):
			/*res = VarBstrFromDisp( V_UNION(ps,pdispVal), lcid, 0, &(pd,bstrVal) );*/
		case( VT_DECIMAL ):
			/*res = VarBstrFromDec( V_UNION(ps,deiVal), lcid, 0, &(pd,bstrVal) );*/
		case( VT_UNKNOWN ):
		default:
			res = DISP_E_TYPEMISMATCH;
			FIXME("Coercion from %d to VT_BSTR\n", vtFrom);
			break;
		}
		break;

     case( VT_CY ):
	switch( vtFrom )
	  {
	  case( VT_I1 ):
	     res = VarCyFromI1( V_UNION(ps,cVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_I2 ):
	     res = VarCyFromI2( V_UNION(ps,iVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_INT ):
	     res = VarCyFromInt( V_UNION(ps,intVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_I4 ):
	     res = VarCyFromI4( V_UNION(ps,lVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_UI1 ):
	     res = VarCyFromUI1( V_UNION(ps,bVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_UI2 ):
	     res = VarCyFromUI2( V_UNION(ps,uiVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_UINT ):
	     res = VarCyFromUint( V_UNION(ps,uintVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_UI4 ):
	     res = VarCyFromUI4( V_UNION(ps,ulVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_R4 ):
	     res = VarCyFromR4( V_UNION(ps,fltVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_R8 ):
	     res = VarCyFromR8( V_UNION(ps,dblVal), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_DATE ):
	     res = VarCyFromDate( V_UNION(ps,date), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_BOOL ):
	     res = VarCyFromBool( V_UNION(ps,date), &V_UNION(pd,cyVal) );
	     break;
	  case( VT_CY ):
	     res = VariantCopy( pd, ps );
	     break;
	  case( VT_BSTR ):
	     res = VarCyFromStr( V_UNION(ps,bstrVal), lcid, 0, &V_UNION(pd,cyVal) );
	     break;
	  case( VT_DISPATCH ):
	     /*res = VarCyFromDisp( V_UNION(ps,pdispVal), lcid, &V_UNION(pd,cyVal) );*/
	  case( VT_DECIMAL ):
	     /*res = VarCyFromDec( V_UNION(ps,deiVal), &V_UNION(pd,cyVal) );*/
	     break;
	  case( VT_UNKNOWN ):
	  default:
	     res = DISP_E_TYPEMISMATCH;
	     FIXME("Coercion from %d to VT_CY\n", vtFrom);
	     break;
	  }
	break;

	case( VT_UNKNOWN ):
	    switch (vtFrom) {
	    case VT_DISPATCH:
		if (V_DISPATCH(ps) == NULL) {
			V_UNKNOWN(pd) = NULL;
		} else {
			res = IDispatch_QueryInterface(V_DISPATCH(ps), &IID_IUnknown, (LPVOID*)&V_UNKNOWN(pd));
		}
		break;
	    case VT_EMPTY: case VT_NULL: case VT_I2: case VT_I4:
	    case VT_R4: case VT_R8: case VT_CY: case VT_DATE:
	    case VT_BSTR: case VT_ERROR: case VT_BOOL:
	    case VT_VARIANT: case VT_DECIMAL: case VT_I1: case VT_UI1:
	    case VT_UI2: case VT_UI4: case VT_I8: case VT_UI8: case VT_INT:
	    case VT_UINT: case VT_VOID: case VT_HRESULT: case VT_PTR:
	    case VT_SAFEARRAY: case VT_CARRAY: case VT_USERDEFINED:
	    case VT_LPSTR: case VT_LPWSTR: case VT_RECORD: case VT_FILETIME:
	    case VT_BLOB: case VT_STREAM: case VT_STORAGE:
	    case VT_STREAMED_OBJECT: case VT_STORED_OBJECT: case VT_BLOB_OBJECT:
	    case VT_CF: case VT_CLSID:
		res = DISP_E_TYPEMISMATCH;
		break;
	    default:
		FIXME("Coercion from %d to VT_UNKNOWN unhandled.\n", vtFrom);
		res = DISP_E_BADVARTYPE;
		break;
	    }
	    break;

	case( VT_DISPATCH ):
	    switch (vtFrom) {
	    case VT_UNKNOWN:
		if (V_UNION(ps,punkVal) == NULL) {
			V_UNION(pd,pdispVal) = NULL;
		} else {
			res = IUnknown_QueryInterface(V_UNION(ps,punkVal), &IID_IDispatch, (LPVOID*)&V_UNION(pd,pdispVal));
		}
		break;
	    case VT_EMPTY: case VT_NULL: case VT_I2: case VT_I4:
	    case VT_R4: case VT_R8: case VT_CY: case VT_DATE:
	    case VT_BSTR: case VT_ERROR: case VT_BOOL:
	    case VT_VARIANT: case VT_DECIMAL: case VT_I1: case VT_UI1:
	    case VT_UI2: case VT_UI4: case VT_I8: case VT_UI8: case VT_INT:
           case VT_UINT: case VT_VOID: case VT_HRESULT:
	    case VT_SAFEARRAY: case VT_CARRAY: case VT_USERDEFINED:
	    case VT_LPSTR: case VT_LPWSTR: case VT_RECORD: case VT_FILETIME:
	    case VT_BLOB: case VT_STREAM: case VT_STORAGE:
	    case VT_STREAMED_OBJECT: case VT_STORED_OBJECT: case VT_BLOB_OBJECT:
	    case VT_CF: case VT_CLSID:
		res = DISP_E_TYPEMISMATCH;
		break;
           case VT_PTR:
               V_UNION(pd,pdispVal) = V_UNION(ps,pdispVal);
               break;
	    default:
		FIXME("Coercion from %d to VT_DISPATCH unhandled.\n", vtFrom);
		res = DISP_E_BADVARTYPE;
		break;
	    }
	    break;

	default:
		res = DISP_E_TYPEMISMATCH;
		FIXME("Coercion from %d to %d\n", vtFrom, vt );
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
static HRESULT ValidateVtRange( VARTYPE vt )
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

/* Copy data from one variant to another. */
static void VARIANT_CopyData(const VARIANT *srcVar, VARTYPE vt, void *pOut)
{
  switch(vt)
  {
  case VT_I1:
  case VT_UI1: memcpy(pOut, &V_UI1(srcVar), sizeof(BYTE)); break;
  case VT_BOOL:
  case VT_I2:
  case VT_UI2: memcpy(pOut, &V_UI2(srcVar), sizeof(SHORT)); break;
  case VT_R4:
  case VT_I4:
  case VT_UI4: memcpy(pOut, &V_UI4(srcVar), sizeof (LONG)); break;
  case VT_R8:
  case VT_DATE:
  case VT_CY:
  case VT_I8:
  case VT_UI8: memcpy(pOut, &V_UI8(srcVar), sizeof (LONG64)); break;
  case VT_DECIMAL: memcpy(pOut, &V_DECIMAL(srcVar), sizeof (DECIMAL)); break;
  default:
    FIXME("VT_ type %d unhandled, please report!\n", vt);
  }
}

/* Coerce VT_DISPATCH to another type */
HRESULT VARIANT_FromDisp(IDispatch* pdispIn, LCID lcid, void* pOut, VARTYPE vt)
{
  VARIANTARG srcVar, dstVar;
  HRESULT hRet;

  V_VT(&srcVar) = VT_DISPATCH;
  V_DISPATCH(&srcVar) = pdispIn;

  hRet = VariantChangeTypeEx(&dstVar, &srcVar, lcid, 0, vt);

  if (SUCCEEDED(hRet))
    VARIANT_CopyData(&dstVar, vt, pOut);
  return hRet;
}

/* Coerce VT_BSTR to a numeric type */
HRESULT VARIANT_NumberFromBstr(OLECHAR* pStrIn, LCID lcid, ULONG ulFlags,
                               void* pOut, VARTYPE vt)
{
  VARIANTARG dstVar;
  HRESULT hRet;
  NUMPARSE np;
  BYTE rgb[1024];

  /* Use VarParseNumFromStr/VarNumFromParseNum as MSDN indicates */
  np.cDig = sizeof(rgb) / sizeof(BYTE);
  np.dwInFlags = NUMPRS_STD;

  hRet = VarParseNumFromStr(pStrIn, lcid, ulFlags, &np, rgb);

  if (SUCCEEDED(hRet))
  {
    /* 1 << vt gives us the VTBIT constant for the destination number type */
    hRet = VarNumFromParseNum(&np, rgb, 1 << vt, &dstVar);
    if (SUCCEEDED(hRet))
      VARIANT_CopyData(&dstVar, vt, pOut);
  }
  return hRet;
}

/******************************************************************************
 *		ValidateVartype	[INTERNAL]
 *
 * Used internally by the hi-level Variant API to determine
 * if the vartypes are valid.
 */
static HRESULT ValidateVariantType( VARTYPE vt )
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
 *		ValidateVt	[INTERNAL]
 *
 * Used internally by the hi-level Variant API to determine
 * if the vartypes are valid.
 */
static HRESULT ValidateVt( VARTYPE vt )
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
 * Check if a variants type is valid.
 */
static inline HRESULT VARIANT_ValidateType(VARTYPE vt)
{
  VARTYPE vtExtra = vt & VT_EXTRA_TYPE;

  vt &= VT_TYPEMASK;

  if (!(vtExtra & (VT_VECTOR|VT_RESERVED)))
  {
    if (vt < VT_VOID || vt == VT_RECORD || vt == VT_CLSID)
    {
      if ((vtExtra & (VT_BYREF|VT_ARRAY)) && vt <= VT_NULL)
        return DISP_E_BADVARTYPE;
      if (vt != (VARTYPE)15)
        return S_OK;
    }
  }
  return DISP_E_BADVARTYPE;
}

/******************************************************************************
 *		VariantInit	[OLEAUT32.8]
 *
 * Initialise a variant.
 *
 * PARAMS
 *  pVarg [O] Variant to initialise
 *
 * RETURNS
 *  Nothing.
 *
 * NOTES
 *  This function simply sets the type of the variant to VT_EMPTY. It does not
 *  free any existing value, use VariantClear() for that.
 */
void WINAPI VariantInit(VARIANTARG* pVarg)
{
  TRACE("(%p)\n", pVarg);

  V_VT(pVarg) = VT_EMPTY; /* Native doesn't set any other fields */
}

/******************************************************************************
 *		VariantClear	[OLEAUT32.9]
 *
 * Clear a variant.
 *
 * PARAMS
 *  pVarg [I/O] Variant to clear
 *
 * RETURNS
 *  Success: S_OK. Any previous value in pVarg is freed and its type is set to VT_EMPTY.
 *  Failure: DISP_E_BADVARTYPE, if the variant is a not a valid variant type.
 */
HRESULT WINAPI VariantClear(VARIANTARG* pVarg)
{
  HRESULT hres = S_OK;

  TRACE("(%p->(%s%s))\n", pVarg, debugstr_VT(pVarg), debugstr_VF(pVarg));

  hres = VARIANT_ValidateType(V_VT(pVarg));

  if (SUCCEEDED(hres))
  {
    if (!V_ISBYREF(pVarg))
    {
      if (V_ISARRAY(pVarg) || V_VT(pVarg) == VT_SAFEARRAY)
      {
        if (V_ARRAY(pVarg))
          hres = SafeArrayDestroy(V_ARRAY(pVarg));
      }
      else if (V_VT(pVarg) == VT_BSTR)
      {
        if (V_BSTR(pVarg))
          SysFreeString(V_BSTR(pVarg));
      }
      else if (V_VT(pVarg) == VT_RECORD)
      {
        struct __tagBRECORD* pBr = &V_UNION(pVarg,brecVal);
        if (pBr->pRecInfo)
        {
          IRecordInfo_RecordClear(pBr->pRecInfo, pBr->pvRecord);
          IRecordInfo_Release(pBr->pRecInfo);
        }
      }
      else if (V_VT(pVarg) == VT_DISPATCH ||
               V_VT(pVarg) == VT_UNKNOWN)
      {
        if (V_UNKNOWN(pVarg))
          IUnknown_Release(V_UNKNOWN(pVarg));
      }
      else if (V_VT(pVarg) == VT_VARIANT)
      {
        if (V_VARIANTREF(pVarg))
          VariantClear(V_VARIANTREF(pVarg));
      }
    }
    V_VT(pVarg) = VT_EMPTY;
  }
  return hres;
}

/******************************************************************************
 * Copy an IRecordInfo object contained in a variant.
 */
static HRESULT VARIANT_CopyIRecordInfo(struct __tagBRECORD* pBr)
{
  HRESULT hres = S_OK;

  if (pBr->pRecInfo)
  {
    ULONG ulSize;

    hres = IRecordInfo_GetSize(pBr->pRecInfo, &ulSize);
    if (SUCCEEDED(hres))
    {
      PVOID pvRecord = HeapAlloc(GetProcessHeap(), 0, ulSize);
      if (!pvRecord)
        hres = E_OUTOFMEMORY;
      else
      {
        memcpy(pvRecord, pBr->pvRecord, ulSize);
        pBr->pvRecord = pvRecord;

        hres = IRecordInfo_RecordCopy(pBr->pRecInfo, pvRecord, pvRecord);
        if (SUCCEEDED(hres))
          IRecordInfo_AddRef(pBr->pRecInfo);
      }
    }
  }
  else if (pBr->pvRecord)
    hres = E_INVALIDARG;
  return hres;
}

/******************************************************************************
 *    VariantCopy  [OLEAUT32.10]
 *
 * Copy a variant.
 *
 * PARAMS
 *  pvargDest [O] Destination for copy
 *  pvargSrc  [I] Source variant to copy
 *
 * RETURNS
 *  Success: S_OK. pvargDest contains a copy of pvargSrc.
 *  Failure: DISP_E_BADVARTYPE, if either variant has an invalid type.
 *           E_OUTOFMEMORY, if memory cannot be allocated. Otherwise an
 *           HRESULT error code from SafeArrayCopy(), IRecordInfo_GetSize(),
 *           or IRecordInfo_RecordCopy(), depending on the type of pvargSrc.
 *
 * NOTES
 *  - If pvargSrc == pvargDest, this function does nothing, and succeeds if
 *    pvargSrc is valid. Otherwise, pvargDest is always cleared using
 *    VariantClear() before pvargSrc is copied to it. If clearing pvargDest
 *    fails, so does this function.
 *  - VT_CLSID is a valid type type for pvargSrc, but not for pvargDest.
 *  - For by-value non-intrinsic types, a deep copy is made, i.e. The whole value
 *    is copied rather than just any pointers to it.
 *  - For by-value object types the object pointer is copied and the objects
 *    reference count increased using IUnknown_AddRef().
 *  - For all by-reference types, only the referencing pointer is copied.
 */
HRESULT WINAPI VariantCopy(VARIANTARG* pvargDest, VARIANTARG* pvargSrc)
{
  HRESULT hres = S_OK;

  TRACE("(%p->(%s%s),%p->(%s%s))\n", pvargDest, debugstr_VT(pvargDest),
        debugstr_VF(pvargDest), pvargSrc, debugstr_VT(pvargSrc),
        debugstr_VF(pvargSrc));

  if (V_TYPE(pvargSrc) == VT_CLSID || /* VT_CLSID is a special case */
      FAILED(VARIANT_ValidateType(V_VT(pvargSrc))))
    return DISP_E_BADVARTYPE;

  if (pvargSrc != pvargDest &&
      SUCCEEDED(hres = VariantClear(pvargDest)))
  {
    *pvargDest = *pvargSrc; /* Shallow copy the value */

    if (!V_ISBYREF(pvargSrc))
    {
      if (V_ISARRAY(pvargSrc))
      {
        if (V_ARRAY(pvargSrc))
          hres = SafeArrayCopy(V_ARRAY(pvargSrc), &V_ARRAY(pvargDest));
      }
      else if (V_VT(pvargSrc) == VT_BSTR)
      {
        if (V_BSTR(pvargSrc))
        {
          V_BSTR(pvargDest) = SysAllocStringLen(V_BSTR(pvargSrc), SysStringLen(V_BSTR(pvargSrc)));
          if (!V_BSTR(pvargDest))
            hres = E_OUTOFMEMORY;
        }
      }
      else if (V_VT(pvargSrc) == VT_RECORD)
      {
        hres = VARIANT_CopyIRecordInfo(&V_UNION(pvargDest,brecVal));
      }
      else if (V_VT(pvargSrc) == VT_DISPATCH ||
               V_VT(pvargSrc) == VT_UNKNOWN)
      {
        if (V_UNKNOWN(pvargSrc))
          IUnknown_AddRef(V_UNKNOWN(pvargSrc));
      }
    }
  }
  return hres;
}

/* Return the byte size of a variants data */
static inline size_t VARIANT_DataSize(const VARIANT* pv)
{
  switch (V_TYPE(pv))
  {
  case VT_I1:
  case VT_UI1:   return sizeof(BYTE); break;
  case VT_I2:
  case VT_UI2:   return sizeof(SHORT); break;
  case VT_INT:
  case VT_UINT:
  case VT_I4:
  case VT_UI4:   return sizeof(LONG); break;
  case VT_I8:
  case VT_UI8:   return sizeof(LONGLONG); break;
  case VT_R4:    return sizeof(float); break;
  case VT_R8:    return sizeof(double); break;
  case VT_DATE:  return sizeof(DATE); break;
  case VT_BOOL:  return sizeof(VARIANT_BOOL); break;
  case VT_DISPATCH:
  case VT_UNKNOWN:
  case VT_BSTR:  return sizeof(void*); break;
  case VT_CY:    return sizeof(CY); break;
  case VT_ERROR: return sizeof(SCODE); break;
  }
  TRACE("Shouldn't be called for vt %s%s!\n", debugstr_VT(pv), debugstr_VF(pv));
  return 0;
}

/******************************************************************************
 *    VariantCopyInd  [OLEAUT32.11]
 *
 * Copy a variant, dereferencing it it is by-reference.
 *
 * PARAMS
 *  pvargDest [O] Destination for copy
 *  pvargSrc  [I] Source variant to copy
 *
 * RETURNS
 *  Success: S_OK. pvargDest contains a copy of pvargSrc.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  Failure: DISP_E_BADVARTYPE, if either variant has an invalid by-value type.
 *           E_INVALIDARG, if pvargSrc  is an invalid by-reference type.
 *           E_OUTOFMEMORY, if memory cannot be allocated. Otherwise an
 *           HRESULT error code from SafeArrayCopy(), IRecordInfo_GetSize(),
 *           or IRecordInfo_RecordCopy(), depending on the type of pvargSrc.
 *
 * NOTES
 *  - If pvargSrc is by-value, this function behaves exactly as VariantCopy().
 *  - If pvargSrc is by-reference, the value copied to pvargDest is the pointed-to
 *    value.
 *  - if pvargSrc == pvargDest, this function dereferences in place. Otherwise,
 *    pvargDest is always cleared using VariantClear() before pvargSrc is copied
 *    to it. If clearing pvargDest fails, so does this function.
 */
HRESULT WINAPI VariantCopyInd(VARIANT* pvargDest, VARIANTARG* pvargSrc)
{
  VARIANTARG vTmp, *pSrc = pvargSrc;
  VARTYPE vt;
  HRESULT hres = S_OK;

  TRACE("(%p->(%s%s),%p->(%s%s))\n", pvargDest, debugstr_VT(pvargDest),
        debugstr_VF(pvargDest), pvargSrc, debugstr_VT(pvargSrc),
        debugstr_VF(pvargSrc));

  if (!V_ISBYREF(pvargSrc))
    return VariantCopy(pvargDest, pvargSrc);

  /* Argument checking is more lax than VariantCopy()... */
  vt = V_TYPE(pvargSrc);
  if (V_ISARRAY(pvargSrc) ||
     (vt > VT_NULL && vt != (VARTYPE)15 && vt < VT_VOID &&
     !(V_VT(pvargSrc) & (VT_VECTOR|VT_RESERVED))))
  {
    /* OK */
  }
  else
    return E_INVALIDARG; /* ...And the return value for invalid types differs too */

  if (pvargSrc == pvargDest)
  {
    /* In place copy. Use a shallow copy of pvargSrc & init pvargDest.
     * This avoids an expensive VariantCopy() call - e.g. SafeArrayCopy().
     */
    vTmp = *pvargSrc;
    pSrc = &vTmp;
    V_VT(pvargDest) = VT_EMPTY;
  }
  else
  {
    /* Copy into another variant. Free the variant in pvargDest */
    if (FAILED(hres = VariantClear(pvargDest)))
      return hres;
  }

  if (V_ISARRAY(pSrc))
  {
    /* Native doesn't check that *V_ARRAYREF(pSrc) is valid */
    hres = SafeArrayCopy(*V_ARRAYREF(pSrc), &V_ARRAY(pvargDest));
  }
  else if (V_VT(pSrc) == (VT_BSTR|VT_BYREF))
  {
    /* Native doesn't check that *V_BSTRREF(pSrc) is valid */
    V_BSTR(pvargDest) = SysAllocStringLen(*V_BSTRREF(pSrc), SysStringLen(*V_BSTRREF(pSrc)));
  }
  else if (V_VT(pSrc) == (VT_RECORD|VT_BYREF))
  {
    V_UNION(pvargDest,brecVal) = V_UNION(pvargSrc,brecVal);
    hres = VARIANT_CopyIRecordInfo(&V_UNION(pvargDest,brecVal));
  }
  else if (V_VT(pSrc) == (VT_DISPATCH|VT_BYREF) ||
           V_VT(pSrc) == (VT_UNKNOWN|VT_BYREF))
  {
    /* Native doesn't check that *V_UNKNOWNREF(pSrc) is valid */
    V_UNKNOWN(pvargDest) = *V_UNKNOWNREF(pSrc);
    if (*V_UNKNOWNREF(pSrc))
      IUnknown_AddRef(*V_UNKNOWNREF(pSrc));
  }
  else if (V_VT(pSrc) == (VT_VARIANT|VT_BYREF))
  {
    /* Native doesn't check that *V_VARIANTREF(pSrc) is valid */
    if (V_VT(V_VARIANTREF(pSrc)) == (VT_VARIANT|VT_BYREF))
      hres = E_INVALIDARG; /* Don't dereference more than one level */
    else
      hres = VariantCopyInd(pvargDest, V_VARIANTREF(pSrc));

    /* Use the dereferenced variants type value, not VT_VARIANT */
    goto VariantCopyInd_Return;
  }
  else if (V_VT(pSrc) == (VT_DECIMAL|VT_BYREF))
  {
    memcpy(&DEC_SCALE(&V_DECIMAL(pvargDest)), &DEC_SCALE(V_DECIMALREF(pSrc)),
           sizeof(DECIMAL) - sizeof(USHORT));
  }
  else
  {
    /* Copy the pointed to data into this variant */
    memcpy(&V_BYREF(pvargDest), V_BYREF(pSrc), VARIANT_DataSize(pSrc));
  }

  V_VT(pvargDest) = V_VT(pSrc) & ~VT_BYREF;

VariantCopyInd_Return:

  if (pSrc != pvargSrc)
    VariantClear(pSrc);

  TRACE("returning 0x%08lx, %p->(%s%s)\n", hres, pvargDest,
        debugstr_VT(pvargDest), debugstr_VF(pvargDest));
  return hres;
}

/******************************************************************************
 * Coerces a full safearray. Not optimal code.
 */
static HRESULT
coerce_array(
	VARIANTARG* src, VARIANTARG *dst, LCID lcid, USHORT wFlags, VARTYPE vt
) {
	SAFEARRAY	*sarr = V_ARRAY(src);
	HRESULT		hres;
	LPVOID		data;
	VARTYPE		vartype;

	SafeArrayGetVartype(sarr,&vartype);
	switch (vt) {
	case VT_BSTR:
		if (sarr->cDims != 1) {
			FIXME("Can not coerce array with dim %d into BSTR\n", sarr->cDims);
			return E_FAIL;
		}
		switch (V_VT(src) & VT_TYPEMASK) {
		case VT_UI1:
			hres = SafeArrayAccessData(sarr, &data);
			if (FAILED(hres)) return hres;

			/* Yes, just memcpied apparently. */
			V_BSTR(dst) = SysAllocStringByteLen(data, sarr->rgsabound[0].cElements);
			hres = SafeArrayUnaccessData(sarr);
			if (FAILED(hres)) return hres;
			break;
		default:
			FIXME("Cannot coerce array of %d into BSTR yet. Please report!\n", V_VT(src) & VT_TYPEMASK);
			return E_FAIL;
		}
		break;
	case VT_SAFEARRAY:
		V_VT(dst) = VT_SAFEARRAY;
		return SafeArrayCopy(sarr, &V_ARRAY(dst));
	default:
		FIXME("Cannot coerce array of vt 0x%x/0x%x into vt 0x%x yet. Please report/implement!\n", vartype, V_VT(src), vt);
		return E_FAIL;
	}
	return S_OK;
}

/******************************************************************************
 *		VariantChangeType	[OLEAUT32.12]
 *
 * Change the type of a variant.
 *  
 * PARAMS
 *  pvargDest [O] Destination for the converted variant
 *  pvargSrc  [O] Source variant to change the type of
 *  wFlags    [I] VARIANT_ flags from "oleauto.h"
 *  vt        [I] Variant type to change pvargSrc into
 *    
 * RETURNS
 *  Success: S_OK. pvargDest contains the converted value.
 *  Failure: An HRESULT error code describing the failure.
 *
 * NOTES
 *  The LCID used for the conversion is LOCALE_USER_DEFAULT.
 *  See VariantChangeTypeEx.
 */
HRESULT WINAPI VariantChangeType(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
							USHORT wFlags, VARTYPE vt)
{
	return VariantChangeTypeEx( pvargDest, pvargSrc, 0, wFlags, vt );
}

/******************************************************************************
 *		VariantChangeTypeEx	[OLEAUT32.147]
 *
 * Change the type of a variant.
 *  
 * PARAMS
 *  pvargDest [O] Destination for the converted variant
 *  pvargSrc  [O] Source variant to change the type of
 *  lcid      [I] LCID for the conversion
 *  wFlags    [I] VARIANT_ flags from "oleauto.h"
 *  vt        [I] Variant type to change pvargSrc into
 *  
 * RETURNS
 *  Success: S_OK. pvargDest contains the converted value.
 *  Failure: An HRESULT error code describing the failure.
 *
 * NOTES
 *  pvargDest and pvargSrc can point to the same variant to perform an in-place
 *  conversion. If the conversion is successful, pvargSrc will be freed.
 */
HRESULT WINAPI VariantChangeTypeEx(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
							  LCID lcid, USHORT wFlags, VARTYPE vt)
{
	HRESULT res = S_OK;
	VARIANTARG varg;
	VariantInit( &varg );

	TRACE("(%p, %p, %ld, %u, %u) vt=%d\n", pvargDest, pvargSrc, lcid, wFlags, vt, V_VT(pvargSrc));
    TRACE("Src Var:\n");
    dump_Variant(pvargSrc);

	/* validate our source argument.
	 */
	res = ValidateVariantType( V_VT(pvargSrc) );

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
		if( V_VT(pvargSrc) & VT_BYREF )
		{
			/* Convert the source variant to a "byvalue" variant.
			 */
			VARIANTARG Variant;

			if ((V_VT(pvargSrc) & 0xf000) != VT_BYREF) {
				FIXME("VT_TYPEMASK %x is unhandled.\n",V_VT(pvargSrc) & VT_TYPEMASK);
				return E_FAIL;
			}

			VariantInit( &Variant );
			res = VariantCopyInd( &Variant, pvargSrc );
			if( res == S_OK )
			{
				res = Coerce( pvargDest, lcid, wFlags, &Variant, vt );
				/* this should not fail.
				 */
				VariantClear( &Variant );
			}
		} else {
			if (V_VT(pvargSrc) & VT_ARRAY) {
				if ((V_VT(pvargSrc) & 0xf000) != VT_ARRAY) {
					FIXME("VT_TYPEMASK %x is unhandled in VT_ARRAY.\n",V_VT(pvargSrc) & VT_TYPEMASK);
					return E_FAIL;
				}
				V_VT(pvargDest) = VT_ARRAY | vt;
				res = coerce_array(pvargSrc, pvargDest, lcid, wFlags, vt);
			} else {
				if ((V_VT(pvargSrc) & 0xf000)) {
					FIXME("VT_TYPEMASK %x is unhandled in normal case.\n",V_VT(pvargSrc) & VT_TYPEMASK);
					return E_FAIL;
				}
				/* Use the current "byvalue" source variant.
				 */
				res = Coerce( pvargDest, lcid, wFlags, pvargSrc, vt );
			}
		}
	}
	/* this should not fail.
	 */
	VariantClear( &varg );

	/* set the type of the destination
	 */
	if ( res == S_OK )
		V_VT(pvargDest) = vt;

    TRACE("Dest Var:\n");
    dump_Variant(pvargDest);

	return res;
}




/******************************************************************************
 *		VarUI1FromI2		[OLEAUT32.130]
 */
HRESULT WINAPI VarUI1FromI2(short sIn, BYTE* pbOut)
{
	TRACE("( %d, %p ), stub\n", sIn, pbOut );

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
 *		VarUI1FromI4		[OLEAUT32.131]
 */
HRESULT WINAPI VarUI1FromI4(LONG lIn, BYTE* pbOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, pbOut );

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
 *		VarUI1FromR4		[OLEAUT32.132]
 */
HRESULT WINAPI VarUI1FromR4(FLOAT fltIn, BYTE* pbOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, pbOut );

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
 *		VarUI1FromR8		[OLEAUT32.133]
 */
HRESULT WINAPI VarUI1FromR8(double dblIn, BYTE* pbOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, pbOut );

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
 *		VarUI1FromDate		[OLEAUT32.135]
 */
HRESULT WINAPI VarUI1FromDate(DATE dateIn, BYTE* pbOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, pbOut );

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
 *		VarUI1FromBool		[OLEAUT32.138]
 */
HRESULT WINAPI VarUI1FromBool(VARIANT_BOOL boolIn, BYTE* pbOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, pbOut );

	*pbOut = (BYTE) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromI1		[OLEAUT32.237]
 */
HRESULT WINAPI VarUI1FromI1(signed char cIn, BYTE* pbOut)
{
	TRACE("( %c, %p ), stub\n", cIn, pbOut );

	*pbOut = cIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI1FromUI2		[OLEAUT32.238]
 */
HRESULT WINAPI VarUI1FromUI2(USHORT uiIn, BYTE* pbOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pbOut );

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
 *		VarUI1FromUI4		[OLEAUT32.239]
 */
HRESULT WINAPI VarUI1FromUI4(ULONG ulIn, BYTE* pbOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, pbOut );

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
 *		VarUI1FromStr		[OLEAUT32.136]
 */
HRESULT WINAPI VarUI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, BYTE* pbOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, pbOut);
  return _VarUI1FromStr(strIn, lcid, dwFlags, pbOut);
}

/**********************************************************************
 *              VarUI1FromCy [OLEAUT32.134]
 * Convert currency to unsigned char
 */
HRESULT WINAPI VarUI1FromCy(CY cyIn, BYTE* pbOut) {
   double t = round((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (t > UI1_MAX || t < UI1_MIN) return DISP_E_OVERFLOW;

   *pbOut = (BYTE)t;
   return S_OK;
}

/******************************************************************************
 *		VarI2FromUI1		[OLEAUT32.48]
 */
HRESULT WINAPI VarI2FromUI1(BYTE bIn, short* psOut)
{
	TRACE("( 0x%08x, %p ), stub\n", bIn, psOut );

	*psOut = (short) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromI4		[OLEAUT32.49]
 */
HRESULT WINAPI VarI2FromI4(LONG lIn, short* psOut)
{
	TRACE("( %lx, %p ), stub\n", lIn, psOut );

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
 *		VarI2FromR4		[OLEAUT32.50]
 */
HRESULT WINAPI VarI2FromR4(FLOAT fltIn, short* psOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, psOut );

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
 *		VarI2FromR8		[OLEAUT32.51]
 */
HRESULT WINAPI VarI2FromR8(double dblIn, short* psOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, psOut );

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
 *		VarI2FromDate		[OLEAUT32.53]
 */
HRESULT WINAPI VarI2FromDate(DATE dateIn, short* psOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, psOut );

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
 *		VarI2FromBool		[OLEAUT32.56]
 */
HRESULT WINAPI VarI2FromBool(VARIANT_BOOL boolIn, short* psOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, psOut );

	*psOut = (short) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromI1		[OLEAUT32.205]
 */
HRESULT WINAPI VarI2FromI1(signed char cIn, short* psOut)
{
	TRACE("( %c, %p ), stub\n", cIn, psOut );

	*psOut = (short) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarI2FromUI2		[OLEAUT32.206]
 */
HRESULT WINAPI VarI2FromUI2(USHORT uiIn, short* psOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, psOut );

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
 *		VarI2FromUI4		[OLEAUT32.207]
 */
HRESULT WINAPI VarI2FromUI4(ULONG ulIn, short* psOut)
{
	TRACE("( %lx, %p ), stub\n", ulIn, psOut );

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
 *		VarI2FromStr		[OLEAUT32.54]
 */
HRESULT WINAPI VarI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, short* psOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, psOut);
  return _VarI2FromStr(strIn, lcid, dwFlags, psOut);
}

/**********************************************************************
 *              VarI2FromCy [OLEAUT32.52]
 * Convert currency to signed short
 */
HRESULT WINAPI VarI2FromCy(CY cyIn, short* psOut) {
   double t = round((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (t > I2_MAX || t < I2_MIN) return DISP_E_OVERFLOW;

   *psOut = (SHORT)t;
   return S_OK;
}

/******************************************************************************
 *		VarI4FromUI1		[OLEAUT32.58]
 */
HRESULT WINAPI VarI4FromUI1(BYTE bIn, LONG* plOut)
{
	TRACE("( %X, %p ), stub\n", bIn, plOut );

	*plOut = (LONG) bIn;

	return S_OK;
}


/******************************************************************************
 *		VarI4FromR4		[OLEAUT32.60]
 */
HRESULT WINAPI VarI4FromR4(FLOAT fltIn, LONG* plOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, plOut );

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
 *		VarI4FromR8		[OLEAUT32.61]
 */
HRESULT WINAPI VarI4FromR8(double dblIn, LONG* plOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, plOut );

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
 *		VarI4FromDate		[OLEAUT32.63]
 */
HRESULT WINAPI VarI4FromDate(DATE dateIn, LONG* plOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, plOut );

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
 *		VarI4FromBool		[OLEAUT32.66]
 */
HRESULT WINAPI VarI4FromBool(VARIANT_BOOL boolIn, LONG* plOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, plOut );

	*plOut = (LONG) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromI1		[OLEAUT32.209]
 */
HRESULT WINAPI VarI4FromI1(signed char cIn, LONG* plOut)
{
	TRACE("( %c, %p ), stub\n", cIn, plOut );

	*plOut = (LONG) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromUI2		[OLEAUT32.210]
 */
HRESULT WINAPI VarI4FromUI2(USHORT uiIn, LONG* plOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, plOut );

	*plOut = (LONG) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromUI4		[OLEAUT32.211]
 */
HRESULT WINAPI VarI4FromUI4(ULONG ulIn, LONG* plOut)
{
	TRACE("( %lx, %p ), stub\n", ulIn, plOut );

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
 *		VarI4FromI2		[OLEAUT32.59]
 */
HRESULT WINAPI VarI4FromI2(short sIn, LONG* plOut)
{
	TRACE("( %d, %p ), stub\n", sIn, plOut );

	*plOut = (LONG) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarI4FromStr		[OLEAUT32.64]
 */
HRESULT WINAPI VarI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, LONG* plOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, plOut);
  return _VarI4FromStr(strIn, lcid, dwFlags, plOut);
}

/**********************************************************************
 *              VarI4FromCy [OLEAUT32.62]
 * Convert currency to signed long
 */
HRESULT WINAPI VarI4FromCy(CY cyIn, LONG* plOut) {
   double t = round((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (t > I4_MAX || t < I4_MIN) return DISP_E_OVERFLOW;

   *plOut = (LONG)t;
   return S_OK;
}

/******************************************************************************
 *		VarR4FromUI1		[OLEAUT32.68]
 */
HRESULT WINAPI VarR4FromUI1(BYTE bIn, FLOAT* pfltOut)
{
	TRACE("( %X, %p ), stub\n", bIn, pfltOut );

	*pfltOut = (FLOAT) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromI2		[OLEAUT32.69]
 */
HRESULT WINAPI VarR4FromI2(short sIn, FLOAT* pfltOut)
{
	TRACE("( %d, %p ), stub\n", sIn, pfltOut );

	*pfltOut = (FLOAT) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromI4		[OLEAUT32.70]
 */
HRESULT WINAPI VarR4FromI4(LONG lIn, FLOAT* pfltOut)
{
	TRACE("( %lx, %p ), stub\n", lIn, pfltOut );

	*pfltOut = (FLOAT) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromR8		[OLEAUT32.71]
 */
HRESULT WINAPI VarR4FromR8(double dblIn, FLOAT* pfltOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, pfltOut );

	/* Check range of value.
	 */
	if( dblIn < -(R4_MAX) || dblIn > R4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pfltOut = (FLOAT) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromDate		[OLEAUT32.73]
 */
HRESULT WINAPI VarR4FromDate(DATE dateIn, FLOAT* pfltOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, pfltOut );

	/* Check range of value.
	 */
	if( dateIn < -(R4_MAX) || dateIn > R4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pfltOut = (FLOAT) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromBool		[OLEAUT32.76]
 */
HRESULT WINAPI VarR4FromBool(VARIANT_BOOL boolIn, FLOAT* pfltOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, pfltOut );

	*pfltOut = (FLOAT) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromI1		[OLEAUT32.213]
 */
HRESULT WINAPI VarR4FromI1(signed char cIn, FLOAT* pfltOut)
{
	TRACE("( %c, %p ), stub\n", cIn, pfltOut );

	*pfltOut = (FLOAT) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromUI2		[OLEAUT32.214]
 */
HRESULT WINAPI VarR4FromUI2(USHORT uiIn, FLOAT* pfltOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pfltOut );

	*pfltOut = (FLOAT) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromUI4		[OLEAUT32.215]
 */
HRESULT WINAPI VarR4FromUI4(ULONG ulIn, FLOAT* pfltOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, pfltOut );

	*pfltOut = (FLOAT) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarR4FromStr		[OLEAUT32.74]
 */
HRESULT WINAPI VarR4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, FLOAT* pfltOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, pfltOut);
  return _VarR4FromStr(strIn, lcid, dwFlags, pfltOut);
}

/**********************************************************************
 *              VarR4FromCy [OLEAUT32.72]
 * Convert currency to float
 */
HRESULT WINAPI VarR4FromCy(CY cyIn, FLOAT* pfltOut) {
   *pfltOut = (FLOAT)((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   return S_OK;
}

/******************************************************************************
 *		VarR8FromUI1		[OLEAUT32.78]
 */
HRESULT WINAPI VarR8FromUI1(BYTE bIn, double* pdblOut)
{
	TRACE("( %d, %p ), stub\n", bIn, pdblOut );

	*pdblOut = (double) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromI2		[OLEAUT32.79]
 */
HRESULT WINAPI VarR8FromI2(short sIn, double* pdblOut)
{
	TRACE("( %d, %p ), stub\n", sIn, pdblOut );

	*pdblOut = (double) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromI4		[OLEAUT32.80]
 */
HRESULT WINAPI VarR8FromI4(LONG lIn, double* pdblOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, pdblOut );

	*pdblOut = (double) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromR4		[OLEAUT32.81]
 */
HRESULT WINAPI VarR8FromR4(FLOAT fltIn, double* pdblOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, pdblOut );

	*pdblOut = (double) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromDate		[OLEAUT32.83]
 */
HRESULT WINAPI VarR8FromDate(DATE dateIn, double* pdblOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, pdblOut );

	*pdblOut = (double) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromBool		[OLEAUT32.86]
 */
HRESULT WINAPI VarR8FromBool(VARIANT_BOOL boolIn, double* pdblOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, pdblOut );

	*pdblOut = (double) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromI1		[OLEAUT32.217]
 */
HRESULT WINAPI VarR8FromI1(signed char cIn, double* pdblOut)
{
	TRACE("( %c, %p ), stub\n", cIn, pdblOut );

	*pdblOut = (double) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromUI2		[OLEAUT32.218]
 */
HRESULT WINAPI VarR8FromUI2(USHORT uiIn, double* pdblOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pdblOut );

	*pdblOut = (double) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromUI4		[OLEAUT32.219]
 */
HRESULT WINAPI VarR8FromUI4(ULONG ulIn, double* pdblOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, pdblOut );

	*pdblOut = (double) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarR8FromStr		[OLEAUT32.84]
 */
HRESULT WINAPI VarR8FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, double* pdblOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, pdblOut);
  return _VarR8FromStr(strIn, lcid, dwFlags, pdblOut);
}

/**********************************************************************
 *              VarR8FromCy [OLEAUT32.82]
 * Convert currency to double
 */
HRESULT WINAPI VarR8FromCy(CY cyIn, double* pdblOut) {
   *pdblOut = (double)((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);
   TRACE("%lu %ld -> %f\n", cyIn.s.Hi, cyIn.s.Lo, *pdblOut);
   return S_OK;
}

/******************************************************************************
 *		VarDateFromUI1		[OLEAUT32.88]
 */
HRESULT WINAPI VarDateFromUI1(BYTE bIn, DATE* pdateOut)
{
	TRACE("( %d, %p ), stub\n", bIn, pdateOut );

	*pdateOut = (DATE) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromI2		[OLEAUT32.89]
 */
HRESULT WINAPI VarDateFromI2(short sIn, DATE* pdateOut)
{
	TRACE("( %d, %p ), stub\n", sIn, pdateOut );

	*pdateOut = (DATE) sIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromI4		[OLEAUT32.90]
 */
HRESULT WINAPI VarDateFromI4(LONG lIn, DATE* pdateOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, pdateOut );

	if( lIn < DATE_MIN || lIn > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromR4		[OLEAUT32.91]
 */
HRESULT WINAPI VarDateFromR4(FLOAT fltIn, DATE* pdateOut)
{
    TRACE("( %f, %p ), stub\n", fltIn, pdateOut );

    if( ceil(fltIn) < DATE_MIN || floor(fltIn) > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromR8		[OLEAUT32.92]
 */
HRESULT WINAPI VarDateFromR8(double dblIn, DATE* pdateOut)
{
    TRACE("( %f, %p ), stub\n", dblIn, pdateOut );

	if( ceil(dblIn) < DATE_MIN || floor(dblIn) > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) dblIn;

	return S_OK;
}

/* Date string parsing */
#define DP_TIMESEP 0x01 /* Time seperator ( _must_ remain 0x1, used as a bitmask) */
#define DP_DATESEP 0x02 /* Date seperator */
#define DP_MONTH   0x04 /* Month name */
#define DP_AM      0x08 /* AM */
#define DP_PM      0x10 /* PM */

typedef struct tagDATEPARSE
{
    DWORD dwCount;      /* Number of fields found so far (maximum 6) */
    DWORD dwParseFlags; /* Global parse flags (DP_ Flags above) */
    DWORD dwFlags[6];   /* Flags for each field */
    DWORD dwValues[6];  /* Value of each field */
} DATEPARSE;

#define TIMEFLAG(i) ((dp.dwFlags[i] & DP_TIMESEP) << i)

#define IsLeapYear(y) (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))

/* Determine if a day is valid in a given month of a given year */
static BOOL VARIANT_IsValidMonthDay(DWORD day, DWORD month, DWORD year)
{
  static const BYTE days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  if (day && month && month < 13)
  {
    if (day <= days[month] || (month == 2 && day == 29 && IsLeapYear(year)))
      return TRUE;
  }
  return FALSE;
}

/* Possible orders for 3 numbers making up a date */
#define ORDER_MDY 0x01
#define ORDER_YMD 0x02
#define ORDER_YDM 0x04
#define ORDER_DMY 0x08
#define ORDER_MYD 0x10 /* Synthetic order, used only for funky 2 digit dates */

/* Determine a date for a particular locale, from 3 numbers */
static inline HRESULT VARIANT_MakeDate(DATEPARSE *dp, DWORD iDate,
                                       DWORD offset, SYSTEMTIME *st)
{
  DWORD dwAllOrders, dwTry, dwCount = 0, v1, v2, v3;

  if (!dp->dwCount)
  {
    v1 = 30; /* Default to (Variant) 0 date part */
    v2 = 12;
    v3 = 1899;
    goto VARIANT_MakeDate_OK;
  }

  v1 = dp->dwValues[offset + 0];
  v2 = dp->dwValues[offset + 1];
  if (dp->dwCount == 2)
  {
    SYSTEMTIME current;
    GetSystemTime(&current);
    v3 = current.wYear;
  }
  else
    v3 = dp->dwValues[offset + 2];

  TRACE("(%ld,%ld,%ld,%ld,%ld)\n", v1, v2, v3, iDate, offset);

  /* If one number must be a month (Because a month name was given), then only
   * consider orders with the month in that position.
   * If we took the current year as 'v3', then only allow a year in that position.
   */
  if (dp->dwFlags[offset + 0] & DP_MONTH)
  {
    dwAllOrders = ORDER_MDY;
  }
  else if (dp->dwFlags[offset + 1] & DP_MONTH)
  {
    dwAllOrders = ORDER_DMY;
    if (dp->dwCount > 2)
      dwAllOrders |= ORDER_YMD;
  }
  else if (dp->dwCount > 2 && dp->dwFlags[offset + 2] & DP_MONTH)
  {
    dwAllOrders = ORDER_YDM;
  }
  else
  {
    dwAllOrders = ORDER_MDY|ORDER_DMY;
    if (dp->dwCount > 2)
      dwAllOrders |= (ORDER_YMD|ORDER_YDM);
  }

VARIANT_MakeDate_Start:
  TRACE("dwAllOrders is 0x%08lx\n", dwAllOrders);

  while (dwAllOrders)
  {
    DWORD dwTemp;

    if (dwCount == 0)
    {
      /* First: Try the order given by iDate */
      switch (iDate)
      {
      case 0:  dwTry = dwAllOrders & ORDER_MDY; break;
      case 1:  dwTry = dwAllOrders & ORDER_DMY; break;
      default: dwTry = dwAllOrders & ORDER_YMD; break;
      }
    }
    else if (dwCount == 1)
    {
      /* Second: Try all the orders compatable with iDate */
      switch (iDate)
      {
      case 0:  dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
      case 1:  dwTry = dwAllOrders & ~(ORDER_MDY|ORDER_YMD|ORDER_MYD); break;
      default: dwTry = dwAllOrders & ~(ORDER_DMY|ORDER_YDM); break;
      }
    }
    else
    {
      /* Finally: Try any remaining orders */
      dwTry = dwAllOrders;
    }

    TRACE("Attempt %ld, dwTry is 0x%08lx\n", dwCount, dwTry);

    dwCount++;
    if (!dwTry)
      continue;

#define DATE_SWAP(x,y) do { dwTemp = x; x = y; y = dwTemp; } while (0)

    if (dwTry & ORDER_MDY)
    {
      if (VARIANT_IsValidMonthDay(v2,v1,v3))
      {
        DATE_SWAP(v1,v2);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_MDY;
    }
    if (dwTry & ORDER_YMD)
    {
      if (VARIANT_IsValidMonthDay(v3,v2,v1))
      {
        DATE_SWAP(v1,v3);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_YMD;
    }
    if (dwTry & ORDER_YDM)
    {
      if (VARIANT_IsValidMonthDay(v2,v3,v1))
      {
        DATE_SWAP(v1,v2);
        DATE_SWAP(v2,v3);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_YDM;
    }
    if (dwTry & ORDER_DMY)
    {
      if (VARIANT_IsValidMonthDay(v1,v2,v3))
        goto VARIANT_MakeDate_OK;
      dwAllOrders &= ~ORDER_DMY;
    }
    if (dwTry & ORDER_MYD)
    {
      /* Only occurs if we are trying a 2 year date as M/Y not D/M */
      if (VARIANT_IsValidMonthDay(v3,v1,v2))
      {
        DATE_SWAP(v1,v3);
        DATE_SWAP(v2,v3);
        goto VARIANT_MakeDate_OK;
      }
      dwAllOrders &= ~ORDER_MYD;
    }
  }

  if (dp->dwCount == 2)
  {
    /* We couldn't make a date as D/M or M/D, so try M/Y or Y/M */
    v3 = 1; /* 1st of the month */
    dwAllOrders = ORDER_YMD|ORDER_MYD;
    dp->dwCount = 0; /* Don't return to this code path again */
    dwCount = 0;
    goto VARIANT_MakeDate_Start;
  }

  /* No valid dates were able to be constructed */
  return DISP_E_TYPEMISMATCH;

VARIANT_MakeDate_OK:

  /* Check that the time part is ok */
  if (st->wHour > 23 || st->wMinute > 59 || st->wSecond > 59)
    return DISP_E_TYPEMISMATCH;

  TRACE("Time %d %d %d\n", st->wHour, st->wMinute, st->wSecond);
  if (st->wHour < 12 && (dp->dwParseFlags & DP_PM))
    st->wHour += 12;
  else if (st->wHour == 12 && (dp->dwParseFlags & DP_AM))
    st->wHour = 0;
  TRACE("Time %d %d %d\n", st->wHour, st->wMinute, st->wSecond);

  st->wDay = v1;
  st->wMonth = v2;
  /* FIXME: For 2 digit dates, I'm not sure if 30 is hard coded or not. It may
   * be retrieved from:
   * HKCU\Control Panel\International\Calendars\TwoDigitYearMax
   * But Wine doesn't have/use that key as at the time of writing.
   */
  st->wYear = v3 < 30 ? 2000 + v3 : v3 < 100 ? 1900 + v3 : v3;
  TRACE("Returning date %ld/%ld/%d\n", v1, v2, st->wYear);
  return S_OK;
}

/******************************************************************************
 * VarDateFromStr [OLEAUT32.94]
 *
 * Convert a VT_BSTR to at VT_DATE.
 *
 * PARAMS
 *  strIn    [I] String to convert
 *  lcid     [I] Locale identifier for the conversion
 *  dwFlags  [I] Flags affecting the conversion (VAR_ flags from "oleauto.h")
 *  pdateOut [O] Destination for the converted value
 *
 * RETURNS
 *  Success: S_OK. pdateOut contains the converted value.
 *  FAILURE: An HRESULT error code indicating the prolem.
 *
 * NOTES
 *  Any date format that can be created using the date formats from lcid
 *  (Either from kernel Nls functions, variant conversion or formatting) is a
 *  valid input to this function. In addition, a few more esoteric formats are
 *  also supported for compatability with the native version. The date is
 *  interpreted according to the date settings in the control panel, unless
 *  the date is invalid in that format, in which the most compatable format
 *  that produces a valid date will be used.
 */
HRESULT WINAPI VarDateFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, DATE* pdateOut)
{
  static const USHORT ParseDateTokens[] =
  {
    LOCALE_SMONTHNAME1, LOCALE_SMONTHNAME2, LOCALE_SMONTHNAME3, LOCALE_SMONTHNAME4,
    LOCALE_SMONTHNAME5, LOCALE_SMONTHNAME6, LOCALE_SMONTHNAME7, LOCALE_SMONTHNAME8,
    LOCALE_SMONTHNAME9, LOCALE_SMONTHNAME10, LOCALE_SMONTHNAME11, LOCALE_SMONTHNAME12,
    LOCALE_SMONTHNAME13,
    LOCALE_SABBREVMONTHNAME1, LOCALE_SABBREVMONTHNAME2, LOCALE_SABBREVMONTHNAME3,
    LOCALE_SABBREVMONTHNAME4, LOCALE_SABBREVMONTHNAME5, LOCALE_SABBREVMONTHNAME6,
    LOCALE_SABBREVMONTHNAME7, LOCALE_SABBREVMONTHNAME8, LOCALE_SABBREVMONTHNAME9,
    LOCALE_SABBREVMONTHNAME10, LOCALE_SABBREVMONTHNAME11, LOCALE_SABBREVMONTHNAME12,
    LOCALE_SABBREVMONTHNAME13,
    LOCALE_SDAYNAME1, LOCALE_SDAYNAME2, LOCALE_SDAYNAME3, LOCALE_SDAYNAME4,
    LOCALE_SDAYNAME5, LOCALE_SDAYNAME6, LOCALE_SDAYNAME7,
    LOCALE_SABBREVDAYNAME1, LOCALE_SABBREVDAYNAME2, LOCALE_SABBREVDAYNAME3,
    LOCALE_SABBREVDAYNAME4, LOCALE_SABBREVDAYNAME5, LOCALE_SABBREVDAYNAME6,
    LOCALE_SABBREVDAYNAME7,
    LOCALE_S1159, LOCALE_S2359
  };
  static const BYTE ParseDateMonths[] =
  {
    1,2,3,4,5,6,7,8,9,10,11,12,13,
    1,2,3,4,5,6,7,8,9,10,11,12,13
  };
  size_t i;
  BSTR tokens[sizeof(ParseDateTokens)/sizeof(ParseDateTokens[0])];
  DATEPARSE dp;
  DWORD dwDateSeps = 0, iDate = 0;
  HRESULT hRet = S_OK;

  if ((dwFlags & (VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY)) ==
      (VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY))
    return E_INVALIDARG;

  if (!strIn)
    return DISP_E_TYPEMISMATCH;

  *pdateOut = 0.0;

  TRACE("(%s,0x%08lx,0x%08lx,%p)\n", debugstr_w(strIn), lcid, dwFlags, pdateOut);

  memset(&dp, 0, sizeof(dp));

  GetLocaleInfoW(lcid, LOCALE_IDATE|LOCALE_RETURN_NUMBER|(dwFlags & LOCALE_NOUSEROVERRIDE),
                 (LPWSTR)&iDate, sizeof(iDate)/sizeof(WCHAR));
  TRACE("iDate is %ld\n", iDate);

  /* Get the month/day/am/pm tokens for this locale */
  for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
  {
    WCHAR buff[128];
    LCTYPE lctype =  ParseDateTokens[i] | (dwFlags & LOCALE_NOUSEROVERRIDE);

    /* FIXME: Alternate calendars - should use GetCalendarInfo() and/or
     *        GetAltMonthNames(). We should really cache these strings too.
     */
    buff[0] = '\0';
    GetLocaleInfoW(lcid, lctype, buff, sizeof(buff)/sizeof(WCHAR));
    tokens[i] = SysAllocString(buff);
    TRACE("token %d is %s\n", i, debugstr_w(tokens[i]));
  }

  /* Parse the string into our structure */
  while (*strIn)
  {
    if (dp.dwCount > 6)
      break;

    if (isdigitW(*strIn))
    {
      dp.dwValues[dp.dwCount] = strtoulW(strIn, &strIn, 10);
      dp.dwCount++;
      strIn--;
    }
    else if (isalpha(*strIn))
    {
      BOOL bFound = FALSE;

      for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
      {
        DWORD dwLen = strlenW(tokens[i]);
        if (dwLen && !strncmpiW(strIn, tokens[i], dwLen))
        {
          if (i <= 25)
          {
            dp.dwValues[dp.dwCount] = ParseDateMonths[i];
            dp.dwFlags[dp.dwCount] |= (DP_MONTH|DP_DATESEP);
            dp.dwCount++;
          }
          else if (i > 39)
          {
            if (!dp.dwCount || dp.dwParseFlags & (DP_AM|DP_PM))
              hRet = DISP_E_TYPEMISMATCH;
            else
            {
              dp.dwFlags[dp.dwCount - 1] |= (i == 40 ? DP_AM : DP_PM);
              dp.dwParseFlags |= (i == 40 ? DP_AM : DP_PM);
            }
          }
          strIn += (dwLen - 1);
          bFound = TRUE;
          break;
        }
      }

      if (!bFound)
      {
        if ((*strIn == 'a' || *strIn == 'A' || *strIn == 'p' || *strIn == 'P') &&
            (dp.dwCount && !(dp.dwParseFlags & (DP_AM|DP_PM))))
        {
          /* Special case - 'a' and 'p' are recognised as short for am/pm */
          if (*strIn == 'a' || *strIn == 'A')
          {
            dp.dwFlags[dp.dwCount - 1] |= DP_AM;
            dp.dwParseFlags |=  DP_AM;
          }
          else
          {
            dp.dwFlags[dp.dwCount - 1] |= DP_PM;
            dp.dwParseFlags |=  DP_PM;
          }
          strIn++;
        }
        else
        {
          TRACE("No matching token for %s\n", debugstr_w(strIn));
          hRet = DISP_E_TYPEMISMATCH;
          break;
        }
      }
    }
    else if (*strIn == ':' ||  *strIn == '.')
    {
      if (!dp.dwCount || !strIn[1])
        hRet = DISP_E_TYPEMISMATCH;
      else
        dp.dwFlags[dp.dwCount - 1] |= DP_TIMESEP;
    }
    else if (*strIn == '-' || *strIn == '/')
    {
      dwDateSeps++;
      if (dwDateSeps > 2 || !dp.dwCount || !strIn[1])
        hRet = DISP_E_TYPEMISMATCH;
      else
        dp.dwFlags[dp.dwCount - 1] |= DP_DATESEP;
    }
    else if (*strIn == ',' || isspaceW(*strIn))
    {
      if (*strIn == ',' && !strIn[1])
        hRet = DISP_E_TYPEMISMATCH;
    }
    else
    {
      hRet = DISP_E_TYPEMISMATCH;
    }
    strIn++;
  }

  if (!dp.dwCount || dp.dwCount > 6 ||
      (dp.dwCount == 1 && !(dp.dwParseFlags & (DP_AM|DP_PM))))
    hRet = DISP_E_TYPEMISMATCH;

  if (SUCCEEDED(hRet))
  {
    SYSTEMTIME st;
    DWORD dwOffset = 0; /* Start of date fields in dp.dwValues */

    st.wDayOfWeek = st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;

    /* Figure out which numbers correspond to which fields.
     *
     * This switch statement works based on the fact that native interprets any
     * fields that are not joined with a time seperator ('.' or ':') as date
     * fields. Thus we construct a value from 0-32 where each set bit indicates
     * a time field. This encapsulates the hundreds of permutations of 2-6 fields.
     * For valid permutations, we set dwOffset to point to the first date field
     * and shorten dp.dwCount by the number of time fields found. The real
     * magic here occurs in VARIANT_MakeDate() above, where we determine what
     * each date number must represent in the context of iDate.
     */
    TRACE("0x%08lx\n", TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4));

    switch (TIMEFLAG(0)|TIMEFLAG(1)|TIMEFLAG(2)|TIMEFLAG(3)|TIMEFLAG(4))
    {
    case 0x1: /* TT TTDD TTDDD */
      if (dp.dwCount > 3 &&
          ((dp.dwFlags[2] & (DP_AM|DP_PM)) || (dp.dwFlags[3] & (DP_AM|DP_PM)) ||
          (dp.dwFlags[4] & (DP_AM|DP_PM))))
        hRet = DISP_E_TYPEMISMATCH;
      else if (dp.dwCount != 2 && dp.dwCount != 4 && dp.dwCount != 5)
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour = dp.dwValues[0];
      st.wMinute  = dp.dwValues[1];
      dp.dwCount -= 2;
      dwOffset = 2;
      break;

    case 0x3: /* TTT TTTDD TTTDDD */
      if (dp.dwCount > 4 &&
          ((dp.dwFlags[3] & (DP_AM|DP_PM)) || (dp.dwFlags[4] & (DP_AM|DP_PM)) ||
          (dp.dwFlags[5] & (DP_AM|DP_PM))))
        hRet = DISP_E_TYPEMISMATCH;
      else if (dp.dwCount != 3 && dp.dwCount != 5 && dp.dwCount != 6)
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[0];
      st.wMinute = dp.dwValues[1];
      st.wSecond = dp.dwValues[2];
      dwOffset = 3;
      dp.dwCount -= 3;
      break;

    case 0x4: /* DDTT */
      if (dp.dwCount != 4 ||
          (dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)))
        hRet = DISP_E_TYPEMISMATCH;

      st.wHour = dp.dwValues[2];
      st.wMinute  = dp.dwValues[3];
      dp.dwCount -= 2;
      break;

   case 0x0: /* T DD DDD TDDD TDDD */
      if (dp.dwCount == 1 && (dp.dwParseFlags & (DP_AM|DP_PM)))
      {
        st.wHour = dp.dwValues[0]; /* T */
        dp.dwCount = 0;
        break;
      }
      else if (dp.dwCount > 4 || (dp.dwCount < 3 && dp.dwParseFlags & (DP_AM|DP_PM)))
      {
        hRet = DISP_E_TYPEMISMATCH;
      }
      else if (dp.dwCount == 3)
      {
        if (dp.dwFlags[0] & (DP_AM|DP_PM)) /* TDD */
        {
          dp.dwCount = 2;
          st.wHour = dp.dwValues[0];
          dwOffset = 1;
          break;
        }
        if (dp.dwFlags[2] & (DP_AM|DP_PM)) /* DDT */
        {
          dp.dwCount = 2;
          st.wHour = dp.dwValues[2];
          break;
        }
        else if (dp.dwParseFlags & (DP_AM|DP_PM))
          hRet = DISP_E_TYPEMISMATCH;
      }
      else if (dp.dwCount == 4)
      {
        dp.dwCount = 3;
        if (dp.dwFlags[0] & (DP_AM|DP_PM)) /* TDDD */
        {
          st.wHour = dp.dwValues[0];
          dwOffset = 1;
        }
        else if (dp.dwFlags[3] & (DP_AM|DP_PM)) /* DDDT */
        {
          st.wHour = dp.dwValues[3];
        }
        else
          hRet = DISP_E_TYPEMISMATCH;
        break;
      }
      /* .. fall through .. */

    case 0x8: /* DDDTT */
      if ((dp.dwCount == 2 && (dp.dwParseFlags & (DP_AM|DP_PM))) ||
          (dp.dwCount == 5 && ((dp.dwFlags[0] & (DP_AM|DP_PM)) ||
           (dp.dwFlags[1] & (DP_AM|DP_PM)) || (dp.dwFlags[2] & (DP_AM|DP_PM)))) ||
           dp.dwCount == 4 || dp.dwCount == 6)
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[3];
      st.wMinute = dp.dwValues[4];
      if (dp.dwCount == 5)
        dp.dwCount -= 2;
      break;

    case 0xC: /* DDTTT */
      if (dp.dwCount != 5 ||
          (dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)))
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[2];
      st.wMinute = dp.dwValues[3];
      st.wSecond = dp.dwValues[4];
      dp.dwCount -= 3;
      break;

    case 0x18: /* DDDTTT */
      if ((dp.dwFlags[0] & (DP_AM|DP_PM)) || (dp.dwFlags[1] & (DP_AM|DP_PM)) ||
          (dp.dwFlags[2] & (DP_AM|DP_PM)))
        hRet = DISP_E_TYPEMISMATCH;
      st.wHour   = dp.dwValues[3];
      st.wMinute = dp.dwValues[4];
      st.wSecond = dp.dwValues[5];
      dp.dwCount -= 3;
      break;

    default:
      hRet = DISP_E_TYPEMISMATCH;
      break;
    }

    if (SUCCEEDED(hRet))
    {
      hRet = VARIANT_MakeDate(&dp, iDate, dwOffset, &st);

      if (dwFlags & VAR_TIMEVALUEONLY)
      {
        st.wYear = 1899;
        st.wMonth = 12;
        st.wDay = 30;
      }
      else if (dwFlags & VAR_DATEVALUEONLY)
       st.wHour = st.wMinute = st.wSecond = 0;

      /* Finally, convert the value to a VT_DATE */
      if (SUCCEEDED(hRet))
        hRet = SystemTimeToVariantTime(&st, pdateOut) ? S_OK : DISP_E_TYPEMISMATCH;
    }
  }

  for (i = 0; i < sizeof(tokens)/sizeof(tokens[0]); i++)
    SysFreeString(tokens[i]);
  return hRet;
}

/******************************************************************************
 *		VarDateFromI1		[OLEAUT32.221]
 */
HRESULT WINAPI VarDateFromI1(signed char cIn, DATE* pdateOut)
{
	TRACE("( %c, %p ), stub\n", cIn, pdateOut );

	*pdateOut = (DATE) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromUI2		[OLEAUT32.222]
 */
HRESULT WINAPI VarDateFromUI2(USHORT uiIn, DATE* pdateOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pdateOut );

	*pdateOut = (DATE) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromUI4		[OLEAUT32.223]
 */
HRESULT WINAPI VarDateFromUI4(ULONG ulIn, DATE* pdateOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, pdateOut );

	if( ulIn < DATE_MIN || ulIn > DATE_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pdateOut = (DATE) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarDateFromBool		[OLEAUT32.96]
 */
HRESULT WINAPI VarDateFromBool(VARIANT_BOOL boolIn, DATE* pdateOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, pdateOut );

	*pdateOut = (DATE) boolIn;

	return S_OK;
}

/**********************************************************************
 *              VarDateFromCy [OLEAUT32.93]
 * Convert currency to date
 */
HRESULT WINAPI VarDateFromCy(CY cyIn, DATE* pdateOut) {
   *pdateOut = (DATE)((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (*pdateOut > DATE_MAX || *pdateOut < DATE_MIN) return DISP_E_TYPEMISMATCH;
   return S_OK;
}

/******************************************************************************
 *		VarBstrFromUI1		[OLEAUT32.108]
 */
HRESULT WINAPI VarBstrFromUI1(BYTE bVal, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %d, %ld, %ld, %p ), stub\n", bVal, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", bVal );

	*pbstrOut =  StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromI2		[OLEAUT32.109]
 */
HRESULT WINAPI VarBstrFromI2(short iVal, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %d, %ld, %ld, %p ), stub\n", iVal, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", iVal );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromI4		[OLEAUT32.110]
 */
HRESULT WINAPI VarBstrFromI4(LONG lIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %ld, %ld, %ld, %p ), stub\n", lIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, "%ld", lIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromR4		[OLEAUT32.111]
 */
HRESULT WINAPI VarBstrFromR4(FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %f, %ld, %ld, %p ), stub\n", fltIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, "%.7G", fltIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromR8		[OLEAUT32.112]
 */
HRESULT WINAPI VarBstrFromR8(double dblIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %f, %ld, %ld, %p ), stub\n", dblIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, "%.15G", dblIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *    VarBstrFromCy   [OLEAUT32.113]
 */
HRESULT WINAPI VarBstrFromCy(CY cyIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut) {
    HRESULT rc = S_OK;
    double curVal = 0.0;

	TRACE("([cyIn], %08lx, %08lx, %p), partial stub (no flags handled).\n", lcid, dwFlags, pbstrOut);

    /* Firstly get the currency in a double, then put it in a buffer */
    rc = VarR8FromCy(cyIn, &curVal);
    if (rc == S_OK) {
        sprintf(pBuffer, "%G", curVal);
        *pbstrOut = StringDupAtoBstr( pBuffer );
    }
	return rc;
}


/******************************************************************************
 *    VarBstrFromDate    [OLEAUT32.114]
 *
 * Convert a VT_DATE to a VT_BSTR.
 *
 * PARAMS
 *  dateIn   [I] Source
 *  lcid     [I] LCID for the conversion
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pbstrOut [O] Destination
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_INVALIDARG, if pbstrOut or dateIn is invalid.
 *           E_OUTOFMEMORY, if memory allocation fails.
 */
HRESULT WINAPI VarBstrFromDate(DATE dateIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
  SYSTEMTIME st;
  DWORD dwFormatFlags = dwFlags & LOCALE_NOUSEROVERRIDE;
  WCHAR date[128], *time;

  TRACE("(%g,0x%08lx,0x%08lx,%p)\n", dateIn, lcid, dwFlags, pbstrOut);

  if (!pbstrOut || !VariantTimeToSystemTime(dateIn, &st))
    return E_INVALIDARG;

  *pbstrOut = NULL;

  if (dwFlags & VAR_CALENDAR_THAI)
      st.wYear += 553; /* Use the Thai buddhist calendar year */
  else if (dwFlags & (VAR_CALENDAR_HIJRI|VAR_CALENDAR_GREGORIAN))
      FIXME("VAR_CALENDAR_HIJRI/VAR_CALENDAR_GREGORIAN not handled\n");

  if (dwFlags & LOCALE_USE_NLS)
    dwFlags &= ~(VAR_TIMEVALUEONLY|VAR_DATEVALUEONLY);
  else
  {
    double whole = dateIn < 0 ? ceil(dateIn) : floor(dateIn);
    double partial = dateIn - whole;

    if (whole == 0.0)
      dwFlags |= VAR_TIMEVALUEONLY;
    else if (partial < 1e-12)
      dwFlags |= VAR_DATEVALUEONLY;
  }

  if (dwFlags & VAR_TIMEVALUEONLY)
    date[0] = '\0';
  else
    if (!GetDateFormatW(lcid, dwFormatFlags|DATE_SHORTDATE, &st, NULL, date,
                        sizeof(date)/sizeof(WCHAR)))
      return E_INVALIDARG;

  if (!(dwFlags & VAR_DATEVALUEONLY))
  {
    time = date + strlenW(date);
    if (time != date)
      *time++ = ' ';
    if (!GetTimeFormatW(lcid, dwFormatFlags, &st, NULL, time,
                        sizeof(date)/sizeof(WCHAR)-(time-date)))
      return E_INVALIDARG;
  }

  *pbstrOut = SysAllocString(date);
  if (*pbstrOut)
    TRACE("returning %s\n", debugstr_w(*pbstrOut));
  return *pbstrOut ? S_OK : E_OUTOFMEMORY;
}

/******************************************************************************
 *		VarBstrFromBool		[OLEAUT32.116]
 */
HRESULT WINAPI VarBstrFromBool(VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %d, %ld, %ld, %p ), stub\n", boolIn, lcid, dwFlags, pbstrOut );

	sprintf( pBuffer, (boolIn == VARIANT_FALSE) ? "False" : "True" );

	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromI1		[OLEAUT32.229]
 */
HRESULT WINAPI VarBstrFromI1(signed char cIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %c, %ld, %ld, %p ), stub\n", cIn, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", cIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromUI2		[OLEAUT32.230]
 */
HRESULT WINAPI VarBstrFromUI2(USHORT uiIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %d, %ld, %ld, %p ), stub\n", uiIn, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%d", uiIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromUI4		[OLEAUT32.231]
 */
HRESULT WINAPI VarBstrFromUI4(ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	TRACE("( %ld, %ld, %ld, %p ), stub\n", ulIn, lcid, dwFlags, pbstrOut );
	sprintf( pBuffer, "%ld", ulIn );
	*pbstrOut = StringDupAtoBstr( pBuffer );

	return S_OK;
}

/******************************************************************************
 *		VarBstrFromDec		[OLEAUT32.@]
 */
HRESULT WINAPI VarBstrFromDec(DECIMAL* pDecIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut)
{
	if(!pDecIn->u.s.sign && !pDecIn->u.s.scale &&
	   !pDecIn->Hi32 && !pDecIn->u1.s1.Mid32)
	    return VarBstrFromUI4(pDecIn->u1.s1.Lo32, lcid, dwFlags, pbstrOut);
	FIXME("%c%08lx%08lx%08lx E%02x stub\n",
	(pDecIn->u.s.sign == DECIMAL_NEG) ? '-' :
	(pDecIn->u.s.sign == 0) ? '+' : '?',
	pDecIn->Hi32, pDecIn->u1.s1.Mid32, pDecIn->u1.s1.Lo32,
	pDecIn->u.s.scale);
	return E_INVALIDARG;
}

/******************************************************************************
 *		VarBoolFromUI1		[OLEAUT32.118]
 */
HRESULT WINAPI VarBoolFromUI1(BYTE bIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %d, %p ), stub\n", bIn, pboolOut );

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
 *		VarBoolFromI2		[OLEAUT32.119]
 */
HRESULT WINAPI VarBoolFromI2(short sIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %d, %p ), stub\n", sIn, pboolOut );

	*pboolOut = (sIn) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromI4		[OLEAUT32.120]
 */
HRESULT WINAPI VarBoolFromI4(LONG lIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, pboolOut );

	*pboolOut = (lIn) ? VARIANT_TRUE : VARIANT_FALSE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromR4		[OLEAUT32.121]
 */
HRESULT WINAPI VarBoolFromR4(FLOAT fltIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, pboolOut );

	*pboolOut = (fltIn == 0.0) ? VARIANT_FALSE : VARIANT_TRUE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromR8		[OLEAUT32.122]
 */
HRESULT WINAPI VarBoolFromR8(double dblIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, pboolOut );

	*pboolOut = (dblIn == 0.0) ? VARIANT_FALSE : VARIANT_TRUE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromDate		[OLEAUT32.123]
 */
HRESULT WINAPI VarBoolFromDate(DATE dateIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, pboolOut );

	*pboolOut = (dateIn == 0.0) ? VARIANT_FALSE : VARIANT_TRUE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromStr		[OLEAUT32.125]
 */
HRESULT WINAPI VarBoolFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL* pboolOut)
{
	static const WCHAR szTrue[] = { 'T','r','u','e','\0' };
	static const WCHAR szFalse[] = { 'F','a','l','s','e','\0' };
	HRESULT ret = S_OK;

	TRACE("( %p, %ld, %ld, %p ), stub\n", strIn, lcid, dwFlags, pboolOut );

	if( strIn == NULL || strlenW( strIn ) == 0 )
	{
		ret = DISP_E_TYPEMISMATCH;
	}

	if( ret == S_OK )
	{
		if( strcmpiW( (LPCWSTR)strIn, szTrue ) == 0 )
		{
			*pboolOut = VARIANT_TRUE;
		}
		else if( strcmpiW( (LPCWSTR)strIn, szFalse ) == 0 )
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
			else
				*pboolOut = (dValue == 0.0) ?
						VARIANT_FALSE : VARIANT_TRUE;
		}
	}

	return ret;
}

/******************************************************************************
 *		VarBoolFromI1		[OLEAUT32.233]
 */
HRESULT WINAPI VarBoolFromI1(signed char cIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %c, %p ), stub\n", cIn, pboolOut );

	*pboolOut = (cIn == 0) ? VARIANT_FALSE : VARIANT_TRUE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromUI2		[OLEAUT32.234]
 */
HRESULT WINAPI VarBoolFromUI2(USHORT uiIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pboolOut );

	*pboolOut = (uiIn == 0) ? VARIANT_FALSE : VARIANT_TRUE;

	return S_OK;
}

/******************************************************************************
 *		VarBoolFromUI4		[OLEAUT32.235]
 */
HRESULT WINAPI VarBoolFromUI4(ULONG ulIn, VARIANT_BOOL* pboolOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, pboolOut );

	*pboolOut = (ulIn == 0) ? VARIANT_FALSE : VARIANT_TRUE;

	return S_OK;
}

/**********************************************************************
 *              VarBoolFromCy [OLEAUT32.124]
 * Convert currency to boolean
 */
HRESULT WINAPI VarBoolFromCy(CY cyIn, VARIANT_BOOL* pboolOut) {
      if (cyIn.s.Hi || cyIn.s.Lo) *pboolOut = -1;
      else *pboolOut = 0;

      return S_OK;
}

/******************************************************************************
 *		VarI1FromUI1		[OLEAUT32.244]
 */
HRESULT WINAPI VarI1FromUI1(BYTE bIn, signed char *pcOut)
{
	TRACE("( %d, %p ), stub\n", bIn, pcOut );

	/* Check range of value.
	 */
	if( bIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromI2		[OLEAUT32.245]
 */
HRESULT WINAPI VarI1FromI2(short uiIn, signed char *pcOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pcOut );

	if( uiIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromI4		[OLEAUT32.246]
 */
HRESULT WINAPI VarI1FromI4(LONG lIn, signed char *pcOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, pcOut );

	if( lIn < I1_MIN || lIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromR4		[OLEAUT32.247]
 */
HRESULT WINAPI VarI1FromR4(FLOAT fltIn, signed char *pcOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, pcOut );

    fltIn = round( fltIn );
	if( fltIn < I1_MIN || fltIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromR8		[OLEAUT32.248]
 */
HRESULT WINAPI VarI1FromR8(double dblIn, signed char *pcOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, pcOut );

    dblIn = round( dblIn );
    if( dblIn < I1_MIN || dblIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromDate		[OLEAUT32.249]
 */
HRESULT WINAPI VarI1FromDate(DATE dateIn, signed char *pcOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, pcOut );

    dateIn = round( dateIn );
	if( dateIn < I1_MIN || dateIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromStr		[OLEAUT32.251]
 */
HRESULT WINAPI VarI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, signed char *pcOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, pcOut);
  return _VarI1FromStr(strIn, lcid, dwFlags, pcOut);
}

/******************************************************************************
 *		VarI1FromBool		[OLEAUT32.253]
 */
HRESULT WINAPI VarI1FromBool(VARIANT_BOOL boolIn, signed char *pcOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, pcOut );

	*pcOut = (CHAR) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromUI2		[OLEAUT32.254]
 */
HRESULT WINAPI VarI1FromUI2(USHORT uiIn, signed char *pcOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pcOut );

	if( uiIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarI1FromUI4		[OLEAUT32.255]
 */
HRESULT WINAPI VarI1FromUI4(ULONG ulIn, signed char *pcOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, pcOut );

	if( ulIn > I1_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pcOut = (CHAR) ulIn;

	return S_OK;
}

/**********************************************************************
 *              VarI1FromCy [OLEAUT32.250]
 * Convert currency to signed char
 */
HRESULT WINAPI VarI1FromCy(CY cyIn, signed char *pcOut) {
   double t = round((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (t > I1_MAX || t < I1_MIN) return DISP_E_OVERFLOW;

   *pcOut = (CHAR)t;
   return S_OK;
}

/******************************************************************************
 *		VarUI2FromUI1		[OLEAUT32.257]
 */
HRESULT WINAPI VarUI2FromUI1(BYTE bIn, USHORT* puiOut)
{
	TRACE("( %d, %p ), stub\n", bIn, puiOut );

	*puiOut = (USHORT) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromI2		[OLEAUT32.258]
 */
HRESULT WINAPI VarUI2FromI2(short uiIn, USHORT* puiOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, puiOut );

	if( uiIn < UI2_MIN )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromI4		[OLEAUT32.259]
 */
HRESULT WINAPI VarUI2FromI4(LONG lIn, USHORT* puiOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, puiOut );

	if( lIn < UI2_MIN || lIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromR4		[OLEAUT32.260]
 */
HRESULT WINAPI VarUI2FromR4(FLOAT fltIn, USHORT* puiOut)
{
	TRACE("( %f, %p ), stub\n", fltIn, puiOut );

    fltIn = round( fltIn );
	if( fltIn < UI2_MIN || fltIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) fltIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromR8		[OLEAUT32.261]
 */
HRESULT WINAPI VarUI2FromR8(double dblIn, USHORT* puiOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, puiOut );

    dblIn = round( dblIn );
    if( dblIn < UI2_MIN || dblIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromDate		[OLEAUT32.262]
 */
HRESULT WINAPI VarUI2FromDate(DATE dateIn, USHORT* puiOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, puiOut );

    dateIn = round( dateIn );
	if( dateIn < UI2_MIN || dateIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromStr		[OLEAUT32.264]
 */
HRESULT WINAPI VarUI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, USHORT* puiOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, puiOut);
  return _VarUI2FromStr(strIn, lcid, dwFlags, puiOut);
}

/******************************************************************************
 *		VarUI2FromBool		[OLEAUT32.266]
 */
HRESULT WINAPI VarUI2FromBool(VARIANT_BOOL boolIn, USHORT* puiOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, puiOut );

	*puiOut = (USHORT) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromI1		[OLEAUT32.267]
 */
HRESULT WINAPI VarUI2FromI1(signed char cIn, USHORT* puiOut)
{
	TRACE("( %c, %p ), stub\n", cIn, puiOut );

	*puiOut = (USHORT) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI2FromUI4		[OLEAUT32.268]
 */
HRESULT WINAPI VarUI2FromUI4(ULONG ulIn, USHORT* puiOut)
{
	TRACE("( %ld, %p ), stub\n", ulIn, puiOut );

	if( ulIn > UI2_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*puiOut = (USHORT) ulIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromStr		[OLEAUT32.277]
 */
HRESULT WINAPI VarUI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, ULONG* pulOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, pulOut);
  return _VarUI4FromStr(strIn, lcid, dwFlags, pulOut);
}

/**********************************************************************
 *              VarUI2FromCy [OLEAUT32.263]
 * Convert currency to unsigned short
 */
HRESULT WINAPI VarUI2FromCy(CY cyIn, USHORT* pusOut) {
   double t = round((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (t > UI2_MAX || t < UI2_MIN) return DISP_E_OVERFLOW;

   *pusOut = (USHORT)t;

   return S_OK;
}

/******************************************************************************
 *		VarUI4FromUI1		[OLEAUT32.270]
 */
HRESULT WINAPI VarUI4FromUI1(BYTE bIn, ULONG* pulOut)
{
	TRACE("( %d, %p ), stub\n", bIn, pulOut );

	*pulOut = (USHORT) bIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromI2		[OLEAUT32.271]
 */
HRESULT WINAPI VarUI4FromI2(short uiIn, ULONG* pulOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pulOut );

	if( uiIn < UI4_MIN )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) uiIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromI4		[OLEAUT32.272]
 */
HRESULT WINAPI VarUI4FromI4(LONG lIn, ULONG* pulOut)
{
	TRACE("( %ld, %p ), stub\n", lIn, pulOut );

	if( lIn < 0 )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) lIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromR4		[OLEAUT32.273]
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
 *		VarUI4FromR8		[OLEAUT32.274]
 */
HRESULT WINAPI VarUI4FromR8(double dblIn, ULONG* pulOut)
{
	TRACE("( %f, %p ), stub\n", dblIn, pulOut );

	dblIn = round( dblIn );
	if( dblIn < UI4_MIN || dblIn > UI4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) dblIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromDate		[OLEAUT32.275]
 */
HRESULT WINAPI VarUI4FromDate(DATE dateIn, ULONG* pulOut)
{
	TRACE("( %f, %p ), stub\n", dateIn, pulOut );

	dateIn = round( dateIn );
	if( dateIn < UI4_MIN || dateIn > UI4_MAX )
	{
		return DISP_E_OVERFLOW;
	}

	*pulOut = (ULONG) dateIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromBool		[OLEAUT32.279]
 */
HRESULT WINAPI VarUI4FromBool(VARIANT_BOOL boolIn, ULONG* pulOut)
{
	TRACE("( %d, %p ), stub\n", boolIn, pulOut );

	*pulOut = (ULONG) boolIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromI1		[OLEAUT32.280]
 */
HRESULT WINAPI VarUI4FromI1(signed char cIn, ULONG* pulOut)
{
	TRACE("( %c, %p ), stub\n", cIn, pulOut );

	*pulOut = (ULONG) cIn;

	return S_OK;
}

/******************************************************************************
 *		VarUI4FromUI2		[OLEAUT32.281]
 */
HRESULT WINAPI VarUI4FromUI2(USHORT uiIn, ULONG* pulOut)
{
	TRACE("( %d, %p ), stub\n", uiIn, pulOut );

	*pulOut = (ULONG) uiIn;

	return S_OK;
}

/**********************************************************************
 *              VarUI4FromCy [OLEAUT32.276]
 * Convert currency to unsigned long
 */
HRESULT WINAPI VarUI4FromCy(CY cyIn, ULONG* pulOut) {
   double t = round((((double)cyIn.s.Hi * 4294967296.0) + (double)cyIn.s.Lo) / 10000);

   if (t > UI4_MAX || t < UI4_MIN) return DISP_E_OVERFLOW;

   *pulOut = (ULONG)t;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromUI1 [OLEAUT32.98]
 * Convert unsigned char to currency
 */
HRESULT WINAPI VarCyFromUI1(BYTE bIn, CY* pcyOut) {
    pcyOut->s.Hi = 0;
    pcyOut->s.Lo = ((ULONG)bIn) * 10000;

    return S_OK;
}

/**********************************************************************
 *              VarCyFromI2 [OLEAUT32.99]
 * Convert signed short to currency
 */
HRESULT WINAPI VarCyFromI2(short sIn, CY* pcyOut) {
    if (sIn < 0) pcyOut->s.Hi = -1;
    else pcyOut->s.Hi = 0;
    pcyOut->s.Lo = ((ULONG)sIn) * 10000;

    return S_OK;
}

/**********************************************************************
 *              VarCyFromI4 [OLEAUT32.100]
 * Convert signed long to currency
 */
HRESULT WINAPI VarCyFromI4(LONG lIn, CY* pcyOut) {
      double t = (double)lIn * (double)10000;
      pcyOut->s.Hi = (LONG)(t / (double)4294967296.0);
      pcyOut->s.Lo = (ULONG)fmod(t, (double)4294967296.0);
      if (lIn < 0) pcyOut->s.Hi--;

      return S_OK;
}

/**********************************************************************
 *              VarCyFromR4 [OLEAUT32.101]
 * Convert float to currency
 */
HRESULT WINAPI VarCyFromR4(FLOAT fltIn, CY* pcyOut) {
   double t = round((double)fltIn * (double)10000);
   pcyOut->s.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->s.Lo = (ULONG)fmod(t, (double)4294967296.0);
   if (fltIn < 0) pcyOut->s.Hi--;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromR8 [OLEAUT32.102]
 * Convert double to currency
 */
HRESULT WINAPI VarCyFromR8(double dblIn, CY* pcyOut) {
   double t = round(dblIn * (double)10000);
   pcyOut->s.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->s.Lo = (ULONG)fmod(t, (double)4294967296.0);
   if (dblIn < 0) pcyOut->s.Hi--;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromDate [OLEAUT32.103]
 * Convert date to currency
 */
HRESULT WINAPI VarCyFromDate(DATE dateIn, CY* pcyOut) {
   double t = round((double)dateIn * (double)10000);
   pcyOut->s.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->s.Lo = (ULONG)fmod(t, (double)4294967296.0);
   if (dateIn < 0) pcyOut->s.Hi--;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromStr [OLEAUT32.104]
 * FIXME: Never tested with decimal separator other than '.'
 */
HRESULT WINAPI VarCyFromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, CY *pcyOut)
{
  TRACE("(%s, 0x%08lx, 0x%08lx, %p)\n", debugstr_w(strIn), lcid, dwFlags, pcyOut);
  return _VarCyFromStr(strIn, lcid, dwFlags, pcyOut);
}


/**********************************************************************
 *              VarCyFromBool [OLEAUT32.106]
 * Convert boolean to currency
 */
HRESULT WINAPI VarCyFromBool(VARIANT_BOOL boolIn, CY* pcyOut) {
   if (boolIn < 0) pcyOut->s.Hi = -1;
   else pcyOut->s.Hi = 0;
   pcyOut->s.Lo = (ULONG)boolIn * (ULONG)10000;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromI1 [OLEAUT32.225]
 * Convert signed char to currency
 */
HRESULT WINAPI VarCyFromI1(signed char cIn, CY* pcyOut) {
   if (cIn < 0) pcyOut->s.Hi = -1;
   else pcyOut->s.Hi = 0;
   pcyOut->s.Lo = (ULONG)cIn * (ULONG)10000;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromUI2 [OLEAUT32.226]
 * Convert unsigned short to currency
 */
HRESULT WINAPI VarCyFromUI2(USHORT usIn, CY* pcyOut) {
   pcyOut->s.Hi = 0;
   pcyOut->s.Lo = (ULONG)usIn * (ULONG)10000;

   return S_OK;
}

/**********************************************************************
 *              VarCyFromUI4 [OLEAUT32.227]
 * Convert unsigned long to currency
 */
HRESULT WINAPI VarCyFromUI4(ULONG ulIn, CY* pcyOut) {
   double t = (double)ulIn * (double)10000;
   pcyOut->s.Hi = (LONG)(t / (double)4294967296.0);
   pcyOut->s.Lo = (ULONG)fmod(t, (double)4294967296.0);

   return S_OK;
}

/**********************************************************************
 *              VarDecFromStr [OLEAUT32.@]
 */
HRESULT WINAPI VarDecFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags,
                             DECIMAL* pdecOut)
{   WCHAR *p=strIn;
    ULONGLONG t;
    ULONG  cy;

    DECIMAL_SETZERO(pdecOut);

    if(*p == (WCHAR)'-')pdecOut->u.s.sign= DECIMAL_NEG;
    if((*p == (WCHAR)'-') || (*p == (WCHAR)'+')) p++;
    for(;*p != (WCHAR)0; p++) {
        if((*p < (WCHAR)'0')||(*p > (WCHAR)'9')) goto error ;
        t = (ULONGLONG)pdecOut->u1.s1.Lo32 *(ULONGLONG)10
          + (ULONGLONG)(*p -(WCHAR)'0');
        cy = (ULONG)(t >> 32);
        pdecOut->u1.s1.Lo32 = (ULONG)(t & (ULONGLONG)UI4_MAX);
        t = (ULONGLONG)pdecOut->u1.s1.Mid32 * (ULONGLONG)10
          + (ULONGLONG)cy;
        cy = (ULONG)(t >> 32);
        pdecOut->u1.s1.Mid32 = (ULONG)(t & (ULONGLONG)UI4_MAX);
        t = (ULONGLONG)pdecOut->Hi32 * (ULONGLONG)10
          + (ULONGLONG)cy;
        cy = (ULONG)(t >> 32);
        pdecOut->Hi32 = (ULONG)(t & (ULONGLONG)UI4_MAX);
        if(cy) goto overflow ;
    }
    TRACE("%s -> sign %02x,hi %08lx,mid %08lx, lo%08lx, scale %08x\n",
          debugstr_w(strIn),
          pdecOut->u.s.sign, pdecOut->Hi32, pdecOut->u1.s1.Mid32,
          pdecOut->u1.s1.Lo32, pdecOut->u.s.scale);
    return S_OK;

overflow:
    /* like NT4 SP5 */
    pdecOut->Hi32 = pdecOut->u1.s1.Mid32 = pdecOut->u1.s1.Lo32 = 0xffffffff;
    return DISP_E_OVERFLOW;

error:
    ERR("%s: unknown char at pos %d\n",
              debugstr_w(strIn),  p - strIn + 1);
    return DISP_E_TYPEMISMATCH;
}

/* Date Conversions */

#define IsLeapYear(y) (((y % 4) == 0) && (((y % 100) != 0) || ((y % 400) == 0)))

/* Convert a VT_DATE value to a Julian Date */
static inline int VARIANT_JulianFromDate(int dateIn)
{
  int julianDays = dateIn;

  julianDays -= DATE_MIN; /* Convert to + days from 1 Jan 100 AD */
  julianDays += 1757585;  /* Convert to + days from 23 Nov 4713 BC (Julian) */
  return julianDays;
}

/* Convert a Julian Date to a VT_DATE value */
static inline int VARIANT_DateFromJulian(int dateIn)
{
  int julianDays = dateIn;

  julianDays -= 1757585;  /* Convert to + days from 1 Jan 100 AD */
  julianDays += DATE_MIN; /* Convert to +/- days from 1 Jan 1899 AD */
  return julianDays;
}

/* Convert a Julian date to Day/Month/Year - from PostgreSQL */
static inline void VARIANT_DMYFromJulian(int jd, USHORT *year, USHORT *month, USHORT *day)
{
  int j, i, l, n;

  l = jd + 68569;
  n = l * 4 / 146097;
  l -= (n * 146097 + 3) / 4;
  i = (4000 * (l + 1)) / 1461001;
  l += 31 - (i * 1461) / 4;
  j = (l * 80) / 2447;
  *day = l - (j * 2447) / 80;
  l = j / 11;
  *month = (j + 2) - (12 * l);
  *year = 100 * (n - 49) + i + l;
}

/* Convert Day/Month/Year to a Julian date - from PostgreSQL */
static inline double VARIANT_JulianFromDMY(USHORT year, USHORT month, USHORT day)
{
  int m12 = (month - 14) / 12;

  return ((1461 * (year + 4800 + m12)) / 4 + (367 * (month - 2 - 12 * m12)) / 12 -
           (3 * ((year + 4900 + m12) / 100)) / 4 + day - 32075);
}

/* Macros for accessing DOS format date/time fields */
#define DOS_YEAR(x)   (1980 + (x >> 9))
#define DOS_MONTH(x)  ((x >> 5) & 0xf)
#define DOS_DAY(x)    (x & 0x1f)
#define DOS_HOUR(x)   (x >> 11)
#define DOS_MINUTE(x) ((x >> 5) & 0x3f)
#define DOS_SECOND(x) ((x & 0x1f) << 1)
/* Create a DOS format date/time */
#define DOS_DATE(d,m,y) (d | (m << 5) | ((y-1980) << 9))
#define DOS_TIME(h,m,s) ((s >> 1) | (m << 5) | (h << 11))

/* Roll a date forwards or backwards to correct it */
static HRESULT VARIANT_RollUdate(UDATE *lpUd)
{
  static const BYTE days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

  TRACE("Raw date: %d/%d/%d %d:%d:%d\n", lpUd->st.wDay, lpUd->st.wMonth,
        lpUd->st.wYear, lpUd->st.wHour, lpUd->st.wMinute, lpUd->st.wSecond);

  /* Years < 100 are treated as 1900 + year */
  if (lpUd->st.wYear < 100)
    lpUd->st.wYear += 1900;

  if (!lpUd->st.wMonth)
  {
    /* Roll back to December of the previous year */
    lpUd->st.wMonth = 12;
    lpUd->st.wYear--;
  }
  else while (lpUd->st.wMonth > 12)
  {
    /* Roll forward the correct number of months */
    lpUd->st.wYear++;
    lpUd->st.wMonth -= 12;
  }

  if (lpUd->st.wYear > 9999 || lpUd->st.wHour > 23 ||
      lpUd->st.wMinute > 59 || lpUd->st.wSecond > 59)
    return E_INVALIDARG; /* Invalid values */

  if (!lpUd->st.wDay)
  {
    /* Roll back the date one day */
    if (lpUd->st.wMonth == 1)
    {
      /* Roll back to December 31 of the previous year */
      lpUd->st.wDay   = 31;
      lpUd->st.wMonth = 12;
      lpUd->st.wYear--;
    }
    else
    {
      lpUd->st.wMonth--; /* Previous month */
      if (lpUd->st.wMonth == 2 && IsLeapYear(lpUd->st.wYear))
        lpUd->st.wDay = 29; /* Februaury has 29 days on leap years */
      else
        lpUd->st.wDay = days[lpUd->st.wMonth]; /* Last day of the month */
    }
  }
  else if (lpUd->st.wDay > 28)
  {
    int rollForward = 0;

    /* Possibly need to roll the date forward */
    if (lpUd->st.wMonth == 2 && IsLeapYear(lpUd->st.wYear))
      rollForward = lpUd->st.wDay - 29; /* Februaury has 29 days on leap years */
    else
      rollForward = lpUd->st.wDay - days[lpUd->st.wMonth];

    if (rollForward > 0)
    {
      lpUd->st.wDay = rollForward;
      lpUd->st.wMonth++;
      if (lpUd->st.wMonth > 12)
      {
        lpUd->st.wMonth = 1; /* Roll forward into January of the next year */
        lpUd->st.wYear++;
      }
    }
  }
  TRACE("Rolled date: %d/%d/%d %d:%d:%d\n", lpUd->st.wDay, lpUd->st.wMonth,
        lpUd->st.wYear, lpUd->st.wHour, lpUd->st.wMinute, lpUd->st.wSecond);
  return S_OK;
}

/**********************************************************************
 *              DosDateTimeToVariantTime [OLEAUT32.14]
 *
 * Convert a Dos format date and time into variant VT_DATE format.
 *
 * PARAMS
 *  wDosDate [I] Dos format date
 *  wDosTime [I] Dos format time
 *  pDateOut [O] Destination for VT_DATE format
 *
 * RETURNS
 *  Success: TRUE. pDateOut contains the converted time.
 *  Failure: FALSE, if wDosDate or wDosTime are invalid (see notes).
 *
 * NOTES
 * - Dos format dates can only hold dates from 1-Jan-1980 to 31-Dec-2099.
 * - Dos format times are accurate to only 2 second precision.
 * - The format of a Dos Date is:
 *| Bits   Values  Meaning
 *| ----   ------  -------
 *| 0-4    1-31    Day of the week. 0 rolls back one day. A value greater than
 *|                the days in the month rolls forward the extra days.
 *| 5-8    1-12    Month of the year. 0 rolls back to December of the previous
 *|                year. 13-15 are invalid.
 *| 9-15   0-119   Year based from 1980 (Max 2099). 120-127 are invalid.
 * - The format of a Dos Time is:
 *| Bits   Values  Meaning
 *| ----   ------  -------
 *| 0-4    0-29    Seconds/2. 30 and 31 are invalid.
 *| 5-10   0-59    Minutes. 60-63 are invalid.
 *| 11-15  0-23    Hours (24 hour clock). 24-32 are invalid.
 */
INT WINAPI DosDateTimeToVariantTime(USHORT wDosDate, USHORT wDosTime,
                                    double *pDateOut)
{
  UDATE ud;

  TRACE("(0x%x(%d/%d/%d),0x%x(%d:%d:%d),%p)\n",
        wDosDate, DOS_YEAR(wDosDate), DOS_MONTH(wDosDate), DOS_DAY(wDosDate),
        wDosTime, DOS_HOUR(wDosTime), DOS_MINUTE(wDosTime), DOS_SECOND(wDosTime),
        pDateOut);

  ud.st.wYear = DOS_YEAR(wDosDate);
  ud.st.wMonth = DOS_MONTH(wDosDate);
  if (ud.st.wYear > 2099 || ud.st.wMonth > 12)
    return FALSE;
  ud.st.wDay = DOS_DAY(wDosDate);
  ud.st.wHour = DOS_HOUR(wDosTime);
  ud.st.wMinute = DOS_MINUTE(wDosTime);
  ud.st.wSecond = DOS_SECOND(wDosTime);
  ud.st.wDayOfWeek = ud.st.wMilliseconds = 0;

  return !VarDateFromUdate(&ud, 0, pDateOut);
}

/**********************************************************************
 *              VariantTimeToDosDateTime [OLEAUT32.13]
 *
 * Convert a variant format date into a Dos format date and time.
 *
 *  dateIn    [I] VT_DATE time format
 *  pwDosDate [O] Destination for Dos format date
 *  pwDosTime [O] Destination for Dos format time
 *
 * RETURNS
 *  Success: TRUE. pwDosDate and pwDosTime contains the converted values.
 *  Failure: FALSE, if dateIn cannot be represented in Dos format.
 *
 * NOTES
 *   See DosDateTimeToVariantTime() for Dos format details and bugs.
 */
INT WINAPI VariantTimeToDosDateTime(double dateIn, USHORT *pwDosDate, USHORT *pwDosTime)
{
  UDATE ud;

  TRACE("(%g,%p,%p)\n", dateIn, pwDosDate, pwDosTime);

  if (FAILED(VarUdateFromDate(dateIn, 0, &ud)))
    return FALSE;

  if (ud.st.wYear < 1980 || ud.st.wYear > 2099)
    return FALSE;

  *pwDosDate = DOS_DATE(ud.st.wDay, ud.st.wMonth, ud.st.wYear);
  *pwDosTime = DOS_TIME(ud.st.wHour, ud.st.wMinute, ud.st.wSecond);

  TRACE("Returning 0x%x(%d/%d/%d), 0x%x(%d:%d:%d)\n",
        *pwDosDate, DOS_YEAR(*pwDosDate), DOS_MONTH(*pwDosDate), DOS_DAY(*pwDosDate),
        *pwDosTime, DOS_HOUR(*pwDosTime), DOS_MINUTE(*pwDosTime), DOS_SECOND(*pwDosTime));
  return TRUE;
}

/***********************************************************************
 *              SystemTimeToVariantTime [OLEAUT32.184]
 *
 * Convert a System format date and time into variant VT_DATE format.
 *
 * PARAMS
 *  lpSt     [I] System format date and time
 *  pDateOut [O] Destination for VT_DATE format date
 *
 * RETURNS
 *  Success: TRUE. *pDateOut contains the converted value.
 *  Failure: FALSE, if lpSt cannot be represented in VT_DATE format.
 */
INT WINAPI SystemTimeToVariantTime(LPSYSTEMTIME lpSt, double *pDateOut)
{
  UDATE ud;

  TRACE("(%p->%d/%d/%d %d:%d:%d,%p)\n", lpSt, lpSt->wDay, lpSt->wMonth,
        lpSt->wYear, lpSt->wHour, lpSt->wMinute, lpSt->wSecond, pDateOut);

  if (lpSt->wMonth > 12)
    return FALSE;

  memcpy(&ud.st, lpSt, sizeof(ud.st));
  return !VarDateFromUdate(&ud, 0, pDateOut);
}

/***********************************************************************
 *              VariantTimeToSystemTime [OLEAUT32.185]
 *
 * Convert a variant VT_DATE into a System format date and time.
 *
 * PARAMS
 *  datein [I] Variant VT_DATE format date
 *  lpSt   [O] Destination for System format date and time
 *
 * RETURNS
 *  Success: TRUE. *lpSt contains the converted value.
 *  Failure: FALSE, if dateIn is too large or small.
 */
INT WINAPI VariantTimeToSystemTime(double dateIn, LPSYSTEMTIME lpSt)
{
  UDATE ud;

  TRACE("(%g,%p)\n", dateIn, lpSt);

  if (FAILED(VarUdateFromDate(dateIn, 0, &ud)))
    return FALSE;

  memcpy(lpSt, &ud.st, sizeof(ud.st));
  return TRUE;
}

/***********************************************************************
 *              VarDateFromUdate [OLEAUT32.330]
 *
 * Convert an unpacked format date and time to a variant VT_DATE.
 *
 * PARAMS
 *  pUdateIn [I] Unpacked format date and time to convert
 *  dwFlags  [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  pDateOut [O] Destination for variant VT_DATE.
 *
 * RETURNS
 *  Success: S_OK. *pDateOut contains the converted value.
 *  Failure: E_INVALIDARG, if pUdateIn cannot be represented in VT_DATE format.
 */
HRESULT WINAPI VarDateFromUdate(UDATE *pUdateIn, ULONG dwFlags, DATE *pDateOut)
{
  UDATE ud;
  double dateVal;

  TRACE("(%p->%d/%d/%d %d:%d:%d:%d %d %d,0x%08lx,%p)\n", pUdateIn,
        pUdateIn->st.wMonth, pUdateIn->st.wDay, pUdateIn->st.wYear,
        pUdateIn->st.wHour, pUdateIn->st.wMinute, pUdateIn->st.wSecond,
        pUdateIn->st.wMilliseconds, pUdateIn->st.wDayOfWeek,
        pUdateIn->wDayOfYear, dwFlags, pDateOut);

  memcpy(&ud, pUdateIn, sizeof(ud));

  if (dwFlags & VAR_VALIDDATE)
    WARN("Ignoring VAR_VALIDDATE\n");

  if (FAILED(VARIANT_RollUdate(&ud)))
    return E_INVALIDARG;

  /* Date */
  dateVal = VARIANT_DateFromJulian(VARIANT_JulianFromDMY(ud.st.wYear, ud.st.wMonth, ud.st.wDay));

  /* Time */
  dateVal += ud.st.wHour / 24.0;
  dateVal += ud.st.wMinute / 1440.0;
  dateVal += ud.st.wSecond / 86400.0;
  dateVal += ud.st.wMilliseconds / 86400000.0;

  TRACE("Returning %g\n", dateVal);
  *pDateOut = dateVal;
  return S_OK;
}

/***********************************************************************
 *              VarUdateFromDate [OLEAUT32.331]
 *
 * Convert a variant VT_DATE into an unpacked format date and time.
 *
 * PARAMS
 *  datein    [I] Variant VT_DATE format date
 *  dwFlags   [I] Flags controlling the conversion (VAR_ flags from "oleauto.h")
 *  lpUdate   [O] Destination for unpacked format date and time
 *
 * RETURNS
 *  Success: S_OK. *lpUdate contains the converted value.
 *  Failure: E_INVALIDARG, if dateIn is too large or small.
 */
HRESULT WINAPI VarUdateFromDate(DATE dateIn, ULONG dwFlags, UDATE *lpUdate)
{
  /* Cumulative totals of days per month */
  static const USHORT cumulativeDays[] =
  {
    0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334
  };
  double datePart, timePart;
  int julianDays;

  TRACE("(%g,0x%08lx,%p)\n", dateIn, dwFlags, lpUdate);

  if (dateIn <= (DATE_MIN - 1.0) || dateIn >= (DATE_MAX + 1.0))
    return E_INVALIDARG;

  datePart = dateIn < 0.0 ? ceil(dateIn) : floor(dateIn);
  /* Compensate for int truncation (always downwards) */
  timePart = dateIn - datePart + 0.00000000001;
  if (timePart >= 1.0)
    timePart -= 0.00000000001;

  /* Date */
  julianDays = VARIANT_JulianFromDate(dateIn);
  VARIANT_DMYFromJulian(julianDays, &lpUdate->st.wYear, &lpUdate->st.wMonth,
                        &lpUdate->st.wDay);

  datePart = (datePart + 1.5) / 7.0;
  lpUdate->st.wDayOfWeek = (datePart - floor(datePart)) * 7;
  if (lpUdate->st.wDayOfWeek == 0)
    lpUdate->st.wDayOfWeek = 5;
  else if (lpUdate->st.wDayOfWeek == 1)
    lpUdate->st.wDayOfWeek = 6;
  else
    lpUdate->st.wDayOfWeek -= 2;

  if (lpUdate->st.wMonth > 2 && IsLeapYear(lpUdate->st.wYear))
    lpUdate->wDayOfYear = 1; /* After February, in a leap year */
  else
    lpUdate->wDayOfYear = 0;

  lpUdate->wDayOfYear += cumulativeDays[lpUdate->st.wMonth];
  lpUdate->wDayOfYear += lpUdate->st.wDay;

  /* Time */
  timePart *= 24.0;
  lpUdate->st.wHour = timePart;
  timePart -= lpUdate->st.wHour;
  timePart *= 60.0;
  lpUdate->st.wMinute = timePart;
  timePart -= lpUdate->st.wMinute;
  timePart *= 60.0;
  lpUdate->st.wSecond = timePart;
  timePart -= lpUdate->st.wSecond;
  lpUdate->st.wMilliseconds = 0;
  if (timePart > 0.5)
  {
    /* Round the milliseconds, adjusting the time/date forward if needed */
    if (lpUdate->st.wSecond < 59)
      lpUdate->st.wSecond++;
    else
    {
      lpUdate->st.wSecond = 0;
      if (lpUdate->st.wMinute < 59)
        lpUdate->st.wMinute++;
      else
      {
        lpUdate->st.wMinute = 0;
        if (lpUdate->st.wHour < 23)
          lpUdate->st.wHour++;
        else
        {
          lpUdate->st.wHour = 0;
          /* Roll over a whole day */
          if (++lpUdate->st.wDay > 28)
            VARIANT_RollUdate(lpUdate);
        }
      }
    }
  }
  return S_OK;
}

#define GET_NUMBER_TEXT(fld,name) \
  buff[0] = 0; \
  if (!GetLocaleInfoW(lcid, lctype|fld, buff, sizeof(WCHAR) * 2)) \
    WARN("buffer too small for " #fld "\n"); \
  else \
    if (buff[0]) lpChars->name = buff[0]; \
  TRACE("lcid 0x%lx, " #name "=%d '%c'\n", lcid, lpChars->name, lpChars->name)

/* Get the valid number characters for an lcid */
void VARIANT_GetLocalisedNumberChars(VARIANT_NUMBER_CHARS *lpChars, LCID lcid, DWORD dwFlags)
{
  static const VARIANT_NUMBER_CHARS defaultChars = { '-','+','.',',','$',0,'.',',' };
  LCTYPE lctype = dwFlags & LOCALE_NOUSEROVERRIDE;
  WCHAR buff[4];

  memcpy(lpChars, &defaultChars, sizeof(defaultChars));
  GET_NUMBER_TEXT(LOCALE_SNEGATIVESIGN, cNegativeSymbol);
  GET_NUMBER_TEXT(LOCALE_SPOSITIVESIGN, cPositiveSymbol);
  GET_NUMBER_TEXT(LOCALE_SDECIMAL, cDecimalPoint);
  GET_NUMBER_TEXT(LOCALE_STHOUSAND, cDigitSeperator);
  GET_NUMBER_TEXT(LOCALE_SMONDECIMALSEP, cCurrencyDecimalPoint);
  GET_NUMBER_TEXT(LOCALE_SMONTHOUSANDSEP, cCurrencyDigitSeperator);

  /* Local currency symbols are often 2 characters */
  lpChars->cCurrencyLocal2 = '\0';
  switch(GetLocaleInfoW(lcid, lctype|LOCALE_SCURRENCY, buff, sizeof(WCHAR) * 4))
  {
    case 3: lpChars->cCurrencyLocal2 = buff[1]; /* Fall through */
    case 2: lpChars->cCurrencyLocal  = buff[0];
            break;
    default: WARN("buffer too small for LOCALE_SCURRENCY\n");
  }
  TRACE("lcid 0x%lx, cCurrencyLocal =%d,%d '%c','%c'\n", lcid, lpChars->cCurrencyLocal,
        lpChars->cCurrencyLocal2, lpChars->cCurrencyLocal, lpChars->cCurrencyLocal2);
}

/* Number Parsing States */
#define B_PROCESSING_EXPONENT 0x1
#define B_NEGATIVE_EXPONENT   0x2
#define B_EXPONENT_START      0x4
#define B_INEXACT_ZEROS       0x8
#define B_LEADING_ZERO        0x10

/**********************************************************************
 *              VarParseNumFromStr [OLEAUT32.46]
 *
 * Parse a string containing a number into a NUMPARSE structure.
 *
 * PARAMS
 *  lpszStr [I]   String to parse number from
 *  lcid    [I]   Locale Id for the conversion
 *  dwFlags [I]   0, or LOCALE_NOUSEROVERRIDE to use system default number chars
 *  pNumprs [I/O] Destination for parsed number
 *  rgbDig  [O]   Destination for digits read in
 *
 * RETURNS
 *  Success: S_OK. pNumprs and rgbDig contain the parsed representation of
 *           the number.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           DISP_E_TYPEMISMATCH, if the string is not a number or is formatted
 *           incorrectly.
 *           DISP_E_OVERFLOW, if rgbDig is too small to hold the number.
 *
 * NOTES
 *  pNumprs must have the following fields set:
 *   cDig: Set to the size of rgbDig.
 *   dwInFlags: Set to the allowable syntax of the number using NUMPRS_ flags
 *            from "oleauto.h".
 *
 * FIXME
 *  - I am unsure if this function should parse non-arabic (e.g. Thai)
 *   numerals, so this has not been implemented.
 */
HRESULT WINAPI VarParseNumFromStr(OLECHAR *lpszStr, LCID lcid, ULONG dwFlags,
                                  NUMPARSE *pNumprs, BYTE *rgbDig)
{
  VARIANT_NUMBER_CHARS chars;
  BYTE rgbTmp[1024];
  DWORD dwState = B_EXPONENT_START|B_INEXACT_ZEROS;
  int iMaxDigits = sizeof(rgbTmp) / sizeof(BYTE);
  int cchUsed = 0;

  TRACE("(%s,%ld,0x%08lx,%p,%p)\n", debugstr_w(lpszStr), lcid, dwFlags, pNumprs, rgbDig);

  if (pNumprs->dwInFlags & NUMPRS_HEX_OCT)
    FIXME("dwInFlags & NUMPRS_HEX_OCT not yet implemented!\n");

  if (!pNumprs || !rgbDig)
    return E_INVALIDARG;

  if (pNumprs->cDig < iMaxDigits)
    iMaxDigits = pNumprs->cDig;

  pNumprs->cDig = 0;
  pNumprs->dwOutFlags = 0;
  pNumprs->cchUsed = 0;
  pNumprs->nBaseShift = 0;
  pNumprs->nPwr10 = 0;

  if (!lpszStr)
    return DISP_E_TYPEMISMATCH;

  VARIANT_GetLocalisedNumberChars(&chars, lcid, dwFlags);

  /* First consume all the leading symbols and space from the string */
  while (1)
  {
    if (pNumprs->dwInFlags & NUMPRS_LEADING_WHITE && isspaceW(*lpszStr))
    {
      pNumprs->dwOutFlags |= NUMPRS_LEADING_WHITE;
      do
      {
        cchUsed++;
        lpszStr++;
      } while (isspaceW(*lpszStr));
    }
    else if (pNumprs->dwInFlags & NUMPRS_LEADING_PLUS &&
             *lpszStr == chars.cPositiveSymbol &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_PLUS))
    {
      pNumprs->dwOutFlags |= NUMPRS_LEADING_PLUS;
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_LEADING_MINUS &&
             *lpszStr == chars.cNegativeSymbol &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_MINUS))
    {
      pNumprs->dwOutFlags |= (NUMPRS_LEADING_MINUS|NUMPRS_NEG);
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_CURRENCY &&
             !(pNumprs->dwOutFlags & NUMPRS_CURRENCY) &&
             *lpszStr == chars.cCurrencyLocal &&
             (!chars.cCurrencyLocal2 || lpszStr[1] == chars.cCurrencyLocal2))
    {
      pNumprs->dwOutFlags |= NUMPRS_CURRENCY;
      cchUsed++;
      lpszStr++;
      /* Only accept currency characters */
      chars.cDecimalPoint = chars.cCurrencyDecimalPoint;
      chars.cDigitSeperator = chars.cCurrencyDigitSeperator;
    }
    else if (pNumprs->dwInFlags & NUMPRS_PARENS && *lpszStr == '(' &&
             !(pNumprs->dwOutFlags & NUMPRS_PARENS))
    {
      pNumprs->dwOutFlags |= NUMPRS_PARENS;
      cchUsed++;
      lpszStr++;
    }
    else
      break;
  }

  if (!(pNumprs->dwOutFlags & NUMPRS_CURRENCY))
  {
    /* Only accept non-currency characters */
    chars.cCurrencyDecimalPoint = chars.cDecimalPoint;
    chars.cCurrencyDigitSeperator = chars.cDigitSeperator;
  }

  /* Strip Leading zeros */
  while (*lpszStr == '0')
  {
    dwState |= B_LEADING_ZERO;
    cchUsed++;
    lpszStr++;
  }

  while (*lpszStr)
  {
    if (isdigitW(*lpszStr))
    {
      if (dwState & B_PROCESSING_EXPONENT)
      {
        int exponentSize = 0;
        if (dwState & B_EXPONENT_START)
        {
          while (*lpszStr == '0')
          {
            /* Skip leading zero's in the exponent */
            cchUsed++;
            lpszStr++;
          }
          if (!isdigitW(*lpszStr))
            break; /* No exponent digits - invalid */
        }

        while (isdigitW(*lpszStr))
        {
          exponentSize *= 10;
          exponentSize += *lpszStr - '0';
          cchUsed++;
          lpszStr++;
        }
        if (dwState & B_NEGATIVE_EXPONENT)
          exponentSize = -exponentSize;
        /* Add the exponent into the powers of 10 */
        pNumprs->nPwr10 += exponentSize;
        dwState &= ~(B_PROCESSING_EXPONENT|B_EXPONENT_START);
        lpszStr--; /* back up to allow processing of next char */
      }
      else
      {
        if (pNumprs->cDig >= iMaxDigits)
        {
          pNumprs->dwOutFlags |= NUMPRS_INEXACT;

          if (*lpszStr != '0')
            dwState &= ~B_INEXACT_ZEROS; /* Inexact number with non-trailing zeros */

          /* This digit can't be represented, but count it in nPwr10 */
          if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
            pNumprs->nPwr10--;
          else
            pNumprs->nPwr10++;
        }
        else
        {
          if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
            pNumprs->nPwr10--; /* Count decimal points in nPwr10 */
          rgbTmp[pNumprs->cDig] = *lpszStr - '0';
        }
        pNumprs->cDig++;
        cchUsed++;
      }
    }
    else if (*lpszStr == chars.cDigitSeperator && pNumprs->dwInFlags & NUMPRS_THOUSANDS)
    {
      pNumprs->dwOutFlags |= NUMPRS_THOUSANDS;
      cchUsed++;
    }
    else if (*lpszStr == chars.cDecimalPoint &&
             pNumprs->dwInFlags & NUMPRS_DECIMAL &&
             !(pNumprs->dwOutFlags & (NUMPRS_DECIMAL|NUMPRS_EXPONENT)))
    {
      pNumprs->dwOutFlags |= NUMPRS_DECIMAL;
      cchUsed++;

      /* Remove trailing zeros from the whole number part */
      while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
      {
        pNumprs->nPwr10++;
        pNumprs->cDig--;
      }

      /* If we have no digits so far, skip leading zeros */
      if (!pNumprs->cDig)
      {
        while (lpszStr[1] == '0')
        {
          dwState |= B_LEADING_ZERO;
          cchUsed++;
          lpszStr++;
        }
      }
    }
    else if ((*lpszStr == 'e' || *lpszStr == 'E') &&
             pNumprs->dwInFlags & NUMPRS_EXPONENT &&
             !(pNumprs->dwOutFlags & NUMPRS_EXPONENT))
    {
      dwState |= B_PROCESSING_EXPONENT;
      pNumprs->dwOutFlags |= NUMPRS_EXPONENT;
      cchUsed++;
    }
    else if (dwState & B_PROCESSING_EXPONENT && *lpszStr == chars.cPositiveSymbol)
    {
      cchUsed++; /* Ignore positive exponent */
    }
    else if (dwState & B_PROCESSING_EXPONENT && *lpszStr == chars.cNegativeSymbol)
    {
      dwState |= B_NEGATIVE_EXPONENT;
      cchUsed++;
    }
    else
      break; /* Stop at an unrecognised character */

    lpszStr++;
  }

  if (!pNumprs->cDig && dwState & B_LEADING_ZERO)
  {
    /* Ensure a 0 on its own gets stored */
    pNumprs->cDig = 1;
    rgbTmp[0] = 0;
  }

  if (pNumprs->dwOutFlags & NUMPRS_EXPONENT && dwState & B_PROCESSING_EXPONENT)
  {
    pNumprs->cchUsed = cchUsed;
    return DISP_E_TYPEMISMATCH; /* Failed to completely parse the exponent */
  }

  if (pNumprs->dwOutFlags & NUMPRS_INEXACT)
  {
    if (dwState & B_INEXACT_ZEROS)
      pNumprs->dwOutFlags &= ~NUMPRS_INEXACT; /* All zeros doesn't set NUMPRS_INEXACT */
  }
  else
  {
    /* Remove trailing zeros from the last (whole number or decimal) part */
    while (pNumprs->cDig > 1 && !rgbTmp[pNumprs->cDig - 1])
    {
      if (pNumprs->dwOutFlags & NUMPRS_DECIMAL)
        pNumprs->nPwr10--;
      else
        pNumprs->nPwr10++;
      pNumprs->cDig--;
    }
  }

  if (pNumprs->cDig <= iMaxDigits)
    pNumprs->dwOutFlags &= ~NUMPRS_INEXACT; /* Ignore stripped zeros for NUMPRS_INEXACT */
  else
    pNumprs->cDig = iMaxDigits; /* Only return iMaxDigits worth of digits */

  /* Copy the digits we processed into rgbDig */
  memcpy(rgbDig, rgbTmp, pNumprs->cDig * sizeof(BYTE));

  /* Consume any trailing symbols and space */
  while (1)
  {
    if ((pNumprs->dwInFlags & NUMPRS_TRAILING_WHITE) && isspaceW(*lpszStr))
    {
      pNumprs->dwOutFlags |= NUMPRS_TRAILING_WHITE;
      do
      {
        cchUsed++;
        lpszStr++;
      } while (isspaceW(*lpszStr));
    }
    else if (pNumprs->dwInFlags & NUMPRS_TRAILING_PLUS &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_PLUS) &&
             *lpszStr == chars.cPositiveSymbol)
    {
      pNumprs->dwOutFlags |= NUMPRS_TRAILING_PLUS;
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_TRAILING_MINUS &&
             !(pNumprs->dwOutFlags & NUMPRS_LEADING_MINUS) &&
             *lpszStr == chars.cNegativeSymbol)
    {
      pNumprs->dwOutFlags |= (NUMPRS_TRAILING_MINUS|NUMPRS_NEG);
      cchUsed++;
      lpszStr++;
    }
    else if (pNumprs->dwInFlags & NUMPRS_PARENS && *lpszStr == ')' &&
             pNumprs->dwOutFlags & NUMPRS_PARENS)
    {
      cchUsed++;
      lpszStr++;
      pNumprs->dwOutFlags |= NUMPRS_NEG;
    }
    else
      break;
  }

  if (pNumprs->dwOutFlags & NUMPRS_PARENS && !(pNumprs->dwOutFlags & NUMPRS_NEG))
  {
    pNumprs->cchUsed = cchUsed;
    return DISP_E_TYPEMISMATCH; /* Opening parenthesis not matched */
  }

  if (pNumprs->dwInFlags & NUMPRS_USE_ALL && *lpszStr != '\0')
    return DISP_E_TYPEMISMATCH; /* Not all chars were consumed */

  if (!pNumprs->cDig)
    return DISP_E_TYPEMISMATCH; /* No Number found */

  pNumprs->cchUsed = cchUsed;
  return S_OK;
}

/* VTBIT flags indicating an integer value */
#define INTEGER_VTBITS (VTBIT_I1|VTBIT_UI1|VTBIT_I2|VTBIT_UI2|VTBIT_I4|VTBIT_UI4|VTBIT_I8|VTBIT_UI8)
/* VTBIT flags indicating a real number value */
#define REAL_VTBITS (VTBIT_R4|VTBIT_R8|VTBIT_CY|VTBIT_DECIMAL)

/**********************************************************************
 *              VarNumFromParseNum [OLEAUT32.47]
 *
 * Convert a NUMPARSE structure into a numeric Variant type.
 *
 * PARAMS
 *  pNumprs  [I] Source for parsed number. cDig must be set to the size of rgbDig
 *  rgbDig   [I] Source for the numbers digits
 *  dwVtBits [I] VTBIT_ flags from "oleauto.h" indicating the acceptable dest types
 *  pVarDst  [O] Destination for the converted Variant value.
 *
 * RETURNS
 *  Success: S_OK. pVarDst contains the converted value.
 *  Failure: E_INVALIDARG, if any parameter is invalid.
 *           DISP_E_OVERFLOW, if the number is too big for the types set in dwVtBits.
 *
 * NOTES
 *  - The smallest favoured type present in dwVtBits that can represent the
 *    number in pNumprs without losing precision is used.
 *  - Signed types are preferrred over unsigned types of the same size.
 *  - Preferred types in order are: integer, float, double, currency then decimal.
 *  - Rounding (dropping of decimal points) occurs without error. See VarI8FromR8()
 *    for details of the rounding method.
 *  - pVarDst is not cleared before the result is stored in it.
 */
HRESULT WINAPI VarNumFromParseNum(NUMPARSE *pNumprs, BYTE *rgbDig,
                                  ULONG dwVtBits, VARIANT *pVarDst)
{
  /* Scale factors and limits for double arithmetics */
  static const double dblMultipliers[11] = {
    1.0, 10.0, 100.0, 1000.0, 10000.0, 100000.0,
    1000000.0, 10000000.0, 100000000.0, 1000000000.0, 10000000000.0
  };
  static const double dblMinimums[11] = {
    R8_MIN, R8_MIN*10.0, R8_MIN*100.0, R8_MIN*1000.0, R8_MIN*10000.0,
    R8_MIN*100000.0, R8_MIN*1000000.0, R8_MIN*10000000.0,
    R8_MIN*100000000.0, R8_MIN*1000000000.0, R8_MIN*10000000000.0
  };
  static const double dblMaximums[11] = {
    R8_MAX, R8_MAX/10.0, R8_MAX/100.0, R8_MAX/1000.0, R8_MAX/10000.0,
    R8_MAX/100000.0, R8_MAX/1000000.0, R8_MAX/10000000.0,
    R8_MAX/100000000.0, R8_MAX/1000000000.0, R8_MAX/10000000000.0
  };

  int wholeNumberDigits, fractionalDigits, divisor10 = 0, multiplier10 = 0;

  TRACE("(%p,%p,0x%lx,%p)\n", pNumprs, rgbDig, dwVtBits, pVarDst);

  if (pNumprs->nBaseShift)
  {
    /* nBaseShift indicates a hex or octal number */
    FIXME("nBaseShift=%d not yet implemented, returning overflow\n", pNumprs->nBaseShift);
    return DISP_E_OVERFLOW;
  }

  /* Count the number of relevant fractional and whole digits stored,
   * And compute the divisor/multiplier to scale the number by.
   */
  if (pNumprs->nPwr10 < 0)
  {
    if (-pNumprs->nPwr10 >= pNumprs->cDig)
    {
      /* A real number < +/- 1.0 e.g. 0.1024 or 0.01024 */
      wholeNumberDigits = 0;
      fractionalDigits = pNumprs->cDig;
      divisor10 = -pNumprs->nPwr10;
    }
    else
    {
      /* An exactly represented real number e.g. 1.024 */
      wholeNumberDigits = pNumprs->cDig + pNumprs->nPwr10;
      fractionalDigits = pNumprs->cDig - wholeNumberDigits;
      divisor10 = pNumprs->cDig - wholeNumberDigits;
    }
  }
  else if (pNumprs->nPwr10 == 0)
  {
    /* An exactly represented whole number e.g. 1024 */
    wholeNumberDigits = pNumprs->cDig;
    fractionalDigits = 0;
  }
  else /* pNumprs->nPwr10 > 0 */
  {
    /* A whole number followed by nPwr10 0's e.g. 102400 */
    wholeNumberDigits = pNumprs->cDig;
    fractionalDigits = 0;
    multiplier10 = pNumprs->nPwr10;
  }

  TRACE("cDig %d; nPwr10 %d, whole %d, frac %d ", pNumprs->cDig,
        pNumprs->nPwr10, wholeNumberDigits, fractionalDigits);
  TRACE("mult %d; div %d\n", multiplier10, divisor10);

  if (dwVtBits & INTEGER_VTBITS &&
      (!fractionalDigits || !(dwVtBits & (REAL_VTBITS|VTBIT_CY|VTBIT_DECIMAL))))
  {
    /* We have one or more integer output choices, and either:
     *  1) An integer input value, or
     *  2) A real number input value but no floating output choices.
     * So, place the integer value into pVarDst, using the smallest type
     * possible and preferring signed over unsigned types.
     */
    BOOL bOverflow = FALSE, bNegative;
    ULONG64 ul64 = 0;
    int i;

    /* Convert the integer part of the number into a UI8 */
    for (i = 0; i < wholeNumberDigits; i++)
    {
      if (ul64 > (UI8_MAX / 10 - rgbDig[i]))
      {
        TRACE("Overflow multiplying digits\n");
        bOverflow = TRUE;
        break;
      }
      ul64 = ul64 * 10 + rgbDig[i];
    }

    /* Account for the scale of the number */
    if (!bOverflow && multiplier10)
    {
      for (i = 0; i < multiplier10; i++)
      {
        if (ul64 > (UI8_MAX / 10))
        {
          TRACE("Overflow scaling number\n");
          bOverflow = TRUE;
          break;
        }
        ul64 = ul64 * 10;
      }
    }

    /* If we have any fractional digits, round the value.
     * Note we dont have to do this if divisor10 is < 1,
     * because this means the fractional part must be < 0.5
     */
    if (!bOverflow && fractionalDigits && divisor10 > 0)
    {
      const BYTE* fracDig = rgbDig + wholeNumberDigits;
      BOOL bAdjust = FALSE;

      TRACE("first decimal value is %d\n", *fracDig);

      if (*fracDig > 5)
        bAdjust = TRUE; /* > 0.5 */
      else if (*fracDig == 5)
      {
        for (i = 1; i < fractionalDigits; i++)
        {
          if (fracDig[i])
          {
            bAdjust = TRUE; /* > 0.5 */
            break;
          }
        }
        /* If exactly 0.5, round only odd values */
        if (i == fractionalDigits && (ul64 & 1))
          bAdjust = TRUE;
      }

      if (bAdjust)
      {
        if (ul64 == UI8_MAX)
        {
          TRACE("Overflow after rounding\n");
          bOverflow = TRUE;
        }
        ul64++;
      }
    }

    /* Zero is not a negative number */
    bNegative = pNumprs->dwOutFlags & NUMPRS_NEG && ul64 ? TRUE : FALSE;

    TRACE("Integer value is %lld, bNeg %d\n", ul64, bNegative);

    /* For negative integers, try the signed types in size order */
    if (!bOverflow && bNegative)
    {
      if (dwVtBits & (VTBIT_I1|VTBIT_I2|VTBIT_I4|VTBIT_I8))
      {
        if (dwVtBits & VTBIT_I1 && ul64 <= -I1_MIN)
        {
          V_VT(pVarDst) = VT_I1;
          V_I1(pVarDst) = -ul64;
          return S_OK;
        }
        else if (dwVtBits & VTBIT_I2 && ul64 <= -I2_MIN)
        {
          V_VT(pVarDst) = VT_I2;
          V_I2(pVarDst) = -ul64;
          return S_OK;
        }
        else if (dwVtBits & VTBIT_I4 && ul64 <= -((LONGLONG)I4_MIN))
        {
          V_VT(pVarDst) = VT_I4;
          V_I4(pVarDst) = -ul64;
          return S_OK;
        }
        else if (dwVtBits & VTBIT_I8 && ul64 <= (ULONGLONG)I8_MAX + 1)
        {
          V_VT(pVarDst) = VT_I8;
          V_I8(pVarDst) = -ul64;
          return S_OK;
        }
      }
    }
    else if (!bOverflow)
    {
      /* For positive integers, try signed then unsigned types in size order */
      if (dwVtBits & VTBIT_I1 && ul64 <= I1_MAX)
      {
        V_VT(pVarDst) = VT_I1;
        V_I1(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_UI1 && ul64 <= UI1_MAX)
      {
        V_VT(pVarDst) = VT_UI1;
        V_UI1(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_I2 && ul64 <= I2_MAX)
      {
        V_VT(pVarDst) = VT_I2;
        V_I2(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_UI2 && ul64 <= UI2_MAX)
      {
        V_VT(pVarDst) = VT_UI2;
        V_UI2(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_I4 && ul64 <= I4_MAX)
      {
        V_VT(pVarDst) = VT_I4;
        V_I4(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_UI4 && ul64 <= UI4_MAX)
      {
        V_VT(pVarDst) = VT_UI4;
        V_UI4(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_I8 && ul64 <= I8_MAX)
      {
        V_VT(pVarDst) = VT_I8;
        V_I8(pVarDst) = ul64;
        return S_OK;
      }
      if (dwVtBits & VTBIT_UI8)
      {
        V_VT(pVarDst) = VT_UI8;
        V_UI8(pVarDst) = ul64;
        return S_OK;
      }
    }
  }

  if (dwVtBits & REAL_VTBITS)
  {
    /* Try to put the number into a float or real */
    BOOL bOverflow = FALSE, bNegative = pNumprs->dwOutFlags & NUMPRS_NEG;
    double whole = 0.0;
    int i;

    /* Convert the number into a double */
    for (i = 0; i < pNumprs->cDig; i++)
      whole = whole * 10.0 + rgbDig[i];

    TRACE("Whole double value is %16.16g\n", whole);

    /* Account for the scale */
    while (multiplier10 > 10)
    {
      if (whole > dblMaximums[10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY);
        bOverflow = TRUE;
        break;
      }
      whole = whole * dblMultipliers[10];
      multiplier10 -= 10;
    }
    if (multiplier10)
    {
      if (whole > dblMaximums[multiplier10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY);
        bOverflow = TRUE;
      }
      else
        whole = whole * dblMultipliers[multiplier10];
    }

    TRACE("Scaled double value is %16.16g\n", whole);

    while (divisor10 > 10)
    {
      if (whole < dblMinimums[10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY); /* Underflow */
        bOverflow = TRUE;
        break;
      }
      whole = whole / dblMultipliers[10];
      divisor10 -= 10;
    }
    if (divisor10)
    {
      if (whole < dblMinimums[divisor10])
      {
        dwVtBits &= ~(VTBIT_R4|VTBIT_R8|VTBIT_CY); /* Underflow */
        bOverflow = TRUE;
      }
      else
        whole = whole / dblMultipliers[divisor10];
    }
    if (!bOverflow)
      TRACE("Final double value is %16.16g\n", whole);

    if (dwVtBits & VTBIT_R4 &&
        ((whole <= R4_MAX && whole >= R4_MIN) || whole == 0.0))
    {
      TRACE("Set R4 to final value\n");
      V_VT(pVarDst) = VT_R4; /* Fits into a float */
      V_R4(pVarDst) = pNumprs->dwOutFlags & NUMPRS_NEG ? -whole : whole;
      return S_OK;
    }

    if (dwVtBits & VTBIT_R8)
    {
      TRACE("Set R8 to final value\n");
      V_VT(pVarDst) = VT_R8; /* Fits into a double */
      V_R8(pVarDst) = pNumprs->dwOutFlags & NUMPRS_NEG ? -whole : whole;
      return S_OK;
    }

    if (dwVtBits & VTBIT_CY)
    {
      if (SUCCEEDED(VarCyFromR8(bNegative ? -whole : whole, &V_CY(pVarDst))))
      {
        V_VT(pVarDst) = VT_CY; /* Fits into a currency */
        TRACE("Set CY to final value\n");
        return S_OK;
      }
      TRACE("Value Overflows CY\n");
    }

    if (!bOverflow && dwVtBits & VTBIT_DECIMAL)
    {
      WARN("VTBIT_DECIMAL not yet implemented\n");
#if 0
      if (SUCCEEDED(VarDecFromR8(bNegative ? -whole : whole, &V_DECIMAL(pVarDst))))
      {
        V_VT(pVarDst) = VT_DECIMAL; /* Fits into a decimal */
        TRACE("Set DECIMAL to final value\n");
        return S_OK;
      }
#endif
    }
  }

  if (dwVtBits & VTBIT_DECIMAL)
  {
    FIXME("VT_DECIMAL > R8 not yet supported, returning overflow\n");
  }
  return DISP_E_OVERFLOW; /* No more output choices */
}

/**********************************************************************
 *              VarBstrCmp [OLEAUT32.314]
 *
 * flags can be:
 *   NORM_IGNORECASE, NORM_IGNORENONSPACE, NORM_IGNORESYMBOLS
 *   NORM_IGNORESTRINGWIDTH, NORM_IGNOREKANATYPE, NORM_IGNOREKASHIDA
 *
 */
HRESULT WINAPI VarBstrCmp(BSTR left, BSTR right, LCID lcid, DWORD flags)
{
    INT r;

    TRACE("( %s %s %ld %lx ) partial stub\n", debugstr_w(left), debugstr_w(right), lcid, flags);

    /* Contrary to the MSDN, this returns eq for null vs null, null vs L"" and L"" vs NULL */
    if((!left) || (!right)) {

        if (!left && (!right || *right==0)) return VARCMP_EQ;
        else if (!right && (!left || *left==0)) return VARCMP_EQ;
        else return VARCMP_NULL;
    }

    if(flags&NORM_IGNORECASE)
        r = lstrcmpiW(left,right);
    else
        r = lstrcmpW(left,right);

    if(r<0)
        return VARCMP_LT;
    if(r>0)
        return VARCMP_GT;

    return VARCMP_EQ;
}

/**********************************************************************
 *              VarBstrCat [OLEAUT32.313]
 */
HRESULT WINAPI VarBstrCat(BSTR left, BSTR right, BSTR *out)
{
    BSTR result;
    int size = 0;

    TRACE("( %s %s %p )\n", debugstr_w(left), debugstr_w(right), out);

    /* On Windows, NULL parms are still handled (as empty strings) */
    if (left)  size=size + lstrlenW(left);
    if (right) size=size + lstrlenW(right);

    if (out) {
        result = SysAllocStringLen(NULL, size);
        *out = result;
        if (left) lstrcatW(result,left);
        if (right) lstrcatW(result,right);
        TRACE("result = %s, [%p]\n", debugstr_w(result), result);
    }
    return S_OK;
}

/**********************************************************************
 *              VarCat [OLEAUT32.318]
 */
HRESULT WINAPI VarCat(LPVARIANT left, LPVARIANT right, LPVARIANT out)
{
    /* Should we VariantClear out? */
    /* Can we handle array, vector, by ref etc. */
    if ((V_VT(left)&VT_TYPEMASK) == VT_NULL &&
        (V_VT(right)&VT_TYPEMASK) == VT_NULL)
    {
        V_VT(out) = VT_NULL;
        return S_OK;
    }

    if (V_VT(left) == VT_BSTR && V_VT(right) == VT_BSTR)
    {
        V_VT(out) = VT_BSTR;
        VarBstrCat (V_BSTR(left), V_BSTR(right), &V_BSTR(out));
        return S_OK;
    }
    if (V_VT(left) == VT_BSTR) {
        VARIANT bstrvar;
	HRESULT hres;

        V_VT(out) = VT_BSTR;
        hres = VariantChangeTypeEx(&bstrvar,right,0,0,VT_BSTR);
	if (hres) {
	    FIXME("Failed to convert right side from vt %d to VT_BSTR?\n",V_VT(right));
	    return hres;
        }
        VarBstrCat (V_BSTR(left), V_BSTR(&bstrvar), &V_BSTR(out));
        return S_OK;
    }
    if (V_VT(right) == VT_BSTR) {
        VARIANT bstrvar;
	HRESULT hres;

        V_VT(out) = VT_BSTR;
        hres = VariantChangeTypeEx(&bstrvar,left,0,0,VT_BSTR);
	if (hres) {
	    FIXME("Failed to convert right side from vt %d to VT_BSTR?\n",V_VT(right));
	    return hres;
        }
        VarBstrCat (V_BSTR(&bstrvar), V_BSTR(right), &V_BSTR(out));
        return S_OK;
    }
    FIXME ("types %d / %d not supported\n",V_VT(left)&VT_TYPEMASK, V_VT(right)&VT_TYPEMASK);
    return S_OK;
}

/**********************************************************************
 *              VarCmp [OLEAUT32.176]
 *
 * flags can be:
 *   NORM_IGNORECASE, NORM_IGNORENONSPACE, NORM_IGNORESYMBOLS
 *   NORM_IGNOREWIDTH, NORM_IGNOREKANATYPE, NORM_IGNOREKASHIDA
 *
 */
HRESULT WINAPI VarCmp(LPVARIANT left, LPVARIANT right, LCID lcid, DWORD flags)
{


    BOOL	lOk        = TRUE;
    BOOL	rOk        = TRUE;
    LONGLONG	lVal = -1;
    LONGLONG	rVal = -1;
    VARIANT	rv,lv;
    DWORD	xmask;
    HRESULT	rc;

    VariantInit(&lv);VariantInit(&rv);
    V_VT(right) &= ~0x8000; /* hack since we sometime get this flag.  */
    V_VT(left) &= ~0x8000; /* hack since we sometime get this flag. */

    TRACE("Left Var:\n");
    dump_Variant(left);
    TRACE("Right Var:\n");
    dump_Variant(right);

    /* If either are null, then return VARCMP_NULL */
    if ((V_VT(left)&VT_TYPEMASK) == VT_NULL ||
        (V_VT(right)&VT_TYPEMASK) == VT_NULL)
        return VARCMP_NULL;

    /* Strings - use VarBstrCmp */
    if ((V_VT(left)&VT_TYPEMASK) == VT_BSTR &&
        (V_VT(right)&VT_TYPEMASK) == VT_BSTR) {
        return VarBstrCmp(V_BSTR(left), V_BSTR(right), lcid, flags);
    }

    xmask = (1<<(V_VT(left)&VT_TYPEMASK))|(1<<(V_VT(right)&VT_TYPEMASK));
    if (xmask & (1<<VT_R8)) {
	rc = VariantChangeType(&lv,left,0,VT_R8);
	if (FAILED(rc)) return rc;
	rc = VariantChangeType(&rv,right,0,VT_R8);
	if (FAILED(rc)) return rc;

	if (V_R8(&lv) == V_R8(&rv)) return VARCMP_EQ;
	if (V_R8(&lv) < V_R8(&rv)) return VARCMP_LT;
	if (V_R8(&lv) > V_R8(&rv)) return VARCMP_GT;
	return E_FAIL; /* can't get here */
    }
    if (xmask & (1<<VT_R4)) {
	rc = VariantChangeType(&lv,left,0,VT_R4);
	if (FAILED(rc)) return rc;
	rc = VariantChangeType(&rv,right,0,VT_R4);
	if (FAILED(rc)) return rc;

	if (V_R4(&lv) == V_R4(&rv)) return VARCMP_EQ;
	if (V_R4(&lv) < V_R4(&rv)) return VARCMP_LT;
	if (V_R4(&lv) > V_R4(&rv)) return VARCMP_GT;
	return E_FAIL; /* can't get here */
    }

    /* Integers - Ideally like to use VarDecCmp, but no Dec support yet
           Use LONGLONG to maximize ranges                              */
    lOk = TRUE;
    switch (V_VT(left)&VT_TYPEMASK) {
    case VT_I1   : lVal = V_UNION(left,cVal); break;
    case VT_I2   : lVal = V_UNION(left,iVal); break;
    case VT_I4   : lVal = V_UNION(left,lVal); break;
    case VT_INT  : lVal = V_UNION(left,lVal); break;
    case VT_UI1  : lVal = V_UNION(left,bVal); break;
    case VT_UI2  : lVal = V_UNION(left,uiVal); break;
    case VT_UI4  : lVal = V_UNION(left,ulVal); break;
    case VT_UINT : lVal = V_UNION(left,ulVal); break;
    case VT_BOOL : lVal = V_UNION(left,boolVal); break;
    default: lOk = FALSE;
    }

    rOk = TRUE;
    switch (V_VT(right)&VT_TYPEMASK) {
    case VT_I1   : rVal = V_UNION(right,cVal); break;
    case VT_I2   : rVal = V_UNION(right,iVal); break;
    case VT_I4   : rVal = V_UNION(right,lVal); break;
    case VT_INT  : rVal = V_UNION(right,lVal); break;
    case VT_UI1  : rVal = V_UNION(right,bVal); break;
    case VT_UI2  : rVal = V_UNION(right,uiVal); break;
    case VT_UI4  : rVal = V_UNION(right,ulVal); break;
    case VT_UINT : rVal = V_UNION(right,ulVal); break;
    case VT_BOOL : rVal = V_UNION(right,boolVal); break;
    default: rOk = FALSE;
    }

    if (lOk && rOk) {
        if (lVal < rVal) {
            return VARCMP_LT;
        } else if (lVal > rVal) {
            return VARCMP_GT;
        } else {
            return VARCMP_EQ;
        }
    }

    /* Strings - use VarBstrCmp */
    if ((V_VT(left)&VT_TYPEMASK) == VT_DATE &&
        (V_VT(right)&VT_TYPEMASK) == VT_DATE) {

        if (floor(V_UNION(left,date)) == floor(V_UNION(right,date))) {
            /* Due to floating point rounding errors, calculate varDate in whole numbers) */
            double wholePart = 0.0;
            double leftR;
            double rightR;

            /* Get the fraction * 24*60*60 to make it into whole seconds */
            wholePart = (double) floor( V_UNION(left,date) );
            if (wholePart == 0) wholePart = 1;
            leftR = floor(fmod( V_UNION(left,date), wholePart ) * (24*60*60));

            wholePart = (double) floor( V_UNION(right,date) );
            if (wholePart == 0) wholePart = 1;
            rightR = floor(fmod( V_UNION(right,date), wholePart ) * (24*60*60));

            if (leftR < rightR) {
                return VARCMP_LT;
            } else if (leftR > rightR) {
                return VARCMP_GT;
            } else {
                return VARCMP_EQ;
            }

        } else if (V_UNION(left,date) < V_UNION(right,date)) {
            return VARCMP_LT;
        } else if (V_UNION(left,date) > V_UNION(right,date)) {
            return VARCMP_GT;
        }
    }
    FIXME("VarCmp partial implementation, doesnt support vt 0x%x / 0x%x\n",V_VT(left), V_VT(right));
    return E_FAIL;
}

/**********************************************************************
 *              VarAnd [OLEAUT32.142]
 *
 */
HRESULT WINAPI VarAnd(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;

    TRACE("Left Var:\n");
    dump_Variant(left);
    TRACE("Right Var:\n");
    dump_Variant(right);

    if ((V_VT(left)&VT_TYPEMASK) == VT_BOOL &&
        (V_VT(right)&VT_TYPEMASK) == VT_BOOL) {

        V_VT(result) = VT_BOOL;
        if (V_BOOL(left) && V_BOOL(right)) {
            V_BOOL(result) = VARIANT_TRUE;
        } else {
            V_BOOL(result) = VARIANT_FALSE;
        }
        rc = S_OK;

    } else {
        /* Integers */
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        LONGLONG     lVal = -1;
        LONGLONG     rVal = -1;
        LONGLONG     res  = -1;
        int          resT = 0; /* Testing has shown I2 & I2 == I2, all else
                                  becomes I4, even unsigned ints (incl. UI2) */

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);  resT=VT_I4; break;
        case VT_I2   : lVal = V_UNION(left,iVal);  resT=VT_I2; break;
        case VT_I4   : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_INT  : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_UI1  : lVal = V_UNION(left,bVal);  resT=VT_I4; break;
        case VT_UI2  : lVal = V_UNION(left,uiVal); resT=VT_I4; break;
        case VT_UI4  : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        case VT_UINT : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  resT=VT_I4; break;
        case VT_I2   : rVal = V_UNION(right,iVal);  resT=max(VT_I2, resT); break;
        case VT_I4   : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_INT  : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  resT=VT_I4; break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); resT=VT_I4; break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        case VT_UINT : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal & rVal);
            V_VT(result) = resT;
            switch (resT) {
            case VT_I2   : V_UNION(result,iVal)  = res; break;
            case VT_I4   : V_UNION(result,lVal)  = res; break;
            default:
                FIXME("Unexpected result variant type %x\n", resT);
                V_UNION(result,lVal)  = res;
            }
            rc = S_OK;

        } else {
            FIXME("VarAnd stub\n");
        }
    }

    TRACE("rc=%d, Result:\n", (int) rc);
    dump_Variant(result);
    return rc;
}

/**********************************************************************
 *              VarAdd [OLEAUT32.141]
 * FIXME: From MSDN: If ... Then
 * Both expressions are of the string type Concatenated.
 * One expression is a string type and the other a character Addition.
 * One expression is numeric and the other is a string Addition.
 * Both expressions are numeric Addition.
 * Either expression is NULL NULL is returned.
 * Both expressions are empty  Integer subtype is returned.
 *
 */
HRESULT WINAPI VarAdd(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;

    TRACE("Left Var:\n");
    dump_Variant(left);
    TRACE("Right Var:\n");
    dump_Variant(right);

    if ((V_VT(left)&VT_TYPEMASK) == VT_EMPTY)
    	return VariantCopy(result,right);

    if ((V_VT(right)&VT_TYPEMASK) == VT_EMPTY)
    	return VariantCopy(result,left);

    if (((V_VT(left)&VT_TYPEMASK) == VT_R8) || ((V_VT(right)&VT_TYPEMASK) == VT_R8)) {
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        double       lVal = -1;
        double       rVal = -1;
        double       res  = -1;

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);   break;
        case VT_I2   : lVal = V_UNION(left,iVal);   break;
        case VT_I4   : lVal = V_UNION(left,lVal);   break;
        case VT_INT  : lVal = V_UNION(left,lVal);   break;
        case VT_UI1  : lVal = V_UNION(left,bVal);   break;
        case VT_UI2  : lVal = V_UNION(left,uiVal);  break;
        case VT_UI4  : lVal = V_UNION(left,ulVal);  break;
        case VT_UINT : lVal = V_UNION(left,ulVal);  break;
        case VT_R4   : lVal = V_UNION(left,fltVal);  break;
        case VT_R8   : lVal = V_UNION(left,dblVal);  break;
	case VT_NULL : lVal = 0.0;  break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  break;
        case VT_I2   : rVal = V_UNION(right,iVal);  break;
        case VT_I4   : rVal = V_UNION(right,lVal);  break;
        case VT_INT  : rVal = V_UNION(right,lVal);  break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); break;
        case VT_UINT : rVal = V_UNION(right,ulVal); break;
        case VT_R4   : rVal = V_UNION(right,fltVal);break;
        case VT_R8   : rVal = V_UNION(right,dblVal);break;
	case VT_NULL : rVal = 0.0; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal + rVal);
            V_VT(result) = VT_R8;
            V_UNION(result,dblVal)  = res;
            rc = S_OK;
        } else {
	    FIXME("Unhandled type pair %d / %d in double addition.\n", 
        	(V_VT(left)&VT_TYPEMASK),
        	(V_VT(right)&VT_TYPEMASK)
	    );
	}
	return rc;
    }

    /* Handle strings as concat */
    if ((V_VT(left)&VT_TYPEMASK) == VT_BSTR &&
        (V_VT(right)&VT_TYPEMASK) == VT_BSTR) {
        V_VT(result) = VT_BSTR;
        rc = VarBstrCat(V_BSTR(left), V_BSTR(right), &V_BSTR(result));
    } else {

        /* Integers */
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        LONGLONG     lVal = -1;
        LONGLONG     rVal = -1;
        LONGLONG     res  = -1;
        int          resT = 0; /* Testing has shown I2 + I2 == I2, all else
                                  becomes I4                                */

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);  resT=VT_I4; break;
        case VT_I2   : lVal = V_UNION(left,iVal);  resT=VT_I2; break;
        case VT_I4   : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_INT  : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_UI1  : lVal = V_UNION(left,bVal);  resT=VT_I4; break;
        case VT_UI2  : lVal = V_UNION(left,uiVal); resT=VT_I4; break;
        case VT_UI4  : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        case VT_UINT : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
	case VT_NULL : lVal = 0; resT = VT_I4; break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  resT=VT_I4; break;
        case VT_I2   : rVal = V_UNION(right,iVal);  resT=max(VT_I2, resT); break;
        case VT_I4   : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_INT  : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  resT=VT_I4; break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); resT=VT_I4; break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        case VT_UINT : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
	case VT_NULL : rVal = 0; resT=VT_I4; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal + rVal);
            V_VT(result) = resT;
            switch (resT) {
            case VT_I2   : V_UNION(result,iVal)  = res; break;
            case VT_I4   : V_UNION(result,lVal)  = res; break;
            default:
                FIXME("Unexpected result variant type %x\n", resT);
                V_UNION(result,lVal)  = res;
            }
            rc = S_OK;

        } else {
            FIXME("unimplemented part (0x%x + 0x%x)\n",V_VT(left), V_VT(right));
        }
    }

    TRACE("rc=%d, Result:\n", (int) rc);
    dump_Variant(result);
    return rc;
}

/**********************************************************************
 *              VarMul [OLEAUT32.156]
 *
 */
HRESULT WINAPI VarMul(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;
    VARTYPE lvt,rvt,resvt;
    VARIANT lv,rv;
    BOOL found;

    TRACE("left: ");dump_Variant(left);
    TRACE("right: ");dump_Variant(right);

    VariantInit(&lv);VariantInit(&rv);
    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;
    found = FALSE;resvt=VT_VOID;
    if (((1<<lvt) | (1<<rvt)) & ((1<<VT_R4)|(1<<VT_R8))) {
	found = TRUE;
	resvt = VT_R8;
    }
    if (!found && (((1<<lvt) | (1<<rvt)) & ((1<<VT_I1)|(1<<VT_I2)|(1<<VT_UI1)|(1<<VT_UI2)|(1<<VT_I4)|(1<<VT_UI4)|(1<<VT_INT)|(1<<VT_UINT)))) {
	found = TRUE;
	resvt = VT_I4;
    }
    if (!found) {
	FIXME("can't expand vt %d vs %d to a target type.\n",lvt,rvt);
	return E_FAIL;
    }
    rc = VariantChangeType(&lv, left, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(left),resvt);
	return rc;
    }
    rc = VariantChangeType(&rv, right, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(right),resvt);
	return rc;
    }
    switch (resvt) {
    case VT_R8:
	V_VT(result) = resvt;
	V_R8(result) = V_R8(&lv) * V_R8(&rv);
	rc = S_OK;
	break;
    case VT_I4:
	V_VT(result) = resvt;
	V_I4(result) = V_I4(&lv) * V_I4(&rv);
	rc = S_OK;
	break;
    }
    TRACE("rc=%d, Result:\n", (int) rc);
    dump_Variant(result);
    return rc;
}

/**********************************************************************
 *              VarDiv [OLEAUT32.143]
 *
 */
HRESULT WINAPI VarDiv(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;
    VARTYPE lvt,rvt,resvt;
    VARIANT lv,rv;
    BOOL found;

    TRACE("left: ");dump_Variant(left);
    TRACE("right: ");dump_Variant(right);

    VariantInit(&lv);VariantInit(&rv);
    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;
    found = FALSE;resvt = VT_VOID;
    if (((1<<lvt) | (1<<rvt)) & ((1<<VT_R4)|(1<<VT_R8))) {
	found = TRUE;
	resvt = VT_R8;
    }
    if (!found && (((1<<lvt) | (1<<rvt)) & ((1<<VT_I1)|(1<<VT_I2)|(1<<VT_UI1)|(1<<VT_UI2)|(1<<VT_I4)|(1<<VT_UI4)|(1<<VT_INT)|(1<<VT_UINT)))) {
	found = TRUE;
	resvt = VT_I4;
    }
    if (!found) {
	FIXME("can't expand vt %d vs %d to a target type.\n",lvt,rvt);
	return E_FAIL;
    }
    rc = VariantChangeType(&lv, left, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(left),resvt);
	return rc;
    }
    rc = VariantChangeType(&rv, right, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(right),resvt);
	return rc;
    }
    switch (resvt) {
    case VT_R8:
	V_VT(result) = resvt;
	V_R8(result) = V_R8(&lv) / V_R8(&rv);
	rc = S_OK;
	break;
    case VT_I4:
	V_VT(result) = resvt;
	V_I4(result) = V_I4(&lv) / V_I4(&rv);
	rc = S_OK;
	break;
    }
    TRACE("rc=%d, Result:\n", (int) rc);
    dump_Variant(result);
    return rc;
}

/**********************************************************************
 *              VarSub [OLEAUT32.159]
 *
 */
HRESULT WINAPI VarSub(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;
    VARTYPE lvt,rvt,resvt;
    VARIANT lv,rv;
    BOOL found;

    TRACE("left: ");dump_Variant(left);
    TRACE("right: ");dump_Variant(right);

    VariantInit(&lv);VariantInit(&rv);
    lvt = V_VT(left)&VT_TYPEMASK;
    rvt = V_VT(right)&VT_TYPEMASK;
    found = FALSE;resvt = VT_VOID;
    if (((1<<lvt) | (1<<rvt)) & ((1<<VT_R4)|(1<<VT_R8))) {
	found = TRUE;
	resvt = VT_R8;
    }
    if (!found && (((1<<lvt) | (1<<rvt)) & ((1<<VT_I1)|(1<<VT_I2)|(1<<VT_UI1)|(1<<VT_UI2)|(1<<VT_I4)|(1<<VT_UI4)|(1<<VT_INT)|(1<<VT_UINT)))) {
	found = TRUE;
	resvt = VT_I4;
    }
    if (!found) {
	FIXME("can't expand vt %d vs %d to a target type.\n",lvt,rvt);
	return E_FAIL;
    }
    rc = VariantChangeType(&lv, left, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(left),resvt);
	return rc;
    }
    rc = VariantChangeType(&rv, right, 0, resvt);
    if (FAILED(rc)) {
	FIXME("Could not convert 0x%x to %d?\n",V_VT(right),resvt);
	return rc;
    }
    switch (resvt) {
    case VT_R8:
	V_VT(result) = resvt;
	V_R8(result) = V_R8(&lv) - V_R8(&rv);
	rc = S_OK;
	break;
    case VT_I4:
	V_VT(result) = resvt;
	V_I4(result) = V_I4(&lv) - V_I4(&rv);
	rc = S_OK;
	break;
    }
    TRACE("rc=%d, Result:\n", (int) rc);
    dump_Variant(result);
    return rc;
}

/**********************************************************************
 *              VarOr [OLEAUT32.157]
 *
 */
HRESULT WINAPI VarOr(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    HRESULT rc = E_FAIL;

    TRACE("Left Var:\n");
    dump_Variant(left);
    TRACE("Right Var:\n");
    dump_Variant(right);

    if ((V_VT(left)&VT_TYPEMASK) == VT_BOOL &&
        (V_VT(right)&VT_TYPEMASK) == VT_BOOL) {

        V_VT(result) = VT_BOOL;
        if (V_BOOL(left) || V_BOOL(right)) {
            V_BOOL(result) = VARIANT_TRUE;
        } else {
            V_BOOL(result) = VARIANT_FALSE;
        }
        rc = S_OK;

    } else {
        /* Integers */
        BOOL         lOk        = TRUE;
        BOOL         rOk        = TRUE;
        LONGLONG     lVal = -1;
        LONGLONG     rVal = -1;
        LONGLONG     res  = -1;
        int          resT = 0; /* Testing has shown I2 & I2 == I2, all else
                                  becomes I4, even unsigned ints (incl. UI2) */

        lOk = TRUE;
        switch (V_VT(left)&VT_TYPEMASK) {
        case VT_I1   : lVal = V_UNION(left,cVal);  resT=VT_I4; break;
        case VT_I2   : lVal = V_UNION(left,iVal);  resT=VT_I2; break;
        case VT_I4   : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_INT  : lVal = V_UNION(left,lVal);  resT=VT_I4; break;
        case VT_UI1  : lVal = V_UNION(left,bVal);  resT=VT_I4; break;
        case VT_UI2  : lVal = V_UNION(left,uiVal); resT=VT_I4; break;
        case VT_UI4  : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        case VT_UINT : lVal = V_UNION(left,ulVal); resT=VT_I4; break;
        default: lOk = FALSE;
        }

        rOk = TRUE;
        switch (V_VT(right)&VT_TYPEMASK) {
        case VT_I1   : rVal = V_UNION(right,cVal);  resT=VT_I4; break;
        case VT_I2   : rVal = V_UNION(right,iVal);  resT=max(VT_I2, resT); break;
        case VT_I4   : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_INT  : rVal = V_UNION(right,lVal);  resT=VT_I4; break;
        case VT_UI1  : rVal = V_UNION(right,bVal);  resT=VT_I4; break;
        case VT_UI2  : rVal = V_UNION(right,uiVal); resT=VT_I4; break;
        case VT_UI4  : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        case VT_UINT : rVal = V_UNION(right,ulVal); resT=VT_I4; break;
        default: rOk = FALSE;
        }

        if (lOk && rOk) {
            res = (lVal | rVal);
            V_VT(result) = resT;
            switch (resT) {
            case VT_I2   : V_UNION(result,iVal)  = res; break;
            case VT_I4   : V_UNION(result,lVal)  = res; break;
            default:
                FIXME("Unexpected result variant type %x\n", resT);
                V_UNION(result,lVal)  = res;
            }
            rc = S_OK;

        } else {
            FIXME("unimplemented part\n");
        }
    }

    TRACE("rc=%d, Result:\n", (int) rc);
    dump_Variant(result);
    return rc;
}

/**********************************************************************
 * VarAbs [OLEAUT32.168]
 *
 * Convert a variant to its absolute value.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the absolute value of pVarIn.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - This function does not process by-reference variants.
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *| VT_BOOL          VT_I2
 *| VT_BSTR          VT_R8
 *| (All others)     Unchanged
 */
HRESULT WINAPI VarAbs(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    VARIANT varIn;
    HRESULT hRet = S_OK;

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

    if (V_ISARRAY(pVarIn) || V_VT(pVarIn) == VT_UNKNOWN ||
        V_VT(pVarIn) == VT_DISPATCH || V_VT(pVarIn) == VT_RECORD ||
        V_VT(pVarIn) == VT_ERROR)
        return DISP_E_TYPEMISMATCH;

    *pVarOut = *pVarIn; /* Shallow copy the value, and invert it if needed */

#define ABS_CASE(typ,min) \
    case VT_##typ: if (V_##typ(pVarIn) == min) hRet = DISP_E_OVERFLOW; \
                  else if (V_##typ(pVarIn) < 0) V_##typ(pVarOut) = -V_##typ(pVarIn); \
                  break

    switch (V_VT(pVarIn))
    {
    ABS_CASE(I1,I1_MIN);
    case VT_BOOL:
        V_VT(pVarOut) = VT_I2;
        /* BOOL->I2, Fall through ... */
    ABS_CASE(I2,I2_MIN);
    case VT_INT:
    ABS_CASE(I4,I4_MIN);
    ABS_CASE(I8,I8_MIN);
    ABS_CASE(R4,R4_MIN);
    case VT_BSTR:
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(&varIn));
        if (FAILED(hRet))
            break;
        V_VT(pVarOut) = VT_R8;
        pVarIn = &varIn;
        /* Fall through ... */
    case VT_DATE:
    ABS_CASE(R8,R8_MIN);
    case VT_CY:
        /* FIXME: Waiting for this function to be implemented */
#if 0
        hRet = VarCyAbs(V_CY(pVarIn), & V_CY(pVarOut));
#endif
        break;
    case VT_DECIMAL:
        DEC_SIGN(&V_DECIMAL(pVarOut)) &= ~DECIMAL_NEG;
        break;
    case VT_UI1:
    case VT_UI2:
    case VT_UINT:
    case VT_UI4:
    case VT_UI8:
    case VT_EMPTY:
    case VT_NULL:
        /* No-Op */
        break;
    default:
        hRet = DISP_E_BADVARTYPE;
    }

    return hRet;
}

/**********************************************************************
 *              VarNot [OLEAUT32.174]
 *
 * Perform a not operation on a variant.
 *
 * PARAMS
 *  pVarIn  [I] Source variant
 *  pVarOut [O] Destination for converted value
 *
 * RETURNS
 *  Success: S_OK. pVarOut contains the converted value.
 *  Failure: An HRESULT error code indicating the error.
 *
 * NOTES
 *  - Strictly speaking, this function performs a bitwise ones compliment
 *    on the variants value (after possibly converting to VT_I4, see below).
 *    This only behaves like a boolean not operation if the value in
 *    pVarIn is either VARIANT_TRUE or VARIANT_FALSE and the type is signed.
 *  - To perform a genuine not operation, convert the variant to a VT_BOOL
 *    before calling this function.
 *  - This function does not process by-reference variants.
 *  - The type of the value stored in pVarOut depends on the type of pVarIn,
 *    according to the following table:
 *| Input Type       Output Type
 *| ----------       -----------
 *| VT_R4            VT_I4
 *| VT_R8            VT_I4
 *| VT_BSTR          VT_I4
 *| VT_DECIMAL       VT_I4
 *| VT_CY            VT_I4
 *| (All others)     Unchanged
 */
HRESULT WINAPI VarNot(LPVARIANT pVarIn, LPVARIANT pVarOut)
{
    VARIANT varIn;
    HRESULT hRet = S_OK;

    TRACE("(%p->(%s%s),%p)\n", pVarIn, debugstr_VT(pVarIn),
          debugstr_VF(pVarIn), pVarOut);

    V_VT(pVarOut) = V_VT(pVarIn);

    switch (V_VT(pVarIn))
    {
    case VT_I1:  V_I1(pVarOut) = ~V_I1(pVarIn); break;
    case VT_UI1: V_UI1(pVarOut) = ~V_UI1(pVarIn); break;
    case VT_BOOL:
    case VT_I2:  V_I2(pVarOut) = ~V_I2(pVarIn); break;
    case VT_UI2: V_UI2(pVarOut) = ~V_UI2(pVarIn); break;
    case VT_DECIMAL:
        /* FIXME  Waiting for this function to be implemented */
/*        hRet = VarI4FromDec(&V_DECIMAL(pVarIn), &V_I4(&varIn)); */
        hRet = DISP_E_OVERFLOW;
        if (FAILED(hRet))
            break;
        pVarIn = &varIn;
        V_VT(pVarOut) = VT_I4;
        /* Fall through ... */
    case VT_INT:
    case VT_I4:  V_I4(pVarOut) = ~V_I4(pVarIn); break;
    case VT_UINT:
    case VT_UI4: V_UI4(pVarOut) = ~V_UI4(pVarIn); break;
    case VT_I8:  V_I8(pVarOut) = ~V_I8(pVarIn); break;
    case VT_UI8: V_UI8(pVarOut) = ~V_UI8(pVarIn); break;
    case VT_R4:
        hRet = VarI4FromR4(V_R4(pVarIn), &V_I4(pVarOut));
        V_I4(pVarOut) = ~V_I4(pVarOut);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_BSTR:
        hRet = VarR8FromStr(V_BSTR(pVarIn), LOCALE_USER_DEFAULT, 0, &V_R8(&varIn));
        if (FAILED(hRet))
            break;
        pVarIn = &varIn;
        /* Fall through ... */
    case VT_DATE:
    case VT_R8:
        hRet = VarI4FromR8(V_R8(pVarIn), &V_I4(pVarOut));
        V_I4(pVarOut) = ~V_I4(pVarOut);
        V_VT(pVarOut) = VT_I4;
        break;
    case VT_CY:
        /* FIXME: */
        break;
    case VT_EMPTY:
    case VT_NULL:
        /* No-Op */
        break;
    default:
        if (V_TYPE(pVarIn) == VT_CLSID || /* VT_CLSID is a special case */
            FAILED(VARIANT_ValidateType(V_VT(pVarIn))))
            hRet = DISP_E_BADVARTYPE;
        else
            hRet = DISP_E_TYPEMISMATCH;
    }
    if (FAILED(hRet))
      V_VT(pVarOut) = VT_EMPTY;

    return hRet;
}

/**********************************************************************
 *              VarCyMulI4 [OLEAUT32.304]
 * Multiply currency value by integer
 */
HRESULT WINAPI VarCyMulI4(CY cyIn, LONG mulBy, CY *pcyOut) {

    double cyVal = 0;
    HRESULT rc = S_OK;

    rc = VarR8FromCy(cyIn, &cyVal);
    if (rc == S_OK) {
        rc = VarCyFromR8((cyVal * (double) mulBy), pcyOut);
        TRACE("Multiply %f by %ld = %f [%ld,%lu]\n", cyVal, mulBy, (cyVal * (double) mulBy),
                    pcyOut->s.Hi, pcyOut->s.Lo);
    }
    return rc;
}

/**********************************************************************
 *              VarMod [OLEAUT32.154]
 *
 */
HRESULT WINAPI VarMod(LPVARIANT left, LPVARIANT right, LPVARIANT result)
{
    FIXME("%p %p %p\n", left, right, result);
    return E_FAIL;
}
