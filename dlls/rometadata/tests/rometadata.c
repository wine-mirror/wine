/*
 * Copyright 2024 Zhiyi Zhang for CodeWeavers
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

#define COBJMACROS
#define _DEFINE_META_DATA_META_CONSTANTS
#include <windows.h>
#include "initguid.h"
#include "cor.h"
#include "corhdr.h"
#include "roapi.h"
#include "rometadata.h"
#include "rometadataapi.h"
#include "wine/test.h"

#define WIDL_using_Wine_Test
#include "test-simple.h"
#include "test-enum.h"

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

static const BYTE *load_resource_data(const WCHAR *name, ULONG *data_size)
{
    const BYTE *data;
    HRSRC res;

    res = FindResourceW(NULL, name, (LPCWSTR)RT_RCDATA);
    ok(!!res, "Failed to load resource %s, error %lu.\n", debugstr_w(name), GetLastError());
    data = LockResource(LoadResource(GetModuleHandleA(NULL), res));
    *data_size = SizeofResource(GetModuleHandleA(NULL), res);
    return data;
}

static WCHAR *load_resource(const WCHAR *name)
{
    static WCHAR pathW[MAX_PATH];
    DWORD written, res_size;
    const BYTE *data;
    HANDLE file;

    GetTempPathW(ARRAY_SIZE(pathW), pathW);
    wcscat(pathW, name);

    file = CreateFileW(pathW, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create file %s, error %lu.\n",
            wine_dbgstr_w(pathW), GetLastError());

    data = load_resource_data(name, &res_size);
    WriteFile(file, data, res_size, &written, NULL);
    ok(written == res_size, "Failed to write resource.\n");
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
    mdToken exp_base;
    const char *exp_contract_name;
    UINT32 exp_contract_version;
};

struct column_info
{
    ULONG exp_offset;
    ULONG exp_col_size;
    ULONG exp_type;
    const char *exp_name;
};

enum coded_idx_type
{
    CT_TypeDefOrRef        = 64,
    CT_HasConstant         = 65,
    CT_HasCustomAttribute  = 66,
    CT_HasFieldMarshal     = 67,
    CT_HasDeclSecurity     = 68,
    CT_MemberRefParent     = 69,
    CT_HasSemantics        = 70,
    CT_MethodDefOrRef      = 71,
    CT_MemberForwarded     = 72,
    CT_Implementation      = 73,
    CT_CustomAttributeType = 74,
    CT_ResolutionScope     = 75,
    CT_TypeOrMethodDef     = 76
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
    static const struct {
        ULONG len;
        const struct column_info columns[9];
    } table_columns[TABLE_MAX] = {
        {5,
         {
             {0, 2, iUSHORT, "Generation"},
             {2, 2, iSTRING, "Name"},
             {4, 2, iGUID, "Mvid"},
             {6, 2, iGUID, "EncId"},
             {8, 2, iGUID, "EncBaseId"}
         }},
        {3,
         {
             {0, 2, CT_ResolutionScope, "ResolutionScope"},
             {2, 2, iSTRING, "Name"},
             {4, 2, iSTRING, "Namespace"}
         }},
        {6,
         {
             {0, 4, iULONG, "Flags"},
             {4, 2, iSTRING, "Name"},
             {6, 2, iSTRING, "Namespace"},
             {8, 2, CT_TypeDefOrRef, "Extends"},
             {10, 2, TABLE_FIELD, "FieldList"},
             {12, 2, TABLE_METHODDEF, "MethodList"}
         }},
        {1, {{0, 2, TABLE_FIELD, "Field"}}},
        {3,
         {
             {0, 2, iUSHORT, "Flags"},
             {2, 2, iSTRING, "Name"},
             {4, 2, iBLOB, "Signature"},
         }},
        {1, {{0, 2, TABLE_METHODDEF, "Method"}}},
        {6,
         {
             {0, 4, iULONG, "RVA"},
             {4, 2, iUSHORT, "ImplFlags"},
             {6, 2, iUSHORT, "Flags"},
             {8, 2, iSTRING, "Name"},
             {10, 2, iBLOB, "Signature"},
             {12, 2, TABLE_PARAM, "ParamList"}
         }},
        {1, {{0, 2, TABLE_PARAM, "Param"}}},
        {3,
         {
             {0, 2, iUSHORT, "Flags"},
             {2, 2, iUSHORT, "Sequence"},
             {4, 2, iSTRING, "Name"}
         }},
        {2,
         {
             {0, 2, TABLE_TYPEDEF, "Class"},
             {2, 2, CT_TypeDefOrRef, "Interface"},
         }},
        {3,
         {
             {0, 2, CT_MemberRefParent, "Class"},
             {2, 2, iSTRING, "Name"},
             {4, 2, iBLOB, "Signature"},
         }},
        {3,
         {
             {0, 1, iBYTE, "Type"},
             {2, 2, CT_HasConstant, "Parent"},
             {4, 2, iBLOB, "Value"}
         }},
        {3,
         {
             {0, 2, CT_HasCustomAttribute, "Parent"},
             {2, 2, CT_CustomAttributeType, "Type"},
             {4, 2, iBLOB, "Value"}
         }},
        {2,
         {
             {0, 2, CT_HasFieldMarshal, "Parent"},
             {2, 2, iBLOB, "NativeType"}
         }},
        {3,
         {
             {0, 2, iSHORT, "Action"},
             {2, 2, CT_HasDeclSecurity, "Parent"},
             {4, 2, iBLOB, "PermissionSet"}
         }},
        {3,
         {
             {0, 2, iUSHORT, "PackingSize"},
             {2, 4, iULONG, "ClassSize"},
             {6, 2, TABLE_TYPEDEF, "Parent"}
         }},
        {2,
         {
             {0, 4, iULONG, "OffSet"},
             {4, 2, TABLE_FIELD, "Field"}
         }},
        {1, {{0, 2, iBLOB, "Signature"}}},
        {2,
         {
             {0, 2, TABLE_TYPEDEF, "Parent"},
             {2, 2, TABLE_EVENT, "EventList"},
         }},
        {1, {{0, 2, TABLE_EVENT, "Event"}}},
        {3,
         {
             {0, 2, iUSHORT, "EventFlags"},
             {2, 2, iSTRING, "Name"},
             {4, 2, CT_TypeDefOrRef, "EventType"}
         }},
        {2,
         {
             {0, 2, TABLE_TYPEDEF, "Parent"},
             {2, 2, TABLE_PROPERTY, "PropertyList"}
         }},
        {1, {{0, 2, TABLE_PROPERTY, "Property"}}},
        {3,
         {
             {0, 2, iUSHORT, "PropFlags"},
             {2, 2, iSTRING, "Name"},
             {4, 2, iBLOB, "Type"}
         }},
        {3,
         {
             {0, 2, iUSHORT, "Semantic"},
             {2, 2, TABLE_METHODDEF, "Method"},
             {4, 2, CT_HasSemantics, "Association"}
         }},
        {3,
         {
             {0, 2, TABLE_TYPEDEF, "Class"},
             {2, 2, CT_MethodDefOrRef, "MethodBody"},
             {4, 2, CT_MethodDefOrRef, "MethodDeclaration"}
         }},
        {1, {{0, 2, iSTRING, "Name"}}},
        {1, {{0, 2, iBLOB, "Signature"}}},
        {4,
         {
             {0, 2, iUSHORT, "MappingFlags"},
             {2, 2, CT_MemberForwarded, "MemberForwarded"},
             {4, 2, iSTRING, "ImportName"},
             {6, 2, TABLE_MODULEREF, "ImportScope"}
         }},
        {2,
         {
             {0, 4, iULONG, "RVA"},
             {4, 2, TABLE_FIELD, "Field"}
         }},
        {2,
         {
             {0, 4, iULONG, "Token"},
             {4, 4, iULONG, "FuncCode"}
         }},
        {1, {{0, 4, iULONG, "Token"}}},
        {9,
         {
             {0, 4, iULONG, "HashAlgId"},
             {4, 2, iUSHORT, "MajorVersion"},
             {6, 2, iUSHORT, "MinorVersion"},
             {8, 2, iUSHORT, "BuildNumber"},
             {10, 2, iUSHORT, "RevisionNumber"},
             {12, 4, iULONG, "Flags"},
             {16, 2, iBLOB, "PublicKey"},
             {18, 2, iSTRING, "Name"},
             {20, 2, iSTRING, "Locale"}
         }},
        {1, {{0, 4, iULONG, "Processor"}}},
        {3,
         {
             {0, 4, iULONG, "OSPlatformId"},
             {4, 4, iULONG, "OSMajorVersion"},
             {8, 4, iULONG, "OSMinorVersion"}
         }},
        {9,
         {
             {0, 2, iUSHORT, "MajorVersion"},
             {2, 2, iUSHORT, "MinorVersion"},
             {4, 2, iUSHORT, "BuildNumber"},
             {6, 2, iUSHORT, "RevisionNumber"},
             {8, 4, iULONG, "Flags"},
             {12, 2, iBLOB, "PublicKeyOrToken"},
             {14, 2, iSTRING, "Name"},
             {16, 2, iSTRING, "Locale"},
             {18, 2, iBLOB, "HashValue"}
         }},
        {2,
         {
             {0, 4, iULONG, "Processor"},
             {4, 2, TABLE_ASSEMBLYREF, "AssemblyRef"}
         }},
        {4,
         {
             {0, 4, iULONG, "OSPlatformId"},
             {4, 4, iULONG, "OSMajorVersion"},
             {8, 4, iULONG, "OSMinorVersion"},
             {12, 2, TABLE_ASSEMBLYREF, "AssemblyRef"}
         }},
        {3,
         {
             {0, 4, iULONG, "Flags"},
             {4, 2, iSTRING, "Name"},
             {6, 2, iBLOB, "HashValue"}
         }},
        {5,
         {
             {0, 4, iULONG, "Flags"},
             {4, 4, iULONG, "TypeDefId"},
             {8, 2, iSTRING, "TypeName"},
             {10, 2, iSTRING, "TypeNamespace"},
             {12, 2, CT_Implementation, "Implementation"}
         }},
        {4,
         {
             {0, 4, iULONG, "Offset"},
             {4, 4, iULONG, "Flags"},
             {8, 2, iSTRING, "Name"},
             {10, 2, CT_Implementation, "Implementation"}
         }},
        {2,
         {
             {0, 2, TABLE_TYPEDEF, "NestedClass"},
             {2, 2, TABLE_TYPEDEF, "EnclosingClass"}
         }},
        {4,
         {
             {0, 2, iUSHORT, "Number"},
             {2, 2, iUSHORT, "Flags"},
             {4, 2, CT_TypeOrMethodDef, "Owner"},
             {6, 2, iSTRING, "Name"}
         }},
        {2,
         {
             {0, 2, CT_MethodDefOrRef, "Method"},
             {2, 2, iBLOB, "Instantiation"}
         }},
        {2,
         {
             {0, 2, TABLE_GENERICPARAM, "Owner"},
             {2, 2, CT_TypeDefOrRef, "Constraint"}
         }},
    };
    static const struct type_info type_defs[3] =
    {
        { 0, "<Module>", NULL },
        { tdInterface | tdAbstract | tdWindowsRuntime, "ITest1", "Wine.Test" },
        { tdPublic | tdSealed | tdWindowsRuntime, "Test1", "Wine.Test" },
    };
    ULONG val = 0, i, guid_ctor_idx = 0, itest1_def_idx = 0, md_size;
    const BYTE *md_bytes = load_resource_data(L"test-simple.winmd", &md_size);
    const WCHAR *filename = load_resource(L"test-simple.winmd");
    const struct row_typedef *type_def = NULL;
    const struct row_module *module = NULL;
    IMetaDataDispenser *dispenser;
    IMetaDataTables *md_tables;
    const GUID *guid = NULL;
    const char *str;
    HRESULT hr;

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScopeOnMemory(dispenser, NULL, 0, 0, &IID_IMetaDataTables, (IUnknown **)&md_tables);
    ok(hr == E_FAIL, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScopeOnMemory(dispenser, NULL, md_size, 0, &IID_IMetaDataTables,
                                              (IUnknown **)&md_tables);
    ok(hr == E_FAIL, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScopeOnMemory(dispenser, md_bytes, 0, 0, &IID_IMetaDataTables,
                                              (IUnknown **)&md_tables);
    ok(hr == CLDB_E_NO_DATA, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScopeOnMemory(dispenser, md_bytes, sizeof(IMAGE_DOS_HEADER), 0, &IID_IMetaDataTables,
                                              (IUnknown **)&md_tables);
    ok(hr == CLDB_E_FILE_CORRUPT, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScopeOnMemory(dispenser, md_bytes, md_size, 0, &IID_IMetaDataTables,
                                              (IUnknown **)&md_tables);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IMetaDataTables_Release(md_tables);

    hr = IMetaDataDispenser_OpenScope(dispenser, filename, 0, &IID_IMetaDataTables, (IUnknown **)&md_tables);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IMetaDataDispenser_Release(dispenser);

    hr = IMetaDataTables_GetStringHeapSize(md_tables, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == 329, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetBlobHeapSize(md_tables, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == 156, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetGuidHeapSize(md_tables, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == 16, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetUserStringHeapSize(md_tables, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == 8, "got val %lu\n", val);

    val = 0;
    hr = IMetaDataTables_GetNumTables(md_tables, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == TABLE_MAX, "got val %lu\n", val);

    for (i = 0; i < TABLE_MAX; i++)
    {
        const struct table_info *table = &tables[i];
        ULONG row_size, rows, cols, key_idx;
        const char *name;
        ULONG j;

        winetest_push_context("tables[%lu]", i);

        hr = IMetaDataTables_GetTableInfo(md_tables, i, &row_size, &rows, &cols, &key_idx, &name);
        ok(hr == S_OK, "got hr %#lx\n", hr);

        ok(row_size == table->exp_row_size, "got row_size %lu != %lu\n", row_size, table->exp_row_size);
        ok(rows == table->exp_rows, "got rows %lu != %lu\n", rows, table->exp_rows);
        ok(cols == table->exp_cols, "got cols %lu != %lu\n", cols, table->exp_cols);
        ok(key_idx == table->exp_key_idx, "got key_idx %lu != %lu\n", key_idx, table->exp_key_idx);
        ok(!strcmp(name, table->exp_name), "got name %s != %s\n", debugstr_a(name), debugstr_a(table->exp_name));

        for (j = 0; j < table_columns[i].len; j++)
        {
            const struct column_info *column = &table_columns[i].columns[j];
            ULONG offset, col_size, type;

            winetest_push_context("column=%lu", j);

            hr = IMetaDataTables_GetColumnInfo(md_tables, i, j, &offset, &col_size, &type, &name);
            ok(hr == S_OK, "got hr %#lx\n", hr);

            ok(offset == column->exp_offset, "got offset %lu != %lu\n", offset, column->exp_offset);
            ok(col_size == column->exp_col_size, "got col_size %lu != %lu\n", col_size, column->exp_col_size);
            ok(type == column->exp_type, "got type %lu != %lu\n", type, column->exp_type);
            ok(!strcmp(name, column->exp_name), "got name %s != %s\n", debugstr_a(name), debugstr_a(column->exp_name));

            winetest_pop_context();
        }

        winetest_pop_context();
    }

    /* Read module information */
    hr = IMetaDataTables_GetRow(md_tables, TABLE_MODULE, 1, (BYTE *)&module);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!!module, "got module=%p\n", module);

    str = NULL;
    hr = IMetaDataTables_GetString(md_tables, module->Name, &str);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(str &&!strcmp(str, "dlls/rometadata/tests/test-simple.winmd"), "got str %s\n", debugstr_a(str));

    hr = IMetaDataTables_GetGuid(md_tables, module->Mvid, &guid);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!!guid, "got guid %p\n", guid);

    hr = IMetaDataTables_GetColumn(md_tables, TABLE_MODULE, 1, 1, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == module->Name, "got val %#lx != %#x\n", val, module->Name);

    hr = IMetaDataTables_GetColumn(md_tables, TABLE_MODULE, 2, 1, &val);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(val == module->Mvid, "got val %#lx != %#x\n", val, module->Mvid);

    /* Read defined types. */
    for (i = 0; i < ARRAY_SIZE(type_defs); i++)
    {
        const struct type_info *type_info = &type_defs[i];

        winetest_push_context("type_info[%lu]", i);

        type_def = NULL;
        hr = IMetaDataTables_GetRow(md_tables, TABLE_TYPEDEF, i + 1, (BYTE *)&type_def);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(!!type_def, "got type_def=%p\n", type_def);

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
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(str && !strcmp(str, type_info->exp_namespace), "got str %s != %s\n", debugstr_a(str),
               debugstr_a(type_info->exp_namespace));
            if (!strcmp(type_info->exp_name, "ITest1") && !strcmp(type_info->exp_namespace, "Wine.Test"))
                itest1_def_idx = ((i + 1) << 5) | 3;
        }

        hr = IMetaDataTables_GetColumn(md_tables, TABLE_TYPEDEF, 0, i + 1, &val);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(val == type_def->Flags, "got val %#lx != %#lx\n", val, type_def->Flags);

        hr = IMetaDataTables_GetColumn(md_tables, TABLE_TYPEDEF, 1, i + 1, &val);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(val == type_def->Name, "got val %#lx != %#x\n", val, type_def->Name);

        hr = IMetaDataTables_GetColumn(md_tables, TABLE_TYPEDEF, 2, i + 1, &val);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(val == type_def->Namespace, "got val %#lx != %#x\n", val, type_def->Namespace);

        winetest_pop_context();
    }

    /* Get the MemberRef row for the GuidAttribute constructor */
    for (i = 0; i < tables[TABLE_MEMBERREF].exp_rows; i++)
    {
        const struct row_memberref *ref = NULL;

        winetest_push_context("i=%lu", i);

        hr = IMetaDataTables_GetRow(md_tables, TABLE_MEMBERREF, i + 1, (BYTE *)&ref);
        ok(hr == S_OK, "got hr %#lx\n", hr);

        hr = IMetaDataTables_GetString(md_tables, ref->Name, &str);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        if (str && !strcmp(str, ".ctor"))
        {
            ULONG rid, type, typeref_row = (ref->Class & ~7) >> 3;
            const struct row_typeref *typeref = NULL;

            /* All MemberRefParent coded indices in test-simple.winmd point to TypeRef entries. */
            hr = IMetaDataTables_GetRow(md_tables, TABLE_TYPEREF, typeref_row, (BYTE *)&typeref);
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

            hr = IMetaDataTables_GetColumn(md_tables, TABLE_TYPEREF, 1, typeref_row, &val);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(val == typeref->Name, "got val %#lx != %#x\n", val, typeref->Name);

            hr = IMetaDataTables_GetColumn(md_tables, TABLE_TYPEREF, 2, typeref_row, &val);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(val == typeref->Namespace, "got val %#lx != %#x\n", val, typeref->Namespace);

            hr = IMetaDataTables_GetColumn(md_tables, TABLE_MEMBERREF, 0, i + 1, &val);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            rid = RidFromToken(val);
            type = TypeFromToken(val);
            ok(rid == typeref_row, "got rid %#lx != %#lx\n", rid, typeref_row);
            ok(type == mdtTypeRef, "got type %#lx != %#x\n", type, mdtTypeRef);
        }

        hr  = IMetaDataTables_GetColumn(md_tables, TABLE_MEMBERREF, 1, i + 1, &val);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(val == ref->Name, "got val %#lx != %#x\n", val, ref->Name);

        winetest_pop_context();
    }

    ok(!!guid_ctor_idx, "got guid_ctor_coded_idx %lu\n", guid_ctor_idx);

    /* Verify ITest1 has the correct GuidAttribute value. */
    for (i = 0; i < tables[TABLE_CUSTOMATTRIBUTE].exp_rows; i++)
    {
        const struct row_custom_attribute *attr = NULL;

        winetest_push_context("i=%lu", i);

        attr = NULL;
        hr = IMetaDataTables_GetRow(md_tables, TABLE_CUSTOMATTRIBUTE, i + 1, (BYTE *)&attr);
        ok(hr == S_OK, "got hr %#lx\n", hr);

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

        hr = IMetaDataTables_GetColumn(md_tables, TABLE_CUSTOMATTRIBUTE, 2, i + 1, &val);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(val == attr->Value, "got val %#lx != %#x\n", val, attr->Value);

        winetest_pop_context();
    }

    IMetaDataTables_Release(md_tables);
}

