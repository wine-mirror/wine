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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <assert.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "wine/asm.h"

#ifdef __i386__

#ifdef __ASM_OBSOLETE

/* the local copy of the LDT */
struct __wine_ldt_copy
{
    void         *base[8192];  /* base address or 0 if entry is free   */
    unsigned long limit[8192]; /* limit in bytes or 0 if entry is free */
    unsigned char flags[8192]; /* flags (defined below) */
} wine_ldt_copy_obsolete = { { 0, 0, 0 } };

#define WINE_LDT_FLAGS_32BIT     0x40  /* Segment is 32-bit (code or stack) */
#define WINE_LDT_FLAGS_ALLOCATED 0x80  /* Segment is allocated (no longer free) */

static inline void *wine_ldt_get_base( const LDT_ENTRY *ent )
{
    return (void *)(ent->BaseLow |
                    (ULONG_PTR)ent->HighWord.Bits.BaseMid << 16 |
                    (ULONG_PTR)ent->HighWord.Bits.BaseHi << 24);
}
static inline unsigned int wine_ldt_get_limit( const LDT_ENTRY *ent )
{
    unsigned int limit = ent->LimitLow | (ent->HighWord.Bits.LimitHi << 16);
    if (ent->HighWord.Bits.Granularity) limit = (limit << 12) | 0xfff;
    return limit;
}

#ifdef __linux__

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
    unsigned int  usable : 1;
    unsigned int  garbage : 25;
};

static inline void fill_modify_ldt_struct( struct modify_ldt_s *ptr, const LDT_ENTRY *entry )
{
    ptr->base_addr       = (unsigned long)wine_ldt_get_base(entry);
    ptr->limit           = entry->LimitLow | (entry->HighWord.Bits.LimitHi << 16);
    ptr->seg_32bit       = entry->HighWord.Bits.Default_Big;
    ptr->contents        = (entry->HighWord.Bits.Type >> 2) & 3;
    ptr->read_exec_only  = !(entry->HighWord.Bits.Type & 2);
    ptr->limit_in_pages  = entry->HighWord.Bits.Granularity;
    ptr->seg_not_present = !entry->HighWord.Bits.Pres;
    ptr->usable          = entry->HighWord.Bits.Sys;
    ptr->garbage         = 0;
}

static inline int modify_ldt( int func, struct modify_ldt_s *ptr, unsigned long count )
{
    return syscall( 123 /* SYS_modify_ldt */, func, ptr, count );
}

static inline int set_thread_area( struct modify_ldt_s *ptr )
{
    return syscall( 243 /* SYS_set_thread_area */, ptr );
}

#endif  /* linux */

#if defined(__svr4__) || defined(_SCO_DS)
#include <sys/sysi86.h>
#ifndef __sun__
#include <sys/seg.h>
#endif
#endif

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__DragonFly__)
#include <machine/segments.h>
#include <machine/sysarch.h>
#endif  /* __NetBSD__ || __FreeBSD__ || __OpenBSD__ */

#ifdef __GNU__
#include <mach/i386/mach_i386.h>
#include <mach/mach_traps.h>
#endif

#ifdef __APPLE__
#include <i386/user_ldt.h>
#endif

static const LDT_ENTRY null_entry;  /* all-zeros, used to clear LDT entries */

#define LDT_FIRST_ENTRY 512
#define LDT_SIZE 8192

/* empty function for default locks */
static void nop(void) { }

static void (*lock_ldt)(void) = nop;
static void (*unlock_ldt)(void) = nop;


static inline int is_gdt_sel( unsigned short sel ) { return !(sel & 4); }

/***********************************************************************
 *           wine_ldt_init_locking
 *
 * Set the LDT locking/unlocking functions.
 */
void wine_ldt_init_locking_obsolete( void (*lock_func)(void), void (*unlock_func)(void) )
{
    lock_ldt = lock_func;
    unlock_ldt = unlock_func;
}


/***********************************************************************
 *           wine_ldt_get_entry
 *
 * Retrieve an LDT entry. Return a null entry if selector is not allocated.
 */
