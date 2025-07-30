/*
 *	PE dumping utility
 *
 * 	Copyright 2001 Eric Pouech
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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#include "windef.h"
#include "winbase.h"
#include "verrsrc.h"
#include "winedump.h"

#define IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE 0x0010 /* Wine extension */

static const IMAGE_NT_HEADERS32*        PE_nt_headers;

static const char builtin_signature[] = "Wine builtin DLL";
static const char fakedll_signature[] = "Wine placeholder DLL";
static int is_builtin;

const char *get_machine_str(int mach)
{
    switch (mach)
    {
    case IMAGE_FILE_MACHINE_UNKNOWN:	return "Unknown";
    case IMAGE_FILE_MACHINE_I386:	return "i386";
    case IMAGE_FILE_MACHINE_R3000:	return "R3000";
    case IMAGE_FILE_MACHINE_R4000:	return "R4000";
    case IMAGE_FILE_MACHINE_R10000:	return "R10000";
    case IMAGE_FILE_MACHINE_ALPHA:	return "Alpha";
    case IMAGE_FILE_MACHINE_POWERPC:	return "PowerPC";
    case IMAGE_FILE_MACHINE_AMD64:      return "AMD64";
    case IMAGE_FILE_MACHINE_IA64:       return "IA64";
    case IMAGE_FILE_MACHINE_ARM64:      return "ARM64";
    case IMAGE_FILE_MACHINE_ARM:        return "ARM";
    case IMAGE_FILE_MACHINE_ARMNT:      return "ARMNT";
    case IMAGE_FILE_MACHINE_THUMB:      return "ARM Thumb";
    case IMAGE_FILE_MACHINE_ALPHA64:	return "Alpha64";
    case IMAGE_FILE_MACHINE_CHPE_X86:	return "CHPE-x86";
    case IMAGE_FILE_MACHINE_ARM64EC:	return "ARM64EC";
    case IMAGE_FILE_MACHINE_ARM64X:	return "ARM64X";
    case IMAGE_FILE_MACHINE_RISCV32:	return "RISC-V 32-bit";
    case IMAGE_FILE_MACHINE_RISCV64:	return "RISC-V 64-bit";
    case IMAGE_FILE_MACHINE_RISCV128:	return "RISC-V 128-bit";
    }
    return "???";
}

static const void*	RVA(unsigned long rva, unsigned long len)
{
    IMAGE_SECTION_HEADER*	sectHead;
    int				i;

    if (rva == 0) return NULL;

    sectHead = IMAGE_FIRST_SECTION(PE_nt_headers);
    for (i = PE_nt_headers->FileHeader.NumberOfSections - 1; i >= 0; i--)
    {
        if (sectHead[i].VirtualAddress <= rva &&
            rva + len <= (DWORD)sectHead[i].VirtualAddress + sectHead[i].SizeOfRawData)
        {
            /* return image import directory offset */
            return PRD(sectHead[i].PointerToRawData + rva - sectHead[i].VirtualAddress, len);
        }
    }

    return NULL;
}

static const IMAGE_NT_HEADERS32 *get_nt_header( void )
{
    const IMAGE_DOS_HEADER *dos;
    dos = PRD(0, sizeof(*dos));
    if (!dos) return NULL;
    is_builtin = (dos->e_lfanew >= sizeof(*dos) + 32 &&
                  !memcmp( dos + 1, builtin_signature, sizeof(builtin_signature) ));
    return PRD(dos->e_lfanew, sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
}

void print_fake_dll( void )
{
    const IMAGE_DOS_HEADER *dos;

    dos = PRD(0, sizeof(*dos) + 32);
    if (dos && dos->e_lfanew >= sizeof(*dos) + 32)
    {
        if (!memcmp( dos + 1, builtin_signature, sizeof(builtin_signature) ))
            printf( "*** This is a Wine builtin DLL ***\n\n" );
        else if (!memcmp( dos + 1, fakedll_signature, sizeof(fakedll_signature) ))
            printf( "*** This is a Wine fake DLL ***\n\n" );
    }
}

static const void *get_data_dir(const IMAGE_NT_HEADERS32 *hdr, unsigned int idx, unsigned int *size)
{
    if(hdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_OPTIONAL_HEADER64 *opt = (const IMAGE_OPTIONAL_HEADER64*)&hdr->OptionalHeader;
        if (idx >= opt->NumberOfRvaAndSizes)
            return NULL;
        if(size)
            *size = opt->DataDirectory[idx].Size;
        return RVA(opt->DataDirectory[idx].VirtualAddress,
                   opt->DataDirectory[idx].Size);
    }
    else
    {
        const IMAGE_OPTIONAL_HEADER32 *opt = (const IMAGE_OPTIONAL_HEADER32*)&hdr->OptionalHeader;
        if (idx >= opt->NumberOfRvaAndSizes)
            return NULL;
        if(size)
            *size = opt->DataDirectory[idx].Size;
        return RVA(opt->DataDirectory[idx].VirtualAddress,
                   opt->DataDirectory[idx].Size);
    }
}

static const void *get_dir_and_size(unsigned int idx, unsigned int *size)
{
    return get_data_dir( PE_nt_headers, idx, size );
}

static	const void*	get_dir(unsigned idx)
{
    return get_dir_and_size(idx, 0);
}

static const char * const DirectoryNames[16] = {
    "EXPORT",		"IMPORT",	"RESOURCE", 	"EXCEPTION",
    "SECURITY", 	"BASERELOC", 	"DEBUG", 	"ARCHITECTURE",
    "GLOBALPTR", 	"TLS", 		"LOAD_CONFIG",	"Bound IAT",
    "IAT", 		"Delay IAT",	"CLR Header", ""
};

static const char *get_magic_type(WORD magic)
{
    switch(magic) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            return "32bit";
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            return "64bit";
        case IMAGE_ROM_OPTIONAL_HDR_MAGIC:
            return "ROM";
    }
    return "???";
}

static const void *get_hybrid_metadata(void)
{
    unsigned int size;

    if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_LOAD_CONFIG_DIRECTORY64 *cfg = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size);
        if (!cfg) return 0;
        size = min( size, cfg->Size );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, CHPEMetadataPointer )) return 0;
        if (!cfg->CHPEMetadataPointer) return 0;
        return RVA( cfg->CHPEMetadataPointer - ((const IMAGE_OPTIONAL_HEADER64 *)&PE_nt_headers->OptionalHeader)->ImageBase, 1 );
    }
    else
    {
        const IMAGE_LOAD_CONFIG_DIRECTORY32 *cfg = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size);
        if (!cfg) return 0;
        size = min( size, cfg->Size );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, CHPEMetadataPointer )) return 0;
        if (!cfg->CHPEMetadataPointer) return 0;
        return RVA( cfg->CHPEMetadataPointer - PE_nt_headers->OptionalHeader.ImageBase, 1 );
    }
}

static inline const char *longlong_str( ULONGLONG value )
{
    static char buffer[20];

    if (sizeof(value) > sizeof(unsigned long) && value >> 32)
        sprintf(buffer, "%lx%08lx", (unsigned long)(value >> 32), (unsigned long)value);
    else
        sprintf(buffer, "%lx", (unsigned long)value);
    return buffer;
}

static inline void print_word(const char *title, WORD value)
{
    printf("  %-34s 0x%-4X         %u\n", title, value, value);
}

static inline void print_dword(const char *title, UINT value)
{
    printf("  %-34s 0x%-8x     %u\n", title, value, value);
}

static inline void print_longlong(const char *title, ULONGLONG value)
{
    printf("  %-34s 0x%s\n", title, longlong_str(value));
}

static inline void print_ver(const char *title, BYTE major, BYTE minor)
{
    printf("  %-34s %u.%02u\n", title, major, minor);
}

static inline void print_subsys(const char *title, WORD value)
{
    const char *str;
    switch (value)
    {
        default:
        case IMAGE_SUBSYSTEM_UNKNOWN:       str = "Unknown";        break;
        case IMAGE_SUBSYSTEM_NATIVE:        str = "Native";         break;
        case IMAGE_SUBSYSTEM_WINDOWS_GUI:   str = "Windows GUI";    break;
        case IMAGE_SUBSYSTEM_WINDOWS_CUI:   str = "Windows CUI";    break;
        case IMAGE_SUBSYSTEM_OS2_CUI:       str = "OS/2 CUI";       break;
        case IMAGE_SUBSYSTEM_POSIX_CUI:     str = "Posix CUI";      break;
        case IMAGE_SUBSYSTEM_NATIVE_WINDOWS:           str = "native Win9x driver";  break;
        case IMAGE_SUBSYSTEM_WINDOWS_CE_GUI:           str = "Windows CE GUI";       break;
        case IMAGE_SUBSYSTEM_EFI_APPLICATION:          str = "EFI application";      break;
        case IMAGE_SUBSYSTEM_EFI_BOOT_SERVICE_DRIVER:  str = "EFI driver (boot)";    break;
        case IMAGE_SUBSYSTEM_EFI_RUNTIME_DRIVER:       str = "EFI driver (runtime)"; break;
        case IMAGE_SUBSYSTEM_EFI_ROM:                  str = "EFI ROM";              break;
        case IMAGE_SUBSYSTEM_XBOX:                     str = "Xbox application";     break;
        case IMAGE_SUBSYSTEM_WINDOWS_BOOT_APPLICATION: str = "Boot application";     break;
    }
    printf("  %-34s 0x%X (%s)\n", title, value, str);
}

static inline void print_dllflags(const char *title, WORD value)
{
    printf("  %-34s 0x%04X\n", title, value);
#define X(f,s) do { if (value & f) printf("    %s\n", s); } while(0)
    if (is_builtin) X(IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE, "PREFER_NATIVE (Wine extension)");
    X(IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA,       "HIGH_ENTROPY_VA");
    X(IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE,          "DYNAMIC_BASE");
    X(IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY,       "FORCE_INTEGRITY");
    X(IMAGE_DLLCHARACTERISTICS_NX_COMPAT,             "NX_COMPAT");
    X(IMAGE_DLLCHARACTERISTICS_NO_ISOLATION,          "NO_ISOLATION");
    X(IMAGE_DLLCHARACTERISTICS_NO_SEH,                "NO_SEH");
    X(IMAGE_DLLCHARACTERISTICS_NO_BIND,               "NO_BIND");
    X(IMAGE_DLLCHARACTERISTICS_APPCONTAINER,          "APPCONTAINER");
    X(IMAGE_DLLCHARACTERISTICS_WDM_DRIVER,            "WDM_DRIVER");
    X(IMAGE_DLLCHARACTERISTICS_GUARD_CF,              "GUARD_CF");
    X(IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE, "TERMINAL_SERVER_AWARE");
#undef X
}

static inline void print_datadirectory(DWORD n, const IMAGE_DATA_DIRECTORY *directory)
{
    unsigned i;
    printf("Data Directory\n");

    for (i = 0; i < n && i < 16; i++)
    {
        printf("  %-12s rva: 0x%-8x  size: 0x%-8x\n",
               DirectoryNames[i], (UINT)directory[i].VirtualAddress,
               (UINT)directory[i].Size);
    }
}

static void dump_optional_header32(const IMAGE_OPTIONAL_HEADER32 *image_oh)
{
    IMAGE_OPTIONAL_HEADER32 oh;
    const IMAGE_OPTIONAL_HEADER32 *optionalHeader;

    /* in case optional header is missing or partial */
    memset(&oh, 0, sizeof(oh));
    memcpy(&oh, image_oh, min(dump_total_len - ((char *)image_oh - (char *)dump_base), sizeof(oh)));
    optionalHeader = &oh;

    print_word("Magic", optionalHeader->Magic);
    print_ver("linker version",
              optionalHeader->MajorLinkerVersion, optionalHeader->MinorLinkerVersion);
    print_dword("size of code", optionalHeader->SizeOfCode);
    print_dword("size of initialized data", optionalHeader->SizeOfInitializedData);
    print_dword("size of uninitialized data", optionalHeader->SizeOfUninitializedData);
    print_dword("entrypoint RVA", optionalHeader->AddressOfEntryPoint);
    print_dword("base of code", optionalHeader->BaseOfCode);
    print_dword("base of data", optionalHeader->BaseOfData);
    print_dword("image base", optionalHeader->ImageBase);
    print_dword("section align", optionalHeader->SectionAlignment);
    print_dword("file align", optionalHeader->FileAlignment);
    print_ver("required OS version",
              optionalHeader->MajorOperatingSystemVersion, optionalHeader->MinorOperatingSystemVersion);
    print_ver("image version",
              optionalHeader->MajorImageVersion, optionalHeader->MinorImageVersion);
    print_ver("subsystem version",
              optionalHeader->MajorSubsystemVersion, optionalHeader->MinorSubsystemVersion);
    print_dword("Win32 Version", optionalHeader->Win32VersionValue);
    print_dword("size of image", optionalHeader->SizeOfImage);
    print_dword("size of headers", optionalHeader->SizeOfHeaders);
    print_dword("checksum", optionalHeader->CheckSum);
    print_subsys("Subsystem", optionalHeader->Subsystem);
    print_dllflags("DLL characteristics:", optionalHeader->DllCharacteristics);
    print_dword("stack reserve size", optionalHeader->SizeOfStackReserve);
    print_dword("stack commit size", optionalHeader->SizeOfStackCommit);
    print_dword("heap reserve size", optionalHeader->SizeOfHeapReserve);
    print_dword("heap commit size", optionalHeader->SizeOfHeapCommit);
    print_dword("loader flags", optionalHeader->LoaderFlags);
    print_dword("RVAs & sizes", optionalHeader->NumberOfRvaAndSizes);
    printf("\n");
    print_datadirectory(optionalHeader->NumberOfRvaAndSizes, optionalHeader->DataDirectory);
    printf("\n");
}

static void dump_optional_header64(const IMAGE_OPTIONAL_HEADER64 *image_oh)
{
    IMAGE_OPTIONAL_HEADER64 oh;
    const IMAGE_OPTIONAL_HEADER64 *optionalHeader;

    /* in case optional header is missing or partial */
    memset(&oh, 0, sizeof(oh));
    memcpy(&oh, image_oh, min(dump_total_len - ((char *)image_oh - (char *)dump_base), sizeof(oh)));
    optionalHeader = &oh;

    print_word("Magic", optionalHeader->Magic);
    print_ver("linker version",
              optionalHeader->MajorLinkerVersion, optionalHeader->MinorLinkerVersion);
    print_dword("size of code", optionalHeader->SizeOfCode);
    print_dword("size of initialized data", optionalHeader->SizeOfInitializedData);
    print_dword("size of uninitialized data", optionalHeader->SizeOfUninitializedData);
    print_dword("entrypoint RVA", optionalHeader->AddressOfEntryPoint);
    print_dword("base of code", optionalHeader->BaseOfCode);
    print_longlong("image base", optionalHeader->ImageBase);
    print_dword("section align", optionalHeader->SectionAlignment);
    print_dword("file align", optionalHeader->FileAlignment);
    print_ver("required OS version",
              optionalHeader->MajorOperatingSystemVersion, optionalHeader->MinorOperatingSystemVersion);
    print_ver("image version",
              optionalHeader->MajorImageVersion, optionalHeader->MinorImageVersion);
    print_ver("subsystem version",
              optionalHeader->MajorSubsystemVersion, optionalHeader->MinorSubsystemVersion);
    print_dword("Win32 Version", optionalHeader->Win32VersionValue);
    print_dword("size of image", optionalHeader->SizeOfImage);
    print_dword("size of headers", optionalHeader->SizeOfHeaders);
    print_dword("checksum", optionalHeader->CheckSum);
    print_subsys("Subsystem", optionalHeader->Subsystem);
    print_dllflags("DLL characteristics:", optionalHeader->DllCharacteristics);
    print_longlong("stack reserve size", optionalHeader->SizeOfStackReserve);
    print_longlong("stack commit size", optionalHeader->SizeOfStackCommit);
    print_longlong("heap reserve size", optionalHeader->SizeOfHeapReserve);
    print_longlong("heap commit size", optionalHeader->SizeOfHeapCommit);
    print_dword("loader flags", optionalHeader->LoaderFlags);
    print_dword("RVAs & sizes", optionalHeader->NumberOfRvaAndSizes);
    printf("\n");
    print_datadirectory(optionalHeader->NumberOfRvaAndSizes, optionalHeader->DataDirectory);
    printf("\n");
}

void dump_optional_header(const IMAGE_OPTIONAL_HEADER32 *optionalHeader)
{
    printf("Optional Header (%s)\n", get_magic_type(optionalHeader->Magic));

    switch(optionalHeader->Magic) {
        case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
            dump_optional_header32(optionalHeader);
            break;
        case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
            dump_optional_header64((const IMAGE_OPTIONAL_HEADER64 *)optionalHeader);
            break;
        default:
            printf("  Unknown optional header magic: 0x%-4X\n", optionalHeader->Magic);
            break;
    }
}

void dump_file_header(const IMAGE_FILE_HEADER *fileHeader, BOOL is_hybrid)
{
    const char *name = get_machine_str(fileHeader->Machine);

    printf("File Header\n");

    if (is_hybrid)
    {
        switch (fileHeader->Machine)
        {
        case IMAGE_FILE_MACHINE_I386: name = "CHPE"; break;
        case IMAGE_FILE_MACHINE_AMD64: name = "ARM64EC"; break;
        case IMAGE_FILE_MACHINE_ARM64: name = "ARM64X"; break;
        }
    }
    printf("  Machine:                      %04X (%s)\n", fileHeader->Machine, name);
    printf("  Number of Sections:           %d\n", fileHeader->NumberOfSections);
    printf("  TimeDateStamp:                %08X (%s)\n",
	   (UINT)fileHeader->TimeDateStamp, get_time_str(fileHeader->TimeDateStamp));
    printf("  PointerToSymbolTable:         %08X\n", (UINT)fileHeader->PointerToSymbolTable);
    printf("  NumberOfSymbols:              %08X\n", (UINT)fileHeader->NumberOfSymbols);
    printf("  SizeOfOptionalHeader:         %04X\n", (UINT)fileHeader->SizeOfOptionalHeader);
    printf("  Characteristics:              %04X\n", (UINT)fileHeader->Characteristics);
#define	X(f,s)	if (fileHeader->Characteristics & f) printf("    %s\n", s)
    X(IMAGE_FILE_RELOCS_STRIPPED, 	"RELOCS_STRIPPED");
    X(IMAGE_FILE_EXECUTABLE_IMAGE, 	"EXECUTABLE_IMAGE");
    X(IMAGE_FILE_LINE_NUMS_STRIPPED, 	"LINE_NUMS_STRIPPED");
    X(IMAGE_FILE_LOCAL_SYMS_STRIPPED, 	"LOCAL_SYMS_STRIPPED");
    X(IMAGE_FILE_AGGRESIVE_WS_TRIM, 	"AGGRESIVE_WS_TRIM");
    X(IMAGE_FILE_LARGE_ADDRESS_AWARE, 	"LARGE_ADDRESS_AWARE");
    X(IMAGE_FILE_16BIT_MACHINE, 	"16BIT_MACHINE");
    X(IMAGE_FILE_BYTES_REVERSED_LO, 	"BYTES_REVERSED_LO");
    X(IMAGE_FILE_32BIT_MACHINE, 	"32BIT_MACHINE");
    X(IMAGE_FILE_DEBUG_STRIPPED, 	"DEBUG_STRIPPED");
    X(IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP, 	"REMOVABLE_RUN_FROM_SWAP");
    X(IMAGE_FILE_NET_RUN_FROM_SWAP, 	"NET_RUN_FROM_SWAP");
    X(IMAGE_FILE_SYSTEM, 		"SYSTEM");
    X(IMAGE_FILE_DLL, 			"DLL");
    X(IMAGE_FILE_UP_SYSTEM_ONLY, 	"UP_SYSTEM_ONLY");
    X(IMAGE_FILE_BYTES_REVERSED_HI, 	"BYTES_REVERSED_HI");
#undef X
    printf("\n");
}

static	void	dump_pe_header(void)
{
    dump_file_header(&PE_nt_headers->FileHeader, get_hybrid_metadata() != NULL);
    dump_optional_header((const IMAGE_OPTIONAL_HEADER32*)&PE_nt_headers->OptionalHeader);
}

void dump_section_characteristics(DWORD characteristics, const char* sep)
{
#define X(b,s)	if (characteristics & b) printf("%s%s", sep, s)
/* #define IMAGE_SCN_TYPE_REG			0x00000000 - Reserved */
/* #define IMAGE_SCN_TYPE_DSECT			0x00000001 - Reserved */
/* #define IMAGE_SCN_TYPE_NOLOAD		0x00000002 - Reserved */
/* #define IMAGE_SCN_TYPE_GROUP			0x00000004 - Reserved */
/* #define IMAGE_SCN_TYPE_NO_PAD		0x00000008 - Reserved */
/* #define IMAGE_SCN_TYPE_COPY			0x00000010 - Reserved */

    X(IMAGE_SCN_CNT_CODE, 		"CODE");
    X(IMAGE_SCN_CNT_INITIALIZED_DATA, 	"INITIALIZED_DATA");
    X(IMAGE_SCN_CNT_UNINITIALIZED_DATA, "UNINITIALIZED_DATA");

    X(IMAGE_SCN_LNK_OTHER, 		"LNK_OTHER");
    X(IMAGE_SCN_LNK_INFO, 		"LNK_INFO");
/* #define	IMAGE_SCN_TYPE_OVER		0x00000400 - Reserved */
    X(IMAGE_SCN_LNK_REMOVE, 		"LNK_REMOVE");
    X(IMAGE_SCN_LNK_COMDAT, 		"LNK_COMDAT");

/* 					0x00002000 - Reserved */
/* #define IMAGE_SCN_MEM_PROTECTED 	0x00004000 - Obsolete */
    X(IMAGE_SCN_MEM_FARDATA, 		"MEM_FARDATA");

/* #define IMAGE_SCN_MEM_SYSHEAP	0x00010000 - Obsolete */
    X(IMAGE_SCN_MEM_PURGEABLE, 		"MEM_PURGEABLE");
    X(IMAGE_SCN_MEM_16BIT, 		"MEM_16BIT");
    X(IMAGE_SCN_MEM_LOCKED, 		"MEM_LOCKED");
    X(IMAGE_SCN_MEM_PRELOAD, 		"MEM_PRELOAD");

    switch (characteristics & IMAGE_SCN_ALIGN_MASK)
    {
#define X2(b,s)	case b: printf("%s%s", sep, s); break
        X2(IMAGE_SCN_ALIGN_1BYTES, 	"ALIGN_1BYTES");
        X2(IMAGE_SCN_ALIGN_2BYTES, 	"ALIGN_2BYTES");
        X2(IMAGE_SCN_ALIGN_4BYTES, 	"ALIGN_4BYTES");
        X2(IMAGE_SCN_ALIGN_8BYTES, 	"ALIGN_8BYTES");
        X2(IMAGE_SCN_ALIGN_16BYTES, 	"ALIGN_16BYTES");
        X2(IMAGE_SCN_ALIGN_32BYTES, 	"ALIGN_32BYTES");
        X2(IMAGE_SCN_ALIGN_64BYTES, 	"ALIGN_64BYTES");
        X2(IMAGE_SCN_ALIGN_128BYTES, 	"ALIGN_128BYTES");
        X2(IMAGE_SCN_ALIGN_256BYTES, 	"ALIGN_256BYTES");
        X2(IMAGE_SCN_ALIGN_512BYTES, 	"ALIGN_512BYTES");
        X2(IMAGE_SCN_ALIGN_1024BYTES, 	"ALIGN_1024BYTES");
        X2(IMAGE_SCN_ALIGN_2048BYTES, 	"ALIGN_2048BYTES");
        X2(IMAGE_SCN_ALIGN_4096BYTES, 	"ALIGN_4096BYTES");
        X2(IMAGE_SCN_ALIGN_8192BYTES, 	"ALIGN_8192BYTES");
#undef X2
    }

    X(IMAGE_SCN_LNK_NRELOC_OVFL, 	"LNK_NRELOC_OVFL");

    X(IMAGE_SCN_MEM_DISCARDABLE, 	"MEM_DISCARDABLE");
    X(IMAGE_SCN_MEM_NOT_CACHED, 	"MEM_NOT_CACHED");
    X(IMAGE_SCN_MEM_NOT_PAGED, 		"MEM_NOT_PAGED");
    X(IMAGE_SCN_MEM_SHARED, 		"MEM_SHARED");
    X(IMAGE_SCN_MEM_EXECUTE, 		"MEM_EXECUTE");
    X(IMAGE_SCN_MEM_READ, 		"MEM_READ");
    X(IMAGE_SCN_MEM_WRITE, 		"MEM_WRITE");
#undef X
}