static const char *debugstr_CorTokenType(CorTokenType type)
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
    int i;

    for (i = 0; i < ARRAY_SIZE(types); i++)
        if (type == types[i].type) return types[i].str;
    return wine_dbg_sprintf("%#x\n", type);
}

static const char *debugstr_mdToken(mdToken token)
{
    CorTokenType type = TypeFromToken(token);
    const char *type_str = debugstr_CorTokenType(type);
    UINT rid = RidFromToken(token);

    if (type_str[0] == 'm') /* Known CorTokenType value */
        return rid ? wine_dbg_sprintf("(%s|%#x)", type_str, rid) : wine_dbg_sprintf("md%sNil", &type_str[3]);
    return wine_dbg_sprintf("(%#x|%#x)", type, rid);
}

#define test_token(iface, token, exp_type, nil) test_token_(__LINE__, iface, token, exp_type, nil)
static void test_token_(int line, IMetaDataImport *iface, mdToken token, CorTokenType exp_type, BOOL nil)
{
    CorTokenType type = TypeFromToken(token);
    BOOL valid;

    ok_(__FILE__, line)(type == exp_type, "got token type %s != %s\n", debugstr_CorTokenType(type),
                        debugstr_CorTokenType(exp_type));
    ok_(__FILE__, line)(nil == IsNilToken(token), "got nil token %s\n", debugstr_mdToken(token));
    valid = IMetaDataImport_IsValidToken(iface, token);
    todo_wine ok_(__FILE__, line)(valid, "got invalid token %s\n", debugstr_mdToken(token));
}
struct method_props
{
    const WCHAR *exp_name;
    CorMethodAttr exp_method_flags;
    CorMethodImpl exp_impl_flags;
    BYTE exp_sig_blob[6];
    ULONG exp_sig_len;
    CorPinvokeMap exp_call_conv;
};

