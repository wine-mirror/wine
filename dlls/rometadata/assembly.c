/*
 * CIL assembly parser
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

#include "rometadatapriv.h"

#include <assert.h>
#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(rometadata);

#pragma pack(push, 1)

#define METADATA_MAGIC ('B' | ('S' << 8) | ('J' << 16) | ('B' << 24))

/* ECMA-335 Partition II.24.2.1, "Metadata root" */
struct metadata_hdr
{
    UINT32 signature;
    UINT16 major_version;
    UINT16 minor_version;
    UINT32 reserved;
    UINT32 length;
    char version[0];
};

/* ECMA-335 Partition II.24.2.2, "Stream header" */
struct stream_hdr
{
    UINT32 offset;
    UINT32 size;
    char name[0];
};

/* ECMA-335 Partition II.24.2.6, "#~ stream" */
struct stream_tables_hdr
{
    UINT32 reserved;
    UINT8 major_version;
    UINT8 minor_version;
    UINT8 heap_size_bits; /* enum heap_type */
    UINT8 reserved2;
    UINT64 valid_tables_bits;
    UINT64 sorted_tables_bits;
    UINT32 table_rows[0];
};

#pragma pack(pop)

struct metadata_stream
{
    UINT32 size;
    const BYTE *start;
};

enum table
{
    TABLE_MODULE                 = 0x00,
    TABLE_TYPEREF                = 0x01,
    TABLE_TYPEDEF                = 0x02,
    TABLE_FIELDPTR               = 0x03,
    TABLE_FIELD                  = 0x04,
    TABLE_METHODPTR              = 0x05,
    TABLE_METHODDEF              = 0x06,
    TABLE_PARAMPTR               = 0x07,
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
    TABLE_EVENTPTR               = 0x13,
    TABLE_EVENT                  = 0x14,
    TABLE_PROPERTYMAP            = 0x15,
    TABLE_PROPERTYPTR            = 0x16,
    TABLE_PROPERTY               = 0x17,
    TABLE_METHODSEMANTICS        = 0x18,
    TABLE_METHODIMPL             = 0x19,
    TABLE_MODULEREF              = 0x1a,
    TABLE_TYPESPEC               = 0x1b,
    TABLE_IMPLMAP                = 0x1c,
    TABLE_FIELDRVA               = 0x1d,
    TABLE_ENCLOG                 = 0x1e,
    TABLE_ENCMAP                 = 0x1f,
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

struct table_coded_idx
{
    UINT8 len;
    const enum table *tables;
};

enum table_column_type
{
    /* A basic data type */
    COLUMN_BASIC,
    /* An index into one of the heaps */
    COLUMN_HEAP_IDX,
    /* An index into a metadata table */
    COLUMN_TABLE_IDX,
    /* A coded index */
    COLUMN_CODED_IDX
};

union table_column_size
{
    UINT8 basic;
    enum heap_type heap;
    enum table table;
    struct table_coded_idx coded;
};

struct table_column
{
    enum table_column_type type;
    union table_column_size size;
    const char *name;
    BOOL primary_key;
};

/* We use TABLE_MAX for unused tags (See CustomAttribute). */
#define DEFINE_CODED_IDX(name, ...) static const enum table name##_tables[] = {__VA_ARGS__}

DEFINE_CODED_IDX(TypeDefOrRef, TABLE_TYPEDEF, TABLE_TYPEREF, TABLE_TYPESPEC);
DEFINE_CODED_IDX(HasConstant, TABLE_FIELD, TABLE_PARAM, TABLE_PROPERTY);
DEFINE_CODED_IDX(HasCustomAttribute,
                 TABLE_METHODDEF,
                 TABLE_FIELD,
                 TABLE_TYPEREF,
                 TABLE_TYPEDEF,
                 TABLE_PARAM,
                 TABLE_INTERFACEIMPL,
                 TABLE_MEMBERREF,
                 TABLE_MODULE,
                 TABLE_PROPERTY,
                 TABLE_EVENT,
                 TABLE_STANDALONESIG,
                 TABLE_MODULEREF,
                 TABLE_TYPESPEC,
                 TABLE_ASSEMBLY,
                 TABLE_ASSEMBLYREF,
                 TABLE_FILE,
                 TABLE_EXPORTEDTYPE,
                 TABLE_MANIFESTRESOURCE,
                 TABLE_GENERICPARAM,
                 TABLE_GENERICPARAMCONSTRAINT,
                 TABLE_METHODSPEC);
DEFINE_CODED_IDX(HasFieldMarshal, TABLE_FIELD, TABLE_PARAM);
DEFINE_CODED_IDX(HasDeclSecurity, TABLE_TYPEDEF, TABLE_METHODDEF, TABLE_ASSEMBLY);
DEFINE_CODED_IDX(MemberRefParent, TABLE_TYPEDEF, TABLE_TYPEREF, TABLE_MODULEREF, TABLE_METHODDEF, TABLE_TYPESPEC);
DEFINE_CODED_IDX(HasSemantics, TABLE_EVENT, TABLE_PROPERTY);
DEFINE_CODED_IDX(MethodDefOrRef, TABLE_METHODDEF, TABLE_MEMBERREF);
DEFINE_CODED_IDX(MemberForwarded, TABLE_FIELD, TABLE_METHODDEF);
DEFINE_CODED_IDX(Implementation, TABLE_FILE, TABLE_ASSEMBLYREF, TABLE_EXPORTEDTYPE);
DEFINE_CODED_IDX(CustomAttributeType, TABLE_MAX, TABLE_MAX, TABLE_METHODDEF, TABLE_MEMBERREF, TABLE_MAX);
DEFINE_CODED_IDX(ResolutionScope, TABLE_MODULE, TABLE_MODULEREF, TABLE_ASSEMBLYREF, TABLE_TYPEREF);
DEFINE_CODED_IDX(TypeOrMethodDef, TABLE_TYPEDEF, TABLE_METHODDEF);

#undef DEFINE_CODED_IDX

struct table_schema
{
    const ULONG columns_len;
    const struct table_column *const columns;
    const char *const name;
};

/* Partition II.22, "Metadata logical format: tables" */
#define DEFINE_TABLE_SCHEMA(name, ...) \
    static const struct table_column name##_columns[] = {__VA_ARGS__}; \
    static const struct table_schema name##_schema = { ARRAY_SIZE(name##_columns), name##_columns, #name }

#define BASIC(n, t) { COLUMN_BASIC, { .basic = sizeof(t) }, n }
#define HEAP(n, h) { COLUMN_HEAP_IDX, { .heap = (HEAP_##h) }, n }
#define TABLE(n, t) { COLUMN_TABLE_IDX, { .table = TABLE_##t }, n }
#define TABLE_PRIMARY(n, t) { COLUMN_TABLE_IDX, { .table= TABLE_##t }, n, TRUE }
#define CODED(n, c) { COLUMN_CODED_IDX, { .coded = { ARRAY_SIZE(c##_tables), c##_tables } }, n }
#define CODED_PRIMARY(n, c) { COLUMN_CODED_IDX, { .coded = { ARRAY_SIZE(c##_tables), c##_tables } }, n, TRUE }

/* Partition II.22.2, "Assembly" */
DEFINE_TABLE_SCHEMA(Assembly,
                    BASIC("HashAlgId", ULONG),
                    BASIC("MajorVersion", USHORT),
                    BASIC("MinorVersion", USHORT),
                    BASIC("BuildNumber", USHORT),
                    BASIC("RevisionNumber", USHORT),
                    BASIC("Flags", ULONG),
                    HEAP("PublicKey", BLOB),
                    HEAP("Name", STRING),
                    HEAP("Locale", STRING));

/* Partition II.22.3, "AssemblyOS" */
DEFINE_TABLE_SCHEMA(AssemblyOS,
                    BASIC("OSPlatformId", ULONG),
                    BASIC("OSMajorVersion", ULONG),
                    BASIC("OSMinorVersion", ULONG));

/* Partition II.2.4, "AssemblyProcessor" */
DEFINE_TABLE_SCHEMA(AssemblyProcessor, BASIC("Processor", ULONG));

/* Partition II.2.5, "AssemblyRef" */
DEFINE_TABLE_SCHEMA(AssemblyRef,
                    BASIC("MajorVersion", USHORT),
                    BASIC("MinorVersion", USHORT),
                    BASIC("BuildNumber", USHORT),
                    BASIC("RevisionNumber", USHORT),
                    BASIC("Flags", ULONG),
                    HEAP("PublicKeyOrToken", BLOB),
                    HEAP("Name", STRING),
                    HEAP("Locale", STRING),
                    HEAP("HashValue", BLOB));

/* Partition II.2.6, "AssemblyRefOS" */
DEFINE_TABLE_SCHEMA(AssemblyRefOS,
                    BASIC("OSPlatformId", ULONG),
                    BASIC("OSMajorVersion", ULONG),
                    BASIC("OSMinorVersion", ULONG),
                    TABLE("AssemblyRef", ASSEMBLYREF));

/* Partition II.2.7, "AssemblyRefProcessor" */
DEFINE_TABLE_SCHEMA(AssemblyRefProcessor, BASIC("Processor", ULONG), TABLE("AssemblyRef", ASSEMBLYREF));

/* Partition II.2.8, "ClassLayout" */
DEFINE_TABLE_SCHEMA(ClassLayout,
                    BASIC("PackingSize", USHORT),
                    BASIC("ClassSize", ULONG),
                    TABLE_PRIMARY("Parent", TYPEDEF));

/* Partition II.22.9, "Constant" */
DEFINE_TABLE_SCHEMA(Constant, BASIC("Type", USHORT), CODED_PRIMARY("Parent", HasConstant), HEAP("Value", BLOB));

/* Partition II.2.10, "CustomAttribute" */
DEFINE_TABLE_SCHEMA(CustomAttribute,
                    CODED_PRIMARY("Parent", HasCustomAttribute),
                    CODED("Type", CustomAttributeType),
                    HEAP("Value", BLOB));

/* Partition II.22.11, "DeclSecurity" */
DEFINE_TABLE_SCHEMA(DeclSecurity,
                    BASIC("Action", SHORT),
                    CODED_PRIMARY("Parent", HasDeclSecurity),
                    HEAP("PermissionSet", BLOB));

DEFINE_TABLE_SCHEMA(ENCLog, BASIC("Token", ULONG), BASIC("FuncCode", ULONG));

DEFINE_TABLE_SCHEMA(ENCMap, BASIC("Token", ULONG));

/* Partition II.22.12, "EventMap" */
DEFINE_TABLE_SCHEMA(EventMap, TABLE("Parent", TYPEDEF), TABLE("EventList", EVENT));

/* Partition II.22.13, "Event" */
DEFINE_TABLE_SCHEMA(Event, BASIC("EventFlags", USHORT), HEAP("Name", STRING), CODED("EventType", TypeDefOrRef));

DEFINE_TABLE_SCHEMA(EventPtr, TABLE("Event", EVENT));

/* Partition II.22.14, "ExportedType" */
DEFINE_TABLE_SCHEMA(ExportedType,
                    BASIC("Flags", ULONG),
                    BASIC("TypeDefId", ULONG),
                    HEAP("TypeName", STRING),
                    HEAP("TypeNamespace", STRING),
                    CODED("Implementation", Implementation));

/* Partition II.22.15, "Field" */
DEFINE_TABLE_SCHEMA(Field, BASIC("Flags", USHORT), HEAP("Name", STRING), HEAP("Signature", BLOB));

/* Partition II.22.16, "FieldLayout" */
DEFINE_TABLE_SCHEMA(FieldLayout, BASIC("OffSet", ULONG), TABLE_PRIMARY("Field", FIELD));

/* Partition II.22.17, "FieldMarshal" */
DEFINE_TABLE_SCHEMA(FieldMarshal, CODED_PRIMARY("Parent", HasFieldMarshal), HEAP("NativeType", BLOB));

DEFINE_TABLE_SCHEMA(FieldPtr, TABLE("Field", FIELD));

/* Partition II.22.18, "FieldRVA" */
DEFINE_TABLE_SCHEMA(FieldRVA, BASIC("RVA", ULONG), TABLE_PRIMARY("Field", FIELD));

/* Partition II.22.19, "File" */
DEFINE_TABLE_SCHEMA(File, BASIC("Flags", ULONG), HEAP("Name", STRING), HEAP("HashValue", BLOB));

/* Partition II.22.20, "GenericParam" */
DEFINE_TABLE_SCHEMA(GenericParam,
                    BASIC("Number", USHORT),
                    BASIC("Flags", USHORT),
                    CODED_PRIMARY("Owner", TypeOrMethodDef),
                    HEAP("Name", STRING));

/* Partition II.22.21, "GenericParamConstraint" */
DEFINE_TABLE_SCHEMA(GenericParamConstraint, TABLE_PRIMARY("Owner", GENERICPARAM), CODED("Constraint", TypeDefOrRef));

/* Partition II.22.22, "ImplMap" */
DEFINE_TABLE_SCHEMA(ImplMap,
                    BASIC("MappingFlags", USHORT),
                    CODED_PRIMARY("MemberForwarded", MemberForwarded),
                    HEAP("ImportName", STRING),
                    TABLE("ImportScope", MODULEREF));

/* Partition II.22.23, "InterfaceImplMap" */
DEFINE_TABLE_SCHEMA(InterfaceImpl, TABLE_PRIMARY("Class", TYPEDEF), CODED("Interface", TypeDefOrRef));

/* Partition II.22.24, "ManifestResource" */
DEFINE_TABLE_SCHEMA(ManifestResource,
                    BASIC("Offset", ULONG),
                    BASIC("Flags", ULONG),
                    HEAP("Name", STRING),
                    CODED("Implementation", Implementation));

/* Partition II.22.25, "MemberRef" */
DEFINE_TABLE_SCHEMA(MemberRef, CODED("Class", MemberRefParent), HEAP("Name", STRING), HEAP("Signature", BLOB));

/* Partition II.22.26, "MethodDef" */
DEFINE_TABLE_SCHEMA(Method,
                    BASIC("RVA", ULONG),
                    BASIC("ImplFlags", USHORT),
                    BASIC("Flags", USHORT),
                    HEAP("Name", STRING),
                    HEAP("Signature", BLOB),
                    TABLE("ParamList", PARAM));

/* Partition II.22.27, "MethodImpl" */
DEFINE_TABLE_SCHEMA(MethodImpl,
                    TABLE_PRIMARY("Class", TYPEDEF),
                    CODED("MethodBody", MethodDefOrRef),
                    CODED("MethodDeclaration", MethodDefOrRef));

DEFINE_TABLE_SCHEMA(MethodPtr, TABLE("Method", METHODDEF));

/* Partition II.22.28, "MethodSemantics" */
DEFINE_TABLE_SCHEMA(MethodSemantics,
                    BASIC("Semantic", USHORT),
                    TABLE("Method", METHODDEF),
                    CODED_PRIMARY("Association", HasSemantics));

/* Partition II.22.29, "MethodSpec" */
DEFINE_TABLE_SCHEMA(MethodSpec, CODED("Method", MethodDefOrRef), HEAP("Instantiation", BLOB));

/* Partition II.22.30, "Module" */
DEFINE_TABLE_SCHEMA(Module,
                    BASIC("Generation", USHORT),
                    HEAP("Name", STRING),
                    HEAP("Mvid", GUID),
                    HEAP("EncId", GUID),
                    HEAP("EncBaseId", GUID));

/* Partition II.22.31, "ModuleRef"*/
DEFINE_TABLE_SCHEMA(ModuleRef, HEAP("Name", STRING));

/* Partition II.22.32, "NestedClass" */
DEFINE_TABLE_SCHEMA(NestedClass, TABLE_PRIMARY("NestedClass", TYPEDEF), TABLE("EnclosingClass", TYPEDEF));

/* Partition II.22.33, "Param" */
DEFINE_TABLE_SCHEMA(Param, BASIC("Flags", USHORT), BASIC("Sequence", USHORT), HEAP("Name", STRING));

DEFINE_TABLE_SCHEMA(ParamPtr, TABLE("Param", PARAM));

/* Partition II.22.34, "Property" */
DEFINE_TABLE_SCHEMA(Property, BASIC("PropFlags", USHORT), HEAP("Name", STRING), HEAP("Type", BLOB));

/* Partition II.22.35, "PropertyMap" */
DEFINE_TABLE_SCHEMA(PropertyMap, TABLE("Parent", TYPEDEF), TABLE("PropertyList", PROPERTY));

DEFINE_TABLE_SCHEMA(PropertyPtr, TABLE("Property", PROPERTY));

/* Partition II.22.36, "StandAloneSig" */
DEFINE_TABLE_SCHEMA(StandAloneSig, HEAP("Signature", BLOB));

/* Partition II.22.37, "TypeDef" */
DEFINE_TABLE_SCHEMA(TypeDef,
                    BASIC("Flags", ULONG),
                    HEAP("Name", STRING),
                    HEAP("Namespace", STRING),
                    CODED("Extends", TypeDefOrRef),
                    TABLE("FieldList", FIELD),
                    TABLE("MethodList", METHODDEF));

/* Partition II.22.38, "TypeRef" */
DEFINE_TABLE_SCHEMA(TypeRef, CODED("ResolutionScope", ResolutionScope), HEAP("Name", STRING), HEAP("Namespace", STRING));

/* Partition II.22.39, "TypeSpec" */
DEFINE_TABLE_SCHEMA(TypeSpec, HEAP("Signature", BLOB));

#undef DEFINE_TABLE_SCHEMA
#undef BASIC
#undef HEAP
#undef TABLE
#undef TABLE_PRIMARY
#undef CODED
#undef CODED_PRIMARY

static const struct table_schema *table_schemas[] = {
    [TABLE_MODULE] = &Module_schema,
    [TABLE_TYPEREF] = &TypeRef_schema,
    [TABLE_TYPEDEF] = &TypeDef_schema,
    [TABLE_FIELDPTR] = &FieldPtr_schema,
    [TABLE_FIELD] = &Field_schema,
    [TABLE_METHODPTR] = &MethodPtr_schema,
    [TABLE_METHODDEF] = &Method_schema,
    [TABLE_PARAMPTR] = &ParamPtr_schema,
    [TABLE_PARAM] = &Param_schema,
    [TABLE_INTERFACEIMPL] = &InterfaceImpl_schema,
    [TABLE_MEMBERREF] = &MemberRef_schema,
    [TABLE_CONSTANT] = &Constant_schema,
    [TABLE_CUSTOMATTRIBUTE] = &CustomAttribute_schema,
    [TABLE_FIELDMARSHAL] = &FieldMarshal_schema,
    [TABLE_DECLSECURITY] = &DeclSecurity_schema,
    [TABLE_CLASSLAYOUT] = &ClassLayout_schema,
    [TABLE_FIELDLAYOUT] = &FieldLayout_schema,
    [TABLE_STANDALONESIG] = &StandAloneSig_schema,
    [TABLE_EVENTMAP] = &EventMap_schema,
    [TABLE_EVENTPTR] = &EventPtr_schema,
    [TABLE_EVENT] = &Event_schema,
    [TABLE_PROPERTYMAP] = &PropertyMap_schema,
    [TABLE_PROPERTYPTR] = &PropertyPtr_schema,
    [TABLE_PROPERTY] = &Property_schema,
    [TABLE_METHODSEMANTICS] = &MethodSemantics_schema,
    [TABLE_METHODIMPL] = &MethodImpl_schema,
    [TABLE_MODULEREF] = &ModuleRef_schema,
    [TABLE_TYPESPEC] = &TypeSpec_schema,
    [TABLE_IMPLMAP] = &ImplMap_schema,
    [TABLE_FIELDRVA] = &FieldRVA_schema,
    [TABLE_ENCLOG] = &ENCLog_schema,
    [TABLE_ENCMAP] = &ENCMap_schema,
    [TABLE_ASSEMBLY] = &Assembly_schema,
    [TABLE_ASSEMBLYPROCESSOR] = &AssemblyProcessor_schema,
    [TABLE_ASSEMBLYOS] = &AssemblyOS_schema,
    [TABLE_ASSEMBLYREF] = &AssemblyRef_schema,
    [TABLE_ASSEMBLYREFPROCESSOR] = &AssemblyRefProcessor_schema,
    [TABLE_ASSEMBLYREFOS] = &AssemblyRefOS_schema,
    [TABLE_FILE] = &File_schema,
    [TABLE_EXPORTEDTYPE] = &ExportedType_schema,
    [TABLE_MANIFESTRESOURCE] = &ManifestResource_schema,
    [TABLE_NESTEDCLASS] = &NestedClass_schema,
    [TABLE_GENERICPARAM] = &GenericParam_schema,
    [TABLE_METHODSPEC] = &MethodSpec_schema,
    [TABLE_GENERICPARAMCONSTRAINT] = &GenericParamConstraint_schema
};
C_ASSERT(ARRAY_SIZE(table_schemas) == TABLE_MAX);

struct table_info
{
    const BYTE *start;
    ULONG num_rows;
    ULONG row_size;
    ULONG *columns_size;
};

struct assembly
{
    HANDLE file;
    HANDLE map;
    const BYTE *data;
    UINT64 size;

