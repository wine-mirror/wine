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

#pragma pack(pop)

struct metadata_stream
{
    UINT32 size;
    const BYTE *start;
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
        offset + offsetof(struct metadata_hdr, version[md_hdr->length]) + sizeof(UINT16) > assembly->size)
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

    return S_OK;
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
    if (assembly->map) UnmapViewOfFile(assembly->map);
    CloseHandle(assembly->map);
    CloseHandle(assembly->file);
    free(assembly);
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
