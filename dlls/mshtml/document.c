/*
 *    HTML Document class
 *
 * Copyright 2003 Mike McCormack
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "docobj.h"
#include "ole2.h"
#include "uuids.h"
#include "urlmon.h"
#include "oleidl.h"
#include "objidl.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

typedef struct _HTMLDocument_impl {
    ICOM_VTABLE(IPersistMoniker) *IPersistMoniker_vtbl;
    ICOM_VTABLE(IPersistFile) *IPersistFile_vtbl;
    ICOM_VTABLE(IPersist) *IPersist_vtbl;
    ICOM_VTABLE(IOleObject) *IOleObject_vtbl;
    ICOM_VTABLE(IViewObject) *IViewObject_vtbl;
    ICOM_VTABLE(IOleDocument) *IOleDocument_vtbl;
    ULONG ref_count;
    IOleClientSite *site;
} HTMLDocument_impl;


static ULONG WINAPI HTMLDocument_AddRef(HTMLDocument_impl *This)
{
    return ++This->ref_count;
}

static ULONG WINAPI HTMLDocument_Release(HTMLDocument_impl *This)
{
    ULONG count;

    count = --This->ref_count ;
    if( !count )
        HeapFree( GetProcessHeap(), 0, This );
    
    return count;
}

static HRESULT WINAPI HTMLDocument_QueryInterface(
         HTMLDocument_impl *This, REFIID riid, LPVOID *ppv)
{
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppv);

    if( IsEqualGUID( riid, &IID_IUnknown ) )
    {
        TRACE("returning IUnknown\n");
        HTMLDocument_AddRef(This);
        *ppv = This;
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IPersistMoniker ) )
    {
        TRACE("returning IPersistMoniker\n");
        HTMLDocument_AddRef(This);
        *ppv = This;
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IPersistFile ) )
    {
        TRACE("returning IPersistFile\n");
        HTMLDocument_AddRef(This);
        *ppv = &(This->IPersistFile_vtbl);
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IOleObject ) )
    {
        TRACE("returning IOleObject\n");
        HTMLDocument_AddRef(This);
        *ppv = &(This->IOleObject_vtbl);
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IPersist ) )
    {
        TRACE("returning IPersist\n");
        HTMLDocument_AddRef(This);
        *ppv = &(This->IPersist_vtbl);
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IViewObject ) )
    {
        TRACE("returning IViewObject\n");
        HTMLDocument_AddRef(This);
        *ppv = &(This->IViewObject_vtbl);
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IOleDocument ) )
    {
        TRACE("returning IOleDocument\n");
        HTMLDocument_AddRef(This);
        *ppv = &(This->IOleDocument_vtbl);
        return S_OK;
    }
    if( IsEqualGUID( riid, &IID_IDispatch ) )
    {
        TRACE("returning IDispatch\n");
    }
    if( IsEqualGUID( riid, &IID_IOleCommandTarget ) )
    {
        TRACE("returning IOleCommandTarget\n");
    }
    return E_NOINTERFACE;
}

static HRESULT WINAPI fnIPersistMoniker_QueryInterface(
         IPersistMoniker *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    return HTMLDocument_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI fnIPersistMoniker_AddRef(IPersistMoniker *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    return HTMLDocument_AddRef(This);
}

static ULONG WINAPI fnIPersistMoniker_Release(IPersistMoniker *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    return HTMLDocument_Release(This);
}

static HRESULT WINAPI fnIPersistMoniker_GetClassID(IPersistMoniker *iface,
           CLSID *pClassID )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistMoniker_IsDirty(IPersistMoniker *iface )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistMoniker_Load(IPersistMoniker *iface,
           BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    IStream *stm;
    HRESULT r;
    unsigned char buffer[0x201];
    ULONG count;

    TRACE("%p %d %p %p %08lx\n", This, 
          fFullyAvailable, pimkName, pibc, grfMode);

    r = IMoniker_BindToStorage( pimkName, pibc, NULL, 
                                &IID_IStream, (LPVOID*) &stm );
    if( FAILED( r ) )
    {
        TRACE("IMoniker_BindToStorage failed %08lx\n", r);
        return r;
    }

    while( 1 )
    {
        r = IStream_Read( stm, buffer, sizeof buffer-1, &count);
        if( FAILED( r ) )
            break;
        if( count == 0 )
            break;
        buffer[count]=0;
        TRACE("%s\n",buffer);
    }

    IStream_Release( stm );

    return S_OK;
}

static HRESULT WINAPI fnIPersistMoniker_Save(IPersistMoniker *iface,
           IMoniker *pinkName, LPBC pibc, BOOL fRemember )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    FIXME("%p %p %p %d\n", This, pinkName, pibc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistMoniker_SaveCompleted(IPersistMoniker *iface,
           IMoniker *pinkName, LPBC pibc )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    FIXME("%p %p %p\n", This, pinkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistMoniker_GetCurMoniker(IPersistMoniker *iface,
           IMoniker **pinkName )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistMoniker_vtbl, iface);
    FIXME("%p\n", This);
    return E_NOTIMPL;
}

static ICOM_VTABLE(IPersistMoniker) IPersistMoniker_vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    fnIPersistMoniker_QueryInterface,
    fnIPersistMoniker_AddRef,
    fnIPersistMoniker_Release,
    fnIPersistMoniker_GetClassID,
    fnIPersistMoniker_IsDirty,
    fnIPersistMoniker_Load,
    fnIPersistMoniker_Save,
    fnIPersistMoniker_SaveCompleted,
    fnIPersistMoniker_GetCurMoniker
};

static ULONG WINAPI fnIPersistFile_AddRef(IPersistFile *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    return HTMLDocument_AddRef(This);
}

static ULONG WINAPI fnIPersistFile_Release(IPersistFile *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    return HTMLDocument_Release(This);
}

static HRESULT WINAPI fnIPersistFile_QueryInterface(
           IPersistFile *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    return HTMLDocument_QueryInterface(This, riid, ppv);
}

static HRESULT WINAPI fnIPersistFile_GetClassID(IPersistFile *iface,
           CLSID *pClassID )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);

    TRACE("%p\n", This);

    memcpy( pClassID, &CLSID_HTMLDocument, sizeof CLSID_HTMLDocument);
    return S_OK;
}

static HRESULT WINAPI fnIPersistFile_IsDirty(IPersistFile *iface )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistFile_Load(IPersistFile *iface,
           LPCOLESTR pszFileName, DWORD grfMode )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    FIXME("%p %s %08lx\n", This, debugstr_w(pszFileName), grfMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistFile_Save(IPersistFile *iface, 
           LPCOLESTR pszFileName, BOOL fRemember )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    FIXME("%p %s %d\n", This, debugstr_w(pszFileName), fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistFile_SaveCompleted(IPersistFile *iface,
           LPCOLESTR pszFileName )
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    FIXME("%p %s\n", This, debugstr_w(pszFileName));
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIPersistFile_GetCurFile(IPersistFile *iface,
           LPOLESTR* ppszFileName)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersistFile_vtbl, iface);
    FIXME("%p %p\n",This,ppszFileName);
    return E_NOTIMPL;
}

static ICOM_VTABLE(IPersistFile) IPersistFile_vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    fnIPersistFile_QueryInterface,
    fnIPersistFile_AddRef,
    fnIPersistFile_Release,
    fnIPersistFile_GetClassID,
    fnIPersistFile_IsDirty,
    fnIPersistFile_Load,
    fnIPersistFile_Save,
    fnIPersistFile_SaveCompleted,
    fnIPersistFile_GetCurFile,
};

static ULONG WINAPI fnIOleObject_AddRef(IOleObject *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    return HTMLDocument_AddRef(This);
}

static ULONG WINAPI fnIOleObject_Release(IOleObject *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    return HTMLDocument_Release(This);
}

static HRESULT WINAPI fnIOleObject_QueryInterface(
           IOleObject *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    return HTMLDocument_QueryInterface(This, riid, ppv);
}

static HRESULT WINAPI fnIOleObject_SetClientSite(IOleObject *iface,
           IOleClientSite *pClientSite)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);

    TRACE("%p %p\n",This, pClientSite);

    if( This->site )
        IOleClientSite_Release( This->site );
    if( pClientSite )
        IOleClientSite_AddRef(pClientSite);
    This->site = pClientSite;

    return S_OK;
}

static HRESULT WINAPI fnIOleObject_GetClientSite(IOleObject *iface,
           IOleClientSite **ppClientSite)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);

    TRACE("%p\n",This);

    if( This->site )
        IOleClientSite_AddRef( This->site );
    *ppClientSite = This->site;

    return S_OK;
}

static HRESULT WINAPI fnIOleObject_SetHostNames(IOleObject *iface,
           LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_Close(IOleObject *iface,
           DWORD dwSaveOption)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_SetMoniker(IOleObject *iface,
           DWORD dwWhichMoniker, IMoniker *pmk)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_GetMoniker(IOleObject *iface,
           DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_InitFromData(IOleObject *iface,
           IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_GetClipboardData(IOleObject *iface,
           DWORD dwReserved, IDataObject **ppDataObject)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_DoVerb(IOleObject *iface,
           LONG iVerb, struct tagMSG *lpmsg, IOleClientSite *pActiveSite,
           LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);

    TRACE("%p %ld %p %p %ld %p %p\n", This, 
          iVerb, lpmsg, pActiveSite, lindex, hwndParent, lprcPosRect);

    switch( iVerb )
    {
    case OLEIVERB_SHOW:
        TRACE("OLEIVERB_SHOW  r = (%ld,%ld)-(%ld,%ld)\n",
              lprcPosRect->left,  lprcPosRect->top,
              lprcPosRect->right, lprcPosRect->bottom );
        break;
    }

    /*return E_NOTIMPL; */
    return S_OK;
}

