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
#include "wine/obj_base.h"
#include "wine/obj_storage.h"
#include "wine/obj_moniker.h"
#include "debug.h"
#include "heap.h"

typedef struct ItemMonikerImpl{

    ICOM_VTABLE(IMoniker)*  lpvtbl;

    ULONG ref;

} ItemMonikerImpl;

HRESULT WINAPI ItemMonikerImpl_QueryInterface(ItemMonikerImpl* This,REFIID riid,void** ppvObject);
ULONG   WINAPI ItemMonikerImpl_AddRef(ItemMonikerImpl* This);
ULONG   WINAPI ItemMonikerImpl_Release(ItemMonikerImpl* This);
HRESULT WINAPI ItemMonikerImpl_GetClassID(ItemMonikerImpl* This, CLSID *pClassID);
HRESULT WINAPI ItemMonikerImpl_IsDirty(ItemMonikerImpl* This);
HRESULT WINAPI ItemMonikerImpl_Load(ItemMonikerImpl* This,LPCOLESTR32 pszItemName,DWORD dwMode);
HRESULT WINAPI ItemMonikerImpl_Save(ItemMonikerImpl* This,LPCOLESTR32 pszItemName,BOOL32 fRemember);
HRESULT WINAPI ItemMonikerImpl_GetSizeMax(ItemMonikerImpl* This,LPOLESTR32 *ppszItemName);
HRESULT WINAPI ItemMonikerImpl_Construct(ItemMonikerImpl* This, LPCOLESTR32 lpszDelim,LPCOLESTR32 lpszItem);
HRESULT WINAPI ItemMonikerImpl_destroy(ItemMonikerImpl* This);
HRESULT WINAPI ItemMonikerImpl_BindToObject(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
HRESULT WINAPI ItemMonikerImpl_BindToStorage(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
HRESULT WINAPI ItemMonikerImpl_Reduce(ItemMonikerImpl* This,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
HRESULT WINAPI ItemMonikerImpl_ComposeWith(ItemMonikerImpl* This,IMoniker* pmkRight,BOOL32 fOnlyIfNotGeneric, IMoniker** ppmkComposite);
HRESULT WINAPI ItemMonikerImpl_Enum(ItemMonikerImpl* This,BOOL32 fForward, IEnumMoniker** ppenumMoniker);
HRESULT WINAPI ItemMonikerImpl_IsEqual(ItemMonikerImpl* This,IMoniker* pmkOtherMoniker);
HRESULT WINAPI ItemMonikerImpl_Hash(ItemMonikerImpl* This,DWORD* pdwHash);
HRESULT WINAPI ItemMonikerImpl_IsRunning(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(ItemMonikerImpl* This, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pItemTime);
HRESULT WINAPI ItemMonikerImpl_Inverse(ItemMonikerImpl* This,IMoniker** ppmk);
HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(ItemMonikerImpl* This,IMoniker* pmkOther, IMoniker** ppmkPrefix);
HRESULT WINAPI ItemMonikerImpl_RelativePathTo(ItemMonikerImpl* This,IMoniker* pmOther, IMoniker** ppmkRelPath);
HRESULT WINAPI ItemMonikerImpl_GetDisplayName(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR32 *ppszDisplayName);
HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR32 pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(ItemMonikerImpl* This,DWORD* pwdMksys);
HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim,LPCOLESTR32  lpszItem,LPMONIKER * ppmk);
HRESULT WINAPI CreateItemMoniker32(LPCOLESTR32 lpszDelim,LPCOLESTR32  lpszItem,LPMONIKER * ppmk);

#define VTABLE_FUNC(a) (void*)(a)
// Virtual function table for the ItemMonikerImpl class.
static ICOM_VTABLE(IMoniker) VT_ItemMonikerImpl =
{
  {
   {
    {
      VTABLE_FUNC(ItemMonikerImpl_QueryInterface),
      VTABLE_FUNC(ItemMonikerImpl_AddRef),
      VTABLE_FUNC(ItemMonikerImpl_Release)
    },
    VTABLE_FUNC(ItemMonikerImpl_GetClassID)
   },
   VTABLE_FUNC(ItemMonikerImpl_IsDirty),
   VTABLE_FUNC(ItemMonikerImpl_Load),
   VTABLE_FUNC(ItemMonikerImpl_Save),
   VTABLE_FUNC(ItemMonikerImpl_GetSizeMax)
  },
  VTABLE_FUNC(ItemMonikerImpl_BindToObject),
  VTABLE_FUNC(ItemMonikerImpl_BindToStorage),
  VTABLE_FUNC(ItemMonikerImpl_Reduce),
  VTABLE_FUNC(ItemMonikerImpl_ComposeWith),
  VTABLE_FUNC(ItemMonikerImpl_Enum),
  VTABLE_FUNC(ItemMonikerImpl_IsEqual),
  VTABLE_FUNC(ItemMonikerImpl_Hash),
  VTABLE_FUNC(ItemMonikerImpl_IsRunning),
  VTABLE_FUNC(ItemMonikerImpl_GetTimeOfLastChange),
  VTABLE_FUNC(ItemMonikerImpl_Inverse),
  VTABLE_FUNC(ItemMonikerImpl_CommonPrefixWith),
  VTABLE_FUNC(ItemMonikerImpl_RelativePathTo),
  VTABLE_FUNC(ItemMonikerImpl_GetDisplayName),
  VTABLE_FUNC(ItemMonikerImpl_ParseDisplayName),
  VTABLE_FUNC(ItemMonikerImpl_IsSystemMoniker)
};

/*******************************************************************************
 *        ItemMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_QueryInterface(ItemMonikerImpl* This,REFIID riid,void** ppvObject){

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
  ItemMonikerImpl_AddRef(This);

  return S_OK;;
}

/******************************************************************************
 *        ItemMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI ItemMonikerImpl_AddRef(ItemMonikerImpl* This){

    TRACE(ole,"(%p)\n",This);

    return ++(This->ref);
}

/******************************************************************************
 *        ItemMoniker_Release
 ******************************************************************************/
ULONG WINAPI ItemMonikerImpl_Release(ItemMonikerImpl* This){

    TRACE(ole,"(%p),stub!\n",This);

    This->ref--;

    if (This->ref==0){
        ItemMonikerImpl_destroy(This);
        return 0;
    }

    return This->ref;;
}

/******************************************************************************
 *        ItemMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetClassID(ItemMonikerImpl* This, CLSID *pClassID){//Pointer to CLSID of object

    FIXME(ole,"(%p),stub!\n",pClassID);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsDirty(ItemMonikerImpl* This)
{
    FIXME(ole,"(%p),stub!\n",This);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Load
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Load(
          ItemMonikerImpl* This,
          LPCOLESTR32 pszFileName,//Pointer to absolute path of the file to open
          DWORD dwMode)           //Specifies the access mode from the STGM enumeration
{
    FIXME(ole,"(%p,%ld),stub!\n",pszFileName,dwMode);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_save
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Save(
          ItemMonikerImpl* This,
          LPCOLESTR32 pszFileName,   //Pointer to absolute path of the file where the object is saved
          BOOL32 fRemember)          //Specifies whether the file is to be the current working file or not
{
    FIXME(ole,"(%p,%d),stub!\n",pszFileName,fRemember);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetSizeMax(
          ItemMonikerImpl* This,
          LPOLESTR32 *ppszFileName)  //Pointer to the path for the current file or the default save prompt
{
    FIXME(ole,"(%p),stub!\n",ppszFileName);
    return E_NOTIMPL;
}

/******************************************************************************
 *         ItemMoniker_Constructor
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Construct(ItemMonikerImpl* This, LPCOLESTR32 lpszDelim,LPCOLESTR32 lpszItem){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,lpszDelim,lpszItem);

    memset(This, 0, sizeof(ItemMonikerImpl));

    //Initialize the virtual fgunction table.
    This->lpvtbl       = &VT_ItemMonikerImpl;
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_destructor
 *******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_destroy(ItemMonikerImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    SEGPTR_FREE(This);
    return S_OK;
}

/******************************************************************************
 *                  ItemMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_BindToObject(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                            REFIID riid, VOID** ppvResult){
    
    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_BindToStorage(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                             REFIID riid, VOID** ppvResult){

    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Reduce(ItemMonikerImpl* This,IBindCtx* pbc, DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft, IMoniker** ppmkReduced){

    FIXME(ole,"(%p,%p,%ld,%p,%p),stub!\n",This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_ComposeWith(ItemMonikerImpl* This,IMoniker* pmkRight,BOOL32 fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite){

    FIXME(ole,"(%p,%p,%d,%p),stub!\n",This,pmkRight,fOnlyIfNotGeneric,ppmkComposite);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Enum
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Enum(ItemMonikerImpl* This,BOOL32 fForward, IEnumMoniker** ppenumMoniker){

    FIXME(ole,"(%p,%d,%p),stub!\n",This,fForward,ppenumMoniker);
    return E_NOTIMPL;

}

/******************************************************************************
 *        ItemMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsEqual(ItemMonikerImpl* This,IMoniker* pmkOtherMoniker){

    FIXME(ole,"(%p,%p),stub!\n",This,pmkOtherMoniker);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Hash
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Hash(ItemMonikerImpl* This,DWORD* pdwHash){

    FIXME(ole,"(%p,%p),stub!\n",This,pdwHash);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsRunning(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning){

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pmkNewlyRunning);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_GetTimeOfLastChange
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(ItemMonikerImpl* This, IBindCtx* pbc, IMoniker* pmkToLeft,
                                                   FILETIME* pFileTime){

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pFileTime);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_Inverse(ItemMonikerImpl* This,IMoniker** ppmk){

    FIXME(ole,"(%p,%p),stub!\n",This,ppmk);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(ItemMonikerImpl* This,IMoniker* pmkOther,
                                                IMoniker** ppmkPrefix){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmkOther,ppmkPrefix);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_RelativePathTo
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_RelativePathTo(ItemMonikerImpl* This,IMoniker* pmOther, IMoniker** ppmkRelPath){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmOther,ppmkRelPath);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_GetDisplayName(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                              LPOLESTR32 *ppszDisplayName){

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,ppszDisplayName);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_ParseDisplayName
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(ItemMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                                LPOLESTR32 pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut){

    FIXME(ole,"(%p,%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

/******************************************************************************
 *        ItemMoniker_IsSystemMonker
 ******************************************************************************/
HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(ItemMonikerImpl* This,DWORD* pwdMksys){

    FIXME(ole,"(%p,%p),stub!\n",This,pwdMksys);
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateItemMoniker16	[OLE2.28]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker16(LPCOLESTR16 lpszDelim,LPCOLESTR32  lpszItem,LPMONIKER* ppmk){// [in] pathname [out] new moniker object

    FIXME(ole,"(%s,%p),stub!\n",lpszDelim,ppmk);
    *ppmk = NULL;
    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateItemMoniker32	[OLE32.55]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker32(LPCOLESTR32 lpszDelim,LPCOLESTR32  lpszItem, LPMONIKER * ppmk)
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

    hr = ItemMonikerImpl_QueryInterface(newItemMoniker,&IID_IMoniker,(void**)ppmk);

    return hr;
}
