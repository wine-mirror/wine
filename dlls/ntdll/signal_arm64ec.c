/*
 * ARM64EC signal handling routines
 *
 * Copyright 1999, 2005, 2023 Alexandre Julliard
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

#ifdef __arm64ec__

#include <stdlib.h>
#include <stdarg.h>
#include <setjmp.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winternl.h"
#include "wine/exception.h"
#include "wine/list.h"
#include "ntdll_misc.h"
#include "unwind.h"
#include "wine/debug.h"
#include "ntsyscalls.h"

WINE_DEFAULT_DEBUG_CHANNEL(seh);
WINE_DECLARE_DEBUG_CHANNEL(relay);


static inline CHPE_V2_CPU_AREA_INFO *get_arm64ec_cpu_area(void)
{
    return NtCurrentTeb()->ChpeV2CpuAreaInfo;
}

static inline BOOL is_valid_arm64ec_frame( ULONG_PTR frame )
{
    if (frame & (sizeof(void*) - 1)) return FALSE;
    if (is_valid_frame( frame )) return TRUE;
    return (frame >= get_arm64ec_cpu_area()->EmulatorStackLimit &&
            frame <= get_arm64ec_cpu_area()->EmulatorStackBase);
}

/*******************************************************************
 *         syscalls
 */
enum syscall_ids
{
#define SYSCALL_ENTRY(id,name,args) __id_##name = id,
ALL_SYSCALLS64
#undef SYSCALL_ENTRY
};

#define SYSCALL_API __attribute__((naked))
#define SYSCALL_FUNC(name) __ASM_SYSCALL_FUNC( __id_##name, name )

NTSTATUS SYSCALL_API NtAcceptConnectPort( HANDLE *handle, ULONG id, LPC_MESSAGE *msg, BOOLEAN accept,
                                          LPC_SECTION_WRITE *write, LPC_SECTION_READ *read )
{
    SYSCALL_FUNC( NtAcceptConnectPort );
}

NTSTATUS SYSCALL_API NtAccessCheck( PSECURITY_DESCRIPTOR descr, HANDLE token, ACCESS_MASK access,
                                    GENERIC_MAPPING *mapping, PRIVILEGE_SET *privs, ULONG *retlen,
                                    ULONG *access_granted, NTSTATUS *access_status )
{
    SYSCALL_FUNC( NtAccessCheck );
}

NTSTATUS SYSCALL_API NtAccessCheckAndAuditAlarm( UNICODE_STRING *subsystem, HANDLE handle,
                                                 UNICODE_STRING *typename, UNICODE_STRING *objectname,
                                                 PSECURITY_DESCRIPTOR descr, ACCESS_MASK access,
                                                 GENERIC_MAPPING *mapping, BOOLEAN creation,
                                                 ACCESS_MASK *access_granted, BOOLEAN *access_status,
                                                 BOOLEAN *onclose )
{
    SYSCALL_FUNC( NtAccessCheckAndAuditAlarm );
}

NTSTATUS SYSCALL_API NtAddAtom( const WCHAR *name, ULONG length, RTL_ATOM *atom )
{
    SYSCALL_FUNC( NtAddAtom );
}

NTSTATUS SYSCALL_API NtAdjustGroupsToken( HANDLE token, BOOLEAN reset, TOKEN_GROUPS *groups,
                                          ULONG length, TOKEN_GROUPS *prev, ULONG *retlen )
{
    SYSCALL_FUNC( NtAdjustGroupsToken );
}

NTSTATUS SYSCALL_API NtAdjustPrivilegesToken( HANDLE token, BOOLEAN disable, TOKEN_PRIVILEGES *privs,
                                              DWORD length, TOKEN_PRIVILEGES *prev, DWORD *retlen )
{
    SYSCALL_FUNC( NtAdjustPrivilegesToken );
}

NTSTATUS SYSCALL_API NtAlertResumeThread( HANDLE handle, ULONG *count )
{
    SYSCALL_FUNC( NtAlertResumeThread );
}

NTSTATUS SYSCALL_API NtAlertThread( HANDLE handle )
{
    SYSCALL_FUNC( NtAlertThread );
}

NTSTATUS SYSCALL_API NtAlertThreadByThreadId( HANDLE tid )
{
    SYSCALL_FUNC( NtAlertThreadByThreadId );
}

NTSTATUS SYSCALL_API NtAllocateLocallyUniqueId( LUID *luid )
{
    SYSCALL_FUNC( NtAllocateLocallyUniqueId );
}

NTSTATUS SYSCALL_API NtAllocateUuids( ULARGE_INTEGER *time, ULONG *delta, ULONG *sequence, UCHAR *seed )
{
    SYSCALL_FUNC( NtAllocateUuids );
}

NTSTATUS SYSCALL_API NtAllocateVirtualMemory( HANDLE process, PVOID *ret, ULONG_PTR zero_bits,
                                              SIZE_T *size_ptr, ULONG type, ULONG protect )
{
    SYSCALL_FUNC( NtAllocateVirtualMemory );
}

NTSTATUS SYSCALL_API NtAllocateVirtualMemoryEx( HANDLE process, PVOID *ret, SIZE_T *size_ptr, ULONG type,
                                                ULONG protect, MEM_EXTENDED_PARAMETER *parameters,
                                                ULONG count )
{
    SYSCALL_FUNC( NtAllocateVirtualMemoryEx );
}

NTSTATUS SYSCALL_API NtAreMappedFilesTheSame(PVOID addr1, PVOID addr2)
{
    SYSCALL_FUNC( NtAreMappedFilesTheSame );
}

NTSTATUS SYSCALL_API NtAssignProcessToJobObject( HANDLE job, HANDLE process )
{
    SYSCALL_FUNC( NtAssignProcessToJobObject );
}

NTSTATUS SYSCALL_API NtCallbackReturn( void *ret_ptr, ULONG ret_len, NTSTATUS status )
{
    SYSCALL_FUNC( NtCallbackReturn );
}

NTSTATUS SYSCALL_API NtCancelIoFile( HANDLE handle, IO_STATUS_BLOCK *io_status )
{
    SYSCALL_FUNC( NtCancelIoFile );
}

NTSTATUS SYSCALL_API NtCancelIoFileEx( HANDLE handle, IO_STATUS_BLOCK *io, IO_STATUS_BLOCK *io_status )
{
    SYSCALL_FUNC( NtCancelIoFileEx );
}

NTSTATUS SYSCALL_API NtCancelSynchronousIoFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                                IO_STATUS_BLOCK *io_status )
{
    SYSCALL_FUNC( NtCancelSynchronousIoFile );
}

NTSTATUS SYSCALL_API NtCancelTimer( HANDLE handle, BOOLEAN *state )
{
    SYSCALL_FUNC( NtCancelTimer );
}

NTSTATUS SYSCALL_API NtClearEvent( HANDLE handle )
{
    SYSCALL_FUNC( NtClearEvent );
}

NTSTATUS SYSCALL_API NtClose( HANDLE handle )
{
    SYSCALL_FUNC( NtClose );
}

NTSTATUS SYSCALL_API NtCommitTransaction( HANDLE transaction, BOOLEAN wait )
{
    SYSCALL_FUNC( NtCommitTransaction );
}

NTSTATUS SYSCALL_API NtCompareObjects( HANDLE first, HANDLE second )
{
    SYSCALL_FUNC( NtCompareObjects );
}

NTSTATUS SYSCALL_API NtCompareTokens( HANDLE first, HANDLE second, BOOLEAN *equal )
{
    SYSCALL_FUNC( NtCompareTokens );
}

NTSTATUS SYSCALL_API NtCompleteConnectPort( HANDLE handle )
{
    SYSCALL_FUNC( NtCompleteConnectPort );
}

NTSTATUS SYSCALL_API NtConnectPort( HANDLE *handle, UNICODE_STRING *name, SECURITY_QUALITY_OF_SERVICE *qos,
                                    LPC_SECTION_WRITE *write, LPC_SECTION_READ *read, ULONG *max_len,
                                    void *info, ULONG *info_len )
{
    SYSCALL_FUNC( NtConnectPort );
}

static NTSTATUS SYSCALL_API syscall_NtContinue( ARM64_NT_CONTEXT *context, BOOLEAN alertable )
{
    __ASM_SYSCALL_FUNC( __id_NtContinue, syscall_NtContinue );
}

NTSTATUS WINAPI NtContinue( CONTEXT *context, BOOLEAN alertable )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtContinue( &arm_ctx, alertable );
}

NTSTATUS SYSCALL_API NtCreateDebugObject( HANDLE *handle, ACCESS_MASK access,
                                          OBJECT_ATTRIBUTES *attr, ULONG flags )
{
    SYSCALL_FUNC( NtCreateDebugObject );
}

NTSTATUS SYSCALL_API NtCreateDirectoryObject( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtCreateDirectoryObject );
}

NTSTATUS SYSCALL_API NtCreateEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                    EVENT_TYPE type, BOOLEAN state )
{
    SYSCALL_FUNC( NtCreateEvent );
}

NTSTATUS SYSCALL_API NtCreateFile( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                   IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size,
                                   ULONG attributes, ULONG sharing, ULONG disposition,
                                   ULONG options, void *ea_buffer, ULONG ea_length )
{
    SYSCALL_FUNC( NtCreateFile );
}

NTSTATUS SYSCALL_API NtCreateIoCompletion( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                           ULONG threads )
{
    SYSCALL_FUNC( NtCreateIoCompletion );
}

NTSTATUS SYSCALL_API NtCreateJobObject( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtCreateJobObject );
}

NTSTATUS SYSCALL_API NtCreateKey( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                  ULONG index, const UNICODE_STRING *class, ULONG options, ULONG *dispos )
{
    SYSCALL_FUNC( NtCreateKey );
}

NTSTATUS SYSCALL_API NtCreateKeyTransacted( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                            ULONG index, const UNICODE_STRING *class, ULONG options,
                                            HANDLE transacted, ULONG *dispos )
{
    SYSCALL_FUNC( NtCreateKeyTransacted );
}

NTSTATUS SYSCALL_API NtCreateKeyedEvent( HANDLE *handle, ACCESS_MASK access,
                                         const OBJECT_ATTRIBUTES *attr, ULONG flags )
{
    SYSCALL_FUNC( NtCreateKeyedEvent );
}

NTSTATUS SYSCALL_API NtCreateLowBoxToken( HANDLE *token_handle, HANDLE token, ACCESS_MASK access,
                                          OBJECT_ATTRIBUTES *attr, SID *sid, ULONG count,
                                          SID_AND_ATTRIBUTES *capabilities, ULONG handle_count,
                                          HANDLE *handle )
{
    SYSCALL_FUNC( NtCreateLowBoxToken );
}

NTSTATUS SYSCALL_API NtCreateMailslotFile( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                           IO_STATUS_BLOCK *io, ULONG options, ULONG quota, ULONG msg_size,
                                           LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtCreateMailslotFile );
}

NTSTATUS SYSCALL_API NtCreateMutant( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                     BOOLEAN owned )
{
    SYSCALL_FUNC( NtCreateMutant );
}

NTSTATUS SYSCALL_API NtCreateNamedPipeFile( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                            IO_STATUS_BLOCK *io, ULONG sharing, ULONG dispo, ULONG options,
                                            ULONG pipe_type, ULONG read_mode, ULONG completion_mode,
                                            ULONG max_inst, ULONG inbound_quota, ULONG outbound_quota,
                                            LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtCreateNamedPipeFile );
}

NTSTATUS SYSCALL_API NtCreatePagingFile( UNICODE_STRING *name, LARGE_INTEGER *min_size,
                                         LARGE_INTEGER *max_size, LARGE_INTEGER *actual_size )
{
    SYSCALL_FUNC( NtCreatePagingFile );
}

NTSTATUS SYSCALL_API NtCreatePort( HANDLE *handle, OBJECT_ATTRIBUTES *attr, ULONG info_len,
                                   ULONG data_len, ULONG *reserved )
{
    SYSCALL_FUNC( NtCreatePort );
}

NTSTATUS SYSCALL_API NtCreateSection( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                      const LARGE_INTEGER *size, ULONG protect,
                                      ULONG sec_flags, HANDLE file )
{
    SYSCALL_FUNC( NtCreateSection );
}

NTSTATUS SYSCALL_API NtCreateSemaphore( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                        LONG initial, LONG max )
{
    SYSCALL_FUNC( NtCreateSemaphore );
}

NTSTATUS SYSCALL_API NtCreateSymbolicLinkObject( HANDLE *handle, ACCESS_MASK access,
                                                 OBJECT_ATTRIBUTES *attr, UNICODE_STRING *target )
{
    SYSCALL_FUNC( NtCreateSymbolicLinkObject );
}

NTSTATUS SYSCALL_API NtCreateThread( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                     HANDLE process, CLIENT_ID *id, CONTEXT *ctx, INITIAL_TEB *teb,
                                     BOOLEAN suspended )
{
    SYSCALL_FUNC( NtCreateThread );
}

