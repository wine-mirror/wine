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
#include "wine/obj_moniker.h"
#include "debug.h"
#include "heap.h"

typedef struct BindCtxImpl{

    ICOM_VTABLE(IBindCtx)*  lpvtbl;   
                                     
    ULONG ref;

} BindCtxImpl;


HRESULT WINAPI BindCtxImpl_QueryInterface(IBindCtx* iface,REFIID riid,void** ppvObject);
ULONG   WINAPI BindCtxImpl_AddRef(IBindCtx* iface);
ULONG   WINAPI BindCtxImpl_Release(IBindCtx* iface);
HRESULT WINAPI BindCtxImpl_RegisterObjectBound(IBindCtx* iface,IUnknown* punk);
HRESULT WINAPI BindCtxImpl_RevokeObjectBound(IBindCtx* iface, IUnknown* punk);
HRESULT WINAPI BindCtxImpl_ReleaseObjects(IBindCtx* iface);
HRESULT WINAPI BindCtxImpl_SetBindOptions(IBindCtx* iface,LPBIND_OPTS2 pbindopts);
HRESULT WINAPI BindCtxImpl_GetBindOptions(IBindCtx* iface,LPBIND_OPTS2 pbindopts);
HRESULT WINAPI BindCtxImpl_GetRunningObjectTable(IBindCtx* iface,IRunningObjectTable** pprot);
HRESULT WINAPI BindCtxImpl_RegisterObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk);
HRESULT WINAPI BindCtxImpl_GetObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk);
HRESULT WINAPI BindCtxImpl_EnumObjectParam(IBindCtx* iface,IEnumString** ppenum);
HRESULT WINAPI BindCtxImpl_RevokeObjectParam(IBindCtx* iface,LPOLESTR pszkey);

HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC * ppbc);
HRESULT WINAPI CreateBindCtx(DWORD reserved, LPBC * ppbc);

HRESULT WINAPI BindCtxImpl_Construct(BindCtxImpl* This);
HRESULT WINAPI BindCtxImpl_Destroy(BindCtxImpl* This);

// Virtual function table for the BindCtx class.
static ICOM_VTABLE(IBindCtx) VT_BindCtxImpl =
    {
    BindCtxImpl_QueryInterface,
    BindCtxImpl_AddRef,
    BindCtxImpl_Release,
    BindCtxImpl_RegisterObjectBound,
    BindCtxImpl_RevokeObjectBound,
    BindCtxImpl_ReleaseObjects,
    BindCtxImpl_SetBindOptions,
    BindCtxImpl_GetBindOptions,
    BindCtxImpl_GetRunningObjectTable,
    BindCtxImpl_RegisterObjectParam,
    BindCtxImpl_GetObjectParam,
    BindCtxImpl_EnumObjectParam,
    BindCtxImpl_RevokeObjectParam
};

/*******************************************************************************
 *        BindCtx_QueryInterface
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_QueryInterface(IBindCtx* iface,REFIID riid,void** ppvObject)
{
  ICOM_THIS(BindCtxImpl,iface);
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
  BindCtxImpl_AddRef(iface);

  return S_OK;
}

/******************************************************************************
 *       BindCtx_ _AddRef
 ******************************************************************************/
ULONG WINAPI BindCtxImpl_AddRef(IBindCtx* iface)
{
    ICOM_THIS(BindCtxImpl,iface);
    TRACE(ole,"(%p)\n",This);

    return ++(This->ref);
}

/******************************************************************************
 *        BindCtx_Release
 ******************************************************************************/
ULONG WINAPI BindCtxImpl_Release(IBindCtx* iface)
{
    ICOM_THIS(BindCtxImpl,iface);
    TRACE(ole,"(%p)\n",This);

    This->ref--;

    if (This->ref==0){
        BindCtxImpl_Destroy(This);
        return 0;
    }
    return This->ref;;
}


/******************************************************************************
 *         BindCtx_Construct
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_Construct(BindCtxImpl* This)
{
    FIXME(ole,"(%p),stub!\n",This);

    memset(This, 0, sizeof(BindCtxImpl));

    //Initialize the virtual fgunction table.
    This->lpvtbl       = &VT_BindCtxImpl;

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_Destroy
 *******************************************************************************/
HRESULT WINAPI BindCtxImpl_Destroy(BindCtxImpl* This)
{
    FIXME(ole,"(%p),stub!\n",This);

    SEGPTR_FREE(This);

    return S_OK;
}


/******************************************************************************
 *        BindCtx_RegisterObjectBound
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RegisterObjectBound(IBindCtx* iface,IUnknown* punk)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_RevokeObjectBound
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RevokeObjectBound(IBindCtx* iface, IUnknown* punk)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_ReleaseObjects
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_ReleaseObjects(IBindCtx* iface)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p),stub!\n",This);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_SetBindOptions
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_SetBindOptions(IBindCtx* iface,LPBIND_OPTS2 pbindopts)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,pbindopts);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_GetBindOptions
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetBindOptions(IBindCtx* iface,LPBIND_OPTS2 pbindopts)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,pbindopts);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_GetRunningObjectTable
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetRunningObjectTable(IBindCtx* iface,IRunningObjectTable** pprot)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,pprot);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_RegisterObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RegisterObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p,%p),stub!\n",This,pszkey,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_GetObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_GetObjectParam(IBindCtx* iface,LPOLESTR pszkey, IUnknown* punk)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p,%p),stub!\n",This,pszkey,punk);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_EnumObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_EnumObjectParam(IBindCtx* iface,IEnumString** ppenum)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,ppenum);

    return E_NOTIMPL;
}

/******************************************************************************
 *        BindCtx_RevokeObjectParam
 ******************************************************************************/
HRESULT WINAPI BindCtxImpl_RevokeObjectParam(IBindCtx* iface,LPOLESTR pszkey)
{
    ICOM_THIS(BindCtxImpl,iface);
    FIXME(ole,"(%p,%p),stub!\n",This,pszkey);

    return E_NOTIMPL;
}


/******************************************************************************
 *        CreateBindCtx16
 ******************************************************************************/
HRESULT WINAPI CreateBindCtx16(DWORD reserved, LPBC * ppbc)
{
    FIXME(ole,"(%ld,%p),stub!\n",reserved,ppbc);

    return E_NOTIMPL;
}

/******************************************************************************
 *        CreateBindCtx32
 ******************************************************************************/
HRESULT WINAPI CreateBindCtx(DWORD reserved, LPBC * ppbc)
{
    BindCtxImpl* newBindCtx = 0;
    HRESULT        hr = S_OK;

    TRACE(ole,"(%ld,%p)\n",reserved,ppbc);

    newBindCtx = HeapAlloc(GetProcessHeap(), 0, sizeof(BindCtxImpl));

    if (newBindCtx == 0)
        return STG_E_INSUFFICIENTMEMORY;

    hr = BindCtxImpl_Construct(newBindCtx);

    if (FAILED(hr))
        return hr;

    hr = BindCtxImpl_QueryInterface((IBindCtx*)newBindCtx,&IID_IBindCtx,(void**)ppbc);

    return hr;
}
