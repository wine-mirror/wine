/*
 * Definitions for the Wine library
 *
 * Copyright 2000 Alexandre Julliard
 */

#ifndef __WINE_WINE_LIBRARY_H
#define __WINE_WINE_LIBRARY_H

#include <sys/types.h>
#include "winbase.h"

/* dll loading */

typedef void (*load_dll_callback_t)( void *, const char * );

extern void wine_dll_set_callback( load_dll_callback_t load );
extern void *wine_dll_load( const char *filename, char *error, int errorsize );
extern void *wine_dll_load_main_exe( const char *name, int search_path );
extern void wine_dll_unload( void *handle );

/* debugging */

extern void wine_dbg_add_option( const char *name, unsigned char set, unsigned char clear );

/* portability */

extern void *wine_anon_mmap( void *start, size_t size, int prot, int flags );

/* LDT management */

extern void wine_ldt_get_entry( unsigned short sel, LDT_ENTRY *entry );
extern int wine_ldt_set_entry( unsigned short sel, const LDT_ENTRY *entry );

/* the local copy of the LDT */
extern struct __wine_ldt_copy
{
    void         *base[8192];  /* base address or 0 if entry is free   */
    unsigned long limit[8192]; /* limit in bytes or 0 if entry is free */
    unsigned char flags[8192]; /* flags (defined below) */
} wine_ldt_copy;

#define WINE_LDT_FLAGS_DATA      0x13  /* Data segment */
#define WINE_LDT_FLAGS_STACK     0x17  /* Stack segment */
#define WINE_LDT_FLAGS_CODE      0x1b  /* Code segment */
#define WINE_LDT_FLAGS_TYPE_MASK 0x1f  /* Mask for segment type */
#define WINE_LDT_FLAGS_32BIT     0x40  /* Segment is 32-bit (code or stack) */
#define WINE_LDT_FLAGS_ALLOCATED 0x80  /* Segment is allocated (no longer free) */

/* helper functions to manipulate the LDT_ENTRY structure */
inline static void wine_ldt_set_base( LDT_ENTRY *ent, const void *base )
{
    ent->BaseLow               = (WORD)(unsigned long)base;
    ent->HighWord.Bits.BaseMid = (BYTE)((unsigned long)base >> 16);
    ent->HighWord.Bits.BaseHi  = (BYTE)((unsigned long)base >> 24);
}
inline static void wine_ldt_set_limit( LDT_ENTRY *ent, unsigned int limit )
{
    if ((ent->HighWord.Bits.Granularity = (limit >= 0x100000))) limit >>= 12;
    ent->LimitLow = (WORD)limit;
    ent->HighWord.Bits.LimitHi = (limit >> 16);
}
inline static void *wine_ldt_get_base( const LDT_ENTRY *ent )
{
    return (void *)(ent->BaseLow |
                    (unsigned long)ent->HighWord.Bits.BaseMid << 16 |
                    (unsigned long)ent->HighWord.Bits.BaseHi << 24);
}
inline static unsigned int wine_ldt_get_limit( const LDT_ENTRY *ent )
{
    unsigned int limit = ent->LimitLow | (ent->HighWord.Bits.LimitHi << 16);
    if (ent->HighWord.Bits.Granularity) limit = (limit << 12) | 0xfff;
    return limit;
}
inline static void wine_ldt_set_flags( LDT_ENTRY *ent, unsigned char flags )
{
    ent->HighWord.Bits.Dpl         = 3;
    ent->HighWord.Bits.Pres        = 1;
    ent->HighWord.Bits.Type        = flags;
    ent->HighWord.Bits.Sys         = 0;
    ent->HighWord.Bits.Reserved_0  = 0;
    ent->HighWord.Bits.Default_Big = (flags & WINE_LDT_FLAGS_32BIT) != 0;
}
inline static unsigned char wine_ldt_get_flags( const LDT_ENTRY *ent )
{
    unsigned char ret = ent->HighWord.Bits.Type;
    if (ent->HighWord.Bits.Default_Big) ret |= WINE_LDT_FLAGS_32BIT;
    return ret;
}

#endif  /* __WINE_WINE_LIBRARY_H */
