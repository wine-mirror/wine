/*
 * File elf.c - processing of ELF files
 *
 * Copyright (C) 1996, Eric Youngdale.
 *		 1999-2004 Eric Pouech
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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif

#include "dbghelp_private.h"

#if defined(__svr4__) || defined(__sun)
#define __ELF__
#endif

#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_SYS_ELF32_H
# include <sys/elf32.h>
#endif
#ifdef HAVE_SYS_EXEC_ELF_H
# include <sys/exec_elf.h>
#endif
#if !defined(DT_NUM)
# if defined(DT_COUNT)
#  define DT_NUM DT_COUNT
# else
/* this seems to be a satisfactory value on Solaris, which doesn't support this AFAICT */
#  define DT_NUM 24
# endif
#endif
#ifdef HAVE_LINK_H
# include <link.h>
#endif
#ifdef HAVE_SYS_LINK_H
# include <sys/link.h>
#endif

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dbghelp);

struct elf_module_info
{
    unsigned long               elf_addr;
    unsigned short	        elf_mark : 1,
                                elf_loader : 1;
};

#ifdef __ELF__

#define ELF_INFO_DEBUG_HEADER   0x0001
#define ELF_INFO_MODULE         0x0002

struct elf_info
{
    unsigned                    flags;          /* IN  one (or several) of the ELF_INFO constants */
    unsigned long               dbg_hdr_addr;   /* OUT address of debug header (if ELF_INFO_DEBUG_HEADER is set) */
    struct module*              module;         /* OUT loaded module (if ELF_INFO_MODULE is set) */
};

struct symtab_elt
{
    struct hash_table_elt       ht_elt;
    const Elf32_Sym*            symp;
    const char*                 filename;
    unsigned                    used;
};

struct thunk_area
{
    const char*                 symname;
    THUNK_ORDINAL               ordinal;
    unsigned long               rva_start;
    unsigned long               rva_end;
};

/******************************************************************
 *		elf_hash_symtab
 *
 * creating an internal hash table to ease use ELF symtab information lookup
 */
static void elf_hash_symtab(const struct module* module, struct pool* pool, 
                            struct hash_table* ht_symtab, const char* map_addr,
                            const Elf32_Shdr* symtab, const Elf32_Shdr* strtab,
                            unsigned num_areas, struct thunk_area* thunks)
{
    int		                i, j, nsym;
    const char*                 strp;
    const char*                 symname;
    const char*                 filename = NULL;
    const Elf32_Sym*            symp;
    struct symtab_elt*          ste;

    symp = (const Elf32_Sym*)(map_addr + symtab->sh_offset);
    nsym = symtab->sh_size / sizeof(*symp);
    strp = (const char*)(map_addr + strtab->sh_offset);

    for (j = 0; j < num_areas; j++)
        thunks[j].rva_start = thunks[j].rva_end = 0;

    for (i = 0; i < nsym; i++, symp++)
    {
        /* Ignore certain types of entries which really aren't of that much
         * interest.
         */
        if (ELF32_ST_TYPE(symp->st_info) == STT_SECTION || symp->st_shndx == SHN_UNDEF)
        {
            continue;
        }

        symname = strp + symp->st_name;

        if (ELF32_ST_TYPE(symp->st_info) == STT_FILE)
        {
            filename = symname;
            continue;
        }
        for (j = 0; j < num_areas; j++)
        {
            if (!strcmp(symname, thunks[j].symname))
            {
                thunks[j].rva_start = symp->st_value;
                thunks[j].rva_end   = symp->st_value + symp->st_size;
                break;
            }
        }
        if (j < num_areas) continue;
        ste = pool_alloc(pool, sizeof(*ste));
        ste->ht_elt.name = symname;
        ste->symp        = symp;
        ste->filename    = filename;
        ste->used        = 0;
        hash_table_add(ht_symtab, &ste->ht_elt);
    }
}

/******************************************************************
 *		elf_lookup_symtab
 *
 * lookup a symbol by name in our internal hash table for the symtab
 */
