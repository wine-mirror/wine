#ifndef __WINE_OLEAUTO_H
#define __WINE_OLEAUTO_H

#include <ole.h>
#include "mapidefs.h"

BSTR16 WINAPI SysAllocString16(LPOLESTR16);
BSTR32 WINAPI SysAllocString32(LPOLESTR32);
#define SysAllocString WINELIB_NAME(SysAllocString)
INT16 WINAPI SysReAllocString16(LPBSTR16,LPOLESTR16);
INT32 WINAPI SysReAllocString32(LPBSTR32,LPOLESTR32);
#define SysReAllocString WINELIB_NAME(SysReAllocString)
VOID WINAPI SysFreeString16(BSTR16);
VOID WINAPI SysFreeString32(BSTR32);
#define SysFreeString WINELIB_NAME(SysFreeString)
BSTR16 WINAPI SysAllocStringLen16(char*, int);
BSTR32 WINAPI SysAllocStringLen32(WCHAR*, int);
#define SysAllocStringLen WINELIB_NAME(SysAllocStringLen)
int WINAPI SysReAllocStringLen16(BSTR16*, char*,  int);
int WINAPI SysReAllocStringLen32(BSTR32*, WCHAR*, int);
#define SysReAllocStringLen WINELIB_NAME(SysReAllocStringLen)
int WINAPI SysStringLen16(BSTR16);
int WINAPI SysStringLen32(BSTR32);
#define SysStringLen WINELIB_NAME(SysStringLen)

typedef void ITypeLib;
typedef ITypeLib * LPTYPELIB;


/*
 * Data types for Variants.
 */

/*
 * 0 == FALSE and -1 == TRUE
 */
typedef short VARIANT_BOOL;

#define VARIANT_TRUE	 0xFFFF
#define VARIANT_FALSE	 0x0000

typedef LONG SCODE;

enum VARENUM {
	VT_EMPTY = 0,
	VT_NULL = 1,
	VT_I2 = 2,
	VT_I4 = 3,
	VT_R4 = 4,
	VT_R8 = 5,
	VT_CY = 6,
	VT_DATE = 7,
	VT_BSTR = 8,
	VT_DISPATCH = 9,
	VT_ERROR  = 10,
	VT_BOOL = 11,
	VT_VARIANT	= 12,
	VT_UNKNOWN	= 13,
	VT_DECIMAL	= 14,
	VT_I1 = 16,
	VT_UI1	= 17,
	VT_UI2	= 18,
	VT_UI4	= 19,
	VT_I8 = 20,
	VT_UI8	= 21,
	VT_INT	= 22,
	VT_UINT = 23,
	VT_VOID = 24,
	VT_HRESULT	= 25,
	VT_PTR	= 26,
	VT_SAFEARRAY  = 27,
	VT_CARRAY = 28,
	VT_USERDEFINED	= 29,
	VT_LPSTR  = 30,
	VT_LPWSTR = 31,
	VT_FILETIME = 64,
	VT_BLOB = 65,
	VT_STREAM = 66,
	VT_STORAGE	= 67,
	VT_STREAMED_OBJECT	= 68,
	VT_STORED_OBJECT  = 69,
	VT_BLOB_OBJECT	= 70,
	VT_CF = 71,
	VT_CLSID  = 72,
	VT_VECTOR = 0x1000,
	VT_ARRAY  = 0x2000,
	VT_BYREF  = 0x4000,
	VT_RESERVED = 0x8000,
	VT_ILLEGAL	= 0xffff,
	VT_ILLEGALMASKED  = 0xfff,
	VT_TYPEMASK = 0xfff
};

/* the largest valide type
 */
#define VT_MAXVALIDTYPE VT_CLSID


/*
 * Declarations of the VARIANT structure and the VARIANT APIs.
 */

/* S_OK 			   : Success.
 * DISP_E_BADVARTYPE   : The variant type vt in not a valid type of variant.
 * DISP_E_OVERFLOW	   : The data pointed to by pvarSrc does not fit in the destination type.
 * DISP_E_TYPEMISMATCH : The variant type vt is not a valid type of variant.
 * E_INVALIDARG 	   : One argument is invalid.
 * E_OUTOFMEMORY	   : Memory could not be allocated for the conversion.
 * DISP_E_ARRAYISLOCKED : The variant contains an array that is locked.
 */

typedef struct tagVARIANT VARIANT;
typedef struct tagVARIANT VARIANTARG;

struct tagVARIANT {
	VARTYPE vt;
	WORD wReserved1;
	WORD wReserved2;
	WORD wReserved3;
	union
	{
		/* By value.
		 */
		CHAR cVal;
		USHORT uiVal;
		ULONG ulVal;
		INT32 intVal;
		UINT32 uintVal;
		BYTE bVal;
		short iVal;
		long lVal;
		float fltVal;
		double dblVal;
		VARIANT_BOOL boolVal;
		SCODE scode;
		DATE date;
		BSTR32 bstrVal;
		CY cyVal;
        /*
        DECIMAL decVal;
		IUnknown* punkVal;
		IDispatch* pdispVal;
        SAFEARRAY* parray;
        */

		/* By reference
		 */
		CHAR* pcVal;
		USHORT* puiVal;
		ULONG* pulVal;
		INT32* pintVal;
		UINT32* puintVal;
		BYTE* pbVal;
		short* piVal;
		long* plVal;
		float* pfltVal;
		double* pdblVal;
		VARIANT_BOOL* pboolVal;
		SCODE* pscode;
		DATE* pdate;
		BSTR32* pbstrVal;
		VARIANT* pvarVal;
		PVOID byref;
		CY* pcyVal;
        /*
        DECIMAL* pdecVal;
		IUnknown** ppunkVal;
		IDispatch** ppdispVal;
        SAFEARRAY** pparray;
        */
	} u;
};


