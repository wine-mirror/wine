/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>

#include "global.h"
#include "module.h"
#include "registers.h"
#include "stackframe.h"
#include "task.h"
#include "stddebug.h"
/* #define DEBUG_RELAY */
#include "debug.h"

#if 0
/* Make make_debug think these were really used */
dprintf_relay
#endif


/***********************************************************************
 *           RELAY_Init
 */
BOOL32 RELAY_Init(void)
{
    WORD codesel;

      /* Allocate the code selector for CallTo16 routines */

    extern void CALLTO16_Start(), CALLTO16_End();
    extern void CALLTO16_Ret_word(), CALLTO16_Ret_long();
    extern DWORD CALLTO16_RetAddr_word, CALLTO16_RetAddr_long;

    codesel = GLOBAL_CreateBlock( GMEM_FIXED, (void *)CALLTO16_Start,
                                   (int)CALLTO16_End - (int)CALLTO16_Start,
                                   0, TRUE, TRUE, FALSE, NULL );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CALLTO16_RetAddr_word=MAKELONG( (int)CALLTO16_Ret_word-(int)CALLTO16_Start,
                                    codesel );
    CALLTO16_RetAddr_long=MAKELONG( (int)CALLTO16_Ret_long-(int)CALLTO16_Start,
                                    codesel );

    return TRUE;
}


/***********************************************************************
 *           RELAY_DebugCallFrom16
 */
void RELAY_DebugCallFrom16( int func_type, char *args,
                            void *entry_point, SIGCONTEXT *context )
{
    STACK16FRAME *frame;
    WORD ordinal;
    char *args16;
    int i;

    if (!debugging_relay) return;

    frame = CURRENT_STACK16;
    printf( "Call %s(", BUILTIN_GetEntryPoint16( frame->entry_cs,
                                                 frame->entry_ip,
                                                 &ordinal ));
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
        printf( "     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                ES_reg(context), EFL_reg(context) );
}


/***********************************************************************
 *           RELAY_DebugCallFrom16Ret
 */
void RELAY_DebugCallFrom16Ret( int func_type, int ret_val, SIGCONTEXT *context)
{
    STACK16FRAME *frame;
    WORD ordinal;

    if (*(DWORD *)PTR_SEG_TO_LIN(IF1632_Stack32_base) != 0xDEADBEEF)
    {
	fprintf(stderr, "Wine wrote past the end of the 32 bit stack. Please report this.\n");
        exit(1);  /* There's probably no point in going on */
    }
    if (!debugging_relay) return;

    frame = CURRENT_STACK16;
    printf( "Ret  %s() ", BUILTIN_GetEntryPoint16( frame->entry_cs,
                                                   frame->entry_ip,
                                                   &ordinal ));
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
        printf( "     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                ES_reg(context), EFL_reg(context) );
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
    WORD ordinal;
    STACK16FRAME *frame = CURRENT_STACK16;
    fprintf(stderr,"No handler for Win16 routine %s (called from %04x:%04x)\n",
            BUILTIN_GetEntryPoint16(frame->entry_cs,frame->entry_ip,&ordinal),
            frame->cs, frame->ip );
    TASK_KillCurrentTask(1);
}


/***********************************************************************
 *           RELAY_Unimplemented32
 *
 * This function is called for unimplemented 32-bit entry points (declared
 * as 'stub' in the spec file).
 * (The args are the same than for RELAY_DebugCallFrom32).
 */
void RELAY_Unimplemented32( int nb_args, void *relay_addr,
                            void *entry_point, int ebp, int ret_addr )
{
    fprintf( stderr, "No handler for Win32 routine %s (called from %08x)\n",
             BUILTIN_GetEntryPoint32( relay_addr ), ret_addr );
    TASK_KillCurrentTask(1);
}


/***********************************************************************
 *           RELAY_DebugCallTo16
 *
 * 'stack' points to the called function address on the 32-bit stack.
 * Stack layout:
 *  ...        ...
 * (stack+12)  arg2
 * (stack+8)   arg1
 * (stack+4)   16-bit ds
 * (stack)     func to call
 */
void RELAY_DebugCallTo16( int* stack, int nbargs )
{
    if (!debugging_relay) return;

    printf( "CallTo16(func=%04x:%04x,ds=%04x",
            HIWORD(stack[0]), LOWORD(stack[0]), LOWORD(stack[1]) );
    stack += 2;
    while (nbargs--) printf( ",0x%04x", *stack++ );
    printf( ")\n" );
}


/***********************************************************************
 *           RELAY_DebugCallFrom32
 */
void RELAY_DebugCallFrom32( int nb_args, void *relay_addr,
                            void *entry_point, int ebp, int ret_addr, int arg1)
{
    int *parg;

    if (!debugging_relay) return;
    printf( "Call %s(", BUILTIN_GetEntryPoint32( relay_addr ));
    for (parg = &arg1; nb_args; parg++, nb_args--)
    {
        printf( "%08x", *parg );
        if (nb_args > 1) printf( "," );
    }
    printf( ") ret=%08x\n", ret_addr );
}


/***********************************************************************
 *           RELAY_DebugCallFrom32Ret
 */
void RELAY_DebugCallFrom32Ret( int ret_val, void *relay_addr,
                               void *entry_point, int ebp, int ret_addr )
{
    if (!debugging_relay) return;
    printf( "Ret  %s() retval=0x%08x ret=%08x\n",
            BUILTIN_GetEntryPoint32( relay_addr ), ret_val, ret_addr );
}


/***********************************************************************
 *           RELAY_DebugCallTo32
 */
void RELAY_DebugCallTo32( unsigned int func, int nbargs, unsigned int arg1  )
{
    unsigned int *argptr;

    if (!debugging_relay) return;

    printf( "CallTo32(func=%08x", func );
    for (argptr = &arg1; nbargs; nbargs--, argptr++)
        printf( ",%08x", *argptr );
    printf( ")\n" );
}


/**********************************************************************
 *	     Catch    (KERNEL.55)
 */
INT16 Catch( LPCATCHBUF lpbuf )
{
    STACK16FRAME *pFrame = CURRENT_STACK16;

    /* Note: we don't save the current ss, as the catch buffer is */
    /* only 9 words long. Hopefully no one will have the silly    */
    /* idea to change the current stack before calling Throw()... */

    lpbuf[0] = IF1632_Saved16_sp;
    lpbuf[1] = LOWORD(IF1632_Saved32_esp);
    lpbuf[2] = HIWORD(IF1632_Saved32_esp);
    lpbuf[3] = pFrame->saved_ss;
    lpbuf[4] = pFrame->saved_sp;
    lpbuf[5] = pFrame->ds;
    lpbuf[6] = pFrame->bp;
    lpbuf[7] = pFrame->ip;
    lpbuf[8] = pFrame->cs;
    return 0;
}


/**********************************************************************
 *	     Throw    (KERNEL.56)
 */
INT16 Throw( LPCATCHBUF lpbuf, INT16 retval )
{
    STACK16FRAME *pFrame;

    IF1632_Saved16_sp  = lpbuf[0] - sizeof(WORD);
    IF1632_Saved32_esp = MAKELONG( lpbuf[1], lpbuf[2] );
    pFrame = CURRENT_STACK16;
    pFrame->saved_ss   = lpbuf[3];
    pFrame->saved_sp   = lpbuf[4];
    pFrame->ds         = lpbuf[5];
    pFrame->bp         = lpbuf[6];
    pFrame->ip         = lpbuf[7];
    pFrame->cs         = lpbuf[8];
    pFrame->es         = 0;
    return retval;
}