static const Elf32_Sym* elf_lookup_symtab(const struct module* module,        
                                          const struct hash_table* ht_symtab,
                                          const char* name, struct symt* compiland)
{
    struct symtab_elt*          weak_result = NULL; /* without compiland name */
    struct symtab_elt*          result = NULL;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;
    const char*                 compiland_name;
    const char*                 compiland_basename;
    const char*                 base;

    /* we need weak match up (at least) when symbols of same name, 
     * defined several times in different compilation units,
     * are merged in a single one (hence a different filename for c.u.)
     */
    if (compiland)
    {
        compiland_name = source_get(module,
                                    ((struct symt_compiland*)compiland)->source);
        compiland_basename = strrchr(compiland_name, '/');
        if (!compiland_basename++) compiland_basename = compiland_name;
    }
    else compiland_name = compiland_basename = NULL;
    
    hash_table_iter_init(ht_symtab, &hti, name);
    while ((ste = hash_table_iter_up(&hti)))
    {
        if (ste->used || strcmp(ste->ht_elt.name, name)) continue;

        weak_result = ste;
        if ((ste->filename && !compiland_name) || (!ste->filename && compiland_name))
            continue;
        if (ste->filename && compiland_name)
        {
            if (strcmp(ste->filename, compiland_name))
            {
                base = strrchr(ste->filename, '/');
                if (!base++) base = ste->filename;
                if (strcmp(base, compiland_basename)) continue;
            }
        }
        if (result)
        {
            FIXME("Already found symbol %s (%s) in symtab %s @%08x and %s @%08x\n", 
                  name, compiland_name, result->filename, result->symp->st_value,
                  ste->filename, ste->symp->st_value);
        }
        else
        {
            result = ste;
            ste->used = 1;
        }
    }
    if (!result && !(result = weak_result))
    {
        FIXME("Couldn't find symbol %s.%s in symtab\n", 
              module->module.ModuleName, name);
        return NULL;
    }
    return result->symp;
}

/******************************************************************
 *		elf_finish_stabs_info
 *
 * - get any relevant information (address & size) from the bits we got from the
 *   stabs debugging information
 */
static void elf_finish_stabs_info(struct module* module, struct hash_table* symtab)
{
    struct hash_table_iter      hti;
    void*                       ptr;
    struct symt_ht*             sym;
    const Elf32_Sym*            symp;

    hash_table_iter_init(&module->ht_symbols, &hti, NULL);
    while ((ptr = hash_table_iter_up(&hti)))
    {
        sym = GET_ENTRY(ptr, struct symt_ht, hash_elt);
        switch (sym->symt.tag)
        {
        case SymTagFunction:
            if (((struct symt_function*)sym)->address != module->elf_info->elf_addr &&
                ((struct symt_function*)sym)->size)
            {
                break;
            }
            symp = elf_lookup_symtab(module, symtab, sym->hash_elt.name, 
                                     ((struct symt_function*)sym)->container);
            if (symp)
            {
                ((struct symt_function*)sym)->address = module->elf_info->elf_addr +
                                                        symp->st_value;
                ((struct symt_function*)sym)->size    = symp->st_size;
            }
            break;
        case SymTagData:
            switch (((struct symt_data*)sym)->kind)
            {
            case DataIsGlobal:
            case DataIsFileStatic:
                if (((struct symt_data*)sym)->u.address != module->elf_info->elf_addr)
                    break;
                symp = elf_lookup_symtab(module, symtab, sym->hash_elt.name, 
                                         ((struct symt_data*)sym)->container);
                if (symp)
                {
                    ((struct symt_data*)sym)->u.address = module->elf_info->elf_addr +
                                                          symp->st_value;
                    ((struct symt_data*)sym)->kind = (ELF32_ST_BIND(symp->st_info) == STB_LOCAL) ?
                        DataIsFileStatic : DataIsGlobal;
                }
                break;
            default:;
            }
            break;
        default:
            FIXME("Unsupported tag %u\n", sym->symt.tag);
            break;
        }
    }
    /* since we may have changed some addresses & sizes, mark the module to be resorted */
    module->sortlist_valid = FALSE;
}

/******************************************************************
 *		elf_load_wine_thunks
 *
 * creating the thunk objects for a wine native DLL
 */
