/***************************************************************************************
 *	                      ItemMonikers implementation
 *
 *           Copyright 1999  Noomen Hamza
 ***************************************************************************************/

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "winerror.h"
#include "wine/obj_moniker.h"
#include "heap.h"
#include "debug.h"

typedef struct ItemMonikerImpl{

    ICOM_VTABLE(IMoniker)*  lpvtbl;

    ULONG ref;

} ItemMonikerImpl;

static HRESULT WINAPI ItemMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject);
static ULONG   WINAPI ItemMonikerImpl_AddRef(IMoniker* iface);
static ULONG   WINAPI ItemMonikerImpl_Release(IMoniker* iface);
static HRESULT WINAPI ItemMonikerImpl_GetClassID(const IMoniker* iface, CLSID *pClassID);
static HRESULT WINAPI ItemMonikerImpl_IsDirty(IMoniker* iface);
static HRESULT WINAPI ItemMonikerImpl_Load(IMoniker* iface, IStream* pStm);
static HRESULT WINAPI ItemMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty);
static HRESULT WINAPI ItemMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize);
static HRESULT WINAPI ItemMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI ItemMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
static HRESULT WINAPI ItemMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
static HRESULT WINAPI ItemMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite);
static HRESULT WINAPI ItemMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker);
static HRESULT WINAPI ItemMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker);
static HRESULT WINAPI ItemMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash);
static HRESULT WINAPI ItemMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
static HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pItemTime);
static HRESULT WINAPI ItemMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk);
static HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther, IMoniker** ppmkPrefix);
static HRESULT WINAPI ItemMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath);
static HRESULT WINAPI ItemMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName);
static HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
static HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys);

static HRESULT WINAPI ItemMonikerImpl_Construct(ItemMonikerImpl* iface, LPCOLESTR lpszDelim,LPCOLESTR lpszItem);
static HRESULT WINAPI ItemMonikerImpl_Destroy(ItemMonikerImpl* iface);

// Virtual function table for the ItemMonikerImpl class.
static ICOM_VTABLE(IMoniker) VT_ItemMonikerImpl =
    {
    ItemMonikerImpl_QueryInterface,
    ItemMonikerImpl_AddRef,
    ItemMonikerImpl_Release,
    ItemMonikerImpl_GetClassID,
    ItemMonikerImpl_IsDirty,
    ItemMonikerImpl_Load,
    ItemMonikerImpl_Save,
    ItemMonikerImpl_GetSizeMax,
    ItemMonikerImpl_BindToObject,
    ItemMonikerImpl_BindToStorage,
    ItemMonikerImpl_Reduce,
    ItemMonikerImpl_ComposeWith,
    ItemMonikerImpl_Enum,
    ItemMonikerImpl_IsEqual,
    ItemMonikerImpl_Hash,
    ItemMonikerImpl_IsRunning,
    ItemMonikerImpl_GetTimeOfLastChange,
    ItemMonikerImpl_Inverse,
    ItemMonikerImpl_CommonPrefixWith,
    ItemMonikerImpl_RelativePathTo,
    ItemMonikerImpl_GetDisplayName,
    ItemMonikerImpl_ParseDisplayName,
    ItemMonikerImpl_IsSystemMoniker
};

/*******************************************************************************
 *        ItemMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

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
  ItemMonikerImpl_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 *        ItemMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI ItemMonikerImpl_AddRef(IMoniker* iface)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    TRACE(ole,"(%p)\n",This);

    return ++(This->ref);
}

/******************************************************************************
 *        ItemMoniker_Release
 ******************************************************************************/
ULONG WINAPI ItemMonikerImpl_Release(IMoniker* iface)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    TRACE(ole,"(%p),stub!\n",This);

    This->ref--;

    if (This->ref==0){
        ItemMonikerImpl_Destroy(This);
        return 0;
    }

    return This->ref;
}

