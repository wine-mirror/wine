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

#include "debugger.h"

#if defined(__svr4__) || defined(__sun)
#define __ELF__
#endif

#ifdef __ELF__
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LINK_H
# include <link.h>
#endif
#ifdef HAVE_SYS_LINK_H
# include <sys/link.h>
#endif
#endif

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(winedbg);

typedef struct tagELF_DBG_INFO
{
    void *elf_addr;
} ELF_DBG_INFO;

#ifdef __ELF__

/*
 * Walk through the entire symbol table and add any symbols we find there.
 * This can be used in cases where we have stripped ELF shared libraries,
 * or it can be used in cases where we have data symbols for which the address
 * isn't encoded in the stabs.
 *
 * This is all really quite easy, since we don't have to worry about line
 * numbers or local data variables.
 */
static int DEBUG_ProcessElfSymtab(DBG_MODULE* module, const char* addr,
				  void *load_addr, const Elf32_Shdr* symtab,
				  const Elf32_Shdr* strtab)
{
    const char*         curfile = NULL;
    struct name_hash*   curr_sym = NULL;
    int		        flags;
    int		        i;
    DBG_VALUE           new_value;
    int		        nsym;
    const char*         strp;
    const char*         symname;
    const Elf32_Sym*    symp;

    symp = (Elf32_Sym *)(addr + symtab->sh_offset);
    nsym = symtab->sh_size / sizeof(*symp);
    strp = (char *)(addr + strtab->sh_offset);

    for (i = 0; i < nsym; i++, symp++)
    {
        /*
         * Ignore certain types of entries which really aren't of that much
         * interest.
         */
        if (ELF32_ST_TYPE(symp->st_info) == STT_SECTION || 
            symp->st_shndx == STN_UNDEF)
        {
            continue;
        }

        symname = strp + symp->st_name;

        /*
         * Save the name of the current file, so we have a way of tracking
         * static functions/data.
         */
        if (ELF32_ST_TYPE(symp->st_info) == STT_FILE)
        {
            curfile = symname;
            continue;
        }

        new_value.type = NULL;
        new_value.addr.seg = 0;
        new_value.addr.off = (unsigned long)load_addr + symp->st_value;
        new_value.cookie = DV_TARGET;
        flags = SYM_WINE | ((ELF32_ST_TYPE(symp->st_info) == STT_FUNC)
                            ? SYM_FUNC : SYM_DATA);
        if (ELF32_ST_BIND(symp->st_info) == STB_GLOBAL)
            curr_sym = DEBUG_AddSymbol(symname, &new_value, NULL, flags);
        else
            curr_sym = DEBUG_AddSymbol(symname, &new_value, curfile, flags);

        /*
         * Record the size of the symbol.  This can come in handy in
         * some cases.  Not really used yet, however.
         */
        if (symp->st_size != 0)
            DEBUG_SetSymbolSize(curr_sym, symp->st_size);
    }

    return TRUE;
}