NTSTATUS SYSCALL_API NtCreateThreadEx( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                       HANDLE process, PRTL_THREAD_START_ROUTINE start, void *param,
                                       ULONG flags, ULONG_PTR zero_bits, SIZE_T stack_commit,
                                       SIZE_T stack_reserve, PS_ATTRIBUTE_LIST *attr_list )
{
    SYSCALL_FUNC( NtCreateThreadEx );
}

NTSTATUS SYSCALL_API NtCreateTimer( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                    TIMER_TYPE type )
{
    SYSCALL_FUNC( NtCreateTimer );
}

NTSTATUS SYSCALL_API NtCreateToken( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                    TOKEN_TYPE type, LUID *token_id, LARGE_INTEGER *expire,
                                    TOKEN_USER *user, TOKEN_GROUPS *groups, TOKEN_PRIVILEGES *privs,
                                    TOKEN_OWNER *owner, TOKEN_PRIMARY_GROUP *group,
                                    TOKEN_DEFAULT_DACL *dacl, TOKEN_SOURCE *source )
{
    SYSCALL_FUNC( NtCreateToken );
}

NTSTATUS SYSCALL_API NtCreateTransaction( HANDLE *handle, ACCESS_MASK mask, OBJECT_ATTRIBUTES *obj_attr,
                                          GUID *guid, HANDLE tm, ULONG options, ULONG isol_level,
                                          ULONG isol_flags, PLARGE_INTEGER timeout,
                                          UNICODE_STRING *description )
{
    SYSCALL_FUNC( NtCreateTransaction );
}

NTSTATUS SYSCALL_API NtCreateUserProcess( HANDLE *process_handle_ptr, HANDLE *thread_handle_ptr,
                                          ACCESS_MASK process_access, ACCESS_MASK thread_access,
                                          OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr,
                                          ULONG process_flags, ULONG thread_flags,
                                          RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info,
                                          PS_ATTRIBUTE_LIST *ps_attr )
{
    SYSCALL_FUNC( NtCreateUserProcess );
}

NTSTATUS SYSCALL_API NtDebugActiveProcess( HANDLE process, HANDLE debug )
{
    SYSCALL_FUNC( NtDebugActiveProcess );
}

NTSTATUS SYSCALL_API NtDebugContinue( HANDLE handle, CLIENT_ID *client, NTSTATUS status )
{
    SYSCALL_FUNC( NtDebugContinue );
}

NTSTATUS SYSCALL_API NtDelayExecution( BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtDelayExecution );
}

NTSTATUS SYSCALL_API NtDeleteAtom( RTL_ATOM atom )
{
    SYSCALL_FUNC( NtDeleteAtom );
}

NTSTATUS SYSCALL_API NtDeleteFile( OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtDeleteFile );
}

NTSTATUS SYSCALL_API NtDeleteKey( HANDLE key )
{
    SYSCALL_FUNC( NtDeleteKey );
}

NTSTATUS SYSCALL_API NtDeleteValueKey( HANDLE key, const UNICODE_STRING *name )
{
    SYSCALL_FUNC( NtDeleteValueKey );
}

NTSTATUS SYSCALL_API NtDeviceIoControlFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                            void *apc_context, IO_STATUS_BLOCK *io, ULONG code,
                                            void *in_buffer, ULONG in_size,
                                            void *out_buffer, ULONG out_size )
{
    SYSCALL_FUNC( NtDeviceIoControlFile );
}

NTSTATUS SYSCALL_API NtDisplayString( UNICODE_STRING *string )
{
    SYSCALL_FUNC( NtDisplayString );
}

NTSTATUS SYSCALL_API NtDuplicateObject( HANDLE source_process, HANDLE source, HANDLE dest_process,
                                        HANDLE *dest, ACCESS_MASK access, ULONG attributes, ULONG options )
{
    SYSCALL_FUNC( NtDuplicateObject );
}

NTSTATUS SYSCALL_API NtDuplicateToken( HANDLE token, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                       BOOLEAN effective_only, TOKEN_TYPE type, HANDLE *handle )
{
    SYSCALL_FUNC( NtDuplicateToken );
}

NTSTATUS SYSCALL_API NtEnumerateKey( HANDLE handle, ULONG index, KEY_INFORMATION_CLASS info_class,
                                     void *info, DWORD length, DWORD *result_len )
{
    SYSCALL_FUNC( NtEnumerateKey );
}

NTSTATUS SYSCALL_API NtEnumerateValueKey( HANDLE handle, ULONG index, KEY_VALUE_INFORMATION_CLASS info_class,
                                          void *info, DWORD length, DWORD *result_len )
{
    SYSCALL_FUNC( NtEnumerateValueKey );
}

NTSTATUS SYSCALL_API NtFilterToken( HANDLE token, ULONG flags, TOKEN_GROUPS *disable_sids,
                                    TOKEN_PRIVILEGES *privileges, TOKEN_GROUPS *restrict_sids,
                                    HANDLE *new_token )
{
    SYSCALL_FUNC( NtFilterToken );
}

NTSTATUS SYSCALL_API NtFindAtom( const WCHAR *name, ULONG length, RTL_ATOM *atom )
{
    SYSCALL_FUNC( NtFindAtom );
}

NTSTATUS SYSCALL_API NtFlushBuffersFile( HANDLE handle, IO_STATUS_BLOCK *io )
{
    SYSCALL_FUNC( NtFlushBuffersFile );
}

NTSTATUS SYSCALL_API NtFlushInstructionCache( HANDLE handle, const void *addr, SIZE_T size )
{
    SYSCALL_FUNC( NtFlushInstructionCache );
}

NTSTATUS SYSCALL_API NtFlushKey( HANDLE key )
{
    SYSCALL_FUNC( NtFlushKey );
}

NTSTATUS SYSCALL_API NtFlushProcessWriteBuffers(void)
{
    SYSCALL_FUNC( NtFlushProcessWriteBuffers );
}

NTSTATUS SYSCALL_API NtFlushVirtualMemory( HANDLE process, LPCVOID *addr_ptr,
                                           SIZE_T *size_ptr, ULONG unknown )
{
    SYSCALL_FUNC( NtFlushVirtualMemory );
}

NTSTATUS SYSCALL_API NtFreeVirtualMemory( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr, ULONG type )
{
    SYSCALL_FUNC( NtFreeVirtualMemory );
}

NTSTATUS SYSCALL_API NtFsControlFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                      IO_STATUS_BLOCK *io, ULONG code, void *in_buffer, ULONG in_size,
                                      void *out_buffer, ULONG out_size )
{
    SYSCALL_FUNC( NtFsControlFile );
}

static NTSTATUS SYSCALL_API syscall_NtGetContextThread( HANDLE handle, ARM64_NT_CONTEXT *context )
{
    __ASM_SYSCALL_FUNC( __id_NtGetContextThread, syscall_NtGetContextThread );
}

NTSTATUS WINAPI NtGetContextThread( HANDLE handle, CONTEXT *context )
{
    ARM64_NT_CONTEXT arm_ctx = { .ContextFlags = ctx_flags_x64_to_arm( context->ContextFlags ) };
    NTSTATUS status = syscall_NtGetContextThread( handle, &arm_ctx );

    if (!status) context_arm_to_x64( (ARM64EC_NT_CONTEXT *)context, &arm_ctx );
    return status;
}

ULONG SYSCALL_API NtGetCurrentProcessorNumber(void)
{
    SYSCALL_FUNC( NtGetCurrentProcessorNumber );
}

NTSTATUS SYSCALL_API NtGetNextThread( HANDLE process, HANDLE thread, ACCESS_MASK access, ULONG attributes,
                                      ULONG flags, HANDLE *handle )
{
    SYSCALL_FUNC( NtGetNextThread );
}

NTSTATUS SYSCALL_API NtGetNlsSectionPtr( ULONG type, ULONG id, void *unknown, void **ptr, SIZE_T *size )
{
    SYSCALL_FUNC( NtGetNlsSectionPtr );
}

NTSTATUS SYSCALL_API NtGetWriteWatch( HANDLE process, ULONG flags, PVOID base, SIZE_T size,
                                      PVOID *addresses, ULONG_PTR *count, ULONG *granularity )
{
    SYSCALL_FUNC( NtGetWriteWatch );
}

NTSTATUS SYSCALL_API NtImpersonateAnonymousToken( HANDLE thread )
{
    SYSCALL_FUNC( NtImpersonateAnonymousToken );
}

NTSTATUS SYSCALL_API NtInitializeNlsFiles( void **ptr, LCID *lcid, LARGE_INTEGER *size )
{
    SYSCALL_FUNC( NtInitializeNlsFiles );
}

NTSTATUS SYSCALL_API NtInitiatePowerAction( POWER_ACTION action, SYSTEM_POWER_STATE state,
                                            ULONG flags, BOOLEAN async )
{
    SYSCALL_FUNC( NtInitiatePowerAction );
}

NTSTATUS SYSCALL_API NtIsProcessInJob( HANDLE process, HANDLE job )
{
    SYSCALL_FUNC( NtIsProcessInJob );
}

NTSTATUS SYSCALL_API NtListenPort( HANDLE handle, LPC_MESSAGE *msg )
{
    SYSCALL_FUNC( NtListenPort );
}

NTSTATUS SYSCALL_API NtLoadDriver( const UNICODE_STRING *name )
{
    SYSCALL_FUNC( NtLoadDriver );
}

NTSTATUS SYSCALL_API NtLoadKey( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file )
{
    SYSCALL_FUNC( NtLoadKey );
}

NTSTATUS SYSCALL_API NtLoadKey2( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file, ULONG flags )
{
    SYSCALL_FUNC( NtLoadKey2 );
}

NTSTATUS SYSCALL_API NtLoadKeyEx( const OBJECT_ATTRIBUTES *attr, OBJECT_ATTRIBUTES *file, ULONG flags,
                                  HANDLE trustkey, HANDLE event, ACCESS_MASK access,
                                  HANDLE *roothandle, IO_STATUS_BLOCK *iostatus )
{
    SYSCALL_FUNC( NtLoadKeyEx );
}

NTSTATUS SYSCALL_API NtLockFile( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void* apc_user,
                                 IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset,
                                 LARGE_INTEGER *count, ULONG *key, BOOLEAN dont_wait, BOOLEAN exclusive )
{
    SYSCALL_FUNC( NtLockFile );
}

NTSTATUS SYSCALL_API NtLockVirtualMemory( HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown )
{
    SYSCALL_FUNC( NtLockVirtualMemory );
}

NTSTATUS SYSCALL_API NtMakePermanentObject( HANDLE handle )
{
    SYSCALL_FUNC( NtMakePermanentObject );
}

NTSTATUS SYSCALL_API NtMakeTemporaryObject( HANDLE handle )
{
    SYSCALL_FUNC( NtMakeTemporaryObject );
}

NTSTATUS SYSCALL_API NtMapViewOfSection( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                         ULONG_PTR zero_bits, SIZE_T commit_size,
                                         const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                         SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect )
{
    SYSCALL_FUNC( NtMapViewOfSection );
}

NTSTATUS SYSCALL_API NtMapViewOfSectionEx( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                           const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                           ULONG alloc_type, ULONG protect,
                                           MEM_EXTENDED_PARAMETER *parameters, ULONG count )
{
    SYSCALL_FUNC( NtMapViewOfSectionEx );
}

NTSTATUS SYSCALL_API NtNotifyChangeDirectoryFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                                  void *apc_context, IO_STATUS_BLOCK *iosb, void *buffer,
                                                  ULONG buffer_size, ULONG filter, BOOLEAN subtree )
{
    SYSCALL_FUNC( NtNotifyChangeDirectoryFile );
}

NTSTATUS SYSCALL_API NtNotifyChangeKey( HANDLE key, HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                        IO_STATUS_BLOCK *io, ULONG filter, BOOLEAN subtree,
                                        void *buffer, ULONG length, BOOLEAN async )
{
    SYSCALL_FUNC( NtNotifyChangeKey );
}

NTSTATUS SYSCALL_API NtNotifyChangeMultipleKeys( HANDLE key, ULONG count, OBJECT_ATTRIBUTES *attr,
                                                 HANDLE event, PIO_APC_ROUTINE apc, void *apc_context,
                                                 IO_STATUS_BLOCK *io, ULONG filter, BOOLEAN subtree,
                                                 void *buffer, ULONG length, BOOLEAN async )
{
    SYSCALL_FUNC( NtNotifyChangeMultipleKeys );
}

NTSTATUS SYSCALL_API NtOpenDirectoryObject( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenDirectoryObject );
}

NTSTATUS SYSCALL_API NtOpenEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenEvent );
}

NTSTATUS SYSCALL_API NtOpenFile( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                 IO_STATUS_BLOCK *io, ULONG sharing, ULONG options )
{
    SYSCALL_FUNC( NtOpenFile );
}