static int elf_new_wine_thunks(struct module* module, struct hash_table* ht_symtab,
                               unsigned num_areas, struct thunk_area* thunks)
{
    int		                j;
    struct symt_compiland*      compiland = NULL;
    const char*                 compiland_name = NULL;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;
    DWORD                       addr;
    int                         idx;

    hash_table_iter_init(ht_symtab, &hti, NULL);
    while ((ste = hash_table_iter_up(&hti)))
    {
        if (ste->used) continue;

        /* FIXME: this is not a good idea anyway... we are creating several
         * compiland objects for a same compilation unit
         * We try to cache the last compiland used, but it's not enough
         * (we should here only create compilands if they are not yet 
         *  defined)
         */
        if (!compiland_name || compiland_name != ste->filename)
            compiland = symt_new_compiland(module, 
                                           compiland_name = ste->filename);

        addr = module->elf_info->elf_addr + ste->symp->st_value;

        for (j = 0; j < num_areas; j++)
        {
            if (ste->symp->st_value >= thunks[j].rva_start && 
                ste->symp->st_value < thunks[j].rva_end)
                break;
        }
        if (j < num_areas) /* thunk found */
        {
            symt_new_thunk(module, compiland, ste->ht_elt.name, thunks[j].ordinal,
                           addr, ste->symp->st_size);
        }
        else
        {
            DWORD       ref_addr;

            idx = symt_find_nearest(module, addr);
            if (idx != -1)
                symt_get_info(&module->addr_sorttab[idx]->symt, 
                              TI_GET_ADDRESS, &ref_addr);
            if (idx == -1 || addr != ref_addr)
            {
                /* creating public symbols for all the ELF symbols which haven't been
                 * used yet (ie we have no debug information on them)
                 * That's the case, for example, of the .spec.c files
                 */
                if (ELF32_ST_TYPE(ste->symp->st_info) == STT_FUNC)
                {
                    symt_new_function(module, compiland, ste->ht_elt.name,
                                      addr, ste->symp->st_size, NULL);
                }
                else
                {
                    symt_new_global_variable(module, compiland, ste->ht_elt.name,
                                             ELF32_ST_BIND(ste->symp->st_info) == STB_LOCAL,
                                             addr, ste->symp->st_size, NULL);
                }
                /* FIXME: this is a hack !!!
                 * we are adding new symbols, but as we're parsing a symbol table
                 * (hopefully without duplicate symbols) we delay rebuilding the sorted
                 * module table until we're done with the symbol table
                 * Otherwise, as we intertwine symbols's add and lookup, performance
                 * is rather bad
                 */
                module->sortlist_valid = TRUE;
            }
            else if (strcmp(ste->ht_elt.name, module->addr_sorttab[idx]->hash_elt.name))
            {
                DWORD   xaddr = 0, xsize = 0;

                symt_get_info(&module->addr_sorttab[idx]->symt, TI_GET_ADDRESS, &xaddr);
                symt_get_info(&module->addr_sorttab[idx]->symt, TI_GET_LENGTH,  &xsize);

                FIXME("Duplicate in %s: %s<%08lx-%08x> %s<%08lx-%08lx>\n", 
                      module->module.ModuleName,
                      ste->ht_elt.name, addr, ste->symp->st_size,
                      module->addr_sorttab[idx]->hash_elt.name, xaddr, xsize);
            }
        }
    }
    /* see comment above */
    module->sortlist_valid = FALSE;
    return TRUE;
}

/******************************************************************
 *		elf_new_public_symbols
 *
 * Creates a set of public symbols from an ELF symtab
 */