void dump_section(const IMAGE_SECTION_HEADER *sectHead, const char* strtable)
{
    unsigned offset;

    /* long section name ? */
    if (strtable && sectHead->Name[0] == '/' &&
        ((offset = atoi((const char*)sectHead->Name + 1)) < *(const DWORD*)strtable))
        printf("  %.8s (%s)", sectHead->Name, strtable + offset);
    else
        printf("  %-8.8s", sectHead->Name);
    printf("   VirtSize: 0x%08x  VirtAddr:  0x%08x\n",
           (UINT)sectHead->Misc.VirtualSize, (UINT)sectHead->VirtualAddress);
    printf("    raw data offs:   0x%08x  raw data size: 0x%08x\n",
           (UINT)sectHead->PointerToRawData, (UINT)sectHead->SizeOfRawData);
    printf("    relocation offs: 0x%08x  relocations:   0x%08x\n",
           (UINT)sectHead->PointerToRelocations, (UINT)sectHead->NumberOfRelocations);
    printf("    line # offs:     %-8u  line #'s:      %-8u\n",
           (UINT)sectHead->PointerToLinenumbers, (UINT)sectHead->NumberOfLinenumbers);
    printf("    characteristics: 0x%08x\n", (UINT)sectHead->Characteristics);
    printf("    ");
    dump_section_characteristics(sectHead->Characteristics, "  ");

    printf("\n\n");
}

static void dump_sections(const void *base, const void* addr, unsigned num_sect)
{
    const IMAGE_SECTION_HEADER*	sectHead = addr;
    unsigned			i;
    const char*                 strtable;

    if (PE_nt_headers && PE_nt_headers->FileHeader.PointerToSymbolTable && PE_nt_headers->FileHeader.NumberOfSymbols)
    {
        strtable = (const char*)base +
            PE_nt_headers->FileHeader.PointerToSymbolTable +
            PE_nt_headers->FileHeader.NumberOfSymbols * sizeof(IMAGE_SYMBOL);
    }
    else strtable = NULL;

    printf("Section Table\n");
    for (i = 0; i < num_sect; i++, sectHead++)
    {
        dump_section(sectHead, strtable);

        if (globals.do_dump_rawdata)
        {
            dump_data_offset((const unsigned char *)base + sectHead->PointerToRawData,
                             sectHead->SizeOfRawData, sectHead->VirtualAddress, "    " );
            printf("\n");
        }
    }
}

static char *get_str( char *buffer, unsigned int rva, unsigned int len )
{
    const WCHAR *wstr = PRD( rva, len );
    char *ret = buffer;

    len /= sizeof(WCHAR);
    while (len--) *buffer++ = *wstr++;
    *buffer = 0;
    return ret;
}

static void dump_section_apiset(void)
{
    const IMAGE_SECTION_HEADER *sect = IMAGE_FIRST_SECTION(PE_nt_headers);
    const UINT *ptr, *entry, *value, *hash;
    unsigned int i, j, count, val_count, rva;
    char buffer[128];

    for (i = 0; i < PE_nt_headers->FileHeader.NumberOfSections; i++, sect++)
    {
        if (strncmp( (const char *)sect->Name, ".apiset", 8 )) continue;
        rva = sect->PointerToRawData;
        ptr = PRD( rva, sizeof(*ptr) );
        printf( "ApiSet section:\n" );
        switch (ptr[0]) /* version */
        {
        case 2:
            printf( "  Version:     %u\n",   ptr[0] );
            printf( "  Count:       %08x\n", ptr[1] );
            count = ptr[1];
            if (!(entry = PRD( rva + 2 * sizeof(*ptr), count * 3 * sizeof(*entry) ))) break;
            for (i = 0; i < count; i++, entry += 3)
            {
                printf( "    %s ->", get_str( buffer, rva + entry[0], entry[1] ));
                if (!(value = PRD( rva + entry[2], sizeof(*value) ))) break;
                val_count = *value++;
                for (j = 0; j < val_count; j++, value += 4)
                {
                    putchar( ' ' );
                    if (value[1]) printf( "%s:", get_str( buffer, rva + value[0], value[1] ));
                    printf( "%s", get_str( buffer, rva + value[2], value[3] ));
                }
                printf( "\n");
            }
            break;
        case 4:
            printf( "  Version:     %u\n",   ptr[0] );
            printf( "  Size:        %08x\n", ptr[1] );
            printf( "  Flags:       %08x\n", ptr[2] );
            printf( "  Count:       %08x\n", ptr[3] );
            count = ptr[3];
            if (!(entry = PRD( rva + 4 * sizeof(*ptr), count * 6 * sizeof(*entry) ))) break;
            for (i = 0; i < count; i++, entry += 6)
            {
                printf( "    %08x %s ->", entry[0], get_str( buffer, rva + entry[1], entry[2] ));
                if (!(value = PRD( rva + entry[5], sizeof(*value) ))) break;
                value++; /* flags */
                val_count = *value++;
                for (j = 0; j < val_count; j++, value += 5)
                {
                    putchar( ' ' );
                    if (value[1]) printf( "%s:", get_str( buffer, rva + value[1], value[2] ));
                    printf( "%s", get_str( buffer, rva + value[3], value[4] ));
                }
                printf( "\n");
            }
            break;
        case 6:
            printf( "  Version:     %u\n",   ptr[0] );
            printf( "  Size:        %08x\n", ptr[1] );
            printf( "  Flags:       %08x\n", ptr[2] );
            printf( "  Count:       %08x\n", ptr[3] );
            printf( "  EntryOffset: %08x\n", ptr[4] );
            printf( "  HashOffset:  %08x\n", ptr[5] );
            printf( "  HashFactor:  %08x\n", ptr[6] );
            count = ptr[3];
            if (!(entry = PRD( rva + ptr[4], count * 6 * sizeof(*entry) ))) break;
            for (i = 0; i < count; i++, entry += 6)
            {
                printf( "    %08x %s ->", entry[0], get_str( buffer, rva + entry[1], entry[2] ));
                if (!(value = PRD( rva + entry[4], entry[5] * 5 * sizeof(*value) ))) break;
                for (j = 0; j < entry[5]; j++, value += 5)
                {
                    putchar( ' ' );
                    if (value[1]) printf( "%s:", get_str( buffer, rva + value[1], value[2] ));
                    printf( "%s", get_str( buffer, rva + value[3], value[4] ));
                }
                printf( "\n" );
            }
            printf( "  Hash table:\n" );
            if (!(hash = PRD( rva + ptr[5], count * 2 * sizeof(*hash) ))) break;
            for (i = 0; i < count; i++, hash += 2)
            {
                entry = PRD( rva + ptr[4] + hash[1] * 6 * sizeof(*entry), 6 * sizeof(*entry) );
                printf( "    %08x -> %s\n", hash[0], get_str( buffer, rva + entry[1], entry[3] ));
            }
            break;
        default:
            printf( "*** Unknown version %u\n", ptr[0] );
            break;
        }
        break;
    }
}

static const char *find_export_from_rva( UINT rva )
{
    UINT i, *func_names;
    const UINT *funcs;
    const UINT *names;
    const WORD *ordinals;
    const IMAGE_EXPORT_DIRECTORY *dir;
    const char *ret = NULL;

    if (!(dir = get_dir( IMAGE_FILE_EXPORT_DIRECTORY ))) return "";
    if (!(funcs = RVA( dir->AddressOfFunctions, dir->NumberOfFunctions * sizeof(DWORD) ))) return "";
    names = RVA( dir->AddressOfNames, dir->NumberOfNames * sizeof(DWORD) );
    ordinals = RVA( dir->AddressOfNameOrdinals, dir->NumberOfNames * sizeof(WORD) );
    func_names = calloc( dir->NumberOfFunctions, sizeof(*func_names) );

    for (i = 0; i < dir->NumberOfNames; i++) func_names[ordinals[i]] = names[i];
    for (i = 0; i < dir->NumberOfFunctions; i++)
    {
        if (funcs[i] != rva) continue;
        if (func_names[i]) ret = get_symbol_str( RVA( func_names[i], sizeof(DWORD) ));
        break;
    }

    free( func_names );
    if (!ret && rva == PE_nt_headers->OptionalHeader.AddressOfEntryPoint) return " <EntryPoint>";
    return ret ? strmake( " (%s)", ret ) : "";
}

static const char *find_import_from_rva( UINT rva )
{
    const IMAGE_IMPORT_DESCRIPTOR *imp;

    if (!(imp = get_dir( IMAGE_FILE_IMPORT_DIRECTORY ))) return "";

    /* check for import thunk */
    switch (PE_nt_headers->FileHeader.Machine)
    {
    case IMAGE_FILE_MACHINE_AMD64:
    {
        const BYTE *ptr = RVA( rva, 6 );
        if (!ptr) return "";
        if (ptr[0] == 0xff && ptr[1] == 0x25)
        {
            rva += 6 + *(int *)(ptr + 2);
            break;
        }
        if (ptr[0] == 0x48 && ptr[1] == 0xff && ptr[2] == 0x25)
        {
            rva += 7 + *(int *)(ptr + 3);
            break;
        }
        return "";
    }
    case IMAGE_FILE_MACHINE_ARM64:
    {
        const UINT *ptr = RVA( rva, sizeof(DWORD) );
        if ((ptr[0] & 0x9f00001f) == 0x90000010 &&  /* adrp x16, page */
            (ptr[1] & 0xffc003ff) == 0xf9400210 &&  /* ldr x16, [x16, #off] */
            ptr[2] == 0xd61f0200)                   /* br x16 */
        {
            rva &= ~0xfff;
            rva += ((ptr[0] & 0x00ffffe0) << 9) + ((ptr[0] & 0x60000000) >> 17);
            rva += (ptr[1] & 0x003ffc00) >> 7;
            break;
        }
        return "";
    }
    default:
        return "";
    }

    for ( ; imp->Name && imp->FirstThunk; imp++)
    {
        UINT imp_rva = imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk;
        UINT thunk_rva = imp->FirstThunk;

        if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        {
            const IMAGE_THUNK_DATA64 *il = RVA( imp_rva, sizeof(DWORD) );
            for ( ; il && il->u1.Ordinal; il++, thunk_rva += sizeof(LONGLONG))
            {
                if (thunk_rva != rva) continue;
                if (IMAGE_SNAP_BY_ORDINAL64(il->u1.Ordinal))
                    return strmake( " (-> #%u)", (WORD)IMAGE_ORDINAL64( il->u1.Ordinal ));
                else
                    return strmake( " (-> %s)", get_symbol_str( RVA( (DWORD)il->u1.AddressOfData + 2, 1 )));
            }
        }
        else
        {
            const IMAGE_THUNK_DATA32 *il = RVA( imp_rva, sizeof(DWORD) );
            for ( ; il && il->u1.Ordinal; il++, thunk_rva += sizeof(LONG))
            {
                if (thunk_rva != rva) continue;
                if (IMAGE_SNAP_BY_ORDINAL32(il->u1.Ordinal))
                    return strmake( " (-> #%u)", (WORD)IMAGE_ORDINAL32( il->u1.Ordinal ));
                else
                    return strmake( " (-> %s)", get_symbol_str( RVA( (DWORD)il->u1.AddressOfData + 2, 1 )));
            }
        }
    }
    return "";
}

static const char *get_function_name( UINT rva )
{
    const char *name = find_export_from_rva( rva );
    if (!*name) name = find_import_from_rva( rva );
    return name;
}

static int match_pattern( const BYTE *instr, const BYTE *pattern, int len )
{
    for (int i = 0; i < len; i++) if (pattern[i] && pattern[i] != instr[i]) return 0;
    return 1;
}

static void dump_export_flags( UINT rva )
{
    switch (PE_nt_headers->FileHeader.Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
    {
        static const BYTE patterns[][18] =
        {
            { 0xb8, 0, 0, 0, 0, 0xba, 0, 0, 0, 0, 0xff, 0xd2 }, /* >= win10 */
            { 0xb8, 0, 0, 0, 0, 0xba, 0, 0, 0, 0, 0xff, 0x12 }, /* winxp */
            { 0xb8, 0, 0, 0, 0, 0x64, 0xff, 0x15, 0xc0, 0, 0, 0 },  /* nt */
            { 0xb8, 0, 0, 0, 0, 0x8d, 0x54, 0x24, 0x04, 0xcd, 0x2e }, /* nt */
            { 0xb8, 0, 0, 0, 0, 0xb9, 0, 0, 0, 0, 0x8d, 0x54, 0x24, 0x04, 0x64, 0xff, 0x15, 0xc0 }, /* vista */
            { 0xb8, 0, 0, 0, 0, 0x33, 0xc9, 0x8d, 0x54, 0x24, 0x04, 0x64, 0xff, 0x15, 0xc0 }, /* vista */
            { 0xb8, 0, 0, 0, 0, 0xe8, 0, 0, 0, 0, 0x8d, 0x54, 0x24, 0x04, 0x64, 0xff, 0x15, 0xc0 }, /* win8 */
            { 0xb8, 0, 0, 0, 0, 0xe8, 0x01, 0, 0, 0, 0xc3, 0x8b, 0xd4, 0x0f, 0x34, 0xc3 },  /* win8 */
            { 0xb8, 0, 0, 0, 0, 0xe8, 0x03, 0, 0, 0, 0xc2, 0, 0, 0x8b, 0xd4, 0x0f, 0x34, 0xc3 }, /* win8 */
        };
        const BYTE *instr = RVA( rva, 32 );

        if (!instr) break;
        for (UINT i = 0; i < ARRAY_SIZE(patterns); i++)
        {
            if (match_pattern( instr, patterns[i], ARRAY_SIZE(patterns[0]) ))
            {
                printf( "  [syscall=%04x]", *(UINT *)(instr + 1) );
                return;
            }
        }
        break;
    }
    case IMAGE_FILE_MACHINE_AMD64:
    {
        static const BYTE patterns[][20] =
        {
            { 0x4c, 0x8b, 0xd1, 0xb8, 0, 0, 0, 0, 0xf6, 0x04, 0x25, 0x08, 0x03, 0xfe,
              0x7f, 0x01, 0x75, 0x03, 0x0f, 0x05 },  /* >= win10 */
            { 0x4c, 0x8b, 0xd1, 0xb8, 0, 0, 0, 0, 0x0f, 0x05, 0xc3 }, /* < win10 */
        };
        const BYTE *instr = RVA( rva, 32 );

        if (!instr) break;
        for (UINT i = 0; i < ARRAY_SIZE(patterns); i++)
        {
            if (match_pattern( instr, patterns[i], ARRAY_SIZE(patterns[0]) ))
            {
                printf( "  [syscall=%04x]", ((UINT *)instr)[1] );
                return;
            }
        }
        break;
    }
    case IMAGE_FILE_MACHINE_ARM64:
    {
        const UINT *instr = RVA( rva, 32 );

        if (!instr) break;
        if (((instr[0] & 0xffe0001f) == 0xd4000001 && instr[1] == 0xd65f03c0) || /* windows */
            ((instr[0] & 0xffe0001f) == 0xd2800008 && instr[1] == 0xaa1e03e9 && /* wine */
             instr[3] == 0xf9400210 && instr[4] == 0xd63f0200 && instr[5] == 0xd65f03c0))
            printf( "  [syscall=%04x]", (instr[0] >> 5) & 0xffff );
        break;
    }
    case IMAGE_FILE_MACHINE_ARMNT:
    {
        const USHORT *instr = RVA( rva & ~1, 16 );
        if (!instr) break;
        if (instr[0] == 0xb40f && (instr[2] & 0x0f00) == 0x0c00 &&
            ((instr[3] == 0xdef8 && instr[4] == 0xb004 && instr[5] == 0x4770) ||  /* windows */
             (instr[3] == 0x4673 && instr[6] == 0xb004 && instr[7] == 0x4770)))  /* wine */
        {
            USHORT imm = ((instr[1] & 0x400) << 1) | (instr[2] & 0xff) | ((instr[2] >> 4) & 0x0700);
            if ((instr[1] & 0xfbf0) == 0xf240)  /* T3 */
            {
                imm |= (instr[1] & 0x0f) << 12;
                printf( "  [syscall=%04x]", imm );
            }
            else if ((instr[1] & 0xfbf0) == 0xf040)  /* T2 */
            {
                switch (imm >> 8)
                {
                case 0: break;
                case 1: imm = (imm & 0xff); break;
                case 2: imm = (imm & 0xff) << 8; break;
                case 3: imm = (imm & 0xff) | ((imm & 0xff) << 8); break;
                default: imm = (0x80 | (imm & 0x7f)) << (32 - (imm >> 7)); break;
                }
                printf( "  [syscall=%04x]", imm );
            }
        }
        break;
    }
    default:
        break;
    }
}

static	void	dump_dir_exported_functions(void)
{
    unsigned int size;
    const IMAGE_EXPORT_DIRECTORY *dir = get_dir_and_size(IMAGE_FILE_EXPORT_DIRECTORY, &size);
    UINT i, *funcs;
    const UINT *pFunc;
    const UINT *pName;
    const WORD *pOrdl;

    if (!dir) return;

    printf("\n");
    printf("  Name:            %s\n", (const char*)RVA(dir->Name, sizeof(DWORD)));
    printf("  Characteristics: %08x\n", (UINT)dir->Characteristics);
    printf("  TimeDateStamp:   %08X %s\n",
           (UINT)dir->TimeDateStamp, get_time_str(dir->TimeDateStamp));
    printf("  Version:         %u.%02u\n", dir->MajorVersion, dir->MinorVersion);
    printf("  Ordinal base:    %u\n", (UINT)dir->Base);
    printf("  # of functions:  %u\n", (UINT)dir->NumberOfFunctions);
    printf("  # of Names:      %u\n", (UINT)dir->NumberOfNames);
    printf("  Functions RVA:   %08X\n", (UINT)dir->AddressOfFunctions);
    printf("  Ordinals RVA:    %08X\n", (UINT)dir->AddressOfNameOrdinals);
    printf("  Names RVA:       %08X\n", (UINT)dir->AddressOfNames);
    printf("\n");
    printf("  Entry Pt  Ordn  Name\n");

    pFunc = RVA(dir->AddressOfFunctions, dir->NumberOfFunctions * sizeof(DWORD));
    if (!pFunc) {printf("Can't grab functions' address table\n"); return;}
    pName = RVA(dir->AddressOfNames, dir->NumberOfNames * sizeof(DWORD));
    pOrdl = RVA(dir->AddressOfNameOrdinals, dir->NumberOfNames * sizeof(WORD));

    funcs = calloc( dir->NumberOfFunctions, sizeof(*funcs) );
    if (!funcs) fatal("no memory");

    for (i = 0; i < dir->NumberOfNames; i++) funcs[pOrdl[i]] = pName[i];

    for (i = 0; i < dir->NumberOfFunctions; i++)
    {
        if (!pFunc[i]) continue;
        printf("  %08X %5u  ", pFunc[i], (UINT)dir->Base + i);
        if (funcs[i])
            printf("%s", get_symbol_str((const char*)RVA(funcs[i], sizeof(DWORD))));
        else
            printf("<by ordinal>");

        /* check for forwarded function */
        if ((const char *)RVA(pFunc[i],1) >= (const char *)dir &&
            (const char *)RVA(pFunc[i],1) < (const char *)dir + size)
            printf(" (-> %s)", (const char *)RVA(pFunc[i],1));
        dump_export_flags( pFunc[i] );
        printf("\n");
    }
    free(funcs);
    printf("\n");
}


struct runtime_function_x86_64
{
    UINT BeginAddress;
    UINT EndAddress;
    UINT UnwindData;
};

struct runtime_function_armnt
{
    UINT BeginAddress;
    union {
        UINT UnwindData;
        struct {
            UINT Flag : 2;
            UINT FunctionLength : 11;
            UINT Ret : 2;
            UINT H : 1;
            UINT Reg : 3;
            UINT R : 1;
            UINT L : 1;
            UINT C : 1;
            UINT StackAdjust : 10;
        };
    };
};

struct runtime_function_arm64
{
    UINT BeginAddress;
    union
    {
        UINT UnwindData;
        struct
        {
            UINT Flag : 2;
            UINT FunctionLength : 11;
            UINT RegF : 3;
            UINT RegI : 4;
            UINT H : 1;
            UINT CR : 2;
            UINT FrameSize : 9;
        };
    };
};

union handler_data
{
    struct runtime_function_x86_64 chain;
    UINT handler;
};

struct opcode
{
    BYTE offset;
    BYTE code : 4;
    BYTE info : 4;
};

struct unwind_info_x86_64
{
    BYTE version : 3;
    BYTE flags : 5;
    BYTE prolog;
    BYTE count;
    BYTE frame_reg : 4;
    BYTE frame_offset : 4;
    struct opcode opcodes[1];  /* count entries */
    /* followed by union handler_data */
};

struct unwind_info_armnt
{
    UINT function_length : 18;
    UINT version : 2;
    UINT x : 1;
    UINT e : 1;
    UINT f : 1;
    UINT count : 5;
    UINT words : 4;
};

struct unwind_info_ext_armnt
{
    WORD excount;
    BYTE exwords;
    BYTE reserved;
};

struct unwind_info_epilogue_armnt
{
    UINT offset : 18;
    UINT res : 2;
    UINT cond : 4;
    UINT index : 8;
};

#define UWOP_PUSH_NONVOL     0
#define UWOP_ALLOC_LARGE     1
#define UWOP_ALLOC_SMALL     2
#define UWOP_SET_FPREG       3
#define UWOP_SAVE_NONVOL     4
#define UWOP_SAVE_NONVOL_FAR 5
#define UWOP_EPILOG          6
#define UWOP_SAVE_XMM128     8
#define UWOP_SAVE_XMM128_FAR 9
#define UWOP_PUSH_MACHFRAME  10

#define UNW_FLAG_EHANDLER  1
#define UNW_FLAG_UHANDLER  2
#define UNW_FLAG_CHAININFO 4

static void dump_c_exception_data( unsigned int rva )
{
    unsigned int i;
    const struct
    {
        UINT count;
        struct
        {
            UINT begin;
            UINT end;
            UINT handler;
            UINT target;
        } rec[];
    } *table = RVA( rva, sizeof(*table) );

    if (!table) return;
    printf( "    C exception data at %08x count %u\n", rva, table->count );
    for (i = 0; i < table->count; i++)
        printf( "      %u: %08x-%08x handler %08x target %08x\n", i,
                table->rec[i].begin, table->rec[i].end, table->rec[i].handler, table->rec[i].target );
}

