/* $Id$
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>
#include "prototypes.h"

/**********************************************************************
 *					print_ldt
 */
void
print_ldt()
{
    char buffer[0x10000];
    struct modify_ldt_ldt_s ldt_info;
    unsigned long *lp;
    unsigned long base_addr, limit;
    int type, dpl, i;
    
    if (get_ldt(buffer) < 0)
	exit(1);
    
    lp = (unsigned long *) buffer;
    for (i = 0; i < 32; i++, lp++)
    {
	/* First 32 bits of descriptor */
	base_addr = (*lp >> 16) & 0x0000FFFF;
	limit = *lp & 0x0000FFFF;
	lp++;
	
	/* First 32 bits of descriptor */
	base_addr |= (*lp & 0xFF000000) | ((*lp << 16) & 0x00FF0000);
	limit |= (*lp & 0x000F0000);
	type = (*lp >> 9) & 7;
	dpl = (*lp >> 13) & 3;

	if (*lp & 1000)
	{
	    printf("Entry %2d: Base %08.8x, Limit %05.5x, DPL %d, Type %d\n",
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
	    printf("          %08.8x %08.8x\n", *(lp), *(lp-1));
	}
	else
	{
	    printf("Entry %2d: Base %08.8x, Limit %05.5x, DPL %d, Type %d\n",
		   i, base_addr, limit, dpl, type);
	    printf("          SYSTEM: %08.8x %08.8x\n", *lp, *(lp-1));
	}
    }
}