static HRESULT WINAPI fnIOleObject_EnumVerbs(IOleObject *iface,
           IEnumOLEVERB **ppEnumOleVerb)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_Update(IOleObject *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_IsUpToDate(IOleObject *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_GetUserClassID(IOleObject *iface,
           CLSID *pClsid)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_GetUserType(IOleObject *iface,
           DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_SetExtent(IOleObject *iface,
           DWORD dwDrawAspect, SIZEL *psizel)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_GetExtent(IOleObject *iface,
           DWORD dwDrawAspect, SIZEL *psizel)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_Advise(IOleObject *iface,
           IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_Unadvise(IOleObject *iface,
           DWORD dwConnection)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_EnumAdvise(IOleObject *iface,
           IEnumSTATDATA **ppenumAdvise)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_GetMiscStatus(IOleObject *iface,
           DWORD dwAspect, DWORD *pdwStatus)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleObject_SetColorScheme(IOleObject *iface,
           struct tagLOGPALETTE *pLogpal)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleObject_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static ICOM_VTABLE(IOleObject) IOleObject_vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    fnIOleObject_QueryInterface,
    fnIOleObject_AddRef,
    fnIOleObject_Release,
    fnIOleObject_SetClientSite,
    fnIOleObject_GetClientSite,
    fnIOleObject_SetHostNames,
    fnIOleObject_Close,
    fnIOleObject_SetMoniker,
    fnIOleObject_GetMoniker,
    fnIOleObject_InitFromData,
    fnIOleObject_GetClipboardData,
    fnIOleObject_DoVerb,
    fnIOleObject_EnumVerbs,
    fnIOleObject_Update,
    fnIOleObject_IsUpToDate,
    fnIOleObject_GetUserClassID,
    fnIOleObject_GetUserType,
    fnIOleObject_SetExtent,
    fnIOleObject_GetExtent,
    fnIOleObject_Advise,
    fnIOleObject_Unadvise,
    fnIOleObject_EnumAdvise,
    fnIOleObject_GetMiscStatus,
    fnIOleObject_SetColorScheme,
};

static ULONG WINAPI fnIPersist_AddRef(IPersist *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersist_vtbl, iface);
    return HTMLDocument_AddRef(This);
}

static ULONG WINAPI fnIPersist_Release(IPersist *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersist_vtbl, iface);
    return HTMLDocument_Release(This);
}

static HRESULT WINAPI fnIPersist_QueryInterface(
           IPersist *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersist_vtbl, iface);
    return HTMLDocument_QueryInterface(This, riid, ppv);
}