static void dump_cxx_exception_data( unsigned int rva, unsigned int func_rva )
{
    unsigned int i, j, flags = 0;
    const unsigned int *ptr = RVA( rva, sizeof(*ptr) );

    const struct
    {
        int  prev;
        UINT handler;
    } *unwind_info;

    const struct
    {
        int  start;
        int  end;
        int  catch;
        int  catchblock_count;
        UINT catchblock;
    } *tryblock;

    const struct
    {
        UINT flags;
        UINT type_info;
        int  offset;
        UINT handler;
        UINT frame;
    } *catchblock;

    const struct
    {
        UINT flags;
        UINT type_info;
        int  offset;
        UINT handler;
    } *catchblock32;

    const struct
    {
        UINT ip;
        int  state;
    } *ipmap;

    const struct
    {
        UINT magic;
        UINT unwind_count;
        UINT unwind_table;
        UINT tryblock_count;
        UINT tryblock;
        UINT ipmap_count;
        UINT ipmap;
        int  unwind_help;
        UINT expect_list;
        UINT flags;
    } *func;

    if (!ptr || !*ptr) return;
    rva = *ptr;
    if (!(func = RVA( rva, sizeof(*func) ))) return;
    if (func->magic < 0x19930520 || func->magic > 0x19930522) return;
    if (func->magic > 0x19930521) flags = func->flags;
    printf( "    C++ exception data at %08x magic %08x", rva, func->magic );
    if (flags & 1) printf( " sync" );
    if (flags & 4) printf( " noexcept" );
    printf( "\n" );
    printf( "      unwind help %+d\n", func->unwind_help );
    if (func->magic > 0x19930520 && func->expect_list)
        printf( "      expect_list %08x\n", func->expect_list );
    if (func->unwind_count)
    {
        printf( "      unwind table at %08x count %u\n", func->unwind_table, func->unwind_count );
        if ((unwind_info = RVA( func->unwind_table, func->unwind_count * sizeof(*unwind_info) )))
        {
            for (i = 0; i < func->unwind_count; i++)
                printf( "        %u: prev %d func %08x\n", i,
                        unwind_info[i].prev, unwind_info[i].handler );
        }
    }
    if (func->tryblock_count)
    {
        printf( "      try table at %08x count %u\n", func->tryblock, func->tryblock_count );
        if ((tryblock = RVA( func->tryblock, func->tryblock_count * sizeof(*tryblock) )))
        {
            for (i = 0; i < func->tryblock_count; i++)
            {
                catchblock = RVA( tryblock[i].catchblock, sizeof(*catchblock) );
                catchblock32 = RVA( tryblock[i].catchblock, sizeof(*catchblock32) );
                printf( "        %d: start %d end %d catch %d count %u\n", i,
                        tryblock[i].start, tryblock[i].end, tryblock[i].catch,
                        tryblock[i].catchblock_count );
                for (j = 0; j < tryblock[i].catchblock_count; j++)
                {
                    const char *type = "<none>";

                    if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                    {
                        if (catchblock[j].type_info) type = RVA( catchblock[j].type_info + 16, 1 );
                        printf( "          %d: flags %x offset %+d handler %08x frame %x type %s\n", j,
                                catchblock[j].flags, catchblock[j].offset,
                                catchblock[j].handler, catchblock[j].frame, type );
                    }
                    else
                    {
                        if (catchblock32[j].type_info) type = RVA( catchblock32[j].type_info + 8, 1 );
                        printf( "          %d: flags %x offset %+d handler %08x type %s\n", j,
                                catchblock32[j].flags, catchblock32[j].offset,
                                catchblock32[j].handler, type );
                    }
                }
            }
        }
    }
    if (func->ipmap_count)
    {
        printf( "      ip map at %08x count %u\n", func->ipmap, func->ipmap_count );
        if ((ipmap = RVA( func->ipmap, func->ipmap_count * sizeof(*ipmap) )))
        {
            for (i = 0; i < func->ipmap_count; i++)
                printf( "        %u: ip %08x state %d\n", i, ipmap[i].ip, ipmap[i].state );
        }
    }
}

static UINT v4_decode_uint( const BYTE **b )
{
    UINT ret;
    const BYTE *p = *b;

    if ((*p & 1) == 0)
    {
        ret = p[0] >> 1;
        p += 1;
    }
    else if ((*p & 3) == 1)
    {
        ret = (p[0] >> 2) + (p[1] << 6);
        p += 2;
    }
    else if ((*p & 7) == 3)
    {
        ret = (p[0] >> 3) + (p[1] << 5) + (p[2] << 13);
        p += 3;
    }
    else if ((*p & 15) == 7)
    {
        ret = (p[0] >> 4) + (p[1] << 4) + (p[2] << 12) + (p[3] << 20);
        p += 4;
    }
    else
    {
        ret = 0;
        p += 5;
    }

    *b = p;
    return ret;
}

static UINT v4_read_rva( const BYTE **b )
{
    UINT ret = *(UINT *)*b;
    *b += sizeof(UINT);
    return ret;
}

#define FUNC_DESCR_IS_CATCH      0x01
#define FUNC_DESCR_IS_SEPARATED  0x02
#define FUNC_DESCR_BBT           0x04
#define FUNC_DESCR_UNWIND_MAP    0x08
#define FUNC_DESCR_TRYBLOCK_MAP  0x10
#define FUNC_DESCR_EHS           0x20
#define FUNC_DESCR_NO_EXCEPT     0x40
#define FUNC_DESCR_RESERVED      0x80

#define CATCHBLOCK_FLAGS         0x01
#define CATCHBLOCK_TYPE_INFO     0x02
#define CATCHBLOCK_OFFSET        0x04
#define CATCHBLOCK_SEPARATED     0x08
#define CATCHBLOCK_RET_ADDR_MASK 0x30
#define CATCHBLOCK_RET_ADDR      0x10
#define CATCHBLOCK_TWO_RET_ADDRS 0x20

static void dump_cxx_exception_data_v4( unsigned int rva, unsigned int func_rva )
{
    const unsigned int *ptr = RVA( rva, sizeof(*ptr) );
    const BYTE *p;
    BYTE flags;
    UINT unwind_map = 0, tryblock_map = 0, ip_map, count, i, j, k;

    if (!ptr || !*ptr) return;
    rva = *ptr;
    if (!(p = RVA( rva, 1 ))) return;
    flags = *p++;
    printf( "    C++ v4 exception data at %08x", rva );
    if (flags & FUNC_DESCR_BBT) printf( " bbt %08x", v4_decode_uint( &p ));
    if (flags & FUNC_DESCR_UNWIND_MAP) unwind_map = v4_read_rva( &p );
    if (flags & FUNC_DESCR_TRYBLOCK_MAP) tryblock_map = v4_read_rva( &p );
    ip_map = v4_read_rva(&p);
    if (flags & FUNC_DESCR_IS_CATCH) printf( " frame %08x", v4_decode_uint( &p ));
    if (flags & FUNC_DESCR_EHS) printf( " sync" );
    if (flags & FUNC_DESCR_NO_EXCEPT) printf( " noexcept" );
    printf( "\n" );

    if (unwind_map)
    {
        int *offsets;
        const BYTE *start, *p = RVA( unwind_map, 1 );
        count = v4_decode_uint( &p );
        printf( "      unwind map at %08x count %u\n", unwind_map, count );
        offsets = calloc( count, sizeof(*offsets) );
        start = p;
        for (i = 0; i < count; i++)
        {
            UINT handler, object, type;
            int offset = p - start, off, prev = -2;

            offsets[i] = offset;
            type = v4_decode_uint( &p );
            off = (type >> 2);
            if (off > offset) prev = -1;
            else for (j = 0; j < i; j++) if (offsets[j] == offset - off) prev = j;

            switch (type & 3)
            {
            case 0:
                printf( "        %u: prev %d no handler\n", i, prev );
                break;
            case 1:
                handler = v4_read_rva( &p );
                object = v4_decode_uint( &p );
                printf( "        %u: prev %d dtor obj handler %08x obj %+d\n",
                        i, prev, handler, (int)object );
                break;
            case 2:
                handler = v4_read_rva( &p );
                object = v4_decode_uint( &p );
                printf( "        %u: prev %d dtor ptr handler %08x obj %+d\n",
                        i, prev, handler, (int)object );
                break;
            case 3:
                handler = v4_read_rva( &p );
                printf( "        %u: prev %d handler %08x\n", i, prev, handler );
                break;
            }
        }
        free( offsets );
    }

    if (tryblock_map)
    {
        const BYTE *p = RVA( tryblock_map, 1 );
        count = v4_decode_uint( &p );
        printf( "      tryblock map at %08x count %u\n", tryblock_map, count );
        for (i = 0; i < count; i++)
        {
            int start = v4_decode_uint( &p );
            int end = v4_decode_uint( &p );
            int catch = v4_decode_uint( &p );
            UINT catchblock = v4_read_rva( &p );

            printf( "        %u: start %d end %d catch %d\n", i, start, end, catch );
            if (catchblock)
            {
                UINT cont[3];
                const BYTE *p = RVA( catchblock, 1 );
                UINT count2 = v4_decode_uint( &p );
                for (j = 0; j < count2; j++)
                {
                    BYTE flags = *p++;
                    printf( "          %u:", j );
                    if (flags & CATCHBLOCK_FLAGS)
                        printf( " flags %08x", v4_decode_uint( &p ));
                    if (flags & CATCHBLOCK_TYPE_INFO)
                        printf( " type %08x", v4_read_rva( &p ));
                    if (flags & CATCHBLOCK_OFFSET)
                        printf( " offset %08x", v4_decode_uint( &p ));
                    printf( " handler %08x", v4_read_rva( &p ));
                    for (k = 0; k < ((flags >> 4) & 3); k++)
                        if (flags & CATCHBLOCK_SEPARATED) cont[k] = v4_read_rva( &p );
                        else cont[k] = func_rva + v4_decode_uint( &p );
                    if (k == 1) printf( " cont %08x", cont[0] );
                    else if (k == 2) printf( " cont %08x,%08x", cont[0], cont[1] );
                    printf( "\n" );
                }
            }
        }
    }

    if (ip_map && (flags & FUNC_DESCR_IS_SEPARATED))
    {
        const BYTE *p = RVA( ip_map, 1 );

        count = v4_decode_uint( &p );
        printf( "      separated ip map at %08x count %u\n", ip_map, count );
        for (i = 0; i < count; i++)
        {
            UINT ip = v4_read_rva( &p );
            UINT map = v4_read_rva( &p );

            if (map)
            {
                UINT state, count2;
                const BYTE *p = RVA( map, 1 );

                count2 = v4_decode_uint( &p );
                printf( "        %u: start %08x map %08x\n", i, ip, map );
                for (j = 0; j < count2; j++)
                {
                    ip += v4_decode_uint( &p );
                    state = v4_decode_uint( &p );
                    printf( "          %u: ip %08x state %d\n", i, ip, state );
                }
            }
        }
    }
    else if (ip_map)
    {
        UINT ip = func_rva, state;
        const BYTE *p = RVA( ip_map, 1 );

        count = v4_decode_uint( &p );
        printf( "      ip map at %08x count %u\n", ip_map, count );
        for (i = 0; i < count; i++)
        {
            ip += v4_decode_uint( &p );
            state = v4_decode_uint( &p );
            printf( "        %u: ip %08x state %d\n", i, ip, state );
        }
    }
}

static void dump_exception_data( unsigned int rva, unsigned int func_rva, const char *name )
{
    if (strstr( name, "__C_specific_handler" )) dump_c_exception_data( rva );
    if (strstr( name, "__CxxFrameHandler4" )) dump_cxx_exception_data_v4( rva, func_rva );
    else dump_cxx_exception_data( rva, func_rva );
}

