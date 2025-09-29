/*
 * File pe_module.c - handle PE module information
 *
 * Copyright (C) 1996,      Eric Youngdale.
 * Copyright (C) 1999-2000, Ulrich Weigand.
 * Copyright (C) 2004-2007, Eric Pouech.
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
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "dbghelp_private.h"
#include "image_private.h"
#include "winternl.h"
#include "wine/debug.h"
#include "wine/heap.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

struct pe_module_info
{
    struct image_file_map       fmap;
};

static const char builtin_signature[] = "Wine builtin DLL";

static void* pe_map_full(struct image_file_map* fmap, IMAGE_NT_HEADERS** nth)
{
    if (!fmap->u.pe.full_map)
    {
        fmap->u.pe.full_map = MapViewOfFile(fmap->u.pe.hMap, FILE_MAP_READ, 0, 0, 0);
    }
    if (fmap->u.pe.full_map)
    {
        if (nth) *nth = RtlImageNtHeader(fmap->u.pe.full_map);
        fmap->u.pe.full_count++;
        return fmap->u.pe.full_map;
    }
    return NULL;
}

static void pe_unmap_full(struct image_file_map* fmap)
{
    if (fmap->u.pe.full_count && !--fmap->u.pe.full_count)
    {
        UnmapViewOfFile(fmap->u.pe.full_map);
        fmap->u.pe.full_map = NULL;
    }
}

/* as we store either IMAGE_OPTIONAL_HEADER(32|64) inside pe_file_map,
 * this helper will read to any field 'field' inside such an header
 */
#define PE_FROM_OPTHDR(fmap, field) (((fmap)->addr_size == 32) ? ((fmap)->u.pe.opt.header32. field) : ((fmap)->u.pe.opt.header64. field))

/******************************************************************
 *		pe_map_section
 *
 * Maps a single section into memory from an PE file
 */
static const char* pe_map_section(struct image_section_map* ism)
{
    void*       mapping;
    struct pe_file_map* fmap = &ism->fmap->u.pe;

    if (ism->sidx >= 0 && ism->sidx < fmap->file_header.NumberOfSections &&
        fmap->sect[ism->sidx].mapped == IMAGE_NO_MAP)
    {
        IMAGE_NT_HEADERS*       nth;

        if (fmap->sect[ism->sidx].shdr.Misc.VirtualSize > fmap->sect[ism->sidx].shdr.SizeOfRawData)
        {
            FIXME("Section %Id: virtual (0x%lx) > raw (0x%lx) size - not supported\n",
                  ism->sidx, fmap->sect[ism->sidx].shdr.Misc.VirtualSize,
                  fmap->sect[ism->sidx].shdr.SizeOfRawData);
            return IMAGE_NO_MAP;
        }
        /* FIXME: that's rather drastic, but that will do for now
         * that's ok if the full file map exists, but we could be less aggressive otherwise and
         * only map the relevant section
         */
        if ((mapping = pe_map_full(ism->fmap, &nth)))
        {
            fmap->sect[ism->sidx].mapped = RtlImageRvaToVa(nth, mapping,
                                                           fmap->sect[ism->sidx].shdr.VirtualAddress,
                                                           NULL);
            return fmap->sect[ism->sidx].mapped;
        }
    }
    return IMAGE_NO_MAP;
}

/******************************************************************
 *		pe_find_section
 *
 * Finds a section by name (and type) into memory from an PE file
 * or its alternate if any
 */
static BOOL pe_find_section(struct image_file_map* fmap, const char* name,
                            struct image_section_map* ism)
{
    const char*                 sectname;
    unsigned                    i;
    char                        tmp[IMAGE_SIZEOF_SHORT_NAME + 1];

    for (i = 0; i < fmap->u.pe.file_header.NumberOfSections; i++)
    {
        sectname = (const char*)fmap->u.pe.sect[i].shdr.Name;
        /* long section names start with a '/' (at least on MinGW32) */
        if (sectname[0] == '/' && fmap->u.pe.strtable)
            sectname = fmap->u.pe.strtable + atoi(sectname + 1);
        else
        {
            /* the section name may not be null terminated */
            sectname = memcpy(tmp, sectname, IMAGE_SIZEOF_SHORT_NAME);
            tmp[IMAGE_SIZEOF_SHORT_NAME] = '\0';
        }
        if (!stricmp(sectname, name))
        {
            ism->fmap = fmap;
            ism->sidx = i;
            return TRUE;
        }
    }
    ism->fmap = NULL;
    ism->sidx = -1;

    return FALSE;
}

/******************************************************************
 *		pe_unmap_section
 *
 * Unmaps a single section from memory
 */
static void pe_unmap_section(struct image_section_map* ism)
{
    if (ism->sidx >= 0 && ism->sidx < ism->fmap->u.pe.file_header.NumberOfSections &&
        ism->fmap->u.pe.sect[ism->sidx].mapped != IMAGE_NO_MAP)
    {
        pe_unmap_full(ism->fmap);
        ism->fmap->u.pe.sect[ism->sidx].mapped = IMAGE_NO_MAP;
    }
}

/******************************************************************
 *		pe_get_map_rva
 *
 * Get the RVA of an PE section
 */
