/*
 * KERNEL32 thunks and other undocumented stuff
 *
 * Copyright 1996, 1997 Alexandre Julliard
 * Copyright 1997, 1998 Marcus Meissner
 * Copyright 1998       Ulrich Weigand
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "rtlsupportapi.h"
#include "wownt16.h"
#include "wownt32.h"
#include "wine/winbase16.h"

#include "wine/debug.h"
#include "kernel16_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);

struct ThunkDataCommon
{
    char                   magic[4];         /* 00 */
    DWORD                  checksum;         /* 04 */
};

struct ThunkDataLS16
{
    struct ThunkDataCommon common;           /* 00 */
    SEGPTR                 targetTable;      /* 08 */
    DWORD                  firstTime;        /* 0C */
};

struct ThunkDataLS32
{
    struct ThunkDataCommon common;           /* 00 */
    DWORD *                targetTable;      /* 08 */
    char                   lateBinding[4];   /* 0C */
    DWORD                  flags;            /* 10 */
    DWORD                  reserved1;        /* 14 */
    DWORD                  reserved2;        /* 18 */
    DWORD                  offsetQTThunk;    /* 1C */
    DWORD                  offsetFTProlog;   /* 20 */
};

struct ThunkDataSL16
{
    struct ThunkDataCommon common;            /* 00 */
    DWORD                  flags1;            /* 08 */
    DWORD                  reserved1;         /* 0C */
    struct ThunkDataSL *   fpData;            /* 10 */
    SEGPTR                 spData;            /* 14 */
    DWORD                  reserved2;         /* 18 */
    char                   lateBinding[4];    /* 1C */
    DWORD                  flags2;            /* 20 */
    DWORD                  reserved3;         /* 20 */
    SEGPTR                 apiDatabase;       /* 28 */
};

struct ThunkDataSL32
{
    struct ThunkDataCommon common;            /* 00 */
    DWORD                  reserved1;         /* 08 */
    struct ThunkDataSL *   data;              /* 0C */
    char                   lateBinding[4];    /* 10 */
    DWORD                  flags;             /* 14 */
    DWORD                  reserved2;         /* 18 */
    DWORD                  reserved3;         /* 1C */
    DWORD                  offsetTargetTable; /* 20 */
};

struct ThunkDataSL
{
#if 0
    This structure differs from the Win95 original,
    but this should not matter since it is strictly internal to
    the thunk handling routines in KRNL386 / KERNEL32.

    For reference, here is the Win95 layout:

    struct ThunkDataCommon common;            /* 00 */
    DWORD                  flags1;            /* 08 */
    SEGPTR                 apiDatabase;       /* 0C */
    WORD                   exePtr;            /* 10 */
    WORD                   segMBA;            /* 12 */
    DWORD                  lenMBATotal;       /* 14 */
    DWORD                  lenMBAUsed;        /* 18 */
    DWORD                  flags2;            /* 1C */
    char                   pszDll16[256];     /* 20 */
    char                   pszDll32[256];     /*120 */

    We do it differently since all our thunk handling is done
    by 32-bit code. Therefore we do not need to provide
    easy access to this data, especially the process target
    table database, for 16-bit code.
#endif

    struct ThunkDataCommon common;
    DWORD                  flags1;
    struct SLApiDB *       apiDB;
    struct SLTargetDB *    targetDB;
    DWORD                  flags2;
    char                   pszDll16[256];
    char                   pszDll32[256];
};

struct SLTargetDB
{
     struct SLTargetDB *   next;
     DWORD                 process;
     DWORD *               targetTable;
};

struct SLApiDB
{
    DWORD                  nrArgBytes;
    DWORD                  errorReturnValue;
};

WORD cbclient_selector = 0;
WORD cbclientex_selector = 0;

extern int call_entry_point( void *func, int nb_args, const DWORD *args );
extern void __wine_call_from_16_thunk(void);
extern void WINAPI FT_Prolog(void);
extern void WINAPI FT_PrologPrime(void);
extern void WINAPI QT_Thunk(void);
extern void WINAPI QT_ThunkPrime(void);

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
__ASM_STDCALL_FUNC( LogApiThkLSF, 4, "ret $4" )

/***********************************************************************
 *           LogApiThkSL    (KERNEL32.44)
 *
 * NOTE: needs to preserve all registers!
 */
__ASM_STDCALL_FUNC( LogApiThkSL, 4, "ret $4" )

/***********************************************************************
 *           LogCBThkSL    (KERNEL32.47)
 *
 * NOTE: needs to preserve all registers!
 */
__ASM_STDCALL_FUNC( LogCBThkSL, 4, "ret $4" )

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
	*x++	= 0x68; *(void **)x = FT_Prolog;
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
	*x++	= 0xB8; *(void **)x = QT_Thunk;
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
    HMODULE16 hmod;
    int ordinal;
    static BOOL done;

    if (!done)
    {
        LoadLibrary16( "gdi.exe" );
        LoadLibrary16( "user.exe" );
        done = TRUE;
    }

    if ((hmod = LoadLibrary16(module)) <= 32)
    {
        ERR("(%s, %s, %s): Unable to load '%s', error %d\n",
                   module, func, module32, module, hmod);
        return 0;
    }

    if (   !(ordinal = NE_GetOrdinal(hmod, func))
        || !(TD16 = MapSL((SEGPTR)NE_GetEntryPointEx(hmod, ordinal, FALSE))))
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
LPVOID WINAPI GetThunkStuff(LPCSTR module, LPCSTR func)
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
 *		ThunkConnect32		(KERNEL32.@)
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

        TRACE("SL01 thunk %s (%p) <- %s (%s), Reason: %ld\n",
              module32, TD, module16, thunkfun16, dwReason);
    }
    else if (!strncmp(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE("LS01 thunk %s (%p) -> %s (%s), Reason: %ld\n",
              module32, TD, module16, thunkfun16, dwReason);
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

                TRACE("Process %08lx allocated TargetDB entry for ThunkDataSL %p\n",
                      GetCurrentProcessId(), SL32->data);
            }
            else
            {
                struct ThunkDataLS32 *LS32 = (struct ThunkDataLS32 *)TD;
                struct ThunkDataLS16 *LS16 = (struct ThunkDataLS16 *)TD16;

                LS32->targetTable = MapSL(LS16->targetTable);

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
 * 		QT_Thunk			(KERNEL32.@)
 *
 * The target address is in EDX.
 * The 16bit arguments start at ESP.
 * The number of 16bit argument bytes is EBP-ESP-0x40 (64 Byte thunksetup).
 * So the stack layout is 16bit argument bytes and then the 64 byte
 * scratch buffer.
 * The scratch buffer is used as work space by Windows' QT_Thunk
 * function.
 * As the programs unfortunately don't always provide a fixed size
 * scratch buffer (danger, stack corruption ahead !!), we simply resort
 * to copying over the whole EBP-ESP range to the 16bit stack
 * (as there's no way to safely figure out the param count
 * due to this misbehaviour of some programs).
 * [ok]
 *
 * See DDJ article 9614c for a very good description of QT_Thunk (also
 * available online !).
 *
 * FIXME: DDJ talks of certain register usage rules; I'm not sure
 * whether we cover this 100%.
 */
void WINAPI __regs_QT_Thunk( CONTEXT *context )
{
    CONTEXT context16;
    DWORD argsize;

    context16 = *context;

    context16.SegCs = HIWORD(context->Edx);
    context16.Eip   = LOWORD(context->Edx);
    /* point EBP to the STACK16FRAME on the stack
     * for the call_to_16 to set up the register content on calling */
    context16.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME,bp);

    /*
     * used to be (problematic):
     * argsize = context->Ebp - context->Esp - 0x40;
     * due to some programs abusing the API, we better assume the full
     * EBP - ESP range for copying instead: */
    argsize = context->Ebp - context->Esp;

    /* ok, too much is insane; let's limit param count a bit again */
    if (argsize > 64)
	argsize = 64; /* 32 WORDs */

    WOWCallback16Ex( 0, WCB16_REGS, argsize, (void *)context->Esp, (DWORD *)&context16 );
    context->Eax = context16.Eax;
    context->Edx = context16.Edx;
    context->Ecx = context16.Ecx;

    /* make sure to update the Win32 ESP, too, in order to throw away
     * the number of parameters that the Win16 function
     * accepted (that it popped from the corresponding Win16 stack) */
    context->Esp +=   LOWORD(context16.Esp) - (CURRENT_SP - argsize);
}
DEFINE_REGS_ENTRYPOINT( QT_Thunk )


/**********************************************************************
 * 		FT_Prolog			(KERNEL32.@)
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
void WINAPI __regs_FT_Prolog( CONTEXT *context )
{
    /* Build stack frame */
    stack32_push(context, context->Ebp);
    context->Ebp = context->Esp;

    /* Allocate 64-byte Thunk Buffer */
    context->Esp -= 64;
    memset((char *)context->Esp, '\0', 64);

    /* Store Flags (ECX) and Target Address (EDX) */
    /* Save other registers to be restored later */
    *(DWORD *)(context->Ebp -  4) = context->Ebx;
    *(DWORD *)(context->Ebp -  8) = context->Esi;
    *(DWORD *)(context->Ebp - 12) = context->Edi;
    *(DWORD *)(context->Ebp - 16) = context->Ecx;

    *(DWORD *)(context->Ebp - 48) = context->Eax;
    *(DWORD *)(context->Ebp - 52) = context->Edx;
}
DEFINE_REGS_ENTRYPOINT( FT_Prolog )

