/*
static char RCSId[] = "$Id: relay.c,v 1.2 1993/07/04 04:04:21 root Exp root $";
static char Copyright[] = "Copyright  Robert J. Amstadt, 1993";
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef linux
#include <linux/unistd.h>
#include <linux/head.h>
#include <linux/ldt.h>
#endif

#include "ldt.h"

#include "neexe.h"
#include "prototypes.h"
#include "dlls.h"
#include "options.h"
#include "stackframe.h"
#include "stddebug.h"
/* #define DEBUG_RELAY */
/* #define DEBUG_STACK */
#include "debug.h"

#if 0
/* Make make_debug think these were really used */
dprintf_relay
dprintf_stack
#endif

#ifdef WINELIB
#define WineLibSkip(x) 0
#else
#define WineLibSkip(x) x
#endif

struct dll_name_table_entry_s dll_builtin_table[N_BUILTINS] =
{
    { "KERNEL",  WineLibSkip(KERNEL_table), 	410, 1, 1 },
    { "USER",    WineLibSkip(USER_table), 	540, 2, 1 },
    { "GDI",     WineLibSkip(GDI_table), 	490, 3, 1 },
    { "WIN87EM", WineLibSkip(WIN87EM_table),  10, 4, 1 },
    { "SHELL",   WineLibSkip(SHELL_table),   103, 5, 1 },
    { "SOUND",   WineLibSkip(SOUND_table),    20, 6, 1 },
    { "KEYBOARD",WineLibSkip(KEYBOARD_table),137, 7, 1 },
    { "WINSOCK", WineLibSkip(WINSOCK_table), 155, 8, 1 },
    { "STRESS",  WineLibSkip(STRESS_table),   15, 9, 1},
    { "MMSYSTEM",WineLibSkip(MMSYSTEM_table),1226,10, 1},
    { "SYSTEM",  WineLibSkip(SYSTEM_table),   20 ,11, 1},
    { "TOOLHELP",WineLibSkip(TOOLHELP_table), 83, 12, 1},
    { "MOUSE",   WineLibSkip(MOUSE_table),     8, 13, 1},
    { "COMMDLG", WineLibSkip(COMMDLG_table),  31, 14, 1},
    { "OLE2",    WineLibSkip(OLE2_table),     31, 15, 1},
    { "OLE2CONV",WineLibSkip(OLE2CONV_table), 31, 16, 1},
    { "OLE2DISP",WineLibSkip(OLE2DISP_table), 31, 17, 1},
    { "OLE2NLS", WineLibSkip(OLE2NLS_table),  31, 18, 1},
    { "OLE2PROX",WineLibSkip(OLE2PROX_table), 31, 19, 1},
    { "OLECLI",  WineLibSkip(OLECLI_table),   31, 20, 1},
    { "OLESVR",  WineLibSkip(OLESVR_table),   31, 21, 1},
    { "COMPOBJ", WineLibSkip(COMPOBJ_table),  31, 22, 1},
    { "STORAGE", WineLibSkip(STORAGE_table),  31, 23, 1}
};
/* don't forget to increase N_BUILTINS in dll.h if you add a dll */

/* the argument conversion tables for each dll */
struct dll_conversions {
	unsigned short *dst_args;   /*  Offsets to arguments on stack */
	unsigned char *src_types;   /* Argument types              */
} dll_conversion_table[N_BUILTINS]= {
  { KERNEL_offsets,   KERNEL_types },   /* KERNEL     */
  { USER_offsets,     USER_types },     /* USER       */
  { GDI_offsets,      GDI_types },      /* GDI        */
  { WIN87EM_offsets,  WIN87EM_types },  /* WIN87EM    */
  { SHELL_offsets,    SHELL_types },    /* SHELL      */
  { SOUND_offsets,    SOUND_types },    /* SOUND      */
  { KEYBOARD_offsets, KEYBOARD_types }, /* KEYBOARD   */
  { WINSOCK_offsets,  WINSOCK_types },  /* WINSOCK    */
  { STRESS_offsets,   STRESS_types },   /* STRESS,     */
  { MMSYSTEM_offsets, MMSYSTEM_types }, /* MMSYSTEM   */
  { SYSTEM_offsets,   SYSTEM_types },   /* SYSTEM     */
  { TOOLHELP_offsets, TOOLHELP_types }, /* TOOLHELP   */
  { MOUSE_offsets,    MOUSE_types },    /* MOUSE      */
  { COMMDLG_offsets,  COMMDLG_types },  /* EMUCOMMDLG */
  { OLE2_offsets,     OLE2_types },     /* OLE2       */
  { OLE2CONV_offsets, OLE2CONV_types }, /* OLE2CONV   */
  { OLE2DISP_offsets, OLE2DISP_types }, /* OLE2DISP   */
  { OLE2NLS_offsets,  OLE2NLS_types },  /* OLE2NLS    */
  { OLE2DISP_offsets, OLE2DISP_types }, /* OLE2PROX   */
  { OLECLI_offsets,   OLECLI_types },   /* OLE2CLI    */
  { OLESVR_offsets,   OLESVR_types },   /* OLE2CLI    */
  { COMPOBJ_offsets,  COMPOBJ_types },  /* COMPOBJ    */
  { STORAGE_offsets,  STORAGE_types }   /* STORAGE    */
};


#ifndef WINELIB

extern unsigned short IF1632_Saved16_sp;
extern unsigned short IF1632_Saved16_bp;
extern unsigned short IF1632_Saved16_ss;

