/*
 * Copyright 2017 Alex Henrie
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

typedef struct
{
    IDHTMLEdit IDHTMLEdit_iface;
    IOleObject IOleObject_iface;
    IPersistStreamInit IPersistStreamInit_iface;
    IOleClientSite *client_site;
    LONG ref;
} DHTMLEditImpl;

static inline DHTMLEditImpl *impl_from_IDHTMLEdit(IDHTMLEdit *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IDHTMLEdit_iface);
}

static inline DHTMLEditImpl *impl_from_IOleObject(IOleObject *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IOleObject_iface);
}

static inline DHTMLEditImpl *impl_from_IPersistStreamInit(IPersistStreamInit *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IPersistStreamInit_iface);
}

static ULONG dhtml_edit_addref(DHTMLEditImpl *This)
{
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

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
    else if (IsEqualGUID(iid, &IID_IPersistStreamInit))
    {
        dhtml_edit_addref(This);
        *out = &This->IPersistStreamInit_iface;
        return S_OK;
    }

    *out = NULL;
    ERR("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG dhtml_edit_release(DHTMLEditImpl *This)
{
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

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
    FIXME("(%p)->(%p) stub\n", This, count);
    *count = 0;
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_GetTypeInfo(IDHTMLEdit *iface, UINT type_index, LCID lcid, ITypeInfo **type_info)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%u, %08x, %p) stub\n", This, type_index, lcid, type_info);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_GetIDsOfNames(IDHTMLEdit *iface, REFIID iid, OLECHAR **names, UINT name_count,
                                              LCID lcid, DISPID *disp_ids)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%s, %p, %u, %08x, %p) stub\n", This, debugstr_guid(iid), names, name_count, lcid, disp_ids);
    return E_NOTIMPL;
}

static HRESULT WINAPI DHTMLEdit_Invoke(IDHTMLEdit *iface, DISPID member, REFIID iid, LCID lcid, WORD flags,
                                       DISPPARAMS *params, VARIANT *ret, EXCEPINFO *exception_info, UINT *error_index)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    FIXME("(%p)->(%d, %s, %08x, 0x%x, %p, %p, %p, %p) stub\n",
          This, member, debugstr_guid(iid), lcid, flags, params, ret, exception_info, error_index);
    return E_NOTIMPL;
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
    return E_NOTIMPL;
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
    FIXME("(%p)->(%d) stub\n", This, value);
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
    FIXME("(%p)->(%d) stub\n", This, value);
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
    FIXME("(%p)->(%u) stub\n", This, save);
    return S_OK;
}

static HRESULT WINAPI OleObject_SetMoniker(IOleObject *iface, DWORD moniker_id, IMoniker *value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%u, %p) stub\n", This, moniker_id, value);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetMoniker(IOleObject *iface, DWORD assign, DWORD moniker_id, IMoniker **value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%u, %p) stub\n", This, moniker_id, value);
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_InitFromData(IOleObject *iface, IDataObject *data_obj, BOOL creation, DWORD reserved)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%p, %u, %u) stub\n", This, data_obj, creation, reserved);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetClipboardData(IOleObject *iface, DWORD reserved, IDataObject **value)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%u, %p) stub\n", This, reserved, value);
    *value = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_DoVerb(IOleObject *iface, LONG verb, MSG *msg, IOleClientSite *active_site,
                                       LONG index, HWND parent, const RECT *pos)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    TRACE("(%p)->(%d, %p, %p, %d, %p, %p) stub\n", This, verb, msg, active_site, index, parent, pos);

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
    FIXME("(%p)->(%u, %p) stub\n", This, type_type, type);
    *type = NULL;
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_SetExtent(IOleObject *iface, DWORD aspect, SIZEL *size_limit)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%u, %p) stub\n", This, aspect, size_limit);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleObject_GetExtent(IOleObject *iface, DWORD aspect, SIZEL *size_limit)
{
    DHTMLEditImpl *This = impl_from_IOleObject(iface);
    FIXME("(%p)->(%u, %p) stub\n", This, aspect, size_limit);
    return E_NOTIMPL;
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
    FIXME("(%p)->(%u) stub\n", This, conn);
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
    FIXME("(%p)->(%u, %p) stub\n", This, aspect, status);
    *status = 0;
    return E_NOTIMPL;
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

HRESULT dhtml_edit_create(REFIID iid, void **out)
{
    DHTMLEditImpl *This;
    HRESULT ret;

    TRACE("(%s, %p)\n", debugstr_guid(iid), out);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IDHTMLEdit_iface.lpVtbl = &DHTMLEditVtbl;
    This->IOleObject_iface.lpVtbl = &OleObjectVtbl;
    This->IPersistStreamInit_iface.lpVtbl = &PersistStreamInitVtbl;
    This->client_site = NULL;
    This->ref = 1;

    ret = IDHTMLEdit_QueryInterface(&This->IDHTMLEdit_iface, iid, out);
    IDHTMLEdit_Release(&This->IDHTMLEdit_iface);
    return ret;
}
