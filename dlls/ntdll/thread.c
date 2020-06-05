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

#include "config.h"
#include "wine/port.h"

#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <sys/types.h>
#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif
#ifdef HAVE_SYS_TIMES_H
#include <sys/times.h>
#endif
#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#endif

#define NONAMELESSUNION
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "winternl.h"
#include "wine/library.h"
#include "wine/server.h"
#include "wine/debug.h"
#include "ntdll_misc.h"
#include "ddk/wdm.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(thread);
WINE_DECLARE_DEBUG_CHANNEL(relay);

struct _KUSER_SHARED_DATA *user_shared_data = (void *)0x7ffe0000;

void (WINAPI *kernel32_start_process)(LPTHREAD_START_ROUTINE,void*) = NULL;

static PEB *peb;
static PEB_LDR_DATA ldr;
static RTL_BITMAP tls_bitmap;
static RTL_BITMAP tls_expansion_bitmap;
static RTL_BITMAP fls_bitmap;
static int nb_threads = 1;

struct ldt_copy *__wine_ldt_copy = NULL;

static RTL_CRITICAL_SECTION peb_lock;
static RTL_CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &peb_lock,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": peb_lock") }
};
static RTL_CRITICAL_SECTION peb_lock = { &critsect_debug, -1, 0, 0, 0, 0 };

#ifdef __linux__

#ifdef HAVE_ELF_H
# include <elf.h>
#endif
#ifdef HAVE_LINK_H
# include <link.h>
#endif
#ifdef HAVE_SYS_AUXV_H
# include <sys/auxv.h>
#endif
#ifndef HAVE_GETAUXVAL
static unsigned long getauxval( unsigned long id )
{
    extern char **__wine_main_environ;
    char **ptr = __wine_main_environ;
    ElfW(auxv_t) *auxv;

    while (*ptr) ptr++;
    while (!*ptr) ptr++;
    for (auxv = (ElfW(auxv_t) *)ptr; auxv->a_type; auxv++)
        if (auxv->a_type == id) return auxv->a_un.a_val;
    return 0;
}
#endif

static ULONG_PTR get_image_addr(void)
{
    ULONG_PTR size, num, phdr_addr = getauxval( AT_PHDR );
    ElfW(Phdr) *phdr;

    if (!phdr_addr) return 0;
    phdr = (ElfW(Phdr) *)phdr_addr;
    size = getauxval( AT_PHENT );
    num = getauxval( AT_PHNUM );
    while (num--)
    {
        if (phdr->p_type == PT_PHDR) return phdr_addr - phdr->p_offset;
        phdr = (ElfW(Phdr) *)((char *)phdr + size);
    }
    return 0;
}

#elif defined(__APPLE__)
#include <mach/mach.h>
#include <mach/mach_error.h>

static ULONG_PTR get_image_addr(void)
{
    ULONG_PTR ret = 0;
#ifdef TASK_DYLD_INFO
    struct task_dyld_info dyld_info;
    mach_msg_type_number_t size = TASK_DYLD_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_DYLD_INFO, (task_info_t)&dyld_info, &size) == KERN_SUCCESS)
        ret = dyld_info.all_image_info_addr;
#endif
    return ret;
}

#else
static ULONG_PTR get_image_addr(void)
{
    return 0;
}
#endif


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


/***********************************************************************
 *           thread_init
 *
 * Setup the initial thread.
 *
 * NOTES: The first allocated TEB on NT is at 0x7ffde000.
 */
