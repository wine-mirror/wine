/*
 * Copyright 2024, 2025 Hans Leidekker for CodeWeavers
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

#include "config.h"
#include <stdarg.h>
#include <stdio.h>

#include "widl.h"
#include "windef.h"
#include "winbase.h"
#include "wincrypt.h"
#include "winnt.h"
#include "utils.h"
#include "typetree.h"

static const IMAGE_DOS_HEADER dos_header =
{
    .e_magic = IMAGE_DOS_SIGNATURE,
    .e_lfanew = sizeof(dos_header),
};

#define FILE_ALIGNMENT 0x200
#define SECTION_ALIGNMENT 0x1000
static IMAGE_NT_HEADERS32 nt_header =
{
    .Signature = IMAGE_NT_SIGNATURE,
    .FileHeader =
    {
        .Machine = IMAGE_FILE_MACHINE_I386,
        .NumberOfSections = 1,
        .SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32),
        .Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_DLL
    },
    .OptionalHeader =
    {
        .Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC,
        .MajorLinkerVersion = 11,
        .ImageBase = 0x400000,
        .SectionAlignment = SECTION_ALIGNMENT,
        .FileAlignment = FILE_ALIGNMENT,
        .MajorOperatingSystemVersion = 6,
        .MinorOperatingSystemVersion = 2,
        .MajorSubsystemVersion = 6,
        .MinorSubsystemVersion = 2,
        .SizeOfHeaders = FILE_ALIGNMENT,
        .Subsystem = IMAGE_SUBSYSTEM_WINDOWS_CUI,
        .DllCharacteristics = IMAGE_DLLCHARACTERISTICS_NO_SEH | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE |
                              IMAGE_DLLCHARACTERISTICS_NX_COMPAT,
        .SizeOfStackReserve = 0x100000,
        .SizeOfHeapReserve = 0x1000,
        .LoaderFlags = 0x100000,
        .NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES,
        .DataDirectory[IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR] =
            { .VirtualAddress = SECTION_ALIGNMENT, .Size = sizeof(IMAGE_COR20_HEADER) }
    }
};

static IMAGE_SECTION_HEADER section_header =
{
    .Name = ".text",
    .Characteristics = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_READ
};

static IMAGE_COR20_HEADER cor_header =
{
    .cb = sizeof(IMAGE_COR20_HEADER),
    .MajorRuntimeVersion = 2,
    .MinorRuntimeVersion = 5,
    .Flags = COMIMAGE_FLAGS_ILONLY
};

#define METADATA_MAGIC ('B' | ('S' << 8) | ('J' << 16) | ('B' << 24))
static struct
{
    UINT   signature;
    USHORT major_version;
    USHORT minor_version;
    UINT   reserved;
    UINT   length;
    char   version[20];
    USHORT flags;
    USHORT num_streams;
}
metadata_header =
{
    METADATA_MAGIC,
    1,
    1,
    0,
    20,
    "WindowsRuntime 1.4"
};

enum
{
    WINMD_STREAM_TABLE,
    WINMD_STREAM_STRING,
    WINMD_STREAM_USERSTRING,
    WINMD_STREAM_GUID,
    WINMD_STREAM_BLOB,
    WINMD_STREAM_MAX
};

static struct
{
    UINT data_offset;
    UINT data_size;
    char name[12];
    UINT header_size;
    const BYTE *data;
}
streams[] =
{
    { 0, 0, "#~", 12 },
    { 0, 0, "#Strings", 20 },
    { 0, 0, "#US", 12 },
    { 0, 0, "#GUID", 16 },
    { 0, 0, "#Blob", 16 }
};

static void write_headers( UINT image_size )
{
    static const BYTE pad[8];
    UINT i, streams_size = 0;
    USHORT num_streams = 0;

    put_data( &dos_header, sizeof(dos_header) );

    image_size += nt_header.OptionalHeader.SizeOfHeaders + sizeof(section_header);
    nt_header.OptionalHeader.SizeOfImage = (image_size + 0x1fff) & ~0x1fff;

    put_data( &nt_header, sizeof(nt_header) );

    for (i = 0; i < WINMD_STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        streams_size += streams[i].header_size + streams[i].data_size;
        num_streams++;
    }

    section_header.PointerToRawData = FILE_ALIGNMENT;
    section_header.VirtualAddress = SECTION_ALIGNMENT;
    section_header.Misc.VirtualSize = sizeof(cor_header) + sizeof(metadata_header) + streams_size + 8;
    section_header.SizeOfRawData = (section_header.Misc.VirtualSize + FILE_ALIGNMENT - 1) & ~(FILE_ALIGNMENT - 1);

    put_data( &section_header, sizeof(section_header) );

    for (i = 0; i < FILE_ALIGNMENT - sizeof(dos_header) - sizeof(nt_header) - sizeof(section_header); i++)
        put_data( pad, 1 );

    cor_header.MetaData.VirtualAddress = section_header.VirtualAddress + sizeof(cor_header) + 8;
    cor_header.MetaData.Size = sizeof(metadata_header) + streams_size;

    put_data( &cor_header, sizeof(cor_header) );
    put_data( pad, 8 );

    metadata_header.num_streams = num_streams;
    put_data( &metadata_header, sizeof(metadata_header) );
    for (i = 0; i < WINMD_STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        put_data( &streams[i], streams[i].header_size );
    }
}

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

#define SORTED_TABLES \
    1ull << TABLE_INTERFACEIMPL |\
    1ull << TABLE_CONSTANT |\
    1ull << TABLE_CUSTOMATTRIBUTE |\
    1ull << TABLE_FIELDMARSHAL |\
    1ull << TABLE_DECLSECURITY |\
    1ull << TABLE_CLASSLAYOUT |\
    1ull << TABLE_FIELDLAYOUT |\
    1ull << TABLE_EVENTMAP |\
    1ull << TABLE_PROPERTYMAP |\
    1ull << TABLE_METHODSEMANTICS |\
    1ull << TABLE_METHODIMPL |\
    1ull << TABLE_IMPLMAP |\
    1ull << TABLE_FIELDRVA |\
    1ull << TABLE_NESTEDCLASS |\
    1ull << TABLE_GENERICPARAM |\
    1ull << TABLE_GENERICPARAMCONSTRAINT

static struct
{
    UINT   reserved;
    BYTE   majorversion;
    BYTE   minor_version;
    BYTE   heap_sizes;
    BYTE   reserved2;
    UINT64 valid;
    UINT64 sorted;
}
tables_header = { 0, 2, 0, 0, 1, 0, SORTED_TABLES };

static struct buffer
{
    UINT  offset;        /* write position */
    UINT  allocated;     /* allocated size in bytes */
    UINT  count;         /* number of entries written */
    BYTE *ptr;
} strings, strings_idx, userstrings, userstrings_idx, blobs, blobs_idx, guids, tables[TABLE_MAX],
  tables_idx[TABLE_MAX], tables_disk;

static void *grow_buffer( struct buffer *buf, UINT size )
{
    UINT new_size;

    if (buf->allocated - buf->offset >= size) return buf->ptr;

    new_size = max( buf->offset + size, buf->allocated * 2 );
    buf->ptr = xrealloc( buf->ptr, new_size );
    buf->allocated = new_size;
    return buf->ptr;
}

static UINT encode_int( UINT value, BYTE *encoded )
{
    if (value < 0x80)
    {
        encoded[0] = value;
        return 1;
    }
    if (value < 0x4000)
    {
        encoded[0] = value >> 8 | 0x80;
        encoded[1] = value & 0xff;
        return 2;
    }
    if (value < 0x20000000)
    {
        encoded[0] = value >> 24 | 0xc0;
        encoded[1] = value >> 16 & 0xff;
        encoded[2] = value >> 8 & 0xff;
        encoded[3] = value & 0xff;
        return 4;
    }
    fprintf( stderr, "Value too large to encode.\n" );
    exit( 1 );
}

static UINT decode_int( const BYTE *encoded, UINT *len )
{
    if (!(encoded[0] & 0x80))
    {
        *len = 1;
        return encoded[0];
    }
    if (!(encoded[0] & 0x40))
    {
        *len = 2;
        return ((encoded[0] & ~0xc0) << 8) + encoded[1];
    }
    if (!(encoded[0] & 0x20))
    {
        *len = 4;
        return ((encoded[0] & ~0xe0) << 24) + (encoded[1] << 16) + (encoded[2] << 8) + encoded[3];
    }
    fprintf( stderr, "Invalid encoding.\n" );
    exit( 1 );
}

struct index
{
    UINT offset;    /* offset into corresponding data buffer */
    UINT size;      /* size of data entry */
};

static inline int cmp_data( const BYTE *data, UINT size, const BYTE *data2, UINT size2 )
{
    if (size < size2) return -1;
    else if (size > size2) return 1;
    return memcmp( data, data2, size );
}

/* return index struct if found, NULL and insert index if not found */
static const struct index *find_index( const struct buffer *buf_idx, const struct buffer *buf_data, const BYTE *data,
                                       UINT data_size, BOOL is_blob, UINT *insert_idx )
{
    int i, c, min = 0, max = buf_idx->count - 1;
    const struct index *idx, *base = (const struct index *)buf_idx->ptr;
    UINT size, len = 0;

    while (min <= max)
    {
        i = (min + max) / 2;
        idx = &base[i];

        if (is_blob) size = decode_int( buf_data->ptr + idx->offset, &len );
        else size = idx->size;

        c = cmp_data( data, data_size, buf_data->ptr + idx->offset + len, size );

        if (c < 0) max = i - 1;
        else if (c > 0) min = i + 1;
        else return idx;
    }

    if (insert_idx) *insert_idx = max + 1;
    return NULL;
}

static void insert_index( struct buffer *buf_idx, UINT idx, UINT offset, UINT size )
{
    struct index new = { offset, size }, *base = grow_buffer( buf_idx, sizeof(new) );

    memmove( &base[idx] + 1, &base[idx], (buf_idx->count - idx) * sizeof(new) );
    base[idx] = new;
    buf_idx->offset += sizeof(new);
    buf_idx->count++;
}

static UINT add_string( const char *str )
{
    UINT insert_idx, size, offset = strings.offset;
    const struct index *idx;

    if (!str) return 0;
    size = strlen( str ) + 1;
    if ((idx = find_index( &strings_idx, &strings, (const BYTE *)str, size, FALSE, &insert_idx )))
        return idx->offset;

    grow_buffer( &strings, size );
    memcpy( strings.ptr + offset, str, size );
    strings.offset += size;
    strings.count++;

    insert_index( &strings_idx, insert_idx, offset, size );
    return offset;
}

static inline int is_special_char( USHORT c )
{
    return (c >= 0x100 || (c >= 0x01 && c <= 0x08) || (c >= 0x0e && c <= 0x1f) || c == 0x27 || c == 0x2d || c == 0x7f);
}

static UINT add_userstring( const USHORT *str, UINT size )
{
    BYTE encoded[4], terminal = 0;
    UINT i, insert_idx, offset = userstrings.offset, len = encode_int( size + (str ? 1 : 0), encoded );
    const struct index *idx;

    if (!str && offset) return 0;

    if ((idx = find_index( &userstrings_idx, &userstrings, (const BYTE *)str, size, TRUE, &insert_idx )))
        return idx->offset;

    grow_buffer( &userstrings, len + size + 1 );
    memcpy( userstrings.ptr + userstrings.offset, encoded, len );
    userstrings.offset += len;
    if (str)
    {
        for (i = 0; i < size / sizeof(USHORT); i++)
        {
            *(USHORT *)(userstrings.ptr + userstrings.offset) = str[i];
            userstrings.offset += sizeof(USHORT);
            if (is_special_char( str[i] )) terminal = 1;
        }
        userstrings.ptr[userstrings.offset++] = terminal;
    }
    userstrings.count++;

    insert_index( &userstrings_idx, insert_idx, offset, size );
    return offset;
}

static UINT add_blob( const BYTE *blob, UINT size )
{
    BYTE encoded[4];
    UINT insert_idx, offset = blobs.offset, len = encode_int( size, encoded );
    const struct index *idx;

    if (!blob && offset) return 0;
    if ((idx = find_index( &blobs_idx, &blobs, blob, size, TRUE, &insert_idx ))) return idx->offset;

    grow_buffer( &blobs, len + size );
    memcpy( blobs.ptr + blobs.offset, encoded, len );
    blobs.offset += len;
    if (blob)
    {
        memcpy( blobs.ptr + blobs.offset, blob, size );
        blobs.offset += size;
    }
    blobs.count++;

    insert_index( &blobs_idx, insert_idx, offset, size );
    return offset;
}

static UINT add_guid( const GUID *guid )
{
    grow_buffer( &guids, sizeof(*guid) );
    memcpy( guids.ptr + guids.offset, guid, sizeof(*guid) );
    guids.offset += sizeof(*guid);
    return ++guids.count;
}

/* returns row number */
static UINT add_row( enum table table, const BYTE *row, UINT row_size )
{
    const struct index *idx;
    UINT insert_idx, offset = tables[table].offset;
    BOOL sort = (table != TABLE_PARAM && table != TABLE_FIELD && table != TABLE_PROPERTY && table != TABLE_EVENT);

    if (sort && (idx = find_index( &tables_idx[table], &tables[table], row, row_size, FALSE, &insert_idx )))
        return idx->offset / row_size + 1;

    grow_buffer( &tables[table], row_size );
    memcpy( tables[table].ptr + offset, row, row_size );
    tables[table].offset += row_size;
    tables[table].count++;

    if (sort) insert_index( &tables_idx[table], insert_idx, offset, row_size );
    return tables[table].count;
}

static void add_bytes( struct buffer *buf, const BYTE *data, UINT size )
{
    grow_buffer( buf, size );
    memcpy( buf->ptr + buf->offset, data, size );
    buf->offset += size;
}

static void serialize_byte( UINT value )
{
    assert( !(value >> 24) );
    add_bytes( &tables_disk, (const BYTE *)&value, sizeof(BYTE) );
}

static void serialize_ushort( UINT value )
{
    assert( !(value >> 16) );
    add_bytes( &tables_disk, (const BYTE *)&value, sizeof(USHORT) );
}

static void serialize_uint( UINT value )
{
    add_bytes( &tables_disk, (const BYTE *)&value, sizeof(value) );
}

static void serialize_string_idx( UINT idx )
{
    UINT size = strings.offset >> 16 ? sizeof(UINT) : sizeof(USHORT);
    add_bytes( &tables_disk, (const BYTE *)&idx, size );
}

