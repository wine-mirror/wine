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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <time.h>
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
# include <sys/stat.h>
#endif
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#include <fcntl.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winedump.h"
#include "pe.h"

#ifndef O_BINARY
# define O_BINARY 0
#endif

void*			PE_base;
unsigned long		PE_total_len;
IMAGE_NT_HEADERS*	PE_nt_headers;

enum FileSig {SIG_UNKNOWN, SIG_DOS, SIG_PE, SIG_DBG, SIG_NE};

static inline unsigned int strlenW( const WCHAR *str )
{
    const WCHAR *s = str;
    while (*s) s++;
    return s - str;
}

char*	get_time_str(DWORD _t)
{
    time_t 	t = (time_t)_t;
    static char	buf[128];

    /* FIXME: I don't get the same values from MS' pedump running under Wine...
     * I wonder if Wine isn't broken wrt to GMT settings...
     */
    strncpy(buf, ctime(&t), sizeof(buf));
    buf[sizeof(buf) - 1] = '\0';
    if (buf[strlen(buf)-1] == '\n')
	buf[strlen(buf)-1] = '\0';
    return buf;
}

static	const char* get_machine_str(DWORD mach)
{
    switch (mach)
    {
    case IMAGE_FILE_MACHINE_UNKNOWN:	return "Unknown";
    case IMAGE_FILE_MACHINE_I860:	return "i860";
    case IMAGE_FILE_MACHINE_I386:	return "i386";
    case IMAGE_FILE_MACHINE_R3000:	return "R3000";
    case IMAGE_FILE_MACHINE_R4000:	return "R4000";
    case IMAGE_FILE_MACHINE_R10000:	return "R10000";
    case IMAGE_FILE_MACHINE_ALPHA:	return "Alpha";
    case IMAGE_FILE_MACHINE_POWERPC:	return "PowerPC";
    }
    return "???";
}

void*	PRD(unsigned long prd, unsigned long len)
{
    return (prd + len > PE_total_len) ? NULL : (char*)PE_base + prd;
}

unsigned long Offset(void* ptr)
{
    if (ptr < PE_base) {printf("<<<<<ptr below\n");return 0;}
    if ((char *)ptr >= (char*)PE_base + PE_total_len) {printf("<<<<<ptr above\n");return 0;}
    return (char*)ptr - (char*)PE_base;
}

void*	RVA(unsigned long rva, unsigned long len)
{
    IMAGE_SECTION_HEADER*	sectHead;
    int				i;

    sectHead = (IMAGE_SECTION_HEADER*)((char*)PE_nt_headers + sizeof(DWORD) +
				       sizeof(IMAGE_FILE_HEADER) +
				       PE_nt_headers->FileHeader.SizeOfOptionalHeader);

    if (rva == 0) return NULL;

    for (i = PE_nt_headers->FileHeader.NumberOfSections - 1; i >= 0; i--)
    {
        if (sectHead[i].VirtualAddress <= rva &&
            rva + len <= (DWORD)sectHead[i].VirtualAddress + sectHead[i].SizeOfRawData)
            break;
    }

    if (i < 0)
    {
	printf("rva not found in any section (%lu)\n", rva);
        return NULL;
    }

    /* return image import directory offset */
    return PRD(sectHead[i].PointerToRawData + rva - sectHead[i].VirtualAddress, len);
}

static	void*	get_dir(unsigned idx)
{
    if (idx >= PE_nt_headers->OptionalHeader.NumberOfRvaAndSizes)
	return NULL;
    return RVA(PE_nt_headers->OptionalHeader.DataDirectory[idx].VirtualAddress,
	       PE_nt_headers->OptionalHeader.DataDirectory[idx].Size);
}

static void *get_dir_and_size(unsigned int idx, unsigned int *size)
{
    if (idx >= PE_nt_headers->OptionalHeader.NumberOfRvaAndSizes)
        return NULL;
    *size = PE_nt_headers->OptionalHeader.DataDirectory[idx].Size;
    return RVA(PE_nt_headers->OptionalHeader.DataDirectory[idx].VirtualAddress, *size);
}

static const char*	DirectoryNames[16] = {
    "EXPORT",		"IMPORT",	"RESOURCE", 	"EXCEPTION",
    "SECURITY", 	"BASERELOC", 	"DEBUG", 	"ARCHITECTURE",
    "GLOBALPTR", 	"TLS", 		"LOAD_CONFIG",	"Bound IAT",
    "IAT", 		"Delay IAT",	"COM Descript", ""
};