/* These are macros that help accessing the VARIANT date type.
 */
#define V_UNION(A, B)	((A)->u.B)
#define V_VT(A) 		((A)->vt)

#define V_ISBYREF(A)	 (V_VT(A)&VT_BYREF)
#define V_ISARRAY(A)	 (V_VT(A)&VT_ARRAY)
#define V_ISVECTOR(A)	 (V_VT(A)&VT_VECTOR)
#define V_NONE(A)		 V_I2(A)

#define V_UI1(A)		 V_UNION(A, bVal)
#define V_UI1REF(A) 	 V_UNION(A, pbVal)
#define V_I2(A) 		 V_UNION(A, iVal)
#define V_I2REF(A)		 V_UNION(A, piVal)
#define V_I4(A) 		 V_UNION(A, lVal)
#define V_I4REF(A)		 V_UNION(A, plVal)
#define V_R4(A) 		 V_UNION(A, fltVal)
#define V_R4REF(A)		 V_UNION(A, pfltVal)
#define V_R8(A) 		 V_UNION(A, dblVal)
#define V_R8REF(A)		 V_UNION(A, pdblVal)
#define V_I1(A) 		 V_UNION(A, cVal)
#define V_I1REF(A)		 V_UNION(A, pcVal)
#define V_UI2(A)		 V_UNION(A, uiVal)
#define V_UI2REF(A) 	 V_UNION(A, puiVal)
#define V_UI4(A)		 V_UNION(A, ulVal)
#define V_UI4REF(A) 	 V_UNION(A, pulVal)
#define V_INT(A)		 V_UNION(A, intVal)
#define V_INTREF(A) 	 V_UNION(A, pintVal)
#define V_UINT(A)		 V_UNION(A, uintVal)
#define V_UINTREF(A)	 V_UNION(A, puintVal)
#define V_CY(A) 		 V_UNION(A, cyVal)
#define V_CYREF(A)		 V_UNION(A, pcyVal)
#define V_DATE(A)		 V_UNION(A, date)
#define V_DATEREF(A)	 V_UNION(A, pdate)
#define V_BSTR(A)		 V_UNION(A, bstrVal)
#define V_BSTRREF(A)	 V_UNION(A, pbstrVal)
#define V_DISPATCH(A)	 V_UNION(A, pdispVal)
#define V_DISPATCHREF(A) V_UNION(A, ppdispVal)
#define V_ERROR(A)		 V_UNION(A, scode)
#define V_ERRORREF(A)	 V_UNION(A, pscode)
#define V_BOOL(A)		 V_UNION(A, boolVal)
#define V_BOOLREF(A)	 V_UNION(A, pboolVal)
#define V_UNKNOWN(A)	 V_UNION(A, punkVal)
#define V_UNKNOWNREF(A)  V_UNION(A, ppunkVal)
#define V_VARIANTREF(A)  V_UNION(A, pvarVal)
#define V_ARRAY(A)		 V_UNION(A, parray)
#define V_ARRAYREF(A)	 V_UNION(A, pparray)
#define V_BYREF(A)		 V_UNION(A, byref)
#define V_DECIMAL(A)	 V_UNION(A, decVal)
#define V_DECIMALREF(A)  V_UNION(A, pdecVal)

/*
 * VARIANT API
 */

void WINAPI VariantInit32(VARIANTARG* pvarg);
#define VariantInit WINELIB_NAME(VariantInit)
HRESULT WINAPI VariantClear32(VARIANTARG* pvarg);
#define VariantClear WINELIB_NAME(VariantClear)
HRESULT WINAPI VariantCopy32(VARIANTARG* pvargDest, VARIANTARG* pvargSrc);
#define VariantCopy WINELIB_NAME(VariantCopy)
HRESULT WINAPI VariantCopyInd32(VARIANT* pvargDest, VARIANTARG* pvargSrc);
#define VariantCopyInd WINELIB_NAME(VariantCopyInd)
HRESULT WINAPI VariantChangeType32(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
						  USHORT wFlags, VARTYPE vt);
#define VariantChangeType WINELIB_NAME(VariantChangeType)
HRESULT WINAPI VariantChangeTypeEx32(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
							LCID lcid, USHORT wFlags, VARTYPE vt);
#define VariantChangeTypeEx WINELIB_NAME(VariantChangeTypeEx)

/*
 * These flags are used for the VariantChangeType and VariantChangeTypeEx APIs.
 */

/*
 * This one is of general use.
 */
#define VARIANT_NOVALUEPROP    0x1
/*
 * This one is used for conversions of VT_BOOL to VT_BSTR,
 * the API will convert to "True"|"False" instead of "-1"|"0".
 */
#define VARIANT_ALPHABOOL	   0x2
/*
 * This one is used for conversions to or from a VT_BSTR string,
 * it passes LOCALE_NOUSEROVERRIDE to the core (low-level) coercion routines.
 * This means it will use the system default locale settings instead of custom
 * local settings.
 */
#define VARIANT_NOUSEROVERRIDE 0x4


/*
 * VARTYPE Coercion API
 */

/* Omits the date portion and return only the time value.
 */
#define VAR_TIMEVALUEONLY	((DWORD)0x00000001)
/* Omits the time portion and return only the date value.
 */
#define VAR_DATEVALUEONLY	((DWORD)0x00000002)


