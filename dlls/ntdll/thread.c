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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include "wine/port.h"

#include "ntstatus.h"
#include "thread.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);


/***********************************************************************
 *           thread_init
 *
 * Setup the initial thread.
 *
 * NOTES: The first allocated TEB on NT is at 0x7ffde000.
 */
DECL_GLOBAL_CONSTRUCTOR(thread_init)
{
    static TEB teb;
    static PEB peb;
    static PEB_LDR_DATA ldr;
    static RTL_USER_PROCESS_PARAMETERS params;  /* default parameters if no parent */
    static RTL_BITMAP tls_bitmap;
    static struct debug_info info;  /* debug info for initial thread */

    if (teb.Tib.Self) return;  /* do it only once */

    info.str_pos = info.strings;
    info.out_pos = info.output;

    teb.Tib.ExceptionList = (void *)~0UL;
    teb.Tib.StackBase     = (void *)~0UL;
    teb.Tib.Self          = &teb.Tib;
    teb.Peb               = &peb;
    teb.tibflags          = TEBF_WIN32;
    teb.request_fd        = -1;
    teb.reply_fd          = -1;
    teb.wait_fd[0]        = -1;
    teb.wait_fd[1]        = -1;
    teb.teb_sel           = wine_ldt_alloc_fs();
    teb.debug_info        = &info;
    teb.StaticUnicodeString.MaximumLength = sizeof(teb.StaticUnicodeBuffer);
    teb.StaticUnicodeString.Buffer        = teb.StaticUnicodeBuffer;
    InitializeListHead( &teb.TlsLinks );

    peb.ProcessParameters = &params;
    peb.TlsBitmap         = &tls_bitmap;
    peb.LdrData           = &ldr;
    RtlInitializeBitMap( &tls_bitmap, (BYTE *)peb.TlsBitmapBits, sizeof(peb.TlsBitmapBits) * 8 );
    InitializeListHead( &ldr.InLoadOrderModuleList );
    InitializeListHead( &ldr.InMemoryOrderModuleList );
    InitializeListHead( &ldr.InInitializationOrderModuleList );

    SYSDEPS_SetCurThread( &teb );
}


/* startup routine for a newly created thread */
static void start_thread( TEB *teb )
{
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)teb->entry_point;
    struct debug_info info;

    info.str_pos = info.strings;
    info.out_pos = info.output;
    teb->debug_info = &info;

    SYSDEPS_SetCurThread( teb );
    SIGNAL_Init();
    wine_server_init_thread();

    NtTerminateThread( GetCurrentThread(), func( NtCurrentTeb()->entry_arg ) );
}


/***********************************************************************
 *              RtlCreateUserThread   (NTDLL.@)
 */