void wine_ldt_get_entry_obsolete( unsigned short sel, LDT_ENTRY *entry )
{
    int index = sel >> 3;

    if (is_gdt_sel(sel))
    {
        *entry = null_entry;
        return;
    }
    lock_ldt();
    if (wine_ldt_copy_obsolete.flags[index] & WINE_LDT_FLAGS_ALLOCATED)
    {
        ULONG_PTR base = (ULONG_PTR)wine_ldt_copy_obsolete.base[index];
        ULONG limit = wine_ldt_copy_obsolete.limit[index];

        entry->BaseLow                   = (WORD)base;
        entry->HighWord.Bits.BaseMid     = (BYTE)(base >> 16);
        entry->HighWord.Bits.BaseHi      = (BYTE)(base >> 24);
        if ((entry->HighWord.Bits.Granularity = (limit >= 0x100000))) limit >>= 12;
        entry->LimitLow                  = (WORD)limit;
        entry->HighWord.Bits.LimitHi     = (limit >> 16);
        entry->HighWord.Bits.Dpl         = 3;
        entry->HighWord.Bits.Pres        = 1;
        entry->HighWord.Bits.Type        = wine_ldt_copy_obsolete.flags[index];
        entry->HighWord.Bits.Sys         = 0;
        entry->HighWord.Bits.Reserved_0  = 0;
        entry->HighWord.Bits.Default_Big = !!(wine_ldt_copy_obsolete.flags[index] & WINE_LDT_FLAGS_32BIT);
    }
    else *entry = null_entry;
    unlock_ldt();
}


/***********************************************************************
 *           internal_set_entry
 *
 * Set an LDT entry, without locking. For internal use only.
 */
static int internal_set_entry( unsigned short sel, const LDT_ENTRY *entry )
{
    int ret = 0, index = sel >> 3;

    if (index < LDT_FIRST_ENTRY) return 0;  /* cannot modify reserved entries */

#ifdef linux
    {
        struct modify_ldt_s ldt_info;

        ldt_info.entry_number = index;
        fill_modify_ldt_struct( &ldt_info, entry );
        if ((ret = modify_ldt(0x11, &ldt_info, sizeof(ldt_info))) < 0)
            perror( "modify_ldt" );
    }
#elif defined(__NetBSD__) || defined(__FreeBSD__) || defined(__FreeBSD_kernel__) || defined(__OpenBSD__) || defined(__DragonFly__)
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
#elif defined(__svr4__) || defined(_SCO_DS)
    {
        struct ssd ldt_mod;
        ldt_mod.sel  = sel;
        ldt_mod.bo   = (unsigned long)wine_ldt_get_base(entry);
        ldt_mod.ls   = entry->LimitLow | (entry->HighWord.Bits.LimitHi << 16);
        ldt_mod.acc1 = entry->HighWord.Bytes.Flags1;
        ldt_mod.acc2 = entry->HighWord.Bytes.Flags2 >> 4;
        if ((ret = sysi86(SI86DSCR, &ldt_mod)) == -1) perror("sysi86");
    }
#elif defined(__APPLE__)
    if ((ret = i386_set_ldt(index, (union ldt_entry *)entry, 1)) < 0)
        perror("i386_set_ldt");
#elif defined(__GNU__)
    if ((ret = i386_set_ldt(mach_thread_self(), sel, (descriptor_list_t)entry, 1)) != KERN_SUCCESS)
        perror("i386_set_ldt");
#else
    fprintf( stderr, "No LDT support on this platform\n" );
    exit(1);
#endif

    if (ret >= 0)
    {
        wine_ldt_copy_obsolete.base[index]  = wine_ldt_get_base(entry);
        wine_ldt_copy_obsolete.limit[index] = wine_ldt_get_limit(entry);
        wine_ldt_copy_obsolete.flags[index] = (entry->HighWord.Bits.Type |
                                 (entry->HighWord.Bits.Default_Big ? WINE_LDT_FLAGS_32BIT : 0) |
                                 (wine_ldt_copy_obsolete.flags[index] & WINE_LDT_FLAGS_ALLOCATED));
    }
    return ret;
}


/***********************************************************************
 *           wine_ldt_set_entry
 *
 * Set an LDT entry.
 */
int wine_ldt_set_entry_obsolete( unsigned short sel, const LDT_ENTRY *entry )
{
    int ret;

    lock_ldt();
    ret = internal_set_entry( sel, entry );
    unlock_ldt();
    return ret;
}


/***********************************************************************
 *           wine_ldt_is_system
 *
 * Check if the selector is a system selector (i.e. not managed by Wine).
 */
int wine_ldt_is_system_obsolete( unsigned short sel )
{
    return is_gdt_sel(sel) || ((sel >> 3) < LDT_FIRST_ENTRY);
}