HRESULT WINAPI VarUI1FromI232(short sIn, BYTE* pbOut);
#define VarUI1FromI2 WINELIB_NAME(VarUI1FromI2)
HRESULT WINAPI VarUI1FromI432(LONG lIn, BYTE* pbOut);
#define VarUI1FromI4 WINELIB_NAME(VarUI1FromI4)
HRESULT WINAPI VarUI1FromR432(FLOAT fltIn, BYTE* pbOut);
#define VarUI1FromR4 WINELIB_NAME(VarUI1FromR4)
HRESULT WINAPI VarUI1FromR832(double dblIn, BYTE* pbOut);
#define VarUI1FromR8 WINELIB_NAME(VarUI1FromR8)
HRESULT WINAPI VarUI1FromDate32(DATE dateIn, BYTE* pbOut);
#define VarUI1FromDate WINELIB_NAME(VarUI1FromDate)
HRESULT WINAPI VarUI1FromBool32(VARIANT_BOOL boolIn, BYTE* pbOut);
#define VarUI1FromBool WINELIB_NAME(VarUI1FromBool)
HRESULT WINAPI VarUI1FromI132(CHAR cIn, BYTE*pbOut);
#define VarUI1FromI1 WINELIB_NAME(VarUI1FromI1)
HRESULT WINAPI VarUI1FromUI232(USHORT uiIn, BYTE*pbOut);
#define VarUI1FromUI2 WINELIB_NAME(VarUI1FromUI2)
HRESULT WINAPI VarUI1FromUI432(ULONG ulIn, BYTE*pbOut);
#define VarUI1FromUI4 WINELIB_NAME(VarUI1FromUI4)
HRESULT WINAPI VarUI1FromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, BYTE* pbOut);
#define VarUI1FromStr WINELIB_NAME(VarUI1FromStr)
HRESULT WINAPI VarUI1FromCy32(CY cyIn, BYTE* pbOut);
#define VarUI1FromCy WINELIB_NAME(VarUI1FromCy)

/*
HRESULT WINAPI VarUI1FromDec32(DECIMAL*pdecIn, BYTE*pbOut);
#define VarUI1FromDec WINELIB_NAME(VarUI1FromDec)
HRESULT WINAPI VarUI1FromDisp32(IDispatch* pdispIn, LCID lcid, BYTE* pbOut);
#define VarUI1FromDisp WINELIB_NAME(VarUI1FromDisp)
*/

HRESULT WINAPI VarI2FromUI132(BYTE bIn, short* psOut);
#define VarI2FromUI1 WINELIB_NAME(VarI2FromUI1)
HRESULT WINAPI VarI2FromI432(LONG lIn, short* psOut);
#define VarI2FromI4 WINELIB_NAME(VarI2FromI4)
HRESULT WINAPI VarI2FromR432(FLOAT fltIn, short* psOut);
#define VarI2FromR4 WINELIB_NAME(VarI2FromR4)
HRESULT WINAPI VarI2FromR832(double dblIn, short* psOut);
#define VarI2FromR8 WINELIB_NAME(VarI2FromR8)
HRESULT WINAPI VarI2FromDate32(DATE dateIn, short* psOut);
#define VarI2FromDate WINELIB_NAME(VarI2FromDate)
HRESULT WINAPI VarI2FromBool32(VARIANT_BOOL boolIn, short* psOut);
#define VarI2FromBool WINELIB_NAME(VarI2FromBool)
HRESULT WINAPI VarI2FromI132(CHAR cIn, short*psOut);
#define VarI2FromI1 WINELIB_NAME(VarI2FromI1)
HRESULT WINAPI VarI2FromUI232(USHORT uiIn, short*psOut);
#define VarI2FromUI2 WINELIB_NAME(VarI2FromUI2)
HRESULT WINAPI VarI2FromUI432(ULONG ulIn, short*psOut);
#define VarI2FromUI4 WINELIB_NAME(VarI2FromUI4)
HRESULT WINAPI VarI2FromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, short* psOut);
#define VarI2FromStr WINELIB_NAME(VarI2FromStr)
HRESULT WINAPI VarI2FromCy32(CY cyIn, short* psOut);
#define VarI2FromCy WINELIB_NAME(VarI2FromCy)
/*
HRESULT WINAPI VarI2FromDec32(DECIMAL*pdecIn, short*psOut);
#define VarI2FromDec WINELIB_NAME(VarI2FromDec)
HRESULT WINAPI VarI2FromDisp32(IDispatch* pdispIn, LCID lcid, short* psOut);
#define VarI2FromDisp WINELIB_NAME(VarI2FromDisp)
*/