/*
 * Loads the symbolic information from ELF module stored in 'filename'
 * the module has been loaded at 'load_offset' address, so symbols' address
 * relocation is performed
 * returns
 *	-1 if the file cannot be found/opened
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
enum DbgInfoLoad DEBUG_LoadElfStabs(DBG_MODULE* module)
{
    enum DbgInfoLoad    dil = DIL_ERROR;
    char*	        addr = (char*)0xffffffff;
    int		        fd = -1;
    struct stat	        statbuf;
    const Elf32_Ehdr*   ehptr;
    const Elf32_Shdr*   spnt;
    const char*	        shstrtab;
    int	       	        i;
    int		        stabsect, stabstrsect, debugsect;
    
    if (module->type != DMT_ELF || !module->elf_dbg_info)
    {
	WINE_ERR("Bad elf module '%s'\n", module->module_name);
	return DIL_ERROR;
    }

    /* check that the file exists, and that the module hasn't been loaded yet */
    if (stat(module->module_name, &statbuf) == -1) goto leave;
    if (S_ISDIR(statbuf.st_mode)) goto leave;

    /*
     * Now open the file, so that we can mmap() it.
     */
    if ((fd = open(module->module_name, O_RDONLY)) == -1) goto leave;

    dil = DIL_NOINFO;
    /*
     * Now mmap() the file.
     */
    addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == (char*)0xffffffff) goto leave;

    /*
     * Next, we need to find a few of the internal ELF headers within
     * this thing.  We need the main executable header, and the section
     * table.
     */
    ehptr = (Elf32_Ehdr*)addr;
    spnt = (Elf32_Shdr*)(addr + ehptr->e_shoff);
    shstrtab = (addr + spnt[ehptr->e_shstrndx].sh_offset);

    stabsect = stabstrsect = debugsect = -1;

    for (i = 0; i < ehptr->e_shnum; i++)
    {
	if (strcmp(shstrtab + spnt[i].sh_name, ".stab") == 0)
	    stabsect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".stabstr") == 0)
	    stabstrsect = i;
	if (strcmp(shstrtab + spnt[i].sh_name, ".debug_info") == 0)
	    debugsect = i;
    }

    if (stabsect != -1 && stabstrsect != -1)
    {
        /*
         * OK, now just parse all of the stabs.
         */
        if (DEBUG_ParseStabs(addr,
                             module->elf_dbg_info->elf_addr,
                             spnt[stabsect].sh_offset,
                             spnt[stabsect].sh_size,
                             spnt[stabstrsect].sh_offset,
                             spnt[stabstrsect].sh_size))
        {
            dil = DIL_LOADED;
        }
        else
        {
            dil = DIL_ERROR;
            WINE_WARN("Couldn't read correctly read stabs\n");
            goto leave;
        }
    }
    else if (debugsect != -1)
    {
        /* Dwarf 2 debug information */
        dil = DIL_NOT_SUPPORTED;
    }
    /* now load dynamic symbol info */
    for (i = 0; i < ehptr->e_shnum; i++)
    {
	if ((strcmp(shstrtab + spnt[i].sh_name, ".symtab") == 0) &&
	    (spnt[i].sh_type == SHT_SYMTAB))
	    DEBUG_ProcessElfSymtab(module, addr, module->elf_dbg_info->elf_addr,
				   spnt + i, spnt + spnt[i].sh_link);

	if ((strcmp(shstrtab + spnt[i].sh_name, ".dynsym") == 0) &&
	    (spnt[i].sh_type == SHT_DYNSYM))
	    DEBUG_ProcessElfSymtab(module, addr, module->elf_dbg_info->elf_addr,
				   spnt + i, spnt + spnt[i].sh_link);
    }

leave:
    if (addr != (char*)0xffffffff) munmap(addr, statbuf.st_size);
    if (fd != -1) close(fd);

    return dil;
}

/*
 * Loads the information for ELF module stored in 'filename'
 * the module has been loaded at 'load_offset' address
 * returns
 *	-1 if the file cannot be found/opened
 *	0 if the file doesn't contain symbolic info (or this info cannot be
 *	read or parsed)
 *	1 on success
 */
static enum DbgInfoLoad DEBUG_ProcessElfFile(HANDLE hProcess,
                                             const char* filename,
					     void *load_offset,
					     struct elf_info* elf_info)
{
    static const unsigned char elf_signature[4] = { ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3 };
    enum DbgInfoLoad    dil = DIL_ERROR;
    const char*	        addr = (char*)0xffffffff;
    int		        fd = -1;
    struct stat	        statbuf;
    const Elf32_Ehdr*   ehptr;
    const Elf32_Shdr*   spnt;
    const Elf32_Phdr*	ppnt;
    const char*         shstrtab;
    int	       	        i;
    DWORD	        delta;

    WINE_TRACE("Processing elf file '%s' at %p\n", filename, load_offset);

    /* check that the file exists, and that the module hasn't been loaded yet */
    if (stat(filename, &statbuf) == -1) goto leave;

    /*
     * Now open the file, so that we can mmap() it.
     */
    if ((fd = open(filename, O_RDONLY)) == -1) goto leave;