static HRESULT WINAPI fnIPersist_GetClassID(
           IPersist *iface, CLSID * pClassID)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IPersist_vtbl, iface);
    TRACE("%p %p\n", This, pClassID );
    memcpy( pClassID, &CLSID_HTMLDocument, sizeof CLSID_HTMLDocument);
    return S_OK;
}

static ICOM_VTABLE(IPersist) IPersist_vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    fnIPersist_QueryInterface,
    fnIPersist_AddRef,
    fnIPersist_Release,
    fnIPersist_GetClassID,
};

static ULONG WINAPI fnIViewObject_AddRef(IViewObject *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    return HTMLDocument_AddRef(This);
}

static ULONG WINAPI fnIViewObject_Release(IViewObject *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    return HTMLDocument_Release(This);
}

static HRESULT WINAPI fnIViewObject_QueryInterface(
           IViewObject *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    return HTMLDocument_QueryInterface(This, riid, ppv);
}

static HRESULT WINAPI fnIViewObject_Draw( IViewObject *iface,
           DWORD dwDrawAspect, LONG lindex, void *pvAspect,
           DVTARGETDEVICE *ptd, HDC hdcTargetDev, HDC hdcDraw,
           LPCRECTL lprcBounds, LPCRECTL lprcWBounds, 
           BOOL (STDMETHODCALLTYPE *pfnContinue)(ULONG_PTR dwContinue),
           DWORD dwContinue)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIViewObject_GetColorSet( IViewObject *iface,
            DWORD dwDrawAspect, LONG lindex, void *pvAspect,
            DVTARGETDEVICE *ptd, HDC hicTargetDevice,
            struct tagLOGPALETTE **ppColorSet)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIViewObject_Freeze( IViewObject *iface,
           DWORD dwDrawAspect, LONG lindex, void *pvAspect, DWORD *pdwFreeze)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIViewObject_Unfreeze( IViewObject *iface,
           DWORD dwFreeze)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIViewObject_SetAdvise( IViewObject *iface,
           DWORD aspects, DWORD advf, IAdviseSink *pAdvSink)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIViewObject_GetAdvise( IViewObject *iface,
           DWORD *pAspects, DWORD *pAdvf, IAdviseSink **ppAdvSink)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IViewObject_vtbl, iface);
    FIXME("%p\n",This);
    return E_NOTIMPL;
}