static	void	dump_pe_header(void)
{
    char			*str;
    IMAGE_FILE_HEADER		*fileHeader;
    IMAGE_OPTIONAL_HEADER	*optionalHeader;
    unsigned			i;

    printf("File Header\n");
    fileHeader = &PE_nt_headers->FileHeader;

    printf("  Machine:                      %04X (%s)\n",
	   fileHeader->Machine, get_machine_str(fileHeader->Machine));
    printf("  Number of Sections:           %d\n", fileHeader->NumberOfSections);
    printf("  TimeDateStamp:                %08lX (%s) offset %ld\n",
	   fileHeader->TimeDateStamp, get_time_str(fileHeader->TimeDateStamp),
	   Offset(&(fileHeader->TimeDateStamp)));
    printf("  PointerToSymbolTable:         %08lX\n", fileHeader->PointerToSymbolTable);
    printf("  NumberOfSymbols:              %08lX\n", fileHeader->NumberOfSymbols);
    printf("  SizeOfOptionalHeader:         %04X\n", fileHeader->SizeOfOptionalHeader);
    printf("  Characteristics:              %04X\n", fileHeader->Characteristics);
#define	X(f,s)	if (fileHeader->Characteristics & f) printf("    %s\n", s)
    X(IMAGE_FILE_RELOCS_STRIPPED, 	"RELOCS_STRIPPED");
    X(IMAGE_FILE_EXECUTABLE_IMAGE, 	"EXECUTABLE_IMAGE");
    X(IMAGE_FILE_LINE_NUMS_STRIPPED, 	"LINE_NUMS_STRIPPED");
    X(IMAGE_FILE_LOCAL_SYMS_STRIPPED, 	"LOCAL_SYMS_STRIPPED");
    X(IMAGE_FILE_16BIT_MACHINE, 	"16BIT_MACHINE");
    X(IMAGE_FILE_BYTES_REVERSED_LO, 	"BYTES_REVERSED_LO");
    X(IMAGE_FILE_32BIT_MACHINE, 	"32BIT_MACHINE");
    X(IMAGE_FILE_DEBUG_STRIPPED, 	"DEBUG_STRIPPED");
    X(IMAGE_FILE_SYSTEM, 		"SYSTEM");
    X(IMAGE_FILE_DLL, 			"DLL");
    X(IMAGE_FILE_BYTES_REVERSED_HI, 	"BYTES_REVERSED_HI");
#undef X
    printf("\n");

    /* hope we have the right size */
    printf("Optional Header\n");
    optionalHeader = &PE_nt_headers->OptionalHeader;
    printf("  Magic                              0x%-4X         %u\n",
	   optionalHeader->Magic, optionalHeader->Magic);
    printf("  linker version                     %u.%02u\n",
	   optionalHeader->MajorLinkerVersion, optionalHeader->MinorLinkerVersion);
    printf("  size of code                       0x%-8lx     %lu\n",
	   optionalHeader->SizeOfCode, optionalHeader->SizeOfCode);
    printf("  size of initialized data           0x%-8lx     %lu\n",
	   optionalHeader->SizeOfInitializedData, optionalHeader->SizeOfInitializedData);
    printf("  size of uninitialized data         0x%-8lx     %lu\n",
	   optionalHeader->SizeOfUninitializedData, optionalHeader->SizeOfUninitializedData);
    printf("  entrypoint RVA                     0x%-8lx     %lu\n",
	   optionalHeader->AddressOfEntryPoint, optionalHeader->AddressOfEntryPoint);
    printf("  base of code                       0x%-8lx     %lu\n",
	   optionalHeader->BaseOfCode, optionalHeader->BaseOfCode);
    printf("  base of data                       0x%-8lX     %lu\n",
	   optionalHeader->BaseOfData, optionalHeader->BaseOfData);
    printf("  image base                         0x%-8lX     %lu\n",
	   optionalHeader->ImageBase, optionalHeader->ImageBase);
    printf("  section align                      0x%-8lx     %lu\n",
	   optionalHeader->SectionAlignment, optionalHeader->SectionAlignment);
    printf("  file align                         0x%-8lx     %lu\n",
	   optionalHeader->FileAlignment, optionalHeader->FileAlignment);
    printf("  required OS version                %u.%02u\n",
	   optionalHeader->MajorOperatingSystemVersion, optionalHeader->MinorOperatingSystemVersion);
    printf("  image version                      %u.%02u\n",
	   optionalHeader->MajorImageVersion, optionalHeader->MinorImageVersion);
    printf("  subsystem version                  %u.%02u\n",
	   optionalHeader->MajorSubsystemVersion, optionalHeader->MinorSubsystemVersion);
    printf("  Win32 Version                      0x%lX\n", optionalHeader->Win32VersionValue);
    printf("  size of image                      0x%-8lx     %lu\n",
	   optionalHeader->SizeOfImage, optionalHeader->SizeOfImage);
    printf("  size of headers                    0x%-8lx     %lu\n",
	   optionalHeader->SizeOfHeaders, optionalHeader->SizeOfHeaders);
    printf("  checksum                           0x%lX\n", optionalHeader->CheckSum);
    switch (optionalHeader->Subsystem)
    {
    default:
    case IMAGE_SUBSYSTEM_UNKNOWN: 	str = "Unknown"; 	break;
    case IMAGE_SUBSYSTEM_NATIVE: 	str = "Native"; 	break;
    case IMAGE_SUBSYSTEM_WINDOWS_GUI: 	str = "Windows GUI"; 	break;
    case IMAGE_SUBSYSTEM_WINDOWS_CUI: 	str = "Windows CUI"; 	break;
    case IMAGE_SUBSYSTEM_OS2_CUI: 	str = "OS/2 CUI"; 	break;
    case IMAGE_SUBSYSTEM_POSIX_CUI: 	str = "Posix CUI"; 	break;
    }
    printf("  Subsystem                          0x%X (%s)\n", optionalHeader->Subsystem, str);
    printf("  DLL flags                          0x%X\n", optionalHeader->DllCharacteristics);
    printf("  stack reserve size                 0x%-8lx     %lu\n",
	   optionalHeader->SizeOfStackReserve, optionalHeader->SizeOfStackReserve);
    printf("  stack commit size                  0x%-8lx     %lu\n",
	   optionalHeader->SizeOfStackCommit, optionalHeader->SizeOfStackCommit);
    printf("  heap reserve size                  0x%-8lx     %lu\n",
	   optionalHeader->SizeOfHeapReserve, optionalHeader->SizeOfHeapReserve);
    printf("  heap commit size                   0x%-8lx     %lu\n",
	   optionalHeader->SizeOfHeapCommit, optionalHeader->SizeOfHeapCommit);
    printf("  loader flags                       0x%lX\n", optionalHeader->LoaderFlags);
    printf("  RVAs & sizes                       0x%lX\n", optionalHeader->NumberOfRvaAndSizes);
    printf("\n");

    printf("Data Directory\n");
    printf("%ld\n", optionalHeader->NumberOfRvaAndSizes * sizeof(IMAGE_DATA_DIRECTORY));

    for (i = 0; i < optionalHeader->NumberOfRvaAndSizes && i < 16; i++)
    {
	printf("  %-12s rva: 0x%-8lX  size: %8lu\n",
	       DirectoryNames[i], optionalHeader->DataDirectory[i].VirtualAddress,
	       optionalHeader->DataDirectory[i].Size);
    }
    printf("\n");
}

