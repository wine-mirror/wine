/*
 * Unit test suite for the PE loader.
 *
 * Copyright 2006 Dmitry Timoshkov
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

#include <stdarg.h>
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "wine/test.h"

#define ALIGN_SIZE(size, alignment) (((size) + (alignment - 1)) & ~((alignment - 1)))

static const struct
{
    WORD e_magic;      /* 00: MZ Header signature */
    WORD unused[29];
    DWORD e_lfanew;    /* 3c: Offset to extended header */
} dos_header =
{
    IMAGE_DOS_SIGNATURE, { 0 }, sizeof(dos_header)
};

static IMAGE_NT_HEADERS nt_header =
{
    IMAGE_NT_SIGNATURE, /* Signature */
#ifdef __i386__
    { IMAGE_FILE_MACHINE_I386, /* Machine */
#else
# error You must specify the machine type
#endif
      1, /* NumberOfSections */
      0, /* TimeDateStamp */
      0, /* PointerToSymbolTable */
      0, /* NumberOfSymbols */
      sizeof(IMAGE_OPTIONAL_HEADER), /* SizeOfOptionalHeader */
      IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_DLL /* Characteristics */
    },
    { IMAGE_NT_OPTIONAL_HDR_MAGIC, /* Magic */
      1, /* MajorLinkerVersion */
      0, /* MinorLinkerVersion */
      0, /* SizeOfCode */
      0, /* SizeOfInitializedData */
      0, /* SizeOfUninitializedData */
      0, /* AddressOfEntryPoint */
      0, /* BaseOfCode */
      0, /* BaseOfData */
      0x10000000, /* ImageBase */
      0, /* SectionAlignment */
      0, /* FileAlignment */
      4, /* MajorOperatingSystemVersion */
      0, /* MinorOperatingSystemVersion */
      1, /* MajorImageVersion */
      0, /* MinorImageVersion */
      4, /* MajorSubsystemVersion */
      0, /* MinorSubsystemVersion */
      0, /* Win32VersionValue */
      sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000, /* SizeOfImage */
      sizeof(dos_header) + sizeof(nt_header), /* SizeOfHeaders */
      0, /* CheckSum */
      IMAGE_SUBSYSTEM_WINDOWS_CUI, /* Subsystem */
      0, /* DllCharacteristics */
      0, /* SizeOfStackReserve */
      0, /* SizeOfStackCommit */
      0, /* SizeOfHeapReserve */
      0, /* SizeOfHeapCommit */
      0, /* LoaderFlags */
      0, /* NumberOfRvaAndSizes */
      { { 0 } } /* DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] */
    }
};

static IMAGE_SECTION_HEADER section =
{
    ".rodata", /* Name */
    { 0x10 }, /* Misc */
    0, /* VirtualAddress */
    0x10, /* SizeOfRawData */
    0, /* PointerToRawData */
    0, /* PointerToRelocations */
    0, /* PointerToLinenumbers */
    0, /* NumberOfRelocations */
    0, /* NumberOfLinenumbers */
    IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ, /* Characteristics */
};

