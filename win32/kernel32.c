/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1997-1998 Marcus Meissner
 * Copyright 1998      Ulrich Weigand
 */

#include <stdio.h>
#include "windows.h"
#include "callback.h"
#include "resource.h"
#include "task.h"
#include "user.h"
#include "heap.h"
#include "module.h"
#include "process.h"
#include "stackframe.h"
#include "heap.h"
#include "selectors.h"
#include "task.h"
#include "win.h"
#include "debug.h"

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
	*x++	= 0x68; *(DWORD*)x = (DWORD)GetProcAddress32(GetModuleHandle32A("KERNEL32"),"FT_Prolog");
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
	*x++	= 0xB8; *(DWORD*)x = (DWORD)GetProcAddress32(GetModuleHandle32A("KERNEL32"),"QT_Thunk");
	x+=4; 	/* mov eax , QT_Thunk */
	*x++	= 0xFF; *x++ = 0xE0;	/* jmp eax */
	/* should fill the rest of the 32 bytes with 0xCC */
}

struct thunkstruct
{
	char	magic[4];
	DWORD	length;
	DWORD	ptr;
	DWORD	x0C;

	DWORD	x10;
	DWORD	x14;
	DWORD	x18;
	DWORD	x1C;
	DWORD	x20;
};

/***********************************************************************
 *		ThunkConnect32		(KERNEL32)
 * Connects a 32bit and a 16bit thunkbuffer.
 */
UINT32 WINAPI ThunkConnect32( 
	struct thunkstruct *ths,	/* [in/out] thunkbuffer */
	LPCSTR thunkfun16,		/* [in] win16 thunkfunction */
	LPCSTR module16,		/* [in] name of win16 dll */
	LPCSTR module32,		/* [in] name of win32 dll */
	HMODULE32 hmod32,		/* [in] hmodule of win32 dll (used?) */
	DWORD dllinitarg1		/* [in] initialisation argument */
) {
	HINSTANCE16	hmm;
	SEGPTR		thkbuf;
	struct	thunkstruct	*ths16;

	TRACE(thunk,"(<struct>,%s,%s,%s,%x,%lx)\n",
		thunkfun16,module32,module16,hmod32,dllinitarg1
	);
	TRACE(thunk,"	magic = %c%c%c%c\n",
		ths->magic[0],
		ths->magic[1],
		ths->magic[2],
		ths->magic[3]
	);
	TRACE(thunk,"	length = %lx\n",ths->length);
	if (lstrncmp32A(ths->magic,"SL01",4)&&lstrncmp32A(ths->magic,"LS01",4))
		return 0;
	hmm=LoadModule16(module16,NULL);
	if (hmm<=32)
		return 0;
	thkbuf=(SEGPTR)WIN32_GetProcAddress16(hmm,thunkfun16);
	if (!thkbuf)
		return 0;
	ths16=(struct thunkstruct*)PTR_SEG_TO_LIN(thkbuf);
	if (lstrncmp32A(ths16->magic,ths->magic,4))
		return 0;

	if (!lstrncmp32A(ths->magic,"SL01",4))  {
		if (ths16->length != ths->length)
			return 0;
		ths->x0C = (DWORD)ths16;

		TRACE(thunk,"	ths16 magic is 0x%08lx\n",*(DWORD*)ths16->magic);
		if (*((DWORD*)ths16->magic) != 0x0000304C)
			return 0;
		if (!*(WORD*)(((LPBYTE)ths16)+0x12))
			return 0;
		
	}
	if (!lstrncmp32A(ths->magic,"LS01",4))  {
		if (ths16->length != ths->length)
			return 0;
		ths->ptr = (DWORD)PTR_SEG_TO_LIN(ths16->ptr);
		/* code offset for QT_Thunk is at 0x1C...  */
		_write_qtthunk (((LPBYTE)ths) + ths->x1C,(DWORD *)ths->ptr);
		/* code offset for FT_Prolog is at 0x20...  */
		_write_ftprolog(((LPBYTE)ths) + ths->x20,(DWORD *)ths->ptr);
		return 1;
	}
	return TRUE;
}


/**********************************************************************
 * 		QT_Thunk			(KERNEL32)
 *
 * The target address is in EDX.
 * The 16 bit arguments start at ESP+4.
 * The number of 16bit argumentbytes is EBP-ESP-0x44 (68 Byte thunksetup).
 * [ok]
 */
VOID WINAPI QT_Thunk(CONTEXT *context)
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