TEB *thread_init( SIZE_T *info_size, BOOL *suspend )
{
    TEB *teb;

    virtual_init();

    teb = unix_funcs->init_threading( &nb_threads, &__wine_ldt_copy, info_size, suspend, &server_cpus,
                                      &is_wow64, &server_start_time );

    peb = teb->Peb;
    peb->FastPebLock        = &peb_lock;
    peb->TlsBitmap          = &tls_bitmap;
    peb->TlsExpansionBitmap = &tls_expansion_bitmap;
    peb->FlsBitmap          = &fls_bitmap;
    peb->LdrData            = &ldr;
    peb->OSMajorVersion     = 5;
    peb->OSMinorVersion     = 1;
    peb->OSBuildNumber      = 0xA28;
    peb->OSPlatformId       = VER_PLATFORM_WIN32_NT;
    ldr.Length = sizeof(ldr);
    ldr.Initialized = TRUE;
    RtlInitializeBitMap( &tls_bitmap, peb->TlsBitmapBits, sizeof(peb->TlsBitmapBits) * 8 );
    RtlInitializeBitMap( &tls_expansion_bitmap, peb->TlsExpansionBitmapBits,
                         sizeof(peb->TlsExpansionBitmapBits) * 8 );
    RtlInitializeBitMap( &fls_bitmap, peb->FlsBitmapBits, sizeof(peb->FlsBitmapBits) * 8 );
    RtlSetBits( peb->TlsBitmap, 0, 1 ); /* TLS index 0 is reserved and should be initialized to NULL. */
    RtlSetBits( peb->FlsBitmap, 0, 1 );
    InitializeListHead( &peb->FlsListHead );
    InitializeListHead( &ldr.InLoadOrderModuleList );
    InitializeListHead( &ldr.InMemoryOrderModuleList );
    InitializeListHead( &ldr.InInitializationOrderModuleList );
    *(ULONG_PTR *)peb->Reserved = get_image_addr();

    /*
     * Starting with Vista, the first user to log on has session id 1.
     * Session id 0 is for processes that don't interact with the user (like services).
     */
    peb->SessionId = 1;

    unix_funcs->get_paths( &build_dir, &data_dir, &config_dir );
    fill_cpu_info();
    server_init_process();
    return teb;
}


/***********************************************************************
 *           RtlExitUserThread  (NTDLL.@)
 */
void WINAPI RtlExitUserThread( ULONG status )
{
    if (status)  /* send the exit code to the server (0 is already the default) */
    {
        SERVER_START_REQ( terminate_thread )
        {
            req->handle    = wine_server_obj_handle( GetCurrentThread() );
            req->exit_code = status;
            wine_server_call( req );
        }
        SERVER_END_REQ;
    }

    if (InterlockedDecrement( &nb_threads ) <= 0)
    {
        LdrShutdownProcess();
        unix_funcs->exit_process( status );
    }

    LdrShutdownThread();
    RtlFreeThreadActivationContextStack();
    for (;;) unix_funcs->exit_thread( status );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
#ifdef __i386__
__ASM_STDCALL_FUNC( RtlUserThreadStart, 8,
                   "pushl %ebp\n\t"
                   __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                   __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                   "movl %esp,%ebp\n\t"
                   __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                   "pushl %ebx\n\t"  /* arg */
                   "pushl %eax\n\t"  /* entry */
                   "call " __ASM_NAME("call_thread_func") )

/* wrapper for apps that don't declare the thread function correctly */
extern DWORD call_thread_func_wrapper( PRTL_THREAD_START_ROUTINE entry, void *arg );
__ASM_GLOBAL_FUNC(call_thread_func_wrapper,
                  "pushl %ebp\n\t"
                  __ASM_CFI(".cfi_adjust_cfa_offset 4\n\t")
                  __ASM_CFI(".cfi_rel_offset %ebp,0\n\t")
                  "movl %esp,%ebp\n\t"
                  __ASM_CFI(".cfi_def_cfa_register %ebp\n\t")
                  "subl $4,%esp\n\t"
                  "pushl 12(%ebp)\n\t"
                  "call *8(%ebp)\n\t"
                  "leave\n\t"
                  __ASM_CFI(".cfi_def_cfa %esp,4\n\t")
                  __ASM_CFI(".cfi_same_value %ebp\n\t")
                  "ret" )

void DECLSPEC_HIDDEN call_thread_func( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        RtlExitUserThread( call_thread_func_wrapper( entry, arg ));
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}

#else  /* __i386__ */

void WINAPI RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    __TRY
    {
        TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", entry, arg );
        RtlExitUserThread( ((LPTHREAD_START_ROUTINE)entry)( arg ));
    }
    __EXCEPT(call_unhandled_exception_filter)
    {
        NtTerminateThread( GetCurrentThread(), GetExceptionCode() );
    }
    __ENDTRY
    abort();  /* should not be reached */
}

