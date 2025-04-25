/*
 * Copyright 2023 Eric Pouech for CodeWeavers
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

#include <assert.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windows.h"
#include "dbghelp.h"
#include "wine/test.h"
#include "winternl.h"
#include "winnt.h"
#include "wine/mscvpdb.h"

static const IMAGE_DOS_HEADER dos_header =
{
    .e_magic = IMAGE_DOS_SIGNATURE,
    .e_lfanew = sizeof(dos_header),
};

static const GUID null_guid;
static const GUID guid1 = {0x1234abcd, 0x3456, 0x5678, {0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x0a, 0x0b}};
static const GUID guid2 = {0x1234abcd, 0x3456, 0x5678, {0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x0a, 0x0c}};

#define ALIGN(v, a)          (((v - 1) / (a) + 1) * (a))
#define NUM_OF(x, a)         (((x) + (a) - 1) / (a))

#define check_write_file(a, b, c) _check_write_file(__LINE__, (a), (b), (c))
static void _check_write_file(unsigned line, HANDLE file, const void* ptr, size_t len)
{
    DWORD written;
    BOOL ret;

    ret = WriteFile(file, ptr, len, &written, NULL);
    ok_(__FILE__, line)(ret, "WriteFile error %ld\n", GetLastError());
    ok_(__FILE__, line)(written == len, "Unexpected written len %lu (%Iu)\n", written, len);
}

/* ==============================================
 *       Helpers for generating PE files
 * ==============================================
 */

static void init_headers64(IMAGE_NT_HEADERS64* header, DWORD timestamp, DWORD size_of_image, DWORD charac)
{
    unsigned int j;

    header->Signature = IMAGE_NT_SIGNATURE;
    header->FileHeader.Machine = IMAGE_FILE_MACHINE_AMD64;
    header->FileHeader.NumberOfSections = 2;
    header->FileHeader.TimeDateStamp = timestamp;
    header->FileHeader.PointerToSymbolTable = 0;
    header->FileHeader.NumberOfSymbols = 0;
    header->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER64);
    header->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_LARGE_ADDRESS_AWARE | charac;

    header->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    header->OptionalHeader.MajorLinkerVersion = 14;
    header->OptionalHeader.MinorLinkerVersion = 35;
    header->OptionalHeader.SizeOfCode = 0x200;
    header->OptionalHeader.SizeOfInitializedData = 0x200;
    header->OptionalHeader.SizeOfUninitializedData = 0;
    header->OptionalHeader.AddressOfEntryPoint = 0x1000;
    header->OptionalHeader.BaseOfCode = 0x1000;
    header->OptionalHeader.ImageBase = 0x180000000;
    header->OptionalHeader.SectionAlignment = 0x1000;
    header->OptionalHeader.FileAlignment = 0x200;
    header->OptionalHeader.MajorOperatingSystemVersion = 6;
    header->OptionalHeader.MinorOperatingSystemVersion = 0;
    header->OptionalHeader.MajorImageVersion = 0;
    header->OptionalHeader.MinorImageVersion = 0;
    header->OptionalHeader.MajorSubsystemVersion = 6;
    header->OptionalHeader.MinorSubsystemVersion = 0;
    header->OptionalHeader.Win32VersionValue = 0;
    header->OptionalHeader.SizeOfImage = size_of_image;
    header->OptionalHeader.SizeOfHeaders = ALIGN(sizeof(dos_header) + sizeof(IMAGE_NT_HEADERS64), header->OptionalHeader.FileAlignment);
    header->OptionalHeader.CheckSum = 0;
    header->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    header->OptionalHeader.DllCharacteristics = 0x160;
    header->OptionalHeader.SizeOfStackReserve = 0x100000;
    header->OptionalHeader.SizeOfStackCommit = 0x1000;
    header->OptionalHeader.SizeOfHeapReserve = 0x100000;
    header->OptionalHeader.SizeOfHeapCommit = 0x1000;
    header->OptionalHeader.LoaderFlags = 0;
    header->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    for (j = 0; j < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; j++)
        header->OptionalHeader.DataDirectory[j].VirtualAddress = header->OptionalHeader.DataDirectory[j].Size = 0;
}

static void init_headers32(IMAGE_NT_HEADERS32* header, DWORD timestamp, DWORD size_of_image, DWORD charac)
{
    unsigned int j;

    header->Signature = IMAGE_NT_SIGNATURE;
    header->FileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    header->FileHeader.NumberOfSections = 2;
    header->FileHeader.TimeDateStamp = timestamp;
    header->FileHeader.PointerToSymbolTable = 0;
    header->FileHeader.NumberOfSymbols = 0;
    header->FileHeader.SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER32);
    header->FileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL | IMAGE_FILE_LARGE_ADDRESS_AWARE | charac;

    header->OptionalHeader.Magic = IMAGE_NT_OPTIONAL_HDR32_MAGIC;
    header->OptionalHeader.MajorLinkerVersion = 14;
    header->OptionalHeader.MinorLinkerVersion = 35;
    header->OptionalHeader.SizeOfCode = 0x200;
    header->OptionalHeader.SizeOfInitializedData = 0x200;
    header->OptionalHeader.SizeOfUninitializedData = 0;
    header->OptionalHeader.AddressOfEntryPoint = 0x1000;
    header->OptionalHeader.BaseOfCode = 0x1000;
    header->OptionalHeader.BaseOfData = 0x2000;
    header->OptionalHeader.ImageBase = 0x18000000;
    header->OptionalHeader.SectionAlignment = 0x1000;
    header->OptionalHeader.FileAlignment = 0x200;
    header->OptionalHeader.MajorOperatingSystemVersion = 6;
    header->OptionalHeader.MinorOperatingSystemVersion = 0;
    header->OptionalHeader.MajorImageVersion = 0;
    header->OptionalHeader.MinorImageVersion = 0;
    header->OptionalHeader.MajorSubsystemVersion = 6;
    header->OptionalHeader.MinorSubsystemVersion = 0;
    header->OptionalHeader.Win32VersionValue = 0;
    header->OptionalHeader.SizeOfImage = size_of_image;
    header->OptionalHeader.SizeOfHeaders = ALIGN(sizeof(dos_header) + sizeof(IMAGE_NT_HEADERS32), header->OptionalHeader.FileAlignment);
    header->OptionalHeader.CheckSum = 0;
    header->OptionalHeader.Subsystem = IMAGE_SUBSYSTEM_WINDOWS_GUI;
    header->OptionalHeader.DllCharacteristics = 0x160;
    header->OptionalHeader.SizeOfStackReserve = 0x100000;
    header->OptionalHeader.SizeOfStackCommit = 0x1000;
    header->OptionalHeader.SizeOfHeapReserve = 0x100000;
    header->OptionalHeader.SizeOfHeapCommit = 0x1000;
    header->OptionalHeader.LoaderFlags = 0;
    header->OptionalHeader.NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    for (j = 0; j < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; j++)
        header->OptionalHeader.DataDirectory[j].VirtualAddress = header->OptionalHeader.DataDirectory[j].Size = 0;
}

/* reminder: the hdr*.FileHeader have the same layout between hdr32 and hdr64...
 * only the optional part differs (except magic field).
 */
union nt_header
{
    IMAGE_NT_HEADERS64 nt_header64;
    IMAGE_NT_HEADERS32 nt_header32;
};
static inline IMAGE_FILE_HEADER* file_header(union nt_header* hdr) {return &hdr->nt_header32.FileHeader;}
static inline IMAGE_DATA_DIRECTORY* header_data_dir(union nt_header* hdr, unsigned d)
{
    switch (hdr->nt_header64.OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        return &hdr->nt_header32.OptionalHeader.DataDirectory[d];
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        return &hdr->nt_header64.OptionalHeader.DataDirectory[d];
    default:
        return NULL;
    }
}

/* a blob is IMAGE_DEBUG_DIRECTORY followed by the directory content. */
struct debug_directory_blob
{
    IMAGE_DEBUG_DIRECTORY debug_directory;
    char content[];
};

static BOOL create_test_dll(union nt_header* hdr, unsigned size_hdr,
                            struct debug_directory_blob** blobs, unsigned num_blobs, const WCHAR* dll_name)
{
    IMAGE_SECTION_HEADER section;
    IMAGE_DATA_DIRECTORY* data_dir;
    HANDLE hfile;
    char filler[0x200];
    DWORD where;
    int i;

    hfile = CreateFileW(dll_name, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "Failed to create %ls err %lu\n", dll_name, GetLastError());
    if (hfile == INVALID_HANDLE_VALUE) return FALSE;

    file_header(hdr)->NumberOfSections = 1;

    strcpy((char*)section.Name, ".rdata");
    section.Misc.VirtualSize = 0x200;
    section.VirtualAddress = 0x2000;
    section.SizeOfRawData = 0x200;
    section.PointerToRawData = 0x400;
    section.PointerToRelocations = 0;
    section.PointerToLinenumbers = 0;
    section.NumberOfRelocations = 0;
    section.NumberOfLinenumbers = 0;
    section.Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ;

    data_dir = header_data_dir(hdr, IMAGE_FILE_DEBUG_DIRECTORY);
    ok(data_dir != NULL, "Unexpected case\n");
    if (!data_dir) return FALSE;

    if (blobs && num_blobs)
    {
        where = section.PointerToRawData;
        data_dir->Size = num_blobs * sizeof(IMAGE_DEBUG_DIRECTORY);
        data_dir->VirtualAddress = section.VirtualAddress;
        where += num_blobs * sizeof(IMAGE_DEBUG_DIRECTORY);
        for (i = 0; i < num_blobs; i++)
        {
            blobs[i]->debug_directory.PointerToRawData = where;
            where += blobs[i]->debug_directory.SizeOfData;
        }
        assert(section.SizeOfRawData >= where - section.PointerToRawData);
    }

    check_write_file(hfile, &dos_header, sizeof(dos_header));

    SetFilePointer(hfile, dos_header.e_lfanew, NULL, FILE_BEGIN);
    check_write_file(hfile, hdr, size_hdr);

    check_write_file(hfile, &section, sizeof(section));

    memset(filler, 0, sizeof(filler));

    SetFilePointer(hfile, 0x200, NULL, FILE_BEGIN);
    check_write_file(hfile, filler, 0x200);

    SetFilePointer(hfile, 0x400, NULL, FILE_BEGIN);
    if (blobs && num_blobs)
    {
        for (i = 0; i < num_blobs; i++)
            check_write_file(hfile, &blobs[i]->debug_directory, sizeof(IMAGE_DEBUG_DIRECTORY));
        for (i = 0; i < num_blobs; i++)
            check_write_file(hfile, blobs[i]->content, blobs[i]->debug_directory.SizeOfData);
    }
    check_write_file(hfile, filler,
                     section.PointerToRawData + section.Misc.VirtualSize - SetFilePointer(hfile, 0, NULL, FILE_CURRENT));

    CloseHandle(hfile);

    return TRUE;
}

static struct debug_directory_blob* make_empty_blob(void)
{
    struct debug_directory_blob* blob;

    blob = malloc(offsetof(struct debug_directory_blob, content[0]));

    blob->debug_directory.AddressOfRawData = 0;
    blob->debug_directory.Characteristics = 0;
    blob->debug_directory.MajorVersion = 0;
    blob->debug_directory.MinorVersion = 0;
    blob->debug_directory.PointerToRawData = 0;
    blob->debug_directory.SizeOfData = 0;
    blob->debug_directory.TimeDateStamp = 0;
    blob->debug_directory.Type = IMAGE_DEBUG_TYPE_UNKNOWN;

