/*
 * LDT copy
 *
 * Copyright 1995 Alexandre Julliard
 */

#ifndef _WINE_LDT_H
#define _WINE_LDT_H

enum seg_type
{
    SEGMENT_DATA  = 0,
    SEGMENT_STACK = 1,
    SEGMENT_CODE  = 2
};

  /* This structure represents a real LDT entry.        */
  /* It is used by get_ldt_entry() and set_ldt_entry(). */
typedef struct
{
    unsigned long base;            /* base address */
    unsigned long limit;           /* segment limit */
    int           seg_32bit;       /* is segment 32-bit? */
    int           read_only;       /* is segment read-only? */
    int           limit_in_pages;  /* is the limit in pages or bytes? */
    enum seg_type type;            /* segment type */
} ldt_entry;

extern void LDT_BytesToEntry( const unsigned long *buffer, ldt_entry *content);
extern void LDT_EntryToBytes( unsigned long *buffer, const ldt_entry *content);
extern int LDT_GetEntry( int entry, ldt_entry *content );
extern int LDT_SetEntry( int entry, const ldt_entry *content );
extern void LDT_Print();


  /* This structure is used to build the local copy of the LDT. */
typedef struct
{
    unsigned long base;    /* base address or 0 if entry is free   */
    unsigned long limit;   /* limit in bytes or 0 if entry is free */
} ldt_copy_entry;

#define LDT_SIZE  8192

extern ldt_copy_entry ldt_copy[LDT_SIZE];

#define __AHSHIFT  3
#define __AHINCR   (1 << __AHSHIFT)

#define SELECTOR_TO_ENTRY(sel)  (((int)(sel) & 0xffff) >> __AHSHIFT)
#define ENTRY_TO_SELECTOR(i)    ((i) ? (((int)(i) << __AHSHIFT) | 7) : 0)
#define IS_LDT_ENTRY_FREE(i)    (!(ldt_copy[(i)].base || ldt_copy[(i)].limit))
#define IS_SELECTOR_FREE(sel)   (IS_LDT_ENTRY_FREE(SELECTOR_TO_ENTRY(sel)))
#define GET_SEL_BASE(sel)       (ldt_copy[SELECTOR_TO_ENTRY(sel)].base)
#define GET_SEL_LIMIT(sel)      (ldt_copy[SELECTOR_TO_ENTRY(sel)].limit)

  /* Convert a segmented ptr (16:16) to a linear (32) pointer */
#define PTR_SEG_TO_LIN(ptr) \
           ((void*)(GET_SEL_BASE((int)(ptr) >> 16) + ((int)(ptr) & 0xffff)))

#define PTR_SEG_OFF_TO_LIN(seg,off) \
           ((void*)(GET_SEL_BASE(seg) + (unsigned int)(off)))


extern unsigned char ldt_flags_copy[LDT_SIZE];

#define LDT_FLAGS_TYPE      0x03  /* Mask for segment type */
#define LDT_FLAGS_READONLY  0x04  /* Segment is read-only (data) */
#define LDT_FLAGS_EXECONLY  0x04  /* Segment is execute-only (code) */
#define LDT_FLAGS_32BIT     0x08  /* Segment is 32-bit (code or stack) */
#define LDT_FLAGS_BIG       0x10  /* Segment is big (limit is in pages) */

#define GET_SEL_FLAGS(sel)   (ldt_flags_copy[SELECTOR_TO_ENTRY(sel)])

#endif  /* _WINE_LDT_H */