static DWORD_PTR pe_get_map_rva(const struct image_section_map* ism)
{
    if (ism->sidx < 0 || ism->sidx >= ism->fmap->u.pe.file_header.NumberOfSections)
        return 0;
    return ism->fmap->u.pe.sect[ism->sidx].shdr.VirtualAddress;
}

/******************************************************************
 *		pe_get_map_size
 *
 * Get the size of a PE section
 */
static unsigned pe_get_map_size(const struct image_section_map* ism)
{
    if (ism->sidx < 0 || ism->sidx >= ism->fmap->u.pe.file_header.NumberOfSections)
        return 0;
    return ism->fmap->u.pe.sect[ism->sidx].shdr.Misc.VirtualSize;
}

/******************************************************************
 *		pe_unmap_file
 *
 * Unmaps an PE file from memory (previously mapped with pe_map_file)
 */
static void pe_unmap_file(struct image_file_map* fmap)
{
    if (fmap->u.pe.hMap != 0)
    {
        struct image_section_map  ism;
        ism.fmap = fmap;
        for (ism.sidx = 0; ism.sidx < fmap->u.pe.file_header.NumberOfSections; ism.sidx++)
        {
            pe_unmap_section(&ism);
        }
        while (fmap->u.pe.full_count) pe_unmap_full(fmap);
        HeapFree(GetProcessHeap(), 0, fmap->u.pe.sect);
        HeapFree(GetProcessHeap(), 0, (void*)fmap->u.pe.strtable); /* FIXME ugly (see pe_map_file) */
        CloseHandle(fmap->u.pe.hMap);
        fmap->u.pe.hMap = NULL;
    }
}

static const struct image_file_map_ops pe_file_map_ops =
{
    pe_map_section,
    pe_unmap_section,
    pe_find_section,
    pe_get_map_rva,
    pe_get_map_size,
    pe_unmap_file,
};

/******************************************************************
 *		pe_is_valid_pointer_table
 *
 * Checks whether the PointerToSymbolTable and NumberOfSymbols in file_header contain
 * valid information.
 */
static BOOL pe_is_valid_pointer_table(const IMAGE_NT_HEADERS* nthdr, const void* mapping, DWORD64 sz)
{
    DWORD64     offset;

    /* is the iSym table inside file size ? (including first DWORD of string table, which is its size) */
    offset = (DWORD64)nthdr->FileHeader.PointerToSymbolTable;
    offset += (DWORD64)nthdr->FileHeader.NumberOfSymbols * sizeof(IMAGE_SYMBOL);
    if (offset + sizeof(DWORD) > sz) return FALSE;
    /* is string table (following iSym table) inside file size ? */
    offset += *(DWORD*)((const char*)mapping + offset);
    return offset <= sz;
}

/******************************************************************
 *		pe_map_file
 *
 * Maps an PE file into memory (and checks it's a real PE file)
 */
BOOL pe_map_file(HANDLE file, struct image_file_map* fmap)
{
    void*                       mapping;
    IMAGE_NT_HEADERS*           nthdr;
    IMAGE_SECTION_HEADER*       section;
    unsigned                    i;

    fmap->modtype = DMT_PE;
    fmap->ops = &pe_file_map_ops;
    fmap->alternate = NULL;
    fmap->u.pe.hMap = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    if (fmap->u.pe.hMap == 0) return FALSE;
    fmap->u.pe.full_count = 0;
    fmap->u.pe.full_map = NULL;
    if (!(mapping = pe_map_full(fmap, NULL))) goto error;

    if (!(nthdr = RtlImageNtHeader(mapping))) goto error;
    memcpy(&fmap->u.pe.file_header, &nthdr->FileHeader, sizeof(fmap->u.pe.file_header));
    switch (nthdr->OptionalHeader.Magic)
    {
    case IMAGE_NT_OPTIONAL_HDR32_MAGIC:
        fmap->addr_size = 32;
        memcpy(&fmap->u.pe.opt.header32, &nthdr->OptionalHeader, sizeof(fmap->u.pe.opt.header32));
        break;
    case IMAGE_NT_OPTIONAL_HDR64_MAGIC:
        fmap->addr_size = 64;
        memcpy(&fmap->u.pe.opt.header64, &nthdr->OptionalHeader, sizeof(fmap->u.pe.opt.header64));
        break;
    default:
        goto error;
    }

    fmap->u.pe.builtin = !memcmp((const IMAGE_DOS_HEADER*)mapping + 1, builtin_signature, sizeof(builtin_signature));
    section = IMAGE_FIRST_SECTION( nthdr );
    fmap->u.pe.sect = HeapAlloc(GetProcessHeap(), 0,
                                nthdr->FileHeader.NumberOfSections * sizeof(fmap->u.pe.sect[0]));
    if (!fmap->u.pe.sect) goto error;
    for (i = 0; i < nthdr->FileHeader.NumberOfSections; i++)
    {
        memcpy(&fmap->u.pe.sect[i].shdr, section + i, sizeof(IMAGE_SECTION_HEADER));
        fmap->u.pe.sect[i].mapped = IMAGE_NO_MAP;
    }
    if (nthdr->FileHeader.PointerToSymbolTable && nthdr->FileHeader.NumberOfSymbols)
    {
        LARGE_INTEGER li;

        if (GetFileSizeEx(file, &li) && pe_is_valid_pointer_table(nthdr, mapping, li.QuadPart))
        {
            /* FIXME ugly: should rather map the relevant content instead of copying it */
            const char* src = (const char*)mapping +
                nthdr->FileHeader.PointerToSymbolTable +
                nthdr->FileHeader.NumberOfSymbols * sizeof(IMAGE_SYMBOL);
            char* dst;
            DWORD sz = *(DWORD*)src;

            if ((dst = HeapAlloc(GetProcessHeap(), 0, sz)))
                memcpy(dst, src, sz);
            fmap->u.pe.strtable = dst;
        }
        else
        {
            WARN("Bad coff table... wipping out\n");
            /* we have bad information here, wipe it out */
            fmap->u.pe.file_header.PointerToSymbolTable = 0;
            fmap->u.pe.file_header.NumberOfSymbols = 0;
            fmap->u.pe.strtable = NULL;
        }
    }
    else fmap->u.pe.strtable = NULL;

    pe_unmap_full(fmap);

    return TRUE;
error:
    pe_unmap_full(fmap);
    CloseHandle(fmap->u.pe.hMap);
    return FALSE;
}

