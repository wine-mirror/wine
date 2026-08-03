/*
 * Win32 WOW Generic Thunk API
 *
 * Copyright 1999 Ulrich Weigand
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

#include <stdarg.h>
#include <errno.h>

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wownt32.h"
#include "excpt.h"
#include "winternl.h"
#include "ntgdi.h"
#include "kernel16_private.h"
#include "wine/asm.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);

/* symbols exported from relay16.s */
extern DWORD WINAPI wine_call_to_16( FARPROC16 target, DWORD cbArgs, PEXCEPTION_HANDLER handler );
extern void WINAPI wine_call_to_16_regs( CONTEXT *context, DWORD cbArgs, PEXCEPTION_HANDLER handler );
extern void __wine_call_to_16_ret(void);
extern BYTE __wine_call16_start[];
extern BYTE __wine_call16_end[];

static SEGPTR call16_ret_addr;  /* segptr to __wine_call_to_16_ret routine */

extern const BYTE cbclient_ret[], cbclient_ret_end[];
__ASM_GLOBAL_FUNC( cbclient_ret,
                   "movzwl %sp,%ebx\n\t"
                   "lssl %ss:-16(%ebx),%esp\n\t"
                   "lretl\n\t"
                   ".globl " __ASM_NAME("cbclient_ret_end") "\n"
                   __ASM_NAME("cbclient_ret_end") ":" )

extern const BYTE cbclientex_ret[], cbclientex_ret_end[];
__ASM_GLOBAL_FUNC( cbclientex_ret,
                   "movzwl %bp,%ebx\n\t"
                   "subw %bp,%sp\n\t"
                   "movzwl %sp,%ebp\n\t"
                   "lssl %ss:-12(%ebx),%esp\n\t"
                   "lretl\n\t"
                   ".globl " __ASM_NAME("cbclientex_ret_end") "\n"
                   __ASM_NAME("cbclientex_ret_end") ":" )

/***********************************************************************
 *           WOWTHUNK_Init
 */
BOOL WOWTHUNK_Init(void)
{
    /* allocate the code selector for CallTo16 routines */
    WORD codesel = SELECTOR_AllocBlock( __wine_call16_start,
                                        (BYTE *)(&CallTo16_TebSelector + 1) - __wine_call16_start,
                                        LDT_FLAGS_CODE | LDT_FLAGS_32BIT );

    cbclient_selector = SELECTOR_AllocBlock( cbclient_ret, cbclient_ret_end - cbclient_ret,
                                             LDT_FLAGS_CODE | LDT_FLAGS_32BIT );
    cbclientex_selector = SELECTOR_AllocBlock( cbclientex_ret, cbclientex_ret_end - cbclientex_ret,
                                               LDT_FLAGS_CODE | LDT_FLAGS_32BIT );
    if (!codesel || !cbclient_selector || !cbclientex_selector)
        return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CallTo16_DataSelector = get_ds();
    call16_ret_addr = MAKESEGPTR( codesel, (BYTE *)__wine_call_to_16_ret - __wine_call16_start );

    if (TRACE_ON(relay) || TRACE_ON(snoop)) RELAY16_InitDebugLists();

    return TRUE;
}


/*************************************************************
 *            fix_selector
 *
 * Fix a selector load that caused an exception if it's in the
 * 16-bit relay code.
 */