    /*
     * Now mmap() the file.
     */
    addr = mmap(0, statbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (addr == (char*)-1) goto leave;

    dil = DIL_NOINFO;

    /*
     * Next, we need to find a few of the internal ELF headers within
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
    elf_info->size = 0;
    for (i = 0; i < ehptr->e_phnum; i++)
    {
	if (ppnt[i].p_type != PT_LOAD) continue;
	if (elf_info->size < ppnt[i].p_vaddr - delta + ppnt[i].p_memsz)
	    elf_info->size = ppnt[i].p_vaddr - delta + ppnt[i].p_memsz;
    }

    for (i = 0; i < ehptr->e_shnum; i++)
    {
	if (strcmp(shstrtab + spnt[i].sh_name, ".bss") == 0 &&
	    spnt[i].sh_type == SHT_NOBITS)
        {
	    if (elf_info->size < spnt[i].sh_addr - delta + spnt[i].sh_size)
		elf_info->size = spnt[i].sh_addr - delta + spnt[i].sh_size;
	}
	if (strcmp(shstrtab + spnt[i].sh_name, ".dynamic") == 0 &&
	    spnt[i].sh_type == SHT_DYNAMIC)
        {
	    if (elf_info->flags & ELF_INFO_DEBUG_HEADER)
            {
                Elf32_Dyn       dyn;
                char*           ptr = (char*)spnt[i].sh_addr;
                unsigned long   len;

                do
                {
                    if (!ReadProcessMemory(hProcess, ptr, &dyn, sizeof(dyn), &len) ||
                        len != sizeof(dyn) ||
                        !((dyn.d_tag >= 0 && 
                           dyn.d_tag < DT_NUM+DT_PROCNUM+DT_EXTRANUM) ||
                          (dyn.d_tag >= DT_LOOS && dyn.d_tag < DT_HIOS) ||
                          (dyn.d_tag >= DT_LOPROC && dyn.d_tag < DT_HIPROC))
                        )
                        dyn.d_tag = DT_NULL;
                    ptr += sizeof(dyn);
                } while (dyn.d_tag != DT_DEBUG && dyn.d_tag != DT_NULL);
                if (dyn.d_tag == DT_NULL)
                {
                    dil = DIL_ERROR;
                    goto leave;
                }
                elf_info->dbg_hdr_addr = dyn.d_un.d_ptr;
            }
	}
    }

    elf_info->segments[0] = elf_info->segments[1] = elf_info->segments[2] = 0;
    if (elf_info->flags & ELF_INFO_PATH)
    {
        strncpy(elf_info->elf_path, filename, elf_info->elf_path_len);
        elf_info->elf_path[elf_info->elf_path_len - 1] = '\0';
    }

    elf_info->load_addr = (load_offset == 0) ? (void *)ehptr->e_entry : load_offset;

    if (elf_info->flags & ELF_INFO_MODULE)
    {
        DBG_MODULE* module;

        module = DEBUG_AddModule(filename, DMT_ELF, elf_info->load_addr, elf_info->size, 0);
        if (module)
        {
            if ((module->elf_dbg_info = DBG_alloc(sizeof(ELF_DBG_INFO))) == NULL) 
            {
                WINE_ERR("OOM\n");
                exit(0);
            }
            module->elf_dbg_info->elf_addr = load_offset;
            module->dil = dil = DEBUG_LoadElfStabs(module);
        }
        else dil = DIL_ERROR;
    }

leave:
    if (addr != (char*)0xffffffff) munmap((void*)addr, statbuf.st_size);
    if (fd != -1) close(fd);

    return dil;
}

static enum DbgInfoLoad DEBUG_ProcessElfFileFromPath(HANDLE hProcess,
                                                     const char * filename,
						     void *load_offset,
						     const char* path,
                                                     struct elf_info* elf_info)
{
    enum DbgInfoLoad	dil = DIL_ERROR;
    char                *s, *t, *fn;
    char*	        paths = NULL;

    if (!path) return -1;

    for (s = paths = DBG_strdup(path); s && *s; s = (t) ? (t+1) : NULL) 
    {
	t = strchr(s, ':');
	if (t) *t = '\0';
	fn = (char*)DBG_alloc(strlen(filename) + 1 + strlen(s) + 1);
	if (!fn) break;
	strcpy(fn, s );
	strcat(fn, "/");
	strcat(fn, filename);
	dil = DEBUG_ProcessElfFile(hProcess, fn, load_offset, elf_info);
	DBG_free(fn);
	if (dil != DIL_ERROR) break;
	s = (t) ? (t+1) : NULL;
    }

    DBG_free(paths);
    return dil;
}

static enum DbgInfoLoad DEBUG_ProcessElfObject(HANDLE hProcess,
                                               const char* filename,
                                               void *load_offset,
					       struct elf_info* elf_info)
{
   enum DbgInfoLoad	dil = DIL_ERROR;

   if (filename == NULL) return DIL_ERROR;
   if (DEBUG_FindModuleByName(filename, DMT_ELF))
   {
       assert(!(elf_info->flags & ELF_INFO_PATH));
       return DIL_LOADED;
   }

   if (strstr(filename, "libstdc++")) return DIL_ERROR; /* We know we can't do it */
   dil = DEBUG_ProcessElfFile(hProcess, filename, load_offset, elf_info);
   /* if relative pathname, try some absolute base dirs */
   if (dil == DIL_ERROR && !strchr(filename, '/'))
   {
       dil = DEBUG_ProcessElfFileFromPath(hProcess, filename, load_offset, 
                                          getenv("PATH"), elf_info);
       if (dil == DIL_ERROR)
           dil = DEBUG_ProcessElfFileFromPath(hProcess, filename, load_offset,
                                              getenv("LD_LIBRARY_PATH"), elf_info);
       if (dil == DIL_ERROR)
           dil = DEBUG_ProcessElfFileFromPath(hProcess, filename, load_offset,
                                              getenv("WINEDLLPATH"), elf_info);
   }

   DEBUG_ReportDIL(dil, "ELF", filename, load_offset);

   return dil;
}

static	BOOL	DEBUG_WalkList(const struct r_debug* dbg_hdr)
{
    void*               lm_addr;
    struct link_map     lm;
    char		bufstr[256];
    struct elf_info     elf_info;

    elf_info.flags = ELF_INFO_MODULE;
    /*
     * Now walk the linked list.  In all known ELF implementations,
     * the dynamic loader maintains this linked list for us.  In some
     * cases the first entry doesn't appear with a name, in other cases it
     * does.
     */
    for (lm_addr = (void *)dbg_hdr->r_map; lm_addr; lm_addr = (void *)lm.l_next)
    {
	if (!DEBUG_READ_MEM_VERBOSE(lm_addr, &lm, sizeof(lm)))
	    return FALSE;

	if (lm.l_prev != NULL && /* skip first entry, normally debuggee itself */
	    lm.l_name != NULL &&
	    DEBUG_READ_MEM_VERBOSE((void*)lm.l_name, bufstr, sizeof(bufstr))) 
        {
	    bufstr[sizeof(bufstr) - 1] = '\0';
	    DEBUG_ProcessElfObject(DEBUG_CurrProcess->handle, bufstr, 
                                   (void *)lm.l_addr, &elf_info);
	}
    }

    return TRUE;
}

static BOOL DEBUG_RescanElf(void)
{
    struct r_debug        dbg_hdr;

    if (DEBUG_CurrProcess &&
	DEBUG_READ_MEM_VERBOSE((void*)DEBUG_CurrProcess->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)))
    {
        switch (dbg_hdr.r_state) 
        {
        case RT_CONSISTENT:
            DEBUG_WalkList(&dbg_hdr);
            DEBUG_CheckDelayedBP();
            break;
        case RT_ADD:
            break;
        case RT_DELETE:
            /* FIXME: this is not currently handled */
            break;
        }
    }
    return FALSE;
}

