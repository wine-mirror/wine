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

#include "windef.h"
#include "winnt.h"

#include "widl.h"
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

static struct buffer
{
    UINT  offset;        /* write position */
    UINT  allocated;     /* allocated size in bytes */
    UINT  count;         /* number of entries written */
    BYTE *ptr;
} strings, strings_idx, userstrings, userstrings_idx;

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

static void add_bytes( struct buffer *buf, const BYTE *data, UINT size )
{
    grow_buffer( buf, size );
    memcpy( buf->ptr + buf->offset, data, size );
    buf->offset += size;
}

static void build_table_stream( const statement_list_t *stmts )
{
    static const USHORT space = 0x20;

    add_string( "" );
    add_userstring( NULL, 0 );
    add_userstring( &space, sizeof(space) );
}

static void build_streams( const statement_list_t *stmts )
{
    static const BYTE pad[4];
    UINT i, len, offset = sizeof(metadata_header);

    build_table_stream( stmts );

    len = (strings.offset + 3) & ~3;
    add_bytes( &strings, pad, len - strings.offset );

    streams[STREAM_STRING].data_size = strings.offset;
    streams[STREAM_STRING].data = strings.ptr;

    len = (userstrings.offset + 3) & ~3;
    add_bytes( &userstrings, pad, len - userstrings.offset );

    streams[STREAM_USERSTRING].data_size = userstrings.offset;
    streams[STREAM_USERSTRING].data = userstrings.ptr;

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
