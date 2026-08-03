/*
 * Initialization code for .so modules
 *
 * Copyright (C) 2020 Alexandre Julliard
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

#if 0
#pragma makedep unix
#endif

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dlfcn.h>
#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LINK_H
# include <link.h>
#endif
#include "windef.h"


#ifdef __FreeBSD__
/* The PT_LOAD segments are sorted in increasing order, and the first
 * starts at the beginning of the ELF file. By parsing the file, we can
 * find that first PT_LOAD segment, from which we can find the base
 * address it wanted, and knowing mapbase where the binary was actually
 * loaded, use them to work out the relocbase offset. */
static BOOL get_relocbase(caddr_t mapbase, caddr_t *relocbase)
{
    Elf_Half i;
#ifdef _WIN64
    const Elf64_Ehdr *elf_header = (Elf64_Ehdr*) mapbase;
#else
    const Elf32_Ehdr *elf_header = (Elf32_Ehdr*) mapbase;
#endif
    const Elf_Phdr *prog_header = (const Elf_Phdr *)(mapbase + elf_header->e_phoff);

    for (i = 0; i < elf_header->e_phnum; i++)
    {
         if (prog_header->p_type == PT_LOAD)
         {
             caddr_t desired_base = (caddr_t)((prog_header->p_vaddr / prog_header->p_align) * prog_header->p_align);
             *relocbase = (caddr_t) (mapbase - desired_base);
             return TRUE;
         }
         prog_header++;
    }
    return FALSE;
}
#endif


/*************************************************************************
 *              __wine_init_so_dll
 */
void __wine_init_so_dll(void)
{
#if defined(HAVE_DLADDR1) || (defined(HAVE_DLINFO) && defined(RTLD_SELF))
    struct link_map *map;
    void (*init_func)(int, char **, char **) = NULL;
    void (**init_array)(int, char **, char **) = NULL;
    long i, init_arraysz = 0;
#ifdef _WIN64
    const Elf64_Dyn *dyn;
#else
    const Elf32_Dyn *dyn;
#endif

#ifdef HAVE_DLADDR1
    Dl_info info;
    if (!dladdr1( __wine_init_so_dll, &info, (void **)&map, RTLD_DL_LINKMAP )) return;
#else
    if (dlinfo( RTLD_SELF, RTLD_DI_LINKMAP, &map )) return;
#endif

    for (dyn = map->l_ld; dyn->d_tag; dyn++)
    {
        caddr_t relocbase = (caddr_t)map->l_addr;

#ifdef __FreeBSD__
        /* On older FreeBSD versions, l_addr was the absolute load address, now it's the relocation offset. */
        if (offsetof(struct link_map, l_addr) == 0)
            if (!get_relocbase(map->l_addr, &relocbase))
                return;
#endif
        switch (dyn->d_tag)
        {
        case 0x60009994: init_array = (void *)(relocbase + dyn->d_un.d_val); break;
        case 0x60009995: init_arraysz = dyn->d_un.d_val; break;
        case 0x60009996: init_func = (void *)(relocbase + dyn->d_un.d_val); break;
        }
    }

    if (init_func) init_func( 0, NULL, NULL );

    if (init_array)
        for (i = 0; i < init_arraysz / sizeof(*init_array); i++)
            init_array[i]( 0, NULL, NULL );
#endif
}