#endif  /* __i386__ */


/***********************************************************************
 *              NtCreateThreadEx   (NTDLL.@)
 */
NTSTATUS WINAPI NtCreateThreadEx( HANDLE *handle_ptr, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                  HANDLE process, PRTL_THREAD_START_ROUTINE start, void *param,
                                  ULONG flags, SIZE_T zero_bits, SIZE_T stack_commit,
                                  SIZE_T stack_reserve, PS_ATTRIBUTE_LIST *attr_list )
{
    return unix_funcs->NtCreateThreadEx( handle_ptr, access, attr, process, start, param,
                                         flags, zero_bits, stack_commit, stack_reserve, attr_list );
}


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
    HANDLE handle;
    NTSTATUS status;
    CLIENT_ID client_id;
    OBJECT_ATTRIBUTES attr;
    PS_ATTRIBUTE_LIST attr_list = { sizeof(attr_list) };

    attr_list.Attributes[0].Attribute = PS_ATTRIBUTE_CLIENT_ID;
    attr_list.Attributes[0].Size      = sizeof(client_id);
    attr_list.Attributes[0].ValuePtr  = &client_id;

    InitializeObjectAttributes( &attr, NULL, 0, NULL, descr );

    status = NtCreateThreadEx( &handle, THREAD_ALL_ACCESS, &attr, process, start, param,
                               flags, 0, stack_commit, stack_reserve, &attr_list );
    if (!status)
    {
        if (id) *id = client_id;
        if (handle_ptr) *handle_ptr = handle;
        else NtClose( handle );
    }
    return status;
}


/******************************************************************************
 *              RtlGetNtGlobalFlags   (NTDLL.@)
 */
ULONG WINAPI RtlGetNtGlobalFlags(void)
{
    if (!peb) return 0;  /* init not done yet */
    return peb->NtGlobalFlag;
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
        req->tid        = HandleToULong(id->UniqueThread);
        req->access     = access;
        req->attributes = attr ? attr->Attributes : 0;
        ret = wine_server_call( req );
        *handle = wine_server_ptr_handle( reply->handle );
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
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            if (count) *count = reply->count;
        }
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
        req->handle = wine_server_obj_handle( handle );
        if (!(ret = wine_server_call( req )))
        {
            if (count) *count = reply->count;
        }
    }
    SERVER_END_REQ;
    return ret;
}


/******************************************************************************
 *              NtAlertResumeThread   (NTDLL.@)
 *              ZwAlertResumeThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtAlertResumeThread( HANDLE handle, PULONG count )
{
    FIXME( "stub: should alert thread %p\n", handle );
    return NtResumeThread( handle, count );
}


/******************************************************************************
 *              NtAlertThread   (NTDLL.@)
 *              ZwAlertThread   (NTDLL.@)
 */
NTSTATUS WINAPI NtAlertThread( HANDLE handle )
{
    FIXME( "stub: %p\n", handle );
    return STATUS_NOT_IMPLEMENTED;
}


/******************************************************************************
 *              NtTerminateThread  (NTDLL.@)
 *              ZwTerminateThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtTerminateThread( HANDLE handle, LONG exit_code )
{
    NTSTATUS ret;
    BOOL self = (handle == GetCurrentThread());

    if (!self || exit_code)
    {
        SERVER_START_REQ( terminate_thread )
        {
            req->handle    = wine_server_obj_handle( handle );
            req->exit_code = exit_code;
            ret = wine_server_call( req );
            self = !ret && reply->self;
        }
        SERVER_END_REQ;
    }
    if (self) unix_funcs->abort_thread( exit_code );
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
        req->handle = wine_server_obj_handle( handle );
        if (func)
        {
            req->call.type              = APC_USER;
            req->call.user.user.func    = wine_server_client_ptr( func );
            req->call.user.user.args[0] = arg1;
            req->call.user.user.args[1] = arg2;
            req->call.user.user.args[2] = arg3;
        }
        else req->call.type = APC_NONE;  /* wake up only */
        ret = wine_server_call( req );
    }
    SERVER_END_REQ;
    return ret;
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


