/*
 *	IEnumFORMATETC, IDataObject
 *
 * selecting and droping objects within the shell and/or common dialogs
 *
 *	Copyright 1998	<juergen.schmied@metronet.de>
 */
#include "debug.h"
#include "wintypes.h"
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
#include "wine/obj_dataobject.h"
#include "objbase.h"
#include "pidl.h"
#include "winerror.h"
#include "shell32_main.h"

UINT32 cfShellIDList=0;
UINT32 cfFileGroupDesc=0;
UINT32 cfFileContents=0;

/***********************************************************************
*   IEnumFORMATETC implementation
*/
typedef struct _IEnumFORMATETC
{
    /* IUnknown fields */
    ICOM_VTABLE(IEnumFORMATETC)* lpvtbl;
    DWORD                        ref;
    /* IEnumFORMATETC fields */
    UINT32        posFmt;
    UINT32        countFmt;
    LPFORMATETC32 pFmt;
} _IEnumFORMATETC;

static HRESULT WINAPI IEnumFORMATETC_fnQueryInterface(LPUNKNOWN iface, REFIID riid, LPVOID* ppvObj);
static ULONG WINAPI IEnumFORMATETC_fnAddRef(LPUNKNOWN iface);
static ULONG WINAPI IEnumFORMATETC_fnRelease(LPUNKNOWN iface);
static HRESULT WINAPI IEnumFORMATETC_fnNext(LPENUMFORMATETC iface, ULONG celt, FORMATETC32* rgelt, ULONG* pceltFethed);
static HRESULT WINAPI IEnumFORMATETC_fnSkip(LPENUMFORMATETC iface, ULONG celt);
static HRESULT WINAPI IEnumFORMATETC_fnReset(LPENUMFORMATETC iface);
static HRESULT WINAPI IEnumFORMATETC_fnClone(LPENUMFORMATETC iface, LPENUMFORMATETC* ppenum);

static struct ICOM_VTABLE(IEnumFORMATETC) efvt = 
{
    {
        IEnumFORMATETC_fnQueryInterface,
        IEnumFORMATETC_fnAddRef,
        IEnumFORMATETC_fnRelease
    },
    IEnumFORMATETC_fnNext,
    IEnumFORMATETC_fnSkip,
    IEnumFORMATETC_fnReset,
    IEnumFORMATETC_fnClone
};

LPENUMFORMATETC IEnumFORMATETC_Constructor(UINT32 cfmt, const FORMATETC32 afmt[])
{
	_IEnumFORMATETC* ef;
	DWORD size=cfmt * sizeof(FORMATETC32);
	
	ef=(_IEnumFORMATETC*)HeapAlloc(GetProcessHeap(),0,sizeof(_IEnumFORMATETC));
	ef->ref=1;
	ef->lpvtbl=&efvt;

	ef->posFmt = 0;
	ef->countFmt = cfmt;
	ef->pFmt = SHAlloc (size);

	if (ef->pFmt)
	{ memcpy(ef->pFmt, afmt, size);
	}

	TRACE(shell,"(%p)->()\n",ef);
	shell32_ObjCount++;
	return (LPENUMFORMATETC)ef;
}
static HRESULT WINAPI IEnumFORMATETC_fnQueryInterface(LPUNKNOWN iface, REFIID riid, LPVOID* ppvObj)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	char    xriid[50];
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
	{ IEnumFORMATETC_AddRef((IEnumFORMATETC*)*ppvObj);
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;

}
static ULONG WINAPI IEnumFORMATETC_fnAddRef(LPUNKNOWN iface)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);
	shell32_ObjCount++;
	return ++(this->ref);
}
static ULONG WINAPI IEnumFORMATETC_fnRelease(LPUNKNOWN iface)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	TRACE(shell,"(%p)->()\n",this);

	shell32_ObjCount--;

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
static HRESULT WINAPI IEnumFORMATETC_fnNext(LPENUMFORMATETC iface, ULONG celt, FORMATETC32 *rgelt, ULONG *pceltFethed)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	UINT32 cfetch;
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
static HRESULT WINAPI IEnumFORMATETC_fnSkip(LPENUMFORMATETC iface, ULONG celt)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	FIXME (shell, "(%p)->(num=%lu)\n", this, celt);

	this->posFmt += celt;
	if (this->posFmt > this->countFmt)
        { this->posFmt = this->countFmt;
	  return S_FALSE;
	}
	return S_OK;
}
static HRESULT WINAPI IEnumFORMATETC_fnReset(LPENUMFORMATETC iface)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	FIXME (shell, "(%p)->()\n", this);

        this->posFmt = 0;
        return S_OK;
}
static HRESULT WINAPI IEnumFORMATETC_fnClone(LPENUMFORMATETC iface, LPENUMFORMATETC* ppenum)
{
	ICOM_THIS(IEnumFORMATETC,iface);
	FIXME (shell, "(%p)->(ppenum=%p)\n", this, ppenum);
	return E_NOTIMPL;
}

