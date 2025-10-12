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

extern HRESULT IMetaDataTables_create(const WCHAR *path, IMetaDataTables **iface);

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

extern HRESULT assembly_open_from_file(const WCHAR *path, assembly_t **out);
extern void assembly_free(assembly_t *assembly);
extern HRESULT assembly_get_table(const assembly_t *assembly, ULONG table_idx, struct metadata_table_info *info);
extern HRESULT assembly_get_column(const assembly_t *assembly, ULONG table_idx, ULONG column_idx, struct metadata_column_info *info);
extern ULONG assembly_get_heap_size(const assembly_t *assembly, enum heap_type heap);
extern const char *assembly_get_string(const assembly_t *assembly, ULONG idx);
extern HRESULT assembly_get_blob(const assembly_t *assembly, ULONG idx, const BYTE **blob, ULONG *size);
extern const GUID *assembly_get_guid(const assembly_t *assembly, ULONG idx);
extern ULONG metadata_coded_value_as_token(ULONG table_idx, ULONG column_idx, ULONG value);

#endif /* __WINE_ROMETADATA_PRIVATE__ */