struct field_props
{
    const WCHAR *exp_name;
    CorFieldAttr exp_flags;
    CorElementType exp_value_type;
    BYTE exp_sig_blob[3];
    ULONG exp_sig_len;
    BOOL has_value;
    const char exp_value[4];
    ULONG value_len;
};

struct property
{
    const WCHAR *exp_name;
    BYTE exp_sig_blob[4];
    ULONG exp_sig_len;
    BOOL has_get;
    BOOL has_set;
};

#define test_contract_value(data, data_len, exp_name, exp_version) test_contract_value_(__LINE__, data, data_len, exp_name, exp_version)
static void test_contract_value_(int line, const BYTE *data, ULONG data_len, const char *exp_name, UINT32 exp_version)
{
    const ULONG name_len = strlen(exp_name);
    /* {0x1, 0x2, <len>, <exp_name>, <exp_version>, 0x0, 0x0} */
    const ULONG exp_len = 3 + name_len + sizeof(exp_version) + 2;

    ok_(__FILE__, line)(!!data, "got data %p\n", data);
    ok_(__FILE__, line)(data_len == exp_len, "got data_len %lu != %lu\n", data_len, exp_len);
    if (data && data_len == exp_len)
    {
        const char *name = (char *)&data[3];
        const UINT32 version = *(UINT32 *)&data[3 + name_len];

        ok_(__FILE__, line)(data[0] == 1 && data[1] == 0 && data[2] == name_len,
                            "unexpected contract value prefix {%#x, %#x, %#x}\n", data[0], data[1], data[2]);
        ok_(__FILE__, line)(!memcmp(name, exp_name, name_len), "unexpected contract name: %s != %s\n",
                            debugstr_an(name, name_len), debugstr_a(exp_name));
        ok_(__FILE__, line)(version == exp_version, "got version %#x != %#x\n", version, exp_version);
    }
}

