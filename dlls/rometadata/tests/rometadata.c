/*
 * Copyright 2024 Zhiyi Zhang for CodeWeavers
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
#include <windows.h>
#include "initguid.h"
#include "cor.h"
#include "roapi.h"
#include "rometadata.h"
#include "rometadataapi.h"
#include "wine/test.h"

#define WIDL_using_Wine_Test
#include "test-simple.h"

DEFINE_GUID(GUID_NULL, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

static void test_MetaDataGetDispenser(void)
{
    IMetaDataDispenserEx *dispenser_ex;
    IMetaDataDispenser *dispenser;
    IUnknown *unknown;
    HRESULT hr;

    /* Invalid parameters */
    hr = MetaDataGetDispenser(&CLSID_NULL, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == CLASS_E_CLASSNOTAVAILABLE, "Got unexpected hr %#lx.\n", hr);

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_NULL, (void **)&dispenser);
    ok(hr == E_NOINTERFACE, "Got unexpected hr %#lx.\n", hr);

    /* Normal calls */
    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IUnknown, (void **)&unknown);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IUnknown_Release(unknown);

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IMetaDataDispenser_Release(dispenser);

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenserEx, (void **)&dispenser_ex);
    ok(hr == S_OK, "Got unexpected hr %#lx.\n", hr);
    if (SUCCEEDED(hr))
        IMetaDataDispenserEx_Release(dispenser_ex);
}

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    wcscat(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            wine_dbgstr_w(pathW), GetLastError());

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(!!res, "Failed to load resource, error %lu.\n", GetLastError());
    ptr = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    WriteFile(file, ptr, SizeofResource( GetModuleHandleA(NULL), res), &written, NULL);
    ok(written == SizeofResource(GetModuleHandleA(NULL), res), "Failed to write resource.\n");
    CloseHandle(file);

    return pathW;
}

#pragma pack(push,1)
struct row_module
{
    WORD Generation;
    WORD Name;
    WORD Mvid;
    WORD EncId;
    WORD EncBaseId;
};

struct row_typeref
{
    WORD ResolutionScope;
    WORD Name;
    WORD Namespace;
};

struct row_typedef
{
    DWORD Flags;
    WORD Name;
    WORD Namespace;
    WORD Extends;
    WORD FieldList;
    WORD MethodList;
};

struct row_memberref
{
    WORD Class;
    WORD Name;
    WORD Signature;
};

struct row_custom_attribute
{
    WORD Parent;
    WORD Type;
    WORD Value;
};

#pragma pack(pop)

enum
{
    TYPE_ATTR_PUBLIC            = 0x000001,
    TYPE_ATTR_NESTEDPUBLIC      = 0x000002,
    TYPE_ATTR_NESTEDPRIVATE     = 0x000003,
    TYPE_ATTR_NESTEDFAMILY      = 0x000004,
    TYPE_ATTR_NESTEDASSEMBLY    = 0x000005,
    TYPE_ATTR_NESTEDFAMANDASSEM = 0x000006,
    TYPE_ATTR_NESTEDFAMORASSEM  = 0x000007,
    TYPE_ATTR_SEQUENTIALLAYOUT  = 0x000008,
    TYPE_ATTR_EXPLICITLAYOUT    = 0x000010,
    TYPE_ATTR_INTERFACE         = 0x000020,
    TYPE_ATTR_ABSTRACT          = 0x000080,
    TYPE_ATTR_SEALED            = 0x000100,
    TYPE_ATTR_SPECIALNAME       = 0x000400,
    TYPE_ATTR_RTSPECIALNAME     = 0x000800,
    TYPE_ATTR_IMPORT            = 0x001000,
    TYPE_ATTR_SERIALIZABLE      = 0x002000,
    TYPE_ATTR_UNKNOWN           = 0x004000,
    TYPE_ATTR_UNICODECLASS      = 0x010000,
    TYPE_ATTR_AUTOCLASS         = 0x020000,
    TYPE_ATTR_CUSTOMFORMATCLASS = 0x030000,
    TYPE_ATTR_HASSECURITY       = 0x040000,
    TYPE_ATTR_BEFOREFIELDINIT   = 0x100000
};

