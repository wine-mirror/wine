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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>

#include "wine/winbase16.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "wownt32.h"
#include "excpt.h"
#include "winreg.h"
#include "winternl.h"
#include "file.h"
#include "task.h"
#include "miscemu.h"
#include "selectors.h"
#include "stackframe.h"
#include "kernel_private.h"
#include "wine/exception.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thunk);
WINE_DECLARE_DEBUG_CHANNEL(relay);
WINE_DECLARE_DEBUG_CHANNEL(snoop);

/*
 * These are the 16-bit side WOW routines.  They reside in wownt16.h
 * in the SDK; since we don't support Win16 source code anyway, I've
 * placed them here for compilation with Wine ...
 */

DWORD WINAPI GetVDMPointer32W16(SEGPTR,UINT16);

DWORD WINAPI LoadLibraryEx32W16(LPCSTR,DWORD,DWORD);
DWORD WINAPI GetProcAddress32W16(DWORD,LPCSTR);
DWORD WINAPI FreeLibrary32W16(DWORD);

#define CPEX_DEST_STDCALL   0x00000000L
#define CPEX_DEST_CDECL     0x80000000L

/* thunk for 16-bit CreateThread */
struct thread_args
{
    FARPROC16 proc;
    DWORD     param;
};

static DWORD CALLBACK start_thread16( LPVOID threadArgs )
{
    struct thread_args args = *(struct thread_args *)threadArgs;
    HeapFree( GetProcessHeap(), 0, threadArgs );
    return K32WOWCallback16( (DWORD)args.proc, args.param );
}


#ifdef __i386__

/* symbols exported from relay16.s */
extern DWORD WINAPI wine_call_to_16( FARPROC16 target, DWORD cbArgs, PEXCEPTION_HANDLER handler );
extern void WINAPI wine_call_to_16_regs( CONTEXT86 *context, DWORD cbArgs, PEXCEPTION_HANDLER handler );
extern void Call16_Ret_Start(), Call16_Ret_End();
extern void CallTo16_Ret();
extern void CALL32_CBClient_Ret();
extern void CALL32_CBClientEx_Ret();
extern DWORD CallTo16_DataSelector;
extern SEGPTR CALL32_CBClient_RetAddr;
extern SEGPTR CALL32_CBClientEx_RetAddr;
extern BYTE Call16_Start;
extern BYTE Call16_End;

extern void RELAY16_InitDebugLists(void);

static LONG CALLBACK vectored_handler( EXCEPTION_POINTERS *ptrs );
static SEGPTR call16_ret_addr;  /* segptr to CallTo16_Ret routine */

/***********************************************************************
 *           WOWTHUNK_Init
 */
BOOL WOWTHUNK_Init(void)
{
    /* allocate the code selector for CallTo16 routines */
    WORD codesel = SELECTOR_AllocBlock( (void *)Call16_Ret_Start,
                                        (char *)Call16_Ret_End - (char *)Call16_Ret_Start,
                                        WINE_LDT_FLAGS_CODE | WINE_LDT_FLAGS_32BIT );
    if (!codesel) return FALSE;

      /* Patch the return addresses for CallTo16 routines */

    CallTo16_DataSelector = wine_get_ds();
    call16_ret_addr = MAKESEGPTR( codesel, (char*)CallTo16_Ret - (char*)Call16_Ret_Start );
    CALL32_CBClient_RetAddr =
        MAKESEGPTR( codesel, (char*)CALL32_CBClient_Ret - (char*)Call16_Ret_Start );
    CALL32_CBClientEx_RetAddr =
        MAKESEGPTR( codesel, (char*)CALL32_CBClientEx_Ret - (char*)Call16_Ret_Start );

    if (TRACE_ON(relay) || TRACE_ON(snoop)) RELAY16_InitDebugLists();

    /* setup emulation of protected instructions from 32-bit code (only for Win9x versions) */
    if (GetVersion() & 0x80000000) RtlAddVectoredExceptionHandler( TRUE, vectored_handler );
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

    if (instr < &Call16_Start || instr >= &Call16_End) return FALSE;

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
        STACK32FRAME *frame32 = (STACK32FRAME *)((char *)frame - offsetof(STACK32FRAME,frame));
        NtCurrentTeb()->cur_stack = frame32->frame16;
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
            DWORD ret = INSTR_EmulateInstruction( record, context );

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
        return INSTR_EmulateInstruction( record, context );
    }

    return ExceptionContinueSearch;
}