static void serialize_guid_idx( UINT idx )
{
    UINT size = guids.offset >> 16 ? sizeof(UINT) : sizeof(USHORT);
    add_bytes( &tables_disk, (const BYTE *)&idx, size );
}

static void serialize_blob_idx( UINT idx )
{
    UINT size = blobs.offset >> 16 ? sizeof(UINT) : sizeof(USHORT);
    add_bytes( &tables_disk, (const BYTE *)&idx, size );
}

static void serialize_table_idx( UINT idx, enum table target )
{
    UINT size = tables[target].count >> 16 ? sizeof(UINT) : sizeof(USHORT);
    add_bytes( &tables_disk, (const BYTE *)&idx, size );
}

static enum table resolution_scope_to_table( UINT token )
{
    switch (token & 0x3)
    {
    case 0: return TABLE_MODULE;
    case 1: return TABLE_MODULEREF;
    case 2: return TABLE_ASSEMBLYREF;
    case 3: return TABLE_TYPEREF;
    default: assert( 0 );
    }
}

static enum table typedef_or_ref_to_table( UINT token )
{
    switch (token & 0x3)
    {
    case 0: return TABLE_TYPEDEF;
    case 1: return TABLE_TYPEREF;
    case 2: return TABLE_TYPESPEC;
    default: assert( 0 );
    }
}

static enum table methoddef_or_ref_to_table( UINT token )
{
    switch (token & 0x1)
    {
    case 0: return TABLE_METHODDEF;
    case 1: return TABLE_MEMBERREF;
    default: assert( 0 );
    }
}

static enum table memberref_parent_to_table( UINT token )
{
    switch (token & 0x7)
    {
    case 0: return TABLE_TYPEDEF;
    case 1: return TABLE_TYPEREF;
    case 2: return TABLE_MODULEREF;
    case 3: return TABLE_METHODDEF;
    case 4: return TABLE_TYPESPEC;
    default: assert( 0 );
    }
}

static enum table has_constant_to_table( UINT token )
{
    switch (token & 0x3)
    {
    case 0: return TABLE_FIELD;
    case 1: return TABLE_PARAM;
    case 2: return TABLE_PROPERTY;
    default: assert( 0 );
    }
}

static enum table has_customattribute_to_table( UINT token )
{
    switch (token & 0x1f)
    {
    case 0: return TABLE_METHODDEF;
    case 1: return TABLE_FIELD;
    case 2: return TABLE_TYPEREF;
    case 3: return TABLE_TYPEDEF;
    case 4: return TABLE_PARAM;
    case 5: return TABLE_INTERFACEIMPL;
    case 6: return TABLE_MEMBERREF;
    case 7: return TABLE_MODULE;
    case 9: return TABLE_PROPERTY;
    case 10: return TABLE_EVENT;
    case 11: return TABLE_STANDALONESIG;
    case 12: return TABLE_MODULEREF;
    case 13: return TABLE_TYPESPEC;
    case 14: return TABLE_ASSEMBLY;
    case 15: return TABLE_ASSEMBLYREF;
    case 16: return TABLE_FILE;
    case 17: return TABLE_EXPORTEDTYPE;
    case 18: return TABLE_MANIFESTRESOURCE;
    default: assert( 0 );
    }
}

static enum table customattribute_type_to_table( UINT token )
{
    switch (token & 0x7)
    {
    case 2: return TABLE_METHODDEF;
    case 3: return TABLE_MEMBERREF;
    default: assert( 0 );
    }
}

static enum table has_semantics_to_table( UINT token )
{
    switch (token & 0x1)
    {
    case 0: return TABLE_EVENT;
    case 1: return TABLE_PROPERTY;
    default: assert( 0 );
    }
}

struct module_row
{
    UINT generation;
    UINT name;
    UINT mvid;
    UINT encid;
    UINT encbaseid;
};

static UINT add_module_row( UINT name, UINT mvid )
{
    struct module_row row = { 0, name, mvid, 0, 0 };
    return add_row( TABLE_MODULE, (const BYTE *)&row, sizeof(row) );
}

static void serialize_module_table( void )
{
    const struct module_row *row = (const struct module_row *)tables[TABLE_MODULE].ptr;

    serialize_ushort( row->generation );
    serialize_string_idx( row->name );
    serialize_guid_idx( row->mvid );
    serialize_guid_idx( row->encid );
    serialize_guid_idx( row->encbaseid );
}

struct typeref_row
{
    UINT scope;
    UINT name;
    UINT namespace;
};

static UINT add_typeref_row( UINT scope, UINT name, UINT namespace )
{
    struct typeref_row row = { scope, name, namespace };
    return add_row( TABLE_TYPEREF, (const BYTE *)&row, sizeof(row) );
}

static void serialize_typeref_table( void )
{
    const struct typeref_row *row = (const struct typeref_row *)tables[TABLE_TYPEREF].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_TYPEREF].count; i++)
    {
        serialize_table_idx( row->scope, resolution_scope_to_table(row->scope) );
        serialize_string_idx( row->name );
        serialize_string_idx( row->namespace );
        row++;
    }
}

struct typedef_row
{
    UINT flags;
    UINT name;
    UINT namespace;
    UINT extends;
    UINT fieldlist;
    UINT methodlist;
};

static UINT add_typedef_row( UINT flags, UINT name, UINT namespace, UINT extends, UINT fieldlist, UINT methodlist )
{
    struct typedef_row row = { flags, name, namespace, extends, fieldlist, methodlist };

    if (!row.fieldlist) row.fieldlist = tables[TABLE_FIELD].count + 1;
    if (!row.methodlist) row.methodlist = tables[TABLE_METHODDEF].count + 1;
    return add_row( TABLE_TYPEDEF, (const BYTE *)&row, sizeof(row) );
}

/* FIXME: enclosing classes should come before enclosed classes */
static void serialize_typedef_table( void )
{
    const struct typedef_row *row = (const struct typedef_row *)tables[TABLE_TYPEDEF].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_TYPEDEF].count; i++)
    {
        serialize_uint( row->flags );
        serialize_string_idx( row->name );
        serialize_string_idx( row->namespace );
        serialize_table_idx( row->extends, typedef_or_ref_to_table(row->extends) );
        serialize_table_idx( row->fieldlist, TABLE_FIELD );
        serialize_table_idx( row->methodlist, TABLE_METHODDEF );
        row++;
    }
}

struct field_row
{
    UINT flags;
    UINT name;
    UINT signature;
};

static UINT add_field_row( UINT flags, UINT name, UINT signature )
{
    struct field_row row = { flags, name, signature };
    return add_row( TABLE_FIELD, (const BYTE *)&row, sizeof(row) );
}

static void serialize_field_table( void )
{
    const struct field_row *row = (const struct field_row *)tables[TABLE_FIELD].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_FIELD].count; i++)
    {
        serialize_ushort( row->flags );
        serialize_string_idx( row->name );
        serialize_blob_idx( row->signature );
        row++;
    }
}

struct methoddef_row
{
    UINT rva;
    UINT implflags;
    UINT flags;
    UINT name;
    UINT signature;
    UINT paramlist;
};

static UINT add_methoddef_row( UINT implflags, UINT flags, UINT name, UINT signature, UINT paramlist )
{
    struct methoddef_row row = { 0, implflags, flags, name, signature, paramlist };

    if (!row.paramlist) row.paramlist = tables[TABLE_PARAM].count + 1;
    return add_row( TABLE_METHODDEF, (const BYTE *)&row, sizeof(row) );
}

static void serialize_methoddef_table( void )
{
    const struct methoddef_row *row = (const struct methoddef_row *)tables[TABLE_METHODDEF].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_METHODDEF].count; i++)
    {
        serialize_uint( row->rva );
        serialize_ushort( row->implflags );
        serialize_ushort( row->flags );
        serialize_string_idx( row->name );
        serialize_blob_idx( row->signature );
        serialize_table_idx( row->paramlist, TABLE_PARAM );
        row++;
    }
}

struct param_row
{
    UINT flags;
    UINT sequence;
    UINT name;
};

static UINT add_param_row( USHORT flags, USHORT sequence, UINT name )
{
    struct param_row row = { flags, sequence, name };
    return add_row( TABLE_PARAM, (const BYTE *)&row, sizeof(row) );
}

static void serialize_param_table( void )
{
    const struct param_row *row = (const struct param_row *)tables[TABLE_PARAM].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_PARAM].count; i++)
    {
        serialize_ushort( row->flags );
        serialize_ushort( row->sequence );
        serialize_string_idx( row->name );
        row++;
    }
}

struct interfaceimpl_row
{
    UINT class;
    UINT interface;
};

static UINT add_interfaceimpl_row( UINT class, UINT interface )
{
    struct interfaceimpl_row row = { class, interface };
    return add_row( TABLE_INTERFACEIMPL, (const BYTE *)&row, sizeof(row) );
}

static int cmp_interfaceimpl_row( const void *a, const void *b )
{
    const struct interfaceimpl_row *row = a, *row2 = b;
    if (row->class > row2->class) return 1;
    if (row->class < row2->class) return -1;
    if (row->interface > row2->interface) return 1;
    if (row->interface < row2->interface) return -1;
    return 0;
}

/* sorted by class, interface */
static void serialize_interfaceimpl_table( void )
{
    const struct interfaceimpl_row *row = (const struct interfaceimpl_row *)tables[TABLE_INTERFACEIMPL].ptr;
    UINT i;

    qsort( tables[TABLE_INTERFACEIMPL].ptr, tables[TABLE_INTERFACEIMPL].count, sizeof(*row),
           cmp_interfaceimpl_row );

    for (i = 0; i < tables_idx[TABLE_INTERFACEIMPL].count; i++)
    {
        serialize_table_idx( row->class, TABLE_TYPEDEF );
        serialize_table_idx( row->interface, typedef_or_ref_to_table(row->interface) );
        row++;
    }
}

struct memberref_row
{
    UINT class;
    UINT name;
    UINT signature;
};

static UINT add_memberref_row( UINT class, UINT name, UINT signature )
{
    struct memberref_row row = { class, name, signature };
    return add_row( TABLE_MEMBERREF, (const BYTE *)&row, sizeof(row) );
}

static void serialize_memberref_table( void )
{
    const struct memberref_row *row = (const struct memberref_row *)tables[TABLE_MEMBERREF].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_MEMBERREF].count; i++)
    {
        serialize_table_idx( row->class, memberref_parent_to_table(row->class) );
        serialize_string_idx( row->name );
        serialize_blob_idx( row->signature );
        row++;
    }
}

struct constant_row
{
    UINT type;
    UINT padding;
    UINT parent;
    UINT value;
};

static UINT add_constant_row( BYTE type, UINT parent, UINT value )
{
    struct constant_row row = { type, 0, parent, value };
    return add_row( TABLE_CONSTANT, (const BYTE *)&row, sizeof(row) );
}

static int cmp_constant_row( const void *a, const void *b )
{
    const struct constant_row *row = a, *row2 = b;
    if (row->parent > row2->parent) return 1;
    if (row->parent < row2->parent) return -1;
    return 0;
}

/* sorted by parent */
static void serialize_constant_table( void )
{
    const struct constant_row *row = (const struct constant_row *)tables[TABLE_CONSTANT].ptr;
    UINT i;

    qsort( tables[TABLE_CONSTANT].ptr, tables[TABLE_CONSTANT].count, sizeof(*row), cmp_constant_row );

    for (i = 0; i < tables_idx[TABLE_CONSTANT].count; i++)
    {
        serialize_byte( row->type );
        serialize_byte( row->padding );
        serialize_table_idx( row->parent, has_constant_to_table(row->parent) );
        serialize_blob_idx( row->value );
        row++;
    }
}

struct customattribute_row
{
    UINT parent;
    UINT type;
    UINT value;
};

static UINT add_customattribute_row( UINT parent, UINT type, UINT value )
{
    struct customattribute_row row = { parent, type, value };
    return add_row( TABLE_CUSTOMATTRIBUTE, (const BYTE *)&row, sizeof(row) );
}

static int cmp_customattribute_row( const void *a, const void *b )
{
    const struct customattribute_row *row = a, *row2 = b;
    if (row->parent > row2->parent) return 1;
    if (row->parent < row2->parent) return -1;
    return 0;
}

/* sorted by parent */
static void serialize_customattribute_table( void )
{
    const struct customattribute_row *row = (const struct customattribute_row *)tables[TABLE_CUSTOMATTRIBUTE].ptr;
    UINT i;

    qsort( tables[TABLE_CUSTOMATTRIBUTE].ptr, tables[TABLE_CUSTOMATTRIBUTE].count, sizeof(*row),
           cmp_customattribute_row );

    for (i = 0; i < tables_idx[TABLE_CUSTOMATTRIBUTE].count; i++)
    {
        serialize_table_idx( row->parent, has_customattribute_to_table(row->parent) );
        serialize_table_idx( row->type, customattribute_type_to_table(row->type) );
        serialize_blob_idx( row->value );
        row++;
    }
}

struct assembly_row
{
    UINT hashalgid;
    UINT majorversion;
    UINT minorversion;
    UINT buildnumber;
    UINT revisionnumber;
    UINT flags;
    UINT publickey;
    UINT name;
    UINT culture;
};

static UINT add_assembly_row( UINT name )
{
    struct assembly_row row = { CALG_SHA, 255, 255, 255, 255, 0x200, 0, name, 0 };
    return add_row( TABLE_ASSEMBLY, (const BYTE *)&row, sizeof(row) );
}

static void serialize_assembly_table( void )
{
    const struct assembly_row *row = (const struct assembly_row *)tables[TABLE_ASSEMBLY].ptr;

    serialize_uint( row->hashalgid );
    serialize_ushort( row->majorversion );
    serialize_ushort( row->minorversion );
    serialize_ushort( row->buildnumber );
    serialize_ushort( row->revisionnumber );
    serialize_uint( row->flags );
    serialize_blob_idx( row->publickey );
    serialize_string_idx( row->name );
    serialize_string_idx( row->culture );
}

struct assemblyref_row
{
    UINT majorversion;
    UINT minorversion;
    UINT buildnumber;
    UINT revisionnumber;
    UINT flags;
    UINT publickey;
    UINT name;
    UINT culture;
    UINT hashvalue;
};

static UINT add_assemblyref_row( UINT flags, UINT publickey, UINT name )
{
    struct assemblyref_row row = { 255, 255, 255, 255, flags, publickey, name, 0, 0 };
    return add_row( TABLE_ASSEMBLYREF, (const BYTE *)&row, sizeof(row) );
}

