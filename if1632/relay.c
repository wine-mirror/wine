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
#include "selectors.h"
#include "stackframe.h"
#include "wine.h"
#include "stddebug.h"
/* #define DEBUG_RELAY */
#include "debug.h"

#if 0
/* Make make_debug think these were really used */
dprintf_relay
#endif

#ifdef WINELIB
#define WineLibSkip(x) 0
#else
#define WineLibSkip(x) x
#endif

struct dll_name_table_entry_s dll_builtin_table[N_BUILTINS] =
{
    { "KERNEL",   WineLibSkip(&KERNEL_table), 1 },
    { "USER",     WineLibSkip(&USER_table), 1 },
    { "GDI",      WineLibSkip(&GDI_table), 1 },
    { "WIN87EM",  WineLibSkip(&WIN87EM_table), 1 },
    { "SHELL",    WineLibSkip(&SHELL_table), 1 },
    { "SOUND",    WineLibSkip(&SOUND_table), 1 },
    { "KEYBOARD", WineLibSkip(&KEYBOARD_table), 1 },
    { "WINSOCK",  WineLibSkip(&WINSOCK_table), 1 },
    { "STRESS",   WineLibSkip(&STRESS_table), 1 },
    { "MMSYSTEM", WineLibSkip(&MMSYSTEM_table), 1 },
    { "SYSTEM",   WineLibSkip(&SYSTEM_table), 1 },
    { "TOOLHELP", WineLibSkip(&TOOLHELP_table), 1 },
    { "MOUSE",    WineLibSkip(&MOUSE_table), 1 },
    { "COMMDLG",  WineLibSkip(&COMMDLG_table), 1 },
    { "OLE2",     WineLibSkip(&OLE2_table), 1 },
    { "OLE2CONV", WineLibSkip(&OLE2CONV_table), 1 },
    { "OLE2DISP", WineLibSkip(&OLE2DISP_table), 1 },
    { "OLE2NLS",  WineLibSkip(&OLE2NLS_table), 1 },
    { "OLE2PROX", WineLibSkip(&OLE2PROX_table), 1 },
    { "OLECLI",   WineLibSkip(&OLECLI_table), 1 },
    { "OLESVR",   WineLibSkip(&OLESVR_table), 1 },
    { "COMPOBJ",  WineLibSkip(&COMPOBJ_table), 1 },
    { "STORAGE",  WineLibSkip(&STORAGE_table), 1 },
    { "WINPROCS", WineLibSkip(&WINPROCS_table), 1 },
    { "DDEML",    WineLibSkip(&DDEML_table), 1 }
    
};
/* don't forget to increase N_BUILTINS in dlls.h if you add a dll */

  /* Saved 16-bit stack */
WORD IF1632_Saved16_ss = 0;
WORD IF1632_Saved16_sp = 0;

  /* Saved 32-bit stack */
DWORD IF1632_Saved32_esp = 0;


/***********************************************************************
 *           RELAY_Init
 */
BOOL RELAY_Init(void)
{
    WORD codesel;

      /* Allocate the code selector for CallTo16 routines */

    extern void CALL16_Start(), CALL16_End();
    extern void CALL16_Ret_word(), CALL16_Ret_long();
    extern DWORD CALL16_RetAddr_word, CALL16_RetAddr_long;

    codesel = SELECTOR_AllocBlock( (void *)CALL16_Start,
                                   (int)CALL16_End - (int)CALL16_Start,
                                   SEGMENT_CODE, TRUE, FALSE );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CALL16_RetAddr_word = MAKELONG( (int)CALL16_Ret_word - (int)CALL16_Start,
                                    codesel );
    CALL16_RetAddr_long = MAKELONG( (int)CALL16_Ret_long - (int)CALL16_Start,
                                    codesel );

    return TRUE;
}


#ifndef WINELIB