static void dump_x86_64_unwind_info( const struct runtime_function_x86_64 *function )
{
    static const char * const reg_names[16] =
        { "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
          "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15" };

    const union handler_data *handler_data;
    const struct unwind_info_x86_64 *info;
    unsigned int i, count;

    printf( "\nFunction %08x-%08x:%s\n", function->BeginAddress, function->EndAddress,
            find_export_from_rva( function->BeginAddress ));
    if (function->UnwindData & 1)
    {
        const struct runtime_function_x86_64 *next = RVA( function->UnwindData & ~1, sizeof(*next) );
        printf( "  -> function %08x-%08x\n", next->BeginAddress, next->EndAddress );
        return;
    }
    info = RVA( function->UnwindData, sizeof(*info) );

    if (!info)
    {
        printf( "  no unwind info (%x)\n", function->UnwindData );
        return;
    }
    printf( "  unwind info at %08x\n", function->UnwindData );
    if (info->version > 2)
    {
        printf( "    *** unknown version %u\n", info->version );
        return;
    }
    printf( "    flags %x", info->flags );
    if (info->flags & UNW_FLAG_EHANDLER) printf( " EHANDLER" );
    if (info->flags & UNW_FLAG_UHANDLER) printf( " UHANDLER" );
    if (info->flags & UNW_FLAG_CHAININFO) printf( " CHAININFO" );
    printf( "\n    prolog 0x%x bytes\n", info->prolog );

    if (info->frame_reg)
        printf( "    frame register %s offset 0x%x(%%rsp)\n",
                reg_names[info->frame_reg], info->frame_offset * 16 );

    for (i = 0; i < info->count; i++)
    {
        if (info->opcodes[i].code == UWOP_EPILOG)
        {
            i++;
            continue;
        }
        printf( "      0x%02x: ", info->opcodes[i].offset );
        switch (info->opcodes[i].code)
        {
        case UWOP_PUSH_NONVOL:
            printf( "push %%%s\n", reg_names[info->opcodes[i].info] );
            break;
        case UWOP_ALLOC_LARGE:
            if (info->opcodes[i].info)
            {
                count = *(const UINT *)&info->opcodes[i+1];
                i += 2;
            }
            else
            {
                count = *(const USHORT *)&info->opcodes[i+1] * 8;
                i++;
            }
            printf( "sub $0x%x,%%rsp\n", count );
            break;
        case UWOP_ALLOC_SMALL:
            count = (info->opcodes[i].info + 1) * 8;
            printf( "sub $0x%x,%%rsp\n", count );
            break;
        case UWOP_SET_FPREG:
            printf( "lea 0x%x(%%rsp),%s\n",
                    info->frame_offset * 16, reg_names[info->frame_reg] );
            break;
        case UWOP_SAVE_NONVOL:
            count = *(const USHORT *)&info->opcodes[i+1] * 8;
            printf( "mov %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
            i++;
            break;
        case UWOP_SAVE_NONVOL_FAR:
            count = *(const UINT *)&info->opcodes[i+1];
            printf( "mov %%%s,0x%x(%%rsp)\n", reg_names[info->opcodes[i].info], count );
            i += 2;
            break;
        case UWOP_SAVE_XMM128:
            count = *(const USHORT *)&info->opcodes[i+1] * 16;
            printf( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
            i++;
            break;
        case UWOP_SAVE_XMM128_FAR:
            count = *(const UINT *)&info->opcodes[i+1];
            printf( "movaps %%xmm%u,0x%x(%%rsp)\n", info->opcodes[i].info, count );
            i += 2;
            break;
        case UWOP_PUSH_MACHFRAME:
            printf( "PUSH_MACHFRAME %u\n", info->opcodes[i].info );
            break;
        default:
            printf( "*** unknown code %u\n", info->opcodes[i].code );
            break;
        }
    }

    if (info->version == 2 && info->opcodes[0].code == UWOP_EPILOG)  /* print the epilogs */
    {
        unsigned int end = function->EndAddress;
        unsigned int size = info->opcodes[0].offset;

        printf( "    epilog 0x%x bytes\n", size );
        if (info->opcodes[0].info) printf( "      at %08x-%08x\n", end - size, end );
        for (i = 1; i < info->count && info->opcodes[i].code == UWOP_EPILOG; i++)
        {
            unsigned int offset = (info->opcodes[i].info << 8) + info->opcodes[i].offset;
            if (!offset) break;
            printf( "      at %08x-%08x\n", end - offset, end - offset + size );
        }
    }

    handler_data = (const union handler_data *)&info->opcodes[(info->count + 1) & ~1];
    if (info->flags & UNW_FLAG_CHAININFO)
    {
        printf( "    -> function %08x-%08x\n",
                handler_data->chain.BeginAddress, handler_data->chain.EndAddress );
        return;
    }
    if (info->flags & (UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER))
    {
        UINT rva = function->UnwindData + ((const char *)(&handler_data->handler + 1) - (const char *)info);
        const char *name = get_function_name( handler_data->handler );

        printf( "    handler %08x%s data at %08x\n", handler_data->handler, name, rva );
        dump_exception_data( rva, function->BeginAddress, name );
    }
}

static const BYTE armnt_code_lengths[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* a0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* c0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* e0 */ 1,1,1,1,1,1,1,1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,3,4,3,4,1,1,1,1,1
};

static void dump_armnt_unwind_info( const struct runtime_function_armnt *fnc )
{
    const struct unwind_info_armnt *info;
    const struct unwind_info_ext_armnt *infoex;
    const struct unwind_info_epilogue_armnt *infoepi = NULL;
    unsigned int rva;
    WORD i, count = 0, words = 0;

    if (fnc->Flag)
    {
        char intregs[32] = {0}, intregspop[32] = {0}, vfpregs[32] = {0};
        WORD pf = 0, ef = 0, fpoffset = 0, stack = fnc->StackAdjust;
        const char *pfx = "    ...     ";

        printf( "\nFunction %08x-%08x: flag=%u ret=%u H=%u reg=%u R=%u L=%u C=%u%s\n",
                fnc->BeginAddress & ~1, (fnc->BeginAddress & ~1) + fnc->FunctionLength * 2,
                fnc->Flag, fnc->Ret, fnc->H, fnc->Reg, fnc->R, fnc->L, fnc->C,
                find_export_from_rva( fnc->BeginAddress ));

        if (fnc->StackAdjust >= 0x03f4)
        {
            pf = fnc->StackAdjust & 0x04;
            ef = fnc->StackAdjust & 0x08;
            stack = (fnc->StackAdjust & 3) + 1;
        }

        if (!fnc->R || pf)
        {
            int first = 4, last = fnc->Reg + 4;
            if (pf)
            {
                first = (~fnc->StackAdjust) & 3;
                if (fnc->R)
                    last = 3;
            }
            if (first == last)
                sprintf(intregs, "r%u", first);
            else
                sprintf(intregs, "r%u-r%u", first, last);
            fpoffset = last + 1 - first;
        }

        if (!fnc->R || ef)
        {
            int first = 4, last = fnc->Reg + 4;
            if (ef)
            {
                first = (~fnc->StackAdjust) & 3;
                if (fnc->R)
                    last = 3;
            }
            if (first == last)
                sprintf(intregspop, "r%u", first);
            else
                sprintf(intregspop, "r%u-r%u", first, last);
        }

        if (fnc->C)
        {
            if (intregs[0])
                strcat(intregs, ", ");
            if (intregspop[0])
                strcat(intregspop, ", ");
            strcat(intregs, "r11");
            strcat(intregspop, "r11");
        }
        if (fnc->L)
        {
            if (intregs[0])
                strcat(intregs, ", ");
            strcat(intregs, "lr");

            if (intregspop[0] && (fnc->Ret != 0 || !fnc->H))
                strcat(intregspop, ", ");
            if (fnc->Ret != 0)
                strcat(intregspop, "lr");
            else if (!fnc->H)
                strcat(intregspop, "pc");
        }

        if (fnc->R)
        {
            if (fnc->Reg)
                sprintf(vfpregs, "d8-d%u", fnc->Reg + 8);
            else
                strcpy(vfpregs, "d8");
        }

        printf( "  Prologue:\n" );
        if (fnc->Flag == 1) {
            if (fnc->H)
                printf( "%s push {r0-r3}\n", pfx );

            if (intregs[0])
                printf( "%s push {%s}\n", pfx, intregs );

            if (fnc->C && fpoffset == 0)
                printf( "%s mov r11, sp\n", pfx );
            else if (fnc->C)
                printf( "%s add r11, sp, #%d\n", pfx, fpoffset * 4 );

            if (fnc->R && fnc->Reg != 0x07)
                printf( "%s vpush {%s}\n", pfx, vfpregs );

            if (stack && !pf)
                printf( "%s sub sp, sp, #%d\n", pfx, stack * 4 );
        }

        if (fnc->Ret == 3)
            return;
        printf( "  Epilogue:\n" );

        if (stack && !ef)
            printf( "%s add sp, sp, #%d\n", pfx, stack * 4 );

        if (fnc->R && fnc->Reg != 0x07)
            printf( "%s vpop {%s}\n", pfx, vfpregs );

        if (intregspop[0])
            printf( "%s pop {%s}\n", pfx, intregspop );

        if (fnc->H && !(fnc->L && fnc->Ret == 0))
            printf( "%s add sp, sp, #16\n", pfx );
        else if (fnc->H && (fnc->L && fnc->Ret == 0))
            printf( "%s ldr pc, [sp], #20\n", pfx );

        if (fnc->Ret == 1)
            printf( "%s bx <reg>\n", pfx );
        else if (fnc->Ret == 2)
            printf( "%s b <address>\n", pfx );

        return;
    }

    info = RVA( fnc->UnwindData, sizeof(*info) );
    rva = fnc->UnwindData + sizeof(*info);
    count = info->count;
    words = info->words;

    printf( "\nFunction %08x-%08x: ver=%u X=%u E=%u F=%u%s\n", fnc->BeginAddress & ~1,
            (fnc->BeginAddress & ~1) + info->function_length * 2,
            info->version, info->x, info->e, info->f, find_export_from_rva( fnc->BeginAddress | 1 ));

    if (!info->count && !info->words)
    {
        infoex = RVA( rva, sizeof(*infoex) );
        rva = rva + sizeof(*infoex);
        count = infoex->excount;
        words = infoex->exwords;
    }

     if (!info->e)
    {
        infoepi = RVA( rva, count * sizeof(*infoepi) );
        rva = rva + count * sizeof(*infoepi);
    }

    if (words)
    {
        const unsigned int *codes;
        BYTE b, *bytes;
        BOOL inepilogue = FALSE;

        codes = RVA( rva, words * sizeof(*codes) );
        rva = rva + words * sizeof(*codes);
        bytes = (BYTE*)codes;

        printf( "  Prologue:\n" );
        for (b = 0; b < words * sizeof(*codes); b++)
        {
            BYTE code = bytes[b];
            BYTE len = armnt_code_lengths[code];

            if (info->e && b == count)
            {
                printf( "  Epilogue:\n" );
                inepilogue = TRUE;
            }
            else if (!info->e && infoepi)
            {
                for (i = 0; i < count; i++)
                    if (b == infoepi[i].index)
                    {
                        printf( "  Epilogue %u at %08x: (res=%x cond=%x)\n", i,
                                (fnc->BeginAddress & ~1) + infoepi[i].offset * 2,
                                infoepi[i].res, infoepi[i].cond );
                        inepilogue = TRUE;
                    }
            }

            printf( "   ");
            for (i = 0; i < len; i++)
                printf( " %02x", bytes[b+i] );
            printf( " %*s", 3 * (3 - len), "" );

            if (code == 0x00)
                printf( "\n" );
            else if (code <= 0x7f)
                printf( "%s sp, sp, #%u\n", inepilogue ? "add" : "sub", code * 4 );
            else if (code <= 0xbf)
            {
                WORD excode, f;
                BOOL first = TRUE;
                BYTE excodes = bytes[++b];

                excode = (code << 8) | excodes;
                printf( "%s {", inepilogue ? "pop" : "push" );

                for (f = 0; f <= 12; f++)
                {
                    if ((excode >> f) & 1)
                    {
                        printf( "%sr%u", first ? "" : ", ", f );
                        first = FALSE;
                    }
                }

                if (excode & 0x2000)
                    printf( "%s%s", first ? "" : ", ", inepilogue ? "pc" : "lr" );

                printf( "}\n" );
            }
            else if (code <= 0xcf)
                if (inepilogue)
                    printf( "mov sp, r%u\n", code & 0x0f );
                else
                    printf( "mov r%u, sp\n", code & 0x0f );
            else if (code <= 0xd3)
                printf( "%s {r4-r%u}\n", inepilogue ? "pop" : "push", (code & 0x03) + 4 );
            else if (code <= 0xd4)
                printf( "%s {r4, %s}\n", inepilogue ? "pop" : "push", inepilogue ? "pc" : "lr" );
            else if (code <= 0xd7)
                printf( "%s {r4-r%u, %s}\n", inepilogue ? "pop" : "push", (code & 0x03) + 4, inepilogue ? "pc" : "lr" );
            else if (code <= 0xdb)
                printf( "%s {r4-r%u}\n", inepilogue ? "pop" : "push", (code & 0x03) + 8 );
            else if (code <= 0xdf)
                printf( "%s {r4-r%u, %s}\n", inepilogue ? "pop" : "push", (code & 0x03) + 8, inepilogue ? "pc" : "lr" );
            else if (code <= 0xe7)
                printf( "%s {d8-d%u}\n", inepilogue ? "vpop" : "vpush", (code & 0x07) + 8 );
            else if (code <= 0xeb)
            {
                WORD excode;
                BYTE excodes = bytes[++b];

                excode = (code << 8) | excodes;
                printf( "%s sp, sp, #%u\n", inepilogue ? "addw" : "subw", (excode & 0x03ff) *4 );
            }
            else if (code <= 0xed)
            {
                WORD excode, f;
                BOOL first = TRUE;
                BYTE excodes = bytes[++b];

                excode = (code << 8) | excodes;
                printf( "%s {", inepilogue ? "pop" : "push" );

                for (f = 0; f < 8; f++)
                {
                    if ((excode >> f) & 1)
                    {
                        printf( "%sr%u", first ? "" : ", ", f );
                        first = FALSE;
                    }
                }

                if (excode & 0x0100)
                    printf( "%s%s", first ? "" : ", ", inepilogue ? "pc" : "lr" );

                printf( "}\n" );
            }
            else if (code == 0xee)
            {
                BYTE excodes = bytes[++b];
                if (excodes == 0x01)
                    printf( "MSFT_OP_MACHINE_FRAME\n");
                else if (excodes == 0x02)
                    printf( "MSFT_OP_CONTEXT\n");
                else
                    printf( "MSFT opcode %u\n", excodes );
            }
            else if (code == 0xef)
            {
                WORD excode;
                BYTE excodes = bytes[++b];

                if (excodes <= 0x0f)
                {
                    excode = (code << 8) | excodes;
                    if (inepilogue)
                        printf( "ldr lr, [sp], #%u\n", (excode & 0x0f) * 4 );
                    else
                        printf( "str lr, [sp, #-%u]!\n", (excode & 0x0f) * 4 );
                }
                else
                    printf( "unknown 32\n" );
            }
            else if (code <= 0xf4)
                printf( "unknown\n" );
            else if (code <= 0xf6)
            {
                WORD excode, offset = (code == 0xf6) ? 16 : 0;
                BYTE excodes = bytes[++b];

                excode = (code << 8) | excodes;
                printf( "%s {d%u-d%u}\n", inepilogue ? "vpop" : "vpush",
                        ((excode & 0x00f0) >> 4) + offset, (excode & 0x0f) + offset );
            }
            else if (code <= 0xf7)
            {
                unsigned int excode;
                BYTE excodes[2];

                excodes[0] = bytes[++b];
                excodes[1] = bytes[++b];
                excode = (code << 16) | (excodes[0] << 8) | excodes[1];
                printf( "%s sp, sp, #%u\n", inepilogue ? "add" : "sub", (excode & 0xffff) *4 );
            }
            else if (code <= 0xf8)
            {
                unsigned int excode;
                BYTE excodes[3];

                excodes[0] = bytes[++b];
                excodes[1] = bytes[++b];
                excodes[2] = bytes[++b];
                excode = (code << 24) | (excodes[0] << 16) | (excodes[1] << 8) | excodes[2];
                printf( "%s sp, sp, #%u\n", inepilogue ? "add" : "sub", (excode & 0xffffff) * 4 );
            }
            else if (code <= 0xf9)
            {
                unsigned int excode;
                BYTE excodes[2];

                excodes[0] = bytes[++b];
                excodes[1] = bytes[++b];
                excode = (code << 16) | (excodes[0] << 8) | excodes[1];
                printf( "%s sp, sp, #%u\n", inepilogue ? "add" : "sub", (excode & 0xffff) *4 );
            }
            else if (code <= 0xfa)
            {
                unsigned int excode;
                BYTE excodes[3];

                excodes[0] = bytes[++b];
                excodes[1] = bytes[++b];
                excodes[2] = bytes[++b];
                excode = (code << 24) | (excodes[0] << 16) | (excodes[1] << 8) | excodes[2];
                printf( "%s sp, sp, #%u\n", inepilogue ? "add" : "sub", (excode & 0xffffff) * 4 );
            }
            else if (code <= 0xfb)
                printf( "nop\n" );
            else if (code <= 0xfc)
                printf( "nop.w\n" );
            else if (code <= 0xfd)
            {
                printf( "(end) nop\n" );
                inepilogue = TRUE;
            }
            else if (code <= 0xfe)
            {
                printf( "(end) nop.w\n" );
                inepilogue = TRUE;
            }
            else
            {
                printf( "end\n" );
                inepilogue = TRUE;
            }
        }
    }

    if (info->x)
    {
        const unsigned int *handler;
        const char *name;

        handler = RVA( rva, sizeof(*handler) );
        rva = rva + sizeof(*handler);
        name = get_function_name( *handler );
        printf( "    handler %08x%s data at %08x\n", *handler, name, rva );
        dump_exception_data( rva, fnc->BeginAddress & ~1, name );
    }
}

struct unwind_info_arm64
{
    UINT function_length : 18;
    UINT version : 2;
    UINT x : 1;
    UINT e : 1;
    UINT epilog : 5;
    UINT codes : 5;
};

struct unwind_info_ext_arm64
{
    WORD epilog;
    BYTE codes;
    BYTE reserved;
};

struct unwind_info_epilog_arm64
{
    UINT offset : 18;
    UINT res : 4;
    UINT index : 10;
};

static const BYTE code_lengths[256] =
{
/* 00 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 20 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 40 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 60 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* 80 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* a0 */ 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
/* c0 */ 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
/* e0 */ 4,1,2,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

static void dump_arm64_codes( const BYTE *ptr, unsigned int count )
{
    unsigned int i, j;

    for (i = 0; i < count; i += code_lengths[ptr[i]])
    {
        BYTE len = code_lengths[ptr[i]];
        unsigned int val = ptr[i];
        for (j = 1; j < len; j++) val = val * 0x100 + ptr[i + j];

        printf( "    %04x: ", i );
        for (j = 0; j < 4; j++)
            if (j < len) printf( "%02x ", ptr[i+j] );
            else printf( "   " );

        if (ptr[i] < 0x20)  /* alloc_s */
        {
            printf( "sub sp,sp,#%#x\n", 16 * (val & 0x1f) );
        }
        else if (ptr[i] < 0x40)  /* save_r19r20_x */
        {
            printf( "stp x19,x20,[sp,-#%#x]!\n", 8 * (val & 0x1f) );
        }
        else if (ptr[i] < 0x80) /* save_fplr */
        {
            printf( "stp x29,lr,[sp,#%#x]\n", 8 * (val & 0x3f) );
        }
        else if (ptr[i] < 0xc0)  /* save_fplr_x */
        {
            printf( "stp x29,lr,[sp,-#%#x]!\n", 8 * (val & 0x3f) + 8 );
        }
        else if (ptr[i] < 0xc8)  /* alloc_m */
        {
            printf( "sub sp,sp,#%#x\n", 16 * (val & 0x7ff) );
        }
        else if (ptr[i] < 0xcc)  /* save_regp */
        {
            int reg = 19 + ((val >> 6) & 0xf);
            printf( "stp x%u,x%u,[sp,#%#x]\n", reg, reg + 1, 8 * (val & 0x3f) );
        }
        else if (ptr[i] < 0xd0)  /* save_regp_x */
        {
            int reg = 19 + ((val >> 6) & 0xf);
            printf( "stp x%u,x%u,[sp,-#%#x]!\n", reg, reg + 1, 8 * (val & 0x3f) + 8 );
        }
        else if (ptr[i] < 0xd4)  /* save_reg */
        {
            int reg = 19 + ((val >> 6) & 0xf);
            printf( "str x%u,[sp,#%#x]\n", reg, 8 * (val & 0x3f) );
        }
        else if (ptr[i] < 0xd6)  /* save_reg_x */
        {
            int reg = 19 + ((val >> 5) & 0xf);
            printf( "str x%u,[sp,-#%#x]!\n", reg, 8 * (val & 0x1f) + 8 );
        }
        else if (ptr[i] < 0xd8)  /* save_lrpair */
        {
            int reg = 19 + 2 * ((val >> 6) & 0x7);
            printf( "stp x%u,lr,[sp,#%#x]\n", reg, 8 * (val & 0x3f) );
        }
        else if (ptr[i] < 0xda)  /* save_fregp */
        {
            int reg = 8 + ((val >> 6) & 0x7);
            printf( "stp d%u,d%u,[sp,#%#x]\n", reg, reg + 1, 8 * (val & 0x3f) );
        }
        else if (ptr[i] < 0xdc)  /* save_fregp_x */
        {
            int reg = 8 + ((val >> 6) & 0x7);
            printf( "stp d%u,d%u,[sp,-#%#x]!\n", reg, reg + 1, 8 * (val & 0x3f) + 8 );
        }
        else if (ptr[i] < 0xde)  /* save_freg */
        {
            int reg = 8 + ((val >> 6) & 0x7);
            printf( "str d%u,[sp,#%#x]\n", reg, 8 * (val & 0x3f) );
        }
        else if (ptr[i] == 0xde)  /* save_freg_x */
        {
            int reg = 8 + ((val >> 5) & 0x7);
            printf( "str d%u,[sp,-#%#x]!\n", reg, 8 * (val & 0x3f) + 8 );
        }
        else if (ptr[i] == 0xe0)  /* alloc_l */
        {
            printf( "sub sp,sp,#%#x\n", 16 * (val & 0xffffff) );
        }
        else if (ptr[i] == 0xe1)  /* set_fp */
        {
            printf( "mov x29,sp\n" );
        }
        else if (ptr[i] == 0xe2)  /* add_fp */
        {
            printf( "add x29,sp,#%#x\n", 8 * (val & 0xff) );
        }
        else if (ptr[i] == 0xe3)  /* nop */
        {
            printf( "nop\n" );
        }
        else if (ptr[i] == 0xe4)  /* end */
        {
            printf( "end\n" );
        }
        else if (ptr[i] == 0xe5)  /* end_c */
        {
            printf( "end_c\n" );
        }
        else if (ptr[i] == 0xe6)  /* save_next */
        {
            printf( "save_next\n" );
        }
        else if (ptr[i] == 0xe7)  /* save_any_reg */
        {
            char reg = "xdq?"[(val >> 6) & 3];
            int num = (val >> 8) & 0x1f;
            if (val & 0x4000)
                printf( "stp %c%u,%c%u", reg, num, reg, num + 1 );
            else
                printf( "str %c%u,", reg, num );
            if (val & 0x2000)
                printf( "[sp,#-%#x]!\n", 16 * (val & 0x3f) + 16 );
            else
                printf( "[sp,#%#x]\n", (val & 0x3f) * ((val & 0x4080) ? 16 : 8) );
        }
        else if (ptr[i] == 0xe8)  /* MSFT_OP_TRAP_FRAME */
        {
            printf( "MSFT_OP_TRAP_FRAME\n" );
        }
        else if (ptr[i] == 0xe9)  /* MSFT_OP_MACHINE_FRAME */
        {
            printf( "MSFT_OP_MACHINE_FRAME\n" );
        }
        else if (ptr[i] == 0xea)  /* MSFT_OP_CONTEXT */
        {
            printf( "MSFT_OP_CONTEXT\n" );
        }
        else if (ptr[i] == 0xeb)  /* MSFT_OP_EC_CONTEXT */
        {
            printf( "MSFT_OP_EC_CONTEXT\n" );
        }
        else if (ptr[i] == 0xec)  /* MSFT_OP_CLEAR_UNWOUND_TO_CALL */
        {
            printf( "MSFT_OP_CLEAR_UNWOUND_TO_CALL\n" );
        }
        else if (ptr[i] == 0xfc)  /* pac_sign_lr */
        {
            printf( "pac_sign_lr\n" );
        }
        else printf( "??\n");
    }
}

static void dump_arm64_packed_info( const struct runtime_function_arm64 *func )
{
    int i, pos = 0, intsz = func->RegI * 8, fpsz = func->RegF * 8, savesz, locsz;

    if (func->CR == 1) intsz += 8;
    if (func->RegF) fpsz += 8;

    savesz = ((intsz + fpsz + 8 * 8 * func->H) + 0xf) & ~0xf;
    locsz = func->FrameSize * 16 - savesz;

    switch (func->CR)
    {
    case 3:
        printf( "    %04x:  mov x29,sp\n", pos++ );
        if (locsz <= 512)
        {
            printf( "    %04x:  stp x29,lr,[sp,-#%#x]!\n", pos++, locsz );
            break;
        }
        printf( "    %04x:  stp x29,lr,[sp,0]\n", pos++ );
        /* fall through */
    case 0:
    case 1:
        if (locsz <= 4080)
        {
            printf( "    %04x:  sub sp,sp,#%#x\n", pos++, locsz );
        }
        else
        {
            printf( "    %04x:  sub sp,sp,#%#x\n", pos++, locsz - 4080 );
            printf( "    %04x:  sub sp,sp,#%#x\n", pos++, 4080 );
        }
        break;
    }

    if (func->H)
    {
        printf( "    %04x:  stp x6,x7,[sp,#%#x]\n", pos++, intsz + fpsz + 48 );
        printf( "    %04x:  stp x4,x5,[sp,#%#x]\n", pos++, intsz + fpsz + 32 );
        printf( "    %04x:  stp x2,x3,[sp,#%#x]\n", pos++, intsz + fpsz + 16 );
        printf( "    %04x:  stp x0,x1,[sp,#%#x]\n", pos++, intsz + fpsz );
    }

    if (func->RegF)
    {
        if (func->RegF % 2 == 0)
            printf( "    %04x:  str d%u,[sp,#%#x]\n", pos++, 8 + func->RegF, intsz + fpsz - 8 );
        for (i = (func->RegF - 1)/ 2; i >= 0; i--)
        {
            if (!i && !intsz)
                printf( "    %04x:  stp d8,d9,[sp,-#%#x]!\n", pos++, savesz );
            else
                printf( "    %04x:  stp d%u,d%u,[sp,#%#x]\n", pos++, 8 + 2 * i, 9 + 2 * i, intsz + 16 * i );
        }
    }

    switch (func->RegI)
    {
    case 0:
        if (func->CR == 1)
            printf( "    %04x:  str lr,[sp,-#%#x]!\n", pos++, savesz );
        break;
    case 1:
        if (func->CR == 1)
            printf( "    %04x:  stp x19,lr,[sp,-#%#x]!\n", pos++, savesz );
        else
            printf( "    %04x:  str x19,[sp,-#%#x]!\n", pos++, savesz );
        break;
    default:
        if (func->RegI % 2)
        {
            if (func->CR == 1)
                printf( "    %04x:  stp x%u,lr,[sp,#%#x]\n", pos++, 18 + func->RegI, 8 * func->RegI - 8 );
            else
                printf( "    %04x:  str x%u,[sp,#%#x]\n", pos++, 18 + func->RegI, 8 * func->RegI - 8 );
        }
        else if (func->CR == 1)
            printf( "    %04x:  str lr,[sp,#%#x]\n", pos++, intsz - 8 );

        for (i = func->RegI / 2 - 1; i >= 0; i--)
            if (i)
                printf( "    %04x:  stp x%u,x%u,[sp,#%#x]\n", pos++, 19 + 2 * i, 20 + 2 * i, 16 * i );
            else
                printf( "    %04x:  stp x19,x20,[sp,-#%#x]!\n", pos++, savesz );
        break;
    }
    printf( "    %04x:  end\n", pos );
}

static void dump_arm64_unwind_info( const struct runtime_function_arm64 *func )
{
    const struct unwind_info_arm64 *info;
    const struct unwind_info_ext_arm64 *infoex;
    const struct unwind_info_epilog_arm64 *infoepi;
    const struct runtime_function_arm64 *parent_func;
    const BYTE *ptr;
    unsigned int i, rva, codes, epilogs;

    switch (func->Flag)
    {
    case 1:
    case 2:
        printf( "\nFunction %08x-%08x:%s\n", func->BeginAddress,
                func->BeginAddress + func->FunctionLength * 4, find_export_from_rva( func->BeginAddress ));
        printf( "    len=%#x flag=%x regF=%u regI=%u H=%u CR=%u frame=%x\n",
                func->FunctionLength, func->Flag, func->RegF, func->RegI,
                func->H, func->CR, func->FrameSize );
        dump_arm64_packed_info( func );
        return;
    case 3:
        rva = func->UnwindData & ~3;
        parent_func = RVA( rva, sizeof(*parent_func) );
        printf( "\nFunction %08x-%08x:%s\n", func->BeginAddress,
                func->BeginAddress + 12 /* adrl x16, <dest>; br x16 */,
                find_export_from_rva( func->BeginAddress ));
        printf( "    forward to parent %08x\n", parent_func->BeginAddress );
        return;
    }

    rva = func->UnwindData;
    info = RVA( rva, sizeof(*info) );
    rva += sizeof(*info);
    epilogs = info->epilog;
    codes = info->codes;

    if (!codes)
    {
        infoex = RVA( rva, sizeof(*infoex) );
        rva = rva + sizeof(*infoex);
        codes = infoex->codes;
        epilogs = infoex->epilog;
    }
    printf( "\nFunction %08x-%08x:\n",
            func->BeginAddress, func->BeginAddress + info->function_length * 4 );
    printf( "    len=%#x ver=%u X=%u E=%u epilogs=%u codes=%u\n",
            info->function_length, info->version, info->x, info->e, epilogs, codes * 4 );
    if (info->e)
    {
        printf( "    epilog 0: code=%04x\n", info->epilog );
    }
    else
    {
        infoepi = RVA( rva, sizeof(*infoepi) * epilogs );
        rva += sizeof(*infoepi) * epilogs;
        for (i = 0; i < epilogs; i++)
            printf( "    epilog %u: pc=%08x code=%04x\n", i,
                    func->BeginAddress + infoepi[i].offset * 4, infoepi[i].index );
    }
    ptr = RVA( rva, codes * 4);
    rva += codes * 4;
    dump_arm64_codes( ptr, codes * 4 );
    if (info->x)
    {
        const UINT *handler = RVA( rva, sizeof(*handler) );
        const char *name = get_function_name( *handler );

        rva += sizeof(*handler);
        printf( "    handler: %08x%s data %08x\n", *handler, name, rva );
        dump_exception_data( rva, func->BeginAddress, name );
    }
}

static void dump_dir_exceptions(void)
{
    static const void *arm64_funcs;
    unsigned int i, size;
    const void *funcs;
    const IMAGE_FILE_HEADER *file_header = &PE_nt_headers->FileHeader;
    const IMAGE_ARM64EC_METADATA *metadata;

    funcs = get_dir_and_size(IMAGE_FILE_EXCEPTION_DIRECTORY, &size);
    if (!funcs) return;

    switch (file_header->Machine)
    {
    case IMAGE_FILE_MACHINE_AMD64:
        size /= sizeof(struct runtime_function_x86_64);
        printf( "%s exception info (%u functions):\n", get_machine_str( file_header->Machine ), size );
        for (i = 0; i < size; i++) dump_x86_64_unwind_info( (struct runtime_function_x86_64*)funcs + i );
        if (!(metadata = get_hybrid_metadata())) break;
        if (!(size = metadata->ExtraRFETableSize)) break;
        if (!(funcs = RVA( metadata->ExtraRFETable, size ))) break;
        if (funcs == arm64_funcs) break; /* already dumped */
        printf( "\n" );
        /* fall through */
    case IMAGE_FILE_MACHINE_ARM64:
        arm64_funcs = funcs;
        size /= sizeof(struct runtime_function_arm64);
        printf( "%s exception info (%u functions):\n", get_machine_str( file_header->Machine ), size );
        for (i = 0; i < size; i++) dump_arm64_unwind_info( (struct runtime_function_arm64*)funcs + i );
        break;
    case IMAGE_FILE_MACHINE_ARMNT:
        size /= sizeof(struct runtime_function_armnt);
        printf( "%s exception info (%u functions):\n", get_machine_str( file_header->Machine ), size );
        for (i = 0; i < size; i++) dump_armnt_unwind_info( (struct runtime_function_armnt*)funcs + i );
        break;
    default:
        printf( "Exception information not supported for %s binaries\n",
                get_machine_str(file_header->Machine));
        break;
    }
    printf( "\n" );
}


static void dump_image_thunk_data64(const IMAGE_THUNK_DATA64 *il, UINT thunk_rva)
{
    /* FIXME: This does not properly handle large images */
    const IMAGE_IMPORT_BY_NAME* iibn;
    for (; il->u1.Ordinal; il++, thunk_rva += sizeof(LONGLONG))
    {
        if (IMAGE_SNAP_BY_ORDINAL64(il->u1.Ordinal))
            printf("  %08x  %4u  <by ordinal>\n", thunk_rva, (UINT)IMAGE_ORDINAL64(il->u1.Ordinal));
        else
        {
            iibn = RVA((DWORD)il->u1.AddressOfData, sizeof(DWORD));
            if (!iibn)
                printf("Can't grab import by name info, skipping to next ordinal\n");
            else
                printf("  %08x  %4u  %s\n", thunk_rva, iibn->Hint, iibn->Name);
        }
    }
}

static void dump_image_thunk_data32(const IMAGE_THUNK_DATA32 *il, int offset, UINT thunk_rva)
{
    const IMAGE_IMPORT_BY_NAME* iibn;
    for (; il->u1.Ordinal; il++, thunk_rva += sizeof(UINT))
    {
        if (IMAGE_SNAP_BY_ORDINAL32(il->u1.Ordinal))
            printf("  %08x  %4u  <by ordinal>\n", thunk_rva, (UINT)IMAGE_ORDINAL32(il->u1.Ordinal));
        else
        {
            iibn = RVA((DWORD)il->u1.AddressOfData - offset, sizeof(DWORD));
            if (!iibn)
                printf("Can't grab import by name info, skipping to next ordinal\n");
            else
                printf("  %08x  %4u  %s\n", thunk_rva, iibn->Hint, iibn->Name);
        }
    }
}

static	void	dump_dir_imported_functions(void)
{
    unsigned directorySize;
    const IMAGE_IMPORT_DESCRIPTOR* importDesc = get_dir_and_size(IMAGE_FILE_IMPORT_DIRECTORY, &directorySize);

    if (!importDesc)	return;

    printf("Import Table size: %08x\n", directorySize);/* FIXME */

    for (;;)
    {
	const IMAGE_THUNK_DATA32*	il;

        if (!importDesc->Name || !importDesc->FirstThunk) break;

	printf("  offset %08lx %s\n", Offset(importDesc), (const char*)RVA(importDesc->Name, sizeof(DWORD)));
	printf("  Hint/Name Table: %08X\n", (UINT)importDesc->OriginalFirstThunk);
	printf("  TimeDateStamp:   %08X (%s)\n",
	       (UINT)importDesc->TimeDateStamp, get_time_str(importDesc->TimeDateStamp));
	printf("  ForwarderChain:  %08X\n", (UINT)importDesc->ForwarderChain);
	printf("  First thunk RVA: %08X\n", (UINT)importDesc->FirstThunk);

	printf("   Thunk    Ordn  Name\n");

	il = (importDesc->OriginalFirstThunk != 0) ?
	    RVA((DWORD)importDesc->OriginalFirstThunk, sizeof(DWORD)) :
	    RVA((DWORD)importDesc->FirstThunk, sizeof(DWORD));

	if (!il)
            printf("Can't grab thunk data, going to next imported DLL\n");
        else
        {
            if(PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                dump_image_thunk_data64((const IMAGE_THUNK_DATA64*)il, importDesc->FirstThunk);
            else
                dump_image_thunk_data32(il, 0, importDesc->FirstThunk);
            printf("\n");
        }
	importDesc++;
    }
    printf("\n");
}

static void dump_hybrid_metadata(void)
{
    unsigned int i;
    const void *metadata = get_hybrid_metadata();

    if (!metadata) return;
    printf( "Hybrid metadata\n" );

    switch (PE_nt_headers->FileHeader.Machine)
    {
    case IMAGE_FILE_MACHINE_I386:
    {
        const IMAGE_CHPE_METADATA_X86 *data = metadata;

        printf( "  Version                                       %#x\n", (int)data->Version );
        printf( "  CHPECodeAddressRangeOffset                    %#x\n", (int)data->CHPECodeAddressRangeOffset );
        printf( "  CHPECodeAddressRangeCount                     %#x\n", (int)data->CHPECodeAddressRangeCount );
        printf( "  WowA64ExceptionHandlerFunctionPointer         %#x\n", (int)data->WowA64ExceptionHandlerFunctionPointer );
        printf( "  WowA64DispatchCallFunctionPointer             %#x\n", (int)data->WowA64DispatchCallFunctionPointer );
        printf( "  WowA64DispatchIndirectCallFunctionPointer     %#x\n", (int)data->WowA64DispatchIndirectCallFunctionPointer );
        printf( "  WowA64DispatchIndirectCallCfgFunctionPointer  %#x\n", (int)data->WowA64DispatchIndirectCallCfgFunctionPointer );
        printf( "  WowA64DispatchRetFunctionPointer              %#x\n", (int)data->WowA64DispatchRetFunctionPointer );
        printf( "  WowA64DispatchRetLeafFunctionPointer          %#x\n", (int)data->WowA64DispatchRetLeafFunctionPointer );
        printf( "  WowA64DispatchJumpFunctionPointer             %#x\n", (int)data->WowA64DispatchJumpFunctionPointer );
        if (data->Version >= 2)
            printf( "  CompilerIATPointer                            %#x\n", (int)data->CompilerIATPointer );
        if (data->Version >= 3)
            printf( "  WowA64RdtscFunctionPointer                    %#x\n", (int)data->WowA64RdtscFunctionPointer );
        if (data->Version >= 4)
        {
            printf( "  unknown[0]                                    %#x\n", (int)data->unknown[0] );
            printf( "  unknown[1]                                    %#x\n", (int)data->unknown[1] );
            printf( "  unknown[2]                                    %#x\n", (int)data->unknown[2] );
            printf( "  unknown[3]                                    %#x\n", (int)data->unknown[3] );
        }

        if (data->CHPECodeAddressRangeOffset)
        {
            const IMAGE_CHPE_RANGE_ENTRY *map = RVA( data->CHPECodeAddressRangeOffset,
                                                     data->CHPECodeAddressRangeCount * sizeof(*map) );

            printf( "\nCode ranges\n" );
            for (i = 0; i < data->CHPECodeAddressRangeCount; i++)
            {
                static const char *types[] = { "x86", "ARM64" };
                unsigned int start = map[i].StartOffset & ~1;
                unsigned int type = map[i].StartOffset & 1;
                printf(  "  %08x - %08x  %s\n", start, start + (int)map[i].Length, types[type] );
            }
        }

        break;
    }

    case IMAGE_FILE_MACHINE_AMD64:
    case IMAGE_FILE_MACHINE_ARM64:
    {
        const IMAGE_ARM64EC_METADATA *data = metadata;

        printf( "  Version                                %#x\n", (int)data->Version );
        printf( "  CodeMap                                %#x\n", (int)data->CodeMap );
        printf( "  CodeMapCount                           %#x\n", (int)data->CodeMapCount );
        printf( "  CodeRangesToEntryPoints                %#x\n", (int)data->CodeRangesToEntryPoints );
        printf( "  RedirectionMetadata                    %#x\n", (int)data->RedirectionMetadata );
        printf( "  __os_arm64x_dispatch_call_no_redirect  %#x\n", (int)data->__os_arm64x_dispatch_call_no_redirect );
        printf( "  __os_arm64x_dispatch_ret               %#x\n", (int)data->__os_arm64x_dispatch_ret );
        printf( "  __os_arm64x_dispatch_call              %#x\n", (int)data->__os_arm64x_dispatch_call );
        printf( "  __os_arm64x_dispatch_icall             %#x\n", (int)data->__os_arm64x_dispatch_icall );
        printf( "  __os_arm64x_dispatch_icall_cfg         %#x\n", (int)data->__os_arm64x_dispatch_icall_cfg );
        printf( "  AlternateEntryPoint                    %#x\n", (int)data->AlternateEntryPoint );
        printf( "  AuxiliaryIAT                           %#x\n", (int)data->AuxiliaryIAT );
        printf( "  CodeRangesToEntryPointsCount           %#x\n", (int)data->CodeRangesToEntryPointsCount );
        printf( "  RedirectionMetadataCount               %#x\n", (int)data->RedirectionMetadataCount );
        printf( "  GetX64InformationFunctionPointer       %#x\n", (int)data->GetX64InformationFunctionPointer );
        printf( "  SetX64InformationFunctionPointer       %#x\n", (int)data->SetX64InformationFunctionPointer );
        printf( "  ExtraRFETable                          %#x\n", (int)data->ExtraRFETable );
        printf( "  ExtraRFETableSize                      %#x\n", (int)data->ExtraRFETableSize );
        printf( "  __os_arm64x_dispatch_fptr              %#x\n", (int)data->__os_arm64x_dispatch_fptr );
        printf( "  AuxiliaryIATCopy                       %#x\n", (int)data->AuxiliaryIATCopy );
        printf( "  __os_arm64x_helper0                    %#x\n", (int)data->__os_arm64x_helper0 );
        printf( "  __os_arm64x_helper1                    %#x\n", (int)data->__os_arm64x_helper1 );
        printf( "  __os_arm64x_helper2                    %#x\n", (int)data->__os_arm64x_helper2 );
        printf( "  __os_arm64x_helper3                    %#x\n", (int)data->__os_arm64x_helper3 );
        printf( "  __os_arm64x_helper4                    %#x\n", (int)data->__os_arm64x_helper4 );
        printf( "  __os_arm64x_helper5                    %#x\n", (int)data->__os_arm64x_helper5 );
        printf( "  __os_arm64x_helper6                    %#x\n", (int)data->__os_arm64x_helper6 );
        printf( "  __os_arm64x_helper7                    %#x\n", (int)data->__os_arm64x_helper7 );
        printf( "  __os_arm64x_helper8                    %#x\n", (int)data->__os_arm64x_helper8 );

        if (data->CodeMap)
        {
            const IMAGE_CHPE_RANGE_ENTRY *map = RVA( data->CodeMap, data->CodeMapCount * sizeof(*map) );

            printf( "\nCode ranges\n" );
            for (i = 0; i < data->CodeMapCount; i++)
            {
                static const char *types[] = { "ARM64", "ARM64EC", "x64", "??" };
                unsigned int start = map[i].StartOffset & ~0x3;
                unsigned int type = map[i].StartOffset & 0x3;
                printf(  "  %08x - %08x  %s\n", start, start + (int)map[i].Length, types[type] );
            }
        }

        if (PE_nt_headers->FileHeader.Machine == IMAGE_FILE_MACHINE_ARM64) break;

        if (data->CodeRangesToEntryPoints)
        {
            const IMAGE_ARM64EC_CODE_RANGE_ENTRY_POINT *map = RVA( data->CodeRangesToEntryPoints,
                                                     data->CodeRangesToEntryPointsCount * sizeof(*map) );

            printf( "\nCode ranges to entry points\n" );
            printf( "    Start  -   End      Entry point\n" );
            for (i = 0; i < data->CodeRangesToEntryPointsCount; i++)
            {
                const char *name = find_export_from_rva( map[i].EntryPoint );
                printf(  "  %08x - %08x   %08x%s\n",
                         (int)map[i].StartRva, (int)map[i].EndRva, (int)map[i].EntryPoint, name );
            }
        }

        if (data->RedirectionMetadata)
        {
            const IMAGE_ARM64EC_REDIRECTION_ENTRY *map = RVA( data->RedirectionMetadata,
                                                     data->RedirectionMetadataCount * sizeof(*map) );

            printf( "\nEntry point redirection\n" );
            for (i = 0; i < data->RedirectionMetadataCount; i++)
            {
                const char *name = find_export_from_rva( map[i].Source );
                printf(  "  %08x -> %08x%s\n", (int)map[i].Source, (int)map[i].Destination, name );
            }
        }
        break;
    }
    }
    printf( "\n" );
}

static void dump_dir_loadconfig(void)
{
    unsigned int size;
    const IMAGE_LOAD_CONFIG_DIRECTORY32 *loadcfg32;

    loadcfg32 = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size);
    if (!loadcfg32) return;
    size = min( size, loadcfg32->Size );

    printf( "Loadconfig\n" );
    print_dword( "Size",                                loadcfg32->Size );
    print_dword( "TimeDateStamp",                       loadcfg32->TimeDateStamp );
    print_word(  "MajorVersion",                        loadcfg32->MajorVersion );
    print_word(  "MinorVersion",                        loadcfg32->MinorVersion );
    print_dword( "GlobalFlagsClear",                    loadcfg32->GlobalFlagsClear );
    print_dword( "GlobalFlagsSet",                      loadcfg32->GlobalFlagsSet );
    print_dword( "CriticalSectionDefaultTimeout",       loadcfg32->CriticalSectionDefaultTimeout );

    if(PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_LOAD_CONFIG_DIRECTORY64 *loadcfg64 = (void *)loadcfg32;

        print_longlong( "DeCommitFreeBlockThreshold",   loadcfg64->DeCommitFreeBlockThreshold );
        print_longlong( "DeCommitTotalFreeThreshold",   loadcfg64->DeCommitTotalFreeThreshold );
        print_longlong( "MaximumAllocationSize",        loadcfg64->MaximumAllocationSize );
        print_longlong( "VirtualMemoryThreshold",       loadcfg64->VirtualMemoryThreshold );
        print_dword(    "ProcessHeapFlags",             loadcfg64->ProcessHeapFlags );
        print_longlong( "ProcessAffinityMask",          loadcfg64->ProcessAffinityMask );
        print_word(     "CSDVersion",                   loadcfg64->CSDVersion );
        print_word(     "DependentLoadFlags",           loadcfg64->DependentLoadFlags );
        print_longlong( "SecurityCookie",               loadcfg64->SecurityCookie );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, SEHandlerTable )) goto done;
        print_longlong( "SEHandlerTable",               loadcfg64->SEHandlerTable );
        print_longlong( "SEHandlerCount",               loadcfg64->SEHandlerCount );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardCFCheckFunctionPointer )) goto done;
        print_longlong( "GuardCFCheckFunctionPointer",  loadcfg64->GuardCFCheckFunctionPointer );
        print_longlong( "GuardCFDispatchFunctionPointer", loadcfg64->GuardCFDispatchFunctionPointer );
        print_longlong( "GuardCFFunctionTable",         loadcfg64->GuardCFFunctionTable );
        print_longlong( "GuardCFFunctionCount",         loadcfg64->GuardCFFunctionCount );
        print_dword(    "GuardFlags",                   loadcfg64->GuardFlags );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, CodeIntegrity )) goto done;
        print_word(     "CodeIntegrity.Flags",          loadcfg64->CodeIntegrity.Flags );
        print_word(     "CodeIntegrity.Catalog",        loadcfg64->CodeIntegrity.Catalog );
        print_dword(    "CodeIntegrity.CatalogOffset",  loadcfg64->CodeIntegrity.CatalogOffset );
        print_dword(    "CodeIntegrity.Reserved",       loadcfg64->CodeIntegrity.Reserved );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardAddressTakenIatEntryTable )) goto done;
        print_longlong( "GuardAddressTakenIatEntryTable", loadcfg64->GuardAddressTakenIatEntryTable );
        print_longlong( "GuardAddressTakenIatEntryCount", loadcfg64->GuardAddressTakenIatEntryCount );
        print_longlong( "GuardLongJumpTargetTable",     loadcfg64->GuardLongJumpTargetTable );
        print_longlong( "GuardLongJumpTargetCount",     loadcfg64->GuardLongJumpTargetCount );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, DynamicValueRelocTable )) goto done;
        print_longlong( "DynamicValueRelocTable",       loadcfg64->DynamicValueRelocTable );
        print_longlong( "CHPEMetadataPointer",          loadcfg64->CHPEMetadataPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardRFFailureRoutine )) goto done;
        print_longlong( "GuardRFFailureRoutine",        loadcfg64->GuardRFFailureRoutine );
        print_longlong( "GuardRFFailureRoutineFuncPtr", loadcfg64->GuardRFFailureRoutineFunctionPointer );
        print_dword(    "DynamicValueRelocTableOffset", loadcfg64->DynamicValueRelocTableOffset );
        print_word(     "DynamicValueRelocTableSection",loadcfg64->DynamicValueRelocTableSection );
        print_word(     "Reserved2",                    loadcfg64->Reserved2 );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardRFVerifyStackPointerFunctionPointer )) goto done;
        print_longlong( "GuardRFVerifyStackPointerFuncPtr", loadcfg64->GuardRFVerifyStackPointerFunctionPointer );
        print_dword(    "HotPatchTableOffset",          loadcfg64->HotPatchTableOffset );
        print_dword(    "Reserved3",                    loadcfg64->Reserved3 );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, EnclaveConfigurationPointer )) goto done;
        print_longlong( "EnclaveConfigurationPointer",  loadcfg64->EnclaveConfigurationPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, VolatileMetadataPointer )) goto done;
        print_longlong( "VolatileMetadataPointer",      loadcfg64->VolatileMetadataPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardEHContinuationTable )) goto done;
        print_longlong( "GuardEHContinuationTable",     loadcfg64->GuardEHContinuationTable );
        print_longlong( "GuardEHContinuationCount",     loadcfg64->GuardEHContinuationCount );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardXFGCheckFunctionPointer )) goto done;
        print_longlong( "GuardXFGCheckFunctionPointer", loadcfg64->GuardXFGCheckFunctionPointer );
        print_longlong( "GuardXFGDispatchFunctionPointer", loadcfg64->GuardXFGDispatchFunctionPointer );
        print_longlong( "GuardXFGTableDispatchFuncPtr", loadcfg64->GuardXFGTableDispatchFunctionPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, CastGuardOsDeterminedFailureMode )) goto done;
        print_longlong( "CastGuardOsDeterminedFailureMode", loadcfg64->CastGuardOsDeterminedFailureMode );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, GuardMemcpyFunctionPointer )) goto done;
        print_longlong( "GuardMemcpyFunctionPointer",   loadcfg64->GuardMemcpyFunctionPointer );
    }
    else
    {
        print_dword( "DeCommitFreeBlockThreshold",      loadcfg32->DeCommitFreeBlockThreshold );
        print_dword( "DeCommitTotalFreeThreshold",      loadcfg32->DeCommitTotalFreeThreshold );
        print_dword( "MaximumAllocationSize",           loadcfg32->MaximumAllocationSize );
        print_dword( "VirtualMemoryThreshold",          loadcfg32->VirtualMemoryThreshold );
        print_dword( "ProcessHeapFlags",                loadcfg32->ProcessHeapFlags );
        print_dword( "ProcessAffinityMask",             loadcfg32->ProcessAffinityMask );
        print_word(  "CSDVersion",                      loadcfg32->CSDVersion );
        print_word(  "DependentLoadFlags",              loadcfg32->DependentLoadFlags );
        print_dword( "SecurityCookie",                  loadcfg32->SecurityCookie );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, SEHandlerTable )) goto done;
        print_dword( "SEHandlerTable",                  loadcfg32->SEHandlerTable );
        print_dword( "SEHandlerCount",                  loadcfg32->SEHandlerCount );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardCFCheckFunctionPointer )) goto done;
        print_dword( "GuardCFCheckFunctionPointer",     loadcfg32->GuardCFCheckFunctionPointer );
        print_dword( "GuardCFDispatchFunctionPointer",  loadcfg32->GuardCFDispatchFunctionPointer );
        print_dword( "GuardCFFunctionTable",            loadcfg32->GuardCFFunctionTable );
        print_dword( "GuardCFFunctionCount",            loadcfg32->GuardCFFunctionCount );
        print_dword( "GuardFlags",                      loadcfg32->GuardFlags );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, CodeIntegrity )) goto done;
        print_word(  "CodeIntegrity.Flags",             loadcfg32->CodeIntegrity.Flags );
        print_word(  "CodeIntegrity.Catalog",           loadcfg32->CodeIntegrity.Catalog );
        print_dword( "CodeIntegrity.CatalogOffset",     loadcfg32->CodeIntegrity.CatalogOffset );
        print_dword( "CodeIntegrity.Reserved",          loadcfg32->CodeIntegrity.Reserved );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardAddressTakenIatEntryTable )) goto done;
        print_dword( "GuardAddressTakenIatEntryTable",  loadcfg32->GuardAddressTakenIatEntryTable );
        print_dword( "GuardAddressTakenIatEntryCount",  loadcfg32->GuardAddressTakenIatEntryCount );
        print_dword( "GuardLongJumpTargetTable",        loadcfg32->GuardLongJumpTargetTable );
        print_dword( "GuardLongJumpTargetCount",        loadcfg32->GuardLongJumpTargetCount );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, DynamicValueRelocTable )) goto done;
        print_dword( "DynamicValueRelocTable",          loadcfg32->DynamicValueRelocTable );
        print_dword( "CHPEMetadataPointer",             loadcfg32->CHPEMetadataPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardRFFailureRoutine )) goto done;
        print_dword( "GuardRFFailureRoutine",           loadcfg32->GuardRFFailureRoutine );
        print_dword( "GuardRFFailureRoutineFuncPtr",    loadcfg32->GuardRFFailureRoutineFunctionPointer );
        print_dword( "DynamicValueRelocTableOffset",    loadcfg32->DynamicValueRelocTableOffset );
        print_word(  "DynamicValueRelocTableSection",   loadcfg32->DynamicValueRelocTableSection );
        print_word(  "Reserved2",                       loadcfg32->Reserved2 );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardRFVerifyStackPointerFunctionPointer )) goto done;
        print_dword( "GuardRFVerifyStackPointerFuncPtr", loadcfg32->GuardRFVerifyStackPointerFunctionPointer );
        print_dword( "HotPatchTableOffset",             loadcfg32->HotPatchTableOffset );
        print_dword( "Reserved3",                       loadcfg32->Reserved3 );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, EnclaveConfigurationPointer )) goto done;
        print_dword( "EnclaveConfigurationPointer",     loadcfg32->EnclaveConfigurationPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, VolatileMetadataPointer )) goto done;
        print_dword( "VolatileMetadataPointer",         loadcfg32->VolatileMetadataPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardEHContinuationTable )) goto done;
        print_dword( "GuardEHContinuationTable",        loadcfg32->GuardEHContinuationTable );
        print_dword( "GuardEHContinuationCount",        loadcfg32->GuardEHContinuationCount );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardXFGCheckFunctionPointer )) goto done;
        print_dword( "GuardXFGCheckFunctionPointer",    loadcfg32->GuardXFGCheckFunctionPointer );
        print_dword( "GuardXFGDispatchFunctionPointer", loadcfg32->GuardXFGDispatchFunctionPointer );
        print_dword( "GuardXFGTableDispatchFuncPtr", loadcfg32->GuardXFGTableDispatchFunctionPointer );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, CastGuardOsDeterminedFailureMode )) goto done;
        print_dword( "CastGuardOsDeterminedFailureMode", loadcfg32->CastGuardOsDeterminedFailureMode );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, GuardMemcpyFunctionPointer )) goto done;
        print_dword( "GuardMemcpyFunctionPointer",      loadcfg32->GuardMemcpyFunctionPointer );
    }
