/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"
#include "winnt.h"
#include "global.h"
#include "heap.h"
#include "module.h"
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
    extern BOOL32 THUNK_Init(void);

      /* Allocate the code selector for CallTo16 routines */

    extern void CALLTO16_Start(), CALLTO16_End();
    extern void CALLTO16_Ret_word(), CALLTO16_Ret_long(), CALLTO16_Ret_regs();
    extern DWORD CALLTO16_RetAddr_word;
    extern DWORD CALLTO16_RetAddr_long;
    extern DWORD CALLTO16_RetAddr_regs;

    codesel = GLOBAL_CreateBlock( GMEM_FIXED, (void *)CALLTO16_Start,
                                   (int)CALLTO16_End - (int)CALLTO16_Start,
                                   0, TRUE, TRUE, FALSE, NULL );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CALLTO16_RetAddr_word=MAKELONG( (int)CALLTO16_Ret_word-(int)CALLTO16_Start,
                                    codesel );
    CALLTO16_RetAddr_long=MAKELONG( (int)CALLTO16_Ret_long-(int)CALLTO16_Start,
                                    codesel );
    CALLTO16_RetAddr_regs=MAKELONG( (int)CALLTO16_Ret_regs-(int)CALLTO16_Start,
                                    codesel );

    /* Create built-in modules */
    if (!BUILTIN_Init()) return FALSE;

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
    VA_START16( args16 );
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
        case 't':
        case 'T':
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
        case 't':
            args16 -= 4;
	    printf( "0x%08x", *(int *)args16 );
            if (HIWORD(*(int *)args16))
                printf( " \"%s\"", (char *)PTR_SEG_TO_LIN(*(int *)args16) );
            break;
        case 'p':
            args16 -= 4;
            printf( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
            break;
        case 'T':
            args16 -= 4;
            printf( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
            if (HIWORD(*(int *)args16))
                printf( " \"%s\"", (char *)PTR_SEG_TO_LIN(*(int *)args16) );
            break;
        }
        args++;
        if (*args) printf( "," );
    }
    printf( ") ret=%04x:%04x ds=%04x\n", frame->cs, frame->ip, frame->ds );
    VA_END16( args16 );

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
                (WORD)CS_reg(context), IP_reg(context), (WORD)DS_reg(context));
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
        CONTEXT *context = (CONTEXT *)stack[0];
        WORD *stack16 = (WORD *)CURRENT_STACK16 - 2 /* for saved %%esp */;
        printf( "CallTo16(func=%04lx:%04x,ds=%04lx",
                CS_reg(context), IP_reg(context), DS_reg(context) );
        nb_args = -stack[1] / sizeof(WORD);
        while (nb_args--) printf( ",0x%04x", *(--stack16) );
        printf( ")\n" );
        printf( "     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x ES=%04x\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                BP_reg(context), (WORD)ES_reg(context) );
    }
    else
    {
        printf( "CallTo16(func=%04x:%04x,ds=%04x",
                HIWORD(stack[0]), LOWORD(stack[0]),
                SELECTOROF(IF1632_Saved16_ss_sp) );
        stack++;
        while (nb_args--) printf( ",0x%04x", *stack++ );
        printf( ")\n" );
    }
}


/**********************************************************************
 *	     Catch    (KERNEL.55)
 *
 * Real prototype is:
 *   INT16 WINAPI Catch( LPCATCHBUF lpbuf );
 */
void WINAPI Catch( CONTEXT *context )
{
    VA_LIST16 valist;
    SEGPTR buf;
    LPCATCHBUF lpbuf;
    STACK16FRAME *pFrame = CURRENT_STACK16;

    VA_START16( valist );
    buf   = VA_ARG16( valist, SEGPTR );
    lpbuf = (LPCATCHBUF)PTR_SEG_TO_LIN( buf );
    VA_END16( valist );

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

    lpbuf[0] = IP_reg(context);
    lpbuf[1] = CS_reg(context);
    lpbuf[2] = LOWORD(pFrame->saved_ss_sp);
    lpbuf[3] = BP_reg(context);
    lpbuf[4] = SI_reg(context);
    lpbuf[5] = DI_reg(context);
    lpbuf[6] = DS_reg(context);
    lpbuf[7] = OFFSETOF(IF1632_Saved16_ss_sp);
    lpbuf[8] = HIWORD(pFrame->saved_ss_sp);
    AX_reg(context) = 0;  /* Return 0 */
}


/**********************************************************************
 *	     Throw    (KERNEL.56)
 *
 * Real prototype is:
 *   INT16 WINAPI Throw( LPCATCHBUF lpbuf, INT16 retval );
 */
