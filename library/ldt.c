/*
 * LDT manipulation functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
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
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "winbase.h"
#define WINE_EXPORT_LDT_COPY
#include "wine/library.h"

#ifdef __i386__

#ifdef linux

#ifdef HAVE_SYS_SYSCALL_H
# include <sys/syscall.h>
#endif

struct modify_ldt_s
{
    unsigned int  entry_number;
    unsigned long base_addr;
    unsigned int  limit;
    unsigned int  seg_32bit : 1;
    unsigned int  contents : 2;
    unsigned int  read_exec_only : 1;
    unsigned int  limit_in_pages : 1;
    unsigned int  seg_not_present : 1;
    unsigned int  useable:1;
};

static inline int modify_ldt( int func, struct modify_ldt_s *ptr,
                                  unsigned long count )
{
    int res;
#ifdef __PIC__
    __asm__ __volatile__( "pushl %%ebx\n\t"
                          "movl %2,%%ebx\n\t"
                          "int $0x80\n\t"
                          "popl %%ebx"
                          : "=a" (res)
                          : "0" (SYS_modify_ldt),
                            "r" (func),
                            "c" (ptr),
                            "d" (count) );
#else
    __asm__ __volatile__("int $0x80"
                         : "=a" (res)
                         : "0" (SYS_modify_ldt),
                           "b" (func),
                           "c" (ptr),
                           "d" (count) );
#endif  /* __PIC__ */
    if (res >= 0) return res;
    errno = -res;
    return -1;
}

#endif  /* linux */

#if defined(__svr4__) || defined(_SCO_DS)
#include <sys/sysi86.h>
extern int sysi86(int,void*);
#ifndef __sun__
#include <sys/seg.h>
#endif
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <machine/segments.h>

extern int i386_get_ldt(int, union descriptor *, int);
extern int i386_set_ldt(int, union descriptor *, int);
#endif  /* __NetBSD__ || __FreeBSD__ || __OpenBSD__ */

#endif  /* __i386__ */

/* local copy of the LDT */
struct __wine_ldt_copy wine_ldt_copy;


/***********************************************************************
 *           ldt_get_entry
 *
 * Retrieve an LDT entry.
 */
void wine_ldt_get_entry( unsigned short sel, LDT_ENTRY *entry )
{
    int index = sel >> 3;
    wine_ldt_set_base(  entry, wine_ldt_copy.base[index] );
    wine_ldt_set_limit( entry, wine_ldt_copy.limit[index] );
    wine_ldt_set_flags( entry, wine_ldt_copy.flags[index] );
}


/***********************************************************************
 *           ldt_set_entry
 *
 * Set an LDT entry.
 */
int wine_ldt_set_entry( unsigned short sel, const LDT_ENTRY *entry )
{
    int ret = 0, index = sel >> 3;

    /* Entry 0 must not be modified; its base and limit are always 0 */
    if (!index) return 0;

#ifdef __i386__

#ifdef linux
    {
        struct modify_ldt_s ldt_info;

        ldt_info.entry_number    = index;
        ldt_info.base_addr       = (unsigned long)wine_ldt_get_base(entry);
        ldt_info.limit           = entry->LimitLow | (entry->HighWord.Bits.LimitHi << 16);
        ldt_info.seg_32bit       = entry->HighWord.Bits.Default_Big;
        ldt_info.contents        = (entry->HighWord.Bits.Type >> 2) & 3;
        ldt_info.read_exec_only  = !(entry->HighWord.Bits.Type & 2);
        ldt_info.limit_in_pages  = entry->HighWord.Bits.Granularity;
        ldt_info.seg_not_present = !entry->HighWord.Bits.Pres;
        ldt_info.useable         = entry->HighWord.Bits.Sys;

        if ((ret = modify_ldt(0x11, &ldt_info, sizeof(ldt_info))) < 0)
            perror( "modify_ldt" );
    }
#endif  /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    {
	LDT_ENTRY entry_copy = *entry;
	/* The kernel will only let us set LDTs with user priority level */
	if (entry_copy.HighWord.Bits.Pres
	    && entry_copy.HighWord.Bits.Dpl != 3)
		entry_copy.HighWord.Bits.Dpl = 3;
        ret = i386_set_ldt(index, (union descriptor *)&entry_copy, 1);
        if (ret < 0)
        {
            perror("i386_set_ldt");
            fprintf( stderr, "Did you reconfigure the kernel with \"options USER_LDT\"?\n" );
            exit(1);
        }
    }
#endif  /* __NetBSD__ || __FreeBSD__ || __OpenBSD__ */

#if defined(__svr4__) || defined(_SCO_DS)
    {
        struct ssd ldt_mod;
        ldt_mod.sel  = sel;
        ldt_mod.bo   = (unsigned long)wine_ldt_get_base(entry);
        ldt_mod.ls   = entry->LimitLow | (entry->HighWord.Bits.LimitHi << 16);
        ldt_mod.acc1 = entry->HighWord.Bytes.Flags1;
        ldt_mod.acc2 = entry->HighWord.Bytes.Flags2 >> 4;
        if ((ret = sysi86(SI86DSCR, &ldt_mod)) == -1) perror("sysi86");
    }
#endif

#endif  /* __i386__ */

    if (ret >= 0)
    {
        wine_ldt_copy.base[index]  = wine_ldt_get_base(entry);
        wine_ldt_copy.limit[index] = wine_ldt_get_limit(entry);
        wine_ldt_copy.flags[index] = (entry->HighWord.Bits.Type |
                                 (entry->HighWord.Bits.Default_Big ? WINE_LDT_FLAGS_32BIT : 0) |
                                 (wine_ldt_copy.flags[index] & WINE_LDT_FLAGS_ALLOCATED));
    }
    return ret;
}


/***********************************************************************
 *           selector access functions
 */
#ifdef __i386__
# ifndef _MSC_VER
/* Nothing needs to be done for MS C, it will do with inline versions from the winnt.h */
__ASM_GLOBAL_FUNC( wine_get_cs, "movw %cs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_ds, "movw %ds,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_es, "movw %es,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_fs, "movw %fs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_gs, "movw %gs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_ss, "movw %ss,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_set_fs, "movl 4(%esp),%eax\n\tmovw %ax,%fs\n\tret" )
__ASM_GLOBAL_FUNC( wine_set_gs, "movl 4(%esp),%eax\n\tmovw %ax,%gs\n\tret" )
# endif /* defined(_MSC_VER) */
#endif /* defined(__i386__) */
