/*
 * Defines the COM interfaces and APIs related to ErrorInfo
 */

#ifndef __WINE_WINE_OBJ_ERRORINFO_H
#define __WINE_WINE_OBJ_ERRORINFO_H

#include "wine/obj_base.h"
#include "wine/obj_oleaut.h"

DEFINE_GUID(IID_IErrorInfo,0x1CF2B120,0x547D,0x101B,0x8E,0x65,0x08,0x00,0x2B,0x2B,0xD1,0x19);
typedef struct IErrorInfo IErrorInfo,*LPERRORINFO;

DEFINE_GUID(IID_ICreateErrorInfo,0x22F03340,0x547D,0x101B,0x8E,0x65,0x08,0x00,0x2B,0x2B,0xD1,0x19);
typedef struct ICreateErrorInfo ICreateErrorInfo,*LPCREATEERRORINFO;

DEFINE_GUID(IID_ISupportErrorInfo,0xDF0B3D60,0x547D,0x101B,0x8E,0x65,0x08,0x00,0x2B,0x2B,0xD1,0x19);
typedef struct ISupportErrorInfo ISupportErrorInfo,*LPSUPPORTERRORINFO;

/*****************************************************************************
 * IErrorInfo
 */
#define ICOM_INTERFACE IErrorInfo
#define IErrorInfo_METHODS \
  ICOM_METHOD1(HRESULT, GetGUID, const GUID ** , pGUID) \
  ICOM_METHOD1(HRESULT, GetSource, BSTR* ,pBstrSource) \
  ICOM_METHOD1(HRESULT, GetDescription, BSTR*, pBstrDescription) \
  ICOM_METHOD1(HRESULT, GetHelpFile, BSTR*, pBstrHelpFile) \
  ICOM_METHOD1(HRESULT, GetHelpContext, DWORD*, pdwHelpContext)
#define IErrorInfo_IMETHODS \
	IUnknown_IMETHODS \
	IDispatch_IMETHODS \
	IErrorInfo_METHODS

ICOM_DEFINE(IErrorInfo, IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define IErrorInfo_QueryInterface(p,a,b)	ICOM_CALL2(QueryInterface,p,a,b)
#define IErrorInfo_AddRef(p)			ICOM_CALL (AddRef,p)
#define IErrorInfo_Release(p)			ICOM_CALL (Release,p)
/*** IDispatch methods ***/
#define IErrorInfo_GetTypeInfoCount(p,a)	ICOM_CALL1 (GetTypeInfoCount,p,a)
#define IErrorInfo_GetTypeInfo(p,a,b,c)		ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define IErrorInfo_GetIDsOfNames(p,a,b,c,d,e)	ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define IErrorInfo_Invoke(p,a,b,c,d,e,f,g,h)	ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** IErrorInfo methods ***/
#define IErrorInfo_GetGUID(p,a)		ICOM_CALL1 (GetGUID,p,a)
#define IErrorInfo_GetSource(p,a)	ICOM_CALL1 (GetSource,p,a)
#define IErrorInfo_GetDescription(p,a)	ICOM_CALL1 (GetDescription,p,a)
#define IErrorInfo_GetHelpFile(p,a)	ICOM_CALL1 (GetHelpFile,p,a)
#define IErrorInfo_GetHelpContext(p,a)	ICOM_CALL1 (GetHelpContext,p,a)

/*****************************************************************************
 * ICreateErrorInfo
 */
#define ICOM_INTERFACE ICreateErrorInfo
#define ICreateErrorInfo_METHODS \
  ICOM_METHOD1(HRESULT, SetGUID, REFGUID, rguid) \
  ICOM_METHOD1(HRESULT, SetSource, LPOLESTR, szSource) \
  ICOM_METHOD1(HRESULT, SetDescription, LPOLESTR, szDescription) \
  ICOM_METHOD1(HRESULT, SetHelpFile, LPOLESTR, szHelpFile) \
  ICOM_METHOD1(HRESULT, SetHelpContext, DWORD, dwHelpContext)
#define ICreateErrorInfo_IMETHODS \
	IUnknown_IMETHODS \
	IDispatch_IMETHODS \
	ICreateErrorInfo_METHODS

ICOM_DEFINE(ICreateErrorInfo, IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ICreateErrorInfo_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define ICreateErrorInfo_AddRef(p)			ICOM_CALL (AddRef,p)
#define ICreateErrorInfo_Release(p)			ICOM_CALL (Release,p)
/*** ICreateErrorInfo methods ***/
#define ICreateErrorInfo_GetTypeInfoCount(p,a)		ICOM_CALL1 (GetTypeInfoCount,p,a)
#define ICreateErrorInfo_GetTypeInfo(p,a,b,c)		ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define ICreateErrorInfo_GetIDsOfNames(p,a,b,c,d,e)	ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define ICreateErrorInfo_Invoke(p,a,b,c,d,e,f,g,h)	ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** ICreateErrorInfo methods ***/
#define ICreateErrorInfo_SetGUID(p,a)		ICOM_CALL1 (SetGUID,p,a)
#define ICreateErrorInfo_SetSource(p,a)		ICOM_CALL1 (SetSource,p,a)
#define ICreateErrorInfo_SetDescription(p,a)	ICOM_CALL1 (SetDescription,p,a)
#define ICreateErrorInfo_SetHelpFile(p,a)	ICOM_CALL1 (SetHelpFile,p,a)
#define ICreateErrorInfo_SetHelpContext(p,a)	ICOM_CALL1 (SetHelpContext,p,a)

/*****************************************************************************
 * ISupportErrorInfo
 */
#define ICOM_INTERFACE ISupportErrorInfo
#define ISupportErrorInfo_METHODS \
  ICOM_METHOD1(HRESULT, InterfaceSupportsErrorInfo,  REFIID,  riid  )
#define ISupportErrorInfo_IMETHODS \
	IUnknown_IMETHODS \
	IDispatch_IMETHODS \
	ISupportErrorInfo_METHODS

ICOM_DEFINE(ISupportErrorInfo, IDispatch)
#undef ICOM_INTERFACE

/*** IUnknown methods ***/
#define ISupportErrorInfo_QueryInterface(p,a,b)		ICOM_CALL2(QueryInterface,p,a,b)
#define ISupportErrorInfo_AddRef(p)			ICOM_CALL (AddRef,p)
#define ISupportErrorInfo_Release(p)			ICOM_CALL (Release,p)
/*** ISupportErrorInfo methods ***/
#define ISupportErrorInfo_GetTypeInfoCount(p,a)		ICOM_CALL1 (GetTypeInfoCount,p,a)
#define ISupportErrorInfo_GetTypeInfo(p,a,b,c)		ICOM_CALL3 (GetTypeInfo,p,a,b,c)
#define ISupportErrorInfo_GetIDsOfNames(p,a,b,c,d,e)	ICOM_CALL5 (GetIDsOfNames,p,a,b,c,d,e)
#define ISupportErrorInfo_Invoke(p,a,b,c,d,e,f,g,h)	ICOM_CALL8 (Invoke,p,a,b,c,d,e,f,g,h)
/*** ISupportErrorInfo methods ***/
#define ISupportErrorInfo_InterfaceSupportsErrorInfo(p,a)	ICOM_CALL1 (InterfaceSupportsErrorInfo,p,a)


#endif /* __WINE_WINE_OBJ_ERRORINFO_H */