void RELAY_DebugCall32( char *args )
{
    STACK16FRAME *frame;
    char *args16;
    int i;

    if (!debugging_relay) return;

    frame = CURRENT_STACK16;
    printf( "Call %s.%d: %s(",
            dll_builtin_table[frame->dll_id-1].dll_name,
            frame->ordinal_number,
            dll_builtin_table[frame->dll_id-1].table->dll_table[frame->ordinal_number].export_name );
    args16 = (char *)frame->args;
    for (i = 0; i < strlen(args); i++)
    {
        switch(args[i])
        {
        case 'w':
        case 's':
            args16 += 2;
            break;
        case 'l':
        case 'p':
            args16 += 4;
            break;
        }
    }

    while (*args)
    {
        switch(*args)
        {
        case 'w':
        case 's':
            args16 -= 2;
            printf( "0x%04x", *(WORD *)args16 );
            break;
        case 'l':
            args16 -= 4;
            printf( "0x%08x", *(int *)args16 );
            break;
        case 'p':
            args16 -= 4;
            printf( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
            break;
        }
        args++;
        if (*args) printf( "," );
    }
    printf( ") ret=%04x:%04x ds=%04x\n", frame->cs, frame->ip, frame->ds );
}


void RELAY_DebugReturn( int short_ret, int ret_val )
{
    STACK16FRAME *frame;

    if (!debugging_relay) return;

    frame = CURRENT_STACK16;
    printf( "Ret  %s.%d: %s() ",
            dll_builtin_table[frame->dll_id-1].dll_name,
            frame->ordinal_number,
            dll_builtin_table[frame->dll_id-1].table->dll_table[frame->ordinal_number].export_name );
    if (short_ret) printf( "retval=0x%04x\n", ret_val & 0xffff );
    else printf( "retval=0x%08x\n", ret_val );
}


void RELAY_Unimplemented(void)
{
    STACK16FRAME *frame = CURRENT_STACK16;

    fprintf( stderr, "No handler for routine %s.%d (%s)\n",
             dll_builtin_table[frame->dll_id-1].dll_name,
             frame->ordinal_number,
             dll_builtin_table[frame->dll_id-1].table->dll_table[frame->ordinal_number].export_name );
    exit(1);
}


/***********************************************************************
 *           RELAY_DebugCall16
 *
 * 'stack' points to the called function address on the 32-bit stack.
 * Stack layout:
 *  ...        ...
 * (stack+12)  arg2
 * (stack+8)   arg1
 * (stack+4)   16-bit ds
 * (stack)     func to call
 */
void RELAY_DebugCall16( int* stack, int nbargs )
{
    if (!debugging_relay) return;

    printf( "CallTo16(func=%04x:%04x,ds=%04x",
            HIWORD(stack[0]), LOWORD(stack[0]), LOWORD(stack[1]) );
    stack += 2;
    while (nbargs--) printf( ",0x%x", *stack++ );
    printf( ")\n" );
}

#endif  /* WINELIB */

/**********************************************************************
 *					FindDLLTable
 */
struct dll_table_s *
FindDLLTable(char *dll_name)
{
    int i;

    for (i = 0; i < N_BUILTINS; i++)
	if (strcasecmp(dll_builtin_table[i].dll_name, dll_name) == 0
	  && dll_builtin_table[i].dll_is_used)
	    return dll_builtin_table[i].table;
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
	if (dll_table == dll_builtin_table[i].table->dll_table)
	    break;
    
    if (i == N_BUILTINS)
	return 0;

    limit = dll_builtin_table[i].table->dll_table_length;
    for (i = 0; i < limit; i++)
	if (strcasecmp(dll_table[i].export_name, func_name) == 0)
	    return i;
    
    return 0;
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
	    table = dll_builtin_table[i].table->dll_table;
	    used = 0;
	    implemented = 0;
	    for(j=0; j < dll_builtin_table[i].table->dll_table_length; j++) {
		    if(table[j].used){
			    used++;
			    if (table[j].export_name[0]) implemented++;
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
