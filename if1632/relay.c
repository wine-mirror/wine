/*
 * Copyright 1993 Robert J. Amstadt
 * Copyright 1995 Alexandre Julliard
 */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "wine/winbase16.h"
#include "winnt.h"
#include "global.h"
#include "heap.h"
#include "module.h"
#include "stackframe.h"
#include "task.h"
#include "syslevel.h"
#include "debugstr.h"
#include "debugtools.h"
#include "main.h"

DEFAULT_DEBUG_CHANNEL(relay)


/***********************************************************************
 *           RELAY_Init
 */
BOOL RELAY_Init(void)
{
    WORD codesel;

      /* Allocate the code selector for CallTo16 routines */

    extern void Call16_Ret_Start(), Call16_Ret_End();
    extern void CallTo16_Ret();
    extern void CALL32_CBClient_Ret();
    extern void CALL32_CBClientEx_Ret();
    extern DWORD CallTo16_RetAddr;
    extern DWORD CALL32_CBClient_RetAddr;
    extern DWORD CALL32_CBClientEx_RetAddr;

    codesel = GLOBAL_CreateBlock( GMEM_FIXED, (void *)Call16_Ret_Start,
                                   (int)Call16_Ret_End - (int)Call16_Ret_Start,
                                   0, TRUE, TRUE, FALSE, NULL );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CallTo16_RetAddr = 
        MAKELONG( (int)CallTo16_Ret -(int)Call16_Ret_Start, codesel );
    CALL32_CBClient_RetAddr = 
        MAKELONG( (int)CALL32_CBClient_Ret -(int)Call16_Ret_Start, codesel );
    CALL32_CBClientEx_RetAddr = 
        MAKELONG( (int)CALL32_CBClientEx_Ret -(int)Call16_Ret_Start, codesel );

    /* Create built-in modules */
    if (!BUILTIN_Init()) return FALSE;

    /* Initialize thunking */
    return THUNK_Init();
}


/* from relay32/relay386.c */
extern char **debug_relay_excludelist,**debug_relay_includelist;

/***********************************************************************
 *           RELAY_DebugCallFrom16
 */
void RELAY_DebugCallFrom16( CONTEXT86 *context )
{
    STACK16FRAME *frame;
    WORD ordinal;
    char *args16, funstr[80];
    const char *args;
    int i, usecdecl, reg_func;

    if (!TRACE_ON(relay)) return;

    frame = CURRENT_STACK16;
    args = BUILTIN_GetEntryPoint16(frame->entry_cs,frame->entry_ip,funstr,&ordinal);
    if (!args) return; /* happens for the two snoop register relays */
    if (!RELAY_ShowDebugmsgRelay(funstr)) return;
    DPRINTF( "Call %s(",funstr);
    VA_START16( args16 );

    usecdecl = ( *args == 'c' );
    args += 2;
    reg_func = (    memcmp( args, "regs_", 5 ) == 0
                 || memcmp( args, "intr_", 5 ) == 0 );
    args += 5;

    if (usecdecl)
    {
        while (*args)
        {
            switch(*args)
            {
            case 'w':
            case 's':
                DPRINTF( "0x%04x", *(WORD *)args16 );
                args16 += 2;
                break;
            case 'l':
                DPRINTF( "0x%08x", *(int *)args16 );
                args16 += 4;
                break;
            case 't':
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                if (HIWORD(*(SEGPTR *)args16))
                    debug_dumpstr( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 ));
                args16 += 4;
                break;
            case 'p':
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                args16 += 4;
                break;
            case 'T':
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                if (HIWORD( *(SEGPTR *)args16 ))
                    debug_dumpstr( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 ));
                args16 += 4;
                break;
            }
            args++;
            if (*args) DPRINTF( "," );
        }
    }
    else  /* not cdecl */
    {
        /* Start with the last arg */
        for (i = 0; args[i]; i++)
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
                DPRINTF( "0x%04x", *(WORD *)args16 );
                break;
            case 'l':
                args16 -= 4;
                DPRINTF( "0x%08x", *(int *)args16 );
                break;
            case 't':
                args16 -= 4;
                DPRINTF( "0x%08x", *(int *)args16 );
                if (HIWORD(*(SEGPTR *)args16))
                    debug_dumpstr( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 ));
                break;
            case 'p':
                args16 -= 4;
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                break;
            case 'T':
                args16 -= 4;
                DPRINTF( "%04x:%04x", *(WORD *)(args16+2), *(WORD *)args16 );
                if (HIWORD( *(SEGPTR *)args16 ))
                    debug_dumpstr( (LPSTR)PTR_SEG_TO_LIN(*(SEGPTR *)args16 ));
                break;
            }
            args++;
            if (*args) DPRINTF( "," );
        }
    }

    DPRINTF( ") ret=%04x:%04x ds=%04x\n", frame->cs, frame->ip, frame->ds );
    VA_END16( args16 );

    if (reg_func)
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                (WORD)ES_reg(context), EFL_reg(context) );

    SYSLEVEL_CheckNotLevel( 2 );
}