static int elf_new_public_symbols(struct module* module, struct hash_table* symtab,
                                  BOOL dont_check)
{
    struct symt_compiland*      compiland = NULL;
    const char*                 compiland_name = NULL;
    struct hash_table_iter      hti;
    struct symtab_elt*          ste;

    if (!(dbghelp_options & SYMOPT_NO_PUBLICS)) return TRUE;

    hash_table_iter_init(symtab, &hti, NULL);
    while ((ste = hash_table_iter_up(&hti)))
    {
        /* FIXME: this is not a good idea anyway... we are creating several
         * compiland objects for a same compilation unit
         * We try to cache the last compiland used, but it's not enough
         * (we should here only create compilands if they are not yet 
         *  defined)
         */
        if (!compiland_name || compiland_name != ste->filename)
            compiland = symt_new_compiland(module, 
                                           compiland_name = ste->filename);

        if (dont_check || !(dbghelp_options & SYMOPT_AUTO_PUBLICS) ||
            symt_find_nearest(module, module->elf_info->elf_addr + ste->symp->st_value) == -1)
        {
            symt_new_public(module, compiland, ste->ht_elt.name,
                            module->elf_info->elf_addr + ste->symp->st_value,
                            ste->symp->st_size, TRUE /* FIXME */, 
                            ELF32_ST_TYPE(ste->symp->st_info) == STT_FUNC);
        }
    }
    return TRUE;
}

/******************************************************************
 *		elf_load_debug_info
 *
 * Loads the symbolic information from ELF module stored in 'filename'
 * the module has been loaded at 'load_offset' address, so symbols' address
 * relocation is performed
 * returns
 *	-1 if the file cannot be found/opened
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
SYM_TYPE elf_load_debug_info(struct module* module)
{
    SYM_TYPE            sym_type = -1;
    char*	        addr = (char*)0xffffffff;
    int		        fd = -1;
    struct stat	        statbuf;
    const Elf32_Ehdr*   ehptr;
    const Elf32_Shdr*   spnt;
    const char*	        shstrtab;
    int	       	        i;
    int                 symtab_sect, dynsym_sect, stab_sect, stabstr_sect, debug_sect;
    struct pool         pool;
    struct hash_table   ht_symtab;
    struct thunk_area   thunks[] = 
    {
        {"__wine_spec_import_thunks",           THUNK_ORDINAL_NOTYPE, 0, 0},    /* inter DLL calls */
        {"__wine_spec_delayed_import_loaders",  THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_spec_delayed_import_thunks",   THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_delay_load",                   THUNK_ORDINAL_LOAD,   0, 0},    /* delayed inter DLL calls */
        {"__wine_spec_thunk_text_16",           -16,                  0, 0},    /* 16 => 32 thunks */
        {"__wine_spec_thunk_data_16",           -16,                  0, 0},    /* 16 => 32 thunks */
        {"__wine_spec_thunk_text_32",           -32,                  0, 0},    /* 32 => 16 thunks */
        {"__wine_spec_thunk_data_32",           -32,                  0, 0},    /* 32 => 16 thunks */
    };

    if (module->type != DMT_ELF || !module->elf_info)
    {
	ERR("Bad elf module '%s'\n", module->module.LoadedImageName);
	return sym_type;
    }

    TRACE("%s\n", module->module.LoadedImageName);
    /* check that the file exists, and that the module hasn't been loaded yet */
    if (stat(module->module.LoadedImageName, &statbuf) == -1) goto leave;
    if (S_ISDIR(statbuf.st_mode)) goto leave;

    /*
     * Now open the file, so that we can mmap() it.
     */
    if ((fd = open(module->module.LoadedImageName, O_RDONLY)) == -1) goto leave;

    /*
     * Now mmap() the file.
     */
    addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == (char*)0xffffffff) goto leave;

    sym_type = SymNone;
    /*
     * Next, we need to find a few of the internal ELF headers within
     * this thing.  We need the main executable header, and the section
     * table.
     */
    ehptr = (Elf32_Ehdr*)addr;
    spnt = (Elf32_Shdr*)(addr + ehptr->e_shoff);
    shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

    symtab_sect = dynsym_sect = stab_sect = stabstr_sect = debug_sect = -1;

    for (i = 0; i < ehptr->e_shnum; i++)
    {
	if (strcmp(shstrtab + spnt[i].sh_name, ".stab") == 0)
	    stab_sect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".stabstr") == 0)
	    stabstr_sect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".debug_info") == 0)
	    debug_sect = i;
	if ((strcmp(shstrtab + spnt[i].sh_name, ".symtab") == 0) &&
	    (spnt[i].sh_type == SHT_SYMTAB))
            symtab_sect = i;
        if ((strcmp(shstrtab + spnt[i].sh_name, ".dynsym") == 0) &&
            (spnt[i].sh_type == SHT_DYNSYM))
            dynsym_sect = i;
    }

    sym_type = SymExport;

    if (symtab_sect == -1)
    {
        /* if we don't have a symtab but a dynsym, process the dynsym
         * section instead. It'll contain less (relevant) information, 
         * but it'll be better than nothing
         */
        if (dynsym_sect == -1) goto leave;
        symtab_sect = dynsym_sect;
    }

    /* FIXME: guess a better size from ELF info */
    pool_init(&pool, 65536);
    hash_table_init(&pool, &ht_symtab, 256);

    /* create a hash table for the symtab */
    elf_hash_symtab(module, &pool, &ht_symtab, addr, 
                    spnt + symtab_sect, spnt + spnt[symtab_sect].sh_link,
                    sizeof(thunks) / sizeof(thunks[0]), thunks);

    if (stab_sect != -1 && stabstr_sect != -1 && 
        !(dbghelp_options & SYMOPT_PUBLICS_ONLY))
    {
        /* OK, now just parse all of the stabs. */
        sym_type = stabs_parse(module, addr, module->elf_info->elf_addr,
                               spnt[stab_sect].sh_offset, spnt[stab_sect].sh_size,
                               spnt[stabstr_sect].sh_offset,
                               spnt[stabstr_sect].sh_size);
        if (sym_type == -1)
        {
            WARN("Couldn't read correctly read stabs\n");
            goto leave;
        }
        /* and fill in the missing information for stabs */
        elf_finish_stabs_info(module, &ht_symtab);
    }
    else if (debug_sect != -1)
    {
        /* Dwarf 2 debug information */
        FIXME("Unsupported Dwarf2 information\n");
        sym_type = SymNone;
    }
    if (strstr(module->module.ModuleName, "<elf>") ||
        !strcmp(module->module.ModuleName, "<wine-loader>"))
    {
        /* add the thunks for native libraries */
        if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
            elf_new_wine_thunks(module, &ht_symtab, 
                                sizeof(thunks) / sizeof(thunks[0]), thunks);
        /* add the public symbols from symtab
        * (only if they haven't been defined yet)
        */
        elf_new_public_symbols(module, &ht_symtab, FALSE);
    }
    else
    {
        /* add all the public symbols from symtab */
        elf_new_public_symbols(module, &ht_symtab, TRUE);
    }

    pool_destroy(&pool);

