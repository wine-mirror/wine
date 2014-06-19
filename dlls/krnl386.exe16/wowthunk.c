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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <errno.h>

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wownt32.h"
#include "excpt.h"
#include "winternl.h"
#include "kernel16_private.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);

/* symbols exported from relay16.s */
extern DWORD WINAPI wine_call_to_16( FARPROC16 target, DWORD cbArgs, PEXCEPTION_HANDLER handler );
extern void WINAPI wine_call_to_16_regs( CONTEXT *context, DWORD cbArgs, PEXCEPTION_HANDLER handler );
extern void __wine_call_to_16_ret(void);
extern void CALL32_CBClient_Ret(void);
extern void CALL32_CBClientEx_Ret(void);
extern void DPMI_PendingEventCheck(void);
extern void DPMI_PendingEventCheck_Cleanup(void);
extern void DPMI_PendingEventCheck_Return(void);
extern BYTE __wine_call16_start[];
extern BYTE __wine_call16_end[];

extern void RELAY16_InitDebugLists(void);

static SEGPTR call16_ret_addr;  /* segptr to __wine_call_to_16_ret routine */

static WORD  dpmi_checker_selector;
static DWORD dpmi_checker_offset_call;
static DWORD dpmi_checker_offset_cleanup;
static DWORD dpmi_checker_offset_return;

/***********************************************************************
 *           WOWTHUNK_Init
 */
BOOL WOWTHUNK_Init(void)
{
    /* allocate the code selector for CallTo16 routines */
    LDT_ENTRY entry;
    WORD codesel = wine_ldt_alloc_entries(1);

    if (!codesel) return FALSE;
    wine_ldt_set_base( &entry, __wine_call16_start );
    wine_ldt_set_limit( &entry, (BYTE *)(&CallTo16_TebSelector + 1) - __wine_call16_start - 1 );
    wine_ldt_set_flags( &entry, WINE_LDT_FLAGS_CODE | WINE_LDT_FLAGS_32BIT );
    wine_ldt_set_entry( codesel, &entry );

      /* Patch the return addresses for CallTo16 routines */

    CallTo16_DataSelector = wine_get_ds();
    call16_ret_addr = MAKESEGPTR( codesel, (BYTE *)__wine_call_to_16_ret - __wine_call16_start );
    CALL32_CBClient_RetAddr =
        MAKESEGPTR( codesel, (BYTE *)CALL32_CBClient_Ret - __wine_call16_start );
    CALL32_CBClientEx_RetAddr =
        MAKESEGPTR( codesel, (BYTE *)CALL32_CBClientEx_Ret - __wine_call16_start );

    /* Prepare selector and offsets for DPMI event checking. */
    dpmi_checker_selector = codesel;
    dpmi_checker_offset_call = (BYTE *)DPMI_PendingEventCheck - __wine_call16_start;
    dpmi_checker_offset_cleanup = (BYTE *)DPMI_PendingEventCheck_Cleanup - __wine_call16_start;
    dpmi_checker_offset_return = (BYTE *)DPMI_PendingEventCheck_Return - __wine_call16_start;

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
    case 0x17: /* pop ss */
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
    stack = wine_ldt_get_ptr( context->SegSs, context->Esp );
    TRACE( "fixing up selector %x for pop instruction\n", *stack );
    *stack = 0;
    return TRUE;
}


/*************************************************************
 *            insert_event_check
 *
 * Make resuming the context check for pending DPMI events
 * before the original context is restored. This is required
 * because DPMI events are asynchronous, they are blocked while 
 * Wine 32-bit code is being executed and we want to prevent 
 * a race when returning back to 16-bit or 32-bit DPMI context.
 */