NTSTATUS SYSCALL_API NtOpenIoCompletion( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenIoCompletion );
}

NTSTATUS SYSCALL_API NtOpenJobObject( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenJobObject );
}

NTSTATUS SYSCALL_API NtOpenKey( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenKey );
}

NTSTATUS SYSCALL_API NtOpenKeyEx( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, ULONG options )
{
    SYSCALL_FUNC( NtOpenKeyEx );
}

NTSTATUS SYSCALL_API NtOpenKeyTransacted( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                          HANDLE transaction )
{
    SYSCALL_FUNC( NtOpenKeyTransacted );
}

NTSTATUS SYSCALL_API NtOpenKeyTransactedEx( HANDLE *key, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr,
                                            ULONG options, HANDLE transaction )
{
    SYSCALL_FUNC( NtOpenKeyTransactedEx );
}

NTSTATUS SYSCALL_API NtOpenKeyedEvent( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenKeyedEvent );
}

NTSTATUS SYSCALL_API NtOpenMutant( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenMutant );
}

NTSTATUS SYSCALL_API NtOpenProcess( HANDLE *handle, ACCESS_MASK access,
                                    const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    SYSCALL_FUNC( NtOpenProcess );
}

NTSTATUS SYSCALL_API NtOpenProcessToken( HANDLE process, DWORD access, HANDLE *handle )
{
    SYSCALL_FUNC( NtOpenProcessToken );
}

NTSTATUS SYSCALL_API NtOpenProcessTokenEx( HANDLE process, DWORD access, DWORD attributes, HANDLE *handle )
{
    SYSCALL_FUNC( NtOpenProcessTokenEx );
}

NTSTATUS SYSCALL_API NtOpenSection( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenSection );
}

NTSTATUS SYSCALL_API NtOpenSemaphore( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenSemaphore );
}

NTSTATUS SYSCALL_API NtOpenSymbolicLinkObject( HANDLE *handle, ACCESS_MASK access,
                                               const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenSymbolicLinkObject );
}

NTSTATUS SYSCALL_API NtOpenThread( HANDLE *handle, ACCESS_MASK access,
                                   const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id )
{
    SYSCALL_FUNC( NtOpenThread );
}

NTSTATUS SYSCALL_API NtOpenThreadToken( HANDLE thread, DWORD access, BOOLEAN self, HANDLE *handle )
{
    SYSCALL_FUNC( NtOpenThreadToken );
}

NTSTATUS SYSCALL_API NtOpenThreadTokenEx( HANDLE thread, DWORD access, BOOLEAN self, DWORD attributes,
                                          HANDLE *handle )
{
    SYSCALL_FUNC( NtOpenThreadTokenEx );
}

NTSTATUS SYSCALL_API NtOpenTimer( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtOpenTimer );
}

NTSTATUS SYSCALL_API NtPowerInformation( POWER_INFORMATION_LEVEL level, void *input, ULONG in_size,
                                         void *output, ULONG out_size )
{
    SYSCALL_FUNC( NtPowerInformation );
}

NTSTATUS SYSCALL_API NtPrivilegeCheck( HANDLE token, PRIVILEGE_SET *privs, BOOLEAN *res )
{
    SYSCALL_FUNC( NtPrivilegeCheck );
}

NTSTATUS SYSCALL_API NtProtectVirtualMemory( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr,
                                             ULONG new_prot, ULONG *old_prot )
{
    SYSCALL_FUNC( NtProtectVirtualMemory );
}

NTSTATUS SYSCALL_API NtPulseEvent( HANDLE handle, LONG *prev_state )
{
    SYSCALL_FUNC( NtPulseEvent );
}

NTSTATUS SYSCALL_API NtQueryAttributesFile( const OBJECT_ATTRIBUTES *attr, FILE_BASIC_INFORMATION *info )
{
    SYSCALL_FUNC( NtQueryAttributesFile );
}

NTSTATUS SYSCALL_API NtQueryDefaultLocale( BOOLEAN user, LCID *lcid )
{
    SYSCALL_FUNC( NtQueryDefaultLocale );
}

NTSTATUS SYSCALL_API NtQueryDefaultUILanguage( LANGID *lang )
{
    SYSCALL_FUNC( NtQueryDefaultUILanguage );
}

NTSTATUS SYSCALL_API NtQueryDirectoryFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc_routine,
                                           void *apc_context, IO_STATUS_BLOCK *io, void *buffer,
                                           ULONG length, FILE_INFORMATION_CLASS info_class,
                                           BOOLEAN single_entry, UNICODE_STRING *mask,
                                           BOOLEAN restart_scan )
{
    SYSCALL_FUNC( NtQueryDirectoryFile );
}

NTSTATUS SYSCALL_API NtQueryDirectoryObject( HANDLE handle, DIRECTORY_BASIC_INFORMATION *buffer,
                                             ULONG size, BOOLEAN single_entry, BOOLEAN restart,
                                             ULONG *context, ULONG *ret_size )
{
    SYSCALL_FUNC( NtQueryDirectoryObject );
}

NTSTATUS SYSCALL_API NtQueryEaFile( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                    BOOLEAN single_entry, void *list, ULONG list_len,
                                    ULONG *index, BOOLEAN restart )
{
    SYSCALL_FUNC( NtQueryEaFile );
}

NTSTATUS SYSCALL_API NtQueryEvent( HANDLE handle, EVENT_INFORMATION_CLASS class,
                                   void *info, ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryEvent );
}

NTSTATUS SYSCALL_API NtQueryFullAttributesFile( const OBJECT_ATTRIBUTES *attr,
                                                FILE_NETWORK_OPEN_INFORMATION *info )
{
    SYSCALL_FUNC( NtQueryFullAttributesFile );
}

NTSTATUS SYSCALL_API NtQueryInformationAtom( RTL_ATOM atom, ATOM_INFORMATION_CLASS class,
                                             void *ptr, ULONG size, ULONG *retsize )
{
    SYSCALL_FUNC( NtQueryInformationAtom );
}

NTSTATUS SYSCALL_API NtQueryInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                             void *ptr, ULONG len, FILE_INFORMATION_CLASS class )
{
    SYSCALL_FUNC( NtQueryInformationFile );
}

NTSTATUS SYSCALL_API NtQueryInformationJobObject( HANDLE handle, JOBOBJECTINFOCLASS class, void *info,
                                                  ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryInformationJobObject );
}

NTSTATUS SYSCALL_API NtQueryInformationProcess( HANDLE handle, PROCESSINFOCLASS class, void *info,
                                                ULONG size, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryInformationProcess );
}

NTSTATUS SYSCALL_API NtQueryInformationThread( HANDLE handle, THREADINFOCLASS class,
                                               void *data, ULONG length, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryInformationThread );
}

NTSTATUS SYSCALL_API NtQueryInformationToken( HANDLE token, TOKEN_INFORMATION_CLASS class,
                                              void *info, ULONG length, ULONG *retlen )
{
    SYSCALL_FUNC( NtQueryInformationToken );
}

NTSTATUS SYSCALL_API NtQueryInstallUILanguage( LANGID *lang )
{
    SYSCALL_FUNC( NtQueryInstallUILanguage );
}

NTSTATUS SYSCALL_API NtQueryIoCompletion( HANDLE handle, IO_COMPLETION_INFORMATION_CLASS class,
                                          void *buffer, ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryIoCompletion );
}

NTSTATUS SYSCALL_API NtQueryKey( HANDLE handle, KEY_INFORMATION_CLASS info_class,
                                 void *info, DWORD length, DWORD *result_len )
{
    SYSCALL_FUNC( NtQueryKey );
}

NTSTATUS SYSCALL_API NtQueryLicenseValue( const UNICODE_STRING *name, ULONG *type,
                                          void *data, ULONG length, ULONG *retlen )
{
    SYSCALL_FUNC( NtQueryLicenseValue );
}

NTSTATUS SYSCALL_API NtQueryMultipleValueKey( HANDLE key, KEY_MULTIPLE_VALUE_INFORMATION *info,
                                              ULONG count, void *buffer, ULONG length, ULONG *retlen )
{
    SYSCALL_FUNC( NtQueryMultipleValueKey );
}

NTSTATUS SYSCALL_API NtQueryMutant( HANDLE handle, MUTANT_INFORMATION_CLASS class,
                                    void *info, ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryMutant );
}

NTSTATUS SYSCALL_API NtQueryObject( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                                    void *ptr, ULONG len, ULONG *used_len )
{
    SYSCALL_FUNC( NtQueryObject );
}

NTSTATUS SYSCALL_API NtQueryPerformanceCounter( LARGE_INTEGER *counter, LARGE_INTEGER *frequency )
{
    SYSCALL_FUNC( NtQueryPerformanceCounter );
}

NTSTATUS SYSCALL_API NtQuerySection( HANDLE handle, SECTION_INFORMATION_CLASS class, void *ptr,
                                     SIZE_T size, SIZE_T *ret_size )
{
    SYSCALL_FUNC( NtQuerySection );
}

NTSTATUS SYSCALL_API NtQuerySecurityObject( HANDLE handle, SECURITY_INFORMATION info,
                                            PSECURITY_DESCRIPTOR descr, ULONG length, ULONG *retlen )
{
    SYSCALL_FUNC( NtQuerySecurityObject );
}

NTSTATUS SYSCALL_API NtQuerySemaphore( HANDLE handle, SEMAPHORE_INFORMATION_CLASS class,
                                       void *info, ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQuerySemaphore );
}

NTSTATUS SYSCALL_API NtQuerySymbolicLinkObject( HANDLE handle, UNICODE_STRING *target, ULONG *length )
{
    SYSCALL_FUNC( NtQuerySymbolicLinkObject );
}

NTSTATUS SYSCALL_API NtQuerySystemEnvironmentValue( UNICODE_STRING *name, WCHAR *buffer, ULONG length,
                                                    ULONG *retlen )
{
    SYSCALL_FUNC( NtQuerySystemEnvironmentValue );
}

NTSTATUS SYSCALL_API NtQuerySystemEnvironmentValueEx( UNICODE_STRING *name, GUID *vendor, void *buffer,
                                                      ULONG *retlen, ULONG *attrib )
{
    SYSCALL_FUNC( NtQuerySystemEnvironmentValueEx );
}

NTSTATUS SYSCALL_API NtQuerySystemInformation( SYSTEM_INFORMATION_CLASS class,
                                               void *info, ULONG size, ULONG *ret_size )
{
    SYSCALL_FUNC( NtQuerySystemInformation );
}

NTSTATUS SYSCALL_API NtQuerySystemInformationEx( SYSTEM_INFORMATION_CLASS class, void *query,
                                                 ULONG query_len, void *info, ULONG size, ULONG *ret_size )
{
    SYSCALL_FUNC( NtQuerySystemInformationEx );
}

NTSTATUS SYSCALL_API NtQuerySystemTime( LARGE_INTEGER *time )
{
    SYSCALL_FUNC( NtQuerySystemTime );
}

NTSTATUS SYSCALL_API NtQueryTimer( HANDLE handle, TIMER_INFORMATION_CLASS class,
                                   void *info, ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtQueryTimer );
}

NTSTATUS SYSCALL_API NtQueryTimerResolution( ULONG *min_res, ULONG *max_res, ULONG *current_res )
{
    SYSCALL_FUNC( NtQueryTimerResolution );
}

NTSTATUS SYSCALL_API NtQueryValueKey( HANDLE handle, const UNICODE_STRING *name,
                                      KEY_VALUE_INFORMATION_CLASS info_class,
                                      void *info, DWORD length, DWORD *result_len )
{
    SYSCALL_FUNC( NtQueryValueKey );
}

NTSTATUS SYSCALL_API NtQueryVirtualMemory( HANDLE process, LPCVOID addr, MEMORY_INFORMATION_CLASS info_class,
                                           PVOID buffer, SIZE_T len, SIZE_T *res_len )
{
    SYSCALL_FUNC( NtQueryVirtualMemory );
}

NTSTATUS SYSCALL_API NtQueryVolumeInformationFile( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer,
                                                   ULONG length, FS_INFORMATION_CLASS info_class )
{
    SYSCALL_FUNC( NtQueryVolumeInformationFile );
}

NTSTATUS SYSCALL_API NtQueueApcThread( HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1,
                                       ULONG_PTR arg2, ULONG_PTR arg3 )
{
    SYSCALL_FUNC( NtQueueApcThread );
}

static NTSTATUS SYSCALL_API syscall_NtRaiseException( EXCEPTION_RECORD *rec, ARM64_NT_CONTEXT *context, BOOL first_chance )
{
    __ASM_SYSCALL_FUNC( __id_NtRaiseException, syscall_NtRaiseException );
}

NTSTATUS WINAPI NtRaiseException( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtRaiseException( rec, &arm_ctx, first_chance );
}