enum table
{
    TABLE_MODULE                 = 0x00,
    TABLE_TYPEREF                = 0x01,
    TABLE_TYPEDEF                = 0x02,
    TABLE_FIELD                  = 0x04,
    TABLE_METHODDEF              = 0x06,
    TABLE_PARAM                  = 0x08,
    TABLE_INTERFACEIMPL          = 0x09,
    TABLE_MEMBERREF              = 0x0a,
    TABLE_CONSTANT               = 0x0b,
    TABLE_CUSTOMATTRIBUTE        = 0x0c,
    TABLE_FIELDMARSHAL           = 0x0d,
    TABLE_DECLSECURITY           = 0x0e,
    TABLE_CLASSLAYOUT            = 0x0f,
    TABLE_FIELDLAYOUT            = 0x10,
    TABLE_STANDALONESIG          = 0x11,
    TABLE_EVENTMAP               = 0x12,
    TABLE_EVENT                  = 0x14,
    TABLE_PROPERTYMAP            = 0x15,
    TABLE_PROPERTY               = 0x17,
    TABLE_METHODSEMANTICS        = 0x18,
    TABLE_METHODIMPL             = 0x19,
    TABLE_MODULEREF              = 0x1a,
    TABLE_TYPESPEC               = 0x1b,
    TABLE_IMPLMAP                = 0x1c,
    TABLE_FIELDRVA               = 0x1d,
    TABLE_ASSEMBLY               = 0x20,
    TABLE_ASSEMBLYPROCESSOR      = 0x21,
    TABLE_ASSEMBLYOS             = 0x22,
    TABLE_ASSEMBLYREF            = 0x23,
    TABLE_ASSEMBLYREFPROCESSOR   = 0x24,
    TABLE_ASSEMBLYREFOS          = 0x25,
    TABLE_FILE                   = 0x26,
    TABLE_EXPORTEDTYPE           = 0x27,
    TABLE_MANIFESTRESOURCE       = 0x28,
    TABLE_NESTEDCLASS            = 0x29,
    TABLE_GENERICPARAM           = 0x2a,
    TABLE_METHODSPEC             = 0x2b,
    TABLE_GENERICPARAMCONSTRAINT = 0x2c,
    TABLE_MAX                    = 0x2d
};

struct table_info
{
    ULONG exp_row_size;
    ULONG exp_rows;
    ULONG exp_cols;
    ULONG exp_key_idx;
    const char *exp_name;
};

struct type_info
{
    DWORD exp_flags;
    const char *exp_name;
    const char *exp_namespace;
};