static void insert_event_check( CONTEXT *context )
{
    char *stack = wine_ldt_get_ptr( context->SegSs, context->Esp );

    /* don't do event check while in system code */
    if (wine_ldt_is_system(context->SegCs))
        return;

    if(context->SegCs == dpmi_checker_selector &&
       context->Eip   >= dpmi_checker_offset_call && 
       context->Eip   <= dpmi_checker_offset_cleanup)
    {
        /*
         * Nested call. Stack will be preserved. 
         */
    }
    else if(context->SegCs == dpmi_checker_selector &&
            context->Eip   == dpmi_checker_offset_return)
    {
        /*
         * Nested call. We have just finished popping the fs
         * register, lets put it back into stack.
         */

        stack -= sizeof(WORD);
        *(WORD*)stack = context->SegFs;

        context->Esp -= 2;
    }
    else
    {
        /*
         * Call is not nested.
         * Push modified registers into stack.
         * These will be popped by the assembler stub.
         */

        stack -= sizeof(DWORD);
        *(DWORD*)stack = context->EFlags;
   
        stack -= sizeof(DWORD);
        *(DWORD*)stack = context->SegCs;

        stack -= sizeof(DWORD);
        *(DWORD*)stack = context->Eip;

        stack -= sizeof(WORD);
        *(WORD*)stack = context->SegFs;

        context->Esp  -= 14;
    }

    /*
     * Modify the context so that we jump into assembler stub.
     * TEB access is made easier by providing the stub
     * with the correct fs register value.
     */

    context->SegCs = dpmi_checker_selector;
    context->Eip   = dpmi_checker_offset_call;
    context->SegFs = wine_get_fs();
}


/*************************************************************
 *            call16_handler
 *
 * Handler for exceptions occurring in 16-bit code.
 */
