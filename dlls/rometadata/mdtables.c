/*
 * IMetaDataTables, IMetaDataImport implementation
 *
 * Copyright 2025 Vibhav Pant
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

#include <assert.h>

#define COBJMACROS
#include "objbase.h"
#include "cor.h"
#include "rometadataapi.h"
#include "wine/debug.h"

#include "rometadatapriv.h"

WINE_DEFAULT_DEBUG_CHANNEL(rometadata);

struct metadata_tables
{
    IMetaDataTables IMetaDataTables_iface;
    IMetaDataImport IMetaDataImport_iface;
    LONG ref;

    assembly_t *assembly;
};

static inline struct metadata_tables *impl_from_IMetaDataTables(IMetaDataTables *iface)
{
    return CONTAINING_RECORD(iface, struct metadata_tables, IMetaDataTables_iface);
}

static HRESULT WINAPI tables_QueryInterface(IMetaDataTables *iface, REFIID iid, void **out)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), out);

    if (IsEqualGUID(&IID_IUnknown, iid) || IsEqualGUID(&IID_IMetaDataTables, iid))
    {
        IMetaDataTables_AddRef((*out = iface));
        return S_OK;
    }
    if (IsEqualGUID(&IID_IMetaDataImport, iid))
    {
        IMetaDataImport_AddRef((*out = &impl->IMetaDataImport_iface));
        return S_OK;
    }

    FIXME("%s not implemented, returning E_NOINTERFACE.\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI tables_AddRef(IMetaDataTables *iface)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    ULONG ref = InterlockedIncrement(&impl->ref);

    TRACE("(%p)\n", impl);

    return ref;
}

static ULONG WINAPI tables_Release(IMetaDataTables *iface)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    ULONG ref = InterlockedDecrement(&impl->ref);

    TRACE("(%p)\n", iface);

    if (!ref)
    {
        assembly_free(impl->assembly);
        free(impl);
    }
    return ref;
}

static HRESULT get_heap_size(IMetaDataTables *iface, enum heap_type type, ULONG *size)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    *size = assembly_get_heap_size(impl->assembly, type);
    return S_OK;
}

static HRESULT WINAPI tables_GetStringHeapSize(IMetaDataTables *iface, ULONG *size)
{
    TRACE("(%p, %p)\n", iface, size);
    return get_heap_size(iface, HEAP_STRING, size);
}

static HRESULT WINAPI tables_GetBlobHeapSize(IMetaDataTables *iface, ULONG *size)
{
    TRACE("(%p, %p)\n", iface, size);
    return get_heap_size(iface, HEAP_BLOB, size);
}

static HRESULT WINAPI tables_GetGuidHeapSize(IMetaDataTables *iface, ULONG *size)
{
    TRACE("(%p, %p)\n", iface, size);
    return get_heap_size(iface, HEAP_GUID, size);
}

static HRESULT WINAPI tables_GetUserStringHeapSize(IMetaDataTables *iface, ULONG *size)
{
    TRACE("(%p, %p)\n", iface, size);
    return get_heap_size(iface, HEAP_USER_STRING, size);
}

static HRESULT WINAPI tables_GetNumTables(IMetaDataTables *iface, ULONG *size)
{
    TRACE("(%p, %p)\n", iface, size);

    /* This returns the number of tables known to the metadata parser, not the number of tables that exist in this
     * assembly. */
    *size = 45;
    return S_OK;
}

static HRESULT WINAPI tables_GetTableIndex(IMetaDataTables *iface, ULONG token, ULONG *idx)
{
    FIXME("(%p %lu %p): stub!\n", iface, token, idx);
    return E_NOTIMPL;
}

static HRESULT WINAPI tables_GetTableInfo(IMetaDataTables *iface, ULONG idx_tbl, ULONG *row_size, ULONG *num_rows,
                                          ULONG *num_cols, ULONG *idx_key, const char **name)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    struct metadata_table_info table;
    HRESULT hr;

    TRACE("(%p, %lu, %p, %p, %p, %p, %p)\n", iface, idx_tbl, row_size, num_rows, num_cols, idx_key, name);

    if (FAILED(hr = assembly_get_table(impl->assembly, idx_tbl, &table))) return hr;

    *row_size = table.row_size;
    *num_rows = table.num_rows;
    *num_cols = table.num_columns;
    *idx_key = table.key_idx;
    *name = table.name;
    return S_OK;
}

