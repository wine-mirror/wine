/*
 *	IEnumFORMATETC, IDataObject
 *
 * selecting and droping objects within the shell and/or common dialogs
 *
 *	Copyright 1998, 1999	<juergen.schmied@metronet.de>
 */
#include <string.h>

#include "pidl.h"
#include "winerror.h"
#include "shell32_main.h"
#include "debugtools.h"
#include "wine/undocshell.h"
#include "wine/obj_dataobject.h"

DEFAULT_DEBUG_CHANNEL(shell)

/***********************************************************************
*   IEnumFORMATETC implementation
*/

typedef struct 
{
    /* IUnknown fields */
    ICOM_VFIELD(IEnumFORMATETC);
    DWORD                        ref;
    /* IEnumFORMATETC fields */
    UINT        posFmt;
    UINT        countFmt;
    LPFORMATETC pFmt;
} IEnumFORMATETCImpl;

static HRESULT WINAPI IEnumFORMATETC_fnQueryInterface(LPENUMFORMATETC iface, REFIID riid, LPVOID* ppvObj);
static ULONG WINAPI IEnumFORMATETC_fnAddRef(LPENUMFORMATETC iface);
static ULONG WINAPI IEnumFORMATETC_fnRelease(LPENUMFORMATETC iface);
static HRESULT WINAPI IEnumFORMATETC_fnNext(LPENUMFORMATETC iface, ULONG celt, FORMATETC* rgelt, ULONG* pceltFethed);
static HRESULT WINAPI IEnumFORMATETC_fnSkip(LPENUMFORMATETC iface, ULONG celt);
static HRESULT WINAPI IEnumFORMATETC_fnReset(LPENUMFORMATETC iface);
static HRESULT WINAPI IEnumFORMATETC_fnClone(LPENUMFORMATETC iface, LPENUMFORMATETC* ppenum);

static struct ICOM_VTABLE(IEnumFORMATETC) efvt = 
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
        IEnumFORMATETC_fnQueryInterface,
        IEnumFORMATETC_fnAddRef,
    IEnumFORMATETC_fnRelease,
    IEnumFORMATETC_fnNext,
    IEnumFORMATETC_fnSkip,
    IEnumFORMATETC_fnReset,
    IEnumFORMATETC_fnClone
};

LPENUMFORMATETC IEnumFORMATETC_Constructor(UINT cfmt, const FORMATETC afmt[])
{
	IEnumFORMATETCImpl* ef;
	DWORD size=cfmt * sizeof(FORMATETC);
	
	ef=(IEnumFORMATETCImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IEnumFORMATETCImpl));

	if(ef)
	{
	  ef->ref=1;
	  ICOM_VTBL(ef)=&efvt;

	  ef->countFmt = cfmt;
	  ef->pFmt = SHAlloc (size);

	  if (ef->pFmt)
	  {
	    memcpy(ef->pFmt, afmt, size);
	  }

	  shell32_ObjCount++;
	}

	TRACE("(%p)->(%u,%p)\n",ef, cfmt, afmt);
	return (LPENUMFORMATETC)ef;
}

static HRESULT WINAPI IEnumFORMATETC_fnQueryInterface(LPENUMFORMATETC iface, REFIID riid, LPVOID* ppvObj)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))
	{
	  *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IEnumFORMATETC))
	{
	  *ppvObj = (IEnumFORMATETC*)This;
	}   

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)(*ppvObj));
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;

}

static ULONG WINAPI IEnumFORMATETC_fnAddRef(LPENUMFORMATETC iface)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	TRACE("(%p)->(count=%lu)\n",This, This->ref);
	shell32_ObjCount++;
	return ++(This->ref);
}

static ULONG WINAPI IEnumFORMATETC_fnRelease(LPENUMFORMATETC iface)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	TRACE("(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{
	  TRACE(" destroying IEnumFORMATETC(%p)\n",This);
	  if (This->pFmt)
	  {
	    SHFree (This->pFmt);
	  }
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

static HRESULT WINAPI IEnumFORMATETC_fnNext(LPENUMFORMATETC iface, ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	int i;

	TRACE("(%p)->(%lu,%p)\n", This, celt, rgelt);

	if(!This->pFmt)return S_FALSE;
	if(!rgelt) return E_INVALIDARG;
	if (pceltFethed)  *pceltFethed = 0;

	for(i = 0; This->posFmt < This->countFmt && celt > i; i++)
   	{
	  *rgelt++ = This->pFmt[This->posFmt++];
	}

	if (pceltFethed) *pceltFethed = i;

	return ((i == celt) ? S_OK : S_FALSE);
}

static HRESULT WINAPI IEnumFORMATETC_fnSkip(LPENUMFORMATETC iface, ULONG celt)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	TRACE("(%p)->(num=%lu)\n", This, celt);

	if((This->posFmt + celt) >= This->countFmt) return S_FALSE;
	This->posFmt += celt;
	return S_OK;
}