done:
    printf( "\n" );
    dump_hybrid_metadata();
}

static void dump_dir_delay_imported_functions(void)
{
    unsigned  directorySize;
    const IMAGE_DELAYLOAD_DESCRIPTOR *importDesc = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT, &directorySize);

    if (!importDesc) return;

    printf("Delay Import Table size: %08x\n", directorySize); /* FIXME */

    for (;;)
    {
        const IMAGE_THUNK_DATA32*       il;
        int                             offset = (importDesc->Attributes.AllAttributes & 1) ? 0 : PE_nt_headers->OptionalHeader.ImageBase;

        if (!importDesc->DllNameRVA || !importDesc->ImportAddressTableRVA || !importDesc->ImportNameTableRVA) break;

        printf("  grAttrs %08x offset %08lx %s\n", (UINT)importDesc->Attributes.AllAttributes,
               Offset(importDesc), (const char *)RVA(importDesc->DllNameRVA - offset, sizeof(DWORD)));
        printf("  Hint/Name Table: %08x\n", (UINT)importDesc->ImportNameTableRVA);
        printf("  Address Table:   %08x\n", (UINT)importDesc->ImportAddressTableRVA);
        printf("  TimeDateStamp:   %08X (%s)\n",
               (UINT)importDesc->TimeDateStamp, get_time_str(importDesc->TimeDateStamp));

        printf("   Thunk    Ordn  Name\n");

        il = RVA(importDesc->ImportNameTableRVA - offset, sizeof(DWORD));

        if (!il)
            printf("Can't grab thunk data, going to next imported DLL\n");
        else
        {
            if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                dump_image_thunk_data64((const IMAGE_THUNK_DATA64 *)il, importDesc->ImportAddressTableRVA);
            else
                dump_image_thunk_data32(il, offset, importDesc->ImportAddressTableRVA);
            printf("\n");
        }
        importDesc++;
    }
    printf("\n");
}

static	void	dump_dir_debug_dir(const IMAGE_DEBUG_DIRECTORY* idd, int idx, const IMAGE_SECTION_HEADER *first_section)
{
    const	char*	str;

    printf("Directory %02u\n", idx + 1);
    printf("  Characteristics:   %08X\n", (UINT)idd->Characteristics);
    printf("  TimeDateStamp:     %08X %s\n",
	   (UINT)idd->TimeDateStamp, get_time_str(idd->TimeDateStamp));
    printf("  Version            %u.%02u\n", idd->MajorVersion, idd->MinorVersion);
    switch (idd->Type)
    {
    default:
    case IMAGE_DEBUG_TYPE_UNKNOWN:	str = "UNKNOWN"; 	break;
    case IMAGE_DEBUG_TYPE_COFF:		str = "COFF"; 		break;
    case IMAGE_DEBUG_TYPE_CODEVIEW:	str = "CODEVIEW"; 	break;
    case IMAGE_DEBUG_TYPE_FPO:		str = "FPO"; 		break;
    case IMAGE_DEBUG_TYPE_MISC:		str = "MISC"; 		break;
    case IMAGE_DEBUG_TYPE_EXCEPTION:	str = "EXCEPTION"; 	break;
    case IMAGE_DEBUG_TYPE_FIXUP:	str = "FIXUP"; 		break;
    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:	str = "OMAP_TO_SRC"; 	break;
    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:str = "OMAP_FROM_SRC"; 	break;
    case IMAGE_DEBUG_TYPE_BORLAND:	str = "BORLAND"; 	break;
    case IMAGE_DEBUG_TYPE_RESERVED10:	str = "RESERVED10"; 	break;
    case IMAGE_DEBUG_TYPE_CLSID:	str = "CLSID"; 	break;
    case IMAGE_DEBUG_TYPE_VC_FEATURE:  str = "VC_FEATURE"; break;
    case IMAGE_DEBUG_TYPE_POGO:        str = "POGO";       break;
    case IMAGE_DEBUG_TYPE_ILTCG:       str = "ILTCG";      break;
    case IMAGE_DEBUG_TYPE_MPX:         str = "MPX";        break;
    case IMAGE_DEBUG_TYPE_REPRO:       str = "REPRO";      break;
    }
    printf("  Type:              %u (%s)\n", (UINT)idd->Type, str);
    printf("  SizeOfData:        %u\n", (UINT)idd->SizeOfData);
    printf("  AddressOfRawData:  %08X\n", (UINT)idd->AddressOfRawData);
    printf("  PointerToRawData:  %08X\n", (UINT)idd->PointerToRawData);

    switch (idd->Type)
    {
    case IMAGE_DEBUG_TYPE_UNKNOWN:
	break;
    case IMAGE_DEBUG_TYPE_COFF:
	dump_coff(idd->PointerToRawData, idd->SizeOfData, first_section);
	break;
    case IMAGE_DEBUG_TYPE_CODEVIEW:
	dump_codeview(idd->PointerToRawData, idd->SizeOfData);
	break;
    case IMAGE_DEBUG_TYPE_FPO:
	dump_frame_pointer_omission(idd->PointerToRawData, idd->SizeOfData);
	break;
    case IMAGE_DEBUG_TYPE_MISC:
        {
            const IMAGE_DEBUG_MISC* misc = PRD(idd->PointerToRawData, idd->SizeOfData);
            if (!misc || idd->SizeOfData < sizeof(*misc)) {printf("Can't get MISC debug information\n"); break;}
            printf("    DataType:          %u (%s)\n",
                   (UINT)misc->DataType, (misc->DataType == IMAGE_DEBUG_MISC_EXENAME) ? "Exe name" : "Unknown");
            printf("    Length:            %u\n", (UINT)misc->Length);
            printf("    Unicode:           %s\n", misc->Unicode ? "Yes" : "No");
            printf("    Data:              %s\n", misc->Data);
        }
        break;
    case IMAGE_DEBUG_TYPE_POGO:
        {
            const unsigned* data = PRD(idd->PointerToRawData, idd->SizeOfData);
            const unsigned* end = (const unsigned*)((const unsigned char*)data + idd->SizeOfData);
            unsigned idx = 0;

            if (!data || idd->SizeOfData < sizeof(unsigned)) {printf("Can't get PODO debug information\n"); break;}
            printf("    Header:            %08x\n", *(const unsigned*)data);
            data++;
            printf("      Index            Name            Offset   Size\n");
            while (data + 2 < end)
            {
                const char* ptr;
                ptrdiff_t s;

                if (!(ptr = memchr(&data[2], '\0', (const char*)end - (const char*)&data[2]))) break;
                printf("      %-5u            %-16s %08x %08x\n", idx, (const char*)&data[2], data[0], data[1]);
                s = ptr - (const char*)&data[2] + 1;
                data += 2 + ((s + sizeof(unsigned) - 1) / sizeof(unsigned));
                idx++;
            }
        }
        break;
    case IMAGE_DEBUG_TYPE_REPRO:
        {
            const IMAGE_DEBUG_REPRO* repro = PRD(idd->PointerToRawData, idd->SizeOfData);
            if (!repro || idd->SizeOfData < sizeof(*repro)) {printf("Can't get REPRO debug information\n"); break;}
            printf("    Flags:             %08X\n", repro->flags);
            printf("    Guid:              %s\n", get_guid_str(&repro->guid));
            printf("    _unk0:             %08X %u\n", repro->unk[0], repro->unk[0]);
            printf("    _unk1:             %08X %u\n", repro->unk[1], repro->unk[1]);
            printf("    _unk2:             %08X %u\n", repro->unk[2], repro->unk[2]);
            printf("    Timestamp:         %08X\n", repro->debug_timestamp);
        }
        break;
    default:
        {
            const unsigned char* data = PRD(idd->PointerToRawData, idd->SizeOfData);
            if (!data) {printf("Can't get debug information for %s\n", str); break;}
            dump_data(data, idd->SizeOfData, "    ");
        }
        break;
    }
    printf("\n");
}

