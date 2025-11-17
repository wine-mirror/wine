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
#include <stdarg.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
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

struct token_enum
{
    ULONG count;
    ULONG row_start;
    ULONG row_cur;
    ULONG row_end;
};

HRESULT token_enum_create(HCORENUM *out)
{
    struct token_enum *md_enum;

    if (!(md_enum = calloc(1, sizeof(*md_enum)))) return E_OUTOFMEMORY;
    *out = md_enum;
    return S_OK;
}

static HRESULT token_enum_get_entries(struct token_enum *henum, CorTokenType type, mdToken *buf, ULONG buf_len,
                                      ULONG *buf_written)
{
    ULONG i = 0;

    while (henum->row_cur <= henum->row_end && i < buf_len)
        buf[i++] = TokenFromRid(henum->row_cur++, type);
    if (buf_written)
        *buf_written = i;
    return !!i ? S_OK : S_FALSE;
}

static void WINAPI import_CloseEnum(IMetaDataImport *iface, HCORENUM henum)
{
    TRACE("(%p, %p)\n", iface, henum);
    free(henum);
}

static HRESULT WINAPI import_CountEnum(IMetaDataImport *iface, HCORENUM henum, ULONG *count)
{
    const struct token_enum *md_enum = henum;

    TRACE("(%p, %p, %p)\n", iface, henum, count);

    *count = henum ? md_enum->count : 0;
    return S_OK;
}

static HRESULT WINAPI import_ResetEnum(IMetaDataImport *iface, HCORENUM henum, ULONG idx)
{
    struct token_enum *md_enum = henum;

    TRACE("(%p, %p, %lu)\n", iface, henum, idx);

    if (henum)
        md_enum->row_cur = md_enum->row_start + idx;
    return S_OK;
}

static const char *debugstr_mdToken(mdToken token)
{
#define TYPE(t) { t, #t }
    static const struct type_str
    {
        CorTokenType type;
        const char *str;
    } types[] = {
        TYPE(mdtModule),
        TYPE(mdtTypeRef),
        TYPE(mdtTypeDef),
        TYPE(mdtFieldDef),
        TYPE(mdtMethodDef),
        TYPE(mdtParamDef),
        TYPE(mdtInterfaceImpl),
        TYPE(mdtMemberRef),
        TYPE(mdtCustomAttribute),
        TYPE(mdtPermission),
        TYPE(mdtSignature),
        TYPE(mdtEvent),
        TYPE(mdtProperty),
        TYPE(mdtModuleRef),
        TYPE(mdtTypeSpec),
        TYPE(mdtAssembly),
        TYPE(mdtAssemblyRef),
        TYPE(mdtFile),
        TYPE(mdtExportedType),
        TYPE(mdtManifestResource),
        TYPE(mdtGenericParam),
        TYPE(mdtMethodSpec),
        TYPE(mdtGenericParamConstraint),
        TYPE(mdtString),
        TYPE(mdtName),
        TYPE(mdtBaseType)
    };
#undef TYPE
    const CorTokenType type = TypeFromToken(token);
    const UINT rid = RidFromToken(token);
    int i;

    if (!token) return "(mdTokenNil)";
    for (i = 0; i < ARRAY_SIZE(types); i++)
    {
        if (type == types[i].type)
            return rid ? wine_dbg_sprintf("(%s|%#x)", types[i].str, rid)
                       : wine_dbg_sprintf("(md%sNil)", &types[i].str[3]);
    }
    return wine_dbg_sprintf("(%#x|%#x)", type, rid);
}

static HRESULT table_get_num_rows(IMetaDataTables *iface, enum table table, ULONG *rows)
{
    ULONG row_size, cols, key_idx;
    const char *name;

    return IMetaDataTables_GetTableInfo(iface, table, &row_size, rows, &cols, &key_idx, &name);
}