static	void	dump_sections(void* addr, unsigned num_sect)
{
    IMAGE_SECTION_HEADER*	sectHead = addr;
    unsigned			i;

    printf("Section Table\n");
    for (i = 0; i < num_sect; i++, sectHead++)
    {
	printf("  %02d %-8s   VirtSize: %-8lu  VirtAddr:  %-8lu 0x%08lx\n",
	       i + 1, sectHead->Name, sectHead->Misc.VirtualSize, sectHead->VirtualAddress,
	       sectHead->VirtualAddress);
	printf("    raw data offs: %-8lu raw data size: %-8lu\n",
	       sectHead->PointerToRawData, sectHead->SizeOfRawData);
	printf("    relocation offs: %-8lu  relocations:   %-8u\n",
	       sectHead->PointerToRelocations, sectHead->NumberOfRelocations);
	printf("    line # offs:     %-8lu  line #'s:      %-8u\n",
	       sectHead->PointerToLinenumbers, sectHead->NumberOfLinenumbers);
	printf("    characteristics: 0x$%08lx\n", sectHead->Characteristics);
	printf("      ");
#define X(b,s)	if (sectHead->Characteristics & b) printf(s "  ")
/* #define IMAGE_SCN_TYPE_REG			0x00000000 - Reserved */
/* #define IMAGE_SCN_TYPE_DSECT			0x00000001 - Reserved */
/* #define IMAGE_SCN_TYPE_NOLOAD		0x00000002 - Reserved */
/* #define IMAGE_SCN_TYPE_GROUP			0x00000004 - Reserved */
/* #define IMAGE_SCN_TYPE_NO_PAD		0x00000008 - Reserved */
/* #define IMAGE_SCN_TYPE_COPY			0x00000010 - Reserved */

	X(IMAGE_SCN_CNT_CODE, 			"CODE");
	X(IMAGE_SCN_CNT_INITIALIZED_DATA, 	"INITIALIZED_DATA");
	X(IMAGE_SCN_CNT_UNINITIALIZED_DATA, 	"UNINITIALIZED_DATA");

	X(IMAGE_SCN_LNK_OTHER, 			"LNK_OTHER");
	X(IMAGE_SCN_LNK_INFO, 			"LNK_INFO");
/* #define	IMAGE_SCN_TYPE_OVER		0x00000400 - Reserved */
	X(IMAGE_SCN_LNK_REMOVE, 		"LNK_REMOVE");
	X(IMAGE_SCN_LNK_COMDAT, 		"LNK_COMDAT");

/* 						0x00002000 - Reserved */
/* #define IMAGE_SCN_MEM_PROTECTED 		0x00004000 - Obsolete */
	X(IMAGE_SCN_MEM_FARDATA, 		"MEM_FARDATA");

/* #define IMAGE_SCN_MEM_SYSHEAP		0x00010000 - Obsolete */
	X(IMAGE_SCN_MEM_PURGEABLE, 		"MEM_PURGEABLE");
	X(IMAGE_SCN_MEM_16BIT, 			"MEM_16BIT");
	X(IMAGE_SCN_MEM_LOCKED, 		"MEM_LOCKED");
	X(IMAGE_SCN_MEM_PRELOAD, 		"MEM_PRELOAD");

	X(IMAGE_SCN_ALIGN_1BYTES, 		"ALIGN_1BYTES");
	X(IMAGE_SCN_ALIGN_2BYTES, 		"ALIGN_2BYTES");
	X(IMAGE_SCN_ALIGN_4BYTES, 		"ALIGN_4BYTES");
	X(IMAGE_SCN_ALIGN_8BYTES, 		"ALIGN_8BYTES");
	X(IMAGE_SCN_ALIGN_16BYTES, 		"ALIGN_16BYTES");
	X(IMAGE_SCN_ALIGN_32BYTES, 		"ALIGN_32BYTES");
	X(IMAGE_SCN_ALIGN_64BYTES, 		"ALIGN_64BYTES");
/* 						0x00800000 - Unused */

	X(IMAGE_SCN_LNK_NRELOC_OVFL, 		"LNK_NRELOC_OVFL");

	X(IMAGE_SCN_MEM_DISCARDABLE, 		"MEM_DISCARDABLE");
	X(IMAGE_SCN_MEM_NOT_CACHED, 		"MEM_NOT_CACHED");
	X(IMAGE_SCN_MEM_NOT_PAGED, 		"MEM_NOT_PAGED");
	X(IMAGE_SCN_MEM_SHARED, 		"MEM_SHARED");
	X(IMAGE_SCN_MEM_EXECUTE, 		"MEM_EXECUTE");
	X(IMAGE_SCN_MEM_READ, 			"MEM_READ");
	X(IMAGE_SCN_MEM_WRITE, 			"MEM_WRITE");
#undef X
	printf("\n\n");
    }
    printf("\n");
}