static void	dump_dir_debug(void)
{
    unsigned			nb_dbg, i;
    const IMAGE_DEBUG_DIRECTORY*debugDir = get_dir_and_size(IMAGE_FILE_DEBUG_DIRECTORY, &nb_dbg);

    nb_dbg /= sizeof(*debugDir);
    if (!debugDir || !nb_dbg) return;

    printf("Debug Table (%u directories)\n", nb_dbg);

    for (i = 0; i < nb_dbg; i++)
    {
	dump_dir_debug_dir(debugDir, i, IMAGE_FIRST_SECTION(PE_nt_headers));
	debugDir++;
    }
    printf("\n");
}

static struct
{
    const BYTE *data;
    UINT size;
} strings, userstrings, blobs, guids, tables;

static void print_clr_flags( const char *title, UINT value )
{
    printf("  %-34s 0x%X\n", title, value);
#define X(f,s) if (value & f) printf("    %s\n", s)
    X(COMIMAGE_FLAGS_ILONLY,           "ILONLY");
    X(COMIMAGE_FLAGS_32BITREQUIRED,    "32BITREQUIRED");
    X(COMIMAGE_FLAGS_IL_LIBRARY,       "IL_LIBRARY");
    X(COMIMAGE_FLAGS_STRONGNAMESIGNED, "STRONGNAMESIGNED");
    X(COMIMAGE_FLAGS_TRACKDEBUGDATA,   "TRACKDEBUGDATA");
#undef X
}

static void print_clr_directory( const char *title, const IMAGE_DATA_DIRECTORY *dir )
{
    printf("  %-23s rva: 0x%-8x  size: 0x%-8x\n", title, (UINT)dir->VirtualAddress, (UINT)dir->Size);
}

static UINT clr_indent = 4;

static void print_clr_indent( void )
{
    UINT i;
    for (i = 0; i < clr_indent; i++) printf( " " );
}

static void print_clr( const char *str )
{
    print_clr_indent();
    printf( "%s", str );
}

static void print_clr_strings( const BYTE *data, UINT data_size )
{
    const char *beg = (const char *)data, *end;
    UINT count = 0;

    if (!data_size) return;
    clr_indent += 4;
    for (;;)
    {
        if (!(end = memchr( beg, '\0', data_size - (beg - (const char *)data) ))) break;
        print_clr_indent();
        printf( "%-10u\"%s\"\n", count, beg );
        beg = end + 1;
        count++;
    }
    clr_indent -= 4;
}