#define POP_DWORD(ctx)		(*((DWORD *)ESP_reg(ctx))++)
#define PUSH_DWORD(ctx, val) 	*--((DWORD *)ESP_reg(ctx)) = (val)

VOID WINAPI FT_Prolog(CONTEXT *context)
{
    /* Pop return address to thunk code */
    EIP_reg(context) = POP_DWORD(context);

    /* Build stack frame */
    PUSH_DWORD(context, EBP_reg(context));
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
    PUSH_DWORD(context, EIP_reg(context));
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

VOID WINAPI FT_Thunk(CONTEXT *context)
{
    DWORD mapESPrelative = *(DWORD *)(EBP_reg(context) - 20);
    DWORD callTarget     = *(DWORD *)(EBP_reg(context) - 52);

    CONTEXT context16;
    DWORD i, argsize;
    LPBYTE newstack;
    THDB *thdb = THREAD_Current();

    memcpy(&context16,context,sizeof(context16));

    CS_reg(&context16)  = HIWORD(callTarget);
    IP_reg(&context16)  = LOWORD(callTarget);
    EBP_reg(&context16) = OFFSETOF( thdb->cur_stack )
                           + (WORD)&((STACK16FRAME*)0)->bp;

    argsize  = EBP_reg(context)-ESP_reg(context)-0x44;
    newstack = ((LPBYTE)THREAD_STACK16(thdb))-argsize;

    memcpy( newstack, (LPBYTE)ESP_reg(context)+4, argsize );

    for (i = 0; i < 32; i++)	/* NOTE: What about > 32 arguments? */
	if (mapESPrelative & (1 << i))
	{
	    SEGPTR *arg = (SEGPTR *)(newstack + 2*i);
	    *arg = PTR_SEG_OFF_TO_SEGPTR(SELECTOROF(thdb->cur_stack), 
					 *(LPBYTE *)arg - newstack);
	}

    EAX_reg(context) = Callbacks->CallRegisterShortProc( &context16, argsize );
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

VOID WINAPI FT_Exit(CONTEXT *context, int nPopArgs)
{
    /* Return value is in EBX */
    EAX_reg(context) = EBX_reg(context);

    /* Restore EBX, ESI, and EDI registers */
    EBX_reg(context) = *(DWORD *)(EBP_reg(context) -  4);
    ESI_reg(context) = *(DWORD *)(EBP_reg(context) -  8);
    EDI_reg(context) = *(DWORD *)(EBP_reg(context) - 12);

    /* Clean up stack frame */
    ESP_reg(context) = EBP_reg(context);
    EBP_reg(context) = POP_DWORD(context);

    /* Pop return address to CALLER of thunk code */
    EIP_reg(context) = POP_DWORD(context);
    /* Remove arguments */
    ESP_reg(context) += nPopArgs;
    /* Push return address back onto stack */
    PUSH_DWORD(context, EIP_reg(context));
}

VOID WINAPI FT_Exit0 (CONTEXT *context) { FT_Exit(context,  0); }
VOID WINAPI FT_Exit4 (CONTEXT *context) { FT_Exit(context,  4); }
VOID WINAPI FT_Exit8 (CONTEXT *context) { FT_Exit(context,  8); }
VOID WINAPI FT_Exit12(CONTEXT *context) { FT_Exit(context, 12); }
VOID WINAPI FT_Exit16(CONTEXT *context) { FT_Exit(context, 16); }
VOID WINAPI FT_Exit20(CONTEXT *context) { FT_Exit(context, 20); }
VOID WINAPI FT_Exit24(CONTEXT *context) { FT_Exit(context, 24); }
VOID WINAPI FT_Exit28(CONTEXT *context) { FT_Exit(context, 28); }
VOID WINAPI FT_Exit32(CONTEXT *context) { FT_Exit(context, 32); }
VOID WINAPI FT_Exit36(CONTEXT *context) { FT_Exit(context, 36); }
VOID WINAPI FT_Exit40(CONTEXT *context) { FT_Exit(context, 40); }
VOID WINAPI FT_Exit44(CONTEXT *context) { FT_Exit(context, 44); }
VOID WINAPI FT_Exit48(CONTEXT *context) { FT_Exit(context, 48); }
VOID WINAPI FT_Exit52(CONTEXT *context) { FT_Exit(context, 52); }
VOID WINAPI FT_Exit56(CONTEXT *context) { FT_Exit(context, 56); }


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
BOOL32 WINAPI WOWCallback16Ex(
	FARPROC16 vpfn16,	/* [in] win16 function to call */
	DWORD dwFlags,		/* [in] flags */
	DWORD cbArgs,		/* [in] nr of arguments */
	LPVOID pArgs,		/* [in] pointer to arguments (LPDWORD) */
	LPDWORD pdwRetCode	/* [out] return value of win16 function */
) {
	return Callbacks->CallWOWCallback16Ex(vpfn16,dwFlags,cbArgs,pArgs,pdwRetCode);
}

/***********************************************************************
 *           _KERNEL32_52    (KERNEL32.52)
 * Returns a pointer to ThkBuf in the 16bit library SYSTHUNK.DLL.
 * [ok probably]
 */
LPVOID WINAPI _KERNEL32_52()
{
	HMODULE32	hmod = LoadLibrary16("systhunk.dll");

	TRACE(thunk, "systhunk.dll module %d\n", hmod);
	
	if (hmod<=32)
		return 0;
	return PTR_SEG_TO_LIN(WIN32_GetProcAddress16(hmod,"ThkBuf"));
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
	HINSTANCE16	hmod;
	LPDWORD		addr;
	SEGPTR		segaddr;

	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		WARN(thunk,"failed to load 16bit DLL %s, error %d\n",
			     dll16,hmod);
		return 0;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
	        WARN(thunk,"no %s exported from %s!\n",thkbuf,dll16);
		return 0;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		WARN(thunk,"thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return 0;
	}
	if (!addr[1])
		return 0;
	*(DWORD*)thunk = addr[1];

	TRACE(thunk, "loaded module %d, func %s (%d) @ %p (%p), returning %p\n",
		     hmod, HIWORD(thkbuf)==0 ? "<ordinal>" : thkbuf, (int)thkbuf, 
		     (void*)segaddr, addr, (void*)addr[1]);

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
VOID WINAPI Common32ThkLS(CONTEXT *context)
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

    memcpy( ((LPBYTE)THREAD_STACK16(thdb))-argsize,
            (LPBYTE)ESP_reg(context)+4, argsize );

    EAX_reg(context) = Callbacks->CallRegisterLongProc(&context16, argsize + 32);

    /* Clean up caller's stack frame */

    EIP_reg(context) = POP_DWORD(context);
    ESP_reg(context) += argsize;
    PUSH_DWORD(context, EIP_reg(context));
}

/***********************************************************************
 * 		_KERNEL32_40 	(KERNEL32.40)
 * YET Another 32->16 thunk, the difference to the others is still mysterious
 * Target address is in EDX.
 *
 * [crashes]
 */
VOID WINAPI _KERNEL32_40(CONTEXT *context)
{
	CONTEXT	context16;
	LPBYTE	curstack;
	DWORD	ret,stacksize;
	THDB *thdb = THREAD_Current();

	__RESTORE_ES;
	TRACE(thunk,"(EDX=0x%08lx)\n", EDX_reg(context) );
	stacksize = EBP_reg(context)-ESP_reg(context);
	TRACE(thunk,"	stacksize = %ld\n",stacksize);
	TRACE(thunk,"on top of stack: 0x%04x\n",*(WORD*)ESP_reg(context));

	memcpy(&context16,context,sizeof(context16));

	CS_reg(&context16)	 = HIWORD(EDX_reg(context));
	IP_reg(&context16)	 = LOWORD(EDX_reg(context));

	curstack = PTR_SEG_TO_LIN(STACK16_PUSH( thdb, stacksize ));
	memcpy(curstack-stacksize,(LPBYTE)ESP_reg(context),stacksize);
	ret = Callbacks->CallRegisterShortProc(&context16,0);
	STACK16_POP( thdb, stacksize );

	TRACE(thunk,". returned %08lx\n",ret);
	EAX_reg(context) 	 = ret;
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
	HMODULE32	hkrnl32 = GetModuleHandle32A("KERNEL32");
	HMODULE16	hmod;
	LPDWORD		addr,addr2;
	DWORD		segaddr;

	/* FIXME: add checks for valid code ... */
	/* write pointers to kernel32.89 and kernel32.90 (+ordinal base of 1) */
	*(DWORD*)(thunk+0x35) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)90);
	*(DWORD*)(thunk+0x6D) = (DWORD)GetProcAddress32(hkrnl32,(LPSTR)89);

	
	hmod = LoadLibrary16(dll16);
	if (hmod<32) {
		ERR(thunk,"failed to load 16bit DLL %s, error %d\n",
			     dll16,hmod);
		return NULL;
	}
	segaddr = (DWORD)WIN32_GetProcAddress16(hmod,(LPSTR)thkbuf);
	if (!segaddr) {
		ERR(thunk,"no %s exported from %s!\n",thkbuf,dll16);
		return NULL;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
		ERR(thunk,"thkbuf length mismatch? %ld vs %ld\n",len,addr[0]);
		return NULL;
	}
	addr2 = PTR_SEG_TO_LIN(addr[1]);
	if (HIWORD(addr2))
		*(DWORD*)thunk = (DWORD)addr2;

	TRACE(thunk, "loaded module %d, func %s(%d) @ %p (%p), returning %p\n",
		      hmod, HIWORD(thkbuf)==0 ? "<ordinal>" : thkbuf, (int)thkbuf, 
		     (void*)segaddr, addr, addr2);

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
VOID WINAPI FT_PrologPrime(CONTEXT *context)
{
    DWORD  targetTableOffset = POP_DWORD(context);
    LPBYTE relayCode = (LPBYTE)POP_DWORD(context);
    DWORD *targetTable = *(DWORD **)(relayCode+targetTableOffset);
    DWORD  targetNr = LOBYTE(ECX_reg(context));

    _write_ftprolog(relayCode, targetTable);

    /* We should actually call the relay code now, */
    /* but we skip it and go directly to FT_Prolog */
    EDX_reg(context) = targetTable[targetNr];
    FT_Prolog(context);
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
VOID WINAPI QT_ThunkPrime(CONTEXT *context)
{
    DWORD  targetTableOffset = EDX_reg(context);
    LPBYTE relayCode = (LPBYTE)EAX_reg(context);
    DWORD *targetTable = *(DWORD **)(relayCode+targetTableOffset);
    DWORD  targetNr = *(DWORD *)(EBP_reg(context) - 4);

    _write_qtthunk(relayCode, targetTable);

    /* We should actually call the relay code now, */
    /* but we skip it and go directly to QT_Thunk */
    EDX_reg(context) = targetTable[targetNr];
    QT_Thunk(context);
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
	HMODULE16	hmod;
	SEGPTR		segaddr;

	hmod = LoadLibrary16(dll16);
	if (hmod < 32) {
		ERR(thunk,"couldn't load %s, error %d\n",dll16,hmod);
		return;
	}
	segaddr = (SEGPTR)WIN32_GetProcAddress16(hmod,thkbuf);
	if (!segaddr) {
		ERR(thunk,"haven't found %s in %s!\n",thkbuf,dll16);
		return;
	}
	addr = (LPDWORD)PTR_SEG_TO_LIN(segaddr);
	if (addr[0] != len) {
	        ERR(thunk,"length of thkbuf differs from expected length! "
			     "(%ld vs %ld)\n",addr[0],len);
		return;
	}
	*(DWORD*)PTR_SEG_TO_LIN(addr[1]) = (DWORD)thunk;

	TRACE(thunk, "loaded module %d, func %s(%d) @ %p (%p)\n",
		     hmod, HIWORD(thkbuf)==0 ? "<ordinal>" : thkbuf, 
		     (int)thkbuf, (void*)segaddr, addr);
}

/**********************************************************************
 *           SSOnBigStack	KERNEL32.87
 * Check if thunking is initialized (ss selector set up etc.)
 * We do that differently, so just return TRUE.
 * [ok]
 * RETURNS
 *	TRUE for success.
 */
BOOL32 WINAPI SSOnBigStack()
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
	FARPROC32 fun,	/* [in] function to call */
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
	fprintf(stderr,"_KERNEL32_88: unsupported nr of arguments, %ld\n",nr);
	ret = 0;
	break;

    }
    TRACE(thunk," returning %ld ...\n",ret);
    return ret;
}

