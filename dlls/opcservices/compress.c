/*
 * Copyright 2018 Nikolay Sivov for CodeWeavers
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

#include <stdarg.h>
#include <stdint.h>
#include <zlib.h>

#include "windef.h"
#include "winternl.h"
#include "msopc.h"

#include "opc_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msopc);

#pragma pack(push,2)
struct local_file_header
{
    DWORD signature;
    WORD version;
    WORD flags;
    WORD method;
    DWORD mtime;
    DWORD crc32;
    DWORD compressed_size;
    DWORD uncompressed_size;
    WORD name_length;
    WORD extra_length;
};

struct zip32_data_descriptor
{
    uint32_t signature;
    uint32_t crc32;
    uint32_t compressed_size;
    uint32_t uncompressed_size;
};

struct central_directory_header
{
    DWORD signature;
    WORD version;
    WORD min_version;
    WORD flags;
    WORD method;
    DWORD mtime;
    DWORD crc32;
    DWORD compressed_size;
    DWORD uncompressed_size;
    WORD name_length;
    WORD extra_length;
    WORD comment_length;
    WORD diskid;
    WORD internal_attributes;
    DWORD external_attributes;
    DWORD local_file_offset;
};

struct zip32_end_of_central_directory
{
    uint32_t signature;
    uint16_t diskid;
    uint16_t firstdisk;
    uint16_t records_num;
    uint16_t records_total;
    uint32_t directory_size;
    uint32_t directory_offset;
    uint16_t comment_length;
};
#pragma pack(pop)

#define CENTRAL_DIR_SIGNATURE 0x02014b50
#define LOCAL_HEADER_SIGNATURE 0x04034b50
#define DIRECTORY_END_SIGNATURE 0x06054b50
#define DATA_DESCRIPTOR_SIGNATURE 0x08074b50
#define ZIP32_VERSION 20

enum entry_flags
{
    USE_DATA_DESCRIPTOR = 0x8,
};

struct zip_archive
{
    struct central_directory_header **files;
    size_t file_count;
    size_t file_size;

    DWORD mtime;
    IStream *output;
    DWORD position;
    HRESULT write_result;

    unsigned char input_buffer[0x8000];
    unsigned char output_buffer[0x8000];
};

HRESULT compress_create_archive(IStream *output, struct zip_archive **out)
{
    struct zip_archive *archive;
    WORD date, time;
    FILETIME ft;

    if (!(archive = malloc(sizeof(*archive))))
        return E_OUTOFMEMORY;

    archive->files = NULL;
    archive->file_size = 0;
    archive->file_count = 0;
    archive->write_result = S_OK;

    archive->output = output;
    IStream_AddRef(archive->output);
    archive->position = 0;

    GetSystemTimeAsFileTime(&ft);
    FileTimeToDosDateTime(&ft, &date, &time);
    archive->mtime = date << 16 | time;

    *out = archive;

    return S_OK;
}

static void compress_write(struct zip_archive *archive, void *data, ULONG size)
{
    ULONG written;

    archive->write_result = IStream_Write(archive->output, data, size, &written);
    if (written != size)
        archive->write_result = E_FAIL;
    else
        archive->position += written;

    if (FAILED(archive->write_result))
        WARN("Failed to write output %p, size %lu, written %lu, hr %#lx.\n",
                data, size, written, archive->write_result);
}

void compress_finalize_archive(struct zip_archive *archive)
{
    struct zip32_end_of_central_directory dir_end = { 0 };
    size_t i;

    dir_end.directory_offset = archive->position;
    dir_end.records_num = archive->file_count;
    dir_end.records_total = archive->file_count;

    /* Directory entries */
    for (i = 0; i < archive->file_count; ++i)
    {
        compress_write(archive, archive->files[i], sizeof(*archive->files[i]));
        compress_write(archive, archive->files[i] + 1, archive->files[i]->name_length);
        dir_end.directory_size += archive->files[i]->name_length + sizeof(*archive->files[i]);
    }

    /* End record */
    dir_end.signature = DIRECTORY_END_SIGNATURE;
    compress_write(archive, &dir_end, sizeof(dir_end));

    IStream_Release(archive->output);

    for (i = 0; i < archive->file_count; i++)
        free(archive->files[i]);
    free(archive->files);
    free(archive);
}

static void *zalloc(void *opaque, unsigned int items, unsigned int size)
{
    return malloc(items * size);
}

static void zfree(void *opaque, void *ptr)
{
    free(ptr);
}