/***********************************************************************
 *              set_thread_context
 */
NTSTATUS set_thread_context( HANDLE handle, const context_t *context, BOOL *self )
{
    NTSTATUS ret;

    SERVER_START_REQ( set_thread_context )
    {
        req->handle  = wine_server_obj_handle( handle );
        wine_server_add_data( req, context, sizeof(*context) );
        ret = wine_server_call( req );
        *self = reply->self;
    }
    SERVER_END_REQ;

    return ret;
}


/***********************************************************************
 *              get_thread_context
 */
NTSTATUS get_thread_context( HANDLE handle, context_t *context, unsigned int flags, BOOL *self )
{
    NTSTATUS ret;

    SERVER_START_REQ( get_thread_context )
    {
        req->handle  = wine_server_obj_handle( handle );
        req->flags   = flags;
        wine_server_set_reply( req, context, sizeof(*context) );
        ret = wine_server_call( req );
        *self = reply->self;
        handle = wine_server_ptr_handle( reply->handle );
    }
    SERVER_END_REQ;

    if (ret == STATUS_PENDING)
    {
        LARGE_INTEGER timeout;
        timeout.QuadPart = -1000000;
        if (NtWaitForSingleObject( handle, FALSE, &timeout ))
        {
            NtClose( handle );
            return STATUS_ACCESS_DENIED;
        }
        SERVER_START_REQ( get_thread_context )
        {
            req->handle  = wine_server_obj_handle( handle );
            req->flags   = flags;
            wine_server_set_reply( req, context, sizeof(*context) );
            ret = wine_server_call( req );
        }
        SERVER_END_REQ;
    }
    return ret;
}


/***********************************************************************
 *              NtSetContextThread  (NTDLL.@)
 *              ZwSetContextThread  (NTDLL.@)
 */
NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    return unix_funcs->NtSetContextThread( handle, context );
}


/***********************************************************************
 *              NtGetContextThread  (NTDLL.@)
 *              ZwGetContextThread  (NTDLL.@)
 */
#ifndef __i386__
NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    return unix_funcs->NtGetContextThread( handle, context );
}
#endif


/******************************************************************************
 *           NtSetLdtEntries   (NTDLL.@)
 *           ZwSetLdtEntries   (NTDLL.@)
 */