static	void	dump_dir_exported_functions(void)
{
    unsigned int size;
    IMAGE_EXPORT_DIRECTORY	*exportDir = get_dir_and_size(IMAGE_FILE_EXPORT_DIRECTORY, &size);
    unsigned int		i;
    DWORD*			pFunc;
    DWORD*			pName;
    WORD*			pOrdl;
    DWORD*			map;
    parsed_symbol		symbol;

    if (!exportDir) return;

    printf("Exports table:\n");
    printf("\n");
    printf("  Name:            %s\n", (char*)RVA(exportDir->Name, sizeof(DWORD)));
    printf("  Characteristics: %08lx\n", exportDir->Characteristics);
    printf("  TimeDateStamp:   %08lX %s\n",
	   exportDir->TimeDateStamp, get_time_str(exportDir->TimeDateStamp));
    printf("  Version:         %u.%02u\n", exportDir->MajorVersion, exportDir->MinorVersion);
    printf("  Ordinal base:    %lu\n", exportDir->Base);
    printf("  # of functions:  %lu\n", exportDir->NumberOfFunctions);
    printf("  # of Names:      %lu\n", exportDir->NumberOfNames);
    printf("Adresses of functions: %08lX\n", exportDir->AddressOfFunctions);
    printf("Adresses of name ordinals: %08lX\n", exportDir->AddressOfNameOrdinals);
    printf("Adresses of names: %08lX\n", exportDir->AddressOfNames);
    printf("\n");
    printf("  Entry Pt  Ordn  Name\n");

    pFunc = RVA(exportDir->AddressOfFunctions, exportDir->NumberOfFunctions * sizeof(DWORD));
    if (!pFunc) {printf("Can't grab functions' address table\n"); return;}
    pName = RVA(exportDir->AddressOfNames, exportDir->NumberOfNames * sizeof(DWORD));
    if (!pName) {printf("Can't grab functions' name table\n"); return;}
    pOrdl = RVA(exportDir->AddressOfNameOrdinals, exportDir->NumberOfNames * sizeof(WORD));
    if (!pOrdl) {printf("Can't grab functions' ordinal table\n"); return;}

    /* bit map of used funcs */
    map = calloc(((exportDir->NumberOfFunctions + 31) & ~31) / 32, sizeof(DWORD));
    if (!map) fatal("no memory");

    for (i = 0; i < exportDir->NumberOfNames; i++, pName++, pOrdl++)
    {
	char*	name;

	map[*pOrdl / 32] |= 1 << (*pOrdl % 32);

	name = (char*)RVA(*pName, sizeof(DWORD));
	if (name && globals.do_demangle)
	{
	    printf("  %08lX  %4lu ", pFunc[*pOrdl], exportDir->Base + *pOrdl);

	    symbol_init(&symbol, name);
	    if (symbol_demangle(&symbol) == -1)
		printf(name);
	    else if (symbol.flags & SYM_DATA)
		printf(symbol.arg_text[0]);
	    else
		output_prototype(stdout, &symbol);
	    symbol_clear(&symbol);
	}
	else
	{
	    printf("  %08lX  %4lu %s", pFunc[*pOrdl], exportDir->Base + *pOrdl, name);
	}
        /* check for forwarded function */
        if ((char *)RVA(pFunc[*pOrdl],sizeof(void*)) >= (char *)exportDir &&
            (char *)RVA(pFunc[*pOrdl],sizeof(void*)) < (char *)exportDir + size)
            printf( " (-> %s)", (char *)RVA(pFunc[*pOrdl],1));
        printf("\n");
    }
    pFunc = RVA(exportDir->AddressOfFunctions, exportDir->NumberOfFunctions * sizeof(DWORD));
    if (!pFunc) {printf("Can't grab functions' address table\n"); return;}
    for (i = 0; i < exportDir->NumberOfFunctions; i++)
    {
	if (pFunc[i] && !(map[i / 32] & (1 << (i % 32))))
	{
	    printf("  %08lX  %4lu <by ordinal>\n", pFunc[i], exportDir->Base + i);
	}
    }
    free(map);
    printf("\n");
}

static	void	dump_dir_imported_functions(void)
{
    IMAGE_IMPORT_DESCRIPTOR	*importDesc = get_dir(IMAGE_FILE_IMPORT_DIRECTORY);
    unsigned			nb_imp, i;

    if (!importDesc)	return;
    nb_imp = PE_nt_headers->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY].Size /
	sizeof(*importDesc);
    if (!nb_imp) return;

    printf("Import Table size: %lu\n",
	   PE_nt_headers->OptionalHeader.DataDirectory[IMAGE_FILE_IMPORT_DIRECTORY].Size);/* FIXME */

    for (i = 0; i < nb_imp - 1; i++) /* the last descr is set as 0 as a sentinel */
    {
	IMAGE_THUNK_DATA*	il;
	IMAGE_IMPORT_BY_NAME*	iibn;

	if (!importDesc->Name ||
	    (importDesc->u.OriginalFirstThunk == NULL && importDesc->FirstThunk == NULL))
	{
	    /* FIXME */
	    printf("<<<<<<<null entry\n");
	    break;
	}
	printf("  offset %lu %s\n", Offset(importDesc), (char*)RVA(importDesc->Name, sizeof(DWORD)));
	printf("  Hint/Name Table: %08lX\n", (DWORD)importDesc->u.OriginalFirstThunk);
	printf("  TimeDataStamp:   %08lX (%s)\n",
	       importDesc->TimeDateStamp, get_time_str(importDesc->TimeDateStamp));
	printf("  ForwarderChain:  %08lX\n", importDesc->ForwarderChain);
	printf("  First thunk RVA: %08lX (delta: %u 0x%x)\n",
	       (DWORD)importDesc->FirstThunk, -1, -1); /* FIXME */

	printf("  Ordn  Name\n");

	il = (importDesc->u.OriginalFirstThunk != 0) ?
	    RVA((DWORD)importDesc->u.OriginalFirstThunk, sizeof(DWORD)) :
	    RVA((DWORD)importDesc->FirstThunk, sizeof(DWORD));

	if (!il) {printf("Can't grab thunk data, going to next imported DLL\n"); continue;}

	for (; il->u1.Ordinal; il++)
	{
	    if (IMAGE_SNAP_BY_ORDINAL(il->u1.Ordinal))
	    {
		printf("  %4lu  <by ordinal>\n", IMAGE_ORDINAL(il->u1.Ordinal));
	    }
	    else
	    {
		iibn = RVA((DWORD)il->u1.AddressOfData, sizeof(DWORD));
		if (!il)
		{
		    printf("Can't grab import by name info, skipping to next ordinal\n");
		}
		else
		{
		    printf("  %4u  %s %lx\n", iibn->Hint, iibn->Name, (DWORD)il->u1.AddressOfData);
		}
	    }
	}
	printf("\n");
	importDesc++;
    }
    printf("\n");
}