/***********************************************************************
 *           RELAY_DebugCallFrom16Ret
 */
void RELAY_DebugCallFrom16Ret( CONTEXT86 *context, int ret_val )
{
    STACK16FRAME *frame;
    WORD ordinal;
    char funstr[80];
    const char *args;

    if (!TRACE_ON(relay)) return;
    frame = CURRENT_STACK16;
    args = BUILTIN_GetEntryPoint16(frame->entry_cs,frame->entry_ip,funstr,&ordinal);
    if (!args) return;
    if (!RELAY_ShowDebugmsgRelay(funstr)) return;
    DPRINTF( "Ret  %s() ",funstr);

    if ( memcmp( args+2, "long_", 5 ) == 0 )
    {
        DPRINTF( "retval=0x%08x ret=%04x:%04x ds=%04x\n",
                 ret_val, frame->cs, frame->ip, frame->ds );
    }
    else if ( memcmp( args+2, "word_", 5 ) == 0 )
    {
        DPRINTF( "retval=0x%04x ret=%04x:%04x ds=%04x\n",
                 ret_val & 0xffff, frame->cs, frame->ip, frame->ds );
    }
    else if (    memcmp( args+2, "regs_", 5 ) == 0 
              || memcmp( args+2, "intr_", 5 ) == 0 )
    {
        DPRINTF("retval=none ret=%04x:%04x ds=%04x\n",
                (WORD)CS_reg(context), IP_reg(context), (WORD)DS_reg(context));
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x ES=%04x EFL=%08lx\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                (WORD)ES_reg(context), EFL_reg(context) );
    }

    SYSLEVEL_CheckNotLevel( 2 );
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
    char name[80];
    STACK16FRAME *frame = CURRENT_STACK16;
    BUILTIN_GetEntryPoint16(frame->entry_cs,frame->entry_ip,name,&ordinal);
    MESSAGE("No handler for Win16 routine %s (called from %04x:%04x)\n",
            name, frame->cs, frame->ip );
    ExitProcess(1);
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
    TEB *teb;

    if (!TRACE_ON(relay)) return;
    teb = NtCurrentTeb();

    if (nb_args == -1)  /* Register function */
    {
        CONTEXT86 *context = (CONTEXT86 *)stack[0];
        WORD *stack16 = (WORD *)THREAD_STACK16(teb);
        DPRINTF("CallTo16(func=%04lx:%04x,ds=%04lx",
                CS_reg(context), IP_reg(context), DS_reg(context) );
        nb_args = stack[1] / sizeof(WORD);
        while (nb_args--) {
	    --stack16;
	    DPRINTF( ",0x%04x", *stack16 );
	}
        DPRINTF(") ss:sp=%04x:%04x\n", SELECTOROF(teb->cur_stack),
                OFFSETOF(teb->cur_stack) );
        DPRINTF("     AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x ES=%04x FS=%04x\n",
                AX_reg(context), BX_reg(context), CX_reg(context),
                DX_reg(context), SI_reg(context), DI_reg(context),
                BP_reg(context), (WORD)ES_reg(context), (WORD)FS_reg(context) );
    }
    else
    {
        DPRINTF("CallTo16(func=%04x:%04x,ds=%04x",
                HIWORD(stack[0]), LOWORD(stack[0]),
                SELECTOROF(teb->cur_stack) );
        stack++;
        while (nb_args--) {
	    DPRINTF(",0x%04x", *stack );
	    stack++;
	}
        DPRINTF(") ss:sp=%04x:%04x\n", SELECTOROF(teb->cur_stack),
                OFFSETOF(teb->cur_stack) );
    }

    SYSLEVEL_CheckNotLevel( 2 );
}


/***********************************************************************
 *           RELAY_DebugCallTo16Ret
 */
void RELAY_DebugCallTo16Ret( int ret_val )
{
    if (!TRACE_ON(relay)) return;

    DPRINTF("CallTo16() ss:sp=%04x:%04x retval=0x%08x\n", 
            SELECTOROF(NtCurrentTeb()->cur_stack),
            OFFSETOF(NtCurrentTeb()->cur_stack), ret_val);
    SYSLEVEL_CheckNotLevel( 2 );
}


/**********************************************************************
 *	     Catch    (KERNEL.55)
 *
 * Real prototype is:
 *   INT16 WINAPI Catch( LPCATCHBUF lpbuf );
 */
void WINAPI Catch16( LPCATCHBUF lpbuf, CONTEXT86 *context )
{
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
    /* Windows pushes 4 more words before saving sp */
    lpbuf[2] = SP_reg(context) - 4 * sizeof(WORD);
    lpbuf[3] = BP_reg(context);
    lpbuf[4] = SI_reg(context);
    lpbuf[5] = DI_reg(context);
    lpbuf[6] = DS_reg(context);
    lpbuf[7] = 0;
    lpbuf[8] = SS_reg(context);
    AX_reg(context) = 0;  /* Return 0 */
}