NTSTATUS SYSCALL_API NtRaiseHardError( NTSTATUS status, ULONG count, UNICODE_STRING *params_mask,
                                       void **params, HARDERROR_RESPONSE_OPTION option,
                                       HARDERROR_RESPONSE *response )
{
    SYSCALL_FUNC( NtRaiseHardError );
}

NTSTATUS SYSCALL_API NtReadFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                 IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                 LARGE_INTEGER *offset, ULONG *key )
{
    SYSCALL_FUNC( NtReadFile );
}

NTSTATUS SYSCALL_API NtReadFileScatter( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                        IO_STATUS_BLOCK *io, FILE_SEGMENT_ELEMENT *segments,
                                        ULONG length, LARGE_INTEGER *offset, ULONG *key )
{
    SYSCALL_FUNC( NtReadFileScatter );
}

NTSTATUS SYSCALL_API NtReadVirtualMemory( HANDLE process, const void *addr, void *buffer,
                                          SIZE_T size, SIZE_T *bytes_read )
{
    SYSCALL_FUNC( NtReadVirtualMemory );
}

NTSTATUS SYSCALL_API NtRegisterThreadTerminatePort( HANDLE handle )
{
    SYSCALL_FUNC( NtRegisterThreadTerminatePort );
}

NTSTATUS SYSCALL_API NtReleaseKeyedEvent( HANDLE handle, const void *key,
                                          BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtReleaseKeyedEvent );
}

NTSTATUS SYSCALL_API NtReleaseMutant( HANDLE handle, LONG *prev_count )
{
    SYSCALL_FUNC( NtReleaseMutant );
}

NTSTATUS SYSCALL_API NtReleaseSemaphore( HANDLE handle, ULONG count, ULONG *previous )
{
    SYSCALL_FUNC( NtReleaseSemaphore );
}

NTSTATUS SYSCALL_API NtRemoveIoCompletion( HANDLE handle, ULONG_PTR *key, ULONG_PTR *value,
                                           IO_STATUS_BLOCK *io, LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtRemoveIoCompletion );
}

NTSTATUS SYSCALL_API NtRemoveIoCompletionEx( HANDLE handle, FILE_IO_COMPLETION_INFORMATION *info,
                                             ULONG count, ULONG *written, LARGE_INTEGER *timeout,
                                             BOOLEAN alertable )
{
    SYSCALL_FUNC( NtRemoveIoCompletionEx );
}

NTSTATUS SYSCALL_API NtRemoveProcessDebug( HANDLE process, HANDLE debug )
{
    SYSCALL_FUNC( NtRemoveProcessDebug );
}

NTSTATUS SYSCALL_API NtRenameKey( HANDLE key, UNICODE_STRING *name )
{
    SYSCALL_FUNC( NtRenameKey );
}

NTSTATUS SYSCALL_API NtReplaceKey( OBJECT_ATTRIBUTES *attr, HANDLE key, OBJECT_ATTRIBUTES *replace )
{
    SYSCALL_FUNC( NtReplaceKey );
}

NTSTATUS SYSCALL_API NtReplyWaitReceivePort( HANDLE handle, ULONG *id, LPC_MESSAGE *reply, LPC_MESSAGE *msg )
{
    SYSCALL_FUNC( NtReplyWaitReceivePort );
}

NTSTATUS SYSCALL_API NtRequestWaitReplyPort( HANDLE handle, LPC_MESSAGE *msg_in, LPC_MESSAGE *msg_out )
{
    SYSCALL_FUNC( NtRequestWaitReplyPort );
}

NTSTATUS SYSCALL_API NtResetEvent( HANDLE handle, LONG *prev_state )
{
    SYSCALL_FUNC( NtResetEvent );
}

NTSTATUS SYSCALL_API NtResetWriteWatch( HANDLE process, PVOID base, SIZE_T size )
{
    SYSCALL_FUNC( NtResetWriteWatch );
}

NTSTATUS SYSCALL_API NtRestoreKey( HANDLE key, HANDLE file, ULONG flags )
{
    SYSCALL_FUNC( NtRestoreKey );
}

NTSTATUS SYSCALL_API NtResumeProcess( HANDLE handle )
{
    SYSCALL_FUNC( NtResumeProcess );
}

NTSTATUS SYSCALL_API NtResumeThread( HANDLE handle, ULONG *count )
{
    SYSCALL_FUNC( NtResumeThread );
}

NTSTATUS SYSCALL_API NtRollbackTransaction( HANDLE transaction, BOOLEAN wait )
{
    SYSCALL_FUNC( NtRollbackTransaction );
}

NTSTATUS SYSCALL_API NtSaveKey( HANDLE key, HANDLE file )
{
    SYSCALL_FUNC( NtSaveKey );
}

NTSTATUS SYSCALL_API NtSecureConnectPort( HANDLE *handle, UNICODE_STRING *name,
                                          SECURITY_QUALITY_OF_SERVICE *qos, LPC_SECTION_WRITE *write,
                                          PSID sid, LPC_SECTION_READ *read, ULONG *max_len,
                                          void *info, ULONG *info_len )
{
    SYSCALL_FUNC( NtSecureConnectPort );
}

static NTSTATUS SYSCALL_API syscall_NtSetContextThread( HANDLE handle, const ARM64_NT_CONTEXT *context )
{
    __ASM_SYSCALL_FUNC( __id_NtSetContextThread, syscall_NtSetContextThread );
}

NTSTATUS WINAPI NtSetContextThread( HANDLE handle, const CONTEXT *context )
{
    ARM64_NT_CONTEXT arm_ctx;

    context_x64_to_arm( &arm_ctx, (ARM64EC_NT_CONTEXT *)context );
    return syscall_NtSetContextThread( handle, &arm_ctx );
}

NTSTATUS SYSCALL_API NtSetDebugFilterState( ULONG component_id, ULONG level, BOOLEAN state )
{
    SYSCALL_FUNC( NtSetDebugFilterState );
}

NTSTATUS SYSCALL_API NtSetDefaultLocale( BOOLEAN user, LCID lcid )
{
    SYSCALL_FUNC( NtSetDefaultLocale );
}

NTSTATUS SYSCALL_API NtSetDefaultUILanguage( LANGID lang )
{
    SYSCALL_FUNC( NtSetDefaultUILanguage );
}

NTSTATUS SYSCALL_API NtSetEaFile( HANDLE handle, IO_STATUS_BLOCK *io, void *buffer, ULONG length )
{
    SYSCALL_FUNC( NtSetEaFile );
}

NTSTATUS SYSCALL_API NtSetEvent( HANDLE handle, LONG *prev_state )
{
    SYSCALL_FUNC( NtSetEvent );
}

NTSTATUS SYSCALL_API NtSetInformationDebugObject( HANDLE handle, DEBUGOBJECTINFOCLASS class,
                                                  void *info, ULONG len, ULONG *ret_len )
{
    SYSCALL_FUNC( NtSetInformationDebugObject );
}

NTSTATUS SYSCALL_API NtSetInformationFile( HANDLE handle, IO_STATUS_BLOCK *io,
                                           void *ptr, ULONG len, FILE_INFORMATION_CLASS class )
{
    SYSCALL_FUNC( NtSetInformationFile );
}

NTSTATUS SYSCALL_API NtSetInformationJobObject( HANDLE handle, JOBOBJECTINFOCLASS class,
                                                void *info, ULONG len )
{
    SYSCALL_FUNC( NtSetInformationJobObject );
}

NTSTATUS SYSCALL_API NtSetInformationKey( HANDLE key, int class, void *info, ULONG length )
{
    SYSCALL_FUNC( NtSetInformationKey );
}

NTSTATUS SYSCALL_API NtSetInformationObject( HANDLE handle, OBJECT_INFORMATION_CLASS info_class,
                                             void *ptr, ULONG len )
{
    SYSCALL_FUNC( NtSetInformationObject );
}

NTSTATUS SYSCALL_API NtSetInformationProcess( HANDLE handle, PROCESSINFOCLASS class,
                                              void *info, ULONG size )
{
    SYSCALL_FUNC( NtSetInformationProcess );
}

NTSTATUS SYSCALL_API NtSetInformationThread( HANDLE handle, THREADINFOCLASS class,
                                             const void *data, ULONG length )
{
    SYSCALL_FUNC( NtSetInformationThread );
}

NTSTATUS SYSCALL_API NtSetInformationToken( HANDLE token, TOKEN_INFORMATION_CLASS class,
                                            void *info, ULONG length )
{
    SYSCALL_FUNC( NtSetInformationToken );
}

NTSTATUS SYSCALL_API NtSetInformationVirtualMemory( HANDLE process,
                                                    VIRTUAL_MEMORY_INFORMATION_CLASS info_class,
                                                    ULONG_PTR count, PMEMORY_RANGE_ENTRY addresses,
                                                    PVOID ptr, ULONG size )
{
    SYSCALL_FUNC( NtSetInformationVirtualMemory );
}

NTSTATUS SYSCALL_API NtSetIntervalProfile( ULONG interval, KPROFILE_SOURCE source )
{
    SYSCALL_FUNC( NtSetIntervalProfile );
}

NTSTATUS SYSCALL_API NtSetIoCompletion( HANDLE handle, ULONG_PTR key, ULONG_PTR value,
                                        NTSTATUS status, SIZE_T count )
{
    SYSCALL_FUNC( NtSetIoCompletion );
}

NTSTATUS SYSCALL_API NtSetLdtEntries( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 )
{
    SYSCALL_FUNC( NtSetLdtEntries );
}

NTSTATUS SYSCALL_API NtSetSecurityObject( HANDLE handle, SECURITY_INFORMATION info,
                                          PSECURITY_DESCRIPTOR descr )
{
    SYSCALL_FUNC( NtSetSecurityObject );
}

NTSTATUS SYSCALL_API NtSetSystemInformation( SYSTEM_INFORMATION_CLASS class, void *info, ULONG length )
{
    SYSCALL_FUNC( NtSetSystemInformation );
}

NTSTATUS SYSCALL_API NtSetSystemTime( const LARGE_INTEGER *new, LARGE_INTEGER *old )
{
    SYSCALL_FUNC( NtSetSystemTime );
}

NTSTATUS SYSCALL_API NtSetThreadExecutionState( EXECUTION_STATE new_state, EXECUTION_STATE *old_state )
{
    SYSCALL_FUNC( NtSetThreadExecutionState );
}

NTSTATUS SYSCALL_API NtSetTimer( HANDLE handle, const LARGE_INTEGER *when, PTIMER_APC_ROUTINE callback,
                                 void *arg, BOOLEAN resume, ULONG period, BOOLEAN *state )
{
    SYSCALL_FUNC( NtSetTimer );
}

NTSTATUS SYSCALL_API NtSetTimerResolution( ULONG res, BOOLEAN set, ULONG *current_res )
{
    SYSCALL_FUNC( NtSetTimerResolution );
}

NTSTATUS SYSCALL_API NtSetValueKey( HANDLE key, const UNICODE_STRING *name, ULONG index,
                                    ULONG type, const void *data, ULONG count )
{
    SYSCALL_FUNC( NtSetValueKey );
}

NTSTATUS SYSCALL_API NtSetVolumeInformationFile( HANDLE handle, IO_STATUS_BLOCK *io, void *info,
                                                 ULONG length, FS_INFORMATION_CLASS class )
{
    SYSCALL_FUNC( NtSetVolumeInformationFile );
}

NTSTATUS SYSCALL_API NtShutdownSystem( SHUTDOWN_ACTION action )
{
    SYSCALL_FUNC( NtShutdownSystem );
}

NTSTATUS SYSCALL_API NtSignalAndWaitForSingleObject( HANDLE signal, HANDLE wait,
                                                     BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtSignalAndWaitForSingleObject );
}

NTSTATUS SYSCALL_API NtSuspendProcess( HANDLE handle )
{
    SYSCALL_FUNC( NtSuspendProcess );
}

NTSTATUS SYSCALL_API NtSuspendThread( HANDLE handle, ULONG *count )
{
    SYSCALL_FUNC( NtSuspendThread );
}

NTSTATUS SYSCALL_API NtSystemDebugControl( SYSDBG_COMMAND command, void *in_buff, ULONG in_len,
                                           void *out_buff, ULONG out_len, ULONG *retlen )
{
    SYSCALL_FUNC( NtSystemDebugControl );
}

NTSTATUS SYSCALL_API NtTerminateJobObject( HANDLE handle, NTSTATUS status )
{
    SYSCALL_FUNC( NtTerminateJobObject );
}

NTSTATUS SYSCALL_API NtTerminateProcess( HANDLE handle, LONG exit_code )
{
    SYSCALL_FUNC( NtTerminateProcess );
}

NTSTATUS SYSCALL_API NtTerminateThread( HANDLE handle, LONG exit_code )
{
    SYSCALL_FUNC( NtTerminateThread );
}

NTSTATUS SYSCALL_API NtTestAlert(void)
{
    SYSCALL_FUNC( NtTestAlert );
}