static	void	dump_dir_debug_dir(IMAGE_DEBUG_DIRECTORY* idd, int idx)
{
    const	char*	str;

    printf("Directory %02u\n", idx + 1);
    printf("  Characteristics:   %08lX\n", idd->Characteristics);
    printf("  TimeDateStamp:     %08lX %s\n",
	   idd->TimeDateStamp, get_time_str(idd->TimeDateStamp));
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
    }
    printf("  Type:              %lu (%s)\n", idd->Type, str);
    printf("  SizeOfData:        %lu\n", idd->SizeOfData);
    printf("  AddressOfRawData:  %08lX\n", idd->AddressOfRawData);
    printf("  PointerToRawData:  %08lX\n", idd->PointerToRawData);

    switch (idd->Type)
    {
    case IMAGE_DEBUG_TYPE_UNKNOWN:
	break;
    case IMAGE_DEBUG_TYPE_COFF:
	dump_coff(idd->PointerToRawData, idd->SizeOfData);
	break;
    case IMAGE_DEBUG_TYPE_CODEVIEW:
	dump_codeview(idd->PointerToRawData, idd->SizeOfData);
	break;
    case IMAGE_DEBUG_TYPE_FPO:
	dump_frame_pointer_omission(idd->PointerToRawData, idd->SizeOfData);
	break;
    case IMAGE_DEBUG_TYPE_MISC:
    {
	IMAGE_DEBUG_MISC* misc = PRD(idd->PointerToRawData, idd->SizeOfData);
	if (!misc) {printf("Can't get misc debug information\n"); break;}
	printf("    DataType:          %lu (%s)\n",
	       misc->DataType,
	       (misc->DataType == IMAGE_DEBUG_MISC_EXENAME) ? "Exe name" : "Unknown");
	printf("    Length:            %lu\n", misc->Length);
	printf("    Unicode:           %s\n", misc->Unicode ? "Yes" : "No");
	printf("    Data:              %s\n", misc->Data);
    }
    break;
    case IMAGE_DEBUG_TYPE_EXCEPTION:
	break;
    case IMAGE_DEBUG_TYPE_FIXUP:
	break;
    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
	break;
    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
	break;
    case IMAGE_DEBUG_TYPE_BORLAND:
	break;
    case IMAGE_DEBUG_TYPE_RESERVED10:
	break;
    }
    printf("\n");
}

static void	dump_dir_debug(void)
{
    IMAGE_DEBUG_DIRECTORY*	debugDir = get_dir(IMAGE_FILE_DEBUG_DIRECTORY);
    unsigned			nb_dbg, i;

    if (!debugDir) return;
    nb_dbg = PE_nt_headers->OptionalHeader.DataDirectory[IMAGE_FILE_DEBUG_DIRECTORY].Size /
	sizeof(*debugDir);
    if (!nb_dbg) return;

    printf("Debug Table (%u directories)\n", nb_dbg);

    for (i = 0; i < nb_dbg; i++)
    {
	dump_dir_debug_dir(debugDir, i);
	debugDir++;
    }
    printf("\n");
}

static void dump_dir_tls(void)
{
    const IMAGE_TLS_DIRECTORY *dir = get_dir(IMAGE_FILE_THREAD_LOCAL_STORAGE);
    const DWORD *callbacks;

    if (!dir) return;
    printf( "Thread Local Storage\n" );
    printf( "  Raw data        %08lx-%08lx (data size %lx zero fill size %lx)\n",
            dir->StartAddressOfRawData, dir->EndAddressOfRawData,
            dir->EndAddressOfRawData - dir->StartAddressOfRawData,
            dir->SizeOfZeroFill );
    printf( "  Index address   %08lx\n", (DWORD)dir->AddressOfIndex );
    printf( "  Characteristics %08lx\n", dir->Characteristics );
    printf( "  Callbacks       %08lx -> {", (DWORD)dir->AddressOfCallBacks );
    if (dir->AddressOfCallBacks)
    {
        callbacks = RVA((DWORD)dir->AddressOfCallBacks - PE_nt_headers->OptionalHeader.ImageBase,0);
        while (*callbacks) printf( " %08lx", *callbacks++ );
    }
    printf(" }\n\n");
}