    return blob;
}

static struct debug_directory_blob* make_pdb_jg_blob(DWORD dd_timestamp, DWORD jg_timestamp, DWORD age, const char* name)
{
    struct debug_directory_blob* blob;
    DWORD size;

    size = ALIGN(16 + strlen(name) + 1, 4);
    blob = malloc(offsetof(struct debug_directory_blob, content[size]));

    blob->debug_directory.AddressOfRawData = 0;
    blob->debug_directory.Characteristics = 0;
    blob->debug_directory.MajorVersion = 0;
    blob->debug_directory.MinorVersion = 0;
    blob->debug_directory.PointerToRawData = 0;
    blob->debug_directory.SizeOfData = size;
    blob->debug_directory.TimeDateStamp = dd_timestamp;
    blob->debug_directory.Type = IMAGE_DEBUG_TYPE_CODEVIEW;

    blob->content[0] = 'N';                       /* signature */
    blob->content[1] = 'B';
    blob->content[2] = '1';
    blob->content[3] = '0';
    *(DWORD*)(blob->content +  4) = 0;            /* file pos */
    *(DWORD*)(blob->content +  8) = jg_timestamp; /* timestamp */
    *(DWORD*)(blob->content + 12) = age;          /* age */
    strcpy(blob->content + 16, name);             /* name */

    return blob;
}

static struct debug_directory_blob* make_pdb_ds_blob(DWORD dd_timestamp, const GUID* guid, DWORD age, const char* name)
{
    struct debug_directory_blob* blob;
    DWORD size;

    size = ALIGN(4 + sizeof(GUID) + 4 + strlen(name) + 1, 4);
    blob = malloc(offsetof(struct debug_directory_blob, content[size]));

    blob->debug_directory.AddressOfRawData = 0;
    blob->debug_directory.Characteristics = 0;
    blob->debug_directory.MajorVersion = 0;
    blob->debug_directory.MinorVersion = 0;
    blob->debug_directory.PointerToRawData = 0;
    blob->debug_directory.SizeOfData = size;
    blob->debug_directory.TimeDateStamp = dd_timestamp;
    blob->debug_directory.Type = IMAGE_DEBUG_TYPE_CODEVIEW;

    blob->content[0] = 'R';                              /* signature */
    blob->content[1] = 'S';
    blob->content[2] = 'D';
    blob->content[3] = 'S';
    memcpy(blob->content + 4, guid, sizeof(*guid));      /* guid */
    *(DWORD*)(blob->content + 4 + sizeof(GUID)) = age;   /* age */
    strcpy(blob->content + 4 + sizeof(GUID) + 4, name);  /* name */

    return blob;
}

static struct debug_directory_blob* make_dbg_blob(DWORD dd_timestamp, const char* name)
{
    struct debug_directory_blob* blob;
    DWORD size;

    size = ALIGN(sizeof(IMAGE_DEBUG_MISC) + strlen(name), 4);
    blob = malloc(offsetof(struct debug_directory_blob, content[size]));

    blob->debug_directory.AddressOfRawData = 0;
    blob->debug_directory.Characteristics = 0;
    blob->debug_directory.MajorVersion = 0;
    blob->debug_directory.MinorVersion = 0;
    blob->debug_directory.PointerToRawData = 0;
    blob->debug_directory.SizeOfData = size;
    blob->debug_directory.TimeDateStamp = dd_timestamp;
    blob->debug_directory.Type = IMAGE_DEBUG_TYPE_MISC;

    *(DWORD*)blob->content = IMAGE_DEBUG_MISC_EXENAME; /* DataType */
    *(DWORD*)(blob->content + 4) = size;               /* Length */
    blob->content[8] = 0;                              /* Unicode */
    blob->content[9] = 0;                              /* Reserved */
    blob->content[10] = 0;                             /* Reserved */
    blob->content[11] = 0;                             /* Reserved */
    strcpy(blob->content + 12, name);                  /* Data */

    return blob;
}

/* ==============================================
 *      Helpers for generating PDB files
 * ==============================================
 */

struct pdb_stream
{
    unsigned int size;
    unsigned int num_buffers;
    struct
    {
        const void*    ptr;
        unsigned int   size;
        unsigned short been_aligned;
        unsigned short padding;
    } buffers[16];
};

struct pdb_file
{
    unsigned int        block_size;
    unsigned short      num_streams;
    struct pdb_stream   streams[16];
};

static void pdb_append_to_stream(struct pdb_stream* stream, const void* buffer, unsigned int len)
{
    assert(stream->num_buffers < ARRAYSIZE(stream->buffers));
    stream->size += len;
    stream->buffers[stream->num_buffers].ptr = buffer;
    stream->buffers[stream->num_buffers].size = len;
    stream->buffers[stream->num_buffers].been_aligned = 0;
    stream->buffers[stream->num_buffers].padding = 0;
    stream->num_buffers++;
}

static struct pdb_stream* pdb_add_stream(struct pdb_file* pdb, unsigned short* strno, const void* buffer, unsigned int len)
{
    struct pdb_stream* stream = &pdb->streams[pdb->num_streams];

    assert(pdb->num_streams < ARRAYSIZE(pdb->streams));
    stream->size = 0;
    stream->num_buffers = 0;

    if (buffer && len)
        pdb_append_to_stream(stream, buffer, len);
    if (strno) *strno = pdb->num_streams;
    pdb->num_streams++;
    return stream;
}

static unsigned int pdb_align_stream(struct pdb_stream* stream, unsigned int align)
{
    assert(stream->num_buffers && !stream->buffers[stream->num_buffers - 1].been_aligned);
    stream->buffers[stream->num_buffers - 1].been_aligned = 1;
    stream->buffers[stream->num_buffers - 1].padding = ALIGN(stream->size, align) - stream->size;
    return stream->size = ALIGN(stream->size, align);
}

static void pdb_init(struct pdb_file* pdb)
{
    pdb->block_size = 0x400;
    pdb->num_streams = 0;
}