NTSTATUS SYSCALL_API NtTraceControl( ULONG code, void *inbuf, ULONG inbuf_len,
                                     void *outbuf, ULONG outbuf_len, ULONG *size )
{
    SYSCALL_FUNC( NtTraceControl );
}

NTSTATUS SYSCALL_API NtUnloadDriver( const UNICODE_STRING *name )
{
    SYSCALL_FUNC( NtUnloadDriver );
}

NTSTATUS SYSCALL_API NtUnloadKey( OBJECT_ATTRIBUTES *attr )
{
    SYSCALL_FUNC( NtUnloadKey );
}

NTSTATUS SYSCALL_API NtUnlockFile( HANDLE handle, IO_STATUS_BLOCK *io_status, LARGE_INTEGER *offset,
                                   LARGE_INTEGER *count, ULONG *key )
{
    SYSCALL_FUNC( NtUnlockFile );
}

NTSTATUS SYSCALL_API NtUnlockVirtualMemory( HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown )
{
    SYSCALL_FUNC( NtUnlockVirtualMemory );
}

NTSTATUS SYSCALL_API NtUnmapViewOfSection( HANDLE process, PVOID addr )
{
    SYSCALL_FUNC( NtUnmapViewOfSection );
}

NTSTATUS SYSCALL_API NtUnmapViewOfSectionEx( HANDLE process, PVOID addr, ULONG flags )
{
    SYSCALL_FUNC( NtUnmapViewOfSectionEx );
}

NTSTATUS SYSCALL_API NtWaitForAlertByThreadId( const void *address, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtWaitForAlertByThreadId );
}

NTSTATUS SYSCALL_API NtWaitForDebugEvent( HANDLE handle, BOOLEAN alertable, LARGE_INTEGER *timeout,
                                          DBGUI_WAIT_STATE_CHANGE *state )
{
    SYSCALL_FUNC( NtWaitForDebugEvent );
}

NTSTATUS SYSCALL_API NtWaitForKeyedEvent( HANDLE handle, const void *key,
                                          BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtWaitForKeyedEvent );
}

NTSTATUS SYSCALL_API NtWaitForMultipleObjects( DWORD count, const HANDLE *handles, BOOLEAN wait_any,
                                               BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtWaitForMultipleObjects );
}

NTSTATUS SYSCALL_API NtWaitForSingleObject( HANDLE handle, BOOLEAN alertable, const LARGE_INTEGER *timeout )
{
    SYSCALL_FUNC( NtWaitForSingleObject );
}

NTSTATUS SYSCALL_API NtWriteFile( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                  IO_STATUS_BLOCK *io, const void *buffer, ULONG length,
                                  LARGE_INTEGER *offset, ULONG *key )
{
    SYSCALL_FUNC( NtWriteFile );
}

NTSTATUS SYSCALL_API NtWriteFileGather( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                        IO_STATUS_BLOCK *io, FILE_SEGMENT_ELEMENT *segments,
                                        ULONG length, LARGE_INTEGER *offset, ULONG *key )
{
    SYSCALL_FUNC( NtWriteFileGather );
}

NTSTATUS SYSCALL_API NtWriteVirtualMemory( HANDLE process, void *addr, const void *buffer,
                                           SIZE_T size, SIZE_T *bytes_written )
{
    SYSCALL_FUNC( NtWriteVirtualMemory );
}

NTSTATUS SYSCALL_API NtYieldExecution(void)
{
    SYSCALL_FUNC( NtYieldExecution );
}

NTSTATUS SYSCALL_API wine_nt_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char *nameA, ULONG *size,
                                                UINT disposition )
{
    SYSCALL_FUNC( wine_nt_to_unix_file_name );
}

NTSTATUS SYSCALL_API wine_unix_to_nt_file_name( const char *name, WCHAR *buffer, ULONG *size )
{
    SYSCALL_FUNC( wine_unix_to_nt_file_name );
}

static void * const arm64ec_syscalls[] =
{
#define SYSCALL_ENTRY(id,name,args) name,
    ALL_SYSCALLS64
#undef SYSCALL_ENTRY
};


/***********************************************************************
 *           LdrpGetX64Information
 */
static NTSTATUS WINAPI LdrpGetX64Information( ULONG type, void *output, void *extra_info )
{
    switch (type)
    {
    case 0:
    {
        UINT64 fpcr, fpsr;

        __asm__ __volatile__( "mrs %0, fpcr; mrs %1, fpsr" : "=r" (fpcr), "=r" (fpsr) );
        *(UINT *)output = fpcsr_to_mxcsr( fpcr, fpsr );
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "not implemented type %u\n", type );
        return STATUS_INVALID_PARAMETER;
    }
}

/***********************************************************************
 *           LdrpSetX64Information
 */
static NTSTATUS WINAPI LdrpSetX64Information( ULONG type, ULONG_PTR input, void *extra_info )
{
    switch (type)
    {
    case 0:
    {
        UINT64 fpcsr = mxcsr_to_fpcsr( input );
        __asm__ __volatile__( "msr fpcr, %0; msr fpsr, %1" :: "r" (fpcsr), "r" (fpcsr >> 32) );
        return STATUS_SUCCESS;
    }
    default:
        FIXME( "not implemented type %u\n", type );
        return STATUS_INVALID_PARAMETER;
    }
}


/**********************************************************************
 *           virtual_unwind
 */
static NTSTATUS virtual_unwind( ULONG type, DISPATCHER_CONTEXT_ARM64EC *dispatch,
                                ARM64EC_NT_CONTEXT *context )
{
    DISPATCHER_CONTEXT_NONVOLREG_ARM64 *nonvol_regs;
    DWORD64 pc = context->Pc;
    int i;

    dispatch->ScopeIndex = 0;
    dispatch->ControlPc  = pc;
    dispatch->ControlPcIsUnwound = (context->ContextFlags & CONTEXT_UNWOUND_TO_CALL) != 0;
    if (dispatch->ControlPcIsUnwound && RtlIsEcCode( pc )) pc -= 4;

    nonvol_regs = (DISPATCHER_CONTEXT_NONVOLREG_ARM64 *)dispatch->NonVolatileRegisters;
    nonvol_regs->GpNvRegs[0]  = context->X19;
    nonvol_regs->GpNvRegs[1]  = context->X20;
    nonvol_regs->GpNvRegs[2]  = context->X21;
    nonvol_regs->GpNvRegs[3]  = context->X22;
    nonvol_regs->GpNvRegs[4]  = 0;
    nonvol_regs->GpNvRegs[5]  = 0;
    nonvol_regs->GpNvRegs[6]  = context->X25;
    nonvol_regs->GpNvRegs[7]  = context->X26;
    nonvol_regs->GpNvRegs[8]  = context->X27;
    nonvol_regs->GpNvRegs[9]  = 0;
    nonvol_regs->GpNvRegs[10] = context->Fp;
    for (i = 0; i < 8; i++) nonvol_regs->FpNvRegs[i] = context->V[i + 8].D[0];

    dispatch->FunctionEntry = RtlLookupFunctionEntry( pc, &dispatch->ImageBase, dispatch->HistoryTable );

    if (RtlVirtualUnwind2( type, dispatch->ImageBase, pc, dispatch->FunctionEntry, &context->AMD64_Context,
                           NULL, &dispatch->HandlerData, &dispatch->EstablisherFrame,
                           NULL, NULL, NULL, &dispatch->LanguageHandler, 0 ))
    {
        WARN( "exception data not found for pc %p\n", (void *)pc );
        return STATUS_INVALID_DISPOSITION;
    }
    return STATUS_SUCCESS;
}


/**********************************************************************
 *           unwind_exception_handler
 *
 * Handler for exceptions happening while calling an unwind handler.
 */
EXCEPTION_DISPOSITION WINAPI unwind_exception_handler( EXCEPTION_RECORD *record, void *frame,
                                                       CONTEXT *context, DISPATCHER_CONTEXT_ARM64EC *dispatch )
{
    DISPATCHER_CONTEXT_ARM64EC *orig_dispatch = ((DISPATCHER_CONTEXT_ARM64EC **)frame)[-2];

    /* copy the original dispatcher into the current one, except for the TargetPc */
    dispatch->ControlPc          = orig_dispatch->ControlPc;
    dispatch->ImageBase          = orig_dispatch->ImageBase;
    dispatch->FunctionEntry      = orig_dispatch->FunctionEntry;
    dispatch->EstablisherFrame   = orig_dispatch->EstablisherFrame;
    dispatch->LanguageHandler    = orig_dispatch->LanguageHandler;
    dispatch->HandlerData        = orig_dispatch->HandlerData;
    dispatch->HistoryTable       = orig_dispatch->HistoryTable;
    dispatch->ScopeIndex         = orig_dispatch->ScopeIndex;
    dispatch->ControlPcIsUnwound = orig_dispatch->ControlPcIsUnwound;
    *dispatch->ContextRecord     = *orig_dispatch->ContextRecord;
    memcpy( dispatch->NonVolatileRegisters, orig_dispatch->NonVolatileRegisters,
            sizeof(DISPATCHER_CONTEXT_NONVOLREG_ARM64) );
    TRACE( "detected collided unwind\n" );
    return ExceptionCollidedUnwind;
}


/**********************************************************************
 *           call_unwind_handler
 */
static DWORD __attribute__((naked)) call_unwind_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                                         CONTEXT *context, void *dispatch,
                                                         PEXCEPTION_ROUTINE handler )
{
    asm( ".seh_proc call_unwind_handler\n\t"
         "stp x29, x30, [sp, #-32]!\n\t"
         ".seh_save_fplr_x 32\n\t"
         ".seh_endprologue\n\t"
         ".seh_handler unwind_exception_handler, @except\n\t"
         "str x3, [sp, #16]\n\t"    /* frame[-2] = dispatch */
         "mov x11, x4\n\t" /* handler */
         "adr x10, $iexit_thunk$cdecl$i8$i8i8i8i8\n\t"
         "adrp x16, __os_arm64x_dispatch_icall\n\t"
         "ldr x16, [x16, #:lo12:__os_arm64x_dispatch_icall]\n\t"
         "blr x16\n\t"
         "blr x11\n\t"
         "ldp x29, x30, [sp], #32\n\t"
         "ret\n\t"
         ".seh_endproc" );
}


/***********************************************************************
 *		call_seh_handler
 */
static DWORD __attribute__((naked)) call_seh_handler( EXCEPTION_RECORD *rec, ULONG_PTR frame,
                                                      CONTEXT *context, void *dispatch, PEXCEPTION_ROUTINE handler )
{
    asm( ".seh_proc call_seh_handler\n\t"
         "stp x29, x30, [sp, #-16]!\n\t"
         ".seh_save_fplr_x 16\n\t"
         ".seh_endprologue\n\t"
         ".seh_handler nested_exception_handler, @except\n\t"
         "mov x11, x4\n\t" /* handler */
         "adr x10, $iexit_thunk$cdecl$i8$i8i8i8i8\n\t"
         "adrp x16, __os_arm64x_dispatch_icall\n\t"
         "ldr x16, [x16, #:lo12:__os_arm64x_dispatch_icall]\n\t"
         "blr x16\n\t"
         "blr x11\n\t"
         "ldp x29, x30, [sp], #16\n\t"
         "ret\n\t"
         ".seh_endproc" );
}


/**********************************************************************
 *           call_seh_handlers
 *
 * Call the SEH handlers.
 */
NTSTATUS call_seh_handlers( EXCEPTION_RECORD *rec, CONTEXT *orig_context )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    DISPATCHER_CONTEXT_NONVOLREG_ARM64 nonvol_regs;
    UNWIND_HISTORY_TABLE table;
    DISPATCHER_CONTEXT_ARM64EC dispatch;
    ARM64EC_NT_CONTEXT context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD res;

    context.AMD64_Context = *orig_context;
    context.ContextFlags &= ~0x40; /* Clear xstate flag. */

    dispatch.TargetPc      = 0;
    dispatch.ContextRecord = &context.AMD64_Context;
    dispatch.HistoryTable  = &table;
    dispatch.NonVolatileRegisters = nonvol_regs.Buffer;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_EHANDLER, &dispatch, &context );
        if (status != STATUS_SUCCESS) return status;

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_arm64ec_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %I64x (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
                   dispatch.LanguageHandler, rec, dispatch.EstablisherFrame, orig_context, &dispatch );
            res = call_seh_handler( rec, dispatch.EstablisherFrame, orig_context,
                                    &dispatch, dispatch.LanguageHandler );
            rec->ExceptionFlags &= EXCEPTION_NONCONTINUABLE;
            TRACE( "handler at %p returned %lu\n", dispatch.LanguageHandler, res );

            switch (res)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind:
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context.AMD64_Context, &dispatch.HandlerData, &frame, NULL );
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
        }
        /* hack: call wine handlers registered in the tib list */
        else while (is_valid_frame( (ULONG_PTR)teb_frame ) && (ULONG64)teb_frame < context.Sp)
        {
            TRACE( "calling TEB handler %p (rec=%p frame=%p context=%p dispatch=%p) sp=%I64x\n",
                   teb_frame->Handler, rec, teb_frame, orig_context, &dispatch, context.Sp );
            res = call_seh_handler( rec, (ULONG_PTR)teb_frame, orig_context,
                                    &dispatch, (PEXCEPTION_ROUTINE)teb_frame->Handler );
            TRACE( "TEB handler at %p returned %lu\n", teb_frame->Handler, res );

            switch (res)
            {
            case ExceptionContinueExecution:
                if (rec->ExceptionFlags & EXCEPTION_NONCONTINUABLE) return STATUS_NONCONTINUABLE_EXCEPTION;
                return STATUS_SUCCESS;
            case ExceptionContinueSearch:
                break;
            case ExceptionNestedException:
                rec->ExceptionFlags |= EXCEPTION_NESTED_CALL;
                TRACE( "nested exception\n" );
                break;
            case ExceptionCollidedUnwind:
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase,
                                  dispatch.ControlPc, dispatch.FunctionEntry,
                                  &context.AMD64_Context, &dispatch.HandlerData, &frame, NULL );
                teb_frame = teb_frame->Prev;
                goto unwind_done;
            default:
                return STATUS_INVALID_DISPOSITION;
            }
            teb_frame = teb_frame->Prev;
        }

        if (context.Sp == (ULONG64)NtCurrentTeb()->Tib.StackBase) break;
    }
    return STATUS_UNHANDLED_EXCEPTION;
}