/******************************************************************
 *		pe_map_directory
 *
 * Maps a directory content out of a PE file
 */
const char* pe_map_directory(struct module* module, int dirno, DWORD* size)
{
    IMAGE_NT_HEADERS*   nth;
    void*               mapping;

    if (module->type != DMT_PE || !module->format_info[DFI_PE]) return NULL;
    if (dirno >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES ||
        !(mapping = pe_map_full(&module->format_info[DFI_PE]->u.pe_info->fmap, &nth)))
        return NULL;
    if (size) *size = nth->OptionalHeader.DataDirectory[dirno].Size;
    return RtlImageRvaToVa(nth, mapping,
                           nth->OptionalHeader.DataDirectory[dirno].VirtualAddress, NULL);
}

BOOL pe_unmap_directory(struct module* module, int dirno, const char *dir)
{
    if (module->type != DMT_PE || !module->format_info[DFI_PE]) return FALSE;
    if (dirno >= IMAGE_NUMBEROF_DIRECTORY_ENTRIES) return FALSE;
    pe_unmap_full(&module->format_info[DFI_PE]->u.pe_info->fmap);
    return TRUE;
}

/* Locks a region from a mapped PE file, from its RVA, and for at least 'size' bytes.
 * Region must fit entirely inside a PE section.
 * 'length', upon success, gets the size from RVA until end of PE section.
 */
const BYTE* pe_lock_region_from_rva(struct module *module, DWORD rva, DWORD size, DWORD *length)
{
    IMAGE_NT_HEADERS*     nth;
    void*                 mapping;
    IMAGE_SECTION_HEADER *section;
    const BYTE           *ret;

    if (module->type != DMT_PE || !module->format_info[DFI_PE]) return NULL;
    if (!(mapping = pe_map_full(&module->format_info[DFI_PE]->u.pe_info->fmap, &nth)))
        return NULL;
    section = NULL;
    ret = RtlImageRvaToVa(nth, mapping, rva, &section);
    if (ret)
    {
        if (rva + size <= section->VirtualAddress + section->SizeOfRawData)
        {
            if (length)
                *length = section->VirtualAddress + section->SizeOfRawData - rva;
            return ret;
        }
        if (rva + size <= section->VirtualAddress + section->Misc.VirtualSize)
            FIXME("Not able to lock regions not present on file\n");
    }
    pe_unmap_full(&module->format_info[DFI_PE]->u.pe_info->fmap);
    return NULL;
}

BOOL pe_unlock_region(struct module *module, const BYTE* region)
{
    if (module->type != DMT_PE || !module->format_info[DFI_PE] || !region) return FALSE;
    pe_unmap_full(&module->format_info[DFI_PE]->u.pe_info->fmap);
    return TRUE;
}

static void pe_module_remove(struct module_format* modfmt)
{
    image_unmap_file(&modfmt->u.pe_info->fmap);
    HeapFree(GetProcessHeap(), 0, modfmt);
}

/******************************************************************
 *		pe_locate_with_coff_symbol_table
 *
 * Use the COFF symbol table (if any) from the IMAGE_FILE_HEADER to set the absolute address
 * of global symbols.
 * Mingw32 requires this for stabs debug information as address for global variables isn't filled in
 * (this is similar to what is done in elf_module.c when using the .symtab ELF section)
 */