    struct metadata_stream stream_tables;
    struct metadata_stream stream_strings;
    struct metadata_stream stream_blobs;
    struct metadata_stream stream_guids;
    struct metadata_stream stream_user_strings;

    const struct stream_tables_hdr *tables_hdr;
    struct table_info tables[TABLE_MAX];
};

static BOOL pe_rva_to_offset(const IMAGE_SECTION_HEADER *sections, UINT32 num_sections, UINT32 rva, UINT32 *offset)
{
    UINT32 i;

    for (i = 0; i < num_sections; i++)
    {
        if (rva >= sections[i].VirtualAddress && rva < (sections[i].VirtualAddress + sections[i].Misc.VirtualSize))
        {
            *offset = rva - sections[i].VirtualAddress + sections[i].PointerToRawData;
            return TRUE;
        }
    }
    return FALSE;
}

static UINT8 assembly_heap_idx_size(const assembly_t *assembly, enum heap_type type)
{
    assert(type >= HEAP_STRING && type <= HEAP_BLOB);
    /* ECMA-335 Partition II.24.2.6, "Heap size flag" */
    return (assembly->tables_hdr->heap_size_bits & (1 << type)) ? 4 : 2;
}

static BOOL assembly_table_exists(const assembly_t *assembly, UINT8 idx)
{
    assert(idx < 64);
    return !!(assembly->tables_hdr->valid_tables_bits & (1ULL << idx));
}

static ULONG index_size(ULONG n)
{
    return n < (1 << 16) ? 2 : 4;
}

static ULONG assembly_get_table_index_size(const assembly_t *assembly, enum table table)
{
    assert(table < TABLE_MAX && table_schemas[table]);
    return index_size(assembly->tables[table].num_rows);
}

static ULONG bit_width(ULONG n)
{
    ULONG bits = 1;

    for (n = n - 1; n; n >>= 1)
        bits++;
    return bits;
}

/* From Partition II.24.2.6, "#~stream":
 *
 * If e is a coded index that points into table t_i out of n possible tables t_0,...,t_{n-1}, then it
 * is stored as e << (log n) | tag{ t_0, ..., t_{n-1} }[ t_i ] using 2 bytes if the maximum number
 * of rows of tables t_0,...,t_{n-1}, is less than 2 (16 – (log n)), and using 4 bytes otherwise.
 */
static ULONG assembly_get_coded_index_size(const assembly_t *assembly, const struct table_coded_idx *coded)
{
    const ULONG tag_bits = bit_width(coded->len);
    ULONG max_row_idx = 0, i;

    for (i = 0; i < coded->len; i++)
    {
        if (coded->tables[i] != TABLE_MAX)
            max_row_idx = max(max_row_idx, assembly->tables[coded->tables[i]].num_rows);
    }
    return max_row_idx < (1 << (16 - tag_bits)) ? 2 : 4;
}

static HRESULT assembly_calculate_table_sizes(assembly_t *assembly, enum table table)
{
    const struct table_schema *schema;
    struct table_info *table_info;
    int i;

    assert(table < TABLE_MAX);
    schema = table_schemas[table];
    table_info = &assembly->tables[table];
    if (!(table_info->columns_size = calloc(sizeof(*table_info->columns_size), schema->columns_len)))
        return E_OUTOFMEMORY;

    for (i = 0; i < schema->columns_len; i++)
    {
        const struct table_column *column = &schema->columns[i];
        ULONG column_size;

        switch (column->type)
        {
        case COLUMN_BASIC:
            column_size = column->size.basic;
            break;
        case COLUMN_HEAP_IDX:
            column_size = assembly_heap_idx_size(assembly, column->size.heap);
            break;
        case COLUMN_TABLE_IDX:
            column_size = assembly_get_table_index_size(assembly, table);
            break;
        case COLUMN_CODED_IDX:
            column_size = assembly_get_coded_index_size(assembly, &column->size.coded);
            break;
        DEFAULT_UNREACHABLE;
        }
        table_info->columns_size[i] = column_size;
        table_info->row_size += column_size;
    }
    return S_OK;
}

static HRESULT assembly_parse_metadata_tables(assembly_t *assembly)
{
    const BYTE *cur_table, *tables_stream_start = assembly->stream_tables.start;
    ULONG num_tables = 0;
    int i;

    for (i = 0; i < 64; i++)
    {
        if (!assembly_table_exists(assembly, i)) continue;
        if (i >= TABLE_MAX) return E_INVALIDARG;
        assert(table_schemas[i]);
        num_tables++;
    }

    cur_table = tables_stream_start + offsetof(struct stream_tables_hdr, table_rows[num_tables]);
    if ((UINT_PTR)(cur_table - assembly->data) > assembly->size) return E_INVALIDARG;

    num_tables = 0;
    for (i = 0; i < TABLE_MAX; i++)
    {
        if (!assembly_table_exists(assembly, i)) continue;
        assembly->tables[i].num_rows = assembly->tables_hdr->table_rows[num_tables++];
    }

    for (i = 0; i < TABLE_MAX; i++)
    {
        struct table_info *table = &assembly->tables[i];

        assembly_calculate_table_sizes(assembly, i);
        if (assembly_table_exists(assembly, i))
        {
            table->start = cur_table;
            cur_table += (size_t)(assembly->tables[i].num_rows * table->row_size);
            if ((UINT_PTR)(cur_table - assembly->data) > assembly->size) return E_INVALIDARG;
        }
    }
    return S_OK;
}

static HRESULT assembly_parse_headers(assembly_t *assembly)
{
    const IMAGE_DOS_HEADER *dos_hdr = (IMAGE_DOS_HEADER *)assembly->data;
    const IMAGE_SECTION_HEADER *sections;
    const IMAGE_NT_HEADERS32 *nt_hdrs;
    const IMAGE_COR20_HEADER *cor_hdr;
    const struct metadata_hdr *md_hdr;
    const BYTE *streams_cur, *md_start, *ptr;
    UINT32 rva, num_sections, offset;
    UINT8 num_streams, i;

    if (assembly->size < sizeof(IMAGE_DOS_HEADER) || dos_hdr->e_magic != IMAGE_DOS_SIGNATURE ||
        assembly->size < (dos_hdr->e_lfanew + sizeof(IMAGE_NT_HEADERS32)))
        return E_INVALIDARG;

    nt_hdrs = (IMAGE_NT_HEADERS32 *)(assembly->data + dos_hdr->e_lfanew);
    if (!(num_sections = nt_hdrs->FileHeader.NumberOfSections)) return E_INVALIDARG;
    switch (nt_hdrs->OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        rva = nt_hdrs->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
        sections = (IMAGE_SECTION_HEADER *)(assembly->data + dos_hdr->e_lfanew + sizeof(IMAGE_NT_HEADERS32));
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
    {
        const IMAGE_NT_HEADERS64 *hdr64 = (IMAGE_NT_HEADERS64 *)(assembly->data + dos_hdr->e_lfanew);

        if (dos_hdr->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > assembly->size) return E_INVALIDARG;
        rva = hdr64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR].VirtualAddress;
        sections = (IMAGE_SECTION_HEADER *)(assembly->data + dos_hdr->e_lfanew + sizeof(IMAGE_NT_HEADERS64));
        break;
    }
    default:
        return E_INVALIDARG;
    }