NTSTATUS WINAPI RtlCreateUserThread( HANDLE process, const SECURITY_DESCRIPTOR *descr,
                                     BOOLEAN suspended, PVOID stack_addr,
                                     SIZE_T stack_reserve, SIZE_T stack_commit,
                                     PRTL_THREAD_START_ROUTINE start, void *param,
                                     HANDLE *handle_ptr, CLIENT_ID *id )
{
    HANDLE handle = 0;
    TEB *teb;
    DWORD tid = 0;
    SIZE_T total_size;
    SIZE_T page_size = getpagesize();
    void *ptr, *base = NULL;
    int request_pipe[2];
    NTSTATUS status;

    if (pipe( request_pipe ) == -1) return STATUS_TOO_MANY_OPENED_FILES;
    fcntl( request_pipe[1], F_SETFD, 1 ); /* set close on exec flag */
    wine_server_send_fd( request_pipe[0] );

    SERVER_START_REQ( new_thread )
    {
        req->suspend    = suspended;
        req->inherit    = 0;  /* FIXME */
        req->request_fd = request_pipe[0];
        if (!(status = wine_server_call( req )))
        {
            handle = reply->handle;
            tid = reply->tid;
        }
        close( request_pipe[0] );
    }
    SERVER_END_REQ;

    if (status) goto error;

    if (!stack_reserve || !stack_commit)
    {
        IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
        if (!stack_reserve) stack_reserve = nt->OptionalHeader.SizeOfStackReserve;
        if (!stack_commit) stack_commit = nt->OptionalHeader.SizeOfStackCommit;
    }
    if (stack_reserve < stack_commit) stack_reserve = stack_commit;
    stack_reserve = (stack_reserve + 0xffff) & ~0xffff;  /* round to 64K boundary */

    /* Memory layout in allocated block:
     *
     *   size                 contents
     * SIGNAL_STACK_SIZE   signal stack
     * stack_size          normal stack (including a PAGE_GUARD page at the bottom)
     * 1 page              TEB (except for initial thread)
     */

    total_size = stack_reserve + SIGNAL_STACK_SIZE + page_size;
    if ((status = NtAllocateVirtualMemory( GetCurrentProcess(), &base, NULL, &total_size,
                                           MEM_COMMIT, PAGE_EXECUTE_READWRITE )) != STATUS_SUCCESS)
        goto error;

    teb = (TEB *)((char *)base + total_size - page_size);

    if (!(teb->teb_sel = wine_ldt_alloc_fs()))
    {
        status = STATUS_TOO_MANY_THREADS;
        goto error;
    }

    teb->Tib.ExceptionList          = (void *)~0UL;
    teb->Tib.StackBase              = (char *)base + SIGNAL_STACK_SIZE + stack_reserve;
    teb->Tib.StackLimit             = base;  /* limit is lower than base since the stack grows down */
    teb->Tib.Self                   = &teb->Tib;
    teb->ClientId.UniqueProcess     = (HANDLE)GetCurrentProcessId();
    teb->ClientId.UniqueThread      = (HANDLE)tid;
    teb->Peb                        = NtCurrentTeb()->Peb;
    teb->DeallocationStack          = base;
    teb->StaticUnicodeString.Buffer = teb->StaticUnicodeBuffer;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);
    RtlAcquirePebLock();
    InsertHeadList( &NtCurrentTeb()->TlsLinks, &teb->TlsLinks );
    RtlReleasePebLock();

    teb->tibflags    = TEBF_WIN32;
    teb->exit_code   = STILL_ACTIVE;
    teb->request_fd  = request_pipe[1];
    teb->reply_fd    = -1;
    teb->wait_fd[0]  = -1;
    teb->wait_fd[1]  = -1;
    teb->entry_point = start;
    teb->entry_arg   = param;
    teb->htask16     = NtCurrentTeb()->htask16;

    /* setup the guard page */
    ptr = (char *)base + SIGNAL_STACK_SIZE;
    NtProtectVirtualMemory( GetCurrentProcess(), &ptr, &page_size,
                            PAGE_EXECUTE_READWRITE | PAGE_GUARD, NULL );

    if (SYSDEPS_SpawnThread( start_thread, teb ) == -1)
    {
        RtlAcquirePebLock();
        RemoveEntryList( &teb->TlsLinks );
        RtlReleasePebLock();
        wine_ldt_free_fs( teb->teb_sel );
        status = STATUS_TOO_MANY_THREADS;
        goto error;
    }

    if (id) id->UniqueThread = (HANDLE)tid;
    if (handle_ptr) *handle_ptr = handle;
    else NtClose( handle );

    return STATUS_SUCCESS;

error:
    if (base)
    {
        total_size = 0;
        NtFreeVirtualMemory( GetCurrentProcess(), &base, &total_size, MEM_RELEASE );
    }
    if (handle) NtClose( handle );
    close( request_pipe[1] );
    return status;
}