static BOOL pe_locate_with_coff_symbol_table(struct module* module)
{
    struct image_file_map* fmap = &module->format_info[DFI_PE]->u.pe_info->fmap;
    const IMAGE_SYMBOL* isym;
    int                 i, numsym, naux;
    char                tmp[9];
    const char*         name;
    struct hash_table_iter      hti;
    void*               ptr;
    struct symt_data*   sym;
    const char*         mapping;

    numsym = fmap->u.pe.file_header.NumberOfSymbols;
    if (!fmap->u.pe.file_header.PointerToSymbolTable || !numsym)
        return TRUE;
    if (!(mapping = pe_map_full(fmap, NULL))) return FALSE;
    isym = (const IMAGE_SYMBOL*)(mapping + fmap->u.pe.file_header.PointerToSymbolTable);

    for (i = 0; i < numsym; i+= naux, isym += naux)
    {
        if (isym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            isym->SectionNumber > 0 && isym->SectionNumber <= fmap->u.pe.file_header.NumberOfSections)
        {
            if (isym->N.Name.Short)
            {
                name = memcpy(tmp, isym->N.ShortName, 8);
                tmp[8] = '\0';
            }
            else name = fmap->u.pe.strtable + isym->N.Name.Long;
            if (name[0] == '_') name++;
            hash_table_iter_init(&module->ht_symbols, &hti, name);
            while ((ptr = hash_table_iter_up(&hti)))
            {
                sym = CONTAINING_RECORD(ptr, struct symt_data, hash_elt);
                if (sym->symt.tag == SymTagData &&
                    (sym->kind == DataIsGlobal || sym->kind == DataIsFileStatic) &&
                    sym->u.var.kind == loc_absolute &&
                    !strcmp(sym->hash_elt.name, name))
                {
                    TRACE("Changing absolute address for %d.%s: %Ix -> %I64x\n",
                          isym->SectionNumber, debugstr_a(name), sym->u.var.offset,
                          module->module.BaseOfImage +
                          fmap->u.pe.sect[isym->SectionNumber - 1].shdr.VirtualAddress +
                          isym->Value);
                    sym->u.var.offset = module->module.BaseOfImage +
                        fmap->u.pe.sect[isym->SectionNumber - 1].shdr.VirtualAddress + isym->Value;
                    break;
                }
            }
        }
        naux = isym->NumberOfAuxSymbols + 1;
    }
    pe_unmap_full(fmap);
    return TRUE;
}

/******************************************************************
 *		pe_load_coff_symbol_table
 *
 * Load public symbols out of the COFF symbol table (if any).
 */
static BOOL pe_load_coff_symbol_table(struct module* module)
{
    struct image_file_map* fmap = &module->format_info[DFI_PE]->u.pe_info->fmap;
    const IMAGE_SYMBOL* isym;
    int                 i, numsym, naux;
    const char*         strtable;
    char                tmp[9];
    const char*         name;
    const char*         lastfilename = NULL;
    struct symt_compiland*   compiland = NULL;
    const IMAGE_SECTION_HEADER* sect;
    const char*         mapping;

    numsym = fmap->u.pe.file_header.NumberOfSymbols;
    if (!fmap->u.pe.file_header.PointerToSymbolTable || !numsym)
        return TRUE;
    if (!(mapping = pe_map_full(fmap, NULL))) return FALSE;
    isym = (const IMAGE_SYMBOL*)(mapping + fmap->u.pe.file_header.PointerToSymbolTable);
    /* FIXME: no way to get strtable size */
    strtable = (const char*)&isym[numsym];
    sect = IMAGE_FIRST_SECTION(RtlImageNtHeader((HMODULE)mapping));

    for (i = 0; i < numsym; i+= naux, isym += naux)
    {
        if (isym->StorageClass == IMAGE_SYM_CLASS_FILE)
        {
            lastfilename = (const char*)(isym + 1);
            compiland = NULL;
        }
        if (isym->StorageClass == IMAGE_SYM_CLASS_EXTERNAL &&
            isym->SectionNumber > 0 && isym->SectionNumber <= fmap->u.pe.file_header.NumberOfSections)
        {
            if (isym->N.Name.Short)
            {
                name = memcpy(tmp, isym->N.ShortName, 8);
                tmp[8] = '\0';
            }
            else name = strtable + isym->N.Name.Long;
            if (name[0] == '_') name++;

            if (!compiland && lastfilename)
                compiland = symt_new_compiland(module, lastfilename);

            if (!(dbghelp_options & SYMOPT_NO_PUBLICS))
                symt_new_public(module, compiland, name, FALSE,
                                module->module.BaseOfImage + sect[isym->SectionNumber - 1].VirtualAddress +
                                     isym->Value,
                                1);
        }
        naux = isym->NumberOfAuxSymbols + 1;
    }
    module->module.SymType = SymCoff;
    module->module.LineNumbers = FALSE;
    module->module.GlobalSymbols = FALSE;
    module->module.TypeInfo = FALSE;
    module->module.SourceIndexed = FALSE;
    module->module.Publics = TRUE;
    pe_unmap_full(fmap);

    return TRUE;
}

/******************************************************************
 *		pe_load_stabs
 *
 * look for stabs information in PE header (it's how the mingw compiler provides 
 * its debugging information)
 */
static BOOL pe_load_stabs(const struct process* pcs, struct module* module)
{
    struct image_file_map*      fmap = &module->format_info[DFI_PE]->u.pe_info->fmap;
    struct image_section_map    sect_stabs, sect_stabstr;
    BOOL                        ret = FALSE;

    if (pe_find_section(fmap, ".stab", &sect_stabs) && pe_find_section(fmap, ".stabstr", &sect_stabstr))
    {
        const char* stab;
        const char* stabstr;

        stab = image_map_section(&sect_stabs);
        stabstr = image_map_section(&sect_stabstr);
        if (stab != IMAGE_NO_MAP && stabstr != IMAGE_NO_MAP)
        {
            ret = stabs_parse(module,
                              module->module.BaseOfImage - PE_FROM_OPTHDR(fmap, ImageBase),
                              stab, image_get_map_size(&sect_stabs) / sizeof(struct stab_nlist), sizeof(struct stab_nlist),
                              stabstr, image_get_map_size(&sect_stabstr),
                              NULL, NULL);
        }
        image_unmap_section(&sect_stabs);
        image_unmap_section(&sect_stabstr);
        if (ret) pe_locate_with_coff_symbol_table(module);
    }
    TRACE("%s the STABS debug info\n", ret ? "successfully loaded" : "failed to load");

    return ret;
}

