/*
 * ErrorInfo API
 *
 * Copyright 2000 Patrik Stridvall, Juergen Schmied
 *
 *
 * The errorinfo is a per-thread object. The reference is stored in the 
 * TEB at offset 0xf80
 */

#include "debugtools.h"
#include "windef.h"
#include "heap.h"
#include "winerror.h"
#include "thread.h"
#include "debugtools.h"
#include "wine/obj_errorinfo.h"

DEFAULT_DEBUG_CHANNEL(ole)

typedef struct ErrorInfoImpl
{
	ICOM_VTABLE(IErrorInfo)		*lpvtei;
	ICOM_VTABLE(ICreateErrorInfo)	*lpvtcei;
	ICOM_VTABLE(ISupportErrorInfo)	*lpvtsei;
	DWORD				ref;
	
	const GUID *pGuid;
} ErrorInfoImpl;

static ICOM_VTABLE(IErrorInfo)		IErrorInfoImpl_VTable;
static ICOM_VTABLE(ICreateErrorInfo)	ICreateErrorInfoImpl_VTable;
static ICOM_VTABLE(ISupportErrorInfo)	ISupportErrorInfoImpl_VTable;

/*
 converts a objectpointer to This
 */
#define _IErrorInfo_Offset ((int)(&(((ErrorInfoImpl*)0)->lpvtei))) 
#define _ICOM_THIS_From_IErrorInfo(class, name) class* This = (class*)(((char*)name)-_IErrorInfo_Offset); 

#define _ICreateErrorInfo_Offset ((int)(&(((ErrorInfoImpl*)0)->lpvtcei))) 
#define _ICOM_THIS_From_ICreateErrorInfo(class, name) class* This = (class*)(((char*)name)-_ICreateErrorInfo_Offset); 

#define _ISupportErrorInfo_Offset ((int)(&(((ErrorInfoImpl*)0)->lpvtsei))) 
#define _ICOM_THIS_From_ISupportErrorInfo(class, name) class* This = (class*)(((char*)name)-_ISupportErrorInfo_Offset); 

/*
 converts This to a objectpointer
 */
#define _IErrorInfo_(This)		(IErrorInfo*)&(This->lpvtei)
#define _ICreateErrorInfo_(This)	(ICreateErrorInfo*)&(This->lpvtcei)
#define _ISupportErrorInfo_(This)	(ISupportErrorInfo*)&(This->lpvtsei)

IErrorInfo * IErrorInfoImpl_Constructor()
{
	ErrorInfoImpl * ei = HeapAlloc(GetProcessHeap(), 0, sizeof(ErrorInfoImpl));
	if (ei)
	{
	  ei->lpvtei = &IErrorInfoImpl_VTable;
	  ei->lpvtcei = &ICreateErrorInfoImpl_VTable;
	  ei->lpvtsei = &ISupportErrorInfoImpl_VTable;
	  ei->ref = 1;
	}
	return (IErrorInfo *)ei;
}


static HRESULT WINAPI IErrorInfoImpl_QueryInterface(
	IErrorInfo* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvoid);

	*ppvoid = NULL;

	if(IsEqualIID(riid, &IID_IErrorInfo))
	{
	  *ppvoid = _IErrorInfo_(This); 
	}
	else if(IsEqualIID(riid, &IID_ICreateErrorInfo))
	{
	  *ppvoid = _ICreateErrorInfo_(This);
	}
	else if(IsEqualIID(riid, &IID_ISupportErrorInfo))
	{
	  *ppvoid = _ISupportErrorInfo_(This);
	}

	if(*ppvoid)
	{
	  IUnknown_AddRef( (IUnknown*)*ppvoid );
	  TRACE("-- Interface: (%p)->(%p)\n",ppvoid,*ppvoid);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

static ULONG WINAPI IErrorInfoImpl_AddRef(
 	IErrorInfo* iface)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);
	return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI IErrorInfoImpl_Release(
	IErrorInfo* iface)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(count=%lu)\n",This,This->ref);

	if (!InterlockedDecrement(&This->ref))
	{
	  TRACE("-- destroying IErrorInfo(%p)\n",This);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IErrorInfoImpl_GetTypeInfoCount(
	IErrorInfo* iface,
	unsigned int* pctinfo)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IErrorInfoImpl_GetTypeInfo(
	IErrorInfo* iface,
	UINT iTInfo,
	LCID lcid,
	ITypeInfo** ppTInfo)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IErrorInfoImpl_GetIDsOfNames(
	IErrorInfo* iface,
	REFIID riid,
	LPOLESTR* rgszNames,
	UINT cNames,
	LCID lcid,
	DISPID* rgDispId)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IErrorInfoImpl_Invoke(
	IErrorInfo* iface,
	DISPID dispIdMember,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS* pDispParams,
	VARIANT* pVarResult,
	EXCEPINFO* pExepInfo,
	UINT* puArgErr)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

/* FIXME: is this OK? Original is GUID* pGUID! This can't work! (js) */
static HRESULT WINAPI IErrorInfoImpl_GetGUID(
	IErrorInfo* iface,
	GUID const ** pGUID) 
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p) check function prototype\n", This);

	if(!pGUID || !*pGUID)return E_INVALIDARG;
	*pGUID = This->pGuid;
	return S_OK;
}