/***********************************************************************
 *           wine_ldt_get_ptr
 *
 * Convert a segment:offset pair to a linear pointer.
 * Note: we don't lock the LDT since this has to be fast.
 */
void *wine_ldt_get_ptr_obsolete( unsigned short sel, unsigned long offset )
{
    int index;

    if (is_gdt_sel(sel))  /* GDT selector */
        return (void *)offset;
    if ((index = (sel >> 3)) < LDT_FIRST_ENTRY)  /* system selector */
        return (void *)offset;
    if (!(wine_ldt_copy_obsolete.flags[index] & WINE_LDT_FLAGS_32BIT)) offset &= 0xffff;
    return (char *)wine_ldt_copy_obsolete.base[index] + offset;
}


/***********************************************************************
 *           wine_ldt_alloc_entries
 *
 * Allocate a number of consecutive ldt entries, without setting the LDT contents.
 * Return a selector for the first entry.
 */
unsigned short wine_ldt_alloc_entries_obsolete( int count )
{
    int i, index, size = 0;

    if (count <= 0) return 0;
    lock_ldt();
    for (i = LDT_FIRST_ENTRY; i < LDT_SIZE; i++)
    {
        if (wine_ldt_copy_obsolete.flags[i] & WINE_LDT_FLAGS_ALLOCATED) size = 0;
        else if (++size >= count)  /* found a large enough block */
        {
            index = i - size + 1;

            /* mark selectors as allocated */
            for (i = 0; i < count; i++) wine_ldt_copy_obsolete.flags[index + i] |= WINE_LDT_FLAGS_ALLOCATED;
            unlock_ldt();
            return (index << 3) | 7;
        }
    }
    unlock_ldt();
    return 0;
}


void wine_ldt_free_entries_obsolete( unsigned short sel, int count );

/***********************************************************************
 *           wine_ldt_realloc_entries
 *
 * Reallocate a number of consecutive ldt entries, without changing the LDT contents.
 * Return a selector for the first entry.
 */
unsigned short wine_ldt_realloc_entries_obsolete( unsigned short sel, int oldcount, int newcount )
{
    int i;

    if (oldcount < newcount)  /* we need to add selectors */
    {
        int index = sel >> 3;

        lock_ldt();
        /* check if the next selectors are free */
        if (index + newcount > LDT_SIZE) i = oldcount;
        else
            for (i = oldcount; i < newcount; i++)
                if (wine_ldt_copy_obsolete.flags[index+i] & WINE_LDT_FLAGS_ALLOCATED) break;

        if (i < newcount)  /* they are not free */
        {
            wine_ldt_free_entries_obsolete( sel, oldcount );
            sel = wine_ldt_alloc_entries_obsolete( newcount );
        }
        else  /* mark the selectors as allocated */
        {
            for (i = oldcount; i < newcount; i++)
                wine_ldt_copy_obsolete.flags[index+i] |= WINE_LDT_FLAGS_ALLOCATED;
        }
        unlock_ldt();
    }
    else if (oldcount > newcount) /* we need to remove selectors */
    {
        wine_ldt_free_entries_obsolete( sel + (newcount << 3), newcount - oldcount );
    }
    return sel;
}


/***********************************************************************
 *           wine_ldt_free_entries
 *
 * Free a number of consecutive ldt entries and clear their contents.
 */
void wine_ldt_free_entries_obsolete( unsigned short sel, int count )
{
    int index;

    lock_ldt();
    for (index = sel >> 3; count > 0; count--, index++)
    {
        internal_set_entry( sel, &null_entry );
        wine_ldt_copy_obsolete.flags[index] = 0;
    }
    unlock_ldt();
}


static int global_fs_sel = -1;  /* global selector for %fs shared among all threads */

/***********************************************************************
 *           wine_ldt_alloc_fs
 *
 * Allocate an LDT entry for a %fs selector, reusing a global
 * GDT selector if possible. Return the selector value.
 */
unsigned short wine_ldt_alloc_fs_obsolete(void)
{
    if (global_fs_sel == -1)
    {
#ifdef __linux__
        struct modify_ldt_s ldt_info;
        int ret;

        /* the preloader may have allocated it already */
        __asm__( "mov %%fs,%0" : "=r" (global_fs_sel) );
        if (global_fs_sel && is_gdt_sel(global_fs_sel)) return global_fs_sel;

        memset( &ldt_info, 0, sizeof(ldt_info) );
        ldt_info.entry_number = -1;
        ldt_info.seg_32bit = 1;
        ldt_info.usable = 1;
        if ((ret = set_thread_area( &ldt_info ) < 0))
        {
            global_fs_sel = 0;  /* don't try it again */
            if (errno != ENOSYS) perror( "set_thread_area" );
        }
        else global_fs_sel = (ldt_info.entry_number << 3) | 3;
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__)
        global_fs_sel = GSEL( GUFS_SEL, SEL_UPL );
#endif
    }
    if (global_fs_sel > 0) return global_fs_sel;
    return wine_ldt_alloc_entries_obsolete( 1 );
}


