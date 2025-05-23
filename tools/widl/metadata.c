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
    STREAM_TABLE,
    STREAM_STRING,
    STREAM_USERSTRING,
    STREAM_GUID,
    STREAM_BLOB,
    STREAM_MAX
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

    for (i = 0; i < STREAM_MAX; i++)
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
    for (i = 0; i < STREAM_MAX; i++)
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
    BOOL sort = (table != TABLE_PARAM && table != TABLE_FIELD);

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

static void serialize_ushort( USHORT value )
{
    add_bytes( &tables_disk, (const BYTE *)&value, sizeof(value) );
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

struct module_row
{
    USHORT generation;
    UINT   name;
    UINT   mvid;
    UINT   encid;
    UINT   encbaseid;
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

struct assembly_row
{
    UINT   hashalgid;
    USHORT majorversion;
    USHORT minorversion;
    USHORT buildnumber;
    USHORT revisionnumber;
    UINT   flags;
    UINT   publickey;
    UINT   name;
    UINT   culture;
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

enum
{
    LARGE_STRING_HEAP = 0x01,
    LARGE_GUID_HEAP   = 0x02,
    LARGE_BLOB_HEAP   = 0x04
};

static char *assembly_name;

static void build_table_stream( const statement_list_t *stmts )
{
    static const GUID guid = { 0x9ddc04c6, 0x04ca, 0x04cc, { 0x52, 0x85, 0x4b, 0x50, 0xb2, 0x60, 0x1d, 0xa8 } };
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

    for (i = 0; i < TABLE_MAX; i++) if (tables[i].count) tables_header.valid |= (1ull << i);

    if (strings.offset >> 16) tables_header.heap_sizes |= LARGE_STRING_HEAP;
    if (guids.offset >> 16) tables_header.heap_sizes |= LARGE_GUID_HEAP;
    if (blobs.offset >> 16) tables_header.heap_sizes |= LARGE_BLOB_HEAP;

    add_bytes( &tables_disk, (const BYTE *)&tables_header, sizeof(tables_header) );

    for (i = 0; i < TABLE_MAX; i++)
        if (tables[i].count) add_bytes( &tables_disk, (const BYTE *)&tables[i].count, sizeof(tables[i].count) );

    serialize_module_table();
    serialize_typedef_table();
    serialize_assembly_table();
}

static void build_streams( const statement_list_t *stmts )
{
    static const BYTE pad[4];
    UINT i, len, offset = sizeof(metadata_header);

    build_table_stream( stmts );

    len = (tables_disk.offset + 3) & ~3;
    add_bytes( &tables_disk, pad, len - tables_disk.offset );

    streams[STREAM_TABLE].data_size = tables_disk.offset;
    streams[STREAM_TABLE].data = tables_disk.ptr;

    len = (strings.offset + 3) & ~3;
    add_bytes( &strings, pad, len - strings.offset );

    streams[STREAM_STRING].data_size = strings.offset;
    streams[STREAM_STRING].data = strings.ptr;

    len = (userstrings.offset + 3) & ~3;
    add_bytes( &userstrings, pad, len - userstrings.offset );

    streams[STREAM_USERSTRING].data_size = userstrings.offset;
    streams[STREAM_USERSTRING].data = userstrings.ptr;

    len = (blobs.offset + 3) & ~3;
    add_bytes( &blobs, pad, len - blobs.offset );

    streams[STREAM_BLOB].data_size = blobs.offset;
    streams[STREAM_BLOB].data = blobs.ptr;

    streams[STREAM_GUID].data_size = guids.offset;
    streams[STREAM_GUID].data = guids.ptr;

    for (i = 0; i < STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        offset += streams[i].header_size;
    }
    for (i = 0; i < STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        streams[i].data_offset = offset;
        offset += streams[i].data_size;
    }
}

static void write_streams( void )
{
    UINT i;
    for (i = 0; i < STREAM_MAX; i++)
    {
        if (!streams[i].data_size) continue;
        put_data( streams[i].data, streams[i].data_size );
    }
}

void write_metadata( const statement_list_t *stmts )
{
    static const BYTE pad[FILE_ALIGNMENT];
    UINT image_size, file_size, i;

    if (!do_metadata) return;

    build_streams( stmts );

    image_size = FILE_ALIGNMENT + sizeof(cor_header) + 8 + sizeof(metadata_header);
    for (i = 0; i < STREAM_MAX; i++) image_size += streams[i].header_size + streams[i].data_size;

    init_output_buffer();

    write_headers( image_size );
    write_streams( );

    file_size = (image_size + FILE_ALIGNMENT - 1) & ~(FILE_ALIGNMENT - 1);
    put_data( pad, file_size - image_size );

    flush_output_buffer( metadata_name );
}
