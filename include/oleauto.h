#ifndef __WINE_OLEAUTO_H
#define __WINE_OLEAUTO_H

#ifdef __cplusplus
extern "C" {
#endif 

/*#include <ole.h> */
#include "mapidefs.h"
#include "wine/obj_oleaut.h"
#include "oaidl.h"

BSTR16 WINAPI SysAllocString16(LPCOLESTR16);
BSTR WINAPI SysAllocString(const OLECHAR*);
INT16 WINAPI SysReAllocString16(LPBSTR16,LPCOLESTR16);
INT WINAPI SysReAllocString(LPBSTR,const OLECHAR*);
VOID WINAPI SysFreeString16(BSTR16);
VOID WINAPI SysFreeString(BSTR);
BSTR16 WINAPI SysAllocStringLen16(const char*, int);
BSTR WINAPI SysAllocStringLen(const OLECHAR*, UINT);
int WINAPI SysReAllocStringLen16(BSTR16*, const char*,  int);
int WINAPI SysReAllocStringLen(BSTR*, const OLECHAR*, UINT);
int WINAPI SysStringLen16(BSTR16);
int WINAPI SysStringLen(BSTR);

/*****************************************************************
 *  SafeArray API
 */

HRESULT WINAPI
SafeArrayAllocDescriptor(UINT cDims, SAFEARRAY **ppsaOut);

HRESULT WINAPI 
SafeArrayAllocData(SAFEARRAY *psa);

SAFEARRAY* WINAPI 
SafeArrayCreate(VARTYPE vt, UINT cDims, SAFEARRAYBOUND *rgsabound);

HRESULT WINAPI 
SafeArrayDestroyDescriptor(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayPutElement(SAFEARRAY *psa, LONG *rgIndices, void *pv);

HRESULT WINAPI 
SafeArrayGetElement(SAFEARRAY *psa, LONG *rgIndices, void *pv);

HRESULT WINAPI 
SafeArrayLock(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayUnlock(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayGetUBound(SAFEARRAY *psa, UINT nDim, LONG *plUbound);

HRESULT WINAPI 
SafeArrayGetLBound(SAFEARRAY *psa, UINT nDim, LONG *plLbound);

UINT  WINAPI 
SafeArrayGetDim(SAFEARRAY *psa);

UINT  WINAPI 
SafeArrayGetElemsize(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayAccessData(SAFEARRAY *psa, void **ppvData);

HRESULT WINAPI 
SafeArrayUnaccessData(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayPtrOfIndex(SAFEARRAY *psa, LONG *rgIndices, void **ppvData);

HRESULT WINAPI 
SafeArrayCopyData(SAFEARRAY *psaSource, SAFEARRAY **psaTarget);

HRESULT WINAPI 
SafeArrayDestroyData(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayDestroy(SAFEARRAY *psa);

HRESULT WINAPI 
SafeArrayCopy(SAFEARRAY *psa, SAFEARRAY **ppsaOut);

SAFEARRAY* WINAPI
SafeArrayCreateVector(VARTYPE vt, LONG lLbound, ULONG cElements);

HRESULT WINAPI 
SafeArrayRedim(SAFEARRAY *psa, SAFEARRAYBOUND *psaboundNew);


/* These are macros that help accessing the VARIANT date type.
 */
#ifdef __cplusplus
#define V_UNION(A, B)	((A)->B)
#define V_VT(A) 		((A)->vt)
#else
#define V_UNION(A, B)	((A)->u.B)
#define V_VT(A) 		((A)->vt)
#endif /* cplusplus */

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

void WINAPI VariantInit(VARIANTARG* pvarg);
HRESULT WINAPI VariantClear(VARIANTARG* pvarg);
HRESULT WINAPI VariantCopy(VARIANTARG* pvargDest, VARIANTARG* pvargSrc);
HRESULT WINAPI VariantCopyInd(VARIANT* pvargDest, VARIANTARG* pvargSrc);
HRESULT WINAPI VariantChangeType(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
						  USHORT wFlags, VARTYPE vt);
HRESULT WINAPI VariantChangeTypeEx(VARIANTARG* pvargDest, VARIANTARG* pvargSrc,
							LCID lcid, USHORT wFlags, VARTYPE vt);

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


HRESULT WINAPI VarUI1FromI2(short sIn, BYTE* pbOut);
HRESULT WINAPI VarUI1FromI4(LONG lIn, BYTE* pbOut);
HRESULT WINAPI VarUI1FromR4(FLOAT fltIn, BYTE* pbOut);
HRESULT WINAPI VarUI1FromR8(double dblIn, BYTE* pbOut);
HRESULT WINAPI VarUI1FromDate(DATE dateIn, BYTE* pbOut);
HRESULT WINAPI VarUI1FromBool(VARIANT_BOOL boolIn, BYTE* pbOut);
HRESULT WINAPI VarUI1FromI1(CHAR cIn, BYTE*pbOut);
HRESULT WINAPI VarUI1FromUI2(USHORT uiIn, BYTE*pbOut);
HRESULT WINAPI VarUI1FromUI4(ULONG ulIn, BYTE*pbOut);
HRESULT WINAPI VarUI1FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, BYTE* pbOut);
HRESULT WINAPI VarUI1FromCy(CY cyIn, BYTE* pbOut);

/*
HRESULT WINAPI VarUI1FromDec32(DECIMAL*pdecIn, BYTE*pbOut);
HRESULT WINAPI VarUI1FromDisp32(IDispatch* pdispIn, LCID lcid, BYTE* pbOut);
*/

HRESULT WINAPI VarI2FromUI1(BYTE bIn, short* psOut);
HRESULT WINAPI VarI2FromI4(LONG lIn, short* psOut);
HRESULT WINAPI VarI2FromR4(FLOAT fltIn, short* psOut);
HRESULT WINAPI VarI2FromR8(double dblIn, short* psOut);
HRESULT WINAPI VarI2FromDate(DATE dateIn, short* psOut);
HRESULT WINAPI VarI2FromBool(VARIANT_BOOL boolIn, short* psOut);
HRESULT WINAPI VarI2FromI1(CHAR cIn, short*psOut);
HRESULT WINAPI VarI2FromUI2(USHORT uiIn, short*psOut);
HRESULT WINAPI VarI2FromUI4(ULONG ulIn, short*psOut);
HRESULT WINAPI VarI2FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, short* psOut);
HRESULT WINAPI VarI2FromCy(CY cyIn, short* psOut);
/*
HRESULT WINAPI VarI2FromDec32(DECIMAL*pdecIn, short*psOut);
HRESULT WINAPI VarI2FromDisp32(IDispatch* pdispIn, LCID lcid, short* psOut);
*/

HRESULT WINAPI VarI4FromUI1(BYTE bIn, LONG* plOut);
HRESULT WINAPI VarI4FromI2(short sIn, LONG* plOut);
HRESULT WINAPI VarI4FromR4(FLOAT fltIn, LONG* plOut);
HRESULT WINAPI VarI4FromR8(double dblIn, LONG* plOut);
HRESULT WINAPI VarI4FromDate(DATE dateIn, LONG* plOut);
HRESULT WINAPI VarI4FromBool(VARIANT_BOOL boolIn, LONG* plOut);
HRESULT WINAPI VarI4FromI1(CHAR cIn, LONG* plOut);
HRESULT WINAPI VarI4FromUI2(USHORT uiIn, LONG*plOut);
HRESULT WINAPI VarI4FromUI4(ULONG ulIn, LONG*plOut);
HRESULT WINAPI VarI4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, LONG* plOut);
HRESULT WINAPI VarI4FromCy(CY cyIn, LONG* plOut);
/*
HRESULT WINAPI VarI4FromDec32(DECIMAL*pdecIn, LONG*plOut);
HRESULT WINAPI VarI4FromDisp32(IDispatch* pdispIn, LCID lcid, LONG* plOut);
*/

HRESULT WINAPI VarR4FromUI1(BYTE bIn, FLOAT* pfltOut);
HRESULT WINAPI VarR4FromI2(short sIn, FLOAT* pfltOut);
HRESULT WINAPI VarR4FromI4(LONG lIn, FLOAT* pfltOut);
HRESULT WINAPI VarR4FromR8(double dblIn, FLOAT* pfltOut);
HRESULT WINAPI VarR4FromDate(DATE dateIn, FLOAT* pfltOut);
HRESULT WINAPI VarR4FromBool(VARIANT_BOOL boolIn, FLOAT* pfltOut);
HRESULT WINAPI VarR4FromI1(CHAR cIn, FLOAT*pfltOut);
HRESULT WINAPI VarR4FromUI2(USHORT uiIn, FLOAT*pfltOut);
HRESULT WINAPI VarR4FromUI4(ULONG ulIn, FLOAT*pfltOut);
HRESULT WINAPI VarR4FromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, FLOAT*pfltOut);
HRESULT WINAPI VarR4FromCy(CY cyIn, FLOAT* pfltOut);
/*
HRESULT WINAPI VarR4FromDec32(DECIMAL*pdecIn, FLOAT*pfltOut);
HRESULT WINAPI VarR4FromDisp32(IDispatch* pdispIn, LCID lcid, FLOAT* pfltOut);
*/

HRESULT WINAPI VarR8FromUI1(BYTE bIn, double* pdblOut);
HRESULT WINAPI VarR8FromI2(short sIn, double* pdblOut);
HRESULT WINAPI VarR8FromI4(LONG lIn, double* pdblOut);
HRESULT WINAPI VarR8FromR4(FLOAT fltIn, double* pdblOut);
HRESULT WINAPI VarR8FromDate(DATE dateIn, double* pdblOut);
HRESULT WINAPI VarR8FromBool(VARIANT_BOOL boolIn, double* pdblOut);
HRESULT WINAPI VarR8FromI1(CHAR cIn, double*pdblOut);
HRESULT WINAPI VarR8FromUI2(USHORT uiIn, double*pdblOut);
HRESULT WINAPI VarR8FromUI4(ULONG ulIn, double*pdblOut);
HRESULT WINAPI VarR8FromStr(OLECHAR*strIn, LCID lcid, ULONG dwFlags, double*pdblOut);
HRESULT WINAPI VarR8FromCy(CY cyIn, double* pdblOut);
/*
HRESULT WINAPI VarR8FromDec32(DECIMAL*pdecIn, double*pdblOut);
HRESULT WINAPI VarR8FromDisp32(IDispatch* pdispIn, LCID lcid, double* pdblOut);
*/

HRESULT WINAPI VarDateFromUI1(BYTE bIn, DATE* pdateOut);
HRESULT WINAPI VarDateFromI2(short sIn, DATE* pdateOut);
HRESULT WINAPI VarDateFromI4(LONG lIn, DATE* pdateOut);
HRESULT WINAPI VarDateFromR4(FLOAT fltIn, DATE* pdateOut);
HRESULT WINAPI VarDateFromR8(double dblIn, DATE* pdateOut);
HRESULT WINAPI VarDateFromStr(OLECHAR*strIn, LCID lcid, ULONG dwFlags, DATE*pdateOut);
HRESULT WINAPI VarDateFromI1(CHAR cIn, DATE*pdateOut);
HRESULT WINAPI VarDateFromUI2(USHORT uiIn, DATE*pdateOut);
HRESULT WINAPI VarDateFromUI4(ULONG ulIn, DATE*pdateOut);
HRESULT WINAPI VarDateFromBool(VARIANT_BOOL boolIn, DATE* pdateOut);
HRESULT WINAPI VarDateFromCy(CY cyIn, DATE* pdateOut);
/*
HRESULT WINAPI VarDateFromDec32(DECIMAL*pdecIn, DATE*pdateOut);
HRESULT WINAPI VarDateFromDisp32(IDispatch* pdispIn, LCID lcid, DATE* pdateOut);
*/
HRESULT WINAPI VarCyFromUI1(BYTE bIn, CY* pcyOut);
HRESULT WINAPI VarCyFromI2(short sIn, CY* pcyOut);
HRESULT WINAPI VarCyFromI4(LONG lIn, CY* pcyOut);
HRESULT WINAPI VarCyFromR4(FLOAT fltIn, CY* pcyOut);
HRESULT WINAPI VarCyFromR8(double dblIn, CY* pcyOut);
HRESULT WINAPI VarCyFromDate(DATE dateIn, CY* pcyOut);
HRESULT WINAPI VarCyFromStr(OLECHAR *strIn, LCID lcid, ULONG dwFlags, CY *pcyOut);
HRESULT WINAPI VarCyFromBool(VARIANT_BOOL boolIn, CY* pcyOut);
HRESULT WINAPI VarCyFromI1(CHAR cIn, CY*pcyOut);
HRESULT WINAPI VarCyFromUI2(USHORT uiIn, CY*pcyOut);
HRESULT WINAPI VarCyFromUI4(ULONG ulIn, CY*pcyOut);
/*
HRESULT WINAPI VarCyFromDec32(DECIMAL*pdecIn, CY*pcyOut);
HRESULT WINAPI VarCyFromStr32(OLECHAR32* strIn, LCID lcid, ULONG dwFlags, CY* pcyOut);
HRESULT WINAPI VarCyFromDisp32(IDispatch* pdispIn, LCID lcid, CY* pcyOut);
*/

HRESULT WINAPI VarBstrFromUI1(BYTE bVal, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromI2(short iVal, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromI4(LONG lIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromR4(FLOAT fltIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromR8(double dblIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromCy(CY cyIn, LCID lcid, ULONG dwFlags, BSTR *pbstrOut);
HRESULT WINAPI VarBstrFromDate(DATE dateIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromBool(VARIANT_BOOL boolIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
HRESULT WINAPI VarBstrFromI1(CHAR cIn, LCID lcid, ULONG dwFlags, BSTR*pbstrOut);
HRESULT WINAPI VarBstrFromUI2(USHORT uiIn, LCID lcid, ULONG dwFlags, BSTR*pbstrOut);
HRESULT WINAPI VarBstrFromUI4(ULONG ulIn, LCID lcid, ULONG dwFlags, BSTR*pbstrOut);
HRESULT WINAPI VarBstrFromCy(CY cyIn, LCID lcid, ULONG dwFlags, BSTR* pbstrOut);
/*
HRESULT WINAPI VarBstrFromDec32(DECIMAL*pdecIn, LCID lcid, ULONG dwFlags, BSTR32*pbstrOut);
HRESULT WINAPI VarBstrFromDisp32(IDispatch* pdispIn, LCID lcid, ULONG dwFlags, BSTR32* pbstrOut);
*/

HRESULT WINAPI VarBoolFromUI1(BYTE bIn, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromI2(short sIn, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromI4(LONG lIn, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromR4(FLOAT fltIn, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromR8(double dblIn, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromDate(DATE dateIn, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromStr(OLECHAR* strIn, LCID lcid, ULONG dwFlags, VARIANT_BOOL* pboolOut);
HRESULT WINAPI VarBoolFromI1(CHAR cIn, VARIANT_BOOL*pboolOut);
HRESULT WINAPI VarBoolFromUI2(USHORT uiIn, VARIANT_BOOL*pboolOut);
HRESULT WINAPI VarBoolFromUI4(ULONG ulIn, VARIANT_BOOL*pboolOut);
HRESULT WINAPI VarBoolFromCy(CY cyIn, VARIANT_BOOL* pboolOut);
/*
HRESULT WINAPI VarBoolFromDec32(DECIMAL*pdecIn, VARIANT_BOOL*pboolOut);
HRESULT WINAPI VarBoolFromDisp32(IDispatch* pdispIn, LCID lcid, VARIANT_BOOL* pboolOut);
*/

HRESULT WINAPI VarI1FromUI1(BYTE bIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromI2(short uiIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromI4(LONG lIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromR4(FLOAT fltIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromR8(double dblIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromDate(DATE dateIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromStr(OLECHAR*strIn, LCID lcid, ULONG dwFlags, CHAR*pcOut);
HRESULT WINAPI VarI1FromBool(VARIANT_BOOL boolIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromUI2(USHORT uiIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromUI4(ULONG ulIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromCy(CY cyIn, CHAR*pcOut);
/*
HRESULT WINAPI VarI1FromDec32(DECIMAL*pdecIn, CHAR*pcOut);
HRESULT WINAPI VarI1FromDisp32(IDispatch*pdispIn, LCID lcid, CHAR*pcOut);
*/

HRESULT WINAPI VarUI2FromUI1(BYTE bIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromI2(short uiIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromI4(LONG lIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromR4(FLOAT fltIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromR8(double dblIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromDate(DATE dateIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromStr(OLECHAR*strIn, LCID lcid, ULONG dwFlags, USHORT*puiOut);
HRESULT WINAPI VarUI2FromBool(VARIANT_BOOL boolIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromI1(CHAR cIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromUI4(ULONG ulIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromCy(CY cyIn, USHORT*puiOut);
/*
HRESULT WINAPI VarUI2FromDec32(DECIMAL*pdecIn, USHORT*puiOut);
HRESULT WINAPI VarUI2FromDisp32(IDispatch*pdispIn, LCID lcid, USHORT*puiOut);
*/

HRESULT WINAPI VarUI4FromStr(OLECHAR*strIn, LCID lcid, ULONG dwFlags, ULONG*pulOut);
HRESULT WINAPI VarUI4FromUI1(BYTE bIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromI2(short uiIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromI4(LONG lIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromR4(FLOAT fltIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromR8(double dblIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromDate(DATE dateIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromBool(VARIANT_BOOL boolIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromI1(CHAR cIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromUI2(USHORT uiIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromCy(CY cyIn, ULONG*pulOut);
/*
HRESULT WINAPI VarUI4FromDec32(DECIMAL*pdecIn, ULONG*pulOut);
HRESULT WINAPI VarUI4FromDisp32(IDispatch*pdispIn, LCID lcid, ULONG*pulOut);

HRESULT WINAPI VarDecFromUI132(BYTE bIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromI232(short uiIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromI432(LONG lIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromR432(FLOAT fltIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromR832(double dblIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromDate32(DATE dateIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromStr32(OLECHAR32*strIn, LCID lcid, ULONG dwFlags, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromBool32(VARIANT_BOOL boolIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromI132(CHAR cIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromUI232(USHORT uiIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromUI432(ULONG ulIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromCy32(CY cyIn, DECIMAL*pdecOut);
HRESULT WINAPI VarDecFromDisp32(IDispatch*pdispIn, LCID lcid, DECIMAL*pdecOut);
*/



#define VarUI4FromUI4( in, pOut ) ( *(pOut) =  (in) )
#define VarI4FromI4( in, pOut )   ( *(pOut) =  (in) )

#define VarUI1FromInt		VarUI1FromI4
#define VarUI1FromUint	VarUI1FromUI4
#define VarI2FromInt		VarI2FromI4
#define VarI2FromUint		VarI2FromUI4
#define VarI4FromInt		VarI4FromI432
#define VarI4FromUint		VarI4FromUI4
#define VarR4FromInt		VarR4FromI4
#define VarR4FromUint		VarR4FromUI4
#define VarR8FromInt		VarR8FromI4
#define VarR8FromUint		VarR8FromUI4
#define VarDateFromInt	VarDateFromI4
#define VarDateFromUint	VarDateFromUI4
#define VarCyFromInt		VarCyFromI4
#define VarCyFromUint		VarCyFromUI4
#define VarBstrFromInt	VarBstrFromI4
#define VarBstrFromUint	VarBstrFromUI4
#define VarBoolFromInt	VarBoolFromI4
#define VarBoolFromUint	VarBoolFromUI4
#define VarI1FromInt		VarI1FromI4
#define VarI1FromUint		VarI1FromUI4
#define VarUI2FromInt		VarUI2FromI4
#define VarUI2FromUint	VarUI2FromUI4
#define VarUI4FromInt		VarUI4FromI4
#define VarUI4FromUint	VarUI4FromUI432
#define VarDecFromInt		VarDecFromI432
#define VarDecFromUint	VarDecFromUI432
#define VarIntFromUI1		VarI4FromUI1
#define VarIntFromI2		VarI4FromI2
#define VarIntFromI4		VarI4FromI432
#define VarIntFromR4		VarI4FromR4
#define VarIntFromR8		VarI4FromR8
#define VarIntFromDate	VarI4FromDate
#define VarIntFromCy		VarI4FromCy
#define VarIntFromStr		VarI4FromStr
#define VarIntFromDisp	VarI4FromDisp32
#define VarIntFromBool	VarI4FromBool
#define VarIntFromI1		VarI4FromI1
#define VarIntFromUI2		VarI4FromUI2
#define VarIntFromUI4		VarI4FromUI4
#define VarIntFromDec		VarI4FromDec32
#define VarIntFromUint	VarI4FromUI4
#define VarUintFromUI1	VarUI4FromUI1
#define VarUintFromI2		VarUI4FromI2
#define VarUintFromI4		VarUI4FromI4
#define VarUintFromR4		VarUI4FromR4
#define VarUintFromR8		VarUI4FromR8
#define VarUintFromDate	VarUI4FromDate
#define VarUintFromCy		VarUI4FromCy
#define VarUintFromStr	VarUI4FromStr
#define VarUintFromDisp	VarUI4FromDisp32
#define VarUintFromBool	VarUI4FromBool
#define VarUintFromI1		VarUI4FromI1
#define VarUintFromUI2	VarUI4FromUI2
#define VarUintFromUI4	VarUI4FromUI432
#define VarUintFromDec	VarUI4FromDec32
#define VarUintFromInt	VarUI4FromI4

#ifdef __cplusplus
} // extern "C"
#endif 

typedef struct tagPARAMDATA {
    OLECHAR16 * szName;    /* parameter name */
    VARTYPE vt;         /* parameter type */
} PARAMDATA, * LPPARAMDATA;

typedef struct tagMETHODDATA {
    OLECHAR16 * szName;    /* method name */
    PARAMDATA * ppdata;  /* pointer to an array of PARAMDATAs */
    DISPID dispid;      /* method ID */
    UINT16 iMeth;         /* method index */
    CALLCONV cc;        /* calling convention */
    UINT16 cArgs;         /* count of arguments */
    WORD wFlags;        /* same wFlags as on IDispatch::Invoke() */
    VARTYPE vtReturn;
} METHODDATA, * LPMETHODDATA;

typedef struct tagINTERFACEDATA {
    METHODDATA * pmethdata;  /* pointer to an array of METHODDATAs */
    UINT16 cMembers;      /* count of members */
} INTERFACEDATA, * LPINTERFACEDATA;

#endif /*__WINE_OLEAUTO_H*/