static void serialize_assemblyref_table( void )
{
    const struct assemblyref_row *row = (const struct assemblyref_row *)tables[TABLE_ASSEMBLYREF].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_ASSEMBLYREF].count; i++)
    {
        serialize_ushort( row->majorversion );
        serialize_ushort( row->minorversion );
        serialize_ushort( row->buildnumber );
        serialize_ushort( row->revisionnumber );
        serialize_uint( row->flags );
        serialize_blob_idx( row->publickey );
        serialize_string_idx( row->name );
        serialize_string_idx( row->culture );
        serialize_blob_idx( row->hashvalue );
        row++;
    }
}

struct propertymap_row
{
    UINT parent;
    UINT proplist;
};

static UINT add_propertymap_row( UINT parent, UINT proplist )
{
    struct propertymap_row row = { parent, proplist };
    return add_row( TABLE_PROPERTYMAP, (const BYTE *)&row, sizeof(row) );
}

static void serialize_propertymap_table( void )
{
    const struct propertymap_row *row = (const struct propertymap_row *)tables[TABLE_PROPERTYMAP].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_PROPERTYMAP].count; i++)
    {
        serialize_table_idx( row->parent, TABLE_TYPEDEF );
        serialize_table_idx( row->proplist, TABLE_PROPERTY );
        row++;
    }
}

struct property_row
{
    UINT flags;
    UINT name;
    UINT type;
};

static UINT add_property_row( USHORT flags, UINT name, UINT type )
{
    struct property_row row = { flags, name, type };
    return add_row( TABLE_PROPERTY, (const BYTE *)&row, sizeof(row) );
}

static void serialize_property_table( void )
{
    const struct property_row *row = (const struct property_row *)tables[TABLE_PROPERTY].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_PROPERTY].count; i++)
    {
        serialize_ushort( row->flags );
        serialize_string_idx( row->name );
        serialize_blob_idx( row->type );
        row++;
    }
}

struct eventmap_row
{
    UINT parent;
    UINT eventlist;
};

static UINT add_eventmap_row( UINT parent, UINT eventlist )
{
    struct eventmap_row row = { parent, eventlist };
    return add_row( TABLE_EVENTMAP, (const BYTE *)&row, sizeof(row) );
}

static void serialize_eventmap_table( void )
{
    const struct eventmap_row *row = (const struct eventmap_row *)tables[TABLE_EVENTMAP].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_EVENTMAP].count; i++)
    {
        serialize_table_idx( row->parent, TABLE_TYPEDEF );
        serialize_table_idx( row->eventlist, TABLE_EVENT );
        row++;
    }
}

struct event_row
{
    UINT flags;
    UINT name;
    UINT type;
};

static UINT add_event_row( USHORT flags, UINT name, UINT type )
{
    struct event_row row = { flags, name, type };
    return add_row( TABLE_EVENT, (const BYTE *)&row, sizeof(row) );
}

static void serialize_event_table( void )
{
    const struct event_row *row = (const struct event_row *)tables[TABLE_EVENT].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_EVENT].count; i++)
    {
        serialize_ushort( row->flags );
        serialize_string_idx( row->name );
        serialize_table_idx( row->type, typedef_or_ref_to_table(row->type) );
        row++;
    }
}

struct methodsemantics_row
{
    UINT semantics;
    UINT method;
    UINT association;
};

static UINT add_methodsemantics_row( USHORT flags, UINT name, UINT type )
{
    struct methodsemantics_row row = { flags, name, type };
    return add_row( TABLE_METHODSEMANTICS, (const BYTE *)&row, sizeof(row) );
}

static int cmp_methodsemantics_row( const void *a, const void *b )
{
    const struct methodsemantics_row *row = a, *row2 = b;
    if (row->association > row2->association) return 1;
    if (row->association < row2->association) return -1;
    return 0;
}

/* sorted by association */
static void serialize_methodsemantics_table( void )
{
    const struct methodsemantics_row *row = (const struct methodsemantics_row *)tables[TABLE_METHODSEMANTICS].ptr;
    UINT i;

    qsort( tables[TABLE_METHODSEMANTICS].ptr, tables[TABLE_METHODSEMANTICS].count, sizeof(*row),
           cmp_methodsemantics_row );

    for (i = 0; i < tables[TABLE_METHODSEMANTICS].count; i++)
    {
        serialize_ushort( row->semantics );
        serialize_table_idx( row->method, TABLE_METHODDEF );
        serialize_table_idx( row->association, has_semantics_to_table(row->association) );
        row++;
    }
}

struct methodimpl_row
{
    UINT class;
    UINT body;
    UINT declaration;
};

static UINT add_methodimpl_row( UINT class, UINT body, UINT declaration )
{
    struct methodimpl_row row = { class, body, declaration };
    return add_row( TABLE_METHODIMPL, (const BYTE *)&row, sizeof(row) );
}

static void serialize_methodimpl_table( void )
{
    const struct methodimpl_row *row = (const struct methodimpl_row *)tables[TABLE_METHODIMPL].ptr;
    UINT i;

    for (i = 0; i < tables[TABLE_METHODIMPL].count; i++)
    {
        serialize_table_idx( row->class, row->class );
        serialize_table_idx( row->body, methoddef_or_ref_to_table(row->body) );
        serialize_table_idx( row->declaration, methoddef_or_ref_to_table(row->declaration) );
        row++;
    }
}

static UINT typedef_or_ref( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_TYPEDEF: return row << 2;
    case TABLE_TYPEREF: return row << 2 | 1;
    case TABLE_TYPESPEC: return row << 2 | 2;
    default: assert( 0 );
    }
}

static UINT methoddef_or_ref( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_METHODDEF: return row << 1;
    case TABLE_MEMBERREF: return row << 1 | 1;
    default: assert( 0 );
    }
}

static UINT resolution_scope( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_MODULE: return row << 2;
    case TABLE_MODULEREF: return row << 2 | 1;
    case TABLE_ASSEMBLYREF: return row << 2 | 2;
    case TABLE_TYPEREF: return row << 2 | 3;
    default: assert( 0 );
    }
}

static UINT has_constant( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_FIELD: return row << 2;
    case TABLE_PARAM: return row << 2 | 1;
    case TABLE_PROPERTY: return row << 2 | 2;
    default: assert( 0 );
    }
}

static UINT memberref_parent( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_TYPEDEF: return row << 3;
    case TABLE_TYPEREF: return row << 3 | 1;
    case TABLE_MODULEREF: return row << 3 | 2;
    case TABLE_METHODDEF: return row << 3 | 3;
    case TABLE_TYPESPEC: return row << 3 | 4;
    default: assert( 0 );
    }
}

static UINT has_customattribute( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_METHODDEF: return row << 5;
    case TABLE_FIELD: return row << 5 | 1;
    case TABLE_TYPEREF: return row << 5 | 2;
    case TABLE_TYPEDEF: return row << 5 | 3;
    case TABLE_PARAM: return row << 5 | 4;
    case TABLE_INTERFACEIMPL: return row << 5 | 5;
    case TABLE_MEMBERREF: return row << 5 | 6;
    case TABLE_MODULE: return row << 5 | 7;
    case TABLE_PROPERTY: return row << 5 | 9;
    case TABLE_EVENT: return row << 5 | 10;
    case TABLE_STANDALONESIG: return row << 5 | 11;
    case TABLE_MODULEREF: return row << 5 | 12;
    case TABLE_TYPESPEC: return row << 5 | 13;
    case TABLE_ASSEMBLY: return row << 5 | 14;
    case TABLE_ASSEMBLYREF: return row << 5 | 15;
    case TABLE_FILE: return row << 5 | 16;
    case TABLE_EXPORTEDTYPE: return row << 5 | 17;
    case TABLE_MANIFESTRESOURCE: return row << 5 | 18;
    default: assert( 0 );
    }
}

static UINT customattribute_type( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_METHODDEF: return row << 3 | 2;
    case TABLE_MEMBERREF: return row << 3 | 3;
    default: assert( 0 );
    }
}

static UINT has_semantics( enum table table, UINT row )
{
    switch (table)
    {
    case TABLE_EVENT: return row << 1;
    case TABLE_PROPERTY: return row << 1 | 1;
    default: assert( 0 );
    }
}

enum element_type
{
    ELEMENT_TYPE_END            = 0x00,
    ELEMENT_TYPE_VOID           = 0x01,
    ELEMENT_TYPE_BOOLEAN        = 0x02,
    ELEMENT_TYPE_CHAR           = 0x03,
    ELEMENT_TYPE_I1             = 0x04,
    ELEMENT_TYPE_U1             = 0x05,
    ELEMENT_TYPE_I2             = 0x06,
    ELEMENT_TYPE_U2             = 0x07,
    ELEMENT_TYPE_I4             = 0x08,
    ELEMENT_TYPE_U4             = 0x09,
    ELEMENT_TYPE_I8             = 0x0a,
    ELEMENT_TYPE_U8             = 0x0b,
    ELEMENT_TYPE_R4             = 0x0c,
    ELEMENT_TYPE_R8             = 0x0d,
    ELEMENT_TYPE_STRING         = 0x0e,
    ELEMENT_TYPE_PTR            = 0x0f,
    ELEMENT_TYPE_BYREF          = 0x10,
    ELEMENT_TYPE_VALUETYPE      = 0x11,
    ELEMENT_TYPE_CLASS          = 0x12,
    ELEMENT_TYPE_VAR            = 0x13,
    ELEMENT_TYPE_ARRAY          = 0x14,
    ELEMENT_TYPE_GENERICINST    = 0x15,
    ELEMENT_TYPE_TYPEDBYREF     = 0x16,
    ELEMENT_TYPE_I              = 0x18,
    ELEMENT_TYPE_U              = 0x19,
    ELEMENT_TYPE_FNPTR          = 0x1b,
    ELEMENT_TYPE_OBJECT         = 0x1c,
    ELEMENT_TYPE_SZARRAY        = 0x1d,
    ELEMENT_TYPE_MVAR           = 0x1e,
    ELEMENT_TYPE_CMOD_REQD      = 0x1f,
    ELEMENT_TYPE_CMOD_OPT       = 0x20,
    ELEMENT_TYPE_INTERNAL       = 0x21,
    ELEMENT_TYPE_MODIFIER       = 0x40,
    ELEMENT_TYPE_SENTINEL       = 0x41,
    ELEMENT_TYPE_PINNED         = 0x45
};

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

enum
{
    FIELD_ATTR_PRIVATE          = 0x0001,
    FIELD_ATTR_FAMANDASSEM      = 0x0002,
    FIELD_ATTR_ASSEMBLY         = 0x0003,
    FIELD_ATTR_FAMILY           = 0x0004,
    FIELD_ATTR_FAMORASSEM       = 0x0005,
    FIELD_ATTR_PUBLIC           = 0x0006,
    FIELD_ATTR_STATIC           = 0x0010,
    FIELD_ATTR_INITONLY         = 0x0020,
    FIELD_ATTR_LITERAL          = 0x0040,
    FIELD_ATTR_NOTSERIALIZED    = 0x0080,
    FIELD_ATTR_HASFIELDRVA      = 0x0100,
    FIELD_ATTR_SPECIALNAME      = 0x0200,
    FIELD_ATTR_RTSPECIALNAME    = 0x0400,
    FIELD_ATTR_HASFIELDMARSHAL  = 0x1000,
    FIELD_ATTR_PINVOKEIMPL      = 0x2000,
    FIELD_ATTR_HASDEFAULT       = 0x8000
};

enum
{
    METHOD_ATTR_COMPILERCONTROLLED = 0x0000,
    METHOD_ATTR_PRIVATE            = 0x0001,
    METHOD_ATTR_FAMANDASSEM        = 0x0002,
    METHOD_ATTR_ASSEM              = 0x0003,
    METHOD_ATTR_FAMILY             = 0x0004,
    METHOD_ATTR_FAMORASSEM         = 0x0005,
    METHOD_ATTR_PUBLIC             = 0x0006,
    METHOD_ATTR_STATIC             = 0x0010,
    METHOD_ATTR_FINAL              = 0x0020,
    METHOD_ATTR_VIRTUAL            = 0x0040,
    METHOD_ATTR_HIDEBYSIG          = 0x0080,
    METHOD_ATTR_NEWSLOT            = 0x0100,
    METHOD_ATTR_STRICT             = 0x0200,
    METHOD_ATTR_ABSTRACT           = 0x0400,
    METHOD_ATTR_SPECIALNAME        = 0x0800,
    METHOD_ATTR_RTSPECIALNAME      = 0x1000,
    METHOD_ATTR_PINVOKEIMPL        = 0x2000
};

enum
{
    METHOD_IMPL_IL        = 0x0000,
    METHOD_IMPL_NATIVE    = 0x0001,
    METHOD_IMPL_OPTIL     = 0x0002,
    METHOD_IMPL_RUNTIME   = 0x0003,
    METHOD_IMPL_UNMANAGED = 0x0004
};

enum
{
    METHOD_SEM_SETTER   = 0x0001,
    METHOD_SEM_GETTER   = 0x0002,
    METHOD_SEM_OTHER    = 0x0004,
    METHOD_SEM_ADDON    = 0x0008,
    METHOD_SEM_REMOVEON = 0x0010
};

enum
{
    PARAM_ATTR_IN       = 0x0001,
    PARAM_ATTR_OUT      = 0x0002,
    PARAM_ATTR_OPTIONAL = 0x0010
};

enum
{
    SIG_TYPE_DEFAULT      = 0x00,
    SIG_TYPE_C            = 0x01,
    SIG_TYPE_STDCALL      = 0x02,
    SIG_TYPE_THISCALL     = 0x03,
    SIG_TYPE_FASTCALL     = 0x04,
    SIG_TYPE_VARARG       = 0x05,
    SIG_TYPE_FIELD        = 0x06,
    SIG_TYPE_LOCALSIG     = 0x07,
    SIG_TYPE_PROPERTY     = 0x08,
    SIG_TYPE_GENERIC      = 0x10,
    SIG_TYPE_HASTHIS      = 0x20,
    SIG_TYPE_EXPLICITTHIS = 0x40
};

static char *assembly_name;     /* current module */
static char **assembly_imports;
static UINT num_assembly_imports;

static void append_assembly_import( const char *import )
{
    char *ptr, *name = xstrdup( import );

    if ((ptr = strrchr( name, '.' ))) *ptr = 0;
    assembly_imports = xrealloc( assembly_imports, (num_assembly_imports + 1) * sizeof(*assembly_imports) );
    assembly_imports[num_assembly_imports++] = name;
}

