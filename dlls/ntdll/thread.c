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

#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#include "ntstatus.h"
#include "thread.h"
#include "winternl.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/pthread.h"
#include "wine/debug.h"
#include "ntdll_misc.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);

static PEB peb;
static PEB_LDR_DATA ldr;
static RTL_USER_PROCESS_PARAMETERS params;  /* default parameters if no parent */
static RTL_BITMAP tls_bitmap;
static LIST_ENTRY tls_links;


/***********************************************************************
 *           alloc_teb
 */
static TEB *alloc_teb( ULONG *size )
{
    TEB *teb;

    *size = SIGNAL_STACK_SIZE + sizeof(TEB);
    teb = wine_anon_mmap( NULL, *size, PROT_READ | PROT_WRITE | PROT_EXEC, 0 );
    if (teb == (TEB *)-1) return NULL;
    if (!(teb->teb_sel = wine_ldt_alloc_fs()))
    {
        munmap( teb, *size );
        return NULL;
    }
    teb->Tib.ExceptionList = (void *)~0UL;
    teb->Tib.StackBase     = (void *)~0UL;
    teb->Tib.Self          = &teb->Tib;
    teb->Peb               = &peb;
    teb->StaticUnicodeString.Buffer        = teb->StaticUnicodeBuffer;
    teb->StaticUnicodeString.MaximumLength = sizeof(teb->StaticUnicodeBuffer);
    return teb;
}


/***********************************************************************
 *           free_teb
 */
static inline void free_teb( TEB *teb )
{
    ULONG size = 0;
    void *addr = teb;

    NtFreeVirtualMemory( GetCurrentProcess(), &addr, &size, MEM_RELEASE );
    wine_ldt_free_fs( teb->teb_sel );
    munmap( teb, SIGNAL_STACK_SIZE + sizeof(TEB) );
}


/***********************************************************************
 *           thread_init
 *
 * Setup the initial thread.
 *
 * NOTES: The first allocated TEB on NT is at 0x7ffde000.
 */
void thread_init(void)
{
    TEB *teb;
    void *addr;
    ULONG size;
    static struct debug_info debug_info;  /* debug info for initial thread */

    debug_info.str_pos = debug_info.strings;
    debug_info.out_pos = debug_info.output;

    peb.ProcessParameters = &params;
    peb.TlsBitmap         = &tls_bitmap;
    peb.LdrData           = &ldr;
    RtlInitializeBitMap( &tls_bitmap, (BYTE *)peb.TlsBitmapBits, sizeof(peb.TlsBitmapBits) * 8 );
    InitializeListHead( &ldr.InLoadOrderModuleList );
    InitializeListHead( &ldr.InMemoryOrderModuleList );
    InitializeListHead( &ldr.InInitializationOrderModuleList );
    InitializeListHead( &tls_links );

    teb = alloc_teb( &size );
    teb->tibflags      = TEBF_WIN32;
    teb->request_fd    = -1;
    teb->reply_fd      = -1;
    teb->wait_fd[0]    = -1;
    teb->wait_fd[1]    = -1;
    teb->debug_info    = &debug_info;
    InsertHeadList( &tls_links, &teb->TlsLinks );

    SYSDEPS_SetCurThread( teb );

    /* setup the server connection */
    server_init_process();
    server_init_thread();

    /* create a memory view for the TEB */
    NtAllocateVirtualMemory( GetCurrentProcess(), &addr, teb, &size,
                             MEM_SYSTEM, PAGE_EXECUTE_READWRITE );

    /* create the process heap */
    if (!(peb.ProcessHeap = RtlCreateHeap( HEAP_GROWABLE, NULL, 0, 0, NULL, NULL )))
    {
        MESSAGE( "wine: failed to create the process heap\n" );
        exit(1);
    }
}


/***********************************************************************
 *           start_thread
 *
 * Startup routine for a newly created thread.
 */