static HRESULT WINAPI IEnumFORMATETC_fnReset(LPENUMFORMATETC iface)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	TRACE("(%p)->()\n", This);

        This->posFmt = 0;
        return S_OK;
}

static HRESULT WINAPI IEnumFORMATETC_fnClone(LPENUMFORMATETC iface, LPENUMFORMATETC* ppenum)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	TRACE("(%p)->(ppenum=%p)\n", This, ppenum);

	if (!ppenum) return E_INVALIDARG;
	*ppenum = IEnumFORMATETC_Constructor(This->countFmt, This->pFmt);
	return S_OK;
}


/***********************************************************************
*   IDataObject implementation
*/

/* number of supported formats */
#define MAX_FORMATS 3

typedef struct
{
	/* IUnknown fields */
	ICOM_VFIELD(IDataObject);
	DWORD		ref;

	/* IDataObject fields */
	LPITEMIDLIST	pidl;
	LPITEMIDLIST *	apidl;
	UINT		cidl;

	FORMATETC	pFormatEtc[MAX_FORMATS];
	UINT		cfShellIDList;
	UINT		cfFileName;

} IDataObjectImpl;

static struct ICOM_VTABLE(IDataObject) dtovt;

/**************************************************************************
*  IDataObject_Constructor
*/
LPDATAOBJECT IDataObject_Constructor(HWND hwndOwner, LPITEMIDLIST pMyPidl, LPITEMIDLIST * apidl, UINT cidl)
{
	IDataObjectImpl* dto;

	dto = (IDataObjectImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDataObjectImpl));

	if (dto)
	{
	  dto->ref = 1;
	  ICOM_VTBL(dto) = &dtovt;
	  dto->pidl = ILClone(pMyPidl);
	  dto->apidl = _ILCopyaPidl(apidl, cidl);
	  dto->cidl = cidl;

	  dto->cfShellIDList = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
	  dto->cfFileName = RegisterClipboardFormatA(CFSTR_FILENAMEA);
	  InitFormatEtc(dto->pFormatEtc[0], dto->cfShellIDList, TYMED_HGLOBAL);
	  InitFormatEtc(dto->pFormatEtc[1], CF_HDROP, TYMED_HGLOBAL);
	  InitFormatEtc(dto->pFormatEtc[2], dto->cfFileName, TYMED_HGLOBAL);

	  shell32_ObjCount++;
	}
	
	TRACE("(%p)->(apidl=%p cidl=%u)\n",dto, apidl, cidl);
	return (LPDATAOBJECT)dto;
}