static const char *get_assembly_import( const char *name )
{
    UINT i;
    for (i = 0; i < num_assembly_imports; i++)
    {
        if (!strcasecmp( name, assembly_imports[i] )) return assembly_imports[i];
    }
    return NULL;
}

#define MODULE_ROW      1
#define MSCORLIB_ROW    1

#define MAX_NAME        256

/* create a type reference if needed and store it in the base type */
static void create_typeref( type_t *type )
{
    UINT namespace, assemblyref, scope;
    char *namespace_str;
    const char *import_name;
    type_t *base_type;

    while (type_get_type( type ) == TYPE_POINTER) type = type_pointer_get_ref_type( type );
    base_type = type_get_real_type( type );

    if (base_type->md.ref) return;

    /* basic types don't get a reference */
    if (!base_type->name) return;

    /* HSTRING is treated as a fundamental type */
    if (type->name && !strcmp( type->name, "HSTRING__" )) return;

    /* GUID is imported from mscorlib */
    if (type->name && !strcmp( type->name, "GUID" ))
    {
        scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
        base_type->md.ref = add_typeref_row( scope, add_string("Guid"), add_string("System") );
        return;
    }

    base_type->md.name = add_string( base_type->name );
    namespace_str = format_namespace( base_type->namespace, "", ".", NULL, NULL );
    base_type->md.namespace = add_string( namespace_str );

    if (base_type->md.namespace && (import_name = get_assembly_import( namespace_str )))
    {
        assemblyref = add_assemblyref_row( 0x200, 0, add_string(import_name) );
        scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
        base_type->md.ref = add_typeref_row( scope, base_type->md.name, base_type->md.namespace );
    }
    else if (!base_type->md.namespace) /* types without namespace are imported from Windows.Foundation */
    {
        namespace = add_string( "Windows.Foundation" );
        assemblyref = add_assemblyref_row( 0x200, 0, namespace );
        scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
        base_type->md.ref = add_typeref_row( scope, base_type->md.name, namespace );
    }
    else
    {
        scope = resolution_scope( TABLE_MODULE, MODULE_ROW );
        base_type->md.ref = add_typeref_row( scope, base_type->md.name, base_type->md.namespace );
    }

    free( namespace_str );
}

static UINT make_field_value_sig( UINT token, BYTE *buf )
{
    UINT len = 2;

    buf[0] = SIG_TYPE_FIELD;
    buf[1] = ELEMENT_TYPE_VALUETYPE;
    len += encode_int( token, buf + 2 );
    return len;
}

enum
{
    LARGE_STRING_HEAP = 0x01,
    LARGE_GUID_HEAP   = 0x02,
    LARGE_BLOB_HEAP   = 0x04
};

static enum element_type map_basic_type( enum type_basic_type type, int sign )
{
    enum element_type elem_type;

    switch (type)
    {
    case TYPE_BASIC_CHAR:   elem_type = ELEMENT_TYPE_BOOLEAN; break;
    case TYPE_BASIC_INT16:  elem_type = (sign > 0) ? ELEMENT_TYPE_U2 : ELEMENT_TYPE_I2; break;
    case TYPE_BASIC_INT:
    case TYPE_BASIC_INT32:
    case TYPE_BASIC_LONG:   elem_type = (sign > 0) ? ELEMENT_TYPE_U4 : ELEMENT_TYPE_I4; break;
    case TYPE_BASIC_INT64:  elem_type = (sign > 0) ? ELEMENT_TYPE_U8 : ELEMENT_TYPE_I8; break;
    case TYPE_BASIC_BYTE:   elem_type = ELEMENT_TYPE_U1; break;
    case TYPE_BASIC_WCHAR:  elem_type = ELEMENT_TYPE_CHAR; break;
    case TYPE_BASIC_FLOAT:  elem_type = ELEMENT_TYPE_R4; break;
    case TYPE_BASIC_DOUBLE: elem_type = ELEMENT_TYPE_R8; break;
    default:
        fprintf( stderr, "Unhandled basic type %u.\n", type );
        exit( 1 );
    }
    return elem_type;
}

static UINT make_struct_field_sig( const var_t *var, BYTE *buf )
{
    const type_t *type = var->declspec.type;

    if (type->name && !strcmp( type->name, "HSTRING" ))
    {
        buf[0] = SIG_TYPE_FIELD;
        buf[1] = ELEMENT_TYPE_STRING;
        return 2;
    }

    if (type->name && !strcmp( type->name, "GUID" ))
    {
        UINT token = typedef_or_ref( TABLE_TYPEREF, type_get_real_type(type)->md.ref );
        return make_field_value_sig( token, buf );
    }

    type = type_get_real_type( type );

    if (type_get_type( type ) == TYPE_ENUM || type_get_type( type ) == TYPE_STRUCT)
    {
        UINT token = typedef_or_ref( TABLE_TYPEREF, type->md.ref );
        return make_field_value_sig( token, buf );
    }

    if (type_get_type( type ) != TYPE_BASIC)
    {
        fprintf( stderr, "Unhandled struct field type %u.\n", type_get_type( type ) );
        exit( 1 );
    }

    buf[0] = SIG_TYPE_FIELD;
    buf[1] = map_basic_type( type_basic_get_type(type), type_basic_get_sign(type) );
    return 2;
}

static UINT make_member_sig( UINT token, BYTE *buf )
{
    UINT len = 4;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 2;
    buf[2] = ELEMENT_TYPE_VOID;
    buf[3] = ELEMENT_TYPE_CLASS;
    len += encode_int( token, buf + 4 );
    buf[len++] = ELEMENT_TYPE_U4;
    return len;
}

static UINT make_member_sig2( UINT type, UINT token, BYTE *buf )
{
    UINT len = 4;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 1;
    buf[2] = ELEMENT_TYPE_VOID;
    buf[3] = type;
    len += encode_int( token, buf + 4 );
    return len;
}

static UINT make_member_sig3( UINT token, BYTE *buf )
{
    UINT len = 4;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 3;
    buf[2] = ELEMENT_TYPE_VOID;
    buf[3] = ELEMENT_TYPE_CLASS;
    len += encode_int( token, buf + 4 );
    buf[len++] = ELEMENT_TYPE_U4;
    buf[len++] = ELEMENT_TYPE_STRING;
    return len;
}

static UINT make_member_sig4( UINT token, UINT token2, BYTE *buf )
{
    UINT len = 4;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 4;
    buf[2] = ELEMENT_TYPE_VOID;
    buf[3] = ELEMENT_TYPE_CLASS;
    len += encode_int( token, buf + 4 );
    buf[len++] = ELEMENT_TYPE_VALUETYPE;
    len += encode_int( token2, buf + len );
    buf[len++] = ELEMENT_TYPE_U4;
    buf[len++] = ELEMENT_TYPE_STRING;
    return len;
}

static UINT make_type_sig( const type_t *type, BYTE *buf )
{
    UINT len = 0;

    if (type->name && !strcmp( type->name, "HSTRING" ))
    {
        buf[0] = ELEMENT_TYPE_STRING;
        return 1;
    }

    type = type_get_real_type( type );

    switch (type_get_type( type ))
    {
    case TYPE_POINTER:
    {
        const type_t *ref_type = type_pointer_get_ref_type( type );
        BOOL skip_byref = FALSE;

        switch (type_get_type( ref_type ))
        {
        case TYPE_DELEGATE:
        case TYPE_INTERFACE:
        case TYPE_RUNTIMECLASS:
            skip_byref = TRUE;
            break;
        default:
            break;
        }
        if (!skip_byref) buf[len++] = ELEMENT_TYPE_BYREF;
        len += make_type_sig( ref_type, buf + len );
        break;
    }
    case TYPE_ARRAY:
        buf[len++] = ELEMENT_TYPE_SZARRAY;
        len += make_type_sig( type_array_get_element_type(type), buf + len );
        break;

    case TYPE_INTERFACE:
        buf[len++] = ELEMENT_TYPE_OBJECT;
        break;

    case TYPE_DELEGATE:
    case TYPE_RUNTIMECLASS:
        buf[len++] = ELEMENT_TYPE_CLASS;
        len += encode_int( typedef_or_ref(TABLE_TYPEREF, type->md.ref), buf + 1 );
        break;

    case TYPE_ENUM:
    case TYPE_STRUCT:
        buf[len++] = ELEMENT_TYPE_VALUETYPE;
        len += encode_int( typedef_or_ref(TABLE_TYPEREF, type->md.ref), buf + 1 );
        break;

    case TYPE_BASIC:
        buf[len++] = map_basic_type( type_basic_get_type(type), type_basic_get_sign(type) );
        break;

    default:
        fprintf( stderr, "Unhandled type %u.\n", type_get_type( type ) );
        exit( 1 );
    }
    return len;
}

static BOOL is_retval( const var_t *arg )
{
    const type_t *type = arg->declspec.type;

    /* array return values are encoded as out parameters even if the retval attribute is present */
    if (!is_attr( arg->attrs, ATTR_RETVAL ) || type_get_type( type ) == TYPE_ARRAY) return FALSE;
    return TRUE;
}

static UINT make_method_sig( const var_t *method, BYTE *buf, BOOL is_static )
{
    const var_t *arg;
    const var_list_t *arg_list = type_function_get_args( method->declspec.type );
    UINT len = 3;

    buf[0] = is_static ? SIG_TYPE_DEFAULT : SIG_TYPE_HASTHIS;
    buf[1] = 0;
    buf[2] = ELEMENT_TYPE_VOID;

    if (!arg_list) return 3;

    /* add return value first */
    LIST_FOR_EACH_ENTRY( arg, arg_list, var_t, entry )
    {
        const type_t *type;

        if (!is_retval( arg )) continue;
        type = type_pointer_get_ref_type( arg->declspec.type ); /* retval must be a pointer */
        len = make_type_sig( type, buf + 2 ) + 2;
    }

    /* add remaining parameters */
    LIST_FOR_EACH_ENTRY( arg, arg_list, var_t, entry )
    {
        if (is_size_param( arg, arg_list ) || is_retval( arg ) ) continue;
        len += make_type_sig( arg->declspec.type, buf + len );
        buf[1]++;
    }
    return len;
}

static UINT make_property_sig( const var_t *method, BYTE *buf, BOOL is_static )
{
    const var_t *arg;
    const var_list_t *arg_list = type_function_get_args( method->declspec.type );
    UINT len = 3;

    buf[0] = is_static ? SIG_TYPE_PROPERTY : SIG_TYPE_HASTHIS | SIG_TYPE_PROPERTY;
    buf[1] = 0;
    buf[2] = ELEMENT_TYPE_VOID;

    LIST_FOR_EACH_ENTRY( arg, arg_list, var_t, entry )
    {
        const type_t *type;

        if (!is_retval( arg )) continue;
        type = type_pointer_get_ref_type( arg->declspec.type ); /* retval must be a pointer */
        len = make_type_sig( type, buf + 2 ) + 2;
    }

    return len;
}

static UINT make_activation_sig( const var_t *method, BYTE *buf )
{
    const var_t *arg;
    UINT len = 3;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 0;
    buf[2] = ELEMENT_TYPE_VOID;

    if (method) LIST_FOR_EACH_ENTRY( arg, type_function_get_args(method->declspec.type), var_t, entry )
    {
        if (is_retval( arg )) continue;
        len += make_type_sig( arg->declspec.type, buf + len );
        buf[1]++;
    }

    return len;
}

static UINT make_composition_sig( const var_t *method, BYTE *buf )
{
    const var_t *arg;
    UINT len = 3, count = 0;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 0;
    buf[2] = ELEMENT_TYPE_VOID;

    if (method) LIST_FOR_EACH_ENTRY( arg, type_function_get_args(method->declspec.type), var_t, entry ) count++;

    if (method) assert( count >= 3 );

    if (count > 3) LIST_FOR_EACH_ENTRY( arg, type_function_get_args(method->declspec.type), var_t, entry )
    {
        if (--count < 3) break; /* omit last 3 standard composition args */
        len += make_type_sig( arg->declspec.type, buf + len );
        buf[1]++;
    }

    return len;
}

static UINT make_deprecated_sig( UINT token, BYTE *buf )
{
    UINT len = 5;

    buf[0] = SIG_TYPE_HASTHIS;
    buf[1] = 4;
    buf[2] = ELEMENT_TYPE_VOID;
    buf[3] = ELEMENT_TYPE_STRING;
    buf[4] = ELEMENT_TYPE_VALUETYPE;
    len += encode_int( token, buf + 5 );
    buf[len++] = ELEMENT_TYPE_U4;
    buf[len++] = ELEMENT_TYPE_STRING;

    return len;
}

static UINT make_contract_value( const attr_t *attr, BYTE *buf )
{
    const expr_t *expr = attr->u.pval;
    const type_t *contract = expr->u.var->declspec.type;
    char *name = format_namespace( contract->namespace, "", ".", contract->name, NULL );
    UINT version = expr->ref->u.integer.value, len = strlen( name );

    buf[0] = 1;
    buf[1] = 0;
    buf[2] = len;
    memcpy( buf + 3, name, len );
    len += 3;
    memcpy( buf + len, &version, sizeof(version) );
    len += sizeof(version);
    buf[len++] = 0;
    buf[len++] = 0;

    free( name );
    return len;
}

static UINT make_version_value( const attr_t *attr, BYTE *buf )
{
    const version_t *version;
    UINT value;

    if (attr && (version = attr->u.pval)) value = (version->major << 16) | version->minor;
    else value = 1;

    buf[0] = 1;
    buf[1] = 0;
    memcpy( buf + 2, &value, sizeof(value) );
    buf[6] = buf[7] = 0;
    return 8;
}

static attr_t *get_attr( const attr_list_t *list, enum attr_type attr_type )
{
    attr_t *attr;
    if (list) LIST_FOR_EACH_ENTRY( attr, list, attr_t, entry )
    {
        if (attr->type == attr_type ) return attr;
    }
    return NULL;
}

static void add_contract_attr_step1( const type_t *type )
{
    UINT assemblyref, scope, typeref, typeref_type, class, sig_size;
    BYTE sig[32];
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_CONTRACT ))) return;

    add_assemblyref_row( 0x200, 0, add_string("windowscontracts") );
    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref_type = add_typeref_row( scope, add_string("Type"), add_string("System") );

    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("ContractVersionAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    sig_size = make_member_sig( typedef_or_ref(TABLE_TYPEREF, typeref_type), sig );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
}

