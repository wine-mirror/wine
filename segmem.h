/* $Id: segmem.h,v 1.1 1993/06/29 15:55:18 root Exp $
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

#endif /* SEGMEM_H */