enum prop_method_type
{
    PROP_METHOD_GET,
    PROP_METHOD_SET
};

#define test_prop_method_token(md, class_token, name, type, token) \
    test_prop_method_token_(__LINE__, md, class_token, name, type, token)
static void test_prop_method_token_(int line, IMetaDataImport *md_import, mdTypeDef class_typedef,
                                    const WCHAR *prop_name, enum prop_method_type type, mdMethodDef token)
{
    static const CorMethodAttr exp_attrs = mdPublic | mdNewSlot | mdVirtual | mdHideBySig | mdAbstract | mdSpecialName;
    const WCHAR *prefix = type == PROP_METHOD_GET ? L"get" : L"put";
    WCHAR exp_name[80], name[80];
    ULONG attrs = 0, impl = 0;
    mdTypeDef type_def = 0;
    HRESULT hr;
    BOOL valid;

    ok_(__FILE__, line)(token && token != mdMethodDefNil, "got token %#x\n", token);
    valid = IMetaDataImport_IsValidToken(md_import, token);
    todo_wine ok_(__FILE__, line)(valid, "got value %d\n", valid);
    name[0] = L'\0';
    hr = IMetaDataImport_GetMethodProps(md_import, token, &type_def, name, ARRAY_SIZE(name), NULL, &attrs, NULL, NULL,
                                        NULL, &impl);
    ok_(__FILE__, line)(hr == S_OK, "GetMethodProps failed, got hr %#lx\n", hr);
    swprintf(exp_name, ARRAY_SIZE(exp_name), L"%s_%s", prefix, prop_name);
    ok_(__FILE__, line)(!wcscmp(name, exp_name), "got name %s != %s\n", debugstr_w(name), debugstr_w(exp_name));
    ok_(__FILE__, line)(attrs == exp_attrs, "got attrs %#lx != %#x\n", attrs, exp_attrs);
    ok_(__FILE__, line)(!impl, "got impl %#lx\n", impl);
}

