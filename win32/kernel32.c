/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1997-1998 Marcus Meissner
 * Copyright 1998      Ulrich Weigand
 *
 */

#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wine/winbase16.h"
#include "callback.h"
#include "task.h"
#include "user.h"
#include "heap.h"
#include "module.h"
#include "neexe.h"
#include "process.h"
#include "stackframe.h"
#include "heap.h"
#include "selectors.h"
#include "task.h"
#include "file.h"
#include "debug.h"
#include "flatthunk.h"
#include "syslevel.h"
#include "winerror.h"

DECLARE_DEBUG_CHANNEL(dosmem)
DECLARE_DEBUG_CHANNEL(thunk)
DECLARE_DEBUG_CHANNEL(win32)


/***********************************************************************
 *                                                                     *
 *                 Win95 internal thunks                               *
 *                                                                     *
 ***********************************************************************/

/***********************************************************************
 * Generates a FT_Prolog call.
 *	
 *  0FB6D1                  movzbl edx,cl
 *  8B1495xxxxxxxx	    mov edx,[4*edx + targetTable]
 *  68xxxxxxxx		    push FT_Prolog
 *  C3			    lret
 */
static void _write_ftprolog(LPBYTE relayCode ,DWORD *targetTable) {
	LPBYTE	x;

	x	= relayCode;
	*x++	= 0x0f;*x++=0xb6;*x++=0xd1; /* movzbl edx,cl */
	*x++	= 0x8B;*x++=0x14;*x++=0x95;*(DWORD**)x= targetTable;
	x+=4;	/* mov edx, [4*edx + targetTable] */
	*x++	= 0x68; *(DWORD*)x = (DWORD)GetProcAddress(GetModuleHandleA("KERNEL32"),"FT_Prolog");
	x+=4; 	/* push FT_Prolog */
	*x++	= 0xC3;		/* lret */
	/* fill rest with 0xCC / int 3 */
}

/***********************************************************************
 *	_write_qtthunk					(internal)
 * Generates a QT_Thunk style call.
 *
 *  33C9                    xor ecx, ecx
 *  8A4DFC                  mov cl , [ebp-04]
 *  8B148Dxxxxxxxx          mov edx, [4*ecx + targetTable]
 *  B8yyyyyyyy              mov eax, QT_Thunk
 *  FFE0                    jmp eax
 */
static void _write_qtthunk(
	LPBYTE relayCode,	/* [in] start of QT_Thunk stub */
	DWORD *targetTable	/* [in] start of thunk (for index lookup) */
) {
	LPBYTE	x;

	x	= relayCode;
	*x++	= 0x33;*x++=0xC9; /* xor ecx,ecx */
	*x++	= 0x8A;*x++=0x4D;*x++=0xFC; /* movb cl,[ebp-04] */
	*x++	= 0x8B;*x++=0x14;*x++=0x8D;*(DWORD**)x= targetTable;
	x+=4;	/* mov edx, [4*ecx + targetTable */
	*x++	= 0xB8; *(DWORD*)x = (DWORD)GetProcAddress(GetModuleHandleA("KERNEL32"),"QT_Thunk");
	x+=4; 	/* mov eax , QT_Thunk */
	*x++	= 0xFF; *x++ = 0xE0;	/* jmp eax */
	/* should fill the rest of the 32 bytes with 0xCC */
}

/***********************************************************************
 *           _loadthunk
 */
static LPVOID _loadthunk(LPCSTR module, LPCSTR func, LPCSTR module32, 
                         struct ThunkDataCommon *TD32, DWORD checksum)
{
    struct ThunkDataCommon *TD16;
    HMODULE hmod;
    int ordinal;

    if ((hmod = LoadLibrary16(module)) <= 32) 
    {
        ERR(thunk, "(%s, %s, %s): Unable to load '%s', error %d\n",
                   module, func, module32, module, hmod);
        return 0;
    }

    if (   !(ordinal = NE_GetOrdinal(hmod, func))
        || !(TD16 = PTR_SEG_TO_LIN(NE_GetEntryPointEx(hmod, ordinal, FALSE))))
    {
        ERR(thunk, "(%s, %s, %s): Unable to find '%s'\n",
                   module, func, module32, func);
        return 0;
    }

    if (TD32 && memcmp(TD16->magic, TD32->magic, 4))
    {
        ERR(thunk, "(%s, %s, %s): Bad magic %c%c%c%c (should be %c%c%c%c)\n",
                   module, func, module32, 
                   TD16->magic[0], TD16->magic[1], TD16->magic[2], TD16->magic[3],
                   TD32->magic[0], TD32->magic[1], TD32->magic[2], TD32->magic[3]);
        return 0;
    }

    if (TD32 && TD16->checksum != TD32->checksum)
    {
        ERR(thunk, "(%s, %s, %s): Wrong checksum %08lx (should be %08lx)\n",
                   module, func, module32, TD16->checksum, TD32->checksum);
        return 0;
    }

    if (!TD32 && checksum && checksum != *(LPDWORD)TD16)
    {
        ERR(thunk, "(%s, %s, %s): Wrong checksum %08lx (should be %08lx)\n",
                   module, func, module32, *(LPDWORD)TD16, checksum);
        return 0;
    }

    return TD16;
}

/***********************************************************************
 *           GetThunkStuff    (KERNEL32.53)
 */
LPVOID WINAPI GetThunkStuff(LPSTR module, LPSTR func)
{
    return _loadthunk(module, func, "<kernel>", NULL, 0L);
}

/***********************************************************************
 *           GetThunkBuff    (KERNEL32.52)
 * Returns a pointer to ThkBuf in the 16bit library SYSTHUNK.DLL.
 */