/******************************************************************
 *		pe_load_dwarf
 *
 * look for dwarf information in PE header (it's also a way for the mingw compiler
 * to provide its debugging information)
 */
static BOOL pe_load_dwarf(struct module* module)
{
    struct image_file_map*      fmap = &module->format_info[DFI_PE]->u.pe_info->fmap;
    BOOL                        ret;

    ret = dwarf2_parse(module,
                       module->module.BaseOfImage - PE_FROM_OPTHDR(fmap, ImageBase),
                       NULL, /* FIXME: some thunks to deal with ? */
                       fmap);
    TRACE("%s the DWARF debug info\n", ret ? "successfully loaded" : "failed to load");

    return ret;
}

/******************************************************************
 *		pe_load_dbg_file
 *
 * loads a .dbg file
 */
static BOOL pe_load_dbg_file(const struct process* pcs, struct module* module,
                             const char* dbg_name, DWORD timestamp)
{
    HANDLE                              hFile = INVALID_HANDLE_VALUE, hMap = 0;
    const BYTE*                         dbg_mapping = NULL;
    BOOL                                ret = FALSE;
    SYMSRV_INDEX_INFOW                  info;

    TRACE("Processing DBG file %s\n", debugstr_a(dbg_name));

    if (path_find_symbol_file(pcs, module, dbg_name, FALSE, NULL, timestamp, 0, &info, &module->module.DbgUnmatched) &&
        (hFile = CreateFileW(info.dbgfile, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) != INVALID_HANDLE_VALUE &&
        ((hMap = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL)) != 0) &&
        ((dbg_mapping = MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0)) != NULL))
    {
        const IMAGE_SEPARATE_DEBUG_HEADER*      hdr;
        const IMAGE_SECTION_HEADER*             sectp;
        const IMAGE_DEBUG_DIRECTORY*            dbg;

        hdr = (const IMAGE_SEPARATE_DEBUG_HEADER*)dbg_mapping;
        /* section headers come immediately after debug header */
        sectp = (const IMAGE_SECTION_HEADER*)(hdr + 1);
        /* and after that and the exported names comes the debug directory */
        dbg = (const IMAGE_DEBUG_DIRECTORY*)
            (dbg_mapping + sizeof(*hdr) +
             hdr->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
             hdr->ExportedNamesSize);

        ret = pe_load_debug_directory(pcs, module, dbg_mapping, sectp,
                                      hdr->NumberOfSections, dbg,
                                      hdr->DebugDirectorySize / sizeof(*dbg));
    }
    else
        ERR("Couldn't find .DBG file %s (%s)\n", debugstr_a(dbg_name), debugstr_w(info.dbgfile));

    if (dbg_mapping) UnmapViewOfFile(dbg_mapping);
    if (hMap) CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);
    return ret;
}

/******************************************************************
 *		pe_load_msc_debug_info
 *
 * Process MSC debug information in PE file.
 */
static BOOL pe_load_msc_debug_info(const struct process* pcs, struct module* module)
{
    struct image_file_map*      fmap = &module->format_info[DFI_PE]->u.pe_info->fmap;
    BOOL                        ret = FALSE;
    const IMAGE_DEBUG_DIRECTORY*dbg;
    ULONG                       nDbg;
    void*                       mapping;
    IMAGE_NT_HEADERS*           nth;

    if (!(mapping = pe_map_full(fmap, &nth))) return FALSE;
    /* Read in debug directory */
    dbg = RtlImageDirectoryEntryToData( mapping, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &nDbg );
    nDbg = dbg ? nDbg / sizeof(IMAGE_DEBUG_DIRECTORY) : 0;

    /* Parse debug directory */
    if (nth->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)
    {
        /* Debug info is stripped to .DBG file */
        const IMAGE_DEBUG_MISC *misc = NULL;
        if (nDbg == 1 && dbg->Type == IMAGE_DEBUG_TYPE_MISC)
        {
            misc = (const IMAGE_DEBUG_MISC *)((const char *)mapping + dbg->PointerToRawData);
            if (misc->DataType == IMAGE_DEBUG_MISC_EXENAME)
                ret = pe_load_dbg_file(pcs, module, (const char*)misc->Data, nth->FileHeader.TimeDateStamp);
            else
                misc = NULL;
        }
        if (!misc)
            WARN("-Debug info stripped, but no .DBG file in module %s\n",
                 debugstr_w(module->modulename));
    }
    else
    {
        /* Debug info is embedded into PE module */
        ret = pe_load_debug_directory(pcs, module, mapping, IMAGE_FIRST_SECTION( nth ),
                                      nth->FileHeader.NumberOfSections, dbg, nDbg);
    }
    pe_unmap_full(fmap);
    return ret;
}