static void pdb_write(HANDLE hfile, struct pdb_file* pdb)
{
    DWORD toc[32], dummy, other;
    unsigned int i, j;
    unsigned num_blocks, toc_where;
    BOOL ret;
    struct PDB_DS_HEADER ds_header =
    {
        "Microsoft C/C++ MSF 7.00\r\n\032DS\0\0",
        pdb->block_size, 1, 4 /* hdr, free list 1 & 2, toc block list */, 0, 0, 3
    };

    /* we use always the same layout:
     * block   0: header
     * block   1: free list (active)   = always filled with 0
     * block   2: free list (inactive) = always filled with 0
     * block   3: block numbers of toc
     * block   4: toc for streams (FIXME only 1 block for toc)
     * block   5: stream 1 (first block)
     * block   x: stream 1 (last block)
     * block x+1: stream 2 (first block)
     * block   y: stream 2 (last block)
     * ...
     * block   z: stream n (last block)
     * (NB: entry is omitted when stream's size is 0)
     * pad to align to block size
     */
    toc[0] = pdb->num_streams; /* number of streams */

    num_blocks = 5; /* header, free lists 1 & 2, block number of toc, toc */
    toc_where = 1 + pdb->num_streams;
    for (i = 0; i < pdb->num_streams; i++)
    {
        char filler[16];
        struct pdb_stream* stream = &pdb->streams[i];
        memset(filler, 0, ARRAY_SIZE(filler));
        if (stream->size)
        {
            /* write stream #i */
            SetFilePointer(hfile, num_blocks * pdb->block_size, NULL, FILE_BEGIN);
            for (j = 0; j < stream->num_buffers; j++)
            {
                ret = WriteFile(hfile, stream->buffers[j].ptr, stream->buffers[j].size, &dummy, NULL);
                ok(ret, "WriteFile error %ld\n", GetLastError());
                if (stream->buffers[j].been_aligned && stream->buffers[j].padding)
                {
                    ret = WriteFile(hfile, filler, stream->buffers[j].padding, &dummy, NULL);
                    ok(ret, "WriteFile error %ld\n", GetLastError());
                }
            }
        }
        toc[1 + i] = stream->size;
        for (j = 0; j < NUM_OF(stream->size, pdb->block_size); j++)
        {
            assert(toc_where < ARRAYSIZE(toc));
            toc[toc_where++] = num_blocks++;
        }
    }
    ds_header.num_blocks = num_blocks;

    /* write toc */
    ds_header.toc_size = toc_where * sizeof(toc[0]);
    assert(ds_header.toc_size < pdb->block_size); /* FIXME: only supporting one block for toc */
    SetFilePointer(hfile, 4 * pdb->block_size, NULL, FILE_BEGIN);
    ret = WriteFile(hfile, toc, ds_header.toc_size, &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    /* write header toc's block_list */
    other = 4;
    SetFilePointer(hfile, 3 * pdb->block_size, NULL, FILE_BEGIN);
    ret = WriteFile(hfile, &other, sizeof(other), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    /* skip free list blocks 1 & 2 (will be zero:ed) */

    /* write ds header */
    SetFilePointer(hfile, 0, NULL, FILE_BEGIN);
    ret = WriteFile(hfile, &ds_header, sizeof(ds_header), &dummy, NULL);
    ok(ret, "WriteFile error %ld\n", GetLastError());

    /* align file size to block size */
    SetFilePointer(hfile, num_blocks * pdb->block_size, NULL, FILE_BEGIN);
    SetEndOfFile(hfile);
}

static BOOL create_test_pdb_ds(const WCHAR* pdb_name, const GUID* guid, DWORD age)
{
    struct PDB_DS_ROOT root =
    {
        .Version = 20000404,
        .TimeDateStamp = 0x32323232, /* it's not reported, so anything will do */
        .Age = ~age, /* actually, it's the age field in DBI which is used, mark to discriminate */
        .guid = *guid,
        .cbNames = 0,
        /* names[] set from root_table */
    };
    unsigned int root_table[] = {0, 1, 0, 0, 0, 0};
    PDB_TYPES TPI =
    {
        .version = /*19990903*/ 20040203 /* llvm chokes on VC 7.0 value */,
        .type_offset = sizeof(TPI),
        .first_index = 0x1000,
        .last_index = 0x1000,
        .type_size = 0,
        .hash_stream = 0xffff,
        .pad = 0xffff,
        .hash_value_size = 4,
        .hash_num_buckets = 0x3ffff,
        .hash_offset = 0,
        .hash_size = 0,
        .search_offset = 0,
        .search_size = 0,
        .type_remap_size = 0,
        .type_remap_offset = 0
    };
    PDB_TYPES IPI = TPI;
    PDB_SYMBOLS DBI =
    {
        .signature = 0xffffffff,
        .version = 19990903, /* VC 7.0 */
        .age = age,
        .global_hash_stream = 0,
        .flags = 0x8700, /* VC 7.0 */
        .public_stream = 0xffff,
        .bldVer = 0,
        .gsym_stream = 0xffff,
        .rbldVer = 0,
        .module_size = 0,
        .sectcontrib_size = 0,
        .segmap_size = 0,
        .srcmodule_size = 0,
        .pdbimport_size = 0,
        .resvd0 = 0,
        .stream_index_size = 0,
        .unknown2_size = 0,
        .resvd3 = 0,
        .machine = IMAGE_FILE_MACHINE_AMD64,
        .resvd4 = 0,
    };
    struct {
        unsigned short count[2];
    } DBI_segments = {{0, 0}};
    PDB_SYMBOL_FILE_EX DBI_modules =
    {
        .unknown1 = 0,
        .range = {1, 0, 0, 1, 0x60500020, 0, 0, 0, 0},
        .flag = 0,
        .stream = 0xffff,
        .symbol_size = 0,
        .lineno_size = 0,
        .lineno2_size = 0,
        .nSrcFiles = 0,
        .attribute = 0,
        .reserved = {0, 0},
        /* filename set on its own */
    };
    PDB_SYMBOL_SOURCE DBI_srcmodules = {1, 0};
    struct {
        unsigned short        the_shorts[2];
        unsigned int          the_long;
        char                  the_names[4];
    } DBI_srcmodules_table = {{0 /* indices */, 0, /* file count */}, 0, {"a.c"}};
    DBI_HASH_HEADER GHASH =
    {
        .signature = 0xffffffff,
        .version = 0xeffe0000 + 19990810,
        .hash_records_size = 0,
        .unknown = 0
    };
    unsigned short dbi_substream[] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, };
    char unknown[] =
    {
        0xfe, 0xef, 0xfe, 0xef, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    static IMAGE_SECTION_HEADER ro_section =
    {
        .Name = ".rodata",
        .Misc.VirtualSize = 0,
        .VirtualAddress = 0,
        .SizeOfRawData = 0,
        .PointerToRawData = 0,
        .PointerToRelocations = 0,
        .PointerToLinenumbers = 0,
        .NumberOfRelocations = 0,
        .NumberOfLinenumbers = 0,
        .Characteristics = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ,
    };
    HANDLE hfile;
    struct pdb_file pdb;
    unsigned int mark;
    struct pdb_stream* stream;

    DBI.age = age;

    pdb_init(&pdb);
    stream = pdb_add_stream(&pdb, NULL, NULL,   0);             /* empty  stream #0 */

                                                                /* always stream #1 */
    stream = pdb_add_stream(&pdb, NULL, &root,  offsetof(struct PDB_DS_ROOT, names));
    pdb_append_to_stream(stream, root_table, sizeof(root_table));

    stream = pdb_add_stream(&pdb, NULL, &TPI,   sizeof(TPI));   /* always stream #2 */

    stream = pdb_add_stream(&pdb, NULL, &DBI,   sizeof(DBI));   /* always stream #3 */
    mark = stream->size;
    pdb_append_to_stream(stream, &DBI_modules, offsetof(PDB_SYMBOL_FILE_EX, filename[0]));
    pdb_append_to_stream(stream, "ab.obj", 7);
    pdb_append_to_stream(stream, "ab.obj", 7);
    DBI.module_size =  pdb_align_stream(stream, 4) - mark;

    /* ranges_size: must be aligned on 4 bytes */

    mark = stream->size;
    pdb_append_to_stream(stream, &DBI_segments, sizeof(DBI_segments));
    DBI.segmap_size = pdb_align_stream(stream, 4) - mark;

    mark = stream->size;
    pdb_append_to_stream(stream, &DBI_srcmodules, offsetof(PDB_SYMBOL_SOURCE, table[0]));
    pdb_append_to_stream(stream, &DBI_srcmodules_table, 4);
    DBI.srcmodule_size = pdb_align_stream(stream, 4) - mark;

    /* pdbimport_size: must be aligned on 4 bytes */

    mark = stream->size;
    /* not really sure of this content, but without it native dbghelp returns error
     * for the PDB file.
     */
    pdb_append_to_stream(stream, unknown, sizeof(unknown));
    DBI.unknown2_size = stream->size - mark;

    mark = stream->size;
    pdb_append_to_stream(stream, dbi_substream, sizeof(dbi_substream));
    DBI.stream_index_size = stream->size - mark;

    stream = pdb_add_stream(&pdb, NULL, &IPI, sizeof(IPI)); /* always stream #4 */

    stream = pdb_add_stream(&pdb, &DBI.global_hash_stream, &GHASH, sizeof(GHASH));
    stream = pdb_add_stream(&pdb, &DBI.gsym_stream, NULL, 0);

    stream = pdb_add_stream(&pdb, &dbi_substream[PDB_SIDX_SECTIONS], &ro_section, sizeof(ro_section));

    hfile = CreateFileW(pdb_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create %ls err %lu\n", pdb_name, GetLastError());
    if (hfile == INVALID_HANDLE_VALUE) return FALSE;

    pdb_write(hfile, &pdb);

    CloseHandle(hfile);

    return TRUE;
}

static BOOL create_test_dbg(const WCHAR* dbg_name, WORD machine, DWORD charac, DWORD timestamp, DWORD size, struct debug_directory_blob *blob)
{
    HANDLE hfile;

    /* minimalistic .dbg made of a header and a DEBUG_DIRECTORY without any data */
    IMAGE_SEPARATE_DEBUG_HEADER header =
    {
        .Signature = 0x4944 /* DI */,
        .Flags = 0, .Machine = machine, .Characteristics = charac, .TimeDateStamp = timestamp,
        .CheckSum = 0, .ImageBase = 0x00040000, .SizeOfImage = size, .NumberOfSections = 0,
        .ExportedNamesSize = 0, .DebugDirectorySize = 0
    };
    DWORD where, expected_size;

    if (blob)
        header.DebugDirectorySize = sizeof(IMAGE_DEBUG_DIRECTORY);

    hfile = CreateFileW(dbg_name, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create %ls err %lu\n", dbg_name, GetLastError());
    if (hfile == INVALID_HANDLE_VALUE) return FALSE;

    check_write_file(hfile, &header, sizeof(header));
    /* FIXME: 0 sections... as header.NumberOfSections */
    if (blob)
    {
        where = SetFilePointer(hfile, 0, NULL, FILE_CURRENT);
        blob->debug_directory.PointerToRawData = (blob->debug_directory.SizeOfData) ?
            where + sizeof(IMAGE_DEBUG_DIRECTORY) : 0;
        check_write_file(hfile, &blob->debug_directory, sizeof(IMAGE_DEBUG_DIRECTORY));
        check_write_file(hfile, blob->content, blob->debug_directory.SizeOfData);
    }
    expected_size = sizeof(header) + header.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
    if (blob)
        expected_size += sizeof(IMAGE_DEBUG_DIRECTORY) + blob->debug_directory.SizeOfData;
    ok(SetFilePointer(hfile, 0, NULL, FILE_CURRENT) == expected_size, "Incorrect file length\n");

    CloseHandle(hfile);
    return TRUE;
}

/* ==============================================
 *                   the tests
 * ==============================================
 */

static void test_srvgetindexes_pe(void)
{
    struct debug_directory_blob* blob_refs[7];
    struct debug_directory_blob* blob_used[4];
    SYMSRV_INDEX_INFOW ssii;
    union nt_header hdr;
    int i, j, bitness;
    BOOL ret;
    WCHAR filename[32];

    static struct
    {
        /* input parameters */
        WORD charac;
        short blobs[ARRAY_SIZE(blob_used)]; /* one of index in blob_refs, -1 to end */
        /* output parameters */
        DWORD age;
        const GUID* guid;
        DWORD sig;
        WCHAR pdb_name[16];
        WCHAR dbg_name[16];
        DWORD last_error;
    }
    indexes[] =
    {
        /* error cases */
/*  0 */{0,                         {-1, -1, -1}, 0,   &null_guid, 0,                                    .last_error = ERROR_BAD_EXE_FORMAT},
        {IMAGE_FILE_DEBUG_STRIPPED, { 0, -1, -1}, 0,   &null_guid, 0,                                    .last_error = ERROR_BAD_EXE_FORMAT},
        {IMAGE_FILE_DEBUG_STRIPPED, { 1, -1, -1}, 123, &null_guid, 0xaaaabbbb, .pdb_name = L"pdbjg.pdb", .last_error = ERROR_BAD_EXE_FORMAT},
        {IMAGE_FILE_DEBUG_STRIPPED, { 2, -1, -1}, 124, &guid1,     0,          .pdb_name = L"pdbds.pdb", .last_error = ERROR_BAD_EXE_FORMAT},
        {IMAGE_FILE_DEBUG_STRIPPED, {-1, -1, -1}, 0,   &null_guid, 0,                                    .last_error = ERROR_BAD_EXE_FORMAT}, /* not 100% logical ! */
        /* success */
/*  5 */{0,                         { 0, -1, -1}, 0,   &null_guid, 0           },
        {0,                         { 1, -1, -1}, 123, &null_guid, 0xaaaabbbb, .pdb_name = L"pdbjg.pdb"},
        {0,                         { 2, -1, -1}, 124, &guid1,     0,          .pdb_name = L"pdbds.pdb"},
        {IMAGE_FILE_DEBUG_STRIPPED, { 3, -1, -1}, 0,   &null_guid, 0,          .dbg_name = L".\\ascii.dbg"},
        {0,                         { 3, -1, -1}, 0,   &null_guid, 0,          },
        /* PDB (JS & DS) records are cumulated (age from JS & guid from DS) */
/* 10 */{0,                         { 1,  2, -1}, 124, &guid1,     0xaaaabbbb, .pdb_name = L"pdbds.pdb"},
        {0,                         { 2,  1, -1}, 123, &guid1,     0xaaaabbbb, .pdb_name = L"pdbjg.pdb"},
        /* cumulative records of same type */
        {0,                         { 1,  4, -1}, 125, &null_guid, 0xaaaacccc, .pdb_name = L"pdbjg2.pdb"},
        {0,                         { 2,  5, -1}, 126, &guid2,     0,          .pdb_name = L"pdbds2.pdb"},
        {0,                         { 3,  6, -1},   0, &null_guid, 0,          },
/* 15 */{IMAGE_FILE_DEBUG_STRIPPED, { 3,  6, -1},   0, &null_guid, 0,          .dbg_name = L".\\ascii.dbg"},
        /* Mixing MISC with PDB (JG or DS) records */
        {IMAGE_FILE_DEBUG_STRIPPED, { 3,  1, -1}, 123, &null_guid, 0xaaaabbbb, .pdb_name = L"pdbjg.pdb", .dbg_name = L".\\ascii.dbg"},
        {0,                         { 3,  1, -1}, 123, &null_guid, 0xaaaabbbb, .pdb_name = L"pdbjg.pdb"},
        {IMAGE_FILE_DEBUG_STRIPPED, { 3,  2, -1}, 124, &guid1,     0,          .pdb_name = L"pdbds.pdb", .dbg_name = L".\\ascii.dbg"},
        {0,                         { 3,  2, -1}, 124, &guid1,     0,          .pdb_name = L"pdbds.pdb"},
/* 20 */{IMAGE_FILE_DEBUG_STRIPPED, { 1,  3, -1}, 123, &null_guid, 0xaaaabbbb, .pdb_name = L"pdbjg.pdb", .dbg_name = L".\\ascii.dbg"},
        {0,                         { 1,  3, -1}, 123, &null_guid, 0xaaaabbbb, .pdb_name = L"pdbjg.pdb"},
        {IMAGE_FILE_DEBUG_STRIPPED, { 2,  3, -1}, 124, &guid1,     0,          .pdb_name = L"pdbds.pdb", .dbg_name = L".\\ascii.dbg"},
        {0,                         { 2,  3, -1}, 124, &guid1,     0,          .pdb_name = L"pdbds.pdb"},
   };

    /* testing PE header with debug directory content */
    blob_refs[0] = make_empty_blob();
    blob_refs[1] = make_pdb_jg_blob(0x67899876, 0xaaaabbbb, 123, "pdbjg.pdb");
    blob_refs[2] = make_pdb_ds_blob(0x67899877, &guid1,     124, "pdbds.pdb");
    blob_refs[3] = make_dbg_blob   (0x67899878, ".\\ascii.dbg"); /* doesn't seem to be returned without path */
    blob_refs[4] = make_pdb_jg_blob(0x67899879, 0xaaaacccc, 125, "pdbjg2.pdb");
    blob_refs[5] = make_pdb_ds_blob(0x67899880, &guid2,     126, "pdbds2.pdb");
    blob_refs[6] = make_dbg_blob   (0x67899801, ".\\ascii.dbg"); /* doesn't seem to be returned without path */

    for (bitness = 32; bitness <= 64; bitness += 32)
    {
        for (i = 0; i < ARRAY_SIZE(indexes); i++)
        {
            winetest_push_context("%u-bit #%02u", bitness, i);

            /* create dll */
            for (j = 0; j < ARRAY_SIZE(indexes[i].blobs); j++)
            {
                if (indexes[i].blobs[j] == -1 || indexes[i].blobs[j] >= ARRAY_SIZE(blob_refs)) break;
                blob_used[j] = blob_refs[indexes[i].blobs[j]];
            }
            swprintf(filename, ARRAY_SIZE(filename), L".\\winetest%02u.dll", i);
            if (bitness == 32)
            {
                init_headers32(&hdr.nt_header32, 0x67890000 + i, 0x0030cafe, indexes[i].charac);
                create_test_dll(&hdr, sizeof(hdr.nt_header32), blob_used, j, filename);
            }
            else
            {
                init_headers64(&hdr.nt_header64, 0x67890000 + i, 0x0030cafe, indexes[i].charac);
                create_test_dll(&hdr, sizeof(hdr.nt_header64), blob_used, j, filename);
            }

            memset(&ssii, 0xa5, sizeof(ssii));
            ssii.sizeofstruct = sizeof(ssii);
            ret = SymSrvGetFileIndexInfoW(filename, &ssii, 0);
            if (indexes[i].last_error)
            {
                ok(!ret, "SymSrvGetFileIndexInfo should have failed\n");
                ok(GetLastError() == indexes[i].last_error, "Mismatch in GetLastError: %lu\n", GetLastError());
            }
            else
                ok(ret, "SymSrvGetFileIndexInfo failed: %lu\n", GetLastError());
            if (ret || indexes[i].last_error == ERROR_BAD_EXE_FORMAT)
            {
                ok(ssii.age == indexes[i].age, "Mismatch in age: %lu\n", ssii.age);
                ok(IsEqualGUID(&ssii.guid, indexes[i].guid),
                   "Mismatch in guid: guid=%s\n", wine_dbgstr_guid(&ssii.guid));

                ok(ssii.sig == indexes[i].sig, "Mismatch in sig: %lx\n", ssii.sig);
                ok(ssii.size == 0x0030cafe, "Mismatch in size: %lx <> %x\n", ssii.size, 0x0030cafe);
                ok(ssii.stripped != 0xa5a5a5a5 &&
                   (!ssii.stripped) == ((indexes[i].charac & IMAGE_FILE_DEBUG_STRIPPED) == 0),
                   "Mismatch in stripped: %x\n", ssii.stripped);
                ok(ssii.timestamp == 0x67890000 + i, "Mismatch in timestamp: %lx\n", ssii.timestamp);
                ok(!wcscmp(ssii.file, filename + 2), "Mismatch in file: '%ls'\n", ssii.file);
                ok(!wcscmp(ssii.dbgfile, indexes[i].dbg_name), "Mismatch in dbgfile: '%ls'\n", ssii.dbgfile);
                ok(!wcscmp(ssii.pdbfile, indexes[i].pdb_name), "Mismatch in pdbfile: '%ls'\n", ssii.pdbfile);
            }
            ret = DeleteFileW(filename);
            ok(ret, "Couldn't delete test DLL file\n");

            winetest_pop_context();
        }
    }
    for (i = 0; i < ARRAY_SIZE(blob_refs); i++) free(blob_refs[i]);
}

static void test_srvgetindexes_pdb(void)
{
    unsigned int i;
    WCHAR filename[128];
    SYMSRV_INDEX_INFOW ssii;
    BOOL ret;

    static struct
    {
        /* input parameters */
        const GUID* guid;
    }
    indexes[] =
    {
        {&null_guid},
        {&guid1},
    };
    for (i = 0; i < ARRAYSIZE(indexes); i++)
    {
        winetest_push_context("pdb#%02u", i);

        /* create dll */
        swprintf(filename, ARRAY_SIZE(filename), L"winetest%02u.pdb", i);
        create_test_pdb_ds(filename, indexes[i].guid, 240 + i);

        memset(&ssii, 0x45, sizeof(ssii));
        ssii.sizeofstruct = sizeof(ssii);
        ret = SymSrvGetFileIndexInfoW(filename, &ssii, 0);
        ok(ret, "SymSrvGetFileIndexInfo failed: %lu\n", GetLastError());

        ok(ssii.age == 240 + i, "Mismatch in age: %lx\n", ssii.age);
        ok(!memcmp(&ssii.guid, indexes[i].guid, sizeof(GUID)),
           "Mismatch in guid: guid=%s\n", wine_dbgstr_guid(&ssii.guid));

        /* DS PDB don't have signature, only JG PDB have */
        ok(ssii.sig == 0, "Mismatch in sig: %lx\n", ssii.sig);
        ok(ssii.size == 0, "Mismatch in size: %lx\n", ssii.size);
        ok(!ssii.stripped, "Mismatch in stripped: %x\n", ssii.stripped);
        ok(ssii.timestamp == 0, "Mismatch in timestamp: %lx\n", ssii.timestamp);
        ok(!wcscmp(ssii.file, filename), "Mismatch in file: %ls\n", ssii.file);
        ok(!ssii.pdbfile[0], "Mismatch in pdbfile: %ls\n", ssii.pdbfile);
        ok(!ssii.dbgfile[0], "Mismatch in dbgfile: %ls\n", ssii.dbgfile);

        DeleteFileW(filename);
        winetest_pop_context();
    }
}

static void test_srvgetindexes_dbg(void)
{
    unsigned int i;
    WCHAR filename[128];
    SYMSRV_INDEX_INFOW ssii;
    BOOL ret;
    struct debug_directory_blob *blob_refs[1];

    static struct
    {
        /* input parameters */
        WORD        machine;
        DWORD       characteristics;
        DWORD       timestamp;
        DWORD       imagesize;
        int         blob;
        /* output parameters */
        DWORD       age;
        const GUID *guid;
        WCHAR       pdbname[16];
        WCHAR       dbgname[16];
        DWORD       last_error;
    }
    indexes[] =
    {
        {IMAGE_FILE_MACHINE_I386,  0, 0x1234, 0x00560000, -1, 0, &null_guid, .last_error = ERROR_BAD_EXE_FORMAT},
        {IMAGE_FILE_MACHINE_AMD64, 0, 0x1235, 0x00570000, -1, 0, &null_guid, .last_error = ERROR_BAD_EXE_FORMAT},
        {IMAGE_FILE_MACHINE_I386,  0, 0x1234, 0x00560000, 0, 123, &guid1, .pdbname=L"foo.pdb"},
        {IMAGE_FILE_MACHINE_AMD64, 0, 0x1235, 0x00570000, 0, 123, &guid1, .pdbname=L"foo.pdb"},
    };

    blob_refs[0] = make_pdb_ds_blob(0x1226, &guid1, 123, "foo.pdb");
    for (i = 0; i < ARRAY_SIZE(indexes); i++)
    {
        winetest_push_context("dbg#%02u", i);

        /* create dll */
        swprintf(filename, ARRAY_SIZE(filename), L"winetest%02u.dbg", i);
        ret = create_test_dbg(filename, indexes[i].machine, indexes[i].characteristics,
                              indexes[i].timestamp, indexes[i].imagesize,
                              indexes[i].blob == -1 ? NULL : blob_refs[indexes[i].blob]);
        ok(ret, "Couldn't create dbg file %ls\n", filename);

        memset(&ssii, 0x45, sizeof(ssii));
        ssii.sizeofstruct = sizeof(ssii);
        ret = SymSrvGetFileIndexInfoW(filename, &ssii, 0);
        if (indexes[i].last_error)
        {
            ok(!ret, "SymSrvGetFileIndexInfo should have\n");
            ok(GetLastError() == ERROR_BAD_EXE_FORMAT, "Unexpected last error: %lu\n", GetLastError());
        }
        else
            ok(ret, "SymSrvGetFileIndexInfo failed: %lu\n", GetLastError());

        ok(ssii.age == indexes[i].age, "Mismatch in age: %lx\n", ssii.age);
        ok(IsEqualGUID(&ssii.guid, indexes[i].guid),
           "Mismatch in guid: guid=%s\n", wine_dbgstr_guid(&ssii.guid));
        ok(ssii.sig == 0, "Mismatch in sig: %lx\n", ssii.sig);
        ok(ssii.size == indexes[i].imagesize, "Mismatch in size: %lx\n", ssii.size);
        ok(!ssii.stripped, "Mismatch in stripped: %x\n", ssii.stripped);
        ok(ssii.timestamp == indexes[i].timestamp, "Mismatch in timestamp: %lx\n", ssii.timestamp);
        ok(!wcscmp(ssii.file, filename), "Mismatch in file: %ls\n", ssii.file);
        ok(!wcscmp(ssii.pdbfile, indexes[i].pdbname), "Mismatch in pdbfile: %ls\n", ssii.pdbfile);
        ok(!wcscmp(ssii.dbgfile, indexes[i].dbgname), "Mismatch in dbgfile: %ls\n", ssii.dbgfile);

        DeleteFileW(filename);
        winetest_pop_context();
    }
    for (i = 0; i < ARRAY_SIZE(blob_refs); i++) free(blob_refs[i]);
}

static void make_path(WCHAR file[MAX_PATH], const WCHAR* topdir, const WCHAR* subdir, const WCHAR* base)
{
    wcscpy(file, topdir);
    if (subdir)
    {
        if (file[wcslen(file) - 1] != '\\') wcscat(file, L"\\");
        wcscat(file, subdir);
    }
    if (base)
    {
        if (file[wcslen(file) - 1] != '\\') wcscat(file, L"\\");
        wcscat(file, base);
    }
}

struct path_validate
{
    GUID guid;
    DWORD age;
    DWORD timestamp;
    DWORD size_of_image;
    unsigned cb_count;
};

static BOOL CALLBACK path_cb(PCWSTR filename, void* _usr)
{
    struct path_validate* pv = _usr;
    SYMSRV_INDEX_INFOW ssii;
    BOOL ret;

    pv->cb_count++;
    ok(filename[0] && filename[1] == ':', "Expecting full path, but got %ls\n", filename);

    memset(&ssii, 0, sizeof(ssii));
    ssii.sizeofstruct = sizeof(ssii);
    ret = SymSrvGetFileIndexInfoW(filename, &ssii, 0);
    ok(ret, "SymSrvGetFileIndexInfo failed: %lu %ls\n", GetLastError(), filename);
    return !(ret && !memcmp(&ssii.guid, &pv->guid, sizeof(GUID)) &&
             ssii.age == pv->age && ssii.timestamp == pv->timestamp && ssii.size == pv->size_of_image);
}

static unsigned char2index(char ch)
{
    unsigned val;
    if (ch >= '0' && ch <= '9')
        val = ch - '0';
    else if (ch >= 'a' && ch <= 'z')
        val = 10 + ch - 'a';
    else val = ~0u;
    return val;
}

/* Despite what MS documentation states, option EXACT_SYMBOLS doesn't always work
 * (it looks like it's only enabled when symsrv.dll is loaded).
 * So, we don't test with EXACT_SYMBOLS set, nor with a NULL callback, and defer
 * to our own custom callback the testing.
 * (We always end when a full matched is found).
 */
static void test_find_in_path_pe(void)
{
    HANDLE proc = (HANDLE)(DWORD_PTR)0x666002;
    WCHAR topdir[MAX_PATH];
    WCHAR search[MAX_PATH];
    WCHAR file[MAX_PATH];
    WCHAR found[MAX_PATH];
    struct path_validate pv;
    DWORD len;
    BOOL ret;
    int i;
    union nt_header hdr;

    static const struct file_tests
    {
        unsigned bitness; /* 32 or 64 */
        DWORD timestamp;
        DWORD size_of_image;
        const WCHAR* module_path;
    }
    file_tests[] =
    {
        /*  0 */ { 64, 0x12345678,     0x0030cafe, L"foobar.dll" },
        /*  1 */ { 64, 0x12345678,     0x0030cafe, L"A\\foobar.dll" },
        /*  2 */ { 64, 0x12345678,     0x0030cafe, L"B\\foobar.dll" },
        /*  3 */ { 64, 0x56781234,     0x0010f00d, L"foobar.dll" },
        /*  4 */ { 64, 0x56781234,     0x0010f00d, L"A\\foobar.dll" },
        /*  5 */ { 64, 0x56781234,     0x0010f00d, L"B\\foobar.dll" },
        /*  6 */ { 32, 0x12345678,     0x0030cafe, L"foobar.dll" },
        /*  7 */ { 32, 0x12345678,     0x0030cafe, L"A\\foobar.dll" },
        /*  8 */ { 32, 0x12345678,     0x0030cafe, L"B\\foobar.dll" },
        /*  9 */ { 32, 0x56781234,     0x0010f00d, L"foobar.dll" },
        /*  a */ { 32, 0x56781234,     0x0010f00d, L"A\\foobar.dll" },
        /*  b */ { 32, 0x56781234,     0x0010f00d, L"B\\foobar.dll" },
    };
    static const struct image_tests
    {
        /* files to generate */
        const char* files;
        /* parameters for lookup */
        DWORD lookup_timestamp;
        DWORD lookup_size_of_image;
        const char* search; /* several of ., A, B to link to directories */
        const WCHAR* module;
        /* expected results */
        const WCHAR* found_subdir;
        DWORD expected_cb_count;
    }
    image_tests[] =
    {
        /* all files 64 bit */
        /* the passed timestamp & size are not checked before calling the callback! */
        /*  0 */ { "0",   0x00000000, 0x00000000, ".", L"foobar.dll",  NULL,   1},
        /*  1 */ { "0",   0x12345678, 0x00000000, ".", L"foobar.dll",  NULL,   1},
        /*  2 */ { "0",   0x00000000, 0x0030cafe, ".", L"foobar.dll",  NULL,   1},
        /*  3 */ { "0",   0x12345678, 0x0030cafe, ".", L"foobar.dll",  L"",    1},
        /* no recursion into subdirectories */
        /*  4 */ { "1",   0x12345678, 0x0030cafe, ".", L"foobar.dll",  NULL,   0},
        /* directories are searched in order */
        /*  5 */ { "15",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /*  6 */ { "15",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"A\\", 2},
        /*  7 */ { "12",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /*  8 */ { "12",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"B\\", 1},

        /* all files 32 bit */
        /* the passed timestamp & size is not checked ! */
        /*  9 */ { "6",   0x00000000, 0x00000000, ".", L"foobar.dll",  NULL,   1},
        /* 10 */ { "6",   0x12345678, 0x00000000, ".", L"foobar.dll",  NULL,   1},
        /* 11 */ { "6",   0x00000000, 0x0030cafe, ".", L"foobar.dll",  NULL,   1},
        /* 12 */ { "6",   0x12345678, 0x0030cafe, ".", L"foobar.dll",  L"",    1},
        /* no recursion into subdirectories */
        /* 13 */ { "7",   0x12345678, 0x0030cafe, ".", L"foobar.dll",  NULL,   0},
        /* directories are searched in order */
        /* 14 */ { "7b",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /* 15 */ { "7b",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"A\\", 2},
        /* 16 */ { "78",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /* 17 */ { "78",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"B\\", 1},

        /* machine and bitness is not used */
        /* 18 */ { "1b",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /* 19 */ { "1b",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"A\\", 2},
        /* 20 */ { "18",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /* 21 */ { "18",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"B\\", 1},
        /* 22 */ { "75",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /* 23 */ { "75",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"A\\", 2},
        /* 24 */ { "72",  0x12345678, 0x0030cafe, "AB", L"foobar.dll", L"A\\", 1},
        /* 25 */ { "72",  0x12345678, 0x0030cafe, "BA", L"foobar.dll", L"B\\", 1},

        /* specifying a full path to module isn't taken into account */
        /* 26 */ { "0",   0x12345678, 0x0030cafe, "AB", L"@foobar.dll", NULL, 0},
        /* 27 */ { "6",   0x12345678, 0x0030cafe, "AB", L"@foobar.dll", NULL, 0},

    };
    static const WCHAR* list_directories[] = {L"A", L"B", L"dll", L"symbols", L"symbols\\dll", L"acm", L"symbols\\acm"};

    ret = SymInitializeW(proc, NULL, FALSE);
    ok(ret, "Couldn't init dbghelp\n");

    len = GetTempPathW(ARRAY_SIZE(topdir), topdir);
    ok(len && len < ARRAY_SIZE(topdir), "Unexpected length\n");
    wcscat(topdir, L"dh.tmp\\");
    ret = CreateDirectoryW(topdir, NULL);
    ok(ret, "Couldn't create directory\n");
    for (i = 0; i < ARRAY_SIZE(list_directories); i++)
    {
        make_path(file, topdir, list_directories[i], NULL);
        ret = CreateDirectoryW(file, NULL);
        ok(ret, "Couldn't create directory %u %ls\n", i, list_directories[i]);
    }

    for (i = 0; i < ARRAY_SIZE(image_tests); ++i)
    {
        const char* ptr;
        winetest_push_context("%u", i);

        for (ptr = image_tests[i].files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(file_tests))
            {
                const struct file_tests* ft = &file_tests[val];
                struct debug_directory_blob* ds_blob;

                make_path(file, topdir, NULL, ft->module_path);
                /* all modules created with ds pdb reference so that SymSrvGetFileInfoInfo() succeeds */
                ds_blob = make_pdb_ds_blob(ft->timestamp, &guid1, 124 + i, "pdbds.pdb");
                if (ft->bitness == 64)
                {
                    init_headers64(&hdr.nt_header64, ft->timestamp, ft->size_of_image, 0);
                    create_test_dll(&hdr, sizeof(hdr.nt_header64), &ds_blob, 1, file);
                }
                else
                {
                    init_headers32(&hdr.nt_header32, ft->timestamp, ft->size_of_image, 0);
                    create_test_dll(&hdr, sizeof(hdr.nt_header32), &ds_blob, 1, file);
                }
                free(ds_blob);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        search[0] = L'\0';
        for (ptr = image_tests[i].search; *ptr; ptr++)
        {
            if (*search) wcscat(search, L";");
            wcscat(search, topdir);
            if (*ptr == '.')
                ;
            else if (*ptr == 'A')
                wcscat(search, L"A");
            else if (*ptr == 'B')
                wcscat(search, L"B");
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        if (image_tests[i].module[0] == L'@')
            make_path(file, topdir, NULL, &image_tests[i].module[1]);
        else
            wcscpy(file, image_tests[i].module);

        memset(&pv, 0, sizeof(pv));
        pv.age = 124 + i;
        pv.guid = guid1;
        pv.timestamp = image_tests[i].lookup_timestamp;
        pv.size_of_image = image_tests[i].lookup_size_of_image;
        memset(found, 0, sizeof(found));

        ret = SymFindFileInPathW(proc, search, file,
                                 (void*)&image_tests[i].lookup_timestamp,
                                 image_tests[i].lookup_size_of_image, 0,
                                 SSRVOPT_DWORDPTR, found, path_cb, &pv);

        ok(pv.cb_count == image_tests[i].expected_cb_count,
           "Mismatch in cb count, got %u (expected %lu)\n",
           pv.cb_count, image_tests[i].expected_cb_count);
        if (image_tests[i].found_subdir)
        {
            size_t l1, l2, l3;

            ok(ret, "Couldn't find file: %ls %lu\n", search, GetLastError());

            l1 = wcslen(topdir);
            l2 = wcslen(image_tests[i].found_subdir);
            l3 = wcslen(image_tests[i].module);

            ok(l1 + l2 + l3 == wcslen(found) &&
               !memcmp(found, topdir, l1 * sizeof(WCHAR)) &&
               !memcmp(&found[l1], image_tests[i].found_subdir, l2 * sizeof(WCHAR)) &&
               !memcmp(&found[l1 + l2], image_tests[i].module, l3 * sizeof(WCHAR)),
               "Mismatch in found file %ls (expecting %ls)\n",
               found, image_tests[i].found_subdir);
        }
        else
        {
            ok(!ret, "File reported found, while failure is expected\n");
            ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Unexpected error %lu\n", GetLastError());
        }
        for (ptr = image_tests[i].files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(file_tests))
            {
                const struct file_tests* ft = &file_tests[val];
                make_path(file, topdir, NULL, ft->module_path);
                ret = DeleteFileW(file);
                ok(ret, "Couldn't delete file %c %ls\n", *ptr, file);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }
        winetest_pop_context();
    }

    SymCleanup(proc);

    for (i = ARRAY_SIZE(list_directories) - 1; i >= 0; i--)
    {
        make_path(file, topdir, list_directories[i], NULL);
        ret = RemoveDirectoryW(file);
        ok(ret, "Couldn't remove directory %u %ls\n", i, list_directories[i]);
    }

    ret = RemoveDirectoryW(topdir);
    ok(ret, "Couldn't remove directory\n");
}

/* Comment on top of test_find_in_path_pe() also applies here */
static void test_find_in_path_pdb(void)
{
    HANDLE proc = (HANDLE)(DWORD_PTR)0x666002;
    WCHAR topdir[MAX_PATH];
    WCHAR search[MAX_PATH];
    WCHAR file[MAX_PATH];
    WCHAR found[MAX_PATH];
    struct path_validate pv;
    DWORD len;
    BOOL ret;
    int i;

    static const struct debug_info_file
    {
        const GUID* guid;
        DWORD       age;
        const WCHAR*module_path;
    }
    test_files[] =
    {
        /*  0 */ { &guid1,     0x0030cafe, L"foobar.pdb" },
        /*  1 */ { &guid1,     0x0030cafe, L"A\\foobar.pdb" },
        /*  2 */ { &guid1,     0x0030cafe, L"B\\foobar.pdb" },
        /*  3 */ { &guid2,     0x0010f00d, L"foobar.pdb" },
        /*  4 */ { &guid2,     0x0010f00d, L"A\\foobar.pdb" },
        /*  5 */ { &guid2,     0x0010f00d, L"B\\foobar.pdb" },
        /*  6 */ { &guid1,     0x0030cafe, L"foobar.pdb" },
        /*  7 */ { &guid1,     0x0030cafe, L"A\\foobar.pdb" },
        /*  8 */ { &guid1,     0x0030cafe, L"B\\foobar.pdb" },
        /*  9 */ { &guid2,     0x0010f00d, L"foobar.pdb" },
        /*  a */ { &guid2,     0x0010f00d, L"A\\foobar.pdb" },
        /*  b */ { &guid2,     0x0010f00d, L"B\\foobar.pdb" },
        /*  c */ { &guid1,     0x0030cafe, L"dll\\foobar.pdb" },
        /*  d */ { &guid1,     0x0030cafe, L"symbols\\dll\\foobar.pdb" },
    };
    static const struct image_tests
    {
        /* files to generate */
        const char* files;
        /* parameters for lookup */
        const GUID* lookup_guid;
        DWORD lookup_age;
        const char* search; /* several of ., A, B to link to directories */
        const WCHAR* module;
        /* expected results */
        const WCHAR* found_subdir;
        DWORD expected_cb_count;
    }
    pdb_tests[] =
    {
        /* all files 64 bit */
        /* the passed timestamp & size are not checked before calling the callback! */
        /*  0 */ { "0",   NULL,   0x00000000, ".", L"foobar.pdb",  NULL,   1},
        /*  1 */ { "0",   &guid1, 0x00000000, ".", L"foobar.pdb",  NULL,   1},
        /*  2 */ { "0",   NULL,   0x0030cafe, ".", L"foobar.pdb",  NULL,   1},
        /*  3 */ { "0",   &guid1, 0x0030cafe, ".", L"foobar.pdb",  L"",    1},
        /* no recursion into subdirectories */
        /*  4 */ { "1",   &guid1, 0x0030cafe, ".", L"foobar.pdb",  NULL,   0},
        /* directories are searched in order */
        /*  5 */ { "15",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /*  6 */ { "15",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"A\\", 2},
        /*  7 */ { "12",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /*  8 */ { "12",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"B\\", 1},

        /* all files 32 bit */
        /* the passed timestamp & size is not checked ! */
        /*  9 */ { "6",   NULL,   0x00000000, ".", L"foobar.pdb",  NULL,   1},
        /* 10 */ { "6",   &guid1, 0x00000000, ".", L"foobar.pdb",  NULL,   1},
        /* 11 */ { "6",   NULL,   0x0030cafe, ".", L"foobar.pdb",  NULL,   1},
        /* 12 */ { "6",   &guid1, 0x0030cafe, ".", L"foobar.pdb",  L"",    1},
        /* no recursion into subdirectories */
        /* 13 */ { "7",   &guid1, 0x0030cafe, ".", L"foobar.pdb",  NULL,   0},
        /* directories are searched in order */
        /* 14 */ { "7b",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /* 15 */ { "7b",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"A\\", 2},
        /* 16 */ { "78",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /* 17 */ { "78",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"B\\", 1},

        /* machine and bitness is not used */
        /* 18 */ { "1b",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /* 19 */ { "1b",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"A\\", 2},
        /* 20 */ { "18",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /* 21 */ { "18",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"B\\", 1},
        /* 22 */ { "75",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /* 23 */ { "75",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"A\\", 2},
        /* 24 */ { "72",  &guid1, 0x0030cafe, "AB", L"foobar.pdb", L"A\\", 1},
        /* 25 */ { "72",  &guid1, 0x0030cafe, "BA", L"foobar.pdb", L"B\\", 1},

        /* specifying a full path to module isn't taken into account */
        /* 26 */ { "0",   &guid1, 0x0030cafe, "AB", L"@foobar.pdb", NULL,  0},
        /* 27 */ { "6",   &guid1, 0x0030cafe, "AB", L"@foobar.pdb", NULL,  0},

        /* subdirectories searched by SymLoadModule*() are not covered implicitely */
        /* 28 */ { "c",   &guid1, 0x0030cafe, ".", L"foobar.pdb",   NULL,  0},
        /* 28 */ { "d",   &guid1, 0x0030cafe, ".", L"foobar.pdb",   NULL,  0},

    };
    static const WCHAR* list_directories[] = {L"A", L"B", L"dll", L"symbols", L"symbols\\dll", L"acm", L"symbols\\acm"};

    ret = SymInitializeW(proc, NULL, FALSE);
    ok(ret, "Couldn't init dbghelp\n");

    len = GetTempPathW(ARRAY_SIZE(topdir), topdir);
    ok(len && len < ARRAY_SIZE(topdir), "Unexpected length\n");
    wcscat(topdir, L"dh.tmp\\");
    ret = CreateDirectoryW(topdir, NULL);
    ok(ret, "Couldn't create directory\n");
    for (i = 0; i < ARRAY_SIZE(list_directories); i++)
    {
        make_path(file, topdir, list_directories[i], NULL);
        ret = CreateDirectoryW(file, NULL);
        ok(ret, "Couldn't create directory %u %ls\n", i, list_directories[i]);
    }

    for (i = 0; i < ARRAY_SIZE(pdb_tests); ++i)
    {
        const char* ptr;
        winetest_push_context("path_pdb_%u", i);

        for (ptr = pdb_tests[i].files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(test_files))
            {
                make_path(file, topdir, NULL, test_files[val].module_path);
                create_test_pdb_ds(file, test_files[val].guid, test_files[val].age);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        search[0] = L'\0';
        for (ptr = pdb_tests[i].search; *ptr; ptr++)
        {
            if (*search) wcscat(search, L";");
            wcscat(search, topdir);
            if (*ptr == '.')
                ;
            else if (*ptr == 'A')
                wcscat(search, L"A");
            else if (*ptr == 'B')
                wcscat(search, L"B");
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        if (pdb_tests[i].module[0] == L'@')
            make_path(file, topdir, NULL, &pdb_tests[i].module[1]);
        else
            wcscpy(file, pdb_tests[i].module);

        memset(&pv, 0, sizeof(pv));
        if (pdb_tests[i].lookup_guid)
            pv.guid = *pdb_tests[i].lookup_guid;
        pv.age = pdb_tests[i].lookup_age;
        memset(found, 0, sizeof(found));

        ret = SymFindFileInPathW(proc, search, file,
                                 (void*)pdb_tests[i].lookup_guid,
                                 0, pdb_tests[i].lookup_age,
                                 SSRVOPT_GUIDPTR, found, path_cb, &pv);

        ok(pv.cb_count == pdb_tests[i].expected_cb_count,
           "Mismatch in cb count, got %u (expected %lu)\n",
           pv.cb_count, pdb_tests[i].expected_cb_count);
        if (pdb_tests[i].found_subdir)
        {
            size_t l1, l2, l3;

            ok(ret, "Couldn't find file: %ls %lu\n", search, GetLastError());

            l1 = wcslen(topdir);
            l2 = wcslen(pdb_tests[i].found_subdir);
            l3 = wcslen(pdb_tests[i].module);

            ok(l1 + l2 + l3 == wcslen(found) &&
               !memcmp(found, topdir, l1 * sizeof(WCHAR)) &&
               !memcmp(&found[l1], pdb_tests[i].found_subdir, l2 * sizeof(WCHAR)) &&
               !memcmp(&found[l1 + l2], pdb_tests[i].module, l3 * sizeof(WCHAR)),
               "Mismatch in found file %ls (expecting %ls)\n",
               found, pdb_tests[i].found_subdir);
        }
        else
        {
            ok(!ret, "File reported found, while failure is expected\n");
            ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Unexpected error %lu\n", GetLastError());
        }

        for (ptr = pdb_tests[i].files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(test_files))
            {
                make_path(file, topdir, NULL, test_files[val].module_path);
                ret = DeleteFileW(file);
                ok(ret, "Couldn't delete file %c %ls\n", *ptr, file);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        winetest_pop_context();
    }

    SymCleanup(proc);

    for (i = ARRAY_SIZE(list_directories) - 1; i >= 0; i--)
    {
        make_path(file, topdir, list_directories[i], NULL);
        ret = RemoveDirectoryW(file);
        ok(ret, "Couldn't remove directory %u %ls\n", i, list_directories[i]);
    }

    ret = RemoveDirectoryW(topdir);
    ok(ret, "Couldn't remove directory\n");
}

static void test_load_modules_path(void)
{
    static const WCHAR* top_subdirs[] = {L"mismatch",
                                         L"search.tmp", L"search.tmp\\dll", L"search.tmp\\symbols", L"search.tmp\\symbols\\dll", L"search.tmp\\dummy"};
    static const struct debug_info_file
    {
        const GUID* guid; /* non-NULL means PDB/DS, DBG otherwise */
        DWORD       age_or_timestamp; /* age for PDB, timestamp for DBG */
        const WCHAR*module_path;
    }
    test_files[] =
    {
        /*  0 */ { &guid1, 0x0030cafe, L"bar.pdb" },
        /*  1 */ { &guid1, 0x0030cafe, L"search.tmp\\bar.pdb" },
        /*  2 */ { &guid1, 0x0030cafe, L"search.tmp\\dll\\bar.pdb" },
        /*  3 */ { &guid1, 0x0030cafe, L"search.tmp\\symbols\\dll\\bar.pdb" },
        /*  4 */ { &guid1, 0x0030cafe, L"search.tmp\\dummy\\bar.pdb" },
        /*  5 */ { &guid1, 0x0031cafe, L"mismatch\\bar.pdb" },
        /*  6 */ { &guid1, 0x0032cafe, L"search.tmp\\bar.pdb" },
        /*  7 */ { &guid1, 0x0033cafe, L"search.tmp\\dll\\bar.pdb" },
    };

    static const struct module_path_test
    {
        /* input parameters */
        DWORD        options;
        const char  *search;     /* search path in SymInitialize (s=search, t=top=dll's dir, m=mismatch) */
        const char  *test_files; /* various test_files to be created */
        /* output parameters */
        int          found_file;
    }
    module_path_tests[] =
    {
/* 0*/   {0,                            NULL,      "0",     0},
         /* matching pdb, various directories searched */
         {0,                            "s",       "0",     0},
         {0,                            "s",       "01",    1},
         {0,                            "s",       "1",     1},
         {0,                            "s",       "2",     2},
/* 5*/   {0,                            "s",       "3",     3},
         {0,                            "s",       "1234",  1},
         {0,                            "s",       "234",   2},
         {0,                            "s",       "34",    3},
         {0,                            "s",       "4",    -1},
         /* no matching pdb, impact of options */
/*10*/   {0,                            "s",       "7",    -1},
         {SYMOPT_LOAD_ANYTHING,         "s",       "7",     7},
         {SYMOPT_LOAD_ANYTHING,         "s",       "67",    6},
         {SYMOPT_EXACT_SYMBOLS,         "s",       "7",    -1}, /* doesn't seem effective on path search */
         /* mismatch and several search directories */
         {0,                            "ms",      "51",    1},
/*15*/   {0,                            "sm",      "51",    1},
         {SYMOPT_LOAD_ANYTHING,         "ms",      "51",    1},
         {SYMOPT_LOAD_ANYTHING,         "sm",      "51",    1},
         {0,                            "ms",      "57",   -1},
         {0,                            "sm",      "57",   -1},
/*20*/   {SYMOPT_LOAD_ANYTHING,         "ms",      "57",    5},
         {SYMOPT_LOAD_ANYTHING,         "sm",      "57",    7},
    };

    int i;
    HANDLE dummy = (HANDLE)(ULONG_PTR)0xc4fef00d;
    DWORD old_options;
    IMAGEHLP_MODULEW64 im;
    BOOL ret;
    DWORD64 base;
    WCHAR topdir[MAX_PATH];
    WCHAR filename[MAX_PATH];
    DWORD len;
    union nt_header h;
    struct debug_directory_blob* blob_ref;

    old_options = SymGetOptions();
    im.SizeOfStruct = sizeof(im);

    len = GetTempPathW(ARRAY_SIZE(topdir), topdir);
    ok(len && len < ARRAY_SIZE(topdir), "Unexpected length\n");
    wcscat(topdir, L"dh.tmp\\");
    ret = CreateDirectoryW(topdir, NULL);
    ok(ret, "Couldn't create directory\n");
    for (i = 0; i < ARRAY_SIZE(top_subdirs); i++)
    {
        make_path(filename, topdir, top_subdirs[i], NULL);
        ret = CreateDirectoryW(filename, NULL);
        ok(ret, "Couldn't create directory %ls\n", filename);
    }

    init_headers64(&h.nt_header64, 12324, 3242, 0);
    blob_ref = make_pdb_ds_blob(12324, &guid1, 0x0030cafe, "bar.pdb");
    make_path(filename, topdir, NULL, L"bar.dll");
    create_test_dll(&h, sizeof(h.nt_header64), &blob_ref, 1, filename);

    for (i = 0; i < ARRAY_SIZE(module_path_tests); i++)
    {
        const struct module_path_test *test = module_path_tests + i;
        const char* ptr;

        winetest_push_context("module_path_test %d", i);

        /* setup debug info files */
        for (ptr = test->test_files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(test_files))
            {
                make_path(filename, topdir, NULL, test_files[val].module_path);
                if (test_files[val].guid)
                    create_test_pdb_ds(filename, test_files[val].guid, test_files[val].age_or_timestamp);
                else
                    /*create_test_dbg(filename, IMAGE_FILE_MACHINE_AMD64, 0x10E, test_files[val].age_or_timestamp, 0x40000 * val * 0x20000, blob); */
                    ok(0, "not supported yet\n");
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        if (test->search)
        {
            filename[0]= L'\0';
            for (ptr = test->search; *ptr; ptr++)
            {
                if (*filename) wcscat(filename, L";");
                wcscat(filename, topdir);
                switch (*ptr)
                {
                case 'm':
                    wcscat(filename, L"mismatch\\");
                    break;
                case 's':
                    wcscat(filename, L"search.tmp\\");
                    break;
                case 't':
                    break;
                default: assert(0);
                }
            }
        }
        SymSetOptions((SymGetOptions() & ~(SYMOPT_EXACT_SYMBOLS | SYMOPT_LOAD_ANYTHING)) | test->options);
        ret = SymInitializeW(dummy, test->search ? filename : NULL, FALSE);
        ok(ret, "SymInitialize failed: %lu\n", GetLastError());
        make_path(filename, topdir, NULL, L"bar.dll");
        base = SymLoadModuleExW(dummy, NULL, filename, NULL, 0x4000, 0x6666, NULL, 0);
        ok(base == 0x4000, "SymLoadModuleExW failed: %lu\n", GetLastError());
        im.SizeOfStruct = sizeof(im);
        ret = SymGetModuleInfoW64(dummy, base, &im);
        ok(ret, "SymGetModuleInfow64 failed: %lu\n", GetLastError());

        make_path(filename, topdir, NULL, L"bar.dll");
        ok(!wcscmp(im.LoadedImageName, filename),
                   "Expected %ls as loaded image file, got '%ls' instead\n", L"bar.dll", im.LoadedImageName);
        if (test->found_file == -1)
        {
            ok(im.SymType == SymNone, "Unexpected symtype %x\n", im.SymType);
            ok(!im.LoadedPdbName[0], "Expected empty loaded pdb file, got '%ls' instead\n", im.LoadedPdbName);
            ok(im.PdbAge == 0x0030cafe, "Expected %x as pdb-age, got %lx instead\n", 0x0030cafe, im.PdbAge);
            ok(!im.PdbUnmatched, "Expecting matched PDB\n");
        }
        else
        {
            ok(im.SymType == SymPdb, "Unexpected symtype %x\n", im.SymType);
            make_path(filename, topdir, NULL, test_files[test->found_file].module_path);
            ok(!wcscmp(im.LoadedPdbName, filename),
               "Expected %ls as loaded pdb file, got '%ls' instead\n", test_files[test->found_file].module_path, im.LoadedPdbName);
            ok(im.PdbAge == test_files[test->found_file].age_or_timestamp,
               "Expected %lx as pdb-age, got %lx instead\n",  test_files[test->found_file].age_or_timestamp, im.PdbAge);
            ok(im.PdbUnmatched == !(test_files[test->found_file].age_or_timestamp == 0x0030cafe), "Expecting matched PDB\n");
        }
        ok(IsEqualGUID(&im.PdbSig70, &guid1), "Unexpected PDB GUID\n");
        ret = SymCleanup(dummy);
        ok(ret, "SymCleanup failed: %lu\n", GetLastError());
        for (ptr = test->test_files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(test_files))
            {
                make_path(filename, topdir, NULL, test_files[val].module_path);
                ret = DeleteFileW(filename);
                ok(ret, "Couldn't delete file %c %ls\n", *ptr, filename);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }
        winetest_pop_context();
    }
    SymSetOptions(old_options);
    free(blob_ref);
    make_path(filename, topdir, NULL, L"bar.dll");
    ret = DeleteFileW(filename);
    ok(ret, "Couldn't delete file %ls\n", filename);
    for (i = ARRAY_SIZE(top_subdirs) - 1; i >= 0; i--)
    {
        make_path(filename, topdir, top_subdirs[i], NULL);
        ret = RemoveDirectoryW(filename);
        ok(ret, "Couldn't create directory %ls\n", filename);
    }
    ret = RemoveDirectoryW(topdir);
    ok(ret, "Couldn't remove directory\n");
}

struct module_details
{
    WCHAR*      name;
    unsigned    count;
};

static BOOL CALLBACK aggregate_module_details_cb(PCWSTR name, DWORD64 base, PVOID usr)
{
    struct module_details* md = usr;

    if (md->count == 0) md->name = wcsdup(name);
    md->count++;
    return TRUE;
}

static BOOL has_mismatch(const char *str, char ch, unsigned *val)
{
    if (str && *str == ch)
    {
        if (val && (ch == 'F' || ch == 'P' || ch == '!'))
            *val = char2index(str[1]);
        return TRUE;
    }
    return FALSE;
}

static void test_load_modules_details(void)
{
    static const struct debug_info_file
    {
        const GUID* guid; /* non-NULL means PDB/DS, DBG otherwise */
        DWORD       age_or_timestamp; /* age for PDB, timestamp for DBG */
        const WCHAR*module_path;
    }
    test_files[] =
    {
        /*  0 */ { &guid1, 0x0030cafe, L"bar.pdb" },
        /*  1 */ { NULL,   0xcafe0030, L"bar.dbg" },
    };

    static const struct module_details_test
    {
        /* input parameters */
        DWORD        flags;
        DWORD        options;
        const WCHAR *in_image_name;
        const WCHAR *in_module_name;
        int          blob_index; /* only when in_image_name is bar.dll */
        const char  *test_files; /* various test_files to be created */
        /* output parameters */
        SYM_TYPE     sym_type;
        const char   *mismatch_in; /* format if CN, where
                                      C={'F' for full match of PDB, 'P' for partial match of PDB, '!' found pdb without info}
                                      N index of expected file */
    }
    module_details_tests[] =
    {
/* 0*/  {SLMFLAG_VIRTUAL,    0,                     NULL,       NULL,       -1, "",  SymVirtual},
        {SLMFLAG_VIRTUAL,    0,                     NULL,       L"foo_bar", -1, "",  SymVirtual},
        {SLMFLAG_VIRTUAL,    0,                     L"foo.dll", L"foo_bar", -1, "",  SymVirtual},
        {SLMFLAG_VIRTUAL,    SYMOPT_DEFERRED_LOADS, L"foo.dll", L"foo_bar", -1, "",  SymDeferred},

        {SLMFLAG_NO_SYMBOLS, 0,                     L"bar.dll", L"foo_bar", -1, "0", SymNone},
/* 5*/  {SLMFLAG_NO_SYMBOLS, SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar", -1, "0", SymDeferred},

        {0,                  0,                     L"bar.dll", L"foo_bar", -1, "0", SymPdb,      "!0"},
        {0,                  SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar", -1, "0", SymDeferred},

        {SLMFLAG_NO_SYMBOLS, 0,                     L"bar.dll", L"foo_bar",  0, "0", SymNone,     "F0"},
        {SLMFLAG_NO_SYMBOLS, SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar",  0, "0", SymDeferred},

/*10*/  {0,                  0,                     L"bar.dll", L"foo_bar",  0, "0", SymPdb,      "F0"},
        {0,                  SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar",  0, "0", SymDeferred},

        {SLMFLAG_NO_SYMBOLS, 0,                     L"bar.dll", L"foo_bar",  1, "0", SymNone,     "P0"},
        {SLMFLAG_NO_SYMBOLS, SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar",  1, "0", SymDeferred},

        {0,                  0,                     L"bar.dll", L"foo_bar",  1, "0", SymNone,     "P0"},
/*15*/  {0,                  SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar",  1, "0", SymDeferred},

        {SLMFLAG_NO_SYMBOLS, 0,                     L"bar.dll", L"foo_bar", -1, "",  SymNone},
        {SLMFLAG_NO_SYMBOLS, SYMOPT_DEFERRED_LOADS, L"bar.dll", L"foo_bar", -1, "",  SymDeferred},

        /* FIXME add lookup path, exact symbol, .DBG files */
    };

    unsigned i;
    HANDLE dummy = (HANDLE)(ULONG_PTR)0xc4fef00d;
    DWORD old_options;
    IMAGEHLP_MODULEW64 im;
    BOOL ret;
    DWORD64 base;
    struct module_details md;
    WCHAR topdir[MAX_PATH];
    WCHAR filename[MAX_PATH];
    const WCHAR* loaded_img_name;
    WCHAR expected_module_name[32];
    WCHAR sym_name[128];
    char buffer[512];
    SYMBOL_INFOW* sym = (void*)buffer;
    DWORD len;

    union nt_header h;
    struct debug_directory_blob* blob_refs[2];

    old_options = SymGetOptions();

    len = GetTempPathW(ARRAY_SIZE(topdir), topdir);
    ok(len && len < ARRAY_SIZE(topdir), "Unexpected length\n");
    wcscat(topdir, L"dh.tmp\\");
    ret = CreateDirectoryW(topdir, NULL);
    ok(ret, "Couldn't create directory\n");

    init_headers64(&h.nt_header64, 12324, 3242, 0);
    blob_refs[0] = make_pdb_ds_blob(12324, &guid1, 0x0030cafe, "bar.pdb");
    blob_refs[1] = make_pdb_ds_blob(12325, &guid1, 0x0030caff, "bar.pdb"); /* shall generate a mismatch */

    for (i = 0; i < ARRAY_SIZE(module_details_tests); i++)
    {
        const struct module_details_test *test = module_details_tests + i;
        const char* ptr;

        winetest_push_context("module_test %d", i);

        /* setup debug info files */
        for (ptr = test->test_files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(test_files))
            {
                make_path(filename, topdir, NULL, test_files[val].module_path);
                if (test_files[val].guid)
                    create_test_pdb_ds(filename, test_files[val].guid, test_files[val].age_or_timestamp);
                else
                    create_test_dbg(filename, IMAGE_FILE_MACHINE_AMD64 /* FIXME */, 0x10E, test_files[val].age_or_timestamp, 0x40000 * val * 0x20000, NULL);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }

        /* set up image files (when needed) */
        if (test->in_image_name && !wcscmp(test->in_image_name, L"bar.dll"))
        {
            make_path(filename, topdir, NULL, test->in_image_name);
            if (test->blob_index >= 0)
            {
                ok(test->blob_index < ARRAY_SIZE(blob_refs), "Out of bounds blob %d\n", test->blob_index);
                create_test_dll(&h, sizeof(h.nt_header64), &blob_refs[test->blob_index], 1, filename);
            }
            else
                create_test_dll(&h, sizeof(h.nt_header64), NULL, 0, filename);
        }

        ret = SymInitializeW(dummy, topdir, FALSE);
        ok(ret, "SymInitialize failed: %lu\n", GetLastError());
        SymSetOptions((SymGetOptions() & ~SYMOPT_DEFERRED_LOADS) | test->options);
        base = SymLoadModuleExW(dummy, NULL,
                                test->in_image_name, test->in_module_name,
                                0x4000, 0x6666, NULL, test->flags);
        ok(base == 0x4000, "SymLoadModuleExW failed: %lu\n", GetLastError());
        memset(&im, 0xA5, sizeof(im));
        im.SizeOfStruct = sizeof(im);
        ret = SymGetModuleInfoW64(dummy, base, &im);
        ok(ret, "SymGetModuleInfow64 failed: %lu\n", GetLastError());
        if (test->in_image_name)
        {
            WCHAR *dot;
            wcscpy_s(expected_module_name, ARRAY_SIZE(expected_module_name), test->in_image_name);
            dot = wcsrchr(expected_module_name, L'.');
            if (dot) *dot = L'\0';
        }
        else
            expected_module_name[0] = L'\0';
        ok(!wcsicmp(im.ModuleName, expected_module_name), "Unexpected module name '%ls'\n", im.ModuleName);
        ok(!wcsicmp(im.ImageName, test->in_image_name ? test->in_image_name : L""), "Unexpected image name '%ls'\n", im.ImageName);
        if ((test->options & SYMOPT_DEFERRED_LOADS) || !test->in_image_name)
            loaded_img_name = L"";
        else if (!wcscmp(test->in_image_name, L"bar.dll"))
        {
            make_path(filename, topdir, NULL, L"bar.dll");
            loaded_img_name = filename;
        }
        else
            loaded_img_name = test->in_image_name;
        ok(!wcsicmp(im.LoadedImageName, (test->options & SYMOPT_DEFERRED_LOADS) ? L"" : loaded_img_name),
           "Unexpected loaded image name '%ls' (%ls)\n", im.LoadedImageName, loaded_img_name);
        ok(im.SymType == test->sym_type, "Unexpected module type %u\n", im.SymType);
        if (test->mismatch_in)
        {
            unsigned val;
            if (has_mismatch(test->mismatch_in, '!', &val))
            {
                ok(val < ARRAY_SIZE(test_files), "Incorrect index\n");
                make_path(filename, topdir, NULL, L"bar.pdb");
                ok(!wcscmp(filename, im.LoadedPdbName), "Unexpected value '%ls\n", im.LoadedPdbName);
                ok(im.PdbUnmatched, "Unexpected value\n");
                ok(!im.DbgUnmatched, "Unexpected value\n");
                ok(IsEqualGUID(&im.PdbSig70, test_files[val].guid), "Unexpected value %s %s\n",
                   wine_dbgstr_guid(&im.PdbSig70), wine_dbgstr_guid(test_files[val].guid));
                ok(im.PdbSig == 0, "Unexpected value\n");
                ok(im.PdbAge == test_files[val].age_or_timestamp, "Unexpected value\n");
            }
            else if (has_mismatch(test->mismatch_in, 'P', &val))
            {
                ok(val < ARRAY_SIZE(test_files), "Incorrect index\n");
                ok(!im.LoadedPdbName[0], "Unexpected value\n");
                ok(!im.PdbUnmatched, "Unexpected value\n");
                ok(!im.DbgUnmatched, "Unexpected value\n");
                ok(IsEqualGUID(&im.PdbSig70, test_files[val].guid), "Unexpected value %s %s\n",
                   wine_dbgstr_guid(&im.PdbSig70), wine_dbgstr_guid(test_files[val].guid));
                ok(im.PdbSig == 0, "Unexpected value\n");
                ok(im.PdbAge == test_files[val].age_or_timestamp + 1, "Unexpected value\n");
            }
            else if (has_mismatch(test->mismatch_in, 'F', &val))
            {
                ok(val < ARRAY_SIZE(test_files), "Incorrect index\n");
                if (test->flags & SLMFLAG_NO_SYMBOLS)
                    ok(!im.LoadedPdbName[0], "Unexpected value\n");
                else
                {
                    make_path(filename, topdir, NULL, L"bar.pdb");
                    ok(!wcscmp(im.LoadedPdbName, filename), "Unexpected value\n");
                }
                ok(!im.PdbUnmatched, "Unexpected value\n");
                ok(!im.DbgUnmatched, "Unexpected value\n");
                ok(IsEqualGUID(&im.PdbSig70, test_files[val].guid), "Unexpected value %s %s\n",
                   wine_dbgstr_guid(&im.PdbSig70), wine_dbgstr_guid(test_files[val].guid));
                ok(im.PdbSig == 0, "Unexpected value\n");
                ok(im.PdbAge == test_files[val].age_or_timestamp, "Unexpected value\n");
            }
            ok(im.TimeDateStamp == 12324, "Unexpected value\n");
        }
        else
        {
            ok(!im.LoadedPdbName[0], "Unexpected value3 %ls\n", im.LoadedPdbName);
            ok(!im.PdbUnmatched, "Unexpected value\n");
            ok(!im.DbgUnmatched, "Unexpected value\n");
            ok(IsEqualGUID(&im.PdbSig70, &null_guid), "Unexpected value %s\n", wine_dbgstr_guid(&im.PdbSig70));
            ok(im.PdbSig == 0, "Unexpected value\n");
            ok(!im.PdbAge, "Unexpected value\n");
            /* native returns either 0 or the actual timestamp depending on test case */
            ok(!im.TimeDateStamp || broken(im.TimeDateStamp == 12324), "Unexpected value\n");
        }
        ok(im.ImageSize == 0x6666, "Unexpected image size\n");
        memset(&md, 0, sizeof(md));
        ret = SymEnumerateModulesW64(dummy, aggregate_module_details_cb, &md);
        ok(ret, "SymEnumerateModules64 failed: %lu\n", GetLastError());
        ok(md.count == 1, "Unexpected module count %u\n", md.count);
        ok(!wcscmp(md.name, expected_module_name), "Unexpected module name %ls\n", md.name);
        free(md.name);
        /* native will fail loading symbol in deferred state, so force loading of debug symbols */
        if (im.SymType == SymDeferred)
        {
             memset(sym, 0, sizeof(*sym));
             sym->SizeOfStruct = sizeof(*sym);
             sym->MaxNameLen = (sizeof(buffer) - sizeof(*sym)) / sizeof(WCHAR);
             SymFromNameW(dummy, L"foo", sym);
        }
        ret = SymAddSymbol(dummy, base, "winetest_symbol_virtual", base + 4242, 13, 0);
        ok(ret, "Failed to add symbol\n");
        memset(sym, 0, sizeof(*sym));
        sym->SizeOfStruct = sizeof(*sym);
        sym->MaxNameLen = (sizeof(buffer) - sizeof(*sym)) / sizeof(WCHAR);

        swprintf(sym_name, ARRAY_SIZE(sym_name), L"%ls!%s", im.ModuleName, L"winetest_symbol_virtual");
        ret = SymFromNameW(dummy, sym_name, (void*)sym);
        ok(ret, "Couldn't find symbol %ls\n", sym_name);
        if (test->in_module_name)
        {
            swprintf(sym_name, ARRAY_SIZE(sym_name), L"%ls!%s", test->in_module_name, L"winetest_symbol_virtual");
            ret = SymFromNameW(dummy, sym_name, (void*)sym);
            ok(ret, "Couldn't find symbol %ls\n", sym_name);
        }
        ret = SymCleanup(dummy);
        ok(ret, "SymCleanup failed: %lu\n", GetLastError());
        for (ptr = test->test_files; *ptr; ptr++)
        {
            unsigned val = char2index(*ptr);
            if (val < ARRAY_SIZE(test_files))
            {
                make_path(filename, topdir, NULL, test_files[val].module_path);
                ret = DeleteFileW(filename);
                ok(ret, "Couldn't delete file %c %ls\n", *ptr, filename);
            }
            else ok(0, "Unrecognized file reference %c\n", *ptr);
        }
        if (test->in_image_name && !wcscmp(test->in_image_name, L"bar.dll"))
        {
            make_path(filename, topdir, NULL, test->in_image_name);
            ret = DeleteFileW(filename);
            ok(ret, "Couldn't delete file %c %ls\n", *ptr, filename);
        }

        winetest_pop_context();
    }
    SymSetOptions(old_options);
    for (i = 0; i < ARRAY_SIZE(blob_refs); i++) free(blob_refs[i]);
    ret = RemoveDirectoryW(topdir);
    ok(ret, "Couldn't remove directory\n");
}

static BOOL skip_too_old_dbghelp(void)
{
    IMAGEHLP_MODULEW64 im64 = {sizeof(im64)};
    IMAGEHLP_MODULE im0 = {sizeof(im0)};
    BOOL will_skip = TRUE;

    if (!strcmp(winetest_platform, "wine")) return FALSE;
    if (SymInitialize(GetCurrentProcess(), NULL, FALSE))
    {
        DWORD64 base = SymLoadModule(GetCurrentProcess(), NULL, "c:\\windows\\system32\\ntdll.dll", NULL, 0x4000, 0);
        /* test if get module info succeeds with oldest structure format */
        if (base)
            will_skip = !SymGetModuleInfoW64(GetCurrentProcess(), base, &im64) &&
                SymGetModuleInfo(GetCurrentProcess(), base, &im0);
        SymCleanup(GetCurrentProcess());
    }
    return will_skip;
}

START_TEST(path)
{
    /* cleanup env variables that affect dbghelp's behavior */
    SetEnvironmentVariableW(L"_NT_SYMBOL_PATH", NULL);
    SetEnvironmentVariableW(L"_NT_ALT_SYMBOL_PATH", NULL);

    test_srvgetindexes_pe();
    test_srvgetindexes_pdb();
    test_srvgetindexes_dbg();
    test_find_in_path_pe();
    test_find_in_path_pdb();

    if (skip_too_old_dbghelp())
        win_skip("Not testing on too old dbghelp version\n");
    else
    {
        test_load_modules_path();
        test_load_modules_details();
    }
}