LPVOID WINAPI GetThunkBuff(void)
{
    return GetThunkStuff("SYSTHUNK.DLL", "ThkBuf");
}

/***********************************************************************
 *		ThunkConnect32		(KERNEL32)
 * Connects a 32bit and a 16bit thunkbuffer.
 */
UINT WINAPI ThunkConnect32( 
	struct ThunkDataCommon *TD,  /* [in/out] thunkbuffer */
	LPSTR thunkfun16,            /* [in] win16 thunkfunction */
	LPSTR module16,              /* [in] name of win16 dll */
	LPSTR module32,              /* [in] name of win32 dll */
	HMODULE hmod32,            /* [in] hmodule of win32 dll */
	DWORD dwReason               /* [in] initialisation argument */
) {
    BOOL directionSL;

    if (!lstrncmpA(TD->magic, "SL01", 4))
    {
        directionSL = TRUE;

        TRACE(thunk, "SL01 thunk %s (%lx) <- %s (%s), Reason: %ld\n",
                     module32, (DWORD)TD, module16, thunkfun16, dwReason);
    }
    else if (!lstrncmpA(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE(thunk, "LS01 thunk %s (%lx) -> %s (%s), Reason: %ld\n",
                     module32, (DWORD)TD, module16, thunkfun16, dwReason);
    }
    else
    {
        ERR(thunk, "Invalid magic %c%c%c%c\n", 
                   TD->magic[0], TD->magic[1], TD->magic[2], TD->magic[3]);
        return 0;
    }
    
    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            struct ThunkDataCommon *TD16;
            if (!(TD16 = _loadthunk(module16, thunkfun16, module32, TD, 0L)))
                return 0;

            if (directionSL)
            {
                struct ThunkDataSL32 *SL32 = (struct ThunkDataSL32 *)TD;
                struct ThunkDataSL16 *SL16 = (struct ThunkDataSL16 *)TD16;
                struct SLTargetDB *tdb;

                if (SL16->fpData == NULL)
                {
                    ERR(thunk, "ThunkConnect16 was not called!\n");
                    return 0;
                }

                SL32->data = SL16->fpData;

                tdb = HeapAlloc(GetProcessHeap(), 0, sizeof(*tdb));
                tdb->process = PROCESS_Current();
                tdb->targetTable = (DWORD *)(thunkfun16 + SL32->offsetTargetTable);

                tdb->next = SL32->data->targetDB;   /* FIXME: not thread-safe! */
                SL32->data->targetDB = tdb;

                TRACE(thunk, "Process %08lx allocated TargetDB entry for ThunkDataSL %08lx\n", 
                             (DWORD)PROCESS_Current(), (DWORD)SL32->data);
            }
            else
            {
                struct ThunkDataLS32 *LS32 = (struct ThunkDataLS32 *)TD;
                struct ThunkDataLS16 *LS16 = (struct ThunkDataLS16 *)TD16;

                LS32->targetTable = PTR_SEG_TO_LIN(LS16->targetTable);

                /* write QT_Thunk and FT_Prolog stubs */
                _write_qtthunk ((LPBYTE)TD + LS32->offsetQTThunk,  LS32->targetTable);
                _write_ftprolog((LPBYTE)TD + LS32->offsetFTProlog, LS32->targetTable);
            }
            break;
        }

        case DLL_PROCESS_DETACH:
            /* FIXME: cleanup */
            break;
    }

    return 1;
}

/**********************************************************************
 * 		QT_Thunk			(KERNEL32)
 *
 * The target address is in EDX.
 * The 16 bit arguments start at ESP+4.
 * The number of 16bit argumentbytes is EBP-ESP-0x44 (68 Byte thunksetup).
 * [ok]
 */