/**********************************************************************
 * 		FT_Thunk			(KERNEL32.@)
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
void WINAPI __regs_FT_Thunk( CONTEXT *context )
{
    DWORD mapESPrelative = *(DWORD *)(context->Ebp - 20);
    DWORD callTarget     = *(DWORD *)(context->Ebp - 52);

    CONTEXT context16;
    DWORD i, argsize;
    DWORD newstack[32];
    LPBYTE oldstack;

    context16 = *context;

    context16.SegCs = HIWORD(callTarget);
    context16.Eip   = LOWORD(callTarget);
    context16.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME,bp);

    argsize  = context->Ebp-context->Esp-0x40;
    if (argsize > sizeof(newstack)) argsize = sizeof(newstack);
    oldstack = (LPBYTE)context->Esp;

    memcpy( newstack, oldstack, argsize );

    for (i = 0; i < 32; i++)	/* NOTE: What about > 32 arguments? */
	if (mapESPrelative & (1 << i))
	{
	    SEGPTR *arg = (SEGPTR *)newstack[i];
	    *arg = MAKESEGPTR( CURRENT_SS, CURRENT_SP - argsize + (*(LPBYTE *)arg - oldstack));
	}

    WOWCallback16Ex( 0, WCB16_REGS, argsize, newstack, (DWORD *)&context16 );
    context->Eax = context16.Eax;
    context->Edx = context16.Edx;
    context->Ecx = context16.Ecx;

    context->Esp +=   LOWORD(context16.Esp) - (CURRENT_SP - argsize);

    /* Copy modified buffers back to 32-bit stack */
    memcpy( oldstack, newstack, argsize );
}
DEFINE_REGS_ENTRYPOINT( FT_Thunk )

/***********************************************************************
 *		FT_Exit0 (KERNEL32.@)
 *		FT_Exit4 (KERNEL32.@)
 *		FT_Exit8 (KERNEL32.@)
 *		FT_Exit12 (KERNEL32.@)
 *		FT_Exit16 (KERNEL32.@)
 *		FT_Exit20 (KERNEL32.@)
 *		FT_Exit24 (KERNEL32.@)
 *		FT_Exit28 (KERNEL32.@)
 *		FT_Exit32 (KERNEL32.@)
 *		FT_Exit36 (KERNEL32.@)
 *		FT_Exit40 (KERNEL32.@)
 *		FT_Exit44 (KERNEL32.@)
 *		FT_Exit48 (KERNEL32.@)
 *		FT_Exit52 (KERNEL32.@)
 *		FT_Exit56 (KERNEL32.@)
 *
 * One of the FT_ExitNN functions is called at the end of the thunk code.
 * It removes the stack frame created by FT_Prolog, moves the function
 * return from EBX to EAX (yes, FT_Thunk did use EAX for the return
 * value, but the thunk code has moved it from EAX to EBX in the
 * meantime ... :-), restores the caller's EBX, ESI, and EDI registers,
 * and perform a return to the CALLER of the thunk code (while removing
 * the given number of arguments from the caller's stack).
 */
#define FT_EXIT_RESTORE_REGS \
    "movl %ebx,%eax\n\t" \
    "movl -4(%ebp),%ebx\n\t" \
    "movl -8(%ebp),%esi\n\t" \
    "movl -12(%ebp),%edi\n\t" \
    "leave\n\t"

#define DEFINE_FT_Exit(n) \
    __ASM_STDCALL_FUNC( FT_Exit ## n, 0, FT_EXIT_RESTORE_REGS "ret $" #n )

DEFINE_FT_Exit(0)
DEFINE_FT_Exit(4)
DEFINE_FT_Exit(8)
DEFINE_FT_Exit(12)
DEFINE_FT_Exit(16)
DEFINE_FT_Exit(20)
DEFINE_FT_Exit(24)
DEFINE_FT_Exit(28)
DEFINE_FT_Exit(32)
DEFINE_FT_Exit(36)
DEFINE_FT_Exit(40)
DEFINE_FT_Exit(44)
DEFINE_FT_Exit(48)
DEFINE_FT_Exit(52)
DEFINE_FT_Exit(56)


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
	*thunk = addr[1];

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
 * the stack.  The number of arguments to be popped is returned
 * in the BL register by the called 16-bit routine.
 *
 */
void WINAPI __regs_Common32ThkLS( CONTEXT *context )
{
    CONTEXT context16;
    DWORD argsize;

    context16 = *context;

    context16.Edi   = LOWORD(context->Ecx);
    context16.SegCs = HIWORD(context->Eax);
    context16.Eip   = LOWORD(context->Eax);
    context16.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME,bp);

    argsize = HIWORD(context->Edx) * 4;

    /* FIXME: hack for stupid USER32 CallbackGlueLS routine */
    if (context->Edx == context->Eip)
        argsize = 6 * 4;

    /* Note: the first 32 bytes we copy are just garbage from the 32-bit stack, in order to reserve
     *       the space. It is safe to do that since the register function prefix has reserved
     *       a lot more space than that below context->Esp.
     */
    WOWCallback16Ex( 0, WCB16_REGS, argsize + 32, (LPBYTE)context->Esp - 32, (DWORD *)&context16 );
    context->Eax = context16.Eax;

    /* Clean up caller's stack frame */
    context->Esp += LOBYTE(context16.Ebx);
}
DEFINE_REGS_ENTRYPOINT( Common32ThkLS )

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
void WINAPI __regs_OT_32ThkLSF( CONTEXT *context )
{
    CONTEXT context16;
    DWORD argsize;

    context16 = *context;

    context16.SegCs = HIWORD(context->Edx);
    context16.Eip   = LOWORD(context->Edx);
    context16.Ebp   = CURRENT_SP + FIELD_OFFSET(STACK16FRAME,bp);

    argsize = 2 * *(WORD *)context->Esp + 2;

    WOWCallback16Ex( 0, WCB16_REGS, argsize, (void *)context->Esp, (DWORD *)&context16 );
    context->Eax = context16.Eax;
    context->Edx = context16.Edx;

    /* Copy modified buffers back to 32-bit stack */
    memcpy( (LPBYTE)context->Esp,
            (LPBYTE)CURRENT_STACK16 - argsize, argsize );

    context->Esp +=   LOWORD(context16.Esp) - (CURRENT_SP - argsize);
}
DEFINE_REGS_ENTRYPOINT( OT_32ThkLSF )