static void add_contract_attr_step2( const type_t *type )
{
    UINT parent, attr_type, value_size;
    BYTE value[MAX_NAME + sizeof(UINT) + 5];
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_CONTRACT ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_contract_value( attr, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static void add_version_attr_step1( const type_t *type )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U4 };
    UINT assemblyref, scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_VERSION )) && is_attr( type->attrs, ATTR_CONTRACT )) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );

    typeref = add_typeref_row( scope, add_string("VersionAttribute"), add_string("Windows.Foundation.Metadata") );
    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_version_attr_step2( const type_t *type )
{
    UINT parent, attr_type, value_size;
    BYTE value[8];
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_VERSION )) && is_attr( type->attrs, ATTR_CONTRACT )) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_version_value( attr, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static void add_flags_attr_step1( const type_t *type )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 0, ELEMENT_TYPE_VOID };
    UINT scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_FLAGS ))) return;

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref = add_typeref_row( scope, add_string("FlagsAttribute"), add_string("System") );
    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_flags_attr_step2( const type_t *type )
{
    static const BYTE value[] = { 0x01, 0x00, 0x00, 0x00 };
    UINT parent, attr_type;
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_FLAGS ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    add_customattribute_row( parent, attr_type, add_blob(value, sizeof(value)) );
}

static void add_enum_type_step1( type_t *type )
{
    UINT scope, typeref;

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref = add_typeref_row( scope, add_string("Enum"), add_string("System") );
    type->md.extends = typedef_or_ref( TABLE_TYPEREF, typeref );

    create_typeref( type );

    add_version_attr_step1( type );
    add_contract_attr_step1( type );
    add_flags_attr_step1( type );
}

static void add_enum_type_step2( type_t *type )
{
    BYTE sig_value[] = { SIG_TYPE_FIELD, ELEMENT_TYPE_I4 };
    UINT field, parent, sig_size;
    BYTE sig_field[32];
    const var_t *var;

    if (is_attr( type->attrs, ATTR_FLAGS )) sig_value[1] = ELEMENT_TYPE_U4;

    field = add_field_row( FIELD_ATTR_PRIVATE | FIELD_ATTR_SPECIALNAME | FIELD_ATTR_RTSPECIALNAME,
                           add_string("value__"), add_blob(sig_value, sizeof(sig_value)) );

    type->md.def = add_typedef_row( TYPE_ATTR_PUBLIC | TYPE_ATTR_SEALED | TYPE_ATTR_UNKNOWN, type->md.name,
                                    type->md.namespace, type->md.extends, field, 1 );

    sig_size = make_field_value_sig( typedef_or_ref(TABLE_TYPEREF, type->md.ref), sig_field );

    LIST_FOR_EACH_ENTRY( var, type_enum_get_values(type), const var_t, entry )
    {
        int val = var->eval->u.integer.value;

        field = add_field_row( FIELD_ATTR_PUBLIC | FIELD_ATTR_LITERAL | FIELD_ATTR_STATIC | FIELD_ATTR_HASDEFAULT,
                               add_string(var->name), add_blob(sig_field, sig_size) );
        parent = has_constant( TABLE_FIELD, field );
        add_constant_row( sig_value[1], parent, add_blob((const BYTE *)&val, sizeof(val)) );
    }

    add_version_attr_step2( type );
    add_contract_attr_step2( type );
    add_flags_attr_step2( type );
}

static void add_struct_type_step1( type_t *type )
{
    UINT scope, typeref;
    const var_t *var;

    LIST_FOR_EACH_ENTRY( var, type_struct_get_fields(type), const var_t, entry )
    {
        create_typeref( var->declspec.type );
    }

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref = add_typeref_row( scope, add_string("ValueType"), add_string("System") );
    type->md.extends = typedef_or_ref( TABLE_TYPEREF, typeref );

    create_typeref( type );

    add_contract_attr_step1( type );
}

static void add_struct_type_step2( type_t *type )
{
    UINT field, flags, sig_size, first_field = 0;
    const var_t *var;
    BYTE sig[32];

    LIST_FOR_EACH_ENTRY( var, type_struct_get_fields(type), const var_t, entry )
    {
        sig_size = make_struct_field_sig( var, sig );
        field = add_field_row( FIELD_ATTR_PUBLIC, add_string(var->name), add_blob(sig, sig_size) );
        if (!first_field) first_field = field;
    }

    flags = TYPE_ATTR_PUBLIC | TYPE_ATTR_SEQUENTIALLAYOUT | TYPE_ATTR_SEALED | TYPE_ATTR_UNKNOWN;
    type->md.def = add_typedef_row( flags, type->md.name, type->md.namespace, type->md.extends, first_field, 0 );

    add_contract_attr_step2( type );
}

static void add_uuid_attr_step1( const type_t *type )
{
    static const BYTE sig[] =
        { SIG_TYPE_HASTHIS, 11, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U4, ELEMENT_TYPE_U2, ELEMENT_TYPE_U2,
          ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1, ELEMENT_TYPE_U1,
          ELEMENT_TYPE_U1, ELEMENT_TYPE_U1 };
    UINT assemblyref, scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_UUID ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("GuidAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_uuid_attr_step2( const type_t *type )
{
    const struct uuid *uuid;
    BYTE value[sizeof(*uuid) + 4] = { 0x01 };
    UINT parent, attr_type;
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_UUID ))) return;

    uuid = attr->u.pval;
    memcpy( value + 2, uuid, sizeof(*uuid) );

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    add_customattribute_row( parent, attr_type, add_blob(value, sizeof(value)) );
}

static UINT make_exclusiveto_value( const attr_t *attr, BYTE *buf )
{
    const type_t *type = attr->u.pval;
    char *name = format_namespace( type->namespace, "", ".", type->name, NULL );
    UINT len = strlen( name );

    buf[0] = 1;
    buf[1] = 0;
    buf[2] = len;
    memcpy( buf + 3, name, len );
    len += 3;
    buf[len++] = 0;
    buf[len++] = 0;

    free( name );
    return len;
}

static void add_exclusiveto_attr_step1( const type_t *type )
{
    UINT assemblyref, scope, typeref, typeref_type, class, sig_size;
    BYTE sig[32];
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_EXCLUSIVETO ))) return;

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref_type = add_typeref_row( scope, add_string("Type"), add_string("System") );

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("ExclusiveToAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    sig_size = make_member_sig2( ELEMENT_TYPE_CLASS, typedef_or_ref(TABLE_TYPEREF, typeref_type), sig );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
}