HRESULT WINAPI VarI4FromUI132(BYTE bIn, LONG* plOut);
#define VarI4FromUI1 WINELIB_NAME(VarI4FromUI1)
HRESULT WINAPI VarI4FromI232(short sIn, LONG* plOut);
#define VarI4FromI2 WINELIB_NAME(VarI4FromI2)
HRESULT WINAPI VarI4FromR432(FLOAT fltIn, LONG* plOut);
#define VarI4FromR4 WINELIB_NAME(VarI4FromR4)
HRESULT WINAPI VarI4FromR832(double dblIn, LONG* plOut);
#define VarI4FromR8 WINELIB_NAME(VarI4FromR8)
HRESULT WINAPI VarI4FromDate32(DATE dateIn, LONG* plOut);
#define VarI4FromDate WINELIB_NAME(VarI4FromDate)
HRESULT WINAPI VarI4FromBool32(VARIANT_BOOL boolIn, LONG* plOut);
#define VarI4FromBool WINELIB_NAME(VarI4FromBool)
HRESULT WINAPI VarI4FromI132(CHAR cIn, LONG* plOut);
#define VarI4FromI1 WINELIB_NAME(VarI4FromI1)
HRESULT WINAPI VarI4FromUI232(USHORT uiIn, LONG*plOut);
#define VarI4FromUI2 WINELIB_NAME(VarI4FromUI2)
HRESULT WINAPI VarI4FromUI432(ULONG ulIn, LONG*plOut);
#define VarI4FromUI4 WINELIB_NAME(VarI4FromUI4)
HRESULT WINAPI VarI4FromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, LONG* plOut);
#define VarI4FromStr WINELIB_NAME(VarI4FromStr)
HRESULT WINAPI VarI4FromCy32(CY cyIn, LONG* plOut);
#define VarI4FromCy WINELIB_NAME(VarI4FromCy)
/*
HRESULT WINAPI VarI4FromDec32(DECIMAL*pdecIn, LONG*plOut);
#define VarI4FromDec WINELIB_NAME(VarI4FromDec)
HRESULT WINAPI VarI4FromDisp32(IDispatch* pdispIn, LCID lcid, LONG* plOut);
#define VarI4FromDisp WINELIB_NAME(VarI4FromDisp)
*/

HRESULT WINAPI VarR4FromUI132(BYTE bIn, FLOAT* pfltOut);
#define VarR4FromUI1 WINELIB_NAME(VarR4FromUI1)
HRESULT WINAPI VarR4FromI232(short sIn, FLOAT* pfltOut);
#define VarR4FromI2 WINELIB_NAME(VarR4FromI2)
HRESULT WINAPI VarR4FromI432(LONG lIn, FLOAT* pfltOut);
#define VarR4FromI4 WINELIB_NAME(VarR4FromI4)
HRESULT WINAPI VarR4FromR832(double dblIn, FLOAT* pfltOut);
#define VarR4FromR8 WINELIB_NAME(VarR4FromR8)
HRESULT WINAPI VarR4FromDate32(DATE dateIn, FLOAT* pfltOut);
#define VarR4FromDate WINELIB_NAME(VarR4FromDate)
HRESULT WINAPI VarR4FromBool32(VARIANT_BOOL boolIn, FLOAT* pfltOut);
#define VarR4FromBool WINELIB_NAME(VarR4FromBool)
HRESULT WINAPI VarR4FromI132(CHAR cIn, FLOAT*pfltOut);
#define VarR4FromI1 WINELIB_NAME(VarR4FromI1)
HRESULT WINAPI VarR4FromUI232(USHORT uiIn, FLOAT*pfltOut);
#define VarR4FromUI2 WINELIB_NAME(VarR4FromUI2)
HRESULT WINAPI VarR4FromUI432(ULONG ulIn, FLOAT*pfltOut);
#define VarR4FromUI4 WINELIB_NAME(VarR4FromUI4)
HRESULT WINAPI VarR4FromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, FLOAT*pfltOut);
#define VarR4FromStr WINELIB_NAME(VarR4FromStr)
HRESULT WINAPI VarR4FromCy32(CY cyIn, FLOAT* pfltOut);
#define VarR4FromCy WINELIB_NAME(VarR4FromCy)
/*
HRESULT WINAPI VarR4FromDec32(DECIMAL*pdecIn, FLOAT*pfltOut);
#define VarR4FromDec WINELIB_NAME(VarR4FromDec)
HRESULT WINAPI VarR4FromDisp32(IDispatch* pdispIn, LCID lcid, FLOAT* pfltOut);
#define VarR4FromDisp WINELIB_NAME(VarR4FromDisp)
*/

HRESULT WINAPI VarR8FromUI132(BYTE bIn, double* pdblOut);
#define VarR8FromUI1 WINELIB_NAME(VarR8FromUI1)
HRESULT WINAPI VarR8FromI232(short sIn, double* pdblOut);
#define VarR8FromI2 WINELIB_NAME(VarR8FromI2)
HRESULT WINAPI VarR8FromI432(LONG lIn, double* pdblOut);
#define VarR8FromI4 WINELIB_NAME(VarR8FromI4)
HRESULT WINAPI VarR8FromR432(FLOAT fltIn, double* pdblOut);
#define VarR8FromR4 WINELIB_NAME(VarR8FromR4)
HRESULT WINAPI VarR8FromDate32(DATE dateIn, double* pdblOut);
#define VarR8FromDate WINELIB_NAME(VarR8FromDate)
HRESULT WINAPI VarR8FromBool32(VARIANT_BOOL boolIn, double* pdblOut);
#define VarR8FromBool WINELIB_NAME(VarR8FromBool)
HRESULT WINAPI VarR8FromI132(CHAR cIn, double*pdblOut);
#define VarR8FromI1 WINELIB_NAME(VarR8FromI1)
HRESULT WINAPI VarR8FromUI232(USHORT uiIn, double*pdblOut);
#define VarR8FromUI2 WINELIB_NAME(VarR8FromUI2)
HRESULT WINAPI VarR8FromUI432(ULONG ulIn, double*pdblOut);
#define VarR8FromUI4 WINELIB_NAME(VarR8FromUI4)
HRESULT WINAPI VarR8FromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, double*pdblOut);
#define VarR8FromStr WINELIB_NAME(VarR8FromStr)
HRESULT WINAPI VarR8FromCy32(CY cyIn, double* pdblOut);
#define VarR8FromCy WINELIB_NAME(VarR8FromCy)
/*
HRESULT WINAPI VarR8FromDec32(DECIMAL*pdecIn, double*pdblOut);
#define VarR8FromDec WINELIB_NAME(VarR8FromDec)
HRESULT WINAPI VarR8FromDisp32(IDispatch* pdispIn, LCID lcid, double* pdblOut);
#define VarR8FromDisp WINELIB_NAME(VarR8FromDisp)
*/