    if (!pe_rva_to_offset(sections, num_sections, rva, &offset) || offset + sizeof(IMAGE_COR20_HEADER) > assembly->size)
        return E_INVALIDARG;

    cor_hdr = (IMAGE_COR20_HEADER *)(assembly->data + offset);
    if (cor_hdr->cb != sizeof(IMAGE_COR20_HEADER)) return E_INVALIDARG;
    if (!(pe_rva_to_offset(sections, num_sections, cor_hdr->MetaData.VirtualAddress, &offset)) ||
        offset + sizeof(struct metadata_hdr) > assembly->size)
        return E_INVALIDARG;

    md_start = assembly->data + offset;
    md_hdr = (struct metadata_hdr *)md_start;
    if (md_hdr->signature != METADATA_MAGIC ||
        offset + offsetof(struct metadata_hdr, version[md_hdr->length]) + sizeof(UINT16) * 2 > assembly->size)
        return E_INVALIDARG;

    num_streams = *(UINT8 *)(md_start + offsetof(struct metadata_hdr, version[md_hdr->length]) + sizeof(UINT16)); /* Flags */
    streams_cur = md_start + offsetof(struct metadata_hdr, version[md_hdr->length]) + sizeof(UINT16) * 2; /* Flags + Streams */

    for (i = 0; i < num_streams; i++)
    {
        const struct stream_hdr *md_stream_hdr = (struct stream_hdr *)streams_cur;
        const struct
        {
            const char *name;
            DWORD name_len;
            struct metadata_stream *stream;
        } streams[] =
        {
            { "#~", 2, &assembly->stream_tables },
            { "#Strings", 8, &assembly->stream_strings },
            { "#Blob", 5, &assembly->stream_blobs },
            { "#GUID", 5, &assembly->stream_guids },
            { "#US", 3, &assembly->stream_user_strings }
        };
        HRESULT hr = E_INVALIDARG;
        int j;

        if ((UINT_PTR)(streams_cur - assembly->data) > assembly->size) return E_INVALIDARG;
        for (j = 0; j < ARRAY_SIZE(streams); j++)
        {
            if (!strncmp(streams[j].name, md_stream_hdr->name, streams[j].name_len))
            {
                if (md_stream_hdr->offset + md_stream_hdr->size <= assembly->size)
                {
                    streams[j].stream->size = md_stream_hdr->size;
                    streams[j].stream->start = md_start + md_stream_hdr->offset;
                    hr = S_OK;
                }
                break;
            }
        }
        if (FAILED(hr)) return hr;
        /* The stream name is padded to the next 4-byte boundary (ECMA-335 Partition II.24.2.2) */
        streams_cur += offsetof(struct stream_hdr, name[(streams[j].name_len + 4) & ~3]);
    }

