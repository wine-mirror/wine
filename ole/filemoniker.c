/***************************************************************************************
 *	                      FileMonikers implementation
 *
 *               Copyright 1999  Noomen Hamza
 ***************************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
#include "debug.h"
#include "heap.h"

typedef struct FileMonikerImpl{

    ICOM_VTABLE(IMoniker)*  lpvtbl;

    ULONG ref;

} FileMonikerImpl;

static HRESULT WINAPI FileMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI FileMonikerImpl_AddRef(IMoniker* iface);
static ULONG   WINAPI FileMonikerImpl_Release(IMoniker* iface);
static HRESULT WINAPI FileMonikerImpl_GetClassID(const IMoniker* iface, CLSID *pClassID);
static HRESULT WINAPI FileMonikerImpl_IsDirty(IMoniker* iface);
static HRESULT WINAPI FileMonikerImpl_Load(IMoniker* iface, IStream* pStm);
static HRESULT WINAPI FileMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty);
static HRESULT WINAPI FileMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize);
static HRESULT WINAPI FileMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI FileMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI FileMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
static HRESULT WINAPI FileMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite);
static HRESULT WINAPI FileMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker);
static HRESULT WINAPI FileMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker);
static HRESULT WINAPI FileMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash);
static HRESULT WINAPI FileMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
static HRESULT WINAPI FileMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pFileTime);
static HRESULT WINAPI FileMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk);
static HRESULT WINAPI FileMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix);
static HRESULT WINAPI FileMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath);
static HRESULT WINAPI FileMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName);
static HRESULT WINAPI FileMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
static HRESULT WINAPI FileMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys);

static HRESULT WINAPI FileMonikerImpl_Construct(FileMonikerImpl* iface, LPCOLESTR lpszPathName);
static HRESULT WINAPI FileMonikerImpl_Destroy(FileMonikerImpl* iface);

// Virtual function table for the FileMonikerImpl class.
static ICOM_VTABLE(IMoniker) VT_FileMonikerImpl =
{
    FileMonikerImpl_QueryInterface,
    FileMonikerImpl_AddRef,
    FileMonikerImpl_Release,
    FileMonikerImpl_GetClassID,
    FileMonikerImpl_IsDirty,
    FileMonikerImpl_Load,
    FileMonikerImpl_Save,
    FileMonikerImpl_GetSizeMax,
    FileMonikerImpl_BindToObject,
    FileMonikerImpl_BindToStorage,
    FileMonikerImpl_Reduce,
    FileMonikerImpl_ComposeWith,
    FileMonikerImpl_Enum,
    FileMonikerImpl_IsEqual,
    FileMonikerImpl_Hash,
    FileMonikerImpl_IsRunning,
    FileMonikerImpl_GetTimeOfLastChange,
    FileMonikerImpl_Inverse,
    FileMonikerImpl_CommonPrefixWith,
    FileMonikerImpl_RelativePathTo,
    FileMonikerImpl_GetDisplayName,
    FileMonikerImpl_ParseDisplayName,
    FileMonikerImpl_IsSystemMoniker
};

/*******************************************************************************
 *        FileMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI FileMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
  FileMonikerImpl* This=(FileMonikerImpl*)iface;
  TRACE(ole,"(%p,%p,%p)\n",This,riid,ppvObject);

  // Perform a sanity check on the parameters.
  if ( (This==0) || (ppvObject==0) )    return E_INVALIDARG;
  
  // Initialize the return parameter.
  *ppvObject = 0;

  // Compare the riid with the interface IDs implemented by this object.
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
      *ppvObject = (IMoniker*)This;
  else
      if (memcmp(&IID_IPersist, riid, sizeof(IID_IPersist)) == 0)
          *ppvObject = (IMoniker*)This;
      else
          if (memcmp(&IID_IPersistStream, riid, sizeof(IID_IPersistStream)) == 0)
              *ppvObject = (IMoniker*)This;
          else
              if (memcmp(&IID_IMoniker, riid, sizeof(IID_IMoniker)) == 0)
                  *ppvObject = (IMoniker*)This;

  // Check that we obtained an interface.
  if ((*ppvObject)==0)        return E_NOINTERFACE;
  
   // Query Interface always increases the reference count by one when it is successful
  FileMonikerImpl_AddRef(iface);

  return S_OK;;
}

/******************************************************************************
 *        FileMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI FileMonikerImpl_AddRef(IMoniker* iface)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    TRACE(ole,"(%p)\n",This);

    return ++(This->ref);
}

/******************************************************************************
 *        FileMoniker_Release
 ******************************************************************************/