static void add_exclusiveto_attr_step2( const type_t *type )
{
    UINT parent, attr_type, value_size;
    BYTE value[MAX_NAME + 5];
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_EXCLUSIVETO ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_exclusiveto_value( attr, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static UINT make_overload_value( const char *name, BYTE *buf )
{
    UINT len = strlen( name );

    buf[0] = 1;
    buf[1] = 0;
    buf[2] = len;
    memcpy( buf + 3, name, len );
    len += 3;
    buf[len++] = 0;
    buf[len++] = 0;

    return len;
}

static void add_overload_attr_step1( const var_t *method )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING };
    UINT assemblyref, scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( method->attrs, ATTR_OVERLOAD ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("OverloadAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_overload_attr_step2( const var_t *method )
{
    const type_t *type = method->declspec.type;
    UINT parent, attr_type, value_size;
    BYTE value[MAX_NAME + 5];
    const attr_t *attr;

    if (!(attr = get_attr( method->attrs, ATTR_OVERLOAD ))) return;

    parent = has_customattribute( TABLE_METHODDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_overload_value( method->name, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static void add_default_overload_attr_step1( const var_t *method )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 0, ELEMENT_TYPE_VOID };
    UINT assemblyref, scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( method->attrs, ATTR_DEFAULT_OVERLOAD )) || !is_attr( method->attrs, ATTR_OVERLOAD )) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("DefaultOverloadAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_default_overload_attr_step2( const var_t *method )
{
    static const BYTE value[] = { 0x01, 0x00, 0x00, 0x00 };
    const type_t *type = method->declspec.type;
    UINT parent, attr_type;
    const attr_t *attr;

    if (!(attr = get_attr( method->attrs, ATTR_DEFAULT_OVERLOAD )) || !is_attr( method->attrs, ATTR_OVERLOAD )) return;

    parent = has_customattribute( TABLE_METHODDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    add_customattribute_row( parent, attr_type, add_blob(value, sizeof(value)) );
}

static UINT make_deprecated_value( const attr_t *attr, BYTE **ret_buf )
{
    static const BYTE zero[] = { 0x00, 0x00, 0x00, 0x00 }, one[] = { 0x01, 0x00, 0x00, 0x00 };
    const expr_t *expr = attr->u.pval;
    const type_t *type = expr->ext2->u.var->declspec.type;
    const char *text = expr->ref->u.sval;
    const char *kind = expr->u.ext->u.sval;
    BYTE encoded[4];
    UINT len, version, len_text = strlen( text ), len_encoded = encode_int( len_text, encoded );
    BYTE *buf = xmalloc( 2 + len_encoded + len_text + 6 + MAX_NAME + 5 );
    char *contract;

    buf[0] = 1;
    buf[1] = 0;
    memcpy( buf + 2, encoded, len_encoded );
    len = 2 + len_encoded;
    memcpy( buf + len, text, len_text );
    len += len_text;
    if (!strcmp( kind, "remove" )) memcpy( buf + len, one, sizeof(one) );
    else memcpy( buf + len, zero, sizeof(zero) );
    len += 4;

    version = expr->ext2->ref->u.integer.value;
    memcpy( buf + len, &version, sizeof(version) );
    len += sizeof(version);

    contract = format_namespace( type->namespace, "", ".", type->name, NULL );
    len_text = strlen( contract );
    buf[len++] = len_text;
    memcpy( buf + len, contract, len_text );
    free( contract );
    len += len_text;
    buf[len++] = 0;
    buf[len++] = 0;

    *ret_buf = buf;
    return len;
}

static void add_deprecated_attr_step1( const var_t *method )
{
    UINT assemblyref, scope, typeref_type, typeref, class, sig_size;
    BYTE sig[32];
    attr_t *attr;

    if (!(attr = get_attr( method->attrs, ATTR_DEPRECATED ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref_type = add_typeref_row( scope, add_string("DeprecationType"), add_string("Windows.Foundation.Metadata") );
    typeref = add_typeref_row( scope, add_string("DeprecatedAttribute"), add_string("Windows.Foundation.Metadata") );

    sig_size = make_deprecated_sig( typedef_or_ref(TABLE_TYPEREF, typeref_type), sig );
    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
}

static void add_deprecated_attr_step2( const var_t *method )
{
    const type_t *type = method->declspec.type;
    UINT parent, attr_type, value_size;
    BYTE *value;
    const attr_t *attr;

    if (!(attr = get_attr( method->attrs, ATTR_DEPRECATED ))) return;

    parent = has_customattribute( TABLE_METHODDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_deprecated_value( attr, &value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    free( value );
}

static void add_method_params_step1( const var_t *method )
{
    const var_list_t *arg_list = type_function_get_args( method->declspec.type );
    const var_t *arg;

    if (arg_list) LIST_FOR_EACH_ENTRY( arg, arg_list, var_t, entry )
    {
        if (is_size_param( arg, arg_list )) continue;
        create_typeref( arg->declspec.type );
    }
}

static void add_runtimeclass_type_step1( type_t * );

static void add_interface_type_step1( type_t *type )
{
    const statement_t *stmt;
    type_t *class;

    create_typeref( type );

    add_exclusiveto_attr_step1( type );

    if ((class = type->details.iface->runtime_class)) add_runtimeclass_type_step1( class );

    add_contract_attr_step1( type );
    add_uuid_attr_step1( type );

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(type) )
    {
        const var_t *method = stmt->u.var;

        add_method_params_step1( method );

        add_overload_attr_step1( method );
        add_default_overload_attr_step1( method );
        add_deprecated_attr_step1( method );
    }
}

static UINT get_param_attrs( const var_t *arg )
{
    UINT attrs = 0;

    if (is_attr( arg->attrs, ATTR_IN )) attrs |= PARAM_ATTR_IN;
    if (is_attr( arg->attrs, ATTR_OUT )) attrs |= PARAM_ATTR_OUT;
    if (is_attr( arg->attrs, ATTR_OPTIONAL )) attrs |= PARAM_ATTR_OPTIONAL;

    return attrs ? attrs : PARAM_ATTR_IN;
}

static UINT add_method_params_step2( var_list_t *arg_list )
{
    UINT first = 0, row, seq = 1;
    var_t *arg;

    if (!arg_list) return 0;

    LIST_FOR_EACH_ENTRY( arg, arg_list, var_t, entry )
    {
        if (is_retval( arg ))
        {
            first = add_param_row( 0, 0, add_string(arg->name) );
            break;
        }
    }

    LIST_FOR_EACH_ENTRY( arg, arg_list, var_t, entry )
    {
        if (is_size_param( arg, arg_list) || is_retval( arg )) continue;
        row = add_param_row( get_param_attrs(arg), seq++, add_string(arg->name) );
        if (!first) first = row;
    }

    return first;
}

static char *get_method_name( const var_t *method )
{
    const char *overload;

    if (is_attr( method->attrs, ATTR_PROPGET )) return strmake( "get_%s", method->name );
    else if (is_attr( method->attrs, ATTR_PROPPUT )) return strmake( "put_%s", method->name );
    else if (is_attr( method->attrs, ATTR_EVENTADD )) return strmake( "add_%s", method->name );
    else if (is_attr( method->attrs, ATTR_EVENTREMOVE )) return strmake( "remove_%s", method->name );

    if ((overload = get_attrp( method->attrs, ATTR_OVERLOAD ))) return strmake( "%s", overload );
    return strmake( "%s", method->name );
}

static BOOL is_special_method( const var_t *method )
{
    if (is_attr( method->attrs, ATTR_PROPGET )  || is_attr( method->attrs, ATTR_PROPPUT ) ||
        is_attr( method->attrs, ATTR_EVENTADD ) || is_attr( method->attrs, ATTR_EVENTREMOVE )) return TRUE;
    return FALSE;
}

static BOOL is_static_iface( const type_t *class, const type_t *iface )
{
    const attr_t *attr;

    if (!class || !class->attrs) return FALSE;

    LIST_FOR_EACH_ENTRY( attr, class->attrs, const attr_t, entry )
    {
        const expr_t *value = attr->u.pval;

        if (attr->type != ATTR_STATIC) continue;
        if (value->u.var->declspec.type == iface) return TRUE;
    }

    return FALSE;
}

static UINT get_method_attrs( const type_t *class, const type_t *iface, const var_t *method, UINT *flags )
{
    UINT attrs = METHOD_ATTR_PUBLIC | METHOD_ATTR_HIDEBYSIG;

    if (!class)
    {
        *flags = 0;
        attrs |= METHOD_ATTR_ABSTRACT | METHOD_ATTR_VIRTUAL | METHOD_ATTR_NEWSLOT;
    }
    else
    {
        *flags = METHOD_IMPL_RUNTIME;
        if (is_static_iface( class, iface )) attrs |= METHOD_ATTR_STATIC;
        else attrs |= METHOD_ATTR_VIRTUAL | METHOD_ATTR_NEWSLOT | METHOD_ATTR_FINAL;
    }

    if (is_special_method( method )) attrs |= METHOD_ATTR_SPECIALNAME;
    return attrs;
}

static void add_propget_method( const type_t *class, const type_t *iface, const var_t *method )
{
    UINT sig_size, property, paramlist, flags, attrs = get_method_attrs( class, iface, method, &flags );
    char *name = get_method_name( method );
    type_t *type = method->declspec.type;
    BYTE sig[256];

    if (class) property = type->md.class_property;
    else property = type->md.iface_property;

    paramlist = add_method_params_step2( type_function_get_args(type) );
    sig_size = make_method_sig( method, sig, is_static_iface(class, iface) );

    type->md.def = add_methoddef_row( flags, attrs, add_string(name), add_blob(sig, sig_size), paramlist );
    add_methodsemantics_row( METHOD_SEM_GETTER, type->md.def, has_semantics(TABLE_PROPERTY, property) );
    free( name );
}

static const var_t *find_propget_method( const type_t *iface, const char *name )
{
    const statement_t *stmt;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *method = stmt->u.var;
        if (is_attr( method->attrs, ATTR_PROPGET ) && !strcmp( method->name, name )) return method;
    }
    return NULL;
}

static void add_propput_method( const type_t *class, const type_t *iface, const var_t *method )
{
    const var_t *propget = find_propget_method( iface, method->name );
    UINT sig_size, paramlist, property, flags, attrs = get_method_attrs( class, iface, method, &flags );
    char *name = get_method_name( method );
    type_t *type = method->declspec.type;
    BYTE sig[256];

    paramlist = add_method_params_step2( type_function_get_args(method->declspec.type) );
    sig_size = make_method_sig( method, sig, is_static_iface(class, iface) );

    type->md.def = add_methoddef_row( flags, attrs, add_string(name), add_blob(sig, sig_size), paramlist );
    free( name );

    /* add propget method first if not already added */
    if (class)
    {
        if (!propget->declspec.type->md.class_property) add_propget_method( class, iface, propget );
        property = type->md.class_property = propget->declspec.type->md.class_property;
    }
    else
    {
        if (!propget->declspec.type->md.iface_property) add_propget_method( class, iface, propget );
        property = type->md.iface_property = propget->declspec.type->md.iface_property;
    }

    add_methodsemantics_row( METHOD_SEM_SETTER, type->md.def, has_semantics(TABLE_PROPERTY, property) );
}

static void add_eventadd_method( const type_t *class, const type_t *iface, const var_t *method )
{
    UINT event, sig_size, paramlist, flags, attrs = get_method_attrs( class, iface, method, &flags );
    char *name = get_method_name( method );
    type_t *type = method->declspec.type;
    BYTE sig[256];

    if (class) event = type->md.class_event;
    else event = type->md.iface_event;

    paramlist = add_method_params_step2( type_function_get_args(type) );
    sig_size = make_method_sig( method, sig, is_static_iface(class, iface) );

    type->md.def = add_methoddef_row( flags, attrs, add_string(name), add_blob(sig, sig_size), paramlist );
    free( name );

    add_methodsemantics_row( METHOD_SEM_ADDON, type->md.def, has_semantics(TABLE_EVENT, event) );
}

static const var_t *find_eventadd_method( const type_t *iface, const char *name )
{
    const statement_t *stmt;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *method = stmt->u.var;
        if (is_attr( method->attrs, ATTR_EVENTADD ) && !strcmp( method->name, name )) return method;
    }
    return NULL;
}

static void add_eventremove_method( const type_t *class, const type_t *iface, const var_t *method )
{
    const var_t *eventadd = find_eventadd_method( iface, method->name );
    UINT event, sig_size, paramlist, flags, attrs = get_method_attrs( class, iface, method, &flags );
    char *name = get_method_name( method );
    type_t *type = method->declspec.type;
    BYTE sig[256];

    paramlist = add_method_params_step2( type_function_get_args(type) );
    sig_size = make_method_sig( method, sig, is_static_iface(class, iface) );

    type->md.def = add_methoddef_row( flags, attrs, add_string(name), add_blob(sig, sig_size), paramlist );
    free( name );

    /* add eventadd method first if not already added */
    if (class)
    {
        if (!eventadd->declspec.type->md.class_event) add_eventadd_method( class, iface, eventadd );
        event = type->md.class_event = eventadd->declspec.type->md.class_event;
    }
    else
    {
        if (!eventadd->declspec.type->md.iface_event) add_eventadd_method( class, iface, eventadd );
        event = type->md.iface_event = eventadd->declspec.type->md.iface_event;
    }

    add_methodsemantics_row( METHOD_SEM_REMOVEON, type->md.def, has_semantics(TABLE_EVENT, event) );
}

static void add_regular_method( const type_t *class, const type_t *iface, const var_t *method )
{
    UINT paramlist, sig_size, flags, attrs = get_method_attrs( class, iface, method, &flags );
    char *name = get_method_name( method );
    type_t *type = method->declspec.type;
    BYTE sig[256];

    paramlist = add_method_params_step2( type_function_get_args(type) );
    sig_size = make_method_sig( method, sig, is_static_iface(class, iface) );

    type->md.def = add_methoddef_row( flags, attrs, add_string(name), add_blob(sig, sig_size), paramlist );
    free( name );
}

static void add_property( type_t *class, type_t *iface, const var_t *method )
{
    UINT sig_size;
    type_t *type = method->declspec.type;
    BYTE sig[256];

    if (!is_attr( method->attrs, ATTR_PROPGET )) return;

    sig_size = make_property_sig( method, sig, is_static_iface(class, iface) );
    if (class)
    {
        type->md.class_property = add_property_row( 0, add_string(method->name), add_blob(sig, sig_size) );
        if (!class->md.propertymap) class->md.propertymap = add_propertymap_row( class->md.def, type->md.class_property );
    }
    else
    {
        type->md.iface_property = add_property_row( 0, add_string(method->name), add_blob(sig, sig_size) );
        if (!iface->md.propertymap) iface->md.propertymap = add_propertymap_row( iface->md.def, type->md.iface_property );
    }
}

static void add_event( type_t *class, type_t *iface, const var_t *method )
{
    UINT event_type = 0;
    type_t *type = method->declspec.type;
    var_t *arg;

    if (!is_attr( method->attrs, ATTR_EVENTADD )) return;

    LIST_FOR_EACH_ENTRY( arg, type_function_get_args(type), var_t, entry )
    {
        type_t *arg_type = arg->declspec.type;

        arg_type = type_pointer_get_ref_type( arg_type ); /* first arg must be a delegate pointer */
        event_type = typedef_or_ref( TABLE_TYPEREF, arg_type->md.ref );
        break;
    }

    if (class)
    {
        type->md.class_event = add_event_row( 0, add_string(method->name), event_type );
        if (!class->md.eventmap) class->md.eventmap = add_eventmap_row( class->md.def, type->md.class_event );
    }
    else
    {
        type->md.iface_event = add_event_row( 0, add_string(method->name), event_type );
        if (!iface->md.eventmap) iface->md.eventmap = add_eventmap_row( iface->md.def, type->md.iface_event );
    }
}

static void add_method( type_t *class, type_t *iface, const var_t *method )
{
    if (is_attr( method->attrs, ATTR_PROPGET )) add_propget_method( class, iface, method );
    else if (is_attr( method->attrs, ATTR_PROPPUT )) add_propput_method( class, iface, method );
    else if (is_attr( method->attrs, ATTR_EVENTADD )) add_eventadd_method( class, iface, method );
    else if (is_attr( method->attrs, ATTR_EVENTREMOVE )) add_eventremove_method( class, iface, method );
    else add_regular_method( class, iface, method );
}

static void add_runtimeclass_type_step2( type_t *type );

static void add_interface_type_step2( type_t *type )
{
    UINT interface, flags = TYPE_ATTR_INTERFACE | TYPE_ATTR_ABSTRACT | TYPE_ATTR_UNKNOWN;
    const typeref_list_t *require_list = type_iface_get_requires( type );
    const typeref_t *require;
    const statement_t *stmt;
    type_t *class;

    if (!is_attr( type->attrs, ATTR_EXCLUSIVETO )) flags |= TYPE_ATTR_PUBLIC;
    type->md.def = add_typedef_row( flags, type->md.name, type->md.namespace, 0, 0, 0 );

    if (require_list) LIST_FOR_EACH_ENTRY( require, require_list, typeref_t, entry )
    {
        interface = typedef_or_ref( TABLE_TYPEREF, require->type->md.ref );
        add_interfaceimpl_row( type->md.def, interface );
    }

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(type) )
    {
        const var_t *method = stmt->u.var;

        add_property( NULL, type, method );
        add_event( NULL, type, method );
        add_method( NULL, type, method );

        add_deprecated_attr_step2( method );
        add_default_overload_attr_step2( method );
        add_overload_attr_step2( method );
    }

    if ((class = type->details.iface->runtime_class)) add_runtimeclass_type_step2( class );

    add_contract_attr_step2( type );
    add_uuid_attr_step2( type );
    add_exclusiveto_attr_step2( type );
}

static void add_contractversion_attr_step1( const type_t *type )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U4 };
    UINT assemblyref, scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_CONTRACTVERSION ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("ContractVersionAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_contractversion_attr_step2( const type_t *type )
{
    UINT parent, attr_type, value_size;
    BYTE value[8];
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_CONTRACTVERSION ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_version_value( attr, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static void add_apicontract_attr_step1( const type_t *type )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 0, ELEMENT_TYPE_VOID };
    UINT assemblyref, scope, typeref, class;
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_APICONTRACT ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("ApiContractAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );
}

static void add_apicontract_attr_step2( const type_t *type )
{
    static const BYTE value[] = { 0x01, 0x00, 0x00, 0x00 };
    UINT parent, attr_type;
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_APICONTRACT ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    add_customattribute_row( parent, attr_type, add_blob(value, sizeof(value)) );
}

static void add_apicontract_type_step1( type_t *type )
{
    UINT scope, typeref;

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref = add_typeref_row( scope, add_string("ValueType"), add_string("System") );
    type->md.extends = typedef_or_ref( TABLE_TYPEREF, typeref );

    create_typeref( type );

    add_contractversion_attr_step1( type );
    add_apicontract_attr_step1( type );
}

static void add_apicontract_type_step2( type_t *type )
{
    UINT flags = TYPE_ATTR_PUBLIC | TYPE_ATTR_SEQUENTIALLAYOUT | TYPE_ATTR_SEALED | TYPE_ATTR_UNKNOWN;

    type->md.def = add_typedef_row( flags, type->md.name, type->md.namespace, type->md.extends, 0, 1 );

    add_contractversion_attr_step2( type );
    add_apicontract_attr_step2( type );
}

static void add_runtimeclass_type_step1( type_t *type )
{
    const typeref_list_t *iface_list = type_runtimeclass_get_ifaces( type );
    const typeref_t *iface;
    UINT scope, typeref;

    create_typeref( type );

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref = add_typeref_row( scope, add_string("Object"), add_string("System") );
    type->md.extends = typedef_or_ref( TABLE_TYPEREF, typeref );

    if (iface_list) LIST_FOR_EACH_ENTRY( iface, iface_list, typeref_t, entry )
    {
        create_typeref( iface->type );
    }
}

static void add_default_attr( UINT interfaceimpl_ref )
{
    static const BYTE sig[] = { SIG_TYPE_HASTHIS, 0, ELEMENT_TYPE_VOID };
    static const BYTE value[] = { 0x01, 0x00, 0x00, 0x00 };
    UINT assemblyref, scope, typeref, class, memberref, parent, attr_type;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("DefaultAttribute"), add_string("Windows.Foundation.Metadata") );
    class = memberref_parent( TABLE_TYPEREF, typeref );
    memberref = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sizeof(sig)) );

    parent = has_customattribute( TABLE_INTERFACEIMPL, interfaceimpl_ref );
    attr_type = customattribute_type( TABLE_MEMBERREF, memberref );
    add_customattribute_row( parent, attr_type, add_blob(value, sizeof(value)) );
}

static void add_method_impl( const type_t *class, const type_t *iface, const var_t *method )
{
    UINT parent, memberref, body, decl, sig_size;
    char *name = get_method_name( method );
    type_t *type = method->declspec.type;
    BYTE sig[256];

    parent = memberref_parent( TABLE_TYPEREF, iface->md.ref );
    sig_size = make_method_sig( method, sig, FALSE );

    memberref = add_memberref_row( parent, add_string(name), add_blob(sig, sig_size) );
    free( name );

    body = methoddef_or_ref( TABLE_METHODDEF, type->md.def );
    decl = methoddef_or_ref( TABLE_MEMBERREF, memberref );

    add_methodimpl_row( class->md.def, body, decl );
}

static void add_method_contract_attrs( const type_t *class, const type_t *iface, const type_t *method )
{
    UINT parent, attr_type, value_size;
    BYTE value[MAX_NAME + sizeof(UINT) + 5];
    const attr_t *attr = get_attr( iface->attrs, ATTR_CONTRACT );

    parent = has_customattribute( TABLE_METHODDEF, method->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_contract_value( get_attr(class->attrs, ATTR_CONTRACT), value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );

    if (method->md.class_property)
    {
        parent = has_customattribute( TABLE_PROPERTY, method->md.class_property );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }

    if (method->md.class_event)
    {
        parent = has_customattribute( TABLE_EVENT, method->md.class_event );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }
}

static UINT make_static_value( const expr_t *attr, BYTE *buf )
{
    const expr_t *contract = attr->ref;
    const type_t *type_iface = attr->u.var->declspec.type, *type_contract = contract->u.var->declspec.type;
    char *name_iface = format_namespace( type_iface->namespace, "", ".", type_iface->name, NULL );
    char *name_contract = format_namespace( type_contract->namespace, "", ".", type_contract->name, NULL );
    UINT len_iface = strlen( name_iface ), len_contract = strlen( name_contract );
    BYTE *ptr = buf;

    ptr[0] = 1;
    ptr[1] = 0;
    ptr[2] = len_iface;
    memcpy( ptr + 3, name_iface, len_iface );
    ptr += len_iface + 3;
    ptr[0] = ptr[1] = 0;

    ptr += 2;
    ptr[0] = 1;
    ptr[1] = 0;
    ptr[2] = len_contract;
    memcpy( ptr + 3, name_contract, len_contract );
    ptr += len_contract + 3;
    ptr[0] = ptr[1] = 0;

    free( name_iface );
    free( name_contract );
    return len_iface + len_contract + 10;
}

static void add_static_attr_step1( const type_t *type )
{
    UINT assemblyref, scope, typeref, typeref_type, class, sig_size;
    BYTE sig[32];
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_STATIC ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("StaticAttribute"), add_string("Windows.Foundation.Metadata") );

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref_type = add_typeref_row( scope, add_string("Type"), add_string("System") );

    class = memberref_parent( TABLE_TYPEREF, typeref );
    sig_size = make_member_sig3( typedef_or_ref(TABLE_TYPEREF, typeref_type), sig );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
}

static void add_static_attr_step2( const type_t *type )
{
    const attr_t *attr;

    if (type->attrs) LIST_FOR_EACH_ENTRY_REV( attr, type->attrs, const attr_t, entry )
    {
        UINT parent, attr_type, value_size;
        BYTE value[MAX_NAME * 2 + 10];

        if (attr->type != ATTR_STATIC) continue;

        parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
        attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
        value_size = make_static_value( attr->u.pval, value );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }
}

static UINT make_activatable_value( const expr_t *attr, BYTE *buf )
{
    char *name_iface = NULL, *name_contract;
    const type_t *type_iface, *type_contract;
    UINT version, len_iface = 0, len_contract, len_extra = 5;
    BYTE *ptr = buf;

    if (attr->type == EXPR_MEMBER)
    {
        type_iface = attr->u.var->declspec.type;
        name_iface = format_namespace( type_iface->namespace, "", ".", type_iface->name, NULL );
        len_iface = strlen( name_iface );

        type_contract = attr->ref->u.var->declspec.type;
        version = attr->ref->ref->u.integer.value;
    }
    else
    {
        type_contract = attr->u.var->declspec.type;
        version = attr->ref->u.integer.value;
    }

    name_contract = format_namespace( type_contract->namespace, "", ".", type_contract->name, NULL );
    len_contract = strlen( name_contract );

    if (len_iface)
    {
        ptr[0] = 1;
        ptr[1] = 0;
        ptr[2] = len_iface;
        memcpy( ptr + 3, name_iface, len_iface );
        ptr += len_iface + 3;
        ptr[0] = ptr[1] = 0;
        ptr += 2;
        len_extra += 5;
    }

    ptr[0] = 1;
    ptr[1] = 0;
    memcpy( ptr + 2, &version, sizeof(version) );
    ptr += sizeof(version) + 2;

    ptr[0] = len_contract;
    memcpy( ptr + 1, name_contract, len_contract );
    ptr += len_contract + 1;
    ptr[0] = ptr[1] = 0;

    free( name_iface );
    free( name_contract );
    return len_iface + sizeof(version) + len_contract + len_extra;
}

static void add_activatable_attr_step1( const type_t *type )
{
    static const BYTE sig_default[] = { SIG_TYPE_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_U4, ELEMENT_TYPE_STRING };
    attr_t *attr;

    if (type->attrs) LIST_FOR_EACH_ENTRY_REV( attr, type->attrs, attr_t, entry )
    {
        UINT assemblyref, scope, typeref, typeref_type, class, sig_size;
        const expr_t *value = attr->u.pval;
        BYTE sig[32];

        if (attr->type != ATTR_ACTIVATABLE) continue;

        assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
        scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
        typeref = add_typeref_row( scope, add_string("ActivatableAttribute"), add_string("Windows.Foundation.Metadata") );

        scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
        typeref_type = add_typeref_row( scope, add_string("Type"), add_string("System") );

        class = memberref_parent( TABLE_TYPEREF, typeref );

        if (value->type == EXPR_MEMBER)
            sig_size = make_member_sig3( typedef_or_ref(TABLE_TYPEREF, typeref_type), sig );
        else
        {
            memcpy( sig, sig_default, sizeof(sig_default) );
            sig_size = sizeof(sig_default);
        }

        attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
    }
}

static void add_activatable_attr_step2( const type_t *type )
{
    const attr_t *attr;

    if (type->attrs) LIST_FOR_EACH_ENTRY_REV( attr, type->attrs, const attr_t, entry )
    {
        UINT parent, attr_type, value_size;
        BYTE value[MAX_NAME * 2 + sizeof(UINT) + 10];

        if (attr->type != ATTR_ACTIVATABLE) continue;

        parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
        attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
        value_size = make_activatable_value( attr->u.pval, value );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }
}

static UINT make_threading_value( const attr_t *attr, BYTE *buf )
{
    UINT value, model = attr->u.ival;

    switch (model)
    {
    case THREADING_SINGLE:
        value = 1;
        break;
    case THREADING_FREE:
        value = 2;
        break;
    case THREADING_BOTH:
        value = 3;
        break;
    default:
        fprintf( stderr, "Unhandled model %u.\n", model );
        return 0;
    }

    buf[0] = 1;
    buf[1] = 0;
    memcpy( buf + 2, &value, sizeof(value) );
    buf[6] = buf[7] = 0;
    return 8;
}

static void add_threading_attr_step1( const type_t *type )
{
    UINT assemblyref, scope, typeref, typeref_attr, class, sig_size;
    BYTE sig[32];
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_THREADING ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("ThreadingModel"), add_string("Windows.Foundation.Metadata") );

    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref_attr = add_typeref_row( scope, add_string("ThreadingAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref_attr );
    sig_size = make_member_sig2( ELEMENT_TYPE_VALUETYPE, typedef_or_ref(TABLE_TYPEREF, typeref), sig );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
}

static void add_threading_attr_step2( const type_t *type )
{
    UINT parent, attr_type, value_size;
    BYTE value[8];
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_THREADING ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_threading_value( attr, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static UINT make_marshalingbehavior_value( const attr_t *attr, BYTE *buf )
{
    UINT marshaling = attr->u.ival;

    buf[0] = 1;
    buf[1] = 0;
    memcpy( buf + 2, &marshaling, sizeof(marshaling) );
    buf[6] = buf[7] = 0;
    return 8;
}

static void add_marshalingbehavior_attr_step1( const type_t *type )
{
    UINT assemblyref, scope, typeref, typeref_attr, class, sig_size;
    BYTE sig[32];
    attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_MARSHALING_BEHAVIOR ))) return;

    assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref = add_typeref_row( scope, add_string("MarshalingType"), add_string("Windows.Foundation.Metadata") );

    scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
    typeref_attr = add_typeref_row( scope, add_string("MarshalingBehaviorAttribute"), add_string("Windows.Foundation.Metadata") );

    class = memberref_parent( TABLE_TYPEREF, typeref_attr );
    sig_size = make_member_sig2( ELEMENT_TYPE_VALUETYPE, typedef_or_ref(TABLE_TYPEREF, typeref), sig );
    attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
}

static void add_marshalingbehavior_attr_step2( const type_t *type )
{
    UINT parent, attr_type, value_size;
    BYTE value[8];
    const attr_t *attr;

    if (!(attr = get_attr( type->attrs, ATTR_MARSHALING_BEHAVIOR ))) return;

    parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
    attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
    value_size = make_marshalingbehavior_value( attr, value );
    add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
}

static UINT make_composable_value( const expr_t *attr, BYTE *buf )
{
    char *name_iface, *name_contract;
    const expr_t *contract = attr->ref;
    const type_t *type_iface = attr->u.var->declspec.type;
    const type_t *type_contract = contract->u.var->declspec.type;
    UINT access_type = 1, contract_version = contract->ref->u.integer.value;
    UINT len_iface, len_contract;
    BYTE *ptr = buf;

    name_iface = format_namespace( type_iface->namespace, "", ".", type_iface->name, NULL );
    len_iface = strlen( name_iface );

    name_contract = format_namespace( type_contract->namespace, "", ".", type_contract->name, NULL );
    len_contract = strlen( name_contract );

    ptr[0] = 1;
    ptr[1] = 0;
    ptr[2] = len_iface;
    memcpy( ptr + 3, name_iface, len_iface );
    ptr += len_iface + 3;

    if (is_attr( attr->u.var->attrs, ATTR_PUBLIC)) access_type = 2;
    memcpy( ptr, &access_type, sizeof(access_type) );
    ptr += sizeof(access_type);

    memcpy( ptr, &contract_version, sizeof(contract_version) );
    ptr += sizeof(contract_version);

    ptr[0] = len_contract;
    memcpy( ptr + 1, name_contract, len_contract );
    ptr += len_contract + 1;
    ptr[0] = ptr[1] = 0;

    free( name_iface );
    free( name_contract );
    return len_iface + sizeof(access_type) + sizeof(contract_version) + len_contract + 6;
}

static void add_composable_attr_step1( const type_t *type )
{
    attr_t *attr;

    if (type->attrs) LIST_FOR_EACH_ENTRY( attr, type->attrs, attr_t, entry )
    {
        UINT assemblyref, scope, typeref, typeref_attr, typeref_type, class, sig_size;
        BYTE sig[64];

        if (attr->type != ATTR_COMPOSABLE) continue;

        scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
        typeref_type = add_typeref_row( scope, add_string("Type"), add_string("System") );

        assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
        scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );
        typeref = add_typeref_row( scope, add_string("CompositionType"), add_string("Windows.Foundation.Metadata") );
        typeref_attr = add_typeref_row( scope, add_string("ComposableAttribute"), add_string("Windows.Foundation.Metadata") );

        class = memberref_parent( TABLE_TYPEREF, typeref_attr );
        sig_size = make_member_sig4( typedef_or_ref(TABLE_TYPEREF, typeref_type), typedef_or_ref(TABLE_TYPEREF, typeref), sig );
        attr->md_member = add_memberref_row( class, add_string(".ctor"), add_blob(sig, sig_size) );
    }
}

static void add_composable_attr_step2( const type_t *type )
{
    const attr_t *attr;

    if (type->attrs) LIST_FOR_EACH_ENTRY( attr, type->attrs, const attr_t, entry )
    {
        UINT parent, attr_type, value_size;
        BYTE value[MAX_NAME + sizeof(UINT) * 2 + 6];

        if (attr->type != ATTR_COMPOSABLE) continue;

        parent = has_customattribute( TABLE_TYPEDEF, type->md.def );
        attr_type = customattribute_type( TABLE_MEMBERREF, attr->md_member );
        value_size = make_composable_value( attr->u.pval, value );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }
}

static void add_member_interfaces( type_t *class )
{
    const typeref_list_t *iface_list = type_runtimeclass_get_ifaces( class );
    const typeref_t *iface;

    if (iface_list) LIST_FOR_EACH_ENTRY( iface, iface_list, typeref_t, entry )
    {
        UINT interface, interfaceimpl_ref;
        const statement_t *stmt;

        create_typeref( iface->type );

        interface = typedef_or_ref( TABLE_TYPEREF, iface->type->md.ref );
        interfaceimpl_ref = add_interfaceimpl_row( class->md.def, interface );

        if (is_attr( iface->attrs, ATTR_DEFAULT )) add_default_attr( interfaceimpl_ref );

        /* add properties in reverse order like midlrt */
        STATEMENTS_FOR_EACH_FUNC_REV( stmt, type_iface_get_stmts(iface->type) )
        {
            const var_t *method = stmt->u.var;

            add_property( class, iface->type, method );
        }

        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface->type) )
        {
            const var_t *method = stmt->u.var;

            add_event( class, iface->type, method );
            add_method( class, iface->type, method );
            add_method_impl( class, iface->type, method );
            add_method_contract_attrs( class, iface->type, method->declspec.type );
        }
    }
}

static void add_static_interfaces( type_t *class )
{
    const attr_t *attr;

    if (class->attrs) LIST_FOR_EACH_ENTRY( attr, class->attrs, const attr_t, entry )
    {
        const expr_t *value = attr->u.pval;
        const statement_t *stmt;
        type_t *iface;

        if (attr->type != ATTR_STATIC) continue;

        iface = value->u.var->declspec.type;

        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            const var_t *method = stmt->u.var;

            add_property( class, iface, method );
            add_event( class, iface, method );
            add_method( class, iface, method );
            add_method_contract_attrs( class, iface, method->declspec.type );
        }
    }
}