/******************************************************************
 *		DEBUG_ReadWineLoaderDbgInfo
 *
 * Try to find a decent wine executable which could have loader the debuggee
 */
enum DbgInfoLoad	DEBUG_ReadWineLoaderDbgInfo(HANDLE hProcess, struct elf_info* elf_info)
{
    const char*         ptr;
    enum DbgInfoLoad    dil;

    /* All binaries are loaded with WINELOADER (if run from tree) or by the
     * main executable (either wine-kthread or wine-pthread)
     * Note: the heuristic use to know wether we need to load wine-pthread or 
     * wine-kthread is not 100% safe
     */
    elf_info->flags |= ELF_INFO_DEBUG_HEADER;
    if ((ptr = getenv("WINELOADER")))
        dil = DEBUG_ProcessElfObject(hProcess, ptr, 0, elf_info);
    else 
    {
        if ((dil = DEBUG_ProcessElfObject(hProcess, "wine-kthread", 0, elf_info)) == DIL_ERROR)
            dil = DEBUG_ProcessElfObject(hProcess, "wine-pthread", 0, elf_info);
    }
    return dil;
}

/******************************************************************
 *		DEBUG_SetElfSoLoadBreakpoint
 *
 * Sets a breakpoint to handle .so loading events, so we can add debug info
 * on the fly
 */
BOOL    DEBUG_SetElfSoLoadBreakpoint(const struct elf_info* elf_info)
{
    struct r_debug      dbg_hdr;
    
    /*
     * OK, now dig into the actual tables themselves.
     */
    if (!DEBUG_READ_MEM_VERBOSE((void*)elf_info->dbg_hdr_addr, &dbg_hdr, sizeof(dbg_hdr)))
        return FALSE;

    assert(!DEBUG_CurrProcess->dbg_hdr_addr);
    DEBUG_CurrProcess->dbg_hdr_addr = elf_info->dbg_hdr_addr;

    if (dbg_hdr.r_brk)
    {
        DBG_VALUE	value;

        WINE_TRACE("Setting up a breakpoint on r_brk(%lx)\n", 
                   (unsigned long)dbg_hdr.r_brk);

        DEBUG_SetBreakpoints(FALSE);
        value.type = NULL;
        value.cookie = DV_TARGET;
        value.addr.seg = 0;
        value.addr.off = (DWORD)dbg_hdr.r_brk;
        DEBUG_AddBreakpoint(&value, DEBUG_RescanElf, TRUE);
        DEBUG_SetBreakpoints(TRUE);
    }
    
    return DEBUG_WalkList(&dbg_hdr);
}

#else	/* !__ELF__ */

enum DbgInfoLoad DEBUG_ReadWineLoaderDbgInfo(HANDLE hProcess, struct elf_info* elf_info)
{
    return DIL_ERROR;
}

BOOL    DEBUG_SetElfSoLoadBreakpoint(const struct elf_info* elf_info)
{
    return FALSE;
}

#endif  /* __ELF__ */