/***********************************************************************
 *			pe_load_export_debug_info
 */
static BOOL pe_load_export_debug_info(const struct process* pcs, struct module* module)
{
    struct image_file_map*              fmap = &module->format_info[DFI_PE]->u.pe_info->fmap;
    unsigned int 		        i;
    const IMAGE_EXPORT_DIRECTORY* 	exports;
    DWORD_PTR			        base = module->module.BaseOfImage;
    DWORD                               size;
    IMAGE_NT_HEADERS*                   nth;
    void*                               mapping;

    if (dbghelp_options & SYMOPT_NO_PUBLICS) return TRUE;

    if (!(mapping = pe_map_full(fmap, &nth))) return FALSE;
#if 0
    /* Add start of DLL (better use the (yet unimplemented) Exe SymTag for this) */
    /* FIXME: module.ModuleName isn't correctly set yet if it's passed in SymLoadModule */
    symt_new_public(module, NULL, module->module.ModuleName, FALSE, base, 1);
#endif
    
    /* Add entry point */
    symt_new_public(module, NULL, "EntryPoint", FALSE,
                    base + nth->OptionalHeader.AddressOfEntryPoint, 1);
#if 0
    /* FIXME: we'd better store addresses linked to sections rather than 
       absolute values */
    IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION( nth );
    /* Add start of sections */
    for (i = 0; i < nth->FileHeader.NumberOfSections; i++, section++) 
    {
	symt_new_public(module, NULL, section->Name, FALSE,
                        RtlImageRvaToVa(nth, mapping, section->VirtualAddress, NULL), 1);
    }
#endif

    /* Add exported functions */
    if ((exports = RtlImageDirectoryEntryToData(mapping, FALSE,
                                                IMAGE_DIRECTORY_ENTRY_EXPORT, &size)))
    {
        const WORD*             ordinals = NULL;
        const DWORD*	        functions = NULL;
        const DWORD*		names = NULL;
        unsigned int		j;
        char			buffer[16];

        functions = RtlImageRvaToVa(nth, mapping, exports->AddressOfFunctions, NULL);
        ordinals  = RtlImageRvaToVa(nth, mapping, exports->AddressOfNameOrdinals, NULL);
        names     = RtlImageRvaToVa(nth, mapping, exports->AddressOfNames, NULL);

        if (functions && ordinals && names)
        {
            for (i = 0; i < exports->NumberOfNames; i++)
            {
                if (!names[i]) continue;
                symt_new_public(module, NULL,
                                RtlImageRvaToVa(nth, mapping, names[i], NULL),
                                FALSE,
                                base + functions[ordinals[i]], 1);
            }

            for (i = 0; i < exports->NumberOfFunctions; i++)
            {
                if (!functions[i]) continue;
                /* Check if we already added it with a name */
                for (j = 0; j < exports->NumberOfNames; j++)
                    if ((ordinals[j] == i) && names[j]) break;
                if (j < exports->NumberOfNames) continue;
                snprintf(buffer, sizeof(buffer), "%ld", i + exports->Base);
                symt_new_public(module, NULL, buffer, FALSE, base + functions[i], 1);
            }
        }
    }
    /* no real debug info, only entry points */
    if (module->module.SymType == SymDeferred)
        module->module.SymType = SymExport;
    pe_unmap_full(fmap);

    return TRUE;
}

/******************************************************************
 *		pe_load_debug_info
 *
 */
BOOL pe_load_debug_info(const struct process* pcs, struct module* module)
{
    BOOL                ret = FALSE;

    if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
    {
        if (!module->dont_load_symbols)
        {
            ret = image_check_alternate(&module->format_info[DFI_PE]->u.pe_info->fmap, module);
            ret = pe_load_stabs(pcs, module) || ret;
            ret = pe_load_dwarf(module) || ret;
        }
        ret = pe_load_msc_debug_info(pcs, module) || ret;
        ret = ret || pe_load_coff_symbol_table(module); /* FIXME */
        /* if we still have no debug info (we could only get SymExport at this
         * point), then do the SymExport except if we have an ELF container,
         * in which case we'll rely on the export's on the ELF side
         */
    }
    /* FIXME:
     * - only loading export debug info in last resort when none of the available formats succeeded
     *   (assuming export debug info is a subset of actual debug infomation).
     */
    if (module->module.SymType == SymDeferred)
    {
        ret = pe_load_export_debug_info(pcs, module) || ret;
        if (module->module.SymType == SymDeferred)
            module->module.SymType = SymNone;
    }
    return ret;
}

struct builtin_search
{
    WCHAR *path;
    struct image_file_map fmap;
};

static BOOL search_builtin_pe(void *param, HANDLE handle, const WCHAR *path)
{
    struct builtin_search *search = param;

    if (!pe_map_file(handle, &search->fmap)) return FALSE;

    search->path = wcsdup(path);
    return TRUE;
}

static const struct module_format_vtable pe_module_format_vtable =
{
    pe_module_remove,
    NULL,
};

/******************************************************************
 *		pe_load_native_module
 *
 */
