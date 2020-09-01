/*
 * NT threads support
 *
 * Copyright 1996, 2003 Alexandre Julliard
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

#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "ddk/wdm.h"
#include "wine/exception.h"

WINE_DECLARE_DEBUG_CHANNEL(relay);

struct _KUSER_SHARED_DATA *user_shared_data = (void *)0x7ffe0000;


/***********************************************************************
 *		__wine_dbg_get_channel_flags  (NTDLL.@)
 *
 * Get the flags to use for a given channel, possibly setting them too in case of lazy init
 */
unsigned char __cdecl __wine_dbg_get_channel_flags( struct __wine_debug_channel *channel )
{
    return unix_funcs->dbg_get_channel_flags( channel );
}

/***********************************************************************
 *		__wine_dbg_strdup  (NTDLL.@)
 */
const char * __cdecl __wine_dbg_strdup( const char *str )
{
    return unix_funcs->dbg_strdup( str );
}

/***********************************************************************
 *		__wine_dbg_header  (NTDLL.@)
 */
int __cdecl __wine_dbg_header( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                               const char *function )
{
    return unix_funcs->dbg_header( cls, channel, function );
}

/***********************************************************************
 *		__wine_dbg_output  (NTDLL.@)
 */
int __cdecl __wine_dbg_output( const char *str )
{
    return unix_funcs->dbg_output( str );
}


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
void WINAPI KiUserApcDispatcher( CONTEXT *context, ULONG_PTR ctx, ULONG_PTR arg1, ULONG_PTR arg2,
                                 PNTAPCFUNC func )
{
    func( ctx, arg1, arg2 );
    NtContinue( context, TRUE );
}


/***********************************************************************
 *           RtlExitUserThread  (NTDLL.@)
 */
void WINAPI RtlExitUserThread( ULONG status )
{
    ULONG last;

    NtQueryInformationThread( GetCurrentThread(), ThreadAmILastThread, &last, sizeof(last), NULL );
    if (last) RtlExitUserProcess( status );
    LdrShutdownThread();
    RtlFreeThreadActivationContextStack();
    for (;;) NtTerminateThread( GetCurrentThread(), status );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( RtlUserThreadStart, 8,
                   "movl %ebx,8(%esp)\n\t"  /* arg */
                   "movl %eax,4(%esp)\n\t"  /* entry */
                   "jmp " __ASM_NAME("call_thread_func") )

/* wrapper to call BaseThreadInitThunk */
extern void DECLSPEC_NORETURN call_thread_func_wrapper( void *thunk, PRTL_THREAD_START_ROUTINE entry, void *arg );
__ASM_GLOBAL_FUNC( call_thread_func_wrapper,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "subl $4,%esp\n\t"
                   "andl $~0xf,%esp\n\t"
                   "xorl %ecx,%ecx\n\t"
                   "movl 12(%ebp),%edx\n\t"
                   "movl 16(%ebp),%eax\n\t"
                   "movl %eax,(%esp)\n\t"
                   "call *8(%ebp)" )

void DECLSPEC_HIDDEN call_thread_func( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        call_thread_func_wrapper( pBaseThreadInitThunk, entry, arg );
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateProcess( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}

#else  /* __i386__ */

void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        pBaseThreadInitThunk( 0, (LPTHREAD_START_ROUTINE)entry, arg );
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateProcess( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
}

#endif  /* __i386__ */


/***********************************************************************
 *              RtlCreateUserThread   (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateUserThread( HANDLE process, SECURITY_DESCRIPTOR *descr,
                                     BOOLEAN suspended, PVOID stack_addr,
                                     SIZE_T stack_reserve, SIZE_T stack_commit,
                                     PRTL_THREAD_START_ROUTINE start, void *param,
                                     HANDLE *handle_ptr, CLIENT_ID *id )
{
    ULONG flags = suspended ? THREAD_CREATE_FLAGS_CREATE_SUSPENDED : 0;
    ULONG_PTR buffer[offsetof( PS_ATTRIBUTE_LIST, Attributes[2] ) / sizeof(ULONG_PTR)];
    PS_ATTRIBUTE_LIST *attr_list = (PS_ATTRIBUTE_LIST *)buffer;
    HANDLE handle, actctx;
    TEB *teb;
    ULONG ret;
    NTSTATUS status;
    CLIENT_ID client_id;
    OBJECT_ATTRIBUTES attr;

    attr_list->TotalLength = sizeof(buffer);
    attr_list->Attributes[0].Attribute    = PS_ATTRIBUTE_CLIENT_ID;
    attr_list->Attributes[0].Size         = sizeof(client_id);
    attr_list->Attributes[0].ValuePtr     = &client_id;
    attr_list->Attributes[0].ReturnLength = NULL;
    attr_list->Attributes[1].Attribute    = PS_ATTRIBUTE_TEB_ADDRESS;
    attr_list->Attributes[1].Size         = sizeof(teb);
    attr_list->Attributes[1].ValuePtr     = &teb;
    attr_list->Attributes[1].ReturnLength = NULL;

    InitializeObjectAttributes( &attr, NULL, 0, NULL, descr );

    RtlGetActiveActivationContext( &actctx );
    if (actctx) flags |= THREAD_CREATE_FLAGS_CREATE_SUSPENDED;

    status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, &attr, process, start, param,
                               flags, 0, stack_commit, stack_reserve, attr_list );
    if (!status)
    {
        if (actctx)
        {
            ULONG_PTR cookie;
            RtlActivateActivationContextEx( 0, teb, actctx, &cookie );
            if (!suspended) NtResumeThread( handle, &ret );
        }
        if (id) *id = client_id;
        if (handle_ptr) *handle_ptr = handle;
        else NtClose( handle );
    }
    if (actctx) RtlReleaseActivationContext( actctx );
    return status;
}


/******************************************************************************
 *              RtlGetNtGlobalFlags   (NTDLL.@)
 */
ULONG WINAPI RtlGetNtGlobalFlags(void)
{
    return NtCurrentTeb()->Peb->NtGlobalFlag;
}


/******************************************************************************
 *              RtlPushFrame  (NTDLL.@)
 */
void WINAPI RtlPushFrame( TEB_ACTIVE_FRAME *frame )
{
    frame->Previous = NtCurrentTeb()->ActiveFrame;
    NtCurrentTeb()->ActiveFrame = frame;
}


/******************************************************************************
 *              RtlPopFrame  (NTDLL.@)
 */
void WINAPI RtlPopFrame( TEB_ACTIVE_FRAME *frame )
{
    NtCurrentTeb()->ActiveFrame = frame->Previous;
}


/******************************************************************************
 *              RtlGetFrame  (NTDLL.@)
 */
TEB_ACTIVE_FRAME * WINAPI RtlGetFrame(void)
{
    return NtCurrentTeb()->ActiveFrame;
}
