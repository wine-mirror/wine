/*
 * VARIANT test program
 *
 * Copyright 1998 Jean-Claude Cote
 *
 * The purpose of this program is validate the implementation
 * of the APIs related to VARIANTs. The validation is done
 * by comparing the results given by the Windows implementation
 * versus the Wine implementation.
 *
 * This program tests the creation/coercion/destruction of VARIANTs.
 *
 * The program does not currently test any API that takes
 * arguments of type: IDispatch, IUnknown, DECIMAL, CURRENCY.
 *
 * Since the purpose of this program is to compare the results
 * from Windows and Wine it is written so that with a simple
 * define it can be compiled either in Windows or Linux.
 *
 *
 * NOTES
 *	 - The Variant structure in Windows has a non-named union. This means
 *	 the member of the union are accessible simply by doing pVariant->pfltVal.
 *	 With gcc it is not possible to have non-named union so it has been named
 *	 'u'.  So it's members are accessible using this name like so
 *	 pVariant->u.pfltVal.  So if this program is compiled in Windows
 *	 the references to 'u' will need to be take out of this file.
 *
 *	 - Also the printf is a little different so the format specifiers may
 *	 need to be tweaked if this file is compile in Windows.
 *	 Printf is also different in that it continues printing numbers
 *	 even after there is no more significative digits left to print.  These
 *	 number are garbage and in windows they are set to zero but not
 *	 on Linux.
 *
 *	 - The VarDateFromStr is not implemented yet.
 *
 *	 - The date and floating point format may not be the exact same format has the one in
 *	 windows depending on what the Internatinal setting are in windows.
 *
 */



#include <stdio.h>
#include <malloc.h>
#include <windows.h>
#include <winerror.h>
#include <winnt.h>

#include <oleauto.h>

#include <math.h>
#include <float.h>
#include <time.h>

#include <windef.h>

#ifdef __unix__
#include <debugtools.h>
extern LPWSTR HEAP_strdupAtoW( HANDLE heap, DWORD flags, LPCSTR str );
#endif


static const int MAX_BUFFER = 1024;


#ifndef __unix__
char* WtoA( OLECHAR* p )
{
	int i = 0;
	char* pc = (char *)malloc( MAX_BUFFER*sizeof(char) );
	pc[0] = '\"';
	pc++;
	i = wcstombs( pc, p, MAX_BUFFER );
	if( i < MAX_BUFFER-1 )
	{
		pc[i] = '\"';
		pc[i+1] = '\0';
	}
	pc--;
	return pc;
}

OLECHAR* AtoW( char* p )
{
	int i = 0;
	OLECHAR* pwc = (OLECHAR *)malloc( MAX_BUFFER*sizeof(OLECHAR) );
	i = mbstowcs( pwc, p, MAX_BUFFER );
	return pwc;
}
#else 
char* WtoA( OLECHAR* p )
{
	return debugstr_wn( p, MAX_BUFFER );
}
OLECHAR* AtoW( char* p )
{
	return HEAP_strdupAtoW( GetProcessHeap(), 0, p );
}
#endif