static UINT clr_blob_size( const BYTE *data, UINT data_size, UINT *skip )
{
    if (!data_size) return 0;
    if (!(data[0] & 0x80))
    {
        *skip = 1;
        return data[0];
    }
    if (data_size < 2) return 0;
    if (!(data[0] & 0x40))
    {
        *skip = 2;
        return ((data[0] & ~0xc0) << 8) + data[1];
    }
    if (data_size < 4) return 0;
    if (!(data[0] & 0x20))
    {
        *skip = 4;
        return ((data[0] & ~0xe0) << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
    }
    return 0;
}

static void print_clr_blobs( const BYTE *data, UINT data_size )
{
    const BYTE *blob = data + 1;
    UINT i, size, skip;

    if (!data_size) return;

    clr_indent += 4;
    print_clr_indent();
    printf( "%-10u %02x\n", 0, data[0] );
    data_size--;
    for (;;)
    {
        if (!(size = clr_blob_size( blob, data_size, &skip ))) break;
        print_clr_indent();
        printf( "%-10u ", (UINT)(blob - data) );
        for (i = 0; i < size; i++)
        {
            printf( "%02x ",  blob[i + skip] );
            if (i && i < size - 1 && !((i + 1) % 30))
            {
                printf( "\n" );
                print_clr( "           " );
            }
        }
        printf( "\n" );
        data_size -= size + skip;
        blob += size + skip;
    }
    clr_indent -= 4;
}

static void print_clr_guids( const BYTE *data, UINT data_size )
{
    const BYTE *guid = data;
    UINT i, j, count = data_size / 16;

    clr_indent += 4;
    for (i = 0; i < count; i++)
    {
        print_clr_indent();
        printf( "%-10u ", count );
        printf( "%08x", *(UINT *)guid );
        printf( " %04x", *(UINT *)guid + 4 );
        printf( " %04x", *(USHORT *)guid + 6 );
        for (j = 0; j < 8; j++) printf( " %02x", guid[j + 8] );
        printf( "\n" );
        guid += 16;
    }
    clr_indent -= 4;
}

static UINT str_idx_size = 2;
static UINT guid_idx_size = 2;
static UINT blob_idx_size = 2;

static void print_clr_byte( const BYTE **ptr, UINT *size, BOOL flags )
{
    if (*size < 1) return;
    printf( flags ? "0x%02x            " : "%-16u", **ptr );
    (*ptr)++; (*size)--;
}

static void print_clr_ushort( const BYTE **ptr, UINT *size, BOOL flags )
{
    if (*size < sizeof(USHORT)) return;
    printf( flags ? "0x%04x          " : "%-16u", *(const USHORT *)*ptr );
    *ptr += sizeof(USHORT); *size -= sizeof(USHORT);
}

static void print_clr_uint( const BYTE **ptr, UINT *size, BOOL flags )
{
    if (*size < sizeof(UINT)) return;
    printf( flags ? "0x%08x      " : "%-16u", *(const UINT *)*ptr );
    *ptr += sizeof(UINT); *size -= sizeof(UINT);
}

static void print_clr_uint64( const BYTE **ptr, UINT *size )
{
    if (*size < sizeof(UINT64)) return;
    printf( "0x%08x%08x", (UINT)(*(const UINT64 *)*ptr >> 32), (UINT)*(const UINT64 *)*ptr );
    *ptr += sizeof(UINT64); *size -= sizeof(UINT64);
}

static void print_clr_string_idx( const BYTE **ptr, UINT *size )
{
    if (*size < str_idx_size) return;
    if (str_idx_size == 2)
    {
        USHORT idx = *(const USHORT *)*ptr;
        printf( "%-16u", idx );
    }
    else
    {
        UINT idx = *(const UINT *)*ptr;
        printf( "%-16u", idx );
    }
    *ptr += str_idx_size; *size -= str_idx_size;
}

static void print_clr_blob_idx( const BYTE **ptr, UINT *size )
{
    if (*size < blob_idx_size) return;
    if (blob_idx_size == 2)
    {
        USHORT idx = *(const USHORT *)*ptr;
        printf( "%-16u", idx );
    }
    else
    {
        UINT idx = *(const UINT *)*ptr;
        printf( "%-16u", idx );
    }
    *ptr += blob_idx_size; *size -= blob_idx_size;
}

static void print_clr_guid_idx( const BYTE **ptr, UINT *size )
{
    if (*size < guid_idx_size) return;
    if (guid_idx_size == 2)
    {
        USHORT idx = *(const USHORT *)*ptr;
        printf( "%-16u", idx );
    }
    else
    {
        UINT idx = *(const UINT *)*ptr;
        printf( "%-16u", idx );
    }
    *ptr += guid_idx_size; *size -= guid_idx_size;
}

enum
{
    CLR_TABLE_MODULE                    = 0x00,
    CLR_TABLE_TYPEREF                   = 0x01,
    CLR_TABLE_TYPEDEF                   = 0x02,
    CLR_TABLE_FIELD                     = 0x04,
    CLR_TABLE_METHODDEF                 = 0x06,
    CLR_TABLE_PARAM                     = 0x08,
    CLR_TABLE_INTERFACEIMPL             = 0x09,
    CLR_TABLE_MEMBERREF                 = 0x0a,
    CLR_TABLE_CONSTANT                  = 0x0b,
    CLR_TABLE_CUSTOMATTRIBUTE           = 0x0c,
    CLR_TABLE_FIELDMARSHAL              = 0x0d,
    CLR_TABLE_DECLSECURITY              = 0x0e,
    CLR_TABLE_CLASSLAYOUT               = 0x0f,
    CLR_TABLE_FIELDLAYOUT               = 0x10,
    CLR_TABLE_STANDALONESIG             = 0x11,
    CLR_TABLE_EVENTMAP                  = 0x12,
    CLR_TABLE_EVENT                     = 0x14,
    CLR_TABLE_PROPERTYMAP               = 0x15,
    CLR_TABLE_PROPERTY                  = 0x17,
    CLR_TABLE_METHODSEMANTICS           = 0x18,
    CLR_TABLE_METHODIMPL                = 0x19,
    CLR_TABLE_MODULEREF                 = 0x1a,
    CLR_TABLE_TYPESPEC                  = 0x1b,
    CLR_TABLE_IMPLMAP                   = 0x1c,
    CLR_TABLE_FIELDRVA                  = 0x1d,
    CLR_TABLE_ASSEMBLY                  = 0x20,
    CLR_TABLE_ASSEMBLYPROCESSOR         = 0x21,
    CLR_TABLE_ASSEMBLYOS                = 0x22,
    CLR_TABLE_ASSEMBLYREF               = 0x23,
    CLR_TABLE_ASSEMBLYREFPROCESSOR      = 0x24,
    CLR_TABLE_ASSEMBLYREFOS             = 0x25,
    CLR_TABLE_FILE                      = 0x26,
    CLR_TABLE_EXPORTEDTYPE              = 0x27,
    CLR_TABLE_MANIFESTRESOURCE          = 0x28,
    CLR_TABLE_NESTEDCLASS               = 0x29,
    CLR_TABLE_GENERICPARAM              = 0x2a,
    CLR_TABLE_METHODSPEC                = 0x2b,
    CLR_TABLE_GENERICPARAMCONSTRAINT    = 0x2c,
    CLR_TABLE_MAX                       = 0x2d,
};

static void print_clr_table_idx( const BYTE **ptr, UINT *size )
{
    USHORT idx;
    if (*size < sizeof(USHORT)) return; /* FIXME should be 4 bytes if the number of rows exceeds 2^16 */
    idx = *(const USHORT *)*ptr;
    printf( "0x%04x          ", idx );
    *ptr += sizeof(USHORT); *size -= sizeof(USHORT);
}

static void print_clr_row( UINT row )
{
    print_clr_indent();
    printf( "%-10u ", row );
}

static void print_clr_table_header( const char * const *header )
{
    const char *str;

    print_clr_indent();
    printf( "%-11s", "#" );
    while ((str = *header++)) printf( "%-16s", str );
    printf( "\n" );
}

static void print_clr_table_module( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Generation", "Name", "Mvid", "EncId", "EncBaseId", NULL };
    UINT i, row = 1;

    print_clr( "Module table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_string_idx( ptr, size );
        print_clr_guid_idx( ptr, size );
        print_clr_guid_idx( ptr, size );
        print_clr_guid_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_typeref( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "ResolutionScope", "TypeName", "TypeNamespace", NULL };
    UINT i, row = 1;

    print_clr( "TypeRef table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_typedef( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Flags", "TypeName", "TypeNamespace", "Extends", "FieldList",
                                           "MethodList", NULL };
    UINT i, row = 1;

    print_clr( "TypeDef table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_field( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Flags", "Name", "Signature", NULL };
    UINT i, row = 1;

    print_clr( "Field table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_methoddef( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "RVA", "ImplFlags", "Flags", "Name", "Signature", "ParamList", NULL };
    UINT i, row = 1;

    print_clr( "MethodDef table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_param( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Flags", "Sequence", "Name", NULL };
    UINT i, row = 1;

    print_clr( "Param table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_string_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_interfaceimpl( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Class", "Interface", NULL };
    UINT i, row = 1;

    print_clr( "InterfaceImpl table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_memberref( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Class", "Name", "Signature", NULL };
    UINT i, row = 1;

    print_clr( "MemberRef table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_constant( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Type", "Padding", "Parent", "Value", NULL };
    UINT i, row = 1;

    print_clr( "Constant table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_byte( ptr, size, FALSE );
        print_clr_byte( ptr, size, FALSE );
        print_clr_table_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_customattribute( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Parent", "Type", "Value", NULL };
    UINT i, row = 1;

    print_clr( "CustomAttribute table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_fieldmarshal( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Parent", "NativeType", NULL };
    UINT i, row = 1;

    print_clr( "FieldMarshal table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_declsecurity( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Action", "Parent", "PermissionSet", NULL };
    UINT i, row = 1;

    print_clr( "DeclSecurity table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_table_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_classlayout( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "PackingSize", "ClassSize", "Parent", NULL };
    UINT i, row = 1;

    print_clr( "ClassLayout table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_uint( ptr, size, FALSE );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_fieldlayout( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Offset", "Field", NULL };
    UINT i, row = 1;

    print_clr( "FieldLayout table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, FALSE );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_standalonesig( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Signature", NULL };
    UINT i, row = 1;

    print_clr( "StandAloneSig table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_eventmap( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Parent", "EventList", NULL };
    UINT i, row = 1;

    print_clr( "EventMap table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_event( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "EventFlags", "Name", "EventType", NULL };
    UINT i, row = 1;

    print_clr( "Event table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_propertymap( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Parent", "PropertyList", NULL };
    UINT i, row = 1;

    print_clr( "PropertyMap table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_property( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Flags", "Name", "Type", NULL };
    UINT i, row = 1;

    print_clr( "Property table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_methodsemantics( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Semantics", "Method", "Association", NULL };
    UINT i, row = 1;

    print_clr( "MethodSemantics table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_methodimpl( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Class", "Body", "Declaration", NULL };
    UINT i, row = 1;

    print_clr( "MethodImpl table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_moduleref( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Name", NULL };
    UINT i, row = 1;

    print_clr( "ModuleRef table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_string_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_typespec( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Signature", NULL };
    UINT i, row = 1;

    print_clr( "TypeSpec table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_implmap( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "MappingFlags", "MemberForwarded", "ImportName", "ImportScope", NULL };
    UINT i, row = 1;

    print_clr( "ImplMap table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_table_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_fieldrva( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "RVA", "Field", NULL };
    UINT i, row = 1;

    print_clr( "FieldRVA table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_assembly( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "HashAlgId", "MajorVersion", "MinorVersion", "BuildNumber",
                                           "RevisionNumber", "Flags", "PublicKey", "Name", "Culture", NULL };
    UINT i, row = 1;

    print_clr( "Assembly table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_uint( ptr, size, TRUE );
        print_clr_blob_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_assemblyprocessor( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Processor", NULL };
    UINT i, row = 1;

    print_clr( "AssemblyProcessor table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_assemblyos( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "OSPlatformID", "OSMajorVersion", "OSMinorVersion", NULL };
    UINT i, row = 1;

    print_clr( "AssemblyOS table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_uint( ptr, size, FALSE );
        print_clr_uint( ptr, size, FALSE );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_assemblyref( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "MajorVersion", "MinorVersion", "BuildNumber", "RevisionNumber", "Flags",
                                           "PublicKey", "Name", "Culture", "HashValue", NULL };
    UINT i, row = 1;

    print_clr( "AssemblyRef table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_uint( ptr, size, TRUE );
        print_clr_blob_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_assemblyrefprocessor( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Processor", "AssemblyRef", NULL };
    UINT i, row = 1;

    print_clr( "AssemblyRefProcessor table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_assemblyrefos( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "OSPlatformId", "OSMajorVersion", "OSMinorVersion", "AssemblyRef", NULL };
    UINT i, row = 1;

    print_clr( "AssemblyRefOS table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_uint( ptr, size, FALSE );
        print_clr_uint( ptr, size, FALSE );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_file( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Flags", "Name", "HashValue", NULL };
    UINT i, row = 1;

    print_clr( "File table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_exportedtype( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Flags", "TypeDefId", "TypeName", "TypeNamespace", "Implementation", NULL };
    UINT i, row = 1;

    print_clr( "ExportedType table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, TRUE );
        print_clr_uint( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_manifestresource( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Offset", "Flags", "Name", "Implementation", NULL };
    UINT i, row = 1;

    print_clr( "ManifestResource table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_uint( ptr, size, FALSE );
        print_clr_uint( ptr, size, TRUE );
        print_clr_string_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_nestedclass( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "NestedClass", "EnclosingClass", NULL };
    UINT i, row = 1;

    print_clr( "NestedClass table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_table_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_genericparam( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Number", "Flags", "Owner", "Name", NULL };
    UINT i, row = 1;

    print_clr( "GenericParam table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_ushort( ptr, size, FALSE );
        print_clr_ushort( ptr, size, TRUE );
        print_clr_table_idx( ptr, size );
        print_clr_string_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_methodspec( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Method", "Instantiation", NULL };
    UINT i, row = 1;

    print_clr( "MethodSpec table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

static void print_clr_table_genericparamconstraint( const BYTE **ptr, UINT *size, UINT row_count )
{
    static const char * const header[] = { "Owner", "Constraint", NULL };
    UINT i, row = 1;

    print_clr( "GenericParamConstraint table\n" );
    clr_indent += 4;
    print_clr_table_header( header );
    for (i = 0; i < row_count; i++)
    {
        print_clr_row( row++ );
        print_clr_table_idx( ptr, size );
        print_clr_blob_idx( ptr, size );
        printf( "\n" );
    }
    clr_indent -= 4;
}

enum
{
    HEAP_SIZE_FLAG_STRING = 0x01,
    HEAP_SIZE_FLAG_GUID   = 0x02,
    HEAP_SIZE_FLAG_BLOB   = 0x04,
};

static void print_clr_tables( const BYTE *data, UINT data_size )
{
    const BYTE *ptr = data;
    BYTE heap_sizes;
    UINT i, size = data_size, count = 0;
    const UINT *row_count;
    UINT64 valid, j;

    if (!data_size) return;

    clr_indent += 4;
    print_clr( "Reserved     " );
    print_clr_uint( &ptr, &size, FALSE );
    printf( "\n" );
    print_clr( "MajorVersion " );
    print_clr_byte( &ptr, &size, FALSE );
    printf( "\n" );
    print_clr( "MinorVersion " );
    print_clr_byte( &ptr, &size, FALSE );
    printf( "\n" );

    if (size < 1) return;
    heap_sizes = *ptr;
    print_clr( "HeapSizes    " );
    print_clr_byte( &ptr, &size, TRUE );
    printf( "\n" );
    if (heap_sizes & HEAP_SIZE_FLAG_STRING) str_idx_size = 4;
    if (heap_sizes & HEAP_SIZE_FLAG_GUID) guid_idx_size = 4;
    if (heap_sizes & HEAP_SIZE_FLAG_BLOB) blob_idx_size = 4;

    print_clr( "Reserved     " );
    print_clr_byte( &ptr, &size, FALSE );
    printf( "\n" );

    if (size < sizeof(UINT64)) return;
    valid = *(UINT64 *)ptr;
    print_clr( "Valid        " );
    print_clr_uint64( &ptr, &size );
    printf( "\n" );
    for (j = 0; j < sizeof(valid) * 8; j++) if (valid & (1ul << j)) count++;

    print_clr( "Sorted       " );
    print_clr_uint64( &ptr, &size );
    printf( "\n" );
    clr_indent -= 4;

    row_count = (const UINT *)ptr;
    if (count) print_clr( "Row counts\n" );
    for (i = 0; i < count; i++)
    {
        clr_indent += 4;
        print_clr_indent();
        print_clr_uint( &ptr, &size, FALSE );
        clr_indent -= 4;
        printf( "\n" );
    }

    for (i = 0, j = 0; j < sizeof(valid) * 8; j++)
    {
        if (!(valid & (1ul << j))) continue;

        if (row_count[i] > 65536)
        {
            printf( "can't handle number of rows > 65536\n" );
            break;
        }

        switch (j)
        {
        case CLR_TABLE_MODULE:
            print_clr_table_module( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_TYPEREF:
            print_clr_table_typeref( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_TYPEDEF:
            print_clr_table_typedef( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_FIELD:
            print_clr_table_field( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_METHODDEF:
            print_clr_table_methoddef( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_PARAM:
            print_clr_table_param( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_INTERFACEIMPL:
            print_clr_table_interfaceimpl( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_MEMBERREF:
            print_clr_table_memberref( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_CONSTANT:
            print_clr_table_constant( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_CUSTOMATTRIBUTE:
            print_clr_table_customattribute( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_FIELDMARSHAL:
            print_clr_table_fieldmarshal( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_DECLSECURITY:
            print_clr_table_declsecurity( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_CLASSLAYOUT:
            print_clr_table_classlayout( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_FIELDLAYOUT:
            print_clr_table_fieldlayout( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_STANDALONESIG:
            print_clr_table_standalonesig( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_EVENTMAP:
            print_clr_table_eventmap( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_EVENT:
            print_clr_table_event( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_PROPERTYMAP:
            print_clr_table_propertymap( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_PROPERTY:
            print_clr_table_property( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_METHODSEMANTICS:
            print_clr_table_methodsemantics( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_METHODIMPL:
            print_clr_table_methodimpl( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_MODULEREF:
            print_clr_table_moduleref( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_TYPESPEC:
            print_clr_table_typespec( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_IMPLMAP:
            print_clr_table_implmap( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_FIELDRVA:
            print_clr_table_fieldrva( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_ASSEMBLY:
            print_clr_table_assembly( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_ASSEMBLYPROCESSOR:
            print_clr_table_assemblyprocessor( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_ASSEMBLYOS:
            print_clr_table_assemblyos( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_ASSEMBLYREF:
            print_clr_table_assemblyref( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_ASSEMBLYREFPROCESSOR:
            print_clr_table_assemblyrefprocessor( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_ASSEMBLYREFOS:
            print_clr_table_assemblyrefos( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_FILE:
            print_clr_table_file( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_EXPORTEDTYPE:
            print_clr_table_exportedtype( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_MANIFESTRESOURCE:
            print_clr_table_manifestresource( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_NESTEDCLASS:
            print_clr_table_nestedclass( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_GENERICPARAM:
            print_clr_table_genericparam( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_METHODSPEC:
            print_clr_table_methodspec( &ptr, &size, row_count[i++] );
            break;
        case CLR_TABLE_GENERICPARAMCONSTRAINT:
            print_clr_table_genericparamconstraint( &ptr, &size, row_count[i++] );
            break;
        default:
            printf( "unknown table 0x%02x\n", (BYTE)j );
            break;
        }
    }
}

static void print_clr_stream( const BYTE *base, const BYTE **ptr, UINT *size )
{
    const char *name;
    UINT len, stream_offset, stream_size;

    if (*size < sizeof(UINT)) return;
    stream_offset = *(const UINT *)*ptr;
    print_clr( "Offset " );
    print_clr_uint( ptr, size, FALSE );
    printf( "\n" );

    if (*size < sizeof(UINT)) return;
    stream_size = *(const UINT *)*ptr;
    print_clr( "Size   " );
    print_clr_uint( ptr, size, FALSE );
    printf( "\n" );

    if (!memchr( *ptr, '\0', *size )) return;
    name = (const char *)*ptr;
    print_clr( "Name   " );
    printf( "%s\n", name );
    len = ((strlen( name ) + 1) + 3) & ~3;
    if (*size < len) return;
    *ptr += len; *size -= len;

    if (!strcmp( name, "#Strings" ))
    {
        strings.data = base + stream_offset;
        strings.size = stream_size;
        /* remove padding */
        while (strings.size > 1 && !strings.data[strings.size -2] && !strings.data[strings.size - 1])
            strings.size--;
    }
    else if (!strcmp( name, "#US" ))
    {
        userstrings.data = base + stream_offset;
        userstrings.size = stream_size;
    }
    else if (!strcmp( name, "#Blob" ))
    {
        blobs.data = base + stream_offset;
        blobs.size = stream_size;
    }
    else if (!strcmp( name, "#GUID" ))
    {
        guids.data = base + stream_offset;
        guids.size = stream_size;
    }
    else if (!strcmp( name, "#~" ))
    {
        tables.data = base + stream_offset;
        tables.size = stream_size;
    }
}

static void print_clr_metadata( const BYTE *data, UINT data_size )
{
    const BYTE *ptr = data;
    UINT size = data_size, len, nb_streams, i;
    const char *version;

    print_clr( "Signature    " );
    print_clr_uint( &ptr, &size, TRUE );
    printf( "\n" );
    print_clr( "MajorVersion " );
    print_clr_ushort(  &ptr, &size, FALSE );
    printf( "\n" );
    print_clr( "MinorVersion " );
    print_clr_ushort( &ptr, &size, FALSE );
    printf( "\n" );
    print_clr( "Reserved     " );
    print_clr_uint( &ptr, &size, FALSE );
    printf( "\n" );

    if (size < sizeof(UINT)) return;
    len = *(const UINT *)ptr;
    print_clr( "Length       " );
    print_clr_uint( &ptr, &size, FALSE );
    printf( "\n" );

    if (size < len || !memchr( ptr, '\0', len )) return;
    version = (const char *)ptr;
    print_clr( "Version      " );
    printf( "%s\n", version );
    len = ((strlen( version ) + 1) + 3) & ~3;
    if (size < len) return;
    ptr += len; size -= len;

    print_clr( "Flags        " );
    print_clr_ushort( &ptr, &size, TRUE );
    printf( "\n" );

    if (size < sizeof(USHORT)) return;
    nb_streams = *(const USHORT *)ptr;
    print_clr( "Streams      " );
    print_clr_ushort( &ptr, &size, FALSE );
    printf( "\n" );

    clr_indent += 4;
    for (i = 0; i < nb_streams; i++) print_clr_stream( data, &ptr, &size );
    clr_indent -= 4;

    print_clr( "Tables\n" );
    print_clr_tables( tables.data, tables.size );
    print_clr( "Strings\n" );
    print_clr_strings( strings.data, strings.size );
    print_clr( "Userstrings\n" );
    print_clr_blobs( userstrings.data, userstrings.size );
    print_clr( "GUIDs\n" );
    print_clr_guids( guids.data, guids.size );
    print_clr( "Blobs\n" );
    print_clr_blobs( blobs.data, blobs.size );
}

static void dump_dir_clr_header(void)
{
    unsigned int size = 0;
    const IMAGE_COR20_HEADER *dir = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR, &size);

    if (!dir) return;

    printf( "CLR Header\n" );
    print_dword( "Header Size", dir->cb );
    print_ver( "Required runtime version", dir->MajorRuntimeVersion, dir->MinorRuntimeVersion );
    print_clr_flags( "Flags", dir->Flags );
    print_dword( "EntryPointToken", dir->EntryPointToken );
    printf("\n");
    printf( "CLR Data Directory\n" );
    print_clr_directory( "MetaData", &dir->MetaData );
    print_clr_metadata( RVA(dir->MetaData.VirtualAddress, dir->MetaData.Size), dir->MetaData.Size );
    print_clr_directory( "Resources", &dir->Resources );
    print_clr_directory( "StrongNameSignature", &dir->StrongNameSignature );
    print_clr_directory( "CodeManagerTable", &dir->CodeManagerTable );
    print_clr_directory( "VTableFixups", &dir->VTableFixups );
    print_clr_directory( "ExportAddressTableJumps", &dir->ExportAddressTableJumps );
    print_clr_directory( "ManagedNativeHeader", &dir->ManagedNativeHeader );
    printf("\n");
}

static void dump_dynamic_relocs_arm64x( const IMAGE_BASE_RELOCATION *base_reloc, unsigned int size )
{
    unsigned int i;
    const IMAGE_BASE_RELOCATION *base_end = (const IMAGE_BASE_RELOCATION *)((const char *)base_reloc + size);

    printf( "Relocations ARM64X\n" );
    while (base_reloc < base_end - 1 && base_reloc->SizeOfBlock)
    {
        const USHORT *rel = (const USHORT *)(base_reloc + 1);
        const USHORT *end = (const USHORT *)base_reloc + base_reloc->SizeOfBlock / sizeof(USHORT);
        printf( "  Page %x\n", (UINT)base_reloc->VirtualAddress );
        while (rel < end && *rel)
        {
            USHORT offset = *rel & 0xfff;
            USHORT type = (*rel >> 12) & 3;
            USHORT arg = *rel >> 14;
            rel++;
            switch (type)
            {
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_ZEROFILL:
                printf( "    off %04x zero-fill %u bytes\n", offset, 1 << arg );
                break;
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE:
                printf( "    off %04x set %u bytes value ", offset, 1 << arg );
                for (i = (1 << arg ) / sizeof(USHORT); i > 0; i--) printf( "%04x", rel[i - 1] );
                rel += (1 << arg) / sizeof(USHORT);
                printf( "\n" );
                break;
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA:
                printf( "    off %04x add offset ", offset );
                if (arg & 1) printf( "-" );
                printf( "%08x\n", (UINT)*rel++ * ((arg & 2) ? 8 : 4) );
                break;
            default:
                printf( "    off %04x unknown (arg %x)\n", offset, arg );
                break;
            }
        }
        base_reloc = (const IMAGE_BASE_RELOCATION *)end;
    }
}

static void dump_dynamic_relocs( const char *ptr, unsigned int size, ULONGLONG symbol )
{
    switch (symbol)
    {
    case IMAGE_DYNAMIC_RELOCATION_GUARD_RF_PROLOGUE:
        printf( "Relocations GUARD_RF_PROLOGUE\n" );
        break;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_RF_EPILOGUE:
        printf( "Relocations GUARD_RF_EPILOGUE\n" );
        break;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_IMPORT_CONTROL_TRANSFER:
        printf( "Relocations GUARD_IMPORT_CONTROL_TRANSFER\n" );
        break;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_INDIR_CONTROL_TRANSFER:
        printf( "Relocations GUARD_INDIR_CONTROL_TRANSFER\n" );
        break;
    case IMAGE_DYNAMIC_RELOCATION_GUARD_SWITCHTABLE_BRANCH:
        printf( "Relocations GUARD_SWITCHTABLE_BRANCH\n" );
        break;
    case IMAGE_DYNAMIC_RELOCATION_ARM64X:
        dump_dynamic_relocs_arm64x( (const IMAGE_BASE_RELOCATION *)ptr, size );
        break;
    default:
        printf( "Unknown relocation symbol %s\n", longlong_str(symbol) );
        break;
    }
}

static const IMAGE_DYNAMIC_RELOCATION_TABLE *get_dyn_reloc_table(void)
{
    unsigned int size, section, offset;
    const IMAGE_SECTION_HEADER *sec;
    const IMAGE_DYNAMIC_RELOCATION_TABLE *table;

    if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_LOAD_CONFIG_DIRECTORY64 *cfg = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size);
        if (!cfg) return NULL;
        size = min( size, cfg->Size );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY64, DynamicValueRelocTableSection )) return NULL;
        offset = cfg->DynamicValueRelocTableOffset;
        section = cfg->DynamicValueRelocTableSection;
    }
    else
    {
        const IMAGE_LOAD_CONFIG_DIRECTORY32 *cfg = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG, &size);
        if (!cfg) return NULL;
        size = min( size, cfg->Size );
        if (size <= offsetof( IMAGE_LOAD_CONFIG_DIRECTORY32, DynamicValueRelocTableSection )) return NULL;
        offset = cfg->DynamicValueRelocTableOffset;
        section = cfg->DynamicValueRelocTableSection;
    }
    if (!section || section > PE_nt_headers->FileHeader.NumberOfSections) return NULL;
    sec = IMAGE_FIRST_SECTION( PE_nt_headers ) + section - 1;
    if (offset >= sec->SizeOfRawData) return NULL;
    return PRD( sec->PointerToRawData + offset, sizeof(*table) );
}

static void dump_dir_dynamic_reloc(void)
{
    const char *ptr, *end;
    const IMAGE_DYNAMIC_RELOCATION_TABLE *table = get_dyn_reloc_table();

    if (!table) return;

    printf( "Dynamic relocations (version %u)\n\n", (UINT)table->Version );
    ptr = (const char *)(table + 1);
    end = ptr + table->Size;
    while (ptr < end)
    {
        switch (table->Version)
        {
        case 1:
            if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                const IMAGE_DYNAMIC_RELOCATION64 *reloc = (const IMAGE_DYNAMIC_RELOCATION64 *)ptr;
                dump_dynamic_relocs( (const char *)(reloc + 1), reloc->BaseRelocSize, reloc->Symbol );
                ptr += sizeof(*reloc) + reloc->BaseRelocSize;
            }
            else
            {
                const IMAGE_DYNAMIC_RELOCATION32 *reloc = (const IMAGE_DYNAMIC_RELOCATION32 *)ptr;
                dump_dynamic_relocs( (const char *)(reloc + 1), reloc->BaseRelocSize, reloc->Symbol );
                ptr += sizeof(*reloc) + reloc->BaseRelocSize;
            }
            break;
        case 2:
            if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                const IMAGE_DYNAMIC_RELOCATION64_V2 *reloc = (const IMAGE_DYNAMIC_RELOCATION64_V2 *)ptr;
                dump_dynamic_relocs( ptr + reloc->HeaderSize, reloc->FixupInfoSize, reloc->Symbol );
                ptr += reloc->HeaderSize + reloc->FixupInfoSize;
            }
            else
            {
                const IMAGE_DYNAMIC_RELOCATION32_V2 *reloc = (const IMAGE_DYNAMIC_RELOCATION32_V2 *)ptr;
                dump_dynamic_relocs( ptr + reloc->HeaderSize, reloc->FixupInfoSize, reloc->Symbol );
                ptr += reloc->HeaderSize + reloc->FixupInfoSize;
            }
            break;
        }
    }
    printf( "\n" );
}

static const IMAGE_BASE_RELOCATION *get_armx_relocs( unsigned int *size )
{
    const char *ptr, *end;
    const IMAGE_DYNAMIC_RELOCATION_TABLE *table = get_dyn_reloc_table();

    if (!table) return NULL;
    ptr = (const char *)(table + 1);
    end = ptr + table->Size;
    while (ptr < end)
    {
        switch (table->Version)
        {
        case 1:
            if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                const IMAGE_DYNAMIC_RELOCATION64 *reloc = (const IMAGE_DYNAMIC_RELOCATION64 *)ptr;
                if (reloc->Symbol == IMAGE_DYNAMIC_RELOCATION_ARM64X)
                {
                    *size = reloc->BaseRelocSize;
                    return (const IMAGE_BASE_RELOCATION *)(reloc + 1);
                }
                ptr += sizeof(*reloc) + reloc->BaseRelocSize;
            }
            else
            {
                const IMAGE_DYNAMIC_RELOCATION32 *reloc = (const IMAGE_DYNAMIC_RELOCATION32 *)ptr;
                if (reloc->Symbol == IMAGE_DYNAMIC_RELOCATION_ARM64X)
                {
                    *size = reloc->BaseRelocSize;
                    return (const IMAGE_BASE_RELOCATION *)(reloc + 1);
                }
                ptr += sizeof(*reloc) + reloc->BaseRelocSize;
            }
            break;
        case 2:
            if (PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            {
                const IMAGE_DYNAMIC_RELOCATION64_V2 *reloc = (const IMAGE_DYNAMIC_RELOCATION64_V2 *)ptr;
                if (reloc->Symbol == IMAGE_DYNAMIC_RELOCATION_ARM64X)
                {
                    *size = reloc->FixupInfoSize;
                    return (const IMAGE_BASE_RELOCATION *)(reloc + 1);
                }
                ptr += reloc->HeaderSize + reloc->FixupInfoSize;
            }
            else
            {
                const IMAGE_DYNAMIC_RELOCATION32_V2 *reloc = (const IMAGE_DYNAMIC_RELOCATION32_V2 *)ptr;
                if (reloc->Symbol == IMAGE_DYNAMIC_RELOCATION_ARM64X)
                {
                    *size = reloc->FixupInfoSize;
                    return (const IMAGE_BASE_RELOCATION *)(reloc + 1);
                }
                ptr += reloc->HeaderSize + reloc->FixupInfoSize;
            }
            break;
        }
    }
    return NULL;
}

static BOOL get_alt_header( void )
{
    unsigned int size;
    const IMAGE_BASE_RELOCATION *end, *reloc = get_armx_relocs( &size );

    if (!reloc) return FALSE;
    end = (const IMAGE_BASE_RELOCATION *)((const char *)reloc + size);

    while (reloc < end - 1 && reloc->SizeOfBlock)
    {
        const USHORT *rel = (const USHORT *)(reloc + 1);
        const USHORT *rel_end = (const USHORT *)reloc + reloc->SizeOfBlock / sizeof(USHORT);
        char *page = reloc->VirtualAddress ? (char *)RVA(reloc->VirtualAddress,1) : dump_base;

        while (rel < rel_end && *rel)
        {
            USHORT offset = *rel & 0xfff;
            USHORT type = (*rel >> 12) & 3;
            USHORT arg = *rel >> 14;
            int val;
            rel++;
            switch (type)
            {
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_ZEROFILL:
                memset( page + offset, 0, 1 << arg );
                break;
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_VALUE:
                memcpy( page + offset, rel, 1 << arg );
                rel += (1 << arg) / sizeof(USHORT);
                break;
            case IMAGE_DVRT_ARM64X_FIXUP_TYPE_DELTA:
                val = (unsigned int)*rel++ * ((arg & 2) ? 8 : 4);
                if (arg & 1) val = -val;
                *(int *)(page + offset) += val;
                break;
            }
        }
        reloc = (const IMAGE_BASE_RELOCATION *)rel_end;
    }
    return TRUE;
}

static void dump_dir_reloc(void)
{
    unsigned int i, size = 0;
    const USHORT *relocs;
    const IMAGE_BASE_RELOCATION *rel = get_dir_and_size(IMAGE_DIRECTORY_ENTRY_BASERELOC, &size);
    const IMAGE_BASE_RELOCATION *end = (const IMAGE_BASE_RELOCATION *)((const char *)rel + size);
    static const char * const names[] =
    {
        "BASED_ABSOLUTE",
        "BASED_HIGH",
        "BASED_LOW",
        "BASED_HIGHLOW",
        "BASED_HIGHADJ",
        "BASED_MIPS_JMPADDR",
        "BASED_SECTION",
        "BASED_REL",
        "unknown 8",
        "BASED_IA64_IMM64",
        "BASED_DIR64",
        "BASED_HIGH3ADJ",
        "unknown 12",
        "unknown 13",
        "unknown 14",
        "unknown 15"
    };

    if (!rel) return;

    printf( "Relocations\n" );
    while (rel < end - 1 && rel->SizeOfBlock)
    {
        printf( "  Page %x\n", (UINT)rel->VirtualAddress );
        relocs = (const USHORT *)(rel + 1);
        i = (rel->SizeOfBlock - sizeof(*rel)) / sizeof(USHORT);
        while (i--)
        {
            USHORT offset = *relocs & 0xfff;
            int type = *relocs >> 12;
            printf( "    off %04x type %s\n", offset, names[type] );
            relocs++;
        }
        rel = (const IMAGE_BASE_RELOCATION *)relocs;
    }
    printf("\n");
}

static void dump_dir_tls(void)
{
    IMAGE_TLS_DIRECTORY64 dir;
    const UINT *callbacks;
    const IMAGE_TLS_DIRECTORY32 *pdir = get_dir(IMAGE_FILE_THREAD_LOCAL_STORAGE);

    if (!pdir) return;

    if(PE_nt_headers->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        memcpy(&dir, pdir, sizeof(dir));
    else
    {
        dir.StartAddressOfRawData = pdir->StartAddressOfRawData;
        dir.EndAddressOfRawData = pdir->EndAddressOfRawData;
        dir.AddressOfIndex = pdir->AddressOfIndex;
        dir.AddressOfCallBacks = pdir->AddressOfCallBacks;
        dir.SizeOfZeroFill = pdir->SizeOfZeroFill;
        dir.Characteristics = pdir->Characteristics;
    }

    /* FIXME: This does not properly handle large images */
    printf( "Thread Local Storage\n" );
    printf( "  Raw data        %08x-%08x (data size %x zero fill size %x)\n",
            (UINT)dir.StartAddressOfRawData, (UINT)dir.EndAddressOfRawData,
            (UINT)(dir.EndAddressOfRawData - dir.StartAddressOfRawData),
            (UINT)dir.SizeOfZeroFill );
    printf( "  Index address   %08x\n", (UINT)dir.AddressOfIndex );
    printf( "  Characteristics %08x\n", (UINT)dir.Characteristics );
    printf( "  Callbacks       %08x -> {", (UINT)dir.AddressOfCallBacks );
    if (dir.AddressOfCallBacks)
    {
        UINT   addr = (UINT)dir.AddressOfCallBacks - PE_nt_headers->OptionalHeader.ImageBase;
        while ((callbacks = RVA(addr, sizeof(UINT))) && *callbacks)
        {
            printf( " %08x", *callbacks );
            addr += sizeof(UINT);
        }
    }
    printf(" }\n\n");
}

enum FileSig get_kind_dbg(void)
{
    const WORD*                pw;

    pw = PRD(0, sizeof(WORD));
    if (!pw) {printf("Can't get main signature, aborting\n"); return 0;}

    if (*pw == 0x4944 /* "DI" */) return SIG_DBG;
    return SIG_UNKNOWN;
}

void	dbg_dump(void)
{
    const IMAGE_SEPARATE_DEBUG_HEADER*  separateDebugHead;
    unsigned			        nb_dbg;
    unsigned			        i;
    const IMAGE_DEBUG_DIRECTORY*	debugDir;

    separateDebugHead = PRD(0, sizeof(*separateDebugHead));
    if (!separateDebugHead) {printf("Can't grab the separate header, aborting\n"); return;}

    printf ("Signature:          %.2s (0x%4X)\n",
	    (const char*)&separateDebugHead->Signature, separateDebugHead->Signature);
    printf ("Flags:              0x%04X\n", separateDebugHead->Flags);
    printf ("Machine:            0x%04X (%s)\n",
	    separateDebugHead->Machine, get_machine_str(separateDebugHead->Machine));
    printf ("Characteristics:    0x%04X\n", separateDebugHead->Characteristics);
    printf ("TimeDateStamp:      0x%08X (%s)\n",
	    (UINT)separateDebugHead->TimeDateStamp, get_time_str(separateDebugHead->TimeDateStamp));
    printf ("CheckSum:           0x%08X\n", (UINT)separateDebugHead->CheckSum);
    printf ("ImageBase:          0x%08X\n", (UINT)separateDebugHead->ImageBase);
    printf ("SizeOfImage:        0x%08X\n", (UINT)separateDebugHead->SizeOfImage);
    printf ("NumberOfSections:   0x%08X\n", (UINT)separateDebugHead->NumberOfSections);
    printf ("ExportedNamesSize:  0x%08X\n", (UINT)separateDebugHead->ExportedNamesSize);
    printf ("DebugDirectorySize: 0x%08X\n", (UINT)separateDebugHead->DebugDirectorySize);

    if (!PRD(sizeof(IMAGE_SEPARATE_DEBUG_HEADER),
	     separateDebugHead->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)))
    {printf("Can't get the sections, aborting\n"); return;}

    dump_sections(separateDebugHead, separateDebugHead + 1, separateDebugHead->NumberOfSections);

    nb_dbg = separateDebugHead->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
    debugDir = PRD(sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
		   separateDebugHead->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
		   separateDebugHead->ExportedNamesSize,
		   nb_dbg * sizeof(IMAGE_DEBUG_DIRECTORY));
    if (!debugDir) {printf("Couldn't get the debug directory info, aborting\n");return;}

    printf("Debug Table (%u directories)\n", nb_dbg);

    for (i = 0; i < nb_dbg; i++)
    {
	dump_dir_debug_dir(debugDir, i, (const IMAGE_SECTION_HEADER*)(separateDebugHead + 1));
	debugDir++;
    }
}

static const char *get_resource_type( unsigned int id )
{
    static const char * const types[] =
    {
        NULL,
        "CURSOR",
        "BITMAP",
        "ICON",
        "MENU",
        "DIALOG",
        "STRING",
        "FONTDIR",
        "FONT",
        "ACCELERATOR",
        "RCDATA",
        "MESSAGETABLE",
        "GROUP_CURSOR",
        NULL,
        "GROUP_ICON",
        NULL,
        "VERSION",
        "DLGINCLUDE",
        NULL,
        "PLUGPLAY",
        "VXD",
        "ANICURSOR",
        "ANIICON",
        "HTML",
        "MANIFEST"
    };

    if ((size_t)id < ARRAY_SIZE(types)) return types[id];
    return NULL;
}

/* dump an ASCII string with proper escaping */
static int dump_strA( const unsigned char *str, size_t len )
{
    static const char escapes[32] = ".......abtnvfr.............e....";
    char buffer[256];
    char *pos = buffer;
    int count = 0;

    for (; len; str++, len--)
    {
        if (pos > buffer + sizeof(buffer) - 8)
        {
            fwrite( buffer, pos - buffer, 1, stdout );
            count += pos - buffer;
            pos = buffer;
        }
        if (*str > 127)  /* hex escape */
        {
            pos += sprintf( pos, "\\x%02x", *str );
            continue;
        }
        if (*str < 32)  /* octal or C escape */
        {
            if (!*str && len == 1) continue;  /* do not output terminating NULL */
            if (escapes[*str] != '.')
                pos += sprintf( pos, "\\%c", escapes[*str] );
            else if (len > 1 && str[1] >= '0' && str[1] <= '7')
                pos += sprintf( pos, "\\%03o", *str );
            else
                pos += sprintf( pos, "\\%o", *str );
            continue;
        }
        if (*str == '\\') *pos++ = '\\';
        *pos++ = *str;
    }
    fwrite( buffer, pos - buffer, 1, stdout );
    count += pos - buffer;
    return count;
}

/* dump a Unicode string with proper escaping */
static int dump_strW( const WCHAR *str, size_t len )
{
    static const char escapes[32] = ".......abtnvfr.............e....";
    char buffer[256];
    char *pos = buffer;
    int count = 0;

    for (; len; str++, len--)
    {
        if (pos > buffer + sizeof(buffer) - 8)
        {
            fwrite( buffer, pos - buffer, 1, stdout );
            count += pos - buffer;
            pos = buffer;
        }
        if (*str > 127)  /* hex escape */
        {
            if (len > 1 && str[1] < 128 && isxdigit((char)str[1]))
                pos += sprintf( pos, "\\x%04x", *str );
            else
                pos += sprintf( pos, "\\x%x", *str );
            continue;
        }
        if (*str < 32)  /* octal or C escape */
        {
            if (!*str && len == 1) continue;  /* do not output terminating NULL */
            if (escapes[*str] != '.')
                pos += sprintf( pos, "\\%c", escapes[*str] );
            else if (len > 1 && str[1] >= '0' && str[1] <= '7')
                pos += sprintf( pos, "\\%03o", *str );
            else
                pos += sprintf( pos, "\\%o", *str );
            continue;
        }
        if (*str == '\\') *pos++ = '\\';
        *pos++ = *str;
    }
    fwrite( buffer, pos - buffer, 1, stdout );
    count += pos - buffer;
    return count;
}

/* dump data for a STRING resource */
static void dump_string_data( const WCHAR *ptr, unsigned int size, unsigned int id, const char *prefix )
{
    int i;

    for (i = 0; i < 16 && size; i++)
    {
        unsigned len = *ptr++;

        if (len >= size)
        {
            len = size;
            size = 0;
        }
        else size -= len + 1;

        if (len)
        {
            printf( "%s%04x \"", prefix, (id - 1) * 16 + i );
            dump_strW( ptr, len );
            printf( "\"\n" );
            ptr += len;
        }
    }
}

/* dump data for a MESSAGETABLE resource */
static void dump_msgtable_data( const void *ptr, unsigned int size, const char *prefix )
{
    const MESSAGE_RESOURCE_DATA *data = ptr;
    const MESSAGE_RESOURCE_BLOCK *block = data->Blocks;
    unsigned i, j;

    for (i = 0; i < data->NumberOfBlocks; i++, block++)
    {
        const MESSAGE_RESOURCE_ENTRY *entry;

        entry = (const MESSAGE_RESOURCE_ENTRY *)((const char *)data + block->OffsetToEntries);
        for (j = block->LowId; j <= block->HighId; j++)
        {
            if (entry->Flags & MESSAGE_RESOURCE_UNICODE)
            {
                const WCHAR *str = (const WCHAR *)entry->Text;
                printf( "%s%08x L\"", prefix, j );
                dump_strW( str, strlenW(str) );
                printf( "\"\n" );
            }
            else
            {
                const char *str = (const char *) entry->Text;
                printf( "%s%08x \"", prefix, j );
                dump_strA( entry->Text, strlen(str) );
                printf( "\"\n" );
            }
            entry = (const MESSAGE_RESOURCE_ENTRY *)((const char *)entry + entry->Length);
        }
    }
}

struct version_info
{
    WORD  len;
    WORD  val_len;
    WORD  type;
    WCHAR key[1];
};
#define GET_VALUE(info) ((void *)((char *)info + ((offsetof(struct version_info, key[strlenW(info->key) + 1]) + 3) & ~3)))
#define GET_CHILD(info) ((void *)((char *)GET_VALUE(info) + ((info->val_len * (info->type ? 2 : 1) + 3) & ~3)))
#define GET_NEXT(info)  ((void *)((char *)info + ((info->len + 3) & ~3)))

static void dump_version_children( const struct version_info *info, const char *prefix, int indent )
{
    const struct version_info *next, *child = GET_CHILD( info );

    for ( ; (char *)child < (char *)info + info->len; child = next)
    {
        next = GET_NEXT( child );
        printf( "%s%*s", prefix, indent * 2, "" );
        if (child->val_len || GET_VALUE( child ) == next)
        {
            printf( "VALUE \"" );
            dump_strW( child->key, strlenW(child->key) );
            if (child->type)
            {
                printf( "\", \"" );
                dump_strW( GET_VALUE(child), child->val_len );
                printf( "\"\n" );
            }
            else
            {
                const WORD *data = GET_VALUE(child);
                unsigned int i;
                printf( "\"," );
                for (i = 0; i < child->val_len / sizeof(WORD); i++) printf( " %#x", data[i] );
                printf( "\n" );
            }
        }
        else
        {
            printf( "BLOCK \"" );
            dump_strW( child->key, strlenW(child->key) );
            printf( "\"\n" );
        }
        dump_version_children( child, prefix, indent + 1 );
    }
}

/* dump data for a VERSION resource */
static void dump_version_data( const void *ptr, unsigned int size, const char *prefix )
{
    const struct version_info *info = ptr;
    const VS_FIXEDFILEINFO *fileinfo = GET_VALUE( info );

    printf( "%sSIGNATURE      %08x\n", prefix, (UINT)fileinfo->dwSignature );
    printf( "%sVERSION        %u.%u\n", prefix,
            HIWORD(fileinfo->dwStrucVersion), LOWORD(fileinfo->dwStrucVersion) );
    printf( "%sFILEVERSION    %u.%u.%u.%u\n", prefix,
            HIWORD(fileinfo->dwFileVersionMS), LOWORD(fileinfo->dwFileVersionMS),
            HIWORD(fileinfo->dwFileVersionLS), LOWORD(fileinfo->dwFileVersionLS) );
    printf( "%sPRODUCTVERSION %u.%u.%u.%u\n", prefix,
            HIWORD(fileinfo->dwProductVersionMS), LOWORD(fileinfo->dwProductVersionMS),
            HIWORD(fileinfo->dwProductVersionLS), LOWORD(fileinfo->dwProductVersionLS) );
    printf( "%sFILEFLAGSMASK  %08x\n", prefix, (UINT)fileinfo->dwFileFlagsMask );
    printf( "%sFILEFLAGS      %08x\n", prefix, (UINT)fileinfo->dwFileFlags );

    switch (fileinfo->dwFileOS)
    {
#define CASE(x) case x: printf( "%sFILEOS         %s\n", prefix, #x ); break
        CASE(VOS_UNKNOWN);
        CASE(VOS_DOS_WINDOWS16);
        CASE(VOS_DOS_WINDOWS32);
        CASE(VOS_OS216_PM16);
        CASE(VOS_OS232_PM32);
        CASE(VOS_NT_WINDOWS32);
#undef CASE
    default:
        printf( "%sFILEOS         %u.%u\n", prefix,
                (WORD)(fileinfo->dwFileOS >> 16), (WORD)fileinfo->dwFileOS );
        break;
    }

    switch (fileinfo->dwFileType)
    {
#define CASE(x) case x: printf( "%sFILETYPE       %s\n", prefix, #x ); break
        CASE(VFT_UNKNOWN);
        CASE(VFT_APP);
        CASE(VFT_DLL);
        CASE(VFT_DRV);
        CASE(VFT_FONT);
        CASE(VFT_VXD);
        CASE(VFT_STATIC_LIB);
#undef CASE
    default:
        printf( "%sFILETYPE       %08x\n", prefix, (UINT)fileinfo->dwFileType );
        break;
    }

    switch (((ULONGLONG)fileinfo->dwFileType << 32) + fileinfo->dwFileSubtype)
    {
#define CASE(t,x) case (((ULONGLONG)t << 32) + x): printf( "%sFILESUBTYPE    %s\n", prefix, #x ); break
        CASE(VFT_DRV, VFT2_UNKNOWN);
        CASE(VFT_DRV, VFT2_DRV_PRINTER);
        CASE(VFT_DRV, VFT2_DRV_KEYBOARD);
        CASE(VFT_DRV, VFT2_DRV_LANGUAGE);
        CASE(VFT_DRV, VFT2_DRV_DISPLAY);
        CASE(VFT_DRV, VFT2_DRV_MOUSE);
        CASE(VFT_DRV, VFT2_DRV_NETWORK);
        CASE(VFT_DRV, VFT2_DRV_SYSTEM);
        CASE(VFT_DRV, VFT2_DRV_INSTALLABLE);
        CASE(VFT_DRV, VFT2_DRV_SOUND);
        CASE(VFT_DRV, VFT2_DRV_COMM);
        CASE(VFT_DRV, VFT2_DRV_INPUTMETHOD);
        CASE(VFT_DRV, VFT2_DRV_VERSIONED_PRINTER);
        CASE(VFT_FONT, VFT2_FONT_RASTER);
        CASE(VFT_FONT, VFT2_FONT_VECTOR);
        CASE(VFT_FONT, VFT2_FONT_TRUETYPE);
#undef CASE
    default:
        printf( "%sFILESUBTYPE    %08x\n", prefix, (UINT)fileinfo->dwFileSubtype );
        break;
    }

    printf( "%sFILEDATE       %08x.%08x\n", prefix,
            (UINT)fileinfo->dwFileDateMS, (UINT)fileinfo->dwFileDateLS );
    dump_version_children( info, prefix, 0 );
}

/* dump data for a HTML/MANIFEST resource */
static void dump_text_data( const void *ptr, unsigned int size, const char *prefix )
{
    const char *p = ptr, *end = p + size;

    while (p < end)
    {
        const char *start = p;
        while (p < end && *p != '\r' && *p != '\n') p++;
        printf( "%s%.*s\n", prefix, (int)(p - start), start );
        while (p < end && (*p == '\r' || *p == '\n')) p++;
    }
}

/* dump data for an MUI resource */
static void dump_mui_data( const void *ptr, unsigned int size, const char *prefix )
{
    UINT i;

    const struct mui_resource
    {
        UINT signature;
        UINT size;
        UINT version;
        UINT path_type;
        UINT file_type;
        UINT system_attributes;
        UINT fallback_location;
        BYTE service_checksum[16];
        BYTE checksum[16];
        UINT unk1[2];
        UINT mui_path_off;
        UINT mui_path_size;
        UINT unk2[2];
        UINT ln_type_name_off;
        UINT ln_type_name_size;
        UINT ln_type_id_off;
        UINT ln_type_id_size;
        UINT mui_type_name_off;
        UINT mui_type_name_size;
        UINT mui_type_id_off;
        UINT mui_type_id_size;
        UINT lang_off;
        UINT lang_size;
        UINT fallback_lang_off;
        UINT fallback_lang_size;
    } *data = ptr;

    printf( "%sSignature:     %x\n", prefix, data->signature );
    printf( "%sVersion:       %u.%u\n", prefix, HIWORD(data->version), LOWORD(data->version) );
    printf( "%sPath type:     %x\n", prefix, data->path_type );
    printf( "%sFile type:     %x\n", prefix, data->file_type );
    printf( "%sAttributes:    %x\n", prefix, data->system_attributes );
    printf( "%sFallback:      %x\n", prefix, data->fallback_location );
    printf( "%sChecksum:      ", prefix );
    for (i = 0; i < sizeof(data->checksum); i++) printf( "%02x", data->checksum[i] );
    printf( "\n%sSvc checksum:  ", prefix );
    for (i = 0; i < sizeof(data->service_checksum); i++) printf( "%02x", data->service_checksum[i] );
    printf( "\n" );
    printf( "%sUnknown1:      %08x %08x\n", prefix, data->unk1[0], data->unk1[1]  );
    printf( "%sUnknown2:      %08x %08x\n", prefix, data->unk2[0], data->unk2[1]  );

    if (data->mui_path_off)
    {
        const WCHAR *name = (WCHAR *)((char *)data + data->mui_path_off);
        printf( "%sMUI path:      ", prefix );
        dump_strW( name, strlenW(name) );
        printf( "\n" );
    }

    printf( "%sResources:     {", prefix );
    if (data->ln_type_name_off)
    {
        const WCHAR *names = (WCHAR *)((char *)data + data->ln_type_name_off);
        while (*names)
        {
            printf( " " );
            dump_strW( names, strlenW(names) );
            names += strlenW( names ) + 1;
        }
    }
    if (data->ln_type_id_off)
    {
        const UINT *types = (UINT *)((char *)data + data->ln_type_id_off);
        for (i = 0; i < data->ln_type_id_size / sizeof(*types); i++)
        {
            const char *type = get_resource_type( types[i] );
            if (type) printf( " %s", type );
            else printf( " %04x", types[i] );
        }
    }
    printf( " }\n" );

    printf( "%sMUI resources: {", prefix );
    if (data->mui_type_name_off)
    {
        const WCHAR *names = (WCHAR *)((char *)data + data->mui_type_name_off);
        while (*names)
        {
            printf( " " );
            dump_strW( names, strlenW(names) );
            names += strlenW( names ) + 1;
        }
    }
    if (data->mui_type_id_off)
    {
        const UINT *types = (UINT *)((char *)data + data->mui_type_id_off);
        for (i = 0; i < data->mui_type_id_size / sizeof(*types); i++)
        {
            const char *type = get_resource_type( types[i] );
            if (type) printf( " %s", type );
            else printf( " %04x", types[i] );
        }
    }
    printf( " }\n" );

    if (data->lang_off)
    {
        const WCHAR *name = (WCHAR *)((char *)data + data->lang_off);
        printf( "%sLanguage:      ", prefix );
        dump_strW( name, strlenW(name) );
        printf( "\n" );
    }

    if (data->fallback_lang_off)
    {
        const WCHAR *name = (WCHAR *)((char *)data + data->fallback_lang_off);
        printf( "%sFallback lang: ", prefix );
        dump_strW( name, strlenW(name) );
        printf( "\n" );
    }
    dump_data( ptr, size, prefix );
}

static int cmp_resource_name( const IMAGE_RESOURCE_DIR_STRING_U *str_res, const char *str )
{
    unsigned int i;

    for (i = 0; i < str_res->Length; i++)
    {
        int res = str_res->NameString[i] - (WCHAR)str[i];
        if (res || !str[i]) return res;
    }
    return -(WCHAR)str[i];
}

static void dump_dir_resource(void)
{
    const IMAGE_RESOURCE_DIRECTORY *root = get_dir(IMAGE_FILE_RESOURCE_DIRECTORY);
    const IMAGE_RESOURCE_DIRECTORY *namedir;
    const IMAGE_RESOURCE_DIRECTORY *langdir;
    const IMAGE_RESOURCE_DIRECTORY_ENTRY *e1, *e2, *e3;
    const IMAGE_RESOURCE_DIR_STRING_U *string;
    const IMAGE_RESOURCE_DATA_ENTRY *data;
    int i, j, k;

    if (!root) return;

    printf( "Resources:" );

    for (i = 0; i< root->NumberOfNamedEntries + root->NumberOfIdEntries; i++)
    {
        e1 = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(root + 1) + i;
        namedir = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + e1->OffsetToDirectory);
        for (j = 0; j < namedir->NumberOfNamedEntries + namedir->NumberOfIdEntries; j++)
        {
            e2 = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(namedir + 1) + j;
            langdir = (const IMAGE_RESOURCE_DIRECTORY *)((const char *)root + e2->OffsetToDirectory);
            for (k = 0; k < langdir->NumberOfNamedEntries + langdir->NumberOfIdEntries; k++)
            {
                e3 = (const IMAGE_RESOURCE_DIRECTORY_ENTRY*)(langdir + 1) + k;

                printf( "\n  " );
                if (e1->NameIsString)
                {
                    string = (const IMAGE_RESOURCE_DIR_STRING_U*)((const char *)root + e1->NameOffset);
                    dump_unicode_str( string->NameString, string->Length );
                }
                else
                {
                    const char *type = get_resource_type( e1->Id );
                    if (type) printf( "%s", type );
                    else printf( "%04x", e1->Id );
                }

                printf( " Name=" );
                if (e2->NameIsString)
                {
                    string = (const IMAGE_RESOURCE_DIR_STRING_U*) ((const char *)root + e2->NameOffset);
                    dump_unicode_str( string->NameString, string->Length );
                }
                else
                    printf( "%04x", e2->Id );

                printf( " Language=%04x:\n", e3->Id );
                data = (const IMAGE_RESOURCE_DATA_ENTRY *)((const char *)root + e3->OffsetToData);
                if (e1->NameIsString)
                {
                    string = (const IMAGE_RESOURCE_DIR_STRING_U*)((const char *)root + e1->NameOffset);
                    if (!cmp_resource_name( string, "TYPELIB" ))
                        tlb_dump_resource( (void *)RVA( data->OffsetToData, data->Size ), data->Size, "  |  " );
                    else if (!cmp_resource_name( string, "MUI" ))
                        dump_mui_data( RVA( data->OffsetToData, data->Size ), data->Size, "  |  " );
                    else if (!cmp_resource_name( string, "WINE_REGISTRY" ))
                        dump_text_data( RVA( data->OffsetToData, data->Size ), data->Size, "  |  " );
                    else
                        dump_data( RVA( data->OffsetToData, data->Size ), data->Size, "    " );
                }
                else switch(e1->Id)
                {
                case 6:  /* RT_STRING */
                    dump_string_data( RVA( data->OffsetToData, data->Size ), data->Size, e2->Id, "    " );
                    break;
                case 11:  /* RT_MESSAGETABLE */
                    dump_msgtable_data( RVA( data->OffsetToData, data->Size ), data->Size, "    " );
                    break;
                case 16:  /* RT_VERSION */
                    dump_version_data( RVA( data->OffsetToData, data->Size ), data->Size, "  |  " );
                    break;
                case 23:  /* RT_HTML */
                case 24:  /* RT_MANIFEST */
                    dump_text_data( RVA( data->OffsetToData, data->Size ), data->Size, "  |  " );
                    break;
                default:
                    dump_data( RVA( data->OffsetToData, data->Size ), data->Size, "    " );
                    break;
                }
            }
        }
    }
    printf( "\n\n" );
}

static void dump_debug(void)
{
    const char* stabs = NULL;
    unsigned    szstabs = 0;
    const char* stabstr = NULL;
    unsigned    szstr = 0;
    unsigned    i;
    const IMAGE_SECTION_HEADER*	sectHead;

    sectHead = IMAGE_FIRST_SECTION(PE_nt_headers);

    for (i = 0; i < PE_nt_headers->FileHeader.NumberOfSections; i++, sectHead++)
    {
        if (!strcmp((const char *)sectHead->Name, ".stab"))
        {
            stabs = RVA(sectHead->VirtualAddress, sectHead->Misc.VirtualSize); 
            szstabs = sectHead->Misc.VirtualSize;
        }
        if (!strncmp((const char *)sectHead->Name, ".stabstr", 8))
        {
            stabstr = RVA(sectHead->VirtualAddress, sectHead->Misc.VirtualSize);
            szstr = sectHead->Misc.VirtualSize;
        }
    }
    if (stabs && stabstr)
        dump_stabs(stabs, szstabs, stabstr, szstr);
}

static void dump_symbol_table(void)
{
    const IMAGE_SYMBOL* sym;
    int                 numsym;

    numsym = PE_nt_headers->FileHeader.NumberOfSymbols;
    if (!PE_nt_headers->FileHeader.PointerToSymbolTable || !numsym)
        return;
    sym = PRD(PE_nt_headers->FileHeader.PointerToSymbolTable,
                                   sizeof(*sym) * numsym);
    if (!sym) return;

    dump_coff_symbol_table(sym, numsym, IMAGE_FIRST_SECTION(PE_nt_headers));
}

enum FileSig get_kind_exec(void)
{
    const WORD*                pw;
    const DWORD*               pdw;
    const IMAGE_DOS_HEADER*    dh;

    pw = PRD(0, sizeof(WORD));
    if (!pw) {printf("Can't get main signature, aborting\n"); return 0;}

    if (*pw != IMAGE_DOS_SIGNATURE) return SIG_UNKNOWN;

    if ((dh = PRD(0, sizeof(IMAGE_DOS_HEADER))))
    {
        /* the signature is the first DWORD */
        pdw = PRD(dh->e_lfanew, sizeof(DWORD));
        if (pdw)
        {
            if (*pdw == IMAGE_NT_SIGNATURE)                     return SIG_PE;
            if (*(const WORD *)pdw == IMAGE_OS2_SIGNATURE)      return SIG_NE;
            if (*(const WORD *)pdw == IMAGE_VXD_SIGNATURE)      return SIG_LE;
        }
        return SIG_DOS;
    }
    return SIG_UNKNOWN;
}

void pe_dump(void)
{
    int alt = 0;

    PE_nt_headers = get_nt_header();
    print_fake_dll();

    for (;;)
    {
        if (alt)
            printf( "\n**** Alternate (%s) data ****\n\n",
                    get_machine_str(PE_nt_headers->FileHeader.Machine));
        else if (get_dyn_reloc_table())
            printf( "**** Native (%s) data ****\n\n",
                    get_machine_str(PE_nt_headers->FileHeader.Machine));

        if (globals.do_dumpheader)
        {
            dump_pe_header();
            /* FIXME: should check ptr */
            dump_sections(PRD(0, 1), IMAGE_FIRST_SECTION(PE_nt_headers),
                          PE_nt_headers->FileHeader.NumberOfSections);
        }
        else if (!globals.dumpsect)
        {
            /* show at least something here */
            dump_pe_header();
        }

        if (globals_dump_sect("import"))
        {
            dump_dir_imported_functions();
            dump_dir_delay_imported_functions();
        }
        if (globals_dump_sect("export"))
            dump_dir_exported_functions();
        if (globals_dump_sect("debug"))
            dump_dir_debug();
        if (globals_dump_sect("resource"))
            dump_dir_resource();
        if (globals_dump_sect("tls"))
            dump_dir_tls();
        if (globals_dump_sect("loadcfg"))
            dump_dir_loadconfig();
        if (globals_dump_sect("clr"))
            dump_dir_clr_header();
        if (globals_dump_sect("reloc"))
            dump_dir_reloc();
        if (globals_dump_sect("dynreloc"))
            dump_dir_dynamic_reloc();
        if (globals_dump_sect("except"))
            dump_dir_exceptions();
        if (globals_dump_sect("apiset"))
            dump_section_apiset();

        if (globals.do_symbol_table)
            dump_symbol_table();
        if (globals.do_debug)
            dump_debug();
        if (alt++) break;
        if (!get_alt_header()) break;
    }
}

typedef struct _dll_symbol {
    size_t	ordinal;
    char       *symbol;
} dll_symbol;

static dll_symbol *dll_symbols = NULL;
static dll_symbol *dll_current_symbol = NULL;

/* Compare symbols by ordinal for qsort */
static int symbol_cmp(const void *left, const void *right)
{
    return ((const dll_symbol *)left)->ordinal > ((const dll_symbol *)right)->ordinal;
}

/*******************************************************************
 *         dll_close
 *
 * Free resources used by DLL
 */
/* FIXME: Not used yet
static void dll_close (void)
{
    dll_symbol*	ds;

    if (!dll_symbols) {
	fatal("No symbols");
    }
    for (ds = dll_symbols; ds->symbol; ds++)
	free(ds->symbol);
    free (dll_symbols);
    dll_symbols = NULL;
}
*/

static	void	do_grab_sym( void )
{
    const IMAGE_EXPORT_DIRECTORY*exportDir;
    UINT i, j, *map;
    const UINT *pName;
    const UINT *pFunc;
    const WORD *pOrdl;
    const char *ptr;

    PE_nt_headers = get_nt_header();
    if (!(exportDir = get_dir(IMAGE_FILE_EXPORT_DIRECTORY))) return;

    pName = RVA(exportDir->AddressOfNames, exportDir->NumberOfNames * sizeof(DWORD));
    if (!pName) {printf("Can't grab functions' name table\n"); return;}
    pOrdl = RVA(exportDir->AddressOfNameOrdinals, exportDir->NumberOfNames * sizeof(WORD));
    if (!pOrdl) {printf("Can't grab functions' ordinal table\n"); return;}
    pFunc = RVA(exportDir->AddressOfFunctions, exportDir->NumberOfFunctions * sizeof(DWORD));
    if (!pFunc) {printf("Can't grab functions' address table\n"); return;}

    /* dll_close(); */

    dll_symbols = xmalloc((exportDir->NumberOfFunctions + 1) * sizeof(dll_symbol));

    /* bit map of used funcs */
    map = calloc(((exportDir->NumberOfFunctions + 31) & ~31) / 32, sizeof(DWORD));
    if (!map) fatal("no memory");

    for (j = 0; j < exportDir->NumberOfNames; j++, pOrdl++)
    {
	map[*pOrdl / 32] |= 1 << (*pOrdl % 32);
	ptr = RVA(*pName++, sizeof(DWORD));
	if (!ptr) ptr = "cant_get_function";
	dll_symbols[j].symbol = xstrdup(ptr);
	dll_symbols[j].ordinal = exportDir->Base + *pOrdl;
	assert(dll_symbols[j].symbol);
    }

    for (i = 0; i < exportDir->NumberOfFunctions; i++)
    {
	if (pFunc[i] && !(map[i / 32] & (1 << (i % 32))))
	{
	    char ordinal_text[256];
	    /* Ordinal only entry */
            sprintf (ordinal_text, "%s_%u",
		      globals.forward_dll ? globals.forward_dll : OUTPUT_UC_DLL_NAME,
                      (UINT)exportDir->Base + i);
	    str_toupper(ordinal_text);
	    dll_symbols[j].symbol = xstrdup(ordinal_text);
	    assert(dll_symbols[j].symbol);
	    dll_symbols[j].ordinal = exportDir->Base + i;
	    j++;
	    assert(j <= exportDir->NumberOfFunctions);
	}
    }
    free(map);

    if (NORMAL)
	printf("%u named symbols in DLL, %u total, %d unique (ordinal base = %d)\n",
	       (UINT)exportDir->NumberOfNames, (UINT)exportDir->NumberOfFunctions,
               j, (UINT)exportDir->Base);

    qsort( dll_symbols, j, sizeof(dll_symbol), symbol_cmp );

    dll_symbols[j].symbol = NULL;

    dll_current_symbol = dll_symbols;
}

/*******************************************************************
 *         dll_open
 *
 * Open a DLL and read in exported symbols
 */
BOOL dll_open (const char *dll_name)
{
    return dump_analysis(dll_name, do_grab_sym, SIG_PE);
}

/*******************************************************************
 *         dll_next_symbol
 *
 * Get next exported symbol from dll
 */
BOOL dll_next_symbol (parsed_symbol * sym)
{
    if (!dll_current_symbol || !dll_current_symbol->symbol)
       return FALSE;
     assert (dll_symbols);
    sym->symbol = xstrdup (dll_current_symbol->symbol);
    sym->ordinal = dll_current_symbol->ordinal;
    dll_current_symbol++;
    return TRUE;
}