static void add_activation_interfaces( const type_t *class )
{
    UINT flags = METHOD_ATTR_PUBLIC | METHOD_ATTR_HIDEBYSIG | METHOD_ATTR_SPECIALNAME | METHOD_ATTR_RTSPECIALNAME;
    const attr_t *attr, *contract_attr = get_attr( class->attrs, ATTR_CONTRACT );

    if (class->attrs) LIST_FOR_EACH_ENTRY_REV( attr, class->attrs, const attr_t, entry )
    {
        UINT methoddef, parent, attr_type, value_size, paramlist = 0, sig_size;
        BYTE value[MAX_NAME + sizeof(UINT) + 5], sig[256];
        const expr_t *activatable = attr->u.pval;
        const type_t *iface = NULL;
        const var_t *method = NULL, *arg;
        const statement_t *stmt;

        if (attr->type != ATTR_ACTIVATABLE) continue;

        /* interface is optional */
        if (activatable->type == EXPR_MEMBER) iface = activatable->u.var->declspec.type;

        if (iface) STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            UINT seq = 1, row;

            method = stmt->u.var;

            LIST_FOR_EACH_ENTRY( arg, type_function_get_args(method->declspec.type), var_t, entry )
            {
                if (is_retval( arg )) continue;
                row = add_param_row( get_param_attrs(arg), seq++, add_string(arg->name) );
                if (!paramlist) paramlist = row;
            }
            break;
        }

        sig_size = make_activation_sig( method, sig );
        methoddef = add_methoddef_row( METHOD_IMPL_RUNTIME, flags, add_string(".ctor"), add_blob(sig, sig_size), paramlist );

        parent = has_customattribute( TABLE_METHODDEF, methoddef );
        attr_type = customattribute_type( TABLE_MEMBERREF, contract_attr->md_member );
        value_size = make_contract_value( contract_attr, value );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }
}