void RelayDebug( unsigned int func_num )
{
    unsigned int dll_id, ordinal;

    if (debugging_relay)
    {
        dll_id  = ((func_num >> 16) & 0xffff) - 1;
        ordinal = func_num & 0xffff;
        printf( "Calling %s.%d\n",
               dll_builtin_table[dll_id].dll_table[ordinal].export_name,
               ordinal );
    }
}


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
    STACK16FRAME *pStack16Frame;
    unsigned int offset;
    unsigned int dll_id;
    unsigned int ordinal;
    int arg_table[DLL_MAX_ARGS];
    void *arg_ptr;
    int (*func_ptr)();
    int i;
    int ret_val;
    int conv_ref;
    unsigned char *type_conv;
    unsigned short *offset_conv;
    STACK16FRAME stackFrameCopy;

    /*
     * Determine address of arguments.
     */
    pStack16Frame = (STACK16FRAME *) PTR_SEG_TO_LIN(seg_off);
    arg_ptr = (void *)pStack16Frame->args;

    /*
     * Extract the DLL number and ordinal number.
     */
    dll_id  = ((func_num >> 16) & 0xffff) - 1;
    ordinal = func_num & 0xffff;
    dll_p   = &dll_builtin_table[dll_id].dll_table[ordinal];

    dprintf_relay( stddeb, "Call %s (%s.%d), stack=%04x:%04x ret=%04x:%04x ds=%04x bp=%04x args=%d\n",
                  dll_p->export_name,
                  dll_builtin_table[dll_id].dll_name, ordinal,
                  seg_off >> 16, seg_off & 0xffff,
                  pStack16Frame->cs, pStack16Frame->ip,
                  pStack16Frame->ds, pStack16Frame->bp,
                  pStack16Frame->arg_length );

    if(debugging_stack)
    {
        unsigned short *stack_p = (unsigned short *) pStack16Frame;
        /* FIXME: Is there an end-of-stack-pointer somewhere ? */
        int n = min(24, (0x10000 - (seg_off & 0xffff)) / sizeof(*stack_p));
        for (i = 0; i < n; i++, stack_p++)
        {
            printf("%04x ", *stack_p);
            if ((i & 7) == 7)
                printf("\n");
        }
        printf("\n");
    }

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
    {
	ret_val = (*func_ptr)(arg_ptr);
	dprintf_relay( stddeb, "Returning %08x from %s (%s.%d) ds=%04x\n",
                       ret_val, dll_p->export_name,
                       dll_builtin_table[dll_id].dll_name, ordinal,
                       pStack16Frame->ds );
	return ret_val;
    }

    /*
     * Getting this far means we need to convert the 16-bit argument stack.
     */
    conv_ref= dll_p->conv_reference;
    type_conv= dll_conversion_table[dll_id].src_types + conv_ref;
    offset_conv= dll_conversion_table[dll_id].dst_args + conv_ref;
    for (i = 0; i < dll_p->n_args; i++,type_conv++,offset_conv++)
    {
	short *sp;
	int *ip;
	
	offset = *offset_conv;

	switch (*type_conv)
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
	    ip = (int *) ((char *) arg_ptr + offset);
	    arg_table[i] = *ip;
	    break;

	  case DLL_ARGTYPE_FARPTR:
	    ip = (int *) ((char *) arg_ptr + offset);
            arg_table[i] = (unsigned int) PTR_SEG_TO_LIN( *ip );
	    break;
	}
    }

    if (debugging_relay)
        memcpy( &stackFrameCopy, pStack16Frame, sizeof(stackFrameCopy) );

    /*
     * Call the handler
     */
    ret_val = (*func_ptr)(arg_table[0], arg_table[1], arg_table[2], 
			  arg_table[3], arg_table[4], arg_table[5], 
			  arg_table[6], arg_table[7], arg_table[8], 
			  arg_table[9], arg_table[10], arg_table[11],
			  arg_table[12], arg_table[13], arg_table[14], 
			  arg_table[15]);

    if (debugging_relay)
    {
        if (memcmp( &stackFrameCopy, pStack16Frame, sizeof(stackFrameCopy) ))
        {
            printf( "**** 16-bit stack corrupted!\n" );
        }
	printf("Returning %08x from %s (%s.%d) ds=%04x\n",
	       ret_val,
	       dll_p->export_name,
	       dll_builtin_table[dll_id].dll_name, ordinal,
               pStack16Frame->ds );
    }

    return ret_val;
}
#endif

/**********************************************************************
 *					FindDLLTable
 */
struct  dll_table_entry_s *
FindDLLTable(char *dll_name)
{
    int i;

    for (i = 0; i < N_BUILTINS; i++)
	if (strcasecmp(dll_builtin_table[i].dll_name, dll_name) == 0
	  && dll_builtin_table[i].dll_is_used)
#ifdef WINELIB
	    return dll_builtin_table[i].dll_number;
#else
	    return dll_builtin_table[i].dll_table;
#endif
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

#ifndef WINELIB
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
				    printf("%s.%d not implemented\n",
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
	    if (used)
		    printf("%s: %d of %d (%3.1f %%)\n", dll_builtin_table[i].dll_name, implemented, used, perc);
    };
	perc = timplemented * 100.00 / tused;
	printf("TOTAL: %d of %d winapi functions implemented (%3.1f %%)\n",timplemented, tused, perc);
}
#endif /* WINESTAT */
#endif /* !WINELIB */