/***********************************************************************
 *           vectored_handler
 *
 * Vectored exception handler used to emulate protected instructions
 * from 32-bit code.
 */
static LONG CALLBACK vectored_handler( EXCEPTION_POINTERS *ptrs )
{
    EXCEPTION_RECORD *record = ptrs->ExceptionRecord;
    CONTEXT *context = ptrs->ContextRecord;

    if (wine_ldt_is_system(context->SegCs) &&
        (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION ||
         record->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION))
    {
        if (INSTR_EmulateInstruction( record, context ) == ExceptionContinueExecution)
            return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}


#else  /* __i386__ */

BOOL WOWTHUNK_Init(void)
{
    return TRUE;
}

#endif  /* __i386__ */


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
#ifdef __i386__
    /*
     * Arguments must be prepared in the correct order by the caller
     * (both for PASCAL and CDECL calling convention), so we simply
     * copy them to the 16-bit stack ...
     */
    WORD *stack = (WORD *)CURRENT_STACK16 - cbArgs / sizeof(WORD);

    memcpy( stack, pArgs, cbArgs );

    if (dwFlags & (WCB16_REGS|WCB16_REGS_LONG))
    {
        CONTEXT *context = (CONTEXT *)pdwRetCode;

        if (TRACE_ON(relay))
        {
            DWORD count = cbArgs / sizeof(WORD);

            DPRINTF("%04lx:CallTo16(func=%04lx:%04x,ds=%04lx",
                    GetCurrentThreadId(),
                    context->SegCs, LOWORD(context->Eip), context->SegDs );
            while (count) DPRINTF( ",%04x", stack[--count] );
            DPRINTF(") ss:sp=%04x:%04x",
                    SELECTOROF(NtCurrentTeb()->cur_stack), OFFSETOF(NtCurrentTeb()->cur_stack) );
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
            __wine_push_frame( &frame );
            __wine_enter_vm86( context );
            __wine_pop_frame( &frame );
        }
        else
        {
            /* push return address */
            if (dwFlags & WCB16_REGS_LONG)
            {
                *((DWORD *)stack - 1) = HIWORD(call16_ret_addr);
                *((DWORD *)stack - 2) = LOWORD(call16_ret_addr);
                cbArgs += 2 * sizeof(DWORD);
            }
            else
            {
                *((SEGPTR *)stack - 1) = call16_ret_addr;
                cbArgs += sizeof(SEGPTR);
            }

            _EnterWin16Lock();
            wine_call_to_16_regs( context, cbArgs, call16_handler );
            _LeaveWin16Lock();
        }

        if (TRACE_ON(relay))
        {
            DPRINTF("%04lx:RetFrom16() ss:sp=%04x:%04x ",
                    GetCurrentThreadId(), SELECTOROF(NtCurrentTeb()->cur_stack),
                    OFFSETOF(NtCurrentTeb()->cur_stack));
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

            DPRINTF("%04lx:CallTo16(func=%04x:%04x,ds=%04x",
                    GetCurrentThreadId(), HIWORD(vpfn16), LOWORD(vpfn16),
                    SELECTOROF(NtCurrentTeb()->cur_stack) );
            while (count) DPRINTF( ",%04x", stack[--count] );
            DPRINTF(") ss:sp=%04x:%04x\n",
                    SELECTOROF(NtCurrentTeb()->cur_stack), OFFSETOF(NtCurrentTeb()->cur_stack) );
            SYSLEVEL_CheckNotLevel( 2 );
        }

        /* push return address */
        *((SEGPTR *)stack - 1) = call16_ret_addr;
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
            DPRINTF("%04lx:RetFrom16() ss:sp=%04x:%04x retval=%08lx\n",
                    GetCurrentThreadId(), SELECTOROF(NtCurrentTeb()->cur_stack),
                    OFFSETOF(NtCurrentTeb()->cur_stack), ret);
            SYSLEVEL_CheckNotLevel( 2 );
        }
    }