HRESULT WINAPI VarDateFromUI132(BYTE bIn, DATE* pdateOut);
#define VarDateFromUI1 WINELIB_NAME(VarDateFromUI1)
HRESULT WINAPI VarDateFromI232(short sIn, DATE* pdateOut);
#define VarDateFromI2 WINELIB_NAME(VarDateFromI2)
HRESULT WINAPI VarDateFromI432(LONG lIn, DATE* pdateOut);
#define VarDateFromI4 WINELIB_NAME(VarDateFromI4)
HRESULT WINAPI VarDateFromR432(FLOAT fltIn, DATE* pdateOut);
#define VarDateFromR4 WINELIB_NAME(VarDateFromR4)
HRESULT WINAPI VarDateFromR832(double dblIn, DATE* pdateOut);
#define VarDateFromR8 WINELIB_NAME(VarDateFromR8)
HRESULT WINAPI VarDateFromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, DATE*pdateOut);
#define VarDateFromStr WINELIB_NAME(VarDateFromStr)
HRESULT WINAPI VarDateFromI132(CHAR cIn, DATE*pdateOut);
#define VarDateFromI1 WINELIB_NAME(VarDateFromI1)
HRESULT WINAPI VarDateFromUI232(USHORT uiIn, DATE*pdateOut);
#define VarDateFromUI2 WINELIB_NAME(VarDateFromUI2)
HRESULT WINAPI VarDateFromUI432(ULONG ulIn, DATE*pdateOut);
#define VarDateFromUI4 WINELIB_NAME(VarDateFromUI4)
HRESULT WINAPI VarDateFromBool32(VARIANT_BOOL boolIn, DATE* pdateOut);
#define VarDateFromBool WINELIB_NAME(VarDateFromBool)
HRESULT WINAPI VarDateFromCy32(CY cyIn, DATE* pdateOut);
#define VarDateFromCy WINELIB_NAME(VarDateFromCy)
/*
HRESULT WINAPI VarDateFromDec32(DECIMAL*pdecIn, DATE*pdateOut);
#define VarDateFromDec WINELIB_NAME(VarDateFromDec)
HRESULT WINAPI VarDateFromDisp32(IDispatch* pdispIn, LCID lcid, DATE* pdateOut);
#define VarDateFromDisp WINELIB_NAME(VarDateFromDisp)
*/
HRESULT WINAPI VarCyFromUI132(BYTE bIn, CY* pcyOut);
#define VarCyFromUI1 WINELIB_NAME(VarCyFromUI1)
HRESULT WINAPI VarCyFromI232(short sIn, CY* pcyOut);
#define VarCyFromI2 WINELIB_NAME(VarCyFromI2)
HRESULT WINAPI VarCyFromI432(LONG lIn, CY* pcyOut);
#define VarCyFromI4 WINELIB_NAME(VarCyFromI4)
HRESULT WINAPI VarCyFromR432(FLOAT fltIn, CY* pcyOut);
#define VarCyFromR4 WINELIB_NAME(VarCyFromR4)
HRESULT WINAPI VarCyFromR832(double dblIn, CY* pcyOut);
#define VarCyFromR8 WINELIB_NAME(VarCyFromR8)
HRESULT WINAPI VarCyFromDate32(DATE dateIn, CY* pcyOut);
#define VarCyFromDate WINELIB_NAME(VarCyFromDate)
HRESULT WINAPI VarCyFromBool32(VARIANT_BOOL boolIn, CY* pcyOut);
#define VarCyFromBool WINELIB_NAME(VarCyFromBool)
HRESULT WINAPI VarCyFromI132(CHAR cIn, CY*pcyOut);
#define VarCyFromI1 WINELIB_NAME(VarCyFromI1)
HRESULT WINAPI VarCyFromUI232(USHORT uiIn, CY*pcyOut);
#define VarCyFromUI2 WINELIB_NAME(VarCyFromUI2)
HRESULT WINAPI VarCyFromUI432(ULONG ulIn, CY*pcyOut);
#define VarCyFromUI4 WINELIB_NAME(VarCyFromUI4)
/*
HRESULT WINAPI VarCyFromDec32(DECIMAL*pdecIn, CY*pcyOut);
#define VarCyFromDec WINELIB_NAME(VarCyFromDec)
HRESULT WINAPI VarCyFromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, CY* pcyOut);
#define VarCyFromStr WINELIB_NAME(VarCyFromStr)
HRESULT WINAPI VarCyFromDisp32(IDispatch* pdispIn, LCID lcid, CY* pcyOut);
#define VarCyFromDisp WINELIB_NAME(VarCyFromDisp)
*/