static HRESULT WINAPI import_EnumTypeDefs(IMetaDataImport *iface, HCORENUM *ret_henum, mdTypeDef *typedefs, ULONG len, ULONG *count)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    struct token_enum *henum = *ret_henum;
    ULONG rows;
    HRESULT hr;

    TRACE("(%p, %p, %p, %lu, %p)\n", iface, ret_henum, typedefs, len, count);

    if (count)
        *count = 0;

    if (!henum)
    {
        hr = table_get_num_rows(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, &rows);
        if (FAILED(hr)) return hr;
        /* Skip the <Module> row. */
        if (rows < 2) return S_FALSE;
        if (FAILED((hr = token_enum_create((HCORENUM *)&henum)))) return hr;

        henum->count = rows - 1;
        henum->row_start = henum->row_cur = 2;
        henum->row_end = rows;
        *ret_henum = henum;
    }

    return token_enum_get_entries(henum, mdtTypeDef, typedefs, len, count);
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

static void type_split_name(const char *nameA, const char **namespace, size_t *ns_len, const char **type_name)
{
    const char *sep;

    if ((sep = strrchr(nameA, '.')))
    {
        *namespace = nameA;
        *ns_len = sep - nameA;
        *type_name = &sep[1];
    }
    else
    {
        *namespace = NULL;
        *ns_len = 0;
        *type_name = nameA;
    }
}

static HRESULT WINAPI import_FindTypeDefByName(IMetaDataImport *iface, const WCHAR *name, mdToken enclosing_class,
                                               mdTypeDef *type_def)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    const char *namespace, *type_name;
    HCORENUM henum = NULL;
    mdTypeDef cur_typedef;
    size_t len, ns_len;
    BOOL found = FALSE;
    char *nameA;
    HRESULT hr;

    TRACE("(%p, %s, %s, %p)\n", iface, debugstr_w(name), debugstr_mdToken(enclosing_class), type_def);

    if (!name) return E_INVALIDARG;
    if (!IsNilToken(enclosing_class) && enclosing_class != TokenFromRid(1, mdtModule))
    {
        CorTokenType type = TypeFromToken(enclosing_class);

        if (type == mdtTypeDef || type == mdtTypeRef)
            FIXME("unsupported enclosing_class: %s\n", debugstr_mdToken(enclosing_class));
        return CLDB_E_RECORD_NOTFOUND;
    }

    len = WideCharToMultiByte(CP_ACP, 0, name, -1, NULL, 0, NULL, NULL);
    if (!(nameA = malloc(len))) return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, name, -1, nameA, len, NULL, NULL);
    type_split_name(nameA, &namespace, &ns_len, &type_name);

    hr = IMetaDataImport_EnumTypeDefs(&impl->IMetaDataImport_iface, &henum, NULL, 0, NULL);
    if (FAILED(hr))
    {
        free(nameA);
        return hr;
    }
    hr = IMetaDataImport_EnumTypeDefs(&impl->IMetaDataImport_iface, &henum, &cur_typedef, 1, NULL);
    while (hr == S_OK)
    {
        ULONG row = RidFromToken(cur_typedef), idx;
        const char *cur_type_name, *cur_ns;

        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, 1, row, &idx)))) break;
        if (FAILED((hr = IMetaDataTables_GetString(&impl->IMetaDataTables_iface, idx, &cur_type_name)))) break;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, 2, row, &idx)))) break;
        if (FAILED((hr = IMetaDataTables_GetString(&impl->IMetaDataTables_iface, idx, &cur_ns)))) break;

        if (!!namespace == !!cur_ns[0] &&
            (!namespace || !strncmp(namespace, cur_ns, ns_len)) &&
            !strcmp(type_name, cur_type_name))
        {
            found = TRUE;
            *type_def = cur_typedef;
            break;
        }
        hr = IMetaDataImport_EnumTypeDefs(&impl->IMetaDataImport_iface, &henum, &cur_typedef, 1, NULL);
    }
    IMetaDataImport_CloseEnum(&impl->IMetaDataImport_iface, henum);
    free(nameA);

    if (found)
        hr = S_OK;
    else if (SUCCEEDED(hr))
        hr = CLDB_E_RECORD_NOTFOUND;
    return hr;
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