static DWORD call16_handler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                             CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
    {
        /* unwinding: restore the stack pointer in the TEB, and leave the Win16 mutex */
        STACK32FRAME *frame32 = CONTAINING_RECORD(frame, STACK32FRAME, frame);
        NtCurrentTeb()->WOW32Reserved = (void *)frame32->frame16;
        _LeaveWin16Lock();
    }
    else if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
             record->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)
    {
        if (wine_ldt_is_system(context->SegCs))
        {
            if (fix_selector( context )) return ExceptionContinueExecution;
        }
        else
        {
            SEGPTR gpHandler;
            DWORD ret = __wine_emulate_instruction( record, context );

            /*
             * Insert check for pending DPMI events. Note that this 
             * check must be inserted after instructions have been 
             * emulated because the instruction emulation requires
             * original CS:IP and the emulation may change TEB.dpmi_vif.
             */
            if(get_vm86_teb_info()->dpmi_vif)
                insert_event_check( context );

            if (ret != ExceptionContinueSearch) return ret;

            /* check for Win16 __GP handler */
            if ((gpHandler = HasGPHandler16( MAKESEGPTR( context->SegCs, context->Eip ) )))
            {
                WORD *stack = wine_ldt_get_ptr( context->SegSs, context->Esp );
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
    else if (record->ExceptionCode == EXCEPTION_VM86_STI)
    {
        insert_event_check( context );
    }
    return ExceptionContinueSearch;
}


/*************************************************************
 *            vm86_handler
 *
 * Handler for exceptions occurring in vm86 code.
 */
static DWORD vm86_handler( EXCEPTION_RECORD *record, EXCEPTION_REGISTRATION_RECORD *frame,
                           CONTEXT *context, EXCEPTION_REGISTRATION_RECORD **pdispatcher )
{
    if (record->ExceptionFlags & (EH_UNWINDING | EH_EXIT_UNWIND))
        return ExceptionContinueSearch;

    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
        record->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)
    {
        return __wine_emulate_instruction( record, context );
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
    case WOW_TYPE_HDC:
    case WOW_TYPE_HFONT:
    case WOW_TYPE_HRGN:
    case WOW_TYPE_HBITMAP:
    case WOW_TYPE_HBRUSH:
    case WOW_TYPE_HPALETTE:
    case WOW_TYPE_HPEN:
    case WOW_TYPE_HACCEL:
        return (HANDLE)(ULONG_PTR)handle;

    case WOW_TYPE_HMETAFILE:
        FIXME( "conversion of metafile handles not supported yet\n" );
        return (HANDLE)(ULONG_PTR)handle;

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

    if (dwFlags & (WCB16_REGS|WCB16_REGS_LONG))
    {
        CONTEXT *context = (CONTEXT *)pdwRetCode;

        if (TRACE_ON(relay))
        {
            DWORD count = cbArgs / sizeof(WORD);
            WORD * wstack = (WORD *)stack;

            DPRINTF("%04x:CallTo16(func=%04x:%04x,ds=%04x",
                    GetCurrentThreadId(),
                    context->SegCs, LOWORD(context->Eip), context->SegDs );
            while (count) DPRINTF( ",%04x", wstack[--count] );
            DPRINTF(") ss:sp=%04x:%04x",
                    SELECTOROF(NtCurrentTeb()->WOW32Reserved), OFFSETOF(NtCurrentTeb()->WOW32Reserved) );
            DPRINTF(" ax=%04x bx=%04x cx=%04x dx=%04x si=%04x di=%04x bp=%04x es=%04x fs=%04x\n",
                    (WORD)context->Eax, (WORD)context->Ebx, (WORD)context->Ecx,
                    (WORD)context->Edx, (WORD)context->Esi, (WORD)context->Edi,
                    (WORD)context->Ebp, (WORD)context->SegEs, (WORD)context->SegFs );
            SYSLEVEL_CheckNotLevel( 2 );
        }

        if (context->EFlags & 0x00020000)  /* v86 mode */
        {
            EXCEPTION_REGISTRATION_RECORD frame;
            frame.Handler = vm86_handler;
            errno = 0;
            __wine_push_frame( &frame );
            __wine_enter_vm86( context );
            __wine_pop_frame( &frame );
            if (errno != 0)  /* enter_vm86 will fail with ENOSYS on x64 kernels */
            {
                WARN("__wine_enter_vm86 failed (errno=%d)\n", errno);
                if (errno == ENOSYS)
                    SetLastError(ERROR_NOT_SUPPORTED);
                else
                    SetLastError(ERROR_GEN_FAILURE);
                return FALSE;
            }
        }
        else
        {
            /* push return address */
            if (dwFlags & WCB16_REGS_LONG)
            {
                stack -= sizeof(DWORD);
                *((DWORD *)stack) = HIWORD(call16_ret_addr);
                stack -= sizeof(DWORD);
                *((DWORD *)stack) = LOWORD(call16_ret_addr);
                cbArgs += 2 * sizeof(DWORD);
            }
            else
            {
                stack -= sizeof(SEGPTR);
                *((SEGPTR *)stack) = call16_ret_addr;
                cbArgs += sizeof(SEGPTR);
            }

            /*
             * Start call by checking for pending events.
             * Note that wine_call_to_16_regs overwrites context stack
             * pointer so we may modify it here without a problem.
             */
            if (get_vm86_teb_info()->dpmi_vif)
            {
                context->SegSs = wine_get_ds();
                context->Esp   = (DWORD)stack;
                insert_event_check( context );
                cbArgs += (DWORD)stack - context->Esp;
            }

            _EnterWin16Lock();
            wine_call_to_16_regs( context, cbArgs, call16_handler );
            _LeaveWin16Lock();
        }

        if (TRACE_ON(relay))
        {
            DPRINTF("%04x:RetFrom16() ss:sp=%04x:%04x ",
                    GetCurrentThreadId(), SELECTOROF(NtCurrentTeb()->WOW32Reserved),
                    OFFSETOF(NtCurrentTeb()->WOW32Reserved));
            DPRINTF(" ax=%04x bx=%04x cx=%04x dx=%04x bp=%04x sp=%04x\n",
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

            DPRINTF("%04x:CallTo16(func=%04x:%04x,ds=%04x",
                    GetCurrentThreadId(), HIWORD(vpfn16), LOWORD(vpfn16),
                    SELECTOROF(NtCurrentTeb()->WOW32Reserved) );
            while (count) DPRINTF( ",%04x", wstack[--count] );
            DPRINTF(") ss:sp=%04x:%04x\n",
                    SELECTOROF(NtCurrentTeb()->WOW32Reserved), OFFSETOF(NtCurrentTeb()->WOW32Reserved) );
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
        _EnterWin16Lock();
        ret = wine_call_to_16( (FARPROC16)vpfn16, cbArgs, call16_handler );
        if (pdwRetCode) *pdwRetCode = ret;
        _LeaveWin16Lock();

        if (TRACE_ON(relay))
        {
            DPRINTF("%04x:RetFrom16() ss:sp=%04x:%04x retval=%08x\n",
                    GetCurrentThreadId(), SELECTOROF(NtCurrentTeb()->WOW32Reserved),
                    OFFSETOF(NtCurrentTeb()->WOW32Reserved), ret);
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