static void start_thread( struct wine_pthread_thread_info *info )
{
    TEB *teb = info->teb_base;
    LPTHREAD_START_ROUTINE func = (LPTHREAD_START_ROUTINE)teb->entry_point;
    struct debug_info debug_info;
    ULONG size;

    debug_info.str_pos = debug_info.strings;
    debug_info.out_pos = debug_info.output;
    teb->debug_info = &debug_info;

    SYSDEPS_SetCurThread( teb );
    SIGNAL_Init();
    server_init_thread();

    /* allocate a memory view for the stack */
    size = info->stack_size;
    NtAllocateVirtualMemory( GetCurrentProcess(), &teb->DeallocationStack, info->stack_base,
                             &size, MEM_SYSTEM, PAGE_EXECUTE_READWRITE );
    /* limit is lower than base since the stack grows down */
    teb->Tib.StackBase  = (char *)info->stack_base + info->stack_size;
    teb->Tib.StackLimit = info->stack_base;

    /* setup the guard page */
    size = 1;
    NtProtectVirtualMemory( GetCurrentProcess(), &teb->DeallocationStack, &size,
                            PAGE_EXECUTE_READWRITE | PAGE_GUARD, NULL );
    RtlFreeHeap( GetProcessHeap(), 0, info );

    RtlAcquirePebLock();
    InsertHeadList( &tls_links, &teb->TlsLinks );
    RtlReleasePebLock();

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
    struct wine_pthread_thread_info *info = NULL;
    HANDLE handle = 0;
    TEB *teb = NULL;
    DWORD tid = 0;
    ULONG size;
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

    if (!(info = RtlAllocateHeap( GetProcessHeap(), 0, sizeof(*info) )))
    {
        status = STATUS_NO_MEMORY;
        goto error;
    }

    if (!(teb = alloc_teb( &size )))
    {
        status = STATUS_NO_MEMORY;
        goto error;
    }
    teb->ClientId.UniqueProcess = (HANDLE)GetCurrentProcessId();
    teb->ClientId.UniqueThread  = (HANDLE)tid;

    teb->tibflags    = TEBF_WIN32;
    teb->exit_code   = STILL_ACTIVE;
    teb->request_fd  = request_pipe[1];
    teb->reply_fd    = -1;
    teb->wait_fd[0]  = -1;
    teb->wait_fd[1]  = -1;
    teb->entry_point = start;
    teb->entry_arg   = param;
    teb->htask16     = NtCurrentTeb()->htask16;

    NtAllocateVirtualMemory( GetCurrentProcess(), &info->teb_base, teb, &size,
                             MEM_SYSTEM, PAGE_EXECUTE_READWRITE );
    info->teb_size = size;
    info->teb_sel  = teb->teb_sel;

    if (!stack_reserve || !stack_commit)
    {
        IMAGE_NT_HEADERS *nt = RtlImageNtHeader( NtCurrentTeb()->Peb->ImageBaseAddress );
        if (!stack_reserve) stack_reserve = nt->OptionalHeader.SizeOfStackReserve;
        if (!stack_commit) stack_commit = nt->OptionalHeader.SizeOfStackCommit;
    }
    if (stack_reserve < stack_commit) stack_reserve = stack_commit;
    stack_reserve = (stack_reserve + 0xffff) & ~0xffff;  /* round to 64K boundary */

    info->stack_base = NULL;
    info->stack_size = stack_reserve;
    info->entry      = start_thread;

    if (wine_pthread_create_thread( info ) == -1)
    {
        status = STATUS_NO_MEMORY;
        goto error;
    }

    if (id) id->UniqueThread = (HANDLE)tid;
    if (handle_ptr) *handle_ptr = handle;
    else NtClose( handle );

    return STATUS_SUCCESS;

error:
    if (teb) free_teb( teb );
    if (info) RtlFreeHeap( GetProcessHeap(), 0, info );
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
        else server_abort_thread( exit_code );
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
            LIST_ENTRY *entry;
            DWORD index;

            if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
            index = *(DWORD *)data;
            if (index >= 64) return STATUS_INVALID_PARAMETER;
            RtlAcquirePebLock();
            for (entry = tls_links.Flink; entry != &tls_links; entry = entry->Flink)
            {
                TEB *teb = CONTAINING_RECORD(entry, TEB, TlsLinks);
                teb->TlsSlots[index] = 0;
            }
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