static BOOL type_full_name(WCHAR *out, ULONG out_len, ULONG *needed, const char *namespace, const char *name)
{
    ULONG full_name_len;
    char *full_name;

    if (namespace[0] && name[0])
        full_name_len = strlen(namespace) + 1 + strlen(name) + 1;
    else
        full_name_len = strlen(namespace[0] ? namespace : name) + 1;
    if (!(full_name = malloc(full_name_len))) return FALSE;
    if (namespace[0] && name[0])
        snprintf(full_name, full_name_len, "%s.%s", namespace, name);
    else
        strcpy(full_name, namespace[0] ? namespace : name);
    *needed = MultiByteToWideChar(CP_ACP, 0, full_name, -1, NULL, 0);
    if (out && out_len)
    {
        ULONG n = MultiByteToWideChar(CP_ACP, 0, full_name, min(full_name_len, out_len) - 1, out, out_len - 1);
        out[n] = L'\0';
    }
    free(full_name);
    return TRUE;
}

static HRESULT WINAPI import_GetTypeDefProps(IMetaDataImport *iface, mdTypeDef type_def, WCHAR *name, ULONG len,
                                             ULONG *written, ULONG *ret_flags, mdToken *base)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    ULONG needed = 0, flags = 0, extends = 0;

    TRACE("(%p, %s, %p, %lu, %p, %p, %p)\n", iface, debugstr_mdToken(type_def), name, len, written, ret_flags, base);

    if (TypeFromToken(type_def) != mdtTypeDef) return S_FALSE;

    if (name && len)
        name[0] = L'\0';
    if (type_def != mdTypeDefNil)
    {
        const ULONG row = RidFromToken(type_def);
        const char *namespace, *nameA;
        ULONG name_idx, ns_idx;
        HRESULT hr;

        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, 0, row, &flags))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, 1, row, &name_idx))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, 2, row, &ns_idx))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, 3, row, &extends))))
            return hr;
        if (FAILED(hr = IMetaDataTables_GetString(&impl->IMetaDataTables_iface, name_idx, &nameA)))
            return hr;
        if (FAILED(hr = IMetaDataTables_GetString(&impl->IMetaDataTables_iface, ns_idx, &namespace)))
            return hr;
        if (!type_full_name(name, len, &needed, namespace, nameA)) return E_OUTOFMEMORY;
        /* Native replaces tokens with rid 0 with mdTypeRefNil. */
        if (IsNilToken(extends))
            extends = mdTypeRefNil;
    }

    if (written)
        *written = needed;
    if (ret_flags)
        *ret_flags = flags;
    if (base)
        *base = extends;

    return (name && needed > len) ? CLDB_S_TRUNCATION : S_OK;
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

static HRESULT table_create_enum_from_token_list(IMetaDataTables *iface, enum table table, enum table list_table,
                                                 ULONG row, ULONG col, HCORENUM *ret_henum)
{
    ULONG num_rows, row_start, row_end;
    struct token_enum *henum;
    HRESULT hr;

    if (FAILED((hr = table_get_num_rows(iface, table, &num_rows)))) return hr;
    if (row > num_rows) return S_FALSE;
    if (FAILED((hr = IMetaDataTables_GetColumn(iface, table, col, row, &row_start)))) return hr;
    row_start = RidFromToken(row_start);

    if (row != num_rows)
    {
        /* Use the next row to get the last token. */
        if (FAILED((hr = IMetaDataTables_GetColumn(iface, table, col, row + 1, &row_end)))) return hr;
        row_end = RidFromToken(row_end) - 1;
    }
    /* This is the last row, so the final token can be derived from the number of rows in list_table. */
    else if (FAILED((hr = table_get_num_rows(iface, list_table, &row_end))))
        return hr;

    if (FAILED((hr = token_enum_create((HCORENUM *)&henum)))) return hr;

    henum->count = row_end - row_start + 1;
    henum->row_start = henum->row_cur = row_start;
    henum->row_end = row_end;
    *ret_henum = henum;
    return S_OK;
}