static void	dump_separate_dbg(void)
{
    IMAGE_SEPARATE_DEBUG_HEADER*separateDebugHead = PRD(0, sizeof(separateDebugHead));
    unsigned			nb_dbg;
    unsigned			i;
    IMAGE_DEBUG_DIRECTORY*	debugDir;

    if (!separateDebugHead) {printf("Can't grab the separate header, aborting\n"); return;}

    printf ("Signature:          %.2s (0x%4X)\n",
	    (char*)&separateDebugHead->Signature, separateDebugHead->Signature);
    printf ("Flags:              0x%04X\n", separateDebugHead->Flags);
    printf ("Machine:            0x%04X (%s)\n",
	    separateDebugHead->Machine, get_machine_str(separateDebugHead->Machine));
    printf ("Characteristics:    0x%04X\n", separateDebugHead->Characteristics);
    printf ("TimeDateStamp:      0x%08lX (%s)\n",
	    separateDebugHead->TimeDateStamp, get_time_str(separateDebugHead->TimeDateStamp));
    printf ("CheckSum:           0x%08lX\n", separateDebugHead->CheckSum);
    printf ("ImageBase:          0x%08lX\n", separateDebugHead->ImageBase);
    printf ("SizeOfImage:        0x%08lX\n", separateDebugHead->SizeOfImage);
    printf ("NumberOfSections:   0x%08lX\n", separateDebugHead->NumberOfSections);
    printf ("ExportedNamesSize:  0x%08lX\n", separateDebugHead->ExportedNamesSize);
    printf ("DebugDirectorySize: 0x%08lX\n", separateDebugHead->DebugDirectorySize);

    if (!PRD(sizeof(IMAGE_SEPARATE_DEBUG_HEADER),
	     separateDebugHead->NumberOfSections * sizeof(IMAGE_SECTION_HEADER)))
    {printf("Can't get the sections, aborting\n"); return;}

    dump_sections(separateDebugHead + 1, separateDebugHead->NumberOfSections);

    nb_dbg = separateDebugHead->DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY);
    debugDir = PRD(sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
		   separateDebugHead->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
		   separateDebugHead->ExportedNamesSize,
		   nb_dbg * sizeof(IMAGE_DEBUG_DIRECTORY));
    if (!debugDir) {printf("Couldn't get the debug directory info, aborting\n");return;}

    printf("Debug Table (%u directories)\n", nb_dbg);

    for (i = 0; i < nb_dbg; i++)
    {
	dump_dir_debug_dir(debugDir, i);
	debugDir++;
    }
}

static void dump_unicode_str( const WCHAR *str, int len )
{
    if (len == -1) for (len = 0; str[len]; len++) ;
    printf( "L\"");
    while (len-- > 0 && *str)
    {
        WCHAR c = *str++;
        switch (c)
        {
        case '\n': printf( "\\n" ); break;
        case '\r': printf( "\\r" ); break;
        case '\t': printf( "\\t" ); break;
        case '"':  printf( "\\\"" ); break;
        case '\\': printf( "\\\\" ); break;
        default:
            if (c >= ' ' && c <= 126) putchar(c);
            else printf( "\\u%04x",c);
        }
    }
    printf( "\"" );
}

static const char *get_resource_type( unsigned int id )
{
    static const char *types[] =
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
        "HTML"
    };

    if ((size_t)id < sizeof(types)/sizeof(types[0])) return types[id];
    return NULL;
}

void dump_data( const unsigned char *ptr, unsigned int size, const char *prefix )
{
    unsigned int i, j;

    printf( "%s", prefix );
    for (i = 0; i < size; i++)
    {
        printf( "%02x%c", ptr[i], (i % 16 == 7) ? '-' : ' ' );
        if ((i % 16) == 15)
        {
            printf( " " );
            for (j = 0; j < 16; j++)
                printf( "%c", isprint(ptr[i-15+j]) ? ptr[i-15+j] : '.' );
            if (i < size-1) printf( "\n%s", prefix );
        }
    }
    if (i % 16)
    {
        printf( "%*s ", 3 * (16-(i%16)), "" );
        for (j = 0; j < i % 16; j++)
            printf( "%c", isprint(ptr[i-(i%16)+j]) ? ptr[i-(i%16)+j] : '.' );
    }
    printf( "\n" );
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
static void dump_msgtable_data( const void *ptr, unsigned int size, unsigned int id, const char *prefix )
{
    const MESSAGE_RESOURCE_DATA *data = ptr;
    const MESSAGE_RESOURCE_BLOCK *block = data->Blocks;
    unsigned i, j;

    for (i = 0; i < data->NumberOfBlocks; i++, block++)
    {
        const MESSAGE_RESOURCE_ENTRY *entry;

        entry = (MESSAGE_RESOURCE_ENTRY *)((char *)data + block->OffsetToEntries);
        for (j = block->LowId; j <= block->HighId; j++)
        {
            if (entry->Flags & MESSAGE_RESOURCE_UNICODE)
            {
                const WCHAR *str = (WCHAR *)entry->Text;
                printf( "%s%08x L\"", prefix, j );
                dump_strW( str, strlenW(str) );
                printf( "\"\n" );
            }
            else
            {
                printf( "%s%08x \"", prefix, j );
                dump_strA( entry->Text, strlen(entry->Text) );
                printf( "\"\n" );
            }
            entry = (MESSAGE_RESOURCE_ENTRY *)((char *)entry + entry->Length);
        }
    }
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
        e1 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(root + 1) + i;
        namedir = (IMAGE_RESOURCE_DIRECTORY *)((char *)root + e1->u2.s3.OffsetToDirectory);
        for (j = 0; j < namedir->NumberOfNamedEntries + namedir->NumberOfIdEntries; j++)
        {
            e2 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(namedir + 1) + j;
            langdir = (IMAGE_RESOURCE_DIRECTORY *)((char *)root + e2->u2.s3.OffsetToDirectory);
            for (k = 0; k < langdir->NumberOfNamedEntries + langdir->NumberOfIdEntries; k++)
            {
                e3 = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(langdir + 1) + k;

                printf( "\n  " );
                if (e1->u1.s1.NameIsString)
                {
                    string = (PIMAGE_RESOURCE_DIR_STRING_U)((char *)root + e1->u1.s1.NameOffset);
                    dump_unicode_str( string->NameString, string->Length );
                }
                else
                {
                    const char *type = get_resource_type( e1->u1.s2.Id );
                    if (type) printf( "%s", type );
                    else printf( "%04x", e1->u1.s2.Id );
                }

                printf( " Name=" );
                if (e2->u1.s1.NameIsString)
                {
                    string = (PIMAGE_RESOURCE_DIR_STRING_U) ((char *)root + e2->u1.s1.NameOffset);
                    dump_unicode_str( string->NameString, string->Length );
                }
                else
                    printf( "%04x", e2->u1.s2.Id );

                printf( " Language=%04x:\n", e3->u1.s2.Id );
                data = (IMAGE_RESOURCE_DATA_ENTRY *)((char *)root + e3->u2.OffsetToData);
                if (e1->u1.s1.NameIsString)
                {
                    dump_data( RVA( data->OffsetToData, data->Size ), data->Size, "        " );
                }
                else switch(e1->u1.s2.Id)
                {
                case 6:
                    dump_string_data( RVA( data->OffsetToData, data->Size ), data->Size,
                                      e2->u1.s2.Id, "    " );
                    break;
                case 11:
                    dump_msgtable_data( RVA( data->OffsetToData, data->Size ), data->Size,
                                        e2->u1.s2.Id, "    " );
                    break;
                default:
                    dump_data( RVA( data->OffsetToData, data->Size ), data->Size, "        " );
                    break;
                }
            }
        }
    }
    printf( "\n\n" );
}