leave:
    if (addr != (char*)0xffffffff) munmap(addr, statbuf.st_size);
    if (fd != -1) close(fd);

    return module->module.SymType = sym_type;
}

/******************************************************************
 *		is_dt_flag_valid
 * returns true iff the section tag is valid 
 */
static unsigned is_dt_flag_valid(unsigned d_tag)
{
#ifndef DT_PROCNUM
#define DT_PROCNUM 0
#endif
#ifndef DT_EXTRANUM
#define DT_EXTRANUM 0
#endif
    return (d_tag >= 0 && d_tag < DT_NUM + DT_PROCNUM + DT_EXTRANUM)
#if defined(DT_LOOS) && defined(DT_HIOS)
        || (d_tag >= DT_LOOS && d_tag < DT_HIOS)
#endif
#if defined(DT_LOPROC) && defined(DT_HIPROC)
        || (d_tag >= DT_LOPROC && d_tag < DT_HIPROC)
#endif
        ;
}

/******************************************************************
 *		elf_load_file
 *
 * Loads the information for ELF module stored in 'filename'
 * the module has been loaded at 'load_offset' address
 * returns
 *	-1 if the file cannot be found/opened
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
static SYM_TYPE elf_load_file(struct process* pcs, const char* filename,
                              unsigned long load_offset, struct elf_info* elf_info)
{
    static const BYTE   elf_signature[4] = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };
    SYM_TYPE            sym_type = -1;
    const char*	        addr = (char*)0xffffffff;
    int		        fd = -1;
    struct stat	        statbuf;
    const Elf32_Ehdr*   ehptr;
    const Elf32_Shdr*   spnt;
    const Elf32_Phdr*	ppnt;
    const char*         shstrtab;
    int	       	        i;
    DWORD	        delta, size;
    unsigned            tmp, page_mask = getpagesize() - 1;

    TRACE("Processing elf file '%s' at %08lx\n", filename, load_offset);

    /* check that the file exists, and that the module hasn't been loaded yet */
    if (stat(filename, &statbuf) == -1) goto leave;

    /* Now open the file, so that we can mmap() it. */
    if ((fd = open(filename, O_RDONLY)) == -1) goto leave;

    /* Now mmap() the file. */
    addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == (char*)-1) goto leave;

    sym_type = SymNone;

    /* Next, we need to find a few of the internal ELF headers within
     * this thing.  We need the main executable header, and the section
     * table.
     */
    ehptr = (const Elf32_Ehdr*)addr;
    if (memcmp(ehptr->e_ident, elf_signature, sizeof(elf_signature))) goto leave;

    spnt = (const Elf32_Shdr*)(addr + ehptr->e_shoff);
    shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

    /* if non relocatable ELF, then remove fixed address from computation
     * otherwise, all addresses are zero based
     */
    delta = (load_offset == 0) ? ehptr->e_entry : 0;

    /* grab size of module once loaded in memory */
    ppnt = (const Elf32_Phdr*)(addr + ehptr->e_phoff);
    size = 0;
    for (i = 0; i < ehptr->e_phnum; i++)
    {
        if (ppnt[i].p_type == PT_LOAD)
        {
            tmp = (ppnt[i].p_vaddr + ppnt[i].p_memsz + page_mask) & ~page_mask;
            if (size < tmp) size = tmp;
        }
    }

    if (elf_info->flags & ELF_INFO_DEBUG_HEADER)
    {
        for (i = 0; i < ehptr->e_shnum; i++)
        {
            if (strcmp(shstrtab + spnt[i].sh_name, ".dynamic") == 0 &&
                spnt[i].sh_type == SHT_DYNAMIC)
            {
                Elf32_Dyn       dyn;
                char*           ptr = (char*)spnt[i].sh_addr;
                unsigned long   len;

                do
                {
                    if (!ReadProcessMemory(pcs->handle, ptr, &dyn, sizeof(dyn), &len) ||
                        len != sizeof(dyn) || !is_dt_flag_valid(dyn.d_tag))
                        dyn.d_tag = DT_NULL;
                    ptr += sizeof(dyn);
                } while (dyn.d_tag != DT_DEBUG && dyn.d_tag != DT_NULL);
                if (dyn.d_tag == DT_NULL)
                {
                    sym_type = -1;
                    goto leave;
                }
                elf_info->dbg_hdr_addr = dyn.d_un.d_ptr;
            }
	}
    }

    if (elf_info->flags & ELF_INFO_MODULE)
    {
        elf_info->module = module_new(pcs, filename, DMT_ELF, 
                                      (load_offset) ? load_offset : ehptr->e_entry, 
                                      size, 0, 0);
        if (elf_info->module)
        {
            elf_info->module->elf_info = HeapAlloc(GetProcessHeap(), 
                                                   0, sizeof(struct elf_module_info));
            if (elf_info->module->elf_info == NULL) 
            {
                ERR("OOM\n");
                exit(0); /* FIXME */
            }
            elf_info->module->elf_info->elf_addr = load_offset;
            elf_info->module->module.SymType = sym_type = 
                (dbghelp_options & SYMOPT_DEFERRED_LOADS) ? SymDeferred : 
                elf_load_debug_info(elf_info->module);
            elf_info->module->elf_info->elf_mark = 1;
            elf_info->module->elf_info->elf_loader = 0;
        }
        else sym_type = -1;
    }