/******************************************************************************
 *        ItemMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetClassID(const IMoniker* iface, CLSID *pClassID)//Pointer to CLSID of object
{
    FIXME(ole,"(%p),stub!\n",pClassID);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsDirty(IMoniker* iface)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p),stub!\n",This);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Load
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Load(
          IMoniker* iface,
          IStream* pStm)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pStm);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Save
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Save(
          IMoniker* iface,
          IStream* pStm,
          BOOL fClearDirty)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%d),stub!\n",This,pStm,fClearDirty);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetSizeMax(
          IMoniker* iface,
          ULARGE_INTEGER* pcbSize)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pcbSize);
    return E_NOTIMPL;
}

/******************************************************************************
 *         ItemMoniker_Construct
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Construct(ItemMonikerImpl* This, LPCOLESTR lpszDelim,LPCOLESTR lpszItem){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,lpszDelim,lpszItem);

    memset(This, 0, sizeof(ItemMonikerImpl));

    //Initialize the virtual fgunction table.
    This->lpvtbl       = &VT_ItemMonikerImpl;
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_Destroy
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Destroy(ItemMonikerImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    SEGPTR_FREE(This);
    return S_OK;
}

/******************************************************************************
 *                  ItemMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_BindToObject(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                            REFIID riid, VOID** ppvResult)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;
    
    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_BindToStorage(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                             REFIID riid, VOID** ppvResult)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Reduce(IMoniker* iface,IBindCtx* pbc, DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%ld,%p,%p),stub!\n",This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_ComposeWith(IMoniker* iface,IMoniker* pmkRight,BOOL fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%d,%p),stub!\n",This,pmkRight,fOnlyIfNotGeneric,ppmkComposite);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Enum
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%d,%p),stub!\n",This,fForward,ppenumMoniker);
    return E_NOTIMPL;

}

/******************************************************************************
 *        ItemMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsEqual(IMoniker* iface,IMoniker* pmkOtherMoniker)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pmkOtherMoniker);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Hash
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pdwHash);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsRunning(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pmkNewlyRunning);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_GetTimeOfLastChange
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                                                   FILETIME* pFileTime)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pFileTime);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,ppmk);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(IMoniker* iface,IMoniker* pmkOther,
                                                IMoniker** ppmkPrefix)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmkOther,ppmkPrefix);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_RelativePathTo
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmOther,ppmkRelPath);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                              LPOLESTR *ppszDisplayName)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,ppszDisplayName);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_ParseDisplayName
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(IMoniker* iface,IBindCtx* pbc, IMoniker* pmkToLeft,
                                                LPOLESTR pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_IsSystemMonker
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    ItemMonikerImpl* This=(ItemMonikerImpl*)iface;

    FIXME(ole,"(%p,%p),stub!\n",This,pwdMksys);
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateItemMoniker16	[OLE2.28]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim,LPCOLESTR  lpszItem,LPMONIKER* ppmk){// [in] pathname [out] new moniker object

    FIXME(ole,"(%s,%p),stub!\n",lpszDelim,ppmk);
    *ppmk = NULL;
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateItemMoniker32	[OLE32.55]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker(LPCOLESTR lpszDelim,LPCOLESTR  lpszItem, LPMONIKER * ppmk)
{

    ItemMonikerImpl* newItemMoniker = 0;
    HRESULT        hr = S_OK;

    TRACE(ole,"(%p,%p,%p)\n",lpszDelim,lpszItem,ppmk);

    newItemMoniker = HeapAlloc(GetProcessHeap(), 0, sizeof(ItemMonikerImpl));

    if (newItemMoniker == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = ItemMonikerImpl_Construct(newItemMoniker,lpszDelim,lpszItem);

    if (FAILED(hr))
        return hr;

    hr = ItemMonikerImpl_QueryInterface((IMoniker*)newItemMoniker,&IID_IMoniker,(void**)ppmk);

    return hr;
}