/***************************************************************************
*  IDataObject_QueryInterface
*/
static HRESULT WINAPI IDataObject_fnQueryInterface(LPDATAOBJECT iface, REFIID riid, LPVOID * ppvObj)
{
	ICOM_THIS(IDataObjectImpl,iface);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,debugstr_guid(riid),ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{
	  *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IDataObject))  /*IDataObject*/
	{
	  *ppvObj = (IDataObject*)This;
	}   

	if(*ppvObj)
	{
	  IUnknown_AddRef((IUnknown*)*ppvObj);      
	  TRACE("-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE("-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}

/**************************************************************************
*  IDataObject_AddRef
*/
static ULONG WINAPI IDataObject_fnAddRef(LPDATAOBJECT iface)
{
	ICOM_THIS(IDataObjectImpl,iface);

	TRACE("(%p)->(count=%lu)\n",This, This->ref);

	shell32_ObjCount++;
	return ++(This->ref);
}

/**************************************************************************
*  IDataObject_Release
*/
static ULONG WINAPI IDataObject_fnRelease(LPDATAOBJECT iface)
{
	ICOM_THIS(IDataObjectImpl,iface);
	TRACE("(%p)->()\n",This);

	shell32_ObjCount--;

	if (!--(This->ref)) 
	{
	  TRACE(" destroying IDataObject(%p)\n",This);
	  _ILFreeaPidl(This->apidl, This->cidl);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}

/**************************************************************************
* IDataObject_fnGetData
*/
static HRESULT WINAPI IDataObject_fnGetData(LPDATAOBJECT iface, LPFORMATETC pformatetcIn, STGMEDIUM *pmedium)
{
	ICOM_THIS(IDataObjectImpl,iface);

	char	szTemp[256];
	
	szTemp[0]=0;
	GetClipboardFormatNameA (pformatetcIn->cfFormat, szTemp, 256);
	TRACE("(%p)->(%p %p format=%s)\n", This, pformatetcIn, pmedium, szTemp);

	if (pformatetcIn->cfFormat == This->cfShellIDList)
	{
	  if (This->cidl < 1) return(E_UNEXPECTED);
	  pmedium->u.hGlobal = RenderSHELLIDLIST(This->pidl, This->apidl, This->cidl);
	}
	else if	(pformatetcIn->cfFormat == CF_HDROP)
	{
	  if (This->cidl < 1) return(E_UNEXPECTED);
	  pmedium->u.hGlobal = RenderHDROP(This->pidl, This->apidl, This->cidl);
	}
	else if	(pformatetcIn->cfFormat == This->cfFileName)
	{
	  if (This->cidl < 1) return(E_UNEXPECTED);
	  pmedium->u.hGlobal = RenderFILENAME(This->pidl, This->apidl, This->cidl);
	}
	else
	{
	  FIXME("-- expected clipformat not implemented\n");
	  return (E_INVALIDARG);
	}
	if (pmedium->u.hGlobal)
	{
	  pmedium->tymed = TYMED_HGLOBAL;
	  pmedium->pUnkForRelease = NULL;
	  return S_OK;
	}
	return E_OUTOFMEMORY;
}

static HRESULT WINAPI IDataObject_fnGetDataHere(LPDATAOBJECT iface, LPFORMATETC pformatetc, STGMEDIUM *pmedium)
{
	ICOM_THIS(IDataObjectImpl,iface);
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnQueryGetData(LPDATAOBJECT iface, LPFORMATETC pformatetc)
{
	ICOM_THIS(IDataObjectImpl,iface);
	UINT i;
	
	TRACE("(%p)->(fmt=0x%08x tym=0x%08lx)\n", This, pformatetc->cfFormat, pformatetc->tymed);
	
	if(!(DVASPECT_CONTENT & pformatetc->dwAspect))
	  return DV_E_DVASPECT;

	/* check our formats table what we have */
	for (i=0; i<MAX_FORMATS; i++)
	{
	  if ((This->pFormatEtc[i].cfFormat == pformatetc->cfFormat)
	   && (This->pFormatEtc[i].tymed == pformatetc->tymed))
	  {
	    return S_OK;
	  }
	}

	return DV_E_TYMED;
}

static HRESULT WINAPI IDataObject_fnGetCanonicalFormatEtc(LPDATAOBJECT iface, LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut)
{
	ICOM_THIS(IDataObjectImpl,iface);
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnSetData(LPDATAOBJECT iface, LPFORMATETC pformatetc, STGMEDIUM *pmedium, BOOL fRelease)
{
	ICOM_THIS(IDataObjectImpl,iface);
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnEnumFormatEtc(LPDATAOBJECT iface, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
	ICOM_THIS(IDataObjectImpl,iface);

	TRACE("(%p)->()\n", This);
	*ppenumFormatEtc=NULL;

	/* only get data */
	if (DATADIR_GET == dwDirection)
	{
	  *ppenumFormatEtc = IEnumFORMATETC_Constructor(MAX_FORMATS, This->pFormatEtc);
	  return (*ppenumFormatEtc) ? S_OK : E_FAIL;
	}
	
	return E_NOTIMPL;
}

static HRESULT WINAPI IDataObject_fnDAdvise(LPDATAOBJECT iface, FORMATETC *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	ICOM_THIS(IDataObjectImpl,iface);
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnDUnadvise(LPDATAOBJECT iface, DWORD dwConnection)
{
	ICOM_THIS(IDataObjectImpl,iface);
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnEnumDAdvise(LPDATAOBJECT iface, IEnumSTATDATA **ppenumAdvise)
{
	ICOM_THIS(IDataObjectImpl,iface);
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
}

static struct ICOM_VTABLE(IDataObject) dtovt = 
{
	ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
	IDataObject_fnQueryInterface,
	IDataObject_fnAddRef,
	IDataObject_fnRelease,
	IDataObject_fnGetData,
	IDataObject_fnGetDataHere,
	IDataObject_fnQueryGetData,
	IDataObject_fnGetCanonicalFormatEtc,
	IDataObject_fnSetData,
	IDataObject_fnEnumFormatEtc,
	IDataObject_fnDAdvise,
	IDataObject_fnDUnadvise,
	IDataObject_fnEnumDAdvise
};

