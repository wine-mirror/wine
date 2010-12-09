/*
 * Copyright 2010 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "shlobj.h"

#include "mshtml_private.h"
#include "pluginhost.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

const IID IID_HTMLPluginContainer =
    {0xbd7a6050,0xb373,0x4f6f,{0xa4,0x93,0xdd,0x40,0xc5,0x23,0xa8,0x6a}};

static void activate_plugin(PluginHost *host)
{
    IClientSecurity *client_security;
    IQuickActivate *quick_activate;
    HRESULT hres;

    if(!host->plugin_unk)
        return;

    /* Note native calls QI on plugin for an undocumented IID and CLSID_HTMLDocument */

    /* FIXME: call FreezeEvents(TRUE) */

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IClientSecurity, (void**)&client_security);
    if(SUCCEEDED(hres)) {
        FIXME("Handle IClientSecurity\n");
        IClientSecurity_Release(client_security);
        return;
    }

    hres = IUnknown_QueryInterface(host->plugin_unk, &IID_IQuickActivate, (void**)&quick_activate);
    if(SUCCEEDED(hres)) {
        QACONTAINER container = {sizeof(container)};
        QACONTROL control = {sizeof(control)};

        container.pClientSite = &host->IOleClientSite_iface;
        container.dwAmbientFlags = QACONTAINER_SUPPORTSMNEMONICS|QACONTAINER_MESSAGEREFLECT|QACONTAINER_USERMODE;
        container.pAdviseSink = &host->IAdviseSinkEx_iface;
        container.pPropertyNotifySink = &host->IPropertyNotifySink_iface;

        hres = IQuickActivate_QuickActivate(quick_activate, &container, &control);
        if(FAILED(hres))
            FIXME("QuickActivate failed: %08x\n", hres);
    }else {
        FIXME("No IQuickActivate\n");
    }
}

void update_plugin_window(PluginHost *host, HWND hwnd, const RECT *rect)
{
    if(!hwnd || (host->hwnd && host->hwnd != hwnd)) {
        FIXME("unhandled hwnd\n");
        return;
    }

    if(!host->hwnd) {
        host->hwnd = hwnd;
        activate_plugin(host);
    }
}

static inline PluginHost *impl_from_IOleClientSite(IOleClientSite *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IOleClientSite_iface);
}