static ICOM_VTABLE(IViewObject) IViewObject_vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    fnIViewObject_QueryInterface,
    fnIViewObject_AddRef,
    fnIViewObject_Release,
    fnIViewObject_Draw,
    fnIViewObject_GetColorSet,
    fnIViewObject_Freeze,
    fnIViewObject_Unfreeze,
    fnIViewObject_SetAdvise,
    fnIViewObject_GetAdvise,
};

static ULONG WINAPI fnIOleDocument_AddRef(IOleDocument *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleDocument_vtbl, iface);
    return HTMLDocument_AddRef(This);
}

static ULONG WINAPI fnIOleDocument_Release(IOleDocument *iface)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleDocument_vtbl, iface);
    return HTMLDocument_Release(This);
}

static HRESULT WINAPI fnIOleDocument_QueryInterface(
           IOleDocument *iface, REFIID riid, LPVOID *ppv)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleDocument_vtbl, iface);
    return HTMLDocument_QueryInterface(This, riid, ppv);
}

static HRESULT WINAPI fnIOleDocument_CreateView(IOleDocument *iface,
           IOleInPlaceSite *pIPSite, IStream *pstm, DWORD dwReserved, 
           IOleDocumentView **ppView)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleDocument_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleDocument_GetDocMiscStatus(IOleDocument *iface,
           DWORD *pdwStatus)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleDocument_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static HRESULT WINAPI fnIOleDocument_EnumViews(IOleDocument *iface,
           IEnumOleDocumentViews **ppEnum, IOleDocumentView **ppView)
{
    ICOM_THIS_MULTI(HTMLDocument_impl, IOleDocument_vtbl, iface);
    TRACE("%p\n",This);
    return E_NOTIMPL;
}

static ICOM_VTABLE(IOleDocument) IOleDocument_vtbl =
{
    ICOM_MSVTABLE_COMPAT_DummyRTTIVALUE
    fnIOleDocument_QueryInterface,
    fnIOleDocument_AddRef,
    fnIOleDocument_Release,
    fnIOleDocument_CreateView,
    fnIOleDocument_GetDocMiscStatus,
    fnIOleDocument_EnumViews,
};

HRESULT HTMLDocument_create(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    HTMLDocument_impl *This;

    TRACE("%p %p\n",pUnkOuter,ppObj);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof (HTMLDocument_impl));
    This->IPersistMoniker_vtbl = &IPersistMoniker_vtbl;
    This->IPersistFile_vtbl = &IPersistFile_vtbl;
    This->IOleObject_vtbl = &IOleObject_vtbl;
    This->IPersist_vtbl = &IPersist_vtbl;
    This->IViewObject_vtbl = &IViewObject_vtbl;
    This->IOleDocument_vtbl = &IOleDocument_vtbl;
    This->ref_count = 1;
    This->site = NULL;

    *ppObj = (LPVOID) This;

    TRACE("(%p) <- %p\n", ppObj, This);
    
    return S_OK;
}
