#ifndef WINELIB
static char RCSId[] = "$Id: ldt.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include "prototypes.h"

#if defined(__NetBSD__) || defined(__FreeBSD__)
#include <machine/segments.h>
#endif

/**********************************************************************
 *					print_ldt
 */
/* XXX These are *real* 386 descriptors !! */
void
print_ldt()
{
    char buffer[0x10000];
    unsigned long *lp;
    unsigned long base_addr, limit;
    int type, dpl, i;
#if defined(__NetBSD__) || defined(__FreeBSD__)
    struct segment_descriptor *sd;
#endif
    
    if (get_ldt(buffer) < 0)
	exit(1);
    
    lp = (unsigned long *) buffer;
#if defined(__NetBSD__) || defined(__FreeBSD__)
    sd = (struct segment_descriptor *) buffer;
#endif
    
    for (i = 0; i < 32; i++, lp++)
    {
	/* First 32 bits of descriptor */
	base_addr = (*lp >> 16) & 0x0000FFFF;
	limit = *lp & 0x0000FFFF;
	lp++;
	
	/* First 32 bits of descriptor */
	base_addr |= (*lp & 0xFF000000) | ((*lp << 16) & 0x00FF0000);
	limit |= (*lp & 0x000F0000);
#ifdef linux
	type = (*lp >> 10) & 5;
	dpl = (*lp >> 13) & 3;
#endif
#if defined(__NetBSD__) || defined(__FreeBSD__)
        type = sd->sd_type;
        dpl = sd->sd_dpl;
	sd++;
#endif
	if (*lp & 1000)
	{
	    printf("Entry %2d: Base %08lx, Limit %05lx, DPL %d, Type %d\n",
		   i, base_addr, limit, dpl, type);
	    printf("          ");
	    if (*lp & 0x100)
		printf("Accessed, ");
	    if (*lp & 8000)
		printf("Present, ");
	    if (*lp & 0x100000)
		printf("User, ");
	    if (*lp & 0x200000)
		printf("X, ");
	    if (*lp & 0x400000)
		printf("32, ");
	    else
		printf("16, ");
	    if (*lp & 0x800000)
		printf("page limit, ");
	    else
		printf("byte limit, ");
	    printf("\n");
	    printf("          %08lx %08lx\n", *(lp), *(lp-1));
	}
	else
	{
	    printf("Entry %2d: Base %08lx, Limit %05lx, DPL %d, Type %d\n",
		   i, base_addr, limit, dpl, type);
	    printf("          SYSTEM: %08lx %08lx\n", *lp, *(lp-1));
	}
    }
}

#endif /* ifndef WINELIB */