/***********************************************************************
 *              NtOpenThread   (NTDLL.@)
 *              ZwOpenThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtOpenThread( HANDLE *handle, ACCESS_MASK access,
                              const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    NTSTATUS ret;

    SERVER_START_REQ( open_thread )
    {
        req->tid     = (thread_id_t)id->UniqueThread;
        req->access  = access;
        req->inherit = attr && (attr->Attributes & OBJ_INHERIT);
        ret = wine_server_call( req );
        *handle = reply->handle;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtSuspendThread   (NTDLL.@)
 *              ZwSuspendThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtSuspendThread( HANDLE handle, PULONG count )
{
    NTSTATUS ret;

    SERVER_START_REQ( suspend_thread )
    {
        req->handle = handle;
        if (!(ret = wine_server_call( req ))) *count = reply->count;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtResumeThread   (NTDLL.@)
 *              ZwResumeThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtResumeThread( HANDLE handle, PULONG count )
{
    NTSTATUS ret;

    SERVER_START_REQ( resume_thread )
    {
        req->handle = handle;
        if (!(ret = wine_server_call( req ))) *count = reply->count;
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtTerminateThread  (NTDLL.@)
 *              ZwTerminateThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtTerminateThread( HANDLE handle, LONG exit_code )
{
    NTSTATUS ret;
    BOOL self, last;

    SERVER_START_REQ( terminate_thread )
    {
        req->handle    = handle;
        req->exit_code = exit_code;
        ret = wine_server_call( req );
        self = !ret && reply->self;
        last = reply->last;
    }
    SERVER_END_REQ;

    if (self)
    {
        if (last) exit( exit_code );
        else SYSDEPS_AbortThread( exit_code );
    }
    return ret;
}


/******************************************************************************
 *              NtQueueApcThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueueApcThread( HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1,
                                  ULONG_PTR arg2, ULONG_PTR arg3 )
{
    NTSTATUS ret;
    SERVER_START_REQ( queue_apc )
    {
        req->handle = handle;
        req->user   = 1;
        req->func   = func;
        req->arg1   = (void *)arg1;
        req->arg2   = (void *)arg2;
        req->arg3   = (void *)arg3;
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    NTSTATUS ret;

    SERVER_START_REQ( set_thread_context )
    {
        req->handle = handle;
        req->flags  = context->ContextFlags;
        wine_server_add_data( req, context, sizeof(*context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    NTSTATUS ret;

    SERVER_START_REQ( get_thread_context )
    {
        req->handle = handle;
        req->flags = context->ContextFlags;
        wine_server_add_data( req, context, sizeof(*context) );
        wine_server_set_reply( req, context, sizeof(*context) );
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtQueryInformationThread  (NTDLL.@)
 *              ZwQueryInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtQueryInformationThread( HANDLE handle, THREADINFOCLASS class,
                                          void *data, ULONG length, ULONG *ret_len )
{
    NTSTATUS status;

    switch(class)
    {
    case ThreadBasicInformation:
        {
            THREAD_BASIC_INFORMATION info;

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = handle;
                req->tid_in = 0;
                if (!(status = wine_server_call( req )))
                {
                    info.ExitStatus             = reply->exit_code;
                    info.TebBaseAddress         = reply->teb;
                    info.ClientId.UniqueProcess = (HANDLE)reply->pid;
                    info.ClientId.UniqueThread  = (HANDLE)reply->tid;
                    info.AffinityMask           = reply->affinity;
                    info.Priority               = reply->priority;
                    info.BasePriority           = reply->priority;  /* FIXME */
                }
            }
            SERVER_END_REQ;
            if (status == STATUS_SUCCESS)
            {
                if (data) memcpy( data, &info, min( length, sizeof(info) ));
                if (ret_len) *ret_len = min( length, sizeof(info) );
            }
        }
        return status;
    case ThreadTimes:
    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadAffinityMask:
    case ThreadImpersonationToken:
    case ThreadDescriptorTableEntry:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadQuerySetWin32StartAddress:
    case ThreadZeroTlsCell:
    case ThreadPerformanceCount:
    case ThreadAmILastThread:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
    case ThreadIsIoPending:
    default:
        FIXME( "info class %d not supported yet\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}


/******************************************************************************
 *              NtSetInformationThread  (NTDLL.@)
 *              ZwSetInformationThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetInformationThread( HANDLE handle, THREADINFOCLASS class,
                                        LPCVOID data, ULONG length )
{
    switch(class)
    {
    case ThreadZeroTlsCell:
        if (handle == GetCurrentThread())
        {
            LIST_ENTRY *entry = &NtCurrentTeb()->TlsLinks;
            DWORD index;

            if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
            index = *(DWORD *)data;
            if (index >= 64) return STATUS_INVALID_PARAMETER;
            RtlAcquirePebLock();
            do
            {
                TEB *teb = CONTAINING_RECORD(entry, TEB, TlsLinks);
                teb->TlsSlots[index] = 0;
                entry = entry->Flink;
            } while (entry != &NtCurrentTeb()->TlsLinks);
            RtlReleasePebLock();
            return STATUS_SUCCESS;
        }
        FIXME( "ZeroTlsCell not supported on other threads\n" );
        return STATUS_NOT_IMPLEMENTED;

    case ThreadBasicInformation:
    case ThreadTimes:
    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadAffinityMask:
    case ThreadImpersonationToken:
    case ThreadDescriptorTableEntry:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadQuerySetWin32StartAddress:
    case ThreadPerformanceCount:
    case ThreadAmILastThread:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
    case ThreadIsIoPending:
    default:
        FIXME( "info class %d not supported yet\n", class );
        return STATUS_NOT_IMPLEMENTED;
    }
}