/*******************************************************************
 *		KiUserExceptionDispatcher (NTDLL.@)
 */
static NTSTATUS __attribute__((used)) dispatch_exception_arm64ec( EXCEPTION_RECORD *rec, ARM64_NT_CONTEXT *arm_ctx )
{
    ARM64EC_NT_CONTEXT context;

    context_arm_to_x64( &context, arm_ctx );
    return dispatch_exception( rec, &context.AMD64_Context );
}
__ASM_GLOBAL_FUNC( "#KiUserExceptionDispatcher",
                   ".seh_context\n\t"
                   ".seh_endprologue\n\t"
                   "add x0, sp, #0x390\n\t"       /* rec (context + 1) */
                   "mov x1, sp\n\t"               /* context */
                   "bl dispatch_exception_arm64ec\n\t"
                   "brk #1" )


/*******************************************************************
 *		KiUserApcDispatcher (NTDLL.@)
 */
static void __attribute__((used)) dispatch_apc( void (CALLBACK *func)(ULONG_PTR,ULONG_PTR,ULONG_PTR,CONTEXT*),
                                                ULONG_PTR arg1, ULONG_PTR arg2, ULONG_PTR arg3,
                                                BOOLEAN alertable, ARM64_NT_CONTEXT *arm_ctx )
{
    ARM64EC_NT_CONTEXT context;

    context_arm_to_x64( &context, arm_ctx );
    func( arg1, arg2, arg3, &context.AMD64_Context );
    NtContinue( &context.AMD64_Context, alertable );
}
__ASM_GLOBAL_FUNC( "#KiUserApcDispatcher",
                   ".seh_context\n\t"
                   "nop\n\t"
                   ".seh_stackalloc 0x30\n\t"
                   ".seh_endprologue\n\t"
                   "ldp x0, x1, [sp]\n\t"         /* func, arg1 */
                   "ldp x2, x3, [sp, #0x10]\n\t"  /* arg2, arg3 */
                   "ldr w4, [sp, #0x20]\n\t"      /* alertable */
                   "add x5, sp, #0x30\n\t"        /* context */
                   "bl " __ASM_NAME("dispatch_apc") "\n\t"
                   "brk #1" )


/*******************************************************************
 *		KiUserCallbackDispatcher (NTDLL.@)
 */
__ASM_GLOBAL_FUNC( "#KiUserCallbackDispatcher",
                   ".seh_pushframe\n\t"
                   "nop\n\t"
                   ".seh_stackalloc 0x20\n\t"
                   "nop\n\t"
                   ".seh_save_reg lr, 0x18\n\t"
                   ".seh_endprologue\n\t"
                   ".seh_handler " __ASM_NAME("user_callback_handler") ", @except\n\t"
                   "ldr x0, [sp]\n\t"             /* args */
                   "ldp w1, w2, [sp, #0x08]\n\t"  /* len, id */
                   "ldr x3, [x18, 0x60]\n\t"      /* peb */
                   "ldr x3, [x3, 0x58]\n\t"       /* peb->KernelCallbackTable */
                   "ldr x15, [x3, x2, lsl #3]\n\t"
                   "blr x15\n\t"
                   ".globl \"#KiUserCallbackDispatcherReturn\"\n"
                   "\"#KiUserCallbackDispatcherReturn\":\n\t"
                   "mov x2, x0\n\t"               /* status */
                   "mov x1, #0\n\t"               /* ret_len */
                   "mov x0, x1\n\t"               /* ret_ptr */
                   "bl " __ASM_NAME("NtCallbackReturn") "\n\t"
                   "bl " __ASM_NAME("RtlRaiseStatus") "\n\t"
                   "brk #1" )


/**************************************************************************
 *              RtlIsEcCode (NTDLL.@)
 */
BOOLEAN WINAPI RtlIsEcCode( ULONG_PTR ptr )
{
    const UINT64 *map = (const UINT64 *)NtCurrentTeb()->Peb->EcCodeBitMap;
    ULONG_PTR page = ptr / page_size;
    return (map[page / 64] >> (page & 63)) & 1;
}


/* unwind context by one call frame */
static void unwind_one_frame( CONTEXT *context )
{
    void *data;
    ULONG_PTR base, frame, pc = context->Rip - 4;
    RUNTIME_FUNCTION *func = RtlLookupFunctionEntry( pc, &base, NULL );

    RtlVirtualUnwind( UNW_FLAG_NHANDLER, base, pc, func, context, &data, &frame, NULL );
}

/* capture context information; helper for RtlCaptureContext */
static void __attribute__((used)) capture_context( CONTEXT *context, UINT cpsr, UINT fpcr, UINT fpsr )
{
    CONTEXT unwind_context;

    context->ContextFlags = CONTEXT_AMD64_FULL;
    context->EFlags = cpsr_to_eflags( cpsr );
    context->MxCsr = fpcsr_to_mxcsr( fpcr, fpsr );
    context->FltSave.ControlWord = 0x27f;
    context->FltSave.StatusWord = 0;
    context->FltSave.MxCsr = context->MxCsr;

    /* unwind one level to get register values from caller function */
    unwind_context = *context;
    unwind_one_frame( &unwind_context );
    memcpy( &context->Rax, &unwind_context.Rax, offsetof(CONTEXT,FltSave) - offsetof(CONTEXT,Rax) );
}

/***********************************************************************
 *		RtlCaptureContext (NTDLL.@)
 */
void __attribute__((naked)) RtlCaptureContext( CONTEXT *context )
{
    asm( ".seh_proc RtlCaptureContext\n\t"
         ".seh_endprologue\n\t"
         "stp x8, x0,   [x0, #0x78]\n\t"    /* context->Rax,Rcx */
         "stp x1, x27,  [x0, #0x88]\n\t"    /* context->Rdx,Rbx */
         "mov x1, sp\n\t"
         "stp x1, x29,  [x0, #0x98]\n\t"    /* context->Rsp,Rbp */
         "stp x25, x26, [x0, #0xa8]\n\t"    /* context->Rsi,Rdi */
         "stp x2, x3,   [x0, #0xb8]\n\t"    /* context->R8,R9 */
         "stp x4, x5,   [x0, #0xc8]\n\t"    /* context->R10,R11 */
         "stp x19, x20, [x0, #0xd8]\n\t"    /* context->R12,R13 */
         "stp x21, x22, [x0, #0xe8]\n\t"    /* context->R14,R15 */
         "str x30,      [x0, #0xf8]\n\t"    /* context->Rip */
         "ubfx x1, x16, #0, #16\n\t"
         "stp x30, x1,  [x0, #0x120]\n\t"   /* context->FloatRegisters[0] */
         "ubfx x1, x16, #16, #16\n\t"
         "stp x6, x1,   [x0, #0x130]\n\t"   /* context->FloatRegisters[1] */
         "ubfx x1, x16, #32, #16\n\t"
         "stp x7, x1,   [x0, #0x140]\n\t"   /* context->FloatRegisters[2] */
         "ubfx x1, x16, #48, #16\n\t"
         "stp x9, x1,   [x0, #0x150]\n\t"   /* context->FloatRegisters[3] */
         "ubfx x1, x17, #0, #16\n\t"
         "stp x10, x1,  [x0, #0x160]\n\t"   /* context->FloatRegisters[4] */
         "ubfx x1, x17, #16, #16\n\t"
         "stp x11, x1,  [x0, #0x170]\n\t"   /* context->FloatRegisters[5] */
         "ubfx x1, x17, #32, #16\n\t"
         "stp x12, x1,  [x0, #0x180]\n\t"   /* context->FloatRegisters[6] */
         "ubfx x1, x17, #48, #16\n\t"
         "stp x15, x1,  [x0, #0x190]\n\t"   /* context->FloatRegisters[7] */
         "stp q0, q1,   [x0, #0x1a0]\n\t"   /* context->Xmm0,Xmm1 */
         "stp q2, q3,   [x0, #0x1c0]\n\t"   /* context->Xmm2,Xmm3 */
         "stp q4, q5,   [x0, #0x1e0]\n\t"   /* context->Xmm4,Xmm5 */
         "stp q6, q7,   [x0, #0x200]\n\t"   /* context->Xmm6,Xmm7 */
         "stp q8, q9,   [x0, #0x220]\n\t"   /* context->Xmm8,Xmm9 */
         "stp q10, q11, [x0, #0x240]\n\t"   /* context->Xmm10,Xmm11 */
         "stp q12, q13, [x0, #0x260]\n\t"   /* context->Xmm12,Xmm13 */
         "stp q14, q15, [x0, #0x280]\n\t"   /* context->Xmm14,Xmm15 */
         "mrs x1, nzcv\n\t"
         "mrs x2, fpcr\n\t"
         "mrs x3, fpsr\n\t"
         "b capture_context\n\t"
         ".seh_endproc" );
}

/* fixup jump buffer information; helper for _setjmpex */
static int __attribute__((used)) do_setjmpex( _JUMP_BUFFER *buf, UINT fpcr, UINT fpsr )
{
    CONTEXT context = { .ContextFlags = CONTEXT_FULL };

    buf->MxCsr = fpcsr_to_mxcsr( fpcr, fpsr );
    buf->FpCsr = 0x27f;

    context.Rbx = buf->Rbx;
    context.Rsp = buf->Rsp;
    context.Rbp = buf->Rbp;
    context.Rsi = buf->Rsi;
    context.Rdi = buf->Rdi;
    context.R12 = buf->R12;
    context.R13 = buf->R13;
    context.R14 = buf->R14;
    context.R15 = buf->R15;
    context.Rip = buf->Rip;
    memcpy( &context.Xmm6, &buf->Xmm6, 10 * sizeof(context.Xmm6) );
    unwind_one_frame( &context );
    if (!RtlIsEcCode( context.Rip ))  /* caller is x64, use its context instead of the ARM one */
    {
        buf->Rbx = context.Rbx;
        buf->Rsp = context.Rsp;
        buf->Rbp = context.Rbp;
        buf->Rsi = context.Rsi;
        buf->Rdi = context.Rdi;
        buf->R12 = context.R12;
        buf->R13 = context.R13;
        buf->R14 = context.R14;
        buf->R15 = context.R15;
        buf->Rip = context.Rip;
        memcpy( &buf->Xmm6, &context.Xmm6, 10 * sizeof(context.Xmm6) );
    }
    return 0;
}

/***********************************************************************
 *		_setjmpex (NTDLL.@)
 */
int __attribute__((naked)) NTDLL__setjmpex( _JUMP_BUFFER *buf, void *frame )
{
    asm( ".seh_proc NTDLL__setjmpex\n\t"
         ".seh_endprologue\n\t"
         "stp x1, x27,  [x0]\n\t"          /* jmp_buf->Frame,Rbx */
         "mov x1, sp\n\t"
         "stp x1, x29,  [x0, #0x10]\n\t"   /* jmp_buf->Rsp,Rbp */
         "stp x25, x26, [x0, #0x20]\n\t"   /* jmp_buf->Rsi,Rdi */
         "stp x19, x20, [x0, #0x30]\n\t"   /* jmp_buf->R12,R13 */
         "stp x21, x22, [x0, #0x40]\n\t"   /* jmp_buf->R14,R15 */
         "str x30,      [x0, #0x50]\n\t"   /* jmp_buf->Rip */
         "stp d8, d9,   [x0, #0x80]\n\t"   /* jmp_buf->Xmm8,Xmm9 */
         "stp d10, d11, [x0, #0xa0]\n\t"   /* jmp_buf->Xmm10,Xmm11 */
         "stp d12, d13, [x0, #0xc0]\n\t"   /* jmp_buf->Xmm12,Xmm13 */
         "stp d14, d15, [x0, #0xe0]\n\t"   /* jmp_buf->Xmm14,Xmm15 */
         "mrs x1, fpcr\n\t"
         "mrs x2, fpsr\n\t"
         "b do_setjmpex\n\t"
         ".seh_endproc" );
}