REGS_ENTRYPOINT(QT_Thunk)
{
    CONTEXT context16;
    DWORD argsize;
    THDB *thdb = THREAD_Current();

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(EDX_reg(context));
    IP_reg(&context16)  = LOWORD(EDX_reg(context));
    EBP_reg(&context16) = OFFSETOF( thdb->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = EBP_reg(context)-ESP_reg(context)-0x44;

    memcpy( ((LPBYTE)THREAD_STACK16(thdb))-argsize,
            (LPBYTE)ESP_reg(context)+4, argsize );

    EAX_reg(context) = Callbacks->CallRegisterShortProc( &context16, argsize );
    EDX_reg(context) = HIWORD(EAX_reg(context));
    EAX_reg(context) = LOWORD(EAX_reg(context));
}


/**********************************************************************
 * 		FT_Prolog			(KERNEL32.233)
 * 
 * The set of FT_... thunk routines is used instead of QT_Thunk,
 * if structures have to be converted from 32-bit to 16-bit
 * (change of member alignment, conversion of members).
 *
 * The thunk function (as created by the thunk compiler) calls
 * FT_Prolog at the beginning, to set up a stack frame and
 * allocate a 64 byte buffer on the stack.
 * The input parameters (target address and some flags) are
 * saved for later use by FT_Thunk.
 *
 * Input:  EDX  16-bit target address (SEGPTR)
 *         CX   bits  0..7   target number (in target table)
 *              bits  8..9   some flags (unclear???)
 *              bits 10..15  number of DWORD arguments
 *
 * Output: A new stackframe is created, and a 64 byte buffer
 *         allocated on the stack. The layout of the stack 
 *         on return is as follows:
 *
 *  (ebp+4)  return address to caller of thunk function
 *  (ebp)    old EBP
 *  (ebp-4)  saved EBX register of caller
 *  (ebp-8)  saved ESI register of caller
 *  (ebp-12) saved EDI register of caller
 *  (ebp-16) saved ECX register, containing flags
 *  (ebp-20) bitmap containing parameters that are to be converted
 *           by FT_Thunk; it is initialized to 0 by FT_Prolog and
 *           filled in by the thunk code before calling FT_Thunk
 *  (ebp-24)
 *    ...    (unclear)
 *  (ebp-44)
 *  (ebp-48) saved EAX register of caller (unclear, never restored???)
 *  (ebp-52) saved EDX register, containing 16-bit thunk target
 *  (ebp-56)
 *    ...    (unclear)
 *  (ebp-64)
 *
 *  ESP is EBP-68 on return.
 *         
 */

REGS_ENTRYPOINT(FT_Prolog)
{
    /* Pop return address to thunk code */
    EIP_reg(context) = STACK32_POP(context);

    /* Build stack frame */
    STACK32_PUSH(context, EBP_reg(context));
    EBP_reg(context) = ESP_reg(context);

    /* Allocate 64-byte Thunk Buffer */
    ESP_reg(context) -= 64;
    memset((char *)ESP_reg(context), '\0', 64);

    /* Store Flags (ECX) and Target Address (EDX) */
    /* Save other registers to be restored later */
    *(DWORD *)(EBP_reg(context) -  4) = EBX_reg(context);
    *(DWORD *)(EBP_reg(context) -  8) = ESI_reg(context);
    *(DWORD *)(EBP_reg(context) - 12) = EDI_reg(context);
    *(DWORD *)(EBP_reg(context) - 16) = ECX_reg(context);

    *(DWORD *)(EBP_reg(context) - 48) = EAX_reg(context);
    *(DWORD *)(EBP_reg(context) - 52) = EDX_reg(context);
    
    /* Push return address back onto stack */
    STACK32_PUSH(context, EIP_reg(context));
}

/**********************************************************************
 * 		FT_Thunk			(KERNEL32.234)
 *
 * This routine performs the actual call to 16-bit code, 
 * similar to QT_Thunk. The differences are:
 *  - The call target is taken from the buffer created by FT_Prolog
 *  - Those arguments requested by the thunk code (by setting the
 *    corresponding bit in the bitmap at EBP-20) are converted
 *    from 32-bit pointers to segmented pointers (those pointers
 *    are guaranteed to point to structures copied to the stack
 *    by the thunk code, so we always use the 16-bit stack selector
 *    for those addresses).
 * 
 *    The bit #i of EBP-20 corresponds here to the DWORD starting at
 *    ESP+4 + 2*i.
 * 
 * FIXME: It is unclear what happens if there are more than 32 WORDs 
 *        of arguments, so that the single DWORD bitmap is no longer
 *        sufficient ...
 */

REGS_ENTRYPOINT(FT_Thunk)
{
    DWORD mapESPrelative = *(DWORD *)(EBP_reg(context) - 20);
    DWORD callTarget     = *(DWORD *)(EBP_reg(context) - 52);

    CONTEXT context16;
    DWORD i, argsize;
    LPBYTE newstack, oldstack;
    THDB *thdb = THREAD_Current();

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(callTarget);
    IP_reg(&context16)  = LOWORD(callTarget);
    EBP_reg(&context16) = OFFSETOF( thdb->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize  = EBP_reg(context)-ESP_reg(context)-0x44;
    newstack = ((LPBYTE)THREAD_STACK16(thdb))-argsize;
    oldstack = (LPBYTE)ESP_reg(context)+4;

    memcpy( newstack, oldstack, argsize );

    for (i = 0; i < 32; i++)	/* NOTE: What about > 32 arguments? */
	if (mapESPrelative & (1 << i))
	{
	    SEGPTR *arg = (SEGPTR *)(newstack + 2*i);
	    *arg = PTR_SEG_OFF_TO_SEGPTR(SELECTOROF(thdb->cur_stack), 
                                         OFFSETOF(thdb->cur_stack) - argsize
					 + (*(LPBYTE *)arg - oldstack));
	}

    EAX_reg(context) = Callbacks->CallRegisterShortProc( &context16, argsize );
    EDX_reg(context) = HIWORD(EAX_reg(context));
    EAX_reg(context) = LOWORD(EAX_reg(context));
}

/**********************************************************************
 * 		FT_ExitNN		(KERNEL32.218 - 232)
 *
 * One of the FT_ExitNN functions is called at the end of the thunk code.
 * It removes the stack frame created by FT_Prolog, moves the function
 * return from EBX to EAX (yes, FT_Thunk did use EAX for the return 
 * value, but the thunk code has moved it from EAX to EBX in the 
 * meantime ... :-), restores the caller's EBX, ESI, and EDI registers,
 * and perform a return to the CALLER of the thunk code (while removing
 * the given number of arguments from the caller's stack).
 */

static void FT_Exit(CONTEXT *context, int nPopArgs)
{
    /* Return value is in EBX */
    EAX_reg(context) = EBX_reg(context);

    /* Restore EBX, ESI, and EDI registers */
    EBX_reg(context) = *(DWORD *)(EBP_reg(context) -  4);
    ESI_reg(context) = *(DWORD *)(EBP_reg(context) -  8);
    EDI_reg(context) = *(DWORD *)(EBP_reg(context) - 12);

    /* Clean up stack frame */
    ESP_reg(context) = EBP_reg(context);
    EBP_reg(context) = STACK32_POP(context);

    /* Pop return address to CALLER of thunk code */
    EIP_reg(context) = STACK32_POP(context);
    /* Remove arguments */
    ESP_reg(context) += nPopArgs;
    /* Push return address back onto stack */
    STACK32_PUSH(context, EIP_reg(context));
}

REGS_ENTRYPOINT(FT_Exit0)  { FT_Exit(context,  0); }
REGS_ENTRYPOINT(FT_Exit4)  { FT_Exit(context,  4); }
REGS_ENTRYPOINT(FT_Exit8)  { FT_Exit(context,  8); }
REGS_ENTRYPOINT(FT_Exit12) { FT_Exit(context, 12); }
REGS_ENTRYPOINT(FT_Exit16) { FT_Exit(context, 16); }
REGS_ENTRYPOINT(FT_Exit20) { FT_Exit(context, 20); }
REGS_ENTRYPOINT(FT_Exit24) { FT_Exit(context, 24); }
REGS_ENTRYPOINT(FT_Exit28) { FT_Exit(context, 28); }
REGS_ENTRYPOINT(FT_Exit32) { FT_Exit(context, 32); }
REGS_ENTRYPOINT(FT_Exit36) { FT_Exit(context, 36); }
REGS_ENTRYPOINT(FT_Exit40) { FT_Exit(context, 40); }
REGS_ENTRYPOINT(FT_Exit44) { FT_Exit(context, 44); }
REGS_ENTRYPOINT(FT_Exit48) { FT_Exit(context, 48); }
REGS_ENTRYPOINT(FT_Exit52) { FT_Exit(context, 52); }
REGS_ENTRYPOINT(FT_Exit56) { FT_Exit(context, 56); }


/**********************************************************************
 *           WOWCallback16 (KERNEL32.62)(WOW32.2)
 * Calls a win16 function with a single DWORD argument.
 * RETURNS
 *	the return value
 */
DWORD WINAPI WOWCallback16(
	FARPROC16 fproc,	/* [in] win16 function to call */
	DWORD arg		/* [in] single DWORD argument to function */
) {
	DWORD	ret;
	TRACE(thunk,"(%p,0x%08lx)...\n",fproc,arg);
	ret =  Callbacks->CallWOWCallbackProc(fproc,arg);
	TRACE(thunk,"... returns %ld\n",ret);
	return ret;
}

/**********************************************************************
 *           WOWCallback16Ex (KERNEL32.55)(WOW32.3)
 * Calls a function in 16bit code.
 * RETURNS
 *	TRUE for success
 */
BOOL WINAPI WOWCallback16Ex(
	FARPROC16 vpfn16,	/* [in] win16 function to call */
	DWORD dwFlags,		/* [in] flags */
	DWORD cbArgs,		/* [in] nr of arguments */
	LPVOID pArgs,		/* [in] pointer to arguments (LPDWORD) */
	LPDWORD pdwRetCode	/* [out] return value of win16 function */
) {
	return Callbacks->CallWOWCallback16Ex(vpfn16,dwFlags,cbArgs,pArgs,pdwRetCode);
}

/***********************************************************************
 * 		ThunkInitLS 	(KERNEL32.43)
 * A thunkbuffer link routine 
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The pointer ptr is written into the first DWORD of 'thunk'.
 * (probably correct implemented)
 * [ok probably]
 * RETURNS
 *	segmented pointer to thunk?
 */
DWORD WINAPI ThunkInitLS(
	LPDWORD thunk,	/* [in] win32 thunk */
	LPCSTR thkbuf,	/* [in] thkbuffer name in win16 dll */
	DWORD len,	/* [in] thkbuffer length */
	LPCSTR dll16,	/* [in] name of win16 dll */
	LPCSTR dll32	/* [in] name of win32 dll (FIXME: not used?) */
) {
	LPDWORD		addr;

	if (!(addr = _loadthunk( dll16, thkbuf, dll32, NULL, len )))
		return 0;

	if (!addr[1])
		return 0;
	*(DWORD*)thunk = addr[1];

	return addr[1];
}

/***********************************************************************
 * 		Common32ThkLS 	(KERNEL32.45)
 * 
 * This is another 32->16 thunk, independent of the QT_Thunk/FT_Thunk
 * style thunks. The basic difference is that the parameter conversion 
 * is done completely on the *16-bit* side here. Thus we do not call
 * the 16-bit target directly, but call a common entry point instead.
 * This entry function then calls the target according to the target
 * number passed in the DI register.
 * 
 * Input:  EAX    SEGPTR to the common 16-bit entry point
 *         CX     offset in thunk table (target number * 4)
 *         DX     error return value if execution fails (unclear???)
 *         EDX.HI number of DWORD parameters
 *
 * (Note that we need to move the thunk table offset from CX to DI !)
 *
 * The called 16-bit stub expects its stack to look like this:
 *     ...
 *   (esp+40)  32-bit arguments
 *     ...
 *   (esp+8)   32 byte of stack space available as buffer
 *   (esp)     8 byte return address for use with 0x66 lret 
 * 
 * The called 16-bit stub uses a 0x66 lret to return to 32-bit code,
 * and uses the EAX register to return a DWORD return value.
 * Thus we need to use a special assembly glue routine 
 * (CallRegisterLongProc instead of CallRegisterShortProc).
 *
 * Finally, we return to the caller, popping the arguments off 
 * the stack.
 *
 * FIXME: The called function uses EBX to return the number of 
 *        arguments that are to be popped off the caller's stack.
 *        This is clobbered by the assembly glue, so we simply use
 *        the original EDX.HI to get the number of arguments.
 *        (Those two values should be equal anyway ...?)
 * 
 */
REGS_ENTRYPOINT(Common32ThkLS)
{
    CONTEXT context16;
    DWORD argsize;
    THDB *thdb = THREAD_Current();

    memcpy(&context16,context,sizeof(context16));

    DI_reg(&context16)  = CX_reg(context);
    CS_reg(&context16)  = HIWORD(EAX_reg(context));
    IP_reg(&context16)  = LOWORD(EAX_reg(context));
    EBP_reg(&context16) = OFFSETOF( thdb->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = HIWORD(EDX_reg(context)) * 4;

    /* FIXME: hack for stupid USER32 CallbackGlueLS routine */
    if (EDX_reg(context) == EIP_reg(context))
        argsize = 6 * 4;

    memcpy( ((LPBYTE)THREAD_STACK16(thdb))-argsize,
            (LPBYTE)ESP_reg(context)+4, argsize );

    EAX_reg(context) = Callbacks->CallRegisterLongProc(&context16, argsize + 32);

    /* Clean up caller's stack frame */

    EIP_reg(context) = STACK32_POP(context);
    ESP_reg(context) += argsize;
    STACK32_PUSH(context, EIP_reg(context));
}

/***********************************************************************
 *		OT_32ThkLSF	(KERNEL32.40)
 *
 * YET Another 32->16 thunk. The difference to Common32ThkLS is that
 * argument processing is done on both the 32-bit and the 16-bit side:
 * The 32-bit side prepares arguments, copying them onto the stack.
 * 
 * When this routine is called, the first word on the stack is the 
 * number of argument bytes prepared by the 32-bit code, and EDX
 * contains the 16-bit target address.
 *
 * The called 16-bit routine is another relaycode, doing further 
 * argument processing and then calling the real 16-bit target
 * whose address is stored at [bp-04].
 *
 * The call proceeds using a normal CallRegisterShortProc.
 * After return from the 16-bit relaycode, the arguments need
 * to be copied *back* to the 32-bit stack, since the 32-bit
 * relaycode processes output parameters.
 * 
 * Note that we copy twice the number of arguments, since some of the
 * 16-bit relaycodes in SYSTHUNK.DLL directly access the original
 * arguments of the caller!
 *
 * (Note that this function seems only to be used for
 *  OLECLI32 -> OLECLI and OLESVR32 -> OLESVR thunking.)
 */
REGS_ENTRYPOINT(OT_32ThkLSF)
{
    CONTEXT context16;
    DWORD argsize;
    THDB *thdb = THREAD_Current();

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(EDX_reg(context));
    IP_reg(&context16)  = LOWORD(EDX_reg(context));
    EBP_reg(&context16) = OFFSETOF( thdb->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = 2 * *(WORD *)(ESP_reg(context) + 4) + 2;

    memcpy( ((LPBYTE)THREAD_STACK16(thdb))-argsize,
            (LPBYTE)ESP_reg(context)+4, argsize );

    EAX_reg(context) = Callbacks->CallRegisterShortProc(&context16, argsize);

    memcpy( (LPBYTE)ESP_reg(context)+4, 
            ((LPBYTE)THREAD_STACK16(thdb))-argsize, argsize );
}

/***********************************************************************
 *		ThunkInitLSF		(KERNEL32.41)
 * A thunk setup routine.
 * Expects a pointer to a preinitialized thunkbuffer in the first argument
 * looking like:
 *	00..03:		unknown	(pointer, check _41, _43, _46)
 *	04: EB1E		jmp +0x20
 *
 *	06..23:		unknown (space for replacement code, check .90)
 *
 *	24:>E800000000		call offset 29
 *	29:>58			pop eax		   ( target of call )
 *	2A: 2D25000000		sub eax,0x00000025 ( now points to offset 4 )
 *	2F: BAxxxxxxxx		mov edx,xxxxxxxx
 *	34: 68yyyyyyyy		push KERNEL32.90
 *	39: C3			ret
 *
 *	3A: EB1E		jmp +0x20
 *	3E ... 59:	unknown (space for replacement code?)
 *	5A: E8xxxxxxxx		call <32bitoffset xxxxxxxx>
 *	5F: 5A			pop edx
 *	60: 81EA25xxxxxx	sub edx, 0x25xxxxxx
 *	66: 52			push edx
 *	67: 68xxxxxxxx		push xxxxxxxx
 *	6C: 68yyyyyyyy		push KERNEL32.89
 *	71: C3			ret
 *	72: end?
 * This function checks if the code is there, and replaces the yyyyyyyy entries
 * by the functionpointers.
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The segpointer ptr is written into the first DWORD of 'thunk'.
 * [ok probably]
 * RETURNS
 *	unclear, pointer to win16 thkbuffer?
 */
LPVOID WINAPI ThunkInitLSF(
	LPBYTE thunk,	/* [in] win32 thunk */
	LPCSTR thkbuf,	/* [in] thkbuffer name in win16 dll */
	DWORD len,	/* [in] length of thkbuffer */
	LPCSTR dll16,	/* [in] name of win16 dll */
	LPCSTR dll32	/* [in] name of win32 dll */
) {
	HMODULE	hkrnl32 = GetModuleHandleA("KERNEL32");
	LPDWORD		addr,addr2;

	/* FIXME: add checks for valid code ... */
	/* write pointers to kernel32.89 and kernel32.90 (+ordinal base of 1) */
	*(DWORD*)(thunk+0x35) = (DWORD)GetProcAddress(hkrnl32,(LPSTR)90);
	*(DWORD*)(thunk+0x6D) = (DWORD)GetProcAddress(hkrnl32,(LPSTR)89);

	
	if (!(addr = _loadthunk( dll16, thkbuf, dll32, NULL, len )))
		return 0;

	addr2 = PTR_SEG_TO_LIN(addr[1]);
	if (HIWORD(addr2))
		*(DWORD*)thunk = (DWORD)addr2;

	return addr2;
}

/***********************************************************************
 *		FT_PrologPrime			(KERNEL32.89)
 * 
 * This function is called from the relay code installed by
 * ThunkInitLSF. It replaces the location from where it was 
 * called by a standard FT_Prolog call stub (which is 'primed'
 * by inserting the correct target table pointer).
 * Finally, it calls that stub.
 * 
 * Input:  ECX    target number + flags (passed through to FT_Prolog)
 *        (ESP)   offset of location where target table pointer 
 *                is stored, relative to the start of the relay code
 *        (ESP+4) pointer to start of relay code
 *                (this is where the FT_Prolog call stub gets written to)
 * 
 * Note: The two DWORD arguments get popped from the stack.
 *        
 */
REGS_ENTRYPOINT(FT_PrologPrime)
{
    DWORD  targetTableOffset = STACK32_POP(context);
    LPBYTE relayCode = (LPBYTE)STACK32_POP(context);
    DWORD *targetTable = *(DWORD **)(relayCode+targetTableOffset);
    DWORD  targetNr = LOBYTE(ECX_reg(context));

    _write_ftprolog(relayCode, targetTable);

    /* We should actually call the relay code now, */
    /* but we skip it and go directly to FT_Prolog */
    EDX_reg(context) = targetTable[targetNr];
    __regs_FT_Prolog(context);
}

/***********************************************************************
 *		QT_ThunkPrime			(KERNEL32.90)
 *
 * This function corresponds to FT_PrologPrime, but installs a 
 * call stub for QT_Thunk instead.
 *
 * Input: (EBP-4) target number (passed through to QT_Thunk)
 *         EDX    target table pointer location offset
 *         EAX    start of relay code
 *      
 */
REGS_ENTRYPOINT(QT_ThunkPrime)
{
    DWORD  targetTableOffset = EDX_reg(context);
    LPBYTE relayCode = (LPBYTE)EAX_reg(context);
    DWORD *targetTable = *(DWORD **)(relayCode+targetTableOffset);
    DWORD  targetNr = LOBYTE(*(DWORD *)(EBP_reg(context) - 4));

    _write_qtthunk(relayCode, targetTable);

    /* We should actually call the relay code now, */
    /* but we skip it and go directly to QT_Thunk */
    EDX_reg(context) = targetTable[targetNr];
    __regs_QT_Thunk(context);
}

/***********************************************************************
 *							(KERNEL32.46)
 * Another thunkbuf link routine.
 * The start of the thunkbuf looks like this:
 * 	00: DWORD	length
 *	04: SEGPTR	address for thunkbuffer pointer
 * [ok probably]
 */
VOID WINAPI ThunkInitSL(
	LPBYTE thunk,		/* [in] start of thunkbuffer */
	LPCSTR thkbuf,		/* [in] name/ordinal of thunkbuffer in win16 dll */
	DWORD len,		/* [in] length of thunkbuffer */
	LPCSTR dll16,		/* [in] name of win16 dll containing the thkbuf */
	LPCSTR dll32		/* [in] win32 dll. FIXME: strange, unused */
) {
	LPDWORD		addr;

	if (!(addr = _loadthunk( dll16, thkbuf, dll32, NULL, len )))
		return;

	*(DWORD*)PTR_SEG_TO_LIN(addr[1]) = (DWORD)thunk;
}

/**********************************************************************
 *           SSInit		KERNEL.700
 * RETURNS
 *	TRUE for success.
 */
BOOL WINAPI SSInit16()
{
    return TRUE;
}

/**********************************************************************
 *           SSOnBigStack	KERNEL32.87
 * Check if thunking is initialized (ss selector set up etc.)
 * We do that differently, so just return TRUE.
 * [ok]
 * RETURNS
 *	TRUE for success.
 */
BOOL WINAPI SSOnBigStack()
{
    TRACE(thunk, "Yes, thunking is initialized\n");
    return TRUE;
}

/**********************************************************************
 *           SSCall
 * One of the real thunking functions. This one seems to be for 32<->32
 * thunks. It should probably be capable of crossing processboundaries.
 *
 * And YES, I've seen nr=48 (somewhere in the Win95 32<->16 OLE coupling)
 * [ok]
 */
DWORD WINAPIV SSCall(
	DWORD nr,	/* [in] number of argument bytes */
	DWORD flags,	/* [in] FIXME: flags ? */
	FARPROC fun,	/* [in] function to call */
	...		/* [in/out] arguments */
) {
    DWORD i,ret;
    DWORD *args = ((DWORD *)&fun) + 1;

    if(TRACE_ON(thunk)){
      dbg_decl_str(thunk, 256);
      for (i=0;i<nr/4;i++) 
	dsprintf(thunk,"0x%08lx,",args[i]);
      TRACE(thunk,"(%ld,0x%08lx,%p,[%s])\n",
		    nr,flags,fun,dbg_str(thunk));
    }
    switch (nr) {
    case 0:	ret = fun();
		break;
    case 4:	ret = fun(args[0]);
		break;
    case 8:	ret = fun(args[0],args[1]);
		break;
    case 12:	ret = fun(args[0],args[1],args[2]);
		break;
    case 16:	ret = fun(args[0],args[1],args[2],args[3]);
		break;
    case 20:	ret = fun(args[0],args[1],args[2],args[3],args[4]);
		break;
    case 24:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5]);
		break;
    case 28:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
		break;
    case 32:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
		break;
    case 36:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
		break;
    case 40:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
		break;
    case 44:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
		break;
    case 48:	ret = fun(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
		break;
    default:
	WARN(thunk,"Unsupported nr of arguments, %ld\n",nr);
	ret = 0;
	break;

    }
    TRACE(thunk," returning %ld ...\n",ret);
    return ret;
}

/**********************************************************************
 *           W32S_BackTo32                      (KERNEL32.51)
 */
REGS_ENTRYPOINT(W32S_BackTo32)
{
    LPDWORD stack = (LPDWORD)ESP_reg( context );
    FARPROC proc = (FARPROC) stack[0];

    EAX_reg( context ) = proc( stack[2], stack[3], stack[4], stack[5], stack[6],
                               stack[7], stack[8], stack[9], stack[10], stack[11] );

    EIP_reg( context ) = stack[1];
}

/**********************************************************************
 *			AllocSLCallback		(KERNEL32)
 *
 * Win95 uses some structchains for callbacks. It allocates them
 * in blocks of 100 entries, size 32 bytes each, layout:
 * blockstart:
 * 	0:	PTR	nextblockstart
 *	4:	entry	*first;
 *	8:	WORD	sel ( start points to blockstart)
 *	A:	WORD	unknown
 * 100xentry:
 *	00..17:		Code
 *	18:	PDB	*owning_process;
 *	1C:	PTR	blockstart
 *
 * We ignore this for now. (Just a note for further developers)
 * FIXME: use this method, so we don't waste selectors...
 *
 * Following code is then generated by AllocSLCallback. The code is 16 bit, so
 * the 0x66 prefix switches from word->long registers.
 *
 *	665A		pop	edx 
 *	6668x arg2 x 	pushl	<arg2>
 *	6652		push	edx
 *	EAx arg1 x	jmpf	<arg1>
 *
 * returns the startaddress of this thunk.
 *
 * Note, that they look very similair to the ones allocates by THUNK_Alloc.
 * RETURNS
 *	segmented pointer to the start of the thunk
 */
DWORD WINAPI
AllocSLCallback(
	DWORD finalizer,	/* [in] finalizer function */
	DWORD callback		/* [in] callback function */
) {
	LPBYTE	x,thunk = HeapAlloc( GetProcessHeap(), 0, 32 );
	WORD	sel;

	x=thunk;
	*x++=0x66;*x++=0x5a;				/* popl edx */
	*x++=0x66;*x++=0x68;*(DWORD*)x=finalizer;x+=4;	/* pushl finalizer */
	*x++=0x66;*x++=0x52;				/* pushl edx */
	*x++=0xea;*(DWORD*)x=callback;x+=4;		/* jmpf callback */

	*(PDB**)(thunk+18) = PROCESS_Current();

	sel = SELECTOR_AllocBlock( thunk , 32, SEGMENT_CODE, FALSE, FALSE );
	return (sel<<16)|0;
}

/**********************************************************************
 * 		FreeSLCallback		(KERNEL32.274)
 * Frees the specified 16->32 callback
 */
void WINAPI
FreeSLCallback(
	DWORD x	/* [in] 16 bit callback (segmented pointer?) */
) {
	FIXME(win32,"(0x%08lx): stub\n",x);
}


/**********************************************************************
 * 		GetTEBSelectorFS	(KERNEL.475)
 * 	Set the 16-bit %fs to the 32-bit %fs (current TEB selector)
 */
VOID WINAPI GetTEBSelectorFS16( CONTEXT *context ) 
{
    GET_FS( FS_reg(context) );
}

/**********************************************************************
 * 		KERNEL_431		(KERNEL.431)
 *		IsPeFormat		(W32SYS.2)
 * Checks the passed filename if it is a PE format executeable
 * RETURNS
 *  TRUE, if it is.
 *  FALSE if not.
 */
BOOL16 WINAPI IsPeFormat16(
	LPSTR	fn,	/* [in] filename to executeable */
	HFILE16 hf16	/* [in] open file, if filename is NULL */
) {
	IMAGE_DOS_HEADER	mzh;
        HFILE                 hf=FILE_GetHandle(hf16);
	OFSTRUCT		ofs;
	DWORD			xmagic;

	if (fn) {
		hf = OpenFile(fn,&ofs,OF_READ);
		if (hf==HFILE_ERROR)
			return FALSE;
	}
	_llseek(hf,0,SEEK_SET);
	if (sizeof(mzh)!=_lread(hf,&mzh,sizeof(mzh))) {
		_lclose(hf);
		return FALSE;
	}
	if (mzh.e_magic!=IMAGE_DOS_SIGNATURE) {
		WARN(dosmem,"File has not got dos signature!\n");
		_lclose(hf);
		return FALSE;
	}
	_llseek(hf,mzh.e_lfanew,SEEK_SET);
	if (sizeof(DWORD)!=_lread(hf,&xmagic,sizeof(DWORD))) {
		_lclose(hf);
		return FALSE;
	}
	_lclose(hf);
	return (xmagic == IMAGE_NT_SIGNATURE);
}

/***********************************************************************
 *           WOWHandle32			(KERNEL32.57)(WOW32.16)
 * Converts a win16 handle of type into the respective win32 handle.
 * We currently just return this handle, since most handles are the same
 * for win16 and win32.
 * RETURNS
 *	The new handle
 */
HANDLE WINAPI WOWHandle32(
	WORD handle,		/* [in] win16 handle */
	WOW_HANDLE_TYPE type	/* [in] handle type */
) {
	TRACE(win32,"(0x%04x,%d)\n",handle,type);
	return (HANDLE)handle;
}

/***********************************************************************
 *           K32Thk1632Prolog			(KERNEL32.492)
 */
REGS_ENTRYPOINT(K32Thk1632Prolog)
{
   LPBYTE code = (LPBYTE)EIP_reg(context) - 5;

   /* Arrrgh! SYSTHUNK.DLL just has to re-implement another method
      of 16->32 thunks instead of using one of the standard methods!
      This means that SYSTHUNK.DLL itself switches to a 32-bit stack,
      and does a far call to the 32-bit code segment of OLECLI32/OLESVR32.
      Unfortunately, our CallTo/CallFrom mechanism is therefore completely
      bypassed, which means it will crash the next time the 32-bit OLE 
      code thunks down again to 16-bit (this *will* happen!).

      The following hack tries to recognize this situation.
      This is possible since the called stubs in OLECLI32/OLESVR32 all
      look exactly the same:
        00   E8xxxxxxxx    call K32Thk1632Prolog
        05   FF55FC        call [ebp-04]
        08   E8xxxxxxxx    call K32Thk1632Epilog
        0D   66CB          retf

      If we recognize this situation, we try to simulate the actions
      of our CallTo/CallFrom mechanism by copying the 16-bit stack
      to our 32-bit stack, creating a proper STACK16FRAME and 
      updating thdb->cur_stack. */ 

   if (   code[5] == 0xFF && code[6] == 0x55 && code[7] == 0xFC
       && code[13] == 0x66 && code[14] == 0xCB)
   {
      WORD  stackSel  = NtCurrentTeb()->stack_sel;
      DWORD stackBase = GetSelectorBase(stackSel);

      THDB *thdb = THREAD_Current();
      DWORD argSize = EBP_reg(context) - ESP_reg(context);
      char *stack16 = (char *)ESP_reg(context);
      char *stack32 = (char *)thdb->cur_stack - argSize;
      STACK16FRAME *frame16 = (STACK16FRAME *)stack16 - 1;

      TRACE(thunk, "before SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), thdb->cur_stack);

      memset(frame16, '\0', sizeof(STACK16FRAME));
      frame16->frame32 = (STACK32FRAME *)thdb->cur_stack;
      frame16->ebp = EBP_reg(context);

      memcpy(stack32, stack16, argSize);
      thdb->cur_stack = PTR_SEG_OFF_TO_SEGPTR(stackSel, (DWORD)frame16 - stackBase);

      ESP_reg(context) = (DWORD)stack32;
      EBP_reg(context) = ESP_reg(context) + argSize;

      TRACE(thunk, "after  SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), thdb->cur_stack);
   }

   SYSLEVEL_ReleaseWin16Lock();
}

/***********************************************************************
 *           K32Thk1632Epilog			(KERNEL32.491)
 */
REGS_ENTRYPOINT(K32Thk1632Epilog)
{
   LPBYTE code = (LPBYTE)EIP_reg(context) - 13;

   SYSLEVEL_RestoreWin16Lock();

   /* We undo the SYSTHUNK hack if necessary. See K32Thk1632Prolog. */

   if (   code[5] == 0xFF && code[6] == 0x55 && code[7] == 0xFC
       && code[13] == 0x66 && code[14] == 0xCB)
   {
      THDB *thdb = THREAD_Current();
      STACK16FRAME *frame16 = (STACK16FRAME *)PTR_SEG_TO_LIN(thdb->cur_stack);
      char *stack16 = (char *)(frame16 + 1);
      DWORD argSize = frame16->ebp - (DWORD)stack16;
      char *stack32 = (char *)frame16->frame32 - argSize;

      DWORD nArgsPopped = ESP_reg(context) - (DWORD)stack32;

      TRACE(thunk, "before SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), thdb->cur_stack);

      thdb->cur_stack = (DWORD)frame16->frame32;

      ESP_reg(context) = (DWORD)stack16 + nArgsPopped;
      EBP_reg(context) = frame16->ebp;

      TRACE(thunk, "after  SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), thdb->cur_stack);
   }
}

/***********************************************************************
 *           UpdateResource32A                 (KERNEL32.707)
 */
BOOL WINAPI UpdateResourceA(
  HANDLE  hUpdate,
  LPCSTR  lpType,
  LPCSTR  lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData) {

  FIXME(win32, ": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/***********************************************************************
 *           UpdateResource32W                 (KERNEL32.708)
 */
BOOL WINAPI UpdateResourceW(
  HANDLE  hUpdate,
  LPCWSTR lpType,
  LPCWSTR lpName,
  WORD    wLanguage,
  LPVOID  lpData,
  DWORD   cbData) {

  FIXME(win32, ": stub\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/***********************************************************************
 *           WaitNamedPipe32A                 [KERNEL32.725]
 */
BOOL WINAPI WaitNamedPipeA (LPCSTR lpNamedPipeName, DWORD nTimeOut)
{	FIXME (win32,"%s 0x%08lx\n",lpNamedPipeName,nTimeOut);
	SetLastError(ERROR_PIPE_NOT_CONNECTED);
	return FALSE;
}
/***********************************************************************
 *           WaitNamedPipe32W                 [KERNEL32.726]
 */
BOOL WINAPI WaitNamedPipeW (LPCWSTR lpNamedPipeName, DWORD nTimeOut)
{	FIXME (win32,"%s 0x%08lx\n",debugstr_w(lpNamedPipeName),nTimeOut);
	SetLastError(ERROR_PIPE_NOT_CONNECTED);
	return FALSE;
}

/*********************************************************************
 *                   PK16FNF [KERNEL32.91]
 *
 *  This routine fills in the supplied 13-byte (8.3 plus terminator)
 *  string buffer with the 8.3 filename of a recently loaded 16-bit
 *  module.  It is unknown exactly what modules trigger this
 *  mechanism or what purpose this serves.  Win98 Explorer (and
 *  probably also Win95 with IE 4 shell integration) calls this
 *  several times during initialization.
 *
 *  FIXME: find out what this really does and make it work.
 */
void WINAPI PK16FNF(LPSTR strPtr)
{
       FIXME(win32, "(%p): stub\n", strPtr);

       /* fill in a fake filename that'll be easy to recognize */
       lstrcpyA(strPtr, "WINESTUB.FIX");
}