static HRESULT WINAPI tables_GetColumnInfo(IMetaDataTables *iface, ULONG idx_tbl, ULONG idx_col, ULONG *offset,
                                           ULONG *col_size, ULONG *type, const char **name)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    struct metadata_column_info column;
    HRESULT hr;

    TRACE("(%p, %lu, %lu, %p, %p, %p, %p)\n", iface, idx_tbl, idx_col, offset, col_size, type, name);

    if (FAILED(hr = assembly_get_column(impl->assembly, idx_tbl, idx_col, &column))) return hr;

    *offset = column.offset;
    *col_size = column.size;
    *type = column.type;
    *name = column.name;
    return S_OK;
}

static HRESULT WINAPI tables_GetCodedTokenInfo(IMetaDataTables *iface, ULONG type, ULONG *tokens_len,
                                               const ULONG **tokens, const char **name)
{
    FIXME("(%p, %lu, %p, %p, %p) stub!\n", iface, type, tokens_len, tokens, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI tables_GetRow(IMetaDataTables *iface, ULONG idx_tbl, ULONG idx_row, const BYTE *row)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    struct metadata_table_info table;
    HRESULT hr;

    TRACE("(%p, %lu, %lu, %p)\n", iface, idx_tbl, idx_row, row);

    if (FAILED(hr = assembly_get_table(impl->assembly, idx_tbl, &table))) return hr;

    assert(table.start);
    idx_row--; /* Row indices are 1-based. */
    if (idx_row >= table.num_rows) return E_INVALIDARG;
    *(const BYTE **)row = table.start + (size_t)(table.row_size * idx_row);
    return S_OK;
}

static HRESULT WINAPI tables_GetColumn(IMetaDataTables *iface, ULONG idx_tbl, ULONG idx_col, ULONG idx_row, ULONG *val)
{
    ULONG raw_val = 0, offset, size, type;
    const BYTE *row = NULL;
    const char *name;
    HRESULT hr;

    TRACE("(%p, %lu, %lu, %lu, %p)\n", iface, idx_tbl, idx_col, idx_row, val);

    if (FAILED(hr = IMetaDataTables_GetRow(iface, idx_tbl, idx_row, (BYTE *)&row))) return hr;
    if (FAILED(hr = IMetaDataTables_GetColumnInfo(iface, idx_tbl, idx_col, &offset, &size, &type, &name))) return hr;

    memcpy(&raw_val, row + offset, size);
    if (type >= 64 && type <= 95) /* IsCodedTokenType */
        raw_val = metadata_coded_value_as_token(idx_tbl, idx_col, raw_val);
    *val = raw_val;
    return S_OK;
}

static HRESULT WINAPI tables_GetString(IMetaDataTables *iface, ULONG idx, const char **str)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    TRACE("(%p, %lu, %p)\n", iface, idx, str);
    return (*str = assembly_get_string(impl->assembly, idx)) ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI tables_GetBlob(IMetaDataTables *iface, ULONG idx, ULONG *size, const BYTE **blob)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    TRACE("(%p, %lu, %p, %p)\n", iface, idx, size, blob);
    return assembly_get_blob(impl->assembly, idx, blob, size);
}

static HRESULT WINAPI tables_GetGuid(IMetaDataTables *iface, ULONG idx, const GUID **guid)
{
    struct metadata_tables *impl = impl_from_IMetaDataTables(iface);
    TRACE("(%p, %lu, %p)!\n", iface, idx, guid);
    return (*guid = assembly_get_guid(impl->assembly, idx)) ? S_OK : E_INVALIDARG;
}

static HRESULT WINAPI tables_GetUserString(IMetaDataTables *iface, ULONG idx, ULONG *size, const BYTE **string)
{
    FIXME("%p %lu %p %p stub!\n", iface, idx, size, string);
    return E_NOTIMPL;
}

static HRESULT WINAPI tables_GetNextString(IMetaDataTables *iface, ULONG idx, ULONG *next)
{
    FIXME("(%p, %lu, %p): stub!\n", iface, idx, next);
    return E_NOTIMPL;
}