struct module* pe_load_native_module(struct process* pcs, const WCHAR* name,
                                     HANDLE hFile, DWORD64 base, DWORD size)
{
    struct module*              module = NULL;
    BOOL                        opened = FALSE;
    struct module_format*       modfmt;
    WCHAR                       loaded_name[MAX_PATH];
    WCHAR*                      real_path = NULL;

    loaded_name[0] = '\0';
    if (!hFile)
    {
        assert(name);

        if ((hFile = FindExecutableImageExW(name, pcs->search_path, loaded_name, NULL, NULL)) == NULL &&
            (hFile = FindExecutableImageExW(name, L".", loaded_name, NULL, NULL)) == NULL)
            return NULL;
        opened = TRUE;
    }
    else
    {
        ULONG sz = sizeof(OBJECT_NAME_INFORMATION) + MAX_PATH * sizeof(WCHAR), needed;
        OBJECT_NAME_INFORMATION *obj_name;
        NTSTATUS nts;

        obj_name = RtlAllocateHeap(GetProcessHeap(), 0, sz);
        if (obj_name)
        {
            nts = NtQueryObject(hFile, ObjectNameInformation, obj_name, sz, &needed);
            if (nts == STATUS_BUFFER_OVERFLOW)
            {
                sz = needed;
                obj_name = RtlReAllocateHeap(GetProcessHeap(), 0, obj_name, sz);
                nts = NtQueryObject(hFile, ObjectNameInformation, obj_name, sz, &needed);
            }
            if (!nts)
            {
                obj_name->Name.Buffer[obj_name->Name.Length / sizeof(WCHAR)] = L'\0';
                real_path = wcsdup(obj_name->Name.Buffer);
            }
            RtlFreeHeap(GetProcessHeap(), 0, obj_name);
        }
        if (name) lstrcpyW(loaded_name, name);
    }

    if ((modfmt = HeapAlloc(GetProcessHeap(), 0, sizeof(struct module_format) + sizeof(struct pe_module_info))))
    {
        modfmt->u.pe_info = (struct pe_module_info*)(modfmt + 1);
        if (pe_map_file(hFile, &modfmt->u.pe_info->fmap))
        {
            struct builtin_search builtin = { NULL };
            if (opened && modfmt->u.pe_info->fmap.u.pe.builtin &&
                search_dll_path(pcs, loaded_name, modfmt->u.pe_info->fmap.u.pe.file_header.Machine, search_builtin_pe, &builtin))
            {
                TRACE("reloaded %s from %s\n", debugstr_w(loaded_name), debugstr_w(builtin.path));
                image_unmap_file(&modfmt->u.pe_info->fmap);
                modfmt->u.pe_info->fmap = builtin.fmap;
                real_path = builtin.path;
            }
            if (!base) base = PE_FROM_OPTHDR(&modfmt->u.pe_info->fmap, ImageBase);
            if (!size) size = PE_FROM_OPTHDR(&modfmt->u.pe_info->fmap, SizeOfImage);

            module = module_new(pcs, loaded_name, DMT_PE, modfmt->u.pe_info->fmap.u.pe.builtin, FALSE,
                                base, size,
                                modfmt->u.pe_info->fmap.u.pe.file_header.TimeDateStamp,
                                PE_FROM_OPTHDR(&modfmt->u.pe_info->fmap, CheckSum),
                                modfmt->u.pe_info->fmap.u.pe.file_header.Machine);
            if (module)
            {
                module->real_path = real_path ? pool_wcsdup(&module->pool, real_path) : NULL;
                modfmt->module = module;
                modfmt->vtable = &pe_module_format_vtable;
                module->format_info[DFI_PE] = modfmt;
                module->reloc_delta = base - PE_FROM_OPTHDR(&modfmt->u.pe_info->fmap, ImageBase);
            }
            else
            {
                ERR("could not load the module '%s'\n", debugstr_w(loaded_name));
                image_unmap_file(&modfmt->u.pe_info->fmap);
            }
        }
        if (!module) HeapFree(GetProcessHeap(), 0, modfmt);
    }

    if (opened) CloseHandle(hFile);
    free(real_path);
    return module;
}

/******************************************************************
 *		pe_load_nt_header
 *
 */
BOOL pe_load_nt_header(HANDLE hProc, DWORD64 base, IMAGE_NT_HEADERS* nth, BOOL* is_builtin)
{
    IMAGE_DOS_HEADER    dos;

    if (!ReadProcessMemory(hProc, (char*)(DWORD_PTR)base, &dos, sizeof(dos), NULL) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE ||
        !ReadProcessMemory(hProc, (char*)(DWORD_PTR)(base + dos.e_lfanew),
                           nth, sizeof(*nth), NULL) ||
        nth->Signature != IMAGE_NT_SIGNATURE)
        return FALSE;
    if (is_builtin)
    {
        if (dos.e_lfanew >= sizeof(dos) + sizeof(builtin_signature))
        {
            char sig[sizeof(builtin_signature)];
            *is_builtin = ReadProcessMemory(hProc, (char*)(DWORD_PTR)base + sizeof(dos), sig, sizeof(sig), NULL) &&
                !memcmp(sig, builtin_signature, sizeof(builtin_signature));
        }
        else *is_builtin = FALSE;
    }
    return TRUE;
}

/******************************************************************
 *		pe_load_builtin_module
 *
 */