/***********************************************************************
*   IDataObject implementation
*/
typedef struct _IDataObject
{
    /* IUnknown fields */
    ICOM_VTABLE(IDataObject)* lpvtbl;
    DWORD                     ref;
    /* IDataObject fields */
    LPSHELLFOLDER psf;
    LPIDLLIST     lpill;       /* the data of the dataobject */
    LPITEMIDLIST  pidl;     
} _IDataObject;

static HRESULT WINAPI IDataObject_fnQueryInterface(LPUNKNOWN iface, REFIID riid, LPVOID* ppvObj);
static ULONG WINAPI IDataObject_fnAddRef(LPUNKNOWN iface);
static ULONG WINAPI IDataObject_fnRelease(LPUNKNOWN iface);
static HRESULT WINAPI IDataObject_fnGetData(LPDATAOBJECT iface, LPFORMATETC32 pformatetcIn, STGMEDIUM32* pmedium);
static HRESULT WINAPI IDataObject_fnGetDataHere(LPDATAOBJECT iface, LPFORMATETC32 pformatetc, STGMEDIUM32* pmedium);
static HRESULT WINAPI IDataObject_fnQueryGetData(LPDATAOBJECT iface, LPFORMATETC32 pformatetc);
static HRESULT WINAPI IDataObject_fnGetCanonicalFormatEtc(LPDATAOBJECT iface, LPFORMATETC32 pformatectIn, LPFORMATETC32 pformatetcOut);
static HRESULT WINAPI IDataObject_fnSetData(LPDATAOBJECT iface, LPFORMATETC32 pformatetc, STGMEDIUM32* pmedium, BOOL32 fRelease);
static HRESULT WINAPI IDataObject_fnEnumFormatEtc(LPDATAOBJECT iface, DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc);
static HRESULT WINAPI IDataObject_fnDAdvise(LPDATAOBJECT iface, LPFORMATETC32* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
static HRESULT WINAPI IDataObject_fnDUnadvise(LPDATAOBJECT iface, DWORD dwConnection);
static HRESULT WINAPI IDataObject_fnEnumDAdvise(LPDATAOBJECT iface, IEnumSTATDATA **ppenumAdvise);

static struct ICOM_VTABLE(IDataObject) dtovt = 
{
    {
        IDataObject_fnQueryInterface,
        IDataObject_fnAddRef,
        IDataObject_fnRelease
    },
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

/**************************************************************************
*  IDataObject_Constructor
*/
LPDATAOBJECT IDataObject_Constructor(HWND32 hwndOwner, LPSHELLFOLDER psf, LPITEMIDLIST * apidl, UINT32 cidl)
{
	_IDataObject* dto;
	if (!(dto = (_IDataObject*)HeapAlloc(GetProcessHeap(),0,sizeof(_IDataObject))))
	  return NULL;
	  
	dto->ref=1;
	dto->lpvtbl=&dtovt;
	dto->psf=psf;
	dto->pidl=ILClone(psf->mpidl); /* FIXME:add a reference and don't copy*/

	/* fill the ItemID List List */
	dto->lpill = IDLList_Constructor (8);
	if (! dto->lpill )
	  return NULL;
	  
	dto->lpill->lpvtbl->fnAddItems(dto->lpill, apidl, cidl); 
	
	TRACE(shell,"(%p)->(sf=%p apidl=%p cidl=%u)\n",dto, psf, apidl, cidl);
	shell32_ObjCount++;
	return (LPDATAOBJECT)dto;
}
/***************************************************************************
*  IDataObject_QueryInterface
*/
static HRESULT WINAPI IDataObject_fnQueryInterface (LPUNKNOWN iface, REFIID riid, LPVOID * ppvObj)
{
	ICOM_THIS(IDataObject,iface);
	char    xriid[50];
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
	{ IDataObject_AddRef((IDataObject*)*ppvObj);      
	  TRACE(shell,"-- Interface: (%p)->(%p)\n",ppvObj,*ppvObj);
	  return S_OK;
	}
	TRACE(shell,"-- Interface: E_NOINTERFACE\n");
	return E_NOINTERFACE;
}   
/**************************************************************************
*  IDataObject_AddRef
*/
static ULONG WINAPI IDataObject_fnAddRef(LPUNKNOWN iface)
{
	ICOM_THIS(IDataObject,iface);

	TRACE(shell,"(%p)->(count=%lu)\n",this,(this->ref)+1);

	shell32_ObjCount++;
	return ++(this->ref);
}
/**************************************************************************
*  IDataObject_Release
*/
static ULONG WINAPI IDataObject_fnRelease(LPUNKNOWN iface)
{
	ICOM_THIS(IDataObject,iface);
	TRACE(shell,"(%p)->()\n",this);

	shell32_ObjCount--;

	if (!--(this->ref)) 
	{ TRACE(shell," destroying IDataObject(%p)\n",this);
	  IDLList_Destructor(this->lpill);
	  HeapFree(GetProcessHeap(),0,this);
	  return 0;
	}
	return this->ref;
}
/**************************************************************************
* DATAOBJECT_InitShellIDList (internal)
*
* NOTES
*  get or register the "Shell IDList Array" clipformat
*/
static BOOL32 DATAOBJECT_InitShellIDList(void)
{	if (cfShellIDList)
        { return(TRUE);
        }

        cfShellIDList = RegisterClipboardFormat32A(CFSTR_SHELLIDLIST);
        return(cfShellIDList != 0);
}

/**************************************************************************
* DATAOBJECT_InitFileGroupDesc (internal)
*
* NOTES
*  get or register the "FileGroupDescriptor" clipformat
*/
/* FIXME: DATAOBJECT_InitFileGroupDesc is not used (19981226)
static BOOL32 DATAOBJECT_InitFileGroupDesc(void)
{	if (cfFileGroupDesc)
        { return(TRUE);
        }

        cfFileGroupDesc = RegisterClipboardFormat32A(CFSTR_FILEDESCRIPTORA);
        return(cfFileGroupDesc != 0);
}
*/
/**************************************************************************
* DATAOBJECT_InitFileContents (internal)
* 
* NOTES
 * get or register the "FileContents" clipformat
*/
/* FIXME: DATAOBJECT_InitFileContents is not used (19981226)
static BOOL32 DATAOBJECT_InitFileContents(void)
{	if (cfFileContents)
        { return(TRUE);
        }

        cfFileContents = RegisterClipboardFormat32A(CFSTR_FILECONTENTS);
        return(cfFileContents != 0);
}
*/

/**************************************************************************
* interface implementation
*/
static HRESULT WINAPI IDataObject_fnGetData(LPDATAOBJECT iface, LPFORMATETC32 pformatetcIn, STGMEDIUM32 *pmedium)
{
	ICOM_THIS(IDataObject,iface);
	char	temp[256];
	UINT32	cItems;
	DWORD	size, size1, size2;
	LPITEMIDLIST pidl;
	LPCIDA pcida;
	HGLOBAL32 hmem;
	
	GetClipboardFormatName32A (pformatetcIn->cfFormat, temp, 256);
	WARN (shell, "(%p)->(%p %p format=%s)semi-stub\n", this, pformatetcIn, pmedium, temp);

	if (!DATAOBJECT_InitShellIDList())	/* is the clipformat registred? */
        { return(E_UNEXPECTED);
        }
	
        if (pformatetcIn->cfFormat == cfShellIDList)
        { if (pformatetcIn->ptd==NULL 
		&& (pformatetcIn->dwAspect & DVASPECT_CONTENT) 
		&& pformatetcIn->lindex==-1
		&& (pformatetcIn->tymed&TYMED_HGLOBAL))
	  { cItems = this->lpill->lpvtbl->fnGetCount(this->lpill);
	    if (cItems < 1)
	    { return(E_UNEXPECTED);
	    }
	    pidl = this->lpill->lpvtbl->fnGetElement(this->lpill, 0);

	    pdump(this->pidl);
	    pdump(pidl);
	    
	    /*hack consider only the first item*/
	    cItems = 2;
	    size = sizeof(CIDA) + sizeof (UINT32)*(cItems-1);
	    size1 = ILGetSize (this->pidl);
	    size2 = ILGetSize (pidl);
	    hmem = GlobalAlloc32(GMEM_FIXED, size+size1+size2);
	    pcida = GlobalLock32 (hmem);
	    if (!pcida)
	    { return(E_OUTOFMEMORY);
	    }

	    pcida->cidl = 1;
	    pcida->aoffset[0] = size;
	    pcida->aoffset[1] = size+size1;

	    TRACE(shell,"-- %lu %lu %lu\n",size, size1, size2 );
	    TRACE(shell,"-- %p %p\n",this->pidl, pidl);
	    TRACE(shell,"-- %p %p %p\n",pcida, (LPBYTE)pcida+size,(LPBYTE)pcida+size+size1);
	    
	    memcpy ((LPBYTE)pcida+size, this->pidl, size1);
	    memcpy ((LPBYTE)pcida+size+size1, pidl, size2);
	    TRACE(shell,"-- after copy\n");

	    GlobalUnlock32(hmem);
	    
	    pmedium->tymed = TYMED_HGLOBAL;
	    pmedium->u.hGlobal = (HGLOBAL32)pcida;
	    pmedium->pUnkForRelease = NULL;
	    TRACE(shell,"-- ready\n");
	    return(NOERROR);
	  }
	}
	FIXME (shell, "-- clipformat not implemented\n");
	return (E_INVALIDARG);
}
static HRESULT WINAPI IDataObject_fnGetDataHere(LPDATAOBJECT iface, LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnQueryGetData(LPDATAOBJECT iface, LPFORMATETC32 pformatetc)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnGetCanonicalFormatEtc(LPDATAOBJECT iface, LPFORMATETC32 pformatectIn, LPFORMATETC32 pformatetcOut)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnSetData(LPDATAOBJECT iface, LPFORMATETC32 pformatetc, STGMEDIUM32 *pmedium, BOOL32 fRelease)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnEnumFormatEtc(LPDATAOBJECT iface, DWORD dwDirection, IEnumFORMATETC **ppenumFormatEtc)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnDAdvise(LPDATAOBJECT iface, LPFORMATETC32 *pformatetc, DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnDUnadvise(LPDATAOBJECT iface, DWORD dwConnection)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
static HRESULT WINAPI IDataObject_fnEnumDAdvise(LPDATAOBJECT iface, IEnumSTATDATA **ppenumAdvise)
{
	ICOM_THIS(IDataObject,iface);
	FIXME (shell, "(%p)->()\n", this);
	return E_NOTIMPL;
}