static	void	do_dump( enum FileSig sig )
{
    int	all = (globals.dumpsect != NULL) && strcmp(globals.dumpsect, "ALL") == 0;

    if (sig == SIG_NE)
    {
        ne_dump( PE_base, PE_total_len );
        return;
    }

    if (globals.do_dumpheader)
    {
	dump_pe_header();
	/* FIXME: should check ptr */
	dump_sections((char*)PE_nt_headers + sizeof(DWORD) +
		      sizeof(IMAGE_FILE_HEADER) + PE_nt_headers->FileHeader.SizeOfOptionalHeader,
		      PE_nt_headers->FileHeader.NumberOfSections);
    }
    else if (!globals.dumpsect)
    {
	/* show at least something here */
	dump_pe_header();
    }

    if (globals.dumpsect)
    {
	if (all || !strcmp(globals.dumpsect, "import"))
	    dump_dir_imported_functions();
	if (all || !strcmp(globals.dumpsect, "export"))
	    dump_dir_exported_functions();
	if (all || !strcmp(globals.dumpsect, "debug"))
	    dump_dir_debug();
	if (all || !strcmp(globals.dumpsect, "resource"))
	    dump_dir_resource();
	if (all || !strcmp(globals.dumpsect, "tls"))
	    dump_dir_tls();
#if 0
	/* FIXME: not implemented yet */
	if (all || !strcmp(globals.dumpsect, "reloc"))
	    dump_dir_reloc();
#endif
    }
}

static	enum FileSig	check_headers(void)
{
    WORD*		pw;
    DWORD*		pdw;
    IMAGE_DOS_HEADER*	dh;
    enum FileSig	sig;

    pw = PRD(0, sizeof(WORD));
    if (!pw) {printf("Can't get main signature, aborting\n"); return 0;}

    switch (*pw)
    {
    case IMAGE_DOS_SIGNATURE:
	sig = SIG_DOS;
	dh = PRD(0, sizeof(IMAGE_DOS_HEADER));
	if (dh && dh->e_lfanew >= sizeof(*dh)) /* reasonable DOS header ? */
	{
	    /* the signature is the first DWORD */
	    pdw = PRD(dh->e_lfanew, sizeof(DWORD));
	    if (pdw)
	    {
		if (*pdw == IMAGE_NT_SIGNATURE)
		{
		    PE_nt_headers = PRD(dh->e_lfanew, sizeof(DWORD)+sizeof(IMAGE_FILE_HEADER));
		    sig = SIG_PE;
		}
                else if (*(WORD *)pdw == IMAGE_OS2_SIGNATURE)
                {
                    sig = SIG_NE;
                }
		else
		{
		    printf("No PE Signature found\n");
		}
	    }
	    else
	    {
		printf("Can't get the extented signature, aborting\n");
	    }
	}
	break;
    case 0x4944: /* "DI" */
	sig = SIG_DBG;
	break;
    default:
	printf("No known main signature (%.2s/%x), aborting\n", (char*)pw, *pw);
	sig = SIG_UNKNOWN;
    }

    return sig;
}