static HRESULT WINAPI PHClientSite_QueryInterface(IOleClientSite *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IOleClientSite(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IOleClientSite_iface;
    }else if(IsEqualGUID(&IID_IOleClientSite, riid)) {
        TRACE("(%p)->(IID_IOleClientSite %p)\n", This, ppv);
        *ppv = &This->IOleClientSite_iface;
    }else if(IsEqualGUID(&IID_IAdviseSink, riid)) {
        TRACE("(%p)->(IID_IAdviseSink %p)\n", This, ppv);
        *ppv = &This->IAdviseSinkEx_iface;
    }else if(IsEqualGUID(&IID_IAdviseSinkEx, riid)) {
        TRACE("(%p)->(IID_IAdviseSinkEx %p)\n", This, ppv);
        *ppv = &This->IAdviseSinkEx_iface;
    }else if(IsEqualGUID(&IID_IPropertyNotifySink, riid)) {
        TRACE("(%p)->(IID_IPropertyNotifySink %p)\n", This, ppv);
        *ppv = &This->IPropertyNotifySink_iface;
    }else if(IsEqualGUID(&IID_IDispatch, riid)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IDispatch_iface;
    }else if(IsEqualGUID(&IID_IOleWindow, riid)) {
        TRACE("(%p)->(IID_IOleWindow %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceSiteEx_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceSite, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceSite %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceSiteEx_iface;
    }else if(IsEqualGUID(&IID_IOleInPlaceSiteEx, riid)) {
        TRACE("(%p)->(IID_IOleInPlaceSiteEx %p)\n", This, ppv);
        *ppv = &This->IOleInPlaceSiteEx_iface;
    }else if(IsEqualGUID(&IID_IOleControlSite, riid)) {
        TRACE("(%p)->(IID_IOleControlSite %p)\n", This, ppv);
        *ppv = &This->IOleControlSite_iface;
    }else if(IsEqualGUID(&IID_IBindHost, riid)) {
        TRACE("(%p)->(IID_IBindHost %p)\n", This, ppv);
        *ppv = &This->IBindHost_iface;
    }else if(IsEqualGUID(&IID_IServiceProvider, riid)) {
        TRACE("(%p)->(IID_IServiceProvider %p)\n", This, ppv);
        *ppv = &This->IServiceProvider_iface;
    }else {
        WARN("Unsupported interface %s\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IOleClientSite_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI PHClientSite_AddRef(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI PHClientSite_Release(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        list_remove(&This->entry);
        if(This->plugin_unk)
            IUnknown_Release(This->plugin_unk);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI PHClientSite_SaveObject(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_GetMoniker(IOleClientSite *iface, DWORD dwAssign,
                                            DWORD dwWhichMoniker, IMoniker **ppmk)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)->(%d %d %p)\n", This, dwAssign, dwWhichMoniker, ppmk);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_GetContainer(IOleClientSite *iface, IOleContainer **ppContainer)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)->(%p)\n", This, ppContainer);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_ShowObject(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_OnShowWindow(IOleClientSite *iface, BOOL fShow)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)->(%x)\n", This, fShow);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHClientSite_RequestNewObjectLayout(IOleClientSite *iface)
{
    PluginHost *This = impl_from_IOleClientSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleClientSiteVtbl OleClientSiteVtbl = {
    PHClientSite_QueryInterface,
    PHClientSite_AddRef,
    PHClientSite_Release,
    PHClientSite_SaveObject,
    PHClientSite_GetMoniker,
    PHClientSite_GetContainer,
    PHClientSite_ShowObject,
    PHClientSite_OnShowWindow,
    PHClientSite_RequestNewObjectLayout
};

static inline PluginHost *impl_from_IAdviseSinkEx(IAdviseSinkEx *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IAdviseSinkEx_iface);
}

static HRESULT WINAPI PHAdviseSinkEx_QueryInterface(IAdviseSinkEx *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHAdviseSinkEx_AddRef(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHAdviseSinkEx_Release(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static void WINAPI PHAdviseSinkEx_OnDataChange(IAdviseSinkEx *iface, FORMATETC *pFormatetc, STGMEDIUM *pStgMedium)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%p %p)\n", This, pFormatetc, pStgMedium);
}

static void WINAPI PHAdviseSinkEx_OnViewChange(IAdviseSinkEx *iface, DWORD dwAspect, LONG lindex)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%d %d)\n", This, dwAspect, lindex);
}

static void WINAPI PHAdviseSinkEx_OnRename(IAdviseSinkEx *iface, IMoniker *pmk)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%p)\n", This, pmk);
}

static void WINAPI PHAdviseSinkEx_OnSave(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)\n", This);
}

static void WINAPI PHAdviseSinkEx_OnClose(IAdviseSinkEx *iface)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)\n", This);
}

static void WINAPI PHAdviseSinkEx_OnViewStatusChange(IAdviseSinkEx *iface, DWORD dwViewStatus)
{
    PluginHost *This = impl_from_IAdviseSinkEx(iface);
    FIXME("(%p)->(%d)\n", This, dwViewStatus);
}

static const IAdviseSinkExVtbl AdviseSinkExVtbl = {
    PHAdviseSinkEx_QueryInterface,
    PHAdviseSinkEx_AddRef,
    PHAdviseSinkEx_Release,
    PHAdviseSinkEx_OnDataChange,
    PHAdviseSinkEx_OnViewChange,
    PHAdviseSinkEx_OnRename,
    PHAdviseSinkEx_OnSave,
    PHAdviseSinkEx_OnClose,
    PHAdviseSinkEx_OnViewStatusChange
};

