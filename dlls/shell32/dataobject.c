/*
 *	IEnumFORMATETC, IDataObject
 *
 * selecting and droping objects within the shell and/or common dialogs
 *
 *	Copyright 1998	<juergen.schmied@metronet.de>
 */
#include <string.h>

#include "oleidl.h"
#include "pidl.h"
#include "winerror.h"
#include "shell32_main.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(shell)

UINT cfShellIDList=0;
UINT cfFileGroupDesc=0;
UINT cfFileContents=0;

/***********************************************************************
*   IEnumFORMATETC implementation
*/
typedef struct 
{
    /* IUnknown fields */
    ICOM_VTABLE(IEnumFORMATETC)* lpvtbl;
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
	
	ef=(IEnumFORMATETCImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IEnumFORMATETCImpl));
	ef->ref=1;
	ef->lpvtbl=&efvt;

	ef->posFmt = 0;
	ef->countFmt = cfmt;
	ef->pFmt = SHAlloc (size);

	if (ef->pFmt)
	{ memcpy(ef->pFmt, afmt, size);
	}

	TRACE("(%p)->()\n",ef);
	shell32_ObjCount++;
	return (LPENUMFORMATETC)ef;
}
static HRESULT WINAPI IEnumFORMATETC_fnQueryInterface(LPENUMFORMATETC iface, REFIID riid, LPVOID* ppvObj)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

			*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IEnumFORMATETC))
	{ *ppvObj = (IDataObject*)This;
	}   

	if(*ppvObj)
	{ IEnumFORMATETC_AddRef((IEnumFORMATETC*)*ppvObj);
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
	{ TRACE(" destroying IEnumFORMATETC(%p)\n",This);
	  if (This->pFmt)
	  { SHFree (This->pFmt);
	  }
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
static HRESULT WINAPI IEnumFORMATETC_fnNext(LPENUMFORMATETC iface, ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	UINT cfetch;
	HRESULT hres = S_FALSE;

	TRACE("(%p)->()\n", This);

        if (This->posFmt < This->countFmt)
        { cfetch = This->countFmt - This->posFmt;
	  if (cfetch >= celt)
	  { cfetch = celt;
	    hres = S_OK;
	  }
	  memcpy(rgelt, &This->pFmt[This->posFmt], cfetch * sizeof(FORMATETC));
	  This->posFmt += cfetch;
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
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	FIXME("(%p)->(num=%lu)\n", This, celt);

	This->posFmt += celt;
	if (This->posFmt > This->countFmt)
        { This->posFmt = This->countFmt;
	  return S_FALSE;
	}
	return S_OK;
}
static HRESULT WINAPI IEnumFORMATETC_fnReset(LPENUMFORMATETC iface)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	FIXME("(%p)->()\n", This);

        This->posFmt = 0;
        return S_OK;
}
static HRESULT WINAPI IEnumFORMATETC_fnClone(LPENUMFORMATETC iface, LPENUMFORMATETC* ppenum)
{
	ICOM_THIS(IEnumFORMATETCImpl,iface);
	FIXME("(%p)->(ppenum=%p)\n", This, ppenum);
	return E_NOTIMPL;
}

/**************************************************************************
 *  IDLList "Item ID List List"
 *
 *  NOTES
 *   interal data holder for IDataObject
 */
#define STDMETHOD(xfn) HRESULT (CALLBACK *fn##xfn)
#define STDMETHOD_(type,xfn) type (CALLBACK *fn##xfn)
#define THIS_ THIS,

typedef struct tagLPIDLLIST *LPIDLLIST, IDLList;

#define THIS LPIDLLIST me
typedef enum
{ State_UnInit=1,
  State_Init=2,
  State_OutOfMem=3
} IDLListState;

typedef struct IDLList_VTable
{ STDMETHOD_(UINT, GetState)(THIS);
  STDMETHOD_(LPITEMIDLIST, GetElement)(THIS_ UINT nIndex);
  STDMETHOD_(UINT, GetCount)(THIS);
  STDMETHOD_(BOOL, StoreItem)(THIS_ LPITEMIDLIST pidl);
  STDMETHOD_(BOOL, AddItems)(THIS_ LPITEMIDLIST *apidl, UINT cidl);
  STDMETHOD_(BOOL, InitList)(THIS);
  STDMETHOD_(void, CleanList)(THIS);
} IDLList_VTable,*LPIDLLIST_VTABLE;

struct tagLPIDLLIST
{ LPIDLLIST_VTABLE  lpvtbl;
  HDPA  dpa;
  UINT  uStep;
};

extern LPIDLLIST IDLList_Constructor (UINT uStep);
extern void IDLList_Destructor(LPIDLLIST me);
#undef THIS



/**************************************************************************
 *  IDLList "Item ID List List"
 * 
 */
static UINT WINAPI IDLList_GetState(LPIDLLIST this);
static LPITEMIDLIST WINAPI IDLList_GetElement(LPIDLLIST this, UINT nIndex);
static UINT WINAPI IDLList_GetCount(LPIDLLIST this);
static BOOL WINAPI IDLList_StoreItem(LPIDLLIST this, LPITEMIDLIST pidl);
static BOOL WINAPI IDLList_AddItems(LPIDLLIST this, LPITEMIDLIST *apidl, UINT cidl);
static BOOL WINAPI IDLList_InitList(LPIDLLIST this);
static void WINAPI IDLList_CleanList(LPIDLLIST this);

static IDLList_VTable idllvt = 
{	IDLList_GetState,
	IDLList_GetElement,
	IDLList_GetCount,
	IDLList_StoreItem,
	IDLList_AddItems,
	IDLList_InitList,
	IDLList_CleanList
};

LPIDLLIST IDLList_Constructor (UINT uStep)
{	LPIDLLIST lpidll;
	if (!(lpidll = (LPIDLLIST)HeapAlloc(GetProcessHeap(),0,sizeof(IDLList))))
	  return NULL;

	lpidll->lpvtbl=&idllvt;
	lpidll->uStep=uStep;
	lpidll->dpa=NULL;

	TRACE("(%p)\n",lpidll);
	return lpidll;
}
void IDLList_Destructor(LPIDLLIST this)
{	TRACE("(%p)\n",this);
	IDLList_CleanList(this);
}
 
static UINT WINAPI IDLList_GetState(LPIDLLIST this)
{	TRACE("(%p)->(uStep=%u dpa=%p)\n",this, this->uStep, this->dpa);

	if (this->uStep == 0)
	{ if (this->dpa)
	    return(State_Init);
          return(State_OutOfMem);
        }
        return(State_UnInit);
}
static LPITEMIDLIST WINAPI IDLList_GetElement(LPIDLLIST this, UINT nIndex)
{	TRACE("(%p)->(index=%u)\n",this, nIndex);
	return((LPITEMIDLIST)pDPA_GetPtr(this->dpa, nIndex));
}
static UINT WINAPI IDLList_GetCount(LPIDLLIST this)
{	TRACE("(%p)\n",this);
	return(IDLList_GetState(this)==State_Init ? DPA_GetPtrCount(this->dpa) : 0);
}
static BOOL WINAPI IDLList_StoreItem(LPIDLLIST this, LPITEMIDLIST pidl)
{	TRACE("(%p)->(pidl=%p)\n",this, pidl);
	if (pidl)
        { if (IDLList_InitList(this) && pDPA_InsertPtr(this->dpa, 0x7fff, (LPSTR)pidl)>=0)
	    return(TRUE);
	  ILFree(pidl);
        }
        IDLList_CleanList(this);
        return(FALSE);
}
static BOOL WINAPI IDLList_AddItems(LPIDLLIST this, LPITEMIDLIST *apidl, UINT cidl)
{	INT i;
	TRACE("(%p)->(apidl=%p cidl=%u)\n",this, apidl, cidl);

	for (i=0; i<cidl; ++i)
        { if (!IDLList_StoreItem(this, ILClone((LPCITEMIDLIST)apidl[i])))
	    return(FALSE);
        }
        return(TRUE);
}
static BOOL WINAPI IDLList_InitList(LPIDLLIST this)
{	TRACE("(%p)\n",this);
	switch (IDLList_GetState(this))
        { case State_Init:
	    return(TRUE);

	  case State_OutOfMem:
	    return(FALSE);

	  case State_UnInit:
	  default:
	    this->dpa = pDPA_Create(this->uStep);
	    this->uStep = 0;
	    return(IDLList_InitList(this));
        }
}
static void WINAPI IDLList_CleanList(LPIDLLIST this)
{	INT i;
	TRACE("(%p)\n",this);

	if (this->uStep != 0)
        { this->dpa = NULL;
	  this->uStep = 0;
	  return;
        }

        if (!this->dpa)
        { return;
        }

        for (i=DPA_GetPtrCount(this->dpa)-1; i>=0; --i)
        { ILFree(IDLList_GetElement(this,i));
        }

        pDPA_Destroy(this->dpa);
        this->dpa=NULL;
}        


/***********************************************************************
*   IDataObject implementation
*/

typedef struct
{
    /* IUnknown fields */
    ICOM_VTABLE(IDataObject)* lpvtbl;
    DWORD                     ref;
    /* IDataObject fields */
    LPSHELLFOLDER psf;
    LPIDLLIST     lpill;       /* the data of the dataobject */
    LPITEMIDLIST  pidl;     
} IDataObjectImpl;

static HRESULT WINAPI IDataObject_fnQueryInterface(LPDATAOBJECT iface, REFIID riid, LPVOID* ppvObj);
static ULONG WINAPI IDataObject_fnAddRef(LPDATAOBJECT iface);
static ULONG WINAPI IDataObject_fnRelease(LPDATAOBJECT iface);
static HRESULT WINAPI IDataObject_fnGetData(LPDATAOBJECT iface, LPFORMATETC pformatetcIn, STGMEDIUM* pmedium);
static HRESULT WINAPI IDataObject_fnGetDataHere(LPDATAOBJECT iface, LPFORMATETC pformatetc, STGMEDIUM* pmedium);
static HRESULT WINAPI IDataObject_fnQueryGetData(LPDATAOBJECT iface, LPFORMATETC pformatetc);
static HRESULT WINAPI IDataObject_fnGetCanonicalFormatEtc(LPDATAOBJECT iface, LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut);
static HRESULT WINAPI IDataObject_fnSetData(LPDATAOBJECT iface, LPFORMATETC pformatetc, STGMEDIUM* pmedium, BOOL fRelease);
static HRESULT WINAPI IDataObject_fnEnumFormatEtc(LPDATAOBJECT iface, DWORD dwDirection, IEnumFORMATETC** ppenumFormatEtc);
static HRESULT WINAPI IDataObject_fnDAdvise(LPDATAOBJECT iface, FORMATETC* pformatetc, DWORD advf, IAdviseSink* pAdvSink, DWORD* pdwConnection);
static HRESULT WINAPI IDataObject_fnDUnadvise(LPDATAOBJECT iface, DWORD dwConnection);
static HRESULT WINAPI IDataObject_fnEnumDAdvise(LPDATAOBJECT iface, IEnumSTATDATA **ppenumAdvise);

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

/**************************************************************************
*  IDataObject_Constructor
*/
LPDATAOBJECT IDataObject_Constructor(HWND hwndOwner, LPSHELLFOLDER psf, LPITEMIDLIST * apidl, UINT cidl)
{
	IDataObjectImpl* dto;
	if (!(dto = (IDataObjectImpl*)HeapAlloc(GetProcessHeap(),0,sizeof(IDataObjectImpl))))
	  return NULL;
	  
	dto->ref=1;
	dto->lpvtbl=&dtovt;
	dto->psf=psf;
	dto->pidl=ILClone(((IGenericSFImpl*)psf)->pMyPidl); /* FIXME:add a reference and don't copy*/

	/* fill the ItemID List List */
	dto->lpill = IDLList_Constructor (8);
	if (! dto->lpill )
	  return NULL;
	  
	dto->lpill->lpvtbl->fnAddItems(dto->lpill, apidl, cidl); 
	
	TRACE("(%p)->(sf=%p apidl=%p cidl=%u)\n",dto, psf, apidl, cidl);
	shell32_ObjCount++;
	return (LPDATAOBJECT)dto;
}
/***************************************************************************
*  IDataObject_QueryInterface
*/
static HRESULT WINAPI IDataObject_fnQueryInterface(LPDATAOBJECT iface, REFIID riid, LPVOID * ppvObj)
{
	ICOM_THIS(IDataObjectImpl,iface);
	char    xriid[50];
	WINE_StringFromCLSID((LPCLSID)riid,xriid);
	TRACE("(%p)->(\n\tIID:\t%s,%p)\n",This,xriid,ppvObj);

	*ppvObj = NULL;

	if(IsEqualIID(riid, &IID_IUnknown))          /*IUnknown*/
	{ *ppvObj = This; 
	}
	else if(IsEqualIID(riid, &IID_IDataObject))  /*IDataObject*/
	{ *ppvObj = (IDataObject*)This;
	}   

	if(*ppvObj)
	{ IDataObject_AddRef((IDataObject*)*ppvObj);      
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
	{ TRACE(" destroying IDataObject(%p)\n",This);
	  IDLList_Destructor(This->lpill);
	  HeapFree(GetProcessHeap(),0,This);
	  return 0;
	}
	return This->ref;
}
/**************************************************************************
* DATAOBJECT_InitShellIDList (internal)
*
* NOTES
*  get or register the "Shell IDList Array" clipformat
*/
static BOOL DATAOBJECT_InitShellIDList(void)
{	if (cfShellIDList)
        { return(TRUE);
        }

        cfShellIDList = RegisterClipboardFormatA(CFSTR_SHELLIDLIST);
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

        cfFileGroupDesc = RegisterClipboardFormatA(CFSTR_FILEDESCRIPTORA);
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

        cfFileContents = RegisterClipboardFormatA(CFSTR_FILECONTENTS);
        return(cfFileContents != 0);
}
*/

/**************************************************************************
* interface implementation
*/
static HRESULT WINAPI IDataObject_fnGetData(LPDATAOBJECT iface, LPFORMATETC pformatetcIn, STGMEDIUM *pmedium)
{
	ICOM_THIS(IDataObjectImpl,iface);
	char	temp[256];
	UINT	cItems;
	DWORD	size, size1, size2;
	LPITEMIDLIST pidl;
	LPCIDA pcida;
	HGLOBAL hmem;
	
	GetClipboardFormatNameA (pformatetcIn->cfFormat, temp, 256);
	WARN("(%p)->(%p %p format=%s)semi-stub\n", This, pformatetcIn, pmedium, temp);

	if (!DATAOBJECT_InitShellIDList())	/* is the clipformat registred? */
        { return(E_UNEXPECTED);
        }
	
        if (pformatetcIn->cfFormat == cfShellIDList)
        { if (pformatetcIn->ptd==NULL 
		&& (pformatetcIn->dwAspect & DVASPECT_CONTENT) 
		&& pformatetcIn->lindex==-1
		&& (pformatetcIn->tymed&TYMED_HGLOBAL))
	  { cItems = This->lpill->lpvtbl->fnGetCount(This->lpill);
	    if (cItems < 1)
	    { return(E_UNEXPECTED);
	    }
	    pidl = This->lpill->lpvtbl->fnGetElement(This->lpill, 0);

	    pdump(This->pidl);
	    pdump(pidl);
	    
	    /*hack consider only the first item*/
	    cItems = 2;
	    size = sizeof(CIDA) + sizeof (UINT)*(cItems-1);
	    size1 = ILGetSize (This->pidl);
	    size2 = ILGetSize (pidl);
	    hmem = GlobalAlloc(GMEM_FIXED, size+size1+size2);
	    pcida = GlobalLock (hmem);
	    if (!pcida)
	    { return(E_OUTOFMEMORY);
	    }

	    pcida->cidl = 1;
	    pcida->aoffset[0] = size;
	    pcida->aoffset[1] = size+size1;

	    TRACE("-- %lu %lu %lu\n",size, size1, size2 );
	    TRACE("-- %p %p\n",This->pidl, pidl);
	    TRACE("-- %p %p %p\n",pcida, (LPBYTE)pcida+size,(LPBYTE)pcida+size+size1);
	    
	    memcpy ((LPBYTE)pcida+size, This->pidl, size1);
	    memcpy ((LPBYTE)pcida+size+size1, pidl, size2);
	    TRACE("-- after copy\n");

	    GlobalUnlock(hmem);
	    
	    pmedium->tymed = TYMED_HGLOBAL;
	    pmedium->u.hGlobal = (HGLOBAL)pcida;
	    pmedium->pUnkForRelease = NULL;
	    TRACE("-- ready\n");
	    return(NOERROR);
	  }
	}
	FIXME("-- clipformat not implemented\n");
	return (E_INVALIDARG);
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
	FIXME("(%p)->()\n", This);
	return E_NOTIMPL;
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
	FIXME("(%p)->()\n", This);
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