/**********************************************************************
 * 		KERNEL_619		(KERNEL)
 * Seems to store y and z depending on x in some internal lists...
 */
WORD WINAPI _KERNEL_619(WORD x,DWORD y,DWORD z)
{
    fprintf(stderr,"KERNEL_619(0x%04x,0x%08lx,0x%08lx)\n",x,y,z);
    return x;
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

	*(PDB32**)(thunk+18) = PROCESS_Current();

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
	fprintf(stderr,"FreeSLCallback(0x%08lx)\n",x);
}


/**********************************************************************
 * 		KERNEL_471		(KERNEL.471)
 * RETURNS
 * 	Seems to return the uncrypted current process pointer. [Not 100% sure].
 */
LPVOID WINAPI
_KERNEL_471() {
	return PROCESS_Current();
}

/**********************************************************************
 * 		KERNEL_472		(KERNEL.472)
 * something like GetCurrenthInstance.
 * RETURNS
 *	the hInstance
 */
VOID WINAPI
_KERNEL_472(CONTEXT *context) {
	fprintf(stderr,"_KERNEL_472(0x%08lx),stub\n",EAX_reg(context));
	if (!EAX_reg(context)) {
		TDB *pTask = (TDB*)GlobalLock16(GetCurrentTask());
		AX_reg(context)=pTask->hInstance;
		return;
	}
	if (!HIWORD(EAX_reg(context)))
		return; /* returns the passed value */
	/* hmm ... fixme */
}