#else
    assert(0);  /* cannot call to 16-bit on non-Intel architectures */
#endif  /* __i386__ */

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
    DOS_FULL_NAME full_name;
    DWORD mutex_count;
    UNICODE_STRING libfileW;
    LPCWSTR filenameW;
    static const WCHAR dllW[] = {'.','D','L','L',0};

    if (!lpszLibFile)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!RtlCreateUnicodeStringFromAsciiz(&libfileW, lpszLibFile))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    /* if the file can not be found, call LoadLibraryExA anyway, since it might be
       a buildin module. This case is handled in MODULE_LoadLibraryExA */

    filenameW = libfileW.Buffer;
    if ( DIR_SearchPath( NULL, filenameW, dllW, &full_name, FALSE ) )
        filenameW = full_name.short_name;

    ReleaseThunkLock( &mutex_count );
    hModule = LoadLibraryExW( filenameW, (HANDLE)hFile, dwFlags );
    RestoreThunkLock( mutex_count );

    RtlFreeUnicodeString(&libfileW);

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

    /*
     * FIXME:  If ( nrofargs & CPEX_DEST_CDECL ) != 0, we should call a
     *         32-bit CDECL routine ...
     */

    if (!proc32) ret = 0;
    else switch (nrofargs)
    {
    case 0: ret = proc32();
            break;
    case 1: ret = proc32(args[0]);
            break;
    case 2: ret = proc32(args[0],args[1]);
            break;
    case 3: ret = proc32(args[0],args[1],args[2]);
            break;
    case 4: ret = proc32(args[0],args[1],args[2],args[3]);
            break;
    case 5: ret = proc32(args[0],args[1],args[2],args[3],args[4]);
            break;
    case 6: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5]);
            break;
    case 7: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6]);
            break;
    case 8: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7]);
            break;
    case 9: ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8]);
            break;
    case 10:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9]);
            break;
    case 11:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10]);
            break;
    case 12:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11]);
            break;
    case 13:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12]);
            break;
    case 14:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13]);
            break;
    case 15:ret = proc32(args[0],args[1],args[2],args[3],args[4],args[5],args[6],args[7],args[8],args[9],args[10],args[11],args[12],args[13],args[14]);
            break;
    default:
            /* FIXME: should go up to 32  arguments */
            ERR("Unsupported number of arguments %ld, please report.\n",nrofargs);
            ret = 0;
            break;
    }

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
    unsigned int i;

    TRACE("(%ld,%ld,%p args[",nrofargs,argconvmask,proc32);

    for (i=0;i<nrofargs;i++)
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
                DPRINTF("%04x ",a);
        }
        calladdr = VA_ARG16(args,DWORD);
        stack16_pop( 3*sizeof(WORD) + x + sizeof(DWORD) );
        DPRINTF(") calling address was 0x%08lx\n",calladdr);
        return 0;
}


/***********************************************************************
 *           CreateThread16   (KERNEL.441)
 */
HANDLE WINAPI CreateThread16( SECURITY_ATTRIBUTES *sa, DWORD stack,
                              FARPROC16 start, SEGPTR param,
                              DWORD flags, LPDWORD id )
{
    struct thread_args *args = HeapAlloc( GetProcessHeap(), 0, sizeof(*args) );
    if (!args) return INVALID_HANDLE_VALUE;
    args->proc = start;
    args->param = param;
    return CreateThread( sa, stack, start_thread16, args, flags, id );
}