static HRESULT WINAPI import_EnumMethods(IMetaDataImport *iface, HCORENUM *ret_henum, mdTypeDef type_def,
                                         mdMethodDef *method_defs, ULONG len, ULONG *count)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    struct token_enum *henum = *ret_henum;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p, %lu, %p)\n", iface, ret_henum, debugstr_mdToken(type_def), method_defs, len, count);

    if (count) *count = 0;
    if (TypeFromToken(type_def) != mdtTypeDef || IsNilToken(type_def)) return S_FALSE;
    if (!henum)
    {
        /* From Partition II.22.37, "TypeDef":
        *
        * The (MethodList) run continues to the smaller of:
        *  the last row of the MethodDef table
        *  the next run of Methods, found by inspecting the MethodList of the next row in this TypeDef table
        */
        hr = table_create_enum_from_token_list(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, TABLE_METHODDEF,
                                               RidFromToken(type_def), 5, ret_henum);
        if (hr != S_OK) return hr;
    }

    return token_enum_get_entries(*ret_henum, mdtMethodDef, method_defs, len, count);
}

static HRESULT WINAPI import_EnumMethodsWithName(IMetaDataImport *iface, HCORENUM *henum, mdTypeDef type_def,
                                                 const WCHAR *name, mdMethodDef *method_defs, ULONG len, ULONG *count)
{
    FIXME("(%p, %p, %#x, %s, %p, %lu, %p): stub!\n", iface, henum, type_def, debugstr_w(name), method_defs, len, count);
    return E_NOTIMPL;
}

static HRESULT WINAPI import_EnumFields(IMetaDataImport *iface, HCORENUM *ret_henum, mdTypeDef token,
                                        mdFieldDef *field_defs, ULONG len, ULONG *count)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    struct token_enum *henum = *ret_henum;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p, %lu, %p)\n", iface, ret_henum, debugstr_mdToken(token), field_defs, len, count);

    if (count) *count = 0;
    if (TypeFromToken(token) != mdtTypeDef || IsNilToken(token)) return S_FALSE;
    if (!henum)
    {
        /* From Partition II.22.37, "TypeDef":
        *
        * The (FieldList) run continues to the smaller of:
        *  the last row of the Field table
        *  the next run of Fields, found by inspecting the FieldList of the next row in this TypeDef table
        */
        hr = table_create_enum_from_token_list(&impl->IMetaDataTables_iface, TABLE_TYPEDEF, TABLE_FIELD,
                                               RidFromToken(token), 4, ret_henum);
        if (hr != S_OK) return hr;
    }

    return token_enum_get_entries(*ret_henum, mdtFieldDef, field_defs, len, count);
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


typedef HRESULT (*WINAPI token_enum_func)(IMetaDataImport *, HCORENUM *, mdTypeDef, mdToken *, ULONG, ULONG *);
static HRESULT token_get_parent_typedef(IMetaDataImport *iface, mdToken token, token_enum_func enum_func, mdTypeDef *parent)
{
    CorTokenType type = TypeFromToken(token);
    mdTypeDef cur_typedef = mdTypeDefNil;
    HCORENUM typedef_enum = NULL;
    HRESULT hr;

    TRACE("(%p, %s, %p, %p)\n", iface, debugstr_mdToken(token), enum_func, parent);

    assert(type == mdtMethodDef || type == mdtFieldDef);

    *parent = mdTypeDefNil;
    hr = IMetaDataImport_EnumTypeDefs(iface, &typedef_enum, &cur_typedef, 1, NULL);
    while (hr == S_OK)
    {
        HCORENUM token_enum = NULL;
        mdMethodDef first, last;
        ULONG count;

        hr = enum_func(iface, &token_enum, cur_typedef, &first, 1, NULL);
        if (FAILED(hr)) break;
        hr = IMetaDataImport_CountEnum(iface, token_enum, &count);
        IMetaDataImport_CloseEnum(iface, token_enum);
        if (FAILED(hr)) break;
        if (count)
        {
            /* Token lists/runs are strictly sequential */
            last = TokenFromRid(RidFromToken(first) + count - 1, type);
            if (token >= first && token <= last)
            {
                *parent = cur_typedef;
                break;
            }
        }
        hr = IMetaDataImport_EnumTypeDefs(iface, &typedef_enum, &cur_typedef, 1, NULL);
    }
    IMetaDataImport_CloseEnum(iface, typedef_enum);
    return hr;
}