    /* IMetaDataTables::GetStringHeapSize returns the string heap size without the nul byte padding.
     * Partition II.24.2.3 says that if there is a string heap, the first entry is always the empty string, so
     * we'll only subtract padding bytes as long as stream_strings.size > 1. */
    ptr = assembly->stream_strings.start + assembly->stream_strings.size - 1;
    while (assembly->stream_strings.size > 1 && !ptr[0] && !ptr[-1])
    {
        assembly->stream_strings.size--;
        ptr--;
    }

    if (assembly->stream_tables.size < sizeof(struct stream_tables_hdr)) return E_INVALIDARG;
    assembly->tables_hdr = (struct stream_tables_hdr *)assembly->stream_tables.start;

    return assembly_parse_metadata_tables(assembly);
}

HRESULT assembly_open_from_file(const WCHAR *path, assembly_t **ret)
{
    assembly_t *assembly;
    HRESULT hr = S_OK;

    TRACE("(%s, %p)\n", debugstr_w(path), ret);

    if (!(assembly = calloc(1, sizeof(*assembly)))) return E_OUTOFMEMORY;
    assembly->file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (assembly->file == INVALID_HANDLE_VALUE)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
    if (!(assembly->map = CreateFileMappingW(assembly->file, NULL, PAGE_READONLY, 0, 0, NULL)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
    if (!(assembly->data = MapViewOfFile(assembly->map, FILE_MAP_READ, 0, 0, 0)))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }
    if (!GetFileSizeEx(assembly->file, (LARGE_INTEGER *)&assembly->size))
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto done;
    }

    hr = assembly_parse_headers(assembly);

done:
    if (FAILED(hr))
        assembly_free(assembly);
    else
        *ret = assembly;
    return hr;
}

