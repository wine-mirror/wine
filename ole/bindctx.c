/***************************************************************************************
 *	                      BindCtx implementation
 *
 *  Copyright 1999  Noomen Hamza
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

typedef struct BindCtxImpl{

    ICOM_VTABLE(IBindCtx)*  lpvtbl;   
                                     
    ULONG ref;

} BindCtxImpl;


HRESULT WINAPI BindCtxImpl_QueryInterface(BindCtxImpl* This,REFIID riid,void** ppvObject);
ULONG   WINAPI BindCtxImpl_AddRef(BindCtxImpl* This);
ULONG   WINAPI BindCtxImpl_Release(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_Construct(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_destroy(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_RegisterObjectBound(BindCtxImpl* This,IUnknown* punk);
HRESULT WINAPI BindCtxImpl_RevokeObjectBound(BindCtxImpl* This, IUnknown* punk);
HRESULT WINAPI BindCtxImpl_ReleaseObjects(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_SetBindOptions(BindCtxImpl* This,LPBIND_OPTS2 pbindopts);
HRESULT WINAPI BindCtxImpl_GetBindOptions(BindCtxImpl* This,LPBIND_OPTS2 pbindopts);
HRESULT WINAPI BindCtxImpl_GetRunningObjectTable(BindCtxImpl* This,IRunningObjectTable** pprot);
HRESULT WINAPI BindCtxImpl_RegisterObjectParam(BindCtxImpl* This,LPOLESTR32 pszkey, IUnknown* punk);
HRESULT WINAPI BindCtxImpl_GetObjectParam(BindCtxImpl* This,LPOLESTR32 pszkey, IUnknown* punk);
HRESULT WINAPI BindCtxImpl_EnumObjectParam(BindCtxImpl* This,IEnumString** ppenum);
HRESULT WINAPI BindCtxImpl_RevokeObjectParam(BindCtxImpl* This,LPOLESTR32 pszkey);
HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC * ppbc);
HRESULT WINAPI CreateBindCtx32(DWORD reserved, LPBC * ppbc);

#define VTABLE_FUNC(a) (void*)(a)
// Virtual function table for the BindCtx class.
static ICOM_VTABLE(IBindCtx) VT_BindCtxImpl =
{
    {
      VTABLE_FUNC(BindCtxImpl_QueryInterface),
      VTABLE_FUNC(BindCtxImpl_AddRef),
      VTABLE_FUNC(BindCtxImpl_Release)
    },
    VTABLE_FUNC(BindCtxImpl_RegisterObjectBound),
    VTABLE_FUNC(BindCtxImpl_RevokeObjectBound),
    VTABLE_FUNC(BindCtxImpl_ReleaseObjects),
    VTABLE_FUNC(BindCtxImpl_SetBindOptions),
    VTABLE_FUNC(BindCtxImpl_GetBindOptions),
    VTABLE_FUNC(BindCtxImpl_GetRunningObjectTable),
    VTABLE_FUNC(BindCtxImpl_RegisterObjectParam),
    VTABLE_FUNC(BindCtxImpl_GetObjectParam),
    VTABLE_FUNC(BindCtxImpl_EnumObjectParam),
    VTABLE_FUNC(BindCtxImpl_RevokeObjectParam)
};

/*******************************************************************************
 *        BindCtx_QueryInterface
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_QueryInterface(BindCtxImpl* This,REFIID riid,void** ppvObject){

  TRACE(ole,"(%p,%p,%p)\n",This,riid,ppvObject);
  // Perform a sanity check on the parameters.
  if ( (This==0) || (ppvObject==0) )    return E_INVALIDARG;
  
  // Initialize the return parameter.
  *ppvObject = 0;

  // Compare the riid with the interface IDs implemented by this object.
  if (memcmp(&IID_IUnknown, riid, sizeof(IID_IUnknown)) == 0)
      *ppvObject = (IBindCtx*)This;
  else
      if (memcmp(&IID_IBindCtx, riid, sizeof(IID_IBindCtx)) == 0)
          *ppvObject = (IBindCtx*)This;

  // Check that we obtained an interface.
  if ((*ppvObject)==0)        return E_NOINTERFACE;
  
   // Query Interface always increases the reference count by one when it is successful
  BindCtxImpl_AddRef(This);

  return S_OK;
}

/******************************************************************************
 *       BindCtx_ _AddRef
 ******************************************************************************/