static HRESULT WINAPI IErrorInfoImpl_GetSource(
	IErrorInfo* iface,
	BSTR *pBstrSource)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IErrorInfoImpl_GetDescription(
	IErrorInfo* iface,
	BSTR *pBstrDescription)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IErrorInfoImpl_GetHelpFile(
	IErrorInfo* iface,
	BSTR *pBstrHelpFile)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IErrorInfoImpl_GetHelpContext(
	IErrorInfo* iface,
	DWORD *pdwHelpContext)
{
	_ICOM_THIS_From_IErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static ICOM_VTABLE(IErrorInfo) IErrorInfoImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  IErrorInfoImpl_QueryInterface,
  IErrorInfoImpl_AddRef,
  IErrorInfoImpl_Release,
  IErrorInfoImpl_GetTypeInfoCount,
  IErrorInfoImpl_GetTypeInfo,
  IErrorInfoImpl_GetIDsOfNames,
  IErrorInfoImpl_Invoke,

  IErrorInfoImpl_GetGUID,
  IErrorInfoImpl_GetSource,
  IErrorInfoImpl_GetDescription,
  IErrorInfoImpl_GetHelpFile,
  IErrorInfoImpl_GetHelpContext
};


static HRESULT WINAPI ICreateErrorInfoImpl_QueryInterface(
	ICreateErrorInfo* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_QueryInterface(_IErrorInfo_(This), riid, ppvoid);
}

static ULONG WINAPI ICreateErrorInfoImpl_AddRef(
 	ICreateErrorInfo* iface)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_AddRef(_IErrorInfo_(This));
}

static ULONG WINAPI ICreateErrorInfoImpl_Release(
	ICreateErrorInfo* iface)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Release(_IErrorInfo_(This));
}

static HRESULT WINAPI ICreateErrorInfoImpl_GetTypeInfoCount(
	ICreateErrorInfo* iface,
	unsigned int* pctinfo)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_GetTypeInfoCount(_IErrorInfo_(This), pctinfo);
}

static HRESULT WINAPI ICreateErrorInfoImpl_GetTypeInfo(
	ICreateErrorInfo* iface,
	UINT iTInfo,
	LCID lcid,
	ITypeInfo** ppTInfo)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_GetTypeInfo(_IErrorInfo_(This),iTInfo,lcid,ppTInfo);
}

static HRESULT WINAPI ICreateErrorInfoImpl_GetIDsOfNames(
	ICreateErrorInfo* iface,
	REFIID riid,
	LPOLESTR* rgszNames,
	UINT cNames,
	LCID lcid,
	DISPID* rgDispId)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_GetIDsOfNames(_IErrorInfo_(This),riid,rgszNames,cNames,lcid,rgDispId);
}

static HRESULT WINAPI ICreateErrorInfoImpl_Invoke(
	ICreateErrorInfo* iface,
	DISPID dispIdMember,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS* pDispParams,
	VARIANT* pVarResult,
	EXCEPINFO* pExepInfo,
	UINT* puArgErr)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Invoke(_IErrorInfo_(This),dispIdMember,riid,lcid,wFlags,pDispParams,
		pVarResult,pExepInfo,puArgErr);
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetGUID(
	ICreateErrorInfo* iface,
	REFGUID rguid)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(%s)\n", This, debugstr_guid(rguid));
	This->pGuid = rguid;
	return S_OK;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetSource(
	ICreateErrorInfo* iface,
	LPOLESTR szSource)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetDescription(
	ICreateErrorInfo* iface,
	LPOLESTR szDescription)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetHelpFile(
	ICreateErrorInfo* iface,
	LPOLESTR szHelpFile)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static HRESULT WINAPI ICreateErrorInfoImpl_SetHelpContext(
	ICreateErrorInfo* iface,
 	DWORD dwHelpContext)
{
	_ICOM_THIS_From_ICreateErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return E_NOTIMPL;
}

static ICOM_VTABLE(ICreateErrorInfo) ICreateErrorInfoImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  ICreateErrorInfoImpl_QueryInterface,
  ICreateErrorInfoImpl_AddRef,
  ICreateErrorInfoImpl_Release,

  ICreateErrorInfoImpl_GetTypeInfoCount,
  ICreateErrorInfoImpl_GetTypeInfo,
  ICreateErrorInfoImpl_GetIDsOfNames,
  ICreateErrorInfoImpl_Invoke,

  ICreateErrorInfoImpl_SetGUID,
  ICreateErrorInfoImpl_SetSource,
  ICreateErrorInfoImpl_SetDescription,
  ICreateErrorInfoImpl_SetHelpFile,
  ICreateErrorInfoImpl_SetHelpContext
};

