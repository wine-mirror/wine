/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <stdio.h>
#include "windows.h"
#include "winnt.h"
#include "global.h"
#include "module.h"
#include "stackframe.h"
#include "task.h"
#include "callback.h"
#include "xmalloc.h"
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
    extern BOOL32 THUNK_Init(void);

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

    /* Initialize thunking */

    return THUNK_Init();
}


/***********************************************************************
 *           RELAY_DebugCallFrom16
 */
void RELAY_DebugCallFrom16( int func_type, char *args,
                            void *entry_point, CONTEXT *context )
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
                (WORD)ES_reg(context), EFL_reg(context) );
}


/***********************************************************************
 *           RELAY_DebugCallFrom16Ret
 */
void RELAY_DebugCallFrom16Ret( int func_type, int ret_val, CONTEXT *context)
{
    STACK16FRAME *frame;
    WORD ordinal;

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
                (WORD)ES_reg(context), EFL_reg(context) );
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
 * (stack+8)   arg2
 * (stack+4)   arg1
 * (stack)     func to call
 */
void RELAY_DebugCallTo16( int* stack, int nb_args )
{
    if (!debugging_relay) return;

    if (nb_args == -1)  /* Register function */
    {
        CONTEXT *context = *(CONTEXT **)stack;
        printf( "CallTo16(func=%04lx:%04x,ds=%04lx)\n",
                CS_reg(context), IP_reg(context), DS_reg(context) );
        printf( "     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x ES=%04x\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                BP_reg(context), (WORD)ES_reg(context) );
    }
    else
    {
        printf( "CallTo16(func=%04x:%04x,ds=%04x",
                HIWORD(stack[0]), LOWORD(stack[0]), CURRENT_DS );
        stack++;
        while (nb_args--) printf( ",0x%04x", *stack++ );
        printf( ")\n" );
    }
}


/***********************************************************************
 *           RELAY_DebugCallFrom32
 *
 * 'stack' points to the saved ebp on the stack.
 * Stack layout:
 *  ...        ...
 * (stack+12)  arg2
 * (stack+8)   arg1
 * (stack+4)   ret addr
 * (stack)     ebp
 * (stack-4)   entry point
 * (stack-8)   relay addr
 */
void RELAY_DebugCallFrom32( int *stack, int nb_args )
{
    int *parg, i;

    if (!debugging_relay) return;
    printf( "Call %s(", BUILTIN_GetEntryPoint32( (void *)stack[-2] ));
    for (i = nb_args & 0x7fffffff, parg = &stack[2]; i; parg++, i--)
    {
        printf( "%08x", *parg );
        if (i > 1) printf( "," );
    }
    printf( ") ret=%08x\n", stack[1] );
    if (nb_args & 0x80000000)  /* Register function */
    {
        CONTEXT *context = (CONTEXT *)((BYTE *)stack - sizeof(CONTEXT) - 8);
        printf( " EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
                context->Eax, context->Ebx, context->Ecx, context->Edx,
                context->Esi, context->Edi );
        printf( " EBP=%08lx ESP=%08lx EIP=%08lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx EFL=%08lx\n",
                context->Ebp, context->Esp, context->Eip, context->SegDs,
                context->SegEs, context->SegFs, context->SegGs,
                context->EFlags );
    }
}


/***********************************************************************
 *           RELAY_DebugCallFrom32Ret
 *
 * 'stack' points to the saved ebp on the stack.
 * Stack layout:
 *  ...        ...
 * (stack+12)  arg2
 * (stack+8)   arg1
 * (stack+4)   ret addr
 * (stack)     ebp
 * (stack-4)   entry point
 * (stack-8)   relay addr
 */
void RELAY_DebugCallFrom32Ret( int *stack, int nb_args, int ret_val )
{
    if (!debugging_relay) return;
    printf( "Ret  %s() retval=%08x ret=%08x\n",
            BUILTIN_GetEntryPoint32( (void *)stack[-2] ), ret_val, stack[1] );
    if (nb_args & 0x80000000)  /* Register function */
    {
        CONTEXT *context = (CONTEXT *)((BYTE *)stack - sizeof(CONTEXT) - 8);
        printf( " EAX=%08lx EBX=%08lx ECX=%08lx EDX=%08lx ESI=%08lx EDI=%08lx\n",
                context->Eax, context->Ebx, context->Ecx, context->Edx,
                context->Esi, context->Edi );
        printf( " EBP=%08lx ESP=%08lx EIP=%08lx DS=%04lx ES=%04lx FS=%04lx GS=%04lx EFL=%08lx\n",
                context->Ebp, context->Esp, context->Eip, context->SegDs,
                context->SegEs, context->SegFs, context->SegGs,
                context->EFlags );
    }
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

    /* Windows uses:
     * lpbuf[0] = ip
     * lpbuf[1] = cs
     * lpbuf[2] = sp
     * lpbuf[3] = bp
     * lpbuf[4] = si
     * lpbuf[5] = di
     * lpbuf[6] = ds
     * lpbuf[7] = unused
     * lpbuf[8] = ss
     */
    /* FIXME: we need to save %si and %di */

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
    WORD es = CURRENT_STACK16->es;

    IF1632_Saved16_sp  = lpbuf[0] - sizeof(WORD);
    IF1632_Saved32_esp = MAKELONG( lpbuf[1], lpbuf[2] );
    pFrame = CURRENT_STACK16;
    pFrame->saved_ss   = lpbuf[3];
    pFrame->saved_sp   = lpbuf[4];
    pFrame->ds         = lpbuf[5];
    pFrame->bp         = lpbuf[6];
    pFrame->ip         = lpbuf[7];
    pFrame->cs         = lpbuf[8];
    pFrame->es         = es;
    if (debugging_relay)  /* Make sure we have a valid entry point address */
    {
        static FARPROC16 entryPoint = NULL;

        if (!entryPoint)  /* Get entry point for Throw() */
            entryPoint = MODULE_GetEntryPoint( GetModuleHandle16("KERNEL"),
                                               56 );
        pFrame->entry_cs = SELECTOROF(entryPoint);
        pFrame->entry_ip = OFFSETOF(entryPoint);
    }
    return retval;
}

/**********************************************************************
 *	     CallProc32W    (KERNEL.56)
 */
DWORD
WIN16_CallProc32W() {
	DWORD *win_stack = (DWORD *)CURRENT_STACK16->args;
	DWORD	nrofargs = win_stack[0];
	DWORD	argconvmask = win_stack[1];
	FARPROC32	proc32 = (FARPROC32)win_stack[2];
	DWORD	*args,ret;
	int	i;

	fprintf(stderr,"CallProc32W(%ld,%ld,%p,args[",nrofargs,argconvmask,proc32);
	args = (DWORD*)xmalloc(sizeof(DWORD)*nrofargs);
	for (i=nrofargs;i--;) {
		if (argconvmask & (1<<i)) {
			args[nrofargs-i] = (DWORD)PTR_SEG_TO_LIN(win_stack[3+i]);
			fprintf(stderr,"%08lx(%p),",win_stack[3+i],PTR_SEG_TO_LIN(win_stack[3+i]));
		} else {
			args[nrofargs-i] = win_stack[3+i];
			fprintf(stderr,"%ld,",win_stack[3+i]);
		}
	}
	fprintf(stderr,"]) - ");
	switch (nrofargs) {
	case 0: ret = CallTo32_0(proc32);
		break;
	case 1:	ret = CallTo32_1(proc32,args[0]);
		break;
	case 2:	ret = CallTo32_2(proc32,args[0],args[1]);
		break;
	case 3:	ret = CallTo32_3(proc32,args[0],args[1],args[2]);
		break;
	case 4:	ret = CallTo32_4(proc32,args[0],args[1],args[2],args[3]);
		break;
	case 5:	ret = CallTo32_5(proc32,args[0],args[1],args[2],args[3],args[4]);
		break;
	default:
		/* FIXME: should go up to 32  arguments */
		fprintf(stderr,"CallProc32W: unsupported number of arguments %ld, please report.\n",nrofargs);
		ret = 0;
		break;
	}
	fprintf(stderr,"returns %08lx\n",ret);
	free(args);
	return ret;
}