leave:
    if (addr != (char*)0xffffffff) munmap((void*)addr, statbuf.st_size);
    if (fd != -1) close(fd);

    return sym_type;
}

/******************************************************************
 *		elf_load_file_from_path
 * tries to load an ELF file from a set of paths (separated by ':')
 */
static SYM_TYPE elf_load_file_from_path(HANDLE hProcess,
                                        const char* filename,
                                        unsigned long load_offset,
                                        const char* path,
                                        struct elf_info* elf_info)
{
    SYM_TYPE            sym_type = -1;
    char                *s, *t, *fn;
    char*	        paths = NULL;

    if (!path) return sym_type;

    paths = strcpy(HeapAlloc(GetProcessHeap(), 0, strlen(path) + 1), path);
    for (s = paths; s && *s; s = (t) ? (t+1) : NULL) 
    {
	t = strchr(s, ':');
	if (t) *t = '\0';
	fn = HeapAlloc(GetProcessHeap(), 0, strlen(filename) + 1 + strlen(s) + 1);
	if (!fn) break;
	strcpy(fn, s);
	strcat(fn, "/");
	strcat(fn, filename);
	sym_type = elf_load_file(hProcess, fn, load_offset, elf_info);
	HeapFree(GetProcessHeap(), 0, fn);
	if (sym_type != -1) break;
	s = (t) ? (t+1) : NULL;
    }
    