static HRESULT WINAPI ISupportErrorInfoImpl_QueryInterface(
	ISupportErrorInfo* iface,
	REFIID     riid,
	VOID**     ppvoid)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	FIXME("(%p)\n", This);
	
	return IErrorInfo_QueryInterface(_IErrorInfo_(This), riid, ppvoid);
}

static ULONG WINAPI ISupportErrorInfoImpl_AddRef(
 	ISupportErrorInfo* iface)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_AddRef(_IErrorInfo_(This));
}

static ULONG WINAPI ISupportErrorInfoImpl_Release(
	ISupportErrorInfo* iface)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Release(_IErrorInfo_(This));
}

static HRESULT WINAPI ISupportErrorInfoImpl_GetTypeInfoCount(
	ISupportErrorInfo* iface,
	unsigned int* pctinfo)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_GetTypeInfoCount(_IErrorInfo_(This), pctinfo);
}

static HRESULT WINAPI ISupportErrorInfoImpl_GetTypeInfo(
	ISupportErrorInfo* iface,
	UINT iTInfo,
	LCID lcid,
	ITypeInfo** ppTInfo)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_GetTypeInfo(_IErrorInfo_(This),iTInfo,lcid,ppTInfo);
}

static HRESULT WINAPI ISupportErrorInfoImpl_GetIDsOfNames(
	ISupportErrorInfo* iface,
	REFIID riid,
	LPOLESTR* rgszNames,
	UINT cNames,
	LCID lcid,
	DISPID* rgDispId)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_GetIDsOfNames(_IErrorInfo_(This),riid,rgszNames,cNames,lcid,rgDispId);
}

static HRESULT WINAPI ISupportErrorInfoImpl_Invoke(
	ISupportErrorInfo* iface,
	DISPID dispIdMember,
	REFIID riid,
	LCID lcid,
	WORD wFlags,
	DISPPARAMS* pDispParams,
	VARIANT* pVarResult,
	EXCEPINFO* pExepInfo,
	UINT* puArgErr)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)\n", This);
	return IErrorInfo_Invoke(_IErrorInfo_(This),dispIdMember,riid,lcid,wFlags,pDispParams,
		pVarResult,pExepInfo,puArgErr);
}

static HRESULT WINAPI ISupportErrorInfoImpl_InterfaceSupportsErrorInfo(
	ISupportErrorInfo* iface,
	REFIID riid)
{
	_ICOM_THIS_From_ISupportErrorInfo(ErrorInfoImpl, iface);
	TRACE("(%p)->(%s)\n", This, debugstr_guid(riid));
	return (riid == This->pGuid) ? S_OK : S_FALSE;
}

static ICOM_VTABLE(ISupportErrorInfo) ISupportErrorInfoImpl_VTable =
{
  ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
  ISupportErrorInfoImpl_QueryInterface,
  ISupportErrorInfoImpl_AddRef,
  ISupportErrorInfoImpl_Release,

  ISupportErrorInfoImpl_GetTypeInfoCount,
  ISupportErrorInfoImpl_GetTypeInfo,
  ISupportErrorInfoImpl_GetIDsOfNames,
  ISupportErrorInfoImpl_Invoke,

  ISupportErrorInfoImpl_InterfaceSupportsErrorInfo
};
/***********************************************************************
 *		CreateErrorInfo
 */
HRESULT WINAPI CreateErrorInfo(ICreateErrorInfo **pperrinfo)
{
	IErrorInfo * pei;
	HRESULT res;

	TRACE("(%p): stub:\n", pperrinfo);

	if(! pperrinfo || !*pperrinfo) return E_INVALIDARG;
	if(!(pei=IErrorInfoImpl_Constructor()))return E_OUTOFMEMORY;
	
	res = IErrorInfo_QueryInterface(pei, &IID_ICreateErrorInfo, (LPVOID*)pperrinfo);
	IErrorInfo_Release(pei);
	return res;
}

/***********************************************************************
 *		GetErrorInfo
 */
HRESULT WINAPI GetErrorInfo(ULONG dwReserved, IErrorInfo **pperrinfo)
{
	TRACE("(%ld, %p): stub:\n", dwReserved, pperrinfo);

	if(! pperrinfo || !*pperrinfo) return E_INVALIDARG;
	if(!(*pperrinfo = (IErrorInfo*)(NtCurrentTeb()->ErrorInfo))) return S_FALSE;

	/* clear thread error state */
	NtCurrentTeb()->ErrorInfo = NULL;
	return S_OK;
}

/***********************************************************************
 *		SetErrorInfo
 */
HRESULT WINAPI SetErrorInfo(ULONG dwReserved, IErrorInfo *perrinfo)
{
	IErrorInfo * pei;
	TRACE("(%ld, %p): stub:\n", dwReserved, perrinfo);

	/* release old errorinfo */
	pei = (IErrorInfo*)NtCurrentTeb()->ErrorInfo;
	if(pei) IErrorInfo_Release(pei);

	/* set to new value */
	NtCurrentTeb()->ErrorInfo = perrinfo;
	if(perrinfo) IErrorInfo_AddRef(perrinfo);
	return S_OK;
}