/**********************************************************************
 *           call_consolidate_callback
 *
 * Wrapper function to call a consolidate callback from a fake frame.
 * If the callback executes RtlUnwindEx (like for example done in C++ handlers),
 * we have to skip all frames which were already processed. To do that we
 * trick the unwinding functions into thinking the call came from somewhere
 * else.
 */
static void __attribute__((naked,noreturn)) consolidate_callback( CONTEXT *context,
                                                                  void *(CALLBACK *callback)(EXCEPTION_RECORD *),
                                                                  EXCEPTION_RECORD *rec )
{
    asm( ".seh_proc consolidate_callback\n\t"
         "stp x29, x30, [sp, #-16]!\n\t"
         ".seh_save_fplr_x 16\n\t"
         "sub sp, sp, #0x4d0\n\t"
         ".seh_stackalloc 0x4d0\n\t"
         ".seh_endprologue\n\t"
         "mov x4, sp\n\t"
         /* copy the context onto the stack */
         "mov x5, #0x4d0/16\n"      /* sizeof(CONTEXT) */
         "1:\tldp x6, x7, [x0], #16\n\t"
         "stp x6, x7, [x4], #16\n\t"
         "subs x5, x5, #1\n\t"
         "b.ne 1b\n\t"
         "mov x0, x2\n\t"           /* rec */
         "b invoke_callback\n\t"
         ".seh_endproc\n\t"
         ".seh_proc invoke_callback\n"
         "invoke_callback:\n\t"
         ".seh_ec_context\n\t"
         ".seh_endprologue\n\t"
         "mov x11, x1\n\t"          /* callback */
         "adr x10, $iexit_thunk$cdecl$i8$i8\n\t"
         "adrp x16, __os_arm64x_dispatch_icall\n\t"
         "ldr x16, [x16, #:lo12:__os_arm64x_dispatch_icall]\n\t"
         "blr x16\n\t"
         "blr x11\n\t"
         "str x0, [sp, #0xf8]\n\t"  /* context->Rip */
         "mov x0, sp\n\t"           /* context */
         "mov w1, #0\n\t"
         "bl \"#NtContinue\"\n\t"
         ".seh_endproc" );
}


/*******************************************************************
 *              RtlRestoreContext (NTDLL.@)
 */
void CDECL RtlRestoreContext( CONTEXT *context, EXCEPTION_RECORD *rec )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;

    if (rec && rec->ExceptionCode == STATUS_LONGJUMP && rec->NumberParameters >= 1)
    {
        struct _JUMP_BUFFER *jmp = (struct _JUMP_BUFFER *)rec->ExceptionInformation[0];

        context->Rbx   = jmp->Rbx;
        context->Rsp   = jmp->Rsp;
        context->Rbp   = jmp->Rbp;
        context->Rsi   = jmp->Rsi;
        context->Rdi   = jmp->Rdi;
        context->R12   = jmp->R12;
        context->R13   = jmp->R13;
        context->R14   = jmp->R14;
        context->R15   = jmp->R15;
        context->Rip   = jmp->Rip;
        context->MxCsr = jmp->MxCsr;
        context->FltSave.MxCsr = jmp->MxCsr;
        context->FltSave.ControlWord = jmp->FpCsr;
        memcpy( &context->Xmm6, &jmp->Xmm6, 10 * sizeof(M128A) );
    }
    else if (rec && rec->ExceptionCode == STATUS_UNWIND_CONSOLIDATE && rec->NumberParameters >= 1)
    {
        void * (CALLBACK *consolidate)(EXCEPTION_RECORD *) = (void *)rec->ExceptionInformation[0];
        TRACE( "calling consolidate callback %p (rec=%p)\n", consolidate, rec );
        consolidate_callback( context, consolidate, rec );
    }

    /* hack: remove no longer accessible TEB frames */
    while ((ULONG64)teb_frame < context->Rsp)
    {
        TRACE( "removing TEB frame: %p\n", teb_frame );
        teb_frame = __wine_pop_frame( teb_frame );
    }

    TRACE( "returning to %p stack %p\n", (void *)context->Rip, (void *)context->Rsp );
    NtContinue( context, FALSE );
}


/*******************************************************************
 *		RtlUnwindEx (NTDLL.@)
 */
void WINAPI RtlUnwindEx( PVOID end_frame, PVOID target_ip, EXCEPTION_RECORD *rec,
                         PVOID retval, CONTEXT *context, UNWIND_HISTORY_TABLE *table )
{
    EXCEPTION_REGISTRATION_RECORD *teb_frame = NtCurrentTeb()->Tib.ExceptionList;
    DISPATCHER_CONTEXT_NONVOLREG_ARM64 nonvol_regs;
    EXCEPTION_RECORD record;
    DISPATCHER_CONTEXT_ARM64EC dispatch;
    ARM64EC_NT_CONTEXT new_context;
    NTSTATUS status;
    ULONG_PTR frame;
    DWORD i, res;
    BOOL is_target_ec = RtlIsEcCode( (ULONG_PTR)target_ip );

    RtlCaptureContext( context );
    new_context.AMD64_Context = *context;

    /* build an exception record, if we do not have one */
    if (!rec)
    {
        record.ExceptionCode    = STATUS_UNWIND;
        record.ExceptionFlags   = 0;
        record.ExceptionRecord  = NULL;
        record.ExceptionAddress = (void *)context->Rip;
        record.NumberParameters = 0;
        rec = &record;
    }

    rec->ExceptionFlags |= EXCEPTION_UNWINDING | (end_frame ? 0 : EXCEPTION_EXIT_UNWIND);

    TRACE( "code=%lx flags=%lx end_frame=%p target_ip=%p\n",
           rec->ExceptionCode, rec->ExceptionFlags, end_frame, target_ip );
    for (i = 0; i < min( EXCEPTION_MAXIMUM_PARAMETERS, rec->NumberParameters ); i++)
        TRACE( " info[%ld]=%016I64x\n", i, rec->ExceptionInformation[i] );
    TRACE_CONTEXT( context );

    dispatch.TargetIp         = (ULONG64)target_ip;
    dispatch.ContextRecord    = context;
    dispatch.HistoryTable     = table;
    dispatch.NonVolatileRegisters = nonvol_regs.Buffer;

    for (;;)
    {
        status = virtual_unwind( UNW_FLAG_UHANDLER, &dispatch, &new_context );
        if (status != STATUS_SUCCESS) raise_status( status, rec );

    unwind_done:
        if (!dispatch.EstablisherFrame) break;

        if (!is_valid_arm64ec_frame( dispatch.EstablisherFrame ))
        {
            ERR( "invalid frame %I64x (%p-%p)\n", dispatch.EstablisherFrame,
                 NtCurrentTeb()->Tib.StackLimit, NtCurrentTeb()->Tib.StackBase );
            rec->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            break;
        }

        if (dispatch.LanguageHandler)
        {
            if (end_frame && (dispatch.EstablisherFrame > (ULONG64)end_frame))
            {
                ERR( "invalid end frame %I64x/%p\n", dispatch.EstablisherFrame, end_frame );
                raise_status( STATUS_INVALID_UNWIND_TARGET, rec );
            }
            if (dispatch.EstablisherFrame == (ULONG64)end_frame) rec->ExceptionFlags |= EXCEPTION_TARGET_UNWIND;

            TRACE( "calling handler %p (rec=%p, frame=%I64x context=%p, dispatch=%p)\n",
                   dispatch.LanguageHandler, rec, dispatch.EstablisherFrame,
                   dispatch.ContextRecord, &dispatch );
            res = call_unwind_handler( rec, dispatch.EstablisherFrame, dispatch.ContextRecord,
                                       &dispatch, dispatch.LanguageHandler );
            TRACE( "handler %p returned %lx\n", dispatch.LanguageHandler, res );

            switch (res)
            {
            case ExceptionContinueSearch:
                rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                break;
            case ExceptionCollidedUnwind:
                new_context.AMD64_Context = *context;
                RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase, dispatch.ControlPc,
                                  dispatch.FunctionEntry, &new_context.AMD64_Context,
                                  &dispatch.HandlerData, &frame, NULL );
                rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                goto unwind_done;
            default:
                raise_status( STATUS_INVALID_DISPOSITION, rec );
                break;
            }
        }
        else  /* hack: call builtin handlers registered in the tib list */
        {
            while (is_valid_arm64ec_frame( (ULONG_PTR)teb_frame ) &&
                   (ULONG64)teb_frame < new_context.Sp &&
                   (ULONG64)teb_frame < (ULONG64)end_frame)
            {
                TRACE( "calling TEB handler %p (rec=%p, frame=%p context=%p, dispatch=%p)\n",
                       teb_frame->Handler, rec, teb_frame, dispatch.ContextRecord, &dispatch );
                res = call_unwind_handler( rec, (ULONG_PTR)teb_frame, dispatch.ContextRecord, &dispatch,
                                           (PEXCEPTION_ROUTINE)teb_frame->Handler );
                TRACE( "handler at %p returned %lu\n", teb_frame->Handler, res );
                teb_frame = __wine_pop_frame( teb_frame );

                switch (res)
                {
                case ExceptionContinueSearch:
                    rec->ExceptionFlags &= ~EXCEPTION_COLLIDED_UNWIND;
                    break;
                case ExceptionCollidedUnwind:
                    new_context.AMD64_Context = *context;
                    RtlVirtualUnwind( UNW_FLAG_NHANDLER, dispatch.ImageBase, dispatch.ControlPc,
                                      dispatch.FunctionEntry, &new_context.AMD64_Context,
                                      &dispatch.HandlerData, &frame, NULL );
                    rec->ExceptionFlags |= EXCEPTION_COLLIDED_UNWIND;
                    goto unwind_done;
                default:
                    raise_status( STATUS_INVALID_DISPOSITION, rec );
                    break;
                }
            }
            if ((ULONG64)teb_frame == (ULONG64)end_frame && (ULONG64)end_frame < new_context.Sp) break;
        }

        if (dispatch.EstablisherFrame == (ULONG64)end_frame)
        {
            if (is_target_ec) break;
            if (!RtlIsEcCode( dispatch.ControlPc )) break;
            /* we just crossed into x64 code, unwind one more frame */
        }
        *context = new_context.AMD64_Context;
    }

    if (rec->ExceptionCode != STATUS_UNWIND_CONSOLIDATE)
        context->Rip = (ULONG64)target_ip;
    else if (rec->ExceptionInformation[10] == -1)
        rec->ExceptionInformation[10] = (ULONG_PTR)&nonvol_regs;

    if (is_target_ec) context->Rcx = (ULONG64)retval;
    else context->Rax = (ULONG64)retval;
    RtlRestoreContext( context, rec );
}


/*************************************************************************
 *		RtlWalkFrameChain (NTDLL.@)
 */
ULONG WINAPI RtlWalkFrameChain( void **buffer, ULONG count, ULONG flags )
{
    UNWIND_HISTORY_TABLE table;
    RUNTIME_FUNCTION *func;
    PEXCEPTION_ROUTINE handler;
    ULONG_PTR pc, frame, base;
    CONTEXT context;
    void *data;
    ULONG i, skip = flags >> 8, num_entries = 0;

    RtlCaptureContext( &context );

    for (i = 0; i < count; i++)
    {
        pc = context.Rip;
        if ((context.ContextFlags & CONTEXT_UNWOUND_TO_CALL) && RtlIsEcCode( pc )) pc -= 4;
        func = RtlLookupFunctionEntry( pc, &base, &table );
        if (RtlVirtualUnwind2( UNW_FLAG_NHANDLER, base, pc, func, &context, NULL,
                               &data, &frame, NULL, NULL, NULL, &handler, 0 ))
            break;
        if (!context.Rip) break;
        if (!frame || !is_valid_frame( frame )) break;
        if (context.Rsp == (ULONG_PTR)NtCurrentTeb()->Tib.StackBase) break;
        if (i >= skip) buffer[num_entries++] = (void *)context.Rip;
    }
    return num_entries;
}


static int code_match( BYTE *code, const BYTE *seq, size_t len )
{
    for ( ; len; len--, code++, seq++) if (*seq && *code != *seq) return 0;
    return 1;
}

