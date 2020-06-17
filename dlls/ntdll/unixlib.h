/*
 * Ntdll Unix interface
 *
 * Copyright (C) 2020 Alexandre Julliard
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

#ifndef __NTDLL_UNIXLIB_H
#define __NTDLL_UNIXLIB_H

#include "wine/server.h"
#include "wine/debug.h"

struct ldt_copy;
struct msghdr;

/* increment this when you change the function table */
#define NTDLL_UNIXLIB_VERSION 52

struct unix_funcs
{
    /* Nt* functions */
    NTSTATUS      (WINAPI *NtAlertResumeThread)( HANDLE handle, ULONG *count );
    NTSTATUS      (WINAPI *NtAlertThread)( HANDLE handle );
    NTSTATUS      (WINAPI *NtAllocateVirtualMemory)( HANDLE process, PVOID *ret, ULONG_PTR zero_bits,
                                                     SIZE_T *size_ptr, ULONG type, ULONG protect );
    NTSTATUS      (WINAPI *NtAreMappedFilesTheSame)(PVOID addr1, PVOID addr2);
    NTSTATUS      (WINAPI *NtAssignProcessToJobObject)( HANDLE job, HANDLE process );
    NTSTATUS      (WINAPI *NtCancelTimer)( HANDLE handle, BOOLEAN *state );
    NTSTATUS      (WINAPI *NtClearEvent)( HANDLE handle );
    NTSTATUS      (WINAPI *NtClose)( HANDLE handle );
    NTSTATUS      (WINAPI *NtContinue)( CONTEXT *context, BOOLEAN alertable );
    NTSTATUS      (WINAPI *NtCreateEvent)( HANDLE *handle, ACCESS_MASK access,
                                           const OBJECT_ATTRIBUTES *attr, EVENT_TYPE type, BOOLEAN state );
    NTSTATUS      (WINAPI *NtCreateFile)( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                          IO_STATUS_BLOCK *io, LARGE_INTEGER *alloc_size,
                                          ULONG attributes, ULONG sharing, ULONG disposition,
                                          ULONG options, void *ea_buffer, ULONG ea_length );
    NTSTATUS      (WINAPI *NtCreateIoCompletion)( HANDLE *handle, ACCESS_MASK access,
                                                  OBJECT_ATTRIBUTES *attr, ULONG threads );
    NTSTATUS      (WINAPI *NtCreateJobObject)( HANDLE *handle, ACCESS_MASK access,
                                               const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtCreateKeyedEvent)( HANDLE *handle, ACCESS_MASK access,
                                                const OBJECT_ATTRIBUTES *attr, ULONG flags );
    NTSTATUS      (WINAPI *NtCreateMailslotFile)( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                                  IO_STATUS_BLOCK *io, ULONG options, ULONG quota,
                                                  ULONG msg_size, LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtCreateMutant)( HANDLE *handle, ACCESS_MASK access,
                                            const OBJECT_ATTRIBUTES *attr, BOOLEAN owned );
    NTSTATUS      (WINAPI *NtCreateNamedPipeFile)( HANDLE *handle, ULONG access, OBJECT_ATTRIBUTES *attr,
                                                   IO_STATUS_BLOCK *io, ULONG sharing, ULONG dispo,
                                                   ULONG options, ULONG pipe_type, ULONG read_mode,
                                                   ULONG completion_mode, ULONG max_inst,
                                                   ULONG inbound_quota, ULONG outbound_quota,
                                                   LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtCreateSection)( HANDLE *handle, ACCESS_MASK access,
                                             const OBJECT_ATTRIBUTES *attr, const LARGE_INTEGER *size,
                                             ULONG protect, ULONG sec_flags, HANDLE file );
    NTSTATUS      (WINAPI *NtCreateSemaphore)( HANDLE *handle, ACCESS_MASK access,
                                               const OBJECT_ATTRIBUTES *attr, LONG initial, LONG max );
    NTSTATUS      (WINAPI *NtCreateThreadEx)( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                              HANDLE process, PRTL_THREAD_START_ROUTINE start, void *param,
                                              ULONG flags, SIZE_T zero_bits, SIZE_T stack_commit,
                                              SIZE_T stack_reserve, PS_ATTRIBUTE_LIST *attr_list );
    NTSTATUS      (WINAPI *NtCreateTimer)( HANDLE *handle, ACCESS_MASK access,
                                           const OBJECT_ATTRIBUTES *attr, TIMER_TYPE type );
    NTSTATUS      (WINAPI *NtCreateUserProcess)( HANDLE *process_handle_ptr, HANDLE *thread_handle_ptr,
                                                 ACCESS_MASK process_access, ACCESS_MASK thread_access,
                                                 OBJECT_ATTRIBUTES *process_attr, OBJECT_ATTRIBUTES *thread_attr,
                                                 ULONG process_flags, ULONG thread_flags,
                                                 RTL_USER_PROCESS_PARAMETERS *params, PS_CREATE_INFO *info,
                                                 PS_ATTRIBUTE_LIST *attr );
    TEB *         (WINAPI *NtCurrentTeb)(void);
    NTSTATUS      (WINAPI *NtDelayExecution)( BOOLEAN alertable, const LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtDeleteFile)( OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtDeviceIoControlFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                                   void *apc_context, IO_STATUS_BLOCK *io, ULONG code,
                                                   void *in_buffer, ULONG in_size,
                                                   void *out_buffer, ULONG out_size );
    NTSTATUS      (WINAPI *NtDuplicateObject)( HANDLE source_process, HANDLE source,
                                               HANDLE dest_process, HANDLE *dest,
                                               ACCESS_MASK access, ULONG attributes, ULONG options );
    NTSTATUS      (WINAPI *NtFlushBuffersFile)( HANDLE handle, IO_STATUS_BLOCK *io );
    NTSTATUS      (WINAPI *NtFlushVirtualMemory)( HANDLE process, LPCVOID *addr_ptr,
                                                  SIZE_T *size_ptr, ULONG unknown );
    NTSTATUS      (WINAPI *NtFreeVirtualMemory)( HANDLE process, PVOID *addr_ptr,
                                                 SIZE_T *size_ptr, ULONG type );
    NTSTATUS      (WINAPI *NtFsControlFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc,
                                             void *apc_context, IO_STATUS_BLOCK *io, ULONG code,
                                             void *in_buffer, ULONG in_size,
                                             void *out_buffer, ULONG out_size );
    NTSTATUS      (WINAPI *NtGetContextThread)( HANDLE handle, CONTEXT *context );
    NTSTATUS      (WINAPI *NtGetWriteWatch)( HANDLE process, ULONG flags, PVOID base, SIZE_T size,
                                             PVOID *addresses, ULONG_PTR *count, ULONG *granularity );
    NTSTATUS      (WINAPI *NtIsProcessInJob)( HANDLE process, HANDLE job );
    NTSTATUS      (WINAPI *NtLockVirtualMemory)( HANDLE process, PVOID *addr, SIZE_T *size, ULONG unknown );
    NTSTATUS      (WINAPI *NtMapViewOfSection)( HANDLE handle, HANDLE process, PVOID *addr_ptr,
                                                ULONG_PTR zero_bits, SIZE_T commit_size,
                                                const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr,
                                                SECTION_INHERIT inherit, ULONG alloc_type, ULONG protect );
    NTSTATUS      (WINAPI *NtOpenEvent)( HANDLE *handle, ACCESS_MASK access,
                                         const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenFile)( HANDLE *handle, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr,
                                        IO_STATUS_BLOCK *io, ULONG sharing, ULONG options );
    NTSTATUS      (WINAPI *NtOpenIoCompletion)( HANDLE *handle, ACCESS_MASK access,
                                                const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenJobObject)( HANDLE *handle, ACCESS_MASK access,
                                             const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenKeyedEvent)( HANDLE *handle, ACCESS_MASK access,
                                              const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenMutant)( HANDLE *handle, ACCESS_MASK access,
                                          const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenSection)( HANDLE *handle, ACCESS_MASK access,
                                           const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenSemaphore)( HANDLE *handle, ACCESS_MASK access,
                                             const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtOpenThread)( HANDLE *handle, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr, const CLIENT_ID *id );
    NTSTATUS      (WINAPI *NtOpenTimer)( HANDLE *handle, ACCESS_MASK access,
                                         const OBJECT_ATTRIBUTES *attr );
    NTSTATUS      (WINAPI *NtProtectVirtualMemory)( HANDLE process, PVOID *addr_ptr, SIZE_T *size_ptr,
                                                    ULONG new_prot, ULONG *old_prot );
    NTSTATUS      (WINAPI *NtPulseEvent)( HANDLE handle, LONG *prev_state );
    NTSTATUS      (WINAPI *NtQueryAttributesFile)( const OBJECT_ATTRIBUTES *attr,
                                                   FILE_BASIC_INFORMATION *info );
    NTSTATUS      (WINAPI *NtQueryDirectoryFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc_routine,
                                                  void *apc_context, IO_STATUS_BLOCK *io, void *buffer,
                                                  ULONG length, FILE_INFORMATION_CLASS info_class,
                                                  BOOLEAN single_entry, UNICODE_STRING *mask,
                                                  BOOLEAN restart_scan );
    NTSTATUS      (WINAPI *NtQueryEvent)( HANDLE handle, EVENT_INFORMATION_CLASS class,
                                          void *info, ULONG len, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQueryFullAttributesFile)( const OBJECT_ATTRIBUTES *attr,
                                                       FILE_NETWORK_OPEN_INFORMATION *info );
    NTSTATUS      (WINAPI *NtQueryInformationFile)( HANDLE hFile, IO_STATUS_BLOCK *io,
                                                    void *ptr, LONG len, FILE_INFORMATION_CLASS class );
    NTSTATUS      (WINAPI *NtQueryInformationJobObject)( HANDLE handle, JOBOBJECTINFOCLASS class,
                                                         void *info, ULONG len, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQueryInformationProcess)( HANDLE handle, PROCESSINFOCLASS class, void *info,
                                                       ULONG size, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQueryIoCompletion)( HANDLE handle, IO_COMPLETION_INFORMATION_CLASS class,
                                                 void *buffer, ULONG len, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQueryMutant)( HANDLE handle, MUTANT_INFORMATION_CLASS class,
                                           void *info, ULONG len, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQueryPerformanceCounter)( LARGE_INTEGER *counter, LARGE_INTEGER *frequency );
    NTSTATUS      (WINAPI *NtQuerySection)( HANDLE handle, SECTION_INFORMATION_CLASS class,
                                            void *ptr, SIZE_T size, SIZE_T *ret_size );
    NTSTATUS      (WINAPI *NtQuerySemaphore)( HANDLE handle, SEMAPHORE_INFORMATION_CLASS class,
                                              void *info, ULONG len, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQuerySystemTime)( LARGE_INTEGER *time );
    NTSTATUS      (WINAPI *NtQueryTimer)( HANDLE handle, TIMER_INFORMATION_CLASS class,
                                          void *info, ULONG len, ULONG *ret_len );
    NTSTATUS      (WINAPI *NtQueryVirtualMemory)( HANDLE process, LPCVOID addr,
                                                  MEMORY_INFORMATION_CLASS info_class,
                                                  PVOID buffer, SIZE_T len, SIZE_T *res_len );
    NTSTATUS      (WINAPI *NtQueueApcThread)( HANDLE handle, PNTAPCFUNC func, ULONG_PTR arg1,
                                              ULONG_PTR arg2, ULONG_PTR arg3 );
    NTSTATUS      (WINAPI *NtRaiseException)( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance );
    NTSTATUS      (WINAPI *NtReadFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                        IO_STATUS_BLOCK *io, void *buffer, ULONG length,
                                        LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtReadFileScatter)( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                               void *apc_user, IO_STATUS_BLOCK *io,
                                               FILE_SEGMENT_ELEMENT *segments, ULONG length,
                                               LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtReadVirtualMemory)( HANDLE process, const void *addr, void *buffer,
                                                 SIZE_T size, SIZE_T *bytes_read );
    NTSTATUS      (WINAPI *NtReleaseKeyedEvent)( HANDLE handle, const void *key,
                                                 BOOLEAN alertable, const LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtReleaseMutant)( HANDLE handle, LONG *prev_count );
    NTSTATUS      (WINAPI *NtReleaseSemaphore)( HANDLE handle, ULONG count, ULONG *previous );
    NTSTATUS      (WINAPI *NtRemoveIoCompletion)( HANDLE handle, ULONG_PTR *key, ULONG_PTR *value,
                                                  IO_STATUS_BLOCK *io, LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtRemoveIoCompletionEx)( HANDLE handle, FILE_IO_COMPLETION_INFORMATION *info,
                                                    ULONG count, ULONG *written,
                                                    LARGE_INTEGER *timeout, BOOLEAN alertable );
    NTSTATUS      (WINAPI *NtResetEvent)( HANDLE handle, LONG *prev_state );
    NTSTATUS      (WINAPI *NtResetWriteWatch)( HANDLE process, PVOID base, SIZE_T size );
    NTSTATUS      (WINAPI *NtResumeThread)( HANDLE handle, ULONG *count );
    NTSTATUS      (WINAPI *NtSetContextThread)( HANDLE handle, const CONTEXT *context );
    NTSTATUS      (WINAPI *NtSetEvent)( HANDLE handle, LONG *prev_state );
    NTSTATUS      (WINAPI *NtSetInformationFile)( HANDLE handle, IO_STATUS_BLOCK *io,
                                                  void *ptr, ULONG len, FILE_INFORMATION_CLASS class );
    NTSTATUS      (WINAPI *NtSetInformationJobObject)( HANDLE handle, JOBOBJECTINFOCLASS class,
                                                       void *info, ULONG len );
    NTSTATUS      (WINAPI *NtSetInformationProcess)( HANDLE handle, PROCESSINFOCLASS class,
                                                     void *info, ULONG size );
    NTSTATUS      (WINAPI *NtSetIoCompletion)( HANDLE handle, ULONG_PTR key, ULONG_PTR value,
                                               NTSTATUS status, SIZE_T count );
    NTSTATUS      (WINAPI *NtSetLdtEntries)( ULONG sel1, LDT_ENTRY entry1, ULONG sel2, LDT_ENTRY entry2 );
    NTSTATUS      (WINAPI *NtSetSystemTime)( const LARGE_INTEGER *new, LARGE_INTEGER *old );
    NTSTATUS      (WINAPI *NtSetTimer)( HANDLE handle, const LARGE_INTEGER *when,
                                        PTIMER_APC_ROUTINE callback, void *arg,
                                        BOOLEAN resume, ULONG period, BOOLEAN *state );
    NTSTATUS      (WINAPI *NtSignalAndWaitForSingleObject)( HANDLE signal, HANDLE wait,
                                                            BOOLEAN alertable, const LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtSuspendThread)( HANDLE handle, ULONG *count );
    NTSTATUS      (WINAPI *NtTerminateJobObject)( HANDLE handle, NTSTATUS status );
    NTSTATUS      (WINAPI *NtTerminateProcess)( HANDLE handle, LONG exit_code );
    NTSTATUS      (WINAPI *NtTerminateThread)( HANDLE handle, LONG exit_code );
    NTSTATUS      (WINAPI *NtUnlockVirtualMemory)( HANDLE process, PVOID *addr,
                                                   SIZE_T *size, ULONG unknown );
    NTSTATUS      (WINAPI *NtUnmapViewOfSection)( HANDLE process, PVOID addr );
    NTSTATUS      (WINAPI *NtWaitForKeyedEvent)( HANDLE handle, const void *key, BOOLEAN alertable,
                                                 const LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtWaitForMultipleObjects)( DWORD count, const HANDLE *handles,
                                                      BOOLEAN wait_any, BOOLEAN alertable,
                                                      const LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtWaitForSingleObject)( HANDLE handle, BOOLEAN alertable,
                                                   const LARGE_INTEGER *timeout );
    NTSTATUS      (WINAPI *NtWriteFile)( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                         IO_STATUS_BLOCK *io, const void *buffer, ULONG length,
                                         LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtWriteFileGather)( HANDLE file, HANDLE event, PIO_APC_ROUTINE apc,
                                               void *apc_user, IO_STATUS_BLOCK *io,
                                               FILE_SEGMENT_ELEMENT *segments, ULONG length,
                                               LARGE_INTEGER *offset, ULONG *key );
    NTSTATUS      (WINAPI *NtWriteVirtualMemory)( HANDLE process, void *addr, const void *buffer,
                                                  SIZE_T size, SIZE_T *bytes_written );
    NTSTATUS      (WINAPI *NtYieldExecution)(void);