/***********************************************************************
 *		ThunkInitLSF		(KERNEL32.41)
 * A thunk setup routine.
 * Expects a pointer to a preinitialized thunkbuffer in the first argument
 * looking like:
 *|	00..03:		unknown	(pointer, check _41, _43, _46)
 *|	04: EB1E		jmp +0x20
 *|
 *|	06..23:		unknown (space for replacement code, check .90)
 *|
 *|	24:>E800000000		call offset 29
 *|	29:>58			pop eax		   ( target of call )
 *|	2A: 2D25000000		sub eax,0x00000025 ( now points to offset 4 )
 *|	2F: BAxxxxxxxx		mov edx,xxxxxxxx
 *|	34: 68yyyyyyyy		push KERNEL32.90
 *|	39: C3			ret
 *|
 *|	3A: EB1E		jmp +0x20
 *|	3E ... 59:	unknown (space for replacement code?)
 *|	5A: E8xxxxxxxx		call <32bitoffset xxxxxxxx>
 *|	5F: 5A			pop edx
 *|	60: 81EA25xxxxxx	sub edx, 0x25xxxxxx
 *|	66: 52			push edx
 *|	67: 68xxxxxxxx		push xxxxxxxx
 *|	6C: 68yyyyyyyy		push KERNEL32.89
 *|	71: C3			ret
 *|	72: end?
 * This function checks if the code is there, and replaces the yyyyyyyy entries
 * by the functionpointers.
 * The thunkbuf looks like:
 *
 *|	00: DWORD	length		? don't know exactly
 *|	04: SEGPTR	ptr		? where does it point to?
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
	LPDWORD		addr,addr2;

	/* FIXME: add checks for valid code ... */
	/* write pointers to kernel32.89 and kernel32.90 (+ordinal base of 1) */
	*(void **)(thunk+0x35) = QT_ThunkPrime;
	*(void **)(thunk+0x6D) = FT_PrologPrime;

	if (!(addr = _loadthunk( dll16, thkbuf, dll32, NULL, len )))
		return 0;

	addr2 = MapSL(addr[1]);
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
void WINAPI __regs_FT_PrologPrime( CONTEXT *context )
{
    DWORD  targetTableOffset;
    LPBYTE relayCode;

    /* Compensate for the fact that the Wine register relay code thought
       we were being called, although we were in fact jumped to */
    context->Esp -= 4;

    /* Write FT_Prolog call stub */
    targetTableOffset = stack32_pop(context);
    relayCode = (LPBYTE)stack32_pop(context);
    _write_ftprolog( relayCode, *(DWORD **)(relayCode+targetTableOffset) );

    /* Jump to the call stub just created */
    context->Eip = (DWORD)relayCode;
}
DEFINE_REGS_ENTRYPOINT( FT_PrologPrime )

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
void WINAPI __regs_QT_ThunkPrime( CONTEXT *context )
{
    DWORD  targetTableOffset;
    LPBYTE relayCode;

    /* Compensate for the fact that the Wine register relay code thought
       we were being called, although we were in fact jumped to */
    context->Esp -= 4;

    /* Write QT_Thunk call stub */
    targetTableOffset = context->Edx;
    relayCode = (LPBYTE)context->Eax;
    _write_qtthunk( relayCode, *(DWORD **)(relayCode+targetTableOffset) );

    /* Jump to the call stub just created */
    context->Eip = (DWORD)relayCode;
}
DEFINE_REGS_ENTRYPOINT( QT_ThunkPrime )

/***********************************************************************
 *		ThunkInitSL (KERNEL32.46)
 * Another thunkbuf link routine.
 * The start of the thunkbuf looks like this:
 * 	00: DWORD	length
 *	04: SEGPTR	address for thunkbuffer pointer
 * [ok probably]
 *
 * RETURNS
 *  Nothing.
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

	*(DWORD*)MapSL(addr[1]) = (DWORD)thunk;
}

/**********************************************************************
 *           SSInit		(KERNEL.700)
 * RETURNS
 *	TRUE for success.
 */
BOOL WINAPI SSInit16(void)
{
    return TRUE;
}

/**********************************************************************
 *           SSOnBigStack	(KERNEL32.87)
 * Check if thunking is initialized (ss selector set up etc.)
 * We do that differently, so just return TRUE.
 * [ok]
 * RETURNS
 *	TRUE for success.
 */
BOOL WINAPI SSOnBigStack(void)
{
    TRACE("Yes, thunking is initialized\n");
    return TRUE;
}

/**********************************************************************
 *           SSConfirmSmallStack     (KERNEL.704)
 *
 * Abort if not on small stack.
 *
 * This must be a register routine as it has to preserve *all* registers.
 */
void WINAPI SSConfirmSmallStack( CONTEXT *context )
{
    /* We are always on the small stack while in 16-bit code ... */
}

/**********************************************************************
 *           SSCall (KERNEL32.88)
 * One of the real thunking functions. This one seems to be for 32<->32
 * thunks. It should probably be capable of crossing processboundaries.
 *
 * And YES, I've seen nr=48 (somewhere in the Win95 32<->16 OLE coupling)
 * [ok]
 *
 * RETURNS
 *  Thunked function result.
 */
DWORD WINAPIV SSCall(
	DWORD nr,	/* [in] number of argument bytes */
	DWORD flags,	/* [in] FIXME: flags ? */
	FARPROC fun,	/* [in] function to call */
	...		/* [in/out] arguments */
) {
    return call_entry_point( fun, nr / sizeof(DWORD), (DWORD *)&fun + 1 );
}

/**********************************************************************
 *           W32S_BackTo32                      (KERNEL32.51)
 */
void WINAPI __regs_W32S_BackTo32( CONTEXT *context )
{
    LPDWORD stack = (LPDWORD)context->Esp;
    FARPROC proc = (FARPROC)context->Eip;

    context->Eax = call_entry_point( proc, 10, stack + 1 );
    context->Eip = stack32_pop(context);
}
DEFINE_REGS_ENTRYPOINT( W32S_BackTo32 )

/**********************************************************************
 *			AllocSLCallback		(KERNEL32.@)
 *
 * Allocate a 16->32 callback.
 *
 * NOTES
 * Win95 uses some structchains for callbacks. It allocates them
 * in blocks of 100 entries, size 32 bytes each, layout:
 * blockstart:
 *| 	0:	PTR	nextblockstart
 *|	4:	entry	*first;
 *|	8:	WORD	sel ( start points to blockstart)
 *|	A:	WORD	unknown
 * 100xentry:
 *|	00..17:		Code
 *|	18:	PDB	*owning_process;
 *|	1C:	PTR	blockstart
 *
 * We ignore this for now. (Just a note for further developers)
 * FIXME: use this method, so we don't waste selectors...
 *
 * Following code is then generated by AllocSLCallback. The code is 16 bit, so
 * the 0x66 prefix switches from word->long registers.
 *
 *|	665A		pop	edx
 *|	6668x arg2 x 	pushl	<arg2>
 *|	6652		push	edx
 *|	EAx arg1 x	jmpf	<arg1>
 *
 * returns the startaddress of this thunk.
 *
 * Note, that they look very similar to the ones allocates by THUNK_Alloc.
 * RETURNS
 *	A segmented pointer to the start of the thunk
 */
DWORD WINAPI
AllocSLCallback(
	DWORD finalizer,	/* [in] Finalizer function */
	DWORD callback		/* [in] Callback function */
) {
	LPBYTE	x,thunk = HeapAlloc( GetProcessHeap(), 0, 32 );
	WORD	sel;

	x=thunk;
	*x++=0x66;*x++=0x5a;				/* popl edx */
	*x++=0x66;*x++=0x68;*(DWORD*)x=finalizer;x+=4;	/* pushl finalizer */
	*x++=0x66;*x++=0x52;				/* pushl edx */
	*x++=0xea;*(DWORD*)x=callback;x+=4;		/* jmpf callback */

	*(DWORD*)(thunk+18) = GetCurrentProcessId();

	sel = SELECTOR_AllocBlock( thunk, 32, LDT_FLAGS_CODE );
	return (sel<<16)|0;
}

/**********************************************************************
 * 		FreeSLCallback		(KERNEL32.@)
 * Frees the specified 16->32 callback
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI
FreeSLCallback(
	DWORD x	/* [in] 16 bit callback (segmented pointer?) */
) {
	FIXME("(0x%08lx): stub\n",x);
}

/**********************************************************************
 * 		AllocMappedBuffer	(KERNEL32.38)
 *
 * This is an undocumented KERNEL32 function that
 * SMapLS's a GlobalAlloc'ed buffer.
 *
 * RETURNS
 *       EDI register: pointer to buffer
 *
 * NOTES
 *       The buffer is preceded by 8 bytes:
 *        ...
 *       edi+0   buffer
 *       edi-4   SEGPTR to buffer
 *       edi-8   some magic Win95 needs for SUnMapLS
 *               (we use it for the memory handle)
 *
 *       The SEGPTR is used by the caller!
 */
void WINAPI __regs_AllocMappedBuffer(
              CONTEXT *context /* [in] EDI register: size of buffer to allocate */
) {
    HGLOBAL handle = GlobalAlloc(0, context->Edi + 8);
    DWORD *buffer = GlobalLock(handle);
    DWORD ptr = 0;

    if (buffer)
        if (!(ptr = MapLS(buffer + 2)))
        {
            GlobalUnlock(handle);
            GlobalFree(handle);
        }

    if (!ptr)
        context->Eax = context->Edi = 0;
    else
    {
        buffer[0] = (DWORD)handle;
        buffer[1] = ptr;

        context->Eax = ptr;
        context->Edi = (DWORD)(buffer + 2);
    }
}
DEFINE_REGS_ENTRYPOINT( AllocMappedBuffer )

/**********************************************************************
 * 		FreeMappedBuffer	(KERNEL32.39)
 *
 * Free a buffer allocated by AllocMappedBuffer
 *
 * RETURNS
 *  Nothing.
 */