/**********************************************************************
 *	     Throw    (KERNEL.56)
 *
 * Real prototype is:
 *   INT16 WINAPI Throw( LPCATCHBUF lpbuf, INT16 retval );
 */
void WINAPI Throw16( LPCATCHBUF lpbuf, INT16 retval, CONTEXT86 *context )
{
    STACK16FRAME *pFrame;
    STACK32FRAME *frame32;
    TEB *teb = NtCurrentTeb();

    AX_reg(context) = retval;

    /* Find the frame32 corresponding to the frame16 we are jumping to */
    pFrame = THREAD_STACK16(teb);
    frame32 = pFrame->frame32;
    while (frame32 && frame32->frame16)
    {
        if (OFFSETOF(frame32->frame16) < OFFSETOF(teb->cur_stack))
            break;  /* Something strange is going on */
        if (OFFSETOF(frame32->frame16) > lpbuf[2])
        {
            /* We found the right frame */
            pFrame->frame32 = frame32;
            break;
        }
        frame32 = ((STACK16FRAME *)PTR_SEG_TO_LIN(frame32->frame16))->frame32;
    }

    IP_reg(context) = lpbuf[0];
    CS_reg(context) = lpbuf[1];
    SP_reg(context) = lpbuf[2] + 4 * sizeof(WORD) - sizeof(WORD) /*extra arg*/;
    BP_reg(context) = lpbuf[3];
    SI_reg(context) = lpbuf[4];
    DI_reg(context) = lpbuf[5];
    DS_reg(context) = lpbuf[6];

    if (lpbuf[8] != SS_reg(context))
        ERR("Switching stack segment with Throw() not supported; expect crash now\n" );

    if (TRACE_ON(relay))  /* Make sure we have a valid entry point address */
    {
        static FARPROC16 entryPoint = NULL;

        if (!entryPoint)  /* Get entry point for Throw() */
            entryPoint = NE_GetEntryPoint( GetModuleHandle16("KERNEL"), 56 );
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
	FARPROC proc32;
	DWORD *args, ret;
        VA_LIST16 valist;
	int i;
	int aix;
	dbg_decl_str(relay, 1024);

	SYSLEVEL_ReleaseWin16Lock();

        VA_START16( valist );
        nrofargs    = VA_ARG16( valist, DWORD );
        argconvmask = VA_ARG16( valist, DWORD );
        proc32      = VA_ARG16( valist, FARPROC );
	dsprintf(relay, "CallProc32W(%ld,%ld,%p, Ex%d args[",nrofargs,argconvmask,proc32,Ex);
	args = (DWORD*)HEAP_xalloc( GetProcessHeap(), 0,
                                    sizeof(DWORD)*nrofargs );
	/* CallProcEx doesn't need its args reversed */
	for (i=0;i<nrofargs;i++) {
		if (Ex) {
		   aix = i;
		} else {
		   aix = nrofargs - i - 1;
		}
		if (argconvmask & (1<<i))
                {
                    SEGPTR ptr = VA_ARG16( valist, SEGPTR );
                    args[aix] = (DWORD)PTR_SEG_TO_LIN(ptr);
                    dsprintf(relay,"%08lx(%p),",ptr,PTR_SEG_TO_LIN(ptr));
		}
                else
                {
                    args[aix] = VA_ARG16( valist, DWORD );
                    dsprintf(relay,"%ld,",args[aix]);
		}
	}
	dsprintf(relay,"])");
        VA_END16( valist );

        if (!proc32) ret = 0;
        else switch (nrofargs)
        {
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
	case 8:	ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
		break;
	case 9:	ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
		break;
	case 10:	ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
		break;
	case 11:	ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
		break;
	default:
		/* FIXME: should go up to 32  arguments */
		ERR("Unsupported number of arguments %ld, please report.\n",nrofargs);
		ret = 0;
		break;
	}
	/* POP nrofargs DWORD arguments and 3 DWORD parameters */
        if (!Ex) STACK16_POP( NtCurrentTeb(), (3 + nrofargs) * sizeof(DWORD) );

	TRACE("%s - returns %08lx\n",dbg_str(relay),ret);
	HeapFree( GetProcessHeap(), 0, args );

	SYSLEVEL_RestoreWin16Lock();

	return ret;
}


/**********************************************************************
 *	     CallProc32W    (KERNEL.517)
 */
DWORD WINAPI CallProc32W_16()
{
        return RELAY_CallProc32W(0);
}


/**********************************************************************
*           CallProcEx32W()   (KERNEL.518)
*
*      C - style linkage to CallProc32W - caller pops stack.
*/
DWORD WINAPI CallProcEx32W_16()
{
	return RELAY_CallProc32W(TRUE);
}
