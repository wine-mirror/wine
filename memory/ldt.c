/*
 * LDT manipulation functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ldt.h"
#include "debugtools.h"

DEFAULT_DEBUG_CHANNEL(ldt)

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


ldt_copy_entry ldt_copy[LDT_SIZE];
unsigned char ldt_flags_copy[LDT_SIZE];

        
/***********************************************************************
 *           LDT_BytesToEntry
 *
 * Convert the raw bytes of the descriptor to an ldt_entry structure.
 */
void LDT_BytesToEntry( const unsigned long *buffer, ldt_entry *content )
{
    content->base  = (*buffer >> 16) & 0x0000ffff;
    content->limit = *buffer & 0x0000ffff;
    buffer++;
    content->base  |= (*buffer & 0xff000000) | ((*buffer << 16) & 0x00ff0000);
    content->limit |= (*buffer & 0x000f0000);
    content->type           = (*buffer >> 10) & 3;
    content->seg_32bit      = (*buffer & 0x00400000) != 0;
    content->read_only      = (*buffer & 0x00000200) == 0;
    content->limit_in_pages = (*buffer & 0x00800000) != 0;
}


/***********************************************************************
 *           LDT_EntryToBytes
 *
 * Convert an ldt_entry structure to the raw bytes of the descriptor.
 */
void LDT_EntryToBytes( unsigned long *buffer, const ldt_entry *content )
{
    *buffer++ = ((content->base & 0x0000ffff) << 16) |
                 (content->limit & 0x0ffff);
    *buffer = (content->base & 0xff000000) |
              ((content->base & 0x00ff0000)>>16) |
              (content->limit & 0xf0000) |
              (content->type << 10) |
              ((content->read_only == 0) << 9) |
              ((content->seg_32bit != 0) << 22) |
              ((content->limit_in_pages != 0) << 23) |
              0xf000;
}


/***********************************************************************
 *           LDT_GetEntry
 *
 * Retrieve an LDT entry.
 */
int LDT_GetEntry( int entry, ldt_entry *content )
{
    int ret = 0;

    content->base           = ldt_copy[entry].base;
    content->limit          = ldt_copy[entry].limit;
    content->type           = (ldt_flags_copy[entry] & LDT_FLAGS_TYPE);
    content->seg_32bit      = (ldt_flags_copy[entry] & LDT_FLAGS_32BIT) != 0;
    content->read_only      = (ldt_flags_copy[entry] & LDT_FLAGS_READONLY) !=0;
    content->limit_in_pages = (ldt_flags_copy[entry] & LDT_FLAGS_BIG) !=0;
    if (content->limit_in_pages) content->limit >>= 12;
    return ret;
}


/***********************************************************************
 *           LDT_SetEntry
 *
 * Set an LDT entry.
 */