static void add_composition_interfaces( const type_t *class )
{
    UINT flags = METHOD_ATTR_FAMILY | METHOD_ATTR_HIDEBYSIG | METHOD_ATTR_SPECIALNAME | METHOD_ATTR_RTSPECIALNAME;
    const attr_t *attr, *contract_attr = get_attr( class->attrs, ATTR_CONTRACT );

    if (class->attrs) LIST_FOR_EACH_ENTRY_REV( attr, class->attrs, const attr_t, entry )
    {
        UINT methoddef, parent, attr_type, value_size, paramlist = 0, sig_size;
        BYTE value[MAX_NAME + sizeof(UINT) + 5], sig[256];
        const expr_t *composable = attr->u.pval;
        const var_t *method = NULL, *arg;
        const statement_t *stmt;
        const type_t *iface;

        if (attr->type != ATTR_COMPOSABLE) continue;

        iface = composable->u.var->declspec.type;

        STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
        {
            UINT seq = 1, row, count = 0;

            method = stmt->u.var;

            LIST_FOR_EACH_ENTRY( arg, type_function_get_args(method->declspec.type), var_t, entry ) count++;

            if (count > 3) LIST_FOR_EACH_ENTRY( arg, type_function_get_args(method->declspec.type), var_t, entry )
            {
                if (--count < 3) break; /* omit last 3 standard composition args */
                row = add_param_row( get_param_attrs(arg), seq++, add_string(arg->name) );
                if (!paramlist) paramlist = row;
            }
            break;
        }

        sig_size = make_composition_sig( method, sig );
        methoddef = add_methoddef_row( METHOD_IMPL_RUNTIME, flags, add_string(".ctor"), add_blob(sig, sig_size), paramlist );

        parent = has_customattribute( TABLE_METHODDEF, methoddef );
        attr_type = customattribute_type( TABLE_MEMBERREF, contract_attr->md_member );
        value_size = make_contract_value( contract_attr, value );
        add_customattribute_row( parent, attr_type, add_blob(value, value_size) );
    }
}

static void add_constructor_overload( const type_t *type )
{
    static const BYTE sig_default[] = { SIG_TYPE_HASTHIS, 0, ELEMENT_TYPE_VOID };
    static const BYTE sig_overload[] = { SIG_TYPE_HASTHIS, 1, ELEMENT_TYPE_VOID, ELEMENT_TYPE_STRING };
    UINT name, namespace;
    const attr_t *attr;

    if (type->attrs) LIST_FOR_EACH_ENTRY_REV( attr, type->attrs, const attr_t, entry )
    {
        const expr_t *value = attr->u.pval;

        if (attr->type == ATTR_COMPOSABLE || (attr->type == ATTR_ACTIVATABLE && value->type == EXPR_MEMBER))
        {
            UINT assemblyref, scope, typeref_default, typeref_overload, class;

            assemblyref = add_assemblyref_row( 0x200, 0, add_string("Windows.Foundation") );
            scope = resolution_scope( TABLE_ASSEMBLYREF, assemblyref );

            name = add_string( "DefaultOverloadAttribute" );
            namespace = add_string( "Windows.Foundation.Metadata" );
            typeref_default = add_typeref_row( scope, name, namespace );

            class = memberref_parent( TABLE_TYPEREF, typeref_default );
            add_memberref_row( class, add_string(".ctor"), add_blob(sig_default, sizeof(sig_default)) );

            name = add_string( "OverloadAttribute" );
            typeref_overload = add_typeref_row( scope, name, namespace );

            class = memberref_parent( TABLE_TYPEREF, typeref_overload );
            add_memberref_row( class, add_string(".ctor"), add_blob(sig_overload, sizeof(sig_overload)) );
            break;
        }
    }
}

static void add_runtimeclass_type_step2( type_t *type )
{
    const typeref_list_t *iface_list = type_runtimeclass_get_ifaces( type );
    UINT flags;

    if (type->md.def) return;

    add_constructor_overload( type );

    flags = TYPE_ATTR_PUBLIC | TYPE_ATTR_UNKNOWN;
    if (!is_attr( type->attrs, ATTR_COMPOSABLE )) flags |= TYPE_ATTR_SEALED;
    if (!iface_list) flags |= TYPE_ATTR_ABSTRACT;

    type->md.def = add_typedef_row( flags, type->md.name, type->md.namespace, type->md.extends, 0, 0 );

    /* add contract first so activation/composition constructors can inherit it */
    add_contract_attr_step1( type );

    add_activation_interfaces( type );
    add_composition_interfaces( type );
    add_member_interfaces( type );
    add_static_interfaces( type );

    add_composable_attr_step1( type );
    add_static_attr_step1( type );
    add_activatable_attr_step1( type );
    add_threading_attr_step1( type );
    add_marshalingbehavior_attr_step1( type );

    add_contract_attr_step2( type );
    add_composable_attr_step2( type );
    add_static_attr_step2( type );
    add_activatable_attr_step2( type );
    add_threading_attr_step2( type );
    add_marshalingbehavior_attr_step2( type );
}

static void add_delegate_type_step1( type_t *type )
{
    const type_t *iface = type_delegate_get_iface( type );
    UINT scope, typeref;
    const statement_t *stmt;

    scope = resolution_scope( TABLE_ASSEMBLYREF, MSCORLIB_ROW );
    typeref = add_typeref_row( scope, add_string("MulticastDelegate"), add_string("System") );
    type->md.extends = typedef_or_ref( TABLE_TYPEREF, typeref );

    create_typeref( type );

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        add_method_params_step1( stmt->u.var );
    }

    add_version_attr_step1( type );
    add_contract_attr_step1( type );
    add_uuid_attr_step1( type );
}

static void add_delegate_type_step2( type_t *type )
{
    static const BYTE sig_ctor[] = { SIG_TYPE_HASTHIS, 2, ELEMENT_TYPE_VOID, ELEMENT_TYPE_OBJECT, ELEMENT_TYPE_I };
    UINT methoddef, flags, paramlist;
    const type_t *iface = type_delegate_get_iface( type );
    const statement_t *stmt;

    flags = METHOD_ATTR_RTSPECIALNAME | METHOD_ATTR_SPECIALNAME | METHOD_ATTR_HIDEBYSIG | METHOD_ATTR_PRIVATE;
    methoddef = add_methoddef_row( METHOD_IMPL_RUNTIME, flags, add_string(".ctor"),
                                   add_blob(sig_ctor, sizeof(sig_ctor)), 0 );

    add_param_row( 0, 1, add_string("object") );
    add_param_row( 0, 2, add_string("method") );

    flags = METHOD_ATTR_SPECIALNAME | METHOD_ATTR_HIDEBYSIG | METHOD_ATTR_PUBLIC | METHOD_ATTR_VIRTUAL |
            METHOD_ATTR_NEWSLOT;

    STATEMENTS_FOR_EACH_FUNC( stmt, type_iface_get_stmts(iface) )
    {
        const var_t *method = stmt->u.var;
        UINT sig_size;
        BYTE sig[256];

        sig_size = make_method_sig( method, sig, FALSE );
        paramlist = add_method_params_step2( type_function_get_args(method->declspec.type) );

        add_methoddef_row( METHOD_IMPL_RUNTIME, flags, add_string("Invoke"), add_blob(sig, sig_size), paramlist );
        break;
    }
    type->md.def = add_typedef_row( TYPE_ATTR_PUBLIC | TYPE_ATTR_SEALED | TYPE_ATTR_UNKNOWN, type->md.name,
                                    type->md.namespace, type->md.extends, 0, methoddef );

    add_uuid_attr_step2( type );
    add_version_attr_step2( type );
    add_contract_attr_step2( type );
}

static void build_tables( const statement_list_t *stmt_list )
{
    const statement_t *stmt;

    /* Adding a type involves two passes: the first creates various references and the second
       uses those references to create the remaining rows. */

    LIST_FOR_EACH_ENTRY( stmt, stmt_list, const statement_t, entry )
    {
        type_t *type = stmt->u.type;

        if (stmt->type == STMT_IMPORT) append_assembly_import( stmt->u.str );

        if (stmt->type != STMT_TYPE) continue;

        switch (type->type_type)
        {
        case TYPE_ENUM:
            add_enum_type_step1( type );
            break;
        case TYPE_STRUCT:
            add_struct_type_step1( type );
            break;
        case TYPE_INTERFACE:
            if (type->signature && !strncmp( type->signature, "pinterface", 10 ))
            {
                /* fprintf( stderr, "Ignoring supported %s parameterized interface.\n", type->name ); */
                break;
            }
            add_interface_type_step1( type );
            break;
        case TYPE_APICONTRACT:
            add_apicontract_type_step1( type );
            break;
        case TYPE_RUNTIMECLASS:
            add_runtimeclass_type_step1( type );
            break;
        case TYPE_DELEGATE:
            add_delegate_type_step1( type );
            break;
        default:
            fprintf( stderr, "Unhandled type %u name '%s'.\n", type->type_type, type->name );
            break;
        }
    }

    LIST_FOR_EACH_ENTRY( stmt, stmt_list, const statement_t, entry )
    {
        type_t *type = stmt->u.type;

        if (stmt->type != STMT_TYPE) continue;

        switch (type->type_type)
        {
        case TYPE_ENUM:
            add_enum_type_step2( type );
            break;
        case TYPE_STRUCT:
            add_struct_type_step2( type );
            break;
        case TYPE_INTERFACE:
            if (type->signature && !strncmp( type->signature, "pinterface", 10 ))
            {
                /* fprintf( stderr, "Ignoring supported %s parameterized interface.\n", type->name ); */
                break;
            }
            add_interface_type_step2( type );
            break;
        case TYPE_APICONTRACT:
            add_apicontract_type_step2( type );
            break;
        case TYPE_RUNTIMECLASS:
            add_runtimeclass_type_step2( type );
            break;
        case TYPE_DELEGATE:
            add_delegate_type_step2( type );
            break;
        default:
            break;
        }
    }
}

static void build_table_stream( const statement_list_t *stmts )
{
    static const GUID guid = { 0x9ddc04c6, 0x04ca, 0x04cc, { 0x52, 0x85, 0x4b, 0x50, 0xb2, 0x60, 0x1d, 0xa8 } };
    static const BYTE token[] = { 0xb7, 0x7a, 0x5c, 0x56, 0x19, 0x34, 0xe0, 0x89 };
    static const USHORT space = 0x20;
    char *ptr;
    UINT i;

    add_string( "" );
    add_userstring( NULL, 0 );
    add_userstring( &space, sizeof(space) );
    add_blob( NULL, 0 );

    assembly_name = xstrdup( metadata_name );
    if ((ptr = strrchr( assembly_name, '.' ))) *ptr = 0;

    add_typedef_row( 0, add_string("<Module>"), 0, 0, 1, 1 );
    add_assembly_row( add_string(assembly_name) );
    add_module_row( add_string(metadata_name), add_guid(&guid) );
    add_assemblyref_row( 0, add_blob(token, sizeof(token)), add_string("mscorlib") );

    build_tables( stmts );

    for (i = 0; i < TABLE_MAX; i++) if (tables[i].count) tables_header.valid |= (1ull << i);

    if (strings.offset >> 16) tables_header.heap_sizes |= LARGE_STRING_HEAP;
    if (guids.offset >> 16) tables_header.heap_sizes |= LARGE_GUID_HEAP;
    if (blobs.offset >> 16) tables_header.heap_sizes |= LARGE_BLOB_HEAP;

    add_bytes( &tables_disk, (const BYTE *)&tables_header, sizeof(tables_header) );

    for (i = 0; i < TABLE_MAX; i++)
        if (tables[i].count) add_bytes( &tables_disk, (const BYTE *)&tables[i].count, sizeof(tables[i].count) );

    serialize_module_table();
    serialize_typeref_table();
    serialize_typedef_table();
    serialize_field_table();
    serialize_methoddef_table();
    serialize_param_table();
    serialize_interfaceimpl_table();
    serialize_memberref_table();
    serialize_constant_table();
    serialize_customattribute_table();
    serialize_eventmap_table();
    serialize_event_table();
    serialize_propertymap_table();
    serialize_property_table();
    serialize_methodsemantics_table();
    serialize_methodimpl_table();
    serialize_assembly_table();
    serialize_assemblyref_table();
}

static void build_streams( const statement_list_t *stmts )
{
    static const BYTE pad[4];
    UINT i, len, offset = sizeof(metadata_header);

    build_table_stream( stmts );

    len = (tables_disk.offset + 3) & ~3;
    add_bytes( &tables_disk, pad, len - tables_disk.offset );

    streams[WINMD_STREAM_TABLE].data_size = tables_disk.offset;
    streams[WINMD_STREAM_TABLE].data = tables_disk.ptr;

    len = (strings.offset + 3) & ~3;
    add_bytes( &strings, pad, len - strings.offset );

    streams[WINMD_STREAM_STRING].data_size = strings.offset;
    streams[WINMD_STREAM_STRING].data = strings.ptr;

    len = (userstrings.offset + 3) & ~3;
    add_bytes( &userstrings, pad, len - userstrings.offset );

    streams[WINMD_STREAM_USERSTRING].data_size = userstrings.offset;
    streams[WINMD_STREAM_USERSTRING].data = userstrings.ptr;

    len = (blobs.offset + 3) & ~3;
    add_bytes( &blobs, pad, len - blobs.offset );

    streams[WINMD_STREAM_BLOB].data_size = blobs.offset;
    streams[WINMD_STREAM_BLOB].data = blobs.ptr;

    streams[WINMD_STREAM_GUID].data_size = guids.offset;
    streams[WINMD_STREAM_GUID].data = guids.ptr;

    for (i = 0; i < WINMD_STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        offset += streams[i].header_size;
    }
    for (i = 0; i < WINMD_STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        streams[i].data_offset = offset;
        offset += streams[i].data_size;
    }
}

static void write_streams( void )
{
    UINT i;
    for (i = 0; i < WINMD_STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        put_data( streams[i].data, streams[i].data_size );
    }
}

void write_metadata( const statement_list_t *stmts )
{
    static const BYTE pad[FILE_ALIGNMENT];
    UINT image_size, file_size, i;

    if (!do_metadata || !winrt_mode) return;

    build_streams( stmts );

    image_size = FILE_ALIGNMENT + sizeof(cor_header) + 8 + sizeof(metadata_header);
    for (i = 0; i < WINMD_STREAM_MAX; i++) image_size += streams[i].header_size + streams[i].data_size;

    init_output_buffer();

    write_headers( image_size );
    write_streams( );

    file_size = (image_size + FILE_ALIGNMENT - 1) & ~(FILE_ALIGNMENT - 1);
    put_data( pad, file_size - image_size );

    flush_output_buffer( metadata_name );
}