static void test_IMetaDataImport(void)
{
    static const struct type_info type_defs[] =
    {
        { tdPublic | tdSealed | tdWindowsRuntime, "TestEnum1", "Wine.Test", 0x1000001, "Windows.Foundation.UniversalApiContract", 0x10000 },
        { tdPublic | tdSealed | tdSequentialLayout | tdWindowsRuntime, "TestStruct1", "Wine.Test", 0x1000005, "Windows.Foundation.UniversalApiContract", 0x20000 },
        { tdInterface | tdAbstract | tdWindowsRuntime, "ITest2", "Wine.Test", 0x1000000, "Windows.Foundation.UniversalApiContract", 0x30000 },
        { tdPublic | tdSealed | tdWindowsRuntime, "Test2", "Wine.Test", 0x100000b, "Windows.Foundation.UniversalApiContract", 0x30000 },
        { tdPublic | tdInterface | tdAbstract | tdWindowsRuntime, "ITest3", "Wine.Test", 0x1000000, "Windows.Foundation.UniversalApiContract", 0x10000 },
    };
    static const struct method_props test2_methods[2] =
    {
        { L"Method1", mdPublic | mdNewSlot | mdFinal | mdVirtual | mdHideBySig, miManaged | miRuntime, { 0x20, 0x2, 0x8, 0x8, 0x8 }, 5, pmCallConvWinapi },
        { L"Method2", mdPublic | mdNewSlot | mdFinal | mdVirtual | mdHideBySig, miManaged | miRuntime, { 0x20, 0x1, 0x11, 0x9, 0x11, 0x9 }, 6, pmCallConvWinapi },
    };
    static const struct field_props testenum1_fields[4] =
    {
        { COR_ENUM_FIELD_NAME_W, fdPrivate | fdSpecialName | fdRTSpecialName, ELEMENT_TYPE_VOID, { 0x6, 0x8 }, 2 },
        { L"Foo", fdPublic | fdStatic | fdLiteral | fdHasDefault, ELEMENT_TYPE_I4, { 0x6, 0x11, 0x9 }, 3, TRUE, { 0, 0, 0, 0 }, 4 },
        { L"Bar", fdPublic | fdStatic | fdLiteral | fdHasDefault, ELEMENT_TYPE_I4, { 0x6, 0x11, 0x9 }, 3, TRUE, { 1, 0, 0, 0 }, 4 },
        { L"Baz", fdPublic | fdStatic | fdLiteral | fdHasDefault, ELEMENT_TYPE_I4, { 0x6, 0x11, 0x9 }, 3, TRUE, { 2, 0, 0, 0 }, 4 },
    };
    static const struct field_props teststruct1_fields[3] =
    {
        { L"a", fdPublic, ELEMENT_TYPE_VOID, { 0x6, ELEMENT_TYPE_I4 }, 2 },
        { L"b", fdPublic, ELEMENT_TYPE_VOID, { 0x6, ELEMENT_TYPE_I4 }, 2 },
        { L"c", fdPublic, ELEMENT_TYPE_VOID, { 0x6, ELEMENT_TYPE_I4 }, 2 },
    };
    static const struct property test3_props[5] =
    {
        { L"Prop1", { 0x28, 0x0, 0x8 }, 3, TRUE, FALSE},
        { L"Prop2", { 0x28, 0x0, 0x11, 0x9 }, 4, TRUE, FALSE },
        { L"Prop3", { 0x28, 0, 0x11, 0x19 }, 4, TRUE, FALSE },
        { L"Prop4", { 0x28, 0, 0x1c }, 3, TRUE, FALSE },
        { L"Prop5", { 0x28, 0x0, 0x8 }, 3, TRUE, TRUE},
    };
    const struct field_enum_test_case
    {
        const WCHAR *type_name;
        const struct field_props *field_props;
        ULONG fields_len;
    } field_enum_test_cases[2] = {
        { L"Wine.Test.TestEnum1", testenum1_fields, ARRAY_SIZE(testenum1_fields) },
        { L"Wine.Test.TestStruct1", teststruct1_fields, ARRAY_SIZE(teststruct1_fields) },
    };
    static const WCHAR *contract_attribute_name = L"Windows.Foundation.Metadata.ContractVersionAttribute";
    static const WCHAR *guid_attribute_name = L"Windows.Foundation.Metadata.GuidAttribute";

    const WCHAR *filename = load_resource(L"test-enum.winmd");
    ULONG buf_len, buf_len2, buf_count, str_len, str_reqd, i;
    mdTypeDef *typedef_tokens, typedef1, typedef2;
    mdMethodDef *methoddef_tokens, methoddef;
    mdFieldDef *fielddef_tokens, fielddef;
    HCORENUM henum = NULL, henum2 = NULL;
    IMetaDataDispenser *dispenser;
    mdProperty *property_tokens;
    IMetaDataImport *md_import;
    const BYTE *data = NULL;
    const GUID *guid;
    mdToken token;
    WCHAR *strW;
    HRESULT hr;
    ULONG val;

    hr = MetaDataGetDispenser(&CLSID_CorMetaDataDispenser, &IID_IMetaDataDispenser, (void **)&dispenser);
    ok(hr == S_OK, "got hr %#lx\n", hr);

    hr = IMetaDataDispenser_OpenScope(dispenser, filename, 0, &IID_IMetaDataImport, (IUnknown **)&md_import);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    IMetaDataDispenser_Release(dispenser);

    buf_count = 0xdeadbeef;
    henum = NULL;
    hr = IMetaDataImport_EnumTypeDefs(md_import, &henum, NULL, 0, &buf_count);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    ok(buf_count == 0, "got buf_reqd %lu\n", buf_count);
    ok(!!henum, "got henum %p\n", henum);

    buf_len = 0;
    hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    /* The <Module> typedef is ommitted. */
    ok(buf_len == ARRAY_SIZE(type_defs), "got len %lu\n", buf_len);

    typedef_tokens = calloc(buf_len, sizeof(*typedef_tokens));
    ok(!!typedef_tokens, "got typedef_tokens %p\n", typedef_tokens);
    hr = IMetaDataImport_EnumTypeDefs(md_import, &henum, typedef_tokens, buf_len, &buf_count);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_len == buf_count, "got len %lu != %lu\n", buf_len, buf_count);
    for (i = 0; i < buf_len; i++)
    {
        const struct type_info *info = &type_defs[i];
        ULONG exp_len, data_len, len;
        mdTypeDef token = 0;
        WCHAR bufW[80];
        char bufA[80];
        mdToken base;

        winetest_push_context("i=%lu", i);

        test_token(md_import, typedef_tokens[i], mdtTypeDef, FALSE);
        bufW[0] = L'\0';
        bufA[0] = '\0';
        exp_len = snprintf(bufA, sizeof(bufA), "%s.%s", info->exp_namespace, info->exp_name) + 1;
        str_reqd = 0;
        hr = IMetaDataImport_GetTypeDefProps(md_import, typedef_tokens[i], NULL, 0, &str_reqd, NULL, NULL);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(str_reqd == exp_len, "got str_reqd %lu != %lu\n", str_reqd, exp_len);
        MultiByteToWideChar(CP_ACP, 0, bufA, -1, bufW, ARRAY_SIZE(bufW));

        str_len = str_reqd;
        strW = calloc(str_len, sizeof(WCHAR));
        hr = IMetaDataImport_GetTypeDefProps(md_import, typedef_tokens[i], strW, str_len - 1, &str_reqd, &val, &base);
        ok(hr == CLDB_S_TRUNCATION, "got hr %#lx\n", hr);
        len = wcslen(strW);
        ok( len == str_len - 2, "got len %lu != %lu\n", len, str_len - 2);
        if (hr == CLDB_S_TRUNCATION)
            ok(!wcsncmp(strW, bufW, str_len - 2), "got bufW %s != %s\n", debugstr_w(strW),
               debugstr_wn(bufW, str_len - 2));
        val = base = 0;
        hr = IMetaDataImport_GetTypeDefProps(md_import, typedef_tokens[i], strW, str_len, NULL, &val, &base);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        if (hr == S_OK)
            ok(1 || !wcscmp(strW, bufW), "got strW %s != %s\n", debugstr_w(strW), debugstr_w(bufW));
        free(strW);

        ok(val == info->exp_flags, "got val %#lx != %#lx\n", val, info->exp_flags);
        ok(base == info->exp_base, "got base %s != %s\n", debugstr_mdToken(base), debugstr_mdToken(info->exp_base));

        hr = IMetaDataImport_FindTypeDefByName(md_import, bufW, 0, &token);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(token == typedef_tokens[i], "got token %s != %s\n", debugstr_mdToken(token), debugstr_mdToken(typedef_tokens[i]));

        if (info->exp_contract_name)
        {
            data_len = 0;
            data = NULL;
            hr = IMetaDataImport_GetCustomAttributeByName(md_import, typedef_tokens[i], contract_attribute_name, &data, &data_len);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            test_contract_value(data, data_len, info->exp_contract_name, info->exp_contract_version);
        }

        winetest_pop_context();
    }
    hr = IMetaDataImport_EnumTypeDefs(md_import, &henum, typedef_tokens, buf_len, &buf_count);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);

    hr = IMetaDataImport_ResetEnum(md_import, henum, 0);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    buf_count = 0xdeadbeef;
    hr = IMetaDataImport_EnumTypeDefs(md_import, &henum, typedef_tokens, buf_len, &buf_count);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_len == buf_count, "got len %lu != %lu\n", buf_len, buf_count);
    hr = IMetaDataImport_EnumTypeDefs(md_import, &henum, typedef_tokens, buf_len, &buf_count);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    IMetaDataImport_CloseEnum(md_import, henum);
    free(typedef_tokens);

    hr = IMetaDataImport_FindTypeDefByName(md_import, NULL, 0, NULL);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);

    hr = IMetaDataImport_FindTypeDefByName(md_import, L"Test2", 0, &typedef1);
    ok(hr == CLDB_E_RECORD_NOTFOUND, "got hr %#lx\n", hr);
    hr = IMetaDataImport_FindTypeDefByName(md_import, NULL, 0, &typedef1);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);

    typedef1 = 0;
    hr = IMetaDataImport_FindTypeDefByName(md_import, L"Wine.Test.Test2", 0, &typedef1);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    test_token(md_import, typedef1, mdtTypeDef, FALSE);
    buf_count = 0xdeadbeef;
    henum = NULL;
    hr = IMetaDataImport_EnumMethods(md_import, &henum, typedef1, NULL, 0, &buf_count);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    ok(buf_count == 0, "got buf_reqd %lu\n", buf_count);
    buf_len = 0;
    hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_len == ARRAY_SIZE(test2_methods), "got buf_len %#lx\n" , buf_len);
    methoddef_tokens = calloc(buf_len, sizeof(*methoddef_tokens));
    ok(!!methoddef_tokens, "got methoddef_tokens %p\n", methoddef_tokens);
    hr = IMetaDataImport_EnumMethods(md_import, &henum, typedef1, methoddef_tokens, buf_len, &buf_count);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_count == buf_len, "got buf_reqd %lu != %lu\n", buf_count, buf_len);
    for (i = 0; i < buf_len; i++)
    {
        ULONG method_flags = 0, impl_flags = 0, sig_len = 0, call_conv = 0;
        const struct method_props *method = &test2_methods[i];
        const COR_SIGNATURE *sig_blob = NULL;
        WCHAR name[80];

        winetest_push_context("i=%lu", i);

        test_token(md_import, methoddef_tokens[i], mdtMethodDef, FALSE);
        name[0] = L'\0';
        str_len = 0;
        hr = IMetaDataImport_GetMethodProps(md_import, methoddef_tokens[i], &typedef2, name, ARRAY_SIZE(name), &str_len,
                                            &method_flags, &sig_blob, &sig_len, NULL, &impl_flags);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(typedef2 == typedef1, "got typedef2 %s != %s\n", debugstr_mdToken(typedef2), debugstr_mdToken(typedef1));
        ok(method_flags == method->exp_method_flags, "got method_flags %#lx != %#x\n", method_flags,
           method->exp_method_flags);
        ok(impl_flags == method->exp_impl_flags, "got impl_flags %#lx != %#x\n", impl_flags, method->exp_impl_flags);
        ok(!!sig_blob, "got sig_blob %p\n", sig_blob);
        ok(sig_len == method->exp_sig_len, "got sig_len %lu != %lu\n", sig_len, method->exp_sig_len);
        if (sig_blob && sig_len == method->exp_sig_len)
            ok(!memcmp(sig_blob, method->exp_sig_blob, method->exp_sig_len), "got unexpected sig_blob\n");
        ok(!wcscmp(name, method->exp_name), "got name %s != %s\n", debugstr_w(name), debugstr_w(method->exp_name));

        hr = IMetaDataImport_GetNativeCallConvFromSig(md_import, sig_blob, sig_len, &call_conv);
        todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(call_conv == method->exp_call_conv, "got call_conv %#lx != %#x\n", call_conv, method->exp_call_conv);

        methoddef = mdMethodDefNil;
        hr = IMetaDataImport_FindMethod(md_import, typedef2, name, sig_blob, sig_len, &methoddef);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(methoddef == methoddef_tokens[i], "got methoddef %s != %s\n", debugstr_mdToken(methoddef),
           debugstr_mdToken(methoddef_tokens[i]));
        methoddef = mdMethodDefNil;
        hr = IMetaDataImport_FindMethod(md_import, typedef2, name, NULL, 0, &methoddef);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(methoddef == methoddef_tokens[i], "got methoddef %s != %s\n", debugstr_mdToken(methoddef),
           debugstr_mdToken(methoddef_tokens[i]));

        henum2 = NULL;
        methoddef = mdMethodDefNil;
        hr = IMetaDataImport_EnumMethodsWithName(md_import, &henum2, typedef2, name, NULL, 0, NULL);
        ok(hr == S_FALSE, "got hr %#lx\n", hr);
        ok(!!henum2, "got henum2 %p\n", henum2);
        buf_len2 = 0;
        hr = IMetaDataImport_CountEnum(md_import, henum2, &buf_len2);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(buf_len2 == 1, "got buf_count2 %lu\n", buf_len2);
        buf_len2 = 0;
        hr = IMetaDataImport_EnumMethodsWithName(md_import, &henum2, typedef2, name, &methoddef, 1, &buf_len2);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(buf_len2 == 1, "got count %lu\n", hr);
        ok(methoddef == methoddef_tokens[i], "got methoddef %s != %s\n", debugstr_mdToken(methoddef),
           debugstr_mdToken(methoddef_tokens[i]));
        IMetaDataImport_CloseEnum(md_import, henum2);

        token = mdTokenNil;
        hr = IMetaDataImport_FindMember(md_import, typedef2, name, sig_blob, sig_len, &token);
        todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(token == methoddef_tokens[i], "got token %s != %s\n", debugstr_mdToken(token),
                     debugstr_mdToken(methoddef_tokens[i]));
        token = mdTokenNil;
        hr = IMetaDataImport_FindMember(md_import, typedef2, name, NULL, 0, &token);
        todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(token == methoddef_tokens[i], "got token %s != %s\n", debugstr_mdToken(token),
                     debugstr_mdToken(methoddef_tokens[i]));

        henum2 = NULL;
        token = mdTokenNil;
        hr = IMetaDataImport_EnumMembersWithName(md_import, &henum2, typedef2, name, &token, 1, NULL);
        todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(!!henum2, "got henum2 %p\n", henum2);
        buf_len2 = 0;
        hr = IMetaDataImport_CountEnum(md_import, henum2, &buf_len2);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(buf_len2 == 1, "got buf_len2 %lu\n", buf_len2);
        todo_wine ok(token == methoddef_tokens[i], "got token %s != %s\n", debugstr_mdToken(token),
                     debugstr_mdToken(methoddef_tokens[i]));
        IMetaDataImport_CloseEnum(md_import, henum2);

        winetest_pop_context();
    }
    hr = IMetaDataImport_FindMethod(md_import, typedef2, NULL, NULL, 0, &methoddef);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);
    hr = IMetaDataImport_FindMethod(md_import, typedef2, L"foo", NULL, 0, &methoddef);
    ok(hr == CLDB_E_RECORD_NOTFOUND, "got hr %#lx\n", hr);
    free(methoddef_tokens);
    IMetaDataImport_CloseEnum(md_import, henum);

    henum = NULL;
    /* EnumMethodsWithName with a NULL name is the same as EnumMethods. */
    hr = IMetaDataImport_EnumMethodsWithName(md_import, &henum, typedef2, NULL, NULL, 0, NULL);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    ok(!!henum, "got henum %p\n", henum);
    buf_len2 = 0;
    hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_len2 == buf_len, "got buf_len2 %lu != %lu\n", buf_len2, buf_len);
    IMetaDataImport_CloseEnum(md_import, henum);

    henum = NULL;
    hr = IMetaDataImport_EnumMembers(md_import, &henum, typedef2, NULL, 0, NULL);
    todo_wine ok(hr == S_FALSE, "got hr %#lx\n", hr);
    todo_wine ok(!!henum, "got henum %p\n", henum);
    buf_len2 = 0;
    hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(buf_len2 == buf_len, "got buf_len2 %lu != %lu\n", buf_len2, buf_len);
    IMetaDataImport_CloseEnum(md_import, henum);

    henum = NULL;
    hr = IMetaDataImport_EnumMembersWithName(md_import, &henum, typedef2, NULL, NULL, 0, NULL);
    todo_wine ok(hr == S_FALSE, "got hr %#lx\n", hr);
    todo_wine ok(!!henum, "got henum %p\n", henum);
    buf_len2 = 0;
    hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    todo_wine ok(buf_len2 == buf_len, "got buf_len2 %lu != %lu\n", buf_len2, buf_len);
    IMetaDataImport_CloseEnum(md_import, henum);

    for (i = 0; i < ARRAY_SIZE(field_enum_test_cases); i++)
    {
        const struct field_props *fields_props = field_enum_test_cases[i].field_props;
        const WCHAR *type_name = field_enum_test_cases[i].type_name;
        const ULONG fields_len = field_enum_test_cases[i].fields_len;
        ULONG field_idx;

        winetest_push_context("%s", debugstr_w(type_name));

        typedef1 = 0;
        hr = IMetaDataImport_FindTypeDefByName(md_import, type_name, 0, &typedef1);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        test_token(md_import, typedef1, mdtTypeDef, FALSE);
        henum = NULL;
        buf_count = 0xdeadbeef;
        hr = IMetaDataImport_EnumFields(md_import, &henum, typedef1, NULL, 0, &buf_count);
        ok(hr == S_FALSE, "got hr %#lx\n", hr);
        ok(!!henum, "got henum %p\n", henum);
        ok(buf_count == 0, "got buf_count %lu\n", buf_count);
        buf_len = 0;
        hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(buf_len == fields_len, "got buf_len %lu\n", buf_len);
        fielddef_tokens = calloc(buf_len, sizeof(*fielddef_tokens));
        ok(!!fielddef_tokens, "got fielddef_tokens %p\n", fielddef_tokens);
        hr = IMetaDataImport_EnumFields(md_import, &henum, typedef1, fielddef_tokens, buf_len, &buf_count);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(buf_count == buf_len, "got buf_count %lu != %lu\n", buf_count, buf_len);
        IMetaDataImport_CloseEnum(md_import, henum);

        henum = NULL;
        /* EnumFieldsWithName with a NULL name is the same as EnumFields. */
        hr = IMetaDataImport_EnumFieldsWithName(md_import, &henum, typedef1, NULL, NULL, 0, NULL);
        ok(hr == S_FALSE, "got hr %#lx\n", hr);
        ok(!!henum, "got henum %p\n", henum);
        buf_len2 = 0;
        hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(buf_len2 == buf_len, "got buf_len2 %lu != %lu\n", buf_len2, buf_len);
        IMetaDataImport_CloseEnum(md_import, henum);

        henum = NULL;
        hr = IMetaDataImport_EnumMembers(md_import, &henum, typedef1, NULL, 0, NULL);
        todo_wine ok(hr == S_FALSE, "got hr %#lx\n", hr);
        todo_wine ok(!!henum, "got henum %p\n", henum);
        buf_len2 = 0;
        hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(buf_len2 == buf_len, "got buf_len2 %lu != %lu\n", buf_len2, buf_len);
        IMetaDataImport_CloseEnum(md_import, henum);

        henum = NULL;
        hr = IMetaDataImport_EnumMembersWithName(md_import, &henum, typedef1, NULL, NULL, 0, NULL);
        todo_wine ok(hr == S_FALSE, "got hr %#lx\n", hr);
        todo_wine ok(!!henum, "got henum %p\n", henum);
        buf_len2 = 0;
        hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        todo_wine ok(buf_len2 == buf_len, "got buf_len2 %lu != %lu\n", buf_len2, buf_len);
        IMetaDataImport_CloseEnum(md_import, henum);

        for (field_idx = 0; field_idx < buf_len; field_idx++)
        {
            ULONG flags = 0, sig_len = 0, value_type = 0, value_len = 0;
            const struct field_props *props = &fields_props[field_idx];
            const COR_SIGNATURE *sig_blob = NULL;
            UVCP_CONSTANT value = NULL;
            WCHAR name[80];

            winetest_push_context("field_idx=%lu", field_idx);

            test_token(md_import, fielddef_tokens[field_idx], mdtFieldDef, FALSE);
            name[0] = L'\0';
            typedef2 = 0;
            hr = IMetaDataImport_GetFieldProps(md_import, fielddef_tokens[field_idx], &typedef2, name, ARRAY_SIZE(name), NULL,
                                               &flags, &sig_blob, &sig_len, &value_type, &value, &value_len);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(typedef2 == typedef1, "got typedef2 %s != %s\n", debugstr_mdToken(typedef2), debugstr_mdToken(typedef1));
            ok(!wcscmp(name, props->exp_name), "got name %s != %s\n", debugstr_w(name), debugstr_w(props->exp_name));
            ok(flags == props->exp_flags, "got flags %#lx != %#x\n", flags, props->exp_flags);
            ok(value_type == props->exp_value_type, "got value_type %#lx != %#x\n", value_type, props->exp_value_type);
            ok(sig_len == props->exp_sig_len, "got sig_len %lu != %lu\n", sig_len, props->exp_sig_len);
            ok(!!sig_blob, "got sig_blob %p\n", sig_blob);
            if (sig_blob && sig_len == props->exp_sig_len)
                ok(!memcmp(sig_blob, props->exp_sig_blob, sig_len), "got unexpected sig_blob\n");
            ok(value_len == 0, "got value_len %lu\n", value_len); /* Non-zero only for string types. */
            ok(props->has_value == !!value, "got value %s\n", debugstr_a(value));
            if (props->has_value)
                ok(value && !memcmp(value, props->exp_value, props->value_len), "got unexpected value %p\n", value);

            henum = NULL;
            hr = IMetaDataImport_EnumFieldsWithName(md_import, &henum, typedef2, name, &fielddef, 0, NULL);
            ok(hr == S_FALSE, "got hr %#lx\n", hr);
            ok(!!henum, "got henum %p\n", henum);
            buf_len2 = 0;
            hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(buf_len2 == 1, "got buf_len2 %lu\n", buf_len2);
            fielddef = mdFieldDefNil;
            hr = IMetaDataImport_EnumFieldsWithName(md_import, &henum, typedef2, name, &fielddef, 1, NULL);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(fielddef == fielddef_tokens[field_idx], "got fielddef %s != %s\n", debugstr_mdToken(fielddef),
               debugstr_mdToken(fielddef_tokens[field_idx]));
            IMetaDataImport_CloseEnum(md_import, henum);

            henum = NULL;
            fielddef = mdFieldDefNil;
            hr = IMetaDataImport_EnumMembersWithName(md_import, &henum, typedef2, name, &fielddef, 1, NULL);
            todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
            todo_wine ok(!!henum, "got henum %p\n", henum);
            buf_len2 = 0;
            hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len2);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            todo_wine ok(buf_len2 == 1, "got buf_len2 %lu\n", buf_len2);
            todo_wine ok(fielddef == fielddef_tokens[field_idx], "got fielddef %s != %s\n", debugstr_mdToken(fielddef),
                         debugstr_mdToken(fielddef_tokens[field_idx]));
            IMetaDataImport_CloseEnum(md_import, henum);

            fielddef = mdFieldDefNil;
            hr = IMetaDataImport_FindField(md_import, typedef2, name, sig_blob, sig_len, &fielddef);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(fielddef == fielddef_tokens[field_idx], "got fielddef %s != %s\n", debugstr_mdToken(fielddef),
               debugstr_mdToken(fielddef_tokens[field_idx]));
            hr = IMetaDataImport_FindField(md_import, typedef2, name, sig_blob, sig_len, &fielddef);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            fielddef = mdFieldDefNil;
            hr = IMetaDataImport_FindField(md_import, typedef2, name, NULL, 0, &fielddef);
            ok(hr == S_OK, "got hr %#lx\n", hr);
            ok(fielddef == fielddef_tokens[field_idx], "got fielddef %s != %s\n", debugstr_mdToken(fielddef),
               debugstr_mdToken(fielddef_tokens[field_idx]));

            token = mdTokenNil;
            hr = IMetaDataImport_FindMember(md_import, typedef2, name, sig_blob, sig_len, &token);
            todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
            todo_wine ok(token == fielddef_tokens[field_idx], "got token %s != %s\n", debugstr_mdToken(token),
                         debugstr_mdToken(fielddef_tokens[field_idx]));
            token = mdTokenNil;
            hr = IMetaDataImport_FindMember(md_import, typedef2, name, NULL, 0, &token);
            todo_wine ok(hr == S_OK, "got hr %#lx\n", hr);
            todo_wine ok(token == fielddef_tokens[field_idx], "got token %s != %s\n", debugstr_mdToken(token),
                         debugstr_mdToken(fielddef_tokens[field_idx]));

            winetest_pop_context();
        }
        free(fielddef_tokens);
        winetest_pop_context();
    }

    hr = IMetaDataImport_FindField(md_import, typedef2, NULL, NULL, 0, &fielddef);
    ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);
    hr = IMetaDataImport_FindField(md_import, typedef2, L"foo", NULL, 0, &fielddef);
    ok(hr == CLDB_E_RECORD_NOTFOUND, "got hr %#lx\n", hr);

    hr = IMetaDataImport_FindMember(md_import, typedef1, NULL, NULL, 0, &token);
    todo_wine ok(hr == E_INVALIDARG, "got hr %#lx\n", hr);
    hr = IMetaDataImport_FindMember(md_import, typedef1, L"foo", NULL, 0, &token);
    todo_wine ok(hr == CLDB_E_RECORD_NOTFOUND, "got hr %#lx\n", hr);

    typedef1 = buf_len = 0;
    data = NULL;
    hr = IMetaDataImport_FindTypeDefByName(md_import, L"Wine.Test.ITest2", 0, &typedef1);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    test_token(md_import, typedef1, mdtTypeDef, FALSE);
    hr = IMetaDataImport_GetCustomAttributeByName(md_import, typedef1, guid_attribute_name, &data, &buf_len);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(!!data, "got data %p\n", data);
    ok(buf_len == sizeof(GUID) + 4, "got buf_len %lu\n", buf_len);
    if (data && buf_len == sizeof(GUID) + 4)
    {
        guid = (GUID *)&data[2];
        ok(IsEqualGUID(guid, &IID_ITest2), "got guid %s\n", debugstr_guid(guid));
    }
    hr = IMetaDataImport_GetCustomAttributeByName(md_import, typedef1, guid_attribute_name, NULL, NULL);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    hr = IMetaDataImport_GetCustomAttributeByName(md_import, typedef1, NULL, &data, &buf_len);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    hr = IMetaDataImport_GetCustomAttributeByName(md_import, mdTypeDefNil, L"foo", &data, &buf_len);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    hr = IMetaDataImport_GetCustomAttributeByName(md_import, TokenFromRid(1, mdtCustomAttribute), L"foo", &data,
                                                  &buf_len);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);

    typedef1 = buf_len = 0;
    hr = IMetaDataImport_FindTypeDefByName(md_import, L"Wine.Test.ITest3", 0, &typedef1);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    henum = NULL;
    hr = IMetaDataImport_EnumProperties(md_import, &henum, typedef1, NULL, 0, NULL);
    ok(hr == S_FALSE, "got hr %#lx\n", hr);
    hr = IMetaDataImport_CountEnum(md_import, henum, &buf_len);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_len == ARRAY_SIZE(test3_props), "got buf_len %lu\n", buf_len);
    property_tokens = calloc(buf_len, sizeof(*property_tokens));
    ok(!!property_tokens, "got property_tokens %p\n", property_tokens);
    buf_count = 0xdeadbeef;
    hr = IMetaDataImport_EnumProperties(md_import, &henum, typedef1, property_tokens, buf_len, &buf_count);
    ok(hr == S_OK, "got hr %#lx\n", hr);
    ok(buf_count == buf_len, "got buf_count %lu != %lu\n", buf_count, buf_len);
    IMetaDataImport_CloseEnum(md_import, henum);
    for (i = 0; i < buf_len; i++)
    {
        ULONG sig_len = 0, value_type = 0, value_len = 0;
        const struct property *props = &test3_props[i];
        mdMethodDef get_method = 0, set_method = 0;
        const COR_SIGNATURE *sig_blob = NULL;
        UVCP_CONSTANT value = NULL;
        WCHAR name[80];

        winetest_push_context("i=%lu", i);

        name[0] = L'\0';
        str_reqd = 0;
        hr = IMetaDataImport_GetPropertyProps(md_import, property_tokens[i], &typedef2, name, ARRAY_SIZE(name),
                                              &str_reqd, &val, &sig_blob, &sig_len, &value_type, &value, &value_len,
                                              &set_method, &get_method, NULL, 0, NULL);
        ok(hr == S_OK, "got hr %#lx\n", hr);
        ok(typedef1 == typedef2, "got typedef1 %#x != %#x\n", typedef1, typedef2);
        ok(!wcscmp(name, props->exp_name), "got name %s != %s\n", debugstr_w(name), debugstr_w(props->exp_name));
        ok(!!sig_blob, "got sig_blob %p\n", sig_blob);
        ok(sig_len == props->exp_sig_len, "got sig_len %lu != %lu\n", sig_len, props->exp_sig_len);
        if (sig_blob && sig_len == props->exp_sig_len)
            ok(!memcmp(sig_blob, props->exp_sig_blob, sig_len), "got unexpected sig_blob\n");
        ok(!value_len, "got value_len %lu\n", value_len);
        ok(!value, "got value %p\n", value);
        if (props->has_get)
            test_prop_method_token(md_import, typedef1, props->exp_name, PROP_METHOD_GET, get_method);
        else
            ok(get_method == mdMethodDefNil, "got get_method %#x\n", get_method);
        if (props->has_set)
            test_prop_method_token(md_import, typedef1, props->exp_name, PROP_METHOD_SET, set_method);
        else
            ok(set_method == mdMethodDefNil, "got set_method %#x\n", set_method);
        winetest_pop_context();
    }
    IMetaDataImport_Release(md_import);
}

START_TEST(rometadata)
{
    HRESULT hr;

    hr = RoInitialize(RO_INIT_MULTITHREADED);
    ok(hr == S_OK, "RoInitialize failed, hr %#lx\n", hr);

    test_MetaDataGetDispenser();
    test_MetaDataDispenser_OpenScope();
    test_IMetaDataImport();

    RoUninitialize();
}