static HRESULT methoddef_get_parent_typedef(IMetaDataImport *iface, mdMethodDef method, mdTypeDef *parent)
{
    return token_get_parent_typedef(iface, method, iface->lpVtbl->EnumMethods, parent);
}

static HRESULT WINAPI import_GetMethodProps(IMetaDataImport *iface, mdMethodDef method_def, mdTypeDef *ret_parent,
                                            WCHAR *method_name, ULONG name_len, ULONG *written, ULONG *method_flags,
                                            const COR_SIGNATURE **ret_sig_blob, ULONG *ret_sig_len, ULONG *rva,
                                            ULONG *ret_impl_flags)
{
    ULONG attrs = 0, impl_flags = 0, addr = 0, sig_len = 0, name_needed = 0;
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    mdToken parent = mdTypeDefNil;
    const BYTE *sig_blob = NULL;

    TRACE("(%p, %s, %p, %p, %lu, %p, %p, %p, %p, %p, %p)\n", iface, debugstr_mdToken(method_def), ret_parent,
          method_name, name_len, written, method_flags, ret_sig_blob, ret_sig_len, rva, ret_impl_flags);

    if (TypeFromToken(method_def) != mdtMethodDef) return S_FALSE;

    if (method_name && name_len)
        method_name[0] = L'\0';
    if (method_def != mdMethodDefNil)
    {
        const ULONG row = RidFromToken(method_def);
        ULONG name_idx, sig_idx;
        const char *nameA = NULL;
        HRESULT hr;

        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_METHODDEF, 0, row, &addr))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_METHODDEF, 1, row, &impl_flags))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_METHODDEF, 2, row, &attrs))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_METHODDEF, 3, row, &name_idx))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_METHODDEF, 4, row, &sig_idx))))
            return hr;

        if (FAILED(hr = IMetaDataTables_GetString(&impl->IMetaDataTables_iface, name_idx, &nameA)))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetBlob(&impl->IMetaDataTables_iface, sig_idx, &sig_len, &sig_blob))))
            return hr;
        if (FAILED((hr = methoddef_get_parent_typedef(iface, method_def, &parent))))
            return hr;

        name_needed = MultiByteToWideChar(CP_ACP, 0, nameA, -1, NULL, 0);
        if (method_name && name_len)
        {
            ULONG n = MultiByteToWideChar(CP_ACP, 0, nameA, min(strlen(nameA), name_len - 1), method_name, name_len - 1);
            method_name[n] = L'\0';
        }
    }
    if (ret_parent)
        *ret_parent = parent;
    if (method_flags)
        *method_flags = attrs;
    if (ret_sig_blob)
        *ret_sig_blob = sig_blob;
    if (ret_sig_len)
        *ret_sig_len = sig_len;
    if (rva)
        *rva = addr;
    if (ret_impl_flags)
        *ret_impl_flags = impl_flags;
    if (written)
        *written = name_needed;

    return (method_name && name_needed > name_len) ? CLDB_S_TRUNCATION : S_OK;
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

static HRESULT token_get_constant_value(IMetaDataImport *iface, mdToken token, ULONG *value_type, const BYTE **value,
                                        ULONG *value_len)
{
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    ULONG type = TypeFromToken(token), row, num_constant;
    HRESULT hr;

    TRACE("(%p, %s, %p, %p, %p)\n", iface, debugstr_mdToken(token), value_type, value, value_len);
    assert(type == mdtFieldDef || type == mdtParamDef || type == mdtProperty);

    if (FAILED((hr = table_get_num_rows(&impl->IMetaDataTables_iface, TABLE_CONSTANT, &num_constant)))) return hr;

    *value_type = ELEMENT_TYPE_VOID;
    *value = NULL;
    *value_len = 0;
    if (!num_constant) return S_OK;

    for (row = 1; row <= num_constant; row++)
    {
        ULONG parent, len;

        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_CONSTANT, 1, row, &parent))))
            break;
        if (parent == token)
        {
            ULONG blob_idx;

            hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_CONSTANT, 0, row, value_type);
            if (FAILED(hr)) break;
            hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_CONSTANT, 2, row, &blob_idx);
            if (FAILED(hr)) break;

            if (FAILED((hr = IMetaDataTables_GetBlob(&impl->IMetaDataTables_iface, blob_idx, &len, value))))
                break;
            if (*value_type == ELEMENT_TYPE_STRING)
                *value_len = len / sizeof(WCHAR);
            break;
        }
    }
    return hr;
}