NTSTATUS WINAPI NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    return unix_funcs->NtSetLdtEntries( sel1, entry1, sel2, entry2 );
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
            const ULONG_PTR affinity_mask = get_system_affinity_mask();

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->tid_in = 0;
                if (!(status = wine_server_call( req )))
                {
                    info.ExitStatus             = reply->exit_code;
                    info.TebBaseAddress         = wine_server_get_ptr( reply->teb );
                    info.ClientId.UniqueProcess = ULongToHandle(reply->pid);
                    info.ClientId.UniqueThread  = ULongToHandle(reply->tid);
                    info.AffinityMask           = reply->affinity & affinity_mask;
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
    case ThreadAffinityMask:
        {
            const ULONG_PTR affinity_mask = get_system_affinity_mask();
            ULONG_PTR affinity = 0;

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->tid_in = 0;
                if (!(status = wine_server_call( req )))
                    affinity = reply->affinity & affinity_mask;
            }
            SERVER_END_REQ;
            if (status == STATUS_SUCCESS)
            {
                if (data) memcpy( data, &affinity, min( length, sizeof(affinity) ));
                if (ret_len) *ret_len = min( length, sizeof(affinity) );
            }
        }
        return status;
    case ThreadTimes:
        {
            KERNEL_USER_TIMES   kusrt;

            SERVER_START_REQ( get_thread_times )
            {
                req->handle = wine_server_obj_handle( handle );
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    kusrt.CreateTime.QuadPart = reply->creation_time;
                    kusrt.ExitTime.QuadPart = reply->exit_time;
                }
            }
            SERVER_END_REQ;
            if (status == STATUS_SUCCESS)
            {
                /* We call times(2) for kernel time or user time */
                /* We can only (portably) do this for the current thread */
                if (handle == GetCurrentThread())
                {
                    struct tms time_buf;
                    long clocks_per_sec = sysconf(_SC_CLK_TCK);

                    times(&time_buf);
                    kusrt.KernelTime.QuadPart = (ULONGLONG)time_buf.tms_stime * 10000000 / clocks_per_sec;
                    kusrt.UserTime.QuadPart = (ULONGLONG)time_buf.tms_utime * 10000000 / clocks_per_sec;
                }
                else
                {
                    static BOOL reported = FALSE;

                    kusrt.KernelTime.QuadPart = 0;
                    kusrt.UserTime.QuadPart = 0;
                    if (reported)
                        TRACE("Cannot get kerneltime or usertime of other threads\n");
                    else
                    {
                        FIXME("Cannot get kerneltime or usertime of other threads\n");
                        reported = TRUE;
                    }
                }
                if (data) memcpy( data, &kusrt, min( length, sizeof(kusrt) ));
                if (ret_len) *ret_len = min( length, sizeof(kusrt) );
            }
        }
        return status;

    case ThreadDescriptorTableEntry:
        return unix_funcs->get_thread_ldt_entry( handle, data, length, ret_len );

    case ThreadAmILastThread:
        {
            SERVER_START_REQ(get_thread_info)
            {
                req->handle = wine_server_obj_handle( handle );
                req->tid_in = 0;
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    BOOLEAN last = reply->last;
                    if (data) memcpy( data, &last, min( length, sizeof(last) ));
                    if (ret_len) *ret_len = min( length, sizeof(last) );
                }
            }
            SERVER_END_REQ;
            return status;
        }
    case ThreadQuerySetWin32StartAddress:
        {
            SERVER_START_REQ( get_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->tid_in = 0;
                status = wine_server_call( req );
                if (status == STATUS_SUCCESS)
                {
                    PRTL_THREAD_START_ROUTINE entry = wine_server_get_ptr( reply->entry_point );
                    if (data) memcpy( data, &entry, min( length, sizeof(entry) ) );
                    if (ret_len) *ret_len = min( length, sizeof(entry) );
                }
            }
            SERVER_END_REQ;
            return status;
        }
    case ThreadGroupInformation:
        {
            const ULONG_PTR affinity_mask = get_system_affinity_mask();
            GROUP_AFFINITY affinity;

            memset(&affinity, 0, sizeof(affinity));
            affinity.Group = 0; /* Wine only supports max 64 processors */

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->tid_in = 0;
                if (!(status = wine_server_call( req )))
                    affinity.Mask = reply->affinity & affinity_mask;
            }
            SERVER_END_REQ;
            if (status == STATUS_SUCCESS)
            {
                if (data) memcpy( data, &affinity, min( length, sizeof(affinity) ));
                if (ret_len) *ret_len = min( length, sizeof(affinity) );
            }
        }
        return status;
    case ThreadIsIoPending:
        FIXME( "ThreadIsIoPending info class not supported yet\n" );
        if (length != sizeof(BOOL)) return STATUS_INFO_LENGTH_MISMATCH;
        if (!data) return STATUS_ACCESS_DENIED;

        *(BOOL*)data = FALSE;
        if (ret_len) *ret_len = sizeof(BOOL);
        return STATUS_SUCCESS;
    case ThreadSuspendCount:
        {
            ULONG count = 0;

            if (length != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;
            if (!data) return STATUS_ACCESS_VIOLATION;

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->tid_in = 0;
                if (!(status = wine_server_call( req )))
                    count = reply->suspend_count;
            }
            SERVER_END_REQ;

            if (!status)
                *(ULONG *)data = count;

            return status;
        }
    case ThreadDescription:
        {
            THREAD_DESCRIPTION_INFORMATION *info = data;
            data_size_t len, desc_len = 0;
            WCHAR *ptr;

            len = length >= sizeof(*info) ? length - sizeof(*info) : 0;
            ptr = info ? (WCHAR *)(info + 1) : NULL;

            SERVER_START_REQ( get_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                if (ptr) wine_server_set_reply( req, ptr, len );
                status = wine_server_call( req );
                desc_len = reply->desc_len;
            }
            SERVER_END_REQ;

            if (!info)
                status = STATUS_BUFFER_TOO_SMALL;
            else if (status == STATUS_SUCCESS)
            {
                info->Description.Length = info->Description.MaximumLength = desc_len;
                info->Description.Buffer = ptr;
            }

            if (ret_len && (status == STATUS_SUCCESS || status == STATUS_BUFFER_TOO_SMALL))
                *ret_len = sizeof(*info) + desc_len;
        }
        return status;
    case ThreadPriority:
    case ThreadBasePriority:
    case ThreadImpersonationToken:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
    case ThreadZeroTlsCell:
    case ThreadPerformanceCount:
    case ThreadIdealProcessor:
    case ThreadPriorityBoost:
    case ThreadSetTlsArrayAddress:
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
    NTSTATUS status;
    switch(class)
    {
    case ThreadZeroTlsCell:
        if (handle == GetCurrentThread())
        {
            LIST_ENTRY *entry;
            DWORD index;

            if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
            index = *(const DWORD *)data;
            if (index < TLS_MINIMUM_AVAILABLE)
            {
                RtlAcquirePebLock();
                for (entry = tls_links.Flink; entry != &tls_links; entry = entry->Flink)
                {
                    TEB *teb = CONTAINING_RECORD(entry, TEB, TlsLinks);
                    teb->TlsSlots[index] = 0;
                }
                RtlReleasePebLock();
            }
            else
            {
                index -= TLS_MINIMUM_AVAILABLE;
                if (index >= 8 * sizeof(NtCurrentTeb()->Peb->TlsExpansionBitmapBits))
                    return STATUS_INVALID_PARAMETER;
                RtlAcquirePebLock();
                for (entry = tls_links.Flink; entry != &tls_links; entry = entry->Flink)
                {
                    TEB *teb = CONTAINING_RECORD(entry, TEB, TlsLinks);
                    if (teb->TlsExpansionSlots) teb->TlsExpansionSlots[index] = 0;
                }
                RtlReleasePebLock();
            }
            return STATUS_SUCCESS;
        }
        FIXME( "ZeroTlsCell not supported on other threads\n" );
        return STATUS_NOT_IMPLEMENTED;

    case ThreadImpersonationToken:
        {
            const HANDLE *phToken = data;
            if (length != sizeof(HANDLE)) return STATUS_INVALID_PARAMETER;
            TRACE("Setting ThreadImpersonationToken handle to %p\n", *phToken );
            SERVER_START_REQ( set_thread_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->token    = wine_server_obj_handle( *phToken );
                req->mask     = SET_THREAD_INFO_TOKEN;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        return status;
    case ThreadBasePriority:
        {
            const DWORD *pprio = data;
            if (length != sizeof(DWORD)) return STATUS_INVALID_PARAMETER;
            SERVER_START_REQ( set_thread_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->priority = *pprio;
                req->mask     = SET_THREAD_INFO_PRIORITY;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        return status;
    case ThreadAffinityMask:
        {
            const ULONG_PTR affinity_mask = get_system_affinity_mask();
            ULONG_PTR req_aff;

            if (length != sizeof(ULONG_PTR)) return STATUS_INVALID_PARAMETER;
            req_aff = *(const ULONG_PTR *)data & affinity_mask;
            if (!req_aff) return STATUS_INVALID_PARAMETER;

            SERVER_START_REQ( set_thread_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->affinity = req_aff;
                req->mask     = SET_THREAD_INFO_AFFINITY;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        return status;
    case ThreadHideFromDebugger:
        /* pretend the call succeeded to satisfy some code protectors */
        return STATUS_SUCCESS;
    case ThreadQuerySetWin32StartAddress:
        {
            const PRTL_THREAD_START_ROUTINE *entry = data;
            if (length != sizeof(PRTL_THREAD_START_ROUTINE)) return STATUS_INVALID_PARAMETER;
            SERVER_START_REQ( set_thread_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->mask     = SET_THREAD_INFO_ENTRYPOINT;
                req->entry_point = wine_server_client_ptr( *entry );
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        return status;
    case ThreadGroupInformation:
        {
            const ULONG_PTR affinity_mask = get_system_affinity_mask();
            const GROUP_AFFINITY *req_aff;

            if (length != sizeof(*req_aff)) return STATUS_INVALID_PARAMETER;
            if (!data) return STATUS_ACCESS_VIOLATION;
            req_aff = data;

            /* On Windows the request fails if the reserved fields are set */
            if (req_aff->Reserved[0] || req_aff->Reserved[1] || req_aff->Reserved[2])
                return STATUS_INVALID_PARAMETER;

            /* Wine only supports max 64 processors */
            if (req_aff->Group) return STATUS_INVALID_PARAMETER;
            if (req_aff->Mask & ~affinity_mask) return STATUS_INVALID_PARAMETER;
            if (!req_aff->Mask) return STATUS_INVALID_PARAMETER;
            SERVER_START_REQ( set_thread_info )
            {
                req->handle   = wine_server_obj_handle( handle );
                req->affinity = req_aff->Mask;
                req->mask     = SET_THREAD_INFO_AFFINITY;
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        return status;
    case ThreadDescription:
        {
            const THREAD_DESCRIPTION_INFORMATION *info = data;

            if (length != sizeof(*info)) return STATUS_INFO_LENGTH_MISMATCH;
            if (!info) return STATUS_ACCESS_VIOLATION;

            if (info->Description.Length != info->Description.MaximumLength) return STATUS_INVALID_PARAMETER;
            if (info->Description.Length && !info->Description.Buffer) return STATUS_ACCESS_VIOLATION;

            SERVER_START_REQ( set_thread_info )
            {
                req->handle = wine_server_obj_handle( handle );
                req->mask   = SET_THREAD_INFO_DESCRIPTION;
                wine_server_add_data( req, info->Description.Buffer, info->Description.Length );
                status = wine_server_call( req );
            }
            SERVER_END_REQ;
        }
        return status;
    case ThreadBasicInformation:
    case ThreadTimes:
    case ThreadPriority:
    case ThreadDescriptorTableEntry:
    case ThreadEnableAlignmentFaultFixup:
    case ThreadEventPair_Reusable:
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
 * NtGetCurrentProcessorNumber (NTDLL.@)
 *
 * Return the processor, on which the thread is running
 *
 */
ULONG WINAPI NtGetCurrentProcessorNumber(void)
{
    ULONG processor;

#if defined(__linux__) && defined(__NR_getcpu)
    int res = syscall(__NR_getcpu, &processor, NULL, NULL);
    if (res != -1) return processor;
#endif

    if (NtCurrentTeb()->Peb->NumberOfProcessors > 1)
    {
        ULONG_PTR thread_mask, processor_mask;
        NTSTATUS status;

        status = NtQueryInformationThread(GetCurrentThread(), ThreadAffinityMask,
                                          &thread_mask, sizeof(thread_mask), NULL);
        if (status == STATUS_SUCCESS)
        {
            for (processor = 0; processor < NtCurrentTeb()->Peb->NumberOfProcessors; processor++)
            {
                processor_mask = (1 << processor);
                if (thread_mask & processor_mask)
                {
                    if (thread_mask != processor_mask)
                        FIXME("need multicore support (%d processors)\n",
                              NtCurrentTeb()->Peb->NumberOfProcessors);
                    return processor;
                }
            }
        }
    }

    /* fallback to the first processor */
    return 0;
}