START_TEST(loader)
{
    static const struct test_data
    {
        WORD number_of_sections, size_of_optional_header;
        DWORD section_alignment, file_alignment;
        DWORD size_of_image, size_of_headers;
        DWORD error; /* 0 means LoadLibrary should succeed */
    } td[] =
    {
        { 1, 0, 0, 0, 0, 0,
          ERROR_BAD_EXE_FORMAT
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0xe00,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_BAD_EXE_FORMAT /* XP doesn't like too small image size */
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x1000,
          0x1f00,
          0x1000,
          ERROR_SUCCESS
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x200, 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_BAD_EXE_FORMAT /* XP doesn't like aligments */
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS
        },
        { 1, sizeof(IMAGE_OPTIONAL_HEADER), 0x1000, 0x200,
          sizeof(dos_header) + sizeof(nt_header) + sizeof(IMAGE_SECTION_HEADER) + 0x1000,
          0x200,
          ERROR_SUCCESS
        },
        /* Mandatory are all fields up to SizeOfHeaders, everything else
         * is really optional (at least that's true for XP).
         */
        { 1, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER) + 0x10,
          sizeof(dos_header) + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum) + sizeof(IMAGE_SECTION_HEADER),
          ERROR_SUCCESS
        },
        { 0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0xd0, /* beyond of the end of file */
          0xc0, /* beyond of the end of file */
          ERROR_SUCCESS
        },
        { 0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0x1000,
          0,
          ERROR_SUCCESS
        },
        { 0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          1,
          0,
          ERROR_SUCCESS
        },
        { 0, FIELD_OFFSET(IMAGE_OPTIONAL_HEADER, CheckSum), 0x200, 0x200,
          0,
          0,
          ERROR_BAD_EXE_FORMAT /* image size == 0 -> failure */
        }
    };
    static const char filler[0x1000];
    int i;
    DWORD dummy, file_size, file_align;
    HANDLE hfile, hlib;
    SYSTEM_INFO si;
    char temp_path[MAX_PATH];
    char dll_name[MAX_PATH];

    GetSystemInfo(&si);
    trace("system page size 0x%04x\n", si.dwPageSize);

    /* prevent displaying of the "Unable to load this DLL" message box */
    SetErrorMode(SEM_FAILCRITICALERRORS);

    GetTempPath(MAX_PATH, temp_path);
    GetTempFileName(temp_path, "ldr", 0, dll_name);

    for (i = 0; i < sizeof(td)/sizeof(td[0]); i++)
    {
        /*trace("creating %s\n", dll_name);*/
        hfile = CreateFileA(dll_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
        if (hfile == INVALID_HANDLE_VALUE)
        {
            ok(0, "could not create %s\n", dll_name);
            break;
        }

        SetLastError(0xdeadbeef);
        ok(WriteFile(hfile, &dos_header, sizeof(dos_header), &dummy, NULL),
           "WriteFile error %d\n", GetLastError());

        nt_header.FileHeader.NumberOfSections = td[i].number_of_sections;
        nt_header.FileHeader.SizeOfOptionalHeader = td[i].size_of_optional_header;

        nt_header.OptionalHeader.SectionAlignment = td[i].section_alignment;
        nt_header.OptionalHeader.FileAlignment = td[i].file_alignment;
        nt_header.OptionalHeader.SizeOfImage = td[i].size_of_image;
        nt_header.OptionalHeader.SizeOfHeaders = td[i].size_of_headers;
        SetLastError(0xdeadbeef);
        ok(WriteFile(hfile, &nt_header, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &dummy, NULL),
           "WriteFile error %d\n", GetLastError());

        if (nt_header.FileHeader.SizeOfOptionalHeader)
        {
            assert(nt_header.FileHeader.SizeOfOptionalHeader <= sizeof(IMAGE_OPTIONAL_HEADER));

            SetLastError(0xdeadbeef);
            ok(WriteFile(hfile, &nt_header.OptionalHeader, nt_header.FileHeader.SizeOfOptionalHeader, &dummy, NULL),
               "WriteFile error %d\n", GetLastError());
        }

        assert(nt_header.FileHeader.NumberOfSections <= 1);
        if (nt_header.FileHeader.NumberOfSections)
        {
            if (nt_header.OptionalHeader.SectionAlignment == si.dwPageSize)
            {
                section.VirtualAddress = nt_header.OptionalHeader.SectionAlignment;
                section.PointerToRawData = nt_header.OptionalHeader.FileAlignment;
                SetLastError(0xdeadbeef);
                ok(WriteFile(hfile, &section, sizeof(section), &dummy, NULL),
                   "WriteFile error %d\n", GetLastError());

                file_size = GetFileSize(hfile, NULL);

                file_align = ALIGN_SIZE(file_size, nt_header.OptionalHeader.FileAlignment) - file_size;
                SetLastError(0xdeadbeef);
                ok(WriteFile(hfile, filler, file_align, &dummy, NULL),
                   "WriteFile error %d\n", GetLastError());
            }
            else
            {
                section.VirtualAddress = nt_header.OptionalHeader.SizeOfHeaders;
                section.PointerToRawData = nt_header.OptionalHeader.SizeOfHeaders;
                SetLastError(0xdeadbeef);
                ok(WriteFile(hfile, &section, sizeof(section), &dummy, NULL),
                   "WriteFile error %d\n", GetLastError());
            }

            /* section data */
            SetLastError(0xdeadbeef);
            ok(WriteFile(hfile, filler, 0x10, &dummy, NULL),
               "WriteFile error %d\n", GetLastError());
        }

        CloseHandle(hfile);

        SetLastError(0xdeadbeef);
        hlib = LoadLibrary(dll_name);
        if (td[i].error == ERROR_SUCCESS)
        {
            MEMORY_BASIC_INFORMATION info;

            ok(hlib != 0, "%d: LoadLibrary error %d\n", i, GetLastError());

            SetLastError(0xdeadbeef);
            ok(VirtualQuery(hlib, &info, sizeof(info)) == sizeof(info),
                "%d: VirtualQuery error %d\n", i, GetLastError());
            ok(info.BaseAddress == hlib, "%d: %p != %p\n", i, info.BaseAddress, hlib);
            ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
            ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
            ok(info.RegionSize == ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize), "%d: got %lx != expected %x\n",
               i, info.RegionSize, ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize));
            ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
            if (nt_header.OptionalHeader.SectionAlignment != si.dwPageSize)
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
            else
                ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
            ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);

            SetLastError(0xdeadbeef);
            ok(VirtualQuery((char *)hlib + info.RegionSize, &info, sizeof(info)) == sizeof(info),
                "%d: VirtualQuery error %d\n", i, GetLastError());
            if (nt_header.OptionalHeader.SectionAlignment == si.dwPageSize ||
                nt_header.OptionalHeader.SectionAlignment == nt_header.OptionalHeader.FileAlignment)
            {
                ok(info.BaseAddress == (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize), "%d: %p != %p\n",
                   i, info.BaseAddress, (char *)hlib + ALIGN_SIZE(nt_header.OptionalHeader.SizeOfImage, si.dwPageSize));
                ok(info.AllocationBase == 0, "%d: %p != 0\n", i, info.AllocationBase);
                ok(info.AllocationProtect == 0, "%d: %x != 0\n", i, info.AllocationProtect);
                /*ok(info.RegionSize == not_practical_value, "%d: %lx != not_practical_value\n", i, info.RegionSize);*/
                ok(info.State == MEM_FREE, "%d: %x != MEM_FREE\n", i, info.State);
                ok(info.Type == 0, "%d: %x != 0\n", i, info.Type);
                ok(info.Protect == PAGE_NOACCESS, "%d: %x != PAGE_NOACCESS\n", i, info.Protect);
            }
            else
            {
                ok(info.Protect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.Protect);
                ok(info.BaseAddress == hlib, "%d: %p != %p\n", i, info.BaseAddress, hlib);
                ok(info.AllocationBase == hlib, "%d: %p != %p\n", i, info.AllocationBase, hlib);
                ok(info.AllocationProtect == PAGE_EXECUTE_WRITECOPY, "%d: %x != PAGE_EXECUTE_WRITECOPY\n", i, info.AllocationProtect);
                ok(info.RegionSize == 0x2000, "%d: %lx != 0x2000\n", i, info.RegionSize);
                ok(info.State == MEM_COMMIT, "%d: %x != MEM_COMMIT\n", i, info.State);
                ok(info.Protect == PAGE_READONLY, "%d: %x != PAGE_READONLY\n", i, info.Protect);
                ok(info.Type == SEC_IMAGE, "%d: %x != SEC_IMAGE\n", i, info.Type);
            }

            SetLastError(0xdeadbeef);
            ok(FreeLibrary(hlib), "FreeLibrary error %d\n", GetLastError());
        }
        else
        {   /* LoadLibrary has failed */
            ok(!hlib, "%d: LoadLibrary should fail\n", i);

            if (GetLastError() == ERROR_GEN_FAILURE) /* Win9x, broken behaviour */
            {
                trace("skipping the loader test on Win9x\n");
                DeleteFile(dll_name);
                return;
            }

            ok(td[i].error == GetLastError(), "%d: expected error %d, got %d\n",
               i, td[i].error, GetLastError());
        }

        SetLastError(0xdeadbeef);
        ok(DeleteFile(dll_name), "DeleteFile error %d\n", GetLastError());
    }
}
