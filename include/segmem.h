/* $Id: segmem.h,v 1.3 1993/07/04 04:04:21 root Exp root $
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#ifndef SEGMEM_H
#define SEGMEM_H

/*
 * Structure to hold info about each selector we create.
 */

struct segment_descriptor_s
{
    void          *base_addr;	/* Pointer to segment in flat memory	*/
    unsigned int   length;	/* Length of segment			*/
    unsigned int   flags;	/* Segment flags (see neexe.h and below)*/
    unsigned short selector;	/* Selector used to access this segment */
};

/*
 * Additional flags
 */
#define NE_SEGFLAGS_MALLOCED	0x00010000 /* Memory allocated with malloc() */

/*
 * Global memory flags
 */
#define GLOBAL_FLAGS_MOVEABLE		0x0002
#define GLOBAL_FLAGS_ZEROINIT		0x0040
#define GLOBAL_FLAGS_CODE		0x00010000
#define GLOBAL_FLAGS_EXECUTEONLY	0x00020000
#define GLOBAL_FLAGS_READONLY		0x00020000

#define FIRST_SELECTOR	8

static __inline__ int Is16bitAddress(void *address)
{
    return ((int) address >= (((FIRST_SELECTOR << 3) | 0x0007) << 16));
}

#endif /* SEGMEM_H */