static void compress_write_content(struct zip_archive *archive, IStream *content,
        OPC_COMPRESSION_OPTIONS options, struct zip32_data_descriptor *data_desc)
{
    int level, flush;
    z_stream z_str;
    LARGE_INTEGER move;
    ULONG num_read;
    HRESULT hr;
    int init_ret;

    data_desc->crc32 = RtlComputeCrc32(0, NULL, 0);
    move.QuadPart = 0;
    IStream_Seek(content, move, STREAM_SEEK_SET, NULL);

    switch (options)
    {
    case OPC_COMPRESSION_NONE:
        level = Z_NO_COMPRESSION;
        break;
    case OPC_COMPRESSION_NORMAL:
        level = Z_DEFAULT_COMPRESSION;
        break;
    case OPC_COMPRESSION_MAXIMUM:
        level = Z_BEST_COMPRESSION;
        break;
    case OPC_COMPRESSION_FAST:
        level = 2;
        break;
    case OPC_COMPRESSION_SUPERFAST:
        level = Z_BEST_SPEED;
        break;
    default:
        WARN("Unsupported compression options %d.\n", options);
        level = Z_DEFAULT_COMPRESSION;
    }

    memset(&z_str, 0, sizeof(z_str));
    z_str.zalloc = zalloc;
    z_str.zfree = zfree;
    if ((init_ret = deflateInit2(&z_str, level, Z_DEFLATED, -MAX_WBITS, MAX_MEM_LEVEL, Z_DEFAULT_STRATEGY)) != Z_OK)
        WARN("Failed to allocate memory in deflateInit2, ret %d.\n", init_ret);

    do
    {
        int ret;

        if (FAILED(hr = IStream_Read(content, archive->input_buffer, sizeof(archive->input_buffer), &num_read)))
        {
            archive->write_result = hr;
            break;
        }

        z_str.avail_in = num_read;
        z_str.next_in = archive->input_buffer;
        data_desc->crc32 = RtlComputeCrc32(data_desc->crc32, archive->input_buffer, num_read);

        flush = sizeof(archive->input_buffer) > num_read ? Z_FINISH : Z_NO_FLUSH;

        do
        {
            ULONG have;

            z_str.avail_out = sizeof(archive->output_buffer);
            z_str.next_out = archive->output_buffer;

            if ((ret = deflate(&z_str, flush)) < 0)
                WARN("Failed to deflate, ret %d.\n", ret);
            have = sizeof(archive->output_buffer) - z_str.avail_out;
            compress_write(archive, archive->output_buffer, have);
        } while (z_str.avail_out == 0);
    } while (flush != Z_FINISH);

    deflateEnd(&z_str);

    data_desc->compressed_size = z_str.total_out;
    data_desc->uncompressed_size = z_str.total_in;
}

HRESULT compress_add_file(struct zip_archive *archive, const WCHAR *path,
        IStream *content, OPC_COMPRESSION_OPTIONS options)
{
    struct central_directory_header *entry;
    struct local_file_header local_header;
    struct zip32_data_descriptor data_desc;
    DWORD local_header_pos;
    char *name;
    DWORD len;

    len = WideCharToMultiByte(CP_ACP, 0, path, -1, NULL, 0, NULL, NULL);
    if (!(name = malloc(len)))
        return E_OUTOFMEMORY;
    WideCharToMultiByte(CP_ACP, 0, path, -1, name, len, NULL, NULL);

    /* Local header */
    local_header.signature = LOCAL_HEADER_SIGNATURE;
    local_header.version = ZIP32_VERSION;
    local_header.flags = USE_DATA_DESCRIPTOR;
    local_header.method = 8; /* Z_DEFLATED */
    local_header.mtime = archive->mtime;
    local_header.crc32 = 0;
    local_header.compressed_size = 0;
    local_header.uncompressed_size = 0;
    local_header.name_length = len - 1;
    local_header.extra_length = 0;

    local_header_pos = archive->position;

    compress_write(archive, &local_header, sizeof(local_header));
    compress_write(archive, name, local_header.name_length);

    /* Content */
    compress_write_content(archive, content, options, &data_desc);

    /* Data descriptor */
    data_desc.signature = DATA_DESCRIPTOR_SIGNATURE;
    compress_write(archive, &data_desc, sizeof(data_desc));

    if (FAILED(archive->write_result))
        return archive->write_result;

    /* Set directory entry */
    if (!(entry = calloc(1, sizeof(*entry) + local_header.name_length)))
    {
        free(name);
        return E_OUTOFMEMORY;
    }

    entry->signature = CENTRAL_DIR_SIGNATURE;
    entry->version = local_header.version;
    entry->min_version = local_header.version;
    entry->flags = local_header.flags;
    entry->method = local_header.method;
    entry->mtime = local_header.mtime;
    entry->crc32 = data_desc.crc32;
    entry->compressed_size = data_desc.compressed_size;
    entry->uncompressed_size = data_desc.uncompressed_size;
    entry->name_length = local_header.name_length;
    entry->local_file_offset = local_header_pos;
    memcpy(entry + 1, name, entry->name_length);
    free(name);

    if (!opc_array_reserve((void **)&archive->files, &archive->file_size, archive->file_count + 1,
            sizeof(*archive->files)))
    {
        free(entry);
        return E_OUTOFMEMORY;
    }

    archive->files[archive->file_count++] = entry;

    return S_OK;
}