void WINAPI __regs_FreeMappedBuffer(
              CONTEXT *context /* [in] EDI register: pointer to buffer */
) {
    if (context->Edi)
    {
        DWORD *buffer = (DWORD *)context->Edi - 2;

        UnMapLS(buffer[1]);

        GlobalUnlock((HGLOBAL)buffer[0]);
        GlobalFree((HGLOBAL)buffer[0]);
    }
}
DEFINE_REGS_ENTRYPOINT( FreeMappedBuffer )

/**********************************************************************
 * 		GetTEBSelectorFS	(KERNEL.475)
 * 	Set the 16-bit %fs to the 32-bit %fs (current TEB selector)
 */
void WINAPI GetTEBSelectorFS16(void)
{
    CURRENT_STACK16->fs = get_fs();
}

/**********************************************************************
 * 		IsPeFormat		(KERNEL.431)
 *
 * Determine if a file is a PE format executable.
 *
 * RETURNS
 *  TRUE, if it is.
 *  FALSE if the file could not be opened or is not a PE file.
 *
 * NOTES
 *  If fn is given as NULL then the function expects hf16 to be valid.
 */
BOOL16 WINAPI IsPeFormat16(
	LPSTR	fn,	/* [in] Filename to the executable */
	HFILE16 hf16)	/* [in] An open file handle */
{
    BOOL ret = FALSE;
    IMAGE_DOS_HEADER mzh;
    OFSTRUCT ofs;
    DWORD xmagic;

    if (fn) hf16 = OpenFile16(fn,&ofs,OF_READ);
    if (hf16 == HFILE_ERROR16) return FALSE;
    _llseek16(hf16,0,SEEK_SET);
    if (sizeof(mzh)!=_lread16(hf16,&mzh,sizeof(mzh))) goto done;
    if (mzh.e_magic!=IMAGE_DOS_SIGNATURE) goto done;
    _llseek16(hf16,mzh.e_lfanew,SEEK_SET);
    if (sizeof(DWORD)!=_lread16(hf16,&xmagic,sizeof(DWORD))) goto done;
    ret = (xmagic == IMAGE_NT_SIGNATURE);
 done:
    _lclose16(hf16);
    return ret;
}


/***********************************************************************
 *           K32Thk1632Prolog			(KERNEL32.@)
 */
void WINAPI __regs_K32Thk1632Prolog( CONTEXT *context )
{
   LPBYTE code = (LPBYTE)context->Eip - 5;

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
      DWORD argSize = context->Ebp - context->Esp;
      char *stack16 = (char *)context->Esp - 4;
      STACK16FRAME *frame16 = (STACK16FRAME *)stack16 - 1;
      STACK32FRAME *frame32 = (STACK32FRAME *)kernel_get_thread_data()->stack;
      char *stack32 = (char *)frame32 - argSize;
      WORD  stackSel  = SELECTOROF(frame32->frame16);
      DWORD stackBase = GetSelectorBase(stackSel);

      TRACE("before SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %04x:%04x\n",
            context->Ebp, context->Esp, CURRENT_SS, CURRENT_SP);

      memset(frame16, '\0', sizeof(STACK16FRAME));
      frame16->frame32 = frame32;
      frame16->ebp = context->Ebp;

      memcpy(stack32, stack16, argSize);
      CURRENT_SS = stackSel;
      CURRENT_SP = (DWORD)frame16 - stackBase;

      context->Esp = (DWORD)stack32 + 4;
      context->Ebp = context->Esp + argSize;

      TRACE("after  SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %04x:%04x\n",
            context->Ebp, context->Esp, CURRENT_SS, CURRENT_SP);
   }

    /* entry_point is never used again once the entry point has
       been called.  Thus we re-use it to hold the Win16Lock count */
   ReleaseThunkLock(&CURRENT_STACK16->entry_point);
}
DEFINE_REGS_ENTRYPOINT( K32Thk1632Prolog )

/***********************************************************************
 *           K32Thk1632Epilog			(KERNEL32.@)
 */
void WINAPI __regs_K32Thk1632Epilog( CONTEXT *context )
{
   LPBYTE code = (LPBYTE)context->Eip - 13;

   RestoreThunkLock(CURRENT_STACK16->entry_point);

   /* We undo the SYSTHUNK hack if necessary. See K32Thk1632Prolog. */

   if (   code[5] == 0xFF && code[6] == 0x55 && code[7] == 0xFC
       && code[13] == 0x66 && code[14] == 0xCB)
   {
      STACK16FRAME *frame16 = CURRENT_STACK16;
      char *stack16 = (char *)(frame16 + 1);
      DWORD argSize = frame16->ebp - (DWORD)stack16;
      char *stack32 = (char *)frame16->frame32 - argSize;

      DWORD nArgsPopped = context->Esp - (DWORD)stack32;

      TRACE("before SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %04x:%04x\n",
            context->Ebp, context->Esp, CURRENT_SS, CURRENT_SP);

      kernel_get_thread_data()->stack = (SEGPTR)frame16->frame32;

      context->Esp = (DWORD)stack16 + nArgsPopped;
      context->Ebp = frame16->ebp;

      TRACE("after  SYSTHUNK hack: EBP: %08lx ESP: %08lx cur_stack: %04x:%04x\n",
            context->Ebp, context->Esp, CURRENT_SS, CURRENT_SP);
   }
}
DEFINE_REGS_ENTRYPOINT( K32Thk1632Epilog )

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
       strcpy(strPtr, "WINESTUB.FIX");
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

        TRACE("SL01 thunk %s (%p) -> %s (%s), Reason: %ld\n",
              module16, TD, module32, thunkfun32, dwReason);
    }
    else if (!strncmp(TD->magic, "LS01", 4))
    {
        directionSL = FALSE;

        TRACE("LS01 thunk %s (%p) <- %s (%s), Reason: %ld\n",
              module16, TD, module32, thunkfun32, dwReason);
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

                    SL->apiDB    = MapSL(SL16->apiDatabase);
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

void WINAPI C16ThkSL(CONTEXT *context)
{
    LPBYTE stub = MapSL(context->Eax), x = stub;

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
     *   call __FLATCS:__wine_call_from_16_thunk
     */

    *x++ = 0xB8; *(WORD *)x = get_ds(); x += sizeof(WORD);
    *x++ = 0x8E; *x++ = 0xC0;
    *x++ = 0x66; *x++ = 0x0F; *x++ = 0xB7; *x++ = 0xC9;
    *x++ = 0x67; *x++ = 0x66; *x++ = 0x26; *x++ = 0x8B;
                 *x++ = 0x91; *(DWORD *)x = context->Edx; x += sizeof(DWORD);

    *x++ = 0x55;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x52;
    *x++ = 0x66; *x++ = 0x9A;
    *(void **)x = __wine_call_from_16_thunk; x += sizeof(void *);
    *(WORD *)x = get_cs(); x += sizeof(WORD);

    /* Jump to the stub code just created */
    context->Eip = LOWORD(context->Eax);
    context->SegCs  = HIWORD(context->Eax);

    /* Since C16ThkSL got called by a jmp, we need to leave the
       original return address on the stack */
    context->Esp -= 4;
}

/***********************************************************************
 *           C16ThkSL01                         (KERNEL.631)
 */

void WINAPI C16ThkSL01(CONTEXT *context)
{
    LPBYTE stub = MapSL(context->Eax), x = stub;

    if (stub)
    {
        struct ThunkDataSL16 *SL16 = MapSL(context->Edx);
        struct ThunkDataSL *td = SL16->fpData;

        DWORD procAddress = (DWORD)GetProcAddress16(GetModuleHandle16("KERNEL"), (LPCSTR)631);

        if (!td)
        {
            ERR("ThunkConnect16 was not called!\n");
            return;
        }

        TRACE("Creating stub for ThunkDataSL %p\n", td);


        /* We produce the following code:
         *
         *   xor eax, eax
         *   mov edx, $td
         *   call C16ThkSL01
         *   push bp
         *   push edx
         *   push dx
         *   push edx
         *   call __FLATCS:__wine_call_from_16_thunk
         */

        *x++ = 0x66; *x++ = 0x33; *x++ = 0xC0;
        *x++ = 0x66; *x++ = 0xBA; *(void **)x = td; x += sizeof(void *);
        *x++ = 0x9A; *(DWORD *)x = procAddress; x += sizeof(DWORD);

        *x++ = 0x55;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x52;
        *x++ = 0x66; *x++ = 0x9A;
        *(void **)x = __wine_call_from_16_thunk; x += sizeof(void *);
        *(WORD *)x = get_cs(); x += sizeof(WORD);

        /* Jump to the stub code just created */
        context->Eip = LOWORD(context->Eax);
        context->SegCs  = HIWORD(context->Eax);

        /* Since C16ThkSL01 got called by a jmp, we need to leave the
           original return address on the stack */
        context->Esp -= 4;
    }
    else
    {
        struct ThunkDataSL *td = (struct ThunkDataSL *)context->Edx;
        DWORD targetNr = LOWORD(context->Ecx) / 4;
        struct SLTargetDB *tdb;

        TRACE("Process %08lx calling target %ld of ThunkDataSL %p\n",
              GetCurrentProcessId(), targetNr, td);

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
            context->Edx = tdb->targetTable[targetNr];

            TRACE("Call target is %08lx\n", context->Edx);
        }
        else
        {
            WORD *stack = MapSL( MAKESEGPTR(context->SegSs, LOWORD(context->Esp)) );
            context->Edx = (context->Edx & ~0xffff) | HIWORD(td->apiDB[targetNr].errorReturnValue);
            context->Eax = (context->Eax & ~0xffff) | LOWORD(td->apiDB[targetNr].errorReturnValue);
            context->Eip = stack[2];
            context->SegCs  = stack[3];
            context->Esp += td->apiDB[targetNr].nrArgBytes + 4;

            ERR("Process %08lx did not ThunkConnect32 %s to %s\n",
                GetCurrentProcessId(), td->pszDll32, td->pszDll16);
        }
    }
}


