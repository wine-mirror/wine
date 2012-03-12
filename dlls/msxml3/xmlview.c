/*
 * Copyright 2012 Piotr Caban for CodeWeavers
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#define COBJMACROS
#include "config.h"

#include <stdarg.h>
#ifdef HAVE_LIBXML2
#include <libxml/parser.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "msxml6.h"
#include "mshtml.h"
#include "perhist.h"
#include "docobj.h"

#include "wine/debug.h"

#include "msxml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(msxml);

typedef struct
{
    IPersistMoniker IPersistMoniker_iface;
    IPersistHistory IPersistHistory_iface;
    IOleCommandTarget IOleCommandTarget_iface;
    IOleObject IOleObject_iface;

    LONG ref;

    IUnknown *html_doc;
} XMLView;

static inline XMLView* impl_from_IPersistMoniker(IPersistMoniker *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_PersistMoniker_QueryInterface(
        IPersistMoniker *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IPersistMoniker(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppvObject);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IPersistMoniker))
        *ppvObject = &This->IPersistMoniker_iface;
    else if(IsEqualGUID(riid, &IID_IPersistHistory) ||
            IsEqualGUID(riid, &IID_IPersist))
        *ppvObject = &This->IPersistHistory_iface;
    else if(IsEqualGUID(riid, &IID_IOleCommandTarget))
        *ppvObject = &This->IOleCommandTarget_iface;
    else if(IsEqualGUID(riid, &IID_IOleObject))
        *ppvObject = &This->IOleObject_iface;
    else
        return IUnknown_QueryInterface(This->html_doc, riid, ppvObject);

    IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
    return S_OK;
}

static ULONG WINAPI XMLView_PersistMoniker_AddRef(IPersistMoniker *iface)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    return ref;
}

static ULONG WINAPI XMLView_PersistMoniker_Release(IPersistMoniker *iface)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if(!ref) {
        IUnknown_Release(This->html_doc);
        heap_free(This);
    }
    return ref;
}

static HRESULT WINAPI XMLView_PersistMoniker_GetClassID(
        IPersistMoniker *iface, CLSID *pClassID)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_IsDirty(IPersistMoniker *iface)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_Load(IPersistMoniker *iface,
        BOOL fFullyAvailable, IMoniker *pimkName, LPBC pibc, DWORD grfMode)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%x %p %p %x)\n", This, fFullyAvailable, pimkName, pibc, grfMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_Save(IPersistMoniker *iface,
        IMoniker *pimkName, LPBC pbc, BOOL fRemember)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pimkName, pbc, fRemember);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_SaveCompleted(
        IPersistMoniker *iface, IMoniker *pimkName, LPBC pibc)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p %p)\n", This, pimkName, pibc);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistMoniker_GetCurMoniker(
        IPersistMoniker *iface, IMoniker **ppimkName)
{
    XMLView *This = impl_from_IPersistMoniker(iface);
    FIXME("(%p)->(%p)\n", This, ppimkName);
    return E_NOTIMPL;
}

static IPersistMonikerVtbl XMLView_PersistMonikerVtbl = {
    XMLView_PersistMoniker_QueryInterface,
    XMLView_PersistMoniker_AddRef,
    XMLView_PersistMoniker_Release,
    XMLView_PersistMoniker_GetClassID,
    XMLView_PersistMoniker_IsDirty,
    XMLView_PersistMoniker_Load,
    XMLView_PersistMoniker_Save,
    XMLView_PersistMoniker_SaveCompleted,
    XMLView_PersistMoniker_GetCurMoniker
};

static inline XMLView* impl_from_IPersistHistory(IPersistHistory *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IPersistHistory_iface);
}

static HRESULT WINAPI XMLView_PersistHistory_QueryInterface(
        IPersistHistory *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    return IPersistMoniker_QueryInterface(&This->IPersistMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI XMLView_PersistHistory_AddRef(IPersistHistory *iface)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    return IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
}

static ULONG WINAPI XMLView_PersistHistory_Release(IPersistHistory *iface)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    return IPersistMoniker_Release(&This->IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_PersistHistory_GetClassID(
        IPersistHistory *iface, CLSID *pClassID)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pClassID);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_LoadHistory(
        IPersistHistory *iface, IStream *pStream, IBindCtx *pbc)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p %p)\n", This, pStream, pbc);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_SaveHistory(
        IPersistHistory *iface, IStream *pStream)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pStream);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_SetPositionCookie(
        IPersistHistory *iface, DWORD dwPositioncookie)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%d)\n", This, dwPositioncookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_PersistHistory_GetPositionCookie(
        IPersistHistory *iface, DWORD *pdwPositioncookie)
{
    XMLView *This = impl_from_IPersistHistory(iface);
    FIXME("(%p)->(%p)\n", This, pdwPositioncookie);
    return E_NOTIMPL;
}

static IPersistHistoryVtbl XMLView_PersistHistoryVtbl = {
    XMLView_PersistHistory_QueryInterface,
    XMLView_PersistHistory_AddRef,
    XMLView_PersistHistory_Release,
    XMLView_PersistHistory_GetClassID,
    XMLView_PersistHistory_LoadHistory,
    XMLView_PersistHistory_SaveHistory,
    XMLView_PersistHistory_SetPositionCookie,
    XMLView_PersistHistory_GetPositionCookie
};

static inline XMLView* impl_from_IOleCommandTarget(IOleCommandTarget *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IOleCommandTarget_iface);
}

static HRESULT WINAPI XMLView_OleCommandTarget_QueryInterface(
        IOleCommandTarget *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    return IPersistMoniker_QueryInterface(&This->IPersistMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI XMLView_OleCommandTarget_AddRef(IOleCommandTarget *iface)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    return IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
}

static ULONG WINAPI XMLView_OleCommandTarget_Release(IOleCommandTarget *iface)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    return IPersistMoniker_Release(&This->IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_OleCommandTarget_QueryStatus(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    FIXME("(%p)->(%p %x %p %p)\n", This, pguidCmdGroup, cCmds, prgCmds, pCmdText);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleCommandTarget_Exec(IOleCommandTarget *iface,
        const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
        VARIANT *pvaIn, VARIANT *pvaOut)
{
    XMLView *This = impl_from_IOleCommandTarget(iface);
    FIXME("(%p)->(%p %d %x %p %p)\n", This, pguidCmdGroup,
            nCmdID, nCmdexecopt, pvaIn, pvaOut);
    return E_NOTIMPL;
}

static IOleCommandTargetVtbl XMLView_OleCommandTargetVtbl = {
    XMLView_OleCommandTarget_QueryInterface,
    XMLView_OleCommandTarget_AddRef,
    XMLView_OleCommandTarget_Release,
    XMLView_OleCommandTarget_QueryStatus,
    XMLView_OleCommandTarget_Exec
};

static inline XMLView* impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, XMLView, IOleObject_iface);
}

static HRESULT WINAPI XMLView_OleObject_QueryInterface(
        IOleObject *iface, REFIID riid, void **ppvObject)
{
    XMLView *This = impl_from_IOleObject(iface);
    return IPersistMoniker_QueryInterface(&This->IPersistMoniker_iface, riid, ppvObject);
}

static ULONG WINAPI XMLView_OleObject_AddRef(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    return IPersistMoniker_AddRef(&This->IPersistMoniker_iface);
}

static ULONG WINAPI XMLView_OleObject_Release(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    return IPersistMoniker_Release(&This->IPersistMoniker_iface);
}

static HRESULT WINAPI XMLView_OleObject_SetClientSite(
        IOleObject *iface, IOleClientSite *pClientSite)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pClientSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetClientSite(
        IOleObject *iface, IOleClientSite **ppClientSite)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppClientSite);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetHostNames(IOleObject *iface,
        LPCOLESTR szContainerApp, LPCOLESTR szContainerObj)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_w(szContainerApp), debugstr_w(szContainerObj));
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Close(IOleObject *iface, DWORD dwSaveOption)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x)\n", This, dwSaveOption);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetMoniker(IOleObject *iface,
        DWORD dwWhichMoniker, IMoniker *pmk)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwWhichMoniker, pmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetMoniker(IOleObject *iface,
        DWORD dwAssign, DWORD dwWhichMoniker, IMoniker **ppmk)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %x %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_InitFromData(IOleObject *iface,
        IDataObject *pDataObject, BOOL fCreation, DWORD dwReserved)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %x %x)\n", This, pDataObject, fCreation, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetClipboardData(IOleObject *iface,
        DWORD dwReserved, IDataObject **ppDataObject)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwReserved, ppDataObject);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_DoVerb(IOleObject *iface,
        LONG iVerb, LPMSG lpmsg, IOleClientSite *pActiveSite,
        LONG lindex, HWND hwndParent, LPCRECT lprcPosRect)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p %p %d %p %p)\n", This, iVerb, lpmsg,
            pActiveSite, lindex, hwndParent, lprcPosRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_EnumVerbs(
        IOleObject *iface, IEnumOLEVERB **ppEnumOleVerb)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->{%p)\n", This, ppEnumOleVerb);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Update(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_IsUpToDate(IOleObject *iface)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetUserClassID(IOleObject *iface, CLSID *pClsid)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pClsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetUserType(IOleObject *iface,
        DWORD dwFormOfType, LPOLESTR *pszUserType)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwFormOfType, pszUserType);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetExtent(IOleObject *iface,
        DWORD dwDrawAspect, SIZEL *psizel)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetExtent(IOleObject *iface,
        DWORD dwDrawAspect, SIZEL *psizel)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%x %p)\n", This, dwDrawAspect, psizel);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Advise(IOleObject *iface,
        IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p %p)\n", This, pAdvSink, pdwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_Unadvise(
        IOleObject *iface, DWORD dwConnection)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d)\n", This, dwConnection);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_EnumAdvise(
        IOleObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, ppenumAdvise);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_GetMiscStatus(
        IOleObject *iface, DWORD dwAspect, DWORD *pdwStatus)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%d %p)\n", This, dwAspect, pdwStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI XMLView_OleObject_SetColorScheme(
        IOleObject *iface, LOGPALETTE *pLogpal)
{
    XMLView *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p)\n", This, pLogpal);
    return E_NOTIMPL;
}

static IOleObjectVtbl XMLView_OleObjectVtbl = {
    XMLView_OleObject_QueryInterface,
    XMLView_OleObject_AddRef,
    XMLView_OleObject_Release,
    XMLView_OleObject_SetClientSite,
    XMLView_OleObject_GetClientSite,
    XMLView_OleObject_SetHostNames,
    XMLView_OleObject_Close,
    XMLView_OleObject_SetMoniker,
    XMLView_OleObject_GetMoniker,
    XMLView_OleObject_InitFromData,
    XMLView_OleObject_GetClipboardData,
    XMLView_OleObject_DoVerb,
    XMLView_OleObject_EnumVerbs,
    XMLView_OleObject_Update,
    XMLView_OleObject_IsUpToDate,
    XMLView_OleObject_GetUserClassID,
    XMLView_OleObject_GetUserType,
    XMLView_OleObject_SetExtent,
    XMLView_OleObject_GetExtent,
    XMLView_OleObject_Advise,
    XMLView_OleObject_Unadvise,
    XMLView_OleObject_EnumAdvise,
    XMLView_OleObject_GetMiscStatus,
    XMLView_OleObject_SetColorScheme
};

HRESULT XMLView_create(IUnknown *outer, void **ppObj)
{
    XMLView *This;
    HRESULT hres;

    TRACE("(%p %p)\n", outer, ppObj);

    if(outer)
        return E_FAIL;

    This = heap_alloc(sizeof(*This));
    if(!This)
        return E_OUTOFMEMORY;

    This->IPersistMoniker_iface.lpVtbl = &XMLView_PersistMonikerVtbl;
    This->IPersistHistory_iface.lpVtbl = &XMLView_PersistHistoryVtbl;
    This->IOleCommandTarget_iface.lpVtbl = &XMLView_OleCommandTargetVtbl;
    This->IOleObject_iface.lpVtbl = &XMLView_OleObjectVtbl;
    This->ref = 1;

    hres = CoCreateInstance(&CLSID_HTMLDocument, (IUnknown*)&This->IPersistMoniker_iface,
            CLSCTX_INPROC_SERVER, &IID_IUnknown, (void**)&This->html_doc);
    if(FAILED(hres)) {
        heap_free(This);
        return hres;
    }

    *ppObj = &This->IPersistMoniker_iface;
    return S_OK;
}
