/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1997, 1998 Marcus Meissner
 * Copyright 1998       Ulrich Weigand
 *
 */

#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wine/winbase16.h"

#include "builtin16.h"
#include "callback.h"
#include "debugtools.h"
#include "flatthunk.h"
#include "heap.h"
#include "module.h"
#include "selectors.h"
#include "stackframe.h"
#include "syslevel.h"
#include "task.h"

DEFAULT_DEBUG_CHANNEL(thunk);


/***********************************************************************
 *                                                                     *
 *                 Win95 internal thunks                               *
 *                                                                     *
 ***********************************************************************/

/***********************************************************************
 *           LogApiThk    (KERNEL.423)
 */
void WINAPI LogApiThk( LPSTR func )
{
    TRACE( "%s\n", debugstr_a(func) );
}

/***********************************************************************
 *           LogApiThkLSF    (KERNEL32.42)
 * 
 * NOTE: needs to preserve all registers!
 */
void WINAPI LogApiThkLSF( LPSTR func, CONTEXT86 *context )
{
    TRACE( "%s\n", debugstr_a(func) );
}

/***********************************************************************
 *           LogApiThkSL    (KERNEL32.44)
 * 
 * NOTE: needs to preserve all registers!
 */
void WINAPI LogApiThkSL( LPSTR func, CONTEXT86 *context )
{
    TRACE( "%s\n", debugstr_a(func) );
}

/***********************************************************************
 *           LogCBThkSL    (KERNEL32.47)
 * 
 * NOTE: needs to preserve all registers!
 */