int PASCAL WinMain (HINSTANCE inst, HINSTANCE prev, LPSTR cmdline, int show)
{
	VARIANTARG va;
	VARIANTARG vb;
	VARIANTARG vc;
	VARIANTARG vd;
	VARIANTARG ve;

	int theInt = 0;
	int* pInt = &theInt;
	VARIANT_BOOL b = 0;
	VARIANT_BOOL* pBool = &b;
	unsigned short uShort = 0;
	unsigned short* pUShort = &uShort;
	unsigned long uLong = 0;
	unsigned long* pULong = &uLong;
	CHAR theChar;
	CHAR* pChar = &theChar;
	BYTE byte;
	BYTE* pByte = &byte;
	short s = 0;
	short* pShort = &s;
	long Long = 0;
	long* pLong = &Long;
	float f = 0;
	float* pFloat = &f;
	double d = 0;
	double* pDouble = &d;
	
	unsigned short i = 0;
	HRESULT res = 0;
	BSTR bstr = NULL;
	int olePtrIndex = 0;
	int nOlePtrs = 120;
	OLECHAR* pOleChar[120];

	pOleChar[0] = AtoW( "-2" );
	pOleChar[1] = AtoW( "-1" );
	pOleChar[2] = AtoW( "-0.51" );
	pOleChar[3] = AtoW( "-0.5" );
	pOleChar[4] = AtoW( "-0.49" );
	pOleChar[5] = AtoW( "-0.0" );
	pOleChar[6] = AtoW( "0.0" );
	pOleChar[7] = AtoW( "0.49" );
	pOleChar[8] = AtoW( "0.5" );
	pOleChar[9] = AtoW( "0.51" );
	pOleChar[10] = AtoW( "1" );
	pOleChar[11] = AtoW( "127" );
	pOleChar[12] = AtoW( "128" );
	pOleChar[13] = AtoW( "129" );
	pOleChar[14] = AtoW( "255" );
	pOleChar[15] = AtoW( "256" );
	pOleChar[16] = AtoW( "257" );
	pOleChar[17] = AtoW( "32767" );
	pOleChar[18] = AtoW( "32768" );
	pOleChar[19] = AtoW( "-32768" );
	pOleChar[20] = AtoW( "-32769" );
	pOleChar[21] = AtoW( "16777216" );
	pOleChar[22] = AtoW( "16777217" );
	pOleChar[23] = AtoW( "-16777216" );
	pOleChar[24] = AtoW( "16777217" );
	pOleChar[25] = AtoW( "2147483647" );
	pOleChar[26] = AtoW( "2147483648" );
	pOleChar[27] = AtoW( "-2147483647" );
	pOleChar[28] = AtoW( "-2147483648" );

	pOleChar[29] = AtoW( "" );
	pOleChar[30] = AtoW( " " );
	pOleChar[31] = AtoW( "1F" );
	pOleChar[32] = AtoW( "1G" );
	pOleChar[33] = AtoW( " 1 " );
	pOleChar[34] = AtoW( " 1 2 " );
	pOleChar[35] = AtoW( "1,2,3" );
	pOleChar[36] = AtoW( "1 2 3" );
	pOleChar[37] = AtoW( "1,2, 3" );
	pOleChar[38] = AtoW( "1;2;3" );
	pOleChar[39] = AtoW( "1.2.3" );

	pOleChar[40] = AtoW( "0." );
	pOleChar[41] = AtoW( ".0" );
	pOleChar[42] = AtoW( "0.1E12" );
	pOleChar[43] = AtoW( "2.4,E1" );
	pOleChar[44] = AtoW( "	+3.2,E1" );
	pOleChar[45] = AtoW( "4E2.5" );
	pOleChar[46] = AtoW( "	2E+2" );
	pOleChar[47] = AtoW( "1 E+2" );
	pOleChar[48] = AtoW( "." );
	pOleChar[49] = AtoW( ".E2" );
	pOleChar[50] = AtoW( "1000000000000000000000000000000000000000000000000000000000000000" );
	pOleChar[51] = AtoW( "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001" );
	pOleChar[52] = AtoW( "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001" );
	pOleChar[53] = AtoW( "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001" );
	pOleChar[54] = AtoW( "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001" );
	pOleChar[55] = AtoW( "65535" );
	pOleChar[56] = AtoW( "65535.5" );
	pOleChar[57] = AtoW( "65536" );
	pOleChar[58] = AtoW( "4294967295" );
	pOleChar[59] = AtoW( "4294967296" );
	
	pOleChar[60] = AtoW( "1 January 99" );
	pOleChar[61] = AtoW( "December 31, 2078" );
	pOleChar[62] = AtoW( "January 1, 1900" );
	pOleChar[63] = AtoW( "January 2 1900" );
	pOleChar[64] = AtoW( "11.11.1999" );
	pOleChar[65] = AtoW( "11/11/1999" );
	pOleChar[66] = AtoW( " 11 / 11 / 1999" );
	pOleChar[67] = AtoW( "11/11/1999:11:11:1134" );
	pOleChar[68] = AtoW( "11/11/1999 11:11:11:1" );
	pOleChar[69] = AtoW( "\t1999/\t11/21 11 :11:11am" );

	pOleChar[70] = AtoW( "11/11/1999 11:11:11Am" );
	pOleChar[71] = AtoW( "11/11/1999 11:11:11PM" );
	pOleChar[72] = AtoW( "11/11/199911:11:11PM" );
	pOleChar[73] = AtoW( "11/11/1999 0:0:11am" );
	pOleChar[74] = AtoW( "11/11/1999 11,11:11am" );
	pOleChar[75] = AtoW( "11/11/1999 11:11:11am" );
	pOleChar[76] = AtoW( "11/11/1999 11/11:11am" );
	pOleChar[77] = AtoW( "11/11/1999 11:11AM" );
	pOleChar[78] = AtoW( "11/11/1999 1AM" );
	pOleChar[79] = AtoW( "11/11/1999 0AM" );

	pOleChar[80] = AtoW( "11/11/1999 11:11:11" );
	pOleChar[81] = AtoW( "11/13/1999 0AM" );
	pOleChar[82] = AtoW( "13/13/1999 0AM" );
	pOleChar[83] = AtoW( "13/11/1999 0AM" );
	pOleChar[84] = AtoW( "11/33/1999 0AM" );
	pOleChar[85] = AtoW( "11/11/1999 AM" );
	pOleChar[86] = AtoW( "1/1/0 0AM" );
	pOleChar[87] = AtoW( "1/1/-1 0AM" );
	pOleChar[88] = AtoW( "1999 January 3 9AM" );
	pOleChar[89] = AtoW( "1 January 1999 11AM" );

	pOleChar[90] = AtoW( "4AM 11/11/1999" );
	pOleChar[91] = AtoW( "4:22 11/11/1999 AM" );
	pOleChar[92] = AtoW( " 1 1 /11/1999" );
	pOleChar[93] = AtoW( "11-11/1999 11:11:11.12AM" );
	pOleChar[94] = AtoW( "1999 January 3, 9AM" );
	pOleChar[95] = AtoW( "December, 31, 2078" );
	pOleChar[96] = AtoW( "December, 31, 2078," );
	pOleChar[97] = AtoW( "December, 31 2078" );
	pOleChar[98] = AtoW( "11/99" );
	pOleChar[99] = AtoW( "11-1999" );

	pOleChar[100] = AtoW( "true" );
	pOleChar[101] = AtoW( "True" );
	pOleChar[102] = AtoW( "TRue" );
	pOleChar[103] = AtoW( "TRUE" );
	pOleChar[104] = AtoW( " TRUE" );
	pOleChar[105] = AtoW( "FALSE " );
	pOleChar[106] = AtoW( "False" );
	pOleChar[107] = AtoW( "JustSomeText" );
	pOleChar[108] = AtoW( "Just Some Text" );
	pOleChar[109] = AtoW( "" );

	pOleChar[110] = AtoW( "1.5" );
	pOleChar[111] = AtoW( "2.5" );
	pOleChar[112] = AtoW( "3.5" );
	pOleChar[113] = AtoW( "4.5" );
	pOleChar[114] = AtoW( "" );
	pOleChar[115] = AtoW( "" );
	pOleChar[116] = AtoW( "" );
	pOleChar[117] = AtoW( "" );
	pOleChar[118] = AtoW( "" );
	pOleChar[119] = AtoW( "" );


	/* Start testing the Low-Level API ( the coercions )
	 */


	/* unsigned char from...
	 */
	printf( "\n\n======== Testing VarUI1FromXXX ========\n");
	
	/* res = VarUI1FromI2( 0, NULL );
	 */
	printf( "VarUI1FromI2: passing in NULL as return val makes it crash, %X\n", (unsigned int) res );

	res = VarUI1FromStr( NULL, 0, 0, pByte );
	printf( "VarUI1FromStr: passing in NULL as param: %X\n", (unsigned int) res );

	res = VarUI1FromI2( 0, pByte );
	printf( "VarUI1FromI2: 0, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromI2( 69, pByte );
	printf( "VarUI1FromI2: 69, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromI2( 70, pByte );
	printf( "VarUI1FromI2: 70, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromI2( 128, pByte );
	printf( "VarUI1FromI2: 128, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromI2( 255, pByte );
	printf( "VarUI1FromI2: 255, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromI2( 256, pByte );
	printf( "VarUI1FromI2: 256, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromI2( 257, pByte );
	printf( "VarUI1FromI2: 257, %X, %X\n", *pByte, (unsigned int) res );

	res = VarUI1FromR8( 0.0, pByte );
	printf( "VarUI1FromR8: 0.0, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( 69.33, pByte );
	printf( "VarUI1FromR8: 69.33, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( 69.66, pByte );
	printf( "VarUI1FromR8: 69.66, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( -69.33, pByte );
	printf( "VarUI1FromR8: -69.33, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( -69.66, pByte );
	printf( "VarUI1FromR8: -69.66, %X, %X\n", *pByte, (unsigned int) res );

	res = VarUI1FromR8( -0.5, pByte );
	printf( "VarUI1FromR8: -0.5, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( -0.51, pByte );
	printf( "VarUI1FromR8: -0.51, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( -0.49, pByte );
	printf( "VarUI1FromR8: -0.49, %X, %X\n", *pByte, (unsigned int) res );

	res = VarUI1FromR8( 0.5, pByte );
	printf( "VarUI1FromR8: 0.5, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( 0.51, pByte );
	printf( "VarUI1FromR8: 0.51, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromR8( 0.49, pByte );
	printf( "VarUI1FromR8: 0.49, %X, %X\n", *pByte, (unsigned int) res );

	res = VarUI1FromDate( 0.0, pByte );
	printf( "VarUI1FromDate: 0.0, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromDate( 69.33, pByte );
	printf( "VarUI1FromDate: 69.33, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromDate( 69.66, pByte );
	printf( "VarUI1FromDate: 69.66, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromDate( -69.33, pByte );
	printf( "VarUI1FromDate: -69.33, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromDate( -69.66, pByte );
	printf( "VarUI1FromDate: -69.66, %X, %X\n", *pByte, (unsigned int) res );

	res = VarUI1FromBool( VARIANT_TRUE, pByte );
	printf( "VarUI1FromBool: VARIANT_TRUE, %X, %X\n", *pByte, (unsigned int) res );
	res = VarUI1FromBool( VARIANT_FALSE, pByte );
	printf( "VarUI1FromBool: VARIANT_FALSE, %X, %X\n", *pByte, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarUI1FromStr( pOleChar[olePtrIndex], 0, 0, pByte );
		printf( "VarUI1FromStr: %s, %X, %X\n", WtoA(pOleChar[olePtrIndex]), *pByte, (unsigned int) res );
	}


	/* unsigned short from ...
	 */
	printf( "\n\n======== Testing VarUI2FromXXX ========\n");
	
	res = VarUI2FromI2( -1, &i );
	printf( "VarUI2FromI2: -1, %d, %X\n", i, (unsigned int) res );

	/* res = VarUI2FromI2( 0, NULL );
	 */
	printf( "VarUI2FromI2: passing in NULL as return val makes it crash, %X\n", (unsigned int) res );

	res = VarUI2FromStr( NULL, 0, 0, pUShort );
	printf( "VarUI2FromStr: passing in NULL as param: %X\n", (unsigned int) res );

	res = VarUI2FromI2( 0, pUShort );
	printf( "VarUI2FromI2: 0, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromI2( 69, pUShort );
	printf( "VarUI2FromI2: 69, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromI2( 70, pUShort );
	printf( "VarUI2FromI2: 70, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromI2( 128, pUShort );
	printf( "VarUI2FromI2: 128, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromI4( 65535, pUShort );
	printf( "VarUI2FromI4: 65535, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromI4( 65536, pUShort );
	printf( "VarUI2FromI4: 65536, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromI4( 65537, pUShort );
	printf( "VarUI2FromI4: 65537, %u, %X\n", *pUShort, (unsigned int) res );

	res = VarUI2FromR8( 0.0, pUShort );
	printf( "VarUI2FromR8: 0.0, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( 69.33, pUShort );
	printf( "VarUI2FromR8: 69.33, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( 69.66, pUShort );
	printf( "VarUI2FromR8: 69.66, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( -69.33, pUShort );
	printf( "VarUI2FromR8: -69.33, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( -69.66, pUShort );
	printf( "VarUI2FromR8: -69.66, %u, %X\n", *pUShort, (unsigned int) res );

	res = VarUI2FromR8( -0.5, pUShort );
	printf( "VarUI2FromR8: -0.5, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( -0.51, pUShort );
	printf( "VarUI2FromR8: -0.51, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( -0.49, pUShort );
	printf( "VarUI2FromR8: -0.49, %u, %X\n", *pUShort, (unsigned int) res );

	res = VarUI2FromR8( 0.5, pUShort );
	printf( "VarUI2FromR8: 0.5, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( 0.51, pUShort );
	printf( "VarUI2FromR8: 0.51, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromR8( 0.49, pUShort );
	printf( "VarUI2FromR8: 0.49, %u, %X\n", *pUShort, (unsigned int) res );

	res = VarUI2FromDate( 0.0, pUShort );
	printf( "VarUI2FromDate: 0.0, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromDate( 69.33, pUShort );
	printf( "VarUI2FromDate: 69.33, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromDate( 69.66, pUShort );
	printf( "VarUI2FromDate: 69.66, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromDate( -69.33, pUShort );
	printf( "VarUI2FromDate: -69.33, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromDate( -69.66, pUShort );
	printf( "VarUI2FromDate: -69.66, %u, %X\n", *pUShort, (unsigned int) res );

	res = VarUI2FromBool( VARIANT_TRUE, pUShort );
	printf( "VarUI2FromBool: VARIANT_TRUE, %u, %X\n", *pUShort, (unsigned int) res );
	res = VarUI2FromBool( VARIANT_FALSE, pUShort );
	printf( "VarUI2FromBool: VARIANT_FALSE, %u, %X\n", *pUShort, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarUI2FromStr( pOleChar[olePtrIndex], 0, 0, pUShort );
		printf( "VarUI2FromStr: %s, %u, %X\n", WtoA(pOleChar[olePtrIndex]), (int)*pUShort, (unsigned int) res );
	}

	/* unsigned long from ...
	 */
	printf( "\n\n======== Testing VarUI4FromXXX ========\n");
	
	/*res = VarUI4FromI2( 0, NULL );
	 */
	printf( "VarUI4FromI2: passing in NULL as return val makes it crash, %X\n", (unsigned int) res );

	res = VarUI4FromStr( NULL, 0, 0, pULong );
	printf( "VarUI4FromStr: passing in NULL as param: %X\n", (unsigned int) res );

	res = VarUI4FromI2( 0, pULong );
	printf( "VarUI4FromI2: 0, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromI2( 69, pULong );
	printf( "VarUI4FromI2: 69, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromI2( 70, pULong );
	printf( "VarUI4FromI2: 70, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromI2( 128, pULong );
	printf( "VarUI4FromI2: 128, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromI2( 255, pULong );
	printf( "VarUI4FromI2: 255, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( 4294967295.0, pULong );
	printf( "VarUI4FromR8: 4294967295, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( 4294967296.0, pULong );
	printf( "VarUI4FromR8: 4294967296, %lu, %X\n", *pULong, (unsigned int) res );

	res = VarUI4FromR8( 0.0, pULong );
	printf( "VarUI4FromR8: 0.0, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( 69.33, pULong );
	printf( "VarUI4FromR8: 69.33, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( 69.66, pULong );
	printf( "VarUI4FromR8: 69.66, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( -69.33, pULong );
	printf( "VarUI4FromR8: -69.33, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( -69.66, pULong );
	printf( "VarUI4FromR8: -69.66, %lu, %X\n", *pULong, (unsigned int) res );

	res = VarUI4FromR8( -0.5, pULong );
	printf( "VarUI4FromR8: -0.5, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( -0.51, pULong );
	printf( "VarUI4FromR8: -0.51, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( -0.49, pULong );
	printf( "VarUI4FromR8: -0.49, %lu, %X\n", *pULong, (unsigned int) res );

	res = VarUI4FromR8( 0.5, pULong );
	printf( "VarUI4FromR8: 0.5, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( 0.51, pULong );
	printf( "VarUI4FromR8: 0.51, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromR8( 0.49, pULong );
	printf( "VarUI4FromR8: 0.49, %lu, %X\n", *pULong, (unsigned int) res );

	res = VarUI4FromDate( 0.0, pULong );
	printf( "VarUI4FromDate: 0.0, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromDate( 69.33, pULong );
	printf( "VarUI4FromDate: 69.33, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromDate( 69.66, pULong );
	printf( "VarUI4FromDate: 69.66, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromDate( -69.33, pULong );
	printf( "VarUI4FromDate: -69.33, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromDate( -69.66, pULong );
	printf( "VarUI4FromDate: -69.66, %lu, %X\n", *pULong, (unsigned int) res );

	res = VarUI4FromBool( VARIANT_TRUE, pULong );
	printf( "VarUI4FromBool: VARIANT_TRUE, %lu, %X\n", *pULong, (unsigned int) res );
	res = VarUI4FromBool( VARIANT_FALSE, pULong );
	printf( "VarUI4FromBool: VARIANT_FALSE, %lu, %X\n", *pULong, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarUI4FromStr( pOleChar[olePtrIndex], 0, 0, pULong );
		printf( "VarUI4FromStr: %s, %lu, %X\n", WtoA(pOleChar[olePtrIndex]), *pULong, (unsigned int) res );
	}

	/* CHAR from ...
	 */
	printf( "\n\n======== Testing VarI1FromXXX ========\n");

	res = VarI1FromBool( VARIANT_TRUE, pByte );
	printf( "VarI1FromBool: VARIANT_TRUE, %d, %X\n", *pByte, (unsigned int) res );

	res = VarI1FromBool( VARIANT_TRUE, pChar );
	printf( "VarI1FromBool: VARIANT_TRUE, %d, %X\n", *pChar, (unsigned int) res );

	res = VarI1FromBool( VARIANT_FALSE, pChar );
	printf( "VarI1FromBool: VARIANT_FALSE, %d, %X\n", *pChar, (unsigned int) res );

	res = VarI1FromUI1( (unsigned char)32767, pChar );
	printf( "VarI1FromUI1: 32767, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromUI1( (unsigned char)65535, pChar );
	printf( "VarI1FromUI1: 65535, %d, %X\n", *pChar, (unsigned int) res );

	res = VarI1FromI4( 32767, pChar );
	printf( "VarI1FromI4: 32767, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromI4( 32768, pChar );
	printf( "VarI1FromI4: 32768, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromI4( -32768, pChar );
	printf( "VarI1FromI4: -32768, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromI4( -32769, pChar );
	printf( "VarI1FromI4: -32769, %d, %X\n", *pChar, (unsigned int) res );

	res = VarI1FromR8( 69.33, pChar );
	printf( "VarI1FromR8: 69.33, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromR8( 69.66, pChar );
	printf( "VarI1FromR8: 69.66, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromR8( -69.33, pChar );
	printf( "VarI1FromR8: -69.33, %d, %X\n", *pChar, (unsigned int) res );
	res = VarI1FromR8( -69.66, pChar );
	printf( "VarI1FromR8: -69.66, %d, %X\n", *pChar, (unsigned int) res );

	res = VarI1FromDate( -69.66, pChar );
	printf( "VarI1FromDate: -69.66, %d, %X\n", *pChar, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarI1FromStr( pOleChar[olePtrIndex], 0, 0, pChar );
		printf( "VarI1FromStr: %s, %d, %X\n", WtoA(pOleChar[olePtrIndex]), *pChar, (unsigned int) res );
	}

	/* short from ...
	 */
	printf( "\n\n======== Testing VarI2FromXXX ========\n");

	res = VarI2FromUI2( 32767, pShort );
	printf( "VarI2FromUI2: 32767, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromUI2( 65535, pShort );
	printf( "VarI2FromUI2: 65535, %d, %X\n", *pShort, (unsigned int) res );

	res = VarI2FromI4( 32767, pShort );
	printf( "VarI2FromI4: 32767, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromI4( 32768, pShort );
	printf( "VarI2FromI4: 32768, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromI4( -32768, pShort );
	printf( "VarI2FromI4: -32768, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromI4( -32769, pShort );
	printf( "VarI2FromI4: -32769, %d, %X\n", *pShort, (unsigned int) res );

	res = VarI2FromR8( 69.33, pShort );
	printf( "VarI2FromR8: 69.33, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromR8( 69.66, pShort );
	printf( "VarI2FromR8: 69.66, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromR8( -69.33, pShort );
	printf( "VarI2FromR8: -69.33, %d, %X\n", *pShort, (unsigned int) res );
	res = VarI2FromR8( -69.66, pShort );
	printf( "VarI2FromR8: -69.66, %d, %X\n", *pShort, (unsigned int) res );

	res = VarI2FromDate( -69.66, pShort );
	printf( "VarI2FromDate: -69.66, %d, %X\n", *pShort, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarI2FromStr( pOleChar[olePtrIndex], 0, 0, pShort );
		printf( "VarI2FromStr: %s, %d, %X\n", WtoA(pOleChar[olePtrIndex]), *pShort, (unsigned int) res );
	}
	
	/* long from ...
	 */
	printf( "\n\n======== Testing VarI4FromXXX ========\n");

	res = VarI4FromI2( 3, (long*)pInt );
	printf( "VarIntFromI2: 3, %d, %X\n", *pInt, (unsigned int) res );

	res = VarI4FromR8( 69.33, pLong );
	printf( "VarI4FromR8: 69.33, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( 69.66, pLong );
	printf( "VarI4FromR8: 69.66, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( -69.33, pLong );
	printf( "VarI4FromR8: -69.33, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( -69.66, pLong );
	printf( "VarI4FromR8: -69.66, %ld, %X\n", *pLong, (unsigned int) res );

	res = VarI4FromR8( 2147483647.0, pLong );
	printf( "VarI4FromR8: 2147483647.0, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( 2147483648.0, pLong );
	printf( "VarI4FromR8: 2147483648.0, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( -2147483647.0, pLong );
	printf( "VarI4FromR8: -2147483647.0, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( -2147483648.0, pLong );
	printf( "VarI4FromR8: -2147483648.0, %ld, %X\n", *pLong, (unsigned int) res );
	res = VarI4FromR8( -2147483649.0, pLong );
	printf( "VarI4FromR8: -2147483649.0, %ld, %X\n", *pLong, (unsigned int) res );

	res = VarI4FromDate( -2147483649.0, pLong );
	printf( "VarI4FromDate: -2147483649.0, %ld, %X\n", *pLong, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarI4FromStr( pOleChar[olePtrIndex], 0, 0, pLong );
		printf( "VarI4FromStr: %s, %ld, %X\n", WtoA(pOleChar[olePtrIndex]), *pLong, (unsigned int) res );
	}

	/* float from ...
	 */
	printf( "\n\n======== Testing VarR4FromXXX ========\n");

	res = VarR4FromI4( 16777216, pFloat );
	printf( "VarR4FromI4: 16777216, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromI4( 16777217, pFloat );
	printf( "VarR4FromI4: 16777217, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromI4( -16777216, pFloat );
	printf( "VarR4FromI4: -16777216, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromI4( -16777217, pFloat );
	printf( "VarR4FromI4: -16777217, %f, %X\n", *pFloat, (unsigned int) res );

	res = VarR4FromR8( 16777216.0, pFloat );
	printf( "VarR4FromR8: 16777216.0, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromR8( 16777217.0, pFloat );
	printf( "VarR4FromR8: 16777217.0, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromR8( -16777216.0, pFloat );
	printf( "VarR4FromR8: -16777216.0, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromR8( -16777217.0, pFloat );
	printf( "VarR4FromR8: -16777217.0, %f, %X\n", *pFloat, (unsigned int) res );
	
	res = VarR4FromR8( 16777218e31, pFloat );
	printf( "VarR4FromR8: 16777218e31, %f, %X\n", *pFloat, (unsigned int) res );
	res = VarR4FromR8( 16777218e32, pFloat );
	printf( "VarR4FromR8: 16777218e32, %f, %X\n", *pFloat, (unsigned int) res );
	
	res = VarR4FromDate( 16777218e31, pFloat );
	printf( "VarR4FromDate: 16777218e31, %f, %X\n", *pFloat, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarR4FromStr( pOleChar[olePtrIndex], 0, 0, pFloat );
		printf( "VarR4FromStr: %s, %f, %X\n", WtoA(pOleChar[olePtrIndex]), *pFloat, (unsigned int) res );
	}

	/* double from ...
	 */
	printf( "\n\n======== Testing VarR8FromXXX ========\n");

	res = VarR8FromDate( 900719925474099.0, pDouble );
	printf( "VarR8FromDate: 900719925474099.0, %f, %X\n", *pDouble, (unsigned int) res );

	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarR8FromStr( pOleChar[olePtrIndex], 0, 0, pDouble );
		printf( "VarR8FromStr: %s, %f, %X\n", WtoA(pOleChar[olePtrIndex]), *pDouble, (unsigned int) res );
	}

	/* date from ...
	 */
	printf( "\n\n======== Testing VarDateFromXXX ========\n");

	res = VarDateFromI4( 2958465, pDouble );
	printf( "VarDateFromI4: 2958465, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromI4( 2958466, pDouble );
	printf( "VarDateFromI4: 2958466, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromI4( -657434, pDouble );
	printf( "VarDateFromI4: -657434, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromI4( -657435, pDouble );
	printf( "VarDateFromI4: -657435, %f, %X\n", *pDouble, (unsigned int) res );

	res = VarDateFromR8( 2958465.9999, pDouble );
	printf( "VarDateFromR8: 2958465.9999, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromR8( 2958466, pDouble );
	printf( "VarDateFromR8: 2958466, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromR8( -657434.9999, pDouble );
	printf( "VarDateFromR8: -657434.9999, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromR8( -657435, pDouble );
	printf( "VarDateFromR8: -657435, %f, %X\n", *pDouble, (unsigned int) res );


	res = VarDateFromR8( 0.0, pDouble );
	printf( "VarDateFromR8: 0.0, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromR8( 1.0, pDouble );
	printf( "VarDateFromR8: 1.0, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromR8( 2.25, pDouble );
	printf( "VarDateFromR8: 2.25, %f, %X\n", *pDouble, (unsigned int) res );
	res = VarDateFromR8( -2.0, pDouble );
	printf( "VarDateFromR8: -2.0, %f, %X\n", *pDouble, (unsigned int) res );

	/* Need some parsing function in Linux to emulate this...
	 * Still in progess.
	 */
	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarDateFromStr( pOleChar[olePtrIndex], 0, 0, pDouble );
		printf( "VarDateFromStr: %s, %f, %X\n", WtoA(pOleChar[olePtrIndex]), *pDouble, (unsigned int) res );
	}
	
	/* bool from ...
	 */
	printf( "\n\n======== Testing VarBoolFromXXX ========\n");

	res = VarBoolFromI4( 0, pBool );
	printf( "VarBoolFromI4: 0, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromI4( 1, pBool );
	printf( "VarBoolFromI4: 1, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromI4( -1, pBool );
	printf( "VarBoolFromI4: -1, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromI4( 2, pBool );
	printf( "VarBoolFromI4: 2, %d, %X\n", *pBool, (unsigned int) res );

	res = VarBoolFromUI1( ' ', pBool );
	printf( "VarBoolFromUI1: ' ', %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromUI1( '\0', pBool );
	printf( "VarBoolFromUI1: '\\0', %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromUI1( 0x0000, pBool );
	printf( "VarBoolFromUI1: 0x0000, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromUI1( (unsigned char)0xFFF, pBool );
	printf( "VarBoolFromUI1: 0xFFF, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromUI1( (unsigned char)0xFFFF, pBool );
	printf( "VarBoolFromUI1: 0xFFFF, %d, %X\n", *pBool, (unsigned int) res );
	
	res = VarBoolFromR8( 0.0, pBool );
	printf( "VarBoolFromR8: 0.0, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( 1.1, pBool );
	printf( "VarBoolFromR8: 1.1, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( 0.5, pBool );
	printf( "VarBoolFromR8: 0.5, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( 0.49, pBool );
	printf( "VarBoolFromR8: 0.49, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( 0.51, pBool );
	printf( "VarBoolFromR8: 0.51, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( -0.5, pBool );
	printf( "VarBoolFromR8: -0.5, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( -0.49, pBool );
	printf( "VarBoolFromR8: -0.49, %d, %X\n", *pBool, (unsigned int) res );
	res = VarBoolFromR8( -0.51, pBool );
	printf( "VarBoolFromR8: -0.51, %d, %X\n", *pBool, (unsigned int) res );

	
	for( olePtrIndex = 0; olePtrIndex < nOlePtrs; olePtrIndex ++ )
	{
		res = VarBoolFromStr( pOleChar[olePtrIndex], 0, 0, pBool );
		printf( "VarBoolFromStr: %s, %d, %X\n", WtoA(pOleChar[olePtrIndex]), *pBool, (unsigned int) res );
	}

	res = VarI1FromBool( VARIANT_TRUE, pByte );
	printf( "VarI1FromBool: VARIANT_TRUE, %d, %X\n", *pByte, (unsigned int) res );

	res = VarUI2FromI2( -1, &i );
	printf( "VarUI2FromI2: -1, %d, %X\n", i, (unsigned int) res );


	/* BSTR from ...
	 */
	printf( "\n\n======== Testing VarBSTRFromXXX ========\n");

	/* integers...
	 */
	res = VarBstrFromI1( -100, 0, 0, &bstr );
	printf( "VarBstrFromI1: -100, %s, %X\n", WtoA( bstr ), (unsigned int) res );

	res = VarBstrFromUI1( 0x5A, 0, 0, &bstr );
	printf( "VarBstrFromUI1: 0x5A, %s, %X\n", WtoA( bstr ), (unsigned int) res );

	res = VarBstrFromI4( 2958465, 0, 0, &bstr );
	printf( "VarBstrFromI4: 2958465, %s, %X\n", WtoA( bstr ), (unsigned int) res );

	/* reals...
	 */
	d=0;
	for( i=0; i<20; i++ )
	{
		/* add an integer to the real number
		 */
		d += ((i%9)+1) * pow( 10, i );
		res = VarBstrFromR8( d, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", d, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR8( -d, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", -d, WtoA( bstr ), (unsigned int) res );
	}
	d=0;
	for( i=0; i<20; i++ )
	{
		/* add a decimal to the real number
		 */
		d += ((i%9)+1) * pow( 10, (i*-1) );
		res = VarBstrFromR8( d, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", d, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR8( d-1, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", d-1, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR8( -d, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", -d, WtoA( bstr ), (unsigned int) res );
	}

	d=0;
	for( i=0; i<20; i++ )
	{
		/* add an integer to the real number
		 */
		d += ((i%9)+1) * pow( 10, i );
		/* add a decimal to the real number
		 */
		d += ((i%9)+1) * pow( 10, (i*-1) );
		res = VarBstrFromR8( d, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", d, WtoA( bstr ), (unsigned int)res );
		res = VarBstrFromR8( -d, 0, 0, &bstr );
		printf( "VarBstrFromR8: %f, %s, %X\n", -d, WtoA( bstr ), (unsigned int) res );
	}



	d=0;
	for( i=0; i<10; i++ )
	{
		/* add an integer to the real number
		 */
		d += ((i%9)+1) * pow( 10, i );
		res = VarBstrFromR4( (float)d, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", d, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR4( (float)-d, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", -d, WtoA( bstr ), (unsigned int) res );
	}
	d=0;
	for( i=0; i<10; i++ )
	{
		/* add a decimal to the real number
		 */
		d += ((i%9)+1) * pow( 10, (i*-1) );
		res = VarBstrFromR4( (float)d, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", d, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR4( (float)d-1, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", d-1, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR4( (float)-d, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", -d, WtoA( bstr ), (unsigned int) res );
	}

	d=0;
	for( i=0; i<10; i++ )
	{
		/* add an integer to the real number
		 */
		d += ((i%9)+1) * pow( 10, i );
		/* add a decimal to the real number
		 */
		d += ((i%9)+1) * pow( 10, (i*-1) );
		res = VarBstrFromR4( (float)d, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", d, WtoA( bstr ), (unsigned int) res );
		res = VarBstrFromR4( (float)-d, 0, 0, &bstr );
		printf( "VarBstrFromR4: %f, %s, %X\n", -d, WtoA( bstr ), (unsigned int) res );
	}

	res = VarBstrFromBool( 0x00, 0, 0, &bstr );
	printf( "VarBstrFromBool: 0x00, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromBool( 0xFF, 0, 0, &bstr );
	printf( "VarBstrFromBool: 0xFF, %s, %X\n", WtoA( bstr ), (unsigned int) res );

	res = VarBstrFromDate( 0.0, 0, 0, &bstr );
	printf( "VarBstrFromDate: 0.0, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromDate( 3.34, 0, 0, &bstr );
	printf( "VarBstrFromDate: 3.34, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromDate( 3339.34, 0, 0, &bstr );
	printf( "VarBstrFromDate: 3339.34, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromDate( 365.00, 0, 0, &bstr );
	printf( "VarBstrFromDate: 365.00, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromDate( 365.25, 0, 0, &bstr );
	printf( "VarBstrFromDate: 365.25, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromDate( 1461.0, 0, 0, &bstr );
	printf( "VarBstrFromDate: 1461.00, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	res = VarBstrFromDate( 1461.5, 0, 0, &bstr );
	printf( "VarBstrFromDate: 1461.5, %s, %X\n", WtoA( bstr ), (unsigned int) res );
	

	res = VarBstrFromBool( 0x00, 0, 0, &bstr );
	printf( "VarBstrFromBool: 0x00, %s, %X\n", WtoA(bstr), (unsigned int) res );
	res = VarBstrFromBool( 0xFF, 0, 0, &bstr );
	printf( "VarBstrFromBool: 0xFF, %s, %X\n", WtoA(bstr), (unsigned int) res );

	res = VarBstrFromDate( 0.0, 0, 0, &bstr );
	printf( "VarBstrFromDate: 0.0, %s, %X\n", WtoA(bstr), (unsigned int) res );
	res = VarBstrFromDate( 3.34, 0, 0, &bstr );
	printf( "VarBstrFromDate: 3.34, %s, %X\n", WtoA(bstr), (unsigned int) res );

	/* Test variant API...
	 */
	printf( "\n\n======== Testing Hi-Level Variant API ========\n");

	bstr = SysAllocString( pOleChar[4] );

	res = VariantClear( &va );
	printf( "Result is: %x\n", (unsigned int)res );

	VariantInit( &va );
	VariantInit( &vb );
	VariantInit( &vc );
	VariantInit( &vd );
	VariantInit( &ve );

	va.vt = VT_BSTR;
	va.u.bstrVal = bstr;
	res = VariantClear( &va );
	printf( "VariantClear: %x\n", (unsigned int)res );
	printf( "VariantClear: %x\n", (unsigned int)res );
	SysFreeString( bstr );
	SysFreeString( bstr );

	res = VariantCopy( &vb, &va );
	printf( "VariantCopy: %x\n", (unsigned int)res );
	res = VariantClear( &vb );
	printf( "VariantClear: %x\n", (unsigned int)res );
	res = VariantClear( &va );
	printf( "VariantClear: %x\n", (unsigned int)res );


	va.vt = VT_R8;
	d = 4.123;
	va.u.dblVal = d;
	res = VariantCopy( &va, &va );
	printf( "VariantCopy %f -> %f: %x\n", V_R8(&va), V_R8(&va), (unsigned int)res );

	va.vt = VT_R8 | VT_BYREF;
	d = 31.123;
	va.u.pdblVal = &d;
	res = VariantCopyInd( &va, &va );
	printf( "VariantCopyInd %f: %x\n", V_R8(&va), (unsigned int)res );

	va.vt = VT_R8;
	d = 1.123;
	va.u.dblVal = d;
	res = VariantCopy( &vb, &va );
	printf( "VariantCopy %f -> %f: %x\n", V_R8(&va), V_R8(&vb), (unsigned int)res );

	va.vt = VT_R8 | VT_BYREF;
	d = 123.123;
	va.u.pdblVal = &d;
	res = VariantCopy( &vb, &va );
	printf( "VariantCopy %f -> %f: %x\n", *(V_R8REF(&va)), *(V_R8REF(&vb)), (unsigned int)res );

	va.vt = VT_R8 | VT_BYREF;
	d = 111.2;
	va.u.pdblVal = &d;
	res = VariantCopyInd( &vb, &va );
	printf( "VariantCopyInd %f -> %f: %x\n", *(V_R8REF(&va)), V_R8(&vb), (unsigned int)res );

	va.vt = VT_R8 | VT_BYREF;
	d = 1211.123453;
	va.u.pdblVal = &d;
	res = VariantChangeTypeEx( &va, &va, 0, 0, VT_I2 );
	printf( "VariantChangeTypeEx %d: %x\n", V_I2(&va), (unsigned int) res );

	va.vt = VT_INT;
	va.u.intVal = 4;
	res = VariantChangeTypeEx(&vb, &va, 0, 0, VT_BSTR );
	printf( "VariantChangeTypeEx %d -> %s: %x\n", V_INT(&va), WtoA(V_BSTR(&vb)), (unsigned int)res );

	va.vt = VT_DATE;
	va.u.date = 34465.332431;
	res = VariantChangeTypeEx(&vb, &va, 0, 0, VT_BSTR );
	printf( "VariantChangeTypeEx %f -> %s: %x\n", V_DATE(&va), WtoA(V_BSTR(&vb)), (unsigned int)res );

	bstr = pOleChar[4];
	va.vt = VT_BSTR;
	va.u.bstrVal = bstr;
	res = VariantChangeTypeEx(&vb, &va, 0, 0, VT_R8 );
	printf( "VariantChangeTypeEx %s -> %f: %x\n", WtoA(V_BSTR(&va)), V_R8(&vb), (unsigned int)res );


	vc.vt = VT_BSTR | VT_BYREF;
	vc.u.pbstrVal = &bstr;
	vb.vt = VT_VARIANT | VT_BYREF;
	vb.u.pvarVal = &vc;
	va.vt = VT_VARIANT | VT_BYREF;
	va.u.pvarVal = &vb;
	res = VariantCopyInd( &vd, &va );
	printf( "VariantCopyInd: %x\n", (unsigned int)res );

	/* test what happens when bad vartypes are passed in
	 */
	printf( "-------------- Testing different VARTYPES ----------------\n" );

	for( i=0; i<100; i++ )
	{
		/* Trying to use variants that are set to be BSTR but
		 * do not contain a valid pointer makes the program crash
		 * in Windows so we will skip those.  We do not need them
		 * anyways to illustrate the behavior.
		 */
		if( i ==  VT_BSTR )
			i = 77;

		va.vt = i;
		d = 4.123;
		va.u.dblVal = d;
		res = VariantCopyInd( &vb, &va );
		printf( "VariantCopyInd: %d -> %x\n", i, (unsigned int)res );

		va.vt = i | VT_BYREF;
		d = 4.123;
		va.u.pdblVal = &d;
		res = VariantCopyInd( &vb, &va );
		printf( "VariantCopyInd: %d -> %x\n", i, (unsigned int)res );

		va.vt = VT_R8;
		d = 4.123;
		va.u.dblVal = d;
		res = VariantChangeTypeEx( &vb, &va, 0, 0, i );
		printf( "VariantChangeTypeEx: %d -> %x\n", i, (unsigned int)res );

		va.vt = VT_R8;
		d = 4.123;
		va.u.dblVal = d;
		res = VariantChangeTypeEx( &vb, &va, 0, 0, i | VT_BYREF );
		printf( "VariantChangeTypeEx: VT_BYREF %d -> %x\n", i, (unsigned int)res );

		va.vt = 99;
		d = 4.123;
		va.u.dblVal = d;
		res = VariantClear( &va );
		printf( "VariantClear: %d -> %x\n", i, (unsigned int)res );

	}
	
	res = VariantClear( &va );
	printf( "VariantClear: %x\n", (unsigned int)res );
	res = VariantClear( &vb );
	printf( "VariantClear: %x\n", (unsigned int)res );
	res = VariantClear( &vc );
	printf( "VariantClear: %x\n", (unsigned int)res );
	res = VariantClear( &vd );
	printf( "VariantClear: %x\n", (unsigned int)res );
	res = VariantClear( &ve );
	printf( "VariantClear: %x\n", (unsigned int)res );


	/* There is alot of memory leaks but this is simply a test program.
	 */

	return 0;
}

