/*
 *	IEnumFORMATETC, IDataObject
 *
 * selecting and droping objects within the shell and/or common dialogs
 *
 *	Copyright 1998	<juergen.schmied@metronet.de>
 */
#include "debug.h"
#include "shlobj.h"
#include "winerror.h"
#include "shell32_main.h"
/***********************************************************************
*   IEnumFORMATETC implementation
*/
static HRESULT WINAPI IEnumFORMATETC_QueryInterface (LPENUMFORMATETC this, REFIID riid, LPVOID * ppvObj);
static ULONG WINAPI IEnumFORMATETC_AddRef (LPENUMFORMATETC this);
static ULONG WINAPI IEnumFORMATETC_Release (LPENUMFORMATETC this);
static HRESULT WINAPI IEnumFORMATETC_Next(LPENUMFORMATETC this, ULONG celt, FORMATETC32 *rgelt, ULONG *pceltFethed);
static HRESULT WINAPI IEnumFORMATETC_Skip(LPENUMFORMATETC this, ULONG celt);
static HRESULT WINAPI IEnumFORMATETC_Reset(LPENUMFORMATETC this);
static HRESULT WINAPI IEnumFORMATETC_Clone(LPENUMFORMATETC this, LPENUMFORMATETC* ppenum);

static struct IEnumFORMATETC_VTable efvt = 
{	IEnumFORMATETC_QueryInterface,
	IEnumFORMATETC_AddRef,
	IEnumFORMATETC_Release,
	IEnumFORMATETC_Next,
	IEnumFORMATETC_Skip,
	IEnumFORMATETC_Reset,
	IEnumFORMATETC_Clone
};

extern LPENUMFORMATETC IEnumFORMATETC_Constructor(UINT32 cfmt, const FORMATETC32 afmt[])
{	LPENUMFORMATETC ef;
	DWORD size=cfmt * sizeof(FORMATETC32);
	
	ef=(LPENUMFORMATETC)HeapAlloc(GetProcessHeap(),0,sizeof(IEnumFORMATETC));
	ef->ref=1;
	ef->lpvtbl=&efvt;

	ef->posFmt = 0;
	ef->countFmt = cfmt;
	ef->pFmt = SHAlloc (size);

	if (ef->pFmt)
	{ memcpy(ef->pFmt, afmt, size);
	}

	TRACE(shell,"(%p)->()\n",ef);
	return ef;
}
static HRESULT WINAPI IEnumFORMATETC_QueryInterface (LPENUMFORMATETC this, REFIID riid, LPVOID * ppvObj)
{	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

			*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))
	{ *ppvObj = this; 
	}
	else if(IsEqualIID(riid, &IID_IEnumFORMATETC))
	{ *ppvObj = (IDataObject*)this;
	}   

	if(*ppvObj)
	{ (*(LPENUMFORMATETC*)ppvObj)->lpvtbl->fnAddRef(this);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;

}
static ULONG WINAPI IEnumFORMATETC_AddRef (LPENUMFORMATETC this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
	return ++(this->ref);
}
static ULONG WINAPI IEnumFORMATETC_Release (LPENUMFORMATETC this)
{	TRACE(shell,"(%p)->()\n",this);
	if (!--(this->ref)) 
	{ TRACE(shell," destroying IEnumFORMATETC(%p)\n",this);
	  if (this->pFmt)
	  { SHFree (this->pFmt);
	  }
	  HeapFree(GetProcessHeap(),0,this);
	  return 0;
	}
	return this->ref;
}
static HRESULT WINAPI IEnumFORMATETC_Next(LPENUMFORMATETC this, ULONG celt, FORMATETC32 *rgelt, ULONG *pceltFethed)
{	UINT32 cfetch;
	HRESULT hres = S_FALSE;

	TRACE (shell, "(%p)->()\n", this);

        if (this->posFmt < this->countFmt)
        { cfetch = this->countFmt - this->posFmt;
	  if (cfetch >= celt)
	  { cfetch = celt;
	    hres = S_OK;
	  }
	  memcpy(rgelt, &this->pFmt[this->posFmt], cfetch * sizeof(FORMATETC32));
	  this->posFmt += cfetch;
	}
	else
	{ cfetch = 0;
        }

        if (pceltFethed)
        { *pceltFethed = cfetch;
        }

        return hres;
}
static HRESULT WINAPI IEnumFORMATETC_Skip(LPENUMFORMATETC this, ULONG celt)
{	FIXME (shell, "(%p)->(num=%lu)\n", this, celt);

	this->posFmt += celt;
	if (this->posFmt > this->countFmt)
        { this->posFmt = this->countFmt;
	  return S_FALSE;
	}
	return S_OK;
}
static HRESULT WINAPI IEnumFORMATETC_Reset(LPENUMFORMATETC this)
{	FIXME (shell, "(%p)->()\n", this);

        this->posFmt = 0;
        return S_OK;
}
static HRESULT WINAPI IEnumFORMATETC_Clone(LPENUMFORMATETC this, LPENUMFORMATETC* ppenum)
{	FIXME (shell, "(%p)->(ppenum=%p)\n", this, ppenum);
	return E_NOTIMPL;
}