    HeapFree(GetProcessHeap(), 0, paths);
    return sym_type;
}

/******************************************************************
 *		elf_search_and_load_file
 *
 * lookup a file in standard ELF locations, and if found, load it
 */
static SYM_TYPE elf_search_and_load_file(struct process* pcs, const char* filename,
                                         unsigned long load_offset, struct elf_info* elf_info)
{
    SYM_TYPE            sym_type = -1;
    struct module*      module;

    if (filename == NULL || *filename == '\0') return sym_type;
    if ((module = module_find_by_name(pcs, filename, DMT_ELF)))
    {
        elf_info->module = module;
        module->elf_info->elf_mark = 1;
        return module->module.SymType;
    }

    if (strstr(filename, "libstdc++")) return -1; /* We know we can't do it */
    sym_type = elf_load_file(pcs, filename, load_offset, elf_info);
    /* if relative pathname, try some absolute base dirs */
    if (sym_type == -1 && !strchr(filename, '/'))
    {
        sym_type = elf_load_file_from_path(pcs, filename, load_offset, 
                                           getenv("PATH"), elf_info);
        if (sym_type == -1)
            sym_type = elf_load_file_from_path(pcs, filename, load_offset,
                                               getenv("LD_LIBRARY_PATH"), elf_info);
        if (sym_type == -1)
            sym_type = elf_load_file_from_path(pcs, filename, load_offset,
                                               getenv("WINEDLLPATH"), elf_info);
    }
    
    return sym_type;
}

/******************************************************************
 *		elf_synchronize_module_list
 *
 * this functions rescans the debuggee module's list and synchronizes it with
 * the one from 'pcs', ie:
 * - if a module is in debuggee and not in pcs, it's loaded into pcs
 * - if a module is in pcs and not in debuggee, it's unloaded from pcs
 */
BOOL	elf_synchronize_module_list(struct process* pcs)
{
    struct r_debug      dbg_hdr;
    void*               lm_addr;
    struct link_map     lm;
    char		bufstr[256];
    struct elf_info     elf_info;
    struct module*      module;

    if (!pcs->dbg_hdr_addr) return FALSE;
    if (!read_mem(pcs->handle, pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)))
        return FALSE;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_ELF) module->elf_info->elf_mark = 0;
    }

    elf_info.flags = ELF_INFO_MODULE;
    /* Now walk the linked list.  In all known ELF implementations,
     * the dynamic loader maintains this linked list for us.  In some
     * cases the first entry doesn't appear with a name, in other cases it
     * does.
     */
    for (lm_addr = (void*)dbg_hdr.r_map; lm_addr; lm_addr = (void*)lm.l_next)
    {
	if (!read_mem(pcs->handle, (ULONG)lm_addr, &lm, sizeof(lm)))
	    return FALSE;

	if (lm.l_prev != NULL && /* skip first entry, normally debuggee itself */
	    lm.l_name != NULL &&
	    read_mem(pcs->handle, (ULONG)lm.l_name, bufstr, sizeof(bufstr))) 
        {
	    bufstr[sizeof(bufstr) - 1] = '\0';
            elf_search_and_load_file(pcs, bufstr, (unsigned long)lm.l_addr,
                                     &elf_info);
	}
    }

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_ELF && !module->elf_info->elf_mark && 
            !module->elf_info->elf_loader)
        {
            module_remove(pcs, module);
            /* restart all over */
            module = pcs->lmodules;
        }
    }
    return TRUE;
}

