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
    LONG ref;
} DHTMLEditImpl;

static inline DHTMLEditImpl *impl_from_IDHTMLEdit(IDHTMLEdit *iface)
{
    return CONTAINING_RECORD(iface, DHTMLEditImpl, IDHTMLEdit_iface);
}

static HRESULT WINAPI DHTMLEdit_QueryInterface(IDHTMLEdit *iface, REFIID iid, void **out)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(iid), out);

    if (IsEqualGUID(iid, &IID_IUnknown) ||
        IsEqualGUID(iid, &IID_IDispatch) ||
        IsEqualGUID(iid, &IID_IDHTMLSafe) ||
        IsEqualGUID(iid, &IID_IDHTMLEdit))
    {
        IUnknown_AddRef(iface);
        *out = iface;
        return S_OK;
    }

    *out = NULL;
    ERR("no interface for %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI DHTMLEdit_AddRef(IDHTMLEdit *iface)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI DHTMLEdit_Release(IDHTMLEdit *iface)
{
    DHTMLEditImpl *This = impl_from_IDHTMLEdit(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
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

HRESULT dhtml_edit_create(REFIID iid, void **out)
{
    DHTMLEditImpl *This;
    HRESULT ret;

    TRACE("(%s, %p)\n", debugstr_guid(iid), out);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
        return E_OUTOFMEMORY;

    This->IDHTMLEdit_iface.lpVtbl = &DHTMLEditVtbl;
    This->ref = 1;

    ret = IDHTMLEdit_QueryInterface(&This->IDHTMLEdit_iface, iid, out);
    IDHTMLEdit_Release(&This->IDHTMLEdit_iface);
    return ret;
}