/***********************************************************************
 *           wine_ldt_init_fs
 *
 * Initialize the entry for the %fs selector of the current thread, and
 * set the thread %fs register.
 *
 * Note: this runs in the context of the new thread, so cannot acquire locks.
 */
void wine_ldt_init_fs_obsolete( unsigned short sel, const LDT_ENTRY *entry )
{
    if ((sel & ~3) == (global_fs_sel & ~3))
    {
#ifdef __linux__
        struct modify_ldt_s ldt_info;
        int ret;

        ldt_info.entry_number = sel >> 3;
        fill_modify_ldt_struct( &ldt_info, entry );
        if ((ret = set_thread_area( &ldt_info ) < 0)) perror( "set_thread_area" );
#elif defined(__FreeBSD__) || defined (__FreeBSD_kernel__) || defined(__DragonFly__)
        i386_set_fsbase( wine_ldt_get_base( entry ));
#endif
    }
    else  /* LDT selector */
    {
        internal_set_entry( sel, entry );
    }
    __asm__( "mov %0,%%fs" :: "r" (sel) );
}


/***********************************************************************
 *           wine_ldt_free_fs
 *
 * Free a %fs selector returned by wine_ldt_alloc_fs.
 */
void wine_ldt_free_fs_obsolete( unsigned short sel )
{
    WORD fs;

    if (is_gdt_sel(sel)) return;  /* nothing to do */
    __asm__( "mov %%fs,%0" : "=r" (fs) );
    if (!((fs ^ sel) & ~3))
    {
        /* FIXME: if freeing current %fs we cannot acquire locks */
        __asm__( "mov %0,%%fs" :: "r" (0) );
        internal_set_entry( sel, &null_entry );
        wine_ldt_copy_obsolete.flags[sel >> 3] = 0;
    }
    else wine_ldt_free_entries_obsolete( sel, 1 );
}


/***********************************************************************
 *           selector access functions
 */
__ASM_GLOBAL_FUNC( wine_get_cs_obsolete, "movw %cs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_ds_obsolete, "movw %ds,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_es_obsolete, "movw %es,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_fs_obsolete, "movw %fs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_gs_obsolete, "movw %gs,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_get_ss_obsolete, "movw %ss,%ax\n\tret" )
__ASM_GLOBAL_FUNC( wine_set_fs_obsolete, "movl 4(%esp),%eax\n\tmovw %ax,%fs\n\tret" )
__ASM_GLOBAL_FUNC( wine_set_gs_obsolete, "movl 4(%esp),%eax\n\tmovw %ax,%gs\n\tret" )


__ASM_OBSOLETE(wine_ldt_alloc_entries);
__ASM_OBSOLETE(wine_ldt_alloc_fs);
__ASM_OBSOLETE(wine_ldt_copy);
__ASM_OBSOLETE(wine_ldt_free_entries);
__ASM_OBSOLETE(wine_ldt_free_fs);
__ASM_OBSOLETE(wine_ldt_get_entry);
__ASM_OBSOLETE(wine_ldt_get_ptr);
__ASM_OBSOLETE(wine_ldt_init_fs);
__ASM_OBSOLETE(wine_ldt_init_locking);
__ASM_OBSOLETE(wine_ldt_is_system);
__ASM_OBSOLETE(wine_ldt_realloc_entries);
__ASM_OBSOLETE(wine_ldt_set_entry);
__ASM_OBSOLETE(wine_get_cs);
__ASM_OBSOLETE(wine_get_ds);
__ASM_OBSOLETE(wine_get_es);
__ASM_OBSOLETE(wine_get_fs);
__ASM_OBSOLETE(wine_get_gs);
__ASM_OBSOLETE(wine_get_ss);
__ASM_OBSOLETE(wine_set_fs);
__ASM_OBSOLETE(wine_set_gs);

#endif /* __ASM_OBSOLETE */

#endif /* __i386__ */
