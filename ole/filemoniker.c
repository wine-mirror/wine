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

HRESULT WINAPI FileMonikerImpl_QueryInterface(FileMonikerImpl* This,REFIID riid,void** ppvObject);
ULONG   WINAPI FileMonikerImpl_AddRef(FileMonikerImpl* This);
ULONG   WINAPI FileMonikerImpl_Release(FileMonikerImpl* This);
HRESULT WINAPI FileMonikerImpl_GetClassID(FileMonikerImpl* This, CLSID *pClassID);
HRESULT WINAPI FileMonikerImpl_IsDirty(FileMonikerImpl* This);
HRESULT WINAPI FileMonikerImpl_Load(FileMonikerImpl* This,LPCOLESTR32 pszFileName,DWORD dwMode);
HRESULT WINAPI FileMonikerImpl_Save(FileMonikerImpl* This,LPCOLESTR32 pszFileName,BOOL32 fRemember);
HRESULT WINAPI FileMonikerImpl_GetSizeMax(FileMonikerImpl* This,LPOLESTR32 *ppszFileName);
HRESULT WINAPI FileMonikerImpl_Construct(FileMonikerImpl* This, LPCOLESTR32 lpszPathName);
HRESULT WINAPI FileMonikerImpl_destroy(FileMonikerImpl* This);
HRESULT WINAPI FileMonikerImpl_BindToObject(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
HRESULT WINAPI FileMonikerImpl_BindToStorage(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, REFIID riid, VOID** ppvResult);
HRESULT WINAPI FileMonikerImpl_Reduce(FileMonikerImpl* This,IBindCtx* pbc, DWORD dwReduceHowFar,IMoniker** ppmkToLeft, IMoniker** ppmkReduced);
HRESULT WINAPI FileMonikerImpl_ComposeWith(FileMonikerImpl* This,IMoniker* pmkRight,BOOL32 fOnlyIfNotGeneric, IMoniker** ppmkComposite);
HRESULT WINAPI FileMonikerImpl_Enum(FileMonikerImpl* This,BOOL32 fForward, IEnumMoniker** ppenumMoniker);
HRESULT WINAPI FileMonikerImpl_IsEqual(FileMonikerImpl* This,IMoniker* pmkOtherMoniker);
HRESULT WINAPI FileMonikerImpl_Hash(FileMonikerImpl* This,DWORD* pdwHash);
HRESULT WINAPI FileMonikerImpl_IsRunning(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, IMoniker* pmkNewlyRunning);
HRESULT WINAPI FileMonikerImpl_GetTimeOfLastChange(FileMonikerImpl* This, IBindCtx* pbc, IMoniker* pmkToLeft, FILETIME* pFileTime);
HRESULT WINAPI FileMonikerImpl_Inverse(FileMonikerImpl* This,IMoniker** ppmk);
HRESULT WINAPI FileMonikerImpl_CommonPrefixWith(FileMonikerImpl* This,IMoniker* pmkOther, IMoniker** ppmkPrefix);
HRESULT WINAPI FileMonikerImpl_RelativePathTo(FileMonikerImpl* This,IMoniker* pmOther, IMoniker** ppmkRelPath);
HRESULT WINAPI FileMonikerImpl_GetDisplayName(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR32 *ppszDisplayName);
HRESULT WINAPI FileMonikerImpl_ParseDisplayName(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft, LPOLESTR32 pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut);
HRESULT WINAPI FileMonikerImpl_IsSystemMoniker(FileMonikerImpl* This,DWORD* pwdMksys);
HRESULT WINAPI CreateFileMoniker16(LPCOLESTR16 lpszPathName,LPMONIKER * ppmk);
HRESULT WINAPI CreateFileMoniker32( LPCOLESTR32 lpszPathName, LPMONIKER * ppmk);

#define VTABLE_FUNC(a) (void*)(a)

// Virtual function table for the FileMonikerImpl class.
static ICOM_VTABLE(IMoniker) VT_FileMonikerImpl =
{
  {
   {
    {
      VTABLE_FUNC(FileMonikerImpl_QueryInterface),
      VTABLE_FUNC(FileMonikerImpl_AddRef),
      VTABLE_FUNC(FileMonikerImpl_Release)
    },
    VTABLE_FUNC(FileMonikerImpl_GetClassID)
   },
   VTABLE_FUNC(FileMonikerImpl_IsDirty),
   VTABLE_FUNC(FileMonikerImpl_Load),
   VTABLE_FUNC(FileMonikerImpl_Save),
   VTABLE_FUNC(FileMonikerImpl_GetSizeMax)
  },
  VTABLE_FUNC(FileMonikerImpl_BindToObject),
  VTABLE_FUNC(FileMonikerImpl_BindToStorage),
  VTABLE_FUNC(FileMonikerImpl_Reduce),
  VTABLE_FUNC(FileMonikerImpl_ComposeWith),
  VTABLE_FUNC(FileMonikerImpl_Enum),
  VTABLE_FUNC(FileMonikerImpl_IsEqual),
  VTABLE_FUNC(FileMonikerImpl_Hash),
  VTABLE_FUNC(FileMonikerImpl_IsRunning),
  VTABLE_FUNC(FileMonikerImpl_GetTimeOfLastChange),
  VTABLE_FUNC(FileMonikerImpl_Inverse),
  VTABLE_FUNC(FileMonikerImpl_CommonPrefixWith),
  VTABLE_FUNC(FileMonikerImpl_RelativePathTo),
  VTABLE_FUNC(FileMonikerImpl_GetDisplayName),
  VTABLE_FUNC(FileMonikerImpl_ParseDisplayName),
  VTABLE_FUNC(FileMonikerImpl_IsSystemMoniker)
};

/*******************************************************************************
 *        FileMoniker_QueryInterface
 *******************************************************************************/
HRESULT WINAPI FileMonikerImpl_QueryInterface(FileMonikerImpl* This,REFIID riid,void** ppvObject){

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
  FileMonikerImpl_AddRef(This);

  return S_OK;;
}

/******************************************************************************
 *        FileMoniker_AddRef
 ******************************************************************************/
ULONG WINAPI FileMonikerImpl_AddRef(FileMonikerImpl* This){

    TRACE(ole,"(%p)\n",This);

    return ++(This->ref);
}

/******************************************************************************
 *        FileMoniker_Release
 ******************************************************************************/
ULONG WINAPI FileMonikerImpl_Release(FileMonikerImpl* This){

    TRACE(ole,"(%p)\n",This);

    This->ref--;

    if (This->ref==0){
        FileMonikerImpl_destroy(This);
        return 0;
    }
    return This->ref;;
}

/******************************************************************************
 *        FileMoniker_GetClassID
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetClassID(FileMonikerImpl* This, CLSID *pClassID){//Pointer to CLSID of object

    FIXME(ole,"(%p,%p),stub!\n",This,pClassID);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsDirty
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsDirty(FileMonikerImpl* This)
{
    FIXME(ole,"(%p),stub!\n",This);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Load
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Load(
          FileMonikerImpl* This,
          LPCOLESTR32 pszFileName,//Pointer to absolute path of the file to open
          DWORD dwMode)           //Specifies the access mode from the STGM enumeration
{
    FIXME(ole,"(%p,%p,%ld),stub!\n",This,pszFileName,dwMode);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_save
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Save(
          FileMonikerImpl* This,
          LPCOLESTR32 pszFileName,   //Pointer to absolute path of the file where the object is saved
          BOOL32 fRemember)          //Specifies whether the file is to be the current working file or not
{
    FIXME(ole,"(%p,%p,%d),stub!\n",This,pszFileName,fRemember);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_GetSizeMax
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetSizeMax(
          FileMonikerImpl* This,
          LPOLESTR32 *ppszFileName)  //Pointer to the path for the current file or the default save prompt
{
    FIXME(ole,"(%p,%p),stub!\n",This,ppszFileName);

    return E_NOTIMPL;
}

/******************************************************************************
 *         FileMoniker_Constructor
 *******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Construct(FileMonikerImpl* This, LPCOLESTR32 lpszPathName){

    FIXME(ole,"(%p,%p),stub!\n",This,lpszPathName);

    memset(This, 0, sizeof(FileMonikerImpl));

    //Initialize the virtual fgunction table.
    This->lpvtbl       = &VT_FileMonikerImpl;
    return S_OK;
}

/******************************************************************************
 *        FileMoniker_destructor
 *******************************************************************************/
HRESULT WINAPI FileMonikerImpl_destroy(FileMonikerImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    SEGPTR_FREE(This);
    return S_OK;
}

/******************************************************************************
 *                  FileMoniker_BindToObject
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_BindToObject(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                            REFIID riid, VOID** ppvResult){
    
    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_BindToStorage
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_BindToStorage(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                             REFIID riid, VOID** ppvResult){

    FIXME(ole,"(%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,riid,ppvResult);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Reduce
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Reduce(FileMonikerImpl* This,IBindCtx* pbc, DWORD dwReduceHowFar,
                                      IMoniker** ppmkToLeft, IMoniker** ppmkReduced){

    FIXME(ole,"(%p,%p,%ld,%p,%p),stub!\n",This,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_ComposeWith
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_ComposeWith(FileMonikerImpl* This,IMoniker* pmkRight,BOOL32 fOnlyIfNotGeneric,
                                           IMoniker** ppmkComposite){

    FIXME(ole,"(%p,%p,%d,%p),stub!\n",This,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Enum
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Enum(FileMonikerImpl* This,BOOL32 fForward, IEnumMoniker** ppenumMoniker){

    FIXME(ole,"(%p,%d,%p),stub!\n",This,fForward,ppenumMoniker);

    return E_NOTIMPL;

}

/******************************************************************************
 *        FileMoniker_IsEqual
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsEqual(FileMonikerImpl* This,IMoniker* pmkOtherMoniker){

    FIXME(ole,"(%p,%p),stub!\n",This,pmkOtherMoniker);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Hash
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Hash(FileMonikerImpl* This,DWORD* pdwHash){

    FIXME(ole,"(%p,%p),stub!\n",This,pdwHash);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsRunning
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsRunning(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning){

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pmkNewlyRunning);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_GetTimeOfLastChange
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetTimeOfLastChange(FileMonikerImpl* This, IBindCtx* pbc, IMoniker* pmkToLeft,
                                                   FILETIME* pFileTime){

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pFileTime);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_Inverse
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_Inverse(FileMonikerImpl* This,IMoniker** ppmk){

    FIXME(ole,"(%p,%p),stub!\n",This,ppmk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_CommonPrefixWith
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_CommonPrefixWith(FileMonikerImpl* This,IMoniker* pmkOther,
                                                IMoniker** ppmkPrefix){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmkOther,ppmkPrefix);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_RelativePathTo
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_RelativePathTo(FileMonikerImpl* This,IMoniker* pmOther, IMoniker** ppmkRelPath){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pmOther,ppmkRelPath);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_GetDisplayName
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_GetDisplayName(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                              LPOLESTR32 *ppszDisplayName){

    FIXME(ole,"(%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,ppszDisplayName);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_ParseDisplayName
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_ParseDisplayName(FileMonikerImpl* This,IBindCtx* pbc, IMoniker* pmkToLeft,
                                                LPOLESTR32 pszDisplayName, ULONG* pchEaten, IMoniker** ppmkOut){

    FIXME(ole,"(%p,%p,%p,%p,%p,%p),stub!\n",This,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);

    return E_NOTIMPL;
}

/******************************************************************************
 *        FileMoniker_IsSystemMonker
 ******************************************************************************/
HRESULT WINAPI FileMonikerImpl_IsSystemMoniker(FileMonikerImpl* This,DWORD* pwdMksys){

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
HRESULT WINAPI CreateFileMoniker32(LPCOLESTR32 lpszPathName, LPMONIKER * ppmk)
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

    hr = FileMonikerImpl_QueryInterface(newFileMoniker,&IID_IMoniker,(void**)ppmk);

    return hr;
}
