static char RCSId[] = "$Id: relay.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef linux
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>
#include <linux/segment.h>
#endif
#include <errno.h>
#include "neexe.h"
#include "segmem.h"
#include "prototypes.h"
#include "dlls.h"

#define N_BUILTINS	8

struct dll_name_table_entry_s dll_builtin_table[N_BUILTINS] =
{
    { "KERNEL",  KERNEL_table, 	410, 1 },
    { "USER",    USER_table, 	540, 2 },
    { "GDI",     GDI_table, 	490, 3 },
    { "UNIXLIB", UNIXLIB_table,  10, 4 },
    { "WIN87EM", WIN87EM_table,  10, 5 },
    { "SHELL",   SHELL_table,   256, 6 },
    { "SOUND",   SOUND_table,    20, 7 },
    { "KEYBOARD",KEYBOARD_table,137, 8 },
};

unsigned short *Stack16Frame;

extern unsigned long  IF1632_Saved16_esp;
extern unsigned long  IF1632_Saved16_ebp;
extern unsigned short IF1632_Saved16_ss;

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
    int ret_val;
    
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

#ifdef DEBUG_RELAY
    {
	unsigned int *ret_addr;
	unsigned short *stack_p;
	
	ret_addr = (unsigned int *) ((char *) seg_off + 0x14);
	printf("Calling %s (%s.%d), 16-bit stack at %04x:%04x, ",
	       dll_p->export_name,
	       dll_builtin_table[dll_id].dll_name, ordinal,
	       seg_off >> 16, seg_off & 0xffff);
	printf("return to %08x\n", *ret_addr);
	printf("  ESP %08x, EBP %08x, SS %04x\n", 
	       IF1632_Saved16_esp, IF1632_Saved16_ebp,
	       IF1632_Saved16_ss);

	if (strcmp("GetMessage", dll_p->export_name) == 0 &&
	    seg_off == 0x00972526 &&
	    *ret_addr == 0x004700cd &&
	    IF1632_Saved16_esp == 0x2526 &&
	    IF1632_Saved16_ebp == 0x2534 &&
	    IF1632_Saved16_ss  == 0x0097)
	    printf("ACK!!\n");

#if 0
	IF1632_Saved16_esp &= 0x0000ffff;
	IF1632_Saved16_ebp &= 0x0000ffff;
#endif

#ifdef DEBUG_STACK
	stack_p = (unsigned short *) seg_off;
	for (i = 0; i < 24; i++, stack_p++)
	{
            printf("%04x ", *stack_p);
	    if ((i & 7) == 7)
		printf("\n");
	}
	printf("\n");
#endif /* DEBUG_STACK */
    }
#endif /* DEBUG_RELAY */

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
    ret_val = (*func_ptr)(arg_table[0], arg_table[1], arg_table[2], 
			  arg_table[3], arg_table[4], arg_table[5], 
			  arg_table[6], arg_table[7], arg_table[8], 
			  arg_table[9], arg_table[10], arg_table[11],
			  arg_table[12], arg_table[13], arg_table[14], 
			  arg_table[15]);

#ifdef DEBUG_RELAY
    printf("Returning %08.8x from %s (%s.%d)\n",
	   ret_val,
	   dll_p->export_name,
	   dll_builtin_table[dll_id].dll_name, ordinal);
#endif

    return ret_val;
}

/**********************************************************************
 *					FindDLLTable
 */
struct  dll_table_entry_s *
FindDLLTable(char *dll_name)
{
    int i;

    for (i = 0; i < N_BUILTINS; i++)
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

    for (i = 0; i < N_BUILTINS; i++)
	if (dll_table == dll_builtin_table[i].dll_table)
	    break;
    
    if (i == N_BUILTINS)
	return 0;

    limit = dll_builtin_table[i].dll_table_length;
    for (i = 0; i < limit; i++)
	if (strcasecmp(dll_table[i].export_name, func_name) == 0)
	    return i;
    
    return 0;
}
/**********************************************************************
 *					ReturnArg
 */
int
ReturnArg(int arg)
{
    return arg;
}

#ifdef WINESTAT
void winestat(){
	int i, j;
	double perc;
	int used, implemented;
	int tused, timplemented;
	struct dll_table_entry_s *table;

	tused = 0;
	timplemented = 0;
    for (i = 0; i < N_BUILTINS; i++) {
	    table = dll_builtin_table[i].dll_table;
	    used = 0;
	    implemented = 0;
	    for(j=0; j < dll_builtin_table[i].dll_table_length; j++) {
		    if(table[j].used){
			    used++;
			    if (table[j].handler) implemented++;
			    else 
				    printf("%s.%d\n",
					   dll_builtin_table[i].dll_name,
					   j);
		    };
	    };
	    tused += used;
	    timplemented += implemented;
	    if(used)
		    perc = implemented * 100.00 / used;
	    else
		    perc = 0.0;
	    printf("%s: %d %d %3.1f\n", dll_builtin_table[i].dll_name, implemented, used, perc);
    };
	perc = timplemented * 100.00 / tused;
	printf("TOTAL: %d %d %3.1f\n",timplemented, tused, perc);
}
#endif /* WINESTAT */