static BOOL fix_selector( CONTEXT *context )
{
    WORD *stack;
    BYTE *instr = (BYTE *)context->Eip;

    if (instr < __wine_call16_start || instr >= __wine_call16_end) return FALSE;

    /* skip prefixes */
    while (*instr == 0x66 || *instr == 0x67) instr++;

    switch(instr[0])
    {
    case 0x07: /* pop es */
    case 0x1f: /* pop ds */
        break;
    case 0x0f: /* extended instruction */
        switch(instr[1])
        {
        case 0xa1: /* pop fs */
        case 0xa9: /* pop gs */
            break;
        default:
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
    stack = ldt_get_ptr( context->SegSs, context->Esp );
    TRACE( "fixing up selector %x for pop instruction\n", *stack );
    *stack = 0;
    return TRUE;
}


/*************************************************************
 *            call16_handler
 *
 * Handler for exceptions occurring in 16-bit code.
 */
static DWORD call16_handler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                             CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EXCEPTION_UNWINDING | EXCEPTION_EXIT_UNWIND))
    {
        /* unwinding: restore the stack pointer in the TEB, and leave the Win16 mutex */
        STACK32FRAME *frame32 = CONTAINING_RECORD(frame, STACK32FRAME, frame);
        kernel_get_thread_data()->stack = frame32->frame16;
        _LeaveWin16Lock();
    }
    else if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
             record->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)
    {
        if (ldt_is_system(context->SegCs))
        {
            if (fix_selector( context )) return ExceptionContinueExecution;
        }
        else
        {
            SEGPTR gpHandler;
            DWORD ret = __wine_emulate_instruction( record, context );

            if (ret != ExceptionContinueSearch) return ret;

            /* check for Win16 __GP handler */
            if ((gpHandler = HasGPHandler16( MAKESEGPTR( context->SegCs, context->Eip ) )))
            {
                WORD *stack = ldt_get_ptr( context->SegSs, context->Esp );
                *--stack = context->SegCs;
                *--stack = context->Eip;

                if (!IS_SELECTOR_32BIT(context->SegSs))
                    context->Esp = MAKELONG( LOWORD(context->Esp - 2*sizeof(WORD)),
                                             HIWORD(context->Esp) );
                else
                    context->Esp -= 2*sizeof(WORD);

                context->SegCs = SELECTOROF( gpHandler );
                context->Eip   = OFFSETOF( gpHandler );
                return ExceptionContinueExecution;
            }
        }
    }
    return ExceptionContinueSearch;
}


/*
 *  32-bit WOW routines (in WOW32, but actually forwarded to KERNEL32)
 */

/**********************************************************************
 *           K32WOWGetDescriptor        (KERNEL32.70)
 */
BOOL WINAPI K32WOWGetDescriptor( SEGPTR segptr, LPLDT_ENTRY ldtent )
{
    return GetThreadSelectorEntry( GetCurrentThread(),
                                   segptr >> 16, ldtent );
}

/**********************************************************************
 *           K32WOWGetVDMPointer        (KERNEL32.56)
 */
LPVOID WINAPI K32WOWGetVDMPointer( DWORD vp, DWORD dwBytes, BOOL fProtectedMode )
{
    /* FIXME: add size check too */

    if ( fProtectedMode )
        return MapSL( vp );
    else
        return DOSMEM_MapRealToLinear( vp );
}

/**********************************************************************
 *           K32WOWGetVDMPointerFix     (KERNEL32.68)
 */
LPVOID WINAPI K32WOWGetVDMPointerFix( DWORD vp, DWORD dwBytes, BOOL fProtectedMode )
{
    /*
     * Hmmm. According to the docu, we should call:
     *
     *          GlobalFix16( SELECTOROF(vp) );
     *
     * But this is unnecessary under Wine, as we never move global
     * memory segments in linear memory anyway.
     *
     * (I'm not so sure what we are *supposed* to do if
     *  fProtectedMode is TRUE, anyway ...)
     */

    return K32WOWGetVDMPointer( vp, dwBytes, fProtectedMode );
}

/**********************************************************************
 *           K32WOWGetVDMPointerUnfix   (KERNEL32.69)
 */
VOID WINAPI K32WOWGetVDMPointerUnfix( DWORD vp )
{
    /*
     * See above why we don't call:
     *
     * GlobalUnfix16( SELECTOROF(vp) );
     *
     */
}

/**********************************************************************
 *           K32WOWGlobalAlloc16        (KERNEL32.59)
 */
WORD WINAPI K32WOWGlobalAlloc16( WORD wFlags, DWORD cb )
{
    return (WORD)GlobalAlloc16( wFlags, cb );
}

/**********************************************************************
 *           K32WOWGlobalFree16         (KERNEL32.62)
 */
