/*
 * Defines the COM interfaces and APIs related to OLE automation support.
 *
 * Depends on 'obj_base.h'.
 */

#ifndef __WINE_WINE_OBJ_OLEAUT_H
#define __WINE_WINE_OBJ_OLEAUT_H

#include "windows.h"
#include "wintypes.h"

/*****************************************************************************
 * Predeclare the interfaces
 */
DEFINE_OLEGUID(IID_IDispatch,       0x00020400,0,0);
typedef struct IDispatch IDispatch,*LPDISPATCH;

DEFINE_OLEGUID(IID_ITypeInfo,       0x00020401,0,0);
typedef struct ITypeInfo ITypeInfo,*LPTYPEINFO;

DEFINE_OLEGUID(IID_ITypeLib,        0x00020402,0,0);
typedef struct ITypeLib ITypeLib,*LPTYPELIB;

DEFINE_OLEGUID(IID_ITypeComp,       0x00020403,0,0);
typedef struct ITypeComp ITypeComp,*LPTYPECOMP;

DEFINE_OLEGUID(IID_IEnumVariant,    0x00020404,0,0);
typedef struct IenumVariant IEnumVariant,*LPENUMVARIANT;

DEFINE_OLEGUID(IID_ICreateTypeInfo, 0x00020405,0,0);
typedef struct ICreateTypeInfo ICreateTypeInfo,*LPCREATETYPEINFO;

DEFINE_OLEGUID(IID_ICreateTypeLib,  0x00020406,0,0);
typedef struct ICreateTypeLib ICreateTypeLib,*LPCREATETYPELIB;

DEFINE_OLEGUID(IID_ICreateTypeInfo2,0x0002040E,0,0);
typedef struct ICreateTypeInfo2 ICreateTypeInfo2,*LPCREATETYPEINFO2;

DEFINE_OLEGUID(IID_ICreateTypeLib2, 0x0002040F,0,0);
typedef struct ICreateTypeLib2 ICreateTypeLib2,*LPCREATETYPELIB2;

DEFINE_OLEGUID(IID_ITypeChangeEvents,0x00020410,0,0);
typedef struct ITypeChangeEvents ITypeChangeEvents,*LPTYPECHANGEEVENTS;

DEFINE_OLEGUID(IID_ITypeLib2,       0x00020411,0,0);
typedef struct ITypeLib2 ITypeLib2,*LPTYPELIB2;

DEFINE_OLEGUID(IID_ITypeInfo2,      0x00020412,0,0);
typedef struct ITypeInfo2 ITypeInfo2,*LPTYPEINFO2;

DEFINE_GUID(IID_IErrorInfo,         0x1CF2B120,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
typedef struct IErrorInfo IErrorInfo,*LPERRORINFO;

DEFINE_GUID(IID_ICreateErrorInfo,   0x22F03340,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
typedef struct ICreateErrorInfo ICreateErrorInfo,*LPCREATEERRORINFO;

DEFINE_GUID(IID_ISupportErrorInfo,  0xDF0B3D60,0x547D,0x101B,0x8E,0x65,
        0x08,0x00, 0x2B,0x2B,0xD1,0x19);
typedef struct ISupportErrorInfo ISupportErrorInfo,*LPSUPPORTERRORINFO;

/*****************************************************************************
 * Automation data types
 */

/*
 * Data types for Variants.
 */

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

#ifdef __cplusplus
#define _wine_tagVARIANT_UNION_NAME 
#else
#define _wine_tagVARIANT_UNION_NAME u
#endif
/* end FIXME */

typedef struct tagVARIANT VARIANT, *LPVARIANT;
typedef struct tagVARIANT VARIANTARG, *LPVARIANTARG;

struct tagVARIANT {
	VARTYPE vt;
	WORD wReserved1;
	WORD wReserved2;
	WORD wReserved3;
  union _wine_tagVARIANT_UNION_NAME 
	{
		/* By value.
		 */
		CHAR cVal;
		USHORT uiVal;
		ULONG ulVal;
		INT intVal;
		UINT uintVal;
		BYTE bVal;
		short iVal;
		long lVal;
		float fltVal;
		double dblVal;
		VARIANT_BOOL boolVal;
		SCODE scode;
		DATE date;
		BSTR bstrVal;
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
		INT* pintVal;
		UINT* puintVal;
		BYTE* pbVal;
		short* piVal;
		long* plVal;
		float* pfltVal;
		double* pdblVal;
		VARIANT_BOOL* pboolVal;
		SCODE* pscode;
		DATE* pdate;
		BSTR* pbstrVal;
		VARIANT* pvarVal;
		PVOID byref;
		CY* pcyVal;
        /*
        DECIMAL* pdecVal;
		IUnknown** ppunkVal;
		IDispatch** ppdispVal;
        SAFEARRAY** pparray;
        */
  } _wine_tagVARIANT_UNION_NAME;
};

typedef LONG DISPID;

typedef struct  tagDISPPARAMS
{
  VARIANTARG* rgvarg;
  DISPID*     rgdispidNamedArgs;
  UINT      cArgs;
  UINT      cNamedArgs;
} DISPPARAMS;

typedef struct tagEXCEPINFO {
    WORD  wCode;
    WORD  wReserved;
    BSTR  bstrSource;
    BSTR  bstrDescription;
    BSTR  bstrHelpFile;
    DWORD dwHelpContext;
    PVOID pvReserved;
    HRESULT (__stdcall *pfnDeferredFillIn)(struct tagEXCEPINFO *);
    SCODE scode;
} EXCEPINFO, * LPEXCEPINFO;

/*****************************************************************************
 * IDispatch interface
 */
#define ICOM_INTERFACE IDispatch
#define IDispatch_METHODS \
  ICOM_METHOD1(HRESULT, GetTypeInfoCount, UINT*, pctinfo); \
  ICOM_METHOD3(HRESULT, GetTypeInfo, UINT, iTInfo, LCID, lcid, ITypeInfo**, ppTInfo); \
  ICOM_METHOD5(HRESULT, GetIDsOfNames, REFIID, riid, LPOLESTR*, rgszNames, UINT, cNames, LCID, lcid, DISPID*, rgDispId); \
  ICOM_METHOD8(HRESULT, Invoke, DISPID, dispIdMember, REFIID, riid, LCID, lcid, WORD, wFlags, DISPPARAMS*, pDispParams, VARIANT*, pVarResult, EXCEPINFO*, pExepInfo, UINT*, puArgErr); 
#define IDispatch_IMETHODS \
  ICOM_INHERITS(IDispatch,IUnknown)
ICOM_DEFINE(IDispatch,IUnknown)
#undef ICOM_INTERFACE

#ifdef ICOM_CINTERFACE
/*** IUnknown methods ***/
#define IDispatch_QueryInterface(p,a,b) ICOM_CALL2(QueryInterface,p,a,b)
#define IDispatch_AddRef(p)             ICOM_CALL (AddRef,p)
#define IDispatch_Release(p)            ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IDispatch_GetTypeInfoCount(p,a)      ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IDispatch_GetTypeInfo(p,a,b,c)       ICOM_CALL3 (GetTypeInfo,p,b,c)
#define IDispatch_GetIDsOfNames(p,a,b,c,d,e) ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IDispatch_Invoke(p,a,b,c,d,e,f,g,h)  ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
#endif


#endif /* __WINE_WINE_OBJ_OLEAUT_H */