static HRESULT fielddef_get_parent_typedef(IMetaDataImport *iface, mdFieldDef field, mdTypeDef *parent)
{
    return token_get_parent_typedef(iface, field, iface->lpVtbl->EnumFields, parent);
}

static HRESULT WINAPI import_GetFieldProps(IMetaDataImport *iface, mdFieldDef fielddef, mdTypeDef *type_def,
                                           WCHAR *field_name, ULONG name_len, ULONG *name_written, ULONG *flags,
                                           const COR_SIGNATURE **ret_sig_blob, ULONG *ret_sig_len, ULONG *value_type_flag,
                                           UVCP_CONSTANT *value, ULONG *value_len)
{
    ULONG attrs = 0, sig_len = 0, name_needed = 0, val_len = 0, val_type = ELEMENT_TYPE_VOID;
    struct metadata_tables *impl = impl_from_IMetaDataImport(iface);
    const BYTE *sig_blob = NULL, *val_blob = NULL;
    mdToken parent = mdTypeDefNil;

    TRACE("(%p, %s, %p, %p, %lu, %p, %p, %p, %p, %p, %p, %p)\n", iface, debugstr_mdToken(fielddef), type_def,
          field_name, name_len, name_written, flags, ret_sig_blob, ret_sig_len, value_type_flag, value, value_len);

    if (TypeFromToken(fielddef) != mdtFieldDef) return S_FALSE;

    if (field_name && name_len)
        field_name[0] = L'\0';
    if (!IsNilToken(fielddef))
    {
        const ULONG row = RidFromToken(fielddef);
        ULONG name_idx, sig_idx;
        const char *nameA;
        HRESULT hr;

        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_FIELD, 0, row, &attrs))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_FIELD, 1, row, &name_idx))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetColumn(&impl->IMetaDataTables_iface, TABLE_FIELD, 2, row, &sig_idx))))
            return hr;

        if (FAILED((hr = IMetaDataTables_GetString(&impl->IMetaDataTables_iface, name_idx, &nameA))))
            return hr;
        if (FAILED((hr = IMetaDataTables_GetBlob(&impl->IMetaDataTables_iface, sig_idx, &sig_len, &sig_blob))))
            return hr;
        if (FAILED((hr = fielddef_get_parent_typedef(iface, fielddef, &parent))))
            return hr;
        if (FAILED((hr = token_get_constant_value(iface, fielddef, &val_type, &val_blob, &val_len))))
            return hr;

        name_needed = MultiByteToWideChar(CP_ACP, 0, nameA, -1, NULL, 0);
        if (field_name && name_len)
        {
            ULONG n = MultiByteToWideChar(CP_ACP, 0, nameA, min(strlen(nameA), name_len - 1), field_name, name_len - 1);
            field_name[n] = L'\0';
        }
    }
    if (type_def)
        *type_def = parent;
    if (name_written)
        *name_written = name_needed;
    if (flags)
        *flags = attrs;
    if (ret_sig_blob)
        *ret_sig_blob = sig_blob;
    if (ret_sig_len)
        *ret_sig_len = sig_len;
    if (value_type_flag)
        *value_type_flag = val_type;
    if (value)
        *value = (UVCP_CONSTANT)val_blob;
    if (value_len)
        *value_len = val_len;

    return (field_name && name_needed > name_len) ? CLDB_S_TRUNCATION : S_OK;
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