WORD WINAPI K32WOWGlobalFree16( WORD hMem )
{
    return (WORD)GlobalFree16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           K32WOWGlobalUnlock16       (KERNEL32.61)
 */
BOOL WINAPI K32WOWGlobalUnlock16( WORD hMem )
{
    return (BOOL)GlobalUnlock16( (HGLOBAL16)hMem );
}

/**********************************************************************
 *           K32WOWGlobalAllocLock16    (KERNEL32.63)
 */
DWORD WINAPI K32WOWGlobalAllocLock16( WORD wFlags, DWORD cb, WORD *phMem )
{
    WORD hMem = K32WOWGlobalAlloc16( wFlags, cb );
    if (phMem) *phMem = hMem;

    return K32WOWGlobalLock16( hMem );
}

/**********************************************************************
 *           K32WOWGlobalLockSize16     (KERNEL32.65)
 */
DWORD WINAPI K32WOWGlobalLockSize16( WORD hMem, PDWORD pcb )
{
    if ( pcb )
        *pcb = GlobalSize16( (HGLOBAL16)hMem );

    return K32WOWGlobalLock16( hMem );
}

/**********************************************************************
 *           K32WOWGlobalUnlockFree16   (KERNEL32.64)
 */
WORD WINAPI K32WOWGlobalUnlockFree16( DWORD vpMem )
{
    if ( !K32WOWGlobalUnlock16( HIWORD(vpMem) ) )
        return FALSE;

    return K32WOWGlobalFree16( HIWORD(vpMem) );
}


/**********************************************************************
 *           K32WOWYield16              (KERNEL32.66)
 */
VOID WINAPI K32WOWYield16( void )
{
    /*
     * This does the right thing for both Win16 and Win32 tasks.
     * More or less, at least :-/
     */
    Yield16();
}

/**********************************************************************
 *           K32WOWDirectedYield16       (KERNEL32.67)
 */
VOID WINAPI K32WOWDirectedYield16( WORD htask16 )
{
    /*
     * Argh.  Our scheduler doesn't like DirectedYield by Win32
     * tasks at all.  So we do hope that this routine is indeed
     * only ever called by Win16 tasks that have thunked up ...
     */
    DirectedYield16( (HTASK16)htask16 );
}

static HANDLE gdi_handle32( WORD handle )
{
    static GDI_SHARED_MEMORY *gdi_shared;

    if (!gdi_shared)
    {
        if (NtCurrentTeb()->GdiBatchCount)
        {
            TEB64 *teb64 = (TEB64 *)(UINT_PTR)NtCurrentTeb()->GdiBatchCount;
            PEB64 *peb64 = (PEB64 *)(UINT_PTR)teb64->Peb;
            gdi_shared = (GDI_SHARED_MEMORY *)(UINT_PTR)peb64->GdiSharedHandleTable;
        }
        else gdi_shared = (GDI_SHARED_MEMORY *)NtCurrentTeb()->Peb->GdiSharedHandleTable;
        if (!gdi_shared) return ULongToHandle( handle );
    }

    return ULongToHandle( (gdi_shared->Handles[handle].Unique << 16) | handle );
}

/***********************************************************************
 *           K32WOWHandle32              (KERNEL32.57)
 */
HANDLE WINAPI K32WOWHandle32( WORD handle, WOW_HANDLE_TYPE type )
{
    switch ( type )
    {
    case WOW_TYPE_HWND:
    case WOW_TYPE_HMENU:
    case WOW_TYPE_HDWP:
    case WOW_TYPE_HDROP:
    case WOW_TYPE_HACCEL:
        return (HANDLE)(ULONG_PTR)handle;

    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HMETAFILE:
        return gdi_handle32( handle );

    case WOW_TYPE_HTASK:
        return ((TDB *)GlobalLock16(handle))->teb->ClientId.UniqueThread;

    case WOW_TYPE_FULLHWND:
        FIXME( "conversion of full window handles not supported yet\n" );
        return (HANDLE)(ULONG_PTR)handle;

    default:
        ERR( "handle 0x%04x of unknown type %d\n", handle, type );
        return (HANDLE)(ULONG_PTR)handle;
    }
}

/***********************************************************************
 *           K32WOWHandle16              (KERNEL32.58)
 */
WORD WINAPI K32WOWHandle16( HANDLE handle, WOW_HANDLE_TYPE type )
{
    switch ( type )
    {
    case WOW_TYPE_HWND:
    case WOW_TYPE_HMENU:
    case WOW_TYPE_HDWP:
    case WOW_TYPE_HDROP:
    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HACCEL:
    case WOW_TYPE_FULLHWND:
    	if ( HIWORD(handle ) )
        	ERR( "handle %p of type %d has non-zero HIWORD\n", handle, type );
        return LOWORD(handle);

    case WOW_TYPE_HMETAFILE:
        FIXME( "conversion of metafile handles not supported yet\n" );
        return LOWORD(handle);

    case WOW_TYPE_HTASK:
        return TASK_GetTaskFromThread( (DWORD)handle );

    default:
        ERR( "handle %p of unknown type %d\n", handle, type );
        return LOWORD(handle);
    }
}

/**********************************************************************
 *           K32WOWCallback16Ex         (KERNEL32.55)
 */
BOOL WINAPI K32WOWCallback16Ex( DWORD vpfn16, DWORD dwFlags,
                                DWORD cbArgs, LPVOID pArgs, LPDWORD pdwRetCode )
{
    /*
     * Arguments must be prepared in the correct order by the caller
     * (both for PASCAL and CDECL calling convention), so we simply
     * copy them to the 16-bit stack ...
     */
    char *stack = (char *)CURRENT_STACK16 - cbArgs;

    memcpy( stack, pArgs, cbArgs );

    if (dwFlags & WCB16_REGS)
    {
        CONTEXT *context = (CONTEXT *)pdwRetCode;

        if (TRACE_ON(relay))
        {
            DWORD count = cbArgs / sizeof(WORD);
            WORD * wstack = (WORD *)stack;

            TRACE_(relay)( "\1CallTo16(func=%04lx:%04x", context->SegCs, LOWORD(context->Eip) );
            while (count) TRACE_(relay)( ",%04x", wstack[--count] );
            TRACE_(relay)( ") ss:sp=%04x:%04x ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x bp=%04x ds=%04x es=%04x\n",
                           CURRENT_SS, CURRENT_SP,
                           (WORD)context->Eax, (WORD)context->Ebx, (WORD)context->Ecx,
                           (WORD)context->Edx, (WORD)context->Esi, (WORD)context->Edi,
                           (WORD)context->Ebp, (WORD)context->SegDs, (WORD)context->SegEs );
            SYSLEVEL_CheckNotLevel( 2 );
        }

        /* push return address */
        stack -= sizeof(SEGPTR);
        *((SEGPTR *)stack) = call16_ret_addr;
        cbArgs += sizeof(SEGPTR);

        if (!(dwFlags & WCB16_INTERRUPT)) _EnterWin16Lock();
        wine_call_to_16_regs( context, cbArgs, call16_handler );
        if (!(dwFlags & WCB16_INTERRUPT)) _LeaveWin16Lock();

        if (TRACE_ON(relay))
        {
            TRACE_(relay)( "\1RetFrom16() ss:sp=%04x:%04x ax=%04x bx=%04x cx=%04x dx=%04x bp=%04x sp=%04x\n",
                           CURRENT_SS, CURRENT_SP,
                           (WORD)context->Eax, (WORD)context->Ebx, (WORD)context->Ecx,
                           (WORD)context->Edx, (WORD)context->Ebp, (WORD)context->Esp );
            SYSLEVEL_CheckNotLevel( 2 );
        }
    }
    else
    {
        DWORD ret;

        if (TRACE_ON(relay))
        {
            DWORD count = cbArgs / sizeof(WORD);
            WORD * wstack = (WORD *)stack;

            TRACE_(relay)( "\1CallTo16(func=%04x:%04x,ds=%04x",
                           HIWORD(vpfn16), LOWORD(vpfn16), CURRENT_SS );
            while (count) TRACE_(relay)( ",%04x", wstack[--count] );
            TRACE_(relay)( ") ss:sp=%04x:%04x\n", CURRENT_SS, CURRENT_SP );
            SYSLEVEL_CheckNotLevel( 2 );
        }

        /* push return address */
        stack -= sizeof(SEGPTR);
        *((SEGPTR *)stack) = call16_ret_addr;
        cbArgs += sizeof(SEGPTR);

        /*
         * Actually, we should take care whether the called routine cleans up
         * its stack or not.  Fortunately, our wine_call_to_16 core doesn't rely on
         * the callee to do so; after the routine has returned, the 16-bit
         * stack pointer is always reset to the position it had before.
         */
        if (!(dwFlags & WCB16_INTERRUPT)) _EnterWin16Lock();
        ret = wine_call_to_16( (FARPROC16)vpfn16, cbArgs, call16_handler );
        if (pdwRetCode) *pdwRetCode = ret;
        if (!(dwFlags & WCB16_INTERRUPT)) _LeaveWin16Lock();

        if (TRACE_ON(relay))
        {
            TRACE_(relay)( "\1RetFrom16() ss:sp=%04x:%04x retval=%08lx\n", CURRENT_SS, CURRENT_SP, ret );
            SYSLEVEL_CheckNotLevel( 2 );
        }
    }

    return TRUE;  /* success */
}

/**********************************************************************
 *           K32WOWCallback16            (KERNEL32.54)
 */
DWORD WINAPI K32WOWCallback16( DWORD vpfn16, DWORD dwParam )
{
    DWORD ret;

    if ( !K32WOWCallback16Ex( vpfn16, WCB16_PASCAL,
                           sizeof(DWORD), &dwParam, &ret ) )
        ret = 0L;

    return ret;
}