void assembly_free(assembly_t *assembly)
{
    ULONG i;

    for (i = 0; i < TABLE_MAX; i++) free(assembly->tables[i].columns_size);
    if (assembly->map) UnmapViewOfFile(assembly->map);
    CloseHandle(assembly->map);
    CloseHandle(assembly->file);
    free(assembly);
}

HRESULT assembly_get_table(const assembly_t *assembly, ULONG table_idx, struct metadata_table_info *info)
{
    const struct table_schema *schema;
    const struct table_info *table;
    ULONG i;

    if (table_idx >= TABLE_MAX) return E_INVALIDARG;

    schema = table_schemas[table_idx];
    table = &assembly->tables[table_idx];
    info->key_idx = -1;
    for (i = 0; i < schema->columns_len; i++)
    {
        if (schema->columns[i].primary_key)
        {
            info->key_idx = i;
            break;
        }
    }
    info->num_rows = table->num_rows;
    info->num_columns = schema->columns_len;
    info->row_size = table->row_size;
    info->column_sizes = table->columns_size;
    info->name = schema->name;
    info->start = table->start;
    return S_OK;
}

ULONG assembly_get_heap_size(const assembly_t *assembly, enum heap_type heap)
{
    switch (heap)
    {
    case HEAP_BLOB:
        return assembly->stream_blobs.size;
    case HEAP_STRING:
        return assembly->stream_strings.size;
    case HEAP_GUID:
        return assembly->stream_guids.size;
    case HEAP_USER_STRING:
        return assembly->stream_user_strings.size;
    DEFAULT_UNREACHABLE;
    }
}