ULONG WINAPI BindCtxImpl_AddRef(BindCtxImpl* This){

    TRACE(ole,"(%p)\n",This);

    return ++(This->ref);
}

/******************************************************************************
 *        BindCtx_Release
 ******************************************************************************/
ULONG WINAPI BindCtxImpl_Release(BindCtxImpl* This){

    TRACE(ole,"(%p)\n",This);

    This->ref--;

    if (This->ref==0){
        BindCtxImpl_destroy(This);
        return 0;
    }
    return This->ref;;
}


/******************************************************************************
 *         BindCtx_Constructor
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_Construct(BindCtxImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    memset(This, 0, sizeof(BindCtxImpl));

    //Initialize the virtual fgunction table.
    This->lpvtbl       = &VT_BindCtxImpl;

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_destructor
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_destroy(BindCtxImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    SEGPTR_FREE(This);

    return S_OK;
}


/******************************************************************************
 *        BindCtx_RegisterObjectBound
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RegisterObjectBound(BindCtxImpl* This,IUnknown* punk){

    FIXME(ole,"(%p,%p),stub!\n",This,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_RevokeObjectBound
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RevokeObjectBound(BindCtxImpl* This, IUnknown* punk){

    FIXME(ole,"(%p,%p),stub!\n",This,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_ReleaseObjects
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_ReleaseObjects(BindCtxImpl* This){

    FIXME(ole,"(%p),stub!\n",This);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_SetBindOptions
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_SetBindOptions(BindCtxImpl* This,LPBIND_OPTS2 pbindopts){

    FIXME(ole,"(%p,%p),stub!\n",This,pbindopts);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_GetBindOptions
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetBindOptions(BindCtxImpl* This,LPBIND_OPTS2 pbindopts){

    FIXME(ole,"(%p,%p),stub!\n",This,pbindopts);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_GetRunningObjectTable
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetRunningObjectTable(BindCtxImpl* This,IRunningObjectTable** pprot){

    FIXME(ole,"(%p,%p),stub!\n",This,pprot);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_RegisterObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RegisterObjectParam(BindCtxImpl* This,LPOLESTR32 pszkey, IUnknown* punk){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pszkey,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_GetObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetObjectParam(BindCtxImpl* This,LPOLESTR32 pszkey, IUnknown* punk){

    FIXME(ole,"(%p,%p,%p),stub!\n",This,pszkey,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_EnumObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_EnumObjectParam(BindCtxImpl* This,IEnumString** ppenum){

    FIXME(ole,"(%p,%p),stub!\n",This,ppenum);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_RevokeObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RevokeObjectParam(BindCtxImpl* This,LPOLESTR32 pszkey){

    FIXME(ole,"(%p,%p),stub!\n",This,pszkey);

    return E_NOTIMPL;
}


/******************************************************************************
 *        CreateBindCtx16
 ******************************************************************************/
HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC * ppbc){

    FIXME(ole,"(%ld,%p),stub!\n",reserved,ppbc);

    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateBindCtx32
 ******************************************************************************/
HRESULT WINAPI CreateBindCtx32(DWORD reserved, LPBC * ppbc){

    BindCtxImpl* newBindCtx = 0;
    HRESULT        hr = S_OK;

    TRACE(ole,"(%ld,%p)\n",reserved,ppbc);

    newBindCtx = HeapAlloc(GetProcessHeap(), 0, sizeof(BindCtxImpl));

    if (newBindCtx == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = BindCtxImpl_Construct(newBindCtx);

    if (FAILED(hr))
        return hr;

    hr = BindCtxImpl_QueryInterface(newBindCtx,&IID_IBindCtx,(void**)ppbc);

    return hr;
}