/***********************************************************************
 * 16<->32 Thunklet/Callback API:
 */

#pragma pack(push,1)
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
#pragma pack(pop)

#define THUNKLET_TYPE_LS  1
#define THUNKLET_TYPE_SL  2

static HANDLE  ThunkletHeap = 0;
static WORD ThunkletCodeSel;
static THUNKLET *ThunkletAnchor = NULL;

static FARPROC ThunkletSysthunkGlueLS = 0;
static SEGPTR    ThunkletSysthunkGlueSL = 0;

static FARPROC ThunkletCallbackGlueLS = 0;
static SEGPTR    ThunkletCallbackGlueSL = 0;


/* map a thunk allocated on ThunkletHeap to a 16-bit pointer */
static inline SEGPTR get_segptr( void *thunk )
{
    if (!thunk) return 0;
    return MAKESEGPTR( ThunkletCodeSel, (char *)thunk - (char *)ThunkletHeap );
}

/***********************************************************************
 *           THUNK_Init
 */
static BOOL THUNK_Init(void)
{
    LPBYTE thunk;

    ThunkletHeap = HeapCreate( HEAP_CREATE_ENABLE_EXECUTE, 0x10000, 0x10000 );
    if (!ThunkletHeap) return FALSE;

    ThunkletCodeSel = SELECTOR_AllocBlock( ThunkletHeap, 0x10000, LDT_FLAGS_CODE );

    thunk = HeapAlloc( ThunkletHeap, 0, 5 );
    if (!thunk) return FALSE;

    ThunkletSysthunkGlueLS = (FARPROC)thunk;
    *thunk++ = 0x58;                             /* popl eax */
    *thunk++ = 0xC3;                             /* ret      */

    ThunkletSysthunkGlueSL = get_segptr( thunk );
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
static THUNKLET *THUNK_FindThunklet( DWORD target, DWORD relay,
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
static FARPROC THUNK_AllocLSThunklet( SEGPTR target, DWORD relay,
                                      FARPROC glue, HTASK16 owner )
{
    THUNKLET *thunk = THUNK_FindThunklet( (DWORD)target, relay, (DWORD)glue,
                                          THUNKLET_TYPE_LS );
    if (!thunk)
    {
        TDB *pTask = GlobalLock16( owner );

        if (!ThunkletHeap) THUNK_Init();
        if ( !(thunk = HeapAlloc( ThunkletHeap, 0, sizeof(THUNKLET) )) )
            return 0;

        thunk->prefix_target = thunk->prefix_relay = 0x90;
        thunk->pushl_target  = thunk->pushl_relay  = 0x68;
        thunk->jmp_glue = 0xE9;

        thunk->target  = (DWORD)target;
        thunk->relay   = relay;
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
static SEGPTR THUNK_AllocSLThunklet( FARPROC target, DWORD relay,
                                     SEGPTR glue, HTASK16 owner )
{
    THUNKLET *thunk = THUNK_FindThunklet( (DWORD)target, relay, (DWORD)glue,
                                          THUNKLET_TYPE_SL );
    if (!thunk)
    {
        TDB *pTask = GlobalLock16( owner );

        if (!ThunkletHeap) THUNK_Init();
        if ( !(thunk = HeapAlloc( ThunkletHeap, 0, sizeof(THUNKLET) )) )
            return 0;

        thunk->prefix_target = thunk->prefix_relay = 0x66;
        thunk->pushl_target  = thunk->pushl_relay  = 0x68;
        thunk->jmp_glue = 0xEA;

        thunk->target  = (DWORD)target;
        thunk->relay   = relay;
        thunk->glue    = (DWORD)glue;

        thunk->type    = THUNKLET_TYPE_SL;
        thunk->owner   = pTask? pTask->hInstance : 0;

        thunk->next    = ThunkletAnchor;
        ThunkletAnchor = thunk;
    }

    return get_segptr( thunk );
}

/**********************************************************************
 *     IsLSThunklet
 */
static BOOL16 IsLSThunklet( THUNKLET *thunk )
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
    if (!ThunkletSysthunkGlueLS) THUNK_Init();
    return THUNK_AllocLSThunklet( (SEGPTR)relay, (DWORD)target,
                                  ThunkletSysthunkGlueLS, GetCurrentTask() );
}

/***********************************************************************
 *     AllocSLThunkletSysthunk             (KERNEL.608)
 */
SEGPTR WINAPI AllocSLThunkletSysthunk16( FARPROC target,
                                       SEGPTR relay, DWORD dummy )
{
    if (!ThunkletSysthunkGlueSL) THUNK_Init();
    return THUNK_AllocSLThunklet( (FARPROC)relay, (DWORD)target,
                                  ThunkletSysthunkGlueSL, GetCurrentTask() );
}


/***********************************************************************
 *     AllocLSThunkletCallbackEx           (KERNEL.567)
 */
FARPROC WINAPI AllocLSThunkletCallbackEx16( SEGPTR target,
                                            DWORD relay, HTASK16 task )
{
    THUNKLET *thunk = MapSL( target );
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
 *     AllocLSThunkletCallback             (KERNEL.561)
 *     AllocLSThunkletCallback_dup         (KERNEL.606)
 */
FARPROC WINAPI AllocLSThunkletCallback16( SEGPTR target, DWORD relay )
{
    return AllocLSThunkletCallbackEx16( target, relay, GetCurrentTask() );
}

/***********************************************************************
 *     AllocSLThunkletCallback             (KERNEL.562)
 *     AllocSLThunkletCallback_dup         (KERNEL.605)
 */
SEGPTR WINAPI AllocSLThunkletCallback16( FARPROC target, DWORD relay )
{
    return AllocSLThunkletCallbackEx16( target, relay, GetCurrentTask() );
}

/***********************************************************************
 *     FindLSThunkletCallback              (KERNEL.563)
 *     FindLSThunkletCallback_dup          (KERNEL.609)
 */
FARPROC WINAPI FindLSThunkletCallback( SEGPTR target, DWORD relay )
{
    THUNKLET *thunk = MapSL( target );
    if (   thunk && IsSLThunklet16( thunk ) && thunk->relay == relay
        && thunk->glue == (DWORD)ThunkletCallbackGlueSL )
        return (FARPROC)thunk->target;

    thunk = THUNK_FindThunklet( (DWORD)target, relay,
                                (DWORD)ThunkletCallbackGlueLS,
                                THUNKLET_TYPE_LS );
    return (FARPROC)thunk;
}

/***********************************************************************
 *     FindSLThunkletCallback              (KERNEL.564)
 *     FindSLThunkletCallback_dup          (KERNEL.610)
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
    return get_segptr( thunk );
}


/***********************************************************************
 *     FreeThunklet            (KERNEL.611)
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
    SEGPTR glueSL = (SEGPTR)GetProcAddress16( kernel, (LPCSTR)604 );

    SetThunkletCallbackGlue16( glueLS, glueSL );
}

/***********************************************************************
 *     CBClientGlueSL                      (KERNEL.604)
 */
void WINAPI CBClientGlueSL( CONTEXT *context )
{
    /* Create stack frame */
    SEGPTR stackSeg = stack16_push( 12 );
    LPWORD stackLin = MapSL( stackSeg );
    SEGPTR glue, *glueTab;

    stackLin[3] = (WORD)context->Ebp;
    stackLin[2] = (WORD)context->Esi;
    stackLin[1] = (WORD)context->Edi;
    stackLin[0] = (WORD)context->SegDs;

    context->Ebp = OFFSETOF( stackSeg ) + 6;
    context->Esp = OFFSETOF( stackSeg ) - 4;
    context->SegGs = 0;

    /* Jump to 16-bit relay code */
    glueTab = MapSL( CBClientRelay16[ stackLin[5] ] );
    glue = glueTab[ stackLin[4] ];
    context->SegCs = SELECTOROF( glue );
    context->Eip   = OFFSETOF  ( glue );
}

/*******************************************************************
 *         CALL32_CBClient
 *
 * Call a CBClient relay stub from 32-bit code (KERNEL.620).
 *
 * Since the relay stub is itself 32-bit, this should not be a problem;
 * unfortunately, the relay stubs are expected to switch back to a
 * 16-bit stack (and 16-bit code) after completion :-(
 *
 * This would conflict with our 16- vs. 32-bit stack handling, so
 * we simply switch *back* to our 32-bit stack before returning to
 * the caller ...
 *
 * The CBClient relay stub expects to be called with the following
 * 16-bit stack layout, and with ebp and ebx pointing into the 16-bit
 * stack at the designated places:
 *
 *    ...
 *  (ebp+14) original arguments to the callback routine
 *  (ebp+10) far return address to original caller
 *  (ebp+6)  Thunklet target address
 *  (ebp+2)  Thunklet relay ID code
 *  (ebp)    BP (saved by CBClientGlueSL)
 *  (ebp-2)  SI (saved by CBClientGlueSL)
 *  (ebp-4)  DI (saved by CBClientGlueSL)
 *  (ebp-6)  DS (saved by CBClientGlueSL)
 *
 *   ...     buffer space used by the 16-bit side glue for temp copies
 *
 *  (ebx+4)  far return address to 16-bit side glue code
 *  (ebx)    saved 16-bit ss:sp (pointing to ebx+4)
 *
 * The 32-bit side glue code accesses both the original arguments (via ebp)
 * and the temporary copies prepared by the 16-bit side glue (via ebx).
 * After completion, the stub will load ss:sp from the buffer at ebx
 * and perform a far return to 16-bit code.
 *
 * To trick the relay stub into returning to us, we replace the 16-bit
 * return address to the glue code by a cs:ip pair pointing to our
 * return entry point (the original return address is saved first).
 * Our return stub thus called will then reload the 32-bit ss:esp and
 * return to 32-bit code (by using and ss:esp value that we have also
 * pushed onto the 16-bit stack before and a cs:eip values found at
 * that position on the 32-bit stack).  The ss:esp to be restored is
 * found relative to the 16-bit stack pointer at:
 *
 *  (ebx-4)   ss  (flat)
 *  (ebx-8)   sp  (32-bit stack pointer)
 *
 * The second variant of this routine, CALL32_CBClientEx, which is used
 * to implement KERNEL.621, has to cope with yet another problem: Here,
 * the 32-bit side directly returns to the caller of the CBClient thunklet,
 * restoring registers saved by CBClientGlueSL and cleaning up the stack.
 * As we have to return to our 32-bit code first, we have to adapt the
 * layout of our temporary area so as to include values for the registers
 * that are to be restored, and later (in the implementation of KERNEL.621)
 * we *really* restore them. The return stub restores DS, DI, SI, and BP
 * from the stack, skips the next 8 bytes (CBClient relay code / target),
 * and then performs a lret NN, where NN is the number of arguments to be
 * removed. Thus, we prepare our temporary area as follows:
 *
 *     (ebx+22) 16-bit cs  (this segment)
 *     (ebx+20) 16-bit ip  ('16-bit' return entry point)
 *     (ebx+16) 32-bit ss  (flat)
 *     (ebx+12) 32-bit sp  (32-bit stack pointer)
 *     (ebx+10) 16-bit bp  (points to ebx+24)
 *     (ebx+8)  16-bit si  (ignored)
 *     (ebx+6)  16-bit di  (ignored)
 *     (ebx+4)  16-bit ds  (we actually use the flat DS here)
 *     (ebx+2)  16-bit ss  (16-bit stack segment)
 *     (ebx+0)  16-bit sp  (points to ebx+4)
 *
 * Note that we ensure that DS is not changed and remains the flat segment,
 * and the 32-bit stack pointer our own return stub needs fits just
 * perfectly into the 8 bytes that are skipped by the Windows stub.
 * One problem is that we have to determine the number of removed arguments,
 * as these have to be really removed in KERNEL.621. Thus, the BP value
 * that we place in the temporary area to be restored, contains the value
 * that SP would have if no arguments were removed. By comparing the actual
 * value of SP with this value in our return stub we can compute the number
 * of removed arguments. This is then returned to KERNEL.621.
 *
 * The stack layout of this function:
 * (ebp+20)  nArgs     pointer to variable receiving nr. of args (Ex only)
 * (ebp+16)  esi       pointer to caller's esi value
 * (ebp+12)  arg       ebp value to be set for relay stub
 * (ebp+8)   func      CBClient relay stub address
 * (ebp+4)   ret addr
 * (ebp)     ebp
 */
extern DWORD CALL32_CBClient( FARPROC proc, LPWORD args, WORD *stackLin, DWORD *esi );
__ASM_GLOBAL_FUNC( CALL32_CBClient,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-12\n\t")
                   "movl 16(%ebp),%ebx\n\t"
                   "leal -8(%esp),%eax\n\t"
                   "movl %eax,-8(%ebx)\n\t"
                   "movl 20(%ebp),%esi\n\t"
                   "movl (%esi),%esi\n\t"
                   "movl 8(%ebp),%eax\n\t"
                   "movl 12(%ebp),%ebp\n\t"
                   "pushl %cs\n\t"
                   "call *%eax\n\t"
                   "movl 32(%esp),%edi\n\t"
                   "movl %esi,(%edi)\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret\n\t" )

/***********************************************************************
 *     CBClientThunkSL                      (KERNEL.620)
 */
void WINAPI CBClientThunkSL( CONTEXT *context )
{
    /* Call 32-bit relay code */

    LPWORD args = MapSL( MAKESEGPTR( context->SegSs, LOWORD(context->Ebp) ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];

    /* fill temporary area for the asm code (see comments in winebuild) */
    SEGPTR stack = stack16_push( 12 );
    LPWORD stackLin = MapSL(stack);
    /* stackLin[0] and stackLin[1] reserved for the 32-bit stack ptr */
    stackLin[2] = get_ds();
    stackLin[3] = 0;
    stackLin[4] = OFFSETOF(stack) + 12;
    stackLin[5] = SELECTOROF(stack);
    stackLin[6] = 0; /* overwrite return address */
    stackLin[7] = cbclient_selector;
    context->Eax = CALL32_CBClient( proc, args, stackLin + 4, &context->Esi );
    stack16_pop( 12 );
}

extern DWORD CALL32_CBClientEx( FARPROC proc, LPWORD args, WORD *stackLin, DWORD *esi, INT *nArgs );
__ASM_GLOBAL_FUNC( CALL32_CBClientEx,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %edi\n\t"
                   __ASM_CFI(".cfi_rel_offset %edi,-4\n\t")
                   "pushl %esi\n\t"
                   __ASM_CFI(".cfi_rel_offset %esi,-8\n\t")
                   "pushl %ebx\n\t"
                   __ASM_CFI(".cfi_rel_offset %ebx,-12\n\t")
                   "movl 16(%ebp),%ebx\n\t"
                   "leal -8(%esp),%eax\n\t"
                   "movl %eax,12(%ebx)\n\t"
                   "movl 20(%ebp),%esi\n\t"
                   "movl (%esi),%esi\n\t"
                   "movl 8(%ebp),%eax\n\t"
                   "movl 12(%ebp),%ebp\n\t"
                   "pushl %cs\n\t"
                   "call *%eax\n\t"
                   "movl 32(%esp),%edi\n\t"
                   "movl %esi,(%edi)\n\t"
                   "movl 36(%esp),%ebx\n\t"
                   "movl %ebp,(%ebx)\n\t"
                   "popl %ebx\n\t"
                   __ASM_CFI(".cfi_same_value %ebx\n\t")
                   "popl %esi\n\t"
                   __ASM_CFI(".cfi_same_value %esi\n\t")
                   "popl %edi\n\t"
                   __ASM_CFI(".cfi_same_value %edi\n\t")
                   "popl %ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                   __ASM_CFI(".cfi_same_value %ebp\n\t")
                   "ret\n\t" )

/***********************************************************************
 *     CBClientThunkSLEx                    (KERNEL.621)
 */
void WINAPI CBClientThunkSLEx( CONTEXT *context )
{
    /* Call 32-bit relay code */

    LPWORD args = MapSL( MAKESEGPTR( context->SegSs, LOWORD(context->Ebp) ) );
    FARPROC proc = CBClientRelay32[ args[2] ][ args[1] ];
    INT nArgs;
    LPWORD stackLin;

    /* fill temporary area for the asm code (see comments in winebuild) */
    SEGPTR stack = stack16_push( 24 );
    stackLin = MapSL(stack);
    stackLin[0] = OFFSETOF(stack) + 4;
    stackLin[1] = SELECTOROF(stack);
    stackLin[2] = get_ds();
    stackLin[5] = OFFSETOF(stack) + 24;
    /* stackLin[6] and stackLin[7] reserved for the 32-bit stack ptr */
    stackLin[8] = get_ds();
    stackLin[9] = 0;
    stackLin[10] = 0;
    stackLin[11] = cbclientex_selector;

    context->Eax = CALL32_CBClientEx( proc, args, stackLin, &context->Esi, &nArgs );
    stack16_pop( 24 );

    /* Restore registers saved by CBClientGlueSL */
    stackLin = (LPWORD)((LPBYTE)CURRENT_STACK16 + sizeof(STACK16FRAME) - 4);
    context->Ebp = (context->Ebp & ~0xffff) | stackLin[3];
    context->Esi = (context->Esi & ~0xffff) | stackLin[2];
    context->Edi = (context->Edi & ~0xffff) | stackLin[1];
    context->SegDs = stackLin[0];
    context->Esp += 16+nArgs;

    /* Return to caller of CBClient thunklet */
    context->SegCs = stackLin[9];
    context->Eip   = stackLin[8];
}


/***********************************************************************
 *           Get16DLLAddress       (KERNEL32.@)
 *
 * This function is used by a Win32s DLL if it wants to call a Win16 function.
 * A 16:16 segmented pointer to the function is returned.
 * Written without any docu.
 */
SEGPTR WINAPI Get16DLLAddress(HMODULE16 handle, LPSTR func_name)
{
    static WORD code_sel32;
    FARPROC16 proc_16;
    LPBYTE thunk;

    if (!code_sel32)
    {
        if (!ThunkletHeap) THUNK_Init();
        code_sel32 = SELECTOR_AllocBlock( ThunkletHeap, 0x10000, LDT_FLAGS_CODE | LDT_FLAGS_32BIT );
        if (!code_sel32) return 0;
    }
    if (!(thunk = HeapAlloc( ThunkletHeap, 0, 32 ))) return 0;

    if (!handle) handle = GetModuleHandle16("WIN32S16");
    proc_16 = GetProcAddress16(handle, func_name);

    /* movl proc_16, $edx */
    *thunk++ = 0xba;
    *(FARPROC16 *)thunk = proc_16;
    thunk += sizeof(FARPROC16);

     /* jmpl QT_Thunk */
    *thunk++ = 0xea;
    *(void **)thunk = QT_Thunk;
    thunk += sizeof(FARPROC16);
    *(WORD *)thunk = get_cs();

    return MAKESEGPTR( code_sel32, (char *)thunk - (char *)ThunkletHeap );
}


/***********************************************************************
 *		GetWin16DOSEnv			(KERNEL32.34)
 * Returns some internal value.... probably the default environment database?
 */
DWORD WINAPI GetWin16DOSEnv(void)
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
int WINAPI __regs_CommonUnimpStub( const char *name, int type )
{
    FIXME("generic stub %s\n", debugstr_a(name));

    switch (type)
    {
    case 15:  return -1;
    case 14:  return ERROR_CALL_NOT_IMPLEMENTED;
    case 13:  return ERROR_NOT_SUPPORTED;
    case 1:   return 1;
    default:  return 0;
    }
}
__ASM_STDCALL_FUNC( CommonUnimpStub, 0,
                    "pushl %ecx\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "shrl $4,%ecx\n\t"
                    "andl $0xf,%ecx\n\t"
                    "pushl %ecx\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "pushl %eax\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                    "call " __ASM_STDCALL("__regs_CommonUnimpStub",8) "\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset -8\n\t")
                    "popl %ecx\n\t"
                    __ASM_CFI(".cfi_adjust_cfa_offset -4\n\t")
                    "andl $0xf,%ecx\n\t"
                    "movl (%esp),%edx\n\t"
                    "leal (%esp,%ecx,4),%esp\n\t"
                    "movl %edx,(%esp)\n\t"
                    "ret" )

/**********************************************************************
 *           HouseCleanLogicallyDeadHandles    (KERNEL32.33)
 */
void WINAPI HouseCleanLogicallyDeadHandles(void)
{
    /* Whatever this is supposed to do, our handles probably
       don't need it :-) */
}

/**********************************************************************
 *		@ (KERNEL32.100)
 */
BOOL WINAPI _KERNEL32_100(HANDLE threadid,DWORD exitcode,DWORD x)
{
	FIXME("(%p,%ld,0x%08lx): stub\n",threadid,exitcode,x);
	return TRUE;
}

/**********************************************************************
 *		@ (KERNEL32.99)
 *
 * Checks whether the clock has to be switched from daylight
 * savings time to standard time or vice versa.
 *
 */
DWORD WINAPI _KERNEL32_99(DWORD x)
{
	FIXME("(0x%08lx): stub\n",x);
	return 1;
}


/***********************************************************************
 * Helper for k32 family functions
 */
static void *user32_proc_address(const char *proc_name)
{
    static HMODULE hUser32;

    if(!hUser32) hUser32 = LoadLibraryA("user32.dll");
    return GetProcAddress(hUser32, proc_name);
}

/***********************************************************************
 *		k32CharToOemBuffA   (KERNEL32.11)
 */
BOOL WINAPI k32CharToOemBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR *bufW;

    if ((bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_ACP, 0, s, len, bufW, len );
        WideCharToMultiByte( CP_OEMCP, 0, bufW, len, d, len, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}

/***********************************************************************
 *		k32CharToOemA   (KERNEL32.10)
 */
BOOL WINAPI k32CharToOemA(LPCSTR s, LPSTR d)
{
    if (!s || !d) return TRUE;
    return k32CharToOemBuffA( s, d, strlen(s) + 1 );
}

/***********************************************************************
 *		k32OemToCharBuffA   (KERNEL32.13)
 */
BOOL WINAPI k32OemToCharBuffA(LPCSTR s, LPSTR d, DWORD len)
{
    WCHAR *bufW;

    if ((bufW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) )))
    {
        MultiByteToWideChar( CP_OEMCP, 0, s, len, bufW, len );
        WideCharToMultiByte( CP_ACP, 0, bufW, len, d, len, NULL, NULL );
        HeapFree( GetProcessHeap(), 0, bufW );
    }
    return TRUE;
}

/***********************************************************************
 *		k32OemToCharA   (KERNEL32.12)
 */
BOOL WINAPI k32OemToCharA(LPCSTR s, LPSTR d)
{
    return k32OemToCharBuffA( s, d, strlen(s) + 1 );
}

/**********************************************************************
 *		k32LoadStringA   (KERNEL32.14)
 */
INT WINAPI k32LoadStringA(HINSTANCE instance, UINT resource_id,
                          LPSTR buffer, INT buflen)
{
    static INT (WINAPI *pLoadStringA)(HINSTANCE, UINT, LPSTR, INT);

    if(!pLoadStringA) pLoadStringA = user32_proc_address("LoadStringA");
    return pLoadStringA(instance, resource_id, buffer, buflen);
}

/***********************************************************************
 *		k32wvsprintfA   (KERNEL32.16)
 */
INT WINAPI k32wvsprintfA(LPSTR buffer, LPCSTR spec, va_list args)
{
    static INT (WINAPI *pwvsprintfA)(LPSTR, LPCSTR, va_list);

    if(!pwvsprintfA) pwvsprintfA = user32_proc_address("wvsprintfA");
    return (*pwvsprintfA)(buffer, spec, args);
}

/***********************************************************************
 *		k32wsprintfA   (KERNEL32.15)
 */
INT WINAPIV k32wsprintfA(LPSTR buffer, LPCSTR spec, ...)
{
    va_list args;
    INT res;

    va_start(args, spec);
    res = k32wvsprintfA(buffer, spec, args);
    va_end(args);
    return res;
}

/**********************************************************************
 *	     Catch    (KERNEL.55)
 *
 * Real prototype is:
 *   INT16 WINAPI Catch( LPCATCHBUF lpbuf );
 */
void WINAPI Catch16( LPCATCHBUF lpbuf, CONTEXT *context )
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

    lpbuf[0] = LOWORD(context->Eip);
    lpbuf[1] = context->SegCs;
    /* Windows pushes 4 more words before saving sp */
    lpbuf[2] = LOWORD(context->Esp) - 4 * sizeof(WORD);
    lpbuf[3] = LOWORD(context->Ebp);
    lpbuf[4] = LOWORD(context->Esi);
    lpbuf[5] = LOWORD(context->Edi);
    lpbuf[6] = context->SegDs;
    lpbuf[7] = 0;
    lpbuf[8] = context->SegSs;
    context->Eax &= ~0xffff;  /* Return 0 */
}


/**********************************************************************
 *	     Throw    (KERNEL.56)
 *
 * Real prototype is:
 *   INT16 WINAPI Throw( LPCATCHBUF lpbuf, INT16 retval );
 */
void WINAPI Throw16( LPCATCHBUF lpbuf, INT16 retval, CONTEXT *context )
{
    STACK16FRAME *pFrame;
    STACK32FRAME *frame32;

    context->Eax = (context->Eax & ~0xffff) | (WORD)retval;

    /* Find the frame32 corresponding to the frame16 we are jumping to */
    pFrame = CURRENT_STACK16;
    frame32 = pFrame->frame32;
    while (frame32 && frame32->frame16)
    {
        if (OFFSETOF(frame32->frame16) < CURRENT_SP)
            break;  /* Something strange is going on */
        if (OFFSETOF(frame32->frame16) > lpbuf[2])
        {
            /* We found the right frame */
            pFrame->frame32 = frame32;
            break;
        }
        frame32 = ((STACK16FRAME *)MapSL(frame32->frame16))->frame32;
    }
    RtlUnwind( &pFrame->frame32->frame, NULL, NULL, 0 );

    context->Eip = lpbuf[0];
    context->SegCs  = lpbuf[1];
    context->Esp = lpbuf[2] + 4 * sizeof(WORD) - sizeof(WORD) /*extra arg*/;
    context->Ebp = lpbuf[3];
    context->Esi = lpbuf[4];
    context->Edi = lpbuf[5];
    context->SegDs  = lpbuf[6];

    if (lpbuf[8] != context->SegSs)
        ERR("Switching stack segment with Throw() not supported; expect crash now\n" );
}


/*
 *  16-bit WOW routines (in KERNEL)
 */

/**********************************************************************
 *           GetVDMPointer32W      (KERNEL.516)
 */
DWORD WINAPI GetVDMPointer32W16( SEGPTR vp, UINT16 fMode )
{
    GlobalPageLock16(GlobalHandle16(SELECTOROF(vp)));
    return (DWORD)K32WOWGetVDMPointer( vp, 0, (DWORD)fMode );
}

/***********************************************************************
 *           LoadLibraryEx32W      (KERNEL.513)
 */
DWORD WINAPI LoadLibraryEx32W16( LPCSTR lpszLibFile, DWORD hFile, DWORD dwFlags )
{
    HMODULE hModule;
    DWORD mutex_count;
    OFSTRUCT ofs;
    const char *p;

    if (!lpszLibFile)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* if the file cannot be found, call LoadLibraryExA anyway, since it might be
       a builtin module. This case is handled in MODULE_LoadLibraryExA */

    if ((p = strrchr( lpszLibFile, '.' )) && !strchr( p, '\\' ))  /* got an extension */
    {
        if (OpenFile16( lpszLibFile, &ofs, OF_EXIST ) != HFILE_ERROR16)
            lpszLibFile = ofs.szPathName;
    }
    else
    {
        char buffer[MAX_PATH+4];
        strcpy( buffer, lpszLibFile );
        strcat( buffer, ".dll" );
        if (OpenFile16( buffer, &ofs, OF_EXIST ) != HFILE_ERROR16)
            lpszLibFile = ofs.szPathName;
    }

    ReleaseThunkLock( &mutex_count );
    hModule = LoadLibraryExA( lpszLibFile, (HANDLE)hFile, dwFlags );
    RestoreThunkLock( mutex_count );

    return (DWORD)hModule;
}

/***********************************************************************
 *           GetProcAddress32W     (KERNEL.515)
 */
DWORD WINAPI GetProcAddress32W16( DWORD hModule, LPCSTR lpszProc )
{
    return (DWORD)GetProcAddress( (HMODULE)hModule, lpszProc );
}

/***********************************************************************
 *           FreeLibrary32W        (KERNEL.514)
 */
DWORD WINAPI FreeLibrary32W16( DWORD hLibModule )
{
    BOOL retv;
    DWORD mutex_count;

    ReleaseThunkLock( &mutex_count );
    retv = FreeLibrary( (HMODULE)hLibModule );
    RestoreThunkLock( mutex_count );
    return (DWORD)retv;
}

/**********************************************************************
 *           WOW_CallProc32W
 */
static DWORD WOW_CallProc32W16( FARPROC proc32, DWORD nrofargs, DWORD *args )
{
    DWORD ret;
    DWORD mutex_count;

    ReleaseThunkLock( &mutex_count );
    if (!proc32) ret = 0;
    else ret = call_entry_point( proc32, nrofargs & ~CPEX_DEST_CDECL, args );
    RestoreThunkLock( mutex_count );

    TRACE("returns %08lx\n",ret);
    return ret;
}

/**********************************************************************
 *           CallProc32W           (KERNEL.517)
 */
DWORD WINAPIV CallProc32W16( DWORD nrofargs, DWORD argconvmask, FARPROC proc32, VA_LIST16 valist )
{
    DWORD args[32];
    unsigned int i;

    TRACE("(%ld,%ld,%p args[",nrofargs,argconvmask,proc32);

    for (i=0;i<nrofargs;i++)
    {
        if (argconvmask & (1<<i))
        {
            SEGPTR ptr = VA_ARG16( valist, SEGPTR );
            /* pascal convention, have to reverse the arguments order */
            args[nrofargs - i - 1] = (DWORD)MapSL(ptr);
            TRACE("%08lx(%p),",ptr,MapSL(ptr));
        }
        else
        {
            DWORD arg = VA_ARG16( valist, DWORD );
            /* pascal convention, have to reverse the arguments order */
            args[nrofargs - i - 1] = arg;
            TRACE("%ld,", arg);
        }
    }
    TRACE("])\n");

    /* POP nrofargs DWORD arguments and 3 DWORD parameters */
    stack16_pop( (3 + nrofargs) * sizeof(DWORD) );

    return WOW_CallProc32W16( proc32, nrofargs, args );
}

/**********************************************************************
 *           _CallProcEx32W         (KERNEL.518)
 */
DWORD WINAPIV CallProcEx32W16( DWORD nrofargs, DWORD argconvmask, FARPROC proc32, VA_LIST16 valist )
{
    DWORD args[32];
    unsigned int i, count = min( 32, nrofargs & ~CPEX_DEST_CDECL );

    TRACE("(%s,%ld,%ld,%p args[", nrofargs & CPEX_DEST_CDECL ? "cdecl": "stdcall",
          nrofargs & ~CPEX_DEST_CDECL, argconvmask, proc32);

    for (i = 0; i < count; i++)
    {
        if (argconvmask & (1<<i))
        {
            SEGPTR ptr = VA_ARG16( valist, SEGPTR );
            args[i] = (DWORD)MapSL(ptr);
            TRACE("%08lx(%p),",ptr,MapSL(ptr));
        }
        else
        {
            DWORD arg = VA_ARG16( valist, DWORD );
            args[i] = arg;
            TRACE("%ld,", arg);
        }
    }
    TRACE("])\n");
    return WOW_CallProc32W16( proc32, nrofargs, args );
}


/**********************************************************************
 *           WOW16Call               (KERNEL.500)
 *
 * FIXME!!!
 *
 */
DWORD WINAPIV WOW16Call(WORD x, WORD y, WORD z, VA_LIST16 args)
{
        int     i;
        DWORD   calladdr;
        FIXME("(0x%04x,0x%04x,%d),calling (",x,y,z);

        for (i=0;i<x/2;i++) {
                WORD    a = VA_ARG16(args,WORD);
                FIXME("%04x ",a);
        }
        calladdr = VA_ARG16(args,DWORD);
        stack16_pop( 3*sizeof(WORD) + x + sizeof(DWORD) );
        FIXME(") calling address was 0x%08lx\n",calladdr);
        return 0;
}
