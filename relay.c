/* $Id$
 */
/*
 * Copyright  Robert J. Amstadt, 1993
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>
#include <linux/segment.h>
#include <errno.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "dlls.h"

struct dll_name_table_entry_s dll_builtin_table[4] =
{
    { "KERNEL",  KERNEL_table, 	256, 1 },
    { "USER",    USER_table, 	256, 2 },
    { "GDI",     GDI_table, 	256, 3 },
    { "UNIXLIB", UNIXLIB_table, 256, 4 },
};

unsigned short *Stack16Frame;

/**********************************************************************
 *					DLLRelay
 *
 * We get a stack frame pointer to data that looks like this:
 *
 *   Hex Offset Contents
 *   ---------- -------
 *	+00	previous saved_16ss
 *	+02	previous saved_16ebp
 *	+06	previous saved_16esp
 *	+0A	16-bit es
 *	+0C	16-bit ds
 *	+0E	16-bit ebp
 *	+12	length of 16-bit arguments
 *	+14	16-bit ip
 *	+16	16-bit cs
 *	+18	arguments
 */
int
DLLRelay(unsigned int func_num, unsigned int seg_off)
{
    struct dll_table_entry_s *dll_p;
    unsigned int segment;
    unsigned int offset;
    unsigned int dll_id;
    unsigned int ordinal;
    int arg_table[DLL_MAX_ARGS];
    void *arg_ptr;
    int (*func_ptr)();
    int i;
    
    /*
     * Determine address of arguments.
     */
    Stack16Frame = (unsigned short *) seg_off;
    arg_ptr = (void *) (seg_off + 0x18);

    /*
     * Extract the DLL number and ordinal number.
     */
    dll_id  = ((func_num >> 16) & 0xffff) - 1;
    ordinal = func_num & 0xffff;
    dll_p   = &dll_builtin_table[dll_id].dll_table[ordinal];

#ifdef RELAY_DEBUG
    {
	unsigned int *ret_addr;
	unsigned short *stack_p;
	
	ret_addr = (unsigned int *) ((char *) seg_off + 0x14);
	printf("RELAY: Calling %s.%d, 16-bit stack at %04x:%04x, ",
	       dll_builtin_table[dll_id].dll_name, ordinal,
	       seg_off >> 16, seg_off & 0xffff);
	printf("return to %08x\n", *ret_addr);

#ifdef STACK_DEBUG
	stack_p = (unsigned short *) seg_off;
	for (i = 0; i < 24; i++, stack_p++)
	{
            printf("%04x ", *stack_p);
	    if ((i & 7) == 7)
		printf("\n");
	}
	printf("\n");
#endif /* STACK_DEBUG */
    }
#endif /* RELAY_DEBUG */

    /*
     * Make sure we have a handler defined for this call.
     */
    if (dll_p->handler == NULL)
    {
	char buffer[100];
	
	sprintf(buffer, "No handler for routine %s.%d", 
		dll_builtin_table[dll_id].dll_name, ordinal);
	myerror(buffer);
    }
    func_ptr = dll_p->handler;

    /*
     * OK, special case.  If the handler is define as taking no arguments
     * then pass the address of the arguments on the 16-bit stack to the
     * handler.  It will just ignore the pointer if it really takes no
     * arguments.  This allows us to write slightly faster library routines
     * if we choose.
     */
    if (dll_p->n_args == 0)
	return (*func_ptr)(arg_ptr);

    /*
     * Getting this far means we need to convert the 16-bit argument stack.
     */
    for (i = 0; i < dll_p->n_args; i++)
    {
	short *sp;
	int *ip;
	
	offset = dll_p->args[i].dst_arg;

	switch (dll_p->args[i].src_type)
	{
	  case DLL_ARGTYPE_SIGNEDWORD:
	    sp = (short *) ((char *) arg_ptr + offset);
	    arg_table[i] = *sp;
	    break;
	    
	  case DLL_ARGTYPE_WORD:
	    sp = (short *) ((char *) arg_ptr + offset);
	    arg_table[i] = (int) *sp & 0xffff;
	    break;
	    
	  case DLL_ARGTYPE_LONG:
	  case DLL_ARGTYPE_FARPTR:
	    ip = (int *) ((char *) arg_ptr + offset);
	    arg_table[i] = *ip;
	    break;
	}
    }

    /*
     * Call the handler
     */
    return (*func_ptr)(arg_table[0], arg_table[1], arg_table[2], 
		       arg_table[3], arg_table[4], arg_table[5], 
		       arg_table[6], arg_table[7], arg_table[8], 
		       arg_table[9], arg_table[10], arg_table[11],
		       arg_table[12], arg_table[13], arg_table[14], 
		       arg_table[15]);
}

/**********************************************************************
 *					FindDLLTable
 */
struct  dll_table_entry_s *
FindDLLTable(char *dll_name)
{
    int i;

    for (i = 0; i < 4; i++)
	if (strcmp(dll_builtin_table[i].dll_name, dll_name) == 0)
	    return dll_builtin_table[i].dll_table;
    
    return NULL;
}

/**********************************************************************
 *					FindOrdinalFromName
 */
int
FindOrdinalFromName(struct dll_table_entry_s *dll_table, char *func_name)
{
    int i, limit;

    for (i = 0; i < 4; i++)
	if (dll_table == dll_builtin_table[i].dll_table)
	    break;
    
    if (i == 4)
	return 0;

    limit = dll_builtin_table[i].dll_table_length;
    for (i = 0; i < limit; i++)
	if (strcasecmp(dll_table[i].export_name, func_name) == 0)
	    return i;
    
    return 0;
}
