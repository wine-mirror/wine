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

typedef struct tagELF_DBG_INFO
{
    unsigned long       elf_addr;
} ELF_DBG_INFO;

#ifdef __ELF__

#define ELF_INFO_DEBUG_HEADER   0x0001
#define ELF_INFO_MODULE         0x0002

struct elf_info
{
    unsigned            flags;          /* IN  one (or several) of the ELF_INFO constants */
    unsigned long       dbg_hdr_addr;   /* OUT address of debug header (if ELF_INFO_DEBUG_HEADER is set) */
    struct module*      module;         /* OUT loaded module (if ELF_INFO_MODULE is set) */
};

/******************************************************************
 *		elf_load_symtab
 *
 * Walk through the entire symbol table and add any symbols we find there.
 * This can be used in cases where we have stripped ELF shared libraries,
 * or it can be used in cases where we have data symbols for which the address
 * isn't encoded in the stabs.
 *
 * This is all really quite easy, since we don't have to worry about line
 * numbers or local data variables.
 */
static int elf_load_symtab(struct module* module, const char* addr,
                           unsigned long load_addr, const Elf32_Shdr* symtab,
                           const Elf32_Shdr* strtab)
{
    int		                i, nsym;
    const char*                 strp;
    const char*                 symname;
    const Elf32_Sym*            symp;
    struct symt_compiland*      compiland = NULL;

    symp = (Elf32_Sym*)(addr + symtab->sh_offset);
    nsym = symtab->sh_size / sizeof(*symp);
    strp = (char*)(addr + strtab->sh_offset);

    for (i = 0; i < nsym; i++, symp++)
    {
        /* Ignore certain types of entries which really aren't of that much
         * interest.
         */
        if (ELF32_ST_TYPE(symp->st_info) == STT_SECTION || 
            symp->st_shndx == SHN_UNDEF)
        {
            continue;
        }

        symname = strp + symp->st_name;

        if (ELF32_ST_TYPE(symp->st_info) == STT_FILE)
        {
            compiland = symt_new_compiland(module, symname);
            continue;
        }
        symt_new_public(module, compiland, symname,
                        load_addr + symp->st_value, symp->st_size,
                        TRUE /* FIXME */, ELF32_ST_TYPE(symp->st_info) == STT_FUNC);
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
    int		        stabsect, stabstrsect, debugsect;
    int                 symsect, dynsect;

    if (module->type != DMT_ELF || !module->elf_dbg_info)
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

    symsect = dynsect = stabsect = stabstrsect = debugsect = -1;

    for (i = 0; i < ehptr->e_shnum; i++)
    {
	if (strcmp(shstrtab + spnt[i].sh_name, ".stab") == 0)
	    stabsect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".stabstr") == 0)
	    stabstrsect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".debug_info") == 0)
	    debugsect = i;
	if ((strcmp(shstrtab + spnt[i].sh_name, ".symtab") == 0) &&
	    (spnt[i].sh_type == SHT_SYMTAB))
            symsect = i;
	if ((strcmp(shstrtab + spnt[i].sh_name, ".dynsym") == 0) &&
	    (spnt[i].sh_type == SHT_DYNSYM))
            dynsect = i;
    }
    /* start loading dynamic symbol info (so that we can get the correct address) */
    if (symsect != -1)
        elf_load_symtab(module, addr, module->elf_dbg_info->elf_addr,
                        spnt + symsect, spnt + spnt[symsect].sh_link);
    else if (dynsect != -1)
        elf_load_symtab(module, addr, module->elf_dbg_info->elf_addr,
                        spnt + dynsect, spnt + spnt[dynsect].sh_link);
    sym_type = SymExport;

    if (!(dbghelp_options & SYMOPT_PUBLICS_ONLY))
    {
        if (stabsect != -1 && stabstrsect != -1)
        {
            /* OK, now just parse all of the stabs. */
            sym_type = stabs_parse(module, addr, module->elf_dbg_info->elf_addr,
                                   spnt[stabsect].sh_offset, spnt[stabsect].sh_size,
                                   spnt[stabstrsect].sh_offset,
                                   spnt[stabstrsect].sh_size);
            if (sym_type == -1)
            {
                WARN("Couldn't read correctly read stabs\n");
                goto leave;
            }
        }
        else if (debugsect != -1)
        {
            /* Dwarf 2 debug information */
            FIXME("Unsupported Dwarf2 information\n");
            sym_type = SymNone;
        }
    }

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
    ehptr = (Elf32_Ehdr*)addr;
    if (memcmp(ehptr->e_ident, elf_signature, sizeof(elf_signature))) goto leave;

    spnt = (Elf32_Shdr*)(addr + ehptr->e_shoff);
    shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

    /* if non relocatable ELF, then remove fixed address from computation
     * otherwise, all addresses are zero based
     */
    delta = (load_offset == 0) ? ehptr->e_entry : 0;

    /* grab size of module once loaded in memory */
    ppnt = (Elf32_Phdr*)(addr + ehptr->e_phoff);
    size = 0;
    for (i = 0; i < ehptr->e_phnum; i++)
    {
	if (ppnt[i].p_type == PT_LOAD)
        {
            size += (ppnt[i].p_align <= 1) ? ppnt[i].p_memsz :
                (ppnt[i].p_memsz + ppnt[i].p_align - 1) & ~(ppnt[i].p_align - 1);
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
            if ((elf_info->module->elf_dbg_info = HeapAlloc(GetProcessHeap(), 0, sizeof(ELF_DBG_INFO))) == NULL) 
            {
                ERR("OOM\n");
                exit(0); /* FIXME */
            }
            elf_info->module->elf_dbg_info->elf_addr = load_offset;
            elf_info->module->module.SymType = sym_type = 
                (dbghelp_options & SYMOPT_DEFERRED_LOADS) ? SymDeferred : 
                elf_load_debug_info(elf_info->module);
            elf_info->module->elf_mark = 1;
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
        module->elf_mark = 1;
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
    if (!read_mem(pcs->handle, pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)) ||
        dbg_hdr.r_state != RT_CONSISTENT)
        return FALSE;

    for (module = pcs->lmodules; module; module = module->next)
    {
        if (module->type == DMT_ELF) module->elf_mark = 0;
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
        if (module->type == DMT_ELF && !module->elf_mark)
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
 * Try to find a decent wine executable which could have loader the debuggee
 */
unsigned long        elf_read_wine_loader_dbg_info(struct process* pcs)
{
    const char*         ptr;
    SYM_TYPE            sym_type;
    struct elf_info     elf_info;

    elf_info.flags = ELF_INFO_DEBUG_HEADER;
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
    return (sym_type < 0) ? 0 : elf_info.dbg_hdr_addr;
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

    if (!read_mem(pcs->handle, pcs->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)) ||
        dbg_hdr.r_state != RT_CONSISTENT)
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

unsigned long elf_read_wine_loader_dbg_info(struct process* pcs)
{
    return -1;
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