static HRESULT WINAPI tables_GetNextBlob(IMetaDataTables *iface, ULONG idx, ULONG *next)
{
    FIXME("(%p, %lu, %p): stub!\n", iface, idx, next);
    return E_NOTIMPL;
}

static HRESULT WINAPI tables_GetNextGuid(IMetaDataTables *iface, ULONG idx, ULONG *next)
{
    FIXME("(%p, %lu, %p): stub!\n", iface, idx, next);
    return E_NOTIMPL;
}

static HRESULT WINAPI tables_GetNextUserString(IMetaDataTables *iface, ULONG idx, ULONG *next)
{
    FIXME("(%p, %lu, %p): stub!\n", iface, idx, next);
    return E_NOTIMPL;
}

static const struct IMetaDataTablesVtbl tables_vtbl =
{
    tables_QueryInterface,
    tables_AddRef,
    tables_Release,
    tables_GetStringHeapSize,
    tables_GetBlobHeapSize,
    tables_GetGuidHeapSize,
    tables_GetUserStringHeapSize,
    tables_GetNumTables,
    tables_GetTableIndex,
    tables_GetTableInfo,
    tables_GetColumnInfo,
    tables_GetCodedTokenInfo,
    tables_GetRow,
    tables_GetColumn,
    tables_GetString,
    tables_GetBlob,
    tables_GetGuid,
    tables_GetUserString,
    tables_GetNextString,
    tables_GetNextBlob,
    tables_GetNextGuid,
    tables_GetNextUserString,
};

static inline struct metadata_tables *impl_from_IMetaDataImport(IMetaDataImport *iface)
{
    return CONTAINING_RECORD(iface, struct metadata_tables, IMetaDataImport_iface);
}

static HRESULT WINAPI import_QueryInterface(IMetaDataImport *iface, REFIID iid, void **out)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    return IMetaDataTables_QueryInterface(&impl->IMetaDataTables_iface, iid, out);
}

static ULONG WINAPI import_AddRef(IMetaDataImport *iface)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    return IMetaDataTables_AddRef(&impl->IMetaDataTables_iface);
}

static ULONG WINAPI import_Release(IMetaDataImport *iface)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    return IMetaDataTables_Release(&impl->IMetaDataTables_iface);
}

static void WINAPI import_CloseEnum(IMetaDataImport *iface, HCORENUM henum)
{
    FIXME("(%p, %p): stub!\n", iface, henum);
}