ULONG WINAPI FileMonikerImpl_Release(IMoniker* iface)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    TRACE(ole,"(%p)\n",This);

    This->ref--;

    if (This->ref==0){
        FileMonikerImpl_Destroy(This);
        return 0;
    }
    return This->ref;;
}

/******************************************************************************
 *        FileMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetClassID(const IMoniker* iface, CLSID *pClassID)//Pointer to CLSID of object
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pClassID);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsDirty(IMoniker* iface)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p),stub!\n",This);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Load
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Load(
          IMoniker* iface,
          IStream* pStm)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pStm);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Save
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Save(
          IMoniker* iface,
          IStream* pStm,
	  BOOL fClearDirty)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%d),stub!\n",This,pStm,fClearDirty);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetSizeMax(
          IMoniker* iface,
          ULARGE_INTEGER* pcbSize)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pcbSize);

    return E_NOTIMPL;
}

/******************************************************************************
 *         FileMoniker_Construct
 *******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Construct(FileMonikerImpl* This, LPCOLESTR lpszPathName){

    FIXME(ole,"(%p,%p),stub!\n",This,lpszPathName);

    memset(This, 0, sizeof(FileMonikerImpl));

    //Initialize the virtual fgunction table.
    This->lpvtbl       = &VT_FileMonikerImpl;
    return S_OK;
}

/******************************************************************************
 *        FileMoniker_Destroy
 *******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Destroy(FileMonikerImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    SEGPTR_FREE(This);
    return S_OK;
}

/******************************************************************************
 *                  FileMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                            REFIID riid, VOID** ppvResult)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;
    
    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                             REFIID riid, VOID** ppvResult)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%ld,%p,%p),stub!\n",This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%d,%p),stub!\n",This,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Enum
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%d,%p),stub!\n",This,fForward,ppenumMoniker);

    return E_NOTIMPL;

}

/******************************************************************************
 *        FileMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pmkOtherMoniker);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Hash
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pdwHash);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pmkNewlyRunning);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_GetTimeOfLastChange
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                                                   FILETIME* pFileTime)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pFileTime);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,ppmk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,
                                                IMoniker** ppmkPrefix)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmkOther,ppmkPrefix);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_RelativePathTo
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmOther,ppmkRelPath);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                              LPOLESTR *ppszDisplayName)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,ppszDisplayName);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_ParseDisplayName
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsSystemMonker
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    FileMonikerImpl* This=(FileMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pwdMksys);

    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateFileMoniker16
 ******************************************************************************/
HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName,LPMONIKER* ppmk){

    FIXME(ole,"(%s,%p),stub!\n",lpszPathName,ppmk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateFileMoniker32
 ******************************************************************************/
HRESULT WINAPI CreateFileMoniker(LPCOLESTR lpszPathName, LPMONIKER * ppmk)
{
    FileMonikerImpl* newFileMoniker = 0;
    HRESULT        hr = S_OK;

    TRACE(ole,"(%p,%p)\n",lpszPathName,ppmk);

    newFileMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(FileMonikerImpl));

    if (newFileMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = FileMonikerImpl_Construct(newFileMoniker,lpszPathName);

    if (FAILED(hr))
        return hr;

    hr = FileMonikerImpl_QueryInterface((IMoniker*)newFileMoniker,&IID_IMoniker,(void**)ppmk);

    return hr;
}
