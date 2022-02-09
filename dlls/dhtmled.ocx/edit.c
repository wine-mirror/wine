/*
 * Copyright 2017 Alex Henrie
 * Copyright 2021 Vijay Kiran Kamuju
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

#include "dhtmled.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dhtmled);

typedef enum tid_t {
    NULL_tid,
    IDHTMLEdit_tid,
    LAST_tid
} tid_t;

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_NULL,
    &IID_IDHTMLEdit
};

static HRESULT load_typelib(void)
{
    ITypeLib *tl;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_DHTMLEDLib, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if (FAILED(hr)) {
        ERR("LoadRegTypeLib failed: %08lx\n", hr);
        return hr;
    }

    if (InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hr;
}

void release_typelib(void)
{
    unsigned i;

    if (!typelib)
        return;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hr;

    if (!typelib)
        hr = load_typelib();
    if (!typelib)
        return hr;

    if (!typeinfos[tid])
    {
        ITypeInfo *ti;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if (FAILED(hr))
        {
            ERR("GetTypeInfoOfGuid(%s) failed: %08lx\n", debugstr_guid(tid_ids[tid]), hr);
            return hr;
        }

        if (InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

typedef struct DHTMLEditImpl DHTMLEditImpl;
typedef struct ConnectionPoint ConnectionPoint;

struct ConnectionPoint
{
    IConnectionPoint IConnectionPoint_iface;
    DHTMLEditImpl *dhed;
    const IID *riid;
};

struct DHTMLEditImpl
{
    IDHTMLEdit IDHTMLEdit_iface;
    IOleObject IOleObject_iface;
    IProvideClassInfo2 IProvideClassInfo2_iface;
    IPersistStorage IPersistStorage_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    IOleControl IOleControl_iface;
    IViewObjectEx IViewObjectEx_iface;
    IOleInPlaceObjectWindowless IOleInPlaceObjectWindowless_iface;
    IOleInPlaceActiveObject IOleInPlaceActiveObject_iface;
    IConnectionPointContainer IConnectionPointContainer_iface;
    IDataObject IDataObject_iface;
    IServiceProvider IServiceProvider_iface;

    IOleClientSite *client_site;
    ConnectionPoint conpt;
    SIZEL extent;
    LONG ref;
};

static inline DHTMLEditImpl *impl_from_IDHTMLEdit(IDHTMLEdit *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IDHTMLEdit_iface);
}

static inline DHTMLEditImpl *impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IOleObject_iface);
}

static inline DHTMLEditImpl *impl_from_IProvideClassInfo2(IProvideClassInfo2 *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IProvideClassInfo2_iface);
}

static inline DHTMLEditImpl *impl_from_IPersistStorage(IPersistStorage *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IPersistStorage_iface);
}

static inline DHTMLEditImpl *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IPersistStreamInit_iface);
}

static inline DHTMLEditImpl *impl_from_IOleControl(IOleControl *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IOleControl_iface);
}

static inline DHTMLEditImpl *impl_from_IViewObjectEx(IViewObjectEx *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IViewObjectEx_iface);
}

static inline DHTMLEditImpl *impl_from_IOleInPlaceObjectWindowless(IOleInPlaceObjectWindowless *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IOleInPlaceObjectWindowless_iface);
}

static inline DHTMLEditImpl *impl_from_IOleInPlaceActiveObject(IOleInPlaceActiveObject *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IOleInPlaceActiveObject_iface);
}

static inline DHTMLEditImpl *impl_from_IConnectionPointContainer(IConnectionPointContainer *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IConnectionPointContainer_iface);
}

static inline DHTMLEditImpl *impl_from_IDataObject(IDataObject *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IDataObject_iface);
}

static inline DHTMLEditImpl *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IServiceProvider_iface);
}

static ULONG dhtml_edit_addref(DHTMLEditImpl *This)
{
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    return ref;
}

static HRESULT dhtml_edit_qi(DHTMLEditImpl *This, REFIID iid, void **out)
{
    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IDispatch) ||
        IsEqualGUID(iid, &IID_IDHTMLEdit))
    {
        dhtml_edit_addref(This);
        *out = &This->IDHTMLEdit_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IOleObject))
    {
        dhtml_edit_addref(This);
        *out = &This->IOleObject_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IProvideClassInfo) ||
             IsEqualGUID(iid, &IID_IProvideClassInfo2))
    {
        dhtml_edit_addref(This);
        *out = &This->IProvideClassInfo2_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IPersistStorage))
    {
        dhtml_edit_addref(This);
        *out = &This->IPersistStorage_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IPersistStreamInit))
    {
        dhtml_edit_addref(This);
        *out = &This->IPersistStreamInit_iface;
        return S_OK;
    }
    else if(IsEqualGUID(iid, &IID_IOleControl))
    {
        dhtml_edit_addref(This);
        *out = &This->IOleControl_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IViewObject) ||
             IsEqualGUID(iid, &IID_IViewObject2) ||
             IsEqualGUID(iid, &IID_IViewObjectEx))
    {
        dhtml_edit_addref(This);
        *out = &This->IViewObjectEx_iface;
        return S_OK;
    }
    else if (IsEqualGUID(iid, &IID_IOleWindow) ||
             IsEqualGUID(iid, &IID_IOleInPlaceObject) ||
             IsEqualGUID(iid, &IID_IOleInPlaceObjectWindowless))
    {
        dhtml_edit_addref(This);
        *out = &This->IOleInPlaceObjectWindowless_iface;
        return S_OK;
    }
    else if(IsEqualGUID(iid, &IID_IOleInPlaceActiveObject))
    {
        dhtml_edit_addref(This);
        *out = &This->IOleInPlaceActiveObject_iface;
        return S_OK;
    }
    else if(IsEqualGUID(iid, &IID_IConnectionPointContainer))
    {
        dhtml_edit_addref(This);
        *out = &This->IConnectionPointContainer_iface;
        return S_OK;
    }
    else if(IsEqualGUID(iid, &IID_IDataObject))
    {
        dhtml_edit_addref(This);
        *out = &This->IDataObject_iface;
        return S_OK;
    }
    else if(IsEqualGUID(iid, &IID_IServiceProvider))
    {
        dhtml_edit_addref(This);
        *out = &This->IServiceProvider_iface;
        return S_OK;
    }


    *out = NULL;
    ERR("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG dhtml_edit_release(DHTMLEditImpl *This)
{
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%ld\n", This, ref);

    if (!ref)
    {
        if (This->client_site)
            IOleClientSite_Release(This->client_site);

        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI DHTMLEdit_QueryInterface(IDHTMLEdit *iface, REFIID iid, void **out)
{
    return dhtml_edit_qi(impl_from_IDHTMLEdit(iface), iid, out);
}

static ULONG WINAPI DHTMLEdit_AddRef(IDHTMLEdit *iface)
{
    return dhtml_edit_addref(impl_from_IDHTMLEdit(iface));
}

static ULONG WINAPI DHTMLEdit_Release(IDHTMLEdit *iface)
{
    return dhtml_edit_release(impl_from_IDHTMLEdit(iface));
}

static HRESULT WINAPI DHTMLEdit_GetTypeInfoCount(IDHTMLEdit *iface, UINT *count)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    TRACE("(%p)->(%p)\n", This, count);
    *count = 1;
    return S_OK;
}

static HRESULT WINAPI DHTMLEdit_GetTypeInfo(IDHTMLEdit *iface, UINT type_index, LCID lcid, ITypeInfo **type_info)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    HRESULT hr;

    TRACE("(%p)->(%u, %08lx, %p)\n", This, type_index, lcid, type_info);

    hr = get_typeinfo(IDHTMLEdit_tid, type_info);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*type_info);
    return hr;
}

static HRESULT WINAPI DHTMLEdit_GetIDsOfNames(IDHTMLEdit *iface, REFIID iid, OLECHAR **names, UINT name_count,
                                              LCID lcid, DISPID *disp_ids)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%s, %p, %u, %08lx, %p)\n", This, debugstr_guid(iid), names, name_count, lcid, disp_ids);

    hr = get_typeinfo(IDHTMLEdit_tid, &ti);
    if (FAILED(hr))
        return hr;

    return ITypeInfo_GetIDsOfNames(ti, names, name_count, disp_ids);
}

static HRESULT WINAPI DHTMLEdit_Invoke(IDHTMLEdit *iface, DISPID member, REFIID iid, LCID lcid, WORD flags,
                                       DISPPARAMS *params, VARIANT *ret, EXCEPINFO *exception_info, UINT *error_index)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p)->(%ld, %s, %08lx, 0x%x, %p, %p, %p, %p)\n",
          This, member, debugstr_guid(iid), lcid, flags, params, ret, exception_info, error_index);

    hr = get_typeinfo(IDHTMLEdit_tid, &ti);
    if (FAILED(hr))
        return hr;

    return ITypeInfo_Invoke(ti, iface, member, flags, params, ret, exception_info, error_index);
}

static HRESULT WINAPI DHTMLEdit_ExecCommand(IDHTMLEdit *iface, DHTMLEDITCMDID cmd_id, OLECMDEXECOPT options,
                                            VARIANT *code_in, VARIANT *code_out)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%u, %u, %s, %p) stub\n", This, cmd_id, options, debugstr_variant(code_in), code_out);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_QueryStatus(IDHTMLEdit *iface, DHTMLEDITCMDID cmd_id, DHTMLEDITCMDF *status)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%u, %p) stub\n", This, cmd_id, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_SetContextMenu(IDHTMLEdit *iface, VARIANT *strings, VARIANT *states)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p, %p) stub\n", This, strings, states);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_NewDocument(IDHTMLEdit *iface)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->() stub\n", This);
    return S_OK;
}

static HRESULT WINAPI DHTMLEdit_LoadURL(IDHTMLEdit *iface, BSTR url)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_w(url));
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_FilterSourceCode(IDHTMLEdit *iface, BSTR in, BSTR *out)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s, %p)\n", This, debugstr_w(in), out);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_Refresh(IDHTMLEdit *iface)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->() stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_DOM(IDHTMLEdit *iface, IHTMLDocument2 **value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_DocumentHTML(IDHTMLEdit *iface, BSTR *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_DocumentHTML(IDHTMLEdit *iface, BSTR html)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s) stub\n", This, debugstr_w(html));
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_ActivateApplets(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_ActivateApplets(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_ActivateActiveXControls(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_ActivateActiveXControls(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_ActivateDTCs(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_ActivateDTCs(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_ShowDetails(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_ShowDetails(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_ShowBorders(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_ShowBorders(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_Appearance(IDHTMLEdit *iface, DHTMLEDITAPPEARANCE *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_Appearance(IDHTMLEdit *iface, DHTMLEDITAPPEARANCE value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%u) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_Scrollbars(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_Scrollbars(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_ScrollbarAppearance(IDHTMLEdit *iface, DHTMLEDITAPPEARANCE *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_ScrollbarAppearance(IDHTMLEdit *iface, DHTMLEDITAPPEARANCE value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%u) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_SourceCodePreservation(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_SourceCodePreservation(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_AbsoluteDropMode(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_AbsoluteDropMode(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_SnapToGridX(IDHTMLEdit *iface, LONG *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_SnapToGridX(IDHTMLEdit *iface, LONG value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%ld) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_SnapToGridY(IDHTMLEdit *iface, LONG *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_SnapToGridY(IDHTMLEdit *iface, LONG value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%ld) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_SnapToGrid(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_SnapToGrid(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_IsDirty(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_CurrentDocumentPath(IDHTMLEdit *iface, BSTR *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_BaseURL(IDHTMLEdit *iface, BSTR *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_BaseURL(IDHTMLEdit *iface, BSTR value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s) stub\n", This, debugstr_w(value));
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_DocumentTitle(IDHTMLEdit *iface, BSTR *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_UseDivOnCarriageReturn(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_UseDivOnCarriageReturn(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_Busy(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_LoadDocument(IDHTMLEdit *iface, VARIANT *path, VARIANT *prompt)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s, %s) stub\n", This, debugstr_variant(path), debugstr_variant(prompt));
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_SaveDocument(IDHTMLEdit *iface, VARIANT *path, VARIANT *prompt)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s, %s) stub\n", This, debugstr_variant(path), debugstr_variant(prompt));
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_PrintDocument(IDHTMLEdit *iface, VARIANT *prompt)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s) stub\n", This, debugstr_variant(prompt));
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_get_BrowseMode(IDHTMLEdit *iface, VARIANT_BOOL *value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_put_BrowseMode(IDHTMLEdit *iface, VARIANT_BOOL value)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%04x) stub\n", This, value);
    return E_NOTIMPL;
}

static const IDHTMLEditVtbl DHTMLEditVtbl = {
    DHTMLEdit_QueryInterface,
    DHTMLEdit_AddRef,
    DHTMLEdit_Release,
    DHTMLEdit_GetTypeInfoCount,
    DHTMLEdit_GetTypeInfo,
    DHTMLEdit_GetIDsOfNames,
    DHTMLEdit_Invoke,
    DHTMLEdit_ExecCommand,
    DHTMLEdit_QueryStatus,
    DHTMLEdit_SetContextMenu,
    DHTMLEdit_NewDocument,
    DHTMLEdit_LoadURL,
    DHTMLEdit_FilterSourceCode,
    DHTMLEdit_Refresh,
    DHTMLEdit_get_DOM,
    DHTMLEdit_get_DocumentHTML,
    DHTMLEdit_put_DocumentHTML,
    DHTMLEdit_get_ActivateApplets,
    DHTMLEdit_put_ActivateApplets,
    DHTMLEdit_get_ActivateActiveXControls,
    DHTMLEdit_put_ActivateActiveXControls,
    DHTMLEdit_get_ActivateDTCs,
    DHTMLEdit_put_ActivateDTCs,
    DHTMLEdit_get_ShowDetails,
    DHTMLEdit_put_ShowDetails,
    DHTMLEdit_get_ShowBorders,
    DHTMLEdit_put_ShowBorders,
    DHTMLEdit_get_Appearance,
    DHTMLEdit_put_Appearance,
    DHTMLEdit_get_Scrollbars,
    DHTMLEdit_put_Scrollbars,
    DHTMLEdit_get_ScrollbarAppearance,
    DHTMLEdit_put_ScrollbarAppearance,
    DHTMLEdit_get_SourceCodePreservation,
    DHTMLEdit_put_SourceCodePreservation,
    DHTMLEdit_get_AbsoluteDropMode,
    DHTMLEdit_put_AbsoluteDropMode,
    DHTMLEdit_get_SnapToGridX,
    DHTMLEdit_put_SnapToGridX,
    DHTMLEdit_get_SnapToGridY,
    DHTMLEdit_put_SnapToGridY,
    DHTMLEdit_get_SnapToGrid,
    DHTMLEdit_put_SnapToGrid,
    DHTMLEdit_get_IsDirty,
    DHTMLEdit_get_CurrentDocumentPath,
    DHTMLEdit_get_BaseURL,
    DHTMLEdit_put_BaseURL,
    DHTMLEdit_get_DocumentTitle,
    DHTMLEdit_get_UseDivOnCarriageReturn,
    DHTMLEdit_put_UseDivOnCarriageReturn,
    DHTMLEdit_get_Busy,
    DHTMLEdit_LoadDocument,
    DHTMLEdit_SaveDocument,
    DHTMLEdit_PrintDocument,
    DHTMLEdit_get_BrowseMode,
    DHTMLEdit_put_BrowseMode
};

static HRESULT WINAPI OleObject_QueryInterface(IOleObject *iface, REFIID iid, void **out)
{
    return dhtml_edit_qi(impl_from_IOleObject(iface), iid, out);
}

static ULONG WINAPI OleObject_AddRef(IOleObject *iface)
{
    return dhtml_edit_addref(impl_from_IOleObject(iface));
}

static ULONG WINAPI OleObject_Release(IOleObject *iface)
{
    return dhtml_edit_release(impl_from_IOleObject(iface));
}

static HRESULT WINAPI OleObject_SetClientSite(IOleObject *iface, IOleClientSite *value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, value);

    if (This->client_site)
        IOleClientSite_Release(This->client_site);

    if (value)
        IOleClientSite_AddRef(value);

    This->client_site = value;

    return S_OK;
}

static HRESULT WINAPI OleObject_GetClientSite(IOleObject *iface, IOleClientSite **value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);

    TRACE("(%p)->(%p)\n", This, value);

    if (This->client_site)
        IOleClientSite_AddRef(This->client_site);

    *value = This->client_site;

    return S_OK;
}

static HRESULT WINAPI OleObject_SetHostNames(IOleObject *iface,
                                             const OLECHAR *container_app, const OLECHAR *container_obj)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p, %p) stub\n", This, container_app, container_obj);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Close(IOleObject *iface, DWORD save)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%lu) stub\n", This, save);
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD moniker_id, IMoniker *value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%lu, %p) stub\n", This, moniker_id, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD assign, DWORD moniker_id, IMoniker **value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%lu, %p) stub\n", This, moniker_id, value);
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *data_obj, BOOL creation, DWORD reserved)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p, %u, %lu) stub\n", This, data_obj, creation, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD reserved, IDataObject **value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%lu, %p) stub\n", This, reserved, value);
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG verb, MSG *msg, IOleClientSite *active_site,
                                       LONG index, HWND parent, const RECT *pos)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    TRACE("(%p)->(%ld, %p, %p, %ld, %p, %p) stub\n", This, verb, msg, active_site, index, parent, pos);

    if (verb == OLEIVERB_INPLACEACTIVATE)
    {
        IOleClientSite_OnShowWindow(This->client_site, TRUE);
        return S_OK;
    }

    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumVerbs(IOleObject *iface, IEnumOLEVERB **verb)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p) stub\n", This, verb);
    *verb = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Update(IOleObject *iface)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_IsUpToDate(IOleObject *iface)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserClassID(IOleObject *iface, CLSID *clsid)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p) stub\n", This, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetUserType(IOleObject *iface, DWORD type_type, OLECHAR **type)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%lu, %p) stub\n", This, type_type, type);
    *type = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD aspect, SIZEL *size_limit)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    TRACE("(%p)->(%lu, %p)\n", This, aspect, size_limit);

    if(aspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    This->extent = *size_limit;
    return S_OK;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD aspect, SIZEL *size_limit)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    TRACE("(%p)->(%lu, %p)\n", This, aspect, size_limit);

    if(aspect != DVASPECT_CONTENT)
        return E_FAIL;

    *size_limit = This->extent;
    return S_OK;
}

static HRESULT WINAPI OleObject_Advise(IOleObject *iface, IAdviseSink *sink, DWORD *conn)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p, %p) stub\n", This, sink, conn);
    *conn = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_Unadvise(IOleObject *iface, DWORD conn)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%lu) stub\n", This, conn);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_EnumAdvise(IOleObject *iface, IEnumSTATDATA **advise)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p) stub\n", This, advise);
    *advise = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMiscStatus(IOleObject *iface, DWORD aspect, DWORD *status)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    TRACE("(%p)->(%lu, %p)\n", This, aspect, status);

    return OleRegGetMiscStatus(&CLSID_DHTMLEdit, aspect, status);
}

static HRESULT WINAPI OleObject_SetColorScheme(IOleObject *iface, LOGPALETTE *palette)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p) stub\n", This, palette);
    return E_NOTIMPL;
}

static const IOleObjectVtbl OleObjectVtbl = {
    OleObject_QueryInterface,
    OleObject_AddRef,
    OleObject_Release,
    OleObject_SetClientSite,
    OleObject_GetClientSite,
    OleObject_SetHostNames,
    OleObject_Close,
    OleObject_SetMoniker,
    OleObject_GetMoniker,
    OleObject_InitFromData,
    OleObject_GetClipboardData,
    OleObject_DoVerb,
    OleObject_EnumVerbs,
    OleObject_Update,
    OleObject_IsUpToDate,
    OleObject_GetUserClassID,
    OleObject_GetUserType,
    OleObject_SetExtent,
    OleObject_GetExtent,
    OleObject_Advise,
    OleObject_Unadvise,
    OleObject_EnumAdvise,
    OleObject_GetMiscStatus,
    OleObject_SetColorScheme
};

static HRESULT WINAPI ProvideClassInfo2_QueryInterface(IProvideClassInfo2 *iface, REFIID iid, LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IProvideClassInfo2(iface), iid, out);
}

static ULONG WINAPI ProvideClassInfo2_AddRef(IProvideClassInfo2 *iface)
{
    return dhtml_edit_addref(impl_from_IProvideClassInfo2(iface));
}

static ULONG WINAPI ProvideClassInfo2_Release(IProvideClassInfo2 *iface)
{
    return dhtml_edit_release(impl_from_IProvideClassInfo2(iface));
}

static HRESULT WINAPI ProvideClassInfo2_GetClassInfo(IProvideClassInfo2 *iface, ITypeInfo **ppTI)
{
    DHTMLEditImpl *This = impl_from_IProvideClassInfo2(iface);
    FIXME("(%p)->(%p)\n", This, ppTI);
    return E_NOTIMPL;
}

static HRESULT WINAPI ProvideClassInfo2_GetGUID(IProvideClassInfo2 *iface, DWORD dwGuidKind, GUID *pGUID)
{
    DHTMLEditImpl *This = impl_from_IProvideClassInfo2(iface);
    FIXME("(%p)->(%ld %p)\n", This, dwGuidKind, pGUID);
    return E_NOTIMPL;
}

static const IProvideClassInfo2Vtbl ProvideClassInfo2Vtbl = {
    ProvideClassInfo2_QueryInterface,
    ProvideClassInfo2_AddRef,
    ProvideClassInfo2_Release,
    ProvideClassInfo2_GetClassInfo,
    ProvideClassInfo2_GetGUID
};

static HRESULT WINAPI PersistStorage_QueryInterface(IPersistStorage *iface, REFIID iid, void **out)
{
    return dhtml_edit_qi(impl_from_IPersistStorage(iface), iid, out);
}

static ULONG WINAPI PersistStorage_AddRef(IPersistStorage *iface)
{
    return dhtml_edit_addref(impl_from_IPersistStorage(iface));
}

static ULONG WINAPI PersistStorage_Release(IPersistStorage *iface)
{
    return dhtml_edit_release(impl_from_IPersistStorage(iface));
}

static HRESULT WINAPI PersistStorage_GetClassID(IPersistStorage *iface, CLSID *clsid)
{
    DHTMLEditImpl *This = impl_from_IPersistStorage(iface);
    FIXME("(%p)->(%p) stub\n", This, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_IsDirty(IPersistStorage *iface)
{
    DHTMLEditImpl *This = impl_from_IPersistStorage(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_InitNew(IPersistStorage *iface, LPSTORAGE storage)
{
    DHTMLEditImpl *This = impl_from_IPersistStorage(iface);
    FIXME("(%p)->(%p) stub\n", This, storage);
    return S_OK;
}

static HRESULT WINAPI PersistStorage_Load(IPersistStorage *iface, LPSTORAGE storage)
{
    DHTMLEditImpl *This = impl_from_IPersistStorage(iface);
    FIXME("(%p)->(%p) stub\n", This, storage);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_Save(IPersistStorage *iface, LPSTORAGE storage, BOOL sameasld)
{
    DHTMLEditImpl *This = impl_from_IPersistStorage(iface);
    FIXME("(%p)->(%p, %u) stub\n", This, storage, sameasld);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStorage_SaveCompleted(IPersistStorage *iface, LPSTORAGE storage)
{
    DHTMLEditImpl *This = impl_from_IPersistStorage(iface);
    FIXME("(%p)->(%p)\n", This, storage);
    return E_NOTIMPL;
}

static const IPersistStorageVtbl PersistStorageVtbl =
{
    PersistStorage_QueryInterface,
    PersistStorage_AddRef,
    PersistStorage_Release,
    PersistStorage_GetClassID,
    PersistStorage_IsDirty,
    PersistStorage_InitNew,
    PersistStorage_Load,
    PersistStorage_Save,
    PersistStorage_SaveCompleted
};

static HRESULT WINAPI PersistStreamInit_QueryInterface(IPersistStreamInit *iface, REFIID iid, void **out)
{
    return dhtml_edit_qi(impl_from_IPersistStreamInit(iface), iid, out);
}

static ULONG WINAPI PersistStreamInit_AddRef(IPersistStreamInit *iface)
{
    return dhtml_edit_addref(impl_from_IPersistStreamInit(iface));
}

static ULONG WINAPI PersistStreamInit_Release(IPersistStreamInit *iface)
{
    return dhtml_edit_release(impl_from_IPersistStreamInit(iface));
}

static HRESULT WINAPI PersistStreamInit_GetClassID(IPersistStreamInit *iface, CLSID *clsid)
{
    DHTMLEditImpl *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p) stub\n", This, clsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_IsDirty(IPersistStreamInit *iface)
{
    DHTMLEditImpl *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_Load(IPersistStreamInit *iface, IStream *stream)
{
    DHTMLEditImpl *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p) stub\n", This, stream);
    return S_OK;
}

static HRESULT WINAPI PersistStreamInit_Save(IPersistStreamInit *iface, IStream *stream, BOOL clear_dirty)
{
    DHTMLEditImpl *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p, %u) stub\n", This, stream, clear_dirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_GetSizeMax(IPersistStreamInit *iface, ULARGE_INTEGER *value)
{
    DHTMLEditImpl *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p)->(%p) stub\n", This, value);
    value->QuadPart = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI PersistStreamInit_InitNew(IPersistStreamInit *iface)
{
    DHTMLEditImpl *This = impl_from_IPersistStreamInit(iface);
    FIXME("(%p) stub\n", This);
    return E_NOTIMPL;
}

static const IPersistStreamInitVtbl PersistStreamInitVtbl = {
    PersistStreamInit_QueryInterface,
    PersistStreamInit_AddRef,
    PersistStreamInit_Release,
    PersistStreamInit_GetClassID,
    PersistStreamInit_IsDirty,
    PersistStreamInit_Load,
    PersistStreamInit_Save,
    PersistStreamInit_GetSizeMax,
    PersistStreamInit_InitNew
};

static HRESULT WINAPI OleControl_QueryInterface(IOleControl *iface, REFIID iid, void **out)
{
    return dhtml_edit_qi(impl_from_IOleControl(iface), iid, out);
}

static ULONG WINAPI OleControl_AddRef(IOleControl *iface)
{
    return dhtml_edit_addref(impl_from_IOleControl(iface));
}

static ULONG WINAPI OleControl_Release(IOleControl *iface)
{
    return dhtml_edit_release(impl_from_IOleControl(iface));
}

static HRESULT WINAPI OleControl_GetControlInfo(IOleControl *iface, CONTROLINFO *pCI)
{
    DHTMLEditImpl *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, pCI);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnMnemonic(IOleControl *iface, MSG *msg)
{
    DHTMLEditImpl *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%p)\n", This, msg);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_OnAmbientPropertyChange(IOleControl *iface, DISPID dispID)
{
    DHTMLEditImpl *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%ld)\n", This, dispID);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleControl_FreezeEvents(IOleControl *iface, BOOL freeze)
{
    DHTMLEditImpl *This = impl_from_IOleControl(iface);
    FIXME("(%p)->(%x)\n", This, freeze);
    return E_NOTIMPL;
}

static const IOleControlVtbl OleControlVtbl = {
    OleControl_QueryInterface,
    OleControl_AddRef,
    OleControl_Release,
    OleControl_GetControlInfo,
    OleControl_OnMnemonic,
    OleControl_OnAmbientPropertyChange,
    OleControl_FreezeEvents
};

static HRESULT WINAPI ViewObjectEx_QueryInterface(IViewObjectEx *iface, REFIID iid, LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IViewObjectEx(iface), iid, out);
}

static ULONG WINAPI ViewObjectEx_AddRef(IViewObjectEx *iface)
{
    return dhtml_edit_addref(impl_from_IViewObjectEx(iface));
}

static ULONG WINAPI ViewObjectEx_Release(IViewObjectEx *iface)
{
    return dhtml_edit_release(impl_from_IViewObjectEx(iface));
}

static HRESULT WINAPI ViewObjectEx_Draw(IViewObjectEx *iface, DWORD drawaspect, LONG index, void *aspect,
    DVTARGETDEVICE *device, HDC target_dev, HDC hdc_draw, const RECTL *bounds, const RECTL *win_bounds,
    BOOL (STDMETHODCALLTYPE *fn_continue)(ULONG_PTR cont), ULONG_PTR cont)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p %p %p %p %p %p %Iu)\n", This, drawaspect, index, aspect, device, target_dev,
        hdc_draw, bounds, win_bounds, fn_continue, cont);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetColorSet(IViewObjectEx *iface, DWORD drawaspect, LONG index, void *aspect,
    DVTARGETDEVICE *device, HDC hic_target, LOGPALETTE **colorset)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, drawaspect, index, aspect, device, hic_target,
        colorset);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_Freeze(IViewObjectEx *iface, DWORD drawaspect, LONG index, void *aspect,
    DWORD *freeze)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p)\n", This, drawaspect, index, aspect, freeze);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_Unfreeze(IViewObjectEx *iface, DWORD freeze)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld)\n", This, freeze);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_SetAdvise(IViewObjectEx *iface, DWORD aspects, DWORD advf, IAdviseSink *sink)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p)\n", This, aspects, advf, sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetAdvise(IViewObjectEx *iface, DWORD *aspects, DWORD *advf, IAdviseSink **sink)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%p %p %p)\n", This, aspects, advf, sink);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetExtent(IViewObjectEx *iface, DWORD draw_aspect, LONG index,
    DVTARGETDEVICE *device, SIZEL *size)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p)\n", This, draw_aspect, index, device, size);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetRect(IViewObjectEx *iface, DWORD aspect, RECTL *rect)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %p)\n", This, aspect, rect);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetViewStatus(IViewObjectEx *iface, DWORD *status)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    TRACE("(%p)->(%p)\n", This, status);

    *status = VIEWSTATUS_OPAQUE | VIEWSTATUS_SOLIDBKGND;
    return S_OK;
}

static HRESULT WINAPI ViewObjectEx_QueryHitPoint(IViewObjectEx *iface, DWORD aspect, const RECT *bounds,
    POINT pt, LONG close_hint, DWORD *hit_result)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %s %s %ld %p)\n", This, aspect, wine_dbgstr_rect(bounds), wine_dbgstr_point(&pt), close_hint, hit_result);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_QueryHitRect(IViewObjectEx *iface, DWORD aspect, const RECT *bounds,
    const RECT *loc, LONG close_hint, DWORD *hit_result)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %s %s %ld %p)\n", This, aspect, wine_dbgstr_rect(bounds), wine_dbgstr_rect(loc), close_hint, hit_result);

    return E_NOTIMPL;
}

static HRESULT WINAPI ViewObjectEx_GetNaturalExtent(IViewObjectEx *iface, DWORD aspect, LONG index,
    DVTARGETDEVICE *device, HDC target_hdc, DVEXTENTINFO *extent_info, SIZEL *size)
{
    DHTMLEditImpl *This = impl_from_IViewObjectEx(iface);

    FIXME("(%p)->(%ld %ld %p %p %p %p)\n", This, aspect, index, device, target_hdc, extent_info, size);

    return E_NOTIMPL;
}

static const IViewObjectExVtbl ViewObjectExVtbl = {
    ViewObjectEx_QueryInterface,
    ViewObjectEx_AddRef,
    ViewObjectEx_Release,
    ViewObjectEx_Draw,
    ViewObjectEx_GetColorSet,
    ViewObjectEx_Freeze,
    ViewObjectEx_Unfreeze,
    ViewObjectEx_SetAdvise,
    ViewObjectEx_GetAdvise,
    ViewObjectEx_GetExtent,
    ViewObjectEx_GetRect,
    ViewObjectEx_GetViewStatus,
    ViewObjectEx_QueryHitPoint,
    ViewObjectEx_QueryHitRect,
    ViewObjectEx_GetNaturalExtent
};

static HRESULT WINAPI OleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID iid, LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IOleInPlaceObjectWindowless(iface), iid, out);
}

static ULONG WINAPI OleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    return dhtml_edit_addref(impl_from_IOleInPlaceObjectWindowless(iface));
}

static ULONG WINAPI OleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    return dhtml_edit_release(impl_from_IOleInPlaceObjectWindowless(iface));
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface, HWND *phwnd)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p %p)\n", This, lprcPosRect, lprcClipRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%u %Iu %Iu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceObjectWindowless(iface);
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static const IOleInPlaceObjectWindowlessVtbl OleInPlaceObjectWindowlessVtbl = {
    OleInPlaceObjectWindowless_QueryInterface,
    OleInPlaceObjectWindowless_AddRef,
    OleInPlaceObjectWindowless_Release,
    OleInPlaceObjectWindowless_GetWindow,
    OleInPlaceObjectWindowless_ContextSensitiveHelp,
    OleInPlaceObjectWindowless_InPlaceDeactivate,
    OleInPlaceObjectWindowless_UIDeactivate,
    OleInPlaceObjectWindowless_SetObjectRects,
    OleInPlaceObjectWindowless_ReactivateAndUndo,
    OleInPlaceObjectWindowless_OnWindowMessage,
    OleInPlaceObjectWindowless_GetDropTarget
};

static HRESULT WINAPI InPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface,
        REFIID iid, LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IOleInPlaceActiveObject(iface), iid, out);
}

static ULONG WINAPI InPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    return dhtml_edit_addref(impl_from_IOleInPlaceActiveObject(iface));
}

static ULONG WINAPI InPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    return dhtml_edit_release(impl_from_IOleInPlaceActiveObject(iface));
}

static HRESULT WINAPI InPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface,
        HWND *phwnd)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceObjectWindowless_GetWindow(&This->IOleInPlaceObjectWindowless_iface, phwnd);
}

static HRESULT WINAPI InPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface,
        BOOL fEnterMode)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    return IOleInPlaceObjectWindowless_ContextSensitiveHelp(&This->IOleInPlaceObjectWindowless_iface, fEnterMode);
}

static HRESULT WINAPI InPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface,
        LPMSG lpmsg)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface,
        BOOL fActivate)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface,
        LPCRECT lprcBorder, IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%p %p %x)\n", This, lprcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI InPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface,
        BOOL fEnable)
{
    DHTMLEditImpl *This = impl_from_IOleInPlaceActiveObject(iface);
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl OleInPlaceActiveObjectVtbl = {
    InPlaceActiveObject_QueryInterface,
    InPlaceActiveObject_AddRef,
    InPlaceActiveObject_Release,
    InPlaceActiveObject_GetWindow,
    InPlaceActiveObject_ContextSensitiveHelp,
    InPlaceActiveObject_TranslateAccelerator,
    InPlaceActiveObject_OnFrameWindowActivate,
    InPlaceActiveObject_OnDocWindowActivate,
    InPlaceActiveObject_ResizeBorder,
    InPlaceActiveObject_EnableModeless
};

static HRESULT WINAPI ConnectionPointContainer_QueryInterface(IConnectionPointContainer *iface,
        REFIID iid, LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IConnectionPointContainer(iface), iid, out);
}

static ULONG WINAPI ConnectionPointContainer_AddRef(IConnectionPointContainer *iface)
{
    return dhtml_edit_addref(impl_from_IConnectionPointContainer(iface));
}

static ULONG WINAPI ConnectionPointContainer_Release(IConnectionPointContainer *iface)
{
    return dhtml_edit_release(impl_from_IConnectionPointContainer(iface));
}

static HRESULT WINAPI ConnectionPointContainer_EnumConnectionPoints(IConnectionPointContainer *iface,
        IEnumConnectionPoints **ppEnum)
{
    DHTMLEditImpl *This = impl_from_IConnectionPointContainer(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPointContainer_FindConnectionPoint(IConnectionPointContainer *iface,
        REFIID riid, IConnectionPoint **point)
{
    DHTMLEditImpl *This = impl_from_IConnectionPointContainer(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), point);

    if (!point)
        return E_POINTER;

    if (IsEqualGUID(riid, This->conpt.riid))
    {
        *point = &This->conpt.IConnectionPoint_iface;
        IConnectionPoint_AddRef(*point);
        return S_OK;
    }

    FIXME("unsupported connection point %s\n", debugstr_guid(riid));
    return CONNECT_E_NOCONNECTION;
}

static const IConnectionPointContainerVtbl ConnectionPointContainerVtbl =
{
    ConnectionPointContainer_QueryInterface,
    ConnectionPointContainer_AddRef,
    ConnectionPointContainer_Release,
    ConnectionPointContainer_EnumConnectionPoints,
    ConnectionPointContainer_FindConnectionPoint
};

static inline ConnectionPoint *impl_from_IConnectionPoint(IConnectionPoint *iface)
{
    return CONTAINING_RECORD(iface, ConnectionPoint, IConnectionPoint_iface);
}

static HRESULT WINAPI ConnectionPoint_QueryInterface(IConnectionPoint *iface, REFIID iid, LPVOID *out)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);

    if (IsEqualGUID(&IID_IUnknown, iid) || IsEqualGUID(&IID_IConnectionPoint, iid))
    {
        *out = &This->IConnectionPoint_iface;
        DHTMLEdit_AddRef(&This->dhed->IDHTMLEdit_iface);
        return S_OK;
    }

    *out = NULL;
    WARN("Unsupported interface %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI ConnectionPoint_AddRef(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_AddRef(&This->dhed->IConnectionPointContainer_iface);
}

static ULONG WINAPI ConnectionPoint_Release(IConnectionPoint *iface)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    return IConnectionPointContainer_Release(&This->dhed->IConnectionPointContainer_iface);
}

static HRESULT WINAPI ConnectionPoint_GetConnectionInterface(IConnectionPoint *iface, IID *iid)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("%p, %p\n", This, iid);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_GetConnectionPointContainer(IConnectionPoint *iface,
        IConnectionPointContainer **container)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("%p, %p\n", This, container);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Advise(IConnectionPoint *iface, IUnknown *unk_sink,
        DWORD *cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("%p, %p, %p\n", This, unk_sink, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_Unadvise(IConnectionPoint *iface, DWORD cookie)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("%p, %ld\n", This, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI ConnectionPoint_EnumConnections(IConnectionPoint *iface,
        IEnumConnections **points)
{
    ConnectionPoint *This = impl_from_IConnectionPoint(iface);
    FIXME("%p, %p\n", This, points);
    return E_NOTIMPL;
}

static const IConnectionPointVtbl ConnectionPointVtbl = {
    ConnectionPoint_QueryInterface,
    ConnectionPoint_AddRef,
    ConnectionPoint_Release,
    ConnectionPoint_GetConnectionInterface,
    ConnectionPoint_GetConnectionPointContainer,
    ConnectionPoint_Advise,
    ConnectionPoint_Unadvise,
    ConnectionPoint_EnumConnections
};

static HRESULT WINAPI DataObject_QueryInterface(IDataObject *iface, REFIID iid, LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IDataObject(iface), iid, out);
}

static ULONG WINAPI DataObject_AddRef(IDataObject *iface)
{
    return dhtml_edit_addref(impl_from_IDataObject(iface));
}

static ULONG WINAPI DataObject_Release(IDataObject *iface)
{
    return dhtml_edit_release(impl_from_IDataObject(iface));
}

static HRESULT WINAPI DataObject_GetData(IDataObject *iface, LPFORMATETC pformatetcIn,
        STGMEDIUM *pmedium)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetDataHere(IDataObject *iface, LPFORMATETC pformatetc,
        STGMEDIUM *pmedium)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_QueryGetData(IDataObject *iface, LPFORMATETC pformatetc)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_GetCanonicalFormatEtc(IDataObject *iface,
        LPFORMATETC pformatectIn, LPFORMATETC pformatetcOut)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_SetData(IDataObject *iface, LPFORMATETC pformatetc,
        STGMEDIUM *pmedium, BOOL fRelease)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumFormatEtc(IDataObject *iface, DWORD dwDirection,
        IEnumFORMATETC **ppenumFormatEtc)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DAdvise(IDataObject *iface, FORMATETC *pformatetc,
        DWORD advf, IAdviseSink *pAdvSink, DWORD *pdwConnection)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_DUnadvise(IDataObject *iface, DWORD dwConnection)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI DataObject_EnumDAdvise(IDataObject *iface, IEnumSTATDATA **ppenumAdvise)
{
    DHTMLEditImpl *This = impl_from_IDataObject(iface);
    FIXME("(%p)->()\n", This);
    return E_NOTIMPL;
}

static const IDataObjectVtbl DataObjectVtbl = {
    DataObject_QueryInterface,
    DataObject_AddRef,
    DataObject_Release,
    DataObject_GetData,
    DataObject_GetDataHere,
    DataObject_QueryGetData,
    DataObject_GetCanonicalFormatEtc,
    DataObject_SetData,
    DataObject_EnumFormatEtc,
    DataObject_DAdvise,
    DataObject_DUnadvise,
    DataObject_EnumDAdvise
};

static HRESULT WINAPI ServiceProvider_QueryInterface(IServiceProvider *iface, REFIID iid,
        LPVOID *out)
{
    return dhtml_edit_qi(impl_from_IServiceProvider(iface), iid, out);
}

static ULONG WINAPI ServiceProvider_AddRef(IServiceProvider *iface)
{
    return dhtml_edit_addref(impl_from_IServiceProvider(iface));
}

static ULONG WINAPI ServiceProvider_Release(IServiceProvider *iface)
{
    return dhtml_edit_release(impl_from_IServiceProvider(iface));
}

static HRESULT WINAPI ServiceProvider_QueryService(IServiceProvider *iface, REFGUID guidService,
        REFIID riid, void **ppv)
{
    DHTMLEditImpl *This = impl_from_IServiceProvider(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_guid(guidService), debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static const IServiceProviderVtbl ServiceProviderVtbl = {
    ServiceProvider_QueryInterface,
    ServiceProvider_AddRef,
    ServiceProvider_Release,
    ServiceProvider_QueryService
};

HRESULT dhtml_edit_create(REFIID iid, void **out)
{
    DHTMLEditImpl *This;
    DWORD dpi_x, dpi_y;
    HDC hdc;
    HRESULT ret;

    TRACE("(%s, %p)\n", debugstr_guid(iid), out);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IDHTMLEdit_iface.lpVtbl = &DHTMLEditVtbl;
    This->IOleObject_iface.lpVtbl = &OleObjectVtbl;
    This->IProvideClassInfo2_iface.lpVtbl = &ProvideClassInfo2Vtbl;
    This->IPersistStorage_iface.lpVtbl = &PersistStorageVtbl;
    This->IPersistStreamInit_iface.lpVtbl = &PersistStreamInitVtbl;
    This->IOleControl_iface.lpVtbl = &OleControlVtbl;
    This->IViewObjectEx_iface.lpVtbl = &ViewObjectExVtbl;
    This->IOleInPlaceObjectWindowless_iface.lpVtbl = &OleInPlaceObjectWindowlessVtbl;
    This->IOleInPlaceActiveObject_iface.lpVtbl = &OleInPlaceActiveObjectVtbl;
    This->IConnectionPointContainer_iface.lpVtbl = &ConnectionPointContainerVtbl;
    This->IDataObject_iface.lpVtbl = &DataObjectVtbl;
    This->IServiceProvider_iface.lpVtbl = &ServiceProviderVtbl;

    This->client_site = NULL;
    This->ref = 1;

    This->conpt.dhed = This;
    This->conpt.riid = &DIID__DHTMLEditEvents;
    This->conpt.IConnectionPoint_iface.lpVtbl = &ConnectionPointVtbl;

    hdc = GetDC(0);
    dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
    dpi_y = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(0, hdc);

    This->extent.cx = MulDiv(192, 2540, dpi_x);
    This->extent.cy = MulDiv(192, 2540, dpi_y);

    ret = IDHTMLEdit_QueryInterface(&This->IDHTMLEdit_iface, iid, out);
    IDHTMLEdit_Release(&This->IDHTMLEdit_iface);
    return ret;
}
