/*
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

#ifndef __WINE_ROMETADATA_PRIVATE__
#define __WINE_ROMETADATA_PRIVATE__

#include <rometadataapi.h>

extern HRESULT IMetaDataTables_create_from_file(const WCHAR *path, IMetaDataTables **iface);
extern HRESULT IMetaDataTables_create_from_data(const BYTE *data, ULONG data_size, IMetaDataTables **iface);

typedef struct assembly assembly_t;
struct metadata_table_info
{
    ULONG num_rows;
    ULONG num_columns;

    ULONG key_idx;

    ULONG row_size;
    const ULONG *column_sizes;

    const char *name;
    const BYTE *start;
};

struct metadata_column_info
{
    ULONG offset;
    ULONG size;
    ULONG type;
    const char *name;
};

enum heap_type
{
    HEAP_STRING      = 0,
    HEAP_GUID        = 1,
    HEAP_BLOB        = 2,
    HEAP_USER_STRING = 3
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

extern HRESULT assembly_open_from_file(const WCHAR *path, assembly_t **out);
extern HRESULT assembly_open_from_data(const BYTE *data, ULONG data_size, assembly_t **out);
extern void assembly_free(assembly_t *assembly);
extern HRESULT assembly_get_table(const assembly_t *assembly, ULONG table_idx, struct metadata_table_info *info);
extern HRESULT assembly_get_column(const assembly_t *assembly, ULONG table_idx, ULONG column_idx, struct metadata_column_info *info);
extern ULONG assembly_get_heap_size(const assembly_t *assembly, enum heap_type heap);
extern const char *assembly_get_string(const assembly_t *assembly, ULONG idx);
extern HRESULT assembly_get_blob(const assembly_t *assembly, ULONG idx, const BYTE **blob, ULONG *size);
extern const GUID *assembly_get_guid(const assembly_t *assembly, ULONG idx);
extern ULONG metadata_coded_value_as_token(ULONG table_idx, ULONG column_idx, ULONG value);

#endif /* __WINE_ROMETADATA_PRIVATE__ */