void WINAPI Throw( CONTEXT *context )
{
    VA_LIST16 valist;
    SEGPTR buf;
    LPCATCHBUF lpbuf;
    STACK16FRAME *pFrame;

    VA_START16( valist );
    AX_reg(context) = VA_ARG16( valist, WORD );  /* retval */
    buf    = VA_ARG16( valist, SEGPTR );
    lpbuf  = (LPCATCHBUF)PTR_SEG_TO_LIN( buf );
    VA_END16( valist );

    IF1632_Saved16_ss_sp = MAKELONG( lpbuf[7] - sizeof(WORD),
                                     HIWORD(IF1632_Saved16_ss_sp) );
    pFrame = CURRENT_STACK16;
    pFrame->saved_ss_sp = MAKELONG( lpbuf[2], lpbuf[8] );
    IP_reg(context) = lpbuf[0];
    CS_reg(context) = lpbuf[1];
    BP_reg(context) = lpbuf[3];
    SI_reg(context) = lpbuf[4];
    DI_reg(context) = lpbuf[5];
    DS_reg(context) = lpbuf[6];

    if (debugging_relay)  /* Make sure we have a valid entry point address */
    {
        static FARPROC16 entryPoint = NULL;

        if (!entryPoint)  /* Get entry point for Throw() */
            entryPoint = MODULE_GetEntryPoint( GetModuleHandle16("KERNEL"),
                                               56 );
        pFrame->entry_cs = SELECTOROF(entryPoint);
        pFrame->entry_ip = OFFSETOF(entryPoint);
    }
}


/**********************************************************************
 *	     RELAY_CallProc32W
 *
 * Helper for CallProc[Ex]32W
 */
static DWORD RELAY_CallProc32W(int Ex)
{
	DWORD nrofargs, argconvmask;
	FARPROC32 proc32;
	DWORD *args, ret;
        VA_LIST16 valist;
	int i;

        VA_START16( valist );
        nrofargs    = VA_ARG16( valist, DWORD );
        argconvmask = VA_ARG16( valist, DWORD );
        proc32      = VA_ARG16( valist, FARPROC32 );
	fprintf(stderr,"CallProc32W(%ld,%ld,%p, Ex%d args[",nrofargs,argconvmask,proc32,Ex);
	args = (DWORD*)HEAP_xalloc( GetProcessHeap(), 0,
                                    sizeof(DWORD)*nrofargs );
	for (i=0;i<nrofargs;i++) {
		if (argconvmask & (1<<i))
                {
                    SEGPTR ptr = VA_ARG16( valist, SEGPTR );
                    args[nrofargs-i-1] = (DWORD)PTR_SEG_TO_LIN(ptr);
                    fprintf(stderr,"%08lx(%p),",ptr,PTR_SEG_TO_LIN(ptr));
		}
                else
                {
                    args[nrofargs-i-1] = VA_ARG16( valist, DWORD );
                    fprintf(stderr,"%ld,",args[nrofargs-i-1]);
		}
	}
	fprintf(stderr,"]) - ");
        VA_END16( valist );

	switch (nrofargs) {
	case 0: ret = proc32();
		break;
	case 1:	ret = proc32(args[0]);
		break;
	case 2:	ret = proc32(args[0],args[1]);
		break;
	case 3:	ret = proc32(args[0],args[1],args[2]);
		break;
	case 4:	ret = proc32(args[0],args[1],args[2],args[3]);
		break;
	case 5:	ret = proc32(args[0],args[1],args[2],args[3],args[4]);
		break;
	case 6:	ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5]);
		break;
	case 7:	ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
		break;
	default:
		/* FIXME: should go up to 32  arguments */
		fprintf(stderr,"CallProc32W: unsupported number of arguments %ld, please report.\n",nrofargs);
		ret = 0;
		break;
	}
	/* POP nrofargs DWORD arguments and 3 DWORD parameters */
        if (!Ex) STACK16_POP( (3 + nrofargs) * sizeof(DWORD) );

	fprintf(stderr,"returns %08lx\n",ret);
	HeapFree( GetProcessHeap(), 0, args );
	return ret;
}


/**********************************************************************
 *	     CallProc32W    (KERNEL.517)
 */
DWORD WINAPI WIN16_CallProc32W()
{
        return RELAY_CallProc32W(0);
}


/**********************************************************************
*           CallProcEx32W()   (KERNEL.518)        
*
*      C - style linkage to CallProc32W - caller pops stack.
*/ 
DWORD WINAPI WIN16_CallProcEx32W()
{
	return RELAY_CallProc32W(TRUE);
}