int LDT_SetEntry( int entry, const ldt_entry *content )
{
    int ret = 0;

    TRACE("entry=%04x base=%08lx limit=%05lx %s %d-bit "
		 "flags=%c%c%c\n", entry, content->base, content->limit,
		 content->limit_in_pages ? "pages" : "bytes",
		 content->seg_32bit ? 32 : 16,
		 content->read_only && (content->type & SEGMENT_CODE) ? '-' : 'r',
		 content->read_only || (content->type & SEGMENT_CODE) ? '-' : 'w',
		 (content->type & SEGMENT_CODE) ? 'x' : '-' );
    
    /* Entry 0 must not be modified; its base and limit are always 0 */
    if (!entry) return 0;

#ifdef __i386__

#ifdef linux
    {
        struct modify_ldt_s ldt_info;

        ldt_info.entry_number    = entry;
        ldt_info.base_addr       = content->base;
        ldt_info.limit           = content->limit;
        ldt_info.seg_32bit       = content->seg_32bit != 0;
        ldt_info.contents        = content->type;
        ldt_info.read_exec_only  = content->read_only != 0;
        ldt_info.limit_in_pages  = content->limit_in_pages != 0;
        ldt_info.seg_not_present = 0;
        /* Make sure the info will be accepted by the kernel */
        /* This is ugly, but what can I do? */
        if (content->type == SEGMENT_STACK)
        {
            /* FIXME */
        }
        else
        {
            if (ldt_info.base_addr >= 0xc0000000)
            {
                WARN("Invalid base addr %08lx\n",
                         ldt_info.base_addr );
                return -1;
            }
            if (content->limit_in_pages)
            {
                if ((ldt_info.limit << 12) + 0xfff >
                                               0xc0000000 - ldt_info.base_addr)
                    ldt_info.limit = (0xc0000000 - 0xfff - ldt_info.base_addr) >> 12;
            }
            else
            {
                if (ldt_info.limit > 0xc0000000 - ldt_info.base_addr)
                    ldt_info.limit = 0xc0000000 - ldt_info.base_addr;
            }
        }
        if ((ret = modify_ldt(1, &ldt_info, sizeof(ldt_info))) < 0)
            perror( "modify_ldt" );
    }
#endif  /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__) || defined(__OpenBSD__)
    {
        long d[2];

        LDT_EntryToBytes( d, content );
        ret = i386_set_ldt(entry, (union descriptor *)d, 1);
        if (ret < 0)
        {
            perror("i386_set_ldt");
            MESSAGE("Did you reconfigure the kernel with \"options USER_LDT\"?\n");
    	    exit(1);
        }
    }
#endif  /* __NetBSD__ || __FreeBSD__ || __OpenBSD__ */

#if defined(__svr4__) || defined(_SCO_DS)
    {
        struct ssd ldt_mod;
        int i;
        ldt_mod.sel = ENTRY_TO_SELECTOR(entry) | 4;
        ldt_mod.bo = content->base;
        ldt_mod.ls = content->limit;
        i = ((content->limit & 0xf0000) |
             (content->type << 10) |
             (((content->read_only != 0) ^ 1) << 9) |
             ((content->seg_32bit != 0) << 22) |
             ((content->limit_in_pages != 0)<< 23) |
             (1<<15) |
             0x7000);

        ldt_mod.acc1 = (i & 0xff00) >> 8;
        ldt_mod.acc2 = (i & 0xf00000) >> 20;

        if (content->base == 0)
        {
            ldt_mod.acc1 =  0;
            ldt_mod.acc2 = 0;
        }
        if ((ret = sysi86(SI86DSCR, &ldt_mod)) == -1) perror("sysi86");
    }    
#endif

#endif  /* __i386__ */

    if (ret < 0) return ret;
    ldt_copy[entry].base = content->base;
    if (!content->limit_in_pages) ldt_copy[entry].limit = content->limit;
    else ldt_copy[entry].limit = (content->limit << 12) | 0x0fff;
    ldt_flags_copy[entry] = (content->type & LDT_FLAGS_TYPE) |
                            (content->read_only ? LDT_FLAGS_READONLY : 0) |
                            (content->seg_32bit ? LDT_FLAGS_32BIT : 0) |
                            (content->limit_in_pages ? LDT_FLAGS_BIG : 0) |
                            (ldt_flags_copy[entry] & LDT_FLAGS_ALLOCATED);
    return ret;
}


/***********************************************************************
 *           LDT_Print
 *
 * Print the content of the LDT on stdout.
 */
void LDT_Print( int start, int length )
{
    int i;
    char flags[3];

    if (length == -1) length = LDT_SIZE - start;
    for (i = start; i < start + length; i++)
    {
        if (!ldt_copy[i].base && !ldt_copy[i].limit) continue; /* Free entry */
        if ((ldt_flags_copy[i] & LDT_FLAGS_TYPE) == SEGMENT_CODE)
        {
            flags[0] = (ldt_flags_copy[i] & LDT_FLAGS_EXECONLY) ? '-' : 'r';
            flags[1] = '-';
            flags[2] = 'x';
        }
        else
        {
            flags[0] = 'r';
            flags[1] = (ldt_flags_copy[i] & LDT_FLAGS_READONLY) ? '-' : 'w';
            flags[2] = '-';
        }
        MESSAGE("%04x: sel=%04x base=%08lx limit=%08lx %d-bit %c%c%c\n",
            i, ENTRY_TO_SELECTOR(i), ldt_copy[i].base, ldt_copy[i].limit,
            ldt_flags_copy[i] & LDT_FLAGS_32BIT ? 32 : 16,
            flags[0], flags[1], flags[2] );
    }
}