/**********************************************************************
 * 		KERNEL_431		(KERNEL.431)
 *		IsPeFormat		(W32SYS.2)
 * Checks the passed filename if it is a PE format executeable
 * RETURNS
 *  TRUE, if it is.
 *  FALSE if not.
 */
BOOL16 WINAPI IsPeFormat(
	LPSTR	fn,	/* [in] filename to executeable */
	HFILE16 hf16	/* [in] open file, if filename is NULL */
) {
	IMAGE_DOS_HEADER	mzh;
	HFILE32			hf=hf16;
	OFSTRUCT		ofs;
	DWORD			xmagic;

	if (fn) {
		hf = OpenFile32(fn,&ofs,OF_READ);
		if (hf==HFILE_ERROR32)
			return FALSE;
	}
	_llseek32(hf,0,SEEK_SET);
	if (sizeof(mzh)!=_lread32(hf,&mzh,sizeof(mzh))) {
		_lclose32(hf);
		return FALSE;
	}
	if (mzh.e_magic!=IMAGE_DOS_SIGNATURE) {
		fprintf(stderr,"file has not got dos signature!\n");
		_lclose32(hf);
		return FALSE;
	}
	_llseek32(hf,mzh.e_lfanew,SEEK_SET);
	if (sizeof(DWORD)!=_lread32(hf,&xmagic,sizeof(DWORD))) {
		_lclose32(hf);
		return FALSE;
	}
	_lclose32(hf);
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
HANDLE32 WINAPI WOWHandle32(
	WORD handle,		/* [in] win16 handle */
	WOW_HANDLE_TYPE type	/* [in] handle type */
) {
	fprintf(stderr,"WOWHandle32(0x%04x,%d)\n",handle,type);
	return (HANDLE32)handle;
}

/***********************************************************************
 *           FUNC004				(KERNEL.631)
 * A 16->32 thunk setup function.
 * Gets called from a thunkbuffer (value of EAX). It overwrites the start
 * with a jmp to a kernel32 function. The kernel32 function gets passed EDX.
 * (and possibly CX).
 */
void WINAPI FUNC004(
	CONTEXT *context	/* [in/out] register context from 1632-relay */
) {

	FIXME(reg,",STUB (edx is 0x%08lx, eax is 0x%08lx,edx[0x10] is 0x%08lx)!\n",
		EDX_reg(context),
		EAX_reg(context),
		((DWORD*)PTR_SEG_TO_LIN(EDX_reg(context)))[0x10/4]
	);

#if 0
{
	LPBYTE	x,target = (LPBYTE)PTR_SEG_TO_LIN(EAX_reg(context));
	WORD	ds,cs;

	GET_DS(ds);
	GET_CS(cs);
/* Won't work anyway since we don't know the function called in KERNEL32 -Marcus*/
	x = target;
	*x++= 0xb8; *(WORD*)x= ds;x+=2;		/* mov ax,KERNEL32_DS */
	*x++= 0x8e; *x++ = 0xc0; 		/* mov es,ax */
	*x++= 0x66; *x++ = 0xba; *(DWORD*)x=EDX_reg(context);x+=4;
						/* mov edx, $EDX */
	*x++= 0x66; *x++ = 0xea;	/* jmp KERNEL32_CS:kernel32fun */
		*(DWORD*)x=0;x+=4;/* FIXME: _what_ function does it call? */
		*(WORD*)x=cs;x+=2;
					

	IP_reg(context) = LOWORD(EAX_reg(context));
	CS_reg(context) = HIWORD(EAX_reg(context));
}
#endif
	return;
}

