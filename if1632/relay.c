/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>

#include "dlls.h"
#include "global.h"
#include "module.h"
#include "registers.h"
#include "stackframe.h"
#include "stddebug.h"
/* #define DEBUG_RELAY */
#include "debug.h"

#if 0
/* Make make_debug think these were really used */
dprintf_relay
#endif

#define DLL_ENTRY(name,flags) \
  { #name, name##_Code_Start, name##_Data_Start, \
    name##_Module_Start, name##_Module_End, (flags) }

BUILTIN_DLL dll_builtin_table[] =
{
    DLL_ENTRY( KERNEL,   0),
    DLL_ENTRY( USER,     0),
    DLL_ENTRY( GDI,      0),
    DLL_ENTRY( WIN87EM,  DLL_FLAG_NOT_USED),
    DLL_ENTRY( SHELL,    0),
    DLL_ENTRY( SOUND,    0),
    DLL_ENTRY( KEYBOARD, 0),
    DLL_ENTRY( WINSOCK,  0),
    DLL_ENTRY( STRESS,   0),
    DLL_ENTRY( MMSYSTEM, 0),
    DLL_ENTRY( SYSTEM,   0),
    DLL_ENTRY( TOOLHELP, 0),
    DLL_ENTRY( MOUSE,    0),
    DLL_ENTRY( COMMDLG,  DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLE2,     DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLE2CONV, DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLE2DISP, DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLE2NLS,  DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLE2PROX, DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLECLI,   DLL_FLAG_NOT_USED),
    DLL_ENTRY( OLESVR,   DLL_FLAG_NOT_USED),
    DLL_ENTRY( COMPOBJ,  DLL_FLAG_NOT_USED),
    DLL_ENTRY( STORAGE,  DLL_FLAG_NOT_USED),
    DLL_ENTRY( WINPROCS, 0),
    DLL_ENTRY( DDEML,    DLL_FLAG_NOT_USED),
    DLL_ENTRY( LZEXPAND, 0),
    { NULL, }  /* Last entry */
};

  /* Saved 16-bit stack */
WORD IF1632_Saved16_ss = 0;
WORD IF1632_Saved16_sp = 0;

  /* Saved 32-bit stack */
DWORD IF1632_Saved32_esp = 0;
SEGPTR IF1632_Stack32_base = 0;
DWORD IF1632_Original32_esp = 0;

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

    codesel = GLOBAL_CreateBlock( GMEM_FIXED, (void *)CALL16_Start,
                                   (int)CALL16_End - (int)CALL16_Start,
                                   0, TRUE, TRUE, FALSE, NULL );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CALL16_RetAddr_word = MAKELONG( (int)CALL16_Ret_word - (int)CALL16_Start,
                                    codesel );
    CALL16_RetAddr_long = MAKELONG( (int)CALL16_Ret_long - (int)CALL16_Start,
                                    codesel );

    return TRUE;
}


/***********************************************************************
 *           RELAY_DebugCall32
 */
void RELAY_DebugCall32( int func_type, char *args,
                        void *entry_point, int args32 )
{
    STACK16FRAME *frame;
    struct dll_table_s *table;
    char *args16, *name;
    int i;

    if (!debugging_relay) return;

    frame = CURRENT_STACK16;
    table = &dll_builtin_table[frame->dll_id-1];
    name  = MODULE_GetEntryPointName( table->hModule, frame->ordinal_number );
    printf( "Call %s.%d: %.*s(",
            table->name, frame->ordinal_number, *name, name + 1 );

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

    if (func_type == 2)  /* register function */
    {
        struct sigcontext_struct *context = (struct sigcontext_struct *)&args32;
        printf( "     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                ES_reg(context), EFL_reg(context) );
    }
}


/***********************************************************************
 *           RELAY_DebugReturn
 */
void RELAY_DebugReturn( int func_type, int ret_val, int args32 )
{
    STACK16FRAME *frame;
    struct dll_table_s *table;
    char *name;

    if (*(DWORD *)PTR_SEG_TO_LIN(IF1632_Stack32_base) != 0xDEADBEEF)
    {
	fprintf(stderr, "Wine wrote past the end of the 32 bit stack. Please report this.\n");
        exit(1);  /* There's probably no point in going on */
    }
    if (!debugging_relay) return;

    frame = CURRENT_STACK16;
    table = &dll_builtin_table[frame->dll_id-1];
    name  = MODULE_GetEntryPointName( table->hModule, frame->ordinal_number );
    printf( "Ret  %s.%d: %.*s() ",
            table->name, frame->ordinal_number, *name, name + 1 );
    switch(func_type)
    {
    case 0: /* long */
        printf( "retval=0x%08x ret=%04x:%04x ds=%04x\n",
                ret_val, frame->cs, frame->ip, frame->ds );
        break;
    case 1: /* word */
        printf( "retval=0x%04x ret=%04x:%04x ds=%04x\n",
                ret_val & 0xffff, frame->cs, frame->ip, frame->ds );
        break;
    case 2: /* regs */
        printf( "retval=none ret=%04x:%04x ds=%04x\n",
                frame->cs, frame->ip, frame->ds );
        {
            struct sigcontext_struct *context = (struct sigcontext_struct *)&args32;
            printf( "     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                    AX_reg(context), BX_reg(context), CX_reg(context),
                    DX_reg(context), SI_reg(context), DI_reg(context),
                    ES_reg(context), EFL_reg(context) );
        }
        break;
    }
}


/***********************************************************************
 *           RELAY_Unimplemented16
 *
 * This function is called for unimplemented 16-bit entry points (declared
 * as 'stub' in the spec file).
 */
void RELAY_Unimplemented16(void)
{
    STACK16FRAME *frame = CURRENT_STACK16;
    struct dll_table_s *table = &dll_builtin_table[frame->dll_id-1];
    char *name = MODULE_GetEntryPointName( table->hModule, frame->ordinal_number );

    fprintf( stderr, "No handler for routine %s.%d (%.*s)\n",
             table->name, frame->ordinal_number, *name, name + 1 );
    exit(1);
}


/***********************************************************************
 *           RELAY_Unimplemented32
 *
 * This function is called for unimplemented 32-bit entry points (declared
 * as 'stub' in the spec file).
 * (The args are the same than for RELAY_DebugStdcall).
 */
void RELAY_Unimplemented32( int nb_args, void *entry_point,
                            const char *func_name )
{
    fprintf( stderr, "No handler for Win32 routine %s\n", func_name );
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
    while (nbargs--) printf( ",0x%04x", *stack++ );
    printf( ")\n" );
}


/***********************************************************************
 *           RELAY_DebugStdcall
 */
void RELAY_DebugStdcall( int nb_args, void *entry_point, const char *func_name,
                         int ebp, int ret_addr, int arg1 )
{
    int  *parg;
    if (!debugging_relay) return;
    printf( "Call %s(", func_name );
    for (parg = &arg1; nb_args; parg++, nb_args--)
    {
        printf( "%08x", *parg );
        if (nb_args > 1) printf( "," );
    }
    printf( ") ret=%08x\n", ret_addr );
}


/***********************************************************************
 *           RELAY_DebugStdcallRet
 */
void RELAY_DebugStdcallRet( int ret_val, void *entry_point,
                            const char *func_name, int ebp, int ret_addr )
{
    if (!debugging_relay) return;
    printf( "Ret  %s() retval=0x%08x ret=%08x\n",
            func_name, ret_val, ret_addr );
}