static HRESULT WINAPI import_CountEnum(IMetaDataImport *iface, HCORENUM henum, ULONG *count)
{
    FIXME("(%p, %p, %p): stub!\n", iface, henum, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_ResetEnum(IMetaDataImport *iface, HCORENUM henum, ULONG idx)
{
    FIXME("(%p, %p, %lu): stub\n", iface, henum, idx);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumTypeDefs(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef *typedefs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, typedefs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumInterfaceImpls(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def,
                                                mdInterfaceImpl *impls, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, type_def, impls, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumTypeRefs(IMetaDataImport *iface, HCORENUM *henum, mdTypeRef *typerefs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, typerefs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_FindTypeDefByName(IMetaDataImport *iface, const WCHAR *name, mdToken enclosing_class, mdTypeDef *type_def)
{
    FIXME("(%p, %p, %#x, %p): stub!\n", iface, debugstr_w(name), enclosing_class, type_def);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetScopeProps(IMetaDataImport *iface, WCHAR *name, ULONG len, ULONG *written, GUID *mvid)
{
    FIXME("(%p, %p, %lu, %p, %p): stub!\n", iface, name, len, written, mvid);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetModuleFromScope(IMetaDataImport *iface, mdModule *module)
{
    FIXME("(%p, %p): stub!\n", iface, module);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetTypeDefProps(IMetaDataImport *iface, mdTypeDef type_def, WCHAR *name, ULONG len,
                                             ULONG *written, ULONG *flags, mdToken *base)
{
    FIXME("(%p, %#x, %p, %lu, %p, %p, %p): stub!\n", iface, type_def, name, len, written, flags, base);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetInterfaceImplProps(IMetaDataImport *iface, mdInterfaceImpl iface_impl,
                                                   mdTypeDef *class_token, mdToken *iface_token)
{
    FIXME("(%p, %#x, %p, %p): stub!\n", iface, iface_impl, class_token, iface_token);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetTypeRefProps(IMetaDataImport *iface, mdTypeRef typeref, mdToken *resolution_scope,
                                             WCHAR *name, ULONG len, ULONG *written)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p): stub!\n", iface, typeref, resolution_scope, name, len, written);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_ResolveTypeRef(IMetaDataImport *iface, mdTypeRef typeref, const GUID *iid,
                                            IUnknown **scope, mdTypeDef *type_def)
{
    FIXME("(%p, %#x, %s, %p, %p): stub!\n", iface, typeref, debugstr_guid(iid), scope, type_def);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMembers(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def,
                                         mdToken *member_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, type_def, member_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMembersWithName(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef token,
                                                 const WCHAR *name, mdToken *member_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %s, %p, %lu, %p): stub!\n", iface, henum, token, debugstr_w(name), member_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMethods(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def,
                                         mdMethodDef *method_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, type_def, method_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMethodsWithName(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def,
                                                 const WCHAR *name, mdMethodDef *method_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %s, %p, %lu, %p): stub!\n", iface, henum, type_def, debugstr_w(name), method_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumFields(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef token,
                                        mdFieldDef *field_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, token, field_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumFieldsWithName(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef token,
                                                const WCHAR *name, mdFieldDef *field_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %s, %p, %lu, %p): stub!\n", iface, henum, token, debugstr_w(name), field_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumParams(IMetaDataImport *iface, HCORENUM *henum, mdMethodDef method_def,
                                        mdParamDef *params, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, method_def, params, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMemberRefs(IMetaDataImport *iface, HCORENUM *henum, mdToken parent,
                                            mdMemberRef *members, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, parent, members, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMethodImpls(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef token,
                                             mdToken *body_tokens, mdToken *decl_tokens, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %p, %lu, %p): stub!\n", iface, henum, token, body_tokens, decl_tokens, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumPermissionSets(IMetaDataImport *iface, HCORENUM *henum, mdToken token, ULONG actions,
                                                mdPermission *perms, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %#lx, %p, %lu, %p): stub!\n", iface, henum, token, actions, perms, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_FindMember(IMetaDataImport *iface, mdTypeDef type_def, const WCHAR *name,
                                        const COR_SIGNATURE *sig_blob, ULONG len, mdToken *member_ref)
{
    FIXME("(%p, %#x, %s, %p, %lu, %p): stub!\n", iface, type_def, debugstr_w(name), sig_blob, len, member_ref);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_FindMethod(IMetaDataImport *iface, mdTypeDef type_def, const WCHAR *name,
                                        const COR_SIGNATURE *sig_blob, ULONG len, mdMethodDef *method_def)
{
    FIXME("(%p, %#x, %s, %p, %lu, %p): stub!\n", iface, type_def, debugstr_w(name), sig_blob, len, method_def);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_FindField(IMetaDataImport *iface, mdTypeDef type_def, const WCHAR *name,
                                       const COR_SIGNATURE *sig_blob, ULONG len, mdFieldDef *field_def)
{
    FIXME("(%p, %#x, %s, %p, %lu, %p): stub!\n", iface, type_def, debugstr_w(name), sig_blob, len, field_def);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_FindMemberRef(IMetaDataImport *iface, mdTypeRef typeref, const WCHAR *name,
                                           const COR_SIGNATURE *sig_blob, ULONG len, mdMemberRef *member_ref)
{
    FIXME("(%p, %#x, %s, %p, %lu, %p): stub!\n", iface, typeref, debugstr_w(name), sig_blob, len, member_ref);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetMethodProps(IMetaDataImport *iface, mdMethodDef method_def, mdTypeDef *type_def,
                                            WCHAR *method_name, ULONG name_len, ULONG *written, ULONG *method_flags,
                                            const COR_SIGNATURE **sig_blob, ULONG *sig_len, ULONG *rva,
                                            ULONG *impl_flags)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p, %p, %p, %p, %p): stub!\n", iface, method_def, type_def, method_name, name_len,
          written, method_flags, sig_blob, sig_len, rva, impl_flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetMemberRefProps(IMetaDataImport *iface, mdMemberRef member_ref, mdToken *token,
                                               WCHAR *member_name, ULONG name_len, ULONG *written,
                                               const COR_SIGNATURE **sig_blob, ULONG *sig_len)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p, %p): stub!\n", iface, member_ref, token, member_name, name_len, written,
          sig_blob, sig_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumProperties(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def,
                                            mdProperty *props, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, type_def, props, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumEvents(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def, mdEvent *events,
                                        ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, type_def, events, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetEventProps(IMetaDataImport *iface, mdEvent event, mdTypeDef *class_typedef, WCHAR *name,
                                           ULONG name_len, ULONG *written, ULONG *event_flags, mdToken *event_type,
                                           mdMethodDef *add_method, mdMethodDef *remove_method,
                                           mdMethodDef *fire_method, mdMethodDef *other_methods, ULONG other_len,
                                           ULONG *other_count)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p, %p, %p, %p, %p, %p, %lu, %p): stub!\n", iface, event, class_typedef, name,
          name_len, written, event_flags, event_type, add_method, remove_method, fire_method, other_methods, other_len,
          other_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumMethodSemantics(IMetaDataImport *iface, HCORENUM *henum, mdMethodDef method_def,
                                                 mdToken *event_props, ULONG len, ULONG *written)
{
    FIXME("(%p, %p, %#x, %p, %lu, %p): stub!\n", iface, henum, method_def, event_props, len, written);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetMethodSemantics(IMetaDataImport *iface, mdMethodDef method_def, mdToken event_prop,
                                                ULONG *semantics)
{
    FIXME("(%p, %#x, %#x, %p): stub!\n", iface, method_def, event_prop, semantics);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetClassLayout(IMetaDataImport *iface, mdTypeDef type_def, ULONG *pack_size,
                                            COR_FIELD_OFFSET *offsets, ULONG len, ULONG *count, ULONG *class_size)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p): stub!\n", iface, type_def, pack_size, offsets, len, count, class_size);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetFieldMarshal(IMetaDataImport *iface, mdToken token,
                                             const COR_SIGNATURE **native_type_blob, ULONG *len)
{
    FIXME("(%p, %#x, %p, %p): stub!\n", iface, token, native_type_blob, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetRVA(IMetaDataImport *iface, mdToken token, ULONG *rva, ULONG *impl_flags)
{
    FIXME("(%p, %#x, %p, %p): stub!\n", iface, token, rva, impl_flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetPermissionSetProps(IMetaDataImport *iface, mdPermission perm, ULONG *decl_sec,
                                                   const BYTE **perm_blob, ULONG *len)
{
    FIXME("(%p, %#x, %p, %p, %p): stub!\n", iface, perm, decl_sec, perm_blob, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetSigFromToken(IMetaDataImport *iface, mdSignature signature,
                                             const COR_SIGNATURE **sig_blob, ULONG *len)
{
    FIXME("(%p, %#x, %p, %p): stub!\n", iface, signature, sig_blob, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetModuleRefProps(IMetaDataImport *iface, mdModuleRef module_ref, WCHAR *name, ULONG len,
                                               ULONG *written)
{
    FIXME("(%p, %#x, %p, %lu, %p): stub!\n", iface, module_ref, name, len, written);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumModuleRefs(IMetaDataImport *iface, HCORENUM *henum, mdModuleRef *module_refs, ULONG len,
                                            ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, module_refs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetTypeSpecFromToken(IMetaDataImport *iface, mdTypeSpec typespec,
                                                  const COR_SIGNATURE **sig_blob, ULONG *len)
{
    FIXME("(%p, %#x, %p, %p): stub!\n", iface, typespec, sig_blob, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetNameFromToken(IMetaDataImport *iface, mdToken token, const char **name)
{
    FIXME("(%p, %#x, %p): stub!\n", iface, token, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumUnresolvedMethods(IMetaDataImport *iface, HCORENUM *henum, mdToken *method_defs,
                                                   ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, method_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetUserString(IMetaDataImport *iface, mdString string_token, WCHAR *str, ULONG len,
                                           ULONG *written)
{
    FIXME("(%p, %#x, %p, %lu, %p): stub!\n", iface, string_token, str, len, written);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetPinvokeMap(IMetaDataImport *iface, mdToken field_method_def, ULONG *map_flags,
                                           WCHAR *import_name, ULONG len, ULONG *written, mdModuleRef *target)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p): stub!\n", iface, field_method_def, map_flags, import_name, len, written, target);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumSignatures(IMetaDataImport *iface, HCORENUM *henum, mdSignature *sigs, ULONG len,
                                            ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, sigs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumTypeSpecs(IMetaDataImport *iface, HCORENUM *henum, mdTypeSpec *typespecs, ULONG len,
                                           ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, typespecs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumUserStrings(IMetaDataImport *iface, HCORENUM *henum, mdString *strings, ULONG len,
                                             ULONG *count)
{
    FIXME("(%p, %p, %p, %lu, %p): stub!\n", iface, henum, strings, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetParamForMethodIndex(IMetaDataImport *iface, mdMethodDef token, ULONG param_seq,
                                                    mdParamDef *param_def)
{
    FIXME("(%p, %#x, %lu, %p): stub!\n", iface, token, param_seq, param_def);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumCustomAttributes(IMetaDataImport *iface, HCORENUM *henum, mdToken token,
                                                  mdToken token_type, mdCustomAttribute *attrs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %#x, %p, %lu, %p): stub!\n", iface, henum, token, token_type, attrs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetCustomAttributeProps(IMetaDataImport *iface, mdCustomAttribute custom_attr, mdToken *obj,
                                                     mdToken *attr_type, const BYTE **blob, ULONG *len)
{
    FIXME("(%p, %#x, %p, %p, %p, %p): stub!\n", iface, custom_attr, obj, attr_type, blob, len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_FindTypeRef(IMetaDataImport *iface, mdToken resolution_scope, const WCHAR *name,
                                         mdTypeRef *typeref)
{
    FIXME("(%p, %#x, %s, %p): stub!\n", iface, resolution_scope, debugstr_w(name), typeref);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetMemberProps(IMetaDataImport *iface, mdToken member, mdTypeDef *type_def, WCHAR *name,
                                            ULONG member_len, ULONG *member_written, ULONG *flags,
                                            const COR_SIGNATURE **sig_blob, ULONG *blob_len, ULONG *rva,
                                            ULONG *impl_flags, ULONG *value_type_flag, UVCP_CONSTANT *value,
                                            ULONG *value_len)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p, %p, %p, %p, %p, %p, %p, %p): stub!\n", iface, member, type_def, name,
          member_len, member_written, flags, sig_blob, blob_len, rva, impl_flags, value_type_flag, value, value_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetFieldProps(IMetaDataImport *iface, mdFieldDef fielddef, mdTypeDef *type_def,
                                           WCHAR *field_name, ULONG name_len, ULONG *name_written, ULONG *flags,
                                           const COR_SIGNATURE **sig_blob, ULONG *sig_len, ULONG *value_type_flag,
                                           UVCP_CONSTANT *value, ULONG *value_len)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p, %p, %p, %p, %p, %p): stub!\n", iface, fielddef, type_def, field_name,
          name_len, name_written, flags, sig_blob, sig_len, value_type_flag, value, value_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetPropertyProps(IMetaDataImport *iface, mdProperty prop, mdTypeDef *type_def, WCHAR *name,
                                              ULONG name_len, ULONG *name_written, ULONG *prop_flags,
                                              const COR_SIGNATURE **sig_blob, ULONG *sig_len, ULONG *value_type_flag,
                                              UVCP_CONSTANT *default_value, ULONG *value_len, mdMethodDef *set_method,
                                              mdMethodDef *get_method, mdMethodDef *other_methods, ULONG other_len,
                                              ULONG *other_count)
{
    FIXME("(%p, %#x, %p, %p, %lu, %p, %p, %p, %p, %p, %p, %p, %p, %p, %p, %lu, %p): stub!\n", iface, prop, type_def,
          name, name_len, name_written, prop_flags, sig_blob, sig_len, value_type_flag, default_value, value_len,
          set_method, get_method, other_methods, other_len, other_count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetParamProps(IMetaDataImport *iface, mdParamDef param_def, mdMethodDef *method_def,
                                           ULONG *param_seq, WCHAR *name, ULONG name_len, ULONG *name_written,
                                           ULONG *flags, ULONG *value_type_flag, UVCP_CONSTANT *value, ULONG *value_len)
{
    FIXME("(%p, %#x, %p, %p, %p, %lu, %p, %p, %p, %p, %p): stub!\n", iface, param_def, method_def, param_seq, name,
          name_len, name_written, flags, value_type_flag, value, value_len);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetCustomAttributeByName(IMetaDataImport *iface, mdToken obj, const WCHAR *name,
                                                      const BYTE **data, ULONG *len)
{
    FIXME("(%p, %#x, %s, %p, %p): stub!\n", iface, obj, debugstr_w(name), data, len);
    return E_NOTIMPL;
}

static BOOL WINAPI import_IsValidToken(IMetaDataImport *iface, mdToken token)
{
    FIXME("(%p, %#x): stub!\n", iface, token);
    return FALSE;
}

static HRESULT WINAPI import_GetNestedClassProps(IMetaDataImport *iface, mdTypeDef nested_class,
                                                 mdTypeDef *enclosing_class)
{
    FIXME("(%p, %#x, %p): stub!\n", iface, nested_class, enclosing_class);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_GetNativeCallConvFromSig(IMetaDataImport *iface, const BYTE *sig_blob, ULONG len,
                                                      ULONG *call_conv)
{
    FIXME("(%p, %p, %lu, %p): stub!\n", iface, sig_blob, len, call_conv);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_IsGlobal(IMetaDataImport *iface, mdToken token, int *is_global)
{
    FIXME("(%p, %#x, %p): stub!\n", iface, token, is_global);
    return E_NOTIMPL;
}

static const IMetaDataImportVtbl import_vtbl = {
    import_QueryInterface,
    import_AddRef,
    import_Release,
    import_CloseEnum,
    import_CountEnum,
    import_ResetEnum,
    import_EnumTypeDefs,
    import_EnumInterfaceImpls,
    import_EnumTypeRefs,
    import_FindTypeDefByName,
    import_GetScopeProps,
    import_GetModuleFromScope,
    import_GetTypeDefProps,
    import_GetInterfaceImplProps,
    import_GetTypeRefProps,
    import_ResolveTypeRef,
    import_EnumMembers,
    import_EnumMembersWithName,
    import_EnumMethods,
    import_EnumMethodsWithName,
    import_EnumFields,
    import_EnumFieldsWithName,
    import_EnumParams,
    import_EnumMemberRefs,
    import_EnumMethodImpls,
    import_EnumPermissionSets,
    import_FindMember,
    import_FindMethod,
    import_FindField,
    import_FindMemberRef,
    import_GetMethodProps,
    import_GetMemberRefProps,
    import_EnumProperties,
    import_EnumEvents,
    import_GetEventProps,
    import_EnumMethodSemantics,
    import_GetMethodSemantics,
    import_GetClassLayout,
    import_GetFieldMarshal,
    import_GetRVA,
    import_GetPermissionSetProps,
    import_GetSigFromToken,
    import_GetModuleRefProps,
    import_EnumModuleRefs,
    import_GetTypeSpecFromToken,
    import_GetNameFromToken,
    import_EnumUnresolvedMethods,
    import_GetUserString,
    import_GetPinvokeMap,
    import_EnumSignatures,
    import_EnumTypeSpecs,
    import_EnumUserStrings,
    import_GetParamForMethodIndex,
    import_EnumCustomAttributes,
    import_GetCustomAttributeProps,
    import_FindTypeRef,
    import_GetMemberProps,
    import_GetFieldProps,
    import_GetPropertyProps,
    import_GetParamProps,
    import_GetCustomAttributeByName,
    import_IsValidToken,
    import_GetNestedClassProps,
    import_GetNativeCallConvFromSig,
    import_IsGlobal
};

HRESULT IMetaDataTables_create(const WCHAR *path, IMetaDataTables **iface)
{
    struct metadata_tables *impl;
    HRESULT hr;

    if (!(impl = calloc(1, sizeof(*impl)))) return E_OUTOFMEMORY;
    if (FAILED(hr = assembly_open_from_file(path, &impl->assembly)))
    {
        free( impl );
        return hr;
    }

    impl->IMetaDataTables_iface.lpVtbl = &tables_vtbl;
    impl->IMetaDataImport_iface.lpVtbl = &import_vtbl;
    impl->ref = 1;
    *iface = &impl->IMetaDataTables_iface;
    return S_OK;
}