static inline PluginHost *impl_from_IPropertyNotifySink(IPropertyNotifySink *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IPropertyNotifySink_iface);
}

static HRESULT WINAPI PHPropertyNotifySink_QueryInterface(IPropertyNotifySink *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHPropertyNotifySink_AddRef(IPropertyNotifySink *iface)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHPropertyNotifySink_Release(IPropertyNotifySink *iface)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHPropertyNotifySink_OnChanged(IPropertyNotifySink *iface, DISPID dispID)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    FIXME("(%p)->(%d)\n", This, dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHPropertyNotifySink_OnRequestEdit(IPropertyNotifySink *iface, DISPID dispID)
{
    PluginHost *This = impl_from_IPropertyNotifySink(iface);
    FIXME("(%p)->(%d)\n", This, dispID);
    return E_NOTIMPL;
}

static const IPropertyNotifySinkVtbl PropertyNotifySinkVtbl = {
    PHPropertyNotifySink_QueryInterface,
    PHPropertyNotifySink_AddRef,
    PHPropertyNotifySink_Release,
    PHPropertyNotifySink_OnChanged,
    PHPropertyNotifySink_OnRequestEdit
};

static inline PluginHost *impl_from_IDispatch(IDispatch *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IDispatch_iface);
}

static HRESULT WINAPI PHDispatch_QueryInterface(IDispatch *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IDispatch(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHDispatch_AddRef(IDispatch *iface)
{
    PluginHost *This = impl_from_IDispatch(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHDispatch_Release(IDispatch *iface)
{
    PluginHost *This = impl_from_IDispatch(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHDispatch_GetTypeInfoCount(IDispatch *iface, UINT *pctinfo)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%p)\n", This, pctinfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHDispatch_GetTypeInfo(IDispatch *iface, UINT iTInfo,
        LCID lcid, ITypeInfo **ppTInfo)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%d %d %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHDispatch_GetIDsOfNames(IDispatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%s %p %d %d %p)\n", This, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHDispatch_Invoke(IDispatch *iface, DISPID dispid,  REFIID riid, LCID lcid,
        WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    PluginHost *This = impl_from_IDispatch(iface);
    FIXME("(%p)->(%d %x %p %p)\n", This, dispid, wFlags, pDispParams, pVarResult);
    return E_NOTIMPL;
}

static const IDispatchVtbl DispatchVtbl = {
    PHDispatch_QueryInterface,
    PHDispatch_AddRef,
    PHDispatch_Release,
    PHDispatch_GetTypeInfoCount,
    PHDispatch_GetTypeInfo,
    PHDispatch_GetIDsOfNames,
    PHDispatch_Invoke
};

static inline PluginHost *impl_from_IOleInPlaceSiteEx(IOleInPlaceSiteEx *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IOleInPlaceSiteEx_iface);
}

static HRESULT WINAPI PHInPlaceSite_QueryInterface(IOleInPlaceSiteEx *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHInPlaceSite_AddRef(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHInPlaceSite_Release(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHInPlaceSite_GetWindow(IOleInPlaceSiteEx *iface, HWND *phwnd)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_ContextSensitiveHelp(IOleInPlaceSiteEx *iface, BOOL fEnterMode)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_CanInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnInPlaceActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnUIActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_GetWindowContext(IOleInPlaceSiteEx *iface,
        IOleInPlaceFrame **ppFrame, IOleInPlaceUIWindow **ppDoc, RECT *lprcPosRect,
        RECT *lprcClipRect, OLEINPLACEFRAMEINFO *frame_info)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%p %p %p %p %p)\n", This, ppFrame, ppDoc, lprcPosRect, lprcClipRect, frame_info);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_Scroll(IOleInPlaceSiteEx *iface, SIZE scrollExtent)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->({%d %d})\n", This, scrollExtent.cx, scrollExtent.cy);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnUIDeactivate(IOleInPlaceSiteEx *iface, BOOL fUndoable)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%x)\n", This, fUndoable);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnInPlaceDeactivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_DiscardUndoState(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_DeactivateAndUndo(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSite_OnPosRectChange(IOleInPlaceSiteEx *iface, LPCRECT lprcPosRect)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%p)\n", This, lprcPosRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSiteEx_OnInPlaceActivateEx(IOleInPlaceSiteEx *iface, BOOL *pfNoRedraw, DWORD dwFlags)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%p %x)\n", This, pfNoRedraw, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSiteEx_OnInPlaceDeactivateEx(IOleInPlaceSiteEx *iface, BOOL fNoRedraw)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)->(%x)\n", This, fNoRedraw);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHInPlaceSiteEx_RequestUIActivate(IOleInPlaceSiteEx *iface)
{
    PluginHost *This = impl_from_IOleInPlaceSiteEx(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleInPlaceSiteExVtbl OleInPlaceSiteExVtbl = {
    PHInPlaceSite_QueryInterface,
    PHInPlaceSite_AddRef,
    PHInPlaceSite_Release,
    PHInPlaceSite_GetWindow,
    PHInPlaceSite_ContextSensitiveHelp,
    PHInPlaceSite_CanInPlaceActivate,
    PHInPlaceSite_OnInPlaceActivate,
    PHInPlaceSite_OnUIActivate,
    PHInPlaceSite_GetWindowContext,
    PHInPlaceSite_Scroll,
    PHInPlaceSite_OnUIDeactivate,
    PHInPlaceSite_OnInPlaceDeactivate,
    PHInPlaceSite_DiscardUndoState,
    PHInPlaceSite_DeactivateAndUndo,
    PHInPlaceSite_OnPosRectChange,
    PHInPlaceSiteEx_OnInPlaceActivateEx,
    PHInPlaceSiteEx_OnInPlaceDeactivateEx,
    PHInPlaceSiteEx_RequestUIActivate
};

static inline PluginHost *impl_from_IOleControlSite(IOleControlSite *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IOleControlSite_iface);
}

static HRESULT WINAPI PHControlSite_QueryInterface(IOleControlSite *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHControlSite_AddRef(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHControlSite_Release(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHControlSite_OnControlInfoChanged(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_LockInPlaceActive(IOleControlSite *iface, BOOL fLock)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%x)\n", This, fLock);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_GetExtendedControl(IOleControlSite *iface, IDispatch **ppDisp)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%p)\n", This, ppDisp);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_TransformCoords(IOleControlSite *iface, POINTL *pPtlHimetric, POINTF *pPtfContainer, DWORD dwFlags)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%p %p %x)\n", This, pPtlHimetric, pPtfContainer, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_TranslateAccelerator(IOleControlSite *iface, MSG *pMsg, DWORD grfModifiers)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%x)\n", This, grfModifiers);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_OnFocus(IOleControlSite *iface, BOOL fGotFocus)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)->(%x)\n", This, fGotFocus);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHControlSite_ShowPropertyFrame(IOleControlSite *iface)
{
    PluginHost *This = impl_from_IOleControlSite(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static const IOleControlSiteVtbl OleControlSiteVtbl = {
    PHControlSite_QueryInterface,
    PHControlSite_AddRef,
    PHControlSite_Release,
    PHControlSite_OnControlInfoChanged,
    PHControlSite_LockInPlaceActive,
    PHControlSite_GetExtendedControl,
    PHControlSite_TransformCoords,
    PHControlSite_TranslateAccelerator,
    PHControlSite_OnFocus,
    PHControlSite_ShowPropertyFrame
};

static inline PluginHost *impl_from_IBindHost(IBindHost *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IBindHost_iface);
}

static HRESULT WINAPI PHBindHost_QueryInterface(IBindHost *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IBindHost(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHBindHost_AddRef(IBindHost *iface)
{
    PluginHost *This = impl_from_IBindHost(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHBindHost_Release(IBindHost *iface)
{
    PluginHost *This = impl_from_IBindHost(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHBindHost_CreateMoniker(IBindHost *iface, LPOLESTR szName, IBindCtx *pBC, IMoniker **ppmk, DWORD dwReserved)
{
    PluginHost *This = impl_from_IBindHost(iface);
    FIXME("(%p)->(%s %p %p %x)\n", This, debugstr_w(szName), pBC, ppmk, dwReserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHBindHost_MonikerBindToStorage(IBindHost *iface, IMoniker *pMk, IBindCtx *pBC,
        IBindStatusCallback *pBSC, REFIID riid, void **ppvObj)
{
    PluginHost *This = impl_from_IBindHost(iface);
    FIXME("(%p)->(%p %p %p %s %p)\n", This, pMk, pBC, pBSC, debugstr_guid(riid), ppvObj);
    return E_NOTIMPL;
}

static HRESULT WINAPI PHBindHost_MonikerBindToObject(IBindHost *iface, IMoniker *pMk, IBindCtx *pBC,
        IBindStatusCallback *pBSC, REFIID riid, void **ppvObj)
{
    PluginHost *This = impl_from_IBindHost(iface);
    FIXME("(%p)->(%p %p %p %s %p)\n", This, pMk, pBC, pBSC, debugstr_guid(riid), ppvObj);
    return E_NOTIMPL;
}

static const IBindHostVtbl BindHostVtbl = {
    PHBindHost_QueryInterface,
    PHBindHost_AddRef,
    PHBindHost_Release,
    PHBindHost_CreateMoniker,
    PHBindHost_MonikerBindToStorage,
    PHBindHost_MonikerBindToObject
};

static inline PluginHost *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, PluginHost, IServiceProvider_iface);
}

static HRESULT WINAPI PHServiceProvider_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    return IOleClientSite_QueryInterface(&This->IOleClientSite_iface, riid, ppv);
}

static ULONG WINAPI PHServiceProvider_AddRef(IServiceProvider *iface)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    return IOleClientSite_AddRef(&This->IOleClientSite_iface);
}

static ULONG WINAPI PHServiceProvider_Release(IServiceProvider *iface)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    return IOleClientSite_Release(&This->IOleClientSite_iface);
}

static HRESULT WINAPI PHServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService, REFIID riid, void **ppv)
{
    PluginHost *This = impl_from_IServiceProvider(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    PHServiceProvider_QueryInterface,
    PHServiceProvider_AddRef,
    PHServiceProvider_Release,
    PHServiceProvider_QueryService
};

void detach_plugin_hosts(HTMLDocumentNode *doc)
{
    PluginHost *iter;

    while(!list_empty(&doc->plugin_hosts)) {
        iter = LIST_ENTRY(list_head(&doc->plugin_hosts), PluginHost, entry);
        list_remove(&iter->entry);
        iter->doc = NULL;
    }
}

HRESULT create_plugin_host(HTMLDocumentNode *doc, IUnknown *unk, PluginHost **ret)
{
    PluginHost *host;

    host = heap_alloc_zero(sizeof(*host));
    if(!host)
        return E_OUTOFMEMORY;

    host->IOleClientSite_iface.lpVtbl      = &OleClientSiteVtbl;
    host->IAdviseSinkEx_iface.lpVtbl       = &AdviseSinkExVtbl;
    host->IPropertyNotifySink_iface.lpVtbl = &PropertyNotifySinkVtbl;
    host->IDispatch_iface.lpVtbl           = &DispatchVtbl;
    host->IOleInPlaceSiteEx_iface.lpVtbl   = &OleInPlaceSiteExVtbl;
    host->IOleControlSite_iface.lpVtbl     = &OleControlSiteVtbl;
    host->IBindHost_iface.lpVtbl           = &BindHostVtbl;
    host->IServiceProvider_iface.lpVtbl    = &ServiceProviderVtbl;

    host->ref = 1;

    IUnknown_AddRef(unk);
    host->plugin_unk = unk;

    host->doc = doc;
    list_add_tail(&doc->plugin_hosts, &host->entry);

    *ret = host;
    return S_OK;
}
