/*
 * Copyright 2023 Eric Pouech for Codeweavers
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

static void test_srvgetindexes(void)
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
        BOOL in_error;
    }
    indexes[] =
    {
        /* error cases */
/*  0 */{0,                         {-1, -1, -1}, .in_error = TRUE},
        {IMAGE_FILE_DEBUG_STRIPPED, { 0, -1, -1}, .in_error = TRUE},
        {IMAGE_FILE_DEBUG_STRIPPED, { 1, -1, -1}, .in_error = TRUE},
        {IMAGE_FILE_DEBUG_STRIPPED, { 2, -1, -1}, .in_error = TRUE},
        {IMAGE_FILE_DEBUG_STRIPPED, {-1, -1, -1}, .in_error = TRUE}, /* not 100% logical ! */
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
            if (indexes[i].in_error)
            {
                ok(!ret, "SymSrvGetFileIndexInfo should have failed\n");
                ok(GetLastError() == ERROR_BAD_EXE_FORMAT, "Mismatch in GetLastError: %lu\n", GetLastError());
            }
            else
            {
                ok(ret, "SymSrvGetFileIndexInfo failed: %lu\n", GetLastError());

                ok(ssii.age == indexes[i].age, "Mismatch in age: %lx\n", ssii.age);
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

START_TEST(path)
{
    /* cleanup env variables that affect dbghelp's behavior */
    SetEnvironmentVariableW(L"_NT_SYMBOL_PATH", NULL);
    SetEnvironmentVariableW(L"_NT_ALT_SYMBOL_PATH", NULL);

    test_srvgetindexes();
}