HRESULT WINAPI VarBstrFromUI132(BYTE bVal, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromUI1 WINELIB_NAME(VarBstrFromUI1)
HRESULT WINAPI VarBstrFromI232(short iVal, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromI2 WINELIB_NAME(VarBstrFromI2)
HRESULT WINAPI VarBstrFromI432(LONG lIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromI4 WINELIB_NAME(VarBstrFromI4)
HRESULT WINAPI VarBstrFromR432(FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromR4 WINELIB_NAME(VarBstrFromR4)
HRESULT WINAPI VarBstrFromR832(double dblIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromR8 WINELIB_NAME(VarBstrFromR8)
HRESULT WINAPI VarBstrFromDate32(DATE dateIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromDate WINELIB_NAME(VarBstrFromDate)
HRESULT WINAPI VarBstrFromBool32(VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromBool WINELIB_NAME(VarBstrFromBool)
HRESULT WINAPI VarBstrFromI132(CHAR cIn, LCID lcid, ULONG dwFlags, BSTR32*pbstrOut);
#define VarBstrFromI1 WINELIB_NAME(VarBstrFromI1)
HRESULT WINAPI VarBstrFromUI232(USHORT uiIn, LCID lcid, ULONG dwFlags, BSTR32*pbstrOut);
#define VarBstrFromUI2 WINELIB_NAME(VarBstrFromUI2)
HRESULT WINAPI VarBstrFromUI432(ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR32*pbstrOut);
#define VarBstrFromUI4 WINELIB_NAME(VarBstrFromUI4)
HRESULT WINAPI VarBstrFromCy32(CY cyIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromCy WINELIB_NAME(VarBstrFromCy)
/*
HRESULT WINAPI VarBstrFromDec32(DECIMAL*pdecIn, LCID lcid, ULONG dwFlags, BSTR32*pbstrOut);
#define VarBstrFromDec WINELIB_NAME(VarBstrFromDec)
HRESULT WINAPI VarBstrFromDisp32(IDispatch* pdispIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
#define VarBstrFromDisp WINELIB_NAME(VarBstrFromDisp)
*/

HRESULT WINAPI VarBoolFromUI132(BYTE bIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromUI1 WINELIB_NAME(VarBoolFromUI1)
HRESULT WINAPI VarBoolFromI232(short sIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromI2 WINELIB_NAME(VarBoolFromI2)
HRESULT WINAPI VarBoolFromI432(LONG lIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromI4 WINELIB_NAME(VarBoolFromI4)
HRESULT WINAPI VarBoolFromR432(FLOAT fltIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromR4 WINELIB_NAME(VarBoolFromR4)
HRESULT WINAPI VarBoolFromR832(double dblIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromR8 WINELIB_NAME(VarBoolFromR8)
HRESULT WINAPI VarBoolFromDate32(DATE dateIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromDate WINELIB_NAME(VarBoolFromDate)
HRESULT WINAPI VarBoolFromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL* pboolOut);
#define VarBoolFromStr WINELIB_NAME(VarBoolFromStr)
HRESULT WINAPI VarBoolFromI132(CHAR cIn, VARIANT_BOOL*pboolOut);
#define VarBoolFromI1 WINELIB_NAME(VarBoolFromI1)
HRESULT WINAPI VarBoolFromUI232(USHORT uiIn, VARIANT_BOOL*pboolOut);
#define VarBoolFromUI2 WINELIB_NAME(VarBoolFromUI2)
HRESULT WINAPI VarBoolFromUI432(ULONG ulIn, VARIANT_BOOL*pboolOut);
#define VarBoolFromUI4 WINELIB_NAME(VarBoolFromUI4)
HRESULT WINAPI VarBoolFromCy32(CY cyIn, VARIANT_BOOL* pboolOut);
#define VarBoolFromCy WINELIB_NAME(VarBoolFromCy)
/*
HRESULT WINAPI VarBoolFromDec32(DECIMAL*pdecIn, VARIANT_BOOL*pboolOut);
#define VarBoolFromDec WINELIB_NAME(VarBoolFromDec)
HRESULT WINAPI VarBoolFromDisp32(IDispatch* pdispIn, LCID lcid, VARIANT_BOOL* pboolOut);
#define VarBoolFromDisp WINELIB_NAME(VarBoolFromDisp)
*/

HRESULT WINAPI VarI1FromUI132(BYTE bIn, CHAR*pcOut);
#define VarI1FromUI1 WINELIB_NAME(VarI1FromUI1)
HRESULT WINAPI VarI1FromI232(short uiIn, CHAR*pcOut);
#define VarI1FromI2 WINELIB_NAME(VarI1FromI2)
HRESULT WINAPI VarI1FromI432(LONG lIn, CHAR*pcOut);
#define VarI1FromI4 WINELIB_NAME(VarI1FromI4)
HRESULT WINAPI VarI1FromR432(FLOAT fltIn, CHAR*pcOut);
#define VarI1FromR4 WINELIB_NAME(VarI1FromR4)
HRESULT WINAPI VarI1FromR832(double dblIn, CHAR*pcOut);
#define VarI1FromR8 WINELIB_NAME(VarI1FromR8)
HRESULT WINAPI VarI1FromDate32(DATE dateIn, CHAR*pcOut);
#define VarI1FromDate WINELIB_NAME(VarI1FromDate)
HRESULT WINAPI VarI1FromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, CHAR*pcOut);
#define VarI1FromStr WINELIB_NAME(VarI1FromStr)
HRESULT WINAPI VarI1FromBool32(VARIANT_BOOL boolIn, CHAR*pcOut);
#define VarI1FromBool WINELIB_NAME(VarI1FromBool)
HRESULT WINAPI VarI1FromUI232(USHORT uiIn, CHAR*pcOut);
#define VarI1FromUI2 WINELIB_NAME(VarI1FromUI2)
HRESULT WINAPI VarI1FromUI432(ULONG ulIn, CHAR*pcOut);
#define VarI1FromUI4 WINELIB_NAME(VarI1FromUI4)
HRESULT WINAPI VarI1FromCy32(CY cyIn, CHAR*pcOut);
#define VarI1FromCy WINELIB_NAME(VarI1FromCy)
/*
HRESULT WINAPI VarI1FromDec32(DECIMAL*pdecIn, CHAR*pcOut);
#define VarI1FromDec WINELIB_NAME(VarI1FromDec)
HRESULT WINAPI VarI1FromDisp32(IDispatch*pdispIn, LCID lcid, CHAR*pcOut);
#define VarI1FromDisp WINELIB_NAME(VarI1FromDisp)
*/

HRESULT WINAPI VarUI2FromUI132(BYTE bIn, USHORT*puiOut);
#define VarUI2FromUI1 WINELIB_NAME(VarUI2FromUI1)
HRESULT WINAPI VarUI2FromI232(short uiIn, USHORT*puiOut);
#define VarUI2FromI2 WINELIB_NAME(VarUI2FromI2)
HRESULT WINAPI VarUI2FromI432(LONG lIn, USHORT*puiOut);
#define VarUI2FromI4 WINELIB_NAME(VarUI2FromI4)
HRESULT WINAPI VarUI2FromR432(FLOAT fltIn, USHORT*puiOut);
#define VarUI2FromR4 WINELIB_NAME(VarUI2FromR4)
HRESULT WINAPI VarUI2FromR832(double dblIn, USHORT*puiOut);
#define VarUI2FromR8 WINELIB_NAME(VarUI2FromR8)
HRESULT WINAPI VarUI2FromDate32(DATE dateIn, USHORT*puiOut);
#define VarUI2FromDate WINELIB_NAME(VarUI2FromDate)
HRESULT WINAPI VarUI2FromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, USHORT*puiOut);
#define VarUI2FromStr WINELIB_NAME(VarUI2FromStr)
HRESULT WINAPI VarUI2FromBool32(VARIANT_BOOL boolIn, USHORT*puiOut);
#define VarUI2FromBool WINELIB_NAME(VarUI2FromBool)
HRESULT WINAPI VarUI2FromI132(CHAR cIn, USHORT*puiOut);
#define VarUI2FromI1 WINELIB_NAME(VarUI2FromI1)
HRESULT WINAPI VarUI2FromUI432(ULONG ulIn, USHORT*puiOut);
#define VarUI2FromUI4 WINELIB_NAME(VarUI2FromUI4)
HRESULT WINAPI VarUI2FromCy32(CY cyIn, USHORT*puiOut);
#define VarUI2FromCy WINELIB_NAME(VarUI2FromCy)
/*
HRESULT WINAPI VarUI2FromDec32(DECIMAL*pdecIn, USHORT*puiOut);
#define VarUI2FromDec WINELIB_NAME(VarUI2FromDec)
HRESULT WINAPI VarUI2FromDisp32(IDispatch*pdispIn, LCID lcid, USHORT*puiOut);
#define VarUI2FromDisp WINELIB_NAME(VarUI2FromDisp)
*/

HRESULT WINAPI VarUI4FromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, ULONG*pulOut);
#define VarUI4FromStr WINELIB_NAME(VarUI4FromStr)
HRESULT WINAPI VarUI4FromUI132(BYTE bIn, ULONG*pulOut);
#define VarUI4FromUI1 WINELIB_NAME(VarUI4FromUI1)
HRESULT WINAPI VarUI4FromI232(short uiIn, ULONG*pulOut);
#define VarUI4FromI2 WINELIB_NAME(VarUI4FromI2)
HRESULT WINAPI VarUI4FromI432(LONG lIn, ULONG*pulOut);
#define VarUI4FromI4 WINELIB_NAME(VarUI4FromI4)
HRESULT WINAPI VarUI4FromR432(FLOAT fltIn, ULONG*pulOut);
#define VarUI4FromR4 WINELIB_NAME(VarUI4FromR4)
HRESULT WINAPI VarUI4FromR832(double dblIn, ULONG*pulOut);
#define VarUI4FromR8 WINELIB_NAME(VarUI4FromR8)
HRESULT WINAPI VarUI4FromDate32(DATE dateIn, ULONG*pulOut);
#define VarUI4FromDate WINELIB_NAME(VarUI4FromDate)
HRESULT WINAPI VarUI4FromBool32(VARIANT_BOOL boolIn, ULONG*pulOut);
#define VarUI4FromBool WINELIB_NAME(VarUI4FromBool)
HRESULT WINAPI VarUI4FromI132(CHAR cIn, ULONG*pulOut);
#define VarUI4FromI1 WINELIB_NAME(VarUI4FromI1)
HRESULT WINAPI VarUI4FromUI232(USHORT uiIn, ULONG*pulOut);
#define VarUI4FromUI2 WINELIB_NAME(VarUI4FromUI2)
HRESULT WINAPI VarUI4FromCy32(CY cyIn, ULONG*pulOut);
#define VarUI4FromCy WINELIB_NAME(VarUI4FromCy)
/*
HRESULT WINAPI VarUI4FromDec32(DECIMAL*pdecIn, ULONG*pulOut);
#define VarUI4FromDec WINELIB_NAME(VarUI4FromDec)
HRESULT WINAPI VarUI4FromDisp32(IDispatch*pdispIn, LCID lcid, ULONG*pulOut);
#define VarUI4FromDisp WINELIB_NAME(VarUI4FromDisp)

HRESULT WINAPI VarDecFromUI132(BYTE bIn, DECIMAL*pdecOut);
#define VarDecFromUI1 WINELIB_NAME(VarDecFromUI1)
HRESULT WINAPI VarDecFromI232(short uiIn, DECIMAL*pdecOut);
#define VarDecFromI2 WINELIB_NAME(VarDecFromI2)
HRESULT WINAPI VarDecFromI432(LONG lIn, DECIMAL*pdecOut);
#define VarDecFromI4 WINELIB_NAME(VarDecFromI4)
HRESULT WINAPI VarDecFromR432(FLOAT fltIn, DECIMAL*pdecOut);
#define VarDecFromR4 WINELIB_NAME(VarDecFromR4)
HRESULT WINAPI VarDecFromR832(double dblIn, DECIMAL*pdecOut);
#define VarDecFromR8 WINELIB_NAME(VarDecFromR8)
HRESULT WINAPI VarDecFromDate32(DATE dateIn, DECIMAL*pdecOut);
#define VarDecFromDate WINELIB_NAME(VarDecFromDate)
HRESULT WINAPI VarDecFromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, DECIMAL*pdecOut);
#define VarDecFromStr WINELIB_NAME(VarDecFromStr)
HRESULT WINAPI VarDecFromBool32(VARIANT_BOOL boolIn, DECIMAL*pdecOut);
#define VarDecFromBool WINELIB_NAME(VarDecFromBool)
HRESULT WINAPI VarDecFromI132(CHAR cIn, DECIMAL*pdecOut);
#define VarDecFromI1 WINELIB_NAME(VarDecFromI1)
HRESULT WINAPI VarDecFromUI232(USHORT uiIn, DECIMAL*pdecOut);
#define VarDecFromUI2 WINELIB_NAME(VarDecFromUI2)
HRESULT WINAPI VarDecFromUI432(ULONG ulIn, DECIMAL*pdecOut);
#define VarDecFromUI4 WINELIB_NAME(VarDecFromUI4)
HRESULT WINAPI VarDecFromCy32(CY cyIn, DECIMAL*pdecOut);
#define VarDecFromCy WINELIB_NAME(VarDecFromCy)
HRESULT WINAPI VarDecFromDisp32(IDispatch*pdispIn, LCID lcid, DECIMAL*pdecOut);
#define VarDecFromDisp WINELIB_NAME(VarDecFromDisp)
*/



#define VarUI4FromUI4( in, pOut ) ( *(pOut) =  (in) )
#define VarI4FromI4( in, pOut )   ( *(pOut) =  (in) )

#define VarUI1FromInt32		VarUI1FromI432
#define VarUI1FromUint32	VarUI1FromUI432
#define VarI2FromInt32		VarI2FromI432
#define VarI2FromUint32		VarI2FromUI432
#define VarI4FromInt32		VarI4FromI432
#define VarI4FromUint32		VarI4FromUI432
#define VarR4FromInt32		VarR4FromI432
#define VarR4FromUint32		VarR4FromUI432
#define VarR8FromInt32		VarR8FromI432
#define VarR8FromUint32		VarR8FromUI432
#define VarDateFromInt32	VarDateFromI432
#define VarDateFromUint32	VarDateFromUI432
#define VarCyFromInt32		VarCyFromI432
#define VarCyFromUint32		VarCyFromUI432
#define VarBstrFromInt32	VarBstrFromI432
#define VarBstrFromUint32	VarBstrFromUI432
#define VarBoolFromInt32	VarBoolFromI432
#define VarBoolFromUint32	VarBoolFromUI432
#define VarI1FromInt32		VarI1FromI432
#define VarI1FromUint32		VarI1FromUI432
#define VarUI2FromInt32		VarUI2FromI432
#define VarUI2FromUint32	VarUI2FromUI432
#define VarUI4FromInt32		VarUI4FromI432
#define VarUI4FromUint32	VarUI4FromUI432
#define VarDecFromInt32		VarDecFromI432
#define VarDecFromUint32	VarDecFromUI432
#define VarIntFromUI132		VarI4FromUI132
#define VarIntFromI232		VarI4FromI232
#define VarIntFromI432		VarI4FromI432
#define VarIntFromR432		VarI4FromR432
#define VarIntFromR832		VarI4FromR832
#define VarIntFromDate32	VarI4FromDate32
#define VarIntFromCy32		VarI4FromCy32
#define VarIntFromStr32		VarI4FromStr32
#define VarIntFromDisp32	VarI4FromDisp32
#define VarIntFromBool32	VarI4FromBool32
#define VarIntFromI132		VarI4FromI132
#define VarIntFromUI232		VarI4FromUI232
#define VarIntFromUI432		VarI4FromUI432
#define VarIntFromDec32		VarI4FromDec32
#define VarIntFromUint32	VarI4FromUI432
#define VarUintFromUI132	VarUI4FromUI132
#define VarUintFromI232		VarUI4FromI232
#define VarUintFromI432		VarUI4FromI432
#define VarUintFromR432		VarUI4FromR432
#define VarUintFromR832		VarUI4FromR832
#define VarUintFromDate32	VarUI4FromDate32
#define VarUintFromCy32		VarUI4FromCy32
#define VarUintFromStr32	VarUI4FromStr32
#define VarUintFromDisp32	VarUI4FromDisp32
#define VarUintFromBool32	VarUI4FromBool32
#define VarUintFromI132		VarUI4FromI132
#define VarUintFromUI232	VarUI4FromUI232
#define VarUintFromUI432	VarUI4FromUI432
#define VarUintFromDec32	VarUI4FromDec32
#define VarUintFromInt32	VarUI4FromI432

#endif /*__WINE_OLEAUTO_H*/