const char *assembly_get_string(const assembly_t *assembly, ULONG idx)
{
    return idx < assembly->stream_strings.size ? (const char *)&assembly->stream_strings.start[idx] : NULL;
}

static HRESULT decode_int(const BYTE *encoded, ULONG *val, ULONG *len)
{
    if (!(encoded[0] & 0x80))
    {
        *len = 1;
        *val = encoded[0];
    }
    else if (!(encoded[0] & 0x40))
    {
        *len = 2;
        *val = ((encoded[0] & ~0xc0) << 8) + encoded[1];
    }
    else if (!(encoded[0] & 0x20))
    {
        *len = 4;
        *val = ((encoded[0] & ~0xe0) << 24) + (encoded[1] << 16) + (encoded[2] << 8) + encoded[3];
    }
    else
        return E_INVALIDARG;
    return S_OK;
}

HRESULT assembly_get_blob(const assembly_t *assembly, ULONG idx, const BYTE **blob, ULONG *size)
{
    const BYTE *ptr;
    ULONG size_len;
    HRESULT hr;

    if (idx >= assembly->stream_blobs.size) return E_INVALIDARG;
    ptr = assembly->stream_blobs.start + idx;
    if (FAILED(hr = decode_int(ptr, size, &size_len))) return hr;
    *blob = ptr + size_len;
    return S_OK;
}

const GUID *assembly_get_guid(const assembly_t *assembly, ULONG idx)
{
    ULONG offset = (idx - 1) * sizeof(GUID); /* Indices into the GUID heap are 1-based */

    return offset < assembly->stream_guids.size ? (const GUID *)(assembly->stream_guids.start + offset) : NULL;
}