static void test_MetaDataDispenser_OpenScope(void)
{
    static const struct table_info tables[TABLE_MAX] = {
        {sizeof(struct row_module), 1, 5, -1, "Module"},
        {sizeof(struct row_typeref), 10, 3, -1, "TypeRef"},
        {sizeof(struct row_typedef), 3, 6, -1, "TypeDef"},
        {2, 0, 1, -1, "FieldPtr"},
        {6, 0, 3, -1, "Field"},
        {2, 0, 1, -1, "MethodPtr"},
        {14, 2, 6, -1, "Method"},
        {2, 0, 1, -1, "ParamPtr"},
        {6, 0, 3, -1, "Param"},
        {4, 1, 2, 0, "InterfaceImpl"},
        {sizeof(struct row_memberref), 6, 3, -1, "MemberRef"},
        {6, 0, 3, 1, "Constant"},
        {sizeof(struct row_custom_attribute), 7, 3, 0, "CustomAttribute"},
        {4, 0, 2, 0, "FieldMarshal"},
        {6, 0, 3, 1, "DeclSecurity"},
        {8, 0, 3, 2, "ClassLayout"},
        {6, 0, 2, 1, "FieldLayout"},
        {2, 0, 1, -1, "StandAloneSig"},
        {4, 0, 2, -1, "EventMap"},
        {2, 0, 1, -1, "EventPtr"},
        {6, 0, 3, -1, "Event"},
        {4, 0, 2, -1, "PropertyMap"},
        {2, 0, 1, -1, "PropertyPtr"},
        {6, 0, 3, -1, "Property"},
        {6, 0, 3, 2, "MethodSemantics"},
        {6, 1, 3, 0, "MethodImpl"},
        {2, 0, 1, -1, "ModuleRef"},
        {2, 0, 1, -1, "TypeSpec"},
        {8, 0, 4, 1, "ImplMap"},
        {6, 0, 2, 1, "FieldRVA"},
        {8, 0, 2, -1, "ENCLog"},
        {4, 0, 1, -1, "ENCMap"},
        {22, 1, 9, -1, "Assembly"},
        {4, 0, 1, -1, "AssemblyProcessor"},
        {12, 0, 3, -1, "AssemblyOS"},
        {20, 3, 9, -1, "AssemblyRef"},
        {6, 0, 2, -1, "AssemblyRefProcessor"},
        {14, 0, 4, -1, "AssemblyRefOS"},
        {8, 0, 3, -1, "File"},
        {14, 0, 5, -1, "ExportedType"},
        {12, 0, 4, -1, "ManifestResource"},
        {4, 0, 2, 0, "NestedClass"},
        {8, 0, 4, 2, "GenericParam"},
        {4, 0, 2, -1, "MethodSpec"},
        {4, 0, 2, 0, "GenericParamConstraint"},
    };
    static const struct type_info type_defs[3] =
    {
        { 0, "<Module>", NULL },
        { TYPE_ATTR_INTERFACE | TYPE_ATTR_ABSTRACT | TYPE_ATTR_UNKNOWN, "ITest1", "Wine.Test" },
        { TYPE_ATTR_PUBLIC | TYPE_ATTR_SEALED | TYPE_ATTR_UNKNOWN, "Test1", "Wine.Test" },
    };
    const WCHAR *filename = load_resource(L"test-simple.winmd");
    ULONG val = 0, i, guid_ctor_idx = 0, itest1_def_idx = 0;
    const struct row_typedef *type_def = NULL;
    const struct row_module *module = NULL;
    IMetaDataDispenser *dispenser;
    IMetaDataTables *md_tables;
    const GUID *guid = NULL;
    const char *str;
    HRESULT hr;

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScope(dispenser, filename, 0, &IID_IMetaDataTables, (IUnknown **)&md_tables);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IMetaDataDispenser_Release(dispenser);

    hr = IMetaDataTables_GetStringHeapSize(md_tables, &val);
    todo_wine
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine
    ok(val == 329, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetBlobHeapSize(md_tables, &val);
    todo_wine
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine
    ok(val == 156, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetGuidHeapSize(md_tables, &val);
    todo_wine
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine
    ok(val == 16, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetUserStringHeapSize(md_tables, &val);
    todo_wine
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine
    ok(val == 8, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetNumTables(md_tables, &val);
    todo_wine
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine
    ok(val == TABLE_MAX, "got val %lu\n", val);

    for (i = 0; i < TABLE_MAX; i++)
    {
        const struct table_info *table = &tables[i];
        ULONG row_size, rows, cols, key_idx;
        const char *name;

        winetest_push_context("tables[%lu]", i);

        hr = IMetaDataTables_GetTableInfo(md_tables, i, &row_size, &rows, &cols, &key_idx, &name);
        todo_wine
        ok(hr == S_OK, "got hr %#lx\n", hr);
        if (FAILED(hr))
        {
            winetest_pop_context();
            continue;
        }

        todo_wine
        ok(row_size == table->exp_row_size, "got row_size %lu != %lu\n", row_size, table->exp_row_size);
        todo_wine
        ok(rows == table->exp_rows, "got rows %lu != %lu\n", rows, table->exp_rows);
        todo_wine
        ok(cols == table->exp_cols, "got cols %lu != %lu\n", cols, table->exp_cols);
        todo_wine
        ok(key_idx == table->exp_key_idx, "got key_idx %lu != %lu\n", key_idx, table->exp_key_idx);
        todo_wine
        ok(!strcmp(name, table->exp_name), "got name %s != %s\n", debugstr_a(name), debugstr_a(table->exp_name));

        winetest_pop_context();
    }

    /* Read module information */
    hr = IMetaDataTables_GetRow(md_tables, TABLE_MODULE, 1, (BYTE *)&module);
    todo_wine
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine
    ok(!!module, "got module=%p\n", module);

    if (SUCCEEDED(hr))
    {
        str = NULL;
        hr = IMetaDataTables_GetString(md_tables, module->Name, &str);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(str &&!strcmp(str, "dlls/rometadata/tests/test-simple.winmd"), "got str %s\n", debugstr_a(str));

        hr = IMetaDataTables_GetGuid(md_tables, module->Mvid, &guid);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(!!guid, "got guid %p\n", guid);
    }

    /* Read defined types. */
    for (i = 0; i < ARRAY_SIZE(type_defs); i++)
    {
        const struct type_info *type_info = &type_defs[i];

        winetest_push_context("type_info[%lu]", i);

        type_def = NULL;
        hr = IMetaDataTables_GetRow(md_tables, TABLE_TYPEDEF, i + 1, (BYTE *)&type_def);
        todo_wine
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine
        ok(!!type_def, "got type_def=%p\n", type_def);
        if (FAILED(hr))
        {
            winetest_pop_context();
            continue;
        }

        ok(type_def->Flags == type_info->exp_flags, "got Flags %#lx != %#lx\n", type_def->Flags, type_info->exp_flags);
        str = NULL;
        hr = IMetaDataTables_GetString(md_tables, type_def->Name, &str);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(str && !strcmp(str, type_info->exp_name), "got str %s != %s\n", debugstr_a(str),
           debugstr_a(type_info->exp_name));
        if (type_info->exp_namespace)
        {
            str = NULL;
            hr = IMetaDataTables_GetString(md_tables, type_def->Namespace, &str);
            ok(str && !strcmp(str, type_info->exp_namespace), "got str %s != %s\n", debugstr_a(str),
               debugstr_a(type_info->exp_namespace));
            if (!strcmp(type_info->exp_name, "ITest1") && !strcmp(type_info->exp_namespace, "Wine.Test"))
                itest1_def_idx = ((i + 1) << 5) | 3;
        }

        winetest_pop_context();
    }

    /* Get the MemberRef row for the GuidAttribute constructor */
    for (i = 0; i < tables[TABLE_MEMBERREF].exp_rows; i++)
    {
        const struct row_memberref *ref = NULL;

        winetest_push_context("i=%lu", i);

        hr = IMetaDataTables_GetRow(md_tables, TABLE_MEMBERREF, i + 1, (BYTE *)&ref);
        todo_wine
        ok(hr == S_OK, "got hr %#lx\n", hr);
        if (FAILED(hr))
        {
            winetest_pop_context();
            continue;
        }

        hr = IMetaDataTables_GetString(md_tables, ref->Name, &str);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        if (str && !strcmp(str, ".ctor"))
        {
            const struct row_typeref *typeref = NULL;

            /* All MemberRefParent coded indices in test-simple.winmd point to TypeRef entries. */
            hr = IMetaDataTables_GetRow(md_tables, TABLE_TYPEREF, (ref->Class & ~7) >> 3, (BYTE *)&typeref);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            hr = IMetaDataTables_GetString(md_tables, typeref->Name, &str);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            if (!strcmp(str, "GuidAttribute"))
            {
                hr = IMetaDataTables_GetString(md_tables, typeref->Namespace, &str);
                ok(hr == S_OK, "got hr %#lx\n", hr);
                if (!strcmp(str, "Windows.Foundation.Metadata"))
                {
                    guid_ctor_idx = ((i + 1) << 3) | 3;
                    winetest_pop_context();
                    break;
                }
            }
        }

        winetest_pop_context();
    }

    todo_wine
    ok(!!guid_ctor_idx, "got guid_ctor_coded_idx %lu\n", guid_ctor_idx);

    /* Verify ITest1 has the correct GuidAttribute value. */
    for (i = 0; i < tables[TABLE_CUSTOMATTRIBUTE].exp_rows; i++)
    {
        const struct row_custom_attribute *attr = NULL;

        winetest_push_context("i=%lu", i);

        attr = NULL;
        hr = IMetaDataTables_GetRow(md_tables, TABLE_CUSTOMATTRIBUTE, i + 1, (BYTE *)&attr);
        todo_wine
        ok(hr == S_OK, "got hr %#lx\n", hr);
        if (FAILED(hr))
        {
            winetest_pop_context();
            continue;
        }

        if (attr->Type == guid_ctor_idx && attr->Parent == itest1_def_idx)
        {
            const BYTE *value = NULL;
            const GUID *guid;
            ULONG size = 0;

            hr = IMetaDataTables_GetBlob(md_tables, attr->Value, &size, &value);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(size == sizeof(GUID) + 4, "got size %lu\n", size);
            guid = (GUID *)&value[2];
            ok(IsEqualGUID(guid, &IID_ITest1), "got guid %s\n", debugstr_guid(guid));
        }

        winetest_pop_context();
    }

    IMetaDataTables_Release(md_tables);
}

START_TEST(rometadata)
{
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    test_MetaDataGetDispenser();
    test_MetaDataDispenser_OpenScope();

    RoUninitialize();
}
