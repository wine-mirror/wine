/*
 * LDT manipulation functions
 *
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <errno.h>
#include "ldt.h"
#include "stddebug.h"
#include "debug.h"

#ifndef WINELIB

#ifdef linux
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>

_syscall3(int, modify_ldt, int, func, void *, ptr, unsigned long, bytecount)
#endif  /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <machine/segments.h>

extern int i386_get_ldt(int, union descriptor *, int);
extern int i386_set_ldt(int, union descriptor *, int);
#endif  /* __NetBSD__ || __FreeBSD__ */

#endif  /* ifndef WINELIB */

        
/***********************************************************************
 *           LDT_BytesToEntry
 *
 * Convert the raw bytes of the descriptor to an ldt_entry structure.
 */
static void LDT_BytesToEntry( unsigned long *buffer, ldt_entry *content )
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
 *           LDT_GetEntry
 *
 * Retrieve an LDT entry.
 */
int LDT_GetEntry( int entry, ldt_entry *content )
{
    int ret = 0;

#ifdef WINELIB
    content->base           = ldt_copy[entry].base;
    content->limit          = ldt_copy[entry].limit;
    content->type           = SEGMENT_DATA;
    content->seg_32bit      = 0;
    content->read_only      = 0;
    content->limit_in_pages = 0;
#else  /* WINELIB */

#ifdef linux
    int size = (entry + 1) * 2 * sizeof(long);
    long *buffer = (long *) malloc( size );
    ret = modify_ldt( 0, buffer, size );
    LDT_BytesToEntry( &buffer[entry*2], content );
    free( buffer );
#endif  /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__)
    long buffer[2];
    ret = i386_get_ldt( entry, (union descriptor *)buffer, 1 );
    LDT_BytesToEntry( buffer, content );
#endif  /* __NetBSD__ || __FreeBSD__ */

#endif  /* WINELIB */

    return ret;
}


/***********************************************************************
 *           LDT_SetEntry
 *
 * Set an LDT entry.
 */
int LDT_SetEntry( int entry, ldt_entry *content )
{
    int ret = 0;

    dprintf_ldt(stddeb,
	  "LDT_SetEntry: entry=%04x base=%08lx limit=%05lx %s %d-bit flags=%c%c%c\n",
          entry, content->base, content->limit,
          content->limit_in_pages ? "pages" : "bytes",
          content->seg_32bit ? 32 : 16,
          content->read_only && (content->type & SEGMENT_CODE) ? '-' : 'r',
          content->read_only || (content->type & SEGMENT_CODE) ? '-' : 'w',
          (content->type & SEGMENT_CODE) ? 'x' : '-' );

#ifndef WINELIB
#ifdef linux
    {
        struct modify_ldt_ldt_s ldt_info;

        memset( &ldt_info, 0, sizeof(ldt_info) );
        ldt_info.entry_number   = entry;
        ldt_info.base_addr      = content->base;
        ldt_info.limit          = content->limit;
        ldt_info.seg_32bit      = content->seg_32bit != 0;
        ldt_info.contents       = content->type;
        ldt_info.read_exec_only = content->read_only != 0;
        ldt_info.limit_in_pages = content->limit_in_pages != 0;
        ret = modify_ldt(1, &ldt_info, sizeof(ldt_info));
    }
#endif  /* linux */

#if defined(__NetBSD__) || defined(__FreeBSD__)
    {
        long d[2];

        d[0] = ((content->base & 0x0000ffff) << 16) |
                (content->limit & 0x0ffff);
        d[1] = (content->base & 0xff000000) |
               ((content->base & 0x00ff0000)>>16) |
               (content->limit & 0xf0000) |
               (content->type << 10) |
               ((content->read_only == 0) << 9) |
               ((content->seg_32bit != 0) << 22) |
               ((content->limit_in_pages != 0) << 23) |
               0xf000;
        ret = i386_set_ldt(entry, (union descriptor *)d, 1);
        if (ret < 0)
        {
            perror("i386_set_ldt");
            fprintf(stderr,
		"Did you reconfigure the kernel with \"options USER_LDT\"?\n");
    	    exit(1);
        }
    }
#endif  /* __NetBSD__ || __FreeBSD__ */
#endif  /* ifndef WINELIB */

    if (ret < 0) return ret;
    ldt_copy[entry].base = content->base;
    if (!content->limit_in_pages) ldt_copy[entry].limit = content->limit;
    else ldt_copy[entry].limit = (content->limit << 12) | 0x0fff;
    return ret;
}


/***********************************************************************
 *           LDT_Print
 *
 * Print the content of the LDT on stdout.
 */
void LDT_Print()
{
    int i;

#ifdef WINELIB
    for (i = 0; i < LDT_SIZE; i++)
    {
        if (ldt_copy[i].base || ldt_copy[i].limit)
        {
            fprintf( stderr, "%04x: sel=%04x base=%08x limit=%05x\n",
                     i, ENTRY_TO_SELECTOR(i),
                     ldt_copy[i].base, ldt_copy[i].limit );
        }
    }
#else  /* WINELIB */

    long buffer[2*LDT_SIZE];
    ldt_entry content;
    int n;

#ifdef linux
    n = modify_ldt( 0, buffer, sizeof(buffer) ) / 8;
#endif  /* linux */
#if defined(__NetBSD__) || defined(__FreeBSD__)
    n = i386_get_ldt( 0, (union descriptor *)buffer, LDT_SIZE );
#endif  /* __NetBSD__ || __FreeBSD__ */
    for (i = 0; i < n; i++)
    {
        LDT_BytesToEntry( &buffer[2*i], &content );
        if (content.base || content.limit)
        {
            fprintf( stderr, "%04x: sel=%04x base=%08lx limit=%05lx %s type=%d\n",
                    i, ENTRY_TO_SELECTOR(i),
                    content.base, content.limit,
                    content.limit_in_pages ? "(pages)" : "(bytes)",
                    content.type );
        }
    }
#endif  /* WINELIB */
}