/******************************************************************
 *		elf_read_wine_loader_dbg_info
 *
 * Try to find a decent wine executable which could have loaded the debuggee
 */
BOOL elf_read_wine_loader_dbg_info(struct process* pcs)
{
    const char*         ptr;
    SYM_TYPE            sym_type;
    struct elf_info     elf_info;

    elf_info.flags = ELF_INFO_DEBUG_HEADER | ELF_INFO_MODULE;
    /* All binaries are loaded with WINELOADER (if run from tree) or by the
     * main executable (either wine-kthread or wine-pthread)
     * Note: the heuristic use to know whether we need to load wine-pthread or
     * wine-kthread is not 100% safe
     */
    if ((ptr = getenv("WINELOADER")))
        sym_type = elf_search_and_load_file(pcs, ptr, 0, &elf_info);
    else 
    {
        if ((sym_type = elf_search_and_load_file(pcs, "wine-kthread", 0, &elf_info)) == -1)
            sym_type = elf_search_and_load_file(pcs, "wine-pthread", 0, &elf_info);
    }
    if (sym_type < 0) return FALSE;
    elf_info.module->elf_info->elf_loader = 1;
    strcpy(elf_info.module->module.ModuleName, "<wine-loader>");
    return (pcs->dbg_hdr_addr = elf_info.dbg_hdr_addr) != 0;
}

/******************************************************************
 *		elf_load_module
 *
 * loads an ELF module and stores it in process' module list
 * if 'sync' is TRUE, let's find module real name and load address from
 * the real loaded modules list in pcs address space
 */
struct module*  elf_load_module(struct process* pcs, const char* name)
{
    struct elf_info     elf_info;
    SYM_TYPE            sym_type = -1;
    const char*         p;
    const char*         xname;
    struct r_debug      dbg_hdr;
    void*               lm_addr;
    struct link_map     lm;
    char                bufstr[256];

    TRACE("(%p %s)\n", pcs, name);

    elf_info.flags = ELF_INFO_MODULE;

    /* do only the lookup from the filename, not the path (as we lookup module name
     * in the process' loaded module list)
     */
    xname = strrchr(name, '/');
    if (!xname++) xname = name;

    if (!read_mem(pcs->handle, pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)))
        return NULL;

    for (lm_addr = (void*)dbg_hdr.r_map; lm_addr; lm_addr = (void*)lm.l_next)
    {
        if (!read_mem(pcs->handle, (ULONG)lm_addr, &lm, sizeof(lm)))
            return NULL;

        if (lm.l_prev != NULL && /* skip first entry, normally debuggee itself */
            lm.l_name != NULL &&
            read_mem(pcs->handle, (ULONG)lm.l_name, bufstr, sizeof(bufstr))) 
        {
            bufstr[sizeof(bufstr) - 1] = '\0';
            /* memcmp is needed for matches when bufstr contains also version information
             * name: libc.so, bufstr: libc.so.6.0
             */
            p = strrchr(bufstr, '/');
            if (!p++) p = bufstr;
            if (!memcmp(p, xname, strlen(xname)))
            {
                sym_type = elf_search_and_load_file(pcs, bufstr,
                                         (unsigned long)lm.l_addr, &elf_info);
                break;
            }
        }
    }
    if (!lm_addr || sym_type == -1) return NULL;
    assert(elf_info.module);
    return elf_info.module;
}

#else	/* !__ELF__ */

BOOL	elf_synchronize_module_list(struct process* pcs)
{
    return FALSE;
}

BOOL elf_read_wine_loader_dbg_info(struct process* pcs)
{
    return FALSE;
}

struct module*  elf_load_module(struct process* pcs, const char* name)
{
    return NULL;
}

SYM_TYPE elf_load_debug_info(struct module* module)
{
    return -1;
}
#endif  /* __ELF__ */