/***********************************************************************
*   IDataObject implementation
*/

static HRESULT WINAPI IDataObject_QueryInterface (LPDATAOBJECT, REFIID riid, LPVOID * ppvObj);
static ULONG WINAPI IDataObject_AddRef (LPDATAOBJECT);
static ULONG WINAPI IDataObject_Release (LPDATAOBJECT);
static HRESULT WINAPI IDataObject_GetData (LPDATAOBJECT, LPFORMATETC32 pformatetcIn, STGMEDIUM32 *pmedium);
static HRESULT WINAPI IDataObject_GetDataHere(LPDATAOBJECT, LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium);
static HRESULT WINAPI IDataObject_QueryGetData(LPDATAOBJECT, LPFORMATETC32 pformatetc);
static HRESULT WINAPI IDataObject_GetCanonicalFormatEtc(LPDATAOBJECT, LPFORMATETC32 pformatectIn, LPFORMATETC32 pformatetcOut);
static HRESULT WINAPI IDataObject_SetData(LPDATAOBJECT, LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium, BOOL32 fRelease);
static HRESULT WINAPI IDataObject_EnumFormatEtc(LPDATAOBJECT, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc);
static HRESULT WINAPI IDataObject_DAdvise (LPDATAOBJECT, LPFORMATETC32 *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection);
static HRESULT WINAPI IDataObject_DUnadvise(LPDATAOBJECT, DWORD dwConnection);
static HRESULT WINAPI IDataObject_EnumDAdvise(LPDATAOBJECT, IEnumSTATDATA **ppenumAdvise);

static struct IDataObject_VTable dtovt = 
{	IDataObject_QueryInterface,
	IDataObject_AddRef,
	IDataObject_Release,
	IDataObject_GetData,
	IDataObject_GetDataHere,
	IDataObject_QueryGetData,
	IDataObject_GetCanonicalFormatEtc,
	IDataObject_SetData,
	IDataObject_EnumFormatEtc,
	IDataObject_DAdvise,
	IDataObject_DUnadvise,
	IDataObject_EnumDAdvise
};

/**************************************************************************
*  IDataObject_Constructor
*/
LPDATAOBJECT IDataObject_Constructor(HWND32 hwndOwner, LPSHELLFOLDER pcf, LPITEMIDLIST * apit, UINT32 cpit)
{	LPDATAOBJECT dto;
	dto=(LPDATAOBJECT)HeapAlloc(GetProcessHeap(),0,sizeof(IDataObject));
	dto->ref=1;
	dto->lpvtbl=&dtovt;
	TRACE(shell,"(%p)->()\n",dto);
	return dto;
}
/***************************************************************************
*  IDataObject_QueryInterface
*/
static HRESULT WINAPI IDataObject_QueryInterface (LPDATAOBJECT this, REFIID riid, LPVOID * ppvObj)
{	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE(shell,"(%p)->(\n\tIID:\t%s,%p)\n",this,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = this; 
	}
	else if(IsEqualIID(riid, &IID_IDataObject))  /*IDataObject*/
	{ *ppvObj = (IDataObject*)this;
	}   

	if(*ppvObj)
	{ (*(LPDATAOBJECT*)ppvObj)->lpvtbl->fnAddRef(this);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}   
/**************************************************************************
*  IDataObject_AddRef
*/
static ULONG WINAPI IDataObject_AddRef(LPDATAOBJECT this)
{	TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
	return ++(this->ref);
}
/**************************************************************************
*  IDataObject_Release
*/
static ULONG WINAPI IDataObject_Release(LPDATAOBJECT this)
{	TRACE(shell,"(%p)->()\n",this);
	if (!--(this->ref)) 
	{ TRACE(shell," destroying IDataObject(%p)\n",this);
	  HeapFree(GetProcessHeap(),0,this);
	  return 0;
	}
	return this->ref;
}
static HRESULT WINAPI IDataObject_GetData (LPDATAOBJECT this, LPFORMATETC32 pformatetcIn, STGMEDIUM32 *pmedium)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_GetDataHere(LPDATAOBJECT this, LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_QueryGetData(LPDATAOBJECT this, LPFORMATETC32 pformatetc)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_GetCanonicalFormatEtc(LPDATAOBJECT this, LPFORMATETC32 pformatectIn, LPFORMATETC32 pformatetcOut)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_SetData(LPDATAOBJECT this, LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium, BOOL32 fRelease)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_EnumFormatEtc(LPDATAOBJECT this, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_DAdvise (LPDATAOBJECT this, LPFORMATETC32 *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_DUnadvise(LPDATAOBJECT this, DWORD dwConnection)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_EnumDAdvise(LPDATAOBJECT this, IEnumSTATDATA **ppenumAdvise)
{	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