void *check_call( void **target, void *exit_thunk, void *dest )
{
    static const BYTE jmp_sequence[] =
    {
        0xff, 0x25              /* jmp *xxx(%rip) */
    };
    static const BYTE fast_forward_sequence[] =
    {
        0x48, 0x8b, 0xc4,       /* mov  %rsp,%rax */
        0x48, 0x89, 0x58, 0x20, /* mov  %rbx,0x20(%rax) */
        0x55,                   /* push %rbp */
        0x5d,                   /* pop  %rbp */
        0xe9                    /* jmp  arm_code */
    };
    static const BYTE syscall_sequence[] =
    {
        0x4c, 0x8b, 0xd1,       /* mov  %rcx,%r10 */
        0xb8, 0, 0, 0, 0,       /* mov  $xxx,%eax */
        0xf6, 0x04, 0x25, 0x08, /* testb $0x1,0x7ffe0308 */
        0x03, 0xfe, 0x7f, 0x01,
        0x75, 0x03,             /* jne 1f */
        0x0f, 0x05,             /* syscall */
        0xc3,                   /* ret */
        0xcd, 0x2e,             /* 1: int $0x2e */
        0xc3                    /* ret */
    };

    for (;;)
    {
        if (dest == __wine_unix_call_dispatcher) return dest;
        if (RtlIsEcCode( (ULONG_PTR)dest )) return dest;
        if (code_match( dest, jmp_sequence, sizeof(jmp_sequence) ))
        {
            int *off_ptr = (int *)((char *)dest + sizeof(jmp_sequence));
            void **addr_ptr = (void **)((char *)(off_ptr + 1) + *off_ptr);
            dest = *addr_ptr;
            continue;
        }
        if (!((ULONG_PTR)dest & 15))  /* fast-forward and syscall thunks are always aligned */
        {
            if (code_match( dest, fast_forward_sequence, sizeof(fast_forward_sequence) ))
            {
                int *off_ptr = (int *)((char *)dest + sizeof(fast_forward_sequence));
                return (char *)(off_ptr + 1) + *off_ptr;
            }
            if (code_match( dest, syscall_sequence, sizeof(syscall_sequence) ))
            {
                ULONG id = ((ULONG *)dest)[1];
                if (id < ARRAY_SIZE(arm64ec_syscalls)) return arm64ec_syscalls[id];
            }
        }
        *target = dest;
        return exit_thunk;
    }
}

static void __attribute__((naked)) arm64x_check_call(void)
{
    asm( "stp x29, x30, [sp,#-0xb0]!\n\t"
         "mov x29, sp\n\t"
         "stp x0, x1,   [sp, #0x10]\n\t"
         "stp x2, x3,   [sp, #0x20]\n\t"
         "stp x4, x5,   [sp, #0x30]\n\t"
         "stp x6, x7,   [sp, #0x40]\n\t"
         "stp x8, x9,   [sp, #0x50]\n\t"
         "stp x10, x15, [sp, #0x60]\n\t"
         "stp d0, d1,   [sp, #0x70]\n\t"
         "stp d2, d3,   [sp, #0x80]\n\t"
         "stp d4, d5,   [sp, #0x90]\n\t"
         "stp d6, d7,   [sp, #0xa0]\n\t"
         "add x0, sp, #0x58\n\t"  /* x9 = &target */
         "mov x1, x10\n\t"        /* x10 = exit_thunk */
         "mov x2, x11\n\t"        /* x11 = dest */
         "bl " __ASM_NAME("check_call") "\n\t"
         "mov x11, x0\n\t"
         "ldp x0, x1,   [sp, #0x10]\n\t"
         "ldp x2, x3,   [sp, #0x20]\n\t"
         "ldp x4, x5,   [sp, #0x30]\n\t"
         "ldp x6, x7,   [sp, #0x40]\n\t"
         "ldp x8, x9,   [sp, #0x50]\n\t"
         "ldp x10, x15, [sp, #0x60]\n\t"
         "ldp d0, d1,   [sp, #0x70]\n\t"
         "ldp d2, d3,   [sp, #0x80]\n\t"
         "ldp d4, d5,   [sp, #0x90]\n\t"
         "ldp d6, d7,   [sp, #0xa0]\n\t"
         "ldp x29, x30, [sp], #0xb0\n\t"
         "ret" );
}


/**************************************************************************
 *		__chkstk (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
void __attribute__((naked)) __chkstk(void)
{
    asm( "ret" );
}


/**************************************************************************
 *		__chkstk_arm64ec (NTDLL.@)
 *
 * Supposed to touch all the stack pages, but we shouldn't need that.
 */
void __attribute__((naked)) __chkstk_arm64ec(void)
{
    asm( "ret" );
}


/*******************************************************************
 *		__C_ExecuteExceptionFilter
 */
LONG __attribute__((naked)) __C_ExecuteExceptionFilter( EXCEPTION_POINTERS *ptrs, void *frame,
                                                        PEXCEPTION_FILTER filter, BYTE *nonvolatile )
{
    asm( ".seh_proc _C_ExecuteExceptionFilter\n\t"
         "stp x29, x30, [sp, #-80]!\n\t"
         ".seh_save_fplr_x 80\n\t"
         "stp x19, x20, [sp, #16]\n\t"
         ".seh_save_regp x19, 16\n\t"
         "stp x21, x22, [sp, #32]\n\t"
         ".seh_save_regp x21, 32\n\t"
         "stp x25, x26, [sp, #48]\n\t"
         ".seh_save_regp x25, 48\n\t"
         "str x27, [sp, #64]\n\t"
         ".seh_save_reg x27, 64\n\t"
         ".seh_endprologue\n\t"
         "ldp x19, x20, [x3, #0]\n\t" /* nonvolatile regs */
         "ldp x21, x22, [x3, #16]\n\t"
         "ldp x25, x26, [x3, #48]\n\t"
         "ldr x27, [x3, #64]\n\t"
         "ldr x1,  [x3, #80]\n\t"     /* x29 = frame */
         "blr x2\n\t"                 /* filter */
         "ldp x19, x20, [sp, #16]\n\t"
         "ldp x21, x22, [sp, #32]\n\t"
         "ldp x25, x26, [sp, #48]\n\t"
         "ldr x27,      [sp, #64]\n\t"
         "ldp x29, x30, [sp], #80\n\t"
         "ret\n\t"
         ".seh_endproc" );
}


/***********************************************************************
 *		RtlRaiseException (NTDLL.@)
 */
void __attribute((naked)) RtlRaiseException( EXCEPTION_RECORD *rec )
{
    asm( ".seh_proc RtlRaiseException\n\t"
         "sub sp, sp, #0x4d0\n\t"     /* sizeof(context) */
         ".seh_stackalloc 0x4d0\n\t"
         "stp x29, x30, [sp, #-0x20]!\n\t"
         ".seh_save_fplr_x 0x20\n\t"
         "str x0, [sp, #0x10]\n\t"
         ".seh_save_any_reg x0, 0x10\n\t"
         ".seh_endprologue\n\t"
         "add x0, sp, #0x20\n\t"
         "bl RtlCaptureContext\n\t"
         "add x1, sp, #0x20\n\t"       /* context pointer */
         "ldr x0, [sp, #0x10]\n\t"     /* rec */
         "ldr x2, [x1, #0xf8]\n\t"     /* context->Rip */
         "str x2, [x0, #0x10]\n\t"     /* rec->ExceptionAddress */
         "ldr w2, [x1, #0x30]\n\t"     /* context->ContextFlags */
         "orr w2, w2, #0x20000000\n\t" /* CONTEXT_UNWOUND_TO_CALL */
         "str w2, [x1, #0x30]\n\t"
         "ldr x3, [x18, #0x60]\n\t"    /* peb */
         "ldrb w2, [x3, #2]\n\t"       /* peb->BeingDebugged */
         "cbnz w2, 1f\n\t"
         "bl dispatch_exception\n"
         "1:\tmov w2, #1\n\t"
         "bl NtRaiseException\n\t"
         "b RtlRaiseStatus\n\t" /* does not return */
         ".seh_endproc" );
}


/*******************************************************************
 *		longjmp (NTDLL.@)
 */
void __cdecl NTDLL_longjmp( _JUMP_BUFFER *buf, int retval )
{
    EXCEPTION_RECORD rec;

    if (!retval) retval = 1;

    rec.ExceptionCode = STATUS_LONGJUMP;
    rec.ExceptionFlags = 0;
    rec.ExceptionRecord = NULL;
    rec.ExceptionAddress = NULL;
    rec.NumberParameters = 1;
    rec.ExceptionInformation[0] = (DWORD_PTR)buf;
    RtlUnwind( (void *)buf->Frame, (void *)buf->Rip, &rec, IntToPtr(retval) );
}


/***********************************************************************
 *           RtlUserThreadStart (NTDLL.@)
 */
void __attribute__((naked)) RtlUserThreadStart( PRTL_THREAD_START_ROUTINE entry, void *arg )
{
    asm( ".seh_proc RtlUserThreadStart\n\t"
         "stp x29, x30, [sp, #-16]!\n\t"
         ".seh_save_fplr_x 16\n\t"
         ".seh_endprologue\n\t"
         "adrp x11, pBaseThreadInitThunk\n\t"
         "ldr x11, [x11, #:lo12:pBaseThreadInitThunk]\n\t"
         "adr x10, $iexit_thunk$cdecl$v$i8i8i8\n\t"
         "mov x2, x1\n\t"
         "mov x1, x0\n\t"
         "mov x0, #0\n\t"
         "adrp x16, __os_arm64x_dispatch_icall\n\t"
         "ldr x16, [x16, #:lo12:__os_arm64x_dispatch_icall]\n\t"
         "blr x16\n\t"
         "blr x11\n\t"
         "brk #1\n\t"
         ".seh_handler call_unhandled_exception_handler, @except\n\t"
         ".seh_endproc" );
}


/******************************************************************
 *		LdrInitializeThunk (NTDLL.@)
 */
void WINAPI LdrInitializeThunk( CONTEXT *arm_context, ULONG_PTR unk2, ULONG_PTR unk3, ULONG_PTR unk4 )
{
    ARM64EC_NT_CONTEXT context;

    if (!__os_arm64x_check_call)
    {
        __os_arm64x_check_call = arm64x_check_call;
        __os_arm64x_check_icall = arm64x_check_call;
        __os_arm64x_check_icall_cfg = arm64x_check_call;
        __os_arm64x_get_x64_information = LdrpGetX64Information;
        __os_arm64x_set_x64_information = LdrpSetX64Information;
    }

    context_arm_to_x64( &context, (ARM64_NT_CONTEXT *)arm_context );
    loader_init( &context.AMD64_Context, (void **)&context.X0 );
    TRACE_(relay)( "\1Starting thread proc %p (arg=%p)\n", (void *)context.X0, (void *)context.X1 );
    NtContinue( &context.AMD64_Context, TRUE );
}


/***********************************************************************
 *           process_breakpoint
 */
__ASM_GLOBAL_FUNC( "#process_breakpoint",
                   ".seh_endprologue\n\t"
                   ".seh_handler process_breakpoint_handler, @except\n\t"
                   "brk #0xf000\n\t"
                   "ret\n"
                   "process_breakpoint_handler:\n\t"
                   "ldr x4, [x2, #0x108]\n\t" /* context->Pc */
                   "add x4, x4, #4\n\t"
                   "str x4, [x2, #0x108]\n\t"
                   "mov w0, #0\n\t"           /* ExceptionContinueExecution */
                   "ret" )


/***********************************************************************
 *		DbgUiRemoteBreakin   (NTDLL.@)
 */
void __attribute__((naked)) DbgUiRemoteBreakin( void *arg )
{
    asm( ".seh_proc DbgUiRemoteBreakin\n\t"
         "stp x29, x30, [sp, #-16]!\n\t"
         ".seh_save_fplr_x 16\n\t"
         ".seh_endprologue\n\t"
         ".seh_handler DbgUiRemoteBreakin_handler, @except\n\t"
         "ldr x0, [x18, #0x60]\n\t"  /* NtCurrentTeb()->Peb */
         "ldrb w0, [x0, 0x02]\n\t"   /* peb->BeingDebugged */
         "cbz w0, 1f\n\t"
         "bl DbgBreakPoint\n"
         "1:\tmov w0, #0\n\t"
         "bl RtlExitUserThread\n"
         "DbgUiRemoteBreakin_handler:\n\t"
         "mov sp, x1\n\t"            /* frame */
         "b 1b\n\t"
         ".seh_endproc" );
}


/**********************************************************************
 *              DbgBreakPoint   (NTDLL.@)
 */
void __attribute__((naked)) DbgBreakPoint(void)
{
    asm( ".seh_proc DbgBreakPoint\n\t"
         ".seh_endprologue\n\t"
         "brk #0xf000\n\t"
         "ret\n\t"
         ".seh_endproc" );
}


/**********************************************************************
 *              DbgUserBreakPoint   (NTDLL.@)
 */
void __attribute__((naked)) DbgUserBreakPoint(void)
{
    asm( ".seh_proc DbgUserBreakPoint\n\t"
         ".seh_endprologue\n\t"
         "brk #0xf000\n\t"
         "ret\n\t"
         ".seh_endproc" );
}

#endif  /* __arm64ec__ */