void WINAPI LogCBThkSL( LPSTR func, CONTEXT86 *context )
{
    TRACE( "%s\n", debugstr_a(func) );
}

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
        ERR("(%s, %s, %s): Unable to load '%s', error %d\n",
                   module, func, module32, module, hmod);
        return 0;
    }

    if (   !(ordinal = NE_GetOrdinal(hmod, func))
        || !(TD16 = PTR_SEG_TO_LIN(NE_GetEntryPointEx(hmod, ordinal, FALSE))))
    {
        ERR("Unable to find thunk data '%s' in %s, required by %s (conflicting/incorrect DLL versions !?).\n",
                   func, module, module32);
        return 0;
    }

    if (TD32 && memcmp(TD16->magic, TD32->magic, 4))
    {
        ERR("(%s, %s, %s): Bad magic %c%c%c%c (should be %c%c%c%c)\n",
                   module, func, module32, 
                   TD16->magic[0], TD16->magic[1], TD16->magic[2], TD16->magic[3],
                   TD32->magic[0], TD32->magic[1], TD32->magic[2], TD32->magic[3]);
        return 0;
    }

    if (TD32 && TD16->checksum != TD32->checksum)
    {
        ERR("(%s, %s, %s): Wrong checksum %08lx (should be %08lx)\n",
                   module, func, module32, TD16->checksum, TD32->checksum);
        return 0;
    }

    if (!TD32 && checksum && checksum != *(LPDWORD)TD16)
    {
        ERR("(%s, %s, %s): Wrong checksum %08lx (should be %08lx)\n",
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

    if (!strncmp(TD->magic, "SL01", 4))
    {
        directionSL = TRUE;

        TRACE("SL01 thunk %s (%lx) <- %s (%s), Reason: %ld\n",
                     module32, (DWORD)TD, module16, thunkfun16, dwReason);
    }
    else if (!strncmp(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE("LS01 thunk %s (%lx) -> %s (%s), Reason: %ld\n",
                     module32, (DWORD)TD, module16, thunkfun16, dwReason);
    }
    else
    {
        ERR("Invalid magic %c%c%c%c\n", 
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
                    ERR("ThunkConnect16 was not called!\n");
                    return 0;
                }

                SL32->data = SL16->fpData;

                tdb = HeapAlloc(GetProcessHeap(), 0, sizeof(*tdb));
                tdb->process = GetCurrentProcessId();
                tdb->targetTable = (DWORD *)(thunkfun16 + SL32->offsetTargetTable);

                tdb->next = SL32->data->targetDB;   /* FIXME: not thread-safe! */
                SL32->data->targetDB = tdb;

                TRACE("Process %08lx allocated TargetDB entry for ThunkDataSL %08lx\n", 
                             GetCurrentProcessId(), (DWORD)SL32->data);
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
 * The 16 bit arguments start at ESP.
 * The number of 16bit argument bytes is EBP-ESP-0x40 (64 Byte thunksetup).
 * [ok]
 */
void WINAPI QT_Thunk( CONTEXT86 *context )
{
    CONTEXT86 context16;
    DWORD argsize;

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(EDX_reg(context));
    EIP_reg(&context16) = LOWORD(EDX_reg(context));
    EBP_reg(&context16) = OFFSETOF( NtCurrentTeb()->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = EBP_reg(context)-ESP_reg(context)-0x40;

    memcpy( (LPBYTE)CURRENT_STACK16 - argsize,
            (LPBYTE)ESP_reg(context), argsize );

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
 *  ESP is EBP-64 after return.
 *         
 */

void WINAPI FT_Prolog( CONTEXT86 *context )
{
    /* Build stack frame */
    stack32_push(context, EBP_reg(context));
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

void WINAPI FT_Thunk( CONTEXT86 *context )
{
    DWORD mapESPrelative = *(DWORD *)(EBP_reg(context) - 20);
    DWORD callTarget     = *(DWORD *)(EBP_reg(context) - 52);

    CONTEXT86 context16;
    DWORD i, argsize;
    LPBYTE newstack, oldstack;

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(callTarget);
    EIP_reg(&context16) = LOWORD(callTarget);
    EBP_reg(&context16) = OFFSETOF( NtCurrentTeb()->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize  = EBP_reg(context)-ESP_reg(context)-0x40;
    newstack = (LPBYTE)CURRENT_STACK16 - argsize;
    oldstack = (LPBYTE)ESP_reg(context);

    memcpy( newstack, oldstack, argsize );

    for (i = 0; i < 32; i++)	/* NOTE: What about > 32 arguments? */
	if (mapESPrelative & (1 << i))
	{
	    SEGPTR *arg = (SEGPTR *)(newstack + 2*i);
	    *arg = PTR_SEG_OFF_TO_SEGPTR(SELECTOROF(NtCurrentTeb()->cur_stack), 
                                         OFFSETOF(NtCurrentTeb()->cur_stack) - argsize
					 + (*(LPBYTE *)arg - oldstack));
	}

    EAX_reg(context) = Callbacks->CallRegisterShortProc( &context16, argsize );
    EDX_reg(context) = HIWORD(EAX_reg(context));
    EAX_reg(context) = LOWORD(EAX_reg(context));

    /* Copy modified buffers back to 32-bit stack */
    memcpy( oldstack, newstack, argsize );
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

static void FT_Exit(CONTEXT86 *context, int nPopArgs)
{
    /* Return value is in EBX */
    EAX_reg(context) = EBX_reg(context);

    /* Restore EBX, ESI, and EDI registers */
    EBX_reg(context) = *(DWORD *)(EBP_reg(context) -  4);
    ESI_reg(context) = *(DWORD *)(EBP_reg(context) -  8);
    EDI_reg(context) = *(DWORD *)(EBP_reg(context) - 12);

    /* Clean up stack frame */
    ESP_reg(context) = EBP_reg(context);
    EBP_reg(context) = stack32_pop(context);

    /* Pop return address to CALLER of thunk code */
    EIP_reg(context) = stack32_pop(context);
    /* Remove arguments */
    ESP_reg(context) += nPopArgs;
}

/***********************************************************************
 *		FT_Exit0 (KERNEL32.218)
 */
void WINAPI FT_Exit0 (CONTEXT86 *context) { FT_Exit(context,  0); }

/***********************************************************************
 *		FT_Exit4 (KERNEL32.219)
 */
void WINAPI FT_Exit4 (CONTEXT86 *context) { FT_Exit(context,  4); }

/***********************************************************************
 *		FT_Exit8 (KERNEL32.220)
 */
void WINAPI FT_Exit8 (CONTEXT86 *context) { FT_Exit(context,  8); }

/***********************************************************************
 *		FT_Exit12 (KERNEL32.221)
 */
void WINAPI FT_Exit12(CONTEXT86 *context) { FT_Exit(context, 12); }

/***********************************************************************
 *		FT_Exit16 (KERNEL32.222)
 */
void WINAPI FT_Exit16(CONTEXT86 *context) { FT_Exit(context, 16); }

/***********************************************************************
 *		FT_Exit20 (KERNEL32.223)
 */
void WINAPI FT_Exit20(CONTEXT86 *context) { FT_Exit(context, 20); }

/***********************************************************************
 *		FT_Exit24 (KERNEL32.224)
 */
void WINAPI FT_Exit24(CONTEXT86 *context) { FT_Exit(context, 24); }

/***********************************************************************
 *		FT_Exit28 (KERNEL32.225)
 */
void WINAPI FT_Exit28(CONTEXT86 *context) { FT_Exit(context, 28); }

/***********************************************************************
 *		FT_Exit32 (KERNEL32.226)
 */
void WINAPI FT_Exit32(CONTEXT86 *context) { FT_Exit(context, 32); }

/***********************************************************************
 *		FT_Exit36 (KERNEL32.227)
 */
void WINAPI FT_Exit36(CONTEXT86 *context) { FT_Exit(context, 36); }

/***********************************************************************
 *		FT_Exit40 (KERNEL32.228)
 */
void WINAPI FT_Exit40(CONTEXT86 *context) { FT_Exit(context, 40); }

/***********************************************************************
 *		FT_Exit44 (KERNEL32.229)
 */
void WINAPI FT_Exit44(CONTEXT86 *context) { FT_Exit(context, 44); }

/***********************************************************************
 *		FT_Exit48 (KERNEL32.230)
 */
void WINAPI FT_Exit48(CONTEXT86 *context) { FT_Exit(context, 48); }

/***********************************************************************
 *		FT_Exit52 (KERNEL32.231)
 */
void WINAPI FT_Exit52(CONTEXT86 *context) { FT_Exit(context, 52); }

/***********************************************************************
 *		FT_Exit56 (KERNEL32.232)
 */
void WINAPI FT_Exit56(CONTEXT86 *context) { FT_Exit(context, 56); }

/***********************************************************************
 * 		ThunkInitLS 	(KERNEL32.43)
 * A thunkbuffer link routine 
 * The thunkbuf looks like:
 *
 *	00: DWORD	length		? don't know exactly
 *	04: SEGPTR	ptr		? where does it point to?
 * The pointer ptr is written into the first DWORD of 'thunk'.
 * (probably correctly implemented)
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
void WINAPI Common32ThkLS( CONTEXT86 *context )
{
    CONTEXT86 context16;
    DWORD argsize;

    memcpy(&context16,context,sizeof(context16));

    DI_reg(&context16)  = CX_reg(context);
    CS_reg(&context16)  = HIWORD(EAX_reg(context));
    EIP_reg(&context16) = LOWORD(EAX_reg(context));
    EBP_reg(&context16) = OFFSETOF( NtCurrentTeb()->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = HIWORD(EDX_reg(context)) * 4;

    /* FIXME: hack for stupid USER32 CallbackGlueLS routine */
    if (EDX_reg(context) == EIP_reg(context))
        argsize = 6 * 4;

    memcpy( (LPBYTE)CURRENT_STACK16 - argsize,
            (LPBYTE)ESP_reg(context), argsize );

    EAX_reg(context) = Callbacks->CallRegisterLongProc(&context16, argsize + 32);

    /* Clean up caller's stack frame */
    ESP_reg(context) += argsize;
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
void WINAPI OT_32ThkLSF( CONTEXT86 *context )
{
    CONTEXT86 context16;
    DWORD argsize;

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(EDX_reg(context));
    EIP_reg(&context16) = LOWORD(EDX_reg(context));
    EBP_reg(&context16) = OFFSETOF( NtCurrentTeb()->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize = 2 * *(WORD *)ESP_reg(context) + 2;

    memcpy( (LPBYTE)CURRENT_STACK16 - argsize,
            (LPBYTE)ESP_reg(context), argsize );

    EAX_reg(context) = Callbacks->CallRegisterShortProc(&context16, argsize);

    memcpy( (LPBYTE)ESP_reg(context), 
            (LPBYTE)CURRENT_STACK16 - argsize, argsize );
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
 * Note: The two DWORD arguments get popped off the stack.
 *        
 */
void WINAPI FT_PrologPrime( CONTEXT86 *context )
{
    DWORD  targetTableOffset;
    LPBYTE relayCode;

    /* Compensate for the fact that the Wine register relay code thought
       we were being called, although we were in fact jumped to */
    ESP_reg(context) -= 4;

    /* Write FT_Prolog call stub */
    targetTableOffset = stack32_pop(context);
    relayCode = (LPBYTE)stack32_pop(context);
    _write_ftprolog( relayCode, *(DWORD **)(relayCode+targetTableOffset) );

    /* Jump to the call stub just created */
    EIP_reg(context) = (DWORD)relayCode;
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
void WINAPI QT_ThunkPrime( CONTEXT86 *context )
{
    DWORD  targetTableOffset;
    LPBYTE relayCode;

    /* Compensate for the fact that the Wine register relay code thought
       we were being called, although we were in fact jumped to */
    ESP_reg(context) -= 4;

    /* Write QT_Thunk call stub */
    targetTableOffset = EDX_reg(context);
    relayCode = (LPBYTE)EAX_reg(context);
    _write_qtthunk( relayCode, *(DWORD **)(relayCode+targetTableOffset) );

    /* Jump to the call stub just created */
    EIP_reg(context) = (DWORD)relayCode;
}

/***********************************************************************
 *		ThunkInitSL (KERNEL32.46)
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
    TRACE("Yes, thunking is initialized\n");
    return TRUE;
}

/**********************************************************************
 *           SSConfirmSmallStack     KERNEL.704
 *
 * Abort if not on small stack.
 *
 * This must be a register routine as it has to preserve *all* registers.
 */
void WINAPI SSConfirmSmallStack( CONTEXT86 *context )
{
    /* We are always on the small stack while in 16-bit code ... */
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

    if(TRACE_ON(thunk))
    {
      DPRINTF("(%ld,0x%08lx,%p,[",nr,flags,fun);
      for (i=0;i<nr/4;i++) 
          DPRINTF("0x%08lx,",args[i]);
      DPRINTF("])\n");
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
	WARN("Unsupported nr of arguments, %ld\n",nr);
	ret = 0;
	break;

    }
    TRACE(" returning %ld ...\n",ret);
    return ret;
}

/**********************************************************************
 *           W32S_BackTo32                      (KERNEL32.51)
 */
void WINAPI W32S_BackTo32( CONTEXT86 *context )
{
    LPDWORD stack = (LPDWORD)ESP_reg( context );
    FARPROC proc = (FARPROC)EIP_reg(context);

    EAX_reg( context ) = proc( stack[1], stack[2], stack[3], stack[4], stack[5],
                               stack[6], stack[7], stack[8], stack[9], stack[10] );

    EIP_reg( context ) = stack32_pop(context);
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

	*(DWORD*)(thunk+18) = GetCurrentProcessId();

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
	FIXME("(0x%08lx): stub\n",x);
}


/**********************************************************************
 * 		GetTEBSelectorFS	(KERNEL.475)
 * 	Set the 16-bit %fs to the 32-bit %fs (current TEB selector)
 */
void WINAPI GetTEBSelectorFS16(void) 
{
    CURRENT_STACK16->fs = __get_fs();
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
	OFSTRUCT		ofs;
	DWORD			xmagic;

	if (fn) {
		hf16 = OpenFile16(fn,&ofs,OF_READ);
		if (hf16==HFILE_ERROR16)
			return FALSE;
	}
	_llseek16(hf16,0,SEEK_SET);
	if (sizeof(mzh)!=_lread16(hf16,&mzh,sizeof(mzh))) {
		_lclose(hf16);
		return FALSE;
	}
	if (mzh.e_magic!=IMAGE_DOS_SIGNATURE) {
		WARN("File has not got dos signature!\n");
		_lclose(hf16);
		return FALSE;
	}
	_llseek16(hf16,mzh.e_lfanew,SEEK_SET);
	if (sizeof(DWORD)!=_lread16(hf16,&xmagic,sizeof(DWORD))) {
		_lclose(hf16);
		return FALSE;
	}
	_lclose(hf16);
	return (xmagic == IMAGE_NT_SIGNATURE);
}


/***********************************************************************
 *           K32Thk1632Prolog			(KERNEL32.492)
 */
void WINAPI K32Thk1632Prolog( CONTEXT86 *context )
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
      updating cur_stack. */ 

   if (   code[5] == 0xFF && code[6] == 0x55 && code[7] == 0xFC
       && code[13] == 0x66 && code[14] == 0xCB)
   {
      WORD  stackSel  = NtCurrentTeb()->stack_sel;
      DWORD stackBase = GetSelectorBase(stackSel);

      DWORD argSize = EBP_reg(context) - ESP_reg(context);
      char *stack16 = (char *)ESP_reg(context) - 4;
      char *stack32 = (char *)NtCurrentTeb()->cur_stack - argSize;
      STACK16FRAME *frame16 = (STACK16FRAME *)stack16 - 1;

      TRACE("before SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), NtCurrentTeb()->cur_stack);

      memset(frame16, '\0', sizeof(STACK16FRAME));
      frame16->frame32 = (STACK32FRAME *)NtCurrentTeb()->cur_stack;
      frame16->ebp = EBP_reg(context);

      memcpy(stack32, stack16, argSize);
      NtCurrentTeb()->cur_stack = PTR_SEG_OFF_TO_SEGPTR(stackSel, (DWORD)frame16 - stackBase);

      ESP_reg(context) = (DWORD)stack32 + 4;
      EBP_reg(context) = ESP_reg(context) + argSize;

      TRACE("after  SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), NtCurrentTeb()->cur_stack);
   }

   SYSLEVEL_ReleaseWin16Lock();
}

/***********************************************************************
 *           K32Thk1632Epilog			(KERNEL32.491)
 */
void WINAPI K32Thk1632Epilog( CONTEXT86 *context )
{
   LPBYTE code = (LPBYTE)EIP_reg(context) - 13;

   SYSLEVEL_RestoreWin16Lock();

   /* We undo the SYSTHUNK hack if necessary. See K32Thk1632Prolog. */

   if (   code[5] == 0xFF && code[6] == 0x55 && code[7] == 0xFC
       && code[13] == 0x66 && code[14] == 0xCB)
   {
      STACK16FRAME *frame16 = (STACK16FRAME *)PTR_SEG_TO_LIN(NtCurrentTeb()->cur_stack);
      char *stack16 = (char *)(frame16 + 1);
      DWORD argSize = frame16->ebp - (DWORD)stack16;
      char *stack32 = (char *)frame16->frame32 - argSize;

      DWORD nArgsPopped = ESP_reg(context) - (DWORD)stack32;

      TRACE("before SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), NtCurrentTeb()->cur_stack);

      NtCurrentTeb()->cur_stack = (DWORD)frame16->frame32;

      ESP_reg(context) = (DWORD)stack16 + nArgsPopped;
      EBP_reg(context) = frame16->ebp;

      TRACE("after  SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %08lx\n",
                   EBP_reg(context), ESP_reg(context), NtCurrentTeb()->cur_stack);
   }
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
       FIXME("(%p): stub\n", strPtr);

       /* fill in a fake filename that'll be easy to recognize */
       lstrcpyA(strPtr, "WINESTUB.FIX");
}

/***********************************************************************
 * 16->32 Flat Thunk routines:
 */

/***********************************************************************
 *              ThunkConnect16          (KERNEL.651)
 * Connects a 32bit and a 16bit thunkbuffer.
 */
UINT WINAPI ThunkConnect16(
        LPSTR module16,              /* [in] name of win16 dll */
        LPSTR module32,              /* [in] name of win32 dll */
        HINSTANCE16 hInst16,         /* [in] hInst of win16 dll */
        DWORD dwReason,              /* [in] initialisation argument */
        struct ThunkDataCommon *TD,  /* [in/out] thunkbuffer */
        LPSTR thunkfun32,            /* [in] win32 thunkfunction */
        WORD cs                      /* [in] CS of win16 dll */
) {
    BOOL directionSL;

    if (!strncmp(TD->magic, "SL01", 4))
    {
        directionSL = TRUE;

        TRACE("SL01 thunk %s (%lx) -> %s (%s), Reason: %ld\n",
              module16, (DWORD)TD, module32, thunkfun32, dwReason);
    }
    else if (!strncmp(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE("LS01 thunk %s (%lx) <- %s (%s), Reason: %ld\n",
              module16, (DWORD)TD, module32, thunkfun32, dwReason);
    }
    else
    {
        ERR("Invalid magic %c%c%c%c\n",
            TD->magic[0], TD->magic[1], TD->magic[2], TD->magic[3]);
        return 0;
    }

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
            if (directionSL)
            {
                struct ThunkDataSL16 *SL16 = (struct ThunkDataSL16 *)TD;
                struct ThunkDataSL   *SL   = SL16->fpData;

                if (SL == NULL)
                {
                    SL = HeapAlloc(GetProcessHeap(), 0, sizeof(*SL));

                    SL->common   = SL16->common;
                    SL->flags1   = SL16->flags1;
                    SL->flags2   = SL16->flags2;

                    SL->apiDB    = PTR_SEG_TO_LIN(SL16->apiDatabase);
                    SL->targetDB = NULL;

                    lstrcpynA(SL->pszDll16, module16, 255);
                    lstrcpynA(SL->pszDll32, module32, 255);

                    /* We should create a SEGPTR to the ThunkDataSL,
                       but since the contents are not in the original format,
                       any access to this by 16-bit code would crash anyway. */
                    SL16->spData = 0;
                    SL16->fpData = SL;
                }


                if (SL->flags2 & 0x80000000)
                {
                    TRACE("Preloading 32-bit library\n");
                    LoadLibraryA(module32);
                }
            }
            else
            {
                /* nothing to do */
            }
            break;

        case DLL_PROCESS_DETACH:
            /* FIXME: cleanup */
            break;
    }

    return 1;
}


/***********************************************************************
 *           C16ThkSL                           (KERNEL.630)
 */

void WINAPI C16ThkSL(CONTEXT86 *context)
{
    LPBYTE stub = PTR_SEG_TO_LIN(EAX_reg(context)), x = stub;
    WORD cs = __get_cs();
    WORD ds = __get_ds();

    /* We produce the following code:
     *
     *   mov ax, __FLATDS
     *   mov es, ax
     *   movzx ecx, cx
     *   mov edx, es:[ecx + $EDX]
     *   push bp
     *   push edx
     *   push dx
     *   push edx
     *   call __FLATCS:CallFrom16Thunk
     */

    *x++ = 0xB8; *((WORD *)x)++ = ds;
    *x++ = 0x8E; *x++ = 0xC0;
    *x++ = 0x66; *x++ = 0x0F; *x++ = 0xB7; *x++ = 0xC9;
    *x++ = 0x67; *x++ = 0x66; *x++ = 0x26; *x++ = 0x8B;
                 *x++ = 0x91; *((DWORD *)x)++ = EDX_reg(context);

    *x++ = 0x55;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x9A; *((DWORD *)x)++ = (DWORD)CallFrom16Thunk;
                              *((WORD *)x)++ = cs;

    /* Jump to the stub code just created */
    EIP_reg(context) = LOWORD(EAX_reg(context));
    CS_reg(context)  = HIWORD(EAX_reg(context));

    /* Since C16ThkSL got called by a jmp, we need to leave the
       original return address on the stack */
    ESP_reg(context) -= 4;
}

/***********************************************************************
 *           C16ThkSL01                         (KERNEL.631)
 */

void WINAPI C16ThkSL01(CONTEXT86 *context)
{
    LPBYTE stub = PTR_SEG_TO_LIN(EAX_reg(context)), x = stub;

    if (stub)
    {
        struct ThunkDataSL16 *SL16 = PTR_SEG_TO_LIN(EDX_reg(context));
        struct ThunkDataSL *td = SL16->fpData;

        DWORD procAddress = (DWORD)GetProcAddress16(GetModuleHandle16("KERNEL"), 631);
        WORD cs = __get_cs();

        if (!td)
        {
            ERR("ThunkConnect16 was not called!\n");
            return;
        }

        TRACE("Creating stub for ThunkDataSL %08lx\n", (DWORD)td);


        /* We produce the following code:
         *
         *   xor eax, eax
         *   mov edx, $td
         *   call C16ThkSL01
         *   push bp
         *   push edx
         *   push dx
         *   push edx
         *   call __FLATCS:CallFrom16Thunk
         */

        *x++ = 0x66; *x++ = 0x33; *x++ = 0xC0;
        *x++ = 0x66; *x++ = 0xBA; *((DWORD *)x)++ = (DWORD)td;
        *x++ = 0x9A; *((DWORD *)x)++ = procAddress;

        *x++ = 0x55;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x9A; *((DWORD *)x)++ = (DWORD)CallFrom16Thunk;
                                  *((WORD *)x)++ = cs;

        /* Jump to the stub code just created */
        EIP_reg(context) = LOWORD(EAX_reg(context));
        CS_reg(context)  = HIWORD(EAX_reg(context));

        /* Since C16ThkSL01 got called by a jmp, we need to leave the
           orginal return address on the stack */
        ESP_reg(context) -= 4;
    }
    else
    {
        struct ThunkDataSL *td = (struct ThunkDataSL *)EDX_reg(context);
        DWORD targetNr = CX_reg(context) / 4;
        struct SLTargetDB *tdb;

        TRACE("Process %08lx calling target %ld of ThunkDataSL %08lx\n",
              GetCurrentProcessId(), targetNr, (DWORD)td);

        for (tdb = td->targetDB; tdb; tdb = tdb->next)
            if (tdb->process == GetCurrentProcessId())
                break;

        if (!tdb)
        {
            TRACE("Loading 32-bit library %s\n", td->pszDll32);
            LoadLibraryA(td->pszDll32);

            for (tdb = td->targetDB; tdb; tdb = tdb->next)
                if (tdb->process == GetCurrentProcessId())
                    break;
        }

        if (tdb)
        {
            EDX_reg(context) = tdb->targetTable[targetNr];

            TRACE("Call target is %08lx\n", EDX_reg(context));
        }
        else
        {
            WORD *stack = PTR_SEG_OFF_TO_LIN(SS_reg(context), LOWORD(ESP_reg(context)));
            DX_reg(context) = HIWORD(td->apiDB[targetNr].errorReturnValue);
            AX_reg(context) = LOWORD(td->apiDB[targetNr].errorReturnValue);
            EIP_reg(context) = stack[2];
            CS_reg(context)  = stack[3];
            ESP_reg(context) += td->apiDB[targetNr].nrArgBytes + 4;

            ERR("Process %08lx did not ThunkConnect32 %s to %s\n",
                GetCurrentProcessId(), td->pszDll32, td->pszDll16);
        }
    }
}


/***********************************************************************
 * 16<->32 Thunklet/Callback API:
 */

#include "pshpack1.h"
typedef struct _THUNKLET
{
    BYTE        prefix_target;
    BYTE        pushl_target;
    DWORD       target;

    BYTE        prefix_relay;
    BYTE        pushl_relay;
    DWORD       relay;

    BYTE        jmp_glue;
    DWORD       glue;

    BYTE        type;
    HINSTANCE16 owner;
    struct _THUNKLET *next;
} THUNKLET;
#include "poppack.h"

#define THUNKLET_TYPE_LS  1
#define THUNKLET_TYPE_SL  2

static HANDLE  ThunkletHeap = 0;
static THUNKLET *ThunkletAnchor = NULL;

static FARPROC ThunkletSysthunkGlueLS = 0;
static SEGPTR    ThunkletSysthunkGlueSL = 0;

static FARPROC ThunkletCallbackGlueLS = 0;
static SEGPTR    ThunkletCallbackGlueSL = 0;

/***********************************************************************
 *           THUNK_Init
 */
BOOL THUNK_Init(void)
{
    LPBYTE thunk;

    ThunkletHeap = HeapCreate(HEAP_WINE_SEGPTR | HEAP_WINE_CODE16SEG, 0, 0);
    if (!ThunkletHeap) return FALSE;

    thunk = HeapAlloc( ThunkletHeap, 0, 5 );
    if (!thunk) return FALSE;
    
    ThunkletSysthunkGlueLS = (FARPROC)thunk;
    *thunk++ = 0x58;                             /* popl eax */
    *thunk++ = 0xC3;                             /* ret      */

    ThunkletSysthunkGlueSL = HEAP_GetSegptr( ThunkletHeap, 0, thunk );
    *thunk++ = 0x66; *thunk++ = 0x58;            /* popl eax */
    *thunk++ = 0xCB;                             /* lret     */

    return TRUE;
}

/***********************************************************************
 *     SetThunkletCallbackGlue             (KERNEL.560)
 */
void WINAPI SetThunkletCallbackGlue16( FARPROC glueLS, SEGPTR glueSL )
{
    ThunkletCallbackGlueLS = glueLS;
    ThunkletCallbackGlueSL = glueSL;
}


/***********************************************************************
 *     THUNK_FindThunklet
 */
THUNKLET *THUNK_FindThunklet( DWORD target, DWORD relay, 
                              DWORD glue, BYTE type ) 
{
    THUNKLET *thunk; 

    for (thunk = ThunkletAnchor; thunk; thunk = thunk->next)
        if (    thunk->type   == type
             && thunk->target == target
             && thunk->relay  == relay 
             && ( type == THUNKLET_TYPE_LS ?
                    ( thunk->glue == glue - (DWORD)&thunk->type )
                  : ( thunk->glue == glue ) ) )
            return thunk;

     return NULL;
}

/***********************************************************************
 *     THUNK_AllocLSThunklet
 */
FARPROC THUNK_AllocLSThunklet( SEGPTR target, DWORD relay, 
                                 FARPROC glue, HTASK16 owner ) 
{
    THUNKLET *thunk = THUNK_FindThunklet( (DWORD)target, relay, (DWORD)glue,
                                          THUNKLET_TYPE_LS );
    if (!thunk)
    {
        TDB *pTask = (TDB*)GlobalLock16( owner );

        if ( !(thunk = HeapAlloc( ThunkletHeap, 0, sizeof(THUNKLET) )) )
            return 0;

        thunk->prefix_target = thunk->prefix_relay = 0x90;
        thunk->pushl_target  = thunk->pushl_relay  = 0x68;
        thunk->jmp_glue = 0xE9;

        thunk->target  = (DWORD)target;
        thunk->relay   = (DWORD)relay;
        thunk->glue    = (DWORD)glue - (DWORD)&thunk->type;

        thunk->type    = THUNKLET_TYPE_LS;
        thunk->owner   = pTask? pTask->hInstance : 0;

        thunk->next    = ThunkletAnchor;
        ThunkletAnchor = thunk;
    }

    return (FARPROC)thunk;
}

/***********************************************************************
 *     THUNK_AllocSLThunklet
 */
SEGPTR THUNK_AllocSLThunklet( FARPROC target, DWORD relay,
                              SEGPTR glue, HTASK16 owner )
{
    THUNKLET *thunk = THUNK_FindThunklet( (DWORD)target, relay, (DWORD)glue,
                                          THUNKLET_TYPE_SL );
    if (!thunk)
    {
        TDB *pTask = (TDB*)GlobalLock16( owner );

        if ( !(thunk = HeapAlloc( ThunkletHeap, 0, sizeof(THUNKLET) )) )
            return 0;

        thunk->prefix_target = thunk->prefix_relay = 0x66;
        thunk->pushl_target  = thunk->pushl_relay  = 0x68;
        thunk->jmp_glue = 0xEA;

        thunk->target  = (DWORD)target;
        thunk->relay   = (DWORD)relay;
        thunk->glue    = (DWORD)glue;

        thunk->type    = THUNKLET_TYPE_SL;
        thunk->owner   = pTask? pTask->hInstance : 0;

        thunk->next    = ThunkletAnchor;
        ThunkletAnchor = thunk;
    }

    return HEAP_GetSegptr( ThunkletHeap, 0, thunk );
}

/**********************************************************************
 *     IsLSThunklet
 */
BOOL16 WINAPI IsLSThunklet( THUNKLET *thunk )
{
    return    thunk->prefix_target == 0x90 && thunk->pushl_target == 0x68
           && thunk->prefix_relay  == 0x90 && thunk->pushl_relay  == 0x68
           && thunk->jmp_glue == 0xE9 && thunk->type == THUNKLET_TYPE_LS;
}

/**********************************************************************
 *     IsSLThunklet                        (KERNEL.612)
 */
BOOL16 WINAPI IsSLThunklet16( THUNKLET *thunk )
{
    return    thunk->prefix_target == 0x66 && thunk->pushl_target == 0x68
           && thunk->prefix_relay  == 0x66 && thunk->pushl_relay  == 0x68
           && thunk->jmp_glue == 0xEA && thunk->type == THUNKLET_TYPE_SL;
}



/***********************************************************************
 *     AllocLSThunkletSysthunk             (KERNEL.607)
 */
FARPROC WINAPI AllocLSThunkletSysthunk16( SEGPTR target, 
                                          FARPROC relay, DWORD dummy )
{
    return THUNK_AllocLSThunklet( (SEGPTR)relay, (DWORD)target, 
                                  ThunkletSysthunkGlueLS, GetCurrentTask() );
}

/***********************************************************************
 *     AllocSLThunkletSysthunk             (KERNEL.608)
 */
SEGPTR WINAPI AllocSLThunkletSysthunk16( FARPROC target, 
                                       SEGPTR relay, DWORD dummy )
{
    return THUNK_AllocSLThunklet( (FARPROC)relay, (DWORD)target, 
                                  ThunkletSysthunkGlueSL, GetCurrentTask() );
}


/***********************************************************************
 *     AllocLSThunkletCallbackEx           (KERNEL.567)
 */
FARPROC WINAPI AllocLSThunkletCallbackEx16( SEGPTR target, 
                                            DWORD relay, HTASK16 task )
{
    THUNKLET *thunk = (THUNKLET *)PTR_SEG_TO_LIN( target );
    if ( !thunk ) return NULL;

    if (   IsSLThunklet16( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueSL )
        return (FARPROC)thunk->target;

    return THUNK_AllocLSThunklet( target, relay, 
                                  ThunkletCallbackGlueLS, task );
}

/***********************************************************************
 *     AllocSLThunkletCallbackEx           (KERNEL.568)
 */
SEGPTR WINAPI AllocSLThunkletCallbackEx16( FARPROC target, 
                                         DWORD relay, HTASK16 task )
{
    THUNKLET *thunk = (THUNKLET *)target;
    if ( !thunk ) return 0;

    if (   IsLSThunklet( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueLS - (DWORD)&thunk->type )
        return (SEGPTR)thunk->target;

    return THUNK_AllocSLThunklet( target, relay, 
                                  ThunkletCallbackGlueSL, task );
}

/***********************************************************************
 *     AllocLSThunkletCallback             (KERNEL.561) (KERNEL.606)
 */
FARPROC WINAPI AllocLSThunkletCallback16( SEGPTR target, DWORD relay )
{
    return AllocLSThunkletCallbackEx16( target, relay, GetCurrentTask() );
}

/***********************************************************************
 *     AllocSLThunkletCallback             (KERNEL.562) (KERNEL.605)
 */
SEGPTR WINAPI AllocSLThunkletCallback16( FARPROC target, DWORD relay )
{
    return AllocSLThunkletCallbackEx16( target, relay, GetCurrentTask() );
}

/***********************************************************************
 *     FindLSThunkletCallback              (KERNEL.563) (KERNEL.609)
 */
FARPROC WINAPI FindLSThunkletCallback( SEGPTR target, DWORD relay )
{
    THUNKLET *thunk = (THUNKLET *)PTR_SEG_TO_LIN( target );
    if (   thunk && IsSLThunklet16( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueSL )
        return (FARPROC)thunk->target;

    thunk = THUNK_FindThunklet( (DWORD)target, relay, 
                                (DWORD)ThunkletCallbackGlueLS, 
                                THUNKLET_TYPE_LS );
    return (FARPROC)thunk;
}

/***********************************************************************
 *     FindSLThunkletCallback              (KERNEL.564) (KERNEL.610)
 */
SEGPTR WINAPI FindSLThunkletCallback( FARPROC target, DWORD relay )
{
    THUNKLET *thunk = (THUNKLET *)target;
    if (   thunk && IsLSThunklet( thunk ) && thunk->relay == relay 
        && thunk->glue == (DWORD)ThunkletCallbackGlueLS - (DWORD)&thunk->type )
        return (SEGPTR)thunk->target;

    thunk = THUNK_FindThunklet( (DWORD)target, relay, 
                                (DWORD)ThunkletCallbackGlueSL, 
                                THUNKLET_TYPE_SL );
    return HEAP_GetSegptr( ThunkletHeap, 0, thunk );
}


/***********************************************************************
 *     FreeThunklet16            (KERNEL.611)
 */
BOOL16 WINAPI FreeThunklet16( DWORD unused1, DWORD unused2 )
{
    return FALSE;
}


/***********************************************************************
 * Callback Client API
 */

#define N_CBC_FIXED    20
#define N_CBC_VARIABLE 10
#define N_CBC_TOTAL    (N_CBC_FIXED + N_CBC_VARIABLE)

static SEGPTR CBClientRelay16[ N_CBC_TOTAL ];
static FARPROC *CBClientRelay32[ N_CBC_TOTAL ];

/***********************************************************************
 *     RegisterCBClient                    (KERNEL.619)
 */
INT16 WINAPI RegisterCBClient16( INT16 wCBCId, 
                                 SEGPTR relay16, FARPROC *relay32 )
{
    /* Search for free Callback ID */
    if ( wCBCId == -1 )
        for ( wCBCId = N_CBC_FIXED; wCBCId < N_CBC_TOTAL; wCBCId++ )
            if ( !CBClientRelay16[ wCBCId ] )
                break;

    /* Register Callback ID */
    if ( wCBCId > 0 && wCBCId < N_CBC_TOTAL )
    {
        CBClientRelay16[ wCBCId ] = relay16;
        CBClientRelay32[ wCBCId ] = relay32;
    }
    else
        wCBCId = 0;

    return wCBCId;
}

/***********************************************************************
 *     UnRegisterCBClient                  (KERNEL.622)
 */
INT16 WINAPI UnRegisterCBClient16( INT16 wCBCId, 
                                   SEGPTR relay16, FARPROC *relay32 )
{
    if (    wCBCId >= N_CBC_FIXED && wCBCId < N_CBC_TOTAL 
         && CBClientRelay16[ wCBCId ] == relay16 
         && CBClientRelay32[ wCBCId ] == relay32 )
    {
        CBClientRelay16[ wCBCId ] = 0;
        CBClientRelay32[ wCBCId ] = 0;
    }
    else
        wCBCId = 0;

    return wCBCId;
}


/***********************************************************************
 *     InitCBClient                        (KERNEL.623)
 */
void WINAPI InitCBClient16( FARPROC glueLS )
{
    HMODULE16 kernel = GetModuleHandle16( "KERNEL" );
    SEGPTR glueSL = (SEGPTR)WIN32_GetProcAddress16( kernel, (LPCSTR)604 );

    SetThunkletCallbackGlue16( glueLS, glueSL );
}

/***********************************************************************
 *     CBClientGlueSL                      (KERNEL.604)
 */
void WINAPI CBClientGlueSL( CONTEXT86 *context )
{
    /* Create stack frame */
    SEGPTR stackSeg = stack16_push( 12 );
    LPWORD stackLin = PTR_SEG_TO_LIN( stackSeg );
    SEGPTR glue, *glueTab;
    
    stackLin[3] = BP_reg( context );
    stackLin[2] = SI_reg( context );
    stackLin[1] = DI_reg( context );
    stackLin[0] = DS_reg( context );

    EBP_reg( context ) = OFFSETOF( stackSeg ) + 6;
    ESP_reg( context ) = OFFSETOF( stackSeg ) - 4;
    GS_reg( context ) = 0;

    /* Jump to 16-bit relay code */
    glueTab = PTR_SEG_TO_LIN( CBClientRelay16[ stackLin[5] ] );
    glue = glueTab[ stackLin[4] ];
    CS_reg ( context ) = SELECTOROF( glue );
    EIP_reg( context ) = OFFSETOF  ( glue );
}

/***********************************************************************
 *     CBClientThunkSL                      (KERNEL.620)
 */
extern DWORD CALL32_CBClient( FARPROC proc, LPWORD args, DWORD *esi );
void WINAPI CBClientThunkSL( CONTEXT86 *context )
{
    /* Call 32-bit relay code */

    LPWORD args = PTR_SEG_OFF_TO_LIN( SS_reg( context ), BP_reg( context ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];

    EAX_reg(context) = CALL32_CBClient( proc, args, &ESI_reg( context ) );
}

/***********************************************************************
 *     CBClientThunkSLEx                    (KERNEL.621)
 */
extern DWORD CALL32_CBClientEx( FARPROC proc, LPWORD args, DWORD *esi, INT *nArgs );
void WINAPI CBClientThunkSLEx( CONTEXT86 *context )
{
    /* Call 32-bit relay code */

    LPWORD args = PTR_SEG_OFF_TO_LIN( SS_reg( context ), BP_reg( context ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];
    INT nArgs;
    LPWORD stackLin;

    EAX_reg(context) = CALL32_CBClientEx( proc, args, &ESI_reg( context ), &nArgs );

    /* Restore registers saved by CBClientGlueSL */
    stackLin = (LPWORD)((LPBYTE)CURRENT_STACK16 + sizeof(STACK16FRAME) - 4);
    BP_reg( context ) = stackLin[3];
    SI_reg( context ) = stackLin[2];
    DI_reg( context ) = stackLin[1];
    DS_reg( context ) = stackLin[0];
    ESP_reg( context ) += 16+nArgs;

    /* Return to caller of CBClient thunklet */
    CS_reg ( context ) = stackLin[9];
    EIP_reg( context ) = stackLin[8];
}


/***********************************************************************
 *           Get16DLLAddress       (KERNEL32)
 *
 * This function is used by a Win32s DLL if it wants to call a Win16 function.
 * A 16:16 segmented pointer to the function is returned.
 * Written without any docu.
 */
SEGPTR WINAPI Get16DLLAddress(HMODULE handle, LPSTR func_name) {
	HANDLE ThunkHeap = HeapCreate(HEAP_WINE_SEGPTR | HEAP_WINE_CODESEG, 0, 64);
        LPBYTE x;
	LPVOID tmpheap = HeapAlloc(ThunkHeap, 0, 32);
	SEGPTR thunk = HEAP_GetSegptr(ThunkHeap, 0, tmpheap);
	DWORD proc_16;

        if (!handle) handle=GetModuleHandle16("WIN32S16");
        proc_16 = (DWORD)WIN32_GetProcAddress16(handle, func_name);

        x=PTR_SEG_TO_LIN(thunk);
        *x++=0xba; *(DWORD*)x=proc_16;x+=4;             /* movl proc_16, $edx */
        *x++=0xea; *(DWORD*)x=(DWORD)GetProcAddress(GetModuleHandleA("KERNEL32"),"QT_Thunk");x+=4;     /* jmpl QT_Thunk */
	*(WORD*)x=__get_cs();
        return thunk;
}


/***********************************************************************
 *		GetWin16DOSEnv			(KERNEL32.34)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI GetWin16DOSEnv()
{
	FIXME("stub, returning 0\n");
	return 0;
}

/**********************************************************************
 *           GetPK16SysVar    (KERNEL32.92)
 */
LPVOID WINAPI GetPK16SysVar(void)
{
    static BYTE PK16SysVar[128];

    FIXME("()\n");
    return PK16SysVar;
}

/**********************************************************************
 *           CommonUnimpStub    (KERNEL32.17)
 */
void WINAPI CommonUnimpStub( CONTEXT86 *context )
{
    if (EAX_reg(context))
        MESSAGE( "*** Unimplemented Win32 API: %s\n", (LPSTR)EAX_reg(context) );

    switch ((ECX_reg(context) >> 4) & 0x0f)
    {
    case 15:  EAX_reg(context) = -1;   break;
    case 14:  EAX_reg(context) = 0x78; break;
    case 13:  EAX_reg(context) = 0x32; break;
    case 1:   EAX_reg(context) = 1;    break;
    default:  EAX_reg(context) = 0;    break;
    }

    ESP_reg(context) += (ECX_reg(context) & 0x0f) * 4;
}

/**********************************************************************
 *           HouseCleanLogicallyDeadHandles    (KERNEL32.33)
 */
void WINAPI HouseCleanLogicallyDeadHandles(void)
{
    /* Whatever this is supposed to do, our handles probably
       don't need it :-) */
}

/**********************************************************************
 *		_KERNEL32_100
 */
BOOL WINAPI _KERNEL32_100(HANDLE threadid,DWORD exitcode,DWORD x)
{
	FIXME("(%d,%ld,0x%08lx): stub\n",threadid,exitcode,x);
	return TRUE;
}

/**********************************************************************
 *		_KERNEL32_99
 */
DWORD WINAPI _KERNEL32_99(DWORD x)
{
	FIXME("(0x%08lx): stub\n",x);
	return 1;
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

    lpbuf[0] = LOWORD(EIP_reg(context));
    lpbuf[1] = CS_reg(context);
    /* Windows pushes 4 more words before saving sp */
    lpbuf[2] = LOWORD(ESP_reg(context)) - 4 * sizeof(WORD);
    lpbuf[3] = LOWORD(EBP_reg(context));
    lpbuf[4] = LOWORD(ESI_reg(context));
    lpbuf[5] = LOWORD(EDI_reg(context));
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

    EIP_reg(context) = lpbuf[0];
    CS_reg(context)  = lpbuf[1];
    ESP_reg(context) = lpbuf[2] + 4 * sizeof(WORD) - sizeof(WORD) /*extra arg*/;
    EBP_reg(context) = lpbuf[3];
    ESI_reg(context) = lpbuf[4];
    EDI_reg(context) = lpbuf[5];
    DS_reg(context)  = lpbuf[6];

    if (lpbuf[8] != SS_reg(context))
        ERR("Switching stack segment with Throw() not supported; expect crash now\n" );
}