struct module* pe_load_builtin_module(struct process* pcs, const WCHAR* name,
                                      DWORD64 base, DWORD64 size)
{
    struct module*      module = NULL;

    if (base && pcs->dbg_hdr_addr)
    {
        IMAGE_NT_HEADERS    nth;
        BOOL is_builtin;

        if (pe_load_nt_header(pcs->handle, base, &nth, &is_builtin))
        {
            if (!size) size = nth.OptionalHeader.SizeOfImage;
            module = module_new(pcs, name, DMT_PE, is_builtin, FALSE, base, size,
                                nth.FileHeader.TimeDateStamp,
                                nth.OptionalHeader.CheckSum,
                                nth.FileHeader.Machine);
        }
    }
    return module;
}

/***********************************************************************
 *           ImageDirectoryEntryToDataEx (DBGHELP.@)
 *
 * Search for specified directory in PE image
 *
 * PARAMS
 *
 *   base    [in]  Image base address
 *   image   [in]  TRUE - image has been loaded by loader, FALSE - raw file image
 *   dir     [in]  Target directory index
 *   size    [out] Receives directory size
 *   section [out] Receives pointer to section header of section containing directory data
 *
 * RETURNS
 *   Success: pointer to directory data
 *   Failure: NULL
 *
 */
PVOID WINAPI ImageDirectoryEntryToDataEx( PVOID base, BOOLEAN image, USHORT dir, PULONG size, PIMAGE_SECTION_HEADER *section )
{
    const IMAGE_NT_HEADERS *nt;
    DWORD_PTR addr;

    *size = 0;
    if (section) *section = NULL;

    if (!(nt = RtlImageNtHeader( base ))) return NULL;
    if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_NT_HEADERS64 *nt64 = (const IMAGE_NT_HEADERS64 *)nt;

        if (dir >= nt64->OptionalHeader.NumberOfRvaAndSizes) return NULL;
        if (!(addr = nt64->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
        *size = nt64->OptionalHeader.DataDirectory[dir].Size;
        if (image || addr < nt64->OptionalHeader.SizeOfHeaders) return (char *)base + addr;
    }
    else if (nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        const IMAGE_NT_HEADERS32 *nt32 = (const IMAGE_NT_HEADERS32 *)nt;

        if (dir >= nt32->OptionalHeader.NumberOfRvaAndSizes) return NULL;
        if (!(addr = nt32->OptionalHeader.DataDirectory[dir].VirtualAddress)) return NULL;
        *size = nt32->OptionalHeader.DataDirectory[dir].Size;
        if (image || addr < nt32->OptionalHeader.SizeOfHeaders) return (char *)base + addr;
    }
    else return NULL;

    return RtlImageRvaToVa( nt, base, addr, section );
}

/***********************************************************************
 *         ImageDirectoryEntryToData   (DBGHELP.@)
 *
 * NOTES
 *   See ImageDirectoryEntryToDataEx
 */
PVOID WINAPI ImageDirectoryEntryToData( PVOID base, BOOLEAN image, USHORT dir, PULONG size )
{
    return ImageDirectoryEntryToDataEx( base, image, dir, size, NULL );
}

DWORD pe_get_file_indexinfo(void* image, DWORD size, SYMSRV_INDEX_INFOW* info)
{
    const IMAGE_NT_HEADERS* nthdr;
    const IMAGE_DEBUG_DIRECTORY* dbg;
    ULONG dirsize;

    if (!(nthdr = RtlImageNtHeader(image))) return ERROR_BAD_FORMAT;

    dbg = RtlImageDirectoryEntryToData(image, FALSE, IMAGE_DIRECTORY_ENTRY_DEBUG, &dirsize);
    if (!dbg) dirsize = 0;

    /* fill in information from NT header */
    info->timestamp = nthdr->FileHeader.TimeDateStamp;
    info->stripped = (nthdr->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) != 0;
    if (nthdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        const IMAGE_NT_HEADERS64* nthdr64 = (const IMAGE_NT_HEADERS64*)nthdr;
        info->size = nthdr64->OptionalHeader.SizeOfImage;
    }
    else if (nthdr->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
    {
        const IMAGE_NT_HEADERS32* nthdr32 = (const IMAGE_NT_HEADERS32*)nthdr;
        info->size = nthdr32->OptionalHeader.SizeOfImage;
    }
    return msc_get_file_indexinfo(image, dbg, dirsize / sizeof(*dbg), info);
}

/* check if image contains a debug entry that contains a gcc/mingw - clang build-id information */
BOOL pe_has_buildid_debug(struct image_file_map *fmap, GUID *guid)
{
    BOOL ret = FALSE;

    if (fmap->modtype == DMT_PE)
    {
        SYMSRV_INDEX_INFOW info = {.sizeofstruct = sizeof(info)};
        const void *image = pe_map_full(fmap, NULL);

        if (image)
        {
            DWORD retval = pe_get_file_indexinfo((void*)image, GetFileSize(fmap->u.pe.hMap, NULL), &info);
            if ((retval == ERROR_SUCCESS || retval == ERROR_BAD_EXE_FORMAT) && info.age && !info.pdbfile[0])
            {
                *guid = info.guid;
                ret = TRUE;
            }
            pe_unmap_full(fmap);
        }
    }
    return ret;
}