int pe_analysis(const char* name, void (*fn)(enum FileSig), enum FileSig wanted_sig)
{
    int			fd;
    enum FileSig	effective_sig;
    int			ret = 1;
    struct stat		s;

    setbuf(stdout, NULL);

    fd = open(name, O_RDONLY | O_BINARY);
    if (fd == -1) fatal("Can't open file");

    if (fstat(fd, &s) < 0) fatal("Can't get size");
    PE_total_len = s.st_size;

#ifdef HAVE_MMAP
    if ((PE_base = mmap(NULL, PE_total_len, PROT_READ, MAP_PRIVATE, fd, 0)) == (void *)-1)
#endif
    {
        if (!(PE_base = malloc( PE_total_len ))) fatal( "Out of memory" );
        if ((unsigned long)read( fd, PE_base, PE_total_len ) != PE_total_len) fatal( "Cannot read file" );
    }

    effective_sig = check_headers();

    if (effective_sig == SIG_UNKNOWN)
    {
	printf("Can't get a recognized file signature, aborting\n");
	ret = 0;
    }
    else if (wanted_sig == SIG_UNKNOWN || wanted_sig == effective_sig)
    {
	switch (effective_sig)
	{
	case SIG_UNKNOWN: /* shouldn't happen... */
	    ret = 0; break;
	case SIG_PE:
	case SIG_NE:
	    printf("Contents of \"%s\": %ld bytes\n\n", name, PE_total_len);
	    (*fn)(effective_sig);
	    break;
	case SIG_DBG:
	    dump_separate_dbg();
	    break;
	case SIG_DOS:
	    ret = 0; break;
	}
    }
    else
    {
	printf("Can't get a suitable file signature, aborting\n");
	ret = 0;
    }

    if (ret) printf("Done dumping %s\n", name);
#ifdef HAVE_MMAP
    if (munmap(PE_base, PE_total_len) == -1)
#endif
    {
        free( PE_base );
    }
    close(fd);

    return ret;
}

void	dump_file(const char* name)
{
    pe_analysis(name, do_dump, SIG_UNKNOWN);
}

#if 0
int 	main(int argc, char* argv[])
{
    if (argc != 2) fatal("usage");
    pe_analysis(argv[1], do_dump);
}
#endif

typedef struct _dll_symbol {
    size_t	ordinal;
    char       *symbol;
} dll_symbol;

static dll_symbol *dll_symbols = NULL;
static dll_symbol *dll_current_symbol = NULL;

/* Compare symbols by ordinal for qsort */
static int symbol_cmp(const void *left, const void *right)
{
    return ((dll_symbol *)left)->ordinal > ((dll_symbol *)right)->ordinal;
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

static	void	do_grab_sym( enum FileSig sig )
{
    IMAGE_EXPORT_DIRECTORY	*exportDir = get_dir(IMAGE_FILE_EXPORT_DIRECTORY);
    unsigned			i, j;
    DWORD*			pName;
    DWORD*			pFunc;
    WORD*			pOrdl;
    char*			ptr;
    DWORD*			map;

    if (!exportDir) return;

    pName = RVA(exportDir->AddressOfNames, exportDir->NumberOfNames * sizeof(DWORD));
    if (!pName) {printf("Can't grab functions' name table\n"); return;}
    pOrdl = RVA(exportDir->AddressOfNameOrdinals, exportDir->NumberOfNames * sizeof(WORD));
    if (!pOrdl) {printf("Can't grab functions' ordinal table\n"); return;}

    /* dll_close(); */

    if (!(dll_symbols = (dll_symbol *) malloc((exportDir->NumberOfFunctions + 1) *
					      sizeof (dll_symbol))))
	fatal ("Out of memory");
    if (exportDir->AddressOfFunctions != exportDir->NumberOfNames || exportDir->Base > 1)
	globals.do_ordinals = 1;

    /* bit map of used funcs */
    map = calloc(((exportDir->NumberOfFunctions + 31) & ~31) / 32, sizeof(DWORD));
    if (!map) fatal("no memory");

    for (j = 0; j < exportDir->NumberOfNames; j++, pOrdl++)
    {
	map[*pOrdl / 32] |= 1 << (*pOrdl % 32);
	ptr = RVA(*pName++, sizeof(DWORD));
	if (!ptr) ptr = "cant_get_function";
	dll_symbols[j].symbol = strdup(ptr);
	dll_symbols[j].ordinal = exportDir->Base + *pOrdl;
	assert(dll_symbols[j].symbol);
    }
    pFunc = RVA(exportDir->AddressOfFunctions, exportDir->NumberOfFunctions * sizeof(DWORD));
    if (!pFunc) {printf("Can't grab functions' address table\n"); return;}

    for (i = 0; i < exportDir->NumberOfFunctions; i++)
    {
	if (pFunc[i] && !(map[i / 32] & (1 << (i % 32))))
	{
	    char ordinal_text[256];
	    /* Ordinal only entry */
	    snprintf (ordinal_text, sizeof(ordinal_text), "%s_%lu",
		      globals.forward_dll ? globals.forward_dll : OUTPUT_UC_DLL_NAME,
		      exportDir->Base + i);
	    str_toupper(ordinal_text);
	    dll_symbols[j].symbol = strdup(ordinal_text);
	    assert(dll_symbols[j].symbol);
	    dll_symbols[j].ordinal = exportDir->Base + i;
	    j++;
	    assert(j <= exportDir->NumberOfFunctions);
	}
    }
    free(map);

    if (NORMAL)
	printf("%lu named symbols in DLL, %lu total, %d unique (ordinal base = %ld)\n",
	       exportDir->NumberOfNames, exportDir->NumberOfFunctions, j, exportDir->Base);

    qsort( dll_symbols, j, sizeof(dll_symbol), symbol_cmp );

    dll_symbols[j].symbol = NULL;

    dll_current_symbol = dll_symbols;
}

/*******************************************************************
 *         dll_open
 *
 * Open a DLL and read in exported symbols
 */
void  dll_open (const char *dll_name)
{
    pe_analysis(dll_name, do_grab_sym, SIG_PE);
}

/*******************************************************************
 *         dll_next_symbol
 *
 * Get next exported symbol from dll
 */
int dll_next_symbol (parsed_symbol * sym)
{
    if (!dll_current_symbol->symbol)
	return 1;

    assert (dll_symbols);

    sym->symbol = strdup (dll_current_symbol->symbol);
    sym->ordinal = dll_current_symbol->ordinal;
    dll_current_symbol++;
    return 0;
}