    /* other Win32 API functions */
    NTSTATUS      (WINAPI *DbgUiIssueRemoteBreakin)( HANDLE process );
    NTSTATUS      (WINAPI *RtlWaitOnAddress)( const void *addr, const void *cmp, SIZE_T size,
                                              const LARGE_INTEGER *timeout );
    void          (WINAPI *RtlWakeAddressAll)( const void *addr );
    void          (WINAPI *RtlWakeAddressSingle)( const void *addr );

    /* fast locks */
    NTSTATUS      (CDECL *fast_RtlpWaitForCriticalSection)( RTL_CRITICAL_SECTION *crit, int timeout );
    NTSTATUS      (CDECL *fast_RtlpUnWaitCriticalSection)( RTL_CRITICAL_SECTION *crit );
    NTSTATUS      (CDECL *fast_RtlDeleteCriticalSection)( RTL_CRITICAL_SECTION *crit );
    NTSTATUS      (CDECL *fast_RtlTryAcquireSRWLockExclusive)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlAcquireSRWLockExclusive)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlTryAcquireSRWLockShared)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlAcquireSRWLockShared)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlReleaseSRWLockExclusive)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlReleaseSRWLockShared)( RTL_SRWLOCK *lock );
    NTSTATUS      (CDECL *fast_RtlSleepConditionVariableSRW)( RTL_CONDITION_VARIABLE *variable,
                                                              RTL_SRWLOCK *lock,
                                                              const LARGE_INTEGER *timeout, ULONG flags );
    NTSTATUS      (CDECL *fast_RtlSleepConditionVariableCS)( RTL_CONDITION_VARIABLE *variable,
                                                             RTL_CRITICAL_SECTION *cs,
                                                             const LARGE_INTEGER *timeout );
    NTSTATUS      (CDECL *fast_RtlWakeConditionVariable)( RTL_CONDITION_VARIABLE *variable, int count );

    /* environment functions */
    void          (CDECL *get_main_args)( int *argc, char **argv[], char **envp[] );
    NTSTATUS      (CDECL *get_initial_environment)( WCHAR **wargv[], WCHAR *env, SIZE_T *size );
    void          (CDECL *get_initial_directory)( UNICODE_STRING *dir );
    void          (CDECL *get_paths)( const char **builddir, const char **datadir, const char **configdir );
    void          (CDECL *get_dll_path)( const char ***paths, SIZE_T *maxlen );
    void          (CDECL *get_unix_codepage)( CPTABLEINFO *table );
    void          (CDECL *get_locales)( WCHAR *sys, WCHAR *user );
    const char *  (CDECL *get_version)(void);
    const char *  (CDECL *get_build_id)(void);
    void          (CDECL *get_host_version)( const char **sysname, const char **release );

    /* virtual memory functions */
    NTSTATUS      (CDECL *map_so_dll)( const IMAGE_NT_HEADERS *nt_descr, HMODULE module );
    NTSTATUS      (CDECL *virtual_map_section)( HANDLE handle, PVOID *addr_ptr, unsigned short zero_bits_64, SIZE_T commit_size,
                                                const LARGE_INTEGER *offset_ptr, SIZE_T *size_ptr, ULONG alloc_type,
                                                ULONG protect, pe_image_info_t *image_info );
    void          (CDECL *virtual_get_system_info)( SYSTEM_BASIC_INFORMATION *info );
    NTSTATUS      (CDECL *virtual_create_builtin_view)( void *module );
    NTSTATUS      (CDECL *virtual_alloc_thread_stack)( INITIAL_TEB *stack, SIZE_T reserve_size, SIZE_T commit_size, SIZE_T *pthread_size );
    ssize_t       (CDECL *virtual_locked_recvmsg)( int fd, struct msghdr *hdr, int flags );
    void          (CDECL *virtual_release_address_space)(void);
    void          (CDECL *virtual_set_large_address_space)(void);

    /* thread/process functions */
    TEB *         (CDECL *init_threading)( int *nb_threads_ptr, struct ldt_copy **ldt_copy, SIZE_T *size,
                                           BOOL *suspend, unsigned int *cpus, BOOL *wow64, timeout_t *start_time );
    void          (CDECL *exit_thread)( int status );
    void          (CDECL *exit_process)( int status );
    NTSTATUS      (CDECL *get_thread_ldt_entry)( HANDLE handle, void *data, ULONG len, ULONG *ret_len );
    NTSTATUS      (CDECL *exec_process)( const UNICODE_STRING *cmdline, const pe_image_info_t *pe_info );

    /* server functions */
    unsigned int  (CDECL *server_call)( void *req_ptr );
    void          (CDECL *server_send_fd)( int fd );
    int           (CDECL *server_get_unix_fd)( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                                               int *needs_close, enum server_fd_type *type, unsigned int *options );
    NTSTATUS      (CDECL *server_fd_to_handle)( int fd, unsigned int access, unsigned int attributes,
                                                HANDLE *handle );
    NTSTATUS      (CDECL *server_handle_to_fd)( HANDLE handle, unsigned int access, int *unix_fd,
                                                unsigned int *options );
    void          (CDECL *server_release_fd)( HANDLE handle, int unix_fd );
    void          (CDECL *server_init_process_done)( void *relay );

    /* file functions */
    NTSTATUS      (CDECL *nt_to_unix_file_name)( const UNICODE_STRING *nameW, ANSI_STRING *unix_name_ret,
                                                 UINT disposition, BOOLEAN check_case );
    NTSTATUS      (CDECL *unix_to_nt_file_name)( const ANSI_STRING *name, UNICODE_STRING *nt );
    void          (CDECL *set_show_dot_files)( BOOL enable );

    /* debugging functions */
    unsigned char (CDECL *dbg_get_channel_flags)( struct __wine_debug_channel *channel );
    const char *  (CDECL *dbg_strdup)( const char *str );
    int           (CDECL *dbg_output)( const char *str );
    int           (CDECL *dbg_header)( enum __wine_debug_class cls, struct __wine_debug_channel *channel,
                                       const char *function );
};

#endif /* __NTDLL_UNIXLIB_H */
